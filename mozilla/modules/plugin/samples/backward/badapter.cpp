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
// Backward Adapter
// This acts as a adapter layer to allow 5.0 plugins work with the 4.0/3.0 
// browser.
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// SECTION 1 - Includes
////////////////////////////////////////////////////////////////////////////////

// extern "C" {
#include "npapi.h"
// }
#include "nsplugin.h"
#include "nsDebug.h"

////////////////////////////////////////////////////////////////////////////////
// SECTION 3 - Classes
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
// CPluginManager
//
// This is the dummy plugin manager that interacts with the 5.0 plugin.
//
class CPluginManager : public nsIPluginManager {
public:

    CPluginManager(void);
    virtual ~CPluginManager(void);

    NS_DECL_ISUPPORTS

    ////////////////////////////////////////////////////////////////////////////
    // from nsIPluginManager:

    // (Corresponds to NPN_GetValue.)
    NS_IMETHOD
    GetValue(nsPluginManagerVariable variable, void *value);

    // (Corresponds to NPN_SetValue.)
    NS_IMETHOD
    SetValue(nsPluginManagerVariable variable, void *value);
    
    NS_IMETHOD
    ReloadPlugins(PRBool reloadPages);

    // (Corresponds to NPN_UserAgent.)
    NS_IMETHOD
    UserAgent(const char* *result);

#ifdef NEW_PLUGIN_STREAM_API

    NS_IMETHOD
    GetURL(nsISupports* pluginInst, 
           const char* url, 
           const char* target = NULL,
           nsIPluginStreamListener* streamListener = NULL,
           nsPluginStreamType streamType = nsPluginStreamType_Normal,
           const char* altHost = NULL,
           const char* referrer = NULL,
           PRBool forceJSEnabled = PR_FALSE);

    NS_IMETHOD
    PostURL(nsISupports* pluginInst,
            const char* url,
            PRUint32 postDataLen, 
            const char* postData,
            PRBool isFile = PR_FALSE,
            const char* target = NULL,
            nsIPluginStreamListener* streamListener = NULL,
            nsPluginStreamType streamType = nsPluginStreamType_Normal,
            const char* altHost = NULL, 
            const char* referrer = NULL,
            PRBool forceJSEnabled = PR_FALSE,
            PRUint32 postHeadersLength = 0, 
            const char* postHeaders = NULL);

#else // !NEW_PLUGIN_STREAM_API
    NS_IMETHOD
    GetURL(nsISupports* peer, const char* url, const char* target,
           void* notifyData = NULL, const char* altHost = NULL,
           const char* referrer = NULL, PRBool forceJSEnabled = PR_FALSE);

    NS_IMETHOD
    PostURL(nsISupports* peer, const char* url, const char* target,
            PRUint32 postDataLen, const char* postData,
            PRBool isFile = PR_FALSE, void* notifyData = NULL,
            const char* altHost = NULL, const char* referrer = NULL,
            PRBool forceJSEnabled = PR_FALSE,
            PRUint32 postHeadersLength = 0, const char* postHeaders = NULL);

#endif // !NEW_PLUGIN_STREAM_API

};

////////////////////////////////////////////////////////////////////////////////
//
// CPluginManagerStream
//
// This is the dummy plugin manager stream that interacts with the 5.0 plugin.
//
class CPluginManagerStream : public nsIOutputStream {

public:

    CPluginManagerStream(NPP npp, NPStream* pstr);
    virtual ~CPluginManagerStream(void);

    NS_DECL_ISUPPORTS

    //////////////////////////////////////////////////////////////////////////
    //
    // Taken from nsIStream
    //
    
    /** Write data into the stream.
     *  @param aBuf the buffer into which the data is read
     *  @param aOffset the start offset of the data
     *  @param aCount the maximum number of bytes to read
     *  @param errorResult the error code if an error occurs
     *  @return number of bytes read or -1 if error
     */   
    NS_IMETHOD
    Write(const char* aBuf, PRInt32 aOffset, PRInt32 aCount, PRInt32 *aWriteCount); 

    //////////////////////////////////////////////////////////////////////////
    //
    // Specific methods to nsIPluginManagerStream.
    //
    
    // Corresponds to NPStream's url field.
    NS_IMETHOD
    GetURL(const char*  *result);

    // Corresponds to NPStream's end field.
    NS_IMETHOD
    GetEnd(PRUint32 *result);

    // Corresponds to NPStream's lastmodfied field.
    NS_IMETHOD
    GetLastModified(PRUint32 *result);

    // Corresponds to NPStream's notifyData field.
    NS_IMETHOD
    GetNotifyData(void*  *result);

    // Corresponds to NPStream's url field.
    NS_IMETHOD Close(void);

protected:

    // npp
    // The plugin instance that the manager stream belongs to.
    NPP npp;

    // pstream
    // The stream the class is using.
    NPStream* pstream;

};

////////////////////////////////////////////////////////////////////////////////
//
// CPluginInstancePeer
//
// This is the dummy instance peer that interacts with the 5.0 plugin.
// In order to do LiveConnect, the class subclasses nsILiveConnectPluginInstancePeer.
//
class CPluginInstancePeer : public nsIPluginInstancePeer, public nsIPluginTagInfo {

public:

    // XXX - I add parameters to the constructor because I wasn't sure if
    // XXX - the 4.0 browser had the npp_instance struct implemented.
    // XXX - If so, then I can access npp_instance through npp->ndata.
    CPluginInstancePeer(nsIPluginInstance* pluginInstance, NPP npp, nsMIMEType typeString, nsPluginMode type,
        PRUint16 attribute_cnt, const char** attribute_list, const char** values_list);

    virtual ~CPluginInstancePeer(void);

    NS_DECL_ISUPPORTS

    // (Corresponds to NPN_GetValue.)
    NS_IMETHOD
    GetValue(nsPluginInstancePeerVariable variable, void *value);

    // (Corresponds to NPN_SetValue.)
    NS_IMETHOD
    SetValue(nsPluginInstancePeerVariable variable, void *value);

    // Corresponds to NPP_New's MIMEType argument.
    NS_IMETHOD
    GetMIMEType(nsMIMEType *result);

    // Corresponds to NPP_New's mode argument.
    NS_IMETHOD
    GetMode(nsPluginMode *result);

    // Get a ptr to the paired list of attribute names and values,
    // returns the length of the array.
    //
    // Each name or value is a null-terminated string.
    NS_IMETHOD
    GetAttributes(PRUint16& n, const char* const*& names, const char* const*& values);

    // Get the value for the named attribute.  Returns null
    // if the attribute was not set.
    NS_IMETHOD
    GetAttribute(const char* name, const char* *result);

    // Corresponds to NPN_NewStream.
    NS_IMETHOD
    NewStream(nsMIMEType type, const char* target, nsIOutputStream* *result);

    // Corresponds to NPN_ShowStatus.
    NS_IMETHOD
    ShowStatus(const char* message);

    NS_IMETHOD
    SetWindowSize(PRUint32 width, PRUint32 height);

	nsIPluginInstance* GetInstance(void) { return mInstance; }
	NPP GetNPPInstance(void) { return npp; }
	
	void SetWindow(NPWindow* window) { mWindow = window; }
	NPWindow* GetWindow() { return mWindow; }
	
protected:

    NPP npp;
    // XXX - The next five variables may need to be here since I
    // XXX - don't think np_instance is available in 4.0X.
    nsIPluginInstance* mInstance;
    NPWindow* mWindow;
    nsMIMEType typeString;
	nsPluginMode type;
	PRUint16 attribute_cnt;
	char** attribute_list;
	char** values_list;
};

