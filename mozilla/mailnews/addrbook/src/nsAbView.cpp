/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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
 * Portions created by the Initial Developer are Copyright (C) 2001
 * the Initial Developer. All Rights Reserved.
 *
 * Original Author:
 *   Seth Spitzer <sspitzer@netscape.com>
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

#include "nsAbView.h"
#include "nsISupports.h"
#include "nsIRDFService.h"
#include "nsCOMPtr.h"
#include "nsIServiceManager.h"
#include "nsIAbCard.h"
#include "nsILocale.h"
#include "nsILocaleService.h"
#include "prmem.h"
#include "nsCollationCID.h"
#include "nsIAddrBookSession.h"
#include "nsAbBaseCID.h"

#define CARD_NOT_FOUND -1
#define ALL_ROWS -1

static NS_DEFINE_CID(kCollationFactoryCID, NS_COLLATIONFACTORY_CID);

NS_IMPL_ADDREF(nsAbView)
NS_IMPL_RELEASE(nsAbView)

NS_INTERFACE_MAP_BEGIN(nsAbView)
   NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIAbView)
   NS_INTERFACE_MAP_ENTRY(nsIAbView)
   NS_INTERFACE_MAP_ENTRY(nsIOutlinerView)
   NS_INTERFACE_MAP_ENTRY(nsIAbListener)
NS_INTERFACE_MAP_END

nsAbView::nsAbView()
{
  NS_INIT_ISUPPORTS();
  mMailListAtom = getter_AddRefs(NS_NewAtom("MailList"));
}

nsAbView::~nsAbView()
{
  nsresult rv = NS_OK;
  nsCOMPtr<nsIAddrBookSession> abSession = do_GetService(NS_ADDRBOOKSESSION_CONTRACTID, &rv); 
  if(NS_SUCCEEDED(rv))
    abSession->RemoveAddressBookListener(this);

  PRInt32 i = mCards.Count();
  while(i-- > 0)
  {
    RemoveCardAt(i);
  }
}

void nsAbView::RemoveCardAt(PRInt32 row)
{
  AbCard *abcard = (AbCard*) (mCards.ElementAt(row));
  NS_IF_RELEASE(abcard->card);
  mCards.RemoveElementAt(row);
  PR_FREEIF(abcard->primaryCollationKey);
  PR_FREEIF(abcard->secondaryCollationKey);
  PR_FREEIF(abcard);
}

