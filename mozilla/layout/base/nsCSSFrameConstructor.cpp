/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
// vim:cindent:ts=2:et:sw=2:
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Dan Rosen <dr@netscape.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or 
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "nsCSSFrameConstructor.h"
#include "nsIArena.h"
#include "nsCRT.h"
#include "nsIAtom.h"
#include "nsIURL.h"
#include "nsISupportsArray.h"
#include "nsHashtable.h"
#include "nsIHTMLContent.h"
#include "nsIStyleRule.h"
#include "nsIFrame.h"
#include "nsHTMLAtoms.h"
#include "nsIPresContext.h"
#include "nsILinkHandler.h"
#include "nsIDocument.h"
#include "nsTableFrame.h"
#include "nsTableColGroupFrame.h"
#include "nsTableColFrame.h"
#include "nsIDOMHTMLTableColElement.h"
#include "nsTableCellFrame.h" // to get IS_CELL_FRAME
#include "nsIStyleFrameConstruction.h"
#include "nsHTMLParts.h"
#include "nsIPresShell.h"
#include "nsIStyleSet.h"
#include "nsIViewManager.h"
#include "nsIScrollableView.h"
#include "nsStyleConsts.h"
#include "nsTableOuterFrame.h"
#include "nsIXMLDocument.h"
#include "nsIDOMXULElement.h"
#include "nsIWebShell.h"
#include "nsHTMLContainerFrame.h"
#include "nsINameSpaceManager.h"
#include "nsLayoutAtoms.h"
#include "nsIDOMHTMLSelectElement.h"
#include "nsIComboboxControlFrame.h"
#include "nsIListControlFrame.h"
#include "nsISelectControlFrame.h"
#include "nsIRadioControlFrame.h"
#include "nsICheckboxControlFrame.h"
#include "nsIDOMCharacterData.h"
#include "nsIDOMHTMLImageElement.h"
#include "nsITextContent.h"
#include "nsPlaceholderFrame.h"
#include "nsTableRowGroupFrame.h"
#include "nsStyleChangeList.h"
#include "nsIFormControl.h"
#include "nsCSSAtoms.h"
#include "nsIDeviceContext.h"
#include "nsTextFragment.h"
#include "nsISupportsArray.h"
#include "nsIAnonymousContentCreator.h"
#include "nsIFrameManager.h"
#include "nsIAttributeContent.h"
#include "nsIPref.h"
#include "nsLegendFrame.h"
#include "nsIContentIterator.h"
#include "nsBoxLayoutState.h"
#include "nsIBindingManager.h"
#include "nsIXBLBinding.h"
#include "nsIElementFactory.h"
#include "nsITheme.h"
#include "nsContentCID.h"
#include "nsIDocShell.h"
#include "nsFormControlHelper.h"
#include "nsObjectFrame.h"
#include "nsRuleNode.h"
#include "nsIXULDocument.h"
#include "nsIPrintPreviewContext.h"
#include "nsIDOMMutationEvent.h"
#include "nsChildIterator.h"
#include "nsCSSRendering.h"
#include "nsISelectElement.h"

static NS_DEFINE_CID(kTextNodeCID,   NS_TEXTNODE_CID);
static NS_DEFINE_CID(kHTMLElementFactoryCID,   NS_HTML_ELEMENT_FACTORY_CID);
static NS_DEFINE_CID(kAttributeContentCID, NS_ATTRIBUTECONTENT_CID);

#include "nsIDOMWindowInternal.h"
#include "nsPIDOMWindow.h"
#include "nsIMenuFrame.h"

#include "nsBox.h"

#ifdef INCLUDE_XUL
#include "nsIRootBox.h"
#include "nsIDOMXULCommandDispatcher.h"
#include "nsIDOMXULDocument.h"
#endif

#include "nsInlineFrame.h"
#include "nsBlockFrame.h"

#include "nsIScrollableFrame.h"

#include "nsIXBLService.h"
#include "nsIStyleRuleSupplier.h"

#undef NOISY_FIRST_LETTER

#ifdef MOZ_MATHML
#include "nsMathMLAtoms.h"
#include "nsMathMLParts.h"
#endif

#ifdef MOZ_SVG
#include "nsSVGAtoms.h"
#include "nsISVGAttribute.h"
#include "nsISVGValue.h"
#include "nsISVGStyleValue.h"

extern nsresult
NS_NewSVGOuterSVGFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsIFrame** aNewFrame);
extern nsresult
NS_NewSVGPolylineFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsIFrame** aNewFrame);
extern nsresult
NS_NewSVGPolygonFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsIFrame** aNewFrame);
extern nsresult
NS_NewSVGCircleFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsIFrame** aNewFrame);
extern nsresult
NS_NewSVGEllipseFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsIFrame** aNewFrame);
extern nsresult
NS_NewSVGLineFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsIFrame** aNewFrame);
extern nsresult
NS_NewSVGRectFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsIFrame** aNewFrame);
extern nsresult
NS_NewSVGGFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsIFrame** aNewFrame);
extern nsresult
NS_NewSVGGenericContainerFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsIFrame** aNewFrame);
extern nsresult
NS_NewSVGForeignObjectFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsIFrame** aNewFrame);
extern nsresult
NS_NewSVGPathFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsIFrame** aNewFrame);
#endif

#include "nsIDocument.h"
#include "nsIDOMElement.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMDocument.h"
#include "nsIDOMDocumentXBL.h"
#include "nsIScrollable.h"
#include "nsINodeInfo.h"
#include "prenv.h"

// Global object maintenance
nsIXBLService * nsCSSFrameConstructor::gXBLService = nsnull;

#ifdef DEBUG
// Set the environment variable GECKO_FRAMECTOR_DEBUG_FLAGS to one or
// more of the following flags (comma separated) for handy debug
// output.
static PRBool gNoisyContentUpdates = PR_FALSE;
static PRBool gReallyNoisyContentUpdates = PR_FALSE;
static PRBool gNoisyInlineConstruction = PR_FALSE;
static PRBool gVerifyFastFindFrame = PR_FALSE;

struct FrameCtorDebugFlags {
  char*   name;
  PRBool* on;
};

static FrameCtorDebugFlags gFlags[] = {
  { "content-updates",              &gNoisyContentUpdates },
  { "really-noisy-content-updates", &gReallyNoisyContentUpdates },
  { "noisy-inline",                 &gNoisyInlineConstruction },
  { "fast-find-frame",              &gVerifyFastFindFrame },
};

#define NUM_DEBUG_FLAGS (sizeof(gFlags) / sizeof(gFlags[0]))
#endif


#ifdef INCLUDE_XUL
#include "nsXULAtoms.h"

#include "nsMenuFrame.h"
#include "nsPopupSetFrame.h"
#include "nsTreeColFrame.h"
#include "nsIBoxObject.h"
#include "nsIListBoxObject.h"
#include "nsListBoxBodyFrame.h"
#include "nsListItemFrame.h"

// To kill #define index(a,b) strchr(a,b) macro in Toolkit types.h
#ifdef XP_OS2_VACPP
#ifdef index
#undef index
#endif
#endif

//------------------------------------------------------------------

nsresult
NS_NewAutoRepeatBoxFrame ( nsIPresShell* aPresShell, nsIFrame** aNewFrame);

nsresult
NS_NewRootBoxFrame ( nsIPresShell* aPresShell, nsIFrame** aNewFrame);

nsresult
NS_NewDocElementBoxFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame);

nsresult
NS_NewThumbFrame ( nsIPresShell* aPresShell, nsIFrame** aNewFrame );

nsresult
NS_NewScrollPortFrame ( nsIPresShell* aPresShell, nsIFrame** aNewFrame );

nsresult
NS_NewGfxScrollFrame ( nsIPresShell* aPresShell, nsIFrame** aNewFrame, nsIDocument* aDocument, PRBool aIsRoot);

nsresult
NS_NewDeckFrame ( nsIPresShell* aPresShell, nsIFrame** aNewFrame, nsIBoxLayout* aLayoutManager = nsnull);

nsresult
NS_NewSpringFrame ( nsIPresShell* aPresShell, nsIFrame** aNewFrame );

nsresult
NS_NewStackFrame ( nsIPresShell* aPresShell, nsIFrame** aNewFrame, nsIBoxLayout* aLayoutManager = nsnull);

nsresult
NS_NewProgressMeterFrame ( nsIPresShell* aPresShell, nsIFrame** aNewFrame );

nsresult
NS_NewImageBoxFrame ( nsIPresShell* aPresShell, nsIFrame** aNewFrame );

nsresult
NS_NewTextBoxFrame ( nsIPresShell* aPresShell, nsIFrame** aNewFrame );

nsresult
NS_NewGroupBoxFrame ( nsIPresShell* aPresShell, nsIFrame** aNewFrame );

nsresult
NS_NewButtonBoxFrame ( nsIPresShell* aPresShell, nsIFrame** aNewFrame);

nsresult
NS_NewSliderFrame ( nsIPresShell* aPresShell, nsIFrame** aNewFrame );

nsresult
NS_NewScrollbarFrame ( nsIPresShell* aPresShell, nsIFrame** aNewFrame );

nsresult
NS_NewScrollbarButtonFrame ( nsIPresShell* aPresShell, nsIFrame** aNewFrame );

nsresult
NS_NewScrollbarFrame ( nsIPresShell* aPresShell, nsIFrame** aNewFrame );

nsresult
NS_NewNativeScrollbarFrame ( nsIPresShell* aPresShell, nsIFrame** aNewFrame );

nsresult
NS_NewGrippyFrame ( nsIPresShell* aPresShell, nsIFrame** aNewFrame );

nsresult
NS_NewSplitterFrame ( nsIPresShell* aPresShell, nsIFrame** aNewFrame );

nsresult
NS_NewMenuPopupFrame ( nsIPresShell* aPresShell, nsIFrame** aNewFrame );

nsresult
NS_NewPopupSetFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame);

nsresult
NS_NewScrollBoxFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame);

nsresult
NS_NewMenuFrame ( nsIPresShell* aPresShell, nsIFrame** aNewFrame, PRUint32 aFlags );

nsresult
NS_NewMenuBarFrame ( nsIPresShell* aPresShell, nsIFrame** aNewFrame );

nsresult
NS_NewListBoxScrollPortFrame ( nsIPresShell* aPresShell, nsIFrame** aNewFrame );

nsresult
NS_NewTreeBodyFrame (nsIPresShell* aPresShell, nsIFrame** aNewFrame);

// grid
nsresult
NS_NewGridLayout2 ( nsIPresShell* aPresShell, nsIBoxLayout** aNewLayout );
nsresult
NS_NewGridRowLeafLayout ( nsIPresShell* aPresShell, nsIBoxLayout** aNewLayout );
nsresult
NS_NewGridRowLeafFrame ( nsIPresShell* aPresShell, nsIFrame** aNewFrame, PRBool aIsRoot, nsIBoxLayout* aLayout);
nsresult
NS_NewGridRowGroupLayout ( nsIPresShell* aPresShell, nsIBoxLayout** aNewLayout );
nsresult
NS_NewGridRowGroupFrame ( nsIPresShell* aPresShell, nsIFrame** aNewFrame, PRBool aIsRoot, nsIBoxLayout* aLayout);

nsresult
NS_NewListBoxLayout ( nsIPresShell* aPresShell, nsCOMPtr<nsIBoxLayout>& aNewLayout );

// end grid

nsresult
NS_NewTitleBarFrame ( nsIPresShell* aPresShell, nsIFrame** aNewFrame);

nsresult
NS_NewResizerFrame ( nsIPresShell* aPresShell, nsIFrame** aNewFrame);


#endif

#ifdef NOISY_FINDFRAME
static PRInt32 FFWC_totalCount=0;
static PRInt32 FFWC_doLoop=0;
static PRInt32 FFWC_doSibling=0;
static PRInt32 FFWC_recursions=0;
static PRInt32 FFWC_nextInFlows=0;
static PRInt32 FFWC_slowSearchForText=0;
#endif

inline PRBool
HasNextSibling(nsIFrame *aFrame)
{
  nsIFrame *sib;
  aFrame->GetNextSibling(&sib);
  return sib != nsnull;
}

//----------------------------------------------------------------------
//
// When inline frames get weird and have block frames in them, we
// annotate them to help us respond to incremental content changes
// more easily.

static inline PRBool
IsFrameSpecial(nsIFrame* aFrame)
{
  nsFrameState state;
  aFrame->GetFrameState(&state);
  return state & NS_FRAME_IS_SPECIAL;
}

static void
GetSpecialSibling(nsIFrameManager* aFrameManager, nsIFrame* aFrame, nsIFrame** aResult)
{
  // We only store the "special sibling" annotation with the first
  // frame in the flow. Walk back to find that frame now.
  while (1) {
    nsIFrame* prev = aFrame;
    aFrame->GetPrevInFlow(&prev);
    if (! prev)
      break;
    aFrame = prev;
  }

  void* value;
  aFrameManager->GetFrameProperty(aFrame, nsLayoutAtoms::IBSplitSpecialSibling, 0, &value);
  *aResult = NS_STATIC_CAST(nsIFrame*, value);
}


static void
SetFrameIsSpecial(nsIFrameManager* aFrameManager, nsIFrame* aFrame, nsIFrame* aSpecialSibling)
{
  NS_PRECONDITION(aFrameManager && aFrame, "bad args!");

  // Mark the frame and all of its siblings as "special".
  for (nsIFrame* frame = aFrame; frame != nsnull; frame->GetNextInFlow(&frame)) {
    nsFrameState state;
    frame->GetFrameState(&state);
    state |= NS_FRAME_IS_SPECIAL;
    frame->SetFrameState(state);
  }

  if (aSpecialSibling) {
#ifdef DEBUG
    // We should be the first-in-flow
    nsIFrame* prev;
    aFrame->GetPrevInFlow(&prev);
    NS_ASSERTION(! prev, "assigning special sibling to other than first-in-flow!");
#endif

    // Store the "special sibling" (if we were given one) with the
    // first frame in the flow.
    aFrameManager->SetFrameProperty(aFrame,
                                    nsLayoutAtoms::IBSplitSpecialSibling,
                                    aSpecialSibling, nsnull);
  }
}

static nsIFrame*
GetIBContainingBlockFor(nsIFrame* aFrame)
{
  NS_PRECONDITION(IsFrameSpecial(aFrame),
                  "GetIBContainingBlockFor() should only be called on known IB frames");

  // Get the first "normal" ancestor of the target frame.
  nsIFrame* parentFrame;
  do {
    aFrame->GetParent(&parentFrame);

    if (! parentFrame) {
      NS_ERROR("no unsplit block frame in IB hierarchy");
      return aFrame;
    }

    if (!IsFrameSpecial(parentFrame))
      break;

    aFrame = parentFrame;
  } while (1);
 
  // post-conditions
  NS_ASSERTION(parentFrame, "no normal ancestor found for special frame in GetIBContainingBlockFor");
  NS_ASSERTION(parentFrame != aFrame, "parentFrame is actually the child frame - bogus reslt");

  return parentFrame;
}

//----------------------------------------------------------------------

// XXX this predicate and its cousins need to migrated to a single
// place in layout - something in nsStyleDisplay maybe?
static PRBool
IsInlineFrame(nsIFrame* aFrame)
{
  // XXXwaterson why don't we use |! display->IsBlockLevel()| here?
  const nsStyleDisplay* display;
  aFrame->GetStyleData(eStyleStruct_Display, (const nsStyleStruct*&) display);
  switch (display->mDisplay) {
    case NS_STYLE_DISPLAY_INLINE:
    case NS_STYLE_DISPLAY_INLINE_BLOCK:
    case NS_STYLE_DISPLAY_INLINE_TABLE:
    case NS_STYLE_DISPLAY_INLINE_BOX:
    case NS_STYLE_DISPLAY_INLINE_GRID:
    case NS_STYLE_DISPLAY_INLINE_STACK:
    case NS_STYLE_DISPLAY_DECK:
    case NS_STYLE_DISPLAY_POPUP:
    case NS_STYLE_DISPLAY_GROUPBOX:
      return PR_TRUE;
    default:
      break;
  }
  return PR_FALSE;
}

// NeedSpecialFrameReframe uses this until we decide what to do about IsInlineFrame() above
static PRBool
IsInlineFrame2(nsIFrame* aFrame)
{
  const nsStyleDisplay* display;
  aFrame->GetStyleData(eStyleStruct_Display, (const nsStyleStruct*&) display);
  return (display) ? !display->IsBlockLevel() : PR_TRUE;
}

//----------------------------------------------------------------------

// Block/inline frame construction logic. We maintain a few invariants here:
//
// 1. Block frames contain block and inline frames.
//
// 2. Inline frames only contain inline frames. If an inline parent has a block
// child then the block child is migrated upward until it lands in a block
// parent (the inline frames containing block is where it will end up).

// XXX consolidate these things
static PRBool
IsBlockFrame(nsIPresContext* aPresContext, nsIFrame* aFrame)
{
  // XXXwaterson this seems wrong; see IsInlineFrame() immediately
  // above, which will treat inline-block (e.g.) as an inline. Why
  // don't we use display->IsBlockLevel() here?
  const nsStyleDisplay* display;
  aFrame->GetStyleData(eStyleStruct_Display, (const nsStyleStruct*&) display);
  return NS_STYLE_DISPLAY_INLINE != display->mDisplay;
}

static nsIFrame*
FindFirstBlock(nsIPresContext* aPresContext, nsIFrame* aKid, nsIFrame** aPrevKid)
{
  nsIFrame* prevKid = nsnull;
  while (aKid) {
    if (IsBlockFrame(aPresContext, aKid)) {
      *aPrevKid = prevKid;
      return aKid;
    }
    prevKid = aKid;
    aKid->GetNextSibling(&aKid);
  }
  *aPrevKid = nsnull;
  return nsnull;
}

static nsIFrame*
FindLastBlock(nsIPresContext* aPresContext, nsIFrame* aKid)
{
  nsIFrame* lastBlock = nsnull;
  while (aKid) {
    if (IsBlockFrame(aPresContext, aKid)) {
      lastBlock = aKid;
    }
    aKid->GetNextSibling(&aKid);
  }
  return lastBlock;
}

/*
 * Unlike the special (next) sibling, the special previous sibling
 * property points only from the anonymous block to the original
 * inline that preceded it.  DO NOT CHANGE THAT -- the
 * GetParentStyleContextFrame code depends on it!  It is useful for
 * finding the "special parent" of a frame (i.e., a frame from which a
 * good parent style context can be obtained), one looks at the
 * special previous sibling annotation of the real parent of the frame
 * (if the real parent has NS_FRAME_IS_SPECIAL).
 */
inline void
MarkIBSpecialPrevSibling(nsIPresContext* aPresContext,
                         nsIFrameManager *aFrameManager,
                         nsIFrame *aAnonymousFrame,
                         nsIFrame *aSpecialParent)
{
  aFrameManager->SetFrameProperty(aAnonymousFrame,
                                  nsLayoutAtoms::IBSplitSpecialPrevSibling,
                                  aSpecialParent,
                                  nsnull);
}

// -----------------------------------------------------------

// Structure used when constructing formatting object trees.
struct nsFrameItems {
  nsIFrame* childList;
  nsIFrame* lastChild;
  
  nsFrameItems(nsIFrame* aFrame = nsnull);

  // Appends the frame to the end of the list
  void AddChild(nsIFrame* aChild);
};

nsFrameItems::nsFrameItems(nsIFrame* aFrame)
  : childList(aFrame), lastChild(aFrame)
{
}

void 
nsFrameItems::AddChild(nsIFrame* aChild)
{
  if (childList == nsnull) {
    childList = lastChild = aChild;
  }
  else
  {
    lastChild->SetNextSibling(aChild);
    lastChild = aChild;
  }
  // if aChild has siblings, lastChild needs to be the last one
  nsIFrame* sib;
  for (lastChild->GetNextSibling(&sib); sib; sib->GetNextSibling(&sib)) {
    lastChild = sib;
  }
}

// -----------------------------------------------------------

// Structure used when constructing formatting object trees. Contains
// state information needed for absolutely positioned elements
struct nsAbsoluteItems : nsFrameItems {
  // containing block for absolutely positioned elements
  nsIFrame* containingBlock;

  nsAbsoluteItems(nsIFrame* aContainingBlock = nsnull);

  // Appends the frame to the end of the list
  void AddChild(nsIFrame* aChild);
};

nsAbsoluteItems::nsAbsoluteItems(nsIFrame* aContainingBlock)
  : containingBlock(aContainingBlock)
{
}

// Additional behavior is that it sets the frame's NS_FRAME_OUT_OF_FLOW flag
void
nsAbsoluteItems::AddChild(nsIFrame* aChild)
{
  nsFrameState  frameState;

  // Mark the frame as being moved out of the flow
  aChild->GetFrameState(&frameState);
  aChild->SetFrameState(frameState | NS_FRAME_OUT_OF_FLOW);
  nsFrameItems::AddChild(aChild);
}

// Structures used to record the creation of pseudo table frames where 
// the content belongs to some ancestor. 

struct nsPseudoFrameData {
  nsIFrame*    mFrame;
  nsFrameItems mChildList;
  nsFrameItems mChildList2;

  nsPseudoFrameData();
  nsPseudoFrameData(nsPseudoFrameData& aOther);
  void Reset();
};

struct nsPseudoFrames {
  nsPseudoFrameData mTableOuter; 
  nsPseudoFrameData mTableInner;  
  nsPseudoFrameData mRowGroup;   
  nsPseudoFrameData mColGroup;
  nsPseudoFrameData mRow;   
  nsPseudoFrameData mCellOuter;
  nsPseudoFrameData mCellInner;

  // the frame type of the most descendant pseudo frame, no AddRef
  nsIAtom*          mLowestType;

  nsPseudoFrames();
  nsPseudoFrames& operator=(const nsPseudoFrames& aOther);
  void Reset(nsPseudoFrames* aSave = nsnull);
  PRBool IsEmpty() { return (!mLowestType && !mColGroup.mFrame); }
};

nsPseudoFrameData::nsPseudoFrameData()
: mFrame(nsnull), mChildList(), mChildList2()
{}

nsPseudoFrameData::nsPseudoFrameData(nsPseudoFrameData& aOther)
: mFrame(aOther.mFrame), mChildList(aOther.mChildList), 
  mChildList2(aOther.mChildList2)
{}

void
nsPseudoFrameData::Reset()
{
  mFrame = nsnull;
  mChildList.childList  = mChildList.lastChild  = nsnull;
  mChildList2.childList = mChildList2.lastChild = nsnull;
}

nsPseudoFrames::nsPseudoFrames() 
: mTableOuter(), mTableInner(), mRowGroup(), mColGroup(), 
  mRow(), mCellOuter(), mCellInner(), mLowestType(nsnull)
{}

nsPseudoFrames& nsPseudoFrames::operator=(const nsPseudoFrames& aOther)
{
  mTableOuter = aOther.mTableOuter;
  mTableInner = aOther.mTableInner;
  mColGroup   = aOther.mColGroup;
  mRowGroup   = aOther.mRowGroup;
  mRow        = aOther.mRow;
  mCellOuter  = aOther.mCellOuter;
  mCellInner  = aOther.mCellInner;
  mLowestType = aOther.mLowestType;

  return *this;
}
void
nsPseudoFrames::Reset(nsPseudoFrames* aSave) 
{
  if (aSave) {
    *aSave = *this;
  }

  mTableOuter.Reset();
  mTableInner.Reset();
  mColGroup.Reset();
  mRowGroup.Reset();
  mRow.Reset();
  mCellOuter.Reset();
  mCellInner.Reset();
  mLowestType = nsnull;
}

// -----------------------------------------------------------

// Structure for saving the existing state when pushing/poping containing
// blocks. The destructor restores the state to its previous state
class nsFrameConstructorSaveState {
public:
  nsFrameConstructorSaveState();
  ~nsFrameConstructorSaveState();

private:
  nsAbsoluteItems* mItems;                // pointer to struct whose data we save/restore
  PRBool*          mFirstLetterStyle;
  PRBool*          mFirstLineStyle;

  nsAbsoluteItems  mSavedItems;           // copy of original data
  PRBool           mSavedFirstLetterStyle;
  PRBool           mSavedFirstLineStyle;

  friend class nsFrameConstructorState;
};

// Structure used for maintaining state information during the
// frame construction process
class nsFrameConstructorState {
public:
  nsCOMPtr<nsIPresShell>    mPresShell;
  nsCOMPtr<nsIFrameManager> mFrameManager;

  // Containing block information for out-of-flow frammes
  nsAbsoluteItems           mFixedItems;
  nsAbsoluteItems           mAbsoluteItems;
  nsAbsoluteItems           mFloatedItems;
  PRBool                    mFirstLetterStyle;
  PRBool                    mFirstLineStyle;
  nsCOMPtr<nsILayoutHistoryState> mFrameState;
  nsPseudoFrames            mPseudoFrames;

  // Constructor
  // Use the passed-in history state.
  nsFrameConstructorState(nsIPresContext*        aPresContext,
                          nsIFrame*              aFixedContainingBlock,
                          nsIFrame*              aAbsoluteContainingBlock,
                          nsIFrame*              aFloaterContainingBlock,
                          nsILayoutHistoryState* aHistoryState);
  // Get the history state from the pres context's pres shell.
  nsFrameConstructorState(nsIPresContext*        aPresContext,
                          nsIFrame*              aFixedContainingBlock,
                          nsIFrame*              aAbsoluteContainingBlock,
                          nsIFrame*              aFloaterContainingBlock);

  // Function to push the existing absolute containing block state and
  // create a new scope
  void PushAbsoluteContainingBlock(nsIFrame* aNewAbsoluteContainingBlock,
                                   nsFrameConstructorSaveState& aSaveState);

  // Function to push the existing floater containing block state and
  // create a new scope
  void PushFloaterContainingBlock(nsIFrame* aNewFloaterContainingBlock,
                                  nsFrameConstructorSaveState& aSaveState,
                                  PRBool aFirstLetterStyle,
                                  PRBool aFirstLineStyle);
};

nsFrameConstructorState::nsFrameConstructorState(nsIPresContext*        aPresContext,
                                                 nsIFrame*              aFixedContainingBlock,
                                                 nsIFrame*              aAbsoluteContainingBlock,
                                                 nsIFrame*              aFloaterContainingBlock,
                                                 nsILayoutHistoryState* aHistoryState)
  : mFixedItems(aFixedContainingBlock),
    mAbsoluteItems(aAbsoluteContainingBlock),
    mFloatedItems(aFloaterContainingBlock),
    mFirstLetterStyle(PR_FALSE),
    mFirstLineStyle(PR_FALSE),
    mFrameState(aHistoryState),
    mPseudoFrames()
{
  aPresContext->GetShell(getter_AddRefs(mPresShell));
  mPresShell->GetFrameManager(getter_AddRefs(mFrameManager));
}

nsFrameConstructorState::nsFrameConstructorState(nsIPresContext*        aPresContext,
                                                 nsIFrame*              aFixedContainingBlock,
                                                 nsIFrame*              aAbsoluteContainingBlock,
                                                 nsIFrame*              aFloaterContainingBlock)
  : mFixedItems(aFixedContainingBlock),
    mAbsoluteItems(aAbsoluteContainingBlock),
    mFloatedItems(aFloaterContainingBlock),
    mFirstLetterStyle(PR_FALSE),
    mFirstLineStyle(PR_FALSE),
    mPseudoFrames()
{
  aPresContext->GetShell(getter_AddRefs(mPresShell));
  mPresShell->GetFrameManager(getter_AddRefs(mFrameManager));
  mPresShell->GetHistoryState(getter_AddRefs(mFrameState));
}

void
nsFrameConstructorState::PushAbsoluteContainingBlock(nsIFrame* aNewAbsoluteContainingBlock,
                                                     nsFrameConstructorSaveState& aSaveState)
{
  aSaveState.mItems = &mAbsoluteItems;
  aSaveState.mSavedItems = mAbsoluteItems;
  mAbsoluteItems = nsAbsoluteItems(aNewAbsoluteContainingBlock);
}

void
nsFrameConstructorState::PushFloaterContainingBlock(nsIFrame* aNewFloaterContainingBlock,
                                                    nsFrameConstructorSaveState& aSaveState,
                                                    PRBool aFirstLetterStyle,
                                                    PRBool aFirstLineStyle)
{
  aSaveState.mItems = &mFloatedItems;
  aSaveState.mFirstLetterStyle = &mFirstLetterStyle;
  aSaveState.mFirstLineStyle = &mFirstLineStyle;
  aSaveState.mSavedItems = mFloatedItems;
  aSaveState.mSavedFirstLetterStyle = mFirstLetterStyle;
  aSaveState.mSavedFirstLineStyle = mFirstLineStyle;
  mFloatedItems = nsAbsoluteItems(aNewFloaterContainingBlock);
  mFirstLetterStyle = aFirstLetterStyle;
  mFirstLineStyle = aFirstLineStyle;
}

nsFrameConstructorSaveState::nsFrameConstructorSaveState()
  : mItems(nsnull), mFirstLetterStyle(nsnull), mFirstLineStyle(nsnull), 
    mSavedFirstLetterStyle(PR_FALSE), mSavedFirstLineStyle(PR_FALSE)
{
}

nsFrameConstructorSaveState::~nsFrameConstructorSaveState()
{
  // Restore the state
  if (mItems) {
    *mItems = mSavedItems;
  }
  if (mFirstLetterStyle) {
    *mFirstLetterStyle = mSavedFirstLetterStyle;
  }
  if (mFirstLineStyle) {
    *mFirstLineStyle = mSavedFirstLineStyle;
  }
}


static 
PRBool IsBorderCollapse(nsIFrame* aFrame)
{
  nsIFrame* frame = aFrame;
  while (frame) {
    nsCOMPtr<nsIAtom> fType;
    frame->GetFrameType(getter_AddRefs(fType));
    if (nsLayoutAtoms::tableFrame == fType.get()) {
      return ((nsTableFrame*)frame)->IsBorderCollapse();
    }
    frame->GetParent(&frame);
  }
  NS_ASSERTION(PR_FALSE, "program error");
  return PR_FALSE;
}

// a helper routine that automatically navigates placeholders.
static nsIFrame*
GetRealFrame(nsIFrame* aFrame)
{
  nsIFrame* result = aFrame;

  // We may be a placeholder.  If we are, go to the real frame.
  nsCOMPtr<nsIAtom>  frameType;
  
  // See if it's a placeholder frame for a floater.
  aFrame->GetFrameType(getter_AddRefs(frameType));
  PRBool isPlaceholder = (nsLayoutAtoms::placeholderFrame == frameType.get());
  if (isPlaceholder) {
    // Get the out-of-flow frame that the placeholder points to.
    // This is the real floater that we should examine.
    result = NS_STATIC_CAST(nsPlaceholderFrame*,aFrame)->GetOutOfFlowFrame();
    NS_ASSERTION(result, "No out of flow frame found for placeholder!\n");
  }
  
  return result;
}

/**
 * Utility method, called from MoveChildrenTo(), that recursively
 * descends down the frame hierarchy looking for out-of-flow frames that
 * need parent pointer adjustments to account for the containment block
 * changes that could occur as the result of the reparenting done in
 * MoveChildrenTo().
 */
static void
AdjustOutOfFlowFrameParentPtrs(nsIPresContext*          aPresContext,
                               nsIFrame*                aFrame,
                               nsFrameConstructorState* aState)
{
  nsIFrame *outOfFlowFrame = GetRealFrame(aFrame);

  if (outOfFlowFrame && outOfFlowFrame != aFrame) {

    // Get the display data for the outOfFlowFrame so we can
    // figure out if it is a floater or absolutely positioned element.

    const nsStyleDisplay* display = nsnull;
    outOfFlowFrame->GetStyleData(eStyleStruct_Display,
                                (const nsStyleStruct*&)display);
 
    if (!display) {
      NS_WARNING("outOfFlowFrame has no display data!");
      return;
    }

    // Update the parent pointer for outOfFlowFrame if it's
    // containing block has changed as the result of reparenting,
    //
    // XXX_kin: I don't think we have to worry about
    // XXX_kin: NS_STYLE_POSITION_FIXED or NS_STYLE_POSITION_RELATIVE.

    if (NS_STYLE_POSITION_ABSOLUTE == display->mPosition) {
      // XXX_kin: I think we'll need to add code here to handle the
      // XXX_kin: reparenting that can happen in ConstructInline()?
      // XXX_kin:
      // XXX_kin: The case I'm thinking about here is when the inline being
      // XXX_kin: constructed has a style="position: relative;" property
      // XXX_kin: on it, and ConstructInline() moves/reparents all child block
      // XXX_kin: frames and any inlines (including placeholders) that happen
      // XXX_kin: to be between these blocks, under the new inline-block it created.
      // XXX_kin: I think right now this case generates an assertion during
      // XXX_kin: reflow, and as a result things fail to render since I believe
      // XXX_kin: the placeholder is parented to the inline-block, and the
      // XXX_kin: outOfFlowFrame is in the original inline frame's absolute list,
      // XXX_kin: and is also parented to it.
    }
    else if (NS_STYLE_FLOAT_NONE != display->mFloats) {
      outOfFlowFrame->SetParent(aState->mFloatedItems.containingBlock);
    }

    // XXX_kin: We'll need to remove the return below when we support
    // XXX_kin: descending into out-of-flow frames. Here are some notes
    // XXX_kin: from waterson:
    // XXX_kin:
    // XXX_kin:     We'd want to continue to descend into placeholders
    // XXX_kin:     until we found an out-of-flow frame that established
    // XXX_kin:     a new containing block for absolutely positioned
    // XXX_kin:     elements. At that point, we could terminate the descent,
    // XXX_kin:     because any absolutely positioned frames below that frame
    // XXX_kin:     would be properly parented.

    return;
  }

  // XXX_kin: Since we're only handling floaters at the moment,
  // XXX_kin: we don't need to cross block boundaries.

  if (IsBlockFrame(aPresContext, aFrame))
    return;

  // Dive down into children to see if any of their
  // placeholders need adjusting.

  nsIFrame *childFrame = nsnull;
  aFrame->FirstChild(aPresContext, nsnull, &childFrame);

  while (childFrame)
  {
    // XXX_kin: Once we add support for adjusting absolutely positioned
    // XXX_kin: frames, we will be crossing block boundaries, we/ll need
    // XXX_kin: to update aState's containingBlock info to avoid incorrectly
    // XXX_kin: reparenting floaters, etc.
    // XXX_kin:
    // XXX_kin: Do we need to prevent descent into anonymous content here?

    AdjustOutOfFlowFrameParentPtrs(aPresContext, childFrame, aState);
    childFrame->GetNextSibling(&childFrame);
  }
}

/**
 * Moves frames to a new parent, updating the style context and
 * propagating relevant frame state bits. |aNewParentSC| may be null,
 * in which case the child frames' style contexts will remain
 * untouched. |aState| may be null, in which case the parent
 * pointers of out-of-flow frames will remain untouched.
 */
static void
MoveChildrenTo(nsIPresContext*          aPresContext,
               nsIStyleContext*         aNewParentSC,
               nsIFrame*                aNewParent,
               nsIFrame*                aFrameList,
               nsFrameConstructorState* aState)
{
  PRBool setHasChildWithView = PR_FALSE;

  while (aFrameList) {
    if (! setHasChildWithView) {
      nsFrameState state;
      aFrameList->GetFrameState(&state);
      if (state & (NS_FRAME_HAS_VIEW | NS_FRAME_HAS_CHILD_WITH_VIEW))
        setHasChildWithView = PR_TRUE;
    }

    aFrameList->SetParent(aNewParent);

    // If aState is not null, the caller expects us to make adjustments
    // so that placeholder out of flow frames point to the correct parent.

    if (aState)
      AdjustOutOfFlowFrameParentPtrs(aPresContext, aFrameList, aState);

#if 0
    // XXX When this is used with {ib} frame hierarchies, it seems
    // fine to leave the style contexts of the children of the
    // anonymous block frame parented by the original inline
    // frame. (In fact, one would expect some inheritance
    // relationships to be broken if we reparented them to the
    // anonymous block frame, but oddly they aren't -- need to
    // investigate that...)
    if (aNewParentSC)
      aPresContext->ReParentStyleContext(aFrameList, aNewParentSC);
#endif

    aFrameList->GetNextSibling(&aFrameList);
  }

  if (setHasChildWithView) {
    nsFrameState state;
    aNewParent->GetFrameState(&state);
    state |= NS_FRAME_HAS_CHILD_WITH_VIEW;
    aNewParent->SetFrameState(state);
  }
}

// -----------------------------------------------------------

// Structure used when creating table frames.
struct nsTableCreator {
  virtual nsresult CreateTableOuterFrame(nsIFrame** aNewFrame);
  virtual nsresult CreateTableFrame(nsIFrame** aNewFrame);
  virtual nsresult CreateTableCaptionFrame(nsIFrame** aNewFrame);
  virtual nsresult CreateTableRowGroupFrame(nsIFrame** aNewFrame);
  virtual nsresult CreateTableColFrame(nsIFrame** aNewFrame);
  virtual nsresult CreateTableColGroupFrame(nsIFrame** aNewFrame);
  virtual nsresult CreateTableRowFrame(nsIFrame** aNewFrame);
  virtual nsresult CreateTableCellFrame(nsIFrame* aParentFrame, nsIFrame** aNewFrame);
  virtual nsresult CreateTableCellInnerFrame(nsIFrame** aNewFrame);

  nsTableCreator(nsIPresShell* aPresShell)
  {
    mPresShell = aPresShell;
  }

  virtual ~nsTableCreator() {};

  nsCOMPtr<nsIPresShell> mPresShell;
};

nsresult
nsTableCreator::CreateTableOuterFrame(nsIFrame** aNewFrame) {
  return NS_NewTableOuterFrame(mPresShell, aNewFrame);
}

nsresult
nsTableCreator::CreateTableFrame(nsIFrame** aNewFrame) {
  return NS_NewTableFrame(mPresShell, aNewFrame);
}

nsresult
nsTableCreator::CreateTableCaptionFrame(nsIFrame** aNewFrame) {
  return NS_NewTableCaptionFrame(mPresShell, aNewFrame);
}

nsresult
nsTableCreator::CreateTableRowGroupFrame(nsIFrame** aNewFrame) {
  return NS_NewTableRowGroupFrame(mPresShell, aNewFrame);
}

nsresult
nsTableCreator::CreateTableColFrame(nsIFrame** aNewFrame) {
  return NS_NewTableColFrame(mPresShell, aNewFrame);
}

nsresult
nsTableCreator::CreateTableColGroupFrame(nsIFrame** aNewFrame) {
  return NS_NewTableColGroupFrame(mPresShell, aNewFrame);
}

nsresult
nsTableCreator::CreateTableRowFrame(nsIFrame** aNewFrame) {
  return NS_NewTableRowFrame(mPresShell, aNewFrame);
}

nsresult
nsTableCreator::CreateTableCellFrame(nsIFrame*  aParentFrame,
                                     nsIFrame** aNewFrame) {
  return NS_NewTableCellFrame(mPresShell, IsBorderCollapse(aParentFrame), aNewFrame);
}

nsresult
nsTableCreator::CreateTableCellInnerFrame(nsIFrame** aNewFrame) {
  return NS_NewTableCellInnerFrame(mPresShell, aNewFrame);
}

//MathML Mod - RBS
#ifdef MOZ_MATHML

// Structure used when creating MathML mtable frames
struct nsMathMLmtableCreator: public nsTableCreator {
  virtual nsresult CreateTableOuterFrame(nsIFrame** aNewFrame);
  virtual nsresult CreateTableFrame(nsIFrame** aNewFrame);
  virtual nsresult CreateTableRowFrame(nsIFrame** aNewFrame);
  virtual nsresult CreateTableCellFrame(nsIFrame* aParentFrame, nsIFrame** aNewFrame);
  virtual nsresult CreateTableCellInnerFrame(nsIFrame** aNewFrame);

  nsMathMLmtableCreator(nsIPresShell* aPresShell)
    :nsTableCreator(aPresShell) {};
};

nsresult
nsMathMLmtableCreator::CreateTableOuterFrame(nsIFrame** aNewFrame)
{
  return NS_NewMathMLmtableOuterFrame(mPresShell, aNewFrame);
}

nsresult
nsMathMLmtableCreator::CreateTableFrame(nsIFrame** aNewFrame)
{
  return NS_NewMathMLmtableFrame(mPresShell, aNewFrame);
}

nsresult
nsMathMLmtableCreator::CreateTableRowFrame(nsIFrame** aNewFrame)
{
  return NS_NewMathMLmtrFrame(mPresShell, aNewFrame);
}

nsresult
nsMathMLmtableCreator::CreateTableCellFrame(nsIFrame*  aParentFrame,
                                            nsIFrame** aNewFrame)
{
  NS_ASSERTION(!IsBorderCollapse(aParentFrame), "not implemented");
  return NS_NewMathMLmtdFrame(mPresShell, aNewFrame);
}

nsresult
nsMathMLmtableCreator::CreateTableCellInnerFrame(nsIFrame** aNewFrame)
{
  // only works if aNewFrame can take care of the lineLayout logic
  return NS_NewMathMLmtdInnerFrame(mPresShell, aNewFrame);
}
#endif // MOZ_MATHML

/**
 * If the parent frame is a |tableFrame| and the child is a
 * |captionFrame|, then we want to insert the frames beneath the
 * |tableFrame|'s parent frame. Returns |PR_TRUE| if the parent frame
 * needed to be fixed up.
 */
static PRBool
GetCaptionAdjustedParent(nsIFrame*        aParentFrame,
                         const nsIFrame*  aChildFrame,
                         nsIFrame**       aAdjParentFrame)
{
  *aAdjParentFrame = aParentFrame;
  PRBool haveCaption = PR_FALSE;
  nsCOMPtr<nsIAtom> childFrameType;
  aChildFrame->GetFrameType(getter_AddRefs(childFrameType));

  if (nsLayoutAtoms::tableCaptionFrame == childFrameType.get()) { 
    haveCaption = PR_TRUE;
    nsCOMPtr<nsIAtom> parentFrameType;
    aParentFrame->GetFrameType(getter_AddRefs(parentFrameType));
    if (nsLayoutAtoms::tableFrame == parentFrameType.get()) {
      aParentFrame->GetParent(aAdjParentFrame);
    }
  }
  return haveCaption;
}

// Helper function that determines the child list name that aChildFrame
// is contained in
static void
GetChildListNameFor(nsIPresContext* aPresContext,
                    nsIFrame*       aParentFrame,
                    nsIFrame*       aChildFrame,
                    nsIAtom**       aListName)
{
  nsFrameState  frameState;
  nsIAtom*      listName;
  
  // See if the frame is moved out of the flow
  aChildFrame->GetFrameState(&frameState);
  if (frameState & NS_FRAME_OUT_OF_FLOW) {
    // Look at the style information to tell
    const nsStyleDisplay* disp;
    aChildFrame->GetStyleData(eStyleStruct_Display, (const nsStyleStruct*&)disp);
    
    if (NS_STYLE_POSITION_ABSOLUTE == disp->mPosition) {
      listName = nsLayoutAtoms::absoluteList;
    } else if (NS_STYLE_POSITION_FIXED == disp->mPosition) {
      listName = nsLayoutAtoms::fixedList;
    } else {
#ifdef NS_DEBUG
      const nsStyleDisplay* display;
      aChildFrame->GetStyleData(eStyleStruct_Display, (const nsStyleStruct*&)display);
      NS_ASSERTION(display->IsFloating(), "not a floated frame");
#endif
      listName = nsLayoutAtoms::floaterList;
    }

  } else {
    listName = nsnull;
  }

  // Verify that the frame is actually in that child list
#ifdef NS_DEBUG 
  nsIFrame* firstChild;
  aParentFrame->FirstChild(aPresContext, listName, &firstChild);

  nsFrameList frameList(firstChild);
  NS_ASSERTION(frameList.ContainsFrame(aChildFrame), "not in child list");
#endif

  NS_IF_ADDREF(listName);
  *aListName = listName;
}

//----------------------------------------------------------------------

nsresult NS_CreateCSSFrameConstructor(nsICSSFrameConstructor **aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = nsnull;

  nsCSSFrameConstructor *c = new nsCSSFrameConstructor();
  if (!c)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(c);
  nsresult rv = CallQueryInterface(c, aResult);
  NS_RELEASE(c);

  return rv;
}

nsCSSFrameConstructor::nsCSSFrameConstructor(void)
  : nsIStyleFrameConstruction(),
    mDocument(nsnull),
    mInitialContainingBlock(nsnull),
    mFixedContainingBlock(nsnull),
    mDocElementContainingBlock(nsnull),
    mGfxScrollFrame(nsnull)
{
  NS_INIT_ISUPPORTS();

#ifdef DEBUG
  static PRBool gFirstTime = PR_TRUE;
  if (gFirstTime) {
    gFirstTime = PR_FALSE;
    char* flags = PR_GetEnv("GECKO_FRAMECTOR_DEBUG_FLAGS");
    if (flags) {
      PRBool error = PR_FALSE;
      for (;;) {
        char* comma = PL_strchr(flags, ',');
        if (comma)
          *comma = '\0';

        PRBool found = PR_FALSE;
        FrameCtorDebugFlags* flag = gFlags;
        FrameCtorDebugFlags* limit = gFlags + NUM_DEBUG_FLAGS;
        while (flag < limit) {
          if (PL_strcasecmp(flag->name, flags) == 0) {
            *(flag->on) = PR_TRUE;
            printf("nsCSSFrameConstructor: setting %s debug flag on\n", flag->name);
            found = PR_TRUE;
            break;
          }
          ++flag;
        }

        if (! found)
          error = PR_TRUE;

        if (! comma)
          break;

        *comma = ',';
        flags = comma + 1;
      }

      if (error) {
        printf("Here are the available GECKO_FRAMECTOR_DEBUG_FLAGS:\n");
        FrameCtorDebugFlags* flag = gFlags;
        FrameCtorDebugFlags* limit = gFlags + NUM_DEBUG_FLAGS;
        while (flag < limit) {
          printf("  %s\n", flag->name);
          ++flag;
        }
        printf("Note: GECKO_FRAMECTOR_DEBUG_FLAGS is a comma separated list of flag\n");
        printf("names (no whitespace)\n");
      }
    }
  }
#endif
}

nsCSSFrameConstructor::~nsCSSFrameConstructor(void)
{
}

NS_IMPL_ISUPPORTS2(nsCSSFrameConstructor, nsIStyleFrameConstruction,nsICSSFrameConstructor);

NS_IMETHODIMP 
nsCSSFrameConstructor::Init(nsIDocument* aDocument)
{
  NS_PRECONDITION(aDocument, "null ptr");
  if (! aDocument)
    return NS_ERROR_NULL_POINTER;

  if (mDocument)
    return NS_ERROR_ALREADY_INITIALIZED;

  mDocument = aDocument; // not refcounted!

  // This initializes the Prefs booleans
  mGotGfxPrefs = PR_FALSE;
  mGotXBLFormPrefs = PR_FALSE;
  mHasGfxScrollbars = PR_FALSE;
  mUseXBLForms = PR_FALSE;

  HasGfxScrollbars();
  UseXBLForms();

  return NS_OK;
}

nsresult
nsCSSFrameConstructor::CreateGeneratedFrameFor(nsIPresContext*       aPresContext,
                                               nsIDocument*          aDocument,
                                               nsIFrame*             aParentFrame,
                                               nsIContent*           aContent,
                                               nsIStyleContext*      aStyleContext,
                                               const nsStyleContent* aStyleContent,
                                               PRUint32              aContentIndex,
                                               nsIFrame**            aFrame)
{
  *aFrame = nsnull;  // initialize OUT parameter

  // Get the content value
  nsStyleContentType  type;
  nsAutoString        contentString;
  aStyleContent->GetContentAt(aContentIndex, type, contentString);

  nsCOMPtr<nsIPresShell> shell;
  aPresContext->GetShell(getter_AddRefs(shell));

  if (eStyleContentType_URL == type) {
    // Create an HTML image content object, and set the SRC.
    // XXX Check if it's an image type we can handle...

    nsCOMPtr<nsINodeInfoManager> nimgr;
    nsresult rv = aDocument->GetNodeInfoManager(*getter_AddRefs(nimgr));
    NS_ENSURE_SUCCESS(rv, rv);

    // XXXldb We should not be creating an |image| element, because it
    // matches selectors!  See bug 109216.
    nsCOMPtr<nsINodeInfo> nodeInfo;
    nimgr->GetNodeInfo(nsHTMLAtoms::img, nsnull, kNameSpaceID_None,
                       *getter_AddRefs(nodeInfo));

    nsCOMPtr<nsIElementFactory> ef(do_CreateInstance(kHTMLElementFactoryCID,&rv));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIContent> content;
    rv = ef->CreateInstanceByTag(nodeInfo,getter_AddRefs(content));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIHTMLContent> imageContent(do_QueryInterface(content,&rv));
    NS_ENSURE_SUCCESS(rv, rv);
    
    imageContent->SetHTMLAttribute(nsHTMLAtoms::src, contentString, PR_FALSE);

    // Set aContent as the parent content and set the document object. This
    // way event handling works
    imageContent->SetParent(aContent);
    imageContent->SetDocument(aDocument, PR_TRUE, PR_TRUE);
  
    // Create an image frame and initialize it
    nsIFrame* imageFrame;
    NS_NewImageFrame(shell, &imageFrame);
    imageFrame->Init(aPresContext, imageContent, aParentFrame, aStyleContext, nsnull);
  
    // Return the image frame
    *aFrame = imageFrame;

  } else {

    switch (type) {
    case eStyleContentType_String:
      break;
  
    case eStyleContentType_Attr:
      {  
        nsIAtom* attrName = nsnull;
        PRInt32 attrNameSpace = kNameSpaceID_None;
        PRInt32 barIndex = contentString.FindChar('|'); // CSS namespace delimiter
        if (-1 != barIndex) {
          nsAutoString  nameSpaceVal;
          contentString.Left(nameSpaceVal, barIndex);
          PRInt32 error;
          attrNameSpace = nameSpaceVal.ToInteger(&error, 10);
          contentString.Cut(0, barIndex + 1);
          if (contentString.Length()) {
            attrName = NS_NewAtom(contentString);
          }
        }
        else {
          attrName = NS_NewAtom(contentString);
        }

        // Creates the content and frame and return if successful
        nsresult rv = NS_ERROR_FAILURE;
        if (nsnull != attrName) {
          nsIFrame*   textFrame = nsnull;
          nsCOMPtr<nsIContent> content(do_CreateInstance(kAttributeContentCID));
          if (content) {
            nsCOMPtr<nsIAttributeContent> attrContent(do_QueryInterface(content));
            if (attrContent) {
              attrContent->Init(aContent, attrNameSpace, attrName);  
            }

            // Set aContent as the parent content and set the document object. This
            // way event handling works
            content->SetParent(aContent);
            content->SetDocument(aDocument, PR_TRUE, PR_TRUE);

            // Create a text frame and initialize it
            NS_NewTextFrame(shell, &textFrame);
            textFrame->Init(aPresContext, content, aParentFrame, aStyleContext, nsnull);

            // Return the text frame
            *aFrame = textFrame;
            rv = NS_OK;
          }
          NS_RELEASE(attrName);
        }
        return rv;
      }
      break;
  
    case eStyleContentType_Counter:
    case eStyleContentType_Counters:
    case eStyleContentType_URL:
      return NS_ERROR_NOT_IMPLEMENTED;  // XXX not supported yet...
  
    case eStyleContentType_OpenQuote:
    case eStyleContentType_CloseQuote:
      {
        const nsStyleQuotes* quotes = (const nsStyleQuotes*)aStyleContext->GetStyleData(eStyleStruct_Quotes);
        PRUint32  quotesCount = quotes->QuotesCount();
        if (quotesCount > 0) {
          nsAutoString  openQuote, closeQuote;
  
          // If the depth is greater than the number of pairs, the last pair
          // is repeated
          PRUint32  quoteDepth = 0;  // XXX really track the nested quotes...
          if (quoteDepth > quotesCount) {
            quoteDepth = quotesCount - 1;
          }
          quotes->GetQuotesAt(quoteDepth, openQuote, closeQuote);
          if (eStyleContentType_OpenQuote == type) {
            contentString = openQuote;
          } else {
            contentString = closeQuote;
          }
  
        } else {
          // XXX Don't assume default. Only use what is in 'quotes' property
          contentString.Assign(PRUnichar('\"'));
        }
      }
      break;
  
    case eStyleContentType_NoOpenQuote:
    case eStyleContentType_NoCloseQuote:
      // XXX Adjust quote depth...
      return NS_OK;
    } // switch
  

    // Create a text content node
    nsIFrame* textFrame = nsnull;
    nsCOMPtr<nsIContent> textContent = do_CreateInstance(kTextNodeCID);
    if (textContent) {
      // Set the text
      nsCOMPtr<nsIDOMCharacterData> domData = do_QueryInterface(textContent);
      if (domData)
        domData->SetData(contentString);
  
      // Set aContent as the parent content and set the document object. This
      // way event handling works
      textContent->SetParent(aContent);
      textContent->SetDocument(aDocument, PR_TRUE, PR_TRUE);
      
      // Create a text frame and initialize it
      NS_NewTextFrame(shell, &textFrame);
      textFrame->Init(aPresContext, textContent, aParentFrame, aStyleContext, nsnull);
    }
  
    // Return the text frame
    *aFrame = textFrame;
  }

  return NS_OK;
}

/*
 *
 * aFrame - the frame that should be the parent of the generated
 *   content.  For non-leaf frames, this is the frame for the
 *   corresponding content node.  For leaf frames (those with non-null
 *   |aWrapperFrame|), this is the parent.
 * aWrapperFrame - inout parameter, which may be null.  If non-null,
 *   then the generated content being created is for a leaf frame so
 *   that the generated content requires a wrapper frame, which we must
 *   create if it hasn't been already.
 */
PRBool
nsCSSFrameConstructor::CreateGeneratedContentFrame(nsIPresShell*        aPresShell, 
                                                   nsIPresContext*  aPresContext,
                                                   nsFrameConstructorState& aState,
                                                   nsIFrame*        aFrame,
                                                   nsIContent*      aContent,
                                                   nsIStyleContext* aStyleContext,
                                                   nsIAtom*         aPseudoElement,
                                                   nsIFrame**       aWrapperFrame,
                                                   nsIFrame**       aResult)
{
  *aResult = nsnull; // initialize OUT parameter

  if (!aContent->IsContentOfType(nsIContent::eELEMENT))
    return PR_FALSE;

  // Probe for the existence of the pseudo-element
  nsCOMPtr<nsIStyleContext> pseudoStyleContext;
  aPresContext->ProbePseudoStyleContextFor(aContent, aPseudoElement, aStyleContext,
                                           getter_AddRefs(pseudoStyleContext));

  if (pseudoStyleContext) {
    const nsStyleDisplay* display;

    // See whether the generated content should be displayed.
    display = (const nsStyleDisplay*)pseudoStyleContext->GetStyleData(eStyleStruct_Display);

    if (NS_STYLE_DISPLAY_NONE != display->mDisplay) {
      // See if there was any content specified
      const nsStyleContent* styleContent =
        (const nsStyleContent*)pseudoStyleContext->GetStyleData(eStyleStruct_Content);
      PRUint32  contentCount = styleContent->ContentCount();

      // XXXldb What is contentCount for |content: ""|?
      if (contentCount > 0) {
        if (aWrapperFrame) {
          if (!*aWrapperFrame) {
            const nsStyleDisplay *display;
            ::GetStyleData(aStyleContext, &display);
            nsIAtom *wrapperPseudo;
            if (display->IsBlockLevel()) {
              NS_NewBlockFrame(aPresShell, aWrapperFrame);
              wrapperPseudo = nsCSSAtoms::mozGCWrapperBlock;
            } else {
              NS_NewInlineFrame(aPresShell, aWrapperFrame);
              wrapperPseudo = nsCSSAtoms::mozGCWrapperInline;
            }        
            nsCOMPtr<nsIStyleContext> parentSC = aStyleContext->GetParent(); 
            nsCOMPtr<nsIStyleContext> wrapperSC;
            aPresContext->ResolvePseudoStyleContextFor(nsnull, wrapperPseudo,
                                          parentSC, getter_AddRefs(wrapperSC));
            // |aFrame| is already the correct parent.
            InitAndRestoreFrame(aPresContext, aState, aContent, aFrame,
                                wrapperSC, nsnull, *aWrapperFrame);
          }
          // Use the wrapper as the parent.
          aFrame = *aWrapperFrame;
        }
        // Create a block box or an inline box depending on the value of
        // the 'display' property
        nsIFrame*     containerFrame;
        nsFrameItems  childFrames;

        if (NS_STYLE_DISPLAY_BLOCK == display->mDisplay) {
          NS_NewBlockFrame(aPresShell, &containerFrame);
        } else {
          NS_NewInlineFrame(aPresShell, &containerFrame);
        }        
        InitAndRestoreFrame(aPresContext, aState, aContent, 
                            aFrame, pseudoStyleContext, nsnull, containerFrame);

        // Mark the frame as being associated with generated content
        nsFrameState  frameState;
        containerFrame->GetFrameState(&frameState);
        frameState |= NS_FRAME_GENERATED_CONTENT;
        containerFrame->SetFrameState(frameState);

        // Create another pseudo style context to use for all the generated child
        // frames
        nsIStyleContext*  textStyleContext;
        aPresContext->ResolveStyleContextForNonElement(pseudoStyleContext,
                                                       &textStyleContext);

        // Now create content objects (and child frames) for each value of the
        // 'content' property

        for (PRUint32 contentIndex = 0; contentIndex < contentCount; contentIndex++) {
          nsIFrame* frame;

          // Create a frame
          nsresult result;
          result = CreateGeneratedFrameFor(aPresContext, mDocument, containerFrame,
                                           aContent, textStyleContext,
                                           styleContent, contentIndex, &frame);
          if (NS_SUCCEEDED(result) && frame) {
            // Add it to the list of child frames
            childFrames.AddChild(frame);
          }
        }
  
        NS_RELEASE(textStyleContext);
        if (childFrames.childList) {
          containerFrame->SetInitialChildList(aPresContext, nsnull, childFrames.childList);
        }
        *aResult = containerFrame;
        return PR_TRUE;
      }
    }
  }

  return PR_FALSE;
}

nsresult
nsCSSFrameConstructor::CreateInputFrame(nsIPresShell    *aPresShell,
                                        nsIPresContext  *aPresContext,
                                        nsIContent      *aContent, 
                                        nsIFrame        *&aFrame,
                                        nsIStyleContext *aStyleContext)
{
  nsCOMPtr<nsIFormControl> control = do_QueryInterface(aContent);
  NS_ASSERTION(control, "input is not an nsIFormControl!");
  
  PRInt32 type;
  control->GetType(&type);
  switch (type) {
    case NS_FORM_INPUT_SUBMIT:
    case NS_FORM_INPUT_RESET:
    case NS_FORM_INPUT_BUTTON:
      if (UseXBLForms())
        return NS_OK;
      return ConstructButtonControlFrame(aPresShell, aPresContext, aFrame);

    case NS_FORM_INPUT_CHECKBOX:
      if (UseXBLForms())
        return NS_OK;
      return ConstructCheckboxControlFrame(aPresShell, aPresContext, aFrame, aContent, aStyleContext);

    case NS_FORM_INPUT_RADIO:
      if (UseXBLForms())
        return NS_OK;
      return ConstructRadioControlFrame(aPresShell, aPresContext, aFrame, aContent, aStyleContext);

    case NS_FORM_INPUT_FILE:
      return NS_NewFileControlFrame(aPresShell, &aFrame);

    case NS_FORM_INPUT_HIDDEN:
      return NS_OK;

    case NS_FORM_INPUT_IMAGE:
      return NS_NewImageControlFrame(aPresShell, &aFrame);

    case NS_FORM_INPUT_TEXT:
    case NS_FORM_INPUT_PASSWORD:
      return ConstructTextControlFrame(aPresShell, aPresContext,
                                       aFrame, aContent);

    default:
      NS_ASSERTION(0, "Unknown input type!");
      return NS_ERROR_INVALID_ARG;
  }
}

static PRBool
IsOnlyWhiteSpace(nsIContent* aContent)
{
  PRBool onlyWhiteSpace = PR_FALSE;
  if (aContent->IsContentOfType(nsIContent::eTEXT)) {
    nsCOMPtr<nsITextContent> textContent = do_QueryInterface(aContent);
    if (textContent) {
      textContent->IsOnlyWhitespace(&onlyWhiteSpace);
    }
  }
  return onlyWhiteSpace;
}
    
/****************************************************
 **  BEGIN TABLE SECTION
 ****************************************************/

// The term pseudo frame is being used instead of anonymous frame, since anonymous
// frame has been used elsewhere to refer to frames that have generated content

// aIncludeSpecial applies to captions, col groups, cols and cells.
// These do not generate pseudo frame wrappers for foreign children. 

static PRBool
IsTableRelated(PRUint8 aDisplay,
               PRBool  aIncludeSpecial) 
{
  if ((aDisplay == NS_STYLE_DISPLAY_TABLE)              ||
      (aDisplay == NS_STYLE_DISPLAY_TABLE_HEADER_GROUP) ||
      (aDisplay == NS_STYLE_DISPLAY_TABLE_ROW_GROUP)    ||
      (aDisplay == NS_STYLE_DISPLAY_TABLE_FOOTER_GROUP) ||
      (aDisplay == NS_STYLE_DISPLAY_TABLE_ROW)) {
    return PR_TRUE;
  }
  else if (aIncludeSpecial && 
           ((aDisplay == NS_STYLE_DISPLAY_TABLE_CAPTION)      ||
            (aDisplay == NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP) ||
            (aDisplay == NS_STYLE_DISPLAY_TABLE_COLUMN)       ||
            (aDisplay == NS_STYLE_DISPLAY_TABLE_CELL))) {
    return PR_TRUE;
  }
  else return PR_FALSE;
}

static PRBool
IsTableRelated(nsIAtom* aParentType,
               PRBool   aIncludeSpecial)
{
  if ((nsLayoutAtoms::tableFrame         == aParentType)  ||
      (nsLayoutAtoms::tableRowGroupFrame == aParentType)  ||
      (nsLayoutAtoms::tableRowFrame      == aParentType)) {
    return PR_TRUE;
  }
  else if (aIncludeSpecial && 
           ((nsLayoutAtoms::tableCaptionFrame  == aParentType)  ||
            (nsLayoutAtoms::tableColGroupFrame == aParentType)  ||
            (nsLayoutAtoms::tableColFrame      == aParentType)  ||
            IS_TABLE_CELL(aParentType))) {
    return PR_TRUE;
  }
  else return PR_FALSE;
}
           
static nsIFrame*
GetOuterTableFrame(nsIFrame* aParentFrame) 
{
  nsIFrame* parent;
  nsCOMPtr<nsIAtom> frameType;
  aParentFrame->GetFrameType(getter_AddRefs(frameType));
  if (nsLayoutAtoms::tableOuterFrame == frameType.get()) {
    parent = aParentFrame;
  }
  else {
    aParentFrame->GetParent(&parent);
  }
  return parent;
}
    
static nsresult 
ProcessPseudoFrame(nsIPresContext*    aPresContext,
                   nsPseudoFrameData& aPseudoData,
                   nsIFrame*&         aParent)
{
  nsresult rv = NS_OK;
  if (!aPresContext) return rv;

  aParent = aPseudoData.mFrame;
  nsFrameItems* items = &aPseudoData.mChildList;
  if (items && items->childList) {
    rv = aParent->SetInitialChildList(aPresContext, nsnull, items->childList);
    if (NS_FAILED(rv)) return rv;
  }
  aPseudoData.Reset();
  return rv;
}

static nsresult 
ProcessPseudoTableFrame(nsIPresContext* aPresContext,
                        nsPseudoFrames& aPseudoFrames,
                        nsIFrame*&      aParent)
{
  nsresult rv = NS_OK;
  if (!aPresContext) return rv;

  // process the col group frame, if it exists
  if (aPseudoFrames.mColGroup.mFrame) {
    rv = ProcessPseudoFrame(aPresContext, aPseudoFrames.mColGroup, aParent);
  }

  // process the inner table frame
  rv = ProcessPseudoFrame(aPresContext, aPseudoFrames.mTableInner, aParent);

  // process the outer table frame
  aParent = aPseudoFrames.mTableOuter.mFrame;
  nsFrameItems* items = &aPseudoFrames.mTableOuter.mChildList;
  if (items && items->childList) {
    rv = aParent->SetInitialChildList(aPresContext, nsnull, items->childList);
    if (NS_FAILED(rv)) return rv;
  }
  nsFrameItems* captions = &aPseudoFrames.mTableOuter.mChildList2;
  if (captions && captions->childList) {
    rv = aParent->SetInitialChildList(aPresContext, nsLayoutAtoms::captionList, captions->childList);
  }
  aPseudoFrames.mTableOuter.Reset();
  return rv;
}

static nsresult 
ProcessPseudoCellFrame(nsIPresContext* aPresContext,
                       nsPseudoFrames& aPseudoFrames,
                       nsIFrame*&      aParent)
{
  nsresult rv = NS_OK;
  if (!aPresContext) return rv;

  rv = ProcessPseudoFrame(aPresContext, aPseudoFrames.mCellInner, aParent);
  if (NS_FAILED(rv)) return rv;
  rv = ProcessPseudoFrame(aPresContext, aPseudoFrames.mCellOuter, aParent);
  return rv;
}


static nsresult 
ProcessPseudoFrames(nsIPresContext* aPresContext,
                    nsPseudoFrames& aPseudoFrames,
                    nsIAtom*        aHighestType,
                    nsIFrame*&      aHighestFrame)
{
  nsresult rv = NS_OK;
  if (!aPresContext) return rv;

  aHighestFrame = nsnull;

  if (nsLayoutAtoms::tableFrame == aPseudoFrames.mLowestType) {
    rv = ProcessPseudoTableFrame(aPresContext, aPseudoFrames, aHighestFrame);
    if (nsLayoutAtoms::tableOuterFrame == aHighestType) return rv;
    
    if (aPseudoFrames.mCellOuter.mFrame) {
      rv = ProcessPseudoCellFrame(aPresContext, aPseudoFrames, aHighestFrame);
      if (IS_TABLE_CELL(aHighestType)) return rv;
    }
    if (aPseudoFrames.mRow.mFrame) {
      rv = ProcessPseudoFrame(aPresContext, aPseudoFrames.mRow, aHighestFrame);
      if (nsLayoutAtoms::tableRowFrame == aHighestType) return rv;
    }
    if (aPseudoFrames.mRowGroup.mFrame) {
      rv = ProcessPseudoFrame(aPresContext, aPseudoFrames.mRowGroup, aHighestFrame);
      if (nsLayoutAtoms::tableRowGroupFrame == aHighestType) return rv;
    }
  }
  else if (nsLayoutAtoms::tableRowGroupFrame == aPseudoFrames.mLowestType) {
    rv = ProcessPseudoFrame(aPresContext, aPseudoFrames.mRowGroup, aHighestFrame);
    if (nsLayoutAtoms::tableRowGroupFrame == aHighestType) return rv;

    if (aPseudoFrames.mTableOuter.mFrame) {
      rv = ProcessPseudoTableFrame(aPresContext, aPseudoFrames, aHighestFrame);
      if (nsLayoutAtoms::tableOuterFrame == aHighestType) return rv;
    }
    if (aPseudoFrames.mCellOuter.mFrame) {
      rv = ProcessPseudoCellFrame(aPresContext, aPseudoFrames, aHighestFrame);
      if (IS_TABLE_CELL(aHighestType)) return rv;
    }
    if (aPseudoFrames.mRow.mFrame) {
      rv = ProcessPseudoFrame(aPresContext, aPseudoFrames.mRow, aHighestFrame);
      if (nsLayoutAtoms::tableRowFrame == aHighestType) return rv;
    }
  }
  else if (nsLayoutAtoms::tableRowFrame == aPseudoFrames.mLowestType) {
    rv = ProcessPseudoFrame(aPresContext, aPseudoFrames.mRow, aHighestFrame);
    if (nsLayoutAtoms::tableRowFrame == aHighestType) return rv;

    if (aPseudoFrames.mRowGroup.mFrame) {
      rv = ProcessPseudoFrame(aPresContext, aPseudoFrames.mRowGroup, aHighestFrame);
      if (nsLayoutAtoms::tableRowGroupFrame == aHighestType) return rv;
    }
    if (aPseudoFrames.mTableOuter.mFrame) {
      rv = ProcessPseudoTableFrame(aPresContext, aPseudoFrames, aHighestFrame);
      if (nsLayoutAtoms::tableOuterFrame == aHighestType) return rv;
    }
    if (aPseudoFrames.mCellOuter.mFrame) {
      rv = ProcessPseudoCellFrame(aPresContext, aPseudoFrames, aHighestFrame);
      if (IS_TABLE_CELL(aHighestType)) return rv;
    }
  }
  else if (IS_TABLE_CELL(aPseudoFrames.mLowestType)) {
    rv = ProcessPseudoCellFrame(aPresContext, aPseudoFrames, aHighestFrame);
    if (IS_TABLE_CELL(aHighestType)) return rv;

    if (aPseudoFrames.mRow.mFrame) {
      rv = ProcessPseudoFrame(aPresContext, aPseudoFrames.mRow, aHighestFrame);
      if (nsLayoutAtoms::tableRowFrame == aHighestType) return rv;
    }
    if (aPseudoFrames.mRowGroup.mFrame) {
      rv = ProcessPseudoFrame(aPresContext, aPseudoFrames.mRowGroup, aHighestFrame);
      if (nsLayoutAtoms::tableRowGroupFrame == aHighestType) return rv;
    }
    if (aPseudoFrames.mTableOuter.mFrame) {
      rv = ProcessPseudoTableFrame(aPresContext, aPseudoFrames, aHighestFrame);
    }
  }
  else if (aPseudoFrames.mColGroup.mFrame) { 
    // process the col group frame
    rv = ProcessPseudoFrame(aPresContext, aPseudoFrames.mColGroup, aHighestFrame);
  }

  return rv;
}

static nsresult 
ProcessPseudoFrames(nsIPresContext* aPresContext,
                    nsPseudoFrames& aPseudoFrames,
                    nsFrameItems&   aItems)
{
  nsIFrame* highestFrame;
  nsresult rv = ProcessPseudoFrames(aPresContext, aPseudoFrames, nsnull, highestFrame);
  if (highestFrame) {
    aItems.AddChild(highestFrame);
  }
  aPseudoFrames.Reset();
  return rv;
}

static nsresult 
ProcessPseudoFrames(nsIPresContext* aPresContext,
                    nsPseudoFrames& aPseudoFrames,
                    nsIAtom*        aHighestType)
{
  nsIFrame* highestFrame;
  nsresult rv = ProcessPseudoFrames(aPresContext, aPseudoFrames, aHighestType, highestFrame);
  return rv;
}

nsresult
nsCSSFrameConstructor::CreatePseudoTableFrame(nsIPresShell*            aPresShell,
                                              nsIPresContext*          aPresContext,
                                              nsTableCreator&          aTableCreator,
                                              nsFrameConstructorState& aState, 
                                              nsIFrame*                aParentFrameIn)
{
  nsresult rv = NS_OK;

  nsIFrame* parentFrame = (aState.mPseudoFrames.mCellInner.mFrame) 
                          ? aState.mPseudoFrames.mCellInner.mFrame : aParentFrameIn;
  if (!parentFrame) return rv;

  nsCOMPtr<nsIStyleContext> parentStyle;
  nsCOMPtr<nsIContent>      parentContent;
  nsCOMPtr<nsIStyleContext> childStyle;

  parentFrame->GetStyleContext(getter_AddRefs(parentStyle)); 
  parentFrame->GetContent(getter_AddRefs(parentContent));   

  // create the SC for the inner table which will be the parent of the outer table's SC
  aPresContext->ResolvePseudoStyleContextFor(parentContent, nsHTMLAtoms::tablePseudo, 
                                             parentStyle,
                                             getter_AddRefs(childStyle));

  nsPseudoFrameData& pseudoOuter = aState.mPseudoFrames.mTableOuter;
  nsPseudoFrameData& pseudoInner = aState.mPseudoFrames.mTableInner;

  // construct the pseudo outer and inner as part of the pseudo frames
  PRBool pseudoParent;
  nsFrameItems items;
  rv = ConstructTableFrame(aPresShell, aPresContext, aState, parentContent,
                           parentFrame, childStyle.get(), aTableCreator,
                           PR_TRUE, items, pseudoOuter.mFrame, 
                           pseudoInner.mFrame, pseudoParent);

  if (NS_FAILED(rv)) return rv;

  // set pseudo data for the newly created frames
  pseudoOuter.mChildList.AddChild(pseudoInner.mFrame);
  aState.mPseudoFrames.mLowestType = nsLayoutAtoms::tableFrame;

  // set pseudo data for the parent
  if (aState.mPseudoFrames.mCellInner.mFrame) {
    aState.mPseudoFrames.mCellInner.mChildList.AddChild(pseudoOuter.mFrame);
  }

  return rv;
}

nsresult
nsCSSFrameConstructor::CreatePseudoRowGroupFrame(nsIPresShell*            aPresShell,
                                                 nsIPresContext*          aPresContext,
                                                 nsTableCreator&          aTableCreator,
                                                 nsFrameConstructorState& aState, 
                                                 nsIFrame*                aParentFrameIn)
{
  nsresult rv = NS_OK;

  nsIFrame* parentFrame = (aState.mPseudoFrames.mTableInner.mFrame) 
                          ? aState.mPseudoFrames.mTableInner.mFrame : aParentFrameIn;
  if (!parentFrame) return rv;

  nsCOMPtr<nsIStyleContext> parentStyle;
  nsCOMPtr<nsIContent>      parentContent;
  nsCOMPtr<nsIStyleContext> childStyle;

  parentFrame->GetStyleContext(getter_AddRefs(parentStyle)); 
  parentFrame->GetContent(getter_AddRefs(parentContent));   

  aPresContext->ResolvePseudoStyleContextFor(parentContent, nsHTMLAtoms::tableRowGroupPseudo, 
                                             parentStyle,
                                             getter_AddRefs(childStyle));

  nsPseudoFrameData& pseudo = aState.mPseudoFrames.mRowGroup;

  // construct the pseudo row group as part of the pseudo frames
  PRBool pseudoParent;
  nsFrameItems items;
  rv = ConstructTableRowGroupFrame(aPresShell, aPresContext, aState, parentContent,
                                   parentFrame, childStyle.get(), aTableCreator,
                                   PR_TRUE, items, pseudo.mFrame, pseudoParent);
  if (NS_FAILED(rv)) return rv;

  // set pseudo data for the newly created frames
  aState.mPseudoFrames.mLowestType = nsLayoutAtoms::tableRowGroupFrame;

  // set pseudo data for the parent
  if (aState.mPseudoFrames.mTableInner.mFrame) {
    aState.mPseudoFrames.mTableInner.mChildList.AddChild(pseudo.mFrame);
  }

  return rv;
}

nsresult 
nsCSSFrameConstructor::CreatePseudoColGroupFrame(nsIPresShell*            aPresShell,
                                                 nsIPresContext*          aPresContext,
                                                 nsTableCreator&          aTableCreator,
                                                 nsFrameConstructorState& aState, 
                                                 nsIFrame*                aParentFrameIn)
{
  nsresult rv = NS_OK;

  nsIFrame* parentFrame = (aState.mPseudoFrames.mTableInner.mFrame) 
                          ? aState.mPseudoFrames.mTableInner.mFrame : aParentFrameIn;
  if (!parentFrame) return rv;

  nsCOMPtr<nsIStyleContext> parentStyle;
  nsCOMPtr<nsIContent>      parentContent;
  nsCOMPtr<nsIStyleContext> childStyle;

  parentFrame->GetStyleContext(getter_AddRefs(parentStyle)); 
  parentFrame->GetContent(getter_AddRefs(parentContent));   

  aPresContext->ResolvePseudoStyleContextFor(parentContent, nsHTMLAtoms::tableColGroupPseudo, 
                                             parentStyle,
                                             getter_AddRefs(childStyle));

  nsPseudoFrameData& pseudo = aState.mPseudoFrames.mColGroup;

  // construct the pseudo col group as part of the pseudo frames
  PRBool pseudoParent;
  nsFrameItems items;
  rv = ConstructTableColGroupFrame(aPresShell, aPresContext, aState, parentContent,
                                   parentFrame, childStyle.get(), aTableCreator,
                                   PR_TRUE, items, pseudo.mFrame, pseudoParent);
  if (NS_FAILED(rv)) return rv;
  ((nsTableColGroupFrame*)pseudo.mFrame)->SetType(eColGroupAnonymousCol);

  // set pseudo data for the parent
  if (aState.mPseudoFrames.mTableInner.mFrame) {
    aState.mPseudoFrames.mTableInner.mChildList.AddChild(pseudo.mFrame);
  }

  return rv;
}

nsresult
nsCSSFrameConstructor::CreatePseudoRowFrame(nsIPresShell*            aPresShell,
                                            nsIPresContext*          aPresContext,
                                            nsTableCreator&          aTableCreator,
                                            nsFrameConstructorState& aState, 
                                            nsIFrame*                aParentFrameIn)
{
  nsresult rv = NS_OK;

  nsIFrame* parentFrame = (aState.mPseudoFrames.mRowGroup.mFrame) 
                          ? aState.mPseudoFrames.mRowGroup.mFrame : aParentFrameIn;
  if (!parentFrame) return rv;

  nsCOMPtr<nsIStyleContext> parentStyle;
  nsCOMPtr<nsIContent>      parentContent;
  nsCOMPtr<nsIStyleContext> childStyle;

  parentFrame->GetStyleContext(getter_AddRefs(parentStyle)); 
  parentFrame->GetContent(getter_AddRefs(parentContent));   

  aPresContext->ResolvePseudoStyleContextFor(parentContent, nsHTMLAtoms::tableRowPseudo, 
                                             parentStyle,
                                             getter_AddRefs(childStyle));

  nsPseudoFrameData& pseudo = aState.mPseudoFrames.mRow;

  // construct the pseudo row as part of the pseudo frames
  PRBool pseudoParent;
  nsFrameItems items;
  rv = ConstructTableRowFrame(aPresShell, aPresContext, aState, parentContent,
                              parentFrame, childStyle.get(), aTableCreator,
                              PR_TRUE, items, pseudo.mFrame, pseudoParent);
  if (NS_FAILED(rv)) return rv;

  aState.mPseudoFrames.mLowestType = nsLayoutAtoms::tableRowFrame;

  // set pseudo data for the parent
  if (aState.mPseudoFrames.mRowGroup.mFrame) {
    aState.mPseudoFrames.mRowGroup.mChildList.AddChild(pseudo.mFrame);
  }

  return rv;
}

nsresult
nsCSSFrameConstructor::CreatePseudoCellFrame(nsIPresShell*            aPresShell,
                                             nsIPresContext*          aPresContext,
                                             nsTableCreator&          aTableCreator,
                                             nsFrameConstructorState& aState, 
                                             nsIFrame*                aParentFrameIn)
{
  nsresult rv = NS_OK;

  nsIFrame* parentFrame = (aState.mPseudoFrames.mRow.mFrame) 
                          ? aState.mPseudoFrames.mRow.mFrame : aParentFrameIn;
  if (!parentFrame) return rv;

  nsCOMPtr<nsIStyleContext> parentStyle;
  nsCOMPtr<nsIContent>      parentContent;
  nsCOMPtr<nsIStyleContext> childStyle;

  parentFrame->GetStyleContext(getter_AddRefs(parentStyle)); 
  parentFrame->GetContent(getter_AddRefs(parentContent));   

  aPresContext->ResolvePseudoStyleContextFor(parentContent, nsHTMLAtoms::tableCellPseudo, 
                                             parentStyle,
                                             getter_AddRefs(childStyle));

  nsPseudoFrameData& pseudoOuter = aState.mPseudoFrames.mCellOuter;
  nsPseudoFrameData& pseudoInner = aState.mPseudoFrames.mCellInner;

  // construct the pseudo outer and inner as part of the pseudo frames
  PRBool pseudoParent;
  nsFrameItems items;
  rv = ConstructTableCellFrame(aPresShell, aPresContext, aState, parentContent,
                               parentFrame, childStyle.get(), aTableCreator,
                               PR_TRUE, items, pseudoOuter.mFrame, 
                               pseudoInner.mFrame, pseudoParent);
  if (NS_FAILED(rv)) return rv;

  // set pseudo data for the newly created frames
  pseudoOuter.mChildList.AddChild(pseudoInner.mFrame);
  // give it nsLayoutAtoms::tableCellFrame, if it is really nsLayoutAtoms::bcTableCellFrame, it will match later
  aState.mPseudoFrames.mLowestType = nsLayoutAtoms::tableCellFrame;

  // set pseudo data for the parent
  if (aState.mPseudoFrames.mRow.mFrame) {
    aState.mPseudoFrames.mRow.mChildList.AddChild(pseudoOuter.mFrame);
  }

  return rv;
}

// called if the parent is not a table
nsresult 
nsCSSFrameConstructor::GetPseudoTableFrame(nsIPresShell*            aPresShell, 
                                           nsIPresContext*          aPresContext, 
                                           nsTableCreator&          aTableCreator,
                                           nsFrameConstructorState& aState, 
                                           nsIFrame&                aParentFrameIn)
{
  nsresult rv = NS_OK;
  if (!aPresShell || !aPresContext) return rv;

  nsPseudoFrames& pseudoFrames = aState.mPseudoFrames;
  nsCOMPtr<nsIAtom> parentFrameType;
  aParentFrameIn.GetFrameType(getter_AddRefs(parentFrameType));

  if (pseudoFrames.IsEmpty()) {
    PRBool created = PR_FALSE;
    if (nsLayoutAtoms::tableRowGroupFrame == parentFrameType.get()) { // row group parent
      rv = CreatePseudoRowFrame(aPresShell, aPresContext, aTableCreator, aState, &aParentFrameIn);
      if (NS_FAILED(rv)) return rv;
      created = PR_TRUE;
    }
    if (created || (nsLayoutAtoms::tableRowFrame == parentFrameType.get())) { // row parent
      rv = CreatePseudoCellFrame(aPresShell, aPresContext, aTableCreator, aState, &aParentFrameIn);
      if (NS_FAILED(rv)) return rv;
    }
    rv = CreatePseudoTableFrame(aPresShell, aPresContext, aTableCreator, aState, &aParentFrameIn);
  }
  else {
    if (!pseudoFrames.mTableInner.mFrame) { 
      if (pseudoFrames.mRowGroup.mFrame && !(pseudoFrames.mRow.mFrame)) {
        rv = CreatePseudoRowFrame(aPresShell, aPresContext, aTableCreator, aState);
        if (NS_FAILED(rv)) return rv;
      }
      if (pseudoFrames.mRow.mFrame && !(pseudoFrames.mCellOuter.mFrame)) {
        rv = CreatePseudoCellFrame(aPresShell, aPresContext, aTableCreator, aState);
        if (NS_FAILED(rv)) return rv;
      }
      CreatePseudoTableFrame(aPresShell, aPresContext, aTableCreator, aState);
    }
  }
  return rv;
}

// called if the parent is not a col group
nsresult 
nsCSSFrameConstructor::GetPseudoColGroupFrame(nsIPresShell*            aPresShell, 
                                              nsIPresContext*          aPresContext, 
                                              nsTableCreator&          aTableCreator,
                                              nsFrameConstructorState& aState, 
                                              nsIFrame&                aParentFrameIn)
{
  nsresult rv = NS_OK;
  if (!aPresShell || !aPresContext) return rv;

  nsPseudoFrames& pseudoFrames = aState.mPseudoFrames;
  nsCOMPtr<nsIAtom> parentFrameType;
  aParentFrameIn.GetFrameType(getter_AddRefs(parentFrameType));

  if (pseudoFrames.IsEmpty()) {
    PRBool created = PR_FALSE;
    if (nsLayoutAtoms::tableRowGroupFrame == parentFrameType.get()) {  // row group parent
      rv = CreatePseudoRowFrame(aPresShell, aPresContext, aTableCreator, aState, &aParentFrameIn);
      created = PR_TRUE;
    }
    if (created || (nsLayoutAtoms::tableRowFrame == parentFrameType.get())) { // row parent
      rv = CreatePseudoCellFrame(aPresShell, aPresContext, aTableCreator, aState, &aParentFrameIn);
      created = PR_TRUE;
    }
    if (created || IS_TABLE_CELL(parentFrameType.get()) || // cell parent
                   !IsTableRelated(parentFrameType.get(), PR_TRUE)) { // block parent
      rv = CreatePseudoTableFrame(aPresShell, aPresContext, aTableCreator, aState, &aParentFrameIn);
    }
    rv = CreatePseudoColGroupFrame(aPresShell, aPresContext, aTableCreator, aState, &aParentFrameIn);
  }
  else {
    if (!pseudoFrames.mColGroup.mFrame) { 
      if (pseudoFrames.mRowGroup.mFrame && !(pseudoFrames.mRow.mFrame)) {
        rv = CreatePseudoRowFrame(aPresShell, aPresContext, aTableCreator, aState);
      }
      if (pseudoFrames.mRow.mFrame && !(pseudoFrames.mCellOuter.mFrame)) {
        rv = CreatePseudoCellFrame(aPresShell, aPresContext, aTableCreator, aState);
      }
      if (pseudoFrames.mCellOuter.mFrame && !(pseudoFrames.mTableOuter.mFrame)) {
        rv = CreatePseudoTableFrame(aPresShell, aPresContext, aTableCreator, aState);
      }
      rv = CreatePseudoColGroupFrame(aPresShell, aPresContext, aTableCreator, aState);
    }
  }
  return rv;
}

// called if the parent is not a row group
nsresult 
nsCSSFrameConstructor::GetPseudoRowGroupFrame(nsIPresShell*            aPresShell, 
                                              nsIPresContext*          aPresContext, 
                                              nsTableCreator&          aTableCreator,
                                              nsFrameConstructorState& aState, 
                                              nsIFrame&                aParentFrameIn)
{
  nsresult rv = NS_OK;
  if (!aPresShell || !aPresContext) return rv;

  nsPseudoFrames& pseudoFrames = aState.mPseudoFrames;
  nsCOMPtr<nsIAtom> parentFrameType;
  aParentFrameIn.GetFrameType(getter_AddRefs(parentFrameType));

  if (pseudoFrames.IsEmpty()) {
    PRBool created = PR_FALSE;
    if (nsLayoutAtoms::tableRowFrame == parentFrameType.get()) {  // row parent
      rv = CreatePseudoCellFrame(aPresShell, aPresContext, aTableCreator, aState, &aParentFrameIn);
      created = PR_TRUE;
    }
    if (created || IS_TABLE_CELL(parentFrameType.get()) || // cell parent
        !IsTableRelated(parentFrameType.get(), PR_TRUE)) { // block parent
      rv = CreatePseudoTableFrame(aPresShell, aPresContext, aTableCreator, aState, &aParentFrameIn);
    }
    rv = CreatePseudoRowGroupFrame(aPresShell, aPresContext, aTableCreator, aState, &aParentFrameIn);
  }
  else {
    if (!pseudoFrames.mRowGroup.mFrame) { 
      if (pseudoFrames.mRow.mFrame && !(pseudoFrames.mCellOuter.mFrame)) {
        rv = CreatePseudoCellFrame(aPresShell, aPresContext, aTableCreator, aState);
      }
      if (pseudoFrames.mCellOuter.mFrame && !(pseudoFrames.mTableOuter.mFrame)) {
        rv = CreatePseudoTableFrame(aPresShell, aPresContext, aTableCreator, aState);
      }
      rv = CreatePseudoRowGroupFrame(aPresShell, aPresContext, aTableCreator, aState);
    }
  }
  return rv;
}

// called if the parent is not a row
nsresult
nsCSSFrameConstructor::GetPseudoRowFrame(nsIPresShell*            aPresShell, 
                                         nsIPresContext*          aPresContext, 
                                         nsTableCreator&          aTableCreator,
                                         nsFrameConstructorState& aState, 
                                         nsIFrame&                aParentFrameIn)
{
  nsresult rv = NS_OK;
  if (!aPresShell || !aPresContext) return rv;

  nsPseudoFrames& pseudoFrames = aState.mPseudoFrames;
  nsCOMPtr<nsIAtom> parentFrameType;
  aParentFrameIn.GetFrameType(getter_AddRefs(parentFrameType));

  if (pseudoFrames.IsEmpty()) {
    PRBool created = PR_FALSE;
    if (IS_TABLE_CELL(parentFrameType.get()) || // cell parent
        !IsTableRelated(parentFrameType.get(), PR_TRUE)) { // block parent
      rv = CreatePseudoTableFrame(aPresShell, aPresContext, aTableCreator, aState, &aParentFrameIn);
      created = PR_TRUE;
    }
    if (created || (nsLayoutAtoms::tableFrame == parentFrameType.get())) { // table parent
      rv = CreatePseudoRowGroupFrame(aPresShell, aPresContext, aTableCreator, aState, &aParentFrameIn);
    }
    rv = CreatePseudoRowFrame(aPresShell, aPresContext, aTableCreator, aState, &aParentFrameIn);
  }
  else {
    if (!pseudoFrames.mRow.mFrame) { 
      if (pseudoFrames.mCellOuter.mFrame && !pseudoFrames.mTableOuter.mFrame) {
        rv = CreatePseudoTableFrame(aPresShell, aPresContext, aTableCreator, aState);
      }
      if (pseudoFrames.mTableInner.mFrame && !(pseudoFrames.mRowGroup.mFrame)) {
        rv = CreatePseudoRowGroupFrame(aPresShell, aPresContext, aTableCreator, aState);
      }
      rv = CreatePseudoRowFrame(aPresShell, aPresContext, aTableCreator, aState);
    }
  }
  return rv;
}

// called if the parent is not a cell or block
nsresult 
nsCSSFrameConstructor::GetPseudoCellFrame(nsIPresShell*            aPresShell, 
                                          nsIPresContext*          aPresContext, 
                                          nsTableCreator&          aTableCreator,
                                          nsFrameConstructorState& aState, 
                                          nsIFrame&                aParentFrameIn)
{
  nsresult rv = NS_OK;
  if (!aPresShell || !aPresContext) return rv;

  nsPseudoFrames& pseudoFrames = aState.mPseudoFrames;
  nsCOMPtr<nsIAtom> parentFrameType;
  aParentFrameIn.GetFrameType(getter_AddRefs(parentFrameType));

  if (pseudoFrames.IsEmpty()) {
    PRBool created = PR_FALSE;
    if (nsLayoutAtoms::tableFrame == parentFrameType.get()) { // table parent
      rv = CreatePseudoRowGroupFrame(aPresShell, aPresContext, aTableCreator, aState, &aParentFrameIn);
      created = PR_TRUE;
    }
    if (created || (nsLayoutAtoms::tableRowGroupFrame == parentFrameType.get())) { // row group parent
      rv = CreatePseudoRowFrame(aPresShell, aPresContext, aTableCreator, aState, &aParentFrameIn);
      created = PR_TRUE;
    }
    rv = CreatePseudoCellFrame(aPresShell, aPresContext, aTableCreator, aState, &aParentFrameIn);
  }
  else if (!pseudoFrames.mCellOuter.mFrame) { 
    if (pseudoFrames.mTableInner.mFrame && !(pseudoFrames.mRowGroup.mFrame)) {
      rv = CreatePseudoRowGroupFrame(aPresShell, aPresContext, aTableCreator, aState);
    }
    if (pseudoFrames.mRowGroup.mFrame && !(pseudoFrames.mRow.mFrame)) {
      rv = CreatePseudoRowFrame(aPresShell, aPresContext, aTableCreator, aState);
    }
    rv = CreatePseudoCellFrame(aPresShell, aPresContext, aTableCreator, aState);
  }
  return rv;
}

nsresult 
nsCSSFrameConstructor::GetParentFrame(nsIPresShell*            aPresShell,
                                      nsIPresContext*          aPresContext,
                                      nsTableCreator&          aTableCreator,
                                      nsIFrame&                aParentFrameIn, 
                                      nsIAtom*                 aChildFrameType, 
                                      nsFrameConstructorState& aState, 
                                      nsIFrame*&               aParentFrame,
                                      PRBool&                  aIsPseudoParent)
{
  nsresult rv = NS_OK;
  if (!aPresShell || !aPresContext) return rv;

  nsCOMPtr<nsIAtom> parentFrameType;
  aParentFrameIn.GetFrameType(getter_AddRefs(parentFrameType));
  nsIFrame* pseudoParentFrame = nsnull;
  nsPseudoFrames& pseudoFrames = aState.mPseudoFrames;
  aParentFrame = &aParentFrameIn;
  aIsPseudoParent = PR_FALSE;

  if (nsLayoutAtoms::tableOuterFrame == aChildFrameType) { // table child
    if (IsTableRelated(parentFrameType.get(), PR_TRUE) &&
        (nsLayoutAtoms::tableCaptionFrame != parentFrameType.get()) ) { // need pseudo cell parent
      rv = GetPseudoCellFrame(aPresShell, aPresContext, aTableCreator, aState, aParentFrameIn);
      if (NS_FAILED(rv)) return rv;
      pseudoParentFrame = pseudoFrames.mCellInner.mFrame;
    }
  } 
  else if (nsLayoutAtoms::tableCaptionFrame == aChildFrameType) { // caption child
    if (nsLayoutAtoms::tableOuterFrame != parentFrameType.get()) { // need pseudo table parent
      rv = GetPseudoTableFrame(aPresShell, aPresContext, aTableCreator, aState, aParentFrameIn);
      if (NS_FAILED(rv)) return rv;
      pseudoParentFrame = pseudoFrames.mTableOuter.mFrame;
    }
  }
  else if (nsLayoutAtoms::tableColGroupFrame == aChildFrameType) { // col group child
    if (nsLayoutAtoms::tableFrame != parentFrameType.get()) { // need pseudo table parent
      rv = GetPseudoTableFrame(aPresShell, aPresContext, aTableCreator, aState, aParentFrameIn);
      if (NS_FAILED(rv)) return rv;
      pseudoParentFrame = pseudoFrames.mTableInner.mFrame;
    }
  }
  else if (nsLayoutAtoms::tableColFrame == aChildFrameType) { // col child
    if (nsLayoutAtoms::tableColGroupFrame != parentFrameType.get()) { // need pseudo col group parent
      rv = GetPseudoColGroupFrame(aPresShell, aPresContext, aTableCreator, aState, aParentFrameIn);
      if (NS_FAILED(rv)) return rv;
      pseudoParentFrame = pseudoFrames.mColGroup.mFrame;
    }
  }
  else if (nsLayoutAtoms::tableRowGroupFrame == aChildFrameType) { // row group child
    // XXX can this go away?
    if (nsLayoutAtoms::tableFrame != parentFrameType.get()) {
      // trees allow row groups to contain row groups, so don't create pseudo frames
        rv = GetPseudoTableFrame(aPresShell, aPresContext, aTableCreator, aState, aParentFrameIn);
        if (NS_FAILED(rv)) return rv;
        pseudoParentFrame = pseudoFrames.mTableInner.mFrame;
     }
  }
  else if (nsLayoutAtoms::tableRowFrame == aChildFrameType) { // row child
    if (nsLayoutAtoms::tableRowGroupFrame != parentFrameType.get()) { // need pseudo row group parent
      rv = GetPseudoRowGroupFrame(aPresShell, aPresContext, aTableCreator, aState, aParentFrameIn);
      if (NS_FAILED(rv)) return rv;
      pseudoParentFrame = pseudoFrames.mRowGroup.mFrame;
    }
  }
  else if (IS_TABLE_CELL(aChildFrameType)) { // cell child
    if (nsLayoutAtoms::tableRowFrame != parentFrameType.get()) { // need pseudo row parent
      rv = GetPseudoRowFrame(aPresShell, aPresContext, aTableCreator, aState, aParentFrameIn);
      if (NS_FAILED(rv)) return rv;
      pseudoParentFrame = pseudoFrames.mRow.mFrame;
    }
  }
  else if (nsLayoutAtoms::tableFrame == aChildFrameType) { // invalid
    NS_ASSERTION(PR_FALSE, "GetParentFrame called on nsLayoutAtoms::tableFrame child");
  }
  else { // foreign frame
    if (IsTableRelated(parentFrameType.get(), PR_FALSE)) { // need pseudo cell parent
      rv = GetPseudoCellFrame(aPresShell, aPresContext, aTableCreator, aState, aParentFrameIn);
      if (NS_FAILED(rv)) return rv;
      pseudoParentFrame = pseudoFrames.mCellInner.mFrame;
    }
  }
  
  if (pseudoParentFrame) {
    aParentFrame = pseudoParentFrame;
    aIsPseudoParent = PR_TRUE;
  }

  return rv;
}

// Construct the outer, inner table frames and the children frames for the table. 
// XXX Page break frames for pseudo table frames are not constructed to avoid the risk
// associated with revising the pseudo frame mechanism. The long term solution
// of having frames handle page-break-before/after will solve the problem. 
nsresult
nsCSSFrameConstructor::ConstructTableFrame(nsIPresShell*            aPresShell,
                                           nsIPresContext*          aPresContext,
                                           nsFrameConstructorState& aState,
                                           nsIContent*              aContent,
                                           nsIFrame*                aParentFrameIn,
                                           nsIStyleContext*         aStyleContext,
                                           nsTableCreator&          aTableCreator,
                                           PRBool                   aIsPseudo,
                                           nsFrameItems&            aChildItems,
                                           nsIFrame*&               aNewOuterFrame,
                                           nsIFrame*&               aNewInnerFrame,
                                           PRBool&                  aIsPseudoParent)
{
  nsresult rv = NS_OK;
  if (!aPresShell || !aPresContext || !aParentFrameIn) return rv;

  // Create the outer table frame which holds the caption and inner table frame
  aTableCreator.CreateTableOuterFrame(&aNewOuterFrame);

  nsIFrame* parentFrame = aParentFrameIn;
  aIsPseudoParent = PR_FALSE;
  if (!aIsPseudo) {
    // this frame may have a pseudo parent
    GetParentFrame(aPresShell, aPresContext, aTableCreator, *aParentFrameIn, 
                   nsLayoutAtoms::tableOuterFrame, aState, parentFrame, aIsPseudoParent);
    if (!aIsPseudoParent && !aState.mPseudoFrames.IsEmpty()) {
      ProcessPseudoFrames(aPresContext, aState.mPseudoFrames, aChildItems);
    }
    if (!aIsPseudo && aIsPseudoParent && aState.mPseudoFrames.mTableOuter.mFrame) {
      ProcessPseudoFrames(aPresContext, aState.mPseudoFrames, nsLayoutAtoms::tableOuterFrame);
    }
  }

  // create the pseudo SC for the outer table as a child of the inner SC
  nsCOMPtr<nsIStyleContext> outerStyleContext;
  aPresContext->ResolvePseudoStyleContextFor(aContent, nsHTMLAtoms::tableOuterPseudo,
                                             aStyleContext,
                                             getter_AddRefs(outerStyleContext));
  
  // Init the table outer frame and see if we need to create a view, e.g.
  // the frame is absolutely positioned  
  InitAndRestoreFrame(aPresContext, aState, aContent, 
                      parentFrame, outerStyleContext, nsnull, aNewOuterFrame);  
  nsHTMLContainerFrame::CreateViewForFrame(aPresContext, aNewOuterFrame,
                                           outerStyleContext, nsnull, PR_FALSE);

  // Create the inner table frame
  aTableCreator.CreateTableFrame(&aNewInnerFrame);

  InitAndRestoreFrame(aPresContext, aState, aContent, 
                      aNewOuterFrame, aStyleContext, nsnull, aNewInnerFrame);

  if (!aIsPseudo) {
    nsFrameItems childItems;
    nsIFrame* captionFrame;

    rv = TableProcessChildren(aPresShell, aPresContext, aState, aContent, aNewInnerFrame,
                              aTableCreator, childItems, captionFrame);
    if (NS_FAILED(rv)) return rv;

    // if there are any anonymous children for the table, create frames for them
    CreateAnonymousFrames(aPresShell, aPresContext, nsnull, aState, aContent, aNewInnerFrame,
                          childItems);

    // Set the inner table frame's initial primary list 
    aNewInnerFrame->SetInitialChildList(aPresContext, nsnull, childItems.childList);

    // Set the outer table frame's primary and option lists
    aNewOuterFrame->SetInitialChildList(aPresContext, nsnull, aNewInnerFrame);
    if (captionFrame) {
      aNewOuterFrame->SetInitialChildList(aPresContext, nsLayoutAtoms::captionList, captionFrame);
    }
    if (aIsPseudoParent) {
      aState.mPseudoFrames.mCellInner.mChildList.AddChild(aNewOuterFrame);
    }
  }

  return rv;
}

nsresult
nsCSSFrameConstructor::ConstructTableCaptionFrame(nsIPresShell*            aPresShell,
                                                  nsIPresContext*          aPresContext,
                                                  nsFrameConstructorState& aState,
                                                  nsIContent*              aContent,
                                                  nsIFrame*                aParentFrameIn,
                                                  nsIStyleContext*         aStyleContext,
                                                  nsTableCreator&          aTableCreator,
                                                  nsFrameItems&            aChildItems,
                                                  nsIFrame*&               aNewFrame,
                                                  PRBool&                  aIsPseudoParent)

{
  nsresult rv = NS_OK;
  if (!aPresShell || !aPresContext || !aParentFrameIn) return rv;

  nsIFrame* parentFrame = aParentFrameIn;
  aIsPseudoParent = PR_FALSE;
  // this frame may have a pseudo parent
  GetParentFrame(aPresShell, aPresContext, aTableCreator, *aParentFrameIn, 
                 nsLayoutAtoms::tableCaptionFrame, aState, parentFrame, aIsPseudoParent);
  if (!aIsPseudoParent && !aState.mPseudoFrames.IsEmpty()) {
    ProcessPseudoFrames(aPresContext, aState.mPseudoFrames, aChildItems);
  }

  rv = aTableCreator.CreateTableCaptionFrame(&aNewFrame);
  if (NS_FAILED(rv)) return rv;
  InitAndRestoreFrame(aPresContext, aState, aContent, 
                      parentFrame, aStyleContext, nsnull, aNewFrame);

  nsFrameItems childItems;
  // pass in aTableCreator so ProcessChildren will call TableProcessChildren
  rv = ProcessChildren(aPresShell, aPresContext, aState, aContent, aNewFrame,
                       PR_TRUE, childItems, PR_TRUE, &aTableCreator);
  if (NS_FAILED(rv)) return rv;
  aNewFrame->SetInitialChildList(aPresContext, nsnull, childItems.childList);
  if (aIsPseudoParent) {
    aState.mPseudoFrames.mTableOuter.mChildList2.AddChild(aNewFrame);
  }
  
  return rv;
}


nsresult
nsCSSFrameConstructor::ConstructTableRowGroupFrame(nsIPresShell*            aPresShell, 
                                                   nsIPresContext*          aPresContext,
                                                   nsFrameConstructorState& aState,
                                                   nsIContent*              aContent,
                                                   nsIFrame*                aParentFrameIn,
                                                   nsIStyleContext*         aStyleContext,
                                                   nsTableCreator&          aTableCreator,
                                                   PRBool                   aIsPseudo,
                                                   nsFrameItems&            aChildItems,
                                                   nsIFrame*&               aNewFrame, 
                                                   PRBool&                  aIsPseudoParent)
{
  nsresult rv = NS_OK;
  if (!aPresShell || !aPresContext || !aParentFrameIn) return rv;

  nsIFrame* parentFrame = aParentFrameIn;
  aIsPseudoParent = PR_FALSE;
  if (!aIsPseudo) {
    // this frame may have a pseudo parent
    GetParentFrame(aPresShell, aPresContext, aTableCreator, *aParentFrameIn, 
                   nsLayoutAtoms::tableRowGroupFrame, aState, parentFrame, aIsPseudoParent);
    if (!aIsPseudoParent && !aState.mPseudoFrames.IsEmpty()) {
      ProcessPseudoFrames(aPresContext, aState.mPseudoFrames, aChildItems);
    }
    if (!aIsPseudo && aIsPseudoParent && aState.mPseudoFrames.mRowGroup.mFrame) {
      ProcessPseudoFrames(aPresContext, aState.mPseudoFrames, nsLayoutAtoms::tableRowGroupFrame);
    }
  }

  const nsStyleDisplay* styleDisplay = 
    (const nsStyleDisplay*) aStyleContext->GetStyleData(eStyleStruct_Display);

  rv = aTableCreator.CreateTableRowGroupFrame(&aNewFrame);

  nsIFrame* scrollFrame = nsnull;
  if (IsScrollable(aPresContext, styleDisplay)) {
    // Create an area container for the frame
    BuildScrollFrame(aPresShell, aPresContext, aState, aContent, aStyleContext, 
                     aNewFrame, parentFrame, scrollFrame, aStyleContext);

  } 
  else {
    if (NS_FAILED(rv)) return rv;
    InitAndRestoreFrame(aPresContext, aState, aContent, parentFrame, 
                        aStyleContext, nsnull, aNewFrame);
  }

  if (!aIsPseudo) {
    nsFrameItems childItems;
    nsIFrame* captionFrame;
    rv = TableProcessChildren(aPresShell, aPresContext, aState, aContent, 
                              aNewFrame, aTableCreator, childItems, captionFrame);
    if (NS_FAILED(rv)) return rv;
    // if there are any anonymous children for the table, create frames for them
    CreateAnonymousFrames(aPresShell, aPresContext, nsnull, aState, aContent, aNewFrame,
                          childItems);

    aNewFrame->SetInitialChildList(aPresContext, nsnull, childItems.childList);
    if (aIsPseudoParent) {
      nsIFrame* child = (scrollFrame) ? scrollFrame : aNewFrame;
      aState.mPseudoFrames.mTableInner.mChildList.AddChild(child);
    }
  } 

  // if there is a scroll frame, use it as the one constructed
  if (scrollFrame) {
    aNewFrame = scrollFrame;
  }
  
  return rv;
}

nsresult
nsCSSFrameConstructor::ConstructTableColGroupFrame(nsIPresShell*            aPresShell, 
                                                   nsIPresContext*          aPresContext,
                                                   nsFrameConstructorState& aState,
                                                   nsIContent*              aContent,
                                                   nsIFrame*                aParentFrameIn,
                                                   nsIStyleContext*         aStyleContext,
                                                   nsTableCreator&          aTableCreator,
                                                   PRBool                   aIsPseudo,
                                                   nsFrameItems&            aChildItems,
                                                   nsIFrame*&               aNewFrame, 
                                                   PRBool&                  aIsPseudoParent)
{
  nsresult rv = NS_OK;
  if (!aPresShell || !aPresContext || !aParentFrameIn) return rv;

  nsIFrame* parentFrame = aParentFrameIn;
  aIsPseudoParent = PR_FALSE;
  if (!aIsPseudo) {
    // this frame may have a pseudo parent
    GetParentFrame(aPresShell, aPresContext, aTableCreator, *aParentFrameIn, 
                   nsLayoutAtoms::tableColGroupFrame, aState, parentFrame, aIsPseudoParent);
    if (!aIsPseudoParent && !aState.mPseudoFrames.IsEmpty()) {
      ProcessPseudoFrames(aPresContext, aState.mPseudoFrames, aChildItems);
    }
    if (!aIsPseudo && aIsPseudoParent && aState.mPseudoFrames.mColGroup.mFrame) {
      ProcessPseudoFrames(aPresContext, aState.mPseudoFrames, nsLayoutAtoms::tableColGroupFrame);
    }
  }

  rv = aTableCreator.CreateTableColGroupFrame(&aNewFrame);
  if (NS_FAILED(rv)) return rv;
  InitAndRestoreFrame(aPresContext, aState, aContent, 
                      parentFrame, aStyleContext, nsnull, aNewFrame);

  if (!aIsPseudo) {
    nsFrameItems childItems;
    nsIFrame* captionFrame;
    rv = TableProcessChildren(aPresShell, aPresContext, aState, aContent, aNewFrame,
                              aTableCreator, childItems, captionFrame);
    if (NS_FAILED(rv)) return rv;
    aNewFrame->SetInitialChildList(aPresContext, nsnull, childItems.childList);
    if (aIsPseudoParent) {
      aState.mPseudoFrames.mTableInner.mChildList.AddChild(aNewFrame);
    }
  }

  return rv;
}

nsresult
nsCSSFrameConstructor::ConstructTableRowFrame(nsIPresShell*            aPresShell, 
                                              nsIPresContext*          aPresContext,
                                              nsFrameConstructorState& aState,
                                              nsIContent*              aContent,
                                              nsIFrame*                aParentFrameIn,
                                              nsIStyleContext*         aStyleContext,
                                              nsTableCreator&          aTableCreator,
                                              PRBool                   aIsPseudo,
                                              nsFrameItems&            aChildItems,
                                              nsIFrame*&               aNewFrame,
                                              PRBool&                  aIsPseudoParent)
{
  nsresult rv = NS_OK;
  if (!aPresShell || !aPresContext || !aParentFrameIn) return rv;

  nsIFrame* parentFrame = aParentFrameIn;
  aIsPseudoParent = PR_FALSE;
  if (!aIsPseudo) {
    // this frame may have a pseudo parent
    GetParentFrame(aPresShell, aPresContext, aTableCreator, *aParentFrameIn, 
                   nsLayoutAtoms::tableRowFrame, aState, parentFrame, aIsPseudoParent);
    if (!aIsPseudoParent && !aState.mPseudoFrames.IsEmpty()) {
      ProcessPseudoFrames(aPresContext, aState.mPseudoFrames, aChildItems);
    }
    if (!aIsPseudo && aIsPseudoParent && aState.mPseudoFrames.mRow.mFrame) {
      ProcessPseudoFrames(aPresContext, aState.mPseudoFrames, nsLayoutAtoms::tableRowFrame);
    }
  }

  rv = aTableCreator.CreateTableRowFrame(&aNewFrame);
  if (NS_FAILED(rv)) return rv;
  InitAndRestoreFrame(aPresContext, aState, aContent, 
                      parentFrame, aStyleContext, nsnull, aNewFrame);
  if (!aIsPseudo) {
    nsFrameItems childItems;
    nsIFrame* captionFrame;
    rv = TableProcessChildren(aPresShell, aPresContext, aState, aContent, aNewFrame,
                              aTableCreator, childItems, captionFrame);
    if (NS_FAILED(rv)) return rv;
    // if there are any anonymous children for the table, create frames for them
    CreateAnonymousFrames(aPresShell, aPresContext, nsnull, aState, aContent, aNewFrame,
                          childItems);

    aNewFrame->SetInitialChildList(aPresContext, nsnull, childItems.childList);
    if (aIsPseudoParent) {
      aState.mPseudoFrames.mRowGroup.mChildList.AddChild(aNewFrame);
    }
  }

  return rv;
}
      
nsresult
nsCSSFrameConstructor::ConstructTableColFrame(nsIPresShell*            aPresShell, 
                                              nsIPresContext*          aPresContext,
                                              nsFrameConstructorState& aState,
                                              nsIContent*              aContent,
                                              nsIFrame*                aParentFrameIn,
                                              nsIStyleContext*         aStyleContext,
                                              nsTableCreator&          aTableCreator,
                                              PRBool                   aIsPseudo,
                                              nsFrameItems&            aChildItems,
                                              nsIFrame*&               aNewFrame,
                                              PRBool&                  aIsPseudoParent)
{
  nsresult rv = NS_OK;
  if (!aPresShell || !aPresContext || !aParentFrameIn || !aStyleContext) return rv;

  nsIFrame* parentFrame = aParentFrameIn;
  aIsPseudoParent = PR_FALSE;
  if (!aIsPseudo) {
    // this frame may have a pseudo parent
    GetParentFrame(aPresShell, aPresContext, aTableCreator, *aParentFrameIn, 
                   nsLayoutAtoms::tableColFrame, aState, parentFrame, aIsPseudoParent);
    if (!aIsPseudoParent && !aState.mPseudoFrames.IsEmpty()) {
      ProcessPseudoFrames(aPresContext, aState.mPseudoFrames, aChildItems);
    }
  }

  rv = aTableCreator.CreateTableColFrame(&aNewFrame); if (NS_FAILED(rv)) return rv;
  InitAndRestoreFrame(aPresContext, aState, aContent, parentFrame, aStyleContext, nsnull, aNewFrame);
  // if the parent frame was anonymous then reparent the style context
  nsCOMPtr<nsIStyleContext> parentStyleContext;
  parentFrame->GetStyleContext(getter_AddRefs(parentStyleContext));
  if (aIsPseudoParent) {
    aPresContext->ReParentStyleContext(aNewFrame, parentStyleContext);
  }

  // construct additional col frames if the col frame has a span > 1
  PRInt32 span = 1;
  nsCOMPtr<nsIDOMHTMLTableColElement> cgContent(do_QueryInterface(aContent));
  if (cgContent) { 
    cgContent->GetSpan(&span);
    nsIFrame* lastCol = aNewFrame;
    nsCOMPtr<nsIStyleContext> styleContext;
    for (PRInt32 spanX = 1; spanX < span; spanX++) {
      // The same content node should always resolve to the same style context.
      if (1 == spanX)
        aNewFrame->GetStyleContext(getter_AddRefs(styleContext));
      nsIFrame* newCol;
      rv = aTableCreator.CreateTableColFrame(&newCol); if (NS_FAILED(rv)) return rv;
      InitAndRestoreFrame(aPresContext, aState, aContent, parentFrame,
                          styleContext, nsnull, newCol);
      ((nsTableColFrame*)newCol)->SetType(eColAnonymousCol);
      lastCol->SetNextSibling(newCol);
      lastCol = newCol;
    }
  }

  if (!aIsPseudo) {
    nsFrameItems childItems;
    nsIFrame* captionFrame;
    rv = TableProcessChildren(aPresShell, aPresContext, aState, aContent, aNewFrame,
                              aTableCreator, childItems, captionFrame);
    if (NS_FAILED(rv)) return rv;
    aNewFrame->SetInitialChildList(aPresContext, nsnull, childItems.childList);
    if (aIsPseudoParent) {
      aState.mPseudoFrames.mColGroup.mChildList.AddChild(aNewFrame);
    }
  }
  
  return rv;
}

nsresult
nsCSSFrameConstructor::ConstructTableCellFrame(nsIPresShell*            aPresShell, 
                                               nsIPresContext*          aPresContext,
                                               nsFrameConstructorState& aState,
                                               nsIContent*              aContent,
                                               nsIFrame*                aParentFrameIn,
                                               nsIStyleContext*         aStyleContext,
                                               nsTableCreator&          aTableCreator,
                                               PRBool                   aIsPseudo,
                                               nsFrameItems&            aChildItems,
                                               nsIFrame*&               aNewCellOuterFrame,
                                               nsIFrame*&               aNewCellInnerFrame,
                                               PRBool&                  aIsPseudoParent)
{
  nsresult rv = NS_OK;
  if (!aPresShell || !aPresContext || !aParentFrameIn) return rv;

  nsIFrame* parentFrame = aParentFrameIn;
  aIsPseudoParent = PR_FALSE;
  if (!aIsPseudo) {
    // this frame may have a pseudo parent
    // use nsLayoutAtoms::tableCellFrame which will match if it is really nsLayoutAtoms::bcTableCellFrame
    GetParentFrame(aPresShell, aPresContext, aTableCreator, *aParentFrameIn, 
                   nsLayoutAtoms::tableCellFrame, aState, parentFrame, aIsPseudoParent);
    if (!aIsPseudoParent && !aState.mPseudoFrames.IsEmpty()) {
      ProcessPseudoFrames(aPresContext, aState.mPseudoFrames, aChildItems);
    }
    if (!aIsPseudo && aIsPseudoParent && aState.mPseudoFrames.mCellOuter.mFrame) {
      ProcessPseudoFrames(aPresContext, aState.mPseudoFrames, nsLayoutAtoms::tableCellFrame);
    }
  }

  rv = aTableCreator.CreateTableCellFrame(parentFrame, &aNewCellOuterFrame);
  if (NS_FAILED(rv)) return rv;
 
  // Initialize the table cell frame
  InitAndRestoreFrame(aPresContext, aState, aContent, 
                      parentFrame, aStyleContext, nsnull, aNewCellOuterFrame);
  // Create a block frame that will format the cell's content
  rv = aTableCreator.CreateTableCellInnerFrame(&aNewCellInnerFrame);

  if (NS_FAILED(rv)) {
    aNewCellOuterFrame->Destroy(aPresContext);
    aNewCellOuterFrame = nsnull;
    return rv;
  }
  
  // Resolve pseudo style and initialize the body cell frame
  nsCOMPtr<nsIStyleContext> innerPseudoStyle;
  aPresContext->ResolvePseudoStyleContextFor(aContent, nsHTMLAtoms::cellContentPseudo,
                                             aStyleContext,
                                             getter_AddRefs(innerPseudoStyle));
  InitAndRestoreFrame(aPresContext, aState, aContent, 
                      aNewCellOuterFrame, innerPseudoStyle, nsnull, aNewCellInnerFrame);

  if (!aIsPseudo) {
    PRBool haveFirstLetterStyle, haveFirstLineStyle;
    HaveSpecialBlockStyle(aPresContext, aContent, aStyleContext,
                          &haveFirstLetterStyle, &haveFirstLineStyle);

    // The block frame is a floater container
    nsFrameConstructorSaveState floaterSaveState;
    aState.PushFloaterContainingBlock(aNewCellInnerFrame, floaterSaveState,
                                      haveFirstLetterStyle, haveFirstLineStyle);

    // Process the child content
    nsFrameItems childItems;
    // pass in null tableCreator so ProcessChildren will not call TableProcessChildren
    rv = ProcessChildren(aPresShell, aPresContext, aState, aContent, aNewCellInnerFrame, 
                         PR_TRUE, childItems, PR_TRUE, nsnull);
    if (NS_FAILED(rv)) return rv;

    aNewCellInnerFrame->SetInitialChildList(aPresContext, nsnull, childItems.childList);
    if (aState.mFloatedItems.childList) {
      aNewCellInnerFrame->SetInitialChildList(aPresContext, nsLayoutAtoms::floaterList,
                                             aState.mFloatedItems.childList);
    }

    aNewCellOuterFrame->SetInitialChildList(aPresContext, nsnull, aNewCellInnerFrame);
    if (aIsPseudoParent) {
      aState.mPseudoFrames.mRow.mChildList.AddChild(aNewCellOuterFrame);
    }
  }

  return rv;
}

PRBool 
nsCSSFrameConstructor::MustGeneratePseudoParent(nsIPresContext*  aPresContext,
                                                nsIFrame*        aParentFrame,
                                                nsIAtom*         aTag,
                                                nsIContent*      aContent,
                                                nsIStyleContext* aStyleContext)
{
  if (!aStyleContext) return PR_FALSE;

  const nsStyleDisplay* display = (const nsStyleDisplay*)
    aStyleContext->GetStyleData(eStyleStruct_Display);

  if (NS_STYLE_DISPLAY_NONE == display->mDisplay) return PR_FALSE;
    
  // check tags first

  if ((nsLayoutAtoms::textTagName == aTag)) {
    return !IsOnlyWhiteSpace(aContent);
  }

  // exclude tags
  if ( nsLayoutAtoms::commentTagName == aTag) {
    return PR_FALSE;
  }

  return PR_TRUE;
}

// this is called when a non table related element is a child of a table, row group, 
// or row, but not a cell.
nsresult
nsCSSFrameConstructor::ConstructTableForeignFrame(nsIPresShell*            aPresShell, 
                                                  nsIPresContext*          aPresContext,
                                                  nsFrameConstructorState& aState,
                                                  nsIContent*              aContent,
                                                  nsIFrame*                aParentFrameIn,
                                                  nsIStyleContext*         aStyleContext,
                                                  nsTableCreator&          aTableCreator,
                                                  nsFrameItems&            aChildItems,
                                                  nsIFrame*&               aNewFrame,
                                                  PRBool&                  aIsPseudoParent)
{
  nsresult rv = NS_OK;
  aNewFrame = nsnull;
  if (!aPresShell || !aPresContext || !aParentFrameIn) return rv;

  nsIFrame* parentFrame = nsnull;
  aIsPseudoParent = PR_FALSE;

  // XXX form code needs to be fixed so that the forms can work without a frame.
  nsCOMPtr<nsIAtom> tag;
  aContent->GetTag(*getter_AddRefs(tag));

  // Do not construct pseudo frames for trees 
  if (MustGeneratePseudoParent(aPresContext, aParentFrameIn, tag.get(), aContent, aStyleContext)) {
    // this frame may have a pseudo parent, use block frame type to trigger foreign
    GetParentFrame(aPresShell, aPresContext, aTableCreator, *aParentFrameIn, 
                   nsLayoutAtoms::blockFrame, aState, parentFrame, aIsPseudoParent);
    if (!aIsPseudoParent && !aState.mPseudoFrames.IsEmpty()) {
      ProcessPseudoFrames(aPresContext, aState.mPseudoFrames, aChildItems);
    }
    //char buf[256];
    //sprintf(buf, "anonymous frame constructed for %s", tag.get());
    //NS_WARN_IF_FALSE(PR_FALSE, buf); 
  }

  if (!parentFrame) return rv; // if pseudo frame wasn't created

  // save the pseudo frame state XXX - why
  nsPseudoFrames prevPseudoFrames; 
  aState.mPseudoFrames.Reset(&prevPseudoFrames);

  nsFrameItems items;
  rv = ConstructFrame(aPresShell, aPresContext, aState, aContent, parentFrame, items);
  aNewFrame = items.childList;

  // restore the pseudo frame state XXX - why
  aState.mPseudoFrames = prevPseudoFrames;

  if (aIsPseudoParent) {
    aState.mPseudoFrames.mCellInner.mChildList.AddChild(aNewFrame);
  }

  return rv;
}

static PRBool 
NeedFrameFor(nsIFrame*   aParentFrame,
             nsIContent* aChildContent) 
{
  // don't create a whitespace frame if aParentFrame doesn't want it
  nsFrameState state;
  aParentFrame->GetFrameState(&state);
  if (NS_FRAME_EXCLUDE_IGNORABLE_WHITESPACE & state) {
    if (IsOnlyWhiteSpace(aChildContent)) {
      return PR_FALSE;
    }
  }
  return PR_TRUE;
}


nsresult
nsCSSFrameConstructor::TableProcessChildren(nsIPresShell*            aPresShell, 
                                            nsIPresContext*          aPresContext,
                                            nsFrameConstructorState& aState,
                                            nsIContent*              aContent,
                                            nsIFrame*                aParentFrame,
                                            nsTableCreator&          aTableCreator,
                                            nsFrameItems&            aChildItems,
                                            nsIFrame*&               aCaption)
{
  nsresult rv = NS_OK;
  if (!aPresShell || !aPresContext || !aContent || !aParentFrame) return rv;

  aCaption = nsnull;

  // save the incoming pseudo frame state 
  nsPseudoFrames priorPseudoFrames; 
  aState.mPseudoFrames.Reset(&priorPseudoFrames);

  nsCOMPtr<nsIAtom> parentFrameType;
  aParentFrame->GetFrameType(getter_AddRefs(parentFrameType));
  nsCOMPtr<nsIStyleContext> parentStyleContext;
  aParentFrame->GetStyleContext(getter_AddRefs(parentStyleContext));

  ChildIterator iter, last;
  for (ChildIterator::Init(aContent, &iter, &last);
       iter != last;
       ++iter) {
    nsCOMPtr<nsIContent> childContent = *iter;
    if (childContent &&
        (childContent->IsContentOfType(nsIContent::eELEMENT) ||
         childContent->IsContentOfType(nsIContent::eTEXT)) &&
        NeedFrameFor(aParentFrame, childContent)) {
      rv = TableProcessChild(aPresShell, aPresContext, aState, childContent,
                             aContent, aParentFrame,
                             parentFrameType, parentStyleContext,
                             aTableCreator, aChildItems, aCaption);
    }
    if (NS_FAILED(rv)) return rv;
  }
  // process the current pseudo frame state
  if (!aState.mPseudoFrames.IsEmpty()) {
    ProcessPseudoFrames(aPresContext, aState.mPseudoFrames, aChildItems);
  }

  // restore the incoming pseudo frame state 
  aState.mPseudoFrames = priorPseudoFrames;
  return rv;
}

nsresult
nsCSSFrameConstructor::TableProcessChild(nsIPresShell*            aPresShell, 
                                         nsIPresContext*          aPresContext,
                                         nsFrameConstructorState& aState,
                                         nsIContent*              aChildContent,
                                         nsIContent*              aParentContent,
                                         nsIFrame*                aParentFrame,
                                         nsIAtom*                 aParentFrameType,
                                         nsIStyleContext*         aParentStyleContext,
                                         nsTableCreator&          aTableCreator,
                                         nsFrameItems&            aChildItems,
                                         nsIFrame*&               aCaption)
{
  nsresult rv = NS_OK;
  
  PRBool childIsCaption = PR_FALSE;
  PRBool isPseudoParent = PR_FALSE;
    
  nsIFrame* childFrame = nsnull;
  nsCOMPtr<nsIStyleContext> childStyleContext;

  // Resolve the style context and get its display
  ResolveStyleContext(aPresContext, aParentFrame, aChildContent,
                      getter_AddRefs(childStyleContext));
  const nsStyleDisplay* styleDisplay = (const nsStyleDisplay*)
    childStyleContext->GetStyleData(eStyleStruct_Display);

  switch (styleDisplay->mDisplay) {
  case NS_STYLE_DISPLAY_TABLE:
    {
      PRBool pageBreakAfter = PR_FALSE;
      PRBool paginated;
      aPresContext->IsPaginated(&paginated);

      if (paginated) {
        // See if there is a page break before, if so construct one. Also see if there is one after
        pageBreakAfter = PageBreakBefore(aPresShell, aPresContext, aState, aChildContent, 
                                       aParentFrame, childStyleContext, aChildItems);
      }
      // construct the table frame
      nsIFrame* innerTableFrame;
      rv = ConstructTableFrame(aPresShell, aPresContext, aState, aChildContent, aParentFrame,
                               childStyleContext, aTableCreator, PR_FALSE, aChildItems,
                               childFrame, innerTableFrame, isPseudoParent);
      if (NS_SUCCEEDED(rv) && pageBreakAfter) {
        // Construct the page break after
        ConstructPageBreakFrame(aPresShell, aPresContext, aState, aChildContent,
                                aParentFrame, childStyleContext, aChildItems);
      }
    }
    break;

  case NS_STYLE_DISPLAY_TABLE_CAPTION:
    if (!aCaption) {  // only allow one caption
      nsIFrame* parentFrame = GetOuterTableFrame(aParentFrame);
      rv = ConstructTableCaptionFrame(aPresShell, aPresContext, aState, aChildContent, 
                                      parentFrame, childStyleContext, aTableCreator, 
                                      aChildItems, aCaption, isPseudoParent);
    }
    childIsCaption = PR_TRUE;
    break;

  case NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP:
    rv = ConstructTableColGroupFrame(aPresShell, aPresContext, aState, aChildContent, 
                                     aParentFrame, childStyleContext, aTableCreator, 
                                     PR_FALSE, aChildItems, childFrame, isPseudoParent);
    break;

  case NS_STYLE_DISPLAY_TABLE_HEADER_GROUP:
  case NS_STYLE_DISPLAY_TABLE_FOOTER_GROUP:
  case NS_STYLE_DISPLAY_TABLE_ROW_GROUP:
    rv = ConstructTableRowGroupFrame(aPresShell, aPresContext, aState, aChildContent, 
                                     aParentFrame, childStyleContext, aTableCreator, 
                                     PR_FALSE, aChildItems, childFrame, isPseudoParent);
    break;

  case NS_STYLE_DISPLAY_TABLE_ROW:
    rv = ConstructTableRowFrame(aPresShell, aPresContext, aState, aChildContent, 
                                aParentFrame, childStyleContext, aTableCreator, 
                                PR_FALSE, aChildItems, childFrame, isPseudoParent);
    break;

  case NS_STYLE_DISPLAY_TABLE_COLUMN:
    rv = ConstructTableColFrame(aPresShell, aPresContext, aState, aChildContent, 
                                aParentFrame, childStyleContext, aTableCreator, 
                                PR_FALSE, aChildItems, childFrame, isPseudoParent);
    break;


  case NS_STYLE_DISPLAY_TABLE_CELL:
    nsIFrame* innerCell;
    rv = ConstructTableCellFrame(aPresShell, aPresContext, aState, aChildContent, 
                                 aParentFrame, childStyleContext, aTableCreator, PR_FALSE, 
                                 aChildItems, childFrame, innerCell, isPseudoParent);
    break;

  case NS_STYLE_DISPLAY_NONE:
    aState.mFrameManager->SetUndisplayedContent(aChildContent,
                                                childStyleContext);
    break;

  default:
    {

      // if <form>'s parent is <tr>/<table>/<tbody>/<thead>/<tfoot> in html,
      // NOT create psuedoframe for it.
      // see bug 159359
      nsCOMPtr<nsINodeInfo> parentNodeInfo, childNodeInfo;
      aChildContent->GetNodeInfo(*getter_AddRefs(childNodeInfo));
      // Sometimes aChildContent is a #text node.  In those cases it
      // does not have a nodeinfo, and in those cases we want to
      // construct a foreign frame for it in any case.  So we can just
      // null-check the nodeinfo here.
      NS_ASSERTION(childNodeInfo ||
                   aChildContent->IsContentOfType(nsIContent::eTEXT),
                   "Non-#text nodes should have a nodeinfo here!");
      if (childNodeInfo) {
        aParentContent->GetNodeInfo(*getter_AddRefs(parentNodeInfo));
        if (childNodeInfo->Equals(nsHTMLAtoms::form, kNameSpaceID_None) &&
            (parentNodeInfo->Equals(nsHTMLAtoms::table, kNameSpaceID_None) ||
             parentNodeInfo->Equals(nsHTMLAtoms::tr, kNameSpaceID_None) ||
             parentNodeInfo->Equals(nsHTMLAtoms::tbody, kNameSpaceID_None) ||
             parentNodeInfo->Equals(nsHTMLAtoms::thead, kNameSpaceID_None) ||
             parentNodeInfo->Equals(nsHTMLAtoms::tfoot, kNameSpaceID_None))) {
          break;
        }
      }

      rv = ConstructTableForeignFrame(aPresShell, aPresContext, aState, aChildContent, 
                                      aParentFrame, childStyleContext, aTableCreator, 
                                      aChildItems, childFrame, isPseudoParent);
    }
    break;
  }

  // for every table related frame except captions and ones with pseudo parents, 
  // link into the child list
  if (childFrame && !childIsCaption && !isPseudoParent) {
    aChildItems.AddChild(childFrame);
  }
  return rv;
}

const nsStyleDisplay* 
nsCSSFrameConstructor:: GetDisplay(nsIFrame* aFrame)
{
  if (nsnull == aFrame) {
    return nsnull;
  }
  nsCOMPtr<nsIStyleContext> styleContext;
  aFrame->GetStyleContext(getter_AddRefs(styleContext));
  const nsStyleDisplay* display = 
    (const nsStyleDisplay*)styleContext->GetStyleData(eStyleStruct_Display);
  return display;
}

/***********************************************
 * END TABLE SECTION
 ***********************************************/

nsresult
nsCSSFrameConstructor::ConstructDocElementTableFrame(nsIPresShell*        aPresShell, 
                                                     nsIPresContext* aPresContext,
                                                     nsIContent*     aDocElement,
                                                     nsIFrame*       aParentFrame,
                                                     nsIFrame*&      aNewTableFrame,
                                                     nsILayoutHistoryState* aFrameState)
{
  nsFrameConstructorState state(aPresContext, nsnull, nsnull, nsnull, aFrameState);
  nsFrameItems    frameItems;

  ConstructFrame(aPresShell, aPresContext, state, aDocElement, aParentFrame, frameItems);
  aNewTableFrame = frameItems.childList;
  if (!aNewTableFrame) {
    NS_WARNING("cannot get table contentFrame");
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

/**
 * New one
 */
nsresult
nsCSSFrameConstructor::ConstructDocElementFrame(nsIPresShell*        aPresShell, 
                                                nsIPresContext*          aPresContext,
                                                nsFrameConstructorState& aState,
                                                nsIContent*              aDocElement,
                                                nsIFrame*                aParentFrame,
                                                nsIStyleContext*         aParentStyleContext,
                                                nsIFrame*&               aNewFrame)
{
    // how the root frame hierarchy should look

    /*

---------------No Scrollbars------


     AreaFrame or BoxFrame (InitialContainingBlock)
  


---------------Native Scrollbars------



     ScrollFrame

         ^
         |
     AreaFrame or BoxFrame (InitialContainingBlock)
  

---------------Gfx Scrollbars ------


     GfxScrollFrame

         ^
         |
     ScrollPort

         ^
         |
     AreaFrame or BoxFrame (InitialContainingBlock)
          
*/    

  aNewFrame = nsnull;

  if (!mTempFrameTreeState)
    aPresShell->CaptureHistoryState(getter_AddRefs(mTempFrameTreeState));

  // ----- reattach gfx scrollbars ------
  // Gfx scrollframes were created in the root frame but the primary frame map may have been destroyed if a 
  // new style sheet was loaded so lets reattach the frames to their content.
  if (mGfxScrollFrame) {
    nsIFrame* scrollPort = nsnull;
    mGfxScrollFrame->FirstChild(aPresContext, nsnull, &scrollPort);

    nsIFrame* gfxScrollbarFrame1 = nsnull;
    nsIFrame* gfxScrollbarFrame2 = nsnull;
    nsresult rv = scrollPort->GetNextSibling(&gfxScrollbarFrame1);
    if (gfxScrollbarFrame1) {
      nsCOMPtr<nsIContent> content;
      gfxScrollbarFrame1->GetContent(getter_AddRefs(content));
      // XXX This works, but why?
      aState.mFrameManager->SetPrimaryFrameFor(content, gfxScrollbarFrame1);

      rv = gfxScrollbarFrame1->GetNextSibling(&gfxScrollbarFrame2);
      if (gfxScrollbarFrame2) {
        gfxScrollbarFrame2->GetContent(getter_AddRefs(content));
        // XXX This works, but why?
        aState.mFrameManager->SetPrimaryFrameFor(content, gfxScrollbarFrame2);
      }
    }
  }

  // --------- CREATE AREA OR BOX FRAME -------
  nsCOMPtr<nsIStyleContext>  styleContext;
  aPresContext->ResolveStyleContextFor(aDocElement, aParentStyleContext,
                                       getter_AddRefs(styleContext));

  const nsStyleDisplay*  display = (const nsStyleDisplay*)
        styleContext->GetStyleData(eStyleStruct_Display);

  // Ensure that our XBL bindings are installed.
  if (!display->mBinding.IsEmpty()) {
    // Get the XBL loader.
    nsresult rv;
    PRBool resolveStyle;
    nsCOMPtr<nsIXBLBinding> binding;
    if (!gXBLService)
      return NS_ERROR_FAILURE;
    rv = gXBLService->LoadBindings(aDocElement, display->mBinding, PR_FALSE, getter_AddRefs(binding), &resolveStyle);
    if (NS_FAILED(rv))
      return NS_OK; // Binding will load asynchronously.

    if (binding) {
      nsCOMPtr<nsIBindingManager> bm;
      mDocument->GetBindingManager(getter_AddRefs(bm));
      if (bm)
        bm->AddToAttachedQueue(binding);
    }

    if (resolveStyle) {
      rv = ResolveStyleContext(aPresContext, aParentFrame, aDocElement,
                               getter_AddRefs(styleContext));
      if (NS_FAILED(rv))
        return rv;
    }
  }

  PRBool docElemIsTable = IsTableRelated(display->mDisplay, PR_FALSE);
 

  // --------- IF SCROLLABLE WRAP IN SCROLLFRAME --------

  PRBool isScrollable = IsScrollable(aPresContext, display);
  PRBool isPaginated = PR_FALSE;
  aPresContext->IsPaginated(&isPaginated);
  nsCOMPtr<nsIPrintPreviewContext> printPreviewContext(do_QueryInterface(aPresContext));

  nsIFrame* scrollFrame = nsnull;

  // build a scrollframe
  if ((!isPaginated || (isPaginated && printPreviewContext)) && isScrollable) {
    nsIFrame* newScrollFrame = nsnull;
    nsCOMPtr<nsIStyleContext> newContext;

    BeginBuildingScrollFrame( aPresShell, aPresContext,
                              aState,
                              aDocElement,
                              styleContext,
                              aParentFrame,
                              nsLayoutAtoms::scrolledContentPseudo,
                              mDocument,
                              PR_FALSE,
                              scrollFrame,
                              newContext,
                              newScrollFrame);

    styleContext = newContext;
    aParentFrame = newScrollFrame;
  }

  nsIFrame* contentFrame = nsnull;
  PRBool isBlockFrame = PR_FALSE;
  nsresult rv;

  if (docElemIsTable) {
      // if the document is a table then just populate it.
      rv = ConstructDocElementTableFrame(aPresShell, aPresContext, aDocElement, 
                                    aParentFrame, contentFrame,
                                    aState.mFrameState);
      if (NS_FAILED(rv)) {
        return rv;
      }
      contentFrame->GetStyleContext(getter_AddRefs(styleContext));
  } else {
        // otherwise build a box or a block
#if defined(MOZ_SVG)
        PRInt32 nameSpaceID;
#endif
#ifdef INCLUDE_XUL
        if (aDocElement->IsContentOfType(nsIContent::eXUL)) {
          rv = NS_NewDocElementBoxFrame(aPresShell, &contentFrame);
          if (NS_FAILED(rv)) {
            return rv;
          }
        }
        else
#endif 
#ifdef MOZ_SVG
        if (NS_SUCCEEDED(aDocElement->GetNameSpaceID(nameSpaceID)) && 
            (nameSpaceID == nsSVGAtoms::nameSpaceID)) {
          rv = NS_NewSVGOuterSVGFrame(aPresShell, aDocElement, &contentFrame);
          if (NS_FAILED(rv)) {
            return rv;
          }
          isBlockFrame = PR_TRUE;
        }
        else 
#endif
        {
          rv = NS_NewDocumentElementFrame(aPresShell, &contentFrame);
          if (NS_FAILED(rv)) {
            return rv;
          }
          isBlockFrame = PR_TRUE;
        }

        // initialize the child
        InitAndRestoreFrame(aPresContext, aState, aDocElement, 
                            aParentFrame, styleContext, nsnull, contentFrame);
  }
  
  // set the primary frame
  aState.mFrameManager->SetPrimaryFrameFor(aDocElement, contentFrame);

  // Finish building the scrollframe
  if (isScrollable) {
    FinishBuildingScrollFrame(aPresContext, 
                          aState,
                          aDocElement,
                          aParentFrame,
                          contentFrame,
                          styleContext);
    // primary is set above (to the contentFrame)
    
    aNewFrame = scrollFrame;
  } else {
     // if not scrollable the new frame is the content frame.
     aNewFrame = contentFrame;
  }

  mInitialContainingBlock = contentFrame;

  // if it was a table then we don't need to process our children.
  if (!docElemIsTable) {
    // Process the child content
    nsFrameConstructorSaveState absoluteSaveState;
    nsFrameConstructorSaveState floaterSaveState;
    nsFrameItems                childItems;

    if (isBlockFrame) {
      PRBool haveFirstLetterStyle, haveFirstLineStyle;
      HaveSpecialBlockStyle(aPresContext, aDocElement, styleContext,
                            &haveFirstLetterStyle, &haveFirstLineStyle);
      aState.PushAbsoluteContainingBlock(contentFrame, absoluteSaveState);
      aState.PushFloaterContainingBlock(contentFrame, floaterSaveState,
                                        haveFirstLetterStyle,
                                        haveFirstLineStyle);
    }

    // Create any anonymous frames the doc element frame requires
    // This must happen before ProcessChildren to ensure that popups are
    // never constructed before the popupset.
    CreateAnonymousFrames(aPresShell, aPresContext, nsnull, aState, aDocElement, contentFrame,
                          childItems, PR_TRUE);
    ProcessChildren(aPresShell, aPresContext, aState, aDocElement, contentFrame,
                    PR_TRUE, childItems, isBlockFrame);

    // Set the initial child lists
    contentFrame->SetInitialChildList(aPresContext, nsnull,
                                      childItems.childList);
 

    // only support absolute positioning if we are a block.
    // if we are a box don't do it.
    if (isBlockFrame) {
        if (aState.mAbsoluteItems.childList) {
          contentFrame->SetInitialChildList(aPresContext,
                                         nsLayoutAtoms::absoluteList,
                                         aState.mAbsoluteItems.childList);
        }
        if (aState.mFloatedItems.childList) {
          contentFrame->SetInitialChildList(aPresContext,
                                         nsLayoutAtoms::floaterList,
                                         aState.mFloatedItems.childList);
        }
    }
  }

  return NS_OK;
}


NS_IMETHODIMP
nsCSSFrameConstructor::ConstructRootFrame(nsIPresShell*        aPresShell, 
                                          nsIPresContext* aPresContext,
                                          nsIContent*     aDocElement,
                                          nsIFrame*&      aNewFrame)
{

  // how the root frame hierarchy should look

    /*

---------------No Scrollbars------



     ViewPortFrame (FixedContainingBlock) <---- RootView

         ^
         |
     RootFrame(DocElementContainingBlock)
  


---------------Native Scrollbars------



     ViewPortFrame (FixedContainingBlock) <---- RootView

         ^
         |
     ScrollFrame <--- RootScrollableView

         ^
         |
     RootFrame(DocElementContainingBlock)
  

---------------Gfx Scrollbars ------


     ViewPortFrame (FixedContainingBlock) <---- RootView

         ^
         |
     GfxScrollFrame

         ^
         |
     ScrollPort <--- RootScrollableView

         ^
         |
     RootFrame(DocElementContainingBlock)
          
*/    

  // Set up our style rule observer.
  nsCOMPtr<nsIBindingManager> bindingManager;
  mDocument->GetBindingManager(getter_AddRefs(bindingManager));
  if (bindingManager) {
    nsCOMPtr<nsIStyleRuleSupplier> ruleSupplier(do_QueryInterface(bindingManager));
    nsCOMPtr<nsIStyleSet> set;
    aPresShell->GetStyleSet(getter_AddRefs(set));
    set->SetStyleRuleSupplier(ruleSupplier);
  }
  
  // --------- BUILD VIEWPORT -----------
  nsIFrame*                 viewportFrame = nsnull;
  nsCOMPtr<nsIStyleContext> viewportPseudoStyle;

  aPresContext->ResolvePseudoStyleContextFor(nsnull, nsLayoutAtoms::viewportPseudo,
                                           nsnull,
                                           getter_AddRefs(viewportPseudoStyle));

  NS_NewViewportFrame(aPresShell, &viewportFrame);


  viewportFrame->Init(aPresContext, nsnull, nsnull, viewportPseudoStyle, nsnull);

  // Bind the viewport frame to the root view
  nsCOMPtr<nsIPresShell> presShell;
  aPresContext->GetShell(getter_AddRefs(presShell));
  nsCOMPtr<nsIViewManager> viewManager;
  presShell->GetViewManager(getter_AddRefs(viewManager));
  nsIView*        rootView;

  viewManager->GetRootView(rootView);
  viewportFrame->SetView(aPresContext, rootView);

  nsContainerFrame::SyncFrameViewProperties(aPresContext, viewportFrame,
                                            viewportPseudoStyle, rootView);

  // The viewport is the containing block for 'fixed' elements
  mFixedContainingBlock = viewportFrame;

  // --------- CREATE ROOT FRAME -------


    // Create the root frame. The document element's frame is a child of the
    // root frame.
    //
    // The root frame serves two purposes:
    // - reserves space for any margins needed for the document element's frame
    // - renders the document element's background. This ensures the background covers
    //   the entire canvas as specified by the CSS2 spec

    PRBool isPaginated = PR_FALSE;
    aPresContext->IsPaginated(&isPaginated);
    nsCOMPtr<nsIPrintPreviewContext> printPreviewContext(do_QueryInterface(aPresContext));

    nsIFrame* rootFrame = nsnull;
    nsIAtom* rootPseudo;
        
    if (!isPaginated) {
#ifdef INCLUDE_XUL
        if (aDocElement->IsContentOfType(nsIContent::eXUL)) 
        {
          NS_NewRootBoxFrame(aPresShell, &rootFrame);
        } else 
#endif
        {
          NS_NewCanvasFrame(aPresShell, &rootFrame);
        }

        rootPseudo = nsLayoutAtoms::canvasPseudo;
        mDocElementContainingBlock = rootFrame;
    } else {
        // Create a page sequence frame
        NS_NewSimplePageSequenceFrame(aPresShell, &rootFrame);
        rootPseudo = nsLayoutAtoms::pageSequencePseudo;
    }


  // --------- IF SCROLLABLE WRAP IN SCROLLFRAME --------

  // If the device supports scrolling (e.g., in galley mode on the screen and
  // for print-preview, but not when printing), then create a scroll frame that
  // will act as the scrolling mechanism for the viewport. 
  // XXX Do we even need a viewport when printing to a printer?
  PRBool isScrollable = PR_TRUE;

  //isScrollable = PR_FALSE;

  // As long as the webshell doesn't prohibit it, and the device supports
  // it, create a scroll frame that will act as the scolling mechanism for
  // the viewport.
  //
  // Threre are three possible values stored in the docshell:
  //  1) NS_STYLE_OVERFLOW_HIDDEN = no scrollbars
  //  2) NS_STYLE_OVERFLOW_AUTO = scrollbars appear if needed
  //  3) NS_STYLE_OVERFLOW_SCROLL = scrollbars always
  // Only need to create a scroll frame/view for cases 2 and 3.
  // Currently OVERFLOW_SCROLL isn't honored, as
  // scrollportview::SetScrollPref is not implemented.

  PRBool isHTML = aDocElement->IsContentOfType(nsIContent::eHTML);
  PRBool isXUL = PR_FALSE;

  if (!isHTML) {
    isXUL = aDocElement->IsContentOfType(nsIContent::eXUL);
  }

  // Never create scrollbars for XUL documents
#ifdef INCLUDE_XUL
  if (isXUL) {
    isScrollable = PR_FALSE;
  } else 
#endif
  {
    nsresult rv;
    nsCOMPtr<nsISupports> container;
    if (nsnull != aPresContext) {
      aPresContext->GetContainer(getter_AddRefs(container));
      if (nsnull != container) {
        nsCOMPtr<nsIScrollable> scrollableContainer = do_QueryInterface(container, &rv);
        if (NS_SUCCEEDED(rv) && scrollableContainer) {
          PRInt32 scrolling = -1;
          // XXX We should get prefs for X and Y and deal with these independently!
          scrollableContainer->GetCurrentScrollbarPreferences(nsIScrollable::ScrollOrientation_Y,&scrolling);
          if (NS_STYLE_OVERFLOW_HIDDEN == scrolling) {
            isScrollable = PR_FALSE;
          }
          // XXX NS_STYLE_OVERFLOW_SCROLL should create 'always on' scrollbars
        }
      }
    }
  }

  if (aPresContext) {
    PRBool isPaginated = PR_FALSE;
    if (NS_SUCCEEDED(aPresContext->IsPaginated(&isPaginated))) {
      if (isPaginated) {
        if (printPreviewContext) { // print preview
          aPresContext->GetPaginatedScrolling(&isScrollable);
        } else {
          isScrollable = PR_FALSE; // we are printing
        }
      }
    }
  }

  // if scrolling is still supported, check for the style data on the HTML and BODY
  // NOTE: the docElement in HTML will have a BODY child, and we have to check for
  //       no scrolling on that element as well as the docElement. Outside of HTML,
  //       we will not check the children of the docElement
  if (isScrollable) {
    NS_ASSERTION(!isXUL, "XUL documents should never be scrollable - see above");

    // see if the style is overflow: hidden, first on the document element
    nsCOMPtr<nsIStyleContext> styleContext;
    aPresContext->ResolveStyleContextFor(aDocElement, nsnull,
                                         getter_AddRefs(styleContext));
    if (styleContext) {
      const nsStyleDisplay* display = (const nsStyleDisplay*)
        styleContext->GetStyleData(eStyleStruct_Display);
      if (display) {
        if (display->mOverflow == NS_STYLE_OVERFLOW_HIDDEN || 
            display->mOverflow == NS_STYLE_OVERFLOW_SCROLLBARS_NONE) {
          isScrollable = PR_FALSE;
        }
      }
    }
    
    // if still scrollable, check the BODY element, but only if we are in an HTML document
    if (isScrollable && isHTML) {
      // XXX: there is a nice convenient method on nsIHTMLDocument that we could use to get the body
      //      element - it is called, strangely enough, GetBodyElement, but we cannot use it here
      //      because the separation between content and layout prohibits including nsIHTMLDocument
      //      without pulling more of content into content/shared, so we do the search the hard way

      // walk the children of the docElement looking fo the BODY
      nsCOMPtr<nsIContent> bodyElement;
      PRInt32 count = 0;
      aDocElement->ChildCount(count);
      for (PRInt32 i = 0; i < count; ++i) {
        nsCOMPtr<nsIContent> kidElement;
        aDocElement->ChildAt(i, *getter_AddRefs(kidElement));
        if (kidElement){
          nsCOMPtr<nsIAtom> kidTag;
          kidElement->GetTag(*getter_AddRefs(kidTag));
          if (kidTag == nsHTMLAtoms::body) {
            bodyElement = kidElement;
            // done looking
            break;
          }
        } else {
          NS_ASSERTION(PR_FALSE, "null child element returned from ChildAt");
          break;
        }
      }      
      if (bodyElement) {
        nsCOMPtr<nsIStyleContext> bodyContext;
        aPresContext->ResolveStyleContextFor(bodyElement, styleContext,
                                             getter_AddRefs(bodyContext));
        if (bodyContext) {
          const nsStyleDisplay* display = (const nsStyleDisplay*)
            bodyContext->GetStyleData(eStyleStruct_Display);
          if (display) {
            if (display->mOverflow == NS_STYLE_OVERFLOW_HIDDEN || 
                display->mOverflow == NS_STYLE_OVERFLOW_SCROLLBARS_NONE) {
              isScrollable = PR_FALSE;
            }
          }
        }
      }
    }
  }

  nsIFrame* newFrame = rootFrame;
  nsCOMPtr<nsIStyleContext> rootPseudoStyle;
  // we must create a state because if the scrollbars are GFX it needs the 
  // state to build the scrollbar frames.
  nsFrameConstructorState state(aPresContext,
                                      nsnull,
                                      nsnull,
                                      nsnull);

  nsIFrame* parentFrame = viewportFrame;

  // If paginated, make sure we don't put scrollbars in
  if (isPaginated && !printPreviewContext)
    aPresContext->ResolvePseudoStyleContextFor(nsnull, rootPseudo,
                                               viewportPseudoStyle,
                                               getter_AddRefs(rootPseudoStyle));
  else if (isScrollable) {

      // Build the frame. We give it the content we are wrapping which is the document,
      // the root frame, the parent view port frame, and we should get back the new
      // frame and the scrollable view if one was created.

      // resolve a context for the scrollframe
      nsCOMPtr<nsIStyleContext>  styleContext;
      aPresContext->ResolvePseudoStyleContextFor(nsnull,
                                                 nsLayoutAtoms::viewportScrollPseudo,
                                                 viewportPseudoStyle,
                                                 getter_AddRefs(styleContext));


      nsIFrame* newScrollableFrame = nsnull;

      BeginBuildingScrollFrame( aPresShell,
                                aPresContext,
                                state,
                                aDocElement,
                                styleContext,
                                viewportFrame,
                                rootPseudo,
                                mDocument,
                                PR_TRUE,
                                newFrame,
                                rootPseudoStyle,
                                newScrollableFrame);

      // Inform the view manager about the root scrollable view
      nsIView* view = nsnull;
      newScrollableFrame->GetView(aPresContext, &view);
      NS_ENSURE_TRUE(view, NS_ERROR_FAILURE);

      nsIScrollableView* scrollableView = nsnull;
      NS_ENSURE_SUCCESS(CallQueryInterface(view, &scrollableView), NS_ERROR_FAILURE);

      viewManager->SetRootScrollableView(scrollableView);
      parentFrame = newScrollableFrame;

      // if gfx scrollbars store them
      if (HasGfxScrollbars())
        mGfxScrollFrame = newFrame;
      else
        mGfxScrollFrame = nsnull;

  } else {
    // If no scrollbars and xul, don't build a scrollframe at all. 
    if (isXUL) {
      aPresContext->ResolvePseudoStyleContextFor(nsnull, rootPseudo,
                                                 viewportPseudoStyle,
                                                 getter_AddRefs(rootPseudoStyle));
    } else {
      // if HTML the always create a scrollframe so anchors work. That way you can scroll to 
      // anchors even if we don't have scrollbars.

      // create a style context for the scrollport of the viewport
      nsCOMPtr<nsIStyleContext> scrollPseudoStyle;
      aPresContext->ResolvePseudoStyleContextFor(nsnull,
                                                nsLayoutAtoms::scrolledContentPseudo,
                                                viewportPseudoStyle,
                                                getter_AddRefs(scrollPseudoStyle));

      // create scrollframe
      nsIFrame* scrollFrame = nsnull;
      NS_NewScrollPortFrame(aPresShell, &scrollFrame);
      NS_ENSURE_TRUE(scrollFrame, NS_ERROR_FAILURE);

      scrollFrame->Init(aPresContext, nsnull, parentFrame, scrollPseudoStyle, nsnull);

      // resolve a new style for the root frame
      aPresContext->ResolvePseudoStyleContextFor(nsnull, rootPseudo,
                                                 scrollPseudoStyle,
                                                 getter_AddRefs(rootPseudoStyle));

      // Inform the view manager about the root scrollable view
      nsIView* view = nsnull;
      scrollFrame->GetView(aPresContext, &view);
      NS_ENSURE_TRUE(view, NS_ERROR_FAILURE);

      nsIScrollableView* scrollableView = nsnull;
      NS_ENSURE_SUCCESS(CallQueryInterface(view, &scrollableView), NS_ERROR_FAILURE);

      viewManager->SetRootScrollableView(scrollableView);

      parentFrame = scrollFrame;
      newFrame = scrollFrame;
    }
  }
  

  rootFrame->Init(aPresContext, aDocElement, parentFrame,
                  rootPseudoStyle, nsnull);
  
  if (!isPaginated || (isPaginated && printPreviewContext)) {
    if (isScrollable) {
      FinishBuildingScrollFrame(aPresContext, 
                                state,
                                aDocElement,
                                parentFrame,
                                rootFrame,
                                rootPseudoStyle);

      // set the primary frame to the root frame
      state.mFrameManager->SetPrimaryFrameFor(aDocElement, rootFrame);
    } else { // if not scrollable
      if (!isXUL) { // if not XUL
        parentFrame->SetInitialChildList(aPresContext, nsnull, rootFrame);
      }
    }
  } 
  
  if (isPaginated) { // paginated
    // Create the first page
    nsIFrame* pageFrame;
    NS_NewPageFrame(aPresShell, &pageFrame);

    // The page is the containing block for 'fixed' elements. which are repeated
    // on every page
    mFixedContainingBlock = pageFrame;

    // Initialize the page and force it to have a view. This makes printing of
    // the pages easier and faster.
    nsCOMPtr<nsIStyleContext> pagePseudoStyle;

    aPresContext->ResolvePseudoStyleContextFor(nsnull, nsLayoutAtoms::pagePseudo,
                                               rootPseudoStyle,
                                               getter_AddRefs(pagePseudoStyle));

    pageFrame->Init(aPresContext, nsnull, rootFrame, pagePseudoStyle,
                    nsnull);
    nsHTMLContainerFrame::CreateViewForFrame(aPresContext, pageFrame,
                                             pagePseudoStyle, nsnull, PR_TRUE);
    nsIFrame* pageContentFrame = nsnull;
    NS_NewPageContentFrame(aPresShell, &pageContentFrame);

    nsCOMPtr<nsIStyleContext> pageContentPseudoStyle;
    aPresContext->ResolvePseudoStyleContextFor(nsnull, nsLayoutAtoms::pageContentPseudo,
                                               pagePseudoStyle,
                                               getter_AddRefs(pageContentPseudoStyle));

    pageContentFrame->Init(aPresContext, nsnull, pageFrame, pageContentPseudoStyle, nsnull);
    nsHTMLContainerFrame::CreateViewForFrame(aPresContext, pageContentFrame,
                                             pageContentPseudoStyle, nsnull, PR_TRUE);

    // The eventual parent of the document element frame
    mDocElementContainingBlock = pageContentFrame;
    mFixedContainingBlock = pageContentFrame;

    // Set the initial child lists
    pageFrame->SetInitialChildList(aPresContext, nsnull, pageContentFrame);

    // Set the initial child lists
    rootFrame->SetInitialChildList(aPresContext, nsnull, pageFrame);

  }

  viewportFrame->SetInitialChildList(aPresContext, nsnull, newFrame);
  
  aNewFrame = viewportFrame;



  return NS_OK;  
}


nsresult
nsCSSFrameConstructor::CreatePlaceholderFrameFor(nsIPresShell*    aPresShell, 
                                                 nsIPresContext*  aPresContext,
                                                 nsIFrameManager* aFrameManager,
                                                 nsIContent*      aContent,
                                                 nsIFrame*        aFrame,
                                                 nsIStyleContext* aStyleContext,
                                                 nsIFrame*        aParentFrame,
                                                 nsIFrame**       aPlaceholderFrame)
{
  nsPlaceholderFrame* placeholderFrame;
  nsresult            rv = NS_NewPlaceholderFrame(aPresShell, (nsIFrame**)&placeholderFrame);

  if (NS_SUCCEEDED(rv)) {
    // The placeholder frame gets a pseudo style context
    nsCOMPtr<nsIStyleContext>  placeholderStyle;
    nsCOMPtr<nsIStyleContext> parentContext = aStyleContext->GetParent();
    aPresContext->ResolveStyleContextForNonElement(parentContext,
                                             getter_AddRefs(placeholderStyle));
    placeholderFrame->Init(aPresContext, aContent, aParentFrame,
                           placeholderStyle, nsnull);
  
    // The placeholder frame has a pointer back to the out-of-flow frame
    placeholderFrame->SetOutOfFlowFrame(aFrame);
  
    nsFrameState frameState;
    aFrame->GetFrameState(&frameState);
    aFrame->SetFrameState(frameState | NS_FRAME_OUT_OF_FLOW);

    // Add mapping from absolutely positioned frame to its placeholder frame
    aFrameManager->RegisterPlaceholderFrame(placeholderFrame);

    *aPlaceholderFrame = NS_STATIC_CAST(nsIFrame*, placeholderFrame);
  }

  return rv;
}


nsWidgetRendering
nsCSSFrameConstructor::GetFormElementRenderingMode(nsIPresContext*		aPresContext,
																									 nsWidgetType				aWidgetType) 
{ 
  if (!aPresContext) { return eWidgetRendering_Gfx;}

  nsWidgetRendering mode;
  aPresContext->GetWidgetRenderingMode(&mode);

	switch (mode)
	{ 
		case eWidgetRendering_Gfx: 
			return eWidgetRendering_Gfx; 

		case eWidgetRendering_PartialGfx: 
			switch (aWidgetType)
			{
				case eWidgetType_Button:
				case eWidgetType_Checkbox:
				case eWidgetType_Radio:
				case eWidgetType_Text:
					return eWidgetRendering_Gfx; 

				default: 
					return eWidgetRendering_Native; 
			} 

		case eWidgetRendering_Native: 
		  PRBool useNativeWidgets = PR_FALSE;
	    nsIDeviceContext* dc;
	    aPresContext->GetDeviceContext(&dc);
	    if (dc) {
	      PRBool  supportsWidgets;
	      if (NS_SUCCEEDED(dc->SupportsNativeWidgets(supportsWidgets))) {
	        useNativeWidgets = supportsWidgets;
	      }
	      NS_RELEASE(dc);
	    }
			if (useNativeWidgets) 
				return eWidgetRendering_Native;
			else
				return eWidgetRendering_Gfx;
	}
	return eWidgetRendering_Gfx; 
}


nsresult
nsCSSFrameConstructor::ConstructRadioControlFrame(nsIPresShell*        aPresShell, 
                                                 nsIPresContext*  aPresContext,
                                                 nsIFrame*&   aNewFrame,
                                                 nsIContent*  aContent,
                                                 nsIStyleContext* aStyleContext)
{
  nsresult rv = NS_OK;
	if (GetFormElementRenderingMode(aPresContext, eWidgetType_Radio) == eWidgetRendering_Gfx)
		rv = NS_NewGfxRadioControlFrame(aPresShell, &aNewFrame);
	else
    NS_ASSERTION(0, "We longer support native widgets");

  if (NS_FAILED(rv)) {
    aNewFrame = nsnull;
    return rv;
  }

  nsCOMPtr<nsIStyleContext> radioStyle;
  aPresContext->ResolvePseudoStyleContextFor(aContent,
                                             nsHTMLAtoms::radioPseudo,
                                             aStyleContext,
                                             getter_AddRefs(radioStyle));
  nsIRadioControlFrame* radio = nsnull;
  if (aNewFrame != nsnull && NS_SUCCEEDED(aNewFrame->QueryInterface(NS_GET_IID(nsIRadioControlFrame), (void**)&radio))) {
    radio->SetRadioButtonFaceStyleContext(radioStyle);
    NS_RELEASE(radio);
  }
  return rv;
}

nsresult
nsCSSFrameConstructor::ConstructCheckboxControlFrame(nsIPresShell*    aPresShell, 
                                                     nsIPresContext*  aPresContext,
                                                     nsIFrame*&       aNewFrame,
                                                     nsIContent*      aContent,
                                                     nsIStyleContext* aStyleContext)
{
  nsresult rv = NS_OK;
	if (GetFormElementRenderingMode(aPresContext, eWidgetType_Checkbox) == eWidgetRendering_Gfx)
		rv = NS_NewGfxCheckboxControlFrame(aPresShell, &aNewFrame);
	else
    NS_ASSERTION(0, "We longer support native widgets");


  if (NS_FAILED(rv)) {
    aNewFrame = nsnull;
  }

  nsCOMPtr<nsIStyleContext> checkboxStyle;
  aPresContext->ResolvePseudoStyleContextFor(aContent, nsHTMLAtoms::checkPseudo, 
                                             aStyleContext, getter_AddRefs(checkboxStyle));
  nsICheckboxControlFrame* checkbox = nsnull;
  if (aNewFrame != nsnull && 
      NS_SUCCEEDED(aNewFrame->QueryInterface(NS_GET_IID(nsICheckboxControlFrame), (void**)&checkbox))) {
    checkbox->SetCheckboxFaceStyleContext(checkboxStyle);
    NS_RELEASE(checkbox);
  }
  return rv;
}

nsresult
nsCSSFrameConstructor::ConstructButtonControlFrame(nsIPresShell*   aPresShell,
                                                   nsIPresContext* aPresContext,
                                                   nsIFrame*&      aNewFrame)
{
  nsresult rv = NS_OK;
  if (GetFormElementRenderingMode(aPresContext, eWidgetType_Button)
      == eWidgetRendering_Gfx)
    rv = NS_NewGfxButtonControlFrame(aPresShell, &aNewFrame);
  else
    NS_ASSERTION(0, "We longer support native widgets");

  if (NS_FAILED(rv)) {
    aNewFrame = nsnull;
  }
  return rv;
}

nsresult
nsCSSFrameConstructor::ConstructTextControlFrame(nsIPresShell*   aPresShell,
                                                 nsIPresContext* aPresContext,
                                                 nsIFrame*&      aNewFrame,
                                                 nsIContent*     aContent)
{
  if (!aPresContext) { return NS_ERROR_NULL_POINTER;}
  nsresult rv = NS_OK;

  nsWidgetRendering mode;
  aPresContext->GetWidgetRenderingMode(&mode);
  if (eWidgetRendering_Gfx == mode) 
  {
    rv = NS_NewTextControlFrame(aPresShell, &aNewFrame);
    if (NS_FAILED(rv)) {
      aNewFrame = nsnull;
    }
  }
  if (!aNewFrame)
  {
    NS_ASSERTION(0, "We longer support native widgets");
  }
  return rv;
}

PRBool
nsCSSFrameConstructor::HasGfxScrollbars()
{
#ifndef INCLUDE_XUL
  return PR_FALSE;
#endif
  // Get the Prefs
  if (!mGotGfxPrefs) {
    nsCOMPtr<nsIPref> pref(do_GetService(NS_PREF_CONTRACTID));
    if (pref) {
      PRBool hasGfxScroll = PR_FALSE; // use a temp since we have a PRPackedBool
      pref->GetBoolPref("nglayout.widget.gfxscrollbars", &hasGfxScroll);
      mHasGfxScrollbars = hasGfxScroll;
      mGotGfxPrefs = PR_TRUE;
    } else {
      mHasGfxScrollbars = PR_FALSE;
    }
  }

  // while we don't support native scrollbars for Seamonkey, some embedding
  // clients demand them (but may still want XUL). Give them that option.
  return mHasGfxScrollbars;
}

PRBool
nsCSSFrameConstructor::UseXBLForms()
{
  if (!mGotXBLFormPrefs) {
    nsCOMPtr<nsIPref> pref(do_GetService(NS_PREF_CONTRACTID));
    if (pref) {
      PRBool useXBLForms = PR_FALSE; // use a temp since we have a PRPackedBool
      pref->GetBoolPref("nglayout.debug.enable_xbl_forms", &useXBLForms);
      mUseXBLForms = useXBLForms;
      mGotXBLFormPrefs = PR_TRUE;
    }
  }

  return mUseXBLForms;
}

nsresult
nsCSSFrameConstructor::ConstructSelectFrame(nsIPresShell*        aPresShell, 
                                            nsIPresContext*          aPresContext,
                                            nsFrameConstructorState& aState,
                                            nsIContent*              aContent,
                                            nsIFrame*                aParentFrame,
                                            nsIAtom*                 aTag,
                                            nsIStyleContext*         aStyleContext,
                                            nsIFrame*&               aNewFrame,
                                            PRBool&                  aProcessChildren,
                                            PRBool                   aIsAbsolutelyPositioned,
                                            PRBool&                  aFrameHasBeenInitialized,
                                            PRBool                   aIsFixedPositioned,
                                            nsFrameItems&            aFrameItems)
{
  nsresult rv = NS_OK;
  const PRInt32 kNoSizeSpecified = -1;

  // Construct a frame-based listbox or combobox
  nsCOMPtr<nsIDOMHTMLSelectElement> sel(do_QueryInterface(aContent));
  PRInt32 size = 1;
  if (sel) {
    sel->GetSize(&size); 
    PRBool multipleSelect = PR_FALSE;
    sel->GetMultiple(&multipleSelect);
     // Construct a combobox if size=1 or no size is specified and its multiple select
    if (((1 == size || 0 == size) || (kNoSizeSpecified  == size)) && (PR_FALSE == multipleSelect)) {
        // Construct a frame-based combo box.
        // The frame-based combo box is built out of tree parts. A display area, a button and
        // a dropdown list. The display area and button are created through anonymous content.
        // The drop-down list's frame is created explicitly. The combobox frame shares it's content
        // with the drop-down list.
      PRUint32 flags = NS_BLOCK_SHRINK_WRAP | 
          ((aIsAbsolutelyPositioned | aIsFixedPositioned)?NS_BLOCK_SPACE_MGR:0);
      nsIFrame * comboboxFrame;
      rv = NS_NewComboboxControlFrame(aPresShell, &comboboxFrame, flags);

      // Determine geometric parent for the combobox frame
      nsIFrame* geometricParent = aParentFrame;
      if (aIsAbsolutelyPositioned) {
        geometricParent = aState.mAbsoluteItems.containingBlock;
      } else if (aIsFixedPositioned) {
        geometricParent = aState.mFixedItems.containingBlock;
      }
      // Save the history state so we don't restore during construction
      // since the complete tree is required before we restore.
      nsILayoutHistoryState *historyState = aState.mFrameState;
      aState.mFrameState = nsnull;
      // Initialize the combobox frame
      InitAndRestoreFrame(aPresContext, aState, aContent, geometricParent,
                          aStyleContext, nsnull, comboboxFrame);

      nsHTMLContainerFrame::CreateViewForFrame(aPresContext, comboboxFrame,
                                               aStyleContext, aParentFrame, PR_FALSE);
      ///////////////////////////////////////////////////////////////////
      // Combobox - Old Native Implementation
      ///////////////////////////////////////////////////////////////////
      nsIComboboxControlFrame* comboBox = nsnull;
      CallQueryInterface(comboboxFrame, &comboBox);
      NS_ASSERTION(comboBox, "NS_NewComboboxControlFrame returned frame that "
                             "doesn't implement nsIComboboxControlFrame");
      comboBox->SetFrameConstructor(this);

        // Create a listbox
      nsIFrame * listFrame;
      rv = NS_NewListControlFrame(aPresShell, &listFrame);

        // Notify the listbox that it is being used as a dropdown list.
      nsIListControlFrame * listControlFrame;
      rv = CallQueryInterface(listFrame, &listControlFrame);
      if (NS_SUCCEEDED(rv)) {
        listControlFrame->SetComboboxFrame(comboboxFrame);
      }
         // Notify combobox that it should use the listbox as it's popup
      comboBox->SetDropDown(listFrame);

        // Resolve psuedo element style for the dropdown list 
      nsCOMPtr<nsIStyleContext> listStyle;
      rv = aPresContext->ResolvePseudoStyleContextFor(aContent, 
                                              nsHTMLAtoms::dropDownListPseudo, 
                                              aStyleContext,
                                              getter_AddRefs(listStyle));

      // Initialize the scroll frame positioned. Note that it is NOT
      // initialized as absolutely positioned.
      nsIFrame* newFrame = nsnull;
      nsIFrame* scrolledFrame = nsnull;
      NS_NewSelectsAreaFrame(aPresShell, &scrolledFrame, flags);

      InitializeSelectFrame(aPresShell, aPresContext, aState, listFrame, scrolledFrame, aContent, comboboxFrame,
                           listStyle, PR_FALSE, PR_FALSE, PR_TRUE);
      newFrame = listFrame;
      // XXX Temporary for Bug 19416
      {
        nsIView * lstView;
        scrolledFrame->GetView(aPresContext, &lstView);
        if (lstView) {
          lstView->IgnoreSetPosition(PR_TRUE);
        }
      }

        // Set flag so the events go to the listFrame not child frames.
        // XXX: We should replace this with a real widget manager similar
        // to how the nsFormControlFrame works. Re-directing events is a temporary Kludge.
      nsIView *listView; 
      listFrame->GetView(aPresContext, &listView);
      NS_ASSERTION(nsnull != listView,"ListFrame's view is nsnull");
      nsIWidget * viewWidget;
      listView->GetWidget(viewWidget);
      //viewWidget->SetOverrideShow(PR_TRUE);
      NS_RELEASE(viewWidget);
      //listView->SetViewFlags(NS_VIEW_PUBLIC_FLAG_DONT_CHECK_CHILDREN);

      // Create display and button frames from the combobox's anonymous content
      nsFrameItems childItems;
      CreateAnonymousFrames(aPresShell, aPresContext, nsHTMLAtoms::combobox,
                            aState, aContent, comboboxFrame, childItems);
  
      comboboxFrame->SetInitialChildList(aPresContext, nsnull,
                                         childItems.childList);

      // Initialize the additional popup child list which contains the
      // dropdown list frame.
      nsFrameItems popupItems;
      popupItems.AddChild(listFrame);
      comboboxFrame->SetInitialChildList(aPresContext,
                                         nsLayoutAtoms::popupList,
                                         popupItems.childList);

      // Don't process, the children, They are already processed by the
      // InitializeScrollFrame call above.
      aProcessChildren = PR_FALSE;
      aNewFrame = comboboxFrame;
      aFrameHasBeenInitialized = PR_TRUE;
      aState.mFrameState = historyState;
      if (aState.mFrameState && aState.mFrameManager) {
        // Restore frame state for the entire subtree of |comboboxFrame|.
        aState.mFrameManager->RestoreFrameState(aPresContext, comboboxFrame,
                                                aState.mFrameState);
      }
    } else {
      ///////////////////////////////////////////////////////////////////
      // ListBox - Old Native Implementation
      ///////////////////////////////////////////////////////////////////
      nsIFrame * listFrame;
      rv = NS_NewListControlFrame(aPresShell, &listFrame);
      aNewFrame = listFrame;

      PRUint32 flags = NS_BLOCK_SHRINK_WRAP | 
          ((aIsAbsolutelyPositioned | aIsFixedPositioned)?NS_BLOCK_SPACE_MGR:0);
      nsIFrame* scrolledFrame = nsnull;
      NS_NewSelectsAreaFrame(aPresShell, &scrolledFrame, flags);

      // ******* this code stolen from Initialze ScrollFrame ********
      // please adjust this code to use BuildScrollFrame.

      InitializeSelectFrame(aPresShell, aPresContext, aState, listFrame, scrolledFrame, aContent, aParentFrame,
                            aStyleContext, aIsAbsolutelyPositioned, aIsFixedPositioned, PR_FALSE);

      aNewFrame = listFrame;

        // Set flag so the events go to the listFrame not child frames.
        // XXX: We should replace this with a real widget manager similar
        // to how the nsFormControlFrame works.
        // Re-directing events is a temporary Kludge.
      nsIView *listView; 
      listFrame->GetView(aPresContext, &listView);
      NS_ASSERTION(nsnull != listView,"ListFrame's view is nsnull");
      //listView->SetViewFlags(NS_VIEW_PUBLIC_FLAG_DONT_CHECK_CHILDREN);
      aFrameHasBeenInitialized = PR_TRUE;
    }
  }
  return rv;

}

/**
 * Used to be InitializeScrollFrame but now its only used for the select tag
 * But the select tag should really be fixed to use GFX scrollbars that can
 * be create with BuildScrollFrame.
 */
nsresult
nsCSSFrameConstructor::InitializeSelectFrame(nsIPresShell*        aPresShell, 
                                             nsIPresContext*          aPresContext,
                                             nsFrameConstructorState& aState,
                                             nsIFrame*                scrollFrame,
                                             nsIFrame*                scrolledFrame,
                                             nsIContent*              aContent,
                                             nsIFrame*                aParentFrame,
                                             nsIStyleContext*         aStyleContext,
                                             PRBool                   aIsAbsolutelyPositioned,
                                             PRBool                   aIsFixedPositioned,
                                             PRBool                   aCreateBlock)
{
  // Initialize it
  nsIFrame* geometricParent = aParentFrame;
    
  if (aIsAbsolutelyPositioned) {
    geometricParent = aState.mAbsoluteItems.containingBlock;
  } else if (aIsFixedPositioned) {
    geometricParent = aState.mFixedItems.containingBlock;
  }
  
  nsCOMPtr<nsIStyleContext> scrollPseudoStyle;
  nsCOMPtr<nsIStyleContext> scrolledPseudoStyle;

  
    aPresContext->ResolvePseudoStyleContextFor(aContent,
                                  nsLayoutAtoms::scrolledContentPseudo,
                                  aStyleContext,
                                  getter_AddRefs(scrolledPseudoStyle));

    InitAndRestoreFrame(aPresContext, aState, aContent, 
                        geometricParent, aStyleContext, nsnull, scrollFrame);

    // Initialize the frame and force it to have a view
    // the scrolled frame is anonymous and does not have a content node
    InitAndRestoreFrame(aPresContext, aState, aContent, 
                        scrollFrame, scrolledPseudoStyle, nsnull, scrolledFrame);

    nsHTMLContainerFrame::CreateViewForFrame(aPresContext, scrolledFrame,
                                           scrolledPseudoStyle, nsnull, PR_TRUE);


    // The area frame is a floater container
    PRBool haveFirstLetterStyle, haveFirstLineStyle;
    HaveSpecialBlockStyle(aPresContext, aContent, aStyleContext,
                          &haveFirstLetterStyle, &haveFirstLineStyle);
    nsFrameConstructorSaveState floaterSaveState;
    aState.PushFloaterContainingBlock(scrolledFrame, floaterSaveState,
                                      haveFirstLetterStyle,
                                      haveFirstLineStyle);

    // Process children
    nsFrameConstructorSaveState absoluteSaveState;
    nsFrameItems                childItems;
    PRBool                      isPositionedContainingBlock = aIsAbsolutelyPositioned ||
                                                              aIsFixedPositioned;

    if (isPositionedContainingBlock) {
      // The area frame becomes a container for child frames that are
      // absolutely positioned
      aState.PushAbsoluteContainingBlock(scrolledFrame, absoluteSaveState);
    }
     
    ProcessChildren(aPresShell, aPresContext, aState, aContent, scrolledFrame, PR_FALSE,
                    childItems, PR_TRUE);

    // if a select is being created with zero options we need to create
    // a special pseudo frame so it can be sized as best it can
    nsCOMPtr<nsIDOMHTMLSelectElement> selectElement;
    nsresult result = aContent->QueryInterface(NS_GET_IID(nsIDOMHTMLSelectElement),
                                                 (void**)getter_AddRefs(selectElement));
    if (NS_SUCCEEDED(result) && selectElement) {
      AddDummyFrameToSelect(aPresContext, aPresShell, aState,
                            scrollFrame, scrolledFrame, &childItems,
                            aContent, selectElement);
    }
    //////////////////////////////////////////////////
    //////////////////////////////////////////////////
    
    // Set the scrolled frame's initial child lists
    scrolledFrame->SetInitialChildList(aPresContext, nsnull, childItems.childList);
    if (isPositionedContainingBlock && aState.mAbsoluteItems.childList) {
      scrolledFrame->SetInitialChildList(aPresContext,
                                         nsLayoutAtoms::absoluteList,
                                         aState.mAbsoluteItems.childList);
    }

    if (aState.mFloatedItems.childList) {
      scrolledFrame->SetInitialChildList(aPresContext,
                                         nsLayoutAtoms::floaterList,
                                         aState.mFloatedItems.childList);
    }

  // Set the scroll frame's initial child list
  scrollFrame->SetInitialChildList(aPresContext, nsnull, scrolledFrame);
                                            
  return NS_OK;
}

/**
 * Used to be InitializeScrollFrame but now its only used for the select tag
 * But the select tag should really be fixed to use GFX scrollbars that can
 * be create with BuildScrollFrame.
 */
nsresult
nsCSSFrameConstructor::ConstructFieldSetFrame(nsIPresShell*            aPresShell, 
                                              nsIPresContext*          aPresContext,
                                              nsFrameConstructorState& aState,
                                              nsIContent*              aContent,
                                              nsIFrame*                aParentFrame,
                                              nsIAtom*                 aTag,
                                              nsIStyleContext*         aStyleContext,
                                              nsIFrame*&               aNewFrame,
                                              PRBool&                  aProcessChildren,
                                              PRBool                   aIsAbsolutelyPositioned,
                                              PRBool&                  aFrameHasBeenInitialized,
                                              PRBool                   aIsFixedPositioned)
{
  nsIFrame * newFrame;
  nsresult rv = NS_NewFieldSetFrame(aPresShell, &newFrame, NS_BLOCK_SPACE_MGR);
  if (NS_FAILED(rv)) {
    return rv;
  }

  nsCOMPtr<nsIPresShell> shell;
  aPresContext->GetShell(getter_AddRefs(shell));

  // Initialize it
  nsIFrame* geometricParent = aParentFrame;
    
  if (aIsAbsolutelyPositioned) {
    geometricParent = aState.mAbsoluteItems.containingBlock;
  } else if (aIsFixedPositioned) {
    geometricParent = aState.mFixedItems.containingBlock;
  }
  
  InitAndRestoreFrame(aPresContext, aState, aContent, 
                      geometricParent, aStyleContext, nsnull, newFrame);

  // See if we need to create a view, e.g. the frame is absolutely
  // positioned
  nsHTMLContainerFrame::CreateViewForFrame(aPresContext, newFrame,
                                           aStyleContext, aParentFrame, PR_FALSE);

    // cache our display type
  const nsStyleDisplay* styleDisplay;
  newFrame->GetStyleData(eStyleStruct_Display, (const nsStyleStruct*&) styleDisplay);

  nsIFrame* areaFrame;
  NS_NewAreaFrame(shell, &areaFrame, NS_BLOCK_SPACE_MGR | NS_BLOCK_SHRINK_WRAP);

  // Resolve style and initialize the frame
  nsIStyleContext* styleContext;
  aPresContext->ResolvePseudoStyleContextFor(aContent, nsHTMLAtoms::fieldsetContentPseudo,
                                             aStyleContext, &styleContext);
  InitAndRestoreFrame(aPresContext, aState, aContent, 
                      newFrame, styleContext, nsnull, areaFrame);

  NS_RELEASE(styleContext);          
  

    // The area frame is a floater container
    PRBool haveFirstLetterStyle, haveFirstLineStyle;
    HaveSpecialBlockStyle(aPresContext, aContent, aStyleContext,
                          &haveFirstLetterStyle, &haveFirstLineStyle);
    nsFrameConstructorSaveState floaterSaveState;
    aState.PushFloaterContainingBlock(areaFrame, floaterSaveState,
                                      haveFirstLetterStyle,
                                      haveFirstLineStyle);

    // Process children
    nsFrameConstructorSaveState absoluteSaveState;
    nsFrameItems                childItems;
    PRBool                      isPositionedContainingBlock = aIsAbsolutelyPositioned ||
                                                              aIsFixedPositioned;

    if (isPositionedContainingBlock) {
      // The area frame becomes a container for child frames that are
      // absolutely positioned
      aState.PushAbsoluteContainingBlock(areaFrame, absoluteSaveState);
    }
     
    ProcessChildren(aPresShell, aPresContext, aState, aContent, areaFrame, PR_FALSE,
                    childItems, PR_TRUE);

    static NS_DEFINE_IID(kLegendFrameCID, NS_LEGEND_FRAME_CID);
    nsIFrame * child      = childItems.childList;
    nsIFrame * previous   = nsnull;
    nsIFrame* legendFrame = nsnull;
    while (nsnull != child) {
      nsresult result = child->QueryInterface(kLegendFrameCID, (void**)&legendFrame);
      if (NS_SUCCEEDED(result) && legendFrame) {
        if (nsnull != previous) {
          nsIFrame * nxt;
          legendFrame->GetNextSibling(&nxt);
          previous->SetNextSibling(nxt);
          areaFrame->SetNextSibling(legendFrame);
          legendFrame->SetParent(newFrame);
          legendFrame->SetNextSibling(nsnull);
          break;
        } else {
          nsIFrame * nxt;
          legendFrame->GetNextSibling(&nxt);
          childItems.childList = nxt;
          areaFrame->SetNextSibling(legendFrame);
          legendFrame->SetParent(newFrame);
          legendFrame->SetNextSibling(nsnull);
          break;
        }
      }
      previous = child;
      child->GetNextSibling(&child);
    }

    // Set the scrolled frame's initial child lists
    areaFrame->SetInitialChildList(aPresContext, nsnull, childItems.childList);
    if (isPositionedContainingBlock && aState.mAbsoluteItems.childList) {
      areaFrame->SetInitialChildList(aPresContext,
                                         nsLayoutAtoms::absoluteList,
                                         aState.mAbsoluteItems.childList);
    }

    if (aState.mFloatedItems.childList) {
      areaFrame->SetInitialChildList(aPresContext,
                                         nsLayoutAtoms::floaterList,
                                         aState.mFloatedItems.childList);
    }

  // Set the scroll frame's initial child list
  newFrame->SetInitialChildList(aPresContext, nsnull, areaFrame);

  // our new frame retured is the top frame which is the list frame. 
  aNewFrame = newFrame; 

  // yes we have already initialized our frame 
  aFrameHasBeenInitialized = PR_TRUE; 

  return NS_OK;
}

nsresult
nsCSSFrameConstructor::ConstructTextFrame(nsIPresShell*            aPresShell, 
                                          nsIPresContext*          aPresContext,
                                          nsFrameConstructorState& aState,
                                          nsIContent*              aContent,
                                          nsIFrame*                aParentFrame,
                                          nsIStyleContext*         aStyleContext,
                                          nsFrameItems&            aFrameItems)
{
  // process pending pseudo frames. whitespace doesn't have an effect.
  if (!aState.mPseudoFrames.IsEmpty() && !IsOnlyWhiteSpace(aContent))
    ProcessPseudoFrames(aPresContext, aState.mPseudoFrames, aFrameItems);

  nsIFrame* newFrame = nsnull;
  nsresult rv = NS_NewTextFrame(aPresShell, &newFrame);
  if (NS_FAILED(rv) || !newFrame)
    return rv;

  // Set the frame state bit for text frames to mark them as replaced.
  // XXX kipp: temporary
  nsFrameState state;
  newFrame->GetFrameState(&state);
  newFrame->SetFrameState(state | NS_FRAME_REPLACED_ELEMENT);

  InitAndRestoreFrame(aPresContext, aState, aContent, aParentFrame,
                      aStyleContext, nsnull, newFrame);

  // We never need to create a view for a text frame.

  // Set the frame's initial child list to null.
  newFrame->SetInitialChildList(aPresContext, nsnull, nsnull);

  // Add the newly constructed frame to the flow
  aFrameItems.AddChild(newFrame);

  // Text frames don't go in the content->frame hash table, because
  // they're anonymous. This keeps the hash table smaller

  return rv;
}

nsresult
nsCSSFrameConstructor::ConstructHTMLFrame(nsIPresShell*            aPresShell, 
                                          nsIPresContext*          aPresContext,
                                          nsFrameConstructorState& aState,
                                          nsIContent*              aContent,
                                          nsIFrame*                aParentFrame,
                                          nsIAtom*                 aTag,
                                          PRInt32                  aNameSpaceID,
                                          nsIStyleContext*         aStyleContext,
                                          nsFrameItems&            aFrameItems)
{
  // Ignore the tag if it's not HTML content
  if (!aContent->IsContentOfType(nsIContent::eHTML)) {
    return NS_OK;
  }

  PRBool    processChildren = PR_FALSE;  // whether we should process child content
  PRBool    isAbsolutelyPositioned = PR_FALSE;
  PRBool    isFixedPositioned = PR_FALSE;
  PRBool    isFloating = PR_FALSE;
  PRBool    isRelativePositioned = PR_FALSE;
  PRBool    canBePositioned = PR_TRUE;
  PRBool    frameHasBeenInitialized = PR_FALSE;
  nsIFrame* newFrame = nsnull;  // the frame we construct
  PRBool    isReplaced = PR_FALSE;
  PRBool    addToHashTable = PR_TRUE;
  PRBool    isFloaterContainer = PR_FALSE;
  PRBool    isPositionedContainingBlock = PR_FALSE;
  nsresult  rv = NS_OK;

  // See if the element is absolute or fixed positioned
  const nsStyleDisplay* display = (const nsStyleDisplay*)
    aStyleContext->GetStyleData(eStyleStruct_Display);
  if (NS_STYLE_POSITION_ABSOLUTE == display->mPosition) {
    isAbsolutelyPositioned = PR_TRUE;
  }
  else if (NS_STYLE_POSITION_FIXED == display->mPosition) {
    isFixedPositioned = PR_TRUE;
  }
  else {
    if (NS_STYLE_FLOAT_NONE != display->mFloats) {
      isFloating = PR_TRUE;
    }
    if (NS_STYLE_POSITION_RELATIVE == display->mPosition) {
      isRelativePositioned = PR_TRUE;
    }
  }

  // Create a frame based on the tag
  if (nsHTMLAtoms::img == aTag) {
    isReplaced = PR_TRUE;
    if (!aState.mPseudoFrames.IsEmpty()) { // process pending pseudo frames
      ProcessPseudoFrames(aPresContext, aState.mPseudoFrames, aFrameItems); 
    }
    // XXX If image display is turned off, then use ConstructAlternateFrame()
    // instead...
    rv = NS_NewImageFrame(aPresShell, &newFrame);
  }
  else if (nsHTMLAtoms::hr == aTag) {
    isReplaced = PR_TRUE;
    if (!aState.mPseudoFrames.IsEmpty()) { // process pending pseudo frames
      ProcessPseudoFrames(aPresContext, aState.mPseudoFrames, aFrameItems); 
    }
    rv = NS_NewHRFrame(aPresShell, &newFrame);
  }
  else if (nsHTMLAtoms::br == aTag) {
    if (!aState.mPseudoFrames.IsEmpty()) { // process pending pseudo frames
      ProcessPseudoFrames(aPresContext, aState.mPseudoFrames, aFrameItems); 
    }
    rv = NS_NewBRFrame(aPresShell, &newFrame);
    isReplaced = PR_TRUE;
    // BR frames don't go in the content->frame hash table: typically
    // there are many BR content objects and this would increase the size
    // of the hash table, and it's doubtful we need the mapping anyway
    addToHashTable = PR_FALSE;
  }
  else if (nsHTMLAtoms::wbr == aTag) {
    if (!aState.mPseudoFrames.IsEmpty()) { // process pending pseudo frames
      ProcessPseudoFrames(aPresContext, aState.mPseudoFrames, aFrameItems); 
    }
    rv = NS_NewWBRFrame(aPresShell, &newFrame);
  }
  else if (nsHTMLAtoms::input == aTag) {
    if (!aState.mPseudoFrames.IsEmpty()) { // process pending pseudo frames
      ProcessPseudoFrames(aPresContext, aState.mPseudoFrames, aFrameItems); 
    }
    isReplaced = PR_TRUE;
    rv = CreateInputFrame(aPresShell, aPresContext,
                          aContent, newFrame, aStyleContext);
  }
  else if (nsHTMLAtoms::textarea == aTag) {
    if (!aState.mPseudoFrames.IsEmpty()) { // process pending pseudo frames
      ProcessPseudoFrames(aPresContext, aState.mPseudoFrames, aFrameItems); 
    }
    isReplaced = PR_TRUE;
    rv = ConstructTextControlFrame(aPresShell, aPresContext, newFrame, aContent);
  }
  else if (nsHTMLAtoms::select == aTag) {
    if (!UseXBLForms()) {
      if (!aState.mPseudoFrames.IsEmpty()) { // process pending pseudo frames
        ProcessPseudoFrames(aPresContext, aState.mPseudoFrames, aFrameItems); 
      }
      isReplaced = PR_TRUE;
      rv = ConstructSelectFrame(aPresShell, aPresContext, aState, aContent, aParentFrame,
                                aTag, aStyleContext, newFrame,  processChildren,
                                isAbsolutelyPositioned, frameHasBeenInitialized,
                                isFixedPositioned, aFrameItems);
    }
  }
  else if (nsHTMLAtoms::object == aTag) {
    if (!aState.mPseudoFrames.IsEmpty()) { // process pending pseudo frames
      ProcessPseudoFrames(aPresContext, aState.mPseudoFrames, aFrameItems); 
    }
    isReplaced = PR_TRUE;
    rv = NS_NewObjectFrame(aPresShell, &newFrame);
    processChildren = PR_FALSE;
  }
  else if (nsHTMLAtoms::applet == aTag) {
    if (!aState.mPseudoFrames.IsEmpty()) { // process pending pseudo frames
      ProcessPseudoFrames(aPresContext, aState.mPseudoFrames, aFrameItems); 
    }
    isReplaced = PR_TRUE;
    rv = NS_NewObjectFrame(aPresShell, &newFrame);
  }
  else if (nsHTMLAtoms::embed == aTag) {
    if (!aState.mPseudoFrames.IsEmpty()) { // process pending pseudo frames
      ProcessPseudoFrames(aPresContext, aState.mPseudoFrames, aFrameItems); 
    }
    isReplaced = PR_TRUE;
    rv = NS_NewObjectFrame(aPresShell, &newFrame);
  }
  else if (nsHTMLAtoms::fieldset == aTag) {
    if (!aState.mPseudoFrames.IsEmpty()) { // process pending pseudo frames
      ProcessPseudoFrames(aPresContext, aState.mPseudoFrames, aFrameItems); 
    }
#define DO_NEWFIELDSET
#ifdef DO_NEWFIELDSET
    rv = ConstructFieldSetFrame(aPresShell, aPresContext, aState, aContent, aParentFrame,
                                aTag, aStyleContext, newFrame,  processChildren,
                                isAbsolutelyPositioned, frameHasBeenInitialized,
                                isFixedPositioned);
    processChildren = PR_FALSE;
#else
    rv = NS_NewFieldSetFrame(aPresShell, &newFrame, isAbsolutelyPositioned ? NS_BLOCK_SPACE_MGR : 0);
    processChildren = PR_TRUE;
#endif
  }
  else if (nsHTMLAtoms::legend == aTag) {
    if (!aState.mPseudoFrames.IsEmpty()) { // process pending pseudo frames
      ProcessPseudoFrames(aPresContext, aState.mPseudoFrames, aFrameItems); 
    }
    rv = NS_NewLegendFrame(aPresShell, &newFrame);
    processChildren = PR_TRUE;
    canBePositioned = PR_FALSE;
  }
  else if (nsHTMLAtoms::frameset == aTag) {
    if (!aState.mPseudoFrames.IsEmpty()) { // process pending pseudo frames
      ProcessPseudoFrames(aPresContext, aState.mPseudoFrames, aFrameItems); 
    }
    
    canBePositioned = PR_FALSE;
    
    PRBool allowSubframes = PR_TRUE;
    if (aPresContext) {
      nsCOMPtr<nsISupports> container;
      aPresContext->GetContainer(getter_AddRefs(container));
      nsCOMPtr<nsIDocShell> docShell(do_QueryInterface(container));
      if (docShell) {
        docShell->GetAllowSubframes(&allowSubframes);
      }
    }
    if (allowSubframes) {
      rv = NS_NewHTMLFramesetFrame(aPresShell, &newFrame);
    }
  }
  else if (nsHTMLAtoms::iframe == aTag) {
    if (!aState.mPseudoFrames.IsEmpty()) { // process pending pseudo frames
      ProcessPseudoFrames(aPresContext, aState.mPseudoFrames, aFrameItems); 
    }
    
    isReplaced = PR_TRUE;
    PRBool allowSubframes = PR_TRUE;
    if (aPresContext) {
      nsCOMPtr<nsISupports> container;
      aPresContext->GetContainer(getter_AddRefs(container));
      nsCOMPtr<nsIDocShell> docShell(do_QueryInterface(container));
      if (docShell) {
        docShell->GetAllowSubframes(&allowSubframes);
      }
    }
    if (allowSubframes) {
      rv = NS_NewHTMLFrameOuterFrame(aPresShell, &newFrame);
      if (newFrame) {
        // the nsFrameOuterFrame needs to know about its content parent during ::Init.
        // there is no reasonable way to get the value there.
        // so we store it as a frame property.
        nsCOMPtr<nsIAtom> contentParentAtom = do_GetAtom("contentParent");
        aState.mFrameManager->SetFrameProperty(newFrame,
                                               contentParentAtom, 
                                               aParentFrame, nsnull);
      }
    }
  }
  else if (nsHTMLAtoms::noframes == aTag) {
    if (!aState.mPseudoFrames.IsEmpty()) { // process pending pseudo frames
      ProcessPseudoFrames(aPresContext, aState.mPseudoFrames, aFrameItems); 
    }
    isReplaced = PR_TRUE;

    PRBool allowSubframes = PR_TRUE;
    if (aPresContext) {
      nsCOMPtr<nsISupports> container;
      aPresContext->GetContainer(getter_AddRefs(container));
      nsCOMPtr<nsIDocShell> docShell(do_QueryInterface(container));
      if (docShell) {
        docShell->GetAllowSubframes(&allowSubframes);
      }
    }
    if (allowSubframes) {
      // make <noframes> be display:none if frames are enabled
      nsStyleDisplay* mutdisplay = (nsStyleDisplay*)aStyleContext->GetUniqueStyleData(aPresContext, eStyleStruct_Display);
      mutdisplay->mDisplay = NS_STYLE_DISPLAY_NONE;
      aState.mFrameManager->SetUndisplayedContent(aContent, aStyleContext);
    } 
    else {
      processChildren = PR_TRUE;
      rv = NS_NewBlockFrame(aPresShell, &newFrame);
    }
  }
  else if (nsHTMLAtoms::spacer == aTag) {
    if (!aState.mPseudoFrames.IsEmpty()) { // process pending pseudo frames
      ProcessPseudoFrames(aPresContext, aState.mPseudoFrames, aFrameItems); 
    }
    rv = NS_NewSpacerFrame(aPresShell, &newFrame);
    canBePositioned = PR_FALSE;
  }
  else if (nsHTMLAtoms::button == aTag) {
    if (!aState.mPseudoFrames.IsEmpty()) { // process pending pseudo frames
      ProcessPseudoFrames(aPresContext, aState.mPseudoFrames, aFrameItems); 
    }
    rv = NS_NewHTMLButtonControlFrame(aPresShell, &newFrame);
    // the html4 button needs to act just like a 
    // regular button except contain html content
    // so it must be replaced or html outside it will
    // draw into its borders. -EDV
    isReplaced = PR_TRUE;
    processChildren = PR_TRUE;
  }
  else if (nsHTMLAtoms::isindex == aTag) {
    if (!aState.mPseudoFrames.IsEmpty()) { // process pending pseudo frames
      ProcessPseudoFrames(aPresContext, aState.mPseudoFrames, aFrameItems);
    }
    isReplaced = PR_TRUE;
    rv = NS_NewIsIndexFrame(aPresShell, &newFrame);
  }

  if (NS_FAILED(rv) || !newFrame)
    return rv;

  // If we succeeded in creating a frame then initialize it, process its
  // children (if requested), and set the initial child list

  // If the frame is a replaced element, then set the frame state bit
  if (isReplaced) {
    nsFrameState  state;
    newFrame->GetFrameState(&state);
    newFrame->SetFrameState(state | NS_FRAME_REPLACED_ELEMENT);
  }

  if (!frameHasBeenInitialized) {
    nsIFrame* geometricParent = aParentFrame;
     
    // Makes sure we use the correct parent frame pointer when initializing
    // the frame
    if (isFloating) {
      geometricParent = aState.mFloatedItems.containingBlock;

    } else if (canBePositioned) {
      if (isAbsolutelyPositioned) {
        geometricParent = aState.mAbsoluteItems.containingBlock;
      } else if (isFixedPositioned) {
        geometricParent = aState.mFixedItems.containingBlock;
      }
    }
    
    InitAndRestoreFrame(aPresContext, aState, aContent, 
                        geometricParent, aStyleContext, nsnull, newFrame);

    // See if we need to create a view, e.g. the frame is absolutely
    // positioned
    nsHTMLContainerFrame::CreateViewForFrame(aPresContext, newFrame,
                                             aStyleContext, aParentFrame, PR_FALSE);

    // Process the child content if requested
    nsFrameItems childItems;
    if (processChildren) {
      if (isPositionedContainingBlock) {
        // The area frame becomes a container for child frames that are
        // absolutely positioned
        nsFrameConstructorSaveState absoluteSaveState;
        aState.PushAbsoluteContainingBlock(newFrame, absoluteSaveState);
        
        // Process the child frames
        rv = ProcessChildren(aPresShell, aPresContext, aState, aContent, newFrame,
                             PR_TRUE, childItems, PR_FALSE);
        
        // Set the frame's absolute list if there were any absolutely positioned children
        if (aState.mAbsoluteItems.childList) {
          newFrame->SetInitialChildList(aPresContext,
                                        nsLayoutAtoms::absoluteList,
                                        aState.mAbsoluteItems.childList);
        }
      }
      else if (isFloaterContainer) {
        // If the frame can contain floaters, then push a floater
        // containing block
        PRBool haveFirstLetterStyle, haveFirstLineStyle;
        HaveSpecialBlockStyle(aPresContext, aContent, aStyleContext,
                              &haveFirstLetterStyle, &haveFirstLineStyle);
        nsFrameConstructorSaveState floaterSaveState;
        aState.PushFloaterContainingBlock(newFrame, floaterSaveState,
                                          PR_FALSE, PR_FALSE);
        
        // Process the child frames
        rv = ProcessChildren(aPresShell, aPresContext, aState, aContent, newFrame,
                             PR_TRUE, childItems, PR_FALSE);
        
        // Set the frame's floater list if there were any floated children
        if (aState.mFloatedItems.childList) {
          newFrame->SetInitialChildList(aPresContext,
                                        nsLayoutAtoms::floaterList,
                                        aState.mFloatedItems.childList);
        }

      } else {
        rv = ProcessChildren(aPresShell, aPresContext, aState, aContent, newFrame,
                             PR_TRUE, childItems, PR_FALSE);
      }
    }

    // if there are any anonymous children create frames for them
    CreateAnonymousFrames(aPresShell, aPresContext, aTag, aState, aContent, newFrame,
                          childItems);


    // Set the frame's initial child list
    newFrame->SetInitialChildList(aPresContext, nsnull, childItems.childList);
  }

  // If the frame is positioned, then create a placeholder frame
  if (canBePositioned && (isAbsolutelyPositioned || isFixedPositioned)) {
    nsIFrame* placeholderFrame;

    CreatePlaceholderFrameFor(aPresShell, aPresContext, aState.mFrameManager, aContent,
                              newFrame, aStyleContext, aParentFrame, &placeholderFrame);

    // Add the positioned frame to its containing block's list of
    // child frames
    if (isAbsolutelyPositioned) {
      aState.mAbsoluteItems.AddChild(newFrame);
    } else {
      aState.mFixedItems.AddChild(newFrame);
    }

    // Add the placeholder frame to the flow
    aFrameItems.AddChild(placeholderFrame);

  } else if (isFloating) {
    nsIFrame* placeholderFrame;
    CreatePlaceholderFrameFor(aPresShell, aPresContext, aState.mFrameManager, aContent, newFrame,
                              aStyleContext, aParentFrame, &placeholderFrame);

    // Add the floating frame to its containing block's list of child frames
    aState.mFloatedItems.AddChild(newFrame);

    // Add the placeholder frame to the flow
    aFrameItems.AddChild(placeholderFrame);

  } else {
    // Handle :before/:after for leaf elements.  For now (XXX bug), just
    // don't create any generated content if the element in question is
    // floating or positioned, since that's *hard* (TM).  It requires the
    // display data in the style context go to the wrapper and the rest go
    // to the frame inside.
    // See also bug 141289.
    if (!processChildren) {
      nsIFrame *wrapperFrame = nsnull, *beforeFrame, *afterFrame;
      if (!CreateGeneratedContentFrame(aPresShell, aPresContext,
                                       aState, aParentFrame, aContent,
                                       aStyleContext, nsCSSAtoms::beforePseudo,
                                       &wrapperFrame, &beforeFrame)) {
        beforeFrame = nsnull;
      }
      if (!CreateGeneratedContentFrame(aPresShell, aPresContext,
                                      aState, aParentFrame, aContent,
                                      aStyleContext, nsCSSAtoms::afterPseudo,
                                      &wrapperFrame, &afterFrame)) {
        afterFrame = nsnull;
      }
      NS_ASSERTION(!(beforeFrame || afterFrame) == !wrapperFrame,
                   "should get wrapper iff GC");
      if (wrapperFrame) {
        // OK, we have generated content for a leaf.
        // This means |CreateGeneratedContentFrame| created a wrapper
        // frame to hold the generated content and the leaf frame.  We now
        // need to link everything together.

        NS_ASSERTION(!HasNextSibling(newFrame),
                     "must not have sibling");
        NS_ASSERTION(!beforeFrame || !HasNextSibling(beforeFrame),
                     "must not have sibling");
        NS_ASSERTION(!afterFrame || !HasNextSibling(afterFrame),
                     "must not have sibling");

        nsIFrame *firstChild = newFrame;
        if (beforeFrame) {
          beforeFrame->SetNextSibling(newFrame);
          firstChild = beforeFrame;
        }
        newFrame->SetNextSibling(afterFrame);

        newFrame->SetParent(wrapperFrame);
        wrapperFrame->SetInitialChildList(aPresContext, nsnull, firstChild);

        // From now on, pretend the wrapper frame is the real frame.
        newFrame = wrapperFrame;
      }
    }

    // Add the newly constructed frame to the flow
    aFrameItems.AddChild(newFrame);
  }

  if (addToHashTable) {
    // Add a mapping from content object to primary frame. Note that for
    // floated and positioned frames this is the out-of-flow frame and not
    // the placeholder frame
    aState.mFrameManager->SetPrimaryFrameFor(aContent, newFrame);
  }

  return rv;
}

// after the node has been constructed and initialized create any
// anonymous content a node needs.
static void
LocateAnonymousFrame(nsIPresContext* aPresContext,
                     nsIFrame*       aParentFrame,
                     nsIContent*     aTargetContent,
                     nsIFrame**      aResult)
{
  // We may be a placeholder.  If we are, go to the real frame.
  aParentFrame = GetRealFrame(aParentFrame);

  // Check ourselves.
  *aResult = nsnull;
  nsCOMPtr<nsIContent> content;
  aParentFrame->GetContent(getter_AddRefs(content));
  if (content.get() == aTargetContent) {
    // We must take into account if the parent is a scrollframe. If it is, we
    // need to bypass the scrolling mechanics and get at the true frame.
    nsCOMPtr<nsIScrollableFrame> scrollFrame ( do_QueryInterface(aParentFrame) );
    if ( scrollFrame )
      scrollFrame->GetScrolledFrame ( aPresContext, *aResult );
    else
      *aResult = aParentFrame;
    return;
  }

  // Check our kids.
  nsIFrame* currFrame;
  aParentFrame->FirstChild(aPresContext, nsnull, &currFrame);
  while (currFrame) {
    LocateAnonymousFrame(aPresContext, currFrame, aTargetContent, aResult);
    if (*aResult)
      return;
    currFrame->GetNextSibling(&currFrame);
  }

  nsCOMPtr<nsIMenuFrame> menuFrame(do_QueryInterface(aParentFrame));
  if (menuFrame) {
    nsIFrame* popupChild;
    menuFrame->GetMenuChild(&popupChild);
    if (popupChild) {
      LocateAnonymousFrame(aPresContext, popupChild, aTargetContent, aResult);
      if (*aResult)
        return;
    }
  }
}

nsresult
nsCSSFrameConstructor::CreateAnonymousFrames(nsIPresShell*            aPresShell, 
                                             nsIPresContext*          aPresContext,
                                             nsIAtom*                 aTag,
                                             nsFrameConstructorState& aState,
                                             nsIContent*              aParent,
                                             nsIFrame*                aNewFrame,
                                             nsFrameItems&            aChildItems,
                                             PRBool                   aIsRoot)
{
  // See if we might have anonymous content
  // by looking at the tag rather than doing a QueryInterface on
  // the frame.  Only these tags' frames can have anonymous content
  // through nsIAnonymousContentCreator.  We do this check for
  // performance reasons. If we did a QueryInterface on every tag it
  // would be inefficient.

  // nsGenericElement::SetDocument ought to keep a list like this one,
  // but it can't because nsGfxScrollFrames get around this.
  if (!aIsRoot &&
      aTag != nsHTMLAtoms::input &&
      aTag != nsHTMLAtoms::textarea &&
      aTag != nsHTMLAtoms::combobox &&
      aTag != nsHTMLAtoms::isindex &&
#ifdef INCLUDE_XUL
      aTag != nsXULAtoms::scrollbar
#endif
      )
    return NS_OK;

  return CreateAnonymousFrames(aPresShell, aPresContext, aState, aParent, mDocument, aNewFrame, aChildItems);
}

// after the node has been constructed and initialized create any
// anonymous content a node needs.
nsresult
nsCSSFrameConstructor::CreateAnonymousFrames(nsIPresShell*            aPresShell, 
                                             nsIPresContext*          aPresContext,
                                             nsFrameConstructorState& aState,
                                             nsIContent*              aParent,
                                             nsIDocument*             aDocument,
                                             nsIFrame*                aParentFrame,
                                             nsFrameItems&            aChildItems)
{
  nsCOMPtr<nsIAnonymousContentCreator> creator(do_QueryInterface(aParentFrame));
  if (!creator)
    return NS_OK;

  nsCOMPtr<nsISupportsArray> anonymousItems;
  NS_NewISupportsArray(getter_AddRefs(anonymousItems));

  creator->CreateAnonymousContent(aPresContext, *anonymousItems);
  
  PRUint32 count = 0;
  anonymousItems->Count(&count);

  if (count) {
    // Inform the pres shell about the anonymous content
    aPresShell->SetAnonymousContentFor(aParent, anonymousItems);

    for (PRUint32 i=0; i < count; i++) {
      // get our child's content and set its parent to our content
      nsCOMPtr<nsIContent> content;
      if (NS_FAILED(anonymousItems->QueryElementAt(i, NS_GET_IID(nsIContent), getter_AddRefs(content))))
        continue;

      content->SetNativeAnonymous(PR_TRUE);
      content->SetParent(aParent);
      content->SetDocument(aDocument, PR_TRUE, PR_TRUE);

#ifdef INCLUDE_XUL
      // Only cut XUL scrollbars off if they're not in a XUL document.  This allows
      // scrollbars to be styled from XUL (although not from XML or HTML).
      nsCOMPtr<nsIAtom> tag;
      content->GetTag(*getter_AddRefs(tag));
      if (tag.get() == nsXULAtoms::scrollbar) {
        nsCOMPtr<nsIDOMXULDocument> xulDoc(do_QueryInterface(aDocument));
        if (xulDoc)
          content->SetBindingParent(aParent);
        else content->SetBindingParent(content);
      }
      else
#endif
        content->SetBindingParent(content);
    
      nsIFrame * newFrame = nsnull;
      nsresult rv = creator->CreateFrameFor(aPresContext, content, &newFrame);
      if (NS_SUCCEEDED(rv) && newFrame != nsnull) {
        aChildItems.AddChild(newFrame);
      }
      else {
        // create the frame and attach it to our frame
        ConstructFrame(aPresShell, aPresContext, aState, content, aParentFrame, aChildItems);
      }
    }
  }

  return NS_OK;
}

#ifdef INCLUDE_XUL
static
PRBool IsXULDisplayType(const nsStyleDisplay* aDisplay)
{
  return (aDisplay->mDisplay == NS_STYLE_DISPLAY_INLINE_BOX || 
          aDisplay->mDisplay == NS_STYLE_DISPLAY_INLINE_GRID || 
          aDisplay->mDisplay == NS_STYLE_DISPLAY_INLINE_STACK ||
          aDisplay->mDisplay == NS_STYLE_DISPLAY_BOX ||
          aDisplay->mDisplay == NS_STYLE_DISPLAY_GRID ||
          aDisplay->mDisplay == NS_STYLE_DISPLAY_STACK ||
          aDisplay->mDisplay == NS_STYLE_DISPLAY_GRID_GROUP ||
          aDisplay->mDisplay == NS_STYLE_DISPLAY_GRID_LINE ||
          aDisplay->mDisplay == NS_STYLE_DISPLAY_DECK ||
          aDisplay->mDisplay == NS_STYLE_DISPLAY_POPUP ||
          aDisplay->mDisplay == NS_STYLE_DISPLAY_GROUPBOX
          );
}

nsresult
nsCSSFrameConstructor::ConstructXULFrame(nsIPresShell*            aPresShell, 
                                         nsIPresContext*          aPresContext,
                                         nsFrameConstructorState& aState,
                                         nsIContent*              aContent,
                                         nsIFrame*                aParentFrame,
                                         nsIAtom*                 aTag,
                                         PRInt32                  aNameSpaceID,
                                         nsIStyleContext*         aStyleContext,
                                         nsFrameItems&            aFrameItems,
                                         PRBool                   aXBLBaseTag,
                                         PRBool&                  aHaltProcessing)
{
  PRBool    primaryFrameSet = PR_FALSE;
  PRBool    processChildren = PR_FALSE;  // whether we should process child content
  PRBool    processAnonymousChildren = PR_FALSE; // whether or not we process anonymous content.
  nsresult  rv = NS_OK;
  PRBool    isAbsolutelyPositioned = PR_FALSE;
  PRBool    isFixedPositioned = PR_FALSE;
  PRBool    isPopup = PR_FALSE;
  PRBool    isReplaced = PR_FALSE;
  PRBool    frameHasBeenInitialized = PR_FALSE;

  // this is the new frame that will be created
  nsIFrame* newFrame = nsnull;
  
  // this is the also the new frame that is created. But if a scroll frame is needed
  // the content will be mapped to the scrollframe and topFrame will point to it.
  // newFrame will still point to the child that we created like a "div" for example.
  nsIFrame* topFrame = nsnull;

  NS_ASSERTION(aTag != nsnull, "null XUL tag");
  if (aTag == nsnull)
    return NS_OK;

  const nsStyleDisplay* display = (const nsStyleDisplay*)
           aStyleContext->GetStyleData(eStyleStruct_Display);
  
  PRBool isXULNS = (aNameSpaceID == nsXULAtoms::nameSpaceID);
  PRBool isXULDisplay = IsXULDisplayType(display);

  if (isXULNS || isXULDisplay) {
    // See if the element is absolutely positioned
    if (NS_STYLE_POSITION_ABSOLUTE == display->mPosition)
      isAbsolutelyPositioned = PR_TRUE;

    if (isXULNS) {
      // First try creating a frame based on the tag
      // BUTTON CONSTRUCTION
      if (aTag == nsXULAtoms::button || aTag == nsXULAtoms::checkbox || aTag == nsXULAtoms::radio) {
        processChildren = PR_TRUE;
        isReplaced = PR_TRUE;
        rv = NS_NewButtonBoxFrame(aPresShell, &newFrame);

        // Boxes can scroll.
        if (IsScrollable(aPresContext, display)) {

          // set the top to be the newly created scrollframe
          BuildScrollFrame(aPresShell, aPresContext, aState, aContent, aStyleContext, newFrame, aParentFrame,
                           topFrame, aStyleContext);

          // we have a scrollframe so the parent becomes the scroll frame.
          newFrame->GetParent(&aParentFrame);

          primaryFrameSet = PR_TRUE;

          frameHasBeenInitialized = PR_TRUE;

        } 
      } // End of BUTTON CONSTRUCTION logic
      // AUTOREPEATBUTTON CONSTRUCTION
      else if (aTag == nsXULAtoms::autorepeatbutton) {
        processChildren = PR_TRUE;
        isReplaced = PR_TRUE;
        rv = NS_NewAutoRepeatBoxFrame(aPresShell, &newFrame);

        // Boxes can scroll.
        if (IsScrollable(aPresContext, display)) {

          // set the top to be the newly created scrollframe
          BuildScrollFrame(aPresShell, aPresContext, aState, aContent, aStyleContext, newFrame, aParentFrame,
                           topFrame, aStyleContext);

          // we have a scrollframe so the parent becomes the scroll frame.
          newFrame->GetParent(&aParentFrame);

          primaryFrameSet = PR_TRUE;

          frameHasBeenInitialized = PR_TRUE;

        } 
      } // End of AUTOREPEATBUTTON CONSTRUCTION logic


	   // TITLEBAR CONSTRUCTION
	   else if (aTag == nsXULAtoms::titlebar) {
        processChildren = PR_TRUE;
        isReplaced = PR_TRUE;
        rv = NS_NewTitleBarFrame(aPresShell, &newFrame);

		  // Boxes can scroll.
        if (IsScrollable(aPresContext, display)) {

          // set the top to be the newly created scrollframe
          BuildScrollFrame(aPresShell, aPresContext, aState, aContent, aStyleContext, newFrame, aParentFrame,
                           topFrame, aStyleContext);

          // we have a scrollframe so the parent becomes the scroll frame.
          newFrame->GetParent(&aParentFrame);

          primaryFrameSet = PR_TRUE;

          frameHasBeenInitialized = PR_TRUE;

        } 
      } // End of TITLEBAR CONSTRUCTION logic

	   // RESIZER CONSTRUCTION
	   else if (aTag == nsXULAtoms::resizer) {
        processChildren = PR_TRUE;
        isReplaced = PR_TRUE;
        rv = NS_NewResizerFrame(aPresShell, &newFrame);

        // Boxes can scroll.
        if (IsScrollable(aPresContext, display)) {

          // set the top to be the newly created scrollframe
          BuildScrollFrame(aPresShell, aPresContext, aState, aContent, aStyleContext, newFrame, aParentFrame,
                           topFrame, aStyleContext);

          // we have a scrollframe so the parent becomes the scroll frame.
          newFrame->GetParent(&aParentFrame);

          primaryFrameSet = PR_TRUE;

          frameHasBeenInitialized = PR_TRUE;

        } 
      } // End of RESIZER CONSTRUCTION logic

      else if (aTag == nsXULAtoms::image) {
        isReplaced = PR_TRUE;
        rv = NS_NewImageBoxFrame(aPresShell, &newFrame);
      }
      else if (aTag == nsXULAtoms::spring ||
               aTag == nsHTMLAtoms::spacer) {
        isReplaced = PR_TRUE;
        rv = NS_NewSpringFrame(aPresShell, &newFrame);
      }
       else if (aTag == nsXULAtoms::treechildren) {
        isReplaced = PR_TRUE;
        rv = NS_NewTreeBodyFrame(aPresShell, &newFrame);
      }
      else if (aTag == nsXULAtoms::treecol) {
        isReplaced = PR_TRUE;
        processChildren = PR_TRUE;
        rv = NS_NewTreeColFrame(aPresShell, &newFrame);
      }
      // TEXT CONSTRUCTION
      else if (aTag == nsXULAtoms::text || aTag == nsHTMLAtoms::label ||
               aTag == nsXULAtoms::description) {
        if ((aTag == nsHTMLAtoms::label || aTag == nsXULAtoms::description) && 
            (! aContent->HasAttr(kNameSpaceID_None, nsHTMLAtoms::value))) {
          processChildren = PR_TRUE;
          rv = NS_NewAreaFrame(aPresShell, &newFrame,
                               NS_BLOCK_SPACE_MGR | NS_BLOCK_SHRINK_WRAP | NS_BLOCK_MARGIN_ROOT);
        }
        else {
          isReplaced = PR_TRUE;
          rv = NS_NewTextBoxFrame(aPresShell, &newFrame);
        }
      }
      // End of TEXT CONSTRUCTION logic

       // Menu Construction    
      else if (aTag == nsXULAtoms::menu ||
               aTag == nsXULAtoms::menuitem || 
               aTag == nsXULAtoms::menubutton) {
        // A derived class box frame
        // that has custom reflow to prevent menu children
        // from becoming part of the flow.
        processChildren = PR_TRUE; // Will need this to be custom.
        isReplaced = PR_TRUE;
        rv = NS_NewMenuFrame(aPresShell, &newFrame, (aTag != nsXULAtoms::menuitem));
        ((nsMenuFrame*) newFrame)->SetFrameConstructor(this);
      }
      else if (aTag == nsXULAtoms::menubar) {
  #if defined(XP_MAC) || defined(XP_MACOSX) // The Mac uses its native menu bar.
        aHaltProcessing = PR_TRUE;
        return NS_OK;
  #else
        processChildren = PR_TRUE;
        rv = NS_NewMenuBarFrame(aPresShell, &newFrame);
  #endif
      }
      else if (aTag == nsXULAtoms::popupgroup) {
        // This frame contains child popups
        processChildren = PR_TRUE;
        isReplaced = PR_TRUE;
        rv = NS_NewPopupSetFrame(aPresShell, &newFrame);
        ((nsPopupSetFrame*) newFrame)->SetFrameConstructor(this);
      }
      else if (aTag == nsXULAtoms::scrollbox) {
            rv = NS_NewScrollBoxFrame(aPresShell, &newFrame);
            processChildren = PR_TRUE;
            isReplaced = PR_TRUE;
      } 
      else if (aTag == nsXULAtoms::iframe || aTag == nsXULAtoms::editor ||
               aTag == nsXULAtoms::browser) {
        isReplaced = PR_TRUE;

        // XXX should turning off frames allow XUL iframes?
        PRBool allowSubframes = PR_TRUE;
        if (aPresContext) {
          nsCOMPtr<nsISupports> container;
          aPresContext->GetContainer(getter_AddRefs(container));
          nsCOMPtr<nsIDocShell> docShell(do_QueryInterface(container));
          if (docShell) {
            docShell->GetAllowSubframes(&allowSubframes);
          }
        }
        if (allowSubframes) {
           rv = NS_NewHTMLFrameOuterFrame(aPresShell, &newFrame);
        }
      }
      // PROGRESS METER CONSTRUCTION
      else if (aTag == nsXULAtoms::progressmeter) {
        processChildren = PR_TRUE;
        isReplaced = PR_TRUE;
        rv = NS_NewProgressMeterFrame(aPresShell, &newFrame);
      }
      // End of PROGRESS METER CONSTRUCTION logic
      // SLIDER CONSTRUCTION
      else if (aTag == nsXULAtoms::slider) {
        processChildren = PR_TRUE;
        isReplaced = PR_TRUE;
        rv = NS_NewSliderFrame(aPresShell, &newFrame);
      }
      // End of SLIDER CONSTRUCTION logic

      // SCROLLBAR CONSTRUCTION
      else if (aTag == nsXULAtoms::scrollbar) {
        processChildren = PR_TRUE;
        isReplaced = PR_TRUE;
        rv = NS_NewScrollbarFrame(aPresShell, &newFrame);
      }
      else if (aTag == nsXULAtoms::nativescrollbar) {
        processChildren = PR_TRUE;
        isReplaced = PR_TRUE;
        rv = NS_NewNativeScrollbarFrame(aPresShell, &newFrame);
      }
      // End of SCROLLBAR CONSTRUCTION logic

      // SCROLLBUTTON CONSTRUCTION
      else if (aTag == nsXULAtoms::scrollbarbutton) {
        processChildren = PR_TRUE;
        isReplaced = PR_TRUE;
        rv = NS_NewScrollbarButtonFrame(aPresShell, &newFrame);
      }
      // End of SCROLLBUTTON CONSTRUCTION logic

      // SPLITTER CONSTRUCTION
      else if (aTag == nsXULAtoms::splitter) {
        processChildren = PR_TRUE;
        isReplaced = PR_TRUE;
        rv = NS_NewSplitterFrame(aPresShell, &newFrame);
      }
      // End of SPLITTER CONSTRUCTION logic

      // GRIPPY CONSTRUCTION
      else if (aTag == nsXULAtoms::grippy) {
        processChildren = PR_TRUE;
        isReplaced = PR_TRUE;
        rv = NS_NewGrippyFrame(aPresShell, &newFrame);
      }
      // End of GRIPPY CONSTRUCTION logic
    }

    // Display types for XUL start here
    // First is BOX
    if (!newFrame && isXULDisplay) {
      if (display->mDisplay == NS_STYLE_DISPLAY_INLINE_BOX ||
               display->mDisplay == NS_STYLE_DISPLAY_BOX) {
        processChildren = PR_TRUE;
        isReplaced = PR_TRUE;

        rv = NS_NewBoxFrame(aPresShell, &newFrame, PR_FALSE, nsnull);

        // Boxes can scroll.
        if (IsScrollable(aPresContext, display)) {

          // set the top to be the newly created scrollframe
          BuildScrollFrame(aPresShell, aPresContext, aState, aContent, aStyleContext, newFrame, aParentFrame,
                           topFrame, aStyleContext);

          // we have a scrollframe so the parent becomes the scroll frame.
          newFrame->GetParent(&aParentFrame);

          primaryFrameSet = PR_TRUE;

          frameHasBeenInitialized = PR_TRUE;

        } 
      } // End of BOX CONSTRUCTION logic

      // ------- Begin Grid ---------
      else if ((!aXBLBaseTag && (display->mDisplay == NS_STYLE_DISPLAY_INLINE_GRID ||
                                 display->mDisplay == NS_STYLE_DISPLAY_GRID)) ||
                aTag == nsXULAtoms::grid) {
        processChildren = PR_TRUE;
        isReplaced = PR_TRUE;
        nsCOMPtr<nsIBoxLayout> layout;
        NS_NewGridLayout2(aPresShell, getter_AddRefs(layout));
        rv = NS_NewBoxFrame(aPresShell, &newFrame, PR_FALSE, layout);

        // Boxes can scroll.
        if (IsScrollable(aPresContext, display)) {

          // set the top to be the newly created scrollframe
          BuildScrollFrame(aPresShell, aPresContext, aState, aContent, aStyleContext, newFrame, aParentFrame,
                           topFrame, aStyleContext);

          // we have a scrollframe so the parent becomes the scroll frame.
          newFrame->GetParent(&aParentFrame);

          primaryFrameSet = PR_TRUE;

          frameHasBeenInitialized = PR_TRUE;

        } 
      } //------- End Grid ------

      // ------- Begin Rows/Columns ---------
      else if (display->mDisplay == NS_STYLE_DISPLAY_GRID_GROUP) {
        processChildren = PR_TRUE;
        isReplaced = PR_TRUE;

        nsCOMPtr<nsIBoxLayout> layout;
      
        PRBool listboxScrollPort = PR_FALSE;
        
        if (aTag == nsXULAtoms::listboxbody) {
          NS_NewListBoxLayout(aPresShell, layout);

          rv = NS_NewListBoxBodyFrame(aPresShell, &newFrame, PR_FALSE, layout);
          ((nsListBoxBodyFrame*)newFrame)->InitGroup(this, aPresContext);
          listboxScrollPort = PR_TRUE;

          processChildren = PR_FALSE;
        }
        else
        {
          NS_NewGridRowGroupLayout(aPresShell, getter_AddRefs(layout));
          rv = NS_NewGridRowGroupFrame(aPresShell, &newFrame, PR_FALSE, layout);
        }

        // Boxes can scroll.
        if (IsScrollable(aPresContext, display)) {

          nsIFrame* scrollPort = nsnull;
          if (listboxScrollPort) {
            NS_NewListBoxScrollPortFrame(aPresShell, &scrollPort);
          }
          // set the top to be the newly created scrollframe
          BuildScrollFrame(aPresShell, aPresContext, aState, aContent, aStyleContext, newFrame, aParentFrame,
                           topFrame, aStyleContext, scrollPort);

          // we have a scrollframe so the parent becomes the scroll frame.
          newFrame->GetParent(&aParentFrame);

          primaryFrameSet = PR_TRUE;

          frameHasBeenInitialized = PR_TRUE;

        } 
      } //------- End Grid ------

      // ------- Begin Row/Column ---------
      else if (display->mDisplay == NS_STYLE_DISPLAY_GRID_LINE) {
        processChildren = PR_TRUE;
        isReplaced = PR_TRUE;
      
        nsCOMPtr<nsIBoxLayout> layout;


        NS_NewGridRowLeafLayout(aPresShell, getter_AddRefs(layout));

        if (aTag == nsXULAtoms::listitem)
          rv = NS_NewListItemFrame(aPresShell, &newFrame, PR_FALSE, layout);
        else
          rv = NS_NewGridRowLeafFrame(aPresShell, &newFrame, PR_FALSE, layout);

        // Boxes can scroll.
        if (IsScrollable(aPresContext, display)) {

          // set the top to be the newly created scrollframe
          BuildScrollFrame(aPresShell, aPresContext, aState, aContent, aStyleContext, newFrame, aParentFrame,
                           topFrame, aStyleContext);

          // we have a scrollframe so the parent becomes the scroll frame.
          newFrame->GetParent(&aParentFrame);

          primaryFrameSet = PR_TRUE;

          frameHasBeenInitialized = PR_TRUE;

        } 
      } //------- End Grid ------
      // End of STACK CONSTRUCTION logic
       // DECK CONSTRUCTION
      else if (display->mDisplay == NS_STYLE_DISPLAY_DECK) {
        processChildren = PR_TRUE;
        isReplaced = PR_TRUE;
        rv = NS_NewDeckFrame(aPresShell, &newFrame);
      }
      // End of DECK CONSTRUCTION logic
      else if (display->mDisplay == NS_STYLE_DISPLAY_GROUPBOX) {
        rv = NS_NewGroupBoxFrame(aPresShell, &newFrame);
        processChildren = PR_TRUE;
        isReplaced = PR_TRUE;

        // Boxes can scroll.
        if (IsScrollable(aPresContext, display)) {

          // set the top to be the newly created scrollframe
          BuildScrollFrame(aPresShell, aPresContext, aState, aContent, aStyleContext, newFrame, aParentFrame,
                           topFrame, aStyleContext);

          // we have a scrollframe so the parent becomes the scroll frame.
          newFrame->GetParent(&aParentFrame);

          primaryFrameSet = PR_TRUE;

          frameHasBeenInitialized = PR_TRUE;
        }
      } 
      // STACK CONSTRUCTION
      else if (display->mDisplay == NS_STYLE_DISPLAY_STACK ||
               display->mDisplay == NS_STYLE_DISPLAY_INLINE_STACK) {
        processChildren = PR_TRUE;
        isReplaced = PR_TRUE;


        rv = NS_NewStackFrame(aPresShell, &newFrame);

         if (IsScrollable(aPresContext, display)) {

          // set the top to be the newly created scrollframe
          BuildScrollFrame(aPresShell, aPresContext, aState, aContent, aStyleContext, newFrame, aParentFrame,
                           topFrame, aStyleContext);

          // we have a scrollframe so the parent becomes the scroll frame.
          newFrame->GetParent(&aParentFrame);
          primaryFrameSet = PR_TRUE;

          frameHasBeenInitialized = PR_TRUE;

        }
      }
      else if (display->mDisplay == NS_STYLE_DISPLAY_POPUP) {
        // This is its own frame that derives from
        // box.
        processChildren = PR_TRUE;
        isReplaced = PR_TRUE;
        rv = NS_NewMenuPopupFrame(aPresShell, &newFrame);

        if (aTag == nsXULAtoms::tooltip) {
          nsAutoString defaultTooltip;
          aContent->GetAttr(kNameSpaceID_None, nsXULAtoms::defaultz, defaultTooltip);
          if (defaultTooltip.EqualsIgnoreCase("true")) {
            // Locate the root frame and tell it about the tooltip.
            nsIFrame* rootFrame = nsnull;
            aState.mFrameManager->GetRootFrame(&rootFrame);
            if (rootFrame)
              rootFrame->FirstChild(aPresContext, nsnull, &rootFrame);   
            nsCOMPtr<nsIRootBox> rootBox(do_QueryInterface(rootFrame));
            if (rootBox)
              rootBox->SetDefaultTooltip(aContent);
          }
        }

        // If a popup is inside a menu, then the menu understands the complex
        // rules/behavior governing the cascade of multiple menu popups and can handle
        // having the real popup frame placed under it as a child.  
        // If, however, the parent is *not* a menu frame, then we need to create
        // a placeholder frame for the popup, and then we add the popup frame to the
        // root popup set (that manages all such "detached" popups).
        nsCOMPtr<nsIMenuFrame> menuFrame(do_QueryInterface(aParentFrame));
        if (!menuFrame)
          isPopup = PR_TRUE;
      } 
    }
  }

  // If we succeeded in creating a frame then initialize it, process its
  // children (if requested), and set the initial child list
  if (NS_SUCCEEDED(rv) && newFrame != nsnull) {

    // if no top frame was created then the top is the new frame
    if (topFrame == nsnull)
        topFrame = newFrame;

    // If the frame is a replaced element, then set the frame state bit
    if (isReplaced) {
      nsFrameState  state;
      newFrame->GetFrameState(&state);
      newFrame->SetFrameState(state | NS_FRAME_REPLACED_ELEMENT);
    }

    // xul does not support absolute positioning
    nsIFrame* geometricParent = aParentFrame;

    /*
    nsIFrame* geometricParent = isAbsolutelyPositioned
        ? aState.mAbsoluteItems.containingBlock
        : aParentFrame;
    */
    // if the new frame was already initialized to initialize it again.
    if (!frameHasBeenInitialized) {

      InitAndRestoreFrame(aPresContext, aState, aContent, 
                      geometricParent, aStyleContext, nsnull, newFrame);

      
      /*
      // if our parent is a block frame then do things the way html likes it
      // if not then we are in a box so do what boxes like. On example is boxes
      // do not support the absolute positioning of their children. While html blocks
      // thats why we call different things here.
      nsCOMPtr<nsIAtom> frameType;
      geometricParent->GetFrameType(getter_AddRefs(frameType));
      if ((frameType.get() == nsLayoutAtoms::blockFrame) ||
          (frameType.get() == nsLayoutAtoms::areaFrame)) {
      */
        // See if we need to create a view, e.g. the frame is absolutely positioned
        nsHTMLContainerFrame::CreateViewForFrame(aPresContext, newFrame,
                                                 aStyleContext, aParentFrame, PR_FALSE);

      /*
      } else {
          // we are in a box so do the box thing.
        nsBoxFrame::CreateViewForFrame(aPresContext, newFrame,
                                                 aStyleContext, PR_FALSE);
      }
      */
      
    }

    // Process the child content if requested
    if (processChildren || processAnonymousChildren) {
      nsFrameItems childItems;
      if (processChildren) {
        nsCOMPtr<nsIBindingManager> bindingManager;
        mDocument->GetBindingManager(getter_AddRefs(bindingManager));
        bindingManager->ShouldBuildChildFrames(aContent, &processChildren);
        if (processChildren)
          rv = ProcessChildren(aPresShell, aPresContext, aState, aContent,
                               newFrame, PR_FALSE, childItems, PR_FALSE);
      }
      
      CreateAnonymousFrames(aPresShell, aPresContext, aTag, aState, aContent,
                            newFrame, childItems);

      // Set the frame's initial child list
      newFrame->SetInitialChildList(aPresContext, nsnull, childItems.childList);
    }

    // If the frame is absolutely positioned, then create a placeholder frame
    if (isAbsolutelyPositioned || isFixedPositioned || isPopup) {
      nsIFrame* placeholderFrame;

      CreatePlaceholderFrameFor(aPresShell, aPresContext, aState.mFrameManager, aContent,
                                newFrame, aStyleContext, aParentFrame, &placeholderFrame);

      // Add the positioned frame to its containing block's list of child frames
      if (isAbsolutelyPositioned) {
        aState.mAbsoluteItems.AddChild(newFrame);
      } else if (isFixedPositioned) {
        aState.mFixedItems.AddChild(newFrame);
      } else if (isPopup) {
        // Locate the root popup set and add ourselves to the popup set's list
        // of popup frames.
        nsIFrame* rootFrame;
        aState.mFrameManager->GetRootFrame(&rootFrame);
        if (rootFrame)
          rootFrame->FirstChild(aPresContext, nsnull, &rootFrame);   
        nsCOMPtr<nsIRootBox> rootBox(do_QueryInterface(rootFrame));
        NS_ASSERTION(rootBox, "unexpected null pointer");
        if (rootBox) {
          nsIFrame* popupSetFrame;
          rootBox->GetPopupSetFrame(&popupSetFrame);
          NS_ASSERTION(popupSetFrame, "unexpected null pointer");
          if (popupSetFrame) {
            nsCOMPtr<nsIPopupSetFrame> popupSet(do_QueryInterface(popupSetFrame));
            NS_ASSERTION(popupSet, "unexpected null pointer");
            if (popupSet)
              popupSet->AddPopupFrame(newFrame);
          }
        }
      }

      // Add the placeholder frame to the flow
      aFrameItems.AddChild(placeholderFrame);
    }
    else
      // Add the new frame to our list of frame items.
      aFrameItems.AddChild(topFrame);
  }


  // register tooltip support if needed
  nsAutoString value;
  if (aTag == nsXULAtoms::treechildren || // trees always need titletips
      aContent->GetAttr(kNameSpaceID_None, nsXULAtoms::tooltiptext, value) !=
        NS_CONTENT_ATTR_NOT_THERE ||
      aContent->GetAttr(kNameSpaceID_None, nsXULAtoms::tooltip, value) !=
        NS_CONTENT_ATTR_NOT_THERE)
  {
    nsIFrame* rootFrame = nsnull;
    aState.mFrameManager->GetRootFrame(&rootFrame);
    if (rootFrame)
      rootFrame->FirstChild(aPresContext, nsnull, &rootFrame);   
    nsCOMPtr<nsIRootBox> rootBox(do_QueryInterface(rootFrame));
    if (rootBox)
      rootBox->AddTooltipSupport(aContent);
  }

// addToHashTable:

  if (topFrame) {
    // the top frame is always what we map the content to. This is the frame that contains a pointer
    // to the content node.

    // Add a mapping from content object to primary frame. Note that for
    // floated and positioned frames this is the out-of-flow frame and not
    // the placeholder frame
    if (!primaryFrameSet)
        aState.mFrameManager->SetPrimaryFrameFor(aContent, topFrame);
  }

  return rv;
}
#endif

nsresult
nsCSSFrameConstructor::BeginBuildingScrollFrame(nsIPresShell* aPresShell, 
                                                nsIPresContext*         aPresContext,
                                               nsFrameConstructorState& aState,
                                               nsIContent*              aContent,
                                               nsIStyleContext*         aContentStyle,
                                               nsIFrame*                aParentFrame,
                                               nsIAtom*                 aScrolledPseudo,
                                               nsIDocument*             aDocument,
                                               PRBool                   aIsRoot,
                                               nsIFrame*&               aNewFrame,                                                                                             
                                               nsCOMPtr<nsIStyleContext>& aScrolledChildStyle,
                                               nsIFrame*&               aScrollableFrame,
                                               nsIFrame*                aScrollPortFrame)
{
  // Check to see the type of parent frame so we know whether we need to 
  // turn off/on scaling for the scrollbars
  //
  // If the parent is a viewportFrame then we are the scrollbars for the UI
  // if not then we are scrollbars inside the document.
  PRBool noScalingOfTwips = PR_FALSE;
  nsCOMPtr<nsIPrintPreviewContext> printPreviewContext(do_QueryInterface(aPresContext));
  if (printPreviewContext) {
    nsCOMPtr<nsIAtom> typeAtom;
    aParentFrame->GetFrameType(getter_AddRefs(typeAtom));
    noScalingOfTwips = typeAtom == nsLayoutAtoms::viewportFrame;
    if (noScalingOfTwips) {
      printPreviewContext->SetScalingOfTwips(PR_FALSE);
    }
  }

  nsIFrame* scrollFrame = nsnull;
  nsIFrame* parentFrame = nsnull;
  nsIFrame* gfxScrollFrame = nsnull;

  nsFrameItems anonymousItems;

  nsCOMPtr<nsIStyleContext> contentStyle = dont_QueryInterface(aContentStyle);

  PRBool isGfx = HasGfxScrollbars();

  if (isGfx) {
  
    BuildGfxScrollFrame(aPresShell, aPresContext, aState, aContent, aDocument, aParentFrame,
                        contentStyle, aIsRoot, gfxScrollFrame, anonymousItems, aScrollPortFrame);

    scrollFrame = anonymousItems.childList; // get the scrollport from the anonymous list
    parentFrame = gfxScrollFrame;
    aNewFrame = gfxScrollFrame;

    // we used the style that was passed in. So resolve another one.
    nsCOMPtr<nsIStyleContext> scrollPseudoStyle;
    aPresContext->ResolvePseudoStyleContextFor(aContent,
                                              nsLayoutAtoms::scrolledContentPseudo,
                                              contentStyle,
                                              getter_AddRefs(scrollPseudoStyle));

    contentStyle = scrollPseudoStyle;
    InitAndRestoreFrame(aPresContext, aState, aContent, 
                        parentFrame, contentStyle, nsnull, scrollFrame);

  } else {
    NS_NewScrollFrame(aPresShell, &scrollFrame);
    aNewFrame = scrollFrame;
    parentFrame = aParentFrame;
    InitAndRestoreFrame(aPresContext, aState, aContent, 
                        parentFrame, contentStyle, nsnull, scrollFrame);
  }

  // initialize the scrolled frame
  nsCOMPtr<nsIStyleContext> scrolledPseudoStyle;
  aPresContext->ResolvePseudoStyleContextFor(aContent,
                                          aScrolledPseudo,
                                          contentStyle,
                                          getter_AddRefs(scrolledPseudoStyle));


  aScrollableFrame = scrollFrame;
  
  // set the child frame for the gfxscrollbar if the is one. This frames will be the 
  // 2 scrollbars and the scrolled frame.
  if (gfxScrollFrame) {
     gfxScrollFrame->SetInitialChildList(aPresContext, nsnull, anonymousItems.childList);
  }


  aScrolledChildStyle = scrolledPseudoStyle;

  if (printPreviewContext && noScalingOfTwips) {
    printPreviewContext->SetScalingOfTwips(PR_TRUE);
  }

  return NS_OK;
}

nsresult
nsCSSFrameConstructor::FinishBuildingScrollFrame(nsIPresContext*      aPresContext,
                                             nsFrameConstructorState& aState,
                                             nsIContent*              aContent,
                                             nsIFrame*                aScrollFrame,
                                             nsIFrame*                aScrolledFrame,
                                             nsIStyleContext*         aScrolledContentStyle)
                                             
{
  // create a view
  nsHTMLContainerFrame::CreateViewForFrame(aPresContext, aScrolledFrame,
                                           aScrolledContentStyle, nsnull, PR_TRUE);

  // the the scroll frames child list
  aScrollFrame->SetInitialChildList(aPresContext, nsnull, aScrolledFrame);

  return NS_OK;
}
 

/**
 * Called to wrap a scrollframe or gfx scrollframe around a frame. The hierarchy will look like this
 *
 *  ------ for native scrollbars -----
 *
 *
 *            ScrollFrame
 *                 ^
 *                 |
 *               Frame (scrolled frame you passed in)
 *
 *
 * ------- for gfx scrollbars ------
 *
 *
 *            GfxScrollFrame
 *                 ^
 *                 |
 *              ScrollPort
 *                 ^
 *                 |
 *               Frame (scrolled frame you passed in)
 *
 *
 *-----------------------------------
 * LEGEND:
 * 
 * ScrollFrame: This is a frame that has a view that manages native scrollbars. It implements
 *              nsIScrollableView. It also manages clipping and scrolling of native widgets by 
 *              having a native scrolling window.
 *
 * GfxScrollFrame: This is a frame that manages gfx cross platform frame based scrollbars.
 *
 * ScrollPort: This is similar to the ScrollFrame above in that is clips and scrolls its children
 *             with a native scrolling window. But because it is contained in a GfxScrollFrame
 *             it does not have any code to do scrollbars so it is much simpler. Infact it only has
 *             1 view attached to it. Where the ScrollFrame above has 5! 
 *             
 *
 * @param aContent the content node of the child to wrap.
 * @param aScrolledFrame The frame of the content to wrap. This should not be
 *                    Initialized. This method will initialize it with a scrolled pseudo
 *                    and no nsIContent. The content will be attached to the scrollframe 
 *                    returned.
 * @param aContentStyle the style context that has already been resolved for the content being passed in.
 *
 * @param aParentFrame The parent to attach the scroll frame to
 *
 * @param aNewFrame The new scrollframe or gfx scrollframe that we create. It will contain the
 *                  scrolled frame you passed in. (returned)
 * @param aScrolledContentStyle the style that was resolved for the scrolled frame. (returned)
 */
nsresult
nsCSSFrameConstructor::BuildScrollFrame       (nsIPresShell* aPresShell, 
                                               nsIPresContext*          aPresContext,
                                               nsFrameConstructorState& aState,
                                               nsIContent*              aContent,
                                               nsIStyleContext*         aContentStyle,
                                               nsIFrame*                aScrolledFrame,
                                               nsIFrame*                aParentFrame,
                                               nsIFrame*&               aNewFrame, 
                                               nsIStyleContext*&        aScrolledContentStyle,
                                               nsIFrame*                aScrollPortFrame)                                                                                                                                          
{
    // Check to see the type of parent frame so we know whether we need to 
    // turn off/on scaling for the scrollbars
    //
    // If the parent is a viewportFrame then we are the scrollbars for the UI
    // if not then we are scrollbars inside the document.
    PRBool noScalingOfTwips = PR_FALSE;
    nsCOMPtr<nsIPrintPreviewContext> printPreviewContext(do_QueryInterface(aPresContext));
    if (printPreviewContext) {
      nsCOMPtr<nsIAtom> typeAtom;
      aParentFrame->GetFrameType(getter_AddRefs(typeAtom));
      noScalingOfTwips = typeAtom == nsLayoutAtoms::viewportFrame;
      if (noScalingOfTwips) {
        printPreviewContext->SetScalingOfTwips(PR_FALSE);
      }
    }

    nsIFrame *scrollFrame;

    nsCOMPtr<nsIStyleContext> scrolledContentStyle;

    
    BeginBuildingScrollFrame(aPresShell, aPresContext,
                     aState,
                     aContent,
                     aContentStyle,
                     aParentFrame,
                     nsLayoutAtoms::scrolledContentPseudo,
                     mDocument,
                     PR_FALSE,
                     aNewFrame,
                     scrolledContentStyle,
                     scrollFrame,
                     aScrollPortFrame);
    
    InitAndRestoreFrame(aPresContext, aState, aContent, 
                        scrollFrame, scrolledContentStyle, nsnull, aScrolledFrame);

    FinishBuildingScrollFrame(aPresContext, 
                          aState,
                          aContent,
                          scrollFrame,
                          aScrolledFrame,
                          scrolledContentStyle);

    aScrolledContentStyle = scrolledContentStyle;

    // now set the primary frame to the ScrollFrame
    aState.mFrameManager->SetPrimaryFrameFor( aContent, aNewFrame );

    if (noScalingOfTwips) {
      printPreviewContext->SetScalingOfTwips(PR_TRUE);
    }

    return NS_OK;

}

/** 
 * If we are building GFX scrollframes this will create one
 */
nsresult
nsCSSFrameConstructor::BuildGfxScrollFrame (nsIPresShell* aPresShell, 
                                            nsIPresContext*          aPresContext,
                                             nsFrameConstructorState& aState,
                                             nsIContent*              aContent,
                                             nsIDocument*             aDocument,
                                             nsIFrame*                aParentFrame,
                                             nsIStyleContext*         aStyleContext,
                                             PRBool                   aIsRoot,
                                             nsIFrame*&               aNewFrame,
                                             nsFrameItems&            aAnonymousFrames,
                                             nsIFrame*                aScrollPortFrame)
{
#ifdef INCLUDE_XUL
  NS_NewGfxScrollFrame(aPresShell, &aNewFrame, aDocument, aIsRoot);

  InitAndRestoreFrame(aPresContext, aState, aContent, 
                      aParentFrame, aStyleContext, nsnull, aNewFrame);

  // Create a view
  nsHTMLContainerFrame::CreateViewForFrame(aPresContext, aNewFrame,
                                             aStyleContext, nsnull, PR_FALSE);

  
  if (!aScrollPortFrame)
    NS_NewScrollPortFrame(aPresShell, &aScrollPortFrame);

  aAnonymousFrames.AddChild(aScrollPortFrame);

  // if there are any anonymous children for the nsScrollFrame create frames for them.
  CreateAnonymousFrames(aPresShell, aPresContext, aState, aContent, aDocument, aNewFrame,
                        aAnonymousFrames);

  return NS_OK;
#endif
  return NS_ERROR_FAILURE;
} 

nsresult
nsCSSFrameConstructor::ConstructFrameByDisplayType(nsIPresShell*            aPresShell, 
                                                   nsIPresContext*          aPresContext,
                                                   nsFrameConstructorState& aState,
                                                   const nsStyleDisplay*    aDisplay,
                                                   nsIContent*              aContent,
                                                   PRInt32                  aNameSpaceID,
                                                   nsIAtom*                 aTag,
                                                   nsIFrame*                aParentFrame,
                                                   nsIStyleContext*         aStyleContext,
                                                   nsFrameItems&            aFrameItems)
{
  PRBool    primaryFrameSet = PR_FALSE;
  PRBool    isAbsolutelyPositioned = PR_FALSE;
  PRBool    isFixedPositioned = PR_FALSE;
  PRBool    isFloating = PR_FALSE;
  PRBool    isBlock = aDisplay->IsBlockLevel();
  nsIFrame* newFrame = nsnull;  // the frame we construct
  nsIFrame* newBlock = nsnull;
  nsIFrame* nextInline = nsnull;
  nsTableCreator tableCreator(aPresShell); // Used to make table frames.
  PRBool    addToHashTable = PR_TRUE;
  PRBool    pseudoParent = PR_FALSE; // is the new frame's parent anonymous
  nsresult  rv = NS_OK;
  PRBool    addNewFrameToChildList = PR_TRUE; 

  // The frame is also a block if it's an inline frame that's floated or
  // absolutely positioned
  if (NS_STYLE_FLOAT_NONE != aDisplay->mFloats) {
    isFloating = PR_TRUE;
  }
  if ((NS_STYLE_DISPLAY_INLINE == aDisplay->mDisplay) &&
      (isFloating || aDisplay->IsAbsolutelyPositioned())) {
    isBlock = PR_TRUE;
  }

  nsIFrame* adjParentFrame = aParentFrame;
  // if the new frame is not table related and the parent is a table, row group, or row,
  // then we need to get or create the pseudo table cell frame and use it as the parent.
  if (adjParentFrame) {
    nsCOMPtr<nsIAtom> parentType;
    adjParentFrame->GetFrameType(getter_AddRefs(parentType));
    if (!IsTableRelated(aDisplay->mDisplay, PR_TRUE) &&
        IsTableRelated(parentType.get(), PR_FALSE) &&
        aTag != nsHTMLAtoms::form) {
      GetPseudoCellFrame(aPresShell, aPresContext, tableCreator, aState, *adjParentFrame);
      if (aState.mPseudoFrames.mCellInner.mFrame) {
        adjParentFrame = aState.mPseudoFrames.mCellInner.mFrame;
        pseudoParent = PR_TRUE;
      }
      else {
        return NS_ERROR_FAILURE;
      }
    }
  }

  // If the frame is a block-level frame and is scrollable, then wrap it
  // in a scroll frame.
  // XXX Ignore tables for the time being
  if ((isBlock && (aDisplay->mDisplay != NS_STYLE_DISPLAY_TABLE)) &&
      IsScrollable(aPresContext, aDisplay)) {

    if (!pseudoParent && !aState.mPseudoFrames.IsEmpty()) { // process pending pseudo frames
      ProcessPseudoFrames(aPresContext, aState.mPseudoFrames, aFrameItems); 
    }
    // See if it's absolute positioned or fixed positioned
    if (NS_STYLE_POSITION_ABSOLUTE == aDisplay->mPosition) {
      isAbsolutelyPositioned = PR_TRUE;
    } else if (NS_STYLE_POSITION_FIXED == aDisplay->mPosition) {
      isFixedPositioned = PR_TRUE;
    }

     // Initialize it
    nsIFrame* geometricParent = adjParentFrame;

    if (isAbsolutelyPositioned) {
      geometricParent = aState.mAbsoluteItems.containingBlock;
    } else if (isFixedPositioned) {
      geometricParent = aState.mFixedItems.containingBlock;
    }

    nsIFrame* scrolledFrame = nsnull;

    NS_NewAreaFrame(aPresShell, &scrolledFrame, NS_BLOCK_SPACE_MGR |
                    NS_BLOCK_SHRINK_WRAP | NS_BLOCK_MARGIN_ROOT);


    nsIStyleContext* newStyle = nsnull;
    // Build the scrollframe it
    BuildScrollFrame(aPresShell, aPresContext, aState, aContent, aStyleContext, scrolledFrame, geometricParent,
                     newFrame, newStyle);

    // buildscrollframe sets the primary frame.
    primaryFrameSet = PR_TRUE;
    
    //-----

    // The area frame is a floater container
    PRBool haveFirstLetterStyle, haveFirstLineStyle;
    HaveSpecialBlockStyle(aPresContext, aContent, aStyleContext,
                          &haveFirstLetterStyle, &haveFirstLineStyle);
    nsFrameConstructorSaveState floaterSaveState;
    aState.PushFloaterContainingBlock(scrolledFrame, floaterSaveState,
                                      haveFirstLetterStyle,
                                      haveFirstLineStyle);

    // Process children
    nsFrameConstructorSaveState absoluteSaveState;
    nsFrameItems                childItems;
    PRBool                      isPositionedContainingBlock = isAbsolutelyPositioned ||
                                                              isFixedPositioned;

    if (isPositionedContainingBlock) {
      // The area frame becomes a container for child frames that are
      // absolutely positioned
      aState.PushAbsoluteContainingBlock(scrolledFrame, absoluteSaveState);
    }
     
    ProcessChildren(aPresShell, aPresContext, aState, aContent, scrolledFrame, PR_FALSE,
                    childItems, PR_TRUE);

    nsCOMPtr<nsIAtom> tag;
    aContent->GetTag(*getter_AddRefs(tag));
    CreateAnonymousFrames(aPresShell, aPresContext, tag, aState, aContent, newFrame,
                            childItems);

      // Set the scrolled frame's initial child lists
    scrolledFrame->SetInitialChildList(aPresContext, nsnull, childItems.childList);
    if (isPositionedContainingBlock && aState.mAbsoluteItems.childList) {
      scrolledFrame->SetInitialChildList(aPresContext,
                                         nsLayoutAtoms::absoluteList,
                                         aState.mAbsoluteItems.childList);
    }

    if (aState.mFloatedItems.childList) {
      scrolledFrame->SetInitialChildList(aPresContext,
                                         nsLayoutAtoms::floaterList,
                                         aState.mFloatedItems.childList);
    }
    ///------
  }
  // See if the frame is absolute or fixed positioned
  else if (aDisplay->IsAbsolutelyPositioned() &&
           ((NS_STYLE_DISPLAY_BLOCK == aDisplay->mDisplay) ||
            (NS_STYLE_DISPLAY_INLINE == aDisplay->mDisplay) ||
            (NS_STYLE_DISPLAY_LIST_ITEM == aDisplay->mDisplay))) {

    if (!pseudoParent && !aState.mPseudoFrames.IsEmpty()) { // process pending pseudo frames
      ProcessPseudoFrames(aPresContext, aState.mPseudoFrames, aFrameItems); 
    }
    if (NS_STYLE_POSITION_ABSOLUTE == aDisplay->mPosition) {
      isAbsolutelyPositioned = PR_TRUE;
    } else {
      isFixedPositioned = PR_TRUE;
    }

    // Create a frame to wrap up the absolute positioned item
    NS_NewAbsoluteItemWrapperFrame(aPresShell, &newFrame);
    InitAndRestoreFrame(aPresContext, aState, aContent, 
                      (isAbsolutelyPositioned
                       ? aState.mAbsoluteItems.containingBlock
                       : aState.mFixedItems.containingBlock), 
                        aStyleContext, nsnull, newFrame);

    // Create a view
    nsHTMLContainerFrame::CreateViewForFrame(aPresContext, newFrame,
                                             aStyleContext, adjParentFrame, PR_FALSE);

    // Process the child content. The area frame becomes a container for child
    // frames that are absolutely positioned
    nsFrameConstructorSaveState absoluteSaveState;
    nsFrameConstructorSaveState floaterSaveState;
    nsFrameItems                childItems;

    PRBool haveFirstLetterStyle = PR_FALSE, haveFirstLineStyle = PR_FALSE;
    if (aDisplay->IsBlockLevel()) {
      HaveSpecialBlockStyle(aPresContext, aContent, aStyleContext,
                            &haveFirstLetterStyle, &haveFirstLineStyle);
    }
    aState.PushAbsoluteContainingBlock(newFrame, absoluteSaveState);
    aState.PushFloaterContainingBlock(newFrame, floaterSaveState,
                                      haveFirstLetterStyle,
                                      haveFirstLineStyle);
    ProcessChildren(aPresShell, aPresContext, aState, aContent, newFrame, PR_TRUE,
                    childItems, PR_TRUE);

    nsCOMPtr<nsIAtom> tag;
    aContent->GetTag(*getter_AddRefs(tag));
    CreateAnonymousFrames(aPresShell, aPresContext, tag, aState, aContent, newFrame,
                          childItems);

    // Set the frame's initial child list(s)
    newFrame->SetInitialChildList(aPresContext, nsnull, childItems.childList);
    if (aState.mAbsoluteItems.childList) {
      newFrame->SetInitialChildList(aPresContext, nsLayoutAtoms::absoluteList,
                                     aState.mAbsoluteItems.childList);
    }
    if (aState.mFloatedItems.childList) {
      newFrame->SetInitialChildList(aPresContext,
                                    nsLayoutAtoms::floaterList,
                                    aState.mFloatedItems.childList);
    }
  }
  // See if the frame is floated, and it's a block or inline frame
  else if (isFloating &&
           ((NS_STYLE_DISPLAY_BLOCK == aDisplay->mDisplay) ||
            (NS_STYLE_DISPLAY_INLINE == aDisplay->mDisplay) ||
            (NS_STYLE_DISPLAY_LIST_ITEM == aDisplay->mDisplay))) {
    if (!pseudoParent && !aState.mPseudoFrames.IsEmpty()) { // process pending pseudo frames
      ProcessPseudoFrames(aPresContext, aState.mPseudoFrames, aFrameItems); 
    }
    // Create an area frame
    NS_NewFloatingItemWrapperFrame(aPresShell, &newFrame);

    // Initialize the frame
    InitAndRestoreFrame(aPresContext, aState, aContent, 
                        aState.mFloatedItems.containingBlock, 
                        aStyleContext, nsnull, newFrame);

    // See if we need to create a view
    nsHTMLContainerFrame::CreateViewForFrame(aPresContext, newFrame,
                                             aStyleContext, adjParentFrame, PR_FALSE);

    // Process the child content
    nsFrameConstructorSaveState floaterSaveState;
    nsFrameItems                childItems;

    PRBool haveFirstLetterStyle = PR_FALSE, haveFirstLineStyle = PR_FALSE;
    if (aDisplay->IsBlockLevel()) {
      HaveSpecialBlockStyle(aPresContext, aContent, aStyleContext,
                            &haveFirstLetterStyle, &haveFirstLineStyle);
    }
    aState.PushFloaterContainingBlock(newFrame, floaterSaveState,
                                      haveFirstLetterStyle,
                                      haveFirstLineStyle);
    ProcessChildren(aPresShell, aPresContext, aState, aContent, newFrame,
                    PR_TRUE, childItems, PR_TRUE);

    nsCOMPtr<nsIAtom> tag;
    aContent->GetTag(*getter_AddRefs(tag));
    CreateAnonymousFrames(aPresShell, aPresContext, tag, aState, aContent, newFrame,
                          childItems);

    // Set the frame's initial child list(s)
    newFrame->SetInitialChildList(aPresContext, nsnull, childItems.childList);
    if (aState.mFloatedItems.childList) {
      newFrame->SetInitialChildList(aPresContext,
                                    nsLayoutAtoms::floaterList,
                                    aState.mFloatedItems.childList);
    }
  }
  // See if it's relatively positioned
  else if ((NS_STYLE_POSITION_RELATIVE == aDisplay->mPosition) &&
           ((NS_STYLE_DISPLAY_BLOCK == aDisplay->mDisplay) ||
            (NS_STYLE_DISPLAY_INLINE == aDisplay->mDisplay) ||
            (NS_STYLE_DISPLAY_LIST_ITEM == aDisplay->mDisplay))) {
    if (!pseudoParent && !aState.mPseudoFrames.IsEmpty()) { // process pending pseudo frames
      ProcessPseudoFrames(aPresContext, aState.mPseudoFrames, aFrameItems); 
    }
    // Is it block-level or inline-level?
    PRBool isBlockFrame = PR_FALSE;
    if ((NS_STYLE_DISPLAY_BLOCK == aDisplay->mDisplay) ||
        (NS_STYLE_DISPLAY_LIST_ITEM == aDisplay->mDisplay)) {
      // Create a wrapper frame. No space manager, though
      NS_NewRelativeItemWrapperFrame(aPresShell, &newFrame);
      isBlockFrame = PR_TRUE;

      // Initialize the frame    
      InitAndRestoreFrame(aPresContext, aState, aContent, 
                          adjParentFrame, aStyleContext, nsnull, newFrame);

      // Create a view
      nsHTMLContainerFrame::CreateViewForFrame(aPresContext, newFrame,
                                               aStyleContext, nsnull, PR_FALSE);

      // Process the child content. Relatively positioned frames becomes a
      // container for child frames that are positioned
      nsFrameConstructorSaveState absoluteSaveState;
      nsFrameConstructorSaveState floaterSaveState;
      nsFrameItems                childItems;

      aState.PushAbsoluteContainingBlock(newFrame, absoluteSaveState);
    
      PRBool haveFirstLetterStyle, haveFirstLineStyle;
      HaveSpecialBlockStyle(aPresContext, aContent, aStyleContext,
                            &haveFirstLetterStyle, &haveFirstLineStyle);
      aState.PushFloaterContainingBlock(newFrame, floaterSaveState,
                                        haveFirstLetterStyle,
                                        haveFirstLineStyle);

      ProcessChildren(aPresShell, aPresContext, aState, aContent, newFrame, PR_TRUE,
                      childItems, isBlockFrame);

      nsCOMPtr<nsIAtom> tag;
      aContent->GetTag(*getter_AddRefs(tag));
      CreateAnonymousFrames(aPresShell, aPresContext, tag, aState, aContent, newFrame,
                            childItems);

      // Set the frame's initial child list
      newFrame->SetInitialChildList(aPresContext, nsnull, childItems.childList);
      if (aState.mAbsoluteItems.childList) {
        newFrame->SetInitialChildList(aPresContext, nsLayoutAtoms::absoluteList,
                                      aState.mAbsoluteItems.childList);
      }
      if (aState.mFloatedItems.childList) {
        newFrame->SetInitialChildList(aPresContext,
                                      nsLayoutAtoms::floaterList,
                                      aState.mFloatedItems.childList);
      }
    } else {
      // Create a positioned inline frame
      NS_NewPositionedInlineFrame(aPresShell, &newFrame);
      ConstructInline(aPresShell, aPresContext, aState, aDisplay, aContent,
                      adjParentFrame, aStyleContext, PR_TRUE, newFrame,
                      &newBlock, &nextInline);
    }
  }
  // See if it's a block frame of some sort
  else if ((NS_STYLE_DISPLAY_BLOCK == aDisplay->mDisplay) ||
           (NS_STYLE_DISPLAY_LIST_ITEM == aDisplay->mDisplay) ||
           (NS_STYLE_DISPLAY_RUN_IN == aDisplay->mDisplay) ||
           (NS_STYLE_DISPLAY_COMPACT == aDisplay->mDisplay) ||
           (NS_STYLE_DISPLAY_INLINE_BLOCK == aDisplay->mDisplay)) {
    if (!pseudoParent && !aState.mPseudoFrames.IsEmpty()) { // process pending pseudo frames
      ProcessPseudoFrames(aPresContext, aState.mPseudoFrames, aFrameItems); 
    }
    // Create the block frame
    rv = NS_NewBlockFrame(aPresShell, &newFrame);
    if (NS_SUCCEEDED(rv)) { // That worked so construct the block and its children
      nsPseudoFrames savePseudo;
      aState.mPseudoFrames.Reset(&savePseudo);
      rv = ConstructBlock(aPresShell, aPresContext, aState, aDisplay, aContent,
                          adjParentFrame, aStyleContext, newFrame);
      if (!aState.mPseudoFrames.IsEmpty()) { // process pending pseudo frames
        ProcessPseudoFrames(aPresContext, aState.mPseudoFrames, aFrameItems); 
      }
      aState.mPseudoFrames = savePseudo;
    }
  }
  // See if it's an inline frame of some sort
  else if ((NS_STYLE_DISPLAY_INLINE == aDisplay->mDisplay) ||
           (NS_STYLE_DISPLAY_MARKER == aDisplay->mDisplay)) {
    if (!pseudoParent && !aState.mPseudoFrames.IsEmpty()) { // process pending pseudo frames
      ProcessPseudoFrames(aPresContext, aState.mPseudoFrames, aFrameItems); 
    }
    // Create the inline frame
    rv = NS_NewInlineFrame(aPresShell, &newFrame);
    if (NS_SUCCEEDED(rv)) { // That worked so construct the inline and its children
      nsPseudoFrames savePseudo;
      aState.mPseudoFrames.Reset(&savePseudo);
      rv = ConstructInline(aPresShell, aPresContext, aState, aDisplay, aContent,
                           adjParentFrame, aStyleContext, PR_FALSE, newFrame,
                           &newBlock, &nextInline);
      if (!aState.mPseudoFrames.IsEmpty()) { // process pending pseudo frames
        ProcessPseudoFrames(aPresContext, aState.mPseudoFrames, aFrameItems); 
      }
      aState.mPseudoFrames = savePseudo;
    }

    // To keep the hash table small don't add inline frames (they're
    // typically things like FONT and B), because we can quickly
    // find them if we need to
    addToHashTable = PR_FALSE;
  }
  // otherwise let the display property influence the frame type to create
  else {
    // XXX This section now only handles table frames; should be
    // factored out probably

    // Use the 'display' property to choose a frame type
    switch (aDisplay->mDisplay) {
    case NS_STYLE_DISPLAY_TABLE:
    {
      if (!aState.mPseudoFrames.IsEmpty()) { // process pending pseudo frames
        ProcessPseudoFrames(aPresContext, aState.mPseudoFrames, aFrameItems); 
      }
      nsIFrame* geometricParent = adjParentFrame;
      if (NS_STYLE_POSITION_ABSOLUTE == aDisplay->mPosition) {
        isAbsolutelyPositioned = PR_TRUE;
        geometricParent = aState.mAbsoluteItems.containingBlock;
      } else if (NS_STYLE_POSITION_FIXED == aDisplay->mPosition) {
        isFixedPositioned = PR_TRUE;
        geometricParent = aState.mFixedItems.containingBlock;
      } else if (isFloating) {
        geometricParent = aState.mFloatedItems.containingBlock;
      }
      nsIFrame* innerTable;
      rv = ConstructTableFrame(aPresShell, aPresContext, aState, aContent, 
                               geometricParent, aStyleContext, tableCreator, 
                               PR_FALSE, aFrameItems, newFrame, innerTable, pseudoParent);
      // if there is a pseudoParent, then newFrame was added to the pseudo cell's child list
      addNewFrameToChildList = !pseudoParent;
      // Note: table construction function takes care of initializing
      // the frame, processing children, and setting the initial child
      // list
      goto nearly_done;
    }
  
    // the next 5 cases are only relevant if the parent is not a table, ConstructTableFrame handles children
    case NS_STYLE_DISPLAY_TABLE_CAPTION:
    {
      // adjParentFrame may be an inner table frame rather than an outer frame 
      // In this case we need to get the outer frame.
      nsIFrame* parentFrame = adjParentFrame;
      nsIFrame* outerFrame = nsnull;
      adjParentFrame->GetParent(&outerFrame);
      nsCOMPtr<nsIAtom> frameType;
      if (outerFrame) {
        outerFrame->GetFrameType(getter_AddRefs(frameType));
        if (nsLayoutAtoms::tableOuterFrame == frameType.get()) {
          parentFrame = outerFrame;
        }
      }
      rv = ConstructTableCaptionFrame(aPresShell, aPresContext, aState, aContent, 
                                      parentFrame, aStyleContext, tableCreator, 
                                      aFrameItems, newFrame, pseudoParent);
      if (!pseudoParent) {
        aFrameItems.AddChild(newFrame);
      }
      return rv;
    }

    case NS_STYLE_DISPLAY_TABLE_ROW_GROUP:
    case NS_STYLE_DISPLAY_TABLE_HEADER_GROUP:
    case NS_STYLE_DISPLAY_TABLE_FOOTER_GROUP:
      rv = ConstructTableRowGroupFrame(aPresShell, aPresContext, aState, aContent, 
                                       adjParentFrame, aStyleContext, tableCreator, 
                                       PR_FALSE, aFrameItems, newFrame, pseudoParent);
      if (!pseudoParent) {
        aFrameItems.AddChild(newFrame);
      }
      return rv;

    case NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP:
      rv = ConstructTableColGroupFrame(aPresShell, aPresContext, aState, aContent, 
                                       adjParentFrame, aStyleContext, tableCreator, 
                                       PR_FALSE, aFrameItems, newFrame, pseudoParent);
      if (!pseudoParent) {
        aFrameItems.AddChild(newFrame);
      }
      return rv;
   
    case NS_STYLE_DISPLAY_TABLE_COLUMN:
      rv = ConstructTableColFrame(aPresShell, aPresContext, aState, aContent, 
                                  adjParentFrame, aStyleContext, tableCreator, 
                                  PR_FALSE, aFrameItems, newFrame, pseudoParent);
      if (!pseudoParent) {
        aFrameItems.AddChild(newFrame);
      }
      return rv;
  
    case NS_STYLE_DISPLAY_TABLE_ROW:
      rv = ConstructTableRowFrame(aPresShell, aPresContext, aState, aContent, 
                                  adjParentFrame, aStyleContext, tableCreator, 
                                  PR_FALSE, aFrameItems, newFrame, pseudoParent);
      if (!pseudoParent) {
        aFrameItems.AddChild(newFrame);
      }
      return rv;
  
    case NS_STYLE_DISPLAY_TABLE_CELL:
      {
        nsIFrame* innerTable;
        rv = ConstructTableCellFrame(aPresShell, aPresContext, aState, aContent, 
                                     adjParentFrame, aStyleContext, tableCreator, 
                                     PR_FALSE, aFrameItems, newFrame, innerTable, pseudoParent);
        if (!pseudoParent) {
          aFrameItems.AddChild(newFrame);
        }
        return rv;
      }
  
    default:
      // Don't create any frame for content that's not displayed...
      if (!aState.mPseudoFrames.IsEmpty()) { // process pending pseudo frames
        ProcessPseudoFrames(aPresContext, aState.mPseudoFrames, aFrameItems); 
      }
      break;
    }
  }

  // If the frame is absolutely positioned, then create a placeholder frame
 nearly_done:
  nsFrameItems& frameItems = (pseudoParent) ? aState.mPseudoFrames.mCellInner.mChildList 
                                            : aFrameItems;
  if (isAbsolutelyPositioned || isFixedPositioned) {
    nsIFrame* placeholderFrame;

    CreatePlaceholderFrameFor(aPresShell, aPresContext, aState.mFrameManager, aContent,
                              newFrame, aStyleContext, adjParentFrame, &placeholderFrame);

    // Add the positioned frame to its containing block's list of child frames
    if (isAbsolutelyPositioned) {
      aState.mAbsoluteItems.AddChild(newFrame);
    } else {
      aState.mFixedItems.AddChild(newFrame);
    }

    // Add the placeholder frame to the flow
    frameItems.AddChild(placeholderFrame);

  } else if (isFloating) {
    nsIFrame* placeholderFrame;
    CreatePlaceholderFrameFor(aPresShell, aPresContext, aState.mFrameManager, aContent, newFrame,
                              aStyleContext, adjParentFrame, &placeholderFrame);

    // Add the floating frame to its containing block's list of child frames
    aState.mFloatedItems.AddChild(newFrame);

    // Add the placeholder frame to the flow
    frameItems.AddChild(placeholderFrame);
  } else if (newFrame && addNewFrameToChildList) {
    // Add the frame we just created to the flowed list
    frameItems.AddChild(newFrame);
    if (newBlock) {
      frameItems.AddChild(newBlock);
      if (nextInline) {
        frameItems.AddChild(nextInline);
      }
    }
  }

  if (newFrame && addToHashTable) {
    // Add a mapping from content object to primary frame. Note that for
    // floated and positioned frames this is the out-of-flow frame and not
    // the placeholder frame
    if (!primaryFrameSet)
      aState.mFrameManager->SetPrimaryFrameFor(aContent, newFrame);
  }

  return rv;
}


PRBool
nsCSSFrameConstructor::IsScrollable(nsIPresContext*       aPresContext,
                                    const nsStyleDisplay* aDisplay)
{
  // For the time being it's scrollable if the overflow property is auto or
  // scroll, regardless of whether the width or height is fixed in size
  switch (aDisplay->mOverflow) {
  	case NS_STYLE_OVERFLOW_SCROLL:
  	case NS_STYLE_OVERFLOW_AUTO:
  	case NS_STYLE_OVERFLOW_SCROLLBARS_NONE:
  	case NS_STYLE_OVERFLOW_SCROLLBARS_HORIZONTAL:
  	case NS_STYLE_OVERFLOW_SCROLLBARS_VERTICAL:
	    return PR_TRUE;
  }
  return PR_FALSE;
}


nsresult 
nsCSSFrameConstructor::InitAndRestoreFrame(nsIPresContext*          aPresContext,
                                           nsFrameConstructorState& aState,
                                           nsIContent*              aContent,
                                           nsIFrame*                aParentFrame,
                                           nsIStyleContext*         aStyleContext,
                                           nsIFrame*                aPrevInFlow,
                                           nsIFrame*                aNewFrame)
{
  nsresult rv = NS_OK;
  
  NS_ASSERTION(aNewFrame, "Null frame cannot be initialized");
  if (!aNewFrame)
    return NS_ERROR_NULL_POINTER;

  // Initialize the frame
  rv = aNewFrame->Init(aPresContext, aContent, aParentFrame, 
                       aStyleContext, aPrevInFlow);

  if (aState.mFrameState && aState.mFrameManager) {
    // Restore frame state for just the newly created frame.
    aState.mFrameManager->RestoreFrameStateFor(aPresContext, aNewFrame,
                                               aState.mFrameState);
  }

  return rv;
}

nsresult
nsCSSFrameConstructor::ResolveStyleContext(nsIPresContext*   aPresContext,
                                           nsIFrame*         aParentFrame,
                                           nsIContent*       aContent,
                                           nsIStyleContext** aStyleContext)
{
  nsresult rv = NS_OK;
  // Resolve the style context based on the content object and the parent
  // style context
  nsCOMPtr<nsIStyleContext> parentStyleContext;

  aParentFrame->GetStyleContext(getter_AddRefs(parentStyleContext));
  if (aContent->IsContentOfType(nsIContent::eELEMENT)) {
    rv = aPresContext->ResolveStyleContextFor(aContent, parentStyleContext,
                                              aStyleContext);
  } else {
#ifdef DEBUG
    {
      nsCOMPtr<nsIAtom> tag;
      aContent->GetTag(*getter_AddRefs(tag));
      NS_ASSERTION(tag == nsLayoutAtoms::textTagName,
                   "shouldn't waste time creating style contexts for "
                   "comments and processing instructions");
    }
#endif
    rv = aPresContext->ResolveStyleContextForNonElement(parentStyleContext,
                                                        aStyleContext);
  }
  return rv;
}

// MathML Mod - RBS
#ifdef MOZ_MATHML
nsresult
nsCSSFrameConstructor::ConstructMathMLFrame(nsIPresShell*            aPresShell,
                                            nsIPresContext*          aPresContext,
                                            nsFrameConstructorState& aState,
                                            nsIContent*              aContent,
                                            nsIFrame*                aParentFrame,
                                            nsIAtom*                 aTag,
                                            PRInt32                  aNameSpaceID,
                                            nsIStyleContext*         aStyleContext,
                                            nsFrameItems&            aFrameItems)
{
  // Make sure that we remain confined in the MathML world
  if (aNameSpaceID != kNameSpaceID_MathML) 
    return NS_OK;

  PRBool    processChildren = PR_TRUE;  // Whether we should process child content.
                                        // MathML frames are inline frames.
                                        // processChildren = PR_TRUE for inline frames.
                                        // see case NS_STYLE_DISPLAY_INLINE in
                                        // ConstructFrameByDisplayType()

  nsresult  rv = NS_OK;
  PRBool    isAbsolutelyPositioned = PR_FALSE;
  PRBool    isFixedPositioned = PR_FALSE;
  PRBool    isReplaced = PR_FALSE;
  PRBool    ignoreInterTagWhitespace = PR_TRUE;

  NS_ASSERTION(aTag != nsnull, "null MathML tag");
  if (aTag == nsnull)
    return NS_OK;

  // Initialize the new frame
  nsIFrame* newFrame = nsnull;

  // See if the element is absolute or fixed positioned
  const nsStyleDisplay* disp = (const nsStyleDisplay*)
    aStyleContext->GetStyleData(eStyleStruct_Display);
  if (NS_STYLE_POSITION_ABSOLUTE == disp->mPosition) {
    isAbsolutelyPositioned = PR_TRUE;
  }
  else if (NS_STYLE_POSITION_FIXED == disp->mPosition) {
    isFixedPositioned = PR_TRUE;
  }

  if (aTag == nsMathMLAtoms::mi_ ||
      aTag == nsMathMLAtoms::mn_ ||
      aTag == nsMathMLAtoms::ms_ ||
      aTag == nsMathMLAtoms::mtext_)
     rv = NS_NewMathMLTokenFrame(aPresShell, &newFrame);
  else if (aTag == nsMathMLAtoms::mo_)
     rv = NS_NewMathMLmoFrame(aPresShell, &newFrame);
  else if (aTag == nsMathMLAtoms::mfrac_)
     rv = NS_NewMathMLmfracFrame(aPresShell, &newFrame);
  else if (aTag == nsMathMLAtoms::msup_)
     rv = NS_NewMathMLmsupFrame(aPresShell, &newFrame);
  else if (aTag == nsMathMLAtoms::msub_)
     rv = NS_NewMathMLmsubFrame(aPresShell, &newFrame);
  else if (aTag == nsMathMLAtoms::msubsup_)
     rv = NS_NewMathMLmsubsupFrame(aPresShell, &newFrame);
  else if (aTag == nsMathMLAtoms::munder_)
     rv = NS_NewMathMLmunderFrame(aPresShell, &newFrame);
  else if (aTag == nsMathMLAtoms::mover_)
     rv = NS_NewMathMLmoverFrame(aPresShell, &newFrame);
  else if (aTag == nsMathMLAtoms::munderover_)
     rv = NS_NewMathMLmunderoverFrame(aPresShell, &newFrame);
  else if (aTag == nsMathMLAtoms::mphantom_)
     rv = NS_NewMathMLmphantomFrame(aPresShell, &newFrame);
  else if (aTag == nsMathMLAtoms::mpadded_)
     rv = NS_NewMathMLmpaddedFrame(aPresShell, &newFrame);
  else if (aTag == nsMathMLAtoms::mspace_)
     rv = NS_NewMathMLmspaceFrame(aPresShell, &newFrame);
  else if (aTag == nsMathMLAtoms::mfenced_)
     rv = NS_NewMathMLmfencedFrame(aPresShell, &newFrame);
  else if (aTag == nsMathMLAtoms::mmultiscripts_)
     rv = NS_NewMathMLmmultiscriptsFrame(aPresShell, &newFrame);
  else if (aTag == nsMathMLAtoms::mstyle_)
     rv = NS_NewMathMLmstyleFrame(aPresShell, &newFrame);
  else if (aTag == nsMathMLAtoms::msqrt_)
     rv = NS_NewMathMLmsqrtFrame(aPresShell, &newFrame);
  else if (aTag == nsMathMLAtoms::mroot_)
     rv = NS_NewMathMLmrootFrame(aPresShell, &newFrame);
  else if (aTag == nsMathMLAtoms::maction_)
     rv = NS_NewMathMLmactionFrame(aPresShell, &newFrame);
  else if (aTag == nsMathMLAtoms::mrow_   ||
           aTag == nsMathMLAtoms::merror_ ||
           aTag == nsMathMLAtoms::none_   ||
           aTag == nsMathMLAtoms::mprescripts_ )
     rv = NS_NewMathMLmrowFrame(aPresShell, &newFrame);
  // CONSTRUCTION of MTABLE elements
  else if (aTag == nsMathMLAtoms::mtable_ &&
           disp->mDisplay == NS_STYLE_DISPLAY_TABLE) {
      // <mtable> is an inline-table -- but this isn't yet supported.
      // What we do here is to wrap the table in an anonymous containing
      // block so that it can mix better with other surrounding MathML markups

      nsCOMPtr<nsIStyleContext> parentContext;
      aParentFrame->GetStyleContext(getter_AddRefs(parentContext));

      // first, create a MathML mrow frame that will wrap the block frame
      rv = NS_NewMathMLmrowFrame(aPresShell, &newFrame);
      if (NS_FAILED(rv)) return rv;
      nsCOMPtr<nsIStyleContext> mrowContext;
      aPresContext->ResolvePseudoStyleContextFor(aContent,
                                                 nsMathMLAtoms::mozMathInline,
                                                 parentContext,
                                                 getter_AddRefs(mrowContext));
      InitAndRestoreFrame(aPresContext, aState, aContent, aParentFrame,
                          mrowContext, nsnull, newFrame);

      // then, create a block frame that will wrap the table frame
      nsIFrame* blockFrame;
      rv = NS_NewBlockFrame(aPresShell, &blockFrame);
      if (NS_FAILED(rv)) return rv;
      nsCOMPtr<nsIStyleContext> blockContext;
      aPresContext->ResolvePseudoStyleContextFor(aContent,
                                                 nsHTMLAtoms::mozAnonymousBlock,
                                                 mrowContext,
                                                 getter_AddRefs(blockContext));
      InitAndRestoreFrame(aPresContext, aState, aContent, newFrame,
                          blockContext, nsnull, blockFrame);

      // then, create the table frame itself
      nsCOMPtr<nsIStyleContext> tableContext;
      aPresContext->ResolveStyleContextFor(aContent, blockContext,
                                           getter_AddRefs(tableContext));
      nsFrameItems tempItems;
      nsIFrame* outerTable;
      nsIFrame* innerTable;
      PRBool pseudoParent;
      nsMathMLmtableCreator mathTableCreator(aPresShell);
      rv = ConstructTableFrame(aPresShell, aPresContext, aState, aContent, 
                               blockFrame, tableContext, mathTableCreator,
                               PR_FALSE, tempItems, outerTable, innerTable, pseudoParent);
      // Note: table construction function takes care of initializing the frame,
      // processing children, and setting the initial child list

      // set the outerTable as the initial child of the anonymous block
      blockFrame->SetInitialChildList(aPresContext, nsnull, outerTable);

      // set the block frame as the initial child of the mrow frame
      newFrame->SetInitialChildList(aPresContext, nsnull, blockFrame);

      // add the new frame to the flow
      aFrameItems.AddChild(newFrame);

      return rv; 
  }
  // End CONSTRUCTION of MTABLE elements 

  else if (aTag == nsMathMLAtoms::math) { 
    // root <math> element
    const nsStyleDisplay* display = (const nsStyleDisplay*)
      aStyleContext->GetStyleData(eStyleStruct_Display);
    PRBool isBlock = (NS_STYLE_DISPLAY_BLOCK == display->mDisplay);
    rv = NS_NewMathMLmathFrame(aPresShell, &newFrame, isBlock);
  }
  else {
     return rv;
  }

  // If we succeeded in creating a frame then initialize it, process its
  // children (if requested), and set the initial child list
  if (NS_SUCCEEDED(rv) && newFrame != nsnull) {
    nsFrameState state;
    newFrame->GetFrameState(&state);
    // If the frame is a replaced element, then set the frame state bit
    if (isReplaced) {
      state |= NS_FRAME_REPLACED_ELEMENT;
    }
    // record that children that are ignorable whitespace should be excluded
    if (ignoreInterTagWhitespace) {
      state |= NS_FRAME_EXCLUDE_IGNORABLE_WHITESPACE;
    }
    newFrame->SetFrameState(state);

    nsIFrame* geometricParent = isAbsolutelyPositioned
                              ? aState.mAbsoluteItems.containingBlock
                              : aParentFrame;
    InitAndRestoreFrame(aPresContext, aState, aContent, 
                        geometricParent, aStyleContext, nsnull, newFrame);

    // See if we need to create a view, e.g. the frame is absolutely positioned
    nsHTMLContainerFrame::CreateViewForFrame(aPresContext, newFrame,
                                             aStyleContext, aParentFrame, PR_FALSE);

    // Process the child content if requested
    nsFrameItems childItems;
    if (processChildren) {
      rv = ProcessChildren(aPresShell, aPresContext, aState, aContent, newFrame, PR_TRUE,
                           childItems, PR_FALSE);

      CreateAnonymousFrames(aPresShell, aPresContext, aTag, aState, aContent, newFrame,
                            childItems);
    }

    // Set the frame's initial child list
    newFrame->SetInitialChildList(aPresContext, nsnull, childItems.childList);

    // If the frame is absolutely positioned then create a placeholder frame
    if (isAbsolutelyPositioned || isFixedPositioned) {
      nsIFrame* placeholderFrame;

      CreatePlaceholderFrameFor(aPresShell, aPresContext, aState.mFrameManager, aContent, newFrame, 
                                aStyleContext, aParentFrame, &placeholderFrame);

      // Add the positioned frame to its containing block's list of child frames
      if (isAbsolutelyPositioned) {
        aState.mAbsoluteItems.AddChild(newFrame);
      } else {
        aState.mFixedItems.AddChild(newFrame);
      }

      // Add the placeholder frame to the flow
      aFrameItems.AddChild(placeholderFrame);
    }
    else {
      // Add the new frame to our list of frame items.
      aFrameItems.AddChild(newFrame);
    }
  }
  return rv;
}
#endif // MOZ_MATHML

// SVG 
#ifdef MOZ_SVG
nsresult
nsCSSFrameConstructor::ConstructSVGFrame(nsIPresShell*            aPresShell,
                                          nsIPresContext*          aPresContext,
                                          nsFrameConstructorState& aState,
                                          nsIContent*              aContent,
                                          nsIFrame*                aParentFrame,
                                          nsIAtom*                 aTag,
                                          PRInt32                  aNameSpaceID,
                                          nsIStyleContext*         aStyleContext,
                                          nsFrameItems&            aFrameItems)
{
  NS_ASSERTION(NS_SUCCEEDED(aContent->GetNameSpaceID(aNameSpaceID)) && 
            (aNameSpaceID == nsSVGAtoms::nameSpaceID), "SVG frame constructed in wrong namespace");

  nsresult  rv = NS_OK;
  PRBool isAbsolutelyPositioned = PR_FALSE;
  PRBool isFixedPositioned = PR_FALSE;
  PRBool forceView = PR_FALSE;
  PRBool isBlock = PR_FALSE;
  PRBool processChildren = PR_FALSE;
  
  NS_ASSERTION(aTag != nsnull, "null SVG tag");
  if (aTag == nsnull)
    return NS_OK;

  // Initialize the new frame
  nsIFrame* newFrame = nsnull;
  //nsSVGTableCreator svgTableCreator(aPresShell); // Used to make table views.
 
  // See if the element is absolute or fixed positioned
  const nsStyleDisplay* disp = (const nsStyleDisplay*)
    aStyleContext->GetStyleData(eStyleStruct_Display);
  if (NS_STYLE_POSITION_ABSOLUTE == disp->mPosition) {
    isAbsolutelyPositioned = PR_TRUE;
  }
  else if (NS_STYLE_POSITION_FIXED == disp->mPosition) {
    isFixedPositioned = PR_TRUE;
  }

  if (aTag == nsSVGAtoms::svg) {
    forceView = PR_TRUE;
    isBlock = PR_TRUE;
    processChildren = PR_TRUE;
    rv = NS_NewSVGOuterSVGFrame(aPresShell, aContent, &newFrame);
  }
  else if (aTag == nsSVGAtoms::g) {
    processChildren = PR_TRUE;
    rv = NS_NewSVGGFrame(aPresShell, aContent, &newFrame);
  }
  else if (aTag == nsSVGAtoms::polygon)
    rv = NS_NewSVGPolygonFrame(aPresShell, aContent, &newFrame);
  else if (aTag == nsSVGAtoms::polyline)
    rv = NS_NewSVGPolylineFrame(aPresShell, aContent, &newFrame);
  else if (aTag == nsSVGAtoms::circle)
    rv = NS_NewSVGCircleFrame(aPresShell, aContent, &newFrame);
  else if (aTag == nsSVGAtoms::ellipse)
    rv = NS_NewSVGEllipseFrame(aPresShell, aContent, &newFrame);
  else if (aTag == nsSVGAtoms::line)
    rv = NS_NewSVGLineFrame(aPresShell, aContent, &newFrame);
  else if (aTag == nsSVGAtoms::rect)
    rv = NS_NewSVGRectFrame(aPresShell, aContent, &newFrame);
  else if (aTag == nsSVGAtoms::foreignObject) {
    processChildren = PR_TRUE;
    rv = NS_NewSVGForeignObjectFrame(aPresShell, aContent, &newFrame);
  }
  else if (aTag == nsSVGAtoms::path)
    rv = NS_NewSVGPathFrame(aPresShell, aContent, &newFrame);
  
  if (newFrame == nsnull) {
    // Either we have an unknown tag, or construction of a frame
    // failed. One reason why frame construction for a known tag might
    // have failed is that the content element doesn't implement all
    // interfaces required by the frame. This happens e.g. when using
    // 'extends' in xbl to extend an xbl binding from an svg
    // element. In that case, the bound content element will always be
    // a standard xml element, and not be of the right type.
    // The best we can do here is to create a generic svg container frame.
#ifdef DEBUG
    printf("Warning: Creating SVGGenericContainerFrame for tag <");
    nsAutoString str;
    aTag->ToString(str);
    printf("%s>\n", NS_ConvertUCS2toUTF8(str).get());
#endif
    processChildren = PR_TRUE;
    rv = NS_NewSVGGenericContainerFrame(aPresShell, aContent, &newFrame);
  }  
  // If we succeeded in creating a frame then initialize it, process its
  // children (if requested), and set the initial child list
  if (NS_SUCCEEDED(rv) && newFrame != nsnull) {

    nsIFrame* geometricParent = isAbsolutelyPositioned
                              ? aState.mAbsoluteItems.containingBlock
                              : aParentFrame;
    InitAndRestoreFrame(aPresContext, aState, aContent, 
                        geometricParent, aStyleContext, nsnull, newFrame);

    nsHTMLContainerFrame::CreateViewForFrame(aPresContext, newFrame,
                                             aStyleContext, aParentFrame,
                                             forceView);

    // Add the new frame to our list of frame items.
    aFrameItems.AddChild(newFrame);

    // Process the child content if requested
    nsFrameItems childItems;
    if (processChildren) {
      rv = ProcessChildren(aPresShell, aPresContext, aState, aContent,
                           newFrame, PR_TRUE, childItems, isBlock);

      CreateAnonymousFrames(aPresShell, aPresContext, aTag, aState, aContent, newFrame,
                            childItems);
    }

    // Set the frame's initial child list
    newFrame->SetInitialChildList(aPresContext, nsnull, childItems.childList);
  
    // If the frame is absolutely positioned then create a placeholder frame
    if (isAbsolutelyPositioned || isFixedPositioned) {
      nsIFrame* placeholderFrame;

      CreatePlaceholderFrameFor(aPresShell, aPresContext, aState.mFrameManager, aContent, newFrame, 
                                aStyleContext, aParentFrame, &placeholderFrame);

      // Add the positioned frame to its containing block's list of child frames
      if (isAbsolutelyPositioned) {
        aState.mAbsoluteItems.AddChild(newFrame);
      } else {
        aState.mFixedItems.AddChild(newFrame);
      }

      // Add the placeholder frame to the flow
      aFrameItems.AddChild(placeholderFrame);
    }
  }
  return rv;
}
#endif // MOZ_SVG

PRBool
nsCSSFrameConstructor::PageBreakBefore(nsIPresShell*            aPresShell,
                                       nsIPresContext*          aPresContext,
                                       nsFrameConstructorState& aState,
                                       nsIContent*              aContent,
                                       nsIFrame*                aParentFrame,
                                       nsIStyleContext*         aStyleContext,
                                       nsFrameItems&            aFrameItems)
{
  const nsStyleDisplay* display = (const nsStyleDisplay*)
    aStyleContext->GetStyleData(eStyleStruct_Display);

  // See if page-break-before is set for all elements except row groups, rows, cells 
  // (these are handled internally by tables) and construct a page break frame if so.
  if (display && ((NS_STYLE_DISPLAY_TABLE == display->mDisplay) ||
                  (!IsTableRelated(display->mDisplay, PR_TRUE)))) { 
    if (display->mBreakBefore) {
      ConstructPageBreakFrame(aPresShell, aPresContext, aState, aContent,
                              aParentFrame, aStyleContext, aFrameItems);
    }
    return display->mBreakAfter;
  }
  return PR_FALSE;
}

nsresult
nsCSSFrameConstructor::ConstructPageBreakFrame(nsIPresShell*            aPresShell, 
                                               nsIPresContext*          aPresContext,
                                               nsFrameConstructorState& aState,
                                               nsIContent*              aContent,
                                               nsIFrame*                aParentFrame,
                                               nsIStyleContext*         aStyleContext,
                                               nsFrameItems&            aFrameItems)
{
  nsCOMPtr<nsIStyleContext> pseudoStyle;
  aPresContext->ResolvePseudoStyleContextFor(nsnull, nsLayoutAtoms::pageBreakPseudo,
                                             aStyleContext,
                                             getter_AddRefs(pseudoStyle));
  nsIFrame* pageBreakFrame;
  nsresult rv = NS_NewPageBreakFrame(aPresShell, &pageBreakFrame); 
  if (NS_SUCCEEDED(rv)) {
    InitAndRestoreFrame(aPresContext, aState, aContent, aParentFrame, 
                        pseudoStyle, nsnull, pageBreakFrame);
    aFrameItems.AddChild(pageBreakFrame);
  }
  return rv;
}

nsresult
nsCSSFrameConstructor::ConstructFrame(nsIPresShell*            aPresShell, 
                                      nsIPresContext*          aPresContext,
                                      nsFrameConstructorState& aState,
                                      nsIContent*              aContent,
                                      nsIFrame*                aParentFrame,
                                      nsFrameItems&            aFrameItems)

{
  NS_PRECONDITION(nsnull != aParentFrame, "no parent frame");

  nsresult rv = NS_OK;

  // don't create a whitespace frame if aParent doesn't want it
  if (!NeedFrameFor(aParentFrame, aContent)) {
    return rv;
  }

  // Get the element's tag
  nsCOMPtr<nsIAtom>  tag;
  aContent->GetTag(*getter_AddRefs(tag));

  // never create frames for comments on PIs
  if (tag == nsLayoutAtoms::commentTagName ||
      tag == nsLayoutAtoms::processingInstructionTagName)
    return rv;

  nsCOMPtr<nsIStyleContext> styleContext;
  rv = ResolveStyleContext(aPresContext, aParentFrame, aContent,
                           getter_AddRefs(styleContext));

  if (NS_SUCCEEDED(rv)) {   
    PRInt32 nameSpaceID;
    aContent->GetNameSpaceID(nameSpaceID);

    PRBool pageBreakAfter = PR_FALSE;
    PRBool paginated;
    aPresContext->IsPaginated(&paginated);

    if (paginated) {
      // See if there is a page break before, if so construct one. Also see if there is one after
      pageBreakAfter = PageBreakBefore(aPresShell, aPresContext, aState, aContent, 
                                       aParentFrame, styleContext, aFrameItems);
    }
    // construct the frame
    rv = ConstructFrameInternal(aPresShell, aPresContext, aState, aContent, aParentFrame,
                                tag, nameSpaceID, styleContext, aFrameItems, PR_FALSE);
    if (NS_SUCCEEDED(rv) && pageBreakAfter) {
      // Construct the page break after
      ConstructPageBreakFrame(aPresShell, aPresContext, aState, aContent,
                              aParentFrame, styleContext, aFrameItems);
    }
  }
  
  return rv;
}


nsresult
nsCSSFrameConstructor::ConstructFrameInternal( nsIPresShell*            aPresShell, 
                                               nsIPresContext*          aPresContext,
                                               nsFrameConstructorState& aState,
                                               nsIContent*              aContent,
                                               nsIFrame*                aParentFrame,
                                               nsIAtom*                 aTag,
                                               PRInt32                  aNameSpaceID,
                                               nsIStyleContext*         aStyleContext,
                                               nsFrameItems&            aFrameItems,
                                               PRBool                   aXBLBaseTag)
{
  // The following code allows the user to specify the base tag
  // of an element using XBL.  XUL and HTML objects (like boxes, menus, etc.)
  // can then be extended arbitrarily.
  const nsStyleDisplay*  display = (const nsStyleDisplay*)
        aStyleContext->GetStyleData(eStyleStruct_Display);
  nsCOMPtr<nsIStyleContext> styleContext(aStyleContext);
  nsCOMPtr<nsIXBLBinding> binding;
  if (!aXBLBaseTag)
  {
    
    // Ensure that our XBL bindings are installed.
    if (!display->mBinding.IsEmpty()) {
      // Get the XBL loader.
      nsresult rv;
      // Load the bindings.
      PRBool resolveStyle;
      if (!gXBLService)
        return NS_ERROR_FAILURE;
      rv = gXBLService->LoadBindings(aContent, display->mBinding, PR_FALSE, getter_AddRefs(binding), &resolveStyle);
      if (NS_FAILED(rv))
        return NS_OK;

      if (resolveStyle) {
        rv = ResolveStyleContext(aPresContext, aParentFrame, aContent,
                                 getter_AddRefs(styleContext));
        if (NS_FAILED(rv))
          return rv;
      }

      nsCOMPtr<nsIAtom> baseTag;
      PRInt32 nameSpaceID;
      gXBLService->ResolveTag(aContent, &nameSpaceID, getter_AddRefs(baseTag));
 
      if (baseTag.get() != aTag || aNameSpaceID != nameSpaceID) {
        // Construct the frame using the XBL base tag.
        rv = ConstructFrameInternal( aPresShell, 
                                  aPresContext,
                                  aState,
                                  aContent,
                                  aParentFrame,
                                  baseTag,
                                  nameSpaceID,
                                  styleContext,
                                  aFrameItems,
                                  PR_TRUE);
        if (binding) {
          nsCOMPtr<nsIBindingManager> bm;
          mDocument->GetBindingManager(getter_AddRefs(bm));
          if (bm)
            bm->AddToAttachedQueue(binding);
        }
        return rv;
      }
    }
  }

  // Pre-check for display "none" - if we find that, don't create
  // any frame at all
  if (NS_STYLE_DISPLAY_NONE == display->mDisplay) {
    aState.mFrameManager->SetUndisplayedContent(aContent, styleContext);
    return NS_OK;
  }

  if (aTag == nsLayoutAtoms::textTagName)
    return ConstructTextFrame(aPresShell, aPresContext, aState,
                              aContent, aParentFrame, styleContext,
                              aFrameItems);

  // Style resolution can normally happen lazily.  However, getting the
  // Visibility struct can cause |SetBidiEnabled| to be called on the
  // pres context, and this needs to happen before we start reflow, so
  // do it now, when constructing frames.  See bug 115291.
  {
    const nsStyleVisibility *vis;
    GetStyleData(styleContext.get(), &vis);
  }

  nsIFrame* lastChild = aFrameItems.lastChild;

  // Handle specific frame types
  nsresult rv = ConstructHTMLFrame(aPresShell, aPresContext, aState,
                                   aContent, aParentFrame, aTag,
                                   aNameSpaceID, styleContext, aFrameItems);

#ifdef INCLUDE_XUL
  // Failing to find a matching HTML frame, try creating a specialized
  // XUL frame. This is temporary, pending planned factoring of this
  // whole process into separate, pluggable steps.
  if (NS_SUCCEEDED(rv) && ((nsnull == aFrameItems.childList) ||
                         (lastChild == aFrameItems.lastChild))) {
    PRBool haltProcessing = PR_FALSE;
    rv = ConstructXULFrame(aPresShell, aPresContext, aState, aContent, aParentFrame,
                           aTag, aNameSpaceID, styleContext, aFrameItems, aXBLBaseTag, haltProcessing);
    if (haltProcessing) {
      return rv;
    }
  } 
#endif

// MathML Mod - RBS
#ifdef MOZ_MATHML
  if (NS_SUCCEEDED(rv) && ((nsnull == aFrameItems.childList) ||
                           (lastChild == aFrameItems.lastChild))) {
    rv = ConstructMathMLFrame(aPresShell, aPresContext, aState, aContent, aParentFrame,
                              aTag, aNameSpaceID, styleContext, aFrameItems);
  }
#endif

// SVG
#ifdef MOZ_SVG
  if (NS_SUCCEEDED(rv) &&
      ((nsnull == aFrameItems.childList) ||
       (lastChild == aFrameItems.lastChild)) &&
      (aNameSpaceID == nsSVGAtoms::nameSpaceID)) {
    rv = ConstructSVGFrame(aPresShell, aPresContext, aState, aContent, aParentFrame,
                              aTag, aNameSpaceID, styleContext, aFrameItems);
  }
#endif

  if (NS_SUCCEEDED(rv) && ((nsnull == aFrameItems.childList) ||
                           (lastChild == aFrameItems.lastChild))) {
    // When there is no explicit frame to create, assume it's a
    // container and let display style dictate the rest
    rv = ConstructFrameByDisplayType(aPresShell, aPresContext, aState, display,
                                     aContent, aNameSpaceID, aTag,
                                     aParentFrame, styleContext, aFrameItems);
  }

  if (binding) {
    nsCOMPtr<nsIBindingManager> bm;
    mDocument->GetBindingManager(getter_AddRefs(bm));
    if (bm)
      bm->AddToAttachedQueue(binding);
  }

  return rv;
}


inline PRBool
IsRootBoxFrame(nsIFrame *aFrame)
{
  nsCOMPtr<nsIAtom> frameType;
  aFrame->GetFrameType(getter_AddRefs(frameType));
  return (frameType == nsLayoutAtoms::rootFrame);
}

NS_IMETHODIMP
nsCSSFrameConstructor::ReconstructDocElementHierarchy(nsIPresContext* aPresContext)
{
  NS_PRECONDITION(aPresContext, "null pres context argument");
  
#ifdef DEBUG
  if (gNoisyContentUpdates) {
    printf("nsCSSFrameConstructor::ReconstructDocElementHierarchy\n");
  }
#endif

  nsresult rv = NS_OK;
  nsCOMPtr<nsIPresShell> shell;
  aPresContext->GetShell(getter_AddRefs(shell));

  if (mDocument && shell) {
    nsCOMPtr<nsIContent> rootContent;
    mDocument->GetRootContent(getter_AddRefs(rootContent));
    
    if (rootContent) {
      // Before removing the frames associated with the content object, ask them to save their
      // state onto a temporary state object.
      CaptureStateForFramesOf(aPresContext, rootContent, mTempFrameTreeState);

      nsFrameConstructorState state(aPresContext, mFixedContainingBlock,
                                    nsnull, nsnull, mTempFrameTreeState);
      nsIFrame* docElementFrame;
        
      // Get the frame that corresponds to the document element
      state.mFrameManager->GetPrimaryFrameFor(rootContent, &docElementFrame);

      // Clear the hash tables that map from content to frame and out-of-flow
      // frame to placeholder frame
      state.mFrameManager->ClearPrimaryFrameMap();
      state.mFrameManager->ClearPlaceholderFrameMap();
      state.mFrameManager->ClearUndisplayedContentMap();

      // Take the docElementFrame, and remove it from its parent. For
      // HTML, we'll be removing the Area frame from the Canvas; for
      // XUL, we'll remove the GfxScroll or Box from the RootBoxFrame.
      //
      // The three possible structures (at least the ones observed so
      // far, see bugs 70258 and 93558) are:
      //
      // (HTML)
      //    ScrollBoxFrame(html)<
      //     ScrollPortFrame(html)<
      //      Canvas(-1)<
      //       Area(html)<
      //        (etc.)
      //
      // (XUL #1)
      //    RootBoxFrame(window)<
      //     GfxScroll<
      //      ScrollBoxFrame(window)<
      //       ScrollPortFrame(window)<
      //        (etc.)
      //
      // (XUL #2)
      //    RootBox<
      //     Box<
      //      (etc.)
      //
      if (docElementFrame) {
        nsIFrame* docParentFrame;
        docElementFrame->GetParent(&docParentFrame);

        // If we're in a XUL document, then we need to crawl up to the
        // RootBoxFrame and remove _its_ child.
        nsCOMPtr<nsIXULDocument> xuldoc = do_QueryInterface(mDocument);
        if (xuldoc) {
          nsCOMPtr<nsIAtom> frameType;
          while (docParentFrame && !IsRootBoxFrame(docParentFrame)) {
            docElementFrame = docParentFrame;
            docParentFrame->GetParent(&docParentFrame);
          }
        }

        NS_ASSERTION(docParentFrame, "should have a parent frame");
        if (docParentFrame) {
          // Remove the old document element hieararchy
          rv = state.mFrameManager->RemoveFrame(aPresContext, *shell,
                                                docParentFrame, nsnull, 
                                                docElementFrame);
          if (NS_SUCCEEDED(rv)) {
            // Remove any existing fixed items: they are always on the FixedContainingBlock
            rv = RemoveFixedItems(aPresContext, shell, state.mFrameManager);
            if (NS_SUCCEEDED(rv)) {
              // Create the new document element hierarchy
              nsIFrame*                 newChild;
              nsCOMPtr<nsIStyleContext> rootPseudoStyle;
          
              docParentFrame->GetStyleContext(getter_AddRefs(rootPseudoStyle));
              rv = ConstructDocElementFrame(shell, aPresContext, state, rootContent,
                                            docParentFrame, rootPseudoStyle,
                                            newChild);

              if (NS_SUCCEEDED(rv)) {
                rv = state.mFrameManager->InsertFrames(aPresContext, *shell,
                                                       docParentFrame, nsnull,
                                                       nsnull, newChild);

                // Tell the fixed containing block about its 'fixed' frames
                if (state.mFixedItems.childList) {
                  state.mFrameManager->InsertFrames(aPresContext, *shell,
                                         mFixedContainingBlock, nsLayoutAtoms::fixedList,
                                         nsnull, state.mFixedItems.childList);
                }
              }
            }
          }
        }
      }
    }
  }

  return rv;
}


nsIFrame*
nsCSSFrameConstructor::GetFrameFor(nsIPresShell*    aPresShell,
                                   nsIPresContext*  aPresContext,
                                   nsIContent*      aContent)
{
  // Get the primary frame associated with the content
  nsIFrame* frame;
  aPresShell->GetPrimaryFrameFor(aContent, &frame);

  if (nsnull != frame) {
    // Check to see if the content is a select and 
    // then if it has a drop down (thus making it a combobox)
    // The drop down is a ListControlFrame derived from a 
    // nsScrollFrame then get the area frame and that will be the parent
    // What is unclear here, is if any of this fails, should it return
    // the nsComboboxControlFrame or null?
    nsCOMPtr<nsIDOMHTMLSelectElement> selectElement;
    nsresult res = aContent->QueryInterface(NS_GET_IID(nsIDOMHTMLSelectElement),
                                                 (void**)getter_AddRefs(selectElement));
    if (NS_SUCCEEDED(res) && selectElement) {
      nsIComboboxControlFrame * comboboxFrame;
      res = frame->QueryInterface(NS_GET_IID(nsIComboboxControlFrame),
                                               (void**)&comboboxFrame);
      nsIFrame * listFrame;
      if (NS_SUCCEEDED(res) && comboboxFrame) {
        comboboxFrame->GetDropDown(&listFrame);
      } else {
        listFrame = frame;
      }

      if (listFrame != nsnull) {
        nsIListControlFrame * list;
        res = listFrame->QueryInterface(NS_GET_IID(nsIListControlFrame),
                                                 (void**)&list);
        if (NS_SUCCEEDED(res) && list) {
          list->GetOptionsContainer(aPresContext, &frame);
        } 
      }
    } else {
      // If the primary frame is a scroll frame, then get the scrolled frame.
      // That's the frame that gets the reflow command
      const nsStyleDisplay* display;
      frame->GetStyleData(eStyleStruct_Display, (const nsStyleStruct*&)display);

      // If the primary frame supports IScrollableFrame, then get the scrolled frame.
      // That's the frame that gets the reflow command                          
      nsIScrollableFrame *pScrollableFrame = nsnull;                            
      if (NS_SUCCEEDED( frame->QueryInterface(NS_GET_IID(nsIScrollableFrame),     
                                              (void **)&pScrollableFrame) ))    
      {                                                                         
        pScrollableFrame->GetScrolledFrame( aPresContext, frame );              
      } 

      // if we get an outer table frame use its 1st child which is a table inner frame
      // if we get a table cell frame   use its 1st child which is an area frame
      else if ((NS_STYLE_DISPLAY_TABLE      == display->mDisplay) ||
               (NS_STYLE_DISPLAY_TABLE_CELL == display->mDisplay)) {
        frame->FirstChild(aPresContext, nsnull, &frame);                   
      }
    }
  }

  return frame;
}

nsIFrame*
nsCSSFrameConstructor::GetAbsoluteContainingBlock(nsIPresContext* aPresContext,
                                                  nsIFrame*       aFrame)
{
  NS_PRECONDITION(nsnull != mInitialContainingBlock, "no initial containing block");
  
  // Starting with aFrame, look for a frame that is absolutely positioned or
  // relatively positioned
  nsIFrame* containingBlock = nsnull;
  for (nsIFrame* frame = aFrame; frame; frame->GetParent(&frame)) {
    // Is it positioned?
    // If it's a table then ignore it, because for the time being tables
    // are not containers for absolutely positioned child frames
    const nsStyleDisplay* disp;
    frame->GetStyleData(eStyleStruct_Display, (const nsStyleStruct*&)disp);

    if (disp->IsPositioned() && disp->mDisplay != NS_STYLE_DISPLAY_TABLE) {
      nsCOMPtr<nsIAtom> frameType;
      frame->GetFrameType(getter_AddRefs(frameType));

      if (nsLayoutAtoms::scrollFrame == frameType) {
        // We want the scrolled frame, not either of the two scroll frames
        nsIFrame* scrolledFrame;
        frame->FirstChild(aPresContext, nsnull, &scrolledFrame);
        if (scrolledFrame) {
          scrolledFrame->GetFrameType(getter_AddRefs(frameType));
          if (nsLayoutAtoms::areaFrame == frameType) {
            containingBlock = scrolledFrame;
            break;
          } else if (nsLayoutAtoms::scrollFrame == frameType) {
            scrolledFrame->FirstChild(aPresContext, nsnull, &scrolledFrame);
            if (scrolledFrame) {
              scrolledFrame->GetFrameType(getter_AddRefs(frameType));
              if (nsLayoutAtoms::areaFrame == frameType) {
                containingBlock = scrolledFrame;
                break;
              }
            }
          }
        }

      } else if ((nsLayoutAtoms::areaFrame == frameType) ||
                 (nsLayoutAtoms::positionedInlineFrame == frameType)) {
        containingBlock = frame;
        break;
      }
    }
  }

  // If we didn't find an absolutely positioned containing block, then use the
  // initial containing block
  if (!containingBlock) {
    containingBlock = mInitialContainingBlock;
  }
  
  return containingBlock;
}

nsIFrame*
nsCSSFrameConstructor::GetFloaterContainingBlock(nsIPresContext* aPresContext,
                                                 nsIFrame*       aFrame)
{
  NS_PRECONDITION(mInitialContainingBlock, "no initial containing block");
  
  // Starting with aFrame, look for a frame that is a real block frame,
  // or a floated inline or absolutely positioned inline frame
  nsIFrame* containingBlock = aFrame;
  while (nsnull != containingBlock) {
    const nsStyleDisplay* display;
    containingBlock->GetStyleData(eStyleStruct_Display,
                                  (const nsStyleStruct*&)display);
    if ((NS_STYLE_DISPLAY_BLOCK == display->mDisplay) ||
        (NS_STYLE_DISPLAY_LIST_ITEM == display->mDisplay)) {
      break;
    }
    else if (NS_STYLE_DISPLAY_INLINE == display->mDisplay) {
      if ((NS_STYLE_FLOAT_NONE != display->mFloats) ||
          (display->IsAbsolutelyPositioned())) {
        if (NS_STYLE_FLOAT_NONE != display->mFloats) {
          nsCOMPtr<nsIAtom> frameType;
          containingBlock->GetFrameType(getter_AddRefs(frameType));
          if (nsLayoutAtoms::letterFrame != frameType.get()) {
            break;
          }
        }
        else {
          break;
        }
      }
    }

    // Continue walking up the hierarchy
    containingBlock->GetParent(&containingBlock);
  }

  // If we didn't find a containing block, then use the initial
  // containing block
  if (nsnull == containingBlock) {
    containingBlock = mInitialContainingBlock;
  }
  return containingBlock;
}

// Helper function to determine whether a given frame is generated content
// for the specified content object. Returns PR_TRUE if the frame is associated
// with generated content and PR_FALSE otherwise
static inline PRBool
IsGeneratedContentFor(nsIContent* aContent, nsIFrame* aFrame, nsIAtom* aPseudoElement)
{
  NS_PRECONDITION(aFrame, "null frame pointer");
  nsFrameState  state;
  PRBool        result = PR_FALSE;

  // First check the frame state bit
  aFrame->GetFrameState(&state);
  if (state & NS_FRAME_GENERATED_CONTENT) {
    nsIContent* content;

    // Check that it has the same content pointer
    aFrame->GetContent(&content);
    if (content == aContent) {
      nsIStyleContext* styleContext;
      nsIAtom*         pseudoType;

      // See if the pseudo element type matches
      aFrame->GetStyleContext(&styleContext);
      styleContext->GetPseudoType(pseudoType);
      result = (pseudoType == aPseudoElement);
      NS_RELEASE(styleContext);
      NS_IF_RELEASE(pseudoType);
    }
    NS_IF_RELEASE(content);
  }

  return result;
}

/**
 * This function is called by ContentAppended() and ContentInserted()
 * when appending flowed frames to a parent's principal child list. It
 * handles the case where the parent frame has :after pseudo-element
 * generated content.
 */
nsresult
nsCSSFrameConstructor::AppendFrames(nsIPresContext*  aPresContext,
                                    nsIPresShell*    aPresShell,
                                    nsIFrameManager* aFrameManager,
                                    nsIContent*      aContainer,
                                    nsIFrame*        aParentFrame,
                                    nsIFrame*        aFrameList)
{
  nsIFrame* firstChild;
  aParentFrame->FirstChild(aPresContext, nsnull, &firstChild);
  nsFrameList frames(firstChild);
  nsIFrame* lastChild = frames.LastChild();

  // See if the parent has an :after pseudo-element
  if (lastChild && IsGeneratedContentFor(aContainer, lastChild, nsCSSAtoms::afterPseudo)) {
    // Insert the frames before the :after pseudo-element.
    return aFrameManager->InsertFrames(aPresContext, *aPresShell, aParentFrame,
                                       nsnull, frames.GetPrevSiblingFor(lastChild),
                                       aFrameList);
  }

  nsresult rv = NS_OK;

  // a col group or col appended to a table may result in an insert rather than an append
  nsCOMPtr<nsIAtom> parentType;
  aParentFrame->GetFrameType(getter_AddRefs(parentType));
  if (nsLayoutAtoms::tableFrame == parentType.get()) { 
    nsTableFrame* tableFrame = NS_REINTERPRET_CAST(nsTableFrame*, aParentFrame);
    nsCOMPtr<nsIAtom> childType;
    aFrameList->GetFrameType(getter_AddRefs(childType));
    if (nsLayoutAtoms::tableColFrame == childType.get()) {
      // table column
      nsIFrame* parentFrame = aParentFrame;
      aFrameList->GetParent(&parentFrame);
      rv = aFrameManager->AppendFrames(aPresContext, *aPresShell, parentFrame,
                                       nsLayoutAtoms::colGroupList, aFrameList);
    }
    else if (nsLayoutAtoms::tableColGroupFrame == childType) {
      // table col group
      nsIFrame* prevSibling;
      PRBool doAppend = nsTableColGroupFrame::GetLastRealColGroup(tableFrame, &prevSibling);
      if (doAppend) {
        rv = aFrameManager->AppendFrames(aPresContext, *aPresShell, aParentFrame,
                                         nsLayoutAtoms::colGroupList, aFrameList);
      }
      else {
        rv = aFrameManager->InsertFrames(aPresContext, *aPresShell, aParentFrame, 
                                         nsLayoutAtoms::colGroupList, prevSibling, aFrameList);
      }
    }
    else if (nsLayoutAtoms::tableCaptionFrame == childType) {
      // table caption
      rv = aFrameManager->AppendFrames(aPresContext, *aPresShell, aParentFrame,
                                       nsLayoutAtoms::captionList, aFrameList);
    }
    else {
      rv = aFrameManager->AppendFrames(aPresContext, *aPresShell, aParentFrame,
                                       nsnull, aFrameList);
    }
  }
  else {
    // Append the frames to the end of the parent's child list
    // check for a table caption which goes on an additional child list with a different parent
    nsIFrame* outerTableFrame; 
    if (GetCaptionAdjustedParent(aParentFrame, aFrameList, &outerTableFrame)) {
      rv = aFrameManager->AppendFrames(aPresContext, *aPresShell, outerTableFrame,
                                       nsLayoutAtoms::captionList, aFrameList);
    }
    else {
      rv = aFrameManager->AppendFrames(aPresContext, *aPresShell, aParentFrame,
                                       nsnull, aFrameList);
    }
  }

  return rv;
}

/**
 * Find the ``rightmost'' frame for the anonymous content immediately
 * preceding aChild, following continuation if necessary.
 */
static nsIFrame*
FindPreviousAnonymousSibling(nsIPresShell* aPresShell,
                             nsIDocument*  aDocument,
                             nsIContent*   aContainer,
                             nsIContent*   aChild)
{
  NS_PRECONDITION(aDocument, "null document from content element in FindNextAnonymousSibling");

  nsCOMPtr<nsIDOMDocumentXBL> xblDoc(do_QueryInterface(aDocument));
  NS_ASSERTION(xblDoc, "null xblDoc for content element in FindNextAnonymousSibling");
  if (! xblDoc)
    return nsnull;

  // Grovel through the anonymous elements looking for aChild. We'll
  // start our search for a previous frame there.
  nsCOMPtr<nsIDOMNodeList> nodeList;
  nsCOMPtr<nsIDOMElement> elt(do_QueryInterface(aContainer));
  xblDoc->GetAnonymousNodes(elt, getter_AddRefs(nodeList));

  if (! nodeList)
    return nsnull;

  PRUint32 length;
  nodeList->GetLength(&length);

  PRInt32 index;
  for (index = PRInt32(length) - 1; index >= 0; --index) {
    nsCOMPtr<nsIDOMNode> node;
    nodeList->Item(PRUint32(index), getter_AddRefs(node));

    nsCOMPtr<nsIContent> child = do_QueryInterface(node);
    if (child.get() == aChild)
      break;
  }

  // We want the node immediately before aChild. Keep going until we
  // run off the beginning of the nodeList, or we find a frame.
  while (--index >= 0) {
    nsCOMPtr<nsIDOMNode> node;
    nodeList->Item(PRUint32(index), getter_AddRefs(node));

    nsCOMPtr<nsIContent> child = do_QueryInterface(node);

    // Get its frame. If it doesn't have one, continue on to the
    // anonymous element that preceded it.
    nsIFrame* prevSibling;
    aPresShell->GetPrimaryFrameFor(child, &prevSibling);
    if (prevSibling) {
      // The frame may have a continuation. If so, we want the
      // last-in-flow as our previous sibling.
      nsIFrame* nextInFlow;
      while (1) {
        prevSibling->GetNextInFlow(&nextInFlow);
        if (! nextInFlow)
          break;
        prevSibling = nextInFlow;
      }

      // If the frame is out-of-flow, GPFF() will have returned the
      // out-of-flow frame; we want the placeholder.
      const nsStyleDisplay* display;
      prevSibling->GetStyleData(eStyleStruct_Display,
                                (const nsStyleStruct*&)display);

      if (display->IsFloating() || display->IsAbsolutelyPositioned()) {
        nsIFrame* placeholderFrame;
        aPresShell->GetPlaceholderFrameFor(prevSibling, &placeholderFrame);
        NS_ASSERTION(placeholderFrame, "no placeholder for out-of-flow frame");
        prevSibling = placeholderFrame;
      }

      // Found a previous sibling, we're done!
      return prevSibling;
    }
  }

  return nsnull;
}

/**
 * Find the frame for the anonymous content immediately following
 * aChild.
 */
static nsIFrame*
FindNextAnonymousSibling(nsIPresShell* aPresShell,
                         nsIDocument*  aDocument,
                         nsIContent*   aContainer,
                         nsIContent*   aChild)
{
  NS_PRECONDITION(aDocument, "null document from content element in FindNextAnonymousSibling");

  nsCOMPtr<nsIDOMDocumentXBL> xblDoc(do_QueryInterface(aDocument));
  NS_ASSERTION(xblDoc, "null xblDoc for content element in FindNextAnonymousSibling");
  if (! xblDoc)
    return nsnull;

  // Grovel through the anonymous elements looking for aChild
  nsCOMPtr<nsIDOMNodeList> nodeList;
  nsCOMPtr<nsIDOMElement> elt(do_QueryInterface(aContainer));
  xblDoc->GetAnonymousNodes(elt, getter_AddRefs(nodeList));

  if (! nodeList)
    return nsnull;

  PRUint32 length;
  nodeList->GetLength(&length);

  PRInt32 index;
  for (index = 0; index < PRInt32(length); ++index) {
    nsCOMPtr<nsIDOMNode> node;
    nodeList->Item(PRUint32(index), getter_AddRefs(node));

    nsCOMPtr<nsIContent> child = do_QueryInterface(node);
    if (child.get() == aChild)
      break;
  }

  // We want the node immediately after aChild. Keep going until we
  // run off the end of the nodeList, or we find a next sibling.
  while (++index < PRInt32(length)) {
    nsCOMPtr<nsIDOMNode> node;
    nodeList->Item(PRUint32(index), getter_AddRefs(node));

    nsCOMPtr<nsIContent> child = do_QueryInterface(node);

    // Get its frame
    nsIFrame* nextSibling;
    aPresShell->GetPrimaryFrameFor(child, &nextSibling);
    if (nextSibling) {
#ifdef DEBUG
      // The primary frame should never be a continuation
      nsIFrame* prevInFlow;
      nextSibling->GetPrevInFlow(&prevInFlow);
      NS_ASSERTION(!prevInFlow, "primary frame is a continuation!?");
#endif

      // If the frame is out-of-flow, GPFF() will have returned the
      // out-of-flow frame; we want the placeholder.
      const nsStyleDisplay* display;
      nextSibling->GetStyleData(eStyleStruct_Display,
                                (const nsStyleStruct*&)display);

      if (display->IsFloating() || display->IsAbsolutelyPositioned()) {
        nsIFrame* placeholderFrame;
        aPresShell->GetPlaceholderFrameFor(nextSibling, &placeholderFrame);
        NS_ASSERTION(placeholderFrame, "no placeholder for out-of-flow frame");
        nextSibling = placeholderFrame;
      }

      // Found a next sibling, we're done!
      return nextSibling;
    }
  }

  return nsnull;
}

#define UNSET_DISPLAY 255
// if the sibling is a col group, col or (row group, caption), then aContent 
// must be the same type, otherwise aContent may get the wrong parent.
PRBool
nsCSSFrameConstructor::IsValidSibling(nsIPresShell&          aPresShell,
                                      const nsIFrame&        aSibling,
                                      PRUint8                aSiblingDisplay,
                                      nsIContent&            aContent,
                                      PRUint8&               aDisplay)
{
  if ((NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP == aSiblingDisplay) ||
      (NS_STYLE_DISPLAY_TABLE_COLUMN       == aSiblingDisplay) ||
      (NS_STYLE_DISPLAY_TABLE_HEADER_GROUP == aSiblingDisplay) ||
      (NS_STYLE_DISPLAY_TABLE_ROW_GROUP    == aSiblingDisplay) ||
      (NS_STYLE_DISPLAY_TABLE_FOOTER_GROUP == aSiblingDisplay)) {
    // if we haven't already, construct a style context to find the display type of aContent
    if (UNSET_DISPLAY == aDisplay) {
      nsCOMPtr<nsIPresContext> context;
      aPresShell.GetPresContext(getter_AddRefs(context));

      nsIFrame* parent;
      aSibling.GetParent(&parent);
      nsCOMPtr<nsIStyleContext> styleContext;
      ResolveStyleContext(context, parent, &aContent, getter_AddRefs(styleContext));
      if (!styleContext) return PR_FALSE;
      const nsStyleDisplay* display = 
        (const nsStyleDisplay*) styleContext->GetStyleData(eStyleStruct_Display);
      if (!display) return PR_FALSE;
      aDisplay = display->mDisplay;
    }
    switch (aSiblingDisplay) {
    case NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP:
      return (NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP == aDisplay);
    case NS_STYLE_DISPLAY_TABLE_COLUMN:
      return (NS_STYLE_DISPLAY_TABLE_COLUMN == aDisplay);
    default: // all of the row group types
      return (NS_STYLE_DISPLAY_TABLE_HEADER_GROUP == aDisplay) ||
             (NS_STYLE_DISPLAY_TABLE_ROW_GROUP    == aDisplay) ||
             (NS_STYLE_DISPLAY_TABLE_FOOTER_GROUP == aDisplay) ||
             (NS_STYLE_DISPLAY_TABLE_CAPTION      == aDisplay);
    }
  }
  return PR_TRUE;
}

/**
 * Find the ``rightmost'' frame for the content immediately preceding
 * aIndexInContainer, following continuations if necessary.
 */
nsIFrame*
nsCSSFrameConstructor::FindPreviousSibling(nsIPresShell*     aPresShell,
                                           nsIContent*       aContainer,
                                           PRInt32           aIndexInContainer,
                                           const nsIContent* aChild)
{
  NS_ASSERTION(aPresShell && aContainer, "null arguments");

  ChildIterator first, iter;
  nsresult rv = ChildIterator::Init(aContainer, &first, &iter);
  NS_ENSURE_SUCCESS(rv, nsnull);
  iter.seek(aIndexInContainer);

  PRUint8 childDisplay = UNSET_DISPLAY;
  // Note: not all content objects are associated with a frame (e.g., if it's
  // `display: hidden') so keep looking until we find a previous frame
  while (iter-- != first) {
    nsIFrame* prevSibling = nsnull;
    aPresShell->GetPrimaryFrameFor(nsCOMPtr<nsIContent>(*iter), &prevSibling);

    if (prevSibling) {
      // The frame may have a continuation. Get the last-in-flow
      nsIFrame* nextInFlow;
      while (1) {
        prevSibling->GetNextInFlow(&nextInFlow);
        if (! nextInFlow)
          break;
        prevSibling = nextInFlow;
      }

      // If the frame is out-of-flow, GPFF() will have returned the
      // out-of-flow frame; we want the placeholder.
      // XXXldb Why not check NS_FRAME_OUT_OF_FLOW state bit?
      const nsStyleDisplay* display;
      prevSibling->GetStyleData(eStyleStruct_Display,
                                (const nsStyleStruct*&)display);
  
      if (aChild && !IsValidSibling(*aPresShell, *prevSibling, display->mDisplay, (nsIContent&)*aChild, childDisplay))
        continue;

      if (display->mDisplay == NS_STYLE_DISPLAY_POPUP) {
        nsIFrame* placeholderFrame;
        aPresShell->GetPlaceholderFrameFor(prevSibling, &placeholderFrame);
        // XXXldb Was this supposed to be a null-check of placeholderFrame?
        if (prevSibling)
          prevSibling = placeholderFrame;
      }
      else if (display->IsFloating() || display->IsAbsolutelyPositioned()) {
        nsIFrame* placeholderFrame;
        aPresShell->GetPlaceholderFrameFor(prevSibling, &placeholderFrame);
        NS_ASSERTION(placeholderFrame, "no placeholder for out-of-flow frame");
        prevSibling = placeholderFrame;
      }

#ifdef DEBUG
      nsIFrame* containerFrame = nsnull;
      aPresShell->GetPrimaryFrameFor(aContainer, &containerFrame);
      NS_ASSERTION(prevSibling != containerFrame, "Previous Sibling is the Container's frame");
#endif
      // Found a previous sibling, we're done!
      return prevSibling;
    }
  }

  return nsnull;
}

/**
 * Find the frame for the content node immediately following
 * aIndexInContainer.
 */
nsIFrame*
nsCSSFrameConstructor::FindNextSibling(nsIPresShell*     aPresShell,
                                       nsIContent*       aContainer,
                                       PRInt32           aIndexInContainer,
                                       const nsIContent* aChild)
{
  ChildIterator iter, last;
  nsresult rv = ChildIterator::Init(aContainer, &iter, &last);
  NS_ENSURE_SUCCESS(rv, nsnull);
  iter.seek(aIndexInContainer);

  // Catch the case where someone tries to append
  if (iter == last)
    return nsnull;

  PRUint8 childDisplay = UNSET_DISPLAY;

  while (++iter != last) {
    nsIFrame* nextSibling = nsnull;
    aPresShell->GetPrimaryFrameFor(nsCOMPtr<nsIContent>(*iter), &nextSibling);

    if (nextSibling) {
#ifdef DEBUG
      // The frame primary frame should never be a continuation
      nsIFrame* prevInFlow;
      nextSibling->GetPrevInFlow(&prevInFlow);
      NS_ASSERTION(!prevInFlow, "primary frame is a continuation!?");
#endif

      // If the frame is out-of-flow, GPFF() will have returned the
      // out-of-flow frame; we want the placeholder.
      const nsStyleDisplay* display;
      nextSibling->GetStyleData(eStyleStruct_Display,
                                (const nsStyleStruct*&)display);

      if (aChild && !IsValidSibling(*aPresShell, *nextSibling, display->mDisplay, (nsIContent&)*aChild, childDisplay))
        continue;

      if (display->IsFloating() || display->IsAbsolutelyPositioned()) {
        // Nope. Get the place-holder instead
        nsIFrame* placeholderFrame;
        aPresShell->GetPlaceholderFrameFor(nextSibling, &placeholderFrame);
        NS_ASSERTION(placeholderFrame, "no placeholder for out-of-flow frame");
        nextSibling = placeholderFrame;
      }

      // We found a next sibling, we're done!
      return nextSibling;
    }
  }

  return nsnull;
}

inline PRBool
ShouldIgnoreSelectChild(nsIContent* aContainer)
{
  // Ignore options and optgroups inside a select (size > 1)
  nsCOMPtr<nsIAtom> containerTag;
  aContainer->GetTag(*getter_AddRefs(containerTag));

  if (containerTag == nsHTMLAtoms::optgroup ||
      containerTag == nsHTMLAtoms::select) {
    nsCOMPtr<nsIContent> selectContent = aContainer;
    nsCOMPtr<nsIContent> tmpContent;
    
    while (selectContent) {
      if (containerTag == nsHTMLAtoms::select)
        break;
      
      tmpContent = selectContent;
      tmpContent->GetParent(*getter_AddRefs(selectContent));
      if (selectContent)
        selectContent->GetTag(*getter_AddRefs(containerTag));
    }
    
    nsCOMPtr<nsISelectElement> selectElement = do_QueryInterface(selectContent);
    if (selectElement) {
      nsAutoString selSize;
      aContainer->GetAttr(kNameSpaceID_None, nsHTMLAtoms::size, selSize);
      if (!selSize.IsEmpty()) {
        PRInt32 err;
        return (selSize.ToInteger(&err) > 1);
      }
    }
  }

  return PR_FALSE;
}


NS_IMETHODIMP
nsCSSFrameConstructor::ContentAppended(nsIPresContext* aPresContext,
                                       nsIContent*     aContainer,
                                       PRInt32         aNewIndexInContainer)
{
#ifdef DEBUG
  if (gNoisyContentUpdates) {
    printf("nsCSSFrameConstructor::ContentAppended container=%p index=%d\n",
           NS_STATIC_CAST(void*, aContainer), aNewIndexInContainer);
    if (gReallyNoisyContentUpdates && aContainer) {
      aContainer->List(stdout, 0);
    }
  }
#endif

  nsCOMPtr<nsIPresShell> shell;
  aPresContext->GetShell(getter_AddRefs(shell));

#ifdef INCLUDE_XUL
  if (aContainer) {
    nsCOMPtr<nsIBindingManager> bindingManager;
    mDocument->GetBindingManager(getter_AddRefs(bindingManager));

    nsCOMPtr<nsIAtom> tag;
    PRInt32 namespaceID;
    bindingManager->ResolveTag(aContainer, &namespaceID, getter_AddRefs(tag));

    // Just ignore tree tags, anyway we don't create any frames for them.
    if (tag == nsXULAtoms::treechildren ||
        tag == nsXULAtoms::treeitem ||
        tag == nsXULAtoms::treerow ||
        (UseXBLForms() && ShouldIgnoreSelectChild(aContainer)))
      return NS_OK;

  }
#endif // INCLUDE_XUL

  // Get the frame associated with the content
  nsIFrame* parentFrame = GetFrameFor(shell, aPresContext, aContainer);
  if (! parentFrame)
    return NS_OK;

  // See if we have an XBL insertion point. If so, then that's our
  // real parent frame; if not, then the frame hasn't been built yet
  // and we just bail.
  //
  nsIFrame* insertionPoint;
  PRBool multiple = PR_FALSE;
  GetInsertionPoint(shell, parentFrame, nsnull, &insertionPoint, &multiple);
  if (! insertionPoint)
    return NS_OK; // Don't build the frames.

  PRBool hasInsertion = PR_FALSE;
  if (!multiple) {
    nsCOMPtr<nsIBindingManager> bindingManager;
    nsCOMPtr<nsIDocument> document;
    nsCOMPtr<nsIContent> firstAppendedChild;
    aContainer->ChildAt(aNewIndexInContainer, *getter_AddRefs(firstAppendedChild));
    if (firstAppendedChild) {
      firstAppendedChild->GetDocument(*getter_AddRefs(document));
    }
    if (document)
      document->GetBindingManager(getter_AddRefs(bindingManager));
    if (bindingManager) {
      nsCOMPtr<nsIContent> insParent;
      bindingManager->GetInsertionParent(firstAppendedChild, getter_AddRefs(insParent));
      if (insParent)
        hasInsertion = PR_TRUE;
    }
  }
  
  if (multiple || hasInsertion) {
    // We have an insertion point.  There are some additional tests we need to do
    // in order to ensure that an append is a safe operation.
    PRInt32 childCount = 0;
      
    if (!multiple) {
      // We may need to make multiple ContentInserted calls instead.  A
      // reasonable heuristic to employ (in order to maintain good performance)
      // is to find out if the insertion point's content node contains any
      // explicit children.  If it does not, then it is highly likely that 
      // an append is occurring.  (Note it is not definite, and there are insane
      // cases we will not deal with by employing this heuristic, but it beats
      // always falling back to multiple ContentInserted calls).
      //
      // In the multiple insertion point case, we know we're going to need to do
      // multiple ContentInserted calls anyway.
      nsCOMPtr<nsIContent> content;
      insertionPoint->GetContent(getter_AddRefs(content));
      content->ChildCount(childCount);
    }

    if (multiple || childCount > 0) {
      // Now comes the fun part.  For each appended child, we must obtain its
      // insertion point and find its exact position within that insertion point.
      // We then make a ContentInserted call with the correct computed index.
      nsCOMPtr<nsIContent> insertionContent;
      insertionPoint->GetContent(getter_AddRefs(insertionContent));
      
      PRInt32 containerCount;
      aContainer->ChildCount(containerCount);
      for (PRInt32 i = aNewIndexInContainer; i < containerCount; i++) {
        nsCOMPtr<nsIContent> child;
        aContainer->ChildAt(i, *getter_AddRefs(child));
        if (multiple) {
          // Filters are in effect, so the insertion point needs to be refetched for
          // each child.
          GetInsertionPoint(shell, parentFrame, child, &insertionPoint);
          insertionPoint->GetContent(getter_AddRefs(insertionContent));
        }

        // Construct an iterator to locate this child at its correct index.
        ChildIterator iter, last;
        for (ChildIterator::Init(insertionContent, &iter, &last);
         iter != last;
         ++iter) {
          nsIContent* item = nsCOMPtr<nsIContent>(*iter);
          if (item == child)
            // Call ContentInserted with this index.
            ContentInserted(aPresContext, aContainer, child, iter.index(), mTempFrameTreeState, PR_FALSE);
        }
      }

      return NS_OK;
    }
  }

  parentFrame = insertionPoint;

  // If the frame we are manipulating is a ``special'' frame (that
  // is, one that's been created as a result of a block-in-inline
  // situation) then do something different instead of just
  // appending newly created frames. Note that only the
  // first-in-flow is marked so we check before getting to the
  // last-in-flow.
  //
  // We run into this situation occasionally while loading web
  // pages, typically when some content generation tool has
  // sprinkled invalid markup into the document. More often than
  // not, we'll be able to just use the normal fast-path frame
  // construction code, because the frames will be appended to the
  // ``anonymous'' block that got created to parent the block
  // children of the inline.
  if (IsFrameSpecial(parentFrame)) {
#ifdef DEBUG
    if (gNoisyContentUpdates) {
      printf("nsCSSFrameConstructor::ContentAppended: parentFrame=");
      nsFrame::ListTag(stdout, parentFrame);
      printf(" is special\n");
    }
#endif

    // Since we're appending, we'll walk to the last anonymous frame
    // that was created for the broken inline frame.
    nsCOMPtr<nsIFrameManager> frameManager;
    shell->GetFrameManager(getter_AddRefs(frameManager));

    while (1) {
      nsIFrame* sibling;
      GetSpecialSibling(frameManager, parentFrame, &sibling);
      if (! sibling)
        break;

      parentFrame = sibling;
    }

    // If this frame is the anonymous block frame, then all's well:
    // just append frames as usual.
    const nsStyleDisplay* display;
    parentFrame->GetStyleData(eStyleStruct_Display,
                              NS_REINTERPRET_CAST(const nsStyleStruct*&, display));

    if (NS_STYLE_DISPLAY_BLOCK != display->mDisplay) {
      // Nope, it's an inline, so just reframe the entire stinkin' mess if the
      // content is a block. We _could_ do better here with a little more work...
      // find out if the child is a block or inline, an inline means we don't have to reframe
      nsCOMPtr<nsIContent> child;
      aContainer->ChildAt(aNewIndexInContainer, *getter_AddRefs(child));
      PRBool needReframe = !child;
      if (child && child->IsContentOfType(nsIContent::eELEMENT)) {
        nsCOMPtr<nsIStyleContext> styleContext;
        ResolveStyleContext(aPresContext, parentFrame, child, getter_AddRefs(styleContext)); 
        const nsStyleDisplay* display = 
          (const nsStyleDisplay*) styleContext->GetStyleData(eStyleStruct_Display);  
        // XXX since the block child goes in the last inline of the sacred triad, frames would 
        // need to be moved into the 2nd triad (block) but that is more work, for now bail.
        needReframe = display->IsBlockLevel();
      }
      if (needReframe)
        return ReframeContainingBlock(aPresContext, parentFrame);
    }
  }

  // Get the parent frame's last-in-flow
  nsIFrame* nextInFlow = parentFrame;
  while (nsnull != nextInFlow) {
    parentFrame->GetNextInFlow(&nextInFlow);
    if (nsnull != nextInFlow) {
      parentFrame = nextInFlow;
    }
  }

  // If we didn't process children when we originally created the frame,
  // then don't do any processing now
  nsCOMPtr<nsIAtom>  frameType;
  parentFrame->GetFrameType(getter_AddRefs(frameType));
  if (frameType.get() == nsLayoutAtoms::objectFrame) {
    // This handles APPLET, EMBED, and OBJECT
    return NS_OK;
  }

  // in case the parent frame is an outer table, use the innner table
  // as the parent. If a new child frame is a caption then the outer
  // table frame will be used (below) as the parent.
  if (frameType.get() == nsLayoutAtoms::tableOuterFrame) {
    nsIFrame* innerTable = nsnull;
    parentFrame->FirstChild(aPresContext, nsnull, &innerTable);
    if (innerTable) {
      parentFrame = innerTable;
    }
    else { // should never happen
      return NS_ERROR_FAILURE;
    }
  }

  // Create some new frames
  PRInt32                 count;
  nsIFrame*               firstAppendedFrame = nsnull;
  nsFrameItems            frameItems;
  nsFrameConstructorState state(aPresContext, mFixedContainingBlock,
                                GetAbsoluteContainingBlock(aPresContext, parentFrame),
                                GetFloaterContainingBlock(aPresContext, parentFrame));

  // See if the containing block has :first-letter style applied.
  PRBool haveFirstLetterStyle, haveFirstLineStyle;
  nsIFrame* containingBlock = state.mFloatedItems.containingBlock;
  nsCOMPtr<nsIContent> blockContent;
  nsCOMPtr<nsIStyleContext> blockSC;
  containingBlock->GetStyleContext(getter_AddRefs(blockSC));
  containingBlock->GetContent(getter_AddRefs(blockContent));
  HaveSpecialBlockStyle(aPresContext, blockContent, blockSC,
                        &haveFirstLetterStyle,
                        &haveFirstLineStyle);

  if (haveFirstLetterStyle) {
    // Before we get going, remove the current letter frames
    RemoveLetterFrames(aPresContext, state.mPresShell,
                       state.mFrameManager, containingBlock);
  }

  // if the container is a table and a caption was appended, it needs to be put in
  // the outer table frame's additional child list. 
  nsFrameItems captionItems; 

  PRInt32 i;
  aContainer->ChildCount(count);
  for (i = aNewIndexInContainer; i < count; i++) {
    nsCOMPtr<nsIContent> childContent;
    aContainer->ChildAt(i, *getter_AddRefs(childContent));
      
    // construct a child of a table frame by putting it on a temporary list and then
    // moving it into the appropriate list. This is more efficient than checking the display
    // type of childContent. During the construction of a caption frame, the outer 
    // table frame will be used as its parent.
    if (nsLayoutAtoms::tableFrame == frameType.get()) {
      nsFrameItems tempItems;
      ConstructFrame(shell, aPresContext, state, childContent, parentFrame, tempItems);
      if (tempItems.childList) {
        nsCOMPtr<nsIAtom> childFrameType;
        tempItems.childList->GetFrameType(getter_AddRefs(childFrameType));
        if (nsLayoutAtoms::tableCaptionFrame == childFrameType.get()) {
          PRBool abortCaption = PR_FALSE;
          // check if a caption already exists in captionItems, and if so, abort the caption
          if (captionItems.childList) {
            abortCaption = PR_TRUE;
          }
          else {
            // check for a caption already appended to the outer table
            nsIFrame* outerTable;
            parentFrame->GetParent(&outerTable);
            if (outerTable) { 
              nsIFrame* existingCaption = nsnull;
              outerTable->FirstChild(aPresContext, nsLayoutAtoms::captionList, &existingCaption); 
              if (existingCaption) {
                abortCaption = PR_TRUE;
              }
            }
          }
          if (abortCaption) {
            tempItems.childList->Destroy(aPresContext);
          }
          else {
            captionItems.AddChild(tempItems.childList);
          }
        }
        else {
          frameItems.AddChild(tempItems.childList);
        }
      }
    }
    // Don't create child frames for iframes/frames, they should not
    // display any content that they contain.
    else if (nsLayoutAtoms::htmlFrameOuterFrame != frameType.get()) {
      // Construct a child frame (that does not have a table as parent)
      ConstructFrame(shell, aPresContext, state, childContent, parentFrame, frameItems);
    }
  }

  // We built some new frames.  Initialize any newly-constructed bindings.
  nsCOMPtr<nsIBindingManager> bm;
  mDocument->GetBindingManager(getter_AddRefs(bm));
  bm->ProcessAttachedQueue();

  // process the current pseudo frame state
  if (!state.mPseudoFrames.IsEmpty()) {
    ProcessPseudoFrames(aPresContext, state.mPseudoFrames, frameItems);
  }

  if (haveFirstLineStyle) {
    // It's possible that some of the new frames go into a
    // first-line frame. Look at them and see...
    AppendFirstLineFrames(shell, aPresContext, state, aContainer, parentFrame,
                          frameItems); 
  }

  nsresult result = NS_OK;
  firstAppendedFrame = frameItems.childList;
  if (!firstAppendedFrame) {
    firstAppendedFrame = captionItems.childList;
  }

  // Notify the parent frame passing it the list of new frames
  if (NS_SUCCEEDED(result) && firstAppendedFrame) {
    // Perform special check for diddling around with the frames in
    // a special inline frame.

    // XXX Bug 18366
    // Although select frame are inline we do not want to call
    // WipeContainingBlock because it will throw away the entire selct frame and 
    // start over which is something we do not want to do
    //
    nsCOMPtr<nsIDOMHTMLSelectElement> selectContent(do_QueryInterface(aContainer));
    if (!selectContent) {
      if (WipeContainingBlock(aPresContext, state, blockContent, parentFrame,
                              frameItems.childList)) {
        return NS_OK;
      }
    }

    // Append the flowed frames to the principal child list, tables need special treatment
    if (nsLayoutAtoms::tableFrame == frameType.get()) {
      if (captionItems.childList) { // append the caption to the outer table
        nsIFrame* outerTable;
        parentFrame->GetParent(&outerTable);
        if (outerTable) { 
          AppendFrames(aPresContext, shell, state.mFrameManager, aContainer,
                       outerTable, captionItems.childList); 
        }
      }
      if (frameItems.childList) { // append children of the inner table
        AppendFrames(aPresContext, shell, state.mFrameManager, aContainer,
                     parentFrame, frameItems.childList);
      }
    }
    else {
      AppendFrames(aPresContext, shell, state.mFrameManager, aContainer,
                   parentFrame, firstAppendedFrame);
    }

    // If there are new absolutely positioned child frames, then notify
    // the parent
    // XXX We can't just assume these frames are being appended, we need to
    // determine where in the list they should be inserted...
    if (state.mAbsoluteItems.childList) {
      state.mAbsoluteItems.containingBlock->AppendFrames(aPresContext, *shell,
                                                         nsLayoutAtoms::absoluteList,
                                                         state.mAbsoluteItems.childList);
    }

    // If there are new fixed positioned child frames, then notify
    // the parent
    // XXX We can't just assume these frames are being appended, we need to
    // determine where in the list they should be inserted...
    if (state.mFixedItems.childList) {
      state.mFixedItems.containingBlock->AppendFrames(aPresContext, *shell,
                                                      nsLayoutAtoms::fixedList,
                                                      state.mFixedItems.childList);
    }

    // If there are new floating child frames, then notify
    // the parent
    // XXX We can't just assume these frames are being appended, we need to
    // determine where in the list they should be inserted...
    if (state.mFloatedItems.childList) {
      state.mFloatedItems.containingBlock->AppendFrames(aPresContext, *shell,
                                                        nsLayoutAtoms::floaterList,
                                                        state.mFloatedItems.childList);
    }

    // Recover first-letter frames
    if (haveFirstLetterStyle) {
      RecoverLetterFrames(shell, aPresContext, state, containingBlock);
    }
  }

  // Here we have been notified that content has been appended so if
  // the select now has a single item we need to go in and removed
  // the dummy frame.
  nsCOMPtr<nsIDOMHTMLSelectElement> sel(do_QueryInterface(aContainer));
  if (sel) {
    nsCOMPtr<nsIContent> childContent;
    aContainer->ChildAt(aNewIndexInContainer, *getter_AddRefs(childContent));
    if (childContent) {
      RemoveDummyFrameFromSelect(aPresContext, shell, aContainer,
                                 childContent, sel);
    }
  } 

#ifdef DEBUG
  if (gReallyNoisyContentUpdates) {
    nsIFrameDebug* fdbg = nsnull;
    CallQueryInterface(parentFrame, &fdbg);
    if (fdbg) {
      printf("nsCSSFrameConstructor::ContentAppended: resulting frame model:\n");
      fdbg->List(aPresContext, stdout, 0);
    }
  }
#endif

  return NS_OK;
}


nsresult
nsCSSFrameConstructor::AddDummyFrameToSelect(nsIPresContext*  aPresContext,
                                        nsIPresShell*    aPresShell,
                                        nsFrameConstructorState& aState,
                                        nsIFrame*        aListFrame,
                                        nsIFrame*        aParentFrame,
                                        nsFrameItems*    aChildItems,
                                        nsIContent*      aContainer,
                                        nsIDOMHTMLSelectElement* aSelectElement)
{
  PRUint32 numOptions = 0;
  nsresult rv = aSelectElement->GetLength(&numOptions);
  if (NS_SUCCEEDED(rv) && 0 == numOptions) {
    nsISelectControlFrame* listFrame = nsnull;
    CallQueryInterface(aListFrame, &listFrame);
    if (listFrame) {
      nsIFrame* dummyFrame;
      listFrame->GetDummyFrame(&dummyFrame);

      if (!dummyFrame) {
        nsCOMPtr<nsIStyleContext> styleContext;
        nsIFrame*         generatedFrame = nsnull;
        aParentFrame->GetStyleContext(getter_AddRefs(styleContext));
        if (CreateGeneratedContentFrame(aPresShell, aPresContext, aState,
                                        aParentFrame, aContainer,
                                        styleContext,
                                        nsLayoutAtoms::dummyOptionPseudo,
                                        nsnull, &generatedFrame)) {
          // Add the generated frame to the child list
          if (aChildItems) {
            aChildItems->AddChild(generatedFrame);
          } else {
            nsCOMPtr<nsIFrameManager> frameManager;
            aPresShell->GetFrameManager(getter_AddRefs(frameManager));
            if (frameManager) {
              frameManager->AppendFrames(aPresContext, *aPresShell,
                                         aParentFrame, nsnull, generatedFrame);
            }
          }

          listFrame->SetDummyFrame(generatedFrame);
          return NS_OK;
        }
      }
    }
  }

  return NS_ERROR_FAILURE;
}

// defined below
static nsresult
DeletingFrameSubtree(nsIPresContext*  aPresContext,
                     nsIPresShell*    aPresShell,
                     nsIFrameManager* aFrameManager,
                     nsIFrame*        aFrame);

nsresult
nsCSSFrameConstructor::RemoveDummyFrameFromSelect(nsIPresContext* aPresContext,
                                                  nsIPresShell *  aPresShell,
                                                  nsIContent*     aContainer,
                                                  nsIContent*     aChild,
                                                  nsIDOMHTMLSelectElement * aSelectElement)
{
  // Check to see if this is the first thing we have added to this frame.
  
  PRUint32 numOptions = 0;
  nsresult rv = aSelectElement->GetLength(&numOptions);
  if (NS_SUCCEEDED(rv) && numOptions > 0) {
    nsIFrame* frame;
    aPresShell->GetPrimaryFrameFor(aContainer, &frame);
    if (frame) {
      nsISelectControlFrame* listFrame = nsnull;
      CallQueryInterface(frame, &listFrame);

      if (listFrame) {
        nsIFrame* dummyFrame;
        listFrame->GetDummyFrame(&dummyFrame);

        if (dummyFrame) {
          listFrame->SetDummyFrame(nsnull);

          // get the child's parent frame (which ought to be the list frame)
          nsIFrame* parentFrame;
          dummyFrame->GetParent(&parentFrame);

          nsCOMPtr<nsIFrameManager> frameManager;
          aPresShell->GetFrameManager(getter_AddRefs(frameManager));
          DeletingFrameSubtree(aPresContext, aPresShell, frameManager,
                               dummyFrame);
          frameManager->RemoveFrame(aPresContext, *aPresShell,
                                    parentFrame, nsnull, dummyFrame);
          return NS_OK;
        }
      }
    }
  }

  return NS_ERROR_FAILURE;
}

// Return TRUE if the insertion of aChild into aParent1,2 should force a reframe. aParent1 is 
// the special inline container which contains a block. aParentFrame is approximately aParent1's
// primary frame and will be set to the correct parent of aChild if a reframe is not necessary. 
// aParent2 is aParentFrame's content. aPrevSibling will be set to the correct prev sibling of
// aChild if a reframe is not necessary.
PRBool
nsCSSFrameConstructor::NeedSpecialFrameReframe(nsIPresShell*   aPresShell,
                                               nsIPresContext* aPresContext,
                                               nsIContent*     aParent1,     
                                               nsIContent*     aParent2,     
                                               nsIFrame*&      aParentFrame, 
                                               nsIContent*     aChild,
                                               PRInt32         aIndexInContainer,
                                               nsIFrame*&      aPrevSibling,
                                               nsIFrame*       aNextSibling)
{
  NS_ENSURE_TRUE(aPrevSibling || aNextSibling, PR_TRUE);

  if (!IsInlineFrame2(aParentFrame)) 
    return PR_FALSE;

  // find out if aChild is a block or inline
  PRBool childIsBlock = PR_FALSE;
  if (aChild->IsContentOfType(nsIContent::eELEMENT)) {
    nsCOMPtr<nsIStyleContext> styleContext;
    ResolveStyleContext(aPresContext, aParentFrame, aChild, getter_AddRefs(styleContext)); 
    const nsStyleDisplay* display = 
      (const nsStyleDisplay*) styleContext->GetStyleData(eStyleStruct_Display);  
    childIsBlock = display->IsBlockLevel();
  }
  nsIFrame* prevParent; // parent of prev sibling
  nsIFrame* nextParent; // parent of next sibling

  if (childIsBlock) { 
    if (aPrevSibling) {
      aPrevSibling->GetParent(&prevParent); 
      NS_ASSERTION(prevParent, "program error - null parent frame");
      if (IsInlineFrame2(prevParent)) { // prevParent is an inline
        // XXX we need to find out if prevParent is the 1st inline or the last. If it
        // is the 1st, then aChild and the frames after aPrevSibling within the 1st inline
        // need to be moved to the block(inline). If it is the last, then aChild and the
        // frames before aPrevSibling within the last need to be moved to the block(inline). 
        return PR_TRUE; // For now, bail.
      }        
      aParentFrame = prevParent; // prevParent is a block, put aChild there
    }
    else {
      nsIFrame* nextSibling = (aIndexInContainer >= 0)
                              ? FindNextSibling(aPresShell, aParent2, aIndexInContainer)
                              : FindNextAnonymousSibling(aPresShell, mDocument, aParent1, aChild);
      if (nextSibling) {
        nextSibling->GetParent(&nextParent); 
        NS_ASSERTION(nextParent, "program error - null parent frame");
        if (IsInlineFrame2(nextParent)) {
          // XXX we need to move aChild, aNextSibling and all the frames after aNextSibling within
          // the 1st inline to the block(inline).
          return PR_TRUE; // for now, bail
        }
        // put aChild in nextParent which is the block(inline) and leave aPrevSibling null
        aParentFrame = nextParent;
      }
    }           
  }
  else { // aChild is an inline
    if (aPrevSibling) {
      aPrevSibling->GetParent(&prevParent); 
      NS_ASSERTION(prevParent, "program error - null parent frame");
      if (IsInlineFrame2(prevParent)) { // prevParent is an inline
        // aChild goes into the same inline frame as aPrevSibling
        aPrevSibling->GetParent(&aParentFrame);
        NS_ASSERTION(aParentFrame, "program error - null parent frame");
      }
      else { // prevParent is a block
        nsIFrame* nextSibling = (aIndexInContainer >= 0)
                                ? FindNextSibling(aPresShell, aParent2, aIndexInContainer)
                                : FindNextAnonymousSibling(aPresShell, mDocument, aParent1, aChild);
        if (nextSibling) {
          nextSibling->GetParent(&nextParent);
          NS_ASSERTION(nextParent, "program error - null parent frame");
          if (IsInlineFrame2(nextParent)) {
            // nextParent is the ending inline frame. Put aChild there and
            // set aPrevSibling to null so aChild is its first element.
            nextSibling->GetParent(&aParentFrame); 
            NS_ASSERTION(aParentFrame, "program error - null parent frame");
            aPrevSibling = nsnull; 
          }
          else { // nextParent is a block
            // prevParent and nextParent should be the same, and aChild goes there
            NS_ASSERTION(prevParent == nextParent, "special frame error");
            aParentFrame = prevParent;
          }
        }
        else { 
          // there is no ending enline frame (which should never happen) but aChild needs to go 
          // there, so for now just bail and force a reframe.
          NS_ASSERTION(PR_FALSE, "no last inline frame");
          return PR_TRUE;
        }
      }
    }
    // else aChild goes into the 1st inline frame which is aParentFrame
  }
  return PR_FALSE;
}

NS_IMETHODIMP
nsCSSFrameConstructor::ContentInserted(nsIPresContext*        aPresContext,
                                       nsIContent*            aContainer,
                                       nsIContent*            aChild,
                                       PRInt32                aIndexInContainer,
                                       nsILayoutHistoryState* aFrameState,
                                       PRBool                 aInContentReplaced)
{
  // XXXldb Do we need to re-resolve style to handle the CSS2 + combinator and
  // the :empty pseudo-class?

#ifdef DEBUG
  if (gNoisyContentUpdates) {
    printf("nsCSSFrameConstructor::ContentInserted container=%p child=%p index=%d\n",
           NS_STATIC_CAST(void*, aContainer),
           NS_STATIC_CAST(void*, aChild),
           aIndexInContainer);
    if (gReallyNoisyContentUpdates) {
      (aContainer ? aContainer : aChild)->List(stdout, 0);
    }
  }
#endif

  nsCOMPtr<nsIPresShell> shell;
  aPresContext->GetShell(getter_AddRefs(shell));
  nsresult rv = NS_OK;

#ifdef INCLUDE_XUL
  if (aContainer) {
    nsCOMPtr<nsIBindingManager> bindingManager;
    mDocument->GetBindingManager(getter_AddRefs(bindingManager));

    nsCOMPtr<nsIAtom> tag;
    aChild->GetTag(*getter_AddRefs(tag));

    PRBool listitem = tag && tag.get() == nsXULAtoms::listitem;
    if (listitem) {
      // XBL insertion may not yet have taken place here (in the case of 
      // replacements), so our only connection to the listbox content is
      // through aContainer.  Use the boxObject to get to the listboxbody
      // frame so we can notify it of the insertion
      nsCOMPtr<nsIDOMXULElement> xulEl(do_QueryInterface(aContainer));
      if (xulEl) {
        nsCOMPtr<nsIBoxObject> boxObject;
        xulEl->GetBoxObject(getter_AddRefs(boxObject));
        nsCOMPtr<nsIListBoxObject> listBoxObject(do_QueryInterface(boxObject));
        nsIListBoxObject* bodyBoxObject = nsnull;
        listBoxObject->GetListboxBody(&bodyBoxObject);
        nsListBoxBodyFrame* listBoxBody = NS_STATIC_CAST(nsListBoxBodyFrame*, bodyBoxObject);
        NS_RELEASE(bodyBoxObject);
        if (listBoxBody)
          listBoxBody->OnContentInserted(aPresContext, aChild);
      }
      return NS_OK;
    }

    PRInt32 namespaceID;
    bindingManager->ResolveTag(aContainer, &namespaceID, getter_AddRefs(tag));

    // Just ignore tree tags, anyway we don't create any frames for them.
    if (tag == nsXULAtoms::treechildren ||
        tag == nsXULAtoms::treeitem ||
        tag == nsXULAtoms::treerow ||
        (UseXBLForms() && ShouldIgnoreSelectChild(aContainer)))
      return NS_OK;

  }
#endif // INCLUDE_XUL
  
  // If we have a null parent, then this must be the document element
  // being inserted
  if (! aContainer) {
    nsCOMPtr<nsIContent> docElement;
    mDocument->GetRootContent(getter_AddRefs(docElement));

    if (aChild == docElement.get()) {
      NS_PRECONDITION(nsnull == mInitialContainingBlock, "initial containing block already created");
      
      if (!mDocElementContainingBlock)
        return NS_OK; // We get into this situation when an XBL binding is asynchronously
                      // applied to the root tag (e.g., <window> in XUL).  It's ok.  We can
                      // just bail here because the root will really be built later during
                      // InitialReflow.

      // Get the style context of the containing block frame
      nsCOMPtr<nsIStyleContext> containerStyle;
      mDocElementContainingBlock->GetStyleContext(getter_AddRefs(containerStyle));
    
      // Create frames for the document element and its child elements
      nsIFrame*               docElementFrame;
      nsFrameConstructorState state(aPresContext, mFixedContainingBlock, nsnull, nsnull, aFrameState);
      ConstructDocElementFrame(shell, aPresContext, 
                               state,
                               docElement, 
                               mDocElementContainingBlock,
                               containerStyle, 
                               docElementFrame);
    
      // Set the initial child list for the parent
      mDocElementContainingBlock->SetInitialChildList(aPresContext, 
                                                      nsnull, 
                                                      docElementFrame);
    
      // Tell the fixed containing block about its 'fixed' frames
      if (state.mFixedItems.childList) {
        mFixedContainingBlock->SetInitialChildList(aPresContext, 
                                                   nsLayoutAtoms::fixedList,
                                                   state.mFixedItems.childList);
      }

#ifdef DEBUG
      if (gReallyNoisyContentUpdates && docElementFrame) {
        nsIFrameDebug* fdbg = nsnull;
        CallQueryInterface(docElementFrame, &fdbg);
        if (fdbg) {
          printf("nsCSSFrameConstructor::ContentInserted: resulting frame model:\n");
          fdbg->List(aPresContext, stdout, 0);
        }
      }
#endif
    }

    nsCOMPtr<nsIBindingManager> bm;
    mDocument->GetBindingManager(getter_AddRefs(bm));
    bm->ProcessAttachedQueue();

    // otherwise this is not a child of the root element, and we
    // won't let it have a frame.
    return NS_OK;
  }

  // Otherwise, we've got parent content. Find its frame.
  nsIFrame* parentFrame = GetFrameFor(shell, aPresContext, aContainer);
  if (! parentFrame)
    return NS_OK; // XXXwaterson will this break selects? (See ``Here
                  // we have been notified...'' below.)

  // See if we have an XBL insertion point. If so, then that's our
  // real parent frame; if not, then the frame hasn't been built yet
  // and we just bail.
  nsIFrame* insertionPoint;
  GetInsertionPoint(shell, parentFrame, aChild, &insertionPoint);
  if (! insertionPoint)
    return NS_OK; // Don't build the frames.

  parentFrame = insertionPoint;

  // Find the frame that precedes the insertion point. Walk backwards
  // from the parent frame to get the parent content, because if an
  // XBL insertion point is involved, we'll need to use _that_ to find
  // the preceding frame.
  nsCOMPtr<nsIContent> container;
  parentFrame->GetContent(getter_AddRefs(container));

  // XXX if the insertionPoint was different from the original
  // parentFrame, then aIndexInContainer is most likely completely
  // wrong. What we need to do here is remember the original index,
  // then as we insert, search the child list where we're about to put
  // the new frame to make sure that it appears after any siblings
  // with a lower index, and before any siblings with a higher
  // index. Same with FindNextSibling(), below.
  nsIFrame* prevSibling = (aIndexInContainer >= 0)
    ? FindPreviousSibling(shell, container, aIndexInContainer, aChild)
    : FindPreviousAnonymousSibling(shell, mDocument, aContainer, aChild);

  PRBool    isAppend = PR_FALSE;
  nsIFrame* nextSibling = nsnull;
    
  // If there is no previous sibling, then find the frame that follows
  if (! prevSibling) {
    nextSibling = (aIndexInContainer >= 0)
      ? FindNextSibling(shell, container, aIndexInContainer, aChild)
      : FindNextAnonymousSibling(shell, mDocument, aContainer, aChild);
  }

  PRBool handleSpecialFrame = IsFrameSpecial(parentFrame) && !aInContentReplaced;

  // Now, find the geometric parent so that we can handle
  // continuations properly. Use the prev sibling if we have it;
  // otherwise use the next sibling.
  if (prevSibling) {
    if (!handleSpecialFrame)
      prevSibling->GetParent(&parentFrame);
  }
  else if (nextSibling) {
    if (!handleSpecialFrame)
      nextSibling->GetParent(&parentFrame);
  }
  else {
    // No previous or next sibling, so treat this like an appended frame.
    isAppend = PR_TRUE;
      
    // If we didn't process children when we originally created the frame,
    // then don't do any processing now
    nsCOMPtr<nsIAtom>  frameType;
    parentFrame->GetFrameType(getter_AddRefs(frameType));
    if (frameType.get() == nsLayoutAtoms::objectFrame) {
      // This handles APPLET, EMBED, and OBJECT
      return NS_OK;
    }
  }

  // If the frame we are manipulating is a special frame then see if we need to reframe 
  // NOTE: if we are in ContentReplaced, then don't reframe as we are already doing just that!
  if (handleSpecialFrame) {
    // a special inline frame has propagated some of its children upward to be children 
    // of the block and those frames may need to move around. Sometimes we may need to reframe
#ifdef DEBUG
    if (gNoisyContentUpdates) {
      printf("nsCSSFrameConstructor::ContentInserted: parentFrame=");
      nsFrame::ListTag(stdout, parentFrame);
      printf(" is special\n");
    }
#endif
    // if we don't need to reframe then set parentFrame and prevSibling to the correct values
    if (NeedSpecialFrameReframe(shell, aPresContext, aContainer, container, parentFrame, 
                                aChild, aIndexInContainer, prevSibling, nextSibling)) {
      return ReframeContainingBlock(aPresContext, parentFrame);
    }
  }

  nsFrameItems            frameItems;
  nsFrameConstructorState state(aPresContext, mFixedContainingBlock,
                                GetAbsoluteContainingBlock(aPresContext, parentFrame),
                                GetFloaterContainingBlock(aPresContext, parentFrame),
                                aFrameState);

  // Recover state for the containing block - we need to know if
  // it has :first-letter or :first-line style applied to it. The
  // reason we care is that the internal structure in these cases
  // is not the normal structure and requires custom updating
  // logic.
  nsIFrame* containingBlock = state.mFloatedItems.containingBlock;
  nsCOMPtr<nsIStyleContext> blockSC;
  nsCOMPtr<nsIContent> blockContent;
  PRBool haveFirstLetterStyle = PR_FALSE;
  PRBool haveFirstLineStyle = PR_FALSE;

  // In order to shave off some cycles, we only dig up the
  // containing block haveFirst* flags if the parent frame where
  // the insertion/append is occuring is an inline or block
  // container. For other types of containers this isn't relevant.
  const nsStyleDisplay* parentDisplay;
  parentFrame->GetStyleData(eStyleStruct_Display,
                            (const nsStyleStruct*&)parentDisplay);

  // Examine the parentFrame where the insertion is taking
  // place. If its a certain kind of container then some special
  // processing is done.
  if ((NS_STYLE_DISPLAY_BLOCK == parentDisplay->mDisplay) ||
      (NS_STYLE_DISPLAY_LIST_ITEM == parentDisplay->mDisplay) ||
      (NS_STYLE_DISPLAY_INLINE == parentDisplay->mDisplay) ||
      (NS_STYLE_DISPLAY_INLINE_BLOCK == parentDisplay->mDisplay)) {
    // Recover the special style flags for the containing block
    containingBlock->GetStyleContext(getter_AddRefs(blockSC));
    containingBlock->GetContent(getter_AddRefs(blockContent));
    HaveSpecialBlockStyle(aPresContext, blockContent, blockSC,
                          &haveFirstLetterStyle,
                          &haveFirstLineStyle);

    if (haveFirstLetterStyle) {
      // Get the correct parentFrame and prevSibling - if a
      // letter-frame is present, use its parent.
      nsCOMPtr<nsIAtom> parentFrameType;
      parentFrame->GetFrameType(getter_AddRefs(parentFrameType));
      if (parentFrameType.get() == nsLayoutAtoms::letterFrame) {
        if (prevSibling)
          prevSibling = parentFrame;

        parentFrame->GetParent(&parentFrame);
      }

      // Remove the old letter frames before doing the insertion
      RemoveLetterFrames(aPresContext, state.mPresShell,
                         state.mFrameManager,
                         state.mFloatedItems.containingBlock);

      // Check again to see if the frame we are manipulating is part
      // of a block-in-inline hierarchy.
      if (IsFrameSpecial(parentFrame)) {
        nsCOMPtr<nsIContent> parentContainer;
        blockContent->GetParent(*getter_AddRefs(parentContainer));
#ifdef DEBUG
        if (gNoisyContentUpdates) {
          printf("nsCSSFrameConstructor::ContentInserted: parentFrame=");
          nsFrame::ListTag(stdout, parentFrame);
          printf(" is special inline\n");
          printf("  ==> blockContent=%p, parentContainer=%p\n",
                 NS_STATIC_CAST(void*, blockContent),
                 NS_STATIC_CAST(void*, parentContainer));
        }
#endif
        if (parentContainer) {
          PRInt32 ix;
          parentContainer->IndexOf(blockContent, ix);
          ContentReplaced(aPresContext, parentContainer, blockContent, blockContent, ix);
        }
        else {
          // XXX uh oh. the block that needs reworking has no parent...
          NS_NOTREACHED("block that needs recreation has no parent");
        }

        return NS_OK;
      }
    }
  }

  ConstructFrame(shell, aPresContext, state, aChild, parentFrame, frameItems);

  // Now that we've created frames, run the attach queue.
  //XXXwaterson should we do this after we've processed pseudos, too?
  nsCOMPtr<nsIBindingManager> bm;
  mDocument->GetBindingManager(getter_AddRefs(bm));
  bm->ProcessAttachedQueue();

  // process the current pseudo frame state
  if (!state.mPseudoFrames.IsEmpty())
    ProcessPseudoFrames(aPresContext, state.mPseudoFrames, frameItems);

  // XXX Bug 19949
  // Although select frame are inline we do not want to call
  // WipeContainingBlock because it will throw away the entire select frame and 
  // start over which is something we do not want to do
  //
  nsCOMPtr<nsIDOMHTMLSelectElement> selectContent = do_QueryInterface(aContainer);
  if (!selectContent) {
    // Perform special check for diddling around with the frames in
    // a special inline frame.
    if (WipeContainingBlock(aPresContext, state, blockContent, parentFrame, frameItems.childList))
      return NS_OK;
  }

  if (haveFirstLineStyle) {
    // It's possible that the new frame goes into a first-line
    // frame. Look at it and see...
    if (isAppend) {
      // Use append logic when appending
      AppendFirstLineFrames(shell, aPresContext, state, aContainer, parentFrame,
                            frameItems); 
    }
    else {
      // Use more complicated insert logic when inserting
      InsertFirstLineFrames(aPresContext, state, aContainer,
                            containingBlock, &parentFrame,
                            prevSibling, frameItems);
    }
  }
      
  nsIFrame* newFrame = frameItems.childList;
  if (NS_SUCCEEDED(rv) && newFrame) {
    // Notify the parent frame
    if (isAppend) {
      AppendFrames(aPresContext, shell, state.mFrameManager,
                   aContainer, parentFrame, newFrame);
    }
    else {
      if (!prevSibling) {
        // We're inserting the new frame as the first child. See if the
        // parent has a :before pseudo-element
        nsIFrame* firstChild;
        parentFrame->FirstChild(aPresContext, nsnull, &firstChild);

        if (firstChild && IsGeneratedContentFor(aContainer, firstChild, nsCSSAtoms::beforePseudo)) {
          // Insert the new frames after the :before pseudo-element
          prevSibling = firstChild;
        }
      }

      // check for a table caption which goes on an additional child list
      nsIFrame* outerTableFrame; 
      if (GetCaptionAdjustedParent(parentFrame, newFrame, &outerTableFrame)) {
        // XXXwaterson this seems wrong; i.e., how can we assume
        // that appending is the right thing to do here?
        state.mFrameManager->AppendFrames(aPresContext, *shell, outerTableFrame,
                                          nsLayoutAtoms::captionList, newFrame);
      }
      else {
        state.mFrameManager->InsertFrames(aPresContext, *shell, parentFrame,
                                          nsnull, prevSibling, newFrame);
      }
    }
        
    // If there are new absolutely positioned child frames, then notify
    // the parent
    // XXX We can't just assume these frames are being appended, we need to
    // determine where in the list they should be inserted...
    if (state.mAbsoluteItems.childList) {
      state.mAbsoluteItems.containingBlock->AppendFrames(aPresContext, *shell,
                                                         nsLayoutAtoms::absoluteList,
                                                         state.mAbsoluteItems.childList);
    }
        
    // If there are new fixed positioned child frames, then notify
    // the parent
    // XXX We can't just assume these frames are being appended, we need to
    // determine where in the list they should be inserted...
    if (state.mFixedItems.childList) {
      state.mFixedItems.containingBlock->AppendFrames(aPresContext, *shell,
                                                      nsLayoutAtoms::fixedList,
                                                      state.mFixedItems.childList);
    }
        
    // If there are new floating child frames, then notify
    // the parent
    // XXX We can't just assume these frames are being appended, we need to
    // determine where in the list they should be inserted...
    if (state.mFloatedItems.childList) {
      state.mFloatedItems.containingBlock->AppendFrames(aPresContext, *shell,
                                                        nsLayoutAtoms::floaterList,
                                                        state.mFloatedItems.childList);
    }

    if (haveFirstLetterStyle) {
      // Recover the letter frames for the containing block when
      // it has first-letter style.
      RecoverLetterFrames(shell, aPresContext, state,
                          state.mFloatedItems.containingBlock);
    }
  }

  // Here we have been notified that content has been insert
  // so if the select now has a single item 
  // we need to go in and removed the dummy frame
  nsCOMPtr<nsIDOMHTMLSelectElement> selectElement = do_QueryInterface(aContainer);
  if (selectElement)
    RemoveDummyFrameFromSelect(aPresContext, shell, aContainer, aChild, selectElement);

#ifdef DEBUG
  if (gReallyNoisyContentUpdates && parentFrame) {
    nsIFrameDebug* fdbg = nsnull;
    CallQueryInterface(parentFrame, &fdbg);
    if (fdbg) {
      printf("nsCSSFrameConstructor::ContentInserted: resulting frame model:\n");
      fdbg->List(aPresContext, stdout, 0);
    }
  }
#endif

  return NS_OK;
}

NS_IMETHODIMP
nsCSSFrameConstructor::ContentReplaced(nsIPresContext* aPresContext,
                                       nsIContent*     aContainer,
                                       nsIContent*     aOldChild,
                                       nsIContent*     aNewChild,
                                       PRInt32         aIndexInContainer)
{
  // XXX For now, do a brute force remove and insert.
  nsresult res = ContentRemoved(aPresContext, aContainer, 
                                aOldChild, aIndexInContainer, PR_TRUE);

  if (NS_SUCCEEDED(res)) {
    res = ContentInserted(aPresContext, aContainer, 
                          aNewChild, aIndexInContainer, nsnull, PR_TRUE);
  }

  return res;
}

// Returns PR_TRUE if aAncestorFrame is an ancestor frame of aFrame
static PRBool
IsAncestorFrame(nsIFrame* aFrame, nsIFrame* aAncestorFrame)
{
  nsIFrame* parentFrame;
  aFrame->GetParent(&parentFrame);

  while (parentFrame) {
    if (parentFrame == aAncestorFrame) {
      return PR_TRUE;
    }

    parentFrame->GetParent(&parentFrame);
  }

  return PR_FALSE;
}

/**
 * Called when a frame subtree is about to be deleted. Two important
 * things happen:
 *
 * 1. For each frame in the subtree, we remove the mapping from the
 *    content object to its frame
 *
 * 2. For child frames that have been moved out of the flow, we
 *    enqueue the out-of-frame for deletion *if* the out-of-flow frame's
 *    geometric parent is not in |aRemovedFrame|'s hierarchy (e.g., an
 *    absolutely positioned element that has been promoted to be a direct
 *    descendant of an area frame).
 *
 * Note: this function should only be called by DeletingFrameSubtree()
 *
 * @param   aRemovedFrame this is the frame that was removed from the
 *            content model. As we recurse we need to remember this so we
 *            can check if out-of-flow frames are a descendent of the frame
 *            being removed
 * @param   aFrame the local subtree that is being deleted. This is initially
 *            the same as aRemovedFrame, but as we recurse down the tree
 *            this changes
 */
static nsresult
DoDeletingFrameSubtree(nsIPresContext*  aPresContext,
                       nsIPresShell*    aPresShell,
                       nsIFrameManager* aFrameManager,
                       nsVoidArray&     aDestroyQueue,
                       nsIFrame*        aRemovedFrame,
                       nsIFrame*        aFrame)
{
  NS_PRECONDITION(aFrameManager, "no frame manager");

  // Remove the mapping from the content object to its frame
  nsCOMPtr<nsIContent> content;
  aFrame->GetContent(getter_AddRefs(content));
  if (content) {
    aFrameManager->SetPrimaryFrameFor(content, nsnull);
    aFrame->RemovedAsPrimaryFrame(aPresContext);
    aFrameManager->ClearAllUndisplayedContentIn(content);
  }

  // Walk aFrame's child frames
  nsCOMPtr<nsIAtom> childListName;
  PRInt32 childListIndex = 0;

  do {
    // Walk aFrame's child frames looking for placeholder frames
    nsIFrame* childFrame;
    aFrame->FirstChild(aPresContext, childListName, &childFrame);
    while (childFrame) {
      // The subtree we need to follow to get to the children; by
      // default, the childFrame.
      nsIFrame* subtree = childFrame;

      // See if it's a placeholder frame
      nsCOMPtr<nsIAtom> frameType;
      childFrame->GetFrameType(getter_AddRefs(frameType));

      if (nsLayoutAtoms::placeholderFrame == frameType.get()) {
        // Get the out-of-flow frame
        nsIFrame* outOfFlowFrame = ((nsPlaceholderFrame*)childFrame)->GetOutOfFlowFrame();
        NS_ASSERTION(outOfFlowFrame, "no out-of-flow frame");
  
        // Remove the mapping from the out-of-flow frame to its placeholder
        aFrameManager->UnregisterPlaceholderFrame((nsPlaceholderFrame*)childFrame);
        
        // Destroy the out-of-flow frame only if aRemovedFrame is _not_
        // one of its ancestor frames or if it is a popup frame. 
        // If aRemovedFrame is an ancestor of the out-of-flow frame, then 
        // the out-of-flow frame will be destroyed by aRemovedFrame.
        const nsStyleDisplay* display;
        outOfFlowFrame->GetStyleData(eStyleStruct_Display,
                                     (const nsStyleStruct*&)display);
        if (display->mDisplay == NS_STYLE_DISPLAY_POPUP || !IsAncestorFrame(outOfFlowFrame, aRemovedFrame)) {
          if (aDestroyQueue.IndexOf(outOfFlowFrame) < 0)
            aDestroyQueue.AppendElement(outOfFlowFrame);
        }

        // We want to descend into the out-of-flow frame's subtree,
        // not the placeholder frame's!
        subtree = outOfFlowFrame;
      }

      // Recursively find and delete any of its out-of-flow frames,
      // and remove the mapping from content objects to frames
      DoDeletingFrameSubtree(aPresContext, aPresShell, aFrameManager, aDestroyQueue,
                             aRemovedFrame, subtree);
  
      // Get the next sibling child frame
      childFrame->GetNextSibling(&childFrame);
    }

    aFrame->GetAdditionalChildListName(childListIndex++, getter_AddRefs(childListName));
  } while (childListName);

  return NS_OK;
}

/**
 * Called when a frame is about to be deleted. Calls DoDeletingFrameSubtree()
 * for aFrame and each of its continuing frames
 */
static nsresult
DeletingFrameSubtree(nsIPresContext*  aPresContext,
                     nsIPresShell*    aPresShell,
                     nsIFrameManager* aFrameManager,
                     nsIFrame*        aFrame)
{
  // If there's no frame manager it's probably because the pres shell is
  // being destroyed
  NS_ENSURE_TRUE(aFrame, NS_OK); // XXXldb Remove this sometime in the future.
  if (aFrameManager) {
    nsAutoVoidArray destroyQueue;

    // If it's a "special" block-in-inline frame, then we need to
    // remember to delete our special siblings, too.  Since every one of
    // the next-in-flows has the same special sibling, just do this
    // once, rather than in the loop below.
    if (IsFrameSpecial(aFrame)) {
      nsIFrame* specialSibling;
      GetSpecialSibling(aFrameManager, aFrame, &specialSibling);
      if (specialSibling)
        DeletingFrameSubtree(aPresContext, aPresShell, aFrameManager,
                             specialSibling);
    }

    do {
      DoDeletingFrameSubtree(aPresContext, aPresShell, aFrameManager,
                             destroyQueue, aFrame, aFrame);

      // If it's split, then get the continuing frame. Note that we only do
      // this for the top-most frame being deleted. Don't do it if we're
      // recursing over a subtree, because those continuing frames should be
      // found as part of the walk over the top-most frame's continuing frames.
      // Walking them again will make this an N^2/2 algorithm
      aFrame->GetNextInFlow(&aFrame);
    } while (aFrame);

    // Now destroy any frames that have been enqueued for destruction.
    for (PRInt32 i = destroyQueue.Count() - 1; i >= 0; --i) {
      nsIFrame* outOfFlowFrame = NS_STATIC_CAST(nsIFrame*, destroyQueue[i]);

      const nsStyleDisplay* display;
      outOfFlowFrame->GetStyleData(eStyleStruct_Display,
                                   (const nsStyleStruct*&)display);
      if (display->mDisplay == NS_STYLE_DISPLAY_POPUP) {
        // Locate the root popup set and remove ourselves from the popup set's list
        // of popup frames.
        nsIFrame* rootFrame;
        aFrameManager->GetRootFrame(&rootFrame);
        if (rootFrame)
          rootFrame->FirstChild(aPresContext, nsnull, &rootFrame);   
        nsCOMPtr<nsIRootBox> rootBox(do_QueryInterface(rootFrame));
        NS_ASSERTION(rootBox, "unexpected null pointer");
        if (rootBox) {
          nsIFrame* popupSetFrame;
          rootBox->GetPopupSetFrame(&popupSetFrame);
          NS_ASSERTION(popupSetFrame, "unexpected null pointer");
          if (popupSetFrame) {
            nsCOMPtr<nsIPopupSetFrame> popupSet(do_QueryInterface(popupSetFrame));
            NS_ASSERTION(popupSet, "unexpected null pointer");
            if (popupSet)
              popupSet->RemovePopupFrame(outOfFlowFrame);
          }
        }
      }
      else {
        // Get the out-of-flow frame's parent
        nsIFrame* parentFrame;
        outOfFlowFrame->GetParent(&parentFrame);
  
        // Get the child list name for the out-of-flow frame
        nsCOMPtr<nsIAtom> listName;
        GetChildListNameFor(aPresContext, parentFrame, outOfFlowFrame,
                            getter_AddRefs(listName));
  
        // Ask the parent to delete the out-of-flow frame
        aFrameManager->RemoveFrame(aPresContext, *aPresShell, parentFrame,
                                   listName, outOfFlowFrame);
      }
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsCSSFrameConstructor::RemoveMappingsForFrameSubtree(nsIPresContext* aPresContext,
                                                     nsIFrame* aRemovedFrame, 
                                                     nsILayoutHistoryState* aFrameState)
{
  nsCOMPtr<nsIPresShell> presShell;
  aPresContext->GetShell(getter_AddRefs(presShell));
  nsCOMPtr<nsIFrameManager> frameManager;
  presShell->GetFrameManager(getter_AddRefs(frameManager));
  
  // Save the frame tree's state before deleting it
  CaptureStateFor(aPresContext, aRemovedFrame, mTempFrameTreeState);

  return DeletingFrameSubtree(aPresContext, presShell, frameManager, aRemovedFrame);
}

static PRBool
HasPseudoStyle(nsIPresContext* aPresContext,
               nsIContent* aContent,
               nsIStyleContext* aStyleContext,
               nsIAtom* aPseudoElement)
{
  nsCOMPtr<nsIStyleContext> pseudoStyleContext;
  if (aContent) {
    aPresContext->ProbePseudoStyleContextFor(aContent,
                                             aPseudoElement,
                                             aStyleContext,
                                             getter_AddRefs(pseudoStyleContext));
  }
  return pseudoStyleContext != nsnull;
}

NS_IMETHODIMP
nsCSSFrameConstructor::ContentRemoved(nsIPresContext* aPresContext,
                                      nsIContent*     aContainer,
                                      nsIContent*     aChild,
                                      PRInt32         aIndexInContainer,
                                      PRBool          aInContentReplaced)
{
  // XXXldb Do we need to re-resolve style to handle the CSS2 + combinator and
  // the :empty pseudo-class?

#ifdef DEBUG
  if (gNoisyContentUpdates) {
    printf("nsCSSFrameConstructor::ContentRemoved container=%p child=%p index=%d\n",
           NS_STATIC_CAST(void*, aContainer),
           NS_STATIC_CAST(void*, aChild),
           aIndexInContainer);
    if (gReallyNoisyContentUpdates) {
      aContainer->List(stdout, 0);
    }
  }
#endif

  nsCOMPtr<nsIPresShell>    shell;
  aPresContext->GetShell(getter_AddRefs(shell));
  nsCOMPtr<nsIFrameManager> frameManager;
  shell->GetFrameManager(getter_AddRefs(frameManager));
  nsresult                  rv = NS_OK;

  // Find the child frame that maps the content
  nsIFrame* childFrame;
  shell->GetPrimaryFrameFor(aChild, &childFrame);

  if (! childFrame) {
    frameManager->ClearUndisplayedContentIn(aChild, aContainer);
  }

  // When the last item is removed from a select, 
  // we need to add a pseudo frame so select gets sized as the best it can
  // so here we see if it is a select and then we get the number of options
  if (aContainer && childFrame) {
    nsCOMPtr<nsIDOMHTMLSelectElement> selectElement = do_QueryInterface(aContainer);
    if (selectElement) {
      // XXX temp needed only native controls
      nsIFrame* selectFrame;
      // XXX temp needed only native controls
      shell->GetPrimaryFrameFor(aContainer, &selectFrame);

      // For "select" add the pseudo frame after the last item is deleted
      nsIFrame* parentFrame = nsnull;
      childFrame->GetParent(&parentFrame);
      if (parentFrame == selectFrame) {
        return NS_ERROR_FAILURE;
      }
      if (shell && parentFrame) {
        nsFrameConstructorState state(aPresContext,
                                      nsnull, nsnull, nsnull);
        AddDummyFrameToSelect(aPresContext, shell, state,
                              selectFrame, parentFrame, nsnull,
                              aContainer, selectElement);
      }
    } 
  }

#ifdef INCLUDE_XUL
  if (aContainer) {
    nsCOMPtr<nsIBindingManager> bindingManager;
    mDocument->GetBindingManager(getter_AddRefs(bindingManager));

    nsCOMPtr<nsIAtom> tag;
    aChild->GetTag(*getter_AddRefs(tag));

    PRBool listitem = tag && tag.get() == nsXULAtoms::listitem;
    if (listitem) {
      nsListBoxBodyFrame* listBoxBody = nsnull;
      if (childFrame) {
        // There is a frame for the removed content, so its parent frame is the listboxbody
        nsIFrame* parentFrame = nsnull;
        childFrame->GetParent(&parentFrame);
        if (parentFrame)
          listBoxBody = (nsListBoxBodyFrame*)parentFrame;
      } else {
        // There is no frame for the removed content, so we need to dig
        // further for the body frame. XBL insertion may not yet have taken
        // place here (in the case of  replacements), so our only connection
        // to the listbox content is through aContainer.  Use the boxObject
        // to get to the listboxbody frame so we can notify it of the removal.
        nsCOMPtr<nsIDOMXULElement> xulEl(do_QueryInterface(aContainer));
        if (xulEl) {
          nsCOMPtr<nsIBoxObject> boxObject;
          xulEl->GetBoxObject(getter_AddRefs(boxObject));
          nsCOMPtr<nsIListBoxObject> listBoxObject(do_QueryInterface(boxObject));
          nsIListBoxObject* bodyBoxObject = nsnull;
          listBoxObject->GetListboxBody(&bodyBoxObject);
          listBoxBody = NS_STATIC_CAST(nsListBoxBodyFrame*, bodyBoxObject);
          NS_RELEASE(bodyBoxObject);
        }
      }

      if (listBoxBody)
        listBoxBody->OnContentRemoved(aPresContext, childFrame, aIndexInContainer);
      return NS_OK;
    }

    PRInt32 namespaceID;
    bindingManager->ResolveTag(aContainer, &namespaceID, getter_AddRefs(tag));

    // Just ignore tree tags, anyway we don't create any frames for them.
    if (tag == nsXULAtoms::treechildren ||
        tag == nsXULAtoms::treeitem ||
        tag == nsXULAtoms::treerow ||
        (UseXBLForms() && ShouldIgnoreSelectChild(aContainer)))
      return NS_OK;

  }
#endif // INCLUDE_XUL

  if (childFrame) {
    // If the frame we are manipulating is a special frame then do
    // something different instead of just inserting newly created
    // frames.
    // NOTE: if we are in ContentReplaced, 
    //       then do not reframe as we are already doing just that!
    if (IsFrameSpecial(childFrame) && !aInContentReplaced) {
      // We are pretty harsh here (and definitely not optimal) -- we
      // wipe out the entire containing block and recreate it from
      // scratch. The reason is that because we know that a special
      // inline frame has propagated some of its children upward to be
      // children of the block and that those frames may need to move
      // around. This logic guarantees a correct answer.
#ifdef DEBUG
      if (gNoisyContentUpdates) {
        printf("nsCSSFrameConstructor::ContentRemoved: childFrame=");
        nsFrame::ListTag(stdout, childFrame);
        printf(" is special\n");
      }
#endif
      return ReframeContainingBlock(aPresContext, childFrame);
    }

    // Get the childFrame's parent frame
    nsIFrame* parentFrame;
    childFrame->GetParent(&parentFrame);

    // See if we have an XBL insertion point. If so, then that's our
    // real parent frame; if not, then the frame hasn't been built yet
    // and we just bail.
    nsIFrame* insertionPoint;
    GetInsertionPoint(shell, parentFrame, aChild, &insertionPoint);
    if (! insertionPoint)
      return NS_OK;

    parentFrame = insertionPoint;

    // Examine the containing-block for the removed content and see if
    // :first-letter style applies.
    nsIFrame* containingBlock =
      GetFloaterContainingBlock(aPresContext, parentFrame);
    nsCOMPtr<nsIStyleContext> blockSC;
    containingBlock->GetStyleContext(getter_AddRefs(blockSC));
    nsCOMPtr<nsIContent> blockContent;
    containingBlock->GetContent(getter_AddRefs(blockContent));
    PRBool haveFLS = HaveFirstLetterStyle(aPresContext, blockContent, blockSC);
    if (haveFLS) {
      // Trap out to special routine that handles adjusting a blocks
      // frame tree when first-letter style is present.
#ifdef NOISY_FIRST_LETTER
      printf("ContentRemoved: containingBlock=");
      nsFrame::ListTag(stdout, containingBlock);
      printf(" parentFrame=");
      nsFrame::ListTag(stdout, parentFrame);
      printf(" childFrame=");
      nsFrame::ListTag(stdout, childFrame);
      printf("\n");
#endif

      // First update the containing blocks structure by removing the
      // existing letter frames. This makes the subsequent logic
      // simpler.
      RemoveLetterFrames(aPresContext, shell, frameManager, containingBlock);

      // Recover childFrame and parentFrame
      shell->GetPrimaryFrameFor(aChild, &childFrame);
      if (!childFrame) {
        frameManager->ClearUndisplayedContentIn(aChild, aContainer);
        return NS_OK;
      }
      childFrame->GetParent(&parentFrame);

#ifdef NOISY_FIRST_LETTER
      printf("  ==> revised parentFrame=");
      nsFrame::ListTag(stdout, parentFrame);
      printf(" childFrame=");
      nsFrame::ListTag(stdout, childFrame);
      printf("\n");
#endif
    }

#ifdef DEBUG
    if (gReallyNoisyContentUpdates) {
      printf("nsCSSFrameConstructor::ContentRemoved: childFrame=");
      nsFrame::ListTag(stdout, childFrame);
      printf("\n");

      if (parentFrame) {
        nsIFrameDebug* fdbg = nsnull;
        CallQueryInterface(parentFrame, &fdbg);
        if (fdbg)
          fdbg->List(aPresContext, stdout, 0);
      }
      else
        printf("  ==> no parent frame\n");
    }
#endif

    // Walk the frame subtree deleting any out-of-flow frames, and
    // remove the mapping from content objects to frames
    DeletingFrameSubtree(aPresContext, shell, frameManager, childFrame);

    // See if the child frame is a floating frame
    //   (positioned frames are handled below in the "else" clause)
    const nsStyleDisplay* display;
    childFrame->GetStyleData(eStyleStruct_Display,
                             (const nsStyleStruct*&)display);
    nsPlaceholderFrame* placeholderFrame = nsnull;
    if (display->mDisplay == NS_STYLE_DISPLAY_POPUP)
      // Get the placeholder frame
      frameManager->GetPlaceholderFrameFor(childFrame,
                                           (nsIFrame**)&placeholderFrame);
    if (placeholderFrame) {
      // Remove the mapping from the frame to its placeholder
      frameManager->UnregisterPlaceholderFrame(placeholderFrame);
    
      // Locate the root popup set and remove ourselves from the popup set's list
      // of popup frames.
      nsIFrame* rootFrame;
      frameManager->GetRootFrame(&rootFrame);
      if (rootFrame)
        rootFrame->FirstChild(aPresContext, nsnull, &rootFrame);   
      nsCOMPtr<nsIRootBox> rootBox(do_QueryInterface(rootFrame));
      if (rootBox) {
        nsIFrame* popupSetFrame;
        rootBox->GetPopupSetFrame(&popupSetFrame);
        if (popupSetFrame) {
          nsCOMPtr<nsIPopupSetFrame> popupSet(do_QueryInterface(popupSetFrame));
          if (popupSet)
            popupSet->RemovePopupFrame(childFrame);
        }
      }

      // Remove the placeholder frame first (XXX second for now) (so
      // that it doesn't retain a dangling pointer to memory)
      if (placeholderFrame) {
        placeholderFrame->GetParent(&parentFrame);
        DeletingFrameSubtree(aPresContext, shell, frameManager, placeholderFrame);
        rv = frameManager->RemoveFrame(aPresContext, *shell, parentFrame,
                                       nsnull, placeholderFrame);
        return NS_OK;
      }
    }
    else if (display->IsFloating()) {
#ifdef NOISY_FIRST_LETTER
      printf("  ==> child display is still floating!\n");
#endif
      // Get the placeholder frame
      nsPlaceholderFrame* placeholderFrame;
      frameManager->GetPlaceholderFrameFor(childFrame,
                                           (nsIFrame**)&placeholderFrame);

      // Remove the mapping from the frame to its placeholder
      if (placeholderFrame)
        frameManager->UnregisterPlaceholderFrame(placeholderFrame);

      // Now we remove the floating frame

      // XXX has to be done first for now: the blocks line list
      // contains an array of pointers to the placeholder - we have to
      // remove the floater first (which gets rid of the lines
      // reference to the placeholder and floater) and then remove the
      // placeholder
      rv = frameManager->RemoveFrame(aPresContext, *shell, parentFrame,
                                     nsLayoutAtoms::floaterList, childFrame);

      // Remove the placeholder frame first (XXX second for now) (so
      // that it doesn't retain a dangling pointer to memory)
      if (nsnull != placeholderFrame) {
        placeholderFrame->GetParent(&parentFrame);
        DeletingFrameSubtree(aPresContext, shell, frameManager, placeholderFrame);
        rv = frameManager->RemoveFrame(aPresContext, *shell, parentFrame,
                                       nsnull, placeholderFrame);
      }
    }
    // See if it's absolutely or fixed positioned
    else if (display->IsAbsolutelyPositioned()) {
      // Get the placeholder frame
      nsPlaceholderFrame* placeholderFrame;
      frameManager->GetPlaceholderFrameFor(childFrame,
                                           (nsIFrame**)&placeholderFrame);

      // Remove the mapping from the frame to its placeholder
      if (placeholderFrame)
        frameManager->UnregisterPlaceholderFrame(placeholderFrame);

      // Generate two notifications. First for the absolutely positioned
      // frame
      rv = frameManager->RemoveFrame(aPresContext, *shell, parentFrame,
        (NS_STYLE_POSITION_FIXED == display->mPosition) ?
        nsLayoutAtoms::fixedList : nsLayoutAtoms::absoluteList, childFrame);

      // Now the placeholder frame
      if (nsnull != placeholderFrame) {
        placeholderFrame->GetParent(&parentFrame);
        rv = frameManager->RemoveFrame(aPresContext, *shell, parentFrame, nsnull,
                                       placeholderFrame);
      }

    } else {
      // Notify the parent frame that it should delete the frame
      // check for a table caption which goes on an additional child list with a different parent
      nsIFrame* outerTableFrame; 
      if (GetCaptionAdjustedParent(parentFrame, childFrame, &outerTableFrame)) {
        rv = frameManager->RemoveFrame(aPresContext, *shell, outerTableFrame,
                                       nsLayoutAtoms::captionList, childFrame);
      }
      else {
        rv = frameManager->RemoveFrame(aPresContext, *shell, insertionPoint,
                                       nsnull, childFrame);
      }
    }

    if (mInitialContainingBlock == childFrame) {
      mInitialContainingBlock = nsnull;
    }

    if (haveFLS && mInitialContainingBlock) {
      nsFrameConstructorState state(aPresContext, mFixedContainingBlock,
                                    GetAbsoluteContainingBlock(aPresContext,
                                                               parentFrame),
                                    GetFloaterContainingBlock(aPresContext,
                                                              parentFrame));
      RecoverLetterFrames(shell, aPresContext, state, containingBlock);
    }

#ifdef DEBUG
    if (gReallyNoisyContentUpdates && parentFrame) {
      nsIFrameDebug* fdbg = nsnull;
      CallQueryInterface(parentFrame, &fdbg);
      if (fdbg) {
        printf("nsCSSFrameConstructor::ContentRemoved: resulting frame model:\n");
        fdbg->List(aPresContext, stdout, 0);
      }
    }
#endif
  }

  return rv;
}

#ifdef DEBUG
  // To ensure that the functions below are only called within
  // |ApplyRenderingChangeToTree|.
static PRBool gInApplyRenderingChangeToTree = PR_FALSE;
#endif

static void
DoApplyRenderingChangeToTree(nsIPresContext* aPresContext,
                             nsIFrame* aFrame,
                             nsIViewManager* aViewManager,
                             nsChangeHint aChange);

static void
UpdateViewsForTree(nsIPresContext* aPresContext, nsIFrame* aFrame, 
                   nsIViewManager* aViewManager, nsRect& aBoundsRect,
                   nsChangeHint aChange)
{
  NS_PRECONDITION(gInApplyRenderingChangeToTree,
                  "should only be called within ApplyRenderingChangeToTree");

  nsIView* view;
  aFrame->GetView(aPresContext, &view);

  if (view) {
    if (aChange & nsChangeHint_RepaintFrame) {
      aViewManager->UpdateView(view, NS_VMREFRESH_NO_SYNC);
    }
    if (aChange & nsChangeHint_SyncFrameView) {
      nsContainerFrame::SyncFrameViewProperties(aPresContext, aFrame, nsnull, view);
    }
  }

  nsRect bounds;
  aFrame->GetRect(bounds);
  nsPoint parentOffset(bounds.x, bounds.y);
  bounds.x = 0;
  bounds.y = 0;

  // now do children of frame
  PRInt32 listIndex = 0;
  nsIAtom* childList = nsnull;
  nsIAtom* frameType = nsnull;

  do {
    nsIFrame* child = nsnull;
    aFrame->FirstChild(aPresContext, childList, &child);
    while (child) {
      nsFrameState  childState;
      child->GetFrameState(&childState);
      if (!(childState & NS_FRAME_OUT_OF_FLOW)) {
        // only do frames that are in flow
        child->GetFrameType(&frameType);
        if (nsLayoutAtoms::placeholderFrame == frameType) { // placeholder
          // get out of flow frame and start over there
          nsIFrame* outOfFlowFrame = ((nsPlaceholderFrame*)child)->GetOutOfFlowFrame();
          NS_ASSERTION(outOfFlowFrame, "no out-of-flow frame");

          DoApplyRenderingChangeToTree(aPresContext, outOfFlowFrame, aViewManager, aChange);
        }
        else {  // regular frame
          nsRect  childBounds;
          UpdateViewsForTree(aPresContext, child, aViewManager, childBounds, aChange);
          bounds.UnionRect(bounds, childBounds);
        }
        NS_IF_RELEASE(frameType);
      }
      child->GetNextSibling(&child);
    }
    NS_IF_RELEASE(childList);
    aFrame->GetAdditionalChildListName(listIndex++, &childList);
  } while (childList);
  NS_IF_RELEASE(childList);
  aBoundsRect = bounds;
  aBoundsRect += parentOffset;
}

static void
DoApplyRenderingChangeToTree(nsIPresContext* aPresContext,
                             nsIFrame* aFrame,
                             nsIViewManager* aViewManager,
                             nsChangeHint aChange)
{
  NS_PRECONDITION(gInApplyRenderingChangeToTree,
                  "should only be called within ApplyRenderingChangeToTree");

  for ( ; aFrame; aFrame->GetNextInFlow(&aFrame)) {
    // Get the frame's bounding rect
    nsRect invalidRect;
    nsPoint viewOffset;

    // Get view if this frame has one and trigger an update. If the
    // frame doesn't have a view, find the nearest containing view
    // (adjusting r's coordinate system to reflect the nesting) and
    // update there.
    nsIView* view = nsnull;
    aFrame->GetView(aPresContext, &view);
    nsIView* parentView;
    if (! view) { // XXX can view have children outside it?
      aFrame->GetOffsetFromView(aPresContext, viewOffset, &parentView);
      NS_ASSERTION(nsnull != parentView, "no view");
    }
    UpdateViewsForTree(aPresContext, aFrame, aViewManager, invalidRect, aChange);

    if (! view && (aChange & nsChangeHint_RepaintFrame)) { // if frame has view, will already be invalidated
      // XXX Instead of calling this we should really be calling
      // Invalidate on on the nsFrame (which does this)
      const nsStyleOutline* outline;
      aFrame->GetStyleData(eStyleStruct_Outline, (const nsStyleStruct*&)outline);
      nscoord width;
      outline->GetOutlineWidth(width);
      if (width > 0) {
        invalidRect.Inflate(width, width);
      }
      nsPoint frameOrigin;
      aFrame->GetOrigin(frameOrigin);
      invalidRect -= frameOrigin;
      invalidRect += viewOffset;
      aViewManager->UpdateView(parentView, invalidRect, NS_VMREFRESH_NO_SYNC);
    }
  }
}

static void
ApplyRenderingChangeToTree(nsIPresContext* aPresContext,
                           nsIFrame* aFrame,
                           nsIViewManager* aViewManager,
                           nsChangeHint aChange)
{
  nsCOMPtr<nsIPresShell> shell;
  aPresContext->GetShell(getter_AddRefs(shell));
  PRBool isPaintingSuppressed = PR_FALSE;
  shell->IsPaintingSuppressed(&isPaintingSuppressed);
  if (isPaintingSuppressed) {
    // Don't allow synchronous rendering changes when painting is turned off.
    aChange = NS_SubtractHint(aChange, nsChangeHint_RepaintFrame);
    if (!aChange) {
      return;
    }
  }

  // If the frame's background is propagated to an ancestor, walk up to
  // that ancestor.
  const nsStyleBackground *bg;
  PRBool isCanvas;
  while (!nsCSSRendering::FindBackground(aPresContext, aFrame,
                                         &bg, &isCanvas)) {
    aFrame->GetParent(&aFrame);
    NS_ASSERTION(aFrame, "root frame must paint");
  }

  nsCOMPtr<nsIViewManager> viewManager(aViewManager);
  if (!viewManager) {
    nsIView* view = nsnull;
    aFrame->GetView(aPresContext, &view);
    if (! view) {
      nsPoint offset;
      aFrame->GetOffsetFromView(aPresContext, offset, &view);
    }
    NS_ASSERTION(view, "no view");
    view->GetViewManager(*getter_AddRefs(viewManager));
  }

  // Trigger rendering updates by damaging this frame and any
  // continuations of this frame.

  // XXX this needs to detect the need for a view due to an opacity change and deal with it...

  viewManager->BeginUpdateViewBatch();

#ifdef DEBUG
  gInApplyRenderingChangeToTree = PR_TRUE;
#endif
  DoApplyRenderingChangeToTree(aPresContext, aFrame, viewManager, aChange);
#ifdef DEBUG
  gInApplyRenderingChangeToTree = PR_FALSE;
#endif

  viewManager->EndUpdateViewBatch(NS_VMREFRESH_NO_SYNC);
}

nsresult
nsCSSFrameConstructor::StyleChangeReflow(nsIPresContext* aPresContext,
                                         nsIFrame* aFrame,
                                         nsIAtom* aAttribute)
{
  // If the frame hasn't even received an initial reflow, then don't
  // send it a style-change reflow!
  nsFrameState state;
  aFrame->GetFrameState(&state);
  if (state & NS_FRAME_FIRST_REFLOW)
    return NS_OK;

#ifdef DEBUG
  if (gNoisyContentUpdates) {
    printf("nsCSSFrameConstructor::StyleChangeReflow: aFrame=");
    nsFrame::ListTag(stdout, aFrame);
    printf("\n");
  }
#endif

  // Is it a box? If so we can coelesce.
  nsresult rv;
  nsIBox *box;
  rv = CallQueryInterface(aFrame, &box);
  if (NS_SUCCEEDED(rv) && box) {
    nsBoxLayoutState state(aPresContext);
    box->MarkStyleChange(state);
  }
  else {
    // If the frame is part of a split block-in-inline hierarchy, then
    // target the style-change reflow at the first ``normal'' ancestor
    // so we're sure that the style change will propagate to any
    // anonymously created siblings.
    if (IsFrameSpecial(aFrame))
      aFrame = GetIBContainingBlockFor(aFrame);

    // Target a style-change reflow at the frame.
    nsCOMPtr<nsIPresShell> shell;
    aPresContext->GetShell(getter_AddRefs(shell));
 
    nsHTMLReflowCommand *reflowCmd;
    rv = NS_NewHTMLReflowCommand(&reflowCmd, aFrame,
                                 eReflowType_StyleChanged,
                                 nsnull,
                                 aAttribute);
  
    if (NS_SUCCEEDED(rv))
      shell->AppendReflowCommand(reflowCmd);
  }

  // If the background of the frame is painted on one of its ancestors,
  // the reflow might not invalidate correctly.
  nsIFrame *ancestor = aFrame;
  const nsStyleBackground *bg;
  PRBool isCanvas;
  while (!nsCSSRendering::FindBackground(aPresContext, ancestor,
                                         &bg, &isCanvas)) {
    ancestor->GetParent(&ancestor);
    NS_ASSERTION(ancestor, "canvas must paint");
  }
  // This isn't the most efficient way to do it, but it saves code
  // size and doesn't add much cost compared to the reflow..
  if (ancestor != aFrame)
    ApplyRenderingChangeToTree(aPresContext, ancestor, nsnull, nsChangeHint_RepaintFrame);

  return NS_OK;
}

NS_IMETHODIMP
nsCSSFrameConstructor::ContentChanged(nsIPresContext* aPresContext,
                                      nsIContent*  aContent,
                                      nsISupports* aSubContent)
{
  nsCOMPtr<nsIPresShell> shell;
  aPresContext->GetShell(getter_AddRefs(shell));
  nsresult      rv = NS_OK;

  // Find the child frame
  nsIFrame* frame;
  shell->GetPrimaryFrameFor(aContent, &frame);

  // Notify the first frame that maps the content. It will generate a reflow
  // command

  // It's possible the frame whose content changed isn't inserted into the
  // frame hierarchy yet, or that there is no frame that maps the content
  if (nsnull != frame) {
#if 0
    NS_FRAME_LOG(NS_FRAME_TRACE_CALLS,
       ("nsHTMLStyleSheet::ContentChanged: content=%p[%s] subcontent=%p frame=%p",
        aContent, ContentTag(aContent, 0),
        aSubContent, frame));
#endif

    // Special check for text content that is a child of a letter
    // frame. There are two interesting cases that we have to handle
    // carefully: text content that is going empty (which means we
    // should select a new text node as the first-letter text) or text
    // content that empty but is no longer empty (it might be the
    // first-letter text but isn't currently).
    //
    // To deal with both of these we make a simple change: map a
    // ContentChanged into a ContentReplaced when we are changing text
    // that is part of a first-letter situation.
    PRBool doContentChanged = PR_TRUE;
    nsCOMPtr<nsITextContent> textContent(do_QueryInterface(aContent));
    if (textContent) {
      // Ok, it's text content. Now do some real work...
      nsIFrame* block = GetFloaterContainingBlock(aPresContext, frame);
      if (block) {
        // See if the block has first-letter style applied to it.
        nsCOMPtr<nsIContent> blockContent;
        block->GetContent(getter_AddRefs(blockContent));
        nsCOMPtr<nsIStyleContext> blockSC;
        block->GetStyleContext(getter_AddRefs(blockSC));
        PRBool haveFirstLetterStyle =
          HaveFirstLetterStyle(aPresContext, blockContent, blockSC);
        if (haveFirstLetterStyle) {
          // The block has first-letter style. Use content-replaced to
          // repair the blocks frame structure properly.
          nsCOMPtr<nsIContent> container;
          aContent->GetParent(*getter_AddRefs(container));
          if (container) {
            PRInt32 ix;
            container->IndexOf(aContent, ix);
            doContentChanged = PR_FALSE;
            rv = ContentReplaced(aPresContext, container,
                                 aContent, aContent, ix);
          }
        }
      }
    }

    if (doContentChanged) {
      frame->ContentChanged(aPresContext, aContent, aSubContent);
    }
  }

  return rv;
}


NS_IMETHODIMP 
nsCSSFrameConstructor::ProcessRestyledFrames(nsStyleChangeList& aChangeList, 
                                             nsIPresContext* aPresContext)
{
  PRInt32 count = aChangeList.Count();
  while (0 < count--) {
    nsIFrame* frame;
    nsIContent* content;
    nsChangeHint hint;
    aChangeList.ChangeAt(count, frame, content, hint);

    if (hint & nsChangeHint_ReconstructDoc) {
      NS_ERROR("This shouldn't happen");
    }
    if (hint & nsChangeHint_ReconstructFrame) {
      RecreateFramesForContent(aPresContext, content);
    } else {
      if (hint & nsChangeHint_ReflowFrame) {
        StyleChangeReflow(aPresContext, frame, nsnull);
      }
      if (hint & (nsChangeHint_RepaintFrame | nsChangeHint_SyncFrameView)) {
        ApplyRenderingChangeToTree(aPresContext, frame, nsnull, hint);
      }
    }
  }
  aChangeList.Clear();
  return NS_OK;
}

NS_IMETHODIMP
nsCSSFrameConstructor::ContentStatesChanged(nsIPresContext* aPresContext, 
                                            nsIContent* aContent1,
                                            nsIContent* aContent2,
                                            PRInt32 aStateMask) 
{
  nsresult  result = NS_OK;

  nsCOMPtr<nsIPresShell> shell;
  aPresContext->GetShell(getter_AddRefs(shell));

  NS_ASSERTION(shell, "couldn't get pres shell");
  if (shell) {
    nsCOMPtr<nsIStyleSet> styleSet;
    shell->GetStyleSet(getter_AddRefs(styleSet));

    NS_ASSERTION(styleSet, "couldn't get style set");
    if (styleSet) { // test if any style rules exist which are dependent on content state
      nsIFrame* primaryFrame1 = nsnull;
      nsIFrame* primaryFrame2 = nsnull;
      PRUint8 app1 = 0;
      PRUint8 app2 = 0;

      shell->GetPrimaryFrameFor(aContent1, &primaryFrame1);
      if (primaryFrame1) {
        const nsStyleDisplay* disp;
        primaryFrame1->GetStyleData(eStyleStruct_Display, (const nsStyleStruct*&)disp);
        app1 = disp->mAppearance;
      }

      if (!app1) {
        PRBool depends = PR_FALSE;
        styleSet->HasStateDependentStyle(aPresContext, aContent1,
                                         aStateMask, &depends);
        if (!depends) {
          primaryFrame1 = nsnull;
          aContent1 = nsnull;
        }
      }

      if (aContent2 == aContent1)
        aContent2 = nsnull;
      else if (aContent2) {
        shell->GetPrimaryFrameFor(aContent2, &primaryFrame2);
        if (primaryFrame2) {
          const nsStyleDisplay* disp2;
          primaryFrame2->GetStyleData(eStyleStruct_Display, (const nsStyleStruct*&)disp2);
          app2 = disp2->mAppearance;
        }

        if (!app2) {
          PRBool depends = PR_FALSE;
          styleSet->HasStateDependentStyle(aPresContext, aContent2,
                                           aStateMask, &depends);
          if (!depends) {
            primaryFrame2 = nsnull;
            aContent2 = nsnull;
          }
        }
      }

      if (primaryFrame1 && primaryFrame2) { // detect if one is parent of other, skip child
        nsIFrame* parent;
        primaryFrame1->GetParent(&parent);
        while (parent) {
          if (parent == primaryFrame2) {  // frame2 is frame1's parent, skip frame1
            primaryFrame1 = nsnull;
            break;
          }
          parent->GetParent(&parent);
        }
        if (primaryFrame1) {
          primaryFrame2->GetParent(&parent);
          while (parent) {
            if (parent == primaryFrame1) {  // frame1 is frame2's parent, skip frame2
              primaryFrame2 = nsnull;
              break;
            }
            parent->GetParent(&parent);
          }
        }
      }

      nsCOMPtr<nsIFrameManager> frameManager;
      shell->GetFrameManager(getter_AddRefs(frameManager));

      if (primaryFrame1) {
        nsStyleChangeList changeList1;
        nsStyleChangeList changeList2;
        nsChangeHint frameChange1 = NS_STYLE_HINT_NONE;
        nsChangeHint frameChange2 = NS_STYLE_HINT_NONE;
        frameManager->ComputeStyleChangeFor(aPresContext, primaryFrame1, 
                                            kNameSpaceID_Unknown, nsnull,
                                            changeList1, NS_STYLE_HINT_NONE, frameChange1);

        if (app1) {
          nsCOMPtr<nsITheme> theme;
          aPresContext->GetTheme(getter_AddRefs(theme));
          PRBool repaint = PR_FALSE;
          if (theme && theme->ThemeSupportsWidget(aPresContext, primaryFrame1, app1))
            theme->WidgetStateChanged(primaryFrame1, app1, nsnull, &repaint);
          if (repaint)
            ApplyRenderingChangeToTree(aPresContext, primaryFrame1, nsnull, nsChangeHint_RepaintFrame);
        }

        if (!(frameChange1 & nsChangeHint_ReconstructDoc) && (primaryFrame2)) {
          frameManager->ComputeStyleChangeFor(aPresContext, primaryFrame2, 
                                              kNameSpaceID_Unknown, nsnull,
                                              changeList2, NS_STYLE_HINT_NONE, frameChange2);
          if (app2) {
            nsCOMPtr<nsITheme> theme;
            aPresContext->GetTheme(getter_AddRefs(theme));
            PRBool repaint = PR_FALSE;
            if (theme && theme->ThemeSupportsWidget(aPresContext, primaryFrame2, app2))
              theme->WidgetStateChanged(primaryFrame2, app2, nsnull, &repaint);
            if (repaint) 
              ApplyRenderingChangeToTree(aPresContext, primaryFrame2, nsnull, nsChangeHint_RepaintFrame);
          }
        }

        if ((frameChange1 & nsChangeHint_ReconstructDoc) || 
            (frameChange2 & nsChangeHint_ReconstructDoc)) {
          result = ReconstructDocElementHierarchy(aPresContext);
        }
        else {
          if (frameChange1 & nsChangeHint_ReconstructFrame) {
            result = RecreateFramesForContent(aPresContext, aContent1);
            changeList1.Clear();
          } else {
            if (frameChange1 & ~(nsChangeHint_AttrChange | nsChangeHint_Aural)) {
              // let primary frame deal with it
              result = primaryFrame1->ContentStateChanged(aPresContext, aContent1, frameChange1);
            }
          }

          if (frameChange2 & nsChangeHint_ReconstructFrame) {
              result = RecreateFramesForContent(aPresContext, aContent2);
              changeList2.Clear();
          } else {
            if (frameChange2 & ~(nsChangeHint_AttrChange | nsChangeHint_Aural)) {
              // let primary frame deal with it
              result = primaryFrame2->ContentStateChanged(aPresContext, aContent2, frameChange2);
              // then process any children that need it
            }
          }
          ProcessRestyledFrames(changeList1, aPresContext);
          ProcessRestyledFrames(changeList2, aPresContext);
        }
      }
      else if (primaryFrame2) {
        nsStyleChangeList changeList;
        nsChangeHint frameChange = NS_STYLE_HINT_NONE;
        frameManager->ComputeStyleChangeFor(aPresContext, primaryFrame2, 
                                            kNameSpaceID_Unknown, nsnull,
                                            changeList, NS_STYLE_HINT_NONE, frameChange);
        if (app2) {
          nsCOMPtr<nsITheme> theme;
          aPresContext->GetTheme(getter_AddRefs(theme));
          PRBool repaint = PR_FALSE;
          if (theme)
            theme->WidgetStateChanged(primaryFrame2, app2, nsnull, &repaint);
          if (repaint)
            ApplyRenderingChangeToTree(aPresContext, primaryFrame2, nsnull, nsChangeHint_RepaintFrame);
        }

         // max change needed for top level frames
        if (frameChange & nsChangeHint_ReconstructDoc) {
          result = ReconstructDocElementHierarchy(aPresContext);
          changeList.Clear();
        } else if (frameChange & nsChangeHint_ReconstructFrame) {
          result = RecreateFramesForContent(aPresContext, aContent2);
          changeList.Clear();
        } else if (frameChange & ~(nsChangeHint_AttrChange | nsChangeHint_Aural)) {
          // let primary frame deal with it
          result = primaryFrame2->ContentStateChanged(aPresContext, aContent2, frameChange);
          // then process any children that need it
        }
        ProcessRestyledFrames(changeList, aPresContext);
      }
      else {  // no frames, reconstruct for content
        if (aContent1) {
          result = RecreateFramesForContent(aPresContext, aContent1);
        }
        if (aContent2) {
          result = RecreateFramesForContent(aPresContext, aContent2);
        }
      }
    }
  }
  return result;
}

NS_IMETHODIMP
nsCSSFrameConstructor::AttributeChanged(nsIPresContext* aPresContext,
                                        nsIContent* aContent,
                                        PRInt32 aNameSpaceID,
                                        nsIAtom* aAttribute,
                                        PRInt32 aModType, 
                                        nsChangeHint aHint)
{
  nsresult  result = NS_OK;

  nsCOMPtr<nsIStyleFrameConstruction> kungFuDeathGrip(this);

  nsCOMPtr<nsIPresShell> shell;
  aPresContext->GetShell(getter_AddRefs(shell));

  // Get the frame associated with the content which is the highest in the frame tree
  nsIFrame* primaryFrame;
  shell->GetPrimaryFrameFor(aContent, &primaryFrame); 
  // Get the frame associated with the content whose style context is highest in the style context tree
  nsIFrame* primaryStyleFrame = primaryFrame;
  if (primaryFrame) {
    PRBool providerIsChild = PR_FALSE;
    nsIFrame *styleContextProvider;
    primaryFrame->GetParentStyleContextFrame(aPresContext,
                                             &styleContextProvider,
                                             &providerIsChild);
    if (providerIsChild)
      primaryStyleFrame = styleContextProvider;
  }

#if 0
  NS_FRAME_LOG(NS_FRAME_TRACE_CALLS,
     ("HTMLStyleSheet::AttributeChanged: content=%p[%s] frame=%p",
      aContent, ContentTag(aContent, 0), frame));
#endif

  // the style tag has its own interpretation based on aHint 
  if (aHint & nsChangeHint_Unknown) { 
    nsCOMPtr<nsIStyledContent> styledContent = do_QueryInterface(aContent);
    if (styledContent) { 
      // Get style hint from HTML content object. 
      styledContent->GetMappedAttributeImpact(aAttribute, aModType, aHint);
    } 
  }

  PRBool reconstruct = (aHint & nsChangeHint_ReconstructDoc) != 0;
  PRBool reframe = (aHint & (nsChangeHint_ReconstructDoc | nsChangeHint_ReconstructFrame)) != 0;
  PRBool restyle = (aHint & ~(nsChangeHint_AttrChange)) != 0;

#ifdef INCLUDE_XUL
  // The following listbox widget trap prevents offscreen listbox widget
  // content from being removed and re-inserted (which is what would
  // happen otherwise).
  if (!primaryFrame && !reframe) {
    nsCOMPtr<nsIBindingManager> bindingManager;
    mDocument->GetBindingManager(getter_AddRefs(bindingManager));

    PRInt32 namespaceID;
    nsCOMPtr<nsIAtom> tag;
    bindingManager->ResolveTag(aContent, &namespaceID, getter_AddRefs(tag));

    if (tag && (tag.get() == nsXULAtoms::listitem || tag.get() == nsXULAtoms::listcell))
      return NS_OK;
  }

  if (aAttribute == nsXULAtoms::tooltiptext ||
      aAttribute == nsXULAtoms::tooltip) 
  {
    nsIFrame* rootFrame = nsnull;
    shell->GetRootFrame(&rootFrame);
    if (rootFrame)
      rootFrame->FirstChild(aPresContext, nsnull, &rootFrame);   
    nsCOMPtr<nsIRootBox> rootBox(do_QueryInterface(rootFrame));
    if (rootBox) {
      if (aModType == nsIDOMMutationEvent::REMOVAL)
        rootBox->RemoveTooltipSupport(aContent);
      if (aModType == nsIDOMMutationEvent::ADDITION)
        rootBox->AddTooltipSupport(aContent);
    }
  }

#endif // INCLUDE_XUL

  // check for inline style.  we need to clear the data at the style
  // context's rule node whenever the inline style property changes.

  nsCOMPtr<nsIStyleContext> styleContext;
  nsCOMPtr<nsIStyleRule> rule;
  PRBool inlineStyle = PR_FALSE;
  if (aAttribute == nsHTMLAtoms::style) {
    nsCOMPtr<nsIStyledContent> scontent(do_QueryInterface(aContent));
    scontent->GetInlineStyleRule(getter_AddRefs(rule));
    if (rule) {
      inlineStyle = PR_TRUE;

      // This style rule exists and we need to blow away any computed
      // data that this rule cached in the rule tree.
      if (primaryStyleFrame)
        primaryStyleFrame->GetStyleContext(getter_AddRefs(styleContext));
      else {
        // We might be in the undisplayed map.  Retrieve the style context from there.
        nsCOMPtr<nsIFrameManager> frameManager;
        shell->GetFrameManager(getter_AddRefs(frameManager));
        frameManager->GetUndisplayedContent(aContent, getter_AddRefs(styleContext));
#ifdef DEBUG
        if (!styleContext) {
          nsCOMPtr<nsIContent> parent;
          aContent->GetParent(*getter_AddRefs(parent));
          if (parent) {
            nsIFrame* parentFrame;
            shell->GetPrimaryFrameFor(parent, &parentFrame);
            NS_ASSERTION(!parentFrame,
                     "parent frame but no child frame or undisplayed entry");
          }
        }
#endif
      }
    }
  }

  // first see if we need to manage the style system: 
  //  inlineStyle changes require us to clear out style data associated with the style attribute
  // NOTE: for reframe, it happens after the old frames have been destroyed, not here, otherwise
  //       we cannot get to the old style information for the old frame, and cannot correctly
  //       deal with positioned frames or floaters (see bug 118415) - this is done in 
  //       RecreateFramesForContent, which is called for a reframe
  if (inlineStyle && 
      (reconstruct || restyle) && 
      !reframe) {
    nsCOMPtr<nsIStyleSet> set;
    shell->GetStyleSet(getter_AddRefs(set));
    // XXXldb If |styleContext| is null, wouldn't it be faster to pass
    // in something to tell it that this change is for inline style?
    set->ClearStyleData(aPresContext, rule, styleContext);
  }

  // See if we have appearance information for a theme.
  if (primaryFrame) {
    const nsStyleDisplay* disp;
    primaryFrame->GetStyleData(eStyleStruct_Display,
                               (const nsStyleStruct*&)disp);
    if (disp && disp->mAppearance) {
      nsCOMPtr<nsITheme> theme;
      aPresContext->GetTheme(getter_AddRefs(theme));
      if (theme && theme->ThemeSupportsWidget(aPresContext, primaryFrame, disp->mAppearance)) {
        PRBool repaint = PR_FALSE;
        theme->WidgetStateChanged(primaryFrame, disp->mAppearance, aAttribute, &repaint);
        if (repaint)
          ApplyRenderingChangeToTree(aPresContext, primaryFrame, nsnull, nsChangeHint_RepaintFrame);
      }
    }
  }

  // apply changes
  if (primaryFrame && (aHint & nsChangeHint_AttrChange) && !(aHint & ~(nsChangeHint_AttrChange)))
    result = primaryFrame->AttributeChanged(aPresContext, aContent, aNameSpaceID, aAttribute, aModType, aHint);
  else if (reconstruct) {
    result = ReconstructDocElementHierarchy(aPresContext);
  }
  else if (reframe) {
    result = RecreateFramesForContent(aPresContext, aContent, inlineStyle, rule, styleContext);
  }
  else if (restyle) {
    // If there is no frame then there is no point in re-styling it,
    // is there?
    if (primaryFrame) {
      nsChangeHint maxHint = aHint;
      nsStyleChangeList changeList;
      // put primary frame on list to deal with, re-resolve may update or add next in flows
      changeList.AppendChange(primaryFrame, aContent, maxHint);
      nsCOMPtr<nsIFrameManager> frameManager;
      shell->GetFrameManager(getter_AddRefs(frameManager));

      PRBool affects;
      frameManager->AttributeAffectsStyle(aAttribute, aContent, affects);
      if (affects) {
#ifdef DEBUG_shaver
        fputc('+', stderr);
#endif
        // there is an effect, so compute it
        frameManager->ComputeStyleChangeFor(aPresContext, primaryFrame, 
                                            aNameSpaceID, aAttribute,
                                            changeList, aHint, maxHint);
      } else {
#ifdef DEBUG_shaver
        fputc('-', stderr);
#endif
        // let this frame update itself, but don't walk the whole frame tree
        maxHint = NS_STYLE_HINT_VISUAL;
      }

      // maxHint is hint for primary only
      if (maxHint & nsChangeHint_ReconstructDoc) {
        result = ReconstructDocElementHierarchy(aPresContext);
        changeList.Clear();
      } else if (maxHint & nsChangeHint_ReconstructFrame) {
        result = RecreateFramesForContent(aPresContext, aContent);
        changeList.Clear();
      } else if (maxHint & ~(nsChangeHint_AttrChange | nsChangeHint_Aural)) {
          // let the frame deal with it, since we don't know how to
          result = primaryFrame->AttributeChanged(aPresContext, aContent, aNameSpaceID, aAttribute, aModType, maxHint);

          // XXXwaterson should probably check for special IB siblings
          // here, and propagate the AttributeChanged notification to
          // them, as well. Currently, inline and block frames don't
          // do anything on this notification, so it's not that big a
          // deal.
      }
      // handle any children (primary may be on list too)
      ProcessRestyledFrames(changeList, aPresContext);
    }
    else {  // no frame now, possibly genetate one with new style data
      result = RecreateFramesForContent(aPresContext, aContent, inlineStyle, rule, styleContext);
    }
  }

  return result;
}

  // Style change notifications
NS_IMETHODIMP
nsCSSFrameConstructor::StyleRuleChanged(nsIPresContext* aPresContext,
                                        nsIStyleSheet* aStyleSheet,
                                        nsIStyleRule* aStyleRule,
                                        nsChangeHint aHint)
{
  nsresult result = NS_OK;
  nsCOMPtr<nsIPresShell> shell;
  aPresContext->GetShell(getter_AddRefs(shell));
  nsIFrame* frame;
  shell->GetRootFrame(&frame);

  if (!frame) {
    return NS_OK;
  }

  PRBool reframe = (aHint & (nsChangeHint_ReconstructDoc | nsChangeHint_ReconstructFrame
                             | nsChangeHint_Unknown)) != 0;
  PRBool restyle = (aHint & ~(nsChangeHint_AttrChange)) != 0;
  // TBD: add "review" to update view?

  if (restyle) {
    nsCOMPtr<nsIStyleSet> set;
    shell->GetStyleSet(getter_AddRefs(set));
    set->ClearStyleData(aPresContext, aStyleRule, nsnull);
  }

  if (reframe) {
    result = ReconstructDocElementHierarchy(aPresContext);
  }
  else {
    PRBool reflow = (aHint & (nsChangeHint_ReconstructDoc | nsChangeHint_ReconstructFrame
                              | nsChangeHint_ReflowFrame | nsChangeHint_Unknown)) != 0;
    PRBool renderOrSync = (aHint & (nsChangeHint_ReconstructDoc | nsChangeHint_ReconstructFrame
                                    | nsChangeHint_ReflowFrame | nsChangeHint_RepaintFrame
                                    | nsChangeHint_Unknown | nsChangeHint_SyncFrameView)) != 0;

    // XXX hack, skip the root and scrolling frames
    frame->FirstChild(aPresContext, nsnull, &frame);
    frame->FirstChild(aPresContext, nsnull, &frame);
    if (reflow) {
      StyleChangeReflow(aPresContext, frame, nsnull);
    }
    if (renderOrSync) {
      ApplyRenderingChangeToTree(aPresContext, frame, nsnull, aHint);
    }
  }

  return result;
}

NS_IMETHODIMP
nsCSSFrameConstructor::StyleRuleAdded(nsIPresContext* aPresContext,
                                      nsIStyleSheet* aStyleSheet,
                                      nsIStyleRule* aStyleRule)
{
  // XXX TBI: should query rule for impact and do minimal work
  ReconstructDocElementHierarchy(aPresContext);
  return NS_OK;
}

NS_IMETHODIMP
nsCSSFrameConstructor::StyleRuleRemoved(nsIPresContext* aPresContext,
                                        nsIStyleSheet* aStyleSheet,
                                        nsIStyleRule* aStyleRule)
{
  // XXX TBI: should query rule for impact and do minimal work
  ReconstructDocElementHierarchy(aPresContext);
  return NS_OK;
}

//STATIC
void nsCSSFrameConstructor::GetAlternateTextFor(nsIContent* aContent,
                                                nsIAtom*    aTag,  // content object's tag
                                                nsString&   aAltText)
{
  nsresult  rv;

  // The "alt" attribute specifies alternate text that is rendered
  // when the image can not be displayed
  rv = aContent->GetAttr(kNameSpaceID_None, nsHTMLAtoms::alt, aAltText);

  // If there's no "alt" attribute, and aContent is an input    
  // element, then use the value of the "value" attribute
  if ((NS_CONTENT_ATTR_NOT_THERE == rv) && (nsHTMLAtoms::input == aTag)) {
    rv = aContent->GetAttr(kNameSpaceID_None, nsHTMLAtoms::value,
                           aAltText);

    // If there's no "value" attribute either, then use the localized string 
    // for "Submit" as the alternate text.
    if (NS_CONTENT_ATTR_NOT_THERE == rv) {
      nsFormControlHelper::GetLocalizedString(nsFormControlHelper::GetHTMLPropertiesFileName(),
                                              NS_LITERAL_STRING("Submit").get(), aAltText);      
    }
  }
}

// Construct an alternate frame to use when the image can't be rendered
nsresult
nsCSSFrameConstructor::ConstructAlternateFrame(nsIPresShell*    aPresShell, 
                                               nsIPresContext*  aPresContext,
                                               nsIContent*      aContent,
                                               nsIStyleContext* aStyleContext,
                                               nsIFrame*        aParentFrame,
                                               nsIFrame*&       aFrame)
{
  nsresult rv;
  nsAutoString  altText;

  // Initialize OUT parameter
  aFrame = nsnull;

  // Get the alternate text to use
  nsCOMPtr<nsIAtom> tag;
  aContent->GetTag(*getter_AddRefs(tag));
  GetAlternateTextFor(aContent, tag, altText);

  // Create a text content element for the alternate text
  nsCOMPtr<nsIContent> altTextContent(do_CreateInstance(kTextNodeCID,&rv));
  if (NS_FAILED(rv))
    return rv;

  // Set the content's text
  nsCOMPtr<nsIDOMCharacterData> domData = do_QueryInterface(altTextContent);
  if (domData)
    domData->SetData(altText);
  
  // Set aContent as the parent content and set the document object
  altTextContent->SetParent(aContent);
  altTextContent->SetDocument(mDocument, PR_TRUE, PR_TRUE);

  // Create either an inline frame, block frame, or area frame
  nsIFrame* containerFrame;
  PRBool    isOutOfFlow = PR_FALSE;
  const nsStyleDisplay* display = (const nsStyleDisplay*)
    aStyleContext->GetStyleData(eStyleStruct_Display);
  
  if (display->IsAbsolutelyPositioned()) {
    NS_NewAbsoluteItemWrapperFrame(aPresShell, &containerFrame);
    isOutOfFlow = PR_TRUE;
  } else if (display->IsFloating()) {
    NS_NewFloatingItemWrapperFrame(aPresShell, &containerFrame);
    isOutOfFlow = PR_TRUE;
  } else if (NS_STYLE_DISPLAY_BLOCK == display->mDisplay) {
    NS_NewBlockFrame(aPresShell, &containerFrame);
  } else {
    NS_NewInlineFrame(aPresShell, &containerFrame);
  }
  containerFrame->Init(aPresContext, aContent, aParentFrame, aStyleContext, nsnull);
  nsHTMLContainerFrame::CreateViewForFrame(aPresContext, containerFrame,
                                           aStyleContext, nsnull, PR_FALSE);

  // If the frame is out-of-flow, then mark it as such
  if (isOutOfFlow) {
    nsFrameState  frameState;
    containerFrame->GetFrameState(&frameState);
    containerFrame->SetFrameState(frameState | NS_FRAME_OUT_OF_FLOW);
  }

  // Create a text frame to display the alt-text. It gets a pseudo-element
  // style context
  nsIFrame*        textFrame;
  nsIStyleContext* textStyleContext;

  NS_NewTextFrame(aPresShell, &textFrame);
  aPresContext->ResolveStyleContextForNonElement(aStyleContext,
                                                 &textStyleContext);

  textFrame->Init(aPresContext, altTextContent, containerFrame,
                  textStyleContext, nsnull);
  NS_RELEASE(textStyleContext);
  containerFrame->SetInitialChildList(aPresContext, nsnull, textFrame);

  // Return the container frame
  aFrame = containerFrame;

  return NS_OK;
}

#ifdef NS_DEBUG
static PRBool
IsPlaceholderFrame(nsIFrame* aFrame)
{
  nsIAtom*  frameType;
  PRBool    result;

  aFrame->GetFrameType(&frameType);
  result = frameType == nsLayoutAtoms::placeholderFrame;
  NS_IF_RELEASE(frameType);
  return result;
}
#endif


static PRBool
HasDisplayableChildren(nsIPresContext* aPresContext, nsIFrame* aContainerFrame)
{
  // Returns 'true' if there are frames within aContainerFrame that
  // could be displayed in the frame list.
  NS_PRECONDITION(aContainerFrame != nsnull, "null ptr");
  if (! aContainerFrame)
    return PR_FALSE;

  nsIFrame* frame;
  aContainerFrame->FirstChild(aPresContext, nsnull, &frame);

  while (frame) {
    // If it's not a text frame, then assume that it's displayable.
    nsCOMPtr<nsIAtom> frameType;
    frame->GetFrameType(getter_AddRefs(frameType));
    if (frameType.get() != nsLayoutAtoms::textFrame)
      return PR_TRUE;

    // Get the text content...
    nsCOMPtr<nsIContent> content;
    frame->GetContent(getter_AddRefs(content));
    
    nsCOMPtr<nsITextContent> text = do_QueryInterface(content);
    NS_ASSERTION(text != nsnull, "oops, not an nsITextContent");
    if (! text)
      return PR_TRUE;
    
    // Is it only whitespace?
    PRBool onlyWhitespace;
    text->IsOnlyWhitespace(&onlyWhitespace);
    
    // If not, then we have displayable content here.
    if (! onlyWhitespace)
      return PR_TRUE;
    
    // Otherwise, on to the next frame...
    frame->GetNextSibling(&frame);
  }

  // If we get here, then we've iterated through all the child frames,
  // and every one is a text frame containing whitespace. (Or, there
  // weren't any frames at all!) There is nothing to diplay.
  return PR_FALSE;
}



NS_IMETHODIMP
nsCSSFrameConstructor::CantRenderReplacedElement(nsIPresShell* aPresShell, 
                                                 nsIPresContext* aPresContext,
                                                 nsIFrame*       aFrame)
{
  nsresult                  rv = NS_OK;

  // Get parent frame and style context
  nsIFrame*                 parentFrame;
  nsCOMPtr<nsIStyleContext> styleContext;
  aFrame->GetParent(&parentFrame);
  aFrame->GetStyleContext(getter_AddRefs(styleContext));

  // Get aFrame's content object and the tag name
  nsCOMPtr<nsIContent>      content;
  PRInt32                   nameSpaceID;
  nsCOMPtr<nsIAtom>         tag;

  aFrame->GetContent(getter_AddRefs(content));
  NS_ASSERTION(content, "null content object");
  content->GetNameSpaceID(nameSpaceID);
  content->GetTag(*getter_AddRefs(tag));

  // Get the child list name that the frame is contained in
  nsCOMPtr<nsIAtom>  listName;
  GetChildListNameFor(aPresContext, parentFrame, aFrame, getter_AddRefs(listName));

  // If the frame is out of the flow, then it has a placeholder frame.
  nsPlaceholderFrame* placeholderFrame = nsnull;
  nsCOMPtr<nsIPresShell> presShell;
  aPresContext->GetShell(getter_AddRefs(presShell));
  if (listName) {
    presShell->GetPlaceholderFrameFor(aFrame, (nsIFrame**)&placeholderFrame);
  }

  // Get the previous sibling frame
  nsIFrame*     firstChild;
  parentFrame->FirstChild(aPresContext, listName, &firstChild);
  nsFrameList   frameList(firstChild);
  
  // See whether it's an IMG or an INPUT element (for image buttons)
  // or if it is an applet with no displayable children
  // XXX need to check nameSpaceID in these spots
  if (nsHTMLAtoms::img == tag.get() || nsHTMLAtoms::input == tag.get() ||
      (nsHTMLAtoms::applet == tag.get() && !HasDisplayableChildren(aPresContext, aFrame))) {
    // Try and construct an alternate frame to use when the
    // image can't be rendered
    nsIFrame* newFrame;
    rv = ConstructAlternateFrame(aPresShell, aPresContext, content, styleContext,
                                 parentFrame, newFrame);

    if (NS_SUCCEEDED(rv)) {
      nsCOMPtr<nsIFrameManager> frameManager;
      presShell->GetFrameManager(getter_AddRefs(frameManager));

      // Replace the old frame with the new frame

      DeletingFrameSubtree(aPresContext, presShell, frameManager, aFrame);

      // Reset the primary frame mapping
      frameManager->SetPrimaryFrameFor(content, newFrame);

      // Replace the old frame with the new frame
      frameManager->ReplaceFrame(aPresContext, *presShell, parentFrame,
                                 listName, aFrame, newFrame);

      // Now that we've replaced the primary frame, if there's a placeholder
      // frame then complete the transition from image frame to new frame
      if (placeholderFrame) {
        // Remove the association between the old frame and its placeholder
        frameManager->UnregisterPlaceholderFrame(placeholderFrame);
        
        // Placeholder frames have a pointer back to the out-of-flow frame.
        // Make sure that's correct, too.
        placeholderFrame->SetOutOfFlowFrame(newFrame);

        // Reuse the existing placeholder frame, and add an association to the
        // new frame
        frameManager->RegisterPlaceholderFrame(placeholderFrame);

        // XXX Work around a bug in the block code where the floater won't get
        // reflowed unless the line containing the placeholder frame is reflowed...
        nsIFrame* placeholderParentFrame;
        placeholderFrame->GetParent(&placeholderParentFrame);
        placeholderParentFrame->ReflowDirtyChild(aPresShell, placeholderFrame);
      }
    }

  } else if ((nsHTMLAtoms::object == tag.get()) ||
             (nsHTMLAtoms::embed == tag.get()) ||
             (nsHTMLAtoms::applet == tag.get())) {
    // It's an OBJECT, EMBED, or APPLET, so we should display the contents
    // instead
    nsIFrame* absoluteContainingBlock;
    nsIFrame* floaterContainingBlock;
    nsIFrame* inFlowParent = parentFrame;

    // If the OBJECT frame is out-of-flow, then get the placeholder frame's
    // parent and use that when determining the absolute containing block and
    // floater containing block
    if (placeholderFrame) {
      placeholderFrame->GetParent(&inFlowParent);
    }

    absoluteContainingBlock = GetAbsoluteContainingBlock(aPresContext, inFlowParent);
    floaterContainingBlock = GetFloaterContainingBlock(aPresContext, inFlowParent);

#ifdef NS_DEBUG
    // Verify that we calculated the same containing block
    if (listName.get() == nsLayoutAtoms::absoluteList) {
      NS_ASSERTION(absoluteContainingBlock == parentFrame,
                   "wrong absolute containing block");
    } else if (listName.get() == nsLayoutAtoms::floaterList) {
      NS_ASSERTION(floaterContainingBlock == parentFrame,
                   "wrong floater containing block");
    }
#endif

    // Now initialize the frame construction state
    nsFrameConstructorState state(aPresContext, mFixedContainingBlock,
                                  absoluteContainingBlock,
                                  floaterContainingBlock);
    nsFrameItems            frameItems;
    const nsStyleDisplay*   display =
      NS_STATIC_CAST(const nsStyleDisplay*, styleContext->GetStyleData(eStyleStruct_Display));

    // Create a new frame based on the display type.
    // Note: if the old frame was out-of-flow, then so will the new frame
    // and we'll get a new placeholder frame
    rv = ConstructFrameByDisplayType(aPresShell, aPresContext, state, display,
                                     content, nameSpaceID, tag,
                                     inFlowParent, styleContext, frameItems);

    if (NS_FAILED(rv)) return rv;

    nsIFrame* newFrame = frameItems.childList;


    if (NS_SUCCEEDED(rv)) {
      if (placeholderFrame) {
        // Remove the association between the old frame and its placeholder
        // Note: ConstructFrameByDisplayType() will already have added an
        // association for the new placeholder frame
        state.mFrameManager->UnregisterPlaceholderFrame(placeholderFrame);

        // Verify that the new frame is also a placeholder frame
        NS_ASSERTION(IsPlaceholderFrame(newFrame), "unexpected frame type");

        // Replace the old placeholder frame with the new placeholder frame
        state.mFrameManager->ReplaceFrame(aPresContext, *presShell, inFlowParent,
                                          nsnull, placeholderFrame, newFrame);
      }

      // Replace the primary frame
      if (listName == nsnull) {
        if (IsInlineFrame(parentFrame) && !AreAllKidsInline(newFrame)) {
          // We're in the uncomfortable position of being an inline
          // that now contains a block. As in ConstructInline(), break
          // the newly constructed frames into three lists: the inline
          // frames before the first block frame (list1), the inline
          // frames after the last block frame (list3), and all the
          // frames between the first and last block frames (list2).
          nsIFrame* list1 = newFrame;

          nsIFrame* prevToFirstBlock;
          nsIFrame* list2 = FindFirstBlock(aPresContext, list1, &prevToFirstBlock);

          if (prevToFirstBlock)
            prevToFirstBlock->SetNextSibling(nsnull);
          else list1 = nsnull;

          nsIFrame* afterFirstBlock;
          list2->GetNextSibling(&afterFirstBlock);

          nsIFrame* lastBlock = FindLastBlock(aPresContext, afterFirstBlock);
          if (! lastBlock)
            lastBlock = list2;

          nsIFrame* list3 = nsnull;
          lastBlock->GetNextSibling(&list3);
          lastBlock->SetNextSibling(nsnull);

          // Create "special" inline-block linkage between the frames
          // XXXldb Do we really need to do this?  It doesn't seem
          // consistent with the use in ConstructInline.
          SetFrameIsSpecial(state.mFrameManager, list1, list2);
          SetFrameIsSpecial(state.mFrameManager, list2, list3);
          SetFrameIsSpecial(state.mFrameManager, list3, nsnull);

          // Recursively split inlines back up to the first containing
          // block frame.
          SplitToContainingBlock(aPresContext, state, parentFrame, list1, list2, list3, PR_FALSE);
        }
      } else if (listName.get() == nsLayoutAtoms::absoluteList) {
        newFrame = state.mAbsoluteItems.childList;
        state.mAbsoluteItems.childList = nsnull;
      } else if (listName.get() == nsLayoutAtoms::fixedList) {
        newFrame = state.mFixedItems.childList;
        state.mFixedItems.childList = nsnull;
      } else if (listName.get() == nsLayoutAtoms::floaterList) {
        newFrame = state.mFloatedItems.childList;
        state.mFloatedItems.childList = nsnull;
      }
      DeletingFrameSubtree(aPresContext, presShell,
                           state.mFrameManager, aFrame);
      state.mFrameManager->ReplaceFrame(aPresContext, *presShell, parentFrame,
                                        listName, aFrame, newFrame);

      // Reset the primary frame mapping. Don't assume that
      // ConstructFrameByDisplayType() has done this
      state.mFrameManager->SetPrimaryFrameFor(content, newFrame);
      
      // If there are new absolutely positioned child frames, then notify
      // the parent
      // XXX We can't just assume these frames are being appended, we need to
      // determine where in the list they should be inserted...
      if (state.mAbsoluteItems.childList) {
        rv = state.mAbsoluteItems.containingBlock->AppendFrames(aPresContext, *presShell,
                                                     nsLayoutAtoms::absoluteList,
                                                     state.mAbsoluteItems.childList);
      }
  
      // If there are new fixed positioned child frames, then notify
      // the parent
      // XXX We can't just assume these frames are being appended, we need to
      // determine where in the list they should be inserted...
      if (state.mFixedItems.childList) {
        rv = state.mFixedItems.containingBlock->AppendFrames(aPresContext, *presShell,
                                                  nsLayoutAtoms::fixedList,
                                                  state.mFixedItems.childList);
      }
  
      // If there are new floating child frames, then notify the parent
      // XXX We can't just assume these frames are being appended, we need to
      // determine where in the list they should be inserted...
      if (state.mFloatedItems.childList) {
        rv = state.mFloatedItems.containingBlock->AppendFrames(aPresContext,
                                                    *presShell,
                                                    nsLayoutAtoms::floaterList,
                                                    state.mFloatedItems.childList);
      }
    }

  } else if (nsHTMLAtoms::input == tag.get()) {
    // XXX image INPUT elements are also image frames, but don't throw away the
    // image frame, because the frame class has extra logic that is specific to
    // INPUT elements

  } else {
    NS_ASSERTION(PR_FALSE, "unexpected tag");
  }

  return rv;
}

nsresult
nsCSSFrameConstructor::CreateContinuingOuterTableFrame(nsIPresShell* aPresShell, 
                                                       nsIPresContext*  aPresContext,
                                                       nsIFrame*        aFrame,
                                                       nsIFrame*        aParentFrame,
                                                       nsIContent*      aContent,
                                                       nsIStyleContext* aStyleContext,
                                                       nsIFrame**       aContinuingFrame)
{
  nsIFrame* newFrame;
  nsresult  rv;

  rv = NS_NewTableOuterFrame(aPresShell, &newFrame);
  if (NS_SUCCEEDED(rv)) {
    newFrame->Init(aPresContext, aContent, aParentFrame, aStyleContext, aFrame);
    nsHTMLContainerFrame::CreateViewForFrame(aPresContext, newFrame,
                                             aStyleContext, nsnull, PR_FALSE);

    // Create a continuing inner table frame, and if there's a caption then
    // replicate the caption
    nsIFrame*     childFrame;
    nsFrameItems  newChildFrames;

    aFrame->FirstChild(aPresContext, nsnull, &childFrame);
    while (childFrame) {
      nsIAtom*  tableType;

      // See if it's the inner table frame
      childFrame->GetFrameType(&tableType);
      if (nsLayoutAtoms::tableFrame == tableType) {
        nsIFrame* continuingTableFrame;

        // It's the inner table frame, so create a continuing frame
        CreateContinuingFrame(aPresShell, aPresContext, childFrame, newFrame, &continuingTableFrame);
        newChildFrames.AddChild(continuingTableFrame);
      } else {
        // XXX remove this code and the above checks. We don't want to replicate 
        // the caption (that is what the thead is for). This code is not executed 
        // anyway, because the caption was put in a different child list.
        nsIContent*           caption;
        nsIStyleContext*      captionStyle;
        const nsStyleDisplay* display;

        childFrame->GetContent(&caption);
        childFrame->GetStyleContext(&captionStyle);
        display = (const nsStyleDisplay*)captionStyle->GetStyleData(eStyleStruct_Display);
        NS_ASSERTION(NS_STYLE_DISPLAY_TABLE_CAPTION == display->mDisplay, "expected caption");

        // Replicate the caption frame
        // XXX We have to do it this way instead of calling ConstructFrameByDisplayType(),
        // because of a bug in the way ConstructTableFrame() handles the initial child
        // list...
        nsIFrame*               captionFrame;
        nsFrameItems            childItems;
        NS_NewTableCaptionFrame(aPresShell, &captionFrame);
        nsFrameConstructorState state(aPresContext, mFixedContainingBlock,
                                      GetAbsoluteContainingBlock(aPresContext, newFrame),
                                      captionFrame);
        captionFrame->Init(aPresContext, caption, newFrame, captionStyle, nsnull);
        ProcessChildren(aPresShell, aPresContext, state, caption, captionFrame,
                        PR_TRUE, childItems, PR_TRUE);
        captionFrame->SetInitialChildList(aPresContext, nsnull, childItems.childList);
        // XXX Deal with absolute and fixed frames...
        if (state.mFloatedItems.childList) {
          captionFrame->SetInitialChildList(aPresContext,
                                            nsLayoutAtoms::floaterList,
                                            state.mFloatedItems.childList);
        }
        newChildFrames.AddChild(captionFrame);
        NS_RELEASE(caption);
        NS_RELEASE(captionStyle);
      }
      NS_IF_RELEASE(tableType);
      childFrame->GetNextSibling(&childFrame);
    }

    // Set the outer table's initial child list
    newFrame->SetInitialChildList(aPresContext, nsnull, newChildFrames.childList);
  }

  *aContinuingFrame = newFrame;
  return rv;
}

nsresult
nsCSSFrameConstructor::CreateContinuingTableFrame(nsIPresShell* aPresShell, 
                                                  nsIPresContext*  aPresContext,
                                                  nsIFrame*        aFrame,
                                                  nsIFrame*        aParentFrame,
                                                  nsIContent*      aContent,
                                                  nsIStyleContext* aStyleContext,
                                                  nsIFrame**       aContinuingFrame)
{
  nsIFrame* newFrame;
  nsresult  rv;
    
  rv = NS_NewTableFrame(aPresShell, &newFrame);
  if (NS_SUCCEEDED(rv)) {
    newFrame->Init(aPresContext, aContent, aParentFrame, aStyleContext, aFrame);
    nsHTMLContainerFrame::CreateViewForFrame(aPresContext, newFrame,
                                             aStyleContext, nsnull, PR_FALSE);

    // Replicate any header/footer frames
    nsIFrame*     rowGroupFrame;
    nsFrameItems  childFrames;

    aFrame->FirstChild(aPresContext, nsnull, &rowGroupFrame);
    while (rowGroupFrame) {
      // See if it's a header/footer
      nsIStyleContext*      rowGroupStyle;
      const nsStyleDisplay* display;

      rowGroupFrame->GetStyleContext(&rowGroupStyle);
      display = (const nsStyleDisplay*)rowGroupStyle->GetStyleData(eStyleStruct_Display);

      if ((NS_STYLE_DISPLAY_TABLE_HEADER_GROUP == display->mDisplay) ||
          (NS_STYLE_DISPLAY_TABLE_FOOTER_GROUP == display->mDisplay)) {
        // If the row group has was continued, then don't replicate it
        nsIFrame* rgNextInFlow;
        rowGroupFrame->GetNextInFlow(&rgNextInFlow);
        if (rgNextInFlow) {
          ((nsTableRowGroupFrame*)rowGroupFrame)->SetRepeatable(PR_FALSE);
        }
        // Replicate the header/footer frame if it is not too tall
        else if (((nsTableRowGroupFrame*)rowGroupFrame)->IsRepeatable()) {        
          nsIFrame*               headerFooterFrame;
          nsFrameItems            childItems;
          nsIContent*             headerFooter;
          nsFrameConstructorState state(aPresContext, mFixedContainingBlock,
                                        GetAbsoluteContainingBlock(aPresContext, newFrame),
                                        nsnull);

          NS_NewTableRowGroupFrame(aPresShell, &headerFooterFrame);
          rowGroupFrame->GetContent(&headerFooter);
          headerFooterFrame->Init(aPresContext, headerFooter, newFrame,
                                  rowGroupStyle, nsnull);
          nsTableCreator tableCreator(aPresShell); 
          ProcessChildren(aPresShell, aPresContext, state, headerFooter, headerFooterFrame,
                          PR_FALSE, childItems, PR_FALSE, &tableCreator);
          NS_ASSERTION(!state.mFloatedItems.childList, "unexpected floated element");
          NS_RELEASE(headerFooter);
          headerFooterFrame->SetInitialChildList(aPresContext, nsnull, childItems.childList);
          ((nsTableRowGroupFrame*)headerFooterFrame)->SetRepeatable(PR_TRUE);

          // Table specific initialization
          ((nsTableRowGroupFrame*)headerFooterFrame)->InitRepeatedFrame
            (aPresContext, (nsTableRowGroupFrame*)rowGroupFrame);

          // XXX Deal with absolute and fixed frames...
          childFrames.AddChild(headerFooterFrame);
        }
      }

      NS_RELEASE(rowGroupStyle);
      
      // Get the next row group frame
      rowGroupFrame->GetNextSibling(&rowGroupFrame);
    }
    
    // Set the table frame's initial child list
    newFrame->SetInitialChildList(aPresContext, nsnull, childFrames.childList);
  }

  *aContinuingFrame = newFrame;
  return rv;
}

NS_IMETHODIMP
nsCSSFrameConstructor::CreateContinuingFrame(nsIPresShell*   aPresShell, 
                                             nsIPresContext* aPresContext,
                                             nsIFrame*       aFrame,
                                             nsIFrame*       aParentFrame,
                                             nsIFrame**      aContinuingFrame)
{
  nsCOMPtr<nsIAtom>          frameType;
  nsCOMPtr<nsIContent>       content;
  nsCOMPtr<nsIStyleContext>  styleContext;
  nsIFrame*                  newFrame = nsnull;
  nsresult                   rv;

  // Use the frame type to determine what type of frame to create
  aFrame->GetFrameType(getter_AddRefs(frameType));
  aFrame->GetContent(getter_AddRefs(content));
  aFrame->GetStyleContext(getter_AddRefs(styleContext));

  if (nsLayoutAtoms::textFrame == frameType) {
    rv = NS_NewContinuingTextFrame(aPresShell, &newFrame);
    if (NS_SUCCEEDED(rv)) {
      newFrame->Init(aPresContext, content, aParentFrame, styleContext, aFrame);
      nsHTMLContainerFrame::CreateViewForFrame(aPresContext, newFrame,
                                               styleContext, nsnull, PR_FALSE);
    }
    
  } else if (nsLayoutAtoms::inlineFrame == frameType) {
    rv = NS_NewInlineFrame(aPresShell, &newFrame);
    if (NS_SUCCEEDED(rv)) {
      newFrame->Init(aPresContext, content, aParentFrame, styleContext, aFrame);
      nsHTMLContainerFrame::CreateViewForFrame(aPresContext, newFrame,
                                               styleContext, nsnull, PR_FALSE);
    }
  
  } else if (nsLayoutAtoms::blockFrame == frameType) {
    rv = NS_NewBlockFrame(aPresShell, &newFrame);
    if (NS_SUCCEEDED(rv)) {
      newFrame->Init(aPresContext, content, aParentFrame, styleContext, aFrame);
      nsHTMLContainerFrame::CreateViewForFrame(aPresContext, newFrame,
                                               styleContext, nsnull, PR_FALSE);
    }
  
  } else if (nsLayoutAtoms::areaFrame == frameType) {
    rv = NS_NewAreaFrame(aPresShell, &newFrame);
    if (NS_SUCCEEDED(rv)) {
      newFrame->Init(aPresContext, content, aParentFrame, styleContext,
                     aFrame);
      nsHTMLContainerFrame::CreateViewForFrame(aPresContext, newFrame,
                                               styleContext, nsnull, PR_FALSE);
    }
  
  } else if (nsLayoutAtoms::positionedInlineFrame == frameType) {
    rv = NS_NewPositionedInlineFrame(aPresShell, &newFrame);
    if (NS_SUCCEEDED(rv)) {
      newFrame->Init(aPresContext, content, aParentFrame, styleContext, aFrame);
      nsHTMLContainerFrame::CreateViewForFrame(aPresContext, newFrame,
                                               styleContext, nsnull, PR_FALSE);
    }

  } else if (nsLayoutAtoms::pageFrame == frameType) {
    rv = NS_NewPageFrame(aPresShell, &newFrame);
    if (NS_SUCCEEDED(rv)) {
      newFrame->Init(aPresContext, content, aParentFrame, styleContext, aFrame);
      nsHTMLContainerFrame::CreateViewForFrame(aPresContext, newFrame,
                                               styleContext, nsnull, PR_TRUE);
      nsIFrame* pageContentFrame = nsnull;
      NS_NewPageContentFrame(aPresShell, &pageContentFrame);

      nsCOMPtr<nsIStyleContext> pageContentPseudoStyle;
      aPresContext->ResolvePseudoStyleContextFor(nsnull, nsLayoutAtoms::pageContentPseudo,
                                                 styleContext,
                                                 getter_AddRefs(pageContentPseudoStyle));

      pageContentFrame->Init(aPresContext, nsnull, newFrame, pageContentPseudoStyle, nsnull);
      nsHTMLContainerFrame::CreateViewForFrame(aPresContext, pageContentFrame,
                                               pageContentPseudoStyle, nsnull, PR_TRUE);

     // Set the initial child lists
      newFrame->SetInitialChildList(aPresContext, nsnull, pageContentFrame);

    }

  } else if (nsLayoutAtoms::tableOuterFrame == frameType) {
    rv = CreateContinuingOuterTableFrame(aPresShell, aPresContext, aFrame, aParentFrame,
                                         content, styleContext, &newFrame);

  } else if (nsLayoutAtoms::tableFrame == frameType) {
    rv = CreateContinuingTableFrame(aPresShell, aPresContext, aFrame, aParentFrame,
                                    content, styleContext, &newFrame);

  } else if (nsLayoutAtoms::tableRowGroupFrame == frameType) {
    rv = NS_NewTableRowGroupFrame(aPresShell, &newFrame);
    if (NS_SUCCEEDED(rv)) {
      newFrame->Init(aPresContext, content, aParentFrame, styleContext, aFrame);
      nsHTMLContainerFrame::CreateViewForFrame(aPresContext, newFrame,
                                               styleContext, nsnull, PR_FALSE);
    }

  } else if (nsLayoutAtoms::tableRowFrame == frameType) {
    rv = NS_NewTableRowFrame(aPresShell, &newFrame);
    if (NS_SUCCEEDED(rv)) {
      newFrame->Init(aPresContext, content, aParentFrame, styleContext, aFrame);
      nsHTMLContainerFrame::CreateViewForFrame(aPresContext, newFrame,
                                               styleContext, nsnull, PR_FALSE);

      // Create a continuing frame for each table cell frame
      nsIFrame*     cellFrame;
      nsFrameItems  newChildList;

      aFrame->FirstChild(aPresContext, nsnull, &cellFrame);
      while (cellFrame) {
        nsIAtom*  tableType;
        
        // See if it's a table cell frame
        cellFrame->GetFrameType(&tableType);
        if (IS_TABLE_CELL(tableType)) {
          nsIFrame* continuingCellFrame;

          CreateContinuingFrame(aPresShell, aPresContext, cellFrame, newFrame, &continuingCellFrame);
          newChildList.AddChild(continuingCellFrame);
        }

        NS_IF_RELEASE(tableType);
        cellFrame->GetNextSibling(&cellFrame);
      }
      
      // Set the table cell's initial child list
      newFrame->SetInitialChildList(aPresContext, nsnull, newChildList.childList);
    }

  } else if (IS_TABLE_CELL(frameType)) {
    rv = NS_NewTableCellFrame(aPresShell, IsBorderCollapse(aParentFrame), &newFrame);
    if (NS_SUCCEEDED(rv)) {
      newFrame->Init(aPresContext, content, aParentFrame, styleContext, aFrame);
      nsHTMLContainerFrame::CreateViewForFrame(aPresContext, newFrame,
                                               styleContext, nsnull, PR_FALSE);

      // Create a continuing area frame
      nsIFrame* areaFrame;
      nsIFrame* continuingAreaFrame;
      aFrame->FirstChild(aPresContext, nsnull, &areaFrame);
      CreateContinuingFrame(aPresShell, aPresContext, areaFrame, newFrame, &continuingAreaFrame);

      // Set the table cell's initial child list
      newFrame->SetInitialChildList(aPresContext, nsnull, continuingAreaFrame);
    }
  
  } else if (nsLayoutAtoms::lineFrame == frameType) {
    rv = NS_NewFirstLineFrame(aPresShell, &newFrame);
    if (NS_SUCCEEDED(rv)) {
      newFrame->Init(aPresContext, content, aParentFrame, styleContext, aFrame);
      nsHTMLContainerFrame::CreateViewForFrame(aPresContext, newFrame,
                                               styleContext, nsnull, PR_FALSE);
    }
  
  } else if (nsLayoutAtoms::letterFrame == frameType) {
    rv = NS_NewFirstLetterFrame(aPresShell, &newFrame);
    if (NS_SUCCEEDED(rv)) {
      newFrame->Init(aPresContext, content, aParentFrame, styleContext, aFrame);
      nsHTMLContainerFrame::CreateViewForFrame(aPresContext, newFrame,
                                               styleContext, nsnull, PR_FALSE);
    }

  } else if (nsLayoutAtoms::imageFrame == frameType) {
    rv = NS_NewImageFrame(aPresShell, &newFrame);
    if (NS_SUCCEEDED(rv)) {
      newFrame->Init(aPresContext, content, aParentFrame, styleContext, aFrame);
    }
  } else if (nsLayoutAtoms::placeholderFrame == frameType) {
    // create a continuing out of flow frame
    nsIFrame* oofFrame = ((nsPlaceholderFrame*)aFrame)->GetOutOfFlowFrame();
    nsIFrame* oofContFrame;
    CreateContinuingFrame(aPresShell, aPresContext, oofFrame, aParentFrame, &oofContFrame);
    if (!oofContFrame) 
      return NS_ERROR_NULL_POINTER;
    // create a continuing placeholder frame
    nsCOMPtr<nsIFrameManager> frameManager;
    aPresShell->GetFrameManager(getter_AddRefs(frameManager));
    NS_ASSERTION(frameManager, "no frame manager");
    CreatePlaceholderFrameFor(aPresShell, aPresContext, frameManager, content, 
                              oofContFrame, styleContext, aParentFrame, &newFrame);
    if (!newFrame) 
      return NS_ERROR_NULL_POINTER;
    newFrame->Init(aPresContext, content, aParentFrame, styleContext, aFrame);
  } else {
    NS_ASSERTION(PR_FALSE, "unexpected frame type");
    rv = NS_ERROR_UNEXPECTED;
  }

  *aContinuingFrame = newFrame; 

  return rv;
}

// Get the frame's next-in-flow, or, if it doesn't have one,
// its special sibling.
static nsIFrame*
GetNifOrSpecialSibling(nsIFrameManager *aFrameManager, nsIFrame *aFrame)
{
  nsIFrame *result;
  aFrame->GetNextInFlow(&result);
  if (result)
    return result;

  if (IsFrameSpecial(aFrame))
    GetSpecialSibling(aFrameManager, aFrame, &result);
  return result;
}

// Helper function that searches the immediate child frames 
// (and their children if the frames are "special")
// for a frame that maps the specified content object
nsIFrame*
nsCSSFrameConstructor::FindFrameWithContent(nsIPresContext*  aPresContext,
                                            nsIFrameManager* aFrameManager,
                                            nsIFrame*        aParentFrame,
                                            nsIContent*      aParentContent,
                                            nsIContent*      aContent,
                                            nsFindFrameHint* aHint)
{
  
#ifdef NOISY_FINDFRAME
  FFWC_totalCount++;
  printf("looking for content=%p, given aParentFrame %p parentContent %p, hint is %s\n", 
         aContent, aParentFrame, aParentContent, aHint ? "set" : "NULL");
#endif

  NS_ENSURE_TRUE(aParentFrame != nsnull, nsnull);

  do {
    // Search for the frame in each child list that aParentFrame supports
    nsIAtom* listName = nsnull;
    PRInt32 listIndex = 0;
    do {
#ifdef NOISY_FINDFRAME
      FFWC_doLoop++;
#endif
      nsIFrame* kidFrame=nsnull;
      // if we were given an hint, try to use it here to find a good
      // previous frame to start our search (|kidFrame|).
      if (aHint) {
#ifdef NOISY_FINDFRAME
        printf("  hint frame is %p\n", aHint->mPrimaryFrameForPrevSibling);
#endif
        // start with the primary frame for aContent's previous sibling
        kidFrame = aHint->mPrimaryFrameForPrevSibling;
        // But if it's out of flow, start from its placeholder.
        if (kidFrame) {
          nsFrameState kidState;
          kidFrame->GetFrameState(&kidState);
          if (kidState & NS_FRAME_OUT_OF_FLOW) {
            aFrameManager->GetPlaceholderFrameFor(kidFrame, &kidFrame);
          }
        }

        if (kidFrame) {
          // then use the next sibling frame as our starting point
          kidFrame->GetNextSibling(&kidFrame);
          if (!kidFrame)
          { // the hint frame had no next frame.  try the next-in-flow fo the parent of the hint frame
            // if there is one
            nsIFrame *parentFrame=nsnull;
            aHint->mPrimaryFrameForPrevSibling->GetParent(&parentFrame);
            if (parentFrame) {
              parentFrame = GetNifOrSpecialSibling(aFrameManager, parentFrame);
            }
            if (parentFrame) 
            { // if we found the next-in-flow for the parent of the hint frame, start with it's first child
              parentFrame->FirstChild(aPresContext, listName, &kidFrame);
              // Leave |aParentFrame| as-is, since the only time we'll
              // reuse it is if the hint fails.
#ifdef DEBUG
              if (kidFrame) {
                nsCOMPtr<nsIContent> parentContent;
                parentFrame->GetContent(getter_AddRefs(parentContent));
                NS_ASSERTION(parentContent == aParentContent,
                             "next-in-flow has different content");
              }
#endif
            }
          }
#ifdef NOISY_FINDFRAME
          printf("  hint gives us kidFrame=%p with parent frame %p content %p\n", 
                  kidFrame, aParentFrame, aParentContent);
#endif
        }
      }
      if (!kidFrame) {  // we didn't have enough info to prune, start searching from the beginning
        aParentFrame->FirstChild(aPresContext, listName, &kidFrame);
      }
      while (kidFrame) {
        nsCOMPtr<nsIContent>  kidContent;
        
        // See if the child frame points to the content object we're
        // looking for
        kidFrame->GetContent(getter_AddRefs(kidContent));
        if (kidContent == aContent) {
          nsCOMPtr<nsIAtom>  frameType;

          // We found a match. See if it's a placeholder frame
          kidFrame->GetFrameType(getter_AddRefs(frameType));
          if (nsLayoutAtoms::placeholderFrame == frameType.get()) {
            // Ignore the placeholder and return the out-of-flow frame instead
            return ((nsPlaceholderFrame*)kidFrame)->GetOutOfFlowFrame();
          } else {
            // Check if kidframe is the :before pseudo frame for aContent. If it
            // is, and aContent is an element, then aContent might be a
            // non-splittable-element, so the real primary frame could be the
            // next sibling.

            if (aContent->IsContentOfType(nsIContent::eELEMENT) &&
                IsGeneratedContentFor(aContent, kidFrame, nsCSSAtoms::beforePseudo)) {
              kidFrame->GetNextSibling(&kidFrame);
#ifdef DEBUG
              NS_ASSERTION(kidFrame, ":before with no next sibling");
              if (kidFrame) {
                nsCOMPtr<nsIContent> nextSiblingContent;
                kidFrame->GetContent(getter_AddRefs(nextSiblingContent));

                // Make sure the content matches, and because I'm paranoid,
                // make sure it's not the :after pseudo frame.

                NS_ASSERTION(nextSiblingContent.get() == aContent &&
                             !IsGeneratedContentFor(aContent, kidFrame,
                                                    nsCSSAtoms::afterPseudo),
                             ":before frame not followed by primary frame");
              }
#endif
            }

            // Return the matching child frame
            return kidFrame;
          }
        }

        // only do this if there is content
        if (kidContent) {
          // We search the immediate children only, but if the child frame has
          // the same content pointer as its parent then we need to search its
          // child frames, too.
          // We also need to search if the child content is anonymous and scoped
          // to the parent content.
          nsCOMPtr<nsIContent> parentScope;
          kidContent->GetBindingParent(getter_AddRefs(parentScope));
          if (aParentContent == kidContent ||
              (aParentContent && (aParentContent == parentScope))) 
          {
#ifdef NOISY_FINDFRAME
            FFWC_recursions++;
            printf("  recursing with new parent set to kidframe=%p, parentContent=%p\n", 
                   kidFrame, aParentContent.get());
#endif
            nsIFrame* matchingFrame =
                FindFrameWithContent(aPresContext, aFrameManager, kidFrame,
                                     aParentContent, aContent, nsnull);

            if (matchingFrame) {
              return matchingFrame;
            }
          }
        }

        // Get the next sibling frame
        kidFrame->GetNextSibling(&kidFrame);
#ifdef NOISY_FINDFRAME
        FFWC_doSibling++;
        if (kidFrame) {
          printf("  searching sibling frame %p\n", kidFrame);
        }
#endif
      }

      if (aHint) {
        // If we get here, and we had a hint, then we didn't find a
        // frame. The hint may have been a floated or absolutely
        // positioned frame, in which case we'd be off in the weeds
        // looking through something other than primary frame
        // list. Reboot the search from scratch, without the hint, but
        // using the null child list again.
        aHint = nsnull;
      } else {
        NS_IF_RELEASE(listName);
        aParentFrame->GetAdditionalChildListName(listIndex++, &listName);
      }
    } while(listName);

    // We didn't find a matching frame. If aFrame has a next-in-flow,
    // then continue looking there
    aParentFrame = GetNifOrSpecialSibling(aFrameManager, aParentFrame);
#ifdef NOISY_FINDFRAME
    if (aParentFrame) {
      FFWC_nextInFlows++;
      printf("  searching NIF frame %p\n", aParentFrame);
    }
#endif
  } while (aParentFrame);

  // No matching frame
  return nsnull;
}

// Request to find the primary frame associated with a given content object.
// This is typically called by the pres shell when there is no mapping in
// the pres shell hash table
NS_IMETHODIMP
nsCSSFrameConstructor::FindPrimaryFrameFor(nsIPresContext*  aPresContext,
                                           nsIFrameManager* aFrameManager,
                                           nsIContent*      aContent,
                                           nsIFrame**       aFrame,
                                           nsFindFrameHint* aHint)
{
  NS_ASSERTION(aPresContext && aFrameManager && aContent && aFrame, "bad arg");

  *aFrame = nsnull;  // initialize OUT parameter 

  // Get the pres shell
  nsCOMPtr<nsIPresShell> presShell;
  aPresContext->GetShell(getter_AddRefs(presShell));

  // We want to be able to quickly map from a content object to its frame,
  // but we also want to keep the hash table small. Therefore, many frames
  // are not added to the hash table when they're first created:
  // - text frames
  // - inline frames (often things like FONT and B)
  // - BR frames
  // - internal table frames (row-group, row, cell, col-group, col)
  //
  // That means we need to need to search for the frame
  nsCOMPtr<nsIContent>   parentContent; // we get this one time
  nsIFrame*              parentFrame;   // this pointer is used to iterate across all frames that map to parentContent

  // Get the frame that corresponds to the parent content object.
  // Note that this may recurse indirectly, because the pres shell will
  // call us back if there is no mapping in the hash table
  aContent->GetParent(*getter_AddRefs(parentContent));
  if (parentContent.get()) {
    aFrameManager->GetPrimaryFrameFor(parentContent, &parentFrame);
    while (parentFrame) {
      // Search the child frames for a match
      *aFrame = FindFrameWithContent(aPresContext, aFrameManager, parentFrame,
                                     parentContent.get(), aContent, aHint);
#ifdef NOISY_FINDFRAME
      printf("FindFrameWithContent returned %p\n", *aFrame);
#endif

#ifdef DEBUG
      // if we're given a hint and we were told to verify, then compare the resulting frame with
      // the frame we get by calling FindFrameWithContent *without* the hint.  
      // Assert if they do not match
      // Note that this makes finding frames *slower* than it was before the fix.
      if (gVerifyFastFindFrame && aHint) 
      {
#ifdef NOISY_FINDFRAME
        printf("VERIFYING...\n");
#endif
        nsIFrame *verifyTestFrame =
            FindFrameWithContent(aPresContext, aFrameManager, parentFrame,
                                 parentContent.get(), aContent, nsnull);
#ifdef NOISY_FINDFRAME
        printf("VERIFY returned %p\n", verifyTestFrame);
#endif
        NS_ASSERTION(verifyTestFrame == *aFrame, "hint shortcut found wrong frame");
      }
#endif
      // If we found a match, then add a mapping to the hash table so
      // next time this will be quick
      if (*aFrame) {
        aFrameManager->SetPrimaryFrameFor(aContent, *aFrame);
        break;
      }
      else if (IsFrameSpecial(parentFrame)) {
        // If it's a "special" frame (that is, part of an inline
        // that's been split because it contained a block), we need to
        // follow the out-of-flow "special sibling" link, and search
        // *that* subtree as well.
        nsIFrame* specialSibling = nsnull;
        GetSpecialSibling(aFrameManager, parentFrame, &specialSibling);
        parentFrame = specialSibling;
      }
      else {
        break;
      }
    }
  }

  if (aHint && !*aFrame)
  { // if we had a hint, and we didn't get a frame, see if we should try the slow way
    nsCOMPtr<nsIAtom>tag;
    aContent->GetTag(*getter_AddRefs(tag));
    if (nsLayoutAtoms::textTagName == tag.get()) 
    {
#ifdef NOISY_FINDFRAME
      FFWC_slowSearchForText++;
#endif
      // since we're passing in a null hint, we're guaranteed to only recurse once
      return FindPrimaryFrameFor(aPresContext, aFrameManager, aContent, aFrame, nsnull);
    }
  }

#ifdef NOISY_FINDFRAME
  printf("%10s %10s %10s %10s %10s \n", 
         "total", "doLoop", "doSibling", "recur", "nextIF", "slowSearch");
  printf("%10d %10d %10d %10d %10d \n", 
         FFWC_totalCount, FFWC_doLoop, FFWC_doSibling, FFWC_recursions, 
         FFWC_nextInFlows, FFWC_slowSearchForText);
#endif

  return NS_OK;
}

NS_IMETHODIMP
nsCSSFrameConstructor::GetInsertionPoint(nsIPresShell* aPresShell,
                                         nsIFrame*     aParentFrame,
                                         nsIContent*   aChildContent,
                                         nsIFrame**    aInsertionPoint,
                                         PRBool*       aMultiple)
{
  // Make the insertion point be the parent frame by default, in case
  // we have to bail early.
  *aInsertionPoint = aParentFrame;

  nsCOMPtr<nsIContent> container;
  aParentFrame->GetContent(getter_AddRefs(container));
  if (!container)
    return NS_OK;

  nsCOMPtr<nsIDocument> document;
  container->GetDocument(*getter_AddRefs(document));
  if (!document)
    return NS_OK;

  nsCOMPtr<nsIBindingManager> bindingManager;
  document->GetBindingManager(getter_AddRefs(bindingManager));
  if (!bindingManager)
    return NS_OK;

  nsCOMPtr<nsIContent> insertionElement;
  if (aChildContent) {
    // We've got an explicit insertion child. Check to see if it's
    // anonymous.
    nsCOMPtr<nsIContent> bindingParent;
    aChildContent->GetBindingParent(getter_AddRefs(bindingParent));
    if (bindingParent == container) {
      // This child content is anonymous. Don't use the insertion
      // point, since that's only for the explicit kids.
      return NS_OK;
    }

    PRUint32 index;
    bindingManager->GetInsertionPoint(container, aChildContent, getter_AddRefs(insertionElement), &index);
  }
  else {
    PRBool multiple;
    PRUint32 index;
    bindingManager->GetSingleInsertionPoint(container, getter_AddRefs(insertionElement), &index, &multiple);
    if (multiple && aMultiple)
      *aMultiple = multiple; // Record the fact that filters are in use.
  }

  if (insertionElement) {
    nsIFrame* insertionPoint = nsnull;
    aPresShell->GetPrimaryFrameFor(insertionElement, &insertionPoint);
    if (insertionPoint) {
      // If the insertion point is a scrollable, then walk ``through''
      // it to get the scrolled frame.
      nsIScrollableFrame* scroll = nsnull;
      CallQueryInterface(insertionPoint, &scroll);
      if (scroll)
        scroll->GetScrolledFrame(nsnull, insertionPoint);

      if (insertionPoint != aParentFrame) 
        GetInsertionPoint(aPresShell, insertionPoint, aChildContent, aInsertionPoint, aMultiple);
    }
    else {
      // There was no frame created yet for the insertion point.
      *aInsertionPoint = nsnull;
    }
  }

  return NS_OK;
}

// Capture state for the frame tree rooted at the frame associated with the
// content object, aContent
nsresult
nsCSSFrameConstructor::CaptureStateForFramesOf(nsIPresContext* aPresContext,
                                               nsIContent* aContent,
                                               nsILayoutHistoryState* aHistoryState)
{
  nsCOMPtr<nsIPresShell> presShell;
  nsresult rv = NS_OK;

  rv = aPresContext->GetShell(getter_AddRefs(presShell));
  if (NS_SUCCEEDED(rv) && presShell) {                    
    nsIFrame* frame;
    rv = presShell->GetPrimaryFrameFor(aContent, &frame);
    if (NS_SUCCEEDED(rv) && frame) {
      CaptureStateFor(aPresContext, frame, aHistoryState);
    }
  }
  return rv;
}

// Capture state for the frame tree rooted at aFrame.
nsresult
nsCSSFrameConstructor::CaptureStateFor(nsIPresContext* aPresContext,
                                       nsIFrame* aFrame,
                                       nsILayoutHistoryState* aHistoryState)
{
  nsresult rv = NS_OK;

  if (aFrame && aPresContext && aHistoryState) {
    nsCOMPtr<nsIPresShell> presShell;
    rv = aPresContext->GetShell(getter_AddRefs(presShell));
    if (NS_SUCCEEDED(rv) && presShell) {                    
      nsCOMPtr<nsIFrameManager> frameManager;
      rv = presShell->GetFrameManager(getter_AddRefs(frameManager));
      if (NS_SUCCEEDED(rv) && frameManager) {
        rv = frameManager->CaptureFrameState(aPresContext, aFrame, aHistoryState);
      }
    }
  }
  return rv;
}

nsresult
nsCSSFrameConstructor::RecreateFramesForContent(nsIPresContext* aPresContext,
                                                nsIContent* aContent, PRBool aInlineStyle,
                                                nsIStyleRule* aInlineStyleRule,
                                                nsIStyleContext* aStyleContext)                                   
{
  // Is the frame `special'? If so, we need to reframe the containing
  // block *here*, rather than trying to remove and re-insert the
  // content (which would otherwise result in *two* nested reframe
  // containing block from ContentRemoved() and ContentInserted(),
  // below!)
  nsCOMPtr<nsIPresShell> shell;
  aPresContext->GetShell(getter_AddRefs(shell));

  nsIFrame* frame;
  shell->GetPrimaryFrameFor(aContent, &frame);

  if (frame && IsFrameSpecial(frame)) {
#ifdef DEBUG
    if (gNoisyContentUpdates) {
      printf("nsCSSFrameConstructor::RecreateFramesForContent: frame=");
      nsFrame::ListTag(stdout, frame);
      printf(" is special\n");
    }
#endif
    return ReframeContainingBlock(aPresContext, frame);
  }

  nsresult rv = NS_OK;
  nsCOMPtr<nsIContent> container;
  aContent->GetParent(*getter_AddRefs(container));
  if (container) {
    PRInt32 indexInContainer;    
    rv = container->IndexOf(aContent, indexInContainer);
    if (NS_SUCCEEDED(rv)) {
      // Before removing the frames associated with the content object, ask them to save their
      // state onto a temporary state object.
      CaptureStateForFramesOf(aPresContext, aContent, mTempFrameTreeState);

      // Remove the frames associated with the content object on which the
      // attribute change occurred.
      rv = ContentRemoved(aPresContext, container, aContent, indexInContainer, PR_FALSE);

      // Now that the old frame is gone (and has stopped depending on obsolete style
      // data), we need to blow away our style information if this reframe happened as
      // a result of an inline style attribute changing.
      if (aInlineStyle) {
        nsCOMPtr<nsIStyleSet> set;
        shell->GetStyleSet(getter_AddRefs(set));
        // XXXldb If |aStyleContext| is null, wouldn't it be faster to pass
        // in something to tell it that this change is for inline style?
        set->ClearStyleData(aPresContext, aInlineStyleRule, aStyleContext);
      }
    
      if (NS_SUCCEEDED(rv)) {
        // Now, recreate the frames associated with this content object.
        rv = ContentInserted(aPresContext, container, aContent, indexInContainer, mTempFrameTreeState, PR_FALSE);
      }      
    }
  } else {
    // The content is the root node, so just rebuild the world.
    // However, double check that it's really part of the document,
    // since rebuilding the frame tree can have bad effects, especially
    // if it's the frame tree for chrome (see bug 157322).
    nsCOMPtr<nsIDocument> doc;
    aContent->GetDocument(*getter_AddRefs(doc));
    NS_ASSERTION(doc, "received style change for content not in document");
    if (doc)
      ReconstructDocElementHierarchy(aPresContext);
  }
  return rv;
}

//////////////////////////////////////////////////////////////////////

// Block frame construction code

nsIStyleContext*
nsCSSFrameConstructor::GetFirstLetterStyle(nsIPresContext* aPresContext,
                                           nsIContent* aContent,
                                           nsIStyleContext* aStyleContext)
{
  nsIStyleContext* fls = nsnull;
  if (aContent) {
    aPresContext->ResolvePseudoStyleContextFor(aContent,
                                               nsHTMLAtoms::firstLetterPseudo,
                                               aStyleContext, &fls);
  }
  return fls;
}

nsIStyleContext*
nsCSSFrameConstructor::GetFirstLineStyle(nsIPresContext* aPresContext,
                                         nsIContent* aContent,
                                         nsIStyleContext* aStyleContext)
{
  nsIStyleContext* fls = nsnull;
  if (aContent) {
    aPresContext->ResolvePseudoStyleContextFor(aContent,
                                               nsHTMLAtoms::firstLinePseudo,
                                               aStyleContext, &fls);
  }
  return fls;
}

// Predicate to see if a given content (block element) has
// first-letter style applied to it.
PRBool
nsCSSFrameConstructor::HaveFirstLetterStyle(nsIPresContext* aPresContext,
                                            nsIContent* aContent,
                                            nsIStyleContext* aStyleContext)
{
  return HasPseudoStyle(aPresContext, aContent, aStyleContext,
                        nsHTMLAtoms::firstLetterPseudo);
}

PRBool
nsCSSFrameConstructor::HaveFirstLineStyle(nsIPresContext* aPresContext,
                                          nsIContent* aContent,
                                          nsIStyleContext* aStyleContext)
{
  return HasPseudoStyle(aPresContext, aContent, aStyleContext,
                        nsHTMLAtoms::firstLinePseudo);
}

void
nsCSSFrameConstructor::HaveSpecialBlockStyle(nsIPresContext* aPresContext,
                                             nsIContent* aContent,
                                             nsIStyleContext* aStyleContext,
                                             PRBool* aHaveFirstLetterStyle,
                                             PRBool* aHaveFirstLineStyle)
{
  *aHaveFirstLetterStyle =
    HaveFirstLetterStyle(aPresContext, aContent, aStyleContext);
  *aHaveFirstLineStyle =
    HaveFirstLineStyle(aPresContext, aContent, aStyleContext);
}

/**
 * Request to process the child content elements and create frames.
 *
 * @param   aContent the content object whose child elements to process
 * @param   aFrame the the associated with aContent. This will be the
 *            parent frame (both content and geometric) for the flowed
 *            child frames
 */
nsresult
nsCSSFrameConstructor::ProcessChildren(nsIPresShell*            aPresShell, 
                                       nsIPresContext*          aPresContext,
                                       nsFrameConstructorState& aState,
                                       nsIContent*              aContent,
                                       nsIFrame*                aFrame,
                                       PRBool                   aCanHaveGeneratedContent,
                                       nsFrameItems&            aFrameItems,
                                       PRBool                   aParentIsBlock,
                                       nsTableCreator*          aTableCreator)
{
  nsresult rv = NS_OK;
  nsCOMPtr<nsIStyleContext> styleContext;
  aFrame->GetStyleContext(getter_AddRefs(styleContext));
    
  if (aCanHaveGeneratedContent) {
    // Probe for generated content before
    nsIFrame* generatedFrame;
    if (CreateGeneratedContentFrame(aPresShell, aPresContext, aState, aFrame, aContent,
                                    styleContext, nsCSSAtoms::beforePseudo,
                                    nsnull, &generatedFrame)) {
      // Add the generated frame to the child list
      aFrameItems.AddChild(generatedFrame);
    }
  }

  if (aTableCreator) { // do special table child processing
    // if there is a caption child here, it gets recorded in aState.mPseudoFrames.
    nsIFrame* captionFrame;
    TableProcessChildren(aPresShell, aPresContext, aState, aContent, aFrame, 
                         *aTableCreator, aFrameItems, captionFrame);
  }
  else {
    // save the incoming pseudo frame state 
    nsPseudoFrames priorPseudoFrames; 
    aState.mPseudoFrames.Reset(&priorPseudoFrames);

    ChildIterator iter, last;
    for (ChildIterator::Init(aContent, &iter, &last);
         iter != last;
         ++iter) {
      rv = ConstructFrame(aPresShell, aPresContext, aState, nsCOMPtr<nsIContent>(*iter),
                          aFrame, aFrameItems);

      if (NS_FAILED(rv))
        return rv;
    }

    // process the current pseudo frame state
    if (!aState.mPseudoFrames.IsEmpty()) {
      ProcessPseudoFrames(aPresContext, aState.mPseudoFrames, aFrameItems);
    }

    // restore the incoming pseudo frame state 
    aState.mPseudoFrames = priorPseudoFrames;
  }

  if (aCanHaveGeneratedContent) {
    // Probe for generated content after
    nsIFrame* generatedFrame;
    if (CreateGeneratedContentFrame(aPresShell, aPresContext, aState, aFrame, aContent,
                                    styleContext, nsCSSAtoms::afterPseudo,
                                    nsnull, &generatedFrame)) {
      // Add the generated frame to the child list
      aFrameItems.AddChild(generatedFrame);
    }
  }

  if (aParentIsBlock) {
    if (aState.mFirstLetterStyle) {
      rv = WrapFramesInFirstLetterFrame(aPresShell, aPresContext, aState, aContent, aFrame, aFrameItems);
    }
    if (aState.mFirstLineStyle) {
      rv = WrapFramesInFirstLineFrame(aPresShell, aPresContext, aState, aContent, aFrame, aFrameItems);
    }
  }

  return rv;
}

//----------------------------------------------------------------------

// Support for :first-line style

static void
ReparentFrame(nsIPresContext* aPresContext,
              nsIFrame* aNewParentFrame,
              nsIStyleContext* aParentStyleContext,
              nsIFrame* aFrame)
{
  aPresContext->ReParentStyleContext(aFrame, aParentStyleContext);
  aFrame->SetParent(aNewParentFrame);
}

// Special routine to handle placing a list of frames into a block
// frame that has first-line style. The routine ensures that the first
// collection of inline frames end up in a first-line frame.
nsresult
nsCSSFrameConstructor::WrapFramesInFirstLineFrame(
  nsIPresShell* aPresShell, 
  nsIPresContext*          aPresContext,
  nsFrameConstructorState& aState,
  nsIContent*              aContent,
  nsIFrame*                aFrame,
  nsFrameItems&            aFrameItems)
{
  nsresult rv = NS_OK;

  // Find the first and last inline frame in aFrameItems
  nsIFrame* kid = aFrameItems.childList;
  nsIFrame* firstInlineFrame = nsnull;
  nsIFrame* lastInlineFrame = nsnull;
  while (kid) {
    if (IsInlineFrame(kid)) {
      if (!firstInlineFrame) firstInlineFrame = kid;
      lastInlineFrame = kid;
    }
    else {
      break;
    }
    kid->GetNextSibling(&kid);
  }

  // If we don't find any inline frames, then there is nothing to do
  if (!firstInlineFrame) {
    return rv;
  }

  // Create line frame
  nsCOMPtr<nsIStyleContext> parentStyle;
  aFrame->GetStyleContext(getter_AddRefs(parentStyle));
  nsCOMPtr<nsIStyleContext> firstLineStyle(
    getter_AddRefs(GetFirstLineStyle(aPresContext, aContent, parentStyle))
    );
  nsIFrame* lineFrame;
  rv = NS_NewFirstLineFrame(aPresShell, &lineFrame);
  if (NS_SUCCEEDED(rv)) {
    // Initialize the line frame
    rv = InitAndRestoreFrame(aPresContext, aState, aContent, 
                      aFrame, firstLineStyle, nsnull, lineFrame);    

    // Mangle the list of frames we are giving to the block: first
    // chop the list in two after lastInlineFrame
    nsIFrame* secondBlockFrame;
    lastInlineFrame->GetNextSibling(&secondBlockFrame);
    lastInlineFrame->SetNextSibling(nsnull);

    // The lineFrame will be the block's first child; the rest of the
    // frame list (after lastInlineFrame) will be the second and
    // subsequent children; join the list together and reset
    // aFrameItems appropriately.
    if (secondBlockFrame) {
      lineFrame->SetNextSibling(secondBlockFrame);
    }
    if (aFrameItems.childList == lastInlineFrame) {
      // Just in case the block had exactly one inline child
      aFrameItems.lastChild = lineFrame;
    }
    aFrameItems.childList = lineFrame;

    // Give the inline frames to the lineFrame <b>after</b> reparenting them
    kid = firstInlineFrame;
    while (kid) {
      ReparentFrame(aPresContext, lineFrame, firstLineStyle, kid);
      kid->GetNextSibling(&kid);
    }
    lineFrame->SetInitialChildList(aPresContext, nsnull, firstInlineFrame);
  }

  return rv;
}

// Special routine to handle appending a new frame to a block frame's
// child list. Takes care of placing the new frame into the right
// place when first-line style is present.
nsresult
nsCSSFrameConstructor::AppendFirstLineFrames(
  nsIPresShell* aPresShell, 
  nsIPresContext*          aPresContext,
  nsFrameConstructorState& aState,
  nsIContent*              aContent,
  nsIFrame*                aBlockFrame,
  nsFrameItems&            aFrameItems)
{
  // It's possible that aBlockFrame needs to have a first-line frame
  // created because it doesn't currently have any children.
  nsIFrame* blockKid;
  aBlockFrame->FirstChild(aPresContext, nsnull, &blockKid);
  if (!blockKid) {
    return WrapFramesInFirstLineFrame(aPresShell, aPresContext, aState, aContent,
                                      aBlockFrame, aFrameItems);
  }

  // Examine the last block child - if it's a first-line frame then
  // appended frames need special treatment.
  nsresult rv = NS_OK;
  nsFrameList blockFrames(blockKid);
  nsIFrame* lastBlockKid = blockFrames.LastChild();
  nsCOMPtr<nsIAtom> frameType;
  lastBlockKid->GetFrameType(getter_AddRefs(frameType));
  if (frameType.get() != nsLayoutAtoms::lineFrame) {
    // No first-line frame at the end of the list, therefore there is
    // an interveening block between any first-line frame the frames
    // we are appending. Therefore, we don't need any special
    // treatment of the appended frames.
    return rv;
  }
  nsIFrame* lineFrame = lastBlockKid;
  nsCOMPtr<nsIStyleContext> firstLineStyle;
  lineFrame->GetStyleContext(getter_AddRefs(firstLineStyle));

  // Find the first and last inline frame in aFrameItems
  nsIFrame* kid = aFrameItems.childList;
  nsIFrame* firstInlineFrame = nsnull;
  nsIFrame* lastInlineFrame = nsnull;
  while (kid) {
    if (IsInlineFrame(kid)) {
      if (!firstInlineFrame) firstInlineFrame = kid;
      lastInlineFrame = kid;
    }
    else {
      break;
    }
    kid->GetNextSibling(&kid);
  }

  // If we don't find any inline frames, then there is nothing to do
  if (!firstInlineFrame) {
    return rv;
  }

  // The inline frames get appended to the lineFrame. Make sure they
  // are reparented properly.
  nsIFrame* remainingFrames;
  lastInlineFrame->GetNextSibling(&remainingFrames);
  lastInlineFrame->SetNextSibling(nsnull);
  kid = firstInlineFrame;
  while (kid) {
    ReparentFrame(aPresContext, lineFrame, firstLineStyle, kid);
    kid->GetNextSibling(&kid);
  }
  aState.mFrameManager->AppendFrames(aPresContext, *aState.mPresShell,
                                     lineFrame, nsnull, firstInlineFrame);

  // The remaining frames get appended to the block frame
  if (remainingFrames) {
    aFrameItems.childList = remainingFrames;
  }
  else {
    aFrameItems.childList = nsnull;
    aFrameItems.lastChild = nsnull;
  }

  return rv;
}

// Special routine to handle inserting a new frame into a block
// frame's child list. Takes care of placing the new frame into the
// right place when first-line style is present.
nsresult
nsCSSFrameConstructor::InsertFirstLineFrames(
  nsIPresContext*          aPresContext,
  nsFrameConstructorState& aState,
  nsIContent*              aContent,
  nsIFrame*                aBlockFrame,
  nsIFrame**               aParentFrame,
  nsIFrame*                aPrevSibling,
  nsFrameItems&            aFrameItems)
{
  nsresult rv = NS_OK;
#if 0
  nsIFrame* parentFrame = *aParentFrame;
  nsIFrame* newFrame = aFrameItems.childList;
  PRBool isInline = IsInlineFrame(newFrame);

  if (!aPrevSibling) {
    // Insertion will become the first frame. Two cases: we either
    // already have a first-line frame or we don't.
    nsIFrame* firstBlockKid;
    aBlockFrame->FirstChild(nsnull, &firstBlockKid);
    nsCOMPtr<nsIAtom> frameType;
    firstBlockKid->GetFrameType(getter_AddRefs(frameType));
    if (frameType.get() == nsLayoutAtoms::lineFrame) {
      // We already have a first-line frame
      nsIFrame* lineFrame = firstBlockKid;
      nsCOMPtr<nsIStyleContext> firstLineStyle;
      lineFrame->GetStyleContext(getter_AddRefs(firstLineStyle));

      if (isInline) {
        // Easy case: the new inline frame will go into the lineFrame.
        ReparentFrame(aPresContext, lineFrame, firstLineStyle, newFrame);
        aState.mFrameManager->InsertFrames(aPresContext, *aState.mPresShell,
                                           lineFrame, nsnull, nsnull,
                                           newFrame);

        // Since the frame is going into the lineFrame, don't let it
        // go into the block too.
        aFrameItems.childList = nsnull;
        aFrameItems.lastChild = nsnull;
      }
      else {
        // Harder case: We are about to insert a block level element
        // before the first-line frame.
        // XXX need a method to steal away frames from the line-frame
      }
    }
    else {
      // We do not have a first-line frame
      if (isInline) {
        // We now need a first-line frame to contain the inline frame.
        nsIFrame* lineFrame;
        rv = NS_NewFirstLineFrame(&lineFrame);
        if (NS_SUCCEEDED(rv)) {
          // Lookup first-line style context
          nsCOMPtr<nsIStyleContext> parentStyle;
          aBlockFrame->GetStyleContext(getter_AddRefs(parentStyle));
          nsCOMPtr<nsIStyleContext> firstLineStyle(
            getter_AddRefs(GetFirstLineStyle(aPresContext, aContent,
                                             parentStyle))
            );

          // Initialize the line frame
          rv = InitAndRestoreFrame(aPresContext, aState, aContent, 
                      aBlockFrame, firstLineStyle, nsnull, lineFrame);

          // Make sure the caller inserts the lineFrame into the
          // blocks list of children.
          aFrameItems.childList = lineFrame;
          aFrameItems.lastChild = lineFrame;

          // Give the inline frames to the lineFrame <b>after</b>
          // reparenting them
          ReparentFrame(aPresContext, lineFrame, firstLineStyle, newFrame);
          lineFrame->SetInitialChildList(aPresContext, nsnull, newFrame);
        }
      }
      else {
        // Easy case: the regular insertion logic can insert the new
        // frame because its a block frame.
      }
    }
  }
  else {
    // Insertion will not be the first frame.
    nsIFrame* prevSiblingParent;
    aPrevSibling->GetParent(&prevSiblingParent);
    if (prevSiblingParent == aBlockFrame) {
      // Easy case: The prev-siblings parent is the block
      // frame. Therefore the prev-sibling is not currently in a
      // line-frame. Therefore the new frame which is going after it,
      // regardless of type, is not going into a line-frame.
    }
    else {
      // If the prevSiblingParent is not the block-frame then it must
      // be a line-frame (if it were a letter-frame, that logic would
      // already have adjusted the prev-sibling to be the
      // letter-frame).
      if (isInline) {
        // Easy case: the insertion can go where the caller thinks it
        // should go (which is into prevSiblingParent).
      }
      else {
        // Block elements don't end up in line-frames, therefore
        // change the insertion point to aBlockFrame. However, there
        // might be more inline elements following aPrevSibling that
        // need to be pulled out of the line-frame and become children
        // of the block.
        nsIFrame* nextSibling;
        aPrevSibling->GetNextSibling(&nextSibling);
        nsIFrame* nextLineFrame;
        prevSiblingParent->GetNextInFlow(&nextLineFrame);
        if (nextSibling || nextLineFrame) {
          // Oy. We have work to do. Create a list of the new frames
          // that are going into the block by stripping them away from
          // the line-frame(s).
          nsFrameList list(nextSibling);
          if (nextSibling) {
            nsLineFrame* lineFrame = (nsLineFrame*) prevSiblingParent;
            lineFrame->StealFramesFrom(nextSibling);
          }

          nsLineFrame* nextLineFrame = (nsLineFrame*) lineFrame;
          for (;;) {
            nextLineFrame->GetNextInFlow(&nextLineFrame);
            if (!nextLineFrame) {
              break;
            }
            nsIFrame* kids;
            nextLineFrame->FirstChild(nsnull, &kids);
          }
        }
        else {
          // We got lucky: aPrevSibling was the last inline frame in
          // the line-frame.
          ReparentFrame(aPresContext, aBlockFrame, firstLineStyle, newFrame);
          aState.mFrameManager->InsertFrames(aPresContext, *aState.mPresShell,
                                             aBlockFrame, nsnull,
                                             prevSiblingParent, newFrame);
          aFrameItems.childList = nsnull;
          aFrameItems.lastChild = nsnull;
        }
      }
    }
  }

#endif
  return rv;
}

//----------------------------------------------------------------------

// First-letter support

// Determine how many characters in the text fragment apply to the
// first letter
static PRInt32
FirstLetterCount(const nsTextFragment* aFragment)
{
  PRInt32 count = 0;
  PRInt32 firstLetterLength = 0;
  PRBool done = PR_FALSE;

  PRInt32 i, n = aFragment->GetLength();
  for (i = 0; i < n; i++) {
    PRUnichar ch = aFragment->CharAt(i);
    if (XP_IS_SPACE(ch)) {
      if (firstLetterLength) {
        done = PR_TRUE;
        break;
      }
      count++;
      continue;
    }
    // XXX I18n
    if ((ch == '\'') || (ch == '\"')) {
      if (firstLetterLength) {
        done = PR_TRUE;
        break;
      }
      // keep looping
      firstLetterLength = 1;
    }
    else {
      count++;
      done = PR_TRUE;
      break;
    }
  }

  return count;
}

static PRBool
NeedFirstLetterContinuation(nsIContent* aContent)
{
  NS_PRECONDITION(aContent, "null ptr");

  PRBool result = PR_FALSE;
  if (aContent) {
    nsCOMPtr<nsITextContent> tc(do_QueryInterface(aContent));
    if (tc) {
      const nsTextFragment* frag = nsnull;
      tc->GetText(&frag);
      PRInt32 flc = FirstLetterCount(frag);
      PRInt32 tl = frag->GetLength();
      if (flc < tl) {
        result = PR_TRUE;
      }
    }
  }
  return result;
}

static PRBool IsFirstLetterContent(nsIContent* aContent)
{
  PRBool result = PR_FALSE;

  nsCOMPtr<nsITextContent> textContent = do_QueryInterface(aContent);
  if (textContent) {
    PRInt32 textLength;
    textContent->GetTextLength(&textLength);
    if (textLength) {
      PRBool onlyWhiteSpace;
      textContent->IsOnlyWhitespace(&onlyWhiteSpace);
      result = !onlyWhiteSpace;
    }
  }

  return result;
}

/**
 * Create a letter frame, only make it a floating frame.
 */
void
nsCSSFrameConstructor::CreateFloatingLetterFrame(
  nsIPresShell* aPresShell, 
  nsIPresContext* aPresContext,
  nsFrameConstructorState& aState,
  nsIContent* aTextContent,
  nsIFrame* aTextFrame,
  nsIContent* aBlockContent,
  nsIFrame* aParentFrame,
  nsIStyleContext* aStyleContext,
  nsFrameItems& aResult)
{
  // Create the first-letter-frame
  nsIFrame* letterFrame;

  NS_NewFirstLetterFrame(aPresShell, &letterFrame);  
  InitAndRestoreFrame(aPresContext, aState, aTextContent, 
                      aParentFrame, aStyleContext, nsnull, letterFrame);

  // Init the text frame to refer to the letter frame. Make sure we
  // get a proper style context for it (the one passed in is for the
  // letter frame and will have the float property set on it; the text
  // frame shouldn't have that set).
  nsCOMPtr<nsIStyleContext> textSC;
  aPresContext->ResolveStyleContextForNonElement(aStyleContext,
                                                 getter_AddRefs(textSC));  
  InitAndRestoreFrame(aPresContext, aState, aTextContent, 
                      letterFrame, textSC, nsnull, aTextFrame);

  // And then give the text frame to the letter frame
  letterFrame->SetInitialChildList(aPresContext, nsnull, aTextFrame);

  // Now make the placeholder
  nsIFrame* placeholderFrame;
  CreatePlaceholderFrameFor(aPresShell, aPresContext, aState.mFrameManager,
                            aTextContent, letterFrame,
                            aStyleContext, aParentFrame,
                            &placeholderFrame);

  // See if we will need to continue the text frame (does it contain
  // more than just the first-letter text or not?) If it does, then we
  // create (in advance) a continuation frame for it.
  nsIFrame* nextTextFrame = nsnull;
  if (NeedFirstLetterContinuation(aTextContent)) {
    // Create continuation
    CreateContinuingFrame(aPresShell, aPresContext, aTextFrame, aParentFrame,
                          &nextTextFrame);

    // Repair the continuations style context
    nsCOMPtr<nsIStyleContext> parentStyleContext;
    parentStyleContext = getter_AddRefs(aStyleContext->GetParent());
    if (parentStyleContext) {
      nsCOMPtr<nsIStyleContext> newSC;
      aPresContext->ResolveStyleContextForNonElement(parentStyleContext,
                                                     getter_AddRefs(newSC));
      if (newSC) {
        nextTextFrame->SetStyleContext(aPresContext, newSC);
      }
    }
  }

  // Update the child lists for the frame containing the floating first
  // letter frame.
  aState.mFloatedItems.AddChild(letterFrame);
  aResult.childList = aResult.lastChild = placeholderFrame;
  if (nextTextFrame) {
    aResult.AddChild(nextTextFrame);
  }
}

/**
 * Create a new letter frame for aTextFrame. The letter frame will be
 * a child of aParentFrame.
 */
nsresult
nsCSSFrameConstructor::CreateLetterFrame(nsIPresShell* aPresShell, nsIPresContext* aPresContext,
                                         nsFrameConstructorState& aState,
                                         nsIContent* aTextContent,
                                         nsIFrame* aParentFrame,
                                         nsFrameItems& aResult)
{
  NS_PRECONDITION(aTextContent->IsContentOfType(nsIContent::eTEXT),
                  "aTextContent isn't text");
  nsCOMPtr<nsIContent> parentContent;
  aParentFrame->GetContent(getter_AddRefs(parentContent));

  // Get style context for the first-letter-frame
  nsCOMPtr<nsIStyleContext> parentStyleContext;
  aParentFrame->GetStyleContext(getter_AddRefs(parentStyleContext));
  if (parentStyleContext) {
    // Use content from containing block so that we can actually
    // find a matching style rule.
    nsCOMPtr<nsIContent> blockContent;
    aState.mFloatedItems.containingBlock->GetContent(
      getter_AddRefs(blockContent));

    // Create first-letter style rule
    nsCOMPtr<nsIStyleContext> sc = getter_AddRefs(
      GetFirstLetterStyle(aPresContext, blockContent, parentStyleContext));
    if (sc) {
      // Create a new text frame (the original one will be discarded)
      nsIFrame* textFrame;
      NS_NewTextFrame(aPresShell, &textFrame);

      // Create the right type of first-letter frame
      const nsStyleDisplay* display = (const nsStyleDisplay*)
        sc->GetStyleData(eStyleStruct_Display);
      if (display->IsFloating()) {
        // Make a floating first-letter frame
        CreateFloatingLetterFrame(aPresShell, aPresContext, aState,
                                  aTextContent, textFrame,
                                  blockContent, aParentFrame,
                                  sc, aResult);
      }
      else {
        // Make an inflow first-letter frame
        nsIFrame* letterFrame;
        nsresult rv = NS_NewFirstLetterFrame(aPresShell, &letterFrame);
        if (NS_SUCCEEDED(rv)) {
          // Initialize the first-letter-frame.
          letterFrame->Init(aPresContext, aTextContent, aParentFrame,
                            sc, nsnull);
          nsCOMPtr<nsIStyleContext> textSC;
          aPresContext->ResolveStyleContextForNonElement(sc,
                                                       getter_AddRefs(textSC));
          InitAndRestoreFrame(aPresContext, aState, aTextContent, 
                              letterFrame, textSC, nsnull, textFrame);          
          letterFrame->SetInitialChildList(aPresContext, nsnull, textFrame);
          aResult.childList = aResult.lastChild = letterFrame;
        }
      }
    }
  }

  return NS_OK;
}

nsresult
nsCSSFrameConstructor::WrapFramesInFirstLetterFrame(
  nsIPresShell*            aPresShell, 
  nsIPresContext*          aPresContext,
  nsFrameConstructorState& aState,
  nsIContent*              aBlockContent,
  nsIFrame*                aBlockFrame,
  nsFrameItems&            aBlockFrames)
{
  nsresult rv = NS_OK;

  nsIFrame* parentFrame = nsnull;
  nsIFrame* textFrame = nsnull;
  nsIFrame* prevFrame = nsnull;
  nsFrameItems letterFrames;
  PRBool stopLooking = PR_FALSE;
  rv = WrapFramesInFirstLetterFrame(aPresShell, aPresContext, aState,
                                    aBlockFrame, aBlockFrames.childList,
                                    &parentFrame, &textFrame, &prevFrame,
                                    letterFrames, &stopLooking);
  if (NS_FAILED(rv)) {
    return rv;
  }
  if (parentFrame) {
    if (parentFrame == aBlockFrame) {
      // Text textFrame out of the blocks frame list and substitute the
      // letter frame(s) instead.
      nsIFrame* nextSibling;
      textFrame->GetNextSibling(&nextSibling);
      textFrame->SetNextSibling(nsnull);
      if (prevFrame) {
        prevFrame->SetNextSibling(letterFrames.childList);
      }
      else {
        aBlockFrames.childList = letterFrames.childList;
      }
      letterFrames.lastChild->SetNextSibling(nextSibling);

      // Destroy the old textFrame
      textFrame->Destroy(aPresContext);

      // Repair lastChild; the only time this needs to happen is when
      // the block had one child (the text frame).
      if (!nextSibling) {
        aBlockFrames.lastChild = letterFrames.lastChild;
      }
    }
    else {
      // Take the old textFrame out of the inline parents child list
      DeletingFrameSubtree(aPresContext, aState.mPresShell, 
                           aState.mFrameManager, textFrame);
      parentFrame->RemoveFrame(aPresContext, *aState.mPresShell.get(),
                               nsnull, textFrame);

      // Insert in the letter frame(s)
      parentFrame->InsertFrames(aPresContext, *aState.mPresShell.get(),
                                nsnull, prevFrame, letterFrames.childList);
    }
  }

  return rv;
}

nsresult
nsCSSFrameConstructor::WrapFramesInFirstLetterFrame(
  nsIPresShell*            aPresShell, 
  nsIPresContext*          aPresContext,
  nsFrameConstructorState& aState,
  nsIFrame*                aParentFrame,
  nsIFrame*                aParentFrameList,
  nsIFrame**               aModifiedParent,
  nsIFrame**               aTextFrame,
  nsIFrame**               aPrevFrame,
  nsFrameItems&            aLetterFrames,
  PRBool*                  aStopLooking)
{
  nsresult rv = NS_OK;

  nsIFrame* prevFrame = nsnull;
  nsIFrame* frame = aParentFrameList;

  while (frame) {
    nsIFrame* nextFrame;
    frame->GetNextSibling(&nextFrame);

    nsCOMPtr<nsIAtom> frameType;
    frame->GetFrameType(getter_AddRefs(frameType));
    if (nsLayoutAtoms::textFrame == frameType.get()) {
      // Wrap up first-letter content in a letter frame
      nsCOMPtr<nsIContent> textContent;
      frame->GetContent(getter_AddRefs(textContent));
      if (IsFirstLetterContent(textContent)) {
        // Create letter frame to wrap up the text
        rv = CreateLetterFrame(aPresShell, aPresContext, aState, textContent,
                               aParentFrame, aLetterFrames);
        if (NS_FAILED(rv)) {
          return rv;
        }

        // Provide adjustment information for parent
        *aModifiedParent = aParentFrame;
        *aTextFrame = frame;
        *aPrevFrame = prevFrame;
        *aStopLooking = PR_TRUE;
        return NS_OK;
      }
    }
    else if ((nsLayoutAtoms::inlineFrame == frameType.get()) ||
             (nsLayoutAtoms::lineFrame == frameType.get())) {
      nsIFrame* kids;
      frame->FirstChild(aPresContext, nsnull, &kids);
      WrapFramesInFirstLetterFrame(aPresShell, aPresContext, aState, frame, kids,
                                   aModifiedParent, aTextFrame,
                                   aPrevFrame, aLetterFrames, aStopLooking);
      if (*aStopLooking) {
        return NS_OK;
      }
    }
    else {
      // This will stop us looking to create more letter frames. For
      // example, maybe the frame-type is "letterFrame" or
      // "placeholderFrame". This keeps us from creating extra letter
      // frames, and also prevents us from creating letter frames when
      // the first real content child of a block is not text (e.g. an
      // image, hr, etc.)
      *aStopLooking = PR_TRUE;
      break;
    }

    prevFrame = frame;
    frame = nextFrame;
  }

  return rv;
}

nsresult
nsCSSFrameConstructor::RemoveFloatingFirstLetterFrames(
  nsIPresContext* aPresContext,
  nsIPresShell* aPresShell,
  nsIFrameManager* aFrameManager,
  nsIFrame* aBlockFrame,
  PRBool* aStopLooking)
{
  // First look for the floater frame that is a letter frame
  nsIFrame* floater;
  aBlockFrame->FirstChild(aPresContext, nsLayoutAtoms::floaterList, &floater);
  while (floater) {
    // See if we found a floating letter frame
    nsCOMPtr<nsIAtom> frameType;
    floater->GetFrameType(getter_AddRefs(frameType));
    if (nsLayoutAtoms::letterFrame == frameType.get()) {
      break;
    }
    floater->GetNextSibling(&floater);
  }
  if (!floater) {
    // No such frame
    return NS_OK;
  }

  // Take the text frame away from the letter frame (so it isn't
  // destroyed when we destroy the letter frame).
  nsIFrame* textFrame;
  floater->FirstChild(aPresContext, nsnull, &textFrame);
  if (!textFrame) {
    return NS_OK;
  }

  // Discover the placeholder frame for the letter frame
  nsIFrame* parentFrame;
  nsPlaceholderFrame* placeholderFrame;
  aFrameManager->GetPlaceholderFrameFor(floater, (nsIFrame**)&placeholderFrame);
  if (!placeholderFrame) {
    // Somethings really wrong
    return NS_OK;
  }
  placeholderFrame->GetParent(&parentFrame);
  if (!parentFrame) {
    // Somethings really wrong
    return NS_OK;
  }

  // Create a new text frame with the right style context that maps
  // all of the content that was previously part of the letter frame
  // (and probably continued elsewhere).
  nsCOMPtr<nsIStyleContext> parentSC;
  parentFrame->GetStyleContext(getter_AddRefs(parentSC));
  if (!parentSC) {
    return NS_OK;
  }
  nsCOMPtr<nsIContent> textContent;
  textFrame->GetContent(getter_AddRefs(textContent));
  if (!textContent) {
    return NS_OK;
  }
  nsCOMPtr<nsIStyleContext> newSC;
  aPresContext->ResolveStyleContextForNonElement(parentSC,
                                                 getter_AddRefs(newSC));
  if (!newSC) {
    return NS_OK;
  }
  nsIFrame* newTextFrame;
  nsresult rv = NS_NewTextFrame(aPresShell, &newTextFrame);
  if (NS_FAILED(rv)) {
    return rv;
  }
  newTextFrame->Init(aPresContext, textContent, parentFrame, newSC, nsnull);

  // Destroy the old text frame's continuations (the old text frame
  // will be destroyed when its letter frame is destroyed).
  nsIFrame* nextTextFrame;
  textFrame->GetNextInFlow(&nextTextFrame);
  if (nextTextFrame) {
    nsIFrame* nextTextParent;
    nextTextFrame->GetParent(&nextTextParent);
    if (nextTextParent) {
      nsSplittableFrame::BreakFromPrevFlow(nextTextFrame);
      DeletingFrameSubtree(aPresContext, aPresShell, 
                           aFrameManager, nextTextFrame);
      aFrameManager->RemoveFrame(aPresContext, *aPresShell, nextTextParent,
                                 nsnull, nextTextFrame);
    }
  }

  // First find out where (in the content) the placeholder frames
  // text is and its previous sibling frame, if any.
  nsIFrame* prevSibling = nsnull;

  nsCOMPtr<nsIContent> container;
  parentFrame->GetContent(getter_AddRefs(container));
  if (container.get() && textContent.get()) {
    PRInt32 ix = 0;
    container->IndexOf(textContent, ix);
    prevSibling = FindPreviousSibling(aPresShell, container, ix);
  }

  // Now that everything is set...
#ifdef NOISY_FIRST_LETTER
  printf("RemoveFloatingFirstLetterFrames: textContent=%p oldTextFrame=%p newTextFrame=%p\n",
         textContent.get(), textFrame, newTextFrame);
#endif
  // Should we call DeletingFrameSubtree on the placeholder instead
  // and skip this call?
  aFrameManager->UnregisterPlaceholderFrame(placeholderFrame);

  // Remove the floater frame
  DeletingFrameSubtree(aPresContext, aPresShell, aFrameManager, floater);
  aFrameManager->RemoveFrame(aPresContext, *aPresShell,
                             aBlockFrame, nsLayoutAtoms::floaterList,
                             floater);

  // Remove placeholder frame
  aFrameManager->RemoveFrame(aPresContext, *aPresShell,
                             parentFrame, nsnull, placeholderFrame);

  // Insert text frame in its place
  aFrameManager->InsertFrames(aPresContext, *aPresShell, parentFrame, nsnull,
                              prevSibling, newTextFrame);

  return NS_OK;
}

nsresult
nsCSSFrameConstructor::RemoveFirstLetterFrames(nsIPresContext* aPresContext,
                                               nsIPresShell* aPresShell,
                                               nsIFrameManager* aFrameManager,
                                               nsIFrame* aFrame,
                                               PRBool* aStopLooking)
{
  nsIFrame* kid;
  nsIFrame* prevSibling = nsnull;
  aFrame->FirstChild(aPresContext, nsnull, &kid);

  while (kid) {
    nsCOMPtr<nsIAtom> frameType;
    kid->GetFrameType(getter_AddRefs(frameType));
    if (nsLayoutAtoms::letterFrame == frameType.get()) {
      // Bingo. Found it. First steal away the text frame.
      nsIFrame* textFrame;
      kid->FirstChild(aPresContext, nsnull, &textFrame);
      if (!textFrame) {
        break;
      }

      // Create a new textframe
      nsCOMPtr<nsIStyleContext> parentSC;
      aFrame->GetStyleContext(getter_AddRefs(parentSC));
      if (!parentSC) {
        break;
      }
      nsCOMPtr<nsIContent> textContent;
      textFrame->GetContent(getter_AddRefs(textContent));
      if (!textContent) {
        break;
      }
      nsCOMPtr<nsIStyleContext> newSC;
      aPresContext->ResolveStyleContextForNonElement(parentSC,
                                                     getter_AddRefs(newSC));
      if (!newSC) {
        break;
      }
      NS_NewTextFrame(aPresShell, &textFrame);
      textFrame->Init(aPresContext, textContent, aFrame, newSC, nsnull);

      // Next rip out the kid and replace it with the text frame
      nsIFrameManager* frameManager = aFrameManager;
      DeletingFrameSubtree(aPresContext, aPresShell, frameManager, kid);
      frameManager->RemoveFrame(aPresContext, *aPresShell,
                                aFrame, nsnull, kid);

      // Insert text frame in its place
      frameManager->InsertFrames(aPresContext, *aPresShell, aFrame, nsnull,
                                 prevSibling, textFrame);

      *aStopLooking = PR_TRUE;
      break;
    }
    else if ((nsLayoutAtoms::inlineFrame == frameType.get()) ||
             (nsLayoutAtoms::lineFrame == frameType.get())) {
      // Look inside child inline frame for the letter frame
      RemoveFirstLetterFrames(aPresContext, aPresShell, aFrameManager, kid,
                              aStopLooking);
      if (*aStopLooking) {
        break;
      }
    }
    prevSibling = kid;
    kid->GetNextSibling(&kid);
  }

  return NS_OK;
}

nsresult
nsCSSFrameConstructor::RemoveLetterFrames(nsIPresContext* aPresContext,
                                          nsIPresShell* aPresShell,
                                          nsIFrameManager* aFrameManager,
                                          nsIFrame* aBlockFrame)
{
  PRBool stopLooking = PR_FALSE;
  nsresult rv = RemoveFloatingFirstLetterFrames(aPresContext, aPresShell,
                                                aFrameManager,
                                                aBlockFrame, &stopLooking);
  if (NS_SUCCEEDED(rv) && !stopLooking) {
    rv = RemoveFirstLetterFrames(aPresContext, aPresShell, aFrameManager,
                                 aBlockFrame, &stopLooking);
  }
  return rv;
}

// Fixup the letter frame situation for the given block
nsresult
nsCSSFrameConstructor::RecoverLetterFrames(nsIPresShell* aPresShell, nsIPresContext* aPresContext,
                                           nsFrameConstructorState& aState,
                                           nsIFrame* aBlockFrame)
{
  nsresult rv = NS_OK;

  nsIFrame* blockKids;
  aBlockFrame->FirstChild(aPresContext, nsnull, &blockKids);
  nsIFrame* parentFrame = nsnull;
  nsIFrame* textFrame = nsnull;
  nsIFrame* prevFrame = nsnull;
  nsFrameItems letterFrames;
  PRBool stopLooking = PR_FALSE;
  rv = WrapFramesInFirstLetterFrame(aPresShell, aPresContext, aState,
                                    aBlockFrame, blockKids,
                                    &parentFrame, &textFrame, &prevFrame,
                                    letterFrames, &stopLooking);
  if (NS_FAILED(rv)) {
    return rv;
  }
  if (parentFrame) {
    // Take the old textFrame out of the parents child list
    DeletingFrameSubtree(aPresContext, aState.mPresShell,
                         aState.mFrameManager, textFrame);
    parentFrame->RemoveFrame(aPresContext, *aState.mPresShell.get(),
                             nsnull, textFrame);

    // Insert in the letter frame(s)
    parentFrame->InsertFrames(aPresContext, *aState.mPresShell.get(),
                              nsnull, prevFrame, letterFrames.childList);

    // Insert in floaters too if needed
    if (aState.mFloatedItems.childList) {
      aBlockFrame->AppendFrames(aPresContext, *aState.mPresShell.get(),
                                nsLayoutAtoms::floaterList,
                                aState.mFloatedItems.childList);
    }
  }
  return rv;
}

//----------------------------------------------------------------------

// listbox Widget Routines

NS_IMETHODIMP
nsCSSFrameConstructor::CreateListBoxContent(nsIPresContext* aPresContext,
                                            nsIFrame*       aParentFrame,
                                            nsIFrame*       aPrevFrame,
                                            nsIContent*     aChild,
                                            nsIFrame**      aNewFrame,
                                            PRBool          aIsAppend,
                                            PRBool          aIsScrollbar,
                                            nsILayoutHistoryState* aFrameState)
{
#ifdef INCLUDE_XUL
  nsCOMPtr<nsIPresShell> shell;
  aPresContext->GetShell(getter_AddRefs(shell));
  nsresult rv = NS_OK;

  // Construct a new frame
  if (nsnull != aParentFrame) {
    nsFrameItems            frameItems;
    nsFrameConstructorState state(aPresContext, mFixedContainingBlock,
                                  GetAbsoluteContainingBlock(aPresContext, aParentFrame),
                                  GetFloaterContainingBlock(aPresContext, aParentFrame), 
                                  mTempFrameTreeState);

    nsCOMPtr<nsIStyleContext> styleContext;
    rv = ResolveStyleContext(aPresContext, aParentFrame, aChild,
                             getter_AddRefs(styleContext));

    if (NS_SUCCEEDED(rv)) {
      // Pre-check for display "none" - only if we find that, do we create
      // any frame at all
      const nsStyleDisplay* display = (const nsStyleDisplay*)
        styleContext->GetStyleData(eStyleStruct_Display);

      if (NS_STYLE_DISPLAY_NONE == display->mDisplay) {
        *aNewFrame = nsnull;
        return NS_OK;
      }
    }

    nsCOMPtr<nsIAtom> tag;
    aChild->GetTag(*getter_AddRefs(tag));

    PRInt32 namespaceID;
    aChild->GetNameSpaceID(namespaceID);

    rv = ConstructFrameInternal(shell, aPresContext, state, aChild, aParentFrame, tag, namespaceID, 
                                styleContext, frameItems, PR_FALSE);
    
    nsIFrame* newFrame = frameItems.childList;
    *aNewFrame = newFrame;

    if (NS_SUCCEEDED(rv) && (nsnull != newFrame)) {
      nsCOMPtr<nsIBindingManager> bm;
      mDocument->GetBindingManager(getter_AddRefs(bm));
      bm->ProcessAttachedQueue();

      // Notify the parent frame
      if (aIsAppend)
        rv = ((nsListBoxBodyFrame*)aParentFrame)->ListBoxAppendFrames(newFrame);
      else
        rv = ((nsListBoxBodyFrame*)aParentFrame)->ListBoxInsertFrames(aPrevFrame, newFrame);
      // If there are new absolutely positioned child frames, then notify
      // the parent
      // XXX We can't just assume these frames are being appended, we need to
      // determine where in the list they should be inserted...
      if (state.mAbsoluteItems.childList) {
        rv = state.mAbsoluteItems.containingBlock->AppendFrames(aPresContext, *shell,
                                                         nsLayoutAtoms::absoluteList,
                                                         state.mAbsoluteItems.childList);
      }
      
      // If there are new fixed positioned child frames, then notify
      // the parent
      // XXX We can't just assume these frames are being appended, we need to
      // determine where in the list they should be inserted...
      if (state.mFixedItems.childList) {
        rv = state.mFixedItems.containingBlock->AppendFrames(aPresContext, *shell,
                                                      nsLayoutAtoms::fixedList,
                                                      state.mFixedItems.childList);
      }
      
      // If there are new floating child frames, then notify
      // the parent
      // XXX We can't just assume these frames are being appended, we need to
      // determine where in the list they should be inserted...
      if (state.mFloatedItems.childList) {
        rv = state.mFloatedItems.containingBlock->AppendFrames(aPresContext, *shell,
                                                    nsLayoutAtoms::floaterList,
                                                    state.mFloatedItems.childList);
      }
    }
  }

  return rv;
#else
  return NS_ERROR_FAILURE;
#endif
}

//----------------------------------------

nsresult
nsCSSFrameConstructor::ConstructBlock(nsIPresShell* aPresShell, 
                                      nsIPresContext*          aPresContext,
                                      nsFrameConstructorState& aState,
                                      const nsStyleDisplay*    aDisplay,
                                      nsIContent*              aContent,
                                      nsIFrame*                aParentFrame,
                                      nsIStyleContext*         aStyleContext,
                                      nsIFrame*                aNewFrame)
{
  InitAndRestoreFrame(aPresContext, aState, aContent, 
                      aParentFrame, aStyleContext, nsnull, aNewFrame);

  // See if we need to create a view, e.g. the frame is absolutely positioned
  nsHTMLContainerFrame::CreateViewForFrame(aPresContext, aNewFrame,
                                           aStyleContext, nsnull, PR_FALSE);

  // If we're the first block to be created (e.g., because we're
  // contained inside a XUL document), then make sure that we've got a
  // space manager so we can handle floaters...
  if (! aState.mFloatedItems.containingBlock) {
    nsFrameState state;
    aNewFrame->GetFrameState(&state);
    state |= NS_BLOCK_SPACE_MGR | NS_BLOCK_MARGIN_ROOT;
    aNewFrame->SetFrameState(state);
  }

  // ...and that we're the absolute containing block.
  nsFrameConstructorSaveState absoluteSaveState;
  if (! aState.mAbsoluteItems.containingBlock)
    aState.PushAbsoluteContainingBlock(aNewFrame, absoluteSaveState);

  // See if the block has first-letter style applied to it...
  PRBool haveFirstLetterStyle, haveFirstLineStyle;
  HaveSpecialBlockStyle(aPresContext, aContent, aStyleContext,
                        &haveFirstLetterStyle, &haveFirstLineStyle);

  // Process the child content
  nsFrameItems childItems;
  nsFrameConstructorSaveState floaterSaveState;
  aState.PushFloaterContainingBlock(aNewFrame, floaterSaveState,
                                    haveFirstLetterStyle,
                                    haveFirstLineStyle);
  nsresult rv = ProcessBlockChildren(aPresShell, aPresContext, aState, aContent, aNewFrame,
                                     PR_TRUE, childItems, PR_TRUE);

  nsCOMPtr<nsIAtom> tag;
  aContent->GetTag(*getter_AddRefs(tag));
  CreateAnonymousFrames(aPresShell, aPresContext, tag, aState, aContent, aNewFrame,
                          childItems);

  // Set the frame's initial child list
  aNewFrame->SetInitialChildList(aPresContext, nsnull, childItems.childList);

  // Set the frame's floater list if there were any floated children
  if (aState.mFloatedItems.childList) {
    aNewFrame->SetInitialChildList(aPresContext,
                                   nsLayoutAtoms::floaterList,
                                   aState.mFloatedItems.childList);
  }

  return rv;
}

nsresult
nsCSSFrameConstructor::ProcessBlockChildren(nsIPresShell* aPresShell, 
                                            nsIPresContext*          aPresContext,
                                            nsFrameConstructorState& aState,
                                            nsIContent*              aContent,
                                            nsIFrame*                aFrame,
                                            PRBool                   aCanHaveGeneratedContent,
                                            nsFrameItems&            aFrameItems,
                                            PRBool                   aParentIsBlock)
{
  nsresult rv = NS_OK;
  nsCOMPtr<nsIStyleContext> styleContext;

  if (aCanHaveGeneratedContent) {
    // Probe for generated content before
    nsIFrame* generatedFrame;
    aFrame->GetStyleContext(getter_AddRefs(styleContext));
    if (CreateGeneratedContentFrame(aPresShell, aPresContext, aState, aFrame, aContent,
                                    styleContext, nsCSSAtoms::beforePseudo,
                                    nsnull, &generatedFrame)) {
      // Add the generated frame to the child list
      aFrameItems.AddChild(generatedFrame);
    }
  }

  // Iterate the child content objects and construct frames
  ChildIterator iter, last;
  for (ChildIterator::Init(aContent, &iter, &last);
       iter != last;
       ++iter) {
    // Construct a child frame
    rv = ConstructFrame(aPresShell, aPresContext, aState, nsCOMPtr<nsIContent>(*iter),
                        aFrame, aFrameItems);

    if (NS_FAILED(rv))
      return rv;
  }

  // process pseudo frames if necessary
  if (!aState.mPseudoFrames.IsEmpty()) {
    ProcessPseudoFrames(aPresContext, aState.mPseudoFrames, aFrameItems);
  }

  if (aCanHaveGeneratedContent) {
    // Probe for generated content after
    nsIFrame* generatedFrame;
    if (CreateGeneratedContentFrame(aPresShell, aPresContext, aState, aFrame, aContent,
                                    styleContext, nsCSSAtoms::afterPseudo,
                                    nsnull, &generatedFrame)) {
      // Add the generated frame to the child list
      aFrameItems.AddChild(generatedFrame);
    }
  }

  if (aParentIsBlock) {
    if (aState.mFirstLetterStyle) {
      rv = WrapFramesInFirstLetterFrame(aPresShell, aPresContext, aState, aContent, aFrame, aFrameItems);
    }
    if (aState.mFirstLineStyle) {
      rv = WrapFramesInFirstLineFrame(aPresShell, aPresContext, aState, aContent, aFrame, aFrameItems);
    }
  }

  return rv;
}


PRBool
nsCSSFrameConstructor::AreAllKidsInline(nsIFrame* aFrameList)
{
  nsIFrame* kid = aFrameList;
  while (kid) {
    if (!IsInlineFrame(kid)) {
      return PR_FALSE;
    }
    kid->GetNextSibling(&kid);
  }
  return PR_TRUE;
}

nsresult
nsCSSFrameConstructor::ConstructInline(nsIPresShell*            aPresShell, 
                                       nsIPresContext*          aPresContext,
                                       nsFrameConstructorState& aState,
                                       const nsStyleDisplay*    aDisplay,
                                       nsIContent*              aContent,
                                       nsIFrame*                aParentFrame,
                                       nsIStyleContext*         aStyleContext,
                                       PRBool                   aIsPositioned,
                                       nsIFrame*                aNewFrame,
                                       nsIFrame**               aNewBlockFrame,
                                       nsIFrame**               aNextInlineFrame)
{
  // Initialize the frame
  InitAndRestoreFrame(aPresContext, aState, aContent, 
                      aParentFrame, aStyleContext, nsnull, aNewFrame);

  nsFrameConstructorSaveState absoluteSaveState;  // definition cannot be inside next block
                                                  // because the object's destructor is significant
                                                  // this is part of the fix for bug 42372
  if (aIsPositioned) {                            
    // Relatively positioned frames need a view
    nsHTMLContainerFrame::CreateViewForFrame(aPresContext, aNewFrame,
                                             aStyleContext, nsnull, PR_FALSE);

    // Relatively positioned frames becomes a container for child
    // frames that are positioned
    aState.PushAbsoluteContainingBlock(aNewFrame, absoluteSaveState);
  }

  // Process the child content
  nsFrameItems childItems;
  PRBool kidsAllInline;
  nsresult rv = ProcessInlineChildren(aPresShell, aPresContext, aState, aContent,
                                      aNewFrame, PR_TRUE, childItems, &kidsAllInline);
  if (kidsAllInline) {
    // Set the inline frame's initial child list
    nsCOMPtr<nsIAtom> tag;
    aContent->GetTag(*getter_AddRefs(tag));
    CreateAnonymousFrames(aPresShell, aPresContext, tag, aState, aContent, aNewFrame,
                            childItems);

    aNewFrame->SetInitialChildList(aPresContext, nsnull, childItems.childList);

    if (aIsPositioned) {
      if (aState.mAbsoluteItems.childList) {
        aNewFrame->SetInitialChildList(aPresContext, nsLayoutAtoms::absoluteList,
                                       aState.mAbsoluteItems.childList);
      }
      if (aState.mFloatedItems.childList) {
        aNewFrame->SetInitialChildList(aPresContext,
                                       nsLayoutAtoms::floaterList,
                                       aState.mFloatedItems.childList);
      }
    }

    *aNewBlockFrame = nsnull;
    *aNextInlineFrame = nsnull;
    return rv;
  }

  // This inline frame contains several types of children. Therefore
  // this frame has to be chopped into several pieces. We will produce
  // as a result of this 3 lists of children. The first list contains
  // all of the inline children that preceed the first block child
  // (and may be empty). The second list contains all of the block
  // children and any inlines that are between them (and must not be
  // empty, otherwise - why are we here?). The final list contains all
  // of the inline children that follow the final block child.

  // Find the first block child which defines list1 and list2
  nsIFrame* list1 = childItems.childList;
  nsIFrame* prevToFirstBlock;
  nsIFrame* list2 = FindFirstBlock(aPresContext, list1, &prevToFirstBlock);
  if (prevToFirstBlock) {
    prevToFirstBlock->SetNextSibling(nsnull);
  }
  else {
    list1 = nsnull;
  }

  // Find the last block child which defines the end of list2 and the
  // start of list3
  nsIFrame* afterFirstBlock;
  list2->GetNextSibling(&afterFirstBlock);
  nsIFrame* list3 = nsnull;
  nsIFrame* lastBlock = FindLastBlock(aPresContext, afterFirstBlock);
  if (!lastBlock) {
    lastBlock = list2;
  }
  lastBlock->GetNextSibling(&list3);
  lastBlock->SetNextSibling(nsnull);

  // list1's frames belong to this inline frame so go ahead and take them
  aNewFrame->SetInitialChildList(aPresContext, nsnull, list1);

  if (aIsPositioned) {
    // XXXwaterson just for shits n' giggles, we'll give you the
    // absolute and floated items, too. Is this right?
    if (aState.mAbsoluteItems.childList) {
      aNewFrame->SetInitialChildList(aPresContext, nsLayoutAtoms::absoluteList,
                                     aState.mAbsoluteItems.childList);
    }
    if (aState.mFloatedItems.childList) {
      aNewFrame->SetInitialChildList(aPresContext,
                                     nsLayoutAtoms::floaterList,
                                     aState.mFloatedItems.childList);
    }
  }

  // list2's frames belong to an anonymous block that we create right
  // now. The anonymous block will be the parent of the block children
  // of the inline.
  nsIFrame* blockFrame;
  nsIAtom* blockStyle;
  if (aIsPositioned) {
    NS_NewRelativeItemWrapperFrame(aPresShell, &blockFrame);
    blockStyle = nsHTMLAtoms::mozAnonymousPositionedBlock;
  }
  else {
    NS_NewBlockFrame(aPresShell, &blockFrame);
    blockStyle = nsHTMLAtoms::mozAnonymousBlock;
  }

  nsCOMPtr<nsIStyleContext> blockSC;
  aPresContext->ResolvePseudoStyleContextFor(aContent, blockStyle,
                                             aStyleContext,
                                             getter_AddRefs(blockSC));

  InitAndRestoreFrame(aPresContext, aState, aContent, 
                      aParentFrame, blockSC, nsnull, blockFrame);  

  if (aIsPositioned) {
    // Relatively positioned frames need a view
    nsHTMLContainerFrame::CreateViewForFrame(aPresContext, blockFrame,
                                             aStyleContext, nsnull, PR_FALSE);

    // Move list2's frames into the new view
    nsIFrame* oldParent;
    list2->GetParent(&oldParent);
    nsHTMLContainerFrame::ReparentFrameViewList(aPresContext, list2, oldParent, blockFrame);
  }

  blockFrame->SetInitialChildList(aPresContext, nsnull, list2);

  nsFrameConstructorState state(aPresContext, mFixedContainingBlock,
                                GetAbsoluteContainingBlock(aPresContext, blockFrame),
                                GetFloaterContainingBlock(aPresContext, blockFrame));

  MoveChildrenTo(aPresContext, blockSC, blockFrame, list2, &state);

  // list3's frames belong to another inline frame
  nsIFrame* inlineFrame = nsnull;

  if (list3) {
    if (aIsPositioned) {
      NS_NewPositionedInlineFrame(aPresShell, &inlineFrame);
    }
    else {
      NS_NewInlineFrame(aPresShell, &inlineFrame);
    }

    InitAndRestoreFrame(aPresContext, aState, aContent, 
                        aParentFrame, aStyleContext, nsnull, inlineFrame);

    if (aIsPositioned) {
      // Relatively positioned frames need a view
      nsHTMLContainerFrame::CreateViewForFrame(aPresContext, inlineFrame,
                                               aStyleContext, nsnull, PR_FALSE);

      // Move list3's frames into the new view
      nsIFrame* oldParent;
      list3->GetParent(&oldParent);
      nsHTMLContainerFrame::ReparentFrameViewList(aPresContext, list3, oldParent, inlineFrame);
    }

    // Reparent (cheaply) the frames in list3 - we don't have to futz
    // with their style context because they already have the right one.
    inlineFrame->SetInitialChildList(aPresContext, nsnull, list3);
    MoveChildrenTo(aPresContext, nsnull, inlineFrame, list3, nsnull);
  }

  // Mark the 3 frames as special. That way if any of the
  // append/insert/remove methods try to fiddle with the children, the
  // containing block will be reframed instead.
  SetFrameIsSpecial(aState.mFrameManager, aNewFrame, blockFrame);
  SetFrameIsSpecial(aState.mFrameManager, blockFrame, inlineFrame);
  MarkIBSpecialPrevSibling(aPresContext, aState.mFrameManager,
                           blockFrame, aNewFrame);

  if (inlineFrame)
    SetFrameIsSpecial(aState.mFrameManager, inlineFrame, nsnull);

#ifdef DEBUG
  if (gNoisyInlineConstruction) {
    nsIFrameDebug*  frameDebug;

    printf("nsCSSFrameConstructor::ConstructInline:\n");
    if (NS_SUCCEEDED(aNewFrame->QueryInterface(NS_GET_IID(nsIFrameDebug), (void**)&frameDebug))) {
      printf("  ==> leading inline frame:\n");
      frameDebug->List(aPresContext, stdout, 2);
    }
    if (NS_SUCCEEDED(blockFrame->QueryInterface(NS_GET_IID(nsIFrameDebug), (void**)&frameDebug))) {
      printf("  ==> block frame:\n");
      frameDebug->List(aPresContext, stdout, 2);
    }
    if (inlineFrame && NS_SUCCEEDED(inlineFrame->QueryInterface(NS_GET_IID(nsIFrameDebug), (void**)&frameDebug))) {
      printf("  ==> trailing inline frame:\n");
      frameDebug->List(aPresContext, stdout, 2);
    }
  }
#endif

  *aNewBlockFrame = blockFrame;
  *aNextInlineFrame = inlineFrame;

  return rv;
}

nsresult
nsCSSFrameConstructor::ProcessInlineChildren(nsIPresShell* aPresShell, 
                                             nsIPresContext*          aPresContext,
                                             nsFrameConstructorState& aState,
                                             nsIContent*              aContent,
                                             nsIFrame*                aFrame,
                                             PRBool                   aCanHaveGeneratedContent,
                                             nsFrameItems&            aFrameItems,
                                             PRBool*                  aKidsAllInline)
{
  nsresult rv = NS_OK;
  nsCOMPtr<nsIStyleContext> styleContext;

  // save the pseudo frame state 
  nsPseudoFrames prevPseudoFrames; 
  aState.mPseudoFrames.Reset(&prevPseudoFrames);

  if (aCanHaveGeneratedContent) {
    // Probe for generated content before
    nsIFrame* generatedFrame;
    aFrame->GetStyleContext(getter_AddRefs(styleContext));
    if (CreateGeneratedContentFrame(aPresShell, aPresContext, aState, aFrame, aContent,
                                    styleContext, nsCSSAtoms::beforePseudo,
                                    nsnull, &generatedFrame)) {
      // Add the generated frame to the child list
      aFrameItems.AddChild(generatedFrame);
    }
  }

  // Iterate the child content objects and construct frames
  PRBool allKidsInline = PR_TRUE;
  ChildIterator iter, last;
  for (ChildIterator::Init(aContent, &iter, &last);
       iter != last;
       ++iter) {
    // Construct a child frame
    nsIFrame* oldLastChild = aFrameItems.lastChild;
    rv = ConstructFrame(aPresShell, aPresContext, aState, nsCOMPtr<nsIContent>(*iter),
                        aFrame, aFrameItems);

    if (NS_FAILED(rv)) {
      return rv;
    }

    // Examine newly added children (we may have added more than one
    // child if the child was another inline frame that ends up
    // being carved in 3 pieces) to maintain the allKidsInline flag.
    if (allKidsInline) {
      nsIFrame* kid;
      if (oldLastChild) {
        oldLastChild->GetNextSibling(&kid);
      }
      else {
        kid = aFrameItems.childList;
      }
      while (kid) {
        if (!IsInlineFrame(kid)) {
          allKidsInline = PR_FALSE;
          break;
        }
        kid->GetNextSibling(&kid);
      }
    }
  }

  if (aCanHaveGeneratedContent) {
    // Probe for generated content after
    nsIFrame* generatedFrame;
    if (CreateGeneratedContentFrame(aPresShell, aPresContext, aState, aFrame, aContent,
                                    styleContext, nsCSSAtoms::afterPseudo,
                                    nsnull, &generatedFrame)) {
      // Add the generated frame to the child list
      aFrameItems.AddChild(generatedFrame);
    }
  }

  *aKidsAllInline = allKidsInline;

  // process the current pseudo frame state
  if (!aState.mPseudoFrames.IsEmpty()) {
    ProcessPseudoFrames(aPresContext, aState.mPseudoFrames, aFrameItems);
  }
  // restore the pseudo frame state
  aState.mPseudoFrames = prevPseudoFrames;

  return rv;
}

// Helper function that recursively removes content to frame mappings and
// undisplayed content mappings.
// This differs from DeletingFrameSubtree() because the frames have not yet been
// added to the frame hierarchy
static void
DoCleanupFrameReferences(nsIPresContext*  aPresContext,
                         nsIFrameManager* aFrameManager,
                         nsIFrame*        aFrameIn)
{
  nsCOMPtr<nsIContent> content;
  aFrameIn->GetContent(getter_AddRefs(content));

  nsIFrame* frame = aFrameIn;
  // if the frame is a placeholder use the out of flow frame
  nsCOMPtr<nsIAtom> frameType;
  aFrameIn->GetFrameType(getter_AddRefs(frameType));
  if (nsLayoutAtoms::placeholderFrame == frameType.get()) {
    frame = ((nsPlaceholderFrame*)frame)->GetOutOfFlowFrame();
    NS_ASSERTION(frame, "program error - null of of flow frame in placeholder");
  }

  // Remove the mapping from the content object to its frame
  aFrameManager->SetPrimaryFrameFor(content, nsnull);
  frame->RemovedAsPrimaryFrame(aPresContext);
  aFrameManager->ClearAllUndisplayedContentIn(content);

  // Recursively walk the child frames.
  // Note: we only need to look at the principal child list
  nsIFrame* childFrame;
  frame->FirstChild(aPresContext, nsnull, &childFrame);
  while (childFrame) {
    DoCleanupFrameReferences(aPresContext, aFrameManager, childFrame);
    
    // Get the next sibling child frame
    childFrame->GetNextSibling(&childFrame);
  }
}

// Helper function that walks a frame list and calls DoCleanupFrameReference()
static void
CleanupFrameReferences(nsIPresContext*  aPresContext,
                       nsIFrameManager* aFrameManager,
                       nsIFrame*        aFrameList)
{
  while (aFrameList) {
    DoCleanupFrameReferences(aPresContext, aFrameManager, aFrameList);

    // Get the sibling frame
    aFrameList->GetNextSibling(&aFrameList);
  }
}

PRBool
nsCSSFrameConstructor::WipeContainingBlock(nsIPresContext* aPresContext,
                                           nsFrameConstructorState& aState,
                                           nsIContent* aBlockContent,
                                           nsIFrame* aFrame,
                                           nsIFrame* aFrameList)
{
  // Before we go and append the frames, check for a special
  // situation: an inline frame that will now contain block
  // frames. This is a no-no and the frame construction logic knows
  // how to fix this.
  if (!aBlockContent)
    return PR_FALSE;

  const nsStyleDisplay* parentDisplay;
  aFrame->GetStyleData(eStyleStruct_Display, (const nsStyleStruct *&) parentDisplay);
  if (NS_STYLE_DISPLAY_INLINE == parentDisplay->mDisplay) {
    if (!AreAllKidsInline(aFrameList)) {
      // XXXwaterson temporary code until we figure out why bug 102931
      // is really happening.
      NS_ASSERTION(aBlockContent != nsnull, "ack, inline without a containing block");
      if (! aBlockContent)
        return PR_FALSE;

      // Ok, reverse tracks: wipe out the frames we just created
      nsCOMPtr<nsIPresShell>    presShell;
      nsCOMPtr<nsIFrameManager> frameManager;

      aPresContext->GetShell(getter_AddRefs(presShell));
      presShell->GetFrameManager(getter_AddRefs(frameManager));

      // Destroy the frames. As we do make sure any content to frame mappings
      // or entries in the undisplayed content map are removed
      nsCOMPtr<nsIContent> parentContent;
      aFrame->GetContent(getter_AddRefs(parentContent));
      frameManager->ClearAllUndisplayedContentIn(parentContent);

      CleanupFrameReferences(aPresContext, frameManager, aFrameList);
      nsFrameList tmp(aFrameList);
      tmp.DestroyFrames(aPresContext);
      if (aState.mAbsoluteItems.childList) {
        CleanupFrameReferences(aPresContext, frameManager, aState.mAbsoluteItems.childList);
        tmp.SetFrames(aState.mAbsoluteItems.childList);
        tmp.DestroyFrames(aPresContext);
      }
      if (aState.mFixedItems.childList) {
        CleanupFrameReferences(aPresContext, frameManager, aState.mFixedItems.childList);
        tmp.SetFrames(aState.mFixedItems.childList);
        tmp.DestroyFrames(aPresContext);
      }
      if (aState.mFloatedItems.childList) {
        CleanupFrameReferences(aPresContext, frameManager, aState.mFloatedItems.childList);
        tmp.SetFrames(aState.mFloatedItems.childList);
        tmp.DestroyFrames(aPresContext);
      }
      // Tell parent of the containing block to reformulate the
      // entire block. This is painful and definitely not optimal
      // but it will *always* get the right answer.
      nsCOMPtr<nsIContent> parentContainer;
      aBlockContent->GetParent(*getter_AddRefs(parentContainer));
#ifdef DEBUG
      if (gNoisyContentUpdates) {
        printf("nsCSSFrameConstructor::WipeContainingBlock: aBlockContent=%p parentContainer=%p\n",
               NS_STATIC_CAST(void*, aBlockContent),
               NS_STATIC_CAST(void*, parentContainer));
      }
#endif
      if (parentContainer) {
        PRInt32 ix;
        parentContainer->IndexOf(aBlockContent, ix);
        ContentReplaced(aPresContext, parentContainer, aBlockContent, aBlockContent, ix);
      }
      else {
        // XXX uh oh. the block we need to reframe has no parent!
      }
      return PR_TRUE;
    }
  }
  return PR_FALSE;
}


/*
 * Recursively split an inline frame until we reach a block frame.
 * Below is an example of how SplitToContainingBlock() works.
 *
 * 1. In the initial state, you've got a block frame |B| that contains
 *    a bunch of inline frames eventually winding down to the <object>
 *    frame |o|. (Block frames are indicated with upper case letters,
 *    inline frames are indicated with lower case.)
 *
 *     A-->B-->C
 *         |
 *         i-->j-->k
 *             |
 *             l-->m-->n
 *                 |
 *                 o
 *
 * 2. Now the object frame |o| gets split into the left inlines |o|,
 *    the block frames |O|, and the right inlines |o'|.
 *
 *             o O o'
 *
 * 3. We call SplitToContainingBlock(), which will split |m| as follows. 
 *
 *             .--------.
 *            /          \
 *       l-->m==>M==>m'   n
 *           |   |   |
 *           o   O   o'
 *
 *    Note that |m| gets split into |M| and |m'|, which correspond to
 *    the anonymous block and inline frames,
 *    respectively. Furthermore, note that |m| still refers to |n| as
 *    its next sibling, and that |m'| does not yet have a next sibling.
 *
 *    The double-arrow line indicates that an annotation is made in
 *    the frame manager that indicates |M| is the ``special sibling''
 *    of |m|, and that |m'| is the ``special sibling'' of |M|.
 *
 * 4. We recurse again to split |j|. At this point, we'll break the
 *    link between |m| and |n|, and make |n| be the next sibling of
 *    |m'|.
 *
 *             .-----------.
 *            /             \
 *       i-->j=====>J==>j'   k
 *           |      |   |
 *           l-->m=>M==>m'-->n
 *               |  |   |
 *               o  O   o'
 *
 *     As before, |j| retains |k| as its next sibling, and |j'| is not
 *     yet assigned its next sibling.
 *
 * 5. When we hit B, the recursion terminates. We now insert |J| and
 *    |j'| immediately after |j|, resulting in the following frame
 *    model. This is done using the "normal" frame insertion
 *    mechanism, nsIFrame::InsertFrames(), which properly recomputes
 *    the line boxes.
 *
 *     A-->B-->C
 *         |
 *         i-->j-=-=-=>J-=->j'-->k
 *             |       |    |
 *             l-->m==>M===>m'-->n
 *                 |   |    |
 *                 o   O    o'
 *
 *    Since B is a block, it is allowed to contain both block and
 *    inline frames, so we can let |J| and |j'| be "real siblings" of
 *    |j|.
 *
 *    Note that |J| is both the ``normal'' sibling and ``special''
 *    sibling of |j|, and |j'| is both the ``normal'' and ``special''
 *    sibling of |J|.
 */
nsresult
nsCSSFrameConstructor::SplitToContainingBlock(nsIPresContext* aPresContext,
                                              nsFrameConstructorState& aState,
                                              nsIFrame* aFrame,
                                              nsIFrame* aLeftInlineChildFrame,
                                              nsIFrame* aBlockChildFrame,
                                              nsIFrame* aRightInlineChildFrame,
                                              PRBool aTransfer)
{
  // If aFrame is an inline frame, then recursively "split" it until
  // we reach a block frame. aLeftInlineChildFrame is the original
  // inline child of aFrame (or null, if there were no frames to the
  // left of the new block); aBlockChildFrame and
  // aRightInlineChildFrame are the newly created frames that were
  // constructed as a result of the previous recursion's
  // "split". aRightInlineChildFrame may be null if there are no
  // inlines to the right of the new block.
  //
  // aBlockChildFrame and aRightInlineChildFrame will be "orphaned" frames upon
  // entry to this routine; that is, they won't be parented. We'll
  // assign them proper parents.
  NS_PRECONDITION(aFrame != nsnull, "no frame to split");
  if (! aFrame)
    return NS_ERROR_NULL_POINTER;

  NS_PRECONDITION(aBlockChildFrame != nsnull, "no block child");
  if (! aBlockChildFrame)
    return NS_ERROR_NULL_POINTER;

  nsCOMPtr<nsIPresShell> shell;
  aPresContext->GetShell(getter_AddRefs(shell));

  if (IsBlockFrame(aPresContext, aFrame)) {
    // If aFrame is a block frame, then we're done: make
    // aBlockChildFrame and aRightInlineChildFrame children of aFrame,
    // and insert aBlockChildFrame and aRightInlineChildFrame after
    // aLeftInlineChildFrame
    aBlockChildFrame->SetParent(aFrame);

    if (aRightInlineChildFrame)
      aRightInlineChildFrame->SetParent(aFrame);

    aBlockChildFrame->SetNextSibling(aRightInlineChildFrame);
    aFrame->InsertFrames(aPresContext, *shell, nsnull, aLeftInlineChildFrame, aBlockChildFrame);

    // If aLeftInlineChild has a view...
    if (aLeftInlineChildFrame) {
      nsFrameState state;
      aLeftInlineChildFrame->GetFrameState(&state);

      if (state & NS_FRAME_HAS_VIEW) {
        // ...create a new view for the block child, and reparent views
        nsCOMPtr<nsIStyleContext> sc;
        aLeftInlineChildFrame->GetStyleContext(getter_AddRefs(sc));

        nsHTMLContainerFrame::CreateViewForFrame(aPresContext, aBlockChildFrame,
                                                 sc, nsnull, PR_FALSE);

        nsIFrame* frame;
        aBlockChildFrame->FirstChild(aPresContext, nsnull, &frame);
        nsHTMLContainerFrame::ReparentFrameViewList(aPresContext, frame, aLeftInlineChildFrame, aBlockChildFrame);

        if (aRightInlineChildFrame) {
          // Same for the right inline children
          nsHTMLContainerFrame::CreateViewForFrame(aPresContext, aRightInlineChildFrame,
                                                   sc, nsnull, PR_FALSE);

          aRightInlineChildFrame->FirstChild(aPresContext, nsnull, &frame);
          nsHTMLContainerFrame::ReparentFrameViewList(aPresContext, frame, aLeftInlineChildFrame, aRightInlineChildFrame);
        }
      }
    }

    return NS_OK;
  }

  // Otherwise, aFrame is inline. Split it, and recurse to find the
  // containing block frame.
  nsCOMPtr<nsIContent> content;
  aFrame->GetContent(getter_AddRefs(content));

  // Create an "anonymous block" frame that will parent
  // aBlockChildFrame. The new frame won't have a parent yet: the recursion
  // will parent it.
  nsIFrame* blockFrame;
  NS_NewBlockFrame(shell, &blockFrame);
  if (! blockFrame)
    return NS_ERROR_OUT_OF_MEMORY;

  nsCOMPtr<nsIStyleContext> styleContext;
  aFrame->GetStyleContext(getter_AddRefs(styleContext));

  nsCOMPtr<nsIStyleContext> blockSC;
  aPresContext->ResolvePseudoStyleContextFor(content,
                                             nsHTMLAtoms::mozAnonymousBlock,
                                             styleContext,
                                             getter_AddRefs(blockSC));

  InitAndRestoreFrame(aPresContext, aState, content,
                      nsnull, blockSC, nsnull, blockFrame);

  blockFrame->SetInitialChildList(aPresContext, nsnull, aBlockChildFrame);
  MoveChildrenTo(aPresContext, blockSC, blockFrame, aBlockChildFrame, nsnull);

  // Create an anonymous inline frame that will parent
  // aRightInlineChildFrame. The new frame won't have a parent yet:
  // the recursion will parent it.
  // XXXldb Why bother if |aRightInlineChildFrame| is null?
  nsIFrame* inlineFrame = nsnull;
  NS_NewInlineFrame(shell, &inlineFrame);
  if (! inlineFrame)
    return NS_ERROR_OUT_OF_MEMORY;

  InitAndRestoreFrame(aPresContext, aState, content,
                      nsnull, styleContext, nsnull, inlineFrame);

  inlineFrame->SetInitialChildList(aPresContext, nsnull, aRightInlineChildFrame);
  MoveChildrenTo(aPresContext, nsnull, inlineFrame, aRightInlineChildFrame, nsnull);

  // Make the "special" inline-block linkage between aFrame and the
  // newly created anonymous frames. We need to create the linkage
  // between the first in flow, so if we're a continuation frame, walk
  // back to find it.
  nsIFrame* firstInFlow = aFrame;
  while (1) {
    nsIFrame* prevInFlow;
    firstInFlow->GetPrevInFlow(&prevInFlow);
    if (! prevInFlow) break;
    firstInFlow = prevInFlow;
  }

  SetFrameIsSpecial(aState.mFrameManager, firstInFlow, blockFrame);
  SetFrameIsSpecial(aState.mFrameManager, blockFrame, inlineFrame);
  SetFrameIsSpecial(aState.mFrameManager, inlineFrame, nsnull);

  MarkIBSpecialPrevSibling(aPresContext, aState.mFrameManager,
                           blockFrame, firstInFlow);

  // If we have a continuation frame, then we need to break the
  // continuation.
  nsIFrame* nextInFlow;
  aFrame->GetNextInFlow(&nextInFlow);
  if (nextInFlow) {
    aFrame->SetNextInFlow(nsnull);
    nextInFlow->SetPrevInFlow(nsnull);
  }

  // This is where the mothership lands and we start to get a bit
  // funky. We're going to do a bit of work to ensure that the frames
  // from the *last* recursion are properly hooked up.
  //
  // aTransfer will be set once the recursion begins to nest. (It's
  // not set at the first level of recursion, because
  // aLeftInlineChildFrame, aBlockChildFrame, and
  // aRightInlineChildFrame already have their sibling and parent
  // pointers properly initialized.)
  //
  // Once we begin to nest recursion, aLeftInlineChildFrame
  // corresponds to the original inline that we're trying to split,
  // and aBlockChildFrame and aRightInlineChildFrame are the anonymous
  // frames we created to protect the inline-block invariant.
  if (aTransfer) {
    // We'd better have the left- and right-inline children!
    NS_ASSERTION(aLeftInlineChildFrame != nsnull, "no left inline child frame");
    NS_ASSERTION(aRightInlineChildFrame != nsnull, "no right inline child frame");

    // We need to move any successors of the original inline
    // (aLeftInlineChildFrame) to aRightInlineChildFrame.
    nsIFrame* nextInlineFrame = nsnull;
    aLeftInlineChildFrame->GetNextSibling(&nextInlineFrame);
    aLeftInlineChildFrame->SetNextSibling(nsnull);
    aRightInlineChildFrame->SetNextSibling(nextInlineFrame);

    // Any frame that was moved will need its parent pointer fixed,
    // and will need to be marked as dirty.
    while (nextInlineFrame) {
      nextInlineFrame->SetParent(inlineFrame);

      nsFrameState state;
      nextInlineFrame->GetFrameState(&state);
      state |= NS_FRAME_IS_DIRTY;
      nextInlineFrame->SetFrameState(state);

      nextInlineFrame->GetNextSibling(&nextInlineFrame);
    }
  }

  // Recurse to the parent frame. This will assign a parent frame to
  // each new frame we've just created.
  nsIFrame* parent;
  aFrame->GetParent(&parent);

  NS_ASSERTION(parent != nsnull, "frame has no geometric parent");
  if (! parent)
    return NS_ERROR_FAILURE;

  // When we recur, we'll make the "left inline child frame" be the
  // inline frame we've just begun to "split", and we'll pass the
  // newly created anonymous frames as aBlockChildFrame and
  // aRightInlineChildFrame.
  return SplitToContainingBlock(aPresContext, aState, parent, aFrame, blockFrame, inlineFrame, PR_TRUE);
}

nsresult
nsCSSFrameConstructor::ReframeContainingBlock(nsIPresContext* aPresContext, nsIFrame* aFrame)
{
#ifdef DEBUG
  PRBool isAttinasi = PR_FALSE;
#ifdef DEBUG_attinasi
  isAttinasi = PR_TRUE;
#endif // DEBUG_attinasi

  // ReframeContainingBlock is a NASTY routine, it causes terrible performance problems
  // so I want to see when it is happening!  Unfortunately, it is happening way to often because
  // so much content on the web causes 'special' block-in-inline frame situations and we handle them
  // very poorly
  if (gNoisyContentUpdates || isAttinasi) {
    printf("nsCSSFrameConstructor::ReframeContainingBlock frame=%p\n",
           NS_STATIC_CAST(void*, aFrame));
  }
#endif

  nsCOMPtr<nsIPresShell> shell;
  aPresContext->GetShell(getter_AddRefs(shell));
  PRBool isReflowing;
  shell->IsReflowLocked(&isReflowing);
  if(isReflowing) {
    // don't ReframeContainingBlock, this will result in a crash
    // if we remove a tree that's in reflow - see bug 121368 for testcase
    NS_ASSERTION(0, "Atemptted to nsCSSFrameConstructor::ReframeContainingBlock during a Reflow!!!");
    return NS_OK;
  }

  // Get the first "normal" ancestor of the target frame.
  nsIFrame* containingBlock = GetIBContainingBlockFor(aFrame);
  if (containingBlock) {
    // From here we look for the containing block in case the target
    // frame is already a block (which can happen when an inline frame
    // wraps some of its content in an anonymous block; see
    // ConstructInline)
   
    // NOTE: We used to get the FloaterContainingBlock here, but it was often wrong.
    // GetIBContainingBlock works much better and provides the correct container in all cases
    // so GetFloaterContainingBlock(aPresContext, aFrame) has been removed

    // And get the containingBlock's content
    nsCOMPtr<nsIContent> blockContent;
    containingBlock->GetContent(getter_AddRefs(blockContent));
    if (blockContent) {
      // Now find the containingBlock's content's parent
      nsCOMPtr<nsIContent> parentContainer;
      blockContent->GetParent(*getter_AddRefs(parentContainer));
      if (parentContainer) {
#ifdef DEBUG
        if (gNoisyContentUpdates) {
          printf("  ==> blockContent=%p, parentContainer=%p\n",
                 NS_STATIC_CAST(void*, blockContent),
                 NS_STATIC_CAST(void*, parentContainer));
        }
#endif
        PRInt32 ix;
        parentContainer->IndexOf(blockContent, ix);
        return ContentReplaced(aPresContext, parentContainer, blockContent, blockContent, ix);
      }
    }
  }

  // If we get here, we're screwed!
  return RecreateEntireFrameTree(aPresContext);
}

nsresult
nsCSSFrameConstructor::RecreateEntireFrameTree(nsIPresContext* aPresContext)
{
  // XXX write me some day
  return NS_OK;
}

nsresult nsCSSFrameConstructor::RemoveFixedItems(nsIPresContext*  aPresContext,
                                                 nsIPresShell*    aPresShell,
                                                 nsIFrameManager* aFrameManager)
{
  nsresult rv=NS_OK;

  if (mFixedContainingBlock) {
    nsIFrame *fixedChild = nsnull;
    do {
      mFixedContainingBlock->FirstChild(aPresContext,
                                        nsLayoutAtoms::fixedList,
                                        &fixedChild);
      if (fixedChild) {
        DeletingFrameSubtree(aPresContext, aPresShell, aFrameManager,
                             fixedChild);
        rv = aFrameManager->RemoveFrame(aPresContext, *aPresShell,
                                        mFixedContainingBlock,
                                        nsLayoutAtoms::fixedList,
                                        fixedChild);
        if (NS_FAILED(rv)) {
          NS_WARNING("Error removing frame from fixed containing block in RemoveFixedItems");
          break;
        }
      }
    } while(fixedChild);
  } else {
    NS_WARNING( "RemoveFixedItems called with no FixedContainingBlock data member set");
  }
  return rv;
}

