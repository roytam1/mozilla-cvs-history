/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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

#include "nsNetUtil.h"

// XXX things to do:
//
//  - prune out unnecessary header files
//  - add xpcom-shutdown observer / cleanup gIOS foo
//  - possibly cache other xpcom factories
//  - is this a shared or static lib?
//  - add any other inline functions declared in necko IDL files (e.g.,
//    nsIFileStreams.idl)
//

// Helper, to simplify getting the I/O service.
const nsGetServiceByCID
do_GetIOService(nsresult* error)
{
    static NS_DEFINE_CID(kIOServiceCID, NS_IOSERVICE_CID);
    return nsGetServiceByCID(kIOServiceCID, 0, error);
}

nsresult
NS_NewURI(nsIURI **result, 
          const nsACString &spec, 
          const char *charset,
          nsIURI *baseURI,
          nsIIOService *ioService)     // pass in nsIIOService to optimize callers
{
    nsresult rv;

    nsCOMPtr<nsIIOService> serv;
    if (ioService == nsnull) {
        serv = do_GetIOService(&rv);
        if (NS_FAILED(rv)) return rv;
        ioService = serv.get();
    }

    return ioService->NewURI(spec, charset, baseURI, result);
}

nsresult
NS_NewFileURI(nsIURI* *result, 
              nsIFile* spec, 
              nsIIOService* ioService)     // pass in nsIIOService to optimize callers
{
    nsresult rv;

    nsCOMPtr<nsIIOService> serv;
    if (ioService == nsnull) {
        serv = do_GetIOService(&rv);
        if (NS_FAILED(rv)) return rv;
        ioService = serv.get();
    }

    return ioService->NewFileURI(spec, result);
}

nsresult
NS_NewChannel(nsIChannel* *result, 
              nsIURI* uri,
              nsIIOService* ioService,    // pass in nsIIOService to optimize callers
              nsILoadGroup* loadGroup,
              nsIInterfaceRequestor* notificationCallbacks,
              nsLoadFlags loadAttributes)
{
    nsresult rv;

    nsCOMPtr<nsIIOService> serv;
    if (ioService == nsnull) {
        serv = do_GetIOService(&rv);
        if (NS_FAILED(rv)) return rv;
        ioService = serv.get();
    }

    nsIChannel* channel = nsnull;
    rv = ioService->NewChannelFromURI(uri, &channel);
    if (NS_FAILED(rv)) return rv;

    if (loadGroup) {
        rv = channel->SetLoadGroup(loadGroup);
        if (NS_FAILED(rv)) return rv;
    }
    if (notificationCallbacks) {
        rv = channel->SetNotificationCallbacks(notificationCallbacks);
        if (NS_FAILED(rv)) return rv;
    }
    if (loadAttributes != nsIRequest::LOAD_NORMAL) {
        rv = channel->SetLoadFlags(loadAttributes);
        if (NS_FAILED(rv)) return rv;
    }

    *result = channel;
    return rv;
}

// Use this function with CAUTION. And do not use it on 
// the UI thread. It creates a stream that blocks when
// you Read() from it and blocking the UI thread is
// illegal. If you don't want to implement a full
// blown asyncrhonous consumer (via nsIStreamListener)
// look at nsIStreamLoader instead.
nsresult
NS_OpenURI(nsIInputStream* *result,
           nsIURI* uri,
           nsIIOService* ioService,     // pass in nsIIOService to optimize callers
           nsILoadGroup* loadGroup,
           nsIInterfaceRequestor* notificationCallbacks,
           nsLoadFlags loadAttributes)
{
    nsresult rv;
    nsCOMPtr<nsIChannel> channel;

    rv = NS_NewChannel(getter_AddRefs(channel), uri, ioService,
                       loadGroup, notificationCallbacks, loadAttributes);
    if (NS_FAILED(rv)) return rv;

    nsIInputStream* inStr;
    rv = channel->Open(&inStr);
    if (NS_FAILED(rv)) return rv;

    *result = inStr;
    return rv;
}

