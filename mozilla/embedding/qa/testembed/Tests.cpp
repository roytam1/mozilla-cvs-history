/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   David Epstein <depstein@netscape.com> 
 *   Dharma Sirnapalli <dsirnapalli@netscape.com>
 *	 Ashish Bhatt <ashishbhatt@netscape.com>
 *
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

// File Overview....
//
// These are QA test case implementations
//

#include "stdafx.h"
#include "TestEmbed.h"
#include "BrowserImpl.h"
#include "BrowserFrm.h"
#include "UrlDialog.h"
#include "ProfileMgr.h"
#include "ProfilesDlg.h"
#include "Tests.h"
#include "nsihistory.h"
#include "nsiwebnav.h"
#include "nsirequest.h"
#include "nsidirserv.h"
#include "domwindow.h"
#include "selection.h"
#include "nsProfile.h"
#include "nsIClipboardCmd.h"
#include "nsIObserServ.h"

#include "QaUtils.h"
#include <stdio.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

nsresult rv;

// Register message for FindDialog communication
static UINT WM_FINDMSG = ::RegisterWindowMessage(FINDMSGSTRING);

BEGIN_MESSAGE_MAP(CTests, CWnd)
	//{{AFX_MSG_MAP(CTests)
	ON_COMMAND(ID_TESTS_CHANGEURL, OnTestsChangeUrl)
	ON_COMMAND(ID_TESTS_GLOBALHISTORY, OnTestsGlobalHistory)
	ON_COMMAND(ID_TESTS_CREATEFILE, OnTestsCreateFile)
	ON_COMMAND(ID_TESTS_CREATEPROFILE, OnTestsCreateprofile)
	ON_COMMAND(ID_TESTS_ADDWEBPROGLISTENER, OnTestsAddWebProgListener)
	ON_COMMAND(ID_TESTS_ADDHISTORYLISTENER, OnTestsAddHistoryListener)
	ON_COMMAND(ID_INTERFACES_NSIFILE, OnInterfacesNsifile)
	ON_COMMAND(ID_TOOLS_REMOVEGHPAGE, OnToolsRemoveGHPage)
	ON_COMMAND(ID_TOOLS_REMOVEALLGH, OnToolsRemoveAllGH)
	ON_COMMAND(ID_TOOLS_TESTYOURMETHOD, OnToolsTestYourMethod)
	ON_COMMAND(ID_TOOLS_TESTYOURMETHOD2, OnToolsTestYourMethod2)
	ON_COMMAND(ID_VERIFYBUGS_70228, OnVerifybugs70228)
	ON_COMMAND(ID_INTERFACES_NSIREQUEST_CANCEL, OnInterfacesNsirequest)
	ON_COMMAND(ID_INTERFACES_NSIDOMWINDOW_RUNALLTESTS, OnInterfacesNsidomwindow)
	ON_COMMAND(ID_INTERFACES_NSIDIRECTORYSERVICE_INIT, OnInterfacesNsidirectoryservice)
	ON_COMMAND(ID_INTERFACES_NSISELECTION_RUNALLTESTS, OnInterfacesNsiselection)
	ON_COMMAND(ID_VERIFYBUGS_90195, OnVerifybugs90195)
	ON_COMMAND(ID_INTERFACES_NSIPROFILE_RUNALLTESTS, OnInterfacesNsiprofile)
	ON_COMMAND(ID_INTERFACES_NSISHISTORY_GETCOUNT, OnInterfacesNsishistory)
	ON_COMMAND(ID_TESTS_REMOVEHISTORYLISTENER, OnTestsRemovehistorylistener)
	ON_COMMAND(ID_INTERFACES_NSIWEBNAV_GETCANGOBACK, OnInterfacesNsiwebnav)
	ON_COMMAND(ID_INTERFACES_NSICLIPBOARDCOMMANDS_CANCOPYSELECTION, OnInterfacesNsiclipboardcommands)
	ON_COMMAND(ID_INTERFACES_NSIDIRECTORYSERVICE_REGISTERPROVIDER, OnInterfacesNsidirectoryservice)
	ON_COMMAND(ID_INTERFACES_NSIDIRECTORYSERVICE_RUNALLTESTS, OnInterfacesNsidirectoryservice)
	ON_COMMAND(ID_INTERFACES_NSIDIRECTORYSERVICE_UNREGISTERPROVIDER, OnInterfacesNsidirectoryservice)
	ON_COMMAND(ID_INTERFACES_NSIDOMWINDOW_GETDOMDOCUMENT, OnInterfacesNsidomwindow)
	ON_COMMAND(ID_INTERFACES_NSIDOMWINDOW_GETFRAMES, OnInterfacesNsidomwindow)
	ON_COMMAND(ID_INTERFACES_NSIDOMWINDOW_GETNAME, OnInterfacesNsidomwindow)
	ON_COMMAND(ID_INTERFACES_NSIDOMWINDOW_GETPARENT, OnInterfacesNsidomwindow)
	ON_COMMAND(ID_INTERFACES_NSIDOMWINDOW_GETSCROLLBARS, OnInterfacesNsidomwindow)
	ON_COMMAND(ID_INTERFACES_NSIDOMWINDOW_GETSCROLLY, OnInterfacesNsidomwindow)
	ON_COMMAND(ID_INTERFACES_NSIDOMWINDOW_GETSCSOLLX, OnInterfacesNsidomwindow)
	ON_COMMAND(ID_INTERFACES_NSIDOMWINDOW_GETSELECTION, OnInterfacesNsidomwindow)
	ON_COMMAND(ID_INTERFACES_NSIDOMWINDOW_SCROLLBY, OnInterfacesNsidomwindow)
	ON_COMMAND(ID_INTERFACES_NSIDOMWINDOW_SCROLLBYLINES, OnInterfacesNsidomwindow)
	ON_COMMAND(ID_INTERFACES_NSIDOMWINDOW_SCROLLBYPAGES, OnInterfacesNsidomwindow)
	ON_COMMAND(ID_INTERFACES_NSIDOMWINDOW_SCROLLTO, OnInterfacesNsidomwindow)
	ON_COMMAND(ID_INTERFACES_NSIDOMWINDOW_SIZETOCONTENT, OnInterfacesNsidomwindow)
	ON_COMMAND(ID_INTERFACES_NSISELECTION_GETANCHORNODE, OnInterfacesNsiselection)
	ON_COMMAND(ID_INTERFACES_NSISELECTION_ADDRANGE, OnInterfacesNsiselection)
	ON_COMMAND(ID_INTERFACES_NSISELECTION_COLLAPSE, OnInterfacesNsiselection)
	ON_COMMAND(ID_INTERFACES_NSISELECTION_COLLAPSETOEND, OnInterfacesNsiselection)
	ON_COMMAND(ID_INTERFACES_NSISELECTION_COLLAPSETOSTART, OnInterfacesNsiselection)
	ON_COMMAND(ID_INTERFACES_NSISELECTION_CONTAINSNODE, OnInterfacesNsiselection)
	ON_COMMAND(ID_INTERFACES_NSISELECTION_DELETEFROMDOCUMENT, OnInterfacesNsiselection)
	ON_COMMAND(ID_INTERFACES_NSISELECTION_EXTEND, OnInterfacesNsiselection)
	ON_COMMAND(ID_INTERFACES_NSISELECTION_GETANCHOROFFSET, OnInterfacesNsiselection)
	ON_COMMAND(ID_INTERFACES_NSISELECTION_GETFOCUSNODE, OnInterfacesNsiselection)
	ON_COMMAND(ID_INTERFACES_NSISELECTION_GETFOCUSOFFSET, OnInterfacesNsiselection)
	ON_COMMAND(ID_INTERFACES_NSISELECTION_GETISCOLLAPSED, OnInterfacesNsiselection)
	ON_COMMAND(ID_INTERFACES_NSISELECTION_GETRANGEAT, OnInterfacesNsiselection)
	ON_COMMAND(ID_INTERFACES_NSISELECTION_GETRANGECOUNT, OnInterfacesNsiselection)
	ON_COMMAND(ID_INTERFACES_NSISELECTION_REMOVEALLRANGES, OnInterfacesNsiselection)
	ON_COMMAND(ID_INTERFACES_NSISELECTION_REMOVERANGE, OnInterfacesNsiselection)
	ON_COMMAND(ID_INTERFACES_NSISELECTION_SELECTALLCHILDREN, OnInterfacesNsiselection)
	ON_COMMAND(ID_INTERFACES_NSISELECTION_SELECTIONLANGUAGECHANGE, OnInterfacesNsiselection)
	ON_COMMAND(ID_INTERFACES_NSISELECTION_TOSTRING, OnInterfacesNsiselection)
	ON_COMMAND(ID_INTERFACES_NSIPROFILE_CLONEPROFILE, OnInterfacesNsiprofile)
	ON_COMMAND(ID_INTERFACES_NSIPROFILE_CREATENEWPROFILE, OnInterfacesNsiprofile)
	ON_COMMAND(ID_INTERFACES_NSIPROFILE_DELETEPROFILE, OnInterfacesNsiprofile)
	ON_COMMAND(ID_INTERFACES_NSIPROFILE_GETCURRENTPROFILE, OnInterfacesNsiprofile)
	ON_COMMAND(ID_INTERFACES_NSIPROFILE_GETPROFILECOUNT, OnInterfacesNsiprofile)
	ON_COMMAND(ID_INTERFACES_NSIPROFILE_GETPROFILELIST, OnInterfacesNsiprofile)
	ON_COMMAND(ID_INTERFACES_NSIPROFILE_PROFILEEXISTS, OnInterfacesNsiprofile)
	ON_COMMAND(ID_INTERFACES_NSIPROFILE_RENAMEPROFILE, OnInterfacesNsiprofile)
	ON_COMMAND(ID_INTERFACES_NSIPROFILE_SETCURRENTPROFILE, OnInterfacesNsiprofile)
	ON_COMMAND(ID_INTERFACES_NSIPROFILE_SHUTDOWNCURRENTPROFILE, OnInterfacesNsiprofile)
	ON_COMMAND(ID_INTERFACES_NSISHISTORY_GETENTRYATINDEX, OnInterfacesNsishistory)
	ON_COMMAND(ID_INTERFACES_NSISHISTORY_GETINDEX, OnInterfacesNsishistory)
	ON_COMMAND(ID_INTERFACES_NSISHISTORY_GETMAXLENGTH, OnInterfacesNsishistory)
	ON_COMMAND(ID_INTERFACES_NSISHISTORY_GETSHISTORYENUMERATOR, OnInterfacesNsishistory)
	ON_COMMAND(ID_INTERFACES_NSISHISTORY_PURGEHISTORY, OnInterfacesNsishistory)
	ON_COMMAND(ID_INTERFACES_NSISHISTORY_RUNALLTESTS, OnInterfacesNsishistory)
	ON_COMMAND(ID_INTERFACES_NSISHISTORY_SETMAXLENGTH, OnInterfacesNsishistory)
	ON_COMMAND(ID_INTERFACES_NSISHISTORY_NSIHISTORYENTRY_GETISSUBFRAME, OnInterfacesNsishistory)
	ON_COMMAND(ID_INTERFACES_NSISHISTORY_NSIHISTORYENTRY_GETTITLE, OnInterfacesNsishistory)
	ON_COMMAND(ID_INTERFACES_NSISHISTORY_NSIHISTORYENTRY_GETURI, OnInterfacesNsishistory)
	ON_COMMAND(ID_INTERFACES_NSISHISTORY_NSIHISTORYENTRY_RUNALLTESTS, OnInterfacesNsishistory)
	ON_COMMAND(ID_INTERFACES_NSIWEBNAV_GETCANGOFORWARD, OnInterfacesNsiwebnav)
	ON_COMMAND(ID_INTERFACES_NSIWEBNAV_GETCURRENTURI, OnInterfacesNsiwebnav)
	ON_COMMAND(ID_INTERFACES_NSIWEBNAV_GETDOCUMENT, OnInterfacesNsiwebnav)
	ON_COMMAND(ID_INTERFACES_NSIWEBNAV_GETSESSIONHISTORY, OnInterfacesNsiwebnav)
	ON_COMMAND(ID_INTERFACES_NSIWEBNAV_GOBACK, OnInterfacesNsiwebnav)
	ON_COMMAND(ID_INTERFACES_NSIWEBNAV_GOFORWARD, OnInterfacesNsiwebnav)
	ON_COMMAND(ID_INTERFACES_NSIWEBNAV_GOTOINDEX, OnInterfacesNsiwebnav)
	ON_COMMAND(ID_INTERFACES_NSIWEBNAV_LOADURI, OnInterfacesNsiwebnav)
	ON_COMMAND(ID_INTERFACES_NSIWEBNAV_RELOAD, OnInterfacesNsiwebnav)
	ON_COMMAND(ID_INTERFACES_NSIWEBNAV_RUNALLTESTS, OnInterfacesNsiwebnav)
	ON_COMMAND(ID_INTERFACES_NSIWEBNAV_SETSESSIONHISTORY, OnInterfacesNsiwebnav)
	ON_COMMAND(ID_INTERFACES_NSIWEBNAV_STOP, OnInterfacesNsiwebnav)
	ON_COMMAND(ID_INTERFACES_NSIREQUEST_GETLOADFLAGS, OnInterfacesNsirequest)
	ON_COMMAND(ID_INTERFACES_NSIREQUEST_GETLOADGROUP, OnInterfacesNsirequest)
	ON_COMMAND(ID_INTERFACES_NSIREQUEST_GETNAME, OnInterfacesNsirequest)
	ON_COMMAND(ID_INTERFACES_NSIREQUEST_GETSTATUS, OnInterfacesNsirequest)
	ON_COMMAND(ID_INTERFACES_NSIREQUEST_ISPENDING, OnInterfacesNsirequest)
	ON_COMMAND(ID_INTERFACES_NSIREQUEST_RESUME, OnInterfacesNsirequest)
	ON_COMMAND(ID_INTERFACES_NSIREQUEST_SETLOADFLAGS, OnInterfacesNsirequest)
	ON_COMMAND(ID_INTERFACES_NSIREQUEST_SETLOADGROUP, OnInterfacesNsirequest)
	ON_COMMAND(ID_INTERFACES_NSIREQUEST_SUSPEND, OnInterfacesNsirequest)
	ON_COMMAND(ID_INTERFACES_NSIREQUEST_RUNALLTESTS, OnInterfacesNsirequest)
	ON_COMMAND(ID_INTERFACES_NSIDOMWINDOW_GETTEXTZOOM, OnInterfacesNsidomwindow)
	ON_COMMAND(ID_INTERFACES_NSIDOMWINDOW_SETTEXTZOOM, OnInterfacesNsidomwindow)
	ON_COMMAND(ID_INTERFACES_NSICLIPBOARDCOMMANDS_CANCUTSELECTION, OnInterfacesNsiclipboardcommands)
	ON_COMMAND(ID_INTERFACES_NSICLIPBOARDCOMMANDS_CANPASTE, OnInterfacesNsiclipboardcommands)
	ON_COMMAND(ID_INTERFACES_NSICLIPBOARDCOMMANDS_COPYLINKLOCATION, OnInterfacesNsiclipboardcommands)
	ON_COMMAND(ID_INTERFACES_NSICLIPBOARDCOMMANDS_COPYSELECTION, OnInterfacesNsiclipboardcommands)
	ON_COMMAND(ID_INTERFACES_NSICLIPBOARDCOMMANDS_CUTSELECTION, OnInterfacesNsiclipboardcommands)
	ON_COMMAND(ID_INTERFACES_NSICLIPBOARDCOMMANDS_PASTE, OnInterfacesNsiclipboardcommands)
	ON_COMMAND(ID_INTERFACES_NSICLIPBOARDCOMMANDS_SELECTALL, OnInterfacesNsiclipboardcommands)
	ON_COMMAND(ID_INTERFACES_NSICLIPBOARDCOMMANDS_SELECTNONE, OnInterfacesNsiclipboardcommands)
	ON_COMMAND(ID_INTERFACES_NSIOBSERVERSERVICE_ADDOBSERVERS, OnInterfacesNsiobserverservice)
	ON_COMMAND(ID_INTERFACES_NSIOBSERVERSERVICE_ENUMERATEOBSERVERS, OnInterfacesNsiobserverservice)
	ON_COMMAND(ID_INTERFACES_NSIOBSERVERSERVICE_NOTIFYOBSERVERS, OnInterfacesNsiobserverservice)
	ON_COMMAND(ID_INTERFACES_NSIOBSERVERSERVICE_REMOVEOBSERVERS, OnInterfacesNsiobserverservice)
	ON_COMMAND(ID_INTERFACES_NSIOBSERVERSERVICE_RUNALLTESTS, OnInterfacesNsiobserverservice)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


