/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

/* Include files we are going to want available to all files....these files include
   NSPR, memory, and string header files among others */

#ifndef msgCore_h__
#define msgCore_h__

#include "nscore.h"
#include "xp_core.h"
#include "nsCRT.h"
#include "prmem.h"
#include "plstr.h"
#include "prprf.h"

#include "nsString.h"
#include "nsVoidArray.h"

#include "nsCRT.h"
#include "nsEscape.h"
#include "nsFileSpec.h"

#include "nsTime.h"

class nsIMessage;
class nsIMsgFolder;

// include common interfaces such as the service manager and the repository....
#include "nsIServiceManager.h"
#include "nsIComponentManager.h"

/* NS_ERROR_MODULE_MAILNEWS is defined in mozilla/xpcom/public/nsError.h */

/*
 * NS_ERROR macros - use these macros to generate error constants
 * to be used by XPCOM interfaces and possibly other useful things
 * do not use these macros in your code - declare error macros for
 * each specific error you need.
 *
 * for example:
 * #define NS_MSG_ERROR_NO_SUCH_FOLDER NS_MSG_GENERATE_FAILURE(4)
 * 
 */

/* use these routines to generate error values */
#define NS_MSG_GENERATE_RESULT(severity, value) \
NS_ERROR_GENERATE(severity, NS_ERROR_MODULE_MAILNEWS, value)

#define NS_MSG_GENERATE_SUCCESS(value) \
NS_ERROR_GENERATE_SUCCESS(NS_ERROR_MODULE_MAILNEWS, value)

#define NS_MSG_GENERATE_FAILURE(value) \
NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_MAILNEWS, value)

/* these are shortcuts to generate simple errors with a zero value */
#define NS_MSG_SUCCESS NS_MSG_GENERATE_SUCCESS(0)
#define NS_MSG_FAILURE NS_MSG_GENERATE_FAILURE(0)

// mscott I should look into moving this into raptor base so everyone can use it...
#define IS_SPACE(VAL)                \
    (((((intn)(VAL)) & 0x7f) == ((intn)(VAL))) && isspace((intn)(VAL)) )

#define IS_DIGIT(i) ((((unsigned int) (i)) > 0x7f) ? (int) 0 : isdigit(i))

#if defined(XP_WIN) || defined(XP_OS2)
#define IS_ALPHA(VAL)  (isascii((int)(VAL)) && isalpha((int)(VAL)))
#else
#define IS_ALPHA(VAL) ((((unsigned int) (VAL)) > 0x7f) ? FALSE : isalpha((int)(VAL)))
#endif


/* for retrieving information out of messenger nsresults */

#define NS_IS_MSG_ERROR(err) \
 (NS_ERROR_GET_MODULE(err) == NS_ERROR_MODULE_MAILNEWS)

#define NS_MSG_SUCCEEDED(err) \
 (NS_IS_MSG_ERROR(err) && NS_SUCCEEDED(err))

#define NS_MSG_FAILED(err) \
 (NS_IS_MSG_ERROR(err) && NS_FAILED(err))

/* is this where we define our errors? Obviously, there has to be a central
	place so we don't use the same error codes.
*/
#define NS_MSG_ERROR_FOLDER_SUMMARY_OUT_OF_DATE NS_MSG_GENERATE_FAILURE(5)
#define NS_MSG_ERROR_FOLDER_SUMMARY_MISSING NS_MSG_GENERATE_FAILURE(6)
#define NS_MSG_ERROR_FOLDER_MISSING NS_MSG_GENERATE_FAILURE(7)

#define NS_MSG_MESSAGE_NOT_FOUND NS_MSG_GENERATE_FAILURE(8)
#define NS_MSG_NOT_A_MAIL_FOLDER NS_MSG_GENERATE_FAILURE(9)

#define NS_MSG_FOLDER_BUSY NS_MSG_GENERATE_FAILURE(10)

#define NS_MSG_COULD_NOT_CREATE_DIRECTORY NS_MSG_GENERATE_FAILURE(11)
#define NS_MSG_CANT_CREATE_FOLDER NS_MSG_GENERATE_FAILURE(12)

#define NS_MSG_FILTER_PARSE_ERROR NS_MSG_GENERATE_FAILURE(13)

#define NS_MSG_FOLDER_UNREADABLE NS_MSG_GENERATE_FAILURE(14)

#define NS_MSG_ERROR_WRITING_MAIL_FOLDER NS_MSG_GENERATE_FAILURE(15)

/* ducarroz: error codes for message compose are defined into compose\src\nsMsgComposeStringBundle.h.
             Message compose use the same error code space than other mailnews modules. To avoid any
             conflict, I reserve values between 12500 and 12999 for it.
*/
#define NS_MSGCOMP_ERROR_BEGIN	12500
#define NS_MSGCOMP_ERROR_END	12999

#define MSG_LINEBREAK NS_LINEBREAK
#define MSG_LINEBREAK_LEN NS_LINEBREAK_LEN

/* mscott - i'm only turning  NS_MSG_BASE on for windows so
   define it as empty for the other platforms. */

#ifdef XP_WIN

#ifdef _IMPL_NS_MSG_BASE
#define NS_MSG_BASE NS_EXPORT
#else
#define NS_MSG_BASE NS_IMPORT
#endif

#else
#define NS_MSG_BASE
#endif

////////////////////////////////////////////////////////////////////////////////
// Utilities 

// mscott: one wouldn't normally have to add the NS_MSG_BASE prefix here 
// except this function is implemented in base\util.
nsresult NS_MSG_BASE
nsGetMailFolderSeparator(nsString& result);

////////////////////////////////////////////////////////////////////////////////

#endif
