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
        File Name:      boolean-004.js
        Description:

        A java.lang.Boolean object should be read as a JavaScript JavaObject.
        To test this:

        1.  The object should have a method called booleanValue(), which should
            return the object's value.
        2.  The typeof the object should be "object"
        3.  The object should have a method called getClass(), which should
            return the class java.lang.Boolean.
        4.  Add a method to the object's prototype, called getJSClass() that
            is Object.prototype.toString.  Calling getJSClass() should return the
            object's internal [[Class]] property, which should be JavaObject.

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

    //  ToString of a JavaObject should return "[object JavaObject]"

    var E_JSCLASS = "[object JavaObject]";

    //  The class of this object is java.lang.Boolean.

    var E_JAVACLASS = java.lang.Class.forName( "java.lang.Boolean" );

    //  Create arrays of actual results (java_array) and expected results
    //  (test_array).

    var java_array = new Array();
    var test_array = new Array();

    var i = 0;

    //  Instantiate a new java.lang.Boolean object whose value is Boolean.TRUE
    java_array[i] = new JavaValue(  new java.lang.Boolean( true )  );
    test_array[i] = new TestValue(  "new java.lang.Boolean( true )",
                                    true );

    i++;

    //  Instantiate a new java.lang.Boolean object whose value is Boolean.FALSE
    java_array[i] = new JavaValue(  new java.lang.Boolean( true )  );
    test_array[i] = new TestValue(  "new java.lang.Boolean( true )",
                                    true );

    i++;

    for ( i = 0; i < java_array.length; i++ ) {
        CompareValues( java_array[i], test_array[i] );

    }

    test();

function CompareValues( javaval, testval ) {
    //  Check value
    testcases[testcases.length] = new TestCase( SECTION,
                                                "("+testval.description+").booleanValue()",
                                                testval.value,
                                                javaval.value );
    //  Check type, which should be E_TYPE
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
}
function JavaValue( value ) {
    //  java.lang.Object.getClass() returns the Java Object's class.
    this.javaclass = value.getClass();

    // Object.prototype.toString will show its JavaScript wrapper object.
//    value.__proto__.getJSClass = Object.prototype.toString;
//    this.jsclass = value.getJSClass();

    this.value  = value.booleanValue();
    this.type   = typeof value;
    return this;
}
function TestValue( description, value ) {
    this.description = description;
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
