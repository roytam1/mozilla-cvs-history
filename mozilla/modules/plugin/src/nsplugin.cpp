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

////////////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF NETSCAPE COMMUNICATOR PLUGINS (NEW C++ API).
//
// This superscedes the old plugin API (npapi.h, npupp.h), and 
// eliminates the need for glue files: npunix.c, npwin.cpp and npmac.cpp. 
// Correspondences to the old API are shown throughout the file.
////////////////////////////////////////////////////////////////////////////////

#include "npglue.h" 
#include "np.h" 

#ifdef OJI
#include "nsplugin.h"
#include "jvmmgr.h" 
#endif
#include "plstr.h" /* PL_strcasecmp */

#include "xp_mem.h"
#include "xpassert.h" 

#ifdef XP_MAC
#include "MacMemAllocator.h"
#include "asyncCursors.h"
#include "LMenuSharing.h"
#include <TArray.h>
#endif

#include "nsHashtable.h"
#include "nsMalloc.h"

#include "intl_csi.h"

static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
static NS_DEFINE_IID(kIPluginManagerIID, NS_IPLUGINMANAGER_IID);
static NS_DEFINE_IID(kIPluginManager2IID, NS_IPLUGINMANAGER2_IID);
static NS_DEFINE_IID(kINetworkManagerIID, NS_INETWORKMANAGER_IID);
static NS_DEFINE_IID(kIJNIEnvIID, NS_IJNIENV_IID); 
static NS_DEFINE_IID(kILiveConnectPluginInstancePeerIID, NS_ILIVECONNECTPLUGININSTANCEPEER_IID);
static NS_DEFINE_IID(kIWindowlessPluginInstancePeerIID, NS_IWINDOWLESSPLUGININSTANCEPEER_IID);
static NS_DEFINE_IID(kIPluginInstanceIID, NS_IPLUGININSTANCE_IID);
static NS_DEFINE_IID(kPluginInstancePeerCID, NS_PLUGININSTANCEPEER_CID);
static NS_DEFINE_IID(kIPluginInstancePeerIID, NS_IPLUGININSTANCEPEER_IID); 
static NS_DEFINE_IID(kIPluginTagInfoIID, NS_IPLUGINTAGINFO_IID); 
static NS_DEFINE_IID(kIPluginTagInfo2IID, NS_IPLUGINTAGINFO2_IID); 
static NS_DEFINE_IID(kIOutputStreamIID, NS_IOUTPUTSTREAM_IID);
static NS_DEFINE_IID(kISeekablePluginStreamPeerIID, NS_ISEEKABLEPLUGINSTREAMPEER_IID);
static NS_DEFINE_IID(kIPluginStreamPeerIID, NS_IPLUGINSTREAMPEER_IID);
static NS_DEFINE_IID(kIPluginStreamPeer2IID, NS_IPLUGINSTREAMPEER2_IID);
static NS_DEFINE_IID(kIFileUtilitiesIID, NS_IFILEUTILITIES_IID);

#include "prerror.h"

// mapping from NPError to nsresult
nsresult fromNPError[] = {
    NS_OK,                          // NPERR_NO_ERROR,
    NS_ERROR_FAILURE,               // NPERR_GENERIC_ERROR,
    NS_ERROR_FAILURE,               // NPERR_INVALID_INSTANCE_ERROR,
    NS_ERROR_NOT_INITIALIZED,       // NPERR_INVALID_FUNCTABLE_ERROR,
    NS_ERROR_FACTORY_NOT_LOADED,    // NPERR_MODULE_LOAD_FAILED_ERROR,
    NS_ERROR_OUT_OF_MEMORY,         // NPERR_OUT_OF_MEMORY_ERROR,
    NS_NOINTERFACE,                 // NPERR_INVALID_PLUGIN_ERROR,
    NS_ERROR_ILLEGAL_VALUE,         // NPERR_INVALID_PLUGIN_DIR_ERROR,
    NS_NOINTERFACE,                 // NPERR_INCOMPATIBLE_VERSION_ERROR,
    NS_ERROR_ILLEGAL_VALUE,         // NPERR_INVALID_PARAM,
    NS_ERROR_ILLEGAL_VALUE,         // NPERR_INVALID_URL,
    NS_ERROR_ILLEGAL_VALUE,         // NPERR_FILE_NOT_FOUND,
    NS_ERROR_FAILURE,               // NPERR_NO_DATA,
    NS_ERROR_FAILURE                // NPERR_STREAM_NOT_SEEKABLE,
};

////////////////////////////////////////////////////////////////////////////////
// THINGS IMPLEMENTED BY THE BROWSER...
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Plugin Manager Interface
// This interface defines the minimum set of functionality that a plugin
// manager will support if it implements plugins. Plugin implementations can
// QueryInterface to determine if a plugin manager implements more specific 
// APIs for the plugin to use.

nsPluginManager* thePluginManager = NULL;

nsPluginManager::nsPluginManager(nsISupports* outer)
    : fJVMMgr(NULL), fMalloc(NULL), fAllocatedMenuIDs(NULL)
{
    NS_INIT_AGGREGATED(outer);
}

nsPluginManager::~nsPluginManager(void)
{
    fJVMMgr->Release();
    fJVMMgr = NULL;
    fMalloc->Release();
    fMalloc = NULL;

#ifdef XP_MAC
    if (fAllocatedMenuIDs != NULL) {
        // Fix me, delete all the elements before deleting the table.
        delete fAllocatedMenuIDs;
    }
#endif
}

NS_IMPL_AGGREGATED(nsPluginManager);

