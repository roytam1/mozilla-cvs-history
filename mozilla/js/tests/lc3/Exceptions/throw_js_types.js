/* The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is Mozilla Communicator client code, released March
 * 31, 1998.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation. Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 * 
 */
/**
    File Name:          throw_js_type.js
    Title:              Throw JS types as exceptions through Java
    Description:        Attempt to throw all of the basic JS primitive types
				ie. 	String
					Number
					Boolean
					Object
			as exceptions through Java. If wrapping/unwrapping
			occurs as expected, the original type will be
			preserved.

    Author:             Chris Cooper
    Email:              coop@netscape.com
    Date:               12/04/1998
*/

var SECTION = "LC3";
var TITLE   = "Throw JS types as exceptions through Java";
startTest();
var testcases = new Array();
var exception = "No exception thrown";
var result = "Failed";
var data_type = "no type";

/**************************
 * JS String
 *************************/
try {
    exception = Packages.com.netscape.javascript.qa.liveconnect.JSObjectEval.eval(this, "throw 'foo';");
} catch ( e ) {
    exception = e.toString();
    data_type = typeof e;
    if (exception == "foo")
	result = "Passed!";
}

testcases[tc++] = new TestCase("Throwing JS string through Java "+
			       "\n=> threw ("+data_type+") "+exception+" ",
			       "Passed!",
			       result );

/**************************
 * JS Number (int)
 *************************/
exception = "No exception thrown";
result = "Failed";
data_type = "no type";

try {
    exception = Packages.com.netscape.javascript.qa.liveconnect.JSObjectEval.eval(this, "throw 42;");
} catch ( e ) {
    exception = e.toString();
    data_type = typeof e;
    if (exception == "42")
	result = "Passed!";
}

testcases[tc++] = new TestCase("Throwing JS number (int) through Java "+
			       "\n=> threw ("+data_type+") "+exception+" ",
			       "Passed!",
			       result );

/**************************
 * JS Number (float)
 *************************/
exception = "No exception thrown";
result = "Failed";
data_type = "no type";

try {
    exception = Packages.com.netscape.javascript.qa.liveconnect.JSObjectEval.eval(this, "throw 4.2;");
} catch ( e ) {
    exception = e.toString();
    data_type = typeof e;
    if (exception == "4.2")
	result = "Passed!";
}

testcases[tc++] = new TestCase("Throwing JS number (float) through Java "+
			       "\n=> threw ("+data_type+") "+exception+" ",
			       "Passed!",
			       result );

/**************************
 * JS Boolean
 *************************/
exception = "No exception thrown";
result = "Failed";
data_type = "no type";

try {
    exception = Packages.com.netscape.javascript.qa.liveconnect.JSObjectEval.eval(this, "throw false;");
} catch ( e ) {
    exception = e.toString();
    data_type = typeof e;
    if (exception == "false")
	result = "Passed!";
}

testcases[tc++] = new TestCase("Throwing JS boolean through Java "+
			       "\n=> threw ("+data_type+") "+exception+" ",
			       "Passed!",
			       result );

/**************************
 * JS Object
 *************************/
exception = "No exception thrown";
result = "Failed";
data_type = "no type";

try {
    exception = Packages.com.netscape.javascript.qa.liveconnect.JSObjectEval.eval(this, "throw {a:5};");
} catch ( e ) {
    exception = e.toString();
    data_type = typeof e;
    result = "Passed!";
}

testcases[tc++] = new TestCase("Throwing JS Object through Java "+
			       "\n=> threw ("+data_type+") "+exception+" ",
			       "Passed!",
			       result );

/**************************
 * JS Object (Date)
 *************************/
exception = "No exception thrown";
result = "Failed";
data_type = "no type";

try {
    exception = Packages.com.netscape.javascript.qa.liveconnect.JSObjectEval.eval(this, "throw new Date();");
} catch ( e ) {
    exception = e.toString();
        data_type = typeof e;
        result = "Passed!";
}

testcases[tc++] = new TestCase("Throwing JS Object (Date)through Java "+
        		       "\n=> threw ("+data_type+") "+exception+" ",
        		       "Passed!",
        		       result );

/**************************
 * JS Object (String)
 *************************/
exception = "No exception thrown";
result = "Failed";
data_type = "no type";

try {
    exception = Packages.com.netscape.javascript.qa.liveconnect.JSObjectEval.eval(this, "throw new String();");
} catch ( e ) {
    exception = e.toString();
    data_type = typeof e;
    result = "Passed!";
}

testcases[tc++] = new TestCase("Throwing JS Object (String) through Java "+
			       "\n=> threw ("+data_type+") "+exception+" ",
			       "Passed!",
			       result );

/**************************
 * Undefined
 *************************/
exception = "No exception thrown";
result = "Failed";
data_type = "no type";

try {
    exception = Packages.com.netscape.javascript.qa.liveconnect.JSObjectEval.eval(this, "throw undefined");
} catch ( e ) {
    exception = "Exception";
    data_type = typeof e;
    result = "Passed!";
}

testcases[tc++] = new TestCase("Throwing undefined through Java "+
			       "\n=> threw ("+data_type+") "+exception+" ",
			       "Passed!",
			       result );

/**************************
 * null
 *************************/
exception = "No exception thrown";
result = "Failed";
data_type = "no type";

try {
    exception = Packages.com.netscape.javascript.qa.liveconnect.JSObjectEval.eval(this, "throw null;");
} catch ( e ) {
    exception = "Exception";
    data_type = typeof e;
    result = "Passed!";
}

testcases[tc++] = new TestCase("Throwing null through Java "+
			       "\n=> threw ("+data_type+") "+exception+" ",
			       "Passed!",
			       result );

test();


function test() {
    //  Iterate through testcases, and print results to the display.  You
    //  probably want to leave this block alone.  You don't really need to
    //  do this, but it helps if you want the test case results to be
    //  written to the display.

    for ( tc=0; tc < testcases.length; tc++ ) {
        testcases[tc].passed = writeTestCaseResult(
                            testcases[tc].expect,
                            testcases[tc].actual,
                            testcases[tc].description +" = "+
                            testcases[tc].actual );

        testcases[tc].reason += ( testcases[tc].passed ) ? "" : "wrong value ";
    }

    //  Leave the following line alone.  All tests must call stop tests just
    //  before they return the testcases object.  This lets Navigator know that
    //  the test has completed.

    stopTest();

    //  Leave the following alone.  All tests must return an array of
    //  TestCase objects.

    return ( testcases );
}
