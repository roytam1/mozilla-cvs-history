/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License.  You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Christopher Blizzard.
 * Portions created by Christopher Blizzard are Copyright (C) 1999
 * Christopher Blizzard.  All Rights Reserved.
 */

#include "nsMotifAppContextService.h"

XtAppContext nsMotifAppContextService::sAppContext = nsnull;
nsMotifAppContextService::nsMotifAppContextService()
{
}

nsMotifAppContextService::~nsMotifAppContextService()
{
}

NS_IMPL_ADDREF(nsMotifAppContextService)
NS_IMPL_RELEASE(nsMotifAppContextService)
NS_IMPL_QUERY_INTERFACE(nsMotifAppContextService, nsCOMTypeInfo<nsIMotifAppContextService>::GetIID())

NS_IMETHODIMP
nsMotifAppContextService::SetAppContext(XtAppContext aAppContext)
{
  NS_ASSERTION(sAppContext == nsnull,"App context can only be set once.");

  sAppContext = aAppContext;

  return NS_OK;
}

NS_IMETHODIMP
nsMotifAppContextService::GetAppContext(XtAppContext * aAppContextOut)
{
  *aAppContextOut = sAppContext;

  return NS_OK;
}
