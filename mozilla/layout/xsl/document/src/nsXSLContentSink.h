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

#ifndef nsXSLContentSink_h__
#define nsXSLContentSink_h__

#include "nsXMLContentSink.h"

// The XSL content sink accepts a parsed XML document
// from the parser and creates a XSL rule data model
// and an XSL stylesheet content model.

class nsXSLContentSink : public nsXMLContentSink {
public:
  nsXSLContentSink();
  ~nsXSLContentSink();

  nsresult Init(nsITransformMediator* aTM,
                nsIDocument* aDoc,
                nsIURI* aURL,
                nsIWebShell* aContainer);

  // nsISupports
  //NS_DECL_ISUPPORTS

  // nsIContentSink
  NS_IMETHOD WillBuildModel(void);
  NS_IMETHOD DidBuildModel(PRInt32 aQualityLevel);
  //NS_IMETHOD WillInterrupt(void);
  //NS_IMETHOD WillResume(void);
  //NS_IMETHOD SetParser(nsIParser* aParser);  
  NS_IMETHOD OpenContainer(const nsIParserNode& aNode);
  NS_IMETHOD CloseContainer(const nsIParserNode& aNode);
  NS_IMETHOD AddLeaf(const nsIParserNode& aNode);
  NS_IMETHOD AddComment(const nsIParserNode& aNode);
  NS_IMETHOD AddProcessingInstruction(const nsIParserNode& aNode);
  NS_IMETHOD NotifyError(const nsParserError* aError);
  //NS_IMETHOD AddDocTypeDecl(const nsIParserNode& aNode, PRInt32 aMode=0);

  // nsIXMLContentSink
  //NS_IMETHOD AddXMLDecl(const nsIParserNode& aNode);  
  //NS_IMETHOD AddCharacterData(const nsIParserNode& aNode);
  //NS_IMETHOD AddUnparsedEntity(const nsIParserNode& aNode);
  //NS_IMETHOD AddNotation(const nsIParserNode& aNode);
  //NS_IMETHOD AddEntityReference(const nsIParserNode& aNode);

protected:

};


nsresult
NS_NewXSLContentSink(nsIXMLContentSink** aResult,
                     nsITransformMediator* aTM,
                     nsIDocument* aDoc,
                     nsIURI* aURL,
                     nsIWebShell* aWebShell);
#endif // nsXSLContentSink_h__
