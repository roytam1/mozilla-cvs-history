/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License.  You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are Copyright (C) 1998
 * Netscape Communications Corporation.  All Rights Reserved.
 */
#include "nsGenericHTMLElement.h"

#include "nsIAtom.h"
#include "nsICSSParser.h"
#include "nsICSSStyleRule.h"
#include "nsICSSDeclaration.h"
#include "nsIDocument.h"
#include "nsIDOMAttr.h"
#include "nsIDOMEventReceiver.h"
#include "nsIDOMNamedNodeMap.h"
#include "nsIDOMNodeList.h"
#include "nsIEventListenerManager.h"
#include "nsIHTMLAttributes.h"
#include "nsIHTMLStyleSheet.h"
#include "nsIHTMLDocument.h"
#include "nsIHTMLContent.h"
#include "nsILinkHandler.h"
#include "nsIScriptContextOwner.h"
#include "nsIScriptGlobalObject.h"
#include "nsIScriptObjectOwner.h"
#include "nsISizeOfHandler.h"
#include "nsIStyleContext.h"
#include "nsIStyleRule.h"
#include "nsISupportsArray.h"
#include "nsIURL.h"
#include "nsStyleConsts.h"
#include "nsXIFConverter.h"
#include "nsFrame.h"
#include "nsIPresShell.h"
#include "nsIView.h"
#include "nsIViewManager.h"

#include "nsIXMLDocument.h"
#include "nsIXMLContent.h"

#include "nsHTMLParts.h"
#include "nsString.h"
#include "nsHTMLAtoms.h"
#include "nsDOMEventsIIDs.h"
#include "nsIEventStateManager.h"
#include "nsDOMEvent.h"
#include "nsIPrivateDOMEvent.h"
#include "nsDOMCID.h"
#include "nsIServiceManager.h"
#include "nsIDOMScriptObjectFactory.h"
#include "nsIDOMCSSStyleDeclaration.h"
#include "nsDOMCSSDeclaration.h"
#include "prprf.h"
#include "prmem.h"

// XXX todo: add in missing out-of-memory checks
NS_DEFINE_IID(kIDOMHTMLElementIID, NS_IDOMHTMLELEMENT_IID);

static NS_DEFINE_IID(kIDOMAttrIID, NS_IDOMATTR_IID);
static NS_DEFINE_IID(kIDOMNamedNodeMapIID, NS_IDOMNAMEDNODEMAP_IID);
static NS_DEFINE_IID(kIPrivateDOMEventIID, NS_IPRIVATEDOMEVENT_IID);
static NS_DEFINE_IID(kIStyleRuleIID, NS_ISTYLE_RULE_IID);
static NS_DEFINE_IID(kIHTMLDocumentIID, NS_IHTMLDOCUMENT_IID);
static NS_DEFINE_IID(kICSSStyleRuleIID, NS_ICSS_STYLE_RULE_IID);
static NS_DEFINE_IID(kIDOMNodeListIID, NS_IDOMNODELIST_IID);
static NS_DEFINE_IID(kIDOMCSSStyleDeclarationIID, NS_IDOMCSSSTYLEDECLARATION_IID);
static NS_DEFINE_IID(kIDOMDocumentIID, NS_IDOMNODE_IID);
static NS_DEFINE_IID(kIXMLContentIID, NS_IXMLCONTENT_IID);
static NS_DEFINE_IID(kIXMLDocumentIID, NS_IXMLDOCUMENT_IID);

//----------------------------------------------------------------------

class nsDOMCSSAttributeDeclaration : public nsDOMCSSDeclaration 
{
public:
  nsDOMCSSAttributeDeclaration(nsIHTMLContent *aContent);
  ~nsDOMCSSAttributeDeclaration();

  virtual void DropReference();
  virtual nsresult GetCSSDeclaration(nsICSSDeclaration **aDecl,
                                     PRBool aAllocate);
  virtual nsresult StylePropertyChanged(const nsString& aPropertyName,
                                        PRInt32 aHint);
  virtual nsresult GetParent(nsISupports **aParent);

protected:
  nsIHTMLContent *mContent;  
};

nsDOMCSSAttributeDeclaration::nsDOMCSSAttributeDeclaration(nsIHTMLContent *aContent)
{
  // This reference is not reference-counted. The content
  // object tells us when its about to go away.
  mContent = aContent;
}

nsDOMCSSAttributeDeclaration::~nsDOMCSSAttributeDeclaration()
{
}

void 
nsDOMCSSAttributeDeclaration::DropReference()
{
  mContent = nsnull;
}

nsresult
nsDOMCSSAttributeDeclaration::GetCSSDeclaration(nsICSSDeclaration **aDecl,
                                                  PRBool aAllocate)
{
  nsHTMLValue val;
  nsIStyleRule* rule;
  nsICSSStyleRule*  cssRule;
  nsresult result = NS_OK;

  *aDecl = nsnull;
  if (nsnull != mContent) {
    mContent->GetAttribute(nsHTMLAtoms::style, val);
    if (eHTMLUnit_ISupports == val.GetUnit()) {
      rule = (nsIStyleRule*) val.GetISupportsValue();
      result = rule->QueryInterface(kICSSStyleRuleIID, (void**)&cssRule);
      if (NS_OK == result) {
        *aDecl = cssRule->GetDeclaration();
        NS_RELEASE(cssRule);
      }      
      NS_RELEASE(rule);
    }
    else if (PR_TRUE == aAllocate) {
      result = NS_NewCSSDeclaration(aDecl);
      if (NS_OK == result) {
        result = NS_NewCSSStyleRule(&cssRule, nsCSSSelector());
        if (NS_OK == result) {
          cssRule->SetDeclaration(*aDecl);
          cssRule->SetWeight(0x7fffffff);
          rule = (nsIStyleRule *)cssRule;
          result = mContent->SetAttribute(nsHTMLAtoms::style, 
                                          nsHTMLValue(cssRule), 
                                          PR_FALSE);
          NS_RELEASE(cssRule);
        }
        else {
          NS_RELEASE(*aDecl);
        }
      }
    }
  }

  return result;
}

nsresult 
nsDOMCSSAttributeDeclaration::StylePropertyChanged(const nsString& aPropertyName,
                                                   PRInt32 aHint)
{
  nsresult result = NS_OK;
  if (nsnull != mContent) {
    nsIDocument *doc;
    result = mContent->GetDocument(doc);
    if (NS_SUCCEEDED(result) && (nsnull != doc)) {
      result = doc->AttributeChanged(mContent, nsHTMLAtoms::style, aHint);
      NS_RELEASE(doc);
    }
  }
  
  return result;
}

nsresult 
nsDOMCSSAttributeDeclaration::GetParent(nsISupports **aParent)
{
  if (nsnull != mContent) {
    return mContent->QueryInterface(kISupportsIID, (void **)aParent);
  }

  return NS_OK;
}

//----------------------------------------------------------------------

static nsresult EnsureWritableAttributes(nsIHTMLContent* aContent,
                                         nsIHTMLAttributes*& aAttributes, PRBool aCreate)
{
  nsresult  result = NS_OK;

  if (nsnull == aAttributes) {
    if (PR_TRUE == aCreate) {
      nsMapAttributesFunc mapFunc;
      result = aContent->GetAttributeMappingFunction(mapFunc);
      if (NS_OK == result) {
        result = NS_NewHTMLAttributes(&aAttributes, mapFunc);
        if (NS_OK == result) {
          aAttributes->AddContentRef();
        }
      }
    }
  }
  else {
    PRInt32 contentRefCount;
    aAttributes->GetContentRefCount(contentRefCount);
    if (1 < contentRefCount) {
      nsIHTMLAttributes*  attrs;
      result = aAttributes->Clone(&attrs);
      if (NS_OK == result) {
        aAttributes->ReleaseContentRef();
        NS_RELEASE(aAttributes);
        aAttributes = attrs;
        aAttributes->AddContentRef();
      }
    }
  }
  return result;
}

static void ReleaseAttributes(nsIHTMLAttributes*& aAttributes)
{
  aAttributes->ReleaseContentRef();
  NS_RELEASE(aAttributes);
}

nsGenericHTMLElement::nsGenericHTMLElement()
{
  mAttributes = nsnull;
}

nsGenericHTMLElement::~nsGenericHTMLElement()
{
  if (nsnull != mAttributes) {
    ReleaseAttributes(mAttributes);
  }
}

nsresult
nsGenericHTMLElement::GetId(nsString& aId)
{
  GetAttribute(nsHTMLAtoms::id, aId);
  return NS_OK;
}

nsresult
nsGenericHTMLElement::SetId(const nsString& aId)
{
  SetAttribute(nsHTMLAtoms::id, aId, PR_TRUE);
  return NS_OK;
}

nsresult
nsGenericHTMLElement::GetTitle(nsString& aTitle)
{
  GetAttribute(nsHTMLAtoms::title, aTitle);
  return NS_OK;
}

nsresult
nsGenericHTMLElement::SetTitle(const nsString& aTitle)
{
  SetAttribute(nsHTMLAtoms::title, aTitle, PR_TRUE);
  return NS_OK;
}

nsresult
nsGenericHTMLElement::GetLang(nsString& aLang)
{
  GetAttribute(nsHTMLAtoms::lang, aLang);
  return NS_OK;
}

nsresult
nsGenericHTMLElement::SetLang(const nsString& aLang)
{
  SetAttribute(nsHTMLAtoms::lang, aLang, PR_TRUE);
  return NS_OK;
}

