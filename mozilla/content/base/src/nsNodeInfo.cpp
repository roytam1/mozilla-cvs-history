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

#include "nsNodeInfo.h"
#include "nsNodeInfoManager.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsIAtom.h"
#include "nsINameSpaceManager.h"


nsNodeInfo::nsNodeInfo()
  : mInner(), mOwnerManager(nsnull)
{
  NS_INIT_REFCNT();
}


nsNodeInfo::~nsNodeInfo()
{
  if (mOwnerManager) {
    mOwnerManager->RemoveNodeInfo(this);
    NS_RELEASE(mOwnerManager);
  }

  NS_IF_RELEASE(mInner.mName);
  NS_IF_RELEASE(mInner.mPrefix);
}


nsresult
nsNodeInfo::Init(nsIAtom *aName, nsIAtom *aPrefix, PRInt32 aNamespaceID,
                 nsNodeInfoManager *aOwnerManager)
{
  NS_ENSURE_TRUE(!mInner.mName && !mInner.mPrefix && !mOwnerManager,
                 NS_ERROR_ALREADY_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aName);
  NS_ENSURE_ARG_POINTER(aOwnerManager);

  mInner.mName = aName;
  NS_ADDREF(mInner.mName);

  mInner.mPrefix = aPrefix;
  NS_IF_ADDREF(mInner.mPrefix);

  mInner.mNamespaceID = aNamespaceID;

  mOwnerManager = aOwnerManager;
  NS_ADDREF(mOwnerManager);

  return NS_OK;
}


// nsISupports

NS_IMPL_THREADSAFE_ISUPPORTS(nsNodeInfo, NS_GET_IID(nsINodeInfo));


// nsINodeInfo

NS_IMETHODIMP
nsNodeInfo::GetName(nsString& aName)
{
  NS_ENSURE_TRUE(mInner.mName, NS_ERROR_NOT_INITIALIZED);

  return mInner.mName->ToString(aName);
}


NS_IMETHODIMP
nsNodeInfo::GetNameAtom(nsIAtom*& aAtom)
{
  NS_ABORT_IF_FALSE(mInner.mName, "nsNodeInfo not initialized!");

  aAtom = mInner.mName;
  NS_IF_ADDREF(aAtom);

  return NS_OK;
}


NS_IMETHODIMP
nsNodeInfo::GetQualifiedName(nsString& aQualifiedName)
{
  NS_ENSURE_TRUE(mInner.mName, NS_ERROR_NOT_INITIALIZED);

#ifdef MOZILLA_IS_READY_FOR_THIS
  if (mInner.mPrefix) {
    mInner.mPrefix->ToString(aQualifiedName);

    aQualifiedName.Append(':');
  }

  nsAutoString tmp;
  mInner.mName->ToString(tmp);

  aQualifiedName.Append(tmp);

#else
  mInner.mName->ToString(aQualifiedName);
#endif

  return NS_OK;
}


NS_IMETHODIMP
nsNodeInfo::GetLocalName(nsString& aLocalName)
{
  NS_ENSURE_TRUE(mInner.mName, NS_ERROR_NOT_INITIALIZED);

  if (mInner.mNamespaceID > 0) {
    return mInner.mName->ToString(aLocalName);
  }

  aLocalName.Truncate();

  return NS_OK;
}


NS_IMETHODIMP
nsNodeInfo::GetPrefix(nsString& aPrefix)
{
  if (mInner.mPrefix) {
    mInner.mPrefix->ToString(aPrefix);
  } else {
    aPrefix.Truncate();
  }

  return NS_OK;
}


NS_IMETHODIMP
nsNodeInfo::GetPrefixAtom(nsIAtom*& aAtom)
{
  aAtom = mInner.mPrefix;
  NS_IF_ADDREF(aAtom);

  return NS_OK;
}


NS_IMETHODIMP
nsNodeInfo::GetNamespaceURI(nsString& aNameSpaceURI)
{
  NS_ENSURE_TRUE(mOwnerManager, NS_ERROR_NOT_INITIALIZED);
  nsresult rv = NS_OK;

  if (mInner.mNamespaceID > 0) {
    nsCOMPtr<nsINameSpaceManager> nsm;

    mOwnerManager->GetNamespaceManager(*getter_AddRefs(nsm));
    NS_ENSURE_TRUE(nsm, NS_ERROR_NOT_INITIALIZED);

    rv = nsm->GetNameSpaceURI(mInner.mNamespaceID, aNameSpaceURI);
  } else {
    aNameSpaceURI.Truncate();
  }

  return rv;
}


NS_IMETHODIMP
nsNodeInfo::GetNamespaceID(PRInt32& aResult)
{
  aResult = mInner.mNamespaceID;

  return NS_OK;
}


NS_IMETHODIMP
nsNodeInfo::GetNodeInfoManager(nsINodeInfoManager*& aNodeInfoManager)
{
  NS_ENSURE_TRUE(mOwnerManager, NS_ERROR_NOT_INITIALIZED);

  aNodeInfoManager = mOwnerManager;

  NS_ADDREF(aNodeInfoManager);

  return NS_OK;
}


NS_IMETHODIMP_(PRBool)
nsNodeInfo::Equals(nsIAtom *aNameAtom)
{
  return mInner.mName == aNameAtom;
}


