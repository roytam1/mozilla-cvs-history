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

/*
    A test file to check default URL parsing.
	-Gagan Saksena 03/25/99
*/

#include <stdio.h>

#include "plstr.h"
#include "nsIServiceManager.h"
#include "nsIIOService.h"
#include "nsIURL.h"
#include "nsCOMPtr.h"
#include "iostream.h"
#include "nsXPIDLString.h"

// Define CIDs...
static NS_DEFINE_CID(kIOServiceCID,              NS_IOSERVICE_CID);
static NS_DEFINE_CID(kStdURLCID, 				 NS_STANDARDURL_CID);

int writeout(const char* i_pURL, PRBool bUseStd =PR_TRUE)
{
    if (i_pURL)
    {
	    cout << "Analyzing " << i_pURL << endl;

	    nsCOMPtr<nsIURI> pURL;
        nsresult result = NS_OK;
		
		if (bUseStd) 
		{
			nsIURI* url;
			result = nsComponentManager::CreateInstance(kStdURLCID, nsnull, 
				nsCOMTypeInfo<nsIURI>::GetIID(), (void**)&url);
			if (NS_FAILED(result))
			{
				cout << "CreateInstance failed" << endl;
				return result;
			}
			pURL = url;
			pURL->SetSpec((char*)i_pURL);
		}
		else 
		{
			NS_WITH_SERVICE(nsIIOService, pService, kIOServiceCID, &result);
			if (NS_FAILED(result)) 
			{
				cout << "Service failed!" << endl;
				return result;
			}	

			result = pService->NewURI(i_pURL, nsnull, getter_AddRefs(pURL));
		}
	    if (NS_SUCCEEDED(result))
	    {
		    nsXPIDLCString temp;
		    PRInt32 port;
		    pURL->GetScheme(getter_Copies(temp));
		    cout << "Got    " << (temp ? (const char*)temp : "") << ',';
		    pURL->GetPreHost(getter_Copies(temp));
		    cout << (temp ? (const char*)temp : "") << ',';
		    pURL->GetHost(getter_Copies(temp));
		    cout << (temp ? (const char*)temp : "") << ',';
		    pURL->GetPort(&port);
		    cout << port << ',';
		    pURL->GetPath(getter_Copies(temp));
		    cout << (temp ? (const char*)temp : "") << endl;
	    } else {
		    cout << "Can not create URL" << endl; 
	    }
        return NS_OK;
    }
    return -1;
}

/* construct a url and print out its five elements separated by commas */
nsresult testURL(const char* i_pURL, PRBool bUseStd=PR_TRUE)
{

	if (i_pURL)
	    return writeout(i_pURL, bUseStd);

	/* 
		If you add a test case then make sure you also add the expected
	   	result in the resultset as well. 
	*/

    const int tests = 12;
    const char* url[tests] = 
	{
		"http://username:password@hostname.com:80/pathname/./more/stuff/../path",
		"username@host:8080/path",
		"http://gagan/",
		"scheme:host/netlib", 
		"", //empty string
		"mailbox:///foo", // No host specified path should be /foo
		"scheme:user@hostname.edu:80/pathname", //this is always http:user and not user:pass
		"http://username:password@hostname:80/pathname",
		"resource:/pathname",
		"ftp://uname%here.com:pwd@there.com/aPath/a.html",
		"http://www.inf.bme.hu?foo=bar",
		"http://test.com/aPath/a.html#/1/2"
	};

	const char* resultset[tests] =
	{
		"http,username:password,hostname.com,80,/pathname/more/path",
		",username,host,8080,/path",
		"http,,gagan,-1,/",
		"scheme,,host,-1,/netlib",
		",,,-1,/",
		"mailbox,,,-1,/foo",
		"scheme,user,hostname.edu,80,/pathname",
		"http,username:password,hostname,80,/pathname",
		"resource,,,-1,/pathname",
		"ftp,uname%here.com:pwd,there.com,21,/aPath/a.html",
		"http,,www.inf.bme.hu,-1,/?foo=bar",
		"http,,test.com,-1,/aPath/a.html#/1/2"
	};

	// These tests will fail to create a URI from NS_NewURI calls...
	// because of a missing scheme: in front. This set assumes
	// an only working http handler is available. When we switch on mail these
    // results will change!
	PRBool failWithURI[tests] =
	{
		PR_FALSE,
		PR_TRUE,
		PR_FALSE,
		PR_TRUE,
		PR_TRUE,
		PR_TRUE,
		PR_TRUE,
		PR_FALSE,
		PR_FALSE,
		PR_FALSE,
        PR_FALSE
	};
	nsresult stat;
	for (int i = 0; i< tests; ++i)
	{
		cout << "--------------------" << endl;
		if (!bUseStd)
			cout << "Should" << (failWithURI[i] ? " not " : " ")
				<< "create URL" << endl;
		stat = writeout(url[i], bUseStd);
		if (NS_FAILED(stat))
			return stat;
		if (bUseStd || !failWithURI[i])
			cout << "Expect " << resultset[i] << endl << endl;
	}
	return 0;
}

