/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#ifndef _nsMsgCompose_H_
#define _nsMsgCompose_H_

#include "nsIMsgCompose.h"
#include "nsCOMPtr.h"
#include "nsMsgCompFields.h"
#include "nsIWebShell.h"
#include "nsIWebShellWindow.h"
#include "nsIOutputStream.h"
#include "nsIMsgQuote.h"
#include "nsIMsgSendListener.h"
#include "nsIMsgCopyServiceListener.h"
#include "nsIMsgSend.h"
#include "nsIStreamListener.h"
#include "nsIMimeHeaders.h"

// Forward declares
class QuotingOutputStreamListener;
class nsMsgComposeSendListener;
class nsMsgDocumentStateListener;

class nsMsgCompose : public nsIMsgCompose
{
 public: 

	nsMsgCompose();
	virtual ~nsMsgCompose();

	/* this macro defines QueryInterface, AddRef and Release for this class */
	NS_DECL_ISUPPORTS

	/*** nsIMsgCompose pure virtual functions */
	NS_DECL_NSIMSGCOMPOSE

   MSG_ComposeType				GetMessageType();

 // Deal with quoting issues...
	nsresult                      QuoteOriginalMessage(const PRUnichar * originalMsgURI, PRInt32 what); // New template
  PRBool                        QuotingToFollow(void);
  nsresult                      SetQuotingToFollow(PRBool aVal);
  nsresult                      LoadAsQuote(nsString  aTextToLoad);
  nsresult                      ConvertHTMLToText(nsFileSpec& aSigFile, nsString &aSigData);
  nsresult                      ConvertTextToHTML(nsFileSpec& aSigFile, nsString &aSigData);
  nsresult                      BuildBodyMessage();

  nsString                      mQuoteURI;
  nsFileSpec                    *mSigFileSpec;
  nsFileSpec                    *mTempComposeFileSpec;

  PRInt32                       mWhatHolder;

  nsresult                      ProcessSignature(nsOutputFileStream *aAppendFileStream,
                                                 nsIMsgIdentity *identity); // for setting up the users compose window environment
  nsresult                      BuildQuotedMessageAndSignature(void);                    // for setting up the users compose window environment
	nsresult                      ShowWindow(PRBool show);
  nsresult                      LoadDataFromFile(nsFileSpec& fSpec, nsString &sigData);


 private:
	nsresult _SendMsg(MSG_DeliverMode deliverMode, nsIMsgIdentity *identity, const PRUnichar *callback);
	nsresult CreateMessage(const PRUnichar * originalMsgURI, MSG_ComposeType type, MSG_ComposeFormat format, nsIMsgCompFields* compFields, nsISupports* object);
	void CleanUpRecipients(nsString& recipients);

	nsMsgComposeSendListener      *m_sendListener;
	nsIEditorShell                *m_editor;
	nsIDOMWindow                  *m_window;
	nsIWebShell                   *m_webShell;
	nsIWebShellWindow             *m_webShellWin;
	nsMsgCompFields               *m_compFields;
  nsCOMPtr<nsIMsgIdentity>      m_identity;
	PRBool						            m_composeHTML;
	QuotingOutputStreamListener   *mQuoteStreamListener;
	nsCOMPtr<nsIOutputStream>     mBaseStream;

  nsCOMPtr<nsIMsgSend>          mMsgSend;   // for composition back end

  // Deal with quoting issues...
	nsCOMPtr<nsIMsgQuote>         mQuote;
	PRBool						            mQuotingToFollow; // Quoting indicator
  nsMsgDocumentStateListener    *mDocumentListener;
  MSG_ComposeType				mType;		//Message type
};

////////////////////////////////////////////////////////////////////////////////////
// THIS IS THE CLASS THAT IS THE STREAM Listener OF THE HTML OUPUT
// FROM LIBMIME. THIS IS FOR QUOTING
////////////////////////////////////////////////////////////////////////////////////
class QuotingOutputStreamListener : public nsIStreamListener
{
public:
    QuotingOutputStreamListener(const PRUnichar *originalMsgURI,
                                PRBool quoteHeaders,
                                nsIMsgIdentity *identity);
    virtual ~QuotingOutputStreamListener(void);

