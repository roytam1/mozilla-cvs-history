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

#include "nsIComputedDOMStyle.h"
#include "nsIPresShell.h"
#include "nsIPresContext.h"
#include "nsIContent.h"
#include "nsIFrame.h"
#include "nsIScrollableFrame.h"
#include "nsLayoutAtoms.h"
#include "nsIDOMElement.h"
#include "nsIStyleContext.h"
#include "nsROCSSPrimitiveValue.h"

#include "nsCSSProps.h"

#include "nsCOMPtr.h"
#include "nsDOMError.h"
#include "nsWeakReference.h"
#include "nsIDocument.h"

#include "prprf.h"
#include "nsReadableUtils.h"
#include "nsContentUtils.h"

/*
 * This is the implementation of the readonly CSSStyleDeclaration that is
 * returned by the getComputedStyle() function.
 *
 * This file is very much work in progress, there's a lot of code in this
 * file that is #ifdef'd out placeholders.
 */

class nsComputedDOMStyle : public nsIComputedDOMStyle
{
public:
  NS_DECL_ISUPPORTS

  // nsIComputedDOMStyle
  NS_IMETHOD Init(nsIDOMElement *aElement, const nsAReadableString& aPseudoElt,
                  nsIPresShell *aPresShell);

  // nsIDOMCSSStyleDeclaration
  NS_DECL_NSIDOMCSSSTYLEDECLARATION

  // nsComputedDOMStyle
  nsComputedDOMStyle();
  virtual ~nsComputedDOMStyle();

private:
  //Helpers
  nsresult GetOffsetWidthFor(PRUint8 aSide, nsIFrame *aFrame,
                             nsIDOMCSSPrimitiveValue*& aValue);
  nsresult GetAbsoluteOffset(PRUint8 aSide, nsIFrame *aFrame,
                             nsIDOMCSSPrimitiveValue*& aValue);
  nsresult GetRelativeOffset(PRUint8 aSide, nsIFrame *aFrame,
                             nsIDOMCSSPrimitiveValue*& aValue);
  nsresult GetStaticOffset(PRUint8 aSide, nsIFrame *aFrame,
                           nsIDOMCSSPrimitiveValue*& aValue);
  nsIFrame* GetContainingBlock(nsIFrame *aFrame);
  nsresult GetStyleData(nsStyleStructID aID,
                        const nsStyleStruct*& aStyleStruct,
                        nsIFrame* aFrame=0);
  nsresult GetPaddingWidthFor(PRUint8 aSide, nsIFrame *aFrame,
                              nsIDOMCSSPrimitiveValue*& aValue);
  nscoord GetPaddingWidthCoordFor(PRUint8 aSide, nsIFrame *aFrame);
  nsresult GetBorderStyleFor(PRUint8 aSide, nsIFrame *aFrame,
                             nsIDOMCSSPrimitiveValue*& aValue);
  nsresult GetBorderWidthFor(PRUint8 aSide, nsIFrame *aFrame,
                             nsIDOMCSSPrimitiveValue*& aValue);
  nscoord GetBorderWidthCoordFor(PRUint8 aSide, nsIFrame *aFrame);
  nsresult GetBorderColorFor(PRUint8 aSide, nsIFrame *aFrame,
                             nsIDOMCSSPrimitiveValue*& aValue);
  nsresult GetMarginWidthFor(PRUint8 aSide, nsIFrame *aFrame,
                             nsIDOMCSSPrimitiveValue*& aValue);
  nscoord GetMarginWidthCoordFor(PRUint8 aSide, nsIFrame *aFrame);

  // Properties
  nsresult GetWidth(nsIFrame *aFrame, nsIDOMCSSPrimitiveValue*& aValue);
  nsresult GetHeight(nsIFrame *aFrame, nsIDOMCSSPrimitiveValue*& aValue);
  nsresult GetLeft(nsIFrame *aFrame, nsIDOMCSSPrimitiveValue*& aValue);
  nsresult GetTop(nsIFrame *aFrame, nsIDOMCSSPrimitiveValue*& aValue);
  nsresult GetRight(nsIFrame *aFrame, nsIDOMCSSPrimitiveValue*& aValue);
  nsresult GetBottom(nsIFrame *aFrame, nsIDOMCSSPrimitiveValue*& aValue);

  //Font properties
  nsresult GetColor(nsIFrame *aFrame, nsIDOMCSSPrimitiveValue*& aValue);
  nsresult GetFontFamily(nsIFrame *aFrame, nsIDOMCSSPrimitiveValue*& aValue);
  nsresult GetFontStyle(nsIFrame *aFrame, nsIDOMCSSPrimitiveValue*& aValue);
  nsresult GetFontSize(nsIFrame *aFrame, nsIDOMCSSPrimitiveValue*& aValue);
  nsresult GetFontWeight(nsIFrame *aFrame, nsIDOMCSSPrimitiveValue*& aValue);
  nsresult GetFontVariant(nsIFrame *aFrame, nsIDOMCSSPrimitiveValue*& aValue);

  // Background properties
  nsresult GetBackgroundColor(nsIFrame *aFrame,
                              nsIDOMCSSPrimitiveValue*& aValue);
  nsresult GetBackgroundImage(nsIFrame *aFrame,
                              nsIDOMCSSPrimitiveValue*& aValue);

  // Padding properties
  nsresult GetPadding(nsIFrame *aFrame, nsIDOMCSSPrimitiveValue*& aValue);
  nsresult GetPaddingTop(nsIFrame *aFrame, nsIDOMCSSPrimitiveValue*& aValue);
  nsresult GetPaddingBottom(nsIFrame *aFrame,
                            nsIDOMCSSPrimitiveValue*& aValue);
  nsresult GetPaddingLeft(nsIFrame *aFrame, nsIDOMCSSPrimitiveValue*& aValue);
  nsresult GetPaddingRight(nsIFrame *aFrame, nsIDOMCSSPrimitiveValue*& aValue);

  // BorderProperties
  nsresult GetBorderStyle(nsIFrame *aFrame, nsIDOMCSSPrimitiveValue*& aValue);
  nsresult GetBorderWidth(nsIFrame *aFrame, nsIDOMCSSPrimitiveValue*& aValue);
  nsresult GetBorderCollapse(nsIFrame *aFrame,
                             nsIDOMCSSPrimitiveValue*& aValue);
  nsresult GetBorderSpacing(nsIFrame *aFrame,
                            nsIDOMCSSPrimitiveValue*& aValue);
  nsresult GetBorderTopStyle(nsIFrame *aFrame,
                             nsIDOMCSSPrimitiveValue*& aValue);
  nsresult GetBorderBottomStyle(nsIFrame *aFrame,
                                nsIDOMCSSPrimitiveValue*& aValue);
  nsresult GetBorderLeftStyle(nsIFrame *aFrame,
                              nsIDOMCSSPrimitiveValue*& aValue);
  nsresult GetBorderRightStyle(nsIFrame *aFrame,
                               nsIDOMCSSPrimitiveValue*& aValue);
  nsresult GetBorderTopWidth(nsIFrame *aFrame,
                             nsIDOMCSSPrimitiveValue*& aValue);
  nsresult GetBorderBottomWidth(nsIFrame *aFrame,
                                nsIDOMCSSPrimitiveValue*& aValue);
  nsresult GetBorderLeftWidth(nsIFrame *aFrame,
                              nsIDOMCSSPrimitiveValue*& aValue);
  nsresult GetBorderRightWidth(nsIFrame *aFrame,
                               nsIDOMCSSPrimitiveValue*& aValue);
  nsresult GetBorderTopColor(nsIFrame *aFrame,
                             nsIDOMCSSPrimitiveValue*& aValue);
  nsresult GetBorderBottomColor(nsIFrame *aFrame,
                                nsIDOMCSSPrimitiveValue*& aValue);
  nsresult GetBorderLeftColor(nsIFrame *aFrame,
                              nsIDOMCSSPrimitiveValue*& aValue);
  nsresult GetBorderRightColor(nsIFrame *aFrame,
                               nsIDOMCSSPrimitiveValue*& aValue);

  // Margin Properties
  nsresult GetMarginWidth(nsIFrame *aFrame, nsIDOMCSSPrimitiveValue*& aValue);
  nsresult GetMarginTopWidth(nsIFrame *aFrame,
                             nsIDOMCSSPrimitiveValue*& aValue);
  nsresult GetMarginBottomWidth(nsIFrame *aFrame,
                                nsIDOMCSSPrimitiveValue*& aValue);
  nsresult GetMarginLeftWidth(nsIFrame *aFrame,
                              nsIDOMCSSPrimitiveValue*& aValue);
  nsresult GetMarginRightWidth(nsIFrame *aFrame,
                               nsIDOMCSSPrimitiveValue*& aValue);

  // Outline Properties
  nsresult GetOutline(nsIFrame *aFrame, nsIDOMCSSPrimitiveValue*& aValue);
  nsresult GetOutlineWidth(nsIFrame *aFrame, nsIDOMCSSPrimitiveValue*& aValue);
  nsresult GetOutlineStyle(nsIFrame *aFrame, nsIDOMCSSPrimitiveValue*& aValue);
  nsresult GetOutlineColor(nsIFrame *aFrame, nsIDOMCSSPrimitiveValue*& aValue);


  //Marker Properties
  nsresult GetMarkerOffset(nsIFrame *aFrame, nsIDOMCSSPrimitiveValue*& aValue);

  // z-index
  nsresult GetZIndex(nsIFrame *aFrame, nsIDOMCSSPrimitiveValue*& aValue);

  // List properties
  nsresult GetListStyleImage(nsIFrame *aFrame,
                             nsIDOMCSSPrimitiveValue*& aValue);

  // Text Properties
  nsresult GetTextAlign(nsIFrame *aFrame, nsIDOMCSSPrimitiveValue*& aValue);
  nsresult GetTextDecoration(nsIFrame *aFrame, nsIDOMCSSPrimitiveValue*& aValue);  

  // Visibility properties
  nsresult GetVisibility(nsIFrame *aFrame, nsIDOMCSSPrimitiveValue*& aValue);

  // Display properties
  nsresult GetBehavior(nsIFrame *aFrame, nsIDOMCSSPrimitiveValue*& aValue);
  nsresult GetCssFloat(nsIFrame *aFrame, nsIDOMCSSPrimitiveValue*& aValue);
  nsresult GetDisplay(nsIFrame *aFrame, nsIDOMCSSPrimitiveValue*& aValue);

  nsROCSSPrimitiveValue* GetROCSSPrimitiveValue();

  nsWeakPtr mPresShellWeak; // XXX could this be high cost?
  nsCOMPtr<nsIContent> mContent;
  // When a frame is unavailable strong reference to the
  // style context while we're accessing  the data from in.
  nsCOMPtr<nsIStyleContext> mStyleContextHolder;
  nsCOMPtr<nsIAtom> mPseudo;

