// -*- moz-jssh-buffer-globalobj: "Components.utils.import('gre:LoggingService.js', null)" -*-
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

debug("*** loading LoggingService.js\n");

Components.utils.import("gre:XPCOMUtils.jsm");
Components.utils.import("gre:ClassUtils.jsm");
Components.utils.import("gre:ArrayUtils.jsm");
Components.utils.import("gre:StringUtils.jsm");
Components.utils.import("gre:ObjectUtils.jsm");

// name our global object:
// function toString() { return "[LoggingService.js]"; }

////////////////////////////////////////////////////////////////////////
// Class LoggingService

var LoggingService = makeClass("LoggingService", SupportsImpl);
LoggingService.addInterfaces(Components.interfaces.zapILoggingService);

// true if we are currently in a log call:
LoggingService.obj("busy", false);

LoggingService.appendCtor(
  function() {
    this.listeners = [];
  });

//----------------------------------------------------------------------
// zapILoggingService implementation:

// void log(in ACString logmodule, in unsigned short level, in ACString message);
LoggingService.fun(
  function log(logmodule, level, message) {
    if (this.busy) return; // ignore silently
    this.busy = true;
    try {
      this.listeners.forEach(function(l) { l.log(logmodule, level, message); });
    } catch(e) {
      this._dump("Error during logging: "+e);
    }
    this.busy = false;
  });

// void addLogListener(in zapILogListener listener);
LoggingService.fun(
  function addLogListener(listener) {
    this.listeners.push(listener);
  });

// void removeLogListener(in zapILogListener listener);
LoggingService.fun(
  function removeLogListener(listener) {
    arraysplit(function(l) { return l==listener; },
               this.listeners);
  });

//----------------------------------------------------------------------
// global LoggingService object:

var theLoggingService = LoggingService.instantiate();


////////////////////////////////////////////////////////////////////////
// Module definition

NSGetModule = XPCOMUtils.generateNSGetModule(
  [{ className  : "ZAP Logging Service",
     cid        : Components.ID("{3ae046d9-a9b0-4033-8637-755eb015a986}"),
     contractID : "@mozilla.org/zap/loggingservice;1",
     factory    : XPCOMUtils.generateFactory(function() { return theLoggingService; })
  }]);