NS_IMETHODIMP nsAbView::Init(const char *aURI)
{
  nsresult rv;

  mURI = aURI;

  /* todo:
  register as a listener of "mail.addr_book.lastnamefirst"
  const kDisplayName = 0;
  const kLastNameFirst = 1;
  const kFirstNameFirst = 2;
  if it changes, rebuild and invalidate
  */

  nsCOMPtr <nsIRDFService> rdfService = do_GetService("@mozilla.org/rdf/rdf-service;1",&rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr <nsIRDFResource> resource;
  rv = rdfService->GetResource(aURI, getter_AddRefs(resource));
  NS_ENSURE_SUCCESS(rv, rv);

  mDirectory = do_QueryInterface(resource, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
    
  rv = EnumerateCards();
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIAddrBookSession> abSession = do_GetService(NS_ADDRBOOKSESSION_CONTRACTID, &rv); 
	NS_ENSURE_SUCCESS(rv,rv);

	rv = abSession->AddAddressBookListener(this);
  NS_ENSURE_SUCCESS(rv,rv);
  return rv;
}

nsresult nsAbView::EnumerateCards()
{
  nsresult rv;    
  nsCOMPtr<nsIEnumerator> cardsEnumerator;
  nsCOMPtr<nsIAbCard> card;

  if (!mDirectory)
    return NS_ERROR_UNEXPECTED;

  rv = mDirectory->GetChildCards(getter_AddRefs(cardsEnumerator));
  if (NS_SUCCEEDED(rv) && cardsEnumerator)
  {
		nsCOMPtr<nsISupports> item;
	  for (rv = cardsEnumerator->First(); NS_SUCCEEDED(rv); rv = cardsEnumerator->Next())
	  {
      rv = cardsEnumerator->CurrentItem(getter_AddRefs(item));
      if (NS_SUCCEEDED(rv))
      {
        nsCOMPtr <nsIAbCard> card = do_QueryInterface(item);
        // malloc these from an arena
        AbCard *abcard = (AbCard *) PR_Calloc(1, sizeof(struct AbCard));
        abcard->card = card;
        NS_IF_ADDREF(abcard->card);
        rv = mCards.AppendElement((void *)abcard);
        NS_ASSERTION(NS_SUCCEEDED(rv), "failed to append card");
      }
    }
  }

  // XXX hard code, default sortby name
  rv = SortBy(NS_LITERAL_STRING("DisplayName").get());
  return NS_OK;
}

NS_IMETHODIMP nsAbView::GetRowCount(PRInt32 *aRowCount)
{
  *aRowCount = mCards.Count();
  return NS_OK;
}

NS_IMETHODIMP nsAbView::GetSelection(nsIOutlinerSelection * *aSelection)
{
  *aSelection = mOutlinerSelection;
  NS_IF_ADDREF(*aSelection);
  return NS_OK;
}

NS_IMETHODIMP nsAbView::SetSelection(nsIOutlinerSelection * aSelection)
{
  mOutlinerSelection = aSelection;
  return NS_OK;
}

NS_IMETHODIMP nsAbView::GetRowProperties(PRInt32 index, nsISupportsArray *properties)
{
    return NS_OK;
}

NS_IMETHODIMP nsAbView::GetCellProperties(PRInt32 row, const PRUnichar *colID, nsISupportsArray *properties)
{
  // "Di" to distinguish "DisplayName" from "Department"
  if (colID[0] != 'D' || colID[1] != 'i')
    return NS_OK;

  nsIAbCard *card = ((AbCard *)(mCards.ElementAt(row)))->card;

  PRBool isMailList = PR_FALSE;
  nsresult rv = card->GetIsMailList(&isMailList);
  NS_ENSURE_SUCCESS(rv,rv);

  if (isMailList)
    properties->AppendElement(mMailListAtom);  

  return NS_OK;
}

NS_IMETHODIMP nsAbView::GetColumnProperties(const PRUnichar *colID, nsIDOMElement *colElt, nsISupportsArray *properties)
{
    return NS_OK;
}

NS_IMETHODIMP nsAbView::IsContainer(PRInt32 index, PRBool *_retval)
{
    *_retval = PR_FALSE;
    return NS_OK;
}

NS_IMETHODIMP nsAbView::IsContainerOpen(PRInt32 index, PRBool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsAbView::IsContainerEmpty(PRInt32 index, PRBool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsAbView::IsSeparator(PRInt32 index, PRBool *_retval)
{
  *_retval = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP nsAbView::IsSorted(PRBool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsAbView::CanDropOn(PRInt32 index, PRBool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsAbView::CanDropBeforeAfter(PRInt32 index, PRBool before, PRBool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsAbView::Drop(PRInt32 row, PRInt32 orientation)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsAbView::GetParentIndex(PRInt32 rowIndex, PRInt32 *_retval)
{
  *_retval = 0;
  return NS_OK;
}

NS_IMETHODIMP nsAbView::HasNextSibling(PRInt32 rowIndex, PRInt32 afterIndex, PRBool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsAbView::GetLevel(PRInt32 index, PRInt32 *_retval)
{
  *_retval = 0;
  return NS_OK;
}

NS_IMETHODIMP nsAbView::GetCellText(PRInt32 row, const PRUnichar *colID, PRUnichar **_retval)
{
  nsIAbCard *card = ((AbCard *)(mCards.ElementAt(row)))->card;

  nsresult rv = card->GetCardUnicharValue(colID, _retval);
  NS_ENSURE_SUCCESS(rv,rv);

  return rv;
}

NS_IMETHODIMP nsAbView::SetOutliner(nsIOutlinerBoxObject *outliner)
{
  mOutliner = outliner;
  return NS_OK;
}

NS_IMETHODIMP nsAbView::ToggleOpenState(PRInt32 index)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsAbView::CycleHeader(const PRUnichar *colID, nsIDOMElement *elt)
{
  nsresult rv = SortBy(colID);
  NS_ENSURE_SUCCESS(rv,rv);

  rv = InvalidateOutliner(ALL_ROWS);
  NS_ENSURE_SUCCESS(rv,rv);
  return rv;
}

nsresult nsAbView::InvalidateOutliner(PRInt32 row)
{
  if (!mOutliner)
    return NS_ERROR_UNEXPECTED;
  
  if (row == ALL_ROWS)
    return mOutliner->Invalidate();
  else
    return mOutliner->InvalidateRow(row);
}

NS_IMETHODIMP nsAbView::SelectionChanged()
{
    return NS_OK;
}

NS_IMETHODIMP nsAbView::CycleCell(PRInt32 row, const PRUnichar *colID)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsAbView::IsEditable(PRInt32 row, const PRUnichar *colID, PRBool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsAbView::SetCellText(PRInt32 row, const PRUnichar *colID, const PRUnichar *value)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsAbView::PerformAction(const PRUnichar *action)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsAbView::PerformActionOnRow(const PRUnichar *action, PRInt32 row)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsAbView::PerformActionOnCell(const PRUnichar *action, PRInt32 row, const PRUnichar *colID)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsAbView::GetCardFromRow(PRInt32 row, nsIAbCard **aCard)
{
  *aCard = ((AbCard *)(mCards.ElementAt(row)))->card;
  NS_IF_ADDREF(*aCard);
  return NS_OK;
}

static int PR_CALLBACK
inplaceSortCallback(const void *data1, const void *data2, void *privateData)
{
  AbCard *card1 = (AbCard *)data1;
  AbCard *card2 = (AbCard *)data2;

  const PRUnichar *colID = (const PRUnichar *)privateData;

  PRInt32 sortValue;

  // if we are sorting the "PrimaryEmail", swap the collation keys, as the secondary is always the 
  // PrimaryEmail.  use the last primary key as the secondary key.
  //
  // "Pr" to distinguish "PrimaryEmail" from "PagerNumber"
  if (colID[0] == 'P' && colID[1] == 'r') {
    sortValue = nsCRT::strcmp(card1->secondaryCollationKey, card2->secondaryCollationKey);
    if (sortValue)
    return sortValue;
    else
      return nsCRT::strcmp(card1->primaryCollationKey, card2->primaryCollationKey);
}
  else {
    sortValue = nsCRT::strcmp(card1->primaryCollationKey, card2->primaryCollationKey);
    if (sortValue)
      return sortValue;
    else
      return nsCRT::strcmp(card1->secondaryCollationKey, card2->secondaryCollationKey);
  }
}

nsresult nsAbView::SortBy(const PRUnichar *colID)
{
  nsresult rv;

  PRInt32 count = mCards.Count();
  PRInt32 i;

  // if we are sorting by how we are already sorted, just reverse
  if (!nsCRT::strcmp(mSortedColumn.get(),colID)) {
    PRInt32 halfPoint = count / 2;
    for (i=0; i < halfPoint; i++) {
      // swap the elements.
      void *ptr1 = mCards.ElementAt(i);
      void *ptr2 = mCards.ElementAt(count - i - 1);
      mCards.ReplaceElementAt(ptr2, i);
      mCards.ReplaceElementAt(ptr1, count - i - 1);
    }
  }
  else {
    // generate collation keys
    for (i=0; i < count; i++) {
      AbCard *abcard = (AbCard *)(mCards.ElementAt(i));

      rv = GenerateCollationKeysForCard(colID, abcard);
      NS_ENSURE_SUCCESS(rv,rv);
    }
    mCards.Sort(inplaceSortCallback, (void *)colID);
    mSortedColumn = colID;
  }

  return NS_OK;
}

nsresult nsAbView::GenerateCollationKeysForCard(const PRUnichar *colID, AbCard *abcard)
{
  nsresult rv;
  nsXPIDLString value;
  // XXX fix me, do this with const to avoid the strcpy
  rv = abcard->card->GetCardUnicharValue(colID, getter_Copies(value));
  NS_ENSURE_SUCCESS(rv,rv);
  
  // XXX be smarter about the key, from an arena
  PR_FREEIF(abcard->primaryCollationKey);
  rv = CreateCollationKey(value, &(abcard->primaryCollationKey));
  NS_ENSURE_SUCCESS(rv,rv);
  
  // XXX fix me, do this with const to avoid the strcpy
  rv = abcard->card->GetCardUnicharValue(NS_LITERAL_STRING("PrimaryEmail").get(), getter_Copies(value));
  NS_ENSURE_SUCCESS(rv,rv);
  
  // XXX be smarter about the key, from an arena
  PR_FREEIF(abcard->secondaryCollationKey);
  rv = CreateCollationKey(value, &(abcard->secondaryCollationKey));
  NS_ENSURE_SUCCESS(rv,rv);
  return rv;
}

nsresult nsAbView::CreateCollationKey(const PRUnichar *source,  PRUnichar **result)
{
	nsresult rv;
	if (!mCollationKeyGenerator)
	{
		nsCOMPtr<nsILocaleService> localeSvc = do_GetService(NS_LOCALESERVICE_CONTRACTID,&rv); 
		NS_ENSURE_SUCCESS(rv, rv);

		nsCOMPtr<nsILocale> locale; 
		rv = localeSvc->GetApplicationLocale(getter_AddRefs(locale));
		NS_ENSURE_SUCCESS(rv, rv);

		nsCOMPtr <nsICollationFactory> factory = do_CreateInstance(kCollationFactoryCID, &rv); 
		NS_ENSURE_SUCCESS(rv, rv);

		rv = factory->CreateCollation(locale, getter_AddRefs(mCollationKeyGenerator));
		NS_ENSURE_SUCCESS(rv, rv);
	}

	nsAutoString sourceString(source);
	PRUint32 aLength;
	rv = mCollationKeyGenerator->GetSortKeyLen(kCollationCaseInSensitive, sourceString, &aLength);
	NS_ENSURE_SUCCESS(rv, rv);

	PRUint8* aKey = (PRUint8* ) nsMemory::Alloc (aLength + 3);    // plus three for null termination
	if (!aKey)
		return NS_ERROR_OUT_OF_MEMORY;

	rv = mCollationKeyGenerator->CreateRawSortKey(kCollationCaseInSensitive,
			sourceString, aKey, &aLength);
	if (NS_FAILED(rv))
	{
		nsMemory::Free (aKey);
		return rv;
	}

	// Generate a null terminated unicode string.
	// Note using PRUnichar* to store collation key is not recommented since the key may contains 0x0000.
	aKey[aLength] = 0;
	aKey[aLength+1] = 0;
	aKey[aLength+2] = 0;

	*result = (PRUnichar *) aKey;
	return rv;
}

NS_IMETHODIMP nsAbView::OnItemAdded(nsISupports *parentDir, nsISupports *item)
{
  nsresult rv;
  nsCOMPtr <nsIAbDirectory> directory = do_QueryInterface(parentDir,&rv);
  NS_ENSURE_SUCCESS(rv,rv);

  if (directory.get() == mDirectory.get()) {
    nsCOMPtr <nsIAbCard> addedCard = do_QueryInterface(item);
    if (addedCard) {
      // malloc these from an arena
      AbCard *abcard = (AbCard *) PR_Calloc(1, sizeof(struct AbCard));
      abcard->card = addedCard;
      NS_IF_ADDREF(abcard->card);
      
      rv = GenerateCollationKeysForCard(mSortedColumn.get(), abcard);
      NS_ENSURE_SUCCESS(rv,rv);
      
      PRInt32 index = FindIndexForInsert(mSortedColumn.get(), abcard);
      rv = mCards.InsertElementAt((void *)abcard, index);
      NS_ENSURE_SUCCESS(rv,rv);
      
      if (mOutliner)
        rv = mOutliner->RowCountChanged(index, 1);
      NS_ENSURE_SUCCESS(rv,rv);
    }
    else {
      // if a mailing list gets created, we'll get notified
      // but the item will be a nsIAbDirectory.
      // ignore it, as we've already been notified from CreateMailListAndAddToDB()
    }
  }
  return rv;
}

PRInt32 nsAbView::FindIndexForInsert(const PRUnichar *colID, AbCard *abcard)
{
  PRInt32 count = mCards.Count();
  PRInt32 i;

  void *item = (void *)abcard;
  
  // XXX todo, binary search
  for (i=0; i < count; i++) {
    void *current = mCards.ElementAt(i);
    PRInt32 value = inplaceSortCallback(item, current, (void *)colID);
    // XXX not right, for ascending and descending
    if (value <= 0) 
      break;
  }
  return i;
}

NS_IMETHODIMP nsAbView::OnItemRemoved(nsISupports *parentDir, nsISupports *item)
{
  nsresult rv;
  nsCOMPtr <nsIAbDirectory> directory = do_QueryInterface(parentDir,&rv);
  NS_ENSURE_SUCCESS(rv,rv);

  if (directory.get() == mDirectory.get()) {
    nsCOMPtr <nsIAbCard> card = do_QueryInterface(item);
    if (item) {
      PRInt32 index = FindIndexForCard(mSortedColumn.get(), card);
      NS_ASSERTION(index != CARD_NOT_FOUND,"card not found");
      if (index != CARD_NOT_FOUND) {
        RemoveCardAt(index);
        if (mOutliner)
          rv = mOutliner->RowCountChanged(index, -1);
        NS_ENSURE_SUCCESS(rv,rv);
      }
    }
    else {
      // if a mailing list gets delete, we'll get notified
      // but the item will be a nsIAbDirectory.
      // ignore it, as we've already been notified from NotifyDirectoryItemDeleted()
    }
  }
  return rv;
}

PRInt32 nsAbView::FindIndexForCard(const PRUnichar *colID, nsIAbCard *card)
{
  PRInt32 count = mCards.Count();
  PRInt32 i;
  
  // XXX todo, binary search
  for (i=0; i < count; i++) {
    AbCard *abcard = (AbCard*) (mCards.ElementAt(i));
    if (abcard->card == card) {
      return i;
    }
  }
  return CARD_NOT_FOUND;
}

NS_IMETHODIMP nsAbView::OnItemPropertyChanged(nsISupports *item, const char *property, const PRUnichar *oldValue, const PRUnichar *newValue)
{
  nsresult rv;

  nsCOMPtr <nsIAbCard> card = do_QueryInterface(item,&rv);
  NS_ENSURE_SUCCESS(rv,rv);
  
  PRInt32 index = FindIndexForCard(mSortedColumn.get(), card);
  if (index != CARD_NOT_FOUND) {
    rv = InvalidateOutliner(index);
    NS_ENSURE_SUCCESS(rv,rv);
  }
  return rv;
}

NS_IMETHODIMP nsAbView::SelectAll()
{
  if (mOutlinerSelection && mOutliner) {
    mOutlinerSelection->SelectAll();
    mOutliner->Invalidate();
  }
  return NS_OK;
}

NS_IMETHODIMP nsAbView::DeleteSelectedCards()
{
  if (mOutlinerSelection)
  {
    PRInt32 selectionCount; 
    nsresult rv = mOutlinerSelection->GetRangeCount(&selectionCount);
    NS_ENSURE_SUCCESS(rv,rv);

    if (!selectionCount)
      return NS_OK;

    nsCOMPtr <nsISupportsArray> cardsToDelete;
    NS_NewISupportsArray(getter_AddRefs(cardsToDelete));

    for (PRInt32 i = 0; i < selectionCount; i++)
    {
      PRInt32 startRange;
      PRInt32 endRange;
      rv = mOutlinerSelection->GetRangeAt(i, &startRange, &endRange);
      NS_ENSURE_SUCCESS(rv, NS_OK); 
      PRInt32 totalCards = mCards.Count();
      if (startRange >= 0 && startRange < totalCards)
      {
        for (PRInt32 rangeIndex = startRange; rangeIndex <= endRange && rangeIndex < totalCards; rangeIndex++) {
          nsCOMPtr<nsIAbCard> abCard;
          rv = GetCardFromRow(rangeIndex, getter_AddRefs(abCard));
          NS_ENSURE_SUCCESS(rv,rv);

          nsCOMPtr<nsISupports> supports = do_QueryInterface(abCard, &rv);
          NS_ENSURE_SUCCESS(rv,rv);

          rv = cardsToDelete->AppendElement(supports);
          NS_ENSURE_SUCCESS(rv,rv);
        }
      }
    }

    rv = mDirectory->DeleteCards(cardsToDelete);
    NS_ENSURE_SUCCESS(rv,rv);
  }
  return NS_OK;
}