NS_METHOD
nsPluginManager::Create(nsISupports* outer, const nsIID& aIID, void* *aInstancePtr)
{
    if (outer && !aIID.Equals(kISupportsIID))
        return NS_NOINTERFACE;   // XXX right error?
    nsPluginManager* mgr = new nsPluginManager(outer);
    nsresult result = mgr->QueryInterface(aIID, aInstancePtr);
    if (result != NS_OK) {
        delete mgr;
    }
    return result;
}

NS_METHOD
nsPluginManager::GetValue(nsPluginManagerVariable variable, void *value)
{
    NPError err = npn_getvalue(NULL, (NPNVariable)variable, value);
    return fromNPError[err];
}

NS_METHOD
nsPluginManager::SetValue(nsPluginManagerVariable variable, void *value)
{
    NPError err = npn_setvalue(NULL, (NPPVariable)variable, value);
    return fromNPError[err];
}

NS_METHOD
nsPluginManager::ReloadPlugins(PRBool reloadPages)
{
    npn_reloadplugins(reloadPages);
    return NS_OK;
}

NS_METHOD
nsPluginManager::UserAgent(const char* *resultingAgentString)
{
    *resultingAgentString = npn_useragent(NULL); // we don't really need an npp here
    return NS_OK;
}

NS_METHOD
nsPluginManager::AggregatedQueryInterface(const nsIID& aIID, void** aInstancePtr) 
{
    if (NULL == aInstancePtr) {                                            
        return NS_ERROR_NULL_POINTER;                                        
    }                                                                      
    if (aIID.Equals(kIPluginManagerIID) || 
        aIID.Equals(kIPluginManager2IID) ||
        aIID.Equals(kISupportsIID)) {
        *aInstancePtr = (nsIPluginManager*) this; 
        AddRef(); 
        return NS_OK; 
    } 
    if (aIID.Equals(kINetworkManagerIID)) {
        *aInstancePtr = (nsINetworkManager*) this; 
        AddRef(); 
        return NS_OK; 
    } 
#if 0   // Browser interface should be JNI from now on. Hence all new code
        // should be written using JNI. sudu.
    if (aIID.Equals(kIJRIEnvIID)) {
        // XXX Need to implement ISupports for JRIEnv
        *aInstancePtr = (void*) ((nsISupports*)npn_getJavaEnv()); 
//        AddRef();     // XXX should the plugin instance peer and the env be linked?
        return NS_OK; 
    } 
#endif
#ifdef OJI
    if (aIID.Equals(kIJNIEnvIID)) {
        // XXX Need to implement ISupports for JNIEnv
        *aInstancePtr = (void*) ((nsISupports*)npn_getJavaEnv());       //=-= Fix this to return a Interface XXX need JNI version
//        AddRef();     // XXX should the plugin instance peer and the env be linked?
        return NS_OK; 
    }
#endif
    // Aggregates...
    nsIJVMManager* jvmMgr = GetJVMMgr(aIID);
    if (jvmMgr) {
        *aInstancePtr = (void*) ((nsISupports*)jvmMgr);
        return NS_OK; 
    }
    if (fMalloc == NULL) {
        if (nsMalloc::Create((nsIPluginManager*)this, kISupportsIID,
                             (void**)&fMalloc) != NS_OK)
            return NS_NOINTERFACE;
    }
    return fMalloc->QueryInterface(aIID, aInstancePtr);
}

nsIJVMManager*
nsPluginManager::GetJVMMgr(const nsIID& aIID)
{
    nsIJVMManager* result = NULL;
#ifdef OJI
    if (fJVMMgr == NULL) {
        // The plugin manager is the outer of the JVM manager
        if (nsJVMMgr::Create((nsIPluginManager*)this, kISupportsIID,
                             (void**)&fJVMMgr) != NS_OK)
            return NULL;
    }
    if (fJVMMgr->QueryInterface(aIID, (void**)&result) != NS_OK)
        return NULL;
#endif
    return result;
}

NS_METHOD
nsPluginManager::BeginWaitCursor(void)
{
    if (fWaiting == 0) {
#ifdef XP_PC
        fOldCursor = (void*)GetCursor();
        HCURSOR newCursor = LoadCursor(NULL, IDC_WAIT);
        if (newCursor)
            SetCursor(newCursor);
#endif
#ifdef XP_MAC
        startAsyncCursors();
#endif
    }
    fWaiting++;
    return NS_OK;
}

NS_METHOD
nsPluginManager::EndWaitCursor(void)
{
    fWaiting--;
    if (fWaiting == 0) {
#ifdef XP_PC
        if (fOldCursor)
            SetCursor((HCURSOR)fOldCursor);
#endif
#ifdef XP_MAC
        stopAsyncCursors();
#endif
        fOldCursor = NULL;
    }
    return NS_OK;
}

NS_METHOD
nsPluginManager::SupportsURLProtocol(const char* protocol, PRBool *result)
{
    int type = NET_URL_Type(protocol);
    *result = (PRBool)(type != 0);
    return NS_OK;
}

NS_METHOD
nsPluginManager::NotifyStatusChange(nsIPlugin* plugin, nsresult errorStatus)
{
    // XXX need to shut down all instances of this plugin
    return NS_OK;
}

#if 0
static NPP getNPPFromHandler(nsIEventHandler* handler)
{
	NPP npp = NULL;
    nsIPluginInstance* pluginInst = NULL;
    if (handler->QueryInterface(kIPluginInstanceIID, (void**)&pluginInst) == NS_OK) {
        nsPluginInstancePeer* myPeer;
        nsresult err = pluginInst->GetPeer((nsIPluginInstancePeer**)&myPeer);
        PR_ASSERT(err == NS_OK);
        npp = myPeer->GetNPP();
        myPeer->Release();
        pluginInst->Release();
    }
    return npp;
}
#endif