nsresult
nsGenericHTMLElement::GetDir(nsString& aDir)
{
  GetAttribute(nsHTMLAtoms::dir, aDir);
  return NS_OK;
}

nsresult
nsGenericHTMLElement::SetDir(const nsString& aDir)
{
  SetAttribute(nsHTMLAtoms::dir, aDir, PR_TRUE);
  return NS_OK;
}

nsresult
nsGenericHTMLElement::GetClassName(nsString& aClassName)
{
  GetAttribute(nsHTMLAtoms::kClass, aClassName);
  return NS_OK;
}

nsresult
nsGenericHTMLElement::SetClassName(const nsString& aClassName)
{
  SetAttribute(nsHTMLAtoms::kClass, aClassName, PR_TRUE);
  return NS_OK;
}

nsresult    
nsGenericHTMLElement::GetStyle(nsIDOMCSSStyleDeclaration** aStyle)
{
  nsresult res = NS_OK;
  nsDOMSlots *slots = GetDOMSlots();

  if (nsnull == slots->mStyle) {
    nsIHTMLContent* htmlContent;
    mContent->QueryInterface(kIHTMLContentIID, (void **)&htmlContent);
    slots->mStyle = new nsDOMCSSAttributeDeclaration(htmlContent);
    NS_RELEASE(htmlContent);
    if (nsnull == slots->mStyle) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    NS_ADDREF(slots->mStyle);
  }
  
  res = slots->mStyle->QueryInterface(kIDOMCSSStyleDeclarationIID,
                                      (void **)aStyle);

  return res;
}

static nsIHTMLStyleSheet* GetAttrStyleSheet(nsIDocument* aDocument)
{
  nsIHTMLStyleSheet*  sheet = nsnull;
  nsIHTMLDocument*  htmlDoc;
  nsIXMLDocument* xmlDoc;
  
  if (nsnull != aDocument) {
    if (NS_OK == aDocument->QueryInterface(kIHTMLDocumentIID, (void**)&htmlDoc)) {
      htmlDoc->GetAttributeStyleSheet(&sheet);
      NS_RELEASE(htmlDoc);
    }
    // XXX The method GetAttributeStyleSheet should be factored into
    // another interface for any document capable of containing HTML
    // content.
    else if (NS_OK == aDocument->QueryInterface(kIXMLDocumentIID, (void **)&xmlDoc)) {
      xmlDoc->GetAttributeStyleSheet(&sheet);
      NS_RELEASE(xmlDoc);
    }
  }
  NS_ASSERTION(nsnull != sheet, "can't get attribute style sheet");
  return sheet;
}

nsresult
nsGenericHTMLElement::SetDocument(nsIDocument* aDocument, PRBool aDeep)
{
  nsresult result = nsGenericElement::SetDocument(aDocument, aDeep);
  
  if (NS_OK != result) {
    return result;
  }

  nsIHTMLContent* htmlContent;
  nsIXMLContent* xmlContent = nsnull;

  // XXX Current XMLContent is also HTMLContent. However, we don't
  // want to use the attribute stylesheet for XMLContent. This code
  // will go away when XMLContent is more pure.
  // TODO However, we still may want to use the attribute stylesheet
  // for attributes in XMLContent with the HTML namespace.
  result = mContent->QueryInterface(kIXMLContentIID, (void **)&xmlContent);

  result = mContent->QueryInterface(kIHTMLContentIID, (void **)&htmlContent);
  if (NS_OK != result) {
    return result;
  }

  if ((nsnull != mDocument) && (nsnull != mAttributes) && (nsnull == xmlContent)) {
    nsIHTMLStyleSheet*  sheet = GetAttrStyleSheet(mDocument);
    if (nsnull != sheet) {
      sheet->SetAttributesFor(htmlContent, mAttributes); // sync attributes with sheet
      NS_RELEASE(sheet);
    }
  }

  NS_RELEASE(htmlContent);
  return result;
}

//void
//nsHTMLTagContent::SizeOfWithoutThis(nsISizeOfHandler* aHandler) const
//{
//  if (!aHandler->HaveSeen(mTag)) {
//    mTag->SizeOf(aHandler);
//  }
//  if (!aHandler->HaveSeen(mAttributes)) {
//    mAttributes->SizeOf(aHandler);
//  }
//}

nsresult
nsGenericHTMLElement::SetAttribute(const nsString& aName,
                                   const nsString& aValue,
                                   PRBool aNotify)
{
  nsAutoString upper;
  aName.ToUpperCase(upper);
  nsIAtom* attr = NS_NewAtom(upper);
  nsresult rv = SetAttribute(attr, aValue, aNotify);
  NS_RELEASE(attr);
  return rv;
}

#if 0
static nsGenericHTMLElement::EnumTable kDirTable[] = {
  { "ltr", NS_STYLE_DIRECTION_LTR },
  { "rtl", NS_STYLE_DIRECTION_RTL },
  { 0 }
};
#endif

nsresult
nsGenericHTMLElement::SetAttribute(nsIAtom* aAttribute,
                                   const nsString& aValue,
                                   PRBool aNotify)
{
  nsresult  result = NS_OK;
#if 0
  if (nsHTMLAtoms::dir == aAttribute) {
    nsHTMLValue val;
    if (ParseEnumValue(aValue, kDirTable, val)) {
      result = SetAttribute(aAttribute, val, aNotify);
    }
    else {
      result = SetStringAttribute(aAttribute, aValue, aNotify);
    }
  }
  else if (nsHTMLAtoms::lang == aAttribute) {
    result = SetStringAttribute(aAttribute, aValue, aNotify);
  }
  else if (nsHTMLAtoms::title == aAttribute) {
    result = SetStringAttribute(aAttribute, aValue, aNotify);
  }
  else
#endif
  if (nsHTMLAtoms::style == aAttribute) {
    // XXX the style sheet language is a document property that
    // should be used to lookup the style sheet parser to parse the
    // attribute.
    nsICSSParser* css;
    result = NS_NewCSSParser(&css);
    if (NS_OK != result) {
      return result;
    }
    nsIStyleRule* rule;
    result = css->ParseDeclarations(aValue, nsnull, rule);
    if ((NS_OK == result) && (nsnull != rule)) {
      result = SetAttribute(aAttribute, nsHTMLValue(rule), aNotify);
      NS_RELEASE(rule);
    }
    NS_RELEASE(css);
  }
  else {
    // Check for event handlers
    if ((nsHTMLAtoms::onclick == aAttribute) || 
             (nsHTMLAtoms::ondblclick == aAttribute) ||
             (nsHTMLAtoms::onmousedown == aAttribute) ||
             (nsHTMLAtoms::onmouseup == aAttribute) ||
             (nsHTMLAtoms::onmouseover == aAttribute) ||
             (nsHTMLAtoms::onmouseout == aAttribute))
      AddScriptEventListener(aAttribute, aValue, kIDOMMouseListenerIID); 
    else if ((nsHTMLAtoms::onkeydown == aAttribute) ||
             (nsHTMLAtoms::onkeyup == aAttribute) ||
             (nsHTMLAtoms::onkeypress == aAttribute))
      AddScriptEventListener(aAttribute, aValue, kIDOMKeyListenerIID); 
    else if (nsHTMLAtoms::onmousemove == aAttribute)
      AddScriptEventListener(aAttribute, aValue, kIDOMMouseMotionListenerIID); 
    else if (nsHTMLAtoms::onload == aAttribute)
      AddScriptEventListener(nsHTMLAtoms::onload, aValue, kIDOMLoadListenerIID); 
    else if ((nsHTMLAtoms::onunload == aAttribute) ||
             (nsHTMLAtoms::onabort == aAttribute) ||
             (nsHTMLAtoms::onerror == aAttribute))
      AddScriptEventListener(aAttribute, aValue, kIDOMLoadListenerIID); 
    else if ((nsHTMLAtoms::onfocus == aAttribute) ||
             (nsHTMLAtoms::onblur == aAttribute))
      AddScriptEventListener(aAttribute, aValue, kIDOMFocusListenerIID); 
    else if ((nsHTMLAtoms::onsubmit == aAttribute) ||
             (nsHTMLAtoms::onreset == aAttribute) ||
             (nsHTMLAtoms::onchange == aAttribute))
      AddScriptEventListener(aAttribute, aValue, kIDOMFormListenerIID); 
    else if (nsHTMLAtoms::onpaint == aAttribute)
      AddScriptEventListener(aAttribute, aValue, kIDOMPaintListenerIID); 

    nsHTMLValue val;
    nsIHTMLContent* htmlContent;
    nsIXMLContent* xmlContent = nsnull;
    
    // XXX Current XMLContent is also HTMLContent. However, we don't
    // want to use the attribute stylesheet for XMLContent. This code
    // will go away when XMLContent is more pure.
    // TODO However, we still may want to use the attribute stylesheet
    // for attributes in XMLContent with the HTML namespace.
    result = mContent->QueryInterface(kIXMLContentIID, (void **)&xmlContent);
    
    result = mContent->QueryInterface(kIHTMLContentIID, (void **)&htmlContent);
    if (NS_OK != result) {
      return result;
    }
    if (NS_CONTENT_ATTR_NOT_THERE !=
        htmlContent->StringToAttribute(aAttribute, aValue, val)) {
      // string value was mapped to nsHTMLValue, set it that way
      result = SetAttribute(aAttribute, val, aNotify);
      NS_RELEASE(htmlContent);
      return result;
    }
    else {
      // set as string value to avoid another string copy
      if ((nsnull != mDocument) && (nsnull == xmlContent)) {  // set attr via style sheet
        nsIHTMLStyleSheet*  sheet = GetAttrStyleSheet(mDocument);
        if (nsnull != sheet) {
          result = sheet->SetAttributeFor(aAttribute, aValue, htmlContent, mAttributes);
          NS_RELEASE(sheet);
        }
      }
      else {  // manage this ourselves and re-sync when we connect to doc
        result = EnsureWritableAttributes(htmlContent, mAttributes, PR_TRUE);
        if (nsnull != mAttributes) {
          PRInt32   count;
          result = mAttributes->SetAttribute(aAttribute, aValue, count);
          if (0 == count) {
            ReleaseAttributes(mAttributes);
          }
        }
      }
    }
    NS_RELEASE(htmlContent);

    if (aNotify && (nsnull != mDocument)) {
      mDocument->AttributeChanged(mContent, aAttribute, NS_STYLE_HINT_UNKNOWN);
    }
  }
  return result;
}

