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

#ifndef nsTextWidget_h__
#define nsTextWidget_h__

#include "nsWindow.h"
#include "nsTextHelper.h"
#include "nsITextWidget.h"

typedef struct _PasswordData {
  nsString mPassword;
  Boolean  mIgnore;
} PasswordData;

/**
 * Native Mac single line edit control wrapper. 
 */

class nsTextWidget : public nsWindow, public nsITextWidget
{

public:
  nsTextWidget();
  virtual ~nsTextWidget();

	NS_DECL_ISUPPORTS

  NS_IMETHOD Create(nsIWidget *aParent,
              const nsRect &aRect,
              EVENT_CALLBACK aHandleEventFunction,
              nsIDeviceContext *aContext = nsnull,
              nsIAppShell *aAppShell = nsnull,
              nsIToolkit *aToolkit = nsnull,
              nsWidgetInitData *aInitData = nsnull);

  NS_IMETHOD Create(nsNativeWidget aParent,
              const nsRect &aRect,
              EVENT_CALLBACK aHandleEventFunction,
              nsIDeviceContext *aContext = nsnull,
              nsIAppShell *aAppShell = nsnull,
              nsIToolkit *aToolkit = nsnull,
              nsWidgetInitData *aInitData = nsnull);

	// nsITextWidget interface
  NS_IMETHOD        SelectAll();
  NS_IMETHOD        SetMaxTextLength(PRUint32 aChars);
  NS_IMETHOD        GetText(nsString& aTextBuffer, PRUint32 aBufferSize, PRUint32& aActualSize);
  NS_IMETHOD        SetText(const nsString &aText, PRUint32& aActualSize);
  NS_IMETHOD        InsertText(const nsString &aText, PRUint32 aStartPos, PRUint32 aEndPos, PRUint32& aActualSize);
  NS_IMETHOD        RemoveText();
  NS_IMETHOD        SetPassword(PRBool aIsPassword);
  NS_IMETHOD        SetReadOnly(PRBool aNewReadOnlyFlag, PRBool& aOldReadOnlyFlag);
  NS_IMETHOD        SetSelection(PRUint32 aStartSel, PRUint32 aEndSel);
  NS_IMETHOD        GetSelection(PRUint32 *aStartSel, PRUint32 *aEndSel);
  NS_IMETHOD        SetCaretPosition(PRUint32 aPosition);
  NS_IMETHOD        GetCaretPosition(PRUint32& aPosition);
  NS_IMETHOD        Resize(PRUint32 aWidth,PRUint32 aHeight, PRBool aRepaint);
  NS_IMETHOD        Resize(PRUint32 aX, PRUint32 aY,PRUint32 aWidth,PRUint32 aHeight, PRBool aRepaint);


  virtual PRBool  OnPaint(nsPaintEvent & aEvent);
  virtual PRBool  OnResize(nsSizeEvent &aEvent);

  // nsTextHelper Interface
  virtual PRBool    AutoErase();
  virtual PRBool 		DispatchMouseEvent(nsMouseEvent &aEvent);

  void							PrimitiveKeyDown(PRInt16	aKey,PRInt16 aModifiers);

protected:
    PRBool        mIsPasswordCallBacksInstalled;

private:
  PRBool 			mMakeReadOnly;
  PRBool 			mMakePassword;
  WEReference	mTE_Data;
};

#endif // nsTextWidget_h__
