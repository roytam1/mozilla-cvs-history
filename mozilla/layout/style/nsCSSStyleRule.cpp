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
 *   David Hyatt <hyatt@netscape.com>
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
#include "nsCSSRule.h"
#include "nsICSSStyleRule.h"
#include "nsICSSGroupRule.h"
#include "nsCSSDeclaration.h"
#include "nsICSSStyleSheet.h"
#include "nsICSSParser.h"
#include "nsICSSLoader.h"
#include "nsIHTMLContentContainer.h"
#include "nsIURL.h"
#include "nsIPresContext.h"
#include "nsIDocument.h"
#include "nsIDeviceContext.h"
#include "nsIArena.h"
#include "nsIAtom.h"
#include "nsCRT.h"
#include "nsString.h"
#include "nsStyleConsts.h"
#include "nsHTMLAtoms.h"
#include "nsUnitConversion.h"
#include "nsIFontMetrics.h"
#include "nsIDOMCSSStyleSheet.h"
#include "nsIDOMCSSRule.h"
#include "nsIDOMCSSStyleRule.h"
#include "nsIDOMCSSStyleDeclaration.h"
#include "nsDOMCSSDeclaration.h"
#include "nsINameSpaceManager.h"
#include "nsINameSpace.h"
#include "nsILookAndFeel.h"
#include "nsRuleNode.h"
#include "nsUnicharUtils.h"

#include "nsIStyleSet.h"

#include "nsContentUtils.h"
#include "nsContentErrors.h"

// -- nsCSSSelector -------------------------------

#define NS_IF_COPY(dest,source,type)  \
  if (nsnull != source)  dest = new type(*(source))

#define NS_IF_DELETE(ptr)   \
  if (nsnull != ptr) { delete ptr; ptr = nsnull; }

#define NS_IF_NEGATED_START(bool,str)  \
  if (bool) { str.Append(NS_LITERAL_STRING(":not(")); }

#define NS_IF_NEGATED_END(bool,str)  \
  if (bool) { str.Append(PRUnichar(')')); }

MOZ_DECL_CTOR_COUNTER(nsAtomList)

nsAtomList::nsAtomList(nsIAtom* aAtom)
  : mAtom(aAtom),
    mNext(nsnull)
{
  MOZ_COUNT_CTOR(nsAtomList);
}

nsAtomList::nsAtomList(const nsString& aAtomValue)
  : mAtom(nsnull),
    mNext(nsnull)
{
  MOZ_COUNT_CTOR(nsAtomList);
  mAtom = do_GetAtom(aAtomValue);
}

nsAtomList::nsAtomList(const nsAtomList& aCopy)
  : mAtom(aCopy.mAtom),
    mNext(nsnull)
{
  MOZ_COUNT_CTOR(nsAtomList);
  NS_IF_COPY(mNext, aCopy.mNext, nsAtomList);
}

nsAtomList::~nsAtomList(void)
{
  MOZ_COUNT_DTOR(nsAtomList);
  NS_IF_DELETE(mNext);
}

PRBool nsAtomList::Equals(const nsAtomList* aOther) const
{
  if (this == aOther) {
    return PR_TRUE;
  }
  if (nsnull != aOther) {
    if (mAtom == aOther->mAtom) {
      if (nsnull != mNext) {
        return mNext->Equals(aOther->mNext);
      }
      return PRBool(nsnull == aOther->mNext);
    }
  }
  return PR_FALSE;
}

MOZ_DECL_CTOR_COUNTER(nsAtomStringList)

nsAtomStringList::nsAtomStringList(nsIAtom* aAtom, const PRUnichar* aString)
  : mAtom(aAtom),
    mString(nsnull),
    mNext(nsnull)
{
  MOZ_COUNT_CTOR(nsAtomStringList);
  if (aString)
    mString = nsCRT::strdup(aString);
}

nsAtomStringList::nsAtomStringList(const nsString& aAtomValue,
                                   const PRUnichar* aString)
  : mAtom(nsnull),
    mString(nsnull),
    mNext(nsnull)
{
  MOZ_COUNT_CTOR(nsAtomStringList);
  mAtom = do_GetAtom(aAtomValue);
  if (aString)
    mString = nsCRT::strdup(aString);
}

nsAtomStringList::nsAtomStringList(const nsAtomStringList& aCopy)
  : mAtom(aCopy.mAtom),
    mString(nsnull),
    mNext(nsnull)
{
  MOZ_COUNT_CTOR(nsAtomStringList);
  if (aCopy.mString)
    mString = nsCRT::strdup(aCopy.mString);
  NS_IF_COPY(mNext, aCopy.mNext, nsAtomStringList);
}

nsAtomStringList::~nsAtomStringList(void)
{
  MOZ_COUNT_DTOR(nsAtomStringList);
  if (mString)
    nsCRT::free(mString);
  NS_IF_DELETE(mNext);
}

PRBool nsAtomStringList::Equals(const nsAtomStringList* aOther) const
{
  return (this == aOther) ||
         (aOther &&
          mAtom == aOther->mAtom &&
          !mString == !aOther->mString &&
          !mNext == !aOther->mNext &&
          (!mNext || mNext->Equals(aOther->mNext)) &&
          // Check strings last, since it's the slowest check.
          (!mString || nsDependentString(mString).Equals(
                                        nsDependentString(aOther->mString),
                                        nsCaseInsensitiveStringComparator())));
}

MOZ_DECL_CTOR_COUNTER(nsAttrSelector)

nsAttrSelector::nsAttrSelector(PRInt32 aNameSpace, const nsString& aAttr)
  : mNameSpace(aNameSpace),
    mAttr(nsnull),
    mFunction(NS_ATTR_FUNC_SET),
    mCaseSensitive(1),
    mValue(),
    mNext(nsnull)
{
  MOZ_COUNT_CTOR(nsAttrSelector);

  mAttr = NS_NewAtom(aAttr);
}

