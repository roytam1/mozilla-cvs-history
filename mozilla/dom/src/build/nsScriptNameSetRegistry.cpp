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


#include "nsScriptNameSetRegistry.h"
#include "nsIScriptExternalNameSet.h"

nsScriptNameSetRegistry::nsScriptNameSetRegistry()
{
}

nsScriptNameSetRegistry::~nsScriptNameSetRegistry()
{
  PRInt32 i, count = mNameSets.Count();

  for(i = 0; i < count; i++) {
    nsIScriptExternalNameSet* ns =
      (nsIScriptExternalNameSet*)mNameSets.ElementAt(i);
    
    NS_RELEASE(ns);
  }
  mNameSets.Clear();
}


#if 0
nsScriptNameSetRegistry::Init()
{

}
#endif


NS_IMETHODIMP 
nsScriptNameSetRegistry::InitializeClasses(nsIScriptContext* aContext)
{
  nsresult result = NS_OK;
  if (nsnull != aContext) {
    PRInt32 i, count = mNameSets.Count();

    for (i = 0; i < count; i++) {
      nsIScriptExternalNameSet* ns = (nsIScriptExternalNameSet*)mNameSets.ElementAt(i);
      if (nsnull != ns) {
        result = ns->InitializeClasses(aContext);
        if (NS_OK != result) {
          break;
        }
      }
    }
  }
  return result;
}

#if 0
NS_IMETHODIMP 
nsScriptNameSetRegistry::PopulateNameSpace(nsIScriptContext* aContext)
{
  nsresult result = NS_OK;
  if (nsnull != aContext) {
    PRInt32 i, count = mNameSets.Count();

    for (i = 0; i < count; i++) {
      nsIScriptExternalNameSet* ns = (nsIScriptExternalNameSet*)mNameSets.ElementAt(i);
      if (nsnull != ns) {
        result = ns->AddNameSet(aContext);
        if (NS_OK != result) {
          break;
        }
      }
    }
  }
  return result;
}
#endif
