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
#include "nsHTMLTagContent.h"
#include "nsIDocument.h"
#include "nsIHTMLAttributes.h"
#include "nsIStyleRule.h"
#include "nsHTMLAtoms.h"
#include "nsStyleConsts.h"
#include "nsString.h"
#include "prprf.h"
#include "nsDOMAttributes.h"

static NS_DEFINE_IID(kIStyleRuleIID, NS_ISTYLE_RULE_IID);
static NS_DEFINE_IID(kIDOMElementIID, NS_IDOMELEMENT_IID);
static NS_DEFINE_IID(kIScriptObjectOwner, NS_ISCRIPTOBJECTOWNER_IID);

nsHTMLTagContent::nsHTMLTagContent()
{
}

nsHTMLTagContent::nsHTMLTagContent(nsIAtom* aTag)
{
  NS_PRECONDITION(nsnull != aTag, "null ptr");
  mTag = aTag;
  NS_IF_ADDREF(mTag);
}

nsHTMLTagContent::~nsHTMLTagContent()
{
  NS_IF_RELEASE(mTag);
  NS_IF_RELEASE(mAttributes);
}

nsresult nsHTMLTagContent::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  nsresult res = nsHTMLContent::QueryInterface(aIID, aInstancePtr); 
  if (NS_NOINTERFACE == res) {
    if (aIID.Equals(kIDOMElementIID)) {
      *aInstancePtr = (void*)(nsIDOMElement*)this;
      AddRef();
      return NS_OK;
    }
  }

  return res;
}

nsrefcnt nsHTMLTagContent::AddRef(void)
{
  return nsHTMLContent::AddRef(); 
}

nsrefcnt nsHTMLTagContent::Release(void)
{
  return nsHTMLContent::Release(); 
}

nsIAtom* nsHTMLTagContent::GetTag() const
{
  NS_IF_ADDREF(mTag);
  return mTag;
}

void nsHTMLTagContent::ToHTMLString(nsString& aBuf) const
{
  aBuf.SetLength(0);
  aBuf.Append('<');

  nsIAtom* tag = GetTag();
  if (nsnull != tag) {
    nsAutoString tmp;
    tag->ToString(tmp);
    aBuf.Append(tmp);
  } else {
    aBuf.Append("?NULL");
  }
  aBuf.Append('>');
}

void nsHTMLTagContent::SetAttribute(const nsString& aName,
                                    const nsString& aValue)
{
  nsAutoString upper;
  aName.ToUpperCase(upper);
  nsIAtom* attr = NS_NewAtom(upper);
  SetAttribute(attr, aValue);
  NS_RELEASE(attr);
}

nsContentAttr nsHTMLTagContent::GetAttribute(const nsString& aName,
                                             nsString& aResult) const
{
  nsAutoString upper;
  aName.ToUpperCase(upper);
  nsIAtom* attr = NS_NewAtom(upper);

  nsHTMLValue value;
  nsContentAttr result = GetAttribute(attr, value);

  char cbuf[20];
  nscolor color;
  if (eContentAttr_HasValue == result) {
    // Try subclass conversion routine first
    if (eContentAttr_HasValue == AttributeToString(attr, value, aResult)) {
      return result;
    }

    // Provide default conversions for most everything
    switch (value.GetUnit()) {
    case eHTMLUnit_String:
    case eHTMLUnit_Null:
      value.GetStringValue(aResult);
      break;

    case eHTMLUnit_Absolute:
    case eHTMLUnit_Pixel:
      aResult.SetLength(0);
      aResult.Append(value.GetIntValue(), 10);
      break;

    case eHTMLUnit_Percent:
      aResult.SetLength(0);
      aResult.Append(PRInt32(value.GetFloatValue() * 100.0f), 10);
      break;

    case eHTMLUnit_Color:
      color = nscolor(value.GetColorValue());
      PR_snprintf(cbuf, sizeof(cbuf), "#%02x%02x%02x",
                  NS_GET_R(color), NS_GET_G(color), NS_GET_B(color));
      aResult.SetLength(0);
      aResult.Append(cbuf);
      break;

    case eHTMLUnit_Enumerated:
      NS_NOTREACHED("no default enumerated value to string conversion");
      result = eContentAttr_NotThere;
      break;
    }
  }

  NS_RELEASE(attr);
  return result;
}

