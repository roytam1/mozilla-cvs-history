/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */
#include "nsNativeAppSupport.h"
 
#include <Gestalt.h>
#include <Dialogs.h>
#include <Resources.h>
 
 
#define rSplashDialog 512

class nsSplashScreenMac : public nsISplashScreen {
public:
    nsSplashScreenMac()
        : mDialog( 0 ), mPicHandle( 0 ), mRefCnt( 0 ) {
    }
    ~nsSplashScreenMac() {
        Hide();
    }

    NS_IMETHOD Show();
    NS_IMETHOD Hide();

    // nsISupports methods
    NS_IMETHOD_(nsrefcnt) AddRef() {
        mRefCnt++;
        return mRefCnt;
    }
    NS_IMETHOD_(nsrefcnt) Release() {
        --mRefCnt;
        if ( !mRefCnt ) {
            delete this;
            return 0;
        }
        return mRefCnt;
    }
    NS_IMETHOD QueryInterface( const nsIID &iid, void**p ) {
        nsresult rv = NS_OK;
        if ( p ) {
            *p = 0;
            if ( iid.Equals( NS_GET_IID( nsISplashScreen ) ) ) {
                nsISplashScreen *result = this;
                *p = result;
                NS_ADDREF( result );
            } else if ( iid.Equals( NS_GET_IID( nsISupports ) ) ) {
                nsISupports *result = NS_STATIC_CAST( nsISupports*, this );
                *p = result;
                NS_ADDREF( result );
            } else {
                rv = NS_NOINTERFACE;
            }
        } else {
            rv = NS_ERROR_NULL_POINTER;
        }
        return rv;
    }

    DialogPtr mDialog;
    PicHandle mPicHandle;
    nsrefcnt mRefCnt;
}; // class nsSplashScreenMac

NS_IMETHODIMP
nsSplashScreenMac::Show() {
	mDialog = ::GetNewDialog( rSplashDialog, nil, (WindowPtr)-1L );
	mPicHandle = GetPicture( rSplashDialog );
	SetWindowPic( mDialog, mPicHandle );
	::ShowWindow( mDialog );
	::SetPort( mDialog );
	Rect rect = (**mPicHandle).picFrame;
	::DrawPicture( mPicHandle, &rect ); 
    return NS_OK;
}

NS_IMETHODIMP
nsSplashScreenMac::Hide() {
	if ( mDialog ) {
		ReleaseResource( (Handle)mPicHandle );
        mPicHandle = 0;
		SetWindowPic( mDialog, NULL );
		DisposeWindow( mDialog );
        mDialog = 0;
	}
    return NS_OK;
}

nsresult NS_CreateSplashScreen(nsISplashScreen**aResult)
{
  if ( aResult ) {	
      *aResult = new nsSplashScreenMac;
      if ( *aResult ) {
          NS_ADDREF( *aResult );
          return NS_OK;
      } else {
          return NS_ERROR_OUT_OF_MEMORY;
      }
  } else {
      return NS_ERROR_NULL_POINTER;
  }
}

PRBool NS_CanRun() 
{
	long response = 0;
	OSErr err = ::Gestalt (gestaltSystemVersion, &response);
	if ( err || response < 0x850)
	{
		::StopAlert (5000, NULL);
		return PR_FALSE;
	}
	return PR_TRUE;
}