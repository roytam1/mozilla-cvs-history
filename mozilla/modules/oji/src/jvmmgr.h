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

#ifndef jvmmgr_h___
#define jvmmgr_h___

#include "nsIJVM.h"
#include "nsscd.h"
#include "nsAgg.h"

class nsSymantecDebugManager;

enum nsJVMStatus {
    nsJVMStatus_Enabled,  // but not Running
    nsJVMStatus_Disabled, // explicitly disabled
    nsJVMStatus_Running,  // enabled and started
    nsJVMStatus_Failed    // enabled but failed to start
};

////////////////////////////////////////////////////////////////////////////////
// JVMMgr is the interface to the JVM manager that the browser sees. All
// files that want to include java services should include this header file.
// nsIJVMManager is the more limited interface what the JVM plugin sees.

class nsJVMMgr : public nsIJVMManager {
public:

    NS_DECL_AGGREGATED
    
    ////////////////////////////////////////////////////////////////////////////
    // from nsIJVMManager:

    // This method may be called by the JVM to indicate a status
    // change, e.g. that the JVM has failed or is shutting down
    // spontaneously. This allows the browser to clean up any
    // JVM-specific state.
    NS_IMETHOD_(void)
    NotifyJVMStatusChange(nsJVMError error);

    // ====> These are usually only called by the plugin, not the browser...
#if 0
    NS_IMETHOD_(PRBool)
    HandOffJSLock(PRThread* oldOwner, PRThread* newOwner);

    NS_IMETHOD_(void)
    ReportJVMError(void* env, nsJVMError err);
#endif
    ////////////////////////////////////////////////////////////////////////////
    // JVMMgr specific methods:

    // ====> From here on are things only called by the browser, not the plugin...

    static NS_METHOD
    Create(nsISupports* outer, const nsIID& aIID, void* *aInstancePtr);

    nsIJVMPlugin* GetJVM(void);

    // Unlike the nsIJVMPlugin::StartupJVM, this version handles putting
    // up any error dialog:
    nsJVMStatus StartupJVM(void);
    nsJVMStatus ShutdownJVM(PRBool fullShutdown = PR_FALSE);
    nsJVMStatus GetJVMStatus(void);
    void SetJVMEnabled(PRBool enabled);

    nsPluginError AddToClassPathRecursively(const char* dirPath);

protected:    
    nsJVMMgr(nsISupports* outer);
    virtual ~nsJVMMgr(void);

    void        EnsurePrefCallbackRegistered(void);
    const char* GetJavaErrorString(JRIEnv* env);

    nsIJVMPlugin*       fJVM;
    nsJVMStatus         fStatus;
    PRBool              fRegisteredJavaPrefChanged;
    nsISupports*        fDebugManager;
 
};

////////////////////////////////////////////////////////////////////////////////
// Symantec Debugger Stuff

class nsSymantecDebugManager : public nsISymantecDebugManager {
public:

    NS_DECL_AGGREGATED

    NS_IMETHOD_(PRBool)
    SetDebugAgentPassword(PRInt32 pwd);

    static NS_METHOD
    Create(nsISupports* outer, const nsIID& aIID, void* *aInstancePtr,
           nsJVMMgr* jvmMgr);

protected:
    nsSymantecDebugManager(nsISupports* outer, nsJVMMgr* jvmMgr);
    virtual ~nsSymantecDebugManager(void);

    nsJVMMgr*             fJVMMgr;

};

////////////////////////////////////////////////////////////////////////////////
// nsJVMPluginTagInfo: The browser makes one of these when it sees an APPLET or
// appropriate OBJECT tag.

class nsJVMPluginTagInfo : public nsIJVMPluginTagInfo {
public:

    NS_DECL_AGGREGATED

    ////////////////////////////////////////////////////////////////////////////
    // from nsIJVMPluginTagInfo:

    // ====> These are usually only called by the plugin, not the browser...

    NS_IMETHOD_(const char *)
    GetCode(void);

    NS_IMETHOD_(const char *)
    GetCodeBase(void);

    NS_IMETHOD_(const char *)
    GetArchive(void);

    NS_IMETHOD_(const char *)
    GetName(void);

    NS_IMETHOD_(PRBool)
    GetMayScript(void);

    ////////////////////////////////////////////////////////////////////////////
    // Methods specific to nsJVMPluginInstancePeer:
    
    // ====> From here on are things only called by the browser, not the plugin...

    static NS_METHOD
    Create(nsISupports* outer, const nsIID& aIID, void* *aInstancePtr,
           nsIPluginTagInfo2* info);

protected:

    nsJVMPluginTagInfo(nsISupports* outer, nsIPluginTagInfo2* info);
    virtual ~nsJVMPluginTagInfo(void);

    // Instance Variables:
    nsIPluginTagInfo2*  fPluginTagInfo;
    char*               fSimulatedCodebase;
    char*               fSimulatedCode;
};

////////////////////////////////////////////////////////////////////////////////
// Convenience Routines

PR_BEGIN_EXTERN_C

// Returns the JVM manager. You must do a Release on the
// pointer returned when you're done with it.
PR_EXTERN(nsJVMMgr*)
JVM_GetJVMMgr(void);

// Returns true if java is enabled, and has not failed to load.
PR_EXTERN(PRBool)
JVM_IsJVMAvailable(void);

PR_EXTERN(nsPluginError)
JVM_AddToClassPathRecursively(const char* dirPath);

PR_EXTERN(void)
JVM_ShowConsole(void);

PR_EXTERN(void)
JVM_HideConsole(void);

PR_EXTERN(PRBool)
JVM_IsConsoleVisible(void);

PR_EXTERN(void)
JVM_ToggleConsole(void);

PR_EXTERN(void)
JVM_PrintToConsole(const char* msg);

PR_EXTERN(void)
JVM_StartDebugger(void);

PR_END_EXTERN_C

////////////////////////////////////////////////////////////////////////////////
#endif /* jvmmgr_h___ */
