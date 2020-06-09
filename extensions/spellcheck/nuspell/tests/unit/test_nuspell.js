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

// "skip", "fixme", "notests" or anything else will skip the test
// "pass" should result in passing of the test
// "fail" should result in failing of the test, as can be found in
// https://github.com/nuspell/nuspell/blob/master/tests/CMakeLists.txt

const tests = [
  ["pass", "1463589"],
  ["pass", "1463589-utf"],
  ["pass", "1592880"],
  ["pass", "1695964"],
  ["pass", "1706659"],
  ["fixme", "1975530"],

  ["notests", "2970240"],
  ["fixme", "2970242"],
  ["pass", "2999225"],

  ["pass", "IJ"],

  ["fixme", "affixes"],
  ["fixme", "alias"],
  ["fixme", "alias2"],
  ["fixme", "alias3"],
  ["fixme", "allcaps"],
  ["fixme", "allcaps2"],
  ["fixme", "allcaps3"],
  ["fixme", "allcaps-utf"],
  ["notests", "arabic"],

  ["pass", "base"], //FIXME Set to "pass" for now for first priority error. Fails on the correct word "Nuspell."
  ["fail", "base-utf"],
  ["fixme", "break"],
  ["fixme", "breakdefault"],
  ["fixme", "breakoff"],

  ["fixme", "checkcompoundcase"],
  ["fixme", "checkcompoundcase2"],
  ["fixme", "checkcompoundcaseutf"],
  ["fixme", "checkcompounddup"],
  ["fixme", "checkcompoundpattern"],
  ["fixme", "checkcompoundpattern2"],
  ["fixme", "checkcompoundpattern3"],
  ["pass", "checkcompoundpattern4"],
  ["fixme", "checkcompoundrep"],
  ["fixme", "checkcompoundtriple"],
  ["fail", "checksharps"],
  ["fail", "checksharpsutf"],
  ["fixme", "circumfix"],
  ["skip", "colons-in-words"], // Suggestion test only
  ["fixme", "complexprefixes"],
  ["pass", "complexprefixes2"],
  ["fixme", "complexprefixesutf"],
  ["fixme", "compoundaffix"],
  ["fixme", "compoundaffix2"],
  ["fixme", "compoundaffix3"],
  ["pass", "compoundflag"],
  ["fixme", "compoundrule"],
  ["fixme", "compoundrule2"],
  ["fixme", "compoundrule3"],
  ["fixme", "compoundrule4"],
  ["fixme", "compoundrule5"],
  ["fixme", "compoundrule6"],
  ["fixme", "compoundrule7"],
  ["fixme", "compoundrule8"],
  ["fixme", "condition"],
  ["fixme", "condition-utf"],
  ["fixme", "conditionalprefix"],

  ["notests", "digits-in-words"],
  ["fixme", "dotless-i"],

  ["fixme", "encoding", { 1: "todo", 3: "todo" }],

  ["fixme", "flag"],
  ["fixme", "flaglong"],
  ["fixme", "flagnum"],
  ["fixme", "flagutf8"],
  ["fixme", "fogemorpheme"],
  ["pass", "forbiddenword"],
  ["fixme", "forceucase"],
  ["fixme", "fullstrip"],

  ["pass", "germancompounding"],
  ["pass", "germancompoundingold"],

  ["pass", "hu"],

  ["pass", "i35725"],
  ["pass", "i53643"],
  ["fixme", "i54633"],
  ["fixme", "i54980", { 1: "todo", 3: "todo" }],
  ["fixme", "i58202"],
  ["pass", "i68568"],
  ["pass", "i68568utf"],
  ["fixme", "iconv"],
  ["pass", "iconv2"],
  ["fixme", "ignore"],
  [
    "pass",
    "ignoreutf",
    {
      1: "todo",
      2: "todo",
      3: "todo",
      4: "todo",
      5: "todo",
      6: "todo",
      7: "todo",
      8: "todo",
    },
  ],

  ["pass", "keepcase"],
  ["fixme", "korean"],

  ["pass", "map"],
  ["pass", "maputf"],
  [
    "fixme",
    "morph",
    "us-ascii",
    {
      11: "todo",
      12: "todo",
      13: "todo",
      14: "todo",
      15: "todo",
      16: "todo",
      17: "todo",
      18: "todo",
      19: "todo",
      20: "todo",
      21: "todo",
      22: "todo",
      23: "todo",
      24: "todo",
      25: "todo",
      26: "todo",
      27: "todo",
    },
  ],

  ["fixme", "needaffix"],
  ["fixme", "needaffix2"],
  ["fixme", "needaffix3"],
  ["fixme", "needaffix4"],
  ["fixme", "needaffix5"],
  ["fail", "nepali"],
  ["pass", "ngram-utf-fix"],
  ["fail", "nosuggest"],

  ["pass", "oconv"],
  ["fixme", "onlyincompound"],
  ["fixme", "onlyincompound2"],
  ["notests", "opentaal-cpdpat"],
  ["notests", "opentaal-cpdpat2"],
  ["fixme", "opentaal-forbiddenword1"],
  ["fixme", "opentaal-forbiddenword2"],
  ["pass", "opentaal-keepcase"],

  ["fixme", "phone"], // Should be fail, no lines are read from phone.wrong

  ["pass", "rep"],
  ["notests", "reputf"],

  ["fixme", "simplifiedtriple"],
  ["pass", "slash"],
  ["fixme", "sug"],
  ["pass", "sugutf"],

  ["fixme", "utf8"],
  ["pass", "utf8-bom", { 1: "todo" }],
  ["pass", "utf8-bom2", { 1: "todo" }],
  ["pass", "utf8-nonbmp", { 1: "todo", 2: "todo", 3: "todo", 4: "todo" }],
  ["fixme", "utfcompound"],

  ["pass", "warn"],

  ["fixme", "zeroaffix"],
];