    NS_DECL_ISUPPORTS
    NS_DECL_NSISTREAMOBSERVER
    NS_DECL_NSISTREAMLISTENER

    NS_IMETHOD  SetComposeObj(nsMsgCompose *obj);
	  NS_IMETHOD  ConvertToPlainText();
	  NS_IMETHOD	SetMimeHeaders(nsIMimeHeaders * headers);

private:
    nsMsgCompose *				mComposeObj;
    nsString       				mMsgBody;
    PRBool						mQuoteHeaders;
    nsCOMPtr<nsIMimeHeaders>	mHeaders;
    nsCOMPtr<nsIMsgIdentity> mIdentity;
};

////////////////////////////////////////////////////////////////////////////////////
// This is the listener class for the send operation. We have to create this class 
// to listen for message send completion and eventually notify the caller
////////////////////////////////////////////////////////////////////////////////////
class nsMsgComposeSendListener : public nsIMsgSendListener, public nsIMsgCopyServiceListener
{
public:
  nsMsgComposeSendListener(void);
  virtual ~nsMsgComposeSendListener(void);

  // nsISupports interface
  NS_DECL_ISUPPORTS

  /* void OnStartSending (in string aMsgID, in PRUint32 aMsgSize); */
  NS_IMETHOD OnStartSending(const char *aMsgID, PRUint32 aMsgSize);
  
  /* void OnProgress (in string aMsgID, in PRUint32 aProgress, in PRUint32 aProgressMax); */
  NS_IMETHOD OnProgress(const char *aMsgID, PRUint32 aProgress, PRUint32 aProgressMax);
  
  /* void OnStatus (in string aMsgID, in wstring aMsg); */
  NS_IMETHOD OnStatus(const char *aMsgID, const PRUnichar *aMsg);
  
  /* void OnStopSending (in string aMsgID, in nsresult aStatus, in wstring aMsg, in nsIFileSpec returnFileSpec); */
  NS_IMETHOD OnStopSending(const char *aMsgID, nsresult aStatus, const PRUnichar *aMsg, 
                           nsIFileSpec *returnFileSpec);

  // For the nsIMsgCopySerivceListener!
  NS_IMETHOD OnStartCopy();
  
  NS_IMETHOD OnProgress(PRUint32 aProgress, PRUint32 aProgressMax);
  
  NS_IMETHOD OnStopCopy(nsresult aStatus);
  NS_IMETHOD SetMessageKey(PRUint32 aMessageKey);
  NS_IMETHOD GetMessageId(nsCString* aMessageId);

  NS_IMETHOD SetComposeObj(nsMsgCompose *obj);
	NS_IMETHOD SetDeliverMode(MSG_DeliverMode deliverMode);
	nsIMsgSendListener **CreateListenerArray();

private:

  nsMsgCompose    *mComposeObj;
	MSG_DeliverMode mDeliverMode;
};

////////////////////////////////////////////////////////////////////////////////////
// This is a class that will allow us to listen to state changes in the Ender 
// compose window. This is important since we must wait until the have this 
////////////////////////////////////////////////////////////////////////////////////

class nsMsgDocumentStateListener : public nsIDocumentStateListener {
public:
  nsMsgDocumentStateListener(void);
  virtual ~nsMsgDocumentStateListener(void);

  // nsISupports interface
  NS_DECL_ISUPPORTS

  NS_IMETHOD  NotifyDocumentCreated(void);
  NS_IMETHOD  NotifyDocumentWillBeDestroyed(void);
  NS_IMETHOD  NotifyDocumentStateChanged(PRBool nowDirty);

  void        SetComposeObj(nsMsgCompose *obj);

  // class vars.
  nsMsgCompose    *mComposeObj;
};


#endif /* _nsMsgCompose_H_ */
