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

#ifndef _MORKSTREAM_
#define _MORKSTREAM_ 1

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKNODE_
#include "morkNode.h"
#endif

#ifndef _MORKFILE_
#include "morkFile.h"
#endif

//3456789_123456789_123456789_123456789_123456789_123456789_123456789_123456789

/*=============================================================================
 * morkStream: buffered file i/o
 */

/*| morkStream exists to define an morkFile subclass that provides buffered
**| i/o for an underlying content file.  Naturally this arrangement only makes
**| sense when the underlying content file is itself not efficiently buffered
**| (especially for character by character i/o).
**|
**|| morkStream is intended for either reading use or writing use, but not
**| both simultaneously or interleaved.  Pick one when the stream is created
**| and don't change your mind.  This restriction is intended to avoid obscure
**| and complex bugs that might arise from interleaved reads and writes -- so
**| just don't do it.  A stream is either a sink or a source, but not both.
**|
**|| (When the underlying content file is intended to support both reading and
**| writing, a developer might use two instances of morkStream where one is for
**| reading and the other is for writing.  In this case, a developer must take
**| care to keep the two streams in sync because each will maintain a separate
**| buffer representing a cache consistency problem for the other.  A simple
**| approach is to invalidate the buffer of one when one uses the other, with
**| the assumption that closely mixed reading and writing is not expected, so
**| that little cost is associated with changing read/write streaming modes.)
**|
**|| Exactly one of mStream_ReadEnd or mStream_WriteEnd must be a null pointer,
**| and this will cause the right thing to occur when inlines use them, because
**| mStream_At < mStream_WriteEnd (for example) will always be false and the
**| else branch of the statement calls a function that raises an appropriate
**| error to complain about either reading a sink or writing a source.
**|
**|| morkStream is a direct clone of mork_Stream from Communicator 4.5's
**| address book code, which in turn was based on the stream class in the
**| public domain Mithril programming language.
|*/

#define morkStream_kPrintBufSize /*i*/ 512 /* buffer size used by printf() */ 

#define morkStream_kMinBufSize /*i*/ 512 /* buffer no fewer bytes */ 
#define morkStream_kMaxBufSize /*i*/ (32 * 1024) /* buffer no more bytes */ 

#define morkDerived_kStream     /*i*/ 0x7A74 /* ascii 'zt' */

class morkStream /*d*/ : public morkFile { /* from Mithril's AgStream class */

// ````` ````` ````` `````   ````` ````` ````` `````  
protected: // protected morkStream members
  mork_u1*    mStream_At;       // pointer into mStream_Buf
  mork_u1*    mStream_ReadEnd;  // null or one byte past last readable byte
  mork_u1*    mStream_WriteEnd; // null or mStream_Buf + mStream_BufSize

  morkFile*   mStream_ContentFile;  // where content is read and written

  mork_u1*    mStream_Buf;      // dynamically allocated memory to buffer io
  mork_size   mStream_BufSize;  // requested buf size (fixed by min and max)
  mork_pos    mStream_BufPos;   // logical position of byte at mStream_Buf
  mork_bool   mStream_Dirty;    // does the buffer need to be flushed?
  mork_bool   mStream_HitEof;   // has eof been reached? (only frozen streams)
  
// { ===== begin morkNode interface =====
public: // morkNode virtual methods
  virtual void CloseMorkNode(morkEnv* ev); // CloseStream() only if open
  virtual ~morkStream(); // assert that CloseStream() executed earlier
  
public: // morkStream construction & destruction
  morkStream(morkEnv* ev, const morkUsage& inUsage, nsIMdbHeap* ioHeap,
      morkFile* ioContentFile, mork_size inBufSize, mork_bool inFrozen);
  void CloseStream(morkEnv* ev); // called by CloseMorkNode();

private: // copying is not allowed
  morkStream(const morkStream& other);
  morkStream& operator=(const morkStream& other);

public: // dynamic type identification
  mork_bool IsStream() const
  { return IsNode() && mNode_Derived == morkDerived_kStream; }
// } ===== end morkNode methods =====

public: // typing
  void NonStreamTypeError(morkEnv* ev);

// ````` ````` ````` `````   ````` ````` ````` `````  
public: // virtual morkFile methods

