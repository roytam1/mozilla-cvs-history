/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols
#include "WizardTypes.h"


/////////////////////////////////////////////////////////////////////////////
// CInterpret:
// See Interpret.cpp for the implementation of this class
//
class CInterpret
{
public:
	CInterpret();
	~CInterpret();
	BOOL NewConfig(WIDGET *curWidget, CString globalsName);
	BOOL BrowseFile(WIDGET *curWidget);
	BOOL BrowseDir(WIDGET *curWidget);
	BOOL Progress();  // Not actually used right now
	void CopyDir(CString from, CString to);
	void ExecuteCommand(char *command, int showflag);
	BOOL IterateListBox(char *parms);
	CString replaceVars(CString str, char *listval);
	CString replaceVars(char *str, char *listval);
	BOOL CallDLL(char *dll, char *proc, char *parms);
	BOOL interpret(char *cmds, WIDGET *curWidget);
	BOOL interpret(CString cmds, WIDGET *curWidget);
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWizardMachineApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation
	DLLINFO m_DLLs;
};


/////////////////////////////////////////////////////////////////////////////

