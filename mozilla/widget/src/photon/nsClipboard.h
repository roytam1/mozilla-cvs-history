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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#ifndef nsClipboard_h__
#define nsClipboard_h__

#include "nsBaseClipboard.h"
#include <Pt.h>


class nsITransferable;
class nsIClipboardOwner;
class nsIWidget;

/**
 * Native Photon Clipboard wrapper
 */

class nsClipboard : public nsBaseClipboard
{

public:
  nsClipboard();
  virtual ~nsClipboard();

  // nsIClipboard  
  NS_IMETHOD ForceDataToClipboard();
  NS_IMETHOD HasDataMatchingFlavors(nsISupportsArray* aFlavorList, PRBool * outResult);

protected:
  NS_IMETHOD SetNativeClipboardData();
  NS_IMETHOD GetNativeClipboardData(nsITransferable * aTransferable);

  nsresult GetFormat(const char* aMimeStr, PhClipHeader *cliphdr );
};

#endif // nsClipboard_h__
