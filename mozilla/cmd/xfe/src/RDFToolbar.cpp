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
   RDFToolbar.cpp - Toolbars supplied by RDF
   Created: Stephen Lamm <slamm@netscape.com>, 13-Aug-1998
 */

#include "BrowserFrame.h"
#include "RDFToolbar.h"
#include "Logo.h"
#include "MozillaApp.h"
#include "RDFUtils.h"

#include "prefapi.h"
#include "felocale.h"
#include "intl_csi.h"
#include "View.h"


#include <Xfe/ToolItem.h>
#include <Xfe/ToolBar.h>
#include <Xfe/ToolTip.h>

#include "ToolbarUrlBar.h"

#if DEBUG_radha
#define D(x) x
#else
#define D(x)
#endif

#define MIN_TOOLBAR_HEIGHT				26
#define MAX_CHILD_WIDTH					100
#define LOGO_NAME						"logo"


extern "C"  {
extern RDF_NCVocab  gNavCenter;
extern XmString fe_ConvertToXmString(unsigned char *, int16, fe_Font, XmFontType, XmFontList * );
extern INTL_CharSetInfo LO_GetDocumentCharacterSetInfo(MWContext *);
}

typedef struct _tbarTooltipCBStruct {
  HT_Resource entry;
  XFE_RDFToolbar * obj;
} tbarTooltipCBStruct;


//////////////////////////////////////////////////////////////////////////
//
// XFE_RDFToolbar notifications
//
//////////////////////////////////////////////////////////////////////////
const char *
XFE_RDFToolbar::navigateToUrlNotice = "XFE_RDFToolbar::navigateToUrlNotice";

XFE_RDFToolbar::XFE_RDFToolbar(XFE_Frame * frame, 
                               XFE_Toolbox * toolbox,
                               HT_View view)
    : XFE_ToolboxItem(frame, toolbox),
      XFE_RDFMenuToolbarBase(frame, FALSE /*only headers*/, TRUE /*fancy*/),
      _frame(frame)
{
	m_widget = XtVaCreateWidget("toolBoxItem",
                   xfeToolItemWidgetClass,
                   toolbox->getBaseWidget(),
                   XmNuserData, this,
                   NULL);

	// Create the toolbar
	_toolbar = XtVaCreateManagedWidget(HT_GetViewName(view),
                   xfeToolBarWidgetClass,
                   m_widget,
                   XmNusePreferredWidth,         False,
                   XmNusePreferredHeight,        True,
                   NULL);


	// Create and configure the logo if needed
	if (XfeChildIsEnabled(m_widget,LOGO_NAME,"Logo",True))
	{
		m_logo = new XFE_Logo(frame,m_widget,LOGO_NAME);
		
		m_logo->setSize(XFE_ANIMATION_SMALL);
		
		configureLogo();
	}

	// Make sure the toolbar is not highlighted first
	setRaised(False);

	// Install the destruction handler
	installDestroyHandler();

    setHTView(view);

    show();

#ifdef NOT_YET
	_frame->registerInterest(XFE_View::chromeNeedsUpdating,
                             this,
                             (XFE_FunctionNotification)update_cb);

	_frame->registerInterest(XFE_View::commandNeedsUpdating,
                             this,
                             (XFE_FunctionNotification)updateCommand_cb);

    XFE_MozillaApp::theApp()->registerInterest(XFE_View::commandNeedsUpdating,
											   this,
											   (XFE_FunctionNotification)updateCommand_cb);

    XFE_MozillaApp::theApp()->registerInterest(XFE_MozillaApp::updateToolbarAppearance,
                                               this,
                                               (XFE_FunctionNotification)updateToolbarAppearance_cb);
#endif

    XFE_MozillaApp::theApp()->registerInterest(
        XFE_MozillaApp::updateToolbarAppearance,
        this,
        (XFE_FunctionNotification)updateIconAppearance_cb);
}


