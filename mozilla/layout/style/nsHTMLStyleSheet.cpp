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
#include "nsIHTMLStyleSheet.h"
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
#include "nsTableFrame.h"
#include "nsHTMLIIDs.h"
#include "nsIStyleFrameConstruction.h"
#include "nsHTMLParts.h"
#include "nsIPresShell.h"
#include "nsIViewManager.h"
#include "nsStyleConsts.h"
#include "nsTableOuterFrame.h"

static NS_DEFINE_IID(kIHTMLStyleSheetIID, NS_IHTML_STYLE_SHEET_IID);
static NS_DEFINE_IID(kIStyleSheetIID, NS_ISTYLE_SHEET_IID);
static NS_DEFINE_IID(kIStyleRuleIID, NS_ISTYLE_RULE_IID);
static NS_DEFINE_IID(kIStyleFrameConstructionIID, NS_ISTYLE_FRAME_CONSTRUCTION_IID);
static NS_DEFINE_IID(kIHTMLTableCellElementIID, NS_IHTMLTABLECELLELEMENT_IID);


class HTMLAnchorRule : public nsIStyleRule {
public:
  HTMLAnchorRule();
  ~HTMLAnchorRule();

  NS_DECL_ISUPPORTS

  NS_IMETHOD Equals(const nsIStyleRule* aRule, PRBool& aValue) const;
  NS_IMETHOD HashValue(PRUint32& aValue) const;
  // Strength is an out-of-band weighting, always 0 here
  NS_IMETHOD GetStrength(PRInt32& aStrength);

  NS_IMETHOD MapStyleInto(nsIStyleContext* aContext, nsIPresContext* aPresContext);

  NS_IMETHOD List(FILE* out = stdout, PRInt32 aIndent = 0) const;

  nscolor mColor;
};

HTMLAnchorRule::HTMLAnchorRule()
{
  NS_INIT_REFCNT();
}

HTMLAnchorRule::~HTMLAnchorRule()
{
}

NS_IMPL_ISUPPORTS(HTMLAnchorRule, kIStyleRuleIID);

NS_IMETHODIMP
HTMLAnchorRule::Equals(const nsIStyleRule* aRule, PRBool& aResult) const
{
  aResult = PRBool(this == aRule);
  return NS_OK;
}

NS_IMETHODIMP
HTMLAnchorRule::HashValue(PRUint32& aValue) const
{
  aValue = (PRUint32)(mColor);
  return NS_OK;
}

// Strength is an out-of-band weighting, always 0 here
NS_IMETHODIMP
HTMLAnchorRule::GetStrength(PRInt32& aStrength)
{
  aStrength = 0;
  return NS_OK;
}

NS_IMETHODIMP
HTMLAnchorRule::MapStyleInto(nsIStyleContext* aContext, nsIPresContext* aPresContext)
{
  nsStyleColor* styleColor = (nsStyleColor*)(aContext->GetMutableStyleData(eStyleStruct_Color));

  if (nsnull != styleColor) {
    styleColor->mColor = mColor;
  }
  return NS_OK;
}

NS_IMETHODIMP
HTMLAnchorRule::List(FILE* out, PRInt32 aIndent) const
{
  return NS_OK;
}

// -----------------------------------------------------------

class AttributeKey: public nsHashKey
{
public:
  AttributeKey(nsMapAttributesFunc aMapFunc, nsIHTMLAttributes* aAttributes);
  virtual ~AttributeKey(void);

  PRBool      Equals(const nsHashKey* aOther) const;
  PRUint32    HashValue(void) const;
  nsHashKey*  Clone(void) const;

private:
  AttributeKey(void);
  AttributeKey(const AttributeKey& aCopy);
  AttributeKey& operator=(const AttributeKey& aCopy);

public:
  nsMapAttributesFunc mMapFunc;
  nsIHTMLAttributes*  mAttributes;
  PRUint32            mHashSet: 1;
  PRUint32            mHashCode: 31;
};

AttributeKey::AttributeKey(nsMapAttributesFunc aMapFunc, nsIHTMLAttributes* aAttributes)
  : mMapFunc(aMapFunc),
    mAttributes(aAttributes)
{
  NS_ADDREF(mAttributes);
  mHashSet = 0;
}

AttributeKey::~AttributeKey(void)
{
  NS_RELEASE(mAttributes);
}

PRBool AttributeKey::Equals(const nsHashKey* aOther) const
{
  const AttributeKey* other = (const AttributeKey*)aOther;
  if (mMapFunc == other->mMapFunc) {
    PRBool  equals;
    mAttributes->Equals(other->mAttributes, equals);
    return equals;
  }
  return PR_FALSE;
}

PRUint32 AttributeKey::HashValue(void) const
{
  if (0 == mHashSet) {
    AttributeKey* self = (AttributeKey*)this; // break const
    PRUint32  hash;
    mAttributes->HashValue(hash);
    self->mHashCode = (0x7FFFFFFF & hash);
    self->mHashCode |= (0x7FFFFFFF & PRUint32(mMapFunc));
    self->mHashSet = 1;
  }
  return mHashCode;
}

nsHashKey* AttributeKey::Clone(void) const
{
  AttributeKey* clown = new AttributeKey(mMapFunc, mAttributes);
  if (nsnull != clown) {
    clown->mHashSet = mHashSet;
    clown->mHashCode = mHashCode;
  }
  return clown;
}

// -----------------------------------------------------------

