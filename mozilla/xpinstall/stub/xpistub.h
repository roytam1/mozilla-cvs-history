/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License.  You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla Communicator client code, 
 * released March 31, 1998. 
 *
 * The Initial Developer of the Original Code is Netscape Communications 
 * Corporation.  Portions created by Netscape are 
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 *
 * Contributors:
 *     Daniel Veditz <dveditz@netscape.com>
 */


#include "nsError.h"
#include "prtypes.h"

#ifdef XP_MAC
#include <Files.h>
#endif

PR_BEGIN_EXTERN_C

/** pfnXPIStart  -- script start callback
 *  
 *  When an install script gets to StartInstall() this function
 *  will be called to tell the observer the pretty-name of the
 *  install package. You are not guaranteed this will be called
 *  for all scripts--there might be a fatal error before it gets
 *  to StartInstall(), either in the script itself or in the
 *  engine trying to set up for it.
 */
typedef void     (*pfnXPIStart)   (const char* UIName);

/** pfnXPIProgress  -- individual install item callback
 *
 *  This callback will be called twice for each installed item,
 *  First when it is scheduled (val and max will both be 0) and
 *  then during the finalize step.
 *
 *  This function must return NS_OK unless it wants to stop
 *  the script execution (for example, to implement a user-cancel
 *  feature). To stop the script return a failing nsresult value.
 */
typedef nsresult (*pfnXPIProgress)(const char* msg, PRInt32 val, PRInt32 max);

/** pfnXPIFinal  -- script end callback
 *
 *  This function will be called when the script calls either
 *  AbortInstall() or FinalizeInstall() and will return the
 *  last error code.
 */
typedef void     (*pfnXPIFinal)   (PRInt32 finalStatus);


/** XPI_Init
 *
 *  call XPI_Init() to initialize XPCOM and the XPInstall
 *  engine, and to pass in your callback functions
 *
 *  @returns    XPCOM status code indicating success or failure
 */
PR_EXTERN(nsresult) XPI_Init(  pfnXPIStart    startCB, 
                               pfnXPIProgress progressCB,
                               pfnXPIFinal    finalCB     );

/** XPI_Install
 *
 *  Install a XPI package from a local file
 *
 *  @param file     Native filename of XPI archive
 *  @param args     Install.arguments, if any
 *  @param flags    the old SmartUpdate trigger flags. This may go away
 */
PR_EXTERN(nsresult) XPI_Install(
#ifndef XP_MAC
                                const char* file, 
#else
                                const FSSpec& file, 
#endif
                                const char* args, 
                                long flags         );

/** XPI_Exit
 * 
 *  call when done to shut down the XPInstall and XPCOM engines
 *  and free allocated memory
 */
PR_EXTERN(void) XPI_Exit();

PR_END_EXTERN_C

