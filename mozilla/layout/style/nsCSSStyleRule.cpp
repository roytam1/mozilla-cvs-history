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
#include "nsICSSStyleRule.h"
#include "nsICSSDeclaration.h"
#include "nsIStyleSheet.h"
#include "nsIStyleContext.h"
#include "nsIPresContext.h"
#include "nsIDeviceContext.h"
#include "nsIArena.h"
#include "nsIAtom.h"
#include "nsCRT.h"
#include "nsString.h"
#include "nsStyleConsts.h"
#include "nsHTMLAtoms.h"
#include "nsUnitConversion.h"
#include "nsStyleUtil.h"
#include "nsIFontMetrics.h"
#include "nsIDOMCSSStyleSheet.h"
#include "nsIDOMCSSStyleRule.h"
#include "nsIDOMCSSStyleRuleSimple.h"
#include "nsIDOMCSSStyleDeclaration.h"
#include "nsIScriptGlobalObject.h"
#include "nsIScriptObjectOwner.h"
#include "nsDOMCSSDeclaration.h"

//#define DEBUG_REFS

static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
static NS_DEFINE_IID(kIStyleRuleIID, NS_ISTYLE_RULE_IID);
static NS_DEFINE_IID(kICSSDeclarationIID, NS_ICSS_DECLARATION_IID);
static NS_DEFINE_IID(kICSSStyleRuleIID, NS_ICSS_STYLE_RULE_IID);
static NS_DEFINE_IID(kIDOMCSSStyleSheetIID, NS_IDOMCSSSTYLESHEET_IID);
static NS_DEFINE_IID(kIDOMCSSStyleRuleIID, NS_IDOMCSSSTYLERULE_IID);
static NS_DEFINE_IID(kIDOMCSSStyleRuleSimpleIID, NS_IDOMCSSSTYLERULESIMPLE_IID);
static NS_DEFINE_IID(kIDOMCSSStyleDeclarationIID, NS_IDOMCSSSTYLEDECLARATION_IID);
static NS_DEFINE_IID(kIScriptObjectOwnerIID, NS_ISCRIPTOBJECTOWNER_IID);

static NS_DEFINE_IID(kCSSFontSID, NS_CSS_FONT_SID);
static NS_DEFINE_IID(kCSSColorSID, NS_CSS_COLOR_SID);
static NS_DEFINE_IID(kCSSTextSID, NS_CSS_TEXT_SID);
static NS_DEFINE_IID(kCSSMarginSID, NS_CSS_MARGIN_SID);
static NS_DEFINE_IID(kCSSPositionSID, NS_CSS_POSITION_SID);
static NS_DEFINE_IID(kCSSListSID, NS_CSS_LIST_SID);
static NS_DEFINE_IID(kCSSDisplaySID, NS_CSS_DISPLAY_SID);
static NS_DEFINE_IID(kCSSTableSID, NS_CSS_TABLE_SID);

// -- nsCSSSelector -------------------------------

nsCSSSelector::nsCSSSelector()
  : mTag(nsnull), mID(nsnull), mClass(nsnull), mPseudoClass(nsnull),
    mNext(nsnull)
{
}

nsCSSSelector::nsCSSSelector(nsIAtom* aTag, nsIAtom* aID, nsIAtom* aClass, nsIAtom* aPseudoClass)
  : mTag(aTag), mID(aID), mClass(aClass), mPseudoClass(aPseudoClass),
    mNext(nsnull)
{
  NS_IF_ADDREF(mTag);
  NS_IF_ADDREF(mID);
  NS_IF_ADDREF(mClass);
  NS_IF_ADDREF(mPseudoClass);
}

nsCSSSelector::nsCSSSelector(const nsCSSSelector& aCopy) 
  : mTag(aCopy.mTag), mID(aCopy.mID), mClass(aCopy.mClass), mPseudoClass(aCopy.mPseudoClass),
    mNext(nsnull)
{ // implmented to support extension to CSS2 (when we have to copy the array)
  NS_IF_ADDREF(mTag);
  NS_IF_ADDREF(mID);
  NS_IF_ADDREF(mClass);
  NS_IF_ADDREF(mPseudoClass);
}

nsCSSSelector::~nsCSSSelector()  
{  
  NS_IF_RELEASE(mTag);
  NS_IF_RELEASE(mID);
  NS_IF_RELEASE(mClass);
  NS_IF_RELEASE(mPseudoClass);
}

nsCSSSelector& nsCSSSelector::operator=(const nsCSSSelector& aCopy)
{
  NS_IF_RELEASE(mTag);
  NS_IF_RELEASE(mID);
  NS_IF_RELEASE(mClass);
  NS_IF_RELEASE(mPseudoClass);
  mTag          = aCopy.mTag;
  mID           = aCopy.mID;
  mClass        = aCopy.mClass;
  mPseudoClass  = aCopy.mPseudoClass;
  NS_IF_ADDREF(mTag);
  NS_IF_ADDREF(mID);
  NS_IF_ADDREF(mClass);
  NS_IF_ADDREF(mPseudoClass);
  return *this;
}

PRBool nsCSSSelector::Equals(const nsCSSSelector* aOther) const
{
  if (nsnull != aOther) {
    return (PRBool)((aOther->mTag == mTag) && (aOther->mID == mID) && 
                    (aOther->mClass == mClass) && (aOther->mPseudoClass == mPseudoClass));
  }
  return PR_FALSE;
}


void nsCSSSelector::Set(const nsString& aTag, const nsString& aID, 
                        const nsString& aClass, const nsString& aPseudoClass)
{
  nsAutoString  buffer;
  NS_IF_RELEASE(mTag);
  NS_IF_RELEASE(mID);
  NS_IF_RELEASE(mClass);
  NS_IF_RELEASE(mPseudoClass);
  if (0 < aTag.Length()) {
    aTag.ToUpperCase(buffer);    // XXX is this correct? what about class?
    mTag = NS_NewAtom(buffer);
  }
  if (0 < aID.Length()) {
    mID = NS_NewAtom(aID);
  }
  if (0 < aClass.Length()) {
    mClass = NS_NewAtom(aClass);
  }
  if (0 < aPseudoClass.Length()) {
    aPseudoClass.ToUpperCase(buffer);
    mPseudoClass = NS_NewAtom(buffer);
    if (nsnull == mTag) {
      mTag = nsHTMLAtoms::a;
      NS_ADDREF(mTag);
    }
  }
}

// -- CSSImportantRule -------------------------------

static nscoord CalcLength(const nsCSSValue& aValue, const nsStyleFont* aFont, 
                          nsIPresContext* aPresContext);
static PRBool SetCoord(const nsCSSValue& aValue, nsStyleCoord& aCoord, 
                       PRInt32 aMask, const nsStyleFont* aFont, 
                       nsIPresContext* aPresContext);

static void MapDeclarationInto(nsICSSDeclaration* aDeclaration, 
                               nsIStyleContext* aContext, nsIPresContext* aPresContext);


class CSSImportantRule : public nsIStyleRule {
public:
  CSSImportantRule(nsICSSDeclaration* aDeclaration);

  NS_DECL_ISUPPORTS

  NS_IMETHOD Equals(const nsIStyleRule* aRule, PRBool& aResult) const;
  NS_IMETHOD HashValue(PRUint32& aValue) const;

  // Strength is an out-of-band weighting, useful for mapping CSS ! important
  NS_IMETHOD GetStrength(PRInt32& aStrength);

  NS_IMETHOD MapStyleInto(nsIStyleContext* aContext, nsIPresContext* aPresContext);

  NS_IMETHOD List(FILE* out = stdout, PRInt32 aIndent = 0) const;

protected:
  ~CSSImportantRule(void);

  nsICSSDeclaration*  mDeclaration;
};

CSSImportantRule::CSSImportantRule(nsICSSDeclaration* aDeclaration)
  : mDeclaration(aDeclaration)
{
  NS_INIT_REFCNT();
  NS_IF_ADDREF(mDeclaration);
}

CSSImportantRule::~CSSImportantRule(void)
{
  NS_IF_RELEASE(mDeclaration);
}

NS_IMPL_ISUPPORTS(CSSImportantRule, kIStyleRuleIID);

NS_IMETHODIMP
CSSImportantRule::Equals(const nsIStyleRule* aRule, PRBool& aResult) const
{
  aResult = PRBool(aRule == this);
  return NS_OK;
}

NS_IMETHODIMP
CSSImportantRule::HashValue(PRUint32& aValue) const
{
  aValue = PRUint32(mDeclaration);
  return NS_OK;
}

// Strength is an out-of-band weighting, useful for mapping CSS ! important
NS_IMETHODIMP
CSSImportantRule::GetStrength(PRInt32& aStrength)
{
  aStrength = 1;
  return NS_OK;
}

NS_IMETHODIMP
CSSImportantRule::MapStyleInto(nsIStyleContext* aContext, nsIPresContext* aPresContext)
{
  MapDeclarationInto(mDeclaration, aContext, aPresContext);
  return NS_OK;
}

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

// -- nsDOMStyleRuleDeclaration -------------------------------

class DOMCSSDeclarationImpl : public nsDOMCSSDeclaration
{
public:
  DOMCSSDeclarationImpl(nsICSSStyleRule *aRule);
  ~DOMCSSDeclarationImpl();

  virtual void DropReference();
  virtual nsresult GetCSSDeclaration(nsICSSDeclaration **aDecl,
                                     PRBool aAllocate);
  virtual nsresult StylePropertyChanged(const nsString& aPropertyName,
                                        PRInt32 aHint);
  virtual nsresult GetParent(nsISupports **aParent);

protected:
  nsICSSStyleRule *mRule;
};

