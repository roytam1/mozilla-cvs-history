/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is TransforMiiX XSLT Processor.
 *
 * The Initial Developer of the Original Code is
 * Axel Hecht.
 * Portions created by the Initial Developer are Copyright (C) 2001
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Axel Hecht <axel@pike.org>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef __TX_ERROR
#define __TX_ERROR

/*
 * Error value mockup for standalone.
 * See nsError.h for details.
 */

#ifdef TX_EXE

#include "baseutils.h"

typedef PRUint32 nsresult;

#define NS_FAILED(_nsresult) ((_nsresult) & 0x80000000)
#define NS_SUCCEEDED(_nsresult) (!((_nsresult) & 0x80000000))
#define NS_OK                              0
#define NS_ERROR_INVALID_POINTER           ((nsresult) 0x80004003L)
#define NS_ERROR_NULL_POINTER              NS_ERROR_INVALID_POINTER
#define NS_ERROR_UNEXPECTED                ((nsresult) 0x8000ffffL)
#define NS_ERROR_NOT_IMPLEMENTED           ((nsresult) 0x80004001L)
#define NS_ERROR_FAILURE                   ((nsresult) 0x80004005L)
#define NS_ERROR_OUT_OF_MEMORY             ((nsresult) 0x8007000eL)
#define NS_ERROR_ILLEGAL_VALUE             ((nsresult) 0x80070057L)
#define NS_ERROR_INVALID_ARG               NS_ERROR_ILLEGAL_VALUE

#define NS_ENSURE_TRUE(value, result) \
    do {                              \
        if (!(value)) {               \
            return (result);          \
        }                             \
    } while(0)

#define NS_ENSURE_FALSE(value, result) \
    NS_ENSURE_TRUE(!(value), result)

#define NS_ENSURE_SUCCESS(value, result) \
    NS_ENSURE_TRUE(NS_SUCCEEDED(value), result)

#else // TX_EXE

#include "nsError.h"

#endif // TX_EXE

#define NS_ERROR_XPATH_EVAL_FAILED         NS_ERROR_FAILURE
#define NS_ERROR_XPATH_PARSE_FAILED        NS_ERROR_FAILURE
#define NS_ERROR_XPATH_INVALID_ARG         NS_ERROR_INVALID_ARG
#define NS_ERROR_XSLT_INVALID_URL          NS_ERROR_INVALID_ARG

#endif // __TX_ERROR
