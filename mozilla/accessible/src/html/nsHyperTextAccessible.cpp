/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developers of the Original Code are
 * Sun Microsystems and IBM Corporation
 * Portions created by the Initial Developer are Copyright (C) 2006
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Ginn Chen (ginn.chen@sun.com)
 *   Aaron Leventhal (aleventh@us.ibm.com)
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "nsHyperTextAccessible.h"
#include "nsAccessibilityAtoms.h"
#include "nsAccessibilityService.h"
#include "nsAccessibleTreeWalker.h"
#include "nsPIAccessNode.h"
#include "nsIClipboard.h"
#include "nsContentCID.h"
#include "nsIDOMAbstractView.h"
#include "nsIDOMCharacterData.h"
#include "nsIDOMDocument.h"
#include "nsIDOMDocumentView.h"
#include "nsIDOMRange.h"
#include "nsIDOMWindowInternal.h"
#include "nsIDOMXULDocument.h"
#include "nsIFontMetrics.h"
#include "nsIFrame.h"
#include "nsIPlaintextEditor.h"
#include "nsIServiceManager.h"
#include "nsTextFragment.h"
#include "nsIPersistentProperties2.h"

static NS_DEFINE_IID(kRangeCID, NS_RANGE_CID);

// ------------
// nsHyperTextAccessible
// ------------

NS_IMPL_ADDREF_INHERITED(nsHyperTextAccessible, nsAccessible)
NS_IMPL_RELEASE_INHERITED(nsHyperTextAccessible, nsAccessible)

nsresult nsHyperTextAccessible::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  *aInstancePtr = nsnull;

  nsCOMPtr<nsIDOMXULDocument> xulDoc(do_QueryInterface(mDOMNode));
  if (mDOMNode && !xulDoc) {
    // We need XUL doc check for now because for now nsDocAccessible must inherit from nsHyperTextAccessible
    // in order for HTML document accessibles to get support for these interfaces
    // However at some point we may push <body> to implement the interfaces and
    // return nsDocAccessible to inherit from nsAccessibleWrap
    if (aIID.Equals(NS_GET_IID(nsIAccessibleText))) {
      // If |this| contains any children
      PRInt32 numChildren;
      GetChildCount(&numChildren);
      if (numChildren > 0) {
        *aInstancePtr = NS_STATIC_CAST(nsIAccessibleText*, this);
        NS_ADDREF_THIS();
        return NS_OK;
      }
      return NS_ERROR_NO_INTERFACE;
    }

    if (aIID.Equals(NS_GET_IID(nsIAccessibleHyperText))) {
      if (IsHyperText()) {
        // If |this| contains text and embedded objects
        *aInstancePtr = NS_STATIC_CAST(nsIAccessibleHyperText*, this);
        NS_ADDREF_THIS();
        return NS_OK;
      }
      return NS_ERROR_NO_INTERFACE;
    }

    if (aIID.Equals(NS_GET_IID(nsIAccessibleEditableText))) {
      // If this contains editable text
      PRUint32 extState;
      GetExtState(&extState);
      if (extState & EXT_STATE_EDITABLE) {
        *aInstancePtr = NS_STATIC_CAST(nsIAccessibleEditableText*, this);
        NS_ADDREF_THIS();
        return NS_OK;
      }
      return NS_ERROR_NO_INTERFACE;
    }
  }

  return nsAccessible::QueryInterface(aIID, aInstancePtr);
}

nsHyperTextAccessible::nsHyperTextAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell):
nsAccessibleWrap(aNode, aShell)
{
}

PRBool nsHyperTextAccessible::IsHyperText()
{
  nsCOMPtr<nsIAccessible> accessible;
  while (NextChild(accessible)) {
    if (IsEmbeddedObject(accessible)) {
      return PR_TRUE;
    }
  }

  return PR_FALSE;
}

NS_IMETHODIMP nsHyperTextAccessible::GetRole(PRUint32 *aRole)
{
  nsCOMPtr<nsIContent> content = do_QueryInterface(mDOMNode);
  if (!content) {
    return NS_ERROR_FAILURE;
  }

  nsIAtom *tag = content->Tag();

  if (tag == nsAccessibilityAtoms::caption) {
    *aRole = ROLE_CAPTION;
  }
  else if (tag == nsAccessibilityAtoms::form) {
    *aRole = ROLE_FORM;
  }
  else if (tag == nsAccessibilityAtoms::div ||
           tag == nsAccessibilityAtoms::blockquote) {
    *aRole = ROLE_SECTION;
  }
  else if (tag == nsAccessibilityAtoms::h1 ||
           tag == nsAccessibilityAtoms::h2 ||
           tag == nsAccessibilityAtoms::h3 ||
           tag == nsAccessibilityAtoms::h4 ||
           tag == nsAccessibilityAtoms::h5 ||
           tag == nsAccessibilityAtoms::h6) {
    *aRole = ROLE_HEADING;
  }
  else {
    nsIFrame *frame = GetFrame();
    if (frame && frame->GetType() == nsAccessibilityAtoms::blockFrame) {
      *aRole = ROLE_PARAGRAPH;
    }
    else {
      *aRole = ROLE_TEXT_CONTAINER; // In ATK this works
    }
  }
  return NS_OK;
}

NS_IMETHODIMP nsHyperTextAccessible::GetExtState(PRUint32 *aExtState)
{
  *aExtState = 0;
  if (!mDOMNode) {
    return NS_ERROR_FAILURE; // Node is shut down
  }

  nsresult rv = nsAccessibleWrap::GetExtState(aExtState);
  nsCOMPtr<nsIEditor> editor = GetEditor();
  if (editor) {
    PRUint32 flags;
    editor->GetFlags(&flags);
    if (0 == (flags & nsIPlaintextEditor::eEditorReadonlyMask)) {
      *aExtState |= EXT_STATE_EDITABLE;
    }
  }

  PRInt32 childCount;
  GetChildCount(&childCount);
  if (childCount > 0) {
    *aExtState |= EXT_STATE_SELECTABLE_TEXT;
  }
  return rv;
}

void nsHyperTextAccessible::CacheChildren()
{
  if (!mWeakShell) {
    // This node has been shut down
    mAccChildCount = eChildCountUninitialized;
    return;
  }

  if (mAccChildCount == eChildCountUninitialized) {
    nsCOMPtr<nsIEditor> editor = GetEditor();
    if (!editor) {
      nsAccessible::CacheChildren();
      return;
    }
    nsCOMPtr<nsIDOMElement> editorRoot;
    editor->GetRootElement(getter_AddRefs(editorRoot));
    nsCOMPtr<nsIDOMNode> editorRootDOMNode = do_QueryInterface(editorRoot);
    if (!editorRootDOMNode) {
      return;
    }
    nsAccessibleTreeWalker walker(mWeakShell, editorRootDOMNode, PR_TRUE);
    nsCOMPtr<nsPIAccessible> privatePrevAccessible;
    PRInt32 childCount = 0;
    walker.GetFirstChild();
    SetFirstChild(walker.mState.accessible);

    while (walker.mState.accessible) {
      ++ childCount;
      privatePrevAccessible = do_QueryInterface(walker.mState.accessible);
      privatePrevAccessible->SetParent(this);
      walker.GetNextSibling();
      privatePrevAccessible->SetNextSibling(walker.mState.accessible);
    }
    mAccChildCount = childCount;
  }
}