class HTMLStyleSheetImpl : public nsIHTMLStyleSheet,
                           public nsIStyleFrameConstruction {
public:
  void* operator new(size_t size);
  void* operator new(size_t size, nsIArena* aArena);
  void operator delete(void* ptr);

  HTMLStyleSheetImpl(nsIURL* aURL);

  NS_IMETHOD QueryInterface(const nsIID& aIID, void** aInstancePtr);
  NS_IMETHOD_(nsrefcnt) AddRef();
  NS_IMETHOD_(nsrefcnt) Release();

  virtual PRInt32 RulesMatching(nsIPresContext* aPresContext,
                                nsIContent* aContent,
                                nsIFrame* aParentFrame,
                                nsISupportsArray* aResults);

  virtual PRInt32 RulesMatching(nsIPresContext* aPresContext,
                                nsIAtom* aPseudoTag,
                                nsIFrame* aParentFrame,
                                nsISupportsArray* aResults);

  virtual nsIURL* GetURL(void);

  NS_IMETHOD SetLinkColor(nscolor aColor);
  NS_IMETHOD SetActiveLinkColor(nscolor aColor);
  NS_IMETHOD SetVisitedLinkColor(nscolor aColor);

  // Attribute management methods, aAttributes is an in/out param
  NS_IMETHOD SetAttributesFor(nsIHTMLContent* aContent, 
                              nsIHTMLAttributes*& aAttributes);
  NS_IMETHOD SetIDFor(nsIAtom* aID, nsIHTMLContent* aContent, 
                      nsIHTMLAttributes*& aAttributes);
  NS_IMETHOD SetClassFor(nsIAtom* aClass, nsIHTMLContent* aContent, 
                         nsIHTMLAttributes*& aAttributes);
  NS_IMETHOD SetAttributeFor(nsIAtom* aAttribute, const nsString& aValue, 
                             nsIHTMLContent* aContent, 
                             nsIHTMLAttributes*& aAttributes);
  NS_IMETHOD SetAttributeFor(nsIAtom* aAttribute, const nsHTMLValue& aValue, 
                             nsIHTMLContent* aContent, 
                             nsIHTMLAttributes*& aAttributes);
  NS_IMETHOD UnsetAttributeFor(nsIAtom* aAttribute, nsIHTMLContent* aContent, 
                               nsIHTMLAttributes*& aAttributes);

  NS_IMETHOD ConstructFrame(nsIPresContext* aPresContext,
                            nsIContent*     aContent,
                            nsIFrame*       aParentFrame,
                            nsIFrame*&      aFrameSubTree);

  NS_IMETHOD ContentAppended(nsIPresContext* aPresContext,
                             nsIDocument*    aDocument,
                             nsIContent*     aContainer,
                             PRInt32         aNewIndexInContainer);

  NS_IMETHOD ContentInserted(nsIPresContext* aPresContext,
                             nsIDocument*    aDocument,
                             nsIContent*     aContainer,
                             nsIContent*     aChild,
                             PRInt32         aIndexInContainer);

  // XXX style rule enumerations

  virtual void List(FILE* out = stdout, PRInt32 aIndent = 0) const;

private: 
  // These are not supported and are not implemented! 
  HTMLStyleSheetImpl(const HTMLStyleSheetImpl& aCopy); 
  HTMLStyleSheetImpl& operator=(const HTMLStyleSheetImpl& aCopy); 

protected:
  virtual ~HTMLStyleSheetImpl();

  NS_IMETHOD EnsureSingleAttributes(nsIHTMLAttributes*& aAttributes, 
                                    nsMapAttributesFunc aMapFunc,
                                    PRBool aCreate, 
                                    nsIHTMLAttributes*& aSingleAttrs);
  NS_IMETHOD UniqueAttributes(nsIHTMLAttributes*& aSingleAttrs,
                              nsMapAttributesFunc aMapFunc,
                              PRInt32 aAttrCount,
                              nsIHTMLAttributes*& aAttributes);

  nsresult ConstructRootFrame(nsIPresContext*  aPresContext,
                              nsIContent*      aContent,
                              nsIStyleContext* aStyleContext,
                              nsIFrame*&       aNewFrame);

  nsresult ConstructTableFrame(nsIPresContext*  aPresContext,
                               nsIContent*      aContent,
                               nsIFrame*        aParentFrame,
                               nsIStyleContext* aStyleContext,
                               nsIFrame*&       aNewFrame);

  nsresult ConstructFrameByTag(nsIPresContext*  aPresContext,
                               nsIContent*      aContent,
                               nsIFrame*        aParentFrame,
                               nsIAtom*         aTag,
                               nsIStyleContext* aStyleContext,
                               nsIFrame*&       aNewFrame);

  nsresult ConstructFrameByDisplayType(nsIPresContext*  aPresContext,
                                       nsIContent*      aContent,
                                       nsIFrame*        aParentFrame,
                                       nsIStyleContext* aStyleContext,
                                       nsIFrame*&       aNewFrame);

  nsresult ProcessChildren(nsIPresContext* aPresContext,
                           nsIFrame*       aFrame,
                           nsIContent*     aContent,
                           nsIFrame*&      aChildList);

  nsresult CreateInputFrame(nsIContent* aContent,
                            nsIFrame*   aParentFrame,
                            nsIFrame*&  aFrame);

protected:
  PRUint32 mInHeap : 1;
  PRUint32 mRefCnt : 31;

  nsIURL*         mURL;
  HTMLAnchorRule* mLinkRule;
  HTMLAnchorRule* mVisitedRule;
  HTMLAnchorRule* mActiveRule;
  nsHashtable     mAttrTable;
  nsIHTMLAttributes*  mRecycledAttrs;
};


void* HTMLStyleSheetImpl::operator new(size_t size)
{
  HTMLStyleSheetImpl* rv = (HTMLStyleSheetImpl*) ::operator new(size);
#ifdef NS_DEBUG
  if (nsnull != rv) {
    nsCRT::memset(rv, 0xEE, size);
  }
#endif
  rv->mInHeap = 1;
  return (void*) rv;
}

void* HTMLStyleSheetImpl::operator new(size_t size, nsIArena* aArena)
{
  HTMLStyleSheetImpl* rv = (HTMLStyleSheetImpl*) aArena->Alloc(PRInt32(size));
#ifdef NS_DEBUG
  if (nsnull != rv) {
    nsCRT::memset(rv, 0xEE, size);
  }
#endif
  rv->mInHeap = 0;
  return (void*) rv;
}

void HTMLStyleSheetImpl::operator delete(void* ptr)
{
  HTMLStyleSheetImpl* sheet = (HTMLStyleSheetImpl*) ptr;
  if (nsnull != sheet) {
    if (sheet->mInHeap) {
      ::delete ptr;
    }
  }
}



HTMLStyleSheetImpl::HTMLStyleSheetImpl(nsIURL* aURL)
  : nsIHTMLStyleSheet(),
    mURL(aURL),
    mLinkRule(nsnull),
    mVisitedRule(nsnull),
    mActiveRule(nsnull),
    mRecycledAttrs(nsnull)
{
  NS_INIT_REFCNT();
  NS_ADDREF(mURL);
}

HTMLStyleSheetImpl::~HTMLStyleSheetImpl()
{
  NS_RELEASE(mURL);
  NS_IF_RELEASE(mLinkRule);
  NS_IF_RELEASE(mVisitedRule);
  NS_IF_RELEASE(mActiveRule);
  NS_IF_RELEASE(mRecycledAttrs);
}

NS_IMPL_ADDREF(HTMLStyleSheetImpl)
NS_IMPL_RELEASE(HTMLStyleSheetImpl)

nsresult HTMLStyleSheetImpl::QueryInterface(const nsIID& aIID,
                                            void** aInstancePtrResult)
{
  NS_PRECONDITION(nsnull != aInstancePtrResult, "null pointer");
  if (nsnull == aInstancePtrResult) {
    return NS_ERROR_NULL_POINTER;
  }
  static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
  if (aIID.Equals(kIHTMLStyleSheetIID)) {
    *aInstancePtrResult = (void*) ((nsIHTMLStyleSheet*)this);
    AddRef();
    return NS_OK;
  }
  if (aIID.Equals(kIStyleSheetIID)) {
    *aInstancePtrResult = (void*) ((nsIStyleSheet*)this);
    AddRef();
    return NS_OK;
  }
  if (aIID.Equals(kIStyleFrameConstructionIID)) {
    *aInstancePtrResult = (void*) ((nsIStyleFrameConstruction*)this);
    AddRef();
    return NS_OK;
  }
  if (aIID.Equals(kISupportsIID)) {
    *aInstancePtrResult = (void*) this;
    AddRef();
    return NS_OK;
  }
  return NS_NOINTERFACE;
}

