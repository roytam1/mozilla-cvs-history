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
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#include "nsDiskCacheRecord.h"
#include "nsINetDataDiskCache.h"
#include "nsNetDiskCacheCID.h"
#include "nsDiskCacheRecordChannel.h"
#include "nsFileStream.h"

#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsIProtocolHandler.h"
#include "nsIIOService.h"
#include "nsIAllocator.h"

#include "plstr.h"
#include "prprf.h"
#include "prmem.h"
#include "prlog.h"
#include "prtypes.h"
#include "netCore.h"

#include "nsDBAccessor.h"

#if !defined(IS_LITTLE_ENDIAN) && !defined(IS_BIG_ENDIAN)
ERROR! Must have a byte order
#endif

#ifdef IS_LITTLE_ENDIAN
#define COPY_INT32(_a,_b)  memcpy(_a, _b, sizeof(int32))
#else
#define COPY_INT32(_a,_b)  /* swap */                   \
    do {                                                \
    ((char *)(_a))[0] = ((char *)(_b))[3];              \
    ((char *)(_a))[1] = ((char *)(_b))[2];              \
    ((char *)(_a))[2] = ((char *)(_b))[1];              \
    ((char *)(_a))[3] = ((char *)(_b))[0];              \
    } while(0)
#endif

nsDiskCacheRecord::nsDiskCacheRecord(nsIDBAccessor* db, nsNetDiskCache* aCache) :
  mKey(0) ,
  mKeyLength(0) ,
  mRecordID(0) ,
  mMetaData(0) ,
  mMetaDataLength(0) ,
  mDB(db) ,
  mInfo(0) ,
  mInfoSize(0) ,
  mDiskCache(aCache) 
{

  NS_INIT_REFCNT();
}

// mem alloced. so caller should do free() on key. 
NS_IMETHODIMP
nsDiskCacheRecord::Init(const char* key, PRUint32 length) 
{
  NS_NewFileSpec(getter_AddRefs(mFile));
  if(!mFile)
    return NS_ERROR_OUT_OF_MEMORY ;

  // copy key
  mKeyLength = length ;
  mKey = NS_STATIC_CAST(char*, nsAllocator::Alloc(mKeyLength*sizeof(char))) ;
  if(!mKey) 
    return NS_ERROR_OUT_OF_MEMORY ;

  memcpy(mKey, key, length) ;

  // get RecordID
  mDB->GetID(key, length, &mRecordID) ;

  // setup the file name
  nsCOMPtr<nsIFileSpec> dbFolder ;
  mDiskCache->GetDiskCacheFolder(getter_AddRefs(dbFolder)) ;

  nsresult rv = mFile->FromFileSpec(dbFolder) ;
  if(NS_FAILED(rv))
    return NS_ERROR_FAILURE ;

  // dir is a hash result of mRecordID%32, hope it's enough 
  char filename[9], dirName[3] ;
  PR_snprintf(dirName, 3, "%.2x", (((PRUint32)mRecordID) % 32)) ;
  mFile->AppendRelativeUnixPath(dirName) ;

  PR_snprintf(filename, 9, "%.8x", mRecordID) ;
  mFile->AppendRelativeUnixPath(filename) ;

  return NS_OK ;
}

nsDiskCacheRecord::~nsDiskCacheRecord()
{
//  printf(" ~nsDiskCacheRecord()\n") ;
  if(mKey)
    nsAllocator::Free(mKey) ;
  if(mMetaData)
    nsAllocator::Free(mMetaData) ;

}

//
// Implement nsISupports methods
//
NS_IMPL_ISUPPORTS(nsDiskCacheRecord, NS_GET_IID(nsINetDataCacheRecord))

///////////////////////////////////////////////////////////////////////
// nsINetDataCacheRecord methods