nsresult
NS_OpenURI(nsIStreamListener* aConsumer, 
           nsISupports* context, 
           nsIURI* uri,
           nsIIOService* ioService,     // pass in nsIIOService to optimize callers
           nsILoadGroup* loadGroup,
           nsIInterfaceRequestor* notificationCallbacks,
           nsLoadFlags loadAttributes)
{
    nsresult rv;
    nsCOMPtr<nsIChannel> channel;

    rv = NS_NewChannel(getter_AddRefs(channel), uri, ioService,
                       loadGroup, notificationCallbacks, loadAttributes);
    if (NS_FAILED(rv)) return rv;

    rv = channel->AsyncOpen(aConsumer, context);
    return rv;
}

nsresult
NS_MakeAbsoluteURI(nsACString &result,
                   const nsACString &spec, 
                   nsIURI *baseURI, 
                   nsIIOService *ioService)     // pass in nsIIOService to optimize callers
{
    if (!baseURI) {
        NS_WARNING("It doesn't make sense to not supply a base URI");
        result = spec;
        return NS_OK;
    }

    if (spec.IsEmpty())
        return baseURI->GetSpec(result);

    return baseURI->Resolve(spec, result);
}

nsresult
NS_MakeAbsoluteURI(char **result,
                   const char *spec, 
                   nsIURI *baseURI, 
                   nsIIOService *ioService)     // pass in nsIIOService to optimize callers
{
    nsCAutoString resultBuf;

    nsresult rv = NS_MakeAbsoluteURI(resultBuf, nsDependentCString(spec), baseURI, ioService);
    if (NS_FAILED(rv)) return rv;

    *result = ToNewCString(resultBuf);
    return *result ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

nsresult
NS_MakeAbsoluteURI(nsAString &result,
                   const nsAString &spec, 
                   nsIURI *baseURI,
                   nsIIOService *ioService)     // pass in nsIIOService to optimize callers
{
    if (!baseURI) {
        NS_WARNING("It doesn't make sense to not supply a base URI");
        result = spec;
        return NS_OK;
    }

    nsCAutoString resultBuf;
    nsresult rv;

    if (spec.IsEmpty())
        rv = baseURI->GetSpec(resultBuf);
    else
        rv = baseURI->Resolve(NS_ConvertUCS2toUTF8(spec), resultBuf);
    if (NS_FAILED(rv)) return rv;

    result = NS_ConvertUTF8toUCS2(resultBuf); // XXX CopyUTF8toUCS2
    return NS_OK;
}

nsresult
NS_NewPostDataStream(nsIInputStream **result,
                     PRBool isFile,
                     const nsACString &data,
                     PRUint32 encodeFlags,
                     nsIIOService* ioService)     // pass in nsIIOService to optimize callers
{
    nsresult rv;

    if (isFile) {
        nsCOMPtr<nsILocalFile> file;
        nsCOMPtr<nsIInputStream> fileStream;

        rv = NS_NewNativeLocalFile(data, PR_FALSE, getter_AddRefs(file));
        if (NS_FAILED(rv)) return rv;

        rv = NS_NewLocalFileInputStream(getter_AddRefs(fileStream), file);
        if (NS_FAILED(rv)) return rv;

        // wrap the file stream with a buffered input stream
        return NS_NewBufferedInputStream(result, fileStream, 8192);
    }

    // otherwise, create a string stream for the data
    return NS_NewCStringInputStream(result, data);
}

nsresult
NS_NewStreamIOChannel(nsIStreamIOChannel **result,
                      nsIURI* uri,
                      nsIStreamIO* io)
{
    nsresult rv;
    nsCOMPtr<nsIStreamIOChannel> channel;
    static NS_DEFINE_CID(kStreamIOChannelCID, NS_STREAMIOCHANNEL_CID);
    rv = nsComponentManager::CreateInstance(kStreamIOChannelCID,
                                            nsnull, 
                                            NS_GET_IID(nsIStreamIOChannel),
                                            getter_AddRefs(channel));
    if (NS_FAILED(rv)) return rv;
    rv = channel->Init(uri, io);
    if (NS_FAILED(rv)) return rv;

    *result = channel;
    NS_ADDREF(*result);
    return NS_OK;
}

nsresult
NS_NewInputStreamChannel(nsIChannel **result,
                         nsIURI* uri,
                         nsIInputStream* inStr,
                         const nsACString &contentType,
                         const nsACString &contentCharset,
                         PRInt32 contentLength)
{
    nsresult rv;
    nsCAutoString spec;
    rv = uri->GetSpec(spec);
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIInputStreamIO> io;
    rv = NS_NewInputStreamIO(getter_AddRefs(io), spec, inStr, 
                             contentType, contentCharset, contentLength);
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIStreamIOChannel> channel;
    rv = NS_NewStreamIOChannel(getter_AddRefs(channel), uri, io);
    if (NS_FAILED(rv)) return rv;

    *result = channel;
    NS_ADDREF(*result);
    return NS_OK;
}

nsresult
NS_NewLoadGroup(nsILoadGroup* *result, nsIRequestObserver* obs)
{
    nsresult rv;
    nsCOMPtr<nsILoadGroup> group;
    static NS_DEFINE_CID(kLoadGroupCID, NS_LOADGROUP_CID);
    rv = nsComponentManager::CreateInstance(kLoadGroupCID, nsnull, 
                                            NS_GET_IID(nsILoadGroup), 
                                            getter_AddRefs(group));
    if (NS_FAILED(rv)) return rv;
    rv = group->SetGroupObserver(obs);
    if (NS_FAILED(rv)) return rv;

    *result = group;
    NS_ADDREF(*result);
    return NS_OK;
}


nsresult
NS_NewDownloader(nsIDownloader* *result,
                   nsIURI* uri,
                   nsIDownloadObserver* observer,
                   nsISupports* context,
                 PRBool synchronous,
                   nsILoadGroup* loadGroup,
                   nsIInterfaceRequestor* notificationCallbacks,
                 nsLoadFlags loadAttributes)
{
    nsresult rv;
    nsCOMPtr<nsIDownloader> downloader;
    static NS_DEFINE_CID(kDownloaderCID, NS_DOWNLOADER_CID);
    rv = nsComponentManager::CreateInstance(kDownloaderCID,
                                            nsnull,
                                            NS_GET_IID(nsIDownloader),
                                            getter_AddRefs(downloader));
    if (NS_FAILED(rv)) return rv;
    rv = downloader->Init(uri, observer, context, synchronous, loadGroup,
                          notificationCallbacks, loadAttributes);
    if (NS_FAILED(rv)) return rv;
    *result = downloader;
    NS_ADDREF(*result);
    return rv;
}

nsresult
NS_NewStreamLoader(nsIStreamLoader **aResult,
                   nsIChannel *aChannel,
                   nsIStreamLoaderObserver *aObserver,
                   nsISupports *aContext)
{
    nsresult rv;
    nsCOMPtr<nsIStreamLoader> loader;
    static NS_DEFINE_CID(kStreamLoaderCID, NS_STREAMLOADER_CID);
    rv = nsComponentManager::CreateInstance(kStreamLoaderCID,
                                            nsnull,
                                            NS_GET_IID(nsIStreamLoader),
                                            getter_AddRefs(loader));
    if (NS_FAILED(rv)) return rv;

    rv = loader->Init(aChannel, aObserver, aContext);
    if (NS_FAILED(rv)) return rv;

    *aResult = loader;
    NS_ADDREF(*aResult);
    return rv;
}

nsresult
NS_NewStreamLoader(nsIStreamLoader* *result,
                   nsIURI* uri,
                   nsIStreamLoaderObserver* observer,
                   nsISupports* context,
                   nsILoadGroup* loadGroup,
                   nsIInterfaceRequestor* notificationCallbacks,
                   nsLoadFlags loadAttributes,
                   nsIURI* referrer)
{
    nsresult rv;

    nsCOMPtr<nsIChannel> channel;
    rv = NS_NewChannel(getter_AddRefs(channel),
                       uri,
                       nsnull,
                       loadGroup,
                       notificationCallbacks,
                       loadAttributes);
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(channel));
    if (httpChannel)
        httpChannel->SetReferrer(referrer);

    return NS_NewStreamLoader(result, channel, observer, context);
}

