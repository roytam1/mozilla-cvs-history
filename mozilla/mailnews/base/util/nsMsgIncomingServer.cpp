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

#include "nsMsgIncomingServer.h"
#include "nscore.h"
#include "nsCom.h"
#include "plstr.h"
#include "prmem.h"
#include "prprf.h"

#include "nsIServiceManager.h"
#include "nsIPref.h"
#include "nsCOMPtr.h"
#include "nsIMsgFolder.h"
#include "nsIMsgFolderCache.h"
#include "nsIMsgFolderCacheElement.h"
#include "nsINetSupportDialogService.h"
#include "nsIPrompt.h"
#include "nsXPIDLString.h"
#include "nsIRDFService.h"
#include "nsIMsgProtocolInfo.h"
#include "nsRDFCID.h"

static NS_DEFINE_CID(kPrefServiceCID, NS_PREF_CID);
static NS_DEFINE_CID(kNetSupportDialogCID, NS_NETSUPPORTDIALOG_CID);
static NS_DEFINE_CID(kRDFServiceCID, NS_RDFSERVICE_CID);

MOZ_DECL_CTOR_COUNTER(nsMsgIncomingServer);

nsMsgIncomingServer::nsMsgIncomingServer():
    m_prefs(0),
    m_serverKey(0),
    m_rootFolder(0)
{
  MOZ_COUNT_CTOR(nsMsgIncomingServer);

  NS_INIT_REFCNT();
  m_serverBusy = PR_FALSE;
  m_password = "";
}

nsMsgIncomingServer::~nsMsgIncomingServer()
{
	
	MOZ_COUNT_DTOR(nsMsgIncomingServer);

    if (m_prefs) nsServiceManager::ReleaseService(kPrefServiceCID,
                                                  m_prefs,
                                                  nsnull);
    PR_FREEIF(m_serverKey)
}

NS_IMPL_ISUPPORTS1(nsMsgIncomingServer, nsIMsgIncomingServer)

NS_IMPL_GETSET(nsMsgIncomingServer, ServerBusy, PRBool, m_serverBusy)
NS_IMPL_GETTER_STR(nsMsgIncomingServer::GetKey, m_serverKey)

NS_IMETHODIMP
nsMsgIncomingServer::SetKey(const char * serverKey)
{
    nsresult rv = NS_OK;
    // in order to actually make use of the key, we need the prefs
    if (!m_prefs)
        rv = nsServiceManager::GetService(kPrefServiceCID,
                                          nsCOMTypeInfo<nsIPref>::GetIID(),
                                          (nsISupports**)&m_prefs);

    PR_FREEIF(m_serverKey);
    m_serverKey = PL_strdup(serverKey);
    return rv;
}
    
NS_IMETHODIMP
nsMsgIncomingServer::SetRootFolder(nsIFolder * aRootFolder)
{
	m_rootFolder = aRootFolder;
	return NS_OK;
}

NS_IMETHODIMP
nsMsgIncomingServer::GetRootFolder(nsIFolder * *aRootFolder)
{
	if (!aRootFolder)
		return NS_ERROR_NULL_POINTER;
    if (m_rootFolder) {
      *aRootFolder = m_rootFolder;
      NS_ADDREF(*aRootFolder);
    } else {
      nsresult rv = CreateRootFolder();
      if (NS_FAILED(rv)) return rv;
      
      *aRootFolder = m_rootFolder;
      NS_IF_ADDREF(*aRootFolder);
    }
	return NS_OK;
}
  
NS_IMETHODIMP
nsMsgIncomingServer::PerformBiff()
{
	//This had to be implemented in the derived class, but in case someone doesn't implement it
	//just return not implemented.
	return NS_ERROR_NOT_IMPLEMENTED;	
}

NS_IMETHODIMP nsMsgIncomingServer::WriteToFolderCache(nsIMsgFolderCache *folderCache)
{
	nsresult rv = NS_OK;
	if (m_rootFolder)
	{
		nsCOMPtr <nsIMsgFolder> msgFolder = do_QueryInterface(m_rootFolder, &rv);
		if (NS_SUCCEEDED(rv) && msgFolder)
			rv = msgFolder->WriteToFolderCache(folderCache);
	}
	return rv;
}

NS_IMETHODIMP
nsMsgIncomingServer::CloseCachedConnections()
{
	// derived class should override if they cache connections.
	return NS_OK;
}


