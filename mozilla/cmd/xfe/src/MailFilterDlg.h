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
   MailFilterDlg.h -- class definition for XFE_MailFilterDlg
   Created: Tao Cheng <tao@netscape.com>, 20-nov-96
 */



#ifndef _MAILFILTERDLG_H_
#define _MAILFILTERDLG_H_

#include "ViewDialog.h"

//
class XFE_MailFilterDlg: public XFE_ViewDialog 
{

public:

  XFE_MailFilterDlg(Widget           parent,
                    char            *name,
                    Boolean          modal,
                    MWContext *context,
                    MSG_Pane *pane);

  virtual ~XFE_MailFilterDlg();

  // Notification callback
  XFE_CALLBACK_DECL(listChanged)


protected:
  virtual void cancel();
  virtual void ok();

private:

};

extern "C" void fe_showMailFilterDlg(Widget toplevel, MWContext *context,
                                     MSG_Pane *pane);

#endif /* _MAILFILTERDLG_H_ */
