/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code mozilla.org code.
 *
 * The Initial Developer of the Original Code Christopher Blizzard
 * <blizzard@mozilla.org>.  Portions created by the Initial Developer
 * are Copyright (C) 2001 the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "nsAppShell.h"
#include "nsIEventQueueService.h"
#include "nsIServiceManagerUtils.h"
#include "plhash.h"
#include "prenv.h"

#include <gtk/gtkmain.h>

static PRBool sInitialized = PR_FALSE;
static PLHashTable *sQueueHashTable = nsnull;
static PLHashTable *sCountHashTable = nsnull;

static gboolean event_processor_callback (GIOChannel *source,
					  GIOCondition condition,
					  gpointer data)
{
  nsIEventQueue *eventQueue = (nsIEventQueue *)data;
  if (eventQueue)
    eventQueue->ProcessPendingEvents();

  // always remove the source event
  return TRUE;
}

#define NUMBER_HASH_KEY(_num) ((PLHashNumber) _num)

static PLHashNumber
IntHashKey(PRInt32 key)
{
  return NUMBER_HASH_KEY(key);
}

nsAppShell::nsAppShell(void)
{
  NS_INIT_REFCNT();
}

nsAppShell::~nsAppShell(void)
{
}

NS_IMPL_ISUPPORTS1(nsAppShell, nsIAppShell)

NS_IMETHODIMP
nsAppShell::Create(int *argc, char **argv)
{
  if (sInitialized)
    return NS_OK;

  sInitialized = PR_TRUE;

  // XXX add all of the command line handling

  gtk_init(argc, &argv);
  
  if (PR_GetEnv("MOZ_DEBUG_PAINTS")) {
    gdk_window_set_debug_updates(TRUE);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsAppShell::Run(void)
{
  if (!mEventQueue)
    Spinup();

  if (!mEventQueue)
    return NS_ERROR_NOT_INITIALIZED;

  // go go gadget gtk2!
  gtk_main();

  Spindown();
  
  return NS_OK;
}

NS_IMETHODIMP
nsAppShell::Spinup(void)
{
  nsresult rv = NS_OK;

  // get the event queue service
  nsCOMPtr <nsIEventQueueService> eventQService = 
    do_GetService(NS_EVENTQUEUESERVICE_CONTRACTID, &rv);

  if (NS_FAILED(rv)) {
    NS_WARNING("Failed to get event queue service");
    return rv;
  }

  // get the event queue for this thread
  rv = eventQService->GetThreadEventQueue(NS_CURRENT_THREAD,
					  getter_AddRefs(mEventQueue));

  // if we got an event queue, just use it
  if (mEventQueue)
    goto done;

  // otherwise creaet a new event queue for the thread
  rv = eventQService->CreateThreadEventQueue();
  if (NS_FAILED(rv)) {
    NS_WARNING("Could not create the thread event queue");
    return rv;
  }

  // ask again for the event queue now that we have create one.
  rv = eventQService->GetThreadEventQueue(NS_CURRENT_THREAD,
					  getter_AddRefs(mEventQueue));

 done:
  ListenToEventQueue(mEventQueue, PR_TRUE);

  return rv;
}

NS_IMETHODIMP
nsAppShell::Spindown(void)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsAppShell::ListenToEventQueue(nsIEventQueue *aQueue, PRBool aListen)
{
  // initialize our hash tables if we have to
  if (!sQueueHashTable)
    sQueueHashTable = PL_NewHashTable(3, (PLHashFunction)IntHashKey,
				      PL_CompareValues, PL_CompareValues, 0, 0);
  if (!sCountHashTable)
    sCountHashTable = PL_NewHashTable(3, (PLHashFunction)IntHashKey,
				      PL_CompareValues, PL_CompareValues, 0, 0);

  if (aListen) {
    /* add a listener */
    PRInt32 key = aQueue->GetEventQueueSelectFD();

    /* only add if we arn't already in the table */
    if (!PL_HashTableLookup(sQueueHashTable, GINT_TO_POINTER(key))) {
      GIOChannel *ioc;
      ioc = g_io_channel_unix_new(key);
      g_io_add_watch_full (ioc, G_PRIORITY_HIGH_IDLE,
			   G_IO_IN,
			   event_processor_callback, aQueue, NULL);
      
      if (ioc) {
        PL_HashTableAdd(sQueueHashTable, GINT_TO_POINTER(key),
			ioc);
      }
    }
    /* bump up the count */
    gint count = GPOINTER_TO_INT(PL_HashTableLookup(sCountHashTable, 
						    GINT_TO_POINTER(key)));
    PL_HashTableAdd(sCountHashTable, GINT_TO_POINTER(key), 
		    GINT_TO_POINTER(count+1));
  } else {
    /* remove listener */
    PRInt32 key = aQueue->GetEventQueueSelectFD();
    
    gint count = GPOINTER_TO_INT(PL_HashTableLookup(sCountHashTable,
						    GINT_TO_POINTER(key)));
    if (count - 1 == 0) {
      GIOChannel *ioc = (GIOChannel *)PL_HashTableLookup(sQueueHashTable,
							 GINT_TO_POINTER(key));
      if (ioc) {
	g_io_channel_shutdown (ioc, TRUE, NULL);
	g_io_channel_unref(ioc);
        PL_HashTableRemove(sQueueHashTable, GINT_TO_POINTER(key));
      }
    }

    // update the count for this key
    PL_HashTableAdd(sCountHashTable, GINT_TO_POINTER(key),
		    GINT_TO_POINTER(count-1));
  }

  return NS_OK;
}

NS_IMETHODIMP
nsAppShell::GetNativeEvent(PRBool &aRealEvent, void * &aEvent)
{
  aRealEvent = PR_FALSE;
  aEvent = 0;

  return NS_OK;
}

NS_IMETHODIMP
nsAppShell::DispatchNativeEvent(PRBool aRealEvent, void *aEvent)
{
  if (!mEventQueue)
    return NS_ERROR_NOT_INITIALIZED;
  
  g_main_context_iteration(NULL, TRUE);

  return NS_OK;
}

NS_IMETHODIMP
nsAppShell::SetDispatchListener(nsDispatchListener *aDispatchListener)
{
  return NS_OK;
}

NS_IMETHODIMP
nsAppShell::Exit(void)
{
  gtk_main_quit();
  return NS_OK;
}
