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

#include "nsClipboard.h"

#include "nsCOMPtr.h"
#include "nsFileSpec.h"

#include "nsISupportsArray.h"
#include "nsIClipboardOwner.h"
#include "nsITransferable.h"   // kTextMime
#include "nsISupportsPrimitives.h"

#include "nsIWidget.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsWidgetsCID.h"
#include "nsXPIDLString.h"
#include "nsPrimitiveHelpers.h"

#include "nsTextFormatter.h"
#include "nsVoidArray.h"


// The class statics:
GtkWidget* nsClipboard::sWidget = 0;


#if defined(DEBUG_mcafee) || defined(DEBUG_pavlov)
#define DEBUG_CLIPBOARD
#endif
 
enum {
  TARGET_NONE,
  TARGET_TEXT_PLAIN,
  TARGET_TEXT_XIF,
  TARGET_TEXT_UNICODE,
  TARGET_TEXT_HTML,
  TARGET_AOLMAIL,
  TARGET_IMAGE_PNG,
  TARGET_IMAGE_JPEG,
  TARGET_IMAGE_GIF,
  // compatibility types
  TARGET_UTF8,
  TARGET_UNKNOWN,
  TARGET_LAST
};

static GdkAtom sSelTypes[TARGET_LAST];

static GdkAtom GDK_SELECTION_CLIPBOARD;

