/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#ifndef nsISoftwareUpdate_h__
#define nsISoftwareUpdate_h__

#include "nsISupports.h"
#include "nsIFactory.h"
#include "nsString.h"

class nsInstallInfo;

#define NS_ISOFTWAREUPDATE_IID                  \
{ 0x18c2f992, 0xb09f, 0x11d2,                   \
{0xbc, 0xde, 0x00, 0x80, 0x5f, 0x0e, 0x13, 0x53}} 


class nsISoftwareUpdate : public nsISupports 
{
    public:
        static const nsIID& IID() { static nsIID iid = NS_ISOFTWAREUPDATE_IID; return iid; }
            
            NS_IMETHOD InstallJar(nsInstallInfo *installInfo) = 0;

            NS_IMETHOD InstallJar(const nsString& fromURL, 
                                  const nsString& flags, 
                                  const nsString& args) = 0;  
            
};



class nsSoftwareUpdateFactory : public nsIFactory 
{
    public:
        
        nsSoftwareUpdateFactory();
        ~nsSoftwareUpdateFactory();
        
        NS_DECL_ISUPPORTS

              NS_IMETHOD CreateInstance(nsISupports *aOuter,
                                        REFNSIID aIID,
                                        void **aResult);

              NS_IMETHOD LockFactory(PRBool aLock);

};


#endif // nsISoftwareUpdate_h__

