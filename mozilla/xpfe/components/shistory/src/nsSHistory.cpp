/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 * The Original Code is the Mozilla browser.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications, Inc.  Portions created by Netscape are
 * Copyright (C) 1999, Mozilla.  All Rights Reserved.
 * 
 * Contributor(s):
 *   Radha Kulkarni <radha@netscape.com>
 */

// Local Includes 
#include "nsSHistory.h"

// Helper Classes
#include "nsXPIDLString.h"

// Interfaces Needed
#include "nsILayoutHistoryState.h"
#include "nsIDocShellLoadInfo.h"
#include "nsXPIDLString.h"
#include "nsISHContainer.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDocShellTreeNode.h"
#include "nsIDocShellLoadInfo.h"
#include "nsIServiceManager.h"
#include "nsIPref.h"

#define PREF_SHISTORY_SIZE "browser.sessionhistory.max_entries"
static PRInt32  gHistoryMaxSize = 0;

//*****************************************************************************
//***    nsSHistory: Object Management
//*****************************************************************************

nsSHistory::nsSHistory() : mListRoot(nsnull), mIndex(-1), mLength(0)
{
  NS_INIT_REFCNT();
}


nsSHistory::~nsSHistory()
{
}

//*****************************************************************************
//    nsSHistory: nsISupports
//*****************************************************************************

NS_IMPL_ADDREF(nsSHistory)
NS_IMPL_RELEASE(nsSHistory)

NS_INTERFACE_MAP_BEGIN(nsSHistory)
   NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsISHistory)
   NS_INTERFACE_MAP_ENTRY(nsISHistory)
   NS_INTERFACE_MAP_ENTRY(nsIWebNavigation)
   NS_INTERFACE_MAP_ENTRY(nsISHistoryInternal)
NS_INTERFACE_MAP_END

//*****************************************************************************
//    nsSHistory: nsISHistory
//*****************************************************************************

/*
 * Init method to get pref settings
 */
NS_IMETHODIMP
nsSHistory::Init()
{
  nsresult res;
  nsCOMPtr<nsIPref> prefs = do_GetService(NS_PREF_CONTRACTID, &res);
  if (NS_SUCCEEDED(res) && prefs) {
    prefs->GetIntPref(PREF_SHISTORY_SIZE, &gHistoryMaxSize);
  }
  return res;
}


/* Add an entry to the History list at mIndex and 
 * increment the index to point to the new entry
 */
NS_IMETHODIMP
nsSHistory::AddEntry(nsISHEntry * aSHEntry, PRBool aPersist)
{
  NS_ENSURE_ARG(aSHEntry);

  nsCOMPtr<nsISHTransaction> currentTxn;

  if(mListRoot)
    GetTransactionAtIndex(mIndex, getter_AddRefs(currentTxn));

  PRBool currentPersist = PR_TRUE;
  if(currentTxn)
    currentTxn->GetPersist(&currentPersist);

  if(!currentPersist)
  {
    NS_ENSURE_SUCCESS(currentTxn->SetSHEntry(aSHEntry),NS_ERROR_FAILURE);
    currentTxn->SetPersist(aPersist);
    return NS_OK;
  }

  nsCOMPtr<nsISHTransaction> txn(do_CreateInstance(NS_SHTRANSACTION_CONTRACTID));
  NS_ENSURE_TRUE(txn, NS_ERROR_FAILURE);

  // Notify any listener about the new addition
  if (mListener) {
    nsCOMPtr<nsISHistoryListener> listener(do_QueryInterface(mListener));
    if (listener) {
      nsCOMPtr<nsIURI> uri;
      nsCOMPtr<nsIHistoryEntry> hEntry(do_QueryInterface(aSHEntry));
      if (hEntry) {
        hEntry->GetURI(getter_AddRefs(uri));
        listener->OnHistoryNewEntry(uri);
      }
    }
  }

  // Set the ShEntry and parent for the transaction. setting the 
  // parent will properly set the parent child relationship
  txn->SetPersist(aPersist);
  NS_ENSURE_SUCCESS(txn->Create(aSHEntry, currentTxn), NS_ERROR_FAILURE);
   
  // A little tricky math here...  Basically when adding an object regardless of
  // what the length was before, it should always be set back to the current and
  // lop off the forward.
  mLength = (++mIndex + 1);

  // If this is the very first transaction, initialize the list
  if(!mListRoot)
    mListRoot = txn;

  //Purge History list if it is too long
  if ((gHistoryMaxSize >= 0) && (mLength > gHistoryMaxSize))
    PurgeHistory(mLength-gHistoryMaxSize);
  
  return NS_OK;
}

