/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#ifndef nsISecureEnv_h___
#define nsISecureEnv_h___

#include "nsISupports.h"
#include "nsIFactory.h"
#include "nsISecurityContext.h"
#include "jni.h"

enum jni_type 
{
    jobject_type = 0,
    jboolean_type,
    jbyte_type,
    jchar_type,
    jshort_type,
    jint_type,
    jlong_type,
    jfloat_type,
    jdouble_type,
    jvoid_type
};

class nsISecureEnv : public nsISupports {
public:

    /**
     * Create new Java object in LiveConnect.
     *
     * @param clazz      -- Java Class object.
     * @param methodID   -- Method id
     * @param args       -- arguments for invoking the constructor.
     * @param result     -- return new Java object.
     * @param ctx        -- security context 
     */
    NS_IMETHOD NewObject(/*[in]*/  jclass clazz, 
                         /*[in]*/  jmethodID methodID,
                         /*[in]*/  jvalue *args, 
                         /*[out]*/ jobject* result,
                         /*[in]*/  nsISecurityContext* ctx = NULL) = 0;
   
    /**
     * Invoke method on Java object in LiveConnect.
     *
     * @param type       -- Return type
     * @param obj        -- Java object.
     * @param methodID   -- method id
     * @param args       -- arguments for invoking the constructor.
     * @param result     -- return result of invocation.
     * @param ctx        -- security context 
     */
    NS_IMETHOD CallMethod(/*[in]*/  jni_type type,
                          /*[in]*/  jobject obj, 
                          /*[in]*/  jmethodID methodID,
                          /*[in]*/  jvalue *args, 
                          /*[out]*/ jvalue* result,
                          /*[in]*/  nsISecurityContext* ctx = NULL) = 0;

    /**
     * Invoke non-virtual method on Java object in LiveConnect.
     *
     * @param type       -- Return type
     * @param obj        -- Java object.
     * @param clazz      -- Class object
     * @param methodID   -- method id
     * @param args       -- arguments for invoking the constructor.
     * @param ctx        -- security context 
     * @param result     -- return result of invocation.
     */
    NS_IMETHOD CallNonvirtualMethod(/*[in]*/  jni_type type,
                                    /*[in]*/  jobject obj, 
                                    /*[in]*/  jclass clazz,
                                    /*[in]*/  jmethodID methodID,
                                    /*[in]*/  jvalue *args, 
                                    /*[out]*/ jvalue* result,
                                    /*[in]*/  nsISecurityContext* ctx = NULL) = 0;

    /**
     * Get a field on Java object in LiveConnect.
     *
     * @param type       -- Return type
     * @param obj        -- Java object.
     * @param fieldID    -- field id
     * @param result     -- return field value
     * @param ctx        -- security context 
     */
    NS_IMETHOD GetField(/*[in]*/  jni_type type,
                        /*[in]*/  jobject obj, 
                        /*[in]*/  jfieldID fieldID,
                        /*[out]*/ jvalue* result,
                        /*[in]*/  nsISecurityContext* ctx = NULL) = 0;

    /**
     * Set a field on Java object in LiveConnect.
     *
     * @param type       -- Return type
     * @param obj        -- Java object.
     * @param fieldID    -- field id
     * @param val        -- field value to set
     * @param ctx        -- security context 
     */
    NS_IMETHOD SetField(/*[in]*/ jni_type type,
                        /*[in]*/ jobject obj, 
                        /*[in]*/ jfieldID fieldID,
                        /*[in]*/ jvalue val,
                        /*[in]*/ nsISecurityContext* ctx = NULL) = 0;

    /**
     * Invoke static method on Java object in LiveConnect.
     *
     * @param type       -- Return type
     * @param clazz      -- Class object.
     * @param methodID   -- method id
     * @param args       -- arguments for invoking the constructor.
     * @param result     -- return result of invocation.
     * @param ctx        -- security context 
     */
    NS_IMETHOD CallStaticMethod(/*[in]*/  jni_type type,
                                /*[in]*/  jclass clazz,
                                /*[in]*/  jmethodID methodID,
                                /*[in]*/  jvalue *args, 
                                /*[out]*/ jvalue* result,
                                /*[in]*/  nsISecurityContext* ctx = NULL) = 0;

