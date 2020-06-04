/******* BEGIN LICENSE BLOCK *******
 * Copyright 2020 Sander van Geloven for only the Nuspell integration
 *
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Initial Developers of the Original Code are Kevin Hendricks (MySpell)
 * and László Németh (Hunspell). Portions created by the Initial Developers
 * are Copyright (C) 2002-2005 the Initial Developers. All Rights Reserved.
 *
 * Contributor(s): Kevin Hendricks (kevin.hendricks@sympatico.ca)
 *                 David Einstein (deinst@world.std.com)
 *                 Michiel van Leeuwen (mvl@exedo.nl)
 *                 Caolan McNamara (cmc@openoffice.org)
 *                 László Németh (nemethl@gyorsposta.hu)
 *                 Davide Prina
 *                 Giuseppe Modugno
 *                 Gianluca Turconi
 *                 Simon Brouwer
 *                 Noll Janos
 *                 Biro Arpad
 *                 Goldman Eleonora
 *                 Sarlos Tamas
 *                 Bencsath Boldizsar
 *                 Halacsy Peter
 *                 Dvornik Laszlo
 *                 Gefferth Andras
 *                 Nagy Viktor
 *                 Varga Daniel
 *                 Chris Halls
 *                 Rene Engelhard
 *                 Bram Moolenaar
 *                 Dafydd Jones
 *                 Harri Pitkanen
 *                 Andras Timar
 *                 Tor Lillqvist
 *                 Jesper Kristensen (mail@jesperkristensen.dk)
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 ******* END LICENSE BLOCK *******/

#include "mozNuspell.h"
#include "nsReadableUtils.h"
#include "nsString.h"
#include "nsIObserverService.h"
#include "nsIDirectoryEnumerator.h"
#include "nsIFile.h"
#include "nsUnicharUtils.h"
#include "nsCRT.h"
#include "mozInlineSpellChecker.h"
#include <stdlib.h>
#include "nsIPrefBranch.h"
#include "nsNetUtil.h"
#include "mozilla/dom/ContentParent.h"
#include "mozilla/Components.h"
#include <sstream>
#include "nsReadLine.h"
#include "nsIInputStream.h"
#include "nsContentUtils.h"
#include "nsILoadInfo.h"
#include "nsNetUtil.h"

using mozilla::dom::ContentParent;
using namespace mozilla;

NS_IMPL_CYCLE_COLLECTING_ADDREF(mozNuspell)
NS_IMPL_CYCLE_COLLECTING_RELEASE(mozNuspell)

NS_INTERFACE_MAP_BEGIN(mozNuspell)
  NS_INTERFACE_MAP_ENTRY(mozISpellCheckingEngine)
  NS_INTERFACE_MAP_ENTRY(nsIObserver)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY(nsIMemoryReporter)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, mozISpellCheckingEngine)
  NS_INTERFACE_MAP_ENTRIES_CYCLE_COLLECTION(mozNuspell)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTION_WEAK(mozNuspell, mPersonalDictionary)

NS_IMPL_COMPONENT_FACTORY(mozNuspell) {
  auto nuspell = MakeRefPtr<mozNuspell>();
  if (NS_SUCCEEDED(nuspell->Init())) {
    return nuspell.forget().downcast<mozISpellCheckingEngine>();
  }
  return nullptr;
}

mozNuspell::mozNuspell() : mNuspell() {
#ifdef DEBUG
  // There must be only one instance of this class.
  static bool hasRun = false;
  MOZ_ASSERT(!hasRun);
  hasRun = true;
#endif
}

nsresult mozNuspell::Init() {
  printf("DEBUG Entering mozNuspell::Init()\n");
  LoadDictionaryList(false);

  nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
  if (obs) {
    obs->AddObserver(this, "profile-do-change", true);
    obs->AddObserver(this, "profile-after-change", true);
  }

  mozilla::RegisterWeakMemoryReporter(this);

  return NS_OK;
}

mozNuspell::~mozNuspell() {
  printf("DEBUG Entering mozNuspell::~mozNuspell()\n");
  mozilla::UnregisterWeakMemoryReporter(this);

  mPersonalDictionary = nullptr;
}

