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
 *
 * Original Author: David W. Hyatt (hyatt@netscape.com)
 *
 * Contributor(s): 
 */

class nsIRDFService;
class nsIRDFResource;
class nsICSSLoader;
class nsISimpleEnumerator;
class nsSupportsHashtable;
class nsIRDFContainer;
class nsIDOMWindow;
class nsIDocument;

#include "nsIRDFDataSource.h"
#include "nsIRDFObserver.h"

class nsChromeUIDataSource : public nsIRDFDataSource, public nsIRDFObserver
{
public:
  NS_DECL_ISUPPORTS

  // nsIRDFDataSource methods
  NS_DECL_NSIRDFDATASOURCE

  // nsIRDFObserver methods
  NS_DECL_NSIRDFOBSERVER

  // nsChromeUIDataSource methods:
  nsChromeUIDataSource(nsIRDFDataSource* aComposite);
  virtual ~nsChromeUIDataSource();

protected:
  nsCOMPtr<nsIRDFDataSource>  mComposite;
  nsCOMPtr<nsISupportsArray>	mObservers;
  nsIRDFService* mRDFService;
};

nsresult NS_NewChromeUIDataSource(nsIRDFDataSource* aComposite, nsIRDFDataSource** aResult);
