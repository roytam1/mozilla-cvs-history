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

#ifndef nsIImageRequest_h___
#define nsIImageRequest_h___

#include <stdio.h>
#include "nsISupports.h"
#include "nscore.h"

class nsIImageRequestObserver;
class nsIImage;

// IID for the nsIImageRequest interface
#define NS_IIMAGEREQUEST_IID    \
{ 0xc31444c0, 0xaec9, 0x11d1,  \
{ 0x9b, 0xc3, 0x00, 0x60, 0x08, 0x8c, 0xa6, 0xb3 } }

/**
 *
 * An image request generated as a result of invoking the 
 * <code>GetImage</code> method of the <code>nsIImageGroup</code>
 * interface. 
 */
class nsIImageRequest : public nsISupports {
public:  
  /// @return the image object associated with the request.
  virtual nsIImage* GetImage() = 0;
  
  /**
   *  Returns the natural dimensions of the image.  Returns 0,0 
   *  if the dimensions are unknown.
   * 
   * @param aWidth, aHeight - pointers to integers to be filled in with
   *      the dimensions.
   */
  virtual void GetNaturalDimensions(PRUint32 *aWidth, PRUint32 *aHeight) = 0;
 
  /** 
   * Add an observers to be informed of image loading notifications.
   *
   * @param aObserver - An observer to add to the observer list.
   * @param boolean indicating whether addition was successful.
   */
  virtual PRBool AddObserver(nsIImageRequestObserver *aObserver) = 0;

  /**
   * Remove a previously added observer from the observer list.
   *
   * @param aObserver - An observer to remove from the observer list.
   * @param boolean indicating whether the removal was successful.
   */ 
  virtual PRBool RemoveObserver(nsIImageRequestObserver *aObserver) = 0;

  /// Interrupt loading of just this image.
  virtual void Interrupt() = 0;
};
#endif
