/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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

#ifdef MOZ_LOGGING
// sorry, this has to be before the pre-compiled header
#define FORCE_PR_LOG /* Allow logging in the release build */
#endif

#include "nsAbSync.h"
#include "prmem.h"
#include "nsAbSyncCID.h"
#include "nsIPref.h"
#include "nsIServiceManager.h"
#include "prprf.h"
#include "nsIAddrBookSession.h"
#include "nsAbBaseCID.h"
#include "nsIRDFResource.h"
#include "nsIRDFService.h"
#include "nsRDFCID.h"
#include "nsIDirectoryService.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsXPIDLString.h"
#include "nsReadableUtils.h"
#include "nsEscape.h"
#include "nsSyncDecoderRing.h"
#include "plstr.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"
#include "nsTextFormatter.h"
#include "nsIStringBundle.h"
#include "nsMsgI18N.h"
#include "nsIScriptGlobalObject.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDocShellTreeNode.h"
#include "nsIPrompt.h"
#include "nsIWindowWatcher.h"

//#define DEBUG_OFFLINE_TEST 1  // Take server response from a file.

PRLogModuleInfo *ABSYNC;  // Logging module

/* Implementation file */
NS_IMPL_ISUPPORTS1(nsAbSync, nsIAbSync)

nsAbSync::nsAbSync()
{
  NS_INIT_ISUPPORTS();

  // For listener array stuff...
  mListenerArrayCount = 0;
  mListenerArray = nsnull;
  mStringBundle = nsnull;
  mRootDocShell = nsnull;
  mUserName = nsnull;

  InternalInit();
  InitSchemaColumns();

  // Init logging module.
  if (!ABSYNC)
    ABSYNC = PR_NewLogModule("ABSYNC");
}

void
nsAbSync::InternalInit()
{
  /* member initializers and constructor code */
  mCurrentState = nsIAbSyncState::nsIAbSyncIdle;
  mTransactionID = 100;
  mPostEngine = nsnull;

  mAbSyncPort = 5000;
  mAbSyncAddressBook = nsnull;
  mAbSyncAddressBookFileName = nsnull;
  mHistoryFile = nsnull;
  mListHistoryFile = nsnull;
  mOldSyncMapingTable = nsnull;
  mNewSyncMapingTable = nsnull;
  mNewServerTable = nsnull;
  mListOldSyncMapingTable = nsnull;
  mListNewSyncMapingTable = nsnull;
  mListNewServerTable = nsnull;

  mLastChangeNum = 1;

  mLocale.Assign(NS_LITERAL_STRING(""));
  mDeletedRecordTags = nsnull;
  mDeletedRecordValues = nsnull;
  mNewRecordTags = nsnull;
  mNewRecordValues = nsnull;

  mPhoneTypes = nsnull;
  mPhoneValues = nsnull;

  mLockFile = nsnull;
  mLastSyncFailed = PR_FALSE;

  mCrashTableSize = 0;
  mCrashTable = nsnull;
}

nsresult
nsAbSync::CleanServerTable(nsVoidArray *aArray)
{
  if (!aArray)
    return NS_OK;

  for (PRInt32 i=0; i<aArray->Count(); i++)
  {
    syncMappingRecord *tRec = (syncMappingRecord *)aArray->ElementAt(i);
    if (!tRec)
      continue;

    nsCRT::free((char *)tRec);
  }

  delete aArray;
  return NS_OK;
}

nsresult nsAbSync::CleanListServerTable(nsVoidArray *aArray)
{
  if (!aArray)
    return NS_OK;

  for (PRInt32 i=0; i<aArray->Count(); i++)
  {
    syncListMappingRecord *tRec = (syncListMappingRecord *)aArray->ElementAt(i);
    if (!tRec)
      continue;

    nsCRT::free((char *)tRec);
  }

  delete aArray;
  return NS_OK;
}
nsresult
nsAbSync::InternalCleanup(nsresult aResult)
{
  /* cleanup code */
  DeleteListeners();

  PR_FREEIF(mAbSyncAddressBook);
  PR_FREEIF(mAbSyncAddressBookFileName);

  PR_FREEIF(mOldSyncMapingTable);
  PR_FREEIF(mNewSyncMapingTable);

  PR_FREEIF(mListOldSyncMapingTable);
  PR_FREEIF(mListNewSyncMapingTable);

  PR_FREEIF(mCrashTable);

  CleanServerTable(mNewServerTable);
  CleanListServerTable(mListNewServerTable);

  if (mHistoryFile)
    mHistoryFile->CloseStream();
  if (mListHistoryFile)
    mListHistoryFile->CloseStream();

  if (mLockFile)
  {
    mLockFile->CloseStream();

    if (NS_SUCCEEDED(aResult))
      mLockFile->Delete(PR_FALSE);
  }

  if (mDeletedRecordTags)
  {
    delete mDeletedRecordTags;
    mDeletedRecordTags = nsnull;
  }

  if (mDeletedRecordValues)
  {
    delete mDeletedRecordValues;
    mDeletedRecordValues = nsnull;
  }

  if (mNewRecordTags)
  {
    delete mNewRecordTags;
    mNewRecordTags = nsnull;
  }

  if (mNewRecordValues)
  {
    delete mNewRecordValues;
    mNewRecordValues = nsnull;
  }

  if (mPhoneTypes)
  {
    delete mPhoneTypes;
    mPhoneTypes = nsnull;
  }

  if (mPhoneValues)
  {
    delete mPhoneValues;
    mPhoneValues = nsnull;
  }

  return NS_OK;
}

nsAbSync::~nsAbSync()
{
  if (mPostEngine)
    mPostEngine->RemovePostListener((nsIAbSyncPostListener *)this);

  InternalCleanup(NS_ERROR_FAILURE);
}

NS_IMETHODIMP
nsAbSync::InitSchemaColumns()
{
  // First, setup the local address book fields...
  mSchemaMappingList[0].abField = kFirstNameColumn;
  mSchemaMappingList[1].abField = kLastNameColumn;
  mSchemaMappingList[2].abField = kDisplayNameColumn;
  mSchemaMappingList[3].abField = kNicknameColumn;
  mSchemaMappingList[4].abField = kPriEmailColumn;
  mSchemaMappingList[5].abField = k2ndEmailColumn;
  mSchemaMappingList[6].abField = kPreferMailFormatColumn;
  mSchemaMappingList[7].abField = kWorkPhoneColumn;
  mSchemaMappingList[8].abField = kHomePhoneColumn;
  mSchemaMappingList[9].abField = kFaxColumn;
  mSchemaMappingList[10].abField = kPagerColumn;
  mSchemaMappingList[11].abField = kCellularColumn;
  mSchemaMappingList[12].abField = kHomeAddressColumn;
  mSchemaMappingList[13].abField = kHomeAddress2Column;
  mSchemaMappingList[14].abField = kHomeCityColumn;
  mSchemaMappingList[15].abField = kHomeStateColumn;
  mSchemaMappingList[16].abField = kHomeZipCodeColumn;
  mSchemaMappingList[17].abField = kHomeCountryColumn;
  mSchemaMappingList[18].abField = kWorkAddressColumn;
  mSchemaMappingList[19].abField = kWorkAddress2Column;
  mSchemaMappingList[20].abField = kWorkCityColumn;
  mSchemaMappingList[21].abField = kWorkStateColumn;
  mSchemaMappingList[22].abField = kWorkZipCodeColumn;
  mSchemaMappingList[23].abField = kWorkCountryColumn;
  mSchemaMappingList[24].abField = kJobTitleColumn;
  mSchemaMappingList[25].abField = kDepartmentColumn;
  mSchemaMappingList[26].abField = kCompanyColumn;
  mSchemaMappingList[27].abField = kAnniversaryMonthColumn; // Don't change the mm, dd, yy ordering here
  mSchemaMappingList[28].abField = kAnniversaryDayColumn;   // since GenerateProtocolForCard() depends on it.
  mSchemaMappingList[29].abField = kAnniversaryYearColumn;
  mSchemaMappingList[30].abField = kSpouseNameColumn;
  mSchemaMappingList[31].abField = kFamilyNameColumn;
  mSchemaMappingList[32].abField = kDefaultAddressColumn;
  mSchemaMappingList[33].abField = kCategoryColumn;
  mSchemaMappingList[34].abField = kWebPage1Column;
  mSchemaMappingList[35].abField = kWebPage2Column;
  mSchemaMappingList[36].abField = kBirthMonthColumn;       // Don't change the mm, dd, yy ordering here
  mSchemaMappingList[37].abField = kBirthDayColumn;         // since GenerateProtocolForCard() depends on it.
  mSchemaMappingList[38].abField = kBirthYearColumn;
  mSchemaMappingList[39].abField = kCustom1Column;
  mSchemaMappingList[40].abField = kCustom2Column;
  mSchemaMappingList[41].abField = kCustom3Column;
  mSchemaMappingList[42].abField = kCustom4Column;
  mSchemaMappingList[43].abField = kNotesColumn;
  mSchemaMappingList[44].abField = kLastModifiedDateColumn;
  mSchemaMappingList[45].abField = kDefaultEmailColumn;
  mSchemaMappingList[46].abField = kAimScreenNameColumn;

  // Now setup the server fields...
  mSchemaMappingList[0].serverField = kServerFirstNameColumn;
  mSchemaMappingList[1].serverField = kServerLastNameColumn;
  mSchemaMappingList[2].serverField = kServerDisplayNameColumn;
  mSchemaMappingList[3].serverField = kServerNicknameColumn;
  mSchemaMappingList[4].serverField = kServerPriEmailColumn;
  mSchemaMappingList[5].serverField = kServer2ndEmailColumn;
  mSchemaMappingList[6].serverField = kServerPlainTextColumn;
  mSchemaMappingList[7].serverField = kServerWorkPhoneColumn;
  mSchemaMappingList[8].serverField = kServerHomePhoneColumn;
  mSchemaMappingList[9].serverField = kServerFaxColumn;
  mSchemaMappingList[10].serverField = kServerPagerColumn;
  mSchemaMappingList[11].serverField = kServerCellularColumn;
  mSchemaMappingList[12].serverField = kServerHomeAddressColumn;
  mSchemaMappingList[13].serverField = kServerHomeAddress2Column;
  mSchemaMappingList[14].serverField = kServerHomeCityColumn;
  mSchemaMappingList[15].serverField = kServerHomeStateColumn;
  mSchemaMappingList[16].serverField = kServerHomeZipCodeColumn;
  mSchemaMappingList[17].serverField = kServerHomeCountryColumn;
  mSchemaMappingList[18].serverField = kServerWorkAddressColumn;
  mSchemaMappingList[19].serverField = kServerWorkAddress2Column;
  mSchemaMappingList[20].serverField = kServerWorkCityColumn;
  mSchemaMappingList[21].serverField = kServerWorkStateColumn;
  mSchemaMappingList[22].serverField = kServerWorkZipCodeColumn;
  mSchemaMappingList[23].serverField = kServerWorkCountryColumn;
  mSchemaMappingList[24].serverField = kServerJobTitleColumn;
  mSchemaMappingList[25].serverField = kServerDepartmentColumn;
  mSchemaMappingList[26].serverField = kServerCompanyColumn;
  mSchemaMappingList[27].serverField = kServerAnniversaryMonthColumn; // Don't change the mm, dd, yy ordering here
  mSchemaMappingList[28].serverField = kServerAnniversaryDayColumn;   // since GenerateProtocolForCard() depends on it.
  mSchemaMappingList[29].serverField = kServerAnniversaryYearColumn;
  mSchemaMappingList[30].serverField = kServerSpouseNameColumn;
  mSchemaMappingList[31].serverField = kServerFamilyNameColumn;
  mSchemaMappingList[32].serverField = kServerDefaultAddressColumn;
  mSchemaMappingList[33].serverField = kServerCategoryColumn;
  mSchemaMappingList[34].serverField = kServerWebPage1Column;
  mSchemaMappingList[35].serverField = kServerWebPage2Column;
  mSchemaMappingList[36].serverField = kServerBirthMonthColumn;       // Don't change the mm, dd, yy ordering here
  mSchemaMappingList[37].serverField = kServerBirthDayColumn;         // since GenerateProtocolForCard() depends on it.
  mSchemaMappingList[38].serverField = kServerBirthYearColumn;
  mSchemaMappingList[39].serverField = kServerCustom1Column;
  mSchemaMappingList[40].serverField = kServerCustom2Column;
  mSchemaMappingList[41].serverField = kServerCustom3Column;
  mSchemaMappingList[42].serverField = kServerCustom4Column;
  mSchemaMappingList[43].serverField = kServerNotesColumn;
  mSchemaMappingList[44].serverField = kServerLastModifiedDateColumn;
  mSchemaMappingList[45].serverField = kServerDefaultEmailColumn;
  mSchemaMappingList[46].serverField = kServerAimScreenNameColumn;

  return NS_OK;
}

nsresult
nsAbSync::DisplayErrorMessage(const PRUnichar * msg)
{
  nsresult rv = NS_OK;

  if ((!msg) || (!*msg))
    return NS_ERROR_INVALID_ARG;

  if (mRootDocShell)
  {
    nsCOMPtr<nsIPrompt> dialog;
    dialog = do_GetInterface(mRootDocShell, &rv);
    if (dialog)
    {
      rv = dialog->Alert(nsnull, msg);
      rv = NS_OK;
    }
  }
  else
    rv = NS_ERROR_NULL_POINTER;

  // If we failed before, fall back to the non-parented modal dialog
  if (NS_FAILED(rv))
  {
    nsCOMPtr<nsIPrompt> dialog;
    nsCOMPtr<nsIWindowWatcher> wwatch(do_GetService("@mozilla.org/embedcomp/window-watcher;1"));
    if (wwatch)
      wwatch->GetNewPrompter(0, getter_AddRefs(dialog));

    if (!dialog)
      return NS_ERROR_FAILURE;
    rv = dialog->Alert(nsnull, msg);
  }

  return rv;
}

nsresult
nsAbSync::SetDOMWindow(nsIDOMWindowInternal *aWindow)
{
	NS_ENSURE_ARG_POINTER(aWindow);

  nsresult rv = NS_OK;
  
  nsCOMPtr<nsIScriptGlobalObject> globalScript(do_QueryInterface((nsISupports *)aWindow));
  nsCOMPtr<nsIDocShell> docShell;
  if (globalScript)
    globalScript->GetDocShell(getter_AddRefs(docShell));
  
  nsCOMPtr<nsIDocShellTreeItem> docShellAsItem(do_QueryInterface(docShell));  
  if(docShellAsItem)
  {
    nsCOMPtr<nsIDocShellTreeItem> rootAsItem;
    docShellAsItem->GetSameTypeRootTreeItem(getter_AddRefs(rootAsItem));
    
    nsCOMPtr<nsIDocShellTreeNode> rootAsNode(do_QueryInterface(rootAsItem));
    nsCOMPtr<nsIDocShell> temp_docShell(do_QueryInterface(rootAsItem));
    mRootDocShell = temp_docShell;
  }

  return rv;
}

/* void SetAbSyncUser (in string aUser); */
NS_IMETHODIMP 
nsAbSync::SetAbSyncUser(const char *aUser)
{
  if (aUser)
    mUserName = nsCRT::strdup(aUser);
  return NS_OK;
}

