/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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


#ifndef __nsMsgRDFDataSource_h
#define __nsMsgRDFDataSource_h

#include "nsCOMPtr.h"
#include "nsIRDFDataSource.h"
#include "nsIRDFService.h"
#include "nsIServiceManager.h"
#include "nsISupportsArray.h"
#include "nsIObserver.h"
#include "nsITransactionManager.h"
#include "nsIMsgWindow.h"
#include "nsIMsgRDFDataSource.h"

class nsMsgRDFDataSource : public nsIRDFDataSource,
                           public nsIObserver,
  						   public nsIMsgRDFDataSource
{
 public:
  nsMsgRDFDataSource();
  virtual ~nsMsgRDFDataSource();
  virtual nsresult Init();
  
  NS_DECL_ISUPPORTS
  NS_DECL_NSIMSGRDFDATASOURCE
  NS_DECL_NSIRDFDATASOURCE
  NS_DECL_NSIOBSERVER

  virtual void Close();

 protected:
	nsIRDFService *getRDFService();
	static PRBool assertEnumFunc(nsISupports *aElement, void *aData);
	static PRBool unassertEnumFunc(nsISupports *aElement, void *aData);
	nsresult  NotifyObservers(nsIRDFResource *subject, nsIRDFResource *property,
								nsIRDFNode *object, PRBool assert);
	nsresult GetTransactionManager(nsISupportsArray *sources, nsITransactionManager **aTransactionManager);

	nsCOMPtr<nsIMsgWindow> mWindow;

	nsCOMPtr<nsISupportsArray> kEmptyArray;
	PRBool m_shuttingDown;
    nsresult  GetIsThreaded(PRBool *threaded);
	nsresult GetViewType(PRUint32 *viewType);

 private:
  nsCOMPtr<nsIRDFService> mRDFService;
  nsCOMPtr<nsISupportsArray> mObservers;

};

#endif