  float mT2P; // For unit conversions
};

static const nsCSSProperty queryableProperties[] = {
  eCSSProperty_width,
  eCSSProperty_height,
  eCSSProperty_left,
  eCSSProperty_top,
  eCSSProperty_right,
  eCSSProperty_bottom,

  eCSSProperty_color,
  eCSSProperty_font_family,
  eCSSProperty_font_style,
  eCSSProperty_font_size,
  eCSSProperty_font_weight,
  eCSSProperty_font_variant,

  eCSSProperty_background_color,
  eCSSProperty_background_image,

  eCSSProperty_display,
  eCSSProperty_binding,
  eCSSProperty_float,

  eCSSProperty_padding,
  eCSSProperty_padding_top,
  eCSSProperty_padding_bottom,
  eCSSProperty_padding_left,
  eCSSProperty_padding_right,

  eCSSProperty_border_style,
  eCSSProperty_border_width,
  eCSSProperty_border_collapse,
  eCSSProperty_border_spacing,
  eCSSProperty_border_top_style,
  eCSSProperty_border_bottom_style,
  eCSSProperty_border_left_style,
  eCSSProperty_border_right_style,
  eCSSProperty_border_top_width,
  eCSSProperty_border_bottom_width,
  eCSSProperty_border_left_width,
  eCSSProperty_border_right_width,
  eCSSProperty_border_top_color,
  eCSSProperty_border_bottom_color,
  eCSSProperty_border_left_color,
  eCSSProperty_border_right_color,

  eCSSProperty_margin,
  eCSSProperty_margin_top,
  eCSSProperty_margin_bottom,
  eCSSProperty_margin_left,
  eCSSProperty_margin_right,

  eCSSProperty_outline,
  eCSSProperty_outline_width,
  eCSSProperty_outline_style,
  eCSSProperty_outline_color,

  eCSSProperty_marker_offset,

  eCSSProperty_z_index,

  eCSSProperty_list_style_image,

  eCSSProperty_text_align,
  eCSSProperty_text_decoration,

};

nsresult
NS_NewComputedDOMStyle(nsIComputedDOMStyle** aComputedStyle)
{
  NS_ENSURE_ARG_POINTER(aComputedStyle);

  *aComputedStyle = new nsComputedDOMStyle();
  NS_ENSURE_TRUE(*aComputedStyle, NS_ERROR_OUT_OF_MEMORY);

  NS_ADDREF(*aComputedStyle);

  return NS_OK;
}


nsComputedDOMStyle::nsComputedDOMStyle() : mPresShellWeak(nsnull),
                                           mT2P(0.0f)
{
  NS_INIT_REFCNT();
}


nsComputedDOMStyle::~nsComputedDOMStyle()
{
}


// QueryInterface implementation for nsComputedDOMStyle
NS_INTERFACE_MAP_BEGIN(nsComputedDOMStyle)
  NS_INTERFACE_MAP_ENTRY(nsIComputedDOMStyle)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCSSStyleDeclaration)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIComputedDOMStyle)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(ComputedCSSStyleDeclaration)
NS_INTERFACE_MAP_END


NS_IMPL_ADDREF(nsComputedDOMStyle);
NS_IMPL_RELEASE(nsComputedDOMStyle);


NS_IMETHODIMP
nsComputedDOMStyle::Init(nsIDOMElement *aElement,
                         const nsAReadableString& aPseudoElt,
                         nsIPresShell *aPresShell)
{
  NS_ENSURE_ARG_POINTER(aElement);
  NS_ENSURE_ARG_POINTER(aPresShell);

  mPresShellWeak = getter_AddRefs(NS_GetWeakReference(aPresShell));

  mContent = do_QueryInterface(aElement);
  if (!mContent) {
    // This should not happen, all our elements support nsIContent!
    return NS_ERROR_FAILURE;
  }

  if(!DOMStringIsNull(aPseudoElt) && !aPseudoElt.IsEmpty()) {
    mPseudo = dont_AddRef(NS_NewAtom(aPseudoElt));
    NS_ENSURE_TRUE(mPseudo, NS_ERROR_OUT_OF_MEMORY);
  }

  nsCOMPtr<nsIPresContext> presCtx;

  aPresShell->GetPresContext(getter_AddRefs(presCtx));

  NS_ENSURE_TRUE(presCtx, NS_ERROR_FAILURE);

  presCtx->GetTwipsToPixels(&mT2P);

  return NS_OK;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetCssText(nsAWritableString& aCssText)
{
  aCssText.Truncate();
  return NS_OK;
}


NS_IMETHODIMP
nsComputedDOMStyle::SetCssText(const nsAReadableString& aCssText)
{
  return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR;
}


NS_IMETHODIMP
nsComputedDOMStyle::GetLength(PRUint32* aLength)
{
  NS_ENSURE_ARG_POINTER(aLength);
  *aLength = sizeof(queryableProperties) / sizeof(nsCSSProperty);
  return NS_OK;
}


NS_IMETHODIMP
nsComputedDOMStyle::GetParentRule(nsIDOMCSSRule** aParentRule)
{
  NS_ENSURE_ARG_POINTER(aParentRule);
  *aParentRule = nsnull;

  return NS_OK;
}


NS_IMETHODIMP
nsComputedDOMStyle::GetPropertyValue(const nsAReadableString& aPropertyName,
                                     nsAWritableString& aReturn)
{
  nsCOMPtr<nsIDOMCSSValue> val;

  aReturn.Truncate();

  nsresult rv = GetPropertyCSSValue(aPropertyName, getter_AddRefs(val));
  NS_ENSURE_SUCCESS(rv, rv);

  if (val) {
    rv = val->GetCssText(aReturn);
  }

  return rv;
}


NS_IMETHODIMP
nsComputedDOMStyle::GetPropertyCSSValue(const nsAReadableString& aPropertyName,
                                        nsIDOMCSSValue** aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);
  *aReturn = nsnull;

  nsCOMPtr<nsIPresShell> presShell=do_QueryReferent(mPresShellWeak);

  NS_ENSURE_TRUE(presShell, NS_ERROR_FAILURE);

  nsIFrame *frame = nsnull;
  presShell->GetPrimaryFrameFor(mContent, &frame);

  nsCOMPtr<nsIDOMCSSPrimitiveValue> val;
  nsresult rv = NS_OK;

  // XXX FIX THIS!!!
  nsCSSProperty prop = nsCSSProps::LookupProperty(aPropertyName);

  switch (prop) {
    case eCSSProperty_binding :
      rv = GetBehavior(frame, *getter_AddRefs(val)); break;
    case eCSSProperty_display :
      rv = GetDisplay(frame, *getter_AddRefs(val)); break;
    case eCSSProperty_float :
      rv = GetCssFloat(frame, *getter_AddRefs(val)); break;
    case eCSSProperty_width :
      rv = GetWidth(frame, *getter_AddRefs(val)); break;
    case eCSSProperty_height :
      rv = GetHeight(frame, *getter_AddRefs(val)); break;
    case eCSSProperty_left :
      rv = GetLeft(frame, *getter_AddRefs(val)); break;
    case eCSSProperty_top :
      rv = GetTop(frame, *getter_AddRefs(val)); break;
    case eCSSProperty_right :
      rv = GetRight(frame, *getter_AddRefs(val)); break;
    case eCSSProperty_bottom :
      rv = GetBottom(frame, *getter_AddRefs(val)); break;

      // Font properties
    case eCSSProperty_color :
      rv = GetColor(frame, *getter_AddRefs(val)); break;
    case eCSSProperty_font_family :
      rv = GetFontFamily(frame, *getter_AddRefs(val)); break;
    case eCSSProperty_font_size :
      rv = GetFontSize(frame, *getter_AddRefs(val)); break;
    case eCSSProperty_font_style :
      rv = GetFontStyle(frame, *getter_AddRefs(val)); break;
    case eCSSProperty_font_weight :
      rv = GetFontWeight(frame, *getter_AddRefs(val)); break;
    case eCSSProperty_font_variant :
      rv = GetFontVariant(frame, *getter_AddRefs(val)); break;

      // Background properties
    case eCSSProperty_background_color :
      rv = GetBackgroundColor(frame, *getter_AddRefs(val)); break;
    case eCSSProperty_background_image :
      rv = GetBackgroundImage(frame, *getter_AddRefs(val)); break;

      // Padding properties
    case eCSSProperty_padding :
      rv = GetPadding(frame, *getter_AddRefs(val)); break;
    case eCSSProperty_padding_top :
      rv = GetPaddingTop(frame, *getter_AddRefs(val)); break;
    case eCSSProperty_padding_bottom :
      rv = GetPaddingBottom(frame, *getter_AddRefs(val)); break;
    case eCSSProperty_padding_left :
      rv = GetPaddingLeft(frame, *getter_AddRefs(val)); break;
    case eCSSProperty_padding_right :
      rv = GetPaddingRight(frame, *getter_AddRefs(val)); break;

      // Border properties
    case eCSSProperty_border_collapse :
      rv = GetBorderCollapse(frame, *getter_AddRefs(val)); break;
    case eCSSProperty_border_top_style :
      rv = GetBorderTopStyle(frame, *getter_AddRefs(val)); break;
    case eCSSProperty_border_bottom_style :
      rv = GetBorderBottomStyle(frame, *getter_AddRefs(val)); break;
    case eCSSProperty_border_left_style :
      rv = GetBorderLeftStyle(frame, *getter_AddRefs(val)); break;
    case eCSSProperty_border_right_style :
      rv = GetBorderRightStyle(frame, *getter_AddRefs(val)); break;
    case eCSSProperty_border_top_width :
      rv = GetBorderTopWidth(frame, *getter_AddRefs(val)); break;
    case eCSSProperty_border_bottom_width :
      rv = GetBorderBottomWidth(frame, *getter_AddRefs(val)); break;
    case eCSSProperty_border_left_width :
      rv = GetBorderLeftWidth(frame, *getter_AddRefs(val)); break;
    case eCSSProperty_border_right_width :
      rv = GetBorderRightWidth(frame, *getter_AddRefs(val)); break;
    case eCSSProperty_border_top_color :
      rv = GetBorderTopColor(frame, *getter_AddRefs(val)); break;
    case eCSSProperty_border_bottom_color :
      rv = GetBorderBottomColor(frame, *getter_AddRefs(val)); break;
    case eCSSProperty_border_left_color :
      rv = GetBorderLeftColor(frame, *getter_AddRefs(val)); break;
    case eCSSProperty_border_right_color :
      rv = GetBorderRightColor(frame, *getter_AddRefs(val)); break;

      // Margin properties
    case eCSSProperty_margin_top :
      rv = GetMarginTopWidth(frame, *getter_AddRefs(val)); break;
    case eCSSProperty_margin_bottom :
      rv = GetMarginBottomWidth(frame, *getter_AddRefs(val)); break;
    case eCSSProperty_margin_left :
      rv = GetMarginLeftWidth(frame, *getter_AddRefs(val)); break;
    case eCSSProperty_margin_right :
      rv = GetMarginRightWidth(frame, *getter_AddRefs(val)); break;

      // Marker property
    case eCSSProperty_marker_offset :
      rv = GetMarkerOffset(frame, *getter_AddRefs(val)); break;

      // Outline properties
    case eCSSProperty_outline_width :
      rv = GetOutlineWidth(frame, *getter_AddRefs(val)); break;
    case eCSSProperty_outline_style :
      rv = GetOutlineStyle(frame, *getter_AddRefs(val)); break;
    case eCSSProperty_outline_color:
      rv = GetOutlineColor(frame, *getter_AddRefs(val)); break;

      // Text properties
    case eCSSProperty_text_align:
      rv = GetTextAlign(frame, *getter_AddRefs(val)); break;

    case eCSSProperty_text_decoration:
      rv = GetTextDecoration(frame, *getter_AddRefs(val)); break;
      
      // List properties
    case eCSSProperty_list_style_image:
      rv = GetListStyleImage(frame, *getter_AddRefs(val)); break; 

      // Display properties
    case eCSSProperty_visibility:
      rv = GetVisibility(frame, *getter_AddRefs(val)); break;

      // Z-Index property
    case eCSSProperty_z_index:
      rv = GetZIndex(frame, *getter_AddRefs(val)); break;
    default :
      break;
  }

  if (val) {
    rv = val->QueryInterface(NS_GET_IID(nsIDOMCSSValue), (void **)aReturn);
  }

  // Release the current style context for it should be re-resolved
  // whenever a frame is not available.
  mStyleContextHolder=nsnull;

  return rv;
}


