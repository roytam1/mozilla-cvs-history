/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#include "nsIEntityConverter.h"
#include "nsIFactory.h"
#include "nsIPersistentProperties.h"


nsresult NS_NewEntityConverter(nsISupports** oResult);

#define kVERSION_STRING_LEN 128

class nsEntityVersionList
{
public:
  nsEntityVersionList() : mEntityProperties(NULL) {}
  ~nsEntityVersionList() {NS_IF_RELEASE(mEntityProperties);}
  PRUint32 mVersion;
  PRUnichar mEntityListName[kVERSION_STRING_LEN+1];
  nsIPersistentProperties *mEntityProperties;
};

class nsEntityConverter: public nsIEntityConverter
{
public:
	
	//
	// implementation methods
	//
	nsEntityConverter();
	virtual ~nsEntityConverter();

	//
	// nsISupports
	//
	NS_DECL_ISUPPORTS

	//
	// nsIEntityConverter
	//
	NS_IMETHOD ConvertToEntity(PRUnichar character, PRUint32 entityVersion, char **_retval);

	NS_IMETHOD ConvertToEntities(const PRUnichar *inString, PRUint32 entityVersion, PRUnichar **_retval);

protected:

  // load a version property file and generate a version list (number/name pair)
  NS_IMETHOD LoadVersionPropertyFile();

  // map version number to version string
  const PRUnichar* GetVersionName(PRUint32 versionNumber);

  // map version number to nsIPersistentProperties
  nsIPersistentProperties* GetVersionPropertyInst(PRUint32 versionNumber);

  // load a properies file
  nsIPersistentProperties* LoadEntityPropertyFile(PRInt32 version);


  nsEntityVersionList *mVersionList;            // array of version number/name pairs
  PRUint32 mVersionListLength;                  // number of supported versions
};
