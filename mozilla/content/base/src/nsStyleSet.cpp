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
 *   Daniel Glazman <glazman@netscape.com>
 *
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
#include "nsCOMPtr.h"
#include "nsIStyleSet.h"
#include "nsIStyleSheet.h"
#include "nsIStyleRuleProcessor.h"
#include "nsIStyleRule.h"
#include "nsICSSStyleRule.h"
#include "nsISupportsArray.h"
#include "nsIFrame.h"
#include "nsIPresContext.h"
#include "nsIPresShell.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIStyleFrameConstruction.h"
#include "nsLayoutAtoms.h"
#include "nsTimer.h"
#include "nsICSSStyleSheet.h"
#include "nsNetUtil.h"
#include "nsIStyleRuleSupplier.h"
#include "nsRuleNode.h"
#include "nsRuleWalker.h"
#include "nsIHTMLDocument.h"
#include "nsIDOMHTMLBodyElement.h"
#include "nsHTMLAtoms.h"
#include "nsHashtable.h"

#ifdef MOZ_PERF_METRICS
  #include "nsITimeRecorder.h"
  #define STYLESET_START_TIMER(a) \
    StartTimer(a)
  #define STYLESET_STOP_TIMER(a) \
    StopTimer(a)
#else
  #define STYLESET_START_TIMER(a) ((void)0)
  #define STYLESET_STOP_TIMER(a) ((void)0)
#endif

#include "nsISizeOfHandler.h"

// =====================================================
// nsRuleNodeList
// A class that represents a chain of rule nodes

struct nsRuleNodeList
{
  nsRuleNodeList(nsRuleNode* aRuleNode, nsRuleNodeList* aNext = nsnull)
    :mRuleNode(aRuleNode), mNext(aNext)
  {};
    
  void* operator new(size_t sz, nsIPresContext* aContext) CPP_THROW_NEW {
    void* result = nsnull;
    aContext->AllocateFromShell(sz, &result);
    return result;
  };

  void Destroy() {
    if (mNext)
      mNext->Destroy();
    mRuleNode->PresContext()->FreeToShell(sizeof(nsRuleNodeList), this);
  };

  nsRuleNode* mRuleNode;
  nsRuleNodeList* mNext;
};

// =====================================================

class StyleSetImpl : public nsIStyleSet 
#ifdef MOZ_PERF_METRICS
                   , public nsITimeRecorder
#endif
{
public:
  StyleSetImpl();

  NS_DECL_ISUPPORTS

  virtual void AppendOverrideStyleSheet(nsIStyleSheet* aSheet);
  virtual void InsertOverrideStyleSheetAfter(nsIStyleSheet* aSheet,
                                             nsIStyleSheet* aAfterSheet);
  virtual void InsertOverrideStyleSheetBefore(nsIStyleSheet* aSheet,
                                              nsIStyleSheet* aBeforeSheet);
  virtual void RemoveOverrideStyleSheet(nsIStyleSheet* aSheet);
  virtual PRInt32 GetNumberOfOverrideStyleSheets();
  virtual nsIStyleSheet* GetOverrideStyleSheetAt(PRInt32 aIndex);

  virtual void AddDocStyleSheet(nsIStyleSheet* aSheet, nsIDocument* aDocument);
  virtual void RemoveDocStyleSheet(nsIStyleSheet* aSheet);
  virtual PRInt32 GetNumberOfDocStyleSheets();
  virtual nsIStyleSheet* GetDocStyleSheetAt(PRInt32 aIndex);

  virtual void AppendUserStyleSheet(nsIStyleSheet* aSheet);
  virtual void InsertUserStyleSheetAfter(nsIStyleSheet* aSheet,
                                             nsIStyleSheet* aAfterSheet);
  virtual void InsertUserStyleSheetBefore(nsIStyleSheet* aSheet,
                                              nsIStyleSheet* aBeforeSheet);
  virtual void RemoveUserStyleSheet(nsIStyleSheet* aSheet);
  virtual PRInt32 GetNumberOfUserStyleSheets();
  virtual nsIStyleSheet* GetUserStyleSheetAt(PRInt32 aIndex);
  virtual void ReplaceUserStyleSheets(nsISupportsArray* aNewSheets);

  virtual void AppendAgentStyleSheet(nsIStyleSheet* aSheet);
  virtual void InsertAgentStyleSheetAfter(nsIStyleSheet* aSheet,
                                             nsIStyleSheet* aAfterSheet);
  virtual void InsertAgentStyleSheetBefore(nsIStyleSheet* aSheet,
                                              nsIStyleSheet* aBeforeSheet);
  virtual void RemoveAgentStyleSheet(nsIStyleSheet* aSheet);
  virtual PRInt32 GetNumberOfAgentStyleSheets();
  virtual nsIStyleSheet* GetAgentStyleSheetAt(PRInt32 aIndex);
  virtual void ReplaceAgentStyleSheets(nsISupportsArray* aNewSheets);
  
  NS_IMETHOD EnableQuirkStyleSheet(PRBool aEnable);

  NS_IMETHOD NotifyStyleSheetStateChanged(PRBool aDisabled);

  virtual nsIStyleContext* ResolveStyleFor(nsIPresContext* aPresContext,
                                           nsIContent* aContent,
                                           nsIStyleContext* aParentContext);

  virtual nsIStyleContext* ResolveStyleForNonElement(
                                           nsIPresContext* aPresContext,
                                           nsIStyleContext* aParentContext);

  virtual nsIStyleContext* ResolvePseudoStyleFor(nsIPresContext* aPresContext,
                                                 nsIContent* aParentContent,
                                                 nsIAtom* aPseudoTag,
                                                 nsIStyleContext* aParentContext,
                                                 nsICSSPseudoComparator* aComparator = nsnull);

  virtual nsIStyleContext* ProbePseudoStyleFor(nsIPresContext* aPresContext,
                                               nsIContent* aParentContent,
                                               nsIAtom* aPseudoTag,
                                               nsIStyleContext* aParentContext);

  NS_IMETHOD Shutdown();

  // The following two methods can be used to tear down and reconstruct a rule tree.  The idea
  // is to first call BeginRuleTreeReconstruct, which will set aside the old rule
  // tree.  The entire frame tree should then have ReResolveStyleContext
  // called on it.  With the old rule tree hidden from view, the newly resolved style contexts will
  // resolve to rule nodes in a fresh rule tree, and the re-resolve system will properly compute
  // the visual impact of the changes.
  //
  // After re-resolution, call EndRuleTreeReconstruct() to finally discard the old rule tree.
  // This trick can be used in lieu of a full frame reconstruction when drastic style changes
  // happen (e.g., stylesheets being added/removed in the DOM, theme switching in the Mozilla app,
  // etc.
  virtual nsresult BeginRuleTreeReconstruct();
  virtual nsresult EndRuleTreeReconstruct();
  
  virtual nsresult GetRuleTree(nsRuleNode** aResult);
  virtual nsresult ClearCachedDataInRuleTree(nsIStyleRule* aRule);
  
  virtual nsresult AddRuleNodeMapping(nsRuleNode* aRuleNode);

  virtual nsresult ClearStyleData(nsIPresContext* aPresContext, nsIStyleRule* aRule, nsIStyleContext* aContext);

  virtual nsresult GetStyleFrameConstruction(nsIStyleFrameConstruction** aResult) {
    *aResult = mFrameConstructor;
    NS_IF_ADDREF(*aResult);
    return NS_OK;
  }

  NS_IMETHOD ReParentStyleContext(nsIPresContext* aPresContext,
                                  nsIStyleContext* aStyleContext, 
                                  nsIStyleContext* aNewParentContext,
                                  nsIStyleContext** aNewStyleContext);

  NS_IMETHOD HasStateDependentStyle(nsIPresContext* aPresContext,
                                    nsIContent*     aContent,
                                    PRInt32         aStateMask,
                                    PRBool*         aResult);

  NS_IMETHOD ConstructRootFrame(nsIPresContext* aPresContext,
                                nsIContent*     aContent,
                                nsIFrame*&      aFrameSubTree);
  NS_IMETHOD ReconstructDocElementHierarchy(nsIPresContext* aPresContext);
  NS_IMETHOD ContentAppended(nsIPresContext* aPresContext,
                             nsIContent*     aContainer,
                             PRInt32         aNewIndexInContainer);
  NS_IMETHOD ContentInserted(nsIPresContext* aPresContext,
                             nsIContent*     aContainer,
                             nsIContent*     aChild,
                             PRInt32         aIndexInContainer);
  NS_IMETHOD ContentReplaced(nsIPresContext* aPresContext,
                             nsIContent*     aContainer,
                             nsIContent*     aOldChild,
                             nsIContent*     aNewChild,
                             PRInt32         aIndexInContainer);
  NS_IMETHOD ContentRemoved(nsIPresContext* aPresContext,
                            nsIContent*     aContainer,
                            nsIContent*     aChild,
                            PRInt32         aIndexInContainer);

  NS_IMETHOD ContentChanged(nsIPresContext*  aPresContext,
                            nsIContent* aContent,
                            nsISupports* aSubContent);
  NS_IMETHOD ContentStatesChanged(nsIPresContext* aPresContext, 
                                  nsIContent* aContent1,
                                  nsIContent* aContent2,
                                  PRInt32 aStateMask);
  NS_IMETHOD AttributeChanged(nsIPresContext*  aPresContext,
                              nsIContent* aChild,
                              PRInt32 aNameSpaceID,
                              nsIAtom* aAttribute,
                              PRInt32 aModType, 
                              nsChangeHint aHint); // See nsStyleConsts fot hint values

  // xxx style rules enumeration

  // Style change notifications
  NS_IMETHOD StyleRuleChanged(nsIPresContext* aPresContext,
                              nsIStyleSheet* aStyleSheet,
                              nsIStyleRule* aStyleRule,
                              nsChangeHint aHint); // See nsStyleConsts fot hint values
  NS_IMETHOD StyleRuleAdded(nsIPresContext* aPresContext,
                            nsIStyleSheet* aStyleSheet,
                            nsIStyleRule* aStyleRule);
  NS_IMETHOD StyleRuleRemoved(nsIPresContext* aPresContext,
                              nsIStyleSheet* aStyleSheet,
                              nsIStyleRule* aStyleRule);

  // Notification that we were unable to render a replaced element.
  NS_IMETHOD CantRenderReplacedElement(nsIPresContext* aPresContext,
                                       nsIFrame*       aFrame);
  
  // Request to create a continuing frame
  NS_IMETHOD CreateContinuingFrame(nsIPresContext* aPresContext,
                                   nsIFrame*       aFrame,
                                   nsIFrame*       aParentFrame,
                                   nsIFrame**      aContinuingFrame);
  
  // Request to find the primary frame associated with a given content object.
  // This is typically called by the pres shell when there is no mapping in
  // the pres shell hash table
  NS_IMETHOD FindPrimaryFrameFor(nsIPresContext*  aPresContext,
                                 nsIFrameManager* aFrameManager,
                                 nsIContent*      aContent,
                                 nsIFrame**       aFrame,
                                 nsFindFrameHint* aHint);

  // Get the XBL insertion point for a child
  NS_IMETHOD GetInsertionPoint(nsIPresShell* aPresShell,
                               nsIFrame*     aParentFrame,
                               nsIContent*   aChildContent,
                               nsIFrame**    aInsertionPoint);

  // APIs for registering objects that can supply additional
  // rules during processing.
  NS_IMETHOD SetStyleRuleSupplier(nsIStyleRuleSupplier* aSupplier);
  NS_IMETHOD GetStyleRuleSupplier(nsIStyleRuleSupplier** aSupplier);

#ifdef DEBUG
  virtual void List(FILE* out = stdout, PRInt32 aIndent = 0);

  virtual void SizeOf(nsISizeOfHandler *aSizeofHandler, PRUint32 &aSize);
#endif
  virtual void ResetUniqueStyleItems(void);

#ifdef MOZ_PERF_METRICS
  NS_DECL_NSITIMERECORDER
#endif

  NS_IMETHOD AttributeAffectsStyle(nsIAtom *aAttribute, nsIContent *aContent,
                                   PRBool &aAffects);

private:
  static nsrefcnt gInstances;
  static nsIURI *gQuirkURI;

  // These are not supported and are not implemented!
  StyleSetImpl(const StyleSetImpl& aCopy);
  StyleSetImpl& operator=(const StyleSetImpl& aCopy);

protected:
  virtual ~StyleSetImpl();
  PRBool EnsureArray(nsCOMPtr<nsISupportsArray> &aArray);
  void RecycleArray(nsCOMPtr<nsISupportsArray> &aArray);
  
  void EnsureRuleWalker(nsIPresContext* aPresContext);

  void ClearRuleProcessors(void);
  void ClearAgentRuleProcessors(void);
  void ClearUserRuleProcessors(void);
  void ClearDocRuleProcessors(void);
  void ClearOverrideRuleProcessors(void);

  nsresult GatherRuleProcessors(void);

  void AddImportantRules(nsRuleNode* aCurrLevelNode, nsRuleNode* aLastPrevLevelNode);

  // Enumerate the rules in a way that cares about the order of the
  // rules.
  void FileRules(nsISupportsArrayEnumFunc aCollectorFunc,
                 RuleProcessorData* aData);

  // Enumerate all the rules in a way that doesn't care about the order
  // of the rules and break out if the enumeration is halted.
  void WalkRuleProcessors(nsISupportsArrayEnumFunc aFunc,
                          RuleProcessorData* aData);

  nsIStyleContext* GetContext(nsIPresContext* aPresContext, 
                              nsIStyleContext* aParentContext,
                              nsIAtom* aPseudoTag);

#ifdef DEBUG
  void  List(FILE* out, PRInt32 aIndent, nsISupportsArray* aSheets);
  void  ListContexts(nsIStyleContext* aRootContext, FILE* out, PRInt32 aIndent);
#endif

  nsCOMPtr<nsISupportsArray> mOverrideSheets;  // most significant first
  nsCOMPtr<nsISupportsArray> mDocSheets;       // " "
  nsCOMPtr<nsISupportsArray> mUserSheets;      // " "
  nsCOMPtr<nsISupportsArray> mAgentSheets;     // " "

  nsCOMPtr<nsISupportsArray> mAgentRuleProcessors;     // least significant first
  nsCOMPtr<nsISupportsArray> mUserRuleProcessors;      // " "
  nsCOMPtr<nsISupportsArray> mDocRuleProcessors;       // " "
  nsCOMPtr<nsISupportsArray> mOverrideRuleProcessors;  // " "

  nsCOMPtr<nsISupportsArray> mRecycler;

  nsIStyleFrameConstruction* mFrameConstructor;
  nsIStyleSheet*    mQuirkStyleSheet; // cached instance for enabling/disabling

  nsCOMPtr<nsIStyleRuleSupplier> mStyleRuleSupplier; 

  nsRuleNode* mRuleTree; // This is the root of our rule tree.  It is a lexicographic tree of
                         // matched rules that style contexts use to look up properties.
  nsRuleNode* mOldRuleTree; // Used during rule tree reconstruction.
  nsRuleWalker* mRuleWalker;   // This is an instance of a rule walker that can be used
                               // to navigate through our tree.
  nsHashtable mRuleMappings; // A hashtable from rules to rule node lists.

  MOZ_TIMER_DECLARE(mStyleResolutionWatch)

#ifdef MOZ_PERF_METRICS
  PRBool            mTimerEnabled;   // true if timing is enabled, false if disabled
#endif

};

