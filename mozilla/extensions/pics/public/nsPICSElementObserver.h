/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL. You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation. Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All Rights
 * Reserved.
 */

#ifndef nsPICSElementObserver_h___
#define nsPICSElementObserver_h___

#include "nsIFactory.h"
#include "nsIElementObserver.h"
#include "nsIObserver.h"


// {E12F6997-F28F-11d2-8ACE-00105A1B8860}
#define NS_IPICSELEMENTOBSERVER_IID \
{ 0xbb170ff3, 0xf814, 0x11d2, { 0x8a, 0xd0, 0x0, 0x10, 0x5a, 0x1b, 0x88, 0x60 } };



class nsPICSElementObserver : public nsIElementObserver, public nsIObserver {
public:
 
    nsPICSElementObserver();
    virtual ~nsPICSElementObserver(void);

    /*
   *   This method return the tag which the observer care about
   */
//  NS_IMETHOD GetTagName(const char * oTag);
  NS_IMETHOD_(const char*) GetTagName();

  /*
   *   Subject call observer when the parser hit the tag
   *   @param aDocumentID- ID of the document
   *   @param aTag- the tag
   *   @param numOfAttributes - number of attributes
   *   @param nameArray - array of name. 
   *   @param valueArray - array of value
   */
  NS_IMETHOD Notify(PRUint32 aDocumentID, eHTMLTags aTag, 
                    PRUint32 numOfAttributes, const PRUnichar* nameArray[], 
                    const PRUnichar* valueArray[]);

  NS_IMETHOD Notify(nsISupports** result);
   
  NS_DECL_ISUPPORTS

   
private:

};

class nsPICSElementObserverFactory : public nsIFactory {
public:

    NS_DECL_ISUPPORTS

    // nsIFactory methods:

    NS_IMETHOD CreateInstance(nsISupports *aOuter,
                              REFNSIID aIID,
                              void **aResult);

    NS_IMETHOD LockFactory(PRBool aLock);

    // nsPICSElementObserver methods:

    nsPICSElementObserverFactory(void);
    virtual ~nsPICSElementObserverFactory(void);

};

extern NS_EXPORT nsresult NS_NewPICSElementObserver(nsIObserver** anObserver);

#endif /* nsPICSElementObserver_h___ */