/* void AddSyncListener (in nsIAbSyncListener aListener); */
NS_IMETHODIMP nsAbSync::AddSyncListener(nsIAbSyncListener *aListener)
{
  if ( (mListenerArrayCount > 0) || mListenerArray )
  {
    ++mListenerArrayCount;
    mListenerArray = (nsIAbSyncListener **) 
                  PR_Realloc(*mListenerArray, sizeof(nsIAbSyncListener *) * mListenerArrayCount);
    if (!mListenerArray)
      return NS_ERROR_OUT_OF_MEMORY;
    else
    {
      mListenerArray[mListenerArrayCount - 1] = aListener;
      return NS_OK;
    }
  }
  else
  {
    mListenerArrayCount = 1;
    mListenerArray = (nsIAbSyncListener **) PR_Malloc(sizeof(nsIAbSyncListener *) * mListenerArrayCount);
    if (!mListenerArray)
      return NS_ERROR_OUT_OF_MEMORY;

    memset(mListenerArray, 0, (sizeof(nsIAbSyncListener *) * mListenerArrayCount));
  
    mListenerArray[0] = aListener;
    NS_ADDREF(mListenerArray[0]);
    return NS_OK;
  }
}

/* void RemoveSyncListener (in nsIAbSyncListener aListener); */
NS_IMETHODIMP nsAbSync::RemoveSyncListener(nsIAbSyncListener *aListener)
{
  PRInt32 i;
  for (i=0; i<mListenerArrayCount; i++)
    if (mListenerArray[i] == aListener)
    {
      NS_RELEASE(mListenerArray[i]);
      mListenerArray[i] = nsnull;
      return NS_OK;
    }

  return NS_ERROR_INVALID_ARG;
}

nsresult
nsAbSync::DeleteListeners()
{
  if ( (mListenerArray) && (*mListenerArray) )
  {
    PRInt32 i;
    for (i=0; i<mListenerArrayCount; i++)
    {
      NS_RELEASE(mListenerArray[i]);
    }
    
    PR_FREEIF(mListenerArray);
  }

  mListenerArrayCount = 0;
  return NS_OK;
}

nsresult
nsAbSync::NotifyListenersOnStartAuthOperation(void)
{
  PRInt32 i;
  for (i=0; i<mListenerArrayCount; i++)
    if (mListenerArray[i] != nsnull)
      mListenerArray[i]->OnStartAuthOperation();

  return NS_OK;
}

nsresult
nsAbSync::NotifyListenersOnStopAuthOperation(nsresult aStatus, const PRUnichar *aMsg, const char *aCookie)
{
  PRInt32 i;
  for (i=0; i<mListenerArrayCount; i++)
    if (mListenerArray[i] != nsnull)
      mListenerArray[i]->OnStopAuthOperation(aStatus, aMsg, aCookie);

  return NS_OK;
}

nsresult
nsAbSync::NotifyListenersOnStartSync(PRInt32 aTransactionID, const PRUint32 aMsgSize)
{
  PRInt32 i;
  for (i=0; i<mListenerArrayCount; i++)
    if (mListenerArray[i] != nsnull)
      mListenerArray[i]->OnStartOperation(aTransactionID, aMsgSize);

  return NS_OK;
}

nsresult
nsAbSync::NotifyListenersOnProgress(PRInt32 aTransactionID, PRUint32 aProgress, PRUint32 aProgressMax)
{
  PRInt32 i;
  for (i=0; i<mListenerArrayCount; i++)
    if (mListenerArray[i] != nsnull)
      mListenerArray[i]->OnProgress(aTransactionID, aProgress, aProgressMax);

  return NS_OK;
}

nsresult
nsAbSync::NotifyListenersOnStatus(PRInt32 aTransactionID, const PRUnichar *aMsg)
{
  PRInt32 i;
  for (i=0; i<mListenerArrayCount; i++)
    if (mListenerArray[i] != nsnull)
      mListenerArray[i]->OnStatus(aTransactionID, aMsg);

  return NS_OK;
}

nsresult
nsAbSync::NotifyListenersOnStopSync(PRInt32 aTransactionID, nsresult aStatus, 
                                    const PRUnichar *aMsg)
{
  PRInt32 i;
  for (i=0; i<mListenerArrayCount; i++)
    if (mListenerArray[i] != nsnull)
      mListenerArray[i]->OnStopOperation(aTransactionID, aStatus, aMsg);

  return NS_OK;
}


/**
  * Notify the observer that the AB Sync Authorization operation has begun. 
  *
  */
NS_IMETHODIMP 
nsAbSync::OnStartAuthOperation(void)
{
  NotifyListenersOnStartAuthOperation();
  return NS_OK;
}

/**
   * Notify the observer that the AB Sync operation has been completed.  
   * 
   * This method is called regardless of whether the the operation was 
   * successful.
   * 
   *  aTransactionID    - the ID for this particular request
   *  aStatus           - Status code for the sync request
   *  aMsg              - A text string describing the error (if any).
   *  aCookie           - hmmm...cooookies!
   */
NS_IMETHODIMP 
nsAbSync::OnStopAuthOperation(nsresult aStatus, const PRUnichar *aMsg, const char *aCookie)
{
  NotifyListenersOnStopAuthOperation(aStatus, aMsg, aCookie);
  return NS_OK;
}

/* void OnStartOperation (in PRInt32 aTransactionID, in PRUint32 aMsgSize); */
NS_IMETHODIMP nsAbSync::OnStartOperation(PRInt32 aTransactionID, PRUint32 aMsgSize)
{
  NotifyListenersOnStartSync(aTransactionID, aMsgSize);
  return NS_OK;
}

/* void OnProgress (in PRInt32 aTransactionID, in PRUint32 aProgress, in PRUint32 aProgressMax); */
NS_IMETHODIMP nsAbSync::OnProgress(PRInt32 aTransactionID, PRUint32 aProgress, PRUint32 aProgressMax)
{
  NotifyListenersOnProgress(aTransactionID, aProgress, aProgressMax);
  return NS_OK;
}

/* void OnStatus (in PRInt32 aTransactionID, in wstring aMsg); */
NS_IMETHODIMP nsAbSync::OnStatus(PRInt32 aTransactionID, const PRUnichar *aMsg)
{
  NotifyListenersOnStatus(aTransactionID, aMsg);
  return NS_OK;
}