nsresult
NS_NewUnicharStreamLoader(nsIUnicharStreamLoader **aResult,
                          nsIChannel *aChannel,
                          nsIUnicharStreamLoaderObserver *aObserver,
                          nsISupports *aContext,
                          PRUint32 aSegmentSize)
{
    nsresult rv;
    nsCOMPtr<nsIUnicharStreamLoader> loader;
    static NS_DEFINE_CID(kUnicharStreamLoaderCID, NS_UNICHARSTREAMLOADER_CID);
    rv = nsComponentManager::CreateInstance(kUnicharStreamLoaderCID,
                                            nsnull,
                                            NS_GET_IID(nsIUnicharStreamLoader),
                                            getter_AddRefs(loader));
    if (NS_FAILED(rv)) return rv;
    rv = loader->Init(aChannel, aObserver, aContext, aSegmentSize);
                      
    if (NS_FAILED(rv)) return rv;
    *aResult = loader;
    NS_ADDREF(*aResult);
    return rv;
}

nsresult
NS_NewRequestObserverProxy(nsIRequestObserver **aResult,
                           nsIRequestObserver *aObserver,
                           nsIEventQueue *aEventQ)
{
    NS_ENSURE_ARG_POINTER(aResult);

    nsresult rv;
    nsCOMPtr<nsIRequestObserverProxy> proxy;
    static NS_DEFINE_CID(kRequestObserverProxyCID, NS_REQUESTOBSERVERPROXY_CID);

    rv = nsComponentManager::CreateInstance(kRequestObserverProxyCID,
                                            nsnull,
                                            NS_GET_IID(nsIRequestObserverProxy),
                                            getter_AddRefs(proxy));
    if (NS_FAILED(rv)) return rv;

    rv = proxy->Init(aObserver, aEventQ);
    if (NS_FAILED(rv)) return rv;

    return CallQueryInterface(proxy, aResult);
}

nsresult
NS_NewStreamListenerProxy(nsIStreamListener **aResult,
                          nsIStreamListener *aListener,
                          nsIEventQueue *aEventQ,
                          PRUint32 aBufferSegmentSize,
                          PRUint32 aBufferMaxSize)
{
    NS_ENSURE_ARG_POINTER(aResult);

    nsresult rv;
    nsCOMPtr<nsIStreamListenerProxy> proxy;
    static NS_DEFINE_CID(kStreamListenerProxyCID, NS_STREAMLISTENERPROXY_CID);

    rv = nsComponentManager::CreateInstance(kStreamListenerProxyCID,
                                            nsnull,
                                            NS_GET_IID(nsIStreamListenerProxy),
                                            getter_AddRefs(proxy));
    if (NS_FAILED(rv)) return rv;

    rv = proxy->Init(aListener, aEventQ, aBufferSegmentSize, aBufferMaxSize);
    if (NS_FAILED(rv)) return rv;

    NS_ADDREF(*aResult = proxy);
    return NS_OK;
}

nsresult
NS_NewStreamProviderProxy(nsIStreamProvider **aResult,
                          nsIStreamProvider *aProvider,
                          nsIEventQueue *aEventQ,
                          PRUint32 aBufferSegmentSize,
                          PRUint32 aBufferMaxSize)
{
    NS_ENSURE_ARG_POINTER(aResult);

    nsresult rv;
    nsCOMPtr<nsIStreamProviderProxy> proxy;
    static NS_DEFINE_CID(kStreamProviderProxyCID, NS_STREAMPROVIDERPROXY_CID);

    rv = nsComponentManager::CreateInstance(kStreamProviderProxyCID,
                                            nsnull,
                                            NS_GET_IID(nsIStreamProviderProxy),
                                            getter_AddRefs(proxy));
    if (NS_FAILED(rv)) return rv;

    rv = proxy->Init(aProvider, aEventQ, aBufferSegmentSize, aBufferMaxSize);
    if (NS_FAILED(rv)) return rv;

    NS_ADDREF(*aResult = proxy);
    return NS_OK;
}

nsresult
NS_NewSimpleStreamListener(nsIStreamListener **aResult,
                           nsIOutputStream *aSink,
                           nsIRequestObserver *aObserver)
{
    NS_ENSURE_ARG_POINTER(aResult);

    nsresult rv;
    nsCOMPtr<nsISimpleStreamListener> listener;
    static NS_DEFINE_CID(kSimpleStreamListenerCID, NS_SIMPLESTREAMLISTENER_CID);
    rv = nsComponentManager::CreateInstance(kSimpleStreamListenerCID,
                                            nsnull,
                                            NS_GET_IID(nsISimpleStreamListener),
                                            getter_AddRefs(listener));
    if (NS_FAILED(rv)) return rv;

    rv = listener->Init(aSink, aObserver);
    if (NS_FAILED(rv)) return rv;

    NS_ADDREF(*aResult = listener);
    return NS_OK;
}

nsresult
NS_NewSimpleStreamProvider(nsIStreamProvider **aResult,
                           nsIInputStream *aSource,
                           nsIRequestObserver *aObserver)
{
    NS_ENSURE_ARG_POINTER(aResult);

    nsresult rv;
    nsCOMPtr<nsISimpleStreamProvider> provider;
    static NS_DEFINE_CID(kSimpleStreamProviderCID, NS_SIMPLESTREAMPROVIDER_CID);
    rv = nsComponentManager::CreateInstance(kSimpleStreamProviderCID,
                                            nsnull,
                                            NS_GET_IID(nsISimpleStreamProvider),
                                            getter_AddRefs(provider));
    if (NS_FAILED(rv)) return rv;

    rv = provider->Init(aSource, aObserver);
    if (NS_FAILED(rv)) return rv;

    NS_ADDREF(*aResult = provider);
    return NS_OK;
}

/*
// Depracated, prefer NS_NewStreamObserverProxy
nsresult
NS_NewAsyncStreamObserver(nsIRequestObserver **result,
                          nsIRequestObserver *receiver,
                          nsIEventQueue *eventQueue)
{
    nsresult rv;
    nsCOMPtr<nsIAsyncStreamObserver> obs;
    static NS_DEFINE_CID(kAsyncStreamObserverCID, NS_ASYNCSTREAMOBSERVER_CID);
    rv = nsComponentManager::CreateInstance(kAsyncStreamObserverCID,
                                            nsnull, 
                                            NS_GET_IID(nsIAsyncStreamObserver),
                                            getter_AddRefs(obs));
    if (NS_FAILED(rv)) return rv;
    rv = obs->Init(receiver, eventQueue);
    if (NS_FAILED(rv)) return rv;

    NS_ADDREF(*result = obs);
    return NS_OK;
}
*/

// Depracated, prefer NS_NewStreamListenerProxy
nsresult
NS_NewAsyncStreamListener(nsIStreamListener **result,
                          nsIStreamListener *receiver,
                          nsIEventQueue *eventQueue)
{
    nsresult rv;
    nsCOMPtr<nsIAsyncStreamListener> lsnr;
    static NS_DEFINE_CID(kAsyncStreamListenerCID, NS_ASYNCSTREAMLISTENER_CID);
    rv = nsComponentManager::CreateInstance(kAsyncStreamListenerCID,
                                            nsnull, 
                                            NS_GET_IID(nsIAsyncStreamListener),
                                            getter_AddRefs(lsnr));
    if (NS_FAILED(rv)) return rv;
    rv = lsnr->Init(receiver, eventQueue);
    if (NS_FAILED(rv)) return rv;

    NS_ADDREF(*result = lsnr);
    return NS_OK;
}

// Depracated, prefer a true synchonous implementation
nsresult
NS_NewSyncStreamListener(nsIInputStream **aInStream, 
                         nsIOutputStream **aOutStream,
                         nsIStreamListener **aResult)
{
    nsresult rv;

    NS_ENSURE_ARG_POINTER(aInStream);
    NS_ENSURE_ARG_POINTER(aOutStream);

    nsCOMPtr<nsIInputStream> pipeIn;
    nsCOMPtr<nsIOutputStream> pipeOut;

    rv = NS_NewPipe(getter_AddRefs(pipeIn),
                    getter_AddRefs(pipeOut),
                    4*1024,   // NS_SYNC_STREAM_LISTENER_SEGMENT_SIZE
                    32*1024); // NS_SYNC_STREAM_LISTENER_BUFFER_SIZE
    if (NS_FAILED(rv)) return rv;

    rv = NS_NewSimpleStreamListener(aResult, pipeOut);
    if (NS_FAILED(rv)) return rv;

    NS_ADDREF(*aInStream = pipeIn);
    NS_ADDREF(*aOutStream = pipeOut);
    return NS_OK;
}

