/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-

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
#include "nsGUIEvent.h"

#include "nsGtkEventHandler.h"

NS_IMPL_ADDREF_INHERITED(nsScrollbar, nsWidget)
NS_IMPL_RELEASE_INHERITED(nsScrollbar, nsWidget)
NS_IMPL_QUERY_INTERFACE2(nsScrollbar, nsIScrollbar, nsIWidget)

//-------------------------------------------------------------------------
//
// nsScrollbar constructor
//
//-------------------------------------------------------------------------
nsScrollbar::nsScrollbar (PRBool aIsVertical):nsWidget (), nsIScrollbar ()
{
  NS_INIT_REFCNT ();

  mOrientation = (aIsVertical) ?
    GTK_ORIENTATION_VERTICAL : GTK_ORIENTATION_HORIZONTAL;
}

//-------------------------------------------------------------------------
//
// nsScrollbar destructor
//
//-------------------------------------------------------------------------
nsScrollbar::~nsScrollbar ()
{
}

//-------------------------------------------------------------------------
//
// Create the native scrollbar widget
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsScrollbar::CreateNative (GtkWidget * parentWindow)
{
  // Create scrollbar, random default values
  mAdjustment = GTK_ADJUSTMENT (gtk_adjustment_new (0, 0, 100, 1, 25, 25));

  switch (mOrientation)
  {
    case GTK_ORIENTATION_HORIZONTAL:
      mWidget = gtk_hscrollbar_new (mAdjustment);
      break;
    case GTK_ORIENTATION_VERTICAL:
      mWidget = gtk_vscrollbar_new (mAdjustment);
      break;
  }

  gtk_widget_set_name (mWidget, "nsScrollbar");

  gtk_signal_connect (GTK_OBJECT (mAdjustment),
                      "value_changed",
                      GTK_SIGNAL_FUNC (handle_scrollbar_value_changed),
                      this);
  gtk_signal_connect (GTK_OBJECT (mAdjustment),
                      "destroy",
                      GTK_SIGNAL_FUNC (DestroySignal),
                      this);

  return NS_OK;
}

void
nsScrollbar::OnDestroySignal(GtkWidget* aGtkWidget)
{
  if ((void*)aGtkWidget == (void*)mAdjustment) {
    mAdjustment = nsnull;
  }
  else {
    nsWidget::OnDestroySignal(aGtkWidget);
  }
}

//-------------------------------------------------------------------------
//
// Define the range settings
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsScrollbar::SetMaxRange (PRUint32 aEndRange)
{
  if (mAdjustment) {
    GTK_ADJUSTMENT (mAdjustment)->upper = (float) aEndRange;
    gtk_signal_emit_by_name (GTK_OBJECT (mAdjustment), "changed");
  }
  return NS_OK;
}


//-------------------------------------------------------------------------
//
// Return the range settings
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsScrollbar::GetMaxRange (PRUint32 & aMaxRange)
{
  if (mAdjustment)
    aMaxRange = (PRUint32) GTK_ADJUSTMENT (mAdjustment)->upper;
  else
    aMaxRange = 0;

  return NS_OK;
}


//-------------------------------------------------------------------------
//
// Set the thumb position
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsScrollbar::SetPosition (PRUint32 aPos)
{
//   if (mAdjustment)
//     gtk_adjustment_set_value (GTK_ADJUSTMENT (mAdjustment), (float) aPos);

  if (mAdjustment && mWidget)
  {
    //
    // The following bit of code borrowed from gtkrange.c,
    // gtk_range_adjustment_value_changed():
    //
    // Ok, so, like, the problem is that the view manager expects
    // SetPosition() to simply do that - set the position of the 
    // scroll bar.  Nothing else!
    //
    // Unfortunately, calling gtk_adjustment_set_value() causes
    // the adjustment object (mAdjustment) to emit a 
    // "value_changed" signal which in turn causes the
    // scrollbar widget (mWidget) to scroll to the given position.
    //
    // The net result of this is that the content is scrolled
    // twice, once by the view manager and once by the 
    // scrollbar - and things get messed up from then onwards.
    //
    // The following bit of code does the equivalent of 
    // gtk_adjustment_set_value(), except no signal is emitted.
    //
    GtkRange * range = GTK_RANGE(mWidget);
    GtkAdjustment * adjustment = GTK_ADJUSTMENT(mAdjustment);
      
    adjustment->value = (float) aPos;
      
    if (range->old_value != adjustment->value)
    {
      gtk_range_slider_update (range);
      gtk_range_clear_background (range);
      
      range->old_value = adjustment->value;
    }
  }

  return NS_OK;
}