nsAttrSelector::nsAttrSelector(PRInt32 aNameSpace, const nsString& aAttr, PRUint8 aFunction, 
                               const nsString& aValue, PRBool aCaseSensitive)
  : mNameSpace(aNameSpace),
    mAttr(nsnull),
    mFunction(aFunction),
    mCaseSensitive(aCaseSensitive),
    mValue(aValue),
    mNext(nsnull)
{
  MOZ_COUNT_CTOR(nsAttrSelector);

  mAttr = NS_NewAtom(aAttr);
}

nsAttrSelector::nsAttrSelector(const nsAttrSelector& aCopy)
  : mNameSpace(aCopy.mNameSpace),
    mAttr(aCopy.mAttr),
    mFunction(aCopy.mFunction),
    mCaseSensitive(aCopy.mCaseSensitive),
    mValue(aCopy.mValue),
    mNext(nsnull)
{
  MOZ_COUNT_CTOR(nsAttrSelector);

  NS_IF_ADDREF(mAttr);
  NS_IF_COPY(mNext, aCopy.mNext, nsAttrSelector);
}

nsAttrSelector::~nsAttrSelector(void)
{
  MOZ_COUNT_DTOR(nsAttrSelector);

  NS_IF_RELEASE(mAttr);
  NS_IF_DELETE(mNext);
}

PRBool nsAttrSelector::Equals(const nsAttrSelector* aOther) const
{
  if (this == aOther) {
    return PR_TRUE;
  }
  if (nsnull != aOther) {
    if ((mNameSpace == aOther->mNameSpace) &&
        (mAttr == aOther->mAttr) && 
        (mFunction == aOther->mFunction) && 
        (mCaseSensitive == aOther->mCaseSensitive) &&
        mValue.Equals(aOther->mValue)) {
      if (nsnull != mNext) {
        return mNext->Equals(aOther->mNext);
      }
      return PRBool(nsnull == aOther->mNext);
    }
  }
  return PR_FALSE;
}

MOZ_DECL_CTOR_COUNTER(nsCSSSelector)

nsCSSSelector::nsCSSSelector(void)
  : mNameSpace(kNameSpaceID_Unknown), mTag(nsnull), 
    mIDList(nsnull), 
    mClassList(nsnull), 
    mPseudoClassList(nsnull),
    mAttrList(nsnull), 
    mOperator(0),
    mNegations(nsnull),
    mNext(nsnull)
{
  MOZ_COUNT_CTOR(nsCSSSelector);
}

nsCSSSelector::nsCSSSelector(const nsCSSSelector& aCopy) 
  : mNameSpace(aCopy.mNameSpace), mTag(aCopy.mTag), 
    mIDList(nsnull), 
    mClassList(nsnull), 
    mPseudoClassList(nsnull),
    mAttrList(nsnull), 
    mOperator(aCopy.mOperator),
    mNegations(nsnull),
    mNext(nsnull)
{
  MOZ_COUNT_CTOR(nsCSSSelector);
  NS_IF_COPY(mIDList, aCopy.mIDList, nsAtomList);
  NS_IF_COPY(mClassList, aCopy.mClassList, nsAtomList);
  NS_IF_COPY(mPseudoClassList, aCopy.mPseudoClassList, nsAtomStringList);
  NS_IF_COPY(mAttrList, aCopy.mAttrList, nsAttrSelector);
  NS_IF_COPY(mNegations, aCopy.mNegations, nsCSSSelector);
}

nsCSSSelector::~nsCSSSelector(void)  
{
  MOZ_COUNT_DTOR(nsCSSSelector);
  Reset();
}

nsCSSSelector& nsCSSSelector::operator=(const nsCSSSelector& aCopy)
{
  NS_IF_DELETE(mIDList);
  NS_IF_DELETE(mClassList);
  NS_IF_DELETE(mPseudoClassList);
  NS_IF_DELETE(mAttrList);
  NS_IF_DELETE(mNegations);
  
  mNameSpace    = aCopy.mNameSpace;
  mTag          = aCopy.mTag;
  NS_IF_COPY(mIDList, aCopy.mIDList, nsAtomList);
  NS_IF_COPY(mClassList, aCopy.mClassList, nsAtomList);
  NS_IF_COPY(mPseudoClassList, aCopy.mPseudoClassList, nsAtomStringList);
  NS_IF_COPY(mAttrList, aCopy.mAttrList, nsAttrSelector);
  mOperator     = aCopy.mOperator;
  NS_IF_COPY(mNegations, aCopy.mNegations, nsCSSSelector);

  return *this;
}

PRBool nsCSSSelector::Equals(const nsCSSSelector* aOther) const
{
  if (this == aOther) {
    return PR_TRUE;
  }
  if (nsnull != aOther) {
    if ((aOther->mNameSpace == mNameSpace) && 
        (aOther->mTag == mTag) && 
        (aOther->mOperator == mOperator)) {
      if (nsnull != mIDList) {
        if (PR_FALSE == mIDList->Equals(aOther->mIDList)) {
          return PR_FALSE;
        }
      }
      else {
        if (nsnull != aOther->mIDList) {
          return PR_FALSE;
        }
      }
      if (nsnull != mClassList) {
        if (PR_FALSE == mClassList->Equals(aOther->mClassList)) {
          return PR_FALSE;
        }
      }
      else {
        if (nsnull != aOther->mClassList) {
          return PR_FALSE;
        }
      }
      if (nsnull != mPseudoClassList) {
        if (PR_FALSE == mPseudoClassList->Equals(aOther->mPseudoClassList)) {
          return PR_FALSE;
        }
      }
      else {
        if (nsnull != aOther->mPseudoClassList) {
          return PR_FALSE;
        }
      }
      if (nsnull != mAttrList) {
        if (PR_FALSE == mAttrList->Equals(aOther->mAttrList)) {
          return PR_FALSE;
        }
      }
      else {
        if (nsnull != aOther->mAttrList) {
          return PR_FALSE;
        }
      }
      if (nsnull != mNegations) {
        if (PR_FALSE == mNegations->Equals(aOther->mNegations)) {
          return PR_FALSE;
        }
      }
      return PR_TRUE;
    }
  }
  return PR_FALSE;
}


