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
#ifndef nsSplittableFrame_h___
#define nsSplittableFrame_h___

#include "nsFrame.h"

// Derived class that allows splitting
class nsSplittableFrame : public nsFrame
{
public:
  NS_IMETHOD Init(nsIPresContext&  aPresContext,
                  nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIStyleContext* aContext,
                  nsIFrame*        aPrevInFlow);
  
  NS_IMETHOD  IsSplittable(nsSplittableType& aIsSplittable) const;
#ifdef DEBUG
  NS_IMETHOD  SizeOf(nsISizeOfHandler* aHandler, PRUint32* aResult) const;
#endif

  // Flow member functions.
  NS_IMETHOD  GetPrevInFlow(nsIFrame** aPrevInFlow) const;
  NS_IMETHOD  SetPrevInFlow(nsIFrame*);
  NS_IMETHOD  GetNextInFlow(nsIFrame** aNextInFlow) const;
  NS_IMETHOD  SetNextInFlow(nsIFrame*);

  /**
   * Return the first frame in our current flow. 
   */
  nsIFrame*   GetFirstInFlow() const;

  /**
   * Return the last frame in our current flow.
   */
  nsIFrame*   GetLastInFlow() const;

  // Remove the frame from the flow. Connects the frame's prev-in-flow
  // and its next-in-flow
  static void RemoveFromFlow(nsIFrame* aFrame);
  
  // Detach from previous frame in flow
  static void BreakFromPrevFlow(nsIFrame* aFrame);

  nsIFrame*   GetPrevInFlow();
  nsIFrame*   GetNextInFlow();

protected:
#ifdef DEBUG
  virtual void DumpBaseRegressionData(nsIPresContext* aPresContext, FILE* out, PRInt32 aIndent);
#endif

  nsIFrame*   mPrevInFlow;
  nsIFrame*   mNextInFlow;
};

#endif /* nsSplittableFrame_h___ */