nsrefcnt StyleSetImpl::gInstances = 0;
nsIURI *StyleSetImpl::gQuirkURI = 0;

StyleSetImpl::StyleSetImpl()
  : mFrameConstructor(nsnull),
    mQuirkStyleSheet(nsnull),
    mRuleTree(nsnull),
    mOldRuleTree(nsnull),
    mRuleWalker(nsnull),
    mRuleMappings(32)
#ifdef MOZ_PERF_METRICS
    ,mTimerEnabled(PR_FALSE)
#endif
{
  NS_INIT_ISUPPORTS();
  if (gInstances++ == 0)
  {
    static const char kQuirk_href[] = "resource:/res/quirk.css";
    NS_NewURI (&gQuirkURI, NS_LITERAL_CSTRING(kQuirk_href));
    NS_ASSERTION (gQuirkURI != 0, "Cannot allocate nsStyleSetImpl::gQuirkURI");
  }
}

StyleSetImpl::~StyleSetImpl()
{
  NS_IF_RELEASE(mFrameConstructor);
  NS_IF_RELEASE(mQuirkStyleSheet);
  if (--gInstances == 0)
  {
    NS_IF_RELEASE (gQuirkURI);
  }
}

#ifndef MOZ_PERF_METRICS
NS_IMPL_ISUPPORTS1(StyleSetImpl, nsIStyleSet)
#else
NS_IMPL_ISUPPORTS2(StyleSetImpl, nsIStyleSet, nsITimeRecorder)
#endif

PRBool StyleSetImpl::EnsureArray(nsCOMPtr<nsISupportsArray> &aArray)
{
  if (nsnull == aArray) {
    aArray = mRecycler;
    mRecycler = nsnull;
    if (nsnull == aArray) {
      if (NS_OK != NS_NewISupportsArray(getter_AddRefs(aArray))) {
        return PR_FALSE;
      }
    }
  }
  return PR_TRUE;
}

void
StyleSetImpl::RecycleArray(nsCOMPtr<nsISupportsArray> &aArray)
{
  if (!mRecycler) {
    mRecycler = aArray;  // take ref
    mRecycler->Clear();
    aArray = nsnull; 
  }
  else {  // already have a recycled array
    aArray = nsnull;
  }
}

void
StyleSetImpl::ClearRuleProcessors(void)
{
  ClearAgentRuleProcessors();
  ClearUserRuleProcessors();
  ClearDocRuleProcessors();
  ClearOverrideRuleProcessors();
}

void
StyleSetImpl::ClearAgentRuleProcessors(void)
{
  if (mAgentRuleProcessors)
    RecycleArray(mAgentRuleProcessors);
}

void
StyleSetImpl::ClearUserRuleProcessors(void)
{
  if (mUserRuleProcessors)
    RecycleArray(mUserRuleProcessors);
}

void
StyleSetImpl::ClearDocRuleProcessors(void)
{
  if (mDocRuleProcessors)
    RecycleArray(mDocRuleProcessors);
}

void
StyleSetImpl::ClearOverrideRuleProcessors(void)
{
  if (mOverrideRuleProcessors)
    RecycleArray(mOverrideRuleProcessors);
}

struct RuleProcessorEnumData {
  RuleProcessorEnumData(nsISupportsArray* aRuleProcessors) 
    : mRuleProcessors(aRuleProcessors),
      mPrevProcessor(nsnull)
  {}

  nsISupportsArray*       mRuleProcessors;
  nsIStyleRuleProcessor*  mPrevProcessor;
};

static PRBool
EnumRuleProcessor(nsISupports* aSheet, void* aData)
{
  nsIStyleSheet*  sheet = (nsIStyleSheet*)aSheet;
  RuleProcessorEnumData* data = (RuleProcessorEnumData*)aData;

  nsIStyleRuleProcessor* processor = nsnull;
  nsresult result = sheet->GetStyleRuleProcessor(processor, data->mPrevProcessor);
  if (NS_SUCCEEDED(result) && processor) {
    if (processor != data->mPrevProcessor) {
      data->mRuleProcessors->AppendElement(processor);
      data->mPrevProcessor = processor; // ref is held by array
    }
    NS_RELEASE(processor);
  }
  return PR_TRUE;
}

