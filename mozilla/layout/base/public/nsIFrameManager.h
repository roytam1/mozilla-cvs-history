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
#ifndef nsIFrameManager_h___
#define nsIFrameManager_h___

#include "nslayout.h"
#include "nsISupports.h"

class nsIAtom;
class nsIContent;
class nsIFrame;
class nsIPresContext;
class nsIPresShell;
class nsIStyleSet;
class nsIStyleContext;
class nsILayoutHistoryState;
class nsStyleChangeList;

#define NS_IFRAMEMANAGER_IID     \
{ 0xa6cf9107, 0x15b3, 0x11d2, \
  {0x93, 0x2e, 0x00, 0x80, 0x5f, 0x8a, 0xdd, 0x32} }
  
// Calback function used to destroy the value associated with a
// given property. used by RemoveFrameProperty()
typedef void 
(*NSFMPropertyDtorFunc)(nsIPresContext* aPresContext,
                        nsIFrame*       aFrame,
                        nsIAtom*        aPropertyName,
                        void*           aPropertyValue);

// Option flags for GetFrameProperty() member function
#define NS_IFRAME_MGR_REMOVE_PROP   0x0001

// nsresult error codes for frame property functions
#define NS_IFRAME_MGR_PROP_NOT_THERE \
  NS_ERROR_GENERATE_SUCCESS(NS_ERROR_MODULE_LAYOUT, 1)

#define NS_IFRAME_MGR_PROP_OVERWRITTEN \
  NS_ERROR_GENERATE_SUCCESS(NS_ERROR_MODULE_LAYOUT, 2)

/**
 * Frame manager interface. The frame manager serves two purposes:
 * <li>provides a serice for mapping from content to frame and from out-of-flow
 * frame to placeholder frame
 * <li>handles structural modifications to the frame model. If the frame model
 * lock can be acquired, then the changes are processed immediately; otherwise,
 * they're queued and processed later
 */
class nsIFrameManager : public nsISupports {
public:
  static const nsIID& GetIID() {static nsIID iid = NS_IFRAMEMANAGER_IID; return iid;}

  // Initialization
  NS_IMETHOD Init(nsIPresShell* aPresShell, nsIStyleSet* aStyleSet) = 0;

  // Gets and sets the root frame (typically the viewport). The lifetime of the
  // root frame is controlled by the frame manager. When the frame manager is
  // destroyed it destroys the entire frame hierarchy
  NS_IMETHOD GetRootFrame(nsIFrame** aRootFrame) const = 0;
  NS_IMETHOD SetRootFrame(nsIFrame* aRootFrame) = 0;

  // Get the canvas frame. The canvas frame may or may not exist, so the
  // argument aCanvasFrame may be nsnull.
  NS_IMETHOD GetCanvasFrame(nsIPresContext* aPresContext, nsIFrame** aCanvasFrame) const = 0;

  // Primary frame functions
  NS_IMETHOD GetPrimaryFrameFor(nsIContent* aContent, nsIFrame** aPrimaryFrame) = 0;
  NS_IMETHOD SetPrimaryFrameFor(nsIContent* aContent,
                                nsIFrame*   aPrimaryFrame) = 0;
  NS_IMETHOD ClearPrimaryFrameMap() = 0;

  // Placeholder frame functions
  NS_IMETHOD GetPlaceholderFrameFor(nsIFrame*  aFrame,
                                    nsIFrame** aPlaceholderFrame) const = 0;
  NS_IMETHOD SetPlaceholderFrameFor(nsIFrame* aFrame,
                                    nsIFrame* aPlaceholderFrame) = 0;
  NS_IMETHOD ClearPlaceholderFrameMap() = 0;

  // Mapping undisplayed content
  NS_IMETHOD SetUndisplayedContent(nsIContent* aContent, nsIStyleContext* aStyleContext) = 0;
  NS_IMETHOD SetUndisplayedPseudoIn(nsIStyleContext* aPseudoContext, 
                                    nsIContent* aParentContent) = 0;
  NS_IMETHOD ClearUndisplayedContentIn(nsIContent* aContent, nsIContent* aParentContent) = 0;
  NS_IMETHOD ClearAllUndisplayedContentIn(nsIContent* aParentContent) = 0;
  NS_IMETHOD ClearUndisplayedContentMap() = 0;

  // Functions for manipulating the frame model
  NS_IMETHOD AppendFrames(nsIPresContext* aPresContext,
                          nsIPresShell&   aPresShell,
                          nsIFrame*       aParentFrame,
                          nsIAtom*        aListName,
                          nsIFrame*       aFrameList) = 0;
  NS_IMETHOD InsertFrames(nsIPresContext* aPresContext,
                          nsIPresShell&   aPresShell,
                          nsIFrame*       aParentFrame,
                          nsIAtom*        aListName,
                          nsIFrame*       aPrevFrame,
                          nsIFrame*       aFrameList) = 0;
  NS_IMETHOD RemoveFrame(nsIPresContext* aPresContext,
                         nsIPresShell&   aPresShell,
                         nsIFrame*       aParentFrame,
                         nsIAtom*        aListName,
                         nsIFrame*       aOldFrame) = 0;
  NS_IMETHOD ReplaceFrame(nsIPresContext* aPresContext,
                          nsIPresShell&   aPresShell,
                          nsIFrame*       aParentFrame,
                          nsIAtom*        aListName,
                          nsIFrame*       aOldFrame,
                          nsIFrame*       aNewFrame) = 0;

