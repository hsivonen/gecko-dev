/* Copyright 2020 Sander van Geloven, Dimitrij Mijoski
 *
 * This file is part of Nuspell.
 *
 * Nuspell is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Nuspell is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Nuspell.  If not, see <http://www.gnu.org/licenses/>.
 */

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
#include <iostream> //TODO Only for debugging, remove later.

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
  printf("DEBUG2 Entering mozNuspell::Init()\n");
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
  printf("DEBUG2 Entering mozNuspell::~mozNuspell()\n");
  mozilla::UnregisterWeakMemoryReporter(this);

  mPersonalDictionary = nullptr;
}

NS_IMETHODIMP
mozNuspell::GetDictionary(nsAString& aDictionary) {
  printf("DEBUG2 Entering mozNuspell::GetDictionary(aDictionary=\"%s\")\n", NS_ConvertUTF16toUTF8(aDictionary).get());
  aDictionary = mDictionary;
  return NS_OK;
}

/* set the Dictionary.
 * This also Loads the dictionary and initializes the converter using the
 * dictionaries converter
 */
NS_IMETHODIMP
mozNuspell::SetDictionary(const nsAString& aDictionary) {
  printf("DEBUG2 Entering mozNuspell::SetDictionary(aDictionary=\"%s\")\n", NS_ConvertUTF16toUTF8(aDictionary).get());
  if (aDictionary.IsEmpty()) {
    printf("DEBUG2   aDictionary is empty\n");
    mNuspell = nuspell::Dictionary();
    mDictionary.Truncate();
    mAffixFileName.Truncate();

    return NS_OK;
  }

  nsIURI* affFile = mDictionaries.GetWeak(aDictionary);
    printf("DEBUG2   affFile=\"%s\"\n", affFile->GetSpecOrDefault().get());
  if (!affFile) { //TODO All xpcshell unit tests fails here.
    return NS_ERROR_FILE_NOT_FOUND;
  }

  nsAutoCString dictFileName, affFileName;

  nsresult rv = affFile->GetSpec(affFileName);
  NS_ENSURE_SUCCESS(rv, rv);
  printf("DEBUG2 mozNuspell::SetDictionary Created nsIURI affFile=\"%s\"\n", affFile->GetSpecOrDefault().get());

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

  nsLineBuffer<char> mLineBuffer;

  auto affStr = std::stringstream();
  nsCOMPtr<nsIURI> affUri;
  rv = NS_NewURI(getter_AddRefs(affUri), affFileName.get());
  NS_ENSURE_SUCCESS(rv, rv);
  printf("DEBUG2 mozNuspell::SetDictionary Created affUri=\"%s\"\n", affUri->GetSpecOrDefault().get());
  nsCOMPtr<nsIChannel> affChannel;
  rv = NS_NewChannel(getter_AddRefs(affChannel), affUri,
                        nsContentUtils::GetSystemPrincipal(),
                        nsILoadInfo::SEC_REQUIRE_SAME_ORIGIN_DATA_INHERITS,
                        nsIContentPolicy::TYPE_OTHER);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIInputStream> mAffStream;
  rv = affChannel->Open(getter_AddRefs(mAffStream));
  NS_ENSURE_SUCCESS(rv, rv);
  int affNumLines = 0;
  nsCString affLine; //TODO nsCString or nsAutoCString
  while (mAffStream) {
    bool more;
    rv = NS_ReadLine(mAffStream.get(), &mLineBuffer, affLine, &more);
    NS_ENSURE_SUCCESS(rv, rv);
    affStr << affLine.get() << '\n';
    ++affNumLines;
    if (!more) {
      mAffStream = nullptr;
      break;
    }
  }
  printf("DEBUG2 mozNuspell::SetDictionary affFileName=\"%s\" read %d lines\n", affFileName.get(),  affNumLines);
  printf("DEBUG2 mozNuspell::SetDictionary Contents affix file:\n%s", affStr.str().c_str());

  auto dictStr = std::stringstream();
  nsCOMPtr<nsIURI> dictUri;
  rv = NS_NewURI(getter_AddRefs(dictUri), dictFileName.get());
  NS_ENSURE_SUCCESS(rv, rv);
  printf("DEBUG2 mozNuspell::SetDictionary Created dictUri=\"%s\"\n", dictUri->GetSpecOrDefault().get());
  nsCOMPtr<nsIChannel> dictChannel;
  rv = NS_NewChannel(getter_AddRefs(dictChannel), dictUri,
                        nsContentUtils::GetSystemPrincipal(),
                        nsILoadInfo::SEC_REQUIRE_SAME_ORIGIN_DATA_INHERITS,
                        nsIContentPolicy::TYPE_OTHER);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIInputStream> mdictStream;
  rv = dictChannel->Open(getter_AddRefs(mdictStream));
  NS_ENSURE_SUCCESS(rv, rv);
  int dictNumLines = 0;
  nsCString dictLine; //TODO nsCString or nsAutoCString
  while (mdictStream) {
    bool more;
    rv = NS_ReadLine(mdictStream.get(), &mLineBuffer, dictLine, &more);
    NS_ENSURE_SUCCESS(rv, rv);
    dictStr << dictLine.get() << '\n';
    ++dictNumLines;
    if (!more) {
      mdictStream = nullptr;
      break;
    }
  }
  printf("DEBUG2 mozNuspell::SetDictionary dictFileName=\"%s\" read %d lines\n", dictFileName.get(), dictNumLines);
  printf("DEBUG2 mozNuspell::SetDictionary Contents dictionary file:\n%s", dictStr.str().c_str());

  affStr.seekg(0);
  dictStr.seekg(0);
  printf("DEBUG2 Calling nuspell::Dictionary::load_from_aff_dic(affStr, dictStr);\n");
  mNuspell = nuspell::Dictionary::load_from_aff_dic(affStr, dictStr);

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
    printf("DEBUG2    mozNuspell::LoadDictionaryList DICPATH=%s\n", dicEnv);
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
  std::cout << "DEBUG2 Entering mozNuspell::LoadDictionariesFromDir(aDir=\"" << aDir << "\")\n";
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
    printf("DEBUG2 mozNuspell::LoadDictionariesFromDir Adding dictionary \"%s\"\n", NS_ConvertUTF16toUTF8(dict).get());

#ifdef DEBUG_bsmedberg
    printf("Adding dictionary: %s\n", NS_ConvertUTF16toUTF8(dict).get());
#endif

    // Replace '_' separator with '-'
    dict.ReplaceChar("_", '-');

    nsCOMPtr<nsIURI> uri;
    rv = NS_NewFileURI(getter_AddRefs(uri), file);
    NS_ENSURE_SUCCESS(rv, rv);

    mDictionaries.Put(dict, uri);
    printf("DEBUG2 mozNuspell::LoadDictionariesFromDir Added dictionary \"%s\"\n", NS_ConvertUTF16toUTF8(dict).get());
  }

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
  printf("DEBUG2 Entering mozNuspell::Check(aWord=\"%s\",\n", NS_ConvertUTF16toUTF8(aWord).get());
  if (NS_WARN_IF(!aResult)) {
    return NS_ERROR_INVALID_ARG;
  }

  NS_ConvertUTF16toUTF8 u8word(aWord);
  std::string charsetWord(u8word.Data(), u8word.Length());

  *aResult = mNuspell.spell(charsetWord);

  nsresult rv = NS_OK;
  if (!*aResult && mPersonalDictionary)
    rv = mPersonalDictionary->Check(aWord, aResult);

  printf("DEBUG2 Leaving mozNuspell::Check(aWord=\"%s\", aResult=%d)\n", NS_ConvertUTF16toUTF8(aWord).get(), *aResult?1:0);
  return rv;
}

