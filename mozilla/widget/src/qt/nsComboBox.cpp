/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

#include "nsComboBox.h"
#include "nsColor.h"
#include "nsGUIEvent.h"
#include "nsString.h"
#include "nsStringUtil.h"
#include "nsIDeviceContext.h"

//JCG #define DBG 0

#define INITIAL_MAX_ITEMS 128
#define ITEMS_GROWSIZE    128

//=============================================================================
//
// nsQComboBox class
//
//=============================================================================
nsQComboBox::nsQComboBox(nsWidget * widget,
                         QWidget * parent, 
                         const char * name)
	: QComboBox(TRUE, parent, name), nsQBaseWidget(widget)
{
}

nsQComboBox::~nsQComboBox()
{
}

NS_IMPL_ADDREF_INHERITED(nsComboBox, nsWidget)
NS_IMPL_RELEASE_INHERITED(nsComboBox, nsWidget)
NS_IMPL_QUERY_INTERFACE3(nsComboBox, nsIComboBox, nsIListWidget, nsIWidget)
 
//-------------------------------------------------------------------------
//
// nsComboBox constructor
//
//-------------------------------------------------------------------------
nsComboBox::nsComboBox() : nsWidget(), nsIListWidget(), nsIComboBox()
{
    PR_LOG(QtWidgetsLM, 
           PR_LOG_DEBUG, 
           ("nsComboBox::nsComboBox()\n"));
    NS_INIT_REFCNT();
    mMultiSelect = PR_FALSE;

    mNumItems = 0;
}

//-------------------------------------------------------------------------
//
// nsComboBox:: destructor
//
//-------------------------------------------------------------------------
nsComboBox::~nsComboBox()
{
    PR_LOG(QtWidgetsLM, 
           PR_LOG_DEBUG, 
           ("nsComboBox::~nsComboBox()\n"));
    if (mWidget)
    {
        delete ((QComboBox *)mWidget);
        mWidget = nsnull;
    }
}

//-------------------------------------------------------------------------
//
//  initializer
//
//-------------------------------------------------------------------------

NS_METHOD nsComboBox::SetMultipleSelection(PRBool aMultipleSelections)
{
    PR_LOG(QtWidgetsLM, 
           PR_LOG_DEBUG, 
           ("nsComboBox::SetMultipleSelection()\n"));
    mMultiSelect = aMultipleSelections;
    return NS_OK;
}


//-------------------------------------------------------------------------
//
//  AddItemAt
//
//-------------------------------------------------------------------------

NS_METHOD nsComboBox::AddItemAt(nsString &aItem, PRInt32 aPosition)
{
    NS_ALLOC_STR_BUF(val, aItem, 256);

    PR_LOG(QtWidgetsLM, 
           PR_LOG_DEBUG, 
           ("nsComboBox::AddItemAt: add %s at %d\n", 
            val, 
            aPosition));

    ((QComboBox *)mWidget)->insertItem(val, aPosition);
    mNumItems++;
    NS_FREE_STR_BUF(val);
    return NS_OK;
}

//-------------------------------------------------------------------------
//
//  Finds an item at a postion
//
//-------------------------------------------------------------------------
PRInt32  nsComboBox::FindItem(nsString &aItem, PRInt32 aStartPos)
{
    NS_ALLOC_STR_BUF(val, aItem, 256);
    int i;
    PRInt32 index = -1;

    PR_LOG(QtWidgetsLM, 
           PR_LOG_DEBUG, 
           ("nsComboBox::FindItem: find %s starting at %d\n",
            val,
            aStartPos));

    int count = ((QComboBox *)mWidget)->count();

    for (i = aStartPos; i < count; i++)
    {
        QString string = ((QComboBox*)mWidget)->text(i);

        if (string == val)
        {
            index = i;
            break;
        }
    }

    NS_FREE_STR_BUF(val);
    return index;
}

//-------------------------------------------------------------------------
//
//  CountItems - Get Item Count
//
//-------------------------------------------------------------------------
PRInt32  nsComboBox::GetItemCount()
{
    PR_LOG(QtWidgetsLM, 
           PR_LOG_DEBUG, 
           ("nsComboBox::GetItemCount: returning %d\n",
            mNumItems));
    return (PRInt32)mNumItems;
}

