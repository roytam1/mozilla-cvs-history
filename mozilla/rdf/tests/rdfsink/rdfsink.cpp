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

#include "nsIEventQueueService.h"
#include "nsIInputStream.h"
#include "nsINetService.h"
#include "nsINetService.h"
#include "nsIOutputStream.h"
#include "nsIPostToServer.h"
#include "nsIRDFCompositeDataSource.h"
#include "nsIRDFXMLDataSource.h"
#include "nsIRDFDocument.h"
#include "nsIRDFNode.h"
#include "nsIRDFService.h"
#include "nsIRDFXMLSource.h"
#include "nsIServiceManager.h"
#include "nsIStreamListener.h"
#include "nsIURL.h"
#include "nsDOMCID.h"    // for NS_SCRIPT_NAMESET_REGISTRY_CID
#include "nsLayoutCID.h" // for NS_NAMESPACEMANAGER_CID
#include "nsParserCIID.h"
#include "nsRDFCID.h"
#include "nsRDFCID.h"
#include "nsRepository.h"
#include "nsXPComCIID.h"
#include "plevent.h"
#include "plstr.h"

#define RDF_DB "rdf:bookmarks"
#define SUCCESS 0
#define FAILURE -1


#if defined(XP_PC)
#define DOM_DLL    "jsdom.dll"
#define LAYOUT_DLL "raptorhtml.dll"
#define NETLIB_DLL "netlib.dll"
#define PARSER_DLL "raptorhtmlpars.dll"
#define RDF_DLL    "rdf.dll"
#define XPCOM_DLL  "xpcom32.dll"
#elif defined(XP_UNIX)
#define DOM_DLL    "libjsdom.so"
#define LAYOUT_DLL "libraptorhtml.so"
#define NETLIB_DLL "libnetlib.so"
#define PARSER_DLL "libraptorhtmlpars.so"
#define RDF_DLL    "librdf.so"
#define XPCOM_DLL  "libxpcom.so"
#elif defined(XP_MAC)
#define DOM_DLL    "DOM_DLL"
#define LAYOUT_DLL "LAYOUT_DLL"
#define NETLIB_DLL "NETLIB_DLL"
#define PARSER_DLL "PARSER_DLL"
#define RDF_DLL    "RDF_DLL"
#define XPCOM_DLL  "XPCOM_DLL"
#endif


////////////////////////////////////////////////////////////////////////
// CIDs

// netlib
static NS_DEFINE_CID(kNetServiceCID,            NS_NETSERVICE_CID);

// rdf
static NS_DEFINE_CID(kRDFBookMarkDataSourceCID, NS_RDFBOOKMARKDATASOURCE_CID);
static NS_DEFINE_CID(kRDFInMemoryDataSourceCID, NS_RDFINMEMORYDATASOURCE_CID);
static NS_DEFINE_CID(kRDFServiceCID,            NS_RDFSERVICE_CID);
static NS_DEFINE_CID(kRDFCompositeDataSourceCID, NS_RDFCOMPOSITEDATASOURCE_CID);
static NS_DEFINE_CID(kRDFContentSinkCID,        NS_RDFCONTENTSINK_CID);
static NS_DEFINE_CID(kRDFXMLDataSourceCID,      NS_RDFXMLDATASOURCE_CID);

// parser
static NS_DEFINE_CID(kParserCID,                NS_PARSER_IID);
static NS_DEFINE_CID(kWellFormedDTDCID,         NS_WELLFORMEDDTD_CID);

// layout
static NS_DEFINE_CID(kNameSpaceManagerCID,      NS_NAMESPACEMANAGER_CID);

// dom
static NS_DEFINE_IID(kScriptNameSetRegistryCID, NS_SCRIPT_NAMESET_REGISTRY_CID);

// xpcom
static NS_DEFINE_CID(kEventQueueServiceCID,     NS_EVENTQUEUESERVICE_CID);

////////////////////////////////////////////////////////////////////////
// IIDs

NS_DEFINE_IID(kIEventQueueServiceIID,  NS_IEVENTQUEUESERVICE_IID);
NS_DEFINE_IID(kIOutputStreamIID,       NS_IOUTPUTSTREAM_IID);
NS_DEFINE_IID(kIRDFXMLDataSourceIID,   NS_IRDFXMLDATASOURCE_IID);
NS_DEFINE_IID(kIRDFServiceIID,         NS_IRDFSERVICE_IID);
NS_DEFINE_IID(kIRDFXMLSourceIID,       NS_IRDFXMLSOURCE_IID);

