/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://wwwt.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#include "nsTextEditFactory.h"
#include "nsITextEditor.h"
#include "nsTextEditor.h"
#include "nsEditor.h"
#include "nsEditorCID.h"
#include "nsIComponentManager.h"


static NS_DEFINE_CID(kTextEditorCID,   NS_TEXTEDITOR_CID);


nsresult
GetTextEditFactory(nsIFactory **aFactory, const nsCID & aClass)
{
  PR_EnterMonitor(GetEditorMonitor());

  nsTextEditFactory *factory = new nsTextEditFactory(aClass);
  if (!factory)
    return NS_ERROR_OUT_OF_MEMORY;
  nsCOMPtr<nsIFactory> pNSIFactory = do_QueryInterface(factory);
  if (!pNSIFactory)
    return NS_ERROR_NO_INTERFACE;

  nsresult result = pNSIFactory->QueryInterface(nsIFactory::GetIID(),
                                                (void **)aFactory);
  PR_ExitMonitor(GetEditorMonitor());
  return result;
}

////////////////////////////////////////////////////////////////////////////
// from nsISupports 

nsresult
nsTextEditFactory::QueryInterface(const nsIID& aIID, void** aInstancePtr) 
{
  if (nsnull == aInstancePtr) {
    NS_NOTREACHED("!nsEditor");
    return NS_ERROR_NULL_POINTER; 
  } 
  if (aIID.Equals(nsIFactory::GetIID()) ||
    aIID.Equals(nsCOMTypeInfo<nsISupports>::GetIID())) {
    *aInstancePtr = (void*) this; 
    AddRef();  
    return NS_OK; 
  }
  return NS_NOINTERFACE; 
}

NS_IMPL_ADDREF(nsTextEditFactory)
NS_IMPL_RELEASE(nsTextEditFactory)


////////////////////////////////////////////////////////////////////////////
// from nsIFactory:

NS_IMETHODIMP
nsTextEditFactory::CreateInstance(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
  *aResult  = nsnull;
  nsISupports *obj = nsnull;
  if (!aResult)
    return NS_ERROR_NULL_POINTER;
  if (aOuter && !aIID.Equals(nsCOMTypeInfo<nsISupports>::GetIID()))
    return NS_NOINTERFACE;   // XXX right error?


  if (mCID.Equals(kTextEditorCID))
  {
    //Need to cast to interface first to avoid "ambiguous conversion..." error
    //  because of multiple nsISupports in the class hierarchy
    obj = (nsISupports *)(nsITextEditor*)new nsTextEditor();
  }
  //more class ids to support. here


  if (obj && NS_FAILED(obj->QueryInterface(aIID, (void**)aResult)) ) 
  {
    delete obj;
    return NS_NOINTERFACE;
  }
  return NS_OK;
}



NS_IMETHODIMP
nsTextEditFactory::LockFactory(PRBool aLock)
{
  return NS_OK;
}



////////////////////////////////////////////////////////////////////////////
// from nsTextEditFactory:

nsTextEditFactory::nsTextEditFactory(const nsCID &aClass)
:mCID(aClass)
{
  NS_INIT_REFCNT();
}

nsTextEditFactory::~nsTextEditFactory()
{
  //nsComponentManager::UnregisterFactory(mCID, (nsIFactory *)this); //we are out of ref counts anyway
}
