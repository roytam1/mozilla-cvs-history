/* -*- Mode: Java; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * The Original Code is from Alexa Internet (www.alexa.com).
 *
 * The Initial Developer of the Original Code is Alexa Internet.
 * Portions created by Alexa Internet are Copyright (C) 2000 Alexa
 * Internet. All Rights Reserved.
 *
 * Contributor(s):
 */

/*
  Code for the Related Links Sidebar Panel
 */


var kUnknownReasonUrl		= 'about:blank';
var kMAMIUrl				= 'http://xslt.alexa.com/data?cli=17&dat=nsa';

var kNoHTTPReasonUrl		= kMAMIUrl + 'req_type=secure_intranet';
var kSkipDomainReasonUrl	= kMAMIUrl + 'req_type=blocked_list';
var kDataPrefixUrl			= kMAMIUrl;

var oNavObserver = null;

function debug(msg) {
	// uncomment for noise
	// dump("-+- related-panel: " + msg + "\n");
}


// Our observer of Navigation Messages
function NavObserver(oDisplayFrame,oContentWindow)
{
	this.m_oDisplayFrame = oDisplayFrame;
	this.m_oContentWindow = oContentWindow;
	this.m_sWindowID = ''+parseInt(Math.random()*32767);
	this.m_sLastDataUrl = 'about:blank';	// The last url that we drove our display to.
}

NavObserver.prototype.Observe =
function (oSubject, sMessage, sContextUrl)
{
    debug("Observer("+sMessage+", "+sContextUrl+")");
	try {
		if (oSubject != this.m_oContentWindow) {
			// only pay attention to our client window.
			return;
		}
		var bReferrer = (this.m_oContentWindow.document.referrer)?true:false;
		if ((sMessage == 'EndDocumentLoad') 
            && (sContextUrl != this.m_oContentWindow.location)) {
				// we were redirected...
			sContextUrl = '' + this.m_oContentWindow.location;
			bReferrer = true;
		}
		this.TrackContext(sContextUrl, bReferrer);
	} catch(ex) {
	}
}

NavObserver.prototype.GetCDT =
function (bReferrer)
{
	var sCDT = '';
	sCDT += 't=' +(bReferrer?'1':'0');
	sCDT += '&pane=nswr6';
	sCDT += '&wid='+this.m_sWindowID;

	return escape(sCDT);
}

NavObserver.prototype.TrackContext =
function (sContextUrl, bReferrer)
{
    debug("TrackContext(" + sContextUrl + ", " + (bReferrer?"true":"false") + ")");
	if (sContextUrl != this.m_sLastContextUrl && this.m_oDisplayFrame) {
		var sDataUrl = this.TranslateContext(sContextUrl,bReferrer);
        debug("TrackContext: setting iframe to " + sDataUrl);
		this.m_oDisplayFrame.setAttribute('src', sDataUrl);
		this.m_sLastContextUrl = sContextUrl;
	}
}

NavObserver.prototype.TranslateContext =
function (sUrl, bReferrer)
{
    debug("TranslateContext(" + sUrl + ", " + (bReferrer?"true":"false") + ")");
	if (!sUrl || ('string' != typeof(sUrl))
        || ('' == sUrl) || sUrl == 'about:blank') {
		debug('TranlateContext: bad argument')
		return kUnknownReasonUrl;
	}

    // Strip off any query strings (Don't want to be too nosy).
	var nQueryPart = sUrl.indexOf('?');
    if (nQueryPart != 1) {
        sUrl = sUrl.slice(0, nQueryPart);
    }

	// We can only get related links data on HTTP URLs
	if (0 != sUrl.indexOf("http://")) {
		debug('TranlateContext: non-http argument:('+sUrl+')');
		return kNoHTTPReasonUrl;
	}

	// ...and non-intranet sites(those that have a '.' in the domain)
	var sUrlSuffix = sUrl.substr(7);			// strip off "http://" prefix

	var nFirstSlash = sUrlSuffix.indexOf('/');
	var nFirstDot = sUrlSuffix.indexOf('.');

	if (-1 == nFirstDot) {
		debug('TranlateContext: no "." in url');
		return kNoHTTPReasonUrl;
	}
	if ((nFirstSlash < nFirstDot) && (-1 != nFirstSlash)) {
		debug('TranlateContext: no "." in domain');
		return kNoHTTPReasonUrl;
	}

	// url is http, non-intranet url: see if the domain is in their blocked list.

	var nPortOffset = sUrlSuffix.indexOf(":");
	var nFirstSlash = sUrlSuffix.indexOf("/");
	var nDomainEnd = (((nPortOffset >=0) && (nPortOffset <= nFirstSlash))
                      ? nPortOffset : nFirstSlash);

	var sDomain = sUrlSuffix;
	if (-1 != nDomainEnd) {
		sDomain = sUrlSuffix.substr(0,nDomainEnd);
	}

	if (DomainInSkipList(sDomain)) {
		debug('TranlateContext: argument in skip-list');
		return kSkipDomainReasonUrl;
	} else {
		// ok! it is a good url!
		var sFinalUrl = kDataPrefixUrl;
		sFinalUrl += 'cdt='+this.GetCDT(bReferrer);
		sFinalUrl += '&url='+sUrl;
		return sFinalUrl;
	}
}


