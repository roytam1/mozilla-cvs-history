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

#include "nsIDOMText.h"
#include "nsIDOMEventReceiver.h"
#include "nsIContent.h"
#include "nsITextContent.h"
#include "nsGenericDOMDataNode.h"
#include "nsIDocument.h"
#include "nsCRT.h"
#include "nsLayoutAtoms.h"
#include "nsString.h"
#include "nsContentUtils.h"


class nsTextNode : public nsITextContent,
                   public nsIDOMText
{
public:
  nsTextNode();
  virtual ~nsTextNode();

  // nsISupports
  NS_DECL_ISUPPORTS

  // nsIDOMNode
  NS_IMPL_NSIDOMNODE_USING_GENERIC_DOM_DATA(mInner)

  // nsIDOMCharacterData
  NS_IMPL_NSIDOMCHARACTERDATA_USING_GENERIC_DOM_DATA(mInner)

  // nsIDOMText
  NS_IMPL_NSIDOMTEXT_USING_GENERIC_DOM_DATA(mInner)

  // nsIContent
  NS_IMPL_ICONTENT_USING_GENERIC_DOM_DATA(mInner)

  NS_IMETHOD SizeOf(nsISizeOfHandler* aSizer, PRUint32* aResult) const {
    return mInner.SizeOf(aSizer, aResult, sizeof(*this));
  }

  // nsITextContent
  NS_IMPL_ITEXTCONTENT_USING_GENERIC_DOM_DATA(mInner)

protected:
  nsGenericDOMDataNode mInner;
  PRUint32 mContentID;
};

nsresult
NS_NewTextNode(nsIContent** aInstancePtrResult)
{
  NS_PRECONDITION(nsnull != aInstancePtrResult, "null ptr");
  if (nsnull == aInstancePtrResult) {
    return NS_ERROR_NULL_POINTER;
  }
  nsTextNode* it;
  NS_NEWXPCOM(it, nsTextNode);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return it->QueryInterface(NS_GET_IID(nsIContent), (void **) aInstancePtrResult);
}

nsTextNode::nsTextNode()
{
  NS_INIT_REFCNT();
  mContentID = 0;
}

nsTextNode::~nsTextNode()
{
}

NS_IMPL_ADDREF(nsTextNode)
NS_IMPL_RELEASE(nsTextNode)


// XPConnect interface list for nsTextNode
NS_CLASSINFO_MAP_BEGIN(Text)
  NS_CLASSINFO_MAP_ENTRY(nsIDOMText)
  NS_CLASSINFO_MAP_ENTRY(nsIDOMEventTarget)
NS_CLASSINFO_MAP_END


// QueryInterface implementation for nsTextNode
NS_INTERFACE_MAP_BEGIN(nsTextNode)
  NS_INTERFACE_MAP_ENTRY_DOM_DATA()
  NS_INTERFACE_MAP_ENTRY(nsITextContent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMText)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCharacterData)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(Text)
NS_INTERFACE_MAP_END


NS_IMETHODIMP 
nsTextNode::GetTag(nsIAtom*& aResult) const
{
  aResult = nsLayoutAtoms::textTagName;
  NS_ADDREF(aResult);
  return NS_OK;
}

NS_IMETHODIMP 
nsTextNode::GetNodeInfo(nsINodeInfo*& aResult) const
{
  aResult = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsTextNode::GetNodeName(nsAWritableString& aNodeName)
{
  aNodeName.Assign(NS_LITERAL_STRING("#text"));
  return NS_OK;
}

NS_IMETHODIMP
nsTextNode::GetNodeType(PRUint16* aNodeType)
{
  *aNodeType = (PRUint16)nsIDOMNode::TEXT_NODE;
  return NS_OK;
}

NS_IMETHODIMP
nsTextNode::CloneNode(PRBool aDeep, nsIDOMNode** aReturn)
{
  nsresult result = NS_OK;
  nsTextNode* it;
  NS_NEWXPCOM(it, nsTextNode);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  // XXX Increment the ref count before calling any
  // methods. If they do a QI and then a Release()
  // the instance will be deleted.
  result = it->QueryInterface(NS_GET_IID(nsIDOMNode), (void**) aReturn);
  if (NS_FAILED(result)) {
    return result;
  }
  nsAutoString data;
  result = GetData(data);
  if (NS_FAILED(result)) {
    NS_RELEASE(*aReturn);
    return result;
  }
  result = it->SetData(data);
  if (NS_FAILED(result)) {
    NS_RELEASE(*aReturn);
    return result;
  }
  return result;
}

NS_IMETHODIMP 
nsTextNode::CloneContent(PRBool aCloneText, nsITextContent** aReturn)
{
  nsresult result = NS_OK;
  nsTextNode* it;
  NS_NEWXPCOM(it, nsTextNode);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  result = it->QueryInterface(NS_GET_IID(nsITextContent), (void**) aReturn);
  if (NS_FAILED(result) || !aCloneText) {
    return result;
  }
  nsAutoString data;
  result = GetData(data);
  if (NS_FAILED(result)) {
    NS_RELEASE(*aReturn);
    return result;
  }
  result = it->SetData(data);
  if (NS_FAILED(result)) {
    NS_RELEASE(*aReturn);
    return result;
  }
  return result;
}

NS_IMETHODIMP
nsTextNode::List(FILE* out, PRInt32 aIndent) const
{
  NS_PRECONDITION(nsnull != mInner.mDocument, "bad content");

  PRInt32 index;
  for (index = aIndent; --index >= 0; ) fputs("  ", out);

  fprintf(out, "Text@%p refcount=%d<", this, mRefCnt);

  nsAutoString tmp;
  mInner.ToCString(tmp, 0, mInner.mText.GetLength());
  fputs(tmp, out);

  fputs(">\n", out);
  return NS_OK;
}

NS_IMETHODIMP
nsTextNode::DumpContent(FILE* out, PRInt32 aIndent,PRBool aDumpAll) const
{
  NS_PRECONDITION(nsnull != mInner.mDocument, "bad content");

  if(aDumpAll) {
    PRInt32 index;
    for (index = aIndent; --index >= 0; ) fputs("  ", out);

    nsAutoString tmp;
    mInner.ToCString(tmp, 0, mInner.mText.GetLength());
    
    if(!tmp.EqualsWithConversion("\\n")) {
      fputs(tmp, out);
      if(aIndent) fputs("\n", out);
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsTextNode::HandleDOMEvent(nsIPresContext* aPresContext,
                           nsEvent* aEvent,
                           nsIDOMEvent** aDOMEvent,
                           PRUint32 aFlags,
                           nsEventStatus* aEventStatus)
{
  return mInner.HandleDOMEvent(aPresContext, aEvent, aDOMEvent,
                               aFlags, aEventStatus);
}

NS_IMETHODIMP
nsTextNode::GetContentID(PRUint32* aID)
{
  *aID = mContentID;
  return NS_OK;
}

NS_IMETHODIMP
nsTextNode::SetContentID(PRUint32 aID) 
{
  mContentID = aID;
  return NS_OK;
}

