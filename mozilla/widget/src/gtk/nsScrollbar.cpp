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

#include <gtk/gtk.h>

#include "nsScrollbar.h"
#include "nsToolkit.h"
#include "nsGUIEvent.h"
#include "nsUnitConversion.h"

#include "nsGtkEventHandler.h"

NS_IMPL_ADDREF(nsScrollbar)
NS_IMPL_RELEASE(nsScrollbar)

//-------------------------------------------------------------------------
//
// nsScrollbar constructor
//
//-------------------------------------------------------------------------
nsScrollbar::nsScrollbar(PRBool aIsVertical) : nsWidget(), nsIScrollbar()
{
  NS_INIT_REFCNT();

  mOrientation  = (aIsVertical) ?
    GTK_ORIENTATION_VERTICAL : GTK_ORIENTATION_HORIZONTAL;
}

//-------------------------------------------------------------------------
//
// Create
//
//-------------------------------------------------------------------------
NS_METHOD nsScrollbar::Create(nsIWidget* aParent,
                      const nsRect &aRect,
                      EVENT_CALLBACK aHandleEventFunction,
                      nsIDeviceContext *aContext,
                      nsIAppShell *aAppShell,
                      nsIToolkit *aToolkit,
                      nsWidgetInitData *aInitData)
{
  GtkWidget *parentWidget = nsnull;

  // handle parent stuff
  if (aParent) {
    aParent->AddChild(this);
    parentWidget = (GtkWidget*) aParent->GetNativeData(NS_NATIVE_WIDGET);
  } else if (aAppShell) {
    parentWidget = (GtkWidget*) aAppShell->GetNativeData(NS_NATIVE_SHELL);
  }

  InitToolkit(aToolkit, aParent);
  InitDeviceContext(aContext, parentWidget);

  // Create scrollbar, random default values
  mAdjustment = gtk_adjustment_new(0, 0, 100, 1, 25, 25);

  if (mOrientation == GTK_ORIENTATION_HORIZONTAL) {
    mWidget = gtk_hscrollbar_new(GTK_ADJUSTMENT(mAdjustment));
  } else {
    mWidget = gtk_vscrollbar_new(GTK_ADJUSTMENT(mAdjustment));
  }

  // add to layout, set size
  gtk_layout_put(GTK_LAYOUT(parentWidget), mWidget, aRect.x, aRect.y);
  gtk_widget_set_usize(mWidget, aRect.width, aRect.height);

  gtk_widget_show(mWidget);

  // save the event callback function
  mEventCallback = aHandleEventFunction;

  InitCallbacks("nsScrollbar");
  return NS_OK;
}

NS_METHOD nsScrollbar::Create(nsNativeWidget aParent,
                      const nsRect &aRect,
                      EVENT_CALLBACK aHandleEventFunction,
                      nsIDeviceContext *aContext,
                      nsIAppShell *aAppShell,
                      nsIToolkit *aToolkit,
                      nsWidgetInitData *aInitData)
{
  // not yet implemented
  return NS_ERROR_FAILURE;
}

//-------------------------------------------------------------------------
//
// nsScrollbar destructor
//
//-------------------------------------------------------------------------
nsScrollbar::~nsScrollbar()
{
}

//-------------------------------------------------------------------------
//
// Query interface implementation
//
//-------------------------------------------------------------------------
nsresult nsScrollbar::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{
  if (aInstancePtr == NULL) {
    return NS_ERROR_NULL_POINTER;
  }

  // get parent's interface
  nsresult result = nsWidget::QueryInterface(aIID, aInstancePtr);

  static NS_DEFINE_IID(kInsScrollbarIID, NS_ISCROLLBAR_IID);
  // if not asking for parent, check for our interface IID
  if (result == NS_NOINTERFACE && aIID.Equals(kInsScrollbarIID)) {
    *aInstancePtr = (void*) ((nsIScrollbar*)this);
    AddRef();
    result = NS_OK;
  }

  return result;
}

//-------------------------------------------------------------------------
//
// Define the range settings
//
//-------------------------------------------------------------------------
NS_METHOD nsScrollbar::SetMaxRange(PRUint32 aEndRange)
{
  GTK_ADJUSTMENT(mAdjustment)->upper = aEndRange;
  gtk_signal_emit_by_name(GTK_OBJECT(mAdjustment), "changed");
  return NS_OK;
}


