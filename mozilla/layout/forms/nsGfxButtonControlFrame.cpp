/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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

#include "nsGfxButtonControlFrame.h"
#include "nsIButton.h"
#include "nsWidgetsCID.h"
#include "nsIFontMetrics.h"
#include "nsFormControlFrame.h"
#include "nsISupportsArray.h"
#include "nsINameSpaceManager.h"
#include "nsContentCID.h"
#ifdef ACCESSIBILITY
#include "nsIAccessibilityService.h"
#endif
#include "nsIServiceManager.h"
#include "nsIDOMNode.h"
#include "nsLayoutAtoms.h"
#include "nsReflowPath.h"
// MouseEvent suppression in PP
#include "nsGUIEvent.h"

static NS_DEFINE_CID(kTextNodeCID,   NS_TEXTNODE_CID);

// Saving PresState
#include "nsIPresState.h"
#include "nsIHTMLContent.h"

const nscoord kSuggestedNotSet = -1;

nsGfxButtonControlFrame::nsGfxButtonControlFrame()
{
  mSuggestedWidth  = kSuggestedNotSet;
  mSuggestedHeight = kSuggestedNotSet;
}

nsresult
NS_NewGfxButtonControlFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame)
{
  NS_PRECONDITION(aNewFrame, "null OUT ptr");
  if (nsnull == aNewFrame) {
    return NS_ERROR_NULL_POINTER;
  }
  nsGfxButtonControlFrame* it = new (aPresShell) nsGfxButtonControlFrame;
  if (!it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  *aNewFrame = it;
  return NS_OK;
}
      
NS_IMETHODIMP
nsGfxButtonControlFrame::GetFrameType(nsIAtom** aType) const
{
  NS_PRECONDITION(nsnull != aType, "null OUT parameter pointer");
  *aType = nsLayoutAtoms::gfxButtonControlFrame; 
  NS_ADDREF(*aType);
  return NS_OK;
}

// Special check for the browse button of a file input.
//
// Since this is actually type "NS_FORM_INPUT_BUTTON", we
// can't tell if it is part of a file input by the type.
// We'll return PR_TRUE if either:
// (a) type is NS_FORM_BROWSE or
// (b) type is NS_FORM_INPUT_BUTTON and our parent is a file input
PRBool
nsGfxButtonControlFrame::IsFileBrowseButton(PRInt32 type)
{
  PRBool rv = PR_FALSE;
  if (NS_FORM_BROWSE == type) {
    rv = PR_TRUE;
  }
  else if (NS_FORM_INPUT_BUTTON == type) {
  
    // Check to see if parent is a file input
    nsresult result;
    nsCOMPtr<nsIContent> parentContent;
    result = mContent->GetParent(*getter_AddRefs(parentContent));
    if (NS_SUCCEEDED(result) && parentContent) {
      nsCOMPtr<nsIAtom> atom;
      result = parentContent->GetTag(*getter_AddRefs(atom));
      if (NS_SUCCEEDED(result) && atom) {
        if (atom.get() == nsHTMLAtoms::input) {

          // It's an input, is it a file input?
          nsAutoString value;
          if (NS_CONTENT_ATTR_HAS_VALUE == parentContent->GetAttr(kNameSpaceID_None, nsHTMLAtoms::type, value)) {
            if (value.EqualsIgnoreCase("file")) {
              rv = PR_TRUE;
            }
          }
        }
      }
    }
  }
  return rv;
}

/*
 * FIXME: this ::GetIID() method has no meaning in life and should be
 * removed.
 * Pierre Phaneuf <pp@ludusdesign.com>
 */
const nsIID&
nsGfxButtonControlFrame::GetIID()
{
  return NS_GET_IID(nsIButton);
}
  
const nsIID&
nsGfxButtonControlFrame::GetCID()
{
  static NS_DEFINE_IID(kButtonCID, NS_BUTTON_CID);
  return kButtonCID;
}

#ifdef DEBUG
NS_IMETHODIMP
nsGfxButtonControlFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("ButtonControl"), aResult);
}
#endif

