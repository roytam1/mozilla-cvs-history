/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef nsCookie_h__
#define nsCookie_h__

#include "nsICookie.h"
#include "nsWeakReference.h"

////////////////////////////////////////////////////////////////////////////////

class nsCookie : public nsICookie,
                        public nsSupportsWeakReference {
public:

  // nsISupports
  NS_DECL_ISUPPORTS
  NS_DECL_NSICOOKIE

  // Note: following constructor takes ownership of the four strings (name, value
  //       host, and path) passed to it so the caller of the constructor must not
  //       free them
  nsCookie
    (char * name,
     char * value,
     PRBool isDomain,
     char * host,
     char * path,
     PRBool isSecure,
     PRUint64 expires,
     nsCookieStatus status,
     nsCookiePolicy policy
     );
  nsCookie();
  virtual ~nsCookie(void);
  
protected:
  char * cookieName;
  char * cookieValue;
  PRBool cookieIsDomain;
  char * cookieHost;
  char * cookiePath;
  PRBool cookieIsSecure;
  PRUint64 cookieExpires;
  nsCookieStatus cookieStatus;
  nsCookiePolicy cookiePolicy;  
};

// {E9FCB9A4-D376-458f-B720-E65E7DF593BC}
#define NS_COOKIE_CID { 0xe9fcb9a4,0xd376,0x458f,{0xb7,0x20,0xe6,0x5e,0x7d,0xf5,0x93,0xbc}}

#endif /* nsCookie_h__ */
