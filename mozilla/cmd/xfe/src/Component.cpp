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
   Component.cpp -- class for all XFE thingy's which have a widget.
   Created: Chris Toshok <toshok@netscape.com>, 7-Aug-96.
 */



#include "mozilla.h"
#include "xfe.h"

#include "Component.h"
#include "xpassert.h"

#include <Xfe/Xfe.h>

// Callback message strings.  (Comment about these declarations in header file)

static char myClassName[] = "XFE_Component::className";

const char *XFE_Component::afterRealizeCallback = "XFE_Component::afterRealizeCallback";

// Progress bar cylon notifications
const char * XFE_Component::progressBarCylonStart = "XFE_Component::progressBarCylonStart";
const char * XFE_Component::progressBarCylonStop = "XFE_Component::progressBarCylonStop";
const char * XFE_Component::progressBarCylonTick = "XFE_Component::progressBarCylonTick";

// Progress bar percentage notifications
const char * XFE_Component::progressBarUpdatePercent = "XFE_Component::progressBarUpdatePercent";
const char * XFE_Component::progressBarUpdateText = "XFE_Component::progressBarUpdateText";

// Logo animation notifications
const char * XFE_Component::logoStartAnimation = "XFE_Component::logoStartAnimation";
const char * XFE_Component::logoStopAnimation = "XFE_Component::logoStopAnimation";


//////////////////////////////////////////////////////////////////////////
XFE_Component::XFE_Component(XFE_Component *toplevel_component)
{
  m_widget = 0;
  m_toplevel = toplevel_component;
}
//////////////////////////////////////////////////////////////////////////
XFE_Component::XFE_Component(Widget w, XFE_Component *toplevel_component)
{
  m_widget = w;
  m_toplevel = toplevel_component;
}
//////////////////////////////////////////////////////////////////////////
XFE_Component::~XFE_Component()
{
	//
	// delete the widget tree rooted at this frame.
	//
	if (m_widget)
    {
		// first we remove the callback to avoid deleting ourselves twice.
		XtRemoveCallback(m_widget, XmNdestroyCallback, destroy_cb, this);
		
		if (XfeIsAlive(m_widget))
		{
			// then we destroy the widget.
			XtDestroyWidget(m_widget);
		}
		
		m_widget = 0;
    }
}
//////////////////////////////////////////////////////////////////////////
const char* 
XFE_Component::getClassName()
{
	return myClassName;
}
//////////////////////////////////////////////////////////////////////////
XP_Bool 
XFE_Component::isClassOf(char *name)
{
	XP_Bool ans = False;
	if (name) {
		char tmp[256];
		sprintf(tmp, "XFE_%s::className", name);

		const char* className = getClassName();
#if defined(DEBUG_tao_)
		printf("\n**XFE_Component::isClassOf %s,%s\n", tmp, className);
#endif
		if (!XP_STRCMP(tmp, className))
			ans = True;
	}/* if */
	return ans;
}
//////////////////////////////////////////////////////////////////////////
Widget
XFE_Component::getBaseWidget()
{
  return m_widget;
}
//////////////////////////////////////////////////////////////////////////
void
XFE_Component::setBaseWidget(Widget w)
{
  /* we don't allow reassigning the base widget of a component. */
  XP_ASSERT(m_widget == 0);

  m_widget = w;
}
//////////////////////////////////////////////////////////////////////////
/* virtual */ void
XFE_Component::setSensitive(Boolean sensitive)
{
  XtSetSensitive(m_widget, sensitive);
}
//////////////////////////////////////////////////////////////////////////
/* virtual */ Boolean
XFE_Component::isSensitive()
{
  return XtIsSensitive(m_widget);
}
//////////////////////////////////////////////////////////////////////////
XFE_Component *
XFE_Component::getToplevel()
{
  return m_toplevel;
}
//////////////////////////////////////////////////////////////////////////
XP_Bool
XFE_Component::isShown()
{
  return XtIsManaged(m_widget);
}
//////////////////////////////////////////////////////////////////////////
XP_Bool
XFE_Component::isAlive()
{
  return XfeIsAlive(m_widget);
}
//////////////////////////////////////////////////////////////////////////

