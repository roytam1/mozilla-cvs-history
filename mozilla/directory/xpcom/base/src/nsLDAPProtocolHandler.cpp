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
 * The Original Code is the mozilla.org LDAP XPCOM component.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are 
 * Copyright (C) 2000 Netscape Communications Corporation.  All
 * Rights Reserved.
 * 
 * Contributor(s): Dan Mosedale <dmose@mozilla.org>
 *		   Brian Ryner <bryner@uiuc.edu>
 * 
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 or later (the
 * "GPL"), in which case the provisions of the GPL are applicable 
 * instead of those above.  If you wish to allow use of your 
 * version of this file only under the terms of the GPL and not to
 * allow others to use your version of this file under the MPL,
 * indicate your decision by deleting the provisions above and
 * replace them with the notice and other provisions required by
 * the GPL.  If you do not delete the provisions above, a recipient
 * may use your version of this file under either the MPL or the
 * GPL.
 */

#include "nsLDAPProtocolHandler.h"
#include "nsCRT.h"
#include "nsIComponentManager.h"
#include "nsIURI.h"
#include "nsLDAPChannel.h"

static NS_DEFINE_CID(kSimpleURICID, NS_SIMPLEURI_CID);

// QueryInterface, AddRef, and Release
//
NS_IMPL_ISUPPORTS1(nsLDAPProtocolHandler, nsIProtocolHandler)

nsLDAPProtocolHandler::nsLDAPProtocolHandler()
{
  NS_INIT_ISUPPORTS();
  /* member initializers and constructor code */
}

nsLDAPProtocolHandler::~nsLDAPProtocolHandler()
{
  /* destructor code */
}

// nsIProtocolHandler methods

// getter method for scheme attr
//
NS_IMETHODIMP
nsLDAPProtocolHandler::GetScheme(char **result)
{
  *result = nsCRT::strdup("ldap");
  if ( *result == nsnull ) 
    return NS_ERROR_OUT_OF_MEMORY;
  else
    return NS_OK;
}

// getter method for defaultPort attribute
//
NS_IMETHODIMP
nsLDAPProtocolHandler::GetDefaultPort(PRInt32 *result)
{
  *result = (PRInt32)389;
  return NS_OK;
}

// construct an appropriate URI
// code borrowed from nsFingerHandler.cpp
//
NS_IMETHODIMP
nsLDAPProtocolHandler::NewURI(const char *aSpec, nsIURI *aBaseURI,
			      nsIURI **result) {
    nsresult rv;

    // no concept of a relative ldap url
    NS_ASSERTION(!aBaseURI, "base url passed into LDAP protocol handler");

    nsIURI* uri;
    rv = nsComponentManager::CreateInstance(kSimpleURICID, nsnull,
                                            NS_GET_IID(nsIURI),
                                            (void**)&uri);
    if (NS_FAILED(rv)) return rv;

    // XXX - possibly should implement using our own nsILDAPURI which 
    // validates the URI on SetSpec().  alternatively, we could call 
    // ldap_url_parse and friends here.
    //
    rv = uri->SetSpec((char*)aSpec);
    if (NS_FAILED(rv)) {
        NS_RELEASE(uri);
        return rv;
    }

    *result = uri;
    return rv;
}

// code borrowed from nsFingerHandler.cpp
//
NS_IMETHODIMP
nsLDAPProtocolHandler::NewChannel(nsIURI* uri, 
				  nsIChannel* *result)
{
  nsresult rv;
    
  nsLDAPChannel* channel;
  rv = nsLDAPChannel::Create(nsnull, NS_GET_IID(nsIChannel),
			     (void**)&channel);
  if (NS_FAILED(rv)) return rv;

  rv = channel->Init(uri);
  if (NS_FAILED(rv)) {
    NS_RELEASE(channel);
    return rv;
  }

  *result = channel;
  return NS_OK;
}
