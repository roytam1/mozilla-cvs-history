/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
#include "softupdt.h"
#define NEW_FE_CONTEXT_FUNCS
#include "zig.h"
#include "net.h"
#include "libevent.h"
#include "prefapi.h"
#include "prprf.h"
#include "mkutils.h"
#include "fe_proto.h"
#include "prthread.h"
#include "xpgetstr.h"
#include "prmon.h"

extern int MK_OUT_OF_MEMORY;
#define MOCHA_CONTEXT_PREFIX "autoinstall:"

/* error codes */
#define su_ErrInvalidArgs -1
#define su_ErrUnknownInstaller -2
#define su_ErrInternalError -3
#define su_ErrBadScript -4
#define su_JarError -5

/* xp_string defines */
extern int SU_NOT_A_JAR_FILE;
extern int SU_SECURITY_CHECK;
extern int SU_INSTALL_FILE_HEADER;
extern int SU_INSTALL_FILE_MISSING;

/* structs */

/* su_DownloadStream
 * keeps track of all SU specific data needed for the stream
 */ 
typedef struct su_DownloadStream_struct	{
	XP_File			fFile;
	char *			fJarFile;	/* xpURL location of the downloaded file */
	URL_Struct *	fURL;
	MWContext *		fContext;
	SoftUpdateCompletionFunction fCompletion;
	void *			fCompletionClosure;
    int32           fFlags;     /* download flags */
} su_DownloadStream;

/* su_URLFeData
 * passes data between the trigger and the stream
 */
typedef struct su_URLFeData_struct {
	SoftUpdateCompletionFunction fCompletion;
	void * fCompletionClosure;
    int32 fFlags;    /* download flags */
} su_URLFeData;

static char * EncodeSoftUpJSArgs(const char * fileName, XP_Bool silent, XP_Bool force);

/* Stream callbacks */
int su_HandleProcess (NET_StreamClass *stream, const char *buffer, int32 buffLen);
void su_HandleComplete (NET_StreamClass *stream);
void su_HandleAbort (NET_StreamClass *stream, int reason);
unsigned int su_HandleWriteReady (NET_StreamClass *stream);

/* Completion routine for stream handler. Deletes su_DownloadStream */
void su_CompleteSoftwareUpdate(MWContext * context,  
								SoftUpdateCompletionFunction f,
								void * completionClosure,
								int result,
								su_DownloadStream* realStream);

void su_NetExitProc(URL_Struct* url, int result, MWContext * context);
void su_HandleCompleteJavaScript (su_DownloadStream* realStream);


/* Completion routine for SU_StartSoftwareUpdate */

void su_CompleteSoftwareUpdate(MWContext * context, 
								SoftUpdateCompletionFunction f,
								void * completionClosure,
								int result,
								su_DownloadStream* realStream)
{
	/* Notify the trigger */
	if ( f != NULL )
		f(result, completionClosure);

	/* Clean up */
	if (realStream)
	{
		if ( realStream->fJarFile )
		{
			result = XP_FileRemove( realStream->fJarFile, xpURL );
			XP_FREE( realStream->fJarFile );
		}
		XP_FREE( realStream);
	}
}

/* a struct to hold the arguments */
struct su_startCallback_
{
	char * url;
	char * name;
	SoftUpdateCompletionFunction f;
	void * completionClosure;
    int32 flags;
	MWContext * context;
} ;

typedef struct su_startCallback_ su_startCallback;

XP_Bool            QInsert( su_startCallback * Item );
su_startCallback * QGetItem(void);
void               SU_InitMonitor(void);
void               SU_DestroyMonitor(void);

/*
 * This struct represents one entry in a queue setup to hold download 
 * requests if there are more than one.
 */
typedef struct su_QItem
{
	struct su_QItem   * next;
	struct su_QItem   * prev;
	su_startCallback  * QItem;
} su_QItem;


XP_Bool     DnLoadInProgress = FALSE;
PRMonitor * su_monitor = NULL;

void SU_InitMonitor(void)
{
	su_monitor = PR_NewMonitor();
	XP_ASSERT( su_monitor != NULL );
}


void SU_DestroyMonitor(void)
{
	if ( su_monitor != NULL ) 
	{
		PR_DestroyMonitor(su_monitor);
		su_monitor = NULL;
	}
}