//-------------------------------------------------------------------------
//
//  Removes an Item at a specified location
//
//-------------------------------------------------------------------------
PRBool  nsComboBox::RemoveItemAt(PRInt32 aPosition)
{ 
    PR_LOG(QtWidgetsLM, 
           PR_LOG_DEBUG, 
           ("nsComboBox::RemoveItemAt: remove at %d\n",
            aPosition));
    if (aPosition >= 0 && aPosition < mNumItems) 
    {
        ((QComboBox *)mWidget)->removeItem(aPosition);
        mNumItems--;
        return PR_TRUE;
    }
    else
    {
        return PR_FALSE;
    }
}

//-------------------------------------------------------------------------
//
//  Removes an Item at a specified location
//
//-------------------------------------------------------------------------
PRBool nsComboBox::GetItemAt(nsString& anItem, PRInt32 aPosition)
{
    PR_LOG(QtWidgetsLM, 
           PR_LOG_DEBUG, 
           ("nsComboBox::GetItemAt: get at %d\n",
            aPosition));
    PRBool result = PR_FALSE;
    if (aPosition >= 0 && aPosition < mNumItems) 
    {
        QString string = ((QComboBox *)mWidget)->text(aPosition);
        anItem = NS_ConvertASCIItoUCS2(string).GetUnicode();

        PR_LOG(QtWidgetsLM, 
               PR_LOG_DEBUG, 
               ("nsComboBox::GetItemAt: found %s at %d\n",
                (const char *) string, 
                aPosition));

        result = PR_TRUE;
    }
    else
    {
        result = PR_FALSE;
    }

    return result;
}

//-------------------------------------------------------------------------
//
//  Gets the text of selected item
//
//-------------------------------------------------------------------------
NS_METHOD nsComboBox::GetSelectedItem(nsString& aItem)
{
    QString string = ((QComboBox *)mWidget)->currentText();
    PR_LOG(QtWidgetsLM, 
           PR_LOG_DEBUG, 
           ("nsComboBox::GetSelectedItem: %s is selected\n",
            (const char *) string));

    aItem = NS_ConvertASCIItoUCS2(string).GetUnicode();
    return NS_OK;
}

//-------------------------------------------------------------------------
//
//  Gets the index of the selected item
//
//-------------------------------------------------------------------------
PRInt32 nsComboBox::GetSelectedIndex()
{
    PRInt32 item = ((QComboBox *)mWidget)->currentItem();
    PR_LOG(QtWidgetsLM, 
           PR_LOG_DEBUG, 
           ("nsComboBox::GetSelectedIndex: %d is selected\n",
            item));
    return item;
}

//-------------------------------------------------------------------------
//
//  SelectItem
//
//-------------------------------------------------------------------------
NS_METHOD nsComboBox::SelectItem(PRInt32 aPosition)
{
    PR_LOG(QtWidgetsLM, 
           PR_LOG_DEBUG, 
           ("nsComboBox::SelectItem: select at %d\n",
            aPosition));
    ((QComboBox *) mWidget)->setCurrentItem(aPosition);

    return NS_OK;
}

//-------------------------------------------------------------------------
//
//  GetSelectedCount
//
//-------------------------------------------------------------------------
PRInt32 nsComboBox::GetSelectedCount()
{
    PR_LOG(QtWidgetsLM, 
           PR_LOG_DEBUG, 
           ("nsComboBox::GetSelectedCount()\n"));
    if (!mMultiSelect) 
    {
        PRInt32 inx = GetSelectedIndex();
        return (inx == -1? 0 : 1);
    } 
    else 
    {
        return 0;
    }
}

//-------------------------------------------------------------------------
//
//  GetSelectedIndices
//
//-------------------------------------------------------------------------
NS_METHOD nsComboBox::GetSelectedIndices(PRInt32 aIndices[], PRInt32 aSize)
{
    PR_LOG(QtWidgetsLM, 
           PR_LOG_DEBUG, 
           ("nsComboBox::GetSelectedIndices()\n"));
    // this is an error
    return NS_ERROR_FAILURE;
}

//-------------------------------------------------------------------------
//
//  Deselect
//
//-------------------------------------------------------------------------
NS_METHOD nsComboBox::Deselect()
{
    PR_LOG(QtWidgetsLM, 
           PR_LOG_DEBUG, 
           ("nsComboBox::Deselect()\n"));
    if (mMultiSelect) 
    {
        return NS_ERROR_FAILURE;
    }
    else
    {
        SelectItem(-1);
    }

    return NS_OK;
}
