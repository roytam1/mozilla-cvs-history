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
        File Name:      boolean-005.js
        Description:

        A java.lang.Boolean object should be read as a JavaScript JavaObject.

        @author     christine@netscape.com
        @version    1.00
*/
    var SECTION = "LiveConnect";
    var VERSION = "1_3";
    var TITLE   = "Java Boolean Object to JavaScript Object";

    var testcases = new Array();

    startTest();
    writeHeaderToLog( SECTION + " "+ TITLE);

    //  In all test cases, the expected type is "object"

    var E_TYPE = "object";

    //  The JavaScrpt [[Class]] of a JavaObject should be JavaObject"

    var E_JSCLASS = "[object JavaObject]";

    //  The Java class of this object is java.lang.Boolean.

    var E_JAVACLASS = java.lang.Class.forName( "java.lang.Boolean" );

    //  Create arrays of actual results (java_array) and expected results
    //  (test_array).

    var java_array = new Array();
    var test_array = new Array();

    var i = 0;

    // Test for java.lang.Boolean.FALSE, which is a Boolean object.
    java_array[i] = new JavaValue(  java.lang.Boolean.FALSE  );
    test_array[i] = new TestValue(  "java.lang.Boolean.FALSE",
                                    false );

    i++;

    // Test for java.lang.Boolean.TRUE, which is a Boolean object.
    java_array[i] = new JavaValue(  java.lang.Boolean.TRUE  );
    test_array[i] = new TestValue(  "java.lang.Boolean.TRUE",
                                    true );

    i++;

    for ( i = 0; i < java_array.length; i++ ) {
        CompareValues( java_array[i], test_array[i] );

    }

    test();

function CompareValues( javaval, testval ) {
    //  Check booleanValue()
    testcases[testcases.length] = new TestCase( SECTION,
                                                "("+testval.description+").booleanValue()",
                                                testval.value,
                                                javaval.value );
    //  Check typeof, which should be E_TYPE
    testcases[testcases.length] = new TestCase( SECTION,
                                                "typeof (" + testval.description +")",
                                                testval.type,
                                                javaval.type );
/*
    //  Check JavaScript class, which should be E_JSCLASS
    testcases[testcases.length] = new TestCase( SECTION,
                                                "(" + testval.description +").getJSClass()",
                                                testval.jsclass,
                                                javaval.jsclass );
*/
    //  Check Java class, which should equal() E_JAVACLASS
    testcases[testcases.length] = new TestCase( SECTION,
                                                "(" + testval.description +").getClass().equals( " + E_JAVACLASS +" )",
                                                true,
                                                javaval.javaclass.equals( testval.javaclass ) );

    // Check string representation
    testcases[testcases.length] = new TestCase( SECTION,
        "("+ testval.description+") + ''",
        testval.string,
        javaval.string );
}
function JavaValue( value ) {
    //  java.lang.Object.getClass() returns the Java Object's class.
    this.javaclass = value.getClass();


//  __proto__ of Java objects is not supported in LC2.
// Object.prototype.toString will show its JavaScript wrapper object.
//    value.__proto__.getJSClass = Object.prototype.toString;
//    this.jsclass = value.getJSClass();

    this.string = value + "";
    print( this.string );
    this.value  = value.booleanValue();
    this.type   = typeof value;

    return this;
}
function TestValue( description, value ) {
    this.description = description;
    this.string = String( value );
    this.value = value;
    this.type =  E_TYPE;
    this.javaclass = E_JAVACLASS;
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
