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

#include <gtk/gtk.h>

#include "nsGtkEventHandler.h"

#include "nsButton.h"
#include "nsString.h"

NS_IMPL_ADDREF_INHERITED(nsButton, nsWidget)
NS_IMPL_RELEASE_INHERITED(nsButton, nsWidget)
NS_IMPL_QUERY_INTERFACE2(nsButton, nsIButton, nsIWidget)

//-------------------------------------------------------------------------
//
// nsButton constructor
//
//-------------------------------------------------------------------------
nsButton::nsButton() : nsWidget() , nsIButton()
{
}

//-------------------------------------------------------------------------
//
// Create the native Button widget
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsButton::CreateNative(GtkObject *parentWindow)
{
    if (!GDK_IS_SUPERWIN(parentWindow)) {
#ifdef DEBUG
    g_print("Damn, brother.  That's not a superwin.\n");
#endif
    return NS_ERROR_FAILURE;
  }
  
  GdkSuperWin *superwin = GDK_SUPERWIN(parentWindow);

  mMozBox = gtk_mozbox_new(superwin->bin_window);

  mWidget = gtk_button_new_with_label("");
  gtk_widget_set_name(mWidget, "nsButton");

  // make sure that we put the scrollbar into the mozbox

  gtk_container_add(GTK_CONTAINER(mMozBox), mWidget);

  return NS_OK;
}

//-------------------------------------------------------------------------
//
// nsButton destructor
//
//-------------------------------------------------------------------------
nsButton::~nsButton()
{
}

void nsButton::InitCallbacks(char * aName)
{
  InstallButtonPressSignal(mWidget);
  InstallButtonReleaseSignal(mWidget);

  InstallEnterNotifySignal(mWidget);
  InstallLeaveNotifySignal(mWidget);

  // These are needed so that the events will go to us and not our parent.
  AddToEventMask(mWidget,
                 GDK_BUTTON_PRESS_MASK |
                 GDK_BUTTON_RELEASE_MASK |
                 GDK_ENTER_NOTIFY_MASK |
                 GDK_EXPOSURE_MASK |
                 GDK_FOCUS_CHANGE_MASK |
                 GDK_KEY_PRESS_MASK |
                 GDK_KEY_RELEASE_MASK |
                 GDK_LEAVE_NOTIFY_MASK |
                 GDK_POINTER_MOTION_MASK);
}

//-------------------------------------------------------------------------
//
// Set this button label
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsButton::SetLabel(const nsString& aText)
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
NS_IMETHODIMP nsButton::GetLabel(nsString& aBuffer)
{
  char * text;

  gtk_label_get(GTK_LABEL(GTK_BIN (mWidget)->child), &text);
  aBuffer.SetLength(0);
  aBuffer.AppendWithConversion(text);

  return (NS_OK);

}

//-------------------------------------------------------------------------
//
// set font for button
//
//-------------------------------------------------------------------------
/* virtual */
void nsButton::SetFontNative(GdkFont *aFont)
{
  GtkStyle *style = gtk_style_copy(GTK_BIN (mWidget)->child->style);
  // gtk_style_copy ups the ref count of the font
  gdk_font_unref (style->font);
  
  style->font = aFont;
  gdk_font_ref(style->font);
  
  gtk_widget_set_style(GTK_BIN (mWidget)->child, style);
  
  gtk_style_unref(style);
}
