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
 *   Pierre Phaneuf <pp@ludusdesign.com>
 *
 * This Original Code has been modified by IBM Corporation. Modifications made by IBM 
 * described herein are Copyright (c) International Business Machines Corporation, 2000.
 * Modifications to Mozilla code or documentation identified per MPL Section 3.3
 *
 * Date             Modified by     Description of modification
 * 04/20/2000       IBM Corp.      OS/2 VisualAge build.
 */
#include "nsINameSpaceManager.h"
#include "nsIHTMLStyleSheet.h"
#include "nsIArena.h"
#include "nsCRT.h"
#include "nsIAtom.h"
#include "nsIURL.h"
#include "nsIServiceManager.h"
#include "nsISupportsArray.h"
#include "nsHashtable.h"
#include "nsIHTMLContent.h"
#include "nsHTMLAttributes.h"
#include "nsILink.h"
#include "nsStyleUtil.h"
#include "nsIStyleRuleProcessor.h"
#include "nsIStyleRule.h"
#include "nsIFrame.h"
#include "nsIStyleContext.h"
#include "nsHTMLAtoms.h"
#include "nsIPresContext.h"
#include "nsIEventStateManager.h"
#include "nsIDocument.h"
#include "nsHTMLIIDs.h"
#include "nsICSSFrameConstructor.h"
#include "nsIStyleFrameConstruction.h"
#include "nsHTMLParts.h"
#include "nsIPresShell.h"
#include "nsStyleConsts.h"
#include "nsLayoutAtoms.h"
#include "nsLayoutCID.h"
#include "nsIDOMHTMLDocument.h"
#include "nsIDOMHTMLElement.h"

#include "nsRuleWalker.h"

#include "nsIStyleSet.h"
#include "nsISizeOfHandler.h"

static NS_DEFINE_CID(kCSSFrameConstructorCID, NS_CSSFRAMECONSTRUCTOR_CID);

class HTMLColorRule : public nsIStyleRule {
public:
  HTMLColorRule(nsIHTMLStyleSheet* aSheet);
  virtual ~HTMLColorRule();

  NS_DECL_ISUPPORTS

  NS_IMETHOD Equals(const nsIStyleRule* aRule, PRBool& aValue) const;
  NS_IMETHOD HashValue(PRUint32& aValue) const;
  NS_IMETHOD GetStyleSheet(nsIStyleSheet*& aSheet) const;
  // Strength is an out-of-band weighting, always 0 here
  NS_IMETHOD GetStrength(PRInt32& aStrength) const;

  // The new mapping function.
  NS_IMETHOD MapRuleInfoInto(nsRuleData* aRuleData);

#ifdef DEBUG
  NS_IMETHOD List(FILE* out = stdout, PRInt32 aIndent = 0) const;

  virtual void SizeOf(nsISizeOfHandler *aSizeOfHandler, PRUint32 &aSize);
#endif

  nscolor             mColor;
  nsIHTMLStyleSheet*  mSheet;
};

class HTMLDocumentColorRule : public HTMLColorRule {
public:
  HTMLDocumentColorRule(nsIHTMLStyleSheet* aSheet);
  virtual ~HTMLDocumentColorRule();

  NS_IMETHOD MapRuleInfoInto(nsRuleData* aRuleData);

#ifdef DEBUG
  virtual void SizeOf(nsISizeOfHandler *aSizeofHandler, PRUint32 &aSize);
#endif

  void Reset() {
    mInitialized = PR_FALSE;
  }

protected:
  void Initialize(nsIPresContext* aPresContext);

  PRBool mInitialized;
};

HTMLColorRule::HTMLColorRule(nsIHTMLStyleSheet* aSheet)
  : mSheet(aSheet)
{
  NS_INIT_REFCNT();
}

HTMLColorRule::~HTMLColorRule()
{
}

NS_IMPL_ISUPPORTS1(HTMLColorRule, nsIStyleRule)

NS_IMETHODIMP
HTMLColorRule::Equals(const nsIStyleRule* aRule, PRBool& aResult) const
{
  aResult = PRBool(this == aRule);
  return NS_OK;
}

NS_IMETHODIMP
HTMLColorRule::HashValue(PRUint32& aValue) const
{
  aValue = (PRUint32)(mColor);
  return NS_OK;
}

NS_IMETHODIMP
HTMLColorRule::GetStyleSheet(nsIStyleSheet*& aSheet) const
{
  NS_IF_ADDREF(mSheet);
  aSheet = mSheet;
  return NS_OK;
}

// Strength is an out-of-band weighting, always 0 here
NS_IMETHODIMP
HTMLColorRule::GetStrength(PRInt32& aStrength) const
{
  aStrength = 0;
  return NS_OK;
}

NS_IMETHODIMP
HTMLColorRule::MapRuleInfoInto(nsRuleData* aRuleData)
{
  if (aRuleData->mSID == eStyleStruct_Color) {
    if (aRuleData->mColorData->mColor.GetUnit() == eCSSUnit_Null)
      aRuleData->mColorData->mColor = nsCSSValue(mColor);
  }
  return NS_OK;
}

#ifdef DEBUG
NS_IMETHODIMP
HTMLColorRule::List(FILE* out, PRInt32 aIndent) const
{
  return NS_OK;
}

/******************************************************************************
* SizeOf method:
*
*  Self (reported as HTMLColorRule's size): 
*    1) sizeof(*this) + 
*
*  Contained / Aggregated data (not reported as HTMLColorRule's size):
*    1) delegate to the mSheet
*
*  Children / siblings / parents:
*    none
*    
******************************************************************************/
void HTMLColorRule::SizeOf(nsISizeOfHandler *aSizeOfHandler, PRUint32 &aSize)
{
  NS_ASSERTION(aSizeOfHandler != nsnull, "SizeOf handler cannot be null");

  // first get the unique items collection
  UNIQUE_STYLE_ITEMS(uniqueItems);

  if(! uniqueItems->AddItem((void*)this) ){
    // object has already been accounted for
    return;
  }

  // get or create a tag for this instance
  nsCOMPtr<nsIAtom> tag;
  tag = getter_AddRefs(NS_NewAtom("HTMLColorRule"));
  // get the size of an empty instance and add to the sizeof handler
  aSize = sizeof(*this);
  aSizeOfHandler->AddSize(tag,aSize);

  if(mSheet){
    PRUint32 localSize=0;
    mSheet->SizeOf(aSizeOfHandler, localSize);
  }
}
#endif

HTMLDocumentColorRule::HTMLDocumentColorRule(nsIHTMLStyleSheet* aSheet) 
  : HTMLColorRule(aSheet)
{
  Reset();
}

HTMLDocumentColorRule::~HTMLDocumentColorRule()
{
}

NS_IMETHODIMP
HTMLDocumentColorRule::MapRuleInfoInto(nsRuleData* aRuleData)
{
  if (aRuleData->mSID == eStyleStruct_Color) {
    if (aRuleData->mColorData->mColor.GetUnit() == eCSSUnit_Null) {
      if (!mInitialized)
        Initialize(aRuleData->mPresContext);
      nsCSSValue val; val.SetColorValue(mColor);
      aRuleData->mColorData->mColor = val;
    }
  }
  return NS_OK;
}

