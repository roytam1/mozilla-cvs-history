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
#ifndef nsBaseAppCore_h___
#define nsBaseAppCore_h___

#include "nsAppCores.h"

//#include "nscore.h"
#include "nsString.h"
#include "nsISupports.h"
#include "nsIScriptObjectOwner.h"
#include "nsIDOMBaseAppCore.h"
#include "nsCOMPtr.h"

// Cannot forward declare a class used with an nsCOMPtr.
// see: http://www.mozilla.org/projects/xpcom/nsCOMPtr.html
// class nsIDOMNode;

#include "nsIDOMNode.h"

class nsIDOMDocument;
class nsIScriptContext;
class nsIDOMWindow;

////////////////////////////////////////////////////////////////////////////////
// nsBaseAppCore:
////////////////////////////////////////////////////////////////////////////////

class nsBaseAppCore : public nsIScriptObjectOwner, public nsIDOMBaseAppCore
{
  public:

    nsBaseAppCore();
    virtual ~nsBaseAppCore();
                 

    NS_DECL_ISUPPORTS
    //NS_IMETHOD    GetScriptObject(nsIScriptContext *aContext, void** aScriptObject);
    NS_IMETHOD    SetScriptObject(void* aScriptObject);

    NS_IMETHOD    Init(const nsString& aId);
    NS_IMETHOD    GetId(nsString& aId);
    NS_IMETHOD    SetDocumentCharset(const nsString& aCharset);

  protected:
    nsCOMPtr<nsIDOMNode>     FindNamedDOMNode(const nsString &aName, nsIDOMNode * aParent, PRInt32 & aCount, PRInt32 aEndCount);
    nsCOMPtr<nsIDOMNode>     GetParentNodeFromDOMDoc(nsIDOMDocument * aDOMDoc);
    nsIScriptContext *       GetScriptContext(nsIDOMWindow * aWin);

    nsString  mId;         /* User ID */
    void     *mScriptObject;
};

#endif // nsBaseAppCore_h___
