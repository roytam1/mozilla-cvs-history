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

#ifndef nsIMenuItem_h__
#define nsIMenuItem_h__

#include "prtypes.h"
#include "nsISupports.h"
#include "nsString.h"

#include "nsIWebShell.h"
#include "nsIDOMElement.h"

// {7F045771-4BEB-11d2-8DBB-00609703C14E}
#define NS_IMENUITEM_IID      \
{ 0x7f045771, 0x4beb, 0x11d2, \
  { 0x8d, 0xbb, 0x0, 0x60, 0x97, 0x3, 0xc1, 0x4e } }

class nsIMenu;
class nsIPopUpMenu;
class nsIWidget;
class nsIMenuListener;

enum {
  knsMenuItemNoModifier      = 0,
  knsMenuItemShiftModifier   = (1 << 0),
  knsMenuItemAltModifier     = (1 << 1),
  knsMenuItemControlModifier = (1 << 2),
  knsMenuItemCommandModifier = (1 << 3)
};

/**
 * MenuItem widget
 */
class nsIMenuItem : public nsISupports {

  public:
    NS_DEFINE_STATIC_IID_ACCESSOR(NS_IMENUITEM_IID)

   /**
    * Creates the MenuItem
    *
    */
    NS_IMETHOD Create(nsISupports    * aParent, 
                      const nsString & aLabel, 
                      PRBool           isSeparator) = 0;
    
   /**
    * Get the MenuItem label
    *
    */
    NS_IMETHOD GetLabel(nsString &aText) = 0;

   /**
    * Get the MenuItem label

    *
    */
    NS_IMETHOD SetLabel(nsString &aText) = 0;

   /**
    * Set the Menu shortcut char
    *
    */
    NS_IMETHOD SetShortcutChar(const nsString &aText) = 0;
  
    /**
    * Get the Menu shortcut char
    *
    */
    NS_IMETHOD GetShortcutChar(nsString &aText) = 0;
   /**
    * Sets whether the item is enabled or disabled
    *
    */
    NS_IMETHOD SetEnabled(PRBool aIsEnabled) = 0;

   /**
    * Gets whether the item is enabled or disabled
    *
    */
    NS_IMETHOD GetEnabled(PRBool *aIsEnabled) = 0;

   /**
    * Sets whether the item is checked or not
    *
    */
    NS_IMETHOD SetChecked(PRBool aIsEnabled) = 0;

   /**
    * Gets whether the item is checked or not
    *
    */
    NS_IMETHOD GetChecked(PRBool *aIsEnabled) = 0;

   /**
    * Sets whether the item is a checkbox type
    *
    */
    NS_IMETHOD SetCheckboxType(PRBool aIsCheckbox) = 0;

   /**
    * Gets whether the item is a checkbox type
    *
    */
    NS_IMETHOD GetCheckboxType(PRBool *aIsCheckbox) = 0;
    
   /**
    * Gets the MenuItem Command identifier
    *
    */
    NS_IMETHOD GetCommand(PRUint32 & aCommand) = 0;

   /**
    * Gets the target for MenuItem
    *
    */
    NS_IMETHOD GetTarget(nsIWidget *& aTarget) = 0;
    
   /**
    * Gets Native Menu Handle
    *
    */
    NS_IMETHOD GetNativeData(void*& aData) = 0;

   /**
    * Adds menu listener
    *
    */
    NS_IMETHOD AddMenuListener(nsIMenuListener * aMenuListener) = 0;

   /**
    * Removes menu listener
    *
    */
    NS_IMETHOD RemoveMenuListener(nsIMenuListener * aMenuListener) = 0;

   /**
    * Indicates whether it is a separator
    *
    */
    NS_IMETHOD IsSeparator(PRBool & aIsSep) = 0;

   /**
    * Sets the JavaScript Command to be invoked when a "gui" event occurs on a source widget
    * @param aStrCmd the JS command to be cached for later execution
    * @return NS_OK 
    */
    NS_IMETHOD SetCommand(const nsString & aStrCmd) = 0;

   /**
    * Executes the "cached" JavaScript Command 
    * @return NS_OK if the command was executed properly, otherwise an error code
    */
    NS_IMETHOD DoCommand() = 0;

    NS_IMETHOD SetDOMNode(nsIDOMNode * aDOMNode) = 0;
    NS_IMETHOD GetDOMNode(nsIDOMNode ** aDOMNode) = 0;
    NS_IMETHOD SetDOMElement(nsIDOMElement * aDOMElement) = 0;
    NS_IMETHOD GetDOMElement(nsIDOMElement ** aDOMElement) = 0;
    NS_IMETHOD SetWebShell(nsIWebShell * aWebShell) = 0;
    
    /**
    *
    */
    NS_IMETHOD SetModifiers(PRUint8 aModifiers) = 0;
    NS_IMETHOD GetModifiers(PRUint8 * aModifiers) = 0;
};

#endif
