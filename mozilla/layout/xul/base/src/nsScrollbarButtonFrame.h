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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

/**

  Eric D Vaughan
  This class lays out its children either vertically or horizontally
 
**/

#ifndef nsScrollbarButtonFrame_h___
#define nsScrollbarButtonFrame_h___

#include "nsTitledButtonFrame.h"
#include "nsITimerCallback.h"

class nsSliderFrame;

class nsScrollbarButtonFrame : public nsTitledButtonFrame, 
                               public nsITimerCallback
{
public:

  // Overrides
  NS_IMETHOD Destroy(nsIPresContext* aPresContext);

  friend nsresult NS_NewScrollBarButtonFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame);

  NS_IMETHOD HandleEvent(nsIPresContext* aPresContext, 
                         nsGUIEvent* aEvent,
                         nsEventStatus* aEventStatus);


  static nsresult GetChildWithTag(nsIAtom* atom, nsIFrame* start, nsIFrame*& result);
  static nsresult GetParentWithTag(nsIAtom* atom, nsIFrame* start, nsIFrame*& result);

   NS_IMETHOD HandlePress(nsIPresContext* aPresContext,
                         nsGUIEvent *    aEvent,
                         nsEventStatus*  aEventStatus);

  NS_IMETHOD HandleMultiplePress(nsIPresContext* aPresContext,
                         nsGUIEvent *    aEvent,
                         nsEventStatus*  aEventStatus)  { return NS_OK; }

  NS_IMETHOD HandleDrag(nsIPresContext* aPresContext,
                        nsGUIEvent *    aEvent,
                        nsEventStatus*  aEventStatus) { return NS_OK; }

  NS_IMETHOD HandleRelease(nsIPresContext* aPresContext,
                           nsGUIEvent *    aEvent,
                           nsEventStatus*  aEventStatus);

  virtual void Notify(nsITimer *timer);

  NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr);
  NS_IMETHOD_(nsrefcnt) AddRef(void) { return NS_OK; }
  NS_IMETHOD_(nsrefcnt) Release(void) { return NS_OK; }


protected:
  virtual void MouseClicked(nsIPresContext* aPresContext);
  virtual void MouseClicked();

  
}; // class nsTabFrame



#endif

