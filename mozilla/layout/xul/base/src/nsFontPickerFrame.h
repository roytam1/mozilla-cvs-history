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

//
// nsFontPickerFrame
//

#ifndef nsFontPickerFrame_h__
#define nsFontPickerFrame_h__


#include "nsLeafFrame.h"
#include "prtypes.h"
#include "nsIAtom.h"
#include "nsCOMPtr.h"

class nsString;


nsresult NS_NewFontPickerFrame(nsIPresShell* aPresShell, nsIFrame** aResult) ;


class nsFontPickerFrame : public nsLeafFrame
{
public:
  nsFontPickerFrame();

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsString& aResult) const {
    return MakeFrameName("FontPickerFrame", aResult);
  }
#endif

  // nsIFrame overrides
  NS_IMETHOD Paint(nsIPresContext* aPresContext,
                   nsIDrawable* aDrawable,
                   const nsRect& aDirtyRect,
                   nsFramePaintLayer aWhichLayer);
  
protected:

  virtual void GetDesiredSize(nsIPresContext* aPresContext,
                              const nsHTMLReflowState& aReflowState,
                              nsHTMLReflowMetrics& aDesiredSize) ;

}; // class nsFontPickerFrame

#endif