//-------------------------------------------------------------------------
//
// nsClipboard constructor
//
//-------------------------------------------------------------------------
nsClipboard::nsClipboard() : nsBaseClipboard()
{
#ifdef DEBUG_CLIPBOARD
  g_print("nsClipboard::nsClipboard()\n");
#endif /* DEBUG_CLIPBOARD */

  //NS_INIT_REFCNT();
  mIgnoreEmptyNotification = PR_FALSE;
  mClipboardOwner = nsnull;
  mTransferable   = nsnull;
  mSelectionData.data = nsnull;
  mSelectionData.length = 0;
  mSelectionAtom = GDK_SELECTION_PRIMARY;

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

void __gtk_selection_target_list_remove (GtkWidget *widget)
{
  GtkSelectionTargetList *sellist;
  GList *tmp_list;
  GList *lists;
  lists = (GList*)gtk_object_get_data (GTK_OBJECT (widget), gtk_selection_handler_key);
  tmp_list = lists;
  while (tmp_list)
    {
      sellist = (GtkSelectionTargetList*)tmp_list->data;
      gtk_target_list_unref (sellist->list);
      g_free (sellist);
      tmp_list = tmp_list->next;
    }
  g_list_free (lists);
  gtk_object_set_data (GTK_OBJECT (widget), gtk_selection_handler_key, NULL);
}

//-------------------------------------------------------------------------
//
// nsClipboard destructor
//
//-------------------------------------------------------------------------
nsClipboard::~nsClipboard()
{
#ifdef DEBUG_CLIPBOARD
  printf("nsClipboard::~nsClipboard()\n");  
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
    nsAllocator::Free(mSelectionData.data);

  gtk_object_remove_data(GTK_OBJECT(sWidget), "cb");

  if (sWidget) {
    gtk_widget_unref(sWidget);
    sWidget = nsnull;
  }
}


// 
// GTK Weirdness!
// This is here in the hope of being able to call
//  gtk_selection_add_targets(w, GDK_SELECTION_PRIMARY,
//                            targets,
//                            1);
// instead of
//   gtk_selection_add_target(sWidget, 
//                            GDK_SELECTION_PRIMARY,
//                            GDK_SELECTION_TYPE_STRING,
//                            GDK_SELECTION_TYPE_STRING);
// but it turns out that this changes the whole gtk selection model;
// when calling add_targets copy uses selection_clear_event and the
// data structure needs to be filled in in a way that we haven't
// figured out; when using add_target copy uses selection_get and
// the data structure is already filled in as much as it needs to be.
// Some gtk internals wizard will need to solve this mystery before
// we can use add_targets().
//static GtkTargetEntry targets[] = {
//  { "strings n stuff", GDK_SELECTION_TYPE_STRING, GDK_SELECTION_TYPE_STRING }
//};
//

//-------------------------------------------------------------------------
void nsClipboard::Init(void)
{
#ifdef DEBUG_CLIPBOARD
  g_print("nsClipboard::Init\n");
#endif

  sSelTypes[TARGET_NONE]          = GDK_NONE;
  sSelTypes[TARGET_TEXT_PLAIN]    = gdk_atom_intern(kTextMime, FALSE);
  sSelTypes[TARGET_TEXT_XIF]      = gdk_atom_intern(kXIFMime, FALSE);
  sSelTypes[TARGET_TEXT_UNICODE]  = gdk_atom_intern(kUnicodeMime, FALSE);
  sSelTypes[TARGET_UTF8]          = gdk_atom_intern("UTF8_STRING", FALSE);
  sSelTypes[TARGET_TEXT_HTML]     = gdk_atom_intern(kHTMLMime, FALSE);
  sSelTypes[TARGET_AOLMAIL]       = gdk_atom_intern(kAOLMailMime, FALSE);
  sSelTypes[TARGET_IMAGE_PNG]     = gdk_atom_intern(kPNGImageMime, FALSE);
  sSelTypes[TARGET_IMAGE_JPEG]    = gdk_atom_intern(kJPEGImageMime, FALSE);
  sSelTypes[TARGET_IMAGE_GIF]     = gdk_atom_intern(kGIFImageMime, FALSE);
  // compatibility with other apps

  GDK_SELECTION_CLIPBOARD         = gdk_atom_intern("CLIPBOARD", FALSE);

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


//-------------------------------------------------------------------------
NS_IMETHODIMP nsClipboard::SetNativeClipboardData()
{
  mIgnoreEmptyNotification = PR_TRUE;

#ifdef DEBUG_CLIPBOARD
  printf("  nsClipboard::SetNativeClipboardData()\n");
#endif /* DEBUG_CLIPBOARD */

  // make sure we have a good transferable
  if (nsnull == mTransferable) {
    printf("nsClipboard::SetNativeClipboardData(): no transferable!\n");
    return NS_ERROR_FAILURE;
  }

  // are we already the owner?
  if (gdk_selection_owner_get(mSelectionAtom) == sWidget->window)
  {
    // if so, clear all the targets
    __gtk_selection_target_list_remove(sWidget);
    //    gtk_selection_remove_all(sWidget);
  }

  // we arn't already the owner, so we will become it
  gint have_selection = gtk_selection_owner_set(sWidget,
                                                mSelectionAtom,
                                                GDK_CURRENT_TIME);
  if (have_selection == 0)
    return NS_ERROR_FAILURE;

  // get flavor list that includes all flavors that can be written (including ones 
  // obtained through conversion)
  nsCOMPtr<nsISupportsArray> flavorList;
  nsresult errCode = mTransferable->FlavorsTransferableCanExport ( getter_AddRefs(flavorList) );
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
      gint format = GetFormat(flavorStr);

      // add these types as selection targets
      RegisterFormat(format);
    }
  }

  mIgnoreEmptyNotification = PR_FALSE;

  return NS_OK;
}

void nsClipboard::AddTarget(GdkAtom aAtom)
{
  gtk_selection_add_target(sWidget,
                           mSelectionAtom,
                           aAtom, aAtom);
}