nsresult
StyleSetImpl::GatherRuleProcessors(void)
{
  nsresult result = NS_ERROR_OUT_OF_MEMORY;
  if (mAgentSheets && !mAgentRuleProcessors) {
    if (EnsureArray(mAgentRuleProcessors)) {
      RuleProcessorEnumData data(mAgentRuleProcessors);
      mAgentSheets->EnumerateBackwards(EnumRuleProcessor, &data);
      PRUint32 count;
      mAgentRuleProcessors->Count(&count);
      if (0 == count) {
        RecycleArray(mAgentRuleProcessors);
      }
    } else return result;
  }

  if (mUserSheets && !mUserRuleProcessors) {
    if (EnsureArray(mUserRuleProcessors)) {
      RuleProcessorEnumData data(mUserRuleProcessors);
      mUserSheets->EnumerateBackwards(EnumRuleProcessor, &data);
      PRUint32 count;
      mUserRuleProcessors->Count(&count);
      if (0 == count) {
        RecycleArray(mUserRuleProcessors);
      }
    } else return result;
  }

  if (mDocSheets && !mDocRuleProcessors) {
    if (EnsureArray(mDocRuleProcessors)) {
      RuleProcessorEnumData data(mDocRuleProcessors);
      mDocSheets->EnumerateBackwards(EnumRuleProcessor, &data);
      PRUint32 count;
      mDocRuleProcessors->Count(&count);
      if (0 == count) {
        RecycleArray(mDocRuleProcessors);
      }
    } else return result;
  }

  if (mOverrideSheets && !mOverrideRuleProcessors) {
    if (EnsureArray(mOverrideRuleProcessors)) {
      RuleProcessorEnumData data(mOverrideRuleProcessors);
      mOverrideSheets->EnumerateBackwards(EnumRuleProcessor, &data);
      PRUint32 count;
      mOverrideRuleProcessors->Count(&count);
      if (0 == count) {
        RecycleArray(mOverrideRuleProcessors);
      }
    } else return result;
  }

  return NS_OK;
}


// ----- Override sheets

void StyleSetImpl::AppendOverrideStyleSheet(nsIStyleSheet* aSheet)
{
  NS_PRECONDITION(nsnull != aSheet, "null arg");
  if (EnsureArray(mOverrideSheets)) {
    mOverrideSheets->RemoveElement(aSheet);
    mOverrideSheets->AppendElement(aSheet);
    ClearOverrideRuleProcessors();
  }
}

void StyleSetImpl::InsertOverrideStyleSheetAfter(nsIStyleSheet* aSheet,
                                                 nsIStyleSheet* aAfterSheet)
{
  NS_PRECONDITION(nsnull != aSheet, "null arg");
  if (EnsureArray(mOverrideSheets)) {
    mOverrideSheets->RemoveElement(aSheet);
    PRInt32 index = mOverrideSheets->IndexOf(aAfterSheet);
    mOverrideSheets->InsertElementAt(aSheet, ++index);
    ClearOverrideRuleProcessors();
  }
}

void StyleSetImpl::InsertOverrideStyleSheetBefore(nsIStyleSheet* aSheet,
                                                  nsIStyleSheet* aBeforeSheet)
{
  NS_PRECONDITION(nsnull != aSheet, "null arg");
  if (EnsureArray(mOverrideSheets)) {
    mOverrideSheets->RemoveElement(aSheet);
    PRInt32 index = mOverrideSheets->IndexOf(aBeforeSheet);
    mOverrideSheets->InsertElementAt(aSheet, ((-1 < index) ? index : 0));
    ClearOverrideRuleProcessors();
  }
}

void StyleSetImpl::RemoveOverrideStyleSheet(nsIStyleSheet* aSheet)
{
  NS_PRECONDITION(nsnull != aSheet, "null arg");

  if (nsnull != mOverrideSheets) {
    mOverrideSheets->RemoveElement(aSheet);
    ClearOverrideRuleProcessors();
  }
}

PRInt32 StyleSetImpl::GetNumberOfOverrideStyleSheets()
{
  if (nsnull != mOverrideSheets) {
    PRUint32 cnt;
    nsresult rv = mOverrideSheets->Count(&cnt);
    if (NS_FAILED(rv)) return 0;        // XXX error?
    return cnt;
  }
  return 0;
}

nsIStyleSheet* StyleSetImpl::GetOverrideStyleSheetAt(PRInt32 aIndex)
{
  nsIStyleSheet* sheet = nsnull;
  if (nsnull != mOverrideSheets) {
    sheet = (nsIStyleSheet*)mOverrideSheets->ElementAt(aIndex);
  }
  return sheet;
}

// -------- Doc Sheets

void StyleSetImpl::AddDocStyleSheet(nsIStyleSheet* aSheet, nsIDocument* aDocument)
{
  NS_PRECONDITION((nsnull != aSheet) && (nsnull != aDocument), "null arg");
  if (EnsureArray(mDocSheets)) {
    mDocSheets->RemoveElement(aSheet);
    // lowest index last
    PRInt32 newDocIndex = 0;
    aDocument->GetIndexOfStyleSheet(aSheet, &newDocIndex);
    PRUint32 count;
    nsresult rv = mDocSheets->Count(&count);
    if (NS_FAILED(rv)) return;  // XXX error?
    PRUint32 index;
    for (index = 0; index < count; index++) {
      nsIStyleSheet* sheet = (nsIStyleSheet*)mDocSheets->ElementAt(index);
      PRInt32 sheetDocIndex = 0;
      aDocument->GetIndexOfStyleSheet(sheet, &sheetDocIndex);
      if (sheetDocIndex < newDocIndex) {
        mDocSheets->InsertElementAt(aSheet, index);
        index = count; // break loop
      }
      NS_RELEASE(sheet);
    }
    PRUint32 cnt;
    rv = mDocSheets->Count(&cnt);
    if (NS_FAILED(rv)) return;  // XXX error?
    if (cnt == count) {  // didn't insert it
      mDocSheets->AppendElement(aSheet);
    }

    if (nsnull == mFrameConstructor) {
      CallQueryInterface(aSheet, &mFrameConstructor);
    }
    ClearDocRuleProcessors();
  }
}

void StyleSetImpl::RemoveDocStyleSheet(nsIStyleSheet* aSheet)
{
  NS_PRECONDITION(nsnull != aSheet, "null arg");

  if (nsnull != mDocSheets) {
    mDocSheets->RemoveElement(aSheet);
    ClearDocRuleProcessors();
  }
}

PRInt32 StyleSetImpl::GetNumberOfDocStyleSheets()
{
  if (nsnull != mDocSheets) {
    PRUint32 cnt;
    nsresult rv = mDocSheets->Count(&cnt);
    if (NS_FAILED(rv)) return 0;        // XXX error?
    return cnt;
  }
  return 0;
}

nsIStyleSheet* StyleSetImpl::GetDocStyleSheetAt(PRInt32 aIndex)
{
  nsIStyleSheet* sheet = nsnull;
  if (nsnull != mDocSheets) {
    sheet = (nsIStyleSheet*)mDocSheets->ElementAt(aIndex);
  }
  return sheet;
}

// ------ user sheets

void StyleSetImpl::AppendUserStyleSheet(nsIStyleSheet* aSheet)
{
  NS_PRECONDITION(nsnull != aSheet, "null arg");
  if (EnsureArray(mUserSheets)) {
    mUserSheets->RemoveElement(aSheet);
    mUserSheets->AppendElement(aSheet);
    ClearUserRuleProcessors();
  }
}

void StyleSetImpl::InsertUserStyleSheetAfter(nsIStyleSheet* aSheet,
                                             nsIStyleSheet* aAfterSheet)
{
  NS_PRECONDITION(nsnull != aSheet, "null arg");
  if (EnsureArray(mUserSheets)) {
    mUserSheets->RemoveElement(aSheet);
    PRInt32 index = mUserSheets->IndexOf(aAfterSheet);
    mUserSheets->InsertElementAt(aSheet, ++index);
    ClearUserRuleProcessors();
  }
}

void StyleSetImpl::InsertUserStyleSheetBefore(nsIStyleSheet* aSheet,
                                              nsIStyleSheet* aBeforeSheet)
{
  NS_PRECONDITION(nsnull != aSheet, "null arg");
  if (EnsureArray(mUserSheets)) {
    mUserSheets->RemoveElement(aSheet);
    PRInt32 index = mUserSheets->IndexOf(aBeforeSheet);
    mUserSheets->InsertElementAt(aSheet, ((-1 < index) ? index : 0));
    ClearUserRuleProcessors();
  }
}

void StyleSetImpl::RemoveUserStyleSheet(nsIStyleSheet* aSheet)
{
  NS_PRECONDITION(nsnull != aSheet, "null arg");

  if (nsnull != mUserSheets) {
    mUserSheets->RemoveElement(aSheet);
    ClearUserRuleProcessors();
  }
}

PRInt32 StyleSetImpl::GetNumberOfUserStyleSheets()
{
  if (nsnull != mUserSheets) {
    PRUint32 cnt;
    nsresult rv = mUserSheets->Count(&cnt);
    if (NS_FAILED(rv)) return 0;        // XXX error?
    return cnt;
  }
  return 0;
}

nsIStyleSheet* StyleSetImpl::GetUserStyleSheetAt(PRInt32 aIndex)
{
  nsIStyleSheet* sheet = nsnull;
  if (nsnull != mUserSheets) {
    sheet = (nsIStyleSheet*)mUserSheets->ElementAt(aIndex);
  }
  return sheet;
}

void
StyleSetImpl::ReplaceUserStyleSheets(nsISupportsArray* aNewUserSheets)
{
  ClearUserRuleProcessors();
  mUserSheets = aNewUserSheets;
}

// ------ agent sheets

void StyleSetImpl::AppendAgentStyleSheet(nsIStyleSheet* aSheet)
{
  NS_PRECONDITION(nsnull != aSheet, "null arg");
  if (EnsureArray(mAgentSheets)) {
    mAgentSheets->RemoveElement(aSheet);
    mAgentSheets->AppendElement(aSheet);
    ClearAgentRuleProcessors();
  }
}

