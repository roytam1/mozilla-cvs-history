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
 * The Initial Developer of the Original Code is Simdesk Technologies Inc.
 * Portions created by the Initial Developer are Copyright (C) 2006
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Clint Talbert <ctalbert.moz@gmail.com>
 *   Matthew Willis <lilmatt@mozilla.com>
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

/**
 * This bar lives inside the message window.
 * Its lifetime is the lifetime of the main thunderbird message window.
 */

var gItipItem;

const onItipItem = {
    observe: function observe(subject, topic, state) {
        if (topic == "onItipItemCreation") {
            checkForItipItem(subject);
        }
    }
};

function checkForItipItem(subject)
{
    var itipItem;
    try {
        if (!subject) {
            var msgUri = GetLoadedMessage();
            var sinkProps = msgWindow.msgHeaderSink.properties;
            // This property was set by LightningTextCalendarConverter.js
            itipItem = sinkProps.getPropertyAsInterface("itipItem",
                                                        Components.interfaces.calIItipItem);
        } else {
            // With Thunderbird 1.5.x we have to use the subject to pass the
            // iTIP item because we don't have sinkProps available.
            itipItem = subject.QueryInterface(Components.interfaces.calIItipItem);
        }
    } catch (e) {
        // This will throw on every message viewed that doesn't have the
        // itipItem property set on it. So we eat the errors and move on.

        // XXX TODO: Only swallow the errors we need to. Throw all others.
        return;
    }

    // We are only called upon receipt of an invite, so ensure that isSend
    // is false.
    itipItem.isSend = false;

    // XXX Get these from preferences
    itipItem.autoResponse = Components.interfaces.calIItipItem.USER;

    var imipMethod = getMsgImipMethod();
    if (imipMethod &&
        imipMethod.length != 0 &&
        imipMethod.toLowerCase() != "nomethod")
    {
        itipItem.receivedMethod = imipMethod;
    } else {
        // There is no METHOD in the content-type header (spec violation).
        // Fall back to using the one from the itipItem's ICS.
        imipMethod = itipItem.receivedMethod;
    }

    gItipItem = itipItem;

    // XXX Bug 351742: no S/MIME or spoofing protection yet
    // handleImipSecurity(imipMethod);

    setupBar(imipMethod);
}

addEventListener("messagepane-loaded", imipOnLoad, true);
addEventListener("messagepane-unloaded", imipOnUnload, true);

/**
 * Add self to gMessageListeners defined in msgHdrViewOverlay.js
 */
function imipOnLoad()
{
    var listener = {};
    listener.onStartHeaders = onImipStartHeaders;
    listener.onEndHeaders = onImipEndHeaders;
    gMessageListeners.push(listener);

    // Set up our observers
    var observerSvc = Components.classes["@mozilla.org/observer-service;1"]
                                .getService(Components.interfaces.nsIObserverService);
    observerSvc.addObserver(onItipItem, "onItipItemCreation", false);
}

function imipOnUnload()
{
    removeEventListener("messagepane-loaded", imipOnLoad, true);
    removeEventListener("messagepane-unloaded", imipOnUnload, true);

    var observerSvc = Components.classes["@mozilla.org/observer-service;1"]
                                .getService(Components.interfaces.nsIObserverService);
    observerSvc.removeObserver(onItipItem, "onItipItemCreation");

    gItipItem = null;
}

function onImipStartHeaders()
{
    var imipBar = document.getElementById("imip-bar");
    imipBar.setAttribute("collapsed", "true");
    document.getElementById("imip-button1").setAttribute("hidden", "true");
    document.getElementById("imip-button2").setAttribute("hidden", "true");

    // A new message is starting.
    // Clear our iMIP/iTIP stuff so it doesn't contain stale information.
    imipMethod = "";
    gItipItem = null;
}

/**
 * Required by MessageListener. no-op
 */
function onImipEndHeaders()
{
    // no-op
}

