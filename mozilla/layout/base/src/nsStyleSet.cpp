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
#include "nsIStyleSet.h"
#include "nsIStyleSheet.h"
#include "nsIStyleRule.h"
#include "nsIStyleContext.h"
#include "nsISupportsArray.h"
#include "nsIFrame.h"
//#include "nsHashtable.h"
#include "nsIPresContext.h"
#include "nsIPresShell.h"
#include "nsIContent.h"
#include "nsIStyleFrameConstruction.h"

// XXX Temporary fix to make sure that ua.css only gets applied
// to HTML content. When this removed, remember to get rid of
// the include dependency in the makefile.
#include "nsIHTMLContent.h"
static NS_DEFINE_IID(kIHTMLContentIID, NS_IHTMLCONTENT_IID);

static NS_DEFINE_IID(kIStyleSetIID, NS_ISTYLE_SET_IID);
static NS_DEFINE_IID(kIStyleFrameConstructionIID, NS_ISTYLE_FRAME_CONSTRUCTION_IID);


class StyleSetImpl : public nsIStyleSet {
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

  virtual void AppendDocStyleSheet(nsIStyleSheet* aSheet);
  virtual void InsertDocStyleSheetAfter(nsIStyleSheet* aSheet,
                                        nsIStyleSheet* aAfterSheet);
  virtual void InsertDocStyleSheetBefore(nsIStyleSheet* aSheet,
                                         nsIStyleSheet* aBeforeSheet);
  virtual void RemoveDocStyleSheet(nsIStyleSheet* aSheet);
  virtual PRInt32 GetNumberOfDocStyleSheets();
  virtual nsIStyleSheet* GetDocStyleSheetAt(PRInt32 aIndex);

  virtual void AppendBackstopStyleSheet(nsIStyleSheet* aSheet);
  virtual void InsertBackstopStyleSheetAfter(nsIStyleSheet* aSheet,
                                             nsIStyleSheet* aAfterSheet);
  virtual void InsertBackstopStyleSheetBefore(nsIStyleSheet* aSheet,
                                              nsIStyleSheet* aBeforeSheet);
  virtual void RemoveBackstopStyleSheet(nsIStyleSheet* aSheet);
  virtual PRInt32 GetNumberOfBackstopStyleSheets();
  virtual nsIStyleSheet* GetBackstopStyleSheetAt(PRInt32 aIndex);

  virtual nsIStyleContext* ResolveStyleFor(nsIPresContext* aPresContext,
                                           nsIContent* aContent,
                                           nsIStyleContext* aParentContext,
                                           PRBool aForceUnique = PR_FALSE);

  virtual nsIStyleContext* ResolvePseudoStyleFor(nsIPresContext* aPresContext,
                                                 nsIContent* aParentContent,
                                                 nsIAtom* aPseudoTag,
                                                 nsIStyleContext* aParentContext,
                                                 PRBool aForceUnique = PR_FALSE);

  virtual nsIStyleContext* ProbePseudoStyleFor(nsIPresContext* aPresContext,
                                               nsIContent* aParentContent,
                                               nsIAtom* aPseudoTag,
                                               nsIStyleContext* aParentContext,
                                               PRBool aForceUnique = PR_FALSE);

  NS_IMETHODIMP ConstructFrame(nsIPresContext* aPresContext,
                               nsIContent*     aContent,
                               nsIFrame*       aParentFrame,
                               nsIFrame*&      aFrameSubTree);
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
  NS_IMETHOD AttributeChanged(nsIPresContext*  aPresContext,
                              nsIContent* aChild,
                              nsIAtom* aAttribute,
                              PRInt32 aHint); // See nsStyleConsts fot hint values

  // xxx style rules enumeration