NS_IMETHODIMP 
nsGfxButtonControlFrame::AddComputedBorderPaddingToDesiredSize(nsHTMLReflowMetrics& aDesiredSize,
                                                               const nsHTMLReflowState& aSuggestedReflowState)
{
  if (kSuggestedNotSet == mSuggestedWidth) {
    aDesiredSize.width  += aSuggestedReflowState.mComputedBorderPadding.left + aSuggestedReflowState.mComputedBorderPadding.right;
  }

  if (kSuggestedNotSet == mSuggestedHeight) {
    aDesiredSize.height += aSuggestedReflowState.mComputedBorderPadding.top + aSuggestedReflowState.mComputedBorderPadding.bottom;
  }
  return NS_OK;
}

nsresult 
nsGfxButtonControlFrame::DoNavQuirksReflow(nsIPresContext*          aPresContext, 
                                           nsHTMLReflowMetrics&     aDesiredSize,
                                           const nsHTMLReflowState& aReflowState, 
                                           nsReflowStatus&          aStatus)
{
  nsIFrame* firstKid = mFrames.FirstChild();

  nsCOMPtr<nsIFontMetrics> fontMet;
  nsresult res = nsFormControlHelper::GetFrameFontFM(aPresContext, (nsIFormControlFrame *)this, getter_AddRefs(fontMet));
  nsSize desiredSize;
  if (NS_SUCCEEDED(res) && fontMet) {
    aReflowState.rendContext->SetFont(fontMet);

    // Get the text from the "value" attribute 
    // for measuring the height, width of the text
    nsAutoString value;
    res = GetValue(&value);

    if (res != NS_CONTENT_ATTR_HAS_VALUE && value.IsEmpty()) {
      // Generate localized label.
      // We can't make any assumption as to what the default would be
      // because the value is localized for non-english platforms, thus
      // it might not be the string "Reset", "Submit Query", or "Browse..."
      res = GetDefaultLabel(value);
      if (NS_FAILED(res)) {
        return res;
      }
    }

    const nsStyleText* textStyle;
    GetStyleData(eStyleStruct_Text,  (const nsStyleStruct *&)textStyle);
    if (!textStyle->WhiteSpaceIsSignificant()) {
      value.CompressWhitespace();
    }
    
    if (value.IsEmpty()) {
      // Have to have _something_ or we won't be drawn
      value.Assign(NS_LITERAL_STRING("  "));
    }

    CalcNavQuirkSizing(aPresContext, aReflowState.rendContext, value,
                       desiredSize);

    // Note: The Quirks sizing includes a 2px border in its calculation of "desiredSize"
    // So we must subtract off the 2 pixel border.
    // Quirk sizing also assumes a 0px padding
    float   p2t;
    aPresContext->GetPixelsToTwips(&p2t);
    nscoord borderTwips = NSIntPixelsToTwips(4, p2t);
    desiredSize.width  -= borderTwips;
    desiredSize.height -= borderTwips;

    // Now figure out how much vertical padding was added and 
    // then subtract it, so we can add in the padding later
    // horizontal pading is 0px;
    nscoord hgt;
    fontMet->GetHeight(hgt);
    desiredSize.height -= PR_MAX(0, desiredSize.height - hgt);

    // This calculates the reflow size
    // get the css size and let the frame use or override it
    nsSize styleSize;
    nsFormControlFrame::GetStyleSize(aPresContext, aReflowState, styleSize);

    if (CSS_NOTSET != styleSize.width) {  // css provides width
      NS_ASSERTION(styleSize.width+aReflowState.mComputedBorderPadding.left + aReflowState.mComputedBorderPadding.right >= 0, "form control's computed width is < 0"); 
      if (NS_INTRINSICSIZE != styleSize.width) {
        desiredSize.width = styleSize.width;
        desiredSize.width  += aReflowState.mComputedBorderPadding.left + aReflowState.mComputedBorderPadding.right;
      }
    } else {
      desiredSize.width  += aReflowState.mComputedBorderPadding.left + aReflowState.mComputedBorderPadding.right;
    }

    if (CSS_NOTSET != styleSize.height) {  // css provides height
      NS_ASSERTION(styleSize.height > 0, "form control's computed height is <= 0"); 
      if (NS_INTRINSICSIZE != styleSize.height) {
        desiredSize.height = styleSize.height;
        desiredSize.height += aReflowState.mComputedBorderPadding.top + aReflowState.mComputedBorderPadding.bottom;
      }
    } else {
      desiredSize.height += aReflowState.mComputedBorderPadding.top + aReflowState.mComputedBorderPadding.bottom;
    }

    aDesiredSize.width   = desiredSize.width;
    aDesiredSize.height  = desiredSize.height;
  } else {
    // XXX ASSERT HERE
    desiredSize.width = 0;
    desiredSize.height = 0;
  }

  // remove it from the the desired size
  // because the content need to fit inside of it
  desiredSize.width  -= (aReflowState.mComputedBorderPadding.left + aReflowState.mComputedBorderPadding.right);
  desiredSize.height -= (aReflowState.mComputedBorderPadding.top + aReflowState.mComputedBorderPadding.bottom);

  // Ok, now we think we know what size we are so we can reflow our contents
  // But we need to make sure we aren't smaller or larger then the min/max
  if (desiredSize.width < aReflowState.mComputedMinWidth) {
    desiredSize.width = aReflowState.mComputedMinWidth - (aReflowState.mComputedBorderPadding.left + aReflowState.mComputedBorderPadding.right);
  } else if (desiredSize.width > aReflowState.mComputedMaxWidth) {
    desiredSize.width = aReflowState.mComputedMaxWidth - (aReflowState.mComputedBorderPadding.left + aReflowState.mComputedBorderPadding.right);
  }

  if (desiredSize.height < aReflowState.mComputedMinHeight) {
    desiredSize.height = aReflowState.mComputedMinHeight - (aReflowState.mComputedBorderPadding.top + aReflowState.mComputedBorderPadding.bottom);
  } else if (desiredSize.height > aReflowState.mComputedMaxHeight) {
    desiredSize.height = aReflowState.mComputedMaxHeight - (aReflowState.mComputedBorderPadding.top + aReflowState.mComputedBorderPadding.bottom);
  }

  // XXX Proper handling of incremental reflow...
  nsReflowReason reason = aReflowState.reason;
  if (eReflowReason_Incremental == reason) {
    // See if it's targeted at us
    nsHTMLReflowCommand *command = aReflowState.path->mReflowCommand;

    if (command) {
      Invalidate(aPresContext, nsRect(0,0,mRect.width,mRect.height), PR_FALSE);

      nsReflowType  reflowType;
      command->GetType(reflowType);
      if (eReflowType_StyleChanged == reflowType) {
        reason = eReflowReason_StyleChange;
      }
      else {
        reason = eReflowReason_Resize;
      }
    }
  }

  // now reflow the first child (generated content)
  nsHTMLReflowState reflowState(aPresContext, aReflowState, firstKid, desiredSize, reason);
  reflowState.mComputedWidth  = desiredSize.width;
  reflowState.mComputedHeight = desiredSize.height;

  nsHTMLReflowMetrics childReflowMetrics(aDesiredSize);
  nsRect kidRect;
  firstKid->GetRect(kidRect);
  ReflowChild(firstKid, aPresContext, childReflowMetrics, reflowState, kidRect.x, kidRect.y, 0, aStatus);

  aDesiredSize.ascent = childReflowMetrics.ascent +
    aReflowState.mComputedBorderPadding.top;
  aDesiredSize.descent = aDesiredSize.height - aDesiredSize.ascent;

  // Center the child and add back in the border and badding
  // our inner area frame is already doing centering so we only need to center vertically.
  nsRect rect = nsRect(aReflowState.mComputedBorderPadding.left, 
                       aReflowState.mComputedBorderPadding.top, 
                       desiredSize.width, 
                       desiredSize.height);
  firstKid->SetRect(aPresContext, rect);

  return NS_OK;
}