/* Get size of the history list */
NS_IMETHODIMP
nsSHistory::GetCount(PRInt32 * aResult)
{
    NS_ENSURE_ARG_POINTER(aResult);
	*aResult = mLength;
	return NS_OK;
}

/* Get index of the history list */
NS_IMETHODIMP
nsSHistory::GetIndex(PRInt32 * aResult)
{
    NS_ENSURE_ARG_POINTER(aResult);
	*aResult = mIndex;
	return NS_OK;
}

NS_IMETHODIMP
nsSHistory::GetEntryAtIndex(PRInt32 aIndex, PRBool aModifyIndex, nsISHEntry** aResult)
{
  nsresult rv;
  nsCOMPtr<nsISHTransaction> txn;

  /* GetTransactionAtIndex ensures aResult is valid and validates aIndex */
  rv = GetTransactionAtIndex(aIndex, getter_AddRefs(txn));
  if (NS_SUCCEEDED(rv) && txn) {
    //Get the Entry from the transaction
    rv = txn->GetSHEntry(getter_AddRefs(aResult));
    if (NS_SUCCEEDED(rv) && (*aResult)) {
      // Set mIndex to the requested index, if asked to do so..
      if (aModifyIndex) {
        mIndex = aIndex;
      }
    } //entry
  }  //Transaction
  return rv;
}


/* Get the entry at a given index */
NS_IMETHODIMP
nsSHistory::GetEntryAtIndex(PRInt32 aIndex, PRBool aModifyIndex, nsIHistoryEntry** aResult)
{
  nsresult rv;
  nsCOMPtr<nsISHEntry> shEntry;
  rv = GetEntryAtIndex(aIndex, aModifyIndex, getter_AddRefs(shEntry));
  if (NS_SUCCEEDED(rv) && shEntry) 
    rv = CallQueryInterface(shEntry, aResult);
 
  return rv;
}

/* Get the transaction at a given index */
NS_IMETHODIMP
nsSHistory::GetTransactionAtIndex(PRInt32 aIndex, nsISHTransaction ** aResult)
{
     nsresult rv;
     NS_ENSURE_ARG_POINTER(aResult);

     if ((mLength <= 0) || (aIndex < 0) || (aIndex >= mLength))
	   return NS_ERROR_FAILURE;

     if (!mListRoot) 
         return NS_ERROR_FAILURE;

     if (aIndex == 0)
	 {
	    *aResult = mListRoot;
	    NS_ADDREF(*aResult);
	    return NS_OK;
	 } 
	 PRInt32   cnt=0;
	 nsCOMPtr<nsISHTransaction>  tempPtr;
       
       rv = GetRootTransaction(getter_AddRefs(tempPtr));
       if (!NS_SUCCEEDED(rv) || !tempPtr)
               return NS_ERROR_FAILURE;

     while(1) {
       nsCOMPtr<nsISHTransaction> ptr;
	   rv = tempPtr->GetNext(getter_AddRefs(ptr));
	   if (NS_SUCCEEDED(rv) && ptr) {
          cnt++;
		  if (cnt == aIndex) {
			  *aResult = ptr;
			  NS_ADDREF(*aResult);
			  break;
		  }
		  else {
            tempPtr = ptr;
            continue;
		  }
	   }  //NS_SUCCEEDED
	   else 
		   return NS_ERROR_FAILURE;
       }  // while 
  
   return NS_OK;
}

