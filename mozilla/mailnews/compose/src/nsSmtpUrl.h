/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

#ifndef nsSmtpUrl_h__
#define nsSmtpUrl_h__

#include "nsISmtpUrl.h"
#include "nsINetlibURL.h" /* this should be temporary until Network N2 project lands */
#include "nsIUrlListenerManager.h"

class nsSmtpUrl : public nsISmtpUrl, public nsINetlibURL
{
public:
    // from nsIURL:

	// mscott: some of these we won't need to implement..as part of the netlib re-write we'll be removing them
	// from nsIURL and then we can remove them from here as well....
    NS_IMETHOD_(PRBool) Equals(const nsIURL *aURL) const;
    NS_IMETHOD GetSpec(const char* *result) const;
    NS_IMETHOD SetSpec(const char* spec);
    NS_IMETHOD GetProtocol(const char* *result) const;
    NS_IMETHOD SetProtocol(const char* protocol);
    NS_IMETHOD GetHost(const char* *result) const;
    NS_IMETHOD SetHost(const char* host);
    NS_IMETHOD GetHostPort(PRUint32 *result) const;
    NS_IMETHOD SetHostPort(PRUint32 port);
    NS_IMETHOD GetFile(const char* *result) const;
    NS_IMETHOD SetFile(const char* file);
    NS_IMETHOD GetRef(const char* *result) const;
    NS_IMETHOD SetRef(const char* ref);
    NS_IMETHOD GetSearch(const char* *result) const;
    NS_IMETHOD SetSearch(const char* search);
    NS_IMETHOD GetContainer(nsISupports* *result) const;
    NS_IMETHOD SetContainer(nsISupports* container);	
    NS_IMETHOD GetLoadAttribs(nsILoadAttribs* *result) const;	// make obsolete
    NS_IMETHOD SetLoadAttribs(nsILoadAttribs* loadAttribs);	// make obsolete
    NS_IMETHOD GetURLGroup(nsIURLGroup* *result) const;	// make obsolete
    NS_IMETHOD SetURLGroup(nsIURLGroup* group);	// make obsolete
    NS_IMETHOD SetPostHeader(const char* name, const char* value);	// make obsolete
    NS_IMETHOD SetPostData(nsIInputStream* input);	// make obsolete
    NS_IMETHOD GetContentLength(PRInt32 *len);
    NS_IMETHOD GetServerStatus(PRInt32 *status);  // make obsolete
    NS_IMETHOD ToString(PRUnichar* *aString) const;

	NS_IMETHOD SetUrlState(PRBool aRunningUrl, nsresult aStatusCode);
	NS_IMETHOD GetUrlState(PRBool * aRunningUrl);

    // from nsINetlibURL:

    NS_IMETHOD GetURLInfo(URL_Struct_ **aResult) const;
    NS_IMETHOD SetURLInfo(URL_Struct_ *URL_s);

	// From nsISmtpUrl
	// mscott: I used to have individual getters for ALL of these fields but it was
	// getting way out of hand...besides in the actual protocol, we want all of these
	// fields anyway so why go through the extra step of making the protocol call
	// 12 get functions...
	NS_IMETHOD GetMessageContents(const char ** aToPart, const char ** aCcPart, const char ** aBccPart, 
		const char ** aFromPart, const char ** aFollowUpToPart, const char ** aOrganizationPart, 
		const char ** aReplyToPart, const char ** aSubjectPart, const char ** aBodyPart, const char ** aHtmlPart, 
		const char ** aReferencePart, const char ** aAttachmentPart, const char ** aPriorityPart, 
		const char ** aNewsgroupPart, const char ** aNewsHostPart, PRBool * aforcePlainText);

	// Caller must call PR_FREE on list when it is done with it. This list is a list of all
	// recipients to send the email to. each name is NULL terminated...
	NS_IMETHOD GetAllRecipients(char ** aRecipientsList);

	// is the url a post message url or a bring up the compose window url? 
	NS_IMETHOD IsPostMessage(PRBool * aPostMessage); 
	
	// used to set the url as a post message url...
	NS_IMETHOD SetPostMessage(PRBool aPostMessage);

	// the message can be stored in a file....allow accessors for getting and setting
	// the file name to post...
	NS_IMETHOD SetPostMessageFile(const nsFilePath& aFileName);
	NS_IMETHOD GetPostMessageFile(const nsFilePath ** aFileName);

	/////////////////////////////////////////////////////////////////////////////// 
	// SMTP Url instance specific getters and setters --> info the protocol needs
	// to know in order to run the url...these are NOT event sinks which are things
	// the caller needs to know...
	///////////////////////////////////////////////////////////////////////////////

	// mscott -- when we have identities it would be nice to just have an identity 
	// interface here that would encapsulte things like username, domain, password,
	// etc...
	NS_IMETHOD GetUserEmailAddress(const char ** aUserName);
	NS_IMETHOD SetUserEmailAddress(const nsString& aUserName);
	NS_IMETHOD GetUserPassword(const nsString ** aUserPassword);
	NS_IMETHOD SetUserPassword(const nsString& aUserPassword);
	
	// nsIMsgMailNewsUrl
	NS_IMETHOD SetErrorMessage (char * errorMessage);
	// caller must free using PR_FREE
	NS_IMETHOD GetErrorMessage (char ** errorMessage) const;
	
	NS_IMETHOD RegisterListener (nsIUrlListener * aUrlListener);
	NS_IMETHOD UnRegisterListener (nsIUrlListener * aUrlListener);

    // nsSmtpUrl

    nsSmtpUrl(nsISupports* aContainer, nsIURLGroup* aGroup);

    NS_DECL_ISUPPORTS

	// protocol specific code to parse a url...
    nsresult ParseURL(const nsString& aSpec, const nsIURL* aURL = nsnull);

protected:
    virtual ~nsSmtpUrl();

    /* Here's our link to the netlib world.... */
    URL_Struct *m_URL_s;

	// data retrieved from parsing the url: (Note the url could be a post from file or it could be inthe url)
    char		*m_toPart;
	char		*m_ccPart;
	char		*m_subjectPart;
	char		*m_newsgroupPart;
	char		*m_newsHostPart;
	char		*m_referencePart;
	char		*m_attachmentPart;
	char	    *m_bodyPart;
	char		*m_bccPart;
	char		*m_followUpToPart;
	char		*m_fromPart;
	char		*m_htmlPart;
	char		*m_organizationPart;
	char		*m_replyToPart;
	char		*m_priorityPart;

	char		*m_spec;
    char		*m_protocol;
    char		*m_host;
    char		*m_file;
    char		*m_ref;
    char		*m_search;
	char		*m_errorMessage;

	PRBool	    m_forcePlainText;
	PRBool		m_runningUrl;
    
	PRInt32 m_port;
    nsISupports*    m_container;

	// manager of all of current url listeners....
	nsIUrlListenerManager * m_urlListeners;

	/* Smtp specific event sinks */
	nsString	m_userPassword;
	char	   *m_userNameString; // char * version of m_userName
	nsString	m_userName;

	nsFilePath  m_fileName;

	void ReconstructSpec(void);
	// it is possible to encode the message to parse in the form of a url.
	// This function is used to decompose the search and path part into the bare
	// message components (to, fcc, bcc, etc.)
	nsresult ParseMessageToPost(char * searchPart);
	// generic function to clear out our local url state...
	nsresult CleanupSmtpState();
};

#endif // nsSmtpUrl_h__