XFE_RDFToolbar::~XFE_RDFToolbar() 
{
#ifdef NOT_YET
	_frame->unregisterInterest(XFE_View::chromeNeedsUpdating,
                               this,
                               (XFE_FunctionNotification)update_cb);

	_frame->unregisterInterest(XFE_View::commandNeedsUpdating,
                               this,
                               (XFE_FunctionNotification)updateCommand_cb);

    XFE_MozillaApp::theApp()->unregisterInterest(XFE_View::commandNeedsUpdating,
												 this,
												 (XFE_FunctionNotification)updateCommand_cb);

    XFE_MozillaApp::theApp()->unregisterInterest(XFE_MozillaApp::updateToolbarAppearance,
                                                 this,
                                                 (XFE_FunctionNotification)updateToolbarAppearance_cb);
#endif

    XFE_MozillaApp::theApp()->unregisterInterest(
        XFE_MozillaApp::updateToolbarAppearance,
        this,
        (XFE_FunctionNotification)updateIconAppearance_cb);
}
#ifdef NOT_YET
//////////////////////////////////////////////////////////////////////////
XFE_CALLBACK_DEFN(XFE_RDFToolbar, updateCommand)(XFE_NotificationCenter */*obj*/, 
					      void */*clientData*/, 
					      void *callData)
{
}
//////////////////////////////////////////////////////////////////////////
XFE_CALLBACK_DEFN(XFE_RDFToolbar, update)(XFE_NotificationCenter */*obj*/, 
									   void */*clientData*/, 
									   void */*callData*/)
{
  update();
}
//////////////////////////////////////////////////////////////////////////
XFE_CALLBACK_DEFN(XFE_RDFToolbar, updateToolbarAppearance)(XFE_NotificationCenter */*obj*/, 
									   void */*clientData*/, 
									   void */*callData*/)
{
	updateAppearance();
}
#endif /*NOT_YET*/
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
XFE_CALLBACK_DEFN(XFE_RDFToolbar,updateIconAppearance)
    (XFE_NotificationCenter *    /*obj*/, 
     void *                        /*clientData*/, 
     void *                        /*callData*/)
{
    // Update the appearance
    updateAppearance();
}
//////////////////////////////////////////////////////////////////////////
void
XFE_RDFToolbar::update()
{
#ifdef NOT_YET
	// Make sure the toolbar is alive
    if (!XfeIsAlive(_toolbar))
    {
        return;
    }
  
	Widget *		children;
	Cardinal		num_children;
	Cardinal		i;

	XfeChildrenGet(_toolbar,&children,&num_children);

	for (i = 0; i < num_children; i ++)
	{
		if (XfeIsButton(children[i]))
        {
            
        }

    }
#endif /*NOT_YET*/
}
//////////////////////////////////////////////////////////////////////////
void
XFE_RDFToolbar::notify(HT_Resource n, HT_Event whatHappened)
{
  D(debugEvent(n, whatHappened,"Toolbar"););
  switch (whatHappened) {
  case HT_EVENT_NODE_ADDED:
      addItem(n);
      break;
  default:
      // Fall through and let the base class handle this.
    break;
  }
  XFE_RDFMenuToolbarBase::notify(n, whatHappened);
}
//////////////////////////////////////////////////////////////////////////
void
XFE_RDFToolbar::setRaised(XP_Bool state)
{
	XP_ASSERT( XfeIsAlive(_toolbar) );

	XtVaSetValues(_toolbar,XmNraised,state,NULL);
}
//////////////////////////////////////////////////////////////////////////
void
XFE_RDFToolbar::destroyToolbarWidgets()
{
	XP_ASSERT( XfeIsAlive(_toolbar) );

 	WidgetList		children;
 	Cardinal		num_children;

 	XfeChildrenGet(_toolbar,&children,&num_children);	

 	// Get rid of the previous items we created
 	if (children && num_children)
	{
		XtUnmanageChildren(children,num_children);
      
		XfeDestroyMenuWidgetTree(children,num_children,True);
	}
}
//////////////////////////////////////////////////////////////////////////
XP_Bool
XFE_RDFToolbar::isToolbarFolderValid()
{
	HT_Resource toolbarRoot = getRootFolder();
 	return ((toolbarRoot != NULL) && HT_IsContainer(toolbarRoot));
}
//////////////////////////////////////////////////////////////////////////
void
XFE_RDFToolbar::addItem(HT_Resource node)
{
    Widget item = NULL;

    HT_Resource   toolbarRoot   = getRootFolder();
    HT_Resource   parent        = HT_GetParent(node);

    // This node doesn't belong here.
    if (toolbarRoot != parent) return;
        
    // Headers
    if (HT_IsContainer(node))
    {
        item = createXfeCascade(_toolbar, node);
    }
    // Separators
    else if (HT_IsSeparator(node))
    {
        item = createSeparator(_toolbar);
    }
    // UrlBar
    else if (HT_IsURLBar(node))
    {
		item = createUrlBar(_toolbar,node);
    }
    // Normal items
    else
    {
        item = createXfeButton(_toolbar, node);
    }
    
    XP_ASSERT( XfeIsAlive(item) );
    
    XtManageChild(item);
}
//////////////////////////////////////////////////////////////////////////
/* virtual */ void
XFE_RDFToolbar::prepareToUpdateRoot()
{
	// Force the items to update
	updateRoot();
}
//////////////////////////////////////////////////////////////////////
/* virtual */ void
XFE_RDFToolbar::updateRoot()
{
	// Since a lot of changes will happen to the toolbar, we tell it to 
	// any geometry or layout changes.  We will force these to occur later.
	XtVaSetValues(_toolbar,XmNlayoutFrozen,True,NULL);

	// Destroy the current widgets
	destroyToolbarWidgets();

	// If the toolbar folder is valid, populate the toolbar
	if (isToolbarFolderValid())
	{
        HT_Resource toolbarRoot = getRootFolder();

        HT_SetOpenState(toolbarRoot, PR_TRUE);
        HT_Cursor child_cursor = HT_NewCursor(toolbarRoot);
        HT_Resource child;
		while ( (child = HT_GetNextItem(child_cursor)) )
		{
			addItem(child);
		}
	}

	// Tell the toolbar not too ignore geometry or layout changes anymore
	XtVaSetValues(_toolbar,XmNlayoutFrozen,False,NULL);

	// Force the toolbar to reconfigure.
	XfeManagerLayout(_toolbar);
}
//////////////////////////////////////////////////////////////////////////
/* virtual */ void
XFE_RDFToolbar::configureXfeButton(Widget item,HT_Resource entry)
{
    int32 toolbar_style;
   unsigned char layout;

    getStyleAndLayout(entry, &toolbar_style, &layout);


    if (toolbar_style == BROWSER_TOOLBAR_TEXT_ONLY)
	{
		XtVaSetValues(item,
					  XmNpixmap,			XmUNSPECIFIED_PIXMAP,
					  XmNpixmapMask,		XmUNSPECIFIED_PIXMAP,
					  NULL);

		XtVaSetValues(item, XmNbuttonLayout, layout, NULL);
	}
	else
	{
		Pixmap pixmap;
		Pixmap pixmapMask;

		getPixmapsForEntry(entry,&pixmap,&pixmapMask,NULL,NULL);
        XtVaSetValues(item,
                      XmNpixmap,		pixmap,
                      XmNpixmapMask,	pixmapMask,
                      XmNbuttonLayout,	layout,
                      NULL);
	}

#ifdef NOT_YET
	// Add popup callback to item
	XtAddCallback(item,
				  XmNbutton3DownCallback,
				  &XFE_RDFToolbar::popupCB,
				  (XtPointer) this);
#endif
}
//////////////////////////////////////////////////////////////////////////
/* virtual */ void
XFE_RDFToolbar::configureXfeCascade(Widget item,HT_Resource entry)
{
    int32 toolbar_style;
    unsigned char layout;

    getStyleAndLayout(entry, &toolbar_style, &layout);
    D(printf("XFE_RDFToolbar::configureXfeCascade: toolbar_style = %d layout = %d\n", toolbar_style, layout););

    if (toolbar_style == BROWSER_TOOLBAR_TEXT_ONLY)
	{
		XtVaSetValues(item,
					  XmNpixmap,			XmUNSPECIFIED_PIXMAP,
					  XmNarmedPixmap,		XmUNSPECIFIED_PIXMAP,
					  XmNpixmapMask,		XmUNSPECIFIED_PIXMAP,
					  XmNarmedPixmapMask,	XmUNSPECIFIED_PIXMAP,
					  NULL);

		XtVaSetValues(item,XmNbuttonLayout,layout,NULL);
	}
	else
	{
		Pixmap		pixmap;
		Pixmap		armedPixmap;
		Pixmap		pixmapMask;
		Pixmap		armedPixmapMask;
		Arg			av[4];
		Cardinal	ac = 0;

		getPixmapsForEntry(entry,&pixmap,&pixmapMask,
						   &armedPixmap,&armedPixmapMask);


		XtSetArg(av[ac],XmNpixmap,			pixmap); ac++;
		XtSetArg(av[ac],XmNpixmapMask,		pixmapMask); ac++;

		// Only show the aremd pixmap/mask if this entry has children
		if (XfeIsAlive(XfeCascadeGetSubMenu(item)))
		{
			XtSetArg(av[ac],XmNarmedPixmap,		armedPixmap); ac++;
			XtSetArg(av[ac],XmNarmedPixmapMask,	armedPixmapMask); ac++;
		}

		XtSetValues(item,av,ac);

		XtVaSetValues(item,XmNbuttonLayout,layout,NULL);
	}

#ifdef NOT_YET
	// Add popup callback to item
	XtAddCallback(item,
				  XmNbutton3DownCallback,
				  &XFE_RDFToolbar::popupCB,
				  (XtPointer) this);
#endif
}
//////////////////////////////////////////////////////////////////////////


