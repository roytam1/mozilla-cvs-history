/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
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
 * The Original Code is Mozilla Communicator client code, 
 * released March 31, 1998. 
 *
 * The Initial Developer of the Original Code is Netscape Communications 
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998-1999 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *     Daniel Veditz <dveditz@netscape.com>
 *     Samir Gehani <sgehani@netscape.com>
 *     Mitch Stoltz <mstoltz@netscape.com>
 */

#ifndef nsZipArchive_h_
#define nsZipArchive_h_

#define ZIP_MAGIC     0x5A49505FL   /* "ZIP_" */
#define ZIPFIND_MAGIC 0x5A495046L   /* "ZIPF" */
#define ZIP_TABSIZE   256
// Keep this odd. The -1 is significant.
#define ZIP_BUFLEN    (4 * 1024 - 1)

#ifdef STANDALONE
#define nsZipArchive nsZipArchiveStandalone

#define ZIP_Seek(fd,p,m) (fseek((fd),(p),(m))==0)
#else

#define PL_ARENA_CONST_ALIGN_MASK 7
#include "plarena.h"
#define ZIP_Seek(fd,p,m) (PR_Seek((fd),((PROffset32)p),(m))==((PROffset32)p))
#endif

#define ZIFLAG_SYMLINK      0x01  /* zip item is a symlink */
#define ZIFLAG_DATAOFFSET   0x02  /* zip item offset points to file data */

class nsZipFind;
class nsZipRead;
class nsZipItemMetadata;

/**
 * nsZipItem -- a helper class for nsZipArchive
 *
 * each nsZipItem represents one file in the archive and all the
 * information needed to manipulate it.
 */
class nsZipItem
{
public:
  char*       name; /* '\0' terminated */

  PRUint32    offset;
  PRUint32    size;
  PRUint32    realsize;
  PRUint32    crc32;

  nsZipItem*  next;

  /*
   * Keep small items together, to avoid overhead.
   */
  PRUint16    mode;
  PRUint16    time;
  PRUint16    date;

  /*
   * Keep small items together, to avoid overhead.
   */
  PRUint8      compression;
  PRUint8      flags;

  /**
   * GetModTime
   *
   * Utility to get an NSPR-friendly formatted string
   * representing the last modified time of this item.
   * 
   * @return nsprstr    an NSPR-friendly string representation 
   *                    of the modification time
   */
  char *GetModTime();

  nsZipItem();
  ~nsZipItem();


private:
  //-- prevent copies and assignments
  nsZipItem& operator=(const nsZipItem& rhs);
  nsZipItem(const nsZipItem& rhs);

};


/** 
 * nsZipArchive -- a class for reading the PKZIP file format.
 *
 */
class nsZipArchive 
{
public:
  /** cookie used to validate supposed objects passed from C code */
  const PRInt32 kMagic;

  /** Block size by which Arena grows. Arena used by filelists */
  const PRUint32 kArenaBlockSize;

  /** constructing does not open the archive. See OpenArchive() */
  nsZipArchive();

  /** destructing the object closes the archive */
  ~nsZipArchive();

  /** 
   * OpenArchive 
   * 
   * It's an error to call this more than once on the same nsZipArchive
   * object. If we were allowed to use exceptions this would have been 
   * part of the constructor 
   *
   * @param   aArchiveName  full pathname of archive
   * @return  status code
   */
  PRInt32 OpenArchive(const char * aArchiveName);

  PRInt32 OpenArchiveWithFileDesc(PRFileDesc* fd);

  /**
   * Test the integrity of items in this archive by running
   * a CRC check after extracting each item into a memory 
   * buffer.  If an entry name is supplied only the 
   * specified item is tested.  Else, if null is supplied
   * then all the items in the archive are tested.
   *
   * @return  status code       
   */
  PRInt32 Test(const char *aEntryName);

  /**
   * Closes an open archive.
   */
  PRInt32 CloseArchive();

  /** 
   * GetItem
   *
   * @param   aFilename Name of file in the archive
   * @return  status code
   */  
  PRInt32 GetItem(const char * aFilename, nsZipItem** result);
  
  /** 
   * ReadInit
   * 
   * Prepare to read from an item in the archive. Must be called
   * before any calls to Read or Available
   *
   * @param   zipEntry name of item in file
   * @param   aRead is filled with appropriate values
   * @return  status code
   */
  PRInt32 ReadInit(const char* zipEntry, nsZipRead* aRead);