NS_IMETHODIMP
mozNuspell::GetDictionary(nsAString& aDictionary) {
  aDictionary = mDictionary;
  return NS_OK;
}

/* set the Dictionary.
 * This also Loads the dictionary and initializes the converter using the
 * dictionaries converter
 */
NS_IMETHODIMP
mozNuspell::SetDictionary(const nsAString& aDictionary) {
  printf("DEBUG Entering mozNuspell::SetDictionary(aDictionary:=\"%s\")\n", NS_ConvertUTF16toUTF8(aDictionary).get());
  if (aDictionary.IsEmpty()) {
    mNuspell = nuspell::Dictionary();
    mDictionary.Truncate();
    mAffixFileName.Truncate();
    mDecoder = nullptr;
    mEncoder = nullptr;

    return NS_OK;
  }

  nsIURI* affFile = mDictionaries.GetWeak(aDictionary);
  if (!affFile) {
    return NS_ERROR_FILE_NOT_FOUND;
  }

  nsAutoCString dictFileName, affFileName;

  nsresult rv = affFile->GetSpec(affFileName);
  NS_ENSURE_SUCCESS(rv, rv);

  if (mAffixFileName.Equals(affFileName)) {
    return NS_OK;
  }

  dictFileName = affFileName;
  int32_t dotPos = dictFileName.RFindChar('.');
  if (dotPos == -1) return NS_ERROR_FAILURE;

  dictFileName.SetLength(dotPos);
  dictFileName.AppendLiteral(".dic");

  // SetDictionary can be called multiple times, so we might have a
  // valid mNuspell instance which needs cleaned up.

  mDictionary = aDictionary;
  mAffixFileName = affFileName;

  //WIP BEGIN
  nsLineBuffer<char> mLineBuffer;
  nsCString aLine; //TODO Which is best? nsACString/nsCString/nsAString

  auto affStr = std::stringstream();
  nsCOMPtr<nsIURI> affUri;
  rv = NS_NewURI(getter_AddRefs(affUri), affFileName.get());
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIChannel> affChannel;
  rv = NS_NewChannel(getter_AddRefs(affChannel), affUri,
                        nsContentUtils::GetSystemPrincipal(),
                        nsILoadInfo::SEC_REQUIRE_SAME_ORIGIN_DATA_INHERITS,
                        nsIContentPolicy::TYPE_OTHER);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIInputStream> mAffStream;
  rv = affChannel->Open(getter_AddRefs(mAffStream));
  NS_ENSURE_SUCCESS(rv, rv);
  int affLines = 0;
  while (mAffStream) {
    bool ok;
    rv = NS_ReadLine(mAffStream.get(), &mLineBuffer, aLine, &ok);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!ok) {
      mAffStream = nullptr;
      break;
    }
    affStr << aLine.get() << "\r\n";
    ++affLines;
  }
  printf("DEBUG mozNuspell::SetDictionary Read %d lines affix file\n", affLines);
//  printf("DEBUG mozNuspell::SetDictionary Contents affix file:\n%s", affStr.str().c_str());

  auto dictStr = std::stringstream();
  nsCOMPtr<nsIURI> dictUri;
  rv = NS_NewURI(getter_AddRefs(dictUri), dictFileName.get());
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIChannel> dictChannel;
  rv = NS_NewChannel(getter_AddRefs(dictChannel), dictUri,
                        nsContentUtils::GetSystemPrincipal(),
                        nsILoadInfo::SEC_REQUIRE_SAME_ORIGIN_DATA_INHERITS,
                        nsIContentPolicy::TYPE_OTHER);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIInputStream> mdictStream;
  rv = dictChannel->Open(getter_AddRefs(mdictStream));
  NS_ENSURE_SUCCESS(rv, rv);
  int dictLines = 0;
  while (mdictStream) {
    bool ok;
    rv = NS_ReadLine(mdictStream.get(), &mLineBuffer, aLine, &ok);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!ok) {
      mdictStream = nullptr;
      break;
    }
    dictStr << aLine.get() << "\r\n";
    ++dictLines;
  }
  printf("DEBUG mozNuspell::SetDictionary Read %d lines dictionary filn", dictLines);
