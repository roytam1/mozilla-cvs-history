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
#ifndef nsTableRowGroup_h__
#define nsTableRowGroup_h__

#include "nscore.h"
#include "nsTableContent.h"

class nsIPresContext;

/**
 * nsTableRowGroup is the content object that represents table row groups 
 * (HTML tags THEAD, TFOOT, and TBODY). This class cannot be reused
 * outside of an nsTablePart.  It assumes that its parent is an nsTableParte, and 
 * its children are nsTableRows.
 * 
 * @see nsTablePart
 * @see nsTableRow
 *
 * @author  sclark
 * TODO: make getter/setters inline
 */
class nsTableRowGroup : public nsTableContent
{

public:

  /** constructor
    * @param aTag  the HTML tag causing this row group to get constructed.
    */
  nsTableRowGroup (nsIAtom* aTag);

  /** constructor
    * @param aTag  the HTML tag causing this row group to get constructed.
    * @param aImplicit  PR_TRUE if there is no actual input tag corresponding to
    *                   this row group.
    */
  nsTableRowGroup (nsIAtom* aTag, PRBool aImplicit);

  /** destructor, not responsible for any memory destruction itself */
  virtual ~nsTableRowGroup();

  /** return the max of the number of columns represented by the contained rows */
  virtual PRInt32 GetMaxColumns();

    // For debugging purposes only
  NS_IMETHOD_(nsrefcnt) AddRef();
  NS_IMETHOD_(nsrefcnt) Release();

  /** @see nsIHTMLContent::CreateFrame */
  virtual nsIFrame* CreateFrame(nsIPresContext* aPresContext,
                                PRInt32 aIndexInParent,
                                nsIFrame* aParentFrame);

  /** return the number of contained rows */
  virtual int GetRowCount ()
  {
    return ChildCount ();
  };

  /** returns nsITableContent::kTableRowGroupType */
  virtual int GetType()
  {
    return nsITableContent::kTableRowGroupType;
  };

  /** notify the containing nsTablePart that cell information has changed */
  virtual void ResetCellMap ();

  /* ----------- overrides from nsTableContent ---------- */

  /** can only append objects that are rows (implement nsITableContent and are .
    * of type nsITableContent::kTableRowType.)
    * @see nsIContent::AppendChild
    */
  virtual PRBool AppendChild (nsIContent * aContent);

  /** can only insert objects that are rows (implement nsITableContent and are .
    * of type nsITableContent::kTableRowType.)
    * @see nsIContent::InsertChildAt
    */  
  virtual PRBool InsertChildAt (nsIContent * aContent, int aIndex);

  /** can only replace child objects with objects that are rows
    * (implement nsITableContent and are * of type nsITableContent::kTableRowe.)
    * @param aContent the object to insert, must be a row
    * @param aIndex   the index of the object to replace.  Must be in the range
    *                 0<=aIndex<ChildCount().
    * @see nsIContent::ReplaceChildAt
    */
  virtual PRBool ReplaceChildAt (nsIContent * aContent, int aIndex);

  /** @see nsIContent::InsertChildAt */
  virtual PRBool RemoveChildAt (int aIndex);

protected:

  virtual PRBool IsRow(nsIContent * aContent) const;

};

#endif

