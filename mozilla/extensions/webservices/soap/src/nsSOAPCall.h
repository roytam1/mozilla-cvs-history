/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef nsSOAPCall_h__
#define nsSOAPCall_h__

#include "nsISOAPCall.h"
#include "nsISecurityCheckedComponent.h"
#include "nsISOAPTransportListener.h"
#include "nsIDOMElement.h"
#include "nsIDOMDocument.h"
#include "nsString.h"
#include "nsISupportsArray.h"
#include "nsISOAPResponseListener.h"
#include "nsISOAPTransport.h"
#include "nsCOMPtr.h"

class nsSOAPCall : public nsISOAPCall,
                   public nsISecurityCheckedComponent,
                   public nsISOAPTransportListener
{
public:
  nsSOAPCall();
  virtual ~nsSOAPCall();

  NS_DECL_ISUPPORTS

  // nsISOAPCall
  NS_DECL_NSISOAPCALL

  // nsISecurityCheckedComponent
  NS_DECL_NSISECURITYCHECKEDCOMPONENT

  // nsISOAPTransportListener
  NS_DECL_NSISOAPTRANSPORTLISTENER

protected:

  nsresult EnsureDocumentAllocated();
  PRBool HasBodyEntry();
  nsresult CreateBodyEntry(PRBool aNewParameters);
  nsresult CreateParameterElements();
  nsresult ClearParameterElements();
  nsresult GetTransport(nsISOAPTransport** aTransport);
  nsresult GetScriptListener(nsISupports* aObject,
                             nsISOAPResponseListener** aListener);

  nsCOMPtr<nsIDOMDocument> mEnvelopeDocument;
  nsCOMPtr<nsIDOMElement> mEnvelopeElement;
  nsCOMPtr<nsIDOMElement> mHeaderElement;
  nsCOMPtr<nsIDOMElement> mBodyElement;
  nsCString mDestinationURI;
  nsCString mActionURI;
  nsCString mTargetObjectURI;
  nsString mMethodName;
  nsCOMPtr<nsISupportsArray> mParameters;
  nsCOMPtr<nsISOAPResponseListener> mListener;
  PRUint32 mStatus;
};

#endif
