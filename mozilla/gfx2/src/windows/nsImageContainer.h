/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is mozilla.org code.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation. Portions created by Netscape are
 * Copyright (C) 2001 Netscape Communications Corporation. All
 * Rights Reserved.
 * 
 * Contributor(s): 
 *   Stuart Parmenter <pavlov@netscape.com>
 */

#include "nsIImageContainer.h"
#include "nsPIImageContainerWin.h"

#include "nsSize.h"

#include "nsSupportsArray.h"

#define NS_IMAGECONTAINER_CID \
{ /* aa699204-1dd1-11b2-84a9-a280c268e4fb */         \
     0xaa699204,                                     \
     0x1dd1,                                         \
     0x11b2,                                         \
    {0x84, 0xa9, 0xa2, 0x80, 0xc2, 0x68, 0xe4, 0xfb} \
}

class nsImageContainer : public nsIImageContainer,
                         public nsPIImageContainerWin
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIIMAGECONTAINER
  NS_DECL_NSPIIMAGECONTAINERWIN

  nsImageContainer();
  virtual ~nsImageContainer();

private:
  /* additional members */
  nsSupportsArray mFrames;
  nsSize mSize;
  PRUint32 mCurrentFrame;
};