// Substring must be entirely within the same text node
nsIntRect nsHyperTextAccessible::GetBoundsForString(nsIFrame *aFrame, PRInt32 aStartOffset, PRInt32 aLength)
{
  nsIntRect screenRect;
  nsIFrame *frame;
  PRInt32 startOffsetInFrame;
  nsresult rv = aFrame->GetChildFrameContainingOffset(aStartOffset, PR_FALSE,
                                                      &startOffsetInFrame, &frame);
  NS_ENSURE_SUCCESS(rv, screenRect);

  nsCOMPtr<nsIPresShell> shell = GetPresShell();
  NS_ENSURE_TRUE(shell, screenRect);

  nsCOMPtr<nsIRenderingContext> rc;
  shell->CreateRenderingContext(frame, getter_AddRefs(rc));
  NS_ENSURE_TRUE(rc, screenRect);

  const nsStyleFont *font = frame->GetStyleFont();
  const nsStyleVisibility *visibility = frame->GetStyleVisibility();

  rv = rc->SetFont(font->mFont, visibility->mLangGroup);
  NS_ENSURE_SUCCESS(rv, screenRect);

  nsPresContext *context = shell->GetPresContext();
  float t2p = context->TwipsToPixels();

  while (frame && aLength > 0) {
    // Start with this frame's screen rect, which we will 
    // shrink based on the substring we care about within it.
    // We will then add that frame to the total screenRect we
    // are returning.
    nsIntRect frameScreenRect = frame->GetScreenRectExternal();

    // Get the length of the substring in this frame that we want the bounds for
    PRInt32 startFrameTextOffset, endFrameTextOffset;
    frame->GetOffsets(startFrameTextOffset, endFrameTextOffset);
    PRInt32 frameTotalTextLength = endFrameTextOffset - startFrameTextOffset;
    PRInt32 frameSubStringLength = PR_MIN(frameTotalTextLength - startOffsetInFrame, aLength);

    // Add the point where the string starts to the frameScreenRect
    nsPoint frameTextStartPoint;
    rv = frame->GetPointFromOffset(context, rc, aStartOffset, &frameTextStartPoint);
    NS_ENSURE_SUCCESS(rv, nsRect());   
    frameScreenRect.x += NSTwipsToIntPixels(frameTextStartPoint.x, t2p);

    // Use the point for the end offset to calculate the width
    nsPoint frameTextEndPoint;
    rv = frame->GetPointFromOffset(context, rc, aStartOffset + frameSubStringLength, &frameTextEndPoint);
    NS_ENSURE_SUCCESS(rv, nsRect());   
    frameScreenRect.width = NSTwipsToIntPixels(frameTextEndPoint.x - frameTextStartPoint.x, t2p);

    screenRect.UnionRect(frameScreenRect, screenRect);

    // Get ready to loop back for next frame continuation
    aStartOffset += frameSubStringLength;
    startOffsetInFrame = 0;
    aLength -= frameSubStringLength;
    frame = frame->GetNextContinuation();
  }

  return screenRect;
}

/*
 * Gets the specified text.
 */
nsIFrame* nsHyperTextAccessible::GetPosAndText(PRInt32& aStartOffset, PRInt32& aEndOffset, nsAString *aText,
                                               nsIFrame **aEndFrame, nsIntRect *aBoundsRect)
{
  PRInt32 startOffset = aStartOffset;
  PRInt32 endOffset = aEndOffset;

  // Clear out parameters and set up loop
  if (aText) {
    aText->Truncate();
  }
  if (endOffset < 0) {
    const PRInt32 kMaxTextLength = 32767;
    endOffset = kMaxTextLength; // Max end offset
  }
  else if (startOffset > endOffset) {
    return nsnull;
  }

  nsIFrame *startFrame = nsnull;
  if (aEndFrame) {
    *aEndFrame = nsnull;
  }
  if (aBoundsRect) {
    aBoundsRect->Empty();
  }

  nsIntRect unionRect;
  nsCOMPtr<nsIAccessible> accessible;

  // Loop through children and collect valid offsets, text and bounds
  // depending on what we need for out parameters
  while (NextChild(accessible)) {
    nsCOMPtr<nsPIAccessNode> accessNode(do_QueryInterface(accessible));
    nsIFrame *frame = accessNode->GetFrame();
    if (!frame) {
      continue;
    }
    if (IsText(accessible)) {
      // Avoid string copies
      PRInt32 substringEndOffset = frame->GetContent()->TextLength();
      nsAutoString newText;
      if (!substringEndOffset) {
        // This is exception to the frame owns the text.
        // The only known case where this occurs is for list bullets
        // We could do this for all accessibles but it's not as performant
        // as dealing with nsIContent directly
        accessible->GetName(newText);
        substringEndOffset = newText.Length();
      }
      if (startOffset < substringEndOffset) {
        // Our start is within this substring
        // XXX Can we somehow optimize further by getting the nsTextFragment
        // and use CopyTo to a PRUnichar buffer to copy it directly to
        // the string?
        if (newText.IsEmpty()) { // Don't have text yet
          frame->GetContent()->AppendTextTo(newText);
        }
        if (startOffset > 0 || endOffset < substringEndOffset) {
          // XXX the Substring operation is efficient, but does the 
          // reassignment to the original nsAutoString cause a copy?
          PRInt32 outStartLineUnused;
          frame->GetChildFrameContainingOffset(startOffset, PR_TRUE, &outStartLineUnused, &frame);
          if (endOffset < substringEndOffset) {
            // Don't take entire substring: stop before the end
            substringEndOffset = endOffset;
          }
          if (aText) {
            newText = Substring(newText, startOffset,
                                substringEndOffset - startOffset);
          }
          if (aEndFrame) {
            *aEndFrame = frame; // We ended in the current frame
          }
          aEndOffset = endOffset;
        }
        if (aText) {
          if (!frame->GetStyleText()->WhiteSpaceIsSignificant()) {
            // Replace \r\n\t in markup with space unless in this is
            // preformatted text  where those characters are significant
            newText.ReplaceChar("\r\n\t", ' ');
          }
          *aText += newText;
        }
        if (aBoundsRect) {
          aBoundsRect->UnionRect(*aBoundsRect, GetBoundsForString(frame, startOffset,
                                                                  substringEndOffset - startOffset));
        }
        if (!startFrame) {
          startFrame = frame;
          aStartOffset = startOffset;
        }
        startOffset = 0;
      }
      else {
        startOffset -= substringEndOffset;
      }
      endOffset -= substringEndOffset;
    }
    else {
      // Embedded object, append marker
      // XXX Append \n for <br>'s
      if (startOffset >= 1) {
        -- startOffset;
      }
      else {
        if (endOffset > 0) {
          if (aText) {
            *aText += (frame->GetType() == nsAccessibilityAtoms::brFrame) ?
                      kForcedNewLineChar : kEmbeddedObjectChar;
          }
          if (aBoundsRect) {
            aBoundsRect->UnionRect(*aBoundsRect, frame->GetScreenRectExternal());
          }
        }
        if (!startFrame) {
          startFrame = frame;
          aStartOffset = 0;
        }
      }
      -- endOffset;
    }
    if (endOffset <= 0 && startFrame) {
      break; // If we don't have startFrame yet, get that in next loop iteration
    }
  }

  if (aEndFrame && !*aEndFrame) {
    *aEndFrame = startFrame;
  }

  return startFrame;
}

