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


#include "net.h"
#include "structs.h"
#include "ctxtfunc.h"

#include "nsIStreamNotification.h"
#include "net_strm.h"

/****************************************************************************/
/* Beginning  of MWContext Evil!!!                                          */
/* -------------------------------                                          */
/*                                                                          */
/* Define a dummy MWContext where all of the upcalls are stubbed out.       */
/*                                                                          */
/****************************************************************************/

PRIVATE int
stub_noop(int x, ...)
{
#ifdef XP_MAC
#pragma unused (x)
#endif
/*  DebugBreak(); */
    return 0;
}

#define MAKE_FE_TYPES_PREFIX(func)	func##_t
#define MAKE_FE_FUNCS_TYPES
#include "mk_cx_fn.h"
#undef MAKE_FE_FUNCS_TYPES

#define stub_CreateNewDocWindow             (CreateNewDocWindow_t)stub_noop
#define stub_LayoutNewDocument              (LayoutNewDocument_t)stub_noop
#define stub_SetDocTitle                    (SetDocTitle_t)stub_noop
#define stub_FinishedLayout                 (FinishedLayout_t)stub_noop
#define stub_TranslateISOText               (TranslateISOText_t)stub_noop
#define stub_GetTextInfo                    (GetTextInfo_t)stub_noop
#define stub_MeasureText                    (MeasureText_t)stub_noop
#define stub_GetEmbedSize                   (GetEmbedSize_t)stub_noop
#define stub_GetJavaAppSize                 (GetJavaAppSize_t)stub_noop
#define stub_GetFormElementInfo             (GetFormElementInfo_t)stub_noop
#define stub_GetFormElementValue            (GetFormElementValue_t)stub_noop
#define stub_ResetFormElement               (ResetFormElement_t)stub_noop
#define stub_SetFormElementToggle           (SetFormElementToggle_t)stub_noop
#define stub_FreeFormElement                (FreeFormElement_t)stub_noop
#define stub_FreeImageElement               (FreeImageElement_t)stub_noop
#define stub_FreeEmbedElement               (FreeEmbedElement_t)stub_noop
#define stub_FreeJavaAppElement             (FreeJavaAppElement_t)stub_noop
#define stub_CreateEmbedWindow              (CreateEmbedWindow_t)stub_noop
#define stub_SaveEmbedWindow                (SaveEmbedWindow_t)stub_noop
#define stub_RestoreEmbedWindow             (RestoreEmbedWindow_t)stub_noop
#define stub_DestroyEmbedWindow             (DestroyEmbedWindow_t)stub_noop
#define stub_HideJavaAppElement             (HideJavaAppElement_t)stub_noop
#define stub_FreeEdgeElement                (FreeEdgeElement_t)stub_noop
#define stub_FormTextIsSubmit               (FormTextIsSubmit_t)stub_noop
#define stub_DisplaySubtext                 (DisplaySubtext_t)stub_noop
#define stub_DisplayText                    (DisplayText_t)stub_noop
#define stub_DisplayImage                   (DisplayImage_t)stub_noop
#define stub_DisplayEmbed                   (DisplayEmbed_t)stub_noop
#define stub_DisplayJavaApp                 (DisplayJavaApp_t)stub_noop
#define stub_DisplaySubImage                (DisplaySubImage_t)stub_noop
#define stub_DisplayEdge                    (DisplayEdge_t)stub_noop
#define stub_DisplayTable                   (DisplayTable_t)stub_noop
#define stub_DisplayCell                    (DisplayCell_t)stub_noop
#define stub_InvalidateEntireTableOrCell    (InvalidateEntireTableOrCell_t)stub_noop
#define stub_DisplayAddRowOrColBorder       (DisplayAddRowOrColBorder_t)stub_noop
#define stub_DisplaySubDoc                  (DisplaySubDoc_t)stub_noop
#define stub_DisplayLineFeed                (DisplayLineFeed_t)stub_noop
#define stub_DisplayHR                      (DisplayHR_t)stub_noop
#define stub_DisplayBullet                  (DisplayBullet_t)stub_noop
#define stub_DisplayFormElement             (DisplayFormElement_t)stub_noop
#define stub_DisplayBorder                  (DisplayBorder_t)stub_noop
#define stub_UpdateEnableStates             (UpdateEnableStates_t)stub_noop
#define stub_DisplayFeedback                (DisplayFeedback_t)stub_noop
#define stub_ClearView                      (ClearView_t)stub_noop
#define stub_SetDocDimension                (SetDocDimension_t)stub_noop
#define stub_SetDocPosition                 (SetDocPosition_t)stub_noop
#define stub_GetDocPosition                 (GetDocPosition_t)stub_noop
#define stub_BeginPreSection                (BeginPreSection_t)stub_noop
#define stub_EndPreSection                  (EndPreSection_t)stub_noop
#define stub_SetProgressBarPercent          (SetProgressBarPercent_t)stub_noop
#define stub_SetBackgroundColor             (SetBackgroundColor_t)stub_noop
#define stub_Progress                       (Progress_t)stub_noop
#define stub_Alert                          (Alert_t)stub_noop
#define stub_SetCallNetlibAllTheTime        (SetCallNetlibAllTheTime_t)stub_noop
#define stub_ClearCallNetlibAllTheTime      (ClearCallNetlibAllTheTime_t)stub_noop
#define stub_GraphProgressInit              (GraphProgressInit_t)stub_noop
#define stub_GraphProgressDestroy           (GraphProgressDestroy_t)stub_noop
#define stub_GraphProgress                  (GraphProgress_t)stub_noop
#define stub_UseFancyFTP                    (UseFancyFTP_t)stub_noop
#define stub_UseFancyNewsgroupListing       (UseFancyNewsgroupListing_t)stub_noop
#define stub_FileSortMethod                 (FileSortMethod_t)stub_noop
#define stub_ShowAllNewsArticles            (ShowAllNewsArticles_t)stub_noop
#define stub_Confirm                        (Confirm_t)stub_noop
#define stub_Prompt                         (Prompt_t)stub_noop
#define stub_PromptWithCaption              (PromptWithCaption_t)stub_noop
#define stub_PromptUsernameAndPassword      (PromptUsernameAndPassword_t)stub_noop
#define stub_PromptPassword                 (PromptPassword_t)stub_noop
#define stub_EnableClicking                 (EnableClicking_t)stub_noop
#define stub_AllConnectionsComplete         (AllConnectionsComplete_t)stub_noop
#define stub_ImageSize                      (ImageSize_t)stub_noop
#define stub_ImageData                      (ImageData_t)stub_noop
#define stub_ImageIcon                      (ImageIcon_t)stub_noop
#define stub_ImageOnScreen                  (ImageOnScreen_t)stub_noop
#define stub_SetColormap                    (SetColormap_t)stub_noop
#ifdef LAYERS
#define stub_EraseBackground                (EraseBackground_t)stub_noop
#define stub_SetDrawable                    (SetDrawable_t)stub_noop
#define stub_GetTextFrame                   (GetTextFrame_t)stub_noop
#define stub_SetClipRegion                  (SetClipRegion_t)stub_noop
#define stub_SetOrigin	                    (SetOrigin_t)stub_noop
#define stub_GetOrigin	                    (GetOrigin_t)stub_noop
#define stub_GetTextFrame                   (GetTextFrame_t)stub_noop
#endif

