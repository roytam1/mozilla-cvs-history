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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
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
#include "nsIDOMHTMLTableSectionElem.h"
#include "nsIDOMEventReceiver.h"
#include "nsIHTMLContent.h"
#include "nsHTMLAttributes.h"
#include "nsGenericHTMLElement.h"
#include "nsHTMLAtoms.h"
#include "nsHTMLParts.h"
#include "nsStyleConsts.h"
#include "nsIPresContext.h"
#include "GenericElementCollection.h"
#include "nsRuleNode.h"
#include "nsDOMError.h"

// you will see the phrases "rowgroup" and "section" used interchangably

class nsHTMLTableSectionElement : public nsGenericHTMLContainerElement,
                                  public nsIDOMHTMLTableSectionElement
{
public:
  nsHTMLTableSectionElement();
  virtual ~nsHTMLTableSectionElement();

  // nsISupports
  NS_DECL_ISUPPORTS_INHERITED

  // nsIDOMNode
  NS_FORWARD_NSIDOMNODE_NO_CLONENODE(nsGenericHTMLContainerElement::)

  // nsIDOMElement
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLContainerElement::)

  // nsIDOMHTMLElement
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLContainerElement::)

  // nsIDOMHTMLTableSectionElement
  NS_DECL_NSIDOMHTMLTABLESECTIONELEMENT

  NS_IMETHOD StringToAttribute(nsIAtom* aAttribute,
                               const nsAString& aValue,
                               nsHTMLValue& aResult);
  NS_IMETHOD AttributeToString(nsIAtom* aAttribute,
                               const nsHTMLValue& aValue,
                               nsAString& aResult) const;
  NS_IMETHOD GetAttributeMappingFunction(nsMapRuleToAttributesFunc& aMapRuleFunc) const;
  NS_IMETHOD GetMappedAttributeImpact(const nsIAtom* aAttribute, PRInt32 aModType,
                                      nsChangeHint& aHint) const;

protected:
  GenericElementCollection *mRows;
};