// yes, mem alloced on *_retval.
NS_IMETHODIMP
nsDiskCacheRecord::GetKey(PRUint32 *length, char** _retval) 
{
  if(!_retval)
    return NS_ERROR_NULL_POINTER ;

  *length = mKeyLength ;
  *_retval = NS_STATIC_CAST(char*, nsAllocator::Alloc(mKeyLength*sizeof(char))) ;
  if(!*_retval)
    return NS_ERROR_OUT_OF_MEMORY ;

  memcpy(*_retval, mKey, mKeyLength) ;

  return NS_OK ;
}

NS_IMETHODIMP
nsDiskCacheRecord::GetRecordID(PRInt32* aRecordID)
{
  *aRecordID = mRecordID ;
  return NS_OK ;
}

// yes, mem alloced on *_retval. 
NS_IMETHODIMP
nsDiskCacheRecord::GetMetaData(PRUint32 *length, char **_retval) 
{
  if(!_retval)
    return NS_ERROR_NULL_POINTER ;

  // always null the return value first. 
  *_retval = nsnull ;

  *length = mMetaDataLength ;

  if(mMetaDataLength) {
    *_retval = NS_STATIC_CAST(char*, nsAllocator::Alloc(mMetaDataLength*sizeof(char))) ;
    if(!*_retval)
      return NS_ERROR_OUT_OF_MEMORY ;

    memcpy(*_retval, mMetaData, mMetaDataLength) ;
  }

  return NS_OK ;
}

NS_IMETHODIMP
nsDiskCacheRecord::SetMetaData(PRUint32 length, const char* data)
{
  // set the mMetaData
  mMetaDataLength = length ;
  if(mMetaData)
    nsAllocator::Free(mMetaData) ;
  mMetaData = NS_STATIC_CAST(char*, nsAllocator::Alloc(mMetaDataLength*sizeof(char))) ;
  if(!mMetaData) {
    return NS_ERROR_OUT_OF_MEMORY ;
  }
  memcpy(mMetaData, data, length) ;

  // Generate mInfo
  nsresult rv = GenInfo() ;
  if(NS_FAILED(rv))
    return rv ;

  // write through into mDB
  rv = mDB->Put(mRecordID, mInfo, mInfoSize) ;
  return rv ;
}

NS_IMETHODIMP
nsDiskCacheRecord::GetStoredContentLength(PRUint32 *aStoredContentLength)
{
  return mFile->GetFileSize(aStoredContentLength) ;
}

// untill nsIFileSpec::Truncate() is in, we have to do all this ugly stuff
NS_IMETHODIMP
nsDiskCacheRecord::SetStoredContentLength(PRUint32 aStoredContentLength)
{
  PRUint32 len = 0 ;
  nsresult rv = mFile->GetFileSize(&len) ;
  if(NS_FAILED(rv))
    return rv ;

  if(len < aStoredContentLength)
  {
    NS_ERROR("Error: can not set filesize to something bigger than itself.\n") ;
    return NS_ERROR_FAILURE ;
  }
  else {
    nsCOMPtr<nsIFileSpec> newfile;
    NS_NewFileSpec(getter_AddRefs(newfile)) ;

    char *newname, *oldname=nsnull ;
    rv = mFile->GetLeafName(&oldname) ; // save the old file name 
    if(!oldname)
      return NS_ERROR_FAILURE ;

    newfile->FromFileSpec(mFile) ;
    newfile->MakeUnique() ; // generate a unique new file name
    newfile->GetLeafName(&newname) ;

    mFile->Rename(newname) ; // rename the old file 
    mFile->SetLeafName(oldname) ;  // get the old name back

    newfile->OpenStreamForReading() ;
    mFile->OpenStreamForWriting() ;

    PRUint32 buffer_size = 1024 ;
    char buffer[1024], *p_buf ;
    PRInt32 result ;
    PRUint32 size_left = aStoredContentLength, size_written = 0 ;
    p_buf = buffer ;

    do {
      if(size_left > buffer_size) 
        size_written = buffer_size ;
      else
        size_written = size_left ;

      rv = newfile->Read(&p_buf, size_written, &result) ;
      if(NS_FAILED(rv) || result != NS_STATIC_CAST(PRInt32, size_written)) 
        return NS_ERROR_FAILURE ;

      rv = mFile->Write(buffer, size_written, &result) ;
      if(NS_FAILED(rv) || result != NS_STATIC_CAST(PRInt32, size_written))
        return NS_ERROR_FAILURE ;

      size_left -= size_written ;
    } while(size_left) ;

    mFile->CloseStream() ;
    newfile->CloseStream() ;

    nsFileSpec extra_file ;
    newfile->GetFileSpec(&extra_file) ;
    extra_file.Delete(PR_TRUE) ;

    return NS_OK ;
  }
}

