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
 * Copyright (C) 1999 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#ifndef _ORKINTABLEROWCURSOR_
#define _ORKINTABLEROWCURSOR_ 1

#ifndef _MDB_
#include "mdb.h"
#endif

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKNODE_
#include "morkNode.h"
#endif

#ifndef _MORKHANDLE_
#include "morkHandle.h"
#endif

#ifndef _MORKTABLEROWCURSOR_
#include "morkTableRowCursor.h"
#endif

//3456789_123456789_123456789_123456789_123456789_123456789_123456789_123456789

class morkTableRowCursor;
#define morkMagic_kTableRowCursor 0x54724375 /* ascii 'TrCu' */

/*| orkinTableRowCursor:
|*/
class orkinTableRowCursor :
  public morkHandle, public nsIMdbTableRowCursor { // nsIMdbCursor

// { ===== begin morkNode interface =====
public: // morkNode virtual methods
  // virtual void CloseMorkNode(morkEnv* ev); // morkHandle is fine
  virtual ~orkinTableRowCursor(); // morkHandle destructor does everything
  
protected: // construction is protected (use the static Make() method)
  orkinTableRowCursor(morkEnv* ev, // note morkUsage is always morkUsage_kPool
    morkHandleFace* ioFace,    // must not be nil, cookie for this handle
    morkTableRowCursor* ioObject); // must not be nil, object for this handle
    
  // void CloseHandle(morkEnv* ev); // don't need to specialize closing

private: // copying is not allowed
  orkinTableRowCursor(const morkHandle& other);
  orkinTableRowCursor& operator=(const morkHandle& other);

// public: // dynamic type identification
  // mork_bool IsHandle() const //
  // { return IsNode() && mNode_Derived == morkDerived_kHandle; }
// } ===== end morkNode methods =====

protected: // morkHandle memory management operators
  void* operator new(size_t inSize, morkPool& ioPool, morkZone& ioZone, morkEnv* ev)
  { return ioPool.NewHandle(ev, inSize, &ioZone); }
  
  void* operator new(size_t inSize, morkPool& ioPool, morkEnv* ev)
  { return ioPool.NewHandle(ev, inSize, (morkZone*) 0); }
  
  void* operator new(size_t inSize, morkHandleFace* ioFace)
  { MORK_USED_1(inSize); return ioFace; }
  
  void operator delete(void* ioAddress)
  { morkNode::OnDeleteAssert(ioAddress); }
  // do NOT call delete on morkHandle instances.  They are collected.
  
public: // construction:

  static orkinTableRowCursor* MakeTableRowCursor(morkEnv* ev, 
    morkTableRowCursor* ioObject);

public: // utilities:

  morkEnv* CanUseTableRowCursor(nsIMdbEnv* mev, mork_bool inMutable,
    mdb_err* outErr) const;

public: // type identification
  mork_bool IsOrkinTableRowCursor() const
  { return mHandle_Magic == morkMagic_kTableRowCursor; }

  mork_bool IsOrkinTableRowCursorHandle() const
  { return this->IsHandle() && this->IsOrkinTableRowCursor(); }

// { ===== begin nsIMdbISupports methods =====
  virtual mdb_err AddRef(); // add strong ref with no
  virtual mdb_err Release(); // cut strong ref
// } ===== end nsIMdbObject methods =====

// { ===== begin nsIMdbObject methods =====

  // { ----- begin attribute methods -----
  virtual mdb_err IsFrozenMdbObject(nsIMdbEnv* ev, mdb_bool* outIsReadonly);
  // same as nsIMdbPort::GetIsPortReadonly() when this object is inside a port.
  // } ----- end attribute methods -----

  // { ----- begin factory methods -----
  virtual mdb_err GetMdbFactory(nsIMdbEnv* ev, nsIMdbFactory** acqFactory); 
  // } ----- end factory methods -----

  // { ----- begin ref counting for well-behaved cyclic graphs -----
  virtual mdb_err GetWeakRefCount(nsIMdbEnv* ev, // weak refs
    mdb_count* outCount);  
  virtual mdb_err GetStrongRefCount(nsIMdbEnv* ev, // strong refs
    mdb_count* outCount);

  virtual mdb_err AddWeakRef(nsIMdbEnv* ev);
  virtual mdb_err AddStrongRef(nsIMdbEnv* ev);

  virtual mdb_err CutWeakRef(nsIMdbEnv* ev);
  virtual mdb_err CutStrongRef(nsIMdbEnv* ev);
  
  virtual mdb_err CloseMdbObject(nsIMdbEnv* ev); // called at strong refs zero
  virtual mdb_err IsOpenMdbObject(nsIMdbEnv* ev, mdb_bool* outOpen);
  // } ----- end ref counting -----
  
// } ===== end nsIMdbObject methods =====

// { ===== begin nsIMdbCursor methods =====

  // { ----- begin attribute methods -----
  virtual mdb_err GetCount(nsIMdbEnv* ev, mdb_count* outCount); // readonly
  virtual mdb_err GetSeed(nsIMdbEnv* ev, mdb_seed* outSeed);    // readonly
  
  virtual mdb_err SetPos(nsIMdbEnv* ev, mdb_pos inPos);   // mutable
  virtual mdb_err GetPos(nsIMdbEnv* ev, mdb_pos* outPos);
  
  virtual mdb_err SetDoFailOnSeedOutOfSync(nsIMdbEnv* ev, mdb_bool inFail);
  virtual mdb_err GetDoFailOnSeedOutOfSync(nsIMdbEnv* ev, mdb_bool* outFail);
  // } ----- end attribute methods -----

// } ===== end nsIMdbCursor methods =====

// { ===== begin nsIMdbTableRowCursor methods =====

  // { ----- begin attribute methods -----
  virtual mdb_err GetTable(nsIMdbEnv* ev, nsIMdbTable** acqTable);
  // } ----- end attribute methods -----

  // { ----- begin oid iteration methods -----
  virtual mdb_err NextRowOid( // get row id of next row in the table
    nsIMdbEnv* ev, // context
    mdbOid* outOid, // out row oid
    mdb_pos* outRowPos);    // zero-based position of the row in table
  // } ----- end oid iteration methods -----

  // { ----- begin row iteration methods -----
  virtual mdb_err NextRow( // get row cells from table for cells already in row
    nsIMdbEnv* ev, // context
    nsIMdbRow** acqRow, // acquire next row in table
    mdb_pos* outRowPos);    // zero-based position of the row in table
  // } ----- end row iteration methods -----

  // { ----- begin duplicate row removal methods -----
  virtual mdb_err CanHaveDupRowMembers(nsIMdbEnv* ev, // cursor might hold dups?
    mdb_bool* outCanHaveDups);
    
  virtual mdb_err MakeUniqueCursor( // clone cursor, removing duplicate rows
    nsIMdbEnv* ev, // context
    nsIMdbTableRowCursor** acqCursor);    // acquire clone with no dups
    // Note that MakeUniqueCursor() is never necessary for a cursor which was
    // created by table method nsIMdbTable::GetTableRowCursor(), because a table
    // never contains the same row as a member more than once.  However, a cursor
    // created by table method nsIMdbTable::FindRowMatches() might contain the
    // same row more than once, because the same row can generate a hit by more
    // than one column with a matching string prefix.  Note this method can
    // return the very same cursor instance with just an incremented refcount,
    // when the original cursor could not contain any duplicate rows (calling
    // CanHaveDupRowMembers() shows this case on a false return).  Otherwise
    // this method returns a different cursor instance.  Callers should not use
    // this MakeUniqueCursor() method lightly, because it tends to defeat the
    // purpose of lazy programming techniques, since it can force creation of
    // an explicit row collection in a new cursor's representation, in order to
    // inspect the row membership and remove any duplicates; this can have big
    // impact if a collection holds tens of thousands of rows or more, when
    // the original cursor with dups simply referenced rows indirectly by row
    // position ranges, without using an explicit row set representation.
    // Callers are encouraged to use nsIMdbCursor::GetCount() to determine
    // whether the row collection is very large (tens of thousands), and to
    // delay calling MakeUniqueCursor() when possible, until a user interface
    // element actually demands the creation of an explicit set representation.
  // } ----- end duplicate row removal methods -----

// } ===== end nsIMdbTableRowCursor methods =====
};

//3456789_123456789_123456789_123456789_123456789_123456789_123456789_123456789

#endif /* _ORKINTABLEROWCURSOR_ */