CTests::CTests(nsIWebBrowser* mWebBrowser,
			   nsIBaseWindow* mBaseWindow,
			   nsIWebNavigation* mWebNav,
			   CBrowserImpl *mpBrowserImpl)
{
	qaWebBrowser = mWebBrowser;
	qaBaseWindow = mBaseWindow;
	qaWebNav = mWebNav;

	qaBrowserImpl = mpBrowserImpl;
}	

CTests::~CTests()
{
}


// depstein: Start QA test cases here
// *********************************************************
// *********************************************************

void CTests::OnTestsChangeUrl() 
{
	char *theUrl = "http://www.aol.com/";
	CUrlDialog myDialog;
	//nsresult rv;  


	if (!qaWebNav)
	{
		QAOutput("Web navigation object not found. Change URL test not performed.", 2);
		return;
	}

	if (myDialog.DoModal() == IDOK)
	{
		QAOutput("Begin Change URL test.", 1);
		strcpy(theUrl, myDialog.m_urlfield);
		rv = qaWebNav->LoadURI(NS_ConvertASCIItoUCS2(theUrl).get(), 
						nsIWebNavigation::LOAD_FLAGS_BYPASS_HISTORY,
            nsnull,
            nsnull,
            nsnull);
	    RvTestResult(rv, "rv LoadURI() test", 1);
		FormatAndPrintOutput("The url = ", theUrl, 2); 

/*
		nsCAutoString uriString;
		nsCOMPtr<nsIURI> pURI;
		// GetcurrentURI() declared in nsIP3PUI.idl
		// used with webNav obj in nsP3PObserverHTML.cpp, line 239
		// this will be used as an indep routine to verify the URI load
		rv = qaWebNav->GetCurrentURI( getter_AddRefs( pURI ) );
		if(NS_FAILED(rv) || !pURI)
			AfxMessageBox("Bad result for GetCurrentURI().");

		rv = pURI->GetSpec(uriString);
		if (NS_FAILED(rv))
			AfxMessageBox("Bad result for GetSpec().");

		AfxMessageBox("Start URL validation test().");
		if (strcmp(uriString, theUrl) == 0)
		{
			QAOutput("Url loaded successfully. Test Passed.", 2);	
		}
		else
		{
			QAOutput("Url didn't load successfully. Test Failed.", 2);
		}
*/
		QAOutput("End Change URL test.", 1);
	}
	else
		QAOutput("Change URL test not executed.", 1);

}