Widget 
XFE_RDFToolbar::createXfeButton(Widget parent,HT_Resource entry)
{
    XP_ASSERT( XfeIsAlive(parent) );

    D(printf("Create xfe push : %s\n",HT_GetNodeName(entry)));

    Widget                  button;
    Arg                     av[20];
    Cardinal                ac;
    ItemCallbackStruct *    data = NULL;
    Boolean                 enableTooltips = True;

    ac = 0;
    XtSetArg(av[ac],XmNuserData, entry);  ac++;
    XtSetArg(av[ac],XmNforceDimensionToMax, False);  ac++;

    button = XfeCreateButton(parent,"bookmarkButton",av,ac);

    // Set the item's label
    setItemLabelString(button,entry);

    configureXfeButton(button,entry);

    // Create a new bookmark data structure for the callbacks
    data = XP_NEW_ZAP(ItemCallbackStruct);

    data->object    = this;
    data->entry        = entry;

      /* Add the tooltip Callback */
       XfeTipStringAdd(button);
       XfeTipStringSetObtainCallback(button, (XfeTipStringObtainCallback)tooltipCB, (XtPointer) data);

       /* Set up the status bar text */
       XfeDocStringAdd(button);
       XfeDocStringSetObtainCallback(button, docStringSetCB, (XtPointer)data);

       XfeDocStringSetCallback(button, docStringCB, (XtPointer)data);

    XtAddCallback(button,
                  XmNactivateCallback,
                  &XFE_RDFMenuToolbarBase::item_activated_cb,
                  (XtPointer) data);

    XtAddCallback(button,
                  XmNarmCallback,
                  &XFE_RDFMenuToolbarBase::item_armed_cb,
                  (XtPointer) data);

    XtAddCallback(button,
                  XmNdisarmCallback,
                  &XFE_RDFMenuToolbarBase::item_disarmed_cb,
                  (XtPointer) data);

    /*
    XtAddCallback(button,
                  XmNenterCallback,
                  &XFE_RDFMenuToolbarBase::item_enter_cb,
                  (XtPointer) data);

    XtAddCallback(button,
                  XmNleaveCallback,
                  &XFE_RDFMenuToolbarBase::item_leave_cb,
                  (XtPointer) data);
                  */
    XtAddCallback(button,
                  XmNdestroyCallback,
                  &XFE_RDFMenuToolbarBase::item_free_data_cb,
                  (XtPointer) data);

    return button;
}
//////////////////////////////////////////////////////////////////////////
Widget 
XFE_RDFToolbar::createXfeCascade(Widget parent,HT_Resource entry)
{
    XP_ASSERT( XfeIsAlive(parent) );

    D(printf("Create xfe cascade : %s\n",HT_GetNodeName(entry)));

    Widget                  cascade;
    Widget                  submenu;
    Arg                     av[5];
    Cardinal                ac;
    ItemCallbackStruct *    data = NULL;

    ac = 0;
    XtSetArg(av[ac],XmNuserData, entry);  ac++;
    XtSetArg(av[ac],XmNforceDimensionToMax, False);  ac++;

    cascade = XfeCreateCascade(parent,"bookmarkCascade",av,ac);

    
    // Set the item's label
    setItemLabelString(cascade,entry);

    configureXfeCascade(cascade,entry);

    // Create a new bookmark data structure for the callbacks
    data = XP_NEW_ZAP(ItemCallbackStruct);

    data->object    = this;
    data->entry        = entry;

    /* Set the tooltip callback */
    XfeTipStringAdd(cascade);
    XfeTipStringSetObtainCallback(cascade, (XfeTipStringObtainCallback)tooltipCB, (XtPointer) data);

    /* Set up the status bar text */
    XfeDocStringAdd(cascade);
    XfeDocStringSetObtainCallback(cascade, docStringSetCB, (XtPointer)data);
    XfeDocStringSetCallback(cascade, docStringCB, (XtPointer)data);



    XtAddCallback(cascade,
                  XmNcascadingCallback,
                  &XFE_RDFMenuToolbarBase::item_cascading_cb,
                  (XtPointer) data);

    /*
    XtAddCallback(cascade,
                  XmNenterCallback,
                  &XFE_RDFMenuToolbarBase::item_enter_cb,
                  (XtPointer) data);

    XtAddCallback(cascade,
                  XmNleaveCallback,
                  &XFE_RDFMenuToolbarBase::item_leave_cb,
                  (XtPointer) data);
                  */
    XtAddCallback(cascade,
                  XmNdestroyCallback,
                  &XFE_RDFMenuToolbarBase::item_free_data_cb,
                  (XtPointer) data);

    // Obtain the cascade's submenu
    XtVaGetValues(cascade,XmNsubMenuId,&submenu,NULL);

    // Keep track of the submenu mapping
    trackSubmenuMapping(submenu);

    return cascade;
}
//////////////////////////////////////////////////////////////////////////
Widget 
XFE_RDFToolbar::createUrlBar(Widget parent,HT_Resource entry)
{
    XP_ASSERT( XfeIsAlive(parent) );

	XFE_ToolbarUrlBar * urlbar = new XFE_ToolbarUrlBar(_frame,
													   parent,
													   entry,
													   "toolbarUrlBar");

	urlbar->registerInterest(XFE_ToolbarUrlBar::urlBarTextActivatedNotice,
                             this,
                             urlBarTextActivatedNotice_cb,
							 (void *) urlbar);

	urlbar->initialize();

	return urlbar->getBaseWidget();
}
//////////////////////////////////////////////////////////////////////////
/* virtual */ void
XFE_RDFToolbar::updateAppearance()
{
    int32 toolbar_style;
    PREF_GetIntPref("browser.chrome.toolbar_style", &toolbar_style);

    if (toolbar_style == BROWSER_TOOLBAR_TEXT_ONLY)
	{
		XtVaSetValues(_toolbar,XmNbuttonLayout,XmBUTTON_LABEL_ONLY,NULL);
	}
	else
	{
		XtVaSetValues(_toolbar,XmNbuttonLayout,XmBUTTON_LABEL_ON_RIGHT,NULL);
	}

	updateRoot();
}
//////////////////////////////////////////////////////////////////////////
/* static */
void
XFE_RDFToolbar::tooltipCB(Widget w, XtPointer client_data, XmString * string_return, Boolean * need_to_free_string )
{

    ItemCallbackStruct * ttip = (ItemCallbackStruct * )client_data;
    XFE_RDFToolbar * obj = (XFE_RDFToolbar *) ttip->object;
    HT_Resource  entry = (HT_Resource) ttip->entry;

    void *        data=NULL;
    XmFontList    font_list;
    XmString      str = NULL;

    HT_GetTemplateData(HT_TopNode(HT_GetView(entry)), gNavCenter->buttonTooltipText, HT_COLUMN_STRING, &data);
    if (data) {
       MWContext * context = (obj->getFrame())->getContext();
       INTL_CharSetInfo charSetInfo =
            LO_GetDocumentCharacterSetInfo(context);
    
       str = fe_ConvertToXmString((unsigned char *) data,
                                        INTL_GetCSIWinCSID(charSetInfo) ,
                                        NULL, XmFONT_IS_FONT, &font_list);

    
       *string_return = str;
       *need_to_free_string = True;
    }
    else 
    {
      MWContext * context = (obj->getFrame())->getContext();

      *string_return = XFE_RDFUtils::getStringFromResource(context,entry);
      *need_to_free_string = True;
    }

}

