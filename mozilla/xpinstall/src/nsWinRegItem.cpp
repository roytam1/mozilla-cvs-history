/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
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

#include "nsWinRegItem.h"
#include "xp.h"
//#include "xp_str.h"
//#include <windows.h>

/* Public Methods */

nsWinRegItem::nsWinRegItem(nsWinReg* regObj, PRInt32 root, PRInt32 action, nsString sub, nsString valname, nsString val)
: nsInstallObject(regObj->installObject())
{
	reg     = regObj;
	command = action;
	rootkey = root;

	/* I'm assuming we need to copy these */
	subkey  = new nsString(sub);
	name    = new nsString(valname);
	value   = new nsString(val);
}

nsWinRegItem::~nsWinRegItem()
{
  if(subkey != nsnull)
    delete subkey;

  if(name != nsnull)
    delete name;

  if(value != nsnull)
    delete value;
}

PRInt32 nsWinRegItem::Complete()
{
  PRInt32 aReturn;

  switch (command)
  {
    case NS_WIN_REG_CREATE:
			reg->finalCreateKey(rootkey, *subkey, *name, &aReturn);
      break;
    case NS_WIN_REG_DELETE:
      reg->finalDeleteKey(rootkey, *subkey, &aReturn);
      break;
    case NS_WIN_REG_DELETE_VAL:
      reg->finalDeleteValue(rootkey, *subkey, *name, &aReturn);
      break;
    case NS_WIN_REG_SET_VAL_STRING:
      reg->finalSetValueString(rootkey, *subkey, *name, *(nsString*)value, &aReturn);
      break;
    case NS_WIN_REG_SET_VAL:
      reg->finalSetValue(rootkey, *subkey, *name, (nsWinRegValue*)value, &aReturn);
      break;
  }
	return aReturn;
}
  
float nsWinRegItem::GetInstallOrder()
{
	return 3;
}

#define kCRK "Create Registry Key "
#define kDRK "Delete Registry key "
#define kDRV "Delete Registry value "
#define kSRV "Store Registry value "
#define kUNK "Unknown "

char* nsWinRegItem::toString()
{
	nsString* keyString;
	nsString* result;
  char*     resultCString;

	switch(command)
	{
	case NS_WIN_REG_CREATE:
		keyString = keystr(rootkey, subkey, nsnull);
    result = new nsString(kCRK);
    result->Append(*keyString);
    resultCString = result->ToNewCString();
    delete [] keyString;
    delete [] result;
		return resultCString;
	case NS_WIN_REG_DELETE:
		keyString = keystr(rootkey, subkey, nsnull);
    result = new nsString(kDRK);
    result->Append(*keyString);
    resultCString = result->ToNewCString();
    delete [] keyString;
    delete [] result;
		return resultCString;
	case NS_WIN_REG_DELETE_VAL:
		keyString = keystr(rootkey, subkey, name);
    result = new nsString(kDRV);
    result->Append(*keyString);
    resultCString = result->ToNewCString();
    delete [] keyString;
    delete [] result;
		return resultCString;
	case NS_WIN_REG_SET_VAL_STRING:
		keyString = keystr(rootkey, subkey, name);
    result = new nsString(kSRV);
    result->Append(*keyString);
    resultCString = result->ToNewCString();
    delete [] keyString;
    delete [] result;
		return resultCString;
	case NS_WIN_REG_SET_VAL:
		keyString = keystr(rootkey, subkey, name);
    result = new nsString(kSRV);
    result->Append(*keyString);
    resultCString = result->ToNewCString();
    delete [] keyString;
    delete [] result;
		return resultCString;
	default:
		keyString = keystr(rootkey, subkey, name);
    result = new nsString(kUNK);
    result->Append(*keyString);
    resultCString = result->ToNewCString();
    delete [] keyString;
    delete [] result;
		return resultCString;
	}
}

PRInt32 nsWinRegItem::Prepare()
{
	return NULL;
}

void nsWinRegItem::Abort()
{
}

/* Private Methods */

nsString* nsWinRegItem::keystr(PRInt32 root, nsString* subkey, nsString* name)
{
	nsString* rootstr;
	nsString* finalstr;

	switch(root)
	{
	case (int)(HKEY_CLASSES_ROOT) :
		rootstr = new nsString("\\HKEY_CLASSES_ROOT\\");
		break;
	case (int)(HKEY_CURRENT_USER) :
		rootstr = new nsString("\\HKEY_CURRENT_USER\\");
		break;
	case (int)(HKEY_LOCAL_MACHINE) :
		rootstr = new nsString("\\HKEY_LOCAL_MACHINE\\");
		break;
	case (int)(HKEY_USERS) :
		rootstr = new nsString("\\HKEY_USERS\\");
		break;
	default:
    rootstr = new nsString("\\#");
    rootstr->Append(itoa(root));
    rootstr->Append("\\");
		break;
	}

  finalstr = new nsString(*rootstr);
	if(name != nsnull)
	{
    finalstr->Append(*subkey);
    finalstr->Append(" [");
    finalstr->Append(*name);
    finalstr->Append("]");
	}
  delete [] rootstr;
  return finalstr;
}


char* nsWinRegItem::itoa(PRInt32 n)
{
	char* s;
	int i, sign;
	if((sign = n) < 0)
		n = -n;
	i = 0;
	
	s = (char*)XP_ALLOC(sizeof(char));

	do
		{
		s = (char*)XP_REALLOC(s, (i+1)*sizeof(char));
		s[i++] = n%10 + '0';
		s[i] = '\0';
		} while ((n/=10) > 0);
		
	if(sign < 0)
	{
		s = (char*)XP_REALLOC(s, (i+1)*sizeof(char));
		s[i++] = '-';
	}
	s[i]  = '\0';
	reverseString(s);
	return s;
}

void nsWinRegItem::reverseString(char* s)
{
	int c, i, j;
	
	for(i=0, j=strlen(s)-1; i<j; i++, j--)
		{
		c = s[i];
		s[i] = s[j];
		s[j] = c;
		}
}

/* CanUninstall
* WinRegItem() does not install any files which can be uninstalled,
* hence this function returns false. 
*/
PRBool
nsWinRegItem:: CanUninstall()
{
    return FALSE;
}

/* RegisterPackageNode
* WinRegItem() installs files which need to be registered,
* hence this function returns true.
*/
PRBool
nsWinRegItem:: RegisterPackageNode()
{
    return TRUE;
}

