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
/* -*- Mode: java; tab-width: 8 -*-
 * Copyright � 1997, 1998 Netscape Communications Corporation,
 * All Rights Reserved.
 */
/**
 *  JavaScript to Java type conversion.
 *
 *  This test passes JavaScript number values to several Java methods
 *  that expect arguments of various types, and verifies that the value is
 *  converted to the correct value and type.
 *
 *  This tests instance methods, and not static methods.
 *
 *  Running these tests successfully requires you to have
 *  com.netscape.javascript.qa.liveconnect.DataTypeClass on your classpath.
 *
 *  Specification:  Method Overloading Proposal for Liveconnect 3.0
 *
 *  @author: christine@netscape.com
 *
 */
    var SECTION = "JavaObject to float";
    var VERSION = "1_4";
    var TITLE   = "LiveConnect 3.0 JavaScript to Java Data Type Conversion " +
                    SECTION;
    var BUGNUMBER = "335899";

    startTest();

    var dt = new DT();

    var a = new Array();
    var i = 0;

    // 1.  If the object does not have a doubleValue() method, a runtime error
    //     occurs.
    // 2. Call the object's doubleValue() method
    // 3. Convert Result(2) to Java numeric type. Round to float precision.
    //  Unrepresentably large numbers are converted to +/-Infinity.

    var newValue = Math.random();
    a[i++] = new TestObject (
         "dt.PUB_DOUBLE_REPRESENTATION = " +newValue,
         "dt.PUB_DOUBLE_REPRESENTATION",
         "new java.lang.Float( dt ).doubleValue()",
         "typeof new java.lang.Float(dt).doubleValue()",
         newValue,
         "number" );

    a[i++] = new TestObject (
         "dt.PUB_DOUBLE_REPRESENTATION = java.lang.Double.MIN_VALUE",
         "dt.PUB_DOUBLE_REPRESENTATION",
         "new java.lang.Float( dt ).doubleValue()",
         "typeof new java.lang.Float(dt).doubleValue()",
         java.lang.Double.MIN_VALUE,
         "number" );

    a[i++] = new TestObject (
         "dt.PUB_DOUBLE_REPRESENTATION = java.lang.Double.MAX_VALUE;" +
         "dt.setFloat( dt )",
         "dt.PUB_FLOAT",
         "dt.getFloat()",
         "typeof dt.getFloat()",
         Infinity,
         "number" );

    a[i++] = new TestObject (
         "dt.PUB_DOUBLE_REPRESENTATION = -java.lang.Double.MAX_VALUE;" +
         "dt.setFloat( dt )",
         "dt.PUB_FLOAT",
         "dt.getFloat()",
         "typeof dt.getFloat()",
         -Infinity,
         "number" );

    a[i++] = new TestObject (
         "dt.PUB_DOUBLE_REPRESENTATION = java.lang.Float.MAX_VALUE * 2;" +
         "dt.setFloat( dt )",
         "dt.PUB_FLOAT",
         "dt.getFloat()",
         "typeof dt.getFloat()",
         Infinity,
         "number" );

    for ( i = 0; i < a.length; i++ ) {
        testcases[testcases.length] = new TestCase(
            a[i].description +"; "+ a[i].javaFieldName,
            a[i].jsValue,
            a[i].javaFieldValue );

        testcases[testcases.length] = new TestCase(
            a[i].description +"; " + a[i].javaMethodName,
            a[i].jsValue,
            a[i].javaMethodValue );

        testcases[testcases.length] = new TestCase(
            a[i].javaTypeName,
            a[i].jsType,
            a[i].javaTypeValue );

    }

    test();

function TestObject( description, javaField, javaMethod, javaType,
    jsValue, jsType )
{
    eval (description );

    this.description = description;
    this.javaFieldName = javaField;
    this.javaFieldValue = eval( javaField );
    this.javaMethodName = javaMethod;
    this.javaMethodValue = eval( javaMethod );
    this.javaTypeName = javaType,
    this.javaTypeValue = typeof this.javaFieldValue;

    this.jsValue   = jsValue;
    this.jsType      = jsType;
}