NS_IMETHODIMP nsHyperTextAccessible::GetText(PRInt32 aStartOffset, PRInt32 aEndOffset, nsAString &aText)
{
  if (!mDOMNode) {
    return NS_ERROR_FAILURE;
  }
  return GetPosAndText(aStartOffset, aEndOffset, &aText) ? NS_OK : NS_ERROR_FAILURE;
}

/*
 * Gets the character count.
 */
NS_IMETHODIMP nsHyperTextAccessible::GetCharacterCount(PRInt32 *aCharacterCount)
{
  *aCharacterCount = 0;
  if (!mDOMNode) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIAccessible> accessible;

  while (NextChild(accessible)) {
    *aCharacterCount += TextLength(accessible);
  }
  return NS_OK;
}

/*
 * Gets the specified character.
 */
NS_IMETHODIMP nsHyperTextAccessible::GetCharacterAtOffset(PRInt32 aOffset, PRUnichar *aCharacter)
{
  if (!mDOMNode) {
    return NS_ERROR_FAILURE;
  }
  nsAutoString text;
  nsresult rv = GetText(aOffset, aOffset + 1, text);
  NS_ENSURE_SUCCESS(rv, rv);
  *aCharacter = text.First();
  return NS_OK;
}

nsresult nsHyperTextAccessible::DOMPointToOffset(nsIDOMNode* aNode, PRInt32 aNodeOffset, PRInt32* aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = 0;
  NS_ENSURE_ARG_POINTER(aNode);
  NS_ENSURE_TRUE(aNodeOffset >= 0, NS_ERROR_INVALID_ARG);

  unsigned short nodeType;
  aNode->GetNodeType(&nodeType);
  if (nodeType != nsIDOMNode::TEXT_NODE) {
    // If not text, aNodeOffset is a child
    // aNode != mDOMNode for nsDocAccessible case, where 
    // aNode is for <body> and mDOMNode is doc element
    nsCOMPtr<nsINode> hyperTextNode(do_QueryInterface(aNode));
    NS_ENSURE_TRUE(hyperTextNode, NS_ERROR_FAILURE);
    nsCOMPtr<nsIAccessible> childAccessible;
    // Find first accessible child starting at aNodeOffset. We need to do this because
    // the node at aNodeOffset might not be an accessible
    while (!childAccessible) {
      nsIContent *childContent = hyperTextNode->GetChildAt(aNodeOffset ++);
      nsCOMPtr<nsIDOMNode> childNode(do_QueryInterface(childContent));
      if (!childNode) {
        break; // Caret at end of hyper text
      }
      GetAccService()->GetAccessibleFor(childNode, getter_AddRefs(childAccessible));
    }
    
    // Loop through, adding offsets until we reach childAccessible
    // If childAccessible is null we will end up adding up the entire length of
    // the hypertext, which is good -- it just means our offset node
    // came after the last accessible child's node
    nsCOMPtr<nsIAccessible> accessible;
    while (NextChild(accessible) && accessible != childAccessible) {
      *aResult += TextLength(accessible);
    }
    return NS_OK;
  }

  // This is text, and aNodeOffset is the nth character in the text
  *aResult = aNodeOffset;
  nsCOMPtr<nsIAccessible> accessible;

  while (NextChild(accessible)) {
    nsCOMPtr<nsPIAccessNode> accessNode(do_QueryInterface(accessible));
    nsIFrame *frame = accessNode->GetFrame();
    if (!frame) {
      return NS_ERROR_FAILURE;
    }

    if (frame && SameCOMIdentity(frame->GetContent(), aNode)) {
      return NS_OK;
    }

    *aResult += TextLength(accessible);
  }

  return NS_ERROR_FAILURE;
}

PRInt32 nsHyperTextAccessible::GetRelativeOffset(nsIPresShell *aPresShell, nsIFrame *aFromFrame, PRInt32 aFromOffset,
                                                 nsSelectionAmount aAmount, nsDirection aDirection, PRBool aNeedsStart)
{
  const PRBool kIsJumpLinesOk = PR_TRUE;          // okay to jump lines
  const PRBool kIsScrollViewAStop = PR_FALSE;     // do not stop at scroll views
  const PRBool kIsKeyboardSelect = PR_TRUE;       // is keyboard selection
  const PRBool kIsVisualBidi = PR_FALSE;          // use visual order for bidi text
  const PRInt32 kDesiredX = aPresShell->GetRootFrame()->GetSize().width;

  EWordMovementType wordMovementType = aNeedsStart ? eStartWord : eEndWord;
  if (aAmount == eSelectLine && aDirection == eDirNext) {
    aAmount = eSelectEndLine; // Select forward to end of line
  }

  nsPeekOffsetStruct pos;
  pos.SetData(aAmount, aDirection, aFromOffset, kDesiredX,
              kIsJumpLinesOk,
              kIsScrollViewAStop, kIsKeyboardSelect, kIsVisualBidi,
              wordMovementType);
  nsresult rv = aFromFrame->PeekOffset(&pos);

  PRInt32 resultOffset = pos.mContentOffset;
  nsIContent *resultContent = nsnull;
  if (NS_SUCCEEDED(rv)) {
    resultContent = pos.mResultContent;
  }
  nsCOMPtr<nsIDOMNode> resultNode = do_QueryInterface(resultContent);
  PRInt32 hyperTextOffset;
  if (!resultContent || NS_FAILED(DOMPointToOffset(resultNode, resultOffset, &hyperTextOffset))) {
    if (aDirection == eDirNext) {
      GetCharacterCount(&hyperTextOffset);
    }
    else {
      hyperTextOffset = 0;
    }
  }

  return hyperTextOffset;
}

/*
Gets the specified text relative to aBoundaryType, which means:
BOUNDARY_CHAR             The character before/at/after the offset is returned.
BOUNDARY_WORD_START       From the word start before/at/after the offset to the next word start.
BOUNDARY_WORD_END         From the word end before/at/after the offset to the next work end.
BOUNDARY_SENTENCE_START   From the sentence start before/at/after the offset to the next sentence start.
BOUNDARY_SENTENCE_END     From the sentence end before/at/after the offset to the next sentence end.
BOUNDARY_LINE_START       From the line start before/at/after the offset to the next line start.
BOUNDARY_LINE_END         From the line end before/at/after the offset to the next line start.
*/

