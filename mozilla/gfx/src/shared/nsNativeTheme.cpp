/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
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
 * Portions created by the Initial Developer are Copyright (C) 2002
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Brian Ryner <bryner@netscape.com>  (Original Author)
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "nsNativeTheme.h"
#include "nsIDocument.h"
#include "nsIContent.h"
#include "nsIFrame.h"
#include "nsIPresShell.h"
#include "nsIPresContext.h"
#include "nsIEventStateManager.h"
#include "nsString.h"
#include "nsINameSpaceManager.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsILookAndFeel.h"
#include "nsThemeConstants.h"
#include "nsIComponentManager.h"

nsMargin nsNativeTheme::sButtonBorderSize(2, 2, 2, 2);
nsMargin nsNativeTheme::sButtonDisabledBorderSize(1, 1, 1, 1);
nsMargin nsNativeTheme::sTextfieldBorderSize(2, 2, 2, 2);
PRBool   nsNativeTheme::sTextfieldBGTransparent = PR_FALSE;

nsNativeTheme::nsNativeTheme()
{
  mDisabledAtom = do_GetAtom("disabled");
  mCheckedAtom = do_GetAtom("checked");
  mSelectedAtom = do_GetAtom("selected");
  mFocusedAtom = do_GetAtom("focused");
  mFirstTabAtom = do_GetAtom("first-tab");
  mDefaultAtom = do_GetAtom("default");
  mValueAtom = do_GetAtom("value");
  mModeAtom = do_GetAtom("mode");
  mClassAtom = do_GetAtom("class");
  mSortDirectionAtom = do_GetAtom("sortDirection");
}

void
nsNativeTheme::GetPrimaryPresShell(nsIFrame* aFrame, nsIPresShell** aResult)
{
  *aResult = nsnull;
  if (!aFrame)
    return;

  nsCOMPtr<nsIDocument> doc;
  nsCOMPtr<nsIContent> content;
  aFrame->GetContent(getter_AddRefs(content));
  content->GetDocument(*getter_AddRefs(doc));
  if (doc)
    doc->GetShellAt(0, aResult); // addrefs
}

PRInt32
nsNativeTheme::GetContentState(nsIFrame* aFrame, PRUint8 aWidgetType)
{
  if (!aFrame)
    return 0;

  PRBool isXULCheckboxRadio = PR_FALSE;
  if (aWidgetType == NS_THEME_CHECKBOX || aWidgetType == NS_THEME_RADIO) {
    nsCOMPtr<nsIContent> checkboxRadioContent;
    aFrame->GetContent(getter_AddRefs(checkboxRadioContent));
    isXULCheckboxRadio = checkboxRadioContent->IsContentOfType(nsIContent::eXUL);
    if (isXULCheckboxRadio)
      aFrame->GetParent(&aFrame);
  }

  nsCOMPtr<nsIPresShell> shell;
  GetPrimaryPresShell(aFrame, getter_AddRefs(shell));
  if (!shell)
    return 0;

  nsCOMPtr<nsIPresContext> context;
  shell->GetPresContext(getter_AddRefs(context));
  nsCOMPtr<nsIEventStateManager> esm;
  context->GetEventStateManager(getter_AddRefs(esm));
  PRInt32 flags = 0;
  nsCOMPtr<nsIContent> content;
  aFrame->GetContent(getter_AddRefs(content));
  esm->GetContentState(content, flags);
  
  if (isXULCheckboxRadio && aWidgetType == NS_THEME_RADIO) {
    if (IsFocused(aFrame))
      flags |= NS_EVENT_STATE_FOCUS;
  }
  
  return flags;
}

PRBool
nsNativeTheme::CheckBooleanAttr(nsIFrame* aFrame, nsIAtom* aAtom)
{
  if (!aFrame)
    return PR_FALSE;

  nsCOMPtr<nsIContent> content;
  aFrame->GetContent(getter_AddRefs(content));
  nsAutoString attr;
  nsresult res = content->GetAttr(kNameSpaceID_None, aAtom, attr);

  // For HTML elements, boolean attributes will return NOT_THERE if they
  // are not present and HAS_VALUE + a string (possibly empty)
  // if they are present.

  // For XML/XUL elements, an attribute must be equal to the literal
  // string "true" to be counted as true.  An empty string should _not_
  // be counted as true.

  PRBool isHTML = content->IsContentOfType(nsIContent::eHTML);
  if (isHTML && (res == NS_CONTENT_ATTR_NO_VALUE ||
                 res != NS_CONTENT_ATTR_NOT_THERE && attr.IsEmpty()))
    return PR_TRUE;

  return attr.EqualsIgnoreCase("true");
}

PRInt32
nsNativeTheme::CheckIntAttr(nsIFrame* aFrame, nsIAtom* aAtom)
{
  if (!aFrame)
    return 0;

  nsCOMPtr<nsIContent> content;
  aFrame->GetContent(getter_AddRefs(content));
  nsAutoString attr;
  content->GetAttr(kNameSpaceID_None, aAtom, attr);
  PRInt32 err, value = attr.ToInteger(&err);
  if (NS_FAILED(err))
    return 0;

  return value;
}

PRBool
nsNativeTheme::GetAttr(nsIFrame* aFrame, nsIAtom* aAtom, nsAString& attrValue)
{
  if (!aFrame)
    return PR_FALSE;

  nsCOMPtr<nsIContent> content;
  aFrame->GetContent(getter_AddRefs(content));
  nsresult res = content->GetAttr(kNameSpaceID_None, aAtom, attrValue);
  return ((res != NS_CONTENT_ATTR_NOT_THERE) &&
	  !(res != NS_CONTENT_ATTR_NO_VALUE && attrValue.IsEmpty()));
}

