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
 *   Jan Varga (varga@ku.sk)
 *   Håkan Waara (hwaara@chello.se)
 *   Jeremy Morton (bugzilla@game-point.net)
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
#include "prmem.h"
#include "nsReadableUtils.h"
#include "nsIMsgCustomColumnHandler.h"
#include "nsMsgDBView.h"
#include "nsISupports.h"
#include "nsIMsgFolder.h"
#include "nsIDBFolderInfo.h"
#include "nsIMsgDatabase.h"
#include "nsIMsgFolder.h"
#include "MailNewsTypes2.h"
#include "nsMsgUtils.h"
#include "nsQuickSort.h"
#include "nsIMsgImapMailFolder.h"
#include "nsImapCore.h"
#include "nsMsgFolderFlags.h"
#include "nsIMsgLocalMailFolder.h"
#include "nsIDOMElement.h"
#include "nsDateTimeFormatCID.h"
#include "nsMsgMimeCID.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsIPrefBranch2.h"
#include "nsIPrefLocalizedString.h"
#include "nsIMsgSearchSession.h"
#include "nsIMsgCopyService.h"
#include "nsMsgBaseCID.h"
#include "nsISpamSettings.h"
#include "nsIMsgAccountManager.h"
#include "nsITreeColumns.h"
#include "nsTextFormatter.h"
#include "nsIMutableArray.h"

nsrefcnt nsMsgDBView::gInstanceCount  = 0;

#ifdef SUPPORT_PRIORITY_COLORS
nsIAtom * nsMsgDBView::kHighestPriorityAtom = nsnull;
nsIAtom * nsMsgDBView::kHighPriorityAtom = nsnull;
nsIAtom * nsMsgDBView::kLowestPriorityAtom = nsnull;
nsIAtom * nsMsgDBView::kLowPriorityAtom = nsnull;
#endif

nsIAtom * nsMsgDBView::kUnreadMsgAtom  = nsnull;
nsIAtom * nsMsgDBView::kNewMsgAtom = nsnull;
nsIAtom * nsMsgDBView::kReadMsgAtom  = nsnull;
nsIAtom * nsMsgDBView::kRepliedMsgAtom = nsnull;
nsIAtom * nsMsgDBView::kForwardedMsgAtom = nsnull;
nsIAtom * nsMsgDBView::kOfflineMsgAtom  = nsnull;
nsIAtom * nsMsgDBView::kFlaggedMsgAtom = nsnull;
nsIAtom * nsMsgDBView::kImapDeletedMsgAtom = nsnull;
nsIAtom * nsMsgDBView::kAttachMsgAtom = nsnull;
nsIAtom * nsMsgDBView::kHasUnreadAtom = nsnull;
nsIAtom * nsMsgDBView::kWatchThreadAtom = nsnull;
nsIAtom * nsMsgDBView::kIgnoreThreadAtom = nsnull;
nsIAtom * nsMsgDBView::kIgnoreSubthreadAtom = nsnull;
nsIAtom * nsMsgDBView::kHasImageAtom = nsnull;

nsIAtom * nsMsgDBView::kJunkMsgAtom = nsnull;
nsIAtom * nsMsgDBView::kNotJunkMsgAtom = nsnull;
nsIAtom * nsMsgDBView::kDummyMsgAtom = nsnull;

nsIAtom * nsMsgDBView::kLabelColorWhiteAtom = nsnull;
nsIAtom * nsMsgDBView::kLabelColorBlackAtom = nsnull;

PRUnichar * nsMsgDBView::kHighestPriorityString = nsnull;
PRUnichar * nsMsgDBView::kHighPriorityString = nsnull;
PRUnichar * nsMsgDBView::kLowestPriorityString = nsnull;
PRUnichar * nsMsgDBView::kLowPriorityString = nsnull;
PRUnichar * nsMsgDBView::kNormalPriorityString = nsnull;
PRUnichar * nsMsgDBView::kReadString = nsnull;
PRUnichar * nsMsgDBView::kRepliedString = nsnull;
PRUnichar * nsMsgDBView::kForwardedString = nsnull;
PRUnichar * nsMsgDBView::kNewString = nsnull;

PRUnichar * nsMsgDBView::kKiloByteString = nsnull;

nsDateFormatSelector  nsMsgDBView::m_dateFormatDefault = kDateFormatShort;
nsDateFormatSelector  nsMsgDBView::m_dateFormatThisWeek = kDateFormatShort;
nsDateFormatSelector  nsMsgDBView::m_dateFormatToday = kDateFormatNone;

static const PRUint32 kMaxNumSortColumns = 2;

// this is passed into NS_QuickSort as custom data.
class viewSortInfo
{
public:
  nsMsgDBView *view;
  nsIMsgDatabase *db;
  PRBool isSecondarySort;
  PRBool ascendingSort;
};


NS_IMPL_ADDREF(nsMsgDBView)
NS_IMPL_RELEASE(nsMsgDBView)

NS_INTERFACE_MAP_BEGIN(nsMsgDBView)
   NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIMsgDBView)
   NS_INTERFACE_MAP_ENTRY(nsIMsgDBView)
   NS_INTERFACE_MAP_ENTRY(nsIDBChangeListener)
   NS_INTERFACE_MAP_ENTRY(nsITreeView)
   NS_INTERFACE_MAP_ENTRY(nsIJunkMailClassificationListener)
NS_INTERFACE_MAP_END

nsMsgDBView::nsMsgDBView()
{
  /* member initializers and constructor code */
  m_sortValid = PR_FALSE;
  m_sortOrder = nsMsgViewSortOrder::none;
  m_viewFlags = nsMsgViewFlagsType::kNone;
  m_secondarySort = nsMsgViewSortType::byId;
  m_secondarySortOrder = nsMsgViewSortOrder::ascending;
  m_cachedMsgKey = nsMsgKey_None;
  m_currentlyDisplayedMsgKey = nsMsgKey_None;
  m_currentlyDisplayedViewIndex = nsMsgViewIndex_None;
  mNumSelectedRows = 0;
  mSuppressMsgDisplay = PR_FALSE;
  mSuppressCommandUpdating = PR_FALSE;
  mSuppressChangeNotification = PR_FALSE;
  mGoForwardEnabled = PR_FALSE;
  mGoBackEnabled = PR_FALSE;

  mIsNews = PR_FALSE;
  mDeleteModel = nsMsgImapDeleteModels::MoveToTrash;
  m_deletingRows = PR_FALSE;
  mJunkIndices = nsnull;
  mNumJunkIndices = 0;
  mNumMessagesRemainingInBatch = 0;
  mShowSizeInLines = PR_FALSE;

  /* mCommandsNeedDisablingBecauseOfSelection - A boolean that tell us if we needed to disable commands because of what's selected.
    If we're offline w/o a downloaded msg selected, or a dummy message was selected.
  */

  mCommandsNeedDisablingBecauseOfSelection = PR_FALSE;
  mRemovingRow = PR_FALSE;
  m_saveRestoreSelectionDepth = 0;
  mRecentlyDeletedArrayIndex = 0;
  // initialize any static atoms or unicode strings
  if (gInstanceCount == 0)
  {
    InitializeAtomsAndLiterals();
    InitDisplayFormats();
  }

  InitLabelStrings();
  gInstanceCount++;
}

void nsMsgDBView::InitializeAtomsAndLiterals()
{
  kUnreadMsgAtom = NS_NewAtom("unread");
  kNewMsgAtom = NS_NewAtom("new");
  kReadMsgAtom = NS_NewAtom("read");
  kRepliedMsgAtom = NS_NewAtom("replied");
  kForwardedMsgAtom = NS_NewAtom("forwarded");
  kOfflineMsgAtom = NS_NewAtom("offline");
  kFlaggedMsgAtom = NS_NewAtom("flagged");
  kImapDeletedMsgAtom = NS_NewAtom("imapdeleted");
  kAttachMsgAtom = NS_NewAtom("attach");
  kHasUnreadAtom = NS_NewAtom("hasUnread");
  kWatchThreadAtom = NS_NewAtom("watch");
  kIgnoreThreadAtom = NS_NewAtom("ignore");
  kIgnoreSubthreadAtom = NS_NewAtom("ignoreSubthread");
  kHasImageAtom = NS_NewAtom("hasimage");
  kJunkMsgAtom = NS_NewAtom("junk");
  kNotJunkMsgAtom = NS_NewAtom("notjunk");
  kDummyMsgAtom = NS_NewAtom("dummy");
#ifdef SUPPORT_PRIORITY_COLORS
  kHighestPriorityAtom = NS_NewAtom("priority-highest");
  kHighPriorityAtom = NS_NewAtom("priority-high");
  kLowestPriorityAtom = NS_NewAtom("priority-lowest");
  kLowPriorityAtom = NS_NewAtom("priority-low");
#endif

  // priority strings
  kHighestPriorityString = GetString(NS_LITERAL_STRING("priorityHighest").get());
  kHighPriorityString = GetString(NS_LITERAL_STRING("priorityHigh").get());
  kLowestPriorityString = GetString(NS_LITERAL_STRING("priorityLowest").get());
  kLowPriorityString = GetString(NS_LITERAL_STRING("priorityLow").get());
  kNormalPriorityString = GetString(NS_LITERAL_STRING("priorityNormal").get());

  kLabelColorWhiteAtom = NS_NewAtom("lc-white");
  kLabelColorBlackAtom = NS_NewAtom("lc-black");

  kReadString = GetString(NS_LITERAL_STRING("read").get());
  kRepliedString = GetString(NS_LITERAL_STRING("replied").get());
  kForwardedString = GetString(NS_LITERAL_STRING("forwarded").get());
  kNewString = GetString(NS_LITERAL_STRING("new").get());

  kKiloByteString = GetString(NS_LITERAL_STRING("kiloByteAbbreviation").get());
}

nsMsgDBView::~nsMsgDBView()
{
  if (m_db)
    m_db->RemoveListener(this);

  gInstanceCount--;
  if (gInstanceCount <= 0)
  {
    NS_IF_RELEASE(kUnreadMsgAtom);
    NS_IF_RELEASE(kNewMsgAtom);
    NS_IF_RELEASE(kReadMsgAtom);
    NS_IF_RELEASE(kRepliedMsgAtom);
    NS_IF_RELEASE(kForwardedMsgAtom);
    NS_IF_RELEASE(kOfflineMsgAtom);
    NS_IF_RELEASE(kFlaggedMsgAtom);
    NS_IF_RELEASE(kImapDeletedMsgAtom);
    NS_IF_RELEASE(kAttachMsgAtom);
    NS_IF_RELEASE(kHasUnreadAtom);
    NS_IF_RELEASE(kWatchThreadAtom);
    NS_IF_RELEASE(kIgnoreThreadAtom);
    NS_IF_RELEASE(kIgnoreSubthreadAtom);
    NS_IF_RELEASE(kHasImageAtom);
    NS_IF_RELEASE(kJunkMsgAtom);
    NS_IF_RELEASE(kNotJunkMsgAtom);
    NS_IF_RELEASE(kDummyMsgAtom);

#ifdef SUPPORT_PRIORITY_COLORS
    NS_IF_RELEASE(kHighestPriorityAtom);
    NS_IF_RELEASE(kHighPriorityAtom);
    NS_IF_RELEASE(kLowestPriorityAtom);
    NS_IF_RELEASE(kLowPriorityAtom);
#endif

    NS_IF_RELEASE(kLabelColorWhiteAtom);
    NS_IF_RELEASE(kLabelColorBlackAtom);

    NS_Free(kHighestPriorityString);
    NS_Free(kHighPriorityString);
    NS_Free(kLowestPriorityString);
    NS_Free(kLowPriorityString);
    NS_Free(kNormalPriorityString);

    NS_Free(kReadString);
    NS_Free(kRepliedString);
    NS_Free(kForwardedString);
    NS_Free(kNewString);
    NS_Free(kKiloByteString);
  }
}

nsresult nsMsgDBView::InitLabelStrings()
{
  nsresult rv = NS_OK;
  nsCString prefString;

  for(PRInt32 i = 0; i < PREF_LABELS_MAX; i++)
  {
    prefString.Assign(PREF_LABELS_DESCRIPTION);
    prefString.AppendInt(i + 1);
    rv = GetPrefLocalizedString(prefString.get(), mLabelPrefDescriptions[i]);
  }
  return rv;
}

// helper function used to fetch strings from the messenger string bundle
PRUnichar * nsMsgDBView::GetString(const PRUnichar *aStringName)
{
  nsresult    res = NS_OK;
  PRUnichar   *ptrv = nsnull;

  if (!mMessengerStringBundle)
  {
    static const char propertyURL[] = MESSENGER_STRING_URL;
    nsCOMPtr<nsIStringBundleService> sBundleService = do_GetService(NS_STRINGBUNDLE_CONTRACTID, &res);
    if (NS_SUCCEEDED(res) && sBundleService)
      res = sBundleService->CreateBundle(propertyURL, getter_AddRefs(mMessengerStringBundle));
  }

  if (mMessengerStringBundle)
    res = mMessengerStringBundle->GetStringFromName(aStringName, &ptrv);

  if ( NS_SUCCEEDED(res) && (ptrv) )
    return ptrv;
  else
    return NS_strdup(aStringName);
}

// helper function used to fetch localized strings from the prefs
nsresult nsMsgDBView::GetPrefLocalizedString(const char *aPrefName, nsString& aResult)
{
  nsresult rv = NS_OK;
  nsCOMPtr<nsIPrefBranch> prefBranch;
  nsCOMPtr<nsIPrefLocalizedString> pls;
  nsString ucsval;

  prefBranch = do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = prefBranch->GetComplexValue(aPrefName, NS_GET_IID(nsIPrefLocalizedString), getter_AddRefs(pls));
  NS_ENSURE_SUCCESS(rv, rv);
  pls->ToString(getter_Copies(ucsval));
  aResult = ucsval.get();
  return rv;
}