// *********************************************************

void CTests::OnTestsGlobalHistory() 
{
	// create instance of myHistory object. Call's XPCOM
	// service manager to pass the contract ID.

	char *theUrl = "http://www.bogussite.com/";
	CUrlDialog myDialog;

	PRBool theRetVal = PR_FALSE;
    
	//nsresult rv; 


	nsCOMPtr<nsIGlobalHistory> myHistory(do_GetService(NS_GLOBALHISTORY_CONTRACTID));

	if (!myHistory)
	{
		QAOutput("Couldn't find history object. No GH tests performed.", 2);
		return;
	}

	if (myDialog.DoModal() == IDOK)
	{
		QAOutput("Begin IsVisited() and AddPage() tests.", 2);

		strcpy(theUrl, myDialog.m_urlfield);

		FormatAndPrintOutput("The history url = ", theUrl, 1);

		// see if url is already in the GH file (pre-AddPage() test)
		rv = myHistory->IsVisited(theUrl, &theRetVal);
	    RvTestResult(rv, "rv IsVisited() test", 1);
		FormatAndPrintOutput("The IsVisited() boolean return value = ", theRetVal, 1); 

		if (theRetVal)
			QAOutput("URL has been visited. Won't execute AddPage().", 2);
		else
		{
			QAOutput("URL hasn't been visited. Will execute AddPage().", 2);

			// adds a url to the global history file
			rv = myHistory->AddPage(theUrl);

			// prints addPage() results to output file
			if (NS_FAILED(rv))
			{
				QAOutput("Invalid results for AddPage(). Url not added. Test failed.", 1);
				return;
			}
			else
				QAOutput("Valid results for AddPage(). Url added. Test passed.", 1);

			// checks if url was visited (post-AddPage() test)
 			myHistory->IsVisited(theUrl, &theRetVal);

			if (theRetVal)
				QAOutput("URL is visited; post-AddPage() test. IsVisited() test passed.", 1);
			else
				QAOutput("URL isn't visited; post-AddPage() test. IsVisited() test failed.", 1);
		}
		QAOutput("End IsVisited() and AddPage() tests.", 2);
	}
	else
		QAOutput("IsVisited() and AddPage() tests not executed.", 1);
}


