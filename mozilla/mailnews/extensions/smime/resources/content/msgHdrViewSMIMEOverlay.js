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
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation. Portions created by Netscape are
 * Copyright (C) 1998-2001 Netscape Communications Corporation. All
 * Rights Reserved.
 * 
 * Contributors:
 *   Scott MacGreogr <mscott@netscape.com>
 */

var gSignedUINode = null;
var gEncryptedUINode = null;
var gSMIMEContainer = null;
var gStatusBar = null;

// manipulates some globals from msgReadSMIMEOverlay.js

const nsICMSMessageErrors = Components.interfaces.nsICMSMessageErrors;

var smimeHeaderSink = 
{ 
  maxWantedNesting: function()
  {
    return 1;
  },

  signedStatus: function(aNestingLevel, aSignatureStatus, aSignerCert)
  {
    if (aNestingLevel > 1) {
      // we are not interested
      return;
    }

    gSignatureStatus = aSignatureStatus;
    gSignerCert = aSignerCert;

    gSMIMEContainer.collapsed = false;
    gSignedUINode.collapsed = false;

    if (nsICMSMessageErrors.SUCCESS == aSignatureStatus)
    {
      gSignedUINode.setAttribute("signed", "ok");
      gStatusBar.setAttribute("signed", "ok");
    }
    else
    {
      gSignedUINode.setAttribute("signed", "notok");
      gStatusBar.setAttribute("signed", "notok");
    }
  },

  encryptionStatus: function(aNestingLevel, aEncryptionStatus, aRecipientCert)
  {
    if (aNestingLevel > 1) {
      // we are not interested
      return;
    }

    gEncryptionStatus = aEncryptionStatus;
    gEncryptionCert = aRecipientCert;

    gSMIMEContainer.collapsed = false; 
    gEncryptedUINode.collapsed = false;

    if (nsICMSMessageErrors.SUCCESS == aEncryptionStatus)
    {
      gEncryptedUINode.setAttribute("encrypted", "ok");
      gStatusBar.setAttribute("encrypted", "ok");
    }
    else
    {
      gEncryptedUINode.setAttribute("encrypted", "notok");
      gStatusBar.setAttribute("encrypted", "notok");
    }
  },

  QueryInterface : function(iid)
  {
    if (iid.equals(Components.interfaces.nsIMsgSMIMEHeaderSink) || iid.equals(Components.interfaces.nsISupports))
      return this;
    throw Components.results.NS_NOINTERFACE;
  }
};

function onSMIMEStartHeaders()
{
  gEncryptionStatus = -1;
  gSignatureStatus = -1;
  
  gSignerCert = null;
  gEncryptionCert = null;
  
  gSMIMEContainer.collapsed = true;
  gSignedUINode.collapsed = true;
  gSignedUINode.removeAttribute("signed");
  gStatusBar.removeAttribute("signed");
  gEncryptedUINode.collapsed = true;
  gEncryptedUINode.removeAttribute("encrypted");
  gStatusBar.removeAttribute("encrypted");
}

function onSMIMEEndHeaders()
{}

function msgHdrViewSMIMEOnLoad(event)
{
  // we want to register our security header sink as an opaque nsISupports
  // on the msgHdrSink used by mail.....
  msgWindow.msgHeaderSink.securityInfo = smimeHeaderSink;

  gSignedUINode = document.getElementById('signedHdrIcon');
  gEncryptedUINode = document.getElementById('encryptedHdrIcon');
  gSMIMEContainer = document.getElementById('smimeBox');
  gStatusBar = document.getElementById('status-bar');

  // add ourself to the list of message display listeners so we get notified when we are about to display a
  // message.
  var listener = {};
  listener.onStartHeaders = onSMIMEStartHeaders;
  listener.onEndHeaders = onSMIMEEndHeaders;
  gMessageListeners.push(listener);
}

addEventListener('messagepane-loaded', msgHdrViewSMIMEOnLoad, true);
