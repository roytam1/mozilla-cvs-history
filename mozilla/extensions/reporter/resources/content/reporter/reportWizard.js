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
 * The Original Code is Mozilla Reporter (r.m.o).
 *
 * The Initial Developer of the Original Code is
 *      Robert Accettura <robert@accettura.com>.
 *
 * Portions created by the Initial Developer are Copyright (C) 2004
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *    Boris Zbarsky <bzbarsky@mit.edu>
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

/********************************************************
 *                *** Warning ****
 *   DO _NOT_ MODIFY THIS FILE without first contacting
 *   Robert Accettura <robert@accettura.com>
 *   or a reporter.mozilla.org Admin!
 *******************************************************/

const RMOURI = "http://reporter-test.mozilla.org/service/";

const gURL = window.arguments[0];
const gPlatform = navigator.platform;
const gUserAgent = navigator.userAgent;
const goscpu = navigator.oscpu;
const geckoStri = "00000000"; //XXX Not used at the moment, 8 0's to ignore
const gLanguage = window.navigator.language;
const gRMOvers = "0.2"; // Do not touch without contacting reporter admin!

// Globals
var gReportID;
var gSysID;
var gFaultCode;
var gFaultMessage;
var gSOAPerror = false;

function product(){
  // only works on > 1.7.5.  Sorry SeaMonkey of old
  if ('nsIChromeRegistrySea' in Components.interfaces) {
    return 'SeaMonkey/'+
    Components.classes['@mozilla.org/network/io-service;1']
              .getService(Components.interfaces.nsIIOService)
              .getProtocolHandler('http')
              .QueryInterface(Components.interfaces.nsIHttpProtocolHandler).misc.substring(3);
  }
  else {
    return navigator.vendor+'/'+navigator.vendorSub;
  }
}


function initPrivacyNotice(){
  // If they agreed, we continue on
  var prefs = Components.classes["@mozilla.org/preferences-service;1"]
                        .getService(Components.interfaces.nsIPrefService)
                     	.getBranch("extensions.reporter.");
  try {
    if (prefs.getBoolPref("hidePrivacyStatement")){
      document.getElementById('reportWizard').advance();
      return;
    }
  } catch (e) {}

  // Don't let users rewind, and default to checked.
  document.getElementById('reportWizard').canRewind= false;
  document.getElementById("dontShowPrivacyStatement").setAttribute("checked", "true");
  document.getElementById('mustagree').collapsed = true;  // only shown if checkbox is unchecked.

  // Load Privacy Policy
  var strbundle=document.getElementById("strings");
  document.getElementById("privacyStatement").setAttribute("src", strbundle.getString("privacyStatementURL"));
}

function privacyPolicyCheckbox(){
  var showDetail = makeIntBool(document.getElementById('dontShowPrivacyStatement').checked);
  if (showDetail){
    // hide message and enable forward button
    document.getElementById('reportWizard').canAdvance= true;
    document.getElementById('mustagree').collapsed = true;
  } else {
    // show message, and disable forward button
    document.getElementById('reportWizard').canAdvance= false;
    document.getElementById('mustagree').collapsed = false;
  }
}

function setPrivacyPref(){
  var dontShowPrivacyStatement = makeIntBool(document.getElementById('dontShowPrivacyStatement').checked);

  var prefs = Components.classes["@mozilla.org/preferences-service;1"]
                        .getService(Components.interfaces.nsIPrefService)
                   	.getBranch("extensions.reporter.");

  if (dontShowPrivacyStatement){
    prefs.setBoolPref("hidePrivacyStatement", true);
  }
}

function initRegistration(){
  // if the user already registered, they can skip this page, otherwise they need to do so
  var prefs = Components.classes["@mozilla.org/preferences-service;1"].
                           getService(Components.interfaces.nsIPrefService).
                     	   getBranch("extensions.reporter.");
  try
  {
    if (prefs.getPrefType("sysid") == prefs.PREF_STRING)
    {
      var sysid = prefs.getCharPref("sysid");
      if (sysid.length == 10)
      {
      	document.getElementById('reportWizard').advance();
        return;
      }
    }
  }
  catch (e) {}

  // XXX sigh... we should let the user go back, but yea, see what initPrivacyNotice() does to see why that's a loopy idea ;-)
  document.getElementById('reportWizard').canRewind= false;
  document.getElementById('reportWizard').canAdvance= false;
}

