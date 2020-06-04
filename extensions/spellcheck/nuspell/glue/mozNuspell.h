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

#ifndef mozNuspell_h__
#define mozNuspell_h__

#include <dictionary.hxx>
#include "mozISpellCheckingEngine.h"
#include "mozIPersonalDictionary.h"
#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsCOMArray.h"
#include "nsIMemoryReporter.h"
#include "nsIObserver.h"
#include "nsIURI.h"
#include "mozilla/Encoding.h"
#include "nsInterfaceHashtable.h"
#include "nsWeakReference.h"
#include "nsCycleCollectionParticipant.h"

#define MOZ_NUSPELL_CONTRACTID "@mozilla.org/spellchecker/engine;1"
#define MOZ_NUSPELL_CID                             \
  /* 56c778e4-1bee-45f3-a689-886692a97fe7 */         \
  {                                                  \
    0x56c778e4, 0x1bee, 0x45f3, {                    \
      0xa6, 0x89, 0x88, 0x66, 0x92, 0xa9, 0x7f, 0xe7 \
    }                                                \
  }

class mozNuspell final : public mozISpellCheckingEngine,
                         public nsIObserver,
                         public nsSupportsWeakReference,
                         public nsIMemoryReporter {
 public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_MOZISPELLCHECKINGENGINE
  NS_DECL_NSIOBSERVER
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(mozNuspell, mozISpellCheckingEngine)

  mozNuspell();

  nsresult Init();

  void LoadDictionaryList(bool aNotifyChildProcesses);

  NS_DECL_NSIMEMORYREPORTER

 protected:
  virtual ~mozNuspell();

  void DictionariesChanged(bool aNotifyChildProcesses);

  nsCOMPtr<mozIPersonalDictionary> mPersonalDictionary;
  mozilla::UniquePtr<mozilla::Decoder> mDecoder; // this is useless with Nuspell

  // Hashtable matches dictionary name to .aff file
  nsInterfaceHashtable<nsStringHashKey, nsIURI> mDictionaries;
  nsString mDictionary;
  nsCString mAffixFileName;

  // dynamic dirs used to search for dictionaries
  nsCOMArray<nsIFile> mDynamicDirectories;
  nsInterfaceHashtable<nsStringHashKey, nsIURI> mDynamicDictionaries;

  nuspell::Dictionary mNuspell;

};

#endif
