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
#ifndef nsHTMLParts_h___
#define nsHTMLParts_h___

#include "nscore.h"
#include "nsISupports.h"
#include "nsIReflowCommand.h"
class nsIArena;
class nsIAtom;
class nsIContent;
class nsIDocument;
class nsIHTMLContent;
class nsIHTMLContentSink;
class nsIHTMLFragmentContentSink;
class nsITextContent;
class nsIURI;
class nsString;
class nsIWebShell;
class nsIAttributeContent;
class nsIPresShell;

// Factory methods for creating html content objects
// XXX argument order is wrong (out parameter should be last)
extern nsresult
NS_NewHTMLAnchorElement(nsIHTMLContent** aResult, nsIAtom* aTag);

extern nsresult
NS_NewHTMLAppletElement(nsIHTMLContent** aResult, nsIAtom* aTag);

extern nsresult
NS_NewHTMLAreaElement(nsIHTMLContent** aResult, nsIAtom* aTag);

extern nsresult
NS_NewHTMLBRElement(nsIHTMLContent** aResult, nsIAtom* aTag);

extern nsresult
NS_NewHTMLBaseElement(nsIHTMLContent** aResult, nsIAtom* aTag);

extern nsresult
NS_NewHTMLBaseFontElement(nsIHTMLContent** aResult, nsIAtom* aTag);

extern nsresult
NS_NewHTMLBodyElement(nsIHTMLContent** aResult, nsIAtom* aTag);

extern nsresult
NS_NewHTMLButtonElement(nsIHTMLContent** aResult, nsIAtom* aTag);

extern nsresult
NS_NewHTMLDListElement(nsIHTMLContent** aResult, nsIAtom* aTag);

extern nsresult
NS_NewHTMLDelElement(nsIHTMLContent** aResult, nsIAtom* aTag);

extern nsresult
NS_NewHTMLDirectoryElement(nsIHTMLContent** aResult, nsIAtom* aTag);

extern nsresult
NS_NewHTMLDivElement(nsIHTMLContent** aResult, nsIAtom* aTag);

extern nsresult
NS_NewHTMLEmbedElement(nsIHTMLContent** aResult, nsIAtom* aTag);

extern nsresult
NS_NewHTMLFieldSetElement(nsIHTMLContent** aResult, nsIAtom* aTag);

extern nsresult
NS_NewHTMLFontElement(nsIHTMLContent** aResult, nsIAtom* aTag);

extern nsresult
NS_NewHTMLFormElement(nsIHTMLContent** aResult, nsIAtom* aTag);

extern nsresult
NS_NewHTMLFrameElement(nsIHTMLContent** aResult, nsIAtom* aTag);

extern nsresult
NS_NewHTMLFrameSetElement(nsIHTMLContent** aResult, nsIAtom* aTag);

extern nsresult
NS_NewHTMLHRElement(nsIHTMLContent** aResult, nsIAtom* aTag);

extern nsresult
NS_NewHTMLHeadElement(nsIHTMLContent** aResult, nsIAtom* aTag);

extern nsresult
NS_NewHTMLHeadingElement(nsIHTMLContent** aResult, nsIAtom* aTag);

extern nsresult
NS_NewHTMLHtmlElement(nsIHTMLContent** aResult, nsIAtom* aTag);

extern nsresult
NS_NewHTMLIFrameElement(nsIHTMLContent** aResult, nsIAtom* aTag);

extern nsresult
NS_NewHTMLImageElement(nsIHTMLContent** aResult, nsIAtom* aTag);

extern nsresult
NS_NewHTMLInputElement(nsIHTMLContent** aResult, nsIAtom* aTag);

extern nsresult
NS_NewHTMLInsElement(nsIHTMLContent** aResult, nsIAtom* aTag);

extern nsresult
NS_NewHTMLIsIndexElement(nsIHTMLContent** aResult, nsIAtom* aTag);

extern nsresult
NS_NewHTMLLIElement(nsIHTMLContent** aResult, nsIAtom* aTag);

extern nsresult
NS_NewHTMLLabelElement(nsIHTMLContent** aResult, nsIAtom* aTag);

extern nsresult
NS_NewHTMLLayerElement(nsIHTMLContent** aResult, nsIAtom* aTag);

extern nsresult
NS_NewHTMLLegendElement(nsIHTMLContent** aResult, nsIAtom* aTag);

