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
 *   David Epstein <depstein@netscape.com> 
 */

// File Overview....
//
// These are utilities to help with QA tasks.

// Includes routine to post results to a QA log file.

#include "stdafx.h"
#include "TestEmbed.h"
#include "BrowserView.h"
#include "BrowserImpl.h"
#include "BrowserFrm.h"
#include "QAUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

BEGIN_MESSAGE_MAP(CQaUtils, CWnd)
	//{{AFX_MSG_MAP(CQaUtils)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


CQaUtils::CQaUtils()
{

}

CQaUtils::~CQaUtils()
{
}

// ***********************************************************************
// ***************** Local QA Methods ******************
// ***********************************************************************


void CQaUtils::RvTestResult(nsresult rv, const char *pLine, int displayMethod)
{
	// note: default displayMethod = 1 in .h file

	CString strLine = pLine;
	char theOutputLine[100];

	if (NS_FAILED(rv))
	   strLine += " failed.";
	else
	   strLine += " passed.";

	strcpy(theOutputLine, strLine);
	QAOutput(theOutputLine, displayMethod);
}

void CQaUtils::WriteToOutputFile(const char *pLine) 
{ 
    CStdioFile myFile; 
    CFileException e; 
    CString strFileName = "c:\\temp\\TestOutput.txt"; 

    if(! myFile.Open( strFileName, CStdioFile::modeCreate | CStdioFile::modeWrite
								| CStdioFile::modeNoTruncate, &e ) ) 
    { 
        CString failCause = "Unable to open file. Reason : "; 
        failCause += e.m_cause; 
        AfxMessageBox(failCause); 
    } 
    else 
    { 
		myFile.SeekToEnd();
        CString strLine = pLine; 
        strLine += "\r\n"; 

        myFile.WriteString(strLine); 

        myFile.Close(); 
    } 
} 

void CQaUtils::QAOutput(const char *pLine, int displayMethod)
{
	// note: default displayMethod = 1 in .h file
//#if 0
   CString strLine = pLine;

   if (displayMethod == 0)
	   AfxMessageBox(strLine);
   else if (displayMethod == 1)
	   WriteToOutputFile(pLine);
   else 
   {
       WriteToOutputFile(pLine);
	   AfxMessageBox(strLine);
   }
//#endif
}

void CQaUtils::FormatAndPrintOutput(const char *theInput, const char *theVar, int outputMode)
{
	nsCString outStr;
	CString strMsg;

	outStr = theInput;
	outStr += theVar;

	strMsg = outStr.get();

	switch (outputMode)
	{
	case 0:
		AfxMessageBox(strMsg); 
		break;
	case 1:
		WriteToOutputFile(outStr.get());
		break;
	case 2:
		WriteToOutputFile(outStr.get());
		AfxMessageBox(strMsg); 
		break;
	}
}

// stringMsg is returned in case embeddor wishes to use it in the calling method.
void CQaUtils::RequestName(nsIRequest *request, nsCString &stringMsg,
						   int displayMethod)
{
    nsXPIDLString theReqName;
	nsresult rv;

	rv = request->GetName(getter_Copies(theReqName));
	if(NS_SUCCEEDED(rv))
	{
		stringMsg.AssignWithConversion(theReqName);
		FormatAndPrintOutput("nsIRequest: The request name = ", stringMsg.get(), displayMethod);
	}
	else
		QAOutput("nsIRequest: We didn't get the request name.", displayMethod);

}

void CQaUtils::WebProgDOMWindowTest(nsIWebProgress *progress, const char *inString,
								  int displayMethod)
{
	nsresult rv;
	nsCString totalStr1, totalStr2;
	nsCOMPtr<nsIDOMWindow> theDOMWindow;

	totalStr1 = inString;
	totalStr1 += ": Didn't get the DOMWindow. Test failed.";

	totalStr2 = inString;
	totalStr2 += ": nsIWebProgress:DOMWindow attribute test";

	rv = progress->GetDOMWindow(getter_AddRefs(theDOMWindow));
	if (!theDOMWindow)
		QAOutput(totalStr1.get(), displayMethod);
	else
		RvTestResult(rv, totalStr2, displayMethod);
}
