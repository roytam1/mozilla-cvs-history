/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
/* 
   RDFChromeTreeView.h -- class definition for XFE_RDFChromeTreeView
   Created: Stephen Lamm <slamm@netscape.com>, 5-Nov-97.
 */



#ifndef _xfe_rdfchrometreeview_h
#define _xfe_rdfchrometreeview_h

#include "View.h"
#include "IconGroup.h"
#include "htrdf.h"
#include "NavCenterView.h"
#include "RDFTreeView.h"
#include "HTMLView.h"

typedef enum _EHtmlPaneSizing 
{
	XFE_HTML_PANE_FIXED,
	XFE_HTML_PANE_PERCENT
} EHtmlPaneSizing;

class XFE_RDFChromeTreeView : public XFE_RDFTreeView
{
public:

  XFE_RDFChromeTreeView(XFE_Component *toplevel, Widget parent,
              XFE_View *parent_view, MWContext *context);

  ~XFE_RDFChromeTreeView();

  // Get tooltipString & docString; 
  // returned string shall be freed by the callee
  // row < 0 indicates heading row; otherwise it is a content row
  // (starting from 0)
  //
  virtual char *getCellTipString(int /* row */, int /* column */) {return NULL;}
  virtual char *getCellDocString(int /* row */, int /* column */) {return NULL;}

	// Open properties dialog
	//void openPropertiesWindow();
	//void closePropertiesWindow();
	
	// Override RDFBase notify method
	void notify(HT_Resource n, HT_Event whatHappened);
	
	// RDF Specific calls
	void setHTTitlebarProperties(HT_View view, Widget titleBar);
	
	// Set the HTML pane height (as a percentage of the view)
	void setHtmlPaneHeightPercent(PRUint32 heightPercent);

	// Set the HTML pane height (as a fixed pixel height)
	void setHtmlPaneHeightFixed(PRUint32 heightPercent);
	
	// Set the HTML pane sizing policy
	void setHtmlPaneSizing(EHtmlPaneSizing sizing);

protected:

    // Override RDFBase methods
	virtual void	updateRoot      ();
	virtual Widget	getTreeParent	();
    virtual void	doAttachments	();

private:

	// The label that displays the currently open pane
	Widget				_viewLabel;     

	// Parent of the label and the button on top
	Widget				_controlToolBar; 

	// Toggle tree operating mode
	Widget				_addBookmarkControl;

	// Close the view
	Widget				_closeControl;

	// Toggle tree operating mode
	Widget				_modeControl;

	// The HTML pane form
	Widget				_htmlPaneForm;

	// The tree/html division form
	Widget				_divisionForm;

	// The HTML pane
	XFE_HTMLView *		_htmlPane;

	// The height of the HTML pane as a percentage of the view
	PRUint32			_htmlPaneHeightPercent;

	// The height of the HTML pane as a fixed pixel height
	PRUint32			_htmlPaneHeightFixed;

	// Type of html pain sizing
	EHtmlPaneSizing		_htmlPaneSizing;

	static void closeRdfView_cb(Widget, XtPointer, XtPointer);

    // Create widgets
    void createControlToolbar();
    void createViewLabel();
    void createDivisionForm();
    void createHtmlPane();
};

#endif /* _xfe_rdfchrometreeview_h */