void nsCSSSelector::Reset(void)
{
  mNameSpace = kNameSpaceID_Unknown;
  mTag = nsnull;
  NS_IF_DELETE(mIDList);
  NS_IF_DELETE(mClassList);
  NS_IF_DELETE(mPseudoClassList);
  NS_IF_DELETE(mAttrList);
  NS_IF_DELETE(mNegations);
  mOperator = PRUnichar(0);
}

void nsCSSSelector::SetNameSpace(PRInt32 aNameSpace)
{
  mNameSpace = aNameSpace;
}

void nsCSSSelector::SetTag(const nsString& aTag)
{
  if (aTag.IsEmpty())
    mTag = nsnull;
  else
    mTag = do_GetAtom(aTag);
}

void nsCSSSelector::AddID(const nsString& aID)
{
  if (!aID.IsEmpty()) {
    nsAtomList** list = &mIDList;
    while (nsnull != *list) {
      list = &((*list)->mNext);
    }
    *list = new nsAtomList(aID);
  }
}

void nsCSSSelector::AddClass(const nsString& aClass)
{
  if (!aClass.IsEmpty()) {
    nsAtomList** list = &mClassList;
    while (nsnull != *list) {
      list = &((*list)->mNext);
    }
    *list = new nsAtomList(aClass);
  }
}

void nsCSSSelector::AddPseudoClass(const nsString& aPseudoClass,
                                   const PRUnichar* aString)
{
  if (!aPseudoClass.IsEmpty()) {
    nsAtomStringList** list = &mPseudoClassList;
    while (nsnull != *list) {
      list = &((*list)->mNext);
    }
    *list = new nsAtomStringList(aPseudoClass, aString);
  }
}

void nsCSSSelector::AddPseudoClass(nsIAtom* aPseudoClass,
                                   const PRUnichar* aString)
{
  if (nsnull != aPseudoClass) {
    nsAtomStringList** list = &mPseudoClassList;
    while (nsnull != *list) {
      list = &((*list)->mNext);
    }
    *list = new nsAtomStringList(aPseudoClass, aString);
  }
}

void nsCSSSelector::AddAttribute(PRInt32 aNameSpace, const nsString& aAttr)
{
  if (!aAttr.IsEmpty()) {
    nsAttrSelector** list = &mAttrList;
    while (nsnull != *list) {
      list = &((*list)->mNext);
    }
    *list = new nsAttrSelector(aNameSpace, aAttr);
  }
}

void nsCSSSelector::AddAttribute(PRInt32 aNameSpace, const nsString& aAttr, PRUint8 aFunc, 
                                 const nsString& aValue, PRBool aCaseSensitive)
{
  if (!aAttr.IsEmpty()) {
    nsAttrSelector** list = &mAttrList;
    while (nsnull != *list) {
      list = &((*list)->mNext);
    }
    *list = new nsAttrSelector(aNameSpace, aAttr, aFunc, aValue, aCaseSensitive);
  }
}

void nsCSSSelector::SetOperator(PRUnichar aOperator)
{
  mOperator = aOperator;
}

PRInt32 nsCSSSelector::CalcWeight(void) const
{
  PRInt32 weight = 0;

  if (nsnull != mTag) {
    weight += 0x000001;
  }
  nsAtomList* list = mIDList;
  while (nsnull != list) {
    weight += 0x010000;
    list = list->mNext;
  }
  list = mClassList;
  while (nsnull != list) {
    weight += 0x000100;
    list = list->mNext;
  }
  nsAtomStringList *plist = mPseudoClassList;
  while (nsnull != plist) {
    weight += 0x000100;
    plist = plist->mNext;
  }
  nsAttrSelector* attr = mAttrList;
  while (nsnull != attr) {
    weight += 0x000100;
    attr = attr->mNext;
  }
  if (nsnull != mNegations) {
    weight += mNegations->CalcWeight();
  }
  return weight;
}

// pseudo-elements are stored in the selectors' chain using fictional elements;
// these fictional elements have mTag starting with a colon
static PRBool IsPseudoElement(nsIAtom* aAtom)
{
  if (aAtom) {
    const char* str;
    aAtom->GetUTF8String(&str);
    return str && (*str == ':');
  }

  return PR_FALSE;
}

void nsCSSSelector::AppendNegationToString(nsAString& aString)
{
  aString.Append(NS_LITERAL_STRING(":not("));
}

