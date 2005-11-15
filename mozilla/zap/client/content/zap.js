// -*- Mode: javascript; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2; moz-jssh-buffer-globalobj: "getWindows()[0]" -*-
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
 * The Original Code is the Mozilla SIP client project.
 *
 * The Initial Developer of the Original Code is 8x8 Inc.
 * Portions created by the Initial Developer are Copyright (C) 2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Alex Fritze <alex@croczilla.com> (original author)
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

////////////////////////////////////////////////////////////////////////
// imports

Components.utils.importModule("rel:ClassUtils.js");
Components.utils.importModule("rel:RDFUtils.js");
Components.utils.importModule("rel:FileUtils.js");
Components.utils.importModule("rel:StringUtils.js");

////////////////////////////////////////////////////////////////////////
// globals

var wUserAgent = "zap/0.1.3"; // String to be sent in User-Agent
                              // header for SIP requests
var wNetUtils = Components.classes["@mozilla.org/zap/netutils;1"].getService(Components.interfaces.zapINetUtils);
var wPromptService = Components.classes["@mozilla.org/embedcomp/prompt-service;1"].getService(Components.interfaces.nsIPromptService);
var wRDF = Components.classes["@mozilla.org/rdf/rdf-service;1"].getService(Components.interfaces.nsIRDFService);
var wRDFContainerUtils = Components.classes["@mozilla.org/rdf/container-utils;1"].createInstance(Components.interfaces.nsIRDFContainerUtils);

var wInteractPane;
var wURLField;

var wSidebarTree;
var wSidebarDS;

var wConfig;
var wConfigDS = wRDF.GetDataSourceBlocking(getProfileFileURL("config.rdf"));
var wServicesContainer;

var wIdentitiesDS = wRDF.GetDataSourceBlocking(getProfileFileURL("identities.rdf"));
var wIdentitiesContainer;
var wCurrentIdentity;

var wContactsDS = wRDF.GetDataSourceBlocking(getProfileFileURL("contacts.rdf"));
// all contacts live in the urn:mozilla:zap:contacts sequence:
var wContactsContainer;
// ... those displayed in the sidebar are also pointed to from the
// urn:mozilla:zap:friends sequence:
var wFriendsContainer;

var wCallsTree;
var wCallsDS = wRDF.GetDataSourceBlocking(getProfileFileURL("calls.rdf"));
var wCallsContainer;
var wRecentCallsContainer;

// an application-global ds for storing emphemeral state, e.g. whether
// a registration is active or not:
var wGlobalEphemeralDS = Components.classes["@mozilla.org/rdf/datasource;1?name=in-memory-datasource"].createInstance(Components.interfaces.nsIRDFDataSource);

var wSipStack;
var wSdpService;

var wErrorLog;

// helper to emit a warning to the current error sink:
function warning(message) {
  ErrorReporterSink.reporterFunction(message);
}

////////////////////////////////////////////////////////////////////////
// Initialization:

function windowInit() {
  dump("Initializing zap main window...\n");

  // set up error logging:
  // XXX this should be configurable (enable/disable)
  wErrorLog = openFileForWriting(getProfileFile("zap.log"), true);
  ErrorReporterSink.reporterFunction = function (mes) {
    if (wErrorLog)
      wErrorLog.write(mes,mes.length);
    else
      dump(mes);
  };
  
  wInteractPane = document.getElementById("interactpane");
  wURLField = document.getElementById("url_field");

  initConfig();
  initContacts();
  initCalls();
  initSidebar();
  wMediaPipeline.init();
  initSipStack();
  initIdentities();
  
  dump("... Done initializing zap main window\n");
}

function windowClose() {
  cleanupSipStack();
  wMediaPipeline.cleanup();
  
  // close error log:
  wErrorLog.close();
  wErrorLog = null;
}

////////////////////////////////////////////////////////////////////////
// UI Commands:

function cmdAbout() {
  window.openDialog("chrome://zap/content/about.xul",
                    "about",
                    "chrome,modal,alwaysRaised",
                    null);
}

function cmdExit() {
  goQuitApplication();
}

function cmdGo() {
  loadPage("chrome://zap/content/make-call.xul", true);
}

function cmdGenericRequest() {
  loadPage("chrome://zap/content/generic-request.xul", false);
}

function cmdRegisterRequest() {
  loadPage("chrome://zap/content/register-request.xul", false);
}
  
////////////////////////////////////////////////////////////////////////
// Interaction pane:

function loadPage(url, force)
{
  clearSidebarSelection();
  if (wInteractPane.getAttribute("src") != url)
    wInteractPane.setAttribute("src", url);
  else if (force)
    reloadPage();
}

function reloadPage()
{
  wInteractPane.webNavigation.reload(0);
}

////////////////////////////////////////////////////////////////////////
// Config:

function initConfig() {
  wServicesContainer = Components.classes["@mozilla.org/rdf/container;1"].
    createInstance(Components.interfaces.nsIRDFContainer);
  wServicesContainer.Init(wConfigDS,
                          wRDF.GetResource("urn:mozilla:zap:services"));
  
  wConfig = Config.instantiate();
  wConfig.initWithResource(wRDF.GetResource("urn:mozilla:zap:config"));
}

var Config = makeClass("Config", PersistentRDFObject);
Config.prototype.datasources["default"] = wConfigDS;
Config.rdfResourceAttrib("urn:mozilla:zap:identity",
                         "urn:mozilla:zap:initial_identity");
Config.rdfLiteralAttrib("urn:mozilla:zap:instance_id", "");
Config.rdfLiteralAttrib("urn:mozilla:zap:max_recent_calls", "10");
Config.rdfLiteralAttrib("urn:mozilla:zap:sip_port_base", "5060");
Config.rdfLiteralAttrib("urn:mozilla:zap:default_registration_interval", "300");
// time constants for flow failure recovery (draft-ietf-sip-outbound-01.txt 4.3):
Config.rdfLiteralAttrib("urn:mozilla:zap:registration_recovery_max_time", "1800");
Config.rdfLiteralAttrib("urn:mozilla:zap:registration_recovery_base_time_all_fail", "30");
Config.rdfLiteralAttrib("urn:mozilla:zap:registration_recovery_base_time_not_failed", "60");

Config.rdfLiteralAttrib("urn:mozilla:zap:dnd_code", "480"); // Temporarily unavail.
Config.rdfLiteralAttrib("urn:mozilla:zap:dnd_headers", ""); // additional headers for DND response

