/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
#ifndef nsIImapMailFolderSink_h__
#define nsIImapMailFolderSink_h__

#include "nscore.h"
#include "nsISupports.h"
#include "nsImapCore.h"
#include "nsIImapProtocol.h"

typedef PRInt32  ImapOnlineCopyState;

/* starting interface:    ImapOnlineCopyStateType */

#define IMAPONLINECOPYSTATETYPE_IID_STR "5f7484b0-68b4-11d3-a53e-0060b0fc04b7"

#define IMAPONLINECOPYSTATETYPE_IID \
  {0x5f7484b0, 0x68b4, 0x11d3, \
    { 0xa5, 0x3e, 0x00, 0x60, 0xb0, 0xfc, 0x04, 0xb7 }}

class ImapOnlineCopyStateType {
 public: 
  NS_DEFINE_STATIC_IID_ACCESSOR(IMAPONLINECOPYSTATETYPE_IID)

  enum { kInProgress = 0 };

  enum { kSuccessfulCopy = 1 };

  enum { kFailedCopy = 2 };

  enum { kSuccessfulDelete = 3 };

  enum { kFailedDelete = 4 };

  enum { kReadyForAppendData = 5 };

  enum { kFailedAppend = 6 };

  enum { kInterruptedState = 7 };
};

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_IMAPONLINECOPYSTATETYPE \

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_IMAPONLINECOPYSTATETYPE(_to) \


/* 3b2dd7e0-e72c-11d2-ab7b-00805f8ac968 */

#define NS_IIMAPMAILFOLDERSINK_IID \
{ 0x3b2dd7e0, 0xe72c, 0x11d2, \
    { 0xab, 0x7b, 0x00, 0x80, 0x5f, 0x8a, 0xc9, 0x68 } }

class nsIImapMailFolderSink : public nsISupports
{
public:
    static const nsIID& GetIID()
    {
        static nsIID iid = NS_IIMAPMAILFOLDERSINK_IID;
        return iid;
    }

    // Tell mail master about the newly selected mailbox
    NS_IMETHOD UpdateImapMailboxInfo(nsIImapProtocol* aProtocol,
                                     nsImapMailboxSpec* aSpec) = 0;
    NS_IMETHOD UpdateImapMailboxStatus(nsIImapProtocol* aProtocol,
                                       nsImapMailboxSpec* aSpec) = 0;
    NS_IMETHOD ChildDiscoverySucceeded(nsIImapProtocol* aProtocol) = 0;
    NS_IMETHOD PromptUserForSubscribeUpdatePath(nsIImapProtocol* aProtocol,
                                                PRBool* aBool) = 0;
    NS_IMETHOD SetupHeaderParseStream(nsIImapProtocol* aProtocol,
                                   StreamInfo* aStreamInfo) = 0;

    NS_IMETHOD ParseAdoptedHeaderLine(nsIImapProtocol* aProtocol,
                                   msg_line_info* aMsgLineInfo) = 0;
    
    NS_IMETHOD NormalEndHeaderParseStream(nsIImapProtocol* aProtocol) = 0;
    
    NS_IMETHOD AbortHeaderParseStream(nsIImapProtocol* aProtocol) = 0;
    
//    NS_IMETHOD OnlineCopyCompleted(nsIImapProtocol* aProtocol, ImapOnlineCopyState aCopyState) = 0;
};

#endif