/* void OnStopOperation (in PRInt32 aTransactionID, in nsresult aStatus, in wstring aMsg, out string aProtocolResponse); */
NS_IMETHODIMP nsAbSync::OnStopOperation(PRInt32 aTransactionID, nsresult aStatus, const PRUnichar *aMsg, const char *aProtocolResponse)
{
  nsresult    rv = aStatus;

  //
  // Now, figure out what the server told us to do with the sync operation.
  //
  if ( (aProtocolResponse) && (NS_SUCCEEDED(aStatus)) )
    rv = ProcessServerResponse(aProtocolResponse);

  NotifyListenersOnStopSync(aTransactionID, rv, aMsg);
  InternalCleanup(aStatus);

  mCurrentState = nsIAbSyncState::nsIAbSyncIdle;

#ifdef DEBUG_ABSYNC
  printf("ABSYNC: OnStopOperation: Status = %d\n", aStatus);
#endif
  return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////////////
//
// This is the implementation of the actual sync logic. 
//
////////////////////////////////////////////////////////////////////////////////////////

/* PRInt32 GetCurrentState (); */
NS_IMETHODIMP nsAbSync::GetCurrentState(PRInt32 *_retval)
{
  *_retval = mCurrentState;
  return NS_OK;
}

/* void CancelAbSync (); */
NS_IMETHODIMP nsAbSync::CancelAbSync()
{
  if (!mPostEngine)
    return NS_ERROR_FAILURE;

  return mPostEngine->CancelAbSync();
}

/* void PerformAbSync (out PRInt32 aTransactionID); */
NS_IMETHODIMP nsAbSync::PerformAbSync(nsIDOMWindowInternal *aDOMWindow, PRInt32 *aTransactionID)
{
  nsresult      rv;
  char          *protocolRequest = nsnull;
  char          *prefixStr = nsnull;
  char          *clientIDStr = nsnull;

  // We'll need later...
  SetDOMWindow(aDOMWindow);

  // If we are already running...don't let anything new start...
  if (mCurrentState != nsIAbSyncState::nsIAbSyncIdle)
    return NS_ERROR_FAILURE;

  // Make sure everything is set back to NULL's
  InternalInit();

  // Ok, now get the server/port number we should be hitting with 
  // this request as well as the local address book we will be 
  // syncing with...
  //
  nsCOMPtr<nsIPref> prefs(do_GetService(NS_PREF_CONTRACTID, &rv)); 
  NS_ENSURE_SUCCESS(rv, rv);

  prefs->CopyCharPref("mail.absync.address_book",     &mAbSyncAddressBook);
  prefs->GetIntPref  ("mail.absync.last_change",      &mLastChangeNum);
  if (NS_FAILED(prefs->GetIntPref("mail.absync.port", &mAbSyncPort)))
    mAbSyncPort = 5000;

  // More sanity...
  if (mLastChangeNum == 0)
    mLastChangeNum = 1;

  // Get the string arrays setup for the phone numbers...
  mPhoneTypes = new nsStringArray();
  mPhoneValues = new nsStringArray();

  // Ok, we need to see if a particular address book was in the prefs
  // If not, then we will use the default, but if there was one specified, 
  // we need to do a prefs lookup and get the file name of interest
  // The pref format is: user_pref("ldap_2.servers.SherrysAddressBook.filename", "abook-1.mab");
  //
  if ( (mAbSyncAddressBook) && (*mAbSyncAddressBook) )
  {
    nsCString prefId("ldap_2.servers.");
    prefId.Append(mAbSyncAddressBook);
    prefId.Append(".filename");

    prefs->CopyCharPref(prefId.get(), &mAbSyncAddressBookFileName);
  }

  mTransactionID++;

  // First, analyze the situation locally and build up a string that we
  // will need to post.
  //
  rv = AnalyzeTheLocalAddressBook();
  if (NS_FAILED(rv))
    goto EarlyExit;

#if DEBUG_OFFLINE_TEST // this is added for loop back test
  rv = ProcessServerResponse("a test response");
  return NS_ERROR_FAILURE;
#endif

  // We can keep this object around for reuse...
  if (!mPostEngine)
  {
    rv = nsComponentManager::CreateInstance(NS_ABSYNC_POST_ENGINE_CONTRACTID, NULL, NS_GET_IID(nsIAbSyncPostEngine), getter_AddRefs(mPostEngine));
    NS_ENSURE_SUCCESS(rv, rv);

    mPostEngine->AddPostListener((nsIAbSyncPostListener *)this);
  }

  rv = mPostEngine->BuildMojoString(mRootDocShell, &clientIDStr);
  if (NS_FAILED(rv) || (!clientIDStr))
    goto EarlyExit;

  if (mPostString.IsEmpty())
    prefixStr = PR_smprintf("last=%u&protocol=%s&client=%s&ver=%s", mLastChangeNum, ABSYNC_PROTOCOL, clientIDStr, ABSYNC_VERSION);
  else
    prefixStr = PR_smprintf("last=%u&protocol=%s&client=%s&ver=%s&", mLastChangeNum, ABSYNC_PROTOCOL, clientIDStr, ABSYNC_VERSION);

  if (!prefixStr)
  {
    rv = NS_ERROR_OUT_OF_MEMORY;
    OnStopOperation(mTransactionID, NS_ERROR_OUT_OF_MEMORY, nsnull, nsnull);
    goto EarlyExit;
  }

  mPostString.Insert(NS_ConvertASCIItoUCS2(prefixStr), 0);
  nsCRT::free(prefixStr);

  protocolRequest = ToNewCString(mPostString);
  if (!protocolRequest)
    goto EarlyExit;

  // Ok, FIRE!
  rv = mPostEngine->SendAbRequest(nsnull, mAbSyncPort, protocolRequest, mTransactionID, mRootDocShell, mUserName);
  if (NS_SUCCEEDED(rv))
  {
    mCurrentState = nsIAbSyncState::nsIAbSyncRunning;
  }
  else
  {
    OnStopOperation(mTransactionID, rv, nsnull, nsnull);
    goto EarlyExit;
  }

EarlyExit:
  PR_FREEIF(protocolRequest);
  PR_FREEIF(clientIDStr);

  if (NS_FAILED(rv))
    InternalCleanup(rv);
  return rv;
}

NS_IMETHODIMP 
nsAbSync::OpenAB(char *aAbName, nsIAddrDatabase **aDatabase)
{
	if (!aDatabase)
    return NS_ERROR_FAILURE;

	nsresult rv = NS_OK;
	nsFileSpec* dbPath = nsnull;

	nsCOMPtr<nsIAddrBookSession> abSession = 
	         do_GetService(NS_ADDRBOOKSESSION_CONTRACTID, &rv); 
	if(NS_SUCCEEDED(rv))
		abSession->GetUserProfileDirectory(&dbPath);
	
	if (dbPath)
	{
    if (!aAbName)
      (*dbPath) += "abook.mab";
    else
      (*dbPath) += aAbName;

		nsCOMPtr<nsIAddrDatabase> addrDBFactory = do_GetService(NS_ADDRDATABASE_CONTRACTID, &rv);

		if (NS_SUCCEEDED(rv) && addrDBFactory)
			rv = addrDBFactory->Open(dbPath, PR_TRUE, aDatabase, PR_TRUE);
	}
  else
    rv = NS_ERROR_FAILURE;

  delete dbPath;

  return rv;
}

NS_IMETHODIMP    
nsAbSync::GenerateProtocolForList(nsIAbCard *aCard, PRBool aAddId, nsString &protLine)
{
  PRUnichar     *aName = nsnull;
  nsString      tProtLine;
	nsresult rv = NS_OK;

  protLine.Truncate();

  if (aAddId)
  {
    PRUint32    aKey;

    nsCOMPtr<nsIAbMDBCard> dbcard(do_QueryInterface(aCard, &rv)); 
    NS_ENSURE_SUCCESS(rv, rv);
    if (NS_FAILED(dbcard->GetKey(&aKey)))
      return NS_ERROR_FAILURE;

    char *tVal = PR_smprintf("%d", (aKey * -1));
    if (tVal)
    {
      tProtLine.Append(NS_LITERAL_STRING("%26cid%3D") + NS_ConvertASCIItoUCS2(tVal));
      nsCRT::free(tVal);
    }
  }
 
  if (NS_SUCCEEDED(aCard->GetCardValue(kDisplayNameColumn, &aName)) && (aName) && (*aName))
  {
    tProtLine.Append(NS_LITERAL_STRING("&") + NS_LITERAL_STRING("listname") + NS_LITERAL_STRING("="));
    AddValueToProtocolLine(aName, tProtLine);
    PR_FREEIF(aName);
  }

  if (!tProtLine.IsEmpty())
  {
    char *escData = nsEscape((char *)NS_ConvertUCS2toUTF8(tProtLine).get(), url_Path);
    if (escData)
      tProtLine = NS_ConvertASCIItoUCS2(escData);

    nsCRT::free(escData);
    protLine = tProtLine;
  }

  return NS_OK;
}

NS_IMETHODIMP    
nsAbSync::GenerateProtocolForCard(nsIAbCard *aCard, PRBool aAddId, nsString &protLine)
{
  PRUnichar     *aName = nsnull;
  nsString      tProtLine;
  PRInt32       phoneCount = 1;
  PRBool        foundPhone = PR_FALSE;
  const char    *phoneType;

  protLine.Truncate();

  if (aAddId)
  {
    PRUint32    aKey;
	  nsresult rv = NS_OK;

    nsCOMPtr<nsIAbMDBCard> dbcard(do_QueryInterface(aCard, &rv)); 
    NS_ENSURE_SUCCESS(rv, rv);
    if (NS_FAILED(dbcard->GetKey(&aKey)))
      return NS_ERROR_FAILURE;

#ifdef DEBUG_ABSYNC
  printf("ABSYNC: GENERATING PROTOCOL FOR CARD - Address Book Card Key: %d\n", aKey);
#endif

    char *tVal = PR_smprintf("%d", (aKey * -1));
    if (tVal)
    {
      tProtLine.Append(NS_LITERAL_STRING("%26cid%3D") + NS_ConvertASCIItoUCS2(tVal));
      nsCRT::free(tVal);
    }
  }

  nsString birthday, anniversary; // for birthday & anniversary
  for (PRInt32 i=0; i<kMaxColumns; i++)
  {
    if (NS_SUCCEEDED(aCard->GetCardValue(mSchemaMappingList[i].abField, &aName)) && (aName) && (*aName))
    {
      // These are unknown tags we are omitting...
      const nsAString& prefix =
        Substring(mSchemaMappingList[i].serverField, 0, 5);
      if (NS_LITERAL_STRING("OMIT:").Equals(prefix,
                                            nsCaseInsensitiveStringComparator()))
        continue;

      // Handle birthday & anniversary dates here
      if (!nsCRT::strncasecmp(mSchemaMappingList[i].abField, kBirthYearColumn, strlen(kBirthYearColumn)) ||
          !nsCRT::strncasecmp(mSchemaMappingList[i].abField, kBirthDayColumn, strlen(kBirthDayColumn)))
      {
        birthday.Append(NS_LITERAL_STRING("/"));
        birthday.Append(aName);
        continue;
      }
      else if (!nsCRT::strncasecmp(mSchemaMappingList[i].abField, kBirthMonthColumn, strlen(kBirthMonthColumn)))
      {
        birthday.Assign(aName);
        continue;
      }
      else if (!nsCRT::strncasecmp(mSchemaMappingList[i].abField, kAnniversaryYearColumn, strlen(kAnniversaryYearColumn)) ||
               !nsCRT::strncasecmp(mSchemaMappingList[i].abField, kAnniversaryDayColumn, strlen(kAnniversaryDayColumn)))
      {
        anniversary.Append(NS_LITERAL_STRING("/"));
        anniversary.Append(aName);
        continue;
      }
      else if (!nsCRT::strncasecmp(mSchemaMappingList[i].abField, kAnniversaryMonthColumn, strlen(kAnniversaryMonthColumn)))
      {
        anniversary.Assign(aName);
        continue;
      }

      // Reset this flag...
      foundPhone = PR_FALSE;
      // If this is a phone number, we have to special case this because
      // phone #'s are handled differently than all other tags...sigh!
      //
      if (!nsCRT::strncasecmp(mSchemaMappingList[i].abField, kWorkPhoneColumn, strlen(kWorkPhoneColumn)))
      {
        foundPhone = PR_TRUE;
        phoneType = ABSYNC_WORK_PHONE_TYPE;
      }
      else if (!nsCRT::strncasecmp(mSchemaMappingList[i].abField, kHomePhoneColumn, strlen(kHomePhoneColumn)))
      {
        foundPhone = PR_TRUE;
        phoneType = ABSYNC_HOME_PHONE_TYPE;
      }
      else if (!nsCRT::strncasecmp(mSchemaMappingList[i].abField, kFaxColumn, strlen(kFaxColumn)))
      {
        foundPhone = PR_TRUE;
        phoneType = ABSYNC_FAX_PHONE_TYPE;
      }
      else if (!nsCRT::strncasecmp(mSchemaMappingList[i].abField, kPagerColumn, strlen(kPagerColumn)))
      {
        foundPhone = PR_TRUE;
        phoneType = ABSYNC_PAGER_PHONE_TYPE;
      }
      else if (!nsCRT::strncasecmp(mSchemaMappingList[i].abField, kCellularColumn, strlen(kCellularColumn)))
      {
        foundPhone = PR_TRUE;
        phoneType = ABSYNC_CELL_PHONE_TYPE;
      }

      if (foundPhone)
      {
        char *pVal = PR_smprintf("phone%d", phoneCount);
        if (pVal)
        {
          tProtLine.Append(NS_LITERAL_STRING("&") + NS_ConvertASCIItoUCS2(pVal) + NS_LITERAL_STRING("="));

          AddValueToProtocolLine(aName, tProtLine);

          tProtLine.Append(NS_LITERAL_STRING("&") + NS_ConvertASCIItoUCS2(pVal) + 
                           NS_LITERAL_STRING("_type=") + NS_ConvertASCIItoUCS2(phoneType));
          PR_FREEIF(pVal);
          phoneCount++;
        }
      }
      else    // Good ole' normal tag...
      {
        tProtLine.Append(NS_LITERAL_STRING("&") + mSchemaMappingList[i].serverField + NS_LITERAL_STRING("="));
        AddValueToProtocolLine(aName, tProtLine);
      }

      PR_FREEIF(aName);
    }
  }

  // Now add birthday andd/or anniversary dates assembled earlier.
  if (!birthday.IsEmpty())
  {
    tProtLine.Append(NS_LITERAL_STRING("&") + kServerBirthdayColumn + NS_LITERAL_STRING("="));
    AddValueToProtocolLine(birthday.get(), tProtLine);
  }
  if (!anniversary.IsEmpty())
  {
    tProtLine.Append(NS_LITERAL_STRING("&") + kServerAnniversaryColumn + NS_LITERAL_STRING("="));
    AddValueToProtocolLine(anniversary.get(), tProtLine);
  }

  if (!tProtLine.IsEmpty())
  {
    // Now, check if this is that flag for the plain text email selection...if so,
    // then tack this information on as well...
    PRUint32 format = nsIAbPreferMailFormat::unknown;
    if (NS_SUCCEEDED(aCard->GetPreferMailFormat(&format)))
    {
      if (format != nsIAbPreferMailFormat::html)
        aName = ToNewUnicode(NS_LITERAL_STRING("0"));
      else  
        aName = ToNewUnicode(NS_LITERAL_STRING("1"));
    
      // Just some sanity...
      if (aName)
      {
        tProtLine.Append(NS_LITERAL_STRING("&") + kServerPlainTextColumn + NS_LITERAL_STRING("="));
        AddValueToProtocolLine(aName, tProtLine);
      
        PR_FREEIF(aName);
      }
    }

    char *escData = nsEscape((char *)NS_ConvertUCS2toUTF8(tProtLine).get(), url_Path);
    if (escData)
      tProtLine = NS_ConvertASCIItoUCS2(escData);

    nsCRT::free(escData);
    protLine = tProtLine;
  }

  return NS_OK;
}

nsresult nsAbSync::AddValueToProtocolLine(const PRUnichar *value, nsString &protocolLine)
{
  // Escape the value string.
  char *escapedStr = nsEscape((char *)NS_ConvertUCS2toUTF8(value).get(), url_Path);
  if (escapedStr)
  {
    protocolLine.Append(NS_ConvertASCIItoUCS2(escapedStr));
    PR_FREEIF(escapedStr);
  }
  else
    protocolLine.Append(value);

  return NS_OK;
}

long nsAbSync::GetCRC(char *str)
{
  cm_t      crcModel;
  p_cm_t    p = &crcModel;
  
  /*
    Name   : Crc-32
    Width  : 32
    Poly   : 04C11DB7
    Init   : FFFFFFFF
    RefIn  : True
    RefOut : True
    XorOut : FFFFFFFF
    Check  : CBF43926
  */
  p->cm_width = 32;
  p->cm_poly = 0x4C11DB7;
  p->cm_init = 0xFFFFFFFF;
  p->cm_refin = PR_TRUE;
  p->cm_refot = PR_TRUE;
  p->cm_xorot = 0xFFFFFFFF;

  char *pChar = str;
  cm_ini( p);
  for (PRUint32 i = 0; i < strlen(str); i++, pChar++)
    cm_nxt( p, *pChar);
  
  return( cm_crc( p) );
}

PRBool          
nsAbSync::ThisCardHasChanged(nsIAbCard *aCard, syncMappingRecord *newSyncRecord, nsString &protLine, PRBool cardIsUser)
{
  syncMappingRecord   *historyRecord = nsnull;
  PRUint32            counter = 0;
  nsString            tempProtocolLine;

  // First, null out the protocol return line
  protLine.Truncate();

  // Use the localID for this entry to lookup the old history record in the 
  // cached array
  //
  if (mOldSyncMapingTable)
  {
    while (counter < mOldTableCount)
    {
      if (mOldSyncMapingTable[counter].localID == newSyncRecord->localID)
      {
        historyRecord = &(mOldSyncMapingTable[counter]);
        break;
      }

      counter++;
    }
  }

  //
  // Now, build up the compare protocol line for this entry.
  //
  if (cardIsUser)
  {
    if (NS_FAILED(GenerateProtocolForCard(aCard, PR_FALSE, tempProtocolLine)))
      return PR_FALSE;
  }
  else
    if (NS_FAILED(GenerateProtocolForList(aCard, PR_FALSE, tempProtocolLine)))
      return PR_FALSE;

  if (tempProtocolLine.IsEmpty())
    return PR_FALSE;

  // Get the CRC for this temp entry line...
  newSyncRecord->CRC = GetCRC((char *)NS_ConvertUCS2toUTF8(tempProtocolLine).get());

  // If we have a history record, we need to carry over old items!
  if (historyRecord)
  {
    newSyncRecord->serverID = historyRecord->serverID;
    historyRecord->flags |= SYNC_PROCESSED;
  }

  // If this is a record that has changed from comparing CRC's, then
  // when can generate a line that should be tacked on to the output
  // going up to the server
  //
  if ( (!historyRecord) || (historyRecord->CRC != newSyncRecord->CRC) )
  {      
    PRUint32    aKey;

    if (!historyRecord)
    {
      newSyncRecord->flags |= SYNC_ADD;

      nsresult rv = NS_OK;
      nsCOMPtr<nsIAbMDBCard> dbcard(do_QueryInterface(aCard, &rv)); 
      NS_ENSURE_SUCCESS(rv, rv);
      if (NS_FAILED(dbcard->GetKey(&aKey)))
        return PR_FALSE;
      
      // Ugh...this should never happen...BUT??
      if (aKey <= 0)
        return PR_FALSE;

      // Needs to be negative, so make it so!
      char *tVal = PR_smprintf("%d", (aKey * -1));
      if (tVal)
      {
        protLine.Append(NS_LITERAL_STRING("%26cid%3D") +
                        NS_ConvertASCIItoUCS2(tVal) +
                        tempProtocolLine);
        nsCRT::free(tVal);
      }
      else
        return PR_FALSE;
    }
    else
    {
      // If 'aCard' is a mailing list then use 'list_id' instead of 'id' for the protocol.
      newSyncRecord->flags |= SYNC_MODIFIED;

      char *tVal2 = PR_smprintf("%d", historyRecord->serverID);
      if (tVal2)
      {
        if (cardIsUser)
          protLine.Append(NS_LITERAL_STRING("%26id%3D") + NS_ConvertASCIItoUCS2(tVal2) + tempProtocolLine);
        else
          protLine.Append(NS_LITERAL_STRING("%26list_id%3D") + NS_ConvertASCIItoUCS2(tVal2) + tempProtocolLine);
        nsCRT::free(tVal2);
      }
      else
        return PR_FALSE;
    }

    return PR_TRUE;
  }
  else 
    return PR_FALSE; // This is the same record as before.

  return PR_FALSE;
}

char*
BuildSyncTimestamp(void)
{
  static char       result[75] = "";
  PRExplodedTime    now;
  char              buffer[128] = "";

  // Generate a time line in format of:  Sat Apr 18 20:01:49 1998
  //
  // Use PR_FormatTimeUSEnglish() to format the date in US English format,
  // then figure out what our local GMT offset is, and append it (since
  // PR_FormatTimeUSEnglish() can't do that.) Generate four digit years as
  // per RFC 1123 (superceding RFC 822.)
  //
  PR_ExplodeTime(PR_Now(), PR_LocalTimeParameters, &now);
	PR_FormatTimeUSEnglish(buffer, sizeof(buffer),
						   "%a %b %d %H:%M:%S %Y",
						   &now);
  
  // This value must be in ctime() format, with English abbreviations.
  // PL_strftime("... %c ...") is no good, because it is localized.
  //
  PL_strcpy(result, buffer);
  PL_strcpy(result + 24, CRLF);
  return result;
}

NS_IMETHODIMP
nsAbSync::AnalyzeAllRecords(nsIAddrDatabase *aDatabase, nsIAbDirectory *directory, PRBool analyzeUser)
{
  nsresult                rv = NS_OK;
  nsIEnumerator           *cardEnum = nsnull;
  nsCOMPtr<nsISupports>   obj = nsnull;
  PRBool                  exists = PR_FALSE;
  PRUint32                readCount = 0;
  PRInt32                 readSize = 0;
  PRUint32                workCounter = 0;
  PRUint32                listIndex = 0;
  PRUint32                numOfCards= 0;
  nsString                singleProtocolLine;

  // Init size vars...
  mOldTableCount = 0;
  mListOldTableCount = 0;
  mNewTableCount = 0;
  mListNewTableCount = 0;
  PR_FREEIF(mOldSyncMapingTable);
  PR_FREEIF(mNewSyncMapingTable);
  PR_FREEIF(mListOldSyncMapingTable);
  PR_FREEIF(mListNewSyncMapingTable);
  CleanServerTable(mNewServerTable);
  CleanListServerTable(mListNewServerTable);
  mCurrentPostRecord = 1;

#ifdef DEBUG_ABSYNC
  printf("ABSYNC: AnalyzeAllRecords:\n");
#endif

  //
  // First thing we need to do is open the absync.dat file
  // to compare against the last sync operation.
  // 
  // we want <profile>/absync.dat
  //

  nsCOMPtr<nsIFile> historyFile; 
  nsCOMPtr<nsIFile> listHistoryFile;
  nsCOMPtr<nsIFile> lockFile;
  
  rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR, getter_AddRefs(historyFile));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR, getter_AddRefs(listHistoryFile));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = historyFile->AppendNative(NS_LITERAL_CSTRING("absync.dat"));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = listHistoryFile->AppendNative(NS_LITERAL_CSTRING("absynclist.dat"));
  NS_ENSURE_SUCCESS(rv, rv);

  // TODO: Convert the rest of the code to use
  // nsIFile and avoid this conversion hack.
  do
  {
    rv = NS_NewFileSpecFromIFile(historyFile, getter_AddRefs(mHistoryFile));
    if (NS_FAILED(rv)) break;
    rv = NS_NewFileSpecFromIFile(listHistoryFile, getter_AddRefs(mListHistoryFile));
    if (NS_FAILED(rv)) break;
  }
  while (0);

  NS_ASSERTION(NS_SUCCEEDED(rv),"ab sync:  failed to specify history file");
  if (NS_FAILED(rv)) 
  {
    rv = NS_ERROR_FAILURE;
    goto GetOut;
  }

  // Now see if we actually have an old table to work with?
  // If not, this is the first sync operation.
  //
  //mOldSyncMapingTable = nsnull;
  //mNewSyncMapingTable = nsnull;
  
  // Do this here to be used in case of a crash recovery situation...
  rv = aDatabase->EnumerateCards(directory, &cardEnum);
  if (NS_FAILED(rv) || (!cardEnum))
  {
    rv = NS_ERROR_FAILURE;
    goto GetOut;
  }

  aDatabase->GetCardCount(&numOfCards);

  //
  // Now, lets get a lot smarter about failures that may happen during the sync
  // operation. If this happens, we should do all that is possible to prevent 
  // duplicate entries from getting stored in an address book. To that end, we
  // will create an "absync.lck" file that will only exist during the synchronization
  // operation. The only time the lock file should be on disk at this point is if the
  // last sync operation failed. If so, we should build a compare list of the current
  // address book and only insert entries that don't have matching CRC's
  //
  // Note: be very tolerant if any of this stuff fails ...
  
  rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR, getter_AddRefs(lockFile));
  if (NS_SUCCEEDED(rv)) 
  {
    rv = lockFile->AppendNative(NS_LITERAL_CSTRING("absync.lck"));
    if (NS_SUCCEEDED(rv)) 
    {
    
      // TODO: Convert the rest of the code to use
      // nsIFile and avoid this conversion hack.
      do
      {
        rv = NS_NewFileSpecFromIFile(lockFile, getter_AddRefs(mLockFile));
        if (NS_FAILED(rv)) break;
      }
      while (0);
      
      if (NS_SUCCEEDED(rv)) 
      {
        PRBool      tExists = PR_FALSE;

        mLockFile->Exists(&tExists);
        if (tExists)
        {
            // OK, the last operation must have failed. Try to recover.
            RecoverUserSyncRecords(numOfCards, cardEnum);

            // TODO: add code to do list recovery
        }
        else  // If here, create the lock file
        {
          if (NS_SUCCEEDED(mLockFile->OpenStreamForWriting()))
          {
            char      *tMsg = BuildSyncTimestamp();
            PRInt32   tWriteSize;

            mLockFile->Write(tMsg, strlen(tMsg), &tWriteSize);
            mLockFile->CloseStream();
          }
        }
      }
    }  
  }  

  // If the old user table exists then load it up!
  mHistoryFile->Exists(&exists);
  if (exists)
  {
    if (NS_FAILED(LoadUsersFromHistoryFile()))
    {
      Log("Loading users from history file", "FAILED!");
      goto GetOut;
    }
  }

  // If the old list table exists then load it up!
  mListHistoryFile->Exists(&exists);
  if (exists)
  {
    if (NS_FAILED(LoadListsFromHistoryFile()))
    {
        Log("Loading lists from history file", "FAILED!");
        goto GetOut;
    }
  }

  //
  // Now go through all the existing cards and store them in the NEW sync
  // mapping table (mNewSyncMapingTable). Comparing this table against the
  // history one we can tell if some records were added, removed or modified.
  //
  if (NS_FAILED(rv = InitUserSyncTable(numOfCards, cardEnum)))
    goto GetOut;

  // Now, init the list sync mapping table.
  if (NS_FAILED(rv = InitListSyncTable(cardEnum)))
    goto GetOut;

  rv = NS_OK;  
  workCounter =0;
  PRUint32 processCount;

  // We need to process users/cards first and then lists/groups.
  // so we use the loop control count to do this (1 = users).
  for (processCount=1; processCount<=2; processCount++)
  {
    cardEnum->First();
    do
    {
      if (NS_FAILED(cardEnum->CurrentItem(getter_AddRefs(obj))))
        break;
      else
      {
        nsCOMPtr<nsIAbCard> card;
        card = do_QueryInterface(obj, &rv);
        if ( NS_SUCCEEDED(rv) && (card) )
        {
          // If the card is not what we're asked to analyze then ignore it.
          PRBool isMailList = PR_FALSE;
          rv = card->GetIsMailList(&isMailList);
          if ( ((processCount==1) && isMailList) || ((processCount==2) && !isMailList) )
            continue;

          // First, we need to fill out the localID for this entry. This should
          // be the ID from the local database for this card entry
          //
          PRUint32    aKey;
          nsresult rv = NS_OK;
          nsCOMPtr<nsIAbMDBCard> dbcard(do_QueryInterface(card, &rv)); 
          if (NS_FAILED(rv) || !dbcard)
            continue;
          if (NS_FAILED(dbcard->GetKey(&aKey)))
            continue;

          // Ugh...this should never happen...BUT??
          if (aKey <= 0)
            continue;

          mNewSyncMapingTable[workCounter].localID = aKey;
          // Store proper flag in the table. This flag is used when processing the deleted records.
          // Since the cards were already removed from the addrbook there's no way to find out if
          // the deleted cards were normal cards, aol groups, or aol additional email addresses. We
          // can only rely on the stored flag in the history table.
          if (isMailList)
            mNewSyncMapingTable[workCounter].flags |= SYNC_IS_LIST;
          else
            mNewSyncMapingTable[workCounter].flags |= WhichCardType(card);

          if (ThisCardHasChanged(card, &(mNewSyncMapingTable[workCounter]), singleProtocolLine, (!isMailList)))
          {
            // For 'email address' type of cards we don't want to send servers any changes
            // because the corresponding cards don't exist on servers. May have to do the
            // same for AOL groups (because you can't change it on server side either).
            if (mNewSyncMapingTable[workCounter].flags & SYNC_IS_AOL_ADDITIONAL_EMAIL)
            {
              workCounter++;
              continue;
            }
            
            // If we get here, we should know if the card is a new one or a
            // modified one and then build the protocol line accordingly.
            //
            // Need the separator for multiple operations.
            if (!mPostString.IsEmpty())
              mPostString.Append(NS_LITERAL_STRING("&"));

            if (mNewSyncMapingTable[workCounter].flags & SYNC_ADD)
            {
#ifdef DEBUG_ABSYNC
  char *t = ToNewCString(singleProtocolLine);
  printf("ABSYNC: ADDING Card: %s\n", t);
  PR_FREEIF(t);
#endif
              mPostString.AppendInt(mCurrentPostRecord);
              mPostString.Append(NS_LITERAL_STRING("="));

              // Use the appropriate cmd for the new list/user.
              if (isMailList)
                mPostString.Append(NS_ConvertASCIItoUCS2(SYNC_ESCAPE_ADDLIST) + singleProtocolLine);
              else
                mPostString.Append(NS_ConvertASCIItoUCS2(SYNC_ESCAPE_ADDUSER) + singleProtocolLine);

              mCurrentPostRecord++;
            }
            else if (mNewSyncMapingTable[workCounter].flags & SYNC_MODIFIED)
            {
#ifdef DEBUG_ABSYNC
  char *t = ToNewCString(singleProtocolLine);
  printf("ABSYNC: MODIFYING Card: %s\n", t);
  PR_FREEIF(t);
#endif
              mPostString.AppendInt(mCurrentPostRecord);
              mPostString.Append(NS_LITERAL_STRING("="));

              // If the card is a mailing list then use SYNC_ESCAPE_MODLIST cmd for the protocol.
              if (isMailList)
                mPostString.Append(NS_ConvertASCIItoUCS2(SYNC_ESCAPE_MODLIST) + singleProtocolLine);
              else
                mPostString.Append(NS_ConvertASCIItoUCS2(SYNC_ESCAPE_MOD) + singleProtocolLine);

              mCurrentPostRecord++;
            }
          }

          // If it's a list then need to generate protocol string for new memebers 
          // or for members have been modified (ie, modified or deleted).
          if (isMailList)
          {
            singleProtocolLine.Truncate();
            if (mNewSyncMapingTable[workCounter].flags & SYNC_ADD)
              GenerateMemberProtocolForNewList(card, listIndex, singleProtocolLine);
            else
              CheckCurrentListForChangedMember(card, listIndex, singleProtocolLine);

            if (!singleProtocolLine.IsEmpty())
            {
              if (!mPostString.IsEmpty())
                mPostString.Append(NS_LITERAL_STRING("&"));
              mPostString.Append(singleProtocolLine);
            }
            listIndex++;
          }
        }
      }

      workCounter++;
    } while (NS_SUCCEEDED(cardEnum->Next()));
  }

  //
  // Go through the old history and see if there are records we need to
  // delete (the ones that are not marked PROCESSED in the history table).
  //
  CheckDeletedRecords();

