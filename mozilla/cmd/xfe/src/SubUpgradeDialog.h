/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

#ifndef _xfe_subupgradedialog_h
#define _xfe_subupgradedialog_h

#include "Dialog.h"
#include "msgcom.h"
#include "Xm/Xm.h"

class XFE_SubUpgradeDialog: public XFE_Dialog
{
 public:
    XFE_SubUpgradeDialog(MWContext *context,
                         const char *hostName);
    
    virtual ~XFE_SubUpgradeDialog();
    
    MSG_IMAPUpgradeType prompt();

    void create();
    void init();
    
    
 private:
    void ok();
    void cancel();
    static void cb_ok(Widget, XtPointer, XtPointer);
    static void cb_cancel(Widget, XtPointer, XtPointer);

    Widget m_automatic_toggle;
    Widget m_custom_toggle;
    
    MSG_IMAPUpgradeType m_retVal;

    XP_Bool m_doneWithLoop;
};

#endif
