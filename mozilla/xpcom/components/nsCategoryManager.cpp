/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2000
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Scott Collins <scc@netscape.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or 
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#define PL_ARENA_CONST_ALIGN_MASK 7

#include "nsICategoryManager.h"
#include "nsCategoryManager.h"

#include "nsCOMPtr.h"
#include "nsHashtable.h"
#include "nsIFactory.h"
#include "nsSupportsPrimitives.h"
#include "nsIObserver.h"
#include "nsComponentManager.h"
#include "nsReadableUtils.h"
#include "nsCRT.h"

#include "nsHashtableEnumerator.h"
#include "nsEnumeratorUtils.h"


  /*
    CategoryDatabase
      contains 0 or more 1-1 mappings of string to Category
        each Category contains 0 or more 1-1 mappings of string keys to string values

    In other words, the CategoryDatabase is a tree, whose root is a hashtable.
    Internal nodes (or Categories) are hashtables.
    Leaf nodes are strings.
  */

// this function is not public yet, hence it is externed here.
extern nsresult NS_GetComponentLoaderManager(nsIComponentLoaderManager* *result);

#define NS_CATEGORYMANAGER_ARENA_SIZE (1024 * 8)

// pulled in from nsComponentManager.cpp
char* ArenaStrndup(const char* s, PRUint32 len, PLArenaPool* aArena);
char* ArenaStrdup(const char* s, PLArenaPool* aArena);

static
NS_IMETHODIMP
ExtractKeyString( nsHashKey* key, void*, void*, nsISupports** _retval )
  /*
    ...works with |nsHashtableEnumerator| to make the hash keys enumerable.
  */
{
  nsresult status = NS_ERROR_FAILURE;

  nsCOMPtr<nsISupportsCString> obj = new nsSupportsCStringImpl();
  if ( obj ) {
    nsCStringKey* strkey = NS_STATIC_CAST(nsCStringKey*, key);
    status = obj->SetData(nsDependentCString(strkey->GetString(),
                                             strkey->GetStringLength()));
  }

  *_retval = obj;
  NS_IF_ADDREF(*_retval);
  return status;
}



class CategoryNode
    : public nsObjectHashtable
  {
    public:
      CategoryNode()
          : nsObjectHashtable((nsHashtableCloneElementFunc) 0, 0,
                                (nsHashtableEnumFunc) 0, 0 )
        {
          // Nothing else to do here...
        }

      const char* find_leaf( const char* );
  };

const char*
CategoryNode::find_leaf( const char* aLeafName )
  {
    nsCStringKey leafNameKey(aLeafName);
    return NS_STATIC_CAST(char*, Get(&leafNameKey));
  }

  /*
    We keep a hashtable of hashtables, therefore, we need a suitable
    destruction function to register with the outer table for destroying
    elements which are the inner tables.
  */
static
PRBool
Destroy_CategoryNode( nsHashKey*, void* aElement, void* )
  {
    delete NS_STATIC_CAST(CategoryNode*, aElement);
    return PR_TRUE;
  }







class nsCategoryManager
    : public nsICategoryManager,
      public nsObjectHashtable
  {
    private:
      friend class nsCategoryManagerFactory;
      nsCategoryManager();

    public:
        virtual ~nsCategoryManager();

      NS_DECL_ISUPPORTS
      NS_DECL_NSICATEGORYMANAGER

    private:
      CategoryNode* find_category( const char* );

    PLArenaPool mArena;
  };

NS_IMPL_ISUPPORTS1(nsCategoryManager, nsICategoryManager)

nsCategoryManager::nsCategoryManager()
    : nsObjectHashtable((nsHashtableCloneElementFunc) 0, 0,
                                (nsHashtableEnumFunc) Destroy_CategoryNode, 0 )

  {
    PL_INIT_ARENA_POOL(&mArena, "CategoryManagerArena",
                       NS_CATEGORYMANAGER_ARENA_SIZE);
  }

nsCategoryManager::~nsCategoryManager()
  {
    PL_FinishArenaPool(&mArena);
  }

CategoryNode*
nsCategoryManager::find_category( const char* aCategoryName )
  {
    nsCStringKey categoryNameKey(aCategoryName);
    return NS_STATIC_CAST(CategoryNode*, Get(&categoryNameKey));
  }

NS_IMETHODIMP
nsCategoryManager::GetCategoryEntry( const char *aCategoryName,
                                     const char *aEntryName,
                                     char **_retval )
  {
    NS_ASSERTION(aCategoryName, "aCategoryName is NULL!");
    NS_ASSERTION(aEntryName,    "aEntryName is NULL!");
    NS_ASSERTION(_retval,       "_retval is NULL!");

    nsresult status = NS_ERROR_NOT_AVAILABLE;
    CategoryNode* category = find_category(aCategoryName);
    if (category) 
      {
        nsCStringKey entryKey(aEntryName);
        const char* entry = NS_STATIC_CAST(char*, category->Get(&entryKey));
        if (entry)
          status = (*_retval = nsCRT::strdup(entry)) ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
      }

    return status;
  }

