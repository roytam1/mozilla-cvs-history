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

#include "nsDragService.h"

#include "nsITransferable.h"
#include "nsString.h"
#include "nsClipboard.h"
#include "nsIRegion.h"
#include "nsVoidArray.h"
#include "nsISupportsPrimitives.h"
#include "nsPrimitiveHelpers.h"
#include "nsCOMPtr.h"
#include "nsXPIDLString.h"

#include "nsWidgetsCID.h"
#include "nsPhWidgetLog.h"
#include "nslog.h"

NS_IMPL_LOG(nsDragServiceLog)
#define PRINTF(args) NS_LOG_PRINTF(nsDragServiceLog, args)
#define FLUSH()      NS_LOG_FLUSH(nsDragServiceLog)

NS_IMPL_ADDREF_INHERITED(nsDragService, nsBaseDragService)
NS_IMPL_RELEASE_INHERITED(nsDragService, nsBaseDragService)
NS_IMPL_QUERY_INTERFACE2(nsDragService, nsIDragService, nsIDragSession)

#define DEBUG_DRAG 1

//-------------------------------------------------------------------------
//
// DragService constructor
//
//-------------------------------------------------------------------------
nsDragService::nsDragService()
{
  PR_LOG(PhWidLog, PR_LOG_DEBUG, ("nsDragService::nsDragService this=<%p>", this));

  NS_INIT_REFCNT();
  mWidget = nsnull;
  mNumFlavors = 0;
}

//-------------------------------------------------------------------------
//
// DragService destructor
//
//-------------------------------------------------------------------------
nsDragService::~nsDragService()
{
  PR_LOG(PhWidLog, PR_LOG_DEBUG, ("nsDragService::~nsDragService this=<%p>", this));

  if (mWidget)
  {
	PtDestroyWidget(mWidget);
	mWidget=nsnull;
  }

  /* free the target list */
}