// construct <localStoreType>://[<username>@]<hostname
NS_IMETHODIMP
nsMsgIncomingServer::GetServerURI(char **aResult)
{
    NS_ENSURE_ARG_POINTER(aResult);
    nsresult rv;
    nsCAutoString uri;

    nsXPIDLCString localStoreType;
    rv = GetLocalStoreType(getter_Copies(localStoreType));
    if (NS_FAILED(rv)) return rv;

    uri += localStoreType;
    uri += "://";

    nsXPIDLCString username;
    rv = GetUsername(getter_Copies(username));

    if (NS_SUCCEEDED(rv) && ((const char*)username) && username[0]) {
        nsXPIDLCString escapedUsername;
        *((char **)getter_Copies(escapedUsername)) =
            nsEscape(username, url_Path);
        // not all servers have a username 
        uri += escapedUsername;
        uri += '@';
    }

    nsXPIDLCString hostname;
    rv = GetHostName(getter_Copies(hostname));

    if (NS_SUCCEEDED(rv) && ((const char*)hostname) && hostname[0]) {
        nsXPIDLCString escapedHostname;
        *((char **)getter_Copies(escapedHostname)) =
            nsEscape(hostname, url_Path);
        // not all servers have a hostname
        uri += escapedHostname;
    }

    *aResult = uri.ToNewCString();
    return NS_OK;
}

nsresult
nsMsgIncomingServer::CreateRootFolder()
{
	nsresult rv;
			  // get the URI from the incoming server
  nsXPIDLCString serverUri;
  rv = GetServerURI(getter_Copies(serverUri));
  if (NS_FAILED(rv)) return rv;

  NS_WITH_SERVICE(nsIRDFService, rdf,
                        kRDFServiceCID, &rv);

  // get the corresponding RDF resource
  // RDF will create the server resource if it doesn't already exist
  nsCOMPtr<nsIRDFResource> serverResource;
  rv = rdf->GetResource(serverUri, getter_AddRefs(serverResource));
  if (NS_FAILED(rv)) return rv;

  // make incoming server know about its root server folder so we 
  // can find sub-folders given an incoming server.
  m_rootFolder = do_QueryInterface(serverResource, &rv);
  return rv;
}

char *
nsMsgIncomingServer::getPrefName(const char *serverKey,
                                 const char *fullPrefName)
{
  return PR_smprintf("mail.server.%s.%s", serverKey, fullPrefName);
}

// this will be slightly faster than the above, and allows
// the "default" server preference root to be set in one place
char *
nsMsgIncomingServer::getDefaultPrefName(const char *fullPrefName)
{
  return PR_smprintf("mail.server.default.%s", fullPrefName);
}


nsresult
nsMsgIncomingServer::GetBoolValue(const char *prefname,
                                 PRBool *val)
{
  char *fullPrefName = getPrefName(m_serverKey, prefname);
  nsresult rv = m_prefs->GetBoolPref(fullPrefName, val);
  PR_Free(fullPrefName);
  
  if (NS_FAILED(rv))
    rv = getDefaultBoolPref(prefname, val);
  
  return rv;
}

nsresult
nsMsgIncomingServer::getDefaultBoolPref(const char *prefname,
                                        PRBool *val) {
  
  char *fullPrefName = getDefaultPrefName(prefname);
  nsresult rv = m_prefs->GetBoolPref(fullPrefName, val);
  PR_Free(fullPrefName);

  if (NS_FAILED(rv)) {
    *val = PR_FALSE;
    rv = NS_OK;
  }
  return rv;
}

nsresult
nsMsgIncomingServer::SetBoolValue(const char *prefname,
                                 PRBool val)
{
  nsresult rv;
  char *fullPrefName = getPrefName(m_serverKey, prefname);

  PRBool defaultValue;
  rv = getDefaultBoolPref(prefname, &defaultValue);

  if (NS_SUCCEEDED(rv) &&
      val == defaultValue)
    m_prefs->ClearUserPref(fullPrefName);
  else
    rv = m_prefs->SetBoolPref(fullPrefName, val);
  
  PR_Free(fullPrefName);
  
  return rv;
}

nsresult
nsMsgIncomingServer::GetIntValue(const char *prefname,
                                PRInt32 *val)
{
  char *fullPrefName = getPrefName(m_serverKey, prefname);
  nsresult rv = m_prefs->GetIntPref(fullPrefName, val);
  PR_Free(fullPrefName);

  if (NS_FAILED(rv))
    rv = getDefaultIntPref(prefname, val);
  
  return rv;
}