NS_IMETHODIMP
nsSHistory::PrintHistory()
{

      nsCOMPtr<nsISHTransaction>   txn;
      PRInt32 index = 0;
      nsresult rv;

      if (!mListRoot) 
              return NS_ERROR_FAILURE;

      txn = mListRoot;
    
      while (1) {
		      if (!txn)
			     break;
              nsCOMPtr<nsISHEntry>  entry;
              rv = txn->GetSHEntry(getter_AddRefs(entry));
              if (!NS_SUCCEEDED(rv) && !entry)
                      return NS_ERROR_FAILURE;

              nsCOMPtr<nsILayoutHistoryState> layoutHistoryState;
              nsCOMPtr<nsIURI>  uri;
              PRUnichar *  title;
              
              nsXPIDLCString  url;

              entry->GetLayoutHistoryState(getter_AddRefs(layoutHistoryState));
              nsCOMPtr<nsIHistoryEntry> hEntry(do_QueryInterface(entry));
              if (hEntry) {
                hEntry->GetURI(getter_AddRefs(uri));
                hEntry->GetTitle(&title);              
              }
              
			  if (uri)
                 uri->GetSpec(getter_Copies(url));

              #if 0
			  char * titleCStr=nsnull;

			  nsString titlestr(title);
              titleCStr = titlestr.ToNewCString();
              printf("**** SH Transaction #%d, Entry = %x\n", index, entry.get());
              printf("\t\t URL = %s\n", url);
              printf("\t\t Title = %s\n", titleCStr);
              printf("\t\t layout History Data = %x\n", layoutHistoryState);
			  Recycle(titleCStr);
              #endif
      
              Recycle(title);
              

              nsCOMPtr<nsISHTransaction> next;
              rv = txn->GetNext(getter_AddRefs(next));
              if (NS_SUCCEEDED(rv) && next) {
                      txn = next;
                      index++;
                      continue;
              }
              else
                      break;
      }
      
  return NS_OK;
}


NS_IMETHODIMP
nsSHistory::GetRootTransaction(nsISHTransaction ** aResult)
{
    nsCOMPtr<nsISHEntry>   entry;

    NS_ENSURE_ARG_POINTER(aResult);
    *aResult=mListRoot;
      NS_IF_ADDREF(*aResult);
      return NS_OK;
}

/* Get the max size of the history list */
NS_IMETHODIMP
nsSHistory::GetMaxLength(PRInt32 * aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = gHistoryMaxSize;
  return NS_OK;
}

/* Set the max size of the history list */
NS_IMETHODIMP
nsSHistory::SetMaxLength(PRInt32 aMaxSize)
{
  if (aMaxSize < 0)
    return NS_ERROR_ILLEGAL_VALUE;

  gHistoryMaxSize = aMaxSize;
  return NS_OK;
}

NS_IMETHODIMP
nsSHistory::PurgeHistory(PRInt32 aEntries)
{
  if (mLength <= 0 || aEntries <= 0)
    return NS_ERROR_FAILURE;
        
  PRBool purgeHistory = PR_TRUE;
  // Notify the listener about the history purge
  if (mListener) {
    nsCOMPtr<nsISHistoryListener> listener(do_QueryInterface(mListener));
    if (listener) {
      listener->OnHistoryPurge(aEntries, &purgeHistory);
    } 
  }

  if (!purgeHistory) {
    // Listener asked us not to purge
    return NS_OK;
  }

  PRInt32 cnt = 0;
  while (cnt < aEntries) {
    nsCOMPtr<nsISHTransaction>  txn = mListRoot;
    nsCOMPtr<nsISHTransaction> nextTxn;
    if (mListRoot)
      mListRoot->GetNext(getter_AddRefs(nextTxn));
    txn = nsnull;
    mListRoot = nextTxn;
    cnt++;        
  }
  mLength -= cnt;
  mIndex -= cnt;
  return NS_OK;
}


