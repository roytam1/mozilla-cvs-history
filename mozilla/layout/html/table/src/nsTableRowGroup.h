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

  // For debugging purposes only
  NS_IMETHOD_(nsrefcnt) AddRef();
  NS_IMETHOD_(nsrefcnt) Release();


  NS_IMETHOD SetAttribute(nsIAtom* aAttribute, const nsString& aValue,
                          PRBool aNotify);

  NS_IMETHOD GetAttributeMappingFunction(nsMapAttributesFunc& aMapFunc) const;

  /** @see nsIHTMLContent::CreateFrame */
  NS_IMETHOD CreateFrame(nsIPresContext*  aPresContext,
                         nsIFrame*        aParentFrame,
                         nsIStyleContext* aStyleContext,
                         nsIFrame*&       aResult);

  /** returns nsITableContent::kTableRowGroupType */
  virtual int GetType();

  /* ----------- overrides from nsTableContent ---------- */

  /** can only append objects that are rows (implement nsITableContent and are .
    * of type nsITableContent::kTableRowType.)
    * @see nsIContent::AppendChildTo
    */
  NS_IMETHOD AppendChildTo(nsIContent* aKid, PRBool aNotify);

  /** can only insert objects that are rows (implement nsITableContent and are .
    * of type nsITableContent::kTableRowType.)
    * @see nsIContent::InsertChildAt
    */  
  NS_IMETHOD InsertChildAt(nsIContent* aKid, PRInt32 aIndex, PRBool aNotify);

  /** can only replace child objects with objects that are rows
    * (implement nsITableContent and are * of type nsITableContent::kTableRowe.)
    * @param aContent the object to insert, must be a row
    * @param aIndex   the index of the object to replace.  Must be in the range
    *                 0<=aIndex<ChildCount().
    * @see nsIContent::ReplaceChildAt
    */
  NS_IMETHOD ReplaceChildAt(nsIContent* aKid, PRInt32 aIndex, PRBool aNotify);

  /** @see nsIContent::InsertChildAt */
  NS_IMETHOD RemoveChildAt(PRInt32 aIndex, PRBool aNotify);

protected:

  virtual PRBool IsRow(nsIContent * aContent) const;
  PRBool IsTableCell(nsIContent * aContent) const;

};


// XXX this is a virtual method so inlining just causes code
// duplication. don't do this!

/** returns nsITableContent::kTableRowGroupType */
inline int nsTableRowGroup::GetType()
{
  return nsITableContent::kTableRowGroupType;
}

#endif