NS_IMETHODIMP
nsDiskCacheRecord::Delete(void)
{
  if(mNumChannels)
    return NS_ERROR_NOT_AVAILABLE ;

  PRUint32 len ;
  mFile->GetFileSize(&len) ;

  nsFileSpec cache_file ;
  nsresult rv = mFile->GetFileSpec(&cache_file) ;

  if(NS_FAILED(rv)) 
    return NS_ERROR_FAILURE ;

  cache_file.Delete(PR_TRUE) ;

  // updata the storage size
  mDiskCache->m_StorageInUse -= len ;

  rv = mDB->Del(mRecordID, mKey, mKeyLength) ;
  if(NS_FAILED(rv)) 
    return NS_ERROR_FAILURE ;
  else
    return NS_OK ;
}

NS_IMETHODIMP
nsDiskCacheRecord::GetFilename(nsIFileSpec * *aFilename) 
{
  if(!aFilename)
    return NS_ERROR_NULL_POINTER ;

  *aFilename = mFile ;
  NS_ADDREF(*aFilename) ;

  return NS_OK ;
}

NS_IMETHODIMP
nsDiskCacheRecord::NewChannel(nsILoadGroup *loadGroup, nsIChannel **_retval) 
{
  nsDiskCacheRecordChannel* channel = new nsDiskCacheRecordChannel(this, loadGroup) ;
  if(!channel) 
    return NS_ERROR_OUT_OF_MEMORY ;

  nsresult rv = channel->Init() ;
  if(NS_FAILED(rv))
    return rv ;

  NS_ADDREF(channel) ;
  *_retval = NS_STATIC_CAST(nsIChannel*, channel) ;
  return NS_OK ;
}

//////////////////////////////////////////////////////////////////////////
// nsDiskCacheRecord methods

// file name is represented by a url string. I hope this would be more
// generic
nsresult
nsDiskCacheRecord::GenInfo() 
{
  if(mInfo)
    nsAllocator::Free(mInfo) ;

  char* file_url=nsnull ;
  PRUint32 name_len ;
  mFile->GetURLString(&file_url) ;
  name_len = PL_strlen(file_url)+1 ;

  mInfoSize  = sizeof(PRUint32) ;    // checksum for mInfoSize
  mInfoSize += sizeof(PRInt32) ;     // RecordID
  mInfoSize += sizeof(PRUint32) ;    // key length
  mInfoSize += mKeyLength ;          // key 
  mInfoSize += sizeof(PRUint32) ;    // metadata length
  mInfoSize += mMetaDataLength ;     // metadata 
  mInfoSize += sizeof(PRUint32) ;    // filename length
  mInfoSize += name_len ;            // filename 

  void* newInfo = nsAllocator::Alloc(mInfoSize*sizeof(char)) ;
  if(!newInfo) {
    return NS_ERROR_OUT_OF_MEMORY ;
  }

  // copy the checksum mInfoSize
  char* cur_ptr = NS_STATIC_CAST(char*, newInfo) ;
  COPY_INT32(cur_ptr, &mInfoSize) ;
  cur_ptr += sizeof(PRUint32) ;

  // copy RecordID
  COPY_INT32(cur_ptr, &mRecordID) ;
  cur_ptr += sizeof(PRInt32) ;

  // copy key length
  COPY_INT32(cur_ptr, &mKeyLength) ;
  cur_ptr += sizeof(PRUint32) ;

  // copy key
  memcpy(cur_ptr, mKey, mKeyLength) ;
  cur_ptr += mKeyLength ;

  // copy metadata length
  COPY_INT32(cur_ptr, &mMetaDataLength) ;
  cur_ptr += sizeof(PRUint32) ;

  // copy metadata
  memcpy(cur_ptr, mMetaData, mMetaDataLength) ;
  cur_ptr += mMetaDataLength ;

  // copy file name length 
  COPY_INT32(cur_ptr, &name_len) ;
  cur_ptr += sizeof(PRUint32) ;

  // copy file name
  memcpy(cur_ptr, file_url, name_len) ;
  cur_ptr += name_len ;

  PR_ASSERT(cur_ptr == NS_STATIC_CAST(char*, newInfo) + mInfoSize);
  mInfo = newInfo ;

  return NS_OK ;
}

