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
 *   Pierre Phaneuf <pp@ludusdesign.com>
 */


#include "TypeInState.h"

/********************************************************************
 *                     XPCOM cruft 
 *******************************************************************/

NS_IMPL_ADDREF(TypeInState)
NS_IMPL_RELEASE(TypeInState)

NS_IMETHODIMP
TypeInState::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  if (nsnull == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }
  if (aIID.Equals(NS_GET_IID(nsISupports))) {
    *aInstancePtr = (void*)(nsISupports*)this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(NS_GET_IID(nsIDOMSelectionListener))) {
    *aInstancePtr = (void*)(nsIDOMSelectionListener*)this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  return NS_NOINTERFACE;
}

/********************************************************************
 *                   public methods
 *******************************************************************/
 
TypeInState::TypeInState() :
 mSetArray()
,mClearedArray()
{
  NS_INIT_REFCNT();
  Reset();
}

TypeInState::~TypeInState()
{
}

NS_IMETHODIMP TypeInState::NotifySelectionChanged(nsIDOMDocument *, nsIDOMSelection *,short)
{ 
  Reset(); 
  return NS_OK;
}

void TypeInState::Reset()
{
  PRInt32 count;
  PropItem *propItemPtr;
  
  while ((count = mClearedArray.Count()))
  {
    // go backwards to keep nsVoidArray from memmoving everything each time
    count--; // nsVoidArray is zero based
    propItemPtr = (PropItem*)mClearedArray.ElementAt(count);
    mClearedArray.RemoveElementAt(count);
    if (propItemPtr) delete propItemPtr;
  }
  while ((count = mSetArray.Count()))
  {
    // go backwards to keep nsVoidArray from memmoving everything each time
    count--; // nsVoidArray is zero based
    propItemPtr = (PropItem*)mSetArray.ElementAt(count);
    mSetArray.RemoveElementAt(count);
    if (propItemPtr) delete propItemPtr;
  }
}


nsresult TypeInState::SetProp(nsIAtom *aProp)
{
  return SetProp(aProp,nsAutoString(),nsAutoString());
}

nsresult TypeInState::SetProp(nsIAtom *aProp, const nsString &aAttr)
{
  return SetProp(aProp,aAttr,nsAutoString());
}

nsresult TypeInState::SetProp(nsIAtom *aProp, const nsString &aAttr, const nsString &aValue)
{
  // if it's already set we are done
  if (IsPropSet(aProp,aAttr,aValue)) return NS_OK;
  
  // make a new propitem
  PropItem *item = new PropItem(aProp,aAttr,aValue);
  if (!item) return NS_ERROR_OUT_OF_MEMORY;
  
  // remove it from the list of cleared properties, if we have a match
  RemovePropFromClearedList(aProp,aAttr,aValue);
  
  // add it to the list of set properties
  mSetArray.AppendElement((void*)item);
  
  return NS_OK;
}


nsresult TypeInState::ClearAllProps()
{
  // null prop means "all" props
  return ClearProp(nsnull,nsAutoString(),nsAutoString());
}

nsresult TypeInState::ClearProp(nsIAtom *aProp)
{
  return ClearProp(aProp,nsAutoString(),nsAutoString());
}

nsresult TypeInState::ClearProp(nsIAtom *aProp, const nsString &aAttr)
{
  return ClearProp(aProp,aAttr,nsAutoString());
}

nsresult TypeInState::ClearProp(nsIAtom *aProp, const nsString &aAttr, const nsString &aValue)
{
  // if it's already cleared we are done
  if (IsPropCleared(aProp,aAttr,aValue)) return NS_OK;
  
  // make a new propitem
  PropItem *item = new PropItem(aProp,aAttr,aValue);
  if (!item) return NS_ERROR_OUT_OF_MEMORY;
  
  // remove it from the list of set properties, if we have a match
  RemovePropFromSetList(aProp,aAttr,aValue);
  
  // add it to the list of cleared properties
  mClearedArray.AppendElement((void*)item);
  
  return NS_OK;
}


/***************************************************************************
 *    TakeClearProperty: hands back next poroperty item on the clear list.
 *                       caller assumes ownership of PropItem and must delete it.
 */  
nsresult TypeInState::TakeClearProperty(PropItem **outPropItem)
{
  if (!outPropItem) return NS_ERROR_NULL_POINTER;
  *outPropItem = nsnull;
  PRInt32 count = mClearedArray.Count();
  if (count) // go backwards to keep nsVoidArray from memmoving everything each time
  {
    count--; // nsVoidArray is zero based
    *outPropItem = (PropItem*)mClearedArray[count];
    mClearedArray.RemoveElementAt(count);
  }
  return NS_OK;
}

/***************************************************************************
 *    TakeSetProperty: hands back next poroperty item on the set list.
 *                     caller assumes ownership of PropItem and must delete it.
 */  
nsresult TypeInState::TakeSetProperty(PropItem **outPropItem)
{
  if (!outPropItem) return NS_ERROR_NULL_POINTER;
  *outPropItem = nsnull;
  PRInt32 count = mSetArray.Count();
  if (count) // go backwards to keep nsVoidArray from memmoving everything each time
  {
    count--; // nsVoidArray is zero based
    *outPropItem = (PropItem*)mSetArray[count];
    mSetArray.RemoveElementAt(count);
  }
  return NS_OK;
}

