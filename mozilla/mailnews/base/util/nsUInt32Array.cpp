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

#include "msgCore.h"    // precompiled header...

#include "MailNewsTypes.h"
#include "nsUInt32Array.h"
#include "nsQuickSort.h"

nsUInt32Array::nsUInt32Array()
{
	m_nSize = 0;
	m_nMaxSize = 0;
	m_nGrowBy = 0;
	m_pData = NULL;
}

nsUInt32Array::~nsUInt32Array()
{
	SetSize(0);
}

/////////////////////////////////////////////////////////////////////////////

PRUint32 nsUInt32Array::GetSize() const
{
	return m_nSize;
}

PRBool nsUInt32Array::SetSize(PRUint32 nSize,
                              PRBool adjustGrowth,
                              PRUint32 nGrowBy)
{
	PR_ASSERT(nSize >= 0);

	if (adjustGrowth)
		m_nGrowBy = nGrowBy;

#ifdef MAX_ARR_ELEMS
	if (nSize > MAX_ARR_ELEMS);
	{
		PR_ASSERT(nSize <= MAX_ARR_ELEMS); // Will fail
		return PR_FALSE;
	}
#endif

	if (nSize == 0)
	{
		// Remove all elements
		PR_Free(m_pData);
		m_nSize = 0;
		m_nMaxSize = 0;
		m_pData = NULL;
	}
	else if (m_pData == NULL)
	{
		// Create a new array
		m_nMaxSize = PR_MAX(8, nSize);
		m_pData = (PRUint32 *)PR_Calloc(1, m_nMaxSize * sizeof(PRUint32));
		if (m_pData)
			m_nSize = nSize;
		else
			m_nSize = m_nMaxSize = 0;
	}
	else if (nSize <= m_nMaxSize)
	{
		// The new size is within the current maximum size, make sure new
		// elements are to initialized to zero
		if (nSize > m_nSize)
			nsCRT::memset(&m_pData[m_nSize], 0, (nSize - m_nSize) * sizeof(PRUint32));

		m_nSize = nSize;
	}
	else
	{
		// The array needs to grow, figure out how much
		PRUint32 nMaxSize;
		nGrowBy  = PR_MAX(m_nGrowBy, PR_MIN(1024, PR_MAX(8, m_nSize / 8)));
		nMaxSize = PR_MAX(nSize, m_nMaxSize + nGrowBy);
#ifdef MAX_ARR_ELEMS
		nMaxSize = PR_MIN(MAX_ARR_ELEMS, nMaxSize);
#endif

		PRUint32 *pNewData = (PRUint32 *)PR_Malloc(nMaxSize * sizeof(PRUint32));
		if (pNewData)
		{
			// Copy the data from the old array to the new one
			nsCRT::memcpy(pNewData, m_pData, m_nSize * sizeof(PRUint32));

			// Zero out the remaining elements
			nsCRT::memset(&pNewData[m_nSize], 0, (nSize - m_nSize) * sizeof(PRUint32));
			m_nSize = nSize;
			m_nMaxSize = nMaxSize;

			// Free the old array
			PR_Free(m_pData);
			m_pData = pNewData;
		}
	}

	return nSize == m_nSize;
}

/////////////////////////////////////////////////////////////////////////////

PRUint32 &nsUInt32Array::ElementAt(PRUint32 nIndex)
{
	PR_ASSERT(nIndex >= 0 && nIndex < m_nSize);
	return m_pData[nIndex];
}

PRUint32 nsUInt32Array::GetAt(PRUint32 nIndex) const
{
	PR_ASSERT(nIndex >= 0 && nIndex < m_nSize);
	return m_pData[nIndex];
}

PRUint32 *nsUInt32Array::GetData() 
{
	return m_pData;
}

void nsUInt32Array::SetAt(PRUint32 nIndex, PRUint32 newElement)
{
	PR_ASSERT(nIndex >= 0 && nIndex < m_nSize);
	m_pData[nIndex] = newElement;
}

/////////////////////////////////////////////////////////////////////////////

PRUint32 nsUInt32Array::Add(PRUint32 newElement)
{
	PRUint32 nIndex = m_nSize;

#ifdef MAX_ARR_ELEMS
	if (nIndex >= MAX_ARR_ELEMS) 
		return -1;	     
#endif			

	SetAtGrow(nIndex, newElement);
	return nIndex;
}