void
XFE_Component::show()
{
  XtManageChild(m_widget);
}
//////////////////////////////////////////////////////////////////////////
void
XFE_Component::hide()
{
  XtUnmanageChild(m_widget);
}
//////////////////////////////////////////////////////////////////////////
void
XFE_Component::setShowingState(XP_Bool showing)
{
  if (showing)
  {
    show();
  }
  else
  {
    hide();
  }
}
//////////////////////////////////////////////////////////////////////////
void
XFE_Component::toggleShowingState()
{
  if (isShown())
  {
    hide();
  }
  else
  {
    show();
  }
}
//////////////////////////////////////////////////////////////////////////
fe_colormap *
XFE_Component::getColormap()
{
  XP_ASSERT(0);
  return NULL;
}
//////////////////////////////////////////////////////////////////////////

void
XFE_Component::translateFromRootCoords(int x_root, int y_root, 
				       int *x, int *y)
{
  Position my_xroot, my_yroot;

  XtTranslateCoords(m_widget, 0, 0, &my_xroot, &my_yroot);

  *x = x_root - my_xroot;
  *y = y_root - my_yroot;
}
//////////////////////////////////////////////////////////////////////////
Boolean 
XFE_Component::isWidgetInside(Widget w)
{
  Widget cur;

  cur = w;

  while (!XtIsShell(cur)
	 && cur != m_widget)
    cur = XtParent(cur);

  if (cur == m_widget)
    {
      return True;
    }
  else
    {
      return False;
    }
}
//////////////////////////////////////////////////////////////////////////
Pixel
XFE_Component::getFGPixel()
{
  XP_ASSERT( m_toplevel != NULL );

  return m_toplevel->getFGPixel();
}
//////////////////////////////////////////////////////////////////////////
Pixel
XFE_Component::getBGPixel()
{
  XP_ASSERT( m_toplevel != NULL );

  return m_toplevel->getFGPixel();
}
//////////////////////////////////////////////////////////////////////////
Pixel
XFE_Component::getTopShadowPixel()
{
  XP_ASSERT( m_toplevel != NULL );

  return m_toplevel->getFGPixel();
}
//////////////////////////////////////////////////////////////////////////
Pixel
XFE_Component::getBottomShadowPixel()
{
  XP_ASSERT( m_toplevel != NULL );

  return m_toplevel->getFGPixel();
}
//////////////////////////////////////////////////////////////////////////
char *
XFE_Component::stringFromResource(char *command_string)
{
  return XfeSubResourceGetWidgetStringValue(m_widget, 
											command_string, 
											command_string /* XXX */);
}
//////////////////////////////////////////////////////////////////////////
// use this method to get 'cmd.labelString' resources
// for the specified widget (default is the base widget)
char *
XFE_Component::getLabelString(char* cmd, Widget widget)
{
	if (!widget) {
		widget = getBaseWidget();
		if (!widget)
			return NULL;
	}
//////////////////////////////////////////////////////////////////////////
	return XfeSubResourceGetStringValue(widget,
										cmd,
										XfeClassNameForWidget(widget),
										XmNlabelString,
										XmCLabelString,
										NULL);
}
//////////////////////////////////////////////////////////////////////////
// use this method to get 'cmd.[show|hide]LabelString' resources
// for the specified widget (default is the base widget)
char*
XFE_Component::getShowHideLabelString(char* cmd, Boolean show, Widget widget)
{
	char* name;

	if (!widget) {
		widget = getBaseWidget();
		if (!widget)
			return NULL;
	}

	if (show)
		name = "showLabelString";
	else
		name = "hideLabelString";

	return XfeSubResourceGetStringValue(widget,
										cmd,
										XfeClassNameForWidget(widget),
										name,
										XmCLabelString,
										NULL);
}
//////////////////////////////////////////////////////////////////////////
void
XFE_Component::installDestroyHandler()
{
	XP_ASSERT( isAlive() );

	XtAddCallback(m_widget, XmNdestroyCallback, destroy_cb, this);
}
//////////////////////////////////////////////////////////////////////////
void
XFE_Component::destroy_cb(Widget, XtPointer cd, XtPointer)
{
  XFE_Component *obj = (XFE_Component*)cd;

  delete obj;
}
//////////////////////////////////////////////////////////////////////////
char *
XFE_Component::getDocString(CommandType /* cmd */)
{
   return NULL;
}
//////////////////////////////////////////////////////////////////////////
char *
XFE_Component::getTipString(CommandType /* cmd */)
{
   return NULL;
}
//////////////////////////////////////////////////////////////////////////