void
HTMLDocumentColorRule::Initialize(nsIPresContext* aPresContext)
{
  aPresContext->GetDefaultColor(&mColor); // in case something below fails

  nsCOMPtr<nsIPresShell> shell;
  aPresContext->GetShell(getter_AddRefs(shell));
  nsCOMPtr<nsIDocument> doc;
  shell->GetDocument(getter_AddRefs(doc));
  nsCOMPtr<nsIDOMHTMLDocument> domdoc = do_QueryInterface(doc);
  if (!domdoc)
    return;
  nsCOMPtr<nsIDOMHTMLElement> body;
  domdoc->GetBody(getter_AddRefs(body));
  nsCOMPtr<nsIContent> bodyContent = do_QueryInterface(body);
  nsIFrame *bodyFrame;
  shell->GetPrimaryFrameFor(bodyContent, &bodyFrame);
  if (!bodyFrame)
    return;
  const nsStyleColor *bodyColor;
  ::GetStyleData(bodyFrame, &bodyColor);
  mColor = bodyColor->mColor;
}

#ifdef DEBUG
/******************************************************************************
* SizeOf method:
*
*  Self (reported as HTMLDocumentColorRule's size): 
*    1) sizeof(*this)
*
*  Contained / Aggregated data (not reported as HTMLDocumentColorRule's size):
*    1) Delegate to the mSheet
*
*  Children / siblings / parents:
*    none
*    
******************************************************************************/
void HTMLDocumentColorRule::SizeOf(nsISizeOfHandler *aSizeOfHandler, PRUint32 &aSize)
{
  NS_ASSERTION(aSizeOfHandler != nsnull, "SizeOf handler cannot be null");

  // first get the unique items collection
  UNIQUE_STYLE_ITEMS(uniqueItems);

  if(! uniqueItems->AddItem((void*)this) ){
    // object has already been accounted for
    return;
  }

  // get or create a tag for this instance
  nsCOMPtr<nsIAtom> tag;
  tag = getter_AddRefs(NS_NewAtom("HTMLDocumentColorRule"));
  // get the size of an empty instance and add to the sizeof handler
  aSize = sizeof(*this);
  aSizeOfHandler->AddSize(tag,aSize);

  if(mSheet){
    PRUint32 localSize;
    mSheet->SizeOf(aSizeOfHandler, localSize);
  }
}
#endif

class GenericTableRule: public nsIStyleRule {
public:
  GenericTableRule(nsIHTMLStyleSheet* aSheet);
  virtual ~GenericTableRule();

  NS_DECL_ISUPPORTS

  NS_IMETHOD Equals(const nsIStyleRule* aRule, PRBool& aResult) const;
  NS_IMETHOD HashValue(PRUint32& aValue) const;
  NS_IMETHOD GetStyleSheet(nsIStyleSheet*& aSheet) const;

  // Strength is an out-of-band weighting, always 0 here
  NS_IMETHOD GetStrength(PRInt32& aStrength) const;

  // The new mapping function.
  NS_IMETHOD MapRuleInfoInto(nsRuleData* aRuleData);

#ifdef DEBUG
  NS_IMETHOD List(FILE* out = stdout, PRInt32 aIndent = 0) const;

  virtual void SizeOf(nsISizeOfHandler *aSizeofHandler, PRUint32 &aSize);
#endif

  void Reset()
  {
  }

  nsIHTMLStyleSheet*  mSheet; // not ref-counted, cleared by content
};

GenericTableRule::GenericTableRule(nsIHTMLStyleSheet* aSheet)
{
  NS_INIT_REFCNT();
  mSheet = aSheet;
}

GenericTableRule::~GenericTableRule()
{
}

NS_IMPL_ISUPPORTS1(GenericTableRule, nsIStyleRule)

NS_IMETHODIMP
GenericTableRule::Equals(const nsIStyleRule* aRule, PRBool& aResult) const
{
  aResult = PRBool(this == aRule);
  return NS_OK;
}

NS_IMETHODIMP
GenericTableRule::HashValue(PRUint32& aValue) const
{
  aValue = 0;
  return NS_OK;
}

NS_IMETHODIMP
GenericTableRule::GetStyleSheet(nsIStyleSheet*& aSheet) const
{
  aSheet = mSheet;
  return NS_OK;
}

// Strength is an out-of-band weighting, useful for mapping CSS ! important
// always 0 here
NS_IMETHODIMP
GenericTableRule::GetStrength(PRInt32& aStrength) const
{
  aStrength = 0;
  return NS_OK;
}

NS_IMETHODIMP
GenericTableRule::MapRuleInfoInto(nsRuleData* aRuleData)
{
  // Nothing to do.
  return NS_OK;
}

#ifdef DEBUG
NS_IMETHODIMP
GenericTableRule::List(FILE* out, PRInt32 aIndent) const
{
  return NS_OK;
}

/******************************************************************************
* SizeOf method:
*
*  Self (reported as GenericTableRule's size): 
*    1) sizeof(*this) + 
*
*  Contained / Aggregated data (not reported as GenericTableRule's size):
*    1) Delegate to the mSheet if it exists
*
*  Children / siblings / parents:
*    none
*    
******************************************************************************/
void GenericTableRule::SizeOf(nsISizeOfHandler *aSizeOfHandler, PRUint32 &aSize)
{
  NS_ASSERTION(aSizeOfHandler != nsnull, "SizeOf handler cannot be null");

  // first get the unique items collection
  UNIQUE_STYLE_ITEMS(uniqueItems);

  if(! uniqueItems->AddItem((void*)this) ){
    // object has already been accounted for
    return;
  }

  // get or create a tag for this instance
  nsCOMPtr<nsIAtom> tag;
  tag = getter_AddRefs(NS_NewAtom("GenericTableRule"));
  // get the size of an empty instance and add to the sizeof handler
  aSize = sizeof(*this);
  aSizeOfHandler->AddSize(tag,aSize);

  if(mSheet){
    PRUint32 localSize;
    mSheet->SizeOf(aSizeOfHandler, localSize);
  }
}
#endif

// -----------------------------------------------------------
// this rule handles <th> inheritance
// -----------------------------------------------------------
class TableTHRule: public GenericTableRule {
public:
  TableTHRule(nsIHTMLStyleSheet* aSheet);
  virtual ~TableTHRule();

  void Reset()
  {
    GenericTableRule::Reset();
  }

  NS_IMETHOD MapRuleInfoInto(nsRuleData* aRuleData);
};

TableTHRule::TableTHRule(nsIHTMLStyleSheet* aSheet)
: GenericTableRule(aSheet)
{
}

TableTHRule::~TableTHRule()
{
}

static void PostResolveCallback(nsStyleStruct* aStyleStruct, nsRuleData* aRuleData)
{
  nsStyleText* text = (nsStyleText*)aStyleStruct;
  if (text->mTextAlign == NS_STYLE_TEXT_ALIGN_DEFAULT) {
    nsCOMPtr<nsIStyleContext> parentContext = getter_AddRefs(aRuleData->mStyleContext->GetParent());

    if (parentContext) {
      const nsStyleText* parentStyleText = 
          (const nsStyleText*)parentContext->GetStyleData(eStyleStruct_Text);
      PRUint8 parentAlign = parentStyleText->mTextAlign;
      text->mTextAlign = (NS_STYLE_TEXT_ALIGN_DEFAULT == parentAlign)
                              ? NS_STYLE_TEXT_ALIGN_CENTER : parentAlign;
    }
  }
}

