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

#include "nsCheckButton.h"
#include "nsColor.h"
#include "nsGUIEvent.h"
#include "nsString.h"
#include "nsStringUtil.h"

NS_IMPL_ADDREF(nsCheckButton)
NS_IMPL_RELEASE(nsCheckButton)

//-------------------------------------------------------------------------
//
// nsCheckButton constructor
//
//-------------------------------------------------------------------------
nsCheckButton::nsCheckButton() : nsWidget() , nsICheckButton()
{
  NS_INIT_REFCNT();
  mLabel = nsnull;
}

//-------------------------------------------------------------------------
//
// nsCheckButton destructor
//
//-------------------------------------------------------------------------
nsCheckButton::~nsCheckButton()
{
}

//-------------------------------------------------------------------------
//
// nsCheckButton Creator
//
//-------------------------------------------------------------------------
NS_METHOD nsCheckButton::Create(nsIWidget *aParent,
                      const nsRect &aRect,
                      EVENT_CALLBACK aHandleEventFunction,
                      nsIDeviceContext *aContext,
                      nsIAppShell *aAppShell,
                      nsIToolkit *aToolkit,
                      nsWidgetInitData *aInitData)
{
  aParent->AddChild(this);
  GtkWidget *parentWidget = nsnull;

  if (aParent) {
    parentWidget = (GtkWidget *) aParent->GetNativeData(NS_NATIVE_WIDGET);
  } else {
    parentWidget = (GtkWidget *) aAppShell->GetNativeData(NS_NATIVE_SHELL);
  }

  InitToolkit(aToolkit, aParent);
  InitDeviceContext(aContext, parentWidget);

  mWidget = gtk_check_button_new();

  gtk_layout_put(GTK_LAYOUT(parentWidget), mWidget, aRect.x, aRect.y);
  gtk_widget_set_usize(mWidget, aRect.width, aRect.height);

  gtk_widget_show(mWidget);
      
/*
  mWidget = ::XtVaCreateManagedWidget("",
                                    xmToggleButtonWidgetClass,
                                    parentWidget,
                                    XmNwidth, aRect.width,
                                    XmNheight, aRect.height,
                                    XmNrecomputeSize, False,
                                    XmNhighlightOnEnter, False,
                                    XmNx, aRect.x,
                                    XmNy, aRect.y,
                                    XmNresizeHeight, False,
                                    XmNresizeWidth, False,
                                    xmNmarginHeight, 0,
                                    XmNmarginWidth, 0,
                                    XmNadjustMargin, False,
                                    XmNspacing, 0,
                                    XmNisAligned, False,
                                    XmNentryBorder, 0,
                                    XmNborderWidth, 0,
                                    0);
*/
  // save the event callback function
  mEventCallback = aHandleEventFunction;

  InitCallbacks();
/*
  XtAddCallback(mWidget,
                XmNarmCallback,
                nsXtWidget_Toggle_ArmCallback,
                this);

  XtAddCallback(mWidget,
                XmNdisarmCallback,
                nsXtWidget_Toggle_DisArmCallback,
                this);
*/

  return NS_OK;
}

//-------------------------------------------------------------------------
//
// nsCheckButton Creator
//
//-------------------------------------------------------------------------
NS_METHOD nsCheckButton::Create(nsNativeWidget aParent,
                      const nsRect &aRect,
                      EVENT_CALLBACK aHandleEventFunction,
                      nsIDeviceContext *aContext,
                      nsIAppShell *aAppShell,
                      nsIToolkit *aToolkit,
                      nsWidgetInitData *aInitData)
{
  return NS_ERROR_FAILURE;
}

/**
 * Implement the standard QueryInterface for NS_IWIDGET_IID and NS_ISUPPORTS_IID
 * @modify gpk 8/4/98
 * @param aIID The name of the class implementing the method
 * @param _classiiddef The name of the #define symbol that defines the IID
 * for the class (e.g. NS_ISUPPORTS_IID)
 *
*/
nsresult nsCheckButton::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{
    if (NULL == aInstancePtr) {
        return NS_ERROR_NULL_POINTER;
    }

    static NS_DEFINE_IID(kICheckButtonIID, NS_ICHECKBUTTON_IID);
    if (aIID.Equals(kICheckButtonIID)) {
        *aInstancePtr = (void*) ((nsICheckButton*)this);
        AddRef();
        return NS_OK;
    }
    return nsWidget::QueryInterface(aIID,aInstancePtr);
}

//-------------------------------------------------------------------------
//
// Armed
//
//-------------------------------------------------------------------------
void nsCheckButton::Armed()
{
  mIsArmed      = PR_TRUE;
  mValueWasSet  = PR_FALSE;
  mInitialState = GTK_TOGGLE_BUTTON(mWidget)->active;
}

//-------------------------------------------------------------------------
//
// DisArmed
//
//-------------------------------------------------------------------------
void nsCheckButton::DisArmed()
{
  if (mValueWasSet) {
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(mWidget), TRUE);
/*
    XmToggleButtonSetState(mWidget, mNewValue, TRUE);
*/
  } else {
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(mWidget), TRUE);
/*
    XmToggleButtonSetState(mWidget, mInitialState, TRUE);
*/
  }
  mIsArmed = PR_FALSE;
}

//-------------------------------------------------------------------------
//
// Set this button label
//
//-------------------------------------------------------------------------
NS_METHOD nsCheckButton::SetState(const PRBool aState)
{
  int state = aState;
  if (mIsArmed) {
    mNewValue    = aState;
    mValueWasSet = PR_TRUE;
  }
  gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(mWidget), aState);
  return NS_OK;
}

//-------------------------------------------------------------------------
//
// Set this button label
//
//-------------------------------------------------------------------------
NS_METHOD nsCheckButton::GetState(PRBool& aState)
{
  int state = GTK_TOGGLE_BUTTON(mWidget)->active;
  if (mIsArmed) {
    if (mValueWasSet) {
      aState = mNewValue;
    } else {
      aState = state;
    }
  } else {
    aState = state;
  }
  return NS_OK;
}

//-------------------------------------------------------------------------
//
// Set this Checkbox label
//
//-------------------------------------------------------------------------
NS_METHOD nsCheckButton::SetLabel(const nsString& aText)
{
  NS_ALLOC_STR_BUF(label, aText, 256);
  if (mLabel) {
    gtk_label_set(GTK_LABEL(mLabel), label);
  } else {
    mLabel = gtk_label_new(label);
    gtk_container_add(GTK_CONTAINER(mWidget), mLabel);
    gtk_widget_show(mLabel); /* XXX */
  }
  NS_FREE_STR_BUF(label);
  return NS_OK;
}


//-------------------------------------------------------------------------
//
// Get this button label
//
//-------------------------------------------------------------------------
NS_METHOD nsCheckButton::GetLabel(nsString& aBuffer)
{
  char * text;
  if (mLabel) {
    gtk_label_get(GTK_LABEL(mLabel), &text);
    aBuffer.SetLength(0);
    aBuffer.Append(text);
  }
  return NS_OK;
}

//-------------------------------------------------------------------------
//
// move, paint, resizes message - ignore
//
//-------------------------------------------------------------------------
PRBool nsCheckButton::OnMove(PRInt32, PRInt32)
{
  return PR_FALSE;
}

PRBool nsCheckButton::OnPaint(nsPaintEvent &aEvent)
{
  return PR_FALSE;
}

PRBool nsCheckButton::OnResize(nsSizeEvent &aEvent)
{
  return PR_FALSE;
}