  /** 
   * Read 
   * 
   * Read from the item specified to ReadInit. ReadInit must be 
   * called first.
   *
   * @param  aRead the structure returned by ReadInit
   * @param  buf buffer to write data into.
   * @param  count number of bytes to read
   * @param  actual (out) number of bytes read
   * @return  status code
   */
  PRInt32 Read(nsZipRead* aRead, char* buf, PRUint32 count, PRUint32* actual );

  /**
   * Available
   *
   * Returns the number of bytes left to be read from the
   * item specified to ReadInit. ReadInit must be called first.
   *
   * @param aRead the structure returned by ReadInit
   * @return the number of bytes still to be read
   */
   PRUint32 Available(nsZipRead* aRead);

  /**
   * ExtractFile 
   *
   * @param   zipEntry  name of file in archive to extract
   * @param   aOutname   where to extract on disk
   * @return  status code
   */
  PRInt32 ExtractFile( const char * zipEntry, const char * aOutname );

  PRInt32 ExtractFileToFileDesc(const char * zipEntry, PRFileDesc* outFD,
                                nsZipItem **outItem);

  /**
   * FindInit
   *
   * Initializes a search for files in the archive. FindNext() returns
   * the actual matches. FindFree() must be called when you're done
   *
   * @param   aPattern    a string or RegExp pattern to search for
   *                      (may be NULL to find all files in archive)
   * @return  a structure used in FindNext. NULL indicates error
   */
  nsZipFind* FindInit( const char * aPattern );

  /**
   * FindNext
   *
   * @param   aFind   the Find structure returned by FindInit
   * @param   aResult the next item that matches the pattern
   */
  PRInt32 FindNext( nsZipFind* aFind, nsZipItem** aResult);

  PRInt32 FindFree( nsZipFind *aFind );

#ifdef XP_UNIX
  /**
   * ResolveSymLinks
   * @param path      where the file is located
   * @param zipItem   the zipItem related to "path" 
   */
  PRInt32  ResolveSymlink(const char *path, nsZipItem *zipItem);
#endif

private:
  //--- private members ---

  PRFileDesc    *mFd;
  nsZipItem*    mFiles[ZIP_TABSIZE];
#ifndef STANDALONE
  PLArenaPool   mArena;
#endif

  //--- private methods ---
  
  nsZipArchive& operator=(const nsZipArchive& rhs); // prevent assignments
  nsZipArchive(const nsZipArchive& rhs);            // prevent copies

  PRInt32           BuildFileList();
  nsZipItem*        GetFileItem( const char * zipEntry );
  PRUint32          HashName( const char* aName );

  PRInt32           SeekToItem(const nsZipItem* aItem);
  PRInt32           CopyItemToBuffer(const nsZipItem* aItem, char* aBuf);
  PRInt32           CopyItemToDisk( const nsZipItem* aItem, PRFileDesc* outFD );
  PRInt32           InflateItem( const nsZipItem* aItem, 
                                 PRFileDesc* outFD,
                                 char* buf );
  PRInt32           TestItem( const nsZipItem *aItem );

};

/** 
 * nsZipRead
 *
 * a helper class for nsZipArchive, representing a read in progress
 */
class nsZipRead
{
public:

  nsZipRead() { MOZ_COUNT_CTOR(nsZipRead); }
  ~nsZipRead()
  {
    PR_FREEIF(mFileBuffer);
    MOZ_COUNT_DTOR(nsZipRead);
  }

  void Init( nsZipArchive* aZip, nsZipItem* aZipItem )
  {
    mArchive = aZip;
    mItem = aZipItem;
    mCurPos = 0;
    mFileBuffer = NULL;
  }

  nsZipArchive* mArchive;
  nsZipItem*    mItem;
  PRUint32      mCurPos;
  char*         mFileBuffer;

private:
  //-- prevent copies and assignments
  nsZipRead& operator=(const nsZipRead& rhs);
  nsZipRead(const nsZipFind& rhs);
};

/** 
 * nsZipFind 
 *
 * a helper class for nsZipArchive, representing a search
 */
class nsZipFind
{
  friend class nsZipArchive;

public:
  const PRInt32       kMagic;

  nsZipFind( nsZipArchive* aZip, char* aPattern, PRBool regExp );
  ~nsZipFind();

  nsZipArchive* GetArchive();

private:
  nsZipArchive* mArchive;
  char*         mPattern;
  PRUint16      mSlot;
  nsZipItem*    mItem;
  PRBool        mRegExp;

  //-- prevent copies and assignments
  nsZipFind& operator=(const nsZipFind& rhs);
  nsZipFind(const nsZipFind& rhs);
};

#endif /* nsZipArchive_h_ */
