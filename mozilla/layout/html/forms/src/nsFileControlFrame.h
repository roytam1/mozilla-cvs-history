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

#ifndef nsFileControlFrame_h___
#define nsFileControlFrame_h___

#include "nsHTMLContainerFrame.h"
#include "nsIFormControlFrame.h"
class nsButtonControlFrame;
class nsTextControlFrame;
class nsFormFrame;

class nsFileControlFrame : public nsHTMLContainerFrame,
                           public nsIFormControlFrame
{
public:
  nsFileControlFrame(nsIContent* aContent, 
                     nsIFrame* aParentFrame);

  NS_IMETHOD QueryInterface(const nsIID& aIID, void** aInstancePtr);

  NS_IMETHOD Reflow(nsIPresContext&      aCX,
                    nsHTMLReflowMetrics& aDesiredSize,
                    const nsReflowState& aReflowState,
                    nsReflowStatus&      aStatus);

  virtual void MouseClicked(nsIPresContext* aPresContext);

  virtual PRInt32 GetMaxNumValues();

  virtual PRBool GetNamesValues(PRInt32 aMaxNumValues, PRInt32& aNumValues,
                                nsString* aValues, nsString* aNames);

  nsTextControlFrame* GetTextFrame() { return mTextFrame; }

  void SetTextFrame(nsTextControlFrame* aFrame) { mTextFrame = aFrame; }

  nsButtonControlFrame* GetBrowseFrame() { return mBrowseFrame; }
  void  SetBrowseFrame(nsButtonControlFrame* aFrame) { mBrowseFrame = aFrame; }
  NS_IMETHOD GetName(nsString* aName);
  virtual void SetFormFrame(nsFormFrame* aFormFrame) { mFormFrame = aFormFrame; }
  virtual PRBool IsSuccessful();
  virtual void Reset();
  NS_IMETHOD GetType(PRInt32* aType) const;

  //static PRInt32 gSpacing;

protected:
  virtual ~nsFileControlFrame();
  virtual PRIntn GetSkipSides() const;

  nsTextControlFrame*   mTextFrame;
  nsButtonControlFrame* mBrowseFrame; 
  nsFormFrame*          mFormFrame;

private:
  NS_IMETHOD_(nsrefcnt) AddRef() { return NS_OK; }
  NS_IMETHOD_(nsrefcnt) Release() { return NS_OK; }
};

#endif


