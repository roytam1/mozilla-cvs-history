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
#include "nsStyleStruct.h"

class nsIAtom;
class nsIContent;
class nsIFrame;
class nsIPresContext;
class nsIPresShell;
class nsIRenderingContext;
class nsISizeOfHandler;
class nsISpaceManager;
class nsIStyleContext;
class nsIView;
class nsIWidget;
class nsIReflowCommand;
class nsIListFilter;
class nsAutoString;
class nsString;

struct nsPoint;
struct nsRect;
struct nsStyleStruct;

struct PRLogModuleInfo;

// IID for the nsIFrame interface 
// a6cf9050-15b3-11d2-932e-00805f8add32
#define NS_IFRAME_IID \
 { 0xa6cf9050, 0x15b3, 0x11d2,{0x93, 0x2e, 0x00, 0x80, 0x5f, 0x8a, 0xdd, 0x32}}

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
typedef PRUint32 nsSplittableType;

#define NS_FRAME_NOT_SPLITTABLE             0   // Note: not a bit!
#define NS_FRAME_SPLITTABLE                 0x1
#define NS_FRAME_SPLITTABLE_NON_RECTANGULAR 0x3

#define NS_FRAME_IS_SPLITTABLE(type)\
  (0 != ((type) & NS_FRAME_SPLITTABLE))

#define NS_FRAME_IS_NOT_SPLITTABLE(type)\
  (0 == ((type) & NS_FRAME_SPLITTABLE))

//----------------------------------------------------------------------

/**
 * Frame state bits. Any bits not listed here are reserved for future
 * extensions, but must be stored by the frames.
 */
typedef PRUint32 nsFrameState;

#define NS_FRAME_IN_REFLOW    0x00000001

// This bit is set when a frame is created. After it has been reflowed
// once (during the DidReflow with a finished state) the bit is
// cleared.
#define NS_FRAME_FIRST_REFLOW 0x00000002

// If this bit is is set then the view position and size should be
// kept in sync with the frame position and size. If the bit is not
// set then it's the responsibility of the frame itself (or whoever
// created the view) to position and size its associated view
#define NS_FRAME_SYNC_FRAME_AND_VIEW 0x00000004

// If this bit is set then there is a child frame in the frame that
// extends outside this frame's bounding box. The implication is that
// the frames rect does not completely cover its children and
// therefore operations like rendering and hit testing (for example)
// must operate differently.
#define NS_FRAME_OUTSIDE_CHILDREN 0x00000008

//----------------------------------------------------------------------

/**
 * A frame in the layout model. This interface is supported by all frame
 * objects.
 *
 * Frames are NOT reference counted. Use the DeleteFrame() member function
 * to delete a frame.
 *
 * The lifetime of the frame hierarchy is bounded by the lifetime of the
 * presentation shell which owns the frames.
 */
class nsIFrame : public nsISupports
{
public:
  /**
   * Initialize the frame passing it its child frame list.
   *
   * This member function is called for all frames just after the frame is
   * constructed.
   *
   * You should reflow the frames when you get your 'initial' reflow
   * notification.
   *   
   * @param   aChildList list of child frames. May be NULL
   * @see     #Reflow()
   */
  NS_IMETHOD  Init(nsIPresContext& aPresContext, nsIFrame* aChildList) = 0;

  /**
   * Add this object's size information to the sizeof handler. Note that
   * this does <b>not</b> add in the size of content, style, or view's
   * (those are sized seperately).
   */
  NS_IMETHOD SizeOf(nsISizeOfHandler* aHandler) const = 0;

  /**
   * Deletes this frame and each of its child frames (recursively calls
   * DeleteFrame() for each child)
   */
  NS_IMETHOD  DeleteFrame(nsIPresContext& aPresContext) = 0;

  /**
   * Get the content object associated with this frame. Adds a reference to
   * the content object so the caller must do a release.
   *
   * @see nsISupports#Release()
   */
  NS_IMETHOD  GetContent(nsIContent*& aContent) const = 0;

  /**
   * Get the style context associated with this frame. Note that GetStyleContext()
   * adds a reference to the style context so the caller must do a release.
   *
   * @see nsISupports#Release()
   */
  NS_IMETHOD  GetStyleContext(nsIPresContext*   aContext,
                              nsIStyleContext*& aStyleContext) = 0;
  NS_IMETHOD  SetStyleContext(nsIPresContext* aPresContext,
                              nsIStyleContext* aContext) = 0;

  /**
   * Get the style data associated with this frame.
   */
  NS_IMETHOD  GetStyleData(nsStyleStructID aSID, const nsStyleStruct*& aStyleStruct) const = 0;

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
   * Child frame enumeration.
   *
   * Child frames are linked together in a singly-linked list.
   */
  NS_IMETHOD  FirstChild(nsIFrame*& aFirstChild) const = 0;
  NS_IMETHOD  GetNextSibling(nsIFrame*& aNextSibling) const = 0;
  NS_IMETHOD  SetNextSibling(nsIFrame* aNextSibling) = 0;

  /**
   * Painting
   */
  NS_IMETHOD  Paint(nsIPresContext&      aPresContext,
                    nsIRenderingContext& aRenderingContext,
                    const nsRect&        aDirtyRect) = 0;