NS_IMETHODIMP
TableTHRule::MapRuleInfoInto(nsRuleData* aRuleData)
{
  if (aRuleData && aRuleData->mSID == eStyleStruct_Text) {
    aRuleData->mCanStoreInRuleTree = PR_FALSE;
    aRuleData->mPostResolveCallback = &PostResolveCallback;
  }
  return NS_OK;
}

static void 
ProcessTableRulesAttribute(nsStyleStruct* aStyleStruct, 
                           nsRuleData*    aRuleData,
                           PRUint8        aSide,
                           PRBool         aGroup,
                           PRUint8        aRulesArg1,
                           PRUint8        aRulesArg2,
                           PRUint8        aRulesArg3)
{
  if (!aStyleStruct || !aRuleData || !aRuleData->mPresContext) return;

  nsCOMPtr<nsIStyleContext> tableContext = getter_AddRefs(aRuleData->mStyleContext->GetParent()); if (!tableContext) return;
  if (!aGroup) {
    tableContext = getter_AddRefs(tableContext->GetParent()); if (!tableContext) return;
  } 
  
  const nsStyleTable* tableData = 
    (const nsStyleTable*)tableContext->GetStyleData(eStyleStruct_Table);
  if (tableData && ((aRulesArg1 == tableData->mRules) ||
                    (aRulesArg2 == tableData->mRules) ||
                    (aRulesArg3 == tableData->mRules))) {
    const nsStyleBorder* tableBorderData = 
      (const nsStyleBorder*)tableContext->GetStyleData(eStyleStruct_Border); if (!tableBorderData) return;
    PRUint8 tableBorderStyle = tableBorderData->GetBorderStyle(aSide);

    nsStyleBorder* borderData = (nsStyleBorder*)aStyleStruct; if (!borderData) return;
    PRUint8 borderStyle = borderData->GetBorderStyle(aSide);
    // XXX It appears that the style system erronously applies the custom style rule after css style, 
    // consequently it does not properly fit into the casade. For now, assume that a border style of none
    // implies that the style has not been set.
    if (NS_STYLE_BORDER_STYLE_NONE == borderStyle) {
      // use the table's border style if it is dashed or dotted, otherwise use solid
      PRUint8 bStyle = ((NS_STYLE_BORDER_STYLE_NONE != tableBorderStyle) &&
                        (NS_STYLE_BORDER_STYLE_HIDDEN != tableBorderStyle)) 
                        ? tableBorderStyle : NS_STYLE_BORDER_STYLE_SOLID;
      if ((NS_STYLE_BORDER_STYLE_DASHED != bStyle) && 
          (NS_STYLE_BORDER_STYLE_DOTTED != bStyle) && 
          (NS_STYLE_BORDER_STYLE_SOLID  != bStyle)) {
        bStyle = NS_STYLE_BORDER_STYLE_SOLID;
      }
      bStyle |= NS_STYLE_BORDER_STYLE_RULES_MASK;
      borderData->SetBorderStyle(aSide, bStyle);

      nscolor borderColor;
      PRBool transparent, foreground;
      borderData->GetBorderColor(aSide, borderColor, transparent, foreground);
      if (transparent || foreground) {
        // use the table's border color if it is set, otherwise use black
        nscolor tableBorderColor;
        tableBorderData->GetBorderColor(aSide, tableBorderColor, transparent, foreground);
        borderColor = (transparent || foreground) ? NS_RGB(0,0,0) : tableBorderColor;
        borderData->SetBorderColor(aSide, borderColor);
      }
      // set the border width to be 1 pixel
      float p2t;
      aRuleData->mPresContext->GetScaledPixelsToTwips(&p2t);
      nscoord onePixel = NSToCoordRound(p2t);
      nsStyleCoord coord(onePixel);
      switch(aSide) {
      case NS_SIDE_TOP:
        borderData->mBorder.SetTop(coord);
        break;
      case NS_SIDE_RIGHT:
        borderData->mBorder.SetRight(coord);
        break;
      case NS_SIDE_BOTTOM:
        borderData->mBorder.SetBottom(coord);
        break;
      default: // NS_SIDE_LEFT
        borderData->mBorder.SetLeft(coord);
        break;
      }
    }
  }
}

// -----------------------------------------------------------
// this rule handles borders on a <thead>, <tbody>, <tfoot> when rules is set on its <table>
// -----------------------------------------------------------
class TableTbodyRule: public GenericTableRule {
public:
  TableTbodyRule(nsIHTMLStyleSheet* aSheet);
  virtual ~TableTbodyRule();

  void Reset()
  {
    GenericTableRule::Reset();
  }

  NS_IMETHOD MapRuleInfoInto(nsRuleData* aRuleData);
};

TableTbodyRule::TableTbodyRule(nsIHTMLStyleSheet* aSheet)
: GenericTableRule(aSheet)
{
}

TableTbodyRule::~TableTbodyRule()
{
}

static void TbodyPostResolveCallback(nsStyleStruct* aStyleStruct, nsRuleData* aRuleData)
{
  ::ProcessTableRulesAttribute(aStyleStruct, aRuleData, NS_SIDE_TOP, PR_TRUE, NS_STYLE_TABLE_RULES_ALL,
                               NS_STYLE_TABLE_RULES_GROUPS, NS_STYLE_TABLE_RULES_ROWS);
  ::ProcessTableRulesAttribute(aStyleStruct, aRuleData, NS_SIDE_BOTTOM, PR_TRUE, NS_STYLE_TABLE_RULES_ALL,
                               NS_STYLE_TABLE_RULES_GROUPS, NS_STYLE_TABLE_RULES_ROWS);
}

NS_IMETHODIMP
TableTbodyRule::MapRuleInfoInto(nsRuleData* aRuleData)
{
  if (aRuleData && aRuleData->mSID == eStyleStruct_Border) {
    aRuleData->mCanStoreInRuleTree = PR_FALSE;
    aRuleData->mPostResolveCallback = &TbodyPostResolveCallback;
  }
  return NS_OK;
}
// -----------------------------------------------------------

// -----------------------------------------------------------
// this rule handles borders on a <row> when rules is set on its <table>
// -----------------------------------------------------------
class TableRowRule: public GenericTableRule {
public:
  TableRowRule(nsIHTMLStyleSheet* aSheet);
  virtual ~TableRowRule();

  void Reset()
  {
    GenericTableRule::Reset();
  }

  NS_IMETHOD MapRuleInfoInto(nsRuleData* aRuleData);
};

TableRowRule::TableRowRule(nsIHTMLStyleSheet* aSheet)
: GenericTableRule(aSheet)
{
}

TableRowRule::~TableRowRule()
{
}

