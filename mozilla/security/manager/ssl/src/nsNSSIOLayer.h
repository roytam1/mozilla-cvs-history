/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s):
 *  Brian Ryner <bryner@netscape.com>
 */

#ifndef _NSNSSIOLAYER_H
#define _NSNSSIOLAYER_H

#include "prtypes.h"
#include "prio.h"
#include "nsString.h"
#include "nsIInterfaceRequestor.h"
#include "nsITransportSecurityInfo.h"
#include "nsISSLSocketControl.h"

class nsIChannel;

class nsNSSSocketInfo : public nsITransportSecurityInfo,
                        public nsISSLSocketControl,
                        public nsIInterfaceRequestor
{
public:
  nsNSSSocketInfo();
  virtual ~nsNSSSocketInfo();
  
  NS_DECL_ISUPPORTS
  NS_DECL_NSITRANSPORTSECURITYINFO
  NS_DECL_NSISSLSOCKETCONTROL
  NS_DECL_NSIINTERFACEREQUESTOR

  nsresult SetHostName(const char *aHostName);
  nsresult SetProxyName(const char *aName);
  
  nsresult SetHostPort(PRInt32 aPort);
  nsresult SetProxyPort(PRInt32 aPort);

  nsresult SetSecurityState(PRInt32 aState);
  nsresult SetShortSecurityDescription(const PRUnichar *aText);

  nsresult SetUseTLS(PRBool useTLS);
  nsresult GetUseTLS(PRBool *useTLS);

  nsresult GetFileDescPtr(PRFileDesc** aFilePtr);
  nsresult SetFileDescPtr(PRFileDesc* aFilePtr);
  
protected:
  nsString mHostName;
  PRInt32 mHostPort;
  
  nsString mProxyName;
  PRInt32 mProxyPort;
  
  nsCOMPtr<nsIInterfaceRequestor> mCallbacks;
  PRFileDesc* mFd;
  PRInt32 mSecurityState;
  nsString mShortDesc;
  PRBool mForceHandshake;
  PRBool mUseTLS;
};

nsresult nsSSLIOLayerNewSocket(const char *host,
                               PRInt32 port,
                               const char *proxyHost,
                               PRInt32 proxyPort,
                               PRFileDesc **fd,
                               nsISupports **securityInfo,
                               PRBool useTLS);

nsresult nsSSLIOLayerAddToSocket(const char *host,
                                 PRInt32 port,
                                 const char *proxyHost,
                                 PRInt32 proxyPort,
                                 PRFileDesc *fd,
                                 nsISupports **securityInfo,
                                 PRBool useTLS);
#endif /* _NSNSSIOLAYER_H */