function register(){
  var param = new Array();;
  param[0] = new SOAPParameter(gLanguage,"language");

  var strbundle=document.getElementById("strings");
  var submitResult = document.getElementById('registrationProgressDescription');
  var meter = document.getElementById('registrationProgressIndicator');

  // get sysID
  meter.setAttribute("value","1%");
  submitResult.setAttribute("value",strbundle.getString("registerSendingRequest")); // Sending Request
  callReporter("register",param,setValSysID);
  meter.setAttribute("value","90%");

  // saving
  submitResult.setAttribute("value",strbundle.getString("registerSavingID")); // Saving ID
  var prefs = Components.classes["@mozilla.org/preferences-service;1"]
                        .getService(Components.interfaces.nsIPrefService)
                     	.getBranch("extensions.reporter.");

  if (gSysID != undefined){
    prefs.setCharPref("sysid", gSysID);
  } else {
    submitResult.setAttribute("value",strbundle.getString("registerSavingFailed")); // Failed
    return;
  }

  // success
  meter.setAttribute("value","100%");
  submitResult.setAttribute("value",strbundle.getString("registerSuccess")); // Success
  document.getElementById('registerButton').disabled = true;

  // user can now go forward
  document.getElementById('reportWizard').canAdvance= true;
}

function initForm(){
  document.getElementById('reportWizard').canRewind = false;

  document.getElementById('url').value = gURL;

  // We don't let the user go forward until they fufill certain requirements - see validateform()
  document.getElementById('reportWizard').canAdvance= false;
}

function validateForm() {
  if(document.getElementById('problem_type').value != "0_0")
    document.getElementById('reportWizard').canAdvance= true;
  else
    document.getElementById('reportWizard').canAdvance= false;
}

function sendReport(){
  // we control the user path from here.
  document.getElementById('reportWizard').canRewind = false;
  document.getElementById('reportWizard').canAdvance = false;
  // why would we need a cancel button?
  document.getElementById('reportWizard').getButton("cancel").disabled = true;

  var strbundle=document.getElementById("strings");
  var statusDescription = document.getElementById('sendReportProgressDescription');
  var statusIndicator = document.getElementById('sendReportProgressIndicator');

  // Data from form we need
  var descriptionStri = document.getElementById('description').value;
  var problemTypeStri = document.getElementById('problem_type').value;
  var behindLoginStri = makeIntBool(document.getElementById('behind_login').checked);
  var emailStri = document.getElementById('email').value;

  var buildConfig = getBuildConfig();

  // SysID
  var prefs = Components.classes["@mozilla.org/preferences-service;1"]
                        .getService(Components.interfaces.nsIPrefService)
                     	.getBranch("extensions.reporter.");
  try
  {
    if (prefs.getPrefType("sysid") == prefs.PREF_STRING)
    {
      var sysid = prefs.getCharPref("sysid");
    }
  }
  catch (e) {}
  // SOAP params
  var param = new Array();
  param[0] = new SOAPParameter(gRMOvers,"rmoVers");
  param[1] = new SOAPParameter(gURL,"url");
  param[2] = new SOAPParameter(problemTypeStri,"problem_type");
  param[3] = new SOAPParameter(descriptionStri,"description");
  param[4] = new SOAPParameter(behindLoginStri,"behind_login");
  param[5] = new SOAPParameter(gPlatform,"platform");
  param[6] = new SOAPParameter(goscpu,"oscpu");
  param[7] = new SOAPParameter(geckoStri,"gecko");
  param[8] = new SOAPParameter(product(),"product");
  param[9] = new SOAPParameter(gUserAgent,"useragent");
  param[10] = new SOAPParameter(buildConfig, "buildconfig");
  param[11] = new SOAPParameter(gLanguage,"language");
  param[12] = new SOAPParameter(emailStri,"email");
  param[13] = new SOAPParameter(sysid,"sysid");

  statusIndicator.setAttribute("value","5%");
  statusDescription.setAttribute("value",strbundle.getString("sendingReport"));
  callReporter("submitReport",param,setValReportID);

  var finishSummary = document.getElementById('finishSummary');

  if (!gSOAPerror)
  {
    // If successful
    var finishExtended = document.getElementById('finishExtendedSuccess');
    document.getElementById('finishExtendedFailed').setAttribute("class", "hide");

    statusIndicator.setAttribute("value","95%");
    statusDescription.setAttribute("value",strbundle.getString("reportSent"));

    document.getElementById('reportWizard').canAdvance= true;
    statusIndicator.setAttribute("value","100%");

    // Send to the finish page
    document.getElementById('reportWizard').advance();

    // report ID returned from the web service
    finishSummary.setAttribute("value",strbundle.getString("successfullyCreatedReport") + " " + gReportID);

    finishExtendedDoc = finishExtended.contentDocument;
    finishExtendedDoc.getElementById('urlStri').textContent = gURL;
    finishExtendedDoc.getElementById('problemTypeStri').textContent = problemTypeStri;
    finishExtendedDoc.getElementById('descriptionStri').textContent = descriptionStri;
    finishExtendedDoc.getElementById('platformStri').textContent = gPlatform;
    finishExtendedDoc.getElementById('oscpuStri').textContent = goscpu;
    finishExtendedDoc.getElementById('productStri').textContent = product();
    finishExtendedDoc.getElementById('geckoStri').textContent = geckoStri;
    finishExtendedDoc.getElementById('buildConfigStri').textContent = buildConfig;
    finishExtendedDoc.getElementById('userAgentStri').textContent = gUserAgent;
    finishExtendedDoc.getElementById('langStri').textContent = gLanguage;
    finishExtendedDoc.getElementById('emailStri').textContent = emailStri;

    document.getElementById('reportWizard').canRewind= false;
  } else {
    // If there was an error from the server

    var finishExtended = document.getElementById('finishExtendedFailed');
    document.getElementById('finishExtendedSuccess').setAttribute("class", "hide");

    document.getElementById('reportWizard').canAdvance= true;
    document.getElementById('reportWizard').advance();

    finishSummary.setAttribute("value",strbundle.getString("failedCreatingReport"));

    finishExtendedDoc = finishExtended.contentDocument;

    finishExtendedDoc.getElementById('faultCode').textContent = gFaultCode;
    finishExtendedDoc.getElementById('faultMessage').textContent = gFaultMessage;
  }
  document.getElementById('finishExtendedFrame').collapsed = true;
  document.getElementById('reportWizard').canRewind = false;
  document.getElementById('reportWizard').getButton("cancel").disabled = true;
}