/*
 * This Method suppose to get all the info from the db record
 * and set them to accroding members. the original values
 * will all be overwritten. only minimal error checking is performed.
 */
NS_IMETHODIMP
nsDiskCacheRecord::RetrieveInfo(void* aInfo, PRUint32 aInfoLength) 
{
  // reset everything
  if(mInfo) {
    nsAllocator::Free(mInfo) ;
    mInfo = nsnull ;
  }

  if(mKey) {
    nsAllocator::Free(mKey) ;
    mKey = nsnull ;
  }
  if(mMetaData) {
    nsAllocator::Free(mMetaData) ;
    mMetaData = nsnull ;
  }

  char * cur_ptr = NS_STATIC_CAST(char*, aInfo) ;

  char* file_url ;
  PRUint32 name_len ;

  // set mInfoSize 
  COPY_INT32(&mInfoSize, cur_ptr) ;
  cur_ptr += sizeof(PRUint32) ;

  // check this at least
  if(mInfoSize != aInfoLength) 
    return NS_ERROR_FAILURE ;

  // set mRecordID 
  COPY_INT32(&mRecordID, cur_ptr) ;
  cur_ptr += sizeof(PRInt32) ;

  // set mKeyLength
  COPY_INT32(&mKeyLength, cur_ptr) ;
  cur_ptr += sizeof(PRUint32) ;

  // set mKey
  mKey = NS_STATIC_CAST(char*, nsAllocator::Alloc(mKeyLength*sizeof(char))) ;
  if(!mKey) 
    return NS_ERROR_OUT_OF_MEMORY ;
  
  memcpy(mKey, cur_ptr, mKeyLength) ;
  cur_ptr += mKeyLength ;

  PRInt32 id ;
  mDB->GetID(mKey, mKeyLength, &id) ;
  NS_ASSERTION(id==mRecordID, "\t ++++++ bad record, somethings wrong\n") ;

  // set mMetaDataLength
  COPY_INT32(&mMetaDataLength, cur_ptr) ;
  cur_ptr += sizeof(PRUint32) ;

  // set mMetaData
  mMetaData = NS_STATIC_CAST(char*, nsAllocator::Alloc(mMetaDataLength*sizeof(char))) ;
  if(!mMetaData) 
    return NS_ERROR_OUT_OF_MEMORY ;
  
  memcpy(mMetaData, cur_ptr, mMetaDataLength) ;
  cur_ptr += mMetaDataLength ;

  // get mFile name length
  COPY_INT32(&name_len, cur_ptr) ;
  cur_ptr += sizeof(PRUint32) ;

  // get mFile native name
  file_url = NS_STATIC_CAST(char*, nsAllocator::Alloc(name_len*sizeof(char))) ;
  if(!file_url) 
    return NS_ERROR_OUT_OF_MEMORY ;
  
  memcpy(file_url, cur_ptr, name_len) ;
  cur_ptr += name_len ;

  PR_ASSERT(cur_ptr == NS_STATIC_CAST(char*, aInfo) + mInfoSize);

  // create mFile if Init() isn't called 
  if(!mFile) {
    NS_NewFileSpec(getter_AddRefs(mFile));
    if(!mFile)
      return NS_ERROR_OUT_OF_MEMORY ;
  }

  // setup mFile
  mFile->SetURLString(file_url) ;

  return NS_OK ;
}

