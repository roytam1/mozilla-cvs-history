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



#include "nsMarkupDocument.h"
#include "nsIContent.h"
#include "nsIURL.h"
#include "nsIDOMElement.h"

#include "nsCSSPropIDs.h"
#include "nsCSSProps.h"
#include "nsICSSStyleSheet.h"
#include "nsICSSStyleRule.h"
#include "nsICSSDeclaration.h"
#include "nsIHTMLCSSStyleSheet.h"
#include "nsHTMLValue.h"
#include "nsXIFConverter.h"

static NS_DEFINE_IID(kIContentIID, NS_ICONTENT_IID);
static NS_DEFINE_IID(kICSSStyleSheetIID, NS_ICSS_STYLE_SHEET_IID);



nsMarkupDocument::nsMarkupDocument() : nsDocument()
{
}

nsMarkupDocument::~nsMarkupDocument()
{
}

/**
 *  Converts a CSS selector to XIF
 *  
 *  @update  gpk 01/17/1998
 *  @param   aConverter -- the XIFConverter where all output is being written
 *  @param   aSelector -- the Object to be converted to XIF
 */
void nsMarkupDocument::CSSSelectorToXIF(nsXIFConverter& aConverter, nsCSSSelector& aSelector)
{
  nsString s;

  nsCSSSelector* next = aSelector.mNext;

  if (nsnull != next)
    CSSSelectorToXIF(aConverter,*next);

  aConverter.BeginCSSSelector();
 
  if (aSelector.mTag != nsnull)
  {
    aSelector.mTag->ToString(s);
    aConverter.AddCSSTag(s);
  }

  if (aSelector.mID != nsnull)
  {
    aSelector.mID->ToString(s);
    aConverter.AddCSSID(s);
  }
  
  if (aSelector.mClass != nsnull)
  {
    aSelector.mClass->ToString(s);
    aConverter.AddCSSClass(s);
  }
  
  if (aSelector.mPseudoClass != nsnull)
  {
    aSelector.mPseudoClass->ToString(s);
    aConverter.AddCSSPsuedoClass(s);
  }
  aConverter.EndCSSSelector();

}


/**
 *  Converts a CSS Declaration to XIF
 *  
 *  @update  gpk 01/17/1998
 *  @param   aConverter -- the XIFConverter where all output is being written
 *  @param   aDeclaration -- the Object to be converted to XIF
 */
void nsMarkupDocument::CSSDeclarationToXIF(nsXIFConverter& aConverter, nsICSSDeclaration& aDeclaration)
{
  nsAutoString  list;
  nsAutoString  decl;

  aConverter.BeginCSSDeclarationList();
  aDeclaration.ToString(list);

  PRInt32 start = 0;
  PRInt32 semiColon = list.Find(';');

  while (-1 < semiColon) {
    decl.Truncate();
    list.Mid(decl, start, semiColon - start);

    if (0 == decl.Compare("/*", PR_FALSE, 2)) {
      // XXX need to append comment
    }
    else {
      PRInt32 colon = decl.Find(':');
      nsAutoString  property;
      nsAutoString  value;

      aConverter.BeginCSSDeclaration();
      if (-1 < colon) {
        decl.Left(property, colon);
        property.StripWhitespace();
        decl.Right(value, (decl.Length() - colon) - 2);
        aConverter.AddCSSDeclaration(property, value);
      }

      aConverter.EndCSSDeclaration();
    }

    start = ++semiColon;
    semiColon = list.Find(';', start);
  }
  aConverter.EndCSSDeclarationList();
}


/**
 *  Converts the CSS Stylesheets in this document to XIF
 *  
 *  @update  gpk 01/17/1998
 *  @param   aConverter -- the XIFConverter where all output is being written
 *  @param   aDeclaration -- the Object to be converted to XIF
 */
