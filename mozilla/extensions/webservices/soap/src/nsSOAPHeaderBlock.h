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

#ifndef nsSOAPHeaderBlock_h__
#define nsSOAPHeaderBlock_h__

#include "nsString.h"
#include "nsIVariant.h"
#include "nsISOAPHeaderBlock.h"
#include "nsISecurityCheckedComponent.h"
#include "nsIXPCScriptable.h"
#include "nsIJSNativeInitializer.h"
#include "nsISOAPEncoding.h"
#include "nsISchema.h"
#include "nsIDOMElement.h"
#include "nsISOAPAttachments.h"
#include "nsCOMPtr.h"

class nsSOAPHeaderBlock : public nsISOAPHeaderBlock,
                        public nsISecurityCheckedComponent,
                        public nsIXPCScriptable,
                        public nsIJSNativeInitializer
{
public:
  nsSOAPHeaderBlock();
  virtual ~nsSOAPHeaderBlock();

  NS_DECL_ISUPPORTS

  // nsISOAPHeaderBlock
  NS_DECL_NSISOAPHEADERBLOCK

  // nsISecurityCheckedComponent
  NS_DECL_NSISECURITYCHECKEDCOMPONENT

  // nsIXPCScriptable
  NS_DECL_NSIXPCSCRIPTABLE

  // nsIJSNativeInitializer
  NS_IMETHOD Initialize(JSContext *cx, JSObject *obj, 
                        PRUint32 argc, jsval *argv);

protected:
  nsString mNamespaceURI;
  nsString mName;
  nsString mActorURI;
  nsCOMPtr<nsISOAPEncoding> mEncoding;
  nsCOMPtr<nsISchemaType> mSchemaType;
  nsCOMPtr<nsISOAPAttachments> mAttachments;
  nsCOMPtr<nsIDOMElement> mElement;
  nsCOMPtr<nsIVariant> mValue;
  nsresult mStatus;
  PRBool mComputeValue;
};

#endif
