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

#ifndef nsIComboboxControlFrame_h___
#define nsIComboboxControlFrame_h___

#include "nsISupports.h"
#include "nsFont.h"
class nsFormFrame;
class nsIPresContext;
class nsString;
class nsIContent;


// IID for the nsIComboboxControlFrame class
#define NS_ICOMBOBOXCONTROLFRAME_IID    \
{ 0x6961f791, 0xa662, 0x11d2,  \
  { 0x8d, 0xcf, 0x0, 0x60, 0x97, 0x3, 0xc1, 0x4e } }

/** 
  * nsIComboboxControlFrame is the common interface for frames of form controls. It
  * provides a uniform way of creating widgets, resizing, and painting.
  * @see nsLeafFrame and its base classes for more info
  */
class nsIComboboxControlFrame : public nsISupports {

public:

  /**
   * Sets the Drop Down List
   *
   */
  NS_IMETHOD SetDropDown(nsIFrame* aPlaceHolderFrame, nsIFrame* aDropDownFrame) = 0;

  /**
   * Sets the Pseudo Style Contexts for the DropDown
   *
   */
  NS_IMETHOD SetDropDownStyleContexts(nsIStyleContext * aVisible, nsIStyleContext * aHidden) = 0;

  /**
   * Sets the Pseudo Style Contexts for the Button
   *
   */
  NS_IMETHOD SetButtonStyleContexts(nsIStyleContext * aOut, nsIStyleContext * aPressed) = 0;

  /**
   * Notifies the Combobox the List was selected
   *
   */
  NS_IMETHOD ListWasSelected(nsIPresContext* aPresContext) = 0;

};

#endif

