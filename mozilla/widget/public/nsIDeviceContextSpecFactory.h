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

#ifndef nsIDeviceContextSpecFactory_h___
#define nsIDeviceContextSpecFactory_h___

#include "nsISupports.h"

class nsIDeviceContextSpec;

#define NS_IDEVICE_CONTEXT_SPEC_FACTORY_IID   \
{ 0xf6669570, 0x7b3d, 0x11d2, \
{ 0xa8, 0x48, 0x00, 0x40, 0x95, 0x9a, 0x28, 0xc9 } }

class nsIDeviceContextSpecFactory : public nsISupports
{
public:
  /**
   * Initialize the device context spec factory
   * @return error status
   */
  NS_IMETHOD Init(void) = 0;

  /**
   * Get a device context specification. Typically, this
   * means getting information about a printer. A previously
   * returned device context spec can be passed in and used as
   * a starting point for getting a new spec (or simply returning
   * the old spec again). Additionally, if it is desirable to
   * get the device context spec without user intervention, any
   * dialog boxes can be supressed by passing in PR_TRUE for the
   * aQuiet parameter.
   * @param aOldSpec a previously obtained devince context spec which
   *        can be used to initialize parameters of the new context to
   *        be returned.
   * @param aNewSpec out parameter for device context spec returned. the
   *        aOldSpec may be returned if the object is recyclable.
   * @param aQuiet if PR_TRUE, prevent the need for user intervention
   *        in obtaining device context spec. if nsnull is passed in for
   *        the aOldSpec, this will typically result in getting a device
   *        context spec for the default output device (i.e. default
   *        printer).
   * @return error status
   */
  NS_IMETHOD CreateDeviceContextSpec(nsIDeviceContextSpec *aOldSpec,
                                     nsIDeviceContextSpec *&aNewSpec,
                                     PRBool aQuiet) = 0;
};

#endif
