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

#include "nsCSSFrameConstructor.h"
#include "nsIArena.h"
#include "nsCRT.h"
#include "nsIAtom.h"
#include "nsIURL.h"
#include "nsISupportsArray.h"
#include "nsHashtable.h"
#include "nsIHTMLContent.h"
#include "nsIHTMLAttributes.h"
#include "nsIStyleRule.h"
#include "nsIFrame.h"
#include "nsIStyleContext.h"
#include "nsHTMLAtoms.h"
#include "nsIPresContext.h"
#include "nsILinkHandler.h"
#include "nsIDocument.h"
#include "nsIHTMLTableCellElement.h"
#include "nsTableColFrame.h"
#include "nsHTMLIIDs.h"
#include "nsIStyleFrameConstruction.h"
#include "nsHTMLParts.h"
#include "nsIPresShell.h"
#include "nsIStyleSet.h"
#include "nsIViewManager.h"
#include "nsStyleConsts.h"
#include "nsTableOuterFrame.h"
#include "nsIXMLDocument.h"
#include "nsIWebShell.h"
#include "nsHTMLContainerFrame.h"
#include "nsINameSpaceManager.h"
#include "nsLayoutAtoms.h"
#include "nsIDOMHTMLSelectElement.h"
#include "nsIComboboxControlFrame.h"
#include "nsIListControlFrame.h"
#include "nsIRadioControlFrame.h"
#include "nsIListControlFrame.h"
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

#include "nsInlineFrame.h"
#undef NOISY_FIRST_LETTER

#ifdef INCLUDE_MATHML
#include "nsMathMLAtoms.h"
#include "nsMathMLParts.h"
#endif

#ifdef INCLUDE_XUL
#include "nsXULAtoms.h"
#include "nsTreeFrame.h"
#include "nsTreeOuterFrame.h"
#include "nsTreeRowGroupFrame.h"
#include "nsTreeRowFrame.h"
#include "nsToolboxFrame.h"
#include "nsToolbarFrame.h"
#include "nsTreeIndentationFrame.h"
#include "nsTreeCellFrame.h"
#include "nsIDOMElement.h"
#include "nsIDOMDocument.h"
#include "nsDocument.h"
#include "nsToolbarItemFrame.h"

#define IS_GFX

nsresult
NS_NewThumbFrame ( nsIFrame** aNewFrame );

nsresult
NS_NewScrollPortFrame ( nsIFrame** aNewFrame );

nsresult
NS_NewGfxScrollFrame ( nsIFrame** aNewFrame );

nsresult
NS_NewTabFrame ( nsIFrame** aNewFrame );

nsresult
NS_NewDeckFrame ( nsIFrame** aNewFrame );

nsresult
NS_NewProgressMeterFrame ( nsIFrame** aNewFrame );

nsresult
NS_NewTitledButtonFrame ( nsIFrame** aNewFrame );

nsresult
NS_NewBoxFrame ( nsIFrame** aNewFrame, PRUint32 aFlags );

nsresult
NS_NewSliderFrame ( nsIFrame** aNewFrame );

nsresult
NS_NewScrollbarFrame ( nsIFrame** aNewFrame );

nsresult
NS_NewSpinnerFrame ( nsIFrame** aNewFrame );

nsresult
NS_NewColorPickerFrame ( nsIFrame** aNewFrame );

nsresult
NS_NewFontPickerFrame ( nsIFrame** aNewFrame );

nsresult
NS_NewScrollbarButtonFrame ( nsIFrame** aNewFrame );

nsresult
NS_NewScrollbarFrame ( nsIFrame** aNewFrame );

nsresult
NS_NewGrippyFrame ( nsIFrame** aNewFrame );

nsresult
NS_NewSplitterFrame ( nsIFrame** aNewFrame );

nsresult
NS_NewMenuPopupFrame ( nsIFrame** aNewFrame );

nsresult
NS_NewPopupSetFrame(nsIFrame** aNewFrame);

nsresult
NS_NewMenuFrame ( nsIFrame** aNewFrame, PRUint32 aFlags );

nsresult
NS_NewMenuBarFrame ( nsIFrame** aNewFrame );

#endif

//static NS_DEFINE_IID(kIStyleRuleIID, NS_ISTYLE_RULE_IID);
static NS_DEFINE_IID(kIStyleFrameConstructionIID, NS_ISTYLE_FRAME_CONSTRUCTION_IID);
static NS_DEFINE_IID(kIHTMLTableCellElementIID, NS_IHTMLTABLECELLELEMENT_IID);
static NS_DEFINE_IID(kIXMLDocumentIID, NS_IXMLDOCUMENT_IID);
static NS_DEFINE_IID(kIWebShellIID, NS_IWEB_SHELL_IID);

static NS_DEFINE_IID(kIDOMHTMLSelectElementIID, NS_IDOMHTMLSELECTELEMENT_IID);
static NS_DEFINE_IID(kIComboboxControlFrameIID, NS_ICOMBOBOXCONTROLFRAME_IID);
static NS_DEFINE_IID(kIRadioControlFrameIID,    NS_IRADIOCONTROLFRAME_IID);
static NS_DEFINE_IID(kIListControlFrameIID,     NS_ILISTCONTROLFRAME_IID);
static NS_DEFINE_IID(kIDOMHTMLImageElementIID, NS_IDOMHTMLIMAGEELEMENT_IID);
static NS_DEFINE_IID(kIDOMCharacterDataIID, NS_IDOMCHARACTERDATA_IID);
static NS_DEFINE_IID(kScrollViewIID, NS_ISCROLLABLEVIEW_IID);
static NS_DEFINE_IID(kIFormControlIID, NS_IFORMCONTROL_IID);
static NS_DEFINE_IID(kIFrameIID, NS_IFRAME_IID);

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

  // Constructor
  nsFrameConstructorState(nsIPresContext* aPresContext,
                          nsIFrame*       aFixedContainingBlock,
                          nsIFrame*       aAbsoluteContainingBlock,
                          nsIFrame*       aFloaterContainingBlock);

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