void StyleSetImpl::InsertAgentStyleSheetAfter(nsIStyleSheet* aSheet,
                                              nsIStyleSheet* aAfterSheet)
{
  NS_PRECONDITION(nsnull != aSheet, "null arg");
  if (EnsureArray(mAgentSheets)) {
    mAgentSheets->RemoveElement(aSheet);
    PRInt32 index = mAgentSheets->IndexOf(aAfterSheet);
    mAgentSheets->InsertElementAt(aSheet, ++index);
    ClearAgentRuleProcessors();
  }
}

void StyleSetImpl::InsertAgentStyleSheetBefore(nsIStyleSheet* aSheet,
                                               nsIStyleSheet* aBeforeSheet)
{
  NS_PRECONDITION(nsnull != aSheet, "null arg");
  if (EnsureArray(mAgentSheets)) {
    mAgentSheets->RemoveElement(aSheet);
    PRInt32 index = mAgentSheets->IndexOf(aBeforeSheet);
    mAgentSheets->InsertElementAt(aSheet, ((-1 < index) ? index : 0));
    ClearAgentRuleProcessors();
  }
}

void StyleSetImpl::RemoveAgentStyleSheet(nsIStyleSheet* aSheet)
{
  NS_PRECONDITION(nsnull != aSheet, "null arg");

  if (nsnull != mAgentSheets) {
    mAgentSheets->RemoveElement(aSheet);
    ClearAgentRuleProcessors();
  }
}

PRInt32 StyleSetImpl::GetNumberOfAgentStyleSheets()
{
  if (nsnull != mAgentSheets) {
    PRUint32 cnt;
    nsresult rv = mAgentSheets->Count(&cnt);
    if (NS_FAILED(rv)) return 0;        // XXX error?
    return cnt;
 }
  return 0;
}

NS_IMETHODIMP StyleSetImpl::EnableQuirkStyleSheet(PRBool aEnable)
{
  nsresult rv = NS_OK;
  if (nsnull == mQuirkStyleSheet) {
    // first find the quirk sheet:
    // - run through all of the agent sheets and check for a CSSStyleSheet that
    //   has the URL we want
    PRUint32 i, nSheets = GetNumberOfAgentStyleSheets();
    for (i=0; i< nSheets; i++) {
      nsCOMPtr<nsIStyleSheet> sheet;
      sheet = getter_AddRefs(GetAgentStyleSheetAt(i));
      if (sheet) {
        nsCOMPtr<nsICSSStyleSheet> cssSheet;
        sheet->QueryInterface(NS_GET_IID(nsICSSStyleSheet), getter_AddRefs(cssSheet));
        if (cssSheet) {
          nsCOMPtr<nsIStyleSheet> quirkSheet;
          PRBool bHasSheet = PR_FALSE;
          NS_ASSERTION(gQuirkURI != nsnull, "StyleSetImpl::gQuirkStyleSet is not initialized!");
          if (gQuirkURI != nsnull
              && NS_SUCCEEDED(cssSheet->ContainsStyleSheet(gQuirkURI, bHasSheet, 
                                                           getter_AddRefs(quirkSheet))) 
              && bHasSheet) {
            NS_ASSERTION(quirkSheet, "QuirkSheet must be set: ContainsStyleSheet is hosed");
            // cache the sheet for faster lookup next time
            mQuirkStyleSheet = quirkSheet.get();
            // addref for our cached reference
            NS_ADDREF(mQuirkStyleSheet);
            // only one quirk style sheet can exist, so stop looking
            break;
          }
        }
      }
    }
  }
  NS_ASSERTION(mQuirkStyleSheet, "no quirk stylesheet");
  if (mQuirkStyleSheet) {
#if defined(DEBUG_warren) || defined(DEBUG_attinasi)
    printf( "%s Quirk StyleSheet\n", aEnable ? "Enabling" : "Disabling" );
#endif
#ifdef DEBUG_dbaron // XXX Make this |DEBUG| once it stops firing.
    PRUint32 count = 0;
    if (mAgentRuleProcessors)
      mAgentRuleProcessors->Count(&count);
    PRBool enabledNow;
    mQuirkStyleSheet->GetEnabled(enabledNow);
    NS_ASSERTION(count == 0 || aEnable == enabledNow,
                 "enabling/disabling quirk stylesheet too late");
    if (count != 0 && aEnable == enabledNow)
      printf("WARNING: We set the quirks mode too many times.\n"); // we do!
#endif
    mQuirkStyleSheet->SetEnabled(aEnable);
  }
  return rv;
}

nsIStyleSheet* StyleSetImpl::GetAgentStyleSheetAt(PRInt32 aIndex)
{
  nsIStyleSheet* sheet = nsnull;
  if (nsnull != mAgentSheets) {
    sheet = (nsIStyleSheet*)mAgentSheets->ElementAt(aIndex);
  }
  return sheet;
}

void
StyleSetImpl::ReplaceAgentStyleSheets(nsISupportsArray* aNewAgentSheets)
{
  ClearAgentRuleProcessors();
  mAgentSheets = aNewAgentSheets;
}

NS_IMETHODIMP 
StyleSetImpl::NotifyStyleSheetStateChanged(PRBool aDisabled)
{
  ClearRuleProcessors();
  GatherRuleProcessors();
  return NS_OK;
}

 
struct RulesMatchingData : public ElementRuleProcessorData {
  RulesMatchingData(nsIPresContext* aPresContext,
                    nsIAtom* aMedium,
                    nsIContent* aContent,
                    nsRuleWalker* aRuleWalker)
    : ElementRuleProcessorData(aPresContext, aContent, aRuleWalker),
      mMedium(aMedium)
  {
  }
  nsIAtom*          mMedium;
};

static PRBool
EnumRulesMatching(nsISupports* aProcessor, void* aData)
{
  nsIStyleRuleProcessor*  processor = (nsIStyleRuleProcessor*)aProcessor;
  RulesMatchingData* data = (RulesMatchingData*)aData;

  processor->RulesMatching(data, data->mMedium);
  return PR_TRUE;
}

/**
 * |GetContext| implements sharing of style contexts (not just the data
 * on the rule nodes) between siblings and cousins of the same
 * generation.  (It works for cousins of the same generation since
 * |aParentContext| could itself be a shared context.)
 */
nsIStyleContext* StyleSetImpl::GetContext(nsIPresContext* aPresContext, 
                                          nsIStyleContext* aParentContext, 
                                          nsIAtom* aPseudoTag)
{
  nsIStyleContext* result = nsnull;
  nsRuleNode* ruleNode = mRuleWalker->GetCurrentNode();
      
  if (aParentContext)
    aParentContext->FindChildWithRules(aPseudoTag, ruleNode, result);

#ifdef NOISY_DEBUG
  if (result)
    fprintf(stdout, "--- SharedSC %d ---\n", ++gSharedCount);
  else
    fprintf(stdout, "+++ NewSC %d +++\n", ++gNewCount);
#endif

  if (!result)
    NS_NewStyleContext(&result, aParentContext, aPseudoTag, ruleNode,
                       aPresContext);

  return result;
}

void
StyleSetImpl::AddImportantRules(nsRuleNode* aCurrLevelNode,
                                nsRuleNode* aLastPrevLevelNode)
{
  if (!aCurrLevelNode || aCurrLevelNode == aLastPrevLevelNode)
    return;

  AddImportantRules(aCurrLevelNode->GetParent(), aLastPrevLevelNode);

  nsCOMPtr<nsIStyleRule> rule;;
  aCurrLevelNode->GetRule(getter_AddRefs(rule));
  nsCOMPtr<nsICSSStyleRule> cssRule(do_QueryInterface(rule));
  if (cssRule) {
    nsCOMPtr<nsIStyleRule> impRule = cssRule->GetImportantRule();
    if (impRule)
      mRuleWalker->Forward(impRule);
  }
}

// Enumerate the rules in a way that cares about the order of the rules.
void
StyleSetImpl::FileRules(nsISupportsArrayEnumFunc aCollectorFunc, 
                        RuleProcessorData* aData)
{

  // Cascading order:
  // [least important]
  //  1. UA normal rules                    = Agent     normal
  //  2. User normal rules                  = User      normal
  //  3. Author normal rules                = Document  normal
  //  4. Override normal rules              = Override  normal
  //  5. Author !important rules            = Document !important
  //  6. Override !important rules          = Override !important
  //  7. User !important rules              = User     !important
  //  8. UA !important rules                = Agent    !important
  // [most important]

  nsRuleNode* lastAgentRN = nsnull;
  if (mAgentRuleProcessors) {
    mAgentRuleProcessors->EnumerateForwards(aCollectorFunc, aData);
    lastAgentRN = mRuleWalker->GetCurrentNode();
  }

  nsRuleNode* lastUserRN = lastAgentRN;
  if (mUserRuleProcessors) {
    mUserRuleProcessors->EnumerateForwards(aCollectorFunc, aData);
    lastUserRN = mRuleWalker->GetCurrentNode();
  }

  nsRuleNode* lastDocRN = lastUserRN;
  PRBool useRuleProcessors = PR_TRUE;
  if (mStyleRuleSupplier) {
    // We can supply additional document-level sheets that should be walked.
    mStyleRuleSupplier->WalkRules(this, aCollectorFunc, aData);
    mStyleRuleSupplier->UseDocumentRules(aData->mContent, &useRuleProcessors);
  }
  if (mDocRuleProcessors && useRuleProcessors) {
    mDocRuleProcessors->EnumerateForwards(aCollectorFunc, aData);
  }
  lastDocRN = mRuleWalker->GetCurrentNode();

  nsRuleNode* lastOvrRN = lastDocRN;
  if (mOverrideRuleProcessors) {
    mOverrideRuleProcessors->EnumerateForwards(aCollectorFunc, aData);
    lastOvrRN = mRuleWalker->GetCurrentNode();
  }

  AddImportantRules(lastDocRN, lastUserRN);   //doc
  AddImportantRules(lastOvrRN, lastDocRN);    //ovr
  AddImportantRules(lastUserRN, lastAgentRN); //user
  AddImportantRules(lastAgentRN, nsnull);     //agent

}