GetOut:
  if (cardEnum)
    delete cardEnum;

  if (NS_FAILED(rv))
  {
    mOldTableCount = 0;
    mNewTableCount = 0;
    PR_FREEIF(mOldSyncMapingTable);
    PR_FREEIF(mNewSyncMapingTable);
    mListOldTableCount = 0;
    PR_FREEIF(mListOldSyncMapingTable);
  }

  // Log the data to be sent to servers.
  if (!mPostString.IsEmpty())
  {
    nsCAutoString str;
    str.AssignWithConversion(mPostString.get());
    char *unescapedStr = nsUnescape((char *)str.get());
    Log("  Protocol data sent to server", unescapedStr);
  }
  else
    Log("  Protocol data sent to server", "Nothing has changed!");

  // Ok, get out!
  return rv;
}

nsresult
nsAbSync::AnalyzeTheLocalAddressBook()
{
  // Time to build the mPostString 
  //
  nsresult        rv = NS_OK;
  nsIAddrDatabase *aDatabase = nsnull;
 
  // Get the address book entry
  nsCOMPtr <nsIRDFResource>     resource = nsnull;
  nsCOMPtr <nsIAbDirectory>     directory = nsnull;

  // Log current time
  Log("Analyzing local addressbook at", BuildSyncTimestamp());

  // Init to null...
  mPostString.Truncate();

  // Now, open the database...for now, make it the default
  rv = OpenAB(mAbSyncAddressBookFileName, &aDatabase);
  if (NS_FAILED(rv))
    return rv;

  // Get the RDF service...
  nsCOMPtr<nsIRDFService> rdfService = do_GetService("@mozilla.org/rdf/rdf-service;1", &rv);
  if (NS_FAILED(rv)) 
    goto EarlyExit;

  // this should not be hardcoded to abook.mab
  // this works for any address book...not sure why
  // absync on go againt abook.mab - candice
  rv = rdfService->GetResource(kPersonalAddressbookUri, getter_AddRefs(resource));
  if (NS_FAILED(rv)) 
    goto EarlyExit;
  
  // query interface 
  directory = do_QueryInterface(resource, &rv);
  if (NS_FAILED(rv)) 
    goto EarlyExit;

  // Ok, Now we need to analyze the records in the database against
  // the sync history file and see what has changed. If it has changed,
  // then we need to create the mPostString protocol stuff to make 
  // the changes
  //
  rv = AnalyzeAllRecords(aDatabase, directory, PR_TRUE);

EarlyExit:
  // Database is open...make sure to close it
  if (aDatabase)
  {
    aDatabase->Close(PR_TRUE);
  }
  NS_IF_RELEASE(aDatabase);

  Log("Finish analyzing local addressbook at", BuildSyncTimestamp());
  return rv;
}

nsresult
nsAbSync::PatchHistoryTableWithNewID(PRInt32 clientID, PRInt32 serverID, PRInt32 aMultiplier, ulong crc)
{
  for (PRUint32 i = 0; i < mNewTableCount; i++)
  {
    if (mNewSyncMapingTable[i].localID == (clientID * aMultiplier))
    {
#ifdef DEBUG_ABSYNC
  printf("ABSYNC: PATCHING History Table - Client: %d - Server: %d\n", clientID, serverID);
#endif

      mNewSyncMapingTable[i].serverID = serverID;
      // Patch CRC if it's not 0.
      if (crc)
        mNewSyncMapingTable[i].CRC = crc;
      return NS_OK;
    }
  }

  return NS_ERROR_FAILURE;
}
///////////////////////////////////////////////
// The following is for protocol parsing
///////////////////////////////////////////////
#define     SERVER_ERROR              "err "

#define     SERVER_OP_RETURN          "~op_return"
#define     SERVER_OP_RETURN_LOCALE   "dlocale="
#define     SERVER_OP_RETURN_RENAME   "op=ren"                       // example: op=ren&cid=-55&sid=88
#define     SERVER_OP_RETURN_MAILLIST_RENAME        "op=maillistRen" // example: op=maillistRen&cid=-57&sid=91
#define     SERVER_OP_RETURN_MAILLIST_MEMBER_RENAME "op=k2maillist"  // example: op=k2maillist&list_id=91&-55=88
#define     SERVER_OP_RETURN_CID      "cid="
#define     SERVER_OP_RETURN_SID      "sid="
#define     SERVER_OP_RETURN_LIST_ID  "list_id="

#define     SERVER_NEW_RECORDS        "~new_records_section "

#define     SERVER_DELETED_RECORDS    "~deleted_records_section "

#define     SERVER_LAST_CHANGED       "~last_chg"

nsresult
nsAbSync::ExtractInteger(char *aLine, char *aTag, char aDelim, PRInt32 *aRetVal)
{
  *aRetVal = 0;

  if ((!aLine) || (!aTag))
    return NS_ERROR_FAILURE;

  char *fLoc = PL_strstr(aLine, aTag);
  if (!fLoc)
    return NS_ERROR_FAILURE;

  fLoc += strlen(aTag);
  if (!*fLoc)
    return NS_ERROR_FAILURE;

  char *endLoc = fLoc;
  while ( (*endLoc) && (*endLoc != aDelim) )
    endLoc++;

  // Terminate it temporarily...
  char  saveLoc = '\0';
  if (*endLoc)
  {
    saveLoc = *endLoc;
    *endLoc = '\0';
  }

  *aRetVal = atoi(fLoc);
  *endLoc = saveLoc;
  return NS_OK;
}

char *
nsAbSync::ExtractCharacterString(char *aLine, char *aTag, char aDelim)
{
  if ((!aLine) || (!aTag))
    return nsnull;

  char *fLoc = PL_strstr(aLine, aTag);
  if (!fLoc)
    return nsnull;

  fLoc += strlen(aTag);
  if (!*fLoc)
    return nsnull;

  char *endLoc = fLoc;
  while ( (*endLoc) && (*endLoc != aDelim) )
    endLoc++;

  // Terminate it temporarily...
  char  saveLoc = '\0';
  if (*endLoc)
  {
    saveLoc = *endLoc;
    *endLoc = '\0';
  }

  char *returnValue = nsCRT::strdup(fLoc);
  *endLoc = saveLoc;
  return returnValue;
}

nsresult
nsAbSync::ExtractMappedMemberIDs(char *aLine, char *aTag, PRInt32 *serverID, PRInt32 *memLocalID, PRInt32 *memServerID)
{
  // Example: "&list_id=166&-106=164"
  NS_ENSURE_ARG_POINTER(aLine);
  NS_ENSURE_ARG_POINTER(aTag);
  NS_ENSURE_ARG_POINTER(serverID);
  NS_ENSURE_ARG_POINTER(memLocalID);
  NS_ENSURE_ARG_POINTER(memServerID);

  *serverID = *memLocalID = *memServerID = 0;
  char *fLoc = PL_strstr(aLine, aTag);
  if (!fLoc)
    return NS_ERROR_FAILURE;

  fLoc += strlen(aTag);
  if (!*fLoc)
    return NS_ERROR_FAILURE;

  char *endLoc = fLoc;
  while ( (*endLoc) && (*endLoc != '&') )
    endLoc++;

  // Terminate it temporarily...
  char  saveLoc;
  if (*endLoc)
  {
    saveLoc = *endLoc;
    *endLoc = '\0';
  }
  // Extract id from "&list_id=166" part.
  *serverID = atoi(fLoc);
  *endLoc = saveLoc;

  fLoc = endLoc;
  if (*fLoc != '&')
    return NS_ERROR_FAILURE;

  fLoc++;
  endLoc = fLoc;
  while ( (*endLoc) && (*endLoc != '=') )
    endLoc++;

  // Terminate it temporarily...
  if (*endLoc)
  {
    saveLoc = *endLoc;
    *endLoc = '\0';
  }

  // Extract ids from &-106=164
  *memLocalID = atoi(fLoc);
  *endLoc = saveLoc;
  if (*(endLoc+1))
    *memServerID = atoi(endLoc+1);
  else
    return NS_ERROR_FAILURE;

  return NS_OK;
}

