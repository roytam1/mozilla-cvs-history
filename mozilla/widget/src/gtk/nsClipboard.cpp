/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
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
 * Copyright (C) 1999-2000 Netscape Communications Corporation.
 * All Rights Reserved.
 * 
 * Contributor(s):
 *   Stuart Parmenter <pavlov@netscape.com>
 *   Mike Pinkerton   <pinkerton@netscape.com>
 */

#include "nsClipboard.h"

#include <gdk/gdkx.h>
#include <X11/Xlib.h>

#include "nsCOMPtr.h"
#include "nsFileSpec.h"
#include "nsCRT.h"
#include "nsISupportsArray.h"
#include "nsISupportsPrimitives.h"

#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsWidgetsCID.h"
#include "nsXPIDLString.h"
#include "nsPrimitiveHelpers.h"

#include "nsTextFormatter.h"
#include "nsVoidArray.h"


#include "nsIServiceManager.h"
#include "nsICharsetConverterManager.h"
// unicode conversion
#define NS_IMPL_IDS
#  include "nsIPlatformCharset.h"
#undef NS_IMPL_IDS


// The class statics:
GtkWidget* nsClipboard::sWidget = 0;

static GdkAtom GDK_SELECTION_CLIPBOARD;

NS_IMPL_ISUPPORTS1(nsClipboard, nsIClipboard);

//-------------------------------------------------------------------------
//
// nsClipboard constructor
//
//-------------------------------------------------------------------------
nsClipboard::nsClipboard()
{
#ifdef DEBUG_CLIPBOARD
  g_print("nsClipboard::nsClipboard()\n");
#endif /* DEBUG_CLIPBOARD */

  NS_INIT_REFCNT();
  mIgnoreEmptyNotification = PR_FALSE;
  mGlobalTransferable = nsnull;
  mSelectionTransferable = nsnull;
  mGlobalOwner = nsnull;
  mSelectionOwner = nsnull;
  mSelectionData.data = nsnull;
  mSelectionData.length = 0;

  // initialize the widget, etc we're binding to
  Init();
}

// XXX if GTK's internal code changes this isn't going to work
// copied from gtk code because it is a static function we can't get to it.
// need to bug owen taylor about getting this code public.

typedef struct _GtkSelectionTargetList GtkSelectionTargetList;

struct _GtkSelectionTargetList {
  GdkAtom selection;
  GtkTargetList *list;
};

static const char *gtk_selection_handler_key = "gtk-selection-handlers";

void __gtk_selection_target_list_remove (GtkWidget *widget, GdkAtom selection)
{
  GtkSelectionTargetList *sellist;
  GList *tmp_list, *tmp_list2;
  GList *lists;
  lists = (GList*)gtk_object_get_data (GTK_OBJECT (widget), gtk_selection_handler_key);
  tmp_list = lists;
  while (tmp_list) {
    sellist = (GtkSelectionTargetList*)tmp_list->data;
    if (sellist->selection == selection) {
      gtk_target_list_unref (sellist->list);
      g_free (sellist);
      tmp_list->data = nsnull;
      tmp_list2 = tmp_list->prev;
      lists = g_list_remove_link(lists, tmp_list);
      g_list_free_1(tmp_list);
      tmp_list = tmp_list2;
    }
    if (tmp_list)
      tmp_list = tmp_list->next;
  }
  gtk_object_set_data(GTK_OBJECT(widget), gtk_selection_handler_key, lists);
}

//-------------------------------------------------------------------------
//
// nsClipboard destructor
//
//-------------------------------------------------------------------------
nsClipboard::~nsClipboard()
{
#ifdef DEBUG_CLIPBOARD
  g_print("nsClipboard::~nsClipboard()\n");  
#endif /* DEBUG_CLIPBOARD */

  // Remove all our event handlers:
  if (sWidget) {
    if (gdk_selection_owner_get(GDK_SELECTION_PRIMARY) == sWidget->window)
      gtk_selection_remove_all(sWidget);

    if (gdk_selection_owner_get(GDK_SELECTION_CLIPBOARD) == sWidget->window)
      gtk_selection_remove_all(sWidget);
  }

  // free the selection data, if any
  if (mSelectionData.data != nsnull)
    nsMemory::Free(mSelectionData.data);

  gtk_object_remove_data(GTK_OBJECT(sWidget), "cb");

  if (sWidget) {
    gtk_widget_unref(sWidget);
    sWidget = nsnull;
  }
}


//-------------------------------------------------------------------------
void nsClipboard::Init(void)
{
#ifdef DEBUG_CLIPBOARD
  g_print("nsClipboard::Init\n");
#endif

  GDK_SELECTION_CLIPBOARD = gdk_atom_intern("CLIPBOARD", FALSE);

  // create invisible widget to use for the clipboard
  sWidget = gtk_invisible_new();

  // add the clipboard pointer to the widget so we can get it.
  gtk_object_set_data(GTK_OBJECT(sWidget), "cb", this);

  // Handle selection requests if we called gtk_selection_add_target:
  gtk_signal_connect(GTK_OBJECT(sWidget), "selection_get",
                     GTK_SIGNAL_FUNC(nsClipboard::SelectionGetCB),
                     nsnull);

  // When someone else takes the selection away:
  gtk_signal_connect(GTK_OBJECT(sWidget), "selection_clear_event",
                     GTK_SIGNAL_FUNC(nsClipboard::SelectionClearCB),
                     nsnull);

  // Set up the paste handler:
  gtk_signal_connect(GTK_OBJECT(sWidget), "selection_received",
                     GTK_SIGNAL_FUNC(nsClipboard::SelectionReceivedCB),
                     nsnull);
}




