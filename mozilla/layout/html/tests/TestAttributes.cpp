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
 */
#include <stdio.h>
#include "nscore.h"
#include "nsIAtom.h"
#include "nsCRT.h"
#include "nsHTMLParts.h"
#include "nsIHTMLContent.h"
#include "nsIHTMLAttributes.h"
#include "nsITextContent.h"
#include "nsString.h"
#include "nsIDocument.h"
#include "nsISupportsArray.h"
#include "nsDocument.h"
#include "nsMarkupDocument.h"
#include "nsIURL.h"
#include "nsIDOMText.h"
#include "nsINameSpaceManager.h"

void testAttributes(nsIHTMLContent* content) {
  nsHTMLValue nullValue;

  nsIAtom* sBORDER = NS_NewAtom("border");
  nsIAtom* sWIDTH = NS_NewAtom("width");
  nsIAtom* sHEIGHT = NS_NewAtom("height");
  nsIAtom* sSRC = NS_NewAtom("src");
  nsIAtom* sBAD = NS_NewAtom("badattribute");
  nsString sempty;
  nsString sfoo_gif; sfoo_gif.AssignWithConversion("foo.gif");

  content->SetHTMLAttribute(sBORDER, nullValue, PR_FALSE);
  content->SetHTMLAttribute(sWIDTH, nsHTMLValue(5, eHTMLUnit_Pixel), PR_FALSE);
  content->SetAttribute(kNameSpaceID_HTML, sHEIGHT, sempty, PR_FALSE);
  content->SetAttribute(kNameSpaceID_HTML, sSRC, sfoo_gif, PR_FALSE);

  nsHTMLValue ret;
  nsresult rv;
  rv = content->GetHTMLAttribute(sBORDER, ret);
  if ((rv != NS_CONTENT_ATTR_NO_VALUE) || (ret.GetUnit() != eHTMLUnit_Null)) {
    printf("test 0 failed\n");
  }

  rv = content->GetHTMLAttribute(sWIDTH, ret);
  if ((rv != NS_CONTENT_ATTR_HAS_VALUE) || (! (ret == nsHTMLValue(5, eHTMLUnit_Pixel)))) {
    printf("test 1 failed\n");
  }

  rv = content->GetHTMLAttribute(sBAD, ret);
  if (rv != NS_CONTENT_ATTR_NOT_THERE) {
    printf("test 2 failed\n");
  }

  content->UnsetAttribute(kNameSpaceID_HTML, sWIDTH, PR_FALSE);

  nsISupportsArray* allNames;
  NS_NewISupportsArray(&allNames);

  PRInt32 na;
  content->GetAttributeCount(na);
  if (na != 3) {
    printf("test 5 (unset attriubte) failed\n");
  }
  PRInt32 index;
  for (index = 0; index < na; index++) {
    nsIAtom* name;
    PRInt32 nameSpaceID;
    content->GetAttributeNameAt(index, nameSpaceID, name);
    allNames->AppendElement(name);
    NS_RELEASE(name);
  }

  PRBool borderFound = PR_FALSE,heightFound = PR_FALSE,srcFound = PR_FALSE;
  for (int n = 0; n < 3; n++) {
    const nsIAtom* ident = (const nsIAtom*)allNames->ElementAt(n);
    if (sBORDER == ident) {
      borderFound = PR_TRUE;
    }
    if (sHEIGHT == ident) {
      heightFound = PR_TRUE;
    }
    if (sSRC == ident) {
      srcFound = PR_TRUE;
    }
  }
  if (!(borderFound && heightFound && srcFound)) {
    printf("test 6 failed\n");
  }

  NS_RELEASE(allNames);

  NS_RELEASE(sBORDER);
  NS_RELEASE(sWIDTH);
  NS_RELEASE(sHEIGHT);
  NS_RELEASE(sSRC);
}

void testStrings(nsIDocument* aDoc) {
  printf("begin string tests\n");

  PRBool val;
  // regular Equals
  val = (NS_ConvertASCIItoUCS2("mrString")).EqualsWithConversion("mrString");
  if (PR_TRUE != val) {
    printf("test 0 failed\n");
  }
  val = (NS_ConvertASCIItoUCS2("mrString")).EqualsWithConversion("MRString");
  if (PR_FALSE != val) {
    printf("test 1 failed\n");
  }
  val = (NS_ConvertASCIItoUCS2("mrString")).EqualsWithConversion("mrStri");
  if (PR_FALSE != val) {
    printf("test 2 failed\n");
  }
  val = (NS_ConvertASCIItoUCS2("mrStri")).EqualsWithConversion("mrString");
  if (PR_FALSE != val) {
    printf("test 3 failed\n");
  }
  // EqualsIgnoreCase
  val = (NS_ConvertASCIItoUCS2("mrString")).EqualsIgnoreCase("mrString");
  if (PR_TRUE != val) {
    printf("test 4 failed\n");
  }
  val = (NS_ConvertASCIItoUCS2("mrString")).EqualsIgnoreCase("mrStrinG");
  if (PR_TRUE != val) {
    printf("test 5 failed\n");
  }
  val = (NS_ConvertASCIItoUCS2("mrString")).EqualsIgnoreCase("mrStri");
  if (PR_FALSE != val) {
    printf("test 6 failed\n");
  }
  val = (NS_ConvertASCIItoUCS2("mrStri")).EqualsIgnoreCase("mrString");
  if (PR_FALSE != val) {
    printf("test 7 failed\n");
  }
  // String vs. Ident
  val = (NS_ConvertASCIItoUCS2("mrString")).EqualsIgnoreCase(NS_NewAtom("mrString"));
  if (PR_TRUE != val) {
    printf("test 8 failed\n");
  }
  val = (NS_ConvertASCIItoUCS2("mrString")).EqualsIgnoreCase(NS_NewAtom("MRStrINg"));
  if (PR_TRUE != val) {
    printf("test 9 failed\n");
  }
  val = (NS_ConvertASCIItoUCS2("mrString")).EqualsIgnoreCase(NS_NewAtom("mrStri"));
  if (PR_FALSE != val) {
    printf("test 10 failed\n");
  }
  val = (NS_ConvertASCIItoUCS2("mrStri")).EqualsIgnoreCase(NS_NewAtom("mrString"));
  if (PR_FALSE != val) {
    printf("test 11 failed\n");
  }

  printf("string tests complete\n");
}

