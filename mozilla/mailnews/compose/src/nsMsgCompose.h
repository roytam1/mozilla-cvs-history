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

#ifndef _nsMsgCompose_H_
#define _nsMsgCompose_H_

#include "nsIMsgCompose.h"
#include "nsCOMPtr.h"
#include "nsWeakReference.h"
#include "nsMsgCompFields.h"
#include "nsIOutputStream.h"
#include "nsIMsgQuote.h"
#include "nsIMsgSendListener.h"
#include "nsIMsgCopyServiceListener.h"
#include "nsIMsgSend.h"
#include "nsIStreamListener.h"
#include "nsIMimeHeaders.h"
#include "nsIBaseWindow.h"
#include "nsIAbDirectory.h"
#include "nsIAbCard.h"
#include "nsIWebProgressListener.h"

// Forward declares
class QuotingOutputStreamListener;
class nsMsgComposeSendListener;
class nsMsgDocumentStateListener;

class nsMsgCompose : public nsIMsgCompose, public nsSupportsWeakReference
{
 public: 

	nsMsgCompose();
	virtual ~nsMsgCompose();

	/* this macro defines QueryInterface, AddRef and Release for this class */
	NS_DECL_ISUPPORTS

	/*** nsIMsgCompose pure virtual functions */
	NS_DECL_NSIMSGCOMPOSE

private:

 // Deal with quoting issues...
	nsresult                      QuoteOriginalMessage(const char * originalMsgURI, PRInt32 what); // New template
  nsresult                      SetQuotingToFollow(PRBool aVal);
  nsresult                      ConvertHTMLToText(nsFileSpec& aSigFile, nsString &aSigData);
  nsresult                      ConvertTextToHTML(nsFileSpec& aSigFile, nsString &aSigData);

  nsCString                     mQuoteURI;
  nsCString                     mOriginalMsgURI; // used so we can mark message disposition flags after we send the message

  PRInt32                       mWhatHolder;

  nsresult                      LoadDataFromFile(nsFileSpec& fSpec, nsString &sigData);

/*
  nsresult                      GetCompFields(nsMsgCompFields **aCompFields) 
                                {
                                  if (aCompFields)
                                    *aCompFields = m_compFields;
                                  return NS_OK;
                                }
 */
  nsresult                      GetIdentity(nsIMsgIdentity **aIdentity)
                                {
                                  *aIdentity = m_identity;
                                  return NS_OK;
                                }
  //m_folderName to store the value of the saved drafts folder.
  nsCString                     m_folderName;

 private:
	nsresult _SendMsg(MSG_DeliverMode deliverMode, nsIMsgIdentity *identity, PRBool entityConversionDone);
	nsresult CreateMessage(const char * originalMsgURI, MSG_ComposeType type, MSG_ComposeFormat format, nsIMsgCompFields* compFields);
	void CleanUpRecipients(nsString& recipients);
  nsresult GetABDirectories(const char * dirUri, nsISupportsArray* directoriesArray, PRBool searchSubDirectory);
  nsresult BuildMailListArray(nsIAddrDatabase* database, nsIAbDirectory* parentDir, nsISupportsArray* array);
  nsresult GetMailListAddresses(nsString& name, nsISupportsArray* mailListArray, nsISupportsArray** addresses);
  nsresult TagConvertible(nsIDOMNode *node,  PRInt32 *_retval);
  nsresult _BodyConvertible(nsIDOMNode *node, PRInt32 *_retval);
 
       // Helper function. Parameters are not checked.
  PRBool mConvertStructs;  // for TagConvertible
  
	nsIEditorShell                    *m_editor;
	nsIDOMWindowInternal              *m_window;
  nsCOMPtr<nsIBaseWindow>           m_baseWindow;
	nsMsgCompFields                   *m_compFields;
	nsCOMPtr<nsIMsgIdentity>          m_identity;
	PRBool						                m_composeHTML;
	QuotingOutputStreamListener       *mQuoteStreamListener;
	nsCOMPtr<nsIOutputStream>         mBaseStream;

	nsCOMPtr<nsIMsgSend>              mMsgSend;   // for composition back end
	nsCOMPtr<nsIMsgProgress>          mProgress;  // use by the back end to report progress to the front end

  // Deal with quoting issues...
  nsString                          mCiteReference;
	nsCOMPtr<nsIMsgQuote>             mQuote;
	PRBool						                mQuotingToFollow; // Quoting indicator
	nsMsgDocumentStateListener        *mDocumentListener;
	MSG_ComposeType                   mType;		//Message type
  nsCOMPtr<nsISupportsArray>        mStateListeners;		// contents are nsISupports
    
