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

#include "nsContentList.h"
#include "nsIContent.h"
#include "nsIDOMNode.h"
#include "nsIScriptGlobalObject.h"
#include "nsIDocument.h"


nsContentList::nsContentList(nsIDocument *aDocument,
			     const nsString& aMatchTag) 
{
  mMatchTag = new nsString(aMatchTag);
  mFunc = nsnull;
  Init(aDocument);
}

nsContentList::nsContentList(nsIDocument *aDocument, 
			     nsContentListMatchFunc aFunc)
{
  mFunc = aFunc;
  mMatchTag = nsnull;
  Init(aDocument);
}

void nsContentList::Init(nsIDocument *aDocument)
{
  NS_INIT_REFCNT();
  mScriptObject = nsnull;
  // XXX We don't reference count this reference.
  // If the document goes away first, we'll be informed and we
  // can drop our reference.
  // If we go away first, we'll get rid of ourselves from the
  // document's observer list.
  mDocument = aDocument;
  mDocument->AddObserver(this);
  nsIContent *root = mDocument->GetRootContent();
  PopulateSelf(root);
  NS_RELEASE(root);
}
 
nsContentList::~nsContentList()
{
  if (nsnull != mDocument) {
    mDocument->RemoveObserver(this);
  }
  
  if (nsnull != mMatchTag) {
    delete mMatchTag;
  }
}

static NS_DEFINE_IID(kIScriptObjectOwnerIID, NS_ISCRIPTOBJECTOWNER_IID);
static NS_DEFINE_IID(kIDOMNodeListIID, NS_IDOMNODELIST_IID);
static NS_DEFINE_IID(kIDOMNodeIID, NS_IDOMNODE_IID);
static NS_DEFINE_IID(kIDOMHTMLCollectionIID, NS_IDOMHTMLCOLLECTION_IID);
static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);

nsresult nsContentList::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  if (nsnull == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }
  if (aIID.Equals(kIDOMNodeListIID)) {
    *aInstancePtr = (void*)(nsIDOMNodeList*)this;
    AddRef();
    return NS_OK;
  }
  if (aIID.Equals(kIDOMHTMLCollectionIID)) {
    *aInstancePtr = (void*)(nsIDOMHTMLCollection*)this;
    AddRef();
    return NS_OK;
  }
  if (aIID.Equals(kIScriptObjectOwnerIID)) {
    *aInstancePtr = (void*)(nsIScriptObjectOwner*)this;
    AddRef();
    return NS_OK;
  }
  if (aIID.Equals(kISupportsIID)) {
    *aInstancePtr = (void*)(nsISupports*)(nsIDOMNodeList*)this;
    AddRef();
    return NS_OK;
  }
  return NS_NOINTERFACE;
}

NS_IMPL_ADDREF(nsContentList)
NS_IMPL_RELEASE(nsContentList)

NS_IMETHODIMP 
nsContentList::Match(nsIContent *aContent, PRBool *aMatch)
{
  if (nsnull != mMatchTag) {
    nsIAtom *name = aContent->GetTag();
    
    if ((nsnull !=name) && mMatchTag->EqualsIgnoreCase(name)) {
      *aMatch = PR_TRUE;
    }
    else {
      *aMatch = PR_FALSE;
    }
    
    NS_IF_RELEASE(name);
  }
  else if (nsnull != mFunc) {
    *aMatch = (*mFunc)(aContent);
  }
  else {
    *aMatch = PR_FALSE;
  }

  return NS_OK;
}

NS_IMETHODIMP 
nsContentList::Add(nsIContent *aContent)
{
  // XXX Should hold a reference ??
  mContent.AppendElement(aContent);
  
  return NS_OK;
}

NS_IMETHODIMP 
nsContentList::Remove(nsIContent *aContent)
{
  // XXX Should drop a reference ??
  mContent.RemoveElement(aContent);
  
  return NS_OK;
}

NS_IMETHODIMP 
nsContentList::Reset()
{
  // XXX Should drop references ??
  mContent.Clear();
  
  return NS_OK;
}

NS_IMETHODIMP 
nsContentList::GetLength(PRUint32* aLength)
{
  *aLength = mContent.Count();
  
  return NS_OK;
}

NS_IMETHODIMP 
nsContentList::Item(PRUint32 aIndex, nsIDOMNode** aReturn)
{
  nsISupports *element = (nsISupports *)mContent.ElementAt(aIndex);

  if (nsnull != element) {
    return element->QueryInterface(kIDOMNodeIID, (void **)aReturn);
  }
  else {
    *aReturn = nsnull;
  }
  
  return NS_OK;
}