/*
 * Queue maintanence is done here 
 */
su_QItem  * Qhead = NULL;
su_QItem  * Qtail = NULL;

XP_Bool QInsert( su_startCallback * Item )
{
	su_QItem *p = XP_ALLOC( sizeof (su_QItem));

	if (p == NULL)
		return FALSE;

	p->QItem = Item;
	p->next = Qhead;
	p->prev = NULL;
	if (Qhead != NULL)
		Qhead->prev = p;
	Qhead = p;
	if (Qtail == NULL) /* First Item inserted in Q? */
		Qtail = p;

	return TRUE;
}

su_startCallback *QGetItem(void)
{
	su_QItem *Qtemp = Qtail;

	if (Qtemp != NULL)
	{
		su_startCallback *p = NULL;
	
		Qtail = Qtemp->prev;
	
		if (Qtail == NULL) /* Last Item deleted from Q? */
			Qhead = NULL;
		else
			Qtail->next = NULL;
	
		p = Qtemp->QItem;
		XP_FREE(Qtemp);
		return p;
	}
	return NULL;
}

/* 
 * timer callback to start the network download of a JAR file
 */
PRIVATE void
su_FE_timer_callback( void * data)
{
	su_startCallback * c;
	URL_Struct * urlS = NULL;
	su_URLFeData * fe_data = NULL;
	XP_Bool  errFound = TRUE;

	c = (su_startCallback*)data;

	if (c->context == NULL)
	{
		su_CompleteSoftwareUpdate(c->context, c->f, c->completionClosure, su_ErrInternalError, NULL);
		goto done;
	}

 	if (c->url == NULL)
	{
		su_CompleteSoftwareUpdate(c->context, c->f, c->completionClosure, su_ErrInvalidArgs, NULL);
		goto done;
	}
    
    urlS = NET_CreateURLStruct(c->url, NET_DONT_RELOAD);
	if (urlS == NULL)
    {
		su_CompleteSoftwareUpdate(c->context, c->f, c->completionClosure, MK_OUT_OF_MEMORY, NULL);
		goto done;
	}

    /* fe_data holds the arguments that need to be passed
     * to our download stream
     */
	fe_data = XP_ALLOC(sizeof(su_URLFeData));

	if (fe_data == NULL)
	{
		su_CompleteSoftwareUpdate(c->context, c->f, c->completionClosure, MK_OUT_OF_MEMORY, NULL);
		NET_FreeURLStruct(urlS);
		goto done;
	}

	fe_data->fCompletion = c->f;
	fe_data->fCompletionClosure = c->completionClosure;
    fe_data->fFlags = c->flags;

	urlS->fe_data = fe_data;
	urlS->must_cache = TRUE;	/* This is needed for partial caching */

	errFound = FALSE;
	NET_GetURL(urlS, FO_CACHE_AND_SOFTUPDATE, c->context, su_NetExitProc);

done:
	if (errFound) 
	{
		/* pops next item from queue or sets download flag to false */
                su_NetExitProc(NULL, 0, NULL);
	}
	if (c->url)
		XP_FREE( c->url);
	if (c->name)
		XP_FREE( c->name);
	XP_FREE( c );
}

/* NET_GetURL exit procedure */
void su_NetExitProc(URL_Struct* url, int result, MWContext * context)
{
	su_startCallback * c;

	PR_EnterMonitor(su_monitor);
	if (c = QGetItem())
	{
		FE_SetTimeout( su_FE_timer_callback, c, 1 );
	}
	else
	{
		DnLoadInProgress = FALSE;
	}
	PR_ExitMonitor(su_monitor);
}

#ifdef XP_MAC
#pragma export on
#endif

/* SU_StartSoftwareUpdate
 * Main public interface to software update
 */
