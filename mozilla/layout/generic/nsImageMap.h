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
#ifndef nsImageMap_h___
#define nsImageMap_h___

#include "nslayout.h"
#include "nsISupports.h"
#include "nsCoord.h"
#include "nsVoidArray.h"
#include "nsIDocumentObserver.h"
#include "nsIDOMFocusListener.h"
#include "nsIFrame.h"

class nsIContent;
class nsIDOMHTMLAreaElement;
class nsIDOMHTMLMapElement;
class nsIPresContext;
class nsIRenderingContext;
class nsIURI;
class nsString;
class nsIDOMEvent;

class nsImageMap : public nsIDocumentObserver, public nsIDOMFocusListener
{
public:
  nsImageMap();

  nsresult Init(nsIPresShell* aPresShell, nsIFrame* aImageFrame, nsIDOMHTMLMapElement* aMap);

  /**
   * See if the given aX,aY <b>pixel</b> coordinates are in the image
   * map. If they are then NS_OK is returned and aAbsURL, aTarget,
   * aAltText, aSuppress are filled in with the values from the
   * underlying area tag. If the coordinates are not in the map
   * then NS_NOT_INSIDE is returned.
   */
  PRBool IsInside(nscoord aX, nscoord aY,
                  nsIContent** aContent,
                  nsString& aAbsURL,
                  nsString& aTarget,
                  nsString& aAltText,
                  PRBool* aSuppress);

  /**
   * See if the given aX,aY <b>pixel</b> coordinates are in the image
   * map. If they are then NS_OK is returned otherwise NS_NOT_INSIDE
   * is returned.
   */
  PRBool IsInside(nscoord aX, nscoord aY);

  void Draw(nsIPresContext* aCX, nsIRenderingContext& aRC);
  
  /** 
   * Called just before the nsImageFrame releases us. 
   * Used to break the cycle caused by the DOM listener.
   */
  void Destroy(void);
  
#ifdef DEBUG
  void SizeOf(nsISizeOfHandler* aHandler, PRUint32* aResult) const;
#endif

  // nsISupports
  NS_DECL_ISUPPORTS

  // nsIDocumentObserver
  NS_IMETHOD BeginUpdate(nsIDocument *aDocument);
  NS_IMETHOD EndUpdate(nsIDocument *aDocument);
  NS_IMETHOD BeginLoad(nsIDocument *aDocument);
  NS_IMETHOD EndLoad(nsIDocument *aDocument);
  NS_IMETHOD BeginReflow(nsIDocument *aDocument, nsIPresShell* aShell);
  NS_IMETHOD EndReflow(nsIDocument *aDocument, nsIPresShell* aShell);
  NS_IMETHOD ContentChanged(nsIDocument *aDocument,
                            nsIContent* aContent,
                            nsISupports* aSubContent);
  NS_IMETHOD ContentStatesChanged(nsIDocument* aDocument,
                                  nsIContent* aContent1,
                                  nsIContent* aContent2);
  NS_IMETHOD AttributeChanged(nsIDocument *aDocument,
                              nsIContent*  aContent,
                              PRInt32      aNameSpaceID,
                              nsIAtom*     aAttribute,
                              PRInt32      aModType, 
                              PRInt32      aHint);
  NS_IMETHOD ContentAppended(nsIDocument *aDocument,
                             nsIContent* aContainer,
                             PRInt32     aNewIndexInContainer);
  NS_IMETHOD ContentInserted(nsIDocument *aDocument,
                             nsIContent* aContainer,
                             nsIContent* aChild,
                             PRInt32 aIndexInContainer);
  NS_IMETHOD ContentReplaced(nsIDocument *aDocument,
                             nsIContent* aContainer,
                             nsIContent* aOldChild,
                             nsIContent* aNewChild,
                             PRInt32 aIndexInContainer);
  NS_IMETHOD ContentRemoved(nsIDocument *aDocument,
                            nsIContent* aContainer,
                            nsIContent* aChild,
                            PRInt32 aIndexInContainer);
  NS_IMETHOD StyleSheetAdded(nsIDocument *aDocument,
                             nsIStyleSheet* aStyleSheet);
  NS_IMETHOD StyleSheetRemoved(nsIDocument *aDocument,
                               nsIStyleSheet* aStyleSheet);
  NS_IMETHOD StyleSheetDisabledStateChanged(nsIDocument *aDocument,
                                            nsIStyleSheet* aStyleSheet,
                                            PRBool aDisabled);
  NS_IMETHOD StyleRuleChanged(nsIDocument *aDocument,
                              nsIStyleSheet* aStyleSheet,
                              nsIStyleRule* aStyleRule,
                              PRInt32 aHint);
  NS_IMETHOD StyleRuleAdded(nsIDocument *aDocument,
                            nsIStyleSheet* aStyleSheet,
                            nsIStyleRule* aStyleRule);
  NS_IMETHOD StyleRuleRemoved(nsIDocument *aDocument,
                              nsIStyleSheet* aStyleSheet,
                              nsIStyleRule* aStyleRule);
  NS_IMETHOD DocumentWillBeDestroyed(nsIDocument *aDocument);

  //nsIDOMFocusListener
  NS_IMETHOD Focus(nsIDOMEvent* aEvent);
  NS_IMETHOD Blur(nsIDOMEvent* aEvent);
  NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent);

protected:
  virtual ~nsImageMap();

  void FreeAreas();

  nsresult UpdateAreas();
  nsresult UpdateAreasForBlock(nsIContent* aParent, PRBool* aFoundAnchor);

  static PRBool IsAncestorOf(nsIContent* aContent,
                             nsIContent* aAncestorContent);

  nsresult AddArea(nsIContent* aArea);
 
  nsresult ChangeFocus(nsIDOMEvent* aEvent, PRBool aFocus);
  nsresult Invalidate(nsIPresContext* aPresContext, nsIFrame* aFrame, nsRect& aRect);

  nsIPresShell* mPresShell; // WEAK - owns the frame that owns us
  nsIFrame* mImageFrame;  // the frame that owns us
  nsIDocument* mDocument; // WEAK - the imagemap will not outlive the document
  nsIDOMHTMLMapElement* mDomMap;
  nsIContent* mMap;
  nsAutoVoidArray mAreas; // almost always has some entries
  PRBool mContainsBlockContents;
};

#endif /* nsImageMap_h___ */