struct AppendData 
{
  AppendData(PRInt32 aBackstopCount, PRInt32 aInsertPoint, nsISupportsArray* aResults)
    : mCount(0),
      mBackstop(aBackstopCount),
      mInsert(aInsertPoint),
      mResults(aResults)
  {}

  PRInt32 mCount;
  PRInt32 mBackstop;
  PRInt32 mInsert;
  nsISupportsArray* mResults;
};

PRBool AppendFunc(nsISupports* aElement, void *aData)
{
  AppendData* data = (AppendData*)aData;
  if (data->mCount < data->mBackstop) {
    data->mResults->InsertElementAt(aElement, data->mInsert++);
  }
  else {
    data->mResults->AppendElement(aElement);
  }
  data->mCount++;
  return PR_TRUE;
}

PRInt32 AppendRulesFrom(nsIFrame* aFrame, nsIPresContext* aPresContext, PRInt32& aInsertPoint, nsISupportsArray* aResults)
{
  PRInt32 count = 0;
  nsIStyleContext*  context;
  aFrame->GetStyleContext(aPresContext, context);
  if (nsnull != context) {
    count = context->GetStyleRuleCount();
    if (0 < count) {
      PRInt32 backstopCount = context->GetBackstopStyleRuleCount();
      nsISupportsArray* rules = context->GetStyleRules();

      AppendData  data(backstopCount, aInsertPoint, aResults);
      rules->EnumerateForwards(AppendFunc, &data);
      aInsertPoint = data.mInsert;
      NS_RELEASE(rules);
    }
    NS_RELEASE(context);
  }
  return count;
}

PRInt32 HTMLStyleSheetImpl::RulesMatching(nsIPresContext* aPresContext,
                                          nsIContent* aContent,
                                          nsIFrame* aParentFrame,
                                          nsISupportsArray* aResults)
{
  NS_PRECONDITION(nsnull != aPresContext, "null arg");
  NS_PRECONDITION(nsnull != aContent, "null arg");
//  NS_PRECONDITION(nsnull != aParentFrame, "null arg");
  NS_PRECONDITION(nsnull != aResults, "null arg");

  PRInt32 matchCount = 0;

  nsIContent* parentContent = nsnull;
  if (nsnull != aParentFrame) {
    aParentFrame->GetContent(parentContent);
  }

  if (aContent != parentContent) {  // if not a pseudo frame...
    nsIHTMLContent* htmlContent;
    if (NS_OK == aContent->QueryInterface(kIHTMLContentIID, (void**)&htmlContent)) {
      nsIAtom*  tag;
      htmlContent->GetTag(tag);
      // if we have anchor colors, check if this is an anchor with an href
      if (tag == nsHTMLAtoms::a) {
        if ((nsnull != mLinkRule) || (nsnull != mVisitedRule) || (nsnull != mActiveRule)) {
          // test link state
          nsILinkHandler* linkHandler;

          if ((NS_OK == aPresContext->GetLinkHandler(&linkHandler)) &&
              (nsnull != linkHandler)) {
            nsAutoString base, href;  // XXX base??
            nsresult attrState = htmlContent->GetAttribute("href", href);

            if (NS_CONTENT_ATTR_HAS_VALUE == attrState) {
              nsIURL* docURL = nsnull;
              nsIDocument* doc = nsnull;
              aContent->GetDocument(doc);
              if (nsnull != doc) {
                docURL = doc->GetDocumentURL();
                NS_RELEASE(doc);
              }

              nsAutoString absURLSpec;
              nsresult rv = NS_MakeAbsoluteURL(docURL, base, href, absURLSpec);
              NS_IF_RELEASE(docURL);

              nsLinkState  state;
              if (NS_OK == linkHandler->GetLinkState(absURLSpec, state)) {
                switch (state) {
                  case eLinkState_Unvisited:
                    if (nsnull != mLinkRule) {
                      aResults->AppendElement(mLinkRule);
                      matchCount++;
                    }
                    break;
                  case eLinkState_Visited:
                    if (nsnull != mVisitedRule) {
                      aResults->AppendElement(mVisitedRule);
                      matchCount++;
                    }
                    break;
                  case eLinkState_Active:
                    if (nsnull != mActiveRule) {
                      aResults->AppendElement(mActiveRule);
                      matchCount++;
                    }
                    break;
                }
              }
            }
            NS_RELEASE(linkHandler);
          }
        }
      } // end A tag
      else if ((tag == nsHTMLAtoms::td) || (tag == nsHTMLAtoms::th)) {
        // propogate row/col style rules
        // XXX: this approach has a few caveats
        //      behavior is bound to HTML content
        //      makes table cells process attributes from other content
        //      inherit based style (ie: font-size: 110%) can double on row & cell
        PRInt32 backstopInsertPoint = 0;

        nsIFrame* rowFrame = aParentFrame;
        nsIFrame* rowGroupFrame;
        nsIFrame* tableFrame;

        rowFrame->GetContentParent(rowGroupFrame);
        rowGroupFrame->GetContentParent(tableFrame);

        nsIHTMLTableCellElement* cell=nsnull;  
        nsresult rv = aContent->QueryInterface(kIHTMLTableCellElementIID, 
                                       (void **)&cell);  // cell: REFCNT++
        if (NS_SUCCEEDED(rv)) {
          PRInt32 colIndex;
          rv = cell->GetColIndex(&colIndex);
          nsTableColFrame* colFrame;
          nsIFrame* colGroupFrame;

        // XXX CONSTRUCTION.
        // Unfortunately the table's children haven't been added yet...
#if 0
          ((nsTableFrame*)tableFrame)->GetColumnFrame(colIndex, colFrame);
          colFrame->GetContentParent(colGroupFrame);

          matchCount += AppendRulesFrom(colGroupFrame, aPresContext, backstopInsertPoint, aResults);
          matchCount += AppendRulesFrom(colFrame, aPresContext, backstopInsertPoint, aResults);
#endif
          NS_RELEASE(cell);                             // cell: REFCNT--
        }
        matchCount += AppendRulesFrom(rowGroupFrame, aPresContext, backstopInsertPoint, aResults);
        matchCount += AppendRulesFrom(rowFrame, aPresContext, backstopInsertPoint, aResults);
      } // end TD/TH tag

      // just get the one and only style rule from the content
      nsIStyleRule* rule;
      htmlContent->GetStyleRule(rule);
      if (nsnull != rule) {
        aResults->AppendElement(rule);
        NS_RELEASE(rule);
        matchCount++;
      }

      NS_IF_RELEASE(tag);
      NS_RELEASE(htmlContent);
    }
  }
  NS_IF_RELEASE(parentContent);

  return matchCount;
}

PRInt32 HTMLStyleSheetImpl::RulesMatching(nsIPresContext* aPresContext,
                                          nsIAtom* aPseudoTag,
                                          nsIFrame* aParentFrame,
                                          nsISupportsArray* aResults)
{
  // no pseudo frame style
  return 0;
}


