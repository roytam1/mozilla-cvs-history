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
 * Copyright (C) 1998-2000 Netscape Communications Corporation.
 * All Rights Reserved.
 * 
 * Contributor(s): 
 *   Stuart Parmenter <pavlov@netscape.com>
 */

#ifndef NSSIZE_H
#define NSSIZE_H

#include "gfxtypes.h"

// Maximum allowable size
#define NS_MAXSIZE  nscoord(1 << 30)

/**
 * nsSize
 *
 * @version 0.0
 **/
struct nsSize {
  gfx_dimension width;
  gfx_dimension height;

  // Constructors
  nsSize() {}
  nsSize(const nsSize& aSize) {width = aSize.width; height = aSize.height;}
  nsSize(gfx_dimension aWidth, gfx_dimension aHeight) {width = aWidth; height = aHeight;}

  void SizeTo(gfx_dimension aWidth, gfx_dimension aHeight) {width = aWidth; height = aHeight;}
  void SizeBy(gfx_dimension aDeltaWidth, gfx_dimension aDeltaHeight) {
    width += aDeltaWidth;
    height += aDeltaHeight;
  }

  // Overloaded operators. Note that '=' isn't defined so we'll get the
  // compiler generated default assignment operator
  PRBool  operator==(const nsSize& aSize) const {
    return (PRBool) ((width == aSize.width) && (height == aSize.height));
  }
  PRBool  operator!=(const nsSize& aSize) const {
    return (PRBool) ((width != aSize.width) || (height != aSize.height));
  }
  nsSize operator+(const nsSize& aSize) const {
    return nsSize(width + aSize.width, height + aSize.height);
  }
  nsSize operator-(const nsSize& aSize) const {
    return nsSize(width - aSize.width, height - aSize.height);
  }
  nsSize& operator+=(const nsSize& aSize) {width += aSize.width;
                                           height += aSize.height;
                                           return *this;}
  nsSize& operator-=(const nsSize& aSize) {width -= aSize.width;
                                           height -= aSize.height;
                                           return *this;}
};

#endif /* NSSIZE_H */