  // Notification that we were unable to render a replaced element
  NS_IMETHOD CantRenderReplacedElement(nsIPresContext* aPresContext,
                                       nsIFrame*       aFrame) = 0;

  // Notification that a frame is about to be destroyed. This allows any outstanding
  // references to the frame to be cleaned up
  NS_IMETHOD NotifyDestroyingFrame(nsIFrame* aFrame) = 0;

  // reparent the style contexts of this frame sub tree to live under the
  // new given parent style context
  NS_IMETHOD ReParentStyleContext(nsIPresContext* aPresContext, 
                                  nsIFrame* aFrame, 
                                  nsIStyleContext* aNewParentContext) = 0;

  // Re-resolve style contexts for frame tree
  NS_IMETHOD ComputeStyleChangeFor(nsIPresContext* aPresContext,
                                   nsIFrame* aFrame, 
                                   PRInt32 aAttrNameSpaceID,
                                   nsIAtom* aAttribute,
                                   nsStyleChangeList& aChangeList,
                                   PRInt32 aMinChange,
                                   PRInt32& aTopLevelChange) = 0;

  // Determine whether an attribute affects style
  NS_IMETHOD AttributeAffectsStyle(nsIAtom *aAttribute, nsIContent *aContent,
                                   PRBool &aAffects) = 0;

  /**
   * Capture/restore frame state for the frame subtree rooted at aFrame.
   * aState is the document state storage object onto which each frame 
   * stores its state.
   */
  NS_IMETHOD CaptureFrameState(nsIPresContext* aPresContext,
                               nsIFrame* aFrame,
                               nsILayoutHistoryState* aState) = 0;
  NS_IMETHOD RestoreFrameState(nsIPresContext* aPresContext,
                               nsIFrame* aFrame,
                               nsILayoutHistoryState* aState) = 0;

  /**
   * Gets a property value for a given frame.
   *
   * @param   aFrame          the frame with the property
   * @param   aPropertyName   property name as an atom
   * @param   aOptions        optional flags
   *                            NS_IFRAME_MGR_REMOVE_PROP removes the property
   * @param   aPropertyValue  the property value or 0 if the property is not set
   * @return  NS_OK if the property is set,
   *          NS_IFRAME_MGR_PROP_NOT_THERE if the property is not set
   */
  NS_IMETHOD GetFrameProperty(nsIFrame* aFrame,
                              nsIAtom*  aPropertyName,
                              PRUint32  aOptions,
                              void**    aPropertyValue) = 0;
  
  /**
   * Sets the property value for a given frame.
   *
   * A frame may only have one property value at a time for a given property
   * name. The existing property value (if there is one) is overwritten, and the
   * old value destroyed
   *
   * @param   aFrame            the frame to set the property on
   * @param   aPropertyName     property name as an atom
   * @param   aPropertyValue    the property value
   * @param   aPropertyDtorFunc when setting a property you can specify the
   *                            dtor function (can be NULL) that will be used
   *                            to destroy the property value. There can be only
   *                            one dtor function for a given property name
   * @return  NS_OK if successful,
   *          NS_IFRAME_MGR_PROP_OVERWRITTEN if there is an existing property
   *            value that was overwritten,
   *          NS_ERROR_INVALID_ARG if the dtor function does not match the
   *            existing dtor function
   */
  NS_IMETHOD SetFrameProperty(nsIFrame*            aFrame,
                              nsIAtom*             aPropertyName,
                              void*                aPropertyValue,
                              NSFMPropertyDtorFunc aPropertyDtorFunc) = 0;

  /**
   * Removes a property and destroys its property value by calling the dtor
   * function associated with the property name.
   *
   * When a frame is destroyed any remaining properties are automatically removed
   *
   * @param   aFrame          the frame to set the property on
   * @param   aPropertyName   property name as an atom
   * @return  NS_OK if the property is successfully removed,
   *          NS_IFRAME_MGR_PROP_NOT_THERE if the property is not set
   */
  NS_IMETHOD RemoveFrameProperty(nsIFrame* aFrame,
                                 nsIAtom*  aPropertyName) = 0;

#ifdef NS_DEBUG
  /**
   * DEBUG ONLY method to verify integrity of style tree versus frame tree
   */
  NS_IMETHOD DebugVerifyStyleTree(nsIPresContext* aPresContext, nsIFrame* aFrame) = 0;
#endif
};

/**
 * Create a frame manager. Upon success, call Init() before attempting to
 * use it.
 */
extern NS_LAYOUT nsresult
  NS_NewFrameManager(nsIFrameManager** aInstancePtrResult);

#endif /* nsIFrameManager_h___ */