//
// Builds the textual representation of a selector. Called by DOM 2 CSS 
// StyleRule:selectorText
//
nsresult nsCSSSelector::ToString( nsAString& aString, nsICSSStyleSheet* aSheet, PRBool aIsPseudoElem,
                                  PRInt8 aNegatedIndex) const
{
  nsAutoString temp;
  PRBool aIsNegated = PRBool(0 < aNegatedIndex);

  // selectors are linked from right-to-left, so the next selector in the linked list
  // actually precedes this one in the resulting string
  if (mNext) {
    mNext->ToString(aString, aSheet, IsPseudoElement(mTag), PR_FALSE);
    if (!aIsNegated && !IsPseudoElement(mTag)) {
      // don't add a leading whitespace if we have a pseudo-element
      // or a negated simple selector
      aString.Append(PRUnichar(' '));
    }
  }
  if (1 < aNegatedIndex) {
    // the first mNegations does not contain a negated type element selector
    // or a negated universal selector
    NS_IF_NEGATED_START(aIsNegated, aString)
  }

  // append the namespace prefix
  if (mNameSpace > 0) {
    nsCOMPtr<nsINameSpace> sheetNS;
    aSheet->GetNameSpace(*getter_AddRefs(sheetNS));
    nsCOMPtr<nsIAtom> prefixAtom;
    // will return null if namespace was the default
    sheetNS->FindNameSpacePrefix(mNameSpace, *getter_AddRefs(prefixAtom));
    if (prefixAtom) {
      nsAutoString prefix;
      prefixAtom->ToString(prefix);
      aString.Append(prefix);
      aString.Append(PRUnichar('|'));
    }
  }
  // smells like a universal selector
  if (!mTag && !mIDList && !mClassList) {
    if (1 != aNegatedIndex) {
      aString.Append(PRUnichar('*'));
    }
    if (1 < aNegatedIndex) {
      NS_IF_NEGATED_END(aIsNegated, aString)
    }
  } else {
    // Append the tag name, if there is one
    if (mTag) {
      nsAutoString prefix;
      mTag->ToString(prefix);
      aString.Append(prefix);
      NS_IF_NEGATED_END(aIsNegated, aString)
    }
    // Append the id, if there is one
    if (mIDList) {
      nsAtomList* list = mIDList;
      while (list != nsnull) {
        list->mAtom->ToString(temp);
        NS_IF_NEGATED_START(aIsNegated, aString)
        aString.Append(PRUnichar('#'));
        aString.Append(temp);
        NS_IF_NEGATED_END(aIsNegated, aString)
        list = list->mNext;
      }
    }
    // Append each class in the linked list
    if (mClassList) {
      nsAtomList* list = mClassList;
      while (list != nsnull) {
        list->mAtom->ToString(temp);
        NS_IF_NEGATED_START(aIsNegated, aString)
        aString.Append(PRUnichar('.'));
        aString.Append(temp);
        NS_IF_NEGATED_END(aIsNegated, aString)
        list = list->mNext;
      }
    }
  }

  // Append each attribute selector in the linked list
  if (mAttrList) {
    nsAttrSelector* list = mAttrList;
    while (list != nsnull) {
      NS_IF_NEGATED_START(aIsNegated, aString)
      aString.Append(PRUnichar('['));
      // Append the namespace prefix
      if (list->mNameSpace > 0) {
        nsCOMPtr<nsINameSpace> sheetNS;
        aSheet->GetNameSpace(*getter_AddRefs(sheetNS));
        nsCOMPtr<nsIAtom> prefixAtom;
        // will return null if namespace was the default
        sheetNS->FindNameSpacePrefix(list->mNameSpace, *getter_AddRefs(prefixAtom));
        if (prefixAtom) { 
          nsAutoString prefix;
          prefixAtom->ToString(prefix);
          aString.Append(prefix);
          aString.Append(PRUnichar('|'));
        }
      }
      // Append the attribute name
      list->mAttr->ToString(temp);
      aString.Append(temp);
      // Append the function
      if (list->mFunction == NS_ATTR_FUNC_EQUALS) {
        aString.Append(PRUnichar('='));
      } else if (list->mFunction == NS_ATTR_FUNC_INCLUDES) {
        aString.Append(PRUnichar('~'));
        aString.Append(PRUnichar('='));
      } else if (list->mFunction == NS_ATTR_FUNC_DASHMATCH) {
        aString.Append(PRUnichar('|'));
        aString.Append(PRUnichar('='));
      } else if (list->mFunction == NS_ATTR_FUNC_BEGINSMATCH) {
        aString.Append(PRUnichar('^'));
        aString.Append(PRUnichar('='));
      } else if (list->mFunction == NS_ATTR_FUNC_ENDSMATCH) {
        aString.Append(PRUnichar('$'));
        aString.Append(PRUnichar('='));
      } else if (list->mFunction == NS_ATTR_FUNC_CONTAINSMATCH) {
        aString.Append(PRUnichar('*'));
        aString.Append(PRUnichar('='));
      }
      // Append the value
      aString.Append(list->mValue);
      aString.Append(PRUnichar(']'));
      NS_IF_NEGATED_END(aIsNegated, aString)
      list = list->mNext;
    }
  }

  // Append each pseudo-class in the linked list
  if (mPseudoClassList) {
    nsAtomStringList* list = mPseudoClassList;
    while (list != nsnull) {
      list->mAtom->ToString(temp);
      NS_IF_NEGATED_START(aIsNegated, aString)
      aString.Append(temp);
      if (nsnull != list->mString) {
        aString.Append(PRUnichar('('));
        aString.Append(list->mString);
        aString.Append(PRUnichar(')'));
      }
      NS_IF_NEGATED_END(aIsNegated, aString)
      list = list->mNext;
    }
  }

  if (mNegations) {
    // chain all the negated selectors
    mNegations->ToString(aString, aSheet, PR_FALSE, aNegatedIndex + 1);
  }

  // Append the operator only if the selector is not negated and is not
  // a pseudo-element
  if (!aIsNegated && mOperator && !aIsPseudoElem) {
    aString.Append(PRUnichar(' '));
    aString.Append(mOperator);
  }
  return NS_OK;
}


// -- CSSImportantRule -------------------------------

class CSSStyleRuleImpl;

class CSSImportantRule : public nsIStyleRule {
public:
  CSSImportantRule(nsICSSStyleSheet* aSheet, nsCSSDeclaration* aDeclaration);

  NS_DECL_ISUPPORTS

  NS_IMETHOD GetStyleSheet(nsIStyleSheet*& aSheet) const;

  // The new mapping function.
  NS_IMETHOD MapRuleInfoInto(nsRuleData* aRuleData);

#ifdef DEBUG
  NS_IMETHOD List(FILE* out = stdout, PRInt32 aIndent = 0) const;
#endif

protected:
  virtual ~CSSImportantRule(void);

  nsCSSDeclaration*  mDeclaration;
  nsICSSStyleSheet*   mSheet;

friend class CSSStyleRuleImpl;
};

CSSImportantRule::CSSImportantRule(nsICSSStyleSheet* aSheet, nsCSSDeclaration* aDeclaration)
  : mDeclaration(aDeclaration),
    mSheet(aSheet)
{
}

CSSImportantRule::~CSSImportantRule(void)
{
  mDeclaration = nsnull;
}

NS_IMPL_ISUPPORTS1(CSSImportantRule, nsIStyleRule)

