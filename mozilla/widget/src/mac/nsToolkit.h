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

#ifndef TOOLKIT_H      
#define TOOLKIT_H

#include "nsIToolkit.h"
#include "nsRepeater.h"

class nsIEventQueueService;


/**
 * The toolkit abstraction is necessary because the message pump must
 * execute within the same thread that created the widget under Win32.
 * We don't care about that on Mac: we have only one thread for the UI
 * and maybe even for the whole application.
 * 
 * So on the Mac, the nsToolkit used to be a unique object, created once
 * at startup along with nsAppShell and passed to all the top-level
 * windows and it became a convenient place to throw in everything we
 * didn't know where else to put, like the NSPR event queue and
 * the handling of global pointers on some special widgets (focused
 * widget, widget hit, widget pointed).
 *
 * All this has changed: the application now usually creates one copy of
 * the nsToolkit per window and the special widgets had to be moved
 * to the nsMacEventHandler. Also, to avoid creating several repeaters,
 * the NSPR event queue has been moved to a global object of its own:
 * the nsMacNSPREventQueueHandler declared below.
 *
 * If by any chance we support one day several threads for the UI
 * on the Mac, will have to create one instance of the NSPR
 * event queue per nsToolkit.
 */

class nsToolkit : public nsIToolkit
{

public:
  nsToolkit();
  virtual				~nsToolkit();
  
  NS_DECL_ISUPPORTS
    
  NS_IMETHOD  	Init(PRThread *aThread);
  
	// Appearance Mgr
	static bool 	HasAppearanceManager();
};


class nsMacNSPREventQueueHandler : public Repeater
{
public:
	nsMacNSPREventQueueHandler();
	virtual			~nsMacNSPREventQueueHandler();

	virtual	void		StartPumping();
	virtual	PRBool	StopPumping();

	// Repeater interface
	virtual	void	RepeatAction(const EventRecord& inMacEvent);
	
protected:
	nsrefcnt								mRefCnt;
	nsIEventQueueService* 	mEventQService;
};



#endif  // TOOLKIT_H