  /**
   * Event handling of GUI events.
   *
   * @param   aEvent event structure describing the type of event and rge widget
   *            where the event originated
   * @param   aEventStatus a return value indicating whether the event was handled
   *            and whether default processing should be done
   *
   * XXX From a frame's perspective it's unclear what the effect of the event status
   * is. Does it cause the event to continue propagating through the frame hierarchy
   * or is it just returned to the widgets?
   *
   * @see     nsGUIEvent
   * @see     nsEventStatus
   */
  NS_IMETHOD  HandleEvent(nsIPresContext& aPresContext,
                          nsGUIEvent*     aEvent,
                          nsEventStatus&  aEventStatus) = 0;

  NS_IMETHOD GetPosition(nsIPresContext&       aPresContext,
                         nsIRenderingContext * aRendContext,
                         nsGUIEvent*           aEvent,
                         nsIFrame *            aNewFrame,
                         PRUint32&             aAcutalContentOffset,
                         PRInt32&              aOffset) = 0;


  /**
   * Get the cursor for a given point in the frame tree. The
   * call returns the desired cursor (or NS_STYLE_CURSOR_INHERIT if
   * no cursor is wanted). In addition, if a cursor is desired
   * then *aFrame is set to the frame that wants the cursor.
   */
  NS_IMETHOD  GetCursorAndContentAt(nsIPresContext& aPresContext,
                          const nsPoint&  aPoint,
                          nsIFrame**      aFrame,
                          nsIContent**    aContent,
                          PRInt32&        aCursor) = 0;

  /**
   * Get the current frame-state value for this frame. aResult is
   * filled in with the state bits. The return value has no
   * meaning.
   */
  NS_IMETHOD  GetFrameState(nsFrameState& aResult) = 0;

  /**
   * Set the current frame-state value for this frame. The return
   * value has no meaning.
   */
  NS_IMETHOD  SetFrameState(nsFrameState aNewState) = 0;

  /**
   * This call is invoked when content is changed in the content tree.
   * The first frame that maps that content is asked to deal with the
   * change by generating an incremental reflow command.
   *
   * @param aIndexInParent the index in the content container where
   *          the new content was deleted.
   */
  NS_IMETHOD  ContentChanged(nsIPresContext* aPresContext,
                             nsIContent*     aChild,
                             nsISupports*    aSubContent) = 0;

  /**
   * This call is invoked when the value of a content objects's attribute
   * is changed. 
   * The first frame that maps that content is asked to deal
   * with the change by generating an incremental reflow command.
   *
   * @param aChild the content object
   * @param aAttribute the attribute whose value changed
   */
  NS_IMETHOD  AttributeChanged(nsIPresContext* aPresContext,
                               nsIContent*     aChild,
                               nsIAtom*        aAttribute,
                               PRInt32         aHint) = 0;

  /**
   * Return how your frame can be split.
   */
  NS_IMETHOD  IsSplittable(nsSplittableType& aIsSplittable) const = 0;

  /**
   * Flow member functions. CreateContinuingFrame() is responsible for
   * appending the continuing frame to the flow.
   */
  NS_IMETHOD  CreateContinuingFrame(nsIPresContext&  aPresContext,
                                    nsIFrame*        aParent,
                                    nsIStyleContext* aStyleContext,
                                    nsIFrame*&       aContinuingFrame) = 0;

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
   * Is this frame a "containing block"?
   */
  NS_IMETHOD  IsPercentageBase(PRBool& aBase) const = 0;

  /**
   * Gets the size of an "auto" margin.
   */
  NS_IMETHOD  GetAutoMarginSize(PRUint8 aSide, nscoord& aSize) const = 0;

  /**
   * Does this frame have content that is considered "transparent"?
   * This is binary transparency as opposed to translucency. MMP
   */
  NS_IMETHOD IsTransparent(PRBool& aTransparent) const = 0;

  /**
   * called when the frame has been scrolled to a new
   * position. only called for frames with views.
   */
  NS_IMETHOD Scrolled(nsIView *aView) = 0;

  // Debugging
  NS_IMETHOD  List(FILE* out = stdout, PRInt32 aIndent = 0, nsIListFilter *aFilter = nsnull) const= 0;
  NS_IMETHOD  ListTag(FILE* out = stdout) const = 0;
  NS_IMETHOD  VerifyTree() const = 0;
  static NS_LAYOUT nsIListFilter * GetFilter(nsString *aFilterName);

  /**
   * See if tree verification is enabled. To enable tree verification add
   * "frameverifytree:1" to your NSPR_LOG_MODULES environment variable
   * (any non-zero debug level will work). Or, call SetVerifyTreeEnable
   * with PR_TRUE.
   */
  static NS_LAYOUT PRBool GetVerifyTreeEnable();

  /**
   * Set the verify-tree enable flag.
   */
  static NS_LAYOUT void SetVerifyTreeEnable(PRBool aEnabled);

  /**
   * The frame class and related classes share an nspr log module
   * for logging frame activity.
   *
   * Note: the log module is created during library initialization which
   * means that you cannot perform logging before then.
   */
  static NS_LAYOUT PRLogModuleInfo* GetLogModuleInfo();

  // Show frame borders when rendering
  static NS_LAYOUT void ShowFrameBorders(PRBool aEnable);
  static NS_LAYOUT PRBool GetShowFrameBorders();

protected:
  static NS_LAYOUT PRLogModuleInfo* gLogModule;

private:
  NS_IMETHOD_(nsrefcnt) AddRef(void) = 0;
  NS_IMETHOD_(nsrefcnt) Release(void) = 0;
};

/* ----- nsIListFilter definition ----- */
class nsIListFilter
{
  public:
    virtual PRBool OutputTag(nsAutoString *aTag) const = 0;
};

#endif /* nsIFrame_h___ */