//////////////////////////////////////////////////////////////////////////
/* static */
void
XFE_RDFToolbar::docStringSetCB(Widget w, XtPointer client_data, XmString * string_return, Boolean * need_to_free_string )
{

    ItemCallbackStruct * ttip = (ItemCallbackStruct * )client_data;
    XFE_RDFToolbar * obj = (XFE_RDFToolbar *) ttip->object;
    HT_Resource  entry = (HT_Resource) ttip->entry;

    void *        data=NULL;
    XmFontList    font_list;
    XmString      str = NULL;

    HT_GetTemplateData(HT_TopNode(HT_GetView(entry)), gNavCenter->buttonStatusbarText, HT_COLUMN_STRING, &data);
    if (data) {
      D(printf("Doc string obtained from HT = %s\n", (char *) data););
       MWContext * context = (obj->getFrame())->getContext();
    
       INTL_CharSetInfo charSetInfo =
            LO_GetDocumentCharacterSetInfo(context);
     
       str = fe_ConvertToXmString((unsigned char *) data,
                                        INTL_GetCSIWinCSID(charSetInfo) ,
                                        NULL, XmFONT_IS_FONT, &font_list);

    
       *string_return = str;
       *need_to_free_string = True;
    }
    else
	{
		
		Boolean isContainer = HT_IsContainer(entry);
		MWContext * context = (obj->getFrame())->getContext();
		
		*string_return = XFE_RDFUtils::getStringFromResource(context,entry);

		*need_to_free_string = True;
    }

}


