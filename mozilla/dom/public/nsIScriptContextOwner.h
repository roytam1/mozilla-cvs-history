/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

#ifndef nsIScriptContextOwner_h__
#define nsIScriptContextOwner_h__

#include "nscore.h"
#include "nsISupports.h"
#include "nsIScriptContext.h"

class nsIScriptContext;

#define NS_ISCRIPTCONTEXTOWNER_IID           \
{ /* a94ec640-0bba-11d2-b326-00805f8a3859 */ \
 0xa94ec640, 0x0bba, 0x11d2,                 \
  {0xb3, 0x26, 0x00, 0x80, 0x5f, 0x8a, 0x38, 0x59} }


/**
 * Implemented by any object capable of supplying a nsIScriptContext.
 * The implentor may create the script context on demand and is
 * allowed (though not expected) to throw it away on release.
 */

class nsIScriptContextOwner : public nsISupports {
public:
  NS_DEFINE_STATIC_IID_ACCESSOR(NS_ISCRIPTCONTEXTOWNER_IID)

  /**
   * Returns a script context. The assumption is that the
   * script context has an associated script global object and
   * is ready for script evaluation.
   */
  NS_IMETHOD  GetScriptContext(nsIScriptContext **aContext) = 0;

  /**
   * Returns the script global object
   */
  NS_IMETHOD  GetScriptGlobalObject(nsIScriptGlobalObject **aGlobal) = 0;

  /**
   * Called to indicate that the script context is no longer needed.
   * The caller should <B>not</B> also call the context's Release()
   * method.
   */
  NS_IMETHOD  ReleaseScriptContext(nsIScriptContext *aContext) = 0;

  /**
   * Error notification method. Informs the owner that an error 
   * occurred while a script was being evaluted.
   *
   */
  NS_IMETHOD ReportScriptError(const char* aErrorString,
                               const char* aFileName,
                               PRInt32     aLineNo,
                               const char* aLineBuf) = 0;
};

#endif // nsIScriptContextOwner_h__
