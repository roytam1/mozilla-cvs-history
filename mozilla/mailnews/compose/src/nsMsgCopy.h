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

#ifndef _nsMsgCopy_H_
#define _nsMsgCopy_H_

#include "nscore.h"
#include "nsIFileSpec.h"
#include "nsMsgSend.h"
#include "nsIMsgFolder.h"
#include "nsITransactionManager.h"
#include "nsIMsgCopyServiceListener.h"
#include "nsIMsgCopyService.h"

// {0874C3B5-317D-11d3-8EFB-00A024A7D144}
#define NS_IMSGCOPY_IID           \
{ 0x874c3b5, 0x317d, 0x11d3,      \
{ 0x8e, 0xfb, 0x0, 0xa0, 0x24, 0xa7, 0xd1, 0x44 } };

// Forward declarations...
class   nsMsgCopy;

////////////////////////////////////////////////////////////////////////////////////
// This is the listener class for the copy operation. We have to create this class 
// to listen for message copy completion and eventually notify the caller
////////////////////////////////////////////////////////////////////////////////////
class CopyListener : public nsIMsgCopyServiceListener
{
public:
  CopyListener(void);
  virtual ~CopyListener(void);

  // nsISupports interface
  NS_DECL_ISUPPORTS

  NS_IMETHOD OnStartCopy();
  
  NS_IMETHOD OnProgress(PRUint32 aProgress, PRUint32 aProgressMax);

  NS_IMETHOD SetMessageKey(PRUint32 aMessageKey);
  
  NS_IMETHOD GetMessageId(nsCString* aMessageId);
  
  NS_IMETHOD OnStopCopy(nsresult aStatus);

  NS_IMETHOD SetMsgComposeAndSendObject(nsMsgComposeAndSend *obj);
  
  nsCOMPtr<nsISupports> mCopyObject;
  PRBool                          mCopyInProgress;

private:
  nsCOMPtr<nsMsgComposeAndSend>       mComposeAndSend;
};

//
// This is a class that deals with processing remote attachments. It implements
// an nsIStreamListener interface to deal with incoming data
//
class nsMsgCopy : public nsISupports
{
public:
  static const nsIID& GetIID() { static nsIID iid = NS_IMSGCOPY_IID; return iid; }

  nsMsgCopy();
  virtual ~nsMsgCopy();

  // nsISupports interface
  NS_DECL_ISUPPORTS

  //////////////////////////////////////////////////////////////////////
  // Object methods...
  //////////////////////////////////////////////////////////////////////
  //
  nsresult              StartCopyOperation(nsIMsgIdentity       *aUserIdentity,
                                           nsIFileSpec          *aFileSpec, 
                                           nsMsgDeliverMode     aMode,
                                           nsMsgComposeAndSend  *aMsgSendObj,
                                           const char           *aSavePref,
                                           nsIMessage           *aMsgToReplace);

  nsresult              DoCopy(nsIFileSpec *aDiskFile, nsIMsgFolder *dstFolder,
                               nsIMessage *aMsgToReplace, PRBool aIsDraft,
                               nsIMsgWindow *msgWindow,
                               nsMsgComposeAndSend   *aMsgSendObj);

  nsresult	GetUnsentMessagesFolder(nsIMsgIdentity *userIdentity, nsIMsgFolder **msgFolder);
  nsresult	GetDraftsFolder(nsIMsgIdentity *userIdentity, nsIMsgFolder **msgFolder);
  nsresult	GetTemplatesFolder(nsIMsgIdentity *userIdentity, nsIMsgFolder **msgFolder);
  nsresult	GetSentFolder(nsIMsgIdentity *userIdentity,  nsIMsgFolder **msgFolder);

  
  //
  // Vars for implementation...
  //
  nsIFileSpec                     *mFileSpec;     // the file we are sending...
  nsMsgDeliverMode                mMode;
  nsCOMPtr<CopyListener>          mCopyListener;
  char                            *mSavePref;
};

// Useful function for the back end...
nsresult	LocateMessageFolder(nsIMsgIdentity   *userIdentity, 
                                       nsMsgDeliverMode aFolderType,
                                       const char       *aSaveURI,
				       nsIMsgFolder **msgFolder);

nsresult	MessageFolderIsLocal(nsIMsgIdentity   *userIdentity, 
                                       nsMsgDeliverMode aFolderType,
                                       const char       *aSaveURI,
				       PRBool		*aResult);

#endif /* _nsMsgCopy_H_ */
