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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */
#include "nsCOMPtr.h"
#include "nsIDOMHTMLFontElement.h"
#include "nsIDOMEventReceiver.h"
#include "nsIHTMLContent.h"
#include "nsGenericHTMLElement.h"
#include "nsHTMLAtoms.h"
#include "nsHTMLIIDs.h"
#include "nsIDeviceContext.h"
#include "nsIStyleContext.h"
#include "nsIMutableStyleContext.h"
#include "nsStyleConsts.h"
#include "nsStyleUtil.h"
#include "nsIPresContext.h"
#include "nsIHTMLAttributes.h"

// MJA: bug 31816
#include "nsIPresShell.h"
#include "nsIDocShellTreeItem.h"
// - END MJA

class nsHTMLFontElement : public nsGenericHTMLContainerElement,
                          public nsIDOMHTMLFontElement
{
public:
  nsHTMLFontElement();
  virtual ~nsHTMLFontElement();

  // nsISupports
  NS_DECL_ISUPPORTS_INHERITED

  // nsIDOMNode
  NS_FORWARD_NSIDOMNODE_NO_CLONENODE(nsGenericHTMLContainerElement::)

  // nsIDOMElement
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLContainerElement::)

  // nsIDOMHTMLElement
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLContainerElement::)

  // nsIDOMHTMLFontElement
  NS_DECL_NSIDOMHTMLFONTELEMENT

  NS_IMETHOD StringToAttribute(nsIAtom* aAttribute,
                               const nsAReadableString& aValue,
                               nsHTMLValue& aResult);
  NS_IMETHOD AttributeToString(nsIAtom* aAttribute,
                               const nsHTMLValue& aValue,
                               nsAWritableString& aResult) const;
  NS_IMETHOD GetMappedAttributeImpact(const nsIAtom* aAttribute,
                                      PRInt32& aHint) const;
  NS_IMETHOD GetAttributeMappingFunctions(nsMapAttributesFunc& aFontMapFunc,
                                          nsMapAttributesFunc& aMapFunc) const;
  NS_IMETHOD SizeOf(nsISizeOfHandler* aSizer, PRUint32* aResult) const;
};