NS_IMETHODIMP
nsCategoryManager::AddCategoryEntry( const char *aCategoryName,
                                     const char *aEntryName,
                                     const char *aValue,
                                     PRBool aPersist,
                                     PRBool aReplace,
                                     char **_retval )
  {
    NS_ASSERTION(aCategoryName, "aCategoryName is NULL!");
    NS_ASSERTION(aEntryName,    "aEntryName is NULL!");
    NS_ASSERTION(aValue,        "aValue is NULL!");


      /*
        Note: if |_retval| is |NULL|, I won't bother returning a copy
        of the replaced value.
      */

    if ( _retval )
      *_retval = 0;


      // Before we can insert a new entry, we'll need to
      //  find the |CategoryNode| to put it in...
    CategoryNode* category;
    if ( !(category = find_category(aCategoryName)) )
      {
          // That category doesn't exist yet; let's make it.
        category = new CategoryNode;
        
        PRUint32 len = strlen(aCategoryName);
        char* categoryName = ArenaStrndup(aCategoryName, len, &mArena);
        nsCStringKey categoryNameKey(categoryName, len, nsCStringKey::NEVER_OWN);
        Put(&categoryNameKey, category);
      }

      // See if this entry is already in this category
    const char* entry = category->find_leaf(aEntryName);

    nsresult status = NS_OK;
    if ( entry )
      {
          // If this entry is in the category already,
          //  then you better have said 'replace'!

        if ( aReplace )
          {
              // return the value that we're replacing
            if ( _retval )
              *_retval = nsCRT::strdup(entry);
          }
        else
          status = NS_ERROR_INVALID_ARG; // ...stops us from putting the value in
      }

      // If you didn't say 'replace', and there was already an entry there,
      //  then we can't put your value in (that's why we set |status|, above),
      //  or make it persistent (see below)

    if ( NS_SUCCEEDED(status) )
      { // it's OK to put a value in

        // we can't delete the entry because we're
        // arena-allocated.. just pretend we do this.
        // delete entry;

        // now put in the new value entry
        entry = ArenaStrdup(aValue, &mArena);
        
        PRUint32 len = strlen(aEntryName);
        char* entryName = ArenaStrndup(aEntryName, len, &mArena);
        
        nsCStringKey entryNameKey(entryName, len, nsCStringKey::NEVER_OWN);
        category->Put(&entryNameKey, (void*)entry);

        nsCOMPtr<nsIComponentLoaderManager> mgr;
        NS_GetComponentLoaderManager(getter_AddRefs(mgr));
        if (mgr)
          mgr->FlushPersistentStore(PR_FALSE);
      }

    return status;
  }



NS_IMETHODIMP
nsCategoryManager::DeleteCategoryEntry( const char *aCategoryName,
                                        const char *aEntryName,
                                        PRBool aDontPersist)
  {

    NS_ASSERTION(aCategoryName, "aCategoryName is NULL!");
    NS_ASSERTION(aEntryName,    "aEntryName is NULL!");

      /*
        Note: no errors are reported since failure to delete
        probably won't hurt you, and returning errors seriously
        inconveniences JS clients
      */

    CategoryNode* category = find_category(aCategoryName);
    if (category)
      {
        nsCStringKey entryKey(aEntryName);
        category->RemoveAndDelete(&entryKey);

        nsCOMPtr<nsIComponentLoaderManager> mgr;
        NS_GetComponentLoaderManager(getter_AddRefs(mgr));
        if (mgr)
          mgr->FlushPersistentStore(PR_FALSE);

      }

    return NS_OK;
  }



NS_IMETHODIMP
nsCategoryManager::DeleteCategory( const char *aCategoryName )
  {
    NS_ASSERTION(aCategoryName, "aCategoryName is NULL!");

    nsCOMPtr<nsIComponentLoaderManager> mgr;
    NS_GetComponentLoaderManager(getter_AddRefs(mgr));
    if (mgr)
      mgr->FlushPersistentStore(PR_FALSE);

      // QUESTION: consider whether this should be an error
    nsCStringKey categoryKey(aCategoryName);
    return RemoveAndDelete(&categoryKey) ? NS_OK : NS_ERROR_NOT_AVAILABLE;
  }





NS_IMETHODIMP
nsCategoryManager::EnumerateCategory( const char *aCategoryName,
                                      nsISimpleEnumerator **_retval )
  {
    NS_ASSERTION(aCategoryName, "aCategoryName is NULL!");
    NS_ASSERTION(_retval,       "_retval is NULL!");

    *_retval = 0;

    nsresult status = NS_ERROR_NOT_AVAILABLE;
    CategoryNode* category = find_category(aCategoryName);
    if (category)
      {
        status = NS_NewHashtableEnumerator(category, ExtractKeyString, 0, _retval);
      }

      // If you couldn't find the category, or had trouble creating an enumerator...
    if ( NS_FAILED(status) )
      {
        NS_IF_RELEASE(*_retval);
        status = NS_NewEmptyEnumerator(_retval);
      }

    return status;
  }


