/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or 
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef nsXPComPrivate_h__
#define nsXPComPrivate_h__

#include "nscore.h"
#include "nsXPCOM.h"
/**
 * Private Method to register an exit routine.  This method
 * allows you to setup a callback that will be called from 
 * the NS_ShutdownXPCOM function after all services and 
 * components have gone away.
 *
 * This API is for the exclusive use of the xpcom glue library.
 * 
 * Note that these APIs are NOT threadsafe and must be called on the
 * main thread.
 * 
 * @status FROZEN
 * @param exitRoutine pointer to user defined callback function
 *                    of type XPCOMExitRoutine. 
 * @param priority    higher priorities are called before lower  
 *                    priorities.
 *
 * @return NS_OK for success;
 *         other error codes indicate a failure.
 *
 */
typedef NS_CALLBACK(XPCOMExitRoutine)(void);

extern "C" NS_COM nsresult
NS_RegisterXPCOMExitRoutine(XPCOMExitRoutine exitRoutine, PRUint32 priority);

extern "C" NS_COM nsresult
NS_UnregisterXPCOMExitRoutine(XPCOMExitRoutine exitRoutine);


// PUBLIC
typedef nsresult (PR_CALLBACK *InitFunc)(nsIServiceManager* *result, nsIFile* binDirectory, nsIDirectoryServiceProvider* appFileLocationProvider);
typedef nsresult (PR_CALLBACK *ShutdownFunc)(nsIServiceManager* servMgr);
typedef nsresult (PR_CALLBACK *GetServiceManagerFunc)(nsIServiceManager* *result);
typedef nsresult (PR_CALLBACK *GetComponentManagerFunc)(nsIComponentManager* *result);
typedef nsresult (PR_CALLBACK *GetComponentRegistrarFunc)(nsIComponentRegistrar* *result);
typedef nsresult (PR_CALLBACK *GetMemoryManagerFunc)(nsIMemory* *result);
typedef nsresult (PR_CALLBACK *NewLocalFileFunc)(const nsAString &path, PRBool followLinks, nsILocalFile* *result);
typedef nsresult (PR_CALLBACK *NewNativeLocalFileFunc)(const nsACString &path, PRBool followLinks, nsILocalFile* *result);
// PRIVATE
typedef nsresult (PR_CALLBACK *RegisterXPCOMExitRoutineFunc)(XPCOMExitRoutine exitRoutine, PRUint32 priority);
typedef nsresult (PR_CALLBACK *UnregisterXPCOMExitRoutineFunc)(XPCOMExitRoutine exitRoutine);

typedef struct XPCOMFunctions{
    PRUint32 version;
    PRUint32 size;

    InitFunc init;
    ShutdownFunc shutdown;
    GetServiceManagerFunc getServiceManager;
    GetComponentManagerFunc getComponentManager;
    GetComponentRegistrarFunc getComponentRegistrar;
    GetMemoryManagerFunc getMemoryManager;
    NewLocalFileFunc newLocalFile;
    NewNativeLocalFileFunc newNativeLocalFile;

    RegisterXPCOMExitRoutineFunc registerExitRoutine;
    UnregisterXPCOMExitRoutineFunc unregisterExitRoutine;
} XPCOMFunctions;

typedef nsresult (PR_CALLBACK *GetFrozenFunctionsFunc)(XPCOMFunctions *entryPoints, const char* libraryPath);
extern "C" NS_COM nsresult
NS_GetFrozenFunctions(XPCOMFunctions *entryPoints, const char* libraryPath);

// think hard before changing this
#define XPCOM_GLUE_VERSION 1

#endif


