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
 * Copyright (C) 1999 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#include "nsDeviceContextSpecFactoryQT.h"
#include "nsDeviceContextSpecQT.h"
#include "nsGfxCIID.h"
#include "plstr.h"
#include "nsRenderingContextQT.h"
#include <qapplication.h>

/** -------------------------------------------------------
 *  Constructor
 *  @update   dc 2/16/98
 */
nsDeviceContextSpecFactoryQT::nsDeviceContextSpecFactoryQT()
{
    PR_LOG(QtGfxLM, 
           PR_LOG_DEBUG, 
           ("nsDeviceContextSpecFactoryQT::nsDeviceContextSpecFactoryQT\n"));
}

/** -------------------------------------------------------
 *  Destructor
 *  @update   dc 2/16/98
 */
nsDeviceContextSpecFactoryQT::~nsDeviceContextSpecFactoryQT()
{
    PR_LOG(QtGfxLM, 
           PR_LOG_DEBUG, 
           ("nsDeviceContextSpecFactoryQT::~nsDeviceContextSpecFactoryQT\n"));
}

static NS_DEFINE_IID(kDeviceContextSpecFactoryIID, 
                     NS_IDEVICE_CONTEXT_SPEC_FACTORY_IID);
static NS_DEFINE_IID(kIDeviceContextSpecIID, NS_IDEVICE_CONTEXT_SPEC_IID);
static NS_DEFINE_IID(kDeviceContextSpecCID, NS_DEVICE_CONTEXT_SPEC_CID);

NS_IMPL_QUERY_INTERFACE(nsDeviceContextSpecFactoryQT, 
                        kDeviceContextSpecFactoryIID)
NS_IMPL_ADDREF(nsDeviceContextSpecFactoryQT)
NS_IMPL_RELEASE(nsDeviceContextSpecFactoryQT)

/** -------------------------------------------------------
 *  Initialize the device context spec factory
 *  @update   dc 2/16/98
 */
NS_IMETHODIMP nsDeviceContextSpecFactoryQT::Init(void)
{
    PR_LOG(QtGfxLM, 
           PR_LOG_DEBUG, 
           ("nsDeviceContextSpecFactoryQT::Init\n"));
    return NS_OK;
}

/** -------------------------------------------------------
 *  Get a device context specification
 *  @update   dc 2/16/98
 */
NS_IMETHODIMP nsDeviceContextSpecFactoryQT::CreateDeviceContextSpec
(
    nsIDeviceContextSpec *aOldSpec,
    nsIDeviceContextSpec *&aNewSpec,
    PRBool aQuiet)
{
    PR_LOG(QtGfxLM, 
           PR_LOG_DEBUG, 
           ("nsDeviceContextSpecFactoryQT::CreateDeviceContextSpec\n"));
    nsresult  					rv = NS_ERROR_FAILURE;
    nsIDeviceContextSpec  *devSpec = nsnull;

    nsComponentManager::CreateInstance(kDeviceContextSpecCID, 
                                       nsnull, 
                                       kIDeviceContextSpecIID, 
                                       (void **)&devSpec);

    if (nsnull != devSpec)
    {
        if (NS_OK == ((nsDeviceContextSpecQT *)devSpec)->Init(aQuiet))
        {
            aNewSpec = devSpec;
            rv = NS_OK;
        }
    }
    return rv;
}