gint nsClipboard::GetFormat(const char* aMimeStr)
{
  gint type = TARGET_NONE;
  nsCAutoString mimeStr ( CBufDescriptor(aMimeStr, PR_TRUE, PL_strlen(aMimeStr)+1) );
#ifdef DEBUG_CLIPBOARD
  g_print("  nsClipboard::GetFormat(%s)\n", aMimeStr);
#endif
  if (mimeStr.Equals(kTextMime)) {
    type = TARGET_TEXT_PLAIN;
  } else if (mimeStr.Equals("STRING")) {
    type = TARGET_TEXT_PLAIN;
  } else if (mimeStr.Equals(kXIFMime)) {
    type = TARGET_TEXT_XIF;
  } else if (mimeStr.Equals(kUnicodeMime)) {
    type = TARGET_TEXT_UNICODE;
  } else if (mimeStr.Equals(kHTMLMime)) {
    type = TARGET_TEXT_HTML;
  } else if (mimeStr.Equals(kAOLMailMime)) {
    type = TARGET_AOLMAIL;
  } else if (mimeStr.Equals(kPNGImageMime)) {
    type = TARGET_IMAGE_PNG;
  } else if (mimeStr.Equals(kJPEGImageMime)) {
    type = TARGET_IMAGE_JPEG;
  } else if (mimeStr.Equals(kGIFImageMime)) {
    type = TARGET_IMAGE_GIF;
  }

#ifdef WE_DO_DND
  else if (mimeStr.Equals(kDropFilesMime)) {
    format = CF_HDROP;
  } else {
    format = ::RegisterClipboardFormat(aMimeStr);
  }
#endif
  return type;
}

void nsClipboard::RegisterFormat(gint format)
{
#ifdef DEBUG_CLIPBOARD
  g_print("  nsClipboard::RegisterFormat(%s)\n", gdk_atom_name(sSelTypes[format]));
#endif

  /* when doing the selection_add_target, each case should have the same last parameter
     which matches the case match */
  switch(format)
  {
  case TARGET_TEXT_PLAIN:
    // text/plain (default)
    AddTarget(sSelTypes[format]);

    // STRING (what X uses)
    AddTarget(GDK_SELECTION_TYPE_STRING);
    break;


  case TARGET_TEXT_XIF:
    // text/xif (default)
    AddTarget(sSelTypes[format]);
    break;


  case TARGET_TEXT_UNICODE:
    // STRING (what X uses) we will do a conversion internally
    AddTarget(GDK_SELECTION_TYPE_STRING);

    // text/unicode (default)
    AddTarget(sSelTypes[format]);

    // UTF8 (what X uses)
    AddTarget(sSelTypes[TARGET_UTF8]);
    break;

  case TARGET_TEXT_HTML:
    // text/html (default)
    AddTarget(sSelTypes[format]);
    break;


  case TARGET_AOLMAIL:
    // text/aolmail (default)
    AddTarget(sSelTypes[format]);
    break;


  case TARGET_IMAGE_PNG:
    // image/png (default)
    AddTarget(sSelTypes[format]);
    break;


  case TARGET_IMAGE_JPEG:
    // image/jpeg (default)
    AddTarget(sSelTypes[format]);
    break;


  case TARGET_IMAGE_GIF:
    // image/gif (default)
    AddTarget(sSelTypes[format]);
    break;


  default:
    // if we don't match something above, then just add it like its something we know about...
    AddTarget(sSelTypes[format]);
  }
}

PRBool nsClipboard::DoRealConvert(GdkAtom type)
{
#ifdef DEBUG_CLIPBOARD
  g_print("    nsClipboard::DoRealConvert(%li)\n    {\n", type);
#endif
  int e = 0;
  // Set a flag saying that we're blocking waiting for the callback:
  mBlocking = PR_TRUE;

  //
  // ask X what kind of data we can get
  //
#ifdef DEBUG_CLIPBOARD
  g_print("     Doing real conversion of atom type '%s'\n", gdk_atom_name(type));
#endif
  gtk_selection_convert(sWidget,
                        mSelectionAtom,
                        type,
                        GDK_CURRENT_TIME);

  // Now we need to wait until the callback comes in ...
  // i is in case we get a runaway (yuck).
#ifdef DEBUG_CLIPBOARD
  printf("      Waiting for the callback... mBlocking = %d\n", mBlocking);
#endif /* DEBUG_CLIPBOARD */
  for (e=0; mBlocking == PR_TRUE && e < 1000; ++e)
  {
    gtk_main_iteration_do(PR_TRUE);
  }

#ifdef DEBUG_CLIPBOARD
  g_print("    }\n");
#endif

  if (mSelectionData.length > 0)
    return PR_TRUE;

  return PR_FALSE;
}