NS_IMETHODIMP
CSSImportantRule::GetStyleSheet(nsIStyleSheet*& aSheet) const
{
  NS_IF_ADDREF(mSheet);
  aSheet = mSheet;
  return NS_OK;
}

NS_IMETHODIMP
CSSImportantRule::MapRuleInfoInto(nsRuleData* aRuleData)
{
  return mDeclaration->MapImportantRuleInfoInto(aRuleData);
}

#ifdef DEBUG
NS_IMETHODIMP
CSSImportantRule::List(FILE* out, PRInt32 aIndent) const
{
  // Indent
  for (PRInt32 index = aIndent; --index >= 0; ) fputs("  ", out);

  fputs("! Important rule ", out);
  if (nsnull != mDeclaration) {
    mDeclaration->List(out);
  }
  else {
    fputs("{ null declaration }", out);
  }
  fputs("\n", out);

  return NS_OK;
}
#endif

// -- nsDOMStyleRuleDeclaration -------------------------------

class DOMCSSDeclarationImpl : public nsDOMCSSDeclaration
{
public:
  DOMCSSDeclarationImpl(nsICSSStyleRule *aRule);
  ~DOMCSSDeclarationImpl(void);

  NS_IMETHOD RemoveProperty(const nsAString& aPropertyName, 
                            nsAString& aReturn);

  virtual void DropReference(void);
  virtual nsresult GetCSSDeclaration(nsCSSDeclaration **aDecl,
                                     PRBool aAllocate);
  virtual nsresult GetCSSParsingEnvironment(nsICSSStyleRule* aRule,
                                            nsICSSStyleSheet** aSheet,
                                            nsIDocument** aDocument,
                                            nsIURI** aURI,
                                            nsICSSLoader** aCSSLoader,
                                            nsICSSParser** aCSSParser);
  virtual nsresult ParsePropertyValue(const nsAString& aPropName,
                                      const nsAString& aPropValue);
  virtual nsresult ParseDeclaration(const nsAString& aDecl,
                                    PRBool aParseOnlyOneDecl,
                                    PRBool aClearOldDecl);
  virtual nsresult GetParent(nsISupports **aParent);

protected:
  nsICSSStyleRule *mRule;
};

MOZ_DECL_CTOR_COUNTER(DOMCSSDeclarationImpl)

DOMCSSDeclarationImpl::DOMCSSDeclarationImpl(nsICSSStyleRule *aRule)
{
  MOZ_COUNT_CTOR(DOMCSSDeclarationImpl);

  // This reference is not reference-counted. The rule
  // object tells us when its about to go away.
  mRule = aRule;
}

DOMCSSDeclarationImpl::~DOMCSSDeclarationImpl(void)
{
  MOZ_COUNT_DTOR(DOMCSSDeclarationImpl);
}

NS_IMETHODIMP
DOMCSSDeclarationImpl::RemoveProperty(const nsAString& aPropertyName, 
                                      nsAString& aReturn)
{
  aReturn.Truncate();

  nsCSSDeclaration* decl;
  nsresult rv = GetCSSDeclaration(&decl, PR_TRUE);

  if (NS_SUCCEEDED(rv) && decl) {
    nsCOMPtr<nsICSSStyleSheet> cssSheet;
    nsCOMPtr<nsIDocument> owningDoc;
    if (mRule) {
      nsCOMPtr<nsIStyleSheet> sheet;
      mRule->GetStyleSheet(*getter_AddRefs(sheet));
      cssSheet = do_QueryInterface(sheet);
      if (sheet) {
        sheet->GetOwningDocument(*getter_AddRefs(owningDoc));
      }
    }
    if (owningDoc) {
      owningDoc->BeginUpdate();
    }
    nsCSSProperty prop = nsCSSProps::LookupProperty(aPropertyName);

    decl->GetValue(prop, aReturn);

    rv = decl->RemoveProperty(prop);

    if (NS_SUCCEEDED(rv)) {
      if (cssSheet) {
        cssSheet->SetModified(PR_TRUE);
      }
      if (owningDoc) {
        owningDoc->StyleRuleChanged(cssSheet, mRule, nsCSSProps::kHintTable[prop]);
      }
    } else {
      // If we tried to remove an invalid property or a property that wasn't 
      //  set we simply return success and an empty string
      rv = NS_OK;
    }
    if (owningDoc) {
      owningDoc->EndUpdate();
    }
  }

  return rv;
}

void 
DOMCSSDeclarationImpl::DropReference(void)
{
  mRule = nsnull;
}

nsresult
DOMCSSDeclarationImpl::GetCSSDeclaration(nsCSSDeclaration **aDecl,
                                         PRBool aAllocate)
{
  if (mRule) {
    *aDecl = mRule->GetDeclaration();
  }
  else {
    *aDecl = nsnull;
  }

  return NS_OK;
}

/*
 * This is a utility function.  It will only fail if it can't get a
 * parser.  This means it can return NS_OK without all of aSheet,
 *  aDocument, aURI, aCSSLoader being initialized
 */
nsresult
DOMCSSDeclarationImpl::GetCSSParsingEnvironment(nsICSSStyleRule* aRule,
                                                nsICSSStyleSheet** aSheet,
                                                nsIDocument** aDocument,
                                                nsIURI** aURI,
                                                nsICSSLoader** aCSSLoader,
                                                nsICSSParser** aCSSParser)
{
  // null out the out params since some of them may not get initialized below
  *aSheet = nsnull;
  *aDocument = nsnull;
  *aURI = nsnull;
  *aCSSLoader = nsnull;
  *aCSSParser = nsnull;
  nsresult result;
  nsCOMPtr<nsIStyleSheet> sheet;
  if (aRule) {
    aRule->GetStyleSheet(*getter_AddRefs(sheet));
    if (sheet) {
      CallQueryInterface(sheet, aSheet);
      sheet->GetOwningDocument(*aDocument);
      sheet->GetURL(*aURI);
    }
  }
  nsCOMPtr<nsIHTMLContentContainer> htmlContainer(do_QueryInterface(*aDocument));
  if (htmlContainer) {
    htmlContainer->GetCSSLoader(*aCSSLoader);
  }
  NS_ASSERTION(*aCSSLoader || !*aDocument, "Document with no CSS loader!");
  if (*aCSSLoader) {
    result = (*aCSSLoader)->GetParserFor(nsnull, aCSSParser);
  } else {
    result = NS_NewCSSParser(aCSSParser);
  }

  return result;
}