// *********************************************************

void CTests::OnTestsCreateFile() 
{
   	//nsresult rv;  
	PRBool exists;
    nsCOMPtr<nsILocalFile> theTestFile(do_GetService(NS_LOCAL_FILE_CONTRACTID));

    if (!theTestFile)
	{
		QAOutput("File object doesn't exist. No File tests performed.", 2);
		return;
	}


	QAOutput("Start Create File test.", 2);

	rv = theTestFile->InitWithPath("c:\\temp\\theFile.txt");
	rv = theTestFile->Exists(&exists);

	QAOutput("File (theFile.txt) doesn't exist. We'll create it.\r\n", 1);
	rv = theTestFile->Create(nsIFile::NORMAL_FILE_TYPE, 0777);
	RvTestResult(rv, "File Create() test", 2);
}

// *********************************************************

void CTests::OnTestsCreateprofile() 
{
    CProfilesDlg    myDialog;
    nsresult        rv;

	if (myDialog.DoModal() == IDOK)
    {       
//      NS_WITH_SERVICE(nsIProfile, profileService, NS_PROFILE_CONTRACTID, &rv);
		nsCOMPtr<nsIProfile> theProfServ(do_GetService(NS_PROFILE_CONTRACTID,&rv));
		if (NS_FAILED(rv))
		{
		   QAOutput("Didn't get profile service. No profile tests performed.", 2);
		   return;
		}

	   QAOutput("Start Profile switch test.", 2);

	   QAOutput("Retrieved profile service.", 2);
       rv = theProfServ->SetCurrentProfile(myDialog.m_SelectedProfile.get());
	   RvTestResult(rv, "SetCurrentProfile() (profile switching) test", 2);

	   QAOutput("End Profile switch test.", 2);
    }
	else
	   QAOutput("Profile switch test not executed.", 2);
	
}