PRBool
nsNativeTheme::GetCheckedOrSelected(nsIFrame* aFrame, PRBool aCheckSelected)
{
  if (!aFrame)
    return PR_FALSE;

  nsCOMPtr<nsIContent> content;
  aFrame->GetContent(getter_AddRefs(content));

  if (content->IsContentOfType(nsIContent::eXUL)) {
    // For a XUL checkbox or radio button, the state of the parent determines
    // the checked state
    aFrame->GetParent(&aFrame);
  } else {
    // Check for an HTML input element
    nsCOMPtr<nsIDOMHTMLInputElement> inputElt = do_QueryInterface(content);
    if (inputElt) {
      PRBool checked;
      inputElt->GetChecked(&checked);
      return checked;
    }
  }

  return CheckBooleanAttr(aFrame, aCheckSelected ? mSelectedAtom : mCheckedAtom);
}

static void
ConvertMarginToTwips(const nsMargin &aSource, nsMargin &aDest, float p2t)
{
  aDest.top = NSIntPixelsToTwips(aSource.top, p2t);
  aDest.left = NSIntPixelsToTwips(aSource.left, p2t);
  aDest.bottom = NSIntPixelsToTwips(aSource.bottom, p2t);
  aDest.right = NSIntPixelsToTwips(aSource.right, p2t);
}

PRBool
nsNativeTheme::IsWidgetStyled(nsIPresContext* aPresContext, nsIFrame* aFrame,
                              PRUint8 aWidgetType)
{
  // Check for specific widgets to see if HTML has overridden the style.
  if (aFrame && (aWidgetType == NS_THEME_BUTTON ||
                 aWidgetType == NS_THEME_TEXTFIELD)) {

    nsCOMPtr<nsIContent> content;
    aFrame->GetContent(getter_AddRefs(content));
    if (content->IsContentOfType(nsIContent::eHTML)) {

      nscolor defaultBGColor, defaultBorderColor;
      PRUint8 defaultBorderStyle;
      nsMargin defaultBorderSize;
      PRBool defaultBGTransparent = PR_FALSE;

      float p2t;
      aPresContext->GetPixelsToTwips(&p2t);

      nsCOMPtr<nsILookAndFeel> lookAndFeel;
      aPresContext->GetLookAndFeel(getter_AddRefs(lookAndFeel));
      if (!lookAndFeel)
        return PR_TRUE;

      switch (aWidgetType) {
      case NS_THEME_BUTTON:
        if (IsDisabled(aFrame)) {
          ConvertMarginToTwips(sButtonDisabledBorderSize, defaultBorderSize, p2t);
          defaultBorderStyle = NS_STYLE_BORDER_STYLE_OUTSET;
          lookAndFeel->GetColor(nsILookAndFeel::eColor_threedshadow,
                                defaultBorderColor);
          lookAndFeel->GetColor(nsILookAndFeel::eColor_threedface,
                                defaultBGColor);
        } else {
          PRInt32 contentState = GetContentState(aFrame, aWidgetType);
          ConvertMarginToTwips(sButtonBorderSize, defaultBorderSize, p2t);
          if (contentState & NS_EVENT_STATE_HOVER &&
              contentState & NS_EVENT_STATE_ACTIVE)
            defaultBorderStyle = NS_STYLE_BORDER_STYLE_INSET;
          else
            defaultBorderStyle = NS_STYLE_BORDER_STYLE_OUTSET;
          lookAndFeel->GetColor(nsILookAndFeel::eColor_threedface,
                                defaultBorderColor);
          defaultBGColor = defaultBorderColor;
        }
        break;

      case NS_THEME_TEXTFIELD:
        defaultBorderStyle = NS_STYLE_BORDER_STYLE_INSET;
        ConvertMarginToTwips(sTextfieldBorderSize, defaultBorderSize, p2t);
        lookAndFeel->GetColor(nsILookAndFeel::eColor_threedface,
                              defaultBorderColor);
        if (!(defaultBGTransparent = sTextfieldBGTransparent)) {
          if (IsDisabled(aFrame))
            defaultBGColor = defaultBorderColor;
          else
            lookAndFeel->GetColor(nsILookAndFeel::eColor__moz_field,
                                  defaultBGColor);
        }
        break;

      default:
        NS_ERROR("nsNativeTheme::IsWidgetStyled widget type not handled");
        return PR_FALSE;
      }

      // Check whether background differs from default
      const nsStyleBackground* ourBG = aFrame->GetStyleBackground();

      if (defaultBGTransparent) {
        if (!(ourBG->mBackgroundFlags & NS_STYLE_BG_COLOR_TRANSPARENT))
          return PR_TRUE;
      } else if (ourBG->mBackgroundColor != defaultBGColor ||
                 ourBG->mBackgroundFlags & NS_STYLE_BG_COLOR_TRANSPARENT)
        return PR_TRUE;

      if (!(ourBG->mBackgroundFlags & NS_STYLE_BG_IMAGE_NONE))
        return PR_TRUE;

      // Check whether border style or color differs from default
      const nsStyleBorder* ourBorder = aFrame->GetStyleBorder();

      for (PRInt32 i = 0; i < 4; ++i) {
        if (ourBorder->GetBorderStyle(i) != defaultBorderStyle)
          return PR_TRUE;

        PRBool borderFG, borderClear;
        nscolor borderColor;
        ourBorder->GetBorderColor(i, borderColor, borderFG, borderClear);
        if (borderColor != defaultBorderColor || borderClear)
          return PR_TRUE;
      }

      // Check whether border size differs from default
      nsMargin borderSize;
      if (ourBorder->GetBorder(borderSize) && borderSize != defaultBorderSize)
        return PR_TRUE;
    }
  }

  return PR_FALSE;
}