    /**
     * Get a static field on Java object in LiveConnect.
     *
     * @param type       -- Return type
     * @param clazz      -- Class object.
     * @param fieldID    -- field id
     * @param result     -- return field value
     * @param ctx        -- security context 
     */
    NS_IMETHOD GetStaticField(/*[in]*/  jni_type type,
                              /*[in]*/  jclass clazz, 
                              /*[in]*/  jfieldID fieldID, 
                              /*[out]*/ jvalue* result,
                              /*[in]*/  nsISecurityContext* ctx = NULL) = 0;


    /**
     * Set a static field on Java object in LiveConnect.
     *
     * @param type       -- Return type
     * @param clazz      -- Class object.
     * @param fieldID    -- field id
     * @param val        -- field value to set
     * @param ctx        -- security context 
     */
    NS_IMETHOD SetStaticField(/*[in]*/ jni_type type,
                              /*[in]*/ jclass clazz, 
                              /*[in]*/ jfieldID fieldID,
                              /*[in]*/ jvalue val,
                              /*[in]*/ nsISecurityContext* ctx = NULL) = 0;


    NS_IMETHOD GetVersion(/*[out]*/ jint* version) = 0;

    NS_IMETHOD DefineClass(/*[in]*/  const char* name,
                           /*[in]*/  jobject loader,
                           /*[in]*/  const jbyte *buf,
                           /*[in]*/  jsize len,
                           /*[out]*/ jclass* clazz) = 0;

    NS_IMETHOD FindClass(/*[in]*/  const char* name, 
                         /*[out]*/ jclass* clazz) = 0;

    NS_IMETHOD GetSuperclass(/*[in]*/  jclass sub,
                             /*[out]*/ jclass* super) = 0;

    NS_IMETHOD IsAssignableFrom(/*[in]*/  jclass sub,
                                /*[in]*/  jclass super,
                                /*[out]*/ jboolean* result) = 0;

    NS_IMETHOD Throw(/*[in]*/  jthrowable obj,
                     /*[out]*/ jint* result) = 0;

    NS_IMETHOD ThrowNew(/*[in]*/  jclass clazz,
                        /*[in]*/  const char *msg,
                        /*[out]*/ jint* result) = 0;

    NS_IMETHOD ExceptionOccurred(/*[out]*/ jthrowable* result) = 0;

    NS_IMETHOD ExceptionDescribe(void) = 0;

    NS_IMETHOD ExceptionClear(void) = 0;

    NS_IMETHOD FatalError(/*[in]*/ const char* msg) = 0;

    NS_IMETHOD NewGlobalRef(/*[in]*/  jobject lobj, 
                            /*[out]*/ jobject* result) = 0;

    NS_IMETHOD DeleteGlobalRef(/*[in]*/ jobject gref) = 0;

    NS_IMETHOD DeleteLocalRef(/*[in]*/ jobject obj) = 0;

    NS_IMETHOD IsSameObject(/*[in]*/  jobject obj1,
                            /*[in]*/  jobject obj2,
                            /*[out]*/ jboolean* result) = 0;

    NS_IMETHOD AllocObject(/*[in]*/  jclass clazz,
                           /*[out]*/ jobject* result) = 0;

    NS_IMETHOD GetObjectClass(/*[in]*/  jobject obj,
                              /*[out]*/ jclass* result) = 0;

    NS_IMETHOD IsInstanceOf(/*[in]*/  jobject obj,
                            /*[in]*/  jclass clazz,
                            /*[out]*/ jboolean* result) = 0;

    NS_IMETHOD GetMethodID(/*[in]*/  jclass clazz, 
                           /*[in]*/  const char* name,
                           /*[in]*/  const char* sig,
                           /*[out]*/ jmethodID* id) = 0;

    NS_IMETHOD GetFieldID(/*[in]*/  jclass clazz, 
                          /*[in]*/  const char* name,
                          /*[in]*/  const char* sig,
                          /*[out]*/ jfieldID* id) = 0;

    NS_IMETHOD GetStaticMethodID(/*[in]*/  jclass clazz, 
                                 /*[in]*/  const char* name,
                                 /*[in]*/  const char* sig,
                                 /*[out]*/ jmethodID* id) = 0;

    NS_IMETHOD GetStaticFieldID(/*[in]*/  jclass clazz, 
                                /*[in]*/  const char* name,
                                /*[in]*/  const char* sig,
                                /*[out]*/ jfieldID* id) = 0;