nsContentAttr nsHTMLTagContent::AttributeToString(nsIAtom* aAttribute,
                                                  nsHTMLValue& aValue,
                                                  nsString& aResult) const
{
  aResult.Truncate();
  return eContentAttr_NotThere;
}

// XXX subclasses should override to parse the value string
void nsHTMLTagContent::SetAttribute(nsIAtom* aAttribute,
                                    const nsString& aValue)
{
  if (nsnull == mAttributes) {
    NS_NewHTMLAttributes(&mAttributes, this);
  }
  if (nsnull != mAttributes) {
    mAttributes->SetAttribute(aAttribute, aValue);
  }
}

void nsHTMLTagContent::SetAttribute(nsIAtom* aAttribute,
                                    const nsHTMLValue& aValue)
{
  if (nsnull == mAttributes) {
    NS_NewHTMLAttributes(&mAttributes, this);
  }
  if (nsnull != mAttributes) {
    mAttributes->SetAttribute(aAttribute, aValue);
  }
}

void nsHTMLTagContent::UnsetAttribute(nsIAtom* aAttribute)
{
  if (nsnull != mAttributes) {
    PRInt32 count = mAttributes->UnsetAttribute(aAttribute);
    if (0 == count) {
      NS_RELEASE(mAttributes);
    }
  }
}

nsContentAttr nsHTMLTagContent::GetAttribute(nsIAtom* aAttribute,
                                             nsHTMLValue& aValue) const
{
  if (nsnull != mAttributes) {
    return mAttributes->GetAttribute(aAttribute, aValue);
  }
  aValue.Reset();
  return eContentAttr_NotThere;
}

PRInt32 nsHTMLTagContent::GetAllAttributeNames(nsISupportsArray* aArray) const
{
  if (nsnull != mAttributes) {
    return mAttributes->GetAllAttributeNames(aArray);
  }
  return 0;
}

PRInt32 nsHTMLTagContent::GetAttributeCount(void) const
{
  if (nsnull != mAttributes) {
    return mAttributes->Count();
  }
  return 0;
}

void nsHTMLTagContent::SetID(nsIAtom* aID)
{
  if (nsnull != aID) {
    if (nsnull == mAttributes) {
      NS_NewHTMLAttributes(&mAttributes, this);
    }
    if (nsnull != mAttributes) {
      mAttributes->SetID(aID);
    }
  }
  else {
    if (nsnull != mAttributes) {
      PRInt32 count = mAttributes->SetID(nsnull);
      if (0 == count) {
        NS_RELEASE(mAttributes);
      }
    }
  }
}

nsIAtom* nsHTMLTagContent::GetID(void) const
{
  if (nsnull != mAttributes) {
    return mAttributes->GetID();
  }
  return nsnull;
}

void nsHTMLTagContent::SetClass(nsIAtom* aClass)
{
  if (nsnull != aClass) {
    if (nsnull == mAttributes) {
      NS_NewHTMLAttributes(&mAttributes, this);
    }
    if (nsnull != mAttributes) {
      mAttributes->SetClass(aClass);
    }
  }
  else {
    if (nsnull != mAttributes) {
      PRInt32 count = mAttributes->SetClass(nsnull);
      if (0 == count) {
        NS_RELEASE(mAttributes);
      }
    }
  }
}

nsIAtom* nsHTMLTagContent::GetClass(void) const
{
  if (nsnull != mAttributes) {
    return mAttributes->GetClass();
  }
  return nsnull;
}