  virtual void List(FILE* out = stdout, PRInt32 aIndent = 0);

private:
  // These are not supported and are not implemented!
  StyleSetImpl(const StyleSetImpl& aCopy);
  StyleSetImpl& operator=(const StyleSetImpl& aCopy);

protected:
  virtual ~StyleSetImpl();
  PRBool EnsureArray(nsISupportsArray** aArray);
  nsIStyleContext* GetContext(nsIPresContext* aPresContext, nsIStyleContext* aParentContext, 
                              nsIAtom* aPseudoTag, nsISupportsArray* aRules, PRBool aForceUnique, 
                              PRBool& aUsedRules);
  PRInt32 RulesMatching(nsISupportsArray* aSheets,
                        nsIPresContext* aPresContext,
                        nsIContent* aContent,
                        nsIStyleContext* aParentContext,
                        nsISupportsArray* aResults);
  PRInt32 RulesMatching(nsISupportsArray* aSheets,
                        nsIPresContext* aPresContext,
                        nsIContent* aParentContent,
                        nsIAtom* aPseudoTag,
                        nsIStyleContext* aParentContext,
                        nsISupportsArray* aResults);
  void  List(FILE* out, PRInt32 aIndent, nsISupportsArray* aSheets);
  void  ListContexts(nsIStyleContext* aRootContext, FILE* out, PRInt32 aIndent);

  nsISupportsArray* mOverrideSheets;
  nsISupportsArray* mDocSheets;
  nsISupportsArray* mBackstopSheets;
  nsISupportsArray* mRecycler;

  nsIStyleFrameConstruction* mFrameConstructor;
};


StyleSetImpl::StyleSetImpl()
  : mOverrideSheets(nsnull),
    mDocSheets(nsnull),
    mBackstopSheets(nsnull),
    mFrameConstructor(nsnull),
    mRecycler(nsnull)
{
  NS_INIT_REFCNT();
}

StyleSetImpl::~StyleSetImpl()
{
  NS_IF_RELEASE(mOverrideSheets);
  NS_IF_RELEASE(mDocSheets);
  NS_IF_RELEASE(mBackstopSheets);
  NS_IF_RELEASE(mFrameConstructor);
  NS_IF_RELEASE(mRecycler);
}

NS_IMPL_ISUPPORTS(StyleSetImpl, kIStyleSetIID)

PRBool StyleSetImpl::EnsureArray(nsISupportsArray** aArray)
{
  if (nsnull == *aArray) {
    if (NS_OK != NS_NewISupportsArray(aArray)) {
      return PR_FALSE;
    }
  }
  return PR_TRUE;
}

// ----- Override sheets

void StyleSetImpl::AppendOverrideStyleSheet(nsIStyleSheet* aSheet)
{
  NS_PRECONDITION(nsnull != aSheet, "null arg");
  if (EnsureArray(&mOverrideSheets)) {
    mOverrideSheets->AppendElement(aSheet);
  }
}

void StyleSetImpl::InsertOverrideStyleSheetAfter(nsIStyleSheet* aSheet,
                                                 nsIStyleSheet* aAfterSheet)
{
  NS_PRECONDITION(nsnull != aSheet, "null arg");
  if (EnsureArray(&mOverrideSheets)) {
    PRInt32 index = mOverrideSheets->IndexOf(aAfterSheet);
    mOverrideSheets->InsertElementAt(aSheet, ++index);
  }
}

void StyleSetImpl::InsertOverrideStyleSheetBefore(nsIStyleSheet* aSheet,
                                                  nsIStyleSheet* aBeforeSheet)
{
  NS_PRECONDITION(nsnull != aSheet, "null arg");
  if (EnsureArray(&mOverrideSheets)) {
    PRInt32 index = mOverrideSheets->IndexOf(aBeforeSheet);
    mOverrideSheets->InsertElementAt(aSheet, ((-1 < index) ? index : 0));
  }
}

void StyleSetImpl::RemoveOverrideStyleSheet(nsIStyleSheet* aSheet)
{
  NS_PRECONDITION(nsnull != aSheet, "null arg");

  if (nsnull != mOverrideSheets) {
    mOverrideSheets->RemoveElement(aSheet);
  }
}

PRInt32 StyleSetImpl::GetNumberOfOverrideStyleSheets()
{
  if (nsnull != mOverrideSheets) {
    return mOverrideSheets->Count();
  }
  return 0;
}

nsIStyleSheet* StyleSetImpl::GetOverrideStyleSheetAt(PRInt32 aIndex)
{
  nsIStyleSheet* sheet = nsnull;
  if (nsnull == mOverrideSheets) {
    sheet = (nsIStyleSheet*)mOverrideSheets->ElementAt(aIndex);
  }
  return sheet;
}

