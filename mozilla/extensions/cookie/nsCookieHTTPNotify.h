/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

#ifndef nsCookieHTTPNotify_h___
#define nsCookieHTTPNotify_h___

#include "nsIHttpNotify.h"
#include "nsCOMPtr.h"
#include "nsIAtom.h"

// {6BC1F522-1F45-11d3-8AD4-00105A1B8860}
#define NS_COOKIEHTTPNOTIFY_CID \
{ 0x6bc1f522, 0x1f45, 0x11d3, { 0x8a, 0xd4, 0x0, 0x10, 0x5a, 0x1b, 0x88, 0x60 } }


class nsCookieHTTPNotify : public nsIHTTPNotify {
public:

  // nsISupports
  NS_DECL_ISUPPORTS

  // Init method
  NS_IMETHOD Init();

  // nsIHttpNotify methods:
  NS_IMETHOD ModifyRequest(nsISupports *aContext);
  NS_IMETHOD AsyncExamineResponse(nsISupports *aContext);
   
  // nsCookieHTTPNotify methods:
  nsCookieHTTPNotify();
  virtual ~nsCookieHTTPNotify();

private:
    nsCOMPtr<nsIAtom> mCookieHeader;
    nsCOMPtr<nsIAtom> mSetCookieHeader;
    nsCOMPtr<nsIAtom> mExpiresHeader;
};

extern NS_EXPORT nsresult NS_NewCookieHTTPNotify(nsIHTTPNotify** aHTTPNotify);

#endif /* nsCookieHTTPNotify_h___ */