nsresult nsHyperTextAccessible::GetTextHelper(EGetTextType aType, nsAccessibleTextBoundary aBoundaryType,
                                              PRInt32 aOffset, PRInt32 *aStartOffset, PRInt32 *aEndOffset,
                                              nsAString &aText)
{
  aText.Truncate();
  *aStartOffset = *aEndOffset = 0;

  nsCOMPtr<nsIPresShell> presShell = GetPresShell();
  if (!presShell) {
    return NS_ERROR_FAILURE;
  }

  PRInt32 startOffset = aOffset;
  PRInt32 endOffset = aOffset;

  if (aBoundaryType == BOUNDARY_LINE_END) {
    // Avoid getting the previous line
    ++ startOffset;
    ++ endOffset;
  }
  // Convert offsets to frame-relative
  nsIFrame *startFrame = GetPosAndText(startOffset, endOffset);
  if (!startFrame) {
    PRInt32 textLength;
    GetCharacterCount(&textLength);
    return (aOffset < 0 || aOffset > textLength) ? NS_ERROR_FAILURE : NS_OK;
  }

  nsSelectionAmount amount;
  PRBool needsStart = PR_FALSE;
  switch (aBoundaryType)
  {
  case BOUNDARY_CHAR:
    amount = eSelectCharacter;
    if (aType == eGetAt) {
      aType = eGetAfter; // Avoid returning 2 characters
    }
    break;
  case BOUNDARY_WORD_START:
    needsStart = PR_TRUE;
    amount = eSelectWord;
    break;
  case BOUNDARY_WORD_END:
    amount = eSelectWord;
    break;
  case BOUNDARY_LINE_START:
    // Newlines are considered at the end of a line,
    // Since getting the BOUNDARY_LINE_START gets the text from the line-start
    // to the next line-start, the newline is included at the end of the string
    amount = eSelectLine;
    break;
  case BOUNDARY_LINE_END:
    // Newlines are considered at the end of a line,
    // Since getting the BOUNDARY_END_START gets the text from the line-end
    // to the next line-end, the newline is included at the beginning of the string
    amount = eSelectLine;
    break;
  case BOUNDARY_SENTENCE_START:
    // XXX Need to implement
    // isStartPreferred = PR_TRUE;
    return NS_ERROR_NOT_IMPLEMENTED;
  case BOUNDARY_SENTENCE_END:
    // XXX Need to implement
    return NS_ERROR_NOT_IMPLEMENTED;
  case BOUNDARY_ATTRIBUTE_RANGE:
    {
      // XXX We should merge identically formatted frames
      // XXX deal with static text case
      // XXX deal with boundary type
      nsIContent *textContent = startFrame->GetContent();
      // If not text, then it's represented by an embedded object char 
      // (length of 1)
      // XXX did this mean to check for eTEXT?
      PRInt32 textLength = textContent ? textContent->TextLength() : 1;
      *aStartOffset = aOffset - startOffset;
      *aEndOffset = *aStartOffset + textLength;
      startOffset = *aStartOffset;
      endOffset = *aEndOffset;
      return GetText(startOffset, endOffset, aText);
    }
  default:
    return NS_ERROR_INVALID_ARG;
  }

  // If aType == eGetAt we'll change both the start and end offset from
  // the original offset
  PRInt32 startForwardSearchOffset;
  if (aType == eGetAfter) {
    startForwardSearchOffset = startOffset = aOffset;
  }
  else {
    startOffset = GetRelativeOffset(presShell, startFrame,  startOffset,
                                    amount, eDirPrevious, needsStart);
    startForwardSearchOffset = startOffset;
    if (amount == eSelectLine) {
      ++ startForwardSearchOffset; // Avoid getting the previous line
      if (aBoundaryType == BOUNDARY_LINE_START && startOffset > 0) {
        // Start line fixup: don't include \n at start of string
        ++ startOffset;
      }
    }
  }

  if (aType == eGetBefore) {
    endOffset = aOffset;
  }
  else {
    // Start moving forward from the start so that we don't get 
    // 2 words/lines/sentences if the offset occured on whitespace boundary    
    endOffset = startForwardSearchOffset; // Passed by reference
    nsIFrame *endFrame = GetPosAndText(startForwardSearchOffset, endOffset);
    if (!endFrame) {
      return NS_ERROR_FAILURE;
    }
    endOffset = GetRelativeOffset(presShell, endFrame, endOffset, amount,
                                  eDirNext, needsStart);
    if (amount == eSelectLine &&
        endFrame->GetType() == nsAccessibilityAtoms::textFrame) {
      PRInt32 startFrameOffsetUnused, endFrameOffset;
      if (NS_SUCCEEDED(endFrame->GetOffsets(startFrameOffsetUnused, endFrameOffset))) {
        nsCOMPtr<nsIDOMNode> endNode = do_QueryInterface(endFrame->GetContent());
        nsCOMPtr<nsIAccessible> endAccessible;
        if (endNode && NS_SUCCEEDED(GetAccService()->GetAccessibleFor(endNode, getter_AddRefs(endAccessible))) &&
            NextChild(endAccessible) && Role(endAccessible) == ROLE_WHITESPACE) {
          ++ endOffset; // Make sure endOffset comes after <br>
        }
        // End line fixup: include \n for <br> at end of string,
        // only if aBoundaryType == BOUNDARY_LINE_START
        if (aBoundaryType == BOUNDARY_LINE_END) {
          -- endOffset;
        }
      }
    }
  }

  // Fix word error for the first character in word: PeekOffset() will return the previous word when 
  // aOffset points to the first character of the word, but accessibility APIs want the current word 
  // that the first character is in
  if (aType == eGetAt && amount == eSelectWord && aOffset == endOffset) { 
    return GetTextHelper(eGetAfter, aBoundaryType, aOffset, aStartOffset, aEndOffset, aText);
  }

  *aStartOffset = startOffset;
  *aEndOffset = endOffset;

  NS_ASSERTION((startOffset < aOffset && endOffset >= aOffset) || aType != eGetBefore, "Incorrect results for GetTextHelper");
  NS_ASSERTION((startOffset <= aOffset && endOffset > aOffset) || aType == eGetBefore, "Incorrect results for GetTextHelper");

  return GetPosAndText(startOffset, endOffset, &aText) ? NS_OK : NS_ERROR_FAILURE;
}

/**
  * nsIAccessibleText impl.
  */
NS_IMETHODIMP nsHyperTextAccessible::GetTextBeforeOffset(PRInt32 aOffset, nsAccessibleTextBoundary aBoundaryType,
                                                         PRInt32 *aStartOffset, PRInt32 *aEndOffset, nsAString & aText)
{
  return GetTextHelper(eGetBefore, aBoundaryType, aOffset, aStartOffset, aEndOffset, aText);
}

NS_IMETHODIMP nsHyperTextAccessible::GetTextAtOffset(PRInt32 aOffset, nsAccessibleTextBoundary aBoundaryType,
                                                     PRInt32 *aStartOffset, PRInt32 *aEndOffset, nsAString & aText)
{
  return GetTextHelper(eGetAt, aBoundaryType, aOffset, aStartOffset, aEndOffset, aText);
}

NS_IMETHODIMP nsHyperTextAccessible::GetTextAfterOffset(PRInt32 aOffset, nsAccessibleTextBoundary aBoundaryType,
                                                        PRInt32 *aStartOffset, PRInt32 *aEndOffset, nsAString & aText)
{
  return GetTextHelper(eGetAfter, aBoundaryType, aOffset, aStartOffset, aEndOffset, aText);
}