static void RowPostResolveCallback(nsStyleStruct* aStyleStruct, nsRuleData* aRuleData)
{
  ::ProcessTableRulesAttribute(aStyleStruct, aRuleData, NS_SIDE_TOP, PR_FALSE, NS_STYLE_TABLE_RULES_ALL,
                               NS_STYLE_TABLE_RULES_ROWS, NS_STYLE_TABLE_RULES_ROWS);
  ::ProcessTableRulesAttribute(aStyleStruct, aRuleData, NS_SIDE_BOTTOM, PR_FALSE, NS_STYLE_TABLE_RULES_ALL,
                               NS_STYLE_TABLE_RULES_ROWS, NS_STYLE_TABLE_RULES_ROWS);
}

NS_IMETHODIMP
TableRowRule::MapRuleInfoInto(nsRuleData* aRuleData)
{
  if (aRuleData && aRuleData->mSID == eStyleStruct_Border) {
    aRuleData->mCanStoreInRuleTree = PR_FALSE;
    aRuleData->mPostResolveCallback = &RowPostResolveCallback;
  }
  return NS_OK;
}

// -----------------------------------------------------------
// this rule handles borders on a <colgroup> when rules is set on its <table>
// -----------------------------------------------------------
class TableColgroupRule: public GenericTableRule {
public:
  TableColgroupRule(nsIHTMLStyleSheet* aSheet);
  virtual ~TableColgroupRule();

  void Reset()
  {
    GenericTableRule::Reset();
  }

  NS_IMETHOD MapRuleInfoInto(nsRuleData* aRuleData);
};

TableColgroupRule::TableColgroupRule(nsIHTMLStyleSheet* aSheet)
: GenericTableRule(aSheet)
{
}

TableColgroupRule::~TableColgroupRule()
{
}

static void ColgroupPostResolveCallback(nsStyleStruct* aStyleStruct, nsRuleData* aRuleData)
{
  ::ProcessTableRulesAttribute(aStyleStruct, aRuleData, NS_SIDE_LEFT, PR_TRUE, NS_STYLE_TABLE_RULES_ALL,
                               NS_STYLE_TABLE_RULES_GROUPS, NS_STYLE_TABLE_RULES_COLS);
  ::ProcessTableRulesAttribute(aStyleStruct, aRuleData, NS_SIDE_RIGHT, PR_TRUE, NS_STYLE_TABLE_RULES_ALL,
                               NS_STYLE_TABLE_RULES_GROUPS, NS_STYLE_TABLE_RULES_COLS);
}

NS_IMETHODIMP
TableColgroupRule::MapRuleInfoInto(nsRuleData* aRuleData)
{
  if (aRuleData && aRuleData->mSID == eStyleStruct_Border) {
    aRuleData->mCanStoreInRuleTree = PR_FALSE;
    aRuleData->mPostResolveCallback = &ColgroupPostResolveCallback;
  }
  return NS_OK;
}

// -----------------------------------------------------------
// this rule handles borders on a <col> when rules is set on its <table>
// -----------------------------------------------------------
class TableColRule: public GenericTableRule {
public:
  TableColRule(nsIHTMLStyleSheet* aSheet);
  virtual ~TableColRule();

  void Reset()
  {
    GenericTableRule::Reset();
  }

  NS_IMETHOD MapRuleInfoInto(nsRuleData* aRuleData);
};

TableColRule::TableColRule(nsIHTMLStyleSheet* aSheet)
: GenericTableRule(aSheet)
{
}

TableColRule::~TableColRule()
{
}

static void ColPostResolveCallback(nsStyleStruct* aStyleStruct, nsRuleData* aRuleData)
{
  ::ProcessTableRulesAttribute(aStyleStruct, aRuleData, NS_SIDE_LEFT, PR_FALSE, NS_STYLE_TABLE_RULES_ALL,
                               NS_STYLE_TABLE_RULES_COLS, NS_STYLE_TABLE_RULES_COLS);
  ::ProcessTableRulesAttribute(aStyleStruct, aRuleData, NS_SIDE_RIGHT, PR_FALSE, NS_STYLE_TABLE_RULES_ALL,
                               NS_STYLE_TABLE_RULES_COLS, NS_STYLE_TABLE_RULES_COLS);
}

NS_IMETHODIMP
TableColRule::MapRuleInfoInto(nsRuleData* aRuleData)
{
  if (aRuleData && aRuleData->mSID == eStyleStruct_Border) {
    aRuleData->mCanStoreInRuleTree = PR_FALSE;
    aRuleData->mPostResolveCallback = &ColPostResolveCallback;
  }
  return NS_OK;
}
// -----------------------------------------------------------

class AttributeKey: public nsHashKey
{
public:
  AttributeKey(nsIHTMLMappedAttributes* aAttributes);
  virtual ~AttributeKey(void);

  PRBool      Equals(const nsHashKey* aOther) const;
  PRUint32    HashCode(void) const;
  nsHashKey*  Clone(void) const;

private:
  AttributeKey(void);
  AttributeKey(const AttributeKey& aCopy);
  AttributeKey& operator=(const AttributeKey& aCopy);

public:
  nsIHTMLMappedAttributes*  mAttributes;
  union {
    struct {
      PRUint32  mHashSet: 1;
      PRUint32  mHashCode: 31;
    } mBits;
    PRUint32    mInitializer;
  } mHash;
};

MOZ_DECL_CTOR_COUNTER(AttributeKey)

AttributeKey::AttributeKey(nsIHTMLMappedAttributes* aAttributes)
  : mAttributes(aAttributes)
{
  MOZ_COUNT_CTOR(AttributeKey);
  NS_ADDREF(mAttributes);
  mHash.mInitializer = 0;
}

AttributeKey::~AttributeKey(void)
{
  MOZ_COUNT_DTOR(AttributeKey);
  NS_RELEASE(mAttributes);
}

PRBool AttributeKey::Equals(const nsHashKey* aOther) const
{
  const AttributeKey* other = (const AttributeKey*)aOther;
  PRBool  equals;
  mAttributes->Equals(other->mAttributes, equals);
  return equals;
}

PRUint32 AttributeKey::HashCode(void) const
{
  if (0 == mHash.mBits.mHashSet) {
    AttributeKey* self = (AttributeKey*)this; // break const
    PRUint32  hash;
    mAttributes->HashValue(hash);
    self->mHash.mBits.mHashCode = (0x7FFFFFFF & hash);
    self->mHash.mBits.mHashSet = 1;
  }
  return mHash.mBits.mHashCode;
}

nsHashKey* AttributeKey::Clone(void) const
{
  AttributeKey* clown = new AttributeKey(mAttributes);
  if (nsnull != clown) {
    clown->mHash.mInitializer = mHash.mInitializer;
  }
  return clown;
}

// -----------------------------------------------------------

