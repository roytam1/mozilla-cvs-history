/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- 
 *
 * ***** BEGIN LICENSE BLOCK *****
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2002
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Peter van der Beken <peterv@netscape.com>
 *   Dan Mosedale <dmose@netscape.com>
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

const nsIMsgDBHdr       = Components.interfaces.nsIMsgDBHdr;

var gJunkmailComponent;
function getJunkmailComponent()
{
    if (!gJunkmailComponent) {
        gJunkmailComponent = Components.classes['@mozilla.org/messenger/filter-plugin;1?name=junkmail']
                .getService(Components.interfaces.nsISupports).wrappedJSObject;
        gJunkmailComponent.initComponent();
    }
}

function analyze(aMessage, aNextFunction)
{
    function callback(aScore) {
    }

    callback.prototype = 
        {
            onMessageScored: function processNext(aScore)
            {
                if (aMessage) {
                    //if (aScore == -1) debugger;
                    if (aScore == 0) {
                        aMessage.setStringProperty("score", "0");
                    }
                    else if (aScore == 1) {
                        aMessage.setStringProperty("score", "100");
                    }
                }
                aNextFunction();
            }
        };


    // Pffft, jumping through hoops here.
    var messageURI = aMessage.folder.generateMessageURI(aMessage.messageKey) + "?fetchCompleteMessage=true";
    var messageURL = mailSession.ConvertMsgURIToMsgURL(messageURI, msgWindow);
    gJunkmailComponent.calculate(messageURL, new callback);
}

function analyzeFolder()
{
    function processNext()
    {
        if (messages.hasMoreElements()) {
            // Pffft, jumping through hoops here.
            var message = messages.getNext().QueryInterface(nsIMsgDBHdr);
            while (!message.isRead) {
                if (!messages.hasMoreElements()) {
                    gJunkmailComponent.batchUpdate = false;
                    return;
                }
                message = messages.getNext().QueryInterface(nsIMsgDBHdr);
            }
            analyze(message, processNext);
        }
        else {
            gJunkmailComponent.batchUpdate = false;
        }
    }

    getJunkmailComponent();
    var folder = GetFirstSelectedMsgFolder();
    var messages = folder.getMessages(msgWindow);
    gJunkmailComponent.batchUpdate = true;
    processNext();
}

function analyzeMessages()
{
    function processNext()
    {
        if (counter < messages.length) {
            // Pffft, jumping through hoops here.
            var messageUri = messages[counter];
            var message = messenger.messageServiceFromURI(messageUri).messageURIToMsgHdr(messageUri);

            ++counter;
            while (!message.isRead) {
                if (counter == messages.length) {
                    gJunkmailComponent.mBatchUpdate = false;
                    return;
                }
                messageUri = messages[counter];
                message = messenger.messageServiceFromURI(messageUri).messageURIToMsgHdr(messageUri);
                ++counter;
            }
            analyze(message, processNext);
        }
        else {
            gJunkmailComponent.batchUpdate = false;
        }
    }

    getJunkmailComponent();
    var messages = GetSelectedMessages();
    var counter = 0;
    gJunkmailComponent.batchUpdate = true;
    processNext();
}

function markMessage(aMessage, aSpam, aNextFunction)
{
    // Pffft, jumping through hoops here.
    var messageURI = aMessage.folder.generateMessageURI(aMessage.messageKey) + "?fetchCompleteMessage=true";
    var messageURL = mailSession.ConvertMsgURIToMsgURL(messageURI, msgWindow);
    var action;
    if (aSpam) {
        action = kUnknownToSpam;
        if (aMessage.getStringProperty("score") == "100") {
            // Marking a spam message as spam does nothing
            aNextFunction();
            return;
        }
        if (aMessage.getStringProperty("score") == "0") {
            // Marking a non-spam message as spam
            action = kNoSpamToSpam;
        }
        aMessage.setStringProperty("score","100");
    }
    else {
        action = kUnknownToNoSpam;
        if (aMessage.getStringProperty("score") == "0") {
            // Marking a non-spam message as non-spam does nothing
            aNextFunction();
            return;
        }
        if (aMessage.getStringProperty("score") == "100") {
            // Marking a spam message as non-spam
            action = kSpamToNoSpam;
        }
        aMessage.setStringProperty("score", "0");
    }
    gJunkmailComponent.mark(messageURL, action, aNextFunction);
}

function markFolder(aSpam)
{
    function processNext()
    {
        if (messages.hasMoreElements()) {
            // Pffft, jumping through hoops here.
            var message = messages.getNext().QueryInterface(nsIMsgDBHdr);
            markMessage(message, aSpam, processNext);
        }
        else {
            gJunkmailComponent.batchUpdate = false;
        }
    }

    getJunkmailComponent();
    var folder = GetFirstSelectedMsgFolder();
    var messages = folder.getMessages(msgWindow);
    gJunkmailComponent.batchUpdate = true;
    processNext();
}

function markMessages(aSpam)
{
    function processNext()
    {
        if (counter < messages.length) {
            // Pffft, jumping through hoops here.
            var messageUri = messages[counter];
            var message = messenger.messageServiceFromURI(messageUri).messageURIToMsgHdr(messageUri);

            ++counter;
            markMessage(message, aSpam, processNext);
        }
        else {
            gJunkmailComponent.batchUpdate = false;
        }
    }

    getJunkmailComponent();
    var messages = GetSelectedMessages();
    var counter = 0;
    gJunkmailComponent.batchUpdate = true;
    processNext();
}

function writeHash()
{
    getJunkmailComponent();
    gJunkmailComponent.mTable.writeHash();
}