nsresult TypeInState::GetTypingState(PRBool &isSet, PRBool &theSetting, nsIAtom *aProp)
{
  return GetTypingState(isSet, theSetting, aProp, nsAutoString(), nsAutoString());
}

nsresult TypeInState::GetTypingState(PRBool &isSet, 
                                     PRBool &theSetting, 
                                     nsIAtom *aProp, 
                                     const nsString &aAttr)
{
  return GetTypingState(isSet, theSetting, aProp, aAttr, nsAutoString());
}


nsresult TypeInState::GetTypingState(PRBool &isSet, 
                                     PRBool &theSetting, 
                                     nsIAtom *aProp,
                                     const nsString &aAttr, 
                                     const nsString &aValue)
{
  if (IsPropSet(aProp, aAttr, aValue))
  {
    isSet = PR_TRUE;
    theSetting = PR_TRUE;
  }
  else if (IsPropCleared(aProp, aAttr, aValue))
  {
    isSet = PR_TRUE;
    theSetting = PR_FALSE;
  }
  else
  {
    isSet = PR_FALSE;
  }
  return NS_OK;
}



/********************************************************************
 *                   protected methods
 *******************************************************************/
 
nsresult TypeInState::RemovePropFromSetList(nsIAtom *aProp, 
                                            const nsString &aAttr,
                                            const nsString &aValue)
{
  PRInt32 index;
  PropItem *item;
  if (!aProp)
  {
    // clear _all_ props
    while ((index = mSetArray.Count()))
    {
      // go backwards to keep nsVoidArray from memmoving everything each time
      index--; // nsVoidArray is zero based
      item = (PropItem*)mSetArray.ElementAt(index);
      mSetArray.RemoveElementAt(index);
      if (item) delete item;
    }
  }
  else if (FindPropInList(aProp, aAttr, aValue, mSetArray, index))
  {
    item = (PropItem*)mSetArray.ElementAt(index);
    mSetArray.RemoveElementAt(index);
    if (item) delete item;
  }
  return NS_OK;
}


nsresult TypeInState::RemovePropFromClearedList(nsIAtom *aProp, 
                                            const nsString &aAttr,
                                            const nsString &aValue)
{
  PRInt32 index;
  if (FindPropInList(aProp, aAttr, aValue, mClearedArray, index))
  {
    PropItem *item = (PropItem*)mClearedArray.ElementAt(index);
    mClearedArray.RemoveElementAt(index);
    if (item) delete item;
  }
  return NS_OK;
}


PRBool TypeInState::IsPropSet(nsIAtom *aProp, 
                              const nsString &aAttr,
                              const nsString &aValue)
{
  PRInt32 i;
  return IsPropSet(aProp, aAttr, aValue, i);
}


PRBool TypeInState::IsPropSet(nsIAtom *aProp, 
                              const nsString &aAttr,
                              const nsString &aValue,
                              PRInt32 &outIndex)
{
  // linear search.  list should be short.
  PRInt32 i, count = mSetArray.Count();
  for (i=0; i<count; i++)
  {
    PropItem *item = (PropItem*)mSetArray[i];
    if ( (item->tag == aProp) &&
         (item->attr == aAttr) )
    {
      outIndex = i;
      return PR_TRUE;
    }
  }
  return PR_FALSE;
}


PRBool TypeInState::IsPropCleared(nsIAtom *aProp, 
                                  const nsString &aAttr,
                                  const nsString &aValue)
{
  PRInt32 i;
  return IsPropCleared(aProp, aAttr, aValue, i);
}


PRBool TypeInState::IsPropCleared(nsIAtom *aProp, 
                                  const nsString &aAttr,
                                  const nsString &aValue,
                                  PRInt32 &outIndex)
{
  if (FindPropInList(aProp, aAttr, aValue, mClearedArray, outIndex))
    return PR_TRUE;
  if (FindPropInList(0, 0, 0, mClearedArray, outIndex))
  {
    // special case for all props cleared
    outIndex = -1;
    return PR_TRUE;
  }
  return PR_FALSE;
}

PRBool TypeInState::FindPropInList(nsIAtom *aProp, 
                                   const nsString &aAttr,
                                   const nsString &aValue,
                                   nsVoidArray &aList,
                                   PRInt32 &outIndex)
{
  // linear search.  list should be short.
  PRInt32 i, count = aList.Count();
  for (i=0; i<count; i++)
  {
    PropItem *item = (PropItem*)aList[i];
    if ( (item->tag == aProp) &&
         (item->attr == aAttr) ) 
    {
      outIndex = i;
      return PR_TRUE;
    }
  }
  return PR_FALSE;
}



/********************************************************************
 *    PropItem: helper struct for TypeInState
 *******************************************************************/

PropItem::PropItem(nsIAtom *aTag, const nsString &aAttr, const nsString &aValue) :
 tag(aTag)
,attr(aAttr)
,value(aValue)
{
}

PropItem::~PropItem()
{
}
