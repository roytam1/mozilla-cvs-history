/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

#ifndef nsIRefreshUrl_h___
#define nsIRefreshUrl_h___

#include "nscore.h"
#include "nsISupports.h"
#include "nsIURL.h"

#include "nspr.h"

/* 69EFC430-2EFE-11d2-9E5D-006008BF092E */
#define NS_IREFRESHURL_IID                              \
{ 0x69efc430, 0x2efe, 0x11d2,                           \
    { 0x9e, 0x5d, 0x0, 0x60, 0x8, 0xbf, 0x9, 0x2e } }

struct nsIRefreshUrl : public nsISupports
{
    /**
      * Reloads a url after waiting millis milliseconds.
      *
      * @param url The url to refresh.
      * @param millis The number of milliseconds to wait.
      * @param repeat Do you want the url to be repeatedly refreshed every millis milliseconds.
      */
    NS_IMETHOD RefreshURL(nsIURL *url, PRInt32 millis, PRBool repeat) = 0;

    /**
      * Cancels all timer loads.
      */
    NS_IMETHOD CancelRefreshURLTimers(void) = 0;
};

#endif /* nsIRefreshUrl_h___ */