nsresult
nsGenericHTMLElement::SetAttribute(nsIAtom* aAttribute,
                                   const nsHTMLValue& aValue,
                                   PRBool aNotify)
{
  nsresult  result = NS_OK;
  nsIHTMLContent* htmlContent;
  nsIXMLContent* xmlContent = nsnull;

  // XXX Current XMLContent is also HTMLContent. However, we don't
  // want to use the attribute stylesheet for XMLContent. This code
  // will go away when XMLContent is more pure.
  // TODO However, we still may want to use the attribute stylesheet
  // for attributes in XMLContent with the HTML namespace.
  result = mContent->QueryInterface(kIXMLContentIID, (void **)&xmlContent);

  result = mContent->QueryInterface(kIHTMLContentIID, (void **)&htmlContent);
  if (NS_OK != result) {
    return result;
  }
  if (nsnull != mDocument) {  // set attr via style sheet
    if (nsnull == xmlContent) {
      nsIHTMLStyleSheet*  sheet = GetAttrStyleSheet(mDocument);
      if (nsnull != sheet) {
        result = sheet->SetAttributeFor(aAttribute, aValue, htmlContent,
                                        mAttributes);
        NS_RELEASE(sheet);
      }
    }
    if (aNotify) {
      mDocument->AttributeChanged(mContent, aAttribute, NS_STYLE_HINT_UNKNOWN);
    }
  }
  else {  // manage this ourselves and re-sync when we connect to doc
    result = EnsureWritableAttributes(htmlContent, mAttributes, PR_TRUE);
    if (nsnull != mAttributes) {
      PRInt32   count;
      result = mAttributes->SetAttribute(aAttribute, aValue, count);
      if (0 == count) {
        ReleaseAttributes(mAttributes);
      }
    }
  }
  NS_RELEASE(htmlContent);
  return result;
}

/**
 * Handle attributes common to all html elements
 */
void
nsGenericHTMLElement::MapCommonAttributesInto(nsIHTMLAttributes* aAttributes, 
                                              nsIStyleContext* aStyleContext,
                                              nsIPresContext* aPresContext)
{
  if (nsnull != aAttributes) {
    nsHTMLValue value;
    aAttributes->GetAttribute(nsHTMLAtoms::dir, value);
    if (value.GetUnit() == eHTMLUnit_Enumerated) {
      nsStyleDisplay* display = (nsStyleDisplay*)
        aStyleContext->GetMutableStyleData(eStyleStruct_Display);
      display->mDirection = value.GetIntValue();
    }
  }
}

nsresult
nsGenericHTMLElement::UnsetAttribute(nsIAtom* aAttribute, PRBool aNotify)
{
  nsresult result = NS_OK;
  nsIHTMLContent* htmlContent;
  nsIXMLContent* xmlContent = nsnull;

  // XXX Current XMLContent is also HTMLContent. However, we don't
  // want to use the attribute stylesheet for XMLContent. This code
  // will go away when XMLContent is more pure.
  // TODO However, we still may want to use the attribute stylesheet
  // for attributes in XMLContent with the HTML namespace.
  result = mContent->QueryInterface(kIXMLContentIID, (void **)&xmlContent);

  result = mContent->QueryInterface(kIHTMLContentIID, (void **)&htmlContent);
  if (NS_OK != result) {
    return result;
  }
  if (nsnull != mDocument) {  // set attr via style sheet
    if (nsnull == xmlContent) {
      nsIHTMLStyleSheet*  sheet = GetAttrStyleSheet(mDocument);
      if (nsnull != sheet) {
        result = sheet->UnsetAttributeFor(aAttribute, htmlContent, mAttributes);
        NS_RELEASE(sheet);
      }
    }
    if (aNotify) {
      mDocument->AttributeChanged(mContent, aAttribute, NS_STYLE_HINT_UNKNOWN);
    }
  }
  else {  // manage this ourselves and re-sync when we connect to doc
    result = EnsureWritableAttributes(htmlContent, mAttributes, PR_FALSE);
    if (nsnull != mAttributes) {
      PRInt32 count;
      result = mAttributes->UnsetAttribute(aAttribute, count);
      if (0 == count) {
        ReleaseAttributes(mAttributes);
      }
    }
  }
  NS_RELEASE(htmlContent);
  return result;
}

nsresult
nsGenericHTMLElement::GetAttribute(const nsString& aName,
                                   nsString& aResult) const
{
  nsAutoString upper;
  aName.ToUpperCase(upper);
  nsIAtom* attr = NS_NewAtom(upper);
  nsresult result = GetAttribute(attr, aResult);
  NS_RELEASE(attr);
  return result;
}

nsresult
nsGenericHTMLElement::GetAttribute(nsIAtom *aAttribute,
                                   nsString &aResult) const
{
  nsHTMLValue value;
  nsresult result = GetAttribute(aAttribute, value);

  char cbuf[20];
  nscolor color;
  if (NS_CONTENT_ATTR_HAS_VALUE == result) {
    nsIHTMLContent* htmlContent;
    result = mContent->QueryInterface(kIHTMLContentIID, (void **)&htmlContent);
    if (NS_OK != result) {
      return result;
    }
    // Try subclass conversion routine first
    if (NS_CONTENT_ATTR_HAS_VALUE ==
        htmlContent->AttributeToString(aAttribute, value, aResult)) {
      NS_RELEASE(htmlContent);
      return result;
    }
    NS_RELEASE(htmlContent);

    // Provide default conversions for most everything
    switch (value.GetUnit()) {
    case eHTMLUnit_Empty:
      aResult.Truncate();
      break;

    case eHTMLUnit_String:
    case eHTMLUnit_Null:
      value.GetStringValue(aResult);
      break;

    case eHTMLUnit_Integer:
      aResult.Truncate();
      aResult.Append(value.GetIntValue(), 10);
      break;

    case eHTMLUnit_Pixel:
      aResult.Truncate();
      aResult.Append(value.GetPixelValue(), 10);
      break;

    case eHTMLUnit_Percent:
      aResult.Truncate(0);
      aResult.Append(PRInt32(value.GetPercentValue() * 100.0f), 10);
      aResult.Append('%');
      break;

    case eHTMLUnit_Color:
      color = nscolor(value.GetColorValue());
      PR_snprintf(cbuf, sizeof(cbuf), "#%02x%02x%02x",
                  NS_GET_R(color), NS_GET_G(color), NS_GET_B(color));
      aResult.Truncate(0);
      aResult.Append(cbuf);
      break;

    default:
    case eHTMLUnit_Enumerated:
      NS_NOTREACHED("no default enumerated value to string conversion");
      result = NS_CONTENT_ATTR_NOT_THERE;
      break;
    }
  }
  return result;
}

nsresult
nsGenericHTMLElement::GetAttribute(nsIAtom* aAttribute,
                                   nsHTMLValue& aValue) const
{
  if (nsnull != mAttributes) {
    return mAttributes->GetAttribute(aAttribute, aValue);
  }
  aValue.Reset();
  return NS_CONTENT_ATTR_NOT_THERE;
}

nsresult
nsGenericHTMLElement::GetAllAttributeNames(nsISupportsArray* aArray,
                                           PRInt32& aCount) const
{
  if (nsnull != mAttributes) {
    return mAttributes->GetAllAttributeNames(aArray, aCount);
  }
  aCount = 0;
  return NS_OK;
}

nsresult
nsGenericHTMLElement::GetAttributeCount(PRInt32& aCount) const
{
  if (nsnull != mAttributes) {
    return mAttributes->Count(aCount);
  }
  aCount = 0;
  return NS_OK;
}

nsresult
nsGenericHTMLElement::SetID(nsIAtom* aID)
{
  nsresult result = NS_OK;
  nsIHTMLContent* htmlContent;

  result = mContent->QueryInterface(kIHTMLContentIID, (void **)&htmlContent);
  if (NS_OK != result) {
    return result;
  }
  if (nsnull != mDocument) {  // set attr via style sheet
    nsIHTMLStyleSheet*  sheet = GetAttrStyleSheet(mDocument);
    if (nsnull != sheet) {
      result = sheet->SetIDFor(aID, htmlContent, mAttributes);
      NS_RELEASE(sheet);
    }
  }
  else {  // manage this ourselves and re-sync when we connect to doc
    EnsureWritableAttributes(htmlContent, mAttributes, PRBool(nsnull != aID));
    if (nsnull != mAttributes) {
      PRInt32 count;
      result = mAttributes->SetID(aID, count);
      if (0 == count) {
        ReleaseAttributes(mAttributes);
      }
    }
  }
  NS_RELEASE(htmlContent);
  return result;
}