NS_IMETHODIMP
nsSHistory::AddSHistoryListener(nsISHistoryListener * aListener)
{
  NS_ENSURE_ARG_POINTER(aListener);

  // Check if the listener supports Weak Reference. This is a must.
  // This listener functionality is used by embedders and we want to 
  // have the right ownership with who ever listens to SHistory
  nsWeakPtr listener = getter_AddRefs(NS_GetWeakReference(aListener));
  if (!listener) return NS_ERROR_FAILURE;
  mListener = listener;
  return NS_OK;
}


NS_IMETHODIMP
nsSHistory::RemoveSHistoryListener(nsISHistoryListener * aListener)
{
  mListener = nsnull;
  return NS_OK;
}

//*****************************************************************************
//    nsSHistory: nsIWebNavigation
//*****************************************************************************

NS_IMETHODIMP
nsSHistory::GetCanGoBack(PRBool * aCanGoBack)
{
   NS_ENSURE_ARG_POINTER(aCanGoBack);
   *aCanGoBack = PR_FALSE;

   PRInt32 index = -1;
   NS_ENSURE_SUCCESS(GetIndex(&index), NS_ERROR_FAILURE);
   if(index > 0)
      *aCanGoBack = PR_TRUE;

   return NS_OK;
}

NS_IMETHODIMP
nsSHistory::GetCanGoForward(PRBool * aCanGoForward)
{
    NS_ENSURE_ARG_POINTER(aCanGoForward);
   *aCanGoForward = PR_FALSE;

   PRInt32 index = -1;
   PRInt32 count = -1;

   NS_ENSURE_SUCCESS(GetIndex(&index), NS_ERROR_FAILURE);
   NS_ENSURE_SUCCESS(GetCount(&count), NS_ERROR_FAILURE);

   if((index >= 0) && (index < (count - 1)))
      *aCanGoForward = PR_TRUE;

   return NS_OK;
}

NS_IMETHODIMP
nsSHistory::GoBack()
{
	PRBool canGoBack = PR_FALSE;

	GetCanGoBack(&canGoBack);
	if (!canGoBack)  // Can't go back
		return NS_ERROR_UNEXPECTED;
  return GotoIndex(mIndex-1);
}


NS_IMETHODIMP
nsSHistory::GoForward()
{
	PRBool canGoForward = PR_FALSE;

	GetCanGoForward(&canGoForward);
	if (!canGoForward)  // Can't go forward
		return NS_ERROR_UNEXPECTED;
  return GotoIndex(mIndex+1);

}

NS_IMETHODIMP
nsSHistory::Reload(PRUint32 aReloadFlags)
{
  nsresult rv;
	nsDocShellInfoLoadType loadType;
	if (aReloadFlags & nsIWebNavigation::LOAD_FLAGS_BYPASS_PROXY && 
	    aReloadFlags & nsIWebNavigation::LOAD_FLAGS_BYPASS_CACHE)
	{
		loadType = nsIDocShellLoadInfo::loadReloadBypassProxyAndCache;
	}
	else if (aReloadFlags & nsIWebNavigation::LOAD_FLAGS_BYPASS_PROXY)
	{
		loadType = nsIDocShellLoadInfo::loadReloadBypassProxy;
	}
	else if (aReloadFlags & nsIWebNavigation::LOAD_FLAGS_BYPASS_CACHE)
	{
		loadType = nsIDocShellLoadInfo::loadReloadBypassCache;
	}
	else
	{
		loadType = nsIDocShellLoadInfo::loadReloadNormal;
	}
  
  // Notify listeners
  PRBool canNavigate = PR_TRUE;
  if (mListener) {
    nsCOMPtr<nsISHistoryListener> listener(do_QueryInterface(mListener));
    // We are reloading. Send Reload notifications. 
    // nsDocShellLoadFlagType is not public, where as nsIWebNavigation
    // is public. So send the reload notifications with the
    // nsIWebNavigation flags. 
    if (listener) {
      nsCOMPtr<nsIURI> currentURI;
      rv = GetCurrentURI(getter_AddRefs(currentURI));
      listener->OnHistoryReload(currentURI, aReloadFlags, &canNavigate);
    }
  }
  if (!canNavigate)
    return NS_OK;

	return LoadEntry(mIndex, loadType);
}

