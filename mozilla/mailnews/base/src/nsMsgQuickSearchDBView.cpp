/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2001
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Navin Gupta <naving@netscape.com> (Original Author)
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
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

#include "msgCore.h"
#include "nsMsgQuickSearchDBView.h"
#include "nsMsgFolderFlags.h"
#include "nsIMsgHdr.h"
#include "nsMsgBaseCID.h"
#include "nsIMsgImapMailFolder.h"
#include "nsImapCore.h"
#include "nsIMsgHdr.h"
#include "nsIDBFolderInfo.h"

nsMsgQuickSearchDBView::nsMsgQuickSearchDBView()
{
  m_usingCachedHits = PR_FALSE;
  m_cacheEmpty = PR_TRUE;
}

nsMsgQuickSearchDBView::~nsMsgQuickSearchDBView()
{	
 /* destructor code */
}

NS_IMPL_ISUPPORTS_INHERITED2(nsMsgQuickSearchDBView, nsMsgDBView, nsIMsgDBView, nsIMsgSearchNotify)

NS_IMETHODIMP nsMsgQuickSearchDBView::Open(nsIMsgFolder *folder, nsMsgViewSortTypeValue sortType, nsMsgViewSortOrderValue sortOrder, nsMsgViewFlagsTypeValue viewFlags, PRInt32 *pCount)
{
  nsresult rv = nsMsgDBView::Open(folder, sortType, sortOrder, viewFlags, pCount);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!m_db)
    return NS_ERROR_NULL_POINTER;
  if (pCount)
    *pCount = 0;
  m_viewFolder = nsnull;
  return InitThreadedView(pCount);
}

nsresult nsMsgQuickSearchDBView::DeleteMessages(nsIMsgWindow *window, nsMsgViewIndex *indices, PRInt32 numIndices, PRBool deleteStorage)
{
  for (nsMsgViewIndex i = 0; i < (nsMsgViewIndex) numIndices; i++) 
  {
    nsCOMPtr<nsIMsgDBHdr> msgHdr; 
    (void) GetMsgHdrForViewIndex(indices[i],getter_AddRefs(msgHdr));
    if (msgHdr)
      RememberDeletedMsgHdr(msgHdr);
  }

  return nsMsgDBView::DeleteMessages(window, indices, numIndices, deleteStorage);
}

NS_IMETHODIMP nsMsgQuickSearchDBView::DoCommand(nsMsgViewCommandTypeValue aCommand)
{
  if (aCommand == nsMsgViewCommandType::markAllRead)
  {
    nsresult rv = NS_OK;
    m_folder->EnableNotifications(nsIMsgFolder::allMessageCountNotifications, PR_FALSE, PR_TRUE /*dbBatching*/);

    for (PRInt32 i=0;NS_SUCCEEDED(rv) && i < GetSize();i++)
    {
      nsCOMPtr<nsIMsgDBHdr> msgHdr;
      m_db->GetMsgHdrForKey(m_keys[i],getter_AddRefs(msgHdr)); 
      rv = m_db->MarkHdrRead(msgHdr, PR_TRUE, nsnull);
    }

    m_folder->EnableNotifications(nsIMsgFolder::allMessageCountNotifications, PR_TRUE, PR_TRUE /*dbBatching*/);

    nsCOMPtr<nsIMsgImapMailFolder> imapFolder = do_QueryInterface(m_folder);
    if (NS_SUCCEEDED(rv) && imapFolder)
      rv = imapFolder->StoreImapFlags(kImapMsgSeenFlag, PR_TRUE, m_keys.Elements(), 
                                      m_keys.Length(), nsnull);

    m_db->SetSummaryValid(PR_TRUE);
    return rv;
  }
  else
    return nsMsgDBView::DoCommand(aCommand);
}

NS_IMETHODIMP nsMsgQuickSearchDBView::GetViewType(nsMsgViewTypeValue *aViewType)
{
    NS_ENSURE_ARG_POINTER(aViewType);
    *aViewType = nsMsgViewType::eShowQuickSearchResults; 
    return NS_OK;
}