nsresult
nsGenericHTMLElement::GetID(nsIAtom*& aResult) const
{
  if (nsnull != mAttributes) {
    return mAttributes->GetID(aResult);
  }
  aResult = nsnull;
  return NS_OK;
}

nsresult
nsGenericHTMLElement::SetClass(nsIAtom* aClass)
{
  nsresult result = NS_OK;
  nsIHTMLContent* htmlContent;

  result = mContent->QueryInterface(kIHTMLContentIID, (void **)&htmlContent);
  if (NS_OK != result) {
    return result;
  }
  if (nsnull != mDocument) {  // set attr via style sheet
    nsIHTMLStyleSheet*  sheet = GetAttrStyleSheet(mDocument);
    if (nsnull != sheet) {
      result = sheet->SetClassFor(aClass, htmlContent, mAttributes);
      NS_RELEASE(sheet);
    }
  }
  else {  // manage this ourselves and re-sync when we connect to doc
    EnsureWritableAttributes(htmlContent, mAttributes, PRBool(nsnull != aClass));
    if (nsnull != mAttributes) {
      PRInt32 count;
      result = mAttributes->SetClass(aClass, count);
      if (0 == count) {
        ReleaseAttributes(mAttributes);
      }
    }
  }
  NS_RELEASE(htmlContent);
  return result;
}

nsresult
nsGenericHTMLElement::GetClass(nsIAtom*& aResult) const
{
  if (nsnull != mAttributes) {
    return mAttributes->GetClass(aResult);
  }
  aResult = nsnull;
  return NS_OK;
}

nsresult
nsGenericHTMLElement::GetStyleRule(nsIStyleRule*& aResult)
{
  nsIStyleRule* result = nsnull;

  if (nsnull != mAttributes) {
    mAttributes->QueryInterface(kIStyleRuleIID, (void**)&result);
  }
  aResult = result;
  return NS_OK;
}

void
nsGenericHTMLElement::ListAttributes(FILE* out) const
{
  nsISupportsArray* attrs;
  if (NS_OK == NS_NewISupportsArray(&attrs)) {
    PRInt32 index, count;
    GetAllAttributeNames(attrs, count);
    for (index = 0; index < count; index++) {
      // name
      nsIAtom* attr = (nsIAtom*)attrs->ElementAt(index);
      nsAutoString buffer;
      attr->ToString(buffer);

      // value
      nsAutoString value;
      GetAttribute(buffer, value);
      buffer.Append("=");
      buffer.Append(value);

      fputs(" ", out);
      fputs(buffer, out);
      NS_RELEASE(attr);
    }
    NS_RELEASE(attrs);
  }
}

nsresult
nsGenericHTMLElement::List(FILE* out, PRInt32 aIndent) const
{
  NS_PRECONDITION(nsnull != mDocument, "bad content");

  PRInt32 index;
  for (index = aIndent; --index >= 0; ) fputs("  ", out);

  nsIAtom* tag;
  GetTag(tag);
  if (tag != nsnull) {
    nsAutoString buf;
    tag->ToString(buf);
    fputs(buf, out);
    NS_RELEASE(tag);
  }

  ListAttributes(out);

  nsIContent* hc = mContent;  
  nsrefcnt r = NS_ADDREF(hc) - 1;
  NS_RELEASE(hc);
  fprintf(out, " refcount=%d<", r);

  PRBool canHaveKids;
  mContent->CanContainChildren(canHaveKids);
  if (canHaveKids) {
    fputs("\n", out);
    PRInt32 kids;
    mContent->ChildCount(kids);
    for (index = 0; index < kids; index++) {
      nsIContent* kid;
      mContent->ChildAt(index, kid);
      kid->List(out, aIndent + 1);
      NS_RELEASE(kid);
    }
    for (index = aIndent; --index >= 0; ) fputs("  ", out);
  }
  fputs(">\n", out);

  return NS_OK;
}

nsresult
nsGenericHTMLElement::ToHTML(FILE* out) const
{
  nsAutoString tmp;
  nsresult rv = ToHTMLString(tmp);
  fputs(tmp, out);
  return rv;
}

// XXX i18n: this is wrong (?) because we need to know the outgoing
// character set (I think)
void
NS_QuoteForHTML(const nsString& aValue, nsString& aResult)
{
  aResult.Truncate();
  const PRUnichar* cp = aValue.GetUnicode();
  const PRUnichar* end = aValue.GetUnicode() + aValue.Length();
  aResult.Append('"');
  while (cp < end) {
    PRUnichar ch = *cp++;
    if ((ch >= 0x20) && (ch <= 0x7f)) {
      if (ch == '\"') {
        aResult.Append("&quot;");
      }
      else {
        aResult.Append(ch);
      }
    }
    else {
      aResult.Append("&#");
      aResult.Append((PRInt32) ch, 10);
      aResult.Append(';');
    }
  }
  aResult.Append('"');
}

nsresult
nsGenericHTMLElement::ToHTMLString(nsString& aBuf) const
{
  aBuf.Truncate(0);
  aBuf.Append('<');

  if (nsnull != mTag) {
    nsAutoString tmp;
    mTag->ToString(tmp);
    aBuf.Append(tmp);
  } else {
    aBuf.Append("?NULL");
  }

  if (nsnull != mAttributes) {
    nsISupportsArray* attrs;
    nsresult rv = NS_NewISupportsArray(&attrs);
    if (NS_OK == rv) {
      PRInt32 i, n;
      mAttributes->GetAllAttributeNames(attrs, n);
      nsAutoString name, value, quotedValue;
      for (i = 0; i < n; i++) {
        nsIAtom* atom = (nsIAtom*) attrs->ElementAt(i);
        atom->ToString(name);
        aBuf.Append(' ');
        aBuf.Append(name);
        value.Truncate();
        GetAttribute(name, value);
        if (value.Length() > 0) {
          aBuf.Append('=');
          NS_QuoteForHTML(value, quotedValue);
          aBuf.Append(quotedValue);
        }
      }
      NS_RELEASE(attrs);
    }
  }

  aBuf.Append('>');
  return NS_OK;
}

//----------------------------------------------------------------------