NS_IMETHODIMP
nsComputedDOMStyle::RemoveProperty(const nsAReadableString& aPropertyName,
                                   nsAWritableString& aReturn)
{
  return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR;
}


NS_IMETHODIMP
nsComputedDOMStyle::GetPropertyPriority(const nsAReadableString& aPropertyName,
                                        nsAWritableString& aReturn)
{
  aReturn.Truncate();
  return NS_OK;
}


NS_IMETHODIMP
nsComputedDOMStyle::SetProperty(const nsAReadableString& aPropertyName,
                                const nsAReadableString& aValue,
                                const nsAReadableString& aPriority)
{
  return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR;
}


NS_IMETHODIMP
nsComputedDOMStyle::Item(PRUint32 aIndex, nsAWritableString& aReturn)
{
  PRUint32 length;
  GetLength(&length);
  aReturn.Truncate();
  if (aIndex >= 0 && aIndex < length) {
    CopyASCIItoUCS2(nsCSSProps::GetStringValue(queryableProperties[aIndex]), aReturn);
  }
  return NS_OK;
}


// Property getters...

#if 0

NS_IMETHODIMP
nsComputedDOMStyle::GetAzimuth(nsAWritableString& aAzimuth)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetBackground(nsAWritableString& aBackground)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetBackgroundAttachment(nsAWritableString& aBackgroundAttachment)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetBackgroundColor(nsAWritableString& aBackgroundColor)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetBackgroundImage(nsAWritableString& aBackgroundImage)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetBackgroundPosition(nsAWritableString& aBackgroundPosition)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetBackgroundRepeat(nsAWritableString& aBackgroundRepeat)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

#endif

nsresult
nsComputedDOMStyle::GetBehavior(nsIFrame *aFrame,
                                nsIDOMCSSPrimitiveValue*& aValue)
{
  nsROCSSPrimitiveValue *val = GetROCSSPrimitiveValue();
  NS_ENSURE_TRUE(val, NS_ERROR_OUT_OF_MEMORY);

  const nsStyleDisplay* display = nsnull;

  GetStyleData(eStyleStruct_Display, (const nsStyleStruct*&)display, aFrame);

  if (display) {
    val->SetString(display->mBinding);
  }
  else {
    val->SetString("");
  }

  return val->QueryInterface(NS_GET_IID(nsIDOMCSSPrimitiveValue),
                             (void **)&aValue);
}

nsresult
nsComputedDOMStyle::GetCssFloat(nsIFrame *aFrame,
                                      nsIDOMCSSPrimitiveValue*& aValue)
{
  nsROCSSPrimitiveValue* val=GetROCSSPrimitiveValue();
  NS_ENSURE_TRUE(val, NS_ERROR_OUT_OF_MEMORY);

  const nsStyleDisplay* display=nsnull;
  GetStyleData(eStyleStruct_Display,(const nsStyleStruct*&)display,aFrame);

  if(display) {
    const nsAFlatCString& cssFloat =
      nsCSSProps::SearchKeywordTable(display->mFloats,
	                                 nsCSSProps::kFloatKTable);
    val->SetString(cssFloat.get());
  }
  else {
    val->SetString("");
  }

  return val->QueryInterface(NS_GET_IID(nsIDOMCSSPrimitiveValue),
	                         (void **)&aValue);
}

#if 0

NS_IMETHODIMP
nsComputedDOMStyle::GetBorder(nsAWritableString& aBorder)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetBorderCollapse(nsAWritableString& aBorderCollapse)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetBorderColor(nsAWritableString& aBorderColor)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetBorderSpacing(nsAWritableString& aBorderSpacing)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetBorderStyle(nsAWritableString& aBorderStyle)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetBorderTop(nsAWritableString& aBorderTop)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetBorderRight(nsAWritableString& aBorderRight)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetBorderBottom(nsAWritableString& aBorderBottom)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetBorderLeft(nsAWritableString& aBorderLeft)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetBorderTopColor(nsAWritableString& aBorderTopColor)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetBorderRightColor(nsAWritableString& aBorderRightColor)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetBorderBottomColor(nsAWritableString& aBorderBottomColor)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetBorderLeftColor(nsAWritableString& aBorderLeftColor)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetBorderTopStyle(nsAWritableString& aBorderTopStyle)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetBorderRightStyle(nsAWritableString& aBorderRightStyle)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetBorderBottomStyle(nsAWritableString& aBorderBottomStyle)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetBorderLeftStyle(nsAWritableString& aBorderLeftStyle)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetBorderTopWidth(nsAWritableString& aBorderTopWidth)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetBorderRightWidth(nsAWritableString& aBorderRightWidth)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetBorderBottomWidth(nsAWritableString& aBorderBottomWidth)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetBorderLeftWidth(nsAWritableString& aBorderLeftWidth)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetBorderWidth(nsAWritableString& aBorderWidth)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}
#endif

nsresult
nsComputedDOMStyle::GetBottom(nsIFrame *aFrame,
                              nsIDOMCSSPrimitiveValue*& aValue)
{
  return GetOffsetWidthFor(NS_SIDE_BOTTOM, aFrame, aValue);
}


static void ColorToHex(nscolor aColor, nsAWritableString& aHexString)
{
   char buf[32];

   PRUint32 len = PR_snprintf(buf, 32, "#%02x%02x%02x",
                              NS_GET_R(aColor),
                              NS_GET_G(aColor),
                              NS_GET_B(aColor));

   CopyASCIItoUCS2(nsDependentCString(buf, len), aHexString);
}


nsresult
nsComputedDOMStyle::GetColor(nsIFrame *aFrame,
                             nsIDOMCSSPrimitiveValue*& aValue)
{
      nsROCSSPrimitiveValue* val=GetROCSSPrimitiveValue();
  NS_ENSURE_TRUE(val, NS_ERROR_OUT_OF_MEMORY);

  const nsStyleColor* color=nsnull;
  GetStyleData(eStyleStruct_Color, (const nsStyleStruct*&)color, aFrame);

  if(color) {
    nsAutoString hex;
    ColorToHex(color->mColor, hex);
    val->SetString(hex);
  }
  else {
    val->SetString("");
  }

  return val->QueryInterface(NS_GET_IID(nsIDOMCSSPrimitiveValue),
                             (void **)&aValue);
}

nsresult
nsComputedDOMStyle::GetFontFamily(nsIFrame *aFrame,
                             nsIDOMCSSPrimitiveValue*& aValue)
{
  nsROCSSPrimitiveValue* val=GetROCSSPrimitiveValue();
  NS_ENSURE_TRUE(val, NS_ERROR_OUT_OF_MEMORY);

  const nsStyleFont* font=nsnull;
  GetStyleData(eStyleStruct_Font, (const nsStyleStruct*&)font, aFrame);

  if(font) {
    nsCOMPtr<nsIPresShell> presShell=do_QueryReferent(mPresShellWeak);
    NS_ASSERTION(presShell, "pres shell is required");
    nsCOMPtr<nsIPresContext> presContext;
    presShell->GetPresContext(getter_AddRefs(presContext));
    NS_ASSERTION(presContext, "pres context is required");

    const nsString& fontName = font->mFont.name;
    PRUint8 generic = font->mFlags & NS_STYLE_FONT_FACE_MASK;
    if (generic == kGenericFont_NONE) { 
      nsFont defaultFont; 
      presContext->GetDefaultFont(kPresContext_DefaultVariableFont_ID, defaultFont);
      PRInt32 lendiff = fontName.Length() - defaultFont.name.Length();
      if (lendiff > 0) {
        val->SetString(Substring(fontName, 0, lendiff-1)); // -1 removes comma
      } else {
        val->SetString(fontName);
      }
    } else {
      val->SetString(fontName);
    }
  }
  else {
    val->SetString("");
  }

  return val->QueryInterface(NS_GET_IID(nsIDOMCSSPrimitiveValue),
                             (void **)&aValue);
}

nsresult
nsComputedDOMStyle::GetFontSize(nsIFrame *aFrame,
                             nsIDOMCSSPrimitiveValue*& aValue)
{
  nsROCSSPrimitiveValue* val=GetROCSSPrimitiveValue();
  NS_ENSURE_TRUE(val, NS_ERROR_OUT_OF_MEMORY);

  const nsStyleFont* font=nsnull;
  GetStyleData(eStyleStruct_Font, (const nsStyleStruct*&)font, aFrame);

  // Note: font->mSize is the 'computed size'; font->mFont.size is the 'actual size'
  val->SetTwips(font? font->mSize:0);

  return val->QueryInterface(NS_GET_IID(nsIDOMCSSPrimitiveValue),
                             (void **)&aValue);
}

nsresult
nsComputedDOMStyle::GetFontStyle(nsIFrame *aFrame,
                                 nsIDOMCSSPrimitiveValue*& aValue)
{
  nsROCSSPrimitiveValue* val=GetROCSSPrimitiveValue();
  NS_ENSURE_TRUE(val, NS_ERROR_OUT_OF_MEMORY);

  const nsStyleFont* font=nsnull;
  GetStyleData(eStyleStruct_Font, (const nsStyleStruct*&)font, aFrame);

  if(font) {
    const nsAFlatCString& style=
      nsCSSProps::SearchKeywordTable(font->mFont.style,
                                     nsCSSProps::kFontStyleKTable);
    val->SetString(style.get());
  }
  else {
    val->SetString("");
  }

  return val->QueryInterface(NS_GET_IID(nsIDOMCSSPrimitiveValue),
                             (void **)&aValue);
}