function DomainInSkipList(sDomain)
{
	var bSkipDomainFlag = false;

	if ('/' == sDomain[sDomain.length - 1]) {
		sDomain = sDomain.substring(0, sDomain.length - 1);
	}

	try {
		var pref = Components.classes["@mozilla.org/preferences;1"];
		if (pref) pref = pref.getService();
		if (pref) pref = pref.QueryInterface(Components.interfaces.nsIPref);
		if (pref) {
			var sDomainList = pref.CopyCharPref("browser.related.disabledForDomains");
			if ((sDomainList) && (sDomainList != "")) {
				debug("\nProposed New Domain: '" + sDomain + "'\n");
				debug("Skip Domain List: '" + sDomainList + "'\n");

				var aDomains = sDomainList.split(",");

				// split on commas
				for (var x=0; x < aDomains.length; x++) {
					var sDomainCopy = sDomain;

					var sTestDomain = aDomains[x];
					debug("Skip Domain #" + x + ": " + sTestDomain + "\n");

					if ('*' == sTestDomain[0]) { // wildcard match
						debug("    Wildcard domain.\n");

                        // strip off the asterisk
						sTestDomain = sTestDomain.substring(1);	
						if (sDomainCopy.length > sTestDomain.length) {
                            var sDomainIndex = sDomain.length - sTestDomain.length;
							sDomainCopy = sDomainCopy.substring(sDomainIndex);
						}
					}

					if (0 == sDomainCopy.lastIndexOf(sTestDomain)) {
						debug("    Skip this domain '" + sDomainCopy + "'\n");

						bSkipDomainFlag = true;
						break;
					}
				}
			}
		}
	} catch(ex) {
	}
	return(bSkipDomainFlag);
}

function Init()
{
	// Initialize the Related Links panel

	// Install our navigation observer so we can track the main client window.

	oContentWindow = window._content;
	oFrame = document.getElementById('daFrame');

	if (oContentWindow && oFrame) {
		window.setTimeout('window._content.resizeBy(0,1);window._content.resizeBy(0,-1);',0);
		var oObserverService = Components.classes["@mozilla.org/observer-service;1"].getService();
		oObserverService = oObserverService.QueryInterface(Components.interfaces.nsIObserverService);

		oNavObserver = new NavObserver(oFrame,oContentWindow);
		oNavObserver.TrackContext(''+oContentWindow.location);

		if (oObserverService && oNavObserver) {
			oObserverService.AddObserver(oNavObserver, "StartDocumentLoad");
			oObserverService.AddObserver(oNavObserver, "EndDocumentLoad");
			oObserverService.AddObserver(oNavObserver, "FailDocumentLoad");
			debug("added observer\n");
		} else {
			oNavObserver = null;
			debug("FAILURE to get observer service\n");
		}
	}
}

function Destruct()
{
	// remove our navigation observer.
	var oObserverService = Components.classes["@mozilla.org/observer-service;1"].getService();
	oObserverService = oObserverService.QueryInterface(Components.interfaces.nsIObserverService);
	if (oObserverService && oNavObserver) {
		debug("Removing observer\n");
		oObserverService.RemoveObserver(oNavObserver, "StartDocumentLoad");
		oObserverService.RemoveObserver(oNavObserver, "EndDocumentLoad");
		oObserverService.RemoveObserver(oNavObserver, "FailDocumentLoad");
		oNavObserver = null;
	} else {
		debug("FAILURE to get observer service\n");
	}
}

addEventListener("load", Init, false);
addEventListener("unload", Destruct, false);
