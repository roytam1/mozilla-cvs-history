/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 * Contributor(s): 
 *   Doug Turner <dougt@netscape.com>
 */

#include "nsTimeBomb.h"
#include "nsIGenericFactory.h"
#include "nsIServiceManager.h"
#include "nsIPref.h"
#include "nspr.h"
#include "plstr.h"

#include "nsIWebShellWindow.h"
#include "nsIAppShellService.h"
#include "nsAppShellCIDs.h"
#include "nsIBrowserWindow.h"

#include "nsIIOService.h"
#include "nsNetUtil.h"

static NS_DEFINE_CID(kPrefCID, NS_PREF_CID);
static NS_DEFINE_CID(kAppShellServiceCID, NS_APPSHELL_SERVICE_CID);


static nsresult DisplayURI(const char *urlStr, PRBool block)
{
    
    nsresult rv;
    nsCOMPtr<nsIURI> URL;
    
    rv = NS_NewURI(getter_AddRefs(URL), (const char *)urlStr);
    if (NS_FAILED(rv)) return rv;

    NS_WITH_SERVICE(nsIAppShellService, appShell, kAppShellServiceCID, &rv);
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIWebShellWindow>  window;
    rv = appShell->CreateTopLevelWindow(nsnull, 
                                        URL,
                                        PR_TRUE, 
                                        PR_TRUE, 
                                        NS_CHROME_ALL_CHROME,
                                        nsnull, 
						                NS_SIZETOCONTENT,           // width 
						                NS_SIZETOCONTENT,           // height
                                        getter_AddRefs(window));

    if (NS_FAILED(rv)) return rv;

    /*
     * Start up the main event loop...
     */	
    if (block)
        rv = appShell->Run();
    
    return rv;
}


class nsTimeBomb : public nsITimeBomb
{
public:

	nsTimeBomb();
	virtual ~nsTimeBomb();

	NS_DECL_ISUPPORTS    
    NS_DECL_NSITIMEBOMB

protected:
    nsCOMPtr<nsIPref> mPrefs;
    nsresult GetInt64ForPref(const char* pref, PRInt64* time);
};

nsTimeBomb::nsTimeBomb()
{
    NS_INIT_REFCNT();
}

nsTimeBomb::~nsTimeBomb()
{
}

NS_IMPL_ISUPPORTS(nsTimeBomb, NS_GET_IID(nsITimeBomb));

NS_IMETHODIMP
nsTimeBomb::Init()
{
    nsresult rv;

    rv = nsServiceManager::GetService(kPrefCID, NS_GET_IID(nsIPref), getter_AddRefs(mPrefs));
    NS_ASSERTION(NS_SUCCEEDED(rv), "failed to get prefs");
	
    PRTime time = LL_Zero();
    rv = GetFirstLaunch(&time);
    if (NS_FAILED(rv))
    {
        time = PR_Now();
        char buffer[30];
        PR_snprintf(buffer, 30, "%lld", time);
        mPrefs->SetCharPref("timebomb.first_launch_time", buffer);
        rv = NS_OK;
    }
    return rv;
}

NS_IMETHODIMP
nsTimeBomb::CheckWithUI(PRBool *expired)
{
    *expired = PR_FALSE;

    PRBool val;
    nsresult rv = GetEnabled(&val);

    if (NS_FAILED(rv) && !val)
    {
        // was not set or not enabled.  
        // no problems.  just return okay.
        return NS_OK;
    }

    rv = GetExpired(&val);
    
    if (NS_SUCCEEDED(rv) && val)
    {
        printf("********  Expired version  ********\n");
        DisplayURI("chrome://timebomb/content/expireText.xul", PR_TRUE);
        *expired = PR_TRUE;
        return NS_OK;
    }

    rv = GetWarned(&val);
    
    if (NS_SUCCEEDED(rv) && val)
    {
        printf("********  ABOUT TO EXPIRE  ********\n");
        DisplayURI("chrome://timebomb/content/warnText.xul", PR_TRUE);
    }

    return NS_OK;
    
}

NS_IMETHODIMP
nsTimeBomb::LoadUpdateURL()
{
    char* url;
    GetTimebombURL(&url);
    nsresult rv = DisplayURI(url, PR_FALSE);
    nsAllocator::Free(url);
    return rv;
}