// Create the text content used as label for the button.
// The frame will be generated by the frame constructor.
NS_IMETHODIMP
nsGfxButtonControlFrame::CreateAnonymousContent(nsIPresContext* aPresContext,
                                                nsISupportsArray& aChildList)
{
  nsresult result;

  // Get the text from the "value" attribute.
  // If it is zero length, set it to a default value (localized)
  nsAutoString initvalue, value;
  result = GetValue(&initvalue);
  value = initvalue;
  if (result != NS_CONTENT_ATTR_HAS_VALUE && value.IsEmpty()) {
    // Generate localized label.
    // We can't make any assumption as to what the default would be
    // because the value is localized for non-english platforms, thus
    // it might not be the string "Reset", "Submit Query", or "Browse..."
    result = GetDefaultLabel(value);
    if (NS_FAILED(result)) {
      return result;
    }
  }

  // Compress whitespace out of label if needed.
  const nsStyleText* textStyle;
  GetStyleData(eStyleStruct_Text,  (const nsStyleStruct *&)textStyle);
  if (!textStyle->WhiteSpaceIsSignificant()) {
    value.CompressWhitespace();
  }

  if (value.IsEmpty()) {
    // Have to have _something_ or we won't be drawn
    value.Assign(NS_LITERAL_STRING("  "));
  }

  // Add a child text content node for the label
  nsCOMPtr<nsIContent> labelContent(do_CreateInstance(kTextNodeCID,&result));
  if (NS_SUCCEEDED(result) && labelContent) {
    // set the value of the text node and add it to the child list
    mTextContent = do_QueryInterface(labelContent, &result);
    if (NS_SUCCEEDED(result) && mTextContent) {
      mTextContent->SetText(value.get(), value.Length(), PR_TRUE);
      aChildList.AppendElement(mTextContent);
    }
  }
  return result;
}