// *********************************************************

void CTests::OnTestsAddWebProgListener()
{
    nsWeakPtr weakling(
        dont_AddRef(NS_GetWeakReference(NS_STATIC_CAST(nsIWebProgressListener*, qaBrowserImpl))));
    rv = qaWebBrowser->AddWebBrowserListener(weakling, NS_GET_IID(nsIWebProgressListener));
	
	RvTestResult(rv, "AddWebBrowserListener(). Add Web Prog Lstnr test", 2);
}

// *********************************************************

void CTests::OnTestsAddHistoryListener()
{
   // addSHistoryListener test
	nsWeakPtr weakling(
        dont_AddRef(NS_GetWeakReference(NS_STATIC_CAST(nsISHistoryListener*, qaBrowserImpl))));
	rv = qaWebBrowser->AddWebBrowserListener(weakling, NS_GET_IID(nsISHistoryListener));
	RvTestResult(rv, "AddWebBrowserListener(). Add History Lstnr test", 2);
}

void CTests::OnTestsRemovehistorylistener() 
{
  // addSHistoryListener test
	nsWeakPtr weakling(
        dont_AddRef(NS_GetWeakReference(NS_STATIC_CAST(nsISHistoryListener*, qaBrowserImpl))));
	rv = qaWebBrowser->RemoveWebBrowserListener(weakling, NS_GET_IID(nsISHistoryListener));
	RvTestResult(rv, "RemoveWebBrowserListener(). Remove History Lstnr test", 2);
}