NS_IMETHODIMP
nsTimeBomb::GetExpired(PRBool *expired)
{
    *expired = PR_FALSE;

    PRTime bombTime    = LL_Zero();
    PRTime currentTime = PR_Now();
    
    // check absolute expiration;

    nsresult rv = GetExpirationTime(&bombTime);

    if (NS_FAILED(rv)) return NS_OK;
    
    if (LL_CMP(bombTime, <, currentTime)) 
    {    
        *expired = PR_TRUE;       
        return NS_OK;
    }
    
    // try relative expiration;

    PRTime offsetTime   = LL_Zero();
    PRTime offset       = LL_Zero();
    
    rv = GetBuildTime(&bombTime);
    if (NS_FAILED(rv)) return NS_OK;
    rv = GetExpirationOffset(&offset);
    if (NS_FAILED(rv)) return NS_OK;

    LL_ADD(offsetTime, bombTime, offset);
    if (LL_CMP(offsetTime, <, currentTime)) 
    {    
        *expired = PR_FALSE;
        return NS_OK;
    }

    rv = GetFirstLaunch(&bombTime);
    if (NS_FAILED(rv)) return NS_OK;
    
    LL_ADD(offsetTime, bombTime, offset);
    if (LL_CMP(offsetTime, <, currentTime)) 
    {    
        *expired = PR_FALSE;
        return NS_OK;
    }


    return NS_OK;
}

NS_IMETHODIMP
nsTimeBomb::GetWarned(PRBool *warn)
{
    *warn = PR_FALSE;

    PRTime bombTime    = LL_Zero();
    PRTime currentTime = PR_Now();
    
    // check absolute expiration;

    nsresult rv = GetWarningTime(&bombTime);

    if (NS_FAILED(rv)) return NS_OK;
    
    if (LL_CMP(bombTime, <, currentTime)) 
    {    
        *warn = PR_TRUE;       
        return NS_OK;
    }
    
    // try relative expiration;

    PRTime offsetTime   = LL_Zero();
    PRTime offset       = LL_Zero();
    
    rv = GetBuildTime(&bombTime);
    if (NS_FAILED(rv)) return NS_OK;
    rv = GetWarningOffset(&offset);
    if (NS_FAILED(rv)) return NS_OK;

    LL_ADD(offsetTime, bombTime, offset);
    if (LL_CMP(offsetTime, <, currentTime)) 
    {    
        *warn = PR_FALSE;
        return NS_OK;
    }

    rv = GetFirstLaunch(&bombTime);
    if (NS_FAILED(rv)) return NS_OK;
    
    LL_ADD(offsetTime, bombTime, offset);
    if (LL_CMP(offsetTime, <, currentTime)) 
    {    
        *warn = PR_FALSE;
        return NS_OK;
    }

    return NS_OK;
}


NS_IMETHODIMP
nsTimeBomb::GetEnabled(PRBool *enabled)
{
    return mPrefs->GetBoolPref("timebomb.enabled",enabled);
}


NS_IMETHODIMP
nsTimeBomb::GetExpirationTime(PRTime *time)
{
    return GetInt64ForPref("timebomb.expiration_time", time);
}


NS_IMETHODIMP
nsTimeBomb::GetWarningTime(PRTime *time)
{
    return GetInt64ForPref("timebomb.warning_time", time);
}


NS_IMETHODIMP
nsTimeBomb::GetBuildTime(PRTime *time)
{
    return GetInt64ForPref("timebomb.build_time", time);
}

NS_IMETHODIMP
nsTimeBomb::GetFirstLaunch(PRTime *time)
{
    return GetInt64ForPref("timebomb.first_launch_time", time);
}


NS_IMETHODIMP
nsTimeBomb::GetWarningOffset(PRInt64 *offset)
{
    return GetInt64ForPref("timebomb.warning_offset", offset);
}

NS_IMETHODIMP
nsTimeBomb::GetExpirationOffset(PRInt64 *offset)
{
    return GetInt64ForPref("timebomb.expiration_offset", offset);
}



NS_IMETHODIMP
nsTimeBomb::GetTimebombURL(char* *url)
{
    char* string;
    nsresult rv = mPrefs->CopyCharPref("timebomb.update_url", &string);
    if (NS_SUCCEEDED(rv))
    {
        *url = (char*)nsAllocator::Clone(string, (strlen(string)+1)*sizeof(char));
        if(*url)
            return NS_ERROR_OUT_OF_MEMORY;
        return NS_OK;
    }
   
    string = "http://www.mozilla.org/projects/seamonkey/";
    *url = (char*)nsAllocator::Clone(string, (strlen(string)+1)*sizeof(char));
    
    if(*url)
        return NS_ERROR_OUT_OF_MEMORY;

    return NS_OK;
}



nsresult 
nsTimeBomb::GetInt64ForPref(const char* pref, PRInt64* time)
{
    char* string;
    nsresult rv = mPrefs->CopyCharPref(pref, &string);
    if (NS_SUCCEEDED(rv))
    {
        PR_sscanf(string, "%lld", time);
        PL_strfree(string);
    }
    return rv;
}


NS_GENERIC_FACTORY_CONSTRUCTOR(nsTimeBomb)

static nsModuleComponentInfo components[] =
{
  { "Netscape TimeBomb", 
    NS_TIMEBOMB_CID, 
    NS_TIMEBOMB_PROGID, 
    nsTimeBombConstructor
  },

};

NS_IMPL_NSGETMODULE("nsTimeBomb", components)