// Create the text content used as label for the button.
// The frame will be generated by the frame constructor.
NS_IMETHODIMP
nsGfxButtonControlFrame::CreateFrameFor(nsIPresContext*   aPresContext,
                                        nsIContent *      aContent,
                                        nsIFrame**        aFrame)
{
  nsIFrame * newFrame = nsnull;
  nsresult rv = NS_ERROR_FAILURE;

  if (aFrame) 
    *aFrame = nsnull; 

  nsCOMPtr<nsIContent> content(do_QueryInterface(mTextContent));
  if (aContent == content.get()) {
    nsCOMPtr<nsIPresShell> shell;
    aPresContext->GetShell(getter_AddRefs(shell));

    nsIFrame * parentFrame = mFrames.FirstChild();
    nsCOMPtr<nsIStyleContext> styleContext;
    parentFrame->GetStyleContext(getter_AddRefs(styleContext));

    rv = NS_NewTextFrame(shell, &newFrame);
    if (NS_FAILED(rv)) { return rv; }
    if (!newFrame)   { return NS_ERROR_NULL_POINTER; }
    nsCOMPtr<nsIStyleContext> textStyleContext;
    rv = aPresContext->ResolveStyleContextForNonElement(
                                             styleContext,
                                             getter_AddRefs(textStyleContext));
    if (NS_FAILED(rv))     { return rv; }
    if (!textStyleContext) { return NS_ERROR_NULL_POINTER; }

    if (styleContext) {
      // initialize the text frame
      newFrame->Init(aPresContext, content, parentFrame, textStyleContext, nsnull);
      newFrame->SetInitialChildList(aPresContext, nsnull, nsnull);
      rv = NS_OK;
    }
  }

  if (aFrame) {
    *aFrame = newFrame;
  }
  return rv;
}

#ifdef ACCESSIBILITY
NS_IMETHODIMP nsGfxButtonControlFrame::GetAccessible(nsIAccessible** aAccessible)
{
  nsCOMPtr<nsIAccessibilityService> accService = do_GetService("@mozilla.org/accessibilityService;1");

  if (accService) {
    return accService->CreateHTMLButtonAccessible(NS_STATIC_CAST(nsIFrame*, this), aAccessible);
  }

  return NS_ERROR_FAILURE;
}
#endif

// Frames are not refcounted, no need to AddRef
NS_IMETHODIMP
nsGfxButtonControlFrame::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{
  NS_ENSURE_ARG_POINTER(aInstancePtr);

  if (aIID.Equals(NS_GET_IID(nsIAnonymousContentCreator))) {
    *aInstancePtr = NS_STATIC_CAST(nsIAnonymousContentCreator*, this);
  }
else {
    return nsHTMLButtonControlFrame::QueryInterface(aIID, aInstancePtr);
  }

  return NS_OK;
}