var Service = makeClass("Service", PersistentRDFObject);
Service.prototype.datasources["default"] = wConfigDS;
Service.rdfLiteralAttrib("http://home.netscape.com/NC-rdf#Name", "");
Service.rdfLiteralAttrib("urn:mozilla:zap:domain", "");
Service.rdfLiteralAttrib("urn:mozilla:zap:route1", "");
Service.rdfLiteralAttrib("urn:mozilla:zap:route2", "");
Service.rdfLiteralAttrib("urn:mozilla:zap:route3", "");
Service.rdfLiteralAttrib("urn:mozilla:zap:route4", "");
Service.spec(
  function createFromDocument(doc) {
    this._Service_createFromDocument(doc);
    // append to services container:
    wServicesContainer.AppendElement(this.resource);
  });

////////////////////////////////////////////////////////////////////////
// Identities:

function initIdentities() {
  wIdentitiesContainer = Components.classes["@mozilla.org/rdf/container;1"].
    createInstance(Components.interfaces.nsIRDFContainer);
  wIdentitiesContainer.Init(wIdentitiesDS,
                           wRDF.GetResource("urn:mozilla:zap:identities"));

  // set the current identity:
  var identitiesList = document.getElementById("identities_list_popup");
  identitiesList.database.AddDataSource(wIdentitiesDS);
  identitiesList.builder.rebuild();
  // select correct identity in identities list:
  document.getElementById("identities_list").selectedItem = document.getElementById(wConfig["urn:mozilla:zap:identity"]);
  // make sure out stack is configured for this identity profile:
  wCurrentIdentity = Identity.instantiate();
  wCurrentIdentity.initWithResource(wRDF.GetResource(wConfig["urn:mozilla:zap:identity"]));
  currentIdentityUpdated();

  // register identities:
  var identity_resources = wIdentitiesContainer.GetElements();
  while (identity_resources.hasMoreElements()) {
    registerIdentity(identity_resources.getNext().QueryInterface(Components.interfaces.nsIRDFResource));
  }
}

// called when the user selects a different identity in the identities_list:
function identityChange() {
  var l = document.getElementById("identities_list").selectedItem.getAttribute("id");
  if (wConfig["urn:mozilla:zap:identity"] == l)
    return;
  else {
    wConfig["urn:mozilla:zap:identity"] = l;
    wConfig.flush();
  }
  wCurrentIdentity = Identity.instantiate();
  wCurrentIdentity.initWithResource(wRDF.GetResource(wConfig["urn:mozilla:zap:identity"]));
  currentIdentityUpdated();
}

// we call this function every time the current identity profile is
// updated or a new one is selected so that we can reconfigure the
// stack from here. Alternatively we could listen in on the identity
// and config datasources, which we should probably do at some point.
function currentIdentityUpdated() {
//   // XXX these try/catch blocks will be redundant once we hook up
//   // syntax checking to PersistentRDFObject
//   try {
//     wSipStack.FromAddress = wSipStack.syntaxFactory.deserializeAddress(wCurrentIdentity["urn:mozilla:zap:from_address"]);
//   }
//   catch(e) {
//     alert("The From-Address in the currently selected identity has invalid syntax");
//   }
//   try {
//     wCurrentIdentity.routeset = wSipStack.syntaxFactory.deserializeRouteSet(wCurrentIdentity["urn:mozilla:zap:route_set"], {});  }
//   catch(e) {
//     alert("The Route Set in the currently selected identity has invalid syntax");
//   }

//   currentAccountsUpdated();
}

//----------------------------------------------------------------------
// Identity class

var Identity = makeClass("Identity", PersistentRDFObject, SupportsImpl);
Identity.addInterfaces(Components.interfaces.zapISipCredentialsProvider);

Identity.prototype.datasources["default"] = wIdentitiesDS;
Identity.prototype.datasources["global-ephemeral"] = wGlobalEphemeralDS;

Identity.rdfResourceAttrib("urn:mozilla:zap:sidebarparent",
                           "urn:mozilla:zap:identities");
Identity.rdfLiteralAttrib("urn:mozilla:zap:chromepage",
                          "chrome://zap/content/identity.xul");
Identity.rdfLiteralAttrib("http://home.netscape.com/NC-rdf#Name", "<sip:thisis@anonymous.invalid>");
Identity.rdfLiteralAttrib("urn:mozilla:zap:nodetype", "identity");
Identity.rdfLiteralAttrib("urn:mozilla:zap:display_name", "");
Identity.rdfLiteralAttrib("urn:mozilla:zap:organization", "");
Identity.rdfLiteralAttrib("urn:mozilla:zap:preference", "0.1");
Identity.rdfLiteralAttrib("urn:mozilla:zap:authentication_username", "");
Identity.rdfResourceAttrib("urn:mozilla:zap:service",
                           "urn:mozilla:zap:automatic_service");
Identity.rdfLiteralAttrib("urn:mozilla:zap:is_registered",
                          "false", "global-ephemeral");


Identity.spec(
  function createFromDocument(doc) {
    this._Identity_createFromDocument(doc);
    // append to identities:
    wIdentitiesContainer.AppendElement(this.resource);
  });

// zapISipCredentialsProvider methods:
Identity.fun(
  function getCredentialsForRealm(realm, username, password) {
    // XXX fill in password
    var rv = wPromptService.promptUsernameAndPassword(null, "Enter credentials",
                                                      "Enter credentials for realm "+
                                                      realm+":",
                                                      username,
                                                      password,
                                                      null,
                                                      {});
    return rv;
  });


// Return host part of the AOR (or empty string if the AOR can't be
// parsed)
Identity.fun(
  function getHost() {
    // XXX these try/catch blocks will be redundant once we hook up
    // syntax checking in PersistentRDFObject
    try {
      var aor = wSipStack.syntaxFactory.deserializeAddress(this["http://home.netscape.com/NC-rdf#Name"]);
      var host = aor.uri.QueryInterface(Components.interfaces.zapISipSIPURI).host;
    }
    catch(e) {
      this._dump("Can't determine domain from "+this["http://home.netscape.com/NC-rdf#Name"]);
      return "";
    }
    return host;
  });

// Work out which Service this Identity uses and return its ID.
Identity.fun(
  function getServiceId() {
    var service_id = this["urn:mozilla:zap:service"];
    if (service_id != "urn:mozilla:zap:automatic_service")
      return service_id;
    //else...
    // we need to try to figure out the service from the AOR:
    // walk through known Services:
    var services = wServicesContainer.GetElements();
    var resourceDomain = wRDF.GetResource("urn:mozilla:zap:domain");
    var resourceTheDomain = wRDF.GetLiteral(this.getHost());
    while (services.hasMoreElements()) {
      var service = services.getNext().QueryInterface(Components.interfaces.nsIRDFResource);
      if (wConfigDS.HasAssertion(service,
                                 resourceDomain,
                                 resourceTheDomain, true)) {
        return service.Value;
      }
    }
    // nope, we don't know the service; use the default service:
    return "urn:mozilla:zap:default_service";
  });