nsIStyleRule* nsHTMLTagContent::GetStyleRule(void)
{
  nsIStyleRule* result = nsnull;

  if (nsnull != mAttributes) {
    mAttributes->QueryInterface(kIStyleRuleIID, (void**)&result);
  }
  return result;
}

nsresult nsHTMLTagContent::GetScriptObject(JSContext *aContext, void** aScriptObject)
{
  nsresult res = NS_OK;
  if (nsnull == mScriptObject) {
    *aScriptObject = nsnull;
    if (nsnull != mParent) {
      nsIScriptObjectOwner *parent;
      if (NS_OK == mParent->QueryInterface(kIScriptObjectOwner, (void**)&parent)) {
        parent->GetScriptObject(aContext, aScriptObject);
        NS_RELEASE(parent);
      }
    }
    res = NS_NewScriptElement(aContext, this, (JSObject*)*aScriptObject, (JSObject**)&mScriptObject);
  }
  *aScriptObject = mScriptObject;
  return res;
}

nsresult nsHTMLTagContent::GetNodeType(PRInt32 *aType)
{
  *aType = nsHTMLContent::ELEMENT;
  return NS_OK;
}

nsresult nsHTMLTagContent::GetParentNode(nsIDOMNode **aNode)
{
  return nsHTMLContent::GetParentNode(aNode);
}

nsresult nsHTMLTagContent::GetChildNodes(nsIDOMNodeIterator **aIterator)
{
  return nsHTMLContent::GetChildNodes(aIterator);
}

nsresult nsHTMLTagContent::HasChildNodes()
{
  return nsHTMLContent::HasChildNodes();
}

nsresult nsHTMLTagContent::GetFirstChild(nsIDOMNode **aNode)
{
  return nsHTMLContent::GetFirstChild(aNode);
}

nsresult nsHTMLTagContent::GetPreviousSibling(nsIDOMNode **aNode)
{
  return nsHTMLContent::GetPreviousSibling(aNode);
}

nsresult nsHTMLTagContent::GetNextSibling(nsIDOMNode **aNode)
{
  return nsHTMLContent::GetNextSibling(aNode);
}

nsresult nsHTMLTagContent::InsertBefore(nsIDOMNode *newChild, nsIDOMNode *refChild)
{
  return nsHTMLContent::InsertBefore(newChild, refChild);
}

nsresult nsHTMLTagContent::ReplaceChild(nsIDOMNode *newChild, nsIDOMNode *oldChild)
{
  return nsHTMLContent::ReplaceChild(newChild, oldChild);
}

nsresult nsHTMLTagContent::RemoveChild(nsIDOMNode *oldChild)
{
  return nsHTMLContent::RemoveChild(oldChild);
}

nsresult nsHTMLTagContent::GetTagName(nsString &aName)
{
  NS_ASSERTION(nsnull != mTag, "no tag");
  mTag->ToString(aName);
  return NS_OK;
}

nsresult nsHTMLTagContent::GetAttributes(nsIDOMAttributeList **aAttributeList)
{
  NS_PRECONDITION(nsnull != aAttributeList, "null pointer argument");
  if (nsnull != mAttributes) {
    *aAttributeList = new nsDOMAttributeList(*this);
    return NS_OK;
  }

  return NS_ERROR_FAILURE;
}

nsresult nsHTMLTagContent::GetDOMAttribute(nsString &aName, nsString &aValue)
{
  GetAttribute(aName, aValue);
  return NS_OK;
}

nsresult nsHTMLTagContent::SetDOMAttribute(nsString &aName, nsString &aValue)
{
  SetAttribute(aName, aValue);
  return NS_OK;
}

nsresult nsHTMLTagContent::RemoveAttribute(nsString &aName)
{
  nsAutoString upper;
  aName.ToUpperCase(upper);
  nsIAtom* attr = NS_NewAtom(upper);
  UnsetAttribute(attr);
  return NS_OK;
}

