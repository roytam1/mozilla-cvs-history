/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * Copyright (C) 2001 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s):
 * Jan Varga (varga@utcru.sk)
 */

#include "msgCore.h"
#include "nsMsgDBView.h"
#include "nsISupports.h"
#include "nsIMsgFolder.h"
#include "nsIDBFolderInfo.h"
#include "nsIDBFolderInfo.h"
#include "nsIMsgDatabase.h"
#include "nsIMsgFolder.h"
#include "MailNewsTypes2.h"
#include "nsMsgUtils.h"
#include "nsXPIDLString.h"
#include "nsQuickSort.h"
#include "nsIMsgImapMailFolder.h"
#include "nsImapCore.h"

#include "nsIDOMElement.h"
#include "nsDateTimeFormatCID.h"
#include "nsMsgMimeCID.h"

/* Implementation file */

static NS_DEFINE_CID(kDateTimeFormatCID,    NS_DATETIMEFORMAT_CID);

nsrefcnt nsMsgDBView::gInstanceCount	= 0;
nsIAtom * nsMsgDBView::kUnreadMsgAtom	= nsnull;
nsIAtom * nsMsgDBView::kHighestPriorityAtom	= nsnull;
nsIAtom * nsMsgDBView::kHighPriorityAtom	= nsnull;
nsIAtom * nsMsgDBView::kLowestPriorityAtom	= nsnull;
nsIAtom * nsMsgDBView::kLowPriorityAtom	= nsnull;

NS_IMPL_ADDREF(nsMsgDBView)
NS_IMPL_RELEASE(nsMsgDBView)

NS_INTERFACE_MAP_BEGIN(nsMsgDBView)
   NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIMsgDBView)
   NS_INTERFACE_MAP_ENTRY(nsIMsgDBView)
   NS_INTERFACE_MAP_ENTRY(nsIDBChangeListener)
   NS_INTERFACE_MAP_ENTRY(nsIOutlinerView)
NS_INTERFACE_MAP_END

nsMsgDBView::nsMsgDBView()
{
  NS_INIT_ISUPPORTS();
  /* member initializers and constructor code */
  m_sortValid = PR_FALSE;
  m_sortOrder = nsMsgViewSortOrder::none;
  m_viewFlags = nsMsgViewFlagsType::kNone;
  m_cachedMsgKey = nsMsgKey_None;
  m_currentlyDisplayedMsgKey = nsMsgKey_None;
  mNumSelectedRows = 0;
  mSupressMsgDisplay = PR_FALSE;

  // initialize any static atoms or unicode strings
  if (gInstanceCount == 0) 
  {
    kUnreadMsgAtom = NS_NewAtom("unread");

    kHighestPriorityAtom = NS_NewAtom("priority-highest");
    kHighPriorityAtom = NS_NewAtom("priority-high");
    kLowestPriorityAtom = NS_NewAtom("priority-lowest");
    kLowPriorityAtom = NS_NewAtom("priority-low");
  }
  
  gInstanceCount++;
}

nsMsgDBView::~nsMsgDBView()
{
  if (m_db)
	  m_db->RemoveListener(this);

  gInstanceCount--;
  if (gInstanceCount <= 0) 
  {
    NS_IF_RELEASE(kUnreadMsgAtom);
    NS_IF_RELEASE(kHighestPriorityAtom);
    NS_IF_RELEASE(kHighPriorityAtom);
    NS_IF_RELEASE(kLowestPriorityAtom);
    NS_IF_RELEASE(kLowPriorityAtom);
  }
}

///////////////////////////////////////////////////////////////////////////
// nsIOutlinerView Implementation Methods (and helper methods)
///////////////////////////////////////////////////////////////////////////

nsresult nsMsgDBView::FetchAuthor(nsIMsgHdr * aHdr, PRUnichar ** aSenderString)
{
  nsXPIDLString unparsedAuthor;
  if (!mHeaderParser)
    mHeaderParser = do_CreateInstance(NS_MAILNEWS_MIME_HEADER_PARSER_CONTRACTID);

  nsresult rv = aHdr->GetMime2DecodedAuthor(getter_Copies(unparsedAuthor));
  // *sigh* how sad, we need to convert our beautiful unicode string to utf8 
  // so we can extract the name part of the address...then convert it back to 
  // unicode again.
  if (mHeaderParser)
  {
    nsXPIDLCString name;
    rv = mHeaderParser->ExtractHeaderAddressName("UTF-8", NS_ConvertUCS2toUTF8(unparsedAuthor), getter_Copies(name));
    if (NS_SUCCEEDED(rv) && (const char*)name)
    {
      *aSenderString = nsCRT::strdup(NS_ConvertUTF8toUCS2(name));
      return NS_OK;
    }
  }

  // if we got here then just return the original string
  *aSenderString = nsCRT::strdup(unparsedAuthor);
  return NS_OK;
}

nsresult nsMsgDBView::FetchSubject(nsIMsgHdr * aMsgHdr, PRUint32 aFlags, PRUnichar ** aValue)
{
  if (aFlags & MSG_FLAG_HAS_RE)
  {
    nsXPIDLString subject;
    aMsgHdr->GetMime2DecodedSubject(getter_Copies(subject));
    nsAutoString reSubject;
    reSubject.Assign(NS_LITERAL_STRING("Re: "));
    reSubject.Append(subject);
    *aValue = reSubject.ToNewUnicode();
  }
  else
    aMsgHdr->GetMime2DecodedSubject(aValue);

  return NS_OK;
}

// in case we want to play around with the date string, I've broken it out into
// a separate routine. 
nsresult nsMsgDBView::FetchDate(nsIMsgHdr * aHdr, PRUnichar ** aDateString)
{
  PRTime dateOfMsg;
  nsAutoString formattedDateString;

  if (!mDateFormater)
    mDateFormater = do_CreateInstance(kDateTimeFormatCID);

  NS_ENSURE_TRUE(mDateFormater, NS_ERROR_FAILURE);

  nsresult rv = aHdr->GetDate(&dateOfMsg);
  // for now, the outline widget doesn't crop yet so don't add
  // the time to the string, it makes the date column take up 
  // too much space. I'm taking the time out just until we can 
  // crop.
  if (NS_SUCCEEDED(rv))
    rv = mDateFormater->FormatPRTime(nsnull /* nsILocale* locale */,
                                      kDateFormatShort,
                                      kTimeFormatSeconds,
                                      PRTime(dateOfMsg),
                                      formattedDateString);

  if (NS_SUCCEEDED(rv))
    *aDateString = formattedDateString.ToNewUnicode();
  
  return rv;
}

nsresult nsMsgDBView::FetchStatus(PRUint32 aFlags, PRUnichar ** aStatusString)
{
  // mscott -> i'll clean this up and used a cached string bundle value
  // later...

  if(aFlags & MSG_FLAG_REPLIED)
    *aStatusString = nsCRT::strdup(NS_LITERAL_STRING("Replied"));
	else if(aFlags & MSG_FLAG_FORWARDED)
		*aStatusString = nsCRT::strdup(NS_LITERAL_STRING("Forwarded"));
	else if(aFlags & MSG_FLAG_NEW)
		*aStatusString = nsCRT::strdup(NS_LITERAL_STRING("New"));
	else if(aFlags & MSG_FLAG_READ)
		*aStatusString = nsCRT::strdup(NS_LITERAL_STRING("Read"));

  return NS_OK;
}

nsresult nsMsgDBView::FetchSize(nsIMsgHdr * aHdr, PRUnichar ** aSizeString)
{
  nsAutoString formattedSizeString;
  PRUint32 msgSize = 0;
  aHdr->GetMessageSize(&msgSize);

	if(msgSize < 1024)
		msgSize = 1024;
	
  PRUint32 sizeInKB = msgSize/1024;
  
  formattedSizeString.AppendInt(sizeInKB);
  formattedSizeString.Append(NS_LITERAL_STRING("KB"));

  *aSizeString = formattedSizeString.ToNewUnicode();

  return NS_OK;
}

nsresult nsMsgDBView::FetchPriority(nsIMsgHdr *aHdr, PRUnichar ** aPriorityString)
{
  // mscott --> fix me and turn me into string bundle calls
  nsMsgPriorityValue priority = nsMsgPriority::notSet;
  nsAutoString priorityString;
  aHdr->GetPriority(&priority);
  switch (priority)
  {
  case nsMsgPriority::highest:
    priorityString = NS_LITERAL_STRING("Highest");
    break;
  case nsMsgPriority::high:
    priorityString = NS_LITERAL_STRING("High");
    break;
  case nsMsgPriority::low:
    priorityString = NS_LITERAL_STRING("Low");
    break;
  case nsMsgPriority::lowest:
    priorityString = NS_LITERAL_STRING("Lowest");
    break;
  case nsMsgPriority::normal:
    priorityString = NS_LITERAL_STRING("Normal");
    break;
  default:
    break;
  }

  if (!priorityString.IsEmpty())
    *aPriorityString = priorityString.ToNewUnicode();

  return NS_OK;
}

// call this AFTER calling ::Sort.
nsresult nsMsgDBView::UpdateSortUI(nsIDOMElement * aNewSortColumn)
{
  if (mCurrentSortColumn && aNewSortColumn != mCurrentSortColumn)
    mCurrentSortColumn->RemoveAttribute(NS_LITERAL_STRING("sortDirection"));

  // set the new sort direction on the new sort column
  mCurrentSortColumn = aNewSortColumn;

  if (m_sortOrder == nsMsgViewSortOrder::ascending)
    mCurrentSortColumn->SetAttribute(NS_LITERAL_STRING("sortDirection"), NS_LITERAL_STRING("ascending"));
  else
    mCurrentSortColumn->SetAttribute(NS_LITERAL_STRING("sortDirection"), NS_LITERAL_STRING("descending"));
  return NS_OK;
}

nsresult nsMsgDBView::SaveSelection(nsMsgKeyArray * aMsgKeyArray)
{
  if (!mOutlinerSelection)
    return NS_OK;

  // first, freeze selection.
  mOutlinerSelection->SetSelectEventsSuppressed(PR_TRUE);
  // second, get an array of view indices for the selection..
  nsUInt32Array selection;
  GetSelectedIndices(&selection);
  PRInt32 numIndices = selection.GetSize();

  // now store the msg key for each selected item.
  nsMsgKey msgKey;
  for (PRInt32 index = 0; index < numIndices; index++)
  {
    msgKey = m_keys.GetAt(selection.GetAt(index));
    aMsgKeyArray->Add(msgKey);
  }

  return NS_OK;
}

nsresult nsMsgDBView::RestoreSelection(nsMsgKeyArray * aMsgKeyArray)
{
  if (!mOutlinerSelection)  // don't assert.
    return NS_OK;

  // first, unfreeze selection.
  mOutlinerSelection->ClearSelection(); // clear the existing selection.
  mOutlinerSelection->SetSelectEventsSuppressed(PR_FALSE);
  
  // second, turn our message keys into corresponding view indices
  PRInt32 arraySize = aMsgKeyArray->GetSize();
  nsMsgViewIndex	currentViewPosition = nsMsgViewIndex_None;
  nsMsgViewIndex	newViewPosition;
  // first, make sure the currentView was preserved....
  if (m_currentlyDisplayedMsgKey != nsMsgKey_None)
  {
    currentViewPosition = FindKey(m_currentlyDisplayedMsgKey, PR_FALSE);
    if (currentViewPosition != nsMsgViewIndex_None)
    {
      mOutlinerSelection->SetCurrentIndex(currentViewPosition);
      mOutlinerSelection->RangedSelect(currentViewPosition, currentViewPosition, PR_TRUE /* augment */);
    }
  }

  for (PRInt32 index = 0; index < arraySize; index ++)
  {
    newViewPosition = FindKey(aMsgKeyArray->GetAt(index), PR_FALSE);  
    // check to make sure newViewPosition is valid.
    // add the index back to the selection.
    if (newViewPosition != currentViewPosition) // don't re-add the current view
      mOutlinerSelection->RangedSelect(newViewPosition, newViewPosition, PR_TRUE /* augment */);
  }

  return NS_OK;
}

nsresult nsMsgDBView::GenerateURIForMsgKey(nsMsgKey aMsgKey, nsIMsgFolder *folder, char ** aURI)
{
  NS_ENSURE_ARG(folder);
  nsXPIDLCString baseURI;
  folder->GetBaseMessageURI(getter_Copies(baseURI));
  nsCAutoString uri;
  uri.Assign(baseURI);

  // append a "#" followed by the message key.
  uri.Append('#');
  uri.AppendInt(aMsgKey);

  *aURI = uri.ToNewCString();
  return NS_OK;
}

