/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * The Original Code is mozilla.org code.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are 
 * Copyright (C) 2001 Netscape Communications Corporation.  All
 * Rights Reserved.
 * 
 * Contributor(s): Dan Mosedale <dmose@netscape.com> (Original Author)
 *
 */

#include "nsCOMPtr.h"
#include "nsIAutoCompleteSession.h"
#include "nsILDAPConnection.h"
#include "nsILDAPMessageListener.h"
#include "nsILDAPAutoCompleteSession.h"
#include "nsILDAPURL.h"
#include "nsString.h"
#include "nsISupportsArray.h"

// 964665d0-1dd1-11b2-aeae-897834fb00b9
//
#define NS_LDAPAUTOCOMPLETESESSION_CID \
{ 0x964665d0, 0x1dd1, 0x11b2, \
 { 0xae, 0xae, 0x89, 0x78, 0x34, 0xfb, 0x00, 0xb9 }}

class nsLDAPAutoCompleteSession : public nsILDAPMessageListener, 
                                  public nsILDAPAutoCompleteSession
{
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIAUTOCOMPLETESESSION
    NS_DECL_NSILDAPMESSAGELISTENER
    NS_DECL_NSILDAPAUTOCOMPLETESESSION

    nsLDAPAutoCompleteSession();
    virtual ~nsLDAPAutoCompleteSession();

  protected:
    enum SessionState { UNBOUND, BINDING, BOUND, SEARCHING } mState;
    PRUint32 mEntriesReturned;                    // # of entries returned?
    nsCOMPtr<nsILDAPConnection> mConnection;      // connection used for search
    nsCOMPtr<nsILDAPOperation> mOperation;        // current ldap op
    nsCOMPtr<nsIAutoCompleteListener> mListener;  // callback 
    nsCOMPtr<nsIAutoCompleteResults> mResults;    // being built up
    nsCOMPtr<nsISupportsArray> mResultsArray;     // cached, to avoid re-gets
    nsString mSearchString;                       // autocomplete this string
    nsString mFilterTemplate;                     // search filter template
    nsString mOutputFormat;                       // how to format output
    nsCOMPtr<nsILDAPURL> mServerURL;        // URL for the directory to search
    PRInt32 mSizeLimit;                     // return at most this many entries


    // stopgap until nsLDAPService works
    nsresult InitConnection();             

    // check that we bound ok and start then call StartLDAPSearch
    nsresult OnLDAPBind(nsILDAPMessage *aMessage); 

    // add to the results set
    nsresult OnLDAPSearchEntry(nsILDAPMessage *aMessage); 

    // all done; call OnAutoComplete
    nsresult OnLDAPSearchResult(nsILDAPMessage *aMessage); 

    // kick off a search
    nsresult StartLDAPSearch();

    // finish a search by calling mListener->OnAutoComplete, resetting state,
    // and freeing resources.
    void FinishAutoCompleteLookup(AutoCompleteStatus aACStatus);
};