// -------- Doc Sheets

void StyleSetImpl::AppendDocStyleSheet(nsIStyleSheet* aSheet)
{
  NS_PRECONDITION(nsnull != aSheet, "null arg");
  if (EnsureArray(&mDocSheets)) {
    mDocSheets->AppendElement(aSheet);
    if (nsnull == mFrameConstructor) {
      aSheet->QueryInterface(kIStyleFrameConstructionIID, (void **)&mFrameConstructor);
    }
  }
}

void StyleSetImpl::InsertDocStyleSheetAfter(nsIStyleSheet* aSheet,
                                         nsIStyleSheet* aAfterSheet)
{
  NS_PRECONDITION(nsnull != aSheet, "null arg");
  if (EnsureArray(&mDocSheets)) {
    PRInt32 index = mDocSheets->IndexOf(aAfterSheet);
    mDocSheets->InsertElementAt(aSheet, ++index);
    if (nsnull == mFrameConstructor) {
      aSheet->QueryInterface(kIStyleFrameConstructionIID, (void **)&mFrameConstructor);
    }
  }
}

void StyleSetImpl::InsertDocStyleSheetBefore(nsIStyleSheet* aSheet,
                                          nsIStyleSheet* aBeforeSheet)
{
  NS_PRECONDITION(nsnull != aSheet, "null arg");
  if (EnsureArray(&mDocSheets)) {
    PRInt32 index = mDocSheets->IndexOf(aBeforeSheet);
    mDocSheets->InsertElementAt(aSheet, ((-1 < index) ? index : 0));
    if (nsnull == mFrameConstructor) {
      aSheet->QueryInterface(kIStyleFrameConstructionIID, (void **)&mFrameConstructor);
    }
  }
}

void StyleSetImpl::RemoveDocStyleSheet(nsIStyleSheet* aSheet)
{
  NS_PRECONDITION(nsnull != aSheet, "null arg");

  if (nsnull != mDocSheets) {
    mDocSheets->RemoveElement(aSheet);
  }
}

PRInt32 StyleSetImpl::GetNumberOfDocStyleSheets()
{
  if (nsnull != mDocSheets) {
    return mDocSheets->Count();
  }
  return 0;
}

nsIStyleSheet* StyleSetImpl::GetDocStyleSheetAt(PRInt32 aIndex)
{
  nsIStyleSheet* sheet = nsnull;
  if (nsnull == mDocSheets) {
    sheet = (nsIStyleSheet*)mDocSheets->ElementAt(aIndex);
  }
  return sheet;
}

// ------ backstop sheets

void StyleSetImpl::AppendBackstopStyleSheet(nsIStyleSheet* aSheet)
{
  NS_PRECONDITION(nsnull != aSheet, "null arg");
  if (EnsureArray(&mBackstopSheets)) {
    mBackstopSheets->AppendElement(aSheet);
  }
}

void StyleSetImpl::InsertBackstopStyleSheetAfter(nsIStyleSheet* aSheet,
                                                 nsIStyleSheet* aAfterSheet)
{
  NS_PRECONDITION(nsnull != aSheet, "null arg");
  if (EnsureArray(&mBackstopSheets)) {
    PRInt32 index = mBackstopSheets->IndexOf(aAfterSheet);
    mBackstopSheets->InsertElementAt(aSheet, ++index);
  }
}

void StyleSetImpl::InsertBackstopStyleSheetBefore(nsIStyleSheet* aSheet,
                                                  nsIStyleSheet* aBeforeSheet)
{
  NS_PRECONDITION(nsnull != aSheet, "null arg");
  if (EnsureArray(&mBackstopSheets)) {
    PRInt32 index = mBackstopSheets->IndexOf(aBeforeSheet);
    mBackstopSheets->InsertElementAt(aSheet, ((-1 < index) ? index : 0));
  }
}

void StyleSetImpl::RemoveBackstopStyleSheet(nsIStyleSheet* aSheet)
{
  NS_PRECONDITION(nsnull != aSheet, "null arg");

  if (nsnull != mBackstopSheets) {
    mBackstopSheets->RemoveElement(aSheet);
  }
}