nsresult
NS_NewHTMLFontElement(nsIHTMLContent** aInstancePtrResult,
                      nsINodeInfo *aNodeInfo)
{
  NS_ENSURE_ARG_POINTER(aInstancePtrResult);

  nsHTMLFontElement* it = new nsHTMLFontElement();

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


nsHTMLFontElement::nsHTMLFontElement()
{
}

nsHTMLFontElement::~nsHTMLFontElement()
{
}

NS_IMPL_ADDREF_INHERITED(nsHTMLFontElement, nsGenericElement);
NS_IMPL_RELEASE_INHERITED(nsHTMLFontElement, nsGenericElement);


// XPConnect interface list for nsHTMLFontElement
NS_CLASSINFO_MAP_BEGIN(HTMLFontElement)
  NS_CLASSINFO_MAP_ENTRY(nsIDOMHTMLFontElement)
  NS_CLASSINFO_MAP_ENTRY_FUNCTION(GetGenericHTMLElementIIDs)
NS_CLASSINFO_MAP_END


// QueryInterface implementation for nsHTMLFontElement
NS_HTML_CONTENT_INTERFACE_MAP_BEGIN(nsHTMLFontElement,
                                    nsGenericHTMLContainerElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMHTMLFontElement)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(HTMLFontElement)
NS_HTML_CONTENT_INTERFACE_MAP_END


nsresult
nsHTMLFontElement::CloneNode(PRBool aDeep, nsIDOMNode** aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);
  *aReturn = nsnull;

  nsHTMLFontElement* it = new nsHTMLFontElement();

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


NS_IMPL_STRING_ATTR(nsHTMLFontElement, Color, color)
NS_IMPL_STRING_ATTR(nsHTMLFontElement, Face, face)
NS_IMPL_STRING_ATTR(nsHTMLFontElement, Size, size)


NS_IMETHODIMP
nsHTMLFontElement::StringToAttribute(nsIAtom* aAttribute,
                              const nsAReadableString& aValue,
                              nsHTMLValue& aResult)
{
  if ((aAttribute == nsHTMLAtoms::size) ||
      (aAttribute == nsHTMLAtoms::pointSize) ||
      (aAttribute == nsHTMLAtoms::fontWeight)) {
    nsAutoString tmp(aValue);
      //rickg: fixed flaw where ToInteger error code was not being checked.
      //       This caused wrong default value for font size.
    PRInt32 ec, v = tmp.ToInteger(&ec);
    if(NS_SUCCEEDED(ec)){
      tmp.CompressWhitespace(PR_TRUE, PR_FALSE);
      PRUnichar ch = tmp.IsEmpty() ? 0 : tmp.First();
      aResult.SetIntValue(v, ((ch == '+') || (ch == '-')) ?
                          eHTMLUnit_Integer : eHTMLUnit_Enumerated);
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }
  else if (aAttribute == nsHTMLAtoms::color) {
    if (ParseColor(aValue, mDocument, aResult)) {
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }
  return NS_CONTENT_ATTR_NOT_THERE;
}

NS_IMETHODIMP
nsHTMLFontElement::AttributeToString(nsIAtom* aAttribute,
                                     const nsHTMLValue& aValue,
                                     nsAWritableString& aResult) const
{
  if ((aAttribute == nsHTMLAtoms::size) ||
      (aAttribute == nsHTMLAtoms::pointSize) ||
      (aAttribute == nsHTMLAtoms::fontWeight)) {
    aResult.Truncate();
    nsAutoString intVal;
    if (aValue.GetUnit() == eHTMLUnit_Enumerated) {
      intVal.AppendInt(aValue.GetIntValue(), 10);
      aResult.Append(intVal);
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
    else if (aValue.GetUnit() == eHTMLUnit_Integer) {
      PRInt32 value = aValue.GetIntValue(); 
      if (value >= 0) {
        aResult.Append(NS_LITERAL_STRING("+"));
      }
      intVal.AppendInt(value, 10);      
      aResult.Append(intVal);
      return NS_CONTENT_ATTR_HAS_VALUE;
    }

    return NS_CONTENT_ATTR_NOT_THERE;
  }

  return nsGenericHTMLContainerElement::AttributeToString(aAttribute, aValue,
                                                          aResult);
}

static void
MapFontAttributesInto(const nsIHTMLMappedAttributes* aAttributes,
                      nsIMutableStyleContext* aContext,
                      nsIPresContext* aPresContext)
{
  if (nsnull != aAttributes) {
    nsHTMLValue value;
    nsMutableStyleFont font(aContext);
    const nsStyleFont* parentFont = font.get();
    nsIStyleContext* parentContext = aContext->GetParent();
    if (nsnull != parentContext) {
      parentFont = (const nsStyleFont*)
        parentContext->GetStyleData(eStyleStruct_Font);
    }
    const nsFont& defaultFont = aPresContext->GetDefaultFontDeprecated(); 
    const nsFont& defaultFixedFont = aPresContext->GetDefaultFixedFontDeprecated(); 

    // face: string list
    aAttributes->GetAttribute(nsHTMLAtoms::face, value);

    if (value.GetUnit() == eHTMLUnit_String) {
      nsCOMPtr<nsIDeviceContext> dc;
      aPresContext->GetDeviceContext(getter_AddRefs(dc));
      if (dc) {
        nsAutoString  familyList;

        value.GetStringValue(familyList);
          
        font->mFont.name = familyList;
        nsAutoString face;

        // MJA: bug 31816 if we are not using document fonts, but this
        // is a xul document, then we set the chromeOverride bit so we
        // use the document fonts anyway
        PRBool chromeOverride = PR_FALSE;
        PRBool useDocumentFonts = PR_TRUE;
        aPresContext->GetCachedBoolPref(kPresContext_UseDocumentFonts,
                                        useDocumentFonts);
        if (!useDocumentFonts) {
          // check if the prefs have been disabled for this shell
          // - if prefs are disabled then we use the document fonts
          // anyway (yet another override)
          PRBool prefsEnabled = PR_TRUE;
          nsCOMPtr<nsIPresShell> shell;
          aPresContext->GetShell(getter_AddRefs(shell));
          if (shell) {
            shell->ArePrefStyleRulesEnabled(prefsEnabled);
          }

          if (!prefsEnabled) {
            useDocumentFonts = PR_TRUE;
          } else {
            // see if we are in the chrome, if so, use the document fonts (override the useDocFonts setting)
            nsresult result = NS_OK;
            nsCOMPtr<nsISupports> container;
            result = aPresContext->GetContainer(getter_AddRefs(container));
            if (NS_SUCCEEDED(result) && container) {
              nsCOMPtr<nsIDocShellTreeItem> docShell(do_QueryInterface(container, &result));
              if (NS_SUCCEEDED(result) && docShell){
                PRInt32 docShellType;
                result = docShell->GetItemType(&docShellType);
                if (NS_SUCCEEDED(result)){
                  if (nsIDocShellTreeItem::typeChrome == docShellType){
                    chromeOverride = PR_TRUE;
                  }
                }      
              }
            }
          }
        }

        // find the correct font if we are usingDocumentFonts OR we
        // are overriding for XUL
        // MJA: bug 31816
        PRBool fontFaceOK = PR_TRUE;
        PRBool isMozFixed = font->mFont.name.EqualsIgnoreCase("-moz-fixed");
        if ((chromeOverride || useDocumentFonts)) {
          fontFaceOK = (NS_OK == dc->FirstExistingFont(font->mFont, face));
        }

        if (!fontFaceOK || !(chromeOverride || useDocumentFonts)) {
          // now set to defaults
          font->mFont.name = defaultFont.name;
          font->mFixedFont.name= defaultFixedFont.name;
        }

        // set to monospace if using moz-fixed
        if (isMozFixed) {
          font->mFlags |= NS_STYLE_FONT_USE_FIXED;
        } else {
          font->mFlags &= ~NS_STYLE_FONT_USE_FIXED;
        }

        font->mFlags |= NS_STYLE_FONT_FACE_EXPLICIT;
      }
    }

    // pointSize: int, enum
    aAttributes->GetAttribute(nsHTMLAtoms::pointSize, value);
    if (value.GetUnit() == eHTMLUnit_Integer) {
      // XXX should probably sanitize value
      font->mFont.size = parentFont->mFont.size +
        NSIntPointsToTwips(value.GetIntValue());
      font->mFixedFont.size = parentFont->mFixedFont.size +
        NSIntPointsToTwips(value.GetIntValue());
      font->mFlags |= NS_STYLE_FONT_SIZE_EXPLICIT;
    }
    else if (value.GetUnit() == eHTMLUnit_Enumerated) {
      font->mFont.size = NSIntPointsToTwips(value.GetIntValue());
      font->mFixedFont.size = NSIntPointsToTwips(value.GetIntValue());
      font->mFlags |= NS_STYLE_FONT_SIZE_EXPLICIT;
    }
    else {
      // size: int, enum , NOTE: this does not count as an explicit size
      // also this has no effect if font is already explicit (quirk mode)
      //
      // NOTE: we now do not emulate this quirk - it is too stupid, IE does it right, and it
      //       messes up other blocks (ie. Headings) when implemented... see bug 25810

#if 0 // removing the quirk...
      nsCompatibility mode;
      aPresContext->GetCompatibilityMode(&mode);
      if ((eCompatibility_Standard == mode) || 
          (0 == (font->mFlags & NS_STYLE_FONT_SIZE_EXPLICIT))) {
#else
      if (1) {
#endif
        aAttributes->GetAttribute(nsHTMLAtoms::size, value);

        if ((value.GetUnit() == eHTMLUnit_Integer) ||
            (value.GetUnit() == eHTMLUnit_Enumerated)) { 
          PRInt32 size = value.GetIntValue();
        
          if (size != 0) {  // bug 32063: ignore <font size="">
	          if (value.GetUnit() == eHTMLUnit_Integer) { // int (+/-)
	            size = 3 + size;  // XXX should be BASEFONT, not three
	          }
	          size = ((0 < size) ? ((size < 8) ? size : 7) : 1); 
	          PRInt32 scaler;
	          aPresContext->GetFontScaler(&scaler);
	          float scaleFactor = nsStyleUtil::GetScalingFactor(scaler);
	          font->mFont.size =
	            nsStyleUtil::CalcFontPointSize(size, (PRInt32)defaultFont.size,
	                                           scaleFactor, aPresContext);
	          font->mFixedFont.size =
	            nsStyleUtil::CalcFontPointSize(size,
	                                           (PRInt32)defaultFixedFont.size,
	                                           scaleFactor, aPresContext);
					}
        }
      }
    }

    // fontWeight: int, enum
    aAttributes->GetAttribute(nsHTMLAtoms::fontWeight, value);
    if (value.GetUnit() == eHTMLUnit_Integer) { // +/-
      PRInt32 intValue = (value.GetIntValue() / 100);
      PRInt32 weight = nsStyleUtil::ConstrainFontWeight(parentFont->mFont.weight + 
                                                        (intValue * NS_STYLE_FONT_WEIGHT_BOLDER));
      font->mFont.weight = weight;
      font->mFixedFont.weight = weight;
    }
    else if (value.GetUnit() == eHTMLUnit_Enumerated) {
      PRInt32 weight = value.GetIntValue();
      weight = nsStyleUtil::ConstrainFontWeight((weight / 100) * 100);
      font->mFont.weight = weight;
      font->mFixedFont.weight = weight;
    }

    NS_IF_RELEASE(parentContext);
  }
}

static void
MapAttributesInto(const nsIHTMLMappedAttributes* aAttributes,
                  nsIMutableStyleContext* aContext,
                  nsIPresContext* aPresContext)
{
  if (nsnull != aAttributes) {
    nsHTMLValue value;

    // color: color
    if (NS_CONTENT_ATTR_NOT_THERE !=
        aAttributes->GetAttribute(nsHTMLAtoms::color, value)) {
      const nsStyleFont* font = (const nsStyleFont*)
        aContext->GetStyleData(eStyleStruct_Font);
      nsMutableStyleColor color(aContext);
      nsMutableStyleText text(aContext);
      if (((eHTMLUnit_Color == value.GetUnit())) ||
          (eHTMLUnit_ColorName == value.GetUnit())) {
        color->mColor = value.GetColorValue();

        // re-apply inherited text decoration, so colors sync
        text->mTextDecoration = font->mFont.decorations;
      }
    }
  }

  nsGenericHTMLElement::MapCommonAttributesInto(aAttributes, aContext,
                                                aPresContext);
}

NS_IMETHODIMP
nsHTMLFontElement::GetMappedAttributeImpact(const nsIAtom* aAttribute,
                                            PRInt32& aHint) const
{
  if (aAttribute == nsHTMLAtoms::color) {
    aHint = NS_STYLE_HINT_VISUAL;
  }
  else if ((aAttribute == nsHTMLAtoms::face) ||
           (aAttribute == nsHTMLAtoms::pointSize) ||
           (aAttribute == nsHTMLAtoms::size) ||
           (aAttribute == nsHTMLAtoms::fontWeight)) {
    aHint = NS_STYLE_HINT_REFLOW;
  }
  else if (!GetCommonMappedAttributesImpact(aAttribute, aHint)) {
    aHint = NS_STYLE_HINT_CONTENT;
  }

  return NS_OK;
}


NS_IMETHODIMP
nsHTMLFontElement::GetAttributeMappingFunctions(nsMapAttributesFunc& aFontMapFunc,
                                                nsMapAttributesFunc& aMapFunc) const
{
  aFontMapFunc = &MapFontAttributesInto;
  aMapFunc = &MapAttributesInto;
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLFontElement::SizeOf(nsISizeOfHandler* aSizer, PRUint32* aResult) const
{
  *aResult = sizeof(*this) + BaseSizeOf(aSizer);

  return NS_OK;
}