  virtual void BecomeTrunk(morkEnv* ev);
  // If this file is a file version branch created by calling AcquireBud(),
  // BecomeTrunk() causes this file's content to replace the original
  // file's content, typically by assuming the original file's identity.

  virtual morkFile*  AcquireBud(morkEnv* ev, nsIMdbHeap* ioHeap);
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
  
  virtual mork_pos   Length(morkEnv* ev) const; // eof
  virtual mork_pos   Tell(morkEnv* ev) const;
  virtual mork_size  Read(morkEnv* ev, void* outBuf, mork_size inSize);
  virtual mork_pos   Seek(morkEnv* ev, mork_pos inPos);
  virtual mork_size  Write(morkEnv* ev, const void* inBuf, mork_size inSize);
  virtual void       Flush(morkEnv* ev);
    
// ````` ````` ````` `````   ````` ````` ````` `````  
protected: // protected non-poly morkStream methods (for char io)

  int     fill_getc(morkEnv* ev);
  void    spill_putc(morkEnv* ev, int c);
  void    spill_buf(morkEnv* ev); // spill/flush from buffer to file
      
// ````` ````` ````` `````   ````` ````` ````` `````  
public: // public non-poly morkStream methods
    
  void NewBadCursorSlotsError(morkEnv* ev) const;
  void NewBadCursorOrderError(morkEnv* ev) const;
  void NewNullStreamBufferError(morkEnv* ev) const;
  void NewCantReadSinkError(morkEnv* ev) const;
  void NewCantWriteSourceError(morkEnv* ev) const;
  void NewPosBeyondEofError(morkEnv* ev) const;
      
  morkFile*  GetStreamContentFile() const { return mStream_ContentFile; }
  mork_size   GetStreamBufferSize() const { return mStream_BufSize; }
  
  void    PutString(morkEnv* ev, const char* inString);
  void    PutStringThenNewline(morkEnv* ev, const char* inString);
  void    PutStringThenNewlineThenSpace(morkEnv* ev, const char* inString);
  void    PutByteThenNewline(morkEnv* ev, int inByte);
  void    PutByteThenNewlineThenSpace(morkEnv* ev, int inByte);
  
  // ````` ````` stdio type methods ````` ````` 
  void    Printf(morkEnv* ev, const char* inFormat, ...);
  
  mork_pos  Ftell(morkEnv* ev) /*i*/ { return this->Tell(ev); }
  
  void    Fsetpos(morkEnv* ev, mork_pos inPos) /*i*/ { this->Seek(ev, inPos); }
  
  void    Rewind(morkEnv* ev) /*i*/ { this->Fsetpos(ev, 0); }
  
  void    Fflush(morkEnv* ev) /*i*/ { this->Flush(ev); }
  
  mork_bool Feof() /*i*/ { return mStream_HitEof; }
    // Feof() is never true for writable stream output sinks 
  
  void    Puts(morkEnv* ev, const char* inString) /*i*/
  { this->PutString(ev, inString); }

  void    Ungetc(int c) /*i*/
  { if ( mStream_At > mStream_Buf && c > 0 ) *--mStream_At = c; }
  
  int     Getc(morkEnv* ev) /*i*/
  { return ( mStream_At < mStream_ReadEnd )? *mStream_At++ : fill_getc(ev); }
  
  void    Putc(morkEnv* ev, int c) /*i*/
  { 
    mStream_Dirty = morkBool_kTrue;
    if ( mStream_At < mStream_WriteEnd )
      *mStream_At++ = c;
    else
      spill_putc(ev, c);
  }
  
public: // typesafe refcounting inlines calling inherited morkNode methods
  static void SlotWeakStream(morkStream* me,
    morkEnv* ev, morkStream** ioSlot)
  { morkNode::SlotWeakNode((morkNode*) me, ev, (morkNode**) ioSlot); }
  
  static void SlotStrongStream(morkStream* me,
    morkEnv* ev, morkStream** ioSlot)
  { morkNode::SlotStrongNode((morkNode*) me, ev, (morkNode**) ioSlot); }
};


//3456789_123456789_123456789_123456789_123456789_123456789_123456789_123456789

#endif /* _MORKSTREAM_ */