#ifdef NEW_PLUGIN_STREAM_API

class CPluginInputStream : public nsIPluginInputStream {
public:

    NS_DECL_ISUPPORTS

    ////////////////////////////////////////////////////////////////////////////
    // from nsIBaseStream:

    /** Close the stream. */
    NS_IMETHOD
    Close(void);

    ////////////////////////////////////////////////////////////////////////////
    // from nsIInputStream:

    /** Return the number of bytes in the stream
     *  @param aLength out parameter to hold the length
     *         of the stream. if an error occurs, the length
     *         will be undefined
     *  @return error status
     */
    NS_IMETHOD
    GetLength(PRInt32 *aLength);

    /** Read data from the stream.
     *  @param aErrorCode the error code if an error occurs
     *  @param aBuf the buffer into which the data is read
     *  @param aOffset the start offset of the data
     *  @param aCount the maximum number of bytes to read
     *  @param aReadCount out parameter to hold the number of
     *         bytes read, eof if 0. if an error occurs, the
     *         read count will be undefined
     *  @return error status
     */   
    NS_IMETHOD
    Read(char* aBuf, PRInt32 aOffset, PRInt32 aCount, PRInt32 *aReadCount); 

    ////////////////////////////////////////////////////////////////////////////
    // from nsIPluginInputStream:

    // (Corresponds to NPStream's lastmodified field.)
    NS_IMETHOD
    GetLastModified(PRUint32 *result);

    NS_IMETHOD
    RequestRead(nsByteRange* rangeList);

    ////////////////////////////////////////////////////////////////////////////
    // CPluginInputStream specific methods:

    CPluginInputStream(nsIPluginStreamListener* listener,
                       nsPluginStreamType streamType);
    virtual ~CPluginInputStream(void);

    void SetStreamInfo(NPP npp, NPStream* stream) {
        mNPP = npp;
        mStream = stream;
    }

    nsIPluginStreamListener* GetListener(void) { return mListener; }
    nsPluginStreamType GetStreamType(void) { return mStreamType; }

    nsresult SetReadBuffer(PRUint32 len, const char* buffer) {
        // XXX this has to be way more sophisticated
        mBuffer = strdup(buffer);
        mBufferLength = len;
        mAmountRead = 0;
        return NS_OK;
    }

protected:
    const char* mURL;
    nsIPluginStreamListener* mListener;
    nsPluginStreamType mStreamType;
    NPP mNPP;
    NPStream* mStream;
    char* mBuffer;
    PRUint32 mBufferLength;
    PRUint32 mAmountRead;

};

#else // !NEW_PLUGIN_STREAM_API

////////////////////////////////////////////////////////////////////////////////
//
// CPluginStreamPeer
//
// This is the dummy stream peer that interacts with the 5.0 plugin.
//
class CPluginStreamPeer : public nsISeekablePluginStreamPeer, public nsIPluginStreamPeer {

public:
    
    CPluginStreamPeer(nsMIMEType type, NPStream* npStream,
		PRBool seekable, PRUint16* stype);
    virtual ~CPluginStreamPeer();

    NS_DECL_ISUPPORTS

    // (Corresponds to NPStream's url field.)
    NS_IMETHOD
    GetURL(const char* *result);

    // (Corresponds to NPStream's end field.)
    NS_IMETHOD
    GetEnd(PRUint32 *result);

    // (Corresponds to NPStream's lastmodified field.)
    NS_IMETHOD
    GetLastModified(PRUint32 *result);

    // (Corresponds to NPStream's notifyData field.)
    NS_IMETHOD
    GetNotifyData(void* *result);

	//////////////////////////////////////////////////////////////////////////
    //
    // From nsIPluginStreamPeer
    //

    // Corresponds to NPP_DestroyStream's reason argument.
    NS_IMETHOD
    GetReason(nsPluginReason *result);

    // Corresponds to NPP_NewStream's MIMEType argument.
    NS_IMETHOD
    GetMIMEType(nsMIMEType *result);

    //////////////////////////////////////////////////////////////////////////
    //
    // From nsISeekablePluginStreamPeer
    //

    // Corresponds to NPN_RequestRead.
    NS_IMETHOD
    RequestRead(nsByteRange* rangeList);

protected:

	nsMIMEType type;
	NPStream* npStream;
	PRBool seekable;
	PRUint16* stype;
    nsPluginReason reason;

};

#endif // !NEW_PLUGIN_STREAM_API

//////////////////////////////////////////////////////////////////////////////

#ifdef XP_UNIX
#define TRACE(foo) trace(foo)
#endif

#ifdef XP_MAC
#undef assert
#define assert(cond)
#endif

