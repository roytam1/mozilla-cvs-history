/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8; c-file-style: "stroustrup" -*-
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
 * Original Author(s):
 *   Robert John Churchill           <rjc@netscape.com>
 *
 * Contributor(s): 
 *   Pierre Phaneuf                  <pp@ludusdesign.com>
 */

#include "nsRDFCID.h"
#include "nsIGenericFactory.h"
#include "nsISearchService.h"

NS_IMETHODIMP	NS_NewLocalSearchService(nsISupports* aOuter, REFNSIID aIID, void** aResult);
NS_IMETHODIMP	NS_NewInternetSearchService(nsISupports* aOuter, REFNSIID aIID, void** aResult);

static nsModuleComponentInfo gSearchComponents[] =
{
    { "Local Search", NS_RDFFINDDATASOURCE_CID,
      NS_LOCALSEARCH_SERVICE_PROGID, NS_NewLocalSearchService,
    },
    { "Local Search", NS_RDFFINDDATASOURCE_CID,
      NS_LOCALSEARCH_DATASOURCE_PROGID, NS_NewLocalSearchService,
    },
    { "Internet Search", NS_RDFSEARCHDATASOURCE_CID,
      NS_INTERNETSEARCH_SERVICE_PROGID, NS_NewInternetSearchService,
    },
    { "Internet Search", NS_RDFSEARCHDATASOURCE_CID,
      NS_INTERNETSEARCH_DATASOURCE_PROGID, NS_NewInternetSearchService,
    },
};

NS_IMPL_NSGETMODULE("nsSearchModule", gSearchComponents)

