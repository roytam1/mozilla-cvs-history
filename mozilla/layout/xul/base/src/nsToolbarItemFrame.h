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
#ifndef nsToolbarItemFrame_h___
#define nsToolbarItemFrame_h___

#include "nsLeafFrame.h"
#include "nsIBox.h"
#include "nsIDOMEventListener.h"
#include "nsIDocument.h"
#include "nsBoxFrame.h"


class nsToolbarItemFrame : public nsBoxFrame
{
public:

  nsToolbarItemFrame();
  //~nsToolbarItemFrame();

  friend nsresult NS_NewToolbarItemFrame(nsIFrame** aNewFrame);

  NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr); 

  NS_IMETHOD_(nsrefcnt) AddRef(void);
  NS_IMETHOD_(nsrefcnt) Release(void);



  NS_IMETHOD  Init(nsIPresContext&  aPresContext,
                   nsIContent*      aContent,
                   nsIFrame*        aParent,
                   nsIStyleContext* aContext,
                   nsIFrame*        asPrevInFlow);

  NS_IMETHOD HandleEvent(nsIPresContext& aPresContext, 
                         nsGUIEvent* aEvent,
                         nsEventStatus& aEventStatus);

 
}; // class nsToolbarItemFrame

#endif /* nsToolbarItemFrame_h___ */