class HTMLStyleSheetImpl : public nsIHTMLStyleSheet,
                           public nsIStyleRuleProcessor {
public:
  void* operator new(size_t size);
  void* operator new(size_t size, nsIArena* aArena);
  void operator delete(void* ptr);

  HTMLStyleSheetImpl(void);
  nsresult Init();

  NS_IMETHOD QueryInterface(const nsIID& aIID, void** aInstancePtr);
  NS_IMETHOD_(nsrefcnt) AddRef();
  NS_IMETHOD_(nsrefcnt) Release();

  // nsIStyleSheet api
  NS_IMETHOD GetURL(nsIURI*& aURL) const;
  NS_IMETHOD GetTitle(nsString& aTitle) const;
  NS_IMETHOD GetType(nsString& aType) const;
  NS_IMETHOD GetMediumCount(PRInt32& aCount) const;
  NS_IMETHOD GetMediumAt(PRInt32 aIndex, nsIAtom*& aMedium) const;
  NS_IMETHOD_(PRBool) UseForMedium(nsIAtom* aMedium) const;

  NS_IMETHOD GetEnabled(PRBool& aEnabled) const;
  NS_IMETHOD SetEnabled(PRBool aEnabled);

  // style sheet owner info
  NS_IMETHOD GetParentSheet(nsIStyleSheet*& aParent) const;  // will be null
  NS_IMETHOD GetOwningDocument(nsIDocument*& aDocument) const;

  NS_IMETHOD SetOwningDocument(nsIDocument* aDocumemt);

  NS_IMETHOD GetStyleRuleProcessor(nsIStyleRuleProcessor*& aProcessor,
                                   nsIStyleRuleProcessor* aPrevProcessor);

  // nsIStyleRuleProcessor API
  NS_IMETHOD RulesMatching(ElementRuleProcessorData* aData,
                           nsIAtom* aMedium);

  NS_IMETHOD RulesMatching(PseudoRuleProcessorData* aData,
                           nsIAtom* aMedium);

  NS_IMETHOD HasStateDependentStyle(StateRuleProcessorData* aData,
                                    nsIAtom* aMedium,
                                    PRBool* aResult);

  // nsIHTMLStyleSheet api
  NS_IMETHOD Init(nsIURI* aURL, nsIDocument* aDocument);
  NS_IMETHOD Reset(nsIURI* aURL);
  NS_IMETHOD GetLinkColor(nscolor& aColor);
  NS_IMETHOD GetActiveLinkColor(nscolor& aColor);
  NS_IMETHOD GetVisitedLinkColor(nscolor& aColor);
  NS_IMETHOD SetLinkColor(nscolor aColor);
  NS_IMETHOD SetActiveLinkColor(nscolor aColor);
  NS_IMETHOD SetVisitedLinkColor(nscolor aColor);

  // Mapped Attribute management methods
  NS_IMETHOD UniqueMappedAttributes(nsIHTMLMappedAttributes* aMapped,
                                    nsIHTMLMappedAttributes*& aUniqueMapped);
  NS_IMETHOD DropMappedAttributes(nsIHTMLMappedAttributes* aMapped);

#ifdef DEBUG
  virtual void List(FILE* out = stdout, PRInt32 aIndent = 0) const;

  virtual void SizeOf(nsISizeOfHandler *aSizeofHandler, PRUint32 &aSize);
#endif

  // If changing the given attribute cannot affect style context, aAffects
  // will be PR_FALSE on return.
  NS_IMETHOD AttributeAffectsStyle(nsIAtom *aAttribute, nsIContent *aContent,
                                   PRBool &aAffects);
private: 
  // These are not supported and are not implemented! 
  HTMLStyleSheetImpl(const HTMLStyleSheetImpl& aCopy); 
  HTMLStyleSheetImpl& operator=(const HTMLStyleSheetImpl& aCopy); 

protected:
  virtual ~HTMLStyleSheetImpl();

protected:
  PRUint32 mInHeap : 1;
  PRUint32 mRefCnt : 31;
  NS_DECL_OWNINGTHREAD // for thread-safety checking

  nsIURI*              mURL;
  nsIDocument*         mDocument;
  HTMLColorRule*       mLinkRule;
  HTMLColorRule*       mVisitedRule;
  HTMLColorRule*       mActiveRule;
  HTMLDocumentColorRule* mDocumentColorRule;
  TableTbodyRule*      mTableTbodyRule;
  TableRowRule*        mTableRowRule;
  TableColgroupRule*   mTableColgroupRule;
  TableColRule*        mTableColRule;
  TableTHRule*         mTableTHRule;
    // NOTE: if adding more rules, be sure to update 
    // the SizeOf method to include them

  nsHashtable          mMappedAttrTable;
};


void* HTMLStyleSheetImpl::operator new(size_t size)
{
  HTMLStyleSheetImpl* rv = (HTMLStyleSheetImpl*) ::operator new(size);
#ifdef NS_DEBUG
  if (nsnull != rv) {
    memset(rv, 0xEE, size);
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
    memset(rv, 0xEE, size);
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
      ::operator delete(ptr);
    }
  }
}

HTMLStyleSheetImpl::HTMLStyleSheetImpl(void)
  : nsIHTMLStyleSheet(),
    mURL(nsnull),
    mDocument(nsnull),
    mLinkRule(nsnull),
    mVisitedRule(nsnull),
    mActiveRule(nsnull),
    mDocumentColorRule(nsnull)
{
  NS_INIT_REFCNT();
}

nsresult
HTMLStyleSheetImpl::Init()
{
  mTableTbodyRule = new TableTbodyRule(this);
  if (!mTableTbodyRule)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(mTableTbodyRule);

  mTableRowRule = new TableRowRule(this);
  if (!mTableRowRule)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(mTableRowRule);

  mTableColgroupRule = new TableColgroupRule(this);
  if (!mTableColgroupRule)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(mTableColgroupRule);

  mTableColRule = new TableColRule(this);
  if (!mTableColRule)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(mTableColRule);

  mTableTHRule = new TableTHRule(this);
  if (!mTableTHRule)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(mTableTHRule);

  mDocumentColorRule = new HTMLDocumentColorRule(this);
  if (!mDocumentColorRule)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(mDocumentColorRule);

  return NS_OK;
}

PR_STATIC_CALLBACK(PRBool)
MappedDropSheet(nsHashKey *aKey, void *aData, void* closure)
{
  nsIHTMLMappedAttributes* mapped = (nsIHTMLMappedAttributes*)aData;
  mapped->DropStyleSheetReference();
  return PR_TRUE;
}

HTMLStyleSheetImpl::~HTMLStyleSheetImpl()
{
  NS_IF_RELEASE(mURL);
  if (nsnull != mLinkRule) {
    mLinkRule->mSheet = nsnull;
    NS_RELEASE(mLinkRule);
  }
  if (nsnull != mVisitedRule) {
    mVisitedRule->mSheet = nsnull;
    NS_RELEASE(mVisitedRule);
  }
  if (nsnull != mActiveRule) {
    mActiveRule->mSheet = nsnull;
    NS_RELEASE(mActiveRule);
  }
  if (nsnull != mDocumentColorRule) {
    mDocumentColorRule->mSheet = nsnull;
    NS_RELEASE(mDocumentColorRule);
  }
  if (nsnull != mTableTbodyRule) {
    mTableTbodyRule->mSheet = nsnull;
    NS_RELEASE(mTableTbodyRule);
  }
  if (nsnull != mTableRowRule) {
    mTableRowRule->mSheet = nsnull;
    NS_RELEASE(mTableRowRule);
  }
  if (nsnull != mTableColgroupRule) {
    mTableColgroupRule->mSheet = nsnull;
    NS_RELEASE(mTableColgroupRule);
  }
  if (nsnull != mTableColRule) {
    mTableColRule->mSheet = nsnull;
    NS_RELEASE(mTableColRule);
  }
  if (nsnull != mTableTHRule) {
    mTableTHRule->mSheet = nsnull;
    NS_RELEASE(mTableTHRule);
  }
  mMappedAttrTable.Enumerate(MappedDropSheet);
}