nsresult
nsMsgIncomingServer::GetFileValue(const char* prefname,
                                  nsIFileSpec **spec)
{
  char *fullPrefName = getPrefName(m_serverKey, prefname);
  nsresult rv = m_prefs->GetFilePref(fullPrefName, spec);
  PR_Free(fullPrefName);

  return rv;
}

nsresult
nsMsgIncomingServer::SetFileValue(const char* prefname,
                                    nsIFileSpec *spec)
{
  char *fullPrefName = getPrefName(m_serverKey, prefname);
  nsresult rv = m_prefs->SetFilePref(fullPrefName, spec, PR_FALSE);
  PR_Free(fullPrefName);

  return rv;
}

nsresult
nsMsgIncomingServer::getDefaultIntPref(const char *prefname,
                                        PRInt32 *val) {
  
  char *fullPrefName = getDefaultPrefName(prefname);
  nsresult rv = m_prefs->GetIntPref(fullPrefName, val);
  PR_Free(fullPrefName);

  if (NS_FAILED(rv)) {
    *val = 0;
    rv = NS_OK;
  }
  
  return rv;
}

nsresult
nsMsgIncomingServer::SetIntValue(const char *prefname,
                                 PRInt32 val)
{
  nsresult rv;
  char *fullPrefName = getPrefName(m_serverKey, prefname);
  
  PRInt32 defaultVal;
  rv = getDefaultIntPref(prefname, &defaultVal);
  
  if (NS_SUCCEEDED(rv) && defaultVal == val)
    m_prefs->ClearUserPref(fullPrefName);
  else
    rv = m_prefs->SetIntPref(fullPrefName, val);
  
  PR_Free(fullPrefName);
  
  return rv;
}

nsresult
nsMsgIncomingServer::GetCharValue(const char *prefname,
                                 char  **val)
{
  char *fullPrefName = getPrefName(m_serverKey, prefname);
  nsresult rv = m_prefs->CopyCharPref(fullPrefName, val);
  PR_Free(fullPrefName);
  
  if (NS_FAILED(rv))
    rv = getDefaultCharPref(prefname, val);
  
  return rv;
}

nsresult
nsMsgIncomingServer::GetUnicharValue(const char *prefname,
                                     PRUnichar **val)
{
  char *fullPrefName = getPrefName(m_serverKey, prefname);
  nsresult rv = m_prefs->CopyUnicharPref(fullPrefName, val);
  PR_Free(fullPrefName);
  
  if (NS_FAILED(rv))
    rv = getDefaultUnicharPref(prefname, val);
  
  return rv;
}

nsresult
nsMsgIncomingServer::getDefaultCharPref(const char *prefname,
                                        char **val) {
  
  char *fullPrefName = getDefaultPrefName(prefname);
  nsresult rv = m_prefs->CopyCharPref(fullPrefName, val);
  PR_Free(fullPrefName);

  if (NS_FAILED(rv)) {
    *val = nsnull;              // null is ok to return here
    rv = NS_OK;
  }
  return rv;
}

nsresult
nsMsgIncomingServer::getDefaultUnicharPref(const char *prefname,
                                           PRUnichar **val) {
  
  char *fullPrefName = getDefaultPrefName(prefname);
  nsresult rv = m_prefs->CopyUnicharPref(fullPrefName, val);
  PR_Free(fullPrefName);

  if (NS_FAILED(rv)) {
    *val = nsnull;              // null is ok to return here
    rv = NS_OK;
  }
  return rv;
}

nsresult
nsMsgIncomingServer::SetCharValue(const char *prefname,
                                 const char * val)
{
  nsresult rv;
  char *fullPrefName = getPrefName(m_serverKey, prefname);

  if (!val) {
    m_prefs->ClearUserPref(fullPrefName);
    return NS_OK;
  }
  
  char *defaultVal=nsnull;
  rv = getDefaultCharPref(prefname, &defaultVal);
  
  if (NS_SUCCEEDED(rv) &&
      PL_strcmp(defaultVal, val) == 0)
    m_prefs->ClearUserPref(fullPrefName);
  else
    rv = m_prefs->SetCharPref(fullPrefName, val);
  
  PR_FREEIF(defaultVal);
  PR_smprintf_free(fullPrefName);
  
  return rv;
}