/**
  * Sets the transferable object
  *
  */
NS_IMETHODIMP nsClipboard::SetData(nsITransferable * aTransferable,
                                   nsIClipboardOwner * anOwner,
                                   PRInt32 aWhichClipboard)
{

  if ((aTransferable == mGlobalTransferable.get() &&
       anOwner == mGlobalOwner.get() &&
       aWhichClipboard == kGlobalClipboard ) ||
      (aTransferable == mSelectionTransferable.get() &&
       anOwner == mSelectionOwner.get() &&
       aWhichClipboard == kSelectionClipboard)
      )
  {
    return NS_OK;
  }

  EmptyClipboard(aWhichClipboard);

  switch(aWhichClipboard) {
  case kSelectionClipboard:
    mSelectionOwner = anOwner;
    mSelectionTransferable = aTransferable;
    break;
  case kGlobalClipboard:
    mGlobalOwner = anOwner;
    mGlobalTransferable = aTransferable;
    break;
  }

  return SetNativeClipboardData(aWhichClipboard);
}

/**
  * Gets the transferable object
  *
  */
NS_IMETHODIMP nsClipboard::GetData(nsITransferable * aTransferable, PRInt32 aWhichClipboard)
{
  if (nsnull != aTransferable) {
    return GetNativeClipboardData(aTransferable, aWhichClipboard);
  } else {
    printf("  nsClipboard::GetData(), aTransferable is NULL.\n");
  }

  return NS_ERROR_FAILURE;
}


/**
  * 
  *
  */
NS_IMETHODIMP nsClipboard::EmptyClipboard(PRInt32 aWhichClipboard)
{
  if (mIgnoreEmptyNotification) {
    return NS_OK;
  }

  switch(aWhichClipboard) {
  case kSelectionClipboard:
    if (mSelectionOwner) {
      mSelectionOwner->LosingOwnership(mSelectionTransferable);
      mSelectionOwner = nsnull;
    }
    mSelectionTransferable = nsnull;
    break;
  case kGlobalClipboard:
    if (mGlobalOwner) {
      mGlobalOwner->LosingOwnership(mGlobalTransferable);
      mGlobalOwner = nsnull;
    }
    mGlobalTransferable = nsnull;
    break;
  }

  return NS_OK;
}