nsresult nsHTMLTagContent::GetAttributeNode(nsString &aName, nsIDOMAttribute **aAttribute)
{
  nsAutoString value;
  if(eContentAttr_NotThere != GetAttribute(aName, value)) {
    *aAttribute = new nsDOMAttribute(aName, value);
  }

  return NS_OK;
}

nsresult nsHTMLTagContent::SetAttributeNode(nsIDOMAttribute *aAttribute)
{
  NS_PRECONDITION(nsnull != aAttribute, "null attribute");
  
  nsresult res = NS_ERROR_FAILURE;

  if (nsnull != aAttribute) {
    nsString name, value;
    res = aAttribute->GetName(name);
    if (NS_OK == res) {
      res = aAttribute->GetValue(value);
      if (NS_OK == res) {
        SetAttribute(name, value);
      }
    }
  }

  return res;
}

nsresult nsHTMLTagContent::RemoveAttributeNode(nsIDOMAttribute *aAttribute)
{
  NS_PRECONDITION(nsnull != aAttribute, "null attribute");
  
  nsresult res = NS_ERROR_FAILURE;

  if (nsnull != aAttribute) {
    nsAutoString name;
    res = aAttribute->GetName(name);
    if (NS_OK == res) {
      nsAutoString upper;
      name.ToUpperCase(upper);
      nsIAtom* attr = NS_NewAtom(upper);
      UnsetAttribute(attr);
    }
  }

  return res;
}

nsresult nsHTMLTagContent::Normalize()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult nsHTMLTagContent::GetElementsByTagName(nsString &aName,nsIDOMNodeIterator **aIterator)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


//----------------------------------------------------------------------

// Attribute parsing utility methods

PRBool nsHTMLTagContent::ParseEnumValue(const nsString& aValue,
                                        EnumTable* aTable,
                                        nsHTMLValue& aResult)
{
  while (nsnull != aTable->tag) {
    if (aValue.EqualsIgnoreCase(aTable->tag)) {
      aResult.Set(aTable->value, eHTMLUnit_Enumerated);
      return PR_TRUE;
    }
    aTable++;
  }
  return PR_FALSE;
}