//-------------------------------------------------------------------------
//
// Return the range settings
//
//-------------------------------------------------------------------------
NS_METHOD nsScrollbar::GetMaxRange(PRUint32 & aMaxRange)
{
  aMaxRange = (PRUint32) GTK_ADJUSTMENT(mAdjustment)->upper;
  return NS_OK;
}


//-------------------------------------------------------------------------
//
// Set the thumb position
//
//-------------------------------------------------------------------------
NS_METHOD nsScrollbar::SetPosition(PRUint32 aPos)
{
  gtk_adjustment_set_value(GTK_ADJUSTMENT(mAdjustment), aPos);
  return NS_OK;
}


//-------------------------------------------------------------------------
//
// Get the current thumb position.
//
//-------------------------------------------------------------------------
NS_METHOD nsScrollbar::GetPosition(PRUint32 & aPos)
{
  aPos = (PRUint32) GTK_ADJUSTMENT(mAdjustment)->value;
  return NS_OK;
}


//-------------------------------------------------------------------------
//
// Set the thumb size
//
//-------------------------------------------------------------------------
NS_METHOD nsScrollbar::SetThumbSize(PRUint32 aSize)
{
  if (aSize > 0) {
    GTK_ADJUSTMENT(mAdjustment)->page_increment = aSize;
    GTK_ADJUSTMENT(mAdjustment)->page_size = aSize;
    gtk_signal_emit_by_name(GTK_OBJECT(mAdjustment), "changed");
  }
  return NS_OK;
}


//-------------------------------------------------------------------------
//
// Get the thumb size
//
//-------------------------------------------------------------------------
NS_METHOD nsScrollbar::GetThumbSize(PRUint32 & aThumbSize)
{
  aThumbSize = (PRUint32) GTK_ADJUSTMENT(mAdjustment)->page_size;
  return NS_OK;
}


//-------------------------------------------------------------------------
//
// Set the line increment for this scrollbar
//
//-------------------------------------------------------------------------
NS_METHOD nsScrollbar::SetLineIncrement(PRUint32 aLineIncrement)
{
  if (aLineIncrement > 0) {
    GTK_ADJUSTMENT(mAdjustment)->step_increment = aLineIncrement;
    gtk_signal_emit_by_name(GTK_OBJECT(mAdjustment), "changed");
  }
  return NS_OK;
}


//-------------------------------------------------------------------------
//
// Get the line increment for this scrollbar
//
//-------------------------------------------------------------------------
NS_METHOD nsScrollbar::GetLineIncrement(PRUint32 & aLineInc)
{
  aLineInc = (PRUint32) GTK_ADJUSTMENT(mAdjustment)->step_increment;
  return NS_OK;
}


//-------------------------------------------------------------------------
//
// Set all scrolling parameters
//
//-------------------------------------------------------------------------
NS_METHOD nsScrollbar::SetParameters(PRUint32 aMaxRange, PRUint32 aThumbSize,
                                PRUint32 aPosition, PRUint32 aLineIncrement)
{
  int thumbSize = (((int)aThumbSize) > 0?aThumbSize:1);
  int maxRange  = (((int)aMaxRange) > 0?aMaxRange:10);
  int mLineIncrement = (((int)aLineIncrement) > 0?aLineIncrement:1);
  
  int maxPos = maxRange - thumbSize;
  int pos    = ((int)aPosition) > maxPos ? maxPos-1 : ((int)aPosition);

  GTK_ADJUSTMENT(mAdjustment)->lower = 0;
  GTK_ADJUSTMENT(mAdjustment)->upper = maxRange;
  GTK_ADJUSTMENT(mAdjustment)->page_size = thumbSize;
  GTK_ADJUSTMENT(mAdjustment)->page_increment = thumbSize;
  GTK_ADJUSTMENT(mAdjustment)->step_increment = mLineIncrement;
  // this will emit the changed signal for us
  gtk_adjustment_set_value(GTK_ADJUSTMENT(mAdjustment), pos);  
  return NS_OK;
}


//-------------------------------------------------------------------------
//
// paint message. Don't send the paint out
//
//-------------------------------------------------------------------------
PRBool nsScrollbar::OnPaint(nsPaintEvent & aEvent)
{
    return PR_FALSE;
}


PRBool nsScrollbar::OnResize(nsSizeEvent &aEvent)
{
#if 0
  return nsWindow::OnResize(aEvent);
#endif
}