NS_IMETHODIMP nsClipboard::SupportsSelectionClipboard(PRBool *_retval)
{
  *_retval = PR_TRUE;   // we don't suport the selection clipboard by default.
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_IMETHODIMP nsClipboard::SetNativeClipboardData(PRInt32 aWhichClipboard)
{
  mIgnoreEmptyNotification = PR_TRUE;

#ifdef DEBUG_CLIPBOARD
  g_print("  nsClipboard::SetNativeClipboardData(%i)\n", aWhichClipboard);
#endif /* DEBUG_CLIPBOARD */


  GdkAtom selectionAtom = GetSelectionAtom(aWhichClipboard);
  nsCOMPtr<nsITransferable> transferable(getter_AddRefs(GetTransferable(aWhichClipboard)));

  // make sure we have a good transferable
  if (nsnull == transferable) {
    printf("nsClipboard::SetNativeClipboardData(): no transferable!\n");
    return NS_ERROR_FAILURE;
  }

  // are we already the owner?
  if (gdk_selection_owner_get(selectionAtom) == sWidget->window)
  {
    // if so, clear all the targets
    __gtk_selection_target_list_remove(sWidget, selectionAtom);
    //    gtk_selection_remove_all(sWidget);
  }

  // we arn't already the owner, so we will become it
  gint have_selection = gtk_selection_owner_set(sWidget,
                                                selectionAtom,
                                                GDK_CURRENT_TIME);
  if (have_selection == 0)
    return NS_ERROR_FAILURE;

  // get flavor list that includes all flavors that can be written (including ones 
  // obtained through conversion)
  nsCOMPtr<nsISupportsArray> flavorList;
  nsresult errCode = transferable->FlavorsTransferableCanExport ( getter_AddRefs(flavorList) );
  if ( NS_FAILED(errCode) )
    return NS_ERROR_FAILURE;

  PRUint32 cnt;
  flavorList->Count(&cnt);
  for ( PRUint32 i=0; i<cnt; ++i )
  {
    nsCOMPtr<nsISupports> genericFlavor;
    flavorList->GetElementAt ( i, getter_AddRefs(genericFlavor) );
    nsCOMPtr<nsISupportsString> currentFlavor ( do_QueryInterface(genericFlavor) );
    if ( currentFlavor ) {
      nsXPIDLCString flavorStr;
      currentFlavor->ToString(getter_Copies(flavorStr));

      // add these types as selection targets
      RegisterFormat(flavorStr, selectionAtom);
    }
  }

  mIgnoreEmptyNotification = PR_FALSE;

  return NS_OK;
}


PRBool nsClipboard::DoRealConvert(GdkAtom type, GdkAtom aSelectionAtom)
{
#ifdef DEBUG_CLIPBOARD
  g_print("    nsClipboard::DoRealConvert(%li)\n    {\n", type);
#endif
  // Set a flag saying that we're blocking waiting for the callback:
  mBlocking = PR_TRUE;

  //
  // ask X what kind of data we can get
  //
#ifdef DEBUG_CLIPBOARD
  g_print("     Doing real conversion of atom type '%s'\n", gdk_atom_name(type));
#endif
  gtk_selection_convert(sWidget,
                        aSelectionAtom,
                        type,
                        GDK_CURRENT_TIME);


  /* because Gtk is smart, it checks in gtk_selection_convert to see if
     the widget we are converting owns the selection, and if it does it
     doesn't bother going to X, so it will be done and mBlocking will be
     set to FALSE by this point, so only poll X if we have to.  (pav)
  */
  if (mBlocking) {
    gtk_grab_add(sWidget);
    // Now we need to wait until the callback comes in ...
    // i is in case we get a runaway (yuck).
#ifdef DEBUG_CLIPBOARD
    g_print("      Waiting for the callback... mBlocking = %d\n", mBlocking);
#endif /* DEBUG_CLIPBOARD */

    XEvent xevent;
    while (!XCheckTypedWindowEvent(GDK_DISPLAY(), GDK_WINDOW_XWINDOW(sWidget->window), SelectionNotify, &xevent));

    GdkEvent event;
    event.selection.type = GDK_SELECTION_NOTIFY;
    event.any.window = gdk_window_lookup(xevent.xany.window);
    event.any.send_event = xevent.xany.send_event ? TRUE : FALSE;
    event.selection.window = event.any.window;
    event.selection.selection = xevent.xselection.selection;
    event.selection.target = xevent.xselection.target;
    event.selection.property = xevent.xselection.property;
    event.selection.time = xevent.xselection.time;
    
    gtk_widget_event(sWidget, &event);

#ifdef DEBUG_CLIPBOARD
    g_print("    }\n");
#endif

    gtk_grab_remove(sWidget);
  }

  if (mSelectionData.length > 0)
    return PR_TRUE;

  return PR_FALSE;
}


//-------------------------------------------------------------------------
//
// The blocking Paste routine
//
//-------------------------------------------------------------------------
NS_IMETHODIMP
nsClipboard::GetNativeClipboardData(nsITransferable * aTransferable, 
                                    PRInt32 aWhichClipboard)
{
  GdkAtom selectionAtom = GetSelectionAtom(aWhichClipboard);

#ifdef DEBUG_CLIPBOARD
  g_print("nsClipboard::GetNativeClipboardData(%i)\n", aWhichClipboard);
#endif /* DEBUG_CLIPBOARD */

  // make sure we have a good transferable
  if (nsnull == aTransferable) {
    printf("  GetNativeClipboardData: Transferable is null!\n");
    return NS_ERROR_FAILURE;
  }

  // get flavor list that includes all acceptable flavors (including ones obtained through
  // conversion)
  nsCOMPtr<nsISupportsArray> flavorList;
  nsresult errCode = aTransferable->FlavorsTransferableCanImport ( getter_AddRefs(flavorList) );
  if ( NS_FAILED(errCode) )
    return NS_ERROR_FAILURE;

  // Walk through flavors and see which flavor matches the one being pasted:
  PRUint32 cnt;
  flavorList->Count(&cnt);
  nsCAutoString foundFlavor;
  PRBool foundData = PR_FALSE;
  for ( PRUint32 i = 0; i < cnt; ++i ) {
    nsCOMPtr<nsISupports> genericFlavor;
    flavorList->GetElementAt ( i, getter_AddRefs(genericFlavor) );
    nsCOMPtr<nsISupportsString> currentFlavor ( do_QueryInterface(genericFlavor) );
    if ( currentFlavor ) {
      nsXPIDLCString flavorStr;
      currentFlavor->ToString ( getter_Copies(flavorStr) );
      if (DoConvert(flavorStr, selectionAtom)) {
        foundFlavor = nsCAutoString(flavorStr);
        foundData = PR_TRUE;
        break;
      }
    }
  }
  
#ifdef DEBUG_CLIPBOARD
  g_print("  Got the callback: '%s', %d\n",
         mSelectionData.data, mSelectionData.length);
#endif /* DEBUG_CLIPBOARD */

  // We're back from the callback, no longer blocking:
  mBlocking = PR_FALSE;

  // 
  // Now we have data in mSelectionData.data.
  // We just have to copy it to the transferable.
  // 

  if ( foundData ) {
    nsCOMPtr<nsISupports> genericDataWrapper;
    nsPrimitiveHelpers::CreatePrimitiveForData(foundFlavor, mSelectionData.data,
                                               mSelectionData.length, getter_AddRefs(genericDataWrapper));
    aTransferable->SetTransferData(foundFlavor,
                                   genericDataWrapper,
                                   mSelectionData.length);
  }
    
  // transferable is now owning the data, so we can free it.
  nsMemory::Free(mSelectionData.data);
  mSelectionData.data = nsnull;
  mSelectionData.length = 0;

  return NS_OK;
}

/**
 * Called when the data from a paste comes in (recieved from gdk_selection_convert)
 *
 * @param  aWidget the widget
 * @param  aSelectionData gtk selection stuff
 * @param  aTime time the selection was requested
 */
void
nsClipboard::SelectionReceivedCB (GtkWidget        *aWidget,
                                  GtkSelectionData *aSelectionData,
                                  guint             aTime)
{
#ifdef DEBUG_CLIPBOARD
  g_print("      nsClipboard::SelectionReceivedCB\n      {\n");
#endif /* DEBUG_CLIPBOARD */
  nsClipboard *cb =(nsClipboard *)gtk_object_get_data(GTK_OBJECT(aWidget),
                                                      "cb");
  if (!cb)
  {
    g_print("no clipboard found.. this is bad.\n");
    return;
  }
  cb->SelectionReceiver(aWidget, aSelectionData);
#ifdef DEBUG_CLIPBOARD
  g_print("      }\n");
#endif
}


/**
 * local method (called from nsClipboard::SelectionReceivedCB)
 *
 * @param  aWidget the widget
 * @param  aSelectionData gtk selection stuff
 */
void
nsClipboard::SelectionReceiver (GtkWidget *aWidget,
                                GtkSelectionData *aSD)
{
  mBlocking = PR_FALSE;

  if (aSD->length <= 0)
  {
#ifdef DEBUG_CLIPBOARD
    g_print("        Error retrieving selection: length was %d\n", aSD->length);
#endif
    return;
  }

  nsCAutoString type(gdk_atom_name(aSD->type));

#ifdef DEBUG_CLIPBOARD
  g_print("        Type is %s\n", type.mBuffer);

  if (type.Equals("ATOM")) {
    g_print("        Asked for TARGETS\n");
  }
#endif

  if (type.Equals("COMPOUND_TEXT")) {
#ifdef DEBUG_CLIPBOARD
    g_print("        Copying mSelectionData pointer -- \n");
#endif
    mSelectionData = *aSD;

    char *data = (char*)aSD->data;
    PRInt32 len = (PRInt32)aSD->length;

    int status = 0;
    XTextProperty prop;

#ifdef DEBUG_CLIPBOARD
    g_print("        Converted text from COMPOUND_TEXT to platform locale\n");
    g_print("        data is %s\n", data);
    g_print("        len is %d\n", len);
#endif

    prop.value = (unsigned char *)data;
    prop.nitems = len;
    prop.encoding = XInternAtom(GDK_DISPLAY(), "COMPOUND_TEXT", FALSE);
    prop.format = 8;

    char **tmpData;
    int foo;
    status = XmbTextPropertyToTextList(GDK_DISPLAY(), &prop, &tmpData, &foo);

    if (foo > 1)
      printf("Got multiple strings from XmbTextPropertyToTextList.. don't know how to handle this yet\n");

    PRInt32 numberOfBytes = 0;

    if (status == Success) {
      data = tmpData[0];
      numberOfBytes = nsCRT::strlen(NS_REINTERPRET_CAST(const char *, data));

#ifdef DEBUG_CLIPBOARD
      g_print("\n        XmbTextListToTextProperty succeeded\n");
      g_print("          text is \"%s\"\n", data);
      g_print("          numberOfBytes is %d\n", numberOfBytes);
#endif
    } else {
      g_print("\n         XmbTextListToTextProperty failed.  returned %d\n", status);
      g_print("          text is \"%s\"\n", tmpData[0]);
      numberOfBytes = nsCRT::strlen(NS_REINTERPRET_CAST(const char *, data));
    }



    nsresult rv;
    PRInt32 outUnicodeLen;
    PRUnichar *unicodeData;

#ifdef DEBUG_CLIPBOARD
    g_print("        Converting from current locale to unicode\n");
#endif

    static nsCOMPtr<nsIUnicodeDecoder> decoder;
    static PRBool hasConverter = PR_FALSE;
    if ( !hasConverter ) {
      // get the charset
      nsAutoString platformCharset;
      nsCOMPtr <nsIPlatformCharset> platformCharsetService = do_GetService(NS_PLATFORMCHARSET_PROGID, &rv);
      if (NS_SUCCEEDED(rv))
        rv = platformCharsetService->GetCharset(kPlatformCharsetSel_Menu, platformCharset);
      if (NS_FAILED(rv))
        platformCharset.AssignWithConversion("ISO-8859-1");
      
      // get the decoder
      NS_WITH_SERVICE(nsICharsetConverterManager, ccm, NS_CHARSETCONVERTERMANAGER_PROGID, &rv);  
      rv = ccm->GetUnicodeDecoder(&platformCharset, getter_AddRefs(decoder));
      
      hasConverter = PR_TRUE;
    }
  
    // Estimate out length and allocate the buffer based on a worst-case estimate, then do
    // the conversion. 
    decoder->GetMaxLength(data, numberOfBytes, &outUnicodeLen);   // |outUnicodeLen| is number of chars
    if (outUnicodeLen) {
      unicodeData = NS_REINTERPRET_CAST(PRUnichar*, nsMemory::Alloc((outUnicodeLen + 1) * sizeof(PRUnichar)));
      if ( unicodeData ) {
        PRInt32 numberTmp = numberOfBytes;
        rv = decoder->Convert(data, &numberTmp, unicodeData, &outUnicodeLen);
        if (numberTmp != numberOfBytes)
          printf("didn't consume all the bytes\n");

        (unicodeData)[outUnicodeLen] = '\0';    // null terminate. Convert() doesn't do it for us
      }
    } // if valid length


    mSelectionData.data = NS_REINTERPRET_CAST(guchar*,unicodeData);
    mSelectionData.length = outUnicodeLen * 2;
  }
  else if (type.Equals("UTF8_STRING")) {
    mSelectionData = *aSD;

    static const PRUnichar unicodeFormatter[] = {
      (PRUnichar)'%',
      (PRUnichar)'s',
      (PRUnichar)0,
    };
    // convert aSD->data (UTF8) to Unicode and place the unicode data in mSelectionData.data
    PRUnichar *unicodeString = nsTextFormatter::smprintf(unicodeFormatter, aSD->data);

    if (!unicodeString) // this would be bad wouldn't it?
      return;

    int len = sizeof(unicodeString);
    mSelectionData.data = g_new(guchar, len + 2);
    memcpy(mSelectionData.data,
           unicodeString,
           len);
    nsTextFormatter::smprintf_free(unicodeString);
    mSelectionData.type = gdk_atom_intern(kUnicodeMime, FALSE);
    mSelectionData.length = len;

  } else if (type.Equals("STRING")) {
#ifdef DEBUG_CLIPBOARD
    g_print("        Copying mSelectionData pointer -- \n");
    g_print("         Data = %s\n         Length = %i\n", aSD->data, aSD->length);
#endif
    mSelectionData = *aSD;

    // convert our plain text to unicode
    const char* castedText = NS_REINTERPRET_CAST(char*, mSelectionData.data);          
    PRUnichar* convertedText = nsnull;
    PRInt32 convertedTextLen = 0;
    nsPrimitiveHelpers::ConvertPlatformPlainTextToUnicode (castedText, mSelectionData.length, 
                                                           &convertedText, &convertedTextLen);
    if (convertedText) {
      // out with the old, in with the new 
      mSelectionData.data = NS_REINTERPRET_CAST(guchar*, convertedText);
      mSelectionData.length = convertedTextLen * 2;
    }
  } else {
    mSelectionData = *aSD;
    mSelectionData.data = g_new(guchar, aSD->length + 1);
    memcpy(mSelectionData.data,
           aSD->data,
           aSD->length);
    mSelectionData.length = aSD->length;
  }
}

/**
 * Some platforms support deferred notification for putting data on the clipboard
 * This method forces the data onto the clipboard in its various formats
 * This may be used if the application going away.
 *
 * @result NS_OK if successful.
 */
NS_IMETHODIMP nsClipboard::ForceDataToClipboard(PRInt32 aWhichClipboard)
{
#ifdef DEBUG_CLIPBOARD
  g_print("  nsClipboard::ForceDataToClipboard()\n");
#endif /* DEBUG_CLIPBOARD */

  // make sure we have a good transferable

  nsCOMPtr<nsITransferable> transferable(getter_AddRefs(GetTransferable(aWhichClipboard)));

  if (nsnull == transferable) {
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsClipboard::HasDataMatchingFlavors(nsISupportsArray* aFlavorList, 
                                    PRInt32 aWhichClipboard, 
                                    PRBool * outResult)
{
  // XXX this doesn't work right.  need to fix it.
  
  // Note to implementor...(from pink the clipboard bitch).
  //
  // If a client asks for unicode, first check if unicode is present. If not, then 
  // check for plain text. If it's there, say "yes" as we will do the conversion
  // in GetNativeClipboardData(). From this point on, no client will
  // ever ask for text/plain explicitly. If they do, you must ASSERT!
#ifdef DEBUG_CLIPBOARD
  g_print("  nsClipboard::HasDataMatchingFlavors()\n  {\n");
#endif


  GetTargets(GetSelectionAtom(aWhichClipboard));

  guchar *data = mSelectionData.data;
  PRInt32 dataLength = mSelectionData.length;
  int position = 0;
  gchar *str;


  *outResult = PR_FALSE;
  PRUint32 length;
  aFlavorList->Count(&length);
  for ( PRUint32 i = 0; i < length; ++i ) {
    nsCOMPtr<nsISupports> genericFlavor;
    aFlavorList->GetElementAt ( i, getter_AddRefs(genericFlavor) );
    nsCOMPtr<nsISupportsString> flavorWrapper ( do_QueryInterface(genericFlavor) );
    if ( flavorWrapper ) {
      nsCAutoString flavorStr;
      nsXPIDLCString myStr;
      flavorWrapper->ToString(getter_Copies(myStr));
      flavorStr = nsCAutoString(myStr);

      position = 0;
      while (position < dataLength) {
        str = gdk_atom_name(*(GdkAtom*)(data+position));
        position += sizeof(GdkAtom);
        nsCAutoString atomName(str);
        
        if (flavorStr.Equals(kUnicodeMime)) {
          if (atomName.Equals("COMPOUND_TEXT") ||
              atomName.Equals("UTF8_STRING") ||
              atomName.Equals("STRING")) {
#ifdef DEBUG_CLIPBOARD
            g_print("    Selection owner has matching type: %s\n", atomName.mBuffer);
#endif
            *outResult = PR_TRUE;
            break;
          }
        }
        if (flavorStr.Equals(atomName)) {
#ifdef DEBUG_CLIPBOARD
          g_print("    Selection owner has matching type: %s\n", atomName.mBuffer);
#endif
          *outResult = PR_TRUE;
          break;
        }
      }
    }
  }
#ifdef DEBUG_CLIPBOARD
  g_print("    returning %i\n  }\n", *outResult);
#endif

  nsMemory::Free(mSelectionData.data);
  mSelectionData.data = nsnull;
  mSelectionData.length = 0;

  return NS_OK;

}

/**
 * This is the callback which is called when another app
 * requests the selection.
 *
 * @param  widget The widget
 * @param  aSelectionData Selection data
 * @param  the type being asked for
 * @param  time Time when the selection request came in
 */
void nsClipboard::SelectionGetCB(GtkWidget        *widget,
                                 GtkSelectionData *aSelectionData,
                                 guint            aInfo,
                                 guint            aTime)
{
#ifdef DEBUG_CLIPBOARD
  g_print("nsClipboard::SelectionGetCB\n"); 
#endif
  nsClipboard *cb = (nsClipboard *)gtk_object_get_data(GTK_OBJECT(widget),
                                                       "cb");

  void     *clipboardData;
  PRUint32 dataLength;
  nsresult rv;

  PRInt32 whichClipboard = -1;

  if (aSelectionData->selection == GDK_SELECTION_PRIMARY)
    whichClipboard = kSelectionClipboard;
  else if (aSelectionData->selection == GDK_SELECTION_CLIPBOARD)
    whichClipboard = kGlobalClipboard;

#ifdef DEBUG_CLIPBOARD
  g_print("  whichClipboard = %d\n", whichClipboard);
#endif
  nsCOMPtr<nsITransferable> transferable(getter_AddRefs(cb->GetTransferable(whichClipboard)));

  // Make sure we have a transferable:
  if (!transferable) {
    g_print("Clipboard has no transferable!\n");
    return;
  }
#ifdef DEBUG_CLIPBOARD
  g_print("  aInfo == %s\n  transferable == %p\n", gdk_atom_name(aInfo), transferable.get());
  g_print("  aSD->type == %s\n  aSD->target == %s\n", gdk_atom_name(aSelectionData->type),
          gdk_atom_name(aSelectionData->target));
#endif
  char* dataFlavor = nsnull;
  nsCAutoString type(gdk_atom_name(aInfo));

  if (type.Equals("STRING") ||
      type.Equals("UTF8_STRING") ||
      type.Equals("COMPOUND_TEXT") ||
      type.Equals("TEXT"))
  {
    dataFlavor = kUnicodeMime;
  } else {
    dataFlavor = type;
  }

  // Get data out of transferable.
  nsCOMPtr<nsISupports> genericDataWrapper;
  rv = transferable->GetTransferData(dataFlavor,
                                     getter_AddRefs(genericDataWrapper),
                                     &dataLength);
  nsPrimitiveHelpers::CreateDataFromPrimitive ( dataFlavor, genericDataWrapper, &clipboardData, dataLength );
  if (NS_SUCCEEDED(rv) && clipboardData && dataLength > 0) {
    size_t size = 1;
    // find the number of bytes in the data for the below thing
    //    size_t size = sizeof((void*)((unsigned char)clipboardData[0]));
    //    g_print("************  *****************      ******************* %i\n", size);
    

#ifdef DEBUG_CLIPBOARD
    printf("got data from transferable\n");
    printf("clipboardData is %s\n", clipboardData);
    printf("length is %d\n", dataLength);
#endif

    if (type.Equals("STRING")) {
      // if someone was asking for text/plain, lookup unicode instead so we can convert it.
      char* plainTextData = nsnull;
      PRUnichar* castedUnicode = NS_REINTERPRET_CAST(PRUnichar*, clipboardData);
      PRInt32 plainTextLen = 0;
      nsPrimitiveHelpers::ConvertUnicodeToPlatformPlainText (castedUnicode, dataLength / 2,
                                                             &plainTextData, &plainTextLen);
      if (clipboardData) {
        nsMemory::Free(NS_REINTERPRET_CAST(char*, clipboardData));
        clipboardData = plainTextData;
        dataLength = plainTextLen;
      }
    } else if (type.Equals("UTF8_STRING")) {
      if (clipboardData) {
        PRUnichar* castedUnicode = NS_REINTERPRET_CAST(PRUnichar*, clipboardData);
        nsString str(castedUnicode, dataLength);
        char *utf8String = str.ToNewUTF8String();
        nsMemory::Free(NS_REINTERPRET_CAST(char*, clipboardData));
        clipboardData = utf8String;
        dataLength = strlen(utf8String);
      }
    } else if (type.Equals("COMPOUND_TEXT") || type.Equals("TEXT")) {
      if (type.Equals("TEXT")) {
        // if we get a request for  TEXT, return COMPOUND_TEXT
        aInfo = gdk_atom_intern("COMPOUND_TEXT", FALSE);
      }
      char *platformText;
      PRInt32 platformLen;
      // Get the appropriate unicode encoder. We're guaranteed that this won't change
      // through the life of the app so we can cache it.

      static nsCOMPtr<nsIUnicodeEncoder> encoder;
      static PRBool hasConverter = PR_FALSE;
      if ( !hasConverter ) {
        // get the charset
        nsAutoString platformCharset;
        nsCOMPtr <nsIPlatformCharset> platformCharsetService = do_GetService(NS_PLATFORMCHARSET_PROGID, &rv);
        if (NS_SUCCEEDED(rv))
          rv = platformCharsetService->GetCharset(kPlatformCharsetSel_Menu, platformCharset);
        if (NS_FAILED(rv))
          platformCharset.AssignWithConversion("ISO-8859-1");
      
        // get the encoder
        NS_WITH_SERVICE(nsICharsetConverterManager, ccm, NS_CHARSETCONVERTERMANAGER_PROGID, &rv);  
        rv = ccm->GetUnicodeEncoder(&platformCharset, getter_AddRefs(encoder));

        hasConverter = PR_TRUE;
      }

      // Estimate out length and allocate the buffer based on a worst-case estimate, then do
      // the conversion.
      PRUnichar *castedData = NS_REINTERPRET_CAST(PRUnichar*, clipboardData);
      encoder->GetMaxLength(castedData, dataLength, &platformLen);
      if ( platformLen ) {
        platformText = NS_REINTERPRET_CAST(char*, nsMemory::Alloc(platformLen + sizeof(char)));
        if ( platformText ) {
          PRInt32 len = (PRInt32)dataLength;
          rv = encoder->Convert(castedData, &len, platformText, &platformLen);
          (platformText)[platformLen] = '\0';  // null terminate. Convert() doesn't do it for us
        }
 } // if valid length

      if (platformLen > 0) {
        int status = 0;
        XTextProperty prop;

#ifdef DEBUG_CLIPBOARD
        g_print("\nConverted text from unicode to platform locale\n");
        g_print("platformText is %s\n", platformText);
        g_print("platformLen is %d\n", platformLen);
#endif

        status = XmbTextListToTextProperty(GDK_DISPLAY(), &platformText, 1, XCompoundTextStyle,
                                           &prop);

        if (status == Success) {
#ifdef DEBUG_CLIPBOARD
          g_print("\nXmbTextListToTextProperty succeeded\n  text is %s\n  length is %d\n", prop.value,
                  prop.nitems);
#endif
          nsMemory::Free(platformText);
          platformText = (char *)prop.value;
          platformLen = prop.nitems;
        }
      }

#ifdef DEBUG_CLIPBOARD
      g_print("\nFinished trying to convert to platform charset\n");
#endif

      if (clipboardData) {
        nsMemory::Free(NS_REINTERPRET_CAST(char*, clipboardData));
        clipboardData = platformText;
        dataLength = platformLen;
      }
    }
#ifdef DEBUG_CLIPBOARD
    g_print("\nPutting data on clipboard:\n");
    g_print("  clipboardData is %s\n", clipboardData);
    g_print("  length is %d\n", dataLength);
#endif
    if (clipboardData && dataLength > 0)
      gtk_selection_data_set(aSelectionData,
                             aInfo, size*8,
                             NS_REINTERPRET_CAST(unsigned char *, clipboardData),
                             dataLength);
    else
      gtk_selection_data_set(aSelectionData,
                             gdk_atom_intern("NULL", FALSE), 8,
                             nsnull, 0);
    nsCRT::free ( NS_REINTERPRET_CAST(char*, clipboardData) );
  }
  else
    printf("Transferable didn't support data flavor %s (type = %d)\n",
           dataFlavor ? dataFlavor : "None", aInfo);
}


/**
 * Called when another app requests selection ownership
 *
 * @param  aWidget the widget
 * @param  aEvent the GdkEvent for the selection
 * @param  aData value passed in from the callback init
 */
void nsClipboard::SelectionClearCB(GtkWidget *aWidget,
                                   GdkEventSelection *aEvent,
                                   gpointer aData)
{
#ifdef DEBUG_CLIPBOARD
  g_print("  nsClipboard::SelectionClearCB\n");
#endif /* DEBUG_CLIPBOARD */

  nsClipboard *cb = (nsClipboard *)gtk_object_get_data(GTK_OBJECT(aWidget),
                                                       "cb");

  if (aEvent->selection == GDK_SELECTION_PRIMARY) {
    g_print("clearing PRIMARY clipboard\n");
    cb->EmptyClipboard(kSelectionClipboard);
  } else if (aEvent->selection == GDK_SELECTION_CLIPBOARD) {
    g_print("clearing CLIPBOARD clipboard\n");
    cb->EmptyClipboard(kGlobalClipboard);
  }
}


/**
 * The routine called when another app asks for the content of the selection
 *
 * @param  aWidget the widget
 * @param  aSelectionData gtk selection stuff
 * @param  aData value passed in from the callback init
 */
void
nsClipboard::SelectionRequestCB (GtkWidget *aWidget,
                                 GtkSelectionData *aSelectionData,
                                 gpointer aData)
{
#ifdef DEBUG_CLIPBOARD
  g_print("  nsClipboard::SelectionRequestCB\n");
#endif /* DEBUG_CLIPBOARD */
}

/**
 * ...
 *
 * @param  aWidget the widget
 * @param  aSelectionData gtk selection stuff
 * @param  aData value passed in from the callback init
 */
void
nsClipboard::SelectionNotifyCB (GtkWidget *aWidget,
                                GtkSelectionData *aSelectionData,
                                gpointer aData)
{
#ifdef DEBUG_CLIPBOARD
  g_print("  nsClipboard::SelectionNotifyCB\n");
#endif /* DEBUG_CLIPBOARD */
}

/* helper functions*/

/* inline */
GdkAtom nsClipboard::GetSelectionAtom(PRInt32 aWhichClipboard)
{
  switch (aWhichClipboard)
  {
  case kGlobalClipboard:
    return GDK_SELECTION_CLIPBOARD;
  case kSelectionClipboard:
  default:
    return GDK_SELECTION_PRIMARY;
  }
}

/* inline */
nsITransferable *nsClipboard::GetTransferable(PRInt32 aWhichClipboard)
{
  nsITransferable *transferable = nsnull;
  switch (aWhichClipboard)
  {
  case kGlobalClipboard:
    transferable = mGlobalTransferable;
    break;
  case kSelectionClipboard:
    transferable = mSelectionTransferable;
    break;
  }
  NS_IF_ADDREF(transferable);
  return transferable;
}

/* inline */
void nsClipboard::AddTarget(GdkAtom aAtom, GdkAtom aSelectionAtom)
{
  gtk_selection_add_target(sWidget, aSelectionAtom, aAtom, aAtom);
}

void nsClipboard::RegisterFormat(const char *aMimeStr, GdkAtom aSelectionAtom)
{
#ifdef DEBUG_CLIPBOARD
  g_print("  nsClipboard::RegisterFormat(%s)\n", aMimeStr);
#endif
  nsCAutoString mimeStr(aMimeStr);

  GdkAtom atom = gdk_atom_intern(aMimeStr, FALSE);

  // for Text and Unicode we want to add some extra types to the X clipboard
  if (mimeStr.Equals(kUnicodeMime)) {
    // we will do the conversions to and from unicode internally
    // anyone asking for TEXT will get COMPOUND_TEXT
    AddTarget(gdk_atom_intern("TEXT", FALSE), aSelectionAtom); 
    AddTarget(gdk_atom_intern("COMPOUND_TEXT", FALSE), aSelectionAtom);
    AddTarget(gdk_atom_intern("UTF8_STRING", FALSE), aSelectionAtom);
    AddTarget(GDK_SELECTION_TYPE_STRING, aSelectionAtom);
  }

  AddTarget(atom, aSelectionAtom);
}

/* return PR_TRUE if we have converted or PR_FALSE if we havn't and need to keep being called */
PRBool nsClipboard::DoConvert(const char *aMimeStr, GdkAtom aSelectionAtom)
{
#ifdef DEBUG_CLIPBOARD
  g_print("  nsClipboard::DoConvert(%s, %s)\n", aMimeStr, gdk_atom_name(aSelectionAtom));
#endif
  /* when doing the selection_add_target, each case should have the same last parameter
     which matches the case match */
  PRBool r = PR_FALSE;

  nsCAutoString mimeStr(aMimeStr);

  if (mimeStr.Equals(kUnicodeMime)) {
    r = DoRealConvert(gdk_atom_intern("COMPOUND_TEXT", FALSE), aSelectionAtom);
    if (r) return r;
    r = DoRealConvert(gdk_atom_intern("UTF8_STRING", FALSE), aSelectionAtom);
    if (r) return r;
    r = DoRealConvert(GDK_SELECTION_TYPE_STRING, aSelectionAtom);
    if (r) return r;
  }

  GdkAtom atom = gdk_atom_intern(aMimeStr, FALSE);
  r = DoRealConvert(atom, aSelectionAtom);
  if (r) return r;

  return r;
}

PRBool nsClipboard::GetTargets(GdkAtom aSelectionAtom)
{
#ifdef DEBUG_CLIPBOARD
  g_print("    nsClipboard::GetTargets(%d)\n    {\n", aSelectionAtom);
#endif
  // Set a flag saying that we're blocking waiting for the callback:
  mBlocking = PR_TRUE;

  //
  // ask X what kind of data we can get
  //
  static GdkAtom targetsAtom = gdk_atom_intern("TARGETS", PR_FALSE);

  gtk_selection_convert(sWidget,
                        aSelectionAtom,
                        targetsAtom,
                        GDK_CURRENT_TIME);

  /* see comment in DoRealConvert for why we check for mBlocking here */
  if (mBlocking) {
    gtk_grab_add(sWidget);
    
    // Now we need to wait until the callback comes in ...
    // i is in case we get a runaway (yuck).
#ifdef DEBUG_CLIPBOARD
    g_print("      Waiting for the callback... mBlocking = %d\n", mBlocking);
#endif /* DEBUG_CLIPBOARD */
    
    XEvent xevent;
    while (!XCheckTypedWindowEvent(GDK_DISPLAY(), GDK_WINDOW_XWINDOW(sWidget->window), SelectionNotify, &xevent));
    
    GdkEvent event;
    event.selection.type = GDK_SELECTION_NOTIFY;
    event.any.window = gdk_window_lookup (xevent.xany.window);
    event.any.send_event = xevent.xany.send_event ? TRUE : FALSE;
    event.selection.window = event.any.window;
    event.selection.selection = xevent.xselection.selection;
    event.selection.target = xevent.xselection.target;
    event.selection.property = xevent.xselection.property;
    event.selection.time = xevent.xselection.time;
    
    gtk_widget_event(sWidget, &event);
    
    gtk_grab_remove(sWidget);
  }

#ifdef DEBUG_CLIPBOARD
  g_print("    }\n");
#endif

  if (mSelectionData.length <= 0)
    return PR_FALSE;

  return PR_TRUE;
}
