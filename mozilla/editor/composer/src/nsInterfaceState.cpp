/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Pierre Phaneuf <pp@ludusdesign.com>
 *
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */



#include "nsCOMPtr.h"
#include "nsIServiceManager.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsISelection.h"
#include "nsIScriptGlobalObject.h"
#include "nsIDOMWindowInternal.h"
#include "nsITimer.h"

#include "nsIEditor.h"
#include "nsIHTMLEditor.h"
#include "nsITransactionManager.h"

#include "nsInterfaceState.h"

nsInterfaceState::nsInterfaceState()
:  mEditor(nsnull)
,  mChromeDoc(nsnull)
,  mDOMWindow(nsnull)
,  mDirtyState(eStateUninitialized)
,  mSelectionCollapsed(eStateUninitialized)
,  mFirstDoOfFirstUndo(PR_TRUE)
,  mBatchDepth(0)
{
	NS_INIT_ISUPPORTS();
}

nsInterfaceState::~nsInterfaceState()
{
}

NS_IMPL_ADDREF(nsInterfaceState);
NS_IMPL_RELEASE(nsInterfaceState);
NS_IMPL_QUERY_INTERFACE4(nsInterfaceState, nsISelectionListener, nsIDocumentStateListener, nsITransactionListener, nsITimerCallback);

NS_IMETHODIMP
nsInterfaceState::Init(nsIHTMLEditor* aEditor, nsIDOMDocument *aChromeDoc)
{
  if (!aEditor)
    return NS_ERROR_INVALID_ARG;

  if (!aChromeDoc)
    return NS_ERROR_INVALID_ARG;

  mEditor = aEditor;		// no addreffing here
  mChromeDoc = aChromeDoc;
  
  return NS_OK;
}

NS_IMETHODIMP
nsInterfaceState::NotifyDocumentCreated()
{
  return NS_OK;
}

NS_IMETHODIMP
nsInterfaceState::NotifyDocumentWillBeDestroyed()
{
  // cancel any outstanding udpate timer
  if (mUpdateTimer)
    mUpdateTimer->Cancel();
  
  return NS_OK;
}


NS_IMETHODIMP
nsInterfaceState::NotifyDocumentStateChanged(PRBool aNowDirty)
{
  // update document modified. We should have some other notifications for this too.
  return UpdateDirtyState(aNowDirty);
}

NS_IMETHODIMP
nsInterfaceState::NotifySelectionChanged(nsIDOMDocument *, nsISelection *, short)
{
  return PrimeUpdateTimer();
}

#ifdef XP_MAC
#pragma mark -
#endif