/* return PR_TRUE if we have converted or PR_FALSE if we havn't and need to keep being called */
PRBool nsClipboard::DoConvert(gint format)
{
#ifdef DEBUG_CLIPBOARD
  g_print("  nsClipboard::DoConvert(%s)\n", gdk_atom_name(sSelTypes[format]));
#endif

  /* when doing the selection_add_target, each case should have the same last parameter
     which matches the case match */
  PRBool r = PR_FALSE;

  switch(format)
  {
  case TARGET_TEXT_PLAIN:
    r = DoRealConvert(sSelTypes[format]);
    if (r) return r;
    r = DoRealConvert(GDK_SELECTION_TYPE_STRING);
    if (r) return r;
    break;

  case TARGET_TEXT_XIF:
    r = DoRealConvert(sSelTypes[format]);
    if (r) return r;
    break;

  case TARGET_TEXT_UNICODE:
    r = DoRealConvert(sSelTypes[format]);
    if (r) return r;
    r = DoRealConvert(sSelTypes[TARGET_UTF8]);
    if (r) return r;
    break;

  case TARGET_TEXT_HTML:
    r = DoRealConvert(sSelTypes[format]);
    if (r) return r;
    break;

  case TARGET_AOLMAIL:
    r = DoRealConvert(sSelTypes[format]);
    if (r) return r;
    break;

  case TARGET_IMAGE_PNG:
    r = DoRealConvert(sSelTypes[format]);
    if (r) return r;
    break;

  case TARGET_IMAGE_JPEG:
    r = DoRealConvert(sSelTypes[format]);
    if (r) return r;
    break;

  case TARGET_IMAGE_GIF:
    r = DoRealConvert(sSelTypes[format]);
    if (r) return r;
    break;

  default:
    g_print("DoConvert called with bogus format\n");
  }
  return r;
}

//-------------------------------------------------------------------------
//
// The blocking Paste routine
//
//-------------------------------------------------------------------------
NS_IMETHODIMP
nsClipboard::GetNativeClipboardData(nsITransferable * aTransferable)
{

#ifdef DEBUG_CLIPBOARD
  printf("nsClipboard::GetNativeClipboardData()\n");
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
      gint format = GetFormat(flavorStr);
      if (DoConvert(format)) {
        foundFlavor = flavorStr;
        foundData = PR_TRUE;
        break;
      }
    }
  }
  if ( !foundData ) {
    // if we still haven't found anything yet and we're asked to find text/unicode, then
    // try to give them text plain if it's there.
    gint format = GetFormat(kTextMime);
    if (DoConvert(format)) {
       const char* castedText = NS_REINTERPRET_CAST(char*, mSelectionData.data);          
       PRUnichar* convertedText = nsnull;
       PRInt32 convertedTextLen = 0;
       nsPrimitiveHelpers::ConvertPlatformPlainTextToUnicode ( castedText, mSelectionData.length, 
                                                                 &convertedText, &convertedTextLen );
       if ( convertedText ) {
         // out with the old, in with the new 
         nsAllocator::Free(mSelectionData.data);
         mSelectionData.data = NS_REINTERPRET_CAST(guchar*, convertedText);
         mSelectionData.length = convertedTextLen * 2;
		 foundFlavor = kUnicodeMime;
         foundData = PR_TRUE;
       }
     } // if plain text data on clipboard
  }
  