nsresult
nsComputedDOMStyle::GetFontWeight(nsIFrame *aFrame,
                             nsIDOMCSSPrimitiveValue*& aValue)
{
  nsROCSSPrimitiveValue* val=GetROCSSPrimitiveValue();
  NS_ENSURE_TRUE(val, NS_ERROR_OUT_OF_MEMORY);

  const nsStyleFont* font=nsnull;
  GetStyleData(eStyleStruct_Font, (const nsStyleStruct*&)font, aFrame);

  if(font) {
    const nsAFlatCString& str_weight=
      nsCSSProps::SearchKeywordTable(font->mFont.weight,
                                     nsCSSProps::kFontWeightKTable);
    if(str_weight.Length()>0) {
      val->SetString(str_weight.get());
    }
    else {
      nsAutoString num_weight;
      num_weight.AppendInt(font->mFont.weight, 10);
      val->SetString(num_weight);
    }
  }
  else {
    val->SetString("");
  }

  return val->QueryInterface(NS_GET_IID(nsIDOMCSSPrimitiveValue),
                             (void **)&aValue);
}

nsresult
nsComputedDOMStyle::GetFontVariant(nsIFrame *aFrame,
                             nsIDOMCSSPrimitiveValue*& aValue)
{
  nsROCSSPrimitiveValue* val=GetROCSSPrimitiveValue();
  NS_ENSURE_TRUE(val, NS_ERROR_OUT_OF_MEMORY);

  const nsStyleFont* font=nsnull;
  GetStyleData(eStyleStruct_Font, (const nsStyleStruct*&)font, aFrame);

  if(font) {
    const nsAFlatCString& variant=
      nsCSSProps::SearchKeywordTable(font->mFont.variant,
                                     nsCSSProps::kFontVariantKTable);
    val->SetString(variant.get());
  }
  else {
    val->SetString("");
  }

  return val->QueryInterface(NS_GET_IID(nsIDOMCSSPrimitiveValue),
                             (void **)&aValue);
}

nsresult
nsComputedDOMStyle::GetBackgroundColor(nsIFrame *aFrame,
                             nsIDOMCSSPrimitiveValue*& aValue)
{
  nsROCSSPrimitiveValue* val=GetROCSSPrimitiveValue();
  NS_ENSURE_TRUE(val, NS_ERROR_OUT_OF_MEMORY);

  const nsStyleBackground* color=nsnull;
  GetStyleData(eStyleStruct_Background, (const nsStyleStruct*&)color, aFrame);

  if(color) {
    if ((color->mBackgroundFlags & NS_STYLE_BG_COLOR_TRANSPARENT) &&
        !(color->mBackgroundFlags & NS_STYLE_BG_PROPAGATED_TO_PARENT)) {
      const nsAFlatCString& backgroundColor =
        nsCSSProps::SearchKeywordTable(NS_STYLE_BG_COLOR_TRANSPARENT,
                                       nsCSSProps::kBackgroundColorKTable);
      val->SetString(backgroundColor.get());
    }
    else {
      nsAutoString hex;
      ColorToHex(color->mBackgroundColor, hex);
      val->SetString(hex);
    }
  }
  else {
    val->SetString("");
  }

  return val->QueryInterface(NS_GET_IID(nsIDOMCSSPrimitiveValue),
                             (void **)&aValue);
}

nsresult
nsComputedDOMStyle::GetBackgroundImage(nsIFrame *aFrame,
                             nsIDOMCSSPrimitiveValue*& aValue)
{
  nsROCSSPrimitiveValue* val=GetROCSSPrimitiveValue();
  NS_ENSURE_TRUE(val, NS_ERROR_OUT_OF_MEMORY);

  const nsStyleBackground* color=nsnull;
  GetStyleData(eStyleStruct_Background, (const nsStyleStruct*&)color, aFrame);

  if(color) {
      val->SetString(color->mBackgroundImage);
  }
  else {
    val->SetString("");
  }

  return val->QueryInterface(NS_GET_IID(nsIDOMCSSPrimitiveValue),
                             (void **)&aValue);
}

nsresult
nsComputedDOMStyle::GetPadding(nsIFrame *aFrame,
                               nsIDOMCSSPrimitiveValue*& aValue)
{
  aValue=nsnull; // return null per spec.
  return NS_OK;
}

nsresult
nsComputedDOMStyle::GetPaddingTop(nsIFrame *aFrame,
                                  nsIDOMCSSPrimitiveValue*& aValue)
{
  return GetPaddingWidthFor(NS_SIDE_TOP, aFrame, aValue);
}

nsresult
nsComputedDOMStyle::GetPaddingBottom(nsIFrame *aFrame,
                                     nsIDOMCSSPrimitiveValue*& aValue)
{
  return GetPaddingWidthFor(NS_SIDE_BOTTOM, aFrame, aValue);
}

nsresult
nsComputedDOMStyle::GetPaddingLeft(nsIFrame *aFrame,
                               nsIDOMCSSPrimitiveValue*& aValue)
{
  return GetPaddingWidthFor(NS_SIDE_LEFT, aFrame, aValue);
}

nsresult
nsComputedDOMStyle::GetPaddingRight(nsIFrame *aFrame,
                               nsIDOMCSSPrimitiveValue*& aValue)
{
  return GetPaddingWidthFor(NS_SIDE_RIGHT, aFrame, aValue);
}

nsresult
nsComputedDOMStyle::GetBorderCollapse(nsIFrame *aFrame,
                                      nsIDOMCSSPrimitiveValue*& aValue)
{
  nsROCSSPrimitiveValue* val=GetROCSSPrimitiveValue();
  NS_ENSURE_TRUE(val, NS_ERROR_OUT_OF_MEMORY);

  const nsStyleTableBorder* table = nsnull;
  GetStyleData(eStyleStruct_TableBorder, (const nsStyleStruct*&)table, aFrame);

  if(table) {
    const nsAFlatCString& ident=
      nsCSSProps::SearchKeywordTable(table->mBorderCollapse,
                                     nsCSSProps::kBorderCollapseKTable);
    val->SetString(ident.get());
  }
  else {
    val->SetString("");
  }

  return val->QueryInterface(NS_GET_IID(nsIDOMCSSPrimitiveValue),
                             (void **)&aValue);
}

nsresult
nsComputedDOMStyle::GetBorderSpacing(nsIFrame *aFrame,
                                     nsIDOMCSSPrimitiveValue*& aValue)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

nsresult
nsComputedDOMStyle::GetBorderStyle(nsIFrame *aFrame,
                                   nsIDOMCSSPrimitiveValue*& aValue)
{
  aValue=nsnull; // return null per spec.
  return NS_OK;
}

nsresult
nsComputedDOMStyle::GetBorderTopStyle(nsIFrame *aFrame,
                                      nsIDOMCSSPrimitiveValue*& aValue)
{
  return GetBorderStyleFor(NS_SIDE_TOP, aFrame, aValue);
}

nsresult
nsComputedDOMStyle::GetBorderBottomStyle(nsIFrame *aFrame,
                                         nsIDOMCSSPrimitiveValue*& aValue)
{
  return GetBorderStyleFor(NS_SIDE_BOTTOM, aFrame, aValue);
}
nsresult
nsComputedDOMStyle::GetBorderLeftStyle(nsIFrame *aFrame,
                                       nsIDOMCSSPrimitiveValue*& aValue)
{
  return GetBorderStyleFor(NS_SIDE_LEFT, aFrame, aValue);
}

nsresult
nsComputedDOMStyle::GetBorderRightStyle(nsIFrame *aFrame,
                                        nsIDOMCSSPrimitiveValue*& aValue)
{
  return GetBorderStyleFor(NS_SIDE_RIGHT, aFrame, aValue);
}

nsresult
nsComputedDOMStyle::GetBorderWidth(nsIFrame *aFrame,
                                      nsIDOMCSSPrimitiveValue*& aValue)
{
  aValue=nsnull; // return null per spec.
  return NS_OK;
}

nsresult
nsComputedDOMStyle::GetBorderTopWidth(nsIFrame *aFrame,
                                      nsIDOMCSSPrimitiveValue*& aValue)
{
  return GetBorderWidthFor(NS_SIDE_TOP, aFrame, aValue);
}

nsresult
nsComputedDOMStyle::GetBorderBottomWidth(nsIFrame *aFrame,
                                         nsIDOMCSSPrimitiveValue*& aValue)
{
  return GetBorderWidthFor(NS_SIDE_BOTTOM, aFrame, aValue);
}

nsresult
nsComputedDOMStyle::GetBorderLeftWidth(nsIFrame *aFrame,
                                       nsIDOMCSSPrimitiveValue*& aValue)
{
   return GetBorderWidthFor(NS_SIDE_LEFT, aFrame, aValue);
}

nsresult
nsComputedDOMStyle::GetBorderRightWidth(nsIFrame *aFrame,
                                        nsIDOMCSSPrimitiveValue*& aValue)
{
   return GetBorderWidthFor(NS_SIDE_RIGHT, aFrame, aValue);
}

nsresult
nsComputedDOMStyle::GetBorderTopColor(nsIFrame *aFrame,
                                      nsIDOMCSSPrimitiveValue*& aValue)
{
  return GetBorderColorFor(NS_SIDE_TOP, aFrame, aValue);
}

nsresult
nsComputedDOMStyle::GetBorderBottomColor(nsIFrame *aFrame,
                                         nsIDOMCSSPrimitiveValue*& aValue)
{
  return GetBorderColorFor(NS_SIDE_BOTTOM, aFrame, aValue);
}

nsresult
nsComputedDOMStyle::GetBorderLeftColor(nsIFrame *aFrame,
                                       nsIDOMCSSPrimitiveValue*& aValue)
{
   return GetBorderColorFor(NS_SIDE_LEFT, aFrame, aValue);
}

nsresult
nsComputedDOMStyle::GetBorderRightColor(nsIFrame *aFrame,
                                        nsIDOMCSSPrimitiveValue*& aValue)
{
   return GetBorderColorFor(NS_SIDE_RIGHT, aFrame, aValue);
}

nsresult
nsComputedDOMStyle::GetMarginWidth(nsIFrame *aFrame,
                              nsIDOMCSSPrimitiveValue*& aValue)
{
  aValue=nsnull; // return null per spec.
  return NS_OK;
}

nsresult
nsComputedDOMStyle::GetMarginTopWidth(nsIFrame *aFrame,
                                      nsIDOMCSSPrimitiveValue*& aValue)
{
  return GetMarginWidthFor(NS_SIDE_TOP, aFrame, aValue);
}

