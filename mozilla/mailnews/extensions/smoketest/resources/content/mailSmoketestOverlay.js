/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation. Portions created by Netscape are
 * Copyright (C) 1998-1999 Netscape Communications Corporation. All
 * Rights Reserved.
 */

function StartSmoketestTimers() 
{
  dump("Inside StartSmoketestTimers \n\n\n");
  setTimeout("SelectInboxFolderMessage();",10000);
  setTimeout("MsgReplySender();",15000);
  setTimeout("SendMessageNow();",25000);
  setTimeout("window.close();",35000);
}

function SelectInboxFolderMessage()
{
  var folderURI = window.parent.GetSelectedFolderURI();
  var server = GetServer(folderURI);
  try {
    OpenInboxForServer(server);
  }
  catch(ex) {
    dump("Error -> " + ex + "\n");
  } 
  MsgNextMessage();
}

function SendMessageNow()
{
  var cwindowManager = Components.classes['@mozilla.org/rdf/datasource;1?name=window-mediator'].getService();
  var iwindowManager = Components.interfaces.nsIWindowMediator;           
  var windowManager  = cwindowManager.QueryInterface(iwindowManager);     
  var composeWindow = windowManager.getMostRecentWindow('msgcompose');       
  composeWindow.SendMessage();
}

addEventListener("load",StartSmoketestTimers,false);