nsresult
DOMCSSDeclarationImpl::ParsePropertyValue(const nsAString& aPropName,
                                          const nsAString& aPropValue)
{
  nsCSSDeclaration* decl;
  nsresult result = GetCSSDeclaration(&decl, PR_TRUE);
  if (!decl) {
    return result;
  }
  nsCOMPtr<nsICSSLoader> cssLoader;
  nsCOMPtr<nsICSSParser> cssParser;
  nsCOMPtr<nsIURI> baseURI;
  nsCOMPtr<nsICSSStyleSheet> cssSheet;
  nsCOMPtr<nsIDocument> owningDoc;
  result = GetCSSParsingEnvironment(mRule,
                                    getter_AddRefs(cssSheet),
                                    getter_AddRefs(owningDoc),
                                    getter_AddRefs(baseURI),
                                    getter_AddRefs(cssLoader),
                                    getter_AddRefs(cssParser));

  if (NS_FAILED(result)) {
    return result;
  }
  
  nsChangeHint hint;
  if (owningDoc) {
    owningDoc->BeginUpdate();
  }
  result = cssParser->ParseProperty(aPropName, aPropValue, baseURI, decl, &hint);
  if (NS_SUCCEEDED(result)) {
    if (cssSheet) {
      cssSheet->SetModified(PR_TRUE);
    }
    if (owningDoc) {
      owningDoc->StyleRuleChanged(cssSheet, mRule, hint);
      owningDoc->EndUpdate();
    }
  }
  if (cssLoader) {
    cssLoader->RecycleParser(cssParser);
  }

  return result;
}

nsresult 
DOMCSSDeclarationImpl::ParseDeclaration(const nsAString& aDecl,
                                        PRBool aParseOnlyOneDecl,
                                        PRBool aClearOldDecl)
{
  nsCSSDeclaration* decl;
  nsresult result = GetCSSDeclaration(&decl, PR_TRUE);

  if (decl) {
    nsCOMPtr<nsICSSLoader> cssLoader;
    nsCOMPtr<nsICSSParser> cssParser;
    nsCOMPtr<nsIURI> baseURI;
    nsCOMPtr<nsICSSStyleSheet> cssSheet;
    nsCOMPtr<nsIDocument> owningDoc;

    result = GetCSSParsingEnvironment(mRule,
                                      getter_AddRefs(cssSheet),
                                      getter_AddRefs(owningDoc),
                                      getter_AddRefs(baseURI),
                                      getter_AddRefs(cssLoader),
                                      getter_AddRefs(cssParser));

    if (NS_SUCCEEDED(result)) {
      nsChangeHint hint;
      result = cssParser->ParseAndAppendDeclaration(aDecl, baseURI, decl,
                                                    aParseOnlyOneDecl, &hint,
                                                    aClearOldDecl);

      if (NS_SUCCEEDED(result)) {
        if (cssSheet) {
          cssSheet->SetModified(PR_TRUE);
        }
        if (owningDoc) {
          owningDoc->StyleRuleChanged(cssSheet, mRule, hint);
        }
      }
      if (cssLoader) {
        cssLoader->RecycleParser(cssParser);
      }
    }
  }

  return result;
}



nsresult 
DOMCSSDeclarationImpl::GetParent(nsISupports **aParent)
{
  NS_ENSURE_ARG_POINTER(aParent);

  if (mRule) {
    return CallQueryInterface(mRule, aParent);
  }

  *aParent = nsnull;

  return NS_OK;
}

// -- nsCSSStyleRule -------------------------------

class CSSStyleRuleImpl : public nsCSSRule,
                         public nsICSSStyleRule, 
                         public nsIDOMCSSStyleRule
{
public:
  CSSStyleRuleImpl(const nsCSSSelector& aSelector);
  CSSStyleRuleImpl(const CSSStyleRuleImpl& aCopy); 

  NS_DECL_ISUPPORTS_INHERITED

  virtual nsCSSSelector* FirstSelector(void);
  virtual void AddSelector(const nsCSSSelector& aSelector);
  virtual void DeleteSelector(nsCSSSelector* aSelector);
  virtual void SetSourceSelectorText(const nsString& aSelectorText);
  virtual void GetSourceSelectorText(nsString& aSelectorText) const;

  virtual PRUint32 GetLineNumber(void) const;
  virtual void SetLineNumber(PRUint32 aLineNumber);

  virtual nsCSSDeclaration* GetDeclaration(void) const;
  virtual void SetDeclaration(nsCSSDeclaration* aDeclaration);

  virtual PRInt32 GetWeight(void) const;
  virtual void SetWeight(PRInt32 aWeight);

  virtual already_AddRefed<nsIStyleRule> GetImportantRule(void);

  // hook for inspector
  virtual nsresult GetValue(nsCSSProperty aProperty, nsCSSValue& aValue);

  NS_IMETHOD GetStyleSheet(nsIStyleSheet*& aSheet) const;
  NS_IMETHOD SetStyleSheet(nsICSSStyleSheet* aSheet);
  
  NS_IMETHOD SetParentRule(nsICSSGroupRule* aRule);

  NS_IMETHOD GetType(PRInt32& aType) const;
  NS_IMETHOD Clone(nsICSSRule*& aClone) const;

  // The new mapping function.
  NS_IMETHOD MapRuleInfoInto(nsRuleData* aRuleData);

#ifdef DEBUG
  NS_IMETHOD List(FILE* out = stdout, PRInt32 aIndent = 0) const;
#endif

  // nsIDOMCSSRule interface
  NS_DECL_NSIDOMCSSRULE

  // nsIDOMCSSStyleRule interface
  NS_DECL_NSIDOMCSSSTYLERULE

private: 
  // These are not supported and are not implemented! 
  CSSStyleRuleImpl& operator=(const CSSStyleRuleImpl& aCopy); 

protected:
  virtual ~CSSStyleRuleImpl(void);

protected:
  nsCSSSelector           mSelector;
  nsCSSDeclaration*      mDeclaration;
  PRInt32                 mWeight;
  CSSImportantRule*       mImportantRule;
  DOMCSSDeclarationImpl*  mDOMDeclaration;                          
  PRUint32                mLineNumber;
};