// Enumerate all the rules in a way that doesn't care about the order
// of the rules and break out if the enumeration is halted.
void
StyleSetImpl::WalkRuleProcessors(nsISupportsArrayEnumFunc aFunc,
                                 RuleProcessorData* aData)
{
  // Walk the agent rules first.
  if (mAgentRuleProcessors)
    if (!mAgentRuleProcessors->EnumerateForwards(aFunc, aData))
      return;

  // Walk the user rules next.
  if (mUserRuleProcessors)
    if (!mUserRuleProcessors->EnumerateForwards(aFunc, aData))
      return;

  PRBool useRuleProcessors = PR_TRUE;
  if (mStyleRuleSupplier) {
    // We can supply additional document-level sheets that should be walked.
    // XXX We ignore whether the enumerator wants to halt here!
    mStyleRuleSupplier->WalkRules(this, aFunc, aData);
    mStyleRuleSupplier->UseDocumentRules(aData->mContent, &useRuleProcessors);
  }

  // Now walk the doc rules.
  if (mDocRuleProcessors && useRuleProcessors)
    if (!mDocRuleProcessors->EnumerateForwards(aFunc, aData))
      return;
  
  // Walk the override rules last.
  if (mOverrideRuleProcessors)
    mOverrideRuleProcessors->EnumerateForwards(aFunc, aData);
}

#ifdef NS_DEBUG
#define NS_ASSERT_REFCOUNT(ptr,cnt,msg) { \
  nsrefcnt  count = ptr->AddRef();        \
  ptr->Release();                         \
  NS_ASSERTION(--count == cnt, msg);      \
}
#else
#define NS_ASSERT_REFCOUNT(ptr,cnt,msg) {}
#endif

void StyleSetImpl::EnsureRuleWalker(nsIPresContext* aPresContext)
{ 
  if (mRuleWalker)
    return;

  nsRuleNode::CreateRootNode(aPresContext, &mRuleTree);
  mRuleWalker = new nsRuleWalker(mRuleTree);
}

nsIStyleContext* StyleSetImpl::ResolveStyleFor(nsIPresContext* aPresContext,
                                               nsIContent* aContent,
                                               nsIStyleContext* aParentContext)
{
  MOZ_TIMER_DEBUGLOG(("Start: StyleSetImpl::ResolveStyleFor(), this=%p\n", this));
  STYLESET_START_TIMER(NS_TIMER_STYLE_RESOLUTION);

  nsIStyleContext*  result = nsnull;

  NS_ASSERTION(aContent, "must have content");
  NS_ASSERTION(aPresContext, "must have pres context");
  NS_ASSERTION(aContent->IsContentOfType(nsIContent::eELEMENT),
               "content must be element");

  if (aContent && aPresContext) {
    GatherRuleProcessors();
    if (mAgentRuleProcessors ||
        mUserRuleProcessors  ||
        mDocRuleProcessors   ||
        mOverrideRuleProcessors) {
      EnsureRuleWalker(aPresContext);
      nsCOMPtr<nsIAtom> medium;
      aPresContext->GetMedium(getter_AddRefs(medium));
      RulesMatchingData data(aPresContext, medium, aContent, mRuleWalker);
      FileRules(EnumRulesMatching, &data);
      result = GetContext(aPresContext, aParentContext, nsnull);
     
      // Now reset the walker back to the root of the tree.
      mRuleWalker->Reset();
    }
  }

  MOZ_TIMER_DEBUGLOG(("Stop: StyleSetImpl::ResolveStyleFor(), this=%p\n", this));
  STYLESET_STOP_TIMER(NS_TIMER_STYLE_RESOLUTION);
  return result;
}

nsIStyleContext* StyleSetImpl::ResolveStyleForNonElement(
                                               nsIPresContext* aPresContext,
                                               nsIStyleContext* aParentContext)
{
  MOZ_TIMER_DEBUGLOG(("Start: StyleSetImpl::ResolveStyleForNonElement(), this=%p\n", this));
  STYLESET_START_TIMER(NS_TIMER_STYLE_RESOLUTION);

  nsIStyleContext* result = nsnull;

  NS_ASSERTION(aPresContext, "must have pres context");

  if (aPresContext) {
    GatherRuleProcessors();
    if (mAgentRuleProcessors ||
        mUserRuleProcessors  ||
        mDocRuleProcessors   ||
        mOverrideRuleProcessors) {
      EnsureRuleWalker(aPresContext);
      result = GetContext(aPresContext, aParentContext,
                          nsHTMLAtoms::mozNonElementPseudo);
      NS_ASSERTION(mRuleWalker->AtRoot(), "rule walker must be at root");
    }
  }

  MOZ_TIMER_DEBUGLOG(("Stop: StyleSetImpl::ResolveStyleForNonElement(), this=%p\n", this));
  STYLESET_STOP_TIMER(NS_TIMER_STYLE_RESOLUTION);
  return result;
}


struct PseudoRulesMatchingData : public PseudoRuleProcessorData {
  PseudoRulesMatchingData(nsIPresContext* aPresContext,
                          nsIAtom* aMedium,
                          nsIContent* aParentContent,
                          nsIAtom* aPseudoTag,
                          nsICSSPseudoComparator* aComparator,
                          nsRuleWalker* aRuleWalker)
    : PseudoRuleProcessorData(aPresContext, aParentContent, aPseudoTag, aComparator,
                     aRuleWalker),
      mMedium(aMedium)
  {
  }
  nsIAtom*                mMedium;
};

static PRBool
EnumPseudoRulesMatching(nsISupports* aProcessor, void* aData)
{
  nsIStyleRuleProcessor*  processor = (nsIStyleRuleProcessor*)aProcessor;
  PseudoRulesMatchingData* data = (PseudoRulesMatchingData*)aData;

  processor->RulesMatching(data, data->mMedium);
  return PR_TRUE;
}

nsIStyleContext* StyleSetImpl::ResolvePseudoStyleFor(nsIPresContext* aPresContext,
                                                     nsIContent* aParentContent,
                                                     nsIAtom* aPseudoTag,
                                                     nsIStyleContext* aParentContext,
                                                     nsICSSPseudoComparator* aComparator)
{
  MOZ_TIMER_DEBUGLOG(("Start: StyleSetImpl::ResolvePseudoStyleFor(), this=%p\n", this));
  STYLESET_START_TIMER(NS_TIMER_STYLE_RESOLUTION);

  nsIStyleContext*  result = nsnull;

  NS_ASSERTION(aPseudoTag, "must have pseudo tag");
  NS_ASSERTION(aPresContext, "must have pres context");
  NS_ASSERTION(!aParentContent ||
               aParentContent->IsContentOfType(nsIContent::eELEMENT),
               "content (if non-null) must be element");

  if (aPseudoTag && aPresContext) {
    GatherRuleProcessors();
    if (mAgentRuleProcessors ||
        mUserRuleProcessors  ||
        mDocRuleProcessors   ||
        mOverrideRuleProcessors) {
      nsCOMPtr<nsIAtom> medium;
      aPresContext->GetMedium(getter_AddRefs(medium));
      EnsureRuleWalker(aPresContext);
      PseudoRulesMatchingData data(aPresContext, medium, aParentContent, 
                                   aPseudoTag, aComparator, mRuleWalker);
      FileRules(EnumPseudoRulesMatching, &data);

      result = GetContext(aPresContext, aParentContext, aPseudoTag);
     
      // Now reset the walker back to the root of the tree.
      mRuleWalker->Reset();
    }
  }

  MOZ_TIMER_DEBUGLOG(("Stop: StyleSetImpl::ResolvePseudoStyleFor(), this=%p\n", this));
  STYLESET_STOP_TIMER(NS_TIMER_STYLE_RESOLUTION);
  return result;
}

nsIStyleContext* StyleSetImpl::ProbePseudoStyleFor(nsIPresContext* aPresContext,
                                                   nsIContent* aParentContent,
                                                   nsIAtom* aPseudoTag,
                                                   nsIStyleContext* aParentContext)
{
  MOZ_TIMER_DEBUGLOG(("Start: StyleSetImpl::ProbePseudoStyleFor(), this=%p\n", this));
  STYLESET_START_TIMER(NS_TIMER_STYLE_RESOLUTION);

  nsIStyleContext*  result = nsnull;

  NS_ASSERTION(aPseudoTag, "must have pseudo tag");
  NS_ASSERTION(aPresContext, "must have pres context");
  NS_ASSERTION(!aParentContent ||
               aParentContent->IsContentOfType(nsIContent::eELEMENT),
               "content (if non-null) must be element");

  if (aPseudoTag && aPresContext) {
    GatherRuleProcessors();
    if (mAgentRuleProcessors ||
        mUserRuleProcessors  ||
        mDocRuleProcessors   ||
        mOverrideRuleProcessors) {
      nsCOMPtr<nsIAtom> medium;
      aPresContext->GetMedium(getter_AddRefs(medium));
      EnsureRuleWalker(aPresContext);
      PseudoRulesMatchingData data(aPresContext, medium, aParentContent, 
                                   aPseudoTag, nsnull, mRuleWalker);
      FileRules(EnumPseudoRulesMatching, &data);

      if (!mRuleWalker->AtRoot())
        result = GetContext(aPresContext, aParentContext, aPseudoTag);
 
      // Now reset the walker back to the root of the tree.
      mRuleWalker->Reset();
    }
  }
  
  MOZ_TIMER_DEBUGLOG(("Stop: StyleSetImpl::ProbePseudoStyleFor(), this=%p\n", this));
  STYLESET_STOP_TIMER(NS_TIMER_STYLE_RESOLUTION);
  return result;
}

PRBool PR_CALLBACK DeleteRuleNodeLists(nsHashKey* aKey, void* aData, void* aClosure)
{
  nsRuleNodeList* ruleNodeList = (nsRuleNodeList*)aData;
  ruleNodeList->Destroy();
  return PR_TRUE;
}