nsresult
nsMsgIncomingServer::SetUnicharValue(const char *prefname,
                                  const PRUnichar * val)
{
  nsresult rv;
  char *fullPrefName = getPrefName(m_serverKey, prefname);

  if (!val) {
    m_prefs->ClearUserPref(fullPrefName);
    return NS_OK;
  }

  PRUnichar *defaultVal=nsnull;
  rv = getDefaultUnicharPref(prefname, &defaultVal);
  if (defaultVal && NS_SUCCEEDED(rv) &&
      nsCRT::strcmp(defaultVal, val) == 0)
    m_prefs->ClearUserPref(fullPrefName);
  else
    rv = m_prefs->SetUnicharPref(fullPrefName, val);
  
  PR_FREEIF(defaultVal);
  
  PR_smprintf_free(fullPrefName);
  
  return rv;
}

// pretty name is the display name to show to the user
NS_IMETHODIMP
nsMsgIncomingServer::GetPrettyName(PRUnichar **retval) {

  nsXPIDLString val;
  nsresult rv = GetUnicharValue("name", getter_Copies(val));
  if (NS_FAILED(rv)) return rv;

  nsAutoString prettyName(val);
  
  // if there's no name, then just return the hostname
  if (prettyName.IsEmpty()) {
    
    nsXPIDLCString username;
    rv = GetUsername(getter_Copies(username));
    if (NS_FAILED(rv)) return rv;
    if ((const char*)username &&
        PL_strcmp((const char*)username, "")!=0) {
      prettyName = username;
      prettyName += " on ";
    }
    
    nsXPIDLCString hostname;
    rv = GetHostName(getter_Copies(hostname));
    if (NS_FAILED(rv)) return rv;


    prettyName += hostname;
  }

  *retval = prettyName.ToNewUnicode();
  
  return NS_OK;
}

NS_IMETHODIMP
nsMsgIncomingServer::SetPrettyName(const PRUnichar *value)
{
    SetUnicharValue("name", value);
    
    nsCOMPtr<nsIFolder> rootFolder;
    GetRootFolder(getter_AddRefs(rootFolder));

    if (rootFolder)
        rootFolder->SetPrettyName(value);

    return NS_OK;
}

NS_IMETHODIMP
nsMsgIncomingServer::ToString(PRUnichar** aResult) {
  nsString servername("[nsIMsgIncomingServer: ");
  servername += m_serverKey;
  servername += "]";
  
  *aResult = servername.ToNewUnicode();
  NS_ASSERTION(*aResult, "no server name!");
  return NS_OK;
}
  

NS_IMETHODIMP nsMsgIncomingServer::SetPassword(const char * aPassword)
{
	// if remember password is turned on, write the password to preferences
	// otherwise, just set the password so we remember it for the rest of the current
	// session.

	PRBool rememberPassword = PR_FALSE;
	GetRememberPassword(&rememberPassword);
	
	if (rememberPassword)
	{
		SetPrefPassword((char *) aPassword);
	}

	m_password = aPassword;

	return NS_OK;
}

NS_IMETHODIMP nsMsgIncomingServer::GetPassword(char ** aPassword)
{
	nsresult rv = NS_OK;
	PRBool rememberPassword = PR_FALSE;
	// okay, here's the scoop for this messs...
	// (1) if we have a password already, go ahead and use it!
	// (2) if remember password is turned on, try reading in from the prefs and if we have one, go ahead
	//	   and use it
	// (3) otherwise prompt the user for a password and then remember that password in the server

	if (m_password.IsEmpty())
	{

		// case (2)
		GetRememberPassword(&rememberPassword);
		if (rememberPassword)
		{
			nsXPIDLCString password;
			GetPrefPassword(getter_Copies(password));
			m_password = password;
		}
	}
    
	*aPassword = m_password.ToNewCString();
    return rv;
}

NS_IMETHODIMP
nsMsgIncomingServer::GetPasswordWithUI(const PRUnichar * aPromptMessage, const PRUnichar *aPromptTitle, char **aPassword) 
{

    nsXPIDLCString prefvalue;
    GetPassword(getter_Copies(prefvalue));

    nsresult rv = NS_OK;
    if (m_password.IsEmpty()) {
		// prompt the user for the password
		NS_WITH_SERVICE(nsIPrompt, dialog, kNetSupportDialogCID, &rv);
		if (NS_SUCCEEDED(rv))
		{
			PRUnichar * uniPassword;
			PRBool okayValue = PR_TRUE;
			dialog->PromptPassword(aPromptMessage, aPromptTitle, &uniPassword, &okayValue);
				
			if (!okayValue) // if the user pressed cancel, just return NULL;
			{
				*aPassword = nsnull;
				return rv;
			}

			// we got a password back...so remember it
			nsCString aCStr(uniPassword); 

			SetPassword((const char *) aCStr);
		} // if we got a prompt dialog
	} // if the password is empty

	*aPassword = m_password.ToNewCString();
	return rv;
}