int makeAbsTest(const char* i_BaseURI, const char* relativePortion)
{
    if (!i_BaseURI)
        return -1;
    
    nsIURI* baseURL;

	nsresult status = nsComponentManager::CreateInstance(kStdURLCID, nsnull, 
		nsCOMTypeInfo<nsIURI>::GetIID(), (void**)&baseURL);
	if (NS_FAILED(status))
	{
		cout << "CreateInstance failed" << endl;
		return status;
	}
	status = baseURL->SetSpec((char*)i_BaseURI);
    if (NS_FAILED(status)) return status;

    nsCOMPtr<nsIURI> url;
    nsresult rv = baseURL->Clone(getter_AddRefs(url));
    if (NS_FAILED(rv) || !url) return -1;
    
    rv = url->SetRelativePath(relativePortion);
    if (NS_FAILED(rv)) return -1;

    nsXPIDLCString temp;
    baseURL->GetSpec(getter_Copies(temp));

    cout << "Analyzing " << temp << endl;
    cout << "With " << relativePortion << endl;
    
    url->GetSpec(getter_Copies(temp));
    cout << "Got    " <<  temp << endl;
    return 0;
}

int doMakeAbsTest(const char* i_URL = 0, const char* i_relativePortion=0)
{
    if (i_URL && i_relativePortion)
    {
        return makeAbsTest(i_URL, i_relativePortion);
    }

    // Run standard tests. These tests are based on the ones described in 
    // rfc1808
    /* Section 5.1.  Normal Examples

      g:h        = <URL:g:h>
      g          = <URL:http://a/b/c/g>
      ./g        = <URL:http://a/b/c/g>
      g/         = <URL:http://a/b/c/g/>
      /g         = <URL:http://a/g>
      //g        = <URL:http://g>
      ?y         = <URL:http://a/b/c/d;p?y>
      g?y        = <URL:http://a/b/c/g?y>
      g?y/./x    = <URL:http://a/b/c/g?y/./x>
      #s         = <URL:http://a/b/c/d;p?q#s>
      g#s        = <URL:http://a/b/c/g#s>
      g#s/./x    = <URL:http://a/b/c/g#s/./x>
      g?y#s      = <URL:http://a/b/c/g?y#s>
      ;x         = <URL:http://a/b/c/d;x>
      g;x        = <URL:http://a/b/c/g;x>
      g;x?y#s    = <URL:http://a/b/c/g;x?y#s>
      .          = <URL:http://a/b/c/>
      ./         = <URL:http://a/b/c/>
      ..         = <URL:http://a/b/>
      ../        = <URL:http://a/b/>
      ../g       = <URL:http://a/b/g>
      ../..      = <URL:http://a/>
      ../../     = <URL:http://a/>
      ../../g    = <URL:http://a/g>

    */

    const char baseURL[] = "http://a/b/c/d;p?q#f";

    struct test {
        const char* relativeURL;
        const char* expectedResult;
    };

    test tests[] = {
        // Tests from rfc1808, section 5.1
        { "g:h",                "g:h" },
        { "g",                  "http://a/b/c/g" },
        { "./g",                "http://a/b/c/g" },
        { "g/",                 "http://a/b/c/g/" },
        { "/g",                 "http://a/g" },
        { "//g",                "http://g" },
        { "?y",                 "http://a/b/c/d;p?y" },
        { "g?y",                "http://a/b/c/g?y" },
        { "g?y/./x",            "http://a/b/c/g?y/./x" },
        { "#s",                 "http://a/b/c/d;p?q#s" },
        { "g#s",                "http://a/b/c/g#s" },
        { "g#s/./x",            "http://a/b/c/g#s/./x" },
        { "g?y#s",              "http://a/b/c/g?y#s" },
        { ";x",                 "http://a/b/c/d;x" },
        { "g;x",                "http://a/b/c/g;x" },
        { "g;x?y#s",            "http://a/b/c/g;x?y#s" },
        { ".",                  "http://a/b/c/" },
        { "./",                 "http://a/b/c/" },
        { "..",                 "http://a/b/" },
        { "../",                "http://a/b/" },
        { "../g",               "http://a/b/g" },
        { "../..",              "http://a/" },
        { "../../",             "http://a/" },
        { "../../g",            "http://a/g" },

        // Our additional tests...
        { "#my::anchor",        "http://a/b/c/d;p?q#my::anchor" },

        // Make sure relative query's work right even if the query
        // string contains absolute urls or other junk.
        { "?http://foo",        "http://a/b/c/d;p?http://foo" },
        { "g?http://foo",       "http://a/b/c/g?http://foo" },
        { "g/h?http://foo",     "http://a/b/c/g/h?http://foo" },
        { "g/h/../H?http://foo","http://a/b/c/g/H?http://foo" },
        { "g/h/../H?http://foo?baz", "http://a/b/c/g/H?http://foo?baz" },
        { "g/h/../H?http://foo;baz", "http://a/b/c/g/H?http://foo;baz" },
        { "g/h/../H?http://foo#bar", "http://a/b/c/g/H?http://foo#bar" },
        { "g/h/../H;baz?http://foo", "http://a/b/c/g/H;baz?http://foo" },
        { "g/h/../H;baz?http://foo#bar", "http://a/b/c/g/H;baz?http://foo#bar" },
    };

    const int numTests = sizeof(tests) / sizeof(tests[0]);

    for (int i = 0 ; i<numTests ; ++i)
    {
        makeAbsTest(baseURL, tests[i].relativeURL);
        cout << "Expect " << tests[i].expectedResult << endl << endl;
    }
    return 0;
}