//
// Calls AsyncWrite on the specified transport, with a stream provider that
// reads data from the specified input stream.
//
nsresult
NS_AsyncWriteFromStream(nsIRequest **aRequest,
                        nsITransport *aTransport,
                        nsIInputStream *aSource,
                        PRUint32 aOffset,
                        PRUint32 aCount,
                        PRUint32 aFlags,
                        nsIRequestObserver *aObserver,
                        nsISupports *aContext)
{
    NS_ENSURE_ARG_POINTER(aTransport);

    nsresult rv;
    nsCOMPtr<nsIStreamProvider> provider;
    rv = NS_NewSimpleStreamProvider(getter_AddRefs(provider),
                                    aSource,
                                    aObserver);
    if (NS_FAILED(rv)) return rv;

    //
    // We can safely allow the transport impl to bypass proxying the provider
    // since we are using a simple stream provider.
    // 
    // A simple stream provider masks the OnDataWritable from consumers.  
    // Moreover, it makes an assumption about the underlying nsIInputStream
    // implementation: namely, that it is thread-safe and blocking.
    //
    // So, let's always make this optimization.
    //
    aFlags |= nsITransport::DONT_PROXY_PROVIDER;

    return aTransport->AsyncWrite(provider, aContext,
                                  aOffset,
                                  aCount,
                                  aFlags,
                                  aRequest);
}

//
// Calls AsyncRead on the specified transport, with a stream listener that
// writes data to the specified output stream.
//
nsresult
NS_AsyncReadToStream(nsIRequest **aRequest,
                     nsITransport *aTransport,
                     nsIOutputStream *aSink,
                     PRUint32 aOffset,
                     PRUint32 aCount,
                     PRUint32 aFlags,
                     nsIRequestObserver *aObserver,
                     nsISupports *aContext)
{
    NS_ENSURE_ARG_POINTER(aTransport);

    nsresult rv;
    nsCOMPtr<nsIStreamListener> listener;
    rv = NS_NewSimpleStreamListener(getter_AddRefs(listener),
                                    aSink,
                                    aObserver);
    if (NS_FAILED(rv)) return rv;

    return aTransport->AsyncRead(listener, aContext,
                                 aOffset,
                                 aCount,
                                 aFlags,
                                 aRequest);
}

nsresult
NS_CheckPortSafety(PRInt32 port, const char* scheme, nsIIOService* ioService)
{
    nsresult rv;

    nsCOMPtr<nsIIOService> serv;
    if (ioService == nsnull) {
        serv = do_GetIOService(&rv);
        if (NS_FAILED(rv)) return rv;
        ioService = serv.get();
    }

    PRBool allow;
    
    rv = ioService->AllowPort(port, scheme, &allow);
    if (NS_FAILED(rv)) {
        NS_ERROR("NS_CheckPortSafety: ioService->AllowPort failed\n");
        return rv;
    }
    
    if (!allow)
        return NS_ERROR_PORT_ACCESS_NOT_ALLOWED;

    return NS_OK;
}

nsresult
NS_NewProxyInfo(const char* type, const char* host, PRInt32 port, nsIProxyInfo* *result)
{
    nsresult rv;

    static NS_DEFINE_CID(kPPSServiceCID, NS_PROTOCOLPROXYSERVICE_CID);
    nsCOMPtr<nsIProtocolProxyService> pps = do_GetService(kPPSServiceCID,&rv);

    if (NS_FAILED(rv)) return rv;

    return pps->NewProxyInfo(type, host, port, result);
}