////////////////////////////////////////////////////////////////////////
// Registration:

// hash of registration groups indexed by identity resource:

// XXX We currently use one registration group per identity. This is a
// bit wasteful if we have several identities with the same
// AOR. Investigate whether this is common enough to care.
var wRegistrations = {};

// set up or refresh a registration group for the given identity resource
function registerIdentity(resource) {
  var identity = Identity.instantiate();
  identity.initWithResource(resource);
  warning("registering "+identity["http://home.netscape.com/NC-rdf#Name"]+"\n");
  var registrationGroup = wRegistrations[resource.Value];
  if (registrationGroup) {
    // We already have a registration group for this identity. We
    // are probably getting called because the user has modified the
    // identity or the service associated with the identity.
    // -> Clear old registration group before creating a new one
    warning("unregistering "+identity["http://home.netscape.com/NC-rdf#Name"]+"\n");
    registrationGroup.unregister();
  }
  // create a new registration group :
  registrationGroup = RegistrationGroup.instantiate();
  if (registrationGroup.init(identity))
    wRegistrations[resource.Value] = registrationGroup;
}


function unregisterIdentity(resource) {
  var registrationGroup = wRegistrations[resource.Value];
  if (!registrationGroup) return; // not registered
  
  warning("unregistering "+registrationGroup.identity["http://home.netscape.com/NC-rdf#Name"]+"\n");
  registrationGroup.unregister();
  delete wRegistrations[resource.Value];
}

// called whenever the user edits a service
function notifyServiceUpdated(service_resource) {
  var service_id = service_resource.Value;
  // reregister any identities that use this service:
  for (var r in wRegistrations) {
    if (wRegistrations[r].service.resource.Value == service_id)
      registerIdentity(wRDF.GetResource(r));
  }
}

//----------------------------------------------------------------------

var RegistrationGroup = makeClass("RegistrationGroup", ErrorReporter);

// true if we are currently unregistering:
RegistrationGroup.obj("unregistering", false);

RegistrationGroup.fun(
  function init(identity) {
    this.identity = identity;
    this.service = Service.instantiate();
    this.service.initWithResource(wRDF.GetResource(this.identity.getServiceId()));
    
    this.domain = this.service["urn:mozilla:zap:domain"];
    
    if (!this.domain) {
      // the service isn't bound to a particular domain (probably
      // because it is the 'default' service.
      // -> use the identity's host:
      this.domain = this.identity.getHost();
    }
  
    if (this.domain == "anonymous.invalid") {
      // don't attempt to register our 'anonymous' identity
      return false;
    }
      
    this.domain = "sip:"+this.domain;
    
    // parse domain and aor into SIP syntax objects:
    try {
      this.domain = wSipStack.syntaxFactory.deserializeURI(this.domain);
    } catch(e) { warning("Domain parse error: "+this.domain+"\n"); return false; }
    try {
      this.aor = wSipStack.syntaxFactory.deserializeAddress(this.identity["http://home.netscape.com/NC-rdf#Name"]);
    } catch(e) { warning("AOR parse error: "+this.identity["http://home.netscape.com/NC-rdf#Name"]); return false; }

    this.interval = wConfig["urn:mozilla:zap:default_registration_interval"];

    // add a registration for each flow:
    this.registrations = [];
    var flow_counter = 0;
    for (var i=1; i<=4; ++i) {
      var route = this.service["urn:mozilla:zap:route"+i];
      if (!route) continue;
      try {
        route = wSipStack.syntaxFactory.deserializeRouteSet(route, {});
      } catch(e) { warning("Invalid route set: "+route+"\n"); continue; }
      var registration = Registration.instantiate();
      registration.init(this,
                        route,
                        ++flow_counter);
      this.registrations.push(registration);
    }
    if (!flow_counter) {
      // The service didn't specify any route.
      // -> Try domain:
      try {
        var route = wSipStack.syntaxFactory.deserializeRouteSet("<"+this.domain.serialize()+">", {});
      } catch(e) { this._warning("Invalid route set: <"+this.domain.serialize()+">"); return false; }
      var registration = Registration.instantiate();
      registration.init(this,
                        route,
                        1);
      this.registrations.push(registration);
    }

    return true;
  });

RegistrationGroup.fun(
  function unregister() {
    // make sure that we don't act upon pending notifications:
    this.unregistering = true;
    // mark our identity as un-registered:
    this.identity["urn:mozilla:zap:is_registered"] = "false";
    // walk through registrations and unregister:
    this.registrations.forEach(function(r) {
                                 r.unregister();
                               });
  });

RegistrationGroup.fun(
  function notifyRegistrationSuccess(registration) {
    if (this.unregistering) return;
    // mark our identity as registered:
    this._dump("Registration to "+this.identity["http://home.netscape.com/NC-rdf#Name"]+" over flow "+registration.flowid+" succeeded");
    this.identity["urn:mozilla:zap:is_registered"] = "true";
  });

RegistrationGroup.fun(
  function notifyRegistrationFailure(registration) {
    if (this.unregistering) return;
    this._dump("Registration to "+this.identity["http://home.netscape.com/NC-rdf#Name"]+" over flow "+registration.flowid+" has failed");
    this.recoverFromFailure(registration);
  });

RegistrationGroup.fun(
  function notifyFlowFailure(registration) {
    if (this.unregistering) return;
    this._dump("Flow "+registration.flowid+" to "+this.identity["http://home.netscape.com/NC-rdf#Name"]+" has failed");
    this.recoverFromFailure(registration);
  });

RegistrationGroup.fun(
  function recoverFromFailure(registration) {
    // recover as described in draft-ietf-outbound-01.txt 4.3:
    var some_active = this.registrations.some(function(r) {
                                                return r.registered;
                                              });
    var base_time;
    if (some_active) {
      base_time = wConfig["urn:mozilla:zap:registration_recovery_base_time_not_failed"];
    }
    else {
      // mark our identity as un-registered:
      this.identity["urn:mozilla:zap:is_registered"] = "false";
    
      base_time = wConfig["urn:mozilla:zap:registration_recovery_base_time_all_fail"];
    }
    base_time = parseFloat(base_time);

    var max_time = parseFloat(wConfig["urn:mozilla:zap:registration_recovery_max_time"]);

    var wait_time_ms = Math.min(max_time, base_time*Math.pow(2, registration.failureCount))*1000*(0.5+0.5*Math.random());
    this._dump("reregistering "+this.identity["http://home.netscape.com/NC-rdf#Name"]+", flow "+registration.flowid+" in "+wait_time_ms+"ms");
    registration.scheduleRefresh(wait_time_ms);
  });