    NS_IMETHOD NewString(/*[in]*/  const jchar* unicode,
                         /*[in]*/  jsize len,
                         /*[out]*/ jstring* result) = 0;

    NS_IMETHOD GetStringLength(/*[in]*/  jstring str,
                               /*[out]*/ jsize* result) = 0;
    
    NS_IMETHOD GetStringChars(/*[in]*/  jstring str,
                              /*[in]*/  jboolean *isCopy,
                              /*[out]*/ const jchar** result) = 0;

    NS_IMETHOD ReleaseStringChars(/*[in]*/  jstring str,
                                  /*[in]*/  const jchar *chars) = 0;

    NS_IMETHOD NewStringUTF(/*[in]*/  const char *utf,
                            /*[out]*/ jstring* result) = 0;

    NS_IMETHOD GetStringUTFLength(/*[in]*/  jstring str,
                                  /*[out]*/ jsize* result) = 0;
    
    NS_IMETHOD GetStringUTFChars(/*[in]*/  jstring str,
                                 /*[in]*/  jboolean *isCopy,
                                 /*[out]*/ const char** result) = 0;

    NS_IMETHOD ReleaseStringUTFChars(/*[in]*/  jstring str,
                                     /*[in]*/  const char *chars) = 0;

    NS_IMETHOD GetArrayLength(/*[in]*/  jarray array,
                              /*[out]*/ jsize* result) = 0;

    NS_IMETHOD NewObjectArray(/*[in]*/  jsize len,
    					/*[in]*/  jclass clazz,
                        /*[in]*/  jobject init,
                        /*[out]*/ jobjectArray* result) = 0;

    NS_IMETHOD GetObjectArrayElement(/*[in]*/  jobjectArray array,
                                     /*[in]*/  jsize index,
                                     /*[out]*/ jobject* result) = 0;

    NS_IMETHOD SetObjectArrayElement(/*[in]*/  jobjectArray array,
                                     /*[in]*/  jsize index,
                                     /*[in]*/  jobject val) = 0;

    NS_IMETHOD NewArray(/*[in]*/ jni_type element_type,
                        /*[in]*/  jsize len,
                        /*[out]*/ jarray* result) = 0;

    NS_IMETHOD GetArrayElements(/*[in]*/  jni_type type,
                                /*[in]*/  jarray array,
                                /*[in]*/  jboolean *isCopy,
                                /*[out]*/ void* result) = 0;

    NS_IMETHOD ReleaseArrayElements(/*[in]*/ jni_type type,
                                    /*[in]*/ jarray array,
                                    /*[in]*/ void *elems,
                                    /*[in]*/ jint mode) = 0;

    NS_IMETHOD GetArrayRegion(/*[in]*/  jni_type type,
                              /*[in]*/  jarray array,
                              /*[in]*/  jsize start,
                              /*[in]*/  jsize len,
                              /*[out]*/ void* buf) = 0;

    NS_IMETHOD SetArrayRegion(/*[in]*/  jni_type type,
                              /*[in]*/  jarray array,
                              /*[in]*/  jsize start,
                              /*[in]*/  jsize len,
                              /*[in]*/  void* buf) = 0;

    NS_IMETHOD RegisterNatives(/*[in]*/  jclass clazz,
                               /*[in]*/  const JNINativeMethod *methods,
                               /*[in]*/  jint nMethods,
                               /*[out]*/ jint* result) = 0;

    NS_IMETHOD UnregisterNatives(/*[in]*/  jclass clazz,
                                 /*[out]*/ jint* result) = 0;

    NS_IMETHOD MonitorEnter(/*[in]*/  jobject obj,
                            /*[out]*/ jint* result) = 0;

    NS_IMETHOD MonitorExit(/*[in]*/  jobject obj,
                           /*[out]*/ jint* result) = 0;

    NS_IMETHOD GetJavaVM(/*[in]*/  JavaVM **vm,
                         /*[out]*/ jint* result) = 0;
};

#define NS_ISECUREENV_IID                               \
{   /* ca9148d0-598a-11d2-a1d4-00805f8f694d */          \
    0xca9148d0,                                         \
    0x598a,                                             \
    0x11d2,                                             \
    {0xa1, 0xd4, 0x00, 0x80, 0x5f, 0x8f, 0x69, 0x4d }   \
}

#endif // nsISecureEnv_h___
