/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 */

#include "nsIAtom.h"
#include "nsParserService.h"
#include "nsHTMLEntities.h"
#include "nsElementTable.h"
#include "nsICategoryManager.h"
#include "nsCategoryManagerUtils.h"

nsParserService::nsParserService() : mEntries(0)
{
  NS_INIT_ISUPPORTS();
  mHaveNotifiedCategoryObservers = PR_FALSE;
}

nsParserService::~nsParserService()
{
  nsObserverEntry *entry = nsnull;
  while( (entry = NS_STATIC_CAST(nsObserverEntry*,mEntries.Pop())) ) {
    NS_RELEASE(entry);
  }
}

NS_IMPL_ISUPPORTS1(nsParserService, nsIParserService)

NS_IMETHODIMP
nsParserService::HTMLAtomTagToId(nsIAtom* aAtom, PRInt32* aId) const
{
  const PRUnichar *tagName = nsnull;
  aAtom->GetUnicode(&tagName);
  NS_ASSERTION(tagName, "Null string in atom!");

  *aId = nsHTMLTags::LookupTag(nsDependentString(tagName));

  return NS_OK;
}

NS_IMETHODIMP
nsParserService::HTMLCaseSensitiveAtomTagToId(nsIAtom* aAtom,
                                              PRInt32* aId) const
{
  const PRUnichar *tagName = nsnull;
  aAtom->GetUnicode(&tagName);
  NS_ASSERTION(tagName, "Null string in atom!");

  *aId = nsHTMLTags::CaseSensitiveLookupTag(tagName);

  return NS_OK;
}

NS_IMETHODIMP
nsParserService::HTMLStringTagToId(const nsAString &aTagName,
                                   PRInt32* aId) const
{
  *aId = nsHTMLTags::LookupTag(aTagName);

  return NS_OK;
}

NS_IMETHODIMP
nsParserService::HTMLIdToStringTag(PRInt32 aId,
                                   const PRUnichar **aTagName) const
{
  *aTagName = nsHTMLTags::GetStringValue((nsHTMLTag)aId);

  return NS_OK;
}

NS_IMETHODIMP
nsParserService::HTMLConvertEntityToUnicode(const nsAString& aEntity,
                                            PRInt32* aUnicode) const
{
  *aUnicode = nsHTMLEntities::EntityToUnicode(aEntity);

  return NS_OK;
}

NS_IMETHODIMP
nsParserService::HTMLConvertUnicodeToEntity(PRInt32 aUnicode,
                                            nsCString& aEntity) const
{
  const char* str = nsHTMLEntities::UnicodeToEntity(aUnicode);
  if (str) {
    aEntity.Assign(str);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsParserService::IsContainer(PRInt32 aId, PRBool& aIsContainer) const
{
  aIsContainer = nsHTMLElement::IsContainer((eHTMLTags)aId);

  return NS_OK;
}

NS_IMETHODIMP
nsParserService::IsBlock(PRInt32 aId, PRBool& aIsBlock) const
{
  if((aId>eHTMLTag_unknown) && (aId<eHTMLTag_userdefined)) {
    aIsBlock=((gHTMLElements[aId].IsMemberOf(kBlock))       ||
              (gHTMLElements[aId].IsMemberOf(kBlockEntity)) ||
              (gHTMLElements[aId].IsMemberOf(kHeading))     ||
              (gHTMLElements[aId].IsMemberOf(kPreformatted))||
              (gHTMLElements[aId].IsMemberOf(kList)));
  }
  else {
    aIsBlock = PR_FALSE;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsParserService::RegisterObserver(nsIElementObserver* aObserver,
                                  const nsAString& aTopic,
                                  const eHTMLTags* aTags)
{
  nsresult result = NS_OK;
  nsObserverEntry* entry = GetEntry(aTopic);

  if(!entry) {
    result = CreateEntry(aTopic,&entry);
    NS_ENSURE_SUCCESS(result,result);
  }

  while (*aTags) {
    if(*aTags != eHTMLTag_userdefined && *aTags <= NS_HTML_TAG_MAX) {
      entry->AddObserver(aObserver,*aTags);
    }
    ++aTags;
  }

  return result;
}

NS_IMETHODIMP
nsParserService::UnregisterObserver(nsIElementObserver* aObserver,
                                    const nsAString& aTopic)
{
  PRInt32 count = mEntries.GetSize();

  for (PRInt32 i=0; i < count; ++i) {
    nsObserverEntry* entry = NS_STATIC_CAST(nsObserverEntry*,mEntries.ObjectAt(i));
    if (entry && entry->Matches(aTopic)) {
      entry->RemoveObserver(aObserver);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsParserService::GetTopicObservers(const nsAString& aTopic,
                                   nsIObserverEntry** aEntry) {
  nsresult result = NS_OK;
  nsObserverEntry* entry = GetEntry(aTopic);

  if (!entry) {
    return NS_ERROR_NULL_POINTER;
  }

  NS_ADDREF(*aEntry = entry);

  return result;
}

class nsMatchesTopic : public nsDequeFunctor{
  const nsAString& mString;
public:
  PRBool matched;
  nsObserverEntry* entry;
  nsMatchesTopic(const nsAString& aString):mString(aString),matched(PR_FALSE){};
  virtual void* operator()(void* anObject){
    entry=NS_STATIC_CAST(nsObserverEntry*, anObject);
    matched=mString.Equals(entry->mTopic);
    return matched ? nsnull : anObject;
  };
};

// XXX This may be more efficient as a HashTable instead of linear search
nsObserverEntry*
nsParserService::GetEntry(const nsAString& aTopic)
{
  if (!mHaveNotifiedCategoryObservers) {
    mHaveNotifiedCategoryObservers = PR_TRUE;
    NS_CreateServicesFromCategory("parser-service-category",
                                  NS_STATIC_CAST(nsISupports*,NS_STATIC_CAST(void*,this)),
                                  "parser-service-start"); 
  }

  nsMatchesTopic matchesTopic(aTopic);
  mEntries.FirstThat(*&matchesTopic);
  return matchesTopic.matched?matchesTopic.entry:nsnull;
}

nsresult
nsParserService::CreateEntry(const nsAString& aTopic, nsObserverEntry** aEntry)
{
  *aEntry = new nsObserverEntry(aTopic);

  if (!aEntry) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  NS_ADDREF(*aEntry);
  mEntries.Push(*aEntry);

  return NS_OK;
}