nsresult nsMsgQuickSearchDBView::OnNewHeader(nsIMsgDBHdr *newHdr, nsMsgKey aParentKey, PRBool ensureListed)
{
  if (newHdr)
  {
    PRBool match=PR_FALSE;
    nsCOMPtr <nsIMsgSearchSession> searchSession = do_QueryReferent(m_searchSession);
    if (searchSession)
      searchSession->MatchHdr(newHdr, m_db, &match);
    if (match)
    {
      // put the new header in m_origKeys, so that expanding a thread will
      // show the newly added header.
      nsMsgKey newKey;
      (void) newHdr->GetMessageKey(&newKey);
      nsMsgViewIndex insertIndex = GetInsertIndexHelper(newHdr, m_origKeys, 
                      nsMsgViewSortOrder::ascending, nsMsgViewSortType::byId);
      m_origKeys.InsertElementAt(insertIndex, newKey);
      nsMsgThreadedDBView::OnNewHeader(newHdr, aParentKey, ensureListed); // do not add a new message if there isn't a match.
    }
  }
  return NS_OK;
}

NS_IMETHODIMP nsMsgQuickSearchDBView::OnHdrFlagsChanged(nsIMsgDBHdr *aHdrChanged, PRUint32 aOldFlags, 
                                       PRUint32 aNewFlags, nsIDBChangeListener *aInstigator)
{
  nsresult rv = nsMsgDBView::OnHdrFlagsChanged(aHdrChanged, aOldFlags, aNewFlags, aInstigator);

  if (m_viewFolder && (aOldFlags & MSG_FLAG_READ) != (aNewFlags & MSG_FLAG_READ))
  {
    // if we're displaying a single folder virtual folder for an imap folder,
    // the search criteria might be on message body, and we might not have the
    // message body offline, in which case we can't tell if the message 
    // matched or not. But if the unread flag changed, we need to update the
    // unread counts. Normally, VirtualFolderChangeListener::OnHdrFlagsChanged will
    // handle this, but it won't work for body criteria when we don't have the
    // body offline.
    nsCOMPtr<nsIMsgImapMailFolder> imapFolder = do_QueryInterface(m_viewFolder);
    if (imapFolder)
    {
      nsMsgViewIndex hdrIndex = FindHdr(aHdrChanged);
      if (hdrIndex != nsMsgViewIndex_None)
      {
        nsCOMPtr <nsIMsgSearchSession> searchSession = do_QueryReferent(m_searchSession);
        if (searchSession)
        {
          PRBool oldMatch, newMatch;
          rv = searchSession->MatchHdr(aHdrChanged, m_db, &newMatch);
          aHdrChanged->SetFlags(aOldFlags);
          rv = searchSession->MatchHdr(aHdrChanged, m_db, &oldMatch);
          aHdrChanged->SetFlags(aNewFlags); 
          // if it doesn't match the criteria, VirtualFolderChangeListener::OnHdrFlagsChanged
          // won't tweak the read/unread counts. So do it here:
          if (!oldMatch && !newMatch)
          {
            nsCOMPtr <nsIMsgDatabase> virtDatabase;
            nsCOMPtr <nsIDBFolderInfo> dbFolderInfo;

            rv = m_viewFolder->GetDBFolderInfoAndDB(getter_AddRefs(dbFolderInfo), getter_AddRefs(virtDatabase));
            NS_ENSURE_SUCCESS(rv, rv);
            dbFolderInfo->ChangeNumUnreadMessages((aOldFlags & MSG_FLAG_READ) ? 1 : -1);
            m_viewFolder->UpdateSummaryTotals(PR_TRUE); // force update from db.
            virtDatabase->Commit(nsMsgDBCommitType::kLargeCommit);
          }
        }
      }
    }
  }
  return rv;
}

