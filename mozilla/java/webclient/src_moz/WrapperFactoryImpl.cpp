/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * The Original Code is RaptorCanvas.
 *
 * The Initial Developer of the Original Code is Kirk Baker and
 * Ian Wilkinson. Portions created by Kirk Baker and Ian Wilkinson are
 * Copyright (C) 1999 Kirk Baker and Ian Wilkinson. All
 * Rights Reserved.
 *
 * Contributor(s): Kirk Baker <kbaker@eb.com>
 *               Ian Wilkinson <iw@ennoble.com>
 *               Mark Lin <mark.lin@eng.sun.com>
 *               Mark Goddard
 *               Ed Burns <edburns@acm.org>
 *               Ashutosh Kulkarni <ashuk@eng.sun.com>
 *               Ann Sunhachawee
 */

#include "org_mozilla_webclient_impl_wrapper_0005fnative_WrapperFactoryImpl.h"

#include "ns_util.h"
#include "nsCRT.h" // for nsCRT::strcmp

#include <nsWidgetsCID.h> // for NS_APPSHELL_CID
#include <nsIComponentManager.h> // for do_CreateInstance
#include <nsILocalFile.h> 
#include <nsEmbedAPI.h>  // for NS_InitEmbedding
#include <nsIComponentRegistrar.h>
#include <nsDependentString.h> // for nsDependentCString

#include <prenv.h> // for PR_SetEnv

#include <nsIAppShell.h>

#ifdef XP_UNIX
#include <unistd.h> // for getpid
#endif
#ifdef XP_PC
#include <windows.h> // for GetCurrentProcessId()
#endif

#include "NativeBrowserControl.h"

static NS_DEFINE_CID(kAppShellCID, NS_APPSHELL_CID);

//
// global data
//

PRLogModuleInfo *prLogModuleInfo = NULL; // declared in ns_globals.h

const char *gImplementedInterfaces[] = {
        "webclient.WindowControl",
        "webclient.Navigation",
        "webclient.CurrentPage",
        "webclient.History",
        "webclient.EventRegistration",
        "webclient.Bookmarks",
        "webclient.Preferences",
        "webclient.ProfileManager",
        nsnull
        };

//
// Local Functions
//

//
// Functions to hook into mozilla
// 

JNIEXPORT jint JNICALL 
Java_org_mozilla_webclient_impl_wrapper_1native_WrapperFactoryImpl_nativeAppInitialize(
										JNIEnv *env, jobject obj, jstring verifiedBinDirAbsolutePath)
{
    prLogModuleInfo = PR_NewLogModule("webclient");
    const char *nativePath = nsnull;
    WebclientContext *result = new WebclientContext();
    nsresult rv;
    nsCOMPtr<nsILocalFile> binDir;

    // PENDING(edburns): We need this for rdf_getChildCount
    PR_SetEnv("XPCOM_CHECK_THREADSAFE=0");

    // 
    // create an nsILocalFile from our argument
    //
    
    PR_LOG(prLogModuleInfo, PR_LOG_DEBUG, 
           ("WrapperFactoryImpl_nativeAppInitialize: entering\n"));

    nativePath = (const char *) ::util_GetStringUTFChars(env, 
                                                   verifiedBinDirAbsolutePath);
    PR_LOG(prLogModuleInfo, PR_LOG_DEBUG, 
           ("WrapperFactoryImpl_nativeAppInitialize: nativeBinDir: %s\n", 
            nativePath));
    
    if (nativePath) {
        rv = NS_NewNativeLocalFile(nsDependentCString(nativePath), 1, 
                                   getter_AddRefs(binDir));
        PR_LOG(prLogModuleInfo, PR_LOG_DEBUG, 
               ("WrapperFactoryImpl_nativeAppInitialize: NS_NewNativeLocalFile rv: %d\n", 
                rv));
        
        if (NS_FAILED(rv)) {
            ::util_ThrowExceptionToJava(env, 
                                  "Can't get nsILocalFile from bin directory");
            return (jint) rv;
        }
    }
    ::util_ReleaseStringUTFChars(env, verifiedBinDirAbsolutePath, nativePath);
    
    //
    // Make the all important NS_InitEmbedding call
    // 
    
    rv = NS_InitEmbedding(binDir, nsnull);
    PR_LOG(prLogModuleInfo, PR_LOG_DEBUG, 
          ("WrapperFactoryImpl_nativeAppInitialize: NS_InitEmbedding rv: %d\n",
           rv));

    if (NS_FAILED(rv)) {
        ::util_ThrowExceptionToJava(env, "NS_InitEmbedding() failed.");
        return (jint) rv;
    }

    // the rest of the startup tasks are coordinated from the java side.
    PR_LOG(prLogModuleInfo, PR_LOG_DEBUG, 
           ("WrapperFactoryImpl_nativeAppInitialize: exiting\n"));

    char propValue[50];
    ::util_getSystemProperty(env, "native.waitForDebugger", propValue, 50);
    if (nsnull != propValue[0] &&
        0 < nsCRT::strlen(propValue)) {
#ifdef XP_UNIX
        pid_t pid = getpid();
        printf("++++++++++++++++debug: pid is: %d\n", pid);
#endif
#ifdef XP_PC
        printf("++++++++++++++++debug: pid is: %d\n", GetCurrentProcessId());
#endif
        fflush(stdout);
        PR_Sleep(700000);
    }
    
    // Store our pointer to the global vm
    if (nsnull == gVm) { // declared in ../src_share/jni_util.h
        ::util_GetJavaVM(env, &gVm);  // save this vm reference
    }

    util_InitializeShareInitContext(env, &(result->shareContext));

    return (jint) result;
}