PRInt32 StyleSetImpl::GetNumberOfBackstopStyleSheets()
{
  if (nsnull != mBackstopSheets) {
    return mBackstopSheets->Count();
  }
  return 0;
}

nsIStyleSheet* StyleSetImpl::GetBackstopStyleSheetAt(PRInt32 aIndex)
{
  nsIStyleSheet* sheet = nsnull;
  if (nsnull == mBackstopSheets) {
    sheet = (nsIStyleSheet*)mBackstopSheets->ElementAt(aIndex);
  }
  return sheet;
}

PRInt32 StyleSetImpl::RulesMatching(nsISupportsArray* aSheets,
                                    nsIPresContext* aPresContext,
                                    nsIContent* aContent,
                                    nsIStyleContext* aParentContext,
                                    nsISupportsArray* aResults)
{
  PRInt32 ruleCount = 0;

  if (nsnull != aSheets) {
    PRInt32 index = aSheets->Count();
    while (0 < index--) {
      nsIStyleSheet* sheet = (nsIStyleSheet*)aSheets->ElementAt(index);
      ruleCount += sheet->RulesMatching(aPresContext, aContent, aParentContext,
                                        aResults);
      NS_RELEASE(sheet);
    }
  }
  return ruleCount;
}

nsIStyleContext* StyleSetImpl::GetContext(nsIPresContext* aPresContext, 
                                          nsIStyleContext* aParentContext, nsIAtom* aPseudoTag, 
                                          nsISupportsArray* aRules,
                                          PRBool aForceUnique, PRBool& aUsedRules)
{
  nsIStyleContext* result = nsnull;

  aUsedRules = PR_FALSE;
  if ((PR_FALSE == aForceUnique) && 
      (nsnull != aParentContext) && (nsnull == aRules) && 
      (0 == aParentContext->GetStyleRuleCount())) {
    nsIAtom*  parentTag = nsnull;
    aParentContext->GetPseudoType(parentTag);
    if (parentTag == aPseudoTag) {
      // this and parent are empty, and compatible
      result = aParentContext;
      NS_ADDREF(result);  // add ref for the caller
    }
    NS_IF_RELEASE(parentTag);
//fprintf(stdout, ".");
  }
  if (nsnull == result) {
    if ((PR_FALSE == aForceUnique) && (nsnull != aParentContext)) {
      aParentContext->FindChildWithRules(aPseudoTag, aRules, result);
    }
    if (nsnull == result) {
      if (NS_OK == NS_NewStyleContext(&result, aParentContext, aPseudoTag, aRules, aPresContext)) {
        if (PR_TRUE == aForceUnique) {
          result->ForceUnique();
        }
        aUsedRules = PRBool(nsnull != aRules);
      }
//fprintf(stdout, "+");
    }
    else {
//fprintf(stdout, "-");
    }
  }
  return result;
}

