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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation. Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#ifndef nsIProperties_h___
#define nsIProperties_h___

#include "nsIProperties.h"
#include "nsIEnumerator.h"
#include "nsISupportsArray.h"
#include "nsID.h"
#include "nsIInputStream.h"
#include "nsIOutputStream.h"
#include "nsString.h"

// {1A180F60-93B2-11d2-9B8B-00805F8A16D9}
#define NS_IPERSISTENTPROPERTIES_IID \
{ 0x1a180f60, 0x93b2, 0x11d2, \
  { 0x9b, 0x8b, 0x0, 0x80, 0x5f, 0x8a, 0x16, 0xd9 } }

// {2245E573-9464-11d2-9B8B-00805F8A16D9}
NS_DECLARE_ID(kPersistentPropertiesCID,
  0x2245e573, 0x9464, 0x11d2, 0x9b, 0x8b, 0x0, 0x80, 0x5f, 0x8a, 0x16, 0xd9);

#define NS_PERSISTENTPROPERTIES_PROGID "component://netscape/persistent-properties"
#define NS_PERSISTENTPROPERTIES_CLASSNAME "Persistent Properties"

class nsIPersistentProperties : public nsIProperties
{
public:
  static const nsIID& GetIID() { static nsIID iid = NS_IPERSISTENTPROPERTIES_IID; return iid; }

  NS_IMETHOD Load(nsIInputStream* aIn) = 0;
  NS_IMETHOD Save(nsIOutputStream* aOut, const nsString& aHeader) = 0;
  NS_IMETHOD Subclass(nsIPersistentProperties* aSubclass) = 0;	

  /**
   * Enumerates the properties in the supplied enumerator.
   * @return NS_ERROR_FAILURE if no properties to enumerate
   */
  NS_IMETHOD EnumerateProperties(nsIBidirectionalEnumerator** aResult) = 0;

  // XXX these 2 methods will be subsumed by the ones from 
  // nsIProperties once we figure this all out
  NS_IMETHOD GetStringProperty(const nsString& aKey, nsString& aValue) = 0;
  NS_IMETHOD SetStringProperty(const nsString& aKey, nsString& aNewValue,
                               nsString& aOldValue) = 0;
};

////////////////////////////////////////////////////////////////////////////////

// {C23C10B3-0E1A-11d3-A430-0060B0EB5963}
#define NS_IPROPERTYELEMENT_IID \
{ 0xc23c10b3, 0xe1a, 0x11d3, \
	{ 0xa4, 0x30, 0x0, 0x60, 0xb0, 0xeb, 0x59, 0x63 } }

// {579C0568-0E1B-11d3-A430-0060B0EB5963}
NS_DECLARE_ID(kPropertyElementCID, 
	0x579c0568, 0xe1b, 0x11d3, 0xa4, 0x30, 0x0, 0x60, 0xb0, 0xeb, 0x59, 0x63);

class nsIPropertyElement : public nsISupports
{
public:
	static const nsIID& GetIID() { static nsIID iid = NS_IPROPERTYELEMENT_IID; return iid; }	

	NS_IMETHOD SetKey(nsString* aKey) = 0;
	NS_IMETHOD SetValue(nsString* aValue) = 0; 
	NS_IMETHOD GetKey(nsString** aReturnKey) = 0;
	NS_IMETHOD GetValue(nsString** aReturnValue) = 0;
};

////////////////////////////////////////////////////////////////////////////////

#endif // nsIProperties_h___