NS_IMPL_ADDREF(HTMLStyleSheetImpl)
NS_IMPL_RELEASE(HTMLStyleSheetImpl)

nsresult HTMLStyleSheetImpl::QueryInterface(const nsIID& aIID,
                                            void** aInstancePtrResult)
{
  NS_PRECONDITION(aInstancePtrResult, "null out param");

  if (aIID.Equals(NS_GET_IID(nsIHTMLStyleSheet))) {
    *aInstancePtrResult = NS_STATIC_CAST(nsIHTMLStyleSheet*, this);
  } else if (aIID.Equals(NS_GET_IID(nsIStyleSheet))) {
    *aInstancePtrResult = NS_STATIC_CAST(nsIStyleSheet *, this);
  } else if (aIID.Equals(NS_GET_IID(nsIStyleRuleProcessor))) {
    *aInstancePtrResult = NS_STATIC_CAST(nsIStyleRuleProcessor *, this);
  } else if (aIID.Equals(NS_GET_IID(nsIStyleFrameConstruction))) {
    // XXX this breaks XPCOM rules since it isn't a proper delegate
    // This is a temporary method of connecting the constructor for
    // now
    nsresult rv;
    nsCOMPtr<nsICSSFrameConstructor> constructor =
      do_CreateInstance(kCSSFrameConstructorCID, &rv);

    if (NS_SUCCEEDED(rv)) {
      rv = constructor->Init(mDocument);

      if (NS_SUCCEEDED(rv)) {
        rv = constructor->QueryInterface(aIID, aInstancePtrResult);
      }
    }

    return rv;
  } else if (aIID.Equals(NS_GET_IID(nsISupports))) {
    *aInstancePtrResult = NS_STATIC_CAST(nsIHTMLStyleSheet *, this);
  } else {
    *aInstancePtrResult = nsnull;

    return NS_NOINTERFACE;
  }

  NS_ADDREF_THIS();

  return NS_OK;
}

NS_IMETHODIMP
HTMLStyleSheetImpl::GetStyleRuleProcessor(nsIStyleRuleProcessor*& aProcessor,
                                          nsIStyleRuleProcessor* /*aPrevProcessor*/)
{
  aProcessor = this;
  NS_ADDREF(aProcessor);
  return NS_OK;
}

NS_IMETHODIMP
HTMLStyleSheetImpl::RulesMatching(ElementRuleProcessorData* aData,
                                  nsIAtom* aMedium)
{
  nsIStyledContent *styledContent = aData->mStyledContent;

  if (styledContent) {
    nsRuleWalker *ruleWalker = aData->mRuleWalker;
    if (styledContent->IsContentOfType(nsIContent::eHTML)) {
      nsIAtom* tag = aData->mContentTag;

      // if we have anchor colors, check if this is an anchor with an href
      if (tag == nsHTMLAtoms::a) {
        if (mLinkRule || mVisitedRule || mActiveRule) {
          if (aData->mIsHTMLLink) {
            switch (aData->mLinkState) {
              case eLinkState_Unvisited:
                if (mLinkRule)
                  ruleWalker->Forward(mLinkRule);
                break;
              case eLinkState_Visited:
                if (mVisitedRule)
                  ruleWalker->Forward(mVisitedRule);
                break;
              default:
                break;
            }

            // No need to add to the active rule if it's not a link
            if (mActiveRule && (aData->mEventState & NS_EVENT_STATE_ACTIVE))
              ruleWalker->Forward(mActiveRule);
          }
        } // end link/visited/active rules
      } // end A tag
      // add the rule to handle text-align for a <th>
      else if (tag == nsHTMLAtoms::th) {
        ruleWalker->Forward(mTableTHRule);
      }
      else if (tag == nsHTMLAtoms::tr) {
        ruleWalker->Forward(mTableRowRule);
      }
      else if ((tag == nsHTMLAtoms::thead) || (tag == nsHTMLAtoms::tbody) || (tag == nsHTMLAtoms::tfoot)) {
        ruleWalker->Forward(mTableTbodyRule);
      }
      else if (tag == nsHTMLAtoms::col) {
        ruleWalker->Forward(mTableColRule);
      }
      else if (tag == nsHTMLAtoms::colgroup) {
        ruleWalker->Forward(mTableColgroupRule);
      }
      else if (tag == nsHTMLAtoms::table) {
        if (aData->mCompatMode == eCompatibility_NavQuirks)
          ruleWalker->Forward(mDocumentColorRule);
      }
    } // end html element

    // just get the style rules from the content
    styledContent->WalkContentStyleRules(ruleWalker);
  }

  return NS_OK;
}

// Test if style is dependent on content state
NS_IMETHODIMP
HTMLStyleSheetImpl::HasStateDependentStyle(StateRuleProcessorData* aData,
                                           nsIAtom* aMedium,
                                           PRBool* aResult)
{
  *aResult = mActiveRule &&
             (aData->mStateMask & NS_EVENT_STATE_ACTIVE) &&
             aData->mStyledContent &&
             aData->mIsHTMLContent &&
             aData->mContentTag == nsHTMLAtoms::a &&
             aData->mStyledContent->HasAttr(kNameSpaceID_None,
                                            nsHTMLAtoms::href);

  return NS_OK;
}


NS_IMETHODIMP
HTMLStyleSheetImpl::RulesMatching(PseudoRuleProcessorData* aData,
                                  nsIAtom* aMedium)
{
  nsIAtom* pseudoTag = aData->mPseudoTag;
  if (pseudoTag == nsHTMLAtoms::tableColPseudo) {
    nsRuleWalker *ruleWalker = aData->mRuleWalker;
    if (ruleWalker) {
      ruleWalker->Forward(mTableColRule);
    }
  }
  return NS_OK;
}


  // nsIStyleSheet api
NS_IMETHODIMP
HTMLStyleSheetImpl::GetURL(nsIURI*& aURL) const
{
  aURL = mURL;
  NS_IF_ADDREF(aURL);
  return NS_OK;
}

NS_IMETHODIMP
HTMLStyleSheetImpl::GetTitle(nsString& aTitle) const
{
  aTitle.Truncate();
  return NS_OK;
}

NS_IMETHODIMP
HTMLStyleSheetImpl::GetType(nsString& aType) const
{
  aType.Assign(NS_LITERAL_STRING("text/html"));
  return NS_OK;
}

NS_IMETHODIMP
HTMLStyleSheetImpl::GetMediumCount(PRInt32& aCount) const
{
  aCount = 0;
  return NS_OK;
}

NS_IMETHODIMP
HTMLStyleSheetImpl::GetMediumAt(PRInt32 aIndex, nsIAtom*& aMedium) const
{
  aMedium = nsnull;
  return NS_ERROR_INVALID_ARG;
}

NS_IMETHODIMP_(PRBool)
HTMLStyleSheetImpl::UseForMedium(nsIAtom* aMedium) const
{
  return PR_TRUE; // works for all media
}


