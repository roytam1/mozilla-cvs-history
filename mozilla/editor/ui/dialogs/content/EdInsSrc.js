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
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
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

/* Insert Source HTML dialog */
var srcInput;

// dialog initialization code
function Startup()
{
  if (!InitEditorShell())
    return;

  var okButton = document.documentElement.getButton("accept");
  if (okButton)
  {
    okButton.removeAttribute("default");
    okButton.setAttribute("label",GetString("Insert"));
  }
  // Create dialog object to store controls for easy access
  srcInput = document.getElementById("srcInput");

  var selection = editorShell.GetContentsAs("text/html", 35);
  selection = (selection.replace(/<body[^>]*>/,"")).replace(/<\/body>/,"");
  if (selection != "")
    srcInput.value = selection;

  // Set initial focus
  srcInput.focus();
  // Note: We can't set the caret location in a multiline textbox
  SetWindowLocation();
}

function onAccept()
{
  if (srcInput.value != "")
    editorShell.InsertSource(srcInput.value);
  else {
    dump("Null value -- not inserting in HTML Source dialog\n");
    return false;
  }
  SaveWindowLocation();

  return true;
}

