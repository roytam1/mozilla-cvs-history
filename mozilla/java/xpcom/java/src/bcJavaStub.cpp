/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Sun Microsystems,
 * Inc. Portions created by Sun are
 * Copyright (C) 1999 Sun Microsystems, Inc. All
 * Rights Reserved.
 *
 * Contributor(s):
 * Igor Kushnirskiy <idk@eng.sun.com>
 */


#include "nscore.h"
#include "xptcall.h"
#include "bcJavaStub.h"
#include "nsIInterfaceInfo.h"
#include "nsIInterfaceInfoManager.h"
#include "bcJavaMarshalToolkit.h"
#include "bcJavaGlobal.h"
#include "bcIIDJava.h"

jclass bcJavaStub::objectClass = NULL;
jclass bcJavaStub::utilitiesClass = NULL;
jmethodID bcJavaStub::callMethodByIndexMID = NULL;

bcJavaStub::bcJavaStub(jobject obj) {
    printf("--bcJavaStub::bcJavaStub \n");
    if (!obj) {
        printf("--bcJavaStub::bcJavaStub obj== 0\n");
        return;
    }
    JNIEnv * env = bcJavaGlobal::GetJNIEnv();
    object = env->NewGlobalRef(obj);
}


bcJavaStub::~bcJavaStub() {
    bcJavaGlobal::GetJNIEnv()->DeleteGlobalRef(object);
}

void bcJavaStub::Dispatch(bcICall *call) {
    //sigsend(P_PID, getpid(),SIGINT);
    JNIEnv * env = bcJavaGlobal::GetJNIEnv();
    bcIID iid; bcOID oid; bcMID mid;
    jobjectArray args;
    call->GetParams(&iid, &oid, &mid);
    nsIInterfaceInfo *interfaceInfo;
    nsIInterfaceInfoManager* iimgr;
    if(iimgr = XPTI_GetInterfaceInfoManager()) {
        if (NS_FAILED(iimgr->GetInfoForIID(&iid, &interfaceInfo))) {
            return;  //nb exception handling
        }
        NS_RELEASE(iimgr);
    } else {
        return;
    }
    nsXPTMethodInfo* info;
    interfaceInfo->GetMethodInfo(mid,(const nsXPTMethodInfo **)&info);
    PRUint32 paramCount = info->GetParamCount();
    args = env->NewObjectArray(paramCount, objectClass,NULL);    
    bcJavaMarshalToolkit * mt = new bcJavaMarshalToolkit(mid, interfaceInfo, args, env,1, call->GetORB());
    bcIUnMarshaler * um = call->GetUnMarshaler();   
    mt->UnMarshal(um);
    if (!objectClass) {
        Init();
        if (!objectClass) {
            return;
        }
    }
    jobject jiid = bcIIDJava::GetObject(&iid);
    bcJavaGlobal::GetJNIEnv()->CallStaticObjectMethod(utilitiesClass, callMethodByIndexMID, object, jiid, (jint)mid, args);
    //nb return value; excepion handling
    bcIMarshaler * m = call->GetMarshaler();     //nb **  to do
    mt->Marshal(m);
    return;
}


void bcJavaStub::Init() {
    JNIEnv * env = bcJavaGlobal::GetJNIEnv();
    objectClass = (jclass)env->NewGlobalRef(env->FindClass("java/lang/Object"));
    if (env->ExceptionOccurred()) {
        env->ExceptionDescribe();
        return;
    }

    utilitiesClass = (jclass)env->NewGlobalRef(env->FindClass("org/mozilla/xpcom/Utilities"));
    if (env->ExceptionOccurred()) {
        env->ExceptionDescribe();
        return;
    }
    callMethodByIndexMID = env->GetStaticMethodID(utilitiesClass,"callMethodByIndex","(Ljava/lang/Object;Lorg/mozilla/xpcom/IID;I[Ljava/lang/Object;)Ljava/lang/Object;");
    if (env->ExceptionOccurred()) {
        env->ExceptionDescribe();
        return;
    }
}





