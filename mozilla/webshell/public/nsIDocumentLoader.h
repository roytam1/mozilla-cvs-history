/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */
#ifndef nsIDocumentLoader_h___
#define nsIDocumentLoader_h___

#include "nsweb.h"
#include "prtypes.h"
#include "nsISupports.h"
#include "nsIChannel.h"

/* Forward declarations... */
class nsString;
class nsIURI;
class nsIFactory;
class nsIPostData;
class nsIContentViewer;
class nsIContentViewerContainer;
class nsIStreamListener;
class nsIStreamObserver;
class nsIDocumentLoaderObserver;
class nsIDocument;
class nsIChannel;
class nsILoadGroup;

/* f43ba260-0737-11d2-beb9-00805f8a66dc */
#define NS_IDOCUMENTLOADERFACTORY_IID   \
 { 0xf43ba260, 0x0737, 0x11d2,{0xbe, 0xb9, 0x00, 0x80, 0x5f, 0x8a, 0x66, 0xdc}}

// Registered components that can instantiate a
// nsIDocumentLoaderFactory for a given mimetype must be prefixed with
// this prefix to be found. The format is <prefix>/%s/%s where the
// first %s is replaced with the command and the second %s is replaced
// with the mimetype. For example, to view a image/gif file you would
// create this progid:
//
//  "component://netscape/content-viewer-factory/view/image/gif"
//
#define NS_DOCUMENT_LOADER_FACTORY_PROGID_PREFIX \
  "component://netscape/content-viewer-factory/"

/*
 * The factory API for creating a content viewer for a given
 * content-type and command.
 */
class nsIDocumentLoaderFactory : public nsISupports
{
public:
    NS_DEFINE_STATIC_IID_ACCESSOR(NS_IDOCUMENTLOADERFACTORY_IID)

    NS_IMETHOD CreateInstance(const char *aCommand,
                              nsIChannel* aChannel,
                              nsILoadGroup* aLoadGroup,
                              const char* aContentType, 
                              nsISupports* aContainer,
                              nsISupports* aExtraInfo,
                              nsIStreamListener** aDocListenerResult,
                              nsIContentViewer** aDocViewerResult) = 0;

    NS_IMETHOD CreateInstanceForDocument(nsISupports* aContainer,
                                         nsIDocument* aDocument,
                                         const char *aCommand,
                                         nsIContentViewer** aDocViewerResult) = 0;
};

//----------------------------------------------------------------------

/* b9d685e0-fcae-11d1-beb9-00805f8a66dc */
#define NS_IDOCUMENTLOADER_IID   \
{ 0xb9d685e0, 0xfcae, 0x11d1, \
  {0xbe, 0xb9, 0x00, 0x80, 0x5f, 0x8a, 0x66, 0xdc} }

/*
 *
 *
 */
class nsIDocumentLoader : public nsISupports 
{
public:
    NS_DEFINE_STATIC_IID_ACCESSOR(NS_IDOCUMENTLOADER_IID)

    NS_IMETHOD LoadDocument(nsIURI * aUri, 
                            const char* aCommand,
                            nsISupports* aContainer,
                            nsIInputStream* aPostDataStream = nsnull,
                            nsISupports* aExtraInfo = nsnull,
                            nsLoadFlags aType = nsIChannel::LOAD_NORMAL,
                            const PRUint32 aLocalIP = 0,
                            const PRUnichar* aReferrer = nsnull) = 0;

    NS_IMETHOD Stop(void) = 0;

    NS_IMETHOD IsBusy(PRBool& aResult) = 0;

    NS_IMETHOD CreateDocumentLoader(nsIDocumentLoader** anInstance) = 0;

    NS_IMETHOD AddObserver(nsIDocumentLoaderObserver *aObserver) = 0;

    NS_IMETHOD RemoveObserver(nsIDocumentLoaderObserver *aObserver) = 0;

    NS_IMETHOD SetContainer(nsISupports* aContainer) = 0;

    NS_IMETHOD GetContainer(nsISupports** aResult) = 0;
    
    NS_IMETHOD GetContentViewerContainer(PRUint32 aDocumentID, nsIContentViewerContainer** aResult) = 0;

	NS_IMETHOD GetLoadGroup(nsILoadGroup** aResult) = 0;

    NS_IMETHOD Destroy() = 0;
};

/* 057b04d0-0ccf-11d2-beba-00805f8a66dc */
#define NS_DOCUMENTLOADER_SERVICE_CID   \
 { 0x057b04d0, 0x0ccf, 0x11d2,{0xbe, 0xba, 0x00, 0x80, 0x5f, 0x8a, 0x66, 0xdc}}

#endif /* nsIDocumentLoader_h___ */
