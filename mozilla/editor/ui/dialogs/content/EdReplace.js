/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 * The Original Code is Mozilla Communicator client code, released March
 * 31, 1998.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation. Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): Kin Blas <kin@netscape.com>
 * Contributor(s): Akkana Peck <akkana@netscape.com>
 *
 */

var gReplaceDialog;      // Quick access to document/form elements.
var gFindInst;           // nsIWebBrowserFind that we're going to use
var gFindService;        // Global service which remembers find params
var gEditor;             // the editor we're using

function initDialogObject()
{
  // Create gReplaceDialog object and initialize.
  gReplaceDialog = new Object();
  gReplaceDialog.findInput       = document.getElementById("dialog.findInput");
  gReplaceDialog.replaceInput    = document.getElementById("dialog.replaceInput");
  gReplaceDialog.caseSensitive   = document.getElementById("dialog.caseSensitive");
  gReplaceDialog.wrap            = document.getElementById("dialog.wrap");
  gReplaceDialog.searchBackwards = document.getElementById("dialog.searchBackwards");
  gReplaceDialog.findNext        = document.getElementById("findNext");
  gReplaceDialog.replace         = document.getElementById("replace");
  gReplaceDialog.replaceAndFind  = document.getElementById("replaceAndFind");
  gReplaceDialog.replaceAll      = document.getElementById("replaceAll");
  gEditor                        = null;
}

function loadDialog()
{
  // Set initial dialog field contents.
  // Set initial dialog field contents. Use the gFindInst attributes first,
  // this is necessary for window.find()
  gReplaceDialog.findInput.value         = (gFindInst.searchString
                                            ? gFindInst.searchString
                                            : gFindService.searchString);
  gReplaceDialog.replaceInput.value = gFindService.replaceString;
  gReplaceDialog.caseSensitive.checked   = (gFindInst.matchCase
                                            ? gFindInst.matchCase
                                            : gFindService.matchCase);
  gReplaceDialog.wrap.checked            = (gFindInst.wrapFind
                                            ? gFindInst.wrapFind
                                            : gFindService.wrapFind);
  gReplaceDialog.searchBackwards.checked = (gFindInst.findBackwards
                                            ? gFindInst.findBackwards
                                            : gFindService.findBackwards);

  doEnabling();
}

function onLoad()
{
  // Get the xul <editor> element:
  var editorXUL = window.opener.document.getElementById("content-frame");

  // Get the nsIWebBrowserFind service:
  gFindInst = editorXUL.webBrowserFind;

  try {
  // get the find service, which stores global find state
    gFindService = Components.classes["@mozilla.org/find/find_service;1"]
                         .getService(Components.interfaces.nsIFindService);
  } catch(e) { dump("No find service!\n"); gFindService = 0; }

  // Init gReplaceDialog.
  initDialogObject();

  try {
    gEditor = editorXUL.editorShell.editor.QueryInterface(Components.interfaces.nsIPlaintextEditor);
  } catch(e) { dump("Couldn't get an editor! " + e + "\n"); }
  // If we don't get the editor, then we won't allow replacing.

  // Change "OK" to "Find".
  //dialog.find.label = document.getElementById("fBLT").getAttribute("label");

  // Fill dialog.
  loadDialog();

  if (gReplaceDialog.findInput.value)
    gReplaceDialog.findInput.select();
  else
    gReplaceDialog.findInput.focus();
}

function onUnload() {
  // Disconnect context from this dialog.
  gFindReplaceData.replaceDialog = null;
}

function saveFindData()
{
  // Set data attributes per user input.
  if (gFindService)
  {
    gFindService.searchString  = gReplaceDialog.findInput.value;
    gFindService.matchCase     = gReplaceDialog.caseSensitive.checked;
    gFindService.wrapFind      = gReplaceDialog.wrap.checked;
    gFindService.findBackwards = gReplaceDialog.searchBackwards.checked;
  }
}

function setUpFindInst()
{
  gFindInst.searchString  = gReplaceDialog.findInput.value;
  gFindInst.matchCase     = gReplaceDialog.caseSensitive.checked;
  gFindInst.wrapFind      = gReplaceDialog.wrap.checked;
  gFindInst.findBackwards = gReplaceDialog.searchBackwards.checked;
}

function onFindNext()
{
  // Transfer dialog contents to the find service.
  saveFindData();
  // set up the find instance
  setUpFindInst();

  // Search.
  var result = gFindInst.findNext();

  if (!result)
  {
    var bundle = document.getElementById("findBundle");
    window.alert(bundle.getString("notFoundWarning"));
    SetTextboxFocus(gReplaceDialog.findKey);
    gReplaceDialog.findInput.select();
    gReplaceDialog.findInput.focus();
    return false;
  } 
  return true;
}

