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


#include "net.h"
#include "mktrace.h"
#include "structs.h"
#include "ctxtfunc.h"
#include "xp_list.h"
#include "plstr.h"

#include "nsString.h"
#include "nsIStreamListener.h"
#include "nsINetSupport.h"
#include "nsNetStream.h"

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

static nsIStreamListener *getStreamListener(URL_Struct *URL_s) 
{
    nsIStreamListener *res = NULL;
    
    if (URL_s && URL_s->fe_data) {
        /* 
         * Retrieve the nsConnectionInfo object from the fe_data field
         * of the URL_Struct...
         */
        nsConnectionInfo *pConn = (nsConnectionInfo *)URL_s->fe_data;
        if (pConn) {
            res = pConn->pConsumer;
        }
    }

    NS_IF_ADDREF(res);

    return res;
}

static nsINetSupport *getNetSupport(URL_Struct *URL_s) 
{
    nsIStreamListener *isl = getStreamListener(URL_s);
    if (isl) {
        nsINetSupport *ins;
        isl->QueryInterface(kINetSupportIID, (void **) &ins);
        isl->Release();
        return ins;
    }
    return NULL;
}

void stub_Alert(MWContext *context,
                const char *msg)
{
    nsINetSupport *ins;

    if (nsnull != (ins = getNetSupport(context->modular_data))) {
        nsString str(msg);
        ins->Alert(str);
        ins->Release();
    } else {
        printf("Alert: %s", msg);
    }
}

extern "C" void FE_Alert(MWContext *context, const char *msg)
{
    stub_Alert(context, msg);
}

#if defined (WIN32)
extern "C" char * FE_GetProgramDirectory(char *buffer, int length) {
    printf("XXX: returning c:\\temp\\ for the FE_GetProgramDirectory!\n");
    strcpy(buffer, "c:\\temp\\");
    return buffer;
}
#elif defined (XP_UNIX)
extern "C" void fe_GetProgramDirectory(char *buffer, int length) {
    printf("XXX: returning /tmp for the fe_GetProgramDirectory!\n");
    strcpy(buffer, "/tmp/");
    return buffer;
}
#endif

XP_Bool stub_Confirm(MWContext *context,
                     const char *msg)
{
    nsINetSupport *ins;
    
    if (nsnull != (ins = getNetSupport(context->modular_data))) {
        XP_Bool res;
        nsString str(msg);
        res = ins->Confirm(str);
        ins->Release();
        return res;
    } else {
        printf("Confirm: %s", msg);
    }
    return FALSE;
}

char *stub_Prompt(MWContext *context,
                  const char *msg,
                  const char *def)
{
    nsINetSupport *ins;
    
    if (nsnull != (ins = getNetSupport(context->modular_data))) {
        nsString str(msg);
        nsString defStr(def);
        nsString res;
        if (ins->Prompt(msg, defStr, res)) {
            ins->Release();
            return res.ToNewCString();
        }
        ins->Release();
    } else {
        char buf[256];
        printf("%s\n", msg);
        printf("Prompt: ");
        scanf("%s", buf);
        if (PL_strlen(buf)) {
            return PL_strdup(buf);
        }
    }

    return NULL;
}

PRIVATE XP_Bool
stub_PromptUsernameAndPassword(MWContext *context,
                               const char *msg,
                               char **username,
                               char **password)
{
    nsINetSupport *ins;
    
    if (nsnull != (ins = getNetSupport(context->modular_data))) {
        nsString str(msg);
        nsString userStr;
        nsString pwdStr;
        if (ins->PromptUserAndPassword(msg, userStr, pwdStr)) {
            ins->Release();
            *username = userStr.ToNewCString();
            *password = pwdStr.ToNewCString();
            return TRUE;
        }
        ins->Release();
    } else {
        char buf[256];
        printf("%s\n", msg);
        printf("Username: ");
        scanf("%s", buf);
        *username = PL_strdup(buf);
        printf("Password: ");
        scanf("%s", buf);
        *password = PL_strdup(buf);
        if (**username) {
             return TRUE;
        }
        PR_FREEIF(*username);
        PR_FREEIF(*password);
    }

    return FALSE;
}