PUBLIC XP_Bool 
SU_StartSoftwareUpdate(MWContext * context, 
						const char * url, 
						const char * name, 
						SoftUpdateCompletionFunction f,
						void * completionClosure,
                        int32 flags)
{
	URL_Struct * urlS = NULL;
	su_URLFeData * fe_data = NULL;
	XP_Bool enabled;

	/* Better safe than sorry */
	PREF_GetBoolPref( AUTOUPDATE_ENABLE_PREF, &enabled);
	if (!enabled)
		return FALSE;

	/* if we do not have a context, create one */

	if ( context == NULL )
	{
		return FALSE;
	}

	/* Need to process this on a timer because netlib is not reentrant */
	{
		su_startCallback * varHolder;
		varHolder = XP_ALLOC( sizeof (su_startCallback));
		if (varHolder == NULL)
		{
			su_CompleteSoftwareUpdate(context, f, completionClosure, MK_OUT_OF_MEMORY, NULL);
			return FALSE;
		}
		varHolder->url = url ? XP_STRDUP( url ) : NULL;
		varHolder->name = name ? XP_STRDUP( name ) : NULL;
		varHolder->f = f;
		varHolder->context = context;
		varHolder->completionClosure = completionClosure;
		varHolder->flags = flags;

		PR_EnterMonitor(su_monitor);
		if ( DnLoadInProgress )
		{
			if (!QInsert( varHolder ))
			{       /* cleanup */
				su_CompleteSoftwareUpdate(context, f, completionClosure, MK_OUT_OF_MEMORY, NULL);
				if (varHolder->url)
					XP_FREE( varHolder->url);
				if (varHolder->name)
					XP_FREE( varHolder->name);
				XP_FREE( varHolder );
				return FALSE;
			}
		}
		else
		{
			DnLoadInProgress = TRUE;
			FE_SetTimeout( su_FE_timer_callback, varHolder, 1 );
		}
		PR_ExitMonitor(su_monitor);
		return TRUE;
	}
}

/* New stream callback */
/* creates the stream, and a opens up a temporary file */
NET_StreamClass * SU_NewStream (int format_out, void * registration,
								URL_Struct * request, MWContext *context)
{
	su_DownloadStream * streamData = NULL;
	su_URLFeData * fe_data = NULL;
	NET_StreamClass * stream = NULL;
	SoftUpdateCompletionFunction completion = NULL;
	void * completionClosure = NULL;
    int32 flags = 0;
	short result = 0;
	XP_Bool	isJar;

	/* Initialize the stream data by data passed in the URL*/
	fe_data = (su_URLFeData *) request->fe_data;

	if ( fe_data != NULL )
	{
		completion = fe_data->fCompletion;
		completionClosure = fe_data->fCompletionClosure;
        flags = fe_data->fFlags;
    }

	/* Make sure that we are loading a Java archive 
	 Strictly, we should accept only APPLICATION_JAVAARCHIVE,\
	but I know that many servers will be misconfigured 
	so we'll try to deal with text/plain, and octet-stream
	So, the logic is:
	Anything but HTML is OK. Netlib will only trigger us
	with the correct MIME type, and when we are using triggers
	this is the right thing to do. HTML is usually an error message
	from the server.
	*/
	if (request->content_type)
	{
		if ( XP_STRCMP( APPLICATION_JAVAARCHIVE, request->content_type) == 0) /* Exact match */
			isJar = TRUE;
		else if ( XP_STRCMP( TEXT_HTML, request->content_type) == 0)
			isJar = FALSE;
		else
			isJar = TRUE;
	}
	else
		isJar = TRUE;	/* Assume we have JAR if no content type */

	/* If we got the wrong MIME type... */
	if (isJar == FALSE)
	{
		if (context)
			FE_Alert(context, XP_GetString(SU_NOT_A_JAR_FILE));
		goto fail;
	}
	
	/* Create all the structs */	
	streamData = XP_CALLOC(sizeof(su_DownloadStream), 1);
	
	if (streamData == NULL)
	{
		result = MK_OUT_OF_MEMORY;
		goto fail;
	}

	stream = NET_NewStream (NULL, 
			su_HandleProcess,
			su_HandleComplete, 
			su_HandleAbort, 
			su_HandleWriteReady, 
			streamData,
			context);

	if (stream == NULL)
	{
		result = MK_OUT_OF_MEMORY;
		goto fail;
	}

	streamData->fURL = request;
	streamData->fContext = context;
	streamData->fCompletion = completion;
	streamData->fCompletionClosure = completionClosure;
    streamData->fFlags = flags;

	if (request->fe_data)
	{
		XP_FREE( request->fe_data);
		request->fe_data = NULL;
	}
	/* Here we jump through few hoops to get the file name in xpURL kind */
	streamData->fJarFile = WH_TempName( xpURL, NULL );

	if (streamData->fJarFile == NULL)
	{
		result = su_ErrInternalError;
		goto fail;
	}
	streamData->fFile = XP_FileOpen( streamData->fJarFile, xpURL, XP_FILE_WRITE_BIN );
	
	if ( streamData->fFile == 0)
	{
		result = su_ErrInternalError;
		goto fail;
	}
/* return the stream */
	return stream;

fail:
	if (stream != NULL)
		XP_FREE( stream );
	su_CompleteSoftwareUpdate( context, completion, completionClosure, result, streamData );
	return NULL;
}

