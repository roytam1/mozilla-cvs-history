/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
/* 
   ABDirPropertyDlg.cpp -- class definition for XFE_ABDirPropertyDlg
   Created: Tao Cheng <tao@netscape.com>, 10-nov-97
 */

#include "ABDirPropertyDlg.h"
#include "PropertySheetView.h"
#include "ABDirGenTabView.h"

#include "abcom.h"

#include "xpgetstr.h"

extern int XFE_ABDIR_DLG_TITLE;

//
// This is the dialog it self
//
XFE_ABDirPropertyDlg::XFE_ABDirPropertyDlg(Widget    parent,
										   char     *name,
										   Boolean   modal,
										   MWContext *context):
	XFE_PropertySheetDialog((XFE_View *)0, parent, name,
						context,
						TRUE,    /* ok */
						TRUE,    /* cancel */
						TRUE,    /* help */
						FALSE,   /* apply */
						FALSE,   /* separator */
						modal),
	m_dir(NULL),
	m_proc(NULL),
	m_callData(NULL)
#if defined(USE_ABCOM)
	, m_pane(NULL)
#endif /* USE_ABCOM */
{
	XFE_PropertySheetView* folderView = (XFE_PropertySheetView *) m_view;
	
	/* Add tabs
	 */
	/* Gen
	 */
	folderView->addTab(new XFE_ABDirGenTabView((XFE_Component *)this, 
											   folderView));
	XtVaSetValues(XtParent(m_chrome), 
				  XmNtitle, XP_GetString(XFE_ABDIR_DLG_TITLE), 
				  NULL);
}

XFE_ABDirPropertyDlg::~XFE_ABDirPropertyDlg()
{

}

#if defined(USE_ABCOM)
void 
XFE_ABDirPropertyDlg::setPane(MSG_Pane *pane)
{
	m_pane = pane;
}
#endif /* USE_ABCOM */

void XFE_ABDirPropertyDlg::setDlgValues(DIR_Server *dir)
{
	m_dir = dir;
	XFE_PropertySheetDialog::setDlgValues();
}

void XFE_ABDirPropertyDlg::apply()
{
  getDlgValues();
  if (m_proc)
	  m_proc(m_dir, m_callData);
#if defined(USE_ABCOM)
  else if (m_pane) {
	  int error = 
		  AB_UpdateDIRServerForContainerPane(m_pane, m_dir);
  }/* else */
#endif /* USE_ABCOM */
}

void XFE_ABDirPropertyDlg::registerCB(ABDirPropertyCBProc proc, 
									  void               *callData)
{
	m_proc = proc;
	m_callData = callData;
}

extern "C" void 
fe_showABDirPropertyDlg(Widget               parent,
						MWContext           *context,
						DIR_Server          *dir,
						ABDirPropertyCBProc  proc,
						void                *callData)
{
	XFE_ABDirPropertyDlg* Dlg = 
		new XFE_ABDirPropertyDlg(parent,
								 "abDirProperties", 
								 True, 
								 context);
	Dlg->setDlgValues(dir);
	Dlg->registerCB(proc, callData);
	Dlg->show();
}

