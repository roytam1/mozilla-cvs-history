/*
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
 * The Original Code is Mozilla Communicator.
 * 
 * The Initial Developer of the Original Code is Intel Corp.
 * Portions created by Intel Corp. are
 * Copyright (C) 1999, 1999 Intel Corp.  All
 * Rights Reserved.
 * 
 * Contributor(s): Yixiong Zou <yixiong.zou@intel.com>
 *                 Carl Wong <carl.wong@intel.com>
 */

/*
 * This file is part of filecache implementation.
 *
 * nsNetDiskCache is the main disk cache module that will create 
 * the cache database, and then store and retrieve nsDiskCacheRecord 
 * objects from it. It also contains some basic error recovery procedure.
 */

#ifndef __gen_nsNetDiskCache_h__
#define __gen_nsNetDiskCache_h__

#include "nsINetDataDiskCache.h"
#include "nsNetDiskCacheCID.h"
#include "nsCOMPtr.h"
#include "nsDBAccessor.h"

class nsIURI; /* forward decl */
class nsICachedNetData; /* forward decl */
class nsISimpleEnumerator; /* forward decl */
class nsIFile; /* forward decl */

/* starting interface:    nsNetDiskCache */

class nsNetDiskCache : public nsINetDataDiskCache {
  public: 

  NS_DECL_ISUPPORTS
  NS_DECL_NSINETDATACACHE
  NS_DECL_NSINETDATADISKCACHE

  NS_IMETHOD Init(void) ;

  nsNetDiskCache() ;
  virtual ~nsNetDiskCache() ;

  protected:

  NS_IMETHOD InitDB(void) ;
  NS_IMETHOD CreateDir(nsIFile* dir_spec) ;
  NS_IMETHOD GetSizeEntry(void) ;
  NS_IMETHOD SetSizeEntry(void) ;

  NS_IMETHOD RenameCacheSubDirs(void) ;
  NS_IMETHOD DBRecovery(void) ;

  private:
  nsresult InitPrefs();
  NS_IMETHODIMP InitCacheFolder();
		
  PRBool 			                mEnabled ;
  PRUint32 			                mNumEntries ;
  nsCOMPtr<nsINetDataCache> 	    mpNextCache ;
  nsCOMPtr<nsIFile>   	        	mDiskCacheFolder ;
  nsCOMPtr<nsIFile>            		mDBFile ;

  PRUint32                          mMaxEntries ;
  PRUint32                          mStorageInUse ;
  nsIDBAccessor*                    mDB ;

  // this is used to indicate a db corruption 
  PRBool                            mDBCorrupted ;

  friend class nsDiskCacheRecord ;
  friend class nsDiskCacheRecordChannel ;
  friend class nsDBEnumerator ;
} ;

#endif /* __gen_nsNetDiskCache_h__ */