nsresult
nsComputedDOMStyle::GetMarginBottomWidth(nsIFrame *aFrame,
                                         nsIDOMCSSPrimitiveValue*& aValue)
{
  return GetMarginWidthFor(NS_SIDE_BOTTOM, aFrame, aValue);
}

nsresult
nsComputedDOMStyle::GetMarginLeftWidth(nsIFrame *aFrame,
                                       nsIDOMCSSPrimitiveValue*& aValue)
{
   return GetMarginWidthFor(NS_SIDE_LEFT, aFrame, aValue);
}

nsresult
nsComputedDOMStyle::GetMarginRightWidth(nsIFrame *aFrame,
                                        nsIDOMCSSPrimitiveValue*& aValue)
{
   return GetMarginWidthFor(NS_SIDE_RIGHT, aFrame, aValue);
}

nsresult
nsComputedDOMStyle::GetMarkerOffset(nsIFrame *aFrame,
                                    nsIDOMCSSPrimitiveValue*& aValue)
{
  nsROCSSPrimitiveValue* val=GetROCSSPrimitiveValue();
  NS_ENSURE_TRUE(val, NS_ERROR_OUT_OF_MEMORY);

  const nsStyleContent* content=nsnull;
  GetStyleData(eStyleStruct_Content, (const nsStyleStruct*&)content, aFrame);

  val->SetTwips(content? content->mMarkerOffset.GetCoordValue():0);

  return val->QueryInterface(NS_GET_IID(nsIDOMCSSPrimitiveValue),
                             (void **)&aValue);
}

nsresult
nsComputedDOMStyle::GetOutline(nsIFrame *aFrame,
                               nsIDOMCSSPrimitiveValue*& aValue)
{
  aValue=nsnull; //  return null per spec.
  return NS_OK;
}

nsresult
nsComputedDOMStyle::GetOutlineWidth(nsIFrame *aFrame,
                                    nsIDOMCSSPrimitiveValue*& aValue)
{
  nsROCSSPrimitiveValue* val=GetROCSSPrimitiveValue();
  NS_ENSURE_TRUE(val, NS_ERROR_OUT_OF_MEMORY);

  const nsStyleOutline* outline=nsnull;
  GetStyleData(eStyleStruct_Outline, (const nsStyleStruct*&)outline, aFrame);

  val->SetTwips(outline? outline->mOutlineWidth.GetCoordValue():0);

  return val->QueryInterface(NS_GET_IID(nsIDOMCSSPrimitiveValue),
                             (void **)&aValue);
}

nsresult
nsComputedDOMStyle::GetOutlineStyle(nsIFrame *aFrame,
                                    nsIDOMCSSPrimitiveValue*& aValue)
{
  nsROCSSPrimitiveValue* val=GetROCSSPrimitiveValue();
  NS_ENSURE_TRUE(val, NS_ERROR_OUT_OF_MEMORY);

  const nsStyleOutline* outline=nsnull;
  GetStyleData(eStyleStruct_Outline, (const nsStyleStruct*&)outline, aFrame);

  if(outline) {
    const nsAFlatCString& style=
      nsCSSProps::SearchKeywordTable(outline->GetOutlineStyle(),
                                     nsCSSProps::kBorderStyleKTable);
    val->SetString(style.get());
  }
  else {
    val->SetString("");
  }

  return val->QueryInterface(NS_GET_IID(nsIDOMCSSPrimitiveValue),
                             (void **)&aValue);
}

nsresult
nsComputedDOMStyle::GetOutlineColor(nsIFrame *aFrame,
                                    nsIDOMCSSPrimitiveValue*& aValue)
{
  nsROCSSPrimitiveValue* val=GetROCSSPrimitiveValue();
  NS_ENSURE_TRUE(val, NS_ERROR_OUT_OF_MEMORY);

  const nsStyleOutline* outline=nsnull;
  GetStyleData(eStyleStruct_Outline, (const nsStyleStruct*&)outline, aFrame);

  if(outline) {
    nscolor color;
    outline->GetOutlineColor(color);

    nsAutoString hex;
    ColorToHex(color, hex);

    val->SetString(hex);
  }
  else {
    val->SetString("");
  }

  return val->QueryInterface(NS_GET_IID(nsIDOMCSSPrimitiveValue),
                             (void **)&aValue);
}

nsresult
nsComputedDOMStyle::GetZIndex(nsIFrame *aFrame,
                              nsIDOMCSSPrimitiveValue*& aValue)
{
  nsROCSSPrimitiveValue* val=GetROCSSPrimitiveValue();
  NS_ENSURE_TRUE(val, NS_ERROR_OUT_OF_MEMORY);

  const nsStylePosition* position = nsnull;
  nsAutoString zindex;

  GetStyleData(eStyleStruct_Position, (const nsStyleStruct*&)position, aFrame);
  if (position) {
    switch (position->mZIndex.GetUnit()) {
      case eStyleUnit_Integer:
        zindex.AppendInt(position->mZIndex.GetIntValue(), 10);
        val->SetString(zindex);
        break;
      case eStyleUnit_Auto:
        val->SetString(NS_LITERAL_STRING("auto"));
        break;
      case eStyleUnit_Inherit:
        val->SetString(NS_LITERAL_STRING("inherit"));
        break;
      default:
        NS_WARNING("Double Check the Unit!");
        val->SetString(NS_LITERAL_STRING("auto"));
        break;
    }
  } else {
    val->SetString(NS_LITERAL_STRING("auto"));
  }
    
  return val->QueryInterface(NS_GET_IID(nsIDOMCSSPrimitiveValue),
                             (void **)&aValue);
}

nsresult
nsComputedDOMStyle::GetListStyleImage(nsIFrame *aFrame,
                                      nsIDOMCSSPrimitiveValue*& aValue)
{
  nsROCSSPrimitiveValue* val=GetROCSSPrimitiveValue();
  NS_ENSURE_TRUE(val, NS_ERROR_OUT_OF_MEMORY);

  const nsStyleList* list;
  GetStyleData(eStyleStruct_List, (const nsStyleStruct*&)list, aFrame);

  if(list) {
    val->SetString(list->mListStyleImage);
  }
  else {
    val->SetString("");
  }
    
  return val->QueryInterface(NS_GET_IID(nsIDOMCSSPrimitiveValue),
                             (void**)&aValue);
}

nsresult
nsComputedDOMStyle::GetTextAlign(nsIFrame *aFrame,
                                      nsIDOMCSSPrimitiveValue*& aValue)
{
  nsROCSSPrimitiveValue* val = GetROCSSPrimitiveValue();
  NS_ENSURE_TRUE(val, NS_ERROR_OUT_OF_MEMORY);

  const nsStyleText* text = nsnull;
  GetStyleData(eStyleStruct_Text, (const nsStyleStruct*&)text, aFrame);

  if(text) {
    const nsAFlatCString& align=
      nsCSSProps::SearchKeywordTable(text->mTextAlign,
                                     nsCSSProps::kTextAlignKTable);
    val->SetString(align.get());
  }
  else {
    val->SetString("start");
  }

  return val->QueryInterface(NS_GET_IID(nsIDOMCSSPrimitiveValue),
                             (void **)&aValue);
}

nsresult
nsComputedDOMStyle::GetTextDecoration(nsIFrame *aFrame,
                                      nsIDOMCSSPrimitiveValue*& aValue)
{
  nsROCSSPrimitiveValue* val=GetROCSSPrimitiveValue();
  NS_ENSURE_TRUE(val, NS_ERROR_OUT_OF_MEMORY);

  const nsStyleTextReset* text=nsnull;
  GetStyleData(eStyleStruct_TextReset,(const nsStyleStruct*&)text,aFrame);

  if(text) {
    const nsAFlatCString& decoration=
      nsCSSProps::SearchKeywordTable(text->mTextDecoration,
	                                 nsCSSProps::kTextDecorationKTable);
    val->SetString(decoration.get());
  }
  else {
    val->SetString("");
  }

  return val->QueryInterface(NS_GET_IID(nsIDOMCSSPrimitiveValue),
	                         (void **)&aValue);
}

nsresult
nsComputedDOMStyle::GetVisibility(nsIFrame *aFrame,
                                  nsIDOMCSSPrimitiveValue*& aValue)
{
  nsROCSSPrimitiveValue* val=GetROCSSPrimitiveValue();
  NS_ENSURE_TRUE(val, NS_ERROR_OUT_OF_MEMORY);

  const nsStyleVisibility* visibility=nsnull;
  GetStyleData(eStyleStruct_Visibility,(const nsStyleStruct*&)visibility,aFrame);

  if(visibility) {
    const nsAFlatCString& value=
      nsCSSProps::SearchKeywordTable(visibility->mVisible,
                                     nsCSSProps::kVisibilityKTable);
    val->SetString(value.get());
  }
  else {
    val->SetString("");
  }

  return val->QueryInterface(NS_GET_IID(nsIDOMCSSPrimitiveValue),
                             (void **)&aValue);
}

#if 0
NS_IMETHODIMP
nsComputedDOMStyle::GetCaptionSide(nsAWritableString& aCaptionSide)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetClear(nsAWritableString& aClear)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetClip(nsAWritableString& aClip)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetColor(nsAWritableString& aColor)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetContent(nsAWritableString& aContent)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetCounterIncrement(nsAWritableString& aCounterIncrement)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetCounterReset(nsAWritableString& aCounterReset)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetCssFloat(nsAWritableString& aFloat)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetCue(nsAWritableString& aCue)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetCueAfter(nsAWritableString& aCueAfter)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetCueBefore(nsAWritableString& aCueBefore)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetCursor(nsAWritableString& aCursor)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetDirection(nsAWritableString& aDirection)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}
#endif

nsresult
nsComputedDOMStyle::GetDisplay(nsIFrame *aFrame,
                               nsIDOMCSSPrimitiveValue*& aValue)
{
  nsROCSSPrimitiveValue *val = GetROCSSPrimitiveValue();
  NS_ENSURE_TRUE(val, NS_ERROR_OUT_OF_MEMORY);


  const nsStyleDisplay* display = nsnull;

  GetStyleData(eStyleStruct_Display, (const nsStyleStruct*&)display, aFrame);

  if (display) {
    switch (display->mDisplay) {
    case NS_STYLE_DISPLAY_NONE :
      val->SetString("none"); break;
    case NS_STYLE_DISPLAY_BLOCK :
      val->SetString("block"); break;
    case NS_STYLE_DISPLAY_INLINE :
      val->SetString("inline"); break;
    case NS_STYLE_DISPLAY_INLINE_BLOCK :
      val->SetString("inline_block"); break;
    case NS_STYLE_DISPLAY_LIST_ITEM :
      val->SetString("list-item"); break;
    case NS_STYLE_DISPLAY_MARKER :
      val->SetString("marker"); break;
    case NS_STYLE_DISPLAY_RUN_IN :
      val->SetString("run-in"); break;
    case NS_STYLE_DISPLAY_COMPACT :
      val->SetString("compact"); break;
    case NS_STYLE_DISPLAY_TABLE :
      val->SetString("table"); break;
    case NS_STYLE_DISPLAY_INLINE_TABLE :
      val->SetString("inline-table"); break;
    case NS_STYLE_DISPLAY_TABLE_ROW_GROUP :
      val->SetString("table-row-group"); break;
    case NS_STYLE_DISPLAY_TABLE_COLUMN :
      val->SetString("table-column"); break;
    case NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP :
      val->SetString("table-column-group"); break;
    case NS_STYLE_DISPLAY_TABLE_HEADER_GROUP :
      val->SetString("table-header-group"); break;
    case NS_STYLE_DISPLAY_TABLE_FOOTER_GROUP :
      val->SetString("table-footer-group"); break;
    case NS_STYLE_DISPLAY_TABLE_ROW :
      val->SetString("table-row"); break;
    case NS_STYLE_DISPLAY_TABLE_CELL :
      val->SetString("table-cell"); break;
    case NS_STYLE_DISPLAY_TABLE_CAPTION :
      val->SetString("table-caption"); break;
    case NS_STYLE_DISPLAY_MENU :
      val->SetString("menu"); break;
    default :
      val->SetString(""); break;

      break;
    }
  }

  return val->QueryInterface(NS_GET_IID(nsIDOMCSSPrimitiveValue),
                             (void **)&aValue);
}

#if 0
NS_IMETHODIMP
nsComputedDOMStyle::GetElevation(nsAWritableString& aElevation)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetEmptyCells(nsAWritableString& aEmptyCells)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetFont(nsAWritableString& aFont)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetFontFamily(nsAWritableString& aFontFamily)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetFontSize(nsAWritableString& aFontSize)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetFontSizeAdjust(nsAWritableString& aFontSizeAdjust)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetFontStretch(nsAWritableString& aFontStretch)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetFontStyle(nsAWritableString& aFontStyle)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetFontVariant(nsAWritableString& aFontVariant)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetFontWeight(nsAWritableString& aFontWeight)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

#endif

nsresult
nsComputedDOMStyle::GetHeight(nsIFrame *aFrame,
                             nsIDOMCSSPrimitiveValue*& aValue)
{
  nsROCSSPrimitiveValue *val = GetROCSSPrimitiveValue();
  NS_ENSURE_TRUE(val, NS_ERROR_OUT_OF_MEMORY);

  PRBool calcHeight = PR_FALSE;
  
  if (aFrame) {
    calcHeight = PR_TRUE;
    // Flush all pending notifications so that our frames are up to date
    nsCOMPtr<nsIDocument> document;
    mContent->GetDocument(*getter_AddRefs(document));

    if (document) {
      document->FlushPendingNotifications();
    }
  
    const nsStyleDisplay* displayData = nsnull;
    GetStyleData(eStyleStruct_Display, (const nsStyleStruct*&)displayData, aFrame);
    if (displayData && displayData->mDisplay == NS_STYLE_DISPLAY_INLINE) {
      nsFrameState frameState;
      aFrame->GetFrameState(&frameState);
      if (! (frameState & NS_FRAME_REPLACED_ELEMENT)) {
        calcHeight = PR_FALSE;
      }
    }
  }

  if (calcHeight) {
    nsRect rect;
    nsMargin padding;
    nsMargin border;
    aFrame->GetRect(rect);
    const nsStylePadding* paddingData=nsnull;
    GetStyleData(eStyleStruct_Padding, (const nsStyleStruct*&)paddingData, aFrame);
    if (paddingData) {
      paddingData->CalcPaddingFor(aFrame, padding);
    }
    const nsStyleBorder* borderData=nsnull;
    GetStyleData(eStyleStruct_Border, (const nsStyleStruct*&)borderData, aFrame);
    if (borderData) {
      borderData->CalcBorderFor(aFrame, border);
    }
  
    val->SetTwips(rect.height - padding.top - padding.bottom -
                  border.top - border.bottom);
  } else {
    // Just return the value in the style context
    const nsStylePosition* positionData = nsnull;
    GetStyleData(eStyleStruct_Position, (const nsStyleStruct*&)positionData, aFrame);
    if (positionData) {
      switch (positionData->mHeight.GetUnit()) {
        case eStyleUnit_Coord:
          val->SetTwips(positionData->mHeight.GetCoordValue());
          break;
        case eStyleUnit_Percent:
          val->SetPercent(positionData->mHeight.GetPercentValue());
          break;
        case eStyleUnit_Auto:
          val->SetString(NS_LITERAL_STRING("auto"));
          break;
        case eStyleUnit_Inherit:
          val->SetString(NS_LITERAL_STRING("inherit"));
          break;
        default:
          NS_WARNING("Double check the unit");
          val->SetTwips(0);
          break;
      }
    } else {
      val->SetTwips(0);
    }
  }
  
  return val->QueryInterface(NS_GET_IID(nsIDOMCSSPrimitiveValue),
                             (void **)&aValue);
}

nsresult
nsComputedDOMStyle::GetLeft(nsIFrame *aFrame,
                            nsIDOMCSSPrimitiveValue*& aValue)
{
  return GetOffsetWidthFor(NS_SIDE_LEFT, aFrame, aValue);
}

#if 0
NS_IMETHODIMP
nsComputedDOMStyle::GetLetterSpacing(nsAWritableString& aLetterSpacing)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetLineHeight(nsAWritableString& aLineHeight)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetListStyle(nsAWritableString& aListStyle)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetListStyleImage(nsAWritableString& aListStyleImage)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetListStylePosition(nsAWritableString& aListStylePosition)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetListStyleType(nsAWritableString& aListStyleType)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetMargin(nsAWritableString& aMargin)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetMarginTop(nsIFrame *aFrame,
                                 nsIDOMCSSPrimitiveValue*& aValue)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetMarginRight(nsIFrame *aFrame,
                                   nsIDOMCSSPrimitiveValue*& aValue)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetMarginBottom(nsIFrame *aFrame,
                                    nsIDOMCSSPrimitiveValue*& aValue)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetMarginLeft(nsIFrame *aFrame,
                                  nsIDOMCSSPrimitiveValue*& aValue)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetMarkerOffset(nsAWritableString& aMarkerOffset)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetMarks(nsAWritableString& aMarks)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetMaxHeight(nsAWritableString& aMaxHeight)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetMaxWidth(nsAWritableString& aMaxWidth)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetMinHeight(nsAWritableString& aMinHeight)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetMinWidth(nsAWritableString& aMinWidth)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetOrphans(nsAWritableString& aOrphans)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetOutline(nsAWritableString& aOutline)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetOutlineColor(nsAWritableString& aOutlineColor)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetOutlineStyle(nsAWritableString& aOutlineStyle)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetOutlineWidth(nsAWritableString& aOutlineWidth)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetOverflow(nsAWritableString& aOverflow)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetPadding(nsAWritableString& aPadding)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetPaddingTop(nsAWritableString& aPaddingTop)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetPaddingRight(nsAWritableString& aPaddingRight)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetPaddingBottom(nsAWritableString& aPaddingBottom)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetPaddingLeft(nsAWritableString& aPaddingLeft)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetPage(nsAWritableString& aPage)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetPageBreakAfter(nsAWritableString& aPageBreakAfter)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetPageBreakBefore(nsAWritableString& aPageBreakBefore)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetPageBreakInside(nsAWritableString& aPageBreakInside)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetPause(nsAWritableString& aPause)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetPauseAfter(nsAWritableString& aPauseAfter)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetPauseBefore(nsAWritableString& aPauseBefore)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetPitch(nsAWritableString& aPitch)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetPitchRange(nsAWritableString& aPitchRange)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetPlayDuring(nsAWritableString& aPlayDuring)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetPosition(nsAWritableString& aPosition)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetQuotes(nsAWritableString& aQuotes)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetRichness(nsAWritableString& aRichness)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}
#endif

nsresult
nsComputedDOMStyle::GetRight(nsIFrame *aFrame,
                             nsIDOMCSSPrimitiveValue*& aValue)
{
  return GetOffsetWidthFor(NS_SIDE_RIGHT, aFrame, aValue);
}

#if 0
NS_IMETHODIMP
nsComputedDOMStyle::GetSize(nsAWritableString& aSize)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetSpeak(nsAWritableString& aSpeak)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetSpeakHeader(nsAWritableString& aSpeakHeader)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetSpeakNumeral(nsAWritableString& aSpeakNumeral)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetSpeakPunctuation(nsAWritableString& aSpeakPunctuation)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetSpeechRate(nsAWritableString& aSpeechRate)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetStress(nsAWritableString& aStress)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetTableLayout(nsAWritableString& aTableLayout)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetTextAlign(nsAWritableString& aTextAlign)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetTextDecoration(nsAWritableString& aTextDecoration)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetTextIndent(nsAWritableString& aTextIndent)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetTextShadow(nsAWritableString& aTextShadow)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetTextTransform(nsAWritableString& aTextTransform)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}
#endif

nsresult
nsComputedDOMStyle::GetTop(nsIFrame *aFrame,
                           nsIDOMCSSPrimitiveValue*& aValue)
{
  return GetOffsetWidthFor(NS_SIDE_TOP, aFrame, aValue);
}

#if 0
NS_IMETHODIMP
nsComputedDOMStyle::GetUnicodeBidi(nsAWritableString& aUnicodeBidi)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetVerticalAlign(nsAWritableString& aVerticalAlign)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetVisibility(nsAWritableString& aVisibility)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetVoiceFamily(nsAWritableString& aVoiceFamily)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetVolume(nsAWritableString& aVolume)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetWhiteSpace(nsAWritableString& aWhiteSpace)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetWidows(nsAWritableString& aWidows)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}
#endif

nsROCSSPrimitiveValue*
nsComputedDOMStyle::GetROCSSPrimitiveValue()
{
  nsISupports *tmp = NS_STATIC_CAST(nsIComputedDOMStyle *, this);

  nsROCSSPrimitiveValue *primitiveValue = new nsROCSSPrimitiveValue(tmp, mT2P);

  NS_ASSERTION(primitiveValue!=0, "ran out of memory");

  return primitiveValue;
}

nsresult
nsComputedDOMStyle::GetOffsetWidthFor(PRUint8 aSide, nsIFrame* aFrame,
                                      nsIDOMCSSPrimitiveValue*& aValue)
{
  const nsStyleDisplay* display = nsnull;
  GetStyleData(eStyleStruct_Display, (const nsStyleStruct*&)display, aFrame);

  // Flush all pending notifications so that our frames are up to date
  nsCOMPtr<nsIDocument> document;
  mContent->GetDocument(*getter_AddRefs(document));
  if (document) {
    document->FlushPendingNotifications();
  }

  nsresult rv = NS_OK;
  if (display) {
    switch (display->mPosition) {
      case NS_STYLE_POSITION_NORMAL:
        rv = GetStaticOffset(aSide, aFrame, aValue);
        break;
      case NS_STYLE_POSITION_RELATIVE:
        rv = GetRelativeOffset(aSide, aFrame, aValue);
        break;
      case NS_STYLE_POSITION_ABSOLUTE:
      case NS_STYLE_POSITION_FIXED:
        rv = GetAbsoluteOffset(aSide, aFrame, aValue);
        break;
      default:
        NS_WARNING("double check the position");
        break;
    }
  }

  return rv;
}