void nsMarkupDocument::StyleSheetsToXIF(nsXIFConverter& aConverter)
{
 
  PRInt32     count = GetNumberOfStyleSheets();
  nsIURL&      docURL = *mDocumentURL;

  for (PRInt32 index = 0; index < count; index++)
  {
    nsIStyleSheet*          sheet = GetStyleSheetAt(index);
    nsICSSStyleSheet*       cssSheet = nsnull;
    
    if (sheet != nsnull)
    {
      nsIURL& sheetURL = *sheet->GetURL();
      
      if (!(sheetURL == docURL))
        break;
      
      nsresult  isCss = sheet->QueryInterface(kICSSStyleSheetIID, (void**)&cssSheet);
      if ((isCss == NS_OK) && (cssSheet != nsnull))
      {
        PRInt32           ruleCount = cssSheet->StyleRuleCount();
        PRInt32           ruleIndex;
        nsICSSStyleRule*  rule = nsnull;

        aConverter.BeginCSSStyleSheet();
        for (ruleIndex = 0; ruleIndex < ruleCount; ruleIndex++)
        {
          if (NS_OK == cssSheet->GetStyleRuleAt(ruleIndex, rule))
          {
            aConverter.BeginCSSRule();

              if (nsnull != rule)
              {
                nsCSSSelector* selector = rule->FirstSelector();
          
                if (nsnull != selector)
                  CSSSelectorToXIF(aConverter,*selector);
  
                nsICSSDeclaration* declaration = rule->GetDeclaration();
                if (nsnull != declaration)
                  CSSDeclarationToXIF(aConverter,*declaration);

                NS_IF_RELEASE(declaration);
                NS_IF_RELEASE(rule);
              } // ruleAt

            aConverter.EndCSSRule();
          } // for loop
        }
        aConverter.EndCSSStyleSheet();
        NS_RELEASE(cssSheet);
      } // css_sheet
      NS_RELEASE(sheet);
    } // sheet
    }
}


void nsMarkupDocument::FinishConvertToXIF(nsXIFConverter& aConverter, nsIDOMNode* aNode)
{
  nsIContent* content = nsnull;
  nsresult    isContent = aNode->QueryInterface(kIContentIID, (void**)&content);
  PRBool      isSynthetic = PR_TRUE;

  if (NS_OK == isContent)
  {
    content->IsSynthetic(isSynthetic);
    if (PR_FALSE == isSynthetic)
    {
      nsIAtom* tag;
      content->GetTag(tag);
      if (tag != nsnull)
      {
        nsString str;
        tag->ToString(str);
        if (str.EqualsIgnoreCase("Head"))
          StyleSheetsToXIF(aConverter);
        NS_RELEASE(tag);
      }
    }
  }
  nsDocument::FinishConvertToXIF(aConverter,aNode);
}

void nsMarkupDocument::ToXIF(nsXIFConverter& aConverter, nsIDOMNode* aNode)
{
  if (aConverter.GetUseSelection() == PR_TRUE)
  {
    nsIContent* content = nsnull;
    nsresult    isContent = aNode->QueryInterface(kIContentIID, (void**)&content);

    if (isContent == NS_OK)
    {
      PRBool  isInSelection = IsInSelection(content);
      
      if (isInSelection == PR_TRUE)
      {
        BeginConvertToXIF(aConverter,aNode);
        ConvertChildrenToXIF(aConverter,aNode);
        FinishConvertToXIF(aConverter,aNode);
      }
      else
      {
        ConvertChildrenToXIF(aConverter,aNode);
      }
      NS_RELEASE(content);
    }
  }
  else
  {
    BeginConvertToXIF(aConverter,aNode);
    ConvertChildrenToXIF(aConverter,aNode);
    FinishConvertToXIF(aConverter,aNode);
  }
}

void nsMarkupDocument::CreateXIF(nsString & aBuffer, PRBool aUseSelection)
{
  
  nsXIFConverter  converter(aBuffer);
  // call the function

  converter.SetUseSelection(aUseSelection);

  converter.AddStartTag("section");
  
  converter.AddStartTag("section_head");
  converter.AddEndTag("section_head");

  converter.AddStartTag("section_body");

  nsIDOMElement* root = nsnull;
  if (NS_OK == GetDocumentElement(&root)) 
  {  
    ToXIF(converter,root);
    NS_RELEASE(root);
  }
  converter.AddEndTag("section_body");

  converter.AddEndTag("section");

  converter.Write();
  
}
