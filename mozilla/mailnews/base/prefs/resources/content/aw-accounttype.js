/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * Alec Flett <alecf@netscape.com>
 */

function onInit() {
    fixLabels(document.getElementById("ispBox"));

    
}


// this is a workaround for a template shortcoming.
// basically: templates can't set the "id" attribute on a node, so we
// have to set "fakeid" and then transfer it manually
function fixLabels(box) {
    if (!box) return;
    var child = box.firstChild;

    while (child) {
        if (child.tagName.toLowerCase() == "div") {
            var input = child.childNodes[0];
            var label = child.childNodes[1];

            dump("Looking at " + input.tagName + " and " + label.tagName + "\n");
            if (input.tagName.toLowerCase() == "input" &&
                label.tagName.toLowerCase() == "label") {
                input.setAttribute("id", input.getAttribute("fakeid"));
            }
        }

        child = child.nextSibling;
    }

}

function onMailChanged(event) {
    //    enableControls(document.getElementById("ispBox"),
    //                   event.target.checked);
}

function enableControls(node, enabled)
{
    var tagName = node.tagName.toLowerCase();
    if (tagName == "input" || tagName == "label") {
        if (enabled)
            node.setAttribute("disabled", "true");
        else
            node.removeAttribute("disabled");
    }

    var child = node.firstChild();
    while (child) {
        enableIspButtons(child, enabled);
        child = child.nextSibling;
    }
}

function onUnload() {
    var pageData = parent.wizardManager.WSM.PageData;
    var wizardMap = parent.wizardMap;

    parent.updateMap(pageData, wizardMap);
    return true;
}