NS_IMETHODIMP nsHyperTextAccessible::GetAttributeRange(PRInt32 aOffset, PRInt32 *aRangeStartOffset, 
                                                       PRInt32 *aRangeEndOffset, nsIAccessible **aAccessibleWithAttrs)
{
  // Return the range of text with common attributes around aOffset
  *aRangeStartOffset = *aRangeEndOffset = 0;
  *aAccessibleWithAttrs = nsnull;

  if (!mDOMNode) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIAccessible> accessible;
  
  while (NextChild(accessible)) {
    PRInt32 length = TextLength(accessible);
    if (*aRangeStartOffset + length > aOffset) {
      *aRangeEndOffset = *aRangeStartOffset + length;
      NS_ADDREF(*aAccessibleWithAttrs = accessible);
      return NS_OK;
    }
    *aRangeStartOffset += length;
  }

  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsHyperTextAccessible::GetAttributes(nsIPersistentProperties **aAttributes)
{
  if (!mDOMNode) {
    return NS_ERROR_FAILURE;  // Node already shut down
  }

  nsresult rv = nsAccessibleWrap::GetAttributes(aAttributes);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));
  NS_ENSURE_TRUE(content, NS_ERROR_UNEXPECTED);
  nsIAtom *tag = content->Tag();

  PRInt32 headLevel = 0;
  if (tag == nsAccessibilityAtoms::h1) {
    headLevel = 1;
  }
  else if (tag == nsAccessibilityAtoms::h2) {
    headLevel = 2;
  }
  else if (tag == nsAccessibilityAtoms::h3) {
    headLevel = 3;
  }
  else if (tag == nsAccessibilityAtoms::h4) {
    headLevel = 4;
  }
  else if (tag == nsAccessibilityAtoms::h5) {
    headLevel = 5;
  }
  else if (tag == nsAccessibilityAtoms::h6) {
    headLevel = 6;
  }
  if (headLevel) {
    nsAutoString valueString;
    valueString.AppendInt(headLevel);
    nsAutoString oldValueUnused;
    (*aAttributes)->SetStringProperty(NS_LITERAL_CSTRING("level"), valueString, oldValueUnused);
  }

  return  NS_OK;
}

/*
 * Given an offset, the x, y, width, and height values are filled appropriately.
 */
NS_IMETHODIMP nsHyperTextAccessible::GetCharacterExtents(PRInt32 aOffset, PRInt32 *aX, PRInt32 *aY,
                                                         PRInt32 *aWidth, PRInt32 *aHeight,
                                                         nsAccessibleCoordType aCoordType)
{
  return GetRangeExtents(aOffset, aOffset + 1, aX, aY, aWidth, aHeight, aCoordType);
}

/*
 * Given a start & end offset, the x, y, width, and height values are filled appropriately.
 */
NS_IMETHODIMP nsHyperTextAccessible::GetRangeExtents(PRInt32 aStartOffset, PRInt32 aEndOffset,
                                                     PRInt32 *aX, PRInt32 *aY,
                                                     PRInt32 *aWidth, PRInt32 *aHeight,
                                                     nsAccessibleCoordType aCoordType)
{
  nsIntRect boundsRect;
  nsIFrame *endFrameUnused;
  if (!GetPosAndText(aStartOffset, aEndOffset, nsnull, &endFrameUnused, &boundsRect)) {
    return NS_ERROR_FAILURE;
  }

  *aX = boundsRect.x;
  *aY = boundsRect.y;
  *aWidth = boundsRect.width;
  *aHeight = boundsRect.height;

  if (aCoordType == COORD_TYPE_WINDOW) {
    //co-ord type = window
    nsCOMPtr<nsIPresShell> shell = GetPresShell();
    NS_ENSURE_TRUE(shell, NS_ERROR_FAILURE);
    nsCOMPtr<nsIDocument> doc = shell->GetDocument();
    nsCOMPtr<nsIDOMDocumentView> docView(do_QueryInterface(doc));
    NS_ENSURE_TRUE(docView, NS_ERROR_FAILURE);

    nsCOMPtr<nsIDOMAbstractView> abstractView;
    docView->GetDefaultView(getter_AddRefs(abstractView));
    NS_ENSURE_TRUE(abstractView, NS_ERROR_FAILURE);

    nsCOMPtr<nsIDOMWindowInternal> windowInter(do_QueryInterface(abstractView));
    NS_ENSURE_TRUE(windowInter, NS_ERROR_FAILURE);

    PRInt32 screenX, screenY;
    if (NS_FAILED(windowInter->GetScreenX(&screenX)) ||
        NS_FAILED(windowInter->GetScreenY(&screenY))) {
      return NS_ERROR_FAILURE;
    }
    *aX -= screenX;
    *aY -= screenY;
  }
  // else default: co-ord type = screen

  return NS_OK;
}

/*
 * Gets the offset of the character located at coordinates x and y. x and y are interpreted as being relative to
 * the screen or this widget's window depending on coords.
 */
NS_IMETHODIMP nsHyperTextAccessible::GetOffsetAtPoint(PRInt32 aX, PRInt32 aY, nsAccessibleCoordType aCoordType, PRInt32 *aOffset)
{
  *aOffset = -1;
  nsCOMPtr<nsIPresShell> shell = GetPresShell();
  if (!shell) {
    return NS_ERROR_FAILURE;
  }
  nsIFrame *hyperFrame = GetFrame();
  if (!hyperFrame) {
    return NS_ERROR_FAILURE;
  }
  nsIntRect frameScreenRect = hyperFrame->GetScreenRectExternal();

  if (aCoordType == COORD_TYPE_WINDOW) {
    nsCOMPtr<nsIDocument> doc = shell->GetDocument();
    nsCOMPtr<nsIDOMDocumentView> docView(do_QueryInterface(doc));
    NS_ENSURE_TRUE(docView, NS_ERROR_FAILURE);

    nsCOMPtr<nsIDOMAbstractView> abstractView;
    docView->GetDefaultView(getter_AddRefs(abstractView));
    NS_ENSURE_TRUE(abstractView, NS_ERROR_FAILURE);

    nsCOMPtr<nsIDOMWindowInternal> windowInter(do_QueryInterface(abstractView));
    NS_ENSURE_TRUE(windowInter, NS_ERROR_FAILURE);

    PRInt32 windowX, windowY;
    if (NS_FAILED(windowInter->GetScreenX(&windowX)) ||
        NS_FAILED(windowInter->GetScreenY(&windowY))) {
      return NS_ERROR_FAILURE;
    }
    aX += windowX;
    aY += windowY;
  }
  // aX, aY are currently screen coordinates, and we need to turn them into
  // frame coordinates relative to the current accessible
  if (!frameScreenRect.Contains(aX, aY)) {
    return NS_OK;   // Not found, will return -1
  }
  nsPoint pointInHyperText(aX - frameScreenRect.x, aY - frameScreenRect.y);
  nsPresContext *context = GetPresContext();
  NS_ENSURE_TRUE(context, NS_ERROR_FAILURE);
  // Convert to twips
  float p2t = context->PixelsToTwips();
  pointInHyperText.x = NSIntPixelsToTwips(pointInHyperText.x, p2t);
  pointInHyperText.y = NSIntPixelsToTwips(pointInHyperText.y, p2t);

  // Go through the frames to check if each one has the point.
  // When one does, add up the character offsets until we have a match

  // We have an point in an accessible child of this, now we need to add up the
  // offsets before it to what we already have
  nsCOMPtr<nsIAccessible> accessible;
  PRInt32 offset = 0;

  while (NextChild(accessible)) {
    nsCOMPtr<nsPIAccessNode> accessNode(do_QueryInterface(accessible));
    nsIFrame *frame = accessNode->GetFrame();
    NS_ENSURE_TRUE(frame, NS_ERROR_FAILURE);
    while (frame) {
      nsIContent *content = frame->GetContent();
      NS_ENSURE_TRUE(content, NS_ERROR_FAILURE);
      nsPoint pointInFrame = pointInHyperText - frame->GetOffsetToExternal(hyperFrame);
      nsSize frameSize = frame->GetSize();
      if (pointInFrame.x < frameSize.width && pointInFrame.y < frameSize.height) {
        // Finished
        if (IsText(accessible)) {
          nsIFrame::ContentOffsets contentOffsets = frame->GetContentOffsetsFromPointExternal(pointInFrame, PR_TRUE);
          if (contentOffsets.IsNull() || contentOffsets.content != content) {
            return NS_OK; // Not found, will return -1
          }
          offset += contentOffsets.offset;
        }
        *aOffset = offset;
        return NS_OK;
      }
      frame = frame->GetNextContinuation();
    }
    offset += TextLength(accessible);
  }

  return NS_OK; // Not found, will return -1
}