#ifdef XP_MAC
#pragma export reset
#endif

/* su_HandleProcess
 * stream method, writes to disk
 */
int su_HandleProcess (NET_StreamClass *stream, const char *buffer, int32 buffLen)
{
	void *streamData=stream->data_object;
	return XP_FileWrite( buffer, buffLen, ((su_DownloadStream*)streamData)->fFile );
}

/* su_HandleAbort
 * Clean up
 */
void su_HandleAbort (NET_StreamClass *stream, int reason)
{
	void *streamData=stream->data_object;
	su_DownloadStream* realStream = (su_DownloadStream*)stream->data_object;
	
	/* Close the files */
	if (realStream->fFile)
		XP_FileClose(realStream->fFile);
	/* Report the result */
	su_CompleteSoftwareUpdate(realStream->fContext, 
					realStream->fCompletion, realStream->fCompletionClosure, reason, realStream);
}

unsigned int su_HandleWriteReady (NET_StreamClass *stream)
{
	return USHRT_MAX;	/* Returning -1 causes errors when loading local file */
}

/* su_HandleComplete
 * Clean up
 */
void su_HandleComplete (NET_StreamClass *stream)
{

	su_DownloadStream* realStream = (su_DownloadStream*)stream->data_object;

	if (realStream->fFile)
		XP_FileClose(realStream->fFile);

	su_HandleCompleteJavaScript( realStream );
}

/* This is called when Mocha is done with evaluation */
static void
su_mocha_eval_exit_fn(void * data, char * str, size_t len, char * wysiwyg_url,
		 char * base_href, Bool valid)
{
	short result = 0;
	su_DownloadStream* realStream = (su_DownloadStream*) data;
	MWContext *context = realStream->fContext;
	
	if (valid == FALSE)
		result = su_ErrBadScript;

	if (str)
		XP_FREE( str );

	su_CompleteSoftwareUpdate( context, 
					realStream->fCompletion, realStream->fCompletionClosure, 0, realStream );

	FE_DestroyWindow(context);
}
/* su_ReadFileIntoBuffer
 * given a file name, reads it into buffer
 * returns an error code
 */
static short su_ReadFileIntoBuffer(char * fileName, void ** buffer, unsigned long * bufferSize)
{
	XP_File file;
	XP_StatStruct st;
	short result = 0;

	if ( XP_Stat( fileName, &st,  xpURL ) != 0 )
	{
		result = su_ErrInternalError;
		goto fail;
	}
	*bufferSize = st.st_size;

	*buffer = XP_ALLOC( st.st_size );
	if (*buffer == NULL)
	{
		result = MK_OUT_OF_MEMORY;
		goto fail;
	}
	file = XP_FileOpen( fileName, xpURL, XP_FILE_READ_BIN );
	if ( file == 0 )
	{
		result = su_ErrInternalError;
		goto fail;
	}
	if ( XP_FileRead( *buffer, *bufferSize, file ) != *bufferSize )
	{
		result = su_ErrInternalError;
		XP_FileClose( file );
		goto fail;
	}
	XP_FileClose( file );
	return result;

fail:
	if (*buffer != NULL)
		XP_FREE( * buffer);
	*buffer = NULL;
	return result;
}

/* Encodes the args in the format
 * MOCHA_CONTEXT_PREFIX<File>CR<silent>CR<force>
 * Booleans are encoded as T or F.
 * DecodeSoftUpJSArgs is in lm_softup.c
 */