extern nsresult
NS_NewHTMLLinkElement(nsIHTMLContent** aResult, nsIAtom* aTag);

extern nsresult
NS_NewHTMLMapElement(nsIHTMLContent** aResult, nsIAtom* aTag);

extern nsresult
NS_NewHTMLMenuElement(nsIHTMLContent** aResult, nsIAtom* aTag);

extern nsresult
NS_NewHTMLMetaElement(nsIHTMLContent** aResult, nsIAtom* aTag);

extern nsresult
NS_NewHTMLModElement(nsIHTMLContent** aResult, nsIAtom* aTag);

extern nsresult
NS_NewHTMLOListElement(nsIHTMLContent** aResult, nsIAtom* aTag);

extern nsresult
NS_NewHTMLObjectElement(nsIHTMLContent** aResult, nsIAtom* aTag);

extern nsresult
NS_NewHTMLOptGroupElement(nsIHTMLContent** aResult, nsIAtom* aTag);

extern nsresult
NS_NewHTMLOptionElement(nsIHTMLContent** aResult, nsIAtom* aTag);

extern nsresult
NS_NewHTMLParagraphElement(nsIHTMLContent** aResult, nsIAtom* aTag);

extern nsresult
NS_NewHTMLParamElement(nsIHTMLContent** aResult, nsIAtom* aTag);

extern nsresult
NS_NewHTMLPreElement(nsIHTMLContent** aResult, nsIAtom* aTag);

extern nsresult
NS_NewHTMLQuoteElement(nsIHTMLContent** aResult, nsIAtom* aTag);

extern nsresult
NS_NewHTMLScriptElement(nsIHTMLContent** aResult, nsIAtom* aTag);

extern nsresult
NS_NewHTMLSelectElement(nsIHTMLContent** aResult, nsIAtom* aTag);

extern nsresult
NS_NewHTMLSpacerElement(nsIHTMLContent** aResult, nsIAtom* aTag);

extern nsresult
NS_NewHTMLSpanElement(nsIHTMLContent** aResult, nsIAtom* aTag);

extern nsresult
NS_NewHTMLStyleElement(nsIHTMLContent** aResult, nsIAtom* aTag);

extern nsresult
NS_NewHTMLTableCaptionElement(nsIHTMLContent** aResult,nsIAtom* aTag);

extern nsresult
NS_NewHTMLTableCellElement(nsIHTMLContent** aResult, nsIAtom* aTag);

extern nsresult
NS_NewHTMLTableColElement(nsIHTMLContent** aResult, nsIAtom* aTag);

extern nsresult
NS_NewHTMLTableColGroupElement(nsIHTMLContent** aResult, nsIAtom* aTag);

extern nsresult
NS_NewHTMLTableElement(nsIHTMLContent** aResult, nsIAtom* aTag);

extern nsresult
NS_NewHTMLTableRowElement(nsIHTMLContent** aResult, nsIAtom* aTag);

extern nsresult
NS_NewHTMLTableSectionElement(nsIHTMLContent** aResult,nsIAtom* aTag);

extern nsresult
NS_NewHTMLTbodyElement(nsIHTMLContent** aResult, nsIAtom* aTag);

extern nsresult
NS_NewHTMLTextAreaElement(nsIHTMLContent** aResult, nsIAtom* aTag);

extern nsresult
NS_NewHTMLTfootElement(nsIHTMLContent** aResult, nsIAtom* aTag);

extern nsresult
NS_NewHTMLTheadElement(nsIHTMLContent** aResult, nsIAtom* aTag);

extern nsresult
NS_NewHTMLTitleElement(nsIHTMLContent** aResult, nsIAtom* aTag);

extern nsresult
NS_NewHTMLUListElement(nsIHTMLContent** aResult, nsIAtom* aTag);

extern nsresult
NS_NewHTMLWBRElement(nsIHTMLContent** aResult, nsIAtom* aTag);

/**
 * Create a new content object for the given tag.
 * Returns NS_ERROR_NOT_AVAILABLE for an unknown/unhandled tag.
 * Returns some other error on error.
 * Returns NS_OK on success
 */
PR_EXTERN(nsresult)
NS_CreateHTMLElement(nsIHTMLContent** aResult,
                     const nsString& aTag);
PR_EXTERN(nsresult)
NS_CreateHTMLElement(nsIHTMLContent** aResult,
                     PRInt32 aID);

