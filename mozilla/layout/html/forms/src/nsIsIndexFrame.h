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
 * Contributor(s): 
 */

#ifndef nsIsIndexFrame_h___
#define nsIsIndexFrame_h___

#include "nsAreaFrame.h"
#include "nsIFormControlFrame.h"
#include "nsIAnonymousContentCreator.h"
#include "nsIStatefulFrame.h"
#include "nsIUnicodeEncoder.h"
#include "nsIDOMKeyListener.h"

#ifndef ENDER_LITE
#include "nsGfxTextControlFrame.h"
typedef   nsGfxTextControlFrame nsNewFrame;
#else
#include "nsGfxTextControlFrame2.h"
#include "nsFormControlHelper.h"
typedef   nsGfxTextControlFrame2 nsNewFrame;
#endif

class nsIPresState;
class nsISupportsArray;
class nsIHTMLContent;

class nsIsIndexFrame : public nsAreaFrame,
                       public nsIAnonymousContentCreator,
                       public nsIDOMKeyListener,
                       public nsIStatefulFrame
{
public:
  nsIsIndexFrame();
  virtual ~nsIsIndexFrame();

  // XXX Hack so we can squirrel away the pres context pointer for the KeyPress method
  NS_IMETHOD Init(nsIPresContext*  aPresContext,
                  nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIStyleContext* aContext,
                  nsIFrame*        aPrevInFlow) {
    mPresContext = aPresContext;
    return nsAreaFrame::Init(aPresContext, aContent, aParent, aContext, aPrevInFlow);
  }

  /**
   * Processes a key pressed event
   * @param aKeyEvent @see nsIDOMEvent.h 
   * @returns whether the event was consumed or ignored. @see nsresult
   */
  virtual nsresult KeyDown(nsIDOMEvent* aKeyEvent) { return NS_OK; }

  /**
   * Processes a key release event
   * @param aKeyEvent @see nsIDOMEvent.h 
   * @returns whether the event was consumed or ignored. @see nsresult
   */
  virtual nsresult KeyUp(nsIDOMEvent* aKeyEvent) { return NS_OK; }

  /**
   * Processes a key typed event
   * @param aKeyEvent @see nsIDOMEvent.h 
   * @returns whether the event was consumed or ignored. @see nsresult
   *
   */
  virtual nsresult KeyPress(nsIDOMEvent* aKeyEvent); // we only care when a key is pressed

  // nsIFormControlFrame
  NS_IMETHOD QueryInterface(const nsIID& aIID, void** aInstancePtr);

  NS_IMETHOD Reflow(nsIPresContext*          aCX,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);

#ifdef NS_DEBUG
  NS_IMETHOD GetFrameName(nsString& aResult) const;
#endif
  NS_IMETHOD AttributeChanged(nsIPresContext* aPresContext,
                              nsIContent*     aChild,
                              PRInt32         aNameSpaceID,
                              nsIAtom*        aAttribute,
                              PRInt32         aHint);

  void           SetFocus(PRBool aOn, PRBool aRepaint);
  void           ScrollIntoView(nsIPresContext* aPresContext);

  // from nsIAnonymousContentCreator
  NS_IMETHOD CreateAnonymousContent(nsIPresContext* aPresContext,
                                    nsISupportsArray& aChildList);
  NS_IMETHOD CreateFrameFor(nsIPresContext*   aPresContext,
                            nsIContent *      aContent,
                            nsIFrame**        aFrame) { if (aFrame) *aFrame = nsnull; return NS_ERROR_FAILURE; }

  virtual nsresult HandleEvent(nsIDOMEvent* aEvent) { return NS_OK; }

  NS_IMETHOD OnSubmit(nsIPresContext* aPresContext);

  //nsIStatefulFrame
  NS_IMETHOD GetStateType(nsIPresContext* aPresContext, nsIStatefulFrame::StateType* aStateType);
  NS_IMETHOD SaveState(nsIPresContext* aPresContext, nsIPresState** aState);
  NS_IMETHOD RestoreState(nsIPresContext* aPresContext, nsIPresState* aState);

protected:
  nsIHTMLContent*     mTextContent;
  nsIHTMLContent*     mInputContent;

  // XXX Hack: pres context needed by function KeyPress() and SetFocus()
  nsIPresContext*     mPresContext;  // weak reference

private:
  NS_IMETHOD UpdatePromptLabel();
  NS_IMETHOD GetInputFrame(nsIPresContext* aPresContext, nsIFormControlFrame** oFrame);
  NS_IMETHOD GetInputValue(nsIPresContext* aPresContext, nsString& oString);
  NS_IMETHOD SetInputValue(nsIPresContext* aPresContext, const nsString aString);

  void GetSubmitCharset(nsString& oCharset);
  NS_IMETHOD GetEncoder(nsIUnicodeEncoder** encoder);
  char* UnicodeToNewBytes(const PRUnichar* aSrc, PRUint32 aLen, nsIUnicodeEncoder* encoder);
  void URLEncode(const nsString& aString, nsIUnicodeEncoder* encoder, nsString& oString);

  NS_IMETHOD_(nsrefcnt) AddRef() { return NS_OK; }
  NS_IMETHOD_(nsrefcnt) Release() { return NS_OK; }
};

#endif