// XXX for now only works for strength 0 & 1
static void SortRulesByStrength(nsISupportsArray* aRules, PRInt32& aBackstopRuleCount)
{
  PRInt32 count = aRules->Count();

  if (1 < count) {
    PRInt32 index;
    PRInt32 strength;
    for (index = 0; index < count; ) {
      nsIStyleRule* rule = (nsIStyleRule*)aRules->ElementAt(index);
      rule->GetStrength(strength);
      if (0 < strength) {
        aRules->RemoveElementAt(index);
        aRules->AppendElement(rule);
        count--;
        if (index < aBackstopRuleCount) {
          aBackstopRuleCount--;
        }
      }
      else {
        index++;
      }
      NS_RELEASE(rule);
    }
  }
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

nsIStyleContext* StyleSetImpl::ResolveStyleFor(nsIPresContext* aPresContext,
                                               nsIContent* aContent,
                                               nsIStyleContext* aParentContext,
                                               PRBool aForceUnique)
{
  nsIStyleContext*  result = nsnull;

  // want to check parent frame's context for cached child context first
  if ((nsnull != aParentContext) && (nsnull != aContent)) {
//XXX Disabled this for the dom, as per peter's note
//XXX    result = aParentContext->FindChildWithContent(aContent);
  }

  if (nsnull == result) {
    // then do a brute force rule search

    nsISupportsArray*  rules = mRecycler;
    mRecycler = nsnull;
    if (nsnull == rules) {
      NS_NewISupportsArray(&rules);
    }

    if (nsnull != rules) {
      nsIHTMLContent *htmlContent;
      nsresult rv = aContent->QueryInterface(kIHTMLContentIID, (void **)&htmlContent);
      PRInt32 ruleCount = 0;
      if (NS_SUCCEEDED(rv)) {
         ruleCount += RulesMatching(mBackstopSheets, aPresContext, aContent, aParentContext, rules);
         NS_RELEASE(htmlContent);
      }
      PRInt32 backstopRules = ruleCount;
      ruleCount += RulesMatching(mDocSheets, aPresContext, aContent, aParentContext, rules);
      ruleCount += RulesMatching(mOverrideSheets, aPresContext, aContent, aParentContext, rules);

      PRBool usedRules = PR_FALSE;
      if (0 < ruleCount) {
        SortRulesByStrength(rules, backstopRules);
        result = GetContext(aPresContext, aParentContext, nsnull, rules, aForceUnique, usedRules);
        if (usedRules) {
          NS_ASSERT_REFCOUNT(rules, 2, "rules array was used elsewhere");
          NS_RELEASE(rules);
        }
        else {
          NS_ASSERT_REFCOUNT(rules, 1, "rules array was used elsewhere");
          rules->Clear();
          mRecycler = rules;
        }
      }
      else {
        NS_ASSERT_REFCOUNT(rules, 1, "rules array was used elsewhere");
        mRecycler = rules;
        result = GetContext(aPresContext, aParentContext, nsnull, nsnull, aForceUnique, usedRules);
      }
    }
  }

  return result;
}


PRInt32 StyleSetImpl::RulesMatching(nsISupportsArray* aSheets,
                                    nsIPresContext* aPresContext,
                                    nsIContent* aParentContent,
                                    nsIAtom* aPseudoTag,
                                    nsIStyleContext* aParentContext,
                                    nsISupportsArray* aResults)
{
  PRInt32 ruleCount = 0;

  if (nsnull != aSheets) {
    PRInt32 index = aSheets->Count();
    while (0 < index--) {
      nsIStyleSheet* sheet = (nsIStyleSheet*)aSheets->ElementAt(index);
      ruleCount += sheet->RulesMatching(aPresContext, aParentContent, aPseudoTag, 
                                        aParentContext, aResults);
      NS_RELEASE(sheet);
    }
  }
  return ruleCount;
}

nsIStyleContext* StyleSetImpl::ResolvePseudoStyleFor(nsIPresContext* aPresContext,
                                                     nsIContent* aParentContent,
                                                     nsIAtom* aPseudoTag,
                                                     nsIStyleContext* aParentContext,
                                                     PRBool aForceUnique)
{
  nsIStyleContext*  result = nsnull;
  // want to check parent frame's context for cached child context first

  // then do a brute force rule search

  nsISupportsArray*  rules = mRecycler;
  mRecycler = nsnull;
  if (nsnull == rules) {
    NS_NewISupportsArray(&rules);
  }

  if (nsnull != rules) {
    PRInt32 ruleCount = RulesMatching(mBackstopSheets, aPresContext, 
                                      aParentContent, aPseudoTag, 
                                      aParentContext, rules);
    PRInt32 backstopRules = ruleCount;
    ruleCount += RulesMatching(mDocSheets, aPresContext, 
                               aParentContent, aPseudoTag, 
                               aParentContext, rules);
    ruleCount += RulesMatching(mOverrideSheets, aPresContext, 
                               aParentContent, aPseudoTag, 
                               aParentContext, rules);

    PRBool usedRules = PR_FALSE;
    if (0 < ruleCount) {
      SortRulesByStrength(rules, backstopRules);
      result = GetContext(aPresContext, aParentContext, aPseudoTag, rules, aForceUnique, usedRules);
      if (usedRules) {
        NS_ASSERT_REFCOUNT(rules, 2, "rules array was used elsewhere");
        NS_RELEASE(rules);
      }
      else {
        NS_ASSERT_REFCOUNT(rules, 1, "rules array was used elsewhere");
        rules->Clear();
        mRecycler = rules;
      }
    }
    else {
      NS_ASSERT_REFCOUNT(rules, 1, "rules array was used elsewhere");
      mRecycler = rules;
      result = GetContext(aPresContext, aParentContext, aPseudoTag, nsnull, aForceUnique, usedRules);
    }
  }

  return result;
}

nsIStyleContext* StyleSetImpl::ProbePseudoStyleFor(nsIPresContext* aPresContext,
                                                   nsIContent* aParentContent,
                                                   nsIAtom* aPseudoTag,
                                                   nsIStyleContext* aParentContext,
                                                   PRBool aForceUnique)
{
  nsIStyleContext*  result = nsnull;
  // want to check parent frame's context for cached child context first

  // then do a brute force rule search

  nsISupportsArray*  rules = mRecycler;
  mRecycler = nsnull;
  if (nsnull == rules) {
    NS_NewISupportsArray(&rules);
  }

  if (nsnull != rules) {
    PRInt32 ruleCount = RulesMatching(mBackstopSheets, aPresContext, 
                                      aParentContent, aPseudoTag, 
                                      aParentContext, rules);
    PRInt32 backstopRules = ruleCount;
    ruleCount += RulesMatching(mDocSheets, aPresContext, 
                               aParentContent, aPseudoTag, 
                               aParentContext, rules);
    ruleCount += RulesMatching(mOverrideSheets, aPresContext, 
                               aParentContent, aPseudoTag, 
                               aParentContext, rules);

    PRBool usedRules = PR_FALSE;
    if (0 < ruleCount) {
      SortRulesByStrength(rules, backstopRules);
      result = GetContext(aPresContext, aParentContext, aPseudoTag, rules, aForceUnique, usedRules);
      if (usedRules) {
        NS_ASSERT_REFCOUNT(rules, 2, "rules array was used elsewhere");
        NS_RELEASE(rules);
      }
      else {
        NS_ASSERT_REFCOUNT(rules, 1, "rules array was used elsewhere");
        rules->Clear();
        mRecycler = rules;
      }
    }
    else {
      NS_ASSERT_REFCOUNT(rules, 1, "rules array was used elsewhere");
      mRecycler = rules;
    }
  }

  return result;
}

NS_IMETHODIMP StyleSetImpl::ConstructFrame(nsIPresContext* aPresContext,
                                           nsIContent*     aContent,
                                           nsIFrame*       aParentFrame,
                                           nsIFrame*&      aFrameSubTree)
{
  return mFrameConstructor->ConstructFrame(aPresContext, aContent,
                                           aParentFrame, aFrameSubTree);
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
                                            aChild, aIndexInContainer);
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
                                           aChild, aIndexInContainer);
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
StyleSetImpl::AttributeChanged(nsIPresContext* aPresContext,
                               nsIContent* aContent,
                               nsIAtom* aAttribute,
                               PRInt32 aHint)
{
  return mFrameConstructor->AttributeChanged(aPresContext, aContent, 
                                             aAttribute, aHint);
}

