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
        File Name:      method-001.js
        Description:

        Call a static method of an object and verify return value.
        This is covered more thoroughly in the type conversion test cases.
        This only covers cases in which JavaObjects are returned.

        @author     christine@netscape.com
        @version    1.00
*/
    var SECTION = "LiveConnect Objects";
    var VERSION = "1_3";
    var TITLE   = "Calling Static Methods";

    var testcases = new Array();

    startTest();
    writeHeaderToLog( SECTION + " "+ TITLE);

    //  All JavaObjects are of the type "object"

    var E_TYPE = "object";

    //  All JavaObjects [[Class]] property is JavaObject
    var E_JSCLASS = "[object JavaObject]";

    //  Create arrays of actual results (java_array) and
    //  expected results (test_array).

    var java_array = new Array();
    var test_array = new Array();

    var i = 0;

    java_array[i] = new JavaValue(  java.lang.String.valueOf(true)  );
    test_array[i] = new TestValue(  "java.lang.String.valueOf(true)",
        "object", "java.lang.String", "true" );

    i++;

    java_array[i] = new JavaValue( java.awt.Color.getHSBColor(0.0, 0.0, 0.0) );
    test_array[i] = new TestValue( "java.awt.Color.getHSBColor(0.0, 0.0, 0.0)",
        "object", "java.awt.Color", "java.awt.Color[r=0,g=0,b=0]" );

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
/*
    //  Check JavaScript class, which should be E_JSCLASS
    testcases[testcases.length] = new TestCase( SECTION,
                                                "(" + testval.description +" ).getJSClass()",
                                                E_JSCLASS,
                                                javaval.jsclass );
*/
    // Check the JavaClass, which should be the same as the result as Class.forName(description).
    testcases[testcases.length] = new TestCase( SECTION,
                                                "("+testval.description +").getClass().equals( " +
                                                "java.lang.Class.forName( '" + testval.classname +
                                                "' ) )",
                                                true,
                                                (javaval.javaclass).equals( testval.javaclass ) );
    // check the string value
    testcases[testcases.length] = new TestCase(
        SECTION,
        "("+testval.description+") +''",
        testval.stringval,
        javaval.value +"" );
}
function JavaValue( value ) {
    this.type   = typeof value;
    this.value = value;
//  LC2 does not support the __proto__ property in Java objects
//  Object.prototype.toString will show its JavaScript wrapper object.
//    value.__proto__.getJSClass = Object.prototype.toString;
//    this.jsclass = value.getJSClass();
    this.javaclass = value.getClass();

    return this;
}
function TestValue( description, type, classname, stringval ) {
    this.description = description;
    this.type =  type;
    this.classname = classname;
    this.javaclass = java.lang.Class.forName( classname );
    this.stringval = stringval;

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
