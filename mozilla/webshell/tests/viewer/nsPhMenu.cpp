/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

#include <Pt.h>

#include "nsBrowserWindow.h"
#include "resources.h"
#include "nscore.h"

#include "stdio.h"

extern void MenuProc(PRUint32 aId);

int MenuItemActivate (PtWidget_t *widget, void *command, PtCallbackInfo_t *cbinfo)
{
//  printf("MenuItemActivateCb command=<%p>\n", command);
  ::MenuProc((PRUint32)command);

  return Pt_CONTINUE;
}


int TopLevelMenuItemArm (PtWidget_t *widget, void *Menu, PtCallbackInfo_t *cbinfo)
{
//  printf("TopLevelMenuItemArm Menu=<%p>\n", Menu);
  
    PtPositionMenu ( (PtWidget_t *) Menu, NULL);
	PtRealizeWidget( (PtWidget_t *) Menu);  
}  

PtWidget_t *CreateTopLevelMenu(PtWidget_t *aParent, char *Label)
{
  PtArg_t     arg[10];
  PtWidget_t  *menu, *menubutton;

    PtSetArg(&arg[0], Pt_ARG_BUTTON_TYPE, Pt_MENU_DOWN, 0);
    PtSetArg(&arg[1], Pt_ARG_TEXT_STRING, Label, 0);
    menubutton = PtCreateWidget(PtMenuButton, aParent, 2, arg);

    /* Now Create the Photon Menu that is attached to the mMenuButton */
    PtSetArg(&arg[0], Pt_ARG_MENU_FLAGS, Pt_MENU_AUTO, 0xFFFFFFFF);
    menu = PtCreateWidget (PtMenu, menubutton, 1, arg);

   /* Set the Call back on this top level menu button */
    PtAddCallback(menubutton, Pt_CB_ARM, TopLevelMenuItemArm, menu);

    PtRealizeWidget(menubutton);

//  printf("CreateTopLevelMenu: Label=<%s> menubutton=<%p> menu=<%p>\n", Label, menubutton, menu);
    return menu;
}


PtWidget_t *CreateSubMenu(PtWidget_t *aParent, char *Label)
{
  PtArg_t     arg[10];
  PtWidget_t  *menu, *menubutton;

	PtSetArg(&arg[0], Pt_ARG_BUTTON_TYPE, Pt_MENU_RIGHT, 0);
    PtSetArg(&arg[1], Pt_ARG_TEXT_STRING, Label, 0);
    menubutton = PtCreateWidget(PtMenuButton, aParent, 2, arg);

    PtSetArg(&arg[0], Pt_ARG_MENU_FLAGS, Pt_MENU_CHILD | Pt_MENU_AUTO, 0xFFFFFFFF);
    menu = PtCreateWidget (PtMenu, menubutton, 1, arg);

    return menu;
}

PtWidget_t *CreateMenuItem(PtWidget_t *aParent, char *Label, int command)
{
  PtArg_t     arg[10];
  PtWidget_t  *menuitem;

//printf("CreateMenuItem: aParent=<%p>, Label=<%s> command=<%d>\n", aParent, Label, command);
  
  PtSetArg ( &arg[0], Pt_ARG_BUTTON_TYPE, Pt_MENU_TEXT, 0);
  PtSetArg ( &arg[1], Pt_ARG_USER_DATA, &command, sizeof(void *) );
  if (Label == NULL)
  {
    PtSetArg ( &arg[2], Pt_ARG_SEP_TYPE, Pt_ETCHED_IN, 0);
    menuitem = PtCreateWidget (PtSeparator, aParent, 3, arg);
  }
  else
  {
    PtSetArg ( &arg[2], Pt_ARG_TEXT_STRING, Label, 0);
    menuitem = PtCreateWidget (PtMenuButton, aParent, 3, arg);
    PtAddCallback(menuitem, Pt_CB_ACTIVATE, MenuItemActivate , (void *) command);
  }

  return menuitem;
}

