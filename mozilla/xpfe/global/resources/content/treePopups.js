/* -*- Mode: Java; tab-width: 4; c-basic-offset: 4; -*-
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
 *
 * Contributor(s):
 *   Dave Hyatt (hyatt@netscape.com)
 *   Peter Annema <disttsc@bart.nl>
 *   Blake Ross <blakeross@telocity.com>
 */

function BuildTreePopup( treeColGroup, treeHeadRow, popup, skipCell )
{
  var popupChild = popup.firstChild;
  var firstTime = !popupChild ? true : false;

  var currTreeCol = treeHeadRow.firstChild;
  var currColNode = treeColGroup.firstChild;
  var count = 0;
  while (currTreeCol) {
    if (currColNode.localName == "splitter")
      currColNode = currColNode.nextSibling;

    if (skipCell != currTreeCol) {
      // Construct an entry for each cell in the row.
      var columnName = currTreeCol.getAttribute("label");
      if (firstTime) {
        if (currTreeCol.getAttribute("collapsed") != "true") {
          popupChild = document.createElement("menuitem");
          popupChild.setAttribute("type", "checkbox");
          popupChild.setAttribute("label", columnName);
          if (!count++) popupChild.setAttribute("disabled", "true");
          if (columnName == "") {
            var display = currTreeCol.getAttribute("display");
            popupChild.setAttribute("label", display);
          }
          popupChild.setAttribute("colid", currColNode.id);
          popupChild.setAttribute("oncommand", "ToggleColumnState(this, document)");
          if ("true" != currColNode.getAttribute("hidden")) {
            popupChild.setAttribute("checked", "true");
          }
          popup.appendChild(popupChild);
        }
      } else {
        if ("true" == currColNode.getAttribute("hidden")) {
          if (popupChild.getAttribute("checked"))
            popupChild.removeAttribute("checked");
        } else {
          if (!popupChild.getAttribute("checked"))
            popupChild.setAttribute("checked", "true");
        }
        if (currColNode.getAttribute("collapsed") == "true")
          popupChild.setAttribute("hidden", "true");
        else
          popupChild.removeAttribute("hidden");

        popupChild = popupChild.nextSibling;
      }
    }

    currTreeCol = currTreeCol.nextSibling;
    currColNode = currColNode.nextSibling;
  }
}

function DestroyPopup(element)
{
/*
  while (element.firstChild) {
    element.removeChild(element.firstChild);
  }
  */
}

function ToggleColumnState(popupElement, doc)
{
  var colid = popupElement.getAttribute("colid");
  var colNode = doc.getElementById(colid);
  if (colNode) {
    var checkedState = popupElement.getAttribute("checked");
    if (checkedState == "true")
      colNode.removeAttribute("hidden");
    else
      colNode.setAttribute("hidden", "true");
  }
}
