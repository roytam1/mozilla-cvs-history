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
        File Name:      method-003.js
        Description:

        JavaMethod objects are of the type "function" since they implement the
        [[Call]] method.


        @author     christine@netscape.com
        @version    1.00
*/
    var SECTION = "LiveConnect Objects";
    var VERSION = "1_3";
    var TITLE   = "Type and Class of Java Methods";

    var testcases = new Array();

    startTest();
    writeHeaderToLog( SECTION + " "+ TITLE);

    //  All JavaMethods are of the type "function"
    var E_TYPE = "function";

    //  All JavaMethods [[Class]] property is Function
    var E_JSCLASS = "[object Function]";

    //  Create arrays of actual results (java_array) and
    //  expected results (test_array).

    var java_array = new Array();
    var test_array = new Array();

    var i = 0;

    java_array[i] = new JavaValue(  java.lang.System.out.println  );
    test_array[i] = new TestValue(  "java.lang.System.out.println" );
    i++;

    for ( i = 0; i < java_array.length; i++ ) {
        CompareValues( java_array[i], test_array[i] );
    }

    test();

function CompareValues( javaval, testval ) {
    //  Check type, which should be E_TYPE
    testcases[testcases.length] = new TestCase( SECTION,
                                                "typeof (" + testval.description +" )",
                                                testval.type,
                                                javaval.type );

    //  Check JavaScript class, which should be E_JSCLASS
    testcases[testcases.length] = new TestCase( SECTION,
                                                "(" + testval.description +" ).getJSClass()",
                                                testval.jsclass,
                                                javaval.jsclass );
}
function JavaValue( value ) {
    this.type   = typeof value;
    // Object.prototype.toString will show its JavaScript wrapper object.
    value.getJSClass = Object.prototype.toString;
    this.jsclass = value.getJSClass();
    return this;
}
function TestValue( description  ) {
    this.description = description;
    this.type =  E_TYPE;
    this.jsclass = E_JSCLASS;
    return this;
}
function test() {
    for ( tc=0; tc < testcases.length; tc++ ) {
        testcases[tc].passed = writeTestCaseResult(
                            testcases[tc].expect,
                            testcases[tc].actual,
                            testcases[tc].description +" = "+
                            testcases[tc].actual );

        testcases[tc].reason += ( testcases[tc].passed ) ? "" : "wrong value ";
    }
    stopTest();
    return ( testcases );
}