//-------------------------------------------------------------------------
//
// Get the current thumb position.
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsScrollbar::GetPosition (PRUint32 & aPos)
{
  if (mAdjustment)
    aPos = (PRUint32) GTK_ADJUSTMENT (mAdjustment)->value;
  else
    aPos = 0;

  return NS_OK;
}


//-------------------------------------------------------------------------
//
// Set the thumb size
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsScrollbar::SetThumbSize (PRUint32 aSize)
{
  if (aSize > 0)
    {
      if (mAdjustment) {
        GTK_ADJUSTMENT (mAdjustment)->page_increment = (float) aSize;
        GTK_ADJUSTMENT (mAdjustment)->page_size = (float) aSize;
        gtk_signal_emit_by_name (GTK_OBJECT (mAdjustment), "changed");
      }
    }
  return NS_OK;
}


//-------------------------------------------------------------------------
//
// Get the thumb size
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsScrollbar::GetThumbSize (PRUint32 & aThumbSize)
{
  if (mAdjustment)
    aThumbSize = (PRUint32) GTK_ADJUSTMENT (mAdjustment)->page_size;
  else
    aThumbSize = 0;

  return NS_OK;
}


//-------------------------------------------------------------------------
//
// Set the line increment for this scrollbar
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsScrollbar::SetLineIncrement (PRUint32 aLineIncrement)
{
  if (aLineIncrement > 0)
    {
      if (mAdjustment) {
        GTK_ADJUSTMENT (mAdjustment)->step_increment = (float) aLineIncrement;
        gtk_signal_emit_by_name (GTK_OBJECT (mAdjustment), "changed");
      }
    }
  return NS_OK;
}


//-------------------------------------------------------------------------
//
// Get the line increment for this scrollbar
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsScrollbar::GetLineIncrement (PRUint32 & aLineInc)
{
  if (mAdjustment) {
    aLineInc = (PRUint32) GTK_ADJUSTMENT (mAdjustment)->step_increment;
  }
  else
    aLineInc = 0;

  return NS_OK;
}


//-------------------------------------------------------------------------
//
// Set all scrolling parameters
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsScrollbar::SetParameters (PRUint32 aMaxRange, PRUint32 aThumbSize,
	       PRUint32 aPosition, PRUint32 aLineIncrement)
{
  if (mAdjustment) {
    int thumbSize = (((int) aThumbSize) > 0 ? aThumbSize : 1);
    int maxRange = (((int) aMaxRange) > 0 ? aMaxRange : 10);
    int mLineIncrement = (((int) aLineIncrement) > 0 ? aLineIncrement : 1);

    int maxPos = maxRange - thumbSize;
    int pos = ((int) aPosition) > maxPos ? maxPos - 1 : ((int) aPosition);

    GTK_ADJUSTMENT (mAdjustment)->lower = 0;
    GTK_ADJUSTMENT (mAdjustment)->upper = maxRange;
    GTK_ADJUSTMENT (mAdjustment)->page_size = thumbSize;
    GTK_ADJUSTMENT (mAdjustment)->page_increment = thumbSize;
    GTK_ADJUSTMENT (mAdjustment)->step_increment = mLineIncrement;
    // this will emit the changed signal for us
    gtk_adjustment_set_value (GTK_ADJUSTMENT (mAdjustment), pos);
  }
  return NS_OK;
}

