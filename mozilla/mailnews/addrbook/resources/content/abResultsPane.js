/* -*- Mode: Java; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 *
 * Original Author:
 *   Seth Spitzer <sspitzer@netscape.com>
 */

function AbResultsPaneOnClick(event)
{
    // we only care about button 0 (left click) events
    if (event.button != 0) return;

    // all we need to worry about here is double clicks
    // and column header clicks.
    //
    // we get in here for clicks on the "outlinercol" (headers)
    // and the "scrollbarbutton" (scrollbar buttons)
    // we don't want those events to cause a "double click"

    var t = event.originalTarget;

    if (t.localName == "outlinercol") {
      var sortDirection = kDefaultDescending;

      if (gAbView) {
        var node = document.getElementById(gAbView.URI);
        sortDirection = node.getAttribute("sortDirection");
        if (sortDirection == kDefaultDescending)
          sortDirection = kDefaultAscending;
        else 
          sortDirection = kDefaultDescending;
      }
      SortAndUpdateIndicators(t.id, sortDirection);
    }
    else if (t.localName == "outlinerbody") {
       var row = new Object;
       var colID = new Object;
       var childElt = new Object;

       var outliner = GetAbResultsOutliner();
       // figure out what cell the click was in
       outliner.boxObject.QueryInterface(Components.interfaces.nsIOutlinerBoxObject).getCellAt(event.clientX, event.clientY, row, colID, childElt);

       if (event.detail == 2) {
         AbResultsPaneDoubleClick(gAbView.getCardFromRow(row.value));
       }
       else {
         OnClickedCard(gAbView.getCardFromRow(row.value));
       }
    }
}