NS_IMETHODIMP
nsSHistory::Stop()
{
	//Not implemented
   return NS_OK;
}


NS_IMETHODIMP
nsSHistory::GetDocument(nsIDOMDocument** aDocument)
{

	// Not implemented
  return NS_OK;
}


NS_IMETHODIMP
nsSHistory::GetCurrentURI(nsIURI** aResultURI)
{
  NS_ENSURE_ARG_POINTER(aResultURI);
  nsresult rv;

  nsCOMPtr<nsIHistoryEntry> currentEntry;
  rv = GetEntryAtIndex(mIndex, PR_FALSE, getter_AddRefs(currentEntry));
  if (NS_FAILED(rv) && !currentEntry) return rv;
  rv = currentEntry->GetURI(aResultURI);
  return rv;
}


NS_IMETHODIMP
nsSHistory::SetSessionHistory(nsISHistory* aSessionHistory)
{
   // Not implemented
   return NS_OK;
}

	
NS_IMETHODIMP
nsSHistory::GetSessionHistory(nsISHistory** aSessionHistory)
{
  // Not implemented
   return NS_OK;
}


NS_IMETHODIMP
nsSHistory::LoadURI(const PRUnichar* aURI, PRUint32 aLoadFlags)
{
  return NS_OK;
}

NS_IMETHODIMP
nsSHistory::GotoIndex(PRInt32 aIndex)
{
	return LoadEntry(aIndex, nsIDocShellLoadInfo::loadHistory);
}