//----------------------------------------------------------------------

var Registration = makeClass("Registration", SupportsImpl);
Registration.addInterfaces(Components.interfaces.zapISipNonInviteRCListener,
                           Components.interfaces.zapISipFlowMonitor,
                           Components.interfaces.nsITimerCallback);

Registration.obj("registered", false);
Registration.obj("failureCount", 0);

Registration.fun(
  function init(group, route, flowid) {
    this._dump("group="+group+", route="+route+", flowid="+flowid);
    this.group = group;
    this.flowid = flowid;
    // should we keep this flow alive with STUN requests?
    this.stunKeepAlive = route[0].uri.QueryInterface(Components.interfaces.zapISipSIPURI).hasURIParameter("sip-stun");
    
    this.registrationRC =
      wSipStack.createRegisterRequestClient(group.domain,
                                            group.aor,
                                            group.interval,
                                            route,
                                            route.length);
    // XXX if (use_outbound)
    var contact = this.registrationRC.request.getTopContactHeader().QueryInterface(Components.interfaces.zapISipContactHeader);
    contact.setParameter("+sip.instance", '"<'+wSipStack.instanceID+'>"');
    contact.setParameter("flow-id", flowid);

    this.registrationRC.listener = this;
    this.registrationRC.sendRequest();
  });

Registration.fun(
  function unregister() {
    if (this.refreshTimer) {
      this.refreshTimer.cancel();
      delete this.refreshTimer;
    }
    if (this.flow) {
      this.flow.removeFlowMonitor(this);
      delete this.flow;
    }
    
    if (this.registrationRC.listener) {
      // We are currently waiting for a registration to complete.
      // XXX what we want to do is cancel the RC.
      this.registrationRC.listener = null;
    }
    else if (this.registered) {
      // XXX maybe we shouldn't send unregistrations.
      var contact = this.registrationRC.request.getTopContactHeader();
      contact.setParameter("expires", "0");
      this.registrationRC.sendRequest();
    }
    delete this.registrationRC;
  });

// zapISipNonInviteRCListener methods:
Registration.fun(
  function notifyResponseReceived(rc, dialog, response, flow) {
    if (response.statusCode[0] == "1")
      return; // just a provisional response
    rc.listener = null;
    if ((response.statusCode == "401" ||
         response.statusCode == "407") &&
        wSipStack.authentication.addAuthorizationHeaders(this.group.identity,
                                                         response,
                                                         rc.request)) {
      // retry with credentials:
      rc.listener = this;
      rc.sendRequest();
      return;
    }
    else if (response.statusCode[0] == "2") {
      // find our registration among the returned contact headers:
      var contacts = response.getHeaders("Contact", {});
      var contact;
      var myURI = rc.request.getTopContactHeader().QueryInterface(Components.interfaces.zapISipContactHeader).address.uri.QueryInterface(Components.interfaces.zapISipSIPURI);
      for (var i=0,l=contacts.length; i<l; ++i) {
        var c = contacts[i].QueryInterface(Components.interfaces.zapISipContactHeader);
        try {
          var uri = c.address.uri.QueryInterface(Components.interfaces.zapISipSIPURI);
          if (uri.equals(myURI)) {
            // we've found a match
            contact = c;
            break;
          }
          else {
            this._dump(myURI.serialize()+" != "+uri.serialize());
          }
        }
        catch(e) {
          // there was an error parsing the uri in the contact header. ignore.
          this._dump("exception during registration response parsing: "+e);
        }
      }
      if (contact) {
        // looks like we're registered ok. let's see when the
        // registration expires (RFC3261 10.2.4):
        var expires = contact.getParameter("expires");
        if (!expires) {
          var expiresHeader = response.getTopHeader("Expires");
          if (expiresHeader) {
            expires = expiresHeader.QueryInterface(Components.interfaces.zapISipExpiresHeader).deltaSeconds;
          }
        }
        else {
          // the 'expires' uri parameter is a string (in contrast to a
          // deltaSeconds value which is already an integer). parse it:
          expires = parseInt(expires);
        }
        
        if (expires != null && !isNaN(expires) && expires != 0) {
          // we've got a valid registration!
          // update our state:
          if (!this.registered) {
            this.failureCount = 0;
            this.registered = true;
            // inform our group:
            this.group.notifyRegistrationSuccess(this);
          }
          // set up a refresh timer:
          this.scheduleRefresh(expires*1000*0.9);
          if (this.stunKeepAlive && !this.flow) {
            this.flow = flow;
            this.flow.addFlowMonitor(this, 1);
          }
          return;
        }
      }
    }
    // failure:
    this.registered = false;
    ++this.failureCount;
    if (this.flow) {
      this.flow.removeFlowMonitor(this);
      delete this.flow;
    }
    this.group.notifyRegistrationFailure(this);
  });

Registration.fun(
  function scheduleRefresh(delay_ms) {
    if (this.refreshTimer) {
      this.refreshTimer.cancel();
    }
    else {
      this.refreshTimer =
        Components.classes["@mozilla.org/timer;1"].createInstance(Components.interfaces.nsITimer);
    }
    this.refreshTimer.initWithCallback(this, delay_ms,
                                       Components.interfaces.nsITimer.TYPE_ONE_SHOT);
  });

// nsITimerCallback methods:
Registration.fun(
  function notify(timer) {
    this.registrationRC.listener = this;
    try {
      this.registrationRC.sendRequest();
    } catch(e) {
      // the rc might still be busy with the last request.
      this._warning("racing registrations");
    }
  });

// zapISipFlowMonitor methods:

// void flowChanged(in zapISipFlow flow, in unsigned short changeFlags);
Registration.fun(
  function flowChanged(flow, changeFlags) {
    // stop monitoring the flow; recover as described in
    // draft-ietf-sip-outbound-01.txt
    flow.removeFlowMonitor(this);
    delete this.flow;
    this.registered = false;
    // we leave recovery up to our group:
    ++this.failureCount;
    this.group.notifyFlowFailure(this);
  });

////////////////////////////////////////////////////////////////////////
// Contacts:

function initContacts() {
  wContactsContainer = Components.classes["@mozilla.org/rdf/container;1"].
    createInstance(Components.interfaces.nsIRDFContainer);
  wContactsContainer.Init(wContactsDS,
                          wRDF.GetResource("urn:mozilla:zap:contacts"));
  wFriendsContainer = Components.classes["@mozilla.org/rdf/container;1"].
    createInstance(Components.interfaces.nsIRDFContainer);
  wFriendsContainer.Init(wContactsDS,
                         wRDF.GetResource("urn:mozilla:zap:friends"));
}

var Contact = makeClass("Contact", PersistentRDFObject);

Contact.prototype.datasources["default"] = wContactsDS;

Contact.rdfResourceAttrib("urn:mozilla:zap:sidebarparent",
                          "urn:mozilla:zap:friends");
Contact.rdfLiteralAttrib("urn:mozilla:zap:chromepage",
                         "chrome://zap/content/contacts.xul");
Contact.rdfLiteralAttrib("http://home.netscape.com/NC-rdf#Name", "");
Contact.rdfLiteralAttrib("urn:mozilla:zap:nodetype", "contact");
Contact.rdfLiteralAttrib("urn:mozilla:zap:sip1", "");
Contact.rdfLiteralAttrib("urn:mozilla:zap:sip2", "");
Contact.rdfLiteralAttrib("urn:mozilla:zap:organization", "");
Contact.rdfLiteralAttrib("urn:mozilla:zap:email", "");
Contact.rdfLiteralAttrib("urn:mozilla:zap:homepage", "");
// if isfriend is true, the contact will be displayed in the sidebar:
Contact.rdfLiteralAttrib("urn:mozilla:zap:isfriend", "true");

Contact.spec(
  function createFromDocument(doc) {
    this._Contact_createFromDocument(doc);
    // append to contacts:
    wContactsContainer.AppendElement(this.resource);
    // ... and if it's a friend, to the friends list as well:
    if (this["urn:mozilla:zap:isfriend"] == "true")
      wFriendsContainer.AppendElement(this.resource);
  });

 Contact.spec(
   function updateFromDocument(doc) {
     this._Contact_updateFromDocument(doc);
     // add/remove from friends container:
     var index = wFriendsContainer.IndexOf(this.resource);
     if (this["urn:mozilla:zap:isfriend"] == "true") {
       if (index == -1)
         wFriendsContainer.AppendElement(this.resource);
     }
     else {
       if (index != -1)
         wFriendsContainer.RemoveElementAt(index, true);
     }
   });

////////////////////////////////////////////////////////////////////////
// Sidebar:

/*
  Sidebar RDF datasource:
                           zap:
                     sidebarparent _n
                            __     __
                           |  |   |  |
                           V  |   |  V
 +------------+         +---------------+    zap:     +----------+
 |  resource: |   _n    |   resource:   | chromepage  | literal: |
 | zap:sidebar|-------->|  sidebarnode  |------------>|   url    |
 +------------+         +---------------+             +----------+
                          |           |
                          | NC:Name   | zap:nodetype
                          V           V
                   +----------+  +----------+
                   | literal: |  | literal: |
                   |   name   |  |  type    |
                   +----------+  +----------+
*/

// initialize wSidebarDS and wSidebarTree:
function initSidebar() {
  wSidebarDS = wRDF.GetDataSourceBlocking(getProfileFileURL("sidebar.rdf"));
  wSidebarTree = document.getElementById("sidebar");
  wSidebarTree.database.AddDataSource(wSidebarDS);
  wSidebarTree.database.AddDataSource(wIdentitiesDS);
  wSidebarTree.database.AddDataSource(wContactsDS);
  wSidebarTree.database.AddDataSource(wGlobalEphemeralDS);

  wSidebarTree.builder.rebuild();
  selectSidebarNode("urn:mozilla:zap:home");
}

function getSelectedSidebarResource()
{
  return getSelectedResource(wSidebarTree);
}

function getSidebarResourceAttrib(source, attrib) {
  var property = wRDF.GetResource(attrib);
  var target = wSidebarTree.database.GetTarget(source, property, true);
  if (target)
    target = target.QueryInterface(Components.interfaces.nsIRDFLiteral).Value;
  return target;
}

function getSidebarResourceParent(source) {
  var property = wRDF.GetResource("urn:mozilla:zap:sidebarparent");
  var target = wSidebarTree.database.GetTarget(source, property, true);
  if (target)
    target = target.QueryInterface(Components.interfaces.nsIRDFResource);
  return target;
}

function clearSidebarSelection()
{
  var selection = wSidebarTree.view.selection;
  if (selection)
    selection.clearSelection();
}

function ensureSidebarNode(uri) {
  var resource = wRDF.GetResource(uri);
  var index = wSidebarTree.builder.getIndexOfResource(resource);
  if (index == -1) {
    // The node isn't in the tree. Try recursively expanding ancestors:
    var parent = getSidebarResourceParent(resource);
    if (parent) {
      var parentIndex = ensureSidebarNode(parent.Value);
      wSidebarTree.builder.toggleOpenState(parentIndex);
      index = wSidebarTree.builder.getIndexOfResource(resource);
    }
  }
  return index;
}

function selectSidebarNode(uri)
{
  var index = ensureSidebarNode(uri);
  if (index < 0) return;
  wSidebarTree.view.selection.select(index);
  wSidebarTree.treeBoxObject.ensureRowIsVisible(index);
}

// called by the sidebar whenever its selection changes:
function sidebarSelectionChange()
{
  var page;
  var selection = getSelectedSidebarResource();
  if (!selection) return;

  clearCallsSelection();
  page = getSidebarResourceAttrib(selection, "urn:mozilla:zap:chromepage");
  if (!page) page = "about:blank";

  if (page != wInteractPane.getAttribute("src"))
    wInteractPane.setAttribute("src", page);
  else
    wInteractPane.webNavigation.reload(0);
}

////////////////////////////////////////////////////////////////////////
// MediaPipeline

var PB = makePropertyBag;