NS_IMETHODIMP nsDragService::StartDragSession()
{
  PR_LOG(PhWidLog, PR_LOG_DEBUG, ("nsDragService::StartDragSession this=<%p>", this));

  NS_WARNING("nsDragService::StartDragSession() - Not Supported Yet");
  nsBaseDragService::StartDragSession();
  
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsDragService::EndDragSession()
{
  PR_LOG(PhWidLog, PR_LOG_DEBUG, ("nsDragService::EndDragSession this=<%p>", this));

  NS_WARNING("nsDragService::EndDragSession()");
  nsBaseDragService::EndDragSession();
  
  //gtk_drag_source_unset(mWidget);

  return NS_ERROR_FAILURE;
}


//-------------------------------------------------------------------------
NS_IMETHODIMP nsDragService::InvokeDragSession (nsIDOMNode *aDOMNode,
						nsISupportsArray *aTransferableArray,
                                                nsIScriptableRegion *aRegion,
                                                PRUint32 aActionType)
{
  nsBaseDragService::InvokeDragSession ( aDOMNode, aTransferableArray, aRegion, aActionType );
  
  PR_LOG(PhWidLog, PR_LOG_DEBUG, ("nsDragService::InvokeDragSession this=<%p> aActionType=<%d>", this, aActionType));
  //mWidget = gtk_invisible_new();
  //gtk_widget_show(mWidget);

  // add the flavors from the transferables. Cache this array for the send data proc
  //GtkTargetList *targetlist = RegisterDragItemsAndFlavors(aTransferableArray);

  switch (aActionType)
  {
    case DRAGDROP_ACTION_NONE:
      //mActionType = GDK_ACTION_DEFAULT;
      break;
    case DRAGDROP_ACTION_COPY:
      //mActionType = GDK_ACTION_COPY;
      break;
    case DRAGDROP_ACTION_MOVE:
      //mActionType = GDK_ACTION_MOVE;
      break;
    case DRAGDROP_ACTION_LINK:
      //mActionType = GDK_ACTION_LINK;
      break;
  }

  StartDragSession();

  // XXX 3rd param ???    &                last param should be a real event...
  //gtk_drag_begin(mWidget, targetlist, mActionType, 1, gdk_event_get());

  return NS_OK;
}



#if 0
GtkTargetList *nsDragService::RegisterDragItemsAndFlavors(nsISupportsArray *inArray)
{
  unsigned int numDragItems = 0;
  inArray->Count(&numDragItems);

  GtkTargetList *targetlist;
  targetlist = gtk_target_list_new(nsnull, numDragItems);

  PRINTF(("nsDragService::RegisterDragItemsAndFlavors"));

  for (unsigned int i = 0; i < numDragItems; ++i)
  {
    nsCOMPtr<nsISupports> genericItem;
    inArray->GetElementAt (i, getter_AddRefs(genericItem));
    nsCOMPtr<nsITransferable> currItem (do_QueryInterface(genericItem));
    if (currItem)
    {
      nsCOMPtr<nsISupportsArray> flavorList;
      if (NS_SUCCEEDED(currItem->FlavorsTransferableCanExport(getter_AddRefs(flavorList))))
      {
        flavorList->Count (&mNumFlavors);
        for (PRUint32 flavorIndex = 0; flavorIndex < mNumFlavors; ++flavorIndex)
        {
          nsCOMPtr<nsISupports> genericWrapper;
          flavorList->GetElementAt ( flavorIndex, getter_AddRefs(genericWrapper) );
          nsCOMPtr<nsISupportsString> currentFlavor ( do_QueryInterface(genericWrapper) );
          if ( currentFlavor )
          {
            nsXPIDLCString flavorStr;
            currentFlavor->ToString ( getter_Copies(flavorStr) );

            // register native flavors
            GdkAtom atom = gdk_atom_intern(flavorStr, PR_TRUE);
            gtk_target_list_add(targetlist, atom, 1, atom);
          }

        } // foreach flavor in item              
      } // if valid flavor list
    } // if item is a transferable
  } // foreach drag item

  return targetlist;
}


/* return PR_TRUE if we have converted or PR_FALSE if we havn't and need to keep being called */
PRBool nsDragService::DoConvert(GdkAtom type)
{

  PRINTF(("nsDragService::DoRealConvert(%li)\n    {", type));
  int e = 0;
  // Set a flag saying that we're blocking waiting for the callback:
  mBlocking = PR_TRUE;

  //
  // ask X what kind of data we can get
  //
#ifdef DEBUG_DRAG
  PRINTF(("     Doing real conversion of atom type '%s'", gdk_atom_name(type)));
#endif
  gtk_selection_convert(mWidget,
                        GDK_SELECTION_PRIMARY,
                        type,
                        GDK_CURRENT_TIME);

  // Now we need to wait until the callback comes in ...
  // i is in case we get a runaway (yuck).
#ifdef DEBUG_DRAG
  PRINTF(("      Waiting for the callback... mBlocking = %d", mBlocking));
#endif /* DEBUG_CLIPBOARD */
  for (e=0; mBlocking == PR_TRUE && e < 1000; ++e)
  {
    gtk_main_iteration_do(PR_TRUE);
  }

#ifdef DEBUG_DRAG
  PRINTF(("    }"));
#endif

  if (mSelectionData.length > 0)
    return PR_TRUE;

  return PR_FALSE;
}

#endif

//-------------------------------------------------------------------------
NS_IMETHODIMP nsDragService::GetNumDropItems (PRUint32 * aNumItems)
{
  PR_LOG(PhWidLog, PR_LOG_DEBUG, ("nsDragService::GetNumDropItems this=<%p>", this));

  *aNumItems = 0;
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_IMETHODIMP nsDragService::GetData (nsITransferable * aTransferable, PRUint32 anItem)
{
  PR_LOG(PhWidLog, PR_LOG_DEBUG, ("nsDragService::GetData this=<%p>", this));

#ifdef DEBUG_DRAG
  PRINTF(("nsClipboard::GetNativeClipboardData()"));
#endif /* DEBUG_CLIPBOARD */


#if 0

  // make sure we have a good transferable
  if (!aTransferable) {
    PRINTF(("  GetData: Transferable is null!"));
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
  for ( PRUint32 i = 0; i < cnt; ++i ) {
    nsCOMPtr<nsISupports> genericFlavor;
    flavorList->GetElementAt ( i, getter_AddRefs(genericFlavor) );
    nsCOMPtr<nsISupportsString> currentFlavor (do_QueryInterface(genericFlavor));
    if ( currentFlavor ) {
      nsXPIDLCString flavorStr;
      currentFlavor->ToString(getter_Copies(flavorStr));
      if (DoConvert(gdk_atom_intern(flavorStr, 1))) {
        foundFlavor = flavorStr;
        break;
      }
    }
  }

#ifdef DEBUG_CLIPBOARD
  PRINTF(("  Got the callback: '%s', %d",
         mSelectionData.data, mSelectionData.length));
#endif /* DEBUG_CLIPBOARD */

  // We're back from the callback, no longer blocking:
  mBlocking = PR_FALSE;

  // 
  // Now we have data in mSelectionData.data.
  // We just have to copy it to the transferable.
  // 

#if 0  
// pinkerton - we have the flavor already from above, so we don't need
// to re-derrive it.
  nsString *name = new nsString((const char*)gdk_atom_name(mSelectionData.type));
  int format = GetFormat(*name);
  df->SetString((const char*)gdk_atom_name(sSelTypes[format]));
#endif

  nsCOMPtr<nsISupports> genericDataWrapper;
  nsPrimitiveHelpers::CreatePrimitiveForData ( foundFlavor, mSelectionData.data, mSelectionData.length, getter_AddRefs(genericDataWrapper) );
  aTransferable->SetTransferData(foundFlavor,
                                 genericDataWrapper,
                                 mSelectionData.length);

  //delete name;
  
  // transferable is now copying the data, so we can free it.
  //  g_free(mSelectionData.data);
  mSelectionData.data = nsnull;
  mSelectionData.length = 0;

  gtk_drag_source_unset(mWidget);

#endif
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_IMETHODIMP nsDragService::IsDataFlavorSupported(const char *aDataFlavor, PRBool *_retval)
{
  PR_LOG(PhWidLog, PR_LOG_DEBUG, ("nsDragService::IsDataFlavorSupported this=<%p>", this));

  PRINTF(("nsDragService::IsDataFlavorSupported"));
  if (!aDataFlavor || !_retval)
    return NS_ERROR_FAILURE;

  *_retval = PR_TRUE;
  return NS_OK;
}

#if 0
//-------------------------------------------------------------------------
NS_IMETHODIMP nsDragService::GetCurrentSession (nsIDragSession **aSession)
{
  PR_LOG(PhWidLog, PR_LOG_DEBUG, ("nsDragService::GetCurrentSession this=<%p>", this));

  PRINTF(("nsDragService::GetCurrentSession"));
  if (!aSession)
    return NS_ERROR_FAILURE;

  *aSession = (nsIDragSession *)this;
  NS_ADDREF(*aSession);
  return NS_OK;
}
#endif

#if 0
//-------------------------------------------------------------------------
void  
nsDragService::DragLeave (GtkWidget	       *widget,
			                    GdkDragContext   *context,
			                    guint             time)
{
  PRINTF(("leave"));
  //gHaveDrag = PR_FALSE;
}

//-------------------------------------------------------------------------
PRBool
nsDragService::DragMotion(GtkWidget	       *widget,
			                    GdkDragContext   *context,
			                    gint              x,
			                    gint              y,
			                    guint             time)
{
  PRINTF(("drag motion"));
  GtkWidget *source_widget;

#if 0
  if (!gHaveDrag) {
      gHaveDrag = PR_TRUE;
  }
#endif

  source_widget = gtk_drag_get_source_widget (context);
  PRINTF(("motion, source %s", source_widget ?
	          gtk_type_name (GTK_OBJECT (source_widget)->klass->type) :
         "unknown"));

  gdk_drag_status (context, context->suggested_action, time);
  
  return PR_TRUE;
}

//-------------------------------------------------------------------------
PRBool
nsDragService::DragDrop(GtkWidget	       *widget,
			                  GdkDragContext   *context,
			                  gint              x,
			                  gint              y,
			                  guint             time)
{
  PRINTF(("drop"));
  //gHaveDrag = PR_FALSE;

  if (context->targets){
    gtk_drag_get_data (widget, context, 
			                 GPOINTER_TO_INT (context->targets->data), 
			                 time);
    return PR_TRUE;
  }
  
  return PR_FALSE;
}

//-------------------------------------------------------------------------
void  
nsDragService::DragDataReceived  (GtkWidget          *widget,
			                            GdkDragContext     *context,
			                            gint                x,
			                            gint                y,
			                            GtkSelectionData   *data,
			                            guint               info,
			                            guint               time)
{
  if ((data->length >= 0) && (data->format == 8)) {
    PRINTF( ("Received \"%s\"", (gchar *)data->data));
    gtk_drag_finish (context, PR_TRUE, PR_FALSE, time);
    return;
  }
  
  gtk_drag_finish (context, PR_FALSE, PR_FALSE, time);
}
  
//-------------------------------------------------------------------------
void  
nsDragService::DragDataGet(GtkWidget          *widget,
		                       GdkDragContext     *context,
		                       GtkSelectionData   *selection_data,
		                       guint               info,
		                       guint               time,
		                       gpointer            data)
{
  gtk_selection_data_set (selection_data,
                          selection_data->target,
                          8, (guchar *)"I'm Data!", 9);
}

//-------------------------------------------------------------------------
void  
nsDragService::DragDataDelete(GtkWidget          *widget,
			                        GdkDragContext     *context,
			                        gpointer            data)
{
  PRINTF( ("Delete the data!"));
}
#endif
