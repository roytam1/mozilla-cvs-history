/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

#ifndef __nsAbSyncDriver_h__
#define __nsAbSyncDriver_h__

#include "nsIAbSyncDriver.h"
#include "nsIMsgStatusFeedback.h"
#include "nsIStringBundle.h"

class nsAbSyncDriver : public nsIAbSyncDriver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIABSYNCDRIVER
  NS_DECL_NSIABSYNCLISTENER

  nsAbSyncDriver();
  virtual ~nsAbSyncDriver();
  /* additional members */
  PRUnichar     *GetString(const PRUnichar *aStringName);

private:

  nsCOMPtr<nsIStringBundle>       mStringBundle;
  PRInt32                         mTransactionID;
  nsCOMPtr<nsIMsgStatusFeedback>  mStatus;
  PRBool                          mCancelled;
};

#endif /* __nsAbSyncDriver_h__ */
