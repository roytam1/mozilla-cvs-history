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

#include "nsRadioButton.h"
#include "nsColor.h"
#include "nsGUIEvent.h"
#include "nsString.h"
#include "nsStringUtil.h"

#include "nsGtkEventHandler.h"
#include <gtk/gtk.h>

NS_IMPL_ADDREF(nsRadioButton)
NS_IMPL_RELEASE(nsRadioButton)

//-------------------------------------------------------------------------
//
// nsRadioButton constructor
//
//-------------------------------------------------------------------------
nsRadioButton::nsRadioButton() : nsWindow(), nsIRadioButton()
{
  NS_INIT_REFCNT();
}


//-------------------------------------------------------------------------
//
// nsRadioButton destructor
//
//-------------------------------------------------------------------------
nsRadioButton::~nsRadioButton()
{
}


//-------------------------------------------------------------------------
//
// Query interface implementation
//
//-------------------------------------------------------------------------
nsresult nsRadioButton::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{
  nsresult result = nsWindow::QueryInterface(aIID, aInstancePtr);

  static NS_DEFINE_IID(kIRadioButtonIID, NS_IRADIOBUTTON_IID);
  if (result == NS_NOINTERFACE && aIID.Equals(kIRadioButtonIID)) {
      *aInstancePtr = (void*) ((nsIRadioButton*)this);
      AddRef();
      result = NS_OK;
  }
  return result;
}


//-------------------------------------------------------------------------
//
// nsRadioButton Creator
//
//-------------------------------------------------------------------------
NS_METHOD nsRadioButton::Create(nsIWidget *aParent,
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
/* FIXME
 * we need to have a slist here, so store 
 * the radio buttons that go with this one.
 */
  mWidget = gtk_radio_button_new(NULL);
/*
  mWidget = ::XmCreateRadioBox(parentWidget, "radio", nsnull, 0);
  XtVaSetValues(mWidget, XmNwidth, aRect.width,
                         XmNheight, aRect.height,
                         XmNx, aRect.x,
                         XmNy, aRect.y,
                         XmNrecomputeSize, False,
                         XmNresizeHeight, False,
                         XmNresizeWidth, False,
                         XmNradioAlwaysOne, False,
                         XmNmarginHeight, 0,
                         XmNmarginWidth, 0,
                         XmNadjustMargin, False,
                         XmNspacing, 0,
                         XmNisAligned, False,
                         XmNentryBorder, 0,
                         XmNorientation, XmVERTICAL,
                         XmNborderWidth, 0,
                         0);

  mRadioBtn = ::XmCreateToggleButton(mWidget, "", nsnull, 0);

// This is goign to be the same as mWidget, not different. FIXME

  XtVaSetValues(mRadioBtn, 
                         XmNwidth, aRect.width,
                         XmNheight, aRect.height,
                         XmNx, 0,
                         XmNy, 0,
                         XmNrecomputeSize, False,
                         XmNresizeHeight, False,
                         XmNresizeWidth, False,
                         XmNmarginHeight, 0,
                         XmNmarginWidth, 0,
                         XmNadjustMargin, False,
                         XmNspacing, 0,
                         XmNisAligned, False,
                         XmNentryBorder, 0,
                         XmNborderWidth, 0,
                         0);

  XtManageChild(mRadioBtn);
*/
  // save the event callback function
  mEventCallback = aHandleEventFunction;

  InitCallbacks();
/*
  XtAddCallback(mRadioBtn,
                XmNarmCallback,
                nsXtWidget_RadioButton_ArmCallback,
                this);

  XtAddCallback(mRadioBtn,
                XmNdisarmCallback,
                nsXtWidget_RadioButton_DisArmCallback,
                this);
*/
  return NS_OK;
}

//-------------------------------------------------------------------------
//
// nsRadioButton Creator
//
//-------------------------------------------------------------------------
NS_METHOD nsRadioButton::Create(nsNativeWidget aParent,
                      const nsRect &aRect,
                      EVENT_CALLBACK aHandleEventFunction,
                      nsIDeviceContext *aContext,
                      nsIAppShell *aAppShell,
                      nsIToolkit *aToolkit,
                      nsWidgetInitData *aInitData)
{
  return NS_ERROR_FAILURE;
}

//-------------------------------------------------------------------------
//
// Armed
//
//-------------------------------------------------------------------------
void nsRadioButton::Armed() 
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
void nsRadioButton::DisArmed() 
{
  if (mValueWasSet) {
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(mWidget), TRUE);
//    XmToggleButtonSetState(mRadioBtn, mNewValue, TRUE);
  } else {
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(mWidget), TRUE);
//    XmToggleButtonSetState(mRadioBtn, mInitialState, TRUE);
  }
  mIsArmed = PR_FALSE;
}

//-------------------------------------------------------------------------
//
// Set this button label
//
//-------------------------------------------------------------------------
NS_METHOD nsRadioButton::SetState(const PRBool aState) 
{
  int state = aState;
  if (mIsArmed) {
    mNewValue    = aState;
    mValueWasSet = PR_TRUE;
  }
//  XmToggleButtonSetState(mRadioBtn, aState, TRUE);
  gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(mWidget), aState);
  return NS_OK;
}

//-------------------------------------------------------------------------
//
// Set this button label
//
//-------------------------------------------------------------------------
NS_METHOD nsRadioButton::GetState(PRBool& aState)
{
//  int state = XmToggleButtonGetState(mRadioBtn);
  int state = GTK_TOGGLE_BUTTON(mWidget)->active;
  if (mIsArmed) {
    if (mValueWasSet) {
      return mNewValue;
    } else {
      return state;
    }
  } else {
    return state;
  }
}

//-------------------------------------------------------------------------
//
// Set this button label
//
//-------------------------------------------------------------------------
NS_METHOD nsRadioButton::SetLabel(const nsString& aText)
{
  NS_ALLOC_STR_BUF(label, aText, 256);
  if (mLabel) {
    gtk_label_set(mLabel, label);
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
NS_METHOD nsRadioButton::GetLabel(nsString& aBuffer)
{
  char * text;
  if (mLabel) {
    gtk_label_get(mLabel, &text);
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
PRBool nsRadioButton::OnMove(PRInt32, PRInt32)
{
  return PR_FALSE;
}

PRBool nsRadioButton::OnPaint(nsPaintEvent &aEvent)
{
  return PR_FALSE;
}

PRBool nsRadioButton::OnResize(nsSizeEvent &aEvent)
{
    return PR_FALSE;
}