NS_METHOD
nsPluginManager::RegisterWindow(nsIEventHandler* handler, nsPluginPlatformWindowRef window)
{
#if 1
	npn_registerwindow(handler, window);
	return NS_OK;
#else
	NPP npp = getNPPFromHandler(handler);
	if (npp != NULL) {
        npn_registerwindow(npp, window);
        return NS_OK;
    }
    return NS_ERROR_FAILURE;
#endif
}

NS_METHOD
nsPluginManager::UnregisterWindow(nsIEventHandler* handler, nsPluginPlatformWindowRef window)
{
#if 1
	npn_unregisterwindow(handler, window);
	return NS_OK;
#else
	NPP npp = getNPPFromHandler(handler);
	if (npp != NULL) {
        npn_unregisterwindow(npp, window);
        return NS_OK;
    }
    return NS_ERROR_FAILURE;
#endif
}

class nsEventHandlerKey : public nsHashKey {
public:
	nsEventHandlerKey(nsIEventHandler* handler) : mHandler(handler) {}

	virtual PRUint32 HashValue(void) const
	{
		return PRUint32(mHandler);
	}
	
	virtual PRBool Equals(const nsHashKey *aKey) const
	{
		return ((nsEventHandlerKey*)aKey)->mHandler == mHandler;
	}
	
	virtual nsHashKey *Clone(void) const
	{
		return new nsEventHandlerKey(mHandler);
	}

private:
	nsIEventHandler* mHandler;
};

NS_METHOD
nsPluginManager::AllocateMenuID(nsIEventHandler* handler, PRBool isSubmenu,
                                PRInt16 *result)
{
#ifdef XP_MAC
	PRInt16 menuID = LMenuSharingAttachment::AllocatePluginMenuID(isSubmenu);
	if (fAllocatedMenuIDs == NULL)
		fAllocatedMenuIDs  = new nsHashtable(16);
	if (fAllocatedMenuIDs != NULL) {
		nsEventHandlerKey key(handler);
		TArray<PRInt16>* menuIDs = (TArray<PRInt16>*) fAllocatedMenuIDs->Get(&key);
		if (menuIDs == NULL) {
			menuIDs = new TArray<PRInt16>;
			fAllocatedMenuIDs->Put(&key, menuIDs);
		}
		if (menuIDs != NULL)
			menuIDs->AddItem(menuID);
	}
	*result = menuID;
    return NS_OK;
#else
    return NS_ERROR_NOT_IMPLEMENTED;
#endif
}

NS_METHOD
nsPluginManager::DeallocateMenuID(nsIEventHandler* handler, PRInt16 menuID)
{
#ifdef XP_MAC
	if (fAllocatedMenuIDs != NULL) {
		nsEventHandlerKey key(handler);
		TArray<PRInt16>* menuIDs = (TArray<PRInt16>*) fAllocatedMenuIDs->Get(&key);
		if (menuIDs != NULL) {
			menuIDs->Remove(menuID);
			if (menuIDs->GetCount() == 0) {
				// let go of the vector and the hash table entry.
				fAllocatedMenuIDs->Remove(&key);
				delete menuIDs;
			}
			return NS_OK;
		}
	}
    return NS_ERROR_FAILURE;
#else
    return NS_ERROR_NOT_IMPLEMENTED;
#endif
}

NS_METHOD
nsPluginManager::HasAllocatedMenuID(nsIEventHandler* handler, PRInt16 menuID, PRBool *result)
{
#ifdef XP_MAC
	if (fAllocatedMenuIDs != NULL) {
		nsEventHandlerKey key(handler);
		TArray<PRInt16>* menuIDs = (TArray<PRInt16>*) fAllocatedMenuIDs->Get(&key);
		if (menuIDs != NULL) {
			TArray<PRInt16>& menus = *menuIDs;
			UInt32 count = menus.GetCount();
			for (UInt32 i = count; i > 0; --i) {
				if (menus[i] == menuID) {
					*result = PR_TRUE;
                    return NS_OK;
                }
            }
		}
	}
	return PR_FALSE;
#else
    return NS_ERROR_NOT_IMPLEMENTED;
#endif
}

static NPL_ProcessNextEventProc npl_ProcessNextEventProc = NULL;
static void* npl_ProcessNextEventData = NULL;

PR_IMPLEMENT(void) 
NPL_InstallProcessNextEventProc(NPL_ProcessNextEventProc proc, void* data)
{
    npl_ProcessNextEventProc = proc;
    npl_ProcessNextEventData = data;
}

NS_METHOD
nsPluginManager::ProcessNextEvent(PRBool *bEventHandled)
{
#ifdef XP_MAC
	if (npl_ProcessNextEventProc != NULL)
  		*bEventHandled = npl_ProcessNextEventProc(npl_ProcessNextEventData);
	return NS_OK;
#else 
    *bEventHandled = PR_FALSE;
    return NS_OK;
#endif
}

////////////////////////////////////////////////////////////////////////////////
// from nsINetworkManager:

#include "plevent.h"

extern "C" {
extern PREventQueue* mozilla_event_queue;
extern PRThread* mozilla_thread;
};

struct GetURLEvent {
    PLEvent event;
    nsPluginInstancePeer* peer;
    const char* url;
    const char* target;
    void* notifyData;
    const char* altHost;
    const char* referrer;
    PRBool forceJSEnabled;
};

static void*
HandleGetURLEvent(PLEvent* event)
{
    PR_ASSERT(PR_CurrentThread() == mozilla_thread);
    GetURLEvent* e = (GetURLEvent*)event;
    NPP npp = e->peer->GetNPP();
    NPError rslt = np_geturlinternal(npp,
                                     e->url,
                                     e->target,
                                     e->altHost,
                                     e->referrer,
                                     e->forceJSEnabled,
                                     e->notifyData != NULL,
                                     e->notifyData);
    return (void*)rslt;
}