// Factory methods for creating html layout objects

// These are variations on AreaFrame with slightly different layout
// policies.

// Flags for block/area frames
#define NS_BLOCK_SHRINK_WRAP     0x00010000
#define NS_BLOCK_NO_AUTO_MARGINS 0x00020000
#define NS_BLOCK_MARGIN_ROOT     0x00040000
#define NS_BLOCK_SPACE_MGR       0x00080000
#define NS_BLOCK_WRAP_SIZE       0x00100000
#define NS_BLOCK_FLAGS_MASK      0x00ff0000

// Create a frame that supports "display: block" layout behavior
extern nsresult NS_NewBlockFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame,
                                 PRUint32   aFlags = 0);

// Special Generated Content Frame
extern nsresult
NS_NewAttributeContent(nsIContent ** aResult);

// Create a basic area frame but the GetFrameForPoint is overridden to always
// return the option frame 
// By default, area frames will extend
// their height to cover any children that "stick out".
extern nsresult NS_NewSelectsAreaFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame,
                                       PRUint32 aFlags = NS_BLOCK_WRAP_SIZE);

// Create a basic area frame. By default, area frames will extend
// their height to cover any children that "stick out".
extern nsresult NS_NewAreaFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame,
                                PRUint32 aFlags = NS_BLOCK_SPACE_MGR|NS_BLOCK_WRAP_SIZE);

// These AreaFrame's shrink wrap around their contents
inline nsresult NS_NewTableCellInnerFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame) {
  return NS_NewBlockFrame(aPresShell, aNewFrame,
                          NS_BLOCK_SPACE_MGR|NS_BLOCK_WRAP_SIZE|NS_BLOCK_MARGIN_ROOT);
}

// This type of AreaFrame is the document root, a margin root, and the
// initial containing block for absolutely positioned elements
inline nsresult NS_NewDocumentElementFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame) {
  return NS_NewAreaFrame(aPresShell, aNewFrame, NS_BLOCK_SPACE_MGR|NS_BLOCK_MARGIN_ROOT);
}

// This type of AreaFrame is a margin root, but does not shrink wrap
inline nsresult NS_NewAbsoluteItemWrapperFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame) {
  return NS_NewAreaFrame(aPresShell, aNewFrame, NS_BLOCK_SPACE_MGR|NS_BLOCK_MARGIN_ROOT);
}

// This type of AreaFrame shrink wraps
inline nsresult NS_NewFloatingItemWrapperFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame) {
  return NS_NewAreaFrame(aPresShell, aNewFrame, NS_BLOCK_SPACE_MGR|NS_BLOCK_SHRINK_WRAP);
}

// This type of AreaFrame doesn't use its own space manager and
// doesn't shrink wrap.
inline nsresult NS_NewRelativeItemWrapperFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame) {
  return NS_NewAreaFrame(aPresShell, aNewFrame);
}

extern nsresult NS_NewBRFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame);

extern nsresult NS_NewCommentFrame(nsIPresShell* aPresShell, nsIFrame** aFrameResult);
extern nsresult NS_NewHRFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame);

// <frame> and <iframe> 
extern nsresult NS_NewHTMLFrameOuterFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame);
// <frameset>
extern nsresult NS_NewHTMLFramesetFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame);

extern nsresult NS_NewViewportFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame);
extern nsresult NS_NewRootFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame);
extern nsresult NS_NewImageFrame(nsIPresShell* aPresShell, nsIFrame** aFrameResult);
extern nsresult NS_NewInlineFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame);
extern nsresult NS_NewPositionedInlineFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame);
extern nsresult NS_NewObjectFrame(nsIPresShell* aPresShell, nsIFrame** aFrameResult);
extern nsresult NS_NewSpacerFrame(nsIPresShell* aPresShell, nsIFrame** aResult);
extern nsresult NS_NewTextFrame(nsIPresShell* aPresShell, nsIFrame** aResult);
extern nsresult NS_NewContinuingTextFrame(nsIPresShell* aPresShell, nsIFrame** aResult);
extern nsresult NS_NewEmptyFrame(nsIPresShell* aPresShell, nsIFrame** aResult);
inline nsresult NS_NewWBRFrame(nsIPresShell* aPresShell, nsIFrame** aResult) {
  return NS_NewEmptyFrame(aPresShell, aResult);
}
extern nsresult NS_NewScrollFrame(nsIPresShell* aPresShell, nsIFrame** aResult);
extern nsresult NS_NewSimplePageSequenceFrame(nsIPresShell* aPresShell, nsIFrame** aResult);
extern nsresult NS_NewPageFrame(nsIPresShell* aPresShell, nsIFrame** aResult);
extern nsresult NS_NewFirstLetterFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame);
extern nsresult NS_NewFirstLineFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame);