//  printf("DEBUG mozNuspell::SetDictionary Contents dictionary file:\n%s", dictStr.str().c_str());


  printf("DEBUG Calling nuspell::Dictionary::load_from_aff_dic(affStr:=\"%s\", dictStr:=\"%s\");\n", affFileName.get(), dictFileName.get());
  mNuspell = nuspell::Dictionary::load_from_aff_dic(affStr, dictStr);

  //TODO Is this sufficient for non-POSIX platforms?
  printf("DEBUG mozNuspell::SetDictionary Calling Encoding::ForLabelNoReplacement(MakeSpan(\"UTF-8\", 6));\n");
  auto encoding =
      mozilla::Encoding::ForLabelNoReplacement(MakeSpan("ISO8859-1", 9));
//      mozilla::Encoding::ForLabelNoReplacement(MakeSpan("UTF-8", 6)); //TODO This doesn't work.
  //WIP END
  if (!encoding) {
    return NS_ERROR_UCONV_NOCONV;
  }
  printf("DEBUG mozNuspell::SetDictionary Creating encoding is OK.\n");

  mEncoder = encoding->NewEncoder();
  mDecoder = encoding->NewDecoderWithoutBOMHandling();

  return NS_OK;
}

NS_IMETHODIMP mozNuspell::GetPersonalDictionary(
    mozIPersonalDictionary** aPersonalDictionary) {
  *aPersonalDictionary = mPersonalDictionary;
  NS_IF_ADDREF(*aPersonalDictionary);
  return NS_OK;
}

NS_IMETHODIMP mozNuspell::SetPersonalDictionary(
    mozIPersonalDictionary* aPersonalDictionary) {
  mPersonalDictionary = aPersonalDictionary;
  return NS_OK;
}

NS_IMETHODIMP mozNuspell::GetDictionaryList(
    nsTArray<nsString>& aDictionaries) {
  MOZ_ASSERT(aDictionaries.IsEmpty());
  for (auto iter = mDictionaries.Iter(); !iter.Done(); iter.Next()) {
    aDictionaries.AppendElement(iter.Key());
  }

  return NS_OK;
}

void mozNuspell::LoadDictionaryList(bool aNotifyChildProcesses) {
  mDictionaries.Clear();

  nsresult rv;

  // find built in dictionaries, or dictionaries specified in
  // spellchecker.dictionary_path in prefs
  nsCOMPtr<nsIFile> dictDir;

  // check preferences first
  nsCOMPtr<nsIPrefBranch> prefs(do_GetService(NS_PREFSERVICE_CONTRACTID));
  if (prefs) {
    nsAutoCString extDictPath;
    rv = prefs->GetCharPref("spellchecker.dictionary_path", extDictPath);
    if (NS_SUCCEEDED(rv)) {
      // set the spellchecker.dictionary_path
      rv = NS_NewNativeLocalFile(extDictPath, true, getter_AddRefs(dictDir));
    }
    if (dictDir) {
      LoadDictionariesFromDir(dictDir);
    }
  }

  // find dictionaries in DICPATH
  char* dicEnv = PR_GetEnv("DICPATH");
  if (dicEnv) {
    // do a two-pass dance so dictionaries are loaded right-to-left as
    // preference
    nsTArray<nsCOMPtr<nsIFile>> dirs;
    nsAutoCString env(dicEnv);  // assume dicEnv is UTF-8

    char* currPath = nullptr;
    char* nextPaths = env.BeginWriting();
    while ((currPath = NS_strtok(":", &nextPaths))) {
      nsCOMPtr<nsIFile> dir;
      rv =
          NS_NewNativeLocalFile(nsCString(currPath), true, getter_AddRefs(dir));
      if (NS_SUCCEEDED(rv)) {
        dirs.AppendElement(dir);
      }
    }

    // load them in reverse order so they override each other properly
    for (int32_t i = dirs.Length() - 1; i >= 0; i--) {
      LoadDictionariesFromDir(dirs[i]);
    }
  }

  // find dictionaries from restartless extensions
  for (int32_t i = 0; i < mDynamicDirectories.Count(); i++) {
    LoadDictionariesFromDir(mDynamicDirectories[i]);
  }

  for (auto iter = mDynamicDictionaries.Iter(); !iter.Done(); iter.Next()) {
    mDictionaries.Put(iter.Key(), iter.Data());
  }

  DictionariesChanged(aNotifyChildProcesses);
}