static void
DestroyGetURLEvent(PLEvent* event)
{
    GetURLEvent* e = (GetURLEvent*)event;
    PR_Free(event);
}

NS_METHOD
nsPluginManager::GetURL(nsISupports* peer, const char* url, const char* target,
                        void* notifyData, const char* altHost,
                        const char* referrer, PRBool forceJSEnabled)
{
    NPError rslt = NPERR_INVALID_PARAM;
    nsPluginInstancePeer* instPeer = NULL;
    if (peer->QueryInterface(kPluginInstancePeerCID, (void**)&instPeer) == NS_OK) {
        if (PR_CurrentThread() == mozilla_thread) {
            NPP npp = instPeer->GetNPP();
            rslt = np_geturlinternal(npp,
                                     url,
                                     target,
                                     altHost,
                                     referrer,
                                     forceJSEnabled,
                                     notifyData != NULL,
                                     notifyData);
        }
        else {
            GetURLEvent* e = PR_NEW(GetURLEvent);
            if (e == NULL) {
                rslt = NPERR_OUT_OF_MEMORY_ERROR;
            }
            else {
                PL_InitEvent(&e->event, NULL, HandleGetURLEvent, DestroyGetURLEvent);
                e->peer = instPeer;
                e->url = url;
                e->target = target;
                e->notifyData = notifyData;
                e->altHost = altHost;
                e->referrer = referrer;
                e->forceJSEnabled = forceJSEnabled;
                rslt = (NPError)PL_PostSynchronousEvent(mozilla_event_queue, &e->event);
            }
        }
        instPeer->Release();
    }
    return fromNPError[rslt];
}

struct PostURLEvent {
    PLEvent event;
    nsPluginInstancePeer* peer;
    const char* url;
    const char* target;
    PRUint32 postDataLen;
    const char* postData;
    PRBool isFile;
    void* notifyData;
    const char* altHost;
    const char* referrer;
    PRBool forceJSEnabled;
    PRUint32 postHeadersLen;
    const char* postHeaders;
};

static void*
HandlePostURLEvent(PLEvent* event)
{
    PR_ASSERT(PR_CurrentThread() == mozilla_thread);
    PostURLEvent* e = (PostURLEvent*)event;
    NPP npp = e->peer->GetNPP();
    NPError rslt = np_posturlinternal(npp,
                                      e->url, 
                                      e->target,
                                      e->altHost,
                                      e->referrer,
                                      e->forceJSEnabled,
                                      e->postDataLen,
                                      e->postData,
                                      e->isFile,
                                      e->notifyData != NULL,
                                      e->notifyData);
    return (void*)rslt;
}

static void
DestroyPostURLEvent(PLEvent* event)
{
    PostURLEvent* e = (PostURLEvent*)event;
    PR_Free(event);
}

NS_METHOD
nsPluginManager::PostURL(nsISupports* peer, const char* url, const char* target,
                         PRUint32 postDataLen, const char* postData,
                         PRBool isFile, void* notifyData,
                         const char* altHost, const char* referrer,
                         PRBool forceJSEnabled,
                         PRUint32 postHeadersLen, const char* postHeaders)
{
    NPError rslt = NPERR_INVALID_PARAM;
    nsPluginInstancePeer* instPeer = NULL;
    if (peer->QueryInterface(kPluginInstancePeerCID, (void**)&instPeer) == NS_OK) {
        if (PR_CurrentThread() == mozilla_thread) {
            NPP npp = instPeer->GetNPP();
            PR_ASSERT(postHeaders == NULL); // XXX need to deal with postHeaders
            rslt = np_posturlinternal(npp,
                                      url, 
                                      target,
                                      altHost,
                                      referrer,
                                      forceJSEnabled,
                                      postDataLen,
                                      postData,
                                      isFile,
                                      notifyData != NULL,
                                      notifyData);
        }
        else {
            PostURLEvent* e = PR_NEW(PostURLEvent);
            if (e == NULL) {
                rslt = NPERR_OUT_OF_MEMORY_ERROR;
            }
            else {
                PL_InitEvent(&e->event, NULL, HandlePostURLEvent, DestroyPostURLEvent);
                e->peer = instPeer;
                e->url = url;
                e->target = target;
                e->notifyData = notifyData;
                e->altHost = altHost;
                e->referrer = referrer;
                e->forceJSEnabled = forceJSEnabled;
                e->postDataLen = postDataLen;
                e->postData = postData;
                e->isFile = isFile;
                e->postHeadersLen = postHeadersLen;
                e->postHeaders = postHeaders;
                rslt = (NPError)PL_PostSynchronousEvent(mozilla_event_queue, &e->event);
            }
        }
        instPeer->Release();
    }
    return fromNPError[rslt];
}

////////////////////////////////////////////////////////////////////////////////
// File Utilities Interface

nsFileUtilities::nsFileUtilities(nsISupports* outer)
    : fProgramPath(NULL)
{
    NS_INIT_AGGREGATED(outer);
}

nsFileUtilities::~nsFileUtilities(void)
{
}

NS_IMPL_AGGREGATED(nsFileUtilities);

NS_METHOD
nsFileUtilities::AggregatedQueryInterface(const nsIID& aIID, void** aInstancePtr) 
{
    if (NULL == aInstancePtr) {
        return NS_ERROR_NULL_POINTER;
    }
    if (aIID.Equals(kIFileUtilitiesIID) || 
        aIID.Equals(kISupportsIID)) {
        *aInstancePtr = (void*) this; 
        AddRef(); 
        return NS_OK; 
    } 
    return NS_NOINTERFACE;
}


