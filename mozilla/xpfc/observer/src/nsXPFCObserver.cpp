/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
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

#include "nsXPFCObserver.h"
#include "nsxpfcCIID.h"
#include "nsIXPFCSubject.h"
#include "nsIXPFCCommand.h"

static NS_DEFINE_IID(kISupportsIID,  NS_ISUPPORTS_IID);
static NS_DEFINE_IID(kXPFCObserverIID, NS_IXPFC_OBSERVER_IID);

nsXPFCObserver :: nsXPFCObserver()
{
  NS_INIT_REFCNT();
}

nsXPFCObserver :: ~nsXPFCObserver()  
{
}

NS_IMPL_ADDREF(nsXPFCObserver)
NS_IMPL_RELEASE(nsXPFCObserver)
NS_IMPL_QUERY_INTERFACE(nsXPFCObserver, kXPFCObserverIID)

nsresult nsXPFCObserver::Init()
{
  return NS_OK;
}

nsresult nsXPFCObserver::Update(nsIXPFCSubject * aSubject, nsIXPFCCommand * aCommand)
{
  return NS_OK;
}