// ------- nsIAccessibleHyperText ---------------
NS_IMETHODIMP nsHyperTextAccessible::GetLinks(PRInt32 *aLinks)
{
  *aLinks = 0;
  if (!mDOMNode) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIAccessible> accessible;

  while (NextChild(accessible)) {
    if (IsEmbeddedObject(accessible)) {
      ++*aLinks;
    }
  }
  return NS_OK;
}


NS_IMETHODIMP nsHyperTextAccessible::GetLink(PRInt32 aIndex, nsIAccessibleHyperLink **aLink)
{
  *aLink = nsnull;
  if (!mDOMNode) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIAccessible> accessible;

  while (NextChild(accessible)) {
    if (IsEmbeddedObject(accessible) && aIndex-- == 0) {
      CallQueryInterface(accessible, aLink);
      return NS_OK;
    }
  }
  return NS_OK;
}

NS_IMETHODIMP nsHyperTextAccessible::GetLinkIndex(PRInt32 aCharIndex, PRInt32 *aLinkIndex)
{
  *aLinkIndex = -1; // API says this magic value means 'not found'

  PRInt32 characterCount = 0;
  PRInt32 linkIndex = 0;
  if (!mDOMNode) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIAccessible> accessible;

  while (NextChild(accessible) && characterCount <= aCharIndex) {
    PRUint32 role = Role(accessible);
    if (role == ROLE_TEXT_LEAF || role == ROLE_STATICTEXT) {
      characterCount += TextLength(accessible);
    }
    else {
      if (characterCount ++ == aCharIndex) {
        *aLinkIndex = linkIndex;
        break;
      }
      if (role != ROLE_WHITESPACE) {
        ++ linkIndex;
      }
    }
  }
  return NS_OK;
}

/**
  * nsIAccessibleEditableText impl.
  */
