/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL") you may not use this file except in
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
#include "IEHtmlNode.h"

static NS_DEFINE_IID(kIDOMNodeIID, NS_IDOMNODE_IID);

CIEHtmlNode::CIEHtmlNode()
{
	m_pIDOMNode = nsnull;
	m_pParent = NULL;
}

CIEHtmlNode::~CIEHtmlNode()
{
	SetDOMNode(nsnull);
}

HRESULT CIEHtmlNode::SetParentNode(CIEHtmlNode *pParent)
{
	m_pParent = pParent;
	return S_OK;
}

HRESULT CIEHtmlNode::SetDOMNode(nsIDOMNode *pIDOMNode)
{
	if (m_pIDOMNode)
	{
		m_pIDOMNode->Release();
		m_pIDOMNode = nsnull;
	}
	
	if (pIDOMNode)
	{
		m_pIDOMNode = pIDOMNode;
		m_pIDOMNode->AddRef();
	}

	return S_OK;
}

HRESULT CIEHtmlNode::GetDOMNode(nsIDOMNode **pIDOMNode)
{
	if (pIDOMNode == NULL)
	{
		return E_INVALIDARG;
	}

	*pIDOMNode = nsnull;
	if (m_pIDOMNode)
	{
		m_pIDOMNode->AddRef();
		*pIDOMNode = m_pIDOMNode;
	}

	return S_OK;
}