NS_IMETHODIMP
mozNuspell::Suggest(const nsAString& aWord, nsTArray<nsString>& aSuggestions) {
  printf("DEBUG2 Entering mozNuspell::Suggest(aWord=\"%s\",\n", NS_ConvertUTF16toUTF8(aWord).get());
  MOZ_ASSERT(aSuggestions.IsEmpty());

  NS_ConvertUTF16toUTF8 u8word(aWord);
  std::string charsetWord(u8word.Data(), u8word.Length());

  std::vector<std::string> suggestions = std::vector<std::string>();
  mNuspell.suggest(charsetWord, suggestions);

  if (!suggestions.empty()) {
    aSuggestions.SetCapacity(suggestions.size());
    for (auto& src : suggestions) {
      printf("DEBUG2    suggestion \"%s\"\n", src.c_str());
      aSuggestions.AppendElement(NS_ConvertUTF8toUTF16(src.data(), src.size()));
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
  std::cout << "DEBUG2 Entering mozNuspell::AddDirectory(aDir=\"" << aDir << "\")\n";
  mDynamicDirectories.AppendObject(aDir);
  LoadDictionaryList(true);
  return NS_OK;
}

NS_IMETHODIMP mozNuspell::RemoveDirectory(nsIFile* aDir) {
  std::cout << "DEBUG2 Entering mozNuspell::RemoveDirectory(aDir=\"" << aDir << "\")\n";
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
  std::cout << "DEBUG2 Entering mozNuspell::AddDictionary(aLang=\"" << NS_ConvertUTF16toUTF8(aLang).get() << "\", aFile=\"" << aFile << "\")\n";
  NS_ENSURE_TRUE(aFile, NS_ERROR_INVALID_ARG);

  mDynamicDictionaries.Put(aLang, aFile);
  mDictionaries.Put(aLang, aFile);
  DictionariesChanged(true);
  return NS_OK;
}

NS_IMETHODIMP mozNuspell::RemoveDictionary(const nsAString& aLang,
                                            nsIURI* aFile, bool* aRetVal) {
  std::cout << "DEBUG2 Entering mozNuspell::RemoveDictionary(aLang=\"" << NS_ConvertUTF16toUTF8(aLang).get() << "\", aFile=\"" << aFile << "\")\n";
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