// forms
extern nsresult NS_NewFormFrame(nsIPresShell* aPresShell, nsIFrame** aResult, PRUint32 aFlags);
extern nsresult NS_NewGfxButtonControlFrame(nsIPresShell* aPresShell, nsIFrame** aResult);
extern nsresult NS_NewNativeButtonControlFrame(nsIPresShell* aPresShell, nsIFrame** aResult);
extern nsresult NS_NewImageControlFrame(nsIPresShell* aPresShell, nsIFrame** aResult);
extern nsresult NS_NewHTMLButtonControlFrame(nsIPresShell* aPresShell, nsIFrame** aResult);
extern nsresult NS_NewGfxCheckboxControlFrame(nsIPresShell* aPresShell, nsIFrame** aResult);
extern nsresult NS_NewNativeCheckboxControlFrame(nsIPresShell* aPresShell, nsIFrame** aResult);
extern nsresult NS_NewFieldSetFrame(nsIPresShell* aPresShell, nsIFrame** aResult, PRUint32 aFlags);
extern nsresult NS_NewFileControlFrame(nsIPresShell* aPresShell, nsIFrame** aResult);
extern nsresult NS_NewLabelFrame(nsIPresShell* aPresShell, nsIFrame** aResult);
extern nsresult NS_NewLegendFrame(nsIPresShell* aPresShell, nsIFrame** aResult);
extern nsresult NS_NewNativeTextControlFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame);
extern nsresult NS_NewGfxTextControlFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame);
extern nsresult NS_NewGfxAutoTextControlFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame);
extern nsresult NS_NewGfxRadioControlFrame(nsIPresShell* aPresShell, nsIFrame** aResult);
extern nsresult NS_NewNativeRadioControlFrame(nsIPresShell* aPresShell, nsIFrame** aResult);
extern nsresult NS_NewNativeSelectControlFrame(nsIPresShell* aPresShell, nsIFrame** aResult);
extern nsresult NS_NewListControlFrame(nsIPresShell* aPresShell, nsIFrame** aResult);
extern nsresult NS_NewComboboxControlFrame(nsIPresShell* aPresShell, nsIFrame** aResult, PRUint32 aFlags);

// Table frame factories
extern nsresult NS_NewTableOuterFrame(nsIPresShell* aPresShell, nsIFrame** aResult);
extern nsresult NS_NewTableFrame(nsIPresShell* aPresShell, nsIFrame** aResult);
extern nsresult NS_NewTableCaptionFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame);

extern nsresult NS_NewTableColFrame(nsIPresShell* aPresShell, nsIFrame** aResult);
extern nsresult NS_NewTableColGroupFrame(nsIPresShell* aPresShell, nsIFrame** aResult);
extern nsresult NS_NewTableRowFrame(nsIPresShell* aPresShell, nsIFrame** aResult);
extern nsresult NS_NewTableRowGroupFrame(nsIPresShell* aPresShell, nsIFrame** aResult);
extern nsresult NS_NewTableCellFrame(nsIPresShell* aPresShell, nsIFrame** aResult);

// XXX passing aWebShell into this is wrong
extern nsresult NS_NewHTMLContentSink(nsIHTMLContentSink** aInstancePtrResult,
                                      nsIDocument* aDoc,
                                      nsIURI* aURL,
                                      nsIWebShell* aWebShell);
extern nsresult NS_NewHTMLFragmentContentSink(nsIHTMLFragmentContentSink** aInstancePtrResult);

/** Create a new HTML reflow command */
extern nsresult
NS_NewHTMLReflowCommand(nsIReflowCommand**           aInstancePtrResult,
                        nsIFrame*                    aTargetFrame,
                        nsIReflowCommand::ReflowType aReflowType,
                        nsIFrame*                    aChildFrame = nsnull,
                        nsIAtom*                     aAttribute = nsnull);

#endif /* nsHTMLParts_h___ */