NS_IMETHODIMP
HTMLStyleSheetImpl::GetEnabled(PRBool& aEnabled) const
{
  aEnabled = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
HTMLStyleSheetImpl::SetEnabled(PRBool aEnabled)
{ // these can't be disabled
  return NS_OK;
}

NS_IMETHODIMP
HTMLStyleSheetImpl::GetParentSheet(nsIStyleSheet*& aParent) const
{
  aParent = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
HTMLStyleSheetImpl::GetOwningDocument(nsIDocument*& aDocument) const
{
  aDocument = mDocument;
  NS_IF_ADDREF(aDocument);
  return NS_OK;
}

NS_IMETHODIMP
HTMLStyleSheetImpl::SetOwningDocument(nsIDocument* aDocument)
{
  mDocument = aDocument; // not refcounted
  return NS_OK;
}

NS_IMETHODIMP
HTMLStyleSheetImpl::Init(nsIURI* aURL, nsIDocument* aDocument)
{
  NS_PRECONDITION(aURL && aDocument, "null ptr");
  if (! aURL || ! aDocument)
    return NS_ERROR_NULL_POINTER;

  if (mURL || mDocument)
    return NS_ERROR_ALREADY_INITIALIZED;

  mDocument = aDocument; // not refcounted!
  mURL = aURL;
  NS_ADDREF(mURL);
  return NS_OK;
}

NS_IMETHODIMP
HTMLStyleSheetImpl::Reset(nsIURI* aURL)
{
  NS_IF_RELEASE(mURL);
  mURL = aURL;
  NS_ADDREF(mURL);

  if (mLinkRule) {
    mLinkRule->mSheet = nsnull;
    NS_RELEASE(mLinkRule);
  }
  if (mVisitedRule) {
    mVisitedRule->mSheet = nsnull;
    NS_RELEASE(mVisitedRule);
  }
  if (mActiveRule) {
    mActiveRule->mSheet = nsnull;
    NS_RELEASE(mActiveRule);
  }
  mDocumentColorRule->Reset();

  mTableTbodyRule->Reset();
  mTableRowRule->Reset();
  mTableColgroupRule->Reset();
  mTableColRule->Reset();
  mTableTHRule->Reset();

  mMappedAttrTable.Enumerate(MappedDropSheet);
  mMappedAttrTable.Reset();

  return NS_OK;
}

NS_IMETHODIMP
HTMLStyleSheetImpl::GetLinkColor(nscolor& aColor)
{
  if (!mLinkRule) {
    return NS_HTML_STYLE_PROPERTY_NOT_THERE;
  }
  else {
    aColor = mLinkRule->mColor;
    return NS_OK;
  }
}

NS_IMETHODIMP
HTMLStyleSheetImpl::GetActiveLinkColor(nscolor& aColor)
{
  if (!mActiveRule) {
    return NS_HTML_STYLE_PROPERTY_NOT_THERE;
  }
  else {
    aColor = mActiveRule->mColor;
    return NS_OK;
  }
}

NS_IMETHODIMP
HTMLStyleSheetImpl::GetVisitedLinkColor(nscolor& aColor)
{
  if (!mVisitedRule) {
    return NS_HTML_STYLE_PROPERTY_NOT_THERE;
  }
  else {
    aColor = mVisitedRule->mColor;
    return NS_OK;
  }
}

NS_IMETHODIMP
HTMLStyleSheetImpl::SetLinkColor(nscolor aColor)
{
  if (!mLinkRule) {
    mLinkRule = new HTMLColorRule(this);
    if (!mLinkRule) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    NS_ADDREF(mLinkRule);
  }
  mLinkRule->mColor = aColor;
  return NS_OK;
}


NS_IMETHODIMP
HTMLStyleSheetImpl::SetActiveLinkColor(nscolor aColor)
{
  if (!mActiveRule) {
    mActiveRule = new HTMLColorRule(this);
    if (!mActiveRule) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    NS_ADDREF(mActiveRule);
  }
  mActiveRule->mColor = aColor;
  return NS_OK;
}

NS_IMETHODIMP
HTMLStyleSheetImpl::SetVisitedLinkColor(nscolor aColor)
{
  if (!mVisitedRule) {
    mVisitedRule = new HTMLColorRule(this);
    if (!mVisitedRule) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    NS_ADDREF(mVisitedRule);
  }
  mVisitedRule->mColor = aColor;
  return NS_OK;
}

NS_IMETHODIMP
HTMLStyleSheetImpl::UniqueMappedAttributes(nsIHTMLMappedAttributes* aMapped,
                                           nsIHTMLMappedAttributes*& aUniqueMapped)
{
  nsresult result = NS_OK;

  AttributeKey key(aMapped);
  nsIHTMLMappedAttributes* sharedAttrs =
    (nsIHTMLMappedAttributes*)mMappedAttrTable.Get(&key);

  if (!sharedAttrs) {  // we have a new unique set
    mMappedAttrTable.Put(&key, aMapped);
    aMapped->SetUniqued(PR_TRUE);
    NS_ADDREF(aMapped);
    aUniqueMapped = aMapped;
  }
  else {  // found existing set
    aUniqueMapped = sharedAttrs;
    NS_ADDREF(aUniqueMapped);
  }
  return result;
}

NS_IMETHODIMP
HTMLStyleSheetImpl::DropMappedAttributes(nsIHTMLMappedAttributes* aMapped)
{
  if (aMapped) {
    PRBool inTable = PR_FALSE;
    aMapped->GetUniqued(inTable);
    if (inTable) {
      AttributeKey key(aMapped);
      nsIHTMLMappedAttributes* old =
        (nsIHTMLMappedAttributes*)mMappedAttrTable.Remove(&key);
      NS_ASSERTION(old == aMapped, "not in table");

      aMapped->SetUniqued(PR_FALSE);
    }
  }
  return NS_OK;
}

#ifdef DEBUG
void HTMLStyleSheetImpl::List(FILE* out, PRInt32 aIndent) const
{
  // Indent
  for (PRInt32 index = aIndent; --index >= 0; ) fputs("  ", out);

  fputs("HTML Style Sheet: ", out);
  nsCAutoString urlSpec;
  mURL->GetSpec(urlSpec);
  if (!urlSpec.IsEmpty()) {
    fputs(urlSpec.get(), out);
  }
  fputs("\n", out);
}


struct MappedAttributeSizeEnumData
{
  MappedAttributeSizeEnumData(nsISizeOfHandler *aSizeOfHandler, 
                              nsUniqueStyleItems *aUniqueStyleItem)
  {
    aHandler = aSizeOfHandler;
    uniqueItems = aUniqueStyleItem;
  }

  // weak references all 'round
  nsISizeOfHandler  *aHandler;
  nsUniqueStyleItems *uniqueItems;
};

PR_STATIC_CALLBACK(PRBool)
MappedSizeAttributes(nsHashKey *aKey, void *aData, void* closure)
{
  MappedAttributeSizeEnumData *pData = (MappedAttributeSizeEnumData *)closure;
  NS_ASSERTION(pData,"null closure is not supported");
  nsIHTMLMappedAttributes* mapped = (nsIHTMLMappedAttributes*)aData;
  NS_ASSERTION(mapped, "null item in enumeration fcn is not supported");
  // if there is an attribute and it has not been counted, the get its size
  if(mapped){
    PRUint32 size=0;
    mapped->SizeOf(pData->aHandler, size);
  }  
  return PR_TRUE;
}


/******************************************************************************
* SizeOf method:
*
*  Self (reported as HTMLStyleSheetImpl's size): 
*    1) sizeof(*this)
*
*  Contained / Aggregated data (not reported as HTMLStyleSheetImpl's size):
*    1) Not really delegated, but counted seperately:
*       - mLinkRule
*       - mVisitedRule
*       - mActiveRule
*       - mDocumentColorRule
*       - mTableTbodyRule
*       - mTableRowRule
*       - mTableColgroupRule
*       - mTableColRule
*       - mTableTHRule
*       - mMappedAttrTable
*    2) Delegates (really) to the MappedAttributes in the mMappedAttrTable
*
*  Children / siblings / parents:
*    none
*    
******************************************************************************/
void
HTMLStyleSheetImpl::SizeOf(nsISizeOfHandler *aSizeOfHandler, PRUint32 &aSize)
{
  NS_ASSERTION(aSizeOfHandler != nsnull, "SizeOf handler cannot be null");

  // first get the unique items collection
  UNIQUE_STYLE_ITEMS(uniqueItems);
  if(! uniqueItems->AddItem((void*)this)){
    // this style sheet is lared accounted for
    return;
  }

  PRUint32 localSize=0;

  // create a tag for this instance
  nsCOMPtr<nsIAtom> tag;
  tag = getter_AddRefs(NS_NewAtom("HTMLStyleSheet"));
  // get the size of an empty instance and add to the sizeof handler
  aSize = sizeof(HTMLStyleSheetImpl);
  aSizeOfHandler->AddSize(tag,aSize);

  // now gather up the sizes of the data members
  // - mLinkRule : sizeof object
  // - mVisitedRule  : sizeof object
  // - mActiveRule  : sizeof object
  // - mDocumentColorRule  : sizeof object
  // - mTableTbodyRule : sizeof object
  // - mTableRowRule : sizeof object
  // - mTableColgroupRule : sizeof object
  // - mTableColRule : sizeof object
  // - mTableTHRule : sizeof object
  // - mMappedAttrTable

  if(mLinkRule && uniqueItems->AddItem((void*)mLinkRule)){
    localSize = sizeof(*mLinkRule);
    tag = getter_AddRefs(NS_NewAtom("LinkRule"));
    aSizeOfHandler->AddSize(tag,localSize);
  }
  if(mVisitedRule && uniqueItems->AddItem((void*)mVisitedRule)){
    localSize = sizeof(*mVisitedRule);
    tag = getter_AddRefs(NS_NewAtom("VisitedRule"));
    aSizeOfHandler->AddSize(tag,localSize);
  }
  if(mActiveRule && uniqueItems->AddItem((void*)mActiveRule)){
    localSize = sizeof(*mActiveRule);
    tag = getter_AddRefs(NS_NewAtom("ActiveRule"));
    aSizeOfHandler->AddSize(tag,localSize);
  }
  if(uniqueItems->AddItem((void*)mDocumentColorRule)){
    localSize = sizeof(*mDocumentColorRule);
    tag = getter_AddRefs(NS_NewAtom("DocumentColorRule"));
    aSizeOfHandler->AddSize(tag,localSize);
  }
  if(uniqueItems->AddItem((void*)mTableTbodyRule)){
    localSize = sizeof(*mTableTbodyRule);
    tag = getter_AddRefs(NS_NewAtom("TableTbodyRule"));
    aSizeOfHandler->AddSize(tag,localSize);
  }
  if(uniqueItems->AddItem((void*)mTableRowRule)){
    localSize = sizeof(*mTableRowRule);
    tag = getter_AddRefs(NS_NewAtom("TableRowRule"));
    aSizeOfHandler->AddSize(tag,localSize);
  }
  if(uniqueItems->AddItem((void*)mTableColgroupRule)){
    localSize = sizeof(*mTableColgroupRule);
    tag = getter_AddRefs(NS_NewAtom("TableColgroupRule"));
    aSizeOfHandler->AddSize(tag,localSize);
  }
  if(uniqueItems->AddItem((void*)mTableColRule)){
    localSize = sizeof(*mTableColRule);
    tag = getter_AddRefs(NS_NewAtom("TableColRule"));
    aSizeOfHandler->AddSize(tag,localSize);
  }
  if(uniqueItems->AddItem((void*)mTableTHRule)){
    localSize = sizeof(*mTableTHRule);
    tag = getter_AddRefs(NS_NewAtom("TableTHRule"));
    aSizeOfHandler->AddSize(tag,localSize);
  }
  
  // for the AttrTable it is kindof sleezy: 
  //  We want the hash table overhead as well as the entries it contains
  //
  //  we get the overall size of the hashtable, and if there are entries,
  //  we calculate a rough overhead estimate as:
  //   number of entries X sizeof each hash-entry 
  //   + the size of a hash table (see plhash.h and nsHashTable.h)
  //  then we add up the size of each unique attribute
  localSize = sizeof(mMappedAttrTable);
  if(mMappedAttrTable.Count() >0){
    localSize += sizeof(PLHashTable);
    localSize += mMappedAttrTable.Count() * sizeof(PLHashEntry);
  }
  tag = getter_AddRefs(NS_NewAtom("MappedAttrTable"));
  aSizeOfHandler->AddSize(tag,localSize);

  // now get each unique attribute
  MappedAttributeSizeEnumData sizeEnumData(aSizeOfHandler, uniqueItems);
  mMappedAttrTable.Enumerate(MappedSizeAttributes, &sizeEnumData);

  // that's it
}
#endif

NS_IMETHODIMP
HTMLStyleSheetImpl::AttributeAffectsStyle(nsIAtom *aAttribute,
                                          nsIContent *aContent,
                                          PRBool &aAffects)
{
  // XXX we should be checking to see if this is an href on an <A> being
  // XXX tweaked, in which case we really want to restyle
  aAffects = PR_FALSE;
  return NS_OK;
}

// XXX For convenience and backwards compatibility
NS_EXPORT nsresult
NS_NewHTMLStyleSheet(nsIHTMLStyleSheet** aInstancePtrResult, nsIURI* aURL, 
                     nsIDocument* aDocument)
{
  nsresult rv;
  nsIHTMLStyleSheet* sheet;
  if (NS_FAILED(rv = NS_NewHTMLStyleSheet(&sheet)))
    return rv;

  if (NS_FAILED(rv = sheet->Init(aURL, aDocument))) {
    NS_RELEASE(sheet);
    return rv;
  }

  *aInstancePtrResult = sheet;
  return NS_OK;
}


NS_EXPORT nsresult
NS_NewHTMLStyleSheet(nsIHTMLStyleSheet** aInstancePtrResult)
{
  NS_ASSERTION(aInstancePtrResult, "null out param");

  HTMLStyleSheetImpl *it = new HTMLStyleSheetImpl();
  if (!it) {
    *aInstancePtrResult = nsnull;
    return NS_ERROR_OUT_OF_MEMORY;
  }

  NS_ADDREF(it);
  nsresult rv = it->Init();
  if (NS_FAILED(rv))
    NS_RELEASE(it);

  *aInstancePtrResult = it; // NS_ADDREF above, or set to null by NS_RELEASE
  return rv;
}
