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

#include <Pt.h>
#include <strings.h>
#include "stdhdrs.h"
#include "nsIContextMenuListener.h"
#include "nsIEmbeddingSiteWindow.h"
#include "nsIWebBrowserChromeFocus.h"
#include "nsICommandHandler.h"
#include "nsWeakReference.h"
#include "nsIInputStream.h"
#include "nsILoadGroup.h"
#include "nsIChannel.h"
#include "nsIContentViewer.h"
#include "nsIStreamListener.h"
#include "nsIDocumentLoaderFactory.h"
#include "nsIContentViewerContainer.h"
#include "nsNetUtil.h"
#include "nsIDocShell.h"
#include "nsPIDOMWindow.h"
#include "nsIDOMWindowInternal.h"
#include "nsIChromeEventHandler.h"
#include "nsIURIContentListener.h"

// This is the class that handles the XPCOM side of things, callback
// interfaces into the web shell and so forth.

class CWebBrowserContainer :
		public nsIWebBrowserChrome,
		public nsIWebBrowserChromeFocus,
		public nsIWebProgressListener,
		public nsIEmbeddingSiteWindow,
		public nsIRequestObserver,
		public nsIURIContentListener,
		public nsIDocShellTreeOwner,
		public nsIInterfaceRequestor,
        public nsIContextMenuListener,
        public nsICommandHandler,
        public nsIPrintListener,
				public nsSupportsWeakReference
{
public:
	CWebBrowserContainer(PtWidget_t *pOwner);
	void InvokeInfoCallback(int type, unsigned int status, char *data);
	int InvokeDialogCallback(int type, char *title, char *text, char *msg, int *value);
	void InvokePrintCallback(int status, unsigned int cur, unsigned int max);

	NS_IMETHOD OpenStream( nsIWebBrowser *webBrowser, const char *aBaseURI, const char *aContentType );
	NS_IMETHOD AppendToStream( const char *aData, int32 aLen );
	NS_IMETHOD CloseStream( void );
	NS_IMETHOD IsStreaming( void );

	// this will get the PIDOMWindow for this widget
	nsresult GetPIDOMWindow( nsPIDOMWindow **aPIWin );

	virtual ~CWebBrowserContainer();

	PtWidget_t *m_pOwner;
	PRBool										 mSkipOnState;

// Protected members
protected:
	nsString m_sTitle;
	nsIURI *m_pCurrentURI;


public:
	NS_DECL_ISUPPORTS
	NS_DECL_NSIWEBBROWSERCHROME
	NS_DECL_NSIWEBBROWSERCHROMEFOCUS
	NS_DECL_NSIEMBEDDINGSITEWINDOW
	NS_DECL_NSIDOCSHELLTREEOWNER
	NS_DECL_NSIURICONTENTLISTENER
	NS_DECL_NSIREQUESTOBSERVER
	NS_DECL_NSIINTERFACEREQUESTOR
	NS_DECL_NSIWEBPROGRESSLISTENER
  NS_DECL_NSICONTEXTMENULISTENER
  NS_DECL_NSICOMMANDHANDLER
	NS_DECL_NSIPRINTLISTENER

private:
	nsCOMPtr<nsIInputStream>   	mStream;
	nsCOMPtr<nsIStreamListener>	mStreamListener;
	nsCOMPtr<nsIChannel>       	mChannel;
	PRUint32                   	mOffset;
	PRBool                     	mDoingStream;
};

nsresult InitializeWindowCreator();

#endif