static nsresult
SetupRegistry(void)
{
    // netlib
    nsRepository::RegisterFactory(kNetServiceCID,            NETLIB_DLL, PR_FALSE, PR_FALSE);

    // parser
    nsRepository::RegisterFactory(kParserCID,                PARSER_DLL, PR_FALSE, PR_FALSE);
    nsRepository::RegisterFactory(kWellFormedDTDCID,         PARSER_DLL, PR_FALSE, PR_FALSE);

    // layout
    nsRepository::RegisterFactory(kNameSpaceManagerCID,      LAYOUT_DLL, PR_FALSE, PR_FALSE);

    // dom
    nsRepository::RegisterFactory(kScriptNameSetRegistryCID, DOM_DLL,    PR_FALSE, PR_FALSE);

    // xpcom
    nsRepository::RegisterFactory(kEventQueueServiceCID,     XPCOM_DLL,  PR_FALSE, PR_FALSE);

    return NS_OK;
}


////////////////////////////////////////////////////////////////////////

class ConsoleOutputStreamImpl : public nsIOutputStream
{
public:
    ConsoleOutputStreamImpl(void) {}
    virtual ~ConsoleOutputStreamImpl(void) {}

    // nsISupports interface
    NS_DECL_ISUPPORTS

    // nsIBaseStream interface
    NS_IMETHOD Close(void) {
        return NS_OK;
    }

    // nsIOutputStream interface
    NS_IMETHOD Write(const char* aBuf, PRUint32 aOffset, PRUint32 aCount, PRUint32 *aWriteCount) {
        PR_Write(PR_GetSpecialFD(PR_StandardOutput), aBuf + aOffset, aCount);
        *aWriteCount = aCount;
        return NS_OK;
    }
};

NS_IMPL_ISUPPORTS(ConsoleOutputStreamImpl, kIOutputStreamIID);


////////////////////////////////////////////////////////////////////////

int
main(int argc, char** argv)
{
    nsresult rv;

    if (argc < 2) {
        fprintf(stderr, "usage: %s [url]\n", argv[0]);
        return 1;
    }

    SetupRegistry();

    nsIEventQueueService* theEventQueueService = nsnull;
    PLEventQueue* mainQueue      = nsnull;
    nsIRDFService* theRDFService = nsnull;
    nsIRDFXMLDataSource* ds      = nsnull;
    nsIOutputStream* out         = nsnull;
    nsIRDFXMLSource* source      = nsnull;

    // Get netlib off the floor...
    if (NS_FAILED(rv = nsServiceManager::GetService(kEventQueueServiceCID,
                                                    kIEventQueueServiceIID,
                                                    (nsISupports**) &theEventQueueService))) {
        NS_ERROR("unable to get event queue service");
        goto done;
    }

    if (NS_FAILED(rv = theEventQueueService->CreateThreadEventQueue())) {
        NS_ERROR("unable to create thread event queue");
        goto done;
    }

    if (NS_FAILED(rv = theEventQueueService->GetThreadEventQueue(PR_GetCurrentThread(),
                                                                 &mainQueue))) {
        NS_ERROR("unable to get event queue for current thread");
        goto done;
    }

    // Create a stream data source and initialize it on argv[1], which
    // is hopefully a "file:" URL. (Actually, we can do _any_ kind of
    // URL, but only a "file:" URL will be written back to disk.)
    if (NS_FAILED(rv = nsRepository::CreateInstance(kRDFXMLDataSourceCID,
                                                    nsnull,
                                                    kIRDFXMLDataSourceIID,
                                                    (void**) &ds))) {
        NS_ERROR("unable to create RDF/XML data source");
        goto done;
    }

    if (NS_FAILED(rv = ds->SetSynchronous(PR_TRUE))) {
        NS_ERROR("unable to mark data source as synchronous");
        goto done;
    }

    // Okay, this should load the XML file...
    if (NS_FAILED(rv = ds->Init(argv[1]))) {
        NS_ERROR("unable to initialize data source");
        goto done;
    }

    // And finally, write it back out.
    if ((out = new ConsoleOutputStreamImpl()) == nsnull) {
        NS_ERROR("unable to create console output stream");
        rv = NS_ERROR_OUT_OF_MEMORY;
        goto done;
    }

    NS_ADDREF(out);

    if (NS_FAILED(rv = ds->QueryInterface(kIRDFXMLSourceIID, (void**) &source))) {
        NS_ERROR("unable to RDF/XML interface");
        goto done;
    }

    if (NS_FAILED(rv = source->Serialize(out))) {
        NS_ERROR("error serializing");
        goto done;
    }

done:
    NS_IF_RELEASE(out);
    NS_IF_RELEASE(ds);
    if (theRDFService) {
        nsServiceManager::ReleaseService(kRDFServiceCID, theRDFService);
        theRDFService = nsnull;
    }
    if (theEventQueueService) {
        nsServiceManager::ReleaseService(kEventQueueServiceCID, theEventQueueService);
        theEventQueueService = nsnull;
    }
    return (NS_FAILED(rv) ? 1 : 0);
}
