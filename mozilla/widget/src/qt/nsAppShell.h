/* -*- Mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

#ifndef nsAppShell_h__
#define nsAppShell_h__

#include "nsIAppShell.h"
#include "plevent.h"

#include "nsQApplication.h"
#include <qwindowsstyle.h>

/**
 * Native QT Application shell wrapper
 */

class nsIEventQueueService;

class nsAppShell : public nsIAppShell
{
public:
    nsAppShell();
    virtual ~nsAppShell();

    NS_DECL_ISUPPORTS

    // nsIAppShellInterface
    NS_IMETHOD		Create(int* argc, char ** argv);
    NS_IMETHOD		Run(); 
    NS_IMETHOD      Spinup();
    NS_IMETHOD      Spindown();
    NS_IMETHOD      ListenToEventQueue(nsIEventQueue *aQueue, PRBool aListen)
                      { return NS_OK; }
    NS_IMETHOD      GetNativeEvent(PRBool &aRealEvent, void *&aEvent);
    NS_IMETHOD      DispatchNativeEvent(PRBool aRealEvent, void * aEvent);
    NS_IMETHOD      EventIsForModalWindow(PRBool aRealEvent, 
                                          void *aEvent,
                                          nsIWidget *aWidget, 
                                          PRBool *aForWindow);
    NS_IMETHOD		Exit();
    NS_IMETHOD		SetDispatchListener(nsDispatchListener* aDispatchListener);
    virtual void*	GetNativeData(PRUint32 aDataType);

    enum GfxToolkit
    {
        eInvalidGfxToolkit = 0,
        eQtGfxToolkit      = 1,
        eXlibGfxToolkit    = 2
    };

    static GfxToolkit GetGfxToolkit();

protected:
    static GfxToolkit      mGfxToolkit;

private:
    nsDispatchListener	 * mDispatchListener;
    nsQApplication       * mApplication;
};

#endif // nsAppShell_h__

