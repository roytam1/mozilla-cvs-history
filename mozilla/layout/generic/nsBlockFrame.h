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
#ifndef nsBlockFrame_h___
#define nsBlockFrame_h___

#include "nsHTMLContainerFrame.h"

class nsBlockReflowState;
class nsBulletFrame;
class nsLineBox;
class nsTextRun;

/**
 * Child list name indices
 * @see #GetAdditionalChildListName()
 */
#define NS_BLOCK_FRAME_FLOATER_LIST_INDEX 0
#define NS_BLOCK_FRAME_BULLET_LIST_INDEX  1
#define NS_BLOCK_FRAME_LAST_LIST_INDEX    NS_BLOCK_FRAME_BULLET_LIST_INDEX

/**
 * Additional frame-state bits
 */
#define NS_BLOCK_FRAME_HAS_OUTSIDE_BULLET 0x10000

#define nsBlockFrameSuper nsHTMLContainerFrame

// Base class for block and inline frames
class nsBlockFrame : public nsBlockFrameSuper
{
public:
  friend nsresult NS_NewBlockFrame(nsIFrame*& aNewFrame, PRUint32 aFlags);

  // nsISupports
  NS_IMETHOD  QueryInterface(const nsIID& aIID, void** aInstancePtr);

  // nsIFrame
  NS_IMETHOD SetInitialChildList(nsIPresContext& aPresContext,
                                 nsIAtom*        aListName,
                                 nsIFrame*       aChildList);
  NS_IMETHOD  AppendFrames(nsIPresContext& aPresContext,
                           nsIPresShell&   aPresShell,
                           nsIAtom*        aListName,
                           nsIFrame*       aFrameList);
  NS_IMETHOD  InsertFrames(nsIPresContext& aPresContext,
                           nsIPresShell&   aPresShell,
                           nsIAtom*        aListName,
                           nsIFrame*       aPrevFrame,
                           nsIFrame*       aFrameList);
  NS_IMETHOD  RemoveFrame(nsIPresContext& aPresContext,
                          nsIPresShell&   aPresShell,
                          nsIAtom*        aListName,
                          nsIFrame*       aOldFrame);
  NS_IMETHOD FirstChild(nsIAtom* aListName, nsIFrame** aFirstChild) const;
  NS_IMETHOD GetAdditionalChildListName(PRInt32   aIndex,
                                        nsIAtom** aListName) const;
  NS_IMETHOD DeleteFrame(nsIPresContext& aPresContext);
  NS_IMETHOD IsSplittable(nsSplittableType& aIsSplittable) const;
  NS_IMETHOD ReResolveStyleContext(nsIPresContext* aPresContext,
                                   nsIStyleContext* aParentContext);
  NS_IMETHOD CreateContinuingFrame(nsIPresContext&  aPresContext,
                                   nsIFrame*        aParent,
                                   nsIStyleContext* aStyleContext,
                                   nsIFrame*&       aContinuingFrame);
  NS_IMETHOD IsPercentageBase(PRBool& aBase) const;
  NS_IMETHOD Paint(nsIPresContext&      aPresContext,
                   nsIRenderingContext& aRenderingContext,
                   const nsRect&        aDirtyRect,
                   nsFramePaintLayer    aWhichLayer);
  NS_IMETHOD List(FILE* out, PRInt32 aIndent) const;
  NS_IMETHOD GetFrameName(nsString& aResult) const;
  NS_IMETHOD GetFrameType(nsIAtom** aType) const;
  NS_IMETHOD VerifyTree() const;
  NS_IMETHOD GetFrameForPoint(const nsPoint& aPoint, nsIFrame** aFrame);

  // nsIHTMLReflow
  NS_IMETHOD Reflow(nsIPresContext&          aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);
  NS_IMETHOD MoveInSpaceManager(nsIPresContext& aPresContext,
                                nsISpaceManager* aSpaceManager,
                                nscoord aDeltaX, nscoord aDeltaY);

#ifdef DO_SELECTION
  NS_IMETHOD  HandleEvent(nsIPresContext& aPresContext,
                          nsGUIEvent* aEvent,
                          nsEventStatus& aEventStatus);

  NS_IMETHOD  HandleDrag(nsIPresContext& aPresContext, 
                         nsGUIEvent*     aEvent,
                         nsEventStatus&  aEventStatus);

  nsIFrame * FindHitFrame(nsBlockFrame * aBlockFrame, 
                          const nscoord aX, const nscoord aY,
                          const nsPoint & aPoint);

#endif

  virtual void DeleteChildsNextInFlow(nsIPresContext& aPresContext,
                                      nsIFrame* aNextInFlow);

protected:
  nsBlockFrame();
  virtual ~nsBlockFrame();

  void SetFlags(PRUint32 aFlags) {
    mFlags = aFlags;
  }