nsFrameConstructorState::nsFrameConstructorState(nsIPresContext* aPresContext,
                                                 nsIFrame*       aFixedContainingBlock,
                                                 nsIFrame*       aAbsoluteContainingBlock,
                                                 nsIFrame*       aFloaterContainingBlock)
  : mFixedItems(aFixedContainingBlock),
    mAbsoluteItems(aAbsoluteContainingBlock),
    mFloatedItems(aFloaterContainingBlock),
    mFirstLetterStyle(PR_FALSE),
    mFirstLineStyle(PR_FALSE)
{
  aPresContext->GetShell(getter_AddRefs(mPresShell));
  mPresShell->GetFrameManager(getter_AddRefs(mFrameManager));
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

// -----------------------------------------------------------

// Structure used when creating table frames.
struct nsTableCreator {
  virtual nsresult CreateTableOuterFrame(nsIFrame** aNewFrame);
  virtual nsresult CreateTableFrame(nsIFrame** aNewFrame);
  virtual nsresult CreateTableRowGroupFrame(nsIFrame** aNewFrame);
  virtual nsresult CreateTableColFrame(nsIFrame** aNewFrame);
  virtual nsresult CreateTableColGroupFrame(nsIFrame** aNewFrame);
  virtual nsresult CreateTableRowFrame(nsIFrame** aNewFrame);
  virtual nsresult CreateTableCellFrame(nsIFrame** aNewFrame);
//MathML Mod - RBS
#ifdef INCLUDE_MATHML
  virtual nsresult CreateTableCellInnerFrame(nsIFrame** aNewFrame);
#endif
  virtual PRBool IsTreeCreator() { return PR_FALSE; };
};

nsresult
nsTableCreator::CreateTableOuterFrame(nsIFrame** aNewFrame) {
  return NS_NewTableOuterFrame(aNewFrame);
}

nsresult
nsTableCreator::CreateTableFrame(nsIFrame** aNewFrame) {
  return NS_NewTableFrame(aNewFrame);
}

nsresult
nsTableCreator::CreateTableRowGroupFrame(nsIFrame** aNewFrame) {
  return NS_NewTableRowGroupFrame(aNewFrame);
}

nsresult
nsTableCreator::CreateTableColFrame(nsIFrame** aNewFrame) {
  return NS_NewTableColFrame(aNewFrame);
}

nsresult
nsTableCreator::CreateTableColGroupFrame(nsIFrame** aNewFrame) {
  return NS_NewTableColGroupFrame(aNewFrame);
}

nsresult
nsTableCreator::CreateTableRowFrame(nsIFrame** aNewFrame) {
  return NS_NewTableRowFrame(aNewFrame);
}

nsresult
nsTableCreator::CreateTableCellFrame(nsIFrame** aNewFrame) {
  return NS_NewTableCellFrame(aNewFrame);
}

#ifdef INCLUDE_XUL

// Structure used when creating tree frames
struct nsTreeCreator: public nsTableCreator {
  nsresult CreateTableOuterFrame(nsIFrame** aNewFrame);
  nsresult CreateTableFrame(nsIFrame** aNewFrame);
  nsresult CreateTableCellFrame(nsIFrame** aNewFrame);
  nsresult CreateTableRowGroupFrame(nsIFrame** aNewFrame);
  nsresult CreateTableRowFrame(nsIFrame** aNewFrame);

  PRBool IsTreeCreator() { return PR_TRUE; };
};

nsresult
nsTreeCreator::CreateTableOuterFrame(nsIFrame** aNewFrame)
{
  return NS_NewTreeOuterFrame(aNewFrame);
}

nsresult
nsTreeCreator::CreateTableFrame(nsIFrame** aNewFrame)
{
  return NS_NewTreeFrame(aNewFrame);
}

nsresult
nsTreeCreator::CreateTableCellFrame(nsIFrame** aNewFrame)
{
  return NS_NewTreeCellFrame(aNewFrame);
}

nsresult
nsTreeCreator::CreateTableRowGroupFrame(nsIFrame** aNewFrame)
{
  return NS_NewTreeRowGroupFrame(aNewFrame);
}

nsresult
nsTreeCreator::CreateTableRowFrame(nsIFrame** aNewFrame) {
  return NS_NewTreeRowFrame(aNewFrame);
}

#endif // INCLUDE_XUL

//MathML Mod - RBS
#ifdef INCLUDE_MATHML
nsresult
nsTableCreator::CreateTableCellInnerFrame(nsIFrame** aNewFrame) {
  return NS_NewTableCellInnerFrame(aNewFrame);
}

// Structure used when creating MathML mtable frames
struct nsMathMLmtableCreator: public nsTableCreator {
  nsresult CreateTableCellInnerFrame(nsIFrame** aNewFrame);
};

nsresult
nsMathMLmtableCreator::CreateTableCellInnerFrame(nsIFrame** aNewFrame)
{
  // only works if aNewFrame is an AreaFrame (to take care of the lineLayout logic)
  return NS_NewMathMLmtdFrame(aNewFrame);
}
#endif // INCLUDE_MATHML

// -----------------------------------------------------------

nsCSSFrameConstructor::nsCSSFrameConstructor(void)
  : nsIStyleFrameConstruction(),
    mDocument(nsnull),
    mInitialContainingBlock(nsnull),
    mFixedContainingBlock(nsnull),
    mDocElementContainingBlock(nsnull),
    mGfxScrollFrame(nsnull)
{
  NS_INIT_REFCNT();
}

nsCSSFrameConstructor::~nsCSSFrameConstructor(void)
{
}

NS_IMPL_ISUPPORTS(nsCSSFrameConstructor, kIStyleFrameConstructionIID);


NS_IMETHODIMP 
nsCSSFrameConstructor::Init(nsIDocument* aDocument)
{
  NS_PRECONDITION(aDocument, "null ptr");
  if (! aDocument)
    return NS_ERROR_NULL_POINTER;

  if (mDocument)
    return NS_ERROR_ALREADY_INITIALIZED;

  mDocument = aDocument; // not refcounted!
  return NS_OK;
}

// Helper function that determines the child list name that aChildFrame
// is contained in
static void
GetChildListNameFor(nsIFrame* aParentFrame,
                    nsIFrame* aChildFrame,
                    nsIAtom** aListName)
{
  nsFrameState  frameState;
  nsIAtom*      listName;
  
  // See if the frame is moved out of the flow
  aChildFrame->GetFrameState(&frameState);
  if (frameState & NS_FRAME_OUT_OF_FLOW) {
    // Look at the style information to tell
    const nsStylePosition* position;
    aChildFrame->GetStyleData(eStyleStruct_Position, (const nsStyleStruct*&)position);
    
    if (NS_STYLE_POSITION_ABSOLUTE == position->mPosition) {
      listName = nsLayoutAtoms::absoluteList;
    } else if (NS_STYLE_POSITION_FIXED == position->mPosition) {
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
  aParentFrame->FirstChild(listName, &firstChild);

  nsFrameList frameList(firstChild);
  NS_ASSERTION(frameList.ContainsFrame(aChildFrame), "not in child list");
#endif

  NS_IF_ADDREF(listName);
  *aListName = listName;
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

  if (eStyleContentType_URL == type) {
    nsIHTMLContent* imageContent;
    
    // Create an HTML image content object, and set the SRC.
    // XXX Check if it's an image type we can handle...
    NS_NewHTMLImageElement(&imageContent, nsHTMLAtoms::img);
    imageContent->SetHTMLAttribute(nsHTMLAtoms::src, contentString, PR_FALSE);

    // Set aContent as the parent content and set the document object. This
    // way event handling works
    imageContent->SetParent(aContent);
    imageContent->SetDocument(aDocument, PR_TRUE);
  
    // Create an image frame and initialize it
    nsIFrame* imageFrame;
    NS_NewImageFrame(&imageFrame);
    imageFrame->Init(*aPresContext, imageContent, aParentFrame, aStyleContext, nsnull);
    NS_RELEASE(imageContent);
  
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
          nsIContent* content = nsnull;
          nsIFrame*   textFrame = nsnull;
          NS_NewAttributeContent(&content);
          if (nsnull != content) {
            nsCOMPtr<nsIAttributeContent> attrContent(do_QueryInterface(content));
            if (attrContent) {
              attrContent->Init(aContent, attrNameSpace, attrName);  
            }

            // Set aContent as the parent content and set the document object. This
            // way event handling works
            content->SetParent(aContent);
            content->SetDocument(aDocument, PR_TRUE);

            // Create a text frame and initialize it
            NS_NewTextFrame(&textFrame);
            textFrame->Init(*aPresContext, content, aParentFrame, aStyleContext, nsnull);

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
        PRUint32  quotesCount = aStyleContent->QuotesCount();
        if (quotesCount > 0) {
          nsAutoString  openQuote, closeQuote;
  
          // If the depth is greater than the number of pairs, the last pair
          // is repeated
          PRUint32  quoteDepth = 0;  // XXX really track the nested quotes...
          if (quoteDepth > quotesCount) {
            quoteDepth = quotesCount - 1;
          }
          aStyleContent->GetQuotesAt(quoteDepth, openQuote, closeQuote);
          if (eStyleContentType_OpenQuote == type) {
            contentString = openQuote;
          } else {
            contentString = closeQuote;
          }
  
        } else {
          contentString = '\"';
        }
      }
      break;
  
    case eStyleContentType_NoOpenQuote:
    case eStyleContentType_NoCloseQuote:
      // XXX Adjust quote depth...
      return NS_OK;
    } // switch
  

    // Create a text content node
    nsIContent* textContent = nsnull;
    nsIDOMCharacterData*  domData;
    nsIFrame*             textFrame = nsnull;
    
    NS_NewTextNode(&textContent);
    if (textContent) {
      // Set aContent as the parent content and set the document object. This
      // way event handling works
      textContent->SetParent(aContent);
      textContent->SetDocument(aDocument, PR_TRUE);

      // Set the text
      textContent->QueryInterface(kIDOMCharacterDataIID, (void**)&domData);
      domData->SetData(contentString);
      NS_RELEASE(domData);
  
      // Create a text frame and initialize it
      NS_NewTextFrame(&textFrame);
      textFrame->Init(*aPresContext, textContent, aParentFrame, aStyleContext, nsnull);
  
      NS_RELEASE(textContent);
    }
  
    // Return the text frame
    *aFrame = textFrame;
  }

  return NS_OK;
}

PRBool
nsCSSFrameConstructor::CreateGeneratedContentFrame(nsIPresContext*  aPresContext,
                                                   nsFrameConstructorState& aState,
                                                   nsIFrame*        aFrame,
                                                   nsIContent*      aContent,
                                                   nsIStyleContext* aStyleContext,
                                                   nsIAtom*         aPseudoElement,
                                                   PRBool           aMakeFirstLetterFrame,
                                                   PRBool           aForBlock,
                                                   nsIFrame**       aResult)
{
  *aResult = nsnull; // initialize OUT parameter

  // Probe for the existence of the pseudo-element
  nsIStyleContext*  pseudoStyleContext;
  aPresContext->ProbePseudoStyleContextFor(aContent, aPseudoElement, aStyleContext,
                                           PR_FALSE, &pseudoStyleContext);

  if (pseudoStyleContext) {
    const nsStyleDisplay* display;

    // See whether the generated content should be displayed.
    display = (const nsStyleDisplay*)pseudoStyleContext->GetStyleData(eStyleStruct_Display);

    if (NS_STYLE_DISPLAY_NONE != display->mDisplay) {
      // See if there was any content specified
      const nsStyleContent* styleContent =
        (const nsStyleContent*)pseudoStyleContext->GetStyleData(eStyleStruct_Content);
      PRUint32  contentCount = styleContent->ContentCount();

      if (contentCount > 0) {
        PRUint8 displayValue = display->mDisplay;
  
        // Make sure the 'display' property value is allowable
        const nsStyleDisplay* subjectDisplay = (const nsStyleDisplay*)
          aStyleContext->GetStyleData(eStyleStruct_Display);
  
        if (subjectDisplay->IsBlockLevel()) {
          // For block-level elements the only allowed 'display' values are:
          // 'none', 'inline', 'block', and 'marker'
          if ((NS_STYLE_DISPLAY_INLINE != displayValue) &&
              (NS_STYLE_DISPLAY_BLOCK != displayValue) &&
              (NS_STYLE_DISPLAY_MARKER != displayValue)) {
            // Pseudo-element behaves as if the value were 'block'
            displayValue = NS_STYLE_DISPLAY_BLOCK;
          }
  
        } else {
          // For inline-level elements the only allowed 'display' values are
          // 'none' and 'inline'
          displayValue = NS_STYLE_DISPLAY_INLINE;
        }
  
        if (display->mDisplay != displayValue) {
          // Reset the value
          nsStyleDisplay* mutableDisplay = (nsStyleDisplay*)
            pseudoStyleContext->GetMutableStyleData(eStyleStruct_Display);
  
          mutableDisplay->mDisplay = displayValue;
        }

        // Also make sure the 'position' property is 'static'. :before and :after
        // pseudo-elements can not be floated or positioned
        const nsStylePosition * stylePosition =
          (const nsStylePosition*)pseudoStyleContext->GetStyleData(eStyleStruct_Position);
        if (NS_STYLE_POSITION_NORMAL != stylePosition->mPosition) {
          // Reset the value
          nsStylePosition* mutablePosition = (nsStylePosition*)
            pseudoStyleContext->GetMutableStyleData(eStyleStruct_Position);
  
          mutablePosition->mPosition = NS_STYLE_POSITION_NORMAL;
        }

        // Create a block box or an inline box depending on the value of
        // the 'display' property
        nsIFrame*     containerFrame;
        nsFrameItems  childFrames;
  
        if (NS_STYLE_DISPLAY_BLOCK == displayValue) {
          NS_NewBlockFrame(&containerFrame);
        } else {
          NS_NewInlineFrame(&containerFrame);
        }
        containerFrame->Init(*aPresContext, aContent, aFrame, pseudoStyleContext, nsnull);

        // Mark the frame as being associated with generated content
        nsFrameState  frameState;
        containerFrame->GetFrameState(&frameState);
        frameState |= NS_FRAME_GENERATED_CONTENT;
        containerFrame->SetFrameState(frameState);

        // Create another pseudo style context to use for all the generated child
        // frames
        nsIStyleContext*  textStyleContext;
        aPresContext->ResolvePseudoStyleContextFor(aContent, nsHTMLAtoms::textPseudo,
                                                   pseudoStyleContext, PR_FALSE,
                                                   &textStyleContext);

        // Now create content objects (and child frames) for each value of the
        // 'content' property
        nsCOMPtr<nsIDocument> document;
        aContent->GetDocument(*getter_AddRefs(document));

        for (PRUint32 contentIndex = 0; contentIndex < contentCount; contentIndex++) {
          nsIFrame* frame;
          nsresult  result;

          // Create a frame
          result = CreateGeneratedFrameFor(aPresContext, document, containerFrame,
                                           aContent, textStyleContext,
                                           styleContent, contentIndex, &frame);
          if (NS_SUCCEEDED(result) && frame) {
            // Add it to the list of child frames
            childFrames.AddChild(frame);

            // If we need to make a first-letter frame and we just
            // made the first frame for the generated content then
            // create the first-letter frame.
            if (aMakeFirstLetterFrame && (0 == contentIndex)) {
              // Create first-letter frame if necessary
              nsCOMPtr<nsIContent> childContent;
              nsresult rv = frame->GetContent(getter_AddRefs(childContent));
              if (NS_SUCCEEDED(rv) && childContent &&
                  ShouldCreateFirstLetterFrame(aPresContext, childContent,
                                               frame)) {
                rv = WrapTextFrame(aPresContext, aState, frame, aContent,
                                   childContent, aFrame, childFrames,
                                   aState.mFloatedItems, aForBlock);
              }
            }
          }
        }
  
        NS_RELEASE(textStyleContext);
        NS_RELEASE(pseudoStyleContext);
        if (childFrames.childList) {
          containerFrame->SetInitialChildList(*aPresContext, nsnull, childFrames.childList);
        }
        *aResult = containerFrame;
        return PR_TRUE;
      }
    }
  }

  return PR_FALSE;
}

#if 0
static PRBool
IsEmptyTextContent(nsIContent* aContent)
{
  PRBool result = PR_FALSE;
  nsCOMPtr<nsITextContent> tc(do_QueryInterface(aContent));
  if (tc) {
    tc->IsOnlyWhitespace(&result);
  }
  return result;
}

#include "nsIDOMHTMLParagraphElement.h"

static PRBool
IsHTMLParagraph(nsIContent* aContent)
{
  nsCOMPtr<nsIDOMHTMLParagraphElement> p(do_QueryInterface(aContent));
  PRBool status = PR_FALSE;
  if (p) {
    status = PR_TRUE;
  }
  return status;
}

static PRBool
IsEmptyContainer(nsIContent* aContainer)
{
  // Check each kid and if each kid is empty text content (or there
  // are no kids) then return true.
  PRInt32 i, numKids;
  aContainer->ChildCount(numKids);
  for (i = 0; i < numKids; i++) {
    nsCOMPtr<nsIContent> kid;
    aContainer->ChildAt(i, *getter_AddRefs(kid));
    if (kid.get()) {
      if (!IsEmptyTextContent(kid)) {
        return PR_FALSE;
      }
    }
  }
  return PR_TRUE;
}

static PRBool
IsEmptyHTMLParagraph(nsIContent* aContent)
{
  if (!IsHTMLParagraph(aContent)) {
    // It's not an HTML paragraph so return false
    return PR_FALSE;
  }
  return IsEmptyContainer(aContent);
}
#endif

nsresult
nsCSSFrameConstructor::CreateInputFrame(nsIPresContext  *aPresContext,
                                        nsIContent      *aContent, 
                                        nsIFrame        *&aFrame,
                                        nsIStyleContext *aStyleContext)
{
  nsresult  rv;

  // Figure out which type of input frame to create
  nsAutoString  val;
  if (NS_OK == aContent->GetAttribute(kNameSpaceID_HTML, nsHTMLAtoms::type, val)) {
    if (val.EqualsIgnoreCase("submit")) {
      rv = ConstructButtonControlFrame(aPresContext, aFrame);
    }
    else if (val.EqualsIgnoreCase("reset")) {
      rv = ConstructButtonControlFrame(aPresContext, aFrame);
    }
    else if (val.EqualsIgnoreCase("button")) {
      rv = ConstructButtonControlFrame(aPresContext, aFrame);
    }
    else if (val.EqualsIgnoreCase("checkbox")) {
      rv = ConstructCheckboxControlFrame(aPresContext, aFrame);
    }
    else if (val.EqualsIgnoreCase("file")) {
      rv = NS_NewFileControlFrame(&aFrame);
    }
    else if (val.EqualsIgnoreCase("hidden")) {
      rv = ConstructButtonControlFrame(aPresContext, aFrame);
    }
    else if (val.EqualsIgnoreCase("image")) {
      rv = NS_NewImageControlFrame(&aFrame);
    }
    else if (val.EqualsIgnoreCase("password")) {
      rv = ConstructTextControlFrame(aPresContext, aFrame, aContent);
    }
    else if (val.EqualsIgnoreCase("radio")) {
      rv = ConstructRadioControlFrame(aPresContext, aFrame, aContent, aStyleContext);
    }
    else if (val.EqualsIgnoreCase("text")) {
      rv = ConstructTextControlFrame(aPresContext, aFrame, aContent);
    }
    else {
      rv = ConstructTextControlFrame(aPresContext, aFrame, aContent);
    }
  } else {
    rv = ConstructTextControlFrame(aPresContext, aFrame, aContent);
  }

  return rv;
}

/****************************************************
 **  BEGIN TABLE SECTION
 ****************************************************/

struct nsTableList {
  nsTableList() {
    mInner = mChild = nsnull;
  }
  void Set(nsIFrame* aInner, nsIFrame* aChild) {
    mInner = aInner;
    mChild = aChild;
  }
  nsIFrame* mInner;
  nsIFrame* mChild;
};

// Construct the outer, inner table frames and the children frames for the table. 
// Tables can have any table-related child.
nsresult
nsCSSFrameConstructor::ConstructTableFrame(nsIPresContext*          aPresContext,
                                           nsFrameConstructorState& aState,
                                           nsIContent*              aContent,
                                           nsIFrame*                aParentFrame,
                                           nsIStyleContext*         aStyleContext,
                                           nsIFrame*&               aNewFrame,
                                           nsTableCreator&          aTableCreator)
{
  nsresult rv = NS_OK;
  nsIFrame* childList;
  nsIFrame* innerFrame;
  nsIFrame* innerChildList = nsnull;
  nsIFrame* captionFrame   = nsnull;

  // Create an anonymous table outer frame which holds the caption and table frame
  aTableCreator.CreateTableOuterFrame(&aNewFrame);

  // Init the table outer frame and see if we need to create a view, e.g.
  // the frame is absolutely positioned
  aNewFrame->Init(*aPresContext, aContent, aParentFrame, aStyleContext, nsnull);
  nsHTMLContainerFrame::CreateViewForFrame(*aPresContext, aNewFrame,
                                           aStyleContext, PR_FALSE);
  nsCOMPtr<nsIStyleContext> parentStyleContext;
  aParentFrame->GetStyleContext(getter_AddRefs(parentStyleContext));
#if 0
  nsCOMPtr<nsIStyleContext> outerStyleContext;
  aPresContext->ResolvePseudoStyleContextFor(aContent, nsHTMLAtoms::tableOuterPseudo,
                                             parentStyleContext,
                                             getter_AddRefs(outerStyleContext));
  aNewFrame->Init(*aPresContext, aContent, aParentFrame, outerStyleContext);
  nsHTMLContainerFrame::CreateViewForFrame(*aPresContext, aNewFrame,
                                           outerStyleContext, PR_FALSE);
#endif
  // Create the inner table frame
  aTableCreator.CreateTableFrame(&innerFrame);
  
  // This gets reset later, since there may also be a caption. 
  // It allows descendants to get at the inner frame before that
  // XXX This is very wrong. You cannot call SetInitialChildList() more
  // than once (see the nsIFrame header file). Either call it once only,
  // _or_ move the caption out into a separate named child list...
  // XXX The other things that's wrong here is that the calls to
  // SetInitialChildList() are bottom-up, and the order is wrong...
  aNewFrame->SetInitialChildList(*aPresContext, nsnull, innerFrame); 
  childList = innerFrame;

  innerFrame->Init(*aPresContext, aContent, aNewFrame, aStyleContext, nsnull);

  nsIFrame* lastChildFrame = nsnull;
  PRInt32   count;
  aContent->ChildCount(count);

  for (PRInt32 i = 0; i < count; i++) { // iterate the child content
    nsCOMPtr<nsIContent> childContent;
    
    if (NS_SUCCEEDED(aContent->ChildAt(i, *getter_AddRefs(childContent)))) {
      nsIFrame* childFrame = nsnull;
      nsIFrame* ignore1;
      nsIFrame* ignore2;
      nsCOMPtr<nsIStyleContext> childStyleContext;

      // Resolve the style context and get its display
      aPresContext->ResolveStyleContextFor(childContent, aStyleContext,
                                           PR_FALSE,
                                           getter_AddRefs(childStyleContext));
      const nsStyleDisplay* styleDisplay = (const nsStyleDisplay*)
        childStyleContext->GetStyleData(eStyleStruct_Display);

      switch (styleDisplay->mDisplay) {
      case NS_STYLE_DISPLAY_TABLE_CAPTION:
        if (nsnull == captionFrame) {  // only allow one caption
          // XXX should absolute items be passed along?
          rv = ConstructTableCaptionFrame(aPresContext, aState, childContent, aNewFrame,
                                          childStyleContext, ignore1, captionFrame, aTableCreator);
        }
        break;

      case NS_STYLE_DISPLAY_TABLE_HEADER_GROUP:
      case NS_STYLE_DISPLAY_TABLE_FOOTER_GROUP:
      case NS_STYLE_DISPLAY_TABLE_ROW_GROUP:
      case NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP:
      {
        PRBool isRowGroup = (NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP != styleDisplay->mDisplay);
        rv = ConstructTableGroupFrame(aPresContext, aState, childContent, innerFrame,
                                      childStyleContext, isRowGroup, childFrame, ignore1,
                                      aTableCreator);
        break;
      }
      case NS_STYLE_DISPLAY_TABLE_ROW:
        rv = ConstructTableRowFrame(aPresContext, aState, childContent, innerFrame,
                                    childStyleContext, childFrame, ignore1, aTableCreator);
        break;

      case NS_STYLE_DISPLAY_TABLE_COLUMN:
        rv = ConstructTableColFrame(aPresContext, aState, childContent, innerFrame,
                                    childStyleContext, childFrame, ignore1, aTableCreator);
        break;


      case NS_STYLE_DISPLAY_TABLE_CELL:
        rv = ConstructTableCellFrame(aPresContext, aState, childContent, innerFrame,
                                     childStyleContext, childFrame, ignore1, ignore2, aTableCreator);
        break;
      default:
        //nsIFrame* nonTableRelatedFrame;
        //nsIAtom*  tag;
        //childContent->GetTag(tag);
        //ConstructFrameByTag(aPresContext, childContent, aNewFrame, tag, childStyleContext,
        //                    aAbsoluteItems, nonTableRelatedFrame);
        //childList->SetNextSibling(nonTableRelatedFrame);
        //NS_IF_RELEASE(tag);
        nsFrameItems childItems;
        TableProcessChild(aPresContext, aState, childContent, innerFrame, parentStyleContext,
                          childItems, aTableCreator);
        childFrame = childItems.childList; // XXX: Need to change this whole function to return a list of frames
                                            // rather than a single frame. - DWH
        break;
      }

      // for every table related frame except captions, link into the child list
      if (nsnull != childFrame) { 
        if (nsnull == lastChildFrame) {
          innerChildList = childFrame;
        } else {
          lastChildFrame->SetNextSibling(childFrame);
        }
        lastChildFrame = childFrame;
      }
    }
  }

  // Set the inner table frame's list of initial child frames
  innerFrame->SetInitialChildList(*aPresContext, nsnull, innerChildList);

  // Set the anonymous table outer frame's initial child list
  aNewFrame->SetInitialChildList(*aPresContext, nsnull, childList);
  return rv;
}

nsresult
nsCSSFrameConstructor::ConstructAnonymousTableFrame(nsIPresContext*          aPresContext, 
                                                    nsFrameConstructorState& aState,
                                                    nsIContent*              aContent, 
                                                    nsIFrame*                aParentFrame, 
                                                    nsIFrame*&               aNewTopFrame,
                                                    nsIFrame*&               aOuterFrame, 
                                                    nsIFrame*&               aInnerFrame,
                                                    nsTableCreator&          aTableCreator)
{
  nsresult rv = NS_OK;
  NS_WARNING("an anonymous table frame was created. \n");
  nsCOMPtr<nsIStyleContext> parentStyleContext;
  aParentFrame->GetStyleContext(getter_AddRefs(parentStyleContext));
  const nsStyleDisplay* parentDisplay = 
   (const nsStyleDisplay*) parentStyleContext->GetStyleData(eStyleStruct_Display);

  PRBool cellIsParent = PR_TRUE;
  nsIFrame* cellFrame;
  nsIFrame* cellBodyFrame;
  nsCOMPtr<nsIStyleContext> cellStyleContext;
  nsIFrame* ignore;

  // construct the ancestors of anonymous table frames
  switch(parentDisplay->mDisplay) {
  case NS_STYLE_DISPLAY_TABLE_CELL:
    break;
  case NS_STYLE_DISPLAY_TABLE_ROW: // construct a cell
    aPresContext->ResolvePseudoStyleContextFor(aContent, nsHTMLAtoms::tableCellPseudo, parentStyleContext,
                                               PR_FALSE, getter_AddRefs(cellStyleContext));
    rv = ConstructTableCellFrame(aPresContext, aState, aContent, aParentFrame, cellStyleContext,
                                 aNewTopFrame, cellFrame, cellBodyFrame, aTableCreator, PR_FALSE); 
    break;
  case NS_STYLE_DISPLAY_TABLE_ROW_GROUP: // construct a row & a cell
  case NS_STYLE_DISPLAY_TABLE_HEADER_GROUP:
  case NS_STYLE_DISPLAY_TABLE_FOOTER_GROUP:
    {
      nsIFrame* rowFrame;
      nsCOMPtr<nsIStyleContext> rowStyleContext;
      aPresContext->ResolvePseudoStyleContextFor(aContent, nsHTMLAtoms::tableRowPseudo, 
                                                 parentStyleContext, PR_FALSE, 
                                                 getter_AddRefs(rowStyleContext));
      rv = ConstructTableRowFrame(aPresContext, aState, aContent, aParentFrame, rowStyleContext, 
                                  aNewTopFrame, rowFrame, aTableCreator);
      if (NS_FAILED(rv)) return rv;
      aPresContext->ResolvePseudoStyleContextFor(aContent, nsHTMLAtoms::tableCellPseudo, parentStyleContext,
                                                 PR_FALSE, getter_AddRefs(cellStyleContext));
      rv = ConstructTableCellFrame(aPresContext, aState, aContent, rowFrame, cellStyleContext, 
                                   ignore, cellFrame, cellBodyFrame, aTableCreator, PR_FALSE);
      rowFrame->SetInitialChildList(*aPresContext, nsnull, cellFrame);
      break;
    }
  case NS_STYLE_DISPLAY_TABLE: // construct a row group, a row, & a cell
    {
      nsIFrame* groupFrame;
      nsCOMPtr<nsIStyleContext> groupStyleContext;
      aPresContext->ResolvePseudoStyleContextFor(aContent, nsHTMLAtoms::tableRowGroupPseudo, 
                                                 parentStyleContext, PR_FALSE, 
                                                 getter_AddRefs(groupStyleContext));
      rv = ConstructTableGroupFrame(aPresContext, aState, aContent, aParentFrame, groupStyleContext, 
                                    PR_TRUE, aNewTopFrame, groupFrame, aTableCreator);
      if (NS_FAILED(rv)) return rv;
      nsIFrame* rowFrame;
      nsCOMPtr<nsIStyleContext> rowStyleContext;
      aPresContext->ResolvePseudoStyleContextFor(aContent, nsHTMLAtoms::tableRowPseudo, 
                                                 parentStyleContext, PR_FALSE, 
                                                 getter_AddRefs(rowStyleContext));
      rv = ConstructTableRowFrame(aPresContext, aState, aContent, aParentFrame, rowStyleContext, 
                                  ignore, rowFrame, aTableCreator);
      if (NS_FAILED(rv)) return rv;
      aPresContext->ResolvePseudoStyleContextFor(aContent, nsHTMLAtoms::tableCellPseudo, parentStyleContext,
                                               PR_FALSE, getter_AddRefs(cellStyleContext));
      rv = ConstructTableCellFrame(aPresContext, aState, aContent, rowFrame, cellStyleContext, 
                                   ignore, cellFrame, cellBodyFrame, aTableCreator, PR_FALSE);
      rowFrame->SetInitialChildList(*aPresContext, nsnull, cellFrame);
      groupFrame->SetInitialChildList(*aPresContext, nsnull, rowFrame);
      break;
    }
  default:
    cellIsParent = PR_FALSE;
  }

  // create the outer table, inner table frames and make the cell frame the parent of the 
  // outer frame
  if (NS_SUCCEEDED(rv)) {
    // create the outer table frame
    nsIFrame* tableParent = nsnull;
    nsIStyleContext* tableParentSC;
    if (cellIsParent) {
      tableParent = cellFrame;
      tableParentSC = cellStyleContext;
    } else {
      tableParent = aParentFrame;
      tableParentSC = parentStyleContext;
    }
    nsCOMPtr<nsIStyleContext> outerStyleContext;
    aPresContext->ResolvePseudoStyleContextFor(aContent, nsHTMLAtoms::tableOuterPseudo, 
                                               tableParentSC, PR_FALSE,
                                               getter_AddRefs(outerStyleContext));
    rv = aTableCreator.CreateTableOuterFrame(&aOuterFrame);
    if (NS_FAILED(rv)) return rv;
    aOuterFrame->Init(*aPresContext, aContent, tableParent, outerStyleContext,
                      nsnull);

    // create the inner table frame
    nsCOMPtr<nsIStyleContext> innerStyleContext;
    aPresContext->ResolvePseudoStyleContextFor(aContent, nsHTMLAtoms::tablePseudo, 
                                               outerStyleContext, PR_FALSE,
                                               getter_AddRefs(innerStyleContext));
    rv = aTableCreator.CreateTableFrame(&aInnerFrame);
    if (NS_FAILED(rv)) return rv;
    aInnerFrame->Init(*aPresContext, aContent, aOuterFrame, innerStyleContext,
                      nsnull);
    //XXX duplicated call here
    aOuterFrame->SetInitialChildList(*aPresContext, nsnull, aInnerFrame);

    if (cellIsParent) {
      cellBodyFrame->SetInitialChildList(*aPresContext, nsnull, aOuterFrame);
    } else {
      aNewTopFrame = aOuterFrame;
    }
  }

  return rv;
}

nsresult
nsCSSFrameConstructor::ConstructTableCaptionFrame(nsIPresContext*          aPresContext,
                                                  nsFrameConstructorState& aState,
                                                  nsIContent*              aContent,
                                                  nsIFrame*                aParentFrame,
                                                  nsIStyleContext*         aStyleContext,
                                                  nsIFrame*&               aNewTopFrame,
                                                  nsIFrame*&               aNewCaptionFrame,
                                                  nsTableCreator&          aTableCreator)
{
  nsresult rv = NS_NewTableCaptionFrame(&aNewCaptionFrame);
  if (NS_FAILED(rv)) return rv;

  const nsStyleDisplay* parentDisplay = GetDisplay(aParentFrame);
  nsIFrame* innerFrame;

  if (NS_STYLE_DISPLAY_TABLE == parentDisplay->mDisplay) { // parent is an outer table
    // determine the inner table frame, it is either aParentFrame or its first child
    nsIFrame* parFrame = aParentFrame;
    aParentFrame->FirstChild(nsnull, &innerFrame);
    const nsStyleDisplay* innerDisplay;
    innerFrame->GetStyleData(eStyleStruct_Display, (const nsStyleStruct *&)innerDisplay);
    if (NS_STYLE_DISPLAY_TABLE != innerDisplay->mDisplay) {
      innerFrame = aParentFrame; 
      innerFrame->GetParent(&parFrame);
    }

    aNewCaptionFrame->Init(*aPresContext, aContent, parFrame, aStyleContext,
                           nsnull);
    innerFrame->SetNextSibling(aNewCaptionFrame);
    // the caller is responsible for calling SetInitialChildList on the outer, inner frames
    aNewTopFrame = aNewCaptionFrame;
  } else { // parent is not a table, need to create a new table
    NS_WARNING("a non table contains a table caption child. \n");
    nsIFrame* outerFrame;
    ConstructAnonymousTableFrame(aPresContext, aState, aContent, aParentFrame,
                                 aNewTopFrame, outerFrame, innerFrame, aTableCreator);
    nsCOMPtr<nsIStyleContext> outerStyleContext;
    outerFrame->GetStyleContext(getter_AddRefs(outerStyleContext));
    nsCOMPtr<nsIStyleContext> adjStyleContext;
    aPresContext->ResolveStyleContextFor(aContent, outerStyleContext,
                                         PR_FALSE, getter_AddRefs(adjStyleContext));
    aNewCaptionFrame->Init(*aPresContext, aContent, outerFrame,
                           adjStyleContext, nsnull);
    innerFrame->SetNextSibling(aNewCaptionFrame);
    //XXX duplicated call here
    outerFrame->SetInitialChildList(*aPresContext, nsnull, innerFrame);
  }

  // The caption frame is a floater container
  PRBool haveFirstLetterStyle, haveFirstLineStyle;
  HaveSpecialBlockStyle(aPresContext, aContent, aStyleContext,
                        &haveFirstLetterStyle, &haveFirstLineStyle);
  nsFrameConstructorSaveState floaterSaveState;
  aState.PushFloaterContainingBlock(aNewCaptionFrame, floaterSaveState,
                                    haveFirstLetterStyle,
                                    haveFirstLineStyle);

  // Process the child content
  nsFrameItems childItems;
  ProcessChildren(aPresContext, aState, aContent, aNewCaptionFrame,
                  PR_TRUE, childItems, PR_TRUE);
  aNewCaptionFrame->SetInitialChildList(*aPresContext, nsnull,
                                        childItems.childList);
  if (aState.mFloatedItems.childList) {
    aNewCaptionFrame->SetInitialChildList(*aPresContext,
                                          nsLayoutAtoms::floaterList,
                                          aState.mFloatedItems.childList);
  }

  return rv;
}

// if aParentFrame is a table, it is assummed that it is an inner table 
nsresult
nsCSSFrameConstructor::ConstructTableGroupFrame(nsIPresContext*          aPresContext,
                                                nsFrameConstructorState& aState,
                                                nsIContent*              aContent,
                                                nsIFrame*                aParentFrame,
                                                nsIStyleContext*         aStyleContext,
                                                PRBool                   aIsRowGroup,
                                                nsIFrame*&               aNewTopFrame, 
                                                nsIFrame*&               aNewGroupFrame,
                                                nsTableCreator&          aTableCreator,
                                                nsTableList*             aToDo)   
{
  nsresult rv = NS_OK;
  const nsStyleDisplay* styleDisplay = (const nsStyleDisplay*)
    aStyleContext->GetStyleData(eStyleStruct_Display);
  nsCOMPtr<nsIStyleContext> styleContext( dont_QueryInterface(aStyleContext) );

  // TRUE if we are being called from above, FALSE if from below (e.g. row)
  PRBool contentDisplayIsGroup = 
    (aIsRowGroup) ? (NS_STYLE_DISPLAY_TABLE_ROW_GROUP    == styleDisplay->mDisplay) ||
                    (NS_STYLE_DISPLAY_TABLE_HEADER_GROUP == styleDisplay->mDisplay) ||
                    (NS_STYLE_DISPLAY_TABLE_FOOTER_GROUP == styleDisplay->mDisplay)
                  : (NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP == styleDisplay->mDisplay);
  if (!contentDisplayIsGroup) {
    NS_ASSERTION(aToDo, "null nsTableList when constructing from below");
  }

  nsCOMPtr<nsIStyleContext> parentStyleContext;
  aParentFrame->GetStyleContext(getter_AddRefs(parentStyleContext));
  const nsStyleDisplay* parentDisplay = 
   (const nsStyleDisplay*) parentStyleContext->GetStyleData(eStyleStruct_Display);

  if (NS_STYLE_DISPLAY_TABLE == parentDisplay->mDisplay) { // parent is an inner table
    if (!contentDisplayIsGroup) { // called from row below
      nsIAtom* pseudoGroup = (aIsRowGroup) ? nsHTMLAtoms::tableRowGroupPseudo : nsHTMLAtoms::tableColGroupPseudo;
      aPresContext->ResolvePseudoStyleContextFor(aContent, pseudoGroup, parentStyleContext,
                                                 PR_FALSE, getter_AddRefs(styleContext));
    }
    // only process the group's children if we're called from above
    rv = ConstructTableGroupFrameOnly(aPresContext, aState, aContent, aParentFrame,
                                      styleContext, aIsRowGroup, aNewTopFrame, aNewGroupFrame, 
                                      aTableCreator, contentDisplayIsGroup);
  } else if (aTableCreator.IsTreeCreator() && 
             parentDisplay->mDisplay == NS_STYLE_DISPLAY_TABLE_ROW_GROUP) {
    // We're a tree view. We want to allow nested row groups.
    // only process the group's children if we're called from above
    rv = ConstructTableGroupFrameOnly(aPresContext, aState, aContent, aParentFrame,
                                      styleContext, aIsRowGroup, aNewTopFrame, aNewGroupFrame, 
                                      aTableCreator, contentDisplayIsGroup);
  } else { // construct anonymous frames
    NS_WARNING("a non table contains a table row or col group child. \n");
    nsIFrame* innerFrame;
    nsIFrame* outerFrame;

    ConstructAnonymousTableFrame(aPresContext, aState, aContent, aParentFrame, aNewTopFrame,
                                 outerFrame, innerFrame, aTableCreator);
    nsCOMPtr<nsIStyleContext> innerStyleContext;
    innerFrame->GetStyleContext(getter_AddRefs(innerStyleContext));
    if (contentDisplayIsGroup) { // called from above
      aPresContext->ResolveStyleContextFor(aContent, innerStyleContext, PR_FALSE,
                                           getter_AddRefs(styleContext));
    } else { // called from row below
      nsIAtom* pseudoGroup = (aIsRowGroup) ? nsHTMLAtoms::tableRowGroupPseudo : nsHTMLAtoms::tableColGroupPseudo;
      aPresContext->ResolvePseudoStyleContextFor(aContent, pseudoGroup, innerStyleContext,
                                                 PR_FALSE, getter_AddRefs(styleContext));
    }
    nsIFrame* topFrame;
    // only process the group's children if we're called from above
    rv = ConstructTableGroupFrameOnly(aPresContext, aState, aContent, innerFrame,
                                      styleContext, aIsRowGroup, topFrame, aNewGroupFrame, 
                                      aTableCreator, contentDisplayIsGroup);
    if (NS_FAILED(rv)) return rv;
    if (contentDisplayIsGroup) { // called from above
      innerFrame->SetInitialChildList(*aPresContext, nsnull, topFrame);
    } else { // called from row below
      aToDo->Set(innerFrame, topFrame);
    }
  }

  return rv;
}

nsresult
nsCSSFrameConstructor::ConstructTableGroupFrameOnly(nsIPresContext*          aPresContext,
                                                    nsFrameConstructorState& aState,
                                                    nsIContent*              aContent,
                                                    nsIFrame*                aParentFrame,
                                                    nsIStyleContext*         aStyleContext,
                                                    PRBool                   aIsRowGroup,
                                                    nsIFrame*&               aNewTopFrame, 
                                                    nsIFrame*&               aNewGroupFrame,
                                                    nsTableCreator&          aTableCreator,
                                                    PRBool                   aProcessChildren) 
{
  nsresult rv = NS_OK;
  const nsStyleDisplay* styleDisplay = 
    (const nsStyleDisplay*) aStyleContext->GetStyleData(eStyleStruct_Display);

  if (IsScrollable(aPresContext, styleDisplay)) {

    // Create an area container for the frame
    rv = (aIsRowGroup) ? aTableCreator.CreateTableRowGroupFrame(&aNewGroupFrame)
                       : aTableCreator.CreateTableColGroupFrame(&aNewGroupFrame);

    rv = BuildScrollFrame(aPresContext, aState, aContent, aNewGroupFrame, aParentFrame,
                                      aStyleContext, aNewTopFrame, PR_FALSE, PR_FALSE, PR_FALSE,
                                      PR_FALSE);

  } else {
    rv = (aIsRowGroup) ? aTableCreator.CreateTableRowGroupFrame(&aNewTopFrame)
                       : aTableCreator.CreateTableColGroupFrame(&aNewTopFrame);
    if (NS_FAILED(rv)) return rv;
    aNewTopFrame->Init(*aPresContext, aContent, aParentFrame, aStyleContext,
                    nsnull);

    aNewGroupFrame = aNewTopFrame;
  }

  if (aProcessChildren) {
    nsFrameItems childItems;

    if (aIsRowGroup) {
      
      // Create some anonymous extras within the tree body.
      if (aTableCreator.IsTreeCreator()) {

     		const nsStyleDisplay *parentDisplay;
        aParentFrame->GetStyleData(eStyleStruct_Display, (const nsStyleStruct *&)parentDisplay);
        if (parentDisplay->mDisplay == NS_STYLE_DISPLAY_TABLE_ROW_GROUP) {
          // We're the child of another row group. If it's lazy, we're lazy.
          nsTreeRowGroupFrame* treeFrame = (nsTreeRowGroupFrame*)aParentFrame;
          if (treeFrame->IsLazy()) {
            ((nsTreeRowGroupFrame*)aNewGroupFrame)->MakeLazy();
            ((nsTreeRowGroupFrame*)aNewGroupFrame)->SetFrameConstructor(this);
          }
        }
        else if (parentDisplay->mDisplay == NS_STYLE_DISPLAY_TABLE) {
          // We're the child of a table.
          // See if our parent is a tree.
          // We will want to have a scrollbar.
          ((nsTreeRowGroupFrame*)aNewGroupFrame)->SetFrameConstructor(this);
          ((nsTreeRowGroupFrame*)aNewGroupFrame)->SetShouldHaveScrollbar();
        }
      }

      TableProcessChildren(aPresContext, aState, aContent, aNewGroupFrame,
                           childItems, aTableCreator);
    } else {
      ProcessChildren(aPresContext, aState, aContent, aNewGroupFrame,
                      PR_FALSE, childItems, PR_FALSE);
    }
    aNewGroupFrame->SetInitialChildList(*aPresContext, nsnull, childItems.childList);
  } 
  
  return rv;
}

nsresult
nsCSSFrameConstructor::ConstructTableRowFrame(nsIPresContext*          aPresContext,
                                              nsFrameConstructorState& aState,
                                              nsIContent*              aContent,
                                              nsIFrame*                aParentFrame,
                                              nsIStyleContext*         aStyleContext,
                                              nsIFrame*&               aNewTopFrame,
                                              nsIFrame*&               aNewRowFrame,
                                              nsTableCreator&          aTableCreator,
                                              nsTableList*             aToDo)
{
  nsresult rv = NS_OK;
  const nsStyleDisplay* display = (const nsStyleDisplay*)
    aStyleContext->GetStyleData(eStyleStruct_Display);

  // TRUE if we are being called from above, FALSE if from below (e.g. cell)
  PRBool contentDisplayIsRow = (NS_STYLE_DISPLAY_TABLE_ROW == display->mDisplay);
  if (!contentDisplayIsRow) {
    NS_ASSERTION(aToDo, "null nsTableList when constructing from below");
  }

  nsCOMPtr<nsIStyleContext> groupStyleContext;
  nsCOMPtr<nsIStyleContext> styleContext( dont_QueryInterface(aStyleContext) );

  const nsStyleDisplay* parentDisplay = GetDisplay(aParentFrame);
  if ((NS_STYLE_DISPLAY_TABLE_ROW_GROUP    == parentDisplay->mDisplay) ||
      (NS_STYLE_DISPLAY_TABLE_HEADER_GROUP == parentDisplay->mDisplay) ||
      (NS_STYLE_DISPLAY_TABLE_FOOTER_GROUP == parentDisplay->mDisplay)) {
    if (!contentDisplayIsRow) { // content is from some (soon to be) child of ours
      aParentFrame = TableGetAsNonScrollFrame(aPresContext, aParentFrame, parentDisplay); 
      aParentFrame->GetStyleContext(getter_AddRefs(groupStyleContext));
      aPresContext->ResolvePseudoStyleContextFor(aContent, nsHTMLAtoms::tableRowPseudo,
                                                 groupStyleContext, PR_FALSE,
                                                 getter_AddRefs(styleContext));
    }
    // only process the row's children if we're called from above
    rv = ConstructTableRowFrameOnly(aPresContext, aState, aContent, aParentFrame,
                                    styleContext, contentDisplayIsRow, aNewRowFrame, 
                                    aTableCreator);
    aNewTopFrame = aNewRowFrame;
  } else { // construct an anonymous row group frame
    nsIFrame* groupFrame;
    nsTableList localToDo;
    nsTableList* toDo = (aToDo) ? aToDo : &localToDo;

    // it may also need to create a table frame
    rv = ConstructTableGroupFrame(aPresContext, aState, aContent, aParentFrame, styleContext,
                                  PR_TRUE, aNewTopFrame, groupFrame, aTableCreator, toDo);
    if (NS_FAILED(rv)) return rv;
    groupFrame->GetStyleContext(getter_AddRefs(groupStyleContext));
    if (contentDisplayIsRow) { // called from above
      aPresContext->ResolveStyleContextFor(aContent, groupStyleContext,
                                           PR_FALSE, getter_AddRefs(styleContext));
    } else { // called from cell below
      aPresContext->ResolvePseudoStyleContextFor(aContent, nsHTMLAtoms::tableRowPseudo,
                                                 groupStyleContext, PR_FALSE,
                                                 getter_AddRefs(styleContext));
    }
    // only process the row's children if we're called from above
    rv = ConstructTableRowFrameOnly(aPresContext, aState, aContent, groupFrame, styleContext, 
                                    contentDisplayIsRow, aNewRowFrame, aTableCreator);
    if (NS_FAILED(rv)) return rv;
    groupFrame->SetInitialChildList(*aPresContext, nsnull, aNewRowFrame);
    if (contentDisplayIsRow) { // called from above
      TableProcessTableList(aPresContext, *toDo);
    }
  }

  return rv;
}


nsresult
nsCSSFrameConstructor::ConstructTableRowFrameOnly(nsIPresContext*          aPresContext,
                                                  nsFrameConstructorState& aState,
                                                  nsIContent*              aContent,
                                                  nsIFrame*                aParentFrame,
                                                  nsIStyleContext*         aStyleContext,
                                                  PRBool                   aProcessChildren,
                                                  nsIFrame*&               aNewRowFrame,
                                                  nsTableCreator&          aTableCreator)
{
  nsresult rv = aTableCreator.CreateTableRowFrame(&aNewRowFrame);
  if (NS_FAILED(rv)) return rv;
  aNewRowFrame->Init(*aPresContext, aContent, aParentFrame, aStyleContext,
                     nsnull);
  if (aProcessChildren) {
    nsFrameItems childItems;
    rv = TableProcessChildren(aPresContext, aState, aContent, aNewRowFrame,
                              childItems, aTableCreator);
    if (NS_FAILED(rv)) return rv;
    aNewRowFrame->SetInitialChildList(*aPresContext, nsnull, childItems.childList);
  }

  return rv;
}


nsresult
nsCSSFrameConstructor::ConstructTableColFrame(nsIPresContext*          aPresContext,
                                              nsFrameConstructorState& aState,
                                              nsIContent*              aContent,
                                              nsIFrame*                aParentFrame,
                                              nsIStyleContext*         aStyleContext,
                                              nsIFrame*&               aNewTopFrame,
                                              nsIFrame*&               aNewColFrame,
                                              nsTableCreator&          aTableCreator)
{
  nsresult rv = NS_OK;
  // the content display here is always table-col and were always called from above
  const nsStyleDisplay* parentDisplay = GetDisplay(aParentFrame);
  if (NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP == parentDisplay->mDisplay) {
    rv = ConstructTableColFrameOnly(aPresContext, aState, aContent, aParentFrame, 
                                    aStyleContext, aNewColFrame, aTableCreator);
    aNewTopFrame = aNewColFrame;
  } else { // construct anonymous col group frame
    nsTableList toDo;
    nsIFrame* groupFrame;
    rv = ConstructTableGroupFrame(aPresContext, aState, aContent, aParentFrame,
                                  aStyleContext, PR_FALSE, aNewTopFrame, groupFrame,
                                  aTableCreator, &toDo);
    if (NS_FAILED(rv)) return rv;
    nsCOMPtr<nsIStyleContext> groupStyleContext; 
    groupFrame->GetStyleContext(getter_AddRefs(groupStyleContext));
    nsCOMPtr<nsIStyleContext> styleContext;
    aPresContext->ResolveStyleContextFor(aContent, groupStyleContext,
                                         PR_FALSE, getter_AddRefs(styleContext));
    rv = ConstructTableColFrameOnly(aPresContext, aState, aContent, groupFrame, 
                                    styleContext, aNewColFrame, aTableCreator);
    if (NS_FAILED(rv)) return rv;
    groupFrame->SetInitialChildList(*aPresContext, nsnull, aNewColFrame);

    // if an anoymous table got created, then set its initial child list
    TableProcessTableList(aPresContext, toDo);
  }

  return rv;
}

nsresult
nsCSSFrameConstructor::ConstructTableColFrameOnly(nsIPresContext*          aPresContext,
                                                  nsFrameConstructorState& aState,
                                                  nsIContent*              aContent,
                                                  nsIFrame*                aParentFrame,
                                                  nsIStyleContext*         aStyleContext,
                                                  nsIFrame*&               aNewColFrame,
                                                  nsTableCreator&          aTableCreator)
{
  nsresult rv = aTableCreator.CreateTableColFrame(&aNewColFrame);
  if (NS_FAILED(rv)) return rv;
  aNewColFrame->Init(*aPresContext, aContent, aParentFrame, 
                     aStyleContext, nsnull);
  nsFrameItems colChildItems;
  rv = ProcessChildren(aPresContext, aState, aContent, aNewColFrame,
                       PR_FALSE, colChildItems, PR_FALSE);
  if (NS_FAILED(rv)) return rv;
  aNewColFrame->SetInitialChildList(*aPresContext, nsnull,
                                    colChildItems.childList);
  return rv;
}

nsresult
nsCSSFrameConstructor::ConstructTableCellFrame(nsIPresContext*          aPresContext,
                                               nsFrameConstructorState& aState,
                                               nsIContent*              aContent,
                                               nsIFrame*                aParentFrame,
                                               nsIStyleContext*         aStyleContext,
                                               nsIFrame*&               aNewTopFrame,
                                               nsIFrame*&               aNewCellFrame,
                                               nsIFrame*&               aNewCellBodyFrame,
                                               nsTableCreator&          aTableCreator,
                                               PRBool                   aProcessChildren)
{
  nsresult rv = NS_OK;

  const nsStyleDisplay* display = (const nsStyleDisplay*)
    aStyleContext->GetStyleData(eStyleStruct_Display);
  // FALSE if we are being called to wrap a cell around the content
  PRBool contentDisplayIsCell = (NS_STYLE_DISPLAY_TABLE_CELL == display->mDisplay);

  nsCOMPtr<nsIStyleContext> parentStyleContext;
  aParentFrame->GetStyleContext(getter_AddRefs(parentStyleContext));
  const nsStyleDisplay* parentDisplay = (const nsStyleDisplay*)
    parentStyleContext->GetStyleData(eStyleStruct_Display);

  nsCOMPtr<nsIStyleContext> styleContext( dont_QueryInterface(aStyleContext) );
  PRBool wrapContent = PR_FALSE;

  if (NS_STYLE_DISPLAY_TABLE_ROW == parentDisplay->mDisplay) {
    nsCOMPtr<nsIStyleContext> styleContextRelease;
    if (!contentDisplayIsCell) { // need to wrap
      aPresContext->ResolvePseudoStyleContextFor(aContent, nsHTMLAtoms::tableCellPseudo,
                                                 parentStyleContext,
                                                 PR_FALSE,
                                                 getter_AddRefs(styleContext));
      wrapContent = PR_TRUE; 
    }
    rv = ConstructTableCellFrameOnly(aPresContext, aState, aContent, aParentFrame,
                                     styleContext, aNewCellFrame, aNewCellBodyFrame,
                                     aTableCreator, aProcessChildren);
    aNewTopFrame = aNewCellFrame;
  } else { // the cell needs some ancestors to be fabricated
    NS_WARNING("WARNING - a non table row contains a table cell child. \n");
    nsTableList toDo;
    nsIFrame* rowFrame;
    rv = ConstructTableRowFrame(aPresContext, aState, aContent, aParentFrame,
                                aStyleContext, aNewTopFrame, rowFrame, aTableCreator,
                                &toDo);
    if (NS_FAILED(rv)) return rv;
    nsCOMPtr<nsIStyleContext> rowStyleContext; 
    rowFrame->GetStyleContext(getter_AddRefs(rowStyleContext));
    if (contentDisplayIsCell) {
      aPresContext->ResolveStyleContextFor(aContent, rowStyleContext,
                                           PR_FALSE, getter_AddRefs(styleContext));
    } else {
      wrapContent = PR_TRUE; // XXX
      aPresContext->ResolvePseudoStyleContextFor(aContent, nsHTMLAtoms::tableCellPseudo,
                                                 rowStyleContext, PR_FALSE,
                                                 getter_AddRefs(styleContext));
    }
    rv = ConstructTableCellFrameOnly(aPresContext, aState, aContent, rowFrame,
                                     styleContext, aNewCellFrame, aNewCellBodyFrame,
                                     aTableCreator, aProcessChildren);
    if (NS_FAILED(rv)) return rv;
    rowFrame->SetInitialChildList(*aPresContext, nsnull, aNewCellFrame);
    TableProcessTableList(aPresContext, toDo);
  }

  return rv;
}

nsresult
nsCSSFrameConstructor::ConstructTableCellFrameOnly(nsIPresContext*          aPresContext,
                                                   nsFrameConstructorState& aState,
                                                   nsIContent*              aContent,
                                                   nsIFrame*                aParentFrame,
                                                   nsIStyleContext*         aStyleContext,
                                                   nsIFrame*&               aNewCellFrame,
                                                   nsIFrame*&               aNewCellBodyFrame,
                                                   nsTableCreator&          aTableCreator,
                                                   PRBool                   aProcessChildren)
{
  nsresult rv;

  // Create a table cell frame
  rv = aTableCreator.CreateTableCellFrame(&aNewCellFrame);
  if (NS_FAILED(rv)) return rv;
 
  // Initialize the table cell frame
  aNewCellFrame->Init(*aPresContext, aContent, aParentFrame, aStyleContext,
                      nsnull);

  // Create an area frame that will format the cell's content
//MathML Mod - RBS
#ifdef INCLUDE_MATHML
  rv = aTableCreator.CreateTableCellInnerFrame(&aNewCellBodyFrame);
#else
  rv = NS_NewTableCellInnerFrame(&aNewCellBodyFrame);
#endif
  if (NS_FAILED(rv)) {
    aNewCellFrame->Destroy(*aPresContext);
    aNewCellFrame = nsnull;
    return rv;
  }
  
  // Resolve pseudo style and initialize the body cell frame
  nsCOMPtr<nsIStyleContext>  bodyPseudoStyle;
  aPresContext->ResolvePseudoStyleContextFor(aContent,
                                             nsHTMLAtoms::cellContentPseudo,
                                             aStyleContext, PR_FALSE,
                                             getter_AddRefs(bodyPseudoStyle));
  aNewCellBodyFrame->Init(*aPresContext, aContent, aNewCellFrame,
                          bodyPseudoStyle, nsnull);

  if (aProcessChildren) {
    PRBool haveFirstLetterStyle, haveFirstLineStyle;
    HaveSpecialBlockStyle(aPresContext, aContent, aStyleContext,
                          &haveFirstLetterStyle, &haveFirstLineStyle);

    // The area frame is a floater container
    nsFrameConstructorSaveState floaterSaveState;
    aState.PushFloaterContainingBlock(aNewCellBodyFrame, floaterSaveState,
                                      haveFirstLetterStyle,
                                      haveFirstLineStyle);

    // Process the child content
    nsFrameItems childItems;
    rv = ProcessChildren(aPresContext, aState, aContent,
                         aNewCellBodyFrame, PR_TRUE, childItems, PR_TRUE);
    if (NS_FAILED(rv)) return rv;

    // if there are any anonymous children create frames for them
    nsCOMPtr<nsIAtom> tagName;
    aContent->GetTag(*getter_AddRefs(tagName));
    if (tagName && tagName.get() == nsXULAtoms::treecell) {
      CreateAnonymousTreeCellFrames(aPresContext, tagName, aState, aContent, aNewCellBodyFrame,
                            childItems);
    }

    aNewCellBodyFrame->SetInitialChildList(*aPresContext, nsnull, childItems.childList);
    if (aState.mFloatedItems.childList) {
      aNewCellBodyFrame->SetInitialChildList(*aPresContext,
                                             nsLayoutAtoms::floaterList,
                                             aState.mFloatedItems.childList);
    }
  }
  aNewCellFrame->SetInitialChildList(*aPresContext, nsnull, aNewCellBodyFrame);

  return rv;
}

// This is only called by table row groups and rows. It allows children that are not
// table related to have a cell wrapped around them.
nsresult
nsCSSFrameConstructor::TableProcessChildren(nsIPresContext*          aPresContext,
                                            nsFrameConstructorState& aState,
                                            nsIContent*              aContent,
                                            nsIFrame*                aParentFrame,
                                            nsFrameItems&            aChildItems,
                                            nsTableCreator&          aTableCreator)
{
  nsresult rv = NS_OK;
  
  // Iterate the child content objects and construct a frame
  PRInt32   count;

  nsCOMPtr<nsIStyleContext> parentStyleContext;
  aParentFrame->GetStyleContext(getter_AddRefs(parentStyleContext));

  const nsStyleDisplay* display = (const nsStyleDisplay*)
        parentStyleContext->GetStyleData(eStyleStruct_Display);
  if (aTableCreator.IsTreeCreator() &&
      (display->mDisplay == NS_STYLE_DISPLAY_TABLE_ROW_GROUP)) {
      // Stop the processing if we're lazy. The tree row group frame builds its children
      // as needed.
      if (((nsTreeRowGroupFrame*)aParentFrame)->IsLazy())
        return NS_OK;
  }
  
  aContent->ChildCount(count);
  for (PRInt32 i = 0; i < count; i++) {
    nsCOMPtr<nsIContent> childContent;
    aContent->ChildAt(i, *getter_AddRefs(childContent));
    rv = TableProcessChild(aPresContext, aState, childContent, aParentFrame, parentStyleContext,
                           aChildItems, aTableCreator);
  }

  return rv;
}

nsresult
nsCSSFrameConstructor::TableProcessChild(nsIPresContext*          aPresContext,
                                         nsFrameConstructorState& aState,
                                         nsIContent*              aChildContent,
                                         nsIFrame*                aParentFrame,
                                         nsIStyleContext*         aParentStyleContext,
                                         nsFrameItems&            aChildItems,
                                         nsTableCreator&          aTableCreator)
{
  nsresult rv = NS_OK;
  
  if (nsnull != aChildContent) {
    nsCOMPtr<nsIStyleContext> childStyleContext;
    aPresContext->ResolveStyleContextFor(aChildContent, aParentStyleContext,
                                         PR_FALSE,
                                         getter_AddRefs(childStyleContext));
    const nsStyleDisplay* childDisplay = (const nsStyleDisplay*)
      childStyleContext->GetStyleData(eStyleStruct_Display);
    if (IsTableRelated(childDisplay->mDisplay)) {
      rv = ConstructFrame(aPresContext, aState, aChildContent, aParentFrame,
                          PR_FALSE, aChildItems);
    } else {
      nsCOMPtr<nsIAtom> tag;
      aChildContent->GetTag(*getter_AddRefs(tag));

      // XXX this needs to be fixed so that the form can work without
      // a frame. This is *disgusting*

      // forms, form controls need a frame but it can't be a child of an inner table
      nsIFormControl* formControl = nsnull;
      nsresult fcResult = aChildContent->QueryInterface(kIFormControlIID, (void**)&formControl);
      NS_IF_RELEASE(formControl);
      if ((nsHTMLAtoms::form == tag.get()) || NS_SUCCEEDED(fcResult)) {
        // if the parent is a table, put the form in the outer table frame
        const nsStyleDisplay* parentDisplay = (const nsStyleDisplay*)
          aParentStyleContext->GetStyleData(eStyleStruct_Display);
        if (parentDisplay->mDisplay == NS_STYLE_DISPLAY_TABLE) {
          nsIFrame* outerFrame; 
          aParentFrame->GetParent(&outerFrame);
          rv = ConstructFrame(aPresContext, aState, aChildContent, outerFrame,
                              PR_FALSE, aChildItems);
          // XXX: Seems like this is going into the inner frame's child list instead of the outer frame. - DWH
        } else {
          rv = ConstructFrame(aPresContext, aState, aChildContent, aParentFrame, 
                              PR_FALSE, aChildItems);
        }
      // wrap it in a table cell, row, row group, table if it is a valid tag or display
      // and not whitespace. For example we don't allow map, head, body, etc.
      } else { 
        if (TableIsValidCellContent(aPresContext, aParentFrame, aChildContent)) {
          PRBool needCell = PR_TRUE;
          nsIDOMCharacterData* domData = nsnull;
          nsresult rv2 = aChildContent->QueryInterface(kIDOMCharacterDataIID, (void**)&domData);
          if ((NS_OK == rv2) && (nsnull != domData)) {
            nsString charData;
            domData->GetData(charData);
            charData = charData.StripWhitespace();
            if ((charData.Length() <= 0) && (charData != " ")) { // XXX check this
              needCell = PR_FALSE;  // only contains whitespace, don't create cell
            }
            NS_RELEASE(domData);
          }
          if (needCell) {
            nsIFrame* cellBodyFrame;
            nsIFrame* cellFrame;
            nsIFrame* childFrame; // XXX: Need to change the ConstructTableCell function to return lists of frames. - DWH
            rv = ConstructTableCellFrame(aPresContext, aState, aChildContent, aParentFrame, childStyleContext, 
                                         childFrame, cellFrame, cellBodyFrame, aTableCreator);
            aChildItems.AddChild(childFrame);
          }
        }
      }
    }
  }
  return rv;
}

PRBool 
nsCSSFrameConstructor::TableIsValidCellContent(nsIPresContext* aPresContext,
                                               nsIFrame*       aParentFrame,
                                               nsIContent*     aContent)
{
  nsCOMPtr<nsIAtom>  tag;
  aContent->GetTag(*getter_AddRefs(tag));

  nsCOMPtr<nsIStyleContext> styleContext;

  nsresult rv = ResolveStyleContext(aPresContext, aParentFrame, aContent, tag, getter_AddRefs(styleContext));
  if (NS_FAILED(rv)) {
    return PR_FALSE;
  }

  const nsStyleDisplay* display = (const nsStyleDisplay*)
    styleContext->GetStyleData(eStyleStruct_Display);

  if (NS_STYLE_DISPLAY_NONE != display->mDisplay) {
    return PR_FALSE;
  } 
   
  // check tags first
  if (  (nsHTMLAtoms::img      == tag.get()) ||
        (nsHTMLAtoms::hr       == tag.get()) ||
        (nsHTMLAtoms::br       == tag.get()) ||
        (nsHTMLAtoms::wbr      == tag.get()) ||
        (nsHTMLAtoms::input    == tag.get()) ||
        (nsHTMLAtoms::textarea == tag.get()) ||
        (nsHTMLAtoms::select   == tag.get()) ||
        (nsHTMLAtoms::applet   == tag.get()) ||
        (nsHTMLAtoms::embed    == tag.get()) ||
        (nsHTMLAtoms::fieldset == tag.get()) ||
        (nsHTMLAtoms::legend   == tag.get()) ||
        (nsHTMLAtoms::object   == tag.get()) ||
        (nsHTMLAtoms::form     == tag.get()) ||
        (nsHTMLAtoms::iframe   == tag.get()) ||
        (nsHTMLAtoms::spacer   == tag.get()) ||
        (nsHTMLAtoms::button   == tag.get()) ||
        (nsHTMLAtoms::label    == tag.get()    )) {
    return PR_TRUE;
  }

#ifdef INCLUDE_XUL
  if (  (nsXULAtoms::button          == tag.get())  ||
	    (nsXULAtoms::titledbutton      == tag.get())  ||
        (nsXULAtoms::grippy          == tag.get())  ||
        (nsXULAtoms::splitter        == tag.get())  ||
        (nsXULAtoms::slider == tag.get())  ||
        (nsXULAtoms::spinner == tag.get())  ||
        (nsXULAtoms::scrollbar == tag.get())  ||
        (nsXULAtoms::scrollbarbutton == tag.get())  ||
        (nsXULAtoms::thumb == tag.get())  ||
        (nsXULAtoms::colorpicker == tag.get())  ||
        (nsXULAtoms::fontpicker == tag.get())  ||
        (nsXULAtoms::radio           == tag.get())  ||
        (nsXULAtoms::text            == tag.get())  ||
        (nsXULAtoms::widget          == tag.get())  ||
        (nsXULAtoms::tree            == tag.get())  ||
        (nsXULAtoms::treechildren    == tag.get())  ||
        (nsXULAtoms::treeitem        == tag.get())  ||
        (nsXULAtoms::treerow         == tag.get())  ||
        (nsXULAtoms::treecell        == tag.get())  ||
        (nsXULAtoms::treeindentation == tag.get())  ||
        (nsXULAtoms::treecol         == tag.get())  ||
        (nsXULAtoms::treecolgroup    == tag.get())  ||
        (nsXULAtoms::treefoot        == tag.get())  ||
        (nsXULAtoms::treepusher      == tag.get())  ||
        (nsXULAtoms::menu          == tag.get())  ||
        (nsXULAtoms::menuitem      == tag.get())  || 
        (nsXULAtoms::menubar       == tag.get())  ||
        (nsXULAtoms::menupopup     == tag.get())  ||
        (nsXULAtoms::popupset  == tag.get())  ||
        (nsXULAtoms::popup == tag.get()) ||
        (nsXULAtoms::toolbox         == tag.get())  ||
        (nsXULAtoms::toolbar         == tag.get())  ||
        (nsXULAtoms::toolbaritem     == tag.get())  ||
        (nsXULAtoms::deck            == tag.get())  ||
        (nsXULAtoms::tabcontrol      == tag.get())  ||
        (nsXULAtoms::tabbox          == tag.get())  ||
        (nsXULAtoms::tabpanel        == tag.get())  ||
        (nsXULAtoms::tabpage         == tag.get())  ||
        (nsXULAtoms::progressmeter   == tag.get())  ||
        (nsXULAtoms::window          == tag.get())) {
    return PR_TRUE;
  }
#endif

//MathML Mod - DJF
#ifdef INCLUDE_MATHML
  if (  (nsMathMLAtoms::math          == tag.get())  ) {
    return PR_TRUE;
  }
#endif
  
  // we should check for display type as well - later
  return PR_FALSE;
}

nsresult
nsCSSFrameConstructor::TableProcessTableList(nsIPresContext* aPresContext,
                                             nsTableList& aTableList)
{
  nsresult rv = NS_OK;
  nsIFrame* inner = (nsIFrame*)aTableList.mInner;
  if (inner) {
    nsIFrame* child = (nsIFrame*)aTableList.mChild;
    rv = inner->SetInitialChildList(*aPresContext, nsnull, child);
  }
  return rv;
}

nsIFrame*
nsCSSFrameConstructor::TableGetAsNonScrollFrame(nsIPresContext*       aPresContext, 
                                                nsIFrame*             aFrame, 
                                                const nsStyleDisplay* aDisplay) 
{
  if (nsnull == aFrame) {
    return nsnull;
  }
  nsIFrame* result = aFrame;
  if (IsScrollable(aPresContext, aDisplay)) {
    aFrame->FirstChild(nsnull, &result);
  }
  return result;
}

//  nsIAtom* pseudoTag;
//  styleContext->GetPseudoType(pseudoTag);
//  if (pseudoTag != nsLayoutAtoms::scrolledContentPseudo) {
//  NS_IF_RELEASE(pseudoTag);

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

PRBool
nsCSSFrameConstructor::IsTableRelated(PRUint8 aDisplay)
{
  return (aDisplay == NS_STYLE_DISPLAY_TABLE)              ||
         (aDisplay == NS_STYLE_DISPLAY_TABLE_HEADER_GROUP) ||
         (aDisplay == NS_STYLE_DISPLAY_TABLE_ROW_GROUP)    ||
         (aDisplay == NS_STYLE_DISPLAY_TABLE_FOOTER_GROUP) ||
         (aDisplay == NS_STYLE_DISPLAY_TABLE_ROW)          ||
         (aDisplay == NS_STYLE_DISPLAY_TABLE_COLUMN)       ||
         (aDisplay == NS_STYLE_DISPLAY_TABLE_CELL);
}

/***********************************************
 * END TABLE SECTION
 ***********************************************/

nsresult
nsCSSFrameConstructor::ConstructDocElementTableFrame(nsIPresContext* aPresContext,
                                                     nsIContent*     aDocElement,
                                                     nsIFrame*       aParentFrame,
                                                     nsIFrame*&      aNewTableFrame)
{
  nsFrameConstructorState state(aPresContext, nsnull, nsnull, nsnull);
  nsFrameItems    frameItems;

  ConstructFrame(aPresContext, state, aDocElement, aParentFrame, PR_FALSE, frameItems);
  aNewTableFrame = frameItems.childList;
  return NS_OK;
}


nsresult
nsCSSFrameConstructor::ConstructDocElementFrame(nsIPresContext*          aPresContext,
                                                nsFrameConstructorState& aState,
                                                nsIContent*              aDocElement,
                                                nsIFrame*                aParentFrame,
                                                nsIStyleContext*         aParentStyleContext,
                                                nsIFrame*&               aNewFrame)
{
  nsIFrame* topFrame = nsnull;

  // Resolve the style context for the document element
  nsCOMPtr<nsIStyleContext>  styleContext;
  aPresContext->ResolveStyleContextFor(aDocElement, aParentStyleContext,
                                       PR_FALSE,
                                       getter_AddRefs(styleContext));
  
  const nsStyleDisplay* display = 
    (const nsStyleDisplay*)styleContext->GetStyleData(eStyleStruct_Display);

  PRBool docElemIsTable = IsTableRelated(display->mDisplay);
  nsIFrame* areaFrame = nsnull;

  // See if we're paginated
  PRBool isPaginated;
  aPresContext->IsPaginated(&isPaginated);
  if (isPaginated) {
    if (docElemIsTable) {
      nsIFrame* tableFrame;
      ConstructDocElementTableFrame(aPresContext, aDocElement, aParentFrame, tableFrame);
      mInitialContainingBlock = tableFrame;
      aNewFrame = tableFrame;
      return NS_OK;
    }

    // Create an area frame for the document element
    PRInt32 nameSpaceID;
    PRBool isBlockFrame = PR_FALSE;
    if (NS_SUCCEEDED(aDocElement->GetNameSpaceID(nameSpaceID)) &&
        nameSpaceID == nsXULAtoms::nameSpaceID) {
      NS_NewBoxFrame(&areaFrame);
    }
    else {
      NS_NewAreaFrame(&areaFrame);
      isBlockFrame = PR_TRUE;
    }

    areaFrame->Init(*aPresContext, aDocElement, aParentFrame, styleContext, nsnull);
    nsHTMLContainerFrame::CreateViewForFrame(*aPresContext, areaFrame,
                                             styleContext, PR_FALSE);

    // The area frame is the "initial containing block"
    mInitialContainingBlock = areaFrame;

    // Process the child content
    nsFrameConstructorSaveState absoluteSaveState;
    nsFrameConstructorSaveState floaterSaveState;
    nsFrameItems                childItems;

    PRBool haveFirstLetterStyle, haveFirstLineStyle;
    HaveSpecialBlockStyle(aPresContext, aDocElement, styleContext,
                          &haveFirstLetterStyle, &haveFirstLineStyle);
    aState.PushAbsoluteContainingBlock(areaFrame, absoluteSaveState);
    aState.PushFloaterContainingBlock(areaFrame, floaterSaveState,
                                      haveFirstLetterStyle,
                                      haveFirstLineStyle);
    ProcessChildren(aPresContext, aState, aDocElement, areaFrame,
                    PR_TRUE, childItems, isBlockFrame);
    
    // Set the initial child lists
    areaFrame->SetInitialChildList(*aPresContext, nsnull,
                                   childItems.childList);
    if (aState.mAbsoluteItems.childList) {
      areaFrame->SetInitialChildList(*aPresContext,
                                     nsLayoutAtoms::absoluteList,
                                     aState.mAbsoluteItems.childList);
    }
    if (aState.mFloatedItems.childList) {
      areaFrame->SetInitialChildList(*aPresContext,
                                     nsLayoutAtoms::floaterList,
                                     aState.mFloatedItems.childList);
    }

    // Return the area frame
    aNewFrame = areaFrame;
  
  } else {
    // Unless the 'overflow' policy forbids scrolling, wrap the frame in a
    // scroll frame.
    nsIFrame* scrollFrame = nsnull;

    if (IsScrollable(aPresContext, display)) {

      // The scrolled frame gets a pseudo element style context
      nsCOMPtr<nsIStyleContext>  scrolledPseudoStyle;
      aPresContext->ResolvePseudoStyleContextFor(nsnull,
                                                 nsLayoutAtoms::scrolledContentPseudo,
                                                 styleContext,
                                                 PR_FALSE,
                                                 getter_AddRefs(scrolledPseudoStyle));

      if (HasGfxScrollbars(aPresContext))  {
         nsIFrame* gfxScrollFrame;
         NS_NewGfxScrollFrame(&gfxScrollFrame);
         gfxScrollFrame->Init(*aPresContext, aDocElement, aParentFrame, styleContext,
                              nsnull);
         NS_NewScrollPortFrame(&scrollFrame);

         scrollFrame->Init(*aPresContext, nsnull, gfxScrollFrame, scrolledPseudoStyle,
                  nsnull);

         nsFrameItems children;

         children.AddChild(scrollFrame);

        // if there are any anonymous children for the nsScrollFrame create frames for them.
        nsCOMPtr<nsIDocument> doc;
        nsresult rv = aDocElement->GetDocument(*getter_AddRefs(doc));
        if (NS_FAILED(rv) || !doc)
          return rv;

         CreateAnonymousFrames(aPresContext, aState, nsnull, doc, gfxScrollFrame,
                               children);

         gfxScrollFrame->SetInitialChildList(*aPresContext, nsnull, children.childList);
         aParentFrame = scrollFrame;
         topFrame = gfxScrollFrame;
      } else {
         NS_NewScrollFrame(&scrollFrame);
         scrollFrame->Init(*aPresContext, nsnull, aParentFrame, styleContext,
                            nsnull);
         aParentFrame = scrollFrame;
         topFrame = scrollFrame;
      }
    
      styleContext = scrolledPseudoStyle;
    }

    nsIFrame* parFrame = aParentFrame;

    if (docElemIsTable) {
      nsIFrame* tableFrame;
      ConstructDocElementTableFrame(aPresContext, aDocElement, parFrame, tableFrame);
      mInitialContainingBlock = tableFrame;
      aNewFrame = tableFrame;
      return NS_OK;
    }
    // Create an area frame for the document element. This serves as the
    // "initial containing block"

    PRInt32 nameSpaceID;
    PRBool isBlockFrame = PR_FALSE;
    if (NS_SUCCEEDED(aDocElement->GetNameSpaceID(nameSpaceID)) &&
        nameSpaceID == nsXULAtoms::nameSpaceID) {
      NS_NewBoxFrame(&areaFrame, NS_BLOCK_DOCUMENT_ROOT);
    }
    else {
      NS_NewDocumentElementFrame(&areaFrame);
      isBlockFrame = PR_TRUE;
    }
    areaFrame->Init(*aPresContext, aDocElement, parFrame, styleContext, nsnull);

    if (scrollFrame) {
      // If the document element is scrollable, then it needs a view. Otherwise,
      // don't bother, because the root frame has a view and the extra view is
      // just overhead we don't need
      nsHTMLContainerFrame::CreateViewForFrame(*aPresContext, areaFrame,
                                               styleContext, PR_FALSE);
    }

    // The area frame is the "initial containing block"
    mInitialContainingBlock = areaFrame;

    // Process the child content
    nsFrameConstructorSaveState absoluteSaveState;
    nsFrameConstructorSaveState floaterSaveState;
    nsFrameItems                childItems;

    // XXX these next lines are wrong for BoxFrame
    PRBool haveFirstLetterStyle, haveFirstLineStyle;
    HaveSpecialBlockStyle(aPresContext, aDocElement, styleContext,
                          &haveFirstLetterStyle, &haveFirstLineStyle);
    aState.PushAbsoluteContainingBlock(areaFrame, absoluteSaveState);
    aState.PushFloaterContainingBlock(areaFrame, floaterSaveState,
                                      haveFirstLetterStyle,
                                      haveFirstLineStyle);
    ProcessChildren(aPresContext, aState, aDocElement, areaFrame,
                    PR_TRUE, childItems, isBlockFrame);

    // See if the document element has a fixed background attachment.
    // Note: the reason we wait until after processing the document element's
    // children is because of special treatment of the background for the HTML
    // element. See BodyFixupRule::MapStyleInto() for details
    const nsStyleColor* color;
    color = (const nsStyleColor*)styleContext->GetStyleData(eStyleStruct_Color);
    if (NS_STYLE_BG_ATTACHMENT_FIXED == color->mBackgroundAttachment) {
      // Fixed background attachments are handled by setting the
      // NS_VIEW_PUBLIC_FLAG_DONT_BITBLT flag bit on the view.
      //
      // If the document element's frame is scrollable, then set the bit on its
      // view; otherwise, set it on the root frame's view. This avoids
      // unnecessarily creating another view and should be faster
      nsIView*  view;

      if (scrollFrame) {
        areaFrame->GetView(&view);
      } else {
        nsIFrame* parentFrame;

        areaFrame->GetParent(&parentFrame);
        parentFrame->GetView(&view);
      }

      NS_ASSERTION(view, "expected a view");
      PRUint32  viewFlags;
      view->GetViewFlags(&viewFlags);
      view->SetViewFlags(viewFlags | NS_VIEW_PUBLIC_FLAG_DONT_BITBLT);
    }
    
    // Set the initial child lists
    areaFrame->SetInitialChildList(*aPresContext, nsnull,
                                   childItems.childList);
    if (aState.mAbsoluteItems.childList) {
      areaFrame->SetInitialChildList(*aPresContext,
                                     nsLayoutAtoms::absoluteList,
                                     aState.mAbsoluteItems.childList);
    }
    if (aState.mFloatedItems.childList) {
      areaFrame->SetInitialChildList(*aPresContext,
                                     nsLayoutAtoms::floaterList,
                                     aState.mFloatedItems.childList);
    }
    if (scrollFrame) {
      scrollFrame->SetInitialChildList(*aPresContext, nsnull, areaFrame);
    }

    aNewFrame = topFrame ? topFrame : areaFrame;
  }

  aState.mFrameManager->SetPrimaryFrameFor(aDocElement, areaFrame);

  // Add a mapping from content object to frame
  return NS_OK;
}

NS_IMETHODIMP
nsCSSFrameConstructor::ConstructRootFrame(nsIPresContext* aPresContext,
                                          nsIContent*     aDocElement,
                                          nsIFrame*&      aNewFrame)
{
#ifdef NS_DEBUG
  {
    nsCOMPtr<nsIDocument>  doc;
    nsIContent*            rootContent;

    // Verify that the content object is really the root content object
    aDocElement->GetDocument(*getter_AddRefs(doc));
    rootContent = doc->GetRootContent();
    NS_ASSERTION(rootContent == aDocElement, "unexpected content");
    NS_RELEASE(rootContent);
  }
#endif

  nsIFrame*                 viewportFrame;
  nsCOMPtr<nsIStyleContext> viewportPseudoStyle;

  // Create the viewport frame
  NS_NewViewportFrame(&viewportFrame);

  // Create a pseudo element style context
  aPresContext->ResolvePseudoStyleContextFor(nsnull, nsLayoutAtoms::viewportPseudo,
                                             nsnull, PR_FALSE,
                                             getter_AddRefs(viewportPseudoStyle));

  { // ensure that the viewport thinks it is a block frame, layout goes pootsy if it doesn't
    nsStyleDisplay* display = (nsStyleDisplay*)viewportPseudoStyle->GetMutableStyleData(eStyleStruct_Display);
    display->mDisplay = NS_STYLE_DISPLAY_BLOCK;
  }

  // Initialize the viewport frame. It has a NULL content object
  viewportFrame->Init(*aPresContext, nsnull, nsnull, viewportPseudoStyle, nsnull);

  // Bind the viewport frame to the root view
  nsCOMPtr<nsIPresShell> presShell;
  aPresContext->GetShell(getter_AddRefs(presShell));
  nsCOMPtr<nsIViewManager> viewManager;
  presShell->GetViewManager(getter_AddRefs(viewManager));
  nsIView*        rootView;

  viewManager->GetRootView(rootView);
  viewportFrame->SetView(rootView);

  // If the device supports scrolling (e.g., in galley mode on the screen and
  // for print-preview, but not when printing), then create a scroll frame that
  // will act as the scrolling mechanism for the viewport. 
  // XXX Do we even need a viewport when printing to a printer?
  // XXX It would be nice to have a better way to query for whether the device
  // is scrollable
  PRBool  isScrollable = PR_TRUE;
  if (aPresContext) {
    nsIDeviceContext* dc;
    aPresContext->GetDeviceContext(&dc);
    if (dc) {
      PRBool  supportsWidgets;
      if (NS_SUCCEEDED(dc->SupportsNativeWidgets(supportsWidgets))) {
        isScrollable = supportsWidgets;
      }
      NS_RELEASE(dc);
    }
  }

  // As long as the webshell doesn't prohibit it, and the device supports
  // it, create a scroll frame that will act as the scolling mechanism for
  // the viewport.
  nsISupports* container;
  if (nsnull != aPresContext) {
    aPresContext->GetContainer(&container);
    if (nsnull != container) {
      nsIWebShell* webShell = nsnull;
      container->QueryInterface(kIWebShellIID, (void**) &webShell);
      if (nsnull != webShell) {
        PRInt32 scrolling = -1;
        webShell->GetScrolling(scrolling);
        if (NS_STYLE_OVERFLOW_HIDDEN == scrolling) {
          isScrollable = PR_FALSE;
        }
        NS_RELEASE(webShell);
      }
      NS_RELEASE(container);
    }
  }

  // If the viewport should offer a scrolling mechanism, then create a
  // scroll frame
  nsIFrame* scrollFrame = nsnull;
  nsIFrame* gfxScrollFrame = nsnull;
  nsCOMPtr<nsIStyleContext> scrollFrameStyle;
  if (isScrollable) {

    nsIFrame* parent = nsnull;

    if (HasGfxScrollbars(aPresContext))  {
       NS_NewGfxScrollFrame(&gfxScrollFrame);
       gfxScrollFrame->Init(*aPresContext, aDocElement, viewportFrame, viewportPseudoStyle,
                            nsnull);
       NS_NewScrollPortFrame(&scrollFrame);
       parent = gfxScrollFrame;
    } else {
       NS_NewScrollFrame(&scrollFrame);
       parent = viewportFrame;
    }

    aPresContext->ResolvePseudoStyleContextFor(nsnull, nsLayoutAtoms::viewportScrollPseudo,
                                               viewportPseudoStyle, PR_FALSE,
                                               getter_AddRefs(scrollFrameStyle));
    scrollFrame->Init(*aPresContext, nsnull, parent, scrollFrameStyle,
                      nsnull);
    
    // Inform the view manager about the root scrollable view
    nsIView*            scrollFrameView;
    nsIScrollableView*  scrollingView;

    scrollFrame->GetView(&scrollFrameView);
    NS_ASSERTION(scrollFrameView, "scroll frame has no view");
    scrollFrameView->QueryInterface(kScrollViewIID, (void**)&scrollingView);
    viewManager->SetRootScrollableView(scrollingView);
  }
  
  PRBool isPaginated;
  aPresContext->IsPaginated(&isPaginated);
  if (isPaginated) {
    nsIFrame* pageSequenceFrame;

    // Create a page sequence frame
    NS_NewSimplePageSequenceFrame(&pageSequenceFrame);
    nsCOMPtr<nsIStyleContext> pageSequenceStyle;
    aPresContext->ResolvePseudoStyleContextFor(nsnull, nsLayoutAtoms::pageSequencePseudo,
                                               (isScrollable ? scrollFrameStyle : viewportPseudoStyle), 
                                               PR_FALSE,
                                               getter_AddRefs(pageSequenceStyle));
    pageSequenceFrame->Init(*aPresContext, nsnull, isScrollable ? scrollFrame :
                            viewportFrame, pageSequenceStyle, nsnull);
    if (isScrollable) {
      nsHTMLContainerFrame::CreateViewForFrame(*aPresContext, pageSequenceFrame,
                                               pageSequenceStyle, PR_TRUE);
    }

    // Create the first page
    nsIFrame* pageFrame;
    NS_NewPageFrame(&pageFrame);

    // The page is the containing block for 'fixed' elements. which are repeated
    // on every page
    mFixedContainingBlock = pageFrame;

    // Initialize the page and force it to have a view. This makes printing of
    // the pages easier and faster.
    nsCOMPtr<nsIStyleContext> pagePseudoStyle;

    aPresContext->ResolvePseudoStyleContextFor(nsnull, nsLayoutAtoms::pagePseudo,
                                               pageSequenceStyle, PR_FALSE,
                                               getter_AddRefs(pagePseudoStyle));

    pageFrame->Init(*aPresContext, nsnull, pageSequenceFrame, pagePseudoStyle,
                    nsnull);
    nsHTMLContainerFrame::CreateViewForFrame(*aPresContext, pageFrame,
                                             pagePseudoStyle, PR_TRUE);

    // The eventual parent of the document element frame
    mDocElementContainingBlock = pageFrame;


    // Set the initial child lists
    pageSequenceFrame->SetInitialChildList(*aPresContext, nsnull, pageFrame);
    if (isScrollable) {
        scrollFrame->SetInitialChildList(*aPresContext, nsnull, pageSequenceFrame);

        if (HasGfxScrollbars(aPresContext)) {
        nsFrameConstructorState state(aPresContext,
                                      nsnull,
                                      nsnull,
                                      nsnull);
        nsFrameItems children;

        children.AddChild(scrollFrame);

        // if there are any anonymous children for the nsScrollFrame create frames for them.
        nsCOMPtr<nsIDocument> doc;
        nsresult rv = aDocElement->GetDocument(*getter_AddRefs(doc));
        if (NS_FAILED(rv) || !doc)
          return rv;

        CreateAnonymousFrames(aPresContext, state, nsnull, doc, gfxScrollFrame,
                              children);

         mGfxScrollFrame = gfxScrollFrame;

         viewportFrame->SetInitialChildList(*aPresContext, nsnull, gfxScrollFrame);
         gfxScrollFrame->SetInitialChildList(*aPresContext, nsnull, children.childList);
      } else
         viewportFrame->SetInitialChildList(*aPresContext, nsnull, scrollFrame);
    } else {
      viewportFrame->SetInitialChildList(*aPresContext, nsnull, pageSequenceFrame);
    }
    

  } else {
    // The viewport is the containing block for 'fixed' elements
    mFixedContainingBlock = viewportFrame;

    // Create the root frame. The document element's frame is a child of the
    // root frame.
    //
    // The root frame serves two purposes:
    // - reserves space for any margins needed for the document element's frame
    // - makes sure that the document element's frame covers the entire canvas
    nsIFrame* rootFrame;
    NS_NewRootFrame(&rootFrame);

    // Initialize the root frame. It gets a pseudo element style context
    nsCOMPtr<nsIStyleContext> canvasPseudoStyle;
    aPresContext->ResolvePseudoStyleContextFor(nsnull, nsLayoutAtoms::canvasPseudo,
                                               isScrollable ? scrollFrameStyle : viewportPseudoStyle,
                                               PR_FALSE,
                                               getter_AddRefs(canvasPseudoStyle));
    rootFrame->Init(*aPresContext, nsnull, isScrollable ? scrollFrame :
                    viewportFrame, canvasPseudoStyle, nsnull);
    if (isScrollable) {
      nsHTMLContainerFrame::CreateViewForFrame(*aPresContext, rootFrame,
                                               canvasPseudoStyle, PR_TRUE);
    }

    // The eventual parent of the document element frame
    mDocElementContainingBlock = rootFrame;

    // Set the initial child lists
    if (isScrollable) {
      scrollFrame->SetInitialChildList(*aPresContext, nsnull, rootFrame);

      if (HasGfxScrollbars(aPresContext)) {
        
        nsFrameConstructorState state(aPresContext,
                                      nsnull,
                                      nsnull,
                                      nsnull);
                                      
        nsFrameItems children;

        children.AddChild(scrollFrame);

        // if there are any anonymous children for the nsScrollFrame create frames for them.
        nsCOMPtr<nsIDocument> doc;
        nsresult rv = aDocElement->GetDocument(*getter_AddRefs(doc));
        if (NS_FAILED(rv) || !doc)
          return rv;

 
        CreateAnonymousFrames(aPresContext, state, nsnull, doc, gfxScrollFrame,
                              children);

        mGfxScrollFrame = gfxScrollFrame;

        viewportFrame->SetInitialChildList(*aPresContext, nsnull, gfxScrollFrame);
        gfxScrollFrame->SetInitialChildList(*aPresContext, nsnull, children.childList);

      } else
         viewportFrame->SetInitialChildList(*aPresContext, nsnull, scrollFrame);

    } else {
      viewportFrame->SetInitialChildList(*aPresContext, nsnull, rootFrame);
    }
  }

  aNewFrame = viewportFrame;
  return NS_OK;  
}

nsresult
nsCSSFrameConstructor::CreatePlaceholderFrameFor(nsIPresContext*  aPresContext,
                                                 nsIFrameManager* aFrameManager,
                                                 nsIContent*      aContent,
                                                 nsIFrame*        aFrame,
                                                 nsIStyleContext* aStyleContext,
                                                 nsIFrame*        aParentFrame,
                                                 nsIFrame**       aPlaceholderFrame)
{
  nsPlaceholderFrame* placeholderFrame;
  nsresult            rv = NS_NewPlaceholderFrame((nsIFrame**)&placeholderFrame);

  if (NS_SUCCEEDED(rv)) {
    // The placeholder frame gets a pseudo style context
    nsCOMPtr<nsIStyleContext>  placeholderPseudoStyle;
    aPresContext->ResolvePseudoStyleContextFor(aContent, nsHTMLAtoms::placeholderPseudo,
                                               aStyleContext,
                                               PR_FALSE,
                                               getter_AddRefs(placeholderPseudoStyle));
    placeholderFrame->Init(*aPresContext, aContent, aParentFrame,
                           placeholderPseudoStyle, nsnull);
  
    // Add mapping from absolutely positioned frame to its placeholder frame
    aFrameManager->SetPlaceholderFrameFor(aFrame, placeholderFrame);

    // The placeholder frame has a pointer back to the out-of-flow frame
    placeholderFrame->SetOutOfFlowFrame(aFrame);
  
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
nsCSSFrameConstructor::ConstructRadioControlFrame(nsIPresContext*  aPresContext,
                                                 nsIFrame*&   aNewFrame,
                                                 nsIContent*  aContent,
                                                 nsIStyleContext* aStyleContext)
{
  nsresult rv = NS_OK;
	if (GetFormElementRenderingMode(aPresContext, eWidgetType_Radio) == eWidgetRendering_Gfx)
		rv = NS_NewGfxRadioControlFrame(&aNewFrame);
	else
    rv = NS_NewNativeRadioControlFrame(&aNewFrame);

  if (NS_FAILED(rv)) {
    aNewFrame = nsnull;
    return rv;
  }

  nsCOMPtr<nsIStyleContext> radioStyle;
  aPresContext->ResolvePseudoStyleContextFor(aContent, nsHTMLAtoms::radioPseudo, 
    aStyleContext, PR_FALSE, getter_AddRefs(radioStyle));
  nsIRadioControlFrame* radio = nsnull;
  if (NS_SUCCEEDED(aNewFrame->QueryInterface(kIRadioControlFrameIID, (void**)&radio))) {
    radio->SetRadioButtonFaceStyleContext(radioStyle);
    NS_RELEASE(radio);
  }
 return rv;
}

nsresult
nsCSSFrameConstructor::ConstructCheckboxControlFrame(nsIPresContext*     		aPresContext,
                                                	 nsIFrame*&          		aNewFrame)
{
  nsresult rv = NS_OK;
	if (GetFormElementRenderingMode(aPresContext, eWidgetType_Checkbox) == eWidgetRendering_Gfx)
		rv = NS_NewGfxCheckboxControlFrame(&aNewFrame);
	else
    rv = NS_NewNativeCheckboxControlFrame(&aNewFrame);

  if (NS_FAILED(rv)) {
    aNewFrame = nsnull;
  }
  return rv;
}

nsresult 
nsCSSFrameConstructor::ConstructButtonLabelFrame(nsIPresContext *aPresContext,
                                              nsIContent     *aContent,
												                      nsIFrame       *&aFrame, 
												                      nsFrameConstructorState& aState,
												                      nsFrameItems&  aFrameItems)
{
    // Construct a button label using generated content specified
    // through style. A style rule of following form must be
    // present in ua.css or other style sheet 
    //   input[type=button][value]:-moz-buttonlabel {
    //      content:attr(value);
    //   }
    // The default for the label is specified with a rule similar to
    // the following:
    //   input[type=reset]:-moz-buttonlabel {
    //      content:"Reset";
    //   }
   nsresult rv = NS_OK;
   nsIStyleContext*  styleContext = nsnull;
   nsIFrame* generatedFrame = nsnull;

     // Probe for generated content before
   aFrame->GetStyleContext(&styleContext);
   if (CreateGeneratedContentFrame(aPresContext, aState, aFrame, aContent,
                                styleContext, nsCSSAtoms::buttonLabelPseudo,
                                PR_FALSE, PR_FALSE, &generatedFrame)) {
      // Add the generated frame to the child list
     aFrameItems.AddChild(generatedFrame);
   }
   return rv;
}


nsresult
nsCSSFrameConstructor::ConstructButtonControlFrame(nsIPresContext*     		aPresContext,
                                                	 nsIFrame*&          		aNewFrame)
{
  nsresult rv = NS_OK;
	if (GetFormElementRenderingMode(aPresContext, eWidgetType_Button) == eWidgetRendering_Gfx)
		rv = NS_NewGfxButtonControlFrame(&aNewFrame);
	else
    rv = NS_NewNativeButtonControlFrame(&aNewFrame);

  if (NS_FAILED(rv)) {
    aNewFrame = nsnull;
  }
  return rv;
}

nsresult
nsCSSFrameConstructor::ConstructTextControlFrame(nsIPresContext*          aPresContext,
                                                 nsIFrame*&               aNewFrame,
                                                 nsIContent*              aContent)
{
  if (!aPresContext) { return NS_ERROR_NULL_POINTER;}
  nsresult rv = NS_OK;

  //Do we want an Autocomplete input text widget?
  nsString val1;
  nsString val2;
  if ((NS_OK == aContent->GetAttribute(kNameSpaceID_HTML, nsHTMLAtoms::autocompletetimeout, val1)) ||
  	  (NS_OK == aContent->GetAttribute(kNameSpaceID_HTML, nsHTMLAtoms::autocompletetype, val2))) {
    if (! val1.IsEmpty() || ! val2.IsEmpty()) {
    	//ducarroz: How can I check if I am in a xul document?
	      rv = NS_NewGfxAutoTextControlFrame(&aNewFrame);
	      if (NS_FAILED(rv)) {
	        aNewFrame = nsnull;
	      }
    	  else
    	    return rv;
    }
  }

  nsWidgetRendering mode;
  aPresContext->GetWidgetRenderingMode(&mode);
  if (eWidgetRendering_Gfx == mode) 
  {
    rv = NS_NewGfxTextControlFrame(&aNewFrame);
    if (NS_FAILED(rv)) {
      aNewFrame = nsnull;
    }
  }
  if (!aNewFrame)
  {
    rv = NS_NewNativeTextControlFrame(&aNewFrame);
  }
  return rv;
}

PRBool
nsCSSFrameConstructor::HasGfxScrollbars(nsIPresContext* aPresContext)
{
  return PR_FALSE;

  /*
  nsWidgetRendering mode;
  aPresContext->GetWidgetRenderingMode(&mode);
  return (eWidgetRendering_Native != mode);
  */
}

nsresult
nsCSSFrameConstructor::ConstructSelectFrame(nsIPresContext*          aPresContext,
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
  nsWidgetRendering mode;
  aPresContext->GetWidgetRenderingMode(&mode);
  const PRInt32 kNoSizeSpecified = -1;

  if (eWidgetRendering_Gfx == mode) {
    // Construct a frame-based listbox or combobox
    nsCOMPtr<nsIDOMHTMLSelectElement> sel(do_QueryInterface(aContent));
    PRInt32 size = 1;
    if (sel) {
      sel->GetSize(&size); 
      PRBool multipleSelect = PR_FALSE;
      sel->GetMultiple(&multipleSelect);
       // Construct a combobox if size=1 or no size is specified and its multiple select
      if (((1 == size) || (kNoSizeSpecified  == size)) && (PR_FALSE == multipleSelect)) {
          // Construct a frame-based combo box.
          // The frame-based combo box is built out of tree parts. A display area, a button and
          // a dropdown list. The display area and button are created through anonymous content.
          // The drop-down list's frame is created explicitly. The combobox frame shares it's content
          // with the drop-down list.
        nsIFrame * comboboxFrame;
        rv = NS_NewComboboxControlFrame(&comboboxFrame);

           // Determine geometric parent for the combobox frame
        nsIFrame* geometricParent = aParentFrame;
        if (aIsAbsolutelyPositioned) {
          geometricParent = aState.mAbsoluteItems.containingBlock;
        } else if (aIsFixedPositioned) {
          geometricParent = aState.mFixedItems.containingBlock;
        }
          // Initialize the combobox frame
        comboboxFrame->Init(*aPresContext, aContent, geometricParent, aStyleContext, nsnull);
        nsIComboboxControlFrame* comboBox = nsnull;
        if (NS_SUCCEEDED(comboboxFrame->QueryInterface(kIComboboxControlFrameIID, (void**)&comboBox))) {
            // Create a listbox
          nsIFrame * listFrame;
          rv = NS_NewListControlFrame(&listFrame);

            // Notify the listbox that it is being used as a dropdown list.
          nsIListControlFrame * listControlFrame;
          if (NS_SUCCEEDED(listFrame->QueryInterface(kIListControlFrameIID, (void**)&listControlFrame))) {
            listControlFrame->SetComboboxFrame(comboboxFrame);
          }
             // Notify combobox that it should use the listbox as it's popup
          comboBox->SetDropDown(listFrame);
         
            // Resolve psuedo element style for the dropdown list 
          nsCOMPtr<nsIStyleContext> listStyle;
          rv = aPresContext->ResolvePseudoStyleContextFor(aContent, 
                                                  nsHTMLAtoms::dropDownListPseudo, 
                                                  aStyleContext,
                                                  PR_FALSE,
                                                  getter_AddRefs(listStyle));

          // Initialize the scroll frame positioned. Note that it is NOT initialized as
          // absolutely positioned.
          nsIFrame* newFrame = nsnull;
          nsIFrame* scrolledFrame = nsnull;
          NS_NewSelectsAreaFrame(&scrolledFrame, NS_BLOCK_SHRINK_WRAP);

          InitializeScrollFrame(aPresContext, aState, listFrame, scrolledFrame, aContent, comboboxFrame,
                               listStyle, PR_TRUE, PR_FALSE, PR_FALSE, PR_TRUE);

          newFrame = listFrame;

            // Set flag so the events go to the listFrame not child frames.
            // XXX: We should replace this with a real widget manager similar
            // to how the nsFormControlFrame works. Re-directing events is a temporary Kludge.
          nsIView *listView; 
          listFrame->GetView(&listView);
          NS_ASSERTION(nsnull != listView,"ListFrame's view is nsnull");
          //listView->SetViewFlags(NS_VIEW_PUBLIC_FLAG_DONT_CHECK_CHILDREN);

          nsIFrame* frame = nsnull;
          if (NS_SUCCEEDED(comboboxFrame->QueryInterface(kIFrameIID, (void**)&frame))) {
            nsFrameItems childItems;
            
              // Create display and button frames from the combobox'es anonymous content
            CreateAnonymousFrames(aPresContext, nsHTMLAtoms::combobox, aState, aContent, frame,
                                  childItems);
          
            frame->SetInitialChildList(*aPresContext, nsnull,
                                   childItems.childList);

              // Initialize the additional popup child list which contains the dropdown list frame.
            nsFrameItems popupItems;
            popupItems.AddChild(listFrame);
            frame->SetInitialChildList(*aPresContext, nsLayoutAtoms::popupList,
                                        popupItems.childList);
          }
           // Don't process, the children, They are already processed by the InitializeScrollFrame
           // call above.
          aProcessChildren = PR_FALSE;
          aNewFrame = comboboxFrame;
          aFrameHasBeenInitialized = PR_TRUE;
        }
      } else {
          // Construct a frame-based list box
        nsIFrame * listFrame;
        rv = NS_NewListControlFrame(&listFrame);
        aNewFrame = listFrame;
        nsIFrame* scrolledFrame = nsnull;
        NS_NewSelectsAreaFrame(&scrolledFrame, NS_BLOCK_SHRINK_WRAP);

        InitializeScrollFrame(aPresContext, aState, listFrame, scrolledFrame, aContent, aParentFrame,
                              aStyleContext, PR_TRUE, aIsAbsolutelyPositioned, aIsFixedPositioned, PR_FALSE);

        aNewFrame = listFrame;

          // Set flag so the events go to the listFrame not child frames.
          // XXX: We should replace this with a real widget manager similar
          // to how the nsFormControlFrame works.
          // Re-directing events is a temporary Kludge.
        nsIView *listView; 
        listFrame->GetView(&listView);
        NS_ASSERTION(nsnull != listView,"ListFrame's view is nsnull");
        //listView->SetViewFlags(NS_VIEW_PUBLIC_FLAG_DONT_CHECK_CHILDREN);
        aFrameHasBeenInitialized = PR_TRUE;
      }
    } else {
      rv = NS_NewNativeSelectControlFrame(&aNewFrame);
    }
  }
  else {
    // Not frame based. Use a SelectFrame which creates a native widget.
     rv = NS_NewNativeSelectControlFrame(&aNewFrame);
  }

  return rv;
}


nsresult
nsCSSFrameConstructor::ConstructFrameByTag(nsIPresContext*          aPresContext,
                                           nsFrameConstructorState& aState,
                                           nsIContent*              aContent,
                                           nsIFrame*                aParentFrame,
                                           nsIAtom*                 aTag,
                                           nsIStyleContext*         aStyleContext,
                                           nsFrameItems&            aFrameItems)
{
  PRBool    processChildren = PR_FALSE;  // whether we should process child content
  PRBool    isAbsolutelyPositioned = PR_FALSE;
  PRBool    isFixedPositioned = PR_FALSE;
  PRBool    isFloating = PR_FALSE;
  PRBool    canBePositioned = PR_TRUE;
  PRBool    frameHasBeenInitialized = PR_FALSE;
  nsIFrame* newFrame = nsnull;  // the frame we construct
  PRBool    isReplaced = PR_FALSE;
  PRBool    addToHashTable = PR_TRUE;
  nsresult  rv = NS_OK;

  if (nsLayoutAtoms::textTagName == aTag) {
    rv = NS_NewTextFrame(&newFrame);
    // Text frames don't go in the content->frame hash table, because
    // they're anonymous. This keeps the hash table smaller
    addToHashTable = PR_FALSE;
    isReplaced = PR_TRUE;   // XXX kipp: temporary
  }
  else {
    nsIHTMLContent *htmlContent;

    // Ignore the tag if it's not HTML content
    if (NS_SUCCEEDED(aContent->QueryInterface(kIHTMLContentIID, (void **)&htmlContent))) {
      NS_RELEASE(htmlContent);
      
      // See if the element is absolute or fixed positioned
      const nsStylePosition* position = (const nsStylePosition*)
        aStyleContext->GetStyleData(eStyleStruct_Position);
      const nsStyleDisplay* display = (const nsStyleDisplay*)
        aStyleContext->GetStyleData(eStyleStruct_Display);
      if (NS_STYLE_POSITION_ABSOLUTE == position->mPosition) {
        isAbsolutelyPositioned = PR_TRUE;
      }
      else if (NS_STYLE_POSITION_FIXED == position->mPosition) {
        isFixedPositioned = PR_TRUE;
      }
      else if (NS_STYLE_FLOAT_NONE != display->mFloats) {
        isFloating = PR_TRUE;
      }

      // Create a frame based on the tag
      if (nsHTMLAtoms::img == aTag) {
        isReplaced = PR_TRUE;
        // XXX If image display is turned off, then use ConstructAlternateImageFrame()
        // instead...
        rv = NS_NewImageFrame(&newFrame);
      }
      else if (nsHTMLAtoms::hr == aTag) {
        rv = NS_NewHRFrame(&newFrame);
      }
      else if (nsHTMLAtoms::br == aTag) {
        rv = NS_NewBRFrame(&newFrame);
        isReplaced = PR_TRUE;
        // BR frames don't go in the content->frame hash table: typically
        // there are many BR content objects and this would increase the size
        // of the hash table, and it's doubtful we need the mapping anyway
        addToHashTable = PR_FALSE;
      }
      else if (nsHTMLAtoms::wbr == aTag) {
        rv = NS_NewWBRFrame(&newFrame);
      }
      else if (nsHTMLAtoms::input == aTag) {
        isReplaced = PR_TRUE;
        rv = CreateInputFrame(aPresContext, aContent, newFrame, aStyleContext);
      }
      else if (nsHTMLAtoms::textarea == aTag) {
        isReplaced = PR_TRUE;
        rv = ConstructTextControlFrame(aPresContext, newFrame, aContent);
      }
      else if (nsHTMLAtoms::select == aTag) {
        isReplaced = PR_TRUE;
        rv = ConstructSelectFrame(aPresContext, aState, aContent, aParentFrame,
                                  aTag, aStyleContext, newFrame,  processChildren,
                                  isAbsolutelyPositioned, frameHasBeenInitialized,
                                  isFixedPositioned, aFrameItems);
      }
      else if (nsHTMLAtoms::applet == aTag) {
        isReplaced = PR_TRUE;
        rv = NS_NewObjectFrame(&newFrame);
      }
      else if (nsHTMLAtoms::embed == aTag) {
        rv = NS_NewObjectFrame(&newFrame);
      }
      else if (nsHTMLAtoms::fieldset == aTag) {
        rv = NS_NewFieldSetFrame(&newFrame);
        processChildren = PR_TRUE;
      }
      else if (nsHTMLAtoms::legend == aTag) {
        rv = NS_NewLegendFrame(&newFrame);
        processChildren = PR_TRUE;
        canBePositioned = PR_FALSE;
      }
      else if (nsHTMLAtoms::object == aTag) {
        isReplaced = PR_TRUE;
        rv = NS_NewObjectFrame(&newFrame);
      }
      else if (nsHTMLAtoms::form == aTag) {
        rv = NS_NewFormFrame(&newFrame);
        processChildren = PR_TRUE;
      }
      else if (nsHTMLAtoms::frameset == aTag) {
        rv = NS_NewHTMLFramesetFrame(&newFrame);
        canBePositioned = PR_FALSE;
      }
      else if (nsHTMLAtoms::iframe == aTag) {
        isReplaced = PR_TRUE;
        rv = NS_NewHTMLFrameOuterFrame(&newFrame);
      }
      else if (nsHTMLAtoms::spacer == aTag) {
        rv = NS_NewSpacerFrame(&newFrame);
        canBePositioned = PR_FALSE;
      }
      else if (nsHTMLAtoms::button == aTag) {
        rv = NS_NewHTMLButtonControlFrame(&newFrame);
        // the html4 button needs to act just like a 
        // regular button except contain html content
        // so it must be replaced or html outside it will
        // draw into its borders. -EDV
        isReplaced = PR_TRUE;
        processChildren = PR_TRUE;
      }
      else if (nsHTMLAtoms::label == aTag) {
        rv = NS_NewLabelFrame(&newFrame);
        processChildren = PR_TRUE;
      }
    }
  }

  // If we succeeded in creating a frame then initialize it, process its
  // children (if requested), and set the initial child list
  if (NS_SUCCEEDED(rv) && (nsnull != newFrame)) {
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
      
      newFrame->Init(*aPresContext, aContent, geometricParent, aStyleContext,
                     nsnull);

      // See if we need to create a view, e.g. the frame is absolutely
      // positioned
      nsHTMLContainerFrame::CreateViewForFrame(*aPresContext, newFrame,
                                               aStyleContext, PR_FALSE);

      // Process the child content if requested
      nsFrameItems childItems;
      if (processChildren) {
        rv = ProcessChildren(aPresContext, aState, aContent, newFrame,
                             PR_TRUE, childItems, PR_FALSE);
      }

      if (nsHTMLAtoms::input == aTag) {
				 // Construct button label frame using generated content
				ConstructButtonLabelFrame(aPresContext, aContent, newFrame, aState, childItems);
			}

      // if there are any anonymous children create frames for them
      CreateAnonymousFrames(aPresContext, aTag, aState, aContent, newFrame,
                            childItems);


      // Set the frame's initial child list
      newFrame->SetInitialChildList(*aPresContext, nsnull, childItems.childList);
    }

    // If the frame is positioned, then create a placeholder frame
    if (canBePositioned && (isAbsolutelyPositioned || isFixedPositioned)) {
      nsIFrame* placeholderFrame;

      CreatePlaceholderFrameFor(aPresContext, aState.mFrameManager, aContent,
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
      CreatePlaceholderFrameFor(aPresContext, aState.mFrameManager, aContent, newFrame,
                                aStyleContext, aParentFrame, &placeholderFrame);

      // Add the floating frame to its containing block's list of child frames
      aState.mFloatedItems.AddChild(newFrame);

      // Add the placeholder frame to the flow
      aFrameItems.AddChild(placeholderFrame);

    } else {
      // Add the newly constructed frame to the flow
      aFrameItems.AddChild(newFrame);
    }

    if (addToHashTable) {
      // Add a mapping from content object to primary frame. Note that for
      // floated and positioned frames this is the out-of-flow frame and not
      // the placeholder frame
      aState.mFrameManager->SetPrimaryFrameFor(aContent, newFrame);
    }
  }

  return rv;
}

// after the node has been constructed and initialized create any
// anonymous content a node needs.
nsresult
nsCSSFrameConstructor::CreateAnonymousFrames(nsIPresContext*          aPresContext,
                                             nsIAtom*                 aTag,
                                             nsFrameConstructorState& aState,
                                             nsIContent*              aParent,
                                             nsIFrame*                aNewFrame,
                                             nsFrameItems&            aChildItems)
{
   // only these tags types can have anonymous content. We do this check for performance
  // reasons. If we did a query interface on every tag it would be very inefficient.
  if (aTag !=  nsHTMLAtoms::input &&
      aTag !=  nsHTMLAtoms::combobox &&
      aTag !=  nsXULAtoms::slider &&
      aTag !=  nsXULAtoms::splitter &&
      aTag !=  nsXULAtoms::scrollbar &&
      aTag !=  nsXULAtoms::menu &&
      aTag !=  nsXULAtoms::menuitem
     ) {
     return NS_OK;

  }
  
  // get the document
  nsCOMPtr<nsIDocument> doc;
  nsresult rv = aParent->GetDocument(*getter_AddRefs(doc));
  if (NS_FAILED(rv) || !doc)
    return rv;

  return CreateAnonymousFrames(aPresContext, aState, aParent, doc, aNewFrame, aChildItems);
}

// after the node has been constructed and initialized create any
// anonymous content a node needs.
nsresult
nsCSSFrameConstructor::CreateAnonymousFrames(nsIPresContext*          aPresContext,
                                             nsFrameConstructorState& aState,
                                             nsIContent*              aParent,
                                             nsIDocument*             aDocument,
                                             nsIFrame*                aNewFrame,
                                             nsFrameItems&            aChildItems)
{

  nsCOMPtr<nsIAnonymousContentCreator> creator(do_QueryInterface(aNewFrame));
  
  if (!creator)
     return NS_OK;

  nsCOMPtr<nsISupportsArray> anonymousItems;
  NS_NewISupportsArray(getter_AddRefs(anonymousItems));


  creator->CreateAnonymousContent(*anonymousItems);
  
  PRUint32 count = 0;
  anonymousItems->Count(&count);

  for (PRUint32 i=0; i < count; i++)
  {
    // get our child's content and set its parent to our content
    nsCOMPtr<nsISupports> node;
    anonymousItems->GetElementAt(i,getter_AddRefs(node));

    nsCOMPtr<nsIContent> content(do_QueryInterface(node));
    content->SetParent(aParent);
    content->SetDocument(aDocument, PR_TRUE);

    // create the frame and attach it to our frame
    ConstructFrame(aPresContext, aState, content, aNewFrame, PR_FALSE, aChildItems);
  }

  return NS_OK;
}

// after the node has been constructed and initialized create any
// anonymous content a node needs.
nsresult
nsCSSFrameConstructor::CreateAnonymousTreeCellFrames(nsIPresContext*  aPresContext,
                                             nsIAtom*                 aTag,
                                             nsFrameConstructorState& aState,
                                             nsIContent*              aParent,
                                             nsIFrame*                aNewFrame,
                                             nsFrameItems&            aChildItems)
{
  // see if the frame implements anonymous content
  nsCOMPtr<nsISupportsArray> anonymousItems;
  NS_NewISupportsArray(getter_AddRefs(anonymousItems));

  nsCOMPtr<nsIDocument> doc;
  nsresult rv = aParent->GetDocument(*getter_AddRefs(doc));
  if (NS_FAILED(rv) || !doc)
    return rv;

  PRInt32 childCount;
  aParent->ChildCount(childCount);
  if (childCount == 0) {
    // Have to do it right here, since the inner cell frame isn't mine, 
    // and i can't have it creating anonymous content.
    nsCOMPtr<nsIDOMNSDocument> nsdoc(do_QueryInterface(doc));
    nsCOMPtr<nsIDOMDocument> document(do_QueryInterface(doc));

    nsString xulNamespace = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
    nsString htmlNamespace = "http://www.w3.org/TR/REC-html40";  
    nsCOMPtr<nsIAtom> classAtom = dont_AddRef(NS_NewAtom("class"));
  
    nsCOMPtr<nsIDOMElement> node;
    nsCOMPtr<nsIContent> content;

    nsAutoString indent;
    nsCOMPtr<nsIDOMElement> parentNode = do_QueryInterface(aParent);
    parentNode->GetAttribute("indent", indent);
    
    nsCOMPtr<nsIDOMElement> boxElement;
    nsCOMPtr<nsIDOMNode> dummy;
    if (indent == "true") {
      // We have to make a box to hold everything.
      nsdoc->CreateElementWithNameSpace("box", xulNamespace, getter_AddRefs(node));
      content = do_QueryInterface(node);
      anonymousItems->AppendElement(content);
      content->SetAttribute(kNameSpaceID_None, classAtom, "tree-icon", PR_FALSE);
      boxElement = do_QueryInterface(content);

      // Make the indentation.
      nsdoc->CreateElementWithNameSpace("treeindentation", xulNamespace, getter_AddRefs(node));
      boxElement->AppendChild(node, getter_AddRefs(dummy));
      
      nsCOMPtr<nsIDOMNode> treeRow;
      nsCOMPtr<nsIDOMNode> treeItem;
      parentNode->GetParentNode(getter_AddRefs(treeRow));
      treeRow->GetParentNode(getter_AddRefs(treeItem));
    
      nsAutoString container;
      nsCOMPtr<nsIDOMElement> treeItemNode = do_QueryInterface(treeItem);
      treeItemNode->GetAttribute("container", container);
        
      // Always make a twisty but disable it for non-containers.
      nsdoc->CreateElementWithNameSpace("titledbutton", xulNamespace, getter_AddRefs(node));
      content = do_QueryInterface(node);
      content->SetAttribute(kNameSpaceID_None, classAtom, "twisty", PR_FALSE);
      if (container != "true")
        content->SetAttribute(kNameSpaceID_None, nsHTMLAtoms::disabled, "true", PR_FALSE);
      else content->SetAttribute(kNameSpaceID_None, nsXULAtoms::treeallowevents, "true", PR_FALSE);

      boxElement->AppendChild(node, getter_AddRefs(dummy));
    }

    nsString classDesc = "tree-button";
    
    nsdoc->CreateElementWithNameSpace("titledbutton", xulNamespace, getter_AddRefs(node));
    content = do_QueryInterface(node);
    content->SetAttribute(kNameSpaceID_None, classAtom, classDesc, PR_FALSE);
    nsAutoString value;

    parentNode->GetAttribute("value", value);
    if (value != "")
      content->SetAttribute(kNameSpaceID_None, nsHTMLAtoms::value, value, PR_FALSE);
  
    nsAutoString crop;
    parentNode->GetAttribute("crop", crop);
    if (crop == "") crop = "right";
    content->SetAttribute(kNameSpaceID_None, nsXULAtoms::crop, crop, PR_FALSE);

    nsAutoString align;
    parentNode->GetAttribute("align", align);
    if (align == "") align = "left";
    content->SetAttribute(kNameSpaceID_None, nsHTMLAtoms::align, align, PR_FALSE);

    if (boxElement) {
      content->SetAttribute(kNameSpaceID_None, nsXULAtoms::flex, "1", PR_FALSE);
      boxElement->AppendChild(node, getter_AddRefs(dummy));
    }
    else anonymousItems->AppendElement(content);
  }

  PRUint32 count = 0;
  anonymousItems->Count(&count);

  for (PRUint32 i=0; i < count; i++)
  {
    // get our child's content and set its parent to our content
    nsCOMPtr<nsISupports> node;
    anonymousItems->GetElementAt(i,getter_AddRefs(node));

    nsCOMPtr<nsIContent> content(do_QueryInterface(node));
    content->SetParent(aParent);
    content->SetDocument(doc, PR_TRUE);

    // create the frame and attach it to our frame
    ConstructFrame(aPresContext, aState, content, aNewFrame, PR_FALSE, aChildItems);
  }

  return NS_OK;
}

#ifdef INCLUDE_XUL
nsresult
nsCSSFrameConstructor::ConstructXULFrame(nsIPresContext*          aPresContext,
                                         nsFrameConstructorState& aState,
                                         nsIContent*              aContent,
                                         nsIFrame*                aParentFrame,
                                         nsIAtom*                 aTag,
                                         nsIStyleContext*         aStyleContext,
                                         nsFrameItems&            aFrameItems,
                                         PRBool&                  haltProcessing)
{
  PRBool    processChildren = PR_FALSE;  // whether we should process child content
  nsresult  rv = NS_OK;
  PRBool    isAbsolutelyPositioned = PR_FALSE;
  PRBool    isFixedPositioned = PR_FALSE;
  PRBool    isReplaced = PR_FALSE;
  PRBool    frameHasBeenInitialized = PR_FALSE;

  // Initialize the new frame
  nsIFrame* newFrame = nsnull;
  nsIFrame* ignore = nsnull; // I have no idea what this is used for.
  nsTreeCreator treeCreator; // Used to make tree views.

  NS_ASSERTION(aTag != nsnull, "null XUL tag");
  if (aTag == nsnull)
    return NS_OK;

  PRInt32 nameSpaceID;
  if (NS_SUCCEEDED(aContent->GetNameSpaceID(nameSpaceID)) &&
      nameSpaceID == nsXULAtoms::nameSpaceID) {
      
    // See if the element is absolutely positioned
    const nsStylePosition* position = (const nsStylePosition*)
      aStyleContext->GetStyleData(eStyleStruct_Position);
    if (NS_STYLE_POSITION_ABSOLUTE == position->mPosition)
      isAbsolutelyPositioned = PR_TRUE;

    // Create a frame based on the tag
    if (aTag == nsXULAtoms::button)
      rv = ConstructButtonControlFrame(aPresContext, newFrame);
    else if (aTag == nsXULAtoms::spinner)
      rv = NS_NewSpinnerFrame(&newFrame);
    else if (aTag == nsXULAtoms::colorpicker)
      rv = NS_NewColorPickerFrame(&newFrame);
    else if (aTag == nsXULAtoms::fontpicker)
      rv = NS_NewFontPickerFrame(&newFrame);
    else if (aTag == nsXULAtoms::radio)
      rv = ConstructRadioControlFrame(aPresContext, newFrame, aContent, aStyleContext);
    else if (aTag == nsXULAtoms::text)
      rv = ConstructTextControlFrame(aPresContext, newFrame, aContent);
    else if (aTag == nsXULAtoms::widget)
      rv = NS_NewObjectFrame(&newFrame);
  
    // TREE CONSTRUCTION
    // The following code is used to construct a tree view from the XUL content
    // model.  
    else if (aTag == nsXULAtoms::treeitem ||
             aTag == nsXULAtoms::treechildren) {
      nsIFrame* newTopFrame;
      rv = ConstructTableGroupFrame(aPresContext, aState, aContent, aParentFrame, aStyleContext,
                                    PR_TRUE, newTopFrame, newFrame, treeCreator, nsnull);
      aFrameItems.AddChild(newFrame);
      return rv;
    }
    else if (aTag == nsXULAtoms::tree)
    {
      nsIFrame* geometricParent = aParentFrame;
      if (NS_STYLE_POSITION_ABSOLUTE == position->mPosition) {
        isAbsolutelyPositioned = PR_TRUE;
        aParentFrame = aState.mAbsoluteItems.containingBlock;
      }
      if (NS_STYLE_POSITION_FIXED == position->mPosition) {
        isFixedPositioned = PR_TRUE;
        aParentFrame = aState.mFixedItems.containingBlock;
      }
      rv = ConstructTableFrame(aPresContext, aState, aContent, geometricParent, aStyleContext,
                               newFrame, treeCreator);
      // Note: table construction function takes care of initializing the frame,
      // processing children, and setting the initial child list
      if (isAbsolutelyPositioned || isFixedPositioned) {
        nsIFrame* placeholderFrame;

        CreatePlaceholderFrameFor(aPresContext, aState.mFrameManager, aContent,
                                  newFrame, aStyleContext, aParentFrame, &placeholderFrame);

        // Add the positioned frame to its containing block's list of child frames
        if (isAbsolutelyPositioned) {
          aState.mAbsoluteItems.AddChild(newFrame);
        } else {
          aState.mFixedItems.AddChild(newFrame);
        }

        // Add the placeholder frame to the flow
        aFrameItems.AddChild(placeholderFrame);
      
      } else {
        // Add the table frame to the flow
        aFrameItems.AddChild(newFrame);
      }
      // Make sure we add a mapping in the content->frame hash table
      goto addToHashTable;
    }
    else if (aTag == nsXULAtoms::treerow)
    {
      // A tree item causes a table row to be constructed that is always
      // slaved to the nearest enclosing table row group (regardless of how
      // deeply nested it is within other tree items).
      rv = ConstructTableRowFrame(aPresContext, aState, aContent, aParentFrame, aStyleContext, 
                                  newFrame, ignore, treeCreator);
      aFrameItems.AddChild(newFrame);
      return rv;
    }
    else if (aTag == nsXULAtoms::treecell)
    {
      // We make a tree cell frame and process the children.
	    // Find out what the attribute value for event allowance is.
      nsIFrame* ignore2;
      rv = ConstructTableCellFrame(aPresContext, aState, aContent, aParentFrame, aStyleContext, 
                                   newFrame, ignore, ignore2, treeCreator);
      aFrameItems.AddChild(newFrame);
      return rv;
    }
    else if (aTag == nsXULAtoms::treeindentation)
    {
      rv = NS_NewTreeIndentationFrame(&newFrame);
    }
    else if (aTag == nsXULAtoms::treepusher)
    {
      processChildren = PR_TRUE;
      isReplaced = PR_TRUE;
      rv = NS_NewTitledButtonFrame(&newFrame);
    }
    // End of TREE CONSTRUCTION code here (there's more later on in the function)

    // TOOLBAR CONSTRUCTION
    else if (aTag == nsXULAtoms::toolbox) {
      processChildren = PR_TRUE;
      rv = NS_NewToolboxFrame(&newFrame);

      const nsStyleDisplay* display = (const nsStyleDisplay*)
      aStyleContext->GetStyleData(eStyleStruct_Display);

      if (IsScrollable(aPresContext, display)) {

        // See if it's absolute positioned or fixed positioned
        if (NS_STYLE_POSITION_ABSOLUTE == position->mPosition) {
          isAbsolutelyPositioned = PR_TRUE;
        } else if (NS_STYLE_POSITION_FIXED == position->mPosition) {
          isFixedPositioned = PR_TRUE;
        }
 
        rv = BuildScrollFrame(aPresContext, aState, aContent, newFrame, aParentFrame,
                              aStyleContext, newFrame, PR_TRUE, isAbsolutelyPositioned, isFixedPositioned,
                              PR_FALSE);
        frameHasBeenInitialized = PR_TRUE;

      }
    }
    else if (aTag == nsXULAtoms::toolbar) {

      processChildren = PR_TRUE;
      rv = NS_NewToolbarFrame(&newFrame);

      const nsStyleDisplay* display = (const nsStyleDisplay*)
      aStyleContext->GetStyleData(eStyleStruct_Display);

       if (IsScrollable(aPresContext, display)) {

        // See if it's absolute positioned or fixed positioned
        if (NS_STYLE_POSITION_ABSOLUTE == position->mPosition) {
          isAbsolutelyPositioned = PR_TRUE;
        } else if (NS_STYLE_POSITION_FIXED == position->mPosition) {
          isFixedPositioned = PR_TRUE;
        }
 
        rv = BuildScrollFrame(aPresContext, aState, aContent, newFrame, aParentFrame,
                              aStyleContext, newFrame, PR_TRUE, isAbsolutelyPositioned, isFixedPositioned,
                              PR_FALSE);
        frameHasBeenInitialized = PR_TRUE;

      }
    }
    else if (aTag == nsXULAtoms::toolbaritem) {
      processChildren = PR_TRUE;
      rv = NS_NewToolbarItemFrame(&newFrame);
    }
    // End of TOOLBAR CONSTRUCTION logic

    // PROGRESS METER CONSTRUCTION
    else if (aTag == nsXULAtoms::progressmeter) {
      processChildren = PR_TRUE;
      isReplaced = PR_TRUE;
      rv = NS_NewProgressMeterFrame(&newFrame);
    }
    // End of PROGRESS METER CONSTRUCTION logic

    // Menu Construction    
    else if (aTag == nsXULAtoms::menu ||
             aTag == nsXULAtoms::menuitem) {
      // A derived class box frame
      // that has custom reflow to prevent menu children
      // from becoming part of the flow.
      processChildren = PR_TRUE; // Will need this to be custom.
      isReplaced = PR_TRUE;
      rv = NS_NewMenuFrame(&newFrame, (aTag == nsXULAtoms::menu));
    }
    else if (aTag == nsXULAtoms::menubar) {
#ifdef XP_MAC // The Mac uses its native menu bar.
      haltProcessing = PR_TRUE;
      return NS_OK;
#else
      processChildren = PR_TRUE;
      rv = NS_NewMenuBarFrame(&newFrame);
#endif
    }
    else if (aTag == nsXULAtoms::popupset) {
      // This frame contains child popups
      processChildren = PR_TRUE;
      isReplaced = PR_TRUE;
      rv = NS_NewPopupSetFrame(&newFrame);
    }
    else if (aTag == nsXULAtoms::menupopup || aTag == nsXULAtoms::popup) {
      // This is its own frame that derives from
      // box.
      processChildren = PR_TRUE;
      isReplaced = PR_TRUE;
      rv = NS_NewMenuPopupFrame(&newFrame);
    }

    // BOX CONSTRUCTION
    else if (aTag == nsXULAtoms::box || aTag == nsXULAtoms::tabbox || 
             aTag == nsXULAtoms::tabpage || aTag == nsXULAtoms::tabcontrol) {
      processChildren = PR_TRUE;
      isReplaced = PR_TRUE;
      rv = NS_NewBoxFrame(&newFrame);

      const nsStyleDisplay* display = (const nsStyleDisplay*)
           aStyleContext->GetStyleData(eStyleStruct_Display);

      // Boxes can scroll.
      if (IsScrollable(aPresContext, display)) {

        // See if it's absolute positioned or fixed positioned
        if (NS_STYLE_POSITION_ABSOLUTE == position->mPosition) {
          isAbsolutelyPositioned = PR_TRUE;
        } else if (NS_STYLE_POSITION_FIXED == position->mPosition) {
          isFixedPositioned = PR_TRUE;
        }
      

        rv = BuildScrollFrame(aPresContext, aState, aContent, newFrame, aParentFrame,
                      aStyleContext, newFrame, PR_TRUE, isAbsolutelyPositioned, isFixedPositioned,
                      PR_FALSE);

        // Build it
        //BuildBoxScrollFrame(aPresContext, aState, aContent, aParentFrame,
         //                     aStyleContext, newFrame, PR_FALSE, isAbsolutelyPositioned, isFixedPositioned,
              //                PR_FALSE);

        frameHasBeenInitialized = PR_TRUE;
      }
    } // End of BOX CONSTRUCTION logic

    // TITLED BUTTON CONSTRUCTION
    else if (aTag == nsXULAtoms::titledbutton) {
      processChildren = PR_TRUE;
      isReplaced = PR_TRUE;
      rv = NS_NewTitledButtonFrame(&newFrame);
    }
    // End of TITLED BUTTON CONSTRUCTION logic

    // DECK CONSTRUCTION
    else if (aTag == nsXULAtoms::deck || aTag == nsXULAtoms::tabpanel) {
      processChildren = PR_TRUE;
      isReplaced = PR_TRUE;
      rv = NS_NewDeckFrame(&newFrame);
    }
    // End of DECK CONSTRUCTION logic

    // TAB CONSTRUCTION
    else if (aTag == nsXULAtoms::tab) {
      processChildren = PR_TRUE;
      isReplaced = PR_TRUE;
      rv = NS_NewTabFrame(&newFrame);
    }
    // End of TAB CONSTRUCTION logic

    // SLIDER CONSTRUCTION
    else if (aTag == nsXULAtoms::slider) {
      processChildren = PR_TRUE;
      isReplaced = PR_TRUE;
      rv = NS_NewSliderFrame(&newFrame);
    }
    // End of SLIDER CONSTRUCTION logic

    // SCROLLBAR CONSTRUCTION
    else if (aTag == nsXULAtoms::scrollbar) {
      processChildren = PR_TRUE;
      isReplaced = PR_TRUE;
      rv = NS_NewScrollbarFrame(&newFrame);

    }
    // End of SCROLLBAR CONSTRUCTION logic

    // SCROLLBUTTON CONSTRUCTION
    else if (aTag == nsXULAtoms::scrollbarbutton) {
      processChildren = PR_TRUE;
      isReplaced = PR_TRUE;
      rv = NS_NewScrollbarButtonFrame(&newFrame);
    }
    // End of SCROLLBUTTON CONSTRUCTION logic

    // THUMB CONSTRUCTION
    else if (aTag == nsXULAtoms::thumb) {
      processChildren = PR_TRUE;
      isReplaced = PR_TRUE;
      rv = NS_NewThumbFrame(&newFrame);
    }
    // End of THUMB CONSTRUCTION logic

    // SPLITTER CONSTRUCTION
    else if (aTag == nsXULAtoms::splitter) {
      processChildren = PR_TRUE;
      isReplaced = PR_TRUE;
      rv = NS_NewSplitterFrame(&newFrame);
    }
    // End of SPLITTER CONSTRUCTION logic

    // GRIPPY CONSTRUCTION
    else if (aTag == nsXULAtoms::grippy) {
      processChildren = PR_TRUE;
      isReplaced = PR_TRUE;
      rv = NS_NewGrippyFrame(&newFrame);
    }
    // End of GRIPPY CONSTRUCTION logic
  }

  // If we succeeded in creating a frame then initialize it, process its
  // children (if requested), and set the initial child list
  if (NS_SUCCEEDED(rv) && newFrame != nsnull) {
    // If the frame is a replaced element, then set the frame state bit
    if (isReplaced) {
      nsFrameState  state;
      newFrame->GetFrameState(&state);
      newFrame->SetFrameState(state | NS_FRAME_REPLACED_ELEMENT);
    }

    if (!frameHasBeenInitialized) {
      nsIFrame* geometricParent = isAbsolutelyPositioned
        ? aState.mAbsoluteItems.containingBlock
        : aParentFrame;
      newFrame->Init(*aPresContext, aContent, geometricParent, aStyleContext,
                     nsnull);

      // See if we need to create a view, e.g. the frame is absolutely positioned
      nsHTMLContainerFrame::CreateViewForFrame(*aPresContext, newFrame,
                                               aStyleContext, PR_FALSE);

      // Process the child content if requested
      nsFrameItems childItems;
      if (processChildren) {
        rv = ProcessChildren(aPresContext, aState, aContent, newFrame,
                             PR_FALSE, childItems, PR_FALSE);
      }

      // if there are any anonymous children create frames for them
      CreateAnonymousFrames(aPresContext, aTag, aState, aContent, newFrame,
                            childItems);

      // Set the frame's initial child list
      newFrame->SetInitialChildList(*aPresContext, nsnull, childItems.childList);
  
    }

    // Add the new frame to our list of frame items.
    aFrameItems.AddChild(newFrame);

    // If the frame is absolutely positioned, then create a placeholder frame
    if (isAbsolutelyPositioned || isFixedPositioned) {
      nsIFrame* placeholderFrame;

      CreatePlaceholderFrameFor(aPresContext, aState.mFrameManager, aContent,
                                newFrame, aStyleContext, aParentFrame, &placeholderFrame);

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

 addToHashTable:
  if (newFrame) {
    // Add a mapping from content object to primary frame. Note that for
    // floated and positioned frames this is the out-of-flow frame and not
    // the placeholder frame
    aState.mFrameManager->SetPrimaryFrameFor(aContent, newFrame);
  }

  return rv;
}
#endif

/**
 * Create a scroll frame whose scrolled content needs a block frame
 */
nsresult
nsCSSFrameConstructor::BuildBlockScrollFrame    (nsIPresContext*          aPresContext,
                                                  nsFrameConstructorState& aState,
                                                  nsIContent*              aContent,
                                                  nsIFrame*                aParentFrame,
                                                  nsIStyleContext*         aStyleContext,
                                                  nsIFrame*&               aNewFrame,
                                                  PRBool                   aProcessChildren,
                                                  PRBool                   aIsAbsolutelyPositioned,
                                                  PRBool                   aIsFixedPositioned,
                                                  PRBool                   aCreateBlock)
{
  // Create an area container for the frame
  nsIFrame* scrolledFrame;
  NS_NewAreaFrame(&scrolledFrame, NS_BLOCK_SHRINK_WRAP | NS_BLOCK_MARGIN_ROOT);


  nsresult rv = BuildScrollFrame(aPresContext, aState, aContent, scrolledFrame, aParentFrame,
                                      aStyleContext, aNewFrame, aProcessChildren, aIsAbsolutelyPositioned, aIsFixedPositioned,
                                      aCreateBlock);

  return rv;
}

/**
 * Create a scroll frame whose scrolled content needs a box frame
 */
nsresult
nsCSSFrameConstructor::BuildBoxScrollFrame      (nsIPresContext*          aPresContext,
                                                  nsFrameConstructorState& aState,
                                                  nsIContent*              aContent,
                                                  nsIFrame*                aParentFrame,
                                                  nsIStyleContext*         aStyleContext,
                                                  nsIFrame*&               aNewFrame,
                                                  PRBool                   aProcessChildren,                                      
                                                  PRBool                   aIsAbsolutelyPositioned,
                                                  PRBool                   aIsFixedPositioned,
                                                  PRBool                   aCreateBlock)
{
  // Create an area container for the frame
  nsIFrame* scrolledFrame;
  NS_NewBoxFrame(&scrolledFrame);


  nsresult rv = BuildScrollFrame(aPresContext, aState, aContent, scrolledFrame, aParentFrame,
                                      aStyleContext, aNewFrame, aProcessChildren, aIsAbsolutelyPositioned, aIsFixedPositioned,
                                      aCreateBlock);

  return rv;
}

/*
nsresult
nsCSSFrameConstructor::BuildScrollFrame      (nsIPresContext*          aPresContext,
                                               nsFrameConstructorState& aState,
                                               nsIContent*              aContent,
                                               nsIFrame*                aParentFrame,
                                               nsIStyleContext*         aStyleContext,
                                               nsIFrame*&               aNewFrame,                                                                                             
                                               nsIFrame*&               aScrollFrame,                                                                                             
                                               PRBool                   aIsAbsolutelyPositioned,
                                               PRBool                   aIsFixedPositioned,
                                               PRBool                   aCreateBlock)
{
  nsFrameItems anonymousItems;

  if (HasGfxScrollbars(aPresContext)) {
    // if gfx we need to make a special scrollframe

    // create the frames
    nsresult rv = BuildGfxScrollFrame(aPresContext, aState, aContent, aParentFrame,
                                       aStyleContext, gfxScrollFrame, anonymousItems);

    // get the scrollframe from the anonymous list
    aScrollFrame = anonymousItems.childList;

    aNewFrame = gfxScrollFrame;
    aNewFrame->SetInitialChildList(*aPresContext, nsnull, anonymousItems.childList);
  } else {
    NS_NewScrollFrame(&scrollFrame);
    aNewFrame = scrollFrame;
    aScrollFrame = scrollFrame;
  }

  aNewFrame->SetInitialChildList(*aPresContext, nsnull, anonymousItems.childList);

  return rv;
}
*/

/**
 * Create a new scroll frame. Populate it and return it.
 */
nsresult
nsCSSFrameConstructor::BuildScrollFrame      (nsIPresContext*          aPresContext,
                                               nsFrameConstructorState& aState,
                                               nsIContent*              aContent,
                                               nsIFrame*                aScrolledFrame,
                                               nsIFrame*                aParentFrame,
                                               nsIStyleContext*         aStyleContext,
                                               nsIFrame*&               aNewFrame,                                                                                             
                                               PRBool                   aProcessChildren,                                              
                                               PRBool                   aIsAbsolutelyPositioned,
                                               PRBool                   aIsFixedPositioned,
                                               PRBool                   aCreateBlock)
{

  nsIFrame* scrollFrame = nsnull;
  nsIFrame* gfxScrollFrame = nsnull;
  nsFrameItems anonymousItems;

  if (HasGfxScrollbars(aPresContext)) {
    // if gfx we need to make a special scrollframe

    // create the frames
    (void) BuildGfxScrollFrame(aPresContext, aState, aContent, aParentFrame,
                               aStyleContext, gfxScrollFrame, anonymousItems);

    // get the scrollframe from the anonymous list
    scrollFrame = anonymousItems.childList;

    aParentFrame = gfxScrollFrame;
    aNewFrame = gfxScrollFrame;
  } else {
    NS_NewScrollFrame(&scrollFrame);
    aNewFrame = scrollFrame;
  }

  // Initialize it
  nsresult rv = InitializeScrollFrame(aPresContext, aState, scrollFrame, aScrolledFrame, aContent, aParentFrame,
                                      aStyleContext, aProcessChildren, aIsAbsolutelyPositioned, aIsFixedPositioned,
                                      aCreateBlock);

  if (HasGfxScrollbars(aPresContext))
     gfxScrollFrame->SetInitialChildList(*aPresContext, nsnull, anonymousItems.childList);

  return rv;
}

nsresult
nsCSSFrameConstructor::BuildGfxScrollFrame (nsIPresContext*          aPresContext,
                                             nsFrameConstructorState& aState,
                                             nsIContent*              aContent,
                                             nsIFrame*                aParentFrame,
                                             nsIStyleContext*         aStyleContext,
                                             nsIFrame*&               aNewFrame,
                                             nsFrameItems&            aAnonymousFrames)
{
  NS_NewGfxScrollFrame(&aNewFrame);

  aNewFrame->Init(*aPresContext, aContent, aParentFrame, aStyleContext,
                  nsnull);

  nsIFrame* scrollbox = nsnull;
  NS_NewScrollPortFrame(&scrollbox);


  aAnonymousFrames.AddChild(scrollbox);

  // if there are any anonymous children for the nsScrollFrame create frames for them.
  nsCOMPtr<nsIDocument> doc;
  nsresult rv = aContent->GetDocument(*getter_AddRefs(doc));
  if (NS_FAILED(rv) || !doc)
    return rv;

  CreateAnonymousFrames(aPresContext, aState, aContent, doc, aNewFrame,
                        aAnonymousFrames);

  return NS_OK;
} 

nsresult
nsCSSFrameConstructor::InitializeScrollFrame(nsIPresContext*          aPresContext,
                                             nsFrameConstructorState& aState,
                                             nsIFrame*                scrollFrame,
                                             nsIFrame*                scrolledFrame,
                                             nsIContent*              aContent,
                                             nsIFrame*                aParentFrame,
                                             nsIStyleContext*         aStyleContext,
                                             PRBool                   aProcessChildren,
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

  // The scroll frame gets the original style context, and the scrolled
  // frame gets a SCROLLED-CONTENT pseudo element style context that
  // inherits the background properties
  nsCOMPtr<nsIStyleContext> scrolledPseudoStyle;
  aPresContext->ResolvePseudoStyleContextFor(aContent,
                                          nsLayoutAtoms::scrolledContentPseudo,
                                          aStyleContext, PR_FALSE,
                                          getter_AddRefs(scrolledPseudoStyle));

  if (HasGfxScrollbars(aPresContext)) {
    scrollFrame->Init(*aPresContext, aContent, geometricParent, scrolledPseudoStyle,
                        nsnull);
  } else {
    scrollFrame->Init(*aPresContext, aContent, geometricParent, aStyleContext,
                        nsnull);
  }
  // Initialize the frame and force it to have a view
  scrolledFrame->Init(*aPresContext, aContent, scrollFrame,
                      scrolledPseudoStyle, nsnull);

  aState.mFrameManager->SetPrimaryFrameFor(aContent, scrolledFrame);

  nsHTMLContainerFrame::CreateViewForFrame(*aPresContext, scrolledFrame,
                                           scrolledPseudoStyle, PR_TRUE);

  if (aProcessChildren) {

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
     
    ProcessChildren(aPresContext, aState, aContent, scrolledFrame, PR_FALSE,
                    childItems, PR_TRUE);

    // if a select is being created with zero options we need to create
    // a special pseudo frame so it can be sized as best it can
    nsCOMPtr<nsIDOMHTMLSelectElement> selectElement;
    nsresult result = aContent->QueryInterface(nsCOMTypeInfo<nsIDOMHTMLSelectElement>::GetIID(),
                                                 (void**)getter_AddRefs(selectElement));
    if (NS_SUCCEEDED(result) && selectElement) {
      PRUint32 numOptions = 0;
      result = selectElement->GetLength(&numOptions);
      if (NS_SUCCEEDED(result) && 0 == numOptions) { 
        nsIStyleContext*  styleContext   = nsnull; 
        nsIFrame*         generatedFrame = nsnull; 
        scrolledFrame->GetStyleContext(&styleContext); 
        if (CreateGeneratedContentFrame(aPresContext, aState, scrolledFrame, aContent, 
                                        styleContext, nsLayoutAtoms::dummyOptionPseudo, 
                                        PR_FALSE, PR_FALSE, &generatedFrame)) { 
          // Add the generated frame to the child list 
          childItems.AddChild(generatedFrame); 
        } 
      }
    } 
    //////////////////////////////////////////////////
    //////////////////////////////////////////////////
    
    // Set the scrolled frame's initial child lists
    scrolledFrame->SetInitialChildList(*aPresContext, nsnull, childItems.childList);
    if (isPositionedContainingBlock && aState.mAbsoluteItems.childList) {
      scrolledFrame->SetInitialChildList(*aPresContext,
                                         nsLayoutAtoms::absoluteList,
                                         aState.mAbsoluteItems.childList);
    }

    if (aState.mFloatedItems.childList) {
      scrolledFrame->SetInitialChildList(*aPresContext,
                                         nsLayoutAtoms::floaterList,
                                         aState.mFloatedItems.childList);
    }
  }

  // Set the scroll frame's initial child list
  scrollFrame->SetInitialChildList(*aPresContext, nsnull, scrolledFrame);
                                            
  return NS_OK;
}

nsresult
nsCSSFrameConstructor::ConstructFrameByDisplayType(nsIPresContext*          aPresContext,
                                                   nsFrameConstructorState& aState,
                                                   const nsStyleDisplay*    aDisplay,
                                                   nsIContent*              aContent,
                                                   nsIFrame*                aParentFrame,
                                                   nsIStyleContext*         aStyleContext,
                                                   PRBool                   aHaveFirstLetterStyle,
                                                   nsFrameItems&            aFrameItems)

{
  PRBool    isAbsolutelyPositioned = PR_FALSE;
  PRBool    isFixedPositioned = PR_FALSE;
  PRBool    isFloating = PR_FALSE;
  PRBool    isBlock = aDisplay->IsBlockLevel();
  nsIFrame* newFrame = nsnull;  // the frame we construct
  nsTableCreator tableCreator; // Used to make table frames.
  PRBool    addToHashTable = PR_TRUE;
  nsresult  rv = NS_OK;

  // Get the position syle info
  const nsStylePosition* position = (const nsStylePosition*)
    aStyleContext->GetStyleData(eStyleStruct_Position);

  // The frame is also a block if it's an inline frame that's floated or
  // absolutely positioned
  if (NS_STYLE_FLOAT_NONE != aDisplay->mFloats) {
    isFloating = PR_TRUE;
  }
  if ((NS_STYLE_DISPLAY_INLINE == aDisplay->mDisplay) &&
      (isFloating || position->IsAbsolutelyPositioned())) {
    isBlock = PR_TRUE;
  }

  // If the frame is a block-level frame and is scrollable, then wrap it
  // in a scroll frame.
  // XXX Ignore tables for the time being
  if ((isBlock && (aDisplay->mDisplay != NS_STYLE_DISPLAY_TABLE)) &&
      IsScrollable(aPresContext, aDisplay)) {

    // See if it's absolute positioned or fixed positioned
    if (NS_STYLE_POSITION_ABSOLUTE == position->mPosition) {
      isAbsolutelyPositioned = PR_TRUE;
    } else if (NS_STYLE_POSITION_FIXED == position->mPosition) {
      isFixedPositioned = PR_TRUE;
    }

    // Build the scrollframe it
    BuildBlockScrollFrame(aPresContext, aState, aContent, aParentFrame,
                          aStyleContext, newFrame, PR_TRUE, isAbsolutelyPositioned, isFixedPositioned,
                          PR_FALSE);
  }
  // See if the frame is absolute or fixed positioned
  else if (position->IsAbsolutelyPositioned() &&
           ((NS_STYLE_DISPLAY_BLOCK == aDisplay->mDisplay) ||
            (NS_STYLE_DISPLAY_INLINE == aDisplay->mDisplay) ||
            (NS_STYLE_DISPLAY_LIST_ITEM == aDisplay->mDisplay))) {

    if (NS_STYLE_POSITION_ABSOLUTE == position->mPosition) {
      isAbsolutelyPositioned = PR_TRUE;
    } else {
      isFixedPositioned = PR_TRUE;
    }

    // Create a frame to wrap up the absolute positioned item
    NS_NewAbsoluteItemWrapperFrame(&newFrame);
    newFrame->Init(*aPresContext, aContent,
                   (isAbsolutelyPositioned
                    ? aState.mAbsoluteItems.containingBlock
                    : aState.mFixedItems.containingBlock),
                   aStyleContext, nsnull);

    // Create a view
    nsHTMLContainerFrame::CreateViewForFrame(*aPresContext, newFrame,
                                             aStyleContext, PR_FALSE);

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
    ProcessChildren(aPresContext, aState, aContent, newFrame, PR_TRUE,
                    childItems, PR_TRUE);

    // Set the frame's initial child list(s)
    newFrame->SetInitialChildList(*aPresContext, nsnull, childItems.childList);
    if (aState.mAbsoluteItems.childList) {
      newFrame->SetInitialChildList(*aPresContext, nsLayoutAtoms::absoluteList,
                                     aState.mAbsoluteItems.childList);
    }
    if (aState.mFloatedItems.childList) {
      newFrame->SetInitialChildList(*aPresContext,
                                    nsLayoutAtoms::floaterList,
                                    aState.mFloatedItems.childList);
    }
  }
  // See if the frame is floated, and it's a block or inline frame
  else if (isFloating &&
           ((NS_STYLE_DISPLAY_BLOCK == aDisplay->mDisplay) ||
            (NS_STYLE_DISPLAY_INLINE == aDisplay->mDisplay) ||
            (NS_STYLE_DISPLAY_LIST_ITEM == aDisplay->mDisplay))) {
    // Create an area frame
    NS_NewFloatingItemWrapperFrame(&newFrame);

    // Initialize the frame
    newFrame->Init(*aPresContext, aContent, aState.mFloatedItems.containingBlock,
                   aStyleContext, nsnull);

    // See if we need to create a view
    nsHTMLContainerFrame::CreateViewForFrame(*aPresContext, newFrame,
                                             aStyleContext, PR_FALSE);

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
    ProcessChildren(aPresContext, aState, aContent, newFrame,
                    PR_TRUE, childItems, PR_TRUE);

    // Set the frame's initial child list(s)
    newFrame->SetInitialChildList(*aPresContext, nsnull, childItems.childList);
    if (aState.mFloatedItems.childList) {
      newFrame->SetInitialChildList(*aPresContext,
                                    nsLayoutAtoms::floaterList,
                                    aState.mFloatedItems.childList);
    }
  }
  // See if it's relatively positioned
  else if ((NS_STYLE_POSITION_RELATIVE == position->mPosition) &&
           ((NS_STYLE_DISPLAY_BLOCK == aDisplay->mDisplay) ||
            (NS_STYLE_DISPLAY_INLINE == aDisplay->mDisplay) ||
            (NS_STYLE_DISPLAY_LIST_ITEM == aDisplay->mDisplay))) {
    // Is it block-level or inline-level?
    PRBool isBlockFrame = PR_FALSE;
    if ((NS_STYLE_DISPLAY_BLOCK == aDisplay->mDisplay) ||
        (NS_STYLE_DISPLAY_LIST_ITEM == aDisplay->mDisplay)) {
      // Create a wrapper frame. No space manager, though
      NS_NewRelativeItemWrapperFrame(&newFrame);
      isBlockFrame = PR_TRUE;
    } else {
      // Create a positioned inline frame
      NS_NewPositionedInlineFrame(&newFrame);
    }

    // Initialize the frame
    newFrame->Init(*aPresContext, aContent, aParentFrame, aStyleContext, nsnull);

    // Create a view
    nsHTMLContainerFrame::CreateViewForFrame(*aPresContext, newFrame,
                                             aStyleContext, PR_FALSE);

    // Process the child content. Relatively positioned frames becomes a
    // container for child frames that are positioned
    nsFrameConstructorSaveState absoluteSaveState;
    nsFrameConstructorSaveState floaterSaveState;
    nsFrameItems                childItems;

    aState.PushAbsoluteContainingBlock(newFrame, absoluteSaveState);
    
    if (isBlockFrame) {
      PRBool haveFirstLetterStyle, haveFirstLineStyle;
      HaveSpecialBlockStyle(aPresContext, aContent, aStyleContext,
                            &haveFirstLetterStyle, &haveFirstLineStyle);
      aState.PushFloaterContainingBlock(newFrame, floaterSaveState,
                                        haveFirstLetterStyle,
                                        haveFirstLineStyle);
    }
    ProcessChildren(aPresContext, aState, aContent, newFrame, PR_TRUE,
                    childItems, isBlockFrame);

    // Set the frame's initial child list
    newFrame->SetInitialChildList(*aPresContext, nsnull, childItems.childList);
    if (aState.mAbsoluteItems.childList) {
      newFrame->SetInitialChildList(*aPresContext, nsLayoutAtoms::absoluteList,
                                    aState.mAbsoluteItems.childList);
    }
    if (isBlockFrame && aState.mFloatedItems.childList) {
      newFrame->SetInitialChildList(*aPresContext,
                                    nsLayoutAtoms::floaterList,
                                    aState.mFloatedItems.childList);
    }
  }
  // See if it's a block frame of some sort
  else if ((NS_STYLE_DISPLAY_BLOCK == aDisplay->mDisplay) ||
           (NS_STYLE_DISPLAY_LIST_ITEM == aDisplay->mDisplay) ||
           (NS_STYLE_DISPLAY_RUN_IN == aDisplay->mDisplay) ||
           (NS_STYLE_DISPLAY_COMPACT == aDisplay->mDisplay) ||
           (NS_STYLE_DISPLAY_INLINE_BLOCK == aDisplay->mDisplay)) {
    // Create the block frame
    rv = NS_NewBlockFrame(&newFrame);
    if (NS_SUCCEEDED(rv)) {
      // That worked so construct the block and its children
      rv = ConstructBlock(aPresContext, aState, aDisplay, aContent,
                          aParentFrame, aStyleContext, newFrame);
    }
  }
  // See if it's an inline frame of some sort
  else if ((NS_STYLE_DISPLAY_INLINE == aDisplay->mDisplay) ||
           (NS_STYLE_DISPLAY_MARKER == aDisplay->mDisplay)) {
    // Create the inline frame
    rv = NS_NewInlineFrame(&newFrame);
    if (NS_SUCCEEDED(rv)) {
      // That worked so construct the inline and its children
      rv = ConstructInline(aPresContext, aState, aDisplay, aContent,
                           aParentFrame, aStyleContext, newFrame);
    }

    // To keep the hash table small don't add inline frames (they're
    // typically things like FONT and B), because we can quickly
    // find them if we need to
    addToHashTable = PR_FALSE;
  }
  // otherwise let the display property influence the frame type to create
  else {
    nsIFrame* ignore;

    // XXX This section now only handles table frames; should be
    // factored out probably

    // Use the 'display' property to choose a frame type
    switch (aDisplay->mDisplay) {
    case NS_STYLE_DISPLAY_TABLE:
    {
      nsIFrame* geometricParent = aParentFrame;
      if (NS_STYLE_POSITION_ABSOLUTE == position->mPosition) {
        isAbsolutelyPositioned = PR_TRUE;
        aParentFrame = aState.mAbsoluteItems.containingBlock;
      }
      if (NS_STYLE_POSITION_FIXED == position->mPosition) {
        isFixedPositioned = PR_TRUE;
        aParentFrame = aState.mFixedItems.containingBlock;
      }
      rv = ConstructTableFrame(aPresContext, aState, aContent, geometricParent,
                               aStyleContext, newFrame, tableCreator);
      // Note: table construction function takes care of initializing
      // the frame, processing children, and setting the initial child
      // list
      goto nearly_done;
    }
  
    // the next 5 cases are only relevant if the parent is not a table, ConstructTableFrame handles children
    case NS_STYLE_DISPLAY_TABLE_CAPTION:
    {
      rv = ConstructTableCaptionFrame(aPresContext, aState, aContent, aParentFrame,
                                      aStyleContext, newFrame, ignore, tableCreator);
      aFrameItems.AddChild(newFrame);
      return rv;
    }
    case NS_STYLE_DISPLAY_TABLE_ROW_GROUP:
    case NS_STYLE_DISPLAY_TABLE_HEADER_GROUP:
    case NS_STYLE_DISPLAY_TABLE_FOOTER_GROUP:
    case NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP:
    {
      PRBool isRowGroup = (NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP != aDisplay->mDisplay);
      rv = ConstructTableGroupFrame(aPresContext, aState, aContent, aParentFrame,
                                    aStyleContext, isRowGroup, newFrame, ignore, tableCreator);
      aFrameItems.AddChild(newFrame);
      return rv;
    }
   
    case NS_STYLE_DISPLAY_TABLE_COLUMN:
      rv = ConstructTableColFrame(aPresContext, aState, aContent, aParentFrame, aStyleContext, 
                                  newFrame, ignore, tableCreator);
      aFrameItems.AddChild(newFrame);
      return rv;
  
  
    case NS_STYLE_DISPLAY_TABLE_ROW:
      rv = ConstructTableRowFrame(aPresContext, aState, aContent, aParentFrame, aStyleContext, 
                                  newFrame, ignore, tableCreator);
      aFrameItems.AddChild(newFrame);
      return rv;
  
    case NS_STYLE_DISPLAY_TABLE_CELL:
      {
        nsIFrame* ignore2;
        rv = ConstructTableCellFrame(aPresContext, aState, aContent, aParentFrame, aStyleContext, 
                                     newFrame, ignore, ignore2, tableCreator);
        aFrameItems.AddChild(newFrame);
        return rv;
      }
  
    default:
      // Don't create any frame for content that's not displayed...
      break;
    }
  }

  // If the frame is absolutely positioned, then create a placeholder frame
 nearly_done:
  if (isAbsolutelyPositioned || isFixedPositioned) {
    nsIFrame* placeholderFrame;

    CreatePlaceholderFrameFor(aPresContext, aState.mFrameManager, aContent,
                              newFrame, aStyleContext, aParentFrame, &placeholderFrame);

    // Add the positioned frame to its containing block's list of child frames
    if (isAbsolutelyPositioned) {
      aState.mAbsoluteItems.AddChild(newFrame);
    } else {
      aState.mFixedItems.AddChild(newFrame);
    }

    // Add the placeholder frame to the flow
    aFrameItems.AddChild(placeholderFrame);

  } else if (isFloating) {
    nsIFrame* placeholderFrame;
    CreatePlaceholderFrameFor(aPresContext, aState.mFrameManager, aContent, newFrame,
                              aStyleContext, aParentFrame, &placeholderFrame);

    // Add the floating frame to its containing block's list of child frames
    aState.mFloatedItems.AddChild(newFrame);

    // Add the placeholder frame to the flow
    aFrameItems.AddChild(placeholderFrame);
  } else if (nsnull != newFrame) {
    // Add the frame we just created to the flowed list
    aFrameItems.AddChild(newFrame);
  }

  if (newFrame && addToHashTable) {
    // Add a mapping from content object to primary frame. Note that for
    // floated and positioned frames this is the out-of-flow frame and not
    // the placeholder frame
    aState.mFrameManager->SetPrimaryFrameFor(aContent, newFrame);
  }

  return rv;
}

nsresult
nsCSSFrameConstructor::ConstructBlock(nsIPresContext*          aPresContext,
                                      nsFrameConstructorState& aState,
                                      const nsStyleDisplay*    aDisplay,
                                      nsIContent*              aContent,
                                      nsIFrame*                aParentFrame,
                                      nsIStyleContext*         aStyleContext,
                                      nsIFrame*                aNewFrame)
{
  // Initialize the frame
  aNewFrame->Init(*aPresContext, aContent, aParentFrame, aStyleContext,
                  nsnull);

  // See if we need to create a view, e.g. the frame is absolutely positioned
  nsHTMLContainerFrame::CreateViewForFrame(*aPresContext, aNewFrame,
                                           aStyleContext, PR_FALSE);

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
  nsresult rv = ProcessChildren(aPresContext, aState, aContent, aNewFrame,
                                PR_TRUE, childItems, PR_TRUE);

  // Set the frame's initial child list
  aNewFrame->SetInitialChildList(*aPresContext, nsnull, childItems.childList);

  // Set the frame's floater list if there were any floated children
  if (aState.mFloatedItems.childList) {
    aNewFrame->SetInitialChildList(*aPresContext,
                                   nsLayoutAtoms::floaterList,
                                   aState.mFloatedItems.childList);
  }

  return rv;
}

nsresult
nsCSSFrameConstructor::ConstructInline(nsIPresContext*          aPresContext,
                                       nsFrameConstructorState& aState,
                                       const nsStyleDisplay*    aDisplay,
                                       nsIContent*              aContent,
                                       nsIFrame*                aParentFrame,
                                       nsIStyleContext*         aStyleContext,
                                       nsIFrame*                aNewFrame)
{
  // Initialize the frame
  aNewFrame->Init(*aPresContext, aContent, aParentFrame, aStyleContext,
                  nsnull);

  // See if we need to create a view, e.g. the frame is absolutely positioned
  nsHTMLContainerFrame::CreateViewForFrame(*aPresContext, aNewFrame,
                                           aStyleContext, PR_FALSE);

  // Process the child content
  nsFrameItems childItems;
  nsresult rv = ProcessChildren(aPresContext, aState, aContent,
                                aNewFrame, PR_TRUE, childItems, PR_FALSE);

  // Set the frame's initial child list
  aNewFrame->SetInitialChildList(*aPresContext, nsnull, childItems.childList);

  return rv;
}

nsresult
nsCSSFrameConstructor::GetAdjustedParentFrame(nsIFrame*  aCurrentParentFrame, 
                                              PRUint8    aChildDisplayType, 
                                              nsIFrame*& aNewParentFrame)
{
  NS_PRECONDITION(nsnull!=aCurrentParentFrame, "bad arg aCurrentParentFrame");

  nsresult rv = NS_OK;
  // by default, the new parent frame is the given current parent frame
  aNewParentFrame = aCurrentParentFrame;
  if (nsnull != aCurrentParentFrame) {
    const nsStyleDisplay* currentParentDisplay;
    aCurrentParentFrame->GetStyleData(eStyleStruct_Display, (const nsStyleStruct *&)currentParentDisplay);
    if (NS_STYLE_DISPLAY_TABLE == currentParentDisplay->mDisplay) {
      if (NS_STYLE_DISPLAY_TABLE_CAPTION != aChildDisplayType) {
        nsIFrame *innerTableFrame = nsnull;
        aCurrentParentFrame->FirstChild(nsnull, &innerTableFrame);
        if (nsnull != innerTableFrame) {
          const nsStyleDisplay* innerTableDisplay;
          innerTableFrame->GetStyleData(eStyleStruct_Display, (const nsStyleStruct *&)innerTableDisplay);
          if (NS_STYLE_DISPLAY_TABLE == innerTableDisplay->mDisplay) { 
            // we were given the outer table frame, use the inner table frame
            aNewParentFrame=innerTableFrame;
          } // else we were already given the inner table frame
        } // else the current parent has no children and cannot be an outer table frame       
      } else { // else the child is a caption and really belongs to the outer table frame 
        nsIFrame* parFrame = nsnull;
        aCurrentParentFrame->GetParent(&parFrame);
        const nsStyleDisplay* parDisplay;
        aCurrentParentFrame->GetStyleData(eStyleStruct_Display, (const nsStyleStruct *&)parDisplay);
        if (NS_STYLE_DISPLAY_TABLE == parDisplay->mDisplay) {
          aNewParentFrame = parFrame; // aNewParentFrame was an inner frame
        }
      }
    }
  } else {
    rv = NS_ERROR_NULL_POINTER;
  }

  NS_POSTCONDITION(nsnull!=aNewParentFrame, "bad result null aNewParentFrame");
  return rv;
}

PRBool
nsCSSFrameConstructor::IsScrollable(nsIPresContext*       aPresContext,
                                    const nsStyleDisplay* aDisplay)
{
  // For the time being it's scrollable if the overflow property is auto or
  // scroll, regardless of whether the width or height is fixed in size
  if ((NS_STYLE_OVERFLOW_SCROLL == aDisplay->mOverflow) ||
      (NS_STYLE_OVERFLOW_AUTO == aDisplay->mOverflow)) {
    return PR_TRUE;
  }

  return PR_FALSE;
}


nsresult
nsCSSFrameConstructor::ResolveStyleContext(nsIPresContext*   aPresContext,
                                           nsIFrame*         aParentFrame,
                                           nsIContent*       aContent,
                                           nsIAtom*          aTag,
                                           nsIStyleContext** aStyleContext)
{
  nsresult rv = NS_OK;
  // Resolve the style context based on the content object and the parent
  // style context
  nsCOMPtr<nsIStyleContext> parentStyleContext;

  aParentFrame->GetStyleContext(getter_AddRefs(parentStyleContext));
  if (nsLayoutAtoms::textTagName == aTag) {
    // Use a special pseudo element style context for text
    nsCOMPtr<nsIContent> parentContent;
    if (nsnull != aParentFrame) {
      aParentFrame->GetContent(getter_AddRefs(parentContent));
    }
    rv = aPresContext->ResolvePseudoStyleContextFor(parentContent, 
                                                    nsHTMLAtoms::textPseudo, 
                                                    parentStyleContext,
                                                    PR_FALSE,
                                                    aStyleContext);
  } else if (nsLayoutAtoms::commentTagName == aTag) {
    // Use a special pseudo element style context for comments
    nsCOMPtr<nsIContent> parentContent;
    if (nsnull != aParentFrame) {
      aParentFrame->GetContent(getter_AddRefs(parentContent));
    }
    rv = aPresContext->ResolvePseudoStyleContextFor(parentContent, 
                                                    nsHTMLAtoms::commentPseudo, 
                                                    parentStyleContext,
                                                    PR_FALSE,
                                                    aStyleContext);
    } else if (nsLayoutAtoms::processingInstructionTagName == aTag) {
    // Use a special pseudo element style context for comments
    nsCOMPtr<nsIContent> parentContent;
    if (nsnull != aParentFrame) {
      aParentFrame->GetContent(getter_AddRefs(parentContent));
    }
    rv = aPresContext->ResolvePseudoStyleContextFor(parentContent, 
                                                    nsHTMLAtoms::processingInstructionPseudo, 
                                                    parentStyleContext,
                                                    PR_FALSE,
                                                    aStyleContext);
  } else {
    rv = aPresContext->ResolveStyleContextFor(aContent, parentStyleContext,
                                              PR_FALSE,
                                              aStyleContext);
  }
  return rv;
}

// MathML Mod - RBS
#ifdef INCLUDE_MATHML
nsresult
nsCSSFrameConstructor::ConstructMathMLFrame(nsIPresContext*          aPresContext,
                                            nsFrameConstructorState& aState,
                                            nsIContent*              aContent,
                                            nsIFrame*                aParentFrame,
                                            nsIAtom*                 aTag,
                                            nsIStyleContext*         aStyleContext,
                                            nsFrameItems&            aFrameItems)
{
  PRBool    processChildren = PR_TRUE;  // Whether we should process child content.
                                        // MathML frames are inline frames.
                                        // processChildren = PR_TRUE for inline frames.
                                        // see case NS_STYLE_DISPLAY_INLINE in
                                        // ConstructFrameByDisplayType()
  
  nsresult  rv = NS_OK;
  PRBool    isAbsolutelyPositioned = PR_FALSE;
  PRBool    isFixedPositioned = PR_FALSE;
  PRBool    isReplaced = PR_FALSE;

  NS_ASSERTION(aTag != nsnull, "null MathML tag");
  if (aTag == nsnull)
    return NS_OK;

  // Make sure that we remain confined in the MathML world
  PRInt32 nameSpaceID;
  rv = aContent->GetNameSpaceID(nameSpaceID);
  if (NS_FAILED(rv) || nameSpaceID != nsMathMLAtoms::nameSpaceID)
    return NS_OK;
                
  // Initialize the new frame
  nsIFrame* newFrame = nsnull;
  nsIFrame* ignore = nsnull;
  nsMathMLmtableCreator mathTableCreator; // Used to make table views.
 
  // See if the element is absolute or fixed positioned
  const nsStylePosition* position = (const nsStylePosition*)
    aStyleContext->GetStyleData(eStyleStruct_Position);
  if (NS_STYLE_POSITION_ABSOLUTE == position->mPosition) {
    isAbsolutelyPositioned = PR_TRUE;
  }
  else if (NS_STYLE_POSITION_FIXED == position->mPosition) {
    isFixedPositioned = PR_TRUE;
  }

       if (aTag == nsMathMLAtoms::mi)
     rv = NS_NewMathMLmiFrame(&newFrame);
  else if (aTag == nsMathMLAtoms::mo)
     rv = NS_NewMathMLmoFrame(&newFrame);
  else if (aTag == nsMathMLAtoms::mfrac)
     rv = NS_NewMathMLmfracFrame(&newFrame);
  else if (aTag == nsMathMLAtoms::msup)
     rv = NS_NewMathMLmsupFrame(&newFrame);
  else if (aTag == nsMathMLAtoms::msub)
     rv = NS_NewMathMLmsubFrame(&newFrame);
  else if (aTag == nsMathMLAtoms::msubsup)
     rv = NS_NewMathMLmsubsupFrame(&newFrame);
  else if (aTag == nsMathMLAtoms::munder)
     rv = NS_NewMathMLmunderFrame(&newFrame);
  else if (aTag == nsMathMLAtoms::mover)
     rv = NS_NewMathMLmoverFrame(&newFrame);
  else if (aTag == nsMathMLAtoms::munderover)
     rv = NS_NewMathMLmunderoverFrame(&newFrame);
  else if (aTag == nsMathMLAtoms::mphantom)
     rv = NS_NewMathMLmphantomFrame(&newFrame);
  else if (aTag == nsMathMLAtoms::mpadded)
     rv = NS_NewMathMLmpaddedFrame(&newFrame);
  else if (aTag == nsMathMLAtoms::mfenced)
     rv = NS_NewMathMLmfencedFrame(&newFrame);
  else if (aTag == nsMathMLAtoms::mmultiscripts)
     rv = NS_NewMathMLmmultiscriptsFrame(&newFrame);
  else if (aTag == nsMathMLAtoms::mstyle)
     rv = NS_NewMathMLmstyleFrame(&newFrame);
  else if (aTag == nsMathMLAtoms::mrow   ||
           aTag == nsMathMLAtoms::mtext  ||
           aTag == nsMathMLAtoms::merror ||
           aTag == nsMathMLAtoms::ms     ||
           aTag == nsMathMLAtoms::mn )
     rv = NS_NewMathMLmrowFrame(&newFrame);
  // CONSTRUCTION of MTABLE elements
  else if (aTag == nsMathMLAtoms::mtable)  {
  // <mtable> is an inline-table, for the moment, we just do what  
  // <table> does, and wait until nsLineLayout::TreatFrameAsBlock
  // can handle NS_STYLE_DISPLAY_INLINE_TABLE.
      nsIFrame* geometricParent = aParentFrame;
      if (isAbsolutelyPositioned) {
        aParentFrame = aState.mAbsoluteItems.containingBlock;
      }
      else if (isFixedPositioned) {
        aParentFrame = aState.mFixedItems.containingBlock;
      }            
      rv = ConstructTableFrame(aPresContext, aState, aContent, geometricParent, aStyleContext,
                               newFrame, mathTableCreator);
      // Note: table construction function takes care of initializing the frame,
      // processing children, and setting the initial child list
      if (isAbsolutelyPositioned || isFixedPositioned) {
        nsIFrame* placeholderFrame;
        CreatePlaceholderFrameFor(aPresContext, aState.mFrameManager, aContent, newFrame,
                                  aStyleContext, aParentFrame, &placeholderFrame);
        // Add the positioned frame to its containing block's list of child frames
        if (isAbsolutelyPositioned) {
          aState.mAbsoluteItems.AddChild(newFrame);
        } else {
          aState.mFixedItems.AddChild(newFrame);
        }
        // Add the placeholder frame to the flow
        aFrameItems.AddChild(placeholderFrame);
      } else {
        // Add the table frame to the flow
        aFrameItems.AddChild(newFrame);
      }
      return rv; 
  }
  else if (aTag == nsMathMLAtoms::mtd) {
    nsIFrame* ignore2;
    rv = ConstructTableCellFrame(aPresContext, aState, aContent, aParentFrame, aStyleContext, 
                                 newFrame, ignore, ignore2, mathTableCreator);
    aFrameItems.AddChild(newFrame);
    return rv;
  }
  // End CONSTRUCTION of MTABLE elements 
  
  else {
     return rv;
  }
 
  // If we succeeded in creating a frame then initialize it, process its
  // children (if requested), and set the initial child list
  if (NS_SUCCEEDED(rv) && newFrame != nsnull) {
    // If the frame is a replaced element, then set the frame state bit
    if (isReplaced) {
      nsFrameState  state;
      newFrame->GetFrameState(&state);
      newFrame->SetFrameState(state | NS_FRAME_REPLACED_ELEMENT);
    }

    nsIFrame* geometricParent = isAbsolutelyPositioned
                              ? aState.mAbsoluteItems.containingBlock
                              : aParentFrame;
    newFrame->Init(*aPresContext, aContent, geometricParent, aStyleContext,
                   nsnull);

    // See if we need to create a view, e.g. the frame is absolutely positioned
    nsHTMLContainerFrame::CreateViewForFrame(*aPresContext, newFrame,
                                             aStyleContext, PR_FALSE);

    // Add the new frame to our list of frame items.
    aFrameItems.AddChild(newFrame);

    // Process the child content if requested
    nsFrameItems childItems;
    if (processChildren) {
      rv = ProcessChildren(aPresContext, aState, aContent, newFrame, PR_FALSE,
                           childItems, PR_FALSE);
    }

    // Set the frame's initial child list
    newFrame->SetInitialChildList(*aPresContext, nsnull, childItems.childList);
  
    // If the frame is absolutely positioned then create a placeholder frame
    if (isAbsolutelyPositioned || isFixedPositioned) {
      nsIFrame* placeholderFrame;

      CreatePlaceholderFrameFor(aPresContext, aState.mFrameManager, aContent, newFrame, 
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
#endif // INCLUDE_MATHML

nsresult
nsCSSFrameConstructor::ConstructFrame(nsIPresContext*          aPresContext,
                                      nsFrameConstructorState& aState,
                                      nsIContent*              aContent,
                                      nsIFrame*                aParentFrame,
                                      PRBool                   aHaveFirstLetterStyle,
                                      nsFrameItems&            aFrameItems)

{
  NS_PRECONDITION(nsnull != aParentFrame, "no parent frame");

  nsresult  rv;

  // Get the element's tag
  nsCOMPtr<nsIAtom>  tag;
  aContent->GetTag(*getter_AddRefs(tag));

  nsCOMPtr<nsIStyleContext> styleContext;
  rv = ResolveStyleContext(aPresContext, aParentFrame, aContent, tag, getter_AddRefs(styleContext));

  if (NS_SUCCEEDED(rv)) {
    // Pre-check for display "none" - if we find that, don't create
    // any frame at all
    const nsStyleDisplay* display = (const nsStyleDisplay*)
      styleContext->GetStyleData(eStyleStruct_Display);

    if (NS_STYLE_DISPLAY_NONE != display->mDisplay) {
      nsIFrame* lastChild = aFrameItems.lastChild;

      // Handle specific frame types
      rv = ConstructFrameByTag(aPresContext, aState, aContent, aParentFrame,
                               tag, styleContext, aFrameItems);

#ifdef INCLUDE_XUL
      // Failing to find a matching HTML frame, try creating a specialized
      // XUL frame. This is temporary, pending planned factoring of this
      // whole process into separate, pluggable steps.
      if (NS_SUCCEEDED(rv) && ((nsnull == aFrameItems.childList) ||
                             (lastChild == aFrameItems.lastChild))) {
        PRBool haltProcessing = PR_FALSE;
        rv = ConstructXULFrame(aPresContext, aState, aContent, aParentFrame,
                               tag, styleContext, aFrameItems, haltProcessing);
        if (haltProcessing) {
          return rv;
        }
      } 
#endif

// MathML Mod - RBS
#ifdef INCLUDE_MATHML
      if (NS_SUCCEEDED(rv) && ((nsnull == aFrameItems.childList) ||
                               (lastChild == aFrameItems.lastChild))) {
        rv = ConstructMathMLFrame(aPresContext, aState, aContent, aParentFrame,
                                  tag, styleContext, aFrameItems);
      }
#endif

      if (NS_SUCCEEDED(rv) && ((nsnull == aFrameItems.childList) ||
                               (lastChild == aFrameItems.lastChild))) {
        // When there is no explicit frame to create, assume it's a
        // container and let display style dictate the rest
        rv = ConstructFrameByDisplayType(aPresContext, aState, display, aContent,
                                         aParentFrame, styleContext,
                                         aHaveFirstLetterStyle, aFrameItems);
      }
    }
  }
  
  return rv;
}

NS_IMETHODIMP
nsCSSFrameConstructor::ReconstructDocElementHierarchy(nsIPresContext* aPresContext)
{
  nsresult rv = NS_OK;
  
  if (nsnull != mDocument) {
    nsCOMPtr<nsIContent> rootContent(dont_AddRef(mDocument->GetRootContent()));

    if (rootContent) {
      nsFrameConstructorState state(aPresContext, mFixedContainingBlock,
                                    nsnull, nsnull);
      nsIFrame* docElementFrame;
        
      // Get the frame that corresponds to the document element
      state.mFrameManager->GetPrimaryFrameFor(rootContent, &docElementFrame);

      // Clear the hash tables that map from content to frame and out-of-flow
      // frame to placeholder frame
      state.mFrameManager->ClearPrimaryFrameMap();
      state.mFrameManager->ClearPlaceholderFrameMap();

      if (docElementFrame) {
        nsIFrame* docParentFrame;
        docElementFrame->GetParent(&docParentFrame);

        if (docParentFrame) {
          // Remove the old document element hieararchy
          nsCOMPtr<nsIPresShell>  shell;
          aPresContext->GetShell(getter_AddRefs(shell));
          rv = state.mFrameManager->RemoveFrame(*aPresContext, *shell,
                                                docParentFrame, nsnull, 
                                                docElementFrame);
          // XXX Remove any existing fixed items...
          if (NS_SUCCEEDED(rv)) {
            nsIFrame*                 newChild;
            nsCOMPtr<nsIStyleContext> rootPseudoStyle;
          
            docParentFrame->GetStyleContext(getter_AddRefs(rootPseudoStyle));
            rv = ConstructDocElementFrame(aPresContext, state, rootContent,
                                          docParentFrame, rootPseudoStyle,
                                          newChild);

            if (NS_SUCCEEDED(rv)) {
              rv = state.mFrameManager->InsertFrames(*aPresContext, *shell,
                                                     docParentFrame, nsnull,
                                                     nsnull, newChild);

              // Tell the fixed containing block about its 'fixed' frames
              if (state.mFixedItems.childList) {
                state.mFrameManager->InsertFrames(*aPresContext, *shell,
                                       mFixedContainingBlock, nsLayoutAtoms::fixedList,
                                       nsnull, state.mFixedItems.childList);
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
    nsresult res = aContent->QueryInterface(nsCOMTypeInfo<nsIDOMHTMLSelectElement>::GetIID(),
                                                 (void**)getter_AddRefs(selectElement));
    if (NS_SUCCEEDED(res) && selectElement) {
      nsIComboboxControlFrame * comboboxFrame;
      res = frame->QueryInterface(nsCOMTypeInfo<nsIComboboxControlFrame>::GetIID(),
                                               (void**)&comboboxFrame);
      nsIFrame * listFrame;
      if (NS_SUCCEEDED(res) && comboboxFrame) {
        comboboxFrame->GetDropDown(&listFrame);
        if (nsnull != listFrame) {
          listFrame->FirstChild(nsnull, &frame);
        }
      } else {
        res = frame->QueryInterface(nsCOMTypeInfo<nsIListControlFrame>::GetIID(),
                                                 (void**)&listFrame);
        if (NS_SUCCEEDED(res) && listFrame) {
          frame->FirstChild(nsnull, &frame);
        } 
      }
    } else {
      // If the primary frame is a scroll frame, then get the scrolled frame.
      // That's the frame that gets the reflow command
      const nsStyleDisplay* display;
      frame->GetStyleData(eStyleStruct_Display, (const nsStyleStruct*&)display);

      if (display->IsBlockLevel() && IsScrollable(aPresContext, display)) {
        frame->FirstChild(nsnull, &frame);
      } 
      // if we get an outer table frame use its 1st child which is a table inner frame
      // if we get a table cell frame   use its 1st child which is an area frame
      else if ((NS_STYLE_DISPLAY_TABLE      == display->mDisplay) ||
               (NS_STYLE_DISPLAY_TABLE_CELL == display->mDisplay)) {
        frame->FirstChild(nsnull, &frame);                   
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
    const nsStylePosition* position;

    // Is it positioned?
    frame->GetStyleData(eStyleStruct_Position, (const nsStyleStruct*&)position);
    if ((position->mPosition == NS_STYLE_POSITION_ABSOLUTE) ||
        (position->mPosition == NS_STYLE_POSITION_RELATIVE)) {
      const nsStyleDisplay* display;
      
      // If it's a table then ignore it, because for the time being tables
      // are not containers for absolutely positioned child frames
      frame->GetStyleData(eStyleStruct_Display, (const nsStyleStruct*&)display);
      if (display->mDisplay != NS_STYLE_DISPLAY_TABLE) {
        nsIAtom*  frameType;
        frame->GetFrameType(&frameType);

        if (nsLayoutAtoms::scrollFrame == frameType) {
          // We want the scrolled frame, not the scroll frame
          nsIFrame* scrolledFrame;
          frame->FirstChild(nsnull, &scrolledFrame);
          NS_RELEASE(frameType);
          if (scrolledFrame) {
            scrolledFrame->GetFrameType(&frameType);
            if (nsLayoutAtoms::areaFrame == frameType) {
              containingBlock = scrolledFrame;
            }
          }

        } else if ((nsLayoutAtoms::areaFrame == frameType) ||
                   (nsLayoutAtoms::positionedInlineFrame == frameType)) {
          containingBlock = frame;
        }
        NS_RELEASE(frameType);
      }
    }

    // See if we found a containing block
    if (containingBlock) {
      break;
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
      const nsStylePosition* position;
      containingBlock->GetStyleData(eStyleStruct_Position,
                                    (const nsStyleStruct*&)position);

      if ((NS_STYLE_FLOAT_NONE != display->mDisplay) ||
          (position->IsAbsolutelyPositioned())) {
        break;
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

// This function is called by ContentAppended() and ContentInserted() when
// appending flowed frames to a parent's principal child list. It handles the
// case where the parent frame has :after pseudo-element generated content
nsresult
nsCSSFrameConstructor::AppendFrames(nsIPresContext*  aPresContext,
                                    nsIPresShell*    aPresShell,
                                    nsIFrameManager* aFrameManager,
                                    nsIContent*      aContainer,
                                    nsIFrame*        aParentFrame,
                                    nsIFrame*        aFrameList)
{
  nsIFrame* firstChild;
  aParentFrame->FirstChild(nsnull, &firstChild);
  nsFrameList frames(firstChild);
  nsIFrame* lastChild = frames.LastChild();

  // See if the parent has an :after pseudo-element
  if (lastChild && IsGeneratedContentFor(aContainer, lastChild, nsCSSAtoms::afterPseudo)) {
    // Insert the frames before the :after pseudo-element
    return aFrameManager->InsertFrames(*aPresContext, *aPresShell, aParentFrame,
                                       nsnull, frames.GetPrevSiblingFor(lastChild),
                                       aFrameList);
  }

  // Append the frames to the end of the parent's child list
  return aFrameManager->AppendFrames(*aPresContext, *aPresShell, aParentFrame,
                                     nsnull, aFrameList);
}

// See if aContent is the "logical" first child of
// aContainingBlock. The check works recursively, skipping upward
// through content until we reach aContainingBlock.
static PRBool
ContentIsLogicalFirstChild(nsIContent* aContent, nsIContent* aContainingBlock)
{
  if (aContent) {
    nsCOMPtr<nsIContent> contentParent;
    aContent->GetParent(*getter_AddRefs(contentParent));
    if (contentParent) {
      PRInt32 ix;
      contentParent->IndexOf(aContent, ix);
      if (0 != ix) {
        return PR_FALSE;
      }
      if (contentParent.get() != aContainingBlock) {
        return ContentIsLogicalFirstChild(contentParent, aContainingBlock);
      }
      return PR_TRUE;
    }
  }
  return PR_FALSE;
}

nsresult
nsCSSFrameConstructor::MaybeCreateContainerFrame(nsIPresContext* aPresContext,
                                                 nsIContent* aContainer)
{
  nsresult rv = NS_OK;
#if 0
  nsCOMPtr<nsIPresShell> shell;
  aPresContext->GetShell(getter_AddRefs(shell));

  // See if aContainer's parent has a frame...
  nsCOMPtr<nsIContent> containerParent;
  aContainer->GetParent(*getter_AddRefs(containerParent));
  if (containerParent) {
    nsIFrame* parentFrame = GetFrameFor(shell, aPresContext, containerParent);
    if (parentFrame) {
      // aContainer's parent has a frame. Maybe aContainer needs a
      // frame now? If it's display none, then it doesn't.
      nsCOMPtr<nsIStyleContext> parentStyleContext;
      parentFrame->GetStyleContext(getter_AddRefs(parentStyleContext));
      nsCOMPtr<nsIStyleContext> styleContext;
      aPresContext->ResolveStyleContextFor(aContainer, parentStyleContext,
                                           PR_FALSE,
                                           getter_AddRefs(styleContext));
      const nsStyleDisplay* display = (const nsStyleDisplay*) 
        styleContext->GetStyleData(eStyleStruct_Display);
      if (NS_STYLE_DISPLAY_NONE != display->mDisplay) {
        // aContainer's display is not none and it has a parent
        // frame. Now we see if it needs a frame after new content
        // was appended.
        if (IsHTMLParagraph(aContainer)) {
          if (!IsEmptyContainer(aContainer)) {
            // It's an HTML paragraph (P) and it has some
            // content. Because of the previous checks we now know
            // that the P used to have nothing but empty content and
            // therefore had no frame created. This condition is no
            // longer true so we create a frame for aContainer (and
            // its children) and insert them into the tree...
            PRInt32 ix;
            containerParent->IndexOf(aContainer, ix);
            if (ix >= 0) {
#ifdef DEBUG_kipp
              printf("Recreating frame for HTML:P\n");
#endif
              ContentInserted(aPresContext, containerParent, aContainer, ix);
            }
          }
        }
      }
    }
    else {
      // If aContainer's parent has no frame then there is no need
      // in trying to create a frame for aContainer.
    }
  }
  else {
    // aContainer has no parent. Odd.
  }
#endif
  return rv;
}

NS_IMETHODIMP
nsCSSFrameConstructor::ContentAppended(nsIPresContext* aPresContext,
                                       nsIContent*     aContainer,
                                       PRInt32         aNewIndexInContainer)
{
  nsCOMPtr<nsIPresShell> shell;
  aPresContext->GetShell(getter_AddRefs(shell));

#ifdef INCLUDE_XUL
  if (aContainer) {
    nsCOMPtr<nsIAtom> tag;
    aContainer->GetTag(*getter_AddRefs(tag));
    if (tag && (tag.get() == nsXULAtoms::treechildren ||
        tag.get() == nsXULAtoms::treeitem)) {
      // Walk up to the outermost tree row group frame and tell it that
      // content was added.
      nsCOMPtr<nsIContent> parent;
      nsCOMPtr<nsIContent> child = dont_QueryInterface(aContainer);
      child->GetParent(*getter_AddRefs(parent));
      while (parent) {
        parent->GetTag(*getter_AddRefs(tag));
        if (tag.get() == nsXULAtoms::tree)
          break;
        child = parent;
        child->GetParent(*getter_AddRefs(parent));
      }

      if (parent) {
        // We found it.  Get the primary frame.
        nsIFrame* parentFrame = GetFrameFor(shell, aPresContext, child);

        // Convert to a tree row group frame.
        nsTreeRowGroupFrame* treeRowGroup = (nsTreeRowGroupFrame*)parentFrame;
        if (treeRowGroup && treeRowGroup->IsLazy()) {
          treeRowGroup->OnContentAdded(*aPresContext);
          return NS_OK;
        }
      }
    }
  }
#endif // INCLUDE_XUL

  // Get the frame associated with the content
  nsIFrame* parentFrame = GetFrameFor(shell, aPresContext, aContainer);
  if (nsnull != parentFrame) {
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

    // Create some new frames
    PRInt32                 count;
    nsIFrame*               firstAppendedFrame = nsnull;
    nsFrameItems            frameItems;
    nsFrameConstructorState state(aPresContext, mFixedContainingBlock,
                                  GetAbsoluteContainingBlock(aPresContext, parentFrame),
                                  GetFloaterContainingBlock(aPresContext, parentFrame));

    // See if the containing block has :first-letter style applied.
    nsIFrame* containingBlock = state.mFloatedItems.containingBlock;
    nsCOMPtr<nsIStyleContext> blockSC;
    containingBlock->GetStyleContext(getter_AddRefs(blockSC));
    nsCOMPtr<nsIContent> blockContent;
    containingBlock->GetContent(getter_AddRefs(blockContent));
    PRBool haveFirstLetterStyle =
      HaveFirstLetterStyle(aPresContext, blockContent, blockSC);
    PRBool haveFirstLineStyle = PR_FALSE;
    PRBool containerIsBlock = containingBlock == parentFrame;
    PRBool whitespaceDoesntMatter = PR_FALSE;
    PRBool okToSkip = PR_FALSE;
    if (containerIsBlock) {
      // See if the container has :first-line style too
      haveFirstLineStyle =
        HaveFirstLineStyle(aPresContext, blockContent, blockSC);

      // Setup the whitespaceDoesntMatter flag so that we know whether
      // or not we can skip over empty text frames.
      const nsStyleText* st;
      parentFrame->GetStyleData(eStyleStruct_Text, (const nsStyleStruct*&) st);
      whitespaceDoesntMatter = !st->WhiteSpaceIsSignificant();
      if (whitespaceDoesntMatter) {
        // If the current last child isa block element then we can
        // skip over empty text content.

        // XXX Performance: this is being done twice because
        // AppendFrames is doing it too.
        nsIFrame* firstChild;
        parentFrame->FirstChild(nsnull, &firstChild);
        nsFrameList frames(firstChild);
        nsIFrame* lastChild = frames.LastChild();
        if (lastChild) {
          const nsStyleDisplay* sd;
          lastChild->GetStyleData(eStyleStruct_Display,
                                  (const nsStyleStruct*&) sd);
          if (sd->IsBlockLevel()) {
            okToSkip = PR_TRUE;
          }
        }
        else {
          okToSkip = PR_TRUE;
        }
      }
    }

    PRInt32 i;
    aContainer->ChildCount(count);
    for (i = aNewIndexInContainer; i < count; i++) {
      nsCOMPtr<nsIContent> childContent;
      aContainer->ChildAt(i, *getter_AddRefs(childContent));

      // Block containers sometimes don't create frames for the kids
      if (containerIsBlock) {
#if 0
        if (okToSkip && IsEmptyTextContent(childContent)) {
          // Don't create a frame for useless compressible whitespace
          continue;
        }
#endif
#if 0
        if (whitespaceDoesntMatter && IsEmptyHTMLParagraph(childContent)) {
          // Don't create a frame for empty html paragraph's
          continue;
        }
#endif
      }

      // Construct a child frame
      okToSkip = PR_FALSE;
      ConstructFrame(aPresContext, state, childContent, parentFrame,
                     PR_FALSE, frameItems);

      // If last frame was a blockish type of frame, then its ok to
      // skip the next empty text content.
      if (containerIsBlock && whitespaceDoesntMatter && frameItems.lastChild) {
        const nsStyleDisplay* sd;
        frameItems.lastChild->GetStyleData(eStyleStruct_Display,
                                           (const nsStyleStruct*&) sd);
        if (sd->IsBlockLevel()) {
          okToSkip = PR_TRUE;
        }
      }

      // See if we just created a frame in a :first-letter situation
      if (haveFirstLetterStyle &&
          (0 == aNewIndexInContainer) &&
          (frameItems.childList == frameItems.lastChild)) {
        MakeFirstLetterFrame(aPresContext, state, aContainer, childContent,
                             blockContent, parentFrame, frameItems);
      }
    }

    if (haveFirstLineStyle) {
      // It's possible that some of the new frames go into a
      // first-line frame. Look at them and see...
      AppendFirstLineFrames(aPresContext, state, aContainer, parentFrame,
                            frameItems); 
    }

    // Adjust parent frame for table inner/outer frame.  We need to do
    // this here because we need both the parent frame and the
    // constructed frame
    nsresult result = NS_OK;
    nsIFrame* adjustedParentFrame = parentFrame;
    firstAppendedFrame = frameItems.childList;
    if (nsnull != firstAppendedFrame) {
      const nsStyleDisplay* firstAppendedFrameDisplay;
      firstAppendedFrame->GetStyleData(eStyleStruct_Display,
                                       (const nsStyleStruct *&)firstAppendedFrameDisplay);
      result = GetAdjustedParentFrame(parentFrame,
                                      firstAppendedFrameDisplay->mDisplay,
                                      adjustedParentFrame);
    }

    // Notify the parent frame passing it the list of new frames
    if (NS_SUCCEEDED(result) && firstAppendedFrame) {
      // Append the flowed frames to the principal child list
      AppendFrames(aPresContext, shell, state.mFrameManager, aContainer,
                   adjustedParentFrame, firstAppendedFrame);

      // If there are new absolutely positioned child frames, then notify
      // the parent
      // XXX We can't just assume these frames are being appended, we need to
      // determine where in the list they should be inserted...
      if (state.mAbsoluteItems.childList) {
        state.mAbsoluteItems.containingBlock->AppendFrames(*aPresContext, *shell,
                                                    nsLayoutAtoms::absoluteList,
                                                    state.mAbsoluteItems.childList);
      }

      // If there are new fixed positioned child frames, then notify
      // the parent
      // XXX We can't just assume these frames are being appended, we need to
      // determine where in the list they should be inserted...
      if (state.mFixedItems.childList) {
        state.mFixedItems.containingBlock->AppendFrames(*aPresContext, *shell,
                                                 nsLayoutAtoms::fixedList,
                                                 state.mFixedItems.childList);
      }

      // If there are new floating child frames, then notify
      // the parent
      // XXX We can't just assume these frames are being appended, we need to
      // determine where in the list they should be inserted...
      if (state.mFloatedItems.childList) {
        state.mFloatedItems.containingBlock->AppendFrames(*aPresContext, *shell,
                                                  nsLayoutAtoms::floaterList,
                                                  state.mFloatedItems.childList);
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

  }
  else {
    // Since the parentFrame wasn't found, aContainer didn't have a
    // frame created for it. Maybe now that it has a new child it
    // should get a frame...
    MaybeCreateContainerFrame(aPresContext, aContainer);
  }

  return NS_OK;
}

nsresult
nsCSSFrameConstructor::MakeFirstLetterFrame(nsIPresContext* aPresContext,
                                            nsFrameConstructorState& aState,
                                            nsIContent* aContainer,
                                            nsIContent* aChild,
                                            nsIContent* aContainingBlock,
                                            nsIFrame* aParentFrame,
                                            nsFrameItems& aFrameItems)
{
  if (aFrameItems.lastChild) {
    // We have two cases where we need to make sure that a
    // :first-letter frame is created.
    nsCOMPtr<nsIStyleContext> parentFrameSC;
    aParentFrame->GetStyleContext(getter_AddRefs(parentFrameSC));
    const nsStyleDisplay* display = (const nsStyleDisplay*)
      parentFrameSC->GetStyleData(eStyleStruct_Display);
    PRBool isBlock =
      (display->mDisplay == NS_STYLE_DISPLAY_BLOCK) ||
      (display->mDisplay == NS_STYLE_DISPLAY_LIST_ITEM);

    // First case: a direct child of a block frame that is its
    // first child.
    if (isBlock &&
        ShouldCreateFirstLetterFrame(aPresContext, aChild,
                                     aFrameItems.lastChild)) {
      WrapTextFrame(aPresContext, aState, aFrameItems.childList,
                    aContainer, aChild, aParentFrame, aFrameItems,
                    aState.mFloatedItems, isBlock);
    }
    // Second case: a direct child of an inline frame, where the
    // inline frame is the first child of the block (or its parent
    // inline frame is the first child of the block, recursively
    // true upward).
    else if ((display->mDisplay == NS_STYLE_DISPLAY_INLINE) &&
             ContentIsLogicalFirstChild(aChild, aContainingBlock) &&
             ShouldCreateFirstLetterFrame(aPresContext, aChild,
                                          aFrameItems.lastChild)) {
      WrapTextFrame(aPresContext, aState, aFrameItems.childList,
                    aContainer, aChild, aParentFrame, aFrameItems,
                    aState.mFloatedItems, isBlock);
    }
  }

  return NS_OK;
}

static nsIFrame*
FindPreviousSibling(nsIPresShell* aPresShell,
                    nsIContent*   aContainer,
                    PRInt32       aIndexInContainer)
{
  nsIFrame* prevSibling = nsnull;

  // Note: not all content objects are associated with a frame (e.g., if their
  // 'display' type is 'hidden') so keep looking until we find a previous frame
  for (PRInt32 i = aIndexInContainer - 1; i >= 0; i--) {
    nsCOMPtr<nsIContent> precedingContent;

    aContainer->ChildAt(i, *getter_AddRefs(precedingContent));
    aPresShell->GetPrimaryFrameFor(precedingContent, &prevSibling);

    if (nsnull != prevSibling) {
      // The frame may have a next-in-flow. Get the last-in-flow
      nsIFrame* nextInFlow;
      do {
        prevSibling->GetNextInFlow(&nextInFlow);
        if (nsnull != nextInFlow) {
          prevSibling = nextInFlow;
        }
      } while (nsnull != nextInFlow);

      // Did we really get the *right* frame?
      const nsStyleDisplay* display;
      prevSibling->GetStyleData(eStyleStruct_Display,
                                (const nsStyleStruct*&)display);
      if (display->IsFloating()) {
        // Nope. Get the place-holder instead
        nsIFrame* placeholderFrame;
        aPresShell->GetPlaceholderFrameFor(prevSibling, &placeholderFrame);
        NS_ASSERTION(nsnull != placeholderFrame, "yikes");
        prevSibling = placeholderFrame;
      }

      break;
    }
  }

  return prevSibling;
}

static nsIFrame*
FindNextSibling(nsIPresShell* aPresShell,
                nsIContent*   aContainer,
                PRInt32       aIndexInContainer)
{
  nsIFrame* nextSibling = nsnull;

  // Note: not all content objects are associated with a frame (e.g., if their
  // 'display' type is 'hidden') so keep looking until we find a previous frame
  PRInt32 count;
  aContainer->ChildCount(count);
  for (PRInt32 i = aIndexInContainer + 1; i < count; i++) {
    nsCOMPtr<nsIContent> nextContent;

    aContainer->ChildAt(i, *getter_AddRefs(nextContent));
    aPresShell->GetPrimaryFrameFor(nextContent, &nextSibling);

    if (nsnull != nextSibling) {
      // The frame may have a next-in-flow. Get the first-in-flow
      nsIFrame* prevInFlow;
      do {
        nextSibling->GetPrevInFlow(&prevInFlow);
        if (nsnull != prevInFlow) {
          nextSibling = prevInFlow;
        }
      } while (nsnull != prevInFlow);

      // Did we really get the *right* frame?
      const nsStyleDisplay* display;
      nextSibling->GetStyleData(eStyleStruct_Display,
                                (const nsStyleStruct*&)display);
      if (display->IsFloating()) {
        // Nope. Get the place-holder instead
        nsIFrame* placeholderFrame;
        aPresShell->GetPlaceholderFrameFor(nextSibling, &placeholderFrame);
        NS_ASSERTION(nsnull != placeholderFrame, "yikes");
        nextSibling = placeholderFrame;
      }

      break;
    }
  }

  return nextSibling;
}

nsresult
nsCSSFrameConstructor::RemoveDummyFrameFromSelect(nsIPresContext* aPresContext,
                                                  nsIPresShell *  aPresShell,
                                                  nsIContent*     aContainer,
                                                  nsIContent*     aChild,
                                                  nsIDOMHTMLSelectElement * aSelectElement)
{
  //check to see if there is one item,
  // meaning we need to remove the dummy frame
  PRUint32 numOptions = 0;
  nsresult result = aSelectElement->GetLength(&numOptions);
  if (NS_SUCCEEDED(result)) {
    if (1 == numOptions) { 
      nsIFrame* parentFrame;
      nsIFrame* childFrame;
      // Get the childFrame for the added child (option)
      // then get the child's parent frame which should be an area frame
      aPresShell->GetPrimaryFrameFor(aChild, &childFrame);
      childFrame->GetParent(&parentFrame);

      // Now loop through all the child looking fr the frame whose content 
      // is equal to the select element's content
      // this is because when gernated content is created it stuff the parent content
      // pointer into the generated frame, so in this case it has the select content
      parentFrame->FirstChild(nsnull, &childFrame);
      nsCOMPtr<nsIContent> selectContent = do_QueryInterface(aSelectElement);
      while (nsnull != childFrame) {
        nsIContent * content;
        childFrame->GetContent(&content);
      
        // Found the dummy frame so get the FrameManager and 
        // delete/remove the dummy frame
        if (selectContent.get() == content) {
          nsCOMPtr<nsIFrameManager> frameManager;
          aPresShell->GetFrameManager(getter_AddRefs(frameManager));
          frameManager->RemoveFrame(*aPresContext, *aPresShell, parentFrame, nsnull, childFrame);
          NS_IF_RELEASE(content);
          return NS_OK;
        }

        NS_IF_RELEASE(content);
        childFrame->GetNextSibling(&childFrame);
      }
    }
  }

  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsCSSFrameConstructor::ContentInserted(nsIPresContext* aPresContext,
                                       nsIContent*     aContainer,
                                       nsIContent*     aChild,
                                       PRInt32         aIndexInContainer)
{
#ifdef INCLUDE_XUL
  if (aContainer) {
    nsCOMPtr<nsIAtom> tag;
    aContainer->GetTag(*getter_AddRefs(tag));
    if (tag && (tag.get() == nsXULAtoms::treechildren ||
        tag.get() == nsXULAtoms::treeitem)) {
      // Walk up to the outermost tree row group frame and tell it that
      // content was added.
      nsCOMPtr<nsIContent> parent;
      nsCOMPtr<nsIContent> child = dont_QueryInterface(aContainer);
      child->GetParent(*getter_AddRefs(parent));
      while (parent) {
        parent->GetTag(*getter_AddRefs(tag));
        if (tag.get() == nsXULAtoms::tree)
          break;
        child = parent;
        child->GetParent(*getter_AddRefs(parent));
      }

      if (parent) {
        // We found it.  Get the primary frame.
        nsCOMPtr<nsIPresShell> shell;
        aPresContext->GetShell(getter_AddRefs(shell));
        nsIFrame*     parentFrame = GetFrameFor(shell, aPresContext, child);

        // Convert to a tree row group frame.
        nsTreeRowGroupFrame* treeRowGroup = (nsTreeRowGroupFrame*)parentFrame;
        if (treeRowGroup && treeRowGroup->IsLazy()) {
          nsIFrame* nextSibling = FindNextSibling(shell, aContainer, aIndexInContainer);
		  if(!nextSibling)
            treeRowGroup->OnContentAdded(*aPresContext);
		  else {
            nsIFrame*     frame = GetFrameFor(shell, aPresContext, aContainer);
            nsTreeRowGroupFrame* frameTreeRowGroup = (nsTreeRowGroupFrame*)frame;
			if(frameTreeRowGroup)
              frameTreeRowGroup->OnContentInserted(*aPresContext, nextSibling);
		  }
          return NS_OK;
        }
      }
    }
  }
#endif // INCLUDE_XUL

  nsCOMPtr<nsIPresShell> shell;
  aPresContext->GetShell(getter_AddRefs(shell));
  nsresult rv = NS_OK;

  // If we have a null parent, then this must be the document element
  // being inserted
  if (nsnull == aContainer) {
    NS_PRECONDITION(nsnull == mInitialContainingBlock, "initial containing block already created");
    nsIContent* docElement = mDocument->GetRootContent();

    // Get the style context of the containing block frame
    nsCOMPtr<nsIStyleContext> containerStyle;
    mDocElementContainingBlock->GetStyleContext(getter_AddRefs(containerStyle));
    
    // Create frames for the document element and its child elements
    nsIFrame*               docElementFrame;
    nsFrameConstructorState state(aPresContext, mFixedContainingBlock, nsnull, nsnull);
    ConstructDocElementFrame(aPresContext, 
                             state,
                             docElement, 
                             mDocElementContainingBlock,
                             containerStyle, 
                             docElementFrame);
    NS_IF_RELEASE(docElement);
    
    // Set the initial child list for the parent
    mDocElementContainingBlock->SetInitialChildList(*aPresContext, 
                                                    nsnull, 
                                                    docElementFrame);
    
    // Tell the fixed containing block about its 'fixed' frames
    if (state.mFixedItems.childList) {
      mFixedContainingBlock->SetInitialChildList(*aPresContext, 
                                                 nsLayoutAtoms::fixedList,
                                                 state.mFixedItems.childList);
    }

  } else {
    // Find the frame that precedes the insertion point.
    nsIFrame* prevSibling = FindPreviousSibling(shell, aContainer, aIndexInContainer);
    nsIFrame* nextSibling = nsnull;
    PRBool    isAppend = PR_FALSE;
    
    // If there is no previous sibling, then find the frame that follows
    if (nsnull == prevSibling) {
      nextSibling = FindNextSibling(shell, aContainer, aIndexInContainer);
    }

    // Get the geometric parent. Use the prev sibling if we have it;
    // otherwise use the next sibling
    nsIFrame* parentFrame;
    if (nsnull != prevSibling) {
      prevSibling->GetParent(&parentFrame);
    }
    else if (nextSibling) {
      nextSibling->GetParent(&parentFrame);
    }
    else {
      // No previous or next sibling so treat this like an appended frame.
      isAppend = PR_TRUE;
      shell->GetPrimaryFrameFor(aContainer, &parentFrame);
      
      if (parentFrame) {
        // If we didn't process children when we originally created the frame,
        // then don't do any processing now
        nsCOMPtr<nsIAtom>  frameType;
        parentFrame->GetFrameType(getter_AddRefs(frameType));
        if (frameType.get() == nsLayoutAtoms::objectFrame) {
          // This handles APPLET, EMBED, and OBJECT
          return NS_OK;
        }
      }
    }

    // Construct a new frame
    if (nsnull != parentFrame) {
      nsFrameItems            frameItems;
      nsFrameConstructorState state(aPresContext, mFixedContainingBlock,
                                    GetAbsoluteContainingBlock(aPresContext, parentFrame),
                                    GetFloaterContainingBlock(aPresContext, parentFrame));

      PRBool needFixup = PR_FALSE;
      nsIFrame* containingBlock = state.mFloatedItems.containingBlock;
      nsCOMPtr<nsIStyleContext> blockSC;
      nsCOMPtr<nsIContent> blockContent;
      PRBool haveFirstLetterStyle = PR_FALSE;
      PRBool haveFirstLineStyle = PR_FALSE;
      const nsStyleDisplay* parentDisplay;
      parentFrame->GetStyleData(eStyleStruct_Display,
                                (const nsStyleStruct*&)parentDisplay);

      // Examine the parentFrame where the insertion is taking
      // place. If its a certain kind of container then some special
      // processing is done.
      if ((NS_STYLE_DISPLAY_BLOCK == parentDisplay->mDisplay) ||
          (NS_STYLE_DISPLAY_INLINE == parentDisplay->mDisplay) ||
          (NS_STYLE_DISPLAY_INLINE_BLOCK == parentDisplay->mDisplay)) {
        // Dig up the special style flags
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
            if (prevSibling) {
              prevSibling = parentFrame;
            }
            parentFrame->GetParent(&parentFrame);
          }

          if (0 == aIndexInContainer) {
            // See if after we construct the new first child if we will
            // need to fixup the second child's frames.
            state.mFirstLetterStyle = PR_TRUE;
            if ((parentFrame == containingBlock) ||
                ContentIsLogicalFirstChild(aChild, blockContent)) {
              needFixup = PR_TRUE;
            }
          }
        }
      }

      rv = ConstructFrame(aPresContext, state, aChild, parentFrame, PR_FALSE,
                          frameItems);

      // See if we just created a frame in a :first-letter situation
      if (haveFirstLetterStyle && (0 == aIndexInContainer)) {
        MakeFirstLetterFrame(aPresContext, state, aContainer, aChild,
                             blockContent, parentFrame, frameItems);
      }

      if (haveFirstLineStyle) {
        // It's possible that the new frame goes into a first-line
        // frame. Look at it and see...
        if (isAppend) {
          // Use append logic when appending
          AppendFirstLineFrames(aPresContext, state, aContainer, parentFrame,
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
      if (NS_SUCCEEDED(rv) && (nsnull != newFrame)) {
        // Notify the parent frame
        if (isAppend) {
          rv = AppendFrames(aPresContext, shell, state.mFrameManager,
                            aContainer, parentFrame, newFrame);
        } else {
          if (!prevSibling) {
            // We're inserting the new frame as the first child. See if the
            // parent has a :before pseudo-element
            nsIFrame* firstChild;
            parentFrame->FirstChild(nsnull, &firstChild);

            if (firstChild && IsGeneratedContentFor(aContainer, firstChild, nsCSSAtoms::beforePseudo)) {
              // Insert the new frames after the :before pseudo-element
              prevSibling = firstChild;
            }
          }

          rv = state.mFrameManager->InsertFrames(*aPresContext, *shell,
                                                 parentFrame,
                                                 nsnull, prevSibling,
                                                 newFrame);
        }
        
        // If there are new absolutely positioned child frames, then notify
        // the parent
        // XXX We can't just assume these frames are being appended, we need to
        // determine where in the list they should be inserted...
        if (state.mAbsoluteItems.childList) {
          rv = state.mAbsoluteItems.containingBlock->AppendFrames(*aPresContext, *shell,
                                                           nsLayoutAtoms::absoluteList,
                                                           state.mAbsoluteItems.childList);
        }
        
        // If there are new fixed positioned child frames, then notify
        // the parent
        // XXX We can't just assume these frames are being appended, we need to
        // determine where in the list they should be inserted...
        if (state.mFixedItems.childList) {
          rv = state.mFixedItems.containingBlock->AppendFrames(*aPresContext, *shell,
                                                        nsLayoutAtoms::fixedList,
                                                        state.mFixedItems.childList);
        }
        
        // If there are new floating child frames, then notify
        // the parent
        // XXX We can't just assume these frames are being appended, we need to
        // determine where in the list they should be inserted...
        if (state.mFloatedItems.childList) {
          rv = state.mFloatedItems.containingBlock->AppendFrames(*aPresContext, *shell,
                                                      nsLayoutAtoms::floaterList,
                                                      state.mFloatedItems.childList);
        }

        if (needFixup) {
          // Now that we have a new first child of the block (either
          // directly, or a "logical" one) we need to reformat the
          // second child because its no longer in a first-letter
          // situation.
          nsCOMPtr<nsIContent> child1;
          aContainer->ChildAt(1, *getter_AddRefs(child1));
          if (child1) {
            // Replace the content with itself; this time it won't
            // get the special first-letter frame wrapped around it.
            rv = ContentReplaced(aPresContext, aContainer,
                                 child1, child1, 1);
          }
        }
      }
    }
    else {
      // Since the parentFrame wasn't found, aContainer didn't have a
      // frame created for it. Maybe now that it has a new child it
      // should get a frame...
      MaybeCreateContainerFrame(aPresContext, aContainer);
    }

    // Here we have been notified that content has been insert
    // so if the select now has a single item 
    // we need to go in and removed the dummy frame
    nsCOMPtr<nsIDOMHTMLSelectElement> selectElement;
    nsresult result = aContainer->QueryInterface(nsCOMTypeInfo<nsIDOMHTMLSelectElement>::GetIID(),
                                                 (void**)getter_AddRefs(selectElement));
    if (NS_SUCCEEDED(result) && selectElement) {
      RemoveDummyFrameFromSelect(aPresContext, shell, aContainer, aChild, selectElement);
    } 

  }

  return rv;
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
                                aOldChild, aIndexInContainer);
  if (NS_OK == res) {
    res = ContentInserted(aPresContext, aContainer, 
                          aNewChild, aIndexInContainer);
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
 * Called to delete a frame subtree. Two important things happen:
 * 1. for each frame in the subtree we remove the mapping from the
 *    content object to its frame
 * 2. for child frames that have been moved out of the flow we delete
 *    the out-of-flow frame as well
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
DeletingFrameSubtree(nsIPresContext*  aPresContext,
                     nsIPresShell*    aPresShell,
                     nsIFrameManager* aFrameManager,
                     nsIFrame*        aRemovedFrame,
                     nsIFrame*        aFrame)
{
  // If there's no frame manager it's probably because the pres shell is
  // being destroyed
  if (aFrameManager) {
    // Remove the mapping from the content object to its frame
    nsCOMPtr<nsIContent> content;
    aFrame->GetContent(getter_AddRefs(content));
    aFrameManager->SetPrimaryFrameFor(content, nsnull);
    
    // Recursively walk aFrame's child frames looking for placeholder frames
    nsIFrame* childFrame;
    aFrame->FirstChild(nsnull, &childFrame);
    while (childFrame) {
      nsIAtom*  frameType;
      PRBool    isPlaceholder;
  
      // See if it's a placeholder frame
      childFrame->GetFrameType(&frameType);
      isPlaceholder = (nsLayoutAtoms::placeholderFrame == frameType);
      NS_IF_RELEASE(frameType);
  
      if (isPlaceholder) {
        // Get the out-of-flow frame
        nsIFrame* outOfFlowFrame = ((nsPlaceholderFrame*)childFrame)->GetOutOfFlowFrame();
        NS_ASSERTION(outOfFlowFrame, "no out-of-flow frame");
  
        // Remove the mapping from the out-of-flow frame to its placeholder
        aFrameManager->SetPlaceholderFrameFor(outOfFlowFrame, nsnull);
        
        // Recursively find and delete any of its out-of-flow frames, and remove
        // the mapping from content objects to frames
        DeletingFrameSubtree(aPresContext, aPresShell, aFrameManager, aRemovedFrame,
                             outOfFlowFrame);
        
        // Don't delete the out-of-flow frame if aRemovedFrame is one of its
        // ancestor frames, because when aRemovedFrame is deleted it will delete
        // its child frames including this out-of-flow frame
        if (!IsAncestorFrame(outOfFlowFrame, aRemovedFrame)) {
          // Get the out-of-flow frame's parent
          nsIFrame* parentFrame;
          outOfFlowFrame->GetParent(&parentFrame);
  
          // Get the child list name for the out-of-flow frame
          nsIAtom* listName;
          GetChildListNameFor(parentFrame, outOfFlowFrame, &listName);
  
          // Ask the parent to delete the out-of-flow frame
          aFrameManager->RemoveFrame(*aPresContext, *aPresShell, parentFrame,
                                     listName, outOfFlowFrame);
          NS_IF_RELEASE(listName);
        }
  
      } else {
        // Recursively find and delete any of its out-of-flow frames, and remove
        // the mapping from content objects to frames
        DeletingFrameSubtree(aPresContext, aPresShell, aFrameManager, aRemovedFrame,
                             childFrame);
      }
  
      // Get the next sibling child frame
      childFrame->GetNextSibling(&childFrame);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsCSSFrameConstructor::RemoveMappingsForFrameSubtree(nsIPresContext* aPresContext,
                              nsIFrame* aRemovedFrame)
{
  nsCOMPtr<nsIPresShell> presShell;
  aPresContext->GetShell(getter_AddRefs(presShell));
  nsCOMPtr<nsIFrameManager> frameManager;
  presShell->GetFrameManager(getter_AddRefs(frameManager));
  return DeletingFrameSubtree(aPresContext, presShell, frameManager, aRemovedFrame,
                              aRemovedFrame);
}

NS_IMETHODIMP
nsCSSFrameConstructor::ContentRemoved(nsIPresContext* aPresContext,
                                      nsIContent*     aContainer,
                                      nsIContent*     aChild,
                                      PRInt32         aIndexInContainer)
{
  nsCOMPtr<nsIPresShell>    shell;
  aPresContext->GetShell(getter_AddRefs(shell));
  nsCOMPtr<nsIFrameManager> frameManager;
  shell->GetFrameManager(getter_AddRefs(frameManager));
  nsresult                  rv = NS_OK;

  // Find the child frame that maps the content
  nsIFrame* childFrame;
  shell->GetPrimaryFrameFor(aChild, &childFrame);

  // When the last item is removed from a select, 
  // we need to add a pseudo frame so select gets sized as the best it can
  // so here we see if it is a select and then we get the number of options
  nsresult result = NS_ERROR_FAILURE;
  if (aContainer && childFrame) {
    nsCOMPtr<nsIDOMHTMLSelectElement> selectElement;
    result = aContainer->QueryInterface(nsCOMTypeInfo<nsIDOMHTMLSelectElement>::GetIID(),
                                               (void**)getter_AddRefs(selectElement));
    if (NS_SUCCEEDED(result) && selectElement) {
      PRUint32 numOptions = 0;
      result = selectElement->GetLength(&numOptions);
      nsIFrame * selectFrame;                             // XXX temp needed only native controls
      shell->GetPrimaryFrameFor(aContainer, &selectFrame);// XXX temp needed only native controls

      // For "select" add the pseudo frame after the last item is deleted
      nsIFrame* parentFrame = nsnull;
      childFrame->GetParent(&parentFrame);
      if (parentFrame == selectFrame) { // XXX temp needed only native controls
        return NS_ERROR_FAILURE;
      }
      if (NS_SUCCEEDED(result) && shell && parentFrame && 1 == numOptions) { 
  
        nsIStyleContext*          styleContext   = nsnull; 
        nsIFrame*                 generatedFrame = nsnull; 
        nsFrameConstructorState   state(aPresContext, nsnull, nsnull, nsnull);

        //shell->GetPrimaryFrameFor(aContainer, &contentFrame);
        parentFrame->GetStyleContext(&styleContext); 
        if (CreateGeneratedContentFrame(aPresContext, state, parentFrame, aContainer, 
                                        styleContext, nsLayoutAtoms::dummyOptionPseudo, 
                                        PR_FALSE, PR_FALSE, &generatedFrame)) { 
          // Add the generated frame to the child list 
          frameManager->AppendFrames(*aPresContext, *shell, parentFrame, nsnull, generatedFrame);
        }
      } 
    } 
  }

#ifdef INCLUDE_XUL
  if (aContainer) {
    nsCOMPtr<nsIAtom> tag;
    aContainer->GetTag(*getter_AddRefs(tag));
    if (tag.get() == nsXULAtoms::treechildren ||
        tag.get() == nsXULAtoms::treeitem) {
      if (childFrame) {
        // Convert to a tree row group frame.
        nsIFrame* parentFrame;
        childFrame->GetParent(&parentFrame);
        nsTreeRowGroupFrame* treeRowGroup = (nsTreeRowGroupFrame*)parentFrame;
        if (treeRowGroup && treeRowGroup->IsLazy()) {
          treeRowGroup->OnContentRemoved(*aPresContext, childFrame);
          return NS_OK;
        }
      }
      else {
        // Ensure that we notify the outermost row group that the item
        // has been removed (so that we can update the scrollbar state).
        // Walk up to the outermost tree row group frame and tell it that
        // content was removed.
        nsCOMPtr<nsIContent> parent;
        nsCOMPtr<nsIContent> child = dont_QueryInterface(aContainer);
        child->GetParent(*getter_AddRefs(parent));
        while (parent) {
          parent->GetTag(*getter_AddRefs(tag));
          if (tag.get() == nsXULAtoms::tree)
            break;
          child = parent;
          child->GetParent(*getter_AddRefs(parent));
        }

        if (parent) {
          // We found it.  Get the primary frame.
          nsIFrame*     parentFrame = GetFrameFor(shell, aPresContext, child);

          // Convert to a tree row group frame.
          nsTreeRowGroupFrame* treeRowGroup = (nsTreeRowGroupFrame*)parentFrame;
          if (treeRowGroup && treeRowGroup->IsLazy()) {
            treeRowGroup->OnContentRemoved(*aPresContext, nsnull);
            return NS_OK;
          }
        }
      }
    }
  }
#endif // INCLUDE_XUL

#if 0
  // It's possible that we just made an HTML paragraph empty...
  if (IsEmptyHTMLParagraph(aContainer)) {
    nsCOMPtr<nsIContent> containerParent;
    aContainer->GetParent(*getter_AddRefs(containerParent));
    if (containerParent) {
      PRInt32 ix;
      containerParent->IndexOf(aContainer, ix);
      if (ix >= 0) {
        // No point in doing the fixup
        needFixup = PR_FALSE;
        rv = ContentRemoved(aPresContext, containerParent, aContainer, ix);
      }
    }
  }
#endif

  if (childFrame) {
    // Get the childFrame's parent frame
    nsIFrame* parentFrame;
    childFrame->GetParent(&parentFrame);

    // Examine the containing-block for the removed content and see if
    // :first-letter style applies.
    nsIFrame* containingBlock =
      GetFloaterContainingBlock(aPresContext, parentFrame);
    nsCOMPtr<nsIStyleContext> blockSC;
    containingBlock->GetStyleContext(getter_AddRefs(blockSC));
    nsCOMPtr<nsIContent> blockContent;
    containingBlock->GetContent(getter_AddRefs(blockContent));
    PRBool haveFLS = HaveFirstLetterStyle(aPresContext, blockContent, blockSC);
    PRBool needFixup = PR_FALSE;
    PRBool haveLetterFrame = PR_FALSE;
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

      // Fixup parentFrame and childFrame
      nsCOMPtr<nsIAtom> type;
      parentFrame->GetFrameType(getter_AddRefs(type));
      if (type.get() == nsLayoutAtoms::letterFrame) {
        haveLetterFrame = PR_TRUE;
        if (0 == aIndexInContainer) {
          // If we get here then not only are we in a first-letter
          // situation (haveFLS is true and aIndexInContainer is zero)
          // but we are targetting the first-letter frame for
          // removal. Therefore, we will need to perform a fixup when
          // finished so that the new first child's frame will be
          // wrapped properly in a first-letter frame.
          //
          // It's important to only set this flag in this condition,
          // otherwise we will recursively reformat every child of
          // aContainer.
          needFixup = PR_TRUE;
        }
        childFrame = parentFrame;
        parentFrame->GetParent(&parentFrame);
#ifdef NOISY_FIRST_LETTER
        printf("  ==> revised parentFrame=");
        nsFrame::ListTag(stdout, parentFrame);
        printf(" childFrame=");
        nsFrame::ListTag(stdout, childFrame);
        printf(" needFixup=%s\n", needFixup ? "yes" : "no");
#endif
      }
    }

    // Walk the frame subtree deleting any out-of-flow frames, and
    // remove the mapping from content objects to frames
    DeletingFrameSubtree(aPresContext, shell, frameManager, childFrame, childFrame);

    // See if the child frame is a floating frame
    const nsStyleDisplay* display;
    childFrame->GetStyleData(eStyleStruct_Display,
                             (const nsStyleStruct*&)display);
    if (display->IsFloating()) {
      // Get the placeholder frame
      nsIFrame* placeholderFrame;
      frameManager->GetPlaceholderFrameFor(childFrame, &placeholderFrame);

      // Remove the mapping from the frame to its placeholder
      frameManager->SetPlaceholderFrameFor(childFrame, nsnull);

      if (haveLetterFrame) {
        // Deal with an ugly case: a floating first-letter frame; the
        // text that didn't make it into the first-letter is continued
        // and we need to make sure that the continuation is removed
        // too. The continuation will not be a child of the
        // first-letter frame.
        nsIFrame* textFrame;
        childFrame->FirstChild(nsnull, &textFrame);
        if (textFrame) {
          // Use the next-in-flow pointer to find the continuation of
          // the text frame.
          nsIFrame* textFrameNextInFlow;
          textFrame->GetNextInFlow(&textFrameNextInFlow);
          if (textFrameNextInFlow) {
            // Tell the parent to remove its child frame (and any
            // other continuations).
            nsIFrame* textFrameNextInFlowParent;
            textFrameNextInFlow->GetParent(&textFrameNextInFlowParent);
            rv = frameManager->RemoveFrame(*aPresContext, *shell,
                                           textFrameNextInFlowParent, nsnull,
                                           textFrameNextInFlow);
          }
        }
      }

      // Now we remove the floating frame

      // XXX has to be done first for now: the blocks line list
      // contains an array of pointers to the placeholder - we have to
      // remove the floater first (which gets rid of the lines
      // reference to the placeholder and floater) and then remove the
      // placeholder
      rv = frameManager->RemoveFrame(*aPresContext, *shell, parentFrame,
                                     nsLayoutAtoms::floaterList, childFrame);

      // Remove the placeholder frame first (XXX second for now) (so
      // that it doesn't retain a dangling pointer to memory)
      if (nsnull != placeholderFrame) {
        placeholderFrame->GetParent(&parentFrame);
        rv = frameManager->RemoveFrame(*aPresContext, *shell, parentFrame,
                                       nsnull, placeholderFrame);
      }
    }
    else {
      // See if it's absolutely or fixed positioned
      const nsStylePosition* position;
      childFrame->GetStyleData(eStyleStruct_Position, (const nsStyleStruct*&)position);
      if (position->IsAbsolutelyPositioned()) {
        // Get the placeholder frame
        nsIFrame* placeholderFrame;
        frameManager->GetPlaceholderFrameFor(childFrame, &placeholderFrame);

        // Remove the mapping from the frame to its placeholder
        frameManager->SetPlaceholderFrameFor(childFrame, nsnull);

        // Generate two notifications. First for the absolutely positioned
        // frame
        rv = frameManager->RemoveFrame(*aPresContext, *shell, parentFrame,
          (NS_STYLE_POSITION_FIXED == position->mPosition) ?
          nsLayoutAtoms::fixedList : nsLayoutAtoms::absoluteList, childFrame);

        // Now the placeholder frame
        if (nsnull != placeholderFrame) {
          placeholderFrame->GetParent(&parentFrame);
          rv = frameManager->RemoveFrame(*aPresContext, *shell, parentFrame, nsnull,
                                         placeholderFrame);
        }

      } else {
        // Notify the parent frame that it should delete the frame
        rv = frameManager->RemoveFrame(*aPresContext, *shell, parentFrame,
                                       nsnull, childFrame);

        // 
      }
    }

    if (mInitialContainingBlock == childFrame) {
      mInitialContainingBlock = nsnull;
    }

    if (needFixup) {
      // Now that we have a new first child of the block (either
      // directly, or a "logical" one) we need to reformat it because
      // its now in a first-letter situation.
      nsCOMPtr<nsIContent> child0;
      aContainer->ChildAt(0, *getter_AddRefs(child0));
      if (child0) {
        // Replace the content with itself; this time it won't
        // get the special first-letter frame wrapped around it.
        rv = ContentReplaced(aPresContext, aContainer,
                             child0, child0, 0);
      }
    }

  }

  return rv;
}

static void
ApplyRenderingChangeToTree(nsIPresContext& aPresContext,
                           nsIFrame* aFrame,
                           nsIViewManager* aViewManager);

static void
SyncAndInvalidateView(nsIView* aView, nsIFrame* aFrame, 
                      nsIViewManager* aViewManager)
{
  const nsStyleColor* color;
  const nsStyleDisplay* disp; 
  aFrame->GetStyleData(eStyleStruct_Color, (const nsStyleStruct*&) color);
  aFrame->GetStyleData(eStyleStruct_Display, (const nsStyleStruct*&) disp);

  // Set the view's opacity
  aViewManager->SetViewOpacity(aView, color->mOpacity);

  // See if the view should be hidden or visible
  PRBool  viewIsVisible = PR_TRUE;
  PRBool  viewHasTransparentContent = (color->mBackgroundFlags &
            NS_STYLE_BG_COLOR_TRANSPARENT) == NS_STYLE_BG_COLOR_TRANSPARENT;

  if (NS_STYLE_VISIBILITY_HIDDEN == disp->mVisible) {
    // If it's a scroll frame, then hide the view. This means that
    // child elements can't override their parent's visibility, but
    // it's not practical to leave it visible in all cases because
    // the scrollbars will be showing
    nsIAtom*  frameType;
    aFrame->GetFrameType(&frameType);

    if (frameType == nsLayoutAtoms::scrollFrame) {
      viewIsVisible = PR_FALSE;

    } else {
      // If it's a container element, then leave the view visible, but
      // mark it as having transparent content. The reason we need to
      // do this is that child elements can override their parent's
      // hidden visibility and be visible anyway
      nsIFrame* firstChild;
  
      aFrame->FirstChild(nsnull, &firstChild);
      if (firstChild) {
        // It's not a left frame, so the view needs to be visible, but
        // marked as having transparent content
        viewHasTransparentContent = PR_TRUE;
      } else {
        // It's a leaf frame so go ahead and hide the view
        viewIsVisible = PR_FALSE;
      }
    }
    NS_IF_RELEASE(frameType);
  }

  // Make sure visibility is correct
  aViewManager->SetViewVisibility(aView, viewIsVisible ? nsViewVisibility_kShow :
                                  nsViewVisibility_kHide);

  // Make sure content transparency is correct
  if (viewIsVisible) {
    aViewManager->SetViewContentTransparency(aView, viewHasTransparentContent);
  }
}

static void
UpdateViewsForTree(nsIPresContext& aPresContext, nsIFrame* aFrame, 
                   nsIViewManager* aViewManager, nsRect& aBoundsRect)
{
  nsIView* view;
  aFrame->GetView(&view);

  if (view) {
    SyncAndInvalidateView(view, aFrame, aViewManager);
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
    aFrame->FirstChild(childList, &child);
    while (child) {
      nsFrameState  childState;
      child->GetFrameState(&childState);
      if (NS_FRAME_OUT_OF_FLOW != (childState & NS_FRAME_OUT_OF_FLOW)) {
        // only do frames that are in flow
        child->GetFrameType(&frameType);
        if (nsLayoutAtoms::placeholderFrame == frameType) { // placeholder
          // get out of flow frame and start over there
          nsIFrame* outOfFlowFrame = ((nsPlaceholderFrame*)child)->GetOutOfFlowFrame();
          NS_ASSERTION(outOfFlowFrame, "no out-of-flow frame");

          ApplyRenderingChangeToTree(aPresContext, outOfFlowFrame, aViewManager);
        }
        else {  // regular frame
          nsRect  childBounds;
          UpdateViewsForTree(aPresContext, child, aViewManager, childBounds);
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
ApplyRenderingChangeToTree(nsIPresContext& aPresContext,
                           nsIFrame* aFrame,
                           nsIViewManager* aViewManager)
{
  nsIViewManager* viewManager = aViewManager;

  // Trigger rendering updates by damaging this frame and any
  // continuations of this frame.

  // XXX this needs to detect the need for a view due to an opacity change and deal with it...

  if (viewManager) {
    NS_ADDREF(viewManager); // add local ref
    viewManager->BeginUpdateViewBatch();
  }
  while (nsnull != aFrame) {
    // Get the frame's bounding rect
    nsRect invalidRect;
    nsPoint viewOffset;

    // Get view if this frame has one and trigger an update. If the
    // frame doesn't have a view, find the nearest containing view
    // (adjusting r's coordinate system to reflect the nesting) and
    // update there.
    nsIView* view = nsnull;
    aFrame->GetView(&view);
    nsIView* parentView;
    if (! view) { // XXX can view have children outside it?
      aFrame->GetOffsetFromView(viewOffset, &parentView);
      NS_ASSERTION(nsnull != parentView, "no view");
      if (! viewManager) {
        parentView->GetViewManager(viewManager);
        viewManager->BeginUpdateViewBatch();
      }
    }
    else {
      if (! viewManager) {
        view->GetViewManager(viewManager);
        viewManager->BeginUpdateViewBatch();
      }
    }
    UpdateViewsForTree(aPresContext, aFrame, viewManager, invalidRect);

    if (! view) { // if frame has view, will already be invalidated
      // XXX Instead of calling this we should really be calling
      // Invalidate on on the nsFrame (which does this)
      const nsStyleSpacing* spacing;
      aFrame->GetStyleData(eStyleStruct_Spacing, (const nsStyleStruct*&)spacing);
      nscoord width;
      spacing->GetOutlineWidth(width);
      if (width > 0) {
        invalidRect.Inflate(width, width);
      }
      nsPoint frameOrigin;
      aFrame->GetOrigin(frameOrigin);
      invalidRect -= frameOrigin;
      invalidRect += viewOffset;
      viewManager->UpdateView(parentView, invalidRect, NS_VMREFRESH_NO_SYNC);
    }

    aFrame->GetNextInFlow(&aFrame);
  }

  if (viewManager) {
    viewManager->EndUpdateViewBatch();
//    viewManager->Composite();
    NS_RELEASE(viewManager);
  }
}

static void
StyleChangeReflow(nsIPresContext* aPresContext,
                  nsIFrame* aFrame,
                  nsIAtom * aAttribute)
{
  nsCOMPtr<nsIPresShell> shell;
  aPresContext->GetShell(getter_AddRefs(shell));
    
  nsIReflowCommand* reflowCmd;
  nsresult rv = NS_NewHTMLReflowCommand(&reflowCmd, aFrame,
                                        nsIReflowCommand::StyleChanged,
                                        nsnull,
                                        aAttribute);
  if (NS_SUCCEEDED(rv)) {
    shell->AppendReflowCommand(reflowCmd);
    NS_RELEASE(reflowCmd);
  }
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
    frame->ContentChanged(aPresContext, aContent, aSubContent);
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
    PRInt32 hint;
    aChangeList.ChangeAt(count, frame, hint);
    switch (hint) {
      case NS_STYLE_HINT_RECONSTRUCT_ALL:
        NS_ERROR("This shouldn't happen");
        break;
      case NS_STYLE_HINT_FRAMECHANGE:
        nsIContent* content;
        frame->GetContent(&content);
        RecreateFramesForContent(aPresContext, content);
        NS_IF_RELEASE(content);
        break;
      case NS_STYLE_HINT_REFLOW:
        StyleChangeReflow(aPresContext, frame, nsnull);
        break;
      case NS_STYLE_HINT_VISUAL:
        ApplyRenderingChangeToTree(*aPresContext, frame, nsnull);
        break;
      case NS_STYLE_HINT_CONTENT:
      default:
        break;
    }
  }
  aChangeList.Clear();
  return NS_OK;
}

NS_IMETHODIMP
nsCSSFrameConstructor::ContentStatesChanged(nsIPresContext* aPresContext, 
                                            nsIContent* aContent1,
                                            nsIContent* aContent2)
{
  nsresult  result = NS_OK;

  nsCOMPtr<nsIPresShell> shell;
  aPresContext->GetShell(getter_AddRefs(shell));

  NS_ASSERTION(shell, "couldn't get pres shell");
  if (shell) {
    nsIStyleSet* styleSet;
    shell->GetStyleSet(&styleSet);

    NS_ASSERTION(styleSet, "couldn't get style set");
    if (styleSet) { // test if any style rules exist which are dependent on content state
      nsIFrame* primaryFrame1 = nsnull;
      nsIFrame* primaryFrame2 = nsnull;
      if (aContent1 && (NS_OK == styleSet->HasStateDependentStyle(aPresContext, aContent1))) {
        shell->GetPrimaryFrameFor(aContent1, &primaryFrame1);
      }
      else {
        aContent1 = nsnull;
      }

      if (aContent2 && (aContent2 != aContent1) && 
          (NS_OK == styleSet->HasStateDependentStyle(aPresContext, aContent2))) {
        shell->GetPrimaryFrameFor(aContent2, &primaryFrame2);
      }
      else {
        aContent2 = nsnull;
      }
      NS_RELEASE(styleSet);

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
        PRInt32 frameChange1 = NS_STYLE_HINT_NONE;
        PRInt32 frameChange2 = NS_STYLE_HINT_NONE;
        frameManager->ComputeStyleChangeFor(*aPresContext, primaryFrame1, changeList1,
                                            NS_STYLE_HINT_NONE, frameChange1);

        if ((frameChange1 != NS_STYLE_HINT_RECONSTRUCT_ALL) && (primaryFrame2)) {
          frameManager->ComputeStyleChangeFor(*aPresContext, primaryFrame2, changeList2,
                                              NS_STYLE_HINT_NONE, frameChange2);
        }

        if ((frameChange1 == NS_STYLE_HINT_RECONSTRUCT_ALL) || 
            (frameChange2 == NS_STYLE_HINT_RECONSTRUCT_ALL)) {
          result = ReconstructDocElementHierarchy(aPresContext);
        }
        else {
          switch (frameChange1) {
            case NS_STYLE_HINT_FRAMECHANGE:
              result = RecreateFramesForContent(aPresContext, aContent1);
              changeList1.Clear();
              break;
            case NS_STYLE_HINT_REFLOW:
            case NS_STYLE_HINT_VISUAL:
            case NS_STYLE_HINT_CONTENT:
              // let primary frame deal with it
              result = primaryFrame1->ContentStateChanged(aPresContext, aContent1, frameChange1);
            default:
              break;
          }
          switch (frameChange2) {
            case NS_STYLE_HINT_FRAMECHANGE:
              result = RecreateFramesForContent(aPresContext, aContent2);
              changeList2.Clear();
              break;
            case NS_STYLE_HINT_REFLOW:
            case NS_STYLE_HINT_VISUAL:
            case NS_STYLE_HINT_CONTENT:
              // let primary frame deal with it
              result = primaryFrame2->ContentStateChanged(aPresContext, aContent2, frameChange2);
              // then process any children that need it
            default:
              break;
          }
          ProcessRestyledFrames(changeList1, aPresContext);
          ProcessRestyledFrames(changeList2, aPresContext);
        }
      }
      else if (primaryFrame2) {
        nsStyleChangeList changeList;
        PRInt32 frameChange = NS_STYLE_HINT_NONE;
        frameManager->ComputeStyleChangeFor(*aPresContext, primaryFrame2, changeList,
                                            NS_STYLE_HINT_NONE, frameChange);

        switch (frameChange) {  // max change needed for top level frames
          case NS_STYLE_HINT_RECONSTRUCT_ALL:
            result = ReconstructDocElementHierarchy(aPresContext);
            changeList.Clear();
            break;
          case NS_STYLE_HINT_FRAMECHANGE:
            result = RecreateFramesForContent(aPresContext, aContent2);
            changeList.Clear();
            break;
          case NS_STYLE_HINT_REFLOW:
          case NS_STYLE_HINT_VISUAL:
          case NS_STYLE_HINT_CONTENT:
            // let primary frame deal with it
            result = primaryFrame2->ContentStateChanged(aPresContext, aContent2, frameChange);
            // then process any children that need it
          default:
            break;
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
                                        nsIAtom* aAttribute,
                                        PRInt32 aHint)
{
  nsresult  result = NS_OK;

  nsCOMPtr<nsIPresShell> shell;
  aPresContext->GetShell(getter_AddRefs(shell));
  nsIFrame*     primaryFrame;
   
  shell->GetPrimaryFrameFor(aContent, &primaryFrame);

  PRBool  reconstruct = PR_FALSE;
  PRBool  restyle = PR_FALSE;
  PRBool  reframe = PR_FALSE;

#if 0
  NS_FRAME_LOG(NS_FRAME_TRACE_CALLS,
     ("HTMLStyleSheet::AttributeChanged: content=%p[%s] frame=%p",
      aContent, ContentTag(aContent, 0), frame));
#endif

  // the style tag has its own interpretation based on aHint 
  if (NS_STYLE_HINT_UNKNOWN == aHint) { 
    nsIStyledContent* styledContent;
    result = aContent->QueryInterface(nsIStyledContent::GetIID(), (void**)&styledContent);

    if (NS_OK == result) { 
      // Get style hint from HTML content object. 
      styledContent->GetMappedAttributeImpact(aAttribute, aHint);
      NS_RELEASE(styledContent); 
    } 
  } 

  switch (aHint) {
    default:
    case NS_STYLE_HINT_RECONSTRUCT_ALL:
      reconstruct = PR_TRUE;
    case NS_STYLE_HINT_FRAMECHANGE:
      reframe = PR_TRUE;
    case NS_STYLE_HINT_REFLOW:
    case NS_STYLE_HINT_VISUAL:
    case NS_STYLE_HINT_UNKNOWN:
    case NS_STYLE_HINT_CONTENT:
    case NS_STYLE_HINT_AURAL:
      restyle = PR_TRUE;
      break;
    case NS_STYLE_HINT_NONE:
      break;
  }

  // apply changes
  if (PR_TRUE == reconstruct) {
    result = ReconstructDocElementHierarchy(aPresContext);
  }
  else if (PR_TRUE == reframe) {
    result = RecreateFramesForContent(aPresContext, aContent);
  }
  else if (PR_TRUE == restyle) {
    // If there is no frame then there is no point in re-styling it,
    // is there?
    if (primaryFrame) {
      PRInt32 maxHint = aHint;
      nsStyleChangeList changeList;
      // put primary frame on list to deal with, re-resolve may update or add next in flows
      changeList.AppendChange(primaryFrame, maxHint);
      nsCOMPtr<nsIFrameManager> frameManager;
      shell->GetFrameManager(getter_AddRefs(frameManager));
      frameManager->ComputeStyleChangeFor(*aPresContext, primaryFrame, changeList,
                                          aHint, maxHint);

      switch (maxHint) {  // maxHint is hint for primary only
        case NS_STYLE_HINT_RECONSTRUCT_ALL:
          result = ReconstructDocElementHierarchy(aPresContext);
          changeList.Clear();
          break;
        case NS_STYLE_HINT_FRAMECHANGE:
          result = RecreateFramesForContent(aPresContext, aContent);
          changeList.Clear();
          break;
        case NS_STYLE_HINT_REFLOW:
        case NS_STYLE_HINT_VISUAL:
        case NS_STYLE_HINT_CONTENT:
          // let the frame deal with it, since we don't know how to
          result = primaryFrame->AttributeChanged(aPresContext, aContent, aAttribute, maxHint);
        default:
          break;
      }
      // handle any children (primary may be on list too)
      ProcessRestyledFrames(changeList, aPresContext);
    }
    else {  // no frame now, possibly genetate one with new style data
      result = RecreateFramesForContent(aPresContext, aContent);
    }
  }

  return result;
}

  // Style change notifications
NS_IMETHODIMP
nsCSSFrameConstructor::StyleRuleChanged(nsIPresContext* aPresContext,
                                        nsIStyleSheet* aStyleSheet,
                                        nsIStyleRule* aStyleRule,
                                        PRInt32 aHint)
{
  nsresult result = NS_OK;
  nsCOMPtr<nsIPresShell> shell;
  aPresContext->GetShell(getter_AddRefs(shell));
  nsIFrame* frame;
  shell->GetRootFrame(&frame);

  PRBool reframe  = PR_FALSE;
  PRBool reflow   = PR_FALSE;
  PRBool render   = PR_FALSE;
  PRBool restyle  = PR_FALSE;
  switch (aHint) {
    default:
    case NS_STYLE_HINT_UNKNOWN:
    case NS_STYLE_HINT_FRAMECHANGE:
      reframe = PR_TRUE;
    case NS_STYLE_HINT_REFLOW:
      reflow = PR_TRUE;
    case NS_STYLE_HINT_VISUAL:
      render = PR_TRUE;
    case NS_STYLE_HINT_CONTENT:
    case NS_STYLE_HINT_AURAL:
      restyle = PR_TRUE;
      break;
    case NS_STYLE_HINT_NONE:
      break;
  }

  if (restyle) {
    nsIStyleContext* sc;
    frame->GetStyleContext(&sc);
    sc->RemapStyle(aPresContext);
    NS_RELEASE(sc);
  }

  if (reframe) {
    result = ReconstructDocElementHierarchy(aPresContext);
  }
  else {
    // XXX hack, skip the root and scrolling frames
    frame->FirstChild(nsnull, &frame);
    frame->FirstChild(nsnull, &frame);
    if (reflow) {
      StyleChangeReflow(aPresContext, frame, nsnull);
    }
    else if (render) {
      ApplyRenderingChangeToTree(*aPresContext, frame, nsnull);
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

// Construct an alternate frame to use when the image can't be rendered
nsresult
nsCSSFrameConstructor::ConstructAlternateImageFrame(nsIPresContext*  aPresContext,
                                                    nsIContent*      aContent,
                                                    nsIStyleContext* aStyleContext,
                                                    nsIFrame*        aParentFrame,
                                                    nsIFrame*&       aFrame)
{
  nsIDOMHTMLImageElement*  imageElement;
  nsresult                 rv;

  // Initialize OUT parameter
  aFrame = nsnull;

  rv = aContent->QueryInterface(kIDOMHTMLImageElementIID, (void**)&imageElement);
  if (NS_SUCCEEDED(rv)) {
    nsAutoString  altText;
  
    // The "alt" attribute specifies alternate text that is rendered
    // when the image can not be displayed
    imageElement->GetAlt(altText);
    if (0 == altText.Length()) {
      // If there's no "alt" attribute, then use the value of the "title"
      // attribute
      imageElement->GetTitle(altText);
    }
    if (0 == altText.Length()) {
      // If there's no "title" attribute, then use the filename minus the
      // extension
      imageElement->GetSrc(altText);
      if (altText.Length() > 0) {
        // Trim off the path part of the filename
        PRInt32 offset = altText.RFindChar('/');
        if (offset >= 0) {
          altText.Cut(0, offset + 1);
        }

        // Trim off the extension
        offset = altText.RFindChar('.');
        if (offset >= 0) {
          altText.Truncate(offset);
        }
      }
    }
    NS_RELEASE(imageElement);
  
    // Create a text content element for the alternate text
    nsCOMPtr<nsIContent> altTextContent;
    NS_NewTextNode(getter_AddRefs(altTextContent));

    // Set aContent as the parent content and set the document object
    nsCOMPtr<nsIDocument> document;
    aContent->GetDocument(*getter_AddRefs(document));
    altTextContent->SetParent(aContent);
    altTextContent->SetDocument(document, PR_TRUE);

    // Set the content's text
    nsIDOMCharacterData* domData;
    altTextContent->QueryInterface(kIDOMCharacterDataIID, (void**)&domData);
    domData->SetData(altText);
    NS_RELEASE(domData);

    // Create either an inline frame, block frame, or area frame
    nsIFrame* containerFrame;
    const nsStyleDisplay* display = (const nsStyleDisplay*)
      aStyleContext->GetStyleData(eStyleStruct_Display);
    const nsStylePosition* position = (const nsStylePosition*)
      aStyleContext->GetStyleData(eStyleStruct_Position);

    if (position->IsAbsolutelyPositioned()) {
      NS_NewAbsoluteItemWrapperFrame(&containerFrame);
    } else if (display->IsFloating() || (NS_STYLE_DISPLAY_BLOCK == display->mDisplay)) {
      NS_NewBlockFrame(&containerFrame);
    } else {
      NS_NewInlineFrame(&containerFrame);
    }
    containerFrame->Init(*aPresContext, aContent, aParentFrame, aStyleContext, nsnull);
    nsHTMLContainerFrame::CreateViewForFrame(*aPresContext, containerFrame,
                                             aStyleContext, PR_FALSE);

    // Create a text frame to display the alt-text. It gets a pseudo-element
    // style context
    nsIFrame*        textFrame;
    nsIStyleContext* textStyleContext;

    NS_NewTextFrame(&textFrame);
    aPresContext->ResolvePseudoStyleContextFor(aContent, nsHTMLAtoms::textPseudo,
                                               aStyleContext, PR_FALSE,
                                               &textStyleContext);

    textFrame->Init(*aPresContext, altTextContent, containerFrame,
                    textStyleContext, nsnull);
    NS_RELEASE(textStyleContext);
    containerFrame->SetInitialChildList(*aPresContext, nsnull, textFrame);

    // Return the container frame
    aFrame = containerFrame;
  }

  return rv;
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

NS_IMETHODIMP
nsCSSFrameConstructor::CantRenderReplacedElement(nsIPresContext* aPresContext,
                                                 nsIFrame*       aFrame)
{
  nsIFrame*                 parentFrame;
  nsCOMPtr<nsIStyleContext> styleContext;
  nsCOMPtr<nsIContent>      content;
  nsCOMPtr<nsIAtom>         tag;
  nsresult                  rv = NS_OK;

  aFrame->GetParent(&parentFrame);
  aFrame->GetStyleContext(getter_AddRefs(styleContext));

  // Get aFrame's content object and the tag name
  aFrame->GetContent(getter_AddRefs(content));
  NS_ASSERTION(content, "null content object");
  content->GetTag(*getter_AddRefs(tag));

  // Get the child list name that the frame is contained in
  nsCOMPtr<nsIAtom>  listName;
  GetChildListNameFor(parentFrame, aFrame, getter_AddRefs(listName));

  // If the frame is out of the flow, then it has a placeholder frame.
  nsIFrame* placeholderFrame = nsnull;
  nsCOMPtr<nsIPresShell> presShell;
  aPresContext->GetShell(getter_AddRefs(presShell));
  if (listName) {
    presShell->GetPlaceholderFrameFor(aFrame, &placeholderFrame);
  }

  // Get the previous sibling frame
  nsIFrame*     firstChild;
  parentFrame->FirstChild(listName, &firstChild);
  nsFrameList   frameList(firstChild);
  
  // See whether it's an IMG or an OBJECT element
  if (nsHTMLAtoms::img == tag.get()) {
    // It's an IMG element. Try and construct an alternate frame to use when the
    // image can't be rendered
    nsIFrame* newFrame;
    rv = ConstructAlternateImageFrame(aPresContext, content, styleContext,
                                      parentFrame, newFrame);

    if (NS_SUCCEEDED(rv)) {
      nsCOMPtr<nsIFrameManager> frameManager;
      presShell->GetFrameManager(getter_AddRefs(frameManager));

      // Replace the old frame with the new frame
      // Reset the primary frame mapping
      frameManager->SetPrimaryFrameFor(content, newFrame);

      if (placeholderFrame) {
        // Reuse the existing placeholder frame, and add an association to the
        // new frame
        frameManager->SetPlaceholderFrameFor(newFrame, placeholderFrame);
      }

      // Replace the old frame with the new frame
      frameManager->ReplaceFrame(*aPresContext, *presShell, parentFrame,
                                 listName, aFrame, newFrame);

      // Now that we've replaced the primary frame, if there's a placeholder
      // frame then complete the transition from image frame to new frame
      if (placeholderFrame) {
        // Remove the association between the old frame and its placeholder
        frameManager->SetPlaceholderFrameFor(aFrame, nsnull);
        
        // Placeholder frames have a pointer back to the out-of-flow frame.
        // Make sure that's correct, too.
        ((nsPlaceholderFrame*)placeholderFrame)->SetOutOfFlowFrame(newFrame);
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

    absoluteContainingBlock = GetAbsoluteContainingBlock(aPresContext, inFlowParent),
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
                                  absoluteContainingBlock, floaterContainingBlock);
    nsFrameItems            frameItems;
    const nsStyleDisplay*   display = (const nsStyleDisplay*)
      styleContext->GetStyleData(eStyleStruct_Display);

    // Create a new frame based on the display type.
    // Note: if the old frame was out-of-flow, then so will the new frame
    // and we'll get a new placeholder frame
    rv = ConstructFrameByDisplayType(aPresContext, state, display, content,
                                     inFlowParent, styleContext, PR_FALSE, frameItems);

    if (NS_SUCCEEDED(rv)) {
      nsIFrame* newFrame = frameItems.childList;

      if (placeholderFrame) {
        // Remove the association between the old frame and its placeholder
        // Note: ConstructFrameByDisplayType() will already have added an
        // association for the new placeholder frame
        state.mFrameManager->SetPlaceholderFrameFor(aFrame, nsnull);

        // Verify that the new frame is also a placeholder frame
        NS_ASSERTION(IsPlaceholderFrame(newFrame), "unexpected frame type");

        // Replace the old placeholder frame with the new placeholder frame
        state.mFrameManager->ReplaceFrame(*aPresContext, *presShell, inFlowParent,
                                          nsnull, placeholderFrame, newFrame);
      }

      // Replace the primary frame
      if (listName.get() == nsLayoutAtoms::absoluteList) {
        newFrame = state.mAbsoluteItems.childList;
        state.mAbsoluteItems.childList = nsnull;
      } else if (listName.get() == nsLayoutAtoms::fixedList) {
        newFrame = state.mFixedItems.childList;
        state.mFixedItems.childList = nsnull;
      } else if (listName.get() == nsLayoutAtoms::floaterList) {
        newFrame = state.mFloatedItems.childList;
        state.mFloatedItems.childList = nsnull;
      }
      state.mFrameManager->ReplaceFrame(*aPresContext, *presShell, parentFrame,
                                        listName, aFrame, newFrame);

      // Reset the primary frame mapping. Don't assume that
      // ConstructFrameByDisplayType() has done this
      state.mFrameManager->SetPrimaryFrameFor(content, newFrame);
      
      // If there are new absolutely positioned child frames, then notify
      // the parent
      // XXX We can't just assume these frames are being appended, we need to
      // determine where in the list they should be inserted...
      if (state.mAbsoluteItems.childList) {
        rv = state.mAbsoluteItems.containingBlock->AppendFrames(*aPresContext, *presShell,
                                                     nsLayoutAtoms::absoluteList,
                                                     state.mAbsoluteItems.childList);
      }
  
      // If there are new fixed positioned child frames, then notify
      // the parent
      // XXX We can't just assume these frames are being appended, we need to
      // determine where in the list they should be inserted...
      if (state.mFixedItems.childList) {
        rv = state.mFixedItems.containingBlock->AppendFrames(*aPresContext, *presShell,
                                                  nsLayoutAtoms::fixedList,
                                                  state.mFixedItems.childList);
      }
  
      // If there are new floating child frames, then notify the parent
      // XXX We can't just assume these frames are being appended, we need to
      // determine where in the list they should be inserted...
      if (state.mFloatedItems.childList) {
        rv = state.mFloatedItems.containingBlock->AppendFrames(*aPresContext,
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
nsCSSFrameConstructor::CreateContinuingOuterTableFrame(nsIPresContext*  aPresContext,
                                                       nsIFrame*        aFrame,
                                                       nsIFrame*        aParentFrame,
                                                       nsIContent*      aContent,
                                                       nsIStyleContext* aStyleContext,
                                                       nsIFrame**       aContinuingFrame)
{
  nsIFrame* newFrame;
  nsresult  rv;

  rv = NS_NewTableOuterFrame(&newFrame);
  if (NS_SUCCEEDED(rv)) {
    newFrame->Init(*aPresContext, aContent, aParentFrame, aStyleContext, aFrame);
    nsHTMLContainerFrame::CreateViewForFrame(*aPresContext, newFrame,
                                             aStyleContext, PR_FALSE);

    // Create a continuing inner table frame, and if there's a caption then
    // replicate the caption
    nsIFrame*     childFrame;
    nsFrameItems  newChildFrames;

    aFrame->FirstChild(nsnull, &childFrame);
    while (childFrame) {
      nsIAtom*  tableType;

      // See if it's the inner table frame
      childFrame->GetFrameType(&tableType);
      if (nsLayoutAtoms::tableFrame == tableType) {
        nsIFrame* continuingTableFrame;

        // It's the inner table frame, so create a continuing frame
        CreateContinuingFrame(aPresContext, childFrame, newFrame, &continuingTableFrame);
        newChildFrames.AddChild(continuingTableFrame);
      } else {
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
        NS_NewTableCaptionFrame(&captionFrame);
        nsFrameConstructorState state(aPresContext, mFixedContainingBlock,
                                      GetAbsoluteContainingBlock(aPresContext, newFrame),
                                      captionFrame);
        captionFrame->Init(*aPresContext, caption, newFrame, captionStyle, nsnull);
        ProcessChildren(aPresContext, state, caption, captionFrame,
                        PR_TRUE, childItems, PR_TRUE);
        captionFrame->SetInitialChildList(*aPresContext, nsnull, childItems.childList);
        // XXX Deal with absolute and fixed frames...
        if (state.mFloatedItems.childList) {
          captionFrame->SetInitialChildList(*aPresContext,
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
    newFrame->SetInitialChildList(*aPresContext, nsnull, newChildFrames.childList);
  }

  *aContinuingFrame = newFrame;
  return rv;
}

nsresult
nsCSSFrameConstructor::CreateContinuingTableFrame(nsIPresContext*  aPresContext,
                                                  nsIFrame*        aFrame,
                                                  nsIFrame*        aParentFrame,
                                                  nsIContent*      aContent,
                                                  nsIStyleContext* aStyleContext,
                                                  nsIFrame**       aContinuingFrame)
{
  nsIFrame* newFrame;
  nsresult  rv;
    
  rv = NS_NewTableFrame(&newFrame);
  if (NS_SUCCEEDED(rv)) {
    newFrame->Init(*aPresContext, aContent, aParentFrame, aStyleContext, aFrame);
    nsHTMLContainerFrame::CreateViewForFrame(*aPresContext, newFrame,
                                             aStyleContext, PR_FALSE);

    // Replicate any header/footer frames
    nsIFrame*     rowGroupFrame;
    nsFrameItems  childFrames;

    aFrame->FirstChild(nsnull, &rowGroupFrame);
    while (rowGroupFrame) {
      // See if it's a header/footer
      nsIStyleContext*      rowGroupStyle;
      const nsStyleDisplay* display;

      rowGroupFrame->GetStyleContext(&rowGroupStyle);
      display = (const nsStyleDisplay*)rowGroupStyle->GetStyleData(eStyleStruct_Display);

      if ((NS_STYLE_DISPLAY_TABLE_HEADER_GROUP == display->mDisplay) ||
          (NS_STYLE_DISPLAY_TABLE_FOOTER_GROUP == display->mDisplay)) {
        
        // Replicate the header/footer frame
        nsIFrame*               headerFooterFrame;
        nsFrameItems            childItems;
        nsIContent*             headerFooter;
        nsFrameConstructorState state(aPresContext, mFixedContainingBlock,
                                      GetAbsoluteContainingBlock(aPresContext, newFrame),
                                      nsnull);

        NS_NewTableRowGroupFrame(&headerFooterFrame);
        rowGroupFrame->GetContent(&headerFooter);
        headerFooterFrame->Init(*aPresContext, headerFooter, newFrame,
                                rowGroupStyle, nsnull);
        ProcessChildren(aPresContext, state, headerFooter, headerFooterFrame,
                        PR_FALSE, childItems, PR_FALSE);
        NS_ASSERTION(!state.mFloatedItems.childList, "unexpected floated element");
        NS_RELEASE(headerFooter);
        headerFooterFrame->SetInitialChildList(*aPresContext, nsnull, childItems.childList);

        // Table specific initialization
        ((nsTableRowGroupFrame*)headerFooterFrame)->InitRepeatedFrame
          ((nsTableRowGroupFrame*)rowGroupFrame);

        // XXX Deal with absolute and fixed frames...
        childFrames.AddChild(headerFooterFrame);
      }

      NS_RELEASE(rowGroupStyle);
      
      // Header and footer must be first, and then the body row groups.
      // So if we found a body row group, then stop looking for header and
      // footer elements
      if (NS_STYLE_DISPLAY_TABLE_ROW_GROUP == display->mDisplay) {
        break;
      }

      // Get the next row group frame
      rowGroupFrame->GetNextSibling(&rowGroupFrame);
    }
    
    // Set the table frame's initial child list
    newFrame->SetInitialChildList(*aPresContext, nsnull, childFrames.childList);
  }

  *aContinuingFrame = newFrame;
  return rv;
}

NS_IMETHODIMP
nsCSSFrameConstructor::CreateContinuingFrame(nsIPresContext* aPresContext,
                                             nsIFrame*       aFrame,
                                             nsIFrame*       aParentFrame,
                                             nsIFrame**      aContinuingFrame)
{
  nsIAtom*          frameType;
  nsIContent*       content;
  nsIStyleContext*  styleContext;
  nsIFrame*         newFrame = nsnull;
  nsresult          rv;

  // Use the frame type to determine what type of frame to create
  aFrame->GetFrameType(&frameType);
  aFrame->GetContent(&content);
  aFrame->GetStyleContext(&styleContext);

  if (nsLayoutAtoms::textFrame == frameType) {
    rv = NS_NewTextFrame(&newFrame);
    if (NS_SUCCEEDED(rv)) {
      newFrame->Init(*aPresContext, content, aParentFrame, styleContext, aFrame);
      nsHTMLContainerFrame::CreateViewForFrame(*aPresContext, newFrame,
                                               styleContext, PR_FALSE);
    }
    
  } else if (nsLayoutAtoms::inlineFrame == frameType) {
    rv = NS_NewInlineFrame(&newFrame);
    if (NS_SUCCEEDED(rv)) {
      newFrame->Init(*aPresContext, content, aParentFrame, styleContext, aFrame);
      nsHTMLContainerFrame::CreateViewForFrame(*aPresContext, newFrame,
                                               styleContext, PR_FALSE);
    }
  
  } else if (nsLayoutAtoms::blockFrame == frameType) {
    rv = NS_NewBlockFrame(&newFrame);
    if (NS_SUCCEEDED(rv)) {
      newFrame->Init(*aPresContext, content, aParentFrame, styleContext, aFrame);
      nsHTMLContainerFrame::CreateViewForFrame(*aPresContext, newFrame,
                                               styleContext, PR_FALSE);
    }
  
  } else if (nsLayoutAtoms::areaFrame == frameType) {
    rv = NS_NewAreaFrame(&newFrame);
    if (NS_SUCCEEDED(rv)) {
      newFrame->Init(*aPresContext, content, aParentFrame, styleContext,
                     aFrame);
      nsHTMLContainerFrame::CreateViewForFrame(*aPresContext, newFrame,
                                               styleContext, PR_FALSE);
    }
  
  } else if (nsLayoutAtoms::positionedInlineFrame == frameType) {
    rv = NS_NewPositionedInlineFrame(&newFrame);
    if (NS_SUCCEEDED(rv)) {
      newFrame->Init(*aPresContext, content, aParentFrame, styleContext, aFrame);
      nsHTMLContainerFrame::CreateViewForFrame(*aPresContext, newFrame,
                                               styleContext, PR_FALSE);
    }

  } else if (nsLayoutAtoms::pageFrame == frameType) {
    rv = NS_NewPageFrame(&newFrame);
    if (NS_SUCCEEDED(rv)) {
      newFrame->Init(*aPresContext, content, aParentFrame, styleContext, aFrame);
      nsHTMLContainerFrame::CreateViewForFrame(*aPresContext, newFrame,
                                               styleContext, PR_TRUE);
    }

  } else if (nsLayoutAtoms::tableOuterFrame == frameType) {
    rv = CreateContinuingOuterTableFrame(aPresContext, aFrame, aParentFrame,
                                         content, styleContext, &newFrame);

  } else if (nsLayoutAtoms::tableFrame == frameType) {
    rv = CreateContinuingTableFrame(aPresContext, aFrame, aParentFrame,
                                    content, styleContext, &newFrame);

  } else if (nsLayoutAtoms::tableRowGroupFrame == frameType) {
    rv = NS_NewTableRowGroupFrame(&newFrame);
    if (NS_SUCCEEDED(rv)) {
      newFrame->Init(*aPresContext, content, aParentFrame, styleContext, aFrame);
      nsHTMLContainerFrame::CreateViewForFrame(*aPresContext, newFrame,
                                               styleContext, PR_FALSE);
    }

  } else if (nsLayoutAtoms::tableRowFrame == frameType) {
    rv = NS_NewTableRowFrame(&newFrame);
    if (NS_SUCCEEDED(rv)) {
      newFrame->Init(*aPresContext, content, aParentFrame, styleContext, aFrame);
      nsHTMLContainerFrame::CreateViewForFrame(*aPresContext, newFrame,
                                               styleContext, PR_FALSE);

      // Create a continuing frame for each table cell frame
      nsIFrame*     cellFrame;
      nsFrameItems  newChildList;

      aFrame->FirstChild(nsnull, &cellFrame);
      while (cellFrame) {
        nsIAtom*  tableType;
        
        // See if it's a table cell frame
        cellFrame->GetFrameType(&tableType);
        if (nsLayoutAtoms::tableCellFrame == tableType) {
          nsIFrame* continuingCellFrame;

          CreateContinuingFrame(aPresContext, cellFrame, newFrame, &continuingCellFrame);
          newChildList.AddChild(continuingCellFrame);
        }

        NS_IF_RELEASE(tableType);
        cellFrame->GetNextSibling(&cellFrame);
      }
      
      // Set the table cell's initial child list
      newFrame->SetInitialChildList(*aPresContext, nsnull, newChildList.childList);
    }

  } else if (nsLayoutAtoms::tableCellFrame == frameType) {
    rv = NS_NewTableCellFrame(&newFrame);
    if (NS_SUCCEEDED(rv)) {
      newFrame->Init(*aPresContext, content, aParentFrame, styleContext, aFrame);
      nsHTMLContainerFrame::CreateViewForFrame(*aPresContext, newFrame,
                                               styleContext, PR_FALSE);

      // Create a continuing area frame
      nsIFrame* areaFrame;
      nsIFrame* continuingAreaFrame;
      aFrame->FirstChild(nsnull, &areaFrame);
      CreateContinuingFrame(aPresContext, areaFrame, newFrame, &continuingAreaFrame);

      // Set the table cell's initial child list
      newFrame->SetInitialChildList(*aPresContext, nsnull, continuingAreaFrame);
    }
  
  } else if (nsLayoutAtoms::lineFrame == frameType) {
    rv = NS_NewFirstLineFrame(&newFrame);
    if (NS_SUCCEEDED(rv)) {
      newFrame->Init(*aPresContext, content, aParentFrame, styleContext, aFrame);
      nsHTMLContainerFrame::CreateViewForFrame(*aPresContext, newFrame,
                                               styleContext, PR_FALSE);
    }
  
  } else if (nsLayoutAtoms::letterFrame == frameType) {
    rv = NS_NewFirstLetterFrame(&newFrame);
    if (NS_SUCCEEDED(rv)) {
      newFrame->Init(*aPresContext, content, aParentFrame, styleContext, aFrame);
      nsHTMLContainerFrame::CreateViewForFrame(*aPresContext, newFrame,
                                               styleContext, PR_FALSE);
    }

  } else {
    NS_ASSERTION(PR_FALSE, "unexpected frame type");
    rv = NS_ERROR_UNEXPECTED;
  }

  *aContinuingFrame = newFrame;
  NS_RELEASE(styleContext);
  NS_IF_RELEASE(content);
  NS_IF_RELEASE(frameType);
  return rv;
}

// Helper function that searches the immediate child frames for a frame that
// maps the specified content object
static nsIFrame*
FindFrameWithContent(nsIFrame*   aParentFrame,
                     nsIContent* aParentContent,
                     nsIContent* aContent)
{
  NS_PRECONDITION(aParentFrame, "No frame to search!");
  if (!aParentFrame) {
    return nsnull;
  }

keepLooking:
  // Search for the frame in each child list that aParentFrame supports
  nsIAtom* listName = nsnull;
  PRInt32 listIndex = 0;
  do {
    nsIFrame* kidFrame;
    aParentFrame->FirstChild(listName, &kidFrame);
    while (kidFrame) {
      nsCOMPtr<nsIContent>  kidContent;
      
      // See if the child frame points to the content object we're
      // looking for
      kidFrame->GetContent(getter_AddRefs(kidContent));
      if (kidContent.get() == aContent) {
        nsCOMPtr<nsIAtom>  frameType;

        // We found a match. See if it's a placeholder frame
        kidFrame->GetFrameType(getter_AddRefs(frameType));
        if (nsLayoutAtoms::placeholderFrame == frameType.get()) {
          // Ignore the placeholder and return the out-of-flow frame instead
          return ((nsPlaceholderFrame*)kidFrame)->GetOutOfFlowFrame();
        } else {
          // Return the matching child frame
          return kidFrame;
        }
      }

      // We search the immediate children only, but if the child frame has
      // the same content pointer as its parent then we need to search its
      // child frames, too
      if (kidContent.get() == aParentContent) {
        nsIFrame* matchingFrame = FindFrameWithContent(kidFrame, aParentContent,
                                                       aContent);

        if (matchingFrame) {
          return matchingFrame;
        }
      }

      // Get the next sibling frame
      kidFrame->GetNextSibling(&kidFrame);
    }

    NS_IF_RELEASE(listName);
    aParentFrame->GetAdditionalChildListName(listIndex++, &listName);
  } while(listName);

  // We didn't find a matching frame. If aFrame has a next-in-flow,
  // then continue looking there
  aParentFrame->GetNextInFlow(&aParentFrame);
  if (aParentFrame) {
    goto keepLooking;
  }

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
                                           nsIFrame**       aFrame)
{
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
  nsCOMPtr<nsIContent>   parentContent;
  nsIFrame*              parentFrame;

  // Get the frame that corresponds to the parent content object.
  // Note that this may recurse indirectly, because the pres shell will
  // call us back if there is no mapping in the hash table
  aContent->GetParent(*getter_AddRefs(parentContent));
  if (parentContent.get()) {
    aFrameManager->GetPrimaryFrameFor(parentContent, &parentFrame);
    if (parentFrame) {
      // Search the child frames for a match
      *aFrame = FindFrameWithContent(parentFrame, parentContent.get(), aContent);

      // If we found a match, then add a mapping to the hash table so
      // next time this will be quick
      if (*aFrame) {
        aFrameManager->SetPrimaryFrameFor(aContent, *aFrame);
      }
    }
  }

  return NS_OK;
}

nsresult
nsCSSFrameConstructor::RecreateFramesForContent(nsIPresContext* aPresContext,
                                                nsIContent* aContent)                                   
{
  nsresult rv = NS_OK;

  nsIContent *container;
  rv = aContent->GetParent(container);
  if (container) {
    PRInt32 indexInContainer;
    rv = container->IndexOf(aContent, indexInContainer);
    if (NS_OK == rv) {
      // First, remove the frames associated with the content object on which the
      // attribute change occurred.
      rv = ContentRemoved(aPresContext, container, aContent, indexInContainer);
  
      if (NS_OK == rv) {
        // Now, recreate the frames associated with this content object.
        rv = ContentInserted(aPresContext, container, aContent, indexInContainer);
      }
    }
    NS_RELEASE(container);    
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
  aPresContext->ResolvePseudoStyleContextFor(aContent,
                                             nsHTMLAtoms::firstLetterPseudo,
                                             aStyleContext, PR_FALSE, &fls);
  return fls;
}

nsIStyleContext*
nsCSSFrameConstructor::GetFirstLineStyle(nsIPresContext* aPresContext,
                                         nsIContent* aContent,
                                         nsIStyleContext* aStyleContext)
{
  nsIStyleContext* fls = nsnull;
  aPresContext->ResolvePseudoStyleContextFor(aContent,
                                             nsHTMLAtoms::firstLinePseudo,
                                             aStyleContext, PR_FALSE, &fls);
  return fls;
}

// Predicate to see if a given content (block element) has
// first-letter style applied to it.
PRBool
nsCSSFrameConstructor::HaveFirstLetterStyle(nsIPresContext* aPresContext,
                                            nsIContent* aContent,
                                            nsIStyleContext* aStyleContext)
{
  nsCOMPtr<nsIStyleContext> fls;
  aPresContext->ProbePseudoStyleContextFor(aContent,
                                           nsHTMLAtoms::firstLetterPseudo,
                                           aStyleContext, PR_FALSE,
                                           getter_AddRefs(fls));
  PRBool result = PR_FALSE;
  if (fls) {
    result = PR_TRUE;
  }
  return result;
}

PRBool
nsCSSFrameConstructor::HaveFirstLineStyle(nsIPresContext* aPresContext,
                                          nsIContent* aContent,
                                          nsIStyleContext* aStyleContext)
{
  nsCOMPtr<nsIStyleContext> fls;
  aPresContext->ProbePseudoStyleContextFor(aContent,
                                           nsHTMLAtoms::firstLinePseudo,
                                           aStyleContext, PR_FALSE,
                                           getter_AddRefs(fls));
  PRBool result = PR_FALSE;
  if (fls) {
    result = PR_TRUE;
  }
  return result;
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

PRBool
nsCSSFrameConstructor::ShouldCreateFirstLetterFrame(
  nsIPresContext* aPresContext,
  nsIContent*      aContent,
  nsIFrame*        aFrame)
{
  PRBool result = PR_FALSE;
  NS_PRECONDITION(aFrame, "null ptr");

  // See if the first frame isa text frame and if it is, that it has
  // some non-whitespace content.
  nsCOMPtr<nsIAtom> frameType;
  aFrame->GetFrameType(getter_AddRefs(frameType));
  if (frameType.get() == nsLayoutAtoms::textFrame) {
    nsITextContent* tc = nsnull;
    nsresult rv = aContent->QueryInterface(kITextContentIID, (void**) &tc);
    if (NS_SUCCEEDED(rv)) {
      tc->IsOnlyWhitespace(&result);
      result = !result;
      NS_RELEASE(tc);
    }
  }
  return result;
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
nsCSSFrameConstructor::ProcessChildren(nsIPresContext*          aPresContext,
                                       nsFrameConstructorState& aState,
                                       nsIContent*              aContent,
                                       nsIFrame*                aFrame,
                                       PRBool                   aCanHaveGeneratedContent,
                                       nsFrameItems&            aFrameItems,
                                       PRBool                   aParentIsBlock)
{
  nsresult rv = NS_OK;
  nsCOMPtr<nsIStyleContext> styleContext;

  PRBool processFirstLetterFrame = aState.mFirstLetterStyle;
  if (aCanHaveGeneratedContent) {
    // Probe for generated content before
    nsIFrame* generatedFrame;
    aFrame->GetStyleContext(getter_AddRefs(styleContext));
    if (CreateGeneratedContentFrame(aPresContext, aState, aFrame, aContent,
                                    styleContext, nsCSSAtoms::beforePseudo,
                                    processFirstLetterFrame,
                                    aParentIsBlock, &generatedFrame)) {
      // Add the generated frame to the child list
      aFrameItems.AddChild(generatedFrame);
      processFirstLetterFrame = PR_FALSE;
    }
  }

  PRBool whitespaceDoesntMatter = PR_FALSE;
  PRBool okToSkip = PR_FALSE;
  if (aParentIsBlock) {
    const nsStyleText* st;
    aFrame->GetStyleData(eStyleStruct_Text, (const nsStyleStruct*&) st);
    whitespaceDoesntMatter = !st->WhiteSpaceIsSignificant();
    okToSkip = whitespaceDoesntMatter;
  }

  // Iterate the child content objects and construct frames
  PRInt32   count;
  aContent->ChildCount(count);
  for (PRInt32 i = 0; i < count; i++) {
    nsCOMPtr<nsIContent> childContent;
    if (NS_SUCCEEDED(aContent->ChildAt(i, *getter_AddRefs(childContent)))) {
      if (aParentIsBlock) {
#if 0
        if (okToSkip && IsEmptyTextContent(childContent)) {
          continue;
        }
#endif
#if 0
        if (whitespaceDoesntMatter && IsEmptyHTMLParagraph(childContent)) {
          continue;
        }
#endif
      }

      // Construct a child frame
      okToSkip = PR_FALSE;
      rv = ConstructFrame(aPresContext, aState, childContent, aFrame, i == 0,
                          aFrameItems);
      if (NS_FAILED(rv)) {
        return rv;
      }

      // If last frame was a blockish type of frame, then its ok to
      // skip the next empty text content.
      if (aParentIsBlock && whitespaceDoesntMatter && aFrameItems.lastChild) {
        const nsStyleDisplay* sd;
        aFrameItems.lastChild->GetStyleData(eStyleStruct_Display,
                                            (const nsStyleStruct*&) sd);
        if (sd->IsBlockLevel()) {
          okToSkip = PR_TRUE;
        }
      }

      // Process first-letter frame that is the immediate child of the parent
      if ((aFrameItems.childList == aFrameItems.lastChild) &&
          processFirstLetterFrame && aFrameItems.childList &&
          ShouldCreateFirstLetterFrame(aPresContext, childContent,
                                       aFrameItems.childList)) {
        rv = WrapTextFrame(aPresContext, aState, aFrameItems.childList,
                           aContent, childContent, aFrame, aFrameItems,
                           aState.mFloatedItems, aParentIsBlock);
        processFirstLetterFrame = PR_FALSE;
      }
    }
  }

  if (aCanHaveGeneratedContent) {
    // Probe for generated content after
    nsIFrame* generatedFrame;
    if (CreateGeneratedContentFrame(aPresContext, aState, aFrame, aContent,
                                    styleContext, nsCSSAtoms::afterPseudo,
                                    processFirstLetterFrame && (0 == count),
                                    aParentIsBlock, &generatedFrame)) {
      // Add the generated frame to the child list
      aFrameItems.AddChild(generatedFrame);
    }
  }

  if (aParentIsBlock && aState.mFirstLineStyle) {
    rv = WrapFramesInFirstLineFrame(aPresContext, aState, aContent, aFrame,
                                    aFrameItems);
  }

  return rv;
}

//----------------------------------------------------------------------

// Support for :first-line style

// XXX this predicate and its cousins need to migrated to a single
// place in layout - something in nsStyleDisplay maybe?
static PRBool
IsInlineFrame(nsIFrame* aFrame)
{
  const nsStyleDisplay* display;
  aFrame->GetStyleData(eStyleStruct_Display, (const nsStyleStruct*&) display);
  switch (display->mDisplay) {
    case NS_STYLE_DISPLAY_INLINE:
    case NS_STYLE_DISPLAY_INLINE_BLOCK:
    case NS_STYLE_DISPLAY_INLINE_TABLE:
      return PR_TRUE;
    default:
      break;
  }
  return PR_FALSE;
}

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
  rv = NS_NewFirstLineFrame(&lineFrame);
  if (NS_SUCCEEDED(rv)) {
    // Initialize the line frame
    rv = lineFrame->Init(*aPresContext, aContent, aFrame,
                         firstLineStyle, nsnull);

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
    lineFrame->SetInitialChildList(*aPresContext, nsnull, firstInlineFrame);
  }

  return rv;
}

// Special routine to handle appending a new frame to a block frame's
// child list. Takes care of placing the new frame into the right
// place when first-line style is present.
nsresult
nsCSSFrameConstructor::AppendFirstLineFrames(
  nsIPresContext*          aPresContext,
  nsFrameConstructorState& aState,
  nsIContent*              aContent,
  nsIFrame*                aBlockFrame,
  nsFrameItems&            aFrameItems)
{
  // It's possible that aBlockFrame needs to have a first-line frame
  // created because it doesn't currently have any children.
  nsIFrame* blockKid;
  aBlockFrame->FirstChild(nsnull, &blockKid);
  if (!blockKid) {
    return WrapFramesInFirstLineFrame(aPresContext, aState, aContent,
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
  aState.mFrameManager->AppendFrames(*aPresContext, *aState.mPresShell,
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
        aState.mFrameManager->InsertFrames(*aPresContext, *aState.mPresShell,
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
          rv = lineFrame->Init(*aPresContext, aContent, aBlockFrame,
                               firstLineStyle, nsnull);

          // Make sure the caller inserts the lineFrame into the
          // blocks list of children.
          aFrameItems.childList = lineFrame;
          aFrameItems.lastChild = lineFrame;

          // Give the inline frames to the lineFrame <b>after</b>
          // reparenting them
          ReparentFrame(aPresContext, lineFrame, firstLineStyle, newFrame);
          lineFrame->SetInitialChildList(*aPresContext, nsnull, newFrame);
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
          aState.mFrameManager->InsertFrames(*aPresContext, *aState.mPresShell,
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

// Determine how many characters in the text fragment apply to the
// first letter
static PRInt32
FirstLetterCount(nsTextFragment* aFragments, PRInt32 aNumFragments)
{
  PRInt32 count = 0;
  PRInt32 firstLetterLength = 0;
  PRBool done = PR_FALSE;
  while (aNumFragments && !done) {
    PRInt32 i, n = aFragments->GetLength();
    for (i = 0; i < n; i++) {
      PRUnichar ch = aFragments->CharAt(i);
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
    aFragments++;
    aNumFragments--;
  }
  return count;
}

static PRInt32
TotalLength(nsTextFragment* aFragments, PRInt32 aNumFragments)
{
  PRInt32 sum = 0;
  while (--aNumFragments >= 0) {
    sum += aFragments->GetLength();
  }
  return sum;
}

static PRBool
NeedFirstLetterContinuation(nsIContent* aContent)
{
  NS_PRECONDITION(aContent, "null ptr");

  PRBool result = PR_FALSE;
  if (aContent) {
    nsCOMPtr<nsITextContent> tc(do_QueryInterface(aContent));
    if (tc) {
      nsTextFragment* frags = nsnull;
      PRInt32 numFrags = 0;
      tc->GetText((const nsTextFragment*&)frags, numFrags);
      PRInt32 flc = FirstLetterCount(frags, numFrags);
      PRInt32 tl = TotalLength(frags, numFrags);
      if (flc < tl) {
        result = PR_TRUE;
      }
    }
  }
  return result;
}

void
nsCSSFrameConstructor::CreateFloatingFirstLetterFrame(
  nsIPresContext* aPresContext,
  nsIFrameManager* aFrameManager,
  nsIFrame* aTextFrame,
  nsIContent* aContent,
  nsIContent* aChildContent,
  nsIFrame* aParentFrame,
  nsFrameItems& aFrameItems,
  nsFrameItems& aFloatingItems,
  nsIStyleContext* aStyleContext)
{
  // See if we will need to continue the text frame (does it contain
  // more than just the first-letter text or not?) If it does, then we
  // create (in advance) a continuation frame for it.
  nsIFrame* nextTextFrame = nsnull;
  if (NeedFirstLetterContinuation(aChildContent)) {
    // Create continuation
    CreateContinuingFrame(aPresContext, aTextFrame, aParentFrame,
                          &nextTextFrame);
  }


  // Create the first-letter-frame
  nsIFrame* firstLetterFrame;
  NS_NewFirstLetterFrame(&firstLetterFrame);
  firstLetterFrame->Init(*aPresContext, aContent, aParentFrame,
                         aStyleContext, nsnull);
  firstLetterFrame->SetInitialChildList(*aPresContext, nsnull, aTextFrame);
  aTextFrame->SetParent(firstLetterFrame);

  // Now make the placeholder
  nsIFrame* placeholderFrame;
  CreatePlaceholderFrameFor(aPresContext, aFrameManager, aContent,
                            firstLetterFrame, aStyleContext, aParentFrame,
                            &placeholderFrame);

  // Update the child lists for the frame containing the floating first
  // letter frame.
  aFloatingItems.AddChild(firstLetterFrame);
  aFrameItems.childList = placeholderFrame;
  aFrameItems.lastChild = placeholderFrame;
  if (nextTextFrame) {
    aFrameItems.AddChild(nextTextFrame);
  }
}

nsresult
nsCSSFrameConstructor::WrapTextFrame(nsIPresContext* aPresContext,
                                     nsFrameConstructorState& aState,
                                     nsIFrame* aTextFrame,
                                     nsIContent* aContent,
                                     nsIContent* aChildContent,
                                     nsIFrame* aParentFrame,
                                     nsFrameItems& aFrameItems,
                                     nsAbsoluteItems& aFloatingItems,
                                     PRBool aForBlock)
{
  // Get style context for the first-letter-frame
  nsIStyleContext* parentStyleContext;
  if (NS_SUCCEEDED(aParentFrame->GetStyleContext(&parentStyleContext)) &&
      parentStyleContext) {
    nsIContent* correctContent = aContent;
    if (!aForBlock) {
      // Use content from containing block so that we can actually
      // find a matching style rule.
      aFloatingItems.containingBlock->GetContent(&correctContent);
    }
    nsIStyleContext* sc = GetFirstLetterStyle(aPresContext, correctContent,
                                              parentStyleContext);
    if (sc) {
      const nsStyleDisplay* display = (const nsStyleDisplay*)
        sc->GetStyleData(eStyleStruct_Display);
      if (display->IsFloating()) {
        CreateFloatingFirstLetterFrame(aPresContext, aState.mFrameManager,
                                       aTextFrame, correctContent,
                                       aChildContent, aParentFrame,
                                       aFrameItems, aFloatingItems, sc);
      }
      else {
        nsIFrame* newFrame;
        nsresult rv = NS_NewFirstLetterFrame(&newFrame);
        if (NS_SUCCEEDED(rv)) {
          // Initialize the first-letter-frame.
          rv = newFrame->Init(*aPresContext, correctContent, aParentFrame, sc,
                              nsnull);
          newFrame->SetInitialChildList(*aPresContext, nsnull, aTextFrame);
          aTextFrame->SetParent(newFrame);

          // Replace the text frame in the flow child list with the
          // first-letter-frame
          aFrameItems.childList = newFrame;
          aFrameItems.lastChild = newFrame;
        }
      }

      // Once we have created a first-letter frame, don't create any more.
      aState.mFirstLetterStyle = PR_FALSE;

      // Make a note of the text-frame in the frame-managers map so
      // that it can be found later on (the letter-frame obscures it
      // from the normal search routines)
      aState.mFrameManager->SetPrimaryFrameFor(aChildContent, aTextFrame);

      NS_RELEASE(sc);
    }
    if (correctContent != aContent) {
      NS_RELEASE(correctContent);
    }
    NS_RELEASE(parentStyleContext);
  }
  return NS_OK;
}

// Tree Widget Routines
NS_IMETHODIMP
nsCSSFrameConstructor::CreateTreeWidgetContent(nsIPresContext* aPresContext,
                                               nsIFrame*       aParentFrame,
                                               nsIFrame*       aPrevFrame,
                                               nsIContent*     aChild,
                                               nsIFrame**      aNewFrame,
                                               PRBool          aIsAppend,
                                               PRBool          aIsScrollbar)
{
  nsCOMPtr<nsIPresShell> shell;
  aPresContext->GetShell(getter_AddRefs(shell));
  nsresult rv = NS_OK;

  // Construct a new frame
  if (nsnull != aParentFrame) {
    nsFrameItems            frameItems;
    nsFrameConstructorState state(aPresContext, mFixedContainingBlock,
                                  GetAbsoluteContainingBlock(aPresContext, aParentFrame),
                                  GetFloaterContainingBlock(aPresContext, aParentFrame));
    rv = ConstructFrame(aPresContext, state, aChild, aParentFrame, PR_FALSE,
                        frameItems);
    
    nsIFrame* newFrame = frameItems.childList;
    *aNewFrame = newFrame;

    if (NS_SUCCEEDED(rv) && (nsnull != newFrame)) {
      // Notify the parent frame
      if (aIsScrollbar)
        ((nsTreeRowGroupFrame*)aParentFrame)->SetScrollbarFrame(newFrame);
      else if (aIsAppend)
        rv = ((nsTreeRowGroupFrame*)aParentFrame)->TreeAppendFrames(newFrame);
      else
        rv = ((nsTreeRowGroupFrame*)aParentFrame)->TreeInsertFrames(aPrevFrame, newFrame);
        
      // If there are new absolutely positioned child frames, then notify
      // the parent
      // XXX We can't just assume these frames are being appended, we need to
      // determine where in the list they should be inserted...
      if (state.mAbsoluteItems.childList) {
        rv = state.mAbsoluteItems.containingBlock->AppendFrames(*aPresContext, *shell,
                                                         nsLayoutAtoms::absoluteList,
                                                         state.mAbsoluteItems.childList);
      }
      
      // If there are new fixed positioned child frames, then notify
      // the parent
      // XXX We can't just assume these frames are being appended, we need to
      // determine where in the list they should be inserted...
      if (state.mFixedItems.childList) {
        rv = state.mFixedItems.containingBlock->AppendFrames(*aPresContext, *shell,
                                                      nsLayoutAtoms::fixedList,
                                                      state.mFixedItems.childList);
      }
      
      // If there are new floating child frames, then notify
      // the parent
      // XXX We can't just assume these frames are being appended, we need to
      // determine where in the list they should be inserted...
      if (state.mFloatedItems.childList) {
        rv = state.mFloatedItems.containingBlock->AppendFrames(*aPresContext, *shell,
                                                    nsLayoutAtoms::floaterList,
                                                    state.mFloatedItems.childList);
      }
    }
  }

  return rv;
}