NS_METHOD
nsFileUtilities::GetProgramPath(const char* *result)
{
    *result = fProgramPath;
    return NS_OK;
}

NS_METHOD
nsFileUtilities::GetTempDirPath(const char* *result)
{
    // XXX I don't need a static really, the browser holds the tempDir name
    // as a static string -- it's just the XP_TempDirName that strdups it.
    static const char* tempDirName = NULL;
    if (tempDirName == NULL)
        tempDirName = XP_TempDirName();
    *result = tempDirName;
    return NS_OK;
}

#if 0
NS_METHOD
nsFileUtilities::GetFileName(const char* fn, FileNameType type,
                             char* resultBuf, PRUint32 bufLen)
{
    // XXX This should be rewritten so that we don't have to malloc the name.
    XP_FileType filetype;

    if (type == SIGNED_APPLET_DBNAME)
        filetype = xpSignedAppletDB;
    else if (type == TEMP_FILENAME)
        filetype = xpTemporary;
    else 
        return NS_ERROR_ILLEGAL_VALUE;

    char* tempName = WH_FileName(fn, filetype);
    if (tempName == NULL)
        return NS_ERROR_OUT_OF_MEMORY;
    XP_STRNCPY_SAFE(resultBuf, tempName, bufLen);
    XP_FREE(tempName);
    return NS_OK;
}
#endif

