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
  g-editor-view.h -- editor views.
  Created: Chris Toshok <toshok@hungry.com>, 18-Jul-98.
*/

#ifndef _moz_editor_view_h
#define _moz_editor_view_h

#include "g-html-view.h"

struct _MozEditorView {
  /* our superclass */
  MozHTMLView _html_view;
};

extern void		moz_editor_view_init(MozEditorView *view, MozFrame *parent_frame, MWContext *context);
extern void		moz_editor_view_deinit(MozEditorView *view);

extern MozEditorView*	moz_editor_view_create(MozFrame *parent_frame, MWContext *context);

extern void moz_editor_view_display_linefeed(MozEditorView *view,
					     LO_LinefeedStruct *line_feed,
					     XP_Bool need_bg);

#endif /*_moz_html_view_h */
