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
   ABNameGenTab.h -- class definition for XFE_ABNameGenTabView
   Created: Tao Cheng <tao@netscape.com>, 12-nov-96
 */

#ifndef _xfe_abnamegentabview_h
#define _xfe_abnamegentabview_h

#include "PropertyTabView.h"

class XFE_ABNameGenTabView: public XFE_PropertyTabView {
public:
  XFE_ABNameGenTabView(XFE_Component *top,
		       XFE_View      *view/* the parent view */);
  virtual ~XFE_ABNameGenTabView();

  virtual void setDlgValues();

#if 1
  enum {AB_FIRST_NAME =  0, 
		AB_LAST_NAME, 
		AB_DISPLAY_NAME, 
		AB_EMAIL, 
		AB_NICKNAME, 
		AB_TITLE, 
		AB_COMPANY_NAME, 
		AB_LAST
  } GEN_TEXTF;
#else
  enum {AB_FIRST_NAME =  0, 
		AB_LAST_NAME, 
		AB_COMPANY_NAME, 
		AB_TITLE, 
		AB_EMAIL, 
		AB_NICKNAME, 
		AB_LAST
  } GEN_TEXTF;
#endif

  static void textFValChgCallback(Widget, XtPointer, XtPointer);

protected:
  virtual void textFValChgCB(Widget, XtPointer);

  virtual void apply(){};
  virtual void getDlgValues();

private:
  /* m_widget is the tab form
   */

  /* widgets in this tab
   */
  Widget m_textFs[AB_LAST+1];
  Widget m_labels[AB_LAST+1];
  Widget m_notesTxt;
  Widget m_prefHTMLTog;

  XP_Bool m_userDefined;

}; /* XFE_ABNameGenTabView */

#endif /* _xfe_abnamegentabview_h */
