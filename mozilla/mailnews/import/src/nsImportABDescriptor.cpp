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
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *   Pierre Phaneuf <pp@ludusdesign.com>
 */


#include "nscore.h"
#include "nsImportABDescriptor.h"

////////////////////////////////////////////////////////////////////////



NS_METHOD nsImportABDescriptor::Create( nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
  if (aOuter)
    return NS_ERROR_NO_AGGREGATION;

  nsImportABDescriptor *it = new nsImportABDescriptor();
  if (it == nsnull)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF( it);
  nsresult rv = it->QueryInterface( aIID, aResult);
  NS_RELEASE( it);
  return rv;
}

NS_IMPL_ISUPPORTS(nsImportABDescriptor, NS_GET_IID(nsIImportABDescriptor));

nsImportABDescriptor::nsImportABDescriptor() 
{ 
	NS_INIT_ISUPPORTS(); 
	m_ref = 0;
	m_id = 0;
	m_size = 0;
	m_import = PR_TRUE;
	m_pFileSpec = nsnull;
	NS_NewFileSpec( &m_pFileSpec);
}