class MyDocument : public nsMarkupDocument {
public:
  MyDocument();
  NS_IMETHOD StartDocumentLoad(const char* aCommand,
                               nsIChannel* aChannel,
                               nsILoadGroup* aLoadGroup,
                               nsISupports* aContainer,
                               nsIStreamListener **aDocListener)
  {
    return NS_OK;
  }

  NS_IMETHOD    ImportNode(nsIDOMNode* aImportedNode, PRBool aDeep, nsIDOMNode** aReturn) {
    return NS_OK;
  }

  NS_IMETHOD    CreateElementNS(const nsString& aNamespaceURI, const nsString& aQualifiedName, nsIDOMElement** aReturn) {
    return NS_OK;
  }

  NS_IMETHOD    CreateAttributeNS(const nsString& aNamespaceURI, const nsString& aQualifiedName, nsIDOMAttr** aReturn) {
    return NS_OK;
  }

  NS_IMETHOD    GetElementsByTagNameNS(const nsString& aNamespaceURI, const nsString& aLocalName, nsIDOMNodeList** aReturn) {
    return NS_OK;
  }

  NS_IMETHOD    GetElementById(const nsString& aElementId, nsIDOMElement** aReturn) {
    return NS_OK;
  }

protected:
  virtual ~MyDocument();
};

MyDocument::MyDocument()
{
}

MyDocument::~MyDocument()
{
}

int main(int argc, char** argv)
{
  /* Create Byte2Unicode converter? Not anymore. The converters are not tested
  here, they have their own test code. And if you just want to use them, you 
  need a properly intialized xpcom system. This simple test program doesn't do
  that. */

  // Create a unicode string
  static const char* srcStr = "This is some meaningless text about nothing at all";
  nsresult rv;
  PRUint32 origSrcLen = nsCRT::strlen((char *)srcStr);
  const int BUFFER_LENGTH = 100;
  PRUnichar destStr[BUFFER_LENGTH];
  PRUint32 srcLen = origSrcLen;
  PRUint32 destLen = BUFFER_LENGTH;
  // hacky Ascii conversion to unicode, because we don't have a real Converter.
  for (PRUint32 i=0; i<srcLen; i++) destStr[i] = ((PRUint8)srcStr[i]);

  // Create test document.
  MyDocument *myDoc = new MyDocument();

  testStrings(myDoc);

  // Create a new text content object.
  nsIContent *text;
  rv = NS_NewTextNode(&text);
  if (NS_OK != rv) {
    printf("Could not create text content.\n");
    return -1;
  }

  nsIDOMText* txt = nsnull;
  static NS_DEFINE_IID(kIDOMTextIID, NS_IDOMTEXT_IID);
  text->QueryInterface(kIDOMTextIID, (void**) &txt);
  nsAutoString tmp(destStr);
  txt->AppendData(tmp);
  NS_RELEASE(txt);

  PRBool canHaveKids;
  text->CanContainChildren(canHaveKids);
  NS_ASSERTION(!canHaveKids,"");
  text->SetDocument(myDoc, PR_FALSE);

#if 0
  // Query ITextContent interface
  static NS_DEFINE_IID(kITextContentIID, NS_ITEXTCONTENT_IID);
  nsITextContent* textContent;
  rv = text->QueryInterface(kITextContentIID,(void **)&textContent);
  if (NS_OK != rv) {
    printf("Created text content does not have the ITextContent interface.\n");
    return -1;
  }

  // Print the contents.
  nsAutoString stringBuf;
  textContent->GetText(stringBuf,0,textContent->GetLength());
  if (!stringBuf.Equals(nsString(destStr,destLen))) {
    printf("something wrong with the text in a text content\n");
  }
#endif

  // Create a simple container.
  nsIHTMLContent* container;
  nsIAtom* li = NS_NewAtom("li");

  rv = NS_NewHTMLLIElement(&container,li);
  if (NS_OK != rv) {
    printf("Could not create container.\n");
    return -1;
  }
  container->CanContainChildren(canHaveKids);
  NS_ASSERTION(canHaveKids,"");
  container->SetDocument(myDoc, PR_FALSE);

  container->AppendChildTo(text, PR_FALSE);
  PRInt32 nk;
  container->ChildCount(nk);
  if (nk != 1) {
    printf("Container has wrong number of children.");
  }

  printf("begin attribute tests\n");
  testAttributes(container);
  printf("attribute tests complete\n");


  // Clean up memory.
  text->Release(); // The textContent interface.
  delete container;
  delete text;
  myDoc->Release();
  return 0;
}
