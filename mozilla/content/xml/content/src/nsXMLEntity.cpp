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


#include "nsIDOMEntity.h"
#include "nsIDOMEventReceiver.h"
#include "nsIContent.h"
#include "nsGenericDOMDataNode.h"
#include "nsGenericElement.h"
#include "nsLayoutAtoms.h"
#include "nsString.h"
#include "nsIXMLContent.h"


class nsXMLEntity : public nsIDOMEntity,
                    public nsIContent
{
public:
  nsXMLEntity(const nsAReadableString& aName, 
              const nsAReadableString& aPublicId,
              const nsAReadableString& aSystemId, 
              const nsAReadableString& aNotationName);
  virtual ~nsXMLEntity();

  // nsISupports
  NS_DECL_ISUPPORTS

  // nsIDOMNode
  NS_IMPL_NSIDOMNODE_USING_GENERIC_DOM_DATA(mInner)

  // nsIDOMEntity
  NS_IMETHOD    GetPublicId(nsAWritableString& aPublicId);
  NS_IMETHOD    GetSystemId(nsAWritableString& aSystemId);
  NS_IMETHOD    GetNotationName(nsAWritableString& aNotationName);

  // nsIContent
  NS_IMPL_ICONTENT_USING_GENERIC_DOM_DATA(mInner)

  NS_IMETHOD SizeOf(nsISizeOfHandler* aSizer, PRUint32* aResult) const;

protected:
  // XXX Processing instructions are currently implemented by using
  // the generic CharacterData inner object, even though PIs are not
  // character data. This is done simply for convenience and should
  // be changed if this restricts what should be done for character data.
  nsGenericDOMDataNode mInner;
  nsString mName;
  nsString mPublicId;
  nsString mSystemId;
  nsString mNotationName;
};

nsresult
NS_NewXMLEntity(nsIContent** aInstancePtrResult,
                const nsAReadableString& aName,
                const nsAReadableString& aPublicId,
                const nsAReadableString& aSystemId,
                const nsAReadableString& aNotationName)
{
  NS_PRECONDITION(nsnull != aInstancePtrResult, "null ptr");
  if (nsnull == aInstancePtrResult) {
    return NS_ERROR_NULL_POINTER;
  }
  nsIContent* it = new nsXMLEntity(aName, aPublicId, aSystemId, aNotationName);

  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return it->QueryInterface(NS_GET_IID(nsIContent), (void **) aInstancePtrResult);
}

nsXMLEntity::nsXMLEntity(const nsAReadableString& aName,
                         const nsAReadableString& aPublicId,
                         const nsAReadableString& aSystemId,
                         const nsAReadableString& aNotationName) :
  mName(aName), mPublicId(aPublicId), mSystemId(aSystemId), mNotationName(aNotationName)
{
  NS_INIT_REFCNT();
}

nsXMLEntity::~nsXMLEntity()
{
}


// XPConnect interface list for nsXMLEntity
NS_CLASSINFO_MAP_BEGIN(Entity)
  NS_CLASSINFO_MAP_ENTRY(nsIDOMEntity)
NS_CLASSINFO_MAP_END


// QueryInterface implementation for nsXMLEntity
NS_INTERFACE_MAP_BEGIN(nsXMLEntity)
  NS_INTERFACE_MAP_ENTRY_DOM_DATA()
  NS_INTERFACE_MAP_ENTRY(nsIDOMEntity)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(Entity)
NS_INTERFACE_MAP_END


NS_IMPL_ADDREF(nsXMLEntity)
NS_IMPL_RELEASE(nsXMLEntity)


NS_IMETHODIMP    
nsXMLEntity::GetPublicId(nsAWritableString& aPublicId)
{
  aPublicId.Assign(mPublicId);

  return NS_OK;
}

NS_IMETHODIMP    
nsXMLEntity::GetSystemId(nsAWritableString& aSystemId)
{
  aSystemId.Assign(mSystemId);

  return NS_OK;
}

NS_IMETHODIMP    
nsXMLEntity::GetNotationName(nsAWritableString& aNotationName)
{
  aNotationName.Assign(mNotationName);

  return NS_OK;
}

