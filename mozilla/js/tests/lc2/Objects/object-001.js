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
        File Name:      object-001.js
        Description:

        Given a JavaObject, calling the getClass() method of java.lang.Object
        should return the java.lang.Class of that object.

        To test this:

        1.  Create a JavaObject by instantiating a new object OR call
            a java method that returns a JavaObject.

        2.  Call getClass() on that object. Compare it to the result of
            java.lang.Class.forName( "<classname>" ).

        3.  Also compare the result of getClass() to the literal classname

        Note:  this test does not use the LiveConnect getClass function, which
        currently is not available.

        @author     christine@netscape.com
        @version    1.00
*/
    var SECTION = "LiveConnect Objects";
    var VERSION = "1_3";
    var TITLE   = "Getting the Class of JavaObjects";

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

    java_array[i] = new JavaValue(  new java.awt.Rectangle(1,2,3,4)  );
    test_array[i] = new TestValue(  "new java.awt.Rectangle(1,2,3,4)", "java.awt.Rectangle" );
    i++;

    java_array[i] = new JavaValue(  new java.io.PrintStream( java.lang.System.out ) );
    test_array[i] = new TestValue(  "new java.io.PrintStream(java.lang.System.out)", "java.io.PrintStream" );
    i++;

    java_array[i] = new JavaValue(  new java.lang.String("hello")  );
    test_array[i] = new TestValue(  "new java.lang.String('hello')", "java.lang.String" );
    i++;

    java_array[i] = new JavaValue(  new java.net.URL("http://home.netscape.com/")  );
    test_array[i] = new TestValue(  "new java.net.URL('http://home.netscape.com')", "java.net.URL" );
    i++;

/*
    java_array[i] = new JavaValue(  java.rmi.RMISecurityManager  );
    test_array[i] = new TestValue(  "java.rmi.RMISecurityManager" );
    i++;
    java_array[i] = new JavaValue(  java.text.DateFormat  );
    test_array[i] = new TestValue(  "java.text.DateFormat" );
    i++;
*/
    java_array[i] = new JavaValue(  new java.util.Vector()  );
    test_array[i] = new TestValue(  "new java.util.Vector()", "java.util.Vector" );
    i++;

/*
    java_array[i] = new JavaValue(  new Packages.com.netscape.javascript.Context()  );
    test_array[i] = new TestValue(  "new Packages.com.netscape.javascript.Context()", "com.netscape.javascript.Context" );
    i++;

    java_array[i] = new JavaValue(  Packages.com.netscape.javascript.examples.Shell  );
    test_array[i] = new TestValue(  "Packages.com.netscape.javascript.examples.Shell" );
    i++;
*/

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
                                                testval.jsclass,
                                                javaval.jsclass );
*/
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
//  LC2 does not support the __proto__ property in Java objects
    // Object.prototype.toString will show its JavaScript wrapper object.
//    value.__proto__.getJSClass = Object.prototype.toString;
//    this.jsclass = value.getJSClass();
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