NS_IMETHODIMP nsInterfaceState::WillDo(nsITransactionManager *aManager,
  nsITransaction *aTransaction, PRBool *aInterrupt)
{
  *aInterrupt = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP nsInterfaceState::DidDo(nsITransactionManager *aManager,
  nsITransaction *aTransaction, nsresult aDoResult)
{
  if (mBatchDepth == 0)
    UpdateUndoCommands(aManager);
  return NS_OK;
}

NS_IMETHODIMP nsInterfaceState::WillUndo(nsITransactionManager *aManager,
  nsITransaction *aTransaction, PRBool *aInterrupt)
{
  *aInterrupt = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP nsInterfaceState::DidUndo(nsITransactionManager *aManager,
  nsITransaction *aTransaction, nsresult aUndoResult)
{
  PRInt32 undoCount;
  aManager->GetNumberOfUndoItems(&undoCount);
  if (undoCount == 0)
    mFirstDoOfFirstUndo = PR_TRUE;    // reset the state for the next do

  CallUpdateCommands(NS_LITERAL_STRING("undo"));
  return NS_OK;
}

NS_IMETHODIMP nsInterfaceState::WillRedo(nsITransactionManager *aManager,
  nsITransaction *aTransaction, PRBool *aInterrupt)
{
  *aInterrupt = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP nsInterfaceState::DidRedo(nsITransactionManager *aManager,  
  nsITransaction *aTransaction, nsresult aRedoResult)
{
  CallUpdateCommands(NS_LITERAL_STRING("undo"));
  return NS_OK;
}

NS_IMETHODIMP nsInterfaceState::WillBeginBatch(nsITransactionManager *aManager, PRBool *aInterrupt)
{
  *aInterrupt = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP nsInterfaceState::DidBeginBatch(nsITransactionManager *aManager, nsresult aResult)
{
  ++mBatchDepth;
  return NS_OK;
}

NS_IMETHODIMP nsInterfaceState::WillEndBatch(nsITransactionManager *aManager, PRBool *aInterrupt)
{
  *aInterrupt = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP nsInterfaceState::DidEndBatch(nsITransactionManager *aManager, nsresult aResult)
{
  --mBatchDepth;
  if (mBatchDepth == 0)
    UpdateUndoCommands(aManager);

  return NS_OK;
}

NS_IMETHODIMP nsInterfaceState::WillMerge(nsITransactionManager *aManager,
        nsITransaction *aTopTransaction, nsITransaction *aTransactionToMerge, PRBool *aInterrupt)
{
  *aInterrupt = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP nsInterfaceState::DidMerge(nsITransactionManager *aManager,
  nsITransaction *aTopTransaction, nsITransaction *aTransactionToMerge,
                    PRBool aDidMerge, nsresult aMergeResult)
{
  return NS_OK;
}

#ifdef XP_MAC
#pragma mark -
#endif


nsresult nsInterfaceState::PrimeUpdateTimer()
{
  nsresult rv = NS_OK;
    
  if (mUpdateTimer)
  {
    // i'd love to be able to just call SetDelay on the existing timer, but
    // i think i have to tear it down and make a new one.
    mUpdateTimer->Cancel();
    mUpdateTimer = NULL;      // free it
  }
  
  mUpdateTimer = do_CreateInstance("@mozilla.org/timer;1", &rv);
  if (NS_FAILED(rv)) return rv;

  const PRUint32 kUpdateTimerDelay = 150;
  return mUpdateTimer->InitWithCallback(NS_STATIC_CAST(nsITimerCallback*, this), 
                                        kUpdateTimerDelay,
                                        nsITimer::TYPE_ONE_SHOT);
}


nsresult nsInterfaceState::UpdateUndoCommands(nsITransactionManager *aManager)
{
  // only need to update if the status of the Undo menu item changes.
  PRInt32 undoCount;
  aManager->GetNumberOfUndoItems(&undoCount);
  if (undoCount == 1)
  {
    if (mFirstDoOfFirstUndo)
      CallUpdateCommands(NS_LITERAL_STRING("undo"));
    mFirstDoOfFirstUndo = PR_FALSE;
  }

  return NS_OK;
}

void nsInterfaceState::TimerCallback()
{
  // if the selection state has changed, update stuff
  PRBool isCollapsed = SelectionIsCollapsed();
  if (isCollapsed != mSelectionCollapsed)
  {
    CallUpdateCommands(NS_LITERAL_STRING("select"));
    mSelectionCollapsed = isCollapsed;
  }
  
  CallUpdateCommands(NS_LITERAL_STRING("style"));
}

nsresult
nsInterfaceState::UpdateDirtyState(PRBool aNowDirty)
{
  if (mDirtyState != aNowDirty)
  {
    CallUpdateCommands(NS_LITERAL_STRING("save"));

    mDirtyState = aNowDirty;
  }
  
  return NS_OK;  
}

nsresult nsInterfaceState::CallUpdateCommands(const nsAString& aCommand)
{
  if (!mDOMWindow)
  {
    nsCOMPtr<nsIEditor> editor = do_QueryInterface(mEditor);
    if (!editor) return NS_ERROR_FAILURE;

    nsCOMPtr<nsIDOMDocument> domDoc;
    editor->GetDocument(getter_AddRefs(domDoc));
    if (!domDoc) return NS_ERROR_FAILURE;

    nsCOMPtr<nsIDocument> theDoc = do_QueryInterface(domDoc);
    if (!theDoc) return NS_ERROR_FAILURE;

    nsCOMPtr<nsIScriptGlobalObject> scriptGlobalObject;
    theDoc->GetScriptGlobalObject(getter_AddRefs(scriptGlobalObject));

    nsCOMPtr<nsIDOMWindowInternal> domWindow = do_QueryInterface(scriptGlobalObject);
    if (!domWindow) return NS_ERROR_FAILURE;
    mDOMWindow = domWindow;
  }
  
  return mDOMWindow->UpdateCommands(aCommand);
}

PRBool
nsInterfaceState::SelectionIsCollapsed()
{
  nsresult rv;
  // we don't care too much about failures here.
  nsCOMPtr<nsIEditor> editor = do_QueryInterface(mEditor, &rv);
  if (NS_SUCCEEDED(rv))
  {
    nsCOMPtr<nsISelection> domSelection;
    rv = editor->GetSelection(getter_AddRefs(domSelection));
    if (NS_SUCCEEDED(rv))
    {    
      PRBool selectionCollapsed = PR_FALSE;
      rv = domSelection->GetIsCollapsed(&selectionCollapsed);
      return selectionCollapsed;
    }
  }
  return PR_FALSE;
}


#ifdef XP_MAC
#pragma mark -
#endif


NS_IMETHODIMP
nsInterfaceState::Notify(nsITimer *timer)
{
  NS_ASSERTION(timer == mUpdateTimer.get(), "Hey, this ain't my timer!");
  mUpdateTimer = NULL;    // release my hold  
  TimerCallback();
  return NS_OK;
}

#ifdef XP_MAC
#pragma mark -
#endif


nsresult NS_NewInterfaceState(nsIHTMLEditor* aEditor, nsIDOMDocument* aChromeDoc, nsISelectionListener** aInstancePtrResult)
{
  nsInterfaceState* newThang = new nsInterfaceState;
  if (!newThang)
    return NS_ERROR_OUT_OF_MEMORY;

  *aInstancePtrResult = nsnull;
  nsresult rv = newThang->Init(aEditor, aChromeDoc);
  if (NS_FAILED(rv))
  {
    delete newThang;
    return rv;
  }
      
  return newThang->QueryInterface(NS_GET_IID(nsISelectionListener), (void **)aInstancePtrResult);
}