  PRBool HaveOutsideBullet() const {
    return 0 != (mState & NS_BLOCK_FRAME_HAS_OUTSIDE_BULLET);
  }

  void SlideFrames(nsIPresContext& aPresContext,
                   nsISpaceManager* aSpaceManager,
                   nsLineBox* aLine, nscoord aDY);

  void SlideFloaters(nsIPresContext& aPresContext,
                     nsISpaceManager* aSpaceManager,
                     nsLineBox* aLine, nscoord aDY,
                     PRBool aUpdateSpaceManager);

  PRBool DrainOverflowLines();

  PRBool RemoveChild(nsLineBox* aLines, nsIFrame* aChild);

  virtual PRIntn GetSkipSides() const;

  virtual void ComputeFinalSize(nsBlockReflowState&  aState,
                                nsHTMLReflowMetrics& aMetrics);

  void MarkEmptyLines(nsIPresContext& aPresContext);

  nsresult AppendNewFrames(nsIPresContext& aPresContext, nsIFrame*);

  nsresult InsertNewFrames(nsIPresContext&  aPresContext,
                           nsIFrame* aFrameList,
                           nsIFrame* aPrevSibling);

  nsresult DoRemoveFrame(nsIPresContext& aPresContext,
                         nsIFrame* aDeletedFrame);

  nsresult PrepareInitialReflow(nsBlockReflowState& aState);

  nsresult PrepareFrameAppendedReflow(nsBlockReflowState& aState);

  nsresult PrepareFrameInsertedReflow(nsBlockReflowState& aState);

  nsresult PrepareFrameRemovedReflow(nsBlockReflowState& aState);

  nsresult PrepareStyleChangedReflow(nsBlockReflowState& aState);

  nsresult PrepareChildIncrementalReflow(nsBlockReflowState& aState);

  nsresult PrepareResizeReflow(nsBlockReflowState& aState);

  nsresult ReflowDirtyLines(nsBlockReflowState& aState);

  nsresult RecoverStateFrom(nsBlockReflowState& aState,
                            nsLineBox* aLine,
                            nscoord aDeltaY);

  //----------------------------------------
  // Methods for line reflow
  // XXX nuke em

  void WillReflowLine(nsBlockReflowState& aState,
                      nsLineBox* aLine);

  nsresult ReflowLine(nsBlockReflowState& aState,
                      nsLineBox* aLine,
                      PRBool* aKeepReflowGoing);

  virtual void DidReflowLine(nsBlockReflowState& aState,
                             nsLineBox* aLine,
                             PRBool aKeepReflowGoing);

  nsresult PlaceLine(nsBlockReflowState& aState,
                     nsLineBox* aLine,
                     PRBool* aKeepReflowGoing);

  // XXX blech
  void PostPlaceLine(nsBlockReflowState& aState,
                     nsLineBox* aLine,
                     const nsSize& aMaxElementSize);

  void ComputeLineMaxElementSize(nsBlockReflowState& aState,
                                 nsLineBox* aLine,
                                 nsSize* aMaxElementSize);

  // XXX where to go
  PRBool ShouldJustifyLine(nsBlockReflowState& aState, nsLineBox* aLine);

  void FindFloaters(nsLineBox* aLine);

  void DeleteLine(nsBlockReflowState& aState, nsLineBox* aLine);

  //----------------------------------------
  // Methods for individual frame reflow

  virtual void WillReflowFrame(nsBlockReflowState& aState,
                               nsLineBox* aLine,
                               nsIFrame* aFrame);

  nsresult ReflowBlockFrame(nsBlockReflowState& aState,
                            nsLineBox* aLine,
                            PRBool* aKeepGoing);

  nsresult ReflowInlineFrame(nsBlockReflowState& aState,
                             nsLineBox* aLine,
                             nsIFrame* aFrame,
                             PRBool* aKeepLineGoing);

  void ReflowFloater(nsIPresContext& aPresContext,
                     nsBlockReflowState& aState,
                     nsIFrame* aFloaterFrame,
                     nsHTMLReflowState& aFloaterReflowState);

  //----------------------------------------
  // Methods for pushing/pulling lines/frames

  virtual nsresult CreateContinuationFor(nsBlockReflowState& aState,
                                         nsLineBox* aLine,
                                         nsIFrame* aFrame,
                                         PRBool& aMadeNewFrame);

  nsresult SplitLine(nsBlockReflowState& aState,
                     nsLineBox* aLine,
                     nsIFrame* aFrame);

  nsBlockFrame* FindFollowingBlockFrame(nsIFrame* aFrame);

  nsresult PullFrame(nsBlockReflowState& aState,
                     nsLineBox* aLine,
                     nsIFrame*& aFrameResult);

  nsresult PullFrame(nsBlockReflowState& aState,
                     nsLineBox* aToLine,
                     nsLineBox** aFromList,
                     PRBool aUpdateGeometricParent,
                     nsIFrame*& aFrameResult,
                     PRBool& aStopPulling);

  void PushLines(nsBlockReflowState& aState);

  //----------------------------------------
  //XXX
  virtual void PaintChildren(nsIPresContext& aPresContext,
                             nsIRenderingContext& aRenderingContext,
                             const nsRect& aDirtyRect,
                             nsFramePaintLayer aWhichLayer);

  void PaintFloaters(nsIPresContext& aPresContext,
                     nsIRenderingContext& aRenderingContext,
                     const nsRect& aDirtyRect,
                     nsFramePaintLayer aWhichLayer);

  nsLineBox* FindLineFor(nsIFrame* aFrame, PRBool& aIsFloaterResult);

  void PropogateReflowDamage(nsBlockReflowState& aState,
                             nsLineBox* aLine,
                             nscoord aDeltaY);

  void TakeRunInFrames(nsBlockFrame* aRunInFrame);

  nsresult ComputeTextRuns(nsBlockReflowState& aState);

  void BuildFloaterList();

  void RenumberLists(nsBlockReflowState& aState);

  void ReflowBullet(nsBlockReflowState& aState,
                    nsHTMLReflowMetrics& aMetrics);

  void RestoreStyleFor(nsIPresContext& aPresContext, nsIFrame* aFrame);

#ifdef NS_DEBUG
  PRBool IsChild(nsIFrame* aFrame);
#endif

  nsLineBox* mLines;

  nsLineBox* mOverflowLines;

  // XXX subclass!
  PRUint32 mFlags;

  nsIStyleContext* mFirstLineStyle;

  nsIStyleContext* mFirstLetterStyle;

  // Text run information
  nsTextRun* mTextRuns;

  // List of all floaters in this block
  nsFrameList mFloaters;

  // XXX_fix_me: subclass one more time!
  // For list-item frames, this is the bullet frame.
  nsBulletFrame* mBullet;

  friend class nsBlockReflowState;
};

//----------------------------------------------------------------------

#define nsAnonymousBlockFrameSuper nsBlockFrame

// Anonymous block frame. An anonymous block is used by some other
// container (the parent frame) to provide block reflow for a set of
// child frames. The parent is responsible for the maintainance of the
// anonymous blocks child list. To accomplish this, the normal methods
// for managing the child list (AppendFrames, InsertFrames, and
// RemoveFrame) forward the operation to the parent frame (the
// container of the anonymous block).
class nsAnonymousBlockFrame : public nsAnonymousBlockFrameSuper {
public:
  friend nsresult NS_NewAnonymousBlockFrame(nsIFrame*& aNewFrame);

  // nsIFrame overrides

  // AppendFrames/InsertFrames/RemoveFrame are implemented to forward
  // the method call to the parent frame.
  NS_IMETHOD  AppendFrames(nsIPresContext& aPresContext,
                           nsIPresShell&   aPresShell,
                           nsIAtom*        aListName,
                           nsIFrame*       aFrameList);
  NS_IMETHOD  InsertFrames(nsIPresContext& aPresContext,
                           nsIPresShell&   aPresShell,
                           nsIAtom*        aListName,
                           nsIFrame*       aPrevFrame,
                           nsIFrame*       aFrameList);
  NS_IMETHOD  RemoveFrame(nsIPresContext& aPresContext,
                          nsIPresShell&   aPresShell,
                          nsIAtom*        aListName,
                          nsIFrame*       aOldFrame);

  // These methods are used by the parent frame to actually modify the
  // child frames of the anonymous block frame.
  nsresult AppendFrames2(nsIPresContext& aPresContext,
                         nsIPresShell&   aPresShell,
                         nsIAtom*        aListName,
                         nsIFrame*       aFrameList);

  nsresult InsertFrames2(nsIPresContext& aPresContext,
                         nsIPresShell&   aPresShell,
                         nsIAtom*        aListName,
                         nsIFrame*       aPrevFrame,
                         nsIFrame*       aFrameList);

  nsresult RemoveFrame2(nsIPresContext& aPresContext,
                        nsIPresShell&   aPresShell,
                        nsIAtom*        aListName,
                        nsIFrame*       aOldFrame);

  // Take the first frame away from the anonymous block frame. The
  // caller is responsible for the first frames final disposition
  // (e.g. deleting it if it wants to).
  void RemoveFirstFrame();

  // Remove from the blocks list of children the frames starting at
  // aFrame until the end of the child list. The caller is responsible
  // for the first frames final disposition (e.g. deleting it if it
  // wants to).
  void RemoveFramesFrom(nsIFrame* aFrame);

protected:
  nsAnonymousBlockFrame();
  ~nsAnonymousBlockFrame();
};

#endif /* nsBlockFrame_h___ */