nsIURL* HTMLStyleSheetImpl::GetURL(void)
{
  NS_ADDREF(mURL);
  return mURL;
}


NS_IMETHODIMP HTMLStyleSheetImpl::SetLinkColor(nscolor aColor)
{
  if (nsnull == mLinkRule) {
    mLinkRule = new HTMLAnchorRule();
    if (nsnull == mLinkRule) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    NS_ADDREF(mLinkRule);
  }
  mLinkRule->mColor = aColor;
  return NS_OK;
}

NS_IMETHODIMP HTMLStyleSheetImpl::SetActiveLinkColor(nscolor aColor)
{
  if (nsnull == mActiveRule) {
    mActiveRule = new HTMLAnchorRule();
    if (nsnull == mActiveRule) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    NS_ADDREF(mActiveRule);
  }
  mActiveRule->mColor = aColor;
  return NS_OK;
}

NS_IMETHODIMP HTMLStyleSheetImpl::SetVisitedLinkColor(nscolor aColor)
{
  if (nsnull == mVisitedRule) {
    mVisitedRule = new HTMLAnchorRule();
    if (nsnull == mVisitedRule) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    NS_ADDREF(mVisitedRule);
  }
  mVisitedRule->mColor = aColor;
  return NS_OK;
}

NS_IMETHODIMP HTMLStyleSheetImpl::SetAttributesFor(nsIHTMLContent* aContent, nsIHTMLAttributes*& aAttributes)
{
  nsIHTMLAttributes*  attrs = aAttributes;

  if (nsnull != attrs) {
    nsMapAttributesFunc mapFunc;
    aContent->GetAttributeMappingFunction(mapFunc);
    AttributeKey  key(mapFunc, attrs);
    nsIHTMLAttributes* sharedAttrs = (nsIHTMLAttributes*)mAttrTable.Get(&key);
    if (nsnull == sharedAttrs) {  // we have a new unique set
      mAttrTable.Put(&key, attrs);
    }
    else {  // found existing set
      if (sharedAttrs != aAttributes) {
        aAttributes->ReleaseContentRef();
        NS_RELEASE(aAttributes);  // release content's ref

        aAttributes = sharedAttrs;
        aAttributes->AddContentRef();
        NS_ADDREF(aAttributes); // add ref for content
      }
    }
  }

  return NS_OK;
}

NS_IMETHODIMP HTMLStyleSheetImpl::SetIDFor(nsIAtom* aID, nsIHTMLContent* aContent, nsIHTMLAttributes*& aAttributes)
{
  nsresult            result = NS_OK;
  nsIHTMLAttributes*  attrs;
  PRBool              hasValue = PRBool(nsnull != aID);
  nsMapAttributesFunc mapFunc;

  aContent->GetAttributeMappingFunction(mapFunc);

  result = EnsureSingleAttributes(aAttributes, mapFunc, hasValue, attrs);

  if ((NS_OK == result) && (nsnull != attrs)) {
    PRInt32 count;
    attrs->SetID(aID, count);

    result = UniqueAttributes(attrs, mapFunc, count, aAttributes);
  }

  return result;
}

NS_IMETHODIMP HTMLStyleSheetImpl::SetClassFor(nsIAtom* aClass, nsIHTMLContent* aContent, nsIHTMLAttributes*& aAttributes)
{
  nsresult            result = NS_OK;
  nsIHTMLAttributes*  attrs;
  PRBool              hasValue = PRBool(nsnull != aClass);
  nsMapAttributesFunc mapFunc;

  aContent->GetAttributeMappingFunction(mapFunc);

  result = EnsureSingleAttributes(aAttributes, mapFunc, hasValue, attrs);

  if ((NS_OK == result) && (nsnull != attrs)) {
    PRInt32 count;
    attrs->SetClass(aClass, count);

    result = UniqueAttributes(attrs, mapFunc, count, aAttributes);
  }

  return result;
}


NS_IMETHODIMP HTMLStyleSheetImpl::SetAttributeFor(nsIAtom* aAttribute, 
                                                  const nsString& aValue,
                                                  nsIHTMLContent* aContent, 
                                                  nsIHTMLAttributes*& aAttributes)
{
  nsresult            result = NS_OK;
  nsIHTMLAttributes*  attrs;
  nsMapAttributesFunc mapFunc;

  aContent->GetAttributeMappingFunction(mapFunc);

  result = EnsureSingleAttributes(aAttributes, mapFunc, PR_TRUE, attrs);

  if ((NS_OK == result) && (nsnull != attrs)) {
    PRInt32 count;
    attrs->SetAttribute(aAttribute, aValue, count);

    result = UniqueAttributes(attrs, mapFunc, count, aAttributes);
  }

  return result;
}

NS_IMETHODIMP
HTMLStyleSheetImpl::EnsureSingleAttributes(nsIHTMLAttributes*& aAttributes, 
                                           nsMapAttributesFunc aMapFunc,
                                           PRBool aCreate, 
                                           nsIHTMLAttributes*& aSingleAttrs)
{
  nsresult  result = NS_OK;
  PRInt32   contentRefCount;

  if (nsnull == aAttributes) {
    if (PR_TRUE == aCreate) {
      if (nsnull != mRecycledAttrs) {
        aSingleAttrs = mRecycledAttrs;
        mRecycledAttrs = nsnull;
        aSingleAttrs->SetMappingFunction(aMapFunc);
      }
      else {
        result = NS_NewHTMLAttributes(&aSingleAttrs, aMapFunc);
      }
    }
    else {
      aSingleAttrs = nsnull;
    }
    contentRefCount = 0;
  }
  else {
    aSingleAttrs = aAttributes;
    aSingleAttrs->GetContentRefCount(contentRefCount);
    NS_ASSERTION(0 < contentRefCount, "bad content ref count");
  }

  if (NS_OK == result) {
    if (1 < contentRefCount) {  // already shared, copy it
      result = aSingleAttrs->Clone(&aSingleAttrs);
      if (NS_OK != result) {
        aSingleAttrs = nsnull;
        return result;
      }
      contentRefCount = 0;
      aAttributes->ReleaseContentRef();
      NS_RELEASE(aAttributes);
    }
    else {  // one content ref, ok to use, remove from table because hash may change
      if (1 == contentRefCount) {
        AttributeKey  key(aMapFunc, aSingleAttrs);
        mAttrTable.Remove(&key);
        NS_ADDREF(aSingleAttrs); // add a local ref so we match up 
      }
    }
    // at this point, content ref count is 0 or 1, and we hold a local ref
    // attrs is also unique and not in the table
    NS_ASSERTION(((0 == contentRefCount) && (nsnull == aAttributes)) ||
                 ((1 == contentRefCount) && (aSingleAttrs == aAttributes)), "this is broken");
  }
  return result;
}

