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

#include "nsButton.h"
#include "nsIButton.h"
#include "nsToolkit.h"
#include "nsColor.h"
#include "nsGUIEvent.h"
#include "nsString.h"
#include "nsStringUtil.h"

#include <gtk/gtk.h>

NS_IMPL_ADDREF(nsButton)
NS_IMPL_RELEASE(nsButton)

//-------------------------------------------------------------------------
//
// nsButton constructor
//
//-------------------------------------------------------------------------
nsButton::nsButton() : nsWindow() , nsIButton()
{
  NS_INIT_REFCNT();
}


NS_METHOD nsButton::Create(nsIWidget        *aParent,
                      const nsRect     &aRect,
                      EVENT_CALLBACK    aHandleEventFunction,
                      nsIDeviceContext *aContext,
                      nsIAppShell      *aAppShell,
                      nsIToolkit       *aToolkit,
                      nsWidgetInitData *aInitData) 
{
  aParent->AddChild(this);
  GtkWidget *parentWidget = nsnull;

  if (aParent) {
    parentWidget = (GtkWidget *) aParent->GetNativeData(NS_NATIVE_WIDGET);
  } else if (aAppShell) {
    parentWidget = (GtkWidget *) aAppShell->GetNativeData(NS_NATIVE_SHELL);
  }

  InitToolkit(aToolkit, aParent);
  InitDeviceContext(aContext, parentWidget);

  mWidget = gtk_button_new_with_label("");
  /*
  mWidget = ::XtVaCreateManagedWidget("button",
                                    xmPushButtonWidgetClass, 
                                    parentWidget,
                                    XmNwidth, aRect.width,
                                    XmNheight, aRect.height,
                                    XmNrecomputeSize, False,
                                    XmNhighlightOnEnter, False,
		                    XmNx, aRect.x,
		                    XmNy, aRect.y, 
                                    nsnull);
*/
  // save the event callback function
  mEventCallback = aHandleEventFunction;

  InitCallbacks("nsButton");
  return NS_OK;

}

NS_METHOD nsButton::Create(nsNativeWidget aParent,
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
// nsButton destructor
//
//-------------------------------------------------------------------------
nsButton::~nsButton()
{
}


/**
 * Implement the standard QueryInterface for NS_IWIDGET_IID and NS_ISUPPORTS_IID
 * @modify gpk 8/4/98
 * @param aIID The name of the class implementing the method
 * @param _classiiddef The name of the #define symbol that defines the IID
 * for the class (e.g. NS_ISUPPORTS_IID)
 * 
*/ 
nsresult nsButton::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{
    if (NULL == aInstancePtr) {
        return NS_ERROR_NULL_POINTER;
    }

    static NS_DEFINE_IID(kIButton, NS_IBUTTON_IID);
    if (aIID.Equals(kIButton)) {
        *aInstancePtr = (void*) ((nsIButton*)this);
        AddRef();
        return NS_OK;
    }

    return nsWindow::QueryInterface(aIID,aInstancePtr);
}


//-------------------------------------------------------------------------
//
// Set this button label
//
//-------------------------------------------------------------------------
NS_METHOD nsButton::SetLabel(const nsString& aText)
{
  NS_ALLOC_STR_BUF(label, aText, 256);

  gtk_label_set(GTK_LABEL(GTK_BIN (mWidget)->child), label);

  NS_FREE_STR_BUF(label);
  return (NS_OK);
}

//-------------------------------------------------------------------------
//
// Get this button label
//
//-------------------------------------------------------------------------
NS_METHOD nsButton::GetLabel(nsString& aBuffer)
{
  char * text;
  
  gtk_label_get(GTK_LABEL(GTK_BIN (mWidget)->child), &text);
  aBuffer.SetLength(0);
  aBuffer.Append(text);

  return (NS_OK);

}

//-------------------------------------------------------------------------
//
// paint message. Don't send the paint out
//
//-------------------------------------------------------------------------
PRBool nsButton::OnMove(PRInt32, PRInt32)
{
  return PR_FALSE;
}

PRBool nsButton::OnPaint(nsPaintEvent &aEvent)
{
  return PR_FALSE;
}

PRBool nsButton::OnResize(nsSizeEvent &aEvent)
{
  return PR_FALSE;
}