void CreateViewerMenus(PtWidget_t *menubar, void *data) 
{
  PtArg_t     arg[10];
  PtWidget_t  *menu, *submenu, *submenu2;

//printf("Entering CreateViewerMenus menubar=<%p>\n", menubar);

  menu = CreateTopLevelMenu(menubar, "File");
  CreateMenuItem(menu, "New Window",VIEWER_WINDOW_OPEN);
  CreateMenuItem(menu, "Open",VIEWER_FILE_OPEN);
  CreateMenuItem(menu, "View Source",VIEW_SOURCE);

  submenu=CreateSubMenu(menu, "Samples");

struct MenuList_s {
 char     *Label;
 PRUint32 Command;
};

struct MenuList_s DemoList[18] = 
{ 
  {"demo #0", VIEWER_DEMO0},
  {"demo #1", VIEWER_DEMO1},
  {"demo #2", VIEWER_DEMO2},
  {"demo #3", VIEWER_DEMO3},
  {"demo #4", VIEWER_DEMO4},
  {"demo #5", VIEWER_DEMO5},
  {"demo #6", VIEWER_DEMO6},
  {"demo #7", VIEWER_DEMO7},
  {"demo #8", VIEWER_DEMO8},
  {"demo #9", VIEWER_DEMO9},
  {"demo #10", VIEWER_DEMO10},
  {"demo #11", VIEWER_DEMO11},
  {"demo #12", VIEWER_DEMO12},
  {"demo #13", VIEWER_DEMO13},
  {"demo #14", VIEWER_DEMO14},
  {"demo #15", VIEWER_DEMO15},
  {"demo #16", VIEWER_DEMO16},
  {"demo #17", VIEWER_DEMO17}
};

  for(int i=0; i<=17; i++)
  {
    CreateMenuItem(submenu, DemoList[i].Label,DemoList[i].Command);
  }

  CreateMenuItem(menu, "Test Sites", VIEWER_TOP100);
  submenu=CreateSubMenu(menu, "XPToolkit Tests");
  CreateMenuItem(submenu, "Toolbar Test 1", VIEWER_XPTOOLKITTOOLBAR1);
  CreateMenuItem(submenu, "Tree Test 2", VIEWER_XPTOOLKITTREE1);
  CreateMenuItem(menu, NULL, 0);
  CreateMenuItem(menu, "Print Preview", VIEWER_ONE_COLUMN);
  CreateMenuItem(menu, "Print", VIEWER_PRINT);
  CreateMenuItem(menu, "Print Setup", VIEWER_PRINT_SETUP);
  CreateMenuItem(menu, NULL, 0);
  CreateMenuItem(menu, "Exit", VIEWER_EXIT);

  menu = CreateTopLevelMenu(menubar, "Edit");
  CreateMenuItem(menu, "Cut", VIEWER_EDIT_CUT);
  CreateMenuItem(menu, "Copy", VIEWER_EDIT_COPY);
  CreateMenuItem(menu, "Paste", VIEWER_EDIT_PASTE);
  CreateMenuItem(menu, NULL, 0);
  CreateMenuItem(menu, "Select All", VIEWER_EDIT_SELECTALL);
  CreateMenuItem(menu, NULL, 0);
  CreateMenuItem(menu, "Find in Page", VIEWER_EDIT_FINDINPAGE);
 
//#ifdef DEBUG 
  menu = CreateTopLevelMenu(menubar, "Debug");
  CreateMenuItem(menu, "Visual Debugging", VIEWER_VISUAL_DEBUGGING);
  CreateMenuItem(menu, "Reflow Test", VIEWER_REFLOW_TEST);
  CreateMenuItem(menu, NULL, 0);
  CreateMenuItem(menu, "Dump Content", VIEWER_DUMP_CONTENT);
  CreateMenuItem(menu, "Dump Frames", VIEWER_DUMP_FRAMES);
  CreateMenuItem(menu, "Dump Views", VIEWER_DUMP_VIEWS);
  CreateMenuItem(menu, NULL, 0);
  CreateMenuItem(menu, "Dump Style Sheets", VIEWER_DUMP_STYLE_SHEETS);
  CreateMenuItem(menu, "Dump Style Contexts", VIEWER_DUMP_STYLE_CONTEXTS);
  CreateMenuItem(menu, NULL, 0);
  CreateMenuItem(menu, "Show Content Size", VIEWER_SHOW_CONTENT_SIZE);
  CreateMenuItem(menu, "Show Frame Size", VIEWER_SHOW_FRAME_SIZE);
  CreateMenuItem(menu, "Show Style Size", VIEWER_SHOW_STYLE_SIZE);
  CreateMenuItem(menu, NULL, 0);
  CreateMenuItem(menu, "Debug Save", VIEWER_DEBUGSAVE);
  CreateMenuItem(menu, "Debug Output Text", VIEWER_DISPLAYTEXT);
  CreateMenuItem(menu, "Debug Output HTML", VIEWER_DISPLAYHTML);
  CreateMenuItem(menu, "Debug Toggle Selection", VIEWER_TOGGLE_SELECTION);
  CreateMenuItem(menu, NULL, 0);
  CreateMenuItem(menu, "Debug Robot", VIEWER_DEBUGROBOT);
  CreateMenuItem(menu, NULL, 0);
  CreateMenuItem(menu, "Show Content Quality", VIEWER_SHOW_CONTENT_QUALITY);
  CreateMenuItem(menu, NULL, 0);
  submenu=CreateSubMenu(menu, "Style");
  submenu2=CreateSubMenu(submenu, "Select Style Sheet");
  CreateMenuItem(submenu2, "List Available Sheets", VIEWER_SELECT_STYLE_LIST);
  CreateMenuItem(submenu2, NULL, 0);
  CreateMenuItem(submenu2, "Select Default", VIEWER_SELECT_STYLE_DEFAULT);
  CreateMenuItem(submenu2, NULL, 0);
  CreateMenuItem(submenu2, "Select Alternative 1", VIEWER_SELECT_STYLE_ONE);
  CreateMenuItem(submenu2, "Select Alternative 2", VIEWER_SELECT_STYLE_TWO);
  CreateMenuItem(submenu2, "Select Alternative 3", VIEWER_SELECT_STYLE_THREE);
  CreateMenuItem(submenu2, "Select Alternative 4", VIEWER_SELECT_STYLE_FOUR);
  submenu2=CreateSubMenu(submenu, "Compatibility Mode");
  CreateMenuItem(submenu2, "Nav Quirks", VIEWER_NAV_QUIRKS_MODE);
  CreateMenuItem(submenu2, "Standard", VIEWER_STANDARD_MODE);
  submenu2=CreateSubMenu(submenu, "Widget Render Mode");
  CreateMenuItem(submenu2, "Native", VIEWER_NATIVE_WIDGET_MODE); 
  CreateMenuItem(submenu2, "Gfx", VIEWER_GFX_WIDGET_MODE); 
//#endif

  menu = CreateTopLevelMenu(menubar, "Tools");
  CreateMenuItem(menu, "JavaScript Console", JS_CONSOLE);
  CreateMenuItem(menu, "Editor Mode", EDITOR_MODE);
}

