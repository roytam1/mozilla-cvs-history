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

#ifndef _MORKFACTORY_
#define _MORKFACTORY_ 1

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKENV_
#include "morkEnv.h"
#endif

#ifndef _MORKOBJECT_
#include "morkObject.h"
#endif

#ifndef _ORKINHEAP_
#include "orkinHeap.h"
#endif

//3456789_123456789_123456789_123456789_123456789_123456789_123456789_123456789

class nsIMdbFactory;

#define morkDerived_kFactory  /*i*/ 0x4663 /* ascii 'Fc' */
#define morkFactory_kWeakRefCountBonus 0 /* try NOT to leak all factories */

/*| morkFactory: 
|*/
class morkFactory : public morkObject { // nsIMdbObject

// public: // slots inherited from morkObject (meant to inform only)
  // nsIMdbHeap*     mNode_Heap;
  // mork_able    mNode_Mutable; // can this node be modified?
  // mork_load    mNode_Load;    // is this node clean or dirty?
  // mork_base    mNode_Base;    // must equal morkBase_kNode
  // mork_derived mNode_Derived; // depends on specific node subclass
  // mork_access  mNode_Access;  // kOpen, kClosing, kShut, or kDead
  // mork_usage   mNode_Usage;   // kHeap, kStack, kMember, kGlobal, kNone
  // mork_uses    mNode_Uses;    // refcount for strong refs
  // mork_refs    mNode_Refs;    // refcount for strong refs + weak refs

  // mork_color   mBead_Color;   // ID for this bead
  // morkHandle*  mObject_Handle;  // weak ref to handle for this object

public: // state is public because the entire Mork system is private

  morkEnv        mFactory_Env; // private env instance used internally
  orkinHeap      mFactory_Heap;
  
// { ===== begin morkNode interface =====
public: // morkFactory virtual methods
  virtual void CloseMorkNode(morkEnv* ev); // CloseFactory() only if open
  virtual ~morkFactory(); // assert that CloseFactory() executed earlier
  
public: // morkYarn construction & destruction
  morkFactory(); // uses orkinHeap
  morkFactory(nsIMdbHeap* ioHeap); // caller supplied heap
  morkFactory(morkEnv* ev, const morkUsage& inUsage, nsIMdbHeap* ioHeap);
  void CloseFactory(morkEnv* ev); // called by CloseMorkNode();
  
  
public: // morkNode memory management operators
  void* operator new(size_t inSize)
  { return ::operator new(inSize); }
  
  void* operator new(size_t inSize, nsIMdbHeap& ioHeap, morkEnv* ev)
  { return morkNode::MakeNew(inSize, ioHeap, ev); }
  
  void operator delete(void* ioAddress)
  { morkNode::OnDeleteAssert(ioAddress); }
  // do NOT call delete on morkNode instances.  Call ZapOld() instead.

private: // copying is not allowed
  morkFactory(const morkFactory& other);
  morkFactory& operator=(const morkFactory& other);

public: // dynamic type identification
  mork_bool IsFactory() const
  { return IsNode() && mNode_Derived == morkDerived_kFactory; }
// } ===== end morkNode methods =====

public: // other factory methods

  nsIMdbFactory* AcquireFactoryHandle(morkEnv* ev); // mObject_Handle
  
  void NonFactoryTypeError(morkEnv* ev);
  
public: // typesafe refcounting inlines calling inherited morkNode methods
  static void SlotWeakFactory(morkFactory* me,
    morkEnv* ev, morkFactory** ioSlot)
  { morkNode::SlotWeakNode((morkNode*) me, ev, (morkNode**) ioSlot); }
  
  static void SlotStrongFactory(morkFactory* me,
    morkEnv* ev, morkFactory** ioSlot)
  { morkNode::SlotStrongNode((morkNode*) me, ev, (morkNode**) ioSlot); }
};

//3456789_123456789_123456789_123456789_123456789_123456789_123456789_123456789

#endif /* _MORKFACTORY_ */
