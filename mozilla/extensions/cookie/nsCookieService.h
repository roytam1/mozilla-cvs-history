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

#ifndef nsCookieService_h__
#define nsCookieService_h__

#include "nsICookieService.h"
#include "nsIObserver.h"
#include "nsIWebProgressListener.h"
#include "nsWeakReference.h"
#include "nsIIOService.h"
#include "nsIFile.h"
#include "nsITimer.h"
#include "nsIObserverService.h"
#include "nsIPrefBranch.h"
#include "nsIPrefBranchInternal.h"
#include "nsXPIDLString.h"

////////////////////////////////////////////////////////////////////////////////
// nsCookiePrefObserver

// enumerated type, used to specify default cookie behavior
typedef enum {
  PERMISSION_Accept,
  PERMISSION_DontAcceptForeign,
  PERMISSION_DontUse,
  PERMISSION_P3P
} PERMISSION_BehaviorEnum;

class nsCookiePrefObserver : public nsIObserver
                           , public nsSupportsWeakReference
{
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER

    nsCookiePrefObserver();
    virtual ~nsCookiePrefObserver();
    nsresult Init();
    nsresult ReadPrefs();

    // member variables for caching prefs
#ifdef MOZ_PHOENIX
    // unfortunately, we require this #ifdef for now, since Phoenix uses different
    // (more optimized) prefs to Mozilla. This will be fixed shortly.
    // the following variables are Phoenix hacks to reduce ifdefs in the code.
    PRPackedBool                mCookiesEnabled_temp,               // These two prefs are collapsed
                                mCookiesForDomainOnly_temp,         // into mCookiesPermissions.
                                mCookiesDisabledForMailNews;        // Disable cookies in mailnews
#else
    PRPackedBool                mCookiesDisabledForMailNews;        // Disable cookies in mailnews
#endif
    PRPackedBool                mCookiesAskPermission, // Ask user permission before storing cookie
                                mCookiesLifetimeEnabled,            // Cookie lifetime limit enabled
                                mCookiesLifetimeCurrentSession;     // Limit cookie lifetime to current session
    PRInt32                     mCookiesLifetimeSec;                // Lifetime limit specified in seconds
    PRBool                      mCookiesStrictDomains; // Optional pref to apply stricter domain checks
    PERMISSION_BehaviorEnum     mCookiesPermissions;   // PERMISSION_{Accept, DontAcceptForeign, DontUse, P3P}
    nsXPIDLCString              mCookiesP3PString;                  // P3P settings

  private:
    nsCOMPtr<nsIPrefBranch> mPrefBranch;
};

extern nsCookiePrefObserver *gCookiePrefObserver;

// nsCookieService
class nsCookieService : public nsICookieService,
                        public nsIObserver,
                        public nsIWebProgressListener,
                        public nsSupportsWeakReference {
public:

  // nsISupports
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER
  NS_DECL_NSIWEBPROGRESSLISTENER
  NS_DECL_NSICOOKIESERVICE

  nsCookieService();
  virtual ~nsCookieService();
  nsresult Init();

protected:
  // cached things
  nsCOMPtr<nsIFile> mCookieFile;
  nsCOMPtr<nsIObserverService> mObserverService;

  // Use LazyWrite to save the cookies file on a timer. It will write
  // the file only once if repeatedly hammered quickly.
  void LazyWrite(PRBool aForce);
  static void DoLazyWrite(nsITimer *aTimer, void *aClosure);

  nsCOMPtr<nsITimer> mWriteTimer;
  PRUint32           mLoadCount;
  PRBool             mWritePending;
};

#endif /* nsCookieService_h__ */
