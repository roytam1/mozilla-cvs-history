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

/*

  A simple test program that reads in RDF/XML into an in-memory data
  source, then uses the RDF/XML serialization API to write an
  equivalent (but not identical) RDF/XML file back to stdout.

  The program takes a single parameter: the URL from which to read.

 */

#include "nsCOMPtr.h"
#include "nsIComponentManager.h"
#include "nsIEventQueueService.h"
#include "nsIGenericFactory.h"
#include "nsIIOService.h"
#include "nsIInputStream.h"
#include "nsIOutputStream.h"
#include "nsIRDFCompositeDataSource.h"
#include "nsIRDFNode.h"
#include "nsIRDFRemoteDataSource.h"
#include "nsIRDFService.h"
#include "nsIRDFXMLSource.h"
#include "nsIServiceManager.h"
#include "nsIStreamListener.h"
#include "nsIURL.h"
#include "nsRDFCID.h"
#include "plevent.h"
#include "plstr.h"
#include "prio.h"
#include "prthread.h"


////////////////////////////////////////////////////////////////////////
// CIDs

// rdf
static NS_DEFINE_CID(kRDFServiceCID,        NS_RDFSERVICE_CID);
static NS_DEFINE_CID(kRDFXMLDataSourceCID,  NS_RDFXMLDATASOURCE_CID);

// xpcom
static NS_DEFINE_CID(kEventQueueServiceCID, NS_EVENTQUEUESERVICE_CID);
static NS_DEFINE_CID(kGenericFactoryCID,    NS_GENERICFACTORY_CID);

////////////////////////////////////////////////////////////////////////
// To get the registry initialized!

#include "../../../webshell/tests/viewer/nsSetupRegistry.cpp"

////////////////////////////////////////////////////////////////////////

class ConsoleOutputStreamImpl : public nsIOutputStream
{
public:
    ConsoleOutputStreamImpl(void) { NS_INIT_REFCNT(); }
    virtual ~ConsoleOutputStreamImpl(void) {}

    // nsISupports interface
    NS_DECL_ISUPPORTS

    // nsIBaseStream interface
    NS_IMETHOD Close(void) {
        return NS_OK;
    }

    // nsIOutputStream interface
    NS_IMETHOD Write(const char* aBuf, PRUint32 aCount, PRUint32 *aWriteCount) {
        PR_Write(PR_GetSpecialFD(PR_StandardOutput), aBuf, aCount);
        *aWriteCount = aCount;
        return NS_OK;
    }

    NS_IMETHOD Flush(void) {
        PR_Sync(PR_GetSpecialFD(PR_StandardOutput));
        return NS_OK;
    }
};

NS_IMPL_ISUPPORTS(ConsoleOutputStreamImpl, nsCOMTypeInfo<nsIOutputStream>::GetIID());


////////////////////////////////////////////////////////////////////////

int
main(int argc, char** argv)
{
    nsresult rv;

    if (argc < 2) {
        fprintf(stderr, "usage: %s <url> [<poll-interval>]\n", argv[0]);
        return 1;
    }

    NS_SetupRegistry();

    // Get netlib off the floor...
    NS_WITH_SERVICE(nsIEventQueueService, theEventQueueService, kEventQueueServiceCID, &rv);
    if (NS_FAILED(rv)) return rv;

    rv = theEventQueueService->CreateThreadEventQueue();
    if (NS_FAILED(rv)) {
        fprintf(stderr, "unable to create thread event queue\n");
        return rv;
    }

    // Create a stream data source and initialize it on argv[1], which
    // is hopefully a "file:" URL.
    nsCOMPtr<nsIRDFDataSource> ds;
    rv = nsComponentManager::CreateInstance(kRDFXMLDataSourceCID,
                                            nsnull,
                                            nsIRDFDataSource::GetIID(),
                                            getter_AddRefs(ds));

    if (NS_FAILED(rv)) {
        fprintf(stderr, "unable to create RDF/XML data source\n");
        return rv;
    }

    nsCOMPtr<nsIRDFRemoteDataSource> remote
        = do_QueryInterface(ds);

    if (! remote)
        return NS_ERROR_UNEXPECTED;

    rv = remote->Init(argv[1]);
    if (NS_FAILED(rv)) {
        fprintf(stderr, "unable to initialize data source\n");
        return rv;
    }

    // Okay, this should load the XML file...
    rv = remote->Refresh(PR_TRUE);
    if (NS_FAILED(rv)) {
        fprintf(stderr, "unable to open file\n");
        return rv;
    }

    // And this should write it back out. The do_QI() on the pointer
    // is a hack to make sure that the new object gets AddRef()-ed.
    nsCOMPtr<nsIOutputStream> out =
        do_QueryInterface(new ConsoleOutputStreamImpl);

    if (! out) {
        fprintf(stderr, "unable to create console output stream\n");
        return NS_ERROR_OUT_OF_MEMORY;
    }

    nsCOMPtr<nsIRDFXMLSource> source = do_QueryInterface(ds);
    if (! source)
        return NS_ERROR_UNEXPECTED;

    rv = source->Serialize(out);
    if (NS_FAILED(rv)) {
        fprintf(stderr, "error serializing\n");
        return rv;
    }

    return NS_OK;
}
