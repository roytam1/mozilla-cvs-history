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

#include "nscore.h"
#include "xp_core.h"
#include "nsCRT.h"
#include "prmem.h"
#include "plstr.h"
#include "nsString.h"
#include "nsVoidArray.h"

/* Right now, plstr.h does not implement strok yet, so we'll go through the string library for this.... */
/* We need to fix this! strtok is not thread-safe on most platforms.
 * we need a better solution for this */
#include <string.h>
#define XP_STRTOK                 	strtok

/* see mozilla/xpcom/public/nsError.h for details */
#define NS_ERROR_MODULE_MAILNEWS 16

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
