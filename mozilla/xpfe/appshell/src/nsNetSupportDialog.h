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
#ifndef _nsNetSupportDialog_h
#define _nsNetSupportDialog_h

#include  "nsString.h"
#include "nsIXULWindowCallbacks.h"
#include "nsIDOMMouseListener.h"
#include "nsINetSupportDialogService.h"
class nsIWebShell;
class nsIWebShellWindow;
class nsIDOMElement;
#ifdef NECKO
class nsNetSupportDialog  : public nsIPrompt, public nsIXULWindowCallbacks , public nsIDOMMouseListener
#else
class nsNetSupportDialog  : public nsINetSupportDialogService, public nsIXULWindowCallbacks , public nsIDOMMouseListener
#endif
{
public:
  nsNetSupportDialog();
  virtual ~nsNetSupportDialog();

#ifdef NECKO
  // nsIPrompt
  NS_IMETHOD Alert(const PRUnichar *text);
  NS_IMETHOD Confirm(const PRUnichar *text, PRBool *_retval);
  NS_IMETHOD ConfirmCheck(const PRUnichar *text, const PRUnichar *checkMsg, PRBool *checkValue, PRBool *_retval);
  NS_IMETHOD Prompt(const PRUnichar *text, const PRUnichar *defaultText, PRUnichar **result, PRBool *_retval);
  NS_IMETHOD PromptUsernameAndPassword(const PRUnichar *text, PRUnichar **user, PRUnichar **pwd, PRBool *_retval);
  NS_IMETHOD PromptPassword(const PRUnichar *text, PRUnichar **pwd, PRBool *_retval);
#else
	NS_IMETHOD 	Alert( const nsString &aText );
  NS_IMETHOD 	Confirm( const nsString &aText,PRInt32* returnValue );
	NS_IMETHOD	ConfirmCheck( const nsString &aText, const nsString& aCheckMsg, PRInt32* returnValue, PRBool* checkValue );
  NS_IMETHOD 	Prompt(	const nsString &aText,
                      const nsString &aDefault,
                      nsString &aResult,PRInt32* returnValue );

  NS_IMETHOD 	PromptUserAndPassword(  const nsString &aText,
                                      nsString &aUser,
                                      nsString &aPassword, PRInt32* returnValue );

  NS_IMETHOD PromptPassword( 	const nsString &aText, nsString &aPassword,PRInt32* returnValue );
#endif

  // COM
	NS_DECL_ISUPPORTS
protected:
	// nsIDOMMouseListener
	// Currently only care about mouse click;
	virtual nsresult HandleEvent(nsIDOMEvent* aEvent){ return NS_OK; }
	virtual nsresult MouseDown(nsIDOMEvent* aMouseEvent) { return NS_OK; }
  virtual nsresult MouseUp(nsIDOMEvent* aMouseEvent) { return NS_OK; }
  virtual nsresult MouseClick(nsIDOMEvent* aMouseEvent);
  virtual nsresult MouseDblClick(nsIDOMEvent* aMouseEvent) { return NS_OK; }
  virtual nsresult MouseOver(nsIDOMEvent* aMouseEvent) { return NS_OK; }
  virtual nsresult MouseOut(nsIDOMEvent* aMouseEvent) { return NS_OK; }
private:	
	void Init();
	
	void AddMouseEventListener(nsIDOMNode* aNode );
	void RemoveEventListener(nsIDOMNode * aNode);
	void OnOK();
	void OnCancel();
	nsresult   DoDialog(  nsString& inXULURL  );
	NS_IMETHOD ConstructBeforeJavaScript(nsIWebShell *aWebShell);
	NS_IMETHOD ConstructAfterJavaScript(nsIWebShell *aWebShell);
	const nsString*	mDefault;
	nsString*	mUser;
	nsString*	mPassword;
	const nsString*	mMsg;
	const nsString* mCheckMsg;
	nsIWebShell*	mWebShell;
	nsIWebShellWindow* mWebShellWindow;
	PRInt32*			mReturnValue;
	PRBool*			mCheckValue;
	nsIDOMElement* mOKButton;
  nsIDOMElement* mCancelButton;
};
#endif
