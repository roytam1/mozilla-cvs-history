/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 * Copyright (C) 1998,1999 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#include "nsIFactory.h"
#include "nsISupports.h"
#include "nsIGenericFactory.h"
#include "nsIModule.h"

#include "nsAbBaseCID.h"
#include "pratom.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "rdf.h"
#include "nsCRT.h"
#include "nsCOMPtr.h"


/* Include all of the interfaces our factory can generate components for */

#include "nsDirectoryDataSource.h"
#include "nsCardDataSource.h"
#include "nsAbDirectory.h"
#include "nsAbCard.h"
#include "nsAddrDatabase.h"
#include "nsAddressBook.h"
#include "nsAddrBookSession.h"
#include "nsAbDirProperty.h"
#include "nsAbAutoCompleteSession.h"
#include "nsAbAddressCollecter.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(nsAddressBook)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsAbDirectoryDataSource)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsAbDirectory)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsAbCardDataSource)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsAbCard)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsAbCardProperty)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsAddrDatabase)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsAbDirProperty)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsAddrBookSession)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsAbAutoCompleteSession)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsAbAddressCollecter)
  
struct components_t {
  nsCID cid;
  nsIGenericFactory::ConstructorProcPtr constructor;
  const char *progid;
  const char *description;
};

static components_t components[] =
{
  { NS_ADDRESSBOOK_CID, &nsAddressBookConstructor, NS_ADDRESSBOOK_PROGID, },
  { NS_ABDIRECTORYDATASOURCE_CID, &nsAbDirectoryDataSourceConstructor, NS_ABDIRECTORYDATASOURCE_PROGID, },
  { NS_ABDIRECTORY_CID, &nsAbDirectoryConstructor, NS_ABDIRECTORY_PROGID, },
  { NS_ABCARDDATASOURCE_CID, &nsAbCardDataSourceConstructor, NS_ABCARDDATASOURCE_PROGID, },
  { NS_ABCARD_CID, &nsAbCardConstructor, NS_ABCARD_PROGID, },
  { NS_ADDRDATABASE_CID, &nsAddrDatabaseConstructor, NS_ADDRDATABASE_PROGID, },
  { NS_ABCARDPROPERTY_CID, &nsAbCardPropertyConstructor, NS_ABCARDPROPERTY_PROGID, },
  { NS_ABDIRPROPERTY_CID, &nsAbDirPropertyConstructor, NS_ABDIRPROPERTY_PROGID, },
  { NS_ADDRBOOKSESSION_CID, &nsAddrBookSessionConstructor, NS_ADDRBOOKSESSION_PROGID, },
  { NS_ABAUTOCOMPLETESESSION_CID, &nsAbAutoCompleteSessionConstructor, NS_ABAUTOCOMPLETESESSION_PROGID, },
  { NS_ABADDRESSCOLLECTER_CID, &nsAbAddressCollecterConstructor, NS_ABADDRESSCOLLECTER_PROGID, },

};


NS_IMPL_MODULE(components)
NS_IMPL_NSGETMODULE(nsModule)