nsresult nsMsgDBView::CycleThreadedColumn(nsIDOMElement * aElement)
{
  nsAutoString currentView;

  // toggle threaded/unthreaded mode
  aElement->GetAttribute(NS_LITERAL_STRING("currentView"), currentView);
  if (currentView.Equals(NS_LITERAL_STRING("threaded")))
  {
    aElement->SetAttribute(NS_LITERAL_STRING("currentView"), NS_LITERAL_STRING("unthreaded"));

  }
  else
  {
     aElement->SetAttribute(NS_LITERAL_STRING("currentView"), NS_LITERAL_STRING("threaded"));
     // we must be unthreaded view. create a threaded view and replace ourself.

  }

  // i think we need to create a new view and switch it in this circumstance since
  // we are toggline between threaded and non threaded mode.


  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::IsEditable(PRInt32 index, const PRUnichar *colID, PRBool * aReturnValue)
{
  * aReturnValue = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::SetCellText(PRInt32 row, const PRUnichar *colID, const PRUnichar * value)
{
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::GetRowCount(PRInt32 *aRowCount)
{
  *aRowCount = GetSize();
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::GetSelection(nsIOutlinerSelection * *aSelection)
{
  *aSelection = mOutlinerSelection;
  NS_IF_ADDREF(*aSelection);
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::SetSelection(nsIOutlinerSelection * aSelection)
{
  mOutlinerSelection = aSelection;
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::SelectionChanged()
{
  // if the currentSelection changed then we have a message to display
  PRInt32 selectionCount; 
  nsresult rv = mOutlinerSelection->GetRangeCount(&selectionCount);
  NS_ENSURE_SUCCESS(rv, NS_OK); // outliner doesn't care if we failed

  // if only one item is selected then we want to display a message
  if (selectionCount == 1 && !mSupressMsgDisplay)
  {
    PRInt32 startRange;
    PRInt32 endRange;
    rv = mOutlinerSelection->GetRangeAt(0, &startRange, &endRange);
    NS_ENSURE_SUCCESS(rv, NS_OK); // outliner doesn't care if we failed

    if (startRange >= 0 && startRange == endRange && startRange < GetSize())
    {
      // get the msgkey for the message
      nsMsgKey msgkey = m_keys.GetAt(startRange);
      if (m_currentlyDisplayedMsgKey != msgkey)
      {
        nsXPIDLCString uri;
        rv = GenerateURIForMsgKey(msgkey, m_folder, getter_Copies(uri));
        NS_ENSURE_SUCCESS(rv,rv);
        mMessengerInstance->OpenURL(uri);
        m_currentlyDisplayedMsgKey = msgkey;
      }
    }
  }

  // determine if we need to push command update notifications out to the UI or not.
  PRUint32 numSelected = 0;
  GetNumSelected(&numSelected);
  // we need to push a command update notification iff, one of the following conditions are met
  // (1) the selection went from 0 to 1
  // (2) it went from 1 to 0
  // (3) it went from 1 to many
  // (4) it went from many to 1 or 0

  if (numSelected == mNumSelectedRows || 
      (numSelected > 1 && mNumSelectedRows > 1) )
  {

  }
  else if (mCommandUpdater) // o.t. push an updated
  {
    mCommandUpdater->UpdateCommandStatus();
  }
  
  mNumSelectedRows = numSelected;
  return NS_OK;
}

nsresult nsMsgDBView::GetSelectedIndices(nsUInt32Array *selection)
{
  if (mOutlinerSelection)
  {
    PRInt32 selectionCount; 
    nsresult rv = mOutlinerSelection->GetRangeCount(&selectionCount);
    for (PRInt32 i = 0; i < selectionCount; i++)
    {
      PRInt32 startRange;
      PRInt32 endRange;
      rv = mOutlinerSelection->GetRangeAt(i, &startRange, &endRange);
      NS_ENSURE_SUCCESS(rv, NS_OK); 
      PRInt32 viewSize = GetSize();
      if (startRange >= 0 && startRange < viewSize)
      {
        for (PRInt32 rangeIndex = startRange; rangeIndex <= endRange && rangeIndex < viewSize; rangeIndex++)
          selection->Add(rangeIndex);
      }
    }
  }
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::GetRowProperties(PRInt32 index, nsISupportsArray *properties)
{
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::GetColumnProperties(const PRUnichar *colID, nsIDOMElement * aElement, nsISupportsArray *properties)
{
  return NS_OK;
}


NS_IMETHODIMP nsMsgDBView::GetCellProperties(PRInt32 aRow, const PRUnichar *colID, nsISupportsArray *properties)
{
  // this is where we tell the outliner to apply styles to a particular row
  // i.e. if the row is an unread message...
  nsMsgKey key = m_keys.GetAt(aRow);
  nsCOMPtr <nsIMsgDBHdr> msgHdr;
  nsresult rv = NS_OK;

  if (key == m_cachedMsgKey)
    msgHdr = m_cachedHdr;
  else
  {
    GetMsgHdrForViewIndex(aRow, getter_AddRefs(msgHdr));
    if (NS_SUCCEEDED(rv))
    {
      m_cachedHdr = msgHdr;
      m_cachedMsgKey = key;
    }
    else
      return rv;
  }

  if (!msgHdr)
    return NS_MSG_INVALID_DBVIEW_INDEX;

  char    flags = m_flags.GetAt(aRow);
  if (!(flags & MSG_FLAG_READ))
    properties->AppendElement(kUnreadMsgAtom);  

  if (colID[0] == 'p') // for the priority column, add special styles....
  {
    nsMsgPriorityValue priority;
    msgHdr->GetPriority(&priority);
    switch (priority)
    {
    case nsMsgPriority::highest:
      properties->AppendElement(kHighestPriorityAtom);  
      break;
    case nsMsgPriority::high:
      properties->AppendElement(kHighPriorityAtom);  
      break;
    case nsMsgPriority::low:
      properties->AppendElement(kLowPriorityAtom);  
      break;
    case nsMsgPriority::lowest:
      properties->AppendElement(kLowestPriorityAtom);  
      break;
    default:
      break;
    }
  }
      
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::IsContainer(PRInt32 index, PRBool *_retval)
{
  if (m_viewFlags & nsMsgViewFlagsType::kThreadedDisplay)
  {
    PRUint32 flags = m_flags[index];
    *_retval = (flags & MSG_VIEW_FLAG_HASCHILDREN);
  }
  else
    *_retval = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::IsContainerOpen(PRInt32 index, PRBool *_retval)
{
  if (m_viewFlags & nsMsgViewFlagsType::kThreadedDisplay)
  {
    PRUint32 flags = m_flags[index];
    *_retval = (flags & MSG_VIEW_FLAG_HASCHILDREN) && !(flags & MSG_FLAG_ELIDED);
  }
  else
    *_retval = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::IsContainerEmpty(PRInt32 index, PRBool *_retval)
{
  if (m_viewFlags & nsMsgViewFlagsType::kThreadedDisplay)
  {
    PRUint32 flags = m_flags[index];
    *_retval = !(flags & MSG_VIEW_FLAG_HASCHILDREN);
  }
  else
    *_retval = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::GetParentIndex(PRInt32 rowIndex, PRInt32 *_retval)
{  
  *_retval = -1;

  PRInt32 rowIndexLevel;
  GetLevel(rowIndex, &rowIndexLevel);

  PRInt32 i;
  for(i = rowIndex; i >= 0; i--) {
    PRInt32 l;
    GetLevel(i, &l);
    if (l < rowIndexLevel) {
      *_retval = i;
      break;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::HasNextSibling(PRInt32 rowIndex, PRInt32 afterIndex, PRBool *_retval)
{
  *_retval = PR_FALSE;

  PRInt32 rowIndexLevel;
  GetLevel(rowIndex, &rowIndexLevel);

  PRInt32 i;
  PRInt32 count;
  GetRowCount(&count);
  for(i = afterIndex + 1; i < count - 1; i++) {
    PRInt32 l;
    GetLevel(i, &l);
    if (l < rowIndexLevel)
      break;
    if (l == rowIndexLevel) {
      *_retval = PR_TRUE;
      break;
    }
  }                                                                       

  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::GetLevel(PRInt32 index, PRInt32 *_retval)
{
  if (m_viewFlags & nsMsgViewFlagsType::kThreadedDisplay)
    *_retval = m_levels[index];
  else
    *_retval = 0;
  return NS_OK;
}

// search view will override this since headers can span db's
nsresult nsMsgDBView::GetMsgHdrForViewIndex(nsMsgViewIndex index, nsIMsgDBHdr **msgHdr)
{
  nsMsgKey key = m_keys.GetAt(index);
  return m_db->GetMsgHdrForKey(key, msgHdr);
}

nsresult nsMsgDBView::GetFolderForViewIndex(nsMsgViewIndex index, nsIMsgFolder **aFolder)
{
  *aFolder = m_folder;
  NS_IF_ADDREF(*aFolder);
  return NS_OK;
}

nsresult nsMsgDBView::GetDBForViewIndex(nsMsgViewIndex index, nsIMsgDatabase **db)
{
  *db = m_db;
  return NS_OK;
}
NS_IMETHODIMP nsMsgDBView::GetCellText(PRInt32 aRow, const PRUnichar * aColID, PRUnichar ** aValue)
{
  nsresult rv = NS_OK;
  nsMsgKey key = m_keys.GetAt(aRow);
  nsCOMPtr <nsIMsgDBHdr> msgHdr;
  if (key == m_cachedMsgKey)
    msgHdr = m_cachedHdr;
  else
  {
    GetMsgHdrForViewIndex(aRow, getter_AddRefs(msgHdr));
    if (NS_SUCCEEDED(rv))
    {
      m_cachedHdr = msgHdr;
      m_cachedMsgKey = key;
    }
    else
      return rv;
  }
  if (!msgHdr)
    return NS_MSG_INVALID_DBVIEW_INDEX;

  *aValue = 0;
  // just a hack
  nsXPIDLCString dbString;
  nsCOMPtr <nsIMsgThread> thread;

  switch (aColID[0])
  {
  case 's':
    if (aColID[1] == 'u') // subject
      rv = FetchSubject(msgHdr, m_flags[aRow], aValue);
    else if (aColID[1] == 'e') // sender
      rv = FetchAuthor(msgHdr, aValue);
    else if (aColID[1] == 'i') // size
      rv = FetchSize(msgHdr, aValue);
    else
      rv = FetchStatus(m_flags[aRow], aValue);
    break;
  case 'd':  // date
    rv = FetchDate(msgHdr, aValue);
    break;
  case 'p': // priority
    rv = FetchPriority(msgHdr, aValue);
    break;
  case 't':   // threaded mode (this is temporary...it's how we are faking twisties
    if (aColID[1] == 'h')
    {
      if (m_viewFlags & nsMsgViewFlagsType::kThreadedDisplay)
      {
        // if it's an open container, print a '-' sign. If it's a closed container,
        // print a "+" sign. o.t. do nothing.
        if (m_flags[aRow] & MSG_FLAG_ELIDED)
          *aValue = nsCRT::strdup(NS_LITERAL_STRING("+"));
        else if (m_flags[aRow] & MSG_VIEW_FLAG_HASCHILDREN)
          *aValue = nsCRT::strdup(NS_LITERAL_STRING("-"));      
      }
    }
    // total msgs in thread column
    else  if (m_viewFlags & nsMsgViewFlagsType::kThreadedDisplay)
    {
      if (m_flags[aRow] & MSG_VIEW_FLAG_ISTHREAD)
      {
        rv = GetThreadContainingIndex(aRow, getter_AddRefs(thread));
        if (NS_SUCCEEDED(rv) && thread)
        {
          nsAutoString formattedCountString;
          PRUint32 numChildren;
          thread->GetNumChildren(&numChildren);
          formattedCountString.AppendInt(numChildren);
          *aValue = formattedCountString.ToNewUnicode();
        }
      }
    }
    break;
  case 'f':
    if (m_flags[aRow] & MSG_FLAG_MARKED)
      *aValue = nsCRT::strdup(NS_LITERAL_STRING("*"));
    else
      *aValue = nsCRT::strdup(NS_LITERAL_STRING("'"));
    break;      
  case 'u': // unread button column
    if (aColID[6] == 'B')
    {
      if (m_flags[aRow] & MSG_FLAG_READ)
        *aValue = nsCRT::strdup(NS_LITERAL_STRING("'"));
      else
        *aValue = nsCRT::strdup(NS_LITERAL_STRING("*"));
    }
    // unread msgs in thread col
    else if (m_viewFlags & nsMsgViewFlagsType::kThreadedDisplay)
    {
      if (m_flags[aRow] & MSG_VIEW_FLAG_ISTHREAD)
      {
        rv = GetThreadContainingIndex(aRow, getter_AddRefs(thread));
        if (NS_SUCCEEDED(rv) && thread)
        {
          nsAutoString formattedCountString;
          PRUint32 numUnreadChildren;
          thread->GetNumUnreadChildren(&numUnreadChildren);
          if (numUnreadChildren > 0)
          {
            formattedCountString.AppendInt(numUnreadChildren);
            *aValue = formattedCountString.ToNewUnicode();
          }
        }
      }
    }
    break;
  default:
    break;
  }

  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::SetOutliner(nsIOutlinerBoxObject *outliner)
{
  mOutliner = outliner;
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::ToggleOpenState(PRInt32 index)
{
  PRUint32 numChanged;
  nsresult rv = ToggleExpansion(index, &numChanged);
  return rv;
}

NS_IMETHODIMP nsMsgDBView::CycleHeader(const PRUnichar * aColID, nsIDOMElement * aElement)
{
  // if the header is a sortable column then we want to call Sort
  // otherwise, we'll do something else =)

  nsMsgViewSortTypeValue sortType = nsMsgViewSortType::bySubject;
  nsMsgViewSortOrderValue sortOrder = nsMsgViewSortOrder::descending;
  PRBool performSort = PR_TRUE;

  nsAutoString sortOrderValue;
  aElement->GetAttribute(NS_LITERAL_STRING("sortDirection"), sortOrderValue);
  if (!sortOrderValue.IsEmpty() && sortOrderValue.Equals(NS_LITERAL_STRING("ascending")))
     sortOrder = nsMsgViewSortOrder::ascending;

  if ((aColID[0] != 't') && (aColID[1] != 'h')) {
    m_viewFlags &= ~nsMsgViewFlagsType::kThreadedDisplay;
  }

  switch (aColID[0])
  {
  case 's':
    if (aColID[1] == 'u') // sort the subject
    {
      sortType = nsMsgViewSortType::bySubject;
    }
    else if (aColID[1] == 'e') // sort by sender
    {
      sortType = nsMsgViewSortType::byAuthor;
    }
    else if (aColID[1] == 'i') // size
    {
      sortType = nsMsgViewSortType::bySize;
    }
    else
    {
      sortType = nsMsgViewSortType::byStatus;
    }
    break;
  case 'u': 
    if (aColID[6] == 'B') // unreadButtonColHeader
    {
      sortType = nsMsgViewSortType::byUnread;
    }
    else  // unreadCol
    {
      printf("fix me\n");
      performSort = PR_FALSE;
    }
    break;
  case 'd': // date
    sortType = nsMsgViewSortType::byDate;
    break;
  case 'p': // priority
    sortType = nsMsgViewSortType::byPriority;
    break;
  case 't': // thread column
    if (aColID[1] == 'h') {
      sortType = nsMsgViewSortType::byThread;
      m_viewFlags |= nsMsgViewFlagsType::kThreadedDisplay;
    }
    else {
      //sortType = nsMsgViewSortType::byTotal;
      printf("fix me\n");
      performSort = PR_FALSE;
    }
    break;
  case 'f': // flagged
    sortType = nsMsgViewSortType::byFlagged;
    break;
  default:
    performSort = PR_FALSE;
    break;
  }
  
  PRInt32 countBeforeSort;
  GetRowCount(&countBeforeSort);

  if (performSort)
  {
    // if we are already sorted by the same order, then toggle ascending / descending.
    if (m_sortType == sortType)
    {
      if (sortOrder == nsMsgViewSortOrder::ascending)
        sortOrder = nsMsgViewSortOrder::descending;
      else
        sortOrder = nsMsgViewSortOrder::ascending;
    }

    Sort(sortType, sortOrder);
    UpdateSortUI(aElement);

  } // if performSort

  PRInt32 countAfterSort;
  GetRowCount(&countAfterSort);

  if (countBeforeSort != countAfterSort) {
    mOutliner->RowCountChanged(0, -countBeforeSort);
    mOutliner->RowCountChanged(0, countAfterSort);
  }

  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::CycleCell(PRInt32 row, const PRUnichar *colID)
{
  switch (colID[0])
  {
  case 'u': // unreadButtonColHeader
    if (colID[6] == 'B') {
      ToggleReadByIndex(row);
    }
   break;
  case 't': // threaded cell or total cell
    if ((colID[1] == 'h') && ((m_viewFlags & nsMsgViewFlagsType::kThreadedDisplay) && (m_flags [row] & MSG_VIEW_FLAG_HASCHILDREN))) // 'th' for threaded, 'to' for total
    {
      PRUint32 numChanged = 0;
      PRInt32 multiplier = -1;
      if (m_flags [row] & MSG_FLAG_ELIDED)
        multiplier = 1;
      ToggleExpansion(row, &numChanged);

      mOutliner->RowCountChanged(row, numChanged * multiplier);
    }
    break;
  case 'f': // flagged column
    // toggle the flagged status of the element at row.
    if (m_flags[row] & MSG_FLAG_MARKED)
      SetFlaggedByIndex(row, PR_FALSE);
    else
      SetFlaggedByIndex(row, PR_TRUE);
    break;
  default:
    break;

  }
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::PerformAction(const PRUnichar *action)
{
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::PerformActionOnRow(const PRUnichar *action, PRInt32 row)
{
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::PerformActionOnCell(const PRUnichar *action, PRInt32 row, const PRUnichar *colID)
{
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::GetSortedColumn(nsIDOMElement ** aColumn)
{
  *aColumn = mCurrentSortColumn;
  NS_IF_ADDREF(*aColumn);

  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::SetSortedColumn(nsIDOMElement * aColumn)
{
  if (!mCurrentSortColumn)
    UpdateSortUI(aColumn);

  mCurrentSortColumn = aColumn;
  return NS_OK;
}

///////////////////////////////////////////////////////////////////////////
// end nsIOutlinerView Implementation Methods
///////////////////////////////////////////////////////////////////////////

NS_IMETHODIMP nsMsgDBView::Open(nsIMsgFolder *folder, nsMsgViewSortTypeValue sortType, nsMsgViewSortOrderValue sortOrder, nsMsgViewFlagsTypeValue viewFlags, PRInt32 *pCount)
{

  m_viewFlags = viewFlags;
  m_sortOrder = sortOrder;
  m_sortType = sortType;

  if (folder) // search view will have a null folder
  {
    nsCOMPtr <nsIDBFolderInfo> folderInfo;
    nsresult rv = folder->GetDBFolderInfoAndDB(getter_AddRefs(folderInfo), getter_AddRefs(m_db));
    NS_ENSURE_SUCCESS(rv,rv);
	  m_db->AddListener(this);
    m_folder = folder;
  }
#ifdef HAVE_PORT
	CacheAdd ();
#endif

	return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::Close()
{
  if (m_db)
  {
  	m_db->RemoveListener(this);
    m_db = nsnull;
  }
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::Init(nsIMessenger * aMessengerInstance, nsIMsgWindow * aMsgWindow, nsIMsgDBViewCommandUpdater *aCmdUpdater)
{
  mMsgWindow = aMsgWindow;
  mMessengerInstance = aMessengerInstance;
  mCommandUpdater = aCmdUpdater;

  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::SetSupressMsgDisplay(PRBool aSupressDisplay)
{
  mSupressMsgDisplay = aSupressDisplay;
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::GetSupressMsgDisplay(PRBool * aSupressDisplay)
{
  *aSupressDisplay = mSupressMsgDisplay;
  return NS_OK;
}

nsresult nsMsgDBView::AddKeys(nsMsgKey *pKeys, PRInt32 *pFlags, const char *pLevels, nsMsgViewSortTypeValue sortType, PRInt32 numKeysToAdd)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

int PR_CALLBACK CompareViewIndices (const void *v1, const void *v2, void *)
{
	nsMsgViewIndex i1 = *(nsMsgViewIndex*) v1;
	nsMsgViewIndex i2 = *(nsMsgViewIndex*) v2;
	return i1 - i2;
}

NS_IMETHODIMP nsMsgDBView::GetIndicesForSelection(nsMsgViewIndex **indices,  PRUint32 *length)
{
  NS_ENSURE_ARG_POINTER(length);
  *length = 0;
  NS_ENSURE_ARG_POINTER(indices);
  *indices = nsnull;

  nsUInt32Array selection;
  GetSelectedIndices(&selection);
  *length = selection.GetSize();
  PRUint32 numIndicies = *length;
  if (!numIndicies) return NS_OK;

  *indices = (nsMsgViewIndex *)nsMemory::Alloc(numIndicies * sizeof(nsMsgViewIndex));
  if (!indices) return NS_ERROR_OUT_OF_MEMORY;
  for (PRUint32 i=0;i<numIndicies;i++) {
    (*indices)[i] = selection.GetAt(i);
  }
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::GetURIsForSelection(char ***uris, PRUint32 *length)
{
  nsresult rv = NS_OK;

#ifdef DEBUG_mscott
  printf("inside GetURIsForSelection\n");
#endif
  NS_ENSURE_ARG_POINTER(length);
  *length = 0;
  NS_ENSURE_ARG_POINTER(uris);
  *uris = nsnull;

  nsUInt32Array selection;
  GetSelectedIndices(&selection);
  *length = selection.GetSize();
  PRUint32 numIndicies = *length;
  if (!numIndicies) return NS_OK;

  nsCOMPtr <nsIMsgFolder> folder = m_folder;
  char **outArray, **next;
  next = outArray = (char **)nsMemory::Alloc(numIndicies * sizeof(char *));
  if (!outArray) return NS_ERROR_OUT_OF_MEMORY;
  for (PRUint32 i=0;i<numIndicies;i++) 
  {
    nsMsgViewIndex selectedIndex = selection.GetAt(i);
    if (!folder)
      GetFolderForViewIndex(selectedIndex, getter_AddRefs(folder));
    rv = GenerateURIForMsgKey(m_keys[selectedIndex], folder, next);
    NS_ENSURE_SUCCESS(rv,rv);
    if (!*next) return NS_ERROR_OUT_OF_MEMORY;
    next++;
  }

  *uris = outArray;
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::GetURIForViewIndex(nsMsgViewIndex index, char **result)
{
  nsresult rv;
  nsCOMPtr <nsIMsgFolder> folder = m_folder;
  if (!folder) {
    rv = GetFolderForViewIndex(index, getter_AddRefs(folder));
    NS_ENSURE_SUCCESS(rv,rv);
  }
  rv = GenerateURIForMsgKey(m_keys[index], folder, result);
  NS_ENSURE_SUCCESS(rv,rv);
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::DoCommandWithFolder(nsMsgViewCommandTypeValue command, nsIMsgFolder *destFolder)
{
  nsUInt32Array selection;

  NS_ENSURE_ARG_POINTER(destFolder);

  GetSelectedIndices(&selection);

  nsMsgViewIndex *indices = selection.GetData();
  PRInt32 numIndices = selection.GetSize();

  nsresult rv = NS_OK;
  switch (command) {
    case nsMsgViewCommandType::copyMessages:
    case nsMsgViewCommandType::moveMessages:
        // since the FE could have constructed the list of indices in
        // any order (e.g. order of discontiguous selection), we have to
        // sort the indices in order to find out which nsMsgViewIndex will
        // be deleted first.
        if (numIndices > 1) {
            NS_QuickSort (indices, numIndices, sizeof(nsMsgViewIndex), CompareViewIndices, nsnull);
        }
        NoteStartChange(nsMsgViewNotificationCode::none, 0, 0);
        rv = ApplyCommandToIndicesWithFolder(command, indices, numIndices, destFolder);
        NoteEndChange(nsMsgViewNotificationCode::none, 0, 0);
        break;
    default:
        NS_ASSERTION(PR_FALSE, "invalid command type");
        rv = NS_ERROR_UNEXPECTED;
        break;
  }
  return rv;

}

NS_IMETHODIMP nsMsgDBView::DoCommand(nsMsgViewCommandTypeValue command)
{
  nsUInt32Array selection;

  GetSelectedIndices(&selection);

  nsMsgViewIndex *indices = selection.GetData();
  PRInt32 numIndices = selection.GetSize();

  nsresult rv = NS_OK;
  switch (command)
  {

  case nsMsgViewCommandType::markMessagesRead:
  case nsMsgViewCommandType::markMessagesUnread:
  case nsMsgViewCommandType::toggleMessageRead:
  case nsMsgViewCommandType::flagMessages:
  case nsMsgViewCommandType::unflagMessages:
  case nsMsgViewCommandType::deleteMsg:
  case nsMsgViewCommandType::deleteNoTrash:
  case nsMsgViewCommandType::markThreadRead:
		// since the FE could have constructed the list of indices in
		// any order (e.g. order of discontiguous selection), we have to
		// sort the indices in order to find out which nsMsgViewIndex will
		// be deleted first.
		if (numIndices > 1)
			NS_QuickSort (indices, numIndices, sizeof(nsMsgViewIndex), CompareViewIndices, nsnull);
#ifdef DOING_UNDO
		if (command != nsMsgViewCommandType::deleteMsg && 
			command != nsMsgViewCommandType::deleteNoTrash)
			GetUndoManager()->AddUndoAction(
				new MarkMessageUndoAction(this, command, indices, 
										  numIndices, GetFolder()));
#endif
//		FEEnd();
//		calledFEEnd = TRUE;
    NoteStartChange(nsMsgViewNotificationCode::none, 0, 0);
		rv = ApplyCommandToIndices(command, indices, numIndices);
    NoteEndChange(nsMsgViewNotificationCode::none, 0, 0);
    break;
  case nsMsgViewCommandType::selectAll:
    if (mOutlinerSelection && mOutliner) {
        // if in threaded mode, we need to expand all before selecting
        if (m_sortType == nsMsgViewSortType::byThread) {
            rv = ExpandAll();
        }
        mOutlinerSelection->SelectAll();
        mOutliner->Invalidate();
    }
    break;
  case nsMsgViewCommandType::markAllRead:
    if (m_folder)
      rv = m_folder->MarkAllMessagesRead();
    break;
  case nsMsgViewCommandType::toggleThreadWatched:
    rv = ToggleWatched(indices,	numIndices);
    break;
  case nsMsgViewCommandType::expandAll:
    rv = ExpandAll();
    mOutliner->Invalidate();
    break;
  case nsMsgViewCommandType::collapseAll:
    rv = CollapseAll();
    mOutliner->Invalidate();
    break;
  default:
    NS_ASSERTION(PR_FALSE, "invalid command type");
    rv = NS_ERROR_UNEXPECTED;
    break;
  }
  return rv;
}

NS_IMETHODIMP nsMsgDBView::GetCommandStatus(nsMsgViewCommandTypeValue command, PRBool *selectable_p, nsMsgViewCommandCheckStateValue *selected_p)
{
  nsUInt32Array selection;

  GetSelectedIndices(&selection);

  //nsMsgViewIndex *indices = selection.GetData();
  PRInt32 numindices = selection.GetSize();

  nsresult rv = NS_OK;
  switch (command)
  {
  case nsMsgViewCommandType::markMessagesRead:
  case nsMsgViewCommandType::markMessagesUnread:
  case nsMsgViewCommandType::toggleMessageRead:
  case nsMsgViewCommandType::flagMessages:
  case nsMsgViewCommandType::unflagMessages:
  case nsMsgViewCommandType::toggleThreadWatched:
  case nsMsgViewCommandType::deleteMsg:
  case nsMsgViewCommandType::deleteNoTrash:
  case nsMsgViewCommandType::markThreadRead:
    *selectable_p = (numindices > 0);
    break;
  case nsMsgViewCommandType::markAllRead:
    printf("fix me\n");
    break;
  case nsMsgViewCommandType::expandAll:
  case nsMsgViewCommandType::collapseAll:
    *selectable_p = (m_sortType == nsMsgViewSortType::byThread);
    break;
  default:
    NS_ASSERTION(PR_FALSE, "invalid command type");
    rv = NS_ERROR_FAILURE;
  }
  return rv;
}

nsresult 
nsMsgDBView::CopyMessages(nsIMsgWindow *window, nsMsgViewIndex *indices, PRInt32 numIndices, PRBool isMove, nsIMsgFolder *destFolder)
{
  nsresult rv = NS_OK;
  NS_ENSURE_ARG_POINTER(destFolder);
  nsCOMPtr<nsISupportsArray> messageArray;
  NS_NewISupportsArray(getter_AddRefs(messageArray));
  for (nsMsgViewIndex index = 0; index < (nsMsgViewIndex) numIndices; index++) {
    nsMsgKey key = m_keys.GetAt(indices[index]);
    nsCOMPtr <nsIMsgDBHdr> msgHdr;
    rv = m_db->GetMsgHdrForKey(key, getter_AddRefs(msgHdr));
    NS_ENSURE_SUCCESS(rv,rv);
    if (msgHdr)
      messageArray->AppendElement(msgHdr);
  }
  rv = destFolder->CopyMessages(m_folder /* source folder */, messageArray, isMove, window, nsnull /* listener */, PR_FALSE /* isFolder */);
  return rv;
}

nsresult
nsMsgDBView::ApplyCommandToIndicesWithFolder(nsMsgViewCommandTypeValue command, nsMsgViewIndex* indices,
                    PRInt32 numIndices, nsIMsgFolder *destFolder)
{
  nsresult rv;

  NS_ENSURE_ARG_POINTER(destFolder);

  switch (command) {
    case nsMsgViewCommandType::copyMessages:
        rv = CopyMessages(mMsgWindow, indices, numIndices, PR_FALSE /* isMove */, destFolder);
        break;
    case nsMsgViewCommandType::moveMessages:
        rv = CopyMessages(mMsgWindow, indices, numIndices, PR_TRUE  /* isMove */, destFolder);
        break;
    default:
        NS_ASSERTION(PR_FALSE, "unhandled command");
        rv = NS_ERROR_UNEXPECTED;
        break;
    }
    return rv;
}

nsresult
nsMsgDBView::ApplyCommandToIndices(nsMsgViewCommandTypeValue command, nsMsgViewIndex* indices,
					PRInt32 numIndices)
{
	nsresult rv = NS_OK;
	nsMsgKeyArray imapUids;

  nsCOMPtr <nsIMsgImapMailFolder> imapFolder = do_QueryInterface(m_folder);
	PRBool thisIsImapFolder = (imapFolder != nsnull);
  if (command == nsMsgViewCommandType::deleteMsg)
		rv = DeleteMessages (mMsgWindow, indices, numIndices, PR_FALSE);
	else if (command == nsMsgViewCommandType::deleteNoTrash)
		rv = DeleteMessages(mMsgWindow, indices, numIndices, PR_TRUE);
	else
	{
		for (int32 i = 0; i < numIndices; i++)
		{
			if (thisIsImapFolder && command != nsMsgViewCommandType::markThreadRead)
				imapUids.Add(GetAt(indices[i]));
			
			switch (command)
			{
			case nsMsgViewCommandType::markMessagesRead:
				rv = SetReadByIndex(indices[i], PR_TRUE);
				break;
			case nsMsgViewCommandType::markMessagesUnread:
				rv = SetReadByIndex(indices[i], PR_FALSE);
				break;
			case nsMsgViewCommandType::toggleMessageRead:
				rv = ToggleReadByIndex(indices[i]);
				break;
			case nsMsgViewCommandType::flagMessages:
				rv = SetFlaggedByIndex(indices[i], PR_TRUE);
				break;
			case nsMsgViewCommandType::unflagMessages:
				rv = SetFlaggedByIndex(indices[i], PR_FALSE);
				break;
			case nsMsgViewCommandType::markThreadRead:
				rv = SetThreadOfMsgReadByIndex(indices[i], imapUids, PR_TRUE);
				break;
			default:
				NS_ASSERTION(PR_FALSE, "unhandled command");
				break;
			}
		}
		
		if (thisIsImapFolder)
		{
			imapMessageFlagsType flags = kNoImapMsgFlag;
			PRBool addFlags = PR_FALSE;
			PRBool isRead = PR_FALSE;

			switch (command)
			{
			case nsMsgViewCommandType::markThreadRead:
			case nsMsgViewCommandType::markMessagesRead:
				flags |= kImapMsgSeenFlag;
				addFlags = PR_TRUE;
				break;
			case nsMsgViewCommandType::markMessagesUnread:
				flags |= kImapMsgSeenFlag;
				addFlags = PR_FALSE;
				break;
			case nsMsgViewCommandType::toggleMessageRead:
				{
					flags |= kImapMsgSeenFlag;
					m_db->IsRead(GetAt(indices[0]), &isRead);
					if (isRead)
						addFlags = PR_TRUE;
					else
            addFlags = PR_FALSE;
				}
				break;
			case nsMsgViewCommandType::flagMessages:
				flags |= kImapMsgFlaggedFlag;
				addFlags = PR_TRUE;
				break;
			case nsMsgViewCommandType::unflagMessages:
				flags |= kImapMsgFlaggedFlag;
        addFlags = PR_FALSE;
				break;
			default:
				break;
			}
			
			if (flags != kNoImapMsgFlag)	// can't get here without thisIsImapThreadPane == TRUE
				imapFolder->StoreImapFlags(flags, addFlags, imapUids.GetArray(), imapUids.GetSize());
			
		}
	}
	return rv;
}
// view modifications methods by index

// This method just removes the specified line from the view. It does
// NOT delete it from the database.
nsresult nsMsgDBView::RemoveByIndex(nsMsgViewIndex index)
{
	if (!IsValidIndex(index))
		return NS_MSG_INVALID_DBVIEW_INDEX;

	m_keys.RemoveAt(index);
	m_flags.RemoveAt(index);
	m_levels.RemoveAt(index);
	NoteChange(index, -1, nsMsgViewNotificationCode::insertOrDelete);
	return NS_OK;
}

nsresult nsMsgDBView::DeleteMessages(nsIMsgWindow *window, nsMsgViewIndex *indices, PRInt32 numIndices, PRBool deleteStorage)
{
  nsresult rv = NS_OK;
	nsCOMPtr<nsISupportsArray> messageArray;
	NS_NewISupportsArray(getter_AddRefs(messageArray));
  for (nsMsgViewIndex index = 0; index < (nsMsgViewIndex) numIndices; index++)
  {
    nsMsgKey key = m_keys.GetAt(indices[index]);
    nsCOMPtr <nsIMsgDBHdr> msgHdr;
    rv = m_db->GetMsgHdrForKey(key, getter_AddRefs(msgHdr));
    NS_ENSURE_SUCCESS(rv,rv);
    if (msgHdr)
      messageArray->AppendElement(msgHdr);

  }
  m_folder->DeleteMessages(messageArray, window, deleteStorage, PR_FALSE);
  return rv;
}


// read/unread handling.
nsresult nsMsgDBView::ToggleReadByIndex(nsMsgViewIndex index)
{
	if (!IsValidIndex(index))
		return NS_MSG_INVALID_DBVIEW_INDEX;
	return SetReadByIndex(index, !(m_flags[index] & MSG_FLAG_READ));
}

nsresult nsMsgDBView::SetReadByIndex(nsMsgViewIndex index, PRBool read)
{
	nsresult rv;

	if (!IsValidIndex(index))
		return NS_MSG_INVALID_DBVIEW_INDEX;
	if (read)
		OrExtraFlag(index, MSG_FLAG_READ);
	else
		AndExtraFlag(index, ~MSG_FLAG_READ);

  nsCOMPtr <nsIMsgDatabase> dbToUse;
  rv = GetDBForViewIndex(index, getter_AddRefs(dbToUse));
  NS_ENSURE_SUCCESS(rv, rv);

	rv = dbToUse->MarkRead(m_keys[index], read, this);
	NoteChange(index, 1, nsMsgViewNotificationCode::changed);
	if (m_sortType == nsMsgViewSortType::byThread)
	{
		nsMsgViewIndex threadIndex = ThreadIndexOfMsg(m_keys[index], index, nsnull, nsnull);
		if (threadIndex != index)
			NoteChange(threadIndex, 1, nsMsgViewNotificationCode::changed);
	}
	return rv;
}

nsresult nsMsgDBView::SetThreadOfMsgReadByIndex(nsMsgViewIndex index, nsMsgKeyArray &keysMarkedRead, PRBool /*read*/)
{
	nsresult rv;

	if (!IsValidIndex(index))
		return NS_MSG_INVALID_DBVIEW_INDEX;
	rv = MarkThreadOfMsgRead(m_keys[index], index, keysMarkedRead, PR_TRUE);
	return rv;
}

nsresult nsMsgDBView::SetFlaggedByIndex(nsMsgViewIndex index, PRBool mark)
{
	nsresult rv;

	if (!IsValidIndex(index))
		return NS_MSG_INVALID_DBVIEW_INDEX;

  nsCOMPtr <nsIMsgDatabase> dbToUse;
  rv = GetDBForViewIndex(index, getter_AddRefs(dbToUse));
  NS_ENSURE_SUCCESS(rv, rv);

	if (mark)
		OrExtraFlag(index, MSG_FLAG_MARKED);
	else
		AndExtraFlag(index, ~MSG_FLAG_MARKED);

	rv = dbToUse->MarkMarked(m_keys[index], mark, this);
	NoteChange(index, 1, nsMsgViewNotificationCode::changed);
	return rv;
}



// reversing threads involves reversing the threads but leaving the
// expanded messages ordered relative to the thread, so we
// make a copy of each array and copy them over.
nsresult nsMsgDBView::ReverseThreads()
{
    nsUInt32Array *newFlagArray = new nsUInt32Array;
    if (!newFlagArray) {
        return NS_ERROR_OUT_OF_MEMORY;
    }
    nsMsgKeyArray *newKeyArray = new nsMsgKeyArray;
    if (!newKeyArray) {
        delete newFlagArray;
        return NS_ERROR_OUT_OF_MEMORY;
    }
    nsUint8Array *newLevelArray = new nsUint8Array;
    if (!newLevelArray) {
        delete newFlagArray;
        delete newKeyArray;
        return NS_ERROR_OUT_OF_MEMORY;
    }

    PRInt32 sourceIndex, destIndex;
    PRInt32 viewSize = GetSize();

    newKeyArray->SetSize(m_keys.GetSize());
    newFlagArray->SetSize(m_flags.GetSize());
    newLevelArray->SetSize(m_levels.GetSize());

    for (sourceIndex = 0, destIndex = viewSize - 1; sourceIndex < viewSize;) {
        PRInt32 endThread;  // find end of current thread.
        PRBool inExpandedThread = PR_FALSE;
        for (endThread = sourceIndex; endThread < viewSize; endThread++) {
            PRUint32 flags = m_flags.GetAt(endThread);
            if (!inExpandedThread && (flags & (MSG_VIEW_FLAG_ISTHREAD|MSG_VIEW_FLAG_HASCHILDREN)) && !(flags & MSG_FLAG_ELIDED))
                inExpandedThread = PR_TRUE;
            else if (flags & MSG_VIEW_FLAG_ISTHREAD) {
                if (inExpandedThread)
                    endThread--;
                break;
            }
        }

        if (endThread == viewSize)
            endThread--;
        PRInt32 saveEndThread = endThread;
        while (endThread >= sourceIndex)
        {
            newKeyArray->SetAt(destIndex, m_keys.GetAt(endThread));
            newFlagArray->SetAt(destIndex, m_flags.GetAt(endThread));
            newLevelArray->SetAt(destIndex, m_levels.GetAt(endThread));
            endThread--;
            destIndex--;
        }
        sourceIndex = saveEndThread + 1;
    }
    // this copies the contents of both arrays - it would be cheaper to
    // just assign the new data ptrs to the old arrays and "forget" the new
    // arrays' data ptrs, so they won't be freed when the arrays are deleted.
    m_keys.RemoveAll();
    m_flags.RemoveAll();
    m_levels.RemoveAll();
    m_keys.InsertAt(0, newKeyArray);
    m_flags.InsertAt(0, newFlagArray);
    m_levels.InsertAt(0, newLevelArray);

    // if we swizzle data pointers for these arrays, this won't be right.
    delete newFlagArray;
    delete newKeyArray;
    delete newLevelArray;

    return NS_OK;
}

nsresult nsMsgDBView::ReverseSort()
{
    PRUint32 num = GetSize();

    // go up half the array swapping values
    for (PRUint32 i = 0; i < (num / 2); i++) {
        // swap flags
        PRUint32 end = num - i - 1;
        PRUint32 tempFlags = m_flags.GetAt(i);
        m_flags.SetAt(i, m_flags.GetAt(end));
        m_flags.SetAt(end, tempFlags);

        // swap keys
        nsMsgKey tempKey = m_keys.GetAt(i);
        m_keys.SetAt(i, m_keys.GetAt(end));
        m_keys.SetAt(end, tempKey);

        // no need to swap elements in m_levels, 
        // since we won't call ReverseSort() if we
        // are in threaded mode, so m_levels are all the same.
    }

    return NS_OK;
}

typedef struct entryInfo {
    nsMsgKey    id;
    PRUint32    bits;
    PRUint32    len;
    PRUint32    pad;
} EntryInfo;

typedef struct tagIdKey {
    EntryInfo   info;
    PRUint8     key[1];
} IdKey;


typedef struct tagIdPtrKey {
    EntryInfo   info;
    PRUint8     *key;
} IdKeyPtr;

int PR_CALLBACK
FnSortIdKey(const void *pItem1, const void *pItem2, void *privateData)
{
    PRInt32 retVal = 0;
    nsresult rv;

    IdKey** p1 = (IdKey**)pItem1;
    IdKey** p2 = (IdKey**)pItem2;

    nsIMsgDatabase *db = (nsIMsgDatabase *)privateData;

    rv = db->CompareCollationKeys((*p1)->key,(*p1)->info.len,(*p2)->key,(*p2)->info.len,&retVal);
    NS_ASSERTION(NS_SUCCEEDED(rv),"compare failed");

    if (retVal != 0)
        return(retVal);
    if ((*p1)->info.id >= (*p2)->info.id)
        return(1);
    else
        return(-1);
}

int PR_CALLBACK
FnSortIdKeyPtr(const void *pItem1, const void *pItem2, void *privateData)
{
    PRInt32 retVal = 0;
    nsresult rv;

    IdKeyPtr** p1 = (IdKeyPtr**)pItem1;
    IdKeyPtr** p2 = (IdKeyPtr**)pItem2;

    nsIMsgDatabase *db = (nsIMsgDatabase *)privateData;

    rv = db->CompareCollationKeys((*p1)->key,(*p1)->info.len,(*p2)->key,(*p2)->info.len,&retVal);
    NS_ASSERTION(NS_SUCCEEDED(rv),"compare failed");

    if (retVal != 0)
        return(retVal);
    if ((*p1)->info.id >= (*p2)->info.id)
        return(1);
    else
        return(-1);
}


typedef struct tagIdDWord {
    EntryInfo   info;
    PRUint32    dword;
} IdDWord;

int PR_CALLBACK
FnSortIdDWord(const void *pItem1, const void *pItem2, void *privateData)
{
    IdDWord** p1 = (IdDWord**)pItem1;
    IdDWord** p2 = (IdDWord**)pItem2;
    if ((*p1)->dword > (*p2)->dword)
        return(1);
    else if ((*p1)->dword < (*p2)->dword)
        return(-1);
    else if ((*p1)->info.id >= (*p2)->info.id)
        return(1);
    else
        return(-1);
}

typedef struct tagIdPRTime {
    EntryInfo   info;
    PRTime      prtime;
} IdPRTime;

int PR_CALLBACK
FnSortIdPRTime(const void *pItem1, const void *pItem2, void *privateData)
{
    IdPRTime ** p1 = (IdPRTime**)pItem1;
    IdPRTime ** p2 = (IdPRTime**)pItem2;

    if (LL_CMP((*p1)->prtime, >, (*p2)->prtime)) 
        return(1);
    else if (LL_CMP((*p1)->prtime, <, (*p2)->prtime))  
        return(-1);
    else if ((*p1)->info.id >= (*p2)->info.id)
        return(1);
    else
        return(-1);
}

// XXX are these still correct? 
const int kMaxSubjectKey = 160;
const int kMaxAuthorKey = 160;
const int kMaxRecipientKey = 80;

nsresult nsMsgDBView::GetFieldTypeAndLenForSort(nsMsgViewSortTypeValue sortType, PRUint16 *pMaxLen, eFieldType *pFieldType)
{
    NS_ENSURE_ARG_POINTER(pMaxLen);
    NS_ENSURE_ARG_POINTER(pFieldType);

    switch (sortType) {
        case nsMsgViewSortType::bySubject:
            *pFieldType = kCollationKey;
            *pMaxLen = kMaxSubjectKey;
            break;
        case nsMsgViewSortType::byRecipient:
            *pFieldType = kCollationKey;
            *pMaxLen = kMaxRecipientKey;
            break;
        case nsMsgViewSortType::byAuthor:
            *pFieldType = kCollationKey;
            *pMaxLen = kMaxAuthorKey;
            break;
        case nsMsgViewSortType::byDate:
            *pFieldType = kPRTime;
            *pMaxLen = sizeof(PRTime);
            break;
        case nsMsgViewSortType::byPriority:
        case nsMsgViewSortType::byThread:
        case nsMsgViewSortType::byId:
        case nsMsgViewSortType::bySize:
        case nsMsgViewSortType::byFlagged:
        case nsMsgViewSortType::byUnread:
        case nsMsgViewSortType::byStatus:
            *pFieldType = kU32;
            *pMaxLen = sizeof(PRUint32);
            break;
        default:
            return NS_ERROR_UNEXPECTED;
    }

    return NS_OK;
}

nsresult nsMsgDBView::GetPRTimeField(nsIMsgHdr *msgHdr, nsMsgViewSortTypeValue sortType, PRTime *result)
{
  nsresult rv;
  NS_ENSURE_ARG_POINTER(msgHdr);
  NS_ENSURE_ARG_POINTER(result);

  switch (sortType) {
    case nsMsgViewSortType::byDate:
        rv = msgHdr->GetDate(result);
        break;
    default:
        NS_ASSERTION(0,"should not be here");
        rv = NS_ERROR_UNEXPECTED;
    }

    NS_ENSURE_SUCCESS(rv,rv);
    return NS_OK;
}

nsresult nsMsgDBView::GetLongField(nsIMsgHdr *msgHdr, nsMsgViewSortTypeValue sortType, PRUint32 *result)
{
  nsresult rv;
  NS_ENSURE_ARG_POINTER(msgHdr);
  NS_ENSURE_ARG_POINTER(result);

  PRBool isRead;
  PRUint32 bits;

  switch (sortType) {
    case nsMsgViewSortType::bySize:
        rv = msgHdr->GetMessageSize(result);
        break;
    case nsMsgViewSortType::byPriority: 
        // want highest priority to have lowest value
        // so ascending sort will have highest priority first.
        nsMsgPriorityValue priority;
        rv = msgHdr->GetPriority(&priority);
        *result = nsMsgPriority::highest - priority;
        break;
    case nsMsgViewSortType::byStatus:
        rv = msgHdr->GetStatusOffset(result);
        break;
    case nsMsgViewSortType::byFlagged:
        bits = 0;
        rv = msgHdr->GetFlags(&bits);
        *result = !(bits & MSG_FLAG_MARKED);  //make flagged come out on top.
        break;
    case nsMsgViewSortType::byUnread:
        rv = msgHdr->GetIsRead(&isRead);
        if (NS_SUCCEEDED(rv)) 
            *result = !isRead;
        break;
    case nsMsgViewSortType::byId:
        // handled by caller, since caller knows the key
    default:
        NS_ASSERTION(0,"should not be here");
        rv = NS_ERROR_UNEXPECTED;
        break;
    }
    
    NS_ENSURE_SUCCESS(rv,rv);
    return NS_OK;
}


nsresult 
nsMsgDBView::GetCollationKey(nsIMsgHdr *msgHdr, nsMsgViewSortTypeValue sortType, PRUint8 **result, PRUint32 *len)
{
  nsresult rv;
  NS_ENSURE_ARG_POINTER(msgHdr);
  NS_ENSURE_ARG_POINTER(result);

  switch (sortType) {
    case nsMsgViewSortType::bySubject:
        rv = msgHdr->GetSubjectCollationKey(result, len);
        break;
    case nsMsgViewSortType::byRecipient:
        rv = msgHdr->GetRecipientsCollationKey(result, len);
        break;
    case nsMsgViewSortType::byAuthor:
        rv = msgHdr->GetAuthorCollationKey(result, len);
        break;
    default:
        rv = NS_ERROR_UNEXPECTED;
        break;
    }

    NS_ENSURE_SUCCESS(rv,rv);
    return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::Sort(nsMsgViewSortTypeValue sortType, nsMsgViewSortOrderValue sortOrder)
{
    nsresult rv;


    // null db is OK.
    nsMsgKeyArray preservedSelection;

#ifdef DEBUG_seth
    printf("XXX nsMsgDBView::Sort(%d,%d)\n",(int)sortType,(int)sortOrder);
#endif
    if (m_sortType == sortType && m_sortValid) {
        if (m_sortOrder == sortOrder) {
            // same as it ever was.  do nothing
            return NS_OK;
        }   
        else {
            SaveSelection(&preservedSelection);
            if (m_sortType != nsMsgViewSortType::byThread) {
                rv = ReverseSort();
                NS_ENSURE_SUCCESS(rv,rv);
            }
            else {
                rv = ReverseThreads();
                NS_ENSURE_SUCCESS(rv,rv);
            }

            m_sortOrder = sortOrder;
            // we just reversed the sort order...we still need to invalidate the view
            RestoreSelection(&preservedSelection);
            mOutliner->Invalidate();
            return NS_OK;
        }
    }

    if (sortType == nsMsgViewSortType::byThread) {
        return NS_OK;
    }

    SaveSelection(&preservedSelection);

    // figure out how much memory we'll need, and the malloc it
    PRUint16 maxLen;
    eFieldType fieldType;

    rv = GetFieldTypeAndLenForSort(sortType, &maxLen, &fieldType);
    NS_ENSURE_SUCCESS(rv,rv);

    nsVoidArray ptrs;
    PRUint32 arraySize = GetSize();
    // use IdPRTime, it is the biggest
    IdPRTime** pPtrBase = (IdPRTime**)PR_Malloc(arraySize * sizeof(IdPRTime*));
    NS_ASSERTION(pPtrBase, "out of memory, can't sort");
    if (!pPtrBase) return NS_ERROR_OUT_OF_MEMORY;
    ptrs.AppendElement((void *)pPtrBase); // remember this pointer so we can free it later
    
    // build up the beast, so we can sort it.
    PRUint32 numSoFar = 0;
    // calc max possible size needed for all the rest
    PRUint32 maxSize = (PRUint32)(maxLen + sizeof(EntryInfo) + 1) * (PRUint32)(arraySize - numSoFar);

    PRUint32 maxBlockSize = (PRUint32) 0xf000L;
    PRUint32 allocSize = PR_MIN(maxBlockSize, maxSize);
    char *pTemp = (char *) PR_Malloc(allocSize);
    NS_ASSERTION(pTemp, "out of memory, can't sort");
    if (!pTemp) {   
        FreeAll(&ptrs);
        return NS_ERROR_OUT_OF_MEMORY;
    }

    ptrs.AppendElement(pTemp); // remember this pointer so we can free it later

    char *pBase = pTemp;
    PRBool more = PR_TRUE;

    nsCOMPtr <nsIMsgDBHdr> msgHdr;
    PRUint8 *keyValue = nsnull;
    PRUint32 longValue;
    PRTime timeValue;
    while (more && numSoFar < arraySize) {
      nsMsgKey thisKey = m_keys.GetAt(numSoFar);
      if (sortType != nsMsgViewSortType::byId) {
        rv = GetMsgHdrForViewIndex(numSoFar, getter_AddRefs(msgHdr));
        NS_ASSERTION(NS_SUCCEEDED(rv) && msgHdr, "header not found");
        if (NS_FAILED(rv) || !msgHdr) {
          FreeAll(&ptrs);
          return NS_ERROR_UNEXPECTED;
        }
      }
      else {
        msgHdr = nsnull;
      }

      // could be a problem here if the ones that appear here are different than the ones already in the array
      void *pField = nsnull;
      PRUint32 actualFieldLen = 0;
      if (fieldType == kCollationKey) {
        rv = GetCollationKey(msgHdr, sortType, &keyValue, &actualFieldLen);
        NS_ENSURE_SUCCESS(rv,rv);

        pField = (void *) keyValue;
      }
      else if (fieldType == kPRTime) {
        rv = GetPRTimeField(msgHdr, sortType, &timeValue);
        NS_ENSURE_SUCCESS(rv,rv);

        pField = (void *) &timeValue;
        actualFieldLen = maxLen;
      }
      else {
        if (sortType == nsMsgViewSortType::byId) {
            longValue = thisKey;
        }
        else {
            rv = GetLongField(msgHdr, sortType, &longValue);
            NS_ENSURE_SUCCESS(rv,rv);
        }
        pField = (void *)&longValue;
        actualFieldLen = maxLen;
      }

      // check to see if this entry fits into the block we have allocated so far
      // pTemp - pBase = the space we have used so far
      // sizeof(EntryInfo) + fieldLen = space we need for this entry
      // allocSize = size of the current block
      if ((PRUint32)(pTemp - pBase) + (PRUint32)sizeof(EntryInfo) + (PRUint32)actualFieldLen >= allocSize) {
        maxSize = (PRUint32)(maxLen + sizeof(EntryInfo) + 1) * (PRUint32)(arraySize - numSoFar);
        maxBlockSize = (PRUint32) 0xf000L;
        allocSize = PR_MIN(maxBlockSize, maxSize);
        pTemp = (char *) PR_Malloc(allocSize);
        NS_ASSERTION(pTemp, "out of memory, can't sort");
        if (!pTemp) {
          FreeAll(&ptrs);
          return NS_ERROR_OUT_OF_MEMORY;
        }
        pBase = pTemp;
        ptrs.AppendElement(pTemp); // remember this pointer so we can free it later
      }

      // make sure there aren't more IDs than we allocated space for
      NS_ASSERTION(numSoFar < arraySize, "out of memory");
      if (numSoFar >= arraySize) {
        FreeAll(&ptrs);
        return NS_ERROR_OUT_OF_MEMORY;
      }

      // now store this entry away in the allocated memory
      pPtrBase[numSoFar] = (IdPRTime*)pTemp;
      EntryInfo *info = (EntryInfo*)pTemp;
      info->id = thisKey;
      info->bits = m_flags.GetAt(numSoFar);
      info->len = actualFieldLen;
      //info->pad = 0;

      pTemp += sizeof(EntryInfo);

      PRInt32 bytesLeft = allocSize - (PRInt32)(pTemp - pBase);
      PRInt32 bytesToCopy = PR_MIN(bytesLeft, (PRInt32)actualFieldLen);
      if (pField && bytesToCopy > 0) {
        nsCRT::memcpy((void *)pTemp, pField, bytesToCopy);
        if (bytesToCopy < (PRInt32)actualFieldLen) {
          NS_ASSERTION(0, "wow, big block");
          info->len = bytesToCopy;
        }
      }
      else {
        *pTemp = 0;
      }
      pTemp += bytesToCopy;
      ++numSoFar;
      PR_FREEIF(keyValue);
    }

    // do the sort
    switch (fieldType) {
        case kCollationKey:
          {

            nsCOMPtr <nsIMsgDatabase> dbToUse = m_db;

            if (!dbToUse) // probably search view
              GetDBForViewIndex(0, getter_AddRefs(dbToUse));
            if (dbToUse)
              NS_QuickSort(pPtrBase, numSoFar, sizeof(IdKey*), FnSortIdKey, dbToUse);
          }
            break;
        case kU32:
            NS_QuickSort(pPtrBase, numSoFar, sizeof(IdDWord*), FnSortIdDWord, nsnull);
            break;
        case kPRTime:
            NS_QuickSort(pPtrBase, numSoFar, sizeof(IdPRTime*), FnSortIdPRTime, nsnull);
            break;
        default:
            NS_ASSERTION(0, "not supposed to get here");
            break;
    }

    // now put the IDs into the array in proper order
    for (PRUint32 i = 0; i < numSoFar; i++) {
        m_keys.SetAt(i, pPtrBase[i]->info.id);
        m_flags.SetAt(i, pPtrBase[i]->info.bits);
    }

    m_sortType = sortType;
    m_sortOrder = sortOrder;

    if (sortOrder == nsMsgViewSortOrder::descending) {
        rv = ReverseSort();
        NS_ASSERTION(NS_SUCCEEDED(rv),"failed to reverse sort");
    }

    // free all the memory we allocated
    FreeAll(&ptrs);

    m_sortValid = PR_TRUE;
    //m_db->SetSortInfo(sortType, sortOrder);

    // last but not least, invalidate the entire view and restore
    // the selection.
    RestoreSelection(&preservedSelection);

    if (mOutliner)
      mOutliner->Invalidate();

    return NS_OK;
}

void nsMsgDBView::FreeAll(nsVoidArray *ptrs)
{
    PRInt32 i;
    PRInt32 count = (PRInt32) ptrs->Count();
    if (count == 0) return;

    for (i=(count - 1);i>=0;i--) {
        void *ptr = (void *) ptrs->ElementAt(i);
        PR_FREEIF(ptr);
        ptrs->RemoveElementAt(i);
    }
}

nsMsgViewIndex nsMsgDBView::GetIndexOfFirstDisplayedKeyInThread(nsIMsgThread *threadHdr)
{
	nsMsgViewIndex	retIndex = nsMsgViewIndex_None;
	PRUint32	childIndex = 0;
	// We could speed up the unreadOnly view by starting our search with the first
	// unread message in the thread. Sometimes, that will be wrong, however, so
	// let's skip it until we're sure it's neccessary.
//	(m_viewFlags & nsMsgViewFlagsType::kUnreadOnly) 
//		? threadHdr->GetFirstUnreadKey(m_db) : threadHdr->GetChildAt(0);
  PRUint32 numThreadChildren;
  threadHdr->GetNumChildren(&numThreadChildren);
	while (retIndex == nsMsgViewIndex_None && childIndex < numThreadChildren)
	{
		nsMsgKey childKey;
    threadHdr->GetChildKeyAt(childIndex++, &childKey);
		retIndex = FindViewIndex(childKey);
	}
	return retIndex;
}

// caller must referTo hdr if they want to hold it or change it!
nsresult nsMsgDBView::GetFirstMessageHdrToDisplayInThread(nsIMsgThread *threadHdr, nsIMsgDBHdr **result)
{
  nsresult rv;

	if (m_viewFlags & nsMsgViewFlagsType::kUnreadOnly)
		rv = threadHdr->GetFirstUnreadChild(result);
	else
		rv = threadHdr->GetChildHdrAt(0, result);
	return rv;
}

// Find the view index of the thread containing the passed msgKey, if
// the thread is in the view. MsgIndex is passed in as a shortcut if
// it turns out the msgKey is the first message in the thread,
// then we can avoid looking for the msgKey.
nsMsgViewIndex nsMsgDBView::ThreadIndexOfMsg(nsMsgKey msgKey, 
											  nsMsgViewIndex msgIndex /* = nsMsgViewIndex_None */,
											  PRInt32 *pThreadCount /* = NULL */,
											  PRUint32 *pFlags /* = NULL */)
{
	if (m_sortType != nsMsgViewSortType::byThread)
		return nsMsgViewIndex_None;
	nsCOMPtr <nsIMsgThread> threadHdr;
  nsCOMPtr <nsIMsgDBHdr> msgHdr;
  nsresult rv = m_db->GetMsgHdrForKey(msgKey, getter_AddRefs(msgHdr));
  NS_ENSURE_SUCCESS(rv, nsMsgViewIndex_None);
  rv = m_db->GetThreadContainingMsgHdr(msgHdr, getter_AddRefs(threadHdr));
  NS_ENSURE_SUCCESS(rv, nsMsgViewIndex_None);

	nsMsgViewIndex retIndex = nsMsgViewIndex_None;

	if (threadHdr != nsnull)
	{
		if (msgIndex == nsMsgViewIndex_None)
			msgIndex = FindViewIndex(msgKey);

		if (msgIndex == nsMsgViewIndex_None)	// key is not in view, need to find by thread
		{
			msgIndex = GetIndexOfFirstDisplayedKeyInThread(threadHdr);
			//nsMsgKey		threadKey = (msgIndex == nsMsgViewIndex_None) ? nsMsgKey_None : GetAt(msgIndex);
			if (pFlags)
				threadHdr->GetFlags(pFlags);
		}
		nsMsgViewIndex startOfThread = msgIndex;
		while ((PRInt32) startOfThread >= 0 && m_levels[startOfThread] != 0)
			startOfThread--;
		retIndex = startOfThread;
		if (pThreadCount)
		{
			PRInt32 numChildren = 0;
			nsMsgViewIndex threadIndex = startOfThread;
			do
			{
				threadIndex++;
				numChildren++;
			}
			while ((int32) threadIndex < m_levels.GetSize() && m_levels[threadIndex] != 0);
			*pThreadCount = numChildren;
		}
	}
	return retIndex;
}

nsMsgKey nsMsgDBView::GetKeyOfFirstMsgInThread(nsMsgKey key)
{
	nsCOMPtr <nsIMsgThread> pThread;
	nsCOMPtr <nsIMsgDBHdr> msgHdr;
    nsresult rv = m_db->GetMsgHdrForKey(key, getter_AddRefs(msgHdr));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = m_db->GetThreadContainingMsgHdr(msgHdr, getter_AddRefs(pThread));
    NS_ENSURE_SUCCESS(rv, rv);
	nsMsgKey	firstKeyInThread = nsMsgKey_None;

	NS_ASSERTION(pThread, "error getting msg from thread");
	if (!pThread)
	{
		return firstKeyInThread;
	}
	// ### dmb UnreadOnly - this is wrong. But didn't seem to matter in 4.x
	pThread->GetChildKeyAt(0, &firstKeyInThread);
	return firstKeyInThread;
}

NS_IMETHODIMP nsMsgDBView::GetKeyAt(nsMsgViewIndex index, nsMsgKey *result)
{
  NS_ENSURE_ARG(result);
  *result = GetAt(index);
  return NS_OK;
}

nsMsgKey nsMsgDBView::GetAt(nsMsgViewIndex index) 
{
	if (index >= m_keys.GetSize() || index == nsMsgViewIndex_None)
		return nsMsgKey_None;
	else
		return(m_keys.GetAt(index));
}

nsMsgViewIndex	nsMsgDBView::FindKey(nsMsgKey key, PRBool expand)
{
	nsMsgViewIndex retIndex = nsMsgViewIndex_None;
	retIndex = (nsMsgViewIndex) (m_keys.FindIndex(key));
	if (key != nsMsgKey_None && retIndex == nsMsgViewIndex_None && expand && m_db)
	{
		nsMsgKey threadKey = GetKeyOfFirstMsgInThread(key);
		if (threadKey != nsMsgKey_None)
		{
			nsMsgViewIndex threadIndex = FindKey(threadKey, PR_FALSE);
			if (threadIndex != nsMsgViewIndex_None)
			{
				PRUint32 flags = m_flags[threadIndex];
				if ((flags & MSG_FLAG_ELIDED) && NS_SUCCEEDED(ExpandByIndex(threadIndex, nsnull)))
					retIndex = FindKey(key, PR_FALSE);
			}
		}
	}
	return retIndex;
}

nsresult nsMsgDBView::GetThreadCount(nsMsgKey messageKey, PRUint32 *pThreadCount)
{
	nsresult rv = NS_MSG_MESSAGE_NOT_FOUND;
	nsCOMPtr <nsIMsgDBHdr> msgHdr;
    rv = m_db->GetMsgHdrForKey(messageKey, getter_AddRefs(msgHdr));
    NS_ENSURE_SUCCESS(rv, rv);
	nsCOMPtr <nsIMsgThread> pThread;
    rv = m_db->GetThreadContainingMsgHdr(msgHdr, getter_AddRefs(pThread));
	if (NS_SUCCEEDED(rv) && pThread != nsnull)
    rv = pThread->GetNumChildren(pThreadCount);
	return rv;
}


// This counts the number of messages in an expanded thread, given the
// index of the first message in the thread.
PRInt32 nsMsgDBView::CountExpandedThread(nsMsgViewIndex index)
{
	PRInt32 numInThread = 0;
	nsMsgViewIndex startOfThread = index;
	while ((PRInt32) startOfThread >= 0 && m_levels[startOfThread] != 0)
		startOfThread--;
	nsMsgViewIndex threadIndex = startOfThread;
	do
	{
		threadIndex++;
		numInThread++;
	}
	while ((PRInt32) threadIndex < m_levels.GetSize() && m_levels[threadIndex] != 0);

	return numInThread;
}

// returns the number of lines that would be added (> 0) or removed (< 0) 
// if we were to try to expand/collapse the passed index.
nsresult nsMsgDBView::ExpansionDelta(nsMsgViewIndex index, PRInt32 *expansionDelta)
{
	PRUint32 numChildren;
	nsresult	rv;

	*expansionDelta = 0;
	if ( index > ((nsMsgViewIndex) m_keys.GetSize()))
		return NS_MSG_MESSAGE_NOT_FOUND;
	char	flags = m_flags[index];

	if (m_sortType != nsMsgViewSortType::byThread)
		return NS_OK;

	// The client can pass in the key of any message
	// in a thread and get the expansion delta for the thread.

	if (!(m_viewFlags & nsMsgViewFlagsType::kUnreadOnly))
	{
		rv = GetThreadCount(m_keys[index], &numChildren);
		NS_ENSURE_SUCCESS(rv, rv);
	}
	else
	{
		numChildren = CountExpandedThread(index);
	}

	if (flags & MSG_FLAG_ELIDED)
		*expansionDelta = numChildren - 1;
	else
		*expansionDelta = - (PRInt32) (numChildren - 1);

	return NS_OK;
}

nsresult nsMsgDBView::ToggleExpansion(nsMsgViewIndex index, PRUint32 *numChanged)
{
  NS_ENSURE_ARG(numChanged);
  *numChanged = 0;
	nsMsgViewIndex threadIndex = ThreadIndexOfMsg(GetAt(index), index);
	if (threadIndex == nsMsgViewIndex_None)
	{
		NS_ASSERTION(PR_FALSE, "couldn't find thread");
		return NS_MSG_MESSAGE_NOT_FOUND;
	}
	PRInt32	flags = m_flags[threadIndex];

	// if not a thread, or doesn't have children, no expand/collapse
	// If we add sub-thread expand collapse, this will need to be relaxed
	if (!(flags & MSG_VIEW_FLAG_ISTHREAD) || !(flags && MSG_VIEW_FLAG_HASCHILDREN))
		return NS_MSG_MESSAGE_NOT_FOUND;
	if (flags & MSG_FLAG_ELIDED)
		return ExpandByIndex(threadIndex, numChanged);
	else
		return CollapseByIndex(threadIndex, numChanged);

}

nsresult nsMsgDBView::ExpandAll()
{
	for (PRInt32 i = GetSize() - 1; i >= 0; i--) 
	{
		PRUint32 numExpanded;
		PRUint32 flags = m_flags[i];
		if (flags & MSG_FLAG_ELIDED)
			ExpandByIndex(i, &numExpanded);
	}
	return NS_OK;
}

nsresult nsMsgDBView::ExpandByIndex(nsMsgViewIndex index, PRUint32 *pNumExpanded)
{
	PRUint32			flags = m_flags[index];
	nsMsgKey		firstIdInThread;
    //nsMsgKey        startMsg = nsMsgKey_None;
	nsresult		rv = NS_OK;
	PRUint32			numExpanded = 0;

	NS_ASSERTION(flags & MSG_FLAG_ELIDED, "can't expand an already expanded thread");
	flags &= ~MSG_FLAG_ELIDED;

	if ((PRUint32) index > m_keys.GetSize())
		return NS_MSG_MESSAGE_NOT_FOUND;

	firstIdInThread = m_keys[index];
	nsCOMPtr <nsIMsgDBHdr> msgHdr;
  nsCOMPtr <nsIMsgThread> pThread;
    m_db->GetMsgHdrForKey(firstIdInThread, getter_AddRefs(msgHdr));
	if (msgHdr == nsnull)
	{
		NS_ASSERTION(PR_FALSE, "couldn't find message to expand");
		return NS_MSG_MESSAGE_NOT_FOUND;
	}
  rv = m_db->GetThreadContainingMsgHdr(msgHdr, getter_AddRefs(pThread));
	m_flags[index] = flags;
  NoteChange(index, 1, nsMsgViewNotificationCode::changed);
	if (m_viewFlags & nsMsgViewFlagsType::kUnreadOnly)
	{
		if (flags & MSG_FLAG_READ)
			m_levels.Add(0);	// keep top level hdr in thread, even though read.
		rv = ListUnreadIdsInThread(pThread,  index, &numExpanded);
	}
	else
		rv = ListIdsInThread(pThread,  index, &numExpanded);

	NoteStartChange(index + 1, numExpanded, nsMsgViewNotificationCode::insertOrDelete);

  NoteEndChange(index + 1, numExpanded, nsMsgViewNotificationCode::insertOrDelete);
	if (pNumExpanded != nsnull)
		*pNumExpanded = numExpanded;
	return rv;
}

nsresult nsMsgDBView::CollapseAll()
{
    for (PRInt32 i = 0; i < GetSize(); i++)
    {
        PRUint32 numExpanded;
        PRUint32 flags = m_flags[i];
        if (!(flags & MSG_FLAG_ELIDED) && (flags & MSG_VIEW_FLAG_HASCHILDREN))
            CollapseByIndex(i, &numExpanded);
    }
    return NS_OK;
}

nsresult nsMsgDBView::CollapseByIndex(nsMsgViewIndex index, PRUint32 *pNumCollapsed)
{
	nsMsgKey		firstIdInThread;
	nsresult	rv;
	PRInt32	flags = m_flags[index];
	PRInt32	threadCount = 0;

	if (flags & MSG_FLAG_ELIDED || m_sortType != nsMsgViewSortType::byThread)
		return NS_OK;
	flags  |= MSG_FLAG_ELIDED;

	if (index > m_keys.GetSize())
		return NS_MSG_MESSAGE_NOT_FOUND;

	firstIdInThread = m_keys[index];
	nsCOMPtr <nsIMsgDBHdr> msgHdr;
  rv = m_db->GetMsgHdrForKey(firstIdInThread, getter_AddRefs(msgHdr));
	if (!NS_SUCCEEDED(rv) || msgHdr == nsnull)
	{
		NS_ASSERTION(PR_FALSE, "error collapsing thread");
		return NS_MSG_MESSAGE_NOT_FOUND;
	}

	m_flags[index] = flags;
	NoteChange(index, 1, nsMsgViewNotificationCode::changed);

	rv = ExpansionDelta(index, &threadCount);
	if (NS_SUCCEEDED(rv))
	{
		PRInt32 numRemoved = threadCount; // don't count first header in thread
    NoteStartChange(index + 1, -numRemoved, nsMsgViewNotificationCode::insertOrDelete);
		// start at first id after thread.
		for (int i = 1; i <= threadCount && index + 1 < m_keys.GetSize(); i++)
		{
			m_keys.RemoveAt(index + 1);
			m_flags.RemoveAt(index + 1);
			m_levels.RemoveAt(index + 1);
		}
		if (pNumCollapsed != nsnull)
			*pNumCollapsed = numRemoved;	
		NoteEndChange(index + 1, -numRemoved, nsMsgViewNotificationCode::insertOrDelete);
	}
	return rv;
}

nsresult nsMsgDBView::OnNewHeader(nsMsgKey newKey, nsMsgKey aParentKey, PRBool /*ensureListed*/)
{
	nsresult rv = NS_MSG_MESSAGE_NOT_FOUND;;
	// views can override this behaviour, which is to append to view.
	// This is the mail behaviour, but threaded views will want
	// to insert in order...
	nsCOMPtr <nsIMsgDBHdr> msgHdr;
  rv = m_db->GetMsgHdrForKey(newKey, getter_AddRefs(msgHdr));
	if (NS_SUCCEEDED(rv) && msgHdr != nsnull)
	{
		rv = AddHdr(msgHdr);
	}
	return rv;
}

nsresult nsMsgDBView::GetThreadContainingIndex(nsMsgViewIndex index, nsIMsgThread **resultThread)
{
  nsCOMPtr <nsIMsgDBHdr> msgHdr;

  nsresult rv = m_db->GetMsgHdrForKey(m_keys[index], getter_AddRefs(msgHdr));
  NS_ENSURE_SUCCESS(rv, rv);
  return m_db->GetThreadContainingMsgHdr(msgHdr, resultThread);
}

nsMsgViewIndex nsMsgDBView::GetIndexForThread(nsIMsgDBHdr *hdr)
{
	nsMsgViewIndex retIndex = nsMsgViewIndex_None;
	nsMsgViewIndex prevInsertIndex = nsMsgViewIndex_None;
	nsMsgKey insertKey;
  hdr->GetMessageKey(&insertKey);

  if (m_sortOrder == nsMsgViewSortOrder::ascending)
	{
		// loop backwards looking for top level message with id > id of header we're inserting 
		// and put new header before found header, or at end.
		for (PRInt32 i = GetSize() - 1; i >= 0; i--) 
		{
			char level = m_levels[i];
			if (level == 0)
			{
				if (insertKey < m_keys.GetAt(i))
					prevInsertIndex = i;
				else if (insertKey >= m_keys.GetAt(i))
				{
					retIndex = (prevInsertIndex == nsMsgViewIndex_None) ? nsMsgViewIndex_None : i + 1;
					if (prevInsertIndex == nsMsgViewIndex_None)
					{
						retIndex = nsMsgViewIndex_None;
					}
					else
					{
						for (retIndex = i + 1; retIndex < (nsMsgViewIndex)GetSize(); retIndex++)
						{
							if (m_levels[retIndex] == 0)
								break;
						}
					}
					break;
				}

			}
		}
	}
	else
	{
		// loop forwards looking for top level message with id < id of header we're inserting and put 
		// new header before found header, or at beginning.
		for (PRInt32 i = 0; i < GetSize(); i++) 
		{
			char level = m_levels[i];
			if (level == 0)
			{
				if (insertKey > m_keys.GetAt(i))
				{
					retIndex = i;
					break;
				}
			}
		}
	}
	return retIndex;
}


nsMsgViewIndex nsMsgDBView::GetInsertIndex(nsIMsgDBHdr *msgHdr)
{
	PRBool done = PR_FALSE;
	PRBool withinOne = PR_FALSE;
	nsMsgViewIndex retIndex = nsMsgViewIndex_None;
	nsMsgViewIndex tryIndex = GetSize() / 2;
	nsMsgViewIndex newTryIndex;
	nsMsgViewIndex lowIndex = 0;
	nsMsgViewIndex highIndex = GetSize() - 1;
	IdDWord	dWordEntryInfo1, dWordEntryInfo2;
	IdKeyPtr	keyInfo1, keyInfo2;
  IdPRTime timeInfo1, timeInfo2;
  void *comparisonContext = nsnull;

  nsresult rv;

	if (GetSize() == 0)
		return 0;

	PRUint16	maxLen;
	eFieldType fieldType;
  rv = GetFieldTypeAndLenForSort(m_sortType, &maxLen, &fieldType);
	const void *pValue1, *pValue2;

	if ((m_viewFlags & nsMsgViewFlagsType::kThreadedDisplay) != 0)
	{
		retIndex = GetIndexForThread(msgHdr);
		return retIndex;
	}

	int (*comparisonFun) (const void *pItem1, const void *pItem2, void *privateData)=nsnull;
	int retStatus = 0;
	switch (fieldType)
	{
		case kCollationKey:
      rv = GetCollationKey(msgHdr, m_sortType, &(keyInfo1.key), &(keyInfo1.info.len));
      NS_ASSERTION(NS_SUCCEEDED(rv),"failed to create collation key");
			msgHdr->GetMessageKey(&keyInfo1.info.id);
      comparisonFun = FnSortIdKeyPtr;
      comparisonContext = m_db.get();
			pValue1 = (void *) &keyInfo1;
			break;
		case kU32:
			GetLongField(msgHdr, m_sortType, &dWordEntryInfo1.dword);
			msgHdr->GetMessageKey(&dWordEntryInfo1.info.id);
			comparisonFun = FnSortIdDWord;
			pValue1 = (void *) &dWordEntryInfo1;
			break;
    case kPRTime:
      rv = GetPRTimeField(msgHdr, m_sortType, &timeInfo1.prtime);
			msgHdr->GetMessageKey(&timeInfo1.info.id);
      NS_ENSURE_SUCCESS(rv,rv);
      comparisonFun = FnSortIdPRTime;
      pValue1 = (void *) &timeInfo1;
      break;
		default:
			done = PR_TRUE;
	}
	while (!done)
	{
		if (highIndex == lowIndex)
			break;
		nsMsgKey	messageKey = GetAt(tryIndex);
		nsCOMPtr <nsIMsgDBHdr> tryHdr;
    rv = m_db->GetMsgHdrForKey(messageKey, getter_AddRefs(tryHdr));
		if (!tryHdr)
			break;
		if (fieldType == kCollationKey)
		{
			rv = GetCollationKey(tryHdr, m_sortType, &(keyInfo2.key), &(keyInfo2.info.len));
      NS_ASSERTION(NS_SUCCEEDED(rv),"failed to create collation key");
			keyInfo2.info.id = messageKey;
      pValue2 = &keyInfo2;
		}
		else if (fieldType == kU32)
		{
			GetLongField(tryHdr, m_sortType, &dWordEntryInfo2.dword);
			dWordEntryInfo2.info.id = messageKey;
			pValue2 = &dWordEntryInfo2;
		}
    else if (fieldType == kPRTime)
    {
			GetPRTimeField(tryHdr, m_sortType, &timeInfo2.prtime);
			timeInfo2.info.id = messageKey;
			pValue2 = &timeInfo2;
    }
		retStatus = (*comparisonFun)(&pValue1, &pValue2, comparisonContext);
		if (retStatus == 0)
			break;
    if (m_sortOrder == nsMsgViewSortOrder::descending)	//switch retStatus based on sort order
			retStatus = (retStatus > 0) ? -1 : 1;

		if (retStatus < 0)
		{
			newTryIndex = tryIndex  - (tryIndex - lowIndex) / 2;
			if (newTryIndex == tryIndex)
			{
				if (!withinOne && newTryIndex > lowIndex)
				{
					newTryIndex--;
					withinOne = PR_TRUE;
				}
			}
			highIndex = tryIndex;
		}
		else
		{
			newTryIndex = tryIndex + (highIndex - tryIndex) / 2;
			if (newTryIndex == tryIndex)
			{
				if (!withinOne && newTryIndex < highIndex)
				{
					withinOne = PR_TRUE;
					newTryIndex++;
				}
				lowIndex = tryIndex;
			}
		}
		if (tryIndex == newTryIndex)
			break;
		else
			tryIndex = newTryIndex;
	}
	if (retStatus >= 0)
		retIndex = tryIndex + 1;
	else if (retStatus < 0)
		retIndex = tryIndex;

	return retIndex;
}

nsresult	nsMsgDBView::AddHdr(nsIMsgDBHdr *msgHdr)
{
	PRUint32	flags = 0;
#ifdef DEBUG_bienvenu
	NS_ASSERTION((int) m_keys.GetSize() == m_flags.GetSize() && (int) m_keys.GetSize() == m_levels.GetSize(), "view arrays out of sync!");
#endif
  msgHdr->GetFlags(&flags);
	if (flags & MSG_FLAG_IGNORED && !GetShowingIgnored())
		return NS_OK;

  nsMsgKey msgKey, threadId;
  nsMsgKey threadParent;
  msgHdr->GetMessageKey(&msgKey);
  msgHdr->GetThreadId(&threadId);
  msgHdr->GetThreadParent(&threadParent);

  // ### this isn't quite right, is it? Should be checking that our thread parent key is none?
	if (threadParent == nsMsgKey_None) 
		flags |= MSG_VIEW_FLAG_ISTHREAD;
	nsMsgViewIndex insertIndex = GetInsertIndex(msgHdr);
	if (insertIndex == nsMsgViewIndex_None)
	{
		// if unreadonly, level is 0 because we must be the only msg in the thread.
    char levelToAdd = 0; // ### TODO ((m_viewFlags & nsMsgViewFlagsType::kUnreadOnly) != 0) ? 0 : msgHdr->GetLevel();

    if (m_sortOrder == nsMsgViewSortOrder::ascending)
		{
			m_keys.Add(msgKey);
			m_flags.Add(flags);
			m_levels.Add(levelToAdd);
			NoteChange(m_keys.GetSize() - 1, 1, nsMsgViewNotificationCode::insertOrDelete);
		}
		else
		{
			m_keys.InsertAt(0, msgKey);
			m_flags.InsertAt(0, flags);
			m_levels.InsertAt(0, levelToAdd);
			NoteChange(0, 1, nsMsgViewNotificationCode::insertOrDelete);
		}
		m_sortValid = PR_FALSE;
	}
	else
	{
		m_keys.InsertAt(insertIndex, msgKey);
		m_flags.InsertAt(insertIndex, flags);
    char level = 0; // ### TODO (m_sortType == nsMsgViewSortType::byThread) ? 0 : msgHdr->GetLevel();
		m_levels.InsertAt(insertIndex, level);
    NoteChange(insertIndex, 1, nsMsgViewNotificationCode::insertOrDelete);
	}
	OnHeaderAddedOrDeleted();
	return NS_OK;
}

nsresult nsMsgDBView::InsertHdrAt(nsIMsgDBHdr *msgHdr, nsMsgViewIndex insertIndex)
{
	PRUint32	flags = 0;
  nsMsgKey msgKey;
	msgHdr->GetFlags(&flags);
  msgHdr->GetMessageKey(&msgKey);

	NoteStartChange(insertIndex, 1, nsMsgViewNotificationCode::changed);
	m_keys.SetAt(insertIndex, msgKey);
	m_flags.SetAt(insertIndex, flags);
  char level = 0; // ### TODO (m_sortType == nsMsgViewSortType::byThread) ? 0 : msgHdr->GetLevel()
	m_levels.SetAt(insertIndex, level);
	NoteEndChange(insertIndex, 1, nsMsgViewNotificationCode::changed);
	OnHeaderAddedOrDeleted();
	return NS_OK;
}


PRBool nsMsgDBView::WantsThisThread(nsIMsgThread * /*threadHdr*/)
{
  return PR_TRUE; // default is to want all threads.
}

PRInt32 nsMsgDBView::FindLevelInThread(nsIMsgDBHdr *msgHdr, nsMsgKey msgKey, nsMsgViewIndex startOfThreadViewIndex)
{
  nsMsgKey threadParent;
  msgHdr->GetThreadParent(&threadParent);
  nsMsgViewIndex parentIndex = m_keys.FindIndex(threadParent, startOfThreadViewIndex);
  if (parentIndex != nsMsgViewIndex_None)
    return m_levels[parentIndex] + 1;
  else
  {
#ifdef DEBUG_bienvenu
    NS_ASSERTION(PR_FALSE, "couldn't find parent of msg");
#endif
    return 1; // well, return level 1.
  }
}

nsresult	nsMsgDBView::ListIdsInThread(nsIMsgThread *threadHdr, nsMsgViewIndex startOfThreadViewIndex, PRUint32 *pNumListed)
{
  NS_ENSURE_ARG(threadHdr);
	// these children ids should be in thread order.
	PRUint32 i;
  nsMsgViewIndex viewIndex = startOfThreadViewIndex + 1;
	*pNumListed = 0;

  PRUint32 numChildren;
  threadHdr->GetNumChildren(&numChildren);
	for (i = 1; i < numChildren; i++)
	{
		nsCOMPtr <nsIMsgDBHdr> msgHdr;
    threadHdr->GetChildHdrAt(i, getter_AddRefs(msgHdr));
		if (msgHdr != nsnull)
		{
      nsMsgKey msgKey;
      PRUint32 msgFlags, newFlags;
      msgHdr->GetMessageKey(&msgKey);
      msgHdr->GetFlags(&msgFlags);
			PRBool isRead = PR_FALSE;
			m_db->IsRead(msgKey, &isRead);
			// just make sure flag is right in db.
			m_db->MarkHdrRead(msgHdr, isRead, nsnull);
//				if (isRead)
//					msgHdr->m_flags |= MSG_FLAG_READ;
//				else
//					msgHdr->m_flags &= ~MSG_FLAG_READ;
			m_keys.InsertAt(viewIndex, msgKey);
      // ### TODO - how about hasChildren flag?
			m_flags.InsertAt(viewIndex, msgFlags & ~MSG_VIEW_FLAGS);
      // ### TODO this is going to be tricky - might use enumerators
      PRInt32 level = FindLevelInThread(msgHdr, msgKey, startOfThreadViewIndex);
			m_levels.InsertAt(viewIndex, level); 
			// turn off thread or elided bit if they got turned on (maybe from new only view?)
			if (i > 0)	
				msgHdr->AndFlags(~(MSG_VIEW_FLAG_ISTHREAD | MSG_FLAG_ELIDED), &newFlags);
			(*pNumListed)++;
      viewIndex++;
		}
	}
  return NS_OK;
}

nsresult	nsMsgDBView::ListUnreadIdsInThread(nsIMsgThread *threadHdr, nsMsgViewIndex startOfThreadViewIndex, PRUint32 *pNumListed)
{
  NS_ENSURE_ARG(threadHdr);
	// these children ids should be in thread order.
  nsMsgViewIndex viewIndex = startOfThreadViewIndex + 1;
	*pNumListed = 0;
  nsUint8Array levelStack;


  PRUint32 numChildren;
  threadHdr->GetNumChildren(&numChildren);
	PRUint32 i;
	for (i = 0; i < numChildren; i++)
	{
		nsCOMPtr <nsIMsgDBHdr> msgHdr;
    threadHdr->GetChildHdrAt(i, getter_AddRefs(msgHdr));
		if (msgHdr != nsnull)
		{
      nsMsgKey msgKey;
      PRUint32 msgFlags;
      msgHdr->GetMessageKey(&msgKey);
      msgHdr->GetFlags(&msgFlags);
			PRBool isRead = PR_FALSE;
			m_db->IsRead(msgKey, &isRead);
			// just make sure flag is right in db.
			m_db->MarkHdrRead(msgHdr, isRead, nsnull);
      // determining the level is going to be tricky, since we're not storing the
      // level in the db anymore. It will mean looking up the view for each of the
      // ancestors of the current msg until we find one in the view. I guess we could
      // check each ancestor to see if it's unread before looking for it in the view.
#if 0
			// if the current header's level is <= to the top of the level stack,
			// pop off the top of the stack.
			while (levelStack.GetSize() > 1 && 
						msgHdr->GetLevel()  <= levelStack.GetAt(levelStack.GetSize() - 1))
			{
				levelStack.RemoveAt(levelStack.GetSize() - 1);
			}
#endif
			if (!isRead)
			{
				PRUint8 levelToAdd;
				// just make sure flag is right in db.
				m_db->MarkHdrRead(msgHdr, PR_FALSE, nsnull);
				m_keys.InsertAt(viewIndex, msgKey);
        m_flags.InsertAt(viewIndex, msgFlags);
//					pLevels[i] = msgHdr->GetLevel();
				if (levelStack.GetSize() == 0)
					levelToAdd = 0;
				else
					levelToAdd = levelStack.GetAt(levelStack.GetSize() - 1) + 1;
        m_levels.InsertAt(viewIndex, levelToAdd);
#ifdef DEBUG_bienvenu
//					XP_Trace("added at level %d\n", levelToAdd);
#endif
				levelStack.Add(levelToAdd);
        viewIndex++;
				(*pNumListed)++;
			}
		}
	}
	return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::OnKeyChange(nsMsgKey aKeyChanged, PRUint32 aOldFlags, 
                                       PRUint32 aNewFlags, nsIDBChangeListener *aInstigator)
{
  // if we're not the instigator, update flags if this key is in our view
  if (aInstigator != this)
  {
    nsMsgViewIndex index = FindViewIndex(aKeyChanged);
    if (index != nsMsgViewIndex_None)
    {
      PRUint32 viewOnlyFlags = m_flags[index] & (MSG_VIEW_FLAGS | MSG_FLAG_ELIDED);

      // ### what about saving the old view only flags, like IsThread and HasChildren?
      // I think we'll want to save those away.
      m_flags[index] = aNewFlags | viewOnlyFlags;
      // tell the view the extra flag changed, so it can
      // update the previous view, if any.
      OnExtraFlagChanged(index, aNewFlags);
      NoteChange(index, 1, nsMsgViewNotificationCode::changed);
    }
    else
    {
      nsMsgViewIndex threadIndex = ThreadIndexOfMsg(aKeyChanged);
      // may need to fix thread counts
      if (threadIndex != nsMsgViewIndex_None)
        NoteChange(threadIndex, 1, nsMsgViewNotificationCode::changed);

    }
  }
  // don't need to propagate notifications, right?
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::OnKeyDeleted(nsMsgKey aKeyChanged, nsMsgKey aParentKey, PRInt32 aFlags, 
                            nsIDBChangeListener *aInstigator)
{
  if (mOutliner)
  {
    nsMsgViewIndex deletedIndex = m_keys.FindIndex(aKeyChanged);
    if (deletedIndex != nsMsgViewIndex_None)
      RemoveByIndex(deletedIndex);
  }
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::OnKeyAdded(nsMsgKey aKeyChanged, nsMsgKey aParentKey, PRInt32 aFlags, 
                          nsIDBChangeListener *aInstigator)
{
  return OnNewHeader(aKeyChanged, aParentKey, PR_FALSE); 
  // probably also want to pass that parent key in, since we went to the trouble
  // of figuring out what it is.
}
                          
NS_IMETHODIMP nsMsgDBView::OnParentChanged (nsMsgKey aKeyChanged, nsMsgKey oldParent, nsMsgKey newParent, nsIDBChangeListener *aInstigator)
{
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::OnAnnouncerGoingAway(nsIDBChangeAnnouncer *instigator)
{
  ClearHdrCache();
  if (m_db)
  {
    m_db->RemoveListener(this);
    m_db = nsnull;
  }
  return NS_OK;
}

    
NS_IMETHODIMP nsMsgDBView::OnReadChanged(nsIDBChangeListener *instigator)
{
  return NS_OK;
}

void nsMsgDBView::ClearHdrCache()
{
  m_cachedHdr = nsnull;
  m_cachedMsgKey = nsMsgKey_None;
}

void	nsMsgDBView::EnableChangeUpdates()
{
}
void	nsMsgDBView::DisableChangeUpdates()
{
}
void	nsMsgDBView::NoteChange(nsMsgViewIndex firstLineChanged, PRInt32 numChanged, 
							 nsMsgViewNotificationCodeValue changeType)
{
  if (mOutliner)
  {
    switch (changeType)
    {
    case nsMsgViewNotificationCode::changed:
      mOutliner->InvalidateRange(firstLineChanged, firstLineChanged + numChanged - 1);
      break;
    case nsMsgViewNotificationCode::insertOrDelete:
      mOutliner->RowCountChanged(firstLineChanged, numChanged);
    case nsMsgViewNotificationCode::all:
      ClearHdrCache();
      break;
    }
  }
}

void	nsMsgDBView::NoteStartChange(nsMsgViewIndex firstlineChanged, PRInt32 numChanged, 
							 nsMsgViewNotificationCodeValue changeType)
{
}
void	nsMsgDBView::NoteEndChange(nsMsgViewIndex firstlineChanged, PRInt32 numChanged, 
							 nsMsgViewNotificationCodeValue changeType)
{
  // send the notification now.
  NoteChange(firstlineChanged, numChanged, changeType);
}

NS_IMETHODIMP nsMsgDBView::GetSortOrder(nsMsgViewSortOrderValue *aSortOrder)
{
    NS_ENSURE_ARG_POINTER(aSortOrder);
    *aSortOrder = m_sortOrder;
    return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::GetSortType(nsMsgViewSortTypeValue *aSortType)
{
    NS_ENSURE_ARG_POINTER(aSortType);
    *aSortType = m_sortType;
    return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::GetViewType(nsMsgViewTypeValue *aViewType)
{
    NS_ASSERTION(0,"you should be overriding this\n");
    return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP nsMsgDBView::GetViewFlags(nsMsgViewFlagsTypeValue *aViewFlags)
{
    NS_ENSURE_ARG_POINTER(aViewFlags);
    *aViewFlags = m_viewFlags;
    return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::SetViewFlags(nsMsgViewFlagsTypeValue aViewFlags)
{
    m_viewFlags = aViewFlags;
    return NS_OK;
}

nsresult nsMsgDBView::MarkThreadOfMsgRead(nsMsgKey msgId, nsMsgViewIndex msgIndex, nsMsgKeyArray &idsMarkedRead, PRBool bRead)
{
    nsCOMPtr <nsIMsgThread> threadHdr;
    nsresult rv = GetThreadContainingIndex(msgIndex, getter_AddRefs(threadHdr));
    NS_ENSURE_SUCCESS(rv, rv);

    nsMsgViewIndex threadIndex;

    NS_ASSERTION(threadHdr, "threadHdr is null");
    if (!threadHdr) {
        return NS_MSG_MESSAGE_NOT_FOUND;
    }
    nsCOMPtr <nsIMsgDBHdr> firstHdr;
    threadHdr->GetChildAt(0, getter_AddRefs(firstHdr));
    nsMsgKey firstHdrId;
    firstHdr->GetMessageKey(&firstHdrId);
    if (msgId != firstHdrId)
        threadIndex = GetIndexOfFirstDisplayedKeyInThread(threadHdr);
    else
        threadIndex = msgIndex;
    rv = MarkThreadRead(threadHdr, threadIndex, idsMarkedRead, bRead);
    return rv;
}

nsresult nsMsgDBView::MarkThreadRead(nsIMsgThread *threadHdr, nsMsgViewIndex threadIndex, nsMsgKeyArray &idsMarkedRead, PRBool bRead)
{
    PRBool threadElided = PR_TRUE;
    if (threadIndex != nsMsgViewIndex_None)
        threadElided = (m_flags.GetAt(threadIndex) & MSG_FLAG_ELIDED);

    PRUint32 numChildren;
    threadHdr->GetNumChildren(&numChildren);
    for (PRInt32 childIndex = 0; childIndex < (PRInt32) numChildren ; childIndex++) {
        nsCOMPtr <nsIMsgDBHdr> msgHdr;
        threadHdr->GetChildHdrAt(childIndex, getter_AddRefs(msgHdr));
        NS_ASSERTION(msgHdr, "msgHdr is null");
        if (!msgHdr) {
            continue;
        }

        PRBool isRead;

        nsMsgKey hdrMsgId;
        msgHdr->GetMessageKey(&hdrMsgId);
        m_db->IsRead(hdrMsgId, &isRead);

        if (isRead != bRead) {
            m_db->MarkHdrRead(msgHdr, bRead, nsnull);
            // insert at the front.  should we insert at the end?
            idsMarkedRead.InsertAt(0, hdrMsgId);
        }
    }

    if (bRead) {
        printf("fix this\n");
        //threadHdr->SetNumNewChildren(0);
    }
    else {
        PRUint32 numChildren;
        threadHdr->GetNumChildren(&numChildren);
        printf("fix this\n");
        //threadHdr->SetNumNewChildren(numChildren);
    }
    return NS_OK;
}

// Starting from startIndex, performs the passed in navigation, including
// any marking read needed, and returns the resultId and resultIndex of the
// destination of the navigation. If there are no more unread in the view,
// it returns a resultId of nsMsgKey_None and an resultIndex of nsMsgViewIndex_None.
NS_IMETHODIMP nsMsgDBView::ViewNavigate(nsMsgNavigationTypeValue motion, nsMsgKey *pResultKey, nsMsgViewIndex *pResultIndex, nsMsgViewIndex *pThreadIndex, PRBool wrap, nsIMsgFolder **resultFolderInfo)
{
    NS_ENSURE_ARG_POINTER(pResultKey);
    NS_ENSURE_ARG_POINTER(pResultIndex);
    NS_ENSURE_ARG_POINTER(pThreadIndex);
    NS_ENSURE_ARG_POINTER(resultFolderInfo);

    PRInt32 currentIndex;
    nsMsgViewIndex startIndex;
    nsresult rv = mOutlinerSelection->GetCurrentIndex(&currentIndex);
    NS_ENSURE_SUCCESS(rv, rv);
    startIndex = currentIndex;

    return nsMsgDBView::NavigateFromPos(motion, startIndex, pResultKey, pResultIndex, pThreadIndex, wrap, resultFolderInfo);
}

nsresult nsMsgDBView::NavigateFromPos(nsMsgNavigationTypeValue motion, nsMsgViewIndex startIndex, nsMsgKey *pResultKey, nsMsgViewIndex *pResultIndex, nsMsgViewIndex *pThreadIndex, PRBool wrap, nsIMsgFolder **resultFolderInfo)
{
    nsresult rv = NS_OK;
    nsMsgKey resultThreadKey;
    nsMsgViewIndex curIndex;
    nsMsgViewIndex lastIndex = (GetSize() > 0) ? (nsMsgViewIndex) GetSize() - 1 : nsMsgViewIndex_None;
    nsMsgViewIndex threadIndex = nsMsgViewIndex_None;

    switch (motion) {
        case nsMsgNavigationType::firstMessage:
            if (GetSize() > 0) {
                *pResultIndex = 0;
                *pResultKey = m_keys.GetAt(0);
            }
            else {
                *pResultIndex = nsMsgViewIndex_None;
                *pResultKey = nsMsgKey_None;
            }
            break;
        case nsMsgNavigationType::nextMessage:
            // return same index and id on next on last message
            *pResultIndex = PR_MIN(startIndex + 1, lastIndex);
            *pResultKey = m_keys.GetAt(*pResultIndex);
            break;
        case nsMsgNavigationType::previousMessage:
            *pResultIndex = (startIndex > 0) ? startIndex - 1 : 0;
            *pResultKey = m_keys.GetAt(*pResultIndex);
            break;
        case nsMsgNavigationType::lastMessage:
            *pResultIndex = lastIndex;
            *pResultKey = m_keys.GetAt(*pResultIndex);
            break;
        case nsMsgNavigationType::firstFlagged:
            rv = FindFirstFlagged(pResultIndex);
            if (IsValidIndex(*pResultIndex))
                *pResultKey = m_keys.GetAt(*pResultIndex);
            break;
        case nsMsgNavigationType::nextFlagged:
            rv = FindNextFlagged(startIndex + 1, pResultIndex);
            if (IsValidIndex(*pResultIndex))
                *pResultKey = m_keys.GetAt(*pResultIndex);
            break;
        case nsMsgNavigationType::previousFlagged:
            rv = FindPrevFlagged(startIndex, pResultIndex);
            if (IsValidIndex(*pResultIndex))
                *pResultKey = m_keys.GetAt(*pResultIndex);
            break;
        case nsMsgNavigationType::firstNew:
            rv = FindFirstNew(pResultIndex);
            if (IsValidIndex(*pResultIndex))
                *pResultKey = m_keys.GetAt(*pResultIndex);
            break;
        case nsMsgNavigationType::firstUnreadMessage:
            startIndex = nsMsgViewIndex_None;        // note fall thru - is this motion ever used?
        case nsMsgNavigationType::nextUnreadMessage:
            for (curIndex = (startIndex == nsMsgViewIndex_None) ? 0 : startIndex; curIndex <= lastIndex && lastIndex != nsMsgViewIndex_None; curIndex++) {
                PRUint32 flags = m_flags.GetAt(curIndex);

                // don't return start index since navigate should move
                if (!(flags & MSG_FLAG_READ) && (curIndex != startIndex)) {
                    *pResultIndex = curIndex;
                    *pResultKey = m_keys.GetAt(*pResultIndex);
                    break;
                }
                // check for collapsed thread with new children
                if (m_sortType == nsMsgViewSortType::byThread && flags & MSG_VIEW_FLAG_ISTHREAD && flags & MSG_FLAG_ELIDED) {
                    nsCOMPtr <nsIMsgThread> threadHdr;
                    GetThreadContainingIndex(curIndex, getter_AddRefs(threadHdr));
                    NS_ENSURE_SUCCESS(rv, rv);

                    NS_ASSERTION(threadHdr, "threadHdr is null");
                    if (!threadHdr) {
                        continue;
                    }
                    PRUint32 numUnreadChildren;
                    threadHdr->GetNumUnreadChildren(&numUnreadChildren);
                    if (numUnreadChildren > 0) {
                        PRUint32 numExpanded;
                        ExpandByIndex(curIndex, &numExpanded);
                        lastIndex += numExpanded;
                        if (pThreadIndex)
                            *pThreadIndex = curIndex;
                    }
                }
            }
            if (curIndex > lastIndex) {
                // wrap around by starting at index 0.
                if (wrap) {
                    nsMsgKey startKey = GetAt(startIndex);

                    rv = NavigateFromPos(nsMsgNavigationType::nextUnreadMessage, nsMsgViewIndex_None, pResultKey, pResultIndex, pThreadIndex, PR_FALSE, resultFolderInfo);
                    if (*pResultKey == startKey) {   
                        // wrapped around and found start message!
                        *pResultIndex = nsMsgViewIndex_None;
                        *pResultKey = nsMsgKey_None;
                    }
                }
                else {
                    *pResultIndex = nsMsgViewIndex_None;
                    *pResultKey = nsMsgKey_None;
                }
            }
            break;
        case nsMsgNavigationType::previousUnreadMessage:
            rv = FindPrevUnread(m_keys.GetAt(startIndex), pResultKey,
                                &resultThreadKey);
            if (NS_SUCCEEDED(rv)) {
                *pResultIndex = FindViewIndex(*pResultKey);
                if (*pResultKey != resultThreadKey && m_sortType == nsMsgViewSortType::byThread) {
                    threadIndex  = ThreadIndexOfMsg(*pResultKey, nsMsgViewIndex_None);
                    if (*pResultIndex == nsMsgViewIndex_None) {
                        nsCOMPtr <nsIMsgThread> threadHdr;
                        nsCOMPtr <nsIMsgDBHdr> msgHdr;
                        rv = m_db->GetMsgHdrForKey(*pResultKey, getter_AddRefs(msgHdr));
                        NS_ENSURE_SUCCESS(rv, rv);
                        rv = m_db->GetThreadContainingMsgHdr(msgHdr, getter_AddRefs(threadHdr));
                        NS_ENSURE_SUCCESS(rv, rv);

                        NS_ASSERTION(threadHdr, "threadHdr is null");
                        if (threadHdr) {
                            break;
                        }
                        PRUint32 numUnreadChildren;
                        threadHdr->GetNumUnreadChildren(&numUnreadChildren);
                        if (numUnreadChildren > 0) {
                            PRUint32 numExpanded;
                            ExpandByIndex(threadIndex, &numExpanded);
                        }
                        *pResultIndex = FindViewIndex(*pResultKey);
                    }
                }
                if (pThreadIndex)
                    *pThreadIndex = threadIndex;
            }
            break;
        case nsMsgNavigationType::lastUnreadMessage:
            break;
        case nsMsgNavigationType::nextUnreadThread:
            {
                nsMsgKeyArray idsMarkedRead;

                if (startIndex == nsMsgViewIndex_None) {
                    NS_ASSERTION(0,"startIndex == nsMsgViewIndex_None");
                    break;
                }
                rv = MarkThreadOfMsgRead(m_keys.GetAt(startIndex), startIndex, idsMarkedRead, PR_TRUE);
                if (NS_SUCCEEDED(rv)) 
                    return NavigateFromPos(nsMsgNavigationType::nextUnreadMessage, startIndex, pResultKey, pResultIndex, pThreadIndex, PR_TRUE, resultFolderInfo);
                break;
            }
        case nsMsgNavigationType::toggleThreadKilled:
            {
                PRBool resultKilled;

                if (startIndex == nsMsgViewIndex_None) {
                    NS_ASSERTION(0,"startIndex == nsMsgViewIndex_None");
                    break;
                }
                threadIndex = ThreadIndexOfMsg(GetAt(startIndex), startIndex);
                ToggleIgnored(&startIndex, 1, &resultKilled);
                if (resultKilled) {
                    if (threadIndex != nsMsgViewIndex_None)
                        CollapseByIndex(threadIndex, nsnull);
                    return NavigateFromPos(nsMsgNavigationType::nextUnreadThread, threadIndex, pResultKey, pResultIndex, pThreadIndex, PR_TRUE, resultFolderInfo);
                }
                else {
                    *pResultIndex = startIndex;
                    *pResultKey = m_keys.GetAt(*pResultIndex);
                    return NS_OK;
                }
            }
        case nsMsgNavigationType::laterMessage:
            if (startIndex == nsMsgViewIndex_None) {
                NS_ASSERTION(0, "unexpected");
                break;
            }
            m_db->MarkLater(m_keys.GetAt(startIndex), LL_ZERO);
            return NavigateFromPos(nsMsgNavigationType::nextUnreadMessage, startIndex, pResultKey, pResultIndex, pThreadIndex, PR_TRUE, resultFolderInfo);
        default:
            NS_ASSERTION(0, "unsupported motion");
            break;
    }
    return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::NavigateStatus(nsMsgNavigationTypeValue motion, PRBool *_retval)
{
    NS_ENSURE_ARG_POINTER(_retval);

    PRBool enable = PR_FALSE;
    nsresult rv = NS_ERROR_FAILURE;
    nsMsgKey resultKey = nsMsgKey_None;
    PRInt32 index;
    nsMsgViewIndex resultIndex = nsMsgViewIndex_None;
    rv = mOutlinerSelection->GetCurrentIndex(&index);

    // warning - we no longer validate index up front because fe passes in -1 for no
    // selection, so if you use index, be sure to validate it before using it
    // as an array index.
    switch (motion) {
        case nsMsgNavigationType::firstMessage:
        case nsMsgNavigationType::lastMessage:
            if (GetSize() > 0)
                enable = PR_TRUE;
            break;
        case nsMsgNavigationType::nextMessage:
            if (IsValidIndex(index) && index < GetSize() - 1)
                enable = PR_TRUE;
            break;
        case nsMsgNavigationType::previousMessage:
            if (IsValidIndex(index) && index != 0 && GetSize() > 1)
                enable = PR_TRUE;
            break;
        case nsMsgNavigationType::firstFlagged:
            rv = FindFirstFlagged(&resultIndex);
            enable = (NS_SUCCEEDED(rv) && resultIndex != nsMsgViewIndex_None);
            break;
        case nsMsgNavigationType::nextFlagged:
            rv = FindNextFlagged(index + 1, &resultIndex);
            enable = (NS_SUCCEEDED(rv) && resultIndex != nsMsgViewIndex_None);
            break;
        case nsMsgNavigationType::previousFlagged:
            if (IsValidIndex(index) && index != 0)
                rv = FindPrevFlagged(index, &resultIndex);
            enable = (NS_SUCCEEDED(rv) && resultIndex != nsMsgViewIndex_None);
            break;
        case nsMsgNavigationType::firstNew:
            rv = FindFirstNew(&resultIndex);
            enable = (NS_SUCCEEDED(rv) && resultIndex != nsMsgViewIndex_None);
            break;
        case nsMsgNavigationType::laterMessage:
            enable = GetSize() > 0;
            break;
        case nsMsgNavigationType::readMore:
            enable = PR_TRUE;  // for now, always true.
            break;
        case nsMsgNavigationType::nextFolder:
        case nsMsgNavigationType::nextUnreadThread:
        case nsMsgNavigationType::nextUnreadMessage:
        case nsMsgNavigationType::toggleThreadKilled:
            enable = PR_TRUE;  // always enabled
            break;
        case nsMsgNavigationType::previousUnreadMessage:
            if (IsValidIndex(index)) {
                nsMsgKey threadId;
                rv = FindPrevUnread(m_keys.GetAt(index), &resultKey, &threadId);
                enable = (resultKey != nsMsgKey_None);
            }
            break;
        default:
            NS_ASSERTION(0,"unexpected");
            break;
    }

    *_retval = enable;
    return NS_OK;
}

// Note that these routines do NOT expand collapsed threads! This mimics the old behaviour,
// but it's also because we don't remember whether a thread contains a flagged message the
// same way we remember if a thread contains new messages. It would be painful to dive down
// into each collapsed thread to update navigate status.
// We could cache this info, but it would still be expensive the first time this status needs
// to get updated.
nsresult nsMsgDBView::FindNextFlagged(nsMsgViewIndex startIndex, nsMsgViewIndex *pResultIndex)
{
    nsMsgViewIndex lastIndex = (nsMsgViewIndex) GetSize() - 1;
    nsMsgViewIndex curIndex;

    *pResultIndex = nsMsgViewIndex_None;

    if (GetSize() > 0) {
        for (curIndex = startIndex; curIndex <= lastIndex; curIndex++) {
            PRUint32 flags = m_flags.GetAt(curIndex);
            if (flags & MSG_FLAG_MARKED) {
                *pResultIndex = curIndex;
                break;
            }
        }
    }

    return NS_OK;
}

nsresult nsMsgDBView::FindFirstNew(nsMsgViewIndex *pResultIndex)
{
    if (m_db) {
        nsMsgKey firstNewKey;
        m_db->GetFirstNew(&firstNewKey);
        if (pResultIndex)
            *pResultIndex = FindKey(firstNewKey, PR_TRUE);
    }
    return NS_OK;
}

// Generic routine to find next unread id. It doesn't do an expand of a
// thread with new messages, so it can't return a view index.
nsresult nsMsgDBView::FindNextUnread(nsMsgKey startId, nsMsgKey *pResultKey,
                                     nsMsgKey *resultThreadId)
{
    nsMsgViewIndex startIndex = FindViewIndex(startId);
    nsMsgViewIndex curIndex = startIndex;
    nsMsgViewIndex lastIndex = (nsMsgViewIndex) GetSize() - 1;
    nsresult rv = NS_OK;

    if (startIndex == nsMsgViewIndex_None)
        return NS_MSG_MESSAGE_NOT_FOUND;

    *pResultKey = nsMsgKey_None;
    if (resultThreadId)
        *resultThreadId = nsMsgKey_None;

    for (; curIndex <= lastIndex && (*pResultKey == nsMsgKey_None); curIndex++) {
        char    flags = m_flags.GetAt(curIndex);

        if (!(flags & MSG_FLAG_READ) && (curIndex != startIndex)) {
            *pResultKey = m_keys.GetAt(curIndex);
            break;
        }
        // check for collapsed thread with unread children
        if (m_sortType == nsMsgViewSortType::byThread && flags & MSG_VIEW_FLAG_ISTHREAD && flags & MSG_FLAG_ELIDED) {
            nsCOMPtr<nsIMsgThread> thread;
            //nsMsgKey threadId = m_keys.GetAt(curIndex);
            rv = GetThreadFromMsgIndex(curIndex, getter_AddRefs(thread));
            if (NS_SUCCEEDED(rv) && thread) {
              nsCOMPtr <nsIMsgDBHdr> unreadChild;
              rv = thread->GetFirstUnreadChild(getter_AddRefs(unreadChild));
              if (NS_SUCCEEDED(rv) && unreadChild)
                unreadChild->GetMessageKey(pResultKey);
            }
            //rv = m_db->GetUnreadKeyInThread(threadId, pResultKey, resultThreadId);
            if (NS_SUCCEEDED(rv) && (*pResultKey != nsMsgKey_None))
                break;
        }
    }
    // found unread message but we don't know the thread
    if (*pResultKey != nsMsgKey_None && resultThreadId && *resultThreadId == nsMsgKey_None) {
        printf("fix this\n");
        //*resultThreadId = m_db->GetThreadIdForMsgId(*pResultKey);
    }
    return rv;
}


nsresult nsMsgDBView::FindPrevUnread(nsMsgKey startKey, nsMsgKey *pResultKey,
                                     nsMsgKey *resultThreadId)
{
    nsMsgViewIndex startIndex = FindViewIndex(startKey);
    nsMsgViewIndex curIndex = startIndex;
    nsresult rv = NS_MSG_MESSAGE_NOT_FOUND;

    if (startIndex == nsMsgViewIndex_None)
        return NS_MSG_MESSAGE_NOT_FOUND;

    *pResultKey = nsMsgKey_None;
    if (resultThreadId)
        *resultThreadId = nsMsgKey_None;

    for (; (int) curIndex >= 0 && (*pResultKey == nsMsgKey_None); curIndex--) {
        PRUint32 flags = m_flags.GetAt(curIndex);

        if (curIndex != startIndex && flags & MSG_VIEW_FLAG_ISTHREAD && flags & MSG_FLAG_ELIDED) {
            printf("fix this\n");
            //nsMsgKey threadId = m_keys.GetAt(curIndex);
            //rv = m_db->GetUnreadKeyInThread(threadId, pResultKey, resultThreadId);
            if (NS_SUCCEEDED(rv) && (*pResultKey != nsMsgKey_None))
                break;
        }
        if (!(flags & MSG_FLAG_READ) && (curIndex != startIndex)) {
            *pResultKey = m_keys.GetAt(curIndex);
            rv = NS_OK;
            break;
        }
    }
    // found unread message but we don't know the thread
    if (*pResultKey != nsMsgKey_None && resultThreadId && *resultThreadId == nsMsgKey_None) {
        printf("fix this\n");
        //*resultThreadId = m_db->GetThreadIdForMsgId(*pResultKey);
    }
    return rv;
}

nsresult nsMsgDBView::FindFirstFlagged(nsMsgViewIndex *pResultIndex)
{
    return FindNextFlagged(0, pResultIndex);
}

nsresult nsMsgDBView::FindPrevFlagged(nsMsgViewIndex startIndex, nsMsgViewIndex *pResultIndex)
{
    nsMsgViewIndex curIndex;

    *pResultIndex = nsMsgViewIndex_None;

    if (GetSize() > 0 && IsValidIndex(startIndex)) {
        curIndex = startIndex;
        do {
            if (curIndex != 0)
                curIndex--;

            PRUint32 flags = m_flags.GetAt(curIndex);
            if (flags & MSG_FLAG_MARKED) {
                *pResultIndex = curIndex;
                break;
            }
        }
        while (curIndex != 0);
    }
    return NS_OK;
}

PRBool nsMsgDBView::IsValidIndex(nsMsgViewIndex index)
{
    return (index < (nsMsgViewIndex) m_keys.GetSize());
}

nsresult nsMsgDBView::OrExtraFlag(nsMsgViewIndex index, PRUint32 orflag)
{
	PRUint32	flag;
	if (!IsValidIndex(index))
		return NS_MSG_INVALID_DBVIEW_INDEX;
	flag = m_flags[index];
	flag |= orflag;
	m_flags[index] = flag;
	OnExtraFlagChanged(index, flag);
	return NS_OK;
}

nsresult nsMsgDBView::AndExtraFlag(nsMsgViewIndex index, PRUint32 andflag)
{
	PRUint32	flag;
	if (!IsValidIndex(index))
		return NS_MSG_INVALID_DBVIEW_INDEX;
	flag = m_flags[index];
	flag &= andflag;
	m_flags[index] = flag;
	OnExtraFlagChanged(index, flag);
	return NS_OK;
}

nsresult nsMsgDBView::SetExtraFlag(nsMsgViewIndex index, PRUint32 extraflag)
{
	if (!IsValidIndex(index))
		return NS_MSG_INVALID_DBVIEW_INDEX;
	m_flags[index] = extraflag;
	OnExtraFlagChanged(index, extraflag);
	return NS_OK;
}


nsresult nsMsgDBView::ToggleIgnored(nsMsgViewIndex * indices, PRInt32 numIndices, PRBool *resultToggleState)
{
  nsresult rv = NS_OK;
  nsCOMPtr <nsIMsgThread> thread;

  if (numIndices == 1)
  {
    nsMsgViewIndex    threadIndex = GetThreadFromMsgIndex(*indices, getter_AddRefs(thread));
    if (thread)
    {
      rv = ToggleThreadIgnored(thread, threadIndex);
      if (resultToggleState)
      {
        PRUint32 threadFlags;
        thread->GetFlags(&threadFlags);
        *resultToggleState = (threadFlags & MSG_FLAG_IGNORED) ? PR_TRUE : PR_FALSE;
      }
    }
  }
  else
  {
    if (numIndices > 1)
      NS_QuickSort (indices, numIndices, sizeof(nsMsgViewIndex), CompareViewIndices, nsnull);
    for (int curIndex = numIndices - 1; curIndex >= 0; curIndex--)
    {
      // here we need to build up the unique threads, and mark them ignored.
      nsMsgViewIndex threadIndex;
      threadIndex = GetThreadFromMsgIndex(*indices, getter_AddRefs(thread));
    }
  }
  return rv;
}

nsMsgViewIndex	nsMsgDBView::GetThreadFromMsgIndex(nsMsgViewIndex index, 
													 nsIMsgThread **threadHdr)
{
	nsMsgKey			msgKey = GetAt(index);
	nsMsgViewIndex		threadIndex;

	NS_ENSURE_ARG(threadHdr);
  nsresult rv = GetThreadContainingIndex(index, threadHdr);
  NS_ENSURE_SUCCESS(rv,nsMsgViewIndex_None);

	if (*threadHdr == nsnull)
		return nsMsgViewIndex_None;

  nsMsgKey threadKey;
  (*threadHdr)->GetThreadKey(&threadKey);
	if (msgKey !=threadKey)
		threadIndex = GetIndexOfFirstDisplayedKeyInThread(*threadHdr);
	else
		threadIndex = index;
	return threadIndex;
}

// ignore handling.
nsresult nsMsgDBView::ToggleThreadIgnored(nsIMsgThread *thread, nsMsgViewIndex threadIndex)

{
	nsresult		rv;
	if (!IsValidIndex(threadIndex))
		return NS_MSG_INVALID_DBVIEW_INDEX;
  PRUint32 threadFlags;
  thread->GetFlags(&threadFlags);
	rv = SetThreadIgnored(thread, threadIndex, !((threadFlags & MSG_FLAG_IGNORED) != 0));
	return rv;
}

nsresult nsMsgDBView::ToggleThreadWatched(nsIMsgThread *thread, nsMsgViewIndex index)
{
	nsresult		rv;
	if (!IsValidIndex(index))
		return NS_MSG_INVALID_DBVIEW_INDEX;
  PRUint32 threadFlags;
  thread->GetFlags(&threadFlags);
	rv = SetThreadWatched(thread, index, !((threadFlags & MSG_FLAG_WATCHED) != 0));
	return rv;
}

nsresult nsMsgDBView::ToggleWatched( nsMsgViewIndex* indices,	PRInt32 numIndices)
{
	nsresult rv;
	nsCOMPtr <nsIMsgThread> thread;

	if (numIndices == 1)
	{
		nsMsgViewIndex	threadIndex = GetThreadFromMsgIndex(*indices, getter_AddRefs(thread));
		if (threadIndex != nsMsgViewIndex_None)
		{
			rv = ToggleThreadWatched(thread, threadIndex);
		}
	}
	else
	{
		if (numIndices > 1)
			NS_QuickSort (indices, numIndices, sizeof(nsMsgViewIndex), CompareViewIndices, nsnull);
		for (int curIndex = numIndices - 1; curIndex >= 0; curIndex--)
		{
		  nsMsgViewIndex	threadIndex;    
          threadIndex = GetThreadFromMsgIndex(*indices, getter_AddRefs(thread));
      // here we need to build up the unique threads, and mark them watched.
		}
	}
	return NS_OK;
}

nsresult nsMsgDBView::SetThreadIgnored(nsIMsgThread *thread, nsMsgViewIndex threadIndex, PRBool ignored)
{
	nsresult rv;

	if (!IsValidIndex(threadIndex))
		return NS_MSG_INVALID_DBVIEW_INDEX;
	rv = m_db->MarkThreadIgnored(thread, m_keys[threadIndex], ignored, this);
	NoteChange(threadIndex, 1, nsMsgViewNotificationCode::changed);
	if (ignored)
	{
		nsMsgKeyArray	idsMarkedRead;

		MarkThreadRead(thread, threadIndex, idsMarkedRead, PR_TRUE);
	}
	return rv;
}
nsresult nsMsgDBView::SetThreadWatched(nsIMsgThread *thread, nsMsgViewIndex index, PRBool watched)
{
	nsresult rv;

	if (!IsValidIndex(index))
		return NS_MSG_INVALID_DBVIEW_INDEX;
	rv = m_db->MarkThreadWatched(thread, m_keys[index], watched, this);
	NoteChange(index, 1, nsMsgViewNotificationCode::changed);
	return rv;
}

NS_IMETHODIMP nsMsgDBView::GetMsgFolder(nsIMsgFolder **aMsgFolder)
{
  NS_ENSURE_ARG_POINTER(aMsgFolder);
  *aMsgFolder = m_folder;
  NS_IF_ADDREF(*aMsgFolder);
  return NS_OK;
}

NS_IMETHODIMP 
nsMsgDBView::GetNumSelected(PRUint32 *numSelected)
{
  NS_ENSURE_ARG_POINTER(numSelected);
  *numSelected = 0;
  if (!mOutlinerSelection) {
    return NS_OK;
  }
   
  // we could just use GetSelectedIndices(), we don't for performance
  // we don't need to know the rows that are selected, just
  // how many of them.  no need to allocate a nsUInt32Array
  // just to throw it away.  we call this a lot from the 
  // front end JS, so make it fast.
  PRInt32 selectionCount;
  nsresult rv = mOutlinerSelection->GetRangeCount(&selectionCount);
  for (PRInt32 i = 0; i < selectionCount; i++) {
    PRInt32 startRange;
    PRInt32 endRange;
    rv = mOutlinerSelection->GetRangeAt(i, &startRange, &endRange);
    NS_ENSURE_SUCCESS(rv, rv);
    PRInt32 viewSize = GetSize();
    if (startRange >= 0 && startRange < viewSize) {
      for (PRInt32 rangeIndex = startRange; rangeIndex <= endRange && rangeIndex < viewSize; rangeIndex++) {
        (*numSelected)++;
      }
    }
  }
  return NS_OK;
}

// if nothing selected, return an NS_ERROR
NS_IMETHODIMP
nsMsgDBView::GetHdrForFirstSelectedMessage(nsIMsgDBHdr **hdr)
{
  NS_ENSURE_ARG_POINTER(hdr);

  nsresult rv;
  nsMsgKey key;
  rv = GetKeyForFirstSelectedMessage(&key);
  // don't assert, it is legal for nothing to be selected
  if (NS_FAILED(rv)) return rv;

  rv = m_db->GetMsgHdrForKey(key, hdr);
  NS_ENSURE_SUCCESS(rv,rv);
  return NS_OK;
}

// if nothing selected, return an NS_ERROR
NS_IMETHODIMP 
nsMsgDBView::GetURIForFirstSelectedMessage(char **uri)
{
  NS_ENSURE_ARG_POINTER(uri);

#ifdef DEBUG_mscott
  printf("inside GetURIForFirstSelectedMessage\n");
#endif

  if (!mOutlinerSelection) return NS_ERROR_UNEXPECTED;
  nsresult rv;
  nsMsgKey key;
  rv = GetKeyForFirstSelectedMessage(&key);
  // don't assert, it is legal for nothing to be selected
  if (NS_FAILED(rv)) return rv;
 
  rv = GenerateURIForMsgKey(key, m_folder, uri);
  NS_ENSURE_SUCCESS(rv,rv);
  return NS_OK;
}

nsresult
nsMsgDBView::GetKeyForFirstSelectedMessage(nsMsgKey *key)
{
  NS_ENSURE_ARG_POINTER(key);
  PRInt32 currentIndex;
  nsresult rv = mOutlinerSelection->GetCurrentIndex(&currentIndex);
  NS_ENSURE_SUCCESS(rv, rv);

  // check that the first index is valid, it may not be if nothing is selected
  if (currentIndex >= 0 && currentIndex < GetSize()) {
    *key = m_keys.GetAt(currentIndex);
  }
  else {
    return NS_ERROR_UNEXPECTED;
  }
  return NS_OK;
}