NS_IMETHODIMP
HTMLStyleSheetImpl::UniqueAttributes(nsIHTMLAttributes*& aSingleAttrs,
                                     nsMapAttributesFunc aMapFunc,
                                     PRInt32 aAttrCount,
                                     nsIHTMLAttributes*& aAttributes)
{
  nsresult result = NS_OK;

  if (0 < aAttrCount) {
    AttributeKey  key(aMapFunc, aSingleAttrs);
    nsIHTMLAttributes* sharedAttrs = (nsIHTMLAttributes*)mAttrTable.Get(&key);
    if (nsnull == sharedAttrs) {  // we have a new unique set
      mAttrTable.Put(&key, aSingleAttrs);
      if (aSingleAttrs != aAttributes) {
        NS_ASSERTION(nsnull == aAttributes, "this is broken");
        aAttributes = aSingleAttrs;
        aAttributes->AddContentRef(); 
        NS_ADDREF(aAttributes); // add ref for content
      }
    }
    else {  // found existing set
      NS_ASSERTION (sharedAttrs != aAttributes, "should never happen");
      if (nsnull != aAttributes) {
        aAttributes->ReleaseContentRef();
        NS_RELEASE(aAttributes);  // release content's ref
      }
      aAttributes = sharedAttrs;
      aAttributes->AddContentRef();
      NS_ADDREF(aAttributes); // add ref for content

      if (nsnull == mRecycledAttrs) {
        mRecycledAttrs = aSingleAttrs;
        NS_ADDREF(mRecycledAttrs);
        mRecycledAttrs->Reset();
      }
    }
  }
  else {  // no attributes to store
    if (nsnull != aAttributes) {
      aAttributes->ReleaseContentRef();
      NS_RELEASE(aAttributes);
    }
    if ((nsnull != aSingleAttrs) && (nsnull == mRecycledAttrs)) {
      mRecycledAttrs = aSingleAttrs;
      NS_ADDREF(mRecycledAttrs);
      mRecycledAttrs->Reset();
    }
  }
  NS_IF_RELEASE(aSingleAttrs);
  return result;
}



NS_IMETHODIMP HTMLStyleSheetImpl::SetAttributeFor(nsIAtom* aAttribute, 
                                                  const nsHTMLValue& aValue,
                                                  nsIHTMLContent* aContent, 
                                                  nsIHTMLAttributes*& aAttributes)
{
  nsresult            result = NS_OK;
  nsIHTMLAttributes*  attrs;
  PRBool              hasValue = PRBool(eHTMLUnit_Null != aValue.GetUnit());
  nsMapAttributesFunc mapFunc;

  aContent->GetAttributeMappingFunction(mapFunc);

  result = EnsureSingleAttributes(aAttributes, mapFunc, hasValue, attrs);

  if ((NS_OK == result) && (nsnull != attrs)) {
    PRInt32 count;
    attrs->SetAttribute(aAttribute, aValue, count);

    result = UniqueAttributes(attrs, mapFunc, count, aAttributes);
  }

  return result;
}

NS_IMETHODIMP HTMLStyleSheetImpl::UnsetAttributeFor(nsIAtom* aAttribute,
                                                    nsIHTMLContent* aContent, 
                                                    nsIHTMLAttributes*& aAttributes)
{
  nsresult            result = NS_OK;
  nsIHTMLAttributes*  attrs;
  nsMapAttributesFunc mapFunc;

  aContent->GetAttributeMappingFunction(mapFunc);

  result = EnsureSingleAttributes(aAttributes, mapFunc, PR_FALSE, attrs);

  if ((NS_OK == result) && (nsnull != attrs)) {
    PRInt32 count;
    attrs->UnsetAttribute(aAttribute, count);

    result = UniqueAttributes(attrs, mapFunc, count, aAttributes);
  }

  return result;

}

nsresult
HTMLStyleSheetImpl::ProcessChildren(nsIPresContext* aPresContext,
                                    nsIFrame*       aFrame,
                                    nsIContent*     aContent,
                                    nsIFrame*&      aChildList)
{
  // Initialize OUT parameter
  aChildList = nsnull;

  // Iterate the child content objects and construct a frame
  nsIFrame* lastChildFrame = nsnull;
  PRInt32   count;
  aContent->ChildCount(count);
  for (PRInt32 i = 0; i < count; i++) {
    nsIContent* childContent;
    aContent->ChildAt(i, childContent);

    if (nsnull != childContent) {
      nsIFrame* childFrame;

      // Construct a child frame
      ConstructFrame(aPresContext, childContent, aFrame, childFrame);

      if (nsnull != childFrame) {
        // Link the frame into the child list
        if (nsnull == lastChildFrame) {
          aChildList = childFrame;
        } else {
          lastChildFrame->SetNextSibling(childFrame);
        }
        lastChildFrame = childFrame;
      }

      NS_RELEASE(childContent);
    }
  }

  return NS_OK;
}

nsresult
HTMLStyleSheetImpl::CreateInputFrame(nsIContent* aContent,
                                     nsIFrame*   aParentFrame,
                                     nsIFrame*&  aFrame)
{
  nsresult  rv;

  // Figure out which type of input frame to create
  nsAutoString  val;
  if (NS_OK == aContent->GetAttribute(nsAutoString("type"), val)) {
    if (val.EqualsIgnoreCase("submit")) {
      rv = NS_NewInputButtonFrame(aContent, aParentFrame, aFrame);
    }
    else if (val.EqualsIgnoreCase("reset")) {
      rv = NS_NewInputButtonFrame(aContent, aParentFrame, aFrame);
    }
    else if (val.EqualsIgnoreCase("button")) {
      rv = NS_NewInputButtonFrame(aContent, aParentFrame, aFrame);
    }
    else if (val.EqualsIgnoreCase("checkbox")) {
      rv = NS_NewInputCheckboxFrame(aContent, aParentFrame, aFrame);
    }
    else if (val.EqualsIgnoreCase("file")) {
      rv = NS_NewInputFileFrame(aContent, aParentFrame, aFrame);
    }
    else if (val.EqualsIgnoreCase("hidden")) {
      rv = NS_NewInputButtonFrame(aContent, aParentFrame, aFrame);
    }
    else if (val.EqualsIgnoreCase("image")) {
      rv = NS_NewInputButtonFrame(aContent, aParentFrame, aFrame);
    }
    else if (val.EqualsIgnoreCase("password")) {
      rv = NS_NewInputTextFrame(aContent, aParentFrame, aFrame);
    }
    else if (val.EqualsIgnoreCase("radio")) {
      rv = NS_NewInputRadioFrame(aContent, aParentFrame, aFrame);
    }
    else if (val.EqualsIgnoreCase("text")) {
      rv = NS_NewInputTextFrame(aContent, aParentFrame, aFrame);
    }
    else {
      rv = NS_NewInputTextFrame(aContent, aParentFrame, aFrame);
    }
  } else {
    rv = NS_NewInputTextFrame(aContent, aParentFrame, aFrame);
  }

  return rv;
}