PRBool nsHTMLTagContent::EnumValueToString(const nsHTMLValue& aValue,
                                           EnumTable* aTable,
                                           nsString& aResult)
{
  aResult.SetLength(0);
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

// XXX check all mappings against ebina's usage
static nsHTMLTagContent::EnumTable kAlignTable[] = {
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

PRBool nsHTMLTagContent::ParseAlignParam(const nsString& aString,
                                       nsHTMLValue& aResult)
{
  return ParseEnumValue(aString, kAlignTable, aResult);
}

PRBool nsHTMLTagContent::AlignParamToString(const nsHTMLValue& aValue,
                                            nsString& aResult)
{
  return EnumValueToString(aValue, kAlignTable, aResult);
}

static nsHTMLTagContent::EnumTable kDivAlignTable[] = {
  { "left", NS_STYLE_TEXT_ALIGN_LEFT },
  { "right", NS_STYLE_TEXT_ALIGN_RIGHT },
  { "center", NS_STYLE_TEXT_ALIGN_CENTER },
  { "justify", NS_STYLE_TEXT_ALIGN_JUSTIFY },
  { 0 }
};

PRBool nsHTMLTagContent::ParseDivAlignParam(const nsString& aString,
                                            nsHTMLValue& aResult)
{
  return ParseEnumValue(aString, kDivAlignTable, aResult);
}

PRBool nsHTMLTagContent::DivAlignParamToString(const nsHTMLValue& aValue,
                                               nsString& aResult)
{
  return EnumValueToString(aValue, kDivAlignTable, aResult);
}

void nsHTMLTagContent::ParseValueOrPercent(const nsString& aString,
                                           nsHTMLValue& aResult)
{
  nsAutoString tmp(aString);
  tmp.CompressWhitespace(PR_TRUE, PR_TRUE);
  PRInt32 ec, val = tmp.ToInteger(&ec);
  if (tmp.Last() == '%') {/* XXX not 100% compatible with ebina's code */
    if (val < 0) val = 0;
    if (val > 100) val = 100;
    aResult.Set(float(val)/100.0f, eHTMLUnit_Percent);
  } else {
    aResult.Set(val, eHTMLUnit_Absolute);
  }
}

PRBool nsHTMLTagContent::ValueOrPercentToString(const nsHTMLValue& aValue,
                                                nsString& aResult)
{
  aResult.SetLength(0);
  switch (aValue.GetUnit()) {
  case eHTMLUnit_Absolute:
    aResult.Append(aValue.GetIntValue(), 10);
    return PR_TRUE;
  case eHTMLUnit_Percent:
    aResult.Append(PRInt32(aValue.GetFloatValue() * 100.0f), 10);
    aResult.Append('%');
    return PR_TRUE;
  }
  return PR_FALSE;
}

void nsHTMLTagContent::ParseValue(const nsString& aString, PRInt32 aMin,
                                  nsHTMLValue& aResult)
{
  PRInt32 ec, val = aString.ToInteger(&ec);
  if (val < aMin) val = aMin;
  aResult.Set(val, eHTMLUnit_Absolute);
}

void nsHTMLTagContent::ParseValue(const nsString& aString, PRInt32 aMin,
                                  PRInt32 aMax,
                                  nsHTMLValue& aResult)
{
  PRInt32 ec, val = aString.ToInteger(&ec);
  if (val < aMin) val = aMin;
  if (val > aMax) val = aMax;
  aResult.Set(val, eHTMLUnit_Absolute);
}

PRBool nsHTMLTagContent::ParseImageProperty(nsIAtom* aAttribute,
                                            const nsString& aString,
                                            nsHTMLValue& aResult)
{
  if ((aAttribute == nsHTMLAtoms::width) ||
      (aAttribute == nsHTMLAtoms::height)) {
    ParseValueOrPercent(aString, aResult);
    return PR_TRUE;
  } else if ((aAttribute == nsHTMLAtoms::border) ||
             (aAttribute == nsHTMLAtoms::hspace) ||
             (aAttribute == nsHTMLAtoms::vspace)) {
    ParseValue(aString, 0, aResult);
    return PR_TRUE;
  }
  return PR_FALSE;
}

PRBool nsHTMLTagContent::ImagePropertyToString(nsIAtom* aAttribute,
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

PRBool nsHTMLTagContent::ParseColor(const nsString& aString,
                                    nsHTMLValue& aResult)
{
  char cbuf[40];
  aString.ToCString(cbuf, sizeof(cbuf));
  if (aString.Length() > 0) {
    nscolor color;
    if (cbuf[0] == '#') {
      if (NS_HexToRGB(cbuf, &color)) {
        aResult.Set(color);
        return PR_TRUE;
      }
    } else {
      if (NS_ColorNameToRGB(cbuf, &color)) {
        aResult.Set(aString);
        return PR_TRUE;
      }
    }
  }

  // Illegal values are mapped to 0
  aResult.Set(0, eHTMLUnit_Absolute);
  return PR_FALSE;
}

PRBool nsHTMLTagContent::ColorToString(const nsHTMLValue& aValue,
                                       nsString& aResult)
{
  if (aValue.GetUnit() == eHTMLUnit_Absolute) {
    nscolor v = (nscolor) aValue.GetIntValue();
    char buf[10];
    PR_snprintf(buf, sizeof(buf), "#%02x%02x%02x",
                NS_GET_R(v), NS_GET_G(v), NS_GET_B(v));
    aResult.SetLength(0);
    aResult.Append(buf);
    return PR_TRUE;
  }
  if (aValue.GetUnit() == eHTMLUnit_String) {
    aValue.GetStringValue(aResult);
    return PR_TRUE;
  }
  return PR_FALSE;
}
