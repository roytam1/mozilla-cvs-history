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

#ifndef _MORKSPACE_
#define _MORKSPACE_ 1

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKNODE_
#include "morkNode.h"
#endif

#ifndef _MORKMAP_
#include "morkMap.h"
#endif

//3456789_123456789_123456789_123456789_123456789_123456789_123456789_123456789

#define morkSpace_kInitialSpaceSlots  /*i*/ 1024 /* default */
#define morkDerived_kSpace  /*i*/ 0x5370 /* ascii 'Sp' */

/*| morkSpace:
|*/
class morkSpace : public morkNode { // 

// public: // slots inherited from morkNode (meant to inform only)
  // nsIMdbHeap*       mNode_Heap;

  // mork_base      mNode_Base;     // must equal morkBase_kNode
  // mork_derived   mNode_Derived;  // depends on specific node subclass
  
  // mork_access    mNode_Access;   // kOpen, kClosing, kShut, or kDead
  // mork_usage     mNode_Usage;    // kHeap, kStack, kMember, kGlobal, kNone
  // mork_able      mNode_Mutable;  // can this node be modified?
  // mork_load      mNode_Load;     // is this node clean or dirty?
  
  // mork_uses      mNode_Uses;     // refcount for strong refs
  // mork_refs      mNode_Refs;     // refcount for strong refs + weak refs

public: // state is public because the entire Mork system is private

  morkStore*  mSpace_Store; // weak ref to containing store
  
  mork_scope  mSpace_Scope; // the scope for this space
  
  mork_bool   mSpace_DoAutoIDs;    // whether db should assign member IDs
  mork_bool   mSpace_HaveDoneAutoIDs; // whether actually auto assigned IDs
  mork_u1     mSpace_Pad[ 2 ];     // pad to u4 alignment

// { ===== begin morkNode interface =====
public: // morkNode virtual methods
  virtual void CloseMorkNode(morkEnv* ev); // CloseSpace() only if open
  virtual ~morkSpace(); // assert that CloseSpace() executed earlier
  
public: // morkMap construction & destruction
  //morkSpace(morkEnv* ev, const morkUsage& inUsage, nsIMdbHeap* ioNodeHeap,
  //  const morkMapForm& inForm, nsIMdbHeap* ioSlotHeap);
  
  morkSpace(morkEnv* ev, const morkUsage& inUsage,mork_scope inScope, 
    morkStore* ioStore, nsIMdbHeap* ioNodeHeap, nsIMdbHeap* ioSlotHeap);
  void CloseSpace(morkEnv* ev); // called by CloseMorkNode();

public: // dynamic type identification
  mork_bool IsSpace() const
  { return IsNode() && mNode_Derived == morkDerived_kSpace; }
// } ===== end morkNode methods =====

public: // other space methods

  static void NonAsciiSpaceScopeName(morkEnv* ev);

  morkPool* GetSpaceStorePool() const;

public: // typesafe refcounting inlines calling inherited morkNode methods
  static void SlotWeakSpace(morkSpace* me,
    morkEnv* ev, morkSpace** ioSlot)
  { morkNode::SlotWeakNode((morkNode*) me, ev, (morkNode**) ioSlot); }
  
  static void SlotStrongSpace(morkSpace* me,
    morkEnv* ev, morkSpace** ioSlot)
  { morkNode::SlotStrongNode((morkNode*) me, ev, (morkNode**) ioSlot); }
};

//3456789_123456789_123456789_123456789_123456789_123456789_123456789_123456789

#endif /* _MORKSPACE_ */