nsresult
HTMLStyleSheetImpl::ConstructTableFrame(nsIPresContext*  aPresContext,
                                        nsIContent*      aContent,
                                        nsIFrame*        aParentFrame,
                                        nsIStyleContext* aStyleContext,
                                        nsIFrame*&       aNewFrame)
{
  nsIFrame* childList;
  nsIFrame* innerFrame;
  nsIFrame* innerChildList = nsnull;
  nsIFrame* captionFrame = nsnull;

  // Create an anonymous table outer frame which holds the caption and the
  // table frame
  NS_NewTableOuterFrame(aContent, aParentFrame, aNewFrame);
  aNewFrame->SetStyleContext(aPresContext, aStyleContext);

  // Create the inner table frame
  NS_NewTableFrame(aContent, aNewFrame, innerFrame);
  childList = innerFrame;

  // Have the inner table frame use the same style context as the outer table frame
  innerFrame->SetStyleContext(aPresContext, aStyleContext);

  // Iterate the child content
  nsIFrame* lastChildFrame = nsnull;
  PRInt32   count;
  aContent->ChildCount(count);
  for (PRInt32 i = 0; i < count; i++) {
    nsIContent* childContent;
    aContent->ChildAt(i, childContent);

    if (nsnull != childContent) {
      nsIFrame*         frame = nsnull;
      nsIStyleContext*  childStyleContext;

      // Resolve the style context
      childStyleContext = aPresContext->ResolveStyleContextFor(childContent, aNewFrame);

      // See how it should be displayed
      const nsStyleDisplay* styleDisplay = (const nsStyleDisplay*)
        childStyleContext->GetStyleData(eStyleStruct_Display);

      switch (styleDisplay->mDisplay) {
      case NS_STYLE_DISPLAY_TABLE_CAPTION:
        // Have we already created a caption? If so, ignore this caption
        if (nsnull == captionFrame) {
          NS_NewBodyFrame(childContent, aNewFrame, captionFrame);
          captionFrame->SetStyleContext(aPresContext, childStyleContext);

          // Process the caption's child content and initialize it
          nsIFrame* captionChildList;
          ProcessChildren(aPresContext, captionFrame, childContent, captionChildList);
          captionFrame->Init(*aPresContext, captionChildList);

          // Prepend the caption frame to the outer frame's child list
          captionFrame->SetNextSibling(innerFrame);
          childList = captionFrame;
        }
        break;

      case NS_STYLE_DISPLAY_TABLE_HEADER_GROUP:
      case NS_STYLE_DISPLAY_TABLE_FOOTER_GROUP:
      case NS_STYLE_DISPLAY_TABLE_ROW_GROUP:
        NS_NewTableRowGroupFrame(childContent, innerFrame, frame);
        break;

      case NS_STYLE_DISPLAY_TABLE_COLUMN:
        NS_NewTableColFrame(childContent, innerFrame, frame);
        break;

      case NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP:
        NS_NewTableColGroupFrame(childContent, innerFrame, frame);
        break;

      default:
        // XXX For the time being ignore everything else. We should deal with
        // things like table cells and create anonymous frames...
        break;
      }

      // If it's not a caption frame, then set the style context and link the
      // frame into the inner frame's child list
      if (nsnull != frame) {
        frame->SetStyleContext(aPresContext, childStyleContext);

        // Process the children, and initialize the frame
        nsIFrame* childChildList;
        ProcessChildren(aPresContext, frame, childContent, childChildList);
        frame->Init(*aPresContext, childChildList);
  
        // Link the frame into the child list
        if (nsnull == lastChildFrame) {
          innerChildList = frame;
        } else {
          lastChildFrame->SetNextSibling(frame);
        }
        lastChildFrame = frame;
      }

      NS_RELEASE(childStyleContext);
      NS_RELEASE(childContent);
    }
  }

  // Initialize the inner table with its child list
  innerFrame->Init(*aPresContext, innerChildList);

  // Initialize the anonymous table outer frame
  aNewFrame->Init(*aPresContext, childList);
  return NS_OK;
}

nsresult
HTMLStyleSheetImpl::ConstructRootFrame(nsIPresContext*  aPresContext,
                                       nsIContent*      aContent,
                                       nsIStyleContext* aStyleContext,
                                       nsIFrame*&       aNewFrame)
{
    // This should only be the case for the root content object.
#ifdef NS_DEBUG
    nsIDocument*  doc;
    nsIContent*   rootContent;

    // Verify it's the root content object
    aContent->GetDocument(doc);
    rootContent = doc->GetRootContent();
    NS_ASSERTION(rootContent == aContent, "unexpected content");
    NS_RELEASE(doc);
    NS_RELEASE(rootContent);
#endif

  // Create the root frame
  nsresult  rv = NS_NewHTMLFrame(aContent, nsnull, aNewFrame);

  if (NS_SUCCEEDED(rv)) {
    // Bind root frame to root view (and root window)
    nsIPresShell*   presShell = aPresContext->GetShell();
    nsIViewManager* viewManager = presShell->GetViewManager();
    nsIView*        rootView;
  
    NS_RELEASE(presShell);
    viewManager->GetRootView(rootView);
    aNewFrame->SetView(rootView);
    NS_RELEASE(viewManager);
  
    // Set the style context
    aNewFrame->SetStyleContext(aPresContext, aStyleContext);
  
    // Process the child content, and initialize the frame
    nsIFrame* childList;
  
    rv = ProcessChildren(aPresContext, aNewFrame, aContent, childList);
    aNewFrame->Init(*aPresContext, childList);
  }

  return rv;
}

nsresult
HTMLStyleSheetImpl::ConstructFrameByTag(nsIPresContext*  aPresContext,
                                        nsIContent*      aContent,
                                        nsIFrame*        aParentFrame,
                                        nsIAtom*         aTag,
                                        nsIStyleContext* aStyleContext,
                                        nsIFrame*&       aNewFrame)
{
  PRBool    processChildren = PR_FALSE;  // whether we should process child content
  nsresult  rv = NS_OK;

  // Initialize OUT parameter
  aNewFrame = nsnull;

  if (nsnull == aTag) {
    rv = NS_NewTextFrame(aContent, aParentFrame, aNewFrame);
  }
  else if (nsHTMLAtoms::img == aTag) {
    rv = NS_NewImageFrame(aContent, aParentFrame, aNewFrame);
  }
  else if (nsHTMLAtoms::hr == aTag) {
    rv = NS_NewHRFrame(aContent, aParentFrame, aNewFrame);
  }
  else if (nsHTMLAtoms::br == aTag) {
    rv = NS_NewBRFrame(aContent, aParentFrame, aNewFrame);
  }
  else if (nsHTMLAtoms::wbr == aTag) {
    rv = NS_NewWBRFrame(aContent, aParentFrame, aNewFrame);
  }
  else if (nsHTMLAtoms::input == aTag) {
    rv = CreateInputFrame(aContent, aParentFrame, aNewFrame);
  }
  else if (nsHTMLAtoms::textarea == aTag) {
    rv = NS_NewInputTextFrame(aContent, aParentFrame, aNewFrame);
  }
  else if (nsHTMLAtoms::select == aTag) {
    rv = NS_NewHTMLSelectFrame(aContent, aParentFrame, aNewFrame);
  }
  else if (nsHTMLAtoms::applet == aTag) {
    rv = NS_NewObjectFrame(aContent, aParentFrame, aNewFrame);
  }
  else if (nsHTMLAtoms::embed == aTag) {
    rv = NS_NewObjectFrame(aContent, aParentFrame, aNewFrame);
  }
  else if (nsHTMLAtoms::object == aTag) {
    rv = NS_NewObjectFrame(aContent, aParentFrame, aNewFrame);
  }
  else if (nsHTMLAtoms::body == aTag) {
    rv = NS_NewBodyFrame(aContent, aParentFrame, aNewFrame);
    processChildren = PR_TRUE;
  }
  else if (nsHTMLAtoms::frameset == aTag) {
    rv = NS_NewHTMLFramesetFrame(aContent, aParentFrame, aNewFrame);
  }
  else if (nsHTMLAtoms::iframe == aTag) {
    rv = NS_NewHTMLFrameOuterFrame(aContent, aParentFrame, aNewFrame);
  }
  else if (nsHTMLAtoms::spacer == aTag) {
    rv = NS_NewSpacerFrame(aContent, aParentFrame, aNewFrame);
  }

  // If we succeeded in creating a frame then set its style context,
  // process its children (if requested), and initialize the frame
  if (NS_SUCCEEDED(rv) && (nsnull != aNewFrame)) {
    aNewFrame->SetStyleContext(aPresContext, aStyleContext);

    // Process the child content if requested
    nsIFrame* childList = nsnull;
    if (processChildren) {
      rv = ProcessChildren(aPresContext, aNewFrame, aContent, childList);
    }

    // Initialize the frame
    aNewFrame->Init(*aPresContext, childList);
  }

  return rv;
}

