/* -*- Mode: Java; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

// the actual filter that we're editing
var gFilter;

// cache the key elements we need
var gFilterNameElement;
var gActionElement;
var gActionTargetElement;
var gActionPriority;
var Bundle;

var nsIMsgSearchValidityManager = Components.interfaces.nsIMsgSearchValidityManager;

var nsMsgFilterAction = Components.interfaces.nsMsgFilterAction;

function filterEditorOnLoad()
{
    initializeSearchWidgets();
    initializeFilterWidgets();

    Bundle = srGetStrBundle("chrome://messenger/locale/filter.properties");
    
    if (window.arguments && window.arguments[0]) {
        var args = window.arguments[0];
        if (args.filter) {
            gFilter = window.arguments[0].filter;

            initializeDialog(gFilter);
        } else {
            gFilterList = args.filterList;
            if (gFilterList)
                setSearchScope(getScopeFromFilterList(gFilterList));
            // fake the first more button press
            onMore(null);
        }
    }

    gFilterNameElement.focus();
    doSetOKCancel(onOk, null);
}

function onOk()
{
    if (!saveFilter()) return;

    // parent should refresh filter list..
    // this should REALLY only happen when some criteria changes that
    // are displayed in the filter dialog, like the filter name
    window.arguments[0].refresh = true;
    window.close();
}

function getScopeFromFilterList(filterList)
{
    if (!filterList) return;
    var type = filterList.folder.server.type;
    if (type == "nntp") return nsIMsgSearchValidityManager.news;
    return nsIMsgSearchValidityManager.onlineMail;
}

function getScope(filter) {
    return getScopeFromFilterList(filter.filterList);
}

function initializeFilterWidgets()
{
    gFilterNameElement = document.getElementById("filterName");
    gActionElement = document.getElementById("actionMenu");
    gActionTargetElement = document.getElementById("actionTargetFolder");
    gActionValueDeck = document.getElementById("actionValueDeck");
    gActionPriority = document.getElementById("actionValuePriority");
}

function initializeDialog(filter)
{
    gFilterNameElement.value = filter.filterName;

    gActionElement.selectedItem=gActionElement.getElementsByAttribute("data", filter.action)[0];
    showActionElementFor(gActionElement.selectedItem);

    if (filter.action == nsMsgFilterAction.MoveToFolder) {
        // there are multiple sub-items that have given attribute
        var targets = gActionTargetElement.getElementsByAttribute("data", filter.actionTargetFolderUri);

        if (targets && targets.length > 0) {
            var target = targets[0];
            if (target.localName == "menuitem")
                gActionTargetElement.selectedItem = target;
        }
    } else if (filter.action == nsMsgFilterAction.ChangePriority) {
        dump("initializing priority..\n");
        var selectedPriority = gActionPriority.getElementsByAttribute("data", filter.actionPriority);

        if (selectedPriority && selectedPriority.length > 0) {
            var selectedPriority = selectedPriority[0];
            gActionPriority.selectedItem = selectedPriority;
        }
    }
        

    var scope = getScope(filter);
    setSearchScope(scope);
    initializeSearchRows(scope, filter.searchTerms)
}


// move to overlay

function saveFilter() {

    var isNewFilter;

    var filterName= gFilterNameElement.value;
    if (!filterName || filterName == "") {
        var str = Bundle.GetStringFromName("mustEnterName");
        window.alert(str);
        gFilterNameElement.focus();
        return false;
    }

    if (!gFilter) {
        gFilter = gFilterList.createFilter(gFilterNameElement.value);
        isNewFilter = true;
    } else {
        gFilter.filterName = gFilterNameElement.value;
        isNewFilter = false;
    }

    saveSearchTerms(gFilter.searchTerms, gFilter);

    var action = gActionElement.selectedItem.getAttribute("data");
    dump("Action = " + action + "\n");
    gFilter.action = action;
    if (action == nsMsgFilterAction.MoveToFolder) {
        var targetUri;
        if (gActionTargetElement)
            targetUri = gActionTargetElement.selectedItem.getAttribute("data");
        dump("folder target = " + gActionTargetElement.selectedItem + "\n");
        if (!targetUri || targetUri == "") {
            var str = Bundle.GetStringFromName("mustSelectFolder");
            window.alert(str);
            gFilter = null;
            return false;
        }
        gFilter.actionTargetFolderUri = targetUri;
    }
    
    else if (action == nsMsgFilterAction.ChangePriority) {
        if (!gActionPriority.selectedItem) {
            var str = Bundle.GetStringFromName("mustSelectPriority");
            window.alert(str);
            gFilter = null;
            return false;
        }
        gFilter.actionPriority = gActionPriority.selectedItem.getAttribute("data");
    }

    
    if (isNewFilter) {
        dump("new filter.. inserting into " + gFilterList + "\n");
        gFilterList.insertFilterAt(0, gFilter);
    }

    // success!
    return true;
}

function onActionChanged(event)
{
    var menuitem = event.target;
    showActionElementFor(menuitem);
}

function showActionElementFor(menuitem)
{
    if (!menuitem) return;
    dump("showActionElementFor(" + menuitem.getAttribute("actionvalueindex") + ")\n");
    gActionValueDeck.setAttribute("index", menuitem.getAttribute("actionvalueindex"));
}