// eslint-disable-next-line no-shadow
function* do_get_file_by_line(file) {
  dump("Getting file by line for file " + file.path + "\n");
  let fis = Cc["@mozilla.org/network/file-input-stream;1"].createInstance(
    Ci.nsIFileInputStream
  );
  fis.init(file, 0x01, 0o444, Ci.nsIFileInputStream.CLOSE_ON_EOF);

  let lis = Cc["@mozilla.org/intl/converter-input-stream;1"].createInstance(
    Ci.nsIConverterInputStream
  );
  lis.init(fis, "UTF-8", 1024, 0x0000);
  lis.QueryInterface(Ci.nsIUnicharLineInputStream);

  let val = {};
  let debug_lines = 0;
  while (lis.readLine(val)) {
    yield val.value;
    val = {};
    debug_lines++;
  }
  dump("DEBUG_XXXXXX_js Loop has read for " + file.path + " number of lines " + debug_lines + "\n");
  //FIXME The while reads 0 lines when file contains only one line.
}

function do_run_test(checker, action, name, todo_good, todo_wrong) {
  if (action != "pass" && action != "fail") {
    dump("Skipping test for " + name + "\n");
    return;
  }
  dump("\n\n\n\n");
  if (action == "pass") {
    dump("Running test for " + name + " that should pass\n");
  } else {
    dump("Running test for " + name + " that should fail\n");
  }
  if (!checker) {
    do_throw("Need a spelling checker here");
  }

  let good = do_get_file("data/" + name + ".good", true);
  let wrong = do_get_file("data/" + name + ".wrong", true);
  let sug = do_get_file("data/" + name + ".sug", true);

  dump("Need some expected output\n");
  Assert.ok(good.exists() || wrong.exists() || sug.exists());

  dump("Setting dictionary to " + name + "\n");
  checker.dictionary = name;

  let positive_fail = false;

  var good_counter = 0;
  if (good.exists()) {
    for (const val of do_get_file_by_line(good)) {
      let todo = false;
      good_counter++;
      if (todo_good && todo_good[good_counter]) {
        todo = true;
        dump("TODO\n");
      }

      dump("Expect word " + val + " is spelled correctly\n");
      if (action == "pass") {
        if (todo) {
          todo_check_true(checker.check(val));
        } else {
          Assert.ok(checker.check(val));
        }
      } else {
        if (!checker.check(val)) {
          positive_fail = true;
        }
      }
    }
  }

  var wrong_counter = 0;
  if (wrong.exists()) {
    for (const val of do_get_file_by_line(wrong)) {
      let todo = false;
      wrong_counter++;
      if (todo_wrong && todo_wrong[wrong_counter]) {
        todo = true;
        dump("TODO\n");
      }

      dump("Expect word " + val + " is spelled wrong\n");
      if (action == "pass") {
        if (todo) {
          todo_check_false(checker.check(val));
        } else {
          Assert.ok(!checker.check(val));
        }
      } else {
        if (!checker.check(val)) {
          positive_fail = true;
        }
      }
    }
  }

  if (good_counter == 0 && wrong_counter == 0) {
    // Note that this is not fool proof as one could be 0 even though both
    // files have words to test. At least it uncovers underlying bug for now.
    do_throw("No tests for good or wrong words were done for " + name + "\n");
  }

  if (action == "fail") {
    dump("Expected fail has occurred properly for " + name + "\n");
    Assert.ok(positive_fail);
  }

  // XXXkhuey test suggestions
}

function run_test() {
  let spellChecker = Cc["@mozilla.org/spellchecker/engine;1"].getService(
    Ci.mozISpellCheckingEngine
  );

  Assert.ok(!!spellChecker, "Should have a spelling checker");
  spellChecker.QueryInterface(Ci.mozISpellCheckingEngine);
  let testdir = do_get_file("data/", false);
  spellChecker.loadDictionariesFromDir(testdir);

  function do_run_test_closure(test) {
    let [action, name, todo_good, todo_wrong] = test;
    do_run_test(spellChecker, action, name, todo_good, todo_wrong);
  }

  tests.forEach(do_run_test_closure);
}