function setupBar(imipMethod)
{
    // XXX - Bug 348666 - Currently we only do REQUEST requests
    // In the future this function will set up the proper actions
    // and attributes for the buttons as based on the iMIP Method
    var imipBar = document.getElementById("imip-bar");
    imipBar.setAttribute("collapsed", "false");
    var description = document.getElementById("imip-description");

    // Bug 348666: here is where we would check if this event was already
    // added to calendar or not and display correct information

    if (imipMethod.toUpperCase() == "REQUEST") {
        if (description.firstChild.data) {
            description.firstChild.data = ltnGetString("lightning",
                                                       "imipBarRequestText");
        }

        var button = document.getElementById("imip-button1");
        button.removeAttribute("hidden");
        button.setAttribute("label", ltnGetString("lightning",
                                                  "imipAcceptInvitation.label"));
        button.setAttribute("oncommand",
                            "setAttendeeResponse('ACCEPTED', 'CONFIRMED');");

        // Create a DECLINE button
        button = document.getElementById("imip-button2");
        button.removeAttribute("hidden");
        button.setAttribute("label", ltnGetString("lightning",
                                                  "imipDeclineInvitation.label"));
        button.setAttribute("oncommand",
                            "setAttendeeResponse('DECLINED', 'CONFIRMED');");
    } else if (imipMethod.toUpperCase() == "REPLY") {
        // Bug xxxx we currently cannot process REPLY messages so just let
        // the user know what this is, and don't give them any options.
        if (description.firstChild.data) {
            description.firstChild.data = ltnGetString("lightning",
                                                       "imipBarReplyText");
        }
    } else if (imipMethod.toUpperCase() == "PUBLISH") {
        if (description.firstChild.data) {
            description.firstChild.data = ltnGetString("lightning",
                                                       "imipBarRequestText");
        }

        var button = document.getElementById("imip-button1");
        button.removeAttribute("hidden");
        button.setAttribute("label", ltnGetString("lightning",
                                                  "imipAddToCalendar.label"));
        
        button.setAttribute("oncommand",
                            "setAttendeeResponse('PUBLISH', '');");
    } else {
        // Bug xxxx TBD: Something went wrong or we found a message we don't
        // support yet. We can show a "This method is not supported in this
        // version" or simply hide the iMIP bar at this point
        if (description.firstChild.data) {
            description.firstChild.data = ltnGetString("lightning",
                                                       "imipBarUnsupportedText");
        }
        Components.utils.reportError("Unknown imipMethod: " + imipMethod);
    }
}

function getMsgImipMethod()
{
    var imipMethod = "";
    var msgURI = GetLoadedMessage();
    var msgHdr = messenger.messageServiceFromURI(msgURI)
                          .messageURIToMsgHdr(msgURI);
    imipMethod = msgHdr.getStringProperty("imip_method");
    return imipMethod;
}

function getMsgRecipient()
{
    var imipRecipient = "";
    var msgURI = GetLoadedMessage();
    var msgHdr = messenger.messageServiceFromURI(msgURI)
                          .messageURIToMsgHdr(msgURI);

    // msgHdr recipients can be a comma separated list of recipients.
    // We then compare against the defaultIdentity to find ourselves.
    // XXX This won't always work:
    //     Users with multiple accounts and invites going to the non-default
    //     account, Users with email aliases where the defaultIdentity.email
    //     doesn't match the recipient, etc.
    if (msgHdr) {
        var recipientList = msgHdr.recipients;
        // Remove any spaces
        recipientList = recipientList.split(" ").join("");
        recipientList = recipientList.split(",");

        var emailSvc = Components.classes["@mozilla.org/calendar/itip-transport;1?type=email"]
                                 .getService(Components.interfaces.calIItipTransport);
        var me = emailSvc.defaultIdentity;

        var lt;
        var gt;
        for each (var recipient in recipientList) {
            // Deal with <foo@bar.com> style addresses
            lt = recipient.indexOf("<");
            gt = recipient.indexOf(">");

            // I chose 6 since <a@b.c> is the shortest technically valid email
            // address I could come up with.
            if ((lt >= 0) && (gt >= 6)) {
                recipient = recipient.substring(lt+1, gt);
            }

            if (recipient.toLowerCase() == me.toLowerCase()) {
                imipRecipient = recipient;
            }
        }
    }
    return imipRecipient;
}

/**
 * Call the calendar picker
 */
