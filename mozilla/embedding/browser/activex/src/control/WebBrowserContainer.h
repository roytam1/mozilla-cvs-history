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
 * Author:
 *   Adam Lock <adamlock@netscape.com>
 *
 * Contributor(s): 
 */
#ifndef WEBBROWSERCONTAINER_H
#define WEBBROWSERCONTAINER_H

#include "nsIContextMenuListener.h"

// This is the class that handles the XPCOM side of things, callback
// interfaces into the web shell and so forth.

class CWebBrowserContainer :
		public nsIBaseWindow,
		public nsIWebBrowserChrome,
		public nsIWebProgressListener,
		public nsIStreamObserver,
		public nsIURIContentListener,
		public nsIDocumentLoaderObserver,
		public nsIDocShellTreeOwner,
		public nsIInterfaceRequestor,
		public nsIPrompt,
        public nsIContextMenuListener
{
public:
	CWebBrowserContainer(CMozillaBrowser *pOwner);

	friend CMozillaBrowser;

protected:
	virtual ~CWebBrowserContainer();

// Protected members
protected:
	CMozillaBrowser *m_pOwner;
	nsString m_sTitle;
	nsIURI *m_pCurrentURI;
	CDWebBrowserEvents1 *m_pEvents1;
	CDWebBrowserEvents2 *m_pEvents2;


public:
	NS_DECL_ISUPPORTS
	NS_DECL_NSIBASEWINDOW
	NS_DECL_NSIWEBBROWSERCHROME
	NS_DECL_NSIDOCSHELLTREEOWNER
	NS_DECL_NSIURICONTENTLISTENER
	NS_DECL_NSISTREAMOBSERVER
	NS_DECL_NSIDOCUMENTLOADEROBSERVER
	NS_DECL_NSIINTERFACEREQUESTOR
	NS_DECL_NSIWEBPROGRESSLISTENER
    NS_DECL_NSICONTEXTMENULISTENER

	// "Services" accessed through nsIInterfaceRequestor
	NS_DECL_NSIPROMPT
};

#endif

