/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License.  You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are Copyright (C) 1998
 * Netscape Communications Corporation.  All Rights Reserved.
 */

#ifndef nsUConvDll_h___
#define nsUConvDll_h___

#include "nsISupports.h"

extern "C" PRInt32 g_InstanceCount;
extern "C" PRInt32 g_LockCount;

// Factory methods

NS_IMETHODIMP
NS_NewCharsetConverterManager(nsISupports* aOuter, const nsIID& aIID,
                              void** aResult);

NS_IMETHODIMP
NS_NewUnicodeDecodeHelper(nsISupports* aOuter, const nsIID& aIID,
                          void** aResult);

NS_IMETHODIMP
NS_NewUnicodeEncodeHelper(nsISupports* aOuter, const nsIID& aIID,
                          void** aResult);

NS_IMETHODIMP
NS_NewPlatformCharset(nsISupports* aOuter, const nsIID& aIID,
                      void** aResult);

NS_IMETHODIMP
NS_NewCharsetAlias(nsISupports* aOuter, const nsIID& aIID,
                   void** aResult);

NS_IMETHODIMP
NS_NewCharsetMenu(nsISupports* aOuter, const nsIID& aIID,
                  void** aResult);

NS_IMETHODIMP
NS_NewTextToSubURI(nsISupports* aOuter, const nsIID& aIID,
                   void** aResult);

#endif /* nsUConvDll_h___ */
