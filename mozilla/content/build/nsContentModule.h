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
#ifndef nsContentModule_h___
#define nsContentModule_h___

#include "nslayout.h"
#include "nsIModule.h"

class nsICSSStyleSheet;
class nsIScriptNameSetRegistry;

// Module implementation for the content library
class nsContentModule : public nsIModule
{
public:
  nsContentModule();
  virtual ~nsContentModule();

  NS_DECL_ISUPPORTS

  NS_DECL_NSIMODULE

  nsresult Initialize();

protected:
  void Shutdown();

  nsresult RegisterDocumentFactories(nsIComponentManager* aCompMgr,
                                     nsIFile* aPath);

  void UnregisterDocumentFactories(nsIComponentManager* aCompMgr,
                                   nsIFile* aPath);

  void SetUserAgent();

  PRBool mInitialized;
//  static nsIFactory* gFactory;
  static nsIScriptNameSetRegistry* gRegistry;

public:
  static nsICSSStyleSheet* gUAStyleSheet;
};

//----------------------------------------------------------------------

class nsContentFactory : public nsIFactory
{   
public:   
  nsContentFactory(const nsCID &aClass);   

  NS_DECL_ISUPPORTS

  NS_DECL_NSIFACTORY

protected:
  virtual ~nsContentFactory();   

  nsCID     mClassID;
};   

#endif /* nsContentModule_h___ */
