/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- 
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
 * Copyright (C) 1999 Netscape Communications Corporation.  All Rights 
 * Reserved. 
 */

#ifndef _MDB_
#include "mdb.h"
#endif

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKNODE_
#include "morkNode.h"
#endif

#ifndef _MORKMAP_
#include "morkMap.h"
#endif

#ifndef _MORKENV_
#include "morkEnv.h"
#endif

#ifndef _MORKTABLE_
#include "morkTable.h"
#endif

#ifndef _MORKSTORE_
#include "morkStore.h"
#endif

#ifndef _MORKROWSPACE_
#include "morkRowSpace.h"
#endif

#ifndef _MORKARRAY_
#include "morkArray.h"
#endif

#ifndef _MORKROW_
#include "morkRow.h"
#endif

#ifndef _ORKINTABLE_
#include "orkinTable.h"
#endif

#ifndef _MORKTABLEROWCURSOR_
#include "morkTableRowCursor.h"
#endif

//3456789_123456789_123456789_123456789_123456789_123456789_123456789_123456789

// ````` ````` ````` ````` ````` 
// { ===== begin morkNode interface =====

/*public virtual*/ void
morkTable::CloseMorkNode(morkEnv* ev) /*i*/ // CloseTable() only if open
{
  if ( this->IsOpenNode() )
  {
    this->MarkClosing();
    this->CloseTable(ev);
    this->MarkShut();
  }
}

/*public virtual*/
morkTable::~morkTable() /*i*/ // assert CloseTable() executed earlier
{
  MORK_ASSERT(this->IsShutNode());
  MORK_ASSERT(mTable_Store==0);
  MORK_ASSERT(mTable_RowSpace==0);
}

/*public non-poly*/
morkTable::morkTable(morkEnv* ev, /*i*/
  const morkUsage& inUsage, nsIMdbHeap* ioHeap, 
  morkStore* ioStore, nsIMdbHeap* ioSlotHeap, morkRowSpace* ioRowSpace,
  mork_tid inTid, mork_kind inKind, mork_bool inMustBeUnique)
: morkObject(ev, inUsage, ioHeap, (morkHandle*) 0)
, mTable_Store( 0 )
, mTable_RowSpace( 0 )
, mTable_Id( inTid )
, mTable_RowMap(ev, morkUsage::kMember, (nsIMdbHeap*) 0, ioSlotHeap,
  morkTable_kStartRowMapSlotCount)
, mTable_RowArray(ev, morkUsage::kMember, (nsIMdbHeap*) 0,
  morkTable_kStartRowArraySize, ioSlotHeap)
, mTable_Kind( inKind )
, mTable_MustBeUnique( inMustBeUnique )
{
  if ( ev->Good() )
  {
    if ( ioStore && ioSlotHeap && ioRowSpace )
    {
      if ( inKind )
      {
        morkStore::SlotWeakStore(ioStore, ev, &mTable_Store);
        morkRowSpace::SlotWeakRowSpace(ioRowSpace, ev, &mTable_RowSpace);
        if ( ev->Good() )
          mNode_Derived = morkDerived_kTable;
      }
      else
        ioRowSpace->ZeroKindError(ev);
    }
    else
      ev->NilPointerError();
  }
}

/*public non-poly*/ void
morkTable::CloseTable(morkEnv* ev) /*i*/ // called by CloseMorkNode();
{
  if ( this )
  {
    if ( this->IsNode() )
    {
      mTable_RowMap.CloseMorkNode(ev);
      mTable_RowArray.CloseMorkNode(ev);
      morkStore::SlotWeakStore((morkStore*) 0, ev, &mTable_Store);
      morkRowSpace::SlotStrongRowSpace((morkRowSpace*) 0,
         ev, &mTable_RowSpace);
      this->MarkShut();
    }
    else
      this->NonNodeError(ev);
  }
  else
    ev->NilPointerError();
}

// } ===== end morkNode methods =====
// ````` ````` ````` ````` ````` 

/*static*/ void
morkTable::NonTableTypeError(morkEnv* ev)
{
  ev->NewError("non morkTable");
}

/*static*/ void
morkTable::NonTableTypeWarning(morkEnv* ev)
{
  ev->NewWarning("non morkTable");
}

/*static*/ void
morkTable::NilRowSpaceError(morkEnv* ev)
{
  ev->NewError("nil mTable_RowSpace");
}