void mozNuspell::DictionariesChanged(bool aNotifyChildProcesses) {
  // Now we have finished updating the list of dictionaries, update the current
  // dictionary and any editors which may use it.
  mozInlineSpellChecker::UpdateCanEnableInlineSpellChecking();

  if (aNotifyChildProcesses) {
    ContentParent::NotifyUpdatedDictionaries();
  }

  // Check if the current dictionary is still available.
  // If not, try to replace it with another dictionary of the same language.
  if (!mDictionary.IsEmpty()) {
    nsresult rv = SetDictionary(mDictionary);
    if (NS_SUCCEEDED(rv)) return;
  }

  // If the current dictionary has gone, and we don't have a good replacement,
  // set no current dictionary.
  if (!mDictionary.IsEmpty()) {
    SetDictionary(EmptyString());
  }
}

NS_IMETHODIMP
mozNuspell::LoadDictionariesFromDir(nsIFile* aDir) {
  nsresult rv;

  bool check = false;
  rv = aDir->Exists(&check);
  if (NS_FAILED(rv) || !check) return NS_ERROR_UNEXPECTED;

  rv = aDir->IsDirectory(&check);
  if (NS_FAILED(rv) || !check) return NS_ERROR_UNEXPECTED;

  nsCOMPtr<nsIDirectoryEnumerator> files;
  rv = aDir->GetDirectoryEntries(getter_AddRefs(files));
  if (NS_FAILED(rv)) return NS_ERROR_UNEXPECTED;

  nsCOMPtr<nsIFile> file;
  while (NS_SUCCEEDED(files->GetNextFile(getter_AddRefs(file))) && file) {
    nsAutoString leafName;
    file->GetLeafName(leafName);
    if (!StringEndsWith(leafName, NS_LITERAL_STRING(".dic"))) continue;

    nsAutoString dict(leafName);
    dict.SetLength(dict.Length() - 4);  // magic length of ".dic"

    // check for the presence of the .aff file
    leafName = dict;
    leafName.AppendLiteral(".aff");
    file->SetLeafName(leafName);
    rv = file->Exists(&check);
    if (NS_FAILED(rv) || !check) continue;

#ifdef DEBUG_bsmedberg
    printf("Adding dictionary: %s\n", NS_ConvertUTF16toUTF8(dict).get());
#endif

    // Replace '_' separator with '-'
    dict.ReplaceChar("_", '-');

    nsCOMPtr<nsIURI> uri;
    rv = NS_NewFileURI(getter_AddRefs(uri), file);
    NS_ENSURE_SUCCESS(rv, rv);

    mDictionaries.Put(dict, uri);
  }

  return NS_OK;
}

nsresult mozNuspell::ConvertCharset(const nsAString& aStr, std::string& aDst) {
  if (NS_WARN_IF(!mEncoder)) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  auto src = MakeSpan(aStr.BeginReading(), aStr.Length());
  CheckedInt<size_t> needed =
      mEncoder->MaxBufferLengthFromUTF16WithoutReplacement(src.Length());
  if (!needed.isValid()) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  aDst.resize(needed.value());

  char* dstPtr = &aDst[0];
  auto dst = MakeSpan(reinterpret_cast<uint8_t*>(dstPtr), needed.value());

  uint32_t result;
  size_t read;
  size_t written;
  Tie(result, read, written) =
      mEncoder->EncodeFromUTF16WithoutReplacement(src, dst, true);
  Unused << read;
  MOZ_ASSERT(result != kOutputFull);
  if (result != kInputEmpty) {
    return NS_ERROR_UENC_NOMAPPING;
  }
  aDst.resize(written);
  mEncoder->Encoding()->NewEncoderInto(*mEncoder);
  return NS_OK;
}

NS_IMETHODIMP
mozNuspell::CollectReports(nsIHandleReportCallback* aHandleReport,
                            nsISupports* aData, bool aAnonymize) {
  MOZ_COLLECT_REPORT("explicit/spell-check", KIND_HEAP, UNITS_BYTES,
                     0,
                     "Memory used by the spell-checking engine.");

  return NS_OK;
}

