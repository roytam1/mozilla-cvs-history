/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

#include "net_serv.h"
#include "net_strm.h"
#include "net.h"
#include "mktrace.h"

/* XXX: Legacy definitions... */
MWContext *new_stub_context();
void free_stub_context(MWContext *window_id);
void bam_exit_routine(URL_Struct *URL_s, int status, MWContext *window_id);

extern "C" {
extern char *XP_AppCodeName;
extern char *XP_AppVersion;
};


nsNetlibService::nsNetlibService()
{
    NS_INIT_REFCNT();

    m_stubContext = new_stub_context();

    /* Initialize netlib with 32 sockets... */
    NET_InitNetLib(0, 32);


    /* XXX: How should the User Agent get initialized? */
    XP_AppCodeName = strdup("Mozilla");
    XP_AppVersion = strdup("5.0 Netscape/5.0 (Windows;I;x86;en)");

}


NS_DEFINE_IID(kINetServiceIID, NS_INETSERVICE_IID);
NS_IMPL_ISUPPORTS(nsNetlibService,kINetServiceIID);


nsNetlibService::~nsNetlibService()
{
    TRACEMSG(("nsNetlibService is being destroyed...\n"));

    if (NULL != m_stubContext) {
        free_stub_context((MWContext *)m_stubContext);
        m_stubContext = NULL;
    }

    NET_ShutdownNetLib();
}


NS_IMETHODIMP nsNetlibService::OpenStream(nsIURL *aUrl, 
                                          nsIStreamNotification *aConsumer)
{
    URL_Struct *URL_s;
    nsConnectionInfo *pConn;

    if ((NULL == aConsumer) || (NULL == aUrl)) {
        return NS_FALSE;
    }

    /* Create the nsConnectionInfo object... */
    pConn = new nsConnectionInfo(aUrl, NULL, aConsumer);
    if (NULL == pConn) {
        return NS_FALSE;
    }
    pConn->AddRef();

    /* Create the URLStruct... */
    URL_s = NET_CreateURLStruct(aUrl->GetSpec(), NET_NORMAL_RELOAD);
    if (NULL == URL_s) {
        pConn->Release();
        return NS_FALSE;
    }

    /* 
     * Mark the URL as background loading.  This prevents many
     * client upcall notifications...
     */
    URL_s->load_background = TRUE;

    /*
     * Attach the Data Consumer to the URL_Struct.
     *
     * Both the Data Consumer and the URL_Struct are freed in the 
     * bam_exit_routine(...)
     */
    URL_s->fe_data = pConn;
    pConn->AddRef();

    /* Start the URL load... */
    NET_GetURL (URL_s,                      /* URL_Struct      */
                FO_CACHE_AND_PRESENT,       /* FO_Present_type */
                (MWContext *)m_stubContext, /* MWContext       */
                bam_exit_routine);          /* Exit routine... */

    /* Remember, the URL_s may have been freed ! */

    aUrl->Set(NET_URLStruct_Address(URL_s));

    return NS_OK;
}


NS_IMETHODIMP nsNetlibService::OpenBlockingStream(nsIURL *aUrl, 
                                              nsIStreamNotification *aConsumer,
                                              nsIInputStream **aNewStream)
{
    URL_Struct *URL_s;
    nsConnectionInfo *pConn;
    nsNetlibStream *pBlockingStream;

    if (NULL == aNewStream) {
        return NS_FALSE;
    }

    if (NULL != aUrl) {
        /* Create the blocking stream... */
        pBlockingStream = new nsBlockingStream();
        if (NULL == pBlockingStream) {
            goto loser;
        }
        /*
         * AddRef the new stream in anticipation of returning it... This will
         * keep it alive :-)
         */
        pBlockingStream->AddRef();

        /* Create the nsConnectionInfo object... */
        pConn = new nsConnectionInfo(aUrl, pBlockingStream, aConsumer);
        if (NULL == pConn) {
            pBlockingStream->Release();
            goto loser;
        }
        pConn->AddRef();

        /* Create the URLStruct... */
        URL_s = NET_CreateURLStruct(aUrl->GetSpec(), NET_NORMAL_RELOAD);
        if (NULL == URL_s) {
            pBlockingStream->Release();
            pConn->Release();
            goto loser;
        }

        /* 
         * Mark the URL as background loading.  This prevents many
         * client upcall notifications...
         */
        URL_s->load_background = TRUE;

        /*
         * Attach the ConnectionInfo object to the URL_Struct.
         *
         * Both the ConnectionInfo and the URL_Struct are freed in the 
         * bam_exit_routine(...)
         */
        URL_s->fe_data = pConn;

/*        printf("+++ Loading %s\n", aUrl); */

        /* Start the URL load... */
        NET_GetURL (URL_s,                      /* URL_Struct      */
                    FO_CACHE_AND_PRESENT,       /* FO_Present_type */
                    (MWContext *)m_stubContext, /* MWContext       */
                    bam_exit_routine);          /* Exit routine... */

        /* Remember, the URL_s may have been freed ! */

        *aNewStream = pBlockingStream;
        return NS_OK;
    }

loser:
    *aNewStream = NULL;
    return NS_FALSE;
}


extern "C" {
/*
 * Factory for creating instance of the NetlibService...
 */
NS_NET nsresult NS_NewINetService(nsINetService** aInstancePtrResult,
                                  nsISupports* aOuter)
{
    static nsNetlibService *pNetlib = NULL;

    if (NULL != aOuter) {
        return NS_ERROR_NO_AGGREGATION;
    }

    /* XXX: For now only allow a single instance of the Netlib Service */
    if (NULL == pNetlib) {
        pNetlib = new nsNetlibService();
    }

    if (NULL == pNetlib) {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    return pNetlib->QueryInterface(kINetServiceIID, (void**)aInstancePtrResult);
}

}; /* extern "C" */


/*
 * This is the generic exit routine for all URLs loaded via the new
 * BAM APIs...
 */
static void bam_exit_routine(URL_Struct *URL_s, int status, MWContext *window_id)
{
    TRACEMSG(("bam_exit_routine was called...\n"));

    if (NULL != URL_s) {
        nsConnectionInfo *pConn = (nsConnectionInfo *)URL_s->fe_data;

        printf("+++ Finished loading %s\n", URL_s->address);
        PR_ASSERT(pConn);

        /* Release the ConnectionInfo object held in the URL_Struct. */
        if (pConn) {
            /* 
             * Normally, the stream is closed when the connection has been
             * completed.  However, if the URL exit proc was called directly
             * by NET_GetURL(...), then the stream may still be around...
             */
            if (pConn->pNetStream) {
                pConn->pNetStream->Close();
                pConn->pNetStream->Release();
                pConn->pNetStream = NULL;
            }

            /* Notify the Data Consumer that the Binding has completed... */
            if (pConn->pConsumer) {
                pConn->pConsumer->OnStopBinding();
                pConn->pConsumer->Release();
                pConn->pConsumer = NULL;
            }

            /* Release the nsConnectionInfo object hanging off of the fe_data */
            URL_s->fe_data = NULL;
            pConn->Release();
        }

        /* Delete the URL_Struct... */
        NET_FreeURLStruct(URL_s);
    }
}

