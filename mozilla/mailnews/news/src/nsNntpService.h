/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

#ifndef nsNntpService_h___
#define nsNntpService_h___

#include "nsINntpService.h"
#include "nsIProtocolHandler.h"
#include "nsIMsgMessageService.h"
#include "nsINntpIncomingServer.h"
#include "nsIFileSpec.h"
#include "MailNewsTypes.h"
#include "nsIMsgProtocolInfo.h"
#include "nsIMsgWindow.h"
#include "nsINntpUrl.h"

class nsIURI;
class nsIUrlListener;

class nsNntpService : public nsINntpService,
                      public nsIMsgMessageService,
                      public nsIProtocolHandler,
                      public nsIMsgProtocolInfo
{
public:

  NS_DECL_ISUPPORTS
  NS_DECL_NSINNTPSERVICE
  NS_DECL_NSIMSGMESSAGESERVICE
  NS_DECL_NSIPROTOCOLHANDLER
  NS_DECL_NSIMSGPROTOCOLINFO
  
  // nsNntpService
  nsNntpService();
  virtual ~nsNntpService();
protected:
  nsresult ConvertNewsMessageURI2NewsURI(const char *messageURI,
                                         nsCString &newsURI,
                                         nsCString &newsgroupName,
                                         nsMsgKey *aKey);
  
  nsresult SetUpNntpUrlForPosting(nsINntpUrl * nntpUrl, const char *newsgroupNames);
  nsresult FindHostFromGroup(nsCString &host, nsCString &groupName);
  void FindServerWithNewsgroup(nsCString &host, nsCString &groupName);
  
  // a convience routine used to put together news urls.
  nsresult ConstructNntpUrl(const char * urlString, const char * newsgroupName, nsMsgKey key, nsIUrlListener *aUrlListener,  nsIURI ** aUrl);
  // a convience routine to run news urls
  nsresult RunNewsUrl (nsIURI * aUrl, nsIMsgWindow *aMsgWindow, nsISupports * aConsumer);
  static PRBool findNewsServerWithGroup(nsISupports *aElement, void *data);
};

#endif /* nsNntpService_h___ */