NS_IMETHODIMP
mozNuspell::Check(const nsAString& aWord, bool* aResult) {
  printf("DEBUG Entering mozNuspell::Check(aWord:=\"%s\",\n", NS_ConvertUTF16toUTF8(aWord).get());
  if (NS_WARN_IF(!aResult)) {
    return NS_ERROR_INVALID_ARG;
  }

  std::string charsetWord;
  nsresult rv = ConvertCharset(aWord, charsetWord);
  NS_ENSURE_SUCCESS(rv, rv);

  *aResult = mNuspell.spell(charsetWord);

  if (!*aResult && mPersonalDictionary)
    rv = mPersonalDictionary->Check(aWord, aResult);

  printf("DEBUG Leaving mozNuspell::Check(aWord=\"%s\", aResult:=%d)\n", NS_ConvertUTF16toUTF8(aWord).get(), aResult?1:0);
  return rv;
}

NS_IMETHODIMP
mozNuspell::Suggest(const nsAString& aWord, nsTArray<nsString>& aSuggestions) {
  printf("DEBUG Entering mozNuspell::Suggest(const nsAString& aWord:=\"%s\",\n", NS_ConvertUTF16toUTF8(aWord).get());
  MOZ_ASSERT(aSuggestions.IsEmpty());

  std::string charsetWord;
  nsresult rv = ConvertCharset(aWord, charsetWord);
  NS_ENSURE_SUCCESS(rv, rv);

  std::vector<std::string> suggestions = std::vector<std::string>();
  mNuspell.suggest(charsetWord, suggestions);

  if (!suggestions.empty()) {
    aSuggestions.SetCapacity(suggestions.size());
    for (Span<const char> charSrc : suggestions) {
      // Convert the suggestion to utf16
      auto src = AsBytes(charSrc);
      rv = mDecoder->Encoding()->DecodeWithoutBOMHandling(
          src, *aSuggestions.AppendElement());
      NS_ENSURE_SUCCESS(rv, rv);
      mDecoder->Encoding()->NewDecoderWithoutBOMHandlingInto(*mDecoder);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
mozNuspell::Observe(nsISupports* aSubj, const char* aTopic,
                     const char16_t* aData) {
  NS_ASSERTION(!strcmp(aTopic, "profile-do-change") ||
                   !strcmp(aTopic, "profile-after-change"),
               "Unexpected observer topic");

  LoadDictionaryList(false);

  return NS_OK;
}

NS_IMETHODIMP mozNuspell::AddDirectory(nsIFile* aDir) {
  mDynamicDirectories.AppendObject(aDir);
  LoadDictionaryList(true);
  return NS_OK;
}

NS_IMETHODIMP mozNuspell::RemoveDirectory(nsIFile* aDir) {
  mDynamicDirectories.RemoveObject(aDir);
  LoadDictionaryList(true);

#ifdef MOZ_THUNDERBIRD
  /*
   * This notification is needed for Thunderbird. Thunderbird derives the
   * dictionary from the document's "lang" attribute. If a dictionary is
   * removed, we need to change the "lang" attribute.
   */
  nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
  if (obs) {
    obs->NotifyObservers(nullptr, SPELLCHECK_DICTIONARY_REMOVE_NOTIFICATION,
                         nullptr);
  }
#endif
  return NS_OK;
}

NS_IMETHODIMP mozNuspell::AddDictionary(const nsAString& aLang,
                                         nsIURI* aFile) {
  NS_ENSURE_TRUE(aFile, NS_ERROR_INVALID_ARG);

  mDynamicDictionaries.Put(aLang, aFile);
  mDictionaries.Put(aLang, aFile);
  DictionariesChanged(true);
  return NS_OK;
}

NS_IMETHODIMP mozNuspell::RemoveDictionary(const nsAString& aLang,
                                            nsIURI* aFile, bool* aRetVal) {
  NS_ENSURE_TRUE(aFile, NS_ERROR_INVALID_ARG);
  *aRetVal = false;

  nsCOMPtr<nsIURI> file = mDynamicDictionaries.Get(aLang);
  bool equal;
  if (file && NS_SUCCEEDED(file->Equals(aFile, &equal)) && equal) {
    mDynamicDictionaries.Remove(aLang);
    LoadDictionaryList(true);
    *aRetVal = true;
  }
  return NS_OK;
}
