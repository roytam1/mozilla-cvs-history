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
#ifndef nsSpaceManager_h___
#define nsSpaceManager_h___

#include "nsISpaceManager.h"
#include "prclist.h"
#include "plhash.h"

/**
 * Implementation of nsISpaceManager that maintains a region data structure of
 * unavailable space
 */
class nsSpaceManager : public nsISpaceManager {
public:
  nsSpaceManager(nsIFrame* aFrame);

  // nsISupports
  NS_DECL_ISUPPORTS

  // nsISpaceManager
  NS_IMETHOD GetFrame(nsIFrame*& aFrame) const;

  NS_IMETHOD Translate(nscoord aDx, nscoord aDy);
  NS_IMETHOD GetTranslation(nscoord& aX, nscoord& aY) const;
  NS_IMETHOD YMost(nscoord& aYMost) const;

  NS_IMETHOD GetBandData(nscoord       aYOffset,
                         const nsSize& aMaxSize,
                         nsBandData&   aBandData) const;

  NS_IMETHOD AddRectRegion(nsIFrame*     aFrame,
                           const nsRect& aUnavailableSpace);
  NS_IMETHOD ResizeRectRegion(nsIFrame*    aFrame,
                              nscoord      aDeltaWidth,
                              nscoord      aDeltaHeight,
                              AffectedEdge aEdge);
  NS_IMETHOD OffsetRegion(nsIFrame* aFrame, nscoord aDx, nscoord aDy);
  NS_IMETHOD RemoveRegion(nsIFrame* aFrame);

  NS_IMETHOD ClearRegions();

protected:
  // Structure that maintains information about the region associated
  // with a particular frame
  struct FrameInfo {
    nsIFrame* const mFrame;
    nsRect          mRect;       // rectangular region

    FrameInfo(nsIFrame* aFrame, const nsRect& aRect);
  };

public:
  // Doubly linked list of band rects
  struct BandRect : PRCListStr {
    nscoord   mLeft, mTop;
    nscoord   mRight, mBottom;
    PRIntn    mNumFrames;    // number of frames occupying this rect
    union {
      nsIFrame*    mFrame;   // single frame occupying the space
      nsVoidArray* mFrames;  // list of frames occupying the space
    };

    BandRect(nscoord aLeft, nscoord aTop,
             nscoord aRight, nscoord aBottom,
             nsIFrame*);
    BandRect(nscoord aLeft, nscoord aTop,
             nscoord aRight, nscoord aBottom,
             nsVoidArray*);
    ~BandRect();

    // List operations
    BandRect* Next() const {return (BandRect*)PR_NEXT_LINK(this);}
    BandRect* Prev() const {return (BandRect*)PR_PREV_LINK(this);}
    void      InsertBefore(BandRect* aBandRect) {PR_INSERT_BEFORE(aBandRect, this);}
    void      InsertAfter(BandRect* aBandRect) {PR_INSERT_AFTER(aBandRect, this);}
    void      Remove() {PR_REMOVE_LINK(this);}

    // Split the band rect into two vertically, with this band rect becoming
    // the top part, and a new band rect being allocated and returned for the
    // bottom part
    //
    // Does not insert the new band rect into the linked list
    BandRect* SplitVertically(nscoord aBottom);

    // Split the band rect into two horizontally, with this band rect becoming
    // the left part, and a new band rect being allocated and returned for the
    // right part
    //
    // Does not insert the new band rect into the linked list
    BandRect* SplitHorizontally(nscoord aRight);

    // Accessor functions
    PRBool  IsOccupiedBy(const nsIFrame*) const;
    void    AddFrame(const nsIFrame*);
    void    RemoveFrame(const nsIFrame*);
    PRBool  HasSameFrameList(const BandRect* aBandRect) const;
    PRInt32 Length() const;
  };

  // Circular linked list of band rects
  struct BandList : BandRect {
    BandList();

    // Accessors
    PRBool    IsEmpty() const {return PR_CLIST_IS_EMPTY((PRCListStr*)this);}
    BandRect* Head() const {return (BandRect*)PR_LIST_HEAD(this);}
    BandRect* Tail() const {return (BandRect*)PR_LIST_TAIL(this);}

    // Operations
    void      Append(BandRect* aBandRect) {PR_APPEND_LINK(aBandRect, this);}

    // Remove and delete all the band rects in the list
    void      Clear();
  };

protected:
  nsIFrame* const mFrame;     // frame associated with the space manager
  nscoord         mX, mY;     // translation from local to global coordinate space
  BandList        mBandList;  // header/sentinel for circular linked list of band rects
  PLHashTable*    mFrameInfoMap;

protected:
  virtual ~nsSpaceManager();
  FrameInfo* GetFrameInfoFor(nsIFrame* aFrame);
  FrameInfo* CreateFrameInfo(nsIFrame* aFrame, const nsRect& aRect);
  void       DestroyFrameInfo(FrameInfo*);

  void       ClearFrameInfo();
  void       ClearBandRects();

  BandRect*  GetNextBand(const BandRect* aBandRect) const;
  void       DivideBand(BandRect* aBand, nscoord aBottom);
  PRBool     CanJoinBands(BandRect* aBand, BandRect* aPrevBand);
  PRBool     JoinBands(BandRect* aBand, BandRect* aPrevBand);
  void       AddRectToBand(BandRect* aBand, BandRect* aBandRect);
  void       InsertBandRect(BandRect* aBandRect);

  nsresult   GetBandAvailableSpace(const BandRect* aBand,
                                   nscoord         aY,
                                   const nsSize&   aMaxSize,
                                   nsBandData&     aAvailableSpace) const;

private:
	nsSpaceManager(const nsSpaceManager&);  // no implementation
	void operator=(const nsSpaceManager&);  // no implementation
  friend PR_CALLBACK PRIntn NS_RemoveFrameInfoEntries(PLHashEntry*, PRIntn, void*);
};

/* prototypes */
PR_CALLBACK PLHashNumber
NS_HashNumber(const void* key);

#endif /* nsSpaceManager_h___ */