  friend class QuotingOutputStreamListener;
	friend class nsMsgDocumentStateListener;
	friend class nsMsgComposeSendListener;
};

////////////////////////////////////////////////////////////////////////////////////
// THIS IS THE CLASS THAT IS THE STREAM Listener OF THE HTML OUPUT
// FROM LIBMIME. THIS IS FOR QUOTING
////////////////////////////////////////////////////////////////////////////////////
class QuotingOutputStreamListener : public nsIStreamListener
{
public:
    QuotingOutputStreamListener(const char *originalMsgURI,
                                PRBool quoteHeaders,
                                PRBool headersOnly,
                                nsIMsgIdentity *identity);
    virtual ~QuotingOutputStreamListener(void);

    NS_DECL_ISUPPORTS
    NS_DECL_NSIREQUESTOBSERVER
    NS_DECL_NSISTREAMLISTENER

    NS_IMETHOD  SetComposeObj(nsIMsgCompose *obj);
	  NS_IMETHOD  ConvertToPlainText(PRBool formatflowed = PR_FALSE);
	  NS_IMETHOD	SetMimeHeaders(nsIMimeHeaders * headers);

private:
    nsWeakPtr                 mWeakComposeObj;
    nsString       				    mMsgBody;
    nsString       				    mCitePrefix;
    nsString       				    mSignature;
    PRBool						        mQuoteHeaders;
    PRBool						        mHeadersOnly;
    nsCOMPtr<nsIMimeHeaders>	mHeaders;
    nsCOMPtr<nsIMsgIdentity>  mIdentity;
    nsString                  mCiteReference;
};

////////////////////////////////////////////////////////////////////////////////////
// This is the listener class for the send operation. We have to create this class 
// to listen for message send completion and eventually notify the caller
////////////////////////////////////////////////////////////////////////////////////
class nsMsgComposeSendListener : public nsIMsgComposeSendListener, public nsIMsgSendListener, public nsIMsgCopyServiceListener, public nsIWebProgressListener
{
public:
  nsMsgComposeSendListener(void);
  virtual ~nsMsgComposeSendListener(void);

  // nsISupports interface
  NS_DECL_ISUPPORTS

  // nsIMsgComposeSendListener interface
  NS_DECL_NSIMSGCOMPOSESENDLISTENER

  // nsIMsgSendListener interface
  NS_DECL_NSIMSGSENDLISTENER
  
  // nsIMsgCopyServiceListener interface
  NS_DECL_NSIMSGCOPYSERVICELISTENER
  
	// nsIWebProgressListener interface
	NS_DECL_NSIWEBPROGRESSLISTENER

  nsresult    RemoveCurrentDraftMessage(nsIMsgCompose *compObj, PRBool calledByCopy);
  nsresult    GetMsgFolder(nsIMsgCompose *compObj, nsIMsgFolder **msgFolder);

private:
  nsWeakPtr               mWeakComposeObj;
	MSG_DeliverMode         mDeliverMode;
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

  void        SetComposeObj(nsIMsgCompose *obj);

  // class vars.
  nsWeakPtr                 mWeakComposeObj;
};

/******************************************************************************
 * nsMsgRecipient
 ******************************************************************************/
class nsMsgRecipient : public nsISupports
{
public:
  nsMsgRecipient();
  nsMsgRecipient(nsString fullAddress, nsString email, PRUint32 preferFormat = nsIAbPreferMailFormat::unknown,
                 PRBool processed = PR_FALSE);
	virtual ~nsMsgRecipient();

  NS_DECL_ISUPPORTS
  
public:
  nsString mAddress;      /* full email address (name + email) */
  nsString mEmail;        /* email address only */
  PRUint32 mPreferFormat; /* nsIAbPreferMailFormat:: unknown, plaintext or html */
  PRBool mProcessed;
};

/******************************************************************************
 * nsMsgMailList
 ******************************************************************************/
class nsMsgMailList : public nsISupports
{
public:
  nsMsgMailList();
  nsMsgMailList(nsString listName, nsString listDescription, nsIAbDirectory* directory);
	virtual ~nsMsgMailList();

  NS_DECL_ISUPPORTS
  
public:
  nsString mFullName;  /* full email address (name + email) */
  nsCOMPtr<nsIAbDirectory> mDirectory;
};

#endif /* _nsMsgCompose_H_ */