// *********************************************************
// *********************************************************
//					TOOLS to help us


void CTests::OnToolsRemoveGHPage() 
{
	char *theUrl = "http://www.bogussite.com/";
	CUrlDialog myDialog;
	PRBool theRetVal = PR_FALSE;
	//nsresult rv;
	nsCOMPtr<nsIGlobalHistory> myGHistory(do_GetService(NS_GLOBALHISTORY_CONTRACTID));
	if (!myGHistory)
	{
		QAOutput("Could not get the global history object.", 2);
		return;
	}
	nsCOMPtr<nsIBrowserHistory> myHistory = do_QueryInterface(myGHistory, &rv);
	if(NS_FAILED(rv)) {
		QAOutput("Could not get the history object.", 2);
		return;
	}
//	nsCOMPtr<nsIBrowserHistory> myHistory(do_GetInterface(myGHistory));


	if (myDialog.DoModal() == IDOK)
	{
		QAOutput("Begin URL removal from the GH file.", 2);
		strcpy(theUrl, myDialog.m_urlfield);

		myGHistory->IsVisited(theUrl, &theRetVal);
		if (theRetVal)
		{
			rv = myHistory->RemovePage(theUrl);
			RvTestResult(rv, "RemovePage() test (url removal from GH file)", 2);
		}
		else
		{
			QAOutput("The URL wasn't in the GH file.\r\n", 1);
		}
		QAOutput("End URL removal from the GH file.", 2);
	}
	else
		QAOutput("URL removal from the GH file not executed.", 2);
}

