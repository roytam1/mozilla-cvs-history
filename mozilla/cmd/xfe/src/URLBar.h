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
   URLBar.h -- The text field chrome above an HTMLView, used in
               BrowserFrames.
   Created: Chris Toshok <toshok@netscape.com>, 1-Nov-96.
   */


#ifndef _xfe_urlbar_h
#define _xfe_urlbar_h

#include "structs.h"
#include "icons.h"
#include "LocationDrag.h"
#include "ToolbarDrop.h"
#include "Button.h"
#include "ToolboxItem.h"

#define URLBAR_POPUP
#ifdef URLBAR_POPUP
#include "PopupMenu.h"
#endif

class XFE_Logo;

class XFE_URLBar : public XFE_ToolboxItem
{
public:

	XFE_URLBar(XFE_Frame *		parent_frame,
			   XFE_Toolbox *	parent_toolbox,
			   History_entry *	first_entry = NULL);
	
	virtual ~XFE_URLBar();

	
	void recordURL(URL_Struct *url);  // add the url to the drop down.
	void setURLString(URL_Struct *url); // set the text's value to the url.
    static void clearAll();
	
	static const char *navigateToURL;  // callback invoked when the user 
	// selects (or types) a new url
	
	Widget		getBookMarkButton	();
	Widget		getProxyIcon		();
	Widget		getUrlLabel			();
	Widget		getFormWidget		();

	// Quickfile menu spec
	static MenuSpec quickfile_menu_spec[];

	void clearText();

protected:

	virtual void		update() {}

private:

	XmString			m_editedLabelString;
	XmString			m_uneditedLabelString;
	XmString			m_netsiteLabelString;
	
	XFE_Frame *			m_parentFrame;

	XFE_Button  *		m_bookmarkButton;
	XFE_Button  *		m_locationProxyIcon;
	Widget				m_locationLabel;
	Widget				m_urlComboBox;

	XP_Bool				m_labelBeingFrobbed;

	XFE_LocationDrag *	m_proxyIconDragSite;
	XFE_QuickfileDrop *	m_bookmarkDropSite;

#ifdef URLBAR_POPUP
    XFE_PopupMenu *m_popup;
    static MenuSpec ccp_popup_spec[];

    void doPopup(XEvent *event);

	// Callbacks and event handlers
	static void comboBoxButtonPressEH (Widget,XtPointer,XEvent *,Boolean *);
#endif

	XP_Bool readUserHistory();
	XP_Bool saveUserHistory();
	static XP_Bool eraseUserHistory();
	
	void frob_label(XP_Bool edited_p, XP_Bool netsite_p);
	
	void url_text(XtPointer);
	static void url_text_cb(Widget, XtPointer, XtPointer);
	
	void url_text_modify();
	static void url_text_modify_cb(Widget, XtPointer, XtPointer);
	
	void url_text_selection(XtPointer);
	static void url_text_selection_cb(Widget, XtPointer, XtPointer);

	static void source_drop_func(fe_dnd_Source *src, fe_dnd_Message msg, void *closure);
	
	void		createBookmarkButton		(Widget);
	void		createProxyIcon				(Widget);
	void		createUrlComboBox			(Widget,History_entry *);

	// URL label resource tricks
	static XtResource		_urlLabelResources[];

	// Tear off support for the bookmark cascade
	static void	bookmarkCascadeTearCB		(Widget, XtPointer, XtPointer);
	void		handleBookmarkCascadeTear	(XP_Bool torn);

	static void	frameTitleChangedCB			(Widget, XtPointer, XtPointer);
	void		handleFrameTitleChanged		(String title);

    XFE_CALLBACK_DECL(_recordURL) // add the url to the drop down.
    XFE_CALLBACK_DECL(_clearAll) // clear all urls from the drop down.
};

#endif /* _xfe_urlbar_h */