NS_IMETHODIMP 
nsXMLEntity::GetTag(nsIAtom*& aResult) const
{
//  aResult = nsLayoutAtoms::EntityTagName;
//  NS_ADDREF(aResult);

  aResult = nsnull;

  return NS_OK;
}

NS_IMETHODIMP 
nsXMLEntity::GetNodeInfo(nsINodeInfo*& aResult) const
{
  aResult = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsXMLEntity::GetNodeName(nsAWritableString& aNodeName)
{
  aNodeName.Assign(mName);
  return NS_OK;
}

NS_IMETHODIMP
nsXMLEntity::GetNodeType(PRUint16* aNodeType)
{
  *aNodeType = (PRUint16)nsIDOMNode::ENTITY_NODE;
  return NS_OK;
}

NS_IMETHODIMP
nsXMLEntity::CloneNode(PRBool aDeep, nsIDOMNode** aReturn)
{
  nsXMLEntity* it = new nsXMLEntity(mName,
                                    mSystemId,
                                    mPublicId,
                                    mNotationName);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return it->QueryInterface(NS_GET_IID(nsIDOMNode), (void**) aReturn);
}

NS_IMETHODIMP
nsXMLEntity::List(FILE* out, PRInt32 aIndent) const
{
  NS_PRECONDITION(nsnull != mInner.mDocument, "bad content");

  PRInt32 index;
  for (index = aIndent; --index >= 0; ) fputs("  ", out);

  fprintf(out, "Entity refcount=%d<!ENTITY ", mRefCnt);

  nsAutoString tmp(mName);
  if (mPublicId.Length()) {
    tmp.AppendWithConversion(" PUBLIC \"");
    tmp.Append(mPublicId);
    tmp.AppendWithConversion("\"");
  }

  if (mSystemId.Length()) {
    tmp.AppendWithConversion(" SYSTEM \"");
    tmp.Append(mSystemId);
    tmp.AppendWithConversion("\"");
  }

  if (mNotationName.Length()) {
    tmp.AppendWithConversion(" NDATA ");
    tmp.Append(mNotationName);
  }

  fputs(tmp, out);

  fputs(">\n", out);
  return NS_OK;
}

NS_IMETHODIMP
nsXMLEntity::DumpContent(FILE* out, PRInt32 aIndent,PRBool aDumpAll) const
{
  return NS_OK;
}

NS_IMETHODIMP
nsXMLEntity::HandleDOMEvent(nsIPresContext* aPresContext,
                            nsEvent* aEvent,
                            nsIDOMEvent** aDOMEvent,
                            PRUint32 aFlags,
                            nsEventStatus* aEventStatus)
{
  // We should never be getting events
  NS_ASSERTION(0, "event handler called for entity");
  return mInner.HandleDOMEvent(aPresContext, aEvent, aDOMEvent,
                               aFlags, aEventStatus);
}

NS_IMETHODIMP
nsXMLEntity::GetContentID(PRUint32* aID) 
{
  *aID = 0;
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsXMLEntity::SetContentID(PRUint32 aID) 
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP_(PRBool)
nsXMLEntity::IsContentOfType(PRUint32 aFlags)
{
  return PR_FALSE;
}

NS_IMETHODIMP
nsXMLEntity::SizeOf(nsISizeOfHandler* aSizer, PRUint32* aResult) const
{
  if (!aResult) return NS_ERROR_NULL_POINTER;
#ifdef DEBUG
  PRUint32 sum;
  mInner.SizeOf(aSizer, &sum, sizeof(*this));
  PRUint32 ssize;
  mName.SizeOf(aSizer, &ssize);
  sum = sum - sizeof(mName) + ssize;
  mPublicId.SizeOf(aSizer, &ssize);
  sum = sum - sizeof(mPublicId) + ssize;
  mSystemId.SizeOf(aSizer, &ssize);
  sum = sum - sizeof(mSystemId) + ssize;
  mNotationName.SizeOf(aSizer, &ssize);
  sum = sum - sizeof(mNotationName) + ssize;
#endif
  return NS_OK;
}
