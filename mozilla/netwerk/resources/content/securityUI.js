/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
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

window.addEventListener("load", SetSecurityButton, false);
window.addEventListener("unload", DestroySecurity, false);

var securityUI;

function SetSecurityButton()
{
    const ui = Components.classes["@mozilla.org/secure_browser_ui;1"];
    if (ui) {
        var securityUI = ui.createInstance(Components.interfaces.nsSecureBrowserUI);

        if ("gBrowser" in window) { // XXXjag see bug 68662
            gBrowser.boxObject.setPropertyAsSupports("xulwindow", window);
            gBrowser.boxObject.setPropertyAsSupports("secureBrowserUI", securityUI);
        }

        var button = document.getElementById("security-button");
        if (button && _content)
            securityUI.init(_content, button);
    }
}

function displayPageInfo()
{
   window.openDialog("chrome://navigator/content/pageInfo.xul", "_blank",
                     "dialog=no", null, "securityTab");
}

function DestroySecurity()
{
    if ("gBrowser" in window) { // XXXjag see bug 68662
        gBrowser.boxObject.removeProperty("xulwindow");
        gBrowser.boxObject.removeProperty("secureBrowserUI");
    }
}
