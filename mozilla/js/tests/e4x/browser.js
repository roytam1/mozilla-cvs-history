/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

/*
 * JavaScript test library shared functions file for running the tests
 * in the browser.  Overrides the shell's print function with document.write
 * and make everything HTML pretty.
 *
 * To run the tests in the browser, use the mkhtml.pl script to generate
 * html pages that include the shell.js, browser.js (this file), and the
 * test js file in script tags.
 *
 * The source of the page that is generated should look something like this:
 *      <script src="./../shell.js"></script>
 *      <script src="./../browser.js"></script>
 *      <script src="./mytest.js"></script>
 */
function writeLineToLog( string ) {
  document.write( string + "<br>\n");
}

function print( string ) {
  writeLineToLog( string );
}

var testcases = new Array();
var tc = testcases.length;
var BUGSTR = '';
var SUMMARY = '';
var DESCRIPTION = '';
var EXPECTED = '';
var ACTUAL = '';
var MSG = '';
var SECTION = '';

function TestCase(n, d, e, a)
{
  this.name = n;
  this.description = d;
  this.expect = e;
  this.actual = a;
  this.passed = ( e == a );
  this.reason = '';
  this.bugnumber = typeof(BUGSTR) != 'undefined' ? BUGSTR : '';
  testcases[tc++] = this;
}

function reportSuccess(section, expected, actual)
{
  var testcase = new TestCase(gTestName,  SUMMARY + DESCRIPTION + ' Section ' + section, expected, actual);
  testcase.passed = true;
};

function reportError(msg, page, line)
{
  var testcase;

  if (typeof SUMMARY == 'undefined')
  {
    SUMMARY = 'Unknown';
  }
  if (typeof SECTION == 'undefined')
  {
    SECTION = 'Unknown';
  }
  if (typeof DESCRIPTION == 'undefined')
  {
    DESCRIPTION = 'Unknown';
  }
  if (typeof EXPECTED == 'undefined')
  {
    EXPECTED = 'Unknown';
  }

  testcase = new TestCase(gTestName, SUMMARY + DESCRIPTION + ' Section ' + SECTION, EXPECTED, "error");

  testcase.passed = false;
  testcase.reason += msg;

  if (typeof(page) != 'undefined')
  {
    testcase.reason += ' Page: ' + page;
  }
  if (typeof(line) != 'undefined')
  {
    testcase.reason += ' Line: ' + line;
  }
  reportFailure(SECTION, msg);

};


var _reportFailure = reportFailure;
reportFailure = function (section, msg)
{
  var testcase;

  testcase = new TestCase(gTestName, SUMMARY + DESCRIPTION + ' Section ' + section, EXPECTED, ACTUAL);

  testcase.passed = false;
  testcase.reason += msg;

  _reportFailure(section, msg);

};


var _printBugNumber = printBugNumber;
printBugNumber = function (num)
{
  BUGSTR = BUGNUMBER + num;
  _printBugNumber(num);
}

var _START = START;
START = function (summary)
{
  SUMMARY = summary;
  printStatus(summary);
}

var _TEST = TEST;
TEST = function (section, expected, actual)
{
  SECTION = section;
  EXPECTED = expected;
  ACTUAL = actual;
  if (_TEST(section, expected, actual))
  {
    reportSuccess(section, expected, actual);
  }
}

var _TEST_XML = TEST_XML;
TEST_XML = function (section, expected, actual)
{
  SECTION = section;
  EXPECTED = expected;
  ACTUAL = actual;
  if (_TEST_XML(section, expected, actual))
  {
    reportSuccess(section, expected, actual);
  }
}

function gc()
{
}

function quit()
{
}

// override NL from shell.js.
// assume running either on unix with \n line endings
// or on windows under cygwin with \n line endings.

function NL()
{
  return '\n';
}

window.onerror = reportError;
