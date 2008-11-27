/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * 
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *  
 * The Original Code is The Waterfall Java Plugin Module
 *  
 * The Initial Developer of the Original Code is Sun Microsystems Inc
 * Portions created by Sun Microsystems Inc are Copyright (C) 2001
 * All Rights Reserved.
 * 
 * $Id$
 * 
 * Contributor(s):
 * 
 *     Nikolay N. Igotti <nikolay.igotti@Sun.Com>
 */

/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class sun_jvmp_mozilla_MozillaPeerFactory */

#ifndef _Included_sun_jvmp_mozilla_MozillaPeerFactory
#define _Included_sun_jvmp_mozilla_MozillaPeerFactory
#ifdef __cplusplus
extern "C" {
#endif
/* Inaccessible static: initialized */
/*
 * Class:     sun_jvmp_mozilla_MozillaPeerFactory
 * Method:    nativeGetProxyInfoForURL
 * Signature: (Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_sun_jvmp_mozilla_MozillaPeerFactory_nativeGetProxyInfoForURL
  (JNIEnv *, jobject, jstring);

/*
 * Class:     sun_jvmp_mozilla_MozillaPeerFactory
 * Method:    nativeHandleCall
 * Signature: (IJ)I
 */
JNIEXPORT jint JNICALL Java_sun_jvmp_mozilla_MozillaPeerFactory_nativeHandleCall
  (JNIEnv *, jobject, jint, jlong);

#ifdef __cplusplus
}
#endif
#endif