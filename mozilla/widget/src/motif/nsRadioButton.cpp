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

#include "nsXtEventHandler.h"
#include <Xm/ToggleB.h>
#include <Xm/RowColumn.h>

#define DBG 0
//-------------------------------------------------------------------------
//
// nsRadioButton constructor
//
//-------------------------------------------------------------------------
nsRadioButton::nsRadioButton(nsISupports *aOuter) : 
  nsWindow(aOuter),
  mIsArmed(PR_FALSE)
{
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
// nsRadioButton Creator
//
//-------------------------------------------------------------------------
void nsRadioButton::Create(nsIWidget *aParent,
                      const nsRect &aRect,
                      EVENT_CALLBACK aHandleEventFunction,
                      nsIDeviceContext *aContext,
                      nsIAppShell *aAppShell,
                      nsIToolkit *aToolkit,
                      nsWidgetInitData *aInitData)
{
  Widget parentWidget = nsnull;

  if (DBG) fprintf(stderr, "aParent 0x%x\n", aParent);

  if (aParent) {
    parentWidget = (Widget) aParent->GetNativeData(NS_NATIVE_WIDGET);
  } else {
    parentWidget = (Widget) aAppShell->GetNativeData(NS_NATIVE_SHELL);
  }

  InitToolkit(aToolkit, aParent);
  InitDeviceContext(aContext, parentWidget);

  if (DBG) fprintf(stderr, "Parent 0x%x\n", parentWidget);

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
//                         XmNentryAlignment, XmALIGNMENT_CENTER,
//                         XmNentryVerticalAlignment, XmALIGNMENT_CENTER,
                         XmNisAligned, False,
                         XmNentryBorder, 0,
                         XmNorientation, XmVERTICAL,
                         XmNborderWidth, 0,
                         0);

  mRadioBtn = ::XmCreateToggleButton(mWidget, "", nsnull, 0);

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

  if (DBG) fprintf(stderr, "Button 0x%x  this 0x%x\n", mWidget, this);

  // save the event callback function
  mEventCallback = aHandleEventFunction;

  InitCallbacks();

  XtAddCallback(mRadioBtn,
                XmNarmCallback,
                nsXtWidget_RadioButton_ArmCallback,
                this);

  XtAddCallback(mRadioBtn,
                XmNdisarmCallback,
                nsXtWidget_RadioButton_DisArmCallback,
                this);

  /*XtAddCallback(mRadioBtn,
                XmNvalueChangedCallback,
                nsXtWidget_Toggle_ValueChangedCallback,
                this);*/



}

//-------------------------------------------------------------------------
//
// nsRadioButton Creator
//
//-------------------------------------------------------------------------
void nsRadioButton::Create(nsNativeWidget aParent,
                      const nsRect &aRect,
                      EVENT_CALLBACK aHandleEventFunction,
                      nsIDeviceContext *aContext,
                      nsIAppShell *aAppShell,
                      nsIToolkit *aToolkit,
                      nsWidgetInitData *aInitData)
{
}

//-------------------------------------------------------------------------
//
// Query interface implementation
//
//-------------------------------------------------------------------------
nsresult nsRadioButton::QueryObject(REFNSIID aIID, void** aInstancePtr)
{
  static NS_DEFINE_IID(kIRadioButtonIID,    NS_IRADIOBUTTON_IID);

  if (aIID.Equals(kIRadioButtonIID)) {
    AddRef();
    *aInstancePtr = (void**) &mAggWidget;
    return NS_OK;
  }
  return nsWindow::QueryObject(aIID, aInstancePtr);
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
  mInitialState = XmToggleButtonGetState(mRadioBtn);
  if (DBG) printf("Arm: InitialValue: %d\n", mInitialState);
}

//-------------------------------------------------------------------------
//
// DisArmed
//
//-------------------------------------------------------------------------
void nsRadioButton::DisArmed() 
{
  if (DBG) printf("DisArm: InitialValue: %d\n", mInitialState);
  if (DBG) printf("DisArm: ActualValue:  %d\n", XmToggleButtonGetState(mRadioBtn));
  if (DBG) printf("DisArm: mValueWasSet  %d\n", mValueWasSet);
  if (DBG) printf("DisArm: mNewValue     %d\n", mNewValue);

  if (mValueWasSet) {
    XmToggleButtonSetState(mRadioBtn, mNewValue, TRUE);
  } else {
    XmToggleButtonSetState(mRadioBtn, mInitialState, TRUE);
  }
  mIsArmed = PR_FALSE;
}

//-------------------------------------------------------------------------
//
// Set this button label
//
//-------------------------------------------------------------------------
void nsRadioButton::SetState(PRBool aState) 
{
  int state = aState;
  if (mIsArmed) {
    mNewValue    = aState;
    mValueWasSet = PR_TRUE;
  }
  XmToggleButtonSetState(mRadioBtn, aState, TRUE);
}

//-------------------------------------------------------------------------
//
// Set this button label
//
//-------------------------------------------------------------------------
PRBool nsRadioButton::GetState()
{
  int state = XmToggleButtonGetState(mRadioBtn);
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
void nsRadioButton::SetLabel(const nsString& aText)
{
  NS_ALLOC_STR_BUF(label, aText, 256);
  XmString str;
  str = XmStringCreate(label, XmFONTLIST_DEFAULT_TAG);
  XtVaSetValues(mRadioBtn, XmNlabelString, str, nsnull);
  NS_FREE_STR_BUF(label);
  XmStringFree(str);
}


//-------------------------------------------------------------------------
//
// Get this button label
//
//-------------------------------------------------------------------------
void nsRadioButton::GetLabel(nsString& aBuffer)
{
  XmString str;
  XtVaGetValues(mRadioBtn, XmNlabelString, &str, nsnull);
  char * text;
  if (XmStringGetLtoR(str, XmFONTLIST_DEFAULT_TAG, &text)) {
    aBuffer.SetLength(0);
    aBuffer.Append(text);
    XtFree(text);
  }
  XmStringFree(str);

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

//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------

#define GET_OUTER() \
  ((nsRadioButton*) ((char*)this - nsRadioButton::GetOuterOffset()))

PRBool nsRadioButton::AggRadioButton::GetState()
{
  return GET_OUTER()->GetState();
}

void nsRadioButton::AggRadioButton::SetState(PRBool aState)
{
  GET_OUTER()->SetState(aState);
}

void nsRadioButton::AggRadioButton::SetLabel(const nsString& aText)
{
  GET_OUTER()->SetLabel(aText);
}

void nsRadioButton::AggRadioButton::GetLabel(nsString& aText)
{
  GET_OUTER()->GetLabel(aText);
}


//----------------------------------------------------------------------

BASE_IWIDGET_IMPL(nsRadioButton, AggRadioButton);