CSSStyleRuleImpl::CSSStyleRuleImpl(const nsCSSSelector& aSelector)
  : nsCSSRule(),
    mSelector(aSelector), mDeclaration(nsnull), 
    mWeight(0), mImportantRule(nsnull),
    mDOMDeclaration(nsnull)
{
}

CSSStyleRuleImpl::CSSStyleRuleImpl(const CSSStyleRuleImpl& aCopy)
  : nsCSSRule(aCopy),
    mSelector(aCopy.mSelector),
    mDeclaration(nsnull),
    mWeight(aCopy.mWeight),
    mImportantRule(nsnull),
    mDOMDeclaration(nsnull)
{
  nsCSSSelector* copySel = aCopy.mSelector.mNext;
  nsCSSSelector* ourSel = &mSelector;

  while (copySel && ourSel) {
    ourSel->mNext = new nsCSSSelector(*copySel);
    ourSel = ourSel->mNext;
    copySel = copySel->mNext;
  }

  if (aCopy.mDeclaration) {
    mDeclaration = aCopy.mDeclaration->Clone();
    if (nsnull != mDeclaration) {
      mDeclaration->AddRef();
    }
  }
  // rest is constructed lazily on existing data
}


CSSStyleRuleImpl::~CSSStyleRuleImpl(void)
{
  nsCSSSelector*  next = mSelector.mNext;

  while (nsnull != next) {
    nsCSSSelector*  selector = next;
    next = selector->mNext;
    delete selector;
  }
  if (nsnull != mDeclaration) {
    mDeclaration->Release();
    mDeclaration = nsnull;
  }
  if (nsnull != mImportantRule) {
    mImportantRule->mSheet = nsnull;
    NS_RELEASE(mImportantRule);
    mImportantRule = nsnull;
  }
  if (nsnull != mDOMDeclaration) {
    mDOMDeclaration->DropReference();
  }
}

// QueryInterface implementation for CSSStyleRuleImpl
NS_INTERFACE_MAP_BEGIN(CSSStyleRuleImpl)
  NS_INTERFACE_MAP_ENTRY(nsICSSStyleRule)
  NS_INTERFACE_MAP_ENTRY(nsICSSRule)
  NS_INTERFACE_MAP_ENTRY(nsIStyleRule)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCSSRule)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCSSStyleRule)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsICSSStyleRule)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(CSSStyleRule)
NS_INTERFACE_MAP_END


NS_IMPL_ADDREF_INHERITED(CSSStyleRuleImpl, nsCSSRule);
NS_IMPL_RELEASE_INHERITED(CSSStyleRuleImpl, nsCSSRule);


nsCSSSelector* CSSStyleRuleImpl::FirstSelector(void)
{
  return &mSelector;
}

void CSSStyleRuleImpl::AddSelector(const nsCSSSelector& aSelector)
{
  nsCSSSelector*  selector = new nsCSSSelector(aSelector);
  nsCSSSelector*  last = &mSelector;

  while (nsnull != last->mNext) {
    last = last->mNext;
  }
  last->mNext = selector;
}


void CSSStyleRuleImpl::DeleteSelector(nsCSSSelector* aSelector)
{
  if (nsnull != aSelector) {
    if (&mSelector == aSelector) {  // handle first selector
      if (nsnull != mSelector.mNext) {
        nsCSSSelector* nextOne = mSelector.mNext;
        mSelector = *nextOne; // assign values
        mSelector.mNext = nextOne->mNext;
        delete nextOne;
      }
      else {
        mSelector.Reset();
      }
    }
    else {
      nsCSSSelector*  selector = &mSelector;

      while (nsnull != selector->mNext) {
        if (aSelector == selector->mNext) {
          selector->mNext = aSelector->mNext;
          delete aSelector;
          return;
        }
        selector = selector->mNext;
      }
    }
  }
}

void CSSStyleRuleImpl::SetSourceSelectorText(const nsString& aSelectorText)
{
    /* no need for set, since get recreates the string */
}

void CSSStyleRuleImpl::GetSourceSelectorText(nsString& aSelectorText) const
{
  mSelector.ToString( aSelectorText, mSheet, IsPseudoElement(mSelector.mTag),
                      0 );
}

PRUint32 CSSStyleRuleImpl::GetLineNumber(void) const
{
  return mLineNumber;
}

void CSSStyleRuleImpl::SetLineNumber(PRUint32 aLineNumber)
{
  mLineNumber = aLineNumber;
}

nsCSSDeclaration* CSSStyleRuleImpl::GetDeclaration(void) const
{
  nsCSSDeclaration* result = mDeclaration;
  return result;
}

void CSSStyleRuleImpl::SetDeclaration(nsCSSDeclaration* aDeclaration)
{
  if (mDeclaration != aDeclaration) {
    NS_IF_RELEASE(mImportantRule); 
    if (nsnull != mDeclaration) {
      mDeclaration->Release();
    }
    mDeclaration = aDeclaration;
    mDeclaration->AddRef();
  }
}

PRInt32 CSSStyleRuleImpl::GetWeight(void) const
{
  return mWeight;
}

void CSSStyleRuleImpl::SetWeight(PRInt32 aWeight)
{
  mWeight = aWeight;
}

