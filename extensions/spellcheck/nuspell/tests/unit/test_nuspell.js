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

// "skip", "fixme" or anything else will skip the test
// "pass" should pass of all of the sub test
// "passnosug" should pass of all of the sub test, except sug test *
// "fail" should fail at least one the sub tests *
// * see https://github.com/nuspell/nuspell/blob/master/tests/CMakeLists.txt

const tests = [
  ["pass", "1463589"],
  ["pass", "1463589-utf"],
  ["pass", "1592880"],
  ["pass", "1695964"],
  ["pass", "1706659"],
  ["pass", "1975530"],

  ["pass", "2970240"],
  ["pass", "2970242"],
  ["pass", "2999225"],

  ["pass", "IJ"],

  ["pass", "affixes"],
  ["pass", "alias"],
  ["pass", "alias2"],
  ["pass", "alias3"],
  ["pass", "allcaps"],
  ["pass", "allcaps2"],
  ["pass", "allcaps3"],
  ["pass", "allcaps-utf"],
  ["pass", "arabic"],

  ["pass", "base"],
  ["fail", "base-utf"], // See * above.
  ["pass", "break"],
  ["pass", "breakdefault"],
  ["pass", "breakoff"],

  ["pass", "checkcompoundcase"],
  ["pass", "checkcompoundcase2"],
  ["pass", "checkcompoundcaseutf"],
  ["pass", "checkcompounddup"],
  ["pass", "checkcompoundpattern"],
  ["pass", "checkcompoundpattern2"],
  ["pass", "checkcompoundpattern3"],
  ["pass", "checkcompoundpattern4"],
  ["pass", "checkcompoundrep"],
  ["pass", "checkcompoundtriple"],
  ["passnosug", "checksharps"], // See * above.
  ["passnosug", "checksharpsutf"], // See * above.
  ["pass", "circumfix"],
  ["skip", "colons-in-words"], // No test files.
  ["pass", "complexprefixes"],
  ["pass", "complexprefixes2"],
  ["pass", "complexprefixesutf"],
  ["pass", "compoundaffix"],
  ["pass", "compoundaffix2"],
  ["pass", "compoundaffix3"],
  ["pass", "compoundflag"],
  ["pass", "compoundrule"],
  ["pass", "compoundrule2"],
  ["pass", "compoundrule3"],
  ["pass", "compoundrule4"],
  ["pass", "compoundrule5"],
  ["pass", "compoundrule6"],
  ["pass", "compoundrule7"],
  ["pass", "compoundrule8"],
  ["pass", "condition"],
  ["pass", "condition-utf"],
  ["pass", "conditionalprefix"],

  ["pass", "digits-in-words"],
  ["pass", "dotless-i"],

  ["pass", "encoding"],

  ["pass", "flag"],
  ["pass", "flaglong"],
  ["pass", "flagnum"],
  ["pass", "flagutf8"],
  ["pass", "fogemorpheme"],
  ["pass", "forbiddenword"],
  ["pass", "forceucase"],
  ["pass", "fullstrip"],

  ["pass", "germancompounding"],
  ["pass", "germancompoundingold"],

  ["pass", "hu"],

  ["pass", "i35725"],
  ["pass", "i53643"],
  ["pass", "i54633"],
  ["pass", "i54980"],
  ["pass", "i58202"],
  ["pass", "i68568"],
  ["pass", "i68568utf"],
  ["pass", "iconv"],
  ["pass", "iconv2"],
  ["pass", "ignore"],
  ["pass", "ignoreutf"],

  ["pass", "keepcase"],
  ["pass", "korean"],

  ["pass", "map"],
  ["pass", "maputf"],
  ["skip", "morph"], // Spaces in good file.

  ["pass", "needaffix"],
  ["pass", "needaffix2"],
  ["pass", "needaffix3"],
  ["pass", "needaffix4"],
  ["pass", "needaffix5"],
  ["fail", "nepali"], // See * above.
  ["passnosug", "ngram-utf-fix"], // No suggestions for "времячко", identical to Nuspell command-line tool.
  ["passnosug", "nosuggest"], // See * above. Empty suggestions file.

  ["pass", "oconv"],
  ["passnosug", "onlyincompound"], // Empty suggestions file.
  ["pass", "onlyincompound2"],
  ["pass", "opentaal-cpdpat"],
  ["pass", "opentaal-cpdpat2"],
  ["passnosug", "opentaal-forbiddenword1"], // No suggestions for "foowordbar", identical to Nuspell command-line tool.
  ["passnosug", "opentaal-forbiddenword2"], // No suggestions for "foowordbar", identical to Nuspell command-line tool.
  ["pass", "opentaal-keepcase"],

  ["passnosug", "phone"], // See * above.

  ["passnosug", "rep"], // Suggestion file does not line up with wrong file.
  ["pass", "reputf"],

  ["pass", "simplifiedtriple"],
  ["pass", "slash"],
  ["pass", "sug"],
  ["pass", "sugutf"],

  ["pass", "utf8"],
  ["pass", "utf8-bom"],
  ["pass", "utf8-bom2"],
  ["passnosug", "utf8-nonbmp"], // See * above.
  ["pass", "utfcompound"],

  ["pass", "warn"],

  ["pass", "zeroaffix"],
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
  // trim() is needed for first line in good file for utf8-nonbmp
  while (lis.readLine(val)) {
    yield val.value.trim();
    val = {};
  }
  yield val.value.trim();
}