//////////////////////////////////////////////////////////////////////////
/* static */
void
XFE_RDFToolbar::docStringCB(Widget w, XtPointer client_data, unsigned char reason, XmString string_return)
{
    ItemCallbackStruct * ttip = (ItemCallbackStruct * )client_data;
    XFE_RDFToolbar * obj = (XFE_RDFToolbar *) ttip->object;
    HT_Resource  entry = (HT_Resource) ttip->entry;

    char *      str = NULL;

    XmStringGetLtoR(string_return, XmSTRING_DEFAULT_CHARSET, &str);

    if (reason  == XfeDOC_STRING_SET) {

      obj->getFrame()->notifyInterested(XFE_View::statusNeedsUpdatingMidTruncated,
                             (void*) str);

    }
    else if (reason == XfeDOC_STRING_CLEAR) {

       obj->getFrame()->notifyInterested(XFE_View::statusNeedsUpdatingMidTruncated,
                             (void*) "");

    }

}
//////////////////////////////////////////////////////////////////////////
XFE_CALLBACK_DEFN(XFE_RDFToolbar,urlBarTextActivatedNotice)
    (XFE_NotificationCenter *    /* obj */, 
     void *                      clientData, 
     void *                      callData)
{
	URL_Struct *			url = (URL_Struct *) callData;
	XFE_ToolbarUrlBar *		urlbar = (XFE_ToolbarUrlBar *) clientData;

	XP_ASSERT( _frame != NULL );
	XP_ASSERT( url != NULL );
	XP_ASSERT( urlbar != NULL );

	// Ask the frame to get the new url.
	// If successfull, update the urlbar
	if (_frame->getURL(url) >= 0)
	{
		// The way this should work is like this:
		//
		// 1.  XFE_BrowserFrame::XFE_BrowserFrame register a newPageLoading
		//     notification with its XFE_HTMLView
		//
		// 2.  In that notification, the browser frame tells the toolbox
		//     that urlbars needs to be updated
		//
		// 3.  The toolbox tells the toolbars that urlbar need to be 
		//     updates
		//
		// 4.  The RDF_Toolbar looks for urlbar and it updates the
		//     urlbar text with the following mehtod.
		//
		// Because im a lazy bastard, ill do this later and for 
		// now ill do the following.
		//
		// Have a nive day -re
		// 
		urlbar->setTextStringFromURL(url);
	}
}
//////////////////////////////////////////////////////////////////////////