// Return true if the server returned an error...
PRBool
nsAbSync::ErrorFromServer(char **errString)
{
  if (!nsCRT::strncasecmp(mProtocolOffset, SERVER_ERROR, strlen(SERVER_ERROR)))
  {
    *errString = mProtocolOffset + strlen(SERVER_ERROR);
    return PR_TRUE;
  }
  else
    return PR_FALSE;
}

// If this returns true, we are done with the data...
PRBool
nsAbSync::EndOfStream()
{
  if ( (!*mProtocolOffset) )
    return PR_TRUE;
  else
    return PR_FALSE;
}

nsresult        
nsAbSync::ProcessOpReturn()
{
  char  *workLine = nsnull;

  while ((workLine = ExtractCurrentLine()) != nsnull)
  {
    if (!*workLine)   // end of this section
      break;

    // Find the right tag and do something with it!
    // First a locale for the data
    if (!nsCRT::strncasecmp(workLine, SERVER_OP_RETURN_LOCALE, 
                            strlen(SERVER_OP_RETURN_LOCALE)))
    {
      char *locale = workLine;
      locale += strlen(SERVER_OP_RETURN_LOCALE);
      if (*locale)
        mLocale = NS_ConvertASCIItoUCS2(locale);
    }
    // this is for renaming records from the server...
    else if ( !nsCRT::strncasecmp(workLine, SERVER_OP_RETURN_RENAME, strlen(SERVER_OP_RETURN_RENAME)) ||
              !nsCRT::strncasecmp(workLine, SERVER_OP_RETURN_MAILLIST_RENAME, strlen(SERVER_OP_RETURN_MAILLIST_RENAME)) )
    {
      char *renop = workLine;
      renop += strlen(SERVER_OP_RETURN_RENAME);
      if (*renop)
      {
        nsresult  rv = NS_OK;
        PRInt32   clientID, serverID;

        rv  = ExtractInteger(renop, SERVER_OP_RETURN_CID, '&', &clientID);
        rv += ExtractInteger(renop, SERVER_OP_RETURN_SID, '&', &serverID);
        if (NS_SUCCEEDED(rv))
        {
          PatchHistoryTableWithNewID(clientID, serverID, -1, 0);
          if (!nsCRT::strncasecmp(workLine, SERVER_OP_RETURN_MAILLIST_RENAME, strlen(SERVER_OP_RETURN_MAILLIST_RENAME)))
            PatchListHistoryTableWithNewID(clientID, serverID, -1);
        }
      }
    }
    else if ( !nsCRT::strncasecmp(workLine, SERVER_OP_RETURN_MAILLIST_MEMBER_RENAME, strlen(SERVER_OP_RETURN_MAILLIST_MEMBER_RENAME)) )
    { // handle server response for mailing list members. example: op=k2maillist&list_id=91&-55=88
      char *renop = workLine;
      renop += strlen(SERVER_OP_RETURN_MAILLIST_MEMBER_RENAME);
      if (*renop)
      {
        nsresult  rv = NS_OK;
        PRInt32   listServerID, memLocalID, memServerID;

        rv = ExtractMappedMemberIDs(renop, SERVER_OP_RETURN_LIST_ID, &listServerID, &memLocalID, &memServerID);
        if (NS_SUCCEEDED(rv))
           PatchListHistoryTableWithNewMemberID(listServerID, memLocalID, memServerID, -1);
      }
    }

    PR_FREEIF(workLine);
  }

  return NS_OK;
}

PRUint32 nsAbSync::GetCardTypeByMemberId(PRUint32 aClientID)
{
  PRUint32   i;
  for (i=0; i<mOldTableCount; i++)
    if (mOldSyncMapingTable[i].localID == (PRInt32) aClientID)
      return mOldSyncMapingTable[i].flags;
  return 0;
}

nsresult
nsAbSync::LocateServerIDFromClientID(PRInt32 aClientID, PRInt32 *aServerID)
{
  PRUint32   i;

  *aServerID = 0;
  for (i=0; i<mOldTableCount; i++)
  {
    if (mOldSyncMapingTable[i].localID == aClientID)
    {
      *aServerID = mOldSyncMapingTable[i].serverID;
      return NS_OK;
    }
  }

  for (i=0; i<mNewTableCount; i++)
  {
    if (mNewSyncMapingTable[i].localID == aClientID)
    {
      *aServerID = mNewSyncMapingTable[i].serverID;
      return NS_OK;
    }
  }

  return NS_ERROR_FAILURE;
}

nsresult
nsAbSync::LocateClientIDFromServerID(PRInt32 aServerID, PRInt32 *aClientID)
{
  PRUint32   i;
  NS_ENSURE_ARG_POINTER(aClientID);

  *aClientID = 0;
  for (i=0; i<mOldTableCount; i++)
  {
    if (mOldSyncMapingTable[i].serverID == aServerID)
    {
      *aClientID = mOldSyncMapingTable[i].localID;
      return NS_OK;
    }
  }

  for (i=0; i<mNewTableCount; i++)
  {
    if (mNewSyncMapingTable[i].serverID == aServerID)
    {
      *aClientID = mNewSyncMapingTable[i].localID;
      return NS_OK;
    }
  }

  PRUint32 cnt = 0;
  if (mNewServerTable)
  {
    while (cnt < (PRUint32) mNewServerTable->Count())
    {
      syncMappingRecord *tRec = (syncMappingRecord *)mNewServerTable->ElementAt(cnt);
      if (!tRec)
      {
        cnt++;
        continue;
      }

      if (tRec->serverID == aServerID)
      {
        *aClientID = tRec->localID;
        return NS_OK;
      }
      cnt++;
    }
  }

  return NS_ERROR_FAILURE;
}

void nsAbSync::MarkDeletedInSyncTable(PRInt32 clientID)
{
  for (PRUint32 i=0; i<mNewTableCount; i++)
  {
    if (mNewSyncMapingTable[i].localID == clientID)
    {
      mNewSyncMapingTable[i].serverID = 0;
      return;
    }
  }
}

nsresult
nsAbSync::DeleteCardByServerID(PRInt32 aServerID)
{
  nsIEnumerator           *cardEnum = nsnull;
  nsCOMPtr<nsISupports>   obj = nsnull;
  PRUint32                aKey;

  // First off, find the aServerID in the history database and find
  // the local client ID for this server ID
  //
  PRInt32   clientID;
  if (NS_FAILED(LocateClientIDFromServerID(aServerID, &clientID)))
  {
    return NS_ERROR_FAILURE;
  }

  // Time to find the entry to delete!
  //
  nsresult        rv = NS_OK;
  nsIAddrDatabase *aDatabase = nsnull;
 
  // Get the address book entry
  nsCOMPtr <nsIRDFResource>     resource = nsnull;
  nsCOMPtr <nsIAbDirectory>     directory = nsnull;

  // Now, open the database...for now, make it the default
  rv = OpenAB(mAbSyncAddressBookFileName, &aDatabase);
  if (NS_FAILED(rv))
    return rv;

  // Get the RDF service...
  nsCOMPtr<nsIRDFService> rdfService = do_GetService("@mozilla.org/rdf/rdf-service;1", &rv);
  if (NS_FAILED(rv)) 
    goto EarlyExit;

  // this should not be hardcoded to abook.mab
  // this works for any address book...not sure why
  // absync on go againt abook.mab - candice
  rv = rdfService->GetResource(kPersonalAddressbookUri, getter_AddRefs(resource));
  if (NS_FAILED(rv)) 
    goto EarlyExit;
  
  // query interface 
  directory = do_QueryInterface(resource, &rv);
  if (NS_FAILED(rv)) 
    goto EarlyExit;

  rv = aDatabase->EnumerateCards(directory, &cardEnum);
  if (NS_FAILED(rv) || (!cardEnum))
  {
    rv = NS_ERROR_FAILURE;
    goto EarlyExit;
  }

  //
  // Now we have to find the entry and delete it from the database!
  //
  cardEnum->First();
  do
  {
    if (NS_FAILED(cardEnum->CurrentItem(getter_AddRefs(obj))))
      continue;
    else
    {
      nsCOMPtr<nsIAbCard> card;
      card = do_QueryInterface(obj, &rv);

      nsresult rv = NS_OK;
      nsCOMPtr<nsIAbMDBCard> dbcard(do_QueryInterface(card, &rv)); 
      if (NS_FAILED(rv) || !dbcard)
        continue;
      if (NS_FAILED(dbcard->GetKey(&aKey)))
        continue;

      if ((PRInt32) aKey == clientID)
      {
        rv = aDatabase->DeleteCard(card, PR_TRUE);
        // Mark deleted in the user sync table as well.
        if (NS_SUCCEEDED(rv))
          MarkDeletedInSyncTable(clientID);
        break;
      }
    }

  } while (NS_SUCCEEDED(cardEnum->Next()));

EarlyExit:
  if (cardEnum)
    delete cardEnum;

  // Database is open...make sure to close it
  if (aDatabase)
  {
    aDatabase->Close(PR_TRUE);
  }
  NS_IF_RELEASE(aDatabase);
  return rv;
}

nsresult        
nsAbSync::DeleteUsers()
{
  nsresult    rv = NS_ERROR_FAILURE;

  for (PRInt32 i=0; i<mDeletedRecordValues->Count(); i+=mDeletedRecordTags->Count())
  {
    nsString *val = mDeletedRecordValues->StringAt(i);
    if ( (!val) || val->IsEmpty() )
      continue;

    PRInt32   aErrorCode;
    PRInt32 delID = val->ToInteger(&aErrorCode);
    if (NS_FAILED(aErrorCode))
      continue;

    rv = DeleteCardByServerID(delID);
  }

  return rv;
}

nsresult        
nsAbSync::ProcessDeletedRecords(PRUint32 sectionId)
{
  nsresult  rv = NS_OK;

  rv =LoadInputValuesAndTags(&mDeletedRecordTags, &mDeletedRecordValues);
  NS_ENSURE_SUCCESS(rv, rv);
  
  // Ok, now that we are here, we check to see if we have anything
  // in the records array. If we do, then deal with it!
  //
  // If nothing in the array...just return!
  if (mDeletedRecordValues->Count() != 0)
  {
    //PRInt32 tType = DetermineTagType(mDeletedRecordTags);
    switch (sectionId) 
    {
    case 0: //SYNC_SINGLE_USER_TYPE:
      rv += DeleteUsers();
      break;

    case 1: //SYNC_MAILLIST_TYPE:
      rv += DeleteMailingLists();
      break;

    case 2: //SYNC_GROUP_TYPE:
      rv += DeleteGroups();
      break;

    case 3:   //SYNC_MAILLIST_MEMBER_TYPE:
      rv = DeleteMailingListMembers();
      break;
    
    //case SYNC_UNKNOWN_TYPE:
    default:
      rv = NS_ERROR_FAILURE;
      break;
    }
  }

  delete mDeletedRecordValues;
  mDeletedRecordValues = nsnull;
  delete mDeletedRecordTags;
  mDeletedRecordTags = nsnull;
  return rv;
}

nsresult        
nsAbSync::ProcessLastChange()
{
  char      *aLine = nsnull;
 
  if (EndOfStream())
    return NS_ERROR_FAILURE;

  if ( (aLine = ExtractCurrentLine()) != nsnull)
  {
    if (*aLine)
    {
      mLastChangeNum = atoi(aLine);
      PR_FREEIF(aLine);
    }
    return NS_OK;
  }
  else
    return NS_ERROR_FAILURE;
}

char *        
nsAbSync::ExtractCurrentLine()
{
  nsString    extractString; 

  while ( (*mProtocolOffset) && 
          ( (*mProtocolOffset != nsCRT::CR) && (*mProtocolOffset != nsCRT::LF) )
        )
  {
    extractString.Append(PRUnichar(*mProtocolOffset));
    mProtocolOffset++;
  }

  if (!*mProtocolOffset)
    return nsnull;
  else
  {
    while ( (*mProtocolOffset) && 
            (*mProtocolOffset == nsCRT::CR) )
            mProtocolOffset++;

    if (*mProtocolOffset == nsCRT::LF)
      mProtocolOffset++;

    char *tString = ToNewCString(extractString);
    if (tString)
    {
      char *ret = nsUnescape(tString);
      return ret;
    }
    else
      return nsnull;
  }
}

nsresult        
nsAbSync::AdvanceToNextLine()
{
  // First, find first nsCRT::CR or nsCRT::LF...
  while ( (*mProtocolOffset) && 
          ( (*mProtocolOffset != nsCRT::CR) && (*mProtocolOffset != nsCRT::LF) )
        )
  {
    mProtocolOffset++;
  }

  // now, make sure we are past the LF...
  if (*mProtocolOffset)
  {
    while ( (*mProtocolOffset) && 
            (*mProtocolOffset != nsCRT::LF) )
        mProtocolOffset++;
    
    if (*mProtocolOffset == nsCRT::LF)
      mProtocolOffset++;
  }

  return NS_OK;   // at end..but this is ok...
}

nsresult
nsAbSync::AdvanceToNextSection()
{
  // If we are sitting on a section...bump past it...
  mProtocolOffset++;
  while (!EndOfStream() && *mProtocolOffset != '~')
    AdvanceToNextLine();
  return NS_OK;
}

// See if we are sitting on a particular tag...and eat if if we are
PRBool          
nsAbSync::TagHit(const char *aTag, PRBool advanceToNextLine)
{
  if ((!aTag) || (!*aTag))
    return PR_FALSE;
  
  if (!nsCRT::strncasecmp(mProtocolOffset, aTag, strlen(aTag)))
  {
    if (advanceToNextLine)
      AdvanceToNextLine();
    else
      mProtocolOffset += strlen(aTag);
    return PR_TRUE;
  }
  else
    return PR_FALSE;
}

PRBool
nsAbSync::ParseNextSection()
{
  nsresult      rv = NS_OK;

  if (TagHit(SERVER_OP_RETURN, PR_TRUE))
    rv = ProcessOpReturn();
  else if (TagHit(SERVER_LAST_CHANGED, PR_TRUE))
    rv = ProcessLastChange();
  else if (TagHit(SERVER_NEW_RECORDS, PR_FALSE))
  {
    if ((*mProtocolOffset >= '0') && (*mProtocolOffset <= '9'))
    {
      PRUint32 sesctionId = *mProtocolOffset - '0';
      AdvanceToNextLine();
      rv = ProcessNewRecords(sesctionId);
    }
    else
      rv = NS_ERROR_FAILURE;
  }
  else if (TagHit(SERVER_DELETED_RECORDS, PR_FALSE))
  {
    if ((*mProtocolOffset >= '0') && (*mProtocolOffset <= '9'))
    {
      PRUint32 sesctionId = *mProtocolOffset - '0';
      AdvanceToNextLine();
      rv = ProcessDeletedRecords(sesctionId);
    }
    else
      rv = NS_ERROR_FAILURE;
  }
  else    // We shouldn't get here...but if we do...
    rv = AdvanceToNextSection();

  // If this is a failure, then get to the next section!
  if (NS_FAILED(rv))
    AdvanceToNextSection();

  return PR_TRUE;
}

