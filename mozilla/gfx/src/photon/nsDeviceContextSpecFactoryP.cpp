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

#include "nsDeviceContextSpecFactoryP.h"
#include "nsDeviceContextSpecPh.h"
#include "nsGfxCIID.h"
#include "plstr.h"
#include "nsIServiceManager.h"

#include "nsIPrintOptions.h"

#include <Pt.h>
#include "nsPhGfxLog.h"

nsDeviceContextSpecFactoryPh :: nsDeviceContextSpecFactoryPh()
{
  NS_INIT_REFCNT();
}

nsDeviceContextSpecFactoryPh :: ~nsDeviceContextSpecFactoryPh()
{
}

static NS_DEFINE_IID(kIDeviceContextSpecIID, NS_IDEVICE_CONTEXT_SPEC_IID);
static NS_DEFINE_IID(kDeviceContextSpecCID, NS_DEVICE_CONTEXT_SPEC_CID);
static NS_DEFINE_CID(kPrintOptionsCID, NS_PRINTOPTIONS_CID);
static NS_DEFINE_IID(kDeviceContextSpecFactoryIID, NS_IDEVICE_CONTEXT_SPEC_FACTORY_IID);

NS_IMPL_QUERY_INTERFACE(nsDeviceContextSpecFactoryPh, kDeviceContextSpecFactoryIID)
NS_IMPL_ADDREF(nsDeviceContextSpecFactoryPh)
NS_IMPL_RELEASE(nsDeviceContextSpecFactoryPh)

NS_IMETHODIMP nsDeviceContextSpecFactoryPh :: Init(void)
{
  return NS_OK;
}

//XXX this method needs to do what the API says...

NS_IMETHODIMP nsDeviceContextSpecFactoryPh :: CreateDeviceContextSpec(nsIWidget *aWidget,
																	nsIDeviceContextSpec *&aNewSpec,
																	PRBool aQuiet)
{
	NS_ENSURE_ARG_POINTER(aWidget);

	nsresult  rv = NS_ERROR_FAILURE;
	nsCOMPtr<nsIPrintOptions> printService = 
	         do_GetService(kPrintOptionsCID, &rv);

	nsIDeviceContextSpec  *devSpec = nsnull;

	nsComponentManager::CreateInstance(kDeviceContextSpecCID, nsnull, kIDeviceContextSpecIID, (void **)&devSpec);

	if (devSpec != nsnull)
	{
		PtWidget_t *widget = (PtWidget_t*) aWidget->GetNativeData( NS_NATIVE_WIDGET );
		PtWidget_t *disjoint = PtFindDisjoint( widget );
		if( !PtWidgetIsClass( disjoint, PtWindow ) ) aQuiet = 1; /* for the embedding stuff, the PrintSelection dialog is displayed by the client */

		if (NS_OK == ((nsDeviceContextSpecPh *)devSpec)->Init(aQuiet))
		{
			aNewSpec = devSpec;
			rv = NS_OK;
		}
	}
	return rv;
}