void
morkTable::GetTableOid(morkEnv* ev, mdbOid* outOid)
{
  morkRowSpace* space = mTable_RowSpace;
  if ( space )
  {
    outOid->mOid_Scope = space->mSpace_Scope;
    outOid->mOid_Id = mTable_Id;
  }
  else
    this->NilRowSpaceError(ev);
}

nsIMdbTable*
morkTable::AcquireTableHandle(morkEnv* ev)
{
  nsIMdbTable* outTable = 0;
  orkinTable* t = (orkinTable*) mObject_Handle;
  if ( t ) // have an old handle?
    t->AddStrongRef(ev->AsMdbEnv());
  else // need new handle?
  {
    t = orkinTable::MakeTable(ev, this);
    mObject_Handle = t;
  }
  if ( t )
    outTable = t;
  return outTable;
}

mork_pos
morkTable::ArrayHasOid(morkEnv* ev, const mdbOid* inOid)
{
  mork_count count = mTable_RowArray.mArray_Fill;
  mork_pos pos = -1;
  while ( ++pos < count )
  {
    morkRow* row = (morkRow*) mTable_RowArray.At(pos);
    MORK_ASSERT(row);
    if ( row && row->EqualOid(inOid) )
    {
      return pos;
    }
  }
  return -1;
}

mork_pos
morkTable::MapHasOid(morkEnv* ev, const mdbOid* inOid)
{
  morkRow* row = mTable_RowMap.GetOid(ev, inOid);
  if ( row )
    return 1;
    
  return -1;
}

mork_bool
morkTable::AddRow(morkEnv* ev, morkRow* ioRow)
{
  morkRow* row = mTable_RowMap.GetRow(ev, ioRow);
  if ( !row && ev->Good() )
  {
    mork_pos pos = mTable_RowArray.AppendSlot(ev, ioRow);
    if ( ev->Good() && pos > 0 )
    {
      ioRow->AddTableUse(ev);
      if ( mTable_RowMap.AddRow(ev, ioRow) )
      {
        // okay, anything else?
      }
      else
        mTable_RowArray.CutSlot(ev, pos);
    }
  }
  return ev->Good();
}

mork_bool
morkTable::CutRow(morkEnv* ev, morkRow* ioRow)
{
  morkRow* row = mTable_RowMap.GetRow(ev, ioRow);
  if ( row )
  {
    mork_count count = mTable_RowArray.mArray_Fill;
    morkRow** rowSlots = (morkRow**) mTable_RowArray.mArray_Slots;
    if ( rowSlots ) // array has vector as expected?
    {
      mork_pos pos = -1;
      morkRow** end = rowSlots + count;
      morkRow** slot = rowSlots - 1; // prepare for preincrement:
      while ( ++slot < end ) // another slot to check?
      {
        if ( *slot == row ) // found the slot containing row?
        {
          pos = slot - rowSlots; // record absolute position
          break; // end while loop
        }
      }
      if ( pos >= 0 ) // need to cut if from the array?
        mTable_RowArray.CutSlot(ev, pos);
      else
        ev->NewWarning("row not found in array");
    }
    else
      mTable_RowArray.NilSlotsAddressError(ev);
      
    mTable_RowMap.CutRow(ev, ioRow);
    if ( ioRow->CutTableUse(ev) == 0 )
      ioRow->OnZeroTableUse(ev);
  }
  return ev->Good();
}

morkTableRowCursor*
morkTable::NewTableRowCursor(morkEnv* ev, mork_pos inRowPos)
{
  morkTableRowCursor* outCursor = 0;
  if ( ev->Good() )
  {
    nsIMdbHeap* heap = mTable_Store->mPort_Heap;
    morkTableRowCursor* cursor = new(*heap, ev)
      morkTableRowCursor(ev, morkUsage::kHeap, heap, this, inRowPos);
    if ( cursor )
    {
      if ( ev->Good() )
        outCursor = cursor;
      else
        cursor->CutStrongRef(ev);
    }
  }
  return outCursor;
}

//3456789_123456789_123456789_123456789_123456789_123456789_123456789_123456789


morkTableMap::~morkTableMap()
{
}

morkTableMap::morkTableMap(morkEnv* ev, const morkUsage& inUsage,
  nsIMdbHeap* ioHeap, nsIMdbHeap* ioSlotHeap)
  : morkNodeMap(ev, inUsage, ioHeap, ioSlotHeap)
{
  if ( ev->Good() )
    mNode_Derived = morkDerived_kTableMap;
}


//3456789_123456789_123456789_123456789_123456789_123456789_123456789_123456789