NS_IMETHODIMP
nsSHistory::LoadEntry(PRInt32 aIndex, long aLoadType)
{
  nsCOMPtr<nsIDocShell> docShell;
  nsCOMPtr<nsISHEntry> shEntry;
  PRInt32 oldIndex = mIndex;

  nsCOMPtr<nsISHEntry> prevEntry;
  GetEntryAtIndex(mIndex, PR_FALSE, getter_AddRefs(prevEntry));
  mIndex = aIndex;
   
  nsCOMPtr<nsISHEntry> nextEntry;   
  GetEntryAtIndex(mIndex, PR_FALSE, getter_AddRefs(nextEntry));
  nsCOMPtr<nsIHistoryEntry> nHEntry(do_QueryInterface(nextEntry));
  if (!nextEntry || !prevEntry || !nHEntry)
    return NS_ERROR_FAILURE;
  
  // Send appropriate listener notifications
  PRBool canNavigate = PR_TRUE;
  // Get the uri for the entry we are about to visit
  nsCOMPtr<nsIURI> nextURI;
  nHEntry->GetURI(getter_AddRefs(nextURI));
  if(mListener) {    
    nsCOMPtr<nsISHistoryListener> listener(do_QueryInterface(mListener));
    if (listener) {
      if (mIndex+1 == oldIndex) {
        // We are going back one entry. Send GoBack notifications
        listener->OnHistoryGoBack(nextURI, &canNavigate);
      }
      else if (mIndex-1 == oldIndex) {
        // We are going forward. Send GoForward notification
        listener->OnHistoryGoForward(nextURI, &canNavigate);
      }
      else if (mIndex != oldIndex) {
        // We are going somewhere else. This is not reload either
        listener->OnHistoryGotoIndex(mIndex, nextURI, &canNavigate);
      }
    }
  }

  if (!canNavigate) {
    // reset the index back to what it was, if the listener
    // asked us not to proceed with the operation.
    mIndex = oldIndex;
    return NS_OK;  // XXX Maybe I can return some other error code?
  }

  nsCOMPtr<nsIURI> nexturi;
  PRInt32 pCount=0, nCount=0;
  nsCOMPtr<nsISHContainer> prevAsContainer(do_QueryInterface(prevEntry));
  nsCOMPtr<nsISHContainer> nextAsContainer(do_QueryInterface(nextEntry));
  if (prevAsContainer && nextAsContainer) {
    prevAsContainer->GetChildCount(&pCount);
    nextAsContainer->GetChildCount(&nCount);
  }
  
  nsCOMPtr<nsIDocShellLoadInfo> loadInfo;
  if (oldIndex == aIndex) {
    // Possibly a reload case 
    docShell = mRootDocShell;
  }
  else {
    // Going back or forward.
    if ((pCount > 0) && (nCount > 0)) {
      /* THis is a subframe navigation. Go find 
       * the docshell in which load should happen
       */
      PRBool result = CompareSHEntry(prevEntry, nextEntry, mRootDocShell, 
                                      getter_AddRefs(docShell),
                                      getter_AddRefs(shEntry));
      if (!result) {
        /* There was an error in finding the entry and docshell
         * where the load should happen. Reset the index back 
         * to what it was. Return failure.
         */
        mIndex = oldIndex;   
        return NS_ERROR_FAILURE;
      }
      else {        
        nextEntry = shEntry;
      }
    }   // (pCount >0)
    else
      docShell = mRootDocShell;
    }
  

    if (!docShell || !nextEntry || !mRootDocShell)
      return NS_ERROR_FAILURE;

    /* Set the loadType in the SHEntry too to  what was passed on.
     * This will be passed on to child subframes later in nsDocShell,
     * so that proper loadType is maintained through out a frameset
     */
    nextEntry->SetLoadType(aLoadType);

    mRootDocShell->CreateLoadInfo (getter_AddRefs(loadInfo));

    loadInfo->SetLoadType(aLoadType);
    loadInfo->SetSHEntry(nextEntry);
    // Time to initiate a document load
    nsresult rv = docShell->LoadURI(nextURI, loadInfo, nsIWebNavigation::LOAD_FLAGS_NONE);
    /* If the loadURI call failed for some reason,
     * reset mIndex to what it was. so that back/forward
     * won't misbehave
     */
    if (!NS_SUCCEEDED(rv))
      mIndex = oldIndex;
    return rv;
}