NS_IMETHODIMP
nsMsgIncomingServer::SetDefaultLocalPath(nsIFileSpec *aDefaultLocalPath)
{
    nsresult rv;
    nsXPIDLCString type;
    GetType(getter_Copies(type));

    nsCAutoString progid(NS_MSGPROTOCOLINFO_PROGID_PREFIX);
    progid += type;

    NS_WITH_SERVICE(nsIMsgProtocolInfo, protocolInfo, progid, &rv);
    if (NS_FAILED(rv)) return rv;

    rv = protocolInfo->SetDefaultLocalPath(aDefaultLocalPath);
    return rv;
}

NS_IMETHODIMP
nsMsgIncomingServer::GetLocalPath(nsIFileSpec **aLocalPath)
{
    nsresult rv;

    // if the local path has already been set, use it
    rv = GetFileValue("directory", aLocalPath);
    if (NS_SUCCEEDED(rv) && *aLocalPath) return rv;
    
    // otherwise, create the path using.  note we are using the
    // server key instead of the hostname
    //
    // TODO:  handle the case where they migrated a server of hostname "server4"
    // and we create a server (with the account wizard) with key "server4"
    // we'd get a collision.
    // need to modify the code that creates keys to check for disk collision
    nsXPIDLCString type;
    GetType(getter_Copies(type));

    nsCAutoString progid(NS_MSGPROTOCOLINFO_PROGID_PREFIX);
    progid += type;

    NS_WITH_SERVICE(nsIMsgProtocolInfo, protocolInfo, progid, &rv);
    if (NS_FAILED(rv)) return rv;
    
    nsCOMPtr<nsIFileSpec> path;
    rv = protocolInfo->GetDefaultLocalPath(getter_AddRefs(path));
    if (NS_FAILED(rv)) return rv;
    
    path->CreateDir();
    
    nsXPIDLCString key;
    rv = GetKey(getter_Copies(key));
    if (NS_FAILED(rv)) return rv;
    rv = path->AppendRelativeUnixPath(key);
    if (NS_FAILED(rv)) return rv;
    rv = SetLocalPath(path);
    if (NS_FAILED(rv)) return rv;

    *aLocalPath = path;
    NS_ADDREF(*aLocalPath);

    return NS_OK;
}

NS_IMETHODIMP
nsMsgIncomingServer::SetLocalPath(nsIFileSpec *spec)
{
    if (spec) {
        spec->CreateDir();
        return SetFileValue("directory", spec);
    }
    else {
        return NS_ERROR_NULL_POINTER;
    }
}

NS_IMETHODIMP
nsMsgIncomingServer::SetRememberPassword(PRBool value)
{
    if (value)
        SetPrefPassword(m_password);
    else
        SetPrefPassword(nsnull);
    return SetBoolValue("remember_password", value);
}

NS_IMETHODIMP
nsMsgIncomingServer::GetRememberPassword(PRBool* value)
{
    return GetBoolValue("remember_password", value);
}

NS_IMETHODIMP
nsMsgIncomingServer::GetLocalStoreType(char **aResult)
{
    NS_NOTYETIMPLEMENTED("nsMsgIncomingServer superclass not implementing GetLocalStoreType!");
    return NS_ERROR_UNEXPECTED;
}

// use the convenience macros to implement the accessors
NS_IMPL_SERVERPREF_STR(nsMsgIncomingServer, HostName, "hostname");
NS_IMPL_SERVERPREF_INT(nsMsgIncomingServer, Port, "port");
NS_IMPL_SERVERPREF_STR(nsMsgIncomingServer, Username, "userName");
NS_IMPL_SERVERPREF_STR(nsMsgIncomingServer, PrefPassword, "password");
NS_IMPL_SERVERPREF_BOOL(nsMsgIncomingServer, DoBiff, "check_new_mail");
NS_IMPL_SERVERPREF_INT(nsMsgIncomingServer, BiffMinutes, "check_time");
NS_IMPL_SERVERPREF_STR(nsMsgIncomingServer, Type, "type");
// in 4.x, this was "mail.pop3_gets_new_mail" for pop and 
// "mail.imap.new_mail_get_headers" for imap (it was global)
// in 5.0, this will be per server, and it will be "download_on_biff"
NS_IMPL_SERVERPREF_BOOL(nsMsgIncomingServer, DownloadOnBiff, "download_on_biff");
NS_IMPL_SERVERPREF_BOOL(nsMsgIncomingServer, Valid, "valid");
