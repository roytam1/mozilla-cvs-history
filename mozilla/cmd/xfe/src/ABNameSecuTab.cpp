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
   ABNameSecuTab.cpp -- class definition for XFE_ABNameSecuTabView
   Created: Tao Cheng <tao@netscape.com>, 12-nov-96
 */

#include "rosetta.h"
#include "ABNameSecuTab.h"

#include <Xm/Form.h>
#include <Xm/PushB.h>
#include <Xm/LabelG.h> 

#include "xfe.h"
#include "xpgetstr.h"

extern int XFE_AB_NAME_SECURITY_TAB;
extern int XFE_AB_SECUR_YES;
extern int XFE_AB_SECUR_NO;
extern int XFE_AB_SECUR_EXPIRED;
extern int XFE_AB_SECUR_REVOKED;
extern int XFE_AB_SECUR_YOU_YES;
extern int XFE_AB_SECUR_YOU_NO;
extern int XFE_AB_SECUR_YOU_EXPIRED;
extern int XFE_AB_SECUR_YOU_REVOKED;
extern int XFE_AB_SECUR_SHOW;
extern int XFE_AB_SECUR_GET;

XFE_ABNameSecuTabView::XFE_ABNameSecuTabView(XFE_Component *top,
					     XFE_View *view):
  XFE_PropertyTabView(top, view, XFE_AB_NAME_SECURITY_TAB)
{
  
}

XFE_ABNameSecuTabView::~XFE_ABNameSecuTabView()
{
}

void XFE_ABNameSecuTabView::setDlgValues()
{
}

void XFE_ABNameSecuTabView::apply()
{
}

void XFE_ABNameSecuTabView::getDlgValues()
{
}

/*
 * Callbacks for outside world
 */
void
XFE_ABNameSecuTabView::showCallback(Widget w, 
				   XtPointer clientData, 
				   XtPointer callData)
{
  XFE_ABNameSecuTabView *obj = (XFE_ABNameSecuTabView *) clientData;
  obj->showCB(w, callData);
}

void XFE_ABNameSecuTabView::showCB(Widget, XtPointer)
{
}