nsresult
nsGenericHTMLElement::AttributeToString(nsIAtom* aAttribute,
                                        nsHTMLValue& aValue,
                                        nsString& aResult) const
{
  if (nsHTMLAtoms::style == aAttribute) {
    if (eHTMLUnit_ISupports == aValue.GetUnit()) {
      nsIStyleRule* rule = (nsIStyleRule*) aValue.GetISupportsValue();
      nsICSSStyleRule*  cssRule;
      if (NS_OK == rule->QueryInterface(kICSSStyleRuleIID, (void**)&cssRule)) {
        nsICSSDeclaration* decl = cssRule->GetDeclaration();
        if (nsnull != decl) {
          decl->ToString(aResult);
        }
        NS_RELEASE(cssRule);
      }
      else {
        aResult = "Unknown rule type";
      }
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }
  aResult.Truncate();
  return NS_CONTENT_ATTR_NOT_THERE;
}

PRBool
nsGenericHTMLElement::ParseEnumValue(const nsString& aValue,
                                     EnumTable* aTable,
                                     nsHTMLValue& aResult)
{
  while (nsnull != aTable->tag) {
    if (aValue.EqualsIgnoreCase(aTable->tag)) {
      aResult.SetIntValue(aTable->value, eHTMLUnit_Enumerated);
      return PR_TRUE;
    }
    aTable++;
  }
  return PR_FALSE;
}

PRBool
nsGenericHTMLElement::ParseCaseSensitiveEnumValue(const nsString& aValue,
                                                  EnumTable* aTable,
                                                  nsHTMLValue& aResult)
{
  while (nsnull != aTable->tag) {
    if (aValue.Equals(aTable->tag)) {
      aResult.SetIntValue(aTable->value, eHTMLUnit_Enumerated);
      return PR_TRUE;
    }
    aTable++;
  }
  return PR_FALSE;
}

PRBool
nsGenericHTMLElement::EnumValueToString(const nsHTMLValue& aValue,
                                        EnumTable* aTable,
                                        nsString& aResult)
{
  aResult.Truncate(0);
  if (aValue.GetUnit() == eHTMLUnit_Enumerated) {
    PRInt32 v = aValue.GetIntValue();
    while (nsnull != aTable->tag) {
      if (aTable->value == v) {
        aResult.Append(aTable->tag);
        return PR_TRUE;
      }
      aTable++;
    }
  }
  return PR_FALSE;
}

PRBool
nsGenericHTMLElement::ParseValueOrPercent(const nsString& aString,
                                          nsHTMLValue& aResult, 
                                          nsHTMLUnit aValueUnit)
{
  nsAutoString tmp(aString);
  tmp.CompressWhitespace(PR_TRUE, PR_TRUE);
  PRInt32 ec, val = tmp.ToInteger(&ec);
  if (NS_OK == ec) {
    if (val < 0) val = 0; 
    if (tmp.Last() == '%') {/* XXX not 100% compatible with ebina's code */
      if (val > 100) val = 100;
      aResult.SetPercentValue(float(val)/100.0f);
    } else {
      if (eHTMLUnit_Pixel == aValueUnit) {
        aResult.SetPixelValue(val);
      }
      else {
        aResult.SetIntValue(val, aValueUnit);
      }
    }
    return PR_TRUE;
  }

  // Illegal values are mapped to empty
  aResult.SetEmptyValue();
  return PR_FALSE;
}

/* used to parse attribute values that could be either:
 *   integer  (n), 
 *   percent  (n%),
 *   or proportional (n*)
 */
void
nsGenericHTMLElement::ParseValueOrPercentOrProportional(const nsString& aString,
                                                        nsHTMLValue& aResult, 
                                                        nsHTMLUnit aValueUnit)
{
  nsAutoString tmp(aString);
  tmp.CompressWhitespace(PR_TRUE, PR_TRUE);
  PRInt32 ec, val = tmp.ToInteger(&ec);
  if (NS_OK == ec) {
    if (val < 0) val = 0;
    if (tmp.Last() == '%') {/* XXX not 100% compatible with ebina's code */
      if (val > 100) val = 100;
      aResult.SetPercentValue(float(val)/100.0f);
	  } else if (tmp.Last() == '*') {
      aResult.SetIntValue(val, eHTMLUnit_Proportional); // proportional values are integers
    } else {
      if (eHTMLUnit_Pixel == aValueUnit) {
        aResult.SetPixelValue(val);
      }
      else {
        aResult.SetIntValue(val, aValueUnit);
      }
    }
  }
}

PRBool
nsGenericHTMLElement::ValueOrPercentToString(const nsHTMLValue& aValue,
                                             nsString& aResult)
{
  aResult.Truncate(0);
  switch (aValue.GetUnit()) {
  case eHTMLUnit_Integer:
    aResult.Append(aValue.GetIntValue(), 10);
    return PR_TRUE;
  case eHTMLUnit_Pixel:
    aResult.Append(aValue.GetPixelValue(), 10);
    return PR_TRUE;
  case eHTMLUnit_Percent:
    aResult.Append(PRInt32(aValue.GetPercentValue() * 100.0f), 10);
    aResult.Append('%');
    return PR_TRUE;
  }
  return PR_FALSE;
}

PRBool
nsGenericHTMLElement::ParseValue(const nsString& aString, PRInt32 aMin,
                                 nsHTMLValue& aResult, nsHTMLUnit aValueUnit)
{
  PRInt32 ec, val = aString.ToInteger(&ec);
  if (NS_OK == ec) {
    if (val < aMin) val = aMin;
    if (eHTMLUnit_Pixel == aValueUnit) {
      aResult.SetPixelValue(val);
    }
    else {
      aResult.SetIntValue(val, aValueUnit);
    }
    return PR_TRUE;
  }

  // Illegal values are mapped to empty
  aResult.SetEmptyValue();
  return PR_FALSE;
}

PRBool
nsGenericHTMLElement::ParseValue(const nsString& aString, PRInt32 aMin,
                                 PRInt32 aMax,
                                 nsHTMLValue& aResult, nsHTMLUnit aValueUnit)
{
  PRInt32 ec, val = aString.ToInteger(&ec);
  if (NS_OK == ec) {
    if (val < aMin) val = aMin;
    if (val > aMax) val = aMax;
    if (eHTMLUnit_Pixel == aValueUnit) {
      aResult.SetPixelValue(val);
    }
    else {
      aResult.SetIntValue(val, aValueUnit);
    }
    return PR_TRUE;
  }

  // Illegal values are mapped to empty
  aResult.SetEmptyValue();
  return PR_FALSE;
}

PRBool
nsGenericHTMLElement::ParseColor(const nsString& aString,
                                 nsHTMLValue& aResult)
{
  if (aString.Length() > 0) {
    nsAutoString  colorStr (aString);
    colorStr.CompressWhitespace();
    char cbuf[40];
    colorStr.ToCString(cbuf, sizeof(cbuf));
    nscolor color;
    if (NS_ColorNameToRGB(cbuf, &color)) {
      aResult.SetStringValue(colorStr);
      return PR_TRUE;
    }
    if (NS_LooseHexToRGB(cbuf, &color)) {
      aResult.SetColorValue(color);
      return PR_TRUE;
    }
  }

  // Illegal values are mapped to empty
  aResult.SetEmptyValue();
  return PR_FALSE;
}

PRBool
nsGenericHTMLElement::ColorToString(const nsHTMLValue& aValue,
                                    nsString& aResult)
{
  if (aValue.GetUnit() == eHTMLUnit_Color) {
    nscolor v = aValue.GetColorValue();
    char buf[10];
    PR_snprintf(buf, sizeof(buf), "#%02x%02x%02x",
                NS_GET_R(v), NS_GET_G(v), NS_GET_B(v));
    aResult.Truncate(0);
    aResult.Append(buf);
    return PR_TRUE;
  }
  if (aValue.GetUnit() == eHTMLUnit_String) {
    aValue.GetStringValue(aResult);
    return PR_TRUE;
  }
  if (aValue.GetUnit() == eHTMLUnit_Empty) {  // was illegal
    aResult.Truncate();
    return PR_TRUE;
  }
  return PR_FALSE;
}

// XXX check all mappings against ebina's usage
static nsGenericHTMLElement::EnumTable kAlignTable[] = {
  { "left", NS_STYLE_TEXT_ALIGN_LEFT },
  { "right", NS_STYLE_TEXT_ALIGN_RIGHT },
  { "texttop", NS_STYLE_VERTICAL_ALIGN_TEXT_TOP },
  { "baseline", NS_STYLE_VERTICAL_ALIGN_BASELINE },
  { "center", NS_STYLE_TEXT_ALIGN_CENTER },
  { "bottom", NS_STYLE_VERTICAL_ALIGN_BOTTOM },
  { "top", NS_STYLE_VERTICAL_ALIGN_TOP },
  { "middle", NS_STYLE_VERTICAL_ALIGN_MIDDLE },
  { "absbottom", NS_STYLE_VERTICAL_ALIGN_BOTTOM },
  { "abscenter", NS_STYLE_VERTICAL_ALIGN_MIDDLE },
  { "absmiddle", NS_STYLE_VERTICAL_ALIGN_MIDDLE },
  { 0 }
};

static nsGenericHTMLElement::EnumTable kDivAlignTable[] = {
  { "left", NS_STYLE_TEXT_ALIGN_LEFT },
  { "right", NS_STYLE_TEXT_ALIGN_RIGHT },
  { "center", NS_STYLE_TEXT_ALIGN_CENTER },
  { "middle", NS_STYLE_TEXT_ALIGN_CENTER },
  { "justify", NS_STYLE_TEXT_ALIGN_JUSTIFY },
  { 0 }
};

static nsGenericHTMLElement::EnumTable kFormAlignTable[] = {
  { "left", NS_STYLE_TEXT_ALIGN_LEFT },
  { "right", NS_STYLE_TEXT_ALIGN_RIGHT },
  { "bottom", NS_STYLE_VERTICAL_ALIGN_BOTTOM },
  { "top", NS_STYLE_VERTICAL_ALIGN_TOP },
  { "middle", NS_STYLE_VERTICAL_ALIGN_MIDDLE },
  { 0 }
};

static nsGenericHTMLElement::EnumTable kFrameborderQuirksTable[] = {
  { "yes", NS_STYLE_FRAME_YES },
  { "no", NS_STYLE_FRAME_NO },
  { "1", NS_STYLE_FRAME_1 },
  { "0", NS_STYLE_FRAME_0 },
  { 0 }
};

static nsGenericHTMLElement::EnumTable kFrameborderStandardTable[] = {
  { "1", NS_STYLE_FRAME_1 },
  { "0", NS_STYLE_FRAME_0 },
  { 0 }
};

static nsGenericHTMLElement::EnumTable kScrollingQuirksTable[] = {
  { "yes", NS_STYLE_FRAME_YES },
  { "no", NS_STYLE_FRAME_NO },
  { "on", NS_STYLE_FRAME_ON },
  { "off", NS_STYLE_FRAME_OFF },
  { "scroll", NS_STYLE_FRAME_SCROLL },
  { "noscroll", NS_STYLE_FRAME_NOSCROLL },
  { "auto", NS_STYLE_FRAME_AUTO },
  { 0 }
};

static nsGenericHTMLElement::EnumTable kScrollingStandardTable[] = {
  { "yes", NS_STYLE_FRAME_YES },
  { "no", NS_STYLE_FRAME_NO },
  { "auto", NS_STYLE_FRAME_AUTO },
  { 0 }
};

static nsGenericHTMLElement::EnumTable kTableHAlignTable[] = {
  { "left",   NS_STYLE_TEXT_ALIGN_LEFT },
  { "right",  NS_STYLE_TEXT_ALIGN_RIGHT },
  { "center", NS_STYLE_TEXT_ALIGN_CENTER },
  { "middle", NS_STYLE_TEXT_ALIGN_CENTER },   // non-standard but necessary for Nav4 compatibility
  { "char",   NS_STYLE_TEXT_ALIGN_CHAR },
  { "justify",NS_STYLE_TEXT_ALIGN_JUSTIFY },
  { 0 }
};

static nsGenericHTMLElement::EnumTable kTableVAlignTable[] = {
  { "top",     NS_STYLE_VERTICAL_ALIGN_TOP },
  { "middle",  NS_STYLE_VERTICAL_ALIGN_MIDDLE },
  { "bottom",  NS_STYLE_VERTICAL_ALIGN_BOTTOM },
  { "baseline",NS_STYLE_VERTICAL_ALIGN_BASELINE },
  { 0 }
};

PRBool
nsGenericHTMLElement::ParseAlignValue(const nsString& aString,
                                      nsHTMLValue& aResult)
{
  return ParseEnumValue(aString, kAlignTable, aResult);
}

PRBool
nsGenericHTMLElement::ParseTableHAlignValue(const nsString& aString,
                                            nsHTMLValue& aResult)
{
  return ParseEnumValue(aString, kTableHAlignTable, aResult);
}

PRBool
nsGenericHTMLElement::ParseTableVAlignValue(const nsString& aString,
                                            nsHTMLValue& aResult)
{
  return ParseEnumValue(aString, kTableVAlignTable, aResult);
}

PRBool
nsGenericHTMLElement::AlignValueToString(const nsHTMLValue& aValue,
                                         nsString& aResult)
{
  return EnumValueToString(aValue, kAlignTable, aResult);
}

PRBool
nsGenericHTMLElement::ParseFormAlignValue(const nsString& aString,
                                          nsHTMLValue& aResult)
{
  return ParseEnumValue(aString, kFormAlignTable, aResult);
}

PRBool
nsGenericHTMLElement::FormAlignValueToString(const nsHTMLValue& aValue,
                                             nsString& aResult)
{
  return EnumValueToString(aValue, kFormAlignTable, aResult);
}

PRBool
nsGenericHTMLElement::TableHAlignValueToString(const nsHTMLValue& aValue,
                                               nsString& aResult)
{
  return EnumValueToString(aValue, kTableHAlignTable, aResult);
}

PRBool
nsGenericHTMLElement::TableVAlignValueToString(const nsHTMLValue& aValue,
                                               nsString& aResult)
{
  return EnumValueToString(aValue, kTableVAlignTable, aResult);
}

PRBool
nsGenericHTMLElement::ParseDivAlignValue(const nsString& aString,
                                         nsHTMLValue& aResult)
{
  return ParseEnumValue(aString, kDivAlignTable, aResult);
}

PRBool
nsGenericHTMLElement::DivAlignValueToString(const nsHTMLValue& aValue,
                                            nsString& aResult)
{
  return EnumValueToString(aValue, kDivAlignTable, aResult);
}

PRBool
nsGenericHTMLElement::ParseImageAttribute(nsIAtom* aAttribute,
                                          const nsString& aString,
                                          nsHTMLValue& aResult)
{
  if ((aAttribute == nsHTMLAtoms::width) ||
      (aAttribute == nsHTMLAtoms::height)) {
    ParseValueOrPercent(aString, aResult, eHTMLUnit_Pixel);
    return PR_TRUE;
  }
  else if ((aAttribute == nsHTMLAtoms::hspace) ||
           (aAttribute == nsHTMLAtoms::vspace) ||
           (aAttribute == nsHTMLAtoms::border)) {
    ParseValue(aString, 0, aResult, eHTMLUnit_Pixel);
    return PR_TRUE;
  }
  return PR_FALSE;
}

PRBool
nsGenericHTMLElement::ImageAttributeToString(nsIAtom* aAttribute,
                                             const nsHTMLValue& aValue,
                                             nsString& aResult)
{
  if ((aAttribute == nsHTMLAtoms::width) ||
      (aAttribute == nsHTMLAtoms::height) ||
      (aAttribute == nsHTMLAtoms::border) ||
      (aAttribute == nsHTMLAtoms::hspace) ||
      (aAttribute == nsHTMLAtoms::vspace)) {
    return ValueOrPercentToString(aValue, aResult);
  }
  return PR_FALSE;
}

PRBool
nsGenericHTMLElement::ParseFrameborderValue(PRBool aStandardMode,
                                            const nsString& aString,
                                            nsHTMLValue& aResult)
{
  if (aStandardMode) {
    return ParseEnumValue(aString, kFrameborderStandardTable, aResult);
  } else {
    return ParseEnumValue(aString, kFrameborderQuirksTable, aResult);
  }
}

PRBool
nsGenericHTMLElement::FrameborderValueToString(PRBool aStandardMode,
                                               const nsHTMLValue& aValue,
                                               nsString& aResult)
{
  if (aStandardMode) {
    return EnumValueToString(aValue, kFrameborderStandardTable, aResult);
  } else {
    return EnumValueToString(aValue, kFrameborderQuirksTable, aResult);
  }
}

PRBool
nsGenericHTMLElement::ParseScrollingValue(PRBool aStandardMode,
                                          const nsString& aString,
                                          nsHTMLValue& aResult)
{
  if (aStandardMode) {
    return ParseEnumValue(aString, kScrollingStandardTable, aResult);
  } else {
    return ParseEnumValue(aString, kScrollingQuirksTable, aResult);
  }
}

PRBool
nsGenericHTMLElement::ScrollingValueToString(PRBool aStandardMode,
                                             const nsHTMLValue& aValue,
                                             nsString& aResult)
{
  if (aStandardMode) {
    return EnumValueToString(aValue, kScrollingStandardTable, aResult);
  } else {
    return EnumValueToString(aValue, kScrollingQuirksTable, aResult);
  }
}

void
nsGenericHTMLElement::MapImageAttributesInto(nsIHTMLAttributes* aAttributes, 
                                             nsIStyleContext* aContext, 
                                             nsIPresContext* aPresContext)
{
  if (nsnull != aAttributes) {
    nsHTMLValue value;

    float p2t = aPresContext->GetPixelsToTwips();
    nsStylePosition* pos = (nsStylePosition*)
      aContext->GetMutableStyleData(eStyleStruct_Position);
    nsStyleSpacing* spacing = (nsStyleSpacing*)
      aContext->GetMutableStyleData(eStyleStruct_Spacing);

    // width: value
    aAttributes->GetAttribute(nsHTMLAtoms::width, value);
    if (value.GetUnit() == eHTMLUnit_Pixel) {
      nscoord twips = NSIntPixelsToTwips(value.GetPixelValue(), p2t);
      pos->mWidth.SetCoordValue(twips);
    }
    else if (value.GetUnit() == eHTMLUnit_Percent) {
      pos->mWidth.SetPercentValue(value.GetPercentValue());
    }

    // height: value
    aAttributes->GetAttribute(nsHTMLAtoms::height, value);
    if (value.GetUnit() == eHTMLUnit_Pixel) {
      nscoord twips = NSIntPixelsToTwips(value.GetPixelValue(), p2t);
      pos->mHeight.SetCoordValue(twips);
    }
    else if (value.GetUnit() == eHTMLUnit_Percent) {
      pos->mHeight.SetPercentValue(value.GetPercentValue());
    }

    // hspace: value
    aAttributes->GetAttribute(nsHTMLAtoms::hspace, value);
    if (value.GetUnit() == eHTMLUnit_Pixel) {
      nscoord twips = NSIntPixelsToTwips(value.GetPixelValue(), p2t);
      nsStyleCoord c(twips);
      spacing->mMargin.SetLeft(c);
      spacing->mMargin.SetRight(c);
    }
    else if (value.GetUnit() == eHTMLUnit_Percent) {
      nsStyleCoord c(value.GetPercentValue(), eStyleUnit_Coord);
      spacing->mMargin.SetLeft(c);
      spacing->mMargin.SetRight(c);
    }

    // vspace: value
    aAttributes->GetAttribute(nsHTMLAtoms::vspace, value);
    if (value.GetUnit() == eHTMLUnit_Pixel) {
      nscoord twips = NSIntPixelsToTwips(value.GetPixelValue(), p2t);
      nsStyleCoord c(twips);
      spacing->mMargin.SetTop(c);
      spacing->mMargin.SetBottom(c);
    }
    else if (value.GetUnit() == eHTMLUnit_Percent) {
      nsStyleCoord c(value.GetPercentValue(), eStyleUnit_Coord);
      spacing->mMargin.SetTop(c);
      spacing->mMargin.SetBottom(c);
    }
  }
}

void
nsGenericHTMLElement::MapImageAlignAttributeInto(nsIHTMLAttributes* aAttributes,
                                                 nsIStyleContext* aContext,
                                                 nsIPresContext* aPresContext)
{
  if (nsnull != aAttributes) {
    nsHTMLValue value;
    aAttributes->GetAttribute(nsHTMLAtoms::align, value);
    if (value.GetUnit() == eHTMLUnit_Enumerated) {
      PRUint8 align = value.GetIntValue();
      nsStyleDisplay* display = (nsStyleDisplay*)
        aContext->GetMutableStyleData(eStyleStruct_Display);
      nsStyleText* text = (nsStyleText*)
        aContext->GetMutableStyleData(eStyleStruct_Text);
      nsStyleSpacing* spacing = (nsStyleSpacing*)
        aContext->GetMutableStyleData(eStyleStruct_Spacing);
      float p2t = aPresContext->GetPixelsToTwips();
      nsStyleCoord three(NSIntPixelsToTwips(3, p2t));
      switch (align) {
      case NS_STYLE_TEXT_ALIGN_LEFT:
        display->mFloats = NS_STYLE_FLOAT_LEFT;
        spacing->mMargin.SetLeft(three);
        spacing->mMargin.SetRight(three);
        break;
      case NS_STYLE_TEXT_ALIGN_RIGHT:
        display->mFloats = NS_STYLE_FLOAT_RIGHT;
        spacing->mMargin.SetLeft(three);
        spacing->mMargin.SetRight(three);
        break;
      default:
        text->mVerticalAlign.SetIntValue(align, eStyleUnit_Enumerated);
        break;
      }
    }
  }
}

void
nsGenericHTMLElement::MapImageBorderAttributesInto(nsIHTMLAttributes* aAttributes, 
                                                   nsIStyleContext* aContext, 
                                                   nsIPresContext* aPresContext,
                                                   nscolor aBorderColors[4])
{
  if (nsnull != aAttributes) {
    nsHTMLValue value;

    // border: pixels
    aAttributes->GetAttribute(nsHTMLAtoms::border, value);
    if (value.GetUnit() != eHTMLUnit_Pixel) {
      if (nsnull == aBorderColors) {
        return;
      }
      // If no border is defined and we are forcing a border, force
      // the size to 2 pixels.
      value.SetPixelValue(2);
    }

    float p2t = aPresContext->GetPixelsToTwips();
    nscoord twips = NSIntPixelsToTwips(value.GetPixelValue(), p2t);

    // Fixup border-padding sums: subtract out the old size and then
    // add in the new size.
    nsStyleSpacing* spacing = (nsStyleSpacing*)
      aContext->GetMutableStyleData(eStyleStruct_Spacing);
    nsStyleCoord coord;
    coord.SetCoordValue(twips);
    spacing->mBorder.SetTop(coord);
    spacing->mBorder.SetRight(coord);
    spacing->mBorder.SetBottom(coord);
    spacing->mBorder.SetLeft(coord);
    spacing->mBorderStyle[0] = NS_STYLE_BORDER_STYLE_SOLID;
    spacing->mBorderStyle[1] = NS_STYLE_BORDER_STYLE_SOLID;
    spacing->mBorderStyle[2] = NS_STYLE_BORDER_STYLE_SOLID;
    spacing->mBorderStyle[3] = NS_STYLE_BORDER_STYLE_SOLID;

    // Use supplied colors if provided, otherwise use color for border
    // color
    if (nsnull != aBorderColors) {
      spacing->mBorderColor[0] = aBorderColors[0];
      spacing->mBorderColor[1] = aBorderColors[1];
      spacing->mBorderColor[2] = aBorderColors[2];
      spacing->mBorderColor[3] = aBorderColors[3];
    }
    else {
      // Color is inherited from "color"
      const nsStyleColor* styleColor = (const nsStyleColor*)
        aContext->GetStyleData(eStyleStruct_Color);
      nscolor color = styleColor->mColor;
      spacing->mBorderColor[0] = color;
      spacing->mBorderColor[1] = color;
      spacing->mBorderColor[2] = color;
      spacing->mBorderColor[3] = color;
    }
  }
}

void
nsGenericHTMLElement::MapBackgroundAttributesInto(nsIHTMLAttributes* aAttributes, 
                                                  nsIStyleContext* aContext,
                                                  nsIPresContext* aPresContext)
{
  nsHTMLValue value;

  // background
  if (NS_CONTENT_ATTR_HAS_VALUE ==
      aAttributes->GetAttribute(nsHTMLAtoms::background, value)) {
    if (eHTMLUnit_String == value.GetUnit()) {
      nsAutoString absURLSpec;
      nsAutoString spec;
      value.GetStringValue(spec);
      if (spec.Length() > 0) {
        // Resolve url to an absolute url
        nsIURL* docURL = nsnull;
        aPresContext->GetBaseURL(docURL);

        nsresult rv = NS_MakeAbsoluteURL(docURL, "", spec, absURLSpec);
        NS_IF_RELEASE(docURL);

        nsStyleColor* color = (nsStyleColor*)
          aContext->GetMutableStyleData(eStyleStruct_Color);
        color->mBackgroundImage = absURLSpec;
        color->mBackgroundFlags &= ~NS_STYLE_BG_IMAGE_NONE;
        color->mBackgroundRepeat = NS_STYLE_BG_REPEAT_XY;
      }
    }
  }

  // bgcolor
  if (NS_CONTENT_ATTR_HAS_VALUE == aAttributes->GetAttribute(nsHTMLAtoms::bgcolor, value)) {
    if (eHTMLUnit_Color == value.GetUnit()) {
      nsStyleColor* color = (nsStyleColor*)
        aContext->GetMutableStyleData(eStyleStruct_Color);
      color->mBackgroundColor = value.GetColorValue();
      color->mBackgroundFlags &= ~NS_STYLE_BG_COLOR_TRANSPARENT;
    }
    else if (eHTMLUnit_String == value.GetUnit()) {
      nsAutoString buffer;
      value.GetStringValue(buffer);
      char cbuf[40];
      buffer.ToCString(cbuf, sizeof(cbuf));

      nsStyleColor* color = (nsStyleColor*)
        aContext->GetMutableStyleData(eStyleStruct_Color);
      NS_ColorNameToRGB(cbuf, &(color->mBackgroundColor));
      color->mBackgroundFlags &= ~NS_STYLE_BG_COLOR_TRANSPARENT;
    }
  }
}

//----------------------------------------------------------------------

nsGenericHTMLLeafElement::nsGenericHTMLLeafElement()
{
}

nsGenericHTMLLeafElement::~nsGenericHTMLLeafElement()
{
}

nsresult
nsGenericHTMLLeafElement::CopyInnerTo(nsIContent* aSrcContent,
                                      nsGenericHTMLLeafElement* aDst)
{
  aDst->mContent = aSrcContent;
  // XXX should the node's document be set?
  // XXX copy attributes not yet impelemented
  return NS_OK;
}

nsresult
nsGenericHTMLLeafElement::GetChildNodes(nsIDOMNodeList** aChildNodes)
{
  nsDOMSlots* slots = GetDOMSlots();

  if (nsnull == slots->mChildNodes) {
    slots->mChildNodes = new nsChildContentList(nsnull);
    if (nsnull == slots->mChildNodes) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    NS_ADDREF(slots->mChildNodes);
  }

  return slots->mChildNodes->QueryInterface(kIDOMNodeListIID, (void **)aChildNodes);
}

// XXX not really implemented (yet)
nsresult
nsGenericHTMLLeafElement::SizeOf(nsISizeOfHandler* aHandler) const
{
  aHandler->Add(sizeof(*this));
  return NS_OK;
}

nsresult
nsGenericHTMLLeafElement::BeginConvertToXIF(nsXIFConverter& aConverter) const
{
  nsresult rv = NS_OK;
  if (nsnull != mTag)
  {
    nsAutoString name;
    mTag->ToString(name);
    aConverter.BeginLeaf(name);    
  }

  // Add all attributes to the convert
  if (nsnull != mAttributes) 
  {
    nsISupportsArray* attrs;
    rv = NS_NewISupportsArray(&attrs);
    if (NS_OK == rv) 
    {
      PRInt32 i, n;
      mAttributes->GetAllAttributeNames(attrs, n);
      nsAutoString name, value;
      for (i = 0; i < n; i++) 
      {
        nsIAtom* atom = (nsIAtom*) attrs->ElementAt(i);
        atom->ToString(name);

        value.Truncate();
        GetAttribute(name, value);
        
        aConverter.AddHTMLAttribute(name,value);
      }
      NS_RELEASE(attrs);
    }
  }
  return rv;
}

nsresult
nsGenericHTMLLeafElement::ConvertContentToXIF(nsXIFConverter& aConverter) const
{
  return NS_OK;
}

nsresult
nsGenericHTMLLeafElement::FinishConvertToXIF(nsXIFConverter& aConverter) const
{
  if (nsnull != mTag)
  {
    nsAutoString name;
    mTag->ToString(name);
    aConverter.EndLeaf(name);    
  }
  return NS_OK;
}

//----------------------------------------------------------------------

nsGenericHTMLContainerElement::nsGenericHTMLContainerElement()
{
}

nsGenericHTMLContainerElement::~nsGenericHTMLContainerElement()
{
  PRInt32 n = mChildren.Count();
  for (PRInt32 i = 0; i < n; i++) {
    nsIContent* kid = (nsIContent *)mChildren.ElementAt(i);
    NS_RELEASE(kid);
  }
}

nsresult
nsGenericHTMLContainerElement::CopyInnerTo(nsIContent* aSrcContent,
                                           nsGenericHTMLContainerElement* aDst)
{
  aDst->mContent = aSrcContent;
  // XXX should the node's document be set?
  // XXX copy attributes not yet impelemented
  // XXX deep copy?
  return NS_OK;
}

nsresult
nsGenericHTMLContainerElement::GetChildNodes(nsIDOMNodeList** aChildNodes)
{
  nsDOMSlots *slots = GetDOMSlots();

  if (nsnull == slots->mChildNodes) {
    slots->mChildNodes = new nsChildContentList(mContent);
    NS_ADDREF(slots->mChildNodes);
  }

  return slots->mChildNodes->QueryInterface(kIDOMNodeListIID, (void **)aChildNodes);
}

nsresult
nsGenericHTMLContainerElement::HasChildNodes(PRBool* aReturn)
{
  if (0 != mChildren.Count()) {
    *aReturn = PR_TRUE;
  } 
  else {
    *aReturn = PR_FALSE;
  }
  return NS_OK;
}

nsresult
nsGenericHTMLContainerElement::GetFirstChild(nsIDOMNode** aNode)
{
  nsIContent *child = (nsIContent *)mChildren.ElementAt(0);
  if (nsnull != child) {
    nsresult res = child->QueryInterface(kIDOMNodeIID, (void**)aNode);
    NS_ASSERTION(NS_OK == res, "Must be a DOM Node"); // must be a DOM Node
    return res;
  }
  *aNode = nsnull;
  return NS_OK;
}

nsresult
nsGenericHTMLContainerElement::GetLastChild(nsIDOMNode** aNode)
{
  nsIContent *child = (nsIContent *)mChildren.ElementAt(mChildren.Count()-1);
  if (nsnull != child) {
    nsresult res = child->QueryInterface(kIDOMNodeIID, (void**)aNode);
    NS_ASSERTION(NS_OK == res, "Must be a DOM Node"); // must be a DOM Node
    return res;
  }
  *aNode = nsnull;
  return NS_OK;
}

// XXX It's possible that newChild has already been inserted in the
// tree; if this is the case then we need to remove it from where it
// was before placing it in it's new home

nsresult
nsGenericHTMLContainerElement::InsertBefore(nsIDOMNode* aNewChild,
                                            nsIDOMNode* aRefChild,
                                            nsIDOMNode** aReturn)
{
  if (nsnull == aNewChild) {
    *aReturn = nsnull;
    return NS_OK;/* XXX wrong error value */
  }

  // Get the nsIContent interface for the new content
  nsIContent* newContent = nsnull;
  nsresult res = aNewChild->QueryInterface(kIContentIID, (void**)&newContent);
  NS_ASSERTION(NS_OK == res, "New child must be an nsIContent");
  if (NS_OK == res) {
    if (nsnull == aRefChild) {
      // Append the new child to the end
      SetDocumentInChildrenOf(newContent, mDocument);
      res = AppendChildTo(newContent, PR_TRUE);
    }
    else {
      // Get the index of where to insert the new child
      nsIContent* refContent = nsnull;
      res = aRefChild->QueryInterface(kIContentIID, (void**)&refContent);
      NS_ASSERTION(NS_OK == res, "Ref child must be an nsIContent");
      if (NS_OK == res) {
        PRInt32 pos;
        IndexOf(refContent, pos);
        if (pos >= 0) {
          SetDocumentInChildrenOf(newContent, mDocument);
          res = InsertChildAt(newContent, pos, PR_TRUE);
        }
        NS_RELEASE(refContent);
      }
    }
    NS_RELEASE(newContent);

    *aReturn = aNewChild;
    NS_ADDREF(aNewChild);
  }
  else {
    *aReturn = nsnull;
  }

  return res;
}

nsresult
nsGenericHTMLContainerElement::ReplaceChild(nsIDOMNode* aNewChild,
                                            nsIDOMNode* aOldChild,
                                            nsIDOMNode** aReturn)
{
  nsIContent* content = nsnull;
  *aReturn = nsnull;
  nsresult res = aOldChild->QueryInterface(kIContentIID, (void**)&content);
  NS_ASSERTION(NS_OK == res, "Must be an nsIContent");
  if (NS_OK == res) {
    PRInt32 pos;
    IndexOf(content, pos);
    if (pos >= 0) {
      nsIContent* newContent = nsnull;
      nsresult res = aNewChild->QueryInterface(kIContentIID, (void**)&newContent);
      NS_ASSERTION(NS_OK == res, "Must be an nsIContent");
      if (NS_OK == res) {
        res = ReplaceChildAt(newContent, pos, PR_TRUE);
        NS_RELEASE(newContent);
      }
      *aReturn = aOldChild;
      NS_ADDREF(aOldChild);
    }
    NS_RELEASE(content);
  }
  
  return res;
}

nsresult
nsGenericHTMLContainerElement::RemoveChild(nsIDOMNode* aOldChild, 
                                           nsIDOMNode** aReturn)
{
  nsIContent* content = nsnull;
  *aReturn = nsnull;
  nsresult res = aOldChild->QueryInterface(kIContentIID, (void**)&content);
  NS_ASSERTION(NS_OK == res, "Must be an nsIContent");
  if (NS_OK == res) {
    PRInt32 pos;
    IndexOf(content, pos);
    if (pos >= 0) {
      res = RemoveChildAt(pos, PR_TRUE);
      *aReturn = aOldChild;
      NS_ADDREF(aOldChild);
    }
    NS_RELEASE(content);
  }

  return res;
}

nsresult
nsGenericHTMLContainerElement::AppendChild(nsIDOMNode* aNewChild, nsIDOMNode** aReturn)
{
  return InsertBefore(aNewChild, nsnull, aReturn);
}

nsresult
nsGenericHTMLContainerElement::BeginConvertToXIF(nsXIFConverter& aConverter) const
{
  nsresult rv = NS_OK;
  if (nsnull != mTag)
  {
    nsAutoString name;
    mTag->ToString(name);
    aConverter.BeginContainer(name);
  }

  // Add all attributes to the convert
  if (nsnull != mAttributes) 
  {
    nsISupportsArray* attrs;
    rv = NS_NewISupportsArray(&attrs);
    if (NS_OK == rv) 
    {
      PRInt32 i, n;
      mAttributes->GetAllAttributeNames(attrs, n);
      nsAutoString name, value;
      for (i = 0; i < n; i++) 
      {
        nsIAtom* atom = (nsIAtom*) attrs->ElementAt(i);
        atom->ToString(name);

        value.Truncate();
        GetAttribute(name, value);
        
        aConverter.AddHTMLAttribute(name,value);
      }
      NS_RELEASE(attrs);
    }
  }
  return NS_OK;
}

nsresult
nsGenericHTMLContainerElement::ConvertContentToXIF(nsXIFConverter& aConverter) const
{
  return NS_OK;
}

nsresult
nsGenericHTMLContainerElement::FinishConvertToXIF(nsXIFConverter& aConverter) const
{
  if (nsnull != mTag)
  {
    nsAutoString name;
    mTag->ToString(name);
    aConverter.EndContainer(name);
  }
  return NS_OK;
}

nsresult
nsGenericHTMLContainerElement::Compact()
{
  mChildren.Compact();
  return NS_OK;
}

nsresult
nsGenericHTMLContainerElement::SizeOf(nsISizeOfHandler* aHandler) const
{
  aHandler->Add(sizeof(*this));
  return NS_OK;
}

nsresult
nsGenericHTMLContainerElement::CanContainChildren(PRBool& aResult) const
{
  aResult = PR_TRUE;
  return NS_OK;
}

nsresult
nsGenericHTMLContainerElement::ChildCount(PRInt32& aCount) const
{
  aCount = mChildren.Count();
  return NS_OK;
}

nsresult
nsGenericHTMLContainerElement::ChildAt(PRInt32 aIndex,
                                       nsIContent*& aResult) const
{
  nsIContent *child = (nsIContent *)mChildren.ElementAt(aIndex);
  if (nsnull != child) {
    NS_ADDREF(child);
  }
  aResult = child;
  return NS_OK;
}

nsresult
nsGenericHTMLContainerElement::IndexOf(nsIContent* aPossibleChild,
                                       PRInt32& aIndex) const
{
  NS_PRECONDITION(nsnull != aPossibleChild, "null ptr");
  aIndex = mChildren.IndexOf(aPossibleChild);
  return NS_OK;
}

nsresult
nsGenericHTMLContainerElement::InsertChildAt(nsIContent* aKid,
                                             PRInt32 aIndex,
                                             PRBool aNotify)
{
  NS_PRECONDITION(nsnull != aKid, "null ptr");
  PRBool rv = mChildren.InsertElementAt(aKid, aIndex);/* XXX fix up void array api to use nsresult's*/
  if (rv) {
    NS_ADDREF(aKid);
    aKid->SetParent(mContent);
    nsIDocument* doc = mDocument;
    if (nsnull != doc) {
      aKid->SetDocument(doc, PR_FALSE);
      if (aNotify) {
        doc->ContentInserted(mContent, aKid, aIndex);
      }
    }
  }
  return NS_OK;
}

nsresult
nsGenericHTMLContainerElement::ReplaceChildAt(nsIContent* aKid,
                                              PRInt32 aIndex,
                                              PRBool aNotify)
{
  NS_PRECONDITION(nsnull != aKid, "null ptr");
  nsIContent* oldKid = (nsIContent *)mChildren.ElementAt(aIndex);
  PRBool rv = mChildren.ReplaceElementAt(aKid, aIndex);
  if (rv) {
    NS_ADDREF(aKid);
    aKid->SetParent(mContent);
    nsIDocument* doc = mDocument;
    if (nsnull != doc) {
      aKid->SetDocument(doc, PR_FALSE);
      if (aNotify) {
        doc->ContentReplaced(mContent, oldKid, aKid, aIndex);
      }
    }
    oldKid->SetDocument(nsnull, PR_TRUE);
    oldKid->SetParent(nsnull);
    NS_RELEASE(oldKid);
  }
  return NS_OK;
}

nsresult
nsGenericHTMLContainerElement::AppendChildTo(nsIContent* aKid, PRBool aNotify)
{
  NS_PRECONDITION((nsnull != aKid) && (aKid != mContent), "null ptr");
  PRBool rv = mChildren.AppendElement(aKid);
  if (rv) {
    NS_ADDREF(aKid);
    aKid->SetParent(mContent);
    nsIDocument* doc = mDocument;
    if (nsnull != doc) {
      aKid->SetDocument(doc, PR_FALSE);
      if (aNotify) {
        doc->ContentAppended(mContent, mChildren.Count() - 1);
      }
    }
  }
  return NS_OK;
}

nsresult
nsGenericHTMLContainerElement::RemoveChildAt(PRInt32 aIndex, PRBool aNotify)
{
  nsIContent* oldKid = (nsIContent *)mChildren.ElementAt(aIndex);
  if (nsnull != oldKid ) {
    nsIDocument* doc = mDocument;
    PRBool rv = mChildren.RemoveElementAt(aIndex);
    if (aNotify) {
      if (nsnull != doc) {
        doc->ContentRemoved(mContent, oldKid, aIndex);
      }
    }
    oldKid->SetDocument(nsnull, PR_TRUE);
    oldKid->SetParent(nsnull);
    NS_RELEASE(oldKid);
  }

  return NS_OK;
}