#if defined(__cplusplus)
extern "C" {
#endif

////////////////////////////////////////////////////////////////////////////////
// SECTION 1 - Includes
////////////////////////////////////////////////////////////////////////////////

#if defined(XP_UNIX) || defined(XP_MAC)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#else
#include <windows.h>
#endif

////////////////////////////////////////////////////////////////////////////////
// SECTION 2 - Global Variables
////////////////////////////////////////////////////////////////////////////////

//
// thePlugin and thePluginManager are used in the life of the plugin.
//
// These two will be created on NPP_Initialize and destroyed on NPP_Shutdown.
//
nsIPluginManager* thePluginManager = NULL;
nsIPlugin* thePlugin = NULL;

//
// nsISupports IDs
//
// Interface IDs for nsISupports
//
NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
NS_DEFINE_IID(kIPluginIID, NS_IPLUGIN_IID);
NS_DEFINE_IID(kIPluginInstanceIID, NS_IPLUGININSTANCE_IID);
NS_DEFINE_IID(kIPluginManagerIID, NS_IPLUGINMANAGER_IID);
NS_DEFINE_IID(kIPluginTagInfoIID, NS_IPLUGINTAGINFO_IID);
NS_DEFINE_IID(kIOutputStreamIID, NS_IOUTPUTSTREAM_IID);
NS_DEFINE_IID(kIPluginInstancePeerIID, NS_IPLUGININSTANCEPEER_IID); 

#ifdef NEW_PLUGIN_STREAM_API
NS_DEFINE_IID(kIPluginInputStreamIID, NS_IPLUGININPUTSTREAM_IID);
#else // !NEW_PLUGIN_STREAM_API
NS_DEFINE_IID(kIPluginStreamPeerIID, NS_IPLUGINSTREAMPEER_IID);
NS_DEFINE_IID(kISeekablePluginStreamPeerIID, NS_ISEEKABLEPLUGINSTREAMPEER_IID);
#endif // !NEW_PLUGIN_STREAM_API

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
// SECTION 4 - API Shim Plugin Implementations
// Glue code to the 5.0x Plugin.
//
// Most of the NPP_* functions that interact with the plug-in will need to get 
// the instance peer from npp->pdata so it can get the plugin instance from the
// peer. Once the plugin instance is available, the appropriate 5.0 plug-in
// function can be called:
//          
//  CPluginInstancePeer* peer = (CPluginInstancePeer* )instance->pdata;
//  nsIPluginInstance* inst = peer->GetUserInstance();
//  inst->NewPluginAPIFunction();
//
// Similar steps takes place with streams.  The stream peer is stored in NPStream's
// pdata.  Get the peer, get the stream, call the function.
//

////////////////////////////////////////////////////////////////////////////////
// UNIX-only API calls
////////////////////////////////////////////////////////////////////////////////

#ifdef XP_UNIX
char* NPP_GetMIMEDescription(void)
{
    int freeFac = 0;
    //fprintf(stderr, "MIME description\n");
    if (thePlugin == NULL) {
        freeFac = 1;
        NSGetFactory(kIPluginIID, (nsIFactory** )&thePlugin, NULL);
    }
    //fprintf(stderr, "Allocated Plugin 0x%08x\n", thePlugin);
    const char * ret;
    nsresult err = thePlugin->GetMIMEDescription(&ret);
    if (err) return NULL;
    //fprintf(stderr, "Get response %s\n", ret);
    if (freeFac) {
        //fprintf(stderr, "Freeing plugin...");
        thePlugin->Release();
        thePlugin = NULL;
    }
    //fprintf(stderr, "Done\n");
    return (char*)ret;
}


//------------------------------------------------------------------------------
// Cross-Platform Plug-in API Calls
//------------------------------------------------------------------------------

//+++++++++++++++++++++++++++++++++++++++++++++++++
// NPP_SetValue:
//+++++++++++++++++++++++++++++++++++++++++++++++++

NPError
NPP_SetValue(NPP instance, NPNVariable variable, void *value)
{
    return NPERR_GENERIC_ERROR; // nothing to set
}

//+++++++++++++++++++++++++++++++++++++++++++++++++
// NPP_GetValue:
//+++++++++++++++++++++++++++++++++++++++++++++++++

NPError
NPP_GetValue(NPP instance, NPPVariable variable, void *value) {
    int freeFac = 0;
    //fprintf(stderr, "MIME description\n");
    if (thePlugin == NULL) {
        freeFac = 1;
        if (NSGetFactory(kIPluginIID, (nsIFactory**)&thePlugin, NULL) != NS_OK)
            return NPERR_GENERIC_ERROR;
    }
    //fprintf(stderr, "Allocated Plugin 0x%08x\n", thePlugin);
    nsresult err = thePlugin->GetValue((nsPluginVariable)variable, value);
    if (err) return NPERR_GENERIC_ERROR;
    //fprintf(stderr, "Get response %08x\n", ret);
    if (freeFac) {
        //fprintf(stderr, "Freeing plugin...");
        thePlugin->Release();
        thePlugin = NULL;
    }
    //fprintf(stderr, "Done\n");
    return NPERR_NO_ERROR;
}
#endif // XP_UNIX

//+++++++++++++++++++++++++++++++++++++++++++++++++
// NPP_Initialize:
// Provides global initialization for a plug-in, and returns an error value. 
//
// This function is called once when a plug-in is loaded, before the first instance
// is created. thePluginManager and thePlugin are both initialized.
//+++++++++++++++++++++++++++++++++++++++++++++++++

NPError
NPP_Initialize(void)
{
//    TRACE("NPP_Initialize\n");

    // Only call initialize the plugin if it hasn't been created.
    // This could happen if GetJavaClass() is called before
    // NPP Initialize.  
    if (thePluginManager == NULL) {
        // Create the plugin manager and plugin classes.
        thePluginManager = new CPluginManager();	
        if ( thePluginManager == NULL ) 
            return NPERR_OUT_OF_MEMORY_ERROR;  
        thePluginManager->AddRef();
    }
    nsresult error = NS_OK;  
    // On UNIX the plugin might have been created when calling NPP_GetMIMEType.
    if (thePlugin == NULL) {
        // create nsIPlugin factory
        error = (NPError)NSGetFactory(kIPluginIID, (nsIFactory**)&thePlugin, NULL);
	    if (error == NS_OK) {
	    	thePlugin->AddRef();
	    }
	}
	
    return (NPError) error;	
}

//+++++++++++++++++++++++++++++++++++++++++++++++++
// NPP_GetJavaClass:
// New in Netscape Navigator 3.0. 
// 
// NPP_GetJavaClass is called during initialization to ask your plugin
// what its associated Java class is. If you don't have one, just return
// NULL. Otherwise, use the javah-generated "use_" function to both
// initialize your class and return it. If you can't find your class, an
// error will be signalled by "use_" and will cause the Navigator to
// complain to the user.
//+++++++++++++++++++++++++++++++++++++++++++++++++

jref
NPP_GetJavaClass(void)
{
    // Only call initialize the plugin if it hasn't been `d.
#if 0
    if (thePluginManager == NULL) {
        // Create the plugin manager and plugin objects.
        NPError result = CPluginManager::Create();	
        if (result) return NULL;
        assert( thePluginManager != NULL );
        thePluginManager->AddRef();
        NP_CreatePlugin(thePluginManager, (nsIPlugin** )(&thePlugin));
        assert( thePlugin != NULL );
    }
    return thePlugin->GetJavaClass();
#endif
    return NULL;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++
// NPP_Shutdown:
// Provides global deinitialization for a plug-in. 
// 
// This function is called once after the last instance of your plug-in 
// is destroyed.  thePluginManager and thePlugin are delete at this time.
//+++++++++++++++++++++++++++++++++++++++++++++++++

void
NPP_Shutdown(void)
{
//    TRACE("NPP_Shutdown\n");

    if (thePlugin) {
        thePlugin->Release();
        thePlugin = NULL;
    }

    if (thePluginManager)  {
        thePluginManager->Release();
        thePluginManager = NULL;
    }
    
    return;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++
// NPP_New:
// Creates a new instance of a plug-in and returns an error value. 
// 
// A plugin instance peer and instance peer is created.  After
// a successful instansiation, the peer is stored in the plugin
// instance's pdata.
//+++++++++++++++++++++++++++++++++++++++++++++++++

NPError 
NPP_New(NPMIMEType pluginType,
	NPP instance,
	PRUint16 mode,
	int16 argc,
	char* argn[],
	char* argv[],
	NPSavedData* saved)
{
//    TRACE("NPP_New\n");
    
    if (instance == NULL)
        return NPERR_INVALID_INSTANCE_ERROR;

    // Create a new plugin instance and start it.
    nsIPluginInstance* pluginInstance = NULL;
    thePlugin->CreateInstance(NULL, kIPluginInstanceIID, (void**)&pluginInstance);
    if (pluginInstance == NULL) {
        return NPERR_OUT_OF_MEMORY_ERROR;
    } 
    
    // Create a new plugin instance peer,
    // XXX - Since np_instance is not implemented in the 4.0x browser, I
    // XXX - had to save the plugin parameter in the peer class.
    // XXX - Ask Warren about np_instance.
    CPluginInstancePeer* peer = new CPluginInstancePeer(pluginInstance, instance, (nsMIMEType)pluginType, 
						                                (nsPluginMode)mode, (PRUint16)argc, (const char** )argn, (const char** )argv);
    assert( peer != NULL );
    if (!peer) return NPERR_OUT_OF_MEMORY_ERROR;
    peer->AddRef();
    pluginInstance->Initialize(peer);
    pluginInstance->Start();
    // Set the user instance and store the peer in npp->pdata.
    instance->pdata = peer;
    peer->Release();

    return NPERR_NO_ERROR;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++
// NPP_Destroy:
// Deletes a specific instance of a plug-in and returns an error value. 
//
// The plugin instance peer and plugin instance are destroyed.
// The instance's pdata is set to NULL.
//+++++++++++++++++++++++++++++++++++++++++++++++++

NPError 
NPP_Destroy(NPP instance, NPSavedData** save)
{
//    TRACE("NPP_Destroy\n");
    
    if (instance == NULL)
        return NPERR_INVALID_INSTANCE_ERROR;
    
    CPluginInstancePeer* peer = (CPluginInstancePeer*) instance->pdata;
    nsIPluginInstance* pluginInstance = peer->GetInstance();
    pluginInstance->Stop();
    pluginInstance->Destroy();
    pluginInstance->Release();
	// peer->Release();
    instance->pdata = NULL;
    
    return NPERR_NO_ERROR;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++
// NPP_SetWindow:
// Sets the window in which a plug-in draws, and returns an error value. 
//+++++++++++++++++++++++++++++++++++++++++++++++++

NPError 
NPP_SetWindow(NPP instance, NPWindow* window)
{
//    TRACE("NPP_SetWindow\n");
    
    if (instance == NULL)
        return NPERR_INVALID_INSTANCE_ERROR;

    CPluginInstancePeer* peer = (CPluginInstancePeer*) instance->pdata;
    if ( peer == NULL)
        return NPERR_INVALID_PLUGIN_ERROR;

	// record the window in the peer, so we can deliver proper events.
	peer->SetWindow(window);

    nsIPluginInstance* pluginInstance = peer->GetInstance();
    if( pluginInstance == 0 )
        return NPERR_INVALID_PLUGIN_ERROR;

    return (NPError)pluginInstance->SetWindow((nsPluginWindow* ) window );
}

//+++++++++++++++++++++++++++++++++++++++++++++++++
// NPP_NewStream:
// Notifies an instance of a new data stream and returns an error value. 
//
// Create a stream peer and stream.  If succesful, save
// the stream peer in NPStream's pdata.
//+++++++++++++++++++++++++++++++++++++++++++++++++

NPError 
NPP_NewStream(NPP instance,
              NPMIMEType type,
              NPStream *stream, 
              NPBool seekable,
              PRUint16 *stype)
{
    // XXX - How do you set the fields of the peer stream and stream?
    // XXX - Looks like these field will have to be created since
    // XXX - We are not using np_stream.
   
//    TRACE("NPP_NewStream\n");

    if (instance == NULL)
        return NPERR_INVALID_INSTANCE_ERROR;
				
#ifdef NEW_PLUGIN_STREAM_API

    CPluginInputStream* inStr = (CPluginInputStream*)stream->notifyData;
    if (inStr == NULL)
        return NPERR_GENERIC_ERROR;
    nsPluginStreamInfo info;
    info.contentType = type;
    info.seekable = seekable;
    nsresult err = inStr->GetListener()->OnStartBinding(stream->url, &info);
    if (err) return err;

    inStr->SetStreamInfo(instance, stream);
    stream->pdata = inStr;
    *stype = inStr->GetStreamType();

#else // !NEW_PLUGIN_STREAM_API

    // Create a new plugin stream peer and plugin stream.
    CPluginStreamPeer* speer = new CPluginStreamPeer((nsMIMEType)type, stream,
                                                     (PRBool)seekable, stype); 
    if (speer == NULL) return NPERR_OUT_OF_MEMORY_ERROR;
    speer->AddRef();

    nsIPluginStream* pluginStream = NULL; 
    CPluginInstancePeer* peer = (CPluginInstancePeer*) instance->pdata;
    nsIPluginInstance* pluginInstance = peer->GetInstance();
    nsresult err = pluginInstance->NewStream(speer, &pluginStream);
    if (err) return NPERR_OUT_OF_MEMORY_ERROR;
    speer->Release();
    
    if (pluginStream == NULL)
        return NPERR_OUT_OF_MEMORY_ERROR;
		
    stream->pdata = (void*) pluginStream;
    err = pluginStream->GetStreamType((nsPluginStreamType*)stype);
    assert(err == NS_OK);
	
#endif // !NEW_PLUGIN_STREAM_API

    return NPERR_NO_ERROR;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++
// NPP_WriteReady:
// Returns the maximum number of bytes that an instance is prepared to accept
// from the stream. 
//+++++++++++++++++++++++++++++++++++++++++++++++++

int32 
NPP_WriteReady(NPP instance, NPStream *stream)
{
//    TRACE("NPP_WriteReady\n");

    if (instance == NULL)
        return -1;

#ifdef NEW_PLUGIN_STREAM_API

    CPluginInputStream* inStr = (CPluginInputStream*)stream->pdata;
    if (inStr == NULL)
        return -1;
    return NP_MAXREADY;
    
#else // !NEW_PLUGIN_STREAM_API

    nsIPluginStream* theStream = (nsIPluginStream*) stream->pdata;	
    if( theStream == 0 )
        return -1;
	
    return 8192;

#endif // !NEW_PLUGIN_STREAM_API
}


//+++++++++++++++++++++++++++++++++++++++++++++++++
// NPP_Write:
// Delivers data from a stream and returns the number of bytes written. 
//+++++++++++++++++++++++++++++++++++++++++++++++++

int32 
NPP_Write(NPP instance, NPStream *stream, int32 offset, int32 len, void *buffer)
{
//    TRACE("NPP_Write\n");

    if (instance == NULL)
        return -1;
	
#ifdef NEW_PLUGIN_STREAM_API

    CPluginInputStream* inStr = (CPluginInputStream*)stream->pdata;
    if (inStr == NULL)
        return -1;
    nsresult err = inStr->SetReadBuffer((PRUint32)len, (const char*)buffer);
    if (err != NS_OK) return -1;
    err = inStr->GetListener()->OnDataAvailable(stream->url, inStr, offset, len);
    if (err != NS_OK) return -1;
    return len;
    
#else // !NEW_PLUGIN_STREAM_API

    nsIPluginStream* theStream = (nsIPluginStream*) stream->pdata;
    if( theStream == 0 )
        return -1;
		
    PRInt32 count;
    nsresult err = theStream->Write((const char* )buffer, offset, len, &count);
    return (err == NS_OK) ? count : -1;

#endif // !NEW_PLUGIN_STREAM_API
}

//+++++++++++++++++++++++++++++++++++++++++++++++++
// NPP_DestroyStream:
// Indicates the closure and deletion of a stream, and returns an error value. 
//
// The stream peer and stream are destroyed.  NPStream's
// pdata is set to NULL.
//+++++++++++++++++++++++++++++++++++++++++++++++++

NPError 
NPP_DestroyStream(NPP instance, NPStream *stream, NPReason reason)
{
//    TRACE("NPP_DestroyStream\n");

    if (instance == NULL)
        return NPERR_INVALID_INSTANCE_ERROR;
		
#ifdef NEW_PLUGIN_STREAM_API

    CPluginInputStream* inStr = (CPluginInputStream*)stream->pdata;
    if (inStr == NULL)
        return NPERR_GENERIC_ERROR;
    inStr->GetListener()->OnStopBinding(stream->url, (nsPluginReason)reason);
    inStr->Release();
    stream->pdata = NULL;
    
#else // !NEW_PLUGIN_STREAM_API

    nsIPluginStream* theStream = (nsIPluginStream*) stream->pdata;
    if( theStream == 0 )
        return NPERR_GENERIC_ERROR;
	
    theStream->Release();
    stream->pdata = NULL;

#endif // !NEW_PLUGIN_STREAM_API
	
    return NPERR_NO_ERROR;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++
// NPP_StreamAsFile:
// Provides a local file name for the data from a stream. 
//+++++++++++++++++++++++++++++++++++++++++++++++++

void 
NPP_StreamAsFile(NPP instance, NPStream *stream, const char* fname)
{
//    TRACE("NPP_StreamAsFile\n");

    if (instance == NULL)
        return;
		
#ifdef NEW_PLUGIN_STREAM_API

    CPluginInputStream* inStr = (CPluginInputStream*)stream->pdata;
    if (inStr == NULL)
        return;
    (void)inStr->GetListener()->OnFileAvailable(stream->url, fname);
    
#else // !NEW_PLUGIN_STREAM_API

    nsIPluginStream* theStream = (nsIPluginStream*) stream->pdata;
    if( theStream == 0 )
        return;

    theStream->AsFile( fname );

#endif // !NEW_PLUGIN_STREAM_API
}

//+++++++++++++++++++++++++++++++++++++++++++++++++
// NPP_Print:
//+++++++++++++++++++++++++++++++++++++++++++++++++

void 
NPP_Print(NPP instance, NPPrint* printInfo)
{
//    TRACE("NPP_Print\n");

    if(printInfo == NULL)   // trap invalid parm
        return;

    if (instance != NULL)
    {
        CPluginInstancePeer* peer = (CPluginInstancePeer*) instance->pdata;
        nsIPluginInstance* pluginInstance = peer->GetInstance();
        pluginInstance->Print((nsPluginPrint* ) printInfo );
    }
}

//+++++++++++++++++++++++++++++++++++++++++++++++++
// NPP_URLNotify:
// Notifies the instance of the completion of a URL request. 
//+++++++++++++++++++++++++++++++++++++++++++++++++

void
NPP_URLNotify(NPP instance, const char* url, NPReason reason, void* notifyData)
{
//    TRACE("NPP_URLNotify\n");

    if (instance != NULL) {
#ifdef NEW_PLUGIN_STREAM_API

        CPluginInputStream* inStr = (CPluginInputStream*)notifyData;
        (void)inStr->GetListener()->OnStopBinding(url, (nsPluginReason)reason);
    
#else // !NEW_PLUGIN_STREAM_API

        CPluginInstancePeer* peer = (CPluginInstancePeer*) instance->pdata;
        nsIPluginInstance* pluginInstance = peer->GetInstance();
        pluginInstance->URLNotify(url, NULL, (nsPluginReason)reason, notifyData);

#endif // !NEW_PLUGIN_STREAM_API
    }
}

//+++++++++++++++++++++++++++++++++++++++++++++++++
// NPP_HandleEvent:
// Mac-only, but stub must be present for Windows
// Delivers a platform-specific event to the instance. 
//+++++++++++++++++++++++++++++++++++++++++++++++++

#ifndef XP_UNIX
int16
NPP_HandleEvent(NPP instance, void* event)
{
//    TRACE("NPP_HandleEvent\n");
    int16 eventHandled = FALSE;
    if (instance == NULL)
        return eventHandled;
	
    NPEvent* npEvent = (NPEvent*) event;
    nsPluginEvent pluginEvent = {
#ifdef XP_MAC
        npEvent, NULL
#else
        npEvent->event, npEvent->wParam, npEvent->lParam
#endif
    };
	
    CPluginInstancePeer* peer = (CPluginInstancePeer*) instance->pdata;
    nsIPluginInstance* pluginInstance = peer->GetInstance();
    if (pluginInstance) {
        PRBool handled;
        nsresult err = pluginInstance->HandleEvent(&pluginEvent, &handled);
        if (err) return FALSE;
        eventHandled = (handled == PR_TRUE);
    }
	
    return eventHandled;
}
#endif // ndef XP_UNIX 

//////////////////////////////////////////////////////////////////////////////
// SECTION 5 - API Browser Implementations
//
// Glue code to the 4.0x Browser.
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//
// CPluginManager
//

//******************************************************************************
//
// Once we moved to the new APIs, we need to implement fJVMMgr.
//
//******************************************************************************

CPluginManager::CPluginManager(void) 
{
    // Set reference count to 0.
    NS_INIT_REFCNT();
}

CPluginManager::~CPluginManager(void) 
{
}

#if 0
//+++++++++++++++++++++++++++++++++++++++++++++++++
// MemAlloc:
//+++++++++++++++++++++++++++++++++++++++++++++++++

NS_METHOD
CPluginManager::MemAlloc(PRUint32 size)
{
    return NPN_MemAlloc(size);
}

//+++++++++++++++++++++++++++++++++++++++++++++++++
// MemFree:
//+++++++++++++++++++++++++++++++++++++++++++++++++

NS_METHOD
CPluginManager::MemFree(void* ptr)
{
    assert( ptr != NULL );

    NPN_MemFree(ptr);
}

//+++++++++++++++++++++++++++++++++++++++++++++++++
// MemFlush:
//+++++++++++++++++++++++++++++++++++++++++++++++++

NS_METHOD
CPluginManager::MemFlush(PRUint32 size)
{
#ifdef XP_MAC
    return NPN_MemFlush(size);	
#else
    return 0;
#endif
}
#endif

//+++++++++++++++++++++++++++++++++++++++++++++++++
// ReloadPlugins:
//+++++++++++++++++++++++++++++++++++++++++++++++++

NS_METHOD
CPluginManager::ReloadPlugins(PRBool reloadPages)
{
    NPN_ReloadPlugins(reloadPages);
    return NS_OK;
}

#ifdef NEW_PLUGIN_STREAM_API

NS_METHOD
CPluginManager::GetURL(nsISupports* pluginInst, 
                       const char* url, 
                       const char* target,
                       nsIPluginStreamListener* streamListener,
                       nsPluginStreamType streamType,
                       const char* altHost,
                       const char* referrer,
                       PRBool forceJSEnabled)
{
    if (altHost != NULL || referrer != NULL || forceJSEnabled != PR_FALSE) {
        return NPERR_INVALID_PARAM;
    }

    nsIPluginInstance* inst = NULL;
    nsresult rslt = pluginInst->QueryInterface(kIPluginInstanceIID, (void**)&inst);
    if (rslt != NS_OK) return rslt;
	CPluginInstancePeer* instancePeer = NULL;
    rslt = inst->GetPeer((nsIPluginInstancePeer**)&instancePeer);
    if (rslt != NS_OK) {
        inst->Release();
        return rslt;
    }
    NPP npp = instancePeer->GetNPPInstance();

    NPError err;
    if (streamListener) {
        CPluginInputStream* inStr = new CPluginInputStream(streamListener, streamType);
        if (inStr == NULL) {
            instancePeer->Release();
            inst->Release();
            return NS_ERROR_OUT_OF_MEMORY;
        }
        inStr->AddRef();
    
        err = NPN_GetURLNotify(npp, url, target, inStr);
    }
    else {
        err = NPN_GetURL(npp, url, target);
    }
    instancePeer->Release();
    inst->Release();
    return fromNPError[err];
}

NS_METHOD
CPluginManager::PostURL(nsISupports* pluginInst,
                        const char* url,
                        PRUint32 postDataLen, 
                        const char* postData,
                        PRBool isFile,
                        const char* target,
                        nsIPluginStreamListener* streamListener,
                        nsPluginStreamType streamType,
                        const char* altHost, 
                        const char* referrer,
                        PRBool forceJSEnabled,
                        PRUint32 postHeadersLength, 
                        const char* postHeaders)
{
    if (altHost != NULL || referrer != NULL || forceJSEnabled != PR_FALSE
        || postHeadersLength != 0 || postHeaders != NULL) {
        return NPERR_INVALID_PARAM;
    }

    nsIPluginInstance* inst = NULL;
    nsresult rslt = pluginInst->QueryInterface(kIPluginInstanceIID, (void**)&inst);
    if (rslt != NS_OK) return rslt;
	CPluginInstancePeer* instancePeer = NULL;
    rslt = inst->GetPeer((nsIPluginInstancePeer**)&instancePeer);
    if (rslt != NS_OK) {
        inst->Release();
        return rslt;
    }
    NPP npp = instancePeer->GetNPPInstance();

    NPError err;
    if (streamListener) {
        CPluginInputStream* inStr = new CPluginInputStream(streamListener, streamType);
        if (inStr == NULL) {
            instancePeer->Release();
            inst->Release();
            return NS_ERROR_OUT_OF_MEMORY;
        }
        inStr->AddRef();
    
        err = NPN_PostURLNotify(npp, url, target, postDataLen, postData, isFile, inStr);
    }
    else {
        err = NPN_PostURL(npp, url, target, postDataLen, postData, isFile);
    }
    instancePeer->Release();
    inst->Release();
    return fromNPError[err];
}

#else // !NEW_PLUGIN_STREAM_API

// (Corresponds to NPN_GetURL and NPN_GetURLNotify.)
//   notifyData: When present, URLNotify is called passing the notifyData back
//          to the client. When NULL, this call behaves like NPN_GetURL.
// New arguments:
//   peer:  A plugin instance peer. The peer's window will be used to display
//          progress information. If NULL, the load happens in the background.
//   altHost: An IP-address string that will be used instead of the host
//          specified in the URL. This is used to prevent DNS-spoofing attacks.
//          Can be defaulted to NULL meaning use the host in the URL.
//   referrer: 
//   forceJSEnabled: Forces JavaScript to be enabled for 'javascript:' URLs,
//          even if the user currently has JavaScript disabled. 
NS_METHOD
CPluginManager::GetURL(nsISupports* pinst, const char* url, const char* target,
                       void* notifyData, const char* altHost,
                       const char* referrer, PRBool forceJSEnabled)
{
    nsIPluginInstance* inst = NULL;
    nsresult rslt = pinst->QueryInterface(kIPluginInstanceIID, (void**)&inst);
    if (rslt != NS_OK) return rslt;
	CPluginInstancePeer* instancePeer = NULL;
    rslt = inst->GetPeer((nsIPluginInstancePeer**)&instancePeer);
    if (rslt != NS_OK) {
        inst->Release();
        return rslt;
    }
    NPP npp = instancePeer->GetNPPInstance();

    NPError err;
    // Call the correct GetURL* function.
    // This is determinded by checking notifyData.
    if (notifyData == NULL) {
        err = NPN_GetURL(npp, url, target);
    } else {
        err = NPN_GetURLNotify(npp, url, target, notifyData);
    }
    instancePeer->Release();
    inst->Release();
    return fromNPError[err];
}


NS_METHOD
CPluginManager::PostURL(nsISupports* pinst, const char* url, const char* target,
                        PRUint32 postDataLen, const char* postData,
                        PRBool isFile, void* notifyData,
                        const char* altHost, const char* referrer,
                        PRBool forceJSEnabled,
                        PRUint32 postHeadersLength, const char* postHeaders)
{
    nsIPluginInstance* inst = NULL;
    nsresult rslt = pinst->QueryInterface(kIPluginInstanceIID, (void**)&inst);
    if (rslt != NS_OK) return rslt;
	CPluginInstancePeer* instancePeer = NULL;
    rslt = inst->GetPeer((nsIPluginInstancePeer**)&instancePeer);
    if (rslt != NS_OK) {
        inst->Release();
        return rslt;
    }
    NPP npp = instancePeer->GetNPPInstance();

    NPError err;
    // Call the correct PostURL* function.
    // This is determinded by checking notifyData.
    if (notifyData == NULL) {
        err = NPN_PostURL(npp, url, target, postDataLen, postData, isFile);
    } else {
        err = NPN_PostURLNotify(npp, url, target, postDataLen, postData, isFile, notifyData);
    }
    instancePeer->Release();
    inst->Release();
    return fromNPError[err];
}

#endif // !NEW_PLUGIN_STREAM_API

//+++++++++++++++++++++++++++++++++++++++++++++++++
// UserAgent:
//+++++++++++++++++++++++++++++++++++++++++++++++++

NS_METHOD
CPluginManager::UserAgent(const char* *result)
{
    *result = NPN_UserAgent(NULL);
    return NS_OK;
}


int varMap[] = {
    (int)NPNVxDisplay,                  // nsPluginManagerVariable_XDisplay = 1,
    (int)NPNVxtAppContext,              // nsPluginManagerVariable_XtAppContext,
    (int)NPNVnetscapeWindow,            // nsPluginManagerVariable_NetscapeWindow,
    (int)NPPVpluginWindowBool,          // nsPluginInstancePeerVariable_WindowBool,
    (int)NPPVpluginTransparentBool,     // nsPluginInstancePeerVariable_TransparentBool,
    (int)NPPVjavaClass,                 // nsPluginInstancePeerVariable_JavaClass,
    (int)NPPVpluginWindowSize,          // nsPluginInstancePeerVariable_WindowSize,
    (int)NPPVpluginTimerInterval,       // nsPluginInstancePeerVariable_TimerInterval
};

//+++++++++++++++++++++++++++++++++++++++++++++++++
// GetValue:
//+++++++++++++++++++++++++++++++++++++++++++++++++

NS_METHOD
CPluginManager::GetValue(nsPluginManagerVariable variable, void *value)
{
#ifdef XP_UNIX
    return fromNPError[NPN_GetValue(NULL, (NPNVariable)varMap[(int)variable], value)];
#else
    return fromNPError[NPERR_GENERIC_ERROR];
#endif // XP_UNIX
}

//+++++++++++++++++++++++++++++++++++++++++++++++++
// SetValue:
//+++++++++++++++++++++++++++++++++++++++++++++++++

NS_METHOD
CPluginManager::SetValue(nsPluginManagerVariable variable, void *value) 
{
#ifdef XP_UNIX
    return fromNPError[NPN_SetValue(NULL, (NPPVariable)varMap[(int)variable], value)];
#else
    return fromNPError[NPERR_GENERIC_ERROR];
#endif // XP_UNIX
}

//+++++++++++++++++++++++++++++++++++++++++++++++++
// nsISupports functions
//+++++++++++++++++++++++++++++++++++++++++++++++++

NS_IMPL_ADDREF(CPluginManager);
NS_IMPL_RELEASE(CPluginManager);

NS_METHOD
CPluginManager::QueryInterface(const nsIID& iid, void** ptr) 
{                                                                        
    if (NULL == ptr) {                                            
        return NS_ERROR_NULL_POINTER;                                        
    }                                                                      
  
    if (iid.Equals(kIPluginManagerIID)) {
        *ptr = (void*) (nsIPluginManager*)this;                                        
        AddRef();                                                            
        return NS_OK;                                                        
    }                                                                      
    if (iid.Equals(kISupportsIID)) {                                      
        *ptr = (void*) ((nsIPluginManager*)this);                        
        AddRef();                                                            
        return NS_OK;                                                        
    }                                                                      
    return NS_NOINTERFACE;                                                 
}


//////////////////////////////////////////////////////////////////////////////
//
// CPluginInstancePeer
//

CPluginInstancePeer::CPluginInstancePeer(nsIPluginInstance* pluginInstance,
                                         NPP npp,
                                         nsMIMEType typeString, 
                                         nsPluginMode type,
                                         PRUint16 attr_cnt, 
                                         const char** attr_list,
                                         const char** val_list)
    :	mInstance(pluginInstance), mWindow(NULL),
		npp(npp), typeString(typeString), type(type), attribute_cnt(attr_cnt),
		attribute_list(NULL), values_list(NULL)
{
    // Set the reference count to 0.
    NS_INIT_REFCNT();
    
    mInstance->AddRef();

    attribute_list = (char**) NPN_MemAlloc(attr_cnt * sizeof(const char*));
    values_list = (char**) NPN_MemAlloc(attr_cnt * sizeof(const char*));

    if (attribute_list != NULL && values_list != NULL) {
        for (int i = 0; i < attribute_cnt; i++)   {
            attribute_list[i] = (char*) NPN_MemAlloc(strlen(attr_list[i]) + 1);
            if (attribute_list[i] != NULL)
                strcpy(attribute_list[i], attr_list[i]);

            values_list[i] = (char*) NPN_MemAlloc(strlen(val_list[i]) + 1);
            if (values_list[i] != NULL)
                strcpy(values_list[i], val_list[i]);
        }
    }
}

CPluginInstancePeer::~CPluginInstancePeer(void) 
{
    if (attribute_list != NULL && values_list != NULL) {
        for (int i = 0; i < attribute_cnt; i++)   {
            NPN_MemFree(attribute_list[i]);
            NPN_MemFree(values_list[i]);
        }

        NPN_MemFree(attribute_list);
        NPN_MemFree(values_list);
    }
}   


//+++++++++++++++++++++++++++++++++++++++++++++++++
// GetValue:
//+++++++++++++++++++++++++++++++++++++++++++++++++

NS_METHOD
CPluginInstancePeer::GetValue(nsPluginInstancePeerVariable variable, void *value)
{
#ifdef XP_UNIX
    return fromNPError[NPN_GetValue(NULL, (NPNVariable)varMap[(int)variable], value)];
#else
    return fromNPError[NPERR_GENERIC_ERROR];
#endif // XP_UNIX
}

//+++++++++++++++++++++++++++++++++++++++++++++++++
// SetValue:
//+++++++++++++++++++++++++++++++++++++++++++++++++

NS_METHOD
CPluginInstancePeer::SetValue(nsPluginInstancePeerVariable variable, void *value) 
{
#ifdef XP_UNIX
    return fromNPError[NPN_SetValue(NULL, (NPPVariable)varMap[(int)variable], value)];
#else
    return fromNPError[NPERR_GENERIC_ERROR];
#endif // XP_UNIX
}

//+++++++++++++++++++++++++++++++++++++++++++++++++
// GetMIMEType:
// Corresponds to NPP_New's MIMEType argument.
//+++++++++++++++++++++++++++++++++++++++++++++++++

NS_METHOD
CPluginInstancePeer::GetMIMEType(nsMIMEType *result) 
{
    *result = typeString;
    return NS_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++
// GetMode:
// Corresponds to NPP_New's mode argument.
//+++++++++++++++++++++++++++++++++++++++++++++++++

NS_METHOD
CPluginInstancePeer::GetMode(nsPluginMode *result)
{
    *result = type;
    return NS_OK;
}


// Get a ptr to the paired list of attribute names and values,
// returns the length of the array.
//
// Each name or value is a null-terminated string.
NS_METHOD
CPluginInstancePeer::GetAttributes(PRUint16& n, const char* const*& names, const char* const*& values)  
{
    n = attribute_cnt;
    names = attribute_list;
    values = values_list;

    return NS_OK;
}

#if defined(XP_MAC)

inline unsigned char toupper(unsigned char c)
{
    return (c >= 'a' && c <= 'z') ? (c - ('a' - 'A')) : c;
}

static int strcasecmp(const char * str1, const char * str2)
{
#if __POWERPC__
	
    const	unsigned char * p1 = (unsigned char *) str1 - 1;
    const	unsigned char * p2 = (unsigned char *) str2 - 1;
    unsigned long		c1, c2;
		
    while (toupper(c1 = *++p1) == toupper(c2 = *++p2))
        if (!c1)
            return(0);

#else
	
    const	unsigned char * p1 = (unsigned char *) str1;
    const	unsigned char * p2 = (unsigned char *) str2;
    unsigned char		c1, c2;
	
    while (toupper(c1 = *p1++) == toupper(c2 = *p2++))
        if (!c1)
            return(0);

#endif
	
    return(toupper(c1) - toupper(c2));
}

#endif /* XP_MAC */

// Get the value for the named attribute.  Returns null
// if the attribute was not set.
NS_METHOD
CPluginInstancePeer::GetAttribute(const char* name, const char* *result) 
{
    for (int i=0; i < attribute_cnt; i++)  {
#if defined(XP_UNIX) || defined(XP_MAC)
        if (strcasecmp(name, attribute_list[i]) == 0)
#else
            if (stricmp(name, attribute_list[i]) == 0) 
#endif
            {
                *result = values_list[i];
                return NS_OK;
            }
    }

    return NS_ERROR_FAILURE;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++
// NewStream:
//+++++++++++++++++++++++++++++++++++++++++++++++++
NS_METHOD
CPluginInstancePeer::NewStream(nsMIMEType type, const char* target, 
                               nsIOutputStream* *result)
{
    assert( npp != NULL );
    
    // Create a new NPStream.
    NPStream* ptr = NULL;
    NPError error = NPN_NewStream(npp, (NPMIMEType)type, target, &ptr);
    if (error) 
        return fromNPError[error];
    
    // Create a new Plugin Manager Stream.
    // XXX - Do we have to Release() the manager stream before doing this?
    // XXX - See the BAM doc for more info.
    CPluginManagerStream* mstream = new CPluginManagerStream(npp, ptr);
    if (mstream == NULL) 
        return NS_ERROR_OUT_OF_MEMORY;
    mstream->AddRef();
    *result = (nsIOutputStream* )mstream;

    return NS_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++
// ShowStatus:
//+++++++++++++++++++++++++++++++++++++++++++++++++

NS_METHOD
CPluginInstancePeer::ShowStatus(const char* message)
{
    assert( message != NULL );

    NPN_Status(npp, message);
	return NS_OK;
}

NS_METHOD
CPluginInstancePeer::SetWindowSize(PRUint32 width, PRUint32 height)
{
    NPError err;
    NPSize size;
    size.width = width;
    size.height = height;
    err = NPN_SetValue(npp, NPPVpluginWindowSize, &size);
    return fromNPError[err];
}

//+++++++++++++++++++++++++++++++++++++++++++++++++
// nsISupports functions
//+++++++++++++++++++++++++++++++++++++++++++++++++

NS_IMPL_ADDREF(CPluginInstancePeer);
NS_IMPL_RELEASE(CPluginInstancePeer);

NS_METHOD
CPluginInstancePeer::QueryInterface(const nsIID& iid, void** ptr) 
{                                                                        
    if (NULL == ptr) {                                            
        return NS_ERROR_NULL_POINTER;                                        
    }                                                                      
  
    if (iid.Equals(kIPluginInstancePeerIID)) {
        *ptr = (void*) this;                                        
        AddRef();                                                            
        return NS_OK;                                                        
    }                                                                      
    if (iid.Equals(kIPluginTagInfoIID) || iid.Equals(kISupportsIID)) {                                      
        *ptr = (void*) ((nsIPluginTagInfo*)this);                        
        AddRef();                                                            
        return NS_OK;                                                        
    }                                                                      
    return NS_NOINTERFACE;                                                 
}

//////////////////////////////////////////////////////////////////////////////
//
// CPluginManagerStream
//

CPluginManagerStream::CPluginManagerStream(NPP npp, NPStream* pstr)
    : npp(npp), pstream(pstr)
{
    // Set the reference count to 0.
    NS_INIT_REFCNT();
}

CPluginManagerStream::~CPluginManagerStream(void)
{
    //pstream = NULL;
    NPN_DestroyStream(npp, pstream, NPRES_DONE);
}


//+++++++++++++++++++++++++++++++++++++++++++++++++
// Write:
//+++++++++++++++++++++++++++++++++++++++++++++++++

NS_METHOD
CPluginManagerStream::Write(const char* buffer, PRInt32 offset, PRInt32 len, 
                            PRInt32 *aWriteCount)
{
    assert( npp != NULL );
    assert( pstream != NULL );

    assert(offset == 0);    // XXX need to handle the non-sequential write case
    *aWriteCount = NPN_Write(npp, pstream, len, (void* )buffer);
    return *aWriteCount >= 0 ? NS_OK : NS_ERROR_FAILURE;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++
// GetURL:
//+++++++++++++++++++++++++++++++++++++++++++++++++

NS_METHOD
CPluginManagerStream::GetURL(const char* *result)
{
    assert( pstream != NULL );

    *result = pstream->url;
	return NS_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++
// GetEnd:
//+++++++++++++++++++++++++++++++++++++++++++++++++

NS_METHOD
CPluginManagerStream::GetEnd(PRUint32 *result)
{
    assert( pstream != NULL );

    *result = pstream->end;
	return NS_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++
// GetLastModified:
//+++++++++++++++++++++++++++++++++++++++++++++++++

NS_METHOD
CPluginManagerStream::GetLastModified(PRUint32 *result)
{
    assert( pstream != NULL );

    *result = pstream->lastmodified;
	return NS_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++
// GetNotifyData:
//+++++++++++++++++++++++++++++++++++++++++++++++++

NS_METHOD
CPluginManagerStream::GetNotifyData(void* *result)
{
    assert( pstream != NULL );

    *result = pstream->notifyData;
	return NS_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++
// GetNotifyData:
//+++++++++++++++++++++++++++++++++++++++++++++++++

NS_METHOD
CPluginManagerStream::Close(void)
{
    assert( pstream != NULL );

    return NS_OK;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++
// nsISupports functions
//+++++++++++++++++++++++++++++++++++++++++++++++++

NS_IMPL_ADDREF(CPluginManagerStream);
NS_IMPL_RELEASE(CPluginManagerStream);

NS_IMPL_QUERY_INTERFACE(CPluginManagerStream, kIOutputStreamIID);

//////////////////////////////////////////////////////////////////////////////

#ifdef NEW_PLUGIN_STREAM_API

CPluginInputStream::CPluginInputStream(nsIPluginStreamListener* listener,
                                       nsPluginStreamType streamType)
    : mListener(listener), mStreamType(streamType),
      mNPP(NULL), mStream(NULL),
      mBuffer(NULL), mBufferLength(0), mAmountRead(0)
{
    NS_INIT_REFCNT();
    mListener->AddRef();
}

CPluginInputStream::~CPluginInputStream(void)
{
    mListener->Release();
    free(mBuffer);
}

NS_IMPL_ISUPPORTS(CPluginInputStream, kIPluginInputStreamIID);

NS_METHOD
CPluginInputStream::Close(void)
{
    if (mNPP == NULL || mStream == NULL)
        return NS_ERROR_FAILURE;
    NPError err = NPN_DestroyStream(mNPP, mStream, NPRES_USER_BREAK);
    return fromNPError[err];
}

NS_METHOD
CPluginInputStream::GetLength(PRInt32 *aLength)
{
    *aLength = mStream->end;
    return NS_OK;
}

NS_METHOD
CPluginInputStream::Read(char* aBuf, PRInt32 aOffset, PRInt32 aCount, PRInt32 *aReadCount)
{
    if (aOffset > (PRInt32)mBufferLength)
        return NS_ERROR_FAILURE;        // XXX right error?
    PRUint32 cnt = PR_MIN(aCount, (PRInt32)mBufferLength - aOffset);
    memcpy(aBuf, &mBuffer[aOffset], cnt);
    *aReadCount = cnt;
    mAmountRead -= cnt;
    return NS_OK;
}

NS_METHOD
CPluginInputStream::GetLastModified(PRUint32 *result)
{
    *result = mStream->lastmodified;
    return NS_OK;
}

NS_METHOD
CPluginInputStream::RequestRead(nsByteRange* rangeList)
{
    NPError err = NPN_RequestRead(mStream, (NPByteRange*)rangeList);
    return fromNPError[err];
}

#else // !NEW_PLUGIN_STREAM_API

//////////////////////////////////////////////////////////////////////////////
//
// CPluginStreamPeer
//

CPluginStreamPeer::CPluginStreamPeer(nsMIMEType type, NPStream* npStream,
                                     PRBool seekable, PRUint16* stype)
	: type(type), npStream(npStream), seekable(seekable),
	  stype(stype), reason(nsPluginReason_NoReason)
{
    // Set the reference count to 0.
    NS_INIT_REFCNT();
}

CPluginStreamPeer::~CPluginStreamPeer(void)
{
}

//+++++++++++++++++++++++++++++++++++++++++++++++++
// GetURL:
//+++++++++++++++++++++++++++++++++++++++++++++++++

NS_METHOD
CPluginStreamPeer::GetURL(const char* *result)
{
    *result = npStream->url;
    return NS_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++
// GetEnd:
//+++++++++++++++++++++++++++++++++++++++++++++++++

NS_METHOD
CPluginStreamPeer::GetEnd(PRUint32 *result)
{
    *result = npStream->end;
    return NS_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++
// GetLastModified:
//+++++++++++++++++++++++++++++++++++++++++++++++++

NS_METHOD
CPluginStreamPeer::GetLastModified(PRUint32 *result)
{
    *result = npStream->lastmodified;
    return NS_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++
// GetNotifyData:
//+++++++++++++++++++++++++++++++++++++++++++++++++

NS_METHOD
CPluginStreamPeer::GetNotifyData(void* *result)
{
    *result = npStream->notifyData;
    return NS_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++
// GetReason:
//+++++++++++++++++++++++++++++++++++++++++++++++++

NS_METHOD
CPluginStreamPeer::GetReason(nsPluginReason *result)
{
    *result = reason;
    return NS_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++
// GetMIMEType:
//+++++++++++++++++++++++++++++++++++++++++++++++++

NS_METHOD
CPluginStreamPeer::GetMIMEType(nsMIMEType *result)
{
    *result = type;
    return NS_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++
// RequestRead:
//+++++++++++++++++++++++++++++++++++++++++++++++++

NS_METHOD
CPluginStreamPeer::RequestRead(nsByteRange* rangeList)
{
    return fromNPError[NPN_RequestRead(npStream, (NPByteRange* )rangeList)];
}

//+++++++++++++++++++++++++++++++++++++++++++++++++
// nsISupports functions
//+++++++++++++++++++++++++++++++++++++++++++++++++

NS_IMPL_ADDREF(CPluginStreamPeer);
NS_IMPL_RELEASE(CPluginStreamPeer);

nsresult CPluginStreamPeer::QueryInterface(const nsIID& iid, void** ptr) 
{
    if (NULL == ptr) {
        return NS_ERROR_NULL_POINTER; 
    } 
    if (iid.Equals(kISeekablePluginStreamPeerIID))  {
        *ptr = (void*) ((nsISeekablePluginStreamPeer*)this); 
        AddRef(); 
        return NS_OK; 
	} else if (iid.Equals(kIPluginStreamPeerIID) ||
			   iid.Equals(kISupportsIID)) {
        *ptr = (void*) ((nsIPluginStreamPeer*)this); 
        AddRef(); 
        return NS_OK; 
    } 
    return NS_NOINTERFACE; 
} 

#endif // !NEW_PLUGIN_STREAM_API

//////////////////////////////////////////////////////////////////////////////

#if defined(__cplusplus)
} /* extern "C" */
#endif