nsresult NS_AutoregisterComponents()
{
  nsresult rv = nsComponentManager::AutoRegister(nsIComponentManager::NS_Startup, NULL /* default */);
  return rv;
}

void printusage(void)
{
    printf("urltest [-std] [-all] <URL> [-abs <relative>]\n");
    printf("\n");
    printf("    -std  : Generate results using nsStdURL. \n");
    printf("    <URL> : The string representing the URL. \n");
    printf("    -all  : Run all standard tests. Ignores <URL> then. \n");
    printf("    -abs  : Make an absolute URL from the base (<URI>) and the\n"); 
    printf("            relative path specified. Can be used with -all. Implies -std.\n");
}

int main(int argc, char **argv)
{
    nsresult result = NS_OK;

    if (argc < 2) {
        printusage();
        return 0;
    }

    
    result = NS_AutoregisterComponents();
	if (NS_FAILED(result)) return result;

	cout << "------------------" << endl << endl; // end of all messages from register components...

	PRBool bStdTest= PR_FALSE;
	PRBool bTestAll= PR_FALSE;
    PRBool bMakeAbs= PR_FALSE;
    char* relativePath = 0;
    char* url = 0;
    for (int i=1; i<argc; i++) {
        if (PL_strcasecmp(argv[i], "-std") == 0) 
		{
			bStdTest = PR_TRUE;
        }
        else if (PL_strcasecmp(argv[i], "-all") == 0) 
		{
			bTestAll = PR_TRUE;
        } 
        else if (PL_strcasecmp(argv[i], "-abs") == 0)
        {
            if (i+1 >= argc)
            {
                printusage(); 
                return 0;
            }
            relativePath = argv[i+1]; 
            bMakeAbs = PR_TRUE;
            i++;
        }
        else
        {
            url = argv[i];
        }
	}
    if (bMakeAbs)
    {
        return bTestAll ? doMakeAbsTest() : doMakeAbsTest(url, relativePath); 
    }
    else
    {
	    return bTestAll ? testURL(0, bStdTest) : testURL(url, bStdTest);
    }
}
