/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2
-*- 
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape 
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */ 

/**
***  Private CAPI header file.
***  Defines the opaque types.
**/
#ifndef __JULIAN_PRIVATE_CAPI_H
#define __JULIAN_PRIVATE_CAPI_H

#include "nsString.h"
#include "xp_mcom.h"
#include "jdefines.h"
#include "julnstr.h"
#include "nspr.h"
#include "plstr.h"
#include "nsCurlParser.h"

typedef struct 
{
    nsCurlParser* pCurl; 
    char* psUser;         /* i: Calendar store (and ":extra" information )  */
    char* psPassword;     /* i: password for sUser  */
    char* psHost;         /* i: calendar server host (and :port)  */
} PCAPISESSION;

typedef struct
{
    PCAPISESSION *pSession;
    char* psFile;
    nsCurlParser* pCurl; 
} PCAPIHANDLE;

typedef struct
{
    CAPICallback pfnSndCallback;    /* i: Snd iCalendar data    */
    void* pCallerSndDataBuf;        /* i: Data buffer pointer Snd function  */
    void* userDataRcv;              /* i: a user supplied value */
    CAPICallback pfnRcvCallback;    /* i: Rcv iCalendar data  */
    void* userDataSnd;              /* i: a user supplied value */
    long lFlags;                    /* i: bit flags (none at this time; set to 0)  */
} PCAPIStream;

#endif  /* __JULIAN_PRIVATE_CAPI_H */