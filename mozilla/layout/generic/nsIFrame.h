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
#ifndef nsIFrame_h___
#define nsIFrame_h___

#include <stdio.h>
#include "nslayout.h"
#include "nsISupports.h"
#include "nsSize.h"
#include "nsGUIEvent.h"

class nsIContent;
class nsIPresContext;
class nsIPresShell;
class nsIRenderingContext;
class nsISpaceManager;
class nsIStyleContext;
class nsIView;
class nsIWidget;
class nsReflowCommand;

struct nsPoint;
struct nsRect;
struct nsReflowMetrics;
struct nsStyleStruct;

// IID for the nsIFrame interface {12B193D0-9F70-11d1-8500-00A02468FAB6}
#define NS_IFRAME_IID         \
{ 0x12b193d0, 0x9f70, 0x11d1, \
  {0x85, 0x0, 0x0, 0xa0, 0x24, 0x68, 0xfa, 0xb6}}

/**
 * Reflow metrics used to return the frame's desired size and alignment
 * information.
 *
 * @see #ResizeReflow()
 * @see #IncrementalReflow()
 * @see #GetReflowMetrics()
 */
struct nsReflowMetrics {
  nscoord width, height;
  nscoord ascent, descent;
};

/**
 * Constant used to indicate an unconstrained size.
 *
 * @see #ResizeReflow()
 * @see #IncrementalReflow()
 */
#define NS_UNCONSTRAINEDSIZE NS_MAXSIZE

/**
 * A frame in the layout model. This interface is supported by all frame
 * objects.
 *
 * Frames are NOT reference counted. Use the DeleteFrame() member function
 * instead
 */
class nsIFrame : private nsISupports
{
public:
  /**
   * QueryInterface() defined in nsISupports. This is the only member
   * function of nsISupports that is public.
   */
  NS_IMETHOD  QueryInterface(const nsIID& aIID, void** aInstancePtr) = 0;

  /**
   * Deletes this frame and each of its child frames (recursively calls
   * DeleteFrame() for each child)
   */
  NS_IMETHOD  DeleteFrame() = 0;

  /**
   * Get the content object associated with this frame. Adds a reference to
   * the content object so the caller must do a release.
   *
   * @see nsISupports#Release()
   */
  NS_IMETHOD  GetContent(nsIContent*& aContent) const = 0;

  /**
   * Get/Set the frame's index in parent.
   */
  NS_IMETHOD  GetIndexInParent(PRInt32& aIndexInParent) const = 0;
  NS_IMETHOD  SetIndexInParent(PRInt32 aIndexInParent) = 0;

  /**
   * Get the style context associated with this frame. Note that GetStyleContext()
   * adds a reference to the style context so the caller must do a release.
   *
   * @see nsISupports#Release()
   */
  NS_IMETHOD  GetStyleContext(nsIPresContext*   aContext,
                              nsIStyleContext*& aStyleContext) = 0;
  NS_IMETHOD  SetStyleContext(nsIStyleContext* aContext) = 0;

  /**
   * Get the style data associated with this frame
   */
  NS_IMETHOD  GetStyleData(const nsIID& aSID, nsStyleStruct*& aStyleStruct) = 0;

  /**
   * Accessor functions for geometric and content parent.
   */
  NS_IMETHOD  GetContentParent(nsIFrame*& aParent) const = 0;
  NS_IMETHOD  SetContentParent(const nsIFrame* aParent) = 0;
  NS_IMETHOD  GetGeometricParent(nsIFrame*& aParent) const = 0;
  NS_IMETHOD  SetGeometricParent(const nsIFrame* aParent) = 0;

  /**
   * Bounding rect of the frame. The values are in twips, and the origin is
   * relative to the upper-left of the geometric parent. The size includes the
   * content area, borders, and padding.
   */
  NS_IMETHOD  GetRect(nsRect& aRect) const = 0;
  NS_IMETHOD  GetOrigin(nsPoint& aPoint) const = 0;
  NS_IMETHOD  GetSize(nsSize& aSize) const = 0;
  NS_IMETHOD  SetRect(const nsRect& aRect) = 0;
  NS_IMETHOD  MoveTo(nscoord aX, nscoord aY) = 0;
  NS_IMETHOD  SizeTo(nscoord aWidth, nscoord aHeight) = 0;

  /**
   * Child frame enumeration
   */
  NS_IMETHOD  ChildCount(PRInt32& aChildCount) const = 0;
  NS_IMETHOD  ChildAt(PRInt32 aIndex, nsIFrame*& aFrame) const = 0;
  NS_IMETHOD  IndexOf(const nsIFrame* aChild, PRInt32& aIndex) const = 0;
  NS_IMETHOD  FirstChild(nsIFrame*& aFirstChild) const = 0;
  NS_IMETHOD  NextChild(const nsIFrame* aChild, nsIFrame*& aNextChild) const = 0;
  NS_IMETHOD  PrevChild(const nsIFrame* aChild, nsIFrame*& aPrevChild) const = 0;
  NS_IMETHOD  LastChild(nsIFrame*& aLastChild) const = 0;

  /**
   * Painting
   */
  NS_IMETHOD  Paint(nsIPresContext&      aPresContext,
                    nsIRenderingContext& aRenderingContext,
                    const nsRect&        aDirtyRect) = 0;

  /**
   * Handle an event. 
   */
  NS_IMETHOD  HandleEvent(nsIPresContext& aPresContext,
                          nsGUIEvent*     aEvent,
                          nsEventStatus&  aEventStatus) = 0;

  /**
   * Get the cursor for a given point in the frame tree. The
   * call returns the desired cursor (or NS_STYLE_CURSOR_INHERIT if
   * no cursor is wanted). In addition, if a cursor is desired
   * then *aFrame is set to the frame that wants the cursor.
   */
  NS_IMETHOD  GetCursorAt(nsIPresContext& aPresContext,
                          const nsPoint&  aPoint,
                          nsIFrame**      aFrame,
                          PRInt32&        aCursor) = 0;

  /**
   * Reflow status returned by the reflow methods. frNotComplete means you didn't
   * map all your content, and so your parent should create a continuing frame
   * for you.
   *
   * @see #ResizeReflow()
   * @see #IncrementalReflow()
   * @see #CreateContinuingFrame()
   */
  enum ReflowStatus {frComplete, frNotComplete};

  /**
   * Resize reflow. The frame is given a maximum size and asked for its desired
   * size. This is the frame's opportunity to reflow its children.
   *
   * @param aDesiredSize <i>out</i> parameter where you should return the
   *          desired size and ascent/descent info. You should include any
   *          space you want for border/padding in the desired size you return.
   * @param aMaxSize the available space in which to lay out. Each dimension
   *          can either be constrained or unconstrained (a value of
   *          NS_UNCONSTRAINEDSIZE). If constrained you should choose a value that's
   *          less than or equal to the constrained size. If unconstrained you can
   *          choose as large a value as you like.
   *
   *          It's okay to return a desired size that exceeds the max size if that's
   *          the smallest you can be, i.e. it's your minimum size.
   *
   * @param aMaxElementSize an optional parameter for returning your maximum
   *          element size. If may be null in which case you don't have to compute
   *          a maximum element size. The maximum element size must be less than or
   *          equal to your desired size.
   */
  NS_IMETHOD  ResizeReflow(nsIPresContext*  aPresContext,
                           nsReflowMetrics& aDesiredSize,
                           const nsSize&    aMaxSize,
                           nsSize*          aMaxElementSize,
                           ReflowStatus&    aStatus) = 0;

  /**
   * Post-processing reflow method invoked when justification is enabled.
   * This is always called after ResizeReflow/IncrementalReflow.
   *
   * @param aAvailableSpace The amount of available space that the frame
   *         should distribute internally.
   */
  NS_IMETHOD  JustifyReflow(nsIPresContext* aPresContext,
                            nscoord         aAvailableSpace) = 0;

  /**
   * Incremental reflow. The reflow command contains information about the
   * type of change. The frame is given a maximum size and asked for its
   * desired size.
   *
   * @param aDesiredSize <i>out</i> parameter where you should return
   *          the desired size and ascent/descent info. You should
   *          include any space you want for border/padding in the
   *          desired size you return.
   *
   * @param aMaxSize the available space in which to lay out. Each
   *          dimension can either be constrained or unconstrained (a
   *          value of NS_UNCONSTRAINEDSIZE). If constrained you
   *          should choose a value that's less than or equal to the
   *          constrained size. If unconstrained you can choose as
   *          large a value as you like. It's okay to return a
   *          desired size that exceeds the max size if that's the
   *          smallest you can be, i.e. it's your minimum size.
   *
   * @see nsReflowCommand#GetTarget()
   * @see nsReflowCommand#GetType()
   */
  NS_IMETHOD  IncrementalReflow(nsIPresContext*  aPresContext,
                                nsReflowMetrics& aDesiredSize,
                                const nsSize&    aMaxSize,
                                nsReflowCommand& aReflowCommand,
                                ReflowStatus&    aStatus) = 0;

  /**
   * This call is invoked when content is appended to the content
   * tree. The container frame that maps that content is asked to deal
   * with the appended content by creating new frames and updating the
   * index-in-parent values for it's affected children. In addition,
   * the call must generate reflow commands that will incrementally
   * reflow and repair the damaged portion of the frame tree.
   */
  NS_IMETHOD  ContentAppended(nsIPresShell*   aShell,
                              nsIPresContext* aPresContext,
                              nsIContent*     aContainer) = 0;

  /**
   * This call is invoked when content is inserted in the content
   * tree. The container frame that maps that content is asked to deal
   * with the inserted content by creating new frames and updating the
   * index-in-parent values for it's affected children. In addition,
   * the call must generate reflow commands that will incrementally
   * reflow and repair the damaged portion of the frame tree.
   *
   * @param aIndexInParent the index in the content container where
   *          the new content was inserted.
   */
  NS_IMETHOD  ContentInserted(nsIPresShell*   aShell,
                              nsIPresContext* aPresContext,
                              nsIContent*     aContainer,
                              nsIContent*     aChild,
                              PRInt32         aIndexInParent) = 0;

  /**
   * This call is invoked when content is replaced in the content
   * tree. The container frame that maps that content is asked to deal
   * with the replaced content by deleting old frames and then
   * creating new frames and updating the index-in-parent values for
   * it's affected children. In addition, the call must generate
   * reflow commands that will incrementally reflow and repair the
   * damaged portion of the frame tree.
   *
   * @param aIndexInParent the index in the content container where
   *          the new content was inserted.  */
  NS_IMETHOD  ContentReplaced(nsIPresShell*   aShell,
                              nsIPresContext* aPresContext,
                              nsIContent*     aContainer,
                              nsIContent*     aOldChild,
                              nsIContent*     aNewChild,
                              PRInt32         aIndexInParent) = 0;

  /**
   * This call is invoked when content is deleted from the content
   * tree. The container frame that maps that content is asked to deal
   * with the deleted content by deleting frames and updating the
   * index-in-parent values for it's affected children. In addition,
   * the call must generate reflow commands that will incrementally
   * reflow and repair the damaged portion of the frame tree.
   *
   * @param aIndexInParent the index in the content container where
   *          the new content was deleted.
   */
  NS_IMETHOD  ContentDeleted(nsIPresShell*   aShell,
                             nsIPresContext* aPresContext,
                             nsIContent*     aContainer,
                             nsIContent*     aChild,
                             PRInt32         aIndexInParent) = 0;

  /**
   * Return the reflow metrics for this frame. If the frame is a
   * container then the values for ascent and descent are computed
   * across the the various children in the appropriate manner
   * (e.g. for a line frame the ascent value would be the maximum
   * ascent of the line's children). Note that the metrics returned
   * apply to the frame as it exists at the time of the call.
   */
  NS_IMETHOD  GetReflowMetrics(nsIPresContext*  aPresContext,
                               nsReflowMetrics& aMetrics) = 0;

  /**
   * Indication of how the frame can be split. This is used when doing runaround
   * of floaters, and when pulling up child frames from a next-in-flow.
   *
   * The choices are splittable, not splittable at all, and splittable in
   * a non-rectangular fashion. This last type only applies to block-level
   * elements, and indicates whether splitting can be used when doing runaround.
   * If you can split across page boundaries, but you expect each continuing
   * frame to be the same width then return frSplittable and not
   * frSplittableNonRectangular.
   *
   * @see #IsSplittable()
   */
  enum SplittableType {frNotSplittable = 0, frSplittable = 1, frSplittableNonRectangular = 3};

  /**
   * Return how your frame can be split.
   */
  NS_IMETHOD  IsSplittable(SplittableType& aIsSplittable) const = 0;

  /**
   * Flow member functions. CreateContinuingFrame() is responsible for appending
   * the continuing frame to the flow.
   */
  NS_IMETHOD  CreateContinuingFrame(nsIPresContext* aPresContext,
                                    nsIFrame*       aParent,
                                    nsIFrame*&      aContinuingFrame) = 0;

  NS_IMETHOD  GetPrevInFlow(nsIFrame*& aPrevInFlow) const = 0;
  NS_IMETHOD  SetPrevInFlow(nsIFrame*) = 0;
  NS_IMETHOD  GetNextInFlow(nsIFrame*& aNextInFlow) const = 0;
  NS_IMETHOD  SetNextInFlow(nsIFrame*) = 0;

  NS_IMETHOD  AppendToFlow(nsIFrame* aAfterFrame) = 0;
  NS_IMETHOD  PrependToFlow(nsIFrame* aBeforeFrame) = 0;
  NS_IMETHOD  RemoveFromFlow() = 0;
  NS_IMETHOD  BreakFromPrevFlow() = 0;
  NS_IMETHOD  BreakFromNextFlow() = 0;

  /**
   * Accessor functions to get/set the associated view object
   */
  NS_IMETHOD  GetView(nsIView*& aView) const = 0;  // may be null
  NS_IMETHOD  SetView(nsIView* aView) = 0;

  /**
   * Find the first geometric parent that has a view
   */
  NS_IMETHOD  GetParentWithView(nsIFrame*& aParent) const = 0;

  /**
   * Returns the offset from this frame to the closest geometric parent that
   * has a view. Also returns the containing view or null in case of error
   */
  NS_IMETHOD  GetOffsetFromView(nsPoint& aOffset, nsIView*& aView) const = 0;

  /**
   * Returns the window that contains this frame. If this frame has a
   * view and the view has a window, then this frames window is
   * returned, otherwise this frame's geometric parent is checked
   * recursively upwards.
   */
  NS_IMETHOD  GetWindow(nsIWidget*&) const = 0;

  /**
   * Sibling pointer used to link together frames
   */
  NS_IMETHOD  GetNextSibling(nsIFrame*& aNextSibling) const = 0;
  NS_IMETHOD  SetNextSibling(nsIFrame* aNextSibling) = 0;

  // Debugging
  NS_IMETHOD  List(FILE* out = stdout, PRInt32 aIndent = 0) const= 0;
  NS_IMETHOD  ListTag(FILE* out = stdout) const = 0;
  NS_IMETHOD  VerifyTree() const = 0;

  // Show frame borders when rendering
  static NS_LAYOUT void ShowFrameBorders(PRBool aEnable);
  static NS_LAYOUT PRBool GetShowFrameBorders();
};

#endif /* nsIFrame_h___ */