NS_IMETHODIMP nsHyperTextAccessible::SetAttributes(PRInt32 aStartPos, PRInt32 aEndPos,
                                                   nsISupports *aAttributes)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsHyperTextAccessible::SetTextContents(const nsAString &aText)
{
  PRInt32 numChars;
  GetCharacterCount(&numChars);
  if (numChars == 0 || NS_SUCCEEDED(DeleteText(0, numChars))) {
    return InsertText(aText, 0);
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsHyperTextAccessible::InsertText(const nsAString &aText, PRInt32 aPosition)
{
  if (NS_SUCCEEDED(SetCaretOffset(aPosition))) {
    nsCOMPtr<nsIEditor> editor = GetEditor();
    nsCOMPtr<nsIPlaintextEditor> peditor(do_QueryInterface(editor));
    return peditor ? peditor->InsertText(aText) : NS_ERROR_FAILURE;
  }

  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsHyperTextAccessible::CopyText(PRInt32 aStartPos, PRInt32 aEndPos)
{
  nsCOMPtr<nsIEditor> editor = GetEditor();
  if (editor && NS_SUCCEEDED(SetSelectionRange(aStartPos, aEndPos)))
    return editor->Copy();

  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsHyperTextAccessible::CutText(PRInt32 aStartPos, PRInt32 aEndPos)
{
  nsCOMPtr<nsIEditor> editor = GetEditor();
  if (editor && NS_SUCCEEDED(SetSelectionRange(aStartPos, aEndPos)))
    return editor->Cut();

  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsHyperTextAccessible::DeleteText(PRInt32 aStartPos, PRInt32 aEndPos)
{
  nsCOMPtr<nsIEditor> editor = GetEditor();
  if (editor && NS_SUCCEEDED(SetSelectionRange(aStartPos, aEndPos)))
    return editor->DeleteSelection(nsIEditor::eNone);

  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsHyperTextAccessible::PasteText(PRInt32 aPosition)
{
  nsCOMPtr<nsIEditor> editor = GetEditor();
  if (editor && NS_SUCCEEDED(SetCaretOffset(aPosition)))
    return editor->Paste(nsIClipboard::kGlobalClipboard);

  return NS_ERROR_FAILURE;
}

/**
  * nsIEditActionListener impl.
  */
NS_IMETHODIMP nsHyperTextAccessible::WillCreateNode(const nsAString& aTag,
                                                    nsIDOMNode *aParent, PRInt32 aPosition)
{
  return NS_OK;
}

NS_IMETHODIMP nsHyperTextAccessible::DidCreateNode(const nsAString& aTag, nsIDOMNode *aNode,
                                                   nsIDOMNode *aParent, PRInt32 aPosition, nsresult aResult)
{
  return NS_OK;
}

NS_IMETHODIMP nsHyperTextAccessible::WillInsertNode(nsIDOMNode *aNode, nsIDOMNode *aParent,
                                                    PRInt32 aPosition)
{
  return NS_OK;
}

NS_IMETHODIMP nsHyperTextAccessible::DidInsertNode(nsIDOMNode *aNode, nsIDOMNode *aParent,
                                                   PRInt32 aPosition, nsresult aResult)
{
  InvalidateChildren();
  AtkTextChange textData;

  textData.add = PR_TRUE;
  nsCOMPtr<nsIContent> content(do_QueryInterface(aNode));
  if (content && content->IsNodeOfType(nsINode::eTEXT)) {
    textData.length = content->TextLength();
    if (!textData.length) {
      return NS_OK;
    }
  }
  else {
    // Don't fire event for the first br
    nsCOMPtr<nsIEditor> editor = GetEditor();
    if (editor) {
      PRBool isEmpty;
      editor->GetDocumentIsEmpty(&isEmpty);
      if (isEmpty) {
        return NS_OK;
      }
    }
    
    textData.length = 1;
  }

  if (NS_FAILED(DOMPointToOffset(aNode, aPosition, &textData.start))) {
    return NS_OK;
  }
  return FireTextChangeEvent(&textData);
}

NS_IMETHODIMP nsHyperTextAccessible::WillDeleteNode(nsIDOMNode *aChild)
{
  AtkTextChange textData;

  textData.add = PR_FALSE;
  textData.length = 1;
  nsCOMPtr<nsIContent> content(do_QueryInterface(aChild));
  if (content && content->IsNodeOfType(nsINode::eTEXT)) {
    textData.length = content->TextLength();
    if (!textData.length) {
      return NS_OK;
    }
  }
  else {
    // Don't fire event for the last br
    nsCOMPtr<nsIEditor> editor = GetEditor();
    if (editor) {
      PRBool isEmpty;
      editor->GetDocumentIsEmpty(&isEmpty);
      if (isEmpty) {
        return NS_OK;
      }
    }
  }

  nsCOMPtr<nsIDOMNode> parentNode;
  aChild->GetParentNode(getter_AddRefs(parentNode));
  nsCOMPtr<nsIContent> parentContent(do_QueryInterface(parentNode));
  NS_ENSURE_TRUE(parentContent, NS_ERROR_FAILURE);
  nsCOMPtr<nsIContent> childContent(do_QueryInterface(aChild));
  NS_ENSURE_TRUE(childContent, NS_ERROR_FAILURE);
  if (NS_FAILED(DOMPointToOffset(parentNode, parentContent->IndexOf(childContent), &textData.start))) {
    return NS_OK;
  }
  return FireTextChangeEvent(&textData);
}

NS_IMETHODIMP nsHyperTextAccessible::DidDeleteNode(nsIDOMNode *aChild, nsresult aResult)
{
  return InvalidateChildren();
}

NS_IMETHODIMP nsHyperTextAccessible::WillSplitNode(nsIDOMNode *aExistingRightNode, PRInt32 aOffset)
{
  return NS_OK;
}

NS_IMETHODIMP nsHyperTextAccessible::DidSplitNode(nsIDOMNode *aExistingRightNode, PRInt32 aOffset,
                                                  nsIDOMNode *aNewLeftNode, nsresult aResult)
{
  return InvalidateChildren();
}

NS_IMETHODIMP nsHyperTextAccessible::WillJoinNodes(nsIDOMNode *aLeftNode,
                                                   nsIDOMNode *aRightNode, nsIDOMNode *aParent)
{
  return NS_OK;
}

NS_IMETHODIMP nsHyperTextAccessible::DidJoinNodes(nsIDOMNode *aLeftNode, nsIDOMNode *aRightNode,
                                                  nsIDOMNode *aParent, nsresult aResult)
{
  return InvalidateChildren();
}

NS_IMETHODIMP nsHyperTextAccessible::WillInsertText(nsIDOMCharacterData *aTextNode,
                                                    PRInt32 aOffset, const nsAString& aString)
{
  return NS_OK;
}

NS_IMETHODIMP nsHyperTextAccessible::DidInsertText(nsIDOMCharacterData *aTextNode, PRInt32 aOffset,
                                                   const nsAString& aString, nsresult aResult)
{
  AtkTextChange textData;

  textData.add = PR_TRUE;
  textData.length = aString.Length();
  if (NS_FAILED(DOMPointToOffset(aTextNode, aOffset, &textData.start))) {
    return NS_OK;
  }
  return FireTextChangeEvent(&textData);
}

NS_IMETHODIMP nsHyperTextAccessible::WillDeleteText(nsIDOMCharacterData *aTextNode,
                                                    PRInt32 aOffset, PRInt32 aLength)
{
  AtkTextChange textData;

  textData.add = PR_FALSE;
  textData.length = aLength;
  if (NS_FAILED(DOMPointToOffset(aTextNode, aOffset, &textData.start))) {
    return NS_OK;
  }
  return FireTextChangeEvent(&textData);
}

NS_IMETHODIMP nsHyperTextAccessible::DidDeleteText(nsIDOMCharacterData *aTextNode, PRInt32 aOffset,
                                                   PRInt32 aLength, nsresult aResult)
{
  return NS_OK;
}

NS_IMETHODIMP nsHyperTextAccessible::WillDeleteSelection(nsISelection *aSelection)
// <input> & <textarea> fires this event while deleting text
// <editor> fires WillDeleteText/WillDeleteNode instead
// XXX Deal with > 1 selections
{
  PRInt32 selectionStart, selectionEnd;
  GetSelectionBounds(0, &selectionStart, &selectionEnd);

  AtkTextChange textData;

  textData.add = PR_FALSE;
  textData.start = PR_MIN(selectionStart, selectionEnd);
  textData.length = PR_ABS(selectionEnd - selectionStart);
  return FireTextChangeEvent(&textData);
}

NS_IMETHODIMP nsHyperTextAccessible::DidDeleteSelection(nsISelection *aSelection)
{
  return NS_OK;
}

/**
  * =================== Caret & Selection ======================
  */

nsresult nsHyperTextAccessible::FireTextChangeEvent(AtkTextChange *aTextData)
{
  nsCOMPtr<nsIAccessible> accessible(do_QueryInterface(NS_STATIC_CAST(nsIAccessibleText*, this)));
  nsCOMPtr<nsPIAccessible> privAccessible(do_QueryInterface(accessible));
  if (privAccessible) {
#ifdef DEBUG_A11Y
    printf("  [start=%d, length=%d, add=%d]\n", aTextData->start, aTextData->length, aTextData->add);
#endif
    privAccessible->FireToolkitEvent(nsIAccessibleEvent::EVENT_ATK_TEXT_CHANGE, accessible, aTextData);
  }
  return NS_OK;
}

nsresult nsHyperTextAccessible::SetSelectionRange(PRInt32 aStartPos, PRInt32 aEndPos)
{
  // Set the selection
  nsresult rv = SetSelectionBounds(0, aStartPos, aEndPos);
  NS_ENSURE_SUCCESS(rv, rv);

  // If range 0 was successfully set, clear any additional selection 
  // ranges remaining from previous selection
  nsCOMPtr<nsISelection> domSel;
  GetSelections(nsnull, getter_AddRefs(domSel));
  if (domSel) {
    PRInt32 numRanges;
    domSel->GetRangeCount(&numRanges);

    for (PRInt32 count = 0; count < numRanges - 1; count ++) {
      nsCOMPtr<nsIDOMRange> range;
      domSel->GetRangeAt(1, getter_AddRefs(range));
      domSel->RemoveRange(range);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP nsHyperTextAccessible::SetCaretOffset(PRInt32 aCaretOffset)
{
  return SetSelectionRange(aCaretOffset, aCaretOffset);
}

/*
 * Gets the offset position of the caret (cursor).
 */
NS_IMETHODIMP nsHyperTextAccessible::GetCaretOffset(PRInt32 *aCaretOffset)
{
  *aCaretOffset = 0;

  nsCOMPtr<nsISelection> domSel;
  nsresult rv = GetSelections(nsnull, getter_AddRefs(domSel));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMNode> caretNode;
  rv = domSel->GetFocusNode(getter_AddRefs(caretNode));
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 caretOffset;
  domSel->GetFocusOffset(&caretOffset);

  return DOMPointToOffset(caretNode, caretOffset, aCaretOffset);
}

nsresult nsHyperTextAccessible::GetSelections(nsISelectionController **aSelCon, nsISelection **aDomSel)
{
  if (aSelCon) {
    *aSelCon = nsnull;
  }
  if (aDomSel) {
    *aDomSel = nsnull;
  }
  
  nsCOMPtr<nsIEditor> editor = GetEditor();
  if (editor) {
    editor->GetSelectionController(aSelCon);
    NS_ENSURE_TRUE(aSelCon, NS_ERROR_FAILURE);
    editor->GetSelection(aDomSel);
    NS_ENSURE_TRUE(aDomSel, NS_ERROR_FAILURE);
    return NS_OK;
  }

  nsIFrame *frame = GetFrame();
  NS_ENSURE_TRUE(frame, NS_ERROR_FAILURE);

  // Get the selection and selection controller
  nsCOMPtr<nsISelectionController> selCon;
  frame->GetSelectionController(GetPresContext(),
                                getter_AddRefs(selCon));
  NS_ENSURE_TRUE(selCon, NS_ERROR_FAILURE);
  if (aSelCon) {
    NS_ADDREF(*aSelCon = selCon);
  }

  if (aDomSel) {
    nsCOMPtr<nsISelection> domSel;
    selCon->GetSelection(nsISelectionController::SELECTION_NORMAL, getter_AddRefs(domSel));
    NS_ENSURE_TRUE(domSel, NS_ERROR_FAILURE);
    NS_ADDREF(*aDomSel = domSel);
  }

  return NS_OK;
}

/*
 * Gets the number of selected regions.
 */
NS_IMETHODIMP nsHyperTextAccessible::GetSelectionCount(PRInt32 *aSelectionCount)
{
  nsCOMPtr<nsISelection> domSel;
  nsresult rv = GetSelections(nsnull, getter_AddRefs(domSel));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool isSelectionCollapsed;
  rv = domSel->GetIsCollapsed(&isSelectionCollapsed);
  NS_ENSURE_SUCCESS(rv, rv);

  if (isSelectionCollapsed)
    *aSelectionCount = 0;

  rv = domSel->GetRangeCount(aSelectionCount);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/*
 * Gets the start and end offset of the specified selection.
 */
NS_IMETHODIMP nsHyperTextAccessible::GetSelectionBounds(PRInt32 aSelectionNum, PRInt32 *aStartOffset, PRInt32 *aEndOffset)
{
  *aStartOffset = *aEndOffset = 0;

  nsCOMPtr<nsISelection> domSel;
  nsresult rv = GetSelections(nsnull, getter_AddRefs(domSel));
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 rangeCount;
  domSel->GetRangeCount(&rangeCount);
  if (aSelectionNum < 0 || aSelectionNum >= rangeCount)
    return NS_ERROR_INVALID_ARG;

  nsCOMPtr<nsIDOMRange> range;
  rv = domSel->GetRangeAt(aSelectionNum, getter_AddRefs(range));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMNode> startNode;
  range->GetStartContainer(getter_AddRefs(startNode));
  PRInt32 startOffset;
  range->GetStartOffset(&startOffset);
  rv = DOMPointToOffset(startNode, startOffset, aStartOffset);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMNode> endNode;
  range->GetEndContainer(getter_AddRefs(endNode));
  PRInt32 endOffset;
  range->GetEndOffset(&endOffset);
  if (startNode == endNode && startOffset == endOffset) {
    // Shortcut for collapsed selection case (caret)
    *aEndOffset = *aStartOffset;
    return NS_OK;
  }
  return DOMPointToOffset(endNode, endOffset, aEndOffset);
}

/*
 * Changes the start and end offset of the specified selection.
 */
NS_IMETHODIMP nsHyperTextAccessible::SetSelectionBounds(PRInt32 aSelectionNum, PRInt32 aStartOffset, PRInt32 aEndOffset)
{
  nsCOMPtr<nsISelection> domSel;
  nsresult rv = GetSelections(nsnull, getter_AddRefs(domSel));
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 isOnlyCaret = (aStartOffset == aEndOffset); // Caret is a collapsed selection

  PRInt32 rangeCount;
  domSel->GetRangeCount(&rangeCount);
  nsCOMPtr<nsIDOMRange> range;
  if (aSelectionNum == rangeCount) { // Add a range
    range = do_CreateInstance(kRangeCID);
    NS_ENSURE_TRUE(range, NS_ERROR_OUT_OF_MEMORY);
  }
  else if (aSelectionNum < 0 || aSelectionNum > rangeCount) {
    return NS_ERROR_INVALID_ARG;
  }
  else {
    domSel->GetRangeAt(aSelectionNum, getter_AddRefs(range));
    NS_ENSURE_TRUE(range, NS_ERROR_FAILURE);
  }

  nsIFrame *endFrame;
  nsIFrame *startFrame = GetPosAndText(aStartOffset, aEndOffset, nsnull, &endFrame);
  NS_ENSURE_TRUE(startFrame, NS_ERROR_FAILURE);

  nsIContent *startParentContent = startFrame->GetContent();
  if (startFrame->GetType() != nsAccessibilityAtoms::textFrame) {
    nsIContent *newParent = startParentContent->GetParent();
    aStartOffset = newParent->IndexOf(startParentContent);
    startParentContent = newParent;
  }
  nsCOMPtr<nsIDOMNode> startParentNode(do_QueryInterface(startParentContent));
  NS_ENSURE_TRUE(startParentNode, NS_ERROR_FAILURE);
  rv = range->SetStart(startParentNode, aStartOffset);
  NS_ENSURE_SUCCESS(rv, rv);

  if (isOnlyCaret) { 
    rv = range->Collapse(PR_TRUE);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    nsIContent *endParentContent = endFrame->GetContent();
    if (endFrame->GetType() != nsAccessibilityAtoms::textFrame) {
      nsIContent *newParent = endParentContent->GetParent();
      aEndOffset = newParent->IndexOf(endParentContent);
      endParentContent = newParent;
    }
    nsCOMPtr<nsIDOMNode> endParentNode(do_QueryInterface(endParentContent));
    NS_ENSURE_TRUE(endParentNode, NS_ERROR_FAILURE);
    rv = range->SetEnd(endParentNode, aEndOffset);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (aSelectionNum == rangeCount) { // Add successfully created new range
    return domSel->AddRange(range);
  }
  return NS_OK;
}

/*
 * Adds a selection bounded by the specified offsets.
 */
NS_IMETHODIMP nsHyperTextAccessible::AddSelection(PRInt32 aStartOffset, PRInt32 aEndOffset)
{
  nsCOMPtr<nsISelection> domSel;
  nsresult rv = GetSelections(nsnull, getter_AddRefs(domSel));
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 rangeCount;
  domSel->GetRangeCount(&rangeCount);

  return SetSelectionBounds(rangeCount, aStartOffset, aEndOffset);
}

/*
 * Removes the specified selection.
 */
NS_IMETHODIMP nsHyperTextAccessible::RemoveSelection(PRInt32 aSelectionNum)
{
  nsCOMPtr<nsISelection> domSel;
  nsresult rv = GetSelections(nsnull, getter_AddRefs(domSel));
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 rangeCount;
  domSel->GetRangeCount(&rangeCount);
  if (aSelectionNum < 0 || aSelectionNum >= rangeCount)
    return NS_ERROR_INVALID_ARG;

  nsCOMPtr<nsIDOMRange> range;
  domSel->GetRangeAt(aSelectionNum, getter_AddRefs(range));
  return domSel->RemoveRange(range);
}

