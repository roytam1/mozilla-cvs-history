/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

/*----------------------------------------------------------------------*/
/*																		*/
/* Name:		<Xfe/ToolBox.h>											*/
/* Description:	XfeToolBox widget public header file.					*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/

#ifndef _XfeToolBox_h_							/* start ToolBox.h		*/
#define _XfeToolBox_h_

#include <Xfe/DynamicManager.h>
#include <Xfe/ToolBar.h>
#include <Xfe/Tab.h>

XFE_BEGIN_CPLUSPLUS_PROTECTION

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeToolBox resource names											*/
/*																		*/
/*----------------------------------------------------------------------*/
#define XmNcloseCallback					"closeCallback"
#define XmNdragAllowCallback				"dragAllowCallback"
#define XmNdragEndCallback					"dragEndCallback"
#define XmNnewItemCallback					"newItemCallback"
#define XmNopenCallback						"openCallback"
#define XmNsnapCallback						"snapCallback"
#define XmNswapCallback						"swapCallback"

/* Things that conflict with Motif 2.x */
#if XmVersion < 2000
#define XmNdragStartCallback				"dragStartCallback"
#endif

#define XmNclosedTabs						"closedTabs"
#define XmNdragButton						"dragButton"
#define XmNdragCursor						"dragCursor"
#define XmNopen								"open"
#define XmNopenedTabs						"openedTabs"
#define XmNswapThreshold					"swapThreshold"
#define XmNtabOffset						"TabOffset"

#define XmCDragButton						"DragButton"
#define XmCDragCursor						"DragCursor"
#define XmCOpen								"Open"
#define XmCSwapThreshold					"SwapThreshold"

/*----------------------------------------------------------------------*/
/*																		*/
/* Callback Reasons														*/
/*																		*/
/*----------------------------------------------------------------------*/
enum
{
    XmCR_TOOL_BOX_ALLOW = XmCR_XFE_LAST_REASON + 99,/* Tool box allow	*/
    XmCR_TOOL_BOX_CLOSE,						/* Tool box close		*/
    XmCR_TOOL_BOX_DRAG_END,						/* Tool box drag end	*/
    XmCR_TOOL_BOX_DRAG_MOTION,					/* Tool box drag motion	*/
    XmCR_TOOL_BOX_DRAG_START,					/* Tool box drag start	*/
    XmCR_TOOL_BOX_NEW_ITEM,						/* Tool box new item    */
    XmCR_TOOL_BOX_OPEN,							/* Tool box open		*/
    XmCR_TOOL_BOX_SNAP,							/* Tool box snap		*/
    XmCR_TOOL_BOX_SWAP							/* Tool box swap		*/
};

/*----------------------------------------------------------------------*/
/*																		*/
/* Tool box callback structures											*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct
{
    int			reason;					/* Reason why CB was invoked	*/
    XEvent *	event;					/* Event that triggered CB		*/
    Widget		item;					/* target Item					*/
    Widget		closed_tab;				/* Corresponding closed tab		*/
    Widget		opened_tab;				/* Corresponding opened tab		*/
	int			index;					/* Index of item				*/
} XfeToolBoxCallbackStruct;

typedef struct
{
    int			reason;					/* Reason why CB was invoked	*/
    XEvent *	event;					/* Event that triggered CB		*/
    Widget		descendant;				/* Descendant requesting drag	*/
    Widget		item;					/* Item to be dragged			*/
    Widget		tab;					/* Corresponding tab			*/
	int			index;					/* Index of item				*/
    Boolean		allow;					/* Allow the drag ?				*/
} XfeToolBoxDragAllowCallbackStruct;

typedef struct
{
    int			reason;					/* Reason why CB was invoked	*/
    XEvent *	event;					/* Event that triggered CB		*/
    Widget		swapped;				/* Item swapped into position	*/
    Widget		displaced;				/* Displaced item				*/
	int			from_index;				/* Index of displaced item		*/
	int			to_index;				/* New index of swapped item	*/
} XfeToolBoxSwapCallbackStruct;

/*----------------------------------------------------------------------*/
/*																		*/
/* XmTOOL_BOX_NOT_FOUND - Used by XfeToolBox methods that return an		*/
/* 'int' index.															*/
/*																		*/
/*----------------------------------------------------------------------*/
#define XmTOOL_BOX_NOT_FOUND -1

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeBox class names													*/
/*																		*/
/*----------------------------------------------------------------------*/
externalref WidgetClass xfeToolBoxWidgetClass;

typedef struct _XfeToolBoxClassRec *		XfeToolBoxWidgetClass;
typedef struct _XfeToolBoxRec *				XfeToolBoxWidget;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeBox subclass test macro											*/
/*																		*/
/*----------------------------------------------------------------------*/
#define XfeIsToolBox(w)	XtIsSubclass(w,xfeToolBoxWidgetClass)

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeToolBox Public Methods											*/
/*																		*/
/*----------------------------------------------------------------------*/
extern Widget
XfeCreateToolBox				(Widget			parent,
								 String			name,
								 Arg *			args,
								 Cardinal		num_args);
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeToolBox drag descendant											*/
/*																		*/
/*----------------------------------------------------------------------*/
extern void
XfeToolBoxAddDragDescendant		(Widget			w,
								 Widget			descendant);
/*----------------------------------------------------------------------*/
extern void
XfeToolBoxRemoveDragDescendant	(Widget			w,
								 Widget			descendant);
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeToolBox index methods												*/
/*																		*/
/*----------------------------------------------------------------------*/
extern int
XfeToolBoxItemGetIndex			(Widget			w,
								 Widget			item);
/*----------------------------------------------------------------------*/
extern Widget
XfeToolBoxItemGetByIndex		(Widget			w,
								 Cardinal		index);
/*----------------------------------------------------------------------*/
extern Widget
XfeToolBoxItemGetTab			(Widget			w,
								 Widget			item,
								 Boolean		opened);
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeToolBox position methods											*/
/*																		*/
/*----------------------------------------------------------------------*/
extern  void
XfeToolBoxItemSetPosition		(Widget			w,
								 Widget			item,
								 int			position);
/*----------------------------------------------------------------------*/
extern int
XfeToolBoxItemGetPosition		(Widget			w,
								 Widget			item);
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeToolBox open methods												*/
/*																		*/
/*----------------------------------------------------------------------*/
extern  void
XfeToolBoxItemSetOpen			(Widget			w,
								 Widget			item,
								 Boolean		open);
/*----------------------------------------------------------------------*/
extern Boolean
XfeToolBoxItemGetOpen			(Widget			w,
								 Widget			item);
/*----------------------------------------------------------------------*/
extern  void
XfeToolBoxItemToggleOpen		(Widget			w,
								 Widget			item);
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeToolBoxIsNeeded()													*/
/*																		*/
/*----------------------------------------------------------------------*/
extern Boolean 
XfeToolBoxIsNeeded				(Widget			w);
/*----------------------------------------------------------------------*/

XFE_END_CPLUSPLUS_PROTECTION

#endif											/* end ToolBox.h		*/