nsresult
nsComputedDOMStyle::GetAbsoluteOffset(PRUint8 aSide, nsIFrame* aFrame,
                                      nsIDOMCSSPrimitiveValue*& aValue)
{
  nsROCSSPrimitiveValue *val = GetROCSSPrimitiveValue();
  NS_ENSURE_TRUE(val, NS_ERROR_OUT_OF_MEMORY);

  nsIFrame* container = GetContainingBlock(aFrame);
  if (container) {
    nsRect rect;
    nsRect containerRect;
    nscoord margin = GetMarginWidthCoordFor(aSide, aFrame);
    nscoord border = GetBorderWidthCoordFor(aSide, container);
    nscoord horScrollBarHeight = 0;
    nscoord verScrollBarWidth = 0;
    aFrame->GetRect(rect);
    container->GetRect(containerRect);
      
    nsCOMPtr<nsIAtom> typeAtom;
    container->GetFrameType(getter_AddRefs(typeAtom));
    if (typeAtom == nsLayoutAtoms::viewportFrame) {
      // For absolutely positioned frames scrollbars are taken into
      // account by virtue of getting a containing block that does
      // _not_ include the scrollbars.  For fixed positioned frames,
      // the containing block is the viewport, which _does_ include
      // scrollbars.  We have to do some extra work.
      nsIFrame* scrollingChild;
      nsCOMPtr<nsIPresShell> presShell = do_QueryReferent(mPresShellWeak);
      NS_ASSERTION(presShell, "Must have a presshell!");
      nsCOMPtr<nsIPresContext> presContext;
      presShell->GetPresContext(getter_AddRefs(presContext));
      // the first child in the default frame list is what we want
      container->FirstChild(presContext, nsnull, &scrollingChild);
      nsCOMPtr<nsIScrollableFrame> scrollFrame(do_QueryInterface(scrollingChild));
      if (scrollFrame) {
        scrollFrame->GetScrollbarSizes(presContext, &verScrollBarWidth,
                                       &horScrollBarHeight);
        PRBool verScrollBarVisible;
        PRBool horScrollBarVisible;
        scrollFrame->GetScrollbarVisibility(presContext, &verScrollBarVisible,
                                            &horScrollBarVisible);
        if (!verScrollBarVisible) {
          verScrollBarWidth = 0;
        }
        if (!horScrollBarVisible) {
          horScrollBarHeight = 0;
        }
      }
    }
    nscoord offset = 0;
    switch (aSide) {
      case NS_SIDE_TOP:
        offset = rect.y - margin - border;
        break;
      case NS_SIDE_RIGHT:
        offset = containerRect.width - rect.width -
          rect.x - margin - border - verScrollBarWidth;
        break;
      case NS_SIDE_BOTTOM:
        offset = containerRect.height - rect.height -
          rect.y - margin - border - horScrollBarHeight;
        break;
      case NS_SIDE_LEFT:
        offset = rect.x - margin - border;
        break;
      default:
        NS_WARNING("double check the side");
        break;
    }
    val->SetTwips(offset);
  } else {
    // XXX no frame.  This property makes no sense
    val->SetTwips(0);
  }

  return val->QueryInterface(NS_GET_IID(nsIDOMCSSPrimitiveValue),
                             (void **)&aValue);
}

nsresult
nsComputedDOMStyle::GetRelativeOffset(PRUint8 aSide, nsIFrame* aFrame,
                                      nsIDOMCSSPrimitiveValue*& aValue)
{
  nsROCSSPrimitiveValue *val = GetROCSSPrimitiveValue();
  NS_ENSURE_TRUE(val, NS_ERROR_OUT_OF_MEMORY);

  const nsStylePosition* positionData = nsnull;
  GetStyleData(eStyleStruct_Position, (const nsStyleStruct*&)positionData, aFrame);
  if (positionData) {
    nsStyleCoord coord;
    PRInt32 sign = 1;
    switch (aSide) {
      case NS_SIDE_TOP:
        positionData->mOffset.GetTop(coord);
        if (coord.GetUnit() != eStyleUnit_Coord &&
            coord.GetUnit() != eStyleUnit_Percent) {
          positionData->mOffset.GetBottom(coord);
          sign = -1;
        }
        break;
      case NS_SIDE_RIGHT:
        positionData->mOffset.GetRight(coord);
        if (coord.GetUnit() != eStyleUnit_Coord &&
            coord.GetUnit() != eStyleUnit_Percent) {
          positionData->mOffset.GetLeft(coord);
          sign = -1;
        }
        break;
      case NS_SIDE_BOTTOM:
        positionData->mOffset.GetBottom(coord);
        if (coord.GetUnit() != eStyleUnit_Coord &&
            coord.GetUnit() != eStyleUnit_Percent) {
          positionData->mOffset.GetTop(coord);
          sign = -1;
        }
        break;
      case NS_SIDE_LEFT:
        positionData->mOffset.GetLeft(coord);
        if (coord.GetUnit() != eStyleUnit_Coord &&
            coord.GetUnit() != eStyleUnit_Percent) {
          positionData->mOffset.GetRight(coord);
          sign = -1;
        }
        break;
      default:
        NS_WARNING("double check the side");
        break;
    }

    nsIFrame* container = nsnull;
    const nsStyleBorder* borderData = nsnull;
    const nsStylePadding* paddingData = nsnull;
    nsMargin border;
    nsMargin padding;
    nsRect rect;
    switch(coord.GetUnit()) {
      case eStyleUnit_Coord:
        val->SetTwips(sign * coord.GetCoordValue());
        break;
      case eStyleUnit_Percent:
        container = GetContainingBlock(aFrame);
        if (container) {
          container->GetStyleData(eStyleStruct_Border, (const nsStyleStruct*&)borderData);
          if (borderData) {
            borderData->CalcBorderFor(container, border);
          }
          container->GetStyleData(eStyleStruct_Padding, (const nsStyleStruct*&)paddingData);
          if (paddingData) {
            paddingData->CalcPaddingFor(container, padding);
          }
          container->GetRect(rect);
          if (aSide == NS_SIDE_LEFT || aSide == NS_SIDE_RIGHT) {
            val->SetTwips(sign * coord.GetPercentValue() *
                          (rect.width - border.left - border.right -
                           padding.left - padding.right));
          } else {
            val->SetTwips(sign * coord.GetPercentValue() *
                          (rect.height - border.top - border.bottom -
                           padding.top - padding.bottom));
          }
        } else {
          // XXX no containing block.
          val->SetTwips(0);
        }
        break;
      default:
        NS_WARNING("double check the unit");
        val->SetTwips(0);
        break;
    }
  }

  return val->QueryInterface(NS_GET_IID(nsIDOMCSSPrimitiveValue),
                             (void **)&aValue);
}

nsresult
nsComputedDOMStyle::GetStaticOffset(PRUint8 aSide, nsIFrame* aFrame,
                                    nsIDOMCSSPrimitiveValue*& aValue)

{
  nsROCSSPrimitiveValue *val = GetROCSSPrimitiveValue();
  NS_ENSURE_TRUE(val, NS_ERROR_OUT_OF_MEMORY);

  const nsStylePosition* positionData = nsnull;
  GetStyleData(eStyleStruct_Position, (const nsStyleStruct*&)positionData, aFrame);
  if (positionData) {
    nsStyleCoord coord;
    switch (aSide) {
      case NS_SIDE_TOP:
        positionData->mOffset.GetTop(coord);
        break;
      case NS_SIDE_RIGHT:
        positionData->mOffset.GetRight(coord);
        break;
      case NS_SIDE_BOTTOM:
        positionData->mOffset.GetBottom(coord);
        break;
      case NS_SIDE_LEFT:
        positionData->mOffset.GetLeft(coord);
        break;
      default:
        NS_WARNING("double check the side");
        break;
    }

    switch(coord.GetUnit()) {
      case eStyleUnit_Coord:
        val->SetTwips(coord.GetCoordValue());
        break;
      case eStyleUnit_Percent:
        val->SetPercent(coord.GetPercentValue());
        break;
      default:
        NS_WARNING("double check the unit");
        val->SetTwips(0);
        break;
    }
  }
  
  return val->QueryInterface(NS_GET_IID(nsIDOMCSSPrimitiveValue),
                             (void **)&aValue);
}


nsIFrame*
nsComputedDOMStyle::GetContainingBlock(nsIFrame *aFrame)
{
  if (!aFrame) {
    // Tell caller that they have no containing block, seeing as they
    // are not being displayed
    return nsnull;
  }
  nsIFrame* container = aFrame;
  PRBool done = PR_FALSE;
  do {
    container->GetParent(&container);
    if (container) {
      (container)->IsPercentageBase(done);
    }
  } while (!done && container);

  NS_POSTCONDITION(container, "Frame has no containing block");
  
  return container;
}

nsresult
nsComputedDOMStyle::GetStyleData(nsStyleStructID aID,
                                 const nsStyleStruct*& aStyleStruct,
                                 nsIFrame* aFrame)
{
  if(aFrame && !mPseudo) {
    aFrame->GetStyleData(aID, aStyleStruct);
  }
  else if (mStyleContextHolder) {
    aStyleStruct = mStyleContextHolder->GetStyleData(aID);    
  } else {
    nsCOMPtr<nsIPresShell> presShell=do_QueryReferent(mPresShellWeak);

    NS_ENSURE_TRUE(presShell, NS_ERROR_FAILURE);

    nsCOMPtr<nsIPresContext> pctx;
    presShell->GetPresContext(getter_AddRefs(pctx));
    if(pctx) {
      nsCOMPtr<nsIStyleContext> sctx;
      if(!mPseudo) {
        pctx->ResolveStyleContextFor(mContent, nsnull, PR_FALSE,
                                     getter_AddRefs(sctx));
      }
      else {
        pctx->ResolvePseudoStyleContextFor(mContent, mPseudo, nsnull, PR_FALSE,
                                           getter_AddRefs(sctx));
      }
      if(sctx) {
        aStyleStruct=sctx->GetStyleData(aID);
      }
      mStyleContextHolder=sctx;
    }
  }
  NS_ASSERTION(aStyleStruct, "Failed to get a style struct");
  return NS_OK;
}

