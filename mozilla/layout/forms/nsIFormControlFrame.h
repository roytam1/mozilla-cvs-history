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

#ifndef nsIFormControlFrame_h___
#define nsIFormControlFrame_h___

#include "nsISupports.h"
#include "nsFont.h"
class nsFormFrame;
class nsIPresContext;
class nsString;
class nsIContent;


// IID for the nsIFormControlFrame class
#define NS_IFORMCONTROLFRAME_IID    \
{ 0x38eb3980, 0x4d99, 0x11d2,  \
  { 0x80, 0x3f, 0x0, 0x60, 0x8, 0x15, 0xa7, 0x91 } }

/** 
  * nsIFormControlFrame is the common interface for frames of form controls. It
  * provides a uniform way of creating widgets, resizing, and painting.
  * @see nsLeafFrame and its base classes for more info
  */
class nsIFormControlFrame : public nsISupports {

public:
  NS_DEFINE_STATIC_IID_ACCESSOR(NS_IFORMCONTROLFRAME_IID)

  NS_IMETHOD GetType(PRInt32* aType) const =  0;

  NS_IMETHOD GetName(nsString* aName) = 0;

  virtual void SetFocus(PRBool aOn = PR_TRUE, PRBool aRepaint = PR_FALSE) = 0;

  virtual void ScrollIntoView(nsIPresContext* aPresContext) = 0;  

  virtual void MouseClicked(nsIPresContext* aPresContext) = 0;

  virtual void Reset(nsIPresContext* aPresContext) = 0;

  virtual PRBool IsSuccessful(nsIFormControlFrame* aSubmitter) = 0;

  virtual PRInt32 GetMaxNumValues() = 0;

  virtual PRBool  GetNamesValues(PRInt32 aMaxNumValues, PRInt32& aNumValues,
                                 nsString* aValues, nsString* aNames) = 0;

  virtual void SetFormFrame(nsFormFrame* aFrame) = 0;

  virtual nscoord GetVerticalInsidePadding(nsIPresContext* aPresContext,
                                           float aPixToTwip,
                                           nscoord aInnerHeight) const = 0;
  virtual nscoord GetHorizontalInsidePadding(nsIPresContext* aPresContext,
                                             float aPixToTwip, 
                                             nscoord aInnerWidth,
                                             nscoord aCharWidth) const = 0;


  /**
   * Set the suggested size for the form element. 
   * This is used to control the size of the element during reflow if it hasn't had it's size
   * explicitly set.
   * @param aWidth width of the form element
   * @param aHeight height of the form element
   * @returns NS_OK 
   */

  NS_IMETHOD SetSuggestedSize(nscoord aWidth, nscoord aHeight) = 0;
  
  /**
   * Determine if the control uses a native widget for rendering
   * @param aRequiresWidget is set to PR_TRUE if it has a native widget, PR_FALSE otherwise.
   * @returns NS_OK 
   */


   /**
   * Determine if the control uses a native widget for rendering
   * @param aRequiresWidget is set to PR_TRUE if it has a native widget, PR_FALSE otherwise.
   * @returns NS_OK 
   */

  virtual nsresult RequiresWidget(PRBool &aRequiresWidget) = 0;
 


  NS_IMETHOD GetFont(nsIPresContext* aPresContext, 
                    nsFont&         aFont) = 0;
  /**
   * Get the content object associated with this frame. Adds a reference to
   * the content object so the caller must do a release.
   *
   * @see nsISupports#Release()
   */
  NS_IMETHOD GetFormContent(nsIContent*& aContent) const = 0;

  /**
   * Set a property on the form control frame.
   *
   * @param aName name of the property to set
   * @param aValue value of the property
   * @returns NS_OK if the property name is valid, otherwise an error code
   */
  
  NS_IMETHOD SetProperty(nsIPresContext* aPresContext, nsIAtom* aName, const nsString& aValue) = 0;
  
  /**
   * Get a property from the form control frame
   *
   * @param aName name of the property to get
   * @param aValue value of the property
   * @returns NS_OK if the property name is valid, otherwise an error code
   */

  NS_IMETHOD GetProperty(nsIAtom* aName, nsString& aValue) = 0; 

  

};

#endif

