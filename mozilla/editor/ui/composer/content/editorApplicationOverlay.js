/* -*- Mode: Java; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998-1999
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

/* Implementations of nsIControllerCommand for composer commands */

function initEditorContextMenuItems(aEvent)
{
  var shouldShowEditPage = !gContextMenu.onImage && !gContextMenu.onLink && !gContextMenu.onTextInput && !gContextMenu.inDirList;
  gContextMenu.showItem( "context-editpage", shouldShowEditPage );

  var shouldShowEditLink = gContextMenu.onSaveableLink; 
  gContextMenu.showItem( "context-editlink", shouldShowEditLink );

  // Hide the applications separator if there's no add-on apps present. 
  gContextMenu.showItem("context-sep-apps", gContextMenu.shouldShowSeparator("context-sep-apps"));
}
  
function initEditorContextMenuListener(aEvent)
{
  var popup = document.getElementById("contentAreaContextMenu");
  if (popup)
    popup.addEventListener("popupshowing", initEditorContextMenuItems, false);
}

addEventListener("load", initEditorContextMenuListener, false);

function editLink(aLinkURL)
{
  urlSecurityCheck(aLinkURL, window.document);  // XXX what is this? Why do we pass the chrome doc?
  editPage(aLinkURL, window, false);
}

function editDocument(aDocument)      
{
  if (!aDocument)
    aDocument = window._content.document;
  
  editPage(aDocument.URL, window, false); 
}

function editPageOrFrame()
{
  var url;
  var focusedWindow = document.commandDispatcher.focusedWindow;
  if (isDocumentFrame(focusedWindow))
    url = focusedWindow.location.href;
  else
    url = window._content.location.href;

  editPage(url, window, false)
}

// Any non-editor window wanting to create an editor with a URL
//   should use this instead of "window.openDialog..."
//  We must always find an existing window with requested URL
// (When calling from a dialog, "launchWindow" is dialog's "opener"
//   and we need a delay to let dialog close)
function editPage(url, launchWindow, delay)
{
  var focusedWindow = document.commandDispatcher.focusedWindow;
  if (isDocumentFrame(focusedWindow))
    url = focusedWindow.location.href;

  // User may not have supplied a window
  if (!launchWindow)
  {
    if (window)
    {
      launchWindow = window;
    }
    else
    {
      dump("No window to launch an editor from!\n");
      return;
    }
  }

  // if the current window is a browser window, then extract the current charset menu setting from the current 
  // document and use it to initialize the new composer window...

  var wintype = document.firstChild.getAttribute('windowtype');
  var charsetArg;

  if (launchWindow && (wintype == "navigator:browser") && launchWindow._content.document)
    charsetArg = "charset=" + launchWindow._content.document.characterSet;

  try {
    var windowManager = Components.classes['@mozilla.org/rdf/datasource;1?name=window-mediator'].getService();
    var windowManagerInterface = windowManager.QueryInterface( Components.interfaces.nsIWindowMediator);
    var enumerator = windowManagerInterface.getEnumerator( "composer:html" );
    var emptyWindow;
    while ( enumerator.hasMoreElements() )
    {
      var win = windowManagerInterface.convertISupportsToDOMWindow( enumerator.getNext() );
      if ( win && win.editorShell)
      {
        if (win.editorShell.checkOpenWindowForURLMatch(url, win))
        {
          // We found an editor with our url
          win.focus();
          return;
        }
        else if (!emptyWindow && win.PageIsEmptyAndUntouched())
        {
          emptyWindow = win;
        }
      }
    }

    if (emptyWindow)
    {
      // we have an empty window we can use
      if (emptyWindow.IsInHTMLSourceMode())
        emptyWindow.FinishHTMLSource();
      emptyWindow.editorShell.LoadUrl(url);
      emptyWindow.focus();
      emptyWindow.SetSaveAndPublishUI(url);
      return;
    }

    // Create new Composer window
    if (delay)
    {
      dump("delaying\n");
      launchWindow.delayedOpenWindow("chrome://editor/content", "chrome,all,dialog=no", url);
    }
    else
      launchWindow.openDialog("chrome://editor/content", "_blank", "chrome,all,dialog=no", url, charsetArg);

  } catch(e) {}
}

function NewEditorFromTemplate()
{
  // XXX not implemented
}

function NewEditorFromDraft()
{
  // XXX not implemented
}
