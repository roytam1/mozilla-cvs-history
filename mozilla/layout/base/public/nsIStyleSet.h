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
#ifndef nsStyleSet_h___
#define nsStyleSet_h___

#include <stdio.h>
#include "nsISupports.h"
#include "nslayout.h"

class nsIAtom;
class nsIStyleRule;
class nsIStyleSheet;
class nsIStyleContext;
class nsIPresContext;
class nsIContent;
class nsIFrame;
class nsIDocument;
class nsIFrameManager;

#include "nsVoidArray.h"
class nsISizeOfHandler;

// IID for the nsIStyleSet interface {e59396b0-b244-11d1-8031-006008159b5a}
#define NS_ISTYLE_SET_IID     \
{0xe59396b0, 0xb244, 0x11d1, {0x80, 0x31, 0x00, 0x60, 0x08, 0x15, 0x9b, 0x5a}}

class nsIStyleSet : public nsISupports {
public:
  static const nsIID& GetIID() { static nsIID iid = NS_ISTYLE_SET_IID; return iid; }

  // Style sheets are ordered, most significant first
  // NOTE: this is the reverse of the way documents store the sheets
  virtual void AppendOverrideStyleSheet(nsIStyleSheet* aSheet) = 0;
  virtual void InsertOverrideStyleSheetAfter(nsIStyleSheet* aSheet,
                                             nsIStyleSheet* aAfterSheet) = 0;
  virtual void InsertOverrideStyleSheetBefore(nsIStyleSheet* aSheet,
                                              nsIStyleSheet* aBeforeSheet) = 0;
  virtual void RemoveOverrideStyleSheet(nsIStyleSheet* aSheet) = 0;
  virtual PRInt32 GetNumberOfOverrideStyleSheets() = 0;
  virtual nsIStyleSheet* GetOverrideStyleSheetAt(PRInt32 aIndex) = 0;

  // the ordering of document style sheets is given by the document
  virtual void AddDocStyleSheet(nsIStyleSheet* aSheet, nsIDocument* aDocument) = 0;
  virtual void RemoveDocStyleSheet(nsIStyleSheet* aSheet) = 0;
  virtual PRInt32 GetNumberOfDocStyleSheets() = 0;
  virtual nsIStyleSheet* GetDocStyleSheetAt(PRInt32 aIndex) = 0;

  virtual void AppendBackstopStyleSheet(nsIStyleSheet* aSheet) = 0;
  virtual void InsertBackstopStyleSheetAfter(nsIStyleSheet* aSheet,
                                             nsIStyleSheet* aAfterSheet) = 0;
  virtual void InsertBackstopStyleSheetBefore(nsIStyleSheet* aSheet,
                                              nsIStyleSheet* aBeforeSheet) = 0;
  virtual void RemoveBackstopStyleSheet(nsIStyleSheet* aSheet) = 0;
  virtual PRInt32 GetNumberOfBackstopStyleSheets() = 0;
  virtual nsIStyleSheet* GetBackstopStyleSheetAt(PRInt32 aIndex) = 0;

  // get a style context for a non-pseudo frame
  virtual nsIStyleContext* ResolveStyleFor(nsIPresContext* aPresContext,
                                           nsIContent* aContent,
                                           nsIStyleContext* aParentContext,
                                           PRBool aForceUnique = PR_FALSE) = 0;

  // get a style context for a pseudo-frame (ie: tag = NS_NewAtom(":first-line");
  virtual nsIStyleContext* ResolvePseudoStyleFor(nsIPresContext* aPresContext,
                                                 nsIContent* aParentContent,
                                                 nsIAtom* aPseudoTag,
                                                 nsIStyleContext* aParentContext,
                                                 PRBool aForceUnique = PR_FALSE) = 0;

  // This funtions just like ResolvePseudoStyleFor except that it will
  // return nsnull if there are no explicit style rules for that
  // pseudo element
  virtual nsIStyleContext* ProbePseudoStyleFor(nsIPresContext* aPresContext,
                                               nsIContent* aParentContent,
                                               nsIAtom* aPseudoTag,
                                               nsIStyleContext* aParentContext,
                                               PRBool aForceUnique = PR_FALSE) = 0;

  // Get a new style context that lives in a different parent
  // The new context will be the same as the old if the new parent == the old parent
  NS_IMETHOD  ReParentStyleContext(nsIPresContext* aPresContext,
                                   nsIStyleContext* aStyleContext, 
                                   nsIStyleContext* aNewParentContext,
                                   nsIStyleContext** aNewStyleContext) = 0;

  // Test if style is dependent on content state
  NS_IMETHOD  HasStateDependentStyle(nsIPresContext* aPresContext,
                                     nsIContent*     aContent) = 0;

  // Create frames for the root content element and its child content
  NS_IMETHOD  ConstructRootFrame(nsIPresContext* aPresContext,
                                 nsIContent*     aDocElement,
                                 nsIFrame*&      aFrameSubTree) = 0;

  // Causes reconstruction of a frame hierarchy rooted by the
  // frame document element frame. This is often called when radical style
  // change precludes incremental reflow.
  NS_IMETHOD ReconstructDocElementHierarchy(nsIPresContext* aPresContext) = 0;

  // Notifications of changes to the content mpodel
  NS_IMETHOD ContentAppended(nsIPresContext* aPresContext,
                             nsIContent*     aContainer,
                             PRInt32         aNewIndexInContainer) = 0;
  NS_IMETHOD ContentInserted(nsIPresContext* aPresContext,
                             nsIContent*     aContainer,
                             nsIContent*     aChild,
                             PRInt32         aIndexInContainer) = 0;
  NS_IMETHOD ContentReplaced(nsIPresContext* aPresContext,
                             nsIContent*     aContainer,
                             nsIContent*     aOldChild,
                             nsIContent*     aNewChild,
                             PRInt32         aIndexInContainer) = 0;
  NS_IMETHOD ContentRemoved(nsIPresContext*  aPresContext,
                            nsIContent*      aContainer,
                            nsIContent*      aChild,
                            PRInt32          aIndexInContainer) = 0;

