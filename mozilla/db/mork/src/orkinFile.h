/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-  */
/* ----- BEGIN LICENSE BLOCK -----
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape Communications Corporation.
 * Portions created by Netscape Communications Corporation are
 * Copyright (C) 1999 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the LGPL or the GPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ----- END LICENSE BLOCK ----- */

#ifndef _ORKINTABLE_
#define _ORKINTABLE_ 1

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

#ifndef _MORKFILE_
#include "morkFile.h"
#endif

//3456789_123456789_123456789_123456789_123456789_123456789_123456789_123456789

#define morkMagic_kFile 0x46696C65 /* ascii 'File' */

/*| orkinFile: 
|*/
class orkinFile : public morkHandle, public nsIMdbFile { // nsIMdbFile

// { ===== begin morkNode interface =====
public: // morkNode virtual methods
  // virtual void CloseMorkNode(morkEnv* ev); // morkHandle is fine
  virtual ~orkinFile(); // morkHandle destructor does everything
  
protected: // construction is protected (use the static Make() method)
  orkinFile(morkEnv* ev, // note morkUsage is always morkUsage_kPool
    morkHandleFace* ioFace,    // must not be nil, cookie for this handle
    morkFile* ioObject); // must not be nil, the object for this handle
    
  // void CloseHandle(morkEnv* ev); // don't need to specialize closing

private: // copying is not allowed
  orkinFile(const morkHandle& other);
  orkinFile& operator=(const morkHandle& other);

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

  static orkinFile* MakeFile(morkEnv* ev, morkFile* ioObject);

public: // utilities:

  morkEnv* CanUseFile(nsIMdbEnv* mev, mork_bool inMutable,
    mdb_err* outErr) const;

public: // type identification
  mork_bool IsOrkinFile() const
  { return mHandle_Magic == morkMagic_kFile; }

  mork_bool IsOrkinFileHandle() const
  { return this->IsHandle() && this->IsOrkinFile(); }

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

// { ===== begin nsIMdbFile methods =====

  // { ----- begin pos methods -----
  virtual mdb_err Tell(nsIMdbEnv* ev, mdb_pos* outPos);
  virtual mdb_err Seek(nsIMdbEnv* ev, mdb_pos inPos);
  virtual mdb_err Eof(nsIMdbEnv* ev, mdb_pos* outPos);
  // } ----- end pos methods -----

  // { ----- begin read methods -----
  virtual mdb_err Read(nsIMdbEnv* ev, void* outBuf, mdb_size inSize,
    mdb_size* outActualSize);
  virtual mdb_err Get(nsIMdbEnv* ev, void* outBuf, mdb_size inSize,
    mdb_pos inPos, mdb_size* outActualSize);
  // } ----- end read methods -----
    
  // { ----- begin write methods -----
  virtual mdb_err  Write(nsIMdbEnv* ev, const void* inBuf, mdb_size inSize,
    mdb_size* outActualSize);
  virtual mdb_err  Put(nsIMdbEnv* ev, const void* inBuf, mdb_size inSize,
    mdb_pos inPos, mdb_size* outActualSize);
  virtual mdb_err  Flush(nsIMdbEnv* ev);
  // } ----- end attribute methods -----
    
  // { ----- begin path methods -----
  virtual mdb_err  Path(nsIMdbEnv* ev, mdbYarn* outFilePath);
  // } ----- end path methods -----
    
  // { ----- begin replacement methods -----
  virtual mdb_err  Steal(nsIMdbEnv* ev, nsIMdbFile* ioThief);
  virtual mdb_err  Thief(nsIMdbEnv* ev, nsIMdbFile** acqThief);
  // } ----- end replacement methods -----

  // { ----- begin versioning methods -----
  virtual mdb_err BecomeTrunk(nsIMdbEnv* ev);
  // If this file is a file version branch created by calling AcquireBud(),
  // BecomeTrunk() causes this file's content to replace the original
  // file's content, typically by assuming the original file's identity.
  // This default implementation of BecomeTrunk() does nothing, and this
  // is appropriate behavior for files which are not branches, and is
  // also the right behavior for files returned from AcquireBud() which are
  // in fact the original file that has been truncated down to zero length.

  virtual mdb_err AcquireBud(nsIMdbEnv* ev, nsIMdbHeap* ioHeap,
    nsIMdbFile** acqBud); // acquired file for new version of content
  // AcquireBud() starts a new "branch" version of the file, empty of content,
  // so that a new version of the file can be written.  This new file
  // can later be told to BecomeTrunk() the original file, so the branch
  // created by budding the file will replace the original file.  Some
  // file subclasses might initially take the unsafe but expedient
  // approach of simply truncating this file down to zero length, and
  // then returning the same morkFile pointer as this, with an extra
  // reference count increment.  Note that the caller of AcquireBud() is
  // expected to eventually call CutStrongRef() on the returned file
  // in order to release the strong reference.  High quality versions
  // of morkFile subclasses will create entirely new files which later
  // are renamed to become the old file, so that better transactional
  // behavior is exhibited by the file, so crashes protect old files.
  // Note that AcquireBud() is an illegal operation on readonly files.
  // } ----- end versioning methods -----

// } ===== end nsIMdbFile methods =====
};
 
//3456789_123456789_123456789_123456789_123456789_123456789_123456789_123456789

#endif /* _ORKINTABLE_ */
