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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 * Original Author: David W. Hyatt (hyatt@netscape.com)
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
#include "nsCOMPtr.h"
#include "nsIBrowserBoxObject.h"
#include "nsBoxObject.h"
#include "nsIPresShell.h"
#include "nsIFrame.h"
#include "nsIDocShell.h"
#include "nsIDocument.h"
#include "nsIScriptGlobalObject.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsXPIDLString.h"

class nsBrowserBoxObject : public nsIBrowserBoxObject, public nsBoxObject
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIBROWSERBOXOBJECT

  nsBrowserBoxObject();
  virtual ~nsBrowserBoxObject();
  
protected:
  nsString* mSrcURL;
};

/* Implementation file */
NS_IMPL_ADDREF(nsBrowserBoxObject)
NS_IMPL_RELEASE(nsBrowserBoxObject)

NS_IMETHODIMP 
nsBrowserBoxObject::QueryInterface(REFNSIID iid, void** aResult)
{
  if (!aResult)
    return NS_ERROR_NULL_POINTER;
  
  if (iid.Equals(NS_GET_IID(nsIBrowserBoxObject))) {
    *aResult = (nsIBrowserBoxObject*)this;
    NS_ADDREF(this);
    return NS_OK;
  }

  return nsBoxObject::QueryInterface(iid, aResult);
}
  
nsBrowserBoxObject::nsBrowserBoxObject()
:mSrcURL(nsnull)
{
  NS_INIT_ISUPPORTS();
}

nsBrowserBoxObject::~nsBrowserBoxObject()
{
  /* destructor code */
  delete mSrcURL;
}

/* void openBrowser (in boolean openFlag); */
NS_IMETHODIMP nsBrowserBoxObject::GetDocShell(nsIDocShell** aResult)
{
  *aResult = nsnull;
  if (!mPresShell)
    return NS_OK;

  nsCOMPtr<nsIDocument> doc, sub_doc;
  mPresShell->GetDocument(getter_AddRefs(doc));

  doc->GetSubDocumentFor(mContent, getter_AddRefs(sub_doc));

  if (!sub_doc) {
    return NS_OK;
  }

  nsCOMPtr<nsIScriptGlobalObject> sgo;
  sub_doc->GetScriptGlobalObject(getter_AddRefs(sgo));

  nsCOMPtr<nsIDocShell> doc_shell(do_GetInterface(sgo));

  *aResult = doc_shell;
  NS_IF_ADDREF(*aResult);

  return NS_OK;
}

// Creation Routine ///////////////////////////////////////////////////////////////////////

nsresult
NS_NewBrowserBoxObject(nsIBoxObject** aResult)
{
  *aResult = new nsBrowserBoxObject;
  if (!*aResult)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(*aResult);
  return NS_OK;
}