nsresult
nsComputedDOMStyle::GetPaddingWidthFor(PRUint8 aSide,
                                       nsIFrame *aFrame,
                                       nsIDOMCSSPrimitiveValue*& aValue)
{
  nsROCSSPrimitiveValue* val=GetROCSSPrimitiveValue();
  NS_ENSURE_TRUE(val, NS_ERROR_OUT_OF_MEMORY);
  
  nscoord width = GetPaddingWidthCoordFor(aSide, aFrame);
  val->SetTwips(width);

  return val->QueryInterface(NS_GET_IID(nsIDOMCSSPrimitiveValue),
                             (void **)&aValue);
}

nscoord
nsComputedDOMStyle::GetPaddingWidthCoordFor(PRUint8 aSide, nsIFrame* aFrame)
{
  const nsStylePadding* paddingData=nsnull;
  GetStyleData(eStyleStruct_Padding, (const nsStyleStruct*&)paddingData, aFrame);

  if(paddingData) {
    nsMargin padding;
    paddingData->CalcPaddingFor(aFrame, padding);
    switch(aSide) {
      case NS_SIDE_TOP    :
        return padding.top;
        break;
      case NS_SIDE_BOTTOM :
        return padding.bottom;
        break;
      case NS_SIDE_LEFT   :
        return padding.left;
        break;
      case NS_SIDE_RIGHT  :
        return padding.right;
        break;
      default:
        NS_WARNING("double check the side");
        break;
    }
  }

  return 0;
}

nscoord
nsComputedDOMStyle::GetBorderWidthCoordFor(PRUint8 aSide, nsIFrame *aFrame)
{
  nsROCSSPrimitiveValue* val=GetROCSSPrimitiveValue();
  NS_ENSURE_TRUE(val, NS_ERROR_OUT_OF_MEMORY);

  const nsStyleBorder* borderData = nsnull;
  GetStyleData(eStyleStruct_Border, (const nsStyleStruct*&)borderData, aFrame);

  if(borderData) {
    nsMargin border;
    borderData->CalcBorderFor(aFrame, border);
    switch(aSide) {
      case NS_SIDE_TOP    :
        return border.top;
        break;
      case NS_SIDE_BOTTOM :
        return border.bottom;
        break;
      case NS_SIDE_LEFT   :
        return border.left;
        break;
      case NS_SIDE_RIGHT  :
        return border.right;
        break;
      default:
        NS_WARNING("double check the side");
        break;
    }
  }

  return 0;
}

nsresult
nsComputedDOMStyle::GetBorderWidthFor(PRUint8 aSide,
                                      nsIFrame *aFrame,
                                      nsIDOMCSSPrimitiveValue*& aValue)
{
  nsROCSSPrimitiveValue* val=GetROCSSPrimitiveValue();
  NS_ENSURE_TRUE(val, NS_ERROR_OUT_OF_MEMORY);

  const nsStyleBorder* border = nsnull;
  GetStyleData(eStyleStruct_Border, (const nsStyleStruct*&)border, aFrame);

  if(border) {
    nsStyleCoord coord;
   PRUint8 borderStyle = border->GetBorderStyle(aSide);
   if (borderStyle == NS_STYLE_BORDER_STYLE_NONE) {
     coord.SetCoordValue(0);
   }
   else {
      switch(aSide) {
        case NS_SIDE_TOP:
          border->mBorder.GetTop(coord); break;
        case NS_SIDE_BOTTOM :
          border->mBorder.GetBottom(coord); break;
        case NS_SIDE_LEFT :
          border->mBorder.GetLeft(coord); break;
        case NS_SIDE_RIGHT :
          border->mBorder.GetRight(coord); break;
        default:
          NS_WARNING("double check the side");
          break;
      }
   }

    switch(coord.GetUnit()) {
      case eStyleUnit_Coord:
        val->SetTwips(coord.GetCoordValue());  break;
      case eStyleUnit_Integer:
      case eStyleUnit_Proportional:
      case eStyleUnit_Enumerated:
      case eStyleUnit_Chars:
        {
          const nsAFlatCString& width=
            nsCSSProps::SearchKeywordTable(coord.GetIntValue(),
                                           nsCSSProps::kBorderWidthKTable);
          val->SetString(width.get()); break;
        }
      default:
        NS_WARNING("double check the unit");
        break;
    }
  }

  return val->QueryInterface(NS_GET_IID(nsIDOMCSSPrimitiveValue),
                             (void **)&aValue);
}

nsresult
nsComputedDOMStyle::GetBorderColorFor(PRUint8 aSide,
                                      nsIFrame *aFrame,
                                      nsIDOMCSSPrimitiveValue*& aValue)
{
  nsROCSSPrimitiveValue* val=GetROCSSPrimitiveValue();
  NS_ENSURE_TRUE(val, NS_ERROR_OUT_OF_MEMORY);

  const nsStyleBorder* border = nsnull;
  GetStyleData(eStyleStruct_Border,(const nsStyleStruct*&)border,aFrame);

  if(border) {
    nscolor color; 
    PRBool transparent;
    PRBool foreground;
    border->GetBorderColor(aSide, color, transparent, foreground);
    if (foreground) {
      const nsStyleColor* colorStruct = nsnull;
      GetStyleData(eStyleStruct_Color,(const nsStyleStruct*&)colorStruct,
                   aFrame);
      color = colorStruct->mColor;
    }

    nsAutoString hex;
    ColorToHex(color, hex);
    val->SetString(hex);
  }
  else {
    val->SetString("");
  }

  return val->QueryInterface(NS_GET_IID(nsIDOMCSSPrimitiveValue),
                             (void **)&aValue);
}

nsresult
nsComputedDOMStyle::GetMarginWidthFor(PRUint8 aSide,
                                      nsIFrame *aFrame,
                                      nsIDOMCSSPrimitiveValue*& aValue)
{
  nsROCSSPrimitiveValue* val=GetROCSSPrimitiveValue();
  NS_ENSURE_TRUE(val, NS_ERROR_OUT_OF_MEMORY);

  nscoord width = GetMarginWidthCoordFor(aSide, aFrame);
  val->SetTwips(width);

  return val->QueryInterface(NS_GET_IID(nsIDOMCSSPrimitiveValue),
                             (void **)&aValue);
}

nscoord
nsComputedDOMStyle::GetMarginWidthCoordFor(PRUint8 aSide,
                                           nsIFrame *aFrame)
{
  const nsStyleMargin* marginData=nsnull;
  GetStyleData(eStyleStruct_Margin, (const nsStyleStruct*&)marginData, aFrame);
  if(marginData) {
    nsMargin margin;
    marginData->CalcMarginFor(aFrame, margin);
    switch(aSide) {
      case NS_SIDE_TOP    :
        return margin.top;
        break;
      case NS_SIDE_BOTTOM :
        return margin.bottom;
        break;
      case NS_SIDE_LEFT   :
        return margin.left;
        break;
      case NS_SIDE_RIGHT  :
        return margin.right;
        break;
      default:
        NS_WARNING("double check the side");
        break;
    }
  }

  return 0;
}

nsresult
nsComputedDOMStyle::GetBorderStyleFor(PRUint8 aSide,
                                      nsIFrame *aFrame,
                                      nsIDOMCSSPrimitiveValue*& aValue)
{
  nsROCSSPrimitiveValue* val=GetROCSSPrimitiveValue();
  NS_ENSURE_TRUE(val, NS_ERROR_OUT_OF_MEMORY);

  const nsStyleBorder* border = nsnull;
  GetStyleData(eStyleStruct_Border, (const nsStyleStruct*&)border, aFrame);

  if(border) {
    const nsAFlatCString& style=
      nsCSSProps::SearchKeywordTable(border->GetBorderStyle(aSide),
                                     nsCSSProps::kBorderStyleKTable);
    val->SetString(style.get());
  }
  else {
    val->SetString("");
  }

  return val->QueryInterface(NS_GET_IID(nsIDOMCSSPrimitiveValue),
                             (void **)&aValue);
}

nsresult
nsComputedDOMStyle::GetWidth(nsIFrame *aFrame,
                             nsIDOMCSSPrimitiveValue*& aValue)
{
  nsROCSSPrimitiveValue* val=GetROCSSPrimitiveValue();
  NS_ENSURE_TRUE(val, NS_ERROR_OUT_OF_MEMORY);

  PRBool calcWidth = PR_FALSE;

  if (aFrame) {
    calcWidth = PR_TRUE;
    // Flush all pending notifications so that our frames are up to date
    nsCOMPtr<nsIDocument> document;
    mContent->GetDocument(*getter_AddRefs(document));
  
    if (document) {
      document->FlushPendingNotifications();
    }
    
    const nsStyleDisplay* displayData = nsnull;
    GetStyleData(eStyleStruct_Display, (const nsStyleStruct*&)displayData, aFrame);
    if (displayData && displayData->mDisplay == NS_STYLE_DISPLAY_INLINE) {
      nsFrameState frameState;
      aFrame->GetFrameState(&frameState);
      if (! (frameState & NS_FRAME_REPLACED_ELEMENT)) {
        calcWidth = PR_FALSE;
      }
    }
  }

  if (calcWidth) {
    nsRect rect;
    nsMargin padding;
    nsMargin border;
    aFrame->GetRect(rect);
    const nsStylePadding* paddingData=nsnull;
    GetStyleData(eStyleStruct_Padding, (const nsStyleStruct*&)paddingData, aFrame);
    if (paddingData) {
      paddingData->CalcPaddingFor(aFrame, padding);
    }
    const nsStyleBorder* borderData=nsnull;
    GetStyleData(eStyleStruct_Border, (const nsStyleStruct*&)borderData, aFrame);
    if (borderData) {
      borderData->CalcBorderFor(aFrame, border);
    }
    val->SetTwips(rect.width - padding.left - padding.right -
                  border.left - border.right);
  } else {
    // Just return the value in the style context
    const nsStylePosition* positionData = nsnull;
    GetStyleData(eStyleStruct_Position, (const nsStyleStruct*&)positionData, aFrame);
    if (positionData) {
      switch (positionData->mWidth.GetUnit()) {
        case eStyleUnit_Coord:
          val->SetTwips(positionData->mWidth.GetCoordValue());
          break;
        case eStyleUnit_Percent:
          val->SetPercent(positionData->mWidth.GetPercentValue());
          break;
        case eStyleUnit_Auto:
          val->SetString(NS_LITERAL_STRING("auto"));
          break;
        case eStyleUnit_Inherit:
          val->SetString(NS_LITERAL_STRING("inherit"));
          break;
        default:
          NS_WARNING("Double check the unit");
          val->SetTwips(0);
          break;
      }
    } else {
      val->SetTwips(0);
    }
  }

  return val->QueryInterface(NS_GET_IID(nsIDOMCSSPrimitiveValue),
                             (void **)&aValue);
}

#if 0

NS_IMETHODIMP
nsComputedDOMStyle::GetWordSpacing(nsAWritableString& aWordSpacing)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetZIndex(nsAWritableString& aZIndex)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsComputedDOMStyle::GetOpacity(nsAWritableString& aOpacity)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

#endif
