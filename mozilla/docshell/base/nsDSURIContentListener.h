/* -*- Mode: C++; tab-width: 3; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
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
 * The Original Code is the Mozilla browser.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications, Inc.  Portions created by Netscape are
 * Copyright (C) 1999, Mozilla.  All Rights Reserved.
 * 
 * Contributor(s):
 *   Travis Bogard <travis@netscape.com>
 */

#ifndef nsDSURIContentListener_h__
#define nsDSURIContentListener_h__

#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsIURIContentListener.h"

class nsDocShell;

class nsDSURIContentListener : public nsIURIContentListener
{
friend class nsDocShell;
public:
   NS_DECL_ISUPPORTS

   NS_DECL_NSIURICONTENTLISTENER

protected:
   nsDSURIContentListener();
   virtual ~nsDSURIContentListener();

   void DocShell(nsDocShell* aDocShell);
   nsDocShell* DocShell();
   void GetPresContext(nsIPresContext** aPresContext);
   void SetPresContext(nsIPresContext* aPresContext);

protected:
   nsDocShell*                      mDocShell;
   nsCOMPtr<nsIPresContext>         mPresContext;
   nsCOMPtr<nsISupports>            mLoadCookie; // the load cookie associated with the window context.

   nsIURIContentListener*            mParentContentListener;  // Weak Reference
};

#endif /* nsDSURIContentListener_h__ */