JNIEXPORT void JNICALL 
Java_org_mozilla_webclient_impl_wrapper_1native_WrapperFactoryImpl_nativeAppSetup
(JNIEnv *env, jobject obj, jint nativeContext)
{
    PR_LOG(prLogModuleInfo, PR_LOG_DEBUG, 
           ("WrapperFactoryImpl_nativeAppSetup: entering\n"));
    nsresult rv;
    WebclientContext *wcContext = (WebclientContext *) nativeContext;
    
    PR_ASSERT(wcContext);

    nsCOMPtr<nsIComponentRegistrar> cr;
    rv = NS_GetComponentRegistrar(getter_AddRefs(cr));
    if (NS_FAILED(rv)) {
        ::util_ThrowExceptionToJava(env, 
                                    "Failed to create Component Registrar");
        return;
    }

    // appshell
    // XXX startup appshell service?
    // XXX create offscreen window for appshell service?
    // XXX remove X prop from offscreen window?
    
    nsCOMPtr<nsIAppShell> appShell;
    appShell = do_CreateInstance(kAppShellCID);
    if (!appShell) {
        ::util_ThrowExceptionToJava(env, 
                                    "Failed to create AppShell");
        return;
    }
    wcContext->sAppShell = appShell.get();
    NS_ADDREF(wcContext->sAppShell);

    rv = wcContext->sAppShell->Create(0, nsnull);
    PR_LOG(prLogModuleInfo, PR_LOG_DEBUG, 
           ("WrapperFactoryImpl_nativeAppSetup: AppShell create rv: %d\n",
            rv));

    if (NS_FAILED(rv)) {
        ::util_ThrowExceptionToJava(env, 
                                    "Failed to create AppShell");
        return;
    }
    rv = wcContext->sAppShell->Spinup();
    PR_LOG(prLogModuleInfo, PR_LOG_DEBUG, 
           ("WrapperFactoryImpl_nativeAppSetup: AppShell spinup rv: %d\n",
            rv));
    if (NS_FAILED(rv)) {
        ::util_ThrowExceptionToJava(env, 
                                    "Failed to Spinup AppShell");
        return;
    }

    PR_LOG(prLogModuleInfo, PR_LOG_DEBUG, 
           ("WrapperFactoryImpl_nativeAppSetup: exiting\n"));

}


JNIEXPORT void JNICALL 
Java_org_mozilla_webclient_impl_wrapper_1native_WrapperFactoryImpl_nativeTerminate
(JNIEnv *env, jobject obj, jint nativeContext)
{
    PR_LOG(prLogModuleInfo, PR_LOG_DEBUG, 
           ("WrapperFactoryImpl_nativeTerminate: entering\n"));
    nsresult rv;
    WebclientContext *wcContext = (WebclientContext *) nativeContext;
    
    PR_ASSERT(wcContext);

    if (wcContext->sAppShell) {
        rv = wcContext->sAppShell->Spindown();
        PR_LOG(prLogModuleInfo, PR_LOG_DEBUG, 
               ("WrapperFactoryImpl_nativeTerminate: Spindown rv: %d\n",
                rv));

        NS_RELEASE(wcContext->sAppShell);
        wcContext->sAppShell = nsnull;
    }
    PR_ASSERT(nsnull == wcContext->sProfile);
    PR_ASSERT(nsnull == wcContext->sProfileInternal);

    util_DeallocateShareInitContext(env, &(wcContext->shareContext));

    delete wcContext;

    // PENDING(edburns): do the rest of the stuff from
    // mozilla/embedding/browser/gtk/src/EmbedPrivate.cpp::PopStartup(void),
    // and NativeEventThread.cpp

    // shut down XPCOM/Embedding
    rv = NS_TermEmbedding();
    PR_LOG(prLogModuleInfo, PR_LOG_DEBUG, 
          ("WrapperFactoryImpl_nativeTerminate: NS_TermEmbedding rv: %d\n",
           rv));

    if (NS_FAILED(rv)) {
        ::util_ThrowExceptionToJava(env, "NS_TermEmbedding() failed.");
    }

    PR_LOG(prLogModuleInfo, PR_LOG_DEBUG, 
           ("WrapperFactoryImpl_nativeTerminate: exiting\n"));
}

JNIEXPORT jboolean JNICALL 
Java_org_mozilla_webclient_impl_wrapper_1native_WrapperFactoryImpl_nativeDoesImplement
(JNIEnv *env, jobject obj, jstring interfaceName)
{
    const char *iName = (const char *) ::util_GetStringUTFChars(env, 
                                                                interfaceName);
    jboolean result = JNI_FALSE;
    
    int i = 0;

    if (nsnull == iName) {
        return result;
    }

    while (nsnull != gImplementedInterfaces[i]) {
        if (0 == nsCRT::strcmp(gImplementedInterfaces[i++], iName)) {
            result = JNI_TRUE;
            break;
        }
    }
    ::util_ReleaseStringUTFChars(env, interfaceName, iName);
    
    return result;
}

JNIEXPORT jint JNICALL Java_org_mozilla_webclient_impl_wrapper_1native_WrapperFactoryImpl_nativeCreateBrowserControl
(JNIEnv *env, jobject obj, jint nativeContext) {
    NativeBrowserControl* initContext = new NativeBrowserControl();

    return (jint) initContext;
}

JNIEXPORT jint JNICALL Java_org_mozilla_webclient_impl_wrapper_1native_WrapperFactoryImpl_nativeDestroyBrowserControl
(JNIEnv *env, jobject obj, jint nativeContext, jint nativeBrowserControl) {
    NativeBrowserControl* initContext = 
        (NativeBrowserControl *)nativeBrowserControl;
    delete initContext;
    return 0;
}

