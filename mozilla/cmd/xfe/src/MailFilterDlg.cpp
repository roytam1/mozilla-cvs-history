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
   MailFilterDlg.cpp -- 
   Created: Tao Cheng <tao@netscape.com>, 20-nov-96
 */



#include "MailFilterDlg.h"
#include "MailFilterView.h"
#include "Outliner.h"
#include "Outlinable.h"

#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/PushB.h>
#include <Xm/TextF.h>
#include <Xm/DialogS.h>

extern "C" {
#include "xfe.h"
};

#include "xpgetstr.h"

XFE_MailFilterDlg::XFE_MailFilterDlg(Widget   parent,
									 char    *name,
									 Boolean  modal,
									 MWContext *context,
									 MSG_Pane *pane):
	XFE_ViewDialog((XFE_View *) 0, parent, name,
				   context,
				   True, /* ok */
				   True, /* cancel */
				   False, /* help */
				   False, /* apply; remove */
				   False, /* separator */
				   modal)
{

  Arg av [20];
  int ac = 0;

  //
  // we don't want a default value, since return does other stuff for
  // us.
  XtVaSetValues(m_chrome, /* the dialog */
				XmNdefaultButton, NULL,
				NULL);

  /* Form: m_chrome is the dialog 
   */
  ac = 0;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNwidth, 500); ac++;
  XtSetArg (av [ac], XmNheight, 300); ac++;
  Widget form = XmCreateForm (m_chrome, "form", av, ac);
  XtManageChild (form);

  /* Search UI
   */
  XFE_MailFilterView *view = 
	  new XFE_MailFilterView(this,     /* toplevel_component */
							 form,     /* parent */
							 NULL,     /* parent_view */
							 m_context,/* context */
							 pane);
  setView(view);
}

XFE_MailFilterDlg::~XFE_MailFilterDlg()
{
	/* need to delete resources here; 
	 * check if this destructor ever gets called
	 */
}

void XFE_MailFilterDlg::cancel()
{
	((XFE_MailFilterView *) m_view)->cancel();
	
}

void XFE_MailFilterDlg::ok()
{
	((XFE_MailFilterView *) m_view)->apply();
}

XFE_CALLBACK_DEFN(XFE_MailFilterDlg, listChanged)(XFE_NotificationCenter */*obj*/,
												  void */*clientData*/,
												  void *callData)
{
	int which = (int) callData;
	// call view to update
	((XFE_MailFilterView *) m_view)->listChanged(which);
}


/* C API
 */
extern "C"  void
fe_showMailFilterDlg(Widget toplevel, MWContext *context, MSG_Pane *pane)
{
	/* recreate every time -> delete for eaech cancel
	 */
	XFE_MailFilterDlg *dlg =  
		new XFE_MailFilterDlg(toplevel, 
							  "filterDialog",
							  False,
							  context,
                              pane);
	dlg->show();
}

