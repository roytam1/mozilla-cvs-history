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

#include "nscore.h"
#include "nsIFactory.h"
#include "nsIStreamObject.h"
#include "nsIStreamListener.h"
#include "nsIXPFCCanvas.h"

class nsStreamObject : public nsIStreamObject,
                       public nsIStreamListener
{
public:

  /**
   * Constructor and Destructor
   */

  nsStreamObject();
  ~nsStreamObject();

  /**
   * ISupports Interface
   */
  NS_DECL_ISUPPORTS

  /**
   * Initialize Method
   * @result The result of the initialization, NS_OK if no errors
   */
  NS_IMETHOD Init();

  NS_IMETHOD OnStartBinding(nsIURL * aURL, const char *aContentType);
  NS_IMETHOD OnProgress(nsIURL* aURL, PRInt32 aProgress, PRInt32 aProgressMax);
  NS_IMETHOD OnStatus(nsIURL* aURL, const nsString &aMsg) ;
  NS_IMETHOD OnStopBinding(nsIURL * aURL, 
			               PRInt32 aStatus, 
			               const nsString &aMsg);

  NS_IMETHOD GetBindInfo(nsIURL * aURL);
  NS_IMETHOD OnDataAvailable(nsIURL * aURL,
      			             nsIInputStream *aIStream, 
                             PRInt32 aLength);

public:
  nsIParser * mParser;
  nsIURL * mUrl;
  nsIDTD * mDTD;
  nsIContentSink * mSink;
  nsIStreamListener * mStreamListener;
  nsIXPFCCanvas * mParentCanvas;
};