char *stub_PromptPassword(MWContext *context,
                          const char *msg)
{
    nsINetSupport *ins;
    
    if (nsnull != (ins = getNetSupport(context->modular_data))) {
        nsString str(msg);
        nsString res;
        if (ins->PromptPassword(msg, res)) {
            ins->Release();
            return res.ToNewCString();
        }
        ins->Release();
    } else {
        char buf[256];
        printf("%s\n", msg);
        printf("Password: ");
        scanf("%s", buf);
        if (PL_strlen(buf)) {
            return PL_strdup(buf);
        }
    }

    return NULL;
}

PRIVATE void stub_GraphProgressInit(MWContext  *context, 
                                    URL_Struct *URL_s, 
                                    int32       content_length)
{
    nsIStreamListener *pListener;

    if (URL_s->load_background)
        return;

    if (nsnull != (pListener = getStreamListener(URL_s))) {
        nsConnectionInfo *pConn = (nsConnectionInfo *) URL_s->fe_data;
        pListener->OnProgress(pConn->pURL, 0, content_length);
        pListener->Release();
    }
}


PRIVATE void stub_GraphProgress(MWContext  *context, 
                                URL_Struct *URL_s, 
                                int32       bytes_received,
                                int32       bytes_since_last_time,
                                int32       content_length)
{
    nsIStreamListener *pListener;

    if (URL_s->load_background)
        return;

    if (nsnull != (pListener = getStreamListener(URL_s))) {
        nsConnectionInfo *pConn = (nsConnectionInfo *) URL_s->fe_data;
        pListener->OnProgress(pConn->pURL, bytes_received, 
                              content_length);
        pListener->Release();
    }
}

PRIVATE void stub_GraphProgressDestroy(MWContext  *context, 
                                       URL_Struct *URL_s, 
                                       int32       content_length,
                                       int32       total_bytes_read)
{
    /*
     * XXX: Currently this function never calls OnProgress(...) because
     *      netlib calls FE_GraphProgressDestroy(...) after closing the
     *      stream...  So, OnStopBinding(...) has already been called and
     *      the nsConnectionInfo->pConsumer has been released and NULLed...
     */
    nsIStreamListener *pListener;

    if (URL_s->load_background)
        return;

    if (nsnull != (pListener = getStreamListener(URL_s))) {
        nsConnectionInfo *pConn = (nsConnectionInfo *) URL_s->fe_data;
        pListener->OnProgress(pConn->pURL, total_bytes_read, 
                              content_length);
    }
}

PRIVATE void stub_Progress(MWContext *context, const char *msg)
{
    nsIStreamListener *pListener;

    if (nsnull != (pListener = getStreamListener(context->modular_data))) {
        nsConnectionInfo *pConn = 
            (nsConnectionInfo *) context->modular_data->fe_data;
        nsAutoString status(msg);
        pListener->OnStatus(pConn->pURL, status);
        pListener->Release();
    } else {
        printf("%s\n", msg);
    }
}