#ifdef DEBUG_CLIPBOARD
  printf("  Got the callback: '%s', %d\n",
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
  nsAllocator::Free(mSelectionData.data);
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
  printf("      nsClipboard::SelectionReceivedCB\n      {\n");
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
  gint type;

  mBlocking = PR_FALSE;

  if (aSD->length < 0)
  {
#ifdef DEBUG_CLIPBOARD
    printf("        Error retrieving selection: length was %d\n", aSD->length);
#endif
    return;
  }

  type = TARGET_NONE;
  for (int i=0; i < TARGET_LAST; i++)
  {
    if (sSelTypes[i] == aSD->type)
    {
      type = i;
      break;
    }
  }

  switch (type)
  {
  case TARGET_UTF8:
    {
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
      mSelectionData.type = TARGET_TEXT_UNICODE;
      mSelectionData.length = len;
    }
    break;

  case GDK_TARGET_STRING:
  case TARGET_TEXT_PLAIN:
  case TARGET_TEXT_XIF:
  case TARGET_TEXT_UNICODE:
  case TARGET_TEXT_HTML:
#ifdef DEBUG_CLIPBOARD
    g_print("        Copying mSelectionData pointer -- ");
#endif
    mSelectionData = *aSD;
    mSelectionData.data = NS_REINTERPRET_CAST(guchar*, nsAllocator::Alloc(aSD->length + 1));
#ifdef DEBUG_CLIPBOARD
    g_print("        Data = %s\n    Length = %i\n", aSD->data, aSD->length);
#endif
    memcpy(mSelectionData.data,
           aSD->data,
           aSD->length);
    // Null terminate in case anyone cares,
    // and so we can print the string for debugging:
    mSelectionData.data[aSD->length] = '\0';
    mSelectionData.length = aSD->length;
    return;

  default:
    mSelectionData = *aSD;
    mSelectionData.data = g_new(guchar, aSD->length + 1);
    memcpy(mSelectionData.data,
           aSD->data,
           aSD->length);
    mSelectionData.length = aSD->length;
    return;
  }
}



/**
 * Some platforms support deferred notification for putting data on the clipboard
 * This method forces the data onto the clipboard in its various formats
 * This may be used if the application going away.
 *
 * @result NS_OK if successful.
 */
