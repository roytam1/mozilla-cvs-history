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

#ifndef nsLegendFrame_h___
#define nsLegendFrame_h___

#include "nsAreaFrame.h"
class  nsIContent;
class  nsIFrame;
class  nsIPresContext;
struct nsHTMLReflowMetrics;
class  nsIRenderingContext;
struct nsRect;

#define NS_LEGEND_FRAME_CID \
{ 0x73805d40, 0x5a24, 0x11d2, { 0x80, 0x46, 0x0, 0x60, 0x8, 0x15, 0xa7, 0x91 } }

class nsLegendFrame : public nsAreaFrame {
public:

  nsLegendFrame();

  NS_IMETHOD QueryInterface(const nsIID& aIID, void** aInstancePtr);

#ifdef NS_DEBUG
  NS_IMETHOD GetFrameName(nsString& aResult) const;
#endif

  PRInt32 GetAlign();
};


#endif // guard