void CTests::OnToolsRemoveAllGH()
{

	//nsresult rv; 

	nsCOMPtr<nsIGlobalHistory> myGHistory(do_GetService(NS_GLOBALHISTORY_CONTRACTID));
	if (!myGHistory)
	{
		QAOutput("Could not get the global history object.", 2);
		return;
	}
	nsCOMPtr<nsIBrowserHistory> myHistory = do_QueryInterface(myGHistory, &rv);
	if(NS_FAILED(rv)) {
		QAOutput("Could not get the history object.", 2);
		return;
	}

	QAOutput("Begin removal of all pages from the GH file.", 2);

	rv = myHistory->RemoveAllPages();
	RvTestResult(rv, "removeAllPages(). Test .", 2);
	
	QAOutput("End removal of all pages from the GH file.", 2);

	// removeAllPages()

}

// ***********************************************************************
void CTests::OnToolsTestYourMethod()
{
	// place your test code here
}

// ***********************************************************************
void CTests::OnToolsTestYourMethod2()
{
	// place your test code here
}

// ***********************************************************************
// ************************** Interface Tests ****************************
// ***********************************************************************

// nsIFile:

void CTests::OnInterfacesNsifile() 
{
   nsCOMPtr<nsILocalFile> theTestFile(do_CreateInstance(NS_LOCAL_FILE_CONTRACTID));
   nsCOMPtr<nsILocalFile> theFileOpDir(do_CreateInstance(NS_LOCAL_FILE_CONTRACTID));

    if (!theTestFile)
 	{
		QAOutput("File object doesn't exist. No File tests performed.", 2);
		return;
	}
	if (!theFileOpDir)
 	{
		QAOutput("File object doesn't exist. No File tests performed.", 2);
		return;
	}

	QAOutput("Begin nsIFile tests.", 2);

	InitWithPathTest(theTestFile);
	AppendRelativePathTest(theTestFile);
	FileCreateTest(theTestFile);
	FileExistsTest(theTestFile);

	// FILE COPY test

	FileCopyTest(theTestFile, theFileOpDir);	

	// FILE MOVE test

	FileMoveTest(theTestFile, theFileOpDir);	

	QAOutput("End nsIFile tests.", 2);	
}

// ***********************************************************************
// Individual nsIFile tests

void CTests::InitWithPathTest(nsILocalFile *theTestFile)
{
	rv = theTestFile->InitWithPath("c:\\temp\\");
	RvTestResult(rv, "InitWithPath() test (initializing file path)", 2);
}

void CTests::AppendRelativePathTest(nsILocalFile *theTestFile)
{
	rv = theTestFile->AppendRelativePath("myFile.txt");
	RvTestResult(rv, "AppendRelativePath() test (append file to the path)", 2);
}

void CTests::FileCreateTest(nsILocalFile *theTestFile)
{
	rv = theTestFile->Exists(&exists);
	if (!exists)
	{
		QAOutput("File doesn't exist. We'll try creating it.", 2);
		rv = theTestFile->Create(nsIFile::NORMAL_FILE_TYPE, 0777);
		RvTestResult(rv, " File Create() test ('myFile.txt')", 2);
	}
	else
		QAOutput("File already exists (myFile.txt). We won't create it.", 2);
}

void CTests::FileExistsTest(nsILocalFile *theTestFile)
{
	rv = theTestFile->Exists(&exists);
	if (!exists)
		QAOutput("Exists() test Failed. File (myFile.txt) doesn't exist.", 2);
	else
		QAOutput("Exists() test Passed. File (myFile.txt) exists.", 2);

}

void CTests::FileCopyTest(nsILocalFile *theTestFile, nsILocalFile *theFileOpDir)
{
	QAOutput("Start File Copy test.", 2);

	rv = theFileOpDir->InitWithPath("c:\\temp\\");
	if (NS_FAILED(rv))
		QAOutput("The target dir wasn't found.", 2);
	else
		QAOutput("The target dir was found.", 2);

	rv = theTestFile->InitWithPath("c:\\temp\\myFile.txt");
	if (NS_FAILED(rv))
		QAOutput("The path wasn't found.", 2);
	else
		QAOutput("The path was found.", 2);

	rv = theTestFile->CopyTo(theFileOpDir, "myFile2.txt");
	RvTestResult(rv, "rv CopyTo() test", 2);

	rv = theTestFile->InitWithPath("c:\\temp\\myFile2.txt");
	rv = theTestFile->Exists(&exists);
	if (!exists)
		QAOutput("File didn't copy. CopyTo() test Failed.", 2);
	else
		QAOutput("File copied. CopyTo() test Passed.", 2);
}

