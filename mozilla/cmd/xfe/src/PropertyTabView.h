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
   PropertyTabView.h -- class definition for XFE_PropertyTabView
   Created: Tao Cheng <tao@netscape.com>, 12-nov-96
 */



#ifndef _xfe_xmltabview_h
#define _xfe_xmltabview_h

#include "View.h"

/* This is a general wrapper of property tab form (goes in a property sheet) */

class XFE_PropertyTabView: public XFE_View {
public:
  XFE_PropertyTabView(XFE_Component *top, /* the parent folderDialog */
		 XFE_View *view, /* the parent view */
                 int tab_string_id);

  virtual ~XFE_PropertyTabView();

  virtual void setDlgValues() {};
  virtual void getDlgValues() {};
  virtual void apply(){};
	virtual void hide();
	virtual void show();

protected:
  Dimension m_labelWidth;

private:
  /* m_widget is the tab form
   */
	Widget m_tab;
}; /* XFE_PropertyTabView */

/* Offsets between widgets when hardwired is needed
 */
const int labelWidth = 125;
const int labelHeight = 30;
const int textFWidth = 175;
const int textFHeight = 30;

const int separatorHeight = 10;

const int majorVSpac = 6;
const int minorVSpac = 3;
const int majorHSpac = 6;
const int minorHSpac = 3;

#endif /* _xfe_xmltabview_h */
