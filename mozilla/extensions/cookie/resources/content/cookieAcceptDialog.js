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
 * The Original Code is cookie manager code.
 *
 * The Initial Developer of the Original Code is
 * Michiel van Leeuwen.
 * Portions created by the Initial Developer are Copyright (C) 2002
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
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

const nsICookieAcceptDialog = Components.interfaces.nsICookieAcceptDialog;
const nsIDialogParamBlock = Components.interfaces.nsIDialogParamBlock;
const nsICookie = Components.interfaces.nsICookie;

var params; 
var cookieBundle;
var gDateService = null;

var showDetails = "";
var showDetailsAccessKey = "";
var hideDetails = "";
var hideDetailsAccessKey = "";

function onload()
{
  doSetOKCancel(cookieAccept, cookieDeny);

  var dialog = document.documentElement;

  document.getElementById("ok").label = dialog.getAttribute("acceptLabel");
  document.getElementById("cancel").label = dialog.getAttribute("cancelLabel");

  if (!gDateService) {
    const nsScriptableDateFormat_CONTRACTID = "@mozilla.org/intl/scriptabledateformat;1";
    const nsIScriptableDateFormat = Components.interfaces.nsIScriptableDateFormat;
    gDateService = Components.classes[nsScriptableDateFormat_CONTRACTID]
                             .getService(nsIScriptableDateFormat);
  }

  cookieBundle = document.getElementById("cookieBundle");

  //cache strings
  if (!showDetails) {
    showDetails = cookieBundle.getString('showDetails');
    showDetailsAccessKey = cookieBundle.getString('showDetailsAccessKey');
  }
  if (!hideDetails) {
    hideDetails = cookieBundle.getString('hideDetails');
    hideDetailsAccessKey = cookieBundle.getString('hideDetailsAccessKey');
  }

  if (document.getElementById('infobox').hidden) {
    document.getElementById('disclosureButton').setAttribute("label",showDetails);
    document.getElementById('disclosureButton').setAttribute("accesskey",showDetailsAccessKey);
  } else {
    document.getElementById('disclosureButton').setAttribute("label",hideDetails);
    document.getElementById('disclosureButton').setAttribute("accesskey",hideDetailsAccessKey);
  }

  if ("arguments" in window && window.arguments.length >= 1 && window.arguments[0]) {
    try {
      params = window.arguments[0].QueryInterface(nsIDialogParamBlock);
      var objects = params.objects;
      var cookie = params.objects.queryElementAt(0,nsICookie);
      
      var cookiesFromHost = params.GetInt(nsICookieAcceptDialog.COOKIESFROMHOST);

      var messageFormat;
      if (params.GetInt(nsICookieAcceptDialog.CHANGINGCOOKIE))
        messageFormat = 'permissionToModifyCookie';
      else if (cookiesFromHost > 1)
        messageFormat = 'permissionToSetAnotherCookie';
      else if (cookiesFromHost == 1)
        messageFormat = 'permissionToSetSecondCookie';
      else
        messageFormat = 'permissionToSetACookie';

      var hostname = params.GetString(nsICookieAcceptDialog.HOSTNAME);

      var messageText;
      if (cookie)
        messageText = cookieBundle.getFormattedString(messageFormat,[hostname, cookiesFromHost]);
      else
        // No cookies means something went wrong. Bring up the dialog anyway
        // to not make the mess worse.
        messageText = cookieBundle.getFormattedString(messageFormat,["",cookiesFromHost]);

      var messageParent = document.getElementById("dialogtextbox");
      var messageParagraphs = messageText.split("\n");

      // use value for the header, so it doesn't wrap.
      var headerNode = document.getElementById("dialog-header");
      headerNode.setAttribute("value",messageParagraphs[0]);

      // use childnodes here, the text can wrap
      for (var i = 1; i < messageParagraphs.length; i++) {
        var descriptionNode = document.createElement("description");
        text = document.createTextNode(messageParagraphs[i]);
        descriptionNode.appendChild(text);
        messageParent.appendChild(descriptionNode);
      }

      if (cookie) {
        document.getElementById('ifl_name').setAttribute("value",cookie.name);
        document.getElementById('ifl_value').setAttribute("value",cookie.value);
        document.getElementById('ifl_host').setAttribute("value",cookie.host);
        document.getElementById('ifl_path').setAttribute("value",cookie.path);
        document.getElementById('ifl_isSecure').setAttribute("value",
                                                                 cookie.isSecure ?
                                                                    cookieBundle.getString("yes") : cookieBundle.getString("no")
                                                          );
        document.getElementById('ifl_expires').setAttribute("value",GetExpiresString(cookie.expires));
        document.getElementById('ifl_isDomain').setAttribute("value",
                                                                 cookie.isDomain ?
                                                                    cookieBundle.getString("domainColon") : cookieBundle.getString("hostColon")
                                                            );
      }
      // set default result to not accept the cookie
      params.SetInt(nsICookieAcceptDialog.ACCEPT_COOKIE, 0);
      // and to not persist
      params.SetInt(nsICookieAcceptDialog.REMEMBER_DECISION, 0);
    } catch (e) {
    }
  }
}

function showhideinfo()
{
  var infobox=document.getElementById('infobox');

  if (infobox.hidden) {
    infobox.setAttribute("hidden","false");
    document.getElementById('disclosureButton').setAttribute("label",hideDetails);
    document.getElementById('disclosureButton').setAttribute("accesskey",hideDetailsAccessKey);
  } else {
    infobox.setAttribute("hidden","true");
    document.getElementById('disclosureButton').setAttribute("label",showDetails);
    document.getElementById('disclosureButton').setAttribute("accesskey",showDetailsAccessKey);
  }
  sizeToContent();
}

function cookieAccept()
{
  // say that the cookie was accepted
  params.SetInt(nsICookieAcceptDialog.ACCEPT_COOKIE, 1); 
  // And remember that when needed
  params.SetInt(nsICookieAcceptDialog.REMEMBER_DECISION, document.getElementById('persistDomainAcceptance').checked);
  window.close();
}

function cookieDeny()
{
  // say that the cookie was rejected
  params.SetInt(nsICookieAcceptDialog.ACCEPT_COOKIE, 0); 
  // And remember that when needed
  params.SetInt(nsICookieAcceptDialog.REMEMBER_DECISION, document.getElementById('persistDomainAcceptance').checked);
  window.close();
}

function GetExpiresString(secondsUntilExpires) {
  if (secondsUntilExpires) {
    var expireDate = new Date(1000*secondsUntilExpires);
    return gDateService.FormatDateTime("", gDateService.dateFormatLong,
                                       gDateService.timeFormatSeconds, 
                                       expireDate.getFullYear(),
                                       expireDate.getMonth()+1, 
                                       expireDate.getDate(), 
                                       expireDate.getHours(),
                                       expireDate.getMinutes(), 
                                       expireDate.getSeconds());
  }
  return cookieBundle.getString("atEndOfSession");
}