nsresult
HTMLStyleSheetImpl::ConstructFrameByDisplayType(nsIPresContext*  aPresContext,
                                                nsIContent*      aContent,
                                                nsIFrame*        aParentFrame,
                                                nsIStyleContext* aStyleContext,
                                                nsIFrame*&       aNewFrame)
{
  const nsStyleDisplay* styleDisplay;
  PRBool                processChildren = PR_FALSE;  // whether we should process child content
  nsresult              rv = NS_OK;

  // Initialize OUT parameter
  aNewFrame = nsnull;

  // Get the 'display' type to choose which kind of frame to create
  styleDisplay = (const nsStyleDisplay*)aStyleContext->GetStyleData(eStyleStruct_Display);

  switch (styleDisplay->mDisplay) {
  case NS_STYLE_DISPLAY_BLOCK:
  case NS_STYLE_DISPLAY_LIST_ITEM:
    rv = NS_NewBlockFrame(aContent, aParentFrame, aNewFrame);
    processChildren = PR_TRUE;
    break;

  case NS_STYLE_DISPLAY_INLINE:
    rv = NS_NewInlineFrame(aContent, aParentFrame, aNewFrame);
    processChildren = PR_TRUE;
    break;

  case NS_STYLE_DISPLAY_TABLE:
    rv = ConstructTableFrame(aPresContext, aContent, aParentFrame,
                             aStyleContext, aNewFrame);
    // Note: table construction function takes care of setting the style context,
    // processing children, and calling Init()
    return rv;

  case NS_STYLE_DISPLAY_TABLE_ROW_GROUP:
  case NS_STYLE_DISPLAY_TABLE_HEADER_GROUP:
  case NS_STYLE_DISPLAY_TABLE_FOOTER_GROUP:
    // XXX We should check for being inside of a table. If there's a missing
    // table then create an anonynmous table frame
    rv = NS_NewTableRowGroupFrame(aContent, aParentFrame, aNewFrame);
    processChildren = PR_TRUE;
    break;

  case NS_STYLE_DISPLAY_TABLE_COLUMN:
    // XXX We should check for being inside of a table column group...
    rv = NS_NewTableColFrame(aContent, aParentFrame, aNewFrame);
    processChildren = PR_TRUE;
    break;

  case NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP:
    // XXX We should check for being inside of a table...
    rv = NS_NewTableColGroupFrame(aContent, aParentFrame, aNewFrame);
    processChildren = PR_TRUE;
    break;

  case NS_STYLE_DISPLAY_TABLE_ROW:
    // XXX We should check for being inside of a table row group...
    rv = NS_NewTableRowFrame(aContent, aParentFrame, aNewFrame);
    processChildren = PR_TRUE;
    break;

  case NS_STYLE_DISPLAY_TABLE_CELL:
    // XXX We should check for being inside of a table row frame...
    rv = NS_NewTableCellFrame(aContent, aParentFrame, aNewFrame);
    processChildren = PR_TRUE;
    break;

  default:
    // Don't create any frame for content that's not displayed...
    break;
  }

  // If we succeeded in creating a frame then set its style context,
  // process children (if requested), and initialize the frame
  if (NS_SUCCEEDED(rv) && (nsnull != aNewFrame)) {
    aNewFrame->SetStyleContext(aPresContext, aStyleContext);

    // Process the child content if requested
    nsIFrame* childList = nsnull;
    if (processChildren) {
      rv = ProcessChildren(aPresContext, aNewFrame, aContent, childList);
    }

    // Initialize the frame
    aNewFrame->Init(*aPresContext, childList);
  }

  return rv;
}

NS_IMETHODIMP
HTMLStyleSheetImpl::ConstructFrame(nsIPresContext*  aPresContext,
                                   nsIContent*      aContent,
                                   nsIFrame*        aParentFrame,
                                   nsIFrame*&       aFrameSubTree)
{
  nsresult  rv;

  // Get the tag
  nsIAtom*  tag;
  aContent->GetTag(tag);

  // Resolve the style context.
  // XXX Cheesy hack for text
  nsIStyleContext* styleContext;
  if (nsnull == tag) {
    styleContext = aPresContext->ResolvePseudoStyleContextFor(nsHTMLAtoms::text, aParentFrame);
  } else {
    styleContext = aPresContext->ResolveStyleContextFor(aContent, aParentFrame);
  }

  // Create a frame.
  if (nsnull == aParentFrame) {
    // Construct the root frame object
    rv = ConstructRootFrame(aPresContext, aContent, styleContext, aFrameSubTree);

  } else {
    // Handle specific frame types
    rv = ConstructFrameByTag(aPresContext, aContent, aParentFrame,
                             tag, styleContext, aFrameSubTree);

    if (NS_SUCCEEDED(rv) && (nsnull == aFrameSubTree)) {
      // When there is no explicit frame to create, assume it's a container
      // and let display style dictate the rest
      rv = ConstructFrameByDisplayType(aPresContext, aContent, aParentFrame,
                                       styleContext, aFrameSubTree);
    }
  }
  
  NS_RELEASE(styleContext);
  NS_IF_RELEASE(tag);
  return rv;
}