// Initially we hardcoded the default strings here.
// Next, we used html.css to store the default label for various types
// of buttons. (nsGfxButtonControlFrame::DoNavQuirksReflow rev 1.20)
// However, since html.css is not internationalized, we now grab the default
// label from a string bundle as is done for all other UI strings.
// See bug 16999 for further details.
nsresult
nsGfxButtonControlFrame::GetDefaultLabel(nsString& aString) 
{
  const char * propname = nsFormControlHelper::GetHTMLPropertiesFileName();
  nsresult rv = NS_OK;
  PRInt32 type;
  GetType(&type);
  if (type == NS_FORM_INPUT_RESET) {
    rv = nsFormControlHelper::GetLocalizedString(propname, NS_LITERAL_STRING("Reset").get(), aString);
  } 
  else if (type == NS_FORM_INPUT_SUBMIT) {
    rv = nsFormControlHelper::GetLocalizedString(propname, NS_LITERAL_STRING("Submit").get(), aString);
  } 
  else if (IsFileBrowseButton(type)) {
    rv = nsFormControlHelper::GetLocalizedString(propname, NS_LITERAL_STRING("Browse").get(), aString);
  }
  else {
    aString.Assign(NS_LITERAL_STRING("  "));
    rv = NS_OK;
  }
  return rv;
}

NS_IMETHODIMP
nsGfxButtonControlFrame::AttributeChanged(nsIPresContext* aPresContext,
                                       nsIContent*     aChild,
                                       PRInt32         aNameSpaceID,
                                       nsIAtom*        aAttribute,
                                       PRInt32         aModType, 
                                       PRInt32         aHint)
{
  nsresult rv = NS_OK;

  // If the value attribute is set, update the text of the label
  if (nsHTMLAtoms::value == aAttribute) {
    nsAutoString value;
    if (mTextContent && mContent) {
      if (NS_CONTENT_ATTR_HAS_VALUE != mContent->GetAttr(kNameSpaceID_None, nsHTMLAtoms::value, value)) {
        value.Truncate();
      }
      rv = mTextContent->SetText(value.get(), value.Length(), PR_TRUE);
    } else {
      rv = NS_ERROR_UNEXPECTED;
    }

  // defer to HTMLButtonControlFrame
  } else {
    rv = nsHTMLButtonControlFrame::AttributeChanged(aPresContext, aChild, aNameSpaceID, aAttribute, aModType, aHint);
  }
  return rv;
}

NS_IMETHODIMP 
nsGfxButtonControlFrame::Reflow(nsIPresContext*          aPresContext, 
                                nsHTMLReflowMetrics&     aDesiredSize,
                                const nsHTMLReflowState& aReflowState, 
                                nsReflowStatus&          aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsGfxButtonControlFrame", aReflowState.reason);
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aDesiredSize, aStatus);

  nsresult rv = NS_OK;

#if 0
  nsresult skiprv = nsFormControlFrame::SkipResizeReflow(mCacheSize, mCachedMaxElementSize, aPresContext, 
                                                         aDesiredSize, aReflowState, aStatus);

  if (NS_SUCCEEDED(skiprv)) {
    return skiprv;
  }
#endif

  if ((kSuggestedNotSet != mSuggestedWidth) || 
      (kSuggestedNotSet != mSuggestedHeight)) {
    nsHTMLReflowState suggestedReflowState(aReflowState);

      // Honor the suggested width and/or height.
    if (kSuggestedNotSet != mSuggestedWidth) {
      suggestedReflowState.mComputedWidth = mSuggestedWidth;
    }

    if (kSuggestedNotSet != mSuggestedHeight) {
      suggestedReflowState.mComputedHeight = mSuggestedHeight;
    }

    rv = nsHTMLButtonControlFrame::Reflow(aPresContext, aDesiredSize, suggestedReflowState, aStatus);

  } else { // Normal reflow.

#if 1
    // nsHTMLButtonControlFrame::Reflow registers it for Standard Mode
    // and sets up mPresContext
    if (eReflowReason_Initial == aReflowState.reason) {
      nsFormControlFrame::RegUnRegAccessKey(aPresContext, NS_STATIC_CAST(nsIFrame*, this), PR_TRUE);
    }
    // Do NavQuirks Sizing and layout
    rv = DoNavQuirksReflow(aPresContext, aDesiredSize, aReflowState, aStatus); 
    
    // Make sure we obey min/max-width and min/max-height
    if (aDesiredSize.width > aReflowState.mComputedMaxWidth) {
      aDesiredSize.width = aReflowState.mComputedMaxWidth;
    }
    if (aDesiredSize.width < aReflowState.mComputedMinWidth) {
      aDesiredSize.width = aReflowState.mComputedMinWidth;
    } 

    if (aDesiredSize.height > aReflowState.mComputedMaxHeight) {
      aDesiredSize.height = aReflowState.mComputedMaxHeight;
    }
    if (aDesiredSize.height < aReflowState.mComputedMinHeight) {
      aDesiredSize.height = aReflowState.mComputedMinHeight;
    } 
#else
    // Do Standard mode sizing and layout
    rv = nsHTMLButtonControlFrame::Reflow(aPresContext, aDesiredSize, aReflowState, aStatus);
#endif
  }