static char *
EncodeSoftUpJSArgs(const char * fileName, XP_Bool force, XP_Bool silent )
{
    char * s;
    int32 length;
    if (fileName == NULL)
        return NULL;

    length = XP_STRLEN(fileName) + 
            XP_STRLEN(MOCHA_CONTEXT_PREFIX) + 5;  /* 2 booleans and a CR */
    s = XP_ALLOC(length);
    if (s != NULL)
    {
        s[0] = 0;
        XP_STRCAT(s, MOCHA_CONTEXT_PREFIX);
        XP_STRCAT(s, fileName);
        s[length - 5] = CR;
        s[length - 4] = silent ? 'T' : 'F';
        s[length - 3] = CR;
        s[length - 2] = force ? 'T' : 'F';
        s[length - 1] = 0;
    }
    return s;
}

/* su_HandleCompleteJavaScript
 * Reads in the mocha script out of the Jar file
 * Executes the script
 */
void su_HandleCompleteJavaScript (su_DownloadStream* realStream)
{
	short result = 0;
	void * buffer = NULL;
	unsigned long bufferSize;
	char * installerJarName = NULL;
	char * installerFileNameURL = NULL;
    char * codebase = NULL;
    int32  urlLen;
	unsigned long fileNameLength;
	ZIG * jarData = NULL;
	char s[255];
    char * jsScope = NULL;
	ETEvalStuff * stuff = NULL;

	/* Initialize the JAR file */

	jarData = SOB_new();
	if ( jarData == NULL)
	{
		result = MK_OUT_OF_MEMORY;
		goto fail;
	}

	SOB_set_context (jarData, realStream->fContext);

	result = SOB_pass_archive( ZIG_F_GUESS, 
						realStream->fJarFile, 
						realStream->fURL->address, 
						jarData);

	if (result < 0)
	{
		char *errMsg = SOB_get_error(result);
		PR_snprintf(s, 255, XP_GetString(SU_SECURITY_CHECK), (errMsg?errMsg:"") );
		FE_Alert(realStream->fContext, s);

		result = su_JarError;
		goto fail;
	}

	/* Get the installer file name */
	result = SOB_get_metainfo( jarData, NULL, INSTALLER_HEADER, (void**)&installerJarName, &fileNameLength);

	if (result < 0)
	{
		FE_Alert(realStream->fContext, XP_GetString(SU_INSTALL_FILE_HEADER));

		result = su_JarError;
		goto fail;
	}
	
	installerJarName[fileNameLength] = 0;	/* Terminate the string */

	/* Need a temporary file name that is the xpURL form */
	installerFileNameURL = WH_TempName( xpURL, NULL );

	if ( installerFileNameURL == NULL)
	{
		result = su_JarError;
		goto fail;
	}
	
	/* Extract the script out */

	result = SOB_verified_extract( jarData, installerJarName, installerFileNameURL);

	if (result < 0)
	{
		PR_snprintf(s, 255, XP_GetString(SU_INSTALL_FILE_MISSING), installerJarName);
		FE_Alert(realStream->fContext, s);
		result = su_JarError;
		goto fail;
	}
	
	/* Read the file in */
	result = su_ReadFileIntoBuffer( installerFileNameURL, &buffer, &bufferSize);
	XP_FileRemove( installerFileNameURL, xpURL);

	if (result < 0)
	{
		result = su_JarError;
		goto fail;
	}

	/* Temporary hack to pass the zig data */
	/* close the archive */
	
	SOB_destroy( jarData);
	jarData = NULL;

	{
		Chrome chrome;
		MWContext * context;
		JSPrincipals *principals;

	/* For security reasons, installer JavaScript has to execute inside a
	special context. This context is created by ET_EvaluateBuffer as a JS object
	of type SoftUpdate. the arguments to the object are passed in the string,
    jsScope
	*/
		jsScope = EncodeSoftUpJSArgs(realStream->fJarFile, 
                                     ((realStream->fFlags & FORCE_INSTALL) != 0),
                                     ((realStream->fFlags & SILENT_INSTALL) != 0));
        if (jsScope == NULL)
            goto fail;

		XP_BZERO(&chrome, sizeof(Chrome));
		chrome.location_is_chrome = TRUE;
		chrome.l_hint = -3000;
		chrome.t_hint = -3000;
		chrome.type = MWContextDialog;
		context = FE_MakeNewWindow(realStream->fContext, NULL, NULL, &chrome);
		if (context == NULL)
			goto fail;

        urlLen = XP_STRLEN(realStream->fURL->address);
        codebase = XP_ALLOC( urlLen + XP_STRLEN(installerJarName) + 2 );
        if ( codebase == NULL)
            goto fail;
        XP_STRCPY( codebase, realStream->fURL->address );
        codebase[urlLen] = '/';
        XP_STRCPY( codebase+urlLen+1, installerJarName );

        principals = LM_NewJSPrincipals(realStream->fURL, installerJarName, codebase );
		if (principals == NULL) {
			FE_DestroyWindow(context);
			goto fail;
		}

		/* Execute the mocha script, result will be reported in the callback */
		realStream->fContext = context;

		stuff = (ETEvalStuff *) XP_NEW_ZAP(ETEvalStuff);
		if (!stuff) {
		    FE_DestroyWindow(context);
		    goto fail;
		}

		stuff->len = bufferSize;
		stuff->line_no = 1;
		stuff->scope_to = jsScope;
		stuff->want_result = JS_TRUE;
		stuff->data = realStream;
		stuff->version = JSVERSION_DEFAULT;
		stuff->principals = principals;

		ET_EvaluateScript(context, buffer, stuff, su_mocha_eval_exit_fn);

	}
	goto done;

fail:
	if (jarData != NULL)
		SOB_destroy( jarData);
	su_CompleteSoftwareUpdate( realStream->fContext, 
					realStream->fCompletion, realStream->fCompletionClosure, result, realStream );
	/* drop through */
done:
	XP_FREEIF(jsScope);
        XP_FREEIF(codebase);
	if ( installerFileNameURL )
		XP_FREE( installerFileNameURL );

	/* Should we purge stuff from the disk cache here? */
}