#define stub_GetDefaultBackgroundColor      (GetDefaultBackgroundColor_t)stub_noop
#define stub_LoadFontResource               (LoadFontResource_t)stub_noop

#define stub_DrawJavaApp                    (DrawJavaApp_t)stub_noop
#define stub_HandleClippingView             (HandleClippingView_t)stub_noop

/* Just reuse the same set of context functions: */
ContextFuncs stub_context_funcs;

MWContext *new_stub_context()
{
    static int funcsInitialized = 0;
    MWContext *context;

    if (!funcsInitialized) {
#define MAKE_FE_FUNCS_PREFIX(f) stub_##f
#define MAKE_FE_FUNCS_ASSIGN stub_context_funcs.
#include "mk_cx_fn.h"

        funcsInitialized = 1;
    }

    context = (MWContext *)calloc(sizeof(struct MWContext_), 1);

    context->funcs = &stub_context_funcs;
    context->type  = MWContextBrowser;

    return context;
}

void free_stub_context(MWContext *window_id)
{
    TRACEMSG(("Freeing stub context...\n"));
    free(window_id);
}


/****************************************************************************/
/* End of MWContext Evil!!!                                                 */
/****************************************************************************/

#if 0 

typedef struct NetlibEvent_OnDataAvailable {
    PREvent              ce;
    stream_connection_t *pStreamInfo;
} NetlibEvent_OnDataAvailable;

PR_STATIC_CALLBACK(void)
bam_HandleEvent_OnDataAvailable(NetlibEvent_OnDataAvailable* e)
{
    pStreamInfo->pConsumer->OnDataAvailable(pStreamInfo->pNetStream);
}

PRIVATE void
bam_OnDataAvailable(stream_connection_t *stream_info)
{
    NetlibEvent_OnDataAvailable* event;

    event = PR_NEW(NetlibEvent_OnDataAvailable);
    if (event == NULL) goto done;
    PL_InitEvent(&event->ce, stream_info,
                 (PLHandleEventProc)bam_HandleEvent_OnDataAvailable, 
                 (PLDestroyEventProc)bam_DestroyEvent_GenericEvent);
    event->pStreamInfo = stream_info;
    PL_PostEvent(netlib_event_queue, &event->ce);
    
  done:
}