#ifdef DEBUG_rodsXXXX
  COMPARE_QUIRK_SIZE("nsGfxButtonControlFrame", 84, 24) // with the text "Press Me" in it
#endif
  aStatus = NS_FRAME_COMPLETE;

  nsFormControlFrame::SetupCachedSizes(mCacheSize, mCachedMaxElementSize, aDesiredSize);

  if (aDesiredSize.maxElementSize != nsnull) {
    aDesiredSize.maxElementSize->width  = aDesiredSize.width;
    aDesiredSize.maxElementSize->height = aDesiredSize.height;
  }

  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
  return rv;
}

void 
nsGfxButtonControlFrame::CalcNavQuirkSizing(nsIPresContext* aPresContext, 
                                            nsIRenderingContext* aRendContext,
                                            nsString&       aLabel,
                                            nsSize&         aSize)
{
  float p2t;
  float t2p;
  aPresContext->GetPixelsToTwips(&p2t);
  aPresContext->GetTwipsToPixels(&t2p);

  // Get text size, round to nearest pixel, multiply by 3/2
  // XXX this algorithm seems suspect: rounding before multiply can't be good,
  //     and 3/2 seems ... arbitrary
  nsFormControlHelper::GetTextSize(aPresContext, this,
                                   aLabel, aSize,
                                   aRendContext);
  aSize.width  = NSToCoordRound(aSize.width * t2p);
  aSize.height = NSToCoordRound(aSize.height * t2p);
  aSize.width  = NSIntPixelsToTwips(3 * aSize.width / 2, p2t);
  aSize.height = NSIntPixelsToTwips(3 * aSize.height / 2, p2t);
}

NS_IMETHODIMP 
nsGfxButtonControlFrame::SetSuggestedSize(nscoord aWidth, nscoord aHeight)
{
  mSuggestedWidth = aWidth;
  mSuggestedHeight = aHeight;
  //mState |= NS_FRAME_IS_DIRTY;
  return NS_OK;
}

NS_IMETHODIMP
nsGfxButtonControlFrame::HandleEvent(nsIPresContext* aPresContext, 
                                      nsGUIEvent*     aEvent,
                                      nsEventStatus*  aEventStatus)
{
  // temp fix until Bug 124990 gets fixed
  PRBool isPaginated = PR_FALSE;
  aPresContext->IsPaginated(&isPaginated);
  if (isPaginated && NS_IS_MOUSE_EVENT(aEvent)) {
    return NS_OK;
  }
  // Override the HandleEvent to prevent the nsFrame::HandleEvent
  // from being called. The nsFrame::HandleEvent causes the button label
  // to be selected (Drawn with an XOR rectangle over the label)
 
  // Do nothing here, nsHTMLInputElement::HandleDOMEvent
  // takes cares of calling MouseClicked for us.

  // do we have user-input style?
  const nsStyleUserInterface* uiStyle;
  GetStyleData(eStyleStruct_UserInterface,  (const nsStyleStruct *&)uiStyle);
  if (uiStyle->mUserInput == NS_STYLE_USER_INPUT_NONE || uiStyle->mUserInput == NS_STYLE_USER_INPUT_DISABLED)
    return nsFrame::HandleEvent(aPresContext, aEvent, aEventStatus);
  
  return NS_OK;
}