//
// This is the response side of getting things back from the server
//
nsresult
nsAbSync::ProcessServerResponse(const char *aProtocolResponse)
{
  nsresult        rv = NS_OK;
  PRUint32        writeCount = 0;
  PRInt32         writeSize = 0;
  char            *errorString; 
  PRBool          parseOk = PR_TRUE;

  // If no response, then this is a problem...
  if (!aProtocolResponse)
  {
    PRUnichar   *outValue = GetString(NS_LITERAL_STRING("syncInvalidResponse").get());
    DisplayErrorMessage(outValue);
    PR_FREEIF(outValue);
    Log("Processing server data", "Invalid server response received!");
    return NS_ERROR_FAILURE;
  }

  // Log current event/time
  Log("Processing server data at", BuildSyncTimestamp());

  // Assign the vars...
  mProtocolResponse = (char *)aProtocolResponse;
  mProtocolOffset = (char *)aProtocolResponse;

  // Log the data received from servers.
  if (mProtocolOffset)
    ParseAndLogServerData(mProtocolOffset);

#if DEBUG_OFFLINE_TEST
  {
    // Debug only - reading server data from a file. It provides a
    // quick and easy way to debug server-to-client problems by simply
    // preapring your server data in a text file.
    char str[4*1024];
    PRInt32 numread;
    FILE *fd = fopen("d:\\temp\\abserverInput.txt", "r");
    numread = fread(str, sizeof(char), 4*1024-1, fd);
    mProtocolOffset = str;
    fclose(fd);
  }
#endif

  if (ErrorFromServer(&errorString))
  {
    PRUnichar   *msgValue, *outValue=nsnull;

    // Covert server exceed max record msg here.
    if (! nsCRT::strncasecmp(errorString, SYNC_ERROR_EXCEED_MAX_RECORD, nsCRT::strlen(SYNC_ERROR_EXCEED_MAX_RECORD)))
      msgValue = GetString(NS_LITERAL_STRING("exceedMaxRecordError").get());
    else
    {
      outValue = GetString(NS_LITERAL_STRING("syncServerError").get());
    msgValue = nsTextFormatter::smprintf(outValue, errorString);
    }

    DisplayErrorMessage(msgValue);

    PR_FREEIF(outValue);
    PR_FREEIF(msgValue);

    return NS_ERROR_FAILURE;
  }

  while ( (!EndOfStream()) && (parseOk) )
  {
    parseOk = ParseNextSection();
  }

  // Write current existing users to user history file.
  if (NS_FAILED(SaveCurrentUsersToHistoryFile()))
    Log("Saving current users to history file", "FAILED!");

  // Write current existing lists to list history file.
  if (NS_FAILED(SaveCurrentListsToHistoryFile()))
    Log("Saving current lists to history file", "FAILED!");

  if (mLastChangeNum > 1)
  {
    nsCOMPtr<nsIPref> prefs(do_GetService(NS_PREF_CONTRACTID, &rv)); 
    if (NS_SUCCEEDED(rv) && prefs) 
    {
      prefs->SetIntPref("mail.absync.last_change", mLastChangeNum);
      prefs->SavePrefFile(nsnull);
    }
  }

  Log("Finish processing server data at", BuildSyncTimestamp());
  return NS_OK;
}

PRInt32
nsAbSync::DetermineTagType(nsStringArray *aArray)
{
  PRBool    isCard = PR_FALSE;
  PRBool    isList = PR_FALSE;
  PRBool    isAOLGroup = PR_FALSE;
  PRBool    isListMember = PR_FALSE;
  PRBool    isListEmailMember = PR_FALSE;

  for (PRInt32 i = 0; i<aArray->Count(); i++)
  {
    nsString *val = mNewRecordTags->StringAt(i);
    if ( (!val) || (val->IsEmpty()) )
      continue;

    if (val->Equals(NS_LITERAL_STRING("record_id")))
      isCard = PR_TRUE;
    else if (val->Equals(NS_LITERAL_STRING("listname")))
      isList = PR_TRUE;
    else if (val->Equals(NS_LITERAL_STRING("group_name")))
      isAOLGroup = PR_TRUE;
    else if (val->Equals(NS_LITERAL_STRING("contact_record_id")))
      isListMember = PR_TRUE;
    else if (val->Equals(NS_LITERAL_STRING("email_string")))
      isListEmailMember = PR_TRUE;
  }

  if (isAOLGroup)
    return SYNC_GROUP_TYPE;
  else if (isListMember)
    return SYNC_MAILLIST_MEMBER_TYPE;
  else if (isListEmailMember)
    return SYNC_MAILLIST_MEMBER_EMAIL_TYPE;
  else if (isList)
    return SYNC_MAILLIST_TYPE;
  else if (isCard)
    return SYNC_SINGLE_USER_TYPE;
  else
    // If we get here, don't have a clue!
    return SYNC_UNKNOWN_TYPE;
}

nsresult nsAbSync::LoadInputValuesAndTags(nsStringArray **recordTags, nsStringArray **recordValues)
{
  NS_ENSURE_ARG_POINTER(recordTags);
  NS_ENSURE_ARG_POINTER(recordValues);
  *recordTags = *recordValues = nsnull;

  // Get all of the tags for new records. These could be one set of tags with multiple sets of values.
  //
  char *workLine;
  nsStringArray *tagsArray = new nsStringArray();
  if (!tagsArray)
    return NS_ERROR_OUT_OF_MEMORY;

  nsStringArray * valuesArray = new nsStringArray();
  if (!valuesArray)
  {
    delete tagsArray;
    return NS_ERROR_OUT_OF_MEMORY;
  }

  while ((workLine = ExtractCurrentLine()) != nsnull)
  {
    if (!*workLine)   // end of this section
      break;

    tagsArray->AppendString(nsString(NS_ConvertASCIItoUCS2(workLine)));
    PR_FREEIF(workLine);
  }

  // Now, see what the next line is...if its a CRLF, then we
  // really don't have anything to do here and we can just return
  //
  while ((workLine = ExtractCurrentLine()) != nsnull)
  {
    if (!*workLine)
     break;
    
    // Ok, if we are here, then we need to loop and get the values
    // for the tags in question starting at the second since the 
    // first has already been eaten
    //
    valuesArray->AppendString(nsString(NS_ConvertASCIItoUCS2(workLine)));
    PR_FREEIF(workLine);
    for (PRInt32 i=0; i<(tagsArray->Count()-1); i++)
    {
      workLine = ExtractCurrentLine();
      if (!workLine)
      {
        delete tagsArray;
        delete valuesArray;
        return NS_ERROR_FAILURE;
      }
      
      valuesArray->AppendString(nsString(NS_ConvertASCIItoUCS2(workLine)));
      PR_FREEIF(workLine);
    }

    // Eat the CRLF!
    workLine = ExtractCurrentLine();
    PR_FREEIF(workLine);
  }

  *recordTags = tagsArray;
  *recordValues = valuesArray;
  return NS_OK;
}

nsresult        
nsAbSync::ProcessNewRecords(PRUint32 sectionId)
{
  nsresult  rv = NS_OK;

  rv =LoadInputValuesAndTags(&mNewRecordTags, &mNewRecordValues);
  NS_ENSURE_SUCCESS(rv, rv);

  // Ok, now that we are here, we need to figure out what type
  // of addition to the address book this is. 
  //
  // But first...sanity! If nothing in the array...just return!
  if (mNewRecordValues->Count() != 0)
  {
    switch (sectionId) 
    {
    case 0: //SYNC_SINGLE_USER_TYPE:
      rv = AddNewUsers();
      break;

    case 1: //SYNC_MAILLIST_TYPE:
      rv = AddNewMailingLists();
      break;

    case 2: //SYNC_GROUP_TYPE:
      rv = AddNewGroups();
      break;

    case 3: //SYNC_MAILLIST_MEMBER_TYPE:
      rv = AddNewMailingListMembers();
      break;

    case 4: //SYNC_MAILLIST_MEMBER_EMAIL_TYPE:
      rv = AddNewMailingListEmailMembers();
      break;
    
    //case SYNC_UNKNOWN_TYPE:
    default:
      return NS_ERROR_FAILURE;
    }
  }

  delete mNewRecordValues;
  mNewRecordValues = nsnull;
  delete mNewRecordTags;
  mNewRecordTags = nsnull;

  return rv;
}

nsresult
nsAbSync::FindCardByClientID(PRInt32           aClientID,
                             nsIAddrDatabase  *aDatabase,
                             nsIAbDirectory   *directory,
                             nsIAbCard        **aReturnCard)
{
  nsIEnumerator           *cardEnum = nsnull;
  nsCOMPtr<nsISupports>   obj = nsnull;
  PRUint32                aKey;
  nsresult                rv = NS_ERROR_FAILURE;

  // Time to find the entry to play with!
  //
  NS_ENSURE_ARG_POINTER(aReturnCard);
  NS_ENSURE_ARG_POINTER(aDatabase);
  NS_ENSURE_ARG_POINTER(directory);

  *aReturnCard = nsnull;
  rv = aDatabase->EnumerateCards(directory, &cardEnum);
  if (NS_FAILED(rv) || (!cardEnum))
  {
    rv = NS_ERROR_FAILURE;
    goto EarlyExit;
  }

  //
  // Now we have to find the entry and return it!
  //
  cardEnum->First();
  do
  {
    if (NS_FAILED(cardEnum->CurrentItem(getter_AddRefs(obj))))
      continue;
    else
    {
      nsCOMPtr<nsIAbCard> card;
      card = do_QueryInterface(obj, &rv);

      nsresult rv=NS_OK;
      nsCOMPtr<nsIAbMDBCard> dbcard(do_QueryInterface(card, &rv)); 
      if (NS_FAILED(rv) || !dbcard)
        continue;
      if (NS_FAILED(dbcard->GetKey(&aKey)))
        continue;

      // Found IT!
      if ((PRInt32) aKey == aClientID)
      {
        *aReturnCard = card;
        rv = NS_OK;
        break;
      }
    }

  } while (NS_SUCCEEDED(cardEnum->Next()));

EarlyExit:
  if (cardEnum)
    delete cardEnum;

  return rv;
}

//
// This routine will be used to hunt through an index of CRC's to see if this
// address book card has already been added to the local database. If so, don't
// duplicate!
// 
PRBool
nsAbSync::CardAlreadyInAddressBook(nsIAbCard        *newCard,
                                   PRInt32          *aClientID,
                                   ulong            *aRetCRC)
{
  PRUint32   i;
  nsString  tProtLine;
  PRBool    rVal = PR_FALSE;

  // First, generate a protocol line for this new card...
  if (NS_FAILED(GenerateProtocolForCard(newCard, PR_FALSE, tProtLine)))
    return PR_FALSE;

  ulong   workCRC = GetCRC((char *)NS_ConvertUCS2toUTF8(tProtLine).get());
  for (i=0; i<mCrashTableSize; i++)
  {
    if (mCrashTable[i].CRC == workCRC)
    {
      *aClientID = mCrashTable[i].localID;
      *aRetCRC = workCRC;
      rVal = PR_TRUE;
    }
  }
  
  return rVal;
}

// 
// This will look for an entry in the address book for this particular person and return 
// the address book card and the client ID if found...otherwise, it will return ZERO
// 
PRInt32
nsAbSync::HuntForExistingABEntryInServerRecord(PRInt32          aPersonIndex, 
                                               nsIAddrDatabase  *aDatabase,
                                               nsIAbDirectory   *directory,
                                               PRBool           isUser,
                                               PRInt32          *aServerID, 
                                               nsIAbCard        **newCard)
{
  NS_ENSURE_ARG_POINTER(aServerID);
  NS_ENSURE_ARG_POINTER(newCard);

  PRInt32   clientID;
  PRInt32   j;

  //
  // Ok, first thing is to find out what the server ID is for this person?
  //
  *aServerID = 0;
  *newCard = nsnull;
  const char *columnName;
  columnName = (isUser) ? "record_id" : "list_id";
  for (j = 0; j < mNewRecordTags->Count(); j++)
  {
    nsString *val = mNewRecordValues->StringAt((aPersonIndex*(mNewRecordTags->Count())) + j);
    if ( (val) && (!val->IsEmpty()) )
    {
      // See if this is the record_id...
      nsString *tagVal = mNewRecordTags->StringAt(j);
      if (isUser && tagVal->Equals(NS_LITERAL_STRING("record_id")))
      {
        PRInt32 errorCode;
        *aServerID = val->ToInteger(&errorCode);
        break;
      }
      else
        if ((!isUser) && tagVal->Equals(NS_LITERAL_STRING("list_id")))
      {
        PRInt32 errorCode;
        *aServerID = val->ToInteger(&errorCode);
        break;
      }
    }
  }

  // Hmm...no server ID...not good...well, better return anyway
  if (*aServerID == 0)
    return 0;

  // If didn't find the client ID, better bail out!
  if (NS_FAILED(LocateClientIDFromServerID(*aServerID, &clientID)))
    return 0;

  // Now, we have the clientID, need to find the address book card in the
  // database for this client ID...
  if (NS_FAILED(FindCardByClientID(clientID, aDatabase, directory, newCard)))
  {
    *aServerID = 0;
    return 0;
  }
  else
    return clientID;
}