PRUint32 nsUInt32Array::Add(PRUint32 *elementPtr, PRUint32 numElements) 
{ 
	if (SetSize(m_nSize + numElements))
		nsCRT::memcpy(m_pData + m_nSize, elementPtr, numElements * sizeof(PRUint32)); 

	return m_nSize; 
} 

PRUint32 *nsUInt32Array::CloneData() 
{ 
	PRUint32 *copyOfData = (PRUint32 *)PR_Malloc(m_nSize * sizeof(PRUint32)); 
	if (copyOfData) 
		nsCRT::memcpy(copyOfData, m_pData, m_nSize * sizeof(PRUint32)); 

	return copyOfData; 
} 

void nsUInt32Array::InsertAt(PRUint32 nIndex, PRUint32 newElement, PRUint32 nCount)
{
	PR_ASSERT(nIndex >= 0);
	PR_ASSERT(nCount > 0);

	if (nIndex >= m_nSize)
	{
		// If the new element is after the end of the array, grow the array
		SetSize(nIndex + nCount);
	}
	else
	{
		// The element is being insert inside the array
		int nOldSize = m_nSize;
		SetSize(m_nSize + nCount);

		// Move the data after the insertion point
		nsCRT::memmove(&m_pData[nIndex + nCount], &m_pData[nIndex],
			       (nOldSize - nIndex) * sizeof(PRUint32));
	}

	// Insert the new elements
	PR_ASSERT(nIndex + nCount <= m_nSize);
	while (nCount--)
		m_pData[nIndex++] = newElement;
}

void nsUInt32Array::InsertAt(PRUint32 nStartIndex, const nsUInt32Array *pNewArray)
{
	PR_ASSERT(nStartIndex >= 0);
	PR_ASSERT(pNewArray != NULL);

	if (pNewArray->GetSize() > 0)
	{
		InsertAt(nStartIndex, pNewArray->GetAt(0), pNewArray->GetSize());
		for (PRUint32 i = 1; i < pNewArray->GetSize(); i++)
			m_pData[nStartIndex + i] = pNewArray->GetAt(i);
	}
}

void nsUInt32Array::RemoveAll()
{
	SetSize(0);
}

void nsUInt32Array::RemoveAt(PRUint32 nIndex, PRUint32 nCount)
{
	PR_ASSERT(nIndex >= 0);
	PR_ASSERT(nIndex + nCount <= m_nSize);

	if (nCount > 0)
	{
		// Make sure not to overstep the end of the array
		int nMoveCount = m_nSize - (nIndex + nCount);
		if (nCount && nMoveCount)
			nsCRT::memmove(&m_pData[nIndex], &m_pData[nIndex + nCount],
		               nMoveCount * sizeof(PRUint32));

		m_nSize -= nCount;
	}
}

void nsUInt32Array::SetAtGrow(PRUint32 nIndex, PRUint32 newElement)
{
	PR_ASSERT(nIndex >= 0);

	if (nIndex >= m_nSize)
		SetSize(nIndex+1);
	m_pData[nIndex] = newElement;
}

/////////////////////////////////////////////////////////////////////////////

void nsUInt32Array::CopyArray(nsUInt32Array *oldA)
{
	CopyArray(*oldA);
}

void nsUInt32Array::CopyArray(nsUInt32Array &oldA)
{
	if (m_pData)
		PR_Free(m_pData);
	m_nSize = oldA.m_nSize;
	m_nMaxSize = oldA.m_nMaxSize;
	m_pData = (PRUint32 *)PR_Malloc(m_nSize * sizeof(PRUint32));
	if (m_pData)
		nsCRT::memcpy(m_pData, oldA.m_pData, m_nSize * sizeof(PRUint32));
}

/////////////////////////////////////////////////////////////////////////////

static int CompareDWord (const void *v1, const void *v2, void *)
{
	// QuickSort callback to compare array values
	PRUint32 i1 = *(PRUint32 *)v1;
	PRUint32 i2 = *(PRUint32 *)v2;
	return i1 - i2;
}

void nsUInt32Array::QuickSort (int (*compare) (const void *elem1, const void *elem2, void *data))
{
	// we don't have a quick sort method in mozilla yet....commenting out for now.
#if 0
	if (m_nSize > 1)
		nsQuickSort(m_pData, m_nSize, sizeof(void*), compare ? compare : CompareDWord, nsnull);
#endif
}
