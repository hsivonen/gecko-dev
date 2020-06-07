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

"use strict";

const { XPCOMUtils } = ChromeUtils.import(
  "resource://gre/modules/XPCOMUtils.jsm"
);
const { OS } = ChromeUtils.import("resource://gre/modules/osfile.jsm");

XPCOMUtils.defineLazyServiceGetter(
  this,
  "spellCheck",
  "@mozilla.org/spellchecker/engine;1",
  "mozISpellCheckingEngine"
);

const nsFile = Components.Constructor(
  "@mozilla.org/file/local;1",
  "nsIFile",
  "initWithPath"
);

add_task(async function() {
  let prof = do_get_profile();

  // Using https://codepoints.net/U+263A as UTF-8 character.
  let basePath = OS.Path.join(prof.path, "\u263a", "dictionaries");
  let baseDir = nsFile(basePath);
  // This will create the directory /tmp/firefox/xpcshellprofile/☺/ on POSIX.
  await OS.File.makeDir(basePath, { from: prof.path });

  let dicPath = OS.Path.join(basePath, "dict.dic");
  let affPath = OS.Path.join(basePath, "dict.aff");

  const WORD = "Flehgragh";

  await OS.File.writeAtomic(dicPath, new TextEncoder().encode(`1\n${WORD}\n`));
  await OS.File.writeAtomic(affPath, new TextEncoder().encode(""));

  spellCheck.loadDictionariesFromDir(baseDir); //TODO Error occurs here.
  spellCheck.dictionary = "dict";

  ok(
    spellCheck.check(WORD),
    "Dictionary should have been loaded from a Unicode path"
  );
});