NS_IMETHODIMP
nsMsgQuickSearchDBView::OnHdrPropertyChanged(nsIMsgDBHdr *aHdrChanged, PRBool aPreChange,
                                        PRUint32 *aStatus, nsIDBChangeListener *aInstigator)
{
  // If the junk mail plugin just activated on a new message, then
  // we'll allow filters to remove from view.
  // Otherwise, just update the view line.

  if (aPreChange)
    return NS_OK;

  if (aHdrChanged)
  {
    nsCOMPtr<nsIMsgSearchSession> searchSession(do_QueryReferent(m_searchSession));
    if (searchSession)
    {
      nsMsgViewIndex index = FindHdr(aHdrChanged);
      if (index != nsMsgViewIndex_None)
      {
        PRBool match = PR_FALSE;
        searchSession->MatchHdr(aHdrChanged, m_db, &match);
        nsCString originStr;
        PRUint32 flags;
        aHdrChanged->GetFlags(&flags);
        (void) aHdrChanged->GetStringProperty("junkscoreorigin", getter_Copies(originStr));
        // check for "plugin" with only first character for performance
        if (!match && originStr.get()[0] == 'p' && flags & MSG_FLAG_NEW)
          RemoveByIndex(index); // remove hdr from view
        else
          NoteChange(index, 1, nsMsgViewNotificationCode::changed);
      }
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsMsgQuickSearchDBView::GetSearchSession(nsIMsgSearchSession* *aSession)
{
  NS_ASSERTION(PR_FALSE, "GetSearchSession method is not implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsMsgQuickSearchDBView::SetSearchSession(nsIMsgSearchSession *aSession)
{
  m_searchSession = do_GetWeakReference(aSession);
  return NS_OK;
}

NS_IMETHODIMP
nsMsgQuickSearchDBView::OnSearchHit(nsIMsgDBHdr* aMsgHdr, nsIMsgFolder *folder)
{
  NS_ENSURE_ARG(aMsgHdr);
  if (!m_db)
    return NS_ERROR_NULL_POINTER;
  // remember search hit and when search is done, reconcile cache
  // with new hits;
  m_hdrHits.AppendObject(aMsgHdr);
  nsMsgKey key;
  aMsgHdr->GetMessageKey(&key);
  // is FindKey going to be expensive here? A lot of hits could make
  // it a little bit slow to search through the view for every hit.
  if (m_cacheEmpty || FindKey(key, PR_FALSE) == nsMsgViewIndex_None)
  return AddHdr(aMsgHdr); 
  else
    return NS_OK;
}

NS_IMETHODIMP
nsMsgQuickSearchDBView::OnSearchDone(nsresult status)
{
  if (m_viewFolder)
  {
    nsTArray<nsMsgKey> keyArray;
    nsCString searchUri;
    m_viewFolder->GetURI(searchUri);
    PRUint32 count = m_hdrHits.Count();
    // build up message keys.
    PRUint32 i;
    for (i = 0; i < count; i++)
    {
      nsMsgKey key;
      m_hdrHits[i]->GetMessageKey(&key);
      keyArray.AppendElement(key);
    }
    nsMsgKey *staleHits;
    PRUint32 numBadHits;
    if (m_db)
    {
      nsresult rv = m_db->RefreshCache(searchUri.get(), m_hdrHits.Count(),
                                       keyArray.Elements(), &numBadHits, &staleHits);
      NS_ENSURE_SUCCESS(rv, rv);
      for (i = 0; i < numBadHits; i++)
      {
        nsMsgViewIndex staleHitIndex = FindKey(staleHits[i], PR_TRUE);
        if (staleHitIndex != nsMsgViewIndex_None)
          RemoveByIndex(staleHitIndex);
      }
      delete [] staleHits;
    }
  }
  if (m_sortType != nsMsgViewSortType::byThread)//we do not find levels for the results.
  {
    m_sortValid = PR_FALSE;       //sort the results 
    Sort(m_sortType, m_sortOrder);
  }
  if (m_viewFolder)
    SetMRUTimeForFolder(m_viewFolder);

  m_hdrHits.Clear();
  return NS_OK;
}


NS_IMETHODIMP
nsMsgQuickSearchDBView::OnNewSearch()
{
  PRInt32 oldSize = GetSize();

  m_keys.Clear();
  m_levels.Clear();
  m_flags.Clear();
  m_hdrHits.Clear();
  // this needs to happen after we remove all the keys, since RowCountChanged() will call our GetRowCount()
  if (mTree)
    mTree->RowCountChanged(0, -oldSize);
  PRUint32 folderFlags = 0;
  if (m_viewFolder)
    m_viewFolder->GetFlags(&folderFlags);
  // check if it's a virtual folder - if so, we should get the cached hits 
  // from the db, and set a flag saying that we're using cached values.
  if (folderFlags & nsMsgFolderFlags::Virtual)
  {
    nsCOMPtr<nsISimpleEnumerator> cachedHits;
    nsCString searchUri;
    m_viewFolder->GetURI(searchUri);
    m_db->GetCachedHits(searchUri.get(), getter_AddRefs(cachedHits));
    if (cachedHits)
    {
      PRBool hasMore;

      m_usingCachedHits = PR_TRUE;
      cachedHits->HasMoreElements(&hasMore);
      m_cacheEmpty = !hasMore;
      if (mTree)
        mTree->BeginUpdateBatch();
      while (hasMore)
      {
        nsCOMPtr <nsIMsgDBHdr> pHeader;
        nsresult rv = cachedHits->GetNext(getter_AddRefs(pHeader));
        NS_ASSERTION(NS_SUCCEEDED(rv), "nsMsgDBEnumerator broken");
        if (pHeader && NS_SUCCEEDED(rv))
          AddHdr(pHeader);
        else
          break;
        cachedHits->HasMoreElements(&hasMore);
      }
      if (mTree)
        mTree->EndUpdateBatch();
    }
  }
  return NS_OK;
}

nsresult nsMsgQuickSearchDBView::GetFirstMessageHdrToDisplayInThread(nsIMsgThread *threadHdr, nsIMsgDBHdr **result)
{
  PRUint32 numChildren;
  nsresult rv = NS_OK;
  PRUint8 minLevel = 0xff;
  nsMsgKey threadRootKey;

  threadHdr->GetNumChildren(&numChildren);
  threadHdr->GetThreadKey(&threadRootKey);
  if ((PRInt32) numChildren < 0)
    numChildren = 0;

  nsCOMPtr <nsIMsgDBHdr> retHdr;

  // iterate over thread, finding mgsHdr in view with the lowest level.
  for (PRUint32 childIndex = 0; childIndex < numChildren; childIndex++)
  {
    nsCOMPtr <nsIMsgDBHdr> child;
    rv = threadHdr->GetChildHdrAt(childIndex, getter_AddRefs(child));
    if (NS_SUCCEEDED(rv) && child)
    {
      nsMsgKey msgKey;
      child->GetMessageKey(&msgKey);

      // this works because we've already sorted m_keys by id.
      nsMsgViewIndex keyIndex = m_origKeys.BinaryIndexOf(msgKey);
      if (keyIndex != kNotFound)
      {
        // this is the root, so it's the best we're going to do.
        if (msgKey == threadRootKey)
        {
          retHdr = child;
          break;
        }
        PRUint8 level = 0;
        nsMsgKey parentId;
        child->GetThreadParent(&parentId);
        nsCOMPtr <nsIMsgDBHdr> parent;
        // count number of ancestors - that's our level
        while (parentId != nsMsgKey_None)
        {
          rv = m_db->GetMsgHdrForKey(parentId, getter_AddRefs(parent));
          if (parent)
          {
            nsMsgKey saveParentId = parentId;
            parent->GetThreadParent(&parentId);
            // message is it's own parent - bad, let's break out of here.
            if (parentId == saveParentId)
              break;
            level++;
          }
          else // if we can't find the parent, don't loop forever.
            break;
        }
        if (level < minLevel)
        {
          minLevel = level;
          retHdr = child;
        }
      }
    }
  }
  NS_IF_ADDREF(*result = retHdr);
  return NS_OK; 
}

nsresult nsMsgQuickSearchDBView::SortThreads(nsMsgViewSortTypeValue sortType, nsMsgViewSortOrderValue sortOrder)
{
  // we don't handle grouping in quick search views yet.
  if (m_viewFlags & nsMsgViewFlagsType::kGroupBySort)
    return NS_OK;

  // iterate over the messages in the view, getting the thread id's
  // sort m_keys so we can quickly find if a key is in the view. 
  m_keys.Sort();
  // array of the threads' root hdr keys.
  nsTArray<nsMsgKey> threadRootIds;
  nsCOMPtr <nsIMsgDBHdr> rootHdr;
  nsCOMPtr <nsIMsgDBHdr> msgHdr;
  nsCOMPtr <nsIMsgThread> threadHdr;
  for (PRUint32 i = 0; i < m_keys.Length(); i++)
  {
    GetMsgHdrForViewIndex(i, getter_AddRefs(msgHdr));
    m_db->GetThreadContainingMsgHdr(msgHdr, getter_AddRefs(threadHdr));
    if (threadHdr)
    {
      nsMsgKey rootKey;
      threadHdr->GetChildKeyAt(0, &rootKey);
      nsMsgViewIndex threadRootIndex = threadRootIds.BinaryIndexOf(rootKey);
      // if we already have that id in top level threads, ignore this msg.
      if (threadRootIndex != kNotFound)
        continue;
      // it would be nice if GetInsertIndexHelper always found the hdr, but it doesn't.
      threadHdr->GetChildHdrAt(0, getter_AddRefs(rootHdr));
      threadRootIndex = GetInsertIndexHelper(rootHdr, threadRootIds, nsMsgViewSortOrder::ascending, nsMsgViewSortType::byId);
      threadRootIds.InsertElementAt(threadRootIndex, rootKey);
    }
  }
  m_origKeys.SwapElements(m_keys);
  // need to sort the top level threads now by sort order, if it's not by id.
  if (sortType != nsMsgViewSortType::byId)
  {
    m_keys.SwapElements(threadRootIds);
    nsMsgDBView::Sort(sortType, sortOrder);
    threadRootIds.SwapElements(m_keys);
  }
  m_keys.Clear();
  m_levels.Clear();
  m_flags.Clear();
  // now we've build up the list of thread ids - need to build the view
  // from that. So for each thread id, we need to list the messages in the thread.
  PRUint32 numThreads = threadRootIds.Length();
  for (PRUint32 threadIndex = 0; threadIndex < numThreads; threadIndex++)
  {
    m_db->GetMsgHdrForKey(threadRootIds[threadIndex], getter_AddRefs(rootHdr));
    if (rootHdr)
    {
      nsCOMPtr <nsIMsgDBHdr> displayRootHdr;
      m_db->GetThreadContainingMsgHdr(rootHdr, getter_AddRefs(threadHdr));
      if (threadHdr)
      {
        nsMsgKey rootKey;
        PRUint32 rootFlags;
        GetFirstMessageHdrToDisplayInThread(threadHdr, getter_AddRefs(displayRootHdr));
        if (!displayRootHdr)
          continue;
        displayRootHdr->GetMessageKey(&rootKey);
        displayRootHdr->GetFlags(&rootFlags);
        rootFlags |= MSG_VIEW_FLAG_ISTHREAD;
        m_keys.AppendElement(rootKey);
        m_flags.AppendElement(rootFlags);
        m_levels.AppendElement(0);

        nsMsgViewIndex startOfThreadViewIndex = m_keys.Length() - 1;
        PRUint32 numListed;
        ListIdsInThread(threadHdr, startOfThreadViewIndex, &numListed);
      }
    }
  }
  NS_ASSERTION(m_origKeys.Length() == m_keys.Length(), "problem threading quick search");
  return NS_OK;
}

nsresult  nsMsgQuickSearchDBView::ListIdsInThread(nsIMsgThread *threadHdr, nsMsgViewIndex startOfThreadViewIndex, PRUint32 *pNumListed)
{
  PRUint32 numChildren;
  threadHdr->GetNumChildren(&numChildren);
  PRUint32 i;
  PRUint32 viewIndex = startOfThreadViewIndex + 1;
  nsCOMPtr <nsIMsgDBHdr> rootHdr;
  nsMsgKey rootKey;
  PRUint32 rootFlags = m_flags[startOfThreadViewIndex];
  *pNumListed = 0;
  GetMsgHdrForViewIndex(startOfThreadViewIndex, getter_AddRefs(rootHdr));
  rootHdr->GetMessageKey(&rootKey);
  for (i = 0; i < numChildren; i++)
  {
    nsCOMPtr <nsIMsgDBHdr> msgHdr;
    threadHdr->GetChildHdrAt(i, getter_AddRefs(msgHdr));
    if (msgHdr != nsnull)
    {
      nsMsgKey msgKey;
      msgHdr->GetMessageKey(&msgKey);
      if (msgKey != rootKey)
      {
        nsMsgViewIndex threadRootIndex = m_origKeys.BinaryIndexOf(msgKey);
        // if this hdr is in the original view, add it to new view.
        if (threadRootIndex != kNotFound)
        {
          PRUint32 childFlags;
          msgHdr->GetFlags(&childFlags);
          PRUint8 levelToAdd;
          m_keys.InsertElementAt(viewIndex, msgKey);
          m_flags.InsertElementAt(viewIndex, childFlags);
          if (! (rootFlags & MSG_VIEW_FLAG_HASCHILDREN))
          {
            rootFlags |= MSG_VIEW_FLAG_HASCHILDREN;
            m_flags[startOfThreadViewIndex] = rootFlags;
          }
          levelToAdd = FindLevelInThread(msgHdr, startOfThreadViewIndex, viewIndex);
          m_levels.InsertElementAt(viewIndex, levelToAdd);
          viewIndex++;
          (*pNumListed)++;
        }
      }
    }
  }
  return NS_OK;
}

nsresult nsMsgQuickSearchDBView::ExpansionDelta(nsMsgViewIndex index, PRInt32 *expansionDelta)
{
  *expansionDelta = 0;
  if (index >= ((nsMsgViewIndex) m_keys.Length()))
    return NS_MSG_MESSAGE_NOT_FOUND;

  char	flags = m_flags[index];

  if (!(m_viewFlags & nsMsgViewFlagsType::kThreadedDisplay))
    return NS_OK;

  // The client can pass in the key of any message
  // in a thread and get the expansion delta for the thread.

  PRInt32 numChildren = CountExpandedThread(index);

  *expansionDelta = (flags & MSG_FLAG_ELIDED) ? 
                    numChildren - 1 : - (PRInt32) (numChildren - 1);
  return NS_OK;
}