// xxx style rules enumeration

void StyleSetImpl::List(FILE* out, PRInt32 aIndent, nsISupportsArray* aSheets)
{
  PRInt32 count = ((nsnull != aSheets) ? aSheets->Count() : 0);

  for (PRInt32 index = 0; index < count; index++) {
    nsIStyleSheet* sheet = (nsIStyleSheet*)aSheets->ElementAt(index);
    sheet->List(out, aIndent);
    fputs("\n", out);
    NS_RELEASE(sheet);
  }
}

void StyleSetImpl::List(FILE* out, PRInt32 aIndent)
{
  List(out, aIndent, mOverrideSheets);
  List(out, aIndent, mDocSheets);
  List(out, aIndent, mBackstopSheets);
}

void StyleSetImpl::ListContexts(nsIStyleContext* aRootContext, FILE* out, PRInt32 aIndent)
{
  aRootContext->List(out, aIndent);
}


NS_LAYOUT nsresult
NS_NewStyleSet(nsIStyleSet** aInstancePtrResult)
{
  if (aInstancePtrResult == nsnull) {
    return NS_ERROR_NULL_POINTER;
  }

  StyleSetImpl  *it = new StyleSetImpl();

  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return it->QueryInterface(kIStyleSetIID, (void **) aInstancePtrResult);
}
