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
  g-browser-frame.h -- browser windows.
  Created: Chris Toshok <toshok@hungry.com>, 9-Apr-98.
*/

#ifndef _moz_browser_frame_h
#define _moz_browser_frame_h

#include "g-frame.h"

struct _MozBrowserFrame {
  /* our superclass */
  MozFrame _frame;

  /* MozBrowserFrame specific data members. */

};

extern void moz_browser_frame_init(MozBrowserFrame *frame);
extern void moz_browser_frame_deinit(MozBrowserFrame *frame);

extern MozBrowserFrame* moz_browser_frame_create();

#define MOZ_BROWSERFRAME(f) &((f)->_browser_frame)

#endif /* _moz_browser_frame_h */