PRBool
nsSHistory::CompareSHEntry(nsISHEntry * aPrevEntry, nsISHEntry * aNextEntry, nsIDocShell * aParent,
					nsIDocShell ** aDSResult, nsISHEntry ** aSHEResult)
{
	NS_ENSURE_ARG_POINTER(aDSResult);
	NS_ENSURE_ARG_POINTER(aSHEResult);

	if (!aPrevEntry || !aNextEntry || !aParent)
	   return PR_FALSE;

	PRBool result = PR_FALSE;
	PRUint32 prevID, nextID;

	aPrevEntry->GetID(&prevID);
	aNextEntry->GetID(&nextID);
 
	// Check the IDs to verify if the pages are different.
    if (prevID != nextID) {
       *aDSResult = aParent;
	   *aSHEResult = aNextEntry;
	   NS_IF_ADDREF(*aSHEResult);
	   NS_IF_ADDREF(*aDSResult);
	   return PR_TRUE;
    }
    result = PR_FALSE;

    /* The root entries are the same, so compare any child frames */
    PRInt32  cnt=0, pcnt=0, ncnt=0, dsCount=0;
	nsCOMPtr<nsISHContainer>  prevContainer(do_QueryInterface(aPrevEntry));
	nsCOMPtr<nsISHContainer>  nextContainer(do_QueryInterface(aNextEntry));
	nsCOMPtr<nsIDocShellTreeNode> dsTreeNode(do_QueryInterface(aParent));

	if (!dsTreeNode)
		return PR_FALSE;
	if (!prevContainer || !nextContainer)
		return PR_FALSE;

    prevContainer->GetChildCount(&pcnt);
    nextContainer->GetChildCount(&ncnt);
    dsTreeNode->GetChildCount(&dsCount);

    //XXX What to do if the children count don't match
    
    for (PRInt32 i=0; i<ncnt; i++){
	  nsCOMPtr<nsISHEntry> pChild, nChild;
      nsCOMPtr<nsIDocShellTreeItem> dsTreeItemChild;
	  
      prevContainer->GetChildAt(i, getter_AddRefs(pChild));
	  nextContainer->GetChildAt(i, getter_AddRefs(nChild));
	  dsTreeNode->GetChildAt(i, getter_AddRefs(dsTreeItemChild));

	  if (!dsTreeItemChild)
		  return PR_FALSE;

	  nsCOMPtr<nsIDocShell> dsChild(do_QueryInterface(dsTreeItemChild));

	  result = CompareSHEntry(pChild, nChild, dsChild, aDSResult, aSHEResult);
	  if (result)  // We have found the docshell in which loadUri is to be called.
		  break;
	}     
    return result;
}




NS_IMETHODIMP
nsSHistory::SetRootDocShell(nsIDocShell * aDocShell)
{
  mRootDocShell = aDocShell;
  return NS_OK;
}

NS_IMETHODIMP
nsSHistory::GetRootDocShell(nsIDocShell ** aDocShell)
{
   NS_ENSURE_ARG_POINTER(aDocShell);

   *aDocShell = mRootDocShell;
   //Not refcounted. May this method should not be available for public
  // NS_IF_ADDREF(*aDocShell);
   return NS_OK;
}


NS_IMETHODIMP
nsSHistory::GetSHistoryEnumerator(nsISimpleEnumerator** aEnumerator)
{
  nsresult status = NS_OK;

  NS_ENSURE_ARG_POINTER(aEnumerator);
  nsSHEnumerator * iterator = new nsSHEnumerator(this);
  if (iterator && !NS_SUCCEEDED(status = CallQueryInterface(iterator, aEnumerator)))
    delete iterator;
  return status;
}


//*****************************************************************************
//***    nsSHEnumerator: Object Management
//*****************************************************************************

nsSHEnumerator::nsSHEnumerator(nsSHistory * aSHistory):mIndex(-1)
{
  NS_INIT_REFCNT();
  mSHistory = aSHistory;
}

nsSHEnumerator::~nsSHEnumerator()
{
  mSHistory = nsnull;
}

NS_IMPL_ISUPPORTS1(nsSHEnumerator, nsISimpleEnumerator)

NS_IMETHODIMP
nsSHEnumerator::HasMoreElements(PRBool * aReturn)
{
  PRInt32 cnt;
  *aReturn = PR_FALSE;
  mSHistory->GetCount(&cnt);
  if (mIndex >= 0 && mIndex < cnt ) { 
    *aReturn = PR_TRUE;
  }
  return NS_OK;
}


NS_IMETHODIMP 
nsSHEnumerator::GetNext(nsISupports **aItem)
{
  NS_ENSURE_ARG_POINTER(aItem);
  PRInt32 cnt= 0;

  nsresult  result = NS_ERROR_FAILURE;
  mSHistory->GetCount(&cnt);
  if (mIndex < (cnt-1)) {
    mIndex++;
    nsCOMPtr<nsIHistoryEntry> hEntry;
    result = mSHistory->GetEntryAtIndex(mIndex, PR_FALSE, getter_AddRefs(hEntry));
  	if (hEntry)
      result = CallQueryInterface(hEntry, aItem);
  }
  return result;
}