NS_IMETHODIMP nsClipboard::ForceDataToClipboard()
{
#ifdef DEBUG_CLIPBOARD
  printf("  nsClipboard::ForceDataToClipboard()\n");
#endif /* DEBUG_CLIPBOARD */

  // make sure we have a good transferable
  if (nsnull == mTransferable) {
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsClipboard::HasDataMatchingFlavors(nsISupportsArray* aFlavorList, PRBool * outResult)
{
  // XXX this doesn't work right.  need to fix it.
  
  // Note to implementor...(from pink the clipboard bitch).
  //
  // If a client asks for unicode, first check if unicode is present. If not, then 
  // check for plain text. If it's there, say "yes" as we will do the conversion
  // in GetNativeClipboardData(). From this point on, no client will
  // ever ask for text/plain explicitly. If they do, you must ASSERT!
  
#if 0
  *outResult = PR_FALSE;
  PRUint32 length;
  aFlavorList->Count(&length);
  for ( PRUint32 i = 0; i < length; ++i ) {
    nsCOMPtr<nsISupports> genericFlavor;
    aFlavorList->GetElementAt ( i, getter_AddRefs(genericFlavor) );
    nsCOMPtr<nsISupportsString> flavorWrapper ( do_QueryInterface(genericFlavor) );
    if ( flavorWrapper ) {
      nsXPIDLCString flavor;
      flavorWrapper->ToString ( getter_Copies(flavor) );
      
      gint format = GetFormat(flavor);
      if (DoConvert(format)) {
        *outResult = PR_TRUE;
        break;
      }
    }
  }
#ifdef DEBUG_CLIPBOARD
  printf("nsClipboard::HasDataMatchingFlavors() called -- returning %i\n", *outResult);
#endif

#else
  *outResult = PR_TRUE;
#endif

  return NS_OK;

}


/**
 * This is the callback which is called when another app
 * requests the selection.
 *
 * @param  widget The widget
 * @param  aSelectionData Selection data
 * @param  info Value passed in from the callback init
 * @param  time Time when the selection request came in
 */
void nsClipboard::SelectionGetCB(GtkWidget        *widget,
                                 GtkSelectionData *aSelectionData,
                                 guint            aInfo,
                                 guint            aTime)
{ 
#ifdef DEBUG_CLIPBOARD
  printf("nsClipboard::SelectionGetCB\n"); 
#endif /* DEBUG_CLIPBOARD */

  nsClipboard *cb = (nsClipboard *)gtk_object_get_data(GTK_OBJECT(widget),
                                                       "cb");

  void     *clipboardData;
  PRUint32 dataLength;
  nsresult rv;

  // Make sure we have a transferable:
  if (!cb->mTransferable) {
    printf("Clipboard has no transferable!\n");
    return;
  }

#ifdef DEBUG_CLIPBOARD
  g_print("  aInfo == %d -", aInfo);
#endif

  char* dataFlavor = nsnull;

  // switch aInfo (atom) to our enum
  int type = (int)aInfo;
  for (int i=0; i < TARGET_LAST; i++)
  {
    if (sSelTypes[i] == aInfo)
    {
      type = i;
      break;
    }
  }

  PRBool needToDoConversionToPlainText = PR_FALSE;
  switch(type)
    {
    case GDK_TARGET_STRING:
    case TARGET_TEXT_PLAIN:
      // if someone was asking for text/plain, lookup unicode instead so we can convert it.
      dataFlavor = kUnicodeMime;
      needToDoConversionToPlainText = PR_TRUE;
      break;
    case TARGET_TEXT_XIF:
      dataFlavor = kXIFMime;
      break;
    case TARGET_TEXT_UNICODE:
    case TARGET_UTF8:
      dataFlavor = kUnicodeMime;
      break;
    case TARGET_TEXT_HTML:
      dataFlavor = kHTMLMime;
      break;
    case TARGET_AOLMAIL:
      dataFlavor = kAOLMailMime;
      break;
    case TARGET_IMAGE_PNG:
      dataFlavor = kPNGImageMime;
      break;
    case TARGET_IMAGE_JPEG:
      dataFlavor = kJPEGImageMime;
      break;
    case TARGET_IMAGE_GIF:
      dataFlavor = kGIFImageMime;
      break;
    default:
      {
        /* handle outside things */
      }
    }
#ifdef DEBUG_CLIPBOARD
  g_print("- aInfo is for %s\n", gdk_atom_name(aInfo));
#endif
      
  // Get data out of transferable.
  nsCOMPtr<nsISupports> genericDataWrapper;
  rv = cb->mTransferable->GetTransferData(dataFlavor, 
                                          getter_AddRefs(genericDataWrapper),
                                          &dataLength);
  nsPrimitiveHelpers::CreateDataFromPrimitive ( dataFlavor, genericDataWrapper, &clipboardData, dataLength );
  if (NS_SUCCEEDED(rv) && clipboardData && dataLength > 0) {
    size_t size = 1;
    // find the number of bytes in the data for the below thing
    //    size_t size = sizeof((void*)((unsigned char)clipboardData[0]));
    //    g_print("************  *****************      ******************* %i\n", size);
    
    // if required, do the extra work to convert unicode to plain text and replace the output
    // values with the plain text.
    if ( needToDoConversionToPlainText ) {
      char* plainTextData = nsnull;
      PRUnichar* castedUnicode = NS_REINTERPRET_CAST(PRUnichar*, clipboardData);
      PRInt32 plainTextLen = 0;
      nsPrimitiveHelpers::ConvertUnicodeToPlatformPlainText ( castedUnicode, dataLength / 2, &plainTextData, &plainTextLen );
      if ( clipboardData ) {
        nsAllocator::Free(NS_REINTERPRET_CAST(char*, clipboardData));
        clipboardData = plainTextData;
        dataLength = plainTextLen;
      }
    }

    if ( clipboardData )
      gtk_selection_data_set(aSelectionData,
                             aInfo, size*8,
                             NS_REINTERPRET_CAST(unsigned char *, clipboardData),
                             dataLength);
    nsCRT::free ( NS_REINTERPRET_CAST(char*, clipboardData) );
  }
  else
    printf("Transferable didn't support data flavor %s (type = %d)\n",
           dataFlavor ? dataFlavor : "None", type);
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
  printf("  nsClipboard::SelectionClearCB\n");
#endif /* DEBUG_CLIPBOARD */

  nsClipboard *cb = (nsClipboard *)gtk_object_get_data(GTK_OBJECT(aWidget),
                                                       "cb");

  cb->EmptyClipboard();
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
  printf("  nsClipboard::SelectionRequestCB\n");
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
  printf("  nsClipboard::SelectionNotifyCB\n");
#endif /* DEBUG_CLIPBOARD */
}
