/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
#include "stdafx.h"

#include <stack>

#include "IEHtmlElement.h"
#include "IEHtmlElementCollection.h"

CIEHtmlElementCollection::CIEHtmlElementCollection()
{
	m_pIDispParent = NULL;
}

CIEHtmlElementCollection::~CIEHtmlElementCollection()
{
}

HRESULT CIEHtmlElementCollection::SetParentNode(IDispatch *pIDispParent)
{
	m_pIDispParent = pIDispParent;
	return S_OK;
}


struct NodeListPos
{
	nsIDOMNodeList *m_pIDOMNodeList;
	PRUint32 m_nListPos;

	NodeListPos(nsIDOMNodeList *pIDOMNodeList, PRUint32 nListPos) :
		m_pIDOMNodeList(pIDOMNodeList),
		m_nListPos(nListPos)
	{
	}

	NodeListPos()
	{
	};
};


HRESULT CIEHtmlElementCollection::CreateFromParentNode(CIEHtmlNode *pParentNode, CIEHtmlElementCollection **pInstance, BOOL bRecurseChildren)
{
	if (pInstance == NULL)
	{
		return E_INVALIDARG;
	}
	
	if (pParentNode == NULL)
	{
		return E_INVALIDARG;
	}

	nsIDOMNode *pIDOMNode = nsnull;
	pParentNode->GetDOMNode(&pIDOMNode);
	if (pIDOMNode == nsnull)
	{
		return E_INVALIDARG;
	}

	*pInstance = NULL;

	CIEHtmlElementCollectionInstance *pCollection = NULL;
	CIEHtmlElementCollectionInstance::CreateInstance(&pCollection);

	CIPtr(IDispatch) cpDispNode;
	pParentNode->GetIDispatch(&cpDispNode);
	pCollection->SetParentNode(cpDispNode);

	// Get elements

	std::stack< NodeListPos > cNodeStack;

	nsIDOMNodeList *pIDOMNodeList = nsnull;
	pIDOMNode->GetChildNodes(&pIDOMNodeList);

	if (pIDOMNodeList)
	{
		cNodeStack.push(NodeListPos(pIDOMNodeList, 0));
	}
	while (!cNodeStack.empty())
	{
		NodeListPos pos = cNodeStack.top();
		cNodeStack.pop();

		nsIDOMNodeList *pIDOMNodeList = pos.m_pIDOMNodeList;
		PRUint32 aLength = 0;
		pIDOMNodeList->GetLength(&aLength);
		for (PRUint32 i = pos.m_nListPos; i < aLength; i++)
		{
			nsIDOMNode *pChildNode = nsnull;
			pIDOMNodeList->Item(i, &pChildNode);
			if (pChildNode == nsnull)
			{
				continue;
			}

			// Create an equivalent IE element
			CIEHtmlElementInstance *pElement = NULL;
			CIEHtmlElementInstance::CreateInstance(&pElement);
			if (pElement)
			{
				pElement->SetDOMNode(pChildNode);
				pElement->SetParentNode(pCollection->m_pIDispParent);
				pCollection->AddNode(pElement);

				if (bRecurseChildren)
				{
					// TODO recurse through
					CIPtr(IHTMLElementCollection) cpCollection;
				}
			}
			pChildNode->Release();
		}
		pIDOMNodeList->Release();
	}

	*pInstance = pCollection;

	pIDOMNode->Release();

	return S_OK;
}