PRIVATE void stub_AllConnectionsComplete(MWContext *context)
{
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
#define stub_FreeBuiltinElement             (FreeBuiltinElement_t)stub_noop
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
#define stub_DisplayBuiltin                 (DisplayBuiltin_t)stub_noop
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
#define stub_Progress                       (Progress_t)stub_Progress
#define stub_Alert                          (Alert_t)stub_Alert
#define stub_SetCallNetlibAllTheTime        (SetCallNetlibAllTheTime_t)stub_noop
#define stub_ClearCallNetlibAllTheTime      (ClearCallNetlibAllTheTime_t)stub_noop
#define stub_GraphProgressInit              (GraphProgressInit_t)stub_GraphProgressInit
#define stub_GraphProgressDestroy           (GraphProgressDestroy_t)stub_GraphProgressDestroy
#define stub_GraphProgress                  (GraphProgress_t)stub_GraphProgress
#define stub_UseFancyFTP                    (UseFancyFTP_t)stub_noop
#define stub_UseFancyNewsgroupListing       (UseFancyNewsgroupListing_t)stub_noop
#define stub_FileSortMethod                 (FileSortMethod_t)stub_noop
#define stub_ShowAllNewsArticles            (ShowAllNewsArticles_t)stub_noop
#define stub_Confirm                        (Confirm_t)stub_Confirm
#define stub_CheckConfirm                   (CheckConfirm_t)stub_noop
#define stub_SelectDialog                   (SelectDialog_t)stub_noop
#define stub_Prompt                         (Prompt_t)stub_Prompt
#define stub_PromptWithCaption              (PromptWithCaption_t)stub_noop
#define stub_PromptUsernameAndPassword      (PromptUsernameAndPassword_t)stub_PromptUsernameAndPassword
#define stub_PromptPassword                 (PromptPassword_t)stub_PromptPassword
#define stub_EnableClicking                 (EnableClicking_t)stub_noop
#define stub_AllConnectionsComplete         (AllConnectionsComplete_t)stub_AllConnectionsComplete
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

XP_List *stub_context_list = NULL;

MWContext *new_stub_context(URL_Struct *URL_s)
{
    static int funcsInitialized = 0;
    MWContext *context;

    if (!funcsInitialized) {
#define MAKE_FE_FUNCS_PREFIX(f) stub_##f
#define MAKE_FE_FUNCS_ASSIGN stub_context_funcs.
#include "mk_cx_fn.h"

        funcsInitialized = 1;

        stub_context_list = XP_ListNew();
    }

    context = (MWContext *)calloc(sizeof(struct MWContext_), 1);

    if (nsnull != context) {
        context->funcs = &stub_context_funcs;
        context->type  = MWContextBrowser;
        context->modular_data = URL_s;

        if (nsnull != stub_context_list) {
            XP_ListAddObjectToEnd(stub_context_list, context);
        }
    }

    return context;
}

void free_stub_context(MWContext *window_id)
{
    TRACEMSG(("Freeing stub context...\n"));

    if (window_id) {
        if (stub_context_list) {
            PRBool result;
        
            result = XP_ListRemoveObject(stub_context_list, window_id);
            PR_ASSERT(PR_TRUE == result);
        }
        free(window_id);
    }
}


extern "C" MWContext * XP_FindContextOfType (MWContext * context, 
                                             MWContextType type)
{
    MWContext *window_id;

    /*
     * Return the context that was passed in if it is the correct type
     */
    if (nsnull != context) {
        if (context->type == type) {
            window_id = context;
        }
    }
    /*
     * Otherwise, the type MUST be a MWBrowserContext, since that is the
     * only type of context that is created...
     * 
     * Return the first stub context, since it is as good as any... :-)
     */
    else if (MWContextBrowser == type) {
        window_id = (MWContext *)XP_ListTopObject(stub_context_list);
    }

    return window_id;
}


/****************************************************************************/
/* End of MWContext Evil!!!                                                 */
/****************************************************************************/

nsConnectionInfo *GetConnectionInfoFromStream(NET_StreamClass *stream)
{
    URL_Struct *URL_s = (URL_Struct *)stream->data_object;

    return (URL_s) ? (nsConnectionInfo *)URL_s->fe_data : NULL;
}


/*
 * Define a NET_StreamClass which pushes its data into an nsIStream
 * and fires off notifications through the nsIStreamListener interface
 */

void stub_complete(NET_StreamClass *stream)
{
    URL_Struct *URL_s = (URL_Struct *)stream->data_object;
    nsConnectionInfo *pConn = GetConnectionInfoFromStream(stream);

    TRACEMSG(("+++ stream complete.\n"));

    /* Close the stream and remove it from the ConnectionInfo... */
    pConn->pNetStream->Close();
    pConn->pNetStream->Release();
    pConn->pNetStream = NULL;


    /* Notify the Data Consumer that the Binding has completed... */
    if (pConn->pConsumer) {
        nsAutoString status;
        pConn->pConsumer->OnStopBinding(pConn->pURL, NS_BINDING_SUCCEEDED, status);
        pConn->pConsumer->Release();
        pConn->pConsumer = NULL;
    }

    /* Release the URL_Struct hanging off of the data_object */
    stream->data_object = NULL;
    NET_DropURLStruct(URL_s);
}

void stub_abort(NET_StreamClass *stream, int status)
{
    URL_Struct *URL_s = (URL_Struct *)stream->data_object;
    nsConnectionInfo *pConn = GetConnectionInfoFromStream(stream);

    TRACEMSG(("+++ stream abort.  Status = %d\n", status));

    /* Close the stream and remove it from the ConnectionInfo... */
    pConn->pNetStream->Close();
    pConn->pNetStream->Release();
    pConn->pNetStream = NULL;

    /* Notify the Data Consumer that the Binding has completed... */
    /* 
     * XXX:  Currently, there is no difference between complete and
     * abort...
     */
    if (pConn->pConsumer) {
        nsAutoString status;

        pConn->pConsumer->OnStopBinding(pConn->pURL, NS_BINDING_ABORTED, status);
        pConn->pConsumer->Release();
        pConn->pConsumer = NULL;
    }

    /* Release the URL_Struct hanging off of the data_object */
    stream->data_object = NULL;
    NET_DropURLStruct(URL_s);
}

int stub_put_block(NET_StreamClass *stream, const char *buffer, int32 length)
{
    PRInt32 bytesWritten;
    nsresult errorCode;
    nsConnectionInfo *pConn = GetConnectionInfoFromStream(stream);

    TRACEMSG(("+++ stream put_block.  Length = %d\n", length));

    /*
     * XXX:  Sometimes put_block(...) will be called without having 
     *       called is_write_ready(...) first.  One case is when a stream
     *       is interrupted...  In this case, Netlib will call put_block(...)
     *       with the string "Transfer Interrupted!"
     */
    errorCode = pConn->pNetStream->Write(buffer, 0, length, &bytesWritten);

    /* Abort the connection... */
    if (NS_BASE_STREAM_EOF == errorCode) {
        return -1;
    }

    if (pConn->pConsumer && (0 < bytesWritten)) {
        errorCode = pConn->pConsumer->OnDataAvailable(pConn->pURL, pConn->pNetStream, bytesWritten);
    }

	/* Abort the connection if an error occurred... */
	if (NS_FAILED(errorCode) || (bytesWritten != length)) {
		return -1;
	}
    return 1;
}

unsigned int stub_is_write_ready(NET_StreamClass *stream)
{
    PRInt32 errorCode;
    unsigned int free_space = 0;
    URL_Struct *URL_s = (URL_Struct *)stream->data_object;
    nsConnectionInfo *pConn = GetConnectionInfoFromStream(stream);

    free_space = (unsigned int)pConn->pNetStream->GetAvailableSpace(&errorCode);

    /*
     * If the InputStream has been closed...  Return 1 byte available so
     * Netlib will call put_block(...) one more time...
     */
    if (NS_BASE_STREAM_EOF == errorCode) {
        free_space = 1;
    }

    TRACEMSG(("+++ stream is_write_ready.  Returning %d\n", free_space));
    return free_space;
}


extern "C" {

/* 
 *Find a converter routine to create a stream and return the stream struct
 */
PUBLIC NET_StreamClass * 
NET_NGLayoutConverter(FO_Present_Types format_out,
                      void *converter_obj,
                      URL_Struct  *URL_s,
                      MWContext   *context)
{
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
             * Retrieve the nsConnectionInfo object from the fe_data field
             * of the URL_Struct...
             */
            pConn = (nsConnectionInfo *)URL_s->fe_data;

            /*
             * If the URL address has been rewritten by netlib then update
             * the cached info in the URL object...
             */
            if ((URL_s->address_modified) && (NULL != pConn->pURL)) {
                pConn->pURL->Set(NET_URLStruct_Address(URL_s));
            }

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

            /* Hang the URL_Struct off of the NET_StreamClass */
            NET_HoldURLStruct(URL_s);
            stream->data_object = URL_s;

            /* Notify the data consumer that Binding is beginning...*/
            /* XXX: check result to terminate connection if necessary */
#ifdef NOISY
            printf("+++ Created a stream for %s\n", URL_s->address);
#endif
            if (pConn->pConsumer) {
                nsresult rv;

                rv = pConn->pConsumer->OnStartBinding(pConn->pURL, URL_s->content_type);
                /*
                 * If OnStartBinding fails, then tear down the NET_StreamClass
                 * and release all references...
                 */
                if (NS_OK != rv) {
                    NET_DropURLStruct(URL_s);
                    free(stream);
                    stream = NULL;

                    /* 
                     * The NET_GetURL(...) exit_routine will clean up the
                     * nsConnectionInfo and associated objects.
                     */
                }
            }
        }
    }

    return stream;
}

}; /* extern "C" */
