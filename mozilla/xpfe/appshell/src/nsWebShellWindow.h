/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License.  You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are Copyright (C) 1998
 * Netscape Communications Corporation.  All Rights Reserved.
 */

#ifndef nsWebShellWindow_h__
#define nsWebShellWindow_h__

#include "nsISupports.h"
#include "nsIWebShellWindow.h"
#include "nsGUIEvent.h"
#include "nsIWebShell.h"  
#include "nsIDocumentLoaderObserver.h"
#include "nsVoidArray.h"

// can't use forward class decl's because of template bugs on Solaris 
#include "nsIDOMDocument.h"
#include "nsIDOMNode.h"
#include "nsIXULCommand.h"

#include "nsCOMPtr.h"

/* Forward declarations.... */
class nsIURL;
class nsIAppShell;
class nsIWidget;
class nsIWidgetController;
class nsIDOMCharacterData;
class nsIDOMHTMLInputElement;
class nsIDOMHTMLImageElement;
class nsIDOMElement;
class nsIStreamObserver;
class nsIDocument;

class nsWebShellWindow : public nsIWebShellWindow,
                         public nsIWebShellContainer,
                         public nsIDocumentLoaderObserver
{
public:
  nsWebShellWindow();

  // nsISupports interface...
  NS_DECL_ISUPPORTS

  // nsIWebShellContainer interface...
  NS_IMETHOD WillLoadURL(nsIWebShell* aShell,
                         const PRUnichar* aURL,
                         nsLoadType aReason);

  NS_IMETHOD BeginLoadURL(nsIWebShell* aShell,
                          const PRUnichar* aURL);

  NS_IMETHOD ProgressLoadURL(nsIWebShell* aShell,
                             const PRUnichar* aURL,
                             PRInt32 aProgress,
                             PRInt32 aProgressMax);

  NS_IMETHOD EndLoadURL(nsIWebShell* aShell,
                        const PRUnichar* aURL,
                        PRInt32 aStatus);

  NS_IMETHOD NewWebShell(PRUint32 aChromeMask,
                         PRBool aVisible,
                         nsIWebShell *&aNewWebShell);

  NS_IMETHOD FindWebShellWithName(const PRUnichar* aName,
                                  nsIWebShell*& aResult);

  NS_IMETHOD FocusAvailable(nsIWebShell* aFocusedWebShell);


  // nsIWebShellWindow methods...
  NS_IMETHOD Show(PRBool aShow);
  NS_IMETHOD GetWebShell(nsIWebShell *& aWebShell);
  NS_IMETHOD GetWidget(nsIWidget *& aWidget);

  // nsWebShellWindow methods...
  nsresult Initialize(nsIAppShell* aShell, nsIURL* aUrl, nsString& aControllerIID, nsIStreamObserver* anObserver,
                      PRInt32 aInitialWidth, PRInt32 aInitialHeight);
  nsresult Initialize(nsIWidget * aParent, nsIAppShell* aShell, nsIURL* aUrl, nsString& aControllerIID, nsIStreamObserver* anObserver,
                      PRInt32 aInitialWidth, PRInt32 aInitialHeight);
  nsIWidget* GetWidget(void) { return mWindow; }

  // nsIDocumentLoaderObserver
  NS_IMETHOD OnStartURLLoad(nsIURL* aURL, const char* aContentType, nsIContentViewer* aViewer);
  NS_IMETHOD OnConnectionsComplete();

protected:
  void ExecuteJavaScriptString(nsString& aJavaScript);

  PRInt32 GetDocHeight(nsIDocument * aDoc);
 
  void LoadMenus(nsIDOMDocument * aDOMDoc, nsIWidget * aParentWindow);
  nsCOMPtr<nsIDOMNode>     FindNamedParentFromDoc(nsIDOMDocument * aDomDoc, const nsString &aName);
  nsCOMPtr<nsIDOMNode>     FindNamedDOMNode(const nsString &aName, nsIDOMNode * aParent, PRInt32 & aCount, PRInt32 aEndCount);
  nsCOMPtr<nsIDOMDocument> GetNamedDOMDoc(const nsString & aWebShellName);
  nsCOMPtr<nsIDOMNode>     GetParentNodeFromDOMDoc(nsIDOMDocument * aDOMDoc);

  nsCOMPtr<nsIDOMNode>     GetDOMNodeFromWebShell(nsIWebShell *aShell);
  void                     ExecuteStartupCode();
  

  virtual ~nsWebShellWindow();

  static nsEventStatus PR_CALLBACK HandleEvent(nsGUIEvent *aEvent);

  nsIWidget*              mWindow;
  nsIWebShell*            mWebShell;
  nsIWidgetController*    mController;
  nsIDOMCharacterData*    mStatusText;
  nsIDOMHTMLInputElement* mURLBarText;
  nsIDOMHTMLImageElement* mThrobber;

  nsVoidArray mMenuDelegates;
};


#endif /* nsWebShellWindow_h__ */