function showdetail(){
  var showDetail = makeIntBool(document.getElementById('showDetail').checked);
  if (showDetail){
    document.getElementById('finishExtendedFrame').collapsed = false;
  } else {
    document.getElementById('finishExtendedFrame').collapsed = true;
  }
}

function getBuildConfig() {
  /* Commented out until it works
  var ioservice =
     Components.classes["@mozilla.org/network/io-service;1"].
       getService(Components.interfaces.nsIIOService);
  var channel = ioservice.newChannel("chrome://global/content/buildconfig.html",
                                     null, null);
  var stream = channel.open();
  var parser = new DOMParser();

  var buildconfig = parser.parseFromStream(stream, "UTF-8", stream.available(),
                                   "application/xhtml+xml");

  return buildconfig.documentElement.firstChild.textContent;
  */
  return 'buildconfig';
}

function makeIntBool(boolStr){
  if (boolStr)
    return 1;
  else
    return 0;
}

/*  NEW WEB SERVICE MODULE */
/*  Based on Apple's example implementation of SOAP at: developer.apple.com/internet/webservices/mozgoogle_source.html */
function callReporter(method,params,callback){
  try {
    netscape.security.PrivilegeManager.enablePrivilege("UniversalBrowserRead");
  } catch (e) {
    alert(e);
    return false;
  }
  var soapCall = new SOAPCall();
  soapCall.transportURI = RMOURI;
  soapCall.encode(0, method, "urn:MozillaReporter", 0, null, params.length, params);
  var response = soapCall.invoke();
  var error = handleSOAPResponse(response);
  if (!error)
  {
    callback(response);
  }
}

function handleSOAPResponse (response)
{
   var fault = response.fault;
   if (fault != null) {
       gSOAPerror = true;
       gFaultCode = fault.faultCode;
       gFaultMessage = fault.faultString;
        return true;
    } else
    {
        return false;
    }
}

function setValSysID(results)
{
  if (!results)
  {
    return;
  }

  var params = results.getParameters(false,{});
  for (var i = 0; i < params.length; i++){
    gSysID = params[i].value;
  }
}

function setValReportID(results)
{
  if (!results)
  {
    return;
  }

  var params = results.getParameters(false,{});
  for (var i = 0; i < params.length; i++){
    gReportID = params[i].value;
  }
}