/* XXX: Move JavaGetBoolPref to lj_init.c.
 * Delete IsJavaSecurityEnabled and IsJavaSecurityDefaultTo30Enabled functions
 * XXX: Cache all security preferences while we are running on mozilla
 *      thread. Hack for 4.0 
 */
#define MAX_PREF 20

struct {
  char *pref_name;
  XP_Bool value;
} cachePref[MAX_PREF];

static int free_idx=0;
static XP_Bool locked = FALSE;

static void AddPrefToCache(char *pref_name, XP_Bool value) 
{
  if (!pref_name)
      return;

  if (free_idx >= MAX_PREF) {
      XP_ASSERT(FALSE); /* Implement dynamic growth of preferences */
      return;
  }

  cachePref[free_idx].pref_name = XP_STRDUP(pref_name);
  cachePref[free_idx].value = value;
  free_idx++;
}

static XP_Bool GetPreference(char *pref_name, XP_Bool *pref_value) 
{
  int idx = 0;
  *pref_value = FALSE;

  if (!pref_name) 
      return FALSE;

  for (; idx < free_idx; idx++) {
    if (XP_STRCMP(cachePref[idx].pref_name, pref_name) == 0) {
        *pref_value = cachePref[idx].value;
        locked = TRUE;
        return TRUE;
    }
  }

  if (locked) {
    XP_ASSERT(FALSE); /* Implement dynamic growth of preferences */
    return FALSE;
  }
      
  if (PREF_GetBoolPref(pref_name, pref_value) >=0) {
      AddPrefToCache(pref_name, *pref_value);
      return TRUE;
  }

  return FALSE;
}

#ifdef XP_MAC
#pragma export on
#endif

int PR_CALLBACK JavaGetBoolPref(char * pref_name) 
{
	XP_Bool pref;
        int ret_val;
        GetPreference(pref_name, &pref);
        ret_val = pref;
	return ret_val;
}

int PR_CALLBACK IsJavaSecurityEnabled() 
{
	XP_Bool pref;
        int ret_val;
	GetPreference("signed.applets.codebase_principal_support", &pref);
        ret_val = !pref;
	return ret_val;
}

int PR_CALLBACK IsJavaSecurityDefaultTo30Enabled() 
{
	return JavaGetBoolPref("signed.applets.local_classes_have_30_powers");
}

#ifdef XP_MAC
#pragma export reset
#endif

