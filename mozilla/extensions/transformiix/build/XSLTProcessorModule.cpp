/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is TransforMiiX XSLT processor.
 * 
 * The Initial Developer of the Original Code is The MITRE Corporation.
 * Portions created by MITRE are Copyright (C) 1999 The MITRE Corporation.
 *
 * Portions created by Peter Van der Beken are Copyright (C) 2000
 * Peter Van der Beken. All Rights Reserved.
 *
 * Contributor(s):
 * Peter Van der Beken, peter.vanderbeken@pandora.be
 *    -- original author.
 *
 */

#include "nsIGenericFactory.h"
#include "nsICategoryManager.h"
#include "nsIScriptNameSpaceManager.h"
#include "prprf.h"

#include "XSLTProcessor.h"
#include "XPathProcessor.h"
#include "nsSyncLoader.h"
#include "nsIScriptNameSpaceManager.h"

// Factory Constructor
NS_GENERIC_FACTORY_CONSTRUCTOR(XSLTProcessor)
NS_GENERIC_FACTORY_CONSTRUCTOR(XPathProcessor)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsSyncLoader)

static NS_METHOD 
RegisterTransformiix(nsIComponentManager *aCompMgr,
		     nsIFile *aPath,
		     const char *registryLocation,
		     const char *componentType,
                     const nsModuleComponentInfo *info)
{
  nsresult rv = NS_OK;

  nsCOMPtr<nsICategoryManager> catman =
    do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);

  if (NS_FAILED(rv))
    return rv;

  nsXPIDLCString previous;
  rv = catman->AddCategoryEntry(JAVASCRIPT_GLOBAL_CONSTRUCTOR_CATEGORY,
				"XSLTProcessor",
                                TRANSFORMIIX_XSLT_PROCESSOR_CONTRACTID,
                                PR_TRUE, PR_TRUE, getter_Copies(previous));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = catman->AddCategoryEntry(JAVASCRIPT_GLOBAL_CONSTRUCTOR_CATEGORY,
				"XPathProcessor",
                                TRANSFORMIIX_XPATH_PROCESSOR_CONTRACTID,
                                PR_TRUE, PR_TRUE, getter_Copies(previous));

  return rv;
}

// Component Table
static nsModuleComponentInfo components[] = {
    { "Transformiix XSLT Processor",
      TRANSFORMIIX_XSLT_PROCESSOR_CID,
      TRANSFORMIIX_XSLT_PROCESSOR_CONTRACTID,
      XSLTProcessorConstructor,
      RegisterTransformiix },
    { "Transformiix XPath Processor",
      TRANSFORMIIX_XPATH_PROCESSOR_CID,
      TRANSFORMIIX_XPATH_PROCESSOR_CONTRACTID,
      XPathProcessorConstructor },
    { "Transformiix Synchronous Loader",
      TRANSFORMIIX_SYNCLOADER_CID,
      TRANSFORMIIX_SYNCLOADER_CONTRACTID,
      nsSyncLoaderConstructor }
};

NS_IMPL_NSGETMODULE("TransformiixModule", components)
