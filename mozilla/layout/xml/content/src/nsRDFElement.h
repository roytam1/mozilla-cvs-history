/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

#ifndef nsRDFElement_h___
#define nsRDFElement_h___

#include "nsIDOMElement.h"
#include "nsIScriptObjectOwner.h"
#include "nsIDOMEventReceiver.h"
#include "nsIRDFContent.h"
#include "nsIJSScriptObject.h"
#include "nsGenericElement.h"

class nsIContent;
class nsIDocument;
class nsIAtom;
class nsIEventListenerManager;
class nsIHTMLAttributes;
class nsIRDFNode;

// XXX should we make this inheirit from the nsXMLElement
// implementation???

class nsRDFElement : public nsIDOMElement,
		     public nsIDOMEventReceiver,
		     public nsIScriptObjectOwner,
		     public nsIJSScriptObject,
		     public nsIRDFContent
{
public:
    nsRDFElement(void);
    ~nsRDFElement(void);

    // nsISupports
    NS_DECL_ISUPPORTS
       
    // nsIDOMNode (from nsIDOMElement)
    NS_DECL_IDOMNODE
  
    // nsIDOMElement
    NS_DECL_IDOMELEMENT

    // nsIScriptObjectOwner
    NS_IMETHOD GetScriptObject(nsIScriptContext* aContext, void** aScriptObject);
    NS_IMETHOD SetScriptObject(void *aScriptObject);

    // nsIContent (from nsIRDFContent via nsIXMLContent)
    NS_IMETHOD GetDocument(nsIDocument*& aResult) const;
    NS_IMETHOD SetDocument(nsIDocument* aDocument, PRBool aDeep);
    NS_IMETHOD GetParent(nsIContent*& aResult) const;
    NS_IMETHOD SetParent(nsIContent* aParent);
    NS_IMETHOD CanContainChildren(PRBool& aResult) const;
    NS_IMETHOD ChildCount(PRInt32& aResult) const;
    NS_IMETHOD ChildAt(PRInt32 aIndex, nsIContent*& aResult) const;
    NS_IMETHOD IndexOf(nsIContent* aPossibleChild, PRInt32& aResult) const;
    NS_IMETHOD InsertChildAt(nsIContent* aKid, PRInt32 aIndex, PRBool aNotify);
    NS_IMETHOD ReplaceChildAt(nsIContent* aKid, PRInt32 aIndex, PRBool aNotify);
    NS_IMETHOD AppendChildTo(nsIContent* aKid, PRBool aNotify);
    NS_IMETHOD RemoveChildAt(PRInt32 aIndex, PRBool aNotify);
    NS_IMETHOD IsSynthetic(PRBool& aResult);
    NS_IMETHOD GetTag(nsIAtom*& aResult) const;
    NS_IMETHOD SetAttribute(const nsString& aName, const nsString& aValue, PRBool aNotify);
    NS_IMETHOD GetAttribute(const nsString& aName, nsString& aResult) const;
    NS_IMETHOD UnsetAttribute(nsIAtom* aAttribute, PRBool aNotify);
    NS_IMETHOD GetAllAttributeNames(nsISupportsArray* aArray, PRInt32& aResult) const;
    NS_IMETHOD GetAttributeCount(PRInt32& aResult) const;
    NS_IMETHOD List(FILE* out, PRInt32 aIndent) const;
    NS_IMETHOD BeginConvertToXIF(nsXIFConverter& aConverter) const;
    NS_IMETHOD ConvertContentToXIF(nsXIFConverter& aConverter) const;
    NS_IMETHOD FinishConvertToXIF(nsXIFConverter& aConverter) const;
    NS_IMETHOD SizeOf(nsISizeOfHandler* aHandler) const;
    NS_IMETHOD HandleDOMEvent(nsIPresContext& aPresContext,
                              nsEvent* aEvent,
                              nsIDOMEvent** aDOMEvent,
                              PRUint32 aFlags,
                              nsEventStatus& aEventStatus);

    // nsIXMLContent (from nsIRDFContent)
    NS_IMETHOD SetNameSpace(nsIAtom* aNameSpace);
    NS_IMETHOD GetNameSpace(nsIAtom*& aNameSpace);
    NS_IMETHOD SetNameSpaceIdentifier(PRInt32 aNameSpaceId);
    NS_IMETHOD GetNameSpaceIdentifier(PRInt32& aNameSpeceId);

    // nsIRDFContent
    NS_IMETHOD SetResource(const nsString& aURI);
    NS_IMETHOD GetResource(nsString& rURI) const;

    NS_IMETHOD SetProperty(const nsString& aPropertyURI, const nsString& aValue);
    NS_IMETHOD GetProperty(const nsString& aPropertyURI, nsString& rValue) const;

    // nsIDOMEventReceiver
    NS_IMETHOD AddEventListener(nsIDOMEventListener *aListener, const nsIID& aIID);
    NS_IMETHOD RemoveEventListener(nsIDOMEventListener *aListener, const nsIID& aIID);
    NS_IMETHOD GetListenerManager(nsIEventListenerManager** aInstancePtrResult);
    NS_IMETHOD GetNewListenerManager(nsIEventListenerManager **aInstancePtrResult);


    // nsIJSScriptObject
    virtual PRBool AddProperty(JSContext *aContext, jsval aID, jsval *aVp);
    virtual PRBool DeleteProperty(JSContext *aContext, jsval aID, jsval *aVp);
    virtual PRBool GetProperty(JSContext *aContext, jsval aID, jsval *aVp);
    virtual PRBool SetProperty(JSContext *aContext, jsval aID, jsval *aVp);
    virtual PRBool EnumerateProperty(JSContext *aContext);
    virtual PRBool Resolve(JSContext *aContext, jsval aID);
    virtual PRBool Convert(JSContext *aContext, jsval aID);
    virtual void   Finalize(JSContext *aContext);

protected:
    nsIDocument* mDocument;
    nsIAtom*     mNameSpace;
    PRInt32      mNameSpaceId;
    void*        mScriptObject;
    nsIRDFNode*  mResource;
};

#endif // nsRDFElement_h___
