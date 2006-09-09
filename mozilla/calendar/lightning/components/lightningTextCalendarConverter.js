/* -*- Mode: javascript; tab-width: 20; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Lightning code.
 *
 * The Initial Developer of the Original Code is Oracle Corporation
 * Portions created by the Initial Developer are Copyright (C) 2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Mike Shaver <shaver@mozilla.org>
 *   Clint Talbert <cmtalbert@myfastmail.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or 
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

const Cc = Components.classes;
const Ci = Components.interfaces;

function makeTableRow(val) {
    return "<tr><td>" + val[0] + "</td><td>" + val[1] + "</td></tr>\n";
}

function getLightningStringBundle()
{
    var svc = Cc["@mozilla.org/intl/stringbundle;1"]
              .getService(Ci.nsIStringBundleService);
    return svc.createBundle("chrome://lightning/locale/lightning.properties");
}

function createHtmlTableSection(label, text)
{
    var tblRow = <tr>
                    <td class="description">
                        <p>{label}</p>
                    </td>
                    <td class="content">
                        <p>{text}</p>
                    </td>
                 </tr>;
    return tblRow;
}

function createHtml(event)
{
    // Creates HTML using the Node strings in the properties file
    var stringBundle = getLightningStringBundle();
    var html;
    if (stringBundle) {
        // Using e4x javascript support here
        html =
               <html>
               <head>
                    <meta http-equiv='Content-Type' content='text/html; charset=utf-8'/>
                    <link rel='stylesheet' type='text/css' href='chrome://lightning/skin/imip.css'/>
               </head>
               <body>
                    <table>
                    </table>
               </body>
               </html>;
        // Create header row
        var labelText = stringBundle.GetStringFromName("imipHtml.header");
        html.body.table.appendChild(
            <tr>
                <td colspan="3" class="header">
                    <p class="header">{labelText}</p>
                </td>
            </tr>
        );
        if (event.title) {
            labelText = stringBundle.GetStringFromName("imipHtml.summary");
            html.body.table.appendChild(createHtmlTableSection(labelText,
                                                               event.title));
        }

        var eventLocation = event.getProperty("LOCATION");
        if (eventLocation) {
            labelText = stringBundle.GetStringFromName("imipHtml.location");
            html.body.table.appendChild(createHtmlTableSection(labelText,
                                                               eventLocation));
        }

        var labelText = stringBundle.GetStringFromName("imipHtml.when");
        html.body.table.appendChild(createHtmlTableSection(labelText,
                                                           event.startDate.jsDate.toLocaleString()));

        if (event.organizer && 
           (event.organizer.commonName || event.organizer.id)) {
            labelText = stringBundle.GetStringFromName("imipHtml.organizer");
            var orgname = event.organizer.commonName || event.organizer.id;
            html.body.table.appendChild(createHtmlTableSection(labelText, orgname));
        }

        var eventDescription = event.getProperty("DESCRIPTION");
        if (eventDescription) {
            labelText = stringBundle.GetStringFromName("imipHtml.description");
            html.body.table.appendChild(createHtmlTableSection(labelText,
                                                               eventDescription));
        }
    }

    return html;
}

function ltnMimeConverter() { }

ltnMimeConverter.prototype = {
    QueryInterface: function (aIID) {
        if (!aIID.equals(Ci.nsISupports) &&
            !aIID.equals(Ci.nsISimpleMimeConverter))
            throw Ci.NS_ERROR_NO_INTERFACE;

        return this;
    },

    convertToHTML: function(contentType, data) {
        var event = Cc["@mozilla.org/calendar/event;1"]
                    .createInstance(Ci.calIEvent);
        event.icalString = data;
        var html = createHtml(event);

        try {
            // Bug 351610: This mechanism is a little flawed
            var itipItem = Cc["@mozilla.org/calendar/itip-item;1"]
                           .createInstance(Ci.calIItipItem);
            itipItem.init(data);
            var observer = Cc["@mozilla.org/observer-service;1"]
                           .getService(Ci.nsIObserverService);
            if (observer) {
                observer.notifyObservers(itipItem, "onItipItemCreation", 0);
            }
        } catch (e) {
            Components.utils.reportError("Cannot Create iTIP Item: " + e);
        }

        return html;
    }
};

var myModule = {
    registerSelf: function (compMgr, fileSpec, location, type) {
        debug("*** Registering Lightning text/calendar handler\n");
        compMgr = compMgr.QueryInterface(Ci.nsIComponentRegistrar);
        compMgr.registerFactoryLocation(this.myCID,
                                        "Lightning text/calendar handler",
                                        this.myContractID,
                                        fileSpec,
                                        location,
                                        type);

        var catman = Components.classes["@mozilla.org/categorymanager;1"]
            .getService(Ci.nsICategoryManager);

        catman.addCategoryEntry("simple-mime-converters", "text/calendar",
                                this.myContractID, true, true);
    },

    getClassObject: function (compMgr, cid, iid) {
        if (!cid.equals(this.myCID))
            throw Components.results.NS_ERROR_NO_INTERFACE;

        if (!iid.equals(Components.interfaces.nsIFactory))
            throw Components.results.NS_ERROR_NOT_IMPLEMENTED;

        return this.myFactory;
    },

    myCID: Components.ID("{c70acb08-464e-4e55-899d-b2c84c5409fa}"),

    myContractID: "@mozilla.org/lightning/mime-converter;1",

    myFactory: {
        createInstance: function (outer, iid) {
            if (outer != null)
                throw Components.results.NS_ERROR_NO_AGGREGATION;

            return (new ltnMimeConverter()).QueryInterface(iid);
        }
    },

    canUnload: function(compMgr) {
        return true;
    }
};

function NSGetModule(compMgr, fileSpec) {
    return myModule;
}

