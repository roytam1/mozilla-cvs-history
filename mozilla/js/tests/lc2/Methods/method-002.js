/* The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 * 
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
 * License for the specific language governing rights and limitations
 * under the License.
 * 
 * The Original Code is Mozilla Communicator client code, released March
 * 31, 1998.
 * 
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation. Portions created by Netscape are Copyright (C) 1998
 * Netscape Communications Corporation. All Rights Reserved.
 * 
 */
/**
        File Name:      method-002.js
        Description:

        Call a method of a JavaObject instance and verify that return value.
        This is covered more thouroughly in the type conversion test cases.

        @author     christine@netscape.com
        @version    1.00
*/
    var SECTION = "LiveConnect Objects";
    var VERSION = "1_3";
    var TITLE   = "Invoking Java Methods";

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

    // method returns an object

    var rect = new java.awt.Rectangle(1,2,3,4);
    var size = rect.getSize();

    testcases[testcases.length] = new TestCase(
        SECTION,
        "var size = (new java.awt.Rectangle(1,2,3,4)).getSize(); "+
        "size.getClass().equals(java.lang.Class.forName(\""+
        "java.awt.Dimension\"))",
        true,
        size.getClass().equals(java.lang.Class.forName("java.awt.Dimension")));

    testcases[testcases.length] = new TestCase(
        SECTION,
        "size.width",
        3,
        size.width );

    testcases[testcases.length] = new TestCase(
        SECTION,
        "size.height",
        4,
        size.height );

    // method returns void
    var r = rect.setSize(5,6);

    testcases[testcases.length] = new TestCase(
        SECTION,
        "var r = rect.setSize(5,6); r",
        void 0,
        r );

    // method returns a string

    var string = new java.lang.String( "     hello     " );
    s = string.trim()

    testcases[testcases.length] = new TestCase(
        SECTION,
        "var string = new java.lang.String(\"     hello     \"); "+
        "var s = string.trim(); s.getClass().equals("+
        "java.lang.Class.forName(\"java.lang.String\")",
        true,
        s.getClass().equals(java.lang.Class.forName("java.lang.String")) );

    // method returns an int
    testcases[testcases.length] = new TestCase(
        SECTION,
        "s.length()",
        5,
        s.length() );

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
    // Check the JavaClass, which should be the same as the result as Class.forName(description).
    testcases[testcases.length] = new TestCase( SECTION,
                                                testval.description +".getClass().equals( " +
                                                "java.lang.Class.forName( '" + testval.classname +
                                                "' ) )",
                                                true,
                                                javaval.javaclass.equals( testval.javaclass ) );
}
function JavaValue( value ) {
    this.type   = typeof value;
    // Object.prototype.toString will show its JavaScript wrapper object.
    value.__proto__.getJSClass = Object.prototype.toString;
    this.jsclass = value.getJSClass();
    this.javaclass = value.getClass();
    return this;
}
function TestValue( description, classname ) {
    this.description = description;
    this.classname = classname;
    this.type =  E_TYPE;
    this.jsclass = E_JSCLASS;
    this.javaclass = java.lang.Class.forName( classname );
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