//-------------------------------------------------------------------------
int nsScrollbar::AdjustScrollBarPosition(int aPosition)
{
  int maxRange;
  int sliderSize;
#if 0
  XtVaGetValues(mWidget, XmNmaximum, &maxRange,
                         XmNsliderSize, &sliderSize,
                         nsnull);
  int cap = maxRange - sliderSize;
  return aPosition > cap ? cap : aPosition;
#endif
}

//-------------------------------------------------------------------------
//
// Deal with scrollbar messages (actually implemented only in nsScrollbar)
//
//-------------------------------------------------------------------------
PRBool nsScrollbar::OnScroll(nsScrollbarEvent & aEvent, PRUint32 cPos)
{
#if 0
    PRBool result = PR_TRUE;
    int newPosition;

    switch (aEvent.message) {

        // scroll one line right or down
        case NS_SCROLLBAR_LINE_NEXT:
        {
            XtVaGetValues(mWidget, XmNvalue, &newPosition, nsnull);
            newPosition += mLineIncrement;
            PRUint32 thumbSize;
            PRUint32 maxRange;
            GetThumbSize(thumbSize);
            GetMaxRange(maxRange);
            PRUint32 max = maxRange - thumbSize;
            if (newPosition > (int)max)
                newPosition = (int)max;

            // if an event callback is registered, give it the chance
            // to change the increment
            if (mEventCallback) {
                aEvent.position = newPosition;
                result = ConvertStatus((*mEventCallback)(&aEvent));
                newPosition = aEvent.position;
            }

            XtVaSetValues(mWidget, XmNvalue,
                          AdjustScrollBarPosition(newPosition), nsnull);
            break;
        }


        // scroll one line left or up
        case NS_SCROLLBAR_LINE_PREV:
        {
            XtVaGetValues(mWidget, XmNvalue, &newPosition, nsnull);

            newPosition -= mLineIncrement;
            if (newPosition < 0)
                newPosition = 0;

            // if an event callback is registered, give it the chance
            // to change the decrement
            if (mEventCallback) {
                aEvent.position = newPosition;

                result = ConvertStatus((*mEventCallback)(&aEvent));
                newPosition = aEvent.position;
            }

            XtVaSetValues(mWidget, XmNvalue, newPosition, nsnull);

            break;
        }

        // Scrolls one page right or down
        case NS_SCROLLBAR_PAGE_NEXT:
        {
            XtVaGetValues(mWidget, XmNvalue, &newPosition, nsnull);
            PRUint32 thumbSize;
            GetThumbSize(thumbSize);
            PRUint32 maxRange;
            GetThumbSize(thumbSize);
            GetMaxRange(maxRange);
            PRUint32 max = maxRange - thumbSize;
            if (newPosition > (int)max)
                newPosition = (int)max;

            // if an event callback is registered, give it the chance
            // to change the increment
            if (mEventCallback) {
                aEvent.position = newPosition;
                result = ConvertStatus((*mEventCallback)(&aEvent));
                newPosition = aEvent.position;
            }
            XtVaSetValues(mWidget, XmNvalue,
                          AdjustScrollBarPosition(newPosition+10), nsnull);
            break;
        }

        // Scrolls one page left or up.
        case NS_SCROLLBAR_PAGE_PREV:
        {
            XtVaGetValues(mWidget, XmNvalue, &newPosition, nsnull);
            if (newPosition < 0)
                newPosition = 0;

            // if an event callback is registered, give it the chance
            // to change the increment
            if (mEventCallback) {
                aEvent.position = newPosition;
                result = ConvertStatus((*mEventCallback)(&aEvent));
                newPosition = aEvent.position;
            }

            XtVaSetValues(mWidget, XmNvalue, newPosition-10, nsnull);
            break;
        }


        // Scrolls to the absolute position. The current position is specified by
        // the cPos parameter.
        case NS_SCROLLBAR_POS:
        {
            newPosition = cPos;

            // if an event callback is registered, give it the chance
            // to change the increment
            if (mEventCallback) {
                aEvent.position = newPosition;
                result = ConvertStatus((*mEventCallback)(&aEvent));
                newPosition = aEvent.position;
            }

            XtVaSetValues(mWidget, XmNvalue,
                          AdjustScrollBarPosition(newPosition), nsnull);

            break;
        }
    }
    return result;
#endif
}