//-------------------------------------------------------------------------
int nsScrollbar::AdjustScrollBarPosition (int aPosition)
{
  return 0;			/* XXX */
}

//-------------------------------------------------------------------------
//
// Deal with scrollbar messages (actually implemented only in nsScrollbar)
//
//-------------------------------------------------------------------------
PRBool nsScrollbar::OnScroll (nsScrollbarEvent & aEvent, PRUint32 cPos)
{
  PRBool result = PR_TRUE;
  float newPosition;

  switch (aEvent.message)
    {

      // scroll one line right or down
    case NS_SCROLLBAR_LINE_NEXT:
      {
        newPosition = GTK_ADJUSTMENT (mAdjustment)->value;
        // newPosition += mLineIncrement;
        newPosition += 10;
        PRUint32 thumbSize;
        PRUint32 maxRange;
        GetThumbSize (thumbSize);
        GetMaxRange (maxRange);
        PRUint32 max = maxRange - thumbSize;
        if (newPosition > (int) max)
          newPosition = (int) max;

        // if an event callback is registered, give it the chance
        // to change the increment
        if (mEventCallback)
          {
            aEvent.position = (PRUint32) newPosition;
            result = ConvertStatus ((*mEventCallback) (&aEvent));
            newPosition = aEvent.position;
          }
        break;
      }


      // scroll one line left or up
    case NS_SCROLLBAR_LINE_PREV:
      {
        newPosition = GTK_ADJUSTMENT (mAdjustment)->value;

        // newPosition -= mLineIncrement;
        newPosition -= 10;
        if (newPosition < 0)
          newPosition = 0;

        // if an event callback is registered, give it the chance
        // to change the decrement
        if (mEventCallback)
          {
            aEvent.position = (PRUint32) newPosition;
            aEvent.widget = (nsWidget *) this;
            result = ConvertStatus ((*mEventCallback) (&aEvent));
            newPosition = aEvent.position;
          }
        break;
      }

      // Scrolls one page right or down
    case NS_SCROLLBAR_PAGE_NEXT:
      {
        newPosition = GTK_ADJUSTMENT (mAdjustment)->value;
        PRUint32 thumbSize;
        GetThumbSize (thumbSize);
        PRUint32 maxRange;
        GetThumbSize (thumbSize);
        GetMaxRange (maxRange);
        PRUint32 max = maxRange - thumbSize;
        if (newPosition > (int) max)
          newPosition = (int) max;

        // if an event callback is registered, give it the chance
        // to change the increment
        if (mEventCallback)
          {
            aEvent.position = (PRUint32) newPosition;
            result = ConvertStatus ((*mEventCallback) (&aEvent));
            newPosition = aEvent.position;
          }
        break;
      }

      // Scrolls one page left or up.
    case NS_SCROLLBAR_PAGE_PREV:
      {
        newPosition = GTK_ADJUSTMENT (mAdjustment)->value;
        if (newPosition < 0)
          newPosition = 0;

        // if an event callback is registered, give it the chance
        // to change the increment
        if (mEventCallback)
          {
            aEvent.position = (PRUint32) newPosition;
            result = ConvertStatus ((*mEventCallback) (&aEvent));
            newPosition = aEvent.position;
          }
        break;
      }


      // Scrolls to the absolute position. The current position is specified by
      // the cPos parameter.
    case NS_SCROLLBAR_POS:
      {
        newPosition = cPos;

        // if an event callback is registered, give it the chance
        // to change the increment
        if (mEventCallback)
          {
            aEvent.position = (PRUint32) newPosition;
            result = ConvertStatus ((*mEventCallback) (&aEvent));
            newPosition = aEvent.position;
          }
        break;
      }
    }
  /*
     GTK_ADJUSTMENT(mAdjustment)->value = newPosition;
     gtk_signal_emit_by_name(GTK_OBJECT(mAdjustment), "value_changed");
   */
  /*
     if (mEventCallback) {
     aEvent.position = cPos;
     result = ConvertStatus((*mEventCallback)(&aEvent));
     newPosition = aEvent.position;
     }
   */
  return result;
}
