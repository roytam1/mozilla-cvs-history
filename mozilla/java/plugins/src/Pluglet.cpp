/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- 
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Initial Developer of the Original Code is Sun Microsystems,
 * Inc. Portions created by Sun are Copyright (C) 1999 Sun Microsystems,
 * Inc. All Rights Reserved. 
 */
#include "Pluglet.h"
#include "PlugletEngine.h"
#include "PlugletLoader.h"
#include "PlugletInstance.h"
#include "string.h"

jmethodID Pluglet::createPlugletInstanceMID = NULL;
jmethodID Pluglet::initializeMID = NULL;  
jmethodID Pluglet::shutdownMID = NULL;


nsresult Pluglet::CreatePluginInstance(const char* aPluginMIMEType, void **aResult) {
    if(!aResult
       || Initialize() != NS_OK) {
	return NS_ERROR_FAILURE;
    }
    JNIEnv *env = PlugletEngine::GetJNIEnv();
    if(!env) {
	return NS_ERROR_FAILURE;
    }
    jstring jstr = env->NewStringUTF(aPluginMIMEType);
    jobject obj = env->CallObjectMethod(jthis,createPlugletInstanceMID, jstr);
    if (!obj) {
        return NS_ERROR_FAILURE;
    }
    if (env->ExceptionOccurred()) {
        env->ExceptionDescribe();
        return NS_ERROR_FAILURE;
    }
    *aResult = new PlugletInstance(obj);
    return NS_OK;
}

nsresult Pluglet::Initialize(void) {
    JNIEnv *env = PlugletEngine::GetJNIEnv();
    if(!env) {
        return NS_ERROR_FAILURE;
    }
    if (!initializeMID) { 
	jclass clazz = env->FindClass("org/mozilla/pluglet/Pluglet");
	if(!clazz) {
	    return NS_ERROR_FAILURE;
	}
	createPlugletInstanceMID = env->GetMethodID(clazz,"createPlugletInstance","(Ljava/lang/String;)Lorg/mozilla/pluglet/PlugletInstance;");
	if (!createPlugletInstanceMID) {
	    return NS_ERROR_FAILURE;
	}
	shutdownMID = env->GetMethodID(clazz,"shutdown","()V");
	if (!shutdownMID) {
	    return NS_ERROR_FAILURE;
	}   
	initializeMID = env->GetMethodID(clazz,"initialize","(Lorg/mozilla/pluglet/mozilla/PlugletManager;)V");
	if (!initializeMID) {
	    return NS_ERROR_FAILURE;
	}
	
    }
    if (!jthis) {
	jthis = PlugletLoader::GetPluglet(path);
	if (!jthis) {
	    return NS_ERROR_FAILURE;
	}
	env->CallVoidMethod(jthis,initializeMID,PlugletEngine::GetPlugletManager());
	if (env->ExceptionOccurred()) {
	    env->ExceptionDescribe();
	    return NS_ERROR_FAILURE;
	}
    }
    return NS_OK;
}

nsresult Pluglet::Shutdown(void) {
    
    if(!jthis) {
	return NS_ERROR_FAILURE;
    }
    JNIEnv *env = PlugletEngine::GetJNIEnv();
    if(!env) {
	return NS_ERROR_FAILURE;
    }
    env->CallVoidMethod(jthis,shutdownMID);
    if (env->ExceptionOccurred()) {
	env->ExceptionDescribe();
	return NS_ERROR_FAILURE;
    }
    return NS_OK;
}

nsresult Pluglet::GetMIMEDescription(const char* *result) {
    if(!result) {
	return NS_ERROR_FAILURE;
    }
    *result = mimeDescription;
    return (*result) ? NS_OK : NS_ERROR_FAILURE;
}

Pluglet::Pluglet(const char *mimeDescription, const char *path) {
    jthis = NULL;
    this->path = new char[strlen(path)+1];
    strcpy(this->path,path);
    this->mimeDescription = new char[strlen(mimeDescription)+1];
    strcpy(this->mimeDescription,mimeDescription);
    
}
 
Pluglet::~Pluglet(void) {
    if (path != NULL) {
        delete[] path;
    }
    if (mimeDescription != NULL) {
        delete[] mimeDescription;
    }
    JNIEnv *env = PlugletEngine::GetJNIEnv();
    if (env != NULL) {
        env->DeleteGlobalRef(jthis);
    }
}

Pluglet * Pluglet::Load(const char * path) {
    char * mime = PlugletLoader::GetMIMEDescription(path);
    Pluglet * result = NULL;
    if (mime) {
	result = new Pluglet(mime,path);
	//delete[] mime;  //nb we have a strange exception here 
    }
    return result;
}

/*
  1 if good
  0 if not good
 */
int Pluglet::Compare(const char *mimeType) {
    /* mimedDescription mimeTypes:extensions:description
       mimeTypes mimeType;mimeType;...
       extensions extension;extension;...
    */
    if (!mimeType) {
        return 0;
    }
    char * terminator = strchr(mimeDescription,':'); // terminator have to be not equal to NULL;
    char *p1 = mimeDescription;
    char *p2 = strchr(p1,';');
    while ( p1 != NULL && p1 < terminator ) {
        size_t n = sizeof(char) * ( ( (p2 == NULL) ? terminator : p2) - p1 );
        if (strncmp(p1,mimeType,n) == 0) {
            return 1;
        }
        p1 = p2 ;
        if (p2 != NULL) {
            p2 = strchr(++p2,';');
            p1++;
        }
    }
    return 0;
}