NS_METHOD
nsFileUtilities::NewTempFileName(const char* prefix, PRUint32 bufLen, char* resultBuf)
{
    // XXX This should be rewritten so that we don't have to malloc the name.
    char* tempName = WH_TempName(xpTemporary, prefix);
    if (tempName == NULL)
        return NS_ERROR_OUT_OF_MEMORY;
    XP_STRNCPY_SAFE(resultBuf, tempName, bufLen);
    XP_FREE(tempName);
    return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////
// Plugin Instance Peer Interface

nsPluginInstancePeer::nsPluginInstancePeer(NPP npp)
    : fNPP(npp), fPluginInst(NULL), fTagInfo(NULL)
{
//    NS_INIT_AGGREGATED(NULL);
    NS_INIT_REFCNT();
    fTagInfo = new nsPluginTagInfo(npp);
    fTagInfo->AddRef();
}

nsPluginInstancePeer::~nsPluginInstancePeer(void)
{
    if (fTagInfo != NULL) {
        fTagInfo->Release();
        fTagInfo = NULL;
    }
}

NS_IMPL_ADDREF(nsPluginInstancePeer);
NS_IMPL_RELEASE(nsPluginInstancePeer);

NS_METHOD
nsPluginInstancePeer::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{
    if (NULL == aInstancePtr) {                                            
        return NS_ERROR_NULL_POINTER;                                        
    }                                                                      
    if (aIID.Equals(kIPluginInstancePeerIID) ||
        aIID.Equals(kPluginInstancePeerCID) ||
        aIID.Equals(kISupportsIID)) {
        // *aInstancePtr = (void*) (nsISupports*) (nsIPluginInstancePeer*)this; 
        *aInstancePtr = (nsIPluginInstancePeer*) this;
        AddRef();
        return NS_OK;
    }
    // beard:  check for interfaces that aren't on the left edge of the inheritance graph.
    // this is required so that the proper offsets are applied to this, and so the proper
    // vtable is used.
    if (aIID.Equals(kILiveConnectPluginInstancePeerIID)) {
        *aInstancePtr = (nsILiveConnectPluginInstancePeer*) this;
        AddRef();
        return NS_OK;
    }
    if (aIID.Equals(kIWindowlessPluginInstancePeerIID)) {
        *aInstancePtr = (nsIWindowlessPluginInstancePeer*) this;
        AddRef();
        return NS_OK;
    }
    return fTagInfo->QueryInterface(aIID, aInstancePtr);
}

NS_METHOD
nsPluginInstancePeer::GetValue(nsPluginInstancePeerVariable variable, void *value)
{
    NPError err = npn_getvalue(fNPP, (NPNVariable)variable, value);
    return fromNPError[err];
}

NS_METHOD
nsPluginInstancePeer::SetValue(nsPluginInstancePeerVariable variable, void *value)
{
    NPError err = npn_setvalue(fNPP, (NPPVariable)variable, value);
    return fromNPError[err];
}

NS_METHOD
nsPluginInstancePeer::GetMIMEType(nsMIMEType *result)
{
    np_instance* instance = (np_instance*)fNPP->ndata;
    *result = instance->typeString;
    return NS_OK;
}

NS_METHOD
nsPluginInstancePeer::GetMode(nsPluginMode *result)
{
    np_instance* instance = (np_instance*)fNPP->ndata;
    *result = (nsPluginMode)instance->type;
    return NS_OK;
}

NS_METHOD
nsPluginInstancePeer::NewStream(nsMIMEType type, const char* target, nsIOutputStream* *result)
{
    NPStream* pstream;
    NPError err = npn_newstream(fNPP, (char*)type, (char*)target, &pstream);
    if (err != NPERR_NO_ERROR)
        return err;
    *result = new nsPluginManagerStream(fNPP, pstream);
    return NS_OK;
}

NS_METHOD
nsPluginInstancePeer::ShowStatus(const char* message)
{
    npn_status(fNPP, message);
    return NS_OK;
}

NS_METHOD
nsPluginInstancePeer::InvalidateRect(nsPluginRect *invalidRect)
{
    npn_invalidaterect(fNPP, (NPRect*)invalidRect);
    return NS_OK;
}

NS_METHOD
nsPluginInstancePeer::InvalidateRegion(nsPluginRegion invalidRegion)
{
    npn_invalidateregion(fNPP, invalidRegion);
    return NS_OK;
}

NS_METHOD
nsPluginInstancePeer::ForceRedraw(void)
{
    npn_forceredraw(fNPP);
    return NS_OK;
}

NS_METHOD
nsPluginInstancePeer::GetJavaPeer(jref *peer)
{
    *peer = npn_getJavaPeer(fNPP);
    return NS_OK;
}

void nsPluginInstancePeer::SetPluginInstance(nsIPluginInstance* inst)
{
    if (fPluginInst != NULL) {
        fPluginInst->Release();
    }
    fPluginInst = inst;
    if (fPluginInst != NULL) {
	    fPluginInst->AddRef();
	 }
}

nsIPluginInstance* nsPluginInstancePeer::GetPluginInstance()
{
	if (fPluginInst != NULL) {
		fPluginInst->AddRef();
		return fPluginInst;
	}
	return NULL;
}

NPP
nsPluginInstancePeer::GetNPP()
{
    return fNPP;
}

extern "C" JSContext *lm_crippled_context; /* XXX kill me */

JSContext *
nsPluginInstancePeer::GetJSContext(void)
{
    MWContext *pMWCX = GetMWContext();
    JSContext *pJSCX = NULL;
    if (!pMWCX || (pMWCX->type != MWContextBrowser && pMWCX->type != MWContextPane))
    {
       return 0;
    }
    if( pMWCX->mocha_context != NULL)
    {
      pJSCX = pMWCX->mocha_context;
    }
    else
    {
       pJSCX = lm_crippled_context;
    }
    return pJSCX;
}

MWContext *
nsPluginInstancePeer::GetMWContext(void)
{
    np_instance* instance = (np_instance*) fNPP->ndata;
    MWContext *pMWCX = instance->cx;

    return pMWCX;
}


////////////////////////////////////////////////////////////////////////////////
// Plugin Tag Info Interface

nsPluginTagInfo::nsPluginTagInfo(NPP npp)
    : fJVMPluginTagInfo(NULL), npp(npp), fUniqueID(0)
{
    NS_INIT_AGGREGATED(NULL);
}

nsPluginTagInfo::~nsPluginTagInfo(void)
{
}

NS_IMPL_AGGREGATED(nsPluginTagInfo);

NS_METHOD
nsPluginTagInfo::AggregatedQueryInterface(const nsIID& aIID, void** aInstancePtr) 
{
    if (NULL == aInstancePtr) {                                            
        return NS_ERROR_NULL_POINTER;                                        
    } 
    if (aIID.Equals(kIPluginTagInfo2IID) ||
        aIID.Equals(kIPluginTagInfoIID) ||
        aIID.Equals(kISupportsIID)) {
        *aInstancePtr = (void*)(nsISupports*)this; 
        AddRef(); 
        return NS_OK; 
    } 
#ifdef OJI
    // Aggregates...
    if (fJVMPluginTagInfo == NULL) {
        nsresult result =
            nsJVMPluginTagInfo::Create((nsISupports*)this, kISupportsIID,
                                       (void**)&fJVMPluginTagInfo, this);
        if (result != NS_OK) return result;
    }
#endif
    return fJVMPluginTagInfo->QueryInterface(aIID, aInstancePtr);
}

static char* empty_list[] = { "", NULL };

NS_METHOD
nsPluginTagInfo::GetAttributes(PRUint16& n, 
                               const char*const*& names, 
                               const char*const*& values)
{
    np_instance* instance = (np_instance*)npp->ndata;

#if 0
    // defense
    PR_ASSERT( 0 != names );
    PR_ASSERT( 0 != values );
    if( 0 == names || 0 == values )
        return 0;
#endif

    if (instance->type == NP_EMBED) {
        np_data* ndata = (np_data*)instance->app->np_data;

#ifdef OJI
        names = (const char*const*)ndata->lo_struct->attributes.names;
        values = (const char*const*)ndata->lo_struct->attributes.values;
        n = (PRUint16)ndata->lo_struct->attributes.n;
#else
        // XXX I think all the parameters and attributes just get
        // munged together under MOZ_JAVA...
        names = (const char*const*) ndata->lo_struct->attribute_list;
        values = (const char*const*) ndata->lo_struct->value_list;
        n = (PRUint16) ndata->lo_struct->attribute_cnt;
#endif

        return NS_OK;
    } else {
        static char _name[] = "PALETTE";
        static char* _names[1];

        static char _value[] = "foreground";
        static char* _values[1];

        _names[0] = _name;
        _values[0] = _value;

        names = (const char*const*) _names;
        values = (const char*const*) _values;
        n = 1;

        return NS_OK;
    }

    // random, sun-spot induced error
    PR_ASSERT( 0 );

    n = 0;
    // const char* const* empty_list = { { '\0' } };
    names = values = (const char*const*)empty_list;

    return NS_ERROR_FAILURE;
}

NS_METHOD
nsPluginTagInfo::GetAttribute(const char* name, const char* *result) 
{
    PRUint16 nAttrs, i;
    const char*const* names;
    const char*const* values;

    nsresult rslt = GetAttributes(nAttrs, names, values);
    if (rslt != NS_OK)
        return rslt;
    
    *result = NULL;
    for( i = 0; i < nAttrs; i++ ) {
        if (PL_strcasecmp(name, names[i]) == 0) {
            *result = values[i];
            return NS_OK;
        }
    }

    return NS_OK;
}

NS_METHOD
nsPluginTagInfo::GetTagType(nsPluginTagType *result)
{
    *result = nsPluginTagType_Unknown;
    switch (GetLayoutElement()->type) {
      case LO_JAVA:
        *result = nsPluginTagType_Applet;
        return NS_OK;
      case LO_EMBED:
        *result = nsPluginTagType_Embed;
        return NS_OK;
      case LO_OBJECT:
        *result = nsPluginTagType_Object;
        return NS_OK;
      default:
        return NS_OK;
    }
}

NS_METHOD
nsPluginTagInfo::GetTagText(const char* *result)
{
    return NS_ERROR_NOT_IMPLEMENTED;    // XXX
}

NS_METHOD
nsPluginTagInfo::GetParameters(PRUint16& n, 
                               const char*const*& names, 
                               const char*const*& values)
{
    np_instance* instance = (np_instance*)npp->ndata;

    if (instance->type == NP_EMBED) {
        np_data* ndata = (np_data*)instance->app->np_data;

#ifdef OJI
        names = (const char*const*)ndata->lo_struct->parameters.names;
        values = (const char*const*)ndata->lo_struct->parameters.values;
        n = (PRUint16)ndata->lo_struct->parameters.n;
#else
        // XXX I think all the parameters and attributes just get
        // munged together under MOZ_JAVA...
        names = (const char*const*) ndata->lo_struct->attribute_list;
        values = (const char*const*) ndata->lo_struct->value_list;
        n = (PRUint16)ndata->lo_struct->attribute_cnt;
#endif

        return NS_OK;
    } else {
        static char _name[] = "PALETTE";
        static char* _names[1];

        static char _value[] = "foreground";
        static char* _values[1];

        _names[0] = _name;
        _values[0] = _value;

        names = (const char*const*) _names;
        values = (const char*const*) _values;
        n = 1;

        return NS_OK;
    }

    // random, sun-spot induced error
    PR_ASSERT( 0 );

    n = 0;
    // static const char* const* empty_list = { { '\0' } };
    names = values = (const char*const*)empty_list;

    return NS_ERROR_FAILURE;
}

NS_METHOD
nsPluginTagInfo::GetParameter(const char* name, const char* *result) 
{
    PRUint16 nParams, i;
    const char*const* names;
    const char*const* values;

    nsresult rslt = GetParameters(nParams, names, values);
    if (rslt != NS_OK)
        return rslt;

    *result = NULL;
    for( i = 0; i < nParams; i++ ) {
        if (PL_strcasecmp(name, names[i]) == 0) {
            *result = values[i];
            return NS_OK;
        }
    }

    return NS_OK;
}

NS_METHOD
nsPluginTagInfo::GetDocumentBase(const char* *result)
{
    *result = (const char*)GetLayoutElement()->base_url;
    return NS_OK;
}

NS_METHOD
nsPluginTagInfo::GetDocumentEncoding(const char* *result)
{
    np_instance* instance = (np_instance*) npp->ndata;
    MWContext* cx = instance->cx;
    INTL_CharSetInfo info = LO_GetDocumentCharacterSetInfo(cx);
    int16 doc_csid = INTL_GetCSIWinCSID(info);
    *result = INTL_CharSetIDToJavaCharSetName(doc_csid);
    return NS_OK;
}

NS_METHOD
nsPluginTagInfo::GetAlignment(const char* *result)
{
    int alignment = GetLayoutElement()->alignment;

    const char* cp;
    switch (alignment) {
      case LO_ALIGN_CENTER:      cp = "abscenter"; break;
      case LO_ALIGN_LEFT:        cp = "left"; break;
      case LO_ALIGN_RIGHT:       cp = "right"; break;
      case LO_ALIGN_TOP:         cp = "texttop"; break;
      case LO_ALIGN_BOTTOM:      cp = "absbottom"; break;
      case LO_ALIGN_NCSA_CENTER: cp = "center"; break;
      case LO_ALIGN_NCSA_BOTTOM: cp = "bottom"; break;
      case LO_ALIGN_NCSA_TOP:    cp = "top"; break;
      default:                   cp = "baseline"; break;
    }
    *result = cp;
    return NS_OK;
}

NS_METHOD
nsPluginTagInfo::GetWidth(PRUint32 *result)
{
    LO_CommonPluginStruct* lo = GetLayoutElement();
    *result = lo->width ? lo->width : 50;
    return NS_OK;
}
    
NS_METHOD
nsPluginTagInfo::GetHeight(PRUint32 *result)
{
    LO_CommonPluginStruct* lo = GetLayoutElement();
    *result = lo->height ? lo->height : 50;
    return NS_OK;
}

NS_METHOD
nsPluginTagInfo::GetBorderVertSpace(PRUint32 *result)
{
    *result = GetLayoutElement()->border_vert_space;
    return NS_OK;
}
    
NS_METHOD
nsPluginTagInfo::GetBorderHorizSpace(PRUint32 *result)
{
    *result = GetLayoutElement()->border_horiz_space;
    return NS_OK;
}

NS_METHOD
nsPluginTagInfo::GetUniqueID(PRUint32 *result)
{
    if (fUniqueID == 0) {
        np_instance* instance = (np_instance*) npp->ndata;
        MWContext* cx = instance->cx;
        History_entry* history_element = SHIST_GetCurrent(&cx->hist);
        if (history_element) {
            fUniqueID = history_element->unique_id;
        } else {
            /*
            ** XXX What to do? This can happen for instance when printing a
            ** mail message that contains an applet.
            */
            static int32 unique_number;
            fUniqueID = --unique_number;
        }
        PR_ASSERT(fUniqueID != 0);
    }
    *result = fUniqueID;
    return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////
// Plugin Manager Stream Interface

nsPluginManagerStream::nsPluginManagerStream(NPP npp, NPStream* pstr)
    : npp(npp), pstream(pstr)
{
    NS_INIT_REFCNT();
    // XXX get rid of the npp instance variable if this is true
    PR_ASSERT(npp == ((np_stream*)pstr->ndata)->instance->npp);
}

nsPluginManagerStream::~nsPluginManagerStream(void)
{
}

NS_METHOD
nsPluginManagerStream::Close(void)
{
    NPError err = npn_destroystream(npp, pstream, nsPluginReason_Done);
    return (nsresult)err;
}

NS_METHOD
nsPluginManagerStream::Write(const char* aBuf, PRInt32 aOffset, PRInt32 aCount,
                             PRInt32 *resultingCount)
{
    PR_ASSERT(aOffset == 0);           // XXX need to handle the non-sequential write case
    PRInt32 rslt = npn_write(npp, pstream, aCount, (void*)aBuf);
    if (rslt == -1) 
        return NS_ERROR_FAILURE;       // XXX what should this be?
    *resultingCount = rslt;
    return NS_OK;
}

NS_IMPL_QUERY_INTERFACE(nsPluginManagerStream, kIOutputStreamIID);
NS_IMPL_ADDREF(nsPluginManagerStream);
NS_IMPL_RELEASE(nsPluginManagerStream);

////////////////////////////////////////////////////////////////////////////////
// Plugin Stream Peer Interface

nsPluginStreamPeer::nsPluginStreamPeer(URL_Struct *urls, np_stream *stream)
    : userStream(NULL), urls(urls), stream(stream), 
      reason(nsPluginReason_NoReason)
{
    NS_INIT_REFCNT();
}

nsPluginStreamPeer::~nsPluginStreamPeer(void)
{
#if 0
    NPError err = npn_destroystream(stream->instance->npp, stream->pstream, reason);
    PR_ASSERT(err == nsPluginError_NoError);
#endif
}

NS_METHOD
nsPluginStreamPeer::GetURL(const char* *result)
{
    *result = stream->pstream->url;
    return NS_OK;
}

NS_METHOD
nsPluginStreamPeer::GetEnd(PRUint32 *result)
{
    *result = stream->pstream->end;
    return NS_OK;
}

NS_METHOD
nsPluginStreamPeer::GetLastModified(PRUint32 *result)
{
    *result = stream->pstream->lastmodified;
    return NS_OK;
}

NS_METHOD
nsPluginStreamPeer::GetNotifyData(void* *result)
{
    *result = stream->pstream->notifyData;
    return NS_OK;
}

NS_METHOD
nsPluginStreamPeer::GetReason(nsPluginReason *result)
{
    *result = reason;
    return NS_OK;
}

NS_METHOD
nsPluginStreamPeer::GetMIMEType(nsMIMEType *result)
{
    *result = (nsMIMEType)urls->content_type;
    return NS_OK;
}

NS_METHOD
nsPluginStreamPeer::GetContentLength(PRUint32 *result)
{
    *result = urls->content_length;
    return NS_OK;
}
#if 0
NS_METHOD_(const char*)
nsPluginStreamPeer::GetContentEncoding(void)
{
    return urls->content_encoding;
}

NS_METHOD_(const char*)
nsPluginStreamPeer::GetCharSet(void)
{
    return urls->charset;
}

NS_METHOD_(const char*)
nsPluginStreamPeer::GetBoundary(void)
{
    return urls->boundary;
}

NS_METHOD_(const char*)
nsPluginStreamPeer::GetContentName(void)
{
    return urls->content_name;
}

NS_METHOD_(time_t)
nsPluginStreamPeer::GetExpires(void)
{
    return urls->expires;
}

NS_METHOD_(time_t)
nsPluginStreamPeer::GetLastModified(void)
{
    return urls->last_modified;
}

NS_METHOD_(time_t)
nsPluginStreamPeer::GetServerDate(void)
{
    return urls->server_date;
}

NS_METHOD_(NPServerStatus)
nsPluginStreamPeer::GetServerStatus(void)
{
    return urls->server_status;
}
#endif
NS_METHOD
nsPluginStreamPeer::GetHeaderFieldCount(PRUint32 *result)
{
    *result = urls->all_headers.empty_index;
    return NS_OK;
}

NS_METHOD
nsPluginStreamPeer::GetHeaderFieldKey(PRUint32 index, const char* *result)
{
    *result = urls->all_headers.key[index];
    return NS_OK;
}

NS_METHOD
nsPluginStreamPeer::GetHeaderField(PRUint32 index, const char* *result)
{
    *result = urls->all_headers.value[index];
    return NS_OK;
}

NS_METHOD
nsPluginStreamPeer::RequestRead(nsByteRange* rangeList)
{
    NPError err = npn_requestread(stream->pstream,
                                  (NPByteRange*)rangeList);
    return fromNPError[err];
}

NS_IMPL_ADDREF(nsPluginStreamPeer);
NS_IMPL_RELEASE(nsPluginStreamPeer);

NS_METHOD
nsPluginStreamPeer::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{
    if (NULL == aInstancePtr) {
        return NS_ERROR_NULL_POINTER; 
    } 
    if ((stream->seekable && aIID.Equals(kISeekablePluginStreamPeerIID)) ||
		aIID.Equals(kIPluginStreamPeer2IID) ||
        aIID.Equals(kIPluginStreamPeerIID) ||
        aIID.Equals(kISupportsIID)) {
        *aInstancePtr = (void*)(nsISupports*)(nsIPluginStreamPeer2*)this; 
        AddRef(); 
        return NS_OK; 
    } 
    return NS_NOINTERFACE; 
} 

////////////////////////////////////////////////////////////////////////////////