NS_IMETHODIMP 
nsCategoryManager::EnumerateCategories(nsISimpleEnumerator **_retval)
{
    NS_ASSERTION(_retval, "_retval is NULL!");
    *_retval = 0;

    nsresult status = NS_ERROR_NOT_AVAILABLE;

    status = NS_NewHashtableEnumerator(this, ExtractKeyString, 0, _retval);
 
    // If you couldn't find the category, or had trouble creating an
    // enumerator...
    if ( NS_FAILED(status) )
    {
        NS_IF_RELEASE(*_retval);
        status = NS_NewEmptyEnumerator(_retval);
    }
    return status;
}

class nsCategoryManagerFactory : public nsIFactory
   {
     public:
       nsCategoryManagerFactory();

       NS_DECL_ISUPPORTS
      NS_DECL_NSIFACTORY
   };

NS_IMPL_ISUPPORTS1(nsCategoryManagerFactory, nsIFactory)

nsCategoryManagerFactory::nsCategoryManagerFactory()
  {
  }

NS_IMETHODIMP
nsCategoryManagerFactory::CreateInstance( nsISupports* aOuter, const nsIID& aIID, void** aResult )
  {
    // assert(aResult);

    *aResult = 0;

    nsresult status = NS_OK;
    if ( aOuter )
      status = NS_ERROR_NO_AGGREGATION;
    else
      {
        nsCategoryManager* raw_category_manager;
        nsCOMPtr<nsICategoryManager> new_category_manager = (raw_category_manager = new nsCategoryManager);
        if ( new_category_manager )
              status = new_category_manager->QueryInterface(aIID, aResult);
        else
          status = NS_ERROR_OUT_OF_MEMORY;
      }

    return status;
  }

NS_IMETHODIMP
nsCategoryManagerFactory::LockFactory( PRBool )
  {
      // Not implemented...
    return NS_OK;
  }

nsresult
NS_CategoryManagerGetFactory( nsIFactory** aFactory )
  {
    // assert(aFactory);

    nsresult status;

    *aFactory = 0;
    nsIFactory* new_factory = NS_STATIC_CAST(nsIFactory*, new nsCategoryManagerFactory);
    if (new_factory)
      {
        *aFactory = new_factory;
        NS_ADDREF(*aFactory);
        status = NS_OK;
      }
    else
      status = NS_ERROR_OUT_OF_MEMORY;

    return status;
  }



/*
 * CreateServicesFromCategory()
 *
 * Given a category, this convenience functions enumerates the category and 
 * creates a service of every CID or ContractID registered under the category.
 * If observerTopic is non null and the service implements nsIObserver,
 * this will attempt to notify the observer with the origin, observerTopic string
 * as parameter.
 */
NS_COM nsresult
NS_CreateServicesFromCategory(const char *category,
                              nsISupports *origin,
                              const char *observerTopic)
{
    nsresult rv = NS_OK;
    
    int nFailed = 0; 
    nsCOMPtr<nsICategoryManager> categoryManager = 
        do_GetService("@mozilla.org/categorymanager;1", &rv);
    if (!categoryManager) return rv;

    nsCOMPtr<nsISimpleEnumerator> enumerator;
    rv = categoryManager->EnumerateCategory(category, 
            getter_AddRefs(enumerator));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsISupports> entry;
    while (NS_SUCCEEDED(enumerator->GetNext(getter_AddRefs(entry)))) {
        // From here on just skip any error we get.
        nsCOMPtr<nsISupportsCString> catEntry = do_QueryInterface(entry, &rv);
        if (NS_FAILED(rv)) {
            nFailed++;
            continue;
        }
        nsCAutoString entryString;
        rv = catEntry->GetData(entryString);
        if (NS_FAILED(rv)) {
            nFailed++;
            continue;
        }
        nsXPIDLCString contractID;
        rv = categoryManager->GetCategoryEntry(category,entryString.get(), getter_Copies(contractID));
        if (NS_FAILED(rv)) {
            nFailed++;
            continue;
        }
        
        nsCOMPtr<nsISupports> instance = do_GetService(contractID, &rv);
        if (NS_FAILED(rv)) {
            nFailed++;
            continue;
        }

        if (observerTopic) {
            // try an observer, if it implements it.
            nsCOMPtr<nsIObserver> observer = do_QueryInterface(instance, &rv);
            if (NS_SUCCEEDED(rv) && observer)
                observer->Observe(origin, observerTopic, NS_LITERAL_STRING("").get());
        }
    }
    return (nFailed ? NS_ERROR_FAILURE : NS_OK);
}