nsresult
NS_GetFileProtocolHandler(nsIFileProtocolHandler **result,
                          nsIIOService *ioService)
{
    nsresult rv;

    nsCOMPtr<nsIIOService> serv;
    if (ioService == nsnull) {
        serv = do_GetIOService(&rv);
        if (NS_FAILED(rv)) return rv;
        ioService = serv.get();
    }

    nsCOMPtr<nsIProtocolHandler> handler;
    rv = ioService->GetProtocolHandler("file", getter_AddRefs(handler));
    if (NS_FAILED(rv)) return rv;

    return CallQueryInterface(handler, result);
}

nsresult
NS_GetFileFromURLSpec(const nsACString &inURL, nsIFile **result,
                      nsIIOService *ioService)
{
    nsCOMPtr<nsIFileProtocolHandler> fileHandler;
    nsresult rv = NS_GetFileProtocolHandler(getter_AddRefs(fileHandler), ioService);
    if (NS_FAILED(rv)) return rv;

    return fileHandler->GetFileFromURLSpec(inURL, result);
}

nsresult
NS_GetURLSpecFromFile(nsIFile* aFile, nsACString &aUrl,
                      nsIIOService *ioService)
{
    nsCOMPtr<nsIFileProtocolHandler> fileHandler;
    nsresult rv = NS_GetFileProtocolHandler(getter_AddRefs(fileHandler), ioService);
    if (NS_FAILED(rv)) return rv;

    return fileHandler->GetURLSpecFromFile(aFile, aUrl);
}

nsresult
NS_NewResumableEntityID(nsIResumableEntityID** aRes,
                        PRUint32 size,
                        PRTime lastModified)
{
    nsresult rv;
    nsCOMPtr<nsIResumableEntityID> ent =
        do_CreateInstance(NS_RESUMABLEENTITYID_CONTRACTID,&rv);
    if (NS_FAILED(rv)) return rv;

    ent->SetSize(size);
    ent->SetLastModified(lastModified);

    *aRes = ent;
    NS_ADDREF(*aRes);
    return NS_OK;
}

nsresult
NS_ExamineForProxy(const char* scheme, const char* host, PRInt32 port, 
                   nsIProxyInfo* *proxyInfo)
{
    nsresult rv;

    static NS_DEFINE_CID(kPPSServiceCID, NS_PROTOCOLPROXYSERVICE_CID);
    nsCOMPtr<nsIProtocolProxyService> pps = do_GetService(kPPSServiceCID,&rv);

    if (NS_FAILED(rv)) return rv;

    nsCAutoString spec(scheme);
    spec.Append("://");
    spec.Append(host);
    spec.Append(':');
    spec.AppendInt(port);
    
    // XXXXX - Under no circumstances whatsoever should any code which
    // wants a uri do this. I do this here because I do not, in fact,
    // actually want a uri (the dummy uris created here may not be 
    // syntactically valid for the specific protocol), and all we need
    // is something which has a valid scheme, hostname, and a string
    // to pass to PAC if needed - bbaetz
    static NS_DEFINE_CID(kSTDURLCID, NS_STANDARDURL_CID);    
    nsCOMPtr<nsIURI> uri = do_CreateInstance(kSTDURLCID, &rv);
    if (NS_FAILED(rv)) return rv;
    rv = uri->SetSpec(spec);
    if (NS_FAILED(rv)) return rv;

    return pps->ExamineForProxy(uri, proxyInfo);
}

nsresult
NS_ParseContentType(const nsACString &rawContentType,
                    nsCString &contentType,
                    nsCString &contentCharset)
{
    // contentCharset is left untouched if not present in rawContentType
    nsACString::const_iterator begin, it, end;
    it = rawContentType.BeginReading(begin);
    rawContentType.EndReading(end);
    if (FindCharInReadable(';', it, end)) {
        contentType = Substring(begin, it);
        // now look for "charset=FOO" and extract "FOO"
        begin = ++it;
        if (FindInReadable(NS_LITERAL_CSTRING("charset="), begin, it = end)) {
            contentCharset = Substring(it, end);
            contentCharset.StripWhitespace();
        }
    }
    else
        contentType = rawContentType;
    ToLowerCase(contentType);
    contentType.StripWhitespace();
    return NS_OK;
}