function do_run_test(checker, action, name) {
  if (action != "pass" && action != "passnosug" && action != "fail") {
    dump("Skipping test for " + name + "\n");
    return;
  }
  dump("\n\n\n\n");
  if (action == "pass" || action == "passnosug") {
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

  // Good
  var good_counter = 0;
  if (good.exists()) {
    for (const val of do_get_file_by_line(good)) {
      good_counter++;
      dump("Expect word \"" + val + "\" is spelled correctly\n");
      if (action == "pass" || action == "passnosug") {
        Assert.ok(checker.check(val));
      } else {
        if (!checker.check(val)) {
          positive_fail = true;
        }
      }
    }
  }

  // Wrong
  var wrong_counter = 0;
  if (wrong.exists()) {
    for (const val of do_get_file_by_line(wrong)) {
      wrong_counter++;
      dump("Expect word \"" + val + "\" is spelled wrong\n");
      if (action == "pass" || action == "passnosug") {
        Assert.ok(!checker.check(val));
      } else {
        if (checker.check(val)) {
          positive_fail = true;
        }
      }
    }
  }

  // Suggestions (only when tests above are expected to pass)
  var sug_counter = 0;
  if (wrong.exists() && sug.exists() && action == "pass") {
    // wrong file can have more lines than sug file, ignore extra lines in wrong
    wrongs = do_get_file_by_line(wrong);
    // iterate over sug file
    for (const val of do_get_file_by_line(sug)) {
      sug_counter++;
      // wrong files tags along via own explicitly called iterator
      wr = wrongs.next().value;
      var suggestions = checker.suggest(wr);
      if (suggestions.length < 1) {
        if (val == "") {
          dump("Wrongly spelled word \"" + wr
           + "\" got no suggestions returned which matches empty string \""
           + val + "\"\n");
          Assert.ok(true);
        } else {
          dump("Wrongly spelled word \"" + wr
           + "\" got no suggestions returned which should match \"" + val
           + "\"\n");
          Assert.ok(false);
          }
      } else {
        dump("Wrongly spelled word \"" + wr + "\" has suggestions returned \""
         + suggestions.join(", ") + "\" which should match \"" + val + "\"\n");
        var index = 0;
        // iterate over expected suggestions
        for (const sg of val.split(", ")) {
          if (index + 1 > suggestions.length) {
            dump("Wrongly spelled word \"" + wr
             + "\" got not enough suggestions returned\n");
            Assert.ok(false);
          }
          dump("Expect wrongly spelled word \"" + wr
           + "\" to have as suggestion #" + index + " \"" + sg + "\"\n");
          Assert.ok(sg == suggestions[index]);
          index++;
        }
      }
    }
  }

  // End
  if (good_counter + wrong_counter == 0) {
    do_throw("No tests were done for " + name + "\n");
  }

  if (action == "fail" && !positive_fail) {
    dump("Expected fail has occurred properly for " + name + "\n");
    Assert.ok(false);
  }
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
    let [action, name] = test;
    do_run_test(spellChecker, action, name);
  }

  tests.forEach(do_run_test_closure);
}