NS_IMETHODIMP 
nsContentList::NamedItem(const nsString& aName, nsIDOMNode** aReturn)
{
  PRInt32 i, count = mContent.Count();

  for (i = 0; i < count; i++) {
    nsIContent *content = (nsIContent *)mContent.ElementAt(i);
    if (nsnull != content) {
      nsAutoString name;
      // XXX Should it be an EqualsIgnoreCase?
      if ((content->GetAttribute("name", name) == eContentAttr_HasValue) &&
	  (aName.Equals(name))) {
	return content->QueryInterface(kIDOMNodeIID, (void **)aReturn);
      }
    }
  }
  
  *aReturn = nsnull;
  return NS_OK;
}

NS_IMETHODIMP 
nsContentList::GetScriptObject(nsIScriptContext *aContext, void** aScriptObject)
{
  nsresult res = NS_OK;
  nsIScriptGlobalObject *global = aContext->GetGlobalObject();

  if (nsnull == mScriptObject) {
    res = NS_NewScriptHTMLCollection(aContext, this, global, (void**)&mScriptObject);
  }
  *aScriptObject = mScriptObject;

  NS_RELEASE(global);
  return res;
}

NS_IMETHODIMP 
nsContentList::ResetScriptObject()
{
  mScriptObject = nsnull;
  return NS_OK;
}

PRBool nsContentList::MatchSelf(nsIContent *aContent)
{
  PRBool match;
  PRInt32 i, count;

  Match(aContent, &match);
  if (match) {
    return PR_TRUE;
  }
  
  count = aContent->ChildCount();
  for (i = 0; i < count; i++) {
    nsIContent *child = aContent->ChildAt(i);
    if (MatchSelf(child)) {
      NS_RELEASE(child);
      return PR_TRUE;
    }
    NS_RELEASE(child);
  }
  
  return PR_FALSE;
}

void nsContentList::PopulateSelf(nsIContent *aContent)
{
  PRBool match;
  PRInt32 i, count;

  Match(aContent, &match);
  if (match) {
    Add(aContent);
  }
  
  count = aContent->ChildCount();
  for (i = 0; i < count; i++) {
    nsIContent *child = aContent->ChildAt(i);
    PopulateSelf(child);
    NS_RELEASE(child);
  }
}

NS_IMETHODIMP
nsContentList::ContentAppended(nsIDocument *aDocument,
			       nsIContent* aContainer)
{
  if (aContainer->ChildCount() > 0) {
    nsIContent *content = aContainer->ChildAt(aContainer->ChildCount()-1);
    if (MatchSelf(content)) {
      Reset();
      nsIContent *root = aDocument->GetRootContent();
      PopulateSelf(root);
      NS_RELEASE(root);
    }
    NS_RELEASE(content);
  }

  return NS_OK;
}

NS_IMETHODIMP 
nsContentList::ContentInserted(nsIDocument *aDocument,
			       nsIContent* aContainer,
			       nsIContent* aChild,
			       PRInt32 aIndexInContainer)
{
  if (MatchSelf(aChild)) {
    Reset();
    nsIContent *root = aDocument->GetRootContent();
    PopulateSelf(root);
    NS_RELEASE(root);
  }

  return NS_OK;
}
 
NS_IMETHODIMP
nsContentList::ContentReplaced(nsIDocument *aDocument,
			       nsIContent* aContainer,
			       nsIContent* aOldChild,
			       nsIContent* aNewChild,
			       PRInt32 aIndexInContainer)
{
  if (MatchSelf(aOldChild) || MatchSelf(aNewChild)) {
    Reset();
    nsIContent *root = aDocument->GetRootContent();
    PopulateSelf(root);
    NS_RELEASE(root);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsContentList::ContentHasBeenRemoved(nsIDocument *aDocument,
				     nsIContent* aContainer,
				     nsIContent* aChild,
				     PRInt32 aIndexInContainer)
{
  if (MatchSelf(aChild)) {
    Reset();
    nsIContent *root = aDocument->GetRootContent();
    PopulateSelf(root);
    NS_RELEASE(root);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsContentList::DocumentWillBeDestroyed(nsIDocument *aDocument)
{
  if (nsnull != mDocument) {
    aDocument->RemoveObserver(this);
    mDocument = nsnull;
  }
  Reset();
  
  return NS_OK;
}