void CTests::FileMoveTest(nsILocalFile *theTestFile, nsILocalFile *theFileOpDir)
{
	QAOutput("Start File Move test.", 2);

	rv = theFileOpDir->InitWithPath("c:\\Program Files\\");
	if (NS_FAILED(rv))
		QAOutput("The target dir wasn't found.", 2);

	rv = theTestFile->InitWithPath("c:\\temp\\myFile2.txt");
	if (NS_FAILED(rv))
		QAOutput("The path wasn't found.", 2);

	rv = theTestFile->MoveTo(theFileOpDir, "myFile2.txt");
	RvTestResult(rv, "MoveTo() test", 2);

	rv = theTestFile->InitWithPath("c:\\Program Files\\myFile2.txt");
	rv = theTestFile->Exists(&exists);
	if (!exists)
		QAOutput("File wasn't moved. MoveTo() test Failed.", 2);
	else
		QAOutput("File was moved. MoveTo() test Passed.", 2);
}

// ***********************************************************************
// ***************** Bug Verifications ******************
// ***********************************************************************

void CTests::OnVerifybugs70228() 
{
	nsCOMPtr<nsIHelperAppLauncherDialog> 
			myHALD(do_CreateInstance(NS_IHELPERAPPLAUNCHERDLG_CONTRACTID));
	if (!myHALD)
		QAOutput("Object not created. It should be. It's a component!", 2);
	else
		QAOutput("Object is created. It's a component!", 2);	

/*
nsCOMPtr<nsIHelperAppLauncher> 
			myHAL(do_CreateInstance(NS_IHELPERAPPLAUNCHERDLG_CONTRACTID));

	rv = myHALD->show(myHal, nsnull);
*/	
}

BOOL CTests::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo) 
{
   // To handle Menu handlers add here. Don't have to do if not handling 
   // menu handlers
	nCommandID = nID ;

	return CWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

void CTests::OnInterfacesNsirequest() 
{
	CNsIRequest oNsIRequest(qaWebBrowser,/*qaBaseWindow,qaWebNav,*/ qaBrowserImpl);
	oNsIRequest.OnStartTests(nCommandID);
}

void CTests::OnInterfacesNsidirectoryservice() 
{
	CNsIDirectoryService oNsIDirectoryService;
	oNsIDirectoryService.StartTests(nCommandID);
}

void CTests::OnInterfacesNsidomwindow() 
{
	CDomWindow oDomWindow(qaWebBrowser) ;
	oDomWindow.OnStartTests(nCommandID);		
}

void CTests::OnInterfacesNsiselection() 
{
	CSelection oSelection(qaWebBrowser);
	oSelection.OnStartTests(nCommandID);
}

void CTests::OnVerifybugs90195() 
{
    nsWeakPtr weakling(
        dont_AddRef(NS_GetWeakReference(NS_STATIC_CAST(nsITooltipListener*, qaBrowserImpl))));
    rv = qaWebBrowser->AddWebBrowserListener(weakling, NS_GET_IID(nsITooltipListener));
	
	RvTestResult(rv, "AddWebBrowserListener(). Add Tool Tip Lstnr test", 2);

/*	nsCOMPtr<nsITooltipTextProvider> oTooltipTextProvider = do_GetService(NS_TOOLTIPTEXTPROVIDER_CONTRACTID) ;
	if (!oTooltipTextProvider)
		AfxMEssageBox("Asdfadf");
*/
}

void CTests::OnInterfacesNsiprofile() 
{
	CProfile oProfile(qaWebBrowser);
	oProfile.OnStartTests(nCommandID);	
}

void CTests::OnInterfacesNsishistory() 
{
	CNsIHistory oHistory(qaWebNav);
	oHistory.OnStartTests(nCommandID);	
}


void CTests::OnInterfacesNsiwebnav() 
{
	CNsIWebNav oWebNav(qaWebNav);
	oWebNav.OnStartTests(nCommandID);	
}		


void CTests::OnInterfacesNsiclipboardcommands() 
{
	CNsIClipBoardCmd  oClipCmd(qaWebBrowser) ;
	oClipCmd.OnStartTests(nCommandID);
}

void CTests::OnInterfacesNsiobserverservice() 
{
	CnsIObserServ oObserv  ;
	oObserv.OnStartTests(nCommandID);

}
