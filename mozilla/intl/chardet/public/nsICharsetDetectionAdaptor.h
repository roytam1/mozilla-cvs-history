/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 * Copyright (C) 1999 Netscape Communications Corporation.  All Rights
 * Reserved.
 */
#ifndef nsICDETAdaptor_h__
#define nsICDETAdaptor_h__
#include "nsISupports.h"

class nsICharsetDetector;
class nsIWebShellServices;

// {12BB8F13-2389-11d3-B3BF-00805F8A6670}
#define NS_ICHARSETDETECTIONADAPTOR_IID \
{ 0x12bb8f13, 0x2389, 0x11d3, { 0xb3, 0xbf, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70 } }

/*  
   This interface is defined to be serverd as an Adaptor between
   nsIWebShell, nsICharsetDetector and nsIParserFilter

   It make the nsICharsetDetector implementation independent from the
   nsIParserFilter and nsIWebShell
 */

class nsICharsetDetectionAdaptor : public nsISupports {
public:  
  NS_DEFINE_STATIC_IID_ACCESSOR(NS_ICHARSETDETECTIONADAPTOR_IID)
  /*   
     Initialize it by setup the nsICharsetDetector and the 
     nsIWebShell
   */
  NS_IMETHOD Init(nsIWebShellServices* aWebShell, nsICharsetDetector *aDetector, const char* aCommand=nsnull) = 0;
};
#endif /* nsICDETAdaptor_h__ */