var wMediaPipeline = {
  init : function() {
    try {
      // We use getService() instead of createInstance() to get a globally
      // well-known media graph instance:
      this.mediagraph = Components.classes["@mozilla.org/zap/mediagraph;1"].getService(Components.interfaces.zapIMediaGraph);

      // 8000Hz, 20ms, 1 channel audio pipe:      
      this.audioin = this.mediagraph.addNode("audioin", null);
      this.asplitter = this.mediagraph.addNode("splitter", null);
      this.audioout = this.mediagraph.addNode("audioout", null);
      this.amixer = this.mediagraph.addNode("audio-mixer", null);
      this.A = this.mediagraph.connect(this.audioin, null,
                                       this.asplitter, null);
      this.B = this.mediagraph.connect(this.amixer, null,
                                       this.audioout, null);
      this.mediagraph.setAlias("ain", this.asplitter);
      this.mediagraph.setAlias("aout", this.amixer);
    }
    catch(e) {
      dump("Error during media pipeline initialization: "+e+"\n");
    }
  },
  
  cleanup: function() {
    try {
      this.mediagraph.shutdown();
    }
    catch(e) {}
    delete this.mediagraph;
  }
  
}

////////////////////////////////////////////////////////////////////////
// SipStack


function initSipStack() {
  wSdpService = Components.classes["@mozilla.org/zap/sdpservice;1"].
    getService(Components.interfaces.zapISdpService);
  
  wSipStack = Components.classes["@mozilla.org/zap/sipstack;1"].createInstance(Components.interfaces.zapISipUAStack);

  // make sure we generate a UA instance id now if we haven't got one
  // already: (see draft-ietf-sip-gruu-05.txt)
  if (!wConfig["urn:mozilla:zap:instance_id"]) {
    var uuidgen = Components.classes["@mozilla.org/zap/uuid-generator;1"].getService(Components.interfaces.zapIUUIDGenerator);
    wConfig["urn:mozilla:zap:instance_id"] = uuidgen.generateUUIDURNString();
    wConfig.flush();
  }
  
  wSipStack.init(wUAHandler,
                 makePropertyBag({$instance_id:wConfig["urn:mozilla:zap:instance_id"],
                                  $port_base:wConfig["urn:mozilla:zap:sip_port_base"],
                                  $methods: "OPTIONS,INVITE,ACK,CANCEL,BYE",
                                  $extensions: "path", // path: RFC3327
                                  $user_agent: wUserAgent
                                 }));
  document.getElementById("status_text").label = "Listening for UDP/TCP SIP traffic on port "+wSipStack.listeningPort;

  // set up logging to profile_dir/zap.sbox:
  wSipTrafficLogger.logOutputStream = openFileForWriting(getProfileFile("zap.sbox"),
                                                         true);
  wSipStack.transport.transceiver.trafficMonitor = wSipTrafficLogger;
}

function cleanupSipStack() {
  wSipTrafficLogger.logOutputStream.close();
  wSipTrafficLogger.logOutputStream = null;
}

// traffic logger for logging the raw sip traffic to profile_dir/zap.sbox
var wSipTrafficLogger = {
  logOutputStream : null,
  notifyPacket: function(data, protocol, localAddress, localPort,
                         remoteAddress, remotePort, outbound) {
    if (!this.logOutputStream) return;
    var entry = "- From "+localAddress+":"+localPort;
    if (outbound)
      entry += "-->";
    else
      entry += "<--";
    entry += remoteAddress+":" + remotePort + " " + protocol + " ";
    entry += data.length + "bytes " + (new Date()).toUTCString() + "\n";
    if (wNetUtils.snoopStunPacket(data) != -2)
      entry += bin2hex(data, 16);
    else
      entry += data;
    entry += "\n\n";

    this.logOutputStream.write(entry, entry.length);
  }
};


// our UA request handler:
var wUAHandler = {
  handleNonInviteRequest: function(rs) {
    return false;
  },

  handleInviteRequest: function(rs) {
    var ic = InboundCall.instantiate();
    ic.createNew(true);
    ic.receiveCall(rs);
    return true;
  }
};

////////////////////////////////////////////////////////////////////////
// Calls

// initialize wCallsDS, wCallsTree, wCallsContainer and
// wRecentCallsContainer:
function initCalls() {
  wCallsDS = wRDF.GetDataSourceBlocking(getProfileFileURL("calls.rdf"));
  wCallsTree = document.getElementById("calls");
  wCallsTree.database.AddDataSource(wCallsDS);
  wCallsTree.builder.rebuild();

  wCallsContainer = Components.classes["@mozilla.org/rdf/container;1"].
    createInstance(Components.interfaces.nsIRDFContainer);
  wCallsContainer.Init(wCallsDS,
                              wRDF.GetResource("urn:mozilla:zap:calls"));
  wRecentCallsContainer = Components.classes["@mozilla.org/rdf/container;1"].
    createInstance(Components.interfaces.nsIRDFContainer);
  wRecentCallsContainer.Init(wCallsDS,
                              wRDF.GetResource("urn:mozilla:zap:recent_calls"));

  // Check database for any active call zombies left over from
  // unclean shutdown:
  var recent = wRecentCallsContainer.GetElements();
  var resourceActive = wRDF.GetResource("urn:mozilla:zap:active");
  var resourceStatus = wRDF.GetResource("urn:mozilla:zap:status");
  while (recent.hasMoreElements()) {
    var resource = recent.getNext();
    var active = wCallsDS.GetTarget(resource, resourceActive, true).QueryInterface(Components.interfaces.nsIRDFLiteral);
    if (active.Value == "true") {
      // aha, a zombie!
      wCallsDS.Change(resource, resourceActive, active, wRDF.GetLiteral("false"), true);
      var old_status = wCallsDS.GetTarget(resource, resourceStatus, true);
      wCallsDS.Change(resource, resourceStatus, old_status, wRDF.GetLiteral("zombied"), true);
    }
  }
}

function clearCallsSelection() {
  var selection = wCallsTree.view.selection;
  if (selection)
    selection.clearSelection();
}

function selectCall(resource) {
  selectResource(wCallsTree, resource);
}

var wSuppressSelectionChangeNotifications = false;

function callsSelectionChange() {
  if (wSuppressSelectionChangeNotifications) return;
  
  var selection = getSelectedResource(wCallsTree);
  if (!selection) return;
  
  clearSidebarSelection();
  if (wInteractPane.getAttribute("src") != "chrome://zap/content/call.xul")
    wInteractPane.setAttribute("src", "chrome://zap/content/call.xul");
  else
    wInteractPane.webNavigation.reload(0);
}

function getSelectedCallResource()
{
  return getSelectedResource(wCallsTree);
}

// hash of active calls, indexed by resource name:
var wActiveCalls = {};

// get the Call object for the given resource. This will either be a
// new Call instance or the cached instance in wActiveCalls if the
// call is currently active:
function getCall(resource) {
  var call = wActiveCalls[resource.Value];
  if (!call) {
    call = Call.instantiate();
    call.initWithResource(resource);
  }
  return call;
}

//----------------------------------------------------------------------
// Call class