nsresult        
nsAbSync::AddNewUsers()
{
  nsresult        rv = NS_OK;
  nsIAddrDatabase *aDatabase = nsnull;
  PRInt32         addCount = 0;
  PRInt32         i,j;
  PRInt32         serverID;
  PRInt32         localID;
  nsCOMPtr<nsIAbCard> newCard;
  nsCOMPtr<nsIAbCard> tCard;
  nsString        tempProtocolLine;
  PRBool          isNewCard = PR_TRUE;
 
  // Get the address book entry
  nsCOMPtr <nsIRDFResource>     resource = nsnull;
  nsCOMPtr <nsIAbDirectory>     directory = nsnull;

  // Do a sanity check...if the numbers don't add up, then 
  // return an error
  //
  if ( (mNewRecordValues->Count() % mNewRecordTags->Count()) != 0)
    return NS_ERROR_FAILURE;

  // Get the add count...
  addCount = mNewRecordValues->Count() / mNewRecordTags->Count();

  // Now, open the database...for now, make it the default
  rv = OpenAB(mAbSyncAddressBookFileName, &aDatabase);
  if (NS_FAILED(rv))
    return rv;

  // Get the RDF service...
  nsCOMPtr<nsIRDFService> rdfService = do_GetService("@mozilla.org/rdf/rdf-service;1", &rv);
  if (NS_FAILED(rv)) 
    goto EarlyExit;

  // this should not be hardcoded to abook.mab
  // this works for any address book...not sure why
  // absync on go againt abook.mab - candice
  rv = rdfService->GetResource(kPersonalAddressbookUri, getter_AddRefs(resource));
  if (NS_FAILED(rv)) 
    goto EarlyExit;
  
  // query interface 
  directory = do_QueryInterface(resource, &rv);
  if (NS_FAILED(rv)) 
    goto EarlyExit;

  // Create the new array for history if it is still null...
  if (!mNewServerTable)
  {
    mNewServerTable = new nsVoidArray();
    if (!mNewServerTable)
    {
      rv = NS_ERROR_OUT_OF_MEMORY;
      goto EarlyExit;
    }
  }

  //
  // 
  // Create the new card that we will eventually add to the 
  // database...
  //
  for (i = 0; i < addCount; i++)
  {    
    serverID = 0;     // for safety

    // 
    // Ok, if we are here, then we need to figure out if this is really a NEW card
    // or just an update of an existing card that is already on this client. 
    //
    // NOTE: i is the entry of the person we are working on!
    //
    localID = HuntForExistingABEntryInServerRecord(i, aDatabase, directory, PR_TRUE, &serverID, getter_AddRefs(tCard));
    if ( (localID > 0) && (nsnull != tCard))
    {
      // This is an existing entry in the local address book
      // We need to dig out the card entry or just assign something here
      newCard = tCard;
      isNewCard = PR_FALSE;
    }
    else
    {
      // This is a new entry!
      rv = nsComponentManager::CreateInstance(NS_ABCARDPROPERTY_CONTRACTID, nsnull, NS_GET_IID(nsIAbCard), 
                                              getter_AddRefs(newCard));
      isNewCard = PR_TRUE;
    }

    if (NS_FAILED(rv) || !newCard)
    {
      rv = NS_ERROR_OUT_OF_MEMORY;
      goto EarlyExit;
    }

    for (j = 0; j < mNewRecordTags->Count(); j++)
    {
      nsString *val = mNewRecordValues->StringAt((i*(mNewRecordTags->Count())) + j);
      if ( (val) && (!val->IsEmpty()) )
      {
        // See if this is the record_id, keep it around for later...
        nsString *tagVal = mNewRecordTags->StringAt(j);
        if (tagVal->Equals(NS_LITERAL_STRING("record_id")))
        {
          PRInt32 errorCode;
          serverID = val->ToInteger(&errorCode);

#ifdef DEBUG_ABSYNC
  printf("ABSYNC: ADDING Card: %d\n", serverID);
#endif
        }

        // Ok, "val" could still be URL Encoded, so we need to decode
        // first and then pass into the call...
        //
        char *ret = nsUnescape((char *)NS_ConvertUCS2toUTF8(*val).get());
        if (ret)
          //val->AssignWithConversion(ret);
          *val = NS_ConvertASCIItoUCS2(ret);
        AddValueToNewCard(newCard, mNewRecordTags->StringAt(j), val);
      }
    }

    // Now do the phone numbers...they are special???
    ProcessPhoneNumbersTheyAreSpecial(newCard);

    // Ok, now that we are here, we should check if this is a recover from a crash situation.
    // If it is, then we should try to find the CRC for this entry in the local address book
    // and tweak a new flag if it is already there.
    //
    PRBool    cardAlreadyThere = PR_FALSE;
    ulong     tempCRC;

    if (mLastSyncFailed)
      cardAlreadyThere = CardAlreadyInAddressBook(newCard, &localID, &tempCRC);

    // Ok, now, lets be extra nice and see if we should build a display name for the
    // card.
    //
    PRUnichar   *tDispName = nsnull;
    if (NS_SUCCEEDED(newCard->GetCardValue(kDisplayNameColumn, &tDispName)))
    {
      if ( (!tDispName) || (!*tDispName) )
      {
        PRUnichar   *tFirstName = nsnull;
        PRUnichar   *tLastName = nsnull;
        nsString    tFullName;

        newCard->GetCardValue(kFirstNameColumn, &tFirstName);
        newCard->GetCardValue(kLastNameColumn, &tLastName);
  
        if (tFirstName)
        {
          tFullName.Append(tFirstName);
          if (tLastName)
            tFullName.Append(NS_LITERAL_STRING(" "));
        }

        if (tLastName)
          tFullName.Append(tLastName);

        PR_FREEIF(tFirstName);
        PR_FREEIF(tLastName);

        // Ok, now we should add a display name...
        newCard->SetDisplayName(tFullName.get());
      }
      else
        PR_FREEIF(tDispName);
    }

    // Ok, now we need to modify or add the card! ONLY IF ITS NOT THERE ALREADY
    if (!cardAlreadyThere)
    {
      if (!isNewCard)
        rv = aDatabase->EditCard(newCard, PR_TRUE);
      else
      {
        // hack
        // there is no way (currently) to set generic attributes on simple abcardproperties
        // so we have to use a mdbcard instead. We can remove this once #128567 is fixed.
        nsCOMPtr<nsIAbMDBCard> dbcard = do_CreateInstance(NS_ABMDBCARD_CONTRACTID, &rv);
        if (NS_FAILED(rv))
          continue;
        
        nsCOMPtr<nsIAbCard> card = do_QueryInterface(dbcard, &rv);
        if (NS_FAILED(rv))
          continue;
        
        rv = card->Copy(newCard);
        if (NS_FAILED(rv))
          continue;
        
        dbcard->SetAbDatabase(aDatabase);
        PRUint32  tID;
        // card has to be a mdbcard, or tID will come back as 0
        rv = aDatabase->CreateNewCardAndAddToDBWithKey(card, PR_TRUE, &tID);
        localID = (PRInt32) tID;
        newCard = card;
      }
    }

    //
    // Now, calculate the NEW CRC for the new or updated card...
    //
    syncMappingRecord *newSyncRecord = (syncMappingRecord *)PR_Malloc(sizeof(syncMappingRecord));
    if (newSyncRecord)
    {
      if (NS_FAILED(GenerateProtocolForCard(newCard, PR_FALSE, tempProtocolLine)))
        continue;

      // Get the CRC for this temp entry line...
      ulong crc = GetCRC((char *)NS_ConvertUCS2toUTF8(tempProtocolLine).get());

      if (!isNewCard)
      {
        // First try to patch the old table if this is an old card...if that
        // fails, then flip the flag to TRUE and have a new record created
        // in newSyncRecord
        //
        if (NS_FAILED(PatchHistoryTableWithNewID(localID, serverID, 1, crc)))
          isNewCard = PR_TRUE;
      }

      if (isNewCard)
      {
        memset(newSyncRecord, 0, sizeof(syncMappingRecord));
        newSyncRecord->CRC = crc;
        newSyncRecord->serverID = serverID;
        newSyncRecord->localID = localID;
        newSyncRecord->flags |= SYNC_IS_CARD;
        mNewServerTable->AppendElement((void *)newSyncRecord);
      }
      else
      {
        PR_FREEIF(newSyncRecord);
      }
    }

    newCard = nsnull;
  }

EarlyExit:
  // Database is open...make sure to close it
  if (aDatabase)
  {
    aDatabase->Close(PR_TRUE);
  }
  NS_IF_RELEASE(aDatabase);  
  return rv;
}

PRInt32 
nsAbSync::GetTypeOfPhoneNumber(const nsAString& tagName)
{
  NS_NAMED_LITERAL_STRING(typePostfix, "_type");
  const nsAString& compValue = tagName + typePostfix;

  for (PRInt32 i=0; i<mPhoneTypes->Count(); i++)
  {
    nsString *val = mPhoneTypes->StringAt(i);
    if ( (!val) || val->IsEmpty() )
      continue;

    const nsAString& valPrefix = Substring(*val, 0, compValue.Length());
    if (compValue.Equals(valPrefix, nsCaseInsensitiveStringComparator()))
    {
      nsString phoneType;

      phoneType.Assign(*val);
      PRInt32 loc = phoneType.FindChar('=');
      if (loc == -1)
        continue;
      
      phoneType.Cut(0, loc+1);
      if (phoneType.Equals(NS_LITERAL_STRING(ABSYNC_HOME_PHONE_TYPE),
                           nsCaseInsensitiveStringComparator()))
        return ABSYNC_HOME_PHONE_ID;
      else if (phoneType.Equals(NS_LITERAL_STRING(ABSYNC_WORK_PHONE_TYPE),
                                nsCaseInsensitiveStringComparator()))
        return ABSYNC_WORK_PHONE_ID;
      else if (phoneType.Equals(NS_LITERAL_STRING(ABSYNC_FAX_PHONE_TYPE),
                                nsCaseInsensitiveStringComparator()))
        return ABSYNC_FAX_PHONE_ID;
      else if (phoneType.Equals(NS_LITERAL_STRING(ABSYNC_PAGER_PHONE_TYPE),
                                nsCaseInsensitiveStringComparator()))
        return ABSYNC_PAGER_PHONE_ID;
      else if (phoneType.Equals(NS_LITERAL_STRING(ABSYNC_CELL_PHONE_TYPE),
                                nsCaseInsensitiveStringComparator()))
        return ABSYNC_CELL_PHONE_ID;
    }
  }

  // If we get here...drop back to the defaults...why, oh why is it this way?
  //
  if (tagName.Equals(NS_LITERAL_STRING("phone1")))
    return ABSYNC_HOME_PHONE_ID;
  else if (tagName.Equals(NS_LITERAL_STRING("phone2")))
    return ABSYNC_WORK_PHONE_ID;
  else if (tagName.Equals(NS_LITERAL_STRING("phone3")))
    return ABSYNC_FAX_PHONE_ID;
  else if (tagName.Equals(NS_LITERAL_STRING("phone4")))
    return ABSYNC_PAGER_PHONE_ID;
  else if (tagName.Equals(NS_LITERAL_STRING("phone5")))
    return ABSYNC_CELL_PHONE_ID;

  return 0;
}

nsresult
nsAbSync::ProcessPhoneNumbersTheyAreSpecial(nsIAbCard *aCard)
{
  PRInt32   i;
  nsString  tagName;
  nsString  phoneNumber;
  PRInt32   loc;

  for (i=0; i<mPhoneValues->Count(); i++)
  {
    nsString *val = mPhoneValues->StringAt(i);
    if ( (!val) || val->IsEmpty() )
      continue;

    tagName.Assign(*val);
    phoneNumber.Assign(*val);
    loc = tagName.FindChar('=');
    if (loc == -1)
      continue;

    tagName.Cut(loc, (tagName.Length() - loc));
    phoneNumber.Cut(0, loc+1);

    PRInt32 pType = GetTypeOfPhoneNumber(tagName);
    if (pType == 0)
      continue;
    
    if (pType == ABSYNC_PAGER_PHONE_ID)
      aCard->SetPagerNumber(phoneNumber.get());
    else if (pType == ABSYNC_HOME_PHONE_ID)
      aCard->SetHomePhone(phoneNumber.get());
    else if (pType == ABSYNC_WORK_PHONE_ID)
      aCard->SetWorkPhone(phoneNumber.get());
    else if (pType == ABSYNC_FAX_PHONE_ID)
      aCard->SetFaxNumber(phoneNumber.get());
    else if (pType == ABSYNC_CELL_PHONE_ID)
      aCard->SetCellularNumber(phoneNumber.get());
  }

  // Reset the phone arrays for next card.
  mPhoneValues->Clear();
  mPhoneTypes->Clear();

  return NS_OK;
}

nsresult nsAbSync::ParseDateFields(nsIAbCard *aCard, nsString *aTagName, nsString *aTagValue)
{
  // Parse out date (in aTagValue) like 03/05/1996 and set yy, mm and dd accordingly.
  NS_ENSURE_ARG_POINTER(aTagName);
  NS_ENSURE_ARG_POINTER(aTagValue);

  PRInt32 atPos, cnt = 0;
  nsAutoString tempStr, digits;
  tempStr.Assign(*aTagValue);
  while ((atPos = tempStr.FindChar('/')) != -1)
  {
	  tempStr.Left(digits, atPos);
    if (cnt == 0)
    {
      if (aTagName->Equals(kServerBirthdayColumn))
        aCard->SetBirthMonth(digits.get());       // month
      else if (aTagName->Equals(kServerAnniversaryColumn))
        aCard->SetAnniversaryMonth(digits.get()); // month
    }
    else
    {
      if (aTagName->Equals(kServerBirthdayColumn))
        aCard->SetBirthDay(digits.get());       // day
      else if (aTagName->Equals(kServerAnniversaryColumn))
        aCard->SetAnniversaryDay(digits.get()); // day
    }
	  tempStr.Cut(0, atPos+1);
    cnt++;
  }
  if (aTagName->Equals(kServerBirthdayColumn))
    aCard->SetBirthYear(tempStr.get());       // year
  else
    aCard->SetAnniversaryYear(tempStr.get());  // year
  return NS_OK;
}

nsresult
nsAbSync::AddValueToNewCard(nsIAbCard *aCard, nsString *aTagName, nsString *aTagValue)
{
  nsresult  rv = NS_OK;

  // 
  // First, we are going to special case the phone entries because they seem to
  // be handled differently than all other tags. All other tags are unique with no specifiers
  // as to their type...this is not the case with phone numbers.
  //
  PRUnichar aChar = '_';
  nsString  outValue;
  char      *tValue = nsnull;

  tValue = ToNewCString(*aTagValue);
  if (tValue)
  {
    rv = nsMsgI18NConvertToUnicode(nsCAutoString("UTF-8"), nsCAutoString(tValue), outValue);
    if (NS_SUCCEEDED(rv))
      aTagValue->Assign(outValue);
    PR_FREEIF(tValue);
  }

  if (Substring(*aTagName, 0, 5).Equals(NS_LITERAL_STRING("phone"),
                                        nsCaseInsensitiveStringComparator()))
  {
    nsString      tempVal;
    tempVal.Append(*aTagName + NS_LITERAL_STRING("=") + *aTagValue);

    if (aTagName->FindChar(aChar) != -1)
      mPhoneTypes->AppendString(tempVal);
    else
      mPhoneValues->AppendString(tempVal);

    return NS_OK;
  }

#ifdef DEBUG_ABSYNC
  char *name = ToNewCString(*aTagName);
  char *value = ToNewCString(*aTagValue);
  printf("ABSYNC: from server:  %s = %s\n", name,value);
  PR_FREEIF(name);
  PR_FREEIF(value);
#endif

  // Ok, we need to figure out what the tag name from the server maps to and assign
  // this value the new nsIAbCard
  //
  if (aTagName->Equals(kServerFirstNameColumn))
    aCard->SetFirstName(aTagValue->get());
  else if (aTagName->Equals(kServerLastNameColumn))
    aCard->SetLastName(aTagValue->get());
  else if (aTagName->Equals(kServerDisplayNameColumn))
    aCard->SetDisplayName(aTagValue->get());
  else if (aTagName->Equals(kServerNicknameColumn))
    aCard->SetNickName(aTagValue->get());
  else if (aTagName->Equals(kServerPriEmailColumn))
    aCard->SetPrimaryEmail(aTagValue->get());
  else if (aTagName->Equals(kServer2ndEmailColumn))
    aCard->SetSecondEmail(aTagValue->get());
  else if (aTagName->Equals(kServerDefaultEmailColumn))
    aCard->SetDefaultEmail(aTagValue->get());
  else if (aTagName->Equals(kServerHomeAddressColumn))
    aCard->SetHomeAddress(aTagValue->get());
  else if (aTagName->Equals(kServerHomeAddress2Column))
    aCard->SetHomeAddress2(aTagValue->get());
  else if (aTagName->Equals(kServerHomeCityColumn))
    aCard->SetHomeCity(aTagValue->get());
  else if (aTagName->Equals(kServerHomeStateColumn))
    aCard->SetHomeState(aTagValue->get());
  else if (aTagName->Equals(kServerHomeZipCodeColumn))
    aCard->SetHomeZipCode(aTagValue->get());
  else if (aTagName->Equals(kServerHomeCountryColumn))
    aCard->SetHomeCountry(aTagValue->get());
  else if (aTagName->Equals(kServerWorkAddressColumn))
    aCard->SetWorkAddress(aTagValue->get());
  else if (aTagName->Equals(kServerWorkAddress2Column))
    aCard->SetWorkAddress2(aTagValue->get());
  else if (aTagName->Equals(kServerWorkCityColumn))
    aCard->SetWorkCity(aTagValue->get());
  else if (aTagName->Equals(kServerWorkStateColumn))
    aCard->SetWorkState(aTagValue->get());
  else if (aTagName->Equals(kServerWorkZipCodeColumn))
    aCard->SetWorkZipCode(aTagValue->get());
  else if (aTagName->Equals(kServerWorkCountryColumn))
    aCard->SetWorkCountry(aTagValue->get());
  else if (aTagName->Equals(kServerJobTitleColumn))
    aCard->SetJobTitle(aTagValue->get());
  else if (aTagName->Equals(kServerNotesColumn))
    aCard->SetNotes(aTagValue->get());
  else if (aTagName->Equals(kServerWebPage1Column))
    aCard->SetWebPage1(aTagValue->get());
  else if (aTagName->Equals(kServerDepartmentColumn))
    aCard->SetDepartment(aTagValue->get());
  else if (aTagName->Equals(kServerCompanyColumn))
    aCard->SetCompany(aTagValue->get());
  else if (aTagName->Equals(kServerAimScreenNameColumn))
    aCard->SetAimScreenName(aTagValue->get());
  else if (aTagName->Equals(kServerBirthdayColumn))
    ParseDateFields(aCard, aTagName, aTagValue);
  else if (aTagName->Equals(kServerAnniversaryColumn))
    ParseDateFields(aCard, aTagName, aTagValue);
  else if (aTagName->Equals(kServerSpouseNameColumn))
    aCard->SetSpouseName(aTagValue->get());
  else if (aTagName->Equals(kServerFamilyNameColumn))
    aCard->SetFamilyName(aTagValue->get());
  else if (aTagName->Equals(kServerDefaultAddressColumn))
    aCard->SetDefaultAddress(aTagValue->get());
  else if (aTagName->Equals(kServerCategoryColumn))
    aCard->SetCategory(aTagValue->get());
  else if (aTagName->Equals(kServerWebPage2Column))
    aCard->SetWebPage2(aTagValue->get());
  else if (aTagName->Equals(kServerCustom1Column))
    aCard->SetCustom1(aTagValue->get());
  else if (aTagName->Equals(kServerCustom2Column))
    aCard->SetCustom2(aTagValue->get());
  else if (aTagName->Equals(kServerCustom3Column))
    aCard->SetCustom3(aTagValue->get());
  else if (aTagName->Equals(kServerCustom4Column))
    aCard->SetCustom4(aTagValue->get());
  else if (aTagName->Equals(kServerPlainTextColumn))
  {
    // This is plain text pref...have to add a little logic.
    if (aTagValue->Equals(NS_LITERAL_STRING("1")))
      aCard->SetPreferMailFormat(nsIAbPreferMailFormat::html);
    else if (aTagValue->Equals(NS_LITERAL_STRING("0")))
      aCard->SetPreferMailFormat(nsIAbPreferMailFormat::unknown);
  }

  return rv;
}