nsresult nsMsgDBView::AppendKeywordProperties(const char *keywords, nsISupportsArray *properties, PRBool addSelectedTextProperty)
{
  // get the top most keyword's color and append that as a property.
  nsresult rv;
  if (!mTagService)
  {
    mTagService = do_GetService(NS_MSGTAGSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCString topKey;
  rv = mTagService->GetTopKey(keywords, topKey);
  NS_ENSURE_SUCCESS(rv, rv);
  if (topKey.IsEmpty())
    return NS_OK;

  nsCString color;
  rv = mTagService->GetColorForKey(topKey, color);
  if (NS_SUCCEEDED(rv) && !color.IsEmpty())
  {
    if (addSelectedTextProperty)
      properties->AppendElement(color.EqualsLiteral(LABEL_COLOR_WHITE_STRING)
        ? kLabelColorBlackAtom
        : kLabelColorWhiteAtom);
    color.Replace(0, 1, NS_LITERAL_CSTRING(LABEL_COLOR_STRING));
    nsCOMPtr <nsIAtom> keywordAtom = do_GetAtom(color.get());
    properties->AppendElement(keywordAtom);
  }
  return rv;
}

///////////////////////////////////////////////////////////////////////////
// nsITreeView Implementation Methods (and helper methods)
///////////////////////////////////////////////////////////////////////////

nsresult nsMsgDBView::FetchAuthor(nsIMsgDBHdr * aHdr, nsAString &aSenderString)
{
  nsString unparsedAuthor;
  if (!mHeaderParser)
    mHeaderParser = do_GetService(NS_MAILNEWS_MIME_HEADER_PARSER_CONTRACTID);

  nsresult rv = aHdr->GetMime2DecodedAuthor(unparsedAuthor);

  // *sigh* how sad, we need to convert our beautiful unicode string to utf8
  // so we can extract the name part of the address...then convert it back to
  // unicode again.
  if (mHeaderParser)
  {
    nsCString name;
    rv = mHeaderParser->ExtractHeaderAddressName("UTF-8", NS_ConvertUTF16toUTF8(unparsedAuthor).get(), getter_Copies(name));
    if (NS_SUCCEEDED(rv) && !name.IsEmpty())
    {
      CopyUTF8toUTF16(name, aSenderString);
      return NS_OK;
    }
  }
  // if we got here then just return the original string
  aSenderString = unparsedAuthor;
  return NS_OK;
}

nsresult nsMsgDBView::FetchAccount(nsIMsgDBHdr * aHdr, nsAString& aAccount)
{
  nsCString accountKey;

  nsresult rv = aHdr->GetAccountKey(getter_Copies(accountKey));

  // Cache the account manager?
  nsCOMPtr <nsIMsgAccountManager> accountManager = do_GetService(NS_MSGACCOUNTMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv,rv);
  nsCOMPtr <nsIMsgAccount> account;
  if (!accountKey.IsEmpty())
    rv = accountManager->GetAccount(accountKey, getter_AddRefs(account));
  if (account)
  {
    nsCOMPtr <nsIMsgIncomingServer> server;
    account->GetIncomingServer(getter_AddRefs(server));
    if (server)
      server->GetPrettyName(aAccount);
  }
  else
    CopyASCIItoUTF16(accountKey, aAccount);
  return NS_OK;
}

nsresult nsMsgDBView::FetchRecipients(nsIMsgDBHdr * aHdr, nsAString &aRecipientsString)
{
  nsString unparsedRecipients;
  if (!mHeaderParser)
    mHeaderParser = do_GetService(NS_MAILNEWS_MIME_HEADER_PARSER_CONTRACTID);

  nsresult rv = aHdr->GetMime2DecodedRecipients(unparsedRecipients);

  // *sigh* how sad, we need to convert our beautiful unicode string to utf8
  // so we can extract the name part of the address...then convert it back to
  // unicode again.
  if (mHeaderParser)
  {
    nsCString names;
    rv = mHeaderParser->ExtractHeaderAddressNames("UTF-8", NS_ConvertUTF16toUTF8(unparsedRecipients).get(), getter_Copies(names));
    if (NS_SUCCEEDED(rv) && !names.IsEmpty())
    {
      CopyUTF8toUTF16(names, aRecipientsString);
      return NS_OK;
    }
  }

  aRecipientsString = unparsedRecipients;
  return NS_OK;
}

nsresult nsMsgDBView::FetchSubject(nsIMsgDBHdr * aMsgHdr, PRUint32 aFlags, nsAString &aValue)
{
  if (aFlags & MSG_FLAG_HAS_RE)
  {
    nsString subject;
    aMsgHdr->GetMime2DecodedSubject(subject);
    aValue.AssignLiteral("Re: ");
    aValue.Append(subject);
  }
  else
    aMsgHdr->GetMime2DecodedSubject(aValue);
  return NS_OK;
}

// in case we want to play around with the date string, I've broken it out into
// a separate routine.  Set rcvDate to PR_TRUE to get the Received: date instead
// of the Date: date.
nsresult nsMsgDBView::FetchDate(nsIMsgDBHdr * aHdr, nsAString &aDateString, PRBool rcvDate)
{
  PRTime dateOfMsg;
  PRTime dateOfMsgLocal;
  PRUint32 rcvDateSecs;
  nsresult rv;

  if (!mDateFormatter)
    mDateFormatter = do_CreateInstance(NS_DATETIMEFORMAT_CONTRACTID);

  NS_ENSURE_TRUE(mDateFormatter, NS_ERROR_FAILURE);
  // Silently return Date: instead if Received: is unavailable
  if (rcvDate)
  {
    rv = aHdr->GetUint32Property("dateReceived", &rcvDateSecs);
    if (rcvDateSecs != 0)
      Seconds2PRTime(rcvDateSecs, &dateOfMsg);
  }
  if (!rcvDate || rcvDateSecs == 0)
    rv = aHdr->GetDate(&dateOfMsg);

  PRTime currentTime = PR_Now();
  PRExplodedTime explodedCurrentTime;
  PR_ExplodeTime(currentTime, PR_LocalTimeParameters, &explodedCurrentTime);
  PRExplodedTime explodedMsgTime;
  PR_ExplodeTime(dateOfMsg, PR_LocalTimeParameters, &explodedMsgTime);

  // if the message is from today, don't show the date, only the time. (i.e. 3:15 pm)
  // if the message is from the last week, show the day of the week.   (i.e. Mon 3:15 pm)
  // in all other cases, show the full date (03/19/01 3:15 pm)

  nsDateFormatSelector dateFormat = m_dateFormatDefault;
  if (explodedCurrentTime.tm_year == explodedMsgTime.tm_year &&
      explodedCurrentTime.tm_month == explodedMsgTime.tm_month &&
      explodedCurrentTime.tm_mday == explodedMsgTime.tm_mday)
  {
    // same day...
    dateFormat = m_dateFormatToday;
  }
  // the following chunk of code allows us to show a day instead of a number if the message was received
  // within the last 7 days. i.e. Mon 5:10pm. (depending on the mail.ui.display.dateformat.thisweek pref)
  // The concrete format used is dependent on a preference setting (see InitDisplayFormats).
  else if (LL_CMP(currentTime, >, dateOfMsg))
  {
    // some constants for calculation
    static PRInt64 microSecondsPerSecond;
    static PRInt64 secondsPerDay;
    static PRInt64 microSecondsPerDay;
    static PRInt64 microSecondsPer6Days;

    static PRBool bGotConstants = PR_FALSE;
    if ( !bGotConstants )
    {
      // seeds
      LL_I2L  ( microSecondsPerSecond,  PR_USEC_PER_SEC );
      LL_UI2L ( secondsPerDay,          60 * 60 * 24 );

      // derivees
      LL_MUL( microSecondsPerDay,   secondsPerDay,      microSecondsPerSecond );
      LL_MUL( microSecondsPer6Days, microSecondsPerDay, 6 );

      bGotConstants = PR_TRUE;
    }

    // setting the time variables to local time
    PRInt64 GMTLocalTimeShift;
    LL_ADD( GMTLocalTimeShift, explodedCurrentTime.tm_params.tp_gmt_offset, explodedCurrentTime.tm_params.tp_dst_offset );
    LL_MUL( GMTLocalTimeShift, GMTLocalTimeShift, microSecondsPerSecond );
    LL_ADD( currentTime, currentTime, GMTLocalTimeShift );
    LL_ADD( dateOfMsgLocal, dateOfMsg, GMTLocalTimeShift );

    // the most recent midnight, counting from current time
    PRInt64 todaysMicroSeconds, mostRecentMidnight;
    LL_MOD( todaysMicroSeconds, currentTime, microSecondsPerDay );
    LL_SUB( mostRecentMidnight, currentTime, todaysMicroSeconds );

    // most recent midnight minus 6 days
    PRInt64 mostRecentWeek;
    LL_SUB( mostRecentWeek, mostRecentMidnight, microSecondsPer6Days );

    // was the message sent during the last week?
    if ( LL_CMP( dateOfMsgLocal, >=, mostRecentWeek ) )
    { // yes ....
      dateFormat = m_dateFormatThisWeek;
    }
  }

  if (NS_SUCCEEDED(rv))
    rv = mDateFormatter->FormatPRTime(nsnull /* nsILocale* locale */,
                                      dateFormat,
                                      kTimeFormatNoSeconds,
                                      dateOfMsg,
                                      aDateString);

  return rv;
}

nsresult nsMsgDBView::FetchStatus(PRUint32 aFlags, nsAString &aStatusString)
{
  if(aFlags & MSG_FLAG_REPLIED)
    aStatusString = kRepliedString;
  else if(aFlags & MSG_FLAG_FORWARDED)
    aStatusString = kForwardedString;
  else if(aFlags & MSG_FLAG_NEW)
    aStatusString = kNewString;
  else if(aFlags & MSG_FLAG_READ)
    aStatusString = kReadString;

  return NS_OK;
}

nsresult nsMsgDBView::FetchSize(nsIMsgDBHdr * aHdr, nsAString &aSizeString)
{
  nsAutoString formattedSizeString;
  PRUint32 msgSize = 0;

  // for news, show the line count, not the size if the user wants so
  if (mShowSizeInLines)
  {
    aHdr->GetLineCount(&msgSize);
    formattedSizeString.AppendInt(msgSize);
  }
  else
  {
    PRUint32 flags = 0;

    aHdr->GetFlags(&flags);
    if (flags & MSG_FLAG_PARTIAL)
      aHdr->GetUint32Property("onlineSize", &msgSize);

    if (msgSize == 0)
      aHdr->GetMessageSize(&msgSize);

    if(msgSize < 1024)
      msgSize = 1024;

    PRUint32 sizeInKB = msgSize/1024;

    // kKiloByteString is a localized string that we use to get the right
    // format to add on the "KB" or equivalent
    nsTextFormatter::ssprintf(formattedSizeString,
                              kKiloByteString,
                              sizeInKB);
  }

  aSizeString = formattedSizeString;
  // the formattingString Length includes the null terminator byte!
  if (!formattedSizeString.Last())
    aSizeString.SetLength(formattedSizeString.Length() - 1);
  return NS_OK;
}

nsresult nsMsgDBView::FetchPriority(nsIMsgDBHdr *aHdr, nsAString & aPriorityString)
{
  nsMsgPriorityValue priority = nsMsgPriority::notSet;
  aHdr->GetPriority(&priority);

  switch (priority)
  {
  case nsMsgPriority::highest:
    aPriorityString = kHighestPriorityString;
    break;
  case nsMsgPriority::high:
    aPriorityString = kHighPriorityString;
    break;
  case nsMsgPriority::low:
    aPriorityString = kLowPriorityString;
    break;
  case nsMsgPriority::lowest:
    aPriorityString = kLowestPriorityString;
    break;
  case nsMsgPriority::normal:
    aPriorityString = kNormalPriorityString;
    break;
  default:
    break;
  }

  return NS_OK;
}

nsresult nsMsgDBView::FetchKeywords(nsIMsgDBHdr *aHdr, nsACString &keywordString)
{
  nsresult rv = NS_OK;
  if (!mTagService)
  {
    mTagService = do_GetService(NS_MSGTAGSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  nsMsgLabelValue label = 0;

  rv = aHdr->GetLabel(&label);
  nsCString keywords;
  aHdr->GetStringProperty("keywords", getter_Copies(keywords));
  if (label > 0)
  {
    nsCAutoString labelStr("$label");
    labelStr.Append((char) (label + '0'));
    if (keywords.Find(labelStr, PR_TRUE) == -1)
    {
      if (!keywords.IsEmpty())
        keywords.Append(' ');
      keywords.Append(labelStr);
    }
  }
  keywordString = keywords;
  return NS_OK;
}

nsresult nsMsgDBView::FetchTags(nsIMsgDBHdr *aHdr, nsAString &aTagString)
{
  nsresult rv = NS_OK;
  if (!mTagService)
  {
    mTagService = do_GetService(NS_MSGTAGSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsString tags;
  nsCString keywords;
  aHdr->GetStringProperty("keywords", getter_Copies(keywords));

  nsMsgLabelValue label = 0;
  rv = aHdr->GetLabel(&label);
  if (label > 0)
  {
    nsCAutoString labelStr("$label");
    labelStr.Append((char) (label + '0'));
    if (keywords.Find(labelStr, PR_TRUE) == -1)
      FetchLabel(aHdr, tags);
  }

  nsCStringArray keywordsArray;
  keywordsArray.ParseString(keywords.get(), " ");
  nsAutoString tag;

  for (PRInt32 i = 0; i < keywordsArray.Count(); i++)
  {
    rv = mTagService->GetTagForKey(*(keywordsArray[i]), tag);
    if (NS_SUCCEEDED(rv) && !tag.IsEmpty())
    {
      if (!tags.IsEmpty())
        tags.Append((PRUnichar) ' ');
      tags.Append(tag);
    }
  }

  aTagString = tags;
  return NS_OK;
}

nsresult nsMsgDBView::FetchLabel(nsIMsgDBHdr *aHdr, nsAString &aLabelString)
{
  nsresult rv = NS_OK;
  nsMsgLabelValue label = 0;

  NS_ENSURE_ARG_POINTER(aHdr);

  rv = aHdr->GetLabel(&label);
  NS_ENSURE_SUCCESS(rv, rv);

  // we don't care if label is not between 1 and PREF_LABELS_MAX inclusive.
  if ((label < 1) || (label > PREF_LABELS_MAX))
  {
    aLabelString.SetLength(0);
    return NS_OK;
  }

  // We need to subtract 1 because mLabelPrefDescriptions is 0 based.
  aLabelString = mLabelPrefDescriptions[label - 1];
  return NS_OK;
}


/*if you call SaveAndClearSelection make sure to call RestoreSelection otherwise
m_saveRestoreSelectionDepth will be incorrect and will lead to selection msg problems*/

nsresult nsMsgDBView::SaveAndClearSelection(nsMsgKey *aCurrentMsgKey, nsTArray<nsMsgKey> &aMsgKeyArray)
{
  // we don't do anything on nested Save / Restore calls.
  m_saveRestoreSelectionDepth++;
  if (m_saveRestoreSelectionDepth != 1)
    return NS_OK;

  if (!mTreeSelection || !mTree)
    return NS_OK;

  // first, freeze selection.
  mTreeSelection->SetSelectEventsSuppressed(PR_TRUE);

  // second, save the current index.
  if (aCurrentMsgKey)
  {
    PRInt32 currentIndex;
    if (NS_SUCCEEDED(mTreeSelection->GetCurrentIndex(&currentIndex)) && currentIndex >= 0 && currentIndex < GetSize())
      *aCurrentMsgKey = m_keys[currentIndex];
    else
      *aCurrentMsgKey = nsMsgKey_None;
  }

  // third, get an array of view indices for the selection.
  nsMsgViewIndexArray selection;
  GetSelectedIndices(selection);
  PRInt32 numIndices = selection.Length();
  aMsgKeyArray.SetLength(numIndices);

  // now store the msg key for each selected item.
  nsMsgKey msgKey;
  for (PRInt32 index = 0; index < numIndices; index++)
  {
    msgKey = m_keys[selection[index]];
    aMsgKeyArray[index] = msgKey;
  }

  // clear the selection, we'll manually restore it later.
  if (mTreeSelection)
    mTreeSelection->ClearSelection();

  return NS_OK;
}

nsresult nsMsgDBView::RestoreSelection(nsMsgKey aCurrentMsgKey, nsTArray<nsMsgKey> &aMsgKeyArray)
{
  // we don't do anything on nested Save / Restore calls.
  m_saveRestoreSelectionDepth--;
  if (m_saveRestoreSelectionDepth)
    return NS_OK;

  if (!mTreeSelection)  // don't assert.
    return NS_OK;

  // turn our message keys into corresponding view indices
  PRInt32 arraySize = aMsgKeyArray.Length();
  nsMsgViewIndex  currentViewPosition = nsMsgViewIndex_None;
  nsMsgViewIndex  newViewPosition = nsMsgViewIndex_None;

  // if we are threaded, we need to do a little more work
  // we need to find (and expand) all the threads that contain messages
  // that we had selected before.
  if (m_viewFlags & nsMsgViewFlagsType::kThreadedDisplay)
  {
    for (PRInt32 index = 0; index < arraySize; index ++)
    {
      FindKey(aMsgKeyArray[index], PR_TRUE /* expand */);
    }
  }

  for (PRInt32 index = 0; index < arraySize; index ++)
  {
    newViewPosition = FindKey(aMsgKeyArray[index], PR_FALSE);
    // add the index back to the selection.
    if (newViewPosition != nsMsgViewIndex_None)
      mTreeSelection->ToggleSelect(newViewPosition);
  }

  // make sure the currentView was preserved....
  if (aCurrentMsgKey != nsMsgKey_None)
    currentViewPosition = FindKey(aCurrentMsgKey, PR_TRUE);

  if (mTree)
    mTreeSelection->SetCurrentIndex(currentViewPosition);

  // make sure the current message is once again visible in the thread pane
  // so we don't have to go search for it in the thread pane
  if (mTree && currentViewPosition != nsMsgViewIndex_None)
    mTree->EnsureRowIsVisible(currentViewPosition);

  // unfreeze selection.
  mTreeSelection->SetSelectEventsSuppressed(PR_FALSE);
  return NS_OK;
}

nsresult nsMsgDBView::GenerateURIForMsgKey(nsMsgKey aMsgKey, nsIMsgFolder *folder, nsACString & aURI)
{
  NS_ENSURE_ARG(folder);
  return folder->GenerateMessageURI(aMsgKey, aURI);
}

nsresult nsMsgDBView::CycleThreadedColumn(nsIDOMElement * aElement)
{
  nsAutoString currentView;

  // toggle threaded/unthreaded mode
  aElement->GetAttribute(NS_LITERAL_STRING("currentView"), currentView);
  if (currentView.EqualsLiteral("threaded"))
    aElement->SetAttribute(NS_LITERAL_STRING("currentView"), NS_LITERAL_STRING("unthreaded"));
  else
     aElement->SetAttribute(NS_LITERAL_STRING("currentView"), NS_LITERAL_STRING("threaded"));

  // i think we need to create a new view and switch it in this circumstance since
  // we are toggline between threaded and non threaded mode.
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::IsEditable(PRInt32 row, nsITreeColumn* col, PRBool* _retval)
{
  NS_ENSURE_ARG_POINTER(col);
  NS_ENSURE_ARG_POINTER(_retval);
  //attempt to retreive a custom column handler. If it exists call it and return
  const PRUnichar* colID;
  col->GetIdConst(&colID);

  nsIMsgCustomColumnHandler* colHandler = GetColumnHandler(colID);

  if (colHandler)
  {
    colHandler->IsEditable(row, col, _retval);
    return NS_OK;
  }

  *_retval = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::IsSelectable(PRInt32 row, nsITreeColumn* col, PRBool* _retval)
{
  *_retval = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::SetCellValue(PRInt32 row, nsITreeColumn* col, const nsAString& value)
{
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::SetCellText(PRInt32 row, nsITreeColumn* col, const nsAString& value)
{
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::GetRowCount(PRInt32 *aRowCount)
{
  *aRowCount = GetSize();
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::GetSelection(nsITreeSelection * *aSelection)
{
  *aSelection = mTreeSelection;
  NS_IF_ADDREF(*aSelection);
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::SetSelection(nsITreeSelection * aSelection)
{
  mTreeSelection = aSelection;
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::ReloadMessageWithAllParts()
{
  if (m_currentlyDisplayedMsgUri.IsEmpty() || mSuppressMsgDisplay)
    return NS_OK;

  nsCAutoString forceAllParts(m_currentlyDisplayedMsgUri);
  forceAllParts += (forceAllParts.FindChar('?') == kNotFound) ? "?" : "&";
  forceAllParts.AppendLiteral("fetchCompleteMessage=true");
  nsCOMPtr<nsIMessenger> messenger (do_QueryReferent(mMessengerWeak));
  return messenger ? messenger->OpenURL(forceAllParts) : NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsMsgDBView::ReloadMessage()
{
  if (m_currentlyDisplayedMsgUri.IsEmpty() || mSuppressMsgDisplay)
    return NS_OK;
  nsCOMPtr<nsIMessenger> messenger (do_QueryReferent(mMessengerWeak));
  return messenger ? messenger->OpenURL(m_currentlyDisplayedMsgUri) : NS_ERROR_FAILURE;
}

nsresult nsMsgDBView::UpdateDisplayMessage(nsMsgViewIndex viewPosition)
{
  nsresult rv;
  if (mCommandUpdater)
  {
    // get the subject and the folder for the message and inform the front end that
    // we changed the message we are currently displaying.
    if (viewPosition != nsMsgViewIndex_None)
    {
      nsCOMPtr <nsIMsgDBHdr> msgHdr;
      rv = GetMsgHdrForViewIndex(viewPosition, getter_AddRefs(msgHdr));
      NS_ENSURE_SUCCESS(rv,rv);

      nsString subject;
      FetchSubject(msgHdr, m_flags[viewPosition], subject);

      nsCString keywords;
      rv = msgHdr->GetStringProperty("keywords", getter_Copies(keywords));
      NS_ENSURE_SUCCESS(rv,rv);

      nsCOMPtr<nsIMsgFolder> folder = m_viewFolder ? m_viewFolder : m_folder;

      mCommandUpdater->DisplayMessageChanged(folder, subject, keywords);

      if (folder)
      {
        rv = folder->SetLastMessageLoaded(m_keys[viewPosition]);
        NS_ENSURE_SUCCESS(rv,rv);
      }
    } // if view position is valid
  } // if we have an updater
  return NS_OK;
}

// given a msg key, we will load the message for it.
NS_IMETHODIMP nsMsgDBView::LoadMessageByMsgKey(nsMsgKey aMsgKey)
{
  return LoadMessageByViewIndex(FindKey(aMsgKey, PR_FALSE));
}

NS_IMETHODIMP nsMsgDBView::LoadMessageByViewIndex(nsMsgViewIndex aViewIndex)
{
  NS_ASSERTION(aViewIndex != nsMsgViewIndex_None,"trying to load nsMsgViewIndex_None");
  if (aViewIndex == nsMsgViewIndex_None) return NS_ERROR_UNEXPECTED;

  nsCString uri;
  nsresult rv = GetURIForViewIndex(aViewIndex, uri);
  if (!mSuppressMsgDisplay && !m_currentlyDisplayedMsgUri.Equals(uri))
  {
    NS_ENSURE_SUCCESS(rv,rv);
    nsCOMPtr<nsIMessenger> messenger (do_QueryReferent(mMessengerWeak));
    NS_ENSURE_TRUE(messenger, NS_ERROR_FAILURE);
    messenger->OpenURL(uri);
    m_currentlyDisplayedMsgKey = m_keys[aViewIndex];
    m_currentlyDisplayedMsgUri = uri;
    m_currentlyDisplayedViewIndex = aViewIndex;
    UpdateDisplayMessage(m_currentlyDisplayedViewIndex);
  }
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::LoadMessageByUrl(const char *aUrl)
{
  NS_ASSERTION(aUrl, "trying to load a null url");
  if (!mSuppressMsgDisplay)
  {
    nsresult rv;
    nsCOMPtr<nsIMessenger> messenger (do_QueryReferent(mMessengerWeak, &rv));
    NS_ENSURE_SUCCESS(rv, rv);
    messenger->LoadURL(NULL, nsDependentCString(aUrl));
    m_currentlyDisplayedMsgKey = nsMsgKey_None;
    m_currentlyDisplayedMsgUri.Truncate();
    m_currentlyDisplayedViewIndex = nsMsgViewIndex_None;
  }
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::SelectionChanged()
{
  // if the currentSelection changed then we have a message to display - not if we are in the middle of deleting rows
  if (m_deletingRows)
    return NS_OK;

  PRUint32 numSelected = 0;

#ifdef DEBUG
  GetNumSelected(&numSelected);
#endif
  nsMsgViewIndexArray selection;
  GetSelectedIndices(selection);
  nsMsgViewIndex *indices = selection.Elements();
  NS_ASSERTION(numSelected == selection.Length(), "selected indices is not equal to num of msg selected!!!");
  numSelected = selection.Length();

  PRBool commandsNeedDisablingBecauseOfSelection = PR_FALSE;

  if(indices)
  {
    if (WeAreOffline())
      commandsNeedDisablingBecauseOfSelection = !OfflineMsgSelected(indices, numSelected);
    if (!NonDummyMsgSelected(indices, numSelected))
      commandsNeedDisablingBecauseOfSelection = PR_TRUE;
  }
  // if only one item is selected then we want to display a message
  if (numSelected == 1)
  {
    PRInt32 startRange;
    PRInt32 endRange;
    nsresult rv = mTreeSelection->GetRangeAt(0, &startRange, &endRange);
    NS_ENSURE_SUCCESS(rv, NS_OK); // tree doesn't care if we failed

    if (startRange >= 0 && startRange == endRange && startRange < GetSize())
    {
      if (!mRemovingRow)
      {
        if (!mSuppressMsgDisplay)
          LoadMessageByViewIndex(startRange);
        else
          UpdateDisplayMessage(startRange);
      }
    }
    else
      numSelected = 0; // selection seems bogus, so set to 0.
  }
  else {
    // if we have zero or multiple items selected, we shouldn't be displaying any message
    m_currentlyDisplayedMsgKey = nsMsgKey_None;
    m_currentlyDisplayedMsgUri.Truncate();
    m_currentlyDisplayedViewIndex = nsMsgViewIndex_None;

    // if we used to have one item selected, and now we have more than one, we should clear the message pane.
    nsCOMPtr<nsIMsgWindow> msgWindow(do_QueryReferent(mMsgWindowWeak));
    nsCOMPtr <nsIMsgWindowCommands> windowCommands;
    if ((mNumSelectedRows == 1) && (numSelected > 1) && msgWindow
        && NS_SUCCEEDED(msgWindow->GetWindowCommands(getter_AddRefs(windowCommands)))
        && windowCommands) {
      windowCommands->ClearMsgPane();
    }
  }

  // determine if we need to push command update notifications out to the UI or not.

  // we need to push a command update notification iff, one of the following conditions are met
  // (1) the selection went from 0 to 1
  // (2) it went from 1 to 0
  // (3) it went from 1 to many
  // (4) it went from many to 1 or 0
  // (5) a different msg was selected - perhaps it was offline or not...matters only when we are offline
  // (6) we did a forward/back, or went from having no history to having history - not sure how to tell this.

  // I think we're going to need to keep track of whether forward/back were enabled/should be enabled,
  // and when this changes, force a command update.

  PRBool enableGoForward = PR_FALSE;
  PRBool enableGoBack = PR_FALSE;

  NavigateStatus(nsMsgNavigationType::forward, &enableGoForward);
  NavigateStatus(nsMsgNavigationType::back, &enableGoBack);
  if ((numSelected == mNumSelectedRows ||
      (numSelected > 1 && mNumSelectedRows > 1)) && (commandsNeedDisablingBecauseOfSelection == mCommandsNeedDisablingBecauseOfSelection)
      && enableGoForward == mGoForwardEnabled && enableGoBack == mGoBackEnabled)
  {
  }
  // don't update commands if we're suppressing them, or if we're removing rows, unless it was the last row.
  else if (!mSuppressCommandUpdating && mCommandUpdater && (!mRemovingRow || GetSize() == 0)) // o.t. push an updated
  {
    mCommandUpdater->UpdateCommandStatus();
  }

  mCommandsNeedDisablingBecauseOfSelection = commandsNeedDisablingBecauseOfSelection;
  mGoForwardEnabled = enableGoForward;
  mGoBackEnabled = enableGoBack;
  mNumSelectedRows = numSelected;
  return NS_OK;
}

nsresult nsMsgDBView::GetSelectedIndices(nsMsgViewIndexArray& selection)
{
  if (mTreeSelection)
  {
    PRInt32 viewSize = GetSize();
    PRInt32 count;
    mTreeSelection->GetCount(&count);
    selection.SetLength(count);
    count = 0;
    PRInt32 selectionCount;
    mTreeSelection->GetRangeCount(&selectionCount);
    for (PRInt32 i = 0; i < selectionCount; i++)
    {
      PRInt32 startRange = -1;
      PRInt32 endRange = -1;
      mTreeSelection->GetRangeAt(i, &startRange, &endRange);
      if (startRange >= 0 && startRange < viewSize)
      {
        for (PRInt32 rangeIndex = startRange; rangeIndex <= endRange && rangeIndex < viewSize; rangeIndex++)
          selection[count++] = rangeIndex;
      }
    }
    NS_ASSERTION(selection.Length() == count, "selection count is wrong");
    selection.SetLength(count);
  }
  else
  {
    // if there is no tree selection object then we must be in stand alone message mode.
    // in that case the selected indices are really just the current message key.
    nsMsgViewIndex viewIndex = FindViewIndex(m_currentlyDisplayedMsgKey);
    if (viewIndex != nsMsgViewIndex_None)
      selection.AppendElement(viewIndex);
  }
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::GetRowProperties(PRInt32 index, nsISupportsArray *properties)
{
  if (!IsValidIndex(index))
    return NS_MSG_INVALID_DBVIEW_INDEX;

  // this is where we tell the tree to apply styles to a particular row
  nsCOMPtr <nsIMsgDBHdr> msgHdr;
  nsresult rv = NS_OK;

  rv = GetMsgHdrForViewIndex(index, getter_AddRefs(msgHdr));

  if (NS_FAILED(rv) || !msgHdr) {
    ClearHdrCache();
    return NS_MSG_INVALID_DBVIEW_INDEX;
  }

  nsCString keywordProperty;
  FetchKeywords(msgHdr, keywordProperty);
  if (!keywordProperty.IsEmpty())
    AppendKeywordProperties(keywordProperty.get(), properties, PR_FALSE);

  // give the custom column handlers a chance to style the row.
  for (int i = 0; i < m_customColumnHandlers.Count(); i++)
    m_customColumnHandlers[i]->GetRowProperties(index, properties);

  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::GetColumnProperties(nsITreeColumn* col, nsISupportsArray *properties)
{
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::GetCellProperties(PRInt32 aRow, nsITreeColumn *col, nsISupportsArray *properties)
{
  if (!IsValidIndex(aRow))
    return NS_MSG_INVALID_DBVIEW_INDEX;

  // this is where we tell the tree to apply styles to a particular row
  // i.e. if the row is an unread message...

  nsCOMPtr <nsIMsgDBHdr> msgHdr;
  nsresult rv = NS_OK;

  rv = GetMsgHdrForViewIndex(aRow, getter_AddRefs(msgHdr));

  if (NS_FAILED(rv) || !msgHdr)
  {
    ClearHdrCache();
    return NS_MSG_INVALID_DBVIEW_INDEX;
  }

  PRUint32 flags;
  msgHdr->GetFlags(&flags);

  if (!(flags & MSG_FLAG_READ))
    properties->AppendElement(kUnreadMsgAtom);
  else
    properties->AppendElement(kReadMsgAtom);

  if (flags & MSG_FLAG_REPLIED)
    properties->AppendElement(kRepliedMsgAtom);

  if (flags & MSG_FLAG_FORWARDED)
    properties->AppendElement(kForwardedMsgAtom);

  if (flags & MSG_FLAG_NEW)
    properties->AppendElement(kNewMsgAtom);

  if (flags & MSG_FLAG_IGNORED)
    properties->AppendElement(kIgnoreSubthreadAtom);

  nsCOMPtr <nsIMsgLocalMailFolder> localFolder = do_QueryInterface(m_folder);

  if ((flags & MSG_FLAG_OFFLINE) || (localFolder && !(flags & MSG_FLAG_PARTIAL)))
    properties->AppendElement(kOfflineMsgAtom);

  if (flags & MSG_FLAG_ATTACHMENT)
    properties->AppendElement(kAttachMsgAtom);

  if ((mDeleteModel == nsMsgImapDeleteModels::IMAPDelete) && (flags & MSG_FLAG_IMAP_DELETED))
    properties->AppendElement(kImapDeletedMsgAtom);

  if (mMessageTypeAtom)
    properties->AppendElement(mMessageTypeAtom);

  nsCString imageSize;
  msgHdr->GetStringProperty("imageSize", getter_Copies(imageSize));
  if (!imageSize.IsEmpty())
  {
    properties->AppendElement(kHasImageAtom);
  }

  nsCString junkScoreStr;
  msgHdr->GetStringProperty("junkscore", getter_Copies(junkScoreStr));
  if (!junkScoreStr.IsEmpty()) {
    properties->AppendElement(junkScoreStr.ToInteger((PRInt32*)&rv) == nsIJunkMailPlugin::IS_SPAM_SCORE ?
                              kJunkMsgAtom : kNotJunkMsgAtom);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Converting junkScore to integer failed.");
  }

  nsCString keywords;
  FetchKeywords(msgHdr, keywords);
  if (!keywords.IsEmpty())
    AppendKeywordProperties(keywords.get(), properties, PR_TRUE);

  // this is a double fetch of the keywords property since we also fetch
  // it for the tags - do we want to do this?
  // I'm not sure anyone uses the kw- property, though it could be nice
  // for people wanting to extend the thread pane.
  nsCString keywordProperty;
  msgHdr->GetStringProperty("keywords", getter_Copies(keywordProperty));
  if (!keywordProperty.IsEmpty())
  {
    nsCAutoString keywords(keywordProperty);
    nsCAutoString nextKeyword;
    PRInt32 spaceIndex = 0;
    do
    {
      spaceIndex = keywords.FindChar(' ');
      PRInt32 endOfKeyword = (spaceIndex == -1) ? keywords.Length() : spaceIndex;
      keywords.Left(nextKeyword, endOfKeyword);
      nextKeyword.Insert("kw-", 0);
      nsCOMPtr <nsIAtom> keywordAtom = do_GetAtom(nextKeyword.get());
      properties->AppendElement(keywordAtom);
      if (spaceIndex > 0)
        keywords.Cut(0, endOfKeyword + 1);
    }
    while (spaceIndex > 0);
  }

#ifdef SUPPORT_PRIORITY_COLORS
  // add special styles for priority
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
#endif

  const PRUnichar* colID;
  col->GetIdConst(&colID);
  if (colID[0] == 'f')
  {
    if (m_flags[aRow] & MSG_FLAG_MARKED)
    {
      properties->AppendElement(kFlaggedMsgAtom);
    }
  }

  if (m_viewFlags & nsMsgViewFlagsType::kThreadedDisplay)
  {
    if (m_flags[aRow] & MSG_VIEW_FLAG_ISTHREAD)
    {
      nsCOMPtr <nsIMsgThread> thread;
      rv = GetThreadContainingIndex(aRow, getter_AddRefs(thread));
      if (NS_SUCCEEDED(rv) && thread)
      {
        PRUint32 numUnreadChildren;
        thread->GetNumUnreadChildren(&numUnreadChildren);
        if (numUnreadChildren > 0)
          properties->AppendElement(kHasUnreadAtom);
        thread->GetFlags(&flags);
        if (flags & MSG_FLAG_WATCHED)
          properties->AppendElement(kWatchThreadAtom);
        if (flags & MSG_FLAG_IGNORED)
          properties->AppendElement(kIgnoreThreadAtom);
      }
    }
  }

  //custom column handlers are called at the end of getCellProperties
  //to make life easier for extension writers
  nsIMsgCustomColumnHandler* colHandler = GetColumnHandler(colID);

  if (colHandler != nsnull)
  {
    colHandler->GetCellProperties(aRow, col, properties);
    return NS_OK;
  }

  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::IsContainer(PRInt32 index, PRBool *_retval)
{
  if (!IsValidIndex(index))
      return NS_MSG_INVALID_DBVIEW_INDEX;

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
  if (!IsValidIndex(index))
      return NS_MSG_INVALID_DBVIEW_INDEX;

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
  if (!IsValidIndex(index))
    return NS_MSG_INVALID_DBVIEW_INDEX;

  if (m_viewFlags & nsMsgViewFlagsType::kThreadedDisplay)
  {
    PRUint32 flags = m_flags[index];
    *_retval = !(flags & MSG_VIEW_FLAG_HASCHILDREN);
  }
  else
    *_retval = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::IsSeparator(PRInt32 index, PRBool *_retval)
{
  if (!IsValidIndex(index))
    return NS_MSG_INVALID_DBVIEW_INDEX;

  *_retval = PR_FALSE;

  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::GetParentIndex(PRInt32 rowIndex, PRInt32 *_retval)
{
  *_retval = -1;

  PRInt32 rowIndexLevel;
  GetLevel(rowIndex, &rowIndexLevel);

  PRInt32 i;
  for(i = rowIndex; i >= 0; i--)
  {
    PRInt32 l;
    GetLevel(i, &l);
    if (l < rowIndexLevel)
    {
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
  for(i = afterIndex + 1; i < count; i++)
  {
    PRInt32 l;
    GetLevel(i, &l);
    if (l < rowIndexLevel)
      break;
    if (l == rowIndexLevel)
    {
      *_retval = PR_TRUE;
      break;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::GetLevel(PRInt32 index, PRInt32 *_retval)
{
  if (!IsValidIndex(index))
    return NS_MSG_INVALID_DBVIEW_INDEX;

  if (m_viewFlags & nsMsgViewFlagsType::kThreadedDisplay)
    *_retval = m_levels[index];
  else
    *_retval = 0;
  return NS_OK;
}

// search view will override this since headers can span db's
nsresult nsMsgDBView::GetMsgHdrForViewIndex(nsMsgViewIndex index, nsIMsgDBHdr **msgHdr)
{
  nsresult rv = NS_OK;
  nsMsgKey key = m_keys[index];
  if (key == nsMsgKey_None || !m_db)
    return NS_MSG_INVALID_DBVIEW_INDEX;

  if (key == m_cachedMsgKey)
  {
    *msgHdr = m_cachedHdr;
    NS_IF_ADDREF(*msgHdr);
  }
  else
  {
    rv = m_db->GetMsgHdrForKey(key, msgHdr);
    if (NS_SUCCEEDED(rv))
    {
      m_cachedHdr = *msgHdr;
      m_cachedMsgKey = key;
    }
  }

  return rv;
}

nsresult nsMsgDBView::GetFolderForViewIndex(nsMsgViewIndex index, nsIMsgFolder **aFolder)
{
  NS_IF_ADDREF(*aFolder = m_folder);
  return NS_OK;
}

nsresult nsMsgDBView::GetDBForViewIndex(nsMsgViewIndex index, nsIMsgDatabase **db)
{
  NS_IF_ADDREF(*db = m_db);
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::GetImageSrc(PRInt32 aRow, nsITreeColumn* aCol, nsAString& aValue)
{
  NS_ENSURE_ARG_POINTER(aCol);
  //attempt to retreive a custom column handler. If it exists call it and return
  const PRUnichar* colID;
  aCol->GetIdConst(&colID);

  nsIMsgCustomColumnHandler* colHandler = GetColumnHandler(colID);

  if (colHandler)
  {
    colHandler->GetImageSrc(aRow, aCol, aValue);
    return NS_OK;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsMsgDBView::GetProgressMode(PRInt32 aRow, nsITreeColumn* aCol, PRInt32* _retval)
{
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::GetCellValue(PRInt32 aRow, nsITreeColumn* aCol, nsAString& aValue)
{
  if (!IsValidIndex(aRow))
    return NS_MSG_INVALID_DBVIEW_INDEX;

  nsCOMPtr <nsIMsgDBHdr> msgHdr;
  nsresult rv = GetMsgHdrForViewIndex(aRow, getter_AddRefs(msgHdr));

  if (NS_FAILED(rv) || !msgHdr)
  {
    ClearHdrCache();
    return NS_MSG_INVALID_DBVIEW_INDEX;
  }

  const PRUnichar* colID;
  aCol->GetIdConst(&colID);

  PRUint32 flags;
  msgHdr->GetFlags(&flags);

  // provide a string "value" for cells that do not normally have text.
  // use empty string for the normal states "Read", "Not Starred", "No Attachment" and "Not Junk"
  switch (colID[0])
  {
    case 'a': // attachment column
      aValue.Assign(GetString ((flags & MSG_FLAG_ATTACHMENT) ?
      NS_LITERAL_STRING("messageHasAttachment").get()
      : EmptyString().get()));
      break;
    case 'f': // flagged (starred) column
      aValue.Assign(GetString ((flags & MSG_FLAG_MARKED) ?
      NS_LITERAL_STRING("messageHasFlag").get()
      : EmptyString().get()));
      break;
    case 'j': // junk column
      if (!mIsNews)
      {
        nsCString junkScoreStr;
        msgHdr->GetStringProperty("junkscore", getter_Copies(junkScoreStr));
        // Only need to assing a real value for junk, it's empty already
        // as it should be for non-junk.
        if (!junkScoreStr.IsEmpty() &&
            (junkScoreStr.ToInteger((PRInt32*)&rv) == nsIJunkMailPlugin::IS_SPAM_SCORE))
          aValue.AssignLiteral("messageJunk");

        NS_ASSERTION(NS_SUCCEEDED(rv), "Converting junkScore to integer failed.");
      }
      break;
    case 't':
      if (colID[1] == 'h' && (m_viewFlags & nsMsgViewFlagsType::kThreadedDisplay))
      {       // thread column
        PRBool isContainer, isContainerEmpty, isContainerOpen;
        IsContainer(aRow, &isContainer);
        if (isContainer)
        {
          IsContainerEmpty(aRow, &isContainerEmpty);
          if (!isContainerEmpty)
          {
            IsContainerOpen(aRow, &isContainerOpen);
            aValue.Assign(GetString (isContainerOpen ?
             NS_LITERAL_STRING("messageExpanded").get()
             : NS_LITERAL_STRING("messageCollapsed").get()));
          }
        }
      }
      break;
    case 'u': // read/unread column
      aValue.Assign(GetString ((flags & MSG_FLAG_READ) ?
      EmptyString().get() : NS_LITERAL_STRING("messageUnread").get()));
      break;
    default:
      aValue.Assign(colID);
      break;
  }

  return rv;
}

void nsMsgDBView::RememberDeletedMsgHdr(nsIMsgDBHdr *msgHdr)
{
  nsCString messageId;
  msgHdr->GetMessageId(getter_Copies(messageId));
  if (mRecentlyDeletedArrayIndex >= mRecentlyDeletedMsgIds.Count())
    mRecentlyDeletedMsgIds.AppendCString(messageId);
  else
    mRecentlyDeletedMsgIds.ReplaceCStringAt(messageId, mRecentlyDeletedArrayIndex);
  // only remember last 20 deleted msgs.
  mRecentlyDeletedArrayIndex = ++mRecentlyDeletedArrayIndex % 20;
}

PRBool nsMsgDBView::WasHdrRecentlyDeleted(nsIMsgDBHdr *msgHdr)
{
  nsCString messageId;
  msgHdr->GetMessageId(getter_Copies(messageId));
  for (PRInt32 i = 0; i < mRecentlyDeletedMsgIds.Count(); i++)
  {
    if (messageId.Equals(*(mRecentlyDeletedMsgIds[i])))
      return PR_TRUE;
  }
  return PR_FALSE;

}
//add a custom column handler
NS_IMETHODIMP nsMsgDBView::AddColumnHandler(const nsAString& column, nsIMsgCustomColumnHandler* handler)
{

  PRInt32 index = m_customColumnHandlerIDs.IndexOf(column);

  nsAutoString strColID(column);

  //does not exist
  if (index == -1)
  {
    m_customColumnHandlerIDs.AppendString(strColID);
    m_customColumnHandlers.AppendObject(handler);
  }
  else
  {
    //insert new handler into the appropriate place in the COMPtr array
    //no need to replace the column ID (it's the same)
    m_customColumnHandlers.ReplaceObjectAt(handler, index);

  }
  // Check if the column name matches any of the columns in
  // m_sortColumns, and if so, set m_sortColumns[i].mColHandler
  for (PRUint32 i = 0; i < m_sortColumns.Length(); i++)
  {
    MsgViewSortColumnInfo &sortInfo = m_sortColumns[i];
    if (sortInfo.mSortType == nsMsgViewSortType::byCustom && 
          sortInfo.mCustomColumnName.Equals(column))
      sortInfo.mColHandler = handler;
  }
  return NS_OK;
}

//remove a custom column handler
NS_IMETHODIMP nsMsgDBView::RemoveColumnHandler(const nsAString& aColID)
{

  // here we should check if the column name matches any of the columns in
  // m_sortColumns, and if so, clear m_sortColumns[i].mColHandler
  PRInt32 index = m_customColumnHandlerIDs.IndexOf(aColID);

  if (index != -1)
  {
    m_customColumnHandlerIDs.RemoveStringAt(index);
    m_customColumnHandlers.RemoveObjectAt(index);
    // Check if the column name matches any of the columns in
    // m_sortColumns, and if so, clear m_sortColumns[i].mColHandler
    for (PRUint32 i = 0; i < m_sortColumns.Length(); i++)
    {
      MsgViewSortColumnInfo &sortInfo = m_sortColumns[i];
      if (sortInfo.mSortType == nsMsgViewSortType::byCustom && 
            sortInfo.mCustomColumnName.Equals(aColID))
        sortInfo.mColHandler = nsnull;
    }

    return NS_OK;
  }

  return NS_ERROR_FAILURE; //can't remove a column that isn't currently custom handled
}

//TODO: NS_ENSURE_SUCCESS
nsIMsgCustomColumnHandler* nsMsgDBView::GetCurColumnHandlerFromDBInfo()
{

  nsAutoString colID;
  GetCurCustomColumn(colID);
  return GetColumnHandler(colID.get());
}

void nsMsgDBView::GetCurCustomColumn(nsString &colID)
{
  if (!m_db)
    return;
  
  nsCOMPtr<nsIDBFolderInfo>  dbInfo;
  m_db->GetDBFolderInfo(getter_AddRefs(dbInfo));
  
  if (!dbInfo)
    return;
  
 dbInfo->GetProperty("customSortCol", colID);
  
}
nsIMsgCustomColumnHandler* nsMsgDBView::GetColumnHandler(const PRUnichar *colID)
{
  nsIMsgCustomColumnHandler* columnHandler = nsnull;

  PRInt32 index = m_customColumnHandlerIDs.IndexOf(nsDependentString(colID));

  if (index > -1)
    columnHandler = m_customColumnHandlers[index];

  return columnHandler;
}


NS_IMETHODIMP nsMsgDBView::GetColumnHandler(const nsAString& aColID, nsIMsgCustomColumnHandler** aHandler)
{
  NS_ENSURE_ARG_POINTER(aHandler);
  nsAutoString column(aColID);
  NS_IF_ADDREF(*aHandler = GetColumnHandler(column.get()));
  return (*aHandler) ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsMsgDBView::GetCellText(PRInt32 aRow, nsITreeColumn* aCol, nsAString& aValue)
{
  nsresult rv = NS_OK;

  if (!IsValidIndex(aRow))
    return NS_MSG_INVALID_DBVIEW_INDEX;

  nsCOMPtr <nsIMsgDBHdr> msgHdr;
  rv = GetMsgHdrForViewIndex(aRow, getter_AddRefs(msgHdr));

  if (NS_FAILED(rv) || !msgHdr)
  {
    ClearHdrCache();
    return NS_MSG_INVALID_DBVIEW_INDEX;
  }

  aValue.SetCapacity(0);
  // XXX fix me by making Fetch* take an nsAString& parameter
  nsString valueText;
  nsCOMPtr <nsIMsgThread> thread;

  const PRUnichar* colID;
  aCol->GetIdConst(&colID);

  //attempt to retreive a custom column handler. If it exists call it and return
  nsIMsgCustomColumnHandler* colHandler = GetColumnHandler(colID);

  if (colHandler)
  {
    colHandler->GetCellText(aRow, aCol, aValue);
    return NS_OK;
  }

  switch (colID[0])
  {
  case 's':
    if (colID[1] == 'u') // subject
      rv = FetchSubject(msgHdr, m_flags[aRow], aValue);
    else if (colID[1] == 'e') // sender
      rv = FetchAuthor(msgHdr, aValue);
    else if (colID[1] == 'i') // size
      rv = FetchSize(msgHdr, aValue);
    else if (colID[1] == 't') // status
    {
      PRUint32 flags;
      msgHdr->GetFlags(&flags);
      rv = FetchStatus(flags, aValue);
    }
    break;
  case 'r':
    if (colID[3] == 'i') // recipient
      rv = FetchRecipients(msgHdr, aValue);
    else if (colID[3] == 'e') // received
      rv = FetchDate(msgHdr, aValue, PR_TRUE);
    break;
  case 'd':  // date
    rv = FetchDate(msgHdr, aValue);
    break;
  case 'p': // priority
    rv = FetchPriority(msgHdr, aValue);
    break;
  case 'a': // account
    if (colID[1] == 'c') // account
      rv = FetchAccount(msgHdr, aValue);
    break;
  case 't':
    // total msgs in thread column
    if (colID[1] == 'o' && (m_viewFlags & nsMsgViewFlagsType::kThreadedDisplay))
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
          aValue.Assign(formattedCountString);
        }
      }
    }
    else if (colID[1] == 'a') // tags
    {
      rv = FetchTags(msgHdr, aValue);
    }
    break;
  case 'u':
    // unread msgs in thread col
    if (colID[6] == 'C' && (m_viewFlags & nsMsgViewFlagsType::kThreadedDisplay))
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
            aValue.Assign(formattedCountString);
          }
        }
      }
    }
    break;
  case 'j':
    {
      nsCString junkScoreStr;
      msgHdr->GetStringProperty("junkscore", getter_Copies(junkScoreStr));
      CopyASCIItoUTF16(junkScoreStr, aValue);
    }
    break;
  case 'i': // id
    {
      nsAutoString keyString;
      nsMsgKey key;
      msgHdr->GetMessageKey(&key);
      keyString.AppendInt(key);
      aValue.Assign(keyString);
    }
  default:
    break;
  }

  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::SetTree(nsITreeBoxObject *tree)
{
  mTree = tree;
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::ToggleOpenState(PRInt32 index)
{
  PRUint32 numChanged;
  nsresult rv = ToggleExpansion(index, &numChanged);
  NS_ENSURE_SUCCESS(rv,rv);
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::CycleHeader(nsITreeColumn* aCol)
{
    // let HandleColumnClick() in threadPane.js handle it
    // since it will set / clear the sort indicators.
    return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::CycleCell(PRInt32 row, nsITreeColumn* col)
{
  if (!IsValidIndex(row))
    return NS_MSG_INVALID_DBVIEW_INDEX;

  const PRUnichar* colID;
  col->GetIdConst(&colID);

  //attempt to retreive a custom column handler. If it exists call it and return
  nsIMsgCustomColumnHandler* colHandler = GetColumnHandler(colID);

  if (colHandler)
  {
    colHandler->CycleCell(row, col);
    return NS_OK;
  }

  switch (colID[0])
  {
  case 'u': // unreadButtonColHeader
    if (colID[6] == 'B')
      ApplyCommandToIndices(nsMsgViewCommandType::toggleMessageRead, (nsMsgViewIndex *) &row, 1);
   break;
  case 't': // tag cell, threaded cell or total cell
    if (colID[1] == 'h')
    {
      ExpandAndSelectThreadByIndex(row, PR_FALSE);
    }
    else if (colID[1] == 'a')
    {
      // ### Do we want to keep this behaviour but switch it to tags?
      // We could enumerate over the tags and go to the next one - it looks
      // to me like this wasn't working before tags landed, so maybe not
      // worth bothering with.
    }
    break;
  case 'f': // flagged column
    // toggle the flagged status of the element at row.
    if (m_flags[row] & MSG_FLAG_MARKED)
      ApplyCommandToIndices(nsMsgViewCommandType::unflagMessages, (nsMsgViewIndex *) &row, 1);
    else
      ApplyCommandToIndices(nsMsgViewCommandType::flagMessages, (nsMsgViewIndex *) &row, 1);
    break;
  case 'j': // junkStatus column
    {
      if (mIsNews) // junk not supported for news yet.
        return NS_OK;

      nsCOMPtr <nsIMsgDBHdr> msgHdr;

      nsresult rv = GetMsgHdrForViewIndex(row, getter_AddRefs(msgHdr));
      if (NS_SUCCEEDED(rv) && msgHdr)
      {
        nsCString junkScoreStr;
        rv = msgHdr->GetStringProperty("junkscore", getter_Copies(junkScoreStr));
        if (junkScoreStr.IsEmpty() || (junkScoreStr.ToInteger((PRInt32*)&rv) == nsIJunkMailPlugin::IS_HAM_SCORE))
          ApplyCommandToIndices(nsMsgViewCommandType::junk, (nsMsgViewIndex *) &row, 1);
        else
          ApplyCommandToIndices(nsMsgViewCommandType::unjunk, (nsMsgViewIndex *) &row, 1);

        NS_ASSERTION(NS_SUCCEEDED(rv), "Converting junkScore to integer failed.");
      }
    }
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

NS_IMETHODIMP nsMsgDBView::PerformActionOnCell(const PRUnichar *action, PRInt32 row, nsITreeColumn* col)
{
  return NS_OK;
}

///////////////////////////////////////////////////////////////////////////
// end nsITreeView Implementation Methods
///////////////////////////////////////////////////////////////////////////

NS_IMETHODIMP nsMsgDBView::Open(nsIMsgFolder *folder, nsMsgViewSortTypeValue sortType, nsMsgViewSortOrderValue sortOrder, nsMsgViewFlagsTypeValue viewFlags, PRInt32 *pCount)
{
  m_viewFlags = viewFlags;
  m_sortOrder = sortOrder;
  m_sortType = sortType;

  nsresult rv;
  nsCOMPtr<nsIMsgAccountManager> accountManager =
           do_GetService(NS_MSGACCOUNTMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  PRBool userNeedsToAuthenticate = PR_FALSE;
  // if we're PasswordProtectLocalCache, then we need to find out if the server is authenticated.
  (void) accountManager->GetUserNeedsToAuthenticate(&userNeedsToAuthenticate);
  if (userNeedsToAuthenticate)
    return NS_MSG_USER_NOT_AUTHENTICATED;

  if (folder) // search view will have a null folder
  {
    nsCOMPtr <nsIDBFolderInfo> folderInfo;
    rv = folder->GetDBFolderInfoAndDB(getter_AddRefs(folderInfo), getter_AddRefs(m_db));
    NS_ENSURE_SUCCESS(rv,rv);
    m_db->AddListener(this);
    m_folder = folder;
    m_viewFolder = folder;

    SetMRUTimeForFolder(m_folder);

    // restore m_sortColumns from db
    nsString sortColumnsString;
    folderInfo->GetProperty("sortColumns", sortColumnsString);
    DecodeColumnSort(sortColumnsString);
    // determine if we are in a news folder or not.
    // if yes, we'll show lines instead of size, and special icons in the thread pane
    nsCOMPtr <nsIMsgIncomingServer> server;
    rv = folder->GetServer(getter_AddRefs(server));
    NS_ENSURE_SUCCESS(rv,rv);
    nsCString type;
    rv = server->GetType(type);
    NS_ENSURE_SUCCESS(rv,rv);

    mIsNews = type.LowerCaseEqualsLiteral("nntp");

    if (type.IsEmpty())
      mMessageTypeAtom = nsnull;
    else  // special case nntp --> news since we'll break themes if we try to be consistent
      mMessageTypeAtom = do_GetAtom(mIsNews ? "news" : type.get());

    GetImapDeleteModel(nsnull);

    if (mIsNews)
    {
      nsCOMPtr<nsIPrefBranch> prefs(do_GetService(NS_PREFSERVICE_CONTRACTID));
      if (prefs)
      {
        PRBool temp;
        rv = prefs->GetBoolPref("news.show_size_in_lines", &temp);
        if (NS_SUCCEEDED(rv))
          mShowSizeInLines = temp;
      }
    }
  }
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::Close()
{
  PRInt32 oldSize = GetSize();
  // this is important, because the tree will ask us for our
  // row count, which get determine from the number of keys.
  m_keys.Clear();
  // be consistent
  m_flags.Clear();
  m_levels.Clear();

  // clear these out since they no longer apply if we're switching a folder
  nsMemory::Free(mJunkIndices);
  mJunkIndices = nsnull;
  mNumJunkIndices = 0;

  // this needs to happen after we remove all the keys, since RowCountChanged() will call our GetRowCount()
  if (mTree)
    mTree->RowCountChanged(0, -oldSize);

  ClearHdrCache();
  if (m_db)
  {
    m_db->RemoveListener(this);
    m_db = nsnull;
  }
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::OpenWithHdrs(nsISimpleEnumerator *aHeaders, nsMsgViewSortTypeValue aSortType,
                                        nsMsgViewSortOrderValue aSortOrder, nsMsgViewFlagsTypeValue aViewFlags,
                                        PRInt32 *aCount)
{
  NS_ASSERTION(PR_FALSE, "not implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsMsgDBView::Init(nsIMessenger * aMessengerInstance, nsIMsgWindow * aMsgWindow, nsIMsgDBViewCommandUpdater *aCmdUpdater)
{
  mMessengerWeak = do_GetWeakReference(aMessengerInstance);
  mMsgWindowWeak = do_GetWeakReference(aMsgWindow);
  mCommandUpdater = aCmdUpdater;
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::SetSuppressCommandUpdating(PRBool aSuppressCommandUpdating)
{
  mSuppressCommandUpdating = aSuppressCommandUpdating;
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::GetSuppressCommandUpdating(PRBool * aSuppressCommandUpdating)
{
  *aSuppressCommandUpdating = mSuppressCommandUpdating;
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::SetSuppressMsgDisplay(PRBool aSuppressDisplay)
{
  PRUint32 numSelected = 0;
  GetNumSelected(&numSelected);

  PRBool forceDisplay = PR_FALSE;
  if (mSuppressMsgDisplay && !aSuppressDisplay && numSelected == 1)
    forceDisplay = PR_TRUE;

  mSuppressMsgDisplay = aSuppressDisplay;
  if (forceDisplay)
  {
    // get the view indexfor the currently selected message
    nsMsgViewIndex viewIndex;
    nsresult rv = GetViewIndexForFirstSelectedMsg(&viewIndex);
    if (NS_SUCCEEDED(rv) && viewIndex != nsMsgViewIndex_None)
       LoadMessageByViewIndex(viewIndex);
  }

  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::GetSuppressMsgDisplay(PRBool * aSuppressDisplay)
{
  *aSuppressDisplay = mSuppressMsgDisplay;
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::GetUsingLines(PRBool * aUsingLines)
{
  *aUsingLines = mShowSizeInLines;
  return NS_OK;
}

int PR_CALLBACK CompareViewIndices (const void *v1, const void *v2, void *)
{
  nsMsgViewIndex i1 = *(nsMsgViewIndex*) v1;
  nsMsgViewIndex i2 = *(nsMsgViewIndex*) v2;
  return i1 - i2;
}

NS_IMETHODIMP nsMsgDBView::GetIndicesForSelection(PRUint32 *length, nsMsgViewIndex **indices)
{
  NS_ENSURE_ARG_POINTER(length);
  *length = 0;
  NS_ENSURE_ARG_POINTER(indices);
  *indices = nsnull;

  nsMsgViewIndexArray selection;
  GetSelectedIndices(selection);
  PRUint32 numIndices = selection.Length();
  if (!numIndices) return NS_OK;
  *length = numIndices;

  PRUint32 datalen = numIndices * sizeof(nsMsgViewIndex);
  *indices = (nsMsgViewIndex *)NS_Alloc(datalen);
  if (!*indices) return NS_ERROR_OUT_OF_MEMORY;
  memcpy(*indices, selection.Elements(), datalen);
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::GetURIsForSelection(PRUint32 *length, char ***uris)
{
  nsresult rv = NS_OK;

  NS_ENSURE_ARG_POINTER(length);
  *length = 0;
  NS_ENSURE_ARG_POINTER(uris);
  *uris = nsnull;

  nsMsgViewIndexArray selection;
  GetSelectedIndices(selection);
  PRUint32 numIndices = selection.Length();
  if (!numIndices) return NS_OK;
  *length = numIndices;

  nsCOMPtr <nsIMsgFolder> folder = m_folder;
  char **outArray, **next;
  next = outArray = (char **)nsMemory::Alloc(numIndices * sizeof(char *));
  if (!outArray) return NS_ERROR_OUT_OF_MEMORY;
  for (PRUint32 i = 0; i < numIndices; i++)
  {
    nsMsgViewIndex selectedIndex = selection[i];
    nsCString tmpUri;
    if (!m_folder) // must be a cross folder view, like search results
      GetFolderForViewIndex(selectedIndex, getter_AddRefs(folder));
    rv = GenerateURIForMsgKey(m_keys[selectedIndex], folder, tmpUri);
    NS_ENSURE_SUCCESS(rv,rv);
    *next = ToNewCString(tmpUri);
    if (!*next) return NS_ERROR_OUT_OF_MEMORY;
    
    next++;
  }

  *uris = outArray;
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::GetURIForViewIndex(nsMsgViewIndex index, nsACString &result)
{
  nsresult rv;
  nsCOMPtr <nsIMsgFolder> folder = m_folder;
  if (!folder)
  {
    rv = GetFolderForViewIndex(index, getter_AddRefs(folder));
    NS_ENSURE_SUCCESS(rv,rv);
  }
  if (index == nsMsgViewIndex_None || m_flags[index] & MSG_VIEW_FLAG_DUMMY)
    return NS_MSG_INVALID_DBVIEW_INDEX;
  return GenerateURIForMsgKey(m_keys[index], folder, result);
}

NS_IMETHODIMP nsMsgDBView::DoCommandWithFolder(nsMsgViewCommandTypeValue command, nsIMsgFolder *destFolder)
{
  NS_ENSURE_ARG_POINTER(destFolder);

  nsMsgViewIndexArray selection;

  GetSelectedIndices(selection);

  nsMsgViewIndex *indices = selection.Elements();
  PRInt32 numIndices = selection.Length();

  nsresult rv = NS_OK;
  switch (command) {
    case nsMsgViewCommandType::copyMessages:
    case nsMsgViewCommandType::moveMessages:
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
  nsMsgViewIndexArray selection;

  GetSelectedIndices(selection);

  nsMsgViewIndex *indices = selection.Elements();
  PRInt32 numIndices = selection.Length();
  nsCOMPtr<nsIMsgWindow> msgWindow(do_QueryReferent(mMsgWindowWeak));

  nsresult rv = NS_OK;
  switch (command)
  {
  case nsMsgViewCommandType::downloadSelectedForOffline:
    return DownloadForOffline(msgWindow, indices, numIndices);
  case nsMsgViewCommandType::downloadFlaggedForOffline:
    return DownloadFlaggedForOffline(msgWindow);
  case nsMsgViewCommandType::markMessagesRead:
  case nsMsgViewCommandType::markMessagesUnread:
  case nsMsgViewCommandType::toggleMessageRead:
  case nsMsgViewCommandType::flagMessages:
  case nsMsgViewCommandType::unflagMessages:
  case nsMsgViewCommandType::deleteMsg:
  case nsMsgViewCommandType::undeleteMsg:
  case nsMsgViewCommandType::deleteNoTrash:
  case nsMsgViewCommandType::markThreadRead:
  case nsMsgViewCommandType::junk:
  case nsMsgViewCommandType::unjunk:
    NoteStartChange(nsMsgViewNotificationCode::none, 0, 0);
    rv = ApplyCommandToIndices(command, indices, numIndices);
    NoteEndChange(nsMsgViewNotificationCode::none, 0, 0);
    break;
  case nsMsgViewCommandType::selectAll:
    if (mTreeSelection && mTree)
    {
        // if in threaded mode, we need to expand all before selecting
        if (m_viewFlags & nsMsgViewFlagsType::kThreadedDisplay)
            rv = ExpandAll();
        mTreeSelection->SelectAll();
        mTree->Invalidate();
    }
    break;
  case nsMsgViewCommandType::selectThread:
    rv = ExpandAndSelectThread();
    break;
  case nsMsgViewCommandType::selectFlagged:
    if (!mTreeSelection)
      rv = NS_ERROR_UNEXPECTED;
    else
    {
      mTreeSelection->SetSelectEventsSuppressed(PR_TRUE);
      mTreeSelection->ClearSelection();
      // XXX ExpandAll?
      nsMsgViewIndex numIndices = GetSize();
      for (nsMsgViewIndex curIndex = 0; curIndex < numIndices; curIndex++)
      {
        if (m_flags[curIndex] & MSG_FLAG_MARKED)
          mTreeSelection->ToggleSelect(curIndex);
      }
      mTreeSelection->SetSelectEventsSuppressed(PR_FALSE);
    }
    break;
  case nsMsgViewCommandType::markAllRead:
    if (m_folder)
      rv = m_folder->MarkAllMessagesRead();
    break;
  case nsMsgViewCommandType::toggleThreadWatched:
    rv = ToggleWatched(indices,  numIndices);
    break;
  case nsMsgViewCommandType::expandAll:
    rv = ExpandAll();
    m_viewFlags |= nsMsgViewFlagsType::kExpandAll;
    SetViewFlags(m_viewFlags);
    NS_ASSERTION(mTree, "no tree, see bug #114956");
    if(mTree)
      mTree->Invalidate();
    break;
  case nsMsgViewCommandType::collapseAll:
    rv = CollapseAll();
    m_viewFlags &= ~nsMsgViewFlagsType::kExpandAll;
    SetViewFlags(m_viewFlags);
    NS_ASSERTION(mTree, "no tree, see bug #114956");
    if(mTree)
      mTree->Invalidate();
    break;
  default:
    NS_ASSERTION(PR_FALSE, "invalid command type");
    rv = NS_ERROR_UNEXPECTED;
    break;
  }
  return rv;
}

PRBool nsMsgDBView::ServerSupportsFilterAfterTheFact()
{
  if (!m_folder)  // cross folder virtual folders might not have a folder set.
    return PR_FALSE;

  // can't manually run news filters yet
  if (mIsNews)
    return PR_FALSE;

  nsCOMPtr <nsIMsgIncomingServer> server;
  nsresult rv = m_folder->GetServer(getter_AddRefs(server));
  if (NS_FAILED(rv))
    return PR_FALSE; // unexpected

  // filter after the fact is implement using search
  // so if you can't search, you can't filter after the fact
  PRBool canSearch;
  rv = server->GetCanSearchMessages(&canSearch);
  if (NS_FAILED(rv))
    return PR_FALSE; // unexpected

  return canSearch;
}

NS_IMETHODIMP nsMsgDBView::GetCommandStatus(nsMsgViewCommandTypeValue command, PRBool *selectable_p, nsMsgViewCommandCheckStateValue *selected_p)
{
  nsresult rv = NS_OK;

  PRBool haveSelection;
  PRInt32 rangeCount;
  nsMsgViewIndexArray selection;
  GetSelectedIndices(selection);
  PRInt32 numIndices = selection.Length();
  nsMsgViewIndex *indices = selection.Elements();
  // if range count is non-zero, we have at least one item selected, so we have a selection
  if (mTreeSelection && NS_SUCCEEDED(mTreeSelection->GetRangeCount(&rangeCount)) && rangeCount > 0)
    haveSelection = NonDummyMsgSelected(indices, numIndices);
  else
    haveSelection = PR_FALSE;

  switch (command)
  {
  case nsMsgViewCommandType::deleteMsg:
  case nsMsgViewCommandType::deleteNoTrash:
    {
      PRBool canDelete;
      // news folders can't delete (or move messages)
      // but we use delete for cancel messages.
      if (m_folder && !mIsNews && NS_SUCCEEDED(m_folder->GetCanDeleteMessages(&canDelete)) && !canDelete)
        *selectable_p = PR_FALSE;
      else
        *selectable_p = haveSelection;
    }
    break;
  case nsMsgViewCommandType::applyFilters:
    // disable if no messages
    // XXX todo, check that we have filters, and at least one is enabled
    *selectable_p = GetSize();
    if (*selectable_p)
      *selectable_p = ServerSupportsFilterAfterTheFact();
    break;
  case nsMsgViewCommandType::runJunkControls:
    // disable if no messages
    // no JMC on news yet
    // XXX todo, check that we have JMC enabled?
    *selectable_p = GetSize() && !mIsNews;
    break;
  case nsMsgViewCommandType::deleteJunk:
    {
      // disable if no messages, or if we can't delete (like news and certain imap folders)
      PRBool canDelete;
      *selectable_p = GetSize() && (m_folder && NS_SUCCEEDED(m_folder->GetCanDeleteMessages(&canDelete)) && canDelete);
    }
    break;
  case nsMsgViewCommandType::markMessagesRead:
  case nsMsgViewCommandType::markMessagesUnread:
  case nsMsgViewCommandType::toggleMessageRead:
  case nsMsgViewCommandType::flagMessages:
  case nsMsgViewCommandType::unflagMessages:
  case nsMsgViewCommandType::toggleThreadWatched:
  case nsMsgViewCommandType::markThreadRead:
  case nsMsgViewCommandType::downloadSelectedForOffline:
    *selectable_p = haveSelection;
    break;
  case nsMsgViewCommandType::junk:
  case nsMsgViewCommandType::unjunk:
    *selectable_p = haveSelection && !mIsNews;  // no junk for news yet
    break;
  case nsMsgViewCommandType::cmdRequiringMsgBody:
    *selectable_p = haveSelection && (!WeAreOffline() || OfflineMsgSelected(indices, numIndices));
    break;
  case nsMsgViewCommandType::downloadFlaggedForOffline:
  case nsMsgViewCommandType::markAllRead:
    *selectable_p = PR_TRUE;
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
  if (m_deletingRows)
  {
    NS_ASSERTION(PR_FALSE, "Last move did not complete");
    return NS_OK;
  }

  m_deletingRows = isMove && mDeleteModel != nsMsgImapDeleteModels::IMAPDelete;

  nsresult rv;
  NS_ENSURE_ARG_POINTER(destFolder);
  nsCOMPtr<nsIMutableArray> messageArray(do_CreateInstance(NS_ARRAY_CONTRACTID));
  for (nsMsgViewIndex index = 0; index < (nsMsgViewIndex) numIndices; index++)
  {
    nsMsgKey key;
    nsMsgViewIndex viewIndex = indices[index];
    if (viewIndex == nsMsgViewIndex_None)
      continue;
    key = m_keys[viewIndex];
    nsCOMPtr <nsIMsgDBHdr> msgHdr;
    rv = m_db->GetMsgHdrForKey(key, getter_AddRefs(msgHdr));
    if (NS_SUCCEEDED(rv) && msgHdr)
    {
      messageArray->AppendElement(msgHdr, PR_FALSE);
      // if we are deleting rows, save off the keys
      if (m_deletingRows)
        mIndicesToNoteChange.AppendElement(indices[index]);
    }
  }

  nsCOMPtr<nsIMsgCopyService> copyService = do_GetService(NS_MSGCOPYSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  return copyService->CopyMessages(m_folder /* source folder */, messageArray, destFolder, isMove, nsnull /* listener */, window, PR_TRUE /*allowUndo*/);
}

nsresult
nsMsgDBView::ApplyCommandToIndicesWithFolder(nsMsgViewCommandTypeValue command, nsMsgViewIndex* indices,
                    PRInt32 numIndices, nsIMsgFolder *destFolder)
{
  nsresult rv = NS_OK;
  NS_ENSURE_ARG_POINTER(destFolder);
  nsCOMPtr<nsIMsgWindow> msgWindow(do_QueryReferent(mMsgWindowWeak));
  switch (command) {
    case nsMsgViewCommandType::copyMessages:
        NS_ASSERTION(!(m_folder == destFolder), "The source folder and the destination folder are the same");
        if (m_folder != destFolder)
          rv = CopyMessages(msgWindow, indices, numIndices, PR_FALSE /* isMove */, destFolder);
        break;
    case nsMsgViewCommandType::moveMessages:
        NS_ASSERTION(!(m_folder == destFolder), "The source folder and the destination folder are the same");
        if (m_folder != destFolder)
          rv = CopyMessages(msgWindow, indices, numIndices, PR_TRUE  /* isMove */, destFolder);
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
  NS_ASSERTION(numIndices >= 0, "nsMsgDBView::ApplyCommandToIndices(): "
               "numIndices is negative!");

  if (numIndices == 0)
    return NS_OK; // return quietly, just in case

  nsCOMPtr<nsIMsgFolder> folder;
  nsresult rv = GetFolderForViewIndex(indices[0], getter_AddRefs(folder));
  nsCOMPtr<nsIMsgWindow> msgWindow(do_QueryReferent(mMsgWindowWeak));
  if (command == nsMsgViewCommandType::deleteMsg)
    return DeleteMessages(msgWindow, indices, numIndices, PR_FALSE);
  if (command == nsMsgViewCommandType::deleteNoTrash)
    return DeleteMessages(msgWindow, indices, numIndices, PR_TRUE);

  nsTArray<nsMsgKey> imapUids;
  nsCOMPtr <nsIMsgImapMailFolder> imapFolder = do_QueryInterface(folder);
  PRBool thisIsImapFolder = (imapFolder != nsnull);
  nsCOMPtr<nsIJunkMailPlugin> junkPlugin;

  // if this is a junk command, start a batch or add to an existing one.
  //
  if (    command == nsMsgViewCommandType::junk
       || command == nsMsgViewCommandType::unjunk )
  {
    // get the folder from the first item; we assume that
    // all messages in the view are from the same folder (no
    // more junk status column in the 'search messages' dialog
    // like in earlier versions...)
     //
     NS_ENSURE_SUCCESS(rv, rv);

     nsCOMPtr<nsIMsgIncomingServer> server;
     rv = folder->GetServer(getter_AddRefs(server));
     NS_ENSURE_SUCCESS(rv, rv);

    if (command == nsMsgViewCommandType::junk)
    {
      // append this batch of junk message indices to the
      // array of junk message indices to be acted upon
      // once OnMessageClassified() is run for the last message
      //
      // note: although message classification is done
      // asynchronously, it is not done in a different thread,
      // so the manipulations of mJunkIndices here and in
      // OnMessageClassified() cannot interrupt each other
      //
      mNumJunkIndices += numIndices;
      mJunkIndices = (nsMsgViewIndex *)nsMemory::Realloc(mJunkIndices, mNumJunkIndices * sizeof(nsMsgViewIndex));
      memcpy(mJunkIndices + (mNumJunkIndices - numIndices), indices, numIndices * sizeof(nsMsgViewIndex));
    }

    nsCOMPtr<nsIMsgFilterPlugin> filterPlugin;
    rv = server->GetSpamFilterPlugin(getter_AddRefs(filterPlugin));
    NS_ENSURE_SUCCESS(rv, rv);

    junkPlugin = do_QueryInterface(filterPlugin, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    // note that if we aren't starting a batch we are
    // actually coalescing the batch of messages this
    // function was called for with the previous
    // batch(es)
    mNumMessagesRemainingInBatch += numIndices;
  }

  folder->EnableNotifications(nsIMsgFolder::allMessageCountNotifications, PR_FALSE, PR_TRUE /*dbBatching*/);
  if (thisIsImapFolder && command != nsMsgViewCommandType::markThreadRead)
    imapUids.SetLength(numIndices);

  for (int32 i = 0; i < numIndices; i++)
  {
    if (thisIsImapFolder && command != nsMsgViewCommandType::markThreadRead)
      imapUids[i] = GetAt(indices[i]);

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
    case nsMsgViewCommandType::junk:
      rv = SetAsJunkByIndex(junkPlugin.get(), indices[i],
                             nsIJunkMailPlugin::JUNK);
      break;
    case nsMsgViewCommandType::unjunk:
    rv = SetAsJunkByIndex(junkPlugin.get(), indices[i],
                             nsIJunkMailPlugin::GOOD);
      break;
    case nsMsgViewCommandType::undeleteMsg:
      break; // this is completely handled in the imap code below.
    default:
      NS_ASSERTION(PR_FALSE, "unhandled command");
      break;
    }
  }

  folder->EnableNotifications(nsIMsgFolder::allMessageCountNotifications, PR_TRUE, PR_TRUE /*dbBatching*/);

  if (thisIsImapFolder)
  {
    imapMessageFlagsType flags = kNoImapMsgFlag;
    PRBool addFlags = PR_FALSE;
    PRBool isRead = PR_FALSE;
    nsCOMPtr<nsIMsgWindow> msgWindow(do_QueryReferent(mMsgWindowWeak));
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
    case nsMsgViewCommandType::undeleteMsg:
      flags = kImapMsgDeletedFlag;
      addFlags = PR_FALSE;
      break;
    case nsMsgViewCommandType::junk:
        return imapFolder->StoreCustomKeywords(msgWindow,
                    NS_LITERAL_CSTRING("Junk"),
                    NS_LITERAL_CSTRING("NonJunk"),
                    imapUids.Elements(), imapUids.Length(),
                    nsnull);
    case nsMsgViewCommandType::unjunk:
        return imapFolder->StoreCustomKeywords(msgWindow,
                    NS_LITERAL_CSTRING("NonJunk"),
                    NS_LITERAL_CSTRING("Junk"),
                    imapUids.Elements(), imapUids.Length(),
                    nsnull);

    default:
      break;
    }

    if (flags != kNoImapMsgFlag)  // can't get here without thisIsImapThreadPane == TRUE
      imapFolder->StoreImapFlags(flags, addFlags, imapUids.Elements(), imapUids.Length(), nsnull);

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
  m_keys.RemoveElementAt(index);
  m_flags.RemoveElementAt(index);
  m_levels.RemoveElementAt(index);

  // the call to NoteChange() has to happen after we remove the key
  // as NoteChange() will call RowCountChanged() which will call our GetRowCount()
  if (!m_deletingRows)
    NoteChange(index, -1, nsMsgViewNotificationCode::insertOrDelete); // an example where view is not the listener - D&D messages

  return NS_OK;
}

nsresult nsMsgDBView::DeleteMessages(nsIMsgWindow *window, nsMsgViewIndex *indices, PRInt32 numIndices, PRBool deleteStorage)
{
  if (m_deletingRows)
  {
    NS_WARNING("Last delete did not complete");
    return NS_OK;
  }
  if (mDeleteModel != nsMsgImapDeleteModels::IMAPDelete)
    m_deletingRows = PR_TRUE;

  nsresult rv;
  nsCOMPtr<nsIMutableArray> messageArray(do_CreateInstance(NS_ARRAY_CONTRACTID));
  for (nsMsgViewIndex index = 0; index < (nsMsgViewIndex) numIndices; index++)
  {
    if (m_flags[indices[index]] & MSG_VIEW_FLAG_DUMMY)
      continue;
    nsMsgKey key = m_keys[indices[index]];
    nsCOMPtr <nsIMsgDBHdr> msgHdr;
    rv = m_db->GetMsgHdrForKey(key, getter_AddRefs(msgHdr));
    if (NS_SUCCEEDED(rv) && msgHdr)
    {
      messageArray->AppendElement(msgHdr, PR_FALSE);
      // if we are deleting rows, save off the keys
      if (m_deletingRows)
        mIndicesToNoteChange.AppendElement(indices[index]);
    }
  }

  rv = m_folder->DeleteMessages(messageArray, window, deleteStorage, PR_FALSE, nsnull, PR_TRUE /*allow Undo*/ );
  if (NS_FAILED(rv))
    m_deletingRows = PR_FALSE;
  return rv;
}

nsresult nsMsgDBView::DownloadForOffline(nsIMsgWindow *window, nsMsgViewIndex *indices, PRInt32 numIndices)
{
  nsresult rv = NS_OK;
  nsCOMPtr<nsIMutableArray> messageArray(do_CreateInstance(NS_ARRAY_CONTRACTID));
  for (nsMsgViewIndex index = 0; index < (nsMsgViewIndex) numIndices; index++)
  {
    nsMsgKey key = m_keys[indices[index]];
    nsCOMPtr <nsIMsgDBHdr> msgHdr;
    rv = m_db->GetMsgHdrForKey(key, getter_AddRefs(msgHdr));
    NS_ENSURE_SUCCESS(rv,rv);
    if (msgHdr)
    {
      PRUint32 flags;
      msgHdr->GetFlags(&flags);
      if (!(flags & MSG_FLAG_OFFLINE))
        messageArray->AppendElement(msgHdr, PR_FALSE);
    }
  }
  m_folder->DownloadMessagesForOffline(messageArray, window);
  return rv;
}

nsresult nsMsgDBView::DownloadFlaggedForOffline(nsIMsgWindow *window)
{
  nsresult rv = NS_OK;
  nsCOMPtr<nsIMutableArray> messageArray(do_CreateInstance(NS_ARRAY_CONTRACTID));
  nsCOMPtr <nsISimpleEnumerator> enumerator;
  rv = m_db->EnumerateMessages(getter_AddRefs(enumerator));
  if (NS_SUCCEEDED(rv) && enumerator)
  {
    PRBool hasMore;

    while (NS_SUCCEEDED(rv = enumerator->HasMoreElements(&hasMore)) && (hasMore == PR_TRUE))
    {
      nsCOMPtr <nsIMsgDBHdr> pHeader;
      rv = enumerator->GetNext(getter_AddRefs(pHeader));
      NS_ASSERTION(NS_SUCCEEDED(rv), "nsMsgDBEnumerator broken");
      if (pHeader && NS_SUCCEEDED(rv))
      {
        PRUint32 flags;
        pHeader->GetFlags(&flags);
        if ((flags & MSG_FLAG_MARKED) && !(flags & MSG_FLAG_OFFLINE))
          messageArray->AppendElement(pHeader, PR_FALSE);
      }
    }
  }
  m_folder->DownloadMessagesForOffline(messageArray, window);
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
  {
    OrExtraFlag(index, MSG_FLAG_READ);
    // MarkRead() will clear this flag in the db
    // and then call OnKeyChange(), but
    // because we are the instigator of the change
    // we'll ignore the change.
    //
    // so we need to clear it in m_flags
    // to keep the db and m_flags in sync
    AndExtraFlag(index, ~MSG_FLAG_NEW);
  }
  else
  {
    AndExtraFlag(index, ~MSG_FLAG_READ);
  }

  nsCOMPtr <nsIMsgDatabase> dbToUse;
  rv = GetDBForViewIndex(index, getter_AddRefs(dbToUse));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = dbToUse->MarkRead(m_keys[index], read, this);
  NoteChange(index, 1, nsMsgViewNotificationCode::changed);
  if (m_viewFlags & nsMsgViewFlagsType::kThreadedDisplay)
  {
    nsMsgViewIndex threadIndex = ThreadIndexOfMsg(m_keys[index], index, nsnull, nsnull);
    if (threadIndex != index)
      NoteChange(threadIndex, 1, nsMsgViewNotificationCode::changed);
  }
  return rv;
}

nsresult nsMsgDBView::SetThreadOfMsgReadByIndex(nsMsgViewIndex index, nsTArray<nsMsgKey> &keysMarkedRead, PRBool /*read*/)
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

nsresult nsMsgDBView::SetStringPropertyByIndex(nsMsgViewIndex index, const char *aProperty, const char *aValue)
{
  if (!IsValidIndex(index))
    return NS_MSG_INVALID_DBVIEW_INDEX;

  nsCOMPtr <nsIMsgDatabase> dbToUse;
  nsresult rv = GetDBForViewIndex(index, getter_AddRefs(dbToUse));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = dbToUse->SetStringProperty(m_keys[index], aProperty, aValue);
  NoteChange(index, 1, nsMsgViewNotificationCode::changed);
  return rv;
}

nsresult nsMsgDBView::SetAsJunkByIndex(nsIJunkMailPlugin *aJunkPlugin,
                                          nsMsgViewIndex aIndex,
                                          nsMsgJunkStatus aNewClassification)
{
    // get the message header (need this to get string properties)
    //
    nsCOMPtr <nsIMsgDBHdr> msgHdr;
    nsresult rv = GetMsgHdrForViewIndex(aIndex, getter_AddRefs(msgHdr));
    NS_ENSURE_SUCCESS(rv, rv);

    // get the old junk score
    //
    nsCString junkScoreStr;
    rv = msgHdr->GetStringProperty("junkscore", getter_Copies(junkScoreStr));

    // and the old origin
    //
    nsCString oldOriginStr;
    rv = msgHdr->GetStringProperty("junkscoreorigin",
                                   getter_Copies(oldOriginStr));

    // if this was not classified by the user, say so
    //
    nsMsgJunkStatus oldUserClassification;
    if (oldOriginStr.get()[0] != 'u') {
        oldUserClassification = nsIJunkMailPlugin::UNCLASSIFIED;
    }
    else {
        // otherwise, pass the actual user classification
        if (junkScoreStr.IsEmpty())
          oldUserClassification = nsIJunkMailPlugin::UNCLASSIFIED;
        else if (junkScoreStr.ToInteger((PRInt32*)&rv) == nsIJunkMailPlugin::IS_SPAM_SCORE)
          oldUserClassification = nsIJunkMailPlugin::JUNK;
        else
          oldUserClassification = nsIJunkMailPlugin::GOOD;

        NS_ASSERTION(NS_SUCCEEDED(rv), "Converting junkScore to integer failed.");
    }

    // get the URI for this message so we can pass it to the plugin
    //
    nsCString uri;
    rv = GetURIForViewIndex(aIndex, uri);
    NS_ENSURE_SUCCESS(rv, rv);

    // tell the plugin about this change, so that it can (potentially)
    // adjust its database appropriately
    nsCOMPtr<nsIMsgWindow> msgWindow(do_QueryReferent(mMsgWindowWeak));
    rv = aJunkPlugin->SetMessageClassification(
        uri.get(), oldUserClassification, aNewClassification, msgWindow, this);
    NS_ENSURE_SUCCESS(rv, rv);

    // this routine is only reached if the user someone touched the UI
    // and told us the junk status of this message.
    // Set origin first so that listeners on the junkscore will
    // know the correct origin.
    rv = SetStringPropertyByIndex(aIndex, "junkscoreorigin", "user");
    NS_ASSERTION(NS_SUCCEEDED(rv), "SetStringPropertyByIndex failed");

    // set the junk score on the message itself

    nsCAutoString msgJunkScore;
    msgJunkScore.AppendInt(aNewClassification == nsIJunkMailPlugin::JUNK ?
          nsIJunkMailPlugin::IS_SPAM_SCORE:
          nsIJunkMailPlugin::IS_HAM_SCORE);
    rv = SetStringPropertyByIndex(aIndex, "junkscore", msgJunkScore.get());
    NS_ENSURE_SUCCESS(rv, rv);

    return rv;
}

nsresult
nsMsgDBView::GetFolderFromMsgURI(const char *aMsgURI, nsIMsgFolder **aFolder)
{
  NS_IF_ADDREF(*aFolder = m_folder);
  return NS_OK;
}

NS_IMETHODIMP
nsMsgDBView::OnMessageClassified(const char *aMsgURI,
                                 nsMsgJunkStatus aClassification,
                                 PRUint32 aJunkPercent)

{
  // Note: we know all messages in a batch have the same
  // classification, since unlike OnMessageClassified
  // methods in other classes (such as nsLocalMailFolder
  // and nsImapMailFolder), this class, nsMsgDBView, currently
  // only triggers message classifications due to a command to
  // mark some of the messages in the view as junk, or as not
  // junk - so the classification is dictated to the filter,
  // not suggested by it.
  //
  // for this reason the only thing we (may) have to do is
  // perform the action on all of the junk messages
  //

  NS_ASSERTION( (aClassification == nsIJunkMailPlugin::GOOD) || (mJunkIndices != nsnull), "the classification of a manually-marked junk message has been classified as junk, yet there seem to be no such outstanding messages");

  // is this the last message in the batch?

  if (--mNumMessagesRemainingInBatch == 0)
  {
    if ( mNumJunkIndices > 0 )
    {
      PerformActionsOnJunkMsgs();
      nsMemory::Free(mJunkIndices);
      mJunkIndices = nsnull;
      mNumJunkIndices = 0;
    }
  }
  return NS_OK;
}

nsresult
nsMsgDBView::PerformActionsOnJunkMsgs()
{
  PRBool movingJunkMessages,markingJunkMessagesRead;
  nsCOMPtr <nsIMsgFolder> junkTargetFolder;

  // question: is it possible for the junk mail move/mark as read
  // options to change after we've handled some of the batches but
  // before we've handled the last one? if so, we can decide when
  // handling each batch whether to save its indices or forget
  // them, and then perform the known action when handling the
  // last batch; however if the options can change between batches
  // we may have to remember in separate arrays the indices to
  // mark as read and the indices to move
  //
  // for now, we assume the options do not change between batches
  //

  nsresult rv = DetermineActionsForJunkMsgs(&movingJunkMessages, &markingJunkMessagesRead, getter_AddRefs(junkTargetFolder));
  NS_ENSURE_SUCCESS(rv,rv);

  // nothing to do, bail out
  if (!(movingJunkMessages || markingJunkMessagesRead))
    return NS_OK;

  NS_ASSERTION( (mNumJunkIndices > 0), "no indices of marked-as-junk messages to act on");

  if (mNumJunkIndices > 1)
    NS_QuickSort(mJunkIndices, mNumJunkIndices, sizeof(nsMsgViewIndex), CompareViewIndices, nsnull);

  if (markingJunkMessagesRead)
  {
    // notes on marking junk as read:
    // 1. there are 2 occasions on which junk messages are marked as
    //    read: after a manual marking (here and in the front end) and after
    //    automatic classification by the bayesian filter (see code for local
    //    mail folders and for imap mail folders). The server-specific
    //    markAsReadOnSpam pref only applies to the latter, the former is
    //    controlled by "mailnews.ui.junk.manualMarkAsJunkMarksRead".
    // 2. even though move/delete on manual mark may be
    //    turned off, we might still need to mark as read

    NoteStartChange(nsMsgViewNotificationCode::none, 0, 0);
    rv = ApplyCommandToIndices(nsMsgViewCommandType::markMessagesRead, mJunkIndices, mNumJunkIndices);
    NoteEndChange(nsMsgViewNotificationCode::none, 0, 0);
    NS_ASSERTION(NS_SUCCEEDED(rv), "marking marked-as-junk messages as read failed");
  }
  if (movingJunkMessages)
  {
    // check if one of the messages to be junked is actually selected
    // if more than one message being junked, one must be selected.
    // if no tree selection at all, must be in stand-alone message window.
    PRBool junkedMsgSelected = mNumJunkIndices > 1 || !mTreeSelection;
    for (nsMsgViewIndex junkIndex = 0; !junkedMsgSelected && junkIndex < mNumJunkIndices; junkIndex++)
      mTreeSelection->IsSelected(mJunkIndices[junkIndex], &junkedMsgSelected);

    // if a junked msg is selected, tell the FE to call SetNextMessageAfterDelete() because a delete is coming
    if (junkedMsgSelected && mCommandUpdater)
    {
      rv = mCommandUpdater->UpdateNextMessageAfterDelete();
      NS_ENSURE_SUCCESS(rv,rv);
    }

    NoteStartChange(nsMsgViewNotificationCode::none, 0, 0);
    if (junkTargetFolder)
      rv = ApplyCommandToIndicesWithFolder(nsMsgViewCommandType::moveMessages, mJunkIndices, mNumJunkIndices, junkTargetFolder);
    else
      rv = ApplyCommandToIndices(nsMsgViewCommandType::deleteMsg, mJunkIndices, mNumJunkIndices);
    NoteEndChange(nsMsgViewNotificationCode::none, 0, 0);

    NS_ASSERTION(NS_SUCCEEDED(rv), "move or deletion of marked-as-junk messages failed");
  }
  return rv;
}

nsresult
nsMsgDBView::DetermineActionsForJunkMsgs(PRBool* movingJunkMessages, PRBool* markingJunkMessagesRead, nsIMsgFolder** junkTargetFolder)
{
  // there are two possible actions which may be performed
  // on messages marked as spam: marking as read and moving
  // somewhere...

  *movingJunkMessages = false;
  *markingJunkMessagesRead = false;

  // ... the 'somewhere', junkTargetFolder, can be a folder,
  // but if it remains null we'll delete the messages

  *junkTargetFolder = nsnull;

  nsCOMPtr<nsIMsgFolder> folder;
  nsresult rv = GetFolderForViewIndex(mJunkIndices[0], getter_AddRefs(folder));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIMsgIncomingServer> server;
  rv = folder->GetServer(getter_AddRefs(server));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr <nsISpamSettings> spamSettings;
  rv = server->GetSpamSettings(getter_AddRefs(spamSettings));
  NS_ENSURE_SUCCESS(rv, rv);

  // if the spam system is completely disabled we won't do anything
  // question: is this a valid choice?
  PRInt32 spamLevel;
  (void)spamSettings->GetLevel(&spamLevel);
  if (!spamLevel)
    return NS_OK;

  // When the user explicitly marks a message as junk, we can mark it as read,
  // too. This is independent of the "markAsReadOnSpam" pref, which applies
  // only to automatically-classified messages.
  // Note that this behaviour should match the one in the front end for marking
  // as junk via toolbar/context menu.
  nsCOMPtr<nsIPrefBranch> prefBranch(do_GetService(NS_PREFSERVICE_CONTRACTID, &rv));
  if (NS_SUCCEEDED(rv))
  {
    prefBranch->GetBoolPref("mailnews.ui.junk.manualMarkAsJunkMarksRead",
                            markingJunkMessagesRead);
  }

  // now let's determine whether we'll be taking the second action,
  // the move / deletion (and also determine which of these two)

  PRBool manualMark;
  (void)spamSettings->GetManualMark(&manualMark);
  if (!manualMark)
    return NS_OK;

  PRInt32 manualMarkMode;
  (void)spamSettings->GetManualMarkMode(&manualMarkMode);
  NS_ASSERTION(manualMarkMode == nsISpamSettings::MANUAL_MARK_MODE_MOVE
            || manualMarkMode == nsISpamSettings::MANUAL_MARK_MODE_DELETE,
            "bad manual mark mode");

  // the folder must allow us to execute the move (or the deletion)
  PRUint32 folderFlags;
  folder->GetFlags(&folderFlags);

  if (manualMarkMode == nsISpamSettings::MANUAL_MARK_MODE_MOVE)
  {
    // if this is a junk folder
    // (not only "the" junk folder for this account)
    // don't do the move
    if (folderFlags & nsMsgFolderFlags::Junk)
      return NS_OK;

    nsCString spamFolderURI;
    rv = spamSettings->GetSpamFolderURI(getter_Copies(spamFolderURI));
    NS_ENSURE_SUCCESS(rv,rv);

    NS_ASSERTION(!spamFolderURI.IsEmpty(), "spam folder URI is empty, can't move");
    if (!spamFolderURI.IsEmpty())
    {
      //nsCOMPtr<nsIMsgFolder> destFolder;
      rv = GetExistingFolder(spamFolderURI, junkTargetFolder);
      NS_ENSURE_SUCCESS(rv,rv);

      *movingJunkMessages = true;
    }
    return NS_OK;
  }

  // at this point manualMarkMode == nsISpamSettings::MANUAL_MARK_MODE_DELETE)

  // if this is in the trash, let's not delete
  if (folderFlags & nsMsgFolderFlags::Trash)
    return NS_OK;

  return folder->GetCanDeleteMessages(movingJunkMessages);
}

// reversing threads involves reversing the threads but leaving the
// expanded messages ordered relative to the thread, so we
// make a copy of each array and copy them over.
void nsMsgDBView::ReverseThreads()
{
    nsTArray<PRUint32> newFlagArray;
    nsTArray<nsMsgKey> newKeyArray;
    nsTArray<PRUint8> newLevelArray;

    PRUint32 viewSize = GetSize();
    PRUint32 startThread = viewSize;
    PRUint32 nextThread = viewSize;
    PRUint32 destIndex = 0;

    newKeyArray.SetLength(m_keys.Length());
    newFlagArray.SetLength(m_flags.Length());
    newLevelArray.SetLength(m_levels.Length());

    while (startThread)
    {
        startThread--;

        if (m_flags[startThread] & MSG_VIEW_FLAG_ISTHREAD)
        {
            for (PRUint32 sourceIndex = startThread; sourceIndex < nextThread; sourceIndex++)
            {
                newKeyArray[destIndex] = m_keys[sourceIndex];
                newFlagArray[destIndex] = m_flags[sourceIndex];
                newLevelArray[destIndex] = m_levels[sourceIndex];
                destIndex++;
            }
            nextThread = startThread; // because we're copying in reverse order
        }
    }
    m_keys.SwapElements(newKeyArray);
    m_flags.SwapElements(newFlagArray);
    m_levels.SwapElements(newLevelArray);
}

void nsMsgDBView::ReverseSort()
{
    PRUint32 topIndex = GetSize();

    nsCOMPtr <nsISupportsArray> folders;
    GetFolders(getter_AddRefs(folders));

    // go up half the array swapping values
    for (PRUint32 bottomIndex = 0; bottomIndex < --topIndex; bottomIndex++) 
    {
        // swap flags
        PRUint32 tempFlags = m_flags[bottomIndex];
        m_flags[bottomIndex] = m_flags[topIndex];
        m_flags[topIndex] = tempFlags;

        // swap keys
        nsMsgKey tempKey = m_keys[bottomIndex];
        m_keys[bottomIndex] = m_keys[topIndex];
        m_keys[topIndex] = tempKey;

        if (folders)
        {
            // swap folders --
            // needed when search is done across multiple folders
            nsCOMPtr<nsISupports> tmpSupports = dont_AddRef(folders->ElementAt(bottomIndex));
            nsCOMPtr<nsISupports> topSupports = dont_AddRef(folders->ElementAt(topIndex));
            folders->SetElementAt(bottomIndex, topSupports);
            folders->SetElementAt(topIndex, tmpSupports);
        }
        // no need to swap elements in m_levels; since we only call
        // ReverseSort in non-threaded mode, m_levels are all the same.
    }
}

struct IdDWord
{
    nsMsgKey    id;
    PRUint32    bits;
    PRUint32    dword;
    nsISupports* folder;
};

struct IdKey : public IdDWord
{
    PRUint8     key[1];
};

struct IdKeyPtr : public IdDWord
{
    PRUint8     *key;
};

int PR_CALLBACK
nsMsgDBView::FnSortIdKey(const void *pItem1, const void *pItem2, void *privateData)
{
    PRInt32 retVal = 0;
    nsresult rv;
    viewSortInfo* sortInfo = (viewSortInfo *) privateData;

    IdKey** p1 = (IdKey**)pItem1;
    IdKey** p2 = (IdKey**)pItem2;

    nsIMsgDatabase *db = sortInfo->db;

    rv = db->CompareCollationKeys((*p1)->key, (*p1)->dword, (*p2)->key, (*p2)->dword, &retVal);
    NS_ASSERTION(NS_SUCCEEDED(rv),"compare failed");

    if (retVal)
      return sortInfo->ascendingSort ? retVal : ~retVal;
    if (sortInfo->view->m_secondarySort == nsMsgViewSortType::byId)
      return (sortInfo->view->m_secondarySortOrder == nsMsgViewSortOrder::ascending &&
              (*p1)->id >= (*p2)->id) ? 1 : -1;
    else
      return sortInfo->view->SecondarySort((*p1)->id, (*p1)->folder, (*p2)->id, (*p2)->folder, sortInfo);
    // here we'd use the secondary sort
}

int PR_CALLBACK
nsMsgDBView::FnSortIdKeyPtr(const void *pItem1, const void *pItem2, void *privateData)
{
  PRInt32 retVal = 0;
  nsresult rv;

  IdKeyPtr** p1 = (IdKeyPtr**)pItem1;
  IdKeyPtr** p2 = (IdKeyPtr**)pItem2;
  viewSortInfo* sortInfo = (viewSortInfo *) privateData;

  nsIMsgDatabase *db = sortInfo->db;

  rv = db->CompareCollationKeys((*p1)->key, (*p1)->dword, (*p2)->key, (*p2)->dword, &retVal);
  NS_ASSERTION(NS_SUCCEEDED(rv),"compare failed");

  if (retVal)
    return sortInfo->ascendingSort ? retVal : ~retVal;

  if (sortInfo->view->m_secondarySort == nsMsgViewSortType::byId)
    return (sortInfo->view->m_secondarySortOrder == nsMsgViewSortOrder::ascending &&
            (*p1)->id >= (*p2)->id) ? 1 : -1;
  else
    return sortInfo->view->SecondarySort((*p1)->id, (*p1)->folder, (*p2)->id, (*p2)->folder, sortInfo);
}

int PR_CALLBACK
nsMsgDBView::FnSortIdDWord(const void *pItem1, const void *pItem2, void *privateData)
{
  IdDWord** p1 = (IdDWord**)pItem1;
  IdDWord** p2 = (IdDWord**)pItem2;
  viewSortInfo* sortInfo = (viewSortInfo *) privateData;

  if ((*p1)->dword > (*p2)->dword)
    return (sortInfo->ascendingSort) ? 1 : -1;
  else if ((*p1)->dword < (*p2)->dword)
      return (sortInfo->ascendingSort) ? -1 : 1;
  if (sortInfo->view->m_secondarySort == nsMsgViewSortType::byId)
    return (sortInfo->view->m_secondarySortOrder == nsMsgViewSortOrder::ascending &&
            (*p1)->id >= (*p2)->id) ? 1 : -1;
  else
    return sortInfo->view->SecondarySort((*p1)->id, (*p1)->folder, (*p2)->id, (*p2)->folder, sortInfo);
}


// XXX are these still correct?
//To compensate for memory alignment required for
//systems such as HP-UX these values must be 4 bytes
//aligned.  Don't break this when modify the constants
const int kMaxSubjectKey = 160;
const int kMaxLocationKey = 160;  // also used for account
const int kMaxAuthorKey = 160;
const int kMaxRecipientKey = 80;

nsresult nsMsgDBView::GetFieldTypeAndLenForSort(nsMsgViewSortTypeValue sortType, PRUint16 *pMaxLen, eFieldType *pFieldType)
{
    NS_ENSURE_ARG_POINTER(pMaxLen);
    NS_ENSURE_ARG_POINTER(pFieldType);

    switch (sortType)
    {
        case nsMsgViewSortType::bySubject:
            *pFieldType = kCollationKey;
            *pMaxLen = kMaxSubjectKey;
            break;
        case nsMsgViewSortType::byAccount:
        case nsMsgViewSortType::byTags:
        case nsMsgViewSortType::byLocation:
            *pFieldType = kCollationKey;
            *pMaxLen = kMaxLocationKey;
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
        case nsMsgViewSortType::byReceived:
        case nsMsgViewSortType::byPriority:
        case nsMsgViewSortType::byThread:
        case nsMsgViewSortType::byId:
        case nsMsgViewSortType::bySize:
        case nsMsgViewSortType::byFlagged:
        case nsMsgViewSortType::byUnread:
        case nsMsgViewSortType::byStatus:
        case nsMsgViewSortType::byJunkStatus:
        case nsMsgViewSortType::byAttachments:
            *pFieldType = kU32;
            *pMaxLen = 0;
            break;
        case nsMsgViewSortType::byCustom:
        {
          nsIMsgCustomColumnHandler* colHandler = GetCurColumnHandlerFromDBInfo();

          if (colHandler != nsnull)
          {
            PRBool isString;
            colHandler->IsString(&isString);

            if (isString)
            {
              *pFieldType = kCollationKey;
              *pMaxLen = kMaxRecipientKey; //80 - do we need a seperate k?
            }
            else
            {
              *pFieldType = kU32;
              *pMaxLen = 0;
            }
          }
          break;
        }
        default:
            return NS_ERROR_UNEXPECTED;
    }

    return NS_OK;
}

#define MSG_STATUS_MASK (MSG_FLAG_REPLIED | MSG_FLAG_FORWARDED)

nsresult nsMsgDBView::GetStatusSortValue(nsIMsgDBHdr *msgHdr, PRUint32 *result)
{
  NS_ENSURE_ARG_POINTER(msgHdr);
  NS_ENSURE_ARG_POINTER(result);

  PRUint32 messageFlags;
  nsresult rv = msgHdr->GetFlags(&messageFlags);
  NS_ENSURE_SUCCESS(rv,rv);

  if (messageFlags & MSG_FLAG_NEW)
  {
    // happily, new by definition stands alone
    *result = 0;
    return NS_OK;
  }

  switch (messageFlags & MSG_STATUS_MASK)
  {
    case MSG_FLAG_REPLIED:
        *result = 2;
        break;
    case MSG_FLAG_FORWARDED|MSG_FLAG_REPLIED:
        *result = 1;
        break;
    case MSG_FLAG_FORWARDED:
        *result = 3;
        break;
    default:
        *result = (messageFlags & MSG_FLAG_READ) ? 4 : 5;
        break;
    }

    return NS_OK;
}

nsresult nsMsgDBView::GetLongField(nsIMsgDBHdr *msgHdr, nsMsgViewSortTypeValue sortType, PRUint32 *result, nsIMsgCustomColumnHandler* colHandler)
{
  nsresult rv;
  NS_ENSURE_ARG_POINTER(msgHdr);
  NS_ENSURE_ARG_POINTER(result);

  PRBool isRead;
  PRUint32 bits;

  switch (sortType)
  {
    case nsMsgViewSortType::bySize:
      rv = (mShowSizeInLines) ? msgHdr->GetLineCount(result) : msgHdr->GetMessageSize(result);
      break;
    case nsMsgViewSortType::byPriority:
        nsMsgPriorityValue priority;
        rv = msgHdr->GetPriority(&priority);

        // treat "none" as "normal" when sorting.
        if (priority == nsMsgPriority::none)
            priority = nsMsgPriority::normal;

        // we want highest priority to have lowest value
        // so ascending sort will have highest priority first.
        *result = nsMsgPriority::highest - priority;
        break;
    case nsMsgViewSortType::byStatus:
        rv = GetStatusSortValue(msgHdr,result);
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
    case nsMsgViewSortType::byJunkStatus:
      {
        nsCString junkScoreStr;
        rv = msgHdr->GetStringProperty("junkscore", getter_Copies(junkScoreStr));
        // unscored messages should come before messages that are scored
        // junkScoreStr is "", and "0" - "100"
        // normalize to 0 - 101
        *result = junkScoreStr.IsEmpty() ? (0) : atoi(junkScoreStr.get()) + 1;
      }
      break;
     case nsMsgViewSortType::byAttachments:
        bits = 0;
        rv = msgHdr->GetFlags(&bits);
        *result = !(bits & MSG_FLAG_ATTACHMENT);
      break;
    case nsMsgViewSortType::byDate:
      // when sorting threads by date, we want the date of the newest msg
      // in the thread
      if (m_viewFlags & nsMsgViewFlagsType::kThreadedDisplay
        && ! (m_viewFlags & nsMsgViewFlagsType::kGroupBySort))
      {
        nsCOMPtr <nsIMsgThread> thread;
        rv = m_db->GetThreadContainingMsgHdr(msgHdr, getter_AddRefs(thread));
        NS_ENSURE_SUCCESS(rv, rv);
        thread->GetNewestMsgDate(result);
      }
      else
        rv = msgHdr->GetDateInSeconds(result);
      break;
    case nsMsgViewSortType::byReceived:
      // when sorting threads by received date, we want the received date of the newest msg
      // in the thread
      if (m_viewFlags & nsMsgViewFlagsType::kThreadedDisplay
        && ! (m_viewFlags & nsMsgViewFlagsType::kGroupBySort))
      {
        nsCOMPtr <nsIMsgThread> thread;
        rv = m_db->GetThreadContainingMsgHdr(msgHdr, getter_AddRefs(thread));
        NS_ENSURE_SUCCESS(rv, rv);
        thread->GetNewestMsgDate(result);
      }
      else
      {
        rv = msgHdr->GetUint32Property("dateReceived", result);  // Already in seconds...
        if (*result == 0)  // Use Date instead, we have no Received property
          rv = msgHdr->GetDateInSeconds(result);
      }
      break;
    case nsMsgViewSortType::byCustom:
      if (colHandler != nsnull)
      {
        colHandler->GetSortLongForRow(msgHdr, result);
        rv = NS_OK;
      }
      else
      {
        NS_ASSERTION(PR_FALSE, "should not be here (Sort Type: byCustom (Long), but no custom handler)");
        rv = NS_ERROR_UNEXPECTED;
      }
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

MsgViewSortColumnInfo::MsgViewSortColumnInfo(const MsgViewSortColumnInfo &other)
{
  mSortType = other.mSortType;
  mSortOrder = other.mSortOrder;
  mCustomColumnName = other.mCustomColumnName;
  mColHandler = other.mColHandler;
}

PRBool MsgViewSortColumnInfo::operator== (const MsgViewSortColumnInfo& other) const
{
  return (mSortType == nsMsgViewSortType::byCustom) ? 
    mCustomColumnName.Equals(other.mCustomColumnName) : mSortType == other.mSortType;
}

nsresult nsMsgDBView::EncodeColumnSort(nsString &columnSortString)
{
  for (PRUint32 i = 0; i < m_sortColumns.Length(); i++)
  {
    MsgViewSortColumnInfo &sortInfo = m_sortColumns[i];
    columnSortString.Append((char) sortInfo.mSortType);
    columnSortString.Append((char) sortInfo.mSortOrder + '0');
    if (sortInfo.mSortType == nsMsgViewSortType::byCustom)
    {
      columnSortString.Append(sortInfo.mCustomColumnName);
      columnSortString.Append((PRUnichar) '\r');
    }
  }
  return NS_OK;
}

nsresult nsMsgDBView::DecodeColumnSort(nsString &columnSortString)
{
  const PRUnichar *stringPtr = columnSortString.BeginReading();
  while (*stringPtr)
  {
    MsgViewSortColumnInfo sortColumnInfo;
    sortColumnInfo.mSortType = (nsMsgViewSortTypeValue) *stringPtr++;
    sortColumnInfo.mSortOrder = (nsMsgViewSortOrderValue) (*stringPtr++) - '0';
    if (sortColumnInfo.mSortType == nsMsgViewSortType::byCustom)
    {
      while (*stringPtr && *stringPtr != '\r')
        sortColumnInfo.mCustomColumnName.Append(*stringPtr++);
      sortColumnInfo.mColHandler = GetColumnHandler(sortColumnInfo.mCustomColumnName.get());
      if (*stringPtr) // advance past '\r'
        stringPtr++;
    }
    m_sortColumns.AppendElement(sortColumnInfo);
  }
  return NS_OK;
}

void nsMsgDBView::PushSort(const MsgViewSortColumnInfo &newSort)
{
  // no sense in keeping secondary sorts if primary sort is date or id
  if (newSort.mSortType == nsMsgViewSortType::byDate || newSort.mSortType == nsMsgViewSortType::byId)
    m_sortColumns.Clear();
  PRInt32 sortIndex = m_sortColumns.IndexOf(newSort, 0);
  if (sortIndex != kNotFound)
    m_sortColumns. RemoveElementAt(sortIndex);
  m_sortColumns.InsertElementAt(0, newSort);
  if (m_sortColumns.Length() > kMaxNumSortColumns)
    m_sortColumns.RemoveElementAt(kMaxNumSortColumns);
}

nsresult
nsMsgDBView::GetCollationKey(nsIMsgDBHdr *msgHdr, nsMsgViewSortTypeValue sortType, PRUint8 **result, PRUint32 *len, nsIMsgCustomColumnHandler* colHandler)
{
  nsresult rv;
  NS_ENSURE_ARG_POINTER(msgHdr);
  NS_ENSURE_ARG_POINTER(result);

  switch (sortType)
  {
    case nsMsgViewSortType::bySubject:
        rv = msgHdr->GetSubjectCollationKey(result, len);
        break;
    case nsMsgViewSortType::byLocation:
        rv = GetLocationCollationKey(msgHdr, result, len);
        break;
    case nsMsgViewSortType::byRecipient:
        rv = msgHdr->GetRecipientsCollationKey(result, len);
        break;
    case nsMsgViewSortType::byAuthor:
        rv = msgHdr->GetAuthorCollationKey(result, len);
        break;
    case nsMsgViewSortType::byAccount:
    case nsMsgViewSortType::byTags:
      {
        nsString str;
        nsCOMPtr <nsIMsgDatabase> dbToUse = m_db;

        if (!dbToUse) // probably search view
          GetDBForViewIndex(0, getter_AddRefs(dbToUse));

        rv = (sortType == nsMsgViewSortType::byAccount)
            ? FetchAccount(msgHdr, str)
            : FetchTags(msgHdr, str);

        if (NS_SUCCEEDED(rv) && dbToUse)
          rv = dbToUse->CreateCollationKey(str, result, len);
      }
      break;
    case nsMsgViewSortType::byCustom:
      if (colHandler != nsnull)
      {
        nsAutoString strKey;
        rv = colHandler->GetSortStringForRow(msgHdr, strKey);
        NS_ASSERTION(NS_SUCCEEDED(rv),"failed to get sort string for custom row");
        nsAutoString strTemp(strKey);

        rv = m_db->CreateCollationKey(strKey, result, len);
      }
      else
      {
        NS_ASSERTION(PR_FALSE,"should not be here (Sort Type: byCustom (String), but no custom handler)");
        //rv = NS_ERROR_UNEXPECTED;
      }
      break;
    default:
        rv = NS_ERROR_UNEXPECTED;
        break;
    }

    // bailing out with failure will stop the sort and leave us in
    // a bad state.  try to continue on, instead
    NS_ASSERTION(NS_SUCCEEDED(rv),"failed to get the collation key");
    if (NS_FAILED(rv))
    {
        *result = nsnull;
        *len = 0;
    }
    return NS_OK;
}

// As the location collation key is created getting folder from the msgHdr,
// it is defined in this file and not from the db.
nsresult
nsMsgDBView::GetLocationCollationKey(nsIMsgDBHdr *msgHdr, PRUint8 **result, PRUint32 *len)
{
  nsCOMPtr <nsIMsgFolder> folder;

  nsresult rv = msgHdr->GetFolder(getter_AddRefs(folder));
  NS_ENSURE_SUCCESS(rv,rv);
  nsCOMPtr <nsIMsgDatabase> dbToUse;
  rv = folder->GetMsgDatabase(nsnull, getter_AddRefs(dbToUse));
  NS_ENSURE_SUCCESS(rv,rv);

  nsString locationString;
  rv = folder->GetPrettiestName(locationString);
  NS_ENSURE_SUCCESS(rv,rv);

  return dbToUse->CreateCollationKey(locationString, result, len);
}

nsresult nsMsgDBView::SaveSortInfo(nsMsgViewSortTypeValue sortType, nsMsgViewSortOrderValue sortOrder)
{
  if (m_viewFolder)
  {
    nsCOMPtr <nsIDBFolderInfo> folderInfo;
    nsCOMPtr <nsIMsgDatabase> db;
    nsresult rv = m_viewFolder->GetDBFolderInfoAndDB(getter_AddRefs(folderInfo), getter_AddRefs(db));
    if (NS_SUCCEEDED(rv) && folderInfo)
    {
      // save off sort type and order, view type and flags
      folderInfo->SetSortType(sortType);
      folderInfo->SetSortOrder(sortOrder);
      
      nsString sortColumnsString;
      rv = EncodeColumnSort(sortColumnsString);
      NS_ENSURE_SUCCESS(rv, rv);
      folderInfo->SetProperty("sortColumns", sortColumnsString);
    }
  }
  return NS_OK;
}

PRInt32  nsMsgDBView::SecondarySort(nsMsgKey key1, nsISupports *supports1, nsMsgKey key2, nsISupports *supports2, viewSortInfo *comparisonContext)
{
  
  // we need to make sure that in the case of the secondary sort field also matching,
  // we don't recurse 
  if (comparisonContext->isSecondarySort)
    return key1 > key2;
  
  nsCOMPtr<nsIMsgFolder> folder1, folder2;
  nsCOMPtr <nsIMsgDBHdr> hdr1, hdr2;
  folder1 = do_QueryInterface(supports1);
  folder2 = do_QueryInterface(supports2);
  nsresult rv = folder1->GetMessageHeader(key1, getter_AddRefs(hdr1));
  NS_ENSURE_SUCCESS(rv, 0);
  rv = folder2->GetMessageHeader(key2, getter_AddRefs(hdr2));
  NS_ENSURE_SUCCESS(rv, 0);
  IdKeyPtr EntryInfo1, EntryInfo2;
  EntryInfo1.key = nsnull;
  EntryInfo2.key = nsnull;

  PRUint16  maxLen;
  eFieldType fieldType;
  nsMsgViewSortTypeValue sortType = comparisonContext->view->m_secondarySort;
  nsMsgViewSortOrderValue sortOrder = comparisonContext->view->m_secondarySortOrder;
  rv = GetFieldTypeAndLenForSort(sortType, &maxLen, &fieldType);
  const void *pValue1 = &EntryInfo1, *pValue2 = &EntryInfo2;
  
  int (* PR_CALLBACK comparisonFun) (const void *pItem1, const void *pItem2, void *privateData) = nsnull;
  int retStatus = 0;
  hdr1->GetMessageKey(&EntryInfo1.id);
  hdr2->GetMessageKey(&EntryInfo2.id);
  
  //check if a custom column handler exists. If it does then grab it and pass it in
  //to either GetCollationKey or GetLongField - we need the custom column handler for
  // the previous sort, if any.
  nsIMsgCustomColumnHandler* colHandler = nsnull; // GetCurColumnHandlerFromDBInfo();
  if (sortType == nsMsgViewSortType::byCustom)
    colHandler = comparisonContext->view->m_sortColumns[1].mColHandler;

  
  switch (fieldType)
  {
    case kCollationKey:
      rv = GetCollationKey(hdr1, sortType, &EntryInfo1.key, &EntryInfo1.dword, colHandler);
      NS_ASSERTION(NS_SUCCEEDED(rv),"failed to create collation key");
      comparisonFun = FnSortIdKeyPtr;
      break;
    case kU32:
      if (sortType == nsMsgViewSortType::byId)
        EntryInfo1.dword = EntryInfo1.id;
      else
        GetLongField(hdr1, sortType, &EntryInfo1.dword, colHandler);
      comparisonFun = FnSortIdDWord;
      break;
    default:
      return 0;
  }
  PRBool saveAscendingSort = comparisonContext->ascendingSort;
  comparisonContext->isSecondarySort = PR_TRUE;
  comparisonContext->ascendingSort = (sortOrder == nsMsgViewSortOrder::ascending);
  if (fieldType == kCollationKey)
  {
    PR_FREEIF(EntryInfo2.key);
    rv = GetCollationKey(hdr2, sortType, &EntryInfo2.key, &EntryInfo2.dword, colHandler);
    NS_ASSERTION(NS_SUCCEEDED(rv),"failed to create collation key");
  }
  else if (fieldType == kU32)
  {
    if (sortType == nsMsgViewSortType::byId)
      EntryInfo2.dword = EntryInfo2.id;
    else 
      GetLongField(hdr2, sortType, &EntryInfo2.dword, colHandler);
  }
  retStatus = (*comparisonFun)(&pValue1, &pValue2, comparisonContext);
  
  comparisonContext->isSecondarySort = PR_FALSE;
  comparisonContext->ascendingSort = saveAscendingSort;

  return retStatus;
}


NS_IMETHODIMP nsMsgDBView::Sort(nsMsgViewSortTypeValue sortType, nsMsgViewSortOrderValue sortOrder)
{
  nsresult rv;
  // if we're doing a stable sort, we can't just reverse the messages.
  // And the custom column we're sorting on might have changed, so to be
  // on the safe side, resort.
  if (m_sortType == sortType && m_sortValid && sortType != nsMsgViewSortType::byCustom
    && m_sortColumns.Length() < 2)
  {
    // same as it ever was.  do nothing
    if (m_sortOrder == sortOrder)
      return NS_OK;
    
    // for secondary sort, remember the sort order on a per column basis.
    if (m_sortColumns.Length())
      m_sortColumns[0].mSortOrder = sortOrder;
    SaveSortInfo(sortType, sortOrder);
    if (m_viewFlags & nsMsgViewFlagsType::kThreadedDisplay)
      ReverseThreads();
    else
      ReverseSort();

    m_sortOrder = sortOrder;
    // we just reversed the sort order...we still need to invalidate the view
    return NS_OK;
  }

  if (sortType == nsMsgViewSortType::byThread)
    return NS_OK;

  if (m_sortType != sortType)
  {
    MsgViewSortColumnInfo sortColumnInfo;
    sortColumnInfo.mSortType = sortType;
    sortColumnInfo.mSortOrder = sortOrder;
    if (sortType == nsMsgViewSortType::byCustom)
    {
      GetCurCustomColumn(sortColumnInfo.mCustomColumnName);
      sortColumnInfo.mColHandler = GetCurColumnHandlerFromDBInfo();
    }

    PushSort(sortColumnInfo);
  }
  
  if (m_sortColumns.Length() > 1)
  {
    m_secondarySort = m_sortColumns[1].mSortType;
    m_secondarySortOrder = m_sortColumns[1].mSortOrder;
  }
  SaveSortInfo(sortType, sortOrder);
  // figure out how much memory we'll need, and the malloc it
  PRUint16 maxLen;
  eFieldType fieldType;

  rv = GetFieldTypeAndLenForSort(sortType, &maxLen, &fieldType);
  NS_ENSURE_SUCCESS(rv,rv);

  nsVoidArray ptrs;
  PRUint32 arraySize = GetSize();

  if (!arraySize)
    return NS_OK;

  nsCOMPtr <nsISupportsArray> folders;
  GetFolders(getter_AddRefs(folders));

  IdKey** pPtrBase = (IdKey**)PR_Malloc(arraySize * sizeof(IdKey*));
  NS_ASSERTION(pPtrBase, "out of memory, can't sort");
  if (!pPtrBase) return NS_ERROR_OUT_OF_MEMORY;
  ptrs.AppendElement((void *)pPtrBase); // remember this pointer so we can free it later

  // build up the beast, so we can sort it.
  PRUint32 numSoFar = 0;
  const PRUint32 keyOffset = offsetof(IdKey, key);
  // calc max possible size needed for all the rest
  PRUint32 maxSize = (keyOffset + maxLen) * (arraySize - numSoFar);

  const PRUint32 maxBlockSize = (PRUint32) 0xf000L;
  PRUint32 allocSize = PR_MIN(maxBlockSize, maxSize);
  char *pTemp = (char *) PR_Malloc(allocSize);
  NS_ASSERTION(pTemp, "out of memory, can't sort");
  if (!pTemp)
  {
    FreeAll(&ptrs);
    return NS_ERROR_OUT_OF_MEMORY;
  }

  ptrs.AppendElement(pTemp); // remember this pointer so we can free it later

  char *pBase = pTemp;
  PRBool more = PR_TRUE;

  nsCOMPtr <nsIMsgDBHdr> msgHdr;
  PRUint8 *keyValue = nsnull;
  PRUint32 longValue;
  while (more && numSoFar < arraySize)
  {
    nsMsgKey thisKey = m_keys[numSoFar];
    if (sortType != nsMsgViewSortType::byId)
    {
      rv = GetMsgHdrForViewIndex(numSoFar, getter_AddRefs(msgHdr));
      NS_ASSERTION(NS_SUCCEEDED(rv) && msgHdr, "header not found");
      if (NS_FAILED(rv) || !msgHdr)
      {
        FreeAll(&ptrs);
        return NS_ERROR_UNEXPECTED;
      }
    }
    else
    {
      msgHdr = nsnull;
    }

    //check if a custom column handler exists. If it does then grab it and pass it in
    //to either GetCollationKey or GetLongField
    nsIMsgCustomColumnHandler* colHandler = GetCurColumnHandlerFromDBInfo();

    // could be a problem here if the ones that appear here are different than the ones already in the array
    PRUint32 actualFieldLen = 0;
    if (fieldType == kCollationKey)
    {
      rv = GetCollationKey(msgHdr, sortType, &keyValue, &actualFieldLen, colHandler);
      NS_ENSURE_SUCCESS(rv,rv);

      longValue = actualFieldLen;
    }
    else
    {
      if (sortType == nsMsgViewSortType::byId)
      {
        longValue = thisKey;
      }
      else
      {
        rv = GetLongField(msgHdr, sortType, &longValue, colHandler);
        NS_ENSURE_SUCCESS(rv,rv);
      }
    }

    // check to see if this entry fits into the block we have allocated so far
    // pTemp - pBase = the space we have used so far
    // sizeof(EntryInfo) + fieldLen = space we need for this entry
    // allocSize = size of the current block
    if ((PRUint32)(pTemp - pBase) + (keyOffset + actualFieldLen) >= allocSize)
    {
      maxSize = (keyOffset + maxLen) * (arraySize - numSoFar);
      allocSize = PR_MIN(maxBlockSize, maxSize);
      // make sure allocSize is big enough for the current value
      allocSize = PR_MAX(allocSize, keyOffset + actualFieldLen);
      pTemp = (char *) PR_Malloc(allocSize);
      NS_ASSERTION(pTemp, "out of memory, can't sort");
      if (!pTemp)
      {
        FreeAll(&ptrs);
        return NS_ERROR_OUT_OF_MEMORY;
      }
      pBase = pTemp;
      ptrs.AppendElement(pTemp); // remember this pointer so we can free it later
    }

    // now store this entry away in the allocated memory
    IdKey *info = (IdKey*)pTemp;
    pPtrBase[numSoFar] = info;
    info->id = thisKey;
    info->bits = m_flags[numSoFar];
    info->dword = longValue;
    //info->pad = 0;

    if (folders)
    {
      nsCOMPtr<nsISupports> curFolder;
      folders->GetElementAt(numSoFar, getter_AddRefs(curFolder));
      info->folder = curFolder;
    }
    else
      info->folder = m_folder;

    memcpy(info->key, keyValue, actualFieldLen);
    //In order to align memory for systems that require it, such as HP-UX
    //calculate the correct value to pad the actualFieldLen value
    const PRUint32 align = sizeof(IdKey) - sizeof(IdDWord) - 1;
    actualFieldLen = (actualFieldLen + align) & ~align;

    pTemp += keyOffset + actualFieldLen;
    ++numSoFar;
    PR_Free(keyValue);
  }

  viewSortInfo qsPrivateData;
  qsPrivateData.view = this;
  qsPrivateData.isSecondarySort = PR_FALSE;
  qsPrivateData.ascendingSort = (sortOrder == nsMsgViewSortOrder::ascending);

  nsCOMPtr <nsIMsgDatabase> dbToUse = m_db;

  if (!dbToUse) // probably search view
    GetDBForViewIndex(0, getter_AddRefs(dbToUse));
  qsPrivateData.db = dbToUse;
  if (dbToUse)
  {
    // do the sort
    switch (fieldType)
    {
      case kCollationKey:
        NS_QuickSort(pPtrBase, numSoFar, sizeof(IdKey*), FnSortIdKey, &qsPrivateData);
        break;
      case kU32:
        NS_QuickSort(pPtrBase, numSoFar, sizeof(IdKey*), FnSortIdDWord, &qsPrivateData);
        break;
      default:
        NS_ASSERTION(0, "not supposed to get here");
        break;
    }
  }

  // now put the IDs into the array in proper order
  for (PRUint32 i = 0; i < numSoFar; i++)
  {
    m_keys[i] = pPtrBase[i]->id;
    m_flags[i] = pPtrBase[i]->bits;

    if (folders)
      folders->SetElementAt(i, pPtrBase[i]->folder);
  }

  m_sortType = sortType;
  m_sortOrder = sortOrder;

  // free all the memory we allocated
  FreeAll(&ptrs);

  m_sortValid = PR_TRUE;
  //m_db->SetSortInfo(sortType, sortOrder);

  return NS_OK;
}

void nsMsgDBView::FreeAll(nsVoidArray *ptrs)
{
  PRInt32 i;
  PRInt32 count = (PRInt32) ptrs->Count();
  if (count == 0)
    return;

  for (i=(count - 1);i>=0;i--)
    PR_Free((void *) ptrs->ElementAt(i));
  ptrs->Clear();
}

nsMsgViewIndex nsMsgDBView::GetIndexOfFirstDisplayedKeyInThread(nsIMsgThread *threadHdr)
{
  nsMsgViewIndex  retIndex = nsMsgViewIndex_None;
  PRUint32        childIndex = 0;
  // We could speed up the unreadOnly view by starting our search with the first
  // unread message in the thread. Sometimes, that will be wrong, however, so
  // let's skip it until we're sure it's necessary.
  //  (m_viewFlags & nsMsgViewFlagsType::kUnreadOnly)
  //    ? threadHdr->GetFirstUnreadKey(m_db) : threadHdr->GetChildAt(0);
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
  if (! (m_viewFlags & nsMsgViewFlagsType::kThreadedDisplay))
    return nsMsgViewIndex_None;
  nsCOMPtr <nsIMsgThread> threadHdr;
  nsCOMPtr <nsIMsgDBHdr> msgHdr;
  nsresult rv = m_db->GetMsgHdrForKey(msgKey, getter_AddRefs(msgHdr));
  NS_ENSURE_SUCCESS(rv, nsMsgViewIndex_None);
  rv = GetThreadContainingMsgHdr(msgHdr, getter_AddRefs(threadHdr));
  NS_ENSURE_SUCCESS(rv, nsMsgViewIndex_None);

  nsMsgViewIndex retIndex = nsMsgViewIndex_None;

  if (threadHdr != nsnull)
  {
    if (msgIndex == nsMsgViewIndex_None)
      msgIndex = FindViewIndex(msgKey);

    if (msgIndex == nsMsgViewIndex_None)  // key is not in view, need to find by thread
    {
      msgIndex = GetIndexOfFirstDisplayedKeyInThread(threadHdr);
      //nsMsgKey    threadKey = (msgIndex == nsMsgViewIndex_None) ? nsMsgKey_None : GetAt(msgIndex);
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
      while (threadIndex < m_levels.Length() && m_levels[threadIndex] != 0);
      *pThreadCount = numChildren;
    }
  }
  return retIndex;
}

nsMsgKey nsMsgDBView::GetKeyOfFirstMsgInThread(nsMsgKey key)
{
  // Just report no key for any failure. This can occur when a
  // message is deleted from a threaded view
  nsCOMPtr <nsIMsgThread> pThread;
  nsCOMPtr <nsIMsgDBHdr> msgHdr;
  nsresult rv = m_db->GetMsgHdrForKey(key, getter_AddRefs(msgHdr));
  if (NS_FAILED(rv))
    return nsMsgKey_None;
  rv = GetThreadContainingMsgHdr(msgHdr, getter_AddRefs(pThread));
  if (NS_FAILED(rv))
    return nsMsgKey_None;
  nsMsgKey  firstKeyInThread = nsMsgKey_None;

  if (!pThread)
    return firstKeyInThread;

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

nsMsgViewIndex nsMsgDBView::FindHdr(nsIMsgDBHdr *msgHdr)
{
  nsMsgKey msgKey;
  msgHdr->GetMessageKey(&msgKey);
  return FindViewIndex(msgKey);
}

nsMsgViewIndex  nsMsgDBView::FindKey(nsMsgKey key, PRBool expand)
{
  nsMsgViewIndex retIndex = nsMsgViewIndex_None;
  retIndex = (nsMsgViewIndex) (m_keys.IndexOf(key));
  // for dummy headers, try to expand if the caller says so. And if the thread is
  // expanded, ignore the dummy header and return the real header index.
  if (retIndex != nsMsgViewIndex_None && m_flags[retIndex] & MSG_VIEW_FLAG_DUMMY &&  !(m_flags[retIndex] & MSG_FLAG_ELIDED))
    return (nsMsgViewIndex) m_keys.IndexOf(key, retIndex + 1);
  if (key != nsMsgKey_None && (retIndex == nsMsgViewIndex_None || m_flags[retIndex] & MSG_VIEW_FLAG_DUMMY)
    && expand && m_db)
  {
    nsMsgKey threadKey = GetKeyOfFirstMsgInThread(key);
    if (threadKey != nsMsgKey_None)
    {
      nsMsgViewIndex threadIndex = FindKey(threadKey, PR_FALSE);
      if (threadIndex != nsMsgViewIndex_None)
      {
        PRUint32 flags = m_flags[threadIndex];
        if ((flags & MSG_FLAG_ELIDED) && NS_SUCCEEDED(ExpandByIndex(threadIndex, nsnull))
          || (flags & MSG_VIEW_FLAG_DUMMY))
          retIndex = (nsMsgViewIndex) m_keys.IndexOf(key, threadIndex + 1);
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
  rv = GetThreadContainingMsgHdr(msgHdr, getter_AddRefs(pThread));
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
  while (threadIndex < m_levels.Length() && m_levels[threadIndex] != 0);

  return numInThread;
}

// returns the number of lines that would be added (> 0) or removed (< 0)
// if we were to try to expand/collapse the passed index.
nsresult nsMsgDBView::ExpansionDelta(nsMsgViewIndex index, PRInt32 *expansionDelta)
{
  PRUint32 numChildren;
  nsresult  rv;

  *expansionDelta = 0;
  if (index >= ((nsMsgViewIndex) m_keys.Length()))
    return NS_MSG_MESSAGE_NOT_FOUND;
  char  flags = m_flags[index];

  if (!(m_viewFlags & nsMsgViewFlagsType::kThreadedDisplay))
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
  PRInt32  flags = m_flags[threadIndex];

  // if not a thread, or doesn't have children, no expand/collapse
  // If we add sub-thread expand collapse, this will need to be relaxed
  if (!(flags & MSG_VIEW_FLAG_ISTHREAD) || !(flags & MSG_VIEW_FLAG_HASCHILDREN))
    return NS_MSG_MESSAGE_NOT_FOUND;
  if (flags & MSG_FLAG_ELIDED)
    return ExpandByIndex(threadIndex, numChanged);
  else
    return CollapseByIndex(threadIndex, numChanged);

}

nsresult nsMsgDBView::ExpandAndSelectThread()
{
    nsresult rv;

    NS_ASSERTION(mTreeSelection, "no tree selection");
    if (!mTreeSelection) return NS_ERROR_UNEXPECTED;

    PRInt32 index;
    rv = mTreeSelection->GetCurrentIndex(&index);
    NS_ENSURE_SUCCESS(rv,rv);

    rv = ExpandAndSelectThreadByIndex(index, PR_FALSE);
    NS_ENSURE_SUCCESS(rv,rv);
    return NS_OK;
}

nsresult nsMsgDBView::ExpandAndSelectThreadByIndex(nsMsgViewIndex index, PRBool augment)
{
  nsresult rv;

  nsMsgViewIndex threadIndex;
  PRBool inThreadedMode = (m_viewFlags & nsMsgViewFlagsType::kThreadedDisplay);

  if (inThreadedMode)
  {
    threadIndex = ThreadIndexOfMsg(GetAt(index), index);
    if (threadIndex == nsMsgViewIndex_None)
    {
      NS_ASSERTION(PR_FALSE, "couldn't find thread");
      return NS_MSG_MESSAGE_NOT_FOUND;
    }
  }
  else
  {
    threadIndex = index;
  }

  PRInt32 flags = m_flags[threadIndex];
  PRInt32 count = 0;

  if (inThreadedMode && (flags & MSG_VIEW_FLAG_ISTHREAD) && (flags & MSG_VIEW_FLAG_HASCHILDREN))
  {
    // if closed, expand this thread.
    if (flags & MSG_FLAG_ELIDED)
    {
      PRUint32 numExpanded;
      rv = ExpandByIndex(threadIndex, &numExpanded);
      NS_ENSURE_SUCCESS(rv,rv);
    }

    // get the number of messages in the expanded thread
    // so we know how many to select
    count = CountExpandedThread(threadIndex);
  }
  else
  {
    count = 1;
  }
  NS_ASSERTION(count > 0, "bad count");

  // update the selection

  NS_ASSERTION(mTreeSelection, "no tree selection");
  if (!mTreeSelection) return NS_ERROR_UNEXPECTED;

  // the count should be 1 or greater. if there was only one message in the thread, we just select it.
  // if more, we select all of them.
  mTreeSelection->RangedSelect(threadIndex + count - 1, threadIndex, augment);
  return NS_OK;
}

nsresult nsMsgDBView::ExpandAll()
{
  if (mTree)
    mTree->BeginUpdateBatch();
  for (PRInt32 i = GetSize() - 1; i >= 0; i--)
  {
    PRUint32 numExpanded;
    PRUint32 flags = m_flags[i];
    if (flags & MSG_FLAG_ELIDED)
      ExpandByIndex(i, &numExpanded);
  }
  if (mTree)
    mTree->EndUpdateBatch();
  return NS_OK;
}

nsresult nsMsgDBView::GetThreadContainingMsgHdr(nsIMsgDBHdr *msgHdr, nsIMsgThread **pThread)
{
  return m_db->GetThreadContainingMsgHdr(msgHdr, pThread);
}

nsresult nsMsgDBView::ExpandByIndex(nsMsgViewIndex index, PRUint32 *pNumExpanded)
{
  PRUint32      flags = m_flags[index];
  nsMsgKey    firstIdInThread;
  //nsMsgKey        startMsg = nsMsgKey_None;
  nsresult    rv = NS_OK;
  PRUint32      numExpanded = 0;

  NS_ASSERTION(flags & MSG_FLAG_ELIDED, "can't expand an already expanded thread");
  flags &= ~MSG_FLAG_ELIDED;

  if ((PRUint32) index > m_keys.Length())
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
  rv = GetThreadContainingMsgHdr(msgHdr, getter_AddRefs(pThread));
  NS_ENSURE_SUCCESS(rv, rv);
  m_flags[index] = flags;
  NoteChange(index, 1, nsMsgViewNotificationCode::changed);
  if (m_viewFlags & nsMsgViewFlagsType::kUnreadOnly)
  {
    if (flags & MSG_FLAG_READ)
      m_levels.AppendElement(0);  // keep top level hdr in thread, even though read.
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
  nsMsgKey    firstIdInThread;
  nsresult  rv;
  PRInt32  flags = m_flags[index];
  PRInt32  threadCount = 0;

  if (flags & MSG_FLAG_ELIDED || !(m_viewFlags & nsMsgViewFlagsType::kThreadedDisplay) || !(flags & MSG_VIEW_FLAG_HASCHILDREN))
    return NS_OK;
  flags  |= MSG_FLAG_ELIDED;

  if (index > m_keys.Length())
    return NS_MSG_MESSAGE_NOT_FOUND;

  firstIdInThread = m_keys[index];
  nsCOMPtr <nsIMsgDBHdr> msgHdr;
  rv = m_db->GetMsgHdrForKey(firstIdInThread, getter_AddRefs(msgHdr));
  if (NS_FAILED(rv) || msgHdr == nsnull)
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
    m_keys.RemoveElementsAt(index + 1, threadCount);
    m_flags.RemoveElementsAt(index + 1, threadCount);
    m_levels.RemoveElementsAt(index + 1, threadCount);
    if (pNumCollapsed != nsnull)
      *pNumCollapsed = numRemoved;
    NoteEndChange(index + 1, -numRemoved, nsMsgViewNotificationCode::insertOrDelete);
  }
  return rv;
}

nsresult nsMsgDBView::OnNewHeader(nsIMsgDBHdr *newHdr, nsMsgKey aParentKey, PRBool /*ensureListed*/)
{
    nsresult rv = NS_OK;
    // views can override this behaviour, which is to append to view.
    // This is the mail behaviour, but threaded views will want
    // to insert in order...
    if (newHdr)
  rv = AddHdr(newHdr);
    return rv;
}

nsresult nsMsgDBView::GetThreadContainingIndex(nsMsgViewIndex index, nsIMsgThread **resultThread)
{
  nsCOMPtr <nsIMsgDBHdr> msgHdr;

  NS_ENSURE_TRUE(m_db, NS_ERROR_NULL_POINTER);

  nsresult rv = m_db->GetMsgHdrForKey(m_keys[index], getter_AddRefs(msgHdr));
  NS_ENSURE_SUCCESS(rv, rv);
  return GetThreadContainingMsgHdr(msgHdr, resultThread);
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
      if (m_levels[i] == 0)
      {
        if (insertKey < m_keys[i])
          prevInsertIndex = i;
        else if (insertKey >= m_keys[i])
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
      if (!m_levels[i])
      {
        if (insertKey > m_keys[i])
        {
          retIndex = i;
          break;
        }
      }
    }
  }
  return retIndex;
}

nsMsgViewIndex nsMsgDBView::GetInsertIndexHelper(nsIMsgDBHdr *msgHdr, nsTArray<nsMsgKey> &keys,
                                                 nsMsgViewSortOrderValue sortOrder, nsMsgViewSortTypeValue sortType)
{
  nsMsgViewIndex highIndex = keys.Length();
  nsMsgViewIndex lowIndex = 0;
  IdKeyPtr EntryInfo1, EntryInfo2;
  EntryInfo1.key = nsnull;
  EntryInfo2.key = nsnull;

  nsresult rv;
  PRUint16  maxLen;
  eFieldType fieldType;
  rv = GetFieldTypeAndLenForSort(sortType, &maxLen, &fieldType);
  const void *pValue1 = &EntryInfo1, *pValue2 = &EntryInfo2;

  EntryInfo1.folder = m_folder;
  nsCOMPtr <nsISupportsArray> folders;
  GetFolders(getter_AddRefs(folders));
  
  int (* PR_CALLBACK comparisonFun) (const void *pItem1, const void *pItem2, void *privateData) = nsnull;
  int retStatus = 0;
  msgHdr->GetMessageKey(&EntryInfo1.id);

  //check if a custom column handler exists. If it does then grab it and pass it in
  //to either GetCollationKey or GetLongField
  nsIMsgCustomColumnHandler* colHandler = GetCurColumnHandlerFromDBInfo();

  viewSortInfo comparisonContext;
  comparisonContext.view = this;
  comparisonContext.isSecondarySort = PR_FALSE;
  comparisonContext.ascendingSort = (sortOrder == nsMsgViewSortOrder::ascending);
  comparisonContext.db = m_db.get();
  switch (fieldType)
  {
    case kCollationKey:
      rv = GetCollationKey(msgHdr, sortType, &EntryInfo1.key, &EntryInfo1.dword, colHandler);
      NS_ASSERTION(NS_SUCCEEDED(rv),"failed to create collation key");
      comparisonFun = FnSortIdKeyPtr;
      break;
    case kU32:
      if (sortType == nsMsgViewSortType::byId)
        EntryInfo1.dword = EntryInfo1.id;
      else
        GetLongField(msgHdr, sortType, &EntryInfo1.dword, colHandler);
      comparisonFun = FnSortIdDWord;
      break;
    default:
      return highIndex;
  }
  while (highIndex > lowIndex)
  {
    nsMsgViewIndex tryIndex = (lowIndex + highIndex - 1) / 2;
    EntryInfo2.id = keys[tryIndex];
    if (folders)
    {
      nsCOMPtr<nsISupports> curFolder;
      folders->GetElementAt(tryIndex, getter_AddRefs(curFolder));
       EntryInfo2.folder = curFolder;
    }
    else
       EntryInfo2.folder = m_folder;
    
    nsCOMPtr <nsIMsgDBHdr> tryHdr;
    nsCOMPtr <nsIMsgDatabase> db;
    GetDBForViewIndex(tryIndex, getter_AddRefs(db));
    if (db)
      rv = db->GetMsgHdrForKey(EntryInfo2.id, getter_AddRefs(tryHdr));
    if (!tryHdr)
      break;
    if (fieldType == kCollationKey)
    {
      PR_FREEIF(EntryInfo2.key);
      rv = GetCollationKey(tryHdr, sortType, &EntryInfo2.key, &EntryInfo2.dword, colHandler);
      NS_ASSERTION(NS_SUCCEEDED(rv),"failed to create collation key");
    }
    else if (fieldType == kU32)
    {
      if (sortType == nsMsgViewSortType::byId) {
        EntryInfo2.dword = EntryInfo2.id;
      }
      else {
        GetLongField(tryHdr, sortType, &EntryInfo2.dword, colHandler);
      }
    }
    retStatus = (*comparisonFun)(&pValue1, &pValue2, &comparisonContext);
    if (retStatus == 0)
    {
      highIndex = tryIndex;
      break;
    }

    if (retStatus < 0)
    {
      highIndex = tryIndex;
    }
    else
    {
      lowIndex = tryIndex + 1;
    }
  }

  PR_Free(EntryInfo1.key);
  PR_Free(EntryInfo2.key);
  return highIndex;
}

nsMsgViewIndex nsMsgDBView::GetInsertIndex(nsIMsgDBHdr *msgHdr)
{
  if (!GetSize())
    return 0;

  if ((m_viewFlags & nsMsgViewFlagsType::kThreadedDisplay) != 0
        && !(m_viewFlags & nsMsgViewFlagsType::kGroupBySort)
        && m_sortOrder != nsMsgViewSortType::byId)
    return GetIndexForThread(msgHdr);

  return GetInsertIndexHelper(msgHdr, m_keys, m_sortOrder, m_sortType);
}

nsresult  nsMsgDBView::AddHdr(nsIMsgDBHdr *msgHdr)
{
  PRUint32  flags = 0;
#ifdef DEBUG_bienvenu
  NS_ASSERTION(m_keys.Length() == m_flags.Length() && (int) m_keys.Length() == m_levels.Length(), "view arrays out of sync!");
#endif

  if (!GetShowingIgnored())
  {
    nsCOMPtr <nsIMsgThread> thread;
    m_db->GetThreadContainingMsgHdr(msgHdr, getter_AddRefs(thread));
    if (thread)
    {
      thread->GetFlags(&flags);
      if (flags & MSG_FLAG_IGNORED)
        return NS_OK;
    }

    PRBool ignored;
    msgHdr->GetIsKilled(&ignored);
    if (ignored)
       return NS_OK;
  }

  nsMsgKey msgKey, threadId;
  nsMsgKey threadParent;
  msgHdr->GetMessageKey(&msgKey);
  msgHdr->GetThreadId(&threadId);
  msgHdr->GetThreadParent(&threadParent);

  msgHdr->GetFlags(&flags);
  // ### this isn't quite right, is it? Should be checking that our thread parent key is none?
  if (threadParent == nsMsgKey_None)
    flags |= MSG_VIEW_FLAG_ISTHREAD;
  nsMsgViewIndex insertIndex = GetInsertIndex(msgHdr);
  if (insertIndex == nsMsgViewIndex_None)
  {
    // if unreadonly, level is 0 because we must be the only msg in the thread.
    PRInt32 levelToAdd = 0;

    if (m_sortOrder == nsMsgViewSortOrder::ascending)
    {
      m_keys.AppendElement(msgKey);
      m_flags.AppendElement(flags);
      m_levels.AppendElement(levelToAdd);

      // the call to NoteChange() has to happen after we add the key
      // as NoteChange() will call RowCountChanged() which will call our GetRowCount()
      NoteChange(GetSize() - 1, 1, nsMsgViewNotificationCode::insertOrDelete);
    }
    else
    {
      m_keys.InsertElementAt(0, msgKey);
      m_flags.InsertElementAt(0, flags);
      m_levels.InsertElementAt(0, levelToAdd);

      // the call to NoteChange() has to happen after we insert the key
      // as NoteChange() will call RowCountChanged() which will call our GetRowCount()
      NoteChange(0, 1, nsMsgViewNotificationCode::insertOrDelete);
    }
    m_sortValid = PR_FALSE;
  }
  else
  {
    m_keys.InsertElementAt(insertIndex, msgKey);
    m_flags.InsertElementAt(insertIndex, flags);
    PRInt32 level = 0;
    m_levels.InsertElementAt(insertIndex, level);

    // the call to NoteChange() has to happen after we add the key
    // as NoteChange() will call RowCountChanged() which will call our GetRowCount()
    NoteChange(insertIndex, 1, nsMsgViewNotificationCode::insertOrDelete);
  }
  OnHeaderAddedOrDeleted();
  return NS_OK;
}

PRBool nsMsgDBView::WantsThisThread(nsIMsgThread * /*threadHdr*/)
{
  return PR_TRUE; // default is to want all threads.
}

nsMsgViewIndex nsMsgDBView::FindParentInThread(nsMsgKey parentKey, nsMsgViewIndex startOfThreadViewIndex)
{
  nsCOMPtr<nsIMsgDBHdr> msgHdr;
  while (parentKey != nsMsgKey_None)
  {
    nsMsgViewIndex parentIndex = m_keys.IndexOf(parentKey, startOfThreadViewIndex);
    if (parentIndex != nsMsgViewIndex_None)
      return parentIndex;

    if (NS_FAILED(m_db->GetMsgHdrForKey(parentKey, getter_AddRefs(msgHdr))))
      break;

    msgHdr->GetThreadParent(&parentKey);
  }

  return startOfThreadViewIndex;
}

nsresult nsMsgDBView::ListIdsInThreadOrder(nsIMsgThread *threadHdr, nsMsgKey parentKey, PRInt32 level, nsMsgViewIndex *viewIndex, PRUint32 *pNumListed)
{
  nsresult rv = NS_OK;
  nsCOMPtr <nsISimpleEnumerator> msgEnumerator;
  threadHdr->EnumerateMessages(parentKey, getter_AddRefs(msgEnumerator));
  PRUint32 numChildren;
  (void) threadHdr->GetNumChildren(&numChildren);
  NS_ASSERTION(numChildren, "Empty thread in view/db");
  if (!numChildren)
    return NS_OK;  // bogus, but harmless.

  numChildren--; // account for the existing thread root

  // skip the first one.
  PRBool hasMore;
  nsCOMPtr <nsISupports> supports;
  nsCOMPtr <nsIMsgDBHdr> msgHdr;
  while (NS_SUCCEEDED(rv) && NS_SUCCEEDED(rv = msgEnumerator->HasMoreElements(&hasMore)) && hasMore)
  {
    rv = msgEnumerator->GetNext(getter_AddRefs(supports));
    if (NS_SUCCEEDED(rv) && supports)
    {
      if (*pNumListed == numChildren)
      {
        NS_NOTREACHED("thread corrupt in db");
        // if we've listed more messages than are in the thread, then the db
        // is corrupt, and we should invalidate it.
        // we'll use this rv to indicate there's something wrong with the db
        // though for now it probably won't get paid attention to.
        m_db->SetSummaryValid(PR_FALSE);
        rv = NS_MSG_ERROR_FOLDER_SUMMARY_OUT_OF_DATE;
        break;
      }

      msgHdr = do_QueryInterface(supports);
      if (!(m_viewFlags & nsMsgViewFlagsType::kShowIgnored))
      {
        PRBool ignored;
        msgHdr->GetIsKilled(&ignored);
        // We are not going to process subthreads, horribly invalidating the
        // numChildren characteristic
        if (ignored)
          continue;
      }

      nsMsgKey msgKey;
      PRUint32 msgFlags, newFlags;
      msgHdr->GetMessageKey(&msgKey);
      msgHdr->GetFlags(&msgFlags);
      AdjustReadFlag(msgHdr, &msgFlags);
      m_keys[*viewIndex] = msgKey;
      // ### TODO - how about hasChildren flag?
      m_flags[*viewIndex] = msgFlags & ~MSG_VIEW_FLAGS;
      // ### TODO this is going to be tricky - might use enumerators
      m_levels[*viewIndex] = level;
      // turn off thread or elided bit if they got turned on (maybe from new only view?)
      msgHdr->AndFlags(~(MSG_VIEW_FLAG_ISTHREAD | MSG_FLAG_ELIDED), &newFlags);
      (*pNumListed)++;
      (*viewIndex)++;
      rv = ListIdsInThreadOrder(threadHdr, msgKey, level + 1, viewIndex, pNumListed);
    }
  }
  return rv; // we don't want to return the rv from the enumerator when it reaches the end, do we?
}

nsresult nsMsgDBView::ListIdsInThread(nsIMsgThread *threadHdr, nsMsgViewIndex startOfThreadViewIndex, PRUint32 *pNumListed)
{
  NS_ENSURE_ARG(threadHdr);
  // these children ids should be in thread order.
  nsresult rv = NS_OK;
  PRUint32 i;
  nsMsgViewIndex viewIndex = startOfThreadViewIndex + 1;
  *pNumListed = 0;

  PRUint32 numChildren;
  threadHdr->GetNumChildren(&numChildren);
  NS_ASSERTION(numChildren, "Empty thread in view/db");
  if (!numChildren)
    return NS_OK;

  numChildren--; // account for the existing thread root
  m_keys.InsertElementsAt(viewIndex, numChildren, 0);
  m_flags.InsertElementsAt(viewIndex, numChildren, 0);
  m_levels.InsertElementsAt(viewIndex, numChildren, 1); // default unthreaded level

  // ### need to rework this when we implemented threading in group views.
  if (m_viewFlags & nsMsgViewFlagsType::kThreadedDisplay && ! (m_viewFlags & nsMsgViewFlagsType::kGroupBySort))
  {
    nsMsgKey parentKey = m_keys[startOfThreadViewIndex];
    rv = ListIdsInThreadOrder(threadHdr, parentKey, 1, &viewIndex, pNumListed);
  }
  else
  {
    PRUint32 ignoredHeaders = 0;
    // if we're not threaded, just list em out in db order
    for (i = 1; i <= numChildren; i++)
    {
      nsCOMPtr <nsIMsgDBHdr> msgHdr;
      threadHdr->GetChildHdrAt(i, getter_AddRefs(msgHdr));

      if (msgHdr != nsnull)
      {
        if (!(m_viewFlags & nsMsgViewFlagsType::kShowIgnored))
        {
          PRBool killed;
          msgHdr->GetIsKilled(&killed);
          if (killed)
          {
            ignoredHeaders++;
            continue;
          }
        }

        nsMsgKey msgKey;
        PRUint32 msgFlags, newFlags;
        msgHdr->GetMessageKey(&msgKey);
        msgHdr->GetFlags(&msgFlags);
        AdjustReadFlag(msgHdr, &msgFlags);
        m_keys[viewIndex] = msgKey;
        m_flags[viewIndex] = msgFlags & ~MSG_VIEW_FLAGS;
        // here, we're either flat, or we're grouped - in either case, level is 1
        // turn off thread or elided bit if they got turned on (maybe from new only view?)
        if (i > 0)
          msgHdr->AndFlags(~(MSG_VIEW_FLAG_ISTHREAD | MSG_FLAG_ELIDED), &newFlags);
        (*pNumListed)++;
        viewIndex++;
      }
    }
    if (ignoredHeaders + *pNumListed < numChildren)
    {
      NS_NOTREACHED("thread corrupt in db");
      // if we've listed fewer messages than are in the thread, then the db
      // is corrupt, and we should invalidate it.
      // we'll use this rv to indicate there's something wrong with the db
      // though for now it probably won't get paid attention to.
      m_db->SetSummaryValid(PR_FALSE);
      rv = NS_MSG_ERROR_FOLDER_SUMMARY_OUT_OF_DATE;
    }
  }

  // We may have added too many elements (i.e., subthreads were cut)
  if (*pNumListed < numChildren)
  {
    m_keys.RemoveElementsAt(viewIndex, numChildren - *pNumListed);
    m_flags.RemoveElementsAt(viewIndex, numChildren - *pNumListed);
    m_levels.RemoveElementsAt(viewIndex, numChildren - *pNumListed);
  }
  return rv;
}

PRInt32 nsMsgDBView::FindLevelInThread(nsIMsgDBHdr *msgHdr, nsMsgViewIndex startOfThread, nsMsgViewIndex viewIndex)
{
  nsCOMPtr <nsIMsgDBHdr> curMsgHdr = msgHdr;
  nsMsgKey msgKey;
  msgHdr->GetMessageKey(&msgKey);

  // look through the ancestors of the passed in msgHdr in turn, looking for them in the view, up to the start of
  // the thread. If we find an ancestor, then our level is one greater than the level of the ancestor.
  while (curMsgHdr)
  {
    nsMsgKey parentKey;
    curMsgHdr->GetThreadParent(&parentKey);
    if (parentKey == nsMsgKey_None)
      break;

    // scan up to find view index of ancestor, if any
    for (nsMsgViewIndex indexToTry = viewIndex; indexToTry && indexToTry-- >= startOfThread;)
    {
      if (m_keys[indexToTry] == parentKey)
        return m_levels[indexToTry] + 1;
    }

    // if msgHdr's key is its parentKey, we'll loop forever, so protect
    // against that corruption.
    if (msgKey == parentKey || NS_FAILED(m_db->GetMsgHdrForKey(parentKey, getter_AddRefs(curMsgHdr))))
    {
      NS_ERROR("msgKey == parentKey, or GetMsgHdrForKey failed, this used to be an infinte loop condition");
      curMsgHdr = nsnull;
    }
    else
    {
      // need to update msgKey so the check for a msgHdr with matching
      // key+parentKey will work after first time through loop
      curMsgHdr->GetMessageKey(&msgKey);
    }
  }
  return 1;
}

nsresult nsMsgDBView::ListUnreadIdsInThread(nsIMsgThread *threadHdr, nsMsgViewIndex startOfThreadViewIndex, PRUint32 *pNumListed)
{
  NS_ENSURE_ARG(threadHdr);
  // these children ids should be in thread order.
  nsMsgViewIndex viewIndex = startOfThreadViewIndex + 1;
  *pNumListed = 0;
  nsMsgKey topLevelMsgKey = m_keys[startOfThreadViewIndex];

  PRUint32 numChildren;
  threadHdr->GetNumChildren(&numChildren);
  PRUint32 i;
  for (i = 0; i < numChildren; i++)
  {
    nsCOMPtr <nsIMsgDBHdr> msgHdr;
    threadHdr->GetChildHdrAt(i, getter_AddRefs(msgHdr));
    if (msgHdr != nsnull)
    {
      if (!(m_viewFlags & nsMsgViewFlagsType::kShowIgnored))
      {
        PRBool killed;
        msgHdr->GetIsKilled(&killed);
        if (killed)
          continue;
      }

      nsMsgKey msgKey;
      PRUint32 msgFlags;
      msgHdr->GetMessageKey(&msgKey);
      msgHdr->GetFlags(&msgFlags);
      PRBool isRead = AdjustReadFlag(msgHdr, &msgFlags);
      // determining the level is going to be tricky, since we're not storing the
      // level in the db anymore. It will mean looking up the view for each of the
      // ancestors of the current msg until we find one in the view. I guess we could
      // check each ancestor to see if it's unread before looking for it in the view.
      if (!isRead)
      {
        PRUint8 levelToAdd;
        // just make sure flag is right in db.
        m_db->MarkHdrRead(msgHdr, PR_FALSE, nsnull);
        if (msgKey != topLevelMsgKey)
        {
          m_keys.InsertElementAt(viewIndex, msgKey);
          m_flags.InsertElementAt(viewIndex, msgFlags);
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

NS_IMETHODIMP nsMsgDBView::OnHdrFlagsChanged(nsIMsgDBHdr *aHdrChanged, PRUint32 aOldFlags,
                                       PRUint32 aNewFlags, nsIDBChangeListener *aInstigator)
{
  // if we're not the instigator, update flags if this key is in our view
  if (aInstigator != this)
  {
    NS_ENSURE_ARG_POINTER(aHdrChanged);
    nsMsgKey msgKey;
    aHdrChanged->GetMessageKey(&msgKey);
    nsMsgViewIndex index = FindViewIndex(msgKey);
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

    PRUint32 deltaFlags = (aOldFlags ^ aNewFlags);
    if (deltaFlags & (MSG_FLAG_READ | MSG_FLAG_NEW))
    {
      nsMsgViewIndex threadIndex = ThreadIndexOfMsg(msgKey);
      // may need to fix thread counts
      if (threadIndex != nsMsgViewIndex_None && threadIndex != index)
        NoteChange(threadIndex, 1, nsMsgViewNotificationCode::changed);
    }
 }
  // don't need to propagate notifications, right?
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::OnHdrDeleted(nsIMsgDBHdr *aHdrChanged, nsMsgKey aParentKey, PRInt32 aFlags,
                            nsIDBChangeListener *aInstigator)
{
  nsMsgViewIndex deletedIndex = FindHdr(aHdrChanged);
  if (deletedIndex != nsMsgViewIndex_None)
    RemoveByIndex(deletedIndex);

  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::OnHdrAdded(nsIMsgDBHdr *aHdrChanged, nsMsgKey aParentKey, PRInt32 aFlags,
                          nsIDBChangeListener *aInstigator)
{
  return OnNewHeader(aHdrChanged, aParentKey, PR_FALSE);
  // probably also want to pass that parent key in, since we went to the trouble
  // of figuring out what it is.
}

NS_IMETHODIMP
nsMsgDBView::OnHdrPropertyChanged(nsIMsgDBHdr *aHdrToChange, PRBool aPreChange, PRUint32 *aStatus, 
                                 nsIDBChangeListener * aInstigator)
{
  if (aPreChange)
    return NS_OK;

  if (aHdrToChange)
  {
    nsMsgViewIndex index = FindHdr(aHdrToChange);
    if (index != nsMsgViewIndex_None)
      NoteChange(index, 1, nsMsgViewNotificationCode::changed);
  }
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::OnParentChanged (nsMsgKey aKeyChanged, nsMsgKey oldParent, nsMsgKey newParent, nsIDBChangeListener *aInstigator)
{
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::OnAnnouncerGoingAway(nsIDBChangeAnnouncer *instigator)
{
  if (m_db)
  {
    m_db->RemoveListener(this);
    m_db = nsnull;
  }

  PRInt32 saveSize = GetSize();
  ClearHdrCache();

  // this is important, because the tree will ask us for our
  // row count, which get determine from the number of keys.
  m_keys.Clear();
  // be consistent
  m_flags.Clear();
  m_levels.Clear();

  // tell the tree all the rows have gone away
  if (mTree)
    mTree->RowCountChanged(0, -saveSize);

  return NS_OK;
}


NS_IMETHODIMP nsMsgDBView::OnReadChanged(nsIDBChangeListener *aInstigator)
{
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::OnJunkScoreChanged(nsIDBChangeListener *aInstigator)
{
  return NS_OK;
}

void nsMsgDBView::ClearHdrCache()
{
  m_cachedHdr = nsnull;
  m_cachedMsgKey = nsMsgKey_None;
}

void nsMsgDBView::EnableChangeUpdates()
{
  mSuppressChangeNotification = PR_FALSE;
}

void nsMsgDBView::DisableChangeUpdates()
{
  mSuppressChangeNotification = PR_TRUE;
}

void nsMsgDBView::NoteChange(nsMsgViewIndex firstLineChanged, PRInt32 numChanged,
                             nsMsgViewNotificationCodeValue changeType)
{
  if (mTree && !mSuppressChangeNotification)
  {
    switch (changeType)
    {
    case nsMsgViewNotificationCode::changed:
      mTree->InvalidateRange(firstLineChanged, firstLineChanged + numChanged - 1);
      break;
    case nsMsgViewNotificationCode::insertOrDelete:
      if (numChanged < 0)
        mRemovingRow = PR_TRUE;
      // the caller needs to have adjusted m_keys before getting here, since
      // RowCountChanged() will call our GetRowCount()
      mTree->RowCountChanged(firstLineChanged, numChanged);
      mRemovingRow = PR_FALSE;
    case nsMsgViewNotificationCode::all:
      ClearHdrCache();
      break;
    }
  }
}

void nsMsgDBView::NoteStartChange(nsMsgViewIndex firstlineChanged, PRInt32 numChanged,
                                  nsMsgViewNotificationCodeValue changeType)
{
}
void nsMsgDBView::NoteEndChange(nsMsgViewIndex firstlineChanged, PRInt32 numChanged,
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

NS_IMETHODIMP nsMsgDBView::SetSortType(nsMsgViewSortTypeValue aSortType)
{
    m_sortType = aSortType;
    return NS_OK;
}


NS_IMETHODIMP nsMsgDBView::GetViewType(nsMsgViewTypeValue *aViewType)
{
    NS_ASSERTION(0,"you should be overriding this\n");
    return NS_ERROR_UNEXPECTED;
}

nsresult nsMsgDBView::PersistFolderInfo(nsIDBFolderInfo **dbFolderInfo)
{
  nsresult rv = m_db->GetDBFolderInfo(dbFolderInfo);
  NS_ENSURE_SUCCESS(rv, rv);
  // save off sort type and order, view type and flags
  (*dbFolderInfo)->SetSortType(m_sortType);
  (*dbFolderInfo)->SetSortOrder(m_sortOrder);
  (*dbFolderInfo)->SetViewFlags(m_viewFlags);
  nsMsgViewTypeValue viewType;
  GetViewType(&viewType);
  (*dbFolderInfo)->SetViewType(viewType);
  return rv;
}


NS_IMETHODIMP nsMsgDBView::GetViewFlags(nsMsgViewFlagsTypeValue *aViewFlags)
{
    NS_ENSURE_ARG_POINTER(aViewFlags);
    *aViewFlags = m_viewFlags;
    return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::SetViewFlags(nsMsgViewFlagsTypeValue aViewFlags)
{
  // if we're turning off threaded display, we need to expand all so that all
  // messages will be displayed.
  if (m_viewFlags & nsMsgViewFlagsType::kThreadedDisplay && ! (aViewFlags & nsMsgViewFlagsType::kThreadedDisplay))
  {
    ExpandAll();
    m_sortValid = PR_FALSE; // invalidate the sort so sorting will do something
  }
  m_viewFlags = aViewFlags;

  if (m_viewFolder)
  {
    nsCOMPtr <nsIMsgDatabase> db;
    nsCOMPtr <nsIDBFolderInfo> folderInfo;
    nsresult rv = m_viewFolder->GetDBFolderInfoAndDB(getter_AddRefs(folderInfo), getter_AddRefs(db));
    NS_ENSURE_SUCCESS(rv,rv);
    return folderInfo->SetViewFlags(aViewFlags);
  }
  else
    return NS_OK;
}

nsresult nsMsgDBView::MarkThreadOfMsgRead(nsMsgKey msgId, nsMsgViewIndex msgIndex, nsTArray<nsMsgKey> &idsMarkedRead, PRBool bRead)
{
    nsCOMPtr <nsIMsgThread> threadHdr;
    nsresult rv = GetThreadContainingIndex(msgIndex, getter_AddRefs(threadHdr));
    NS_ENSURE_SUCCESS(rv, rv);

    nsMsgViewIndex threadIndex;

    NS_ASSERTION(threadHdr, "threadHdr is null");
    if (!threadHdr)
        return NS_MSG_MESSAGE_NOT_FOUND;

    nsCOMPtr <nsIMsgDBHdr> firstHdr;
    threadHdr->GetChildAt(0, getter_AddRefs(firstHdr));
    nsMsgKey firstHdrId;
    firstHdr->GetMessageKey(&firstHdrId);
    if (msgId != firstHdrId)
        threadIndex = GetIndexOfFirstDisplayedKeyInThread(threadHdr);
    else
        threadIndex = msgIndex;
    return MarkThreadRead(threadHdr, threadIndex, idsMarkedRead, bRead);
}

nsresult nsMsgDBView::MarkThreadRead(nsIMsgThread *threadHdr, nsMsgViewIndex threadIndex, nsTArray<nsMsgKey> &idsMarkedRead, PRBool bRead)
{
    PRBool threadElided = PR_TRUE;
    if (threadIndex != nsMsgViewIndex_None)
        threadElided = (m_flags[threadIndex] & MSG_FLAG_ELIDED);

    PRUint32 numChildren;
    threadHdr->GetNumChildren(&numChildren);
    idsMarkedRead.SetCapacity(numChildren);
    for (PRInt32 childIndex = 0; childIndex < (PRInt32) numChildren ; childIndex++)
    {
        nsCOMPtr <nsIMsgDBHdr> msgHdr;
        threadHdr->GetChildHdrAt(childIndex, getter_AddRefs(msgHdr));
        NS_ASSERTION(msgHdr, "msgHdr is null");
        if (!msgHdr)
            continue;

        PRBool isRead;

        nsMsgKey hdrMsgId;
        msgHdr->GetMessageKey(&hdrMsgId);
        m_db->IsRead(hdrMsgId, &isRead);

        if (isRead != bRead)
        {
            // MarkHdrRead will change the unread count on the thread
            m_db->MarkHdrRead(msgHdr, bRead, nsnull);
            // insert at the front.  should we insert at the end?
            idsMarkedRead.InsertElementAt(0, hdrMsgId);
        }
    }

    return NS_OK;
}

PRBool nsMsgDBView::AdjustReadFlag(nsIMsgDBHdr *msgHdr, PRUint32 *msgFlags)
{
  PRBool isRead = PR_FALSE;
  nsMsgKey msgKey;
  msgHdr->GetMessageKey(&msgKey);
  m_db->IsRead(msgKey, &isRead);
    // just make sure flag is right in db.
#ifdef DEBUG_bienvenu
  NS_ASSERTION(isRead == (*msgFlags & MSG_FLAG_READ != 0), "msgFlags out of sync");
#endif
  if (isRead)
    *msgFlags |= MSG_FLAG_READ;
  else
    *msgFlags &= ~MSG_FLAG_READ;
  m_db->MarkHdrRead(msgHdr, isRead, nsnull);
  return isRead;
}

// Starting from startIndex, performs the passed in navigation, including
// any marking read needed, and returns the resultId and resultIndex of the
// destination of the navigation.  If no message is found in the view,
// it returns a resultId of nsMsgKey_None and an resultIndex of nsMsgViewIndex_None.
NS_IMETHODIMP nsMsgDBView::ViewNavigate(nsMsgNavigationTypeValue motion, nsMsgKey *pResultKey, nsMsgViewIndex *pResultIndex, nsMsgViewIndex *pThreadIndex, PRBool wrap)
{
    NS_ENSURE_ARG_POINTER(pResultKey);
    NS_ENSURE_ARG_POINTER(pResultIndex);
    NS_ENSURE_ARG_POINTER(pThreadIndex);

    PRInt32 currentIndex;
    nsMsgViewIndex startIndex;

    if (!mTreeSelection) // we must be in stand alone message mode
    {
      currentIndex = FindViewIndex(m_currentlyDisplayedMsgKey);
    }
    else
    {
      nsresult rv = mTreeSelection->GetCurrentIndex(&currentIndex);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    startIndex = currentIndex;
    return nsMsgDBView::NavigateFromPos(motion, startIndex, pResultKey, pResultIndex, pThreadIndex, wrap);
}

nsresult nsMsgDBView::NavigateFromPos(nsMsgNavigationTypeValue motion, nsMsgViewIndex startIndex, nsMsgKey *pResultKey, nsMsgViewIndex *pResultIndex, nsMsgViewIndex *pThreadIndex, PRBool wrap)
{
    nsresult rv = NS_OK;
    nsMsgKey resultThreadKey;
    nsMsgViewIndex curIndex;
    nsMsgViewIndex lastIndex = (GetSize() > 0) ? (nsMsgViewIndex) GetSize() - 1 : nsMsgViewIndex_None;
    nsMsgViewIndex threadIndex = nsMsgViewIndex_None;

    // if there aren't any messages in the view, bail out.
    if (GetSize() <= 0)
    {
      *pResultIndex = nsMsgViewIndex_None;
      *pResultKey = nsMsgKey_None;
      return NS_OK;
    }

    switch (motion)
    {
        case nsMsgNavigationType::firstMessage:
            *pResultIndex = 0;
            *pResultKey = m_keys[0];
            break;
        case nsMsgNavigationType::nextMessage:
            // return same index and id on next on last message
            *pResultIndex = PR_MIN(startIndex + 1, lastIndex);
            *pResultKey = m_keys[*pResultIndex];
            break;
        case nsMsgNavigationType::previousMessage:
            *pResultIndex = (startIndex != nsMsgViewIndex_None && startIndex > 0) ? startIndex - 1 : 0;
            *pResultKey = m_keys[*pResultIndex];
            break;
        case nsMsgNavigationType::lastMessage:
            *pResultIndex = lastIndex;
            *pResultKey = m_keys[*pResultIndex];
            break;
        case nsMsgNavigationType::firstFlagged:
            rv = FindFirstFlagged(pResultIndex);
            if (IsValidIndex(*pResultIndex))
                *pResultKey = m_keys[*pResultIndex];
            break;
        case nsMsgNavigationType::nextFlagged:
            rv = FindNextFlagged(startIndex + 1, pResultIndex);
            if (IsValidIndex(*pResultIndex))
                *pResultKey = m_keys[*pResultIndex];
            break;
        case nsMsgNavigationType::previousFlagged:
            rv = FindPrevFlagged(startIndex, pResultIndex);
            if (IsValidIndex(*pResultIndex))
                *pResultKey = m_keys[*pResultIndex];
            break;
        case nsMsgNavigationType::firstNew:
            rv = FindFirstNew(pResultIndex);
            if (IsValidIndex(*pResultIndex))
                *pResultKey = m_keys[*pResultIndex];
            break;
        case nsMsgNavigationType::firstUnreadMessage:
            startIndex = nsMsgViewIndex_None;        // note fall thru - is this motion ever used?
        case nsMsgNavigationType::nextUnreadMessage:
            for (curIndex = (startIndex == nsMsgViewIndex_None) ? 0 : startIndex; curIndex <= lastIndex && lastIndex != nsMsgViewIndex_None; curIndex++) {
                PRUint32 flags = m_flags[curIndex];

                // don't return start index since navigate should move
                if (!(flags & (MSG_FLAG_READ | MSG_VIEW_FLAG_DUMMY)) && (curIndex != startIndex))
                {
                    *pResultIndex = curIndex;
                    *pResultKey = m_keys[*pResultIndex];
                    break;
                }
                // check for collapsed thread with new children
                if ((m_viewFlags & nsMsgViewFlagsType::kThreadedDisplay) && flags & MSG_VIEW_FLAG_ISTHREAD && flags & MSG_FLAG_ELIDED) {
                    nsCOMPtr <nsIMsgThread> threadHdr;
                    GetThreadContainingIndex(curIndex, getter_AddRefs(threadHdr));
                    NS_ENSURE_SUCCESS(rv, rv);

                    NS_ASSERTION(threadHdr, "threadHdr is null");
                    if (!threadHdr)
                        continue;
                    PRUint32 numUnreadChildren;
                    threadHdr->GetNumUnreadChildren(&numUnreadChildren);
                    if (numUnreadChildren > 0)
                    {
                        PRUint32 numExpanded;
                        ExpandByIndex(curIndex, &numExpanded);
                        lastIndex += numExpanded;
                        if (pThreadIndex)
                            *pThreadIndex = curIndex;
                    }
                }
            }
            if (curIndex > lastIndex)
            {
                // wrap around by starting at index 0.
                if (wrap)
                {
                    nsMsgKey startKey = GetAt(startIndex);

                    rv = NavigateFromPos(nsMsgNavigationType::nextUnreadMessage, nsMsgViewIndex_None, pResultKey, pResultIndex, pThreadIndex, PR_FALSE);

                    if (*pResultKey == startKey)
                    {
                        // wrapped around and found start message!
                        *pResultIndex = nsMsgViewIndex_None;
                        *pResultKey = nsMsgKey_None;
                    }
                }
                else
                {
                    *pResultIndex = nsMsgViewIndex_None;
                    *pResultKey = nsMsgKey_None;
                }
            }
            break;
        case nsMsgNavigationType::previousUnreadMessage:
            if (startIndex == nsMsgViewIndex_None)
              break;
            rv = FindPrevUnread(m_keys[startIndex], pResultKey,
                                &resultThreadKey);
            if (NS_SUCCEEDED(rv))
            {
                *pResultIndex = FindViewIndex(*pResultKey);
                if (*pResultKey != resultThreadKey && (m_viewFlags & nsMsgViewFlagsType::kThreadedDisplay))
                {
                    threadIndex  = ThreadIndexOfMsg(*pResultKey, nsMsgViewIndex_None);
                    if (*pResultIndex == nsMsgViewIndex_None)
                    {
                        nsCOMPtr <nsIMsgThread> threadHdr;
                        nsCOMPtr <nsIMsgDBHdr> msgHdr;
                        rv = m_db->GetMsgHdrForKey(*pResultKey, getter_AddRefs(msgHdr));
                        NS_ENSURE_SUCCESS(rv, rv);
                        rv = GetThreadContainingMsgHdr(msgHdr, getter_AddRefs(threadHdr));
                        NS_ENSURE_SUCCESS(rv, rv);

                        NS_ASSERTION(threadHdr, "threadHdr is null");
                        if (threadHdr)
                            break;
                        PRUint32 numUnreadChildren;
                        threadHdr->GetNumUnreadChildren(&numUnreadChildren);
                        if (numUnreadChildren > 0)
                        {
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
            if (startIndex != nsMsgViewIndex_None)
              ApplyCommandToIndices(nsMsgViewCommandType::markThreadRead, &startIndex, 1);

            return NavigateFromPos(nsMsgNavigationType::nextUnreadMessage, startIndex, pResultKey, pResultIndex, pThreadIndex, PR_TRUE);
        case nsMsgNavigationType::toggleThreadKilled:
            {
                PRBool resultKilled;
                nsMsgViewIndexArray selection;
                GetSelectedIndices(selection);
                ToggleIgnored(selection.Elements(), selection.Length(), &threadIndex, &resultKilled);
                if (resultKilled)
                {
                    return NavigateFromPos(nsMsgNavigationType::nextUnreadThread, threadIndex, pResultKey, pResultIndex, pThreadIndex, PR_TRUE);
                }
                else
                {
                    *pResultIndex = nsMsgViewIndex_None;
                    *pResultKey = nsMsgKey_None;
                    return NS_OK;
                }
            }
        case nsMsgNavigationType::toggleSubthreadKilled:
            {
                PRBool resultKilled;
                nsMsgViewIndexArray selection;
                GetSelectedIndices(selection);
                ToggleMessageKilled(selection.Elements(), selection.Length(),
                    &threadIndex, &resultKilled);
                if (resultKilled)
                {
                    return NavigateFromPos(nsMsgNavigationType::nextUnreadMessage, threadIndex, pResultKey, pResultIndex, pThreadIndex, PR_TRUE);
                }
                else
                {
                    *pResultIndex = nsMsgViewIndex_None;
                    *pResultKey = nsMsgKey_None;
                    return NS_OK;
                }
            }
          // check where navigate says this will take us. If we have the message in the view,
          // return it. Otherwise, return an error.
      case nsMsgNavigationType::back:
      case nsMsgNavigationType::forward:
        {
          nsCString folderUri, msgUri;
          nsCString viewFolderUri;
          nsCOMPtr<nsIMsgFolder> curFolder = m_viewFolder ? m_viewFolder : m_folder;
          curFolder->GetURI(viewFolderUri);
          PRInt32 relPos = (motion == nsMsgNavigationType::forward)
            ? 1 : (m_currentlyDisplayedMsgKey != nsMsgKey_None) ? -1 : 0;
          PRInt32 curPos;
          nsresult rv;
          nsCOMPtr<nsIMessenger> messenger (do_QueryReferent(mMessengerWeak, &rv));
          NS_ENSURE_SUCCESS(rv, rv);
          rv = messenger->GetFolderUriAtNavigatePos(relPos, folderUri);
          NS_ENSURE_SUCCESS(rv, rv);
          if (folderUri.Equals(viewFolderUri))
          {
            nsCOMPtr <nsIMsgDBHdr> msgHdr;
            rv = messenger->GetMsgUriAtNavigatePos(relPos, msgUri);
            NS_ENSURE_SUCCESS(rv, rv);
            messenger->MsgHdrFromURI(msgUri, getter_AddRefs(msgHdr));
            if (msgHdr)
            {
              messenger->GetNavigatePos(&curPos);
              curPos += relPos;
              *pResultIndex = FindHdr(msgHdr);
              messenger->SetNavigatePos(curPos);
              msgHdr->GetMessageKey(pResultKey);
              return NS_OK;
            }
          }
          *pResultIndex = nsMsgViewIndex_None;
          *pResultKey = nsMsgKey_None;
          break;

        }
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
    PRInt32 index = nsMsgKey_None;
    nsMsgViewIndex resultIndex = nsMsgViewIndex_None;
    if (mTreeSelection)
      (void) mTreeSelection->GetCurrentIndex(&index);
    else
      index = FindViewIndex(m_currentlyDisplayedMsgKey);
    nsCOMPtr<nsIMessenger> messenger (do_QueryReferent(mMessengerWeak));
    // warning - we no longer validate index up front because fe passes in -1 for no
    // selection, so if you use index, be sure to validate it before using it
    // as an array index.
    switch (motion)
    {
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
            if (IsValidIndex(index))
            {
                nsMsgKey threadId;
                rv = FindPrevUnread(m_keys[index], &resultKey, &threadId);
                enable = (resultKey != nsMsgKey_None);
            }
            break;
        case nsMsgNavigationType::forward:
        case nsMsgNavigationType::back:
        {
          PRUint32 curPos;
          PRUint32 historyCount;
          if (messenger)
          {
            messenger->GetNavigateHistory(&curPos, &historyCount, nsnull);
            PRInt32 desiredPos = (PRInt32) curPos;
            if (motion == nsMsgNavigationType::forward)
              desiredPos++;
            else
              desiredPos--; //? operator code didn't work for me
            enable = (desiredPos >= 0 && desiredPos < (PRInt32) historyCount / 2);
          }
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

    if (GetSize() > 0)
    {
        for (curIndex = startIndex; curIndex <= lastIndex; curIndex++)
        {
            PRUint32 flags = m_flags[curIndex];
            if (flags & MSG_FLAG_MARKED)
            {
                *pResultIndex = curIndex;
                break;
            }
        }
    }

    return NS_OK;
}

nsresult nsMsgDBView::FindFirstNew(nsMsgViewIndex *pResultIndex)
{
  if (m_db)
  {
    nsMsgKey firstNewKey = nsMsgKey_None;
    m_db->GetFirstNew(&firstNewKey);
    *pResultIndex = (firstNewKey != nsMsgKey_None)
        ? FindKey(firstNewKey, PR_TRUE) : nsMsgViewIndex_None;
  }
  return NS_OK;
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

    for (; (int) curIndex >= 0 && (*pResultKey == nsMsgKey_None); curIndex--)
    {
        PRUint32 flags = m_flags[curIndex];

        if (curIndex != startIndex && flags & MSG_VIEW_FLAG_ISTHREAD && flags & MSG_FLAG_ELIDED)
        {
            NS_ASSERTION(0,"fix this");
            //nsMsgKey threadId = m_keys[curIndex];
            //rv = m_db->GetUnreadKeyInThread(threadId, pResultKey, resultThreadId);
            if (NS_SUCCEEDED(rv) && (*pResultKey != nsMsgKey_None))
                break;
        }
        if (!(flags & (MSG_FLAG_READ | MSG_VIEW_FLAG_DUMMY)) && (curIndex != startIndex))
        {
            *pResultKey = m_keys[curIndex];
            rv = NS_OK;
            break;
        }
    }
    // found unread message but we don't know the thread
    NS_ASSERTION(!(*pResultKey != nsMsgKey_None && resultThreadId && *resultThreadId == nsMsgKey_None),
      "fix this");
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

    if (GetSize() > 0 && IsValidIndex(startIndex))
    {
        curIndex = startIndex;
        do
        {
            if (curIndex != 0)
                curIndex--;

            PRUint32 flags = m_flags[curIndex];
            if (flags & MSG_FLAG_MARKED)
            {
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
    return ((index >=0) && (index < (nsMsgViewIndex) m_keys.Length()));
}

nsresult nsMsgDBView::OrExtraFlag(nsMsgViewIndex index, PRUint32 orflag)
{
  PRUint32  flag;
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
  PRUint32  flag;
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


nsresult nsMsgDBView::ToggleIgnored(nsMsgViewIndex * indices, PRInt32 numIndices, nsMsgViewIndex *resultIndex, PRBool *resultToggleState)
{
  nsCOMPtr <nsIMsgThread> thread;

  // Ignored state is toggled based on the first selected thread
  nsMsgViewIndex threadIndex = GetThreadFromMsgIndex(indices[0], getter_AddRefs(thread));
  PRUint32 threadFlags;
  thread->GetFlags(&threadFlags);
  PRUint32 ignored = threadFlags & MSG_FLAG_IGNORED;

  // Process threads in reverse order
  // Otherwise collapsing the threads will invalidate the indices
  threadIndex = nsMsgViewIndex_None;
  while (numIndices)
  {
    numIndices--;
    if (indices[numIndices] < threadIndex)
    {
      threadIndex = GetThreadFromMsgIndex(indices[numIndices], getter_AddRefs(thread));
      thread->GetFlags(&threadFlags);
      if ((threadFlags & MSG_FLAG_IGNORED) == ignored)
        SetThreadIgnored(thread, threadIndex, !ignored);
    }
  }

  if (resultIndex)
    *resultIndex = threadIndex;
  if (resultToggleState)
    *resultToggleState = !ignored;

  return NS_OK;
}

nsresult nsMsgDBView::ToggleMessageKilled(nsMsgViewIndex * indices, PRInt32 numIndices, nsMsgViewIndex *resultIndex, PRBool *resultToggleState)
{
  nsCOMPtr <nsIMsgDBHdr> header;
  nsresult rv;

  // Ignored state is toggled based on the first selected message
  rv = GetMsgHdrForViewIndex(indices[0], getter_AddRefs(header));
  PRUint32 msgFlags;
  header->GetFlags(&msgFlags);
  PRUint32 ignored = msgFlags & MSG_FLAG_IGNORED;

  // Process messages in reverse order
  // Otherwise the indices may be invalidated...
  nsMsgViewIndex msgIndex = nsMsgViewIndex_None;
  while (numIndices)
  {
    numIndices--;
    if (indices[numIndices] < msgIndex)
    {
      msgIndex = indices[numIndices];
      rv = GetMsgHdrForViewIndex(msgIndex, getter_AddRefs(header));
      header->GetFlags(&msgFlags);
      if ((msgFlags & MSG_FLAG_IGNORED) == ignored)
        SetSubthreadKilled(header, msgIndex, !ignored);
    }
  }

  if (resultIndex)
    *resultIndex = msgIndex;
  if (resultToggleState)
    *resultToggleState = !ignored;

  return NS_OK;
}

nsMsgViewIndex  nsMsgDBView::GetThreadFromMsgIndex(nsMsgViewIndex index,
                                                   nsIMsgThread **threadHdr)
{
  nsMsgKey        msgKey = GetAt(index);
  nsMsgViewIndex  threadIndex;

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

nsresult nsMsgDBView::ToggleWatched( nsMsgViewIndex* indices,  PRInt32 numIndices)
{
  nsCOMPtr <nsIMsgThread> thread;

  // Watched state is toggled based on the first selected thread
  nsMsgViewIndex threadIndex = GetThreadFromMsgIndex(indices[0], getter_AddRefs(thread));
  PRUint32 threadFlags;
  thread->GetFlags(&threadFlags);
  PRUint32 watched = threadFlags & MSG_FLAG_WATCHED;

  // Process threads in reverse order
  // for consistency with ToggleIgnored
  threadIndex = nsMsgViewIndex_None;
  while (numIndices)
  {
    numIndices--;
    if (indices[numIndices] < threadIndex)
    {
      threadIndex = GetThreadFromMsgIndex(indices[numIndices], getter_AddRefs(thread));
      thread->GetFlags(&threadFlags);
      if ((threadFlags & MSG_FLAG_WATCHED) == watched)
        SetThreadWatched(thread, threadIndex, !watched);
    }
  }

  return NS_OK;
}

nsresult nsMsgDBView::SetThreadIgnored(nsIMsgThread *thread, nsMsgViewIndex threadIndex, PRBool ignored)
{
  if (!IsValidIndex(threadIndex))
    return NS_MSG_INVALID_DBVIEW_INDEX;

  NoteChange(threadIndex, 1, nsMsgViewNotificationCode::changed);
  if (ignored)
  {
    nsTArray<nsMsgKey> idsMarkedRead;

    MarkThreadRead(thread, threadIndex, idsMarkedRead, PR_TRUE);
    CollapseByIndex(threadIndex, nsnull);
  }
  return m_db->MarkThreadIgnored(thread, m_keys[threadIndex], ignored, this);
}

nsresult nsMsgDBView::SetSubthreadKilled(nsIMsgDBHdr *header, nsMsgViewIndex msgIndex, PRBool ignored)
{
  if (!IsValidIndex(msgIndex))
    return NS_MSG_INVALID_DBVIEW_INDEX;

  NoteChange(msgIndex, 1, nsMsgViewNotificationCode::changed);
  nsresult rv;

  rv = m_db->MarkHeaderKilled(header, ignored, this);
  NS_ENSURE_SUCCESS(rv, rv);
 
  if (ignored)
  {
    nsCOMPtr <nsIMsgThread> thread;
    nsresult rv;
    rv = m_db->GetThreadContainingMsgHdr(header, getter_AddRefs(thread));
    if (NS_FAILED(rv))
      return NS_OK; // So we didn't mark threads read

    PRUint32 children, current;
    thread->GetNumChildren(&children);

    nsMsgKey headKey;
    header->GetMessageKey(&headKey);

    for (current = 0; current < children; current++)
    {
       nsMsgKey newKey;
       thread->GetChildKeyAt(current, &newKey);
       if (newKey == headKey)
         break;
    }

    // Process all messages, starting with this message.
    for (; current < children; current++)
    {
       nsCOMPtr <nsIMsgDBHdr> nextHdr;
       PRBool isKilled;
       
       thread->GetChildHdrAt(current, getter_AddRefs(nextHdr));
       nextHdr->GetIsKilled(&isKilled);
       
       // Ideally, the messages should stop processing here.
       // However, the children are ordered not by thread...
       if (isKilled)
         nextHdr->MarkRead(PR_TRUE);
    }
  }
  return NS_OK;
}

nsresult nsMsgDBView::SetThreadWatched(nsIMsgThread *thread, nsMsgViewIndex index, PRBool watched)
{
  if (!IsValidIndex(index))
    return NS_MSG_INVALID_DBVIEW_INDEX;

  NoteChange(index, 1, nsMsgViewNotificationCode::changed);
  return m_db->MarkThreadWatched(thread, m_keys[index], watched, this);
}

NS_IMETHODIMP nsMsgDBView::GetMsgFolder(nsIMsgFolder **aMsgFolder)
{
  NS_ENSURE_ARG_POINTER(aMsgFolder);
  NS_IF_ADDREF(*aMsgFolder = m_folder);
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::SetViewFolder(nsIMsgFolder *aMsgFolder)
{
  m_viewFolder = aMsgFolder;
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::GetViewFolder(nsIMsgFolder **aMsgFolder)
{
  NS_ENSURE_ARG_POINTER(aMsgFolder);
  NS_IF_ADDREF(*aMsgFolder = m_viewFolder);
  return NS_OK;
}


NS_IMETHODIMP
nsMsgDBView::GetNumSelected(PRUint32 *numSelected)
{
  NS_ENSURE_ARG_POINTER(numSelected);

  if (!mTreeSelection)
  {
    *numSelected = 0;
    return NS_OK;
  }

  // We call this a lot from the front end JS, so make it fast.
  return mTreeSelection->GetCount((PRInt32*)numSelected);
}

NS_IMETHODIMP
nsMsgDBView::GetMsgToSelectAfterDelete(nsMsgViewIndex *msgToSelectAfterDelete)
{
  NS_ENSURE_ARG_POINTER(msgToSelectAfterDelete);
  *msgToSelectAfterDelete = nsMsgViewIndex_None;
  if (!mTreeSelection)
  {
    // if we don't have an tree selection then we must be in stand alone mode.
    // return the index of the current message key as the first selected index.
    *msgToSelectAfterDelete = FindViewIndex(m_currentlyDisplayedMsgKey);
    return NS_OK;
  }

  PRInt32 selectionCount;
  PRInt32 startRange;
  PRInt32 endRange;
  nsresult rv = mTreeSelection->GetRangeCount(&selectionCount);
  for (PRInt32 i = 0; i < selectionCount; i++)
  {
    rv = mTreeSelection->GetRangeAt(i, &startRange, &endRange);
    *msgToSelectAfterDelete = PR_MIN(*msgToSelectAfterDelete, startRange);
  }
  nsCOMPtr<nsIMsgFolder> folder;
  GetMsgFolder(getter_AddRefs(folder));
  nsCOMPtr <nsIMsgImapMailFolder> imapFolder = do_QueryInterface(folder);
  PRBool thisIsImapFolder = (imapFolder != nsnull);
  if (thisIsImapFolder) //need to update the imap-delete model, can change more than once in a session.
    GetImapDeleteModel(nsnull);

  // If mail.delete_matches_sort_order is true,
  // for views sorted in descending order (newest at the top), make msgToSelectAfterDelete
  // advance in the same direction as the sort order.
  PRBool deleteMatchesSort = PR_FALSE;
  if (m_sortOrder == nsMsgViewSortOrder::descending && *msgToSelectAfterDelete)
  {
    nsCOMPtr<nsIPrefBranch> prefBranch (do_GetService(NS_PREFSERVICE_CONTRACTID, &rv));
    NS_ENSURE_SUCCESS(rv, rv);
    prefBranch->GetBoolPref("mail.delete_matches_sort_order", &deleteMatchesSort);
  }

  if (mDeleteModel == nsMsgImapDeleteModels::IMAPDelete)
  {
    if (selectionCount > 1 || (endRange-startRange) > 0)  //multiple selection either using Ctrl or Shift keys
      *msgToSelectAfterDelete = nsMsgViewIndex_None;
    else if(deleteMatchesSort)
      *msgToSelectAfterDelete -= 1;
    else
      *msgToSelectAfterDelete += 1;
  }
  else if (deleteMatchesSort)
  {
    *msgToSelectAfterDelete -= 1;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsMsgDBView::GetRemoveRowOnMoveOrDelete(PRBool *aRemoveRowOnMoveOrDelete)
{
  NS_ENSURE_ARG_POINTER(aRemoveRowOnMoveOrDelete);
  nsCOMPtr <nsIMsgImapMailFolder> imapFolder = do_QueryInterface(m_folder);
  if (!imapFolder)
  {
    *aRemoveRowOnMoveOrDelete = PR_TRUE;
    return NS_OK;
  }

  // need to update the imap-delete model, can change more than once in a session.
  GetImapDeleteModel(nsnull);

  // unlike the other imap delete models, "mark as deleted" does not remove rows on delete (or move)
  *aRemoveRowOnMoveOrDelete = (mDeleteModel != nsMsgImapDeleteModels::IMAPDelete);
  return NS_OK;
}


NS_IMETHODIMP
nsMsgDBView::GetCurrentlyDisplayedMessage(nsMsgViewIndex *currentlyDisplayedMessage)
{
  NS_ENSURE_ARG_POINTER(currentlyDisplayedMessage);
  *currentlyDisplayedMessage = FindViewIndex(m_currentlyDisplayedMsgKey);
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

  if (!m_db)
    return NS_MSG_MESSAGE_NOT_FOUND;

  rv = m_db->GetMsgHdrForKey(key, hdr);
  NS_ENSURE_SUCCESS(rv,rv);
  return NS_OK;
}

// if nothing selected, return an NS_ERROR
NS_IMETHODIMP
nsMsgDBView::GetURIForFirstSelectedMessage(nsACString &uri)
{
  nsresult rv;
  nsMsgViewIndex viewIndex;
  rv = GetViewIndexForFirstSelectedMsg(&viewIndex);
  // don't assert, it is legal for nothing to be selected
  if (NS_FAILED(rv)) return rv;

  return GetURIForViewIndex(viewIndex, uri);
}

NS_IMETHODIMP
nsMsgDBView::OnDeleteCompleted(PRBool aSucceeded)
{
  if (m_deletingRows)
  {
    if (aSucceeded)
    {
      PRUint32 numIndices = mIndicesToNoteChange.Length();
      if (numIndices)
      {
        if (mTree)
        {
          if (numIndices > 1)
            mIndicesToNoteChange.Sort();

          // the call to NoteChange() has to happen after we are done removing the keys
          // as NoteChange() will call RowCountChanged() which will call our GetRowCount()
          if (numIndices > 1)
            mTree->BeginUpdateBatch();
          for (PRUint32 i=0;i<numIndices;i++)
            NoteChange(mIndicesToNoteChange[i], -1, nsMsgViewNotificationCode::insertOrDelete);
          if (numIndices > 1)
            mTree->EndUpdateBatch();
        }
        mIndicesToNoteChange.Clear();
      }
    }
  }

 m_deletingRows = PR_FALSE;
 return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::GetDb(nsIMsgDatabase **aDB)
{
  NS_ENSURE_ARG_POINTER(aDB);
  NS_IF_ADDREF(*aDB = m_db);
  return NS_OK;
}

PRBool nsMsgDBView::OfflineMsgSelected(nsMsgViewIndex * indices, PRInt32 numIndices)
{
  nsCOMPtr <nsIMsgLocalMailFolder> localFolder = do_QueryInterface(m_folder);
  if (localFolder)
    return PR_TRUE;

  for (nsMsgViewIndex index = 0; index < (nsMsgViewIndex) numIndices; index++)
  {
    PRUint32 flags = m_flags[indices[index]];
    if ((flags & MSG_FLAG_OFFLINE))
      return PR_TRUE;
  }
  return PR_FALSE;
}

PRBool nsMsgDBView::NonDummyMsgSelected(nsMsgViewIndex * indices, PRInt32 numIndices)
{
  for (nsMsgViewIndex index = 0; index < (nsMsgViewIndex) numIndices; index++)
  {
    PRUint32 flags = m_flags[indices[index]];
    if (!(flags & MSG_VIEW_FLAG_DUMMY))
      return PR_TRUE;
  }
  return PR_FALSE;
}

NS_IMETHODIMP nsMsgDBView::GetViewIndexForFirstSelectedMsg(nsMsgViewIndex *aViewIndex)
{
  NS_ENSURE_ARG_POINTER(aViewIndex);
  // if we don't have an tree selection we must be in stand alone mode....
  if (!mTreeSelection)
  {
    *aViewIndex = m_currentlyDisplayedViewIndex;
    return NS_OK;
  }

  PRInt32 startRange;
  PRInt32 endRange;
  nsresult rv = mTreeSelection->GetRangeAt(0, &startRange, &endRange);
  // don't assert, it is legal for nothing to be selected
  if (NS_FAILED(rv))
    return rv;

  // check that the first index is valid, it may not be if nothing is selected
  if (startRange >= 0 && startRange < GetSize())
    *aViewIndex = startRange;
  else
    return NS_ERROR_UNEXPECTED;
  return NS_OK;
}

NS_IMETHODIMP
nsMsgDBView::GetKeyForFirstSelectedMessage(nsMsgKey *key)
{
  NS_ENSURE_ARG_POINTER(key);
  // if we don't have an tree selection we must be in stand alone mode....
  if (!mTreeSelection)
  {
    *key = m_currentlyDisplayedMsgKey;
    return NS_OK;
  }

  PRInt32 startRange;
  PRInt32 endRange;
  nsresult rv = mTreeSelection->GetRangeAt(0, &startRange, &endRange);
  // don't assert, it is legal for nothing to be selected
  if (NS_FAILED(rv))
    return rv;

  // check that the first index is valid, it may not be if nothing is selected
  if (startRange >= 0 && startRange < GetSize())
  {
    if (m_flags[startRange] & MSG_VIEW_FLAG_DUMMY)
      return NS_MSG_INVALID_DBVIEW_INDEX;

    *key = m_keys[startRange];
  }
  else
    return NS_ERROR_UNEXPECTED;
  return NS_OK;
}

nsresult nsMsgDBView::GetFolders(nsISupportsArray **aFolders)
{
    NS_ENSURE_ARG_POINTER(aFolders);
    *aFolders = nsnull;

    return NS_OK;
}

nsresult nsMsgDBView::AdjustRowCount(PRInt32 rowCountBeforeSort, PRInt32 rowCountAfterSort)
{
  PRInt32 rowChange = rowCountAfterSort - rowCountBeforeSort;

  if (rowChange)
  {
    // this is not safe to use when you have a selection
    // RowCountChanged() will call AdjustSelection()
    PRUint32 numSelected = 0;
    GetNumSelected(&numSelected);
    NS_ASSERTION(numSelected == 0, "it is not save to call AdjustRowCount() when you have a selection");

    if (mTree)
      mTree->RowCountChanged(0, rowChange);
  }
  return NS_OK;
}

nsresult nsMsgDBView::GetImapDeleteModel(nsIMsgFolder *folder)
{
   nsresult rv = NS_OK;
   nsCOMPtr <nsIMsgIncomingServer> server;
   if (folder) //for the search view
     folder->GetServer(getter_AddRefs(server));
   else if (m_folder)
     m_folder->GetServer(getter_AddRefs(server));
   nsCOMPtr<nsIImapIncomingServer> imapServer = do_QueryInterface(server, &rv);
   if (NS_SUCCEEDED(rv) && imapServer )
     imapServer->GetDeleteModel(&mDeleteModel);
   return rv;
}


//
// CanDrop
//
// Can't drop on the thread pane.
//
NS_IMETHODIMP nsMsgDBView::CanDrop(PRInt32 index, PRInt32 orient, PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = PR_FALSE;

  return NS_OK;
}


//
// Drop
//
// Can't drop on the thread pane.
//
NS_IMETHODIMP nsMsgDBView::Drop(PRInt32 row, PRInt32 orient)
{
  return NS_OK;
}


//
// IsSorted
//
// ...
//
NS_IMETHODIMP nsMsgDBView::IsSorted(PRBool *_retval)
{
  *_retval = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::SelectFolderMsgByKey(nsIMsgFolder *aFolder, nsMsgKey aKey)
{
  NS_ENSURE_ARG_POINTER(aFolder);
  if (aKey == nsMsgKey_None)
    return NS_ERROR_FAILURE;

  // this is OK for non search views.

  nsMsgViewIndex viewIndex = FindKey(aKey, PR_TRUE /* expand */);

  if (mTree)
    mTreeSelection->SetCurrentIndex(viewIndex);

  // make sure the current message is once again visible in the thread pane
  // so we don't have to go search for it in the thread pane
  if (mTree && viewIndex != nsMsgViewIndex_None)
  {
    mTreeSelection->Select(viewIndex);
    mTree->EnsureRowIsVisible(viewIndex);
  }
  return NS_OK;
}

NS_IMETHODIMP nsMsgDBView::SelectMsgByKey(nsMsgKey aKey)
{
  NS_ASSERTION(aKey != nsMsgKey_None, "bad key");
  if (aKey == nsMsgKey_None)
    return NS_OK;

  // use SaveAndClearSelection()
  // and RestoreSelection() so that we'll clear the current selection
  // but pass in a different key array so that we'll
  // select (and load) the desired message

  nsAutoTArray<nsMsgKey, 1> preservedSelection;
  nsresult rv = SaveAndClearSelection(nsnull, preservedSelection);
  NS_ENSURE_SUCCESS(rv,rv);

  // now, restore our desired selection
  nsAutoTArray<nsMsgKey, 1> keyArray;
  keyArray.AppendElement(aKey);

  // if the key was not found
  // (this can happen with "remember last selected message")
  // nothing will be selected
  rv = RestoreSelection(aKey, keyArray);
  NS_ENSURE_SUCCESS(rv,rv);
  return NS_OK;
}

NS_IMETHODIMP
nsMsgDBView::CloneDBView(nsIMessenger *aMessengerInstance, nsIMsgWindow *aMsgWindow, nsIMsgDBViewCommandUpdater *aCmdUpdater, nsIMsgDBView **_retval)
{
  nsMsgDBView* newMsgDBView;

  NS_NEWXPCOM(newMsgDBView, nsMsgDBView);
  if (!newMsgDBView)
    return NS_ERROR_OUT_OF_MEMORY;

  nsresult rv = CopyDBView(newMsgDBView, aMessengerInstance, aMsgWindow, aCmdUpdater);
  NS_ENSURE_SUCCESS(rv,rv);

  NS_IF_ADDREF(*_retval = newMsgDBView);
  return NS_OK;
}

nsresult nsMsgDBView::CopyDBView(nsMsgDBView *aNewMsgDBView, nsIMessenger *aMessengerInstance, nsIMsgWindow *aMsgWindow, nsIMsgDBViewCommandUpdater *aCmdUpdater)
{
  NS_ENSURE_ARG_POINTER(aNewMsgDBView);
  if (aMsgWindow)
  {
    aNewMsgDBView->mMsgWindowWeak = do_GetWeakReference(aMsgWindow);
    aMsgWindow->SetOpenFolder(m_viewFolder? m_viewFolder : m_folder);
  }
  aNewMsgDBView->mMessengerWeak = do_GetWeakReference(aMessengerInstance);
  aNewMsgDBView->mCommandUpdater = aCmdUpdater;
  aNewMsgDBView->m_folder = m_folder;
  aNewMsgDBView->m_viewFlags = m_viewFlags;
  aNewMsgDBView->m_sortOrder = m_sortOrder;
  aNewMsgDBView->m_sortType = m_sortType;
  aNewMsgDBView->m_db = m_db;
  aNewMsgDBView->mDateFormatter = mDateFormatter;
  if (m_db)
    aNewMsgDBView->m_db->AddListener(aNewMsgDBView);
  aNewMsgDBView->mIsNews = mIsNews;
  aNewMsgDBView->mShowSizeInLines = mShowSizeInLines;
  aNewMsgDBView->mHeaderParser = mHeaderParser;
  aNewMsgDBView->mDeleteModel = mDeleteModel;
  aNewMsgDBView->m_flags = m_flags;
  aNewMsgDBView->m_levels = m_levels;
  aNewMsgDBView->m_keys = m_keys;

  return NS_OK;
}

NS_IMETHODIMP
nsMsgDBView::GetSearchSession(nsIMsgSearchSession* *aSession)
{
  NS_ASSERTION(PR_FALSE, "should be overriden by child class");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsMsgDBView::SetSearchSession(nsIMsgSearchSession *aSession)
{
  NS_ASSERTION(PR_FALSE, "should be overriden by child class");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsMsgDBView::GetSupportsThreading(PRBool *aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
nsMsgDBView::FindIndexFromKey(nsMsgKey aMsgKey, PRBool aExpand, nsMsgViewIndex *aIndex)
{
  NS_ENSURE_ARG_POINTER(aIndex);

  *aIndex = FindKey(aMsgKey, aExpand);
  return NS_OK;
}

static void getDateFormatPref( nsIPrefBranch* _prefBranch, const char* _prefLocalName, nsDateFormatSelector& _format )
{
  // read
  PRInt32 nFormatSetting( 0 );
  nsresult result = _prefBranch->GetIntPref( _prefLocalName, &nFormatSetting );
  if ( NS_SUCCEEDED( result ) )
  {
    // translate
    nsDateFormatSelector res( nFormatSetting );
    // transfer if valid
    if ( ( res >= kDateFormatNone ) && ( res <= kDateFormatWeekday ) )
      _format = res;
  }
}

nsresult nsMsgDBView::InitDisplayFormats()
{
  m_dateFormatDefault   = kDateFormatShort;
  m_dateFormatThisWeek  = kDateFormatShort;
  m_dateFormatToday     = kDateFormatNone;

  nsresult rv = NS_OK;
  nsCOMPtr<nsIPrefService> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv,rv);
  nsCOMPtr<nsIPrefBranch> dateFormatPrefs;
  rv = prefs->GetBranch("mail.ui.display.dateformat.", getter_AddRefs(dateFormatPrefs));
  NS_ENSURE_SUCCESS(rv,rv);

  getDateFormatPref( dateFormatPrefs, "default", m_dateFormatDefault );
  getDateFormatPref( dateFormatPrefs, "thisweek", m_dateFormatThisWeek );
  getDateFormatPref( dateFormatPrefs, "today", m_dateFormatToday );
  return rv;
}

void nsMsgDBView::SetMRUTimeForFolder(nsIMsgFolder *folder)
{
  PRUint32 seconds;
  PRTime2Seconds(PR_Now(), &seconds);
  nsCAutoString nowStr;
  nowStr.AppendInt(seconds);
  folder->SetStringProperty(MRU_TIME_PROPERTY, nowStr);
}