  NS_IMETHOD ContentChanged(nsIPresContext*  aPresContext,
                            nsIContent* aContent,
                            nsISupports* aSubContent) = 0;
  NS_IMETHOD ContentStatesChanged(nsIPresContext* aPresContext, 
                                  nsIContent* aContent1,
                                  nsIContent* aContent2) = 0;
  NS_IMETHOD AttributeChanged(nsIPresContext*  aPresContext,
                              nsIContent* aChild,
                              PRInt32 aNameSpaceID,
                              nsIAtom* aAttribute,
                              PRInt32 aHint) = 0; // See nsStyleConsts fot hint values

  // Style change notifications
  NS_IMETHOD StyleRuleChanged(nsIPresContext* aPresContext,
                              nsIStyleSheet* aStyleSheet,
                              nsIStyleRule* aStyleRule,
                              PRInt32 aHint) = 0; // See nsStyleConsts fot hint values
  NS_IMETHOD StyleRuleAdded(nsIPresContext* aPresContext,
                            nsIStyleSheet* aStyleSheet,
                            nsIStyleRule* aStyleRule) = 0;
  NS_IMETHOD StyleRuleRemoved(nsIPresContext* aPresContext,
                              nsIStyleSheet* aStyleSheet,
                              nsIStyleRule* aStyleRule) = 0;

  // Notification that we were unable to render a replaced element.
  // Called when the replaced element can not be rendered, and we should
  // instead render the element's contents.
  // The content object associated with aFrame should either be a IMG
  // element or an OBJECT element.
  NS_IMETHOD CantRenderReplacedElement(nsIPresContext* aPresContext,
                                       nsIFrame*       aFrame) = 0;

  // Request to create a continuing frame
  NS_IMETHOD CreateContinuingFrame(nsIPresContext* aPresContext,
                                   nsIFrame*       aFrame,
                                   nsIFrame*       aParentFrame,
                                   nsIFrame**      aContinuingFrame) = 0;

  // Request to find the primary frame associated with a given content object.
  // This is typically called by the pres shell when there is no mapping in
  // the pres shell hash table
  NS_IMETHOD FindPrimaryFrameFor(nsIPresContext*  aPresContext,
                                 nsIFrameManager* aFrameManager,
                                 nsIContent*      aContent,
                                 nsIFrame**       aFrame) = 0;

  virtual void List(FILE* out = stdout, PRInt32 aIndent = 0) = 0;
  virtual void ListContexts(nsIStyleContext* aRootContext, FILE* out = stdout, PRInt32 aIndent = 0) = 0;

  virtual void SizeOf(nsISizeOfHandler *aSizeofHandler, PRUint32 &aSize) = 0;
  virtual void ResetUniqueStyleItems(void) = 0;

  // If changing the given attribute cannot affect style context, aAffects
  // will be PR_FALSE on return.
  NS_IMETHOD AttributeAffectsStyle(nsIAtom *aAttribute, nsIContent *aContent,
                                   PRBool &aAffects) = 0;

#ifdef DEBUG_SC_SHARING
  // add and remove from the cache of all contexts
  NS_IMETHOD AddStyleContext(nsIStyleContext *aNewStyleContext) = 0;
  NS_IMETHOD RemoveStyleContext(nsIStyleContext *aNewStyleContext) = 0;
#endif
};

extern NS_LAYOUT nsresult
  NS_NewStyleSet(nsIStyleSet** aInstancePtrResult);


#define kUniqueItemsStartSize 128

class nsUniqueStyleItems : private nsVoidArray
{
public :
  // return a singleton instance of the nsUniqueStyleItems object
  static nsUniqueStyleItems *GetUniqueStyleItems( void ){
    if(mInstance == nsnull){
      nsUniqueStyleItems *pInstance = new nsUniqueStyleItems;
      NS_ASSERTION(pInstance == mInstance, "Singleton?");
    }
    return mInstance;
  }
  
  void *GetItem(void *aPtr){
    PRInt32 index = nsVoidArray::IndexOf(aPtr);
    if( index != -1){
      return nsVoidArray::ElementAt(index);
    } else {
      return nsnull;
    }
  }
  
  PRBool AddItem(void *aPtr){
    if(nsVoidArray::IndexOf(aPtr) == -1){
      return nsVoidArray::AppendElement(aPtr);
    } else {
      return PR_FALSE;
    }
  }
  
  PRBool RemoveItem(void *aPtr){
    return nsVoidArray::RemoveElement(aPtr);
  }

  PRInt32 Count(void){ 
    return nsVoidArray::Count(); 
  }

  void Clear(void){
    nsVoidArray::Clear();
  }
protected:
  // disallow these:
  nsUniqueStyleItems( const nsUniqueStyleItems& src);
  nsUniqueStyleItems& operator =(const nsUniqueStyleItems& src);

  // make this accessable to factory only
  nsUniqueStyleItems(void) : nsVoidArray(kUniqueItemsStartSize){
    NS_ASSERTION(mInstance == nsnull, "singleton?");
    mInstance=this;
  }

  static nsUniqueStyleItems *mInstance;
};

#define UNIQUE_STYLE_ITEMS(_name) \
  nsUniqueStyleItems* ##_name = nsUniqueStyleItems::GetUniqueStyleItems(); \
  NS_ASSERTION(##_name != nsnull, "UniqueItems cannot be null: error in nsUniqueStyleImtes factory");


#endif /* nsIStyleSet_h___ */