HRESULT CIEHtmlElementCollection::AddNode(IDispatch *pNode)
{
	if (pNode == NULL)
	{
		NG_ASSERT(0);
		return E_INVALIDARG;
	}

	m_cNodeList.push_back(pNode);

	return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IHTMLElementCollection methods


HRESULT STDMETHODCALLTYPE CIEHtmlElementCollection::toString(BSTR __RPC_FAR *String)
{
	if (String == NULL)
	{
		return E_INVALIDARG;
	}
	*String = SysAllocString(OLESTR("ElementCollection"));
	return S_OK;
}


HRESULT STDMETHODCALLTYPE CIEHtmlElementCollection::put_length(long v)
{
	// What is the point of this method?
	return S_OK;
}


HRESULT STDMETHODCALLTYPE CIEHtmlElementCollection::get_length(long __RPC_FAR *p)
{
	if (p == NULL)
	{
		return E_INVALIDARG;
	}
	
	// Return the size of the collection
	*p = m_cNodeList.size();
	return S_OK;
}

typedef CComObject<CComEnum<IEnumVARIANT, &IID_IEnumVARIANT, VARIANT, _Copy<VARIANT> > > CComEnumVARIANT;

HRESULT STDMETHODCALLTYPE CIEHtmlElementCollection::get__newEnum(IUnknown __RPC_FAR *__RPC_FAR *p)
{
	if (p == NULL)
	{
		return E_INVALIDARG;
	}

	*p = NULL;

	// Create a new IEnumVARIANT object
	CComEnumVARIANT *pEnumVARIANT = NULL;
	CComEnumVARIANT::CreateInstance(&pEnumVARIANT);
	if (pEnumVARIANT == NULL)
	{
		NG_ASSERT(0);
		return E_OUTOFMEMORY;
	}

	int nObject;
	int nObjects = m_cNodeList.size();

	// Create an array of VARIANTs
	VARIANT *avObjects = new VARIANT[nObjects];
	if (avObjects == NULL)
	{
		NG_ASSERT(0);
		return E_OUTOFMEMORY;
	}

	// Copy the contents of the collection to the array
	for (nObject = 0; nObject < nObjects; nObject++)
	{
		VARIANT *pVariant = &avObjects[nObject];
		IUnknown *pUnkObject = m_cNodeList[nObject];
		VariantInit(pVariant);
		pVariant->vt = VT_UNKNOWN;
		pVariant->punkVal = pUnkObject;
		pUnkObject->AddRef();
	}

	// Copy the variants to the enumeration object
	pEnumVARIANT->Init(&avObjects[0], &avObjects[nObjects], NULL, AtlFlagCopy);

	// Cleanup the array
	for (nObject = 0; nObject < nObjects; nObject++)
	{
		VARIANT *pVariant = &avObjects[nObject];
		VariantClear(pVariant);
	}
	delete []avObjects;

	return pEnumVARIANT->QueryInterface(IID_IUnknown, (void**) p);
}


HRESULT STDMETHODCALLTYPE CIEHtmlElementCollection::item(VARIANT name, VARIANT index, IDispatch __RPC_FAR *__RPC_FAR *pdisp)
{
	if (pdisp == NULL)
	{
		return E_INVALIDARG;
	}
	
	*pdisp = NULL;

	// Note: parameter "name" contains the index unless its a string
	//       in which case index does. Sensible huh?

	CComVariant vIndex;
	if (SUCCEEDED(vIndex.ChangeType(VT_I4, &name)) ||
		SUCCEEDED(vIndex.ChangeType(VT_I4, &index)))
	{
		// Test for stupid values
		int nIndex = vIndex.lVal;
		if (nIndex < 0 || nIndex >= m_cNodeList.size())
		{
			return E_INVALIDARG;
		}
	
		*pdisp = NULL;
		IDispatch *pNode = m_cNodeList[nIndex];
		if (pNode == NULL)
		{
			NG_ASSERT(0);
			return E_UNEXPECTED;
		}

		pNode->QueryInterface(IID_IDispatch, (void **) pdisp);
		return S_OK;
	}
	else if (SUCCEEDED(vIndex.ChangeType(VT_BSTR, &index)))
	{
		// If this parameter contains a string, the method returns
		// a collection of objects, where the value of the name or
		// id property for each object is equal to the string. 

		// TODO all of the above!
		return E_NOTIMPL;
	}
	else
	{
		return E_INVALIDARG;
	}
}

HRESULT STDMETHODCALLTYPE CIEHtmlElementCollection::tags(VARIANT tagName, IDispatch __RPC_FAR *__RPC_FAR *pdisp)
{
	if (pdisp == NULL)
	{
		return E_INVALIDARG;
	}
	
	*pdisp = NULL;

	// TODO
	// iterate through collection looking for elements with matching tags
	
	return E_NOTIMPL;
}