function getTargetCalendar()
{
    var calendarToReturn;
    var calMgr = Components.classes["@mozilla.org/calendar/manager;1"]
                           .getService(Components.interfaces.calICalendarManager);
    var count = new Object();
    var calArray = calMgr.getCalendars(count);

    if (count.value == 1) {
        // There's only one calendar, so it's silly to ask what calendar
        // the user wants to import into.
        calendarToReturn = calArray[0];
    } else {
        // Ask what calendar to import into
        var args = new Object();
        var aCal;
        args.onOk = function selectCalendar(aCal) { calendarToReturn = aCal; };
        args.promptText = calGetString("calendar", "importPrompt");
        openDialog("chrome://calendar/content/chooseCalendarDialog.xul",
                   "_blank", "chrome,titlebar,modal,resizable", args);
    }
    return calendarToReturn;
}

/**
 * Type is type of response
 * event_status is an optional directive to set the Event STATUS property
 */
function setAttendeeResponse(type, eventStatus)
{
    var myAddress = getMsgRecipient();
    if (type && gItipItem) {
        // We set the attendee status appropriately
        switch (type) {
            case "ACCEPTED":
            case "TENTATIVE":
                gItipItem.setAttendeeStatus(myAddress, type);
                // fall through
            case "REPLY":
            case "PUBLISH":
                var targetCalendar = getTargetCalendar();
                gItipItem.targetCalendar = targetCalendar;
                doResponse(eventStatus);
                break;
            case "DECLINED":
                gItipItem.setAttendeeStatus(myAddress, type);
                doResponse(eventStatus);
                break;
            default:
                // no-op. The attendee wishes to disregard the mail, so no
                // further action is required.
                break;
        }
    }
}

/**
 * doResponse performs the iTIP action for the current ItipItem that we
 * parsed from the email.
 * @param  aLocalStatus  optional parameter to set the event STATUS property.
 *         aLocalStatus can be empty, "TENTATIVE", "CONFIRMED", or "CANCELLED"
 */
function doResponse(aLocalStatus)
{
    // calIOperationListener so that we can properly return status to the
    // imip-bar
    var operationListener = {
        onOperationComplete:
        function ooc(aCalendar, aStatus, aOperationType, aId, aDetail) {
            // Call finishItipAction to set the status of the operation
            finishItipAction(aOperationType, aStatus, aDetail);
        },

        onGetResult:
        function ogr(aCalendar, aStatus, aItemType, aDetail, aCount, aItems) {
            // no-op
        }
    };

    // The spec is unclear if we must add all the items or if the
    // user should get to pick which item gets added.

    if (aLocalStatus != null) {
        gItipItem.localStatus = aLocalStatus;
    }

    var itipProc = Components.classes["@mozilla.org/calendar/itip-processor;1"]
                             .createInstance(Components.interfaces.calIItipProcessor);

    itipProc.processItipItem(gItipItem, operationListener);
}

/**
 * Bug 348666 (complete iTIP support) - This gives the user an indication
 * that the Action occurred.
 *
 * In the future, this will store the status of the invitation in the
 * invitation manager.  This will enable us to provide the ability to request
 * updates from the organizer and to suggest changes to invitations.
 *
 * Currently, this is called from our calIOperationListener that is sent to
 * the ItipProcessor. This conveys the status of the local iTIP processing
 * on your calendar. It does not convey the success or failure of sending a
 * response to the ItipItem.
 */
function finishItipAction(aOperationType, aStatus, aDetail)
{
    // For now, we just state the status for the user something very simple
    var desc = document.getElementById("imip-description");
    if (desc.firstChild != null) {
        if (Components.isSuccessCode(aStatus)) {
            desc.firstChild.data = ltnGetString("lightning",
                                                "imipAddedItemToCal");
            document.getElementById("imip-button1").setAttribute("hidden",
                                                                 true);
            document.getElementById("imip-button2").setAttribute("hidden",
                                                                 true);
        } else {
            // Bug 348666: When we handle more iTIP methods, we need to create
            // more sophisticated error handling.
            document.getElementById("imip-bar").setAttribute("collapsed", true);
            var msg = "Invitation could not be processed. Status: " + aStatus;
            if (aDetail) {
                msg += "\nDetails: " + aDetail;
            }
            // Defined in import-export
            showError(msg);
        }
    }
}
