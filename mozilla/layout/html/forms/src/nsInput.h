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

#ifndef nsInput_h___
#define nsInput_h___

#include "nsHTMLTagContent.h"
#include "nsIFormControl.h"
class nsIFormManager;
class nsIWidget;
class nsIView;
class nsString;

/**
  * nsInput represents an html Input element. This is a base class for
  * the various Input types (button, checkbox, file, hidden, password,
  * reset, radio, submit, text)
  */
class nsInput : public nsHTMLTagContent {
public:
  /** 
    * main constructor
    * @param aTag the html tag associated with this object
    * @param aManager the form manager to manage this input
    */
  nsInput(nsIAtom* aTag, nsIFormManager* aManager);

  /**
    * @see nsISupports QueryInterface
    */
  NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr);

  /**
    * @see nsISupports Release
    */
  NS_IMETHOD_(nsrefcnt) Release(void);

  /**
    * @see nsIContentDelegate CreateFrame
    */
  virtual nsIFrame* CreateFrame(nsIPresContext* aPresContext,
                                PRInt32 aIndexInParent,
                                nsIFrame* aParentFrame);

  // nsIFormControl methods

  /**
    * @see nsIFormControl GetFormManager
    */
  virtual nsIFormManager* GetFormManager() const;

  /**
    * @see nsIFormControl GetFormManager
    */
  virtual PRInt32 GetMaxNumValues();

  /**
    * @see nsIFormControl GetFormManager
    */
  virtual PRBool GetName(nsString& aName);

  /**
    * @see nsIFormControl GetFormManager
    */
  virtual PRBool GetValues(PRInt32 aMaxNumValues, PRInt32& aNumValues,
                           nsString* aValues);

  /**
    * @see nsIFormControl GetFormManager
    */
  virtual void Reset();

  /**
    * @see nsIFormControl GetFormManager
    */
  virtual void SetFormManager(nsIFormManager* aFormMan, PRBool aDecrementRef = PR_TRUE);

  // attribute methods

  /**
    * Get a named attribute of this input
    * @param aAttribute the name of the attribute
    * @param aValue the value of the attribute
    * @return eContentAttr_HasValue if there is a value, eContentAttr_NoValue
    * if there is an attribute but no value, or eContentAttr_HasValue
    * if there is no attribute.
    */
  virtual nsContentAttr GetAttribute(nsIAtom* aAttribute,
                                     nsHTMLValue& aValue) const;

  /**
    * Set the named attribute of this input
    * @param aAttribute the name of the attribute
    * @param aValue the value of the attribute
    */
  virtual void SetAttribute(nsIAtom* aAttribute, const nsString& aValue);


  // misc methods

  /**
    * Get the number of references to this input
    * @return the number of references
   **/
  virtual nsrefcnt GetRefCount() const;

  /**
    * Get the widget associated with this input
    * @return the widget, not a copy
   **/
  nsIWidget* GetWidget();

  /**
    * Set the widget associated with this input
    * @param aWidget the widget
   **/
  void SetWidget(nsIWidget* aWidget);

  // this should not be public
  static PRInt32 GetOuterOffset() {
    return offsetof(nsInput,mControl);
  }

protected:
  virtual         ~nsInput();

  /*
   * Get the type of this input object
   */
  virtual void GetType(nsString& aResult) const = 0;

  nsIWidget*      mWidget;
  nsIFormManager* mFormMan;

  // Attributes common to all html form elements
  nsString*       mName;

  // Aggregator class and instance variable used to aggregate in the
  // nsIFormControl interface to nsInput w/o using multiple
  // inheritance.
  class AggInputControl : public nsIFormControl {
  public:
    AggInputControl();
    ~AggInputControl();

    // nsISupports
    NS_DECL_ISUPPORTS

    // nsIFormControl
    virtual PRBool GetName(nsString& aName);
    virtual PRInt32 GetMaxNumValues();
    virtual PRBool GetValues(PRInt32 aMaxNumValues, PRInt32& aNumValues,
                             nsString* aValues);
    virtual void Reset();
    virtual void SetFormManager(nsIFormManager* aFormMan, PRBool aDecrementRef = PR_TRUE);
    virtual nsIFormManager* GetFormManager() const;
    virtual nsrefcnt GetRefCount() const;
  };
  AggInputControl mControl;
};

#endif