already_AddRefed<nsIStyleRule> CSSStyleRuleImpl::GetImportantRule(void)
{
  if (!mImportantRule && mDeclaration) {
    if (mDeclaration->HasImportantData()) {
      mImportantRule = new CSSImportantRule(mSheet, mDeclaration);
      NS_ADDREF(mImportantRule);
    }
  }
  NS_IF_ADDREF(mImportantRule);
  return mImportantRule;
}

nsresult
CSSStyleRuleImpl::GetValue(nsCSSProperty aProperty, nsCSSValue& aValue)
{
  return mDeclaration->GetValueOrImportantValue(aProperty, aValue);
}

NS_IMETHODIMP
CSSStyleRuleImpl::GetStyleSheet(nsIStyleSheet*& aSheet) const
{
  return nsCSSRule::GetStyleSheet(aSheet);
}

NS_IMETHODIMP
CSSStyleRuleImpl::SetStyleSheet(nsICSSStyleSheet* aSheet)
{
  nsCSSRule::SetStyleSheet(aSheet);
  if (nsnull != mImportantRule) { // we're responsible for this guy too
    mImportantRule->mSheet = aSheet;
  }
  return NS_OK;
}

NS_IMETHODIMP
CSSStyleRuleImpl::SetParentRule(nsICSSGroupRule* aRule)
{
  return nsCSSRule::SetParentRule(aRule);
}

NS_IMETHODIMP
CSSStyleRuleImpl::GetType(PRInt32& aType) const
{
  aType = nsICSSRule::STYLE_RULE;
  return NS_OK;
}

NS_IMETHODIMP
CSSStyleRuleImpl::Clone(nsICSSRule*& aClone) const
{
  CSSStyleRuleImpl* clone = new CSSStyleRuleImpl(*this);
  if (clone) {
    return CallQueryInterface(clone, &aClone);
  }
  aClone = nsnull;
  return NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP
CSSStyleRuleImpl::MapRuleInfoInto(nsRuleData* aRuleData)
{
  return mDeclaration->MapRuleInfoInto(aRuleData);
}

#ifdef DEBUG
NS_IMETHODIMP
CSSStyleRuleImpl::List(FILE* out, PRInt32 aIndent) const
{
  // Indent
  for (PRInt32 index = aIndent; --index >= 0; ) fputs("  ", out);

  nsAutoString buffer;
  mSelector.ToString(buffer, mSheet, PR_FALSE, 0);

  buffer.Append(NS_LITERAL_STRING(" weight: "));
  buffer.AppendInt(mWeight, 10);
  buffer.Append(NS_LITERAL_STRING(" "));
  fputs(NS_LossyConvertUCS2toASCII(buffer).get(), out);
  if (nsnull != mDeclaration) {
    mDeclaration->List(out);
  }
  else {
    fputs("{ null declaration }", out);
  }
  fputs("\n", out);

  return NS_OK;
}
#endif

NS_IMETHODIMP    
CSSStyleRuleImpl::GetType(PRUint16* aType)
{
  *aType = nsIDOMCSSRule::STYLE_RULE;
  
  return NS_OK;
}

NS_IMETHODIMP    
CSSStyleRuleImpl::GetCssText(nsAString& aCssText)
{
  mSelector.ToString( aCssText, mSheet, IsPseudoElement(mSelector.mTag),
                      0 );
  aCssText.Append(PRUnichar(' '));
  aCssText.Append(PRUnichar('{'));
  aCssText.Append(PRUnichar(' '));
  if (mDeclaration)
  {
    nsAutoString   tempString;
    mDeclaration->ToString( tempString );
    aCssText.Append( tempString );
  }
  aCssText.Append(PRUnichar(' '));
  aCssText.Append(PRUnichar('}'));
  return NS_OK;
}

NS_IMETHODIMP    
CSSStyleRuleImpl::SetCssText(const nsAString& aCssText)
{
  // XXX TBI - need to re-parse rule & declaration
  return NS_OK;
}

NS_IMETHODIMP    
CSSStyleRuleImpl::GetParentStyleSheet(nsIDOMCSSStyleSheet** aSheet)
{
  if (mSheet) {
    return CallQueryInterface(mSheet, aSheet);
  }
  *aSheet = nsnull;
  return NS_OK;
}

NS_IMETHODIMP    
CSSStyleRuleImpl::GetParentRule(nsIDOMCSSRule** aParentRule)
{
  if (mParentRule) {
    return CallQueryInterface(mParentRule, aParentRule);
  }
  *aParentRule = nsnull;
  return NS_OK;
}

NS_IMETHODIMP    
CSSStyleRuleImpl::GetSelectorText(nsAString& aSelectorText)
{
  mSelector.ToString( aSelectorText, mSheet, IsPseudoElement(mSelector.mTag), 0 );
  return NS_OK;
}

NS_IMETHODIMP    
CSSStyleRuleImpl::SetSelectorText(const nsAString& aSelectorText)
{
  // XXX TBI - get a parser and re-parse the selectors, 
  // XXX then need to re-compute the cascade
  // XXX and dirty sheet
  return NS_OK;
}

NS_IMETHODIMP    
CSSStyleRuleImpl::GetStyle(nsIDOMCSSStyleDeclaration** aStyle)
{
  if (nsnull == mDOMDeclaration) {
    mDOMDeclaration = new DOMCSSDeclarationImpl(this);
    if (nsnull == mDOMDeclaration) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    NS_ADDREF(mDOMDeclaration);
  }
  
  *aStyle = mDOMDeclaration;
  NS_ADDREF(mDOMDeclaration);
  
  return NS_OK;
}

nsresult
NS_NewCSSStyleRule(nsICSSStyleRule** aInstancePtrResult,
                   const nsCSSSelector& aSelector)
{
  if (aInstancePtrResult == nsnull) {
    return NS_ERROR_NULL_POINTER;
  }

  CSSStyleRuleImpl *it = new CSSStyleRuleImpl(aSelector);
  if (!it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return CallQueryInterface(it, aInstancePtrResult);
}