NS_IMETHODIMP
StyleSetImpl::Shutdown()
{
  mRuleMappings.Enumerate(DeleteRuleNodeLists);
  mRuleMappings.Reset();

  delete mRuleWalker;
  if (mRuleTree)
  {
    mRuleTree->Destroy();
    mRuleTree = nsnull;
  }
  return NS_OK;
}

nsresult
StyleSetImpl::GetRuleTree(nsRuleNode** aResult)
{
  *aResult = mRuleTree;
  return NS_OK;
}

nsresult 
StyleSetImpl::AddRuleNodeMapping(nsRuleNode* aRuleNode)
{
  nsVoidKey key(aRuleNode->Rule());
  nsRuleNodeList* ruleList = 
    new (aRuleNode->PresContext()) nsRuleNodeList(aRuleNode, 
                                                  NS_STATIC_CAST(nsRuleNodeList*, mRuleMappings.Get(&key)));
  mRuleMappings.Put(&key, ruleList);
  return NS_OK;
}

nsresult
StyleSetImpl::BeginRuleTreeReconstruct()
{
  delete mRuleWalker;
  mRuleWalker = nsnull;
  mOldRuleTree = mRuleTree;
  mRuleTree = nsnull;
  mRuleMappings.Enumerate(DeleteRuleNodeLists);
  mRuleMappings.Reset();
  return NS_OK;
}

nsresult
StyleSetImpl::EndRuleTreeReconstruct()
{
  if (mOldRuleTree) {
    mOldRuleTree->Destroy();
    mOldRuleTree = nsnull;
  }
  return NS_OK;
}

nsresult
StyleSetImpl::ClearCachedDataInRuleTree(nsIStyleRule* aInlineStyleRule)
{
  if (mRuleTree)
    mRuleTree->ClearCachedDataInSubtree(aInlineStyleRule);
  return NS_OK;
}