nsresult
NS_NewHTMLTableSectionElement(nsIHTMLContent** aInstancePtrResult,
                              nsINodeInfo *aNodeInfo)
{
  NS_ENSURE_ARG_POINTER(aInstancePtrResult);

  nsHTMLTableSectionElement* it = new nsHTMLTableSectionElement();

  if (!it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  nsresult rv = it->Init(aNodeInfo);

  if (NS_FAILED(rv)) {
    delete it;

    return rv;
  }

  *aInstancePtrResult = NS_STATIC_CAST(nsIHTMLContent *, it);
  NS_ADDREF(*aInstancePtrResult);

  return NS_OK;
}


nsHTMLTableSectionElement::nsHTMLTableSectionElement()
{
  mRows = nsnull;
}

nsHTMLTableSectionElement::~nsHTMLTableSectionElement()
{
  if (nsnull!=mRows) {
    mRows->ParentDestroyed();
    NS_RELEASE(mRows);
  }
}


NS_IMPL_ADDREF_INHERITED(nsHTMLTableSectionElement, nsGenericElement) 
NS_IMPL_RELEASE_INHERITED(nsHTMLTableSectionElement, nsGenericElement) 


// QueryInterface implementation for nsHTMLTableSectionElement
NS_HTML_CONTENT_INTERFACE_MAP_BEGIN(nsHTMLTableSectionElement,
                                    nsGenericHTMLContainerElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMHTMLTableSectionElement)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(HTMLTableSectionElement)
NS_HTML_CONTENT_INTERFACE_MAP_END


nsresult
nsHTMLTableSectionElement::CloneNode(PRBool aDeep, nsIDOMNode** aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);
  *aReturn = nsnull;

  nsHTMLTableSectionElement* it = new nsHTMLTableSectionElement();

  if (!it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  nsCOMPtr<nsIDOMNode> kungFuDeathGrip(it);

  nsresult rv = it->Init(mNodeInfo);

  if (NS_FAILED(rv))
    return rv;

  CopyInnerTo(this, it, aDeep);

  *aReturn = NS_STATIC_CAST(nsIDOMNode *, it);

  NS_ADDREF(*aReturn);

  return NS_OK;
}


NS_IMPL_STRING_ATTR_DEFAULT_VALUE(nsHTMLTableSectionElement, Align, align, "left")
NS_IMPL_STRING_ATTR_DEFAULT_VALUE(nsHTMLTableSectionElement, VAlign, valign, "middle")
NS_IMPL_STRING_ATTR_DEFAULT_VALUE(nsHTMLTableSectionElement, Ch, _char, ".")
NS_IMPL_STRING_ATTR(nsHTMLTableSectionElement, ChOff, charoff)


NS_IMETHODIMP
nsHTMLTableSectionElement::GetRows(nsIDOMHTMLCollection** aValue)
{
  *aValue = nsnull;

  if (!mRows) {
    //XXX why was this here NS_ADDREF(nsHTMLAtoms::tr);
    mRows = new GenericElementCollection(this, nsHTMLAtoms::tr);

    NS_ENSURE_TRUE(mRows, NS_ERROR_OUT_OF_MEMORY);

    NS_ADDREF(mRows); // this table's reference, released in the destructor
  }

  return CallQueryInterface(mRows, aValue);
}


NS_IMETHODIMP
nsHTMLTableSectionElement::InsertRow(PRInt32 aIndex,
                                     nsIDOMHTMLElement** aValue)
{
  *aValue = nsnull;

  if (aIndex < -1) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  nsCOMPtr<nsIDOMHTMLCollection> rows;
  GetRows(getter_AddRefs(rows));

  PRUint32 rowCount;
  rows->GetLength(&rowCount);

  if (aIndex > (PRInt32)rowCount) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  PRBool doInsert = (aIndex < PRInt32(rowCount)) && (aIndex != -1);

  // create the row
  nsCOMPtr<nsINodeInfo> nodeInfo;
  mNodeInfo->NameChanged(nsHTMLAtoms::tr, *getter_AddRefs(nodeInfo));

  nsCOMPtr<nsIHTMLContent> rowContent;
  nsresult rv = NS_NewHTMLTableRowElement(getter_AddRefs(rowContent),
                                          nodeInfo);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMNode> rowNode(do_QueryInterface(rowContent));
  NS_ASSERTION(rowNode, "Should implement nsIDOMNode!");

  nsCOMPtr<nsIDOMNode> retChild;

  if (doInsert) {
    nsCOMPtr<nsIDOMNode> refRow;
    rows->Item(aIndex, getter_AddRefs(refRow));

    rv = InsertBefore(rowNode, refRow, getter_AddRefs(retChild));
  } else {
    rv = AppendChild(rowNode, getter_AddRefs(retChild));
  }

  if (retChild) {
    CallQueryInterface(retChild, aValue);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLTableSectionElement::DeleteRow(PRInt32 aValue)
{
  if (aValue < -1) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  nsCOMPtr<nsIDOMHTMLCollection> rows;
  GetRows(getter_AddRefs(rows));

  nsresult rv;
  PRUint32 refIndex;
  if (aValue == -1) {
    rv = rows->GetLength(&refIndex);
    NS_ENSURE_SUCCESS(rv, rv);

    if (refIndex == 0) {
      return NS_OK;
    }

    --refIndex;
  }
  else {
    refIndex = (PRUint32)aValue;
  }

  nsCOMPtr<nsIDOMNode> row;
  rv = rows->Item(aValue, getter_AddRefs(row));
  NS_ENSURE_SUCCESS(rv, rv);

  if (!row) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  nsCOMPtr<nsIDOMNode> retChild;
  return RemoveChild(row, getter_AddRefs(retChild));
}

NS_IMETHODIMP
nsHTMLTableSectionElement::StringToAttribute(nsIAtom* aAttribute,
                                             const nsAString& aValue,
                                             nsHTMLValue& aResult)
{
  /* ignore these attributes, stored simply as strings
     ch
   */
  /* attributes that resolve to integers */
  if (aAttribute == nsHTMLAtoms::charoff) {
    if (aResult.ParseIntWithBounds(aValue, eHTMLUnit_Integer, 0)) {
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }
  else if (aAttribute == nsHTMLAtoms::height) {
    /* attributes that resolve to integers or percents */

    if (aResult.ParseSpecialIntValue(aValue, eHTMLUnit_Pixel, PR_TRUE, PR_FALSE)) {
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }
  else if (aAttribute == nsHTMLAtoms::align) {
    /* other attributes */

    if (ParseTableCellHAlignValue(aValue, aResult)) {
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }
  else if (aAttribute == nsHTMLAtoms::bgcolor) {
    if (aResult.ParseColor(aValue, mDocument)) {
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }
  else if (aAttribute == nsHTMLAtoms::valign) {
    if (ParseTableVAlignValue(aValue, aResult)) {
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }

  return NS_CONTENT_ATTR_NOT_THERE;
}

NS_IMETHODIMP
nsHTMLTableSectionElement::AttributeToString(nsIAtom* aAttribute,
                                             const nsHTMLValue& aValue,
                                             nsAString& aResult) const
{
  /* ignore these attributes, stored already as strings
     ch
   */
  /* ignore attributes that are of standard types
     charoff, height, width, background, bgcolor
   */
  if (aAttribute == nsHTMLAtoms::align) {
    if (TableCellHAlignValueToString(aValue, aResult)) {
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }
  else if (aAttribute == nsHTMLAtoms::valign) {
    if (TableVAlignValueToString(aValue, aResult)) {
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }

  return nsGenericHTMLContainerElement::AttributeToString(aAttribute, aValue,
                                                          aResult);
}

static 
void MapAttributesIntoRule(const nsIHTMLMappedAttributes* aAttributes, nsRuleData* aData)
{
  if (!aAttributes || !aData)
    return;

  if (aData->mPositionData) {
    // height: value
    nsHTMLValue value;
    if (aData->mPositionData->mHeight.GetUnit() == eCSSUnit_Null) {
      aAttributes->GetAttribute(nsHTMLAtoms::height, value);
      if (value.GetUnit() == eHTMLUnit_Pixel)
        aData->mPositionData->mHeight.SetFloatValue((float)value.GetPixelValue(), eCSSUnit_Pixel);   
    }
  }
  else if (aData->mTextData) {
    if (aData->mSID == eStyleStruct_Text) {
      if (aData->mTextData->mTextAlign.GetUnit() == eCSSUnit_Null) {
        // align: enum
        nsHTMLValue value;
        aAttributes->GetAttribute(nsHTMLAtoms::align, value);
        if (value.GetUnit() == eHTMLUnit_Enumerated)
          aData->mTextData->mTextAlign.SetIntValue(value.GetIntValue(), eCSSUnit_Enumerated);
      }
    }
    else {
      if (aData->mTextData->mVerticalAlign.GetUnit() == eCSSUnit_Null) {
        // valign: enum
        nsHTMLValue value;
        aAttributes->GetAttribute(nsHTMLAtoms::valign, value);
        if (value.GetUnit() == eHTMLUnit_Enumerated) 
          aData->mTextData->mVerticalAlign.SetIntValue(value.GetIntValue(), eCSSUnit_Enumerated);
      }
    }
  }

  nsGenericHTMLElement::MapBackgroundAttributesInto(aAttributes, aData);
  nsGenericHTMLElement::MapCommonAttributesInto(aAttributes, aData);
}

NS_IMETHODIMP
nsHTMLTableSectionElement::GetMappedAttributeImpact(const nsIAtom* aAttribute, PRInt32 aModType,
                                                    nsChangeHint& aHint) const
{
  static const AttributeImpactEntry attributes[] = {
    { &nsHTMLAtoms::align, NS_STYLE_HINT_REFLOW }, 
    { &nsHTMLAtoms::valign, NS_STYLE_HINT_REFLOW },
    { &nsHTMLAtoms::height, NS_STYLE_HINT_REFLOW },
    { nsnull, NS_STYLE_HINT_NONE }
  };

  static const AttributeImpactEntry* const map[] = {
    attributes,
    sCommonAttributeMap,
    sBackgroundAttributeMap,
  };

  FindAttributeImpact(aAttribute, aHint, map, NS_ARRAY_LENGTH(map));
  return NS_OK;
}


NS_IMETHODIMP
nsHTMLTableSectionElement::GetAttributeMappingFunction(nsMapRuleToAttributesFunc& aMapRuleFunc) const
{
  aMapRuleFunc = &MapAttributesIntoRule;
  return NS_OK;
}