DOMCSSDeclarationImpl::DOMCSSDeclarationImpl(nsICSSStyleRule *aRule)
{
  // This reference is not reference-counted. The rule
  // object tells us when its about to go away.
  mRule = aRule;
}

DOMCSSDeclarationImpl::~DOMCSSDeclarationImpl()
{
}

void 
DOMCSSDeclarationImpl::DropReference()
{
  mRule = nsnull;
}

nsresult
DOMCSSDeclarationImpl::GetCSSDeclaration(nsICSSDeclaration **aDecl,
                                             PRBool aAllocate)
{
  if (nsnull != mRule) {
    *aDecl = mRule->GetDeclaration();
  }
  else {
    *aDecl = nsnull;
  }

  return NS_OK;
}

nsresult 
DOMCSSDeclarationImpl::StylePropertyChanged(const nsString& aPropertyName,
                                            PRInt32 aHint)
{
  // XXX TBI
  return NS_OK;
}

nsresult 
DOMCSSDeclarationImpl::GetParent(nsISupports **aParent)
{
  if (nsnull != mRule) {
    return mRule->QueryInterface(kISupportsIID, (void **)aParent);
  }

  return NS_OK;
}

// -- nsCSSStyleRule -------------------------------

class CSSStyleRuleImpl : public nsICSSStyleRule, 
                         public nsIDOMCSSStyleRuleSimple, 
                         public nsIScriptObjectOwner {
public:
  void* operator new(size_t size);
  void* operator new(size_t size, nsIArena* aArena);
  void operator delete(void* ptr);

  CSSStyleRuleImpl(const nsCSSSelector& aSelector);

  NS_IMETHOD QueryInterface(const nsIID& aIID, void** aInstancePtr);
  NS_IMETHOD_(nsrefcnt) AddRef();
  NS_IMETHOD_(nsrefcnt) Release();

  NS_IMETHOD Equals(const nsIStyleRule* aRule, PRBool& aResult) const;
  NS_IMETHOD HashValue(PRUint32& aValue) const;
  // Strength is an out-of-band weighting, useful for mapping CSS ! important
  NS_IMETHOD GetStrength(PRInt32& aStrength);

  virtual nsCSSSelector* FirstSelector(void);
  virtual void AddSelector(const nsCSSSelector& aSelector);
  virtual void DeleteSelector(nsCSSSelector* aSelector);

  virtual nsICSSDeclaration* GetDeclaration(void) const;
  virtual void SetDeclaration(nsICSSDeclaration* aDeclaration);

  virtual PRInt32 GetWeight(void) const;
  virtual void SetWeight(PRInt32 aWeight);

  virtual nsIStyleRule* GetImportantRule(void);

  virtual nsIStyleSheet* GetStyleSheet(void);
  virtual void SetStyleSheet(nsIStyleSheet *aSheet);

  NS_IMETHOD MapStyleInto(nsIStyleContext* aContext, nsIPresContext* aPresContext);

  NS_IMETHOD List(FILE* out = stdout, PRInt32 aIndent = 0) const;

  // nsIDOMCSSStyleRule interface
  NS_IMETHOD    GetType(nsString& aType);

  // nsIDOMCSSStyleRuleSimple interface
  NS_IMETHOD    GetSelectorText(nsString& aSelectorText);
  NS_IMETHOD    SetSelectorText(const nsString& aSelectorText);
  NS_IMETHOD    GetStyle(nsIDOMCSSStyleDeclaration** aStyle);

  // nsIScriptObjectOwner interface
  NS_IMETHOD GetScriptObject(nsIScriptContext *aContext, void** aScriptObject);
  NS_IMETHOD SetScriptObject(void* aScriptObject);

private: 
  // These are not supported and are not implemented! 
  CSSStyleRuleImpl(const CSSStyleRuleImpl& aCopy); 
  CSSStyleRuleImpl& operator=(const CSSStyleRuleImpl& aCopy); 

protected:
  virtual ~CSSStyleRuleImpl();

protected:
  PRUint32 mInHeap : 1;
  PRUint32 mRefCnt : 31;

  nsCSSSelector       mSelector;
  nsICSSDeclaration*  mDeclaration;
  PRInt32             mWeight;
  CSSImportantRule*   mImportantRule;
  nsIStyleSheet*      mStyleSheet;                         
  DOMCSSDeclarationImpl *mDOMDeclaration;                          
  void*               mScriptObject;                           
#ifdef DEBUG_REFS
  PRInt32 mInstance;
#endif
};


void* CSSStyleRuleImpl::operator new(size_t size)
{
  CSSStyleRuleImpl* rv = (CSSStyleRuleImpl*) ::operator new(size);
#ifdef NS_DEBUG
  if (nsnull != rv) {
    nsCRT::memset(rv, 0xEE, size);
  }
#endif
  rv->mInHeap = 1;
  return (void*) rv;
}

void* CSSStyleRuleImpl::operator new(size_t size, nsIArena* aArena)
{
  CSSStyleRuleImpl* rv = (CSSStyleRuleImpl*) aArena->Alloc(PRInt32(size));
#ifdef NS_DEBUG
  if (nsnull != rv) {
    nsCRT::memset(rv, 0xEE, size);
  }
#endif
  rv->mInHeap = 0;
  return (void*) rv;
}

void CSSStyleRuleImpl::operator delete(void* ptr)
{
  CSSStyleRuleImpl* rule = (CSSStyleRuleImpl*) ptr;
  if (nsnull != rule) {
    if (rule->mInHeap) {
      ::delete ptr;
    }
  }
}


#ifdef DEBUG_REFS
static PRInt32 gInstanceCount;
static const PRInt32 kInstrument = 1075;
#endif

CSSStyleRuleImpl::CSSStyleRuleImpl(const nsCSSSelector& aSelector)
  : mSelector(aSelector), mDeclaration(nsnull), 
    mWeight(0), mImportantRule(nsnull)
{
  NS_INIT_REFCNT();
  mDOMDeclaration = nsnull;
  mScriptObject = nsnull;
#ifdef DEBUG_REFS
  mInstance = gInstanceCount++;
  fprintf(stdout, "%d of %d + CSSStyleRule\n", mInstance, gInstanceCount);
#endif
}

CSSStyleRuleImpl::~CSSStyleRuleImpl()
{
  nsCSSSelector*  next = mSelector.mNext;

  while (nsnull != next) {
    nsCSSSelector*  selector = next;
    next = selector->mNext;
    delete selector;
  }
  NS_IF_RELEASE(mDeclaration);
  NS_IF_RELEASE(mImportantRule);
  if (nsnull != mDOMDeclaration) {
    mDOMDeclaration->DropReference();
  }
#ifdef DEBUG_REFS
  --gInstanceCount;
  fprintf(stdout, "%d of %d - CSSStyleRule\n", mInstance, gInstanceCount);
#endif
}

#ifdef DEBUG_REFS
nsrefcnt CSSStyleRuleImpl::AddRef(void)                                
{                                    
  if (mInstance == kInstrument) {
    fprintf(stdout, "%d AddRef CSSStyleRule\n", mRefCnt + 1);
  }
  return ++mRefCnt;                                          
}

nsrefcnt CSSStyleRuleImpl::Release(void)                         
{                                                      
  if (mInstance == kInstrument) {
    fprintf(stdout, "%d Release CSSStyleRule\n", mRefCnt - 1);
  }
  if (--mRefCnt == 0) {                                
    NS_DELETEXPCOM(this);
    return 0;                                          
  }                                                    
  return mRefCnt;                                      
}
#else
NS_IMPL_ADDREF(CSSStyleRuleImpl)
NS_IMPL_RELEASE(CSSStyleRuleImpl)
#endif

