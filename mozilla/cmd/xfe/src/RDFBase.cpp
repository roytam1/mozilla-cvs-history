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
   RDFBase.cpp - Class wrapper for HT/RDF backend
                 HT Pane creation and notification management
   Created: Stephen Lamm <slamm@netscape.com>, 28-Jul-1998
 */

#include "RDFBase.h"
#include "xp_str.h"
#include "xpassert.h"

#if DEBUG_slamm
#define D(x) x
#else
#define D(x)
#endif

//////////////////////////////////////////////////////////////////////////
//
// RDFBase 'public' methods
//
//////////////////////////////////////////////////////////////////////////

XFE_RDFBase::XFE_RDFBase()
    : _ht_pane(NULL),
      _ht_view(NULL),
      _ht_ns(NULL)
{
}
/*virtual*/
XFE_RDFBase::~XFE_RDFBase() 
{
}
//////////////////////////////////////////////////////////////////////////
void
XFE_RDFBase::newPane()
{
    initPane();

    _ht_pane = HT_NewPane(_ht_ns);
    HT_SetPaneFEData(_ht_pane, this);
}
//////////////////////////////////////////////////////////////////////////
void
XFE_RDFBase::newPaneFromURL(char *url)
{
    int32 param_count = 0;
    char ** param_names = NULL;
    char ** param_values = NULL;

    initPane();

    _ht_pane = HT_PaneFromURL(NULL, url, _ht_ns, 0,
                              param_count, param_names, param_values);
    HT_SetPaneFEData(_ht_pane, this);
}
//////////////////////////////////////////////////////////////////////////
void
XFE_RDFBase::newPaneFromResource(HT_Resource node)
{
    newPaneFromResource(HT_GetRDFResource(node));
}
//////////////////////////////////////////////////////////////////////////
void
XFE_RDFBase::newPaneFromResource(RDF_Resource node)
{
    initPane();

    _ht_pane = HT_PaneFromResource(node, _ht_ns,
                                   PR_FALSE, PR_TRUE, PR_TRUE);

    HT_SetPaneFEData(_ht_pane, this);
}
//////////////////////////////////////////////////////////////////////////
void
XFE_RDFBase::newToolbarPane()
{
    initPane();

    _ht_pane = HT_NewToolbarPane(_ht_ns);
    HT_SetPaneFEData(_ht_pane, this);
}
//////////////////////////////////////////////////////////////////////////
HT_Resource
XFE_RDFBase::getRootFolder()
{
    XP_ASSERT(_ht_pane);

    if (_ht_pane) {
        if (_ht_view == NULL) {
            // No view set.  Get the selected view from HT-backend.
            _ht_view = HT_GetSelectedView(_ht_pane);
        }
        // View are often loaded from the Net, so make sure we have
        // one before we try to use it.
        if (_ht_view) {
            return HT_TopNode(_ht_view);
        }
    }
    return NULL;
}
//////////////////////////////////////////////////////////////////////////
/*virtual*/ void
XFE_RDFBase::updateRoot()
{
}
//////////////////////////////////////////////////////////////////////////
/*virtual*/ void
XFE_RDFBase::notify(HT_Resource n, HT_Event whatHappened)
{
#ifdef DEBUG
  debugEvent(n, whatHappened);
#endif
}
//////////////////////////////////////////////////////////////////////////
//
// RDFBase 'protected' methods
//
//////////////////////////////////////////////////////////////////////////
void 
XFE_RDFBase::notify_cb(HT_Notification ns, HT_Resource n, 
                       HT_Event whatHappened, 
                       void * /*token*/, uint32 /*tokenType*/)
{
  XFE_RDFBase * xfe_rdfpane_obj = (XFE_RDFBase *)ns->data;

  xfe_rdfpane_obj->notify(n, whatHappened);
}
//////////////////////////////////////////////////////////////////////
void
XFE_RDFBase::initPane()
{
    deletePane();

    // Setup the notification struct
    _ht_ns = new HT_NotificationStruct;
    XP_BZERO(_ht_ns, sizeof(HT_NotificationStruct));
    _ht_ns->notifyProc = notify_cb;
    _ht_ns->data = this;
}
//////////////////////////////////////////////////////////////////////
void
XFE_RDFBase::deletePane()
{
    // Only delete the pane if we have a notification struct.
    // No nofication struct means the pane was created by another object.
    if (_ht_ns)
    {
        delete _ht_ns;

        XP_ASSERT(_ht_pane);

        if (_ht_pane)
        {
            HT_DeletePane(_ht_pane);
        }
    }
}
//////////////////////////////////////////////////////////////////////////
void
XFE_RDFBase::setHTView(HT_View view)
{
    XP_ASSERT(view);

    // Nothing to do
    if (view == _ht_view) return;

    _ht_view = view;
    _ht_pane = HT_GetPane(_ht_view);

    updateRoot();
}
//////////////////////////////////////////////////////////////////////////
#ifdef DEBUG
void
XFE_RDFBase::debugEvent(HT_Resource n, HT_Event whatHappened)
{
    HT_View view = HT_GetView(n);

    XP_ASSERT(view);

    char *viewName = HT_GetViewName(view);
    char *nodeName = HT_GetNodeName(n);

    if (strcmp(viewName, nodeName) == 0)
        nodeName = "<same>";

#define EVENTDEBUG(x) printf("%-21s %s, %s\n",(x),\
                             viewName,nodeName);

  switch (whatHappened) {
  case HT_EVENT_NODE_ADDED:
    EVENTDEBUG("NODE_ADDED");
    break;
  case HT_EVENT_NODE_DELETED_DATA:
    EVENTDEBUG("NODE_DELETED_DATA");
    break;
  case HT_EVENT_NODE_DELETED_NODATA:
    EVENTDEBUG("NODE_DELETED_NODATA");
    break;
  case HT_EVENT_NODE_VPROP_CHANGED:
    EVENTDEBUG("NODE_VPROP_CHANGED");
    break;
  case HT_EVENT_NODE_SELECTION_CHANGED:
    EVENTDEBUG("NODE_SELECT");
    break;
  case HT_EVENT_NODE_OPENCLOSE_CHANGED:
    EVENTDEBUG("NODE_OPENCLOSE_CHANGED");
    break;
  case HT_EVENT_VIEW_CLOSED:
    EVENTDEBUG("VIEW_CLOSED");
    break;
  case HT_EVENT_VIEW_SELECTED:
    EVENTDEBUG("VIEW_SELECTED");
    break;
  case HT_EVENT_VIEW_ADDED:
    EVENTDEBUG("VIEW_ADDED");
    break;
  case HT_EVENT_NODE_OPENCLOSE_CHANGING:
    EVENTDEBUG("NODE_OPENCLOSE_CHANGING");
    break;
  case HT_EVENT_VIEW_SORTING_CHANGED:
    EVENTDEBUG("VIEW_SORTING_CHANGED");
    break;
  case HT_EVENT_VIEW_REFRESH:
    EVENTDEBUG("VIEW_REFRESH");
    break;
  case HT_EVENT_VIEW_WORKSPACE_REFRESH:
    EVENTDEBUG("VIEW_WORKSPACE_REFRESH");
    break;
  case HT_EVENT_NODE_EDIT:
    EVENTDEBUG("NODE_EDIT");
    break;
  case HT_EVENT_WORKSPACE_EDIT:
    EVENTDEBUG("WORKSPACE_EDIT");
    break;
  case HT_EVENT_VIEW_HTML_ADD:
    EVENTDEBUG("VIEW_HTML_ADD");
    break;
  case HT_EVENT_VIEW_HTML_REMOVE:
    EVENTDEBUG("VIEW_HTML_REMOVE");
    break;
  case HT_EVENT_NODE_ENABLE:
    EVENTDEBUG("NODE_ENABLE");
    break;
  case HT_EVENT_NODE_DISABLE:
    EVENTDEBUG("NODE_DISABLE");
    break;
  case HT_EVENT_NODE_SCROLLTO:
    EVENTDEBUG("NODE_SCROLLTO");
    break;
  case HT_EVENT_COLUMN_ADD:
    EVENTDEBUG("COLUMN_ADD");
    break;
  case HT_EVENT_COLUMN_DELETE:
    EVENTDEBUG("COLUMN_DELETE");
    break;
  case HT_EVENT_COLUMN_SIZETO:
    EVENTDEBUG("COLUMN_SIZETO");
    break;
  case HT_EVENT_COLUMN_REORDER:
    EVENTDEBUG("COLUMN_REORDER");
    break;
  case HT_EVENT_COLUMN_SHOW:
    EVENTDEBUG("COLUMN_SHOW");
    break;
  case HT_EVENT_COLUMN_HIDE:
    EVENTDEBUG("COLUMN_HIDE");
    break;
  default:
    D(printf("RDFBase: Unknown event, %d, on %s\n",
             whatHappened, HT_GetNodeName(n)));
    break;
  }
}
#endif
