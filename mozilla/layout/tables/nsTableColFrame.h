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
#ifndef nsTableColFrame_h__
#define nsTableColFrame_h__

#include "nscore.h"
#include "nsContainerFrame.h"

class nsTableColFrame : public nsFrame {
public:

  enum {eWIDTH_SOURCE_NONE          =0,   // no cell has contributed to the width style
        eWIDTH_SOURCE_CELL          =1,   // a cell specified a width
        eWIDTH_SOURCE_CELL_WITH_SPAN=2    // a cell implicitly specified a width via colspan
  };

  void Init(PRInt32 aColIndex, PRInt32 aRepeat);

  static nsresult NewFrame(nsIFrame** aInstancePtrResult,
                           nsIContent* aContent,
                           nsIFrame*   aParent);

  NS_IMETHOD Paint(nsIPresContext& aPresContext,
                   nsIRenderingContext& aRenderingContext,
                   const nsRect& aDirtyRect);

  NS_IMETHOD Reflow(nsIPresContext*      aPresContext,
                    nsReflowMetrics&     aDesiredSize,
                    const nsReflowState& aReflowState,
                    nsReflowStatus&      aStatus);

  /** return the index of the column the col represents.  always >= 0 */
  virtual int GetColumnIndex ();

  /** return the number of the columns the col represents.  always >= 0 */
  virtual int GetRepeat ();

  /** set the index of the column this content object represents.  must be >= 0 */
  virtual void SetColumnIndex (int aColIndex);

  /** convenience method, calls into cellmap */
  nsVoidArray * GetCells();

  nscoord GetMaxColWidth();
  void SetMaxColWidth(nscoord aMaxColWidth);

  nscoord GetMinColWidth();
  void SetMinColWidth(nscoord aMinColWidth);

  nscoord GetEffectiveMaxColWidth();
  void SetEffectiveMaxColWidth(nscoord aMaxColWidth);

  nscoord GetEffectiveMinColWidth();
  void SetEffectiveMinColWidth(nscoord aMinColWidth);

  nscoord GetAdjustedMinColWidth();
  void SetAdjustedMinColWidth(nscoord aMinColWidth);

  PRInt32 GetWidthSource();
  void SetWidthSource(PRInt32 aMinColWidth);

  /** convenience method, calls into cellmap */
  PRInt32 Count() const;

  NS_IMETHOD  List(FILE* out = stdout, PRInt32 aIndent = 0, nsIListFilter *aFilter = nsnull) const;

protected:

  nsTableColFrame(nsIContent* aContent, nsIFrame* aParentFrame);

  ~nsTableColFrame();

  /** the starting index of the column (starting at 0) that this col object represents */
  PRInt32  mColIndex;

  /** the number of columns that the attributes of this column extend to */
  PRInt32  mRepeat;

  nscoord mMaxColWidth;
  nscoord mMinColWidth;

  nscoord mMaxEffectiveColWidth;
  nscoord mMinEffectiveColWidth;

  nscoord mMinAdjustedColWidth;

  PRInt32 mWidthSource;

};


inline void nsTableColFrame::Init(PRInt32 aColIndex, PRInt32 aRepeat)
{
  NS_ASSERTION(0<=aColIndex, "bad col index param");
  NS_ASSERTION(0<=aRepeat, "bad repeat param");

  mColIndex = aColIndex;
  mRepeat = aRepeat;
}

inline nsTableColFrame::GetColumnIndex()
{ return mColIndex; }

inline nsTableColFrame::GetRepeat()
{ return mRepeat; }
  
inline void nsTableColFrame::SetColumnIndex (int aColIndex)
{  mColIndex = aColIndex;}

inline nscoord nsTableColFrame::GetMaxColWidth()
{ return mMaxColWidth; }

inline void nsTableColFrame::SetMaxColWidth(nscoord aMaxColWidth)
{ mMaxColWidth = aMaxColWidth; }

inline nscoord nsTableColFrame::GetMinColWidth()
{ return mMinColWidth; }

inline void nsTableColFrame::SetMinColWidth(nscoord aMinColWidth)
{ mMinColWidth = aMinColWidth; }

inline nscoord nsTableColFrame::GetEffectiveMaxColWidth()
{ return mMaxEffectiveColWidth; }

inline void nsTableColFrame::SetEffectiveMaxColWidth(nscoord aMaxColWidth)
{ mMaxEffectiveColWidth = aMaxColWidth; }

inline nscoord nsTableColFrame::GetEffectiveMinColWidth()
{ return mMinEffectiveColWidth; }

inline void nsTableColFrame::SetEffectiveMinColWidth(nscoord aMinEffectiveColWidth)
{ mMinEffectiveColWidth = aMinEffectiveColWidth; }

inline nscoord nsTableColFrame::GetAdjustedMinColWidth()
{ return mMinAdjustedColWidth; }

inline void nsTableColFrame::SetAdjustedMinColWidth(nscoord aMinAdjustedColWidth)
{ mMinAdjustedColWidth = aMinAdjustedColWidth; }

inline PRInt32 nsTableColFrame::GetWidthSource()
{ return mWidthSource; }

inline void nsTableColFrame::SetWidthSource(PRInt32 aWidthSource)
{ mWidthSource = aWidthSource; }
#endif