nsresult CSSStyleRuleImpl::QueryInterface(const nsIID& aIID,
                                            void** aInstancePtrResult)
{
  NS_PRECONDITION(nsnull != aInstancePtrResult, "null pointer");
  if (nsnull == aInstancePtrResult) {
    return NS_ERROR_NULL_POINTER;
  }
  if (aIID.Equals(kICSSStyleRuleIID)) {
    *aInstancePtrResult = (void*) ((nsICSSStyleRule*)this);
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(kIStyleRuleIID)) {
    *aInstancePtrResult = (void*) ((nsIStyleRule*)this);
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(kIDOMCSSStyleRuleIID)) {
    nsIDOMCSSStyleRule *tmp = this;
    *aInstancePtrResult = (void*) tmp;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(kIDOMCSSStyleRuleSimpleIID)) {
    nsIDOMCSSStyleRuleSimple *tmp = this;
    *aInstancePtrResult = (void*) tmp;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(kIScriptObjectOwnerIID)) {
    nsIScriptObjectOwner *tmp = this;
    *aInstancePtrResult = (void*) tmp;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(kISupportsIID)) {
    nsICSSStyleRule *tmp = this;
    nsISupports *tmp2 = tmp;
    *aInstancePtrResult = (void*) tmp2;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  return NS_NOINTERFACE;
}


NS_IMETHODIMP CSSStyleRuleImpl::Equals(const nsIStyleRule* aRule, PRBool& aResult) const
{
  nsICSSStyleRule* iCSSRule;

  if (this == aRule) {
    aResult = PR_TRUE;
  }
  else {
    aResult = PR_FALSE;
    if ((nsnull != aRule) && 
        (NS_OK == ((nsIStyleRule*)aRule)->QueryInterface(kICSSStyleRuleIID, (void**) &iCSSRule))) {

      CSSStyleRuleImpl* rule = (CSSStyleRuleImpl*)iCSSRule;
      const nsCSSSelector* local = &mSelector;
      const nsCSSSelector* other = &(rule->mSelector);
      aResult = PR_TRUE;

      if ((rule->mDeclaration != mDeclaration) || 
          (rule->mWeight != mWeight)) {
        aResult = PR_FALSE;
      }
      while ((PR_TRUE == aResult) && (nsnull != local) && (nsnull != other)) {
        if (! local->Equals(other)) {
          aResult = PR_FALSE;
        }
        local = local->mNext;
        other = other->mNext;
      }
      if ((nsnull != local) || (nsnull != other)) { // more were left
        aResult = PR_FALSE;
      }
      NS_RELEASE(iCSSRule);
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
CSSStyleRuleImpl::HashValue(PRUint32& aValue) const
{
  aValue = (PRUint32)this;
  return NS_OK;
}

// Strength is an out-of-band weighting, useful for mapping CSS ! important
NS_IMETHODIMP
CSSStyleRuleImpl::GetStrength(PRInt32& aStrength)
{
  aStrength = 0;
  return NS_OK;
}

nsCSSSelector* CSSStyleRuleImpl::FirstSelector(void)
{
  return &mSelector;
}

void CSSStyleRuleImpl::AddSelector(const nsCSSSelector& aSelector)
{
  if ((nsnull != aSelector.mTag) || (nsnull != aSelector.mID) ||
      (nsnull != aSelector.mClass) || (nsnull != aSelector.mPseudoClass)) { // skip empty selectors
    nsCSSSelector*  selector = new nsCSSSelector(aSelector);
    nsCSSSelector*  last = &mSelector;

    while (nsnull != last->mNext) {
      last = last->mNext;
    }
    last->mNext = selector;
  }
}


void CSSStyleRuleImpl::DeleteSelector(nsCSSSelector* aSelector)
{
  if (nsnull != aSelector) {
    if (&mSelector == aSelector) {  // handle first selector
      mSelector = *aSelector; // assign value
      mSelector.mNext = aSelector->mNext;
      delete aSelector;
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

nsICSSDeclaration* CSSStyleRuleImpl::GetDeclaration(void) const
{
  NS_IF_ADDREF(mDeclaration);
  return mDeclaration;
}

void CSSStyleRuleImpl::SetDeclaration(nsICSSDeclaration* aDeclaration)
{
  NS_IF_RELEASE(mImportantRule); 
  NS_IF_RELEASE(mDeclaration);
  mDeclaration = aDeclaration;
  NS_IF_ADDREF(mDeclaration);
}

PRInt32 CSSStyleRuleImpl::GetWeight(void) const
{
  return mWeight;
}

void CSSStyleRuleImpl::SetWeight(PRInt32 aWeight)
{
  mWeight = aWeight;
}

nsIStyleRule* CSSStyleRuleImpl::GetImportantRule(void)
{
  if ((nsnull == mImportantRule) && (nsnull != mDeclaration)) {
    nsICSSDeclaration*  important;
    mDeclaration->GetImportantValues(important);
    if (nsnull != important) {
      mImportantRule = new CSSImportantRule(important);
      NS_ADDREF(mImportantRule);
      NS_RELEASE(important);
    }
  }
  NS_IF_ADDREF(mImportantRule);
  return mImportantRule;
}

nsIStyleSheet* CSSStyleRuleImpl::GetStyleSheet(void)
{
  NS_IF_ADDREF(mStyleSheet);
  
  return mStyleSheet;
}

void CSSStyleRuleImpl::SetStyleSheet(nsIStyleSheet *aSheet)
{
  // XXX We don't reference count this up reference. The style sheet
  // will tell us when it's going away or when we're detached from
  // it.
  mStyleSheet = aSheet;
}

nscoord CalcLength(const nsCSSValue& aValue,
                   const nsStyleFont* aFont, 
                   nsIPresContext* aPresContext)
{
  NS_ASSERTION(aValue.IsLengthUnit(), "not a length unit");
  if (aValue.IsFixedLengthUnit()) {
    return aValue.GetLengthTwips();
  }
  nsCSSUnit unit = aValue.GetUnit();
  switch (unit) {
    case eCSSUnit_EM:
      return NSToCoordRound(aValue.GetFloatValue() * (float)aFont->mFont.size);
      // XXX scale against font metrics height instead
    case eCSSUnit_EN:
      return NSToCoordRound((aValue.GetFloatValue() * (float)aFont->mFont.size) / 2.0f);
    case eCSSUnit_XHeight: {
      nsIFontMetrics* fm = aPresContext->GetMetricsFor(aFont->mFont);
      NS_ASSERTION(nsnull != fm, "can't get font metrics");
      nscoord xHeight;
      if (nsnull != fm) {
        fm->GetXHeight(xHeight);
        NS_RELEASE(fm);
      }
      else {
        xHeight = ((aFont->mFont.size / 3) * 2);
      }
      return NSToCoordRound(aValue.GetFloatValue() * (float)xHeight);
    }
    case eCSSUnit_CapHeight: {
      NS_NOTYETIMPLEMENTED("cap height unit");
      nscoord capHeight = ((aFont->mFont.size / 3) * 2); // XXX HACK!
      return NSToCoordRound(aValue.GetFloatValue() * (float)capHeight);
    }
    case eCSSUnit_Pixel:
      float p2t;
      aPresContext->GetScaledPixelsToTwips(p2t);
      return NSFloatPixelsToTwips(aValue.GetFloatValue(), p2t);
  }
  return 0;
}


#define SETCOORD_NORMAL       0x01
#define SETCOORD_AUTO         0x02
#define SETCOORD_INHERIT      0x04
#define SETCOORD_PERCENT      0x08
#define SETCOORD_FACTOR       0x10
#define SETCOORD_LENGTH       0x20
#define SETCOORD_INTEGER      0x40
#define SETCOORD_ENUMERATED   0x80

#define SETCOORD_LP     (SETCOORD_LENGTH | SETCOORD_PERCENT)
#define SETCOORD_LH     (SETCOORD_LENGTH | SETCOORD_INHERIT)
#define SETCOORD_AH     (SETCOORD_AUTO | SETCOORD_INHERIT)
#define SETCOORD_LPH    (SETCOORD_LP | SETCOORD_INHERIT)
#define SETCOORD_LPFHN  (SETCOORD_LPH | SETCOORD_FACTOR | SETCOORD_NORMAL)
#define SETCOORD_LPAH   (SETCOORD_LP | SETCOORD_AH)
#define SETCOORD_LPEH   (SETCOORD_LP | SETCOORD_ENUMERATED | SETCOORD_INHERIT)
#define SETCOORD_LEH    (SETCOORD_LENGTH | SETCOORD_ENUMERATED | SETCOORD_INHERIT)
#define SETCOORD_IAH    (SETCOORD_INTEGER | SETCOORD_AH)

static PRBool SetCoord(const nsCSSValue& aValue, nsStyleCoord& aCoord, 
                       PRInt32 aMask, const nsStyleFont* aFont, 
                       nsIPresContext* aPresContext)
{
  PRBool  result = PR_TRUE;
  if (aValue.GetUnit() == eCSSUnit_Null) {
    result = PR_FALSE;
  }
  else if (((aMask & SETCOORD_LENGTH) != 0) && 
           aValue.IsLengthUnit()) {
    aCoord.SetCoordValue(CalcLength(aValue, aFont, aPresContext));
  } 
  else if (((aMask & SETCOORD_PERCENT) != 0) && 
           (aValue.GetUnit() == eCSSUnit_Percent)) {
    aCoord.SetPercentValue(aValue.GetPercentValue());
  } 
  else if (((aMask & SETCOORD_INTEGER) != 0) && 
           (aValue.GetUnit() == eCSSUnit_Integer)) {
    aCoord.SetIntValue(aValue.GetIntValue(), eStyleUnit_Integer);
  } 
  else if (((aMask & SETCOORD_ENUMERATED) != 0) && 
           (aValue.GetUnit() == eCSSUnit_Enumerated)) {
    aCoord.SetIntValue(aValue.GetIntValue(), eStyleUnit_Enumerated);
  } 
  else if (((aMask & SETCOORD_AUTO) != 0) && 
           (aValue.GetUnit() == eCSSUnit_Auto)) {
    aCoord.SetAutoValue();
  } 
  else if (((aMask & SETCOORD_INHERIT) != 0) && 
           (aValue.GetUnit() == eCSSUnit_Inherit)) {
    aCoord.SetInheritValue();
  }
  else if (((aMask & SETCOORD_NORMAL) != 0) && 
           (aValue.GetUnit() == eCSSUnit_Normal)) {
    aCoord.SetNormalValue();
  }
  else if (((aMask & SETCOORD_FACTOR) != 0) && 
           (aValue.GetUnit() == eCSSUnit_Number)) {
    aCoord.SetFactorValue(aValue.GetFloatValue());
  }
  else {
    result = PR_FALSE;  // didn't set anything
  }
  return result;
}

static PRBool SetColor(const nsCSSValue& aValue, const nscolor aParentColor, nscolor& aResult)
{
  PRBool  result = PR_FALSE;
  nsCSSUnit unit = aValue.GetUnit();

  if (eCSSUnit_Color == unit) {
    aResult = aValue.GetColorValue();
    result = PR_TRUE;
  }
  else if (eCSSUnit_String == unit) {
    nsAutoString  value;
    char  cbuf[100];
    aValue.GetStringValue(value);
    value.ToCString(cbuf, sizeof(cbuf));
    nscolor rgba;
    if (NS_ColorNameToRGB(cbuf, &rgba)) {
      aResult = rgba;
      result = PR_TRUE;
    }
  }
  return result;
}

NS_IMETHODIMP
CSSStyleRuleImpl::MapStyleInto(nsIStyleContext* aContext, nsIPresContext* aPresContext)
{
  MapDeclarationInto(mDeclaration, aContext, aPresContext);
  return NS_OK;
}

void MapDeclarationInto(nsICSSDeclaration* aDeclaration, 
                        nsIStyleContext* aContext, nsIPresContext* aPresContext)
{
  if (nsnull != aDeclaration) {
    nsIStyleContext* parentContext = aContext->GetParent();
    nsStyleFont*  font = (nsStyleFont*)aContext->GetMutableStyleData(eStyleStruct_Font);
    const nsStyleFont* parentFont = font;
    if (nsnull != parentContext) {
      parentFont = (const nsStyleFont*)parentContext->GetStyleData(eStyleStruct_Font);
    }

    nsCSSFont*  ourFont;
    if (NS_OK == aDeclaration->GetData(kCSSFontSID, (nsCSSStruct**)&ourFont)) {
      if (nsnull != ourFont) {
        const nsFont& defaultFont = aPresContext->GetDefaultFont();
        const nsFont& defaultFixedFont = aPresContext->GetDefaultFixedFont();

        // font-family: string list, enum, inherit
        if (eCSSUnit_String == ourFont->mFamily.GetUnit()) {
          nsIDeviceContext* dc = aPresContext->GetDeviceContext();
          if (nsnull != dc) {
            nsAutoString  familyList;

            ourFont->mFamily.GetStringValue(familyList);

            font->mFont.name = familyList;
            nsAutoString  face;
            if (NS_OK == dc->FirstExistingFont(font->mFont, face)) {
              if (face.EqualsIgnoreCase("monospace")) {
                font->mFont = font->mFixedFont;
              }
              else {
                font->mFixedFont.name = familyList;
              }
            }
            else {
              font->mFont.name = defaultFont.name;
              font->mFixedFont.name = defaultFixedFont.name;
            }
            font->mFlags |= NS_STYLE_FONT_FACE_EXPLICIT;
            NS_RELEASE(dc);
          }
        }
        else if (eCSSUnit_Enumerated == ourFont->mFamily.GetUnit()) {
          NS_NOTYETIMPLEMENTED("system font");
        }
        else if (eCSSUnit_Inherit == ourFont->mFamily.GetUnit()) {
          font->mFont.name = parentFont->mFont.name;
          font->mFixedFont.name = parentFont->mFixedFont.name;
          font->mFlags &= ~NS_STYLE_FONT_FACE_EXPLICIT;
          font->mFlags |= (parentFont->mFlags & NS_STYLE_FONT_FACE_EXPLICIT);
        }

        // font-style: enum, normal, inherit
        if (eCSSUnit_Enumerated == ourFont->mStyle.GetUnit()) {
          font->mFont.style = ourFont->mStyle.GetIntValue();
          font->mFixedFont.style = ourFont->mStyle.GetIntValue();
        }
        else if (eCSSUnit_Normal == ourFont->mStyle.GetUnit()) {
          font->mFont.style = NS_STYLE_FONT_STYLE_NORMAL;
          font->mFixedFont.style = NS_STYLE_FONT_STYLE_NORMAL;
        }
        else if (eCSSUnit_Inherit == ourFont->mStyle.GetUnit()) {
          font->mFont.style = parentFont->mFont.style;
          font->mFixedFont.style = parentFont->mFixedFont.style;
        }

        // font-variant: enum, normal, inherit
        if (eCSSUnit_Enumerated == ourFont->mVariant.GetUnit()) {
          font->mFont.variant = ourFont->mVariant.GetIntValue();
          font->mFixedFont.variant = ourFont->mVariant.GetIntValue();
        }
        else if (eCSSUnit_Normal == ourFont->mVariant.GetUnit()) {
          font->mFont.variant = NS_STYLE_FONT_VARIANT_NORMAL;
          font->mFixedFont.variant = NS_STYLE_FONT_VARIANT_NORMAL;
        }
        else if (eCSSUnit_Inherit == ourFont->mVariant.GetUnit()) {
          font->mFont.variant = parentFont->mFont.variant;
          font->mFixedFont.variant = parentFont->mFixedFont.variant;
        }

        // font-weight: int, enum, normal, inherit
        if (eCSSUnit_Integer == ourFont->mWeight.GetUnit()) {
          font->mFont.weight = ourFont->mWeight.GetIntValue();
          font->mFixedFont.weight = ourFont->mWeight.GetIntValue();
        }
        else if (eCSSUnit_Enumerated == ourFont->mWeight.GetUnit()) {
          PRInt32 value = ourFont->mWeight.GetIntValue();
          switch (value) {
            case NS_STYLE_FONT_WEIGHT_NORMAL:
            case NS_STYLE_FONT_WEIGHT_BOLD:
              font->mFont.weight = value;
              font->mFixedFont.weight = value;
              break;
            case NS_STYLE_FONT_WEIGHT_BOLDER:
            case NS_STYLE_FONT_WEIGHT_LIGHTER:
              font->mFont.weight = (parentFont->mFont.weight + value);
              font->mFixedFont.weight = (parentFont->mFixedFont.weight + value);
              break;
          }
        }
        else if (eCSSUnit_Normal == ourFont->mWeight.GetUnit()) {
          font->mFont.weight = NS_STYLE_FONT_WEIGHT_NORMAL;
          font->mFixedFont.weight = NS_STYLE_FONT_WEIGHT_NORMAL;
        }
        else if (eCSSUnit_Inherit == ourFont->mWeight.GetUnit()) {
          font->mFont.weight = parentFont->mFont.weight;
          font->mFixedFont.weight = parentFont->mFixedFont.weight;
        }

        // font-size: enum, length, percent, inherit
        if (eCSSUnit_Enumerated == ourFont->mSize.GetUnit()) {
          PRInt32 value = ourFont->mSize.GetIntValue();
          PRInt32 scaler = aPresContext->GetFontScaler();
          float scaleFactor = nsStyleUtil::GetScalingFactor(scaler);

          if ((NS_STYLE_FONT_SIZE_XXSMALL <= value) && 
              (value <= NS_STYLE_FONT_SIZE_XXLARGE)) {
            font->mFont.size = nsStyleUtil::CalcFontPointSize(value, (PRInt32)defaultFont.size, scaleFactor);
            font->mFixedFont.size = nsStyleUtil::CalcFontPointSize(value, (PRInt32)defaultFixedFont.size, scaleFactor);
          }
          else if (NS_STYLE_FONT_SIZE_LARGER == value) {
            PRInt32 index = nsStyleUtil::FindNextLargerFontSize(parentFont->mFont.size, (PRInt32)defaultFont.size, scaleFactor);
            font->mFont.size = nsStyleUtil::CalcFontPointSize(index, (PRInt32)defaultFont.size, scaleFactor);
            font->mFixedFont.size = nsStyleUtil::CalcFontPointSize(index, (PRInt32)defaultFixedFont.size, scaleFactor);
          }
          else if (NS_STYLE_FONT_SIZE_SMALLER == value) {
            PRInt32 index = nsStyleUtil::FindNextSmallerFontSize(parentFont->mFont.size, (PRInt32)defaultFont.size, scaleFactor);
            font->mFont.size = nsStyleUtil::CalcFontPointSize(index, (PRInt32)defaultFont.size, scaleFactor);
            font->mFixedFont.size = nsStyleUtil::CalcFontPointSize(index, (PRInt32)defaultFixedFont.size, scaleFactor);
          }
          // this does NOT explicitly set font size
          font->mFlags &= ~NS_STYLE_FONT_SIZE_EXPLICIT;
        }
        else if (ourFont->mSize.IsLengthUnit()) {
          font->mFont.size = CalcLength(ourFont->mSize, parentFont, aPresContext);
          font->mFixedFont.size = CalcLength(ourFont->mSize, parentFont, aPresContext);
          font->mFlags |= NS_STYLE_FONT_SIZE_EXPLICIT;
        }
        else if (eCSSUnit_Percent == ourFont->mSize.GetUnit()) {
          font->mFont.size = (nscoord)((float)(parentFont->mFont.size) * ourFont->mSize.GetPercentValue());
          font->mFixedFont.size = (nscoord)((float)(parentFont->mFixedFont.size) * ourFont->mSize.GetPercentValue());
          font->mFlags |= NS_STYLE_FONT_SIZE_EXPLICIT;
        }
        else if (eCSSUnit_Inherit == ourFont->mSize.GetUnit()) {
          font->mFont.size = parentFont->mFont.size;
          font->mFixedFont.size = parentFont->mFixedFont.size;
          font->mFlags &= ~NS_STYLE_FONT_SIZE_EXPLICIT;
          font->mFlags |= (parentFont->mFlags & NS_STYLE_FONT_SIZE_EXPLICIT);
        }
      }
    }

    nsCSSText*  ourText;
    if (NS_OK == aDeclaration->GetData(kCSSTextSID, (nsCSSStruct**)&ourText)) {
      if (nsnull != ourText) {
        // Get our text style and our parent's text style
        nsStyleText* text = (nsStyleText*) aContext->GetMutableStyleData(eStyleStruct_Text);
        const nsStyleText* parentText = text;
        if (nsnull != parentContext) {
          parentText = (const nsStyleText*)parentContext->GetStyleData(eStyleStruct_Text);
        }

        // letter-spacing: normal, length, inherit
        SetCoord(ourText->mLetterSpacing, text->mLetterSpacing, SETCOORD_LH | SETCOORD_NORMAL, 
                 font, aPresContext);

        // line-height: normal, number, length, percent, inherit
        SetCoord(ourText->mLineHeight, text->mLineHeight, SETCOORD_LPFHN, font, aPresContext);

        // text-align: enum, string, inherit
        if (eCSSUnit_Enumerated == ourText->mTextAlign.GetUnit()) {
          text->mTextAlign = ourText->mTextAlign.GetIntValue();
        }
        else if (eCSSUnit_String == ourText->mTextAlign.GetUnit()) {
          NS_NOTYETIMPLEMENTED("align string");
        }
        else if (eCSSUnit_Inherit == ourText->mTextAlign.GetUnit()) {
          text->mTextAlign = parentText->mTextAlign;
        }

        // text-indent: length, percent, inherit
        SetCoord(ourText->mTextIndent, text->mTextIndent, SETCOORD_LPH, font, aPresContext);

        // text-decoration: none, enum (bit field), inherit
        if (eCSSUnit_Enumerated == ourText->mDecoration.GetUnit()) {
          PRInt32 td = ourText->mDecoration.GetIntValue();
          font->mFont.decorations = (parentFont->mFont.decorations | td);
          font->mFixedFont.decorations = (parentFont->mFixedFont.decorations | td);
          text->mTextDecoration = td;
        }
        else if (eCSSUnit_None == ourText->mDecoration.GetUnit()) {
          font->mFont.decorations = NS_STYLE_TEXT_DECORATION_NONE;
          font->mFixedFont.decorations = NS_STYLE_TEXT_DECORATION_NONE;
          text->mTextDecoration = NS_STYLE_TEXT_DECORATION_NONE;
        }
        else if (eCSSUnit_Inherit == ourText->mDecoration.GetUnit()) {
          font->mFont.decorations = parentFont->mFont.decorations;
          font->mFixedFont.decorations = parentFont->mFixedFont.decorations;
          text->mTextDecoration = parentText->mTextDecoration;
        }

        // text-transform: enum, none, inherit
        if (eCSSUnit_Enumerated == ourText->mTextTransform.GetUnit()) {
          text->mTextTransform = ourText->mTextTransform.GetIntValue();
        }
        else if (eCSSUnit_None == ourText->mTextTransform.GetUnit()) {
          text->mTextTransform = NS_STYLE_TEXT_TRANSFORM_NONE;
        }
        else if (eCSSUnit_Inherit == ourText->mTextTransform.GetUnit()) {
          text->mTextTransform = parentText->mTextTransform;
        }

        // vertical-align: enum, length, percent, inherit
        if (! SetCoord(ourText->mVerticalAlign, text->mVerticalAlign, SETCOORD_LP | SETCOORD_ENUMERATED,
                 font, aPresContext)) {
          // XXX this really needs to pass the inherit value along...
          if (eCSSUnit_Inherit == ourText->mVerticalAlign.GetUnit()) {
            text->mVerticalAlign = parentText->mVerticalAlign;
          }
        }

        // white-space: enum, normal, inherit
        if (eCSSUnit_Enumerated == ourText->mWhiteSpace.GetUnit()) {
          text->mWhiteSpace = ourText->mWhiteSpace.GetIntValue();
        }
        else if (eCSSUnit_Normal == ourText->mWhiteSpace.GetUnit()) {
          text->mWhiteSpace = NS_STYLE_WHITESPACE_NORMAL;
        }
        else if (eCSSUnit_Inherit == ourText->mWhiteSpace.GetUnit()) {
          text->mWhiteSpace = parentText->mWhiteSpace;
        }

        // word-spacing: normal, length, inherit
        SetCoord(ourText->mWordSpacing, text->mWordSpacing, SETCOORD_LH | SETCOORD_NORMAL,
                 font, aPresContext);
      }
    }

    nsCSSDisplay*  ourDisplay;
    if (NS_OK == aDeclaration->GetData(kCSSDisplaySID, (nsCSSStruct**)&ourDisplay)) {
      if (nsnull != ourDisplay) {
        // Get our style and our parent's style
        nsStyleDisplay* display = (nsStyleDisplay*)
          aContext->GetMutableStyleData(eStyleStruct_Display);

        const nsStyleDisplay* parentDisplay = display;
        if (nsnull != parentContext) {
          parentDisplay = (const nsStyleDisplay*)parentContext->GetStyleData(eStyleStruct_Display);
        }

        // display: enum, none, inherit
        if (eCSSUnit_Enumerated == ourDisplay->mDisplay.GetUnit()) {
          display->mDisplay = ourDisplay->mDisplay.GetIntValue();
        }
        else if (eCSSUnit_None == ourDisplay->mDisplay.GetUnit()) {
          display->mDisplay = NS_STYLE_DISPLAY_NONE;
        }
        else if (eCSSUnit_Inherit == ourDisplay->mDisplay.GetUnit()) {
          display->mDisplay = parentDisplay->mDisplay;
        }

        // direction: enum, inherit
        if (eCSSUnit_Enumerated == ourDisplay->mDirection.GetUnit()) {
          display->mDirection = ourDisplay->mDirection.GetIntValue();
        }
        else if (eCSSUnit_Inherit == ourDisplay->mDirection.GetUnit()) {
          display->mDirection = parentDisplay->mDirection;
        }

        // clear: enum, none, inherit
        if (eCSSUnit_Enumerated == ourDisplay->mClear.GetUnit()) {
          display->mBreakType = ourDisplay->mClear.GetIntValue();
        }
        else if (eCSSUnit_None == ourDisplay->mClear.GetUnit()) {
          display->mBreakType = NS_STYLE_CLEAR_NONE;
        }
        else if (eCSSUnit_Inherit == ourDisplay->mClear.GetUnit()) {
          display->mBreakType = parentDisplay->mBreakType;
        }

        // float: enum, none, inherit
        if (eCSSUnit_Enumerated == ourDisplay->mFloat.GetUnit()) {
          display->mFloats = ourDisplay->mFloat.GetIntValue();
        }
        else if (eCSSUnit_None == ourDisplay->mFloat.GetUnit()) {
          display->mFloats = NS_STYLE_FLOAT_NONE;
        }
        else if (eCSSUnit_Inherit == ourDisplay->mFloat.GetUnit()) {
          display->mFloats = parentDisplay->mFloats;
        }

        // visibility: enum, inherit
        if (eCSSUnit_Enumerated == ourDisplay->mVisibility.GetUnit()) {
          display->mVisible = PRBool (NS_STYLE_VISIBILITY_VISIBLE == ourDisplay->mVisibility.GetIntValue());
        }
        else if (eCSSUnit_Inherit == ourDisplay->mVisibility.GetUnit()) {
          display->mVisible = parentDisplay->mVisible;
        }

        // overflow: enum, auto, inherit
        if (eCSSUnit_Enumerated == ourDisplay->mOverflow.GetUnit()) {
          display->mOverflow = ourDisplay->mOverflow.GetIntValue();
        }
        else if (eCSSUnit_Auto == ourDisplay->mOverflow.GetUnit()) {
          display->mOverflow = NS_STYLE_OVERFLOW_AUTO;
        }
        else if (eCSSUnit_Inherit == ourDisplay->mOverflow.GetUnit()) {
          display->mOverflow = parentDisplay->mOverflow;
        }

        // clip property: length, auto, inherit
        if (nsnull != ourDisplay->mClip) {
          if (eCSSUnit_Inherit == ourDisplay->mClip->mTop.GetUnit()) { // if one is inherit, they all are
            display->mClipFlags = NS_STYLE_CLIP_INHERIT;
          }
          else {
            PRBool  fullAuto = PR_TRUE;

            display->mClipFlags = 0; // clear it

            if (eCSSUnit_Auto == ourDisplay->mClip->mTop.GetUnit()) {
              display->mClip.top = 0;
              display->mClipFlags |= NS_STYLE_CLIP_TOP_AUTO;
            } 
            else if (ourDisplay->mClip->mTop.IsLengthUnit()) {
              display->mClip.top = CalcLength(ourDisplay->mClip->mTop, font, aPresContext);
              fullAuto = PR_FALSE;
            }
            if (eCSSUnit_Auto == ourDisplay->mClip->mRight.GetUnit()) {
              display->mClip.right = 0;
              display->mClipFlags |= NS_STYLE_CLIP_RIGHT_AUTO;
            } 
            else if (ourDisplay->mClip->mRight.IsLengthUnit()) {
              display->mClip.right = CalcLength(ourDisplay->mClip->mRight, font, aPresContext);
              fullAuto = PR_FALSE;
            }
            if (eCSSUnit_Auto == ourDisplay->mClip->mBottom.GetUnit()) {
              display->mClip.bottom = 0;
              display->mClipFlags |= NS_STYLE_CLIP_BOTTOM_AUTO;
            } 
            else if (ourDisplay->mClip->mBottom.IsLengthUnit()) {
              display->mClip.bottom = CalcLength(ourDisplay->mClip->mBottom, font, aPresContext);
              fullAuto = PR_FALSE;
            }
            if (eCSSUnit_Auto == ourDisplay->mClip->mLeft.GetUnit()) {
              display->mClip.left = 0;
              display->mClipFlags |= NS_STYLE_CLIP_LEFT_AUTO;
            } 
            else if (ourDisplay->mClip->mLeft.IsLengthUnit()) {
              display->mClip.left = CalcLength(ourDisplay->mClip->mLeft, font, aPresContext);
              fullAuto = PR_FALSE;
            }
            display->mClipFlags &= ~NS_STYLE_CLIP_TYPE_MASK;
            if (fullAuto) {
              display->mClipFlags |= NS_STYLE_CLIP_AUTO;
            }
            else {
              display->mClipFlags |= NS_STYLE_CLIP_RECT;
            }
          }
        }
      }
    }


    nsCSSColor*  ourColor;
    if (NS_OK == aDeclaration->GetData(kCSSColorSID, (nsCSSStruct**)&ourColor)) {
      if (nsnull != ourColor) {
        nsStyleColor* color = (nsStyleColor*)aContext->GetMutableStyleData(eStyleStruct_Color);

        const nsStyleColor* parentColor = color;
        if (nsnull != parentContext) {
          parentColor = (const nsStyleColor*)parentContext->GetStyleData(eStyleStruct_Color);
        }

        // color: color, string, inherit
        if (! SetColor(ourColor->mColor, parentColor->mColor, color->mColor)) {
          if (eCSSUnit_Inherit == ourColor->mColor.GetUnit()) {
            color->mColor = parentColor->mColor;
          }
        }

        // cursor: enum, auto, url, inherit
        nsCSSValueList*  list = ourColor->mCursor;
        if (nsnull != list) {
          // XXX need to deal with multiple URL values
          if (eCSSUnit_Enumerated == list->mValue.GetUnit()) {
            color->mCursor = list->mValue.GetIntValue();
          }
          else if (eCSSUnit_Auto == list->mValue.GetUnit()) {
            color->mCursor = NS_STYLE_CURSOR_AUTO;
          }
          else if (eCSSUnit_URL == list->mValue.GetUnit()) {
            list->mValue.GetStringValue(color->mCursorImage);
          }
          else if (eCSSUnit_Inherit == list->mValue.GetUnit()) {
            color->mCursor = parentColor->mCursor;
          }
        }

        // background-color: color, string, enum (flags), inherit
        if (SetColor(ourColor->mBackColor, parentColor->mBackgroundColor, color->mBackgroundColor)) {
          color->mBackgroundFlags &= ~NS_STYLE_BG_COLOR_TRANSPARENT;
        }
        else if (eCSSUnit_Enumerated == ourColor->mBackColor.GetUnit()) {
          color->mBackgroundFlags |= NS_STYLE_BG_COLOR_TRANSPARENT;
        }
        else if (eCSSUnit_Inherit == ourColor->mBackColor.GetUnit()) {
          color->mBackgroundColor = parentColor->mBackgroundColor;
          color->mBackgroundFlags &= ~NS_STYLE_BG_COLOR_TRANSPARENT;
          color->mBackgroundFlags |= (parentColor->mBackgroundFlags & NS_STYLE_BG_COLOR_TRANSPARENT);
        }

        // background-image: url, none, inherit
        if (eCSSUnit_URL == ourColor->mBackImage.GetUnit()) {
          ourColor->mBackImage.GetStringValue(color->mBackgroundImage);
          color->mBackgroundFlags &= ~NS_STYLE_BG_IMAGE_NONE;
        }
        else if (eCSSUnit_None == ourColor->mBackImage.GetUnit()) {
          color->mBackgroundImage.Truncate();
          color->mBackgroundFlags |= NS_STYLE_BG_IMAGE_NONE;
        }
        else if (eCSSUnit_Inherit == ourColor->mBackImage.GetUnit()) {
          color->mBackgroundImage = parentColor->mBackgroundImage;
          color->mBackgroundFlags &= ~NS_STYLE_BG_IMAGE_NONE;
          color->mBackgroundFlags |= (parentColor->mBackgroundFlags & NS_STYLE_BG_IMAGE_NONE);
        }

        // background-repeat: enum, inherit
        if (eCSSUnit_Enumerated == ourColor->mBackRepeat.GetUnit()) {
          color->mBackgroundRepeat = ourColor->mBackRepeat.GetIntValue();
        }
        else if (eCSSUnit_Inherit == ourColor->mBackRepeat.GetUnit()) {
          color->mBackgroundRepeat = parentColor->mBackgroundRepeat;
        }

        // background-attachment: enum, inherit
        if (eCSSUnit_Enumerated == ourColor->mBackAttachment.GetUnit()) {
          color->mBackgroundAttachment = ourColor->mBackAttachment.GetIntValue();
        }
        else if (eCSSUnit_Inherit == ourColor->mBackAttachment.GetUnit()) {
          color->mBackgroundAttachment = parentColor->mBackgroundAttachment;
        }

        // background-position: enum, length, percent (flags), inherit
        if (eCSSUnit_Percent == ourColor->mBackPositionX.GetUnit()) {
          color->mBackgroundXPosition = (nscoord)(100.0f * ourColor->mBackPositionX.GetPercentValue());
          color->mBackgroundFlags |= NS_STYLE_BG_X_POSITION_PERCENT;
          color->mBackgroundFlags &= ~NS_STYLE_BG_X_POSITION_LENGTH;
        }
        else if (ourColor->mBackPositionX.IsLengthUnit()) {
          color->mBackgroundXPosition = CalcLength(ourColor->mBackPositionX,
                                                   font, aPresContext);
          color->mBackgroundFlags |= NS_STYLE_BG_X_POSITION_LENGTH;
          color->mBackgroundFlags &= ~NS_STYLE_BG_X_POSITION_PERCENT;
        }
        else if (eCSSUnit_Enumerated == ourColor->mBackPositionX.GetUnit()) {
          color->mBackgroundXPosition = (nscoord)ourColor->mBackPositionX.GetIntValue();
          color->mBackgroundFlags |= NS_STYLE_BG_X_POSITION_PERCENT;
          color->mBackgroundFlags &= ~NS_STYLE_BG_X_POSITION_LENGTH;
        }
        else if (eCSSUnit_Inherit == ourColor->mBackPositionX.GetUnit()) {
          color->mBackgroundXPosition = parentColor->mBackgroundXPosition;
          color->mBackgroundFlags &= ~NS_STYLE_BG_X_POSITION_LENGTH;
          color->mBackgroundFlags |= (parentColor->mBackgroundFlags & NS_STYLE_BG_X_POSITION_PERCENT);
        }

        if (eCSSUnit_Percent == ourColor->mBackPositionY.GetUnit()) {
          color->mBackgroundYPosition = (nscoord)(100.0f * ourColor->mBackPositionY.GetPercentValue());
          color->mBackgroundFlags |= NS_STYLE_BG_Y_POSITION_PERCENT;
          color->mBackgroundFlags &= ~NS_STYLE_BG_Y_POSITION_LENGTH;
        }
        else if (ourColor->mBackPositionY.IsLengthUnit()) {
          color->mBackgroundYPosition = CalcLength(ourColor->mBackPositionY,
                                                   font, aPresContext);
          color->mBackgroundFlags |= NS_STYLE_BG_Y_POSITION_LENGTH;
          color->mBackgroundFlags &= ~NS_STYLE_BG_Y_POSITION_PERCENT;
        }
        else if (eCSSUnit_Enumerated == ourColor->mBackPositionY.GetUnit()) {
          color->mBackgroundYPosition = (nscoord)ourColor->mBackPositionY.GetIntValue();
          color->mBackgroundFlags |= NS_STYLE_BG_Y_POSITION_PERCENT;
          color->mBackgroundFlags &= ~NS_STYLE_BG_Y_POSITION_LENGTH;
        }
        else if (eCSSUnit_Inherit == ourColor->mBackPositionY.GetUnit()) {
          color->mBackgroundYPosition = parentColor->mBackgroundYPosition;
          color->mBackgroundFlags &= ~NS_STYLE_BG_Y_POSITION_LENGTH;
          color->mBackgroundFlags |= (parentColor->mBackgroundFlags & NS_STYLE_BG_Y_POSITION_PERCENT);
        }

        // opacity: factor, percent, inherit
        if (eCSSUnit_Percent == ourColor->mOpacity.GetUnit()) {
          color->mOpacity = ourColor->mOpacity.GetPercentValue();
        }
        else if (eCSSUnit_Number == ourColor->mOpacity.GetUnit()) {
          color->mOpacity = ourColor->mOpacity.GetFloatValue();
        }
        else if (eCSSUnit_Inherit == ourColor->mOpacity.GetUnit()) {
          color->mOpacity = parentColor->mOpacity;
        }

  // XXX: NYI        nsCSSValue mBackFilter;
      }
    }

    nsCSSMargin*  ourMargin;
    if (NS_OK == aDeclaration->GetData(kCSSMarginSID, (nsCSSStruct**)&ourMargin)) {
      if (nsnull != ourMargin) {
        nsStyleSpacing* spacing = (nsStyleSpacing*)
          aContext->GetMutableStyleData(eStyleStruct_Spacing);

        const nsStyleSpacing* parentSpacing = spacing;
        if (nsnull != parentContext) {
          parentSpacing = (const nsStyleSpacing*)parentContext->GetStyleData(eStyleStruct_Spacing);
        }

        // margin: length, percent, auto, inherit
        if (nsnull != ourMargin->mMargin) {
          nsStyleCoord  coord;
          if (SetCoord(ourMargin->mMargin->mLeft, coord, SETCOORD_LPAH, font, aPresContext)) {
            spacing->mMargin.SetLeft(coord);
          }
          if (SetCoord(ourMargin->mMargin->mTop, coord, SETCOORD_LPAH, font, aPresContext)) {
            spacing->mMargin.SetTop(coord);
          }
          if (SetCoord(ourMargin->mMargin->mRight, coord, SETCOORD_LPAH, font, aPresContext)) {
            spacing->mMargin.SetRight(coord);
          }
          if (SetCoord(ourMargin->mMargin->mBottom, coord, SETCOORD_LPAH, font, aPresContext)) {
            spacing->mMargin.SetBottom(coord);
          }
        }

        // padding: length, percent, inherit
        if (nsnull != ourMargin->mPadding) {
          nsStyleCoord  coord;
          if (SetCoord(ourMargin->mPadding->mLeft, coord, SETCOORD_LPH, font, aPresContext)) {
            spacing->mPadding.SetLeft(coord);
          }
          if (SetCoord(ourMargin->mPadding->mTop, coord, SETCOORD_LPH, font, aPresContext)) {
            spacing->mPadding.SetTop(coord);
          }
          if (SetCoord(ourMargin->mPadding->mRight, coord, SETCOORD_LPH, font, aPresContext)) {
            spacing->mPadding.SetRight(coord);
          }
          if (SetCoord(ourMargin->mPadding->mBottom, coord, SETCOORD_LPH, font, aPresContext)) {
            spacing->mPadding.SetBottom(coord);
          }
        }

        // border-size: length, enum, inherit
        if (nsnull != ourMargin->mBorderWidth) {
          nsStyleCoord  coord;
          if (SetCoord(ourMargin->mBorderWidth->mLeft, coord, SETCOORD_LEH, font, aPresContext)) {
            spacing->mBorder.SetLeft(coord);
          }
          if (SetCoord(ourMargin->mBorderWidth->mTop, coord, SETCOORD_LEH, font, aPresContext)) {
            spacing->mBorder.SetTop(coord);
          }
          if (SetCoord(ourMargin->mBorderWidth->mRight, coord, SETCOORD_LEH, font, aPresContext)) {
            spacing->mBorder.SetRight(coord);
          }
          if (SetCoord(ourMargin->mBorderWidth->mBottom, coord, SETCOORD_LEH, font, aPresContext)) {
            spacing->mBorder.SetBottom(coord);
          }
        }

        // border-style: enum, none, inhert
        if (nsnull != ourMargin->mBorderStyle) {
          nsCSSRect* ourStyle = ourMargin->mBorderStyle;
          if (eCSSUnit_Enumerated == ourStyle->mTop.GetUnit()) {
            spacing->mBorderStyle[NS_SIDE_TOP] = ourStyle->mTop.GetIntValue();
          }
          else if (eCSSUnit_None == ourStyle->mTop.GetUnit()) {
            spacing->mBorderStyle[NS_SIDE_TOP] = NS_STYLE_BORDER_STYLE_NONE;
          }
          else if (eCSSUnit_Inherit == ourStyle->mTop.GetUnit()) {
            spacing->mBorderStyle[NS_SIDE_TOP] = parentSpacing->mBorderStyle[NS_SIDE_TOP];
          }

          if (eCSSUnit_Enumerated == ourStyle->mRight.GetUnit()) {
            spacing->mBorderStyle[NS_SIDE_RIGHT] = ourStyle->mRight.GetIntValue();
          }
          else if (eCSSUnit_None == ourStyle->mRight.GetUnit()) {
            spacing->mBorderStyle[NS_SIDE_RIGHT] = NS_STYLE_BORDER_STYLE_NONE;
          }
          else if (eCSSUnit_Inherit == ourStyle->mRight.GetUnit()) {
            spacing->mBorderStyle[NS_SIDE_RIGHT] = parentSpacing->mBorderStyle[NS_SIDE_RIGHT];
          }

          if (eCSSUnit_Enumerated == ourStyle->mBottom.GetUnit()) {
            spacing->mBorderStyle[NS_SIDE_BOTTOM] = ourStyle->mBottom.GetIntValue();
          }
          else if (eCSSUnit_None == ourStyle->mBottom.GetUnit()) {
            spacing->mBorderStyle[NS_SIDE_BOTTOM] = NS_STYLE_BORDER_STYLE_NONE;
          }
          else if (eCSSUnit_Inherit == ourStyle->mBottom.GetUnit()) {
            spacing->mBorderStyle[NS_SIDE_BOTTOM] = parentSpacing->mBorderStyle[NS_SIDE_BOTTOM];
          }

          if (eCSSUnit_Enumerated == ourStyle->mLeft.GetUnit()) {
            spacing->mBorderStyle[NS_SIDE_LEFT] = ourStyle->mLeft.GetIntValue();
          }
          else if (eCSSUnit_None == ourStyle->mLeft.GetUnit()) {
            spacing->mBorderStyle[NS_SIDE_LEFT] = NS_STYLE_BORDER_STYLE_NONE;
          }
          else if (eCSSUnit_Inherit == ourStyle->mLeft.GetUnit()) {
            spacing->mBorderStyle[NS_SIDE_LEFT] = parentSpacing->mBorderStyle[NS_SIDE_LEFT];
          }
        }

        // border-color: color. string, enum, inherit
        if (nsnull != ourMargin->mBorderColor) {
          nsCSSRect* ourColor = ourMargin->mBorderColor;
          if (! SetColor(ourColor->mTop, parentSpacing->mBorderColor[NS_SIDE_TOP], 
                         spacing->mBorderColor[NS_SIDE_TOP])) {
            if (eCSSUnit_Enumerated == ourColor->mTop.GetUnit()) {
              spacing->mBorderColor[NS_SIDE_TOP] = NS_RGBA(0, 0, 0, 0); // transparent
            }
            else if (eCSSUnit_Inherit == ourColor->mTop.GetUnit()) {
              spacing->mBorderColor[NS_SIDE_TOP] = parentSpacing->mBorderColor[NS_SIDE_TOP];
            }
          }

          if (! SetColor(ourColor->mRight, parentSpacing->mBorderColor[NS_SIDE_RIGHT], 
                         spacing->mBorderColor[NS_SIDE_RIGHT])) {
            if (eCSSUnit_Enumerated == ourColor->mRight.GetUnit()) {
              spacing->mBorderColor[NS_SIDE_RIGHT] = NS_RGBA(0, 0, 0, 0); // transparent
            }
            else if (eCSSUnit_Inherit == ourColor->mRight.GetUnit()) {
              spacing->mBorderColor[NS_SIDE_RIGHT] = parentSpacing->mBorderColor[NS_SIDE_RIGHT];
            }
          }

          if (! SetColor(ourColor->mBottom, parentSpacing->mBorderColor[NS_SIDE_BOTTOM], 
                         spacing->mBorderColor[NS_SIDE_BOTTOM])) {
            if (eCSSUnit_Enumerated == ourColor->mBottom.GetUnit()) {
              spacing->mBorderColor[NS_SIDE_BOTTOM] = NS_RGBA(0, 0, 0, 0); // transparent
            }
            else if (eCSSUnit_Inherit == ourColor->mBottom.GetUnit()) {
              spacing->mBorderColor[NS_SIDE_BOTTOM] = parentSpacing->mBorderColor[NS_SIDE_BOTTOM];
            }
          }

          if (! SetColor(ourColor->mLeft, parentSpacing->mBorderColor[NS_SIDE_LEFT], 
                         spacing->mBorderColor[NS_SIDE_LEFT])) {
            if (eCSSUnit_Enumerated == ourColor->mLeft.GetUnit()) {
              spacing->mBorderColor[NS_SIDE_LEFT] = NS_RGBA(0, 0, 0, 0); // transparent
            }
            else if (eCSSUnit_Inherit == ourColor->mLeft.GetUnit()) {
              spacing->mBorderColor[NS_SIDE_LEFT] = parentSpacing->mBorderColor[NS_SIDE_LEFT];
            }
          }
        }
      }
    }

    nsCSSPosition*  ourPosition;
    if (NS_OK == aDeclaration->GetData(kCSSPositionSID, (nsCSSStruct**)&ourPosition)) {
      if (nsnull != ourPosition) {
        nsStylePosition* position = (nsStylePosition*)aContext->GetMutableStyleData(eStyleStruct_Position);

        const nsStylePosition* parentPosition = position;
        if (nsnull != parentContext) {
          parentPosition = (const nsStylePosition*)parentContext->GetStyleData(eStyleStruct_Position);
        }

        // position: enum, inherit
        if (eCSSUnit_Enumerated == ourPosition->mPosition.GetUnit()) {
          position->mPosition = ourPosition->mPosition.GetIntValue();
        }
        else if (eCSSUnit_Inherit == ourPosition->mPosition.GetUnit()) {
          position->mPosition = parentPosition->mPosition;
        }

        // box offsets: length, percent, auto, inherit
        if (nsnull != ourPosition->mOffset) {
          SetCoord(ourPosition->mOffset->mTop, position->mTopOffset, SETCOORD_LPAH, font, aPresContext);
          // XXX right bottom
          SetCoord(ourPosition->mOffset->mLeft, position->mLeftOffset, SETCOORD_LPAH, font, aPresContext);
        }

        SetCoord(ourPosition->mWidth, position->mWidth, SETCOORD_LPAH, font, aPresContext);
        SetCoord(ourPosition->mMinWidth, position->mWidth, SETCOORD_LPH, font, aPresContext);
        if (! SetCoord(ourPosition->mMaxWidth, position->mWidth, SETCOORD_LPH, font, aPresContext)) {
          if (eCSSUnit_None == ourPosition->mMaxWidth.GetUnit()) {
            position->mMaxWidth.Reset();
          }
        }

        SetCoord(ourPosition->mHeight, position->mHeight, SETCOORD_LPAH, font, aPresContext);
        SetCoord(ourPosition->mMinHeight, position->mHeight, SETCOORD_LPH, font, aPresContext);
        if (! SetCoord(ourPosition->mMaxHeight, position->mHeight, SETCOORD_LPH, font, aPresContext)) {
          if (eCSSUnit_None == ourPosition->mMaxHeight.GetUnit()) {
            position->mMaxHeight.Reset();
          }
        }

        // z-index
        SetCoord(ourPosition->mZIndex, position->mZIndex, SETCOORD_IAH, nsnull, nsnull);
      }
    }

    nsCSSList* ourList;
    if (NS_OK == aDeclaration->GetData(kCSSListSID, (nsCSSStruct**)&ourList)) {
      if (nsnull != ourList) {
        nsStyleList* list = (nsStyleList*)aContext->GetMutableStyleData(eStyleStruct_List);

        const nsStyleList* parentList = list;
        if (nsnull != parentList) {
          parentList = (const nsStyleList*)parentContext->GetStyleData(eStyleStruct_List);
        }

        // list-style-type: enum, none, inherit
        if (eCSSUnit_Enumerated == ourList->mType.GetUnit()) {
          list->mListStyleType = ourList->mType.GetIntValue();
        }
        else if (eCSSUnit_None == ourList->mType.GetUnit()) {
          list->mListStyleType = NS_STYLE_LIST_STYLE_NONE;
        }
        else if (eCSSUnit_Inherit == ourList->mType.GetUnit()) {
          list->mListStyleType = parentList->mListStyleType;
        }

        // list-style-image: url, none, inherit
        if (eCSSUnit_URL == ourList->mImage.GetUnit()) {
          ourList->mImage.GetStringValue(list->mListStyleImage);
        }
        else if (eCSSUnit_None == ourList->mImage.GetUnit()) {
          list->mListStyleImage.Truncate();
        }
        else if (eCSSUnit_Inherit == ourList->mImage.GetUnit()) {
          list->mListStyleImage = parentList->mListStyleImage;
        }

        // list-style-position: enum, inherit
        if (eCSSUnit_Enumerated == ourList->mPosition.GetUnit()) {
          list->mListStylePosition = ourList->mPosition.GetIntValue();
        }
        else if (eCSSUnit_Inherit == ourList->mPosition.GetUnit()) {
          list->mListStylePosition = parentList->mListStylePosition;
        }
      }
    }

    nsCSSTable* ourTable;
    if (NS_OK == aDeclaration->GetData(kCSSTableSID, (nsCSSStruct**)&ourTable)) {
      if (nsnull != ourTable) {
        nsStyleTable* table = (nsStyleTable*)aContext->GetMutableStyleData(eStyleStruct_Table);

        const nsStyleTable* parentTable = table;
        if (nsnull != parentTable) {
          parentTable = (const nsStyleTable*)parentContext->GetStyleData(eStyleStruct_Table);
        }
        nsStyleCoord  coord;

#if 0
        // border-collapse: enum, inherit
        if (eCSSUnit_Enumerated == ourTable->mBorderCollapse.GetUnit()) {
          table->m = ourTable->mBorderCollapse.GetIntValue();
        }
        else if (eCSSUnit_Inherit == ourTable->mBorderCollapse.GetUnit()) {
          table->m = parentTable->m;
        }

        // border-spacing-x: length, inherit
        if (SetCoord(ourTable->mBorderSpacingX, coord, SETCOORD_LENGTH, font, aPresContext)) {
          table->m = coord.GetCoordValue();
        }
        else if (eCSSUnit_Inherit == ourTable->mBorderSpacingX.GetUnit()) {
          table->m = parentTable->m;
        }
        // border-spacing-y: length, inherit
        if (SetCoord(ourTable->mBorderSpacingY, coord, SETCOORD_LENGTH, font, aPresContext)) {
          table->m = coord.GetCoordValue();
        }
        else if (eCSSUnit_Inherit == ourTable->mBorderSpacingY.GetUnit()) {
          table->m = parentTable->m;
        }

        // caption-side: enum, inherit
        if (eCSSUnit_Enumerated == ourTable->mCaptionSide.GetUnit()) {
          table->m = ourTable->mCaptionSide.GetIntValue();
        }
        else if (eCSSUnit_Inherit == ourTable->mCaptionSide.GetUnit()) {
          table->m = parentTable->m;
        }

        // empty-cells: enum, inherit
        if (eCSSUnit_Enumerated == ourTable->mEmptyCells.GetUnit()) {
          table->m = ourTable->mEmptyCells.GetIntValue();
        }
        else if (eCSSUnit_Inherit == ourTable->mEmptyCells.GetUnit()) {
          table->m = parentTable->m;
        }
#endif

        // table-layout: auto, enum, inherit
        if (eCSSUnit_Enumerated == ourTable->mLayout.GetUnit()) {
          table->mLayoutStrategy = ourTable->mLayout.GetIntValue();
        }
        else if (eCSSUnit_Auto == ourTable->mLayout.GetUnit()) {
          table->mLayoutStrategy = NS_STYLE_TABLE_LAYOUT_AUTO;
        }
        else if (eCSSUnit_Inherit == ourTable->mLayout.GetUnit()) {
          table->mLayoutStrategy = parentTable->mLayoutStrategy;
        }
      }
    }

    NS_IF_RELEASE(parentContext);
  }
}



static void ListSelector(FILE* out, const nsCSSSelector* aSelector)
{
  nsAutoString buffer;

  if (nsnull != aSelector->mTag) {
    aSelector->mTag->ToString(buffer);
    fputs(buffer, out);
  }
  if (nsnull != aSelector->mID) {
    aSelector->mID->ToString(buffer);
    fputs("#", out);
    fputs(buffer, out);
  }
  if (nsnull != aSelector->mClass) {
    aSelector->mClass->ToString(buffer);
    fputs(".", out);
    fputs(buffer, out);
  }
  if (nsnull != aSelector->mPseudoClass) {
    aSelector->mPseudoClass->ToString(buffer);
    fputs(":", out);
    fputs(buffer, out);
  }
}

NS_IMETHODIMP
CSSStyleRuleImpl::List(FILE* out, PRInt32 aIndent) const
{
  // Indent
  for (PRInt32 index = aIndent; --index >= 0; ) fputs("  ", out);

  const nsCSSSelector*  selector = &mSelector;

  while (nsnull != selector) {
    ListSelector(out, selector);
    fputs(" ", out);
    selector = selector->mNext;
  }

  nsAutoString buffer;

  buffer.Append("weight: ");
  buffer.Append(mWeight, 10);
  buffer.Append(" ");
  fputs(buffer, out);
  if (nsnull != mDeclaration) {
    mDeclaration->List(out);
  }
  else {
    fputs("{ null declaration }", out);
  }
  fputs("\n", out);

  return NS_OK;
}

NS_IMETHODIMP    
CSSStyleRuleImpl::GetType(nsString& aType)
{
  // XXX Need to define the different types
  aType.SetString("simple");
  
  return NS_OK;
}

NS_IMETHODIMP    
CSSStyleRuleImpl::GetSelectorText(nsString& aSelectorText)
{
  nsAutoString buffer;
  nsAutoString thisSelector;
  nsCSSSelector *selector = &mSelector;

  // XXX Ugh...they're in reverse order from the source. Sorry
  // for the ugliness.
  aSelectorText.SetLength(0);
  while (nsnull != selector) {
    thisSelector.SetLength(0);
    if (nsnull != selector->mTag) {
      selector->mTag->ToString(buffer);
      thisSelector.Append(buffer);
    }
    if (nsnull != selector->mID) {
      selector->mID->ToString(buffer);
      thisSelector.Append("#");
      thisSelector.Append(buffer);
    }
    if (nsnull != selector->mClass) {
      selector->mClass->ToString(buffer);
      thisSelector.Append(".");
      thisSelector.Append(buffer);
    }
    if (nsnull != selector->mPseudoClass) {
      selector->mPseudoClass->ToString(buffer);
      thisSelector.Append(":");
      thisSelector.Append(buffer);
    }
    aSelectorText.Insert(thisSelector, 0, thisSelector.Length());
    selector = selector->mNext;
  }
  
  return NS_OK;
}

NS_IMETHODIMP    
CSSStyleRuleImpl::SetSelectorText(const nsString& aSelectorText)
{
  // XXX TBI
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

NS_IMETHODIMP 
CSSStyleRuleImpl::GetScriptObject(nsIScriptContext *aContext, void** aScriptObject)
{
  nsresult res = NS_OK;
  nsIScriptGlobalObject *global = aContext->GetGlobalObject();

  if (nsnull == mScriptObject) {
    nsISupports *supports = (nsISupports *)(nsICSSStyleRule *)this;
    // XXX Parent should be the style sheet
    // XXX Should be done through factory
    res = NS_NewScriptCSSStyleRuleSimple(aContext, 
                                         supports,
                                         (nsISupports *)global, 
                                         (void**)&mScriptObject);
  }
  *aScriptObject = mScriptObject;

  NS_RELEASE(global);
  return res;
}

NS_IMETHODIMP 
CSSStyleRuleImpl::SetScriptObject(void* aScriptObject)
{
  mScriptObject = aScriptObject;
  return NS_OK;
}

NS_HTML nsresult
  NS_NewCSSStyleRule(nsICSSStyleRule** aInstancePtrResult, const nsCSSSelector& aSelector)
{
  if (aInstancePtrResult == nsnull) {
    return NS_ERROR_NULL_POINTER;
  }

  CSSStyleRuleImpl  *it = new CSSStyleRuleImpl(aSelector);

  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return it->QueryInterface(kICSSStyleRuleIID, (void **) aInstancePtrResult);
}
