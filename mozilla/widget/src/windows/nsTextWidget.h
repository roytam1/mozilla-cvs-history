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

#include "nsdefs.h"
#include "nsWindow.h"
#include "nsSwitchToUIThread.h"
#include "nsTextHelper.h"

#include "nsITextWidget.h"

/**
 * Native WIN32 single line edit control wrapper. 
 */

class nsTextWidget : public nsTextHelper
{

public:
    nsTextWidget(nsISupports *aOuter);
    virtual ~nsTextWidget();

    // nsISupports. Forward to the nsObject base class
    BASE_SUPPORT

    virtual nsresult  QueryObject(const nsIID& aIID, void** aInstancePtr);
    virtual PRBool  OnPaint();
    virtual PRBool  OnMove(PRInt32 aX, PRInt32 aY);
    virtual PRBool  OnResize(nsRect &aWindowRect);
    virtual void    GetBounds(nsRect &aRect);

    virtual void SubclassWindow(BOOL bState);

    // nsIWidget interface
    BASE_IWIDGET_IMPL

protected:
    virtual LPCTSTR     WindowClass();
    virtual DWORD       WindowStyle();
    virtual DWORD       WindowExStyle();

    static LRESULT CALLBACK TextWindowProc(HWND hWnd,
                                    UINT msg,
                                    WPARAM wParam,
                                    LPARAM lParam);

};

#endif // nsTextWidget_h__