NS_IMETHODIMP_(PRBool)
nsNodeInfo::Equals(const nsString& aName)
{
  if (!mInner.mName) return PR_FALSE;

  const PRUnichar *name;
  mInner.mName->GetUnicode(&name);

  return aName.Equals(name);
}


NS_IMETHODIMP_(PRBool)
nsNodeInfo::Equals(nsIAtom *aNameAtom, nsIAtom *aPrefixAtom)
{
  return (mInner.mName == aNameAtom) && (mInner.mPrefix == aPrefixAtom);
}


NS_IMETHODIMP_(PRBool)
nsNodeInfo::Equals(const nsString& aName, const nsString& aPrefix)
{
  if (!mInner.mName) return PR_FALSE;

  const PRUnichar *name, *prefix = nsnull;
  mInner.mName->GetUnicode(&name);

  if (mInner.mPrefix)
    mInner.mPrefix->GetUnicode(&prefix);

  return aName.Equals(name) && aPrefix.Equals(prefix);
}


NS_IMETHODIMP_(PRBool)
nsNodeInfo::Equals(nsIAtom *aNameAtom, PRInt32 aNamespaceID)
{
  return (mInner.mName == aNameAtom) && (mInner.mNamespaceID == aNamespaceID);
}


NS_IMETHODIMP_(PRBool)
nsNodeInfo::Equals(const nsString& aName, PRInt32 aNamespaceID)
{
  if (!mInner.mName) return PR_FALSE;

  const PRUnichar *name;
  mInner.mName->GetUnicode(&name);

  return aName.Equals(name) && (mInner.mNamespaceID == aNamespaceID);
}


NS_IMETHODIMP_(PRBool)
nsNodeInfo::Equals(nsIAtom *aNameAtom, nsIAtom *aPrefixAtom,
                   PRInt32 aNamespaceID)
{
  return (mInner.mName == aNameAtom) &&
         (mInner.mPrefix == aPrefixAtom) &&
         (mInner.mNamespaceID == aNamespaceID);
}


NS_IMETHODIMP_(PRBool)
nsNodeInfo::Equals(const nsString& aName, const nsString& aPrefix,
                   PRInt32 aNamespaceID)
{
  if (!mInner.mName) return PR_FALSE;

  const PRUnichar *name, *prefix = nsnull;
  mInner.mName->GetUnicode(&name);

  if (mInner.mPrefix)
    mInner.mPrefix->GetUnicode(&prefix);

  return aName.Equals(name) && aPrefix.Equals(prefix) &&
         (mInner.mNamespaceID == aNamespaceID);
}


NS_IMETHODIMP_(PRBool)
nsNodeInfo::NamespaceEquals(PRInt32 aNamespaceID)
{
  return mInner.mNamespaceID == aNamespaceID;
}


NS_IMETHODIMP_(PRBool)
nsNodeInfo::NamespaceEquals(const nsString& aNamespaceURI)
{
  NS_ENSURE_TRUE(mOwnerManager, NS_ERROR_NOT_INITIALIZED);
  nsCOMPtr<nsINameSpaceManager> nsmgr;

  NS_ENSURE_SUCCESS(mOwnerManager->GetNamespaceManager(*getter_AddRefs(nsmgr)),
                    NS_ERROR_NOT_INITIALIZED);

  PRInt32 nsid;
  nsmgr->GetNameSpaceID(aNamespaceURI, nsid);

  return mInner.mNamespaceID == nsid;
}


NS_IMETHODIMP
nsNodeInfo::NameChanged(nsIAtom *aName, nsINodeInfo*& aResult)
{
  NS_ENSURE_TRUE(mOwnerManager, NS_ERROR_NOT_INITIALIZED);

  return mOwnerManager->GetNodeInfo(aName, mInner.mPrefix, mInner.mNamespaceID,
                                    aResult);
}


NS_IMETHODIMP
nsNodeInfo::PrefixChanged(nsIAtom *aPrefix, nsINodeInfo*& aResult)
{
  NS_ENSURE_TRUE(mOwnerManager, NS_ERROR_NOT_INITIALIZED);

  return mOwnerManager->GetNodeInfo(mInner.mName, aPrefix, mInner.mNamespaceID,
                                    aResult);
}


PLHashNumber
nsNodeInfoInner::GetHashValue(const void *key)
{
#ifdef NS_DEBUG // Just to shut down a compiler warning
  NS_WARN_IF_FALSE(key, "Null key passed to nsNodeInfo::GetHashValue!");
#endif

  if (key) {
    const nsNodeInfoInner *node = (const nsNodeInfoInner *)key;

    // Is this an acceptable has value?
    return (((PLHashNumber)node->mName) & 0xffff) >> 8;
  }

  return 0;
}


PRIntn
nsNodeInfoInner::KeyCompare(const void *key1, const void *key2)
{
#ifdef NS_DEBUG // Just to shut down a compiler warning
  NS_WARN_IF_FALSE(key1 && key2, "Null key passed to nsNodeInfo::KeyCompare!");
#endif

  if (!key1 || !key2) {
    return PR_FALSE;
  }

  const nsNodeInfoInner *node1 = (const nsNodeInfoInner *)key1;
  const nsNodeInfoInner *node2 = (const nsNodeInfoInner *)key2;

  if (node1->mName == node2->mName &&
      node1->mPrefix == node2->mPrefix &&
      node1->mNamespaceID == node2->mNamespaceID) {

    return PR_TRUE;
  }

  return PR_FALSE;
}