#define AB_STRING_URL       "chrome://messenger/locale/addressbook/absync.properties"

PRUnichar *
nsAbSync::GetString(const PRUnichar *aStringName)
{
	nsresult    res = NS_OK;
  PRUnichar   *ptrv = nsnull;

	if (!mStringBundle)
	{
		static const char propertyURL[] = AB_STRING_URL;

		nsCOMPtr<nsIStringBundleService> sBundleService = do_GetService(NS_STRINGBUNDLE_CONTRACTID, &res); 
		if (NS_SUCCEEDED(res) && (nsnull != sBundleService)) 
		{
			res = sBundleService->CreateBundle(propertyURL, getter_AddRefs(mStringBundle));
		}
	}

	if (mStringBundle)
		res = mStringBundle->GetStringFromName(aStringName, &ptrv);

  if ( NS_SUCCEEDED(res) && (ptrv) )
    return ptrv;
  else
    return nsCRT::strdup(aStringName);
}

nsresult nsAbSync::RecoverUserSyncRecords(PRUint32 numOfCards, nsIEnumerator *cardEnum)
{
  // Ok, when this function is called it means that the last operation must have
  // failed and we should build a table of the CRC's of the address book we have
  // locally and prevent us from adding the same person twice. Hope this works.
  //
  NS_ENSURE_ARG_POINTER(cardEnum);

  mCrashTableSize = numOfCards;
  nsresult rv = NS_OK;  
  mLastSyncFailed = PR_TRUE;

  mCrashTable = (syncMappingRecord *) PR_MALLOC(mCrashTableSize * sizeof(syncMappingRecord));
  if (!mCrashTable)
  {
    mCrashTableSize = 0;
  }
  else
  {
    // Init the memory!
    memset(mCrashTable, 0, (mCrashTableSize * sizeof(syncMappingRecord)) );
    nsString        tProtLine; 
    PRUint32        workCounter = 0;
    nsCOMPtr<nsISupports> obj = nsnull;

    cardEnum->First();
    do
    {
      if (NS_FAILED(cardEnum->CurrentItem(getter_AddRefs(obj))))
        break;
      else
      {
        nsCOMPtr<nsIAbCard> card;
        card = do_QueryInterface(obj, &rv);
        if ( NS_SUCCEEDED(rv) && (card) )
        {
          // First, we need to fill out the localID for this entry. This should
          // be the ID from the local database for this card entry
          //
          PRUint32    aKey;
          nsresult rv = NS_OK;
          nsCOMPtr<nsIAbMDBCard> dbcard(do_QueryInterface(card, &rv)); 
          if (NS_FAILED(rv) || !dbcard)
            continue;
          if (NS_FAILED(dbcard->GetKey(&aKey)))
            continue;

          // Ugh...this should never happen...BUT??
          if (aKey <= 0)
            continue;

          // Ok, now get the data for this record....
          mCrashTable[workCounter].localID = aKey;
          if (NS_SUCCEEDED(GenerateProtocolForCard(card, PR_FALSE, tProtLine)))
            mCrashTable[workCounter].CRC = GetCRC((char *)NS_ConvertUCS2toUTF8(tProtLine).get());

        }
      }

      workCounter++;
    } while (NS_SUCCEEDED(cardEnum->Next()));
  }

  return rv;
}

nsresult nsAbSync::LoadUsersFromHistoryFile()
{
  if (!mHistoryFile)
    return NS_ERROR_FAILURE;

  PRUint32 fileSize;
  nsresult rv = mHistoryFile->GetFileSize(&fileSize);
  NS_ENSURE_SUCCESS(rv,rv);

  rv = mHistoryFile->OpenStreamForReading();
  NS_ENSURE_SUCCESS(rv,rv);

  mOldSyncMapingTable = (syncMappingRecord *) PR_MALLOC(fileSize);
  if (!mOldSyncMapingTable)
    return(NS_ERROR_OUT_OF_MEMORY);

  // Init the memory!
  memset(mOldSyncMapingTable, 0, fileSize);

  // Now get the number of records in the table size!
  mOldTableCount = fileSize / sizeof(syncMappingRecord);

  // Now read the history file into memory!
  PRUint32 readCount = 0;
  PRInt32  readSize = 0;
  while (readCount < mOldTableCount)
  {
    syncMappingRecord   *tRecord = &mOldSyncMapingTable[readCount];
    rv = mHistoryFile->Read((char **)&tRecord, sizeof(syncMappingRecord), &readSize);
    if (NS_FAILED(rv) || (readSize != sizeof(syncMappingRecord)))
      break;

#ifdef DEBUG_ABSYNC
  printf("------ Entry #%d --------\n", readCount);
  printf("Old Sync Table: %d\n", mOldSyncMapingTable[readCount].serverID);
  printf("Old Sync Table: %d\n", mOldSyncMapingTable[readCount].localID);
  printf("Old Sync Table: %d\n", mOldSyncMapingTable[readCount].CRC);
  printf("Old Sync Table: %d\n", mOldSyncMapingTable[readCount].flags);
#endif

    readCount++;
  }
  mHistoryFile->CloseStream();
  return(NS_OK);
}

nsresult nsAbSync::SaveCurrentUsersToHistoryFile()
{
  if (!mHistoryFile)
    return NS_ERROR_FAILURE;

  nsresult rv = mHistoryFile->OpenStreamForWriting();
  NS_ENSURE_SUCCESS(rv,rv);

  // Ok, these are the lists that exist when we started the sync op.
  PRInt32 writeSize = 0;
  PRUint32 writeCount = 0;
  while (writeCount < mNewTableCount)
  {
    // Sanity one more time...
    if (mNewSyncMapingTable[writeCount].serverID != 0)
    {
      rv = mHistoryFile->Write((char *)&(mNewSyncMapingTable[writeCount]), sizeof(syncMappingRecord), &writeSize);
      if (NS_FAILED(rv) || (writeSize != sizeof(syncMappingRecord)))
      {
        mHistoryFile->CloseStream();
        return rv;
      }
    }

    writeCount++;
  }

  /// These are the lists that we got back from the server and are new to us now!
  writeCount = 0;
  if (mNewServerTable)
  {
    while (writeCount < (PRUint32) mNewServerTable->Count())
    {
      syncMappingRecord *tRec = (syncMappingRecord *)mNewServerTable->ElementAt(writeCount);
      if (!tRec)
      {
        writeCount++;
        continue;
      }
      rv = mHistoryFile->Write((char *)(tRec), sizeof(syncMappingRecord), &writeSize);
      if (NS_FAILED(rv) || (writeSize != sizeof(syncMappingRecord)))
      {
        mHistoryFile->CloseStream();
        return rv;
      }

      writeCount++;
    }
  }

  mHistoryFile->CloseStream();
  return NS_OK;
}

void nsAbSync::CheckDeletedRecords()
{
  // If a record has not been marked PROCESSED then it has been deleted.
  //
  PRUint32 readCount = 0;
  while (readCount < mOldTableCount)
  {
    if (!(mOldSyncMapingTable[readCount].flags & SYNC_PROCESSED))
    {
      // We can ignore SYNC_IS_AOL_ADDITIONAL_EMAIL since it doesn't exist on AOL side.
      if (mOldSyncMapingTable[readCount].flags & SYNC_IS_AOL_ADDITIONAL_EMAIL)
      {
        readCount++;
        continue;
      }

      // Need the separator for multiple operations...
      if (!mPostString.IsEmpty())
        mPostString.Append(NS_LITERAL_STRING("&"));

#ifdef DEBUG_ABSYNC
  printf("ABSYNC: DELETING Card: %d\n", mOldSyncMapingTable[readCount].serverID);
#endif

      mPostString.AppendInt(mCurrentPostRecord);
      mPostString.Append(NS_LITERAL_STRING("="));

      // If it's a list use 'list_id=' with 'maillistDel' cmd. Otherwise if it's an 
      // AOL group use 'grpDel' cmd instead of 'del' cmd. Use 'id=' in both cases.
      // All members will be deleted once the list is removed.
      if (mOldSyncMapingTable[readCount].flags & SYNC_IS_AOL_GROUPS)
        mPostString.Append(NS_ConvertASCIItoUCS2(SYNC_ESCAPE_DELGROUP) + NS_LITERAL_STRING("%26id="));
      else if (mOldSyncMapingTable[readCount].flags & SYNC_IS_CARD)
        mPostString.Append(NS_ConvertASCIItoUCS2(SYNC_ESCAPE_DEL) + NS_LITERAL_STRING("%26id="));
      else if (mOldSyncMapingTable[readCount].flags & SYNC_IS_LIST)
        mPostString.Append(NS_ConvertASCIItoUCS2(SYNC_ESCAPE_DELLIST) + NS_LITERAL_STRING("%26list_id="));
      // Now append the server id for this card
      mPostString.AppendInt(mOldSyncMapingTable[readCount].serverID);

      mCurrentPostRecord++;
    }

    readCount++;
  }
}

PRUint32 nsAbSync::WhichCardType(nsIAbCard *card)
{
  nsXPIDLString cardType;
  nsresult rv = card->GetCardType(getter_Copies(cardType));
  if (cardType && cardType.Equals(NS_LITERAL_STRING(AB_CARD_IS_AOL_GROUPS)))
   return SYNC_IS_AOL_GROUPS;
  else if (cardType && cardType.Equals(NS_LITERAL_STRING(AB_CARD_IS_AOL_ADDITIONAL_EMAIL)))
    return SYNC_IS_AOL_ADDITIONAL_EMAIL;
  else
    return SYNC_IS_CARD;
}

nsresult nsAbSync::InitUserSyncTable(PRUint32 numOfCards, nsIEnumerator *cardEnum)
{
  mNewTableCount = numOfCards;
  mNewSyncMapingTable = (syncMappingRecord *) PR_MALLOC(mNewTableCount * sizeof(syncMappingRecord));
  if (!mNewSyncMapingTable)
    return NS_ERROR_OUT_OF_MEMORY;

  // Init the memory!
  memset(mNewSyncMapingTable, 0, (mNewTableCount * sizeof(syncMappingRecord)) );
  return NS_OK;
}

// This function is used to parse server protocol lines and log them line-by-line for
// easier reading. Note that absync server data lines are separated by a single '\10'.
void nsAbSync::ParseAndLogServerData(const char *logData)
{
  if (!logData || !(*logData))
    return;

  nsCAutoString workStr;
  char *pChar, *start, *end;

  workStr.Assign(logData);
  pChar = start = (char *)workStr.get();
  end = start + strlen(start);
  while (start < end)
  {
    while ((pChar < end) && (*pChar != nsCRT::LF))
      pChar++;

    if (pChar < end)
    {
      // Found a line so log it.
      *pChar = 0;
      Log("  Protocol data received from server", start);
      pChar++;
      start = pChar;
    }
    else if (start < end)
    {
      // Log the last line and we're done.
      *pChar = 0;
      Log("  Protocol data received from server", start);
      break;
    }
  }
}

#define LOG_LINE_BUF_SIZE 256

void nsAbSync::Log(const char *logSubName, char *logData)
{
  if (!logData || !logSubName)
    return;

  if (PR_LOG_TEST(ABSYNC, PR_LOG_ALWAYS))
  {
    PRInt32 length = strlen(logData);
    if (length > LOG_LINE_BUF_SIZE)
    {
      // PR_LOG() only writes out 512 (LINE_BUF_SIZE in prlog.c) char buffer
      // at a time so we need to fold the input data here. Note that depending 
      // on the CRLF in the log data the output buffer may be 20 or 40 bytes 
      // less than the original data, so it's safer to make our log data much 
      // smaller than 512. 
      char saveChar, *pChar = logData;
      while (1)
      {
        saveChar = *(pChar + LOG_LINE_BUF_SIZE);
        *(pChar + LOG_LINE_BUF_SIZE) = 0;
        PR_LOG(ABSYNC, PR_LOG_ALWAYS, ("%s: %s", logSubName, pChar));
        *(pChar + LOG_LINE_BUF_SIZE) = saveChar;
        length -= LOG_LINE_BUF_SIZE;
        pChar = pChar + LOG_LINE_BUF_SIZE;
        if (length < LOG_LINE_BUF_SIZE)
        {
          // Last block of data
          PR_LOG(ABSYNC, PR_LOG_ALWAYS, ("%s: %s", logSubName, pChar));
          break;
        }
      }
    }
    else
      PR_LOG(ABSYNC, PR_LOG_ALWAYS, ("%s: %s", logSubName, logData));
  }
}
