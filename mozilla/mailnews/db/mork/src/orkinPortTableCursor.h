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

#ifndef _ORKINPORTTABLECURSOR_
#define _ORKINPORTTABLECURSOR_ 1

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

#ifndef _MORKPORTTABLECURSOR_
#include "morkPortTableCursor.h"
#endif

//3456789_123456789_123456789_123456789_123456789_123456789_123456789_123456789

class morkPortTableCursor;
#define morkMagic_kPortTableCursor 0x50744375 /* ascii 'PtCu' */

/*| orkinPortTableCursor: cursor class for iterating port tables
**|
**|| port: the cursor is associated with a specific port, which can be
**| set to a different port (which resets the position to -1 so the
**| next table acquired is the first in the port.
**|
|*/
class orkinPortTableCursor :
  public morkHandle, public nsIMdbPortTableCursor { // nsIMdbCursor

// { ===== begin morkNode interface =====
public: // morkNode virtual methods
  // virtual void CloseMorkNode(morkEnv* ev); // morkHandle is fine
  virtual ~orkinPortTableCursor(); // morkHandle destructor does everything
  
protected: // construction is protected (use the static Make() method)
  orkinPortTableCursor(morkEnv* ev, // note morkUsage is always morkUsage_kPool
    morkHandleFace* ioFace,    // must not be nil, cookie for this handle
    morkPortTableCursor* ioObject); // must not be nil, the object for this handle
    
  // void CloseHandle(morkEnv* ev); // don't need to specialize closing

private: // copying is not allowed
  orkinPortTableCursor(const morkHandle& other);
  orkinPortTableCursor& operator=(const morkHandle& other);

// public: // dynamic type identification
  // mork_bool IsHandle() const //
  // { return IsNode() && mNode_Derived == morkDerived_kHandle; }
// } ===== end morkNode methods =====

protected: // morkHandle memory management operators
  void* operator new(size_t inSize, morkPool& ioPool, morkEnv* ev)
  { return ioPool.NewHandle(ev, inSize); }
  
  void* operator new(size_t inSize, morkHandleFace* ioFace)
  { return ioFace; }
  
  void operator delete(void* ioAddress)
  { morkNode::OnDeleteAssert(ioAddress); }
  // do NOT call delete on morkHandle instances.  They are collected.
  
public: // construction:

  static orkinPortTableCursor* MakePortTableCursor(morkEnv* ev, 
    morkPortTableCursor* ioObject);

public: // utilities:

  morkEnv* CanUsePortTableCursor(nsIMdbEnv* mev, mork_bool inMutable,
    mdb_err* outErr) const;

public: // type identification
  mork_bool IsOrkinPortTableCursor() const
  { return mHandle_Magic == morkMagic_kPortTableCursor; }

  mork_bool IsOrkinPortTableCursorHandle() const
  { return this->IsHandle() && this->IsOrkinPortTableCursor(); }

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

// { ===== begin nsIMdbPortTableCursor methods =====

  // { ----- begin attribute methods -----
  virtual mdb_err SetPort(nsIMdbEnv* ev, nsIMdbPort* ioPort); // sets pos to -1
  virtual mdb_err GetPort(nsIMdbEnv* ev, nsIMdbPort** acqPort);
  
  virtual mdb_err SetRowScope(nsIMdbEnv* ev, // sets pos to -1
    mdb_scope inRowScope);
  virtual mdb_err GetRowScope(nsIMdbEnv* ev, mdb_scope* outRowScope); 
  // setting row scope to zero iterates over all row scopes in port
    
  virtual mdb_err SetTableKind(nsIMdbEnv* ev, // sets pos to -1
    mdb_kind inTableKind);
  virtual mdb_err GetTableKind(nsIMdbEnv* ev, mdb_kind* outTableKind);
  // setting table kind to zero iterates over all table kinds in row scope
  // } ----- end attribute methods -----

  // { ----- begin table iteration methods -----
  virtual mdb_err NextTable( // get table at next position in the db
    nsIMdbEnv* ev, // context
    nsIMdbTable** acqTable); // the next table in the iteration
  // } ----- end table iteration methods -----

// } ===== end nsIMdbPortTableCursor methods =====
};

//3456789_123456789_123456789_123456789_123456789_123456789_123456789_123456789

#endif /* _ORKINPORTTABLECURSOR_ */