var Call = makeClass("Call", PersistentRDFObject);

Call.prototype.datasources["default"] = wCallsDS;
Call.addInMemoryDS("ephemeral");

// persistent attributes:
Call.rdfLiteralAttrib("urn:mozilla:zap:active", "true");
Call.rdfLiteralAttrib("urn:mozilla:zap:status", "");
Call.rdfLiteralAttrib("urn:mozilla:zap:remote", "");
Call.rdfLiteralAttrib("urn:mozilla:zap:subject", "");

// ephemeral attributes:
Call.rdfLiteralAttrib("urn:mozilla:zap:session-running", "false", "ephemeral");

// This is to provide an entrypoint for template recursion. see
// e.g. call.xul for usage and comments in
// RDFUtils.js::rdfPointerAttrib:
Call.rdfPointerAttrib("urn:mozilla:zap:root",
                      "urn:mozilla:zap:current-call",
                      "ephemeral");


// Add this call to the recent calls container (so that it displays
// in the call list). Maybe remove old items if the list is getting too large:
Call.fun(
  function addToRecentCalls() {
    wSuppressSelectionChangeNotifications = true;
    
    this._assert(this.resource, "can't add noninitialized call object");
    wRecentCallsContainer.InsertElementAt(this.resource, 1, true);
    // XXX renumber etc.

    // check if we need to remove items from the recent calls list:
    var last_call_index = wRecentCallsContainer.GetCount();
    var overflow = last_call_index - wConfig["urn:mozilla:zap:max_recent_calls"];
    while (overflow > 0 && last_call_index >= 1) {
      this._dump("looking at "+last_call_index);
      var target = wCallsDS.GetTarget(wRDF.GetResource("urn:mozilla:zap:recent_calls"),
                                        wRDFContainerUtils.IndexToOrdinalResource(last_call_index), true);

      //this._assert(target, "recent calls ds broken at "+last_call_index);
      if (!target) {
        --last_call_index;
        continue;
      }
      
      // if this is an inactive call, remove it:
      var active = wCallsDS.GetTarget(target, wRDF.GetResource("urn:mozilla:zap:active"), true).QueryInterface(Components.interfaces.nsIRDFLiteral).Value;
      if (active == "true") {
        --last_call_index;
        continue;
      }
      
      wRecentCallsContainer.RemoveElementAt(last_call_index, true);
      --last_call_index;
      --overflow;
    }
    wSuppressSelectionChangeNotifications = false;
  });

//----------------------------------------------------------------------
// Active call

var ActiveCall = makeClass("ActiveCall", Call, SupportsImpl);
ActiveCall.addInterfaces(Components.interfaces.zapISipDialogListener);

// Mediasession and dialog will only be valid for our subclasses
// OutboundCall and InboundCall while a call is in progress:
ActiveCall.obj("mediasession", null);
ActiveCall.obj("dialog", null);

ActiveCall.spec(
  function createFromDocument(doc) {
    this._ActiveCall_createFromDocument(doc);
    // append to calls container:
    wCallsContainer.AppendElement(this.resource);
    
    // ... and to recent calls as well:
    this.addToRecentCalls();

    // enter into active calls hash:
    wActiveCalls[this.resource.Value] = this;
  });

ActiveCall.spec(
  function createNew(addAssertions) {
    this._ActiveCall_createNew(addAssertions);
    // append to calls container:
    wCallsContainer.AppendElement(this.resource);

    // ... and to recent calls as well:
    this.addToRecentCalls();

    // enter into active calls hash:
    wActiveCalls[this.resource.Value] = this;
  });

// clean up after call has terminated
ActiveCall.fun(
  function terminated() { 
    this["urn:mozilla:zap:active"] = "false";
    delete wActiveCalls[this.resource.Value];
    if (this.mediasession) {
      this.mediasession.shutdown();
      this.mediasession = null;
    }
    this.dialog = null;
    this.flush();
  });

// zapISipDialogListener methods:
ActiveCall.fun(
  function notifyDialogTerminated(dialog) {
    this["urn:mozilla:zap:status"] = "Terminated";
    this.terminated();
  });

//----------------------------------------------------------------------
// OutboundCall class

var OutboundCall = makeClass("OutboundCall", ActiveCall);

OutboundCall.rdfLiteralAttrib("urn:mozilla:zap:direction", "outbound");

OutboundCall.fun(
  function makeCall(toAddress) {
    this.callHandler = OutboundCallHandler.instantiate();
    this.callHandler.call = this;

    var rc = wSipStack.createInviteRequestClient(toAddress,
                                                 wCurrentIdentity.routeset,
                                                 wCurrentIdentity.routeset.length);
    rc.listener = this.callHandler;
    this.callHandler.rc = rc;

    var subject = this["urn:mozilla:zap:subject"];
    if (subject) {
      try {
        var s = wSipStack.syntaxFactory.createHeader("Subject").QueryInterface(Components.interfaces.zapISipSubjectHeader);
        s.subject = subject;
        rc.request.appendHeader(s);
      }
      catch(e) {
        this._warning("Exception during Subject header construction: "+e);
      }
    }
    
    var offer = this.mediasession.generateSDPOffer();

    rc.request.setContent("application", "sdp", offer.serialize());
    rc.sendInvite(this.callHandler);
  });

//----------------------------------------------------------------------
// OutboundCallHandler
// CALLING -> 2XX_RECEIVED -> TERMINATED

var OutboundCallHandler = makeClass("OutboundCallHandler",
                                    SupportsImpl, StateMachine);
OutboundCallHandler.addInterfaces(Components.interfaces.zapISipInviteResponseHandler,
                                  Components.interfaces.zapISipInviteRCListener);

// zapISipInviteResponseHandler methods:
OutboundCallHandler.fun(
  function handle2XXResponse(rc, dialog, response, ackTemplate) {
    if (this.call.dialog) {
      this._dump("received additional dialog for outbound call");
      // we already have a dialog. this must be a subsequent dialog
      // from a forking proxy. send a bye immediately:
      // XXX probably not a good idea to do this before sending the ack
      dialog.createNonInviteRequestClient("BYE").sendRequest();
      return ackTemplate;
    }

    // try to parse answer:
    var sd;
    try {
      sd = wSdpService.deserializeSessionDescription(response.body);
    }
    catch(e) {
      this["urn:mozilla:zap:status"] = "Can't parse other party's session description";
      // send a BYE:
      // XXX need a way to send ACK first
      dialog.createNonInviteRequestClient("BYE").sendRequest();
      return ackTemplate;
    }

    // we've got a parsed answer; process it:
    try {
      this.call.mediasession.processSDPAnswer(sd);
    }
    catch(e) {
      // XXX add verbose error
      this["urn:mozilla:zap:status"] = "Session negotiation failed";
      
      // send a BYE:
      // XXX need a way to send ACK first
      dialog.createNonInviteRequestClient("BYE").sendRequest();

      return ackTemplate;
    }

    // finally. we've cleared all the hurdles:
    this.call.mediasession.startSession();
    this.call.dialog = dialog;
    this.call.dialog.listener = this.call;
    this.call["urn:mozilla:zap:session-running"] = "true";
    return ackTemplate;
  });