NS_IMETHODIMP
HTMLStyleSheetImpl::ContentAppended(nsIPresContext* aPresContext,
                                    nsIDocument*    aDocument,
                                    nsIContent*     aContainer,
                                    PRInt32         aNewIndexInContainer)
{
  nsIPresShell* shell = aPresContext->GetShell();
  nsIFrame* parentFrame = shell->FindFrameWithContent(aContainer);

#ifdef NS_DEBUG
  if (nsnull == parentFrame) {
    NS_WARNING("Ignoring content-appended notification with no matching frame...");
  }
#endif
  if (nsnull != parentFrame) {
    // Get the parent frame's last-in-flow
    nsIFrame* nextInFlow = parentFrame;
    while (nsnull != nextInFlow) {
      parentFrame->GetNextInFlow(nextInFlow);
      if (nsnull != nextInFlow) {
        parentFrame = nextInFlow;
      }
    }

    // Create some new frames
    PRInt32   count;
    nsIFrame* lastChildFrame = nsnull;
    nsIFrame* firstAppendedFrame = nsnull;

    aContainer->ChildCount(count);

    for (PRInt32 i = aNewIndexInContainer; i < count; i++) {
      nsIContent* child;
      nsIFrame*   frame;

      aContainer->ChildAt(i, child);
      ConstructFrame(aPresContext, child, parentFrame, frame);

      // Link the frame into the child frame list
      if (nsnull == lastChildFrame) {
        firstAppendedFrame = frame;
      } else {
        lastChildFrame->SetNextSibling(frame);
      }

      // XXX We should probably mark the frame as being dirty: that way the
      // parent frame can easily identify the newly added frames. Either that
      // or pass along in count in which case they must be contiguus...
      lastChildFrame = frame;
      NS_RELEASE(child);
    }

    // Notify the parent frame with a reflow command, passing it the list of
    // new frames.
    nsIReflowCommand* reflowCmd;
    nsresult          result;

    result = NS_NewHTMLReflowCommand(&reflowCmd, parentFrame,
                                     nsIReflowCommand::FrameAppended,
                                     firstAppendedFrame);
    if (NS_SUCCEEDED(result)) {
      shell->AppendReflowCommand(reflowCmd);
      NS_RELEASE(reflowCmd);
    }
  }

  NS_RELEASE(shell);
  return NS_OK;
}

static nsIFrame*
FindPreviousSibling(nsIPresShell* aPresShell,
                    nsIContent*   aContainer,
                    PRInt32       aIndexInContainer)
{
  nsIFrame* prevSibling = nsnull;

  // Note: not all content objects are associated with a frame so
  // keep looking until we find a previous frame
  for (PRInt32 index = aIndexInContainer - 1; index > 0; index--) {
    nsIContent* precedingContent;
    aContainer->ChildAt(index, precedingContent);
    prevSibling = aPresShell->FindFrameWithContent(precedingContent);
    NS_RELEASE(precedingContent);

    if (nsnull != prevSibling) {
      // The frame may have a next-in-flow. Get the last-in-flow
      nsIFrame* nextInFlow;
      do {
        prevSibling->GetNextInFlow(nextInFlow);
        if (nsnull != nextInFlow) {
          prevSibling = nextInFlow;
        }
      } while (nsnull != nextInFlow);

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

  // Note: not all content objects are associated with a frame so
  // keep looking until we find a next frame
  PRInt32 count;
  aContainer->ChildCount(count);
  for (PRInt32 index = aIndexInContainer + 1; index < count; index++) {
    nsIContent* nextContent;
    aContainer->ChildAt(index, nextContent);
    nextSibling = aPresShell->FindFrameWithContent(nextContent);
    NS_RELEASE(nextContent);

    if (nsnull != nextSibling) {
      // The frame may have a next-in-flow. Get the last-in-flow
      nsIFrame* nextInFlow;
      do {
        nextSibling->GetNextInFlow(nextInFlow);
        if (nsnull != nextInFlow) {
          nextSibling = nextInFlow;
        }
      } while (nsnull != nextInFlow);

      break;
    }
  }

  return nextSibling;
}

NS_IMETHODIMP
HTMLStyleSheetImpl::ContentInserted(nsIPresContext* aPresContext,
                                    nsIDocument*    aDocument,
                                    nsIContent*     aContainer,
                                    nsIContent*     aChild,
                                    PRInt32         aIndexInContainer)
{
  nsIPresShell* shell = aPresContext->GetShell();

  // Find the frame that precedes the insertion point.
  nsIFrame* prevSibling = FindPreviousSibling(shell, aContainer, aIndexInContainer);
  nsIFrame* nextSibling = nsnull;
  PRBool    isAppend = PR_FALSE;

  // If there is no previous sibling, then find the frame that follows
  if (nsnull == prevSibling) {
    nextSibling = FindNextSibling(shell, aContainer, aIndexInContainer);
  }

  // Get the geometric parent.
  nsIFrame* parentFrame;
  if ((nsnull == prevSibling) && (nsnull == nextSibling)) {
    // No previous or next sibling so treat this like an appended frame.
    // XXX This won't always be true if there's auto-generated before/after
    // content
    isAppend = PR_TRUE;
    parentFrame = shell->FindFrameWithContent(aContainer);

  } else {
    // Use the prev sibling if we have it; otherwise use the next sibling.
    // Note that we use the content parent, and not the geometric parent,
    // in case the frame has been moved out of the flow...
    if (nsnull != prevSibling) {
      prevSibling->GetContentParent(parentFrame);
    } else {
      nextSibling->GetContentParent(parentFrame);
    }
  }

  // Construct a new frame
  nsresult  rv = NS_OK;
  if (nsnull != parentFrame) {
    nsIFrame* newFrame;
    rv = ConstructFrame(aPresContext, aChild, parentFrame, newFrame);

    if (NS_SUCCEEDED(rv) && (nsnull != newFrame)) {
      nsIReflowCommand* reflowCmd = nsnull;

      // Notify the parent frame with a reflow command.
      if (isAppend) {
        // Generate a FrameAppended reflow command
        rv = NS_NewHTMLReflowCommand(&reflowCmd, parentFrame,
                                     nsIReflowCommand::FrameAppended, newFrame);
      } else {
        // Generate a FrameInserted reflow command
        rv = NS_NewHTMLReflowCommand(&reflowCmd, parentFrame, newFrame, prevSibling);
      }

      if (NS_SUCCEEDED(rv)) {
        shell->AppendReflowCommand(reflowCmd);
        NS_RELEASE(reflowCmd);
      }
    }
  }

  NS_RELEASE(shell);
  return rv;
}

void HTMLStyleSheetImpl::List(FILE* out, PRInt32 aIndent) const
{
  nsAutoString buffer;

  // Indent
  for (PRInt32 index = aIndent; --index >= 0; ) fputs("  ", out);

  fputs("HTML Style Sheet: ", out);
  mURL->ToString(buffer);
  fputs(buffer, out);
  fputs("\n", out);

}

NS_HTML nsresult
  NS_NewHTMLStyleSheet(nsIHTMLStyleSheet** aInstancePtrResult, nsIURL* aURL)
{
  if (aInstancePtrResult == nsnull) {
    return NS_ERROR_NULL_POINTER;
  }

  HTMLStyleSheetImpl  *it = new HTMLStyleSheetImpl(aURL);

  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return it->QueryInterface(kIHTMLStyleSheetIID, (void **) aInstancePtrResult);
}