function onReplace()
{
  if (!gEditor)
    return;

  // Does the current selection match the find string?
  var selection = gEditor.selection;

  var selStr = selection.toString();
  var specStr = gReplaceDialog.findInput.value;
  if (!gReplaceDialog.caseSensitive.checked)
  {
    selStr = selStr.toLowerCase();
    specStr = specStr.toLowerCase();
  }
  // Unfortunately, because of whitespace we can't just check
  // whether (selStr == specStr), but have to loop ourselves.
  // N chars of whitespace in specStr can match any M >= N in selStr.
  var matches = true;
  var specLen = specStr.length;
  var selLen = selStr.length;
  if (selLen < specLen)
    matches = false;
  else
  {
    specArray = specStr.match(/\S+|\s+/g);
    selArray = selStr.match(/\S+|\s+/g);
    if ( specArray.length != selArray.length)
      matches = false;
    else
    {
      for (i=0; i<selArray.length; i++)
      {
        if (selArray[i] != specArray[i])
        {
          if ( selArray[i][0].search(/\s/) == -1 ||
               specArray[i][0].search(/\s/) == -1)
          {
            // lowercase \s, not a space chunk -- match fails
            matches = false;
            break;
          }
          else if ( selArray[i].length < specArray[i].length )
          {
            // if it's a space chunk then we only care that sel be
            // at least as long as spec
            matches = false;
            break;
          }
        }
      }
    }
  }

  // If the current selection doesn't match the pattern,
  // then we want to find the next match, but not do the replace.
  // That's what most other apps seem to do.
  if (!matches)
    return onFindNext();

  // Transfer dialog contents to the find service.
  saveFindData();

  gEditor.insertText(gReplaceDialog.replaceInput.value);
  return true;
}

function onReplaceAll()
{
  if (!gEditor)
    return;

  var findStr = gReplaceDialog.findInput.value;
  var repStr = gReplaceDialog.replaceInput.value;

  // Transfer dialog contents to the find service.
  saveFindData();

  var finder = Components.classes["@mozilla.org/embedcomp/rangefind;1"].createInstance().QueryInterface(Components.interfaces.nsIFind);

  finder.caseSensitive    = gReplaceDialog.caseSensitive.checked;
  finder.findBackwards = gReplaceDialog.searchBackwards.checked;

  // Make a range containing the current selection, 
  // so we don't go past it when we wrap.
  var selection = gEditor.selection;
  var selecRange;
  if (selection.rangeCount > 0)
    selecRange = selection.getRangeAt(0);
  var origRange = selecRange.cloneRange();

  // We'll need a range for the whole document:
  var wholeDocRange = gEditor.document.createRange();
  var rootNode = gEditor.rootElement.QueryInterface(Components.interfaces.nsIDOMNode);
  wholeDocRange.selectNodeContents(rootNode);

  // And start and end points:
  var startPt = gEditor.document.createRange();
  startPt.setStart(wholeDocRange.startContainer, wholeDocRange.startOffset);
  startPt.setEnd(wholeDocRange.startContainer, wholeDocRange.startOffset);

  var endPt = gEditor.document.createRange();
  endPt.setStart(wholeDocRange.endContainer, wholeDocRange.endOffset);
  endPt.setEnd(wholeDocRange.endContainer, wholeDocRange.endOffset);

  // Find and replace from here to end of document:
  var foundRange;
  while ((foundRange = finder.Find(findStr, wholeDocRange,
                                   selecRange, endPt)) != null)
  {
    gEditor.selection.removeAllRanges();
    gEditor.selection.addRange(foundRange);
    gEditor.insertText(repStr);
    selection = gEditor.selection;
    if (selection.rangeCount <= 0) {
      return;
    }
    selecRange = selection.getRangeAt(0);
  }

  // If wrapping, find from start of document back to start point.
  selecRange.setStart(wholeDocRange.startContainer, wholeDocRange.startOffset);
  selecRange.setEnd(wholeDocRange.startContainer, wholeDocRange.startOffset);
  // Collapse origRange to start:
  origRange.setEnd(origRange.startContainer, origRange.startOffset);
  if (gReplaceDialog.wrap.checked)
  {
    while ((foundRange = finder.Find(findStr, wholeDocRange,
                                     selecRange, origRange)) != null)
    {
      gEditor.selection.removeAllRanges();
      gEditor.selection.addRange(foundRange);
      gEditor.insertText(repStr);
      selection = gEditor.selection;
      if (selection.rangeCount <= 0) {
        return;
      }
      selecRange = selection.getRangeAt(0);
    }
  }
}

function doEnabling()
{
  var findStr = gReplaceDialog.findInput.value;
  var repStr = gReplaceDialog.replaceInput.value;
  gReplaceDialog.enabled = findStr;
  gReplaceDialog.findNext.disabled = !findStr;
  gReplaceDialog.replace.disabled = (!findStr || !repStr);
  gReplaceDialog.replaceAndFind.disabled = (!findStr || !repStr);
  gReplaceDialog.replaceAll.disabled = (!findStr || !repStr);
}