// zapISipInviteRCListener methods:
OutboundCallHandler.fun(
  function notifyResponseReceived(rc, dialog, response, flow) {
    this._dump(response.statusCode+" response for call="+this.call);
    if (response.statusCode[0] == "1")
      this.call["urn:mozilla:zap:status"] =
        response.statusCode + " " + response.reasonPhrase;
    else if (response.statusCode[0] == "2") {
      this.call["urn:mozilla:zap:status"] = "Connected";
      this.changeState("2XX_RECEIVED");
      // we'll terminate from notifyTerminated now
    }
    else if (response.statusCode == "401" ||
             response.statusCode == "407") {
      if (wSipStack.authentication.
          addAuthorizationHeaders(wCurrentIdentity,
                                  response,
                                  rc.request)) {
        // we've got new credentials -> retry
        rc.sendInvite(this);
      }
    }
//     else if (response.statusCode == "301" ||
//              ...) {
//       // retry with new destination
//     }
    else {
      this.call["urn:mozilla:zap:status"] =
        response.statusCode + " " + response.reasonPhrase;
      this.terminate();
    }
  });

OutboundCallHandler.statefun(
  "2XX_RECEIVED",
  function notifyTerminated(rc) {
    this._dump("terminated in 2XX_RECEIVED state. call="+this.call);
    this.terminate();
  });

OutboundCallHandler.statefun(
  "*",
  function notifyTerminated(rc) {
    this._dump("ignoring responsehandler termination.");
  });

OutboundCallHandler.fun(
  function terminate() {
    this.changeState("TERMINATED");
    if (!this.call.dialog) {
      this.call.terminated();
    }
    delete this.call.callHandler;
    delete this.call;
    delete this.rc;
  });

//----------------------------------------------------------------------
// InboundCall

var InboundCall = makeClass("InboundCall", ActiveCall);

InboundCall.rdfLiteralAttrib("urn:mozilla:zap:direction", "inbound");

InboundCall.fun(
  function receiveCall(rs) {
    this["urn:mozilla:zap:remote"] = rs.request.getFromHeader().address.serialize();
    var subjectHeader = rs.request.getTopHeader("Subject");
    if (subjectHeader) {
      this["urn:mozilla:zap:subject"] = subjectHeader.QueryInterface(Compontents.interfaces.zapISipSubjectHeader).subject;
    }
    this.callHandler = InboundCallHandler.instantiate();
    this.callHandler.handleCall(this, rs);
  });

//----------------------------------------------------------------------
// InboundCallHandler

var InboundCallHandler = makeClass("InboundCallHandler", SupportsImpl);
InboundCallHandler.addInterfaces(Components.interfaces.zapISipInviteRSListener);

InboundCallHandler.fun(
  function handleCall(call, rs) {
    this.call = call;
    this.rs = rs;
    rs.listener = this;
    // reject based on busy settings:
    if (document.getElementById("button_dnd").checked) {
      this._respond(wConfig["urn:mozilla:zap:dnd_code"],
                    wConfig["urn:mozilla:zap:dnd_headers"]);
      return;
    }

    // check if there is a media offer in the request:
    // XXXif (){}
    
    // parse offer; check if it's acceptable:
    try {
      var offer = wSdpService.deserializeSessionDescription(rs.request.body);
      this.call.mediasession = Components.classes["@mozilla.org/zap/mediasession;1"]
        .createInstance(Components.interfaces.zapIMediaSession);
      this.call.mediasession.init("zap",
                                  wSipStack.hostAddress,
                                  wSipStack.hostAddress);
      this.answer = this.call.mediasession.processSDPOffer(offer);
    }
    catch(e) {
      // Session negotiation failed.
      // Send 488 (Not acceptable) (RFC3261 13.3.1.3)
      // XXX set a Warning header
      this._respond("488");
      return;
    }

    // the offer is acceptable; we have an answer at hand. ring the user:
    this._respond("180");
  });

InboundCallHandler.fun(
  function acceptCall() {
    // formulate accepting response:
    var resp = this.rs.formulateResponse("200");
    // append session description:
    resp.setContent("application", "sdp", this.answer.serialize());
    this.call["urn:mozilla:zap:status"] = resp.statusCode+" "+resp.reasonPhrase;
    
    this.call.mediasession.startSession();
    this.call.dialog = this.rs.sendResponse(resp);
    this.call.dialog.listener = this.call;    
    this.call["urn:mozilla:zap:session-running"] = "true";
  });

InboundCallHandler.fun(
  function rejectCall() {
    this._respond("603");
  });

InboundCallHandler.fun(
  function _respond(code, extra_headers) {
    var resp = this.rs.formulateResponse(code);

    if (extra_headers) {
      // add in extra header, converting line endings appropriately:
      var responseTxt = resp.serialize();
      responseTxt = responseTxt.substring(0, responseTxt.length-2); // strip last CRLF
      responseTxt += extra_headers;
      responseTxt = responseTxt.replace(/\r/g, "");
      responseTxt = responseTxt.replace(/\n/g, "\r\n");
      responseTxt += "\r\n\r\n";
      resp = wSipStack.syntaxFactory.deserializeMessage(responseTxt).QueryInterface(Components.interfaces.zapISipResponse);
    }
    
    this.call["urn:mozilla:zap:status"] = resp.statusCode+" "+resp.reasonPhrase;
    return this.rs.sendResponse(resp);
  });
  
// zapISipInviteRSListener methods:
InboundCallHandler.fun(
  function notifyCancelled(rs) {
    this.call["urn:mozilla:zap:status"] = "Missed";
  });

InboundCallHandler.fun(
  function notifyACKReceived(rs, ack) {
    this.call["urn:mozilla:zap:status"] = "Confirmed";
  });

InboundCallHandler.fun(
  function notifyTerminated(rs) {
    if (!this.call.dialog) {
      this.call.terminated();
    }
    this.call.callHandler = null;
    this.call = null;
    this.rs = null;
  });

