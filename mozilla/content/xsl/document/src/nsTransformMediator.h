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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#ifndef nsTransformMediator_h__
#define nsTransformMediator_h__

#include "nsITransformMediator.h"
#include "nsIDocumentTransformer.h"
#include "nsIDOMElement.h"
#include "nsIDOMDocument.h"
#include "nsIObserver.h"

class nsTransformMediator : public nsITransformMediator {
public:
  nsTransformMediator();
  virtual ~nsTransformMediator();

  nsresult Init(const nsString& aMimeType);

  // nsISupports
  NS_DECL_ISUPPORTS

  // nsITransformMediator
  NS_IMETHOD SetEnabled(PRBool aValue);
  NS_IMETHOD SetSourceContentModel(nsIDOMElement* aSource);  
  NS_IMETHOD SetStyleSheetContentModel(nsIDOMElement* aStyle);
  NS_IMETHOD SetCurrentDocument(nsIDOMDocument* aDoc);
  NS_IMETHOD SetTransformObserver(nsIObserver* aObserver);
  
protected:
  void TryToTransform();

  PRBool mEnabled;
  nsIDocumentTransformer* mTransformer; // Strong reference
  nsIDOMElement* mSourceDOM;            // Weak reference
  nsIDOMElement* mStyleDOM;             // Weak reference
  nsIDOMDocument* mCurrentDoc;          // Weak reference
  nsIObserver* mObserver;               // Weak reference
};

#endif // nsTransformMediator_h__