nsresult
StyleSetImpl::ClearStyleData(nsIPresContext* aPresContext, nsIStyleRule* aRule, nsIStyleContext* aContext)
{
  // XXXdwh.  If we're willing to *really* optimize this
  // invalidation, we could only invalidate the struct data
  // that actually changed.  For example, if someone changes
  // style.left, we really only need to blow away cached
  // data in the position struct.
  if (aContext) {
    // |aRule| should never be null, but we'll sanity check it just in case.
    if (aRule) {
      // Obtain our rule node list and clear out the cached data in all rule nodes
      // that correspond to this rule.  See bug 99344 for more details as to how
      // inline style rules can end up with multiple rule nodes in a rule tree.
      nsVoidKey key(aRule);
      nsRuleNodeList* ruleList = NS_STATIC_CAST(nsRuleNodeList*, mRuleMappings.Get(&key));
      for ( ; ruleList; ruleList = ruleList->mNext)
        ruleList->mRuleNode->ClearCachedData(aRule);
    }
    
    // XXXdwh I'm just being paranoid here.  Also clear out the data starting at the style
    // context's rule node.  This really should always be done in the for loop above,
    // but I'm going to leave this here just in case (for now).
    nsRuleNode* ruleNode;
    aContext->GetRuleNode(&ruleNode);
    ruleNode->ClearCachedData(aRule); 

    // XXX We need to clear style data here in case there's a style context
    // that inherits a struct from its parent where the parent uses data
    // that's cached on the rule node.  Otherwise we could crash while
    // doing checks comparing old data to new data during reresolution.
    // This could make some of those checks incorrect.
    aContext->ClearStyleData(aPresContext, nsnull);
  }
  else {
    // XXXdwh This is not terribly fast, but fortunately this case is rare (and often a full tree
    // invalidation anyway).  Improving performance here would involve a footprint
    // increase.  Mappings from rule nodes to their associated style contexts as well as
    // mappings from rules to their associated rule nodes would enable us to avoid the two
    // tree walks that occur here.

    // Crawl the entire rule tree and blow away all data for rule nodes (and their descendants)
    // that have the given rule.
    if (mRuleTree)
      mRuleTree->ClearCachedDataInSubtree(aRule);

    // We need to crawl the entire style context tree, and for each style context we need 
    // to see if the specified rule is matched.  If so, that context and all its descendant
    // contexts must have their data wiped.
    nsCOMPtr<nsIPresShell> shell;
    aPresContext->GetShell(getter_AddRefs(shell));
    nsIFrame* rootFrame;
    shell->GetRootFrame(&rootFrame);
    if (rootFrame) {
      nsCOMPtr<nsIStyleContext> rootContext;
      rootFrame->GetStyleContext(getter_AddRefs(rootContext));
      if (rootContext)
        rootContext->ClearStyleData(aPresContext, aRule);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
StyleSetImpl::ReParentStyleContext(nsIPresContext* aPresContext,
                                   nsIStyleContext* aStyleContext, 
                                   nsIStyleContext* aNewParentContext,
                                   nsIStyleContext** aNewStyleContext)
{
  NS_ASSERTION(aPresContext, "must have pres context");
  NS_ASSERTION(aStyleContext, "must have style context");
  NS_ASSERTION(aNewStyleContext, "must have new style context");

  nsresult result = NS_ERROR_NULL_POINTER;

  if (aPresContext && aStyleContext && aNewStyleContext) {
    nsCOMPtr<nsIStyleContext> oldParent = aStyleContext->GetParent();

    if (oldParent == aNewParentContext) {
      result = NS_OK;
      NS_ADDREF(aStyleContext);   // for return
      *aNewStyleContext = aStyleContext;
    }
    else {  // really a new parent
      nsIStyleContext*  newChild = nsnull;
      nsCOMPtr<nsIAtom>  pseudoTag;
      aStyleContext->GetPseudoType(*getter_AddRefs(pseudoTag));

      nsRuleNode* ruleNode;
      aStyleContext->GetRuleNode(&ruleNode);
      if (aNewParentContext) {
        result = aNewParentContext->FindChildWithRules(pseudoTag, ruleNode, newChild);
      }
      if (newChild) { // new parent already has one
        *aNewStyleContext = newChild;
      }
      else {  // need to make one in the new parent
        result = NS_NewStyleContext(aNewStyleContext, aNewParentContext, pseudoTag,
                                    ruleNode, aPresContext);
      }
    }
  }
  return result;
}

struct StatefulData : public StateRuleProcessorData {
  StatefulData(nsIPresContext* aPresContext, nsIAtom* aMedium,
               nsIContent* aContent, PRInt32 aStateMask)
    : StateRuleProcessorData(aPresContext, aContent, aStateMask),
      mMedium(aMedium),
      mStateful(PR_FALSE)
  {}
  nsIAtom*        mMedium;
  PRBool          mStateful;
}; 

static PRBool SheetHasStatefulStyle(nsISupports* aProcessor, void *aData)
{
  nsIStyleRuleProcessor* processor = (nsIStyleRuleProcessor*)aProcessor;
  StatefulData* data = (StatefulData*)aData;
  PRBool hasStateful;
  processor->HasStateDependentStyle(data, data->mMedium, &hasStateful);
  if (hasStateful) {
    data->mStateful = PR_TRUE;
    // Stop iteration.  Note that StyleSetImpl::WalkRuleProcessors uses
    // this to stop its own iteration in some cases, but not all (the
    // style rule supplier case).  Since this optimization is only for
    // the case where we have a lot more work to do, it's not worth the
    // code needed to make the stopping perfect.
    return PR_FALSE;
  }
  return PR_TRUE; // continue
}

// Test if style is dependent on content state
NS_IMETHODIMP
StyleSetImpl::HasStateDependentStyle(nsIPresContext* aPresContext,
                                     nsIContent*     aContent,
                                     PRInt32         aStateMask,
                                     PRBool*         aResult)
{
  GatherRuleProcessors();

  if (aContent->IsContentOfType(nsIContent::eELEMENT) &&
      (mAgentRuleProcessors ||
       mUserRuleProcessors  ||
       mDocRuleProcessors   ||
       mOverrideRuleProcessors)) {  
    nsIAtom* medium = nsnull;
    aPresContext->GetMedium(&medium);
    StatefulData data(aPresContext, medium, aContent, aStateMask);
    WalkRuleProcessors(SheetHasStatefulStyle, &data);
    NS_IF_RELEASE(medium);
    *aResult = data.mStateful;
  } else {
    *aResult = PR_FALSE;
  }

  return NS_OK;
}


NS_IMETHODIMP StyleSetImpl::ConstructRootFrame(nsIPresContext* aPresContext,
                                               nsIContent*     aDocElement,
                                               nsIFrame*&      aFrameSubTree)
{
  nsCOMPtr<nsIPresShell> shell;
  aPresContext->GetShell(getter_AddRefs(shell));
  return mFrameConstructor->ConstructRootFrame(shell, aPresContext, aDocElement,
                                               aFrameSubTree);
}

NS_IMETHODIMP   
StyleSetImpl::ReconstructDocElementHierarchy(nsIPresContext* aPresContext)
{
  return mFrameConstructor->ReconstructDocElementHierarchy(aPresContext);
}

NS_IMETHODIMP StyleSetImpl::ContentAppended(nsIPresContext* aPresContext,
                                            nsIContent*     aContainer,
                                            PRInt32         aNewIndexInContainer)
{
  return mFrameConstructor->ContentAppended(aPresContext, 
                                            aContainer, aNewIndexInContainer);
}

NS_IMETHODIMP StyleSetImpl::ContentInserted(nsIPresContext* aPresContext,
                                            nsIContent*     aContainer,
                                            nsIContent*     aChild,
                                            PRInt32         aIndexInContainer)
{
  return mFrameConstructor->ContentInserted(aPresContext, aContainer,
                                            aChild, aIndexInContainer, 
                                            nsnull, PR_FALSE);
}

NS_IMETHODIMP StyleSetImpl::ContentReplaced(nsIPresContext* aPresContext,
                                            nsIContent*     aContainer,
                                            nsIContent*     aOldChild,
                                            nsIContent*     aNewChild,
                                            PRInt32         aIndexInContainer)
{
  return mFrameConstructor->ContentReplaced(aPresContext, aContainer,
                                            aOldChild, aNewChild, aIndexInContainer);
}

NS_IMETHODIMP StyleSetImpl::ContentRemoved(nsIPresContext* aPresContext,
                                           nsIContent*     aContainer,
                                           nsIContent*     aChild,
                                           PRInt32         aIndexInContainer)
{
  return mFrameConstructor->ContentRemoved(aPresContext, aContainer,
                                           aChild, aIndexInContainer, PR_FALSE);
}

NS_IMETHODIMP
StyleSetImpl::ContentChanged(nsIPresContext* aPresContext,
                             nsIContent* aContent,
                             nsISupports* aSubContent)
{
  return mFrameConstructor->ContentChanged(aPresContext, 
                                           aContent, aSubContent);
}

NS_IMETHODIMP
StyleSetImpl::ContentStatesChanged(nsIPresContext* aPresContext, 
                                   nsIContent* aContent1,
                                   nsIContent* aContent2,
                                   PRInt32 aStateMask)
{
  return mFrameConstructor->ContentStatesChanged(aPresContext, aContent1,
                                                 aContent2, aStateMask);
}


NS_IMETHODIMP
StyleSetImpl::AttributeChanged(nsIPresContext* aPresContext,
                               nsIContent* aContent,
                               PRInt32 aNameSpaceID,
                               nsIAtom* aAttribute,
                               PRInt32 aModType, 
                               nsChangeHint aHint)
{
  return mFrameConstructor->AttributeChanged(aPresContext, aContent, 
                                             aNameSpaceID, aAttribute, aModType, aHint);
}


// Style change notifications
NS_IMETHODIMP
StyleSetImpl::StyleRuleChanged(nsIPresContext* aPresContext,
                               nsIStyleSheet* aStyleSheet,
                               nsIStyleRule* aStyleRule,
                               nsChangeHint aHint)
{
  return mFrameConstructor->StyleRuleChanged(aPresContext, aStyleSheet, aStyleRule, aHint);
}

NS_IMETHODIMP
StyleSetImpl::StyleRuleAdded(nsIPresContext* aPresContext,
                             nsIStyleSheet* aStyleSheet,
                             nsIStyleRule* aStyleRule)
{
  return mFrameConstructor->StyleRuleAdded(aPresContext, aStyleSheet, aStyleRule);
}

NS_IMETHODIMP
StyleSetImpl::StyleRuleRemoved(nsIPresContext* aPresContext,
                               nsIStyleSheet* aStyleSheet,
                               nsIStyleRule* aStyleRule)
{
  return mFrameConstructor->StyleRuleRemoved(aPresContext, aStyleSheet, aStyleRule);
}

NS_IMETHODIMP
StyleSetImpl::CantRenderReplacedElement(nsIPresContext* aPresContext,
                                        nsIFrame*       aFrame)
{
  nsCOMPtr<nsIPresShell> shell;
  aPresContext->GetShell(getter_AddRefs(shell));
  return mFrameConstructor->CantRenderReplacedElement(shell, aPresContext, aFrame);
}

NS_IMETHODIMP
StyleSetImpl::CreateContinuingFrame(nsIPresContext* aPresContext,
                                    nsIFrame*       aFrame,
                                    nsIFrame*       aParentFrame,
                                    nsIFrame**      aContinuingFrame)
{
  nsCOMPtr<nsIPresShell> shell;
  aPresContext->GetShell(getter_AddRefs(shell));
  return mFrameConstructor->CreateContinuingFrame(shell, aPresContext, aFrame, aParentFrame,
                                                  aContinuingFrame);
}

// Request to find the primary frame associated with a given content object.
// This is typically called by the pres shell when there is no mapping in
// the pres shell hash table
NS_IMETHODIMP
StyleSetImpl::FindPrimaryFrameFor(nsIPresContext*  aPresContext,
                                  nsIFrameManager* aFrameManager,
                                  nsIContent*      aContent,
                                  nsIFrame**       aFrame,
                                  nsFindFrameHint* aHint)
{
  return mFrameConstructor->FindPrimaryFrameFor(aPresContext, aFrameManager,
                                                aContent, aFrame, aHint);
}

NS_IMETHODIMP
StyleSetImpl::GetInsertionPoint(nsIPresShell* aPresShell,
                                nsIFrame*     aParentFrame,
                                nsIContent*   aChildContent,
                                nsIFrame**    aInsertionPoint)
{
  return mFrameConstructor->GetInsertionPoint(aPresShell, aParentFrame,
                                              aChildContent, aInsertionPoint);
}

#ifdef DEBUG
void StyleSetImpl::List(FILE* out, PRInt32 aIndent, nsISupportsArray* aSheets)
{
  PRUint32 cnt = 0;
  if (aSheets) {
    nsresult rv = aSheets->Count(&cnt);
    if (NS_FAILED(rv)) return;    // XXX error?
  }

  for (PRInt32 index = 0; index < (PRInt32)cnt; index++) {
    nsIStyleSheet* sheet = (nsIStyleSheet*)aSheets->ElementAt(index);
    sheet->List(out, aIndent);
    fputs("\n", out);
    NS_RELEASE(sheet);
  }
}
#endif

// APIs for registering objects that can supply additional
// rules during processing.
NS_IMETHODIMP
StyleSetImpl::SetStyleRuleSupplier(nsIStyleRuleSupplier* aSupplier)
{
  mStyleRuleSupplier = aSupplier;
  return NS_OK;
}

NS_IMETHODIMP
StyleSetImpl::GetStyleRuleSupplier(nsIStyleRuleSupplier** aSupplier)
{
  *aSupplier = mStyleRuleSupplier;
  NS_IF_ADDREF(*aSupplier);
  return NS_OK;
}


#ifdef DEBUG
void StyleSetImpl::List(FILE* out, PRInt32 aIndent)
{
//  List(out, aIndent, mOverrideSheets);
  List(out, aIndent, mDocSheets);
//  List(out, aIndent, mUserSheets);
//  List(out, aIndent, mAgentSheets);
}


void StyleSetImpl::ListContexts(nsIStyleContext* aRootContext, FILE* out, PRInt32 aIndent)
{
  aRootContext->List(out, aIndent);
}
#endif


NS_EXPORT nsresult
NS_NewStyleSet(nsIStyleSet** aInstancePtrResult)
{
  if (aInstancePtrResult == nsnull) {
    return NS_ERROR_NULL_POINTER;
  }

  StyleSetImpl  *it = new StyleSetImpl();

  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return it->QueryInterface(NS_GET_IID(nsIStyleSet), (void **) aInstancePtrResult);
}

// nsITimeRecorder implementation

#ifdef MOZ_PERF_METRICS

NS_IMETHODIMP
StyleSetImpl::EnableTimer(PRUint32 aTimerID)
{
  nsresult rv = NS_OK;

  if (NS_TIMER_STYLE_RESOLUTION == aTimerID) {
    mTimerEnabled = PR_TRUE;
  }
  else 
    rv = NS_ERROR_NOT_IMPLEMENTED;
  
  return rv;
}

NS_IMETHODIMP
StyleSetImpl::DisableTimer(PRUint32 aTimerID)
{
  nsresult rv = NS_OK;

  if (NS_TIMER_STYLE_RESOLUTION == aTimerID) {
    mTimerEnabled = PR_FALSE;
  }
  else 
    rv = NS_ERROR_NOT_IMPLEMENTED;
  
  return rv;
}

NS_IMETHODIMP
StyleSetImpl::IsTimerEnabled(PRBool *aIsEnabled, PRUint32 aTimerID)
{
  NS_ASSERTION(aIsEnabled != nsnull, "aIsEnabled paramter cannot be null" );
  nsresult rv = NS_OK;

  if (NS_TIMER_STYLE_RESOLUTION == aTimerID) {
	  if (*aIsEnabled != nsnull) {
      *aIsEnabled = mTimerEnabled;		
	  }
  }
  else 
    rv = NS_ERROR_NOT_IMPLEMENTED;
  
  return rv;
}

NS_IMETHODIMP
StyleSetImpl::ResetTimer(PRUint32 aTimerID)
{
  nsresult rv = NS_OK;

  if (NS_TIMER_STYLE_RESOLUTION == aTimerID) {
    MOZ_TIMER_RESET(mStyleResolutionWatch);
  }
  else 
    rv = NS_ERROR_NOT_IMPLEMENTED;
  
  return rv;
}

NS_IMETHODIMP
StyleSetImpl::StartTimer(PRUint32 aTimerID)
{
  nsresult rv = NS_OK;

  if (NS_TIMER_STYLE_RESOLUTION == aTimerID) {
    // only do it if enabled
    if (mTimerEnabled) {
      MOZ_TIMER_START(mStyleResolutionWatch);
    } else {
#ifdef NOISY_DEBUG
      printf( "Attempt to start timer while disabled - ignoring\n" );
#endif
    }
  }
  else 
    rv = NS_ERROR_NOT_IMPLEMENTED;
  
  return rv;
}

NS_IMETHODIMP
StyleSetImpl::StopTimer(PRUint32 aTimerID)
{
  nsresult rv = NS_OK;

  if (NS_TIMER_STYLE_RESOLUTION == aTimerID) {
    // only do it if enabled
    if (mTimerEnabled) {
      MOZ_TIMER_STOP(mStyleResolutionWatch);
    } else {
#ifdef NOISY_DEBUG
      printf( "Attempt to stop timer while disabled - ignoring\n" );
#endif
    }
  }
  else 
    rv = NS_ERROR_NOT_IMPLEMENTED;
  
  return rv;
}

NS_IMETHODIMP
StyleSetImpl::PrintTimer(PRUint32 aTimerID)
{
  nsresult rv = NS_OK;

  if (NS_TIMER_STYLE_RESOLUTION == aTimerID) {
    MOZ_TIMER_PRINT(mStyleResolutionWatch);
  }
  else 
    rv = NS_ERROR_NOT_IMPLEMENTED;
  
  return rv;
}

#endif

//-----------------------------------------------------------------------------

// static 
nsUniqueStyleItems *nsUniqueStyleItems ::mInstance = nsnull;

void StyleSetImpl::ResetUniqueStyleItems(void)
{
  UNIQUE_STYLE_ITEMS(uniqueItems);
  uniqueItems->Clear();  
}

struct AttributeContentPair {
  nsIAtom *attribute;
  nsIContent *content;
};

static PRBool
EnumAffectsStyle(nsISupports *aElement, void *aData)
{
  nsIStyleSheet *sheet = NS_STATIC_CAST(nsIStyleSheet *, aElement);
  AttributeContentPair *pair = (AttributeContentPair *)aData;
  PRBool affects;
  nsresult res =
      sheet->AttributeAffectsStyle(pair->attribute, pair->content, affects);
  if (NS_FAILED(res) || affects)
    return PR_FALSE;            // stop checking

  return PR_TRUE;
}

NS_IMETHODIMP
StyleSetImpl::AttributeAffectsStyle(nsIAtom *aAttribute, nsIContent *aContent,
                                    PRBool &aAffects)
{
  AttributeContentPair pair;
  pair.attribute = aAttribute;
  pair.content = aContent;

  /* scoped sheets should be checked first, since - if present - they will contain
     the bulk of the applicable rules for the content node. */
  if (mStyleRuleSupplier)
    mStyleRuleSupplier->AttributeAffectsStyle(EnumAffectsStyle, &pair, aContent, &aAffects);

  if (!aAffects) {
    /* check until we find a sheet that will be affected */
    if ((mDocSheets && !mDocSheets->EnumerateForwards(EnumAffectsStyle, &pair)) ||
        (mOverrideSheets && !mOverrideSheets->EnumerateForwards(EnumAffectsStyle,
                                                                &pair)) ||
        (mUserSheets && !mUserSheets->EnumerateForwards(EnumAffectsStyle,
                                                                &pair)) ||
        (mAgentSheets && !mAgentSheets->EnumerateForwards(EnumAffectsStyle,
                                                                &pair))) {
      aAffects = PR_TRUE;
    } else {
      aAffects = PR_FALSE;
    }
  }

  return NS_OK;
}

#ifdef DEBUG
/******************************************************************************
* SizeOf method:
*
*  Self (reported as StyleSetImpl's size): 
*    1) sizeof(*this) + sizeof (overhead only) each collection that exists
*       and the FrameConstructor overhead
*
*  Contained / Aggregated data (not reported as StyleSetImpl's size):
*    1) Override Sheets, DocSheets, UserSheets, AgentSheets, RuleProcessors, Recycler
*       are all delegated to.
*
*  Children / siblings / parents:
*    none
*    
******************************************************************************/
void StyleSetImpl::SizeOf(nsISizeOfHandler *aSizeOfHandler, PRUint32 &aSize)
{
  NS_ASSERTION(aSizeOfHandler != nsnull, "SizeOf handler cannot be null");

  // first get the unique items collection
  UNIQUE_STYLE_ITEMS(uniqueItems);

  if(! uniqueItems->AddItem((void*)this) ){
    NS_ASSERTION(0, "StyleSet has already been conted in SizeOf operation");
    // styleset has already been accounted for
    return;
  }
  // get or create a tag for this instance
  nsCOMPtr<nsIAtom> tag;
  tag = getter_AddRefs(NS_NewAtom("StyleSet"));
  // get the size of an empty instance and add to the sizeof handler
  aSize = sizeof(StyleSetImpl);

  // Next get the size of the OVERHEAD of objects we will delegate to:
  if (mOverrideSheets && uniqueItems->AddItem(mOverrideSheets)){
    aSize += sizeof(*mOverrideSheets);
  }
  if (mDocSheets && uniqueItems->AddItem(mDocSheets)){
    aSize += sizeof(*mDocSheets);
  }
  if (mUserSheets && uniqueItems->AddItem(mUserSheets)){
    aSize += sizeof(*mUserSheets);
  }
  if (mAgentSheets && uniqueItems->AddItem(mAgentSheets)){
    aSize += sizeof(*mAgentSheets);
  }
  if (mAgentRuleProcessors && uniqueItems->AddItem(mAgentRuleProcessors)){
    aSize += sizeof(*mAgentRuleProcessors);
  }
  if (mUserRuleProcessors && uniqueItems->AddItem(mUserRuleProcessors)){
    aSize += sizeof(*mUserRuleProcessors);
  }
  if (mDocRuleProcessors && uniqueItems->AddItem(mDocRuleProcessors)){
    aSize += sizeof(*mDocRuleProcessors);
  }
  if (mOverrideRuleProcessors && uniqueItems->AddItem(mOverrideRuleProcessors)){
    aSize += sizeof(*mOverrideRuleProcessors);
  }
  if (mRecycler && uniqueItems->AddItem(mRecycler)){
    aSize += sizeof(*mRecycler);
  }
  if (mQuirkStyleSheet) {
    aSize += sizeof(mQuirkStyleSheet);  // just the pointer: the sheet is counted elsewhere
  }
  ///////////////////////////////////////////////
  // now the FrameConstructor
  if(mFrameConstructor && uniqueItems->AddItem((void*)mFrameConstructor)){
    aSize += sizeof(mFrameConstructor);
  }
  aSizeOfHandler->AddSize(tag,aSize);

  ///////////////////////////////////////////////
  // Now travers the collections and delegate
  PRInt32 numSheets, curSheet;
  PRUint32 localSize=0;
  numSheets = GetNumberOfOverrideStyleSheets();
  for(curSheet=0; curSheet < numSheets; curSheet++){
    nsIStyleSheet* pSheet = GetOverrideStyleSheetAt(curSheet); //addref
    if(pSheet){
      localSize=0;
      pSheet->SizeOf(aSizeOfHandler, localSize);
    }
    NS_IF_RELEASE(pSheet);
  }

  numSheets = GetNumberOfDocStyleSheets();
  for(curSheet=0; curSheet < numSheets; curSheet++){
    nsIStyleSheet* pSheet = GetDocStyleSheetAt(curSheet);
    if(pSheet){
      localSize=0;
      pSheet->SizeOf(aSizeOfHandler, localSize);
    }
    NS_IF_RELEASE(pSheet);
  }

  numSheets = GetNumberOfUserStyleSheets();
  for(curSheet=0; curSheet < numSheets; curSheet++){
    nsIStyleSheet* pSheet = GetUserStyleSheetAt(curSheet);
    if(pSheet){
      localSize=0;
      pSheet->SizeOf(aSizeOfHandler, localSize);
    }
    NS_IF_RELEASE(pSheet);
  }

  numSheets = GetNumberOfAgentStyleSheets();
  for(curSheet=0; curSheet < numSheets; curSheet++){
    nsIStyleSheet* pSheet = GetAgentStyleSheetAt(curSheet);
    if(pSheet){
      localSize=0;
      pSheet->SizeOf(aSizeOfHandler, localSize);
    }
    NS_IF_RELEASE(pSheet);
  }
  ///////////////////////////////////////////////
  // rule processors
  PRUint32 numRuleProcessors,curRuleProcessor;
  if(mAgentRuleProcessors){
    mAgentRuleProcessors->Count(&numRuleProcessors);
    for(curRuleProcessor=0; curRuleProcessor < numRuleProcessors; curRuleProcessor++){
      nsIStyleRuleProcessor* processor = 
        (nsIStyleRuleProcessor* )mAgentRuleProcessors->ElementAt(curRuleProcessor);
      if(processor){
        localSize=0;
        processor->SizeOf(aSizeOfHandler, localSize);
      }
      NS_IF_RELEASE(processor);
    }
  }
  if(mUserRuleProcessors){
    mUserRuleProcessors->Count(&numRuleProcessors);
    for(curRuleProcessor=0; curRuleProcessor < numRuleProcessors; curRuleProcessor++){
      nsIStyleRuleProcessor* processor = 
        (nsIStyleRuleProcessor* )mUserRuleProcessors->ElementAt(curRuleProcessor);
      if(processor){
        localSize=0;
        processor->SizeOf(aSizeOfHandler, localSize);
      }
      NS_IF_RELEASE(processor);
    }
  }
  if(mDocRuleProcessors){
    mDocRuleProcessors->Count(&numRuleProcessors);
    for(curRuleProcessor=0; curRuleProcessor < numRuleProcessors; curRuleProcessor++){
      nsIStyleRuleProcessor* processor = 
        (nsIStyleRuleProcessor* )mDocRuleProcessors->ElementAt(curRuleProcessor);
      if(processor){
        localSize=0;
        processor->SizeOf(aSizeOfHandler, localSize);
      }
      NS_IF_RELEASE(processor);
    }
  }
  if(mOverrideRuleProcessors){
    mOverrideRuleProcessors->Count(&numRuleProcessors);
    for(curRuleProcessor=0; curRuleProcessor < numRuleProcessors; curRuleProcessor++){
      nsIStyleRuleProcessor* processor = 
        (nsIStyleRuleProcessor* )mOverrideRuleProcessors->ElementAt(curRuleProcessor);
      if(processor){
        localSize=0;
        processor->SizeOf(aSizeOfHandler, localSize);
      }
      NS_IF_RELEASE(processor);
    }
  }
  
  ///////////////////////////////////////////////
  // and the recycled ones too
  if(mRecycler){
    mRecycler->Count(&numRuleProcessors);
    for(curRuleProcessor=0; curRuleProcessor < numRuleProcessors; curRuleProcessor++){
      nsIStyleRuleProcessor* processor = 
        (nsIStyleRuleProcessor* )mRecycler->ElementAt(curRuleProcessor);
      if(processor && uniqueItems->AddItem((void*)processor)){
        localSize=0;
        processor->SizeOf(aSizeOfHandler, localSize);
      }
      NS_IF_RELEASE(processor);
    }
  }

  // XXX - do the stylecontext cache too

  // now delegate the sizeof to the larger or more complex aggregated objects
  // - none
}
#endif
