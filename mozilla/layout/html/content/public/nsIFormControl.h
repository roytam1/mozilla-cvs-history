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
#ifndef nsIFormControl_h___
#define nsIFormControl_h___

#include "nsISupports.h"
class nsIDOMHTMLFormElement;
class nsIWidget;

#define NS_FORM_BROWSE          0
#define NS_FORM_BUTTON          1
#define NS_FORM_FIELDSET        2
#define NS_FORM_INPUT_BUTTON    3
#define NS_FORM_INPUT_CHECKBOX  4
#define NS_FORM_INPUT_FILE      5
#define NS_FORM_INPUT_HIDDEN    6
#define NS_FORM_INPUT_RESET     7
#define NS_FORM_INPUT_IMAGE     8
#define NS_FORM_INPUT_PASSWORD  9
#define NS_FORM_INPUT_RADIO    10
#define NS_FORM_INPUT_SUBMIT   11
#define NS_FORM_INPUT_TEXT     12
#define NS_FORM_OPTION         13
#define NS_FORM_OPTGROUP       14
#define NS_FORM_LEGEND         15
#define NS_FORM_SELECT         16
#define NS_FORM_TEXTAREA       17

#define NS_FORM_NOTOK          0xFFFFFFF7
#define NS_FORM_NOTSET         0xFFFFFFF7

#define NS_IFORMCONTROL_IID   \
{ 0x282ff440, 0xcd7e, 0x11d1, \
  {0x89, 0xad, 0x00, 0x60, 0x08, 0x91, 0x1b, 0x81} }


/**
  * Interface which all form controls (e.g. buttons, checkboxes, text,
  * radio buttons, select, etc) implement in addition to their dom specific interface. 
 **/
class nsIFormControl : public nsISupports {
public:
  /**
    * Get the form for this form control. 
    * @param aForm the form to get
    * @return NS_OK
    */
  NS_IMETHOD GetForm(nsIDOMHTMLFormElement** aForm) = 0;

  /**
    * Set the form for this form control.
    * @param aForm the form
    * @return NS_OK
    */
  NS_IMETHOD SetForm(nsIDOMHTMLFormElement* aForm) = 0;

  /**
    * Get the type of this control
    * @param aType the type to be returned
    * @return NS_OK
    */
  NS_IMETHOD GetType(PRInt32* aType) = 0;

  NS_IMETHOD SetWidget(nsIWidget* aWidget) = 0;

  NS_IMETHOD Init() = 0;

};

#endif /* nsIFormControl_h___ */
