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
 * Copyright (C) 1999 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#include "nsDeviceContextSpecFactoryG.h"
#include "nsDeviceContextSpecG.h"
#include "nsGfxCIID.h"
#include "plstr.h"

/** -------------------------------------------------------
 *  Constructor
 *  @update   dc 2/16/98
 */
nsDeviceContextSpecFactoryGTK :: nsDeviceContextSpecFactoryGTK()
{
}

/** -------------------------------------------------------
 *  Destructor
 *  @update   dc 2/16/98
 */
nsDeviceContextSpecFactoryGTK :: ~nsDeviceContextSpecFactoryGTK()
{
}

static NS_DEFINE_IID(kDeviceContextSpecFactoryIID, NS_IDEVICE_CONTEXT_SPEC_FACTORY_IID);
static NS_DEFINE_IID(kIDeviceContextSpecIID, NS_IDEVICE_CONTEXT_SPEC_IID);
static NS_DEFINE_IID(kDeviceContextSpecCID, NS_DEVICE_CONTEXT_SPEC_CID);

NS_IMPL_QUERY_INTERFACE(nsDeviceContextSpecFactoryGTK, kDeviceContextSpecFactoryIID)
NS_IMPL_ADDREF(nsDeviceContextSpecFactoryGTK)
NS_IMPL_RELEASE(nsDeviceContextSpecFactoryGTK)

/** -------------------------------------------------------
 *  Initialize the device context spec factory
 *  @update   dc 2/16/98
 */
NS_IMETHODIMP nsDeviceContextSpecFactoryGTK :: Init(void)
{
  return NS_OK;
}

/** -------------------------------------------------------
 *  Get a device context specification
 *  @update   dc 2/16/98
 */
NS_IMETHODIMP nsDeviceContextSpecFactoryGTK :: CreateDeviceContextSpec(nsIDeviceContextSpec *aOldSpec,
                                                                       nsIDeviceContextSpec *&aNewSpec,
                                                                       PRBool aQuiet)
{
nsresult  						rv = NS_ERROR_FAILURE;
nsIDeviceContextSpec  *devSpec = nsnull;

	nsComponentManager::CreateInstance(kDeviceContextSpecCID, nsnull, kIDeviceContextSpecIID, (void **)&devSpec);

	if (nsnull != devSpec){
	  if (NS_OK == ((nsDeviceContextSpecGTK *)devSpec)->Init(aQuiet)){
	    aNewSpec = devSpec;
	    rv = NS_OK;
	  }
	}
	return rv;
}