#endif /* 0 */


/*
 * Define a NET_StreamClass which pushes its data into an nsIStream
 * and fires off notifications through the nsIStreamNotification interface
 */

void stub_complete(NET_StreamClass *stream)
{
    nsConnectionInfo *pConn = (nsConnectionInfo *)stream->data_object;

    TRACEMSG(("+++ stream complete.\n"));

    pConn->pNetStream->Close();

    /* Notify the Data Consumer that the Binding has completed... */
    if (pConn->pConsumer) {
        pConn->pConsumer->OnStopBinding();
        pConn->pConsumer->Release();
        pConn->pConsumer = NULL;
    }

    /* Free the nsConnectionInfo object hanging off of the data_object */
    pConn->pNetStream->Release();
    pConn->pNetStream = NULL;
    pConn->Release();
    stream->data_object = NULL;
}

void stub_abort(NET_StreamClass *stream, int status)
{
    nsConnectionInfo *pConn = (nsConnectionInfo *)stream->data_object;

    TRACEMSG(("+++ stream abort.  Status = %d\n", status));

    pConn->pNetStream->Close();

    /* Notify the Data Consumer that the Binding has completed... */
    /* 
     * XXX:  Currently, there is no difference between complete and
     * abort...
     */
    if (pConn->pConsumer) {
        pConn->pConsumer->OnStopBinding();
        pConn->pConsumer->Release();
        pConn->pConsumer = NULL;
    }

    /* Free the nsConnectionInfo object hanging off of the data_object */
    pConn->pNetStream->Release();
    pConn->pNetStream = NULL;
    pConn->Release();
    stream->data_object = NULL;
}

int stub_put_block(NET_StreamClass *stream, const char *buffer, int32 length)
{
    PRInt32 bytesWritten;
    nsConnectionInfo *pConn = (nsConnectionInfo *)stream->data_object;

    TRACEMSG(("+++ stream put_block.  Length = %d\n", length));

    bytesWritten = pConn->pNetStream->Write(buffer, length);

    /* XXX: check return value to abort connection if necessary */
    if (pConn->pConsumer) {
        pConn->pConsumer->OnDataAvailable(pConn->pNetStream);
    }

    PR_ASSERT(bytesWritten == length);
    return (bytesWritten == length);
}

unsigned int stub_is_write_ready(NET_StreamClass *stream)
{
    unsigned int free_space = 0;
    nsConnectionInfo *pConn = (nsConnectionInfo *)stream->data_object;

    free_space = (unsigned int)pConn->pNetStream->GetAvailableSpace();

    TRACEMSG(("+++ stream is_write_ready.  Returning %d\n", free_space));
    return free_space;
}


extern "C" {

/* 
 *Find a converter routine to create a stream and return the stream struct
 */
PUBLIC NET_StreamClass * 
NET_StreamBuilder  (FO_Present_Types format_out,
                    URL_Struct  *URL_s,
                    MWContext   *context)
{
//  MOZ_FUNCTION_STUB;
    NET_StreamClass *stream = NULL;
    PRBool bSuccess = PR_TRUE;

    /* 
     * Only create a stream if an nsConnectionInfo object is 
     * available from the fe_data...
     */
    if (NULL != URL_s->fe_data) {
        stream = (NET_StreamClass *)calloc(sizeof(NET_StreamClass), 1);

        if (NULL != stream) {
            nsConnectionInfo *pConn;

            /*
             * Initialize the NET_StreamClass instance...
             */
            stream->name           = "Stub Stream";
            stream->window_id      = context;

            stream->complete       = stub_complete;
            stream->abort          = stub_abort;
            stream->put_block      = stub_put_block;
            stream->is_write_ready = stub_is_write_ready;

            /* 
             * Create a stream_connection struct to hold the information
             * about the connection progress...
             * 
             * Remember to AddRef() the objects because we are storing 
             * references to them...
             */
            pConn = (nsConnectionInfo *)URL_s->fe_data;

            /* 
             * Create an Async stream unless a blocking stream is already
             * available in the ConnectionInfo...
             */
            if (NULL == pConn->pNetStream) {
                pConn->pNetStream = new nsAsyncStream(8192);
                if (NULL == pConn->pNetStream) {
                    free(stream);
                    return NULL;
                }
                pConn->pNetStream->AddRef();
            }

            /* Hang the stream_connection off of the NET_StreamClass */
            stream->data_object = pConn;
            pConn->AddRef();

            /* Notify the data consumer that Binding is beginning...*/
            /* XXX: check result to terminate connection if necessary */
            printf("+++ Created a stream for %s\n", URL_s->address);
            if (pConn->pConsumer) {
                pConn->pConsumer->OnStartBinding();
            }
        }
    }

    return stream;
}

}; /* extern "C" */
