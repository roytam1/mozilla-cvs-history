/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * The Original Code is JSIRC Test Client #3
 *
 * The Initial Developer of the Original Code is New Dimensions Consulting,
 * Inc. Portions created by New Dimensions Consulting, Inc. are
 * Copyright (C) 1999 New Dimenstions Consulting, Inc. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *
 *
 * Contributor(s):
 *  Robert Ginda, rginda@ndcico.com, original author
 */

/*
 * currently recognized prefs:
 * + extensions.irc.
 *   +- nickname (String)  initial nickname
 *   +- username (String)  initial username (ie: username@host.tld)
 *   +- desc     (String)  initial description (used in whois info)
 *   +- defaultNet (String) default network to use for irc:// urls
 *   +- initialURLs (String) irc:// urls to connect to on startup, semicolon
 *   |                       seperated
 *   +- initialScripts (String) urls for scripts to run at startup,
 *   |                          semicolon seperated
 *   +- newTabThreshold (Number) max number of tabs to have open before disabling
 *   |                           automatic tab creation for private messages.
 *   |                           use 0 for unlimited new tabs, or 1 to disable
 *   |                           automatic tab creation.
 *   +- focusNewTab (Boolean) bring new tabs created in response to a private
 *   |                        message to the front.
 *   +- munger   (Boolean) send output through text->html munger
 *   |  +- colorCodes (Boolean) enable color code handling
 *   |  +- <various>  (Boolean) enable specific munger entry
 *   |  +- smileyText (Boolean) true => display text (and graphic) when
 *   |                                  matching smileys
 *   |                          false => show only the smiley graphic
 *   +- nickCompleteStr (String) String to use when tab-completing nicknames
 *   |                           at the beginning of sentences
 *   +- stalkWords (String) List of words to add to the stalk victims list
 *   |                      semicolon seperated (see the /stalk command)
 *   +- deleteOnPart (Boolean) Delete channel window automatically after a /part
 *   |
 *   |  The following beep prefs can be set to the text "beep" to use the
 *   |  system beep, or "" to disable the beep.
 *   +- msgBeep   (String) url to sound to play when a /msg is recieved
 *   +- stalkBeep (String) url to sound to play when a /stalk matches
 *   +- queryBeep (String) url to sound to play for new msgs in a /query
 *   |
 *   +- notify
 *   |  +- aggressive (Boolean) flash trayicon/ bring window to top when
 *   |                          your nickname is mentioned.
 *   +- settings
 *   |  +- autoSave (Boolean) Save settings on exit
 *   +- style   
 *   |  +- default (String) url to default style sheet
 *   +- views
 *   |  +- collapseMsgs (Boolean) Collapse consecutive messages from same
 *   |  |                         user
 *   |  +- client
 *   |  |  +- maxlines  (Number) max lines to keep in *client* view
 *   |  +- network
 *   |  |  +- maxlines  (Number) max lines to keep in network views
 *   |  +- channel
 *   |  |  +- maxlines  (Number) max lines to keep in channel views
 *   |  +- chanuser
 *   |     +- maxlines  (Number) max lines to keep in /msg views
 *   +- debug
 *      +- tracer (Boolean) enable/disable debug message tracing
 */

function readIRCPrefs (rootNode)
{
    var pref =
        Components.classes["@mozilla.org/preferences;1"].createInstance();
    if(!pref)
        throw ("Can't find pref component.");

    if (!rootNode)
        rootNode = "extensions.irc.";

    if (!rootNode.match(/\.$/))
        rootNode += ".";
    
    pref = pref.QueryInterface(Components.interfaces.nsIPref);

    CIRCNetwork.prototype.INITIAL_NICK =
        getCharPref (pref, rootNode + "nickname",
                     CIRCNetwork.prototype.INITIAL_NICK);
    CIRCNetwork.prototype.INITIAL_NAME =
        getCharPref (pref, rootNode + "username",
                     CIRCNetwork.prototype.INITIAL_NAME);
    CIRCNetwork.prototype.INITIAL_DESC =
        getCharPref (pref, rootNode + "desc",
                     CIRCNetwork.prototype.INITIAL_DESC);
    client.DEFAULT_NETWORK =
        getCharPref (pref, rootNode + "defaultNet", "moznet");    
    client.INITIAL_URLS =
        getCharPref (pref, rootNode + "initialURLs", "");
    client.INITIAL_SCRIPTS =
        getCharPref (pref, rootNode + "initialScripts", "");
    client.NEW_TAB_THRESHOLD =
        getIntPref (pref, rootNode + "newTabThreshold", 15);
    client.FOCUS_NEW_TAB =
        getIntPref (pref, rootNode + "focusNewTab", false);
    client.ADDRESSED_NICK_SEP =
        getCharPref (pref, rootNode + "nickCompleteStr",
                     client.ADDRESSED_NICK_SEP).replace(/\s*$/, "");
    client.INITIAL_VICTIMS =
        getCharPref (pref, rootNode + "stalkWords", "");
    
    client.DELETE_ON_PART =
        getCharPref (pref, rootNode + "deleteOnPart", true);

    client.STALK_BEEP =
        getCharPref (pref, rootNode + "stalkBeep", "beep");
    client.MSG_BEEP =
        getCharPref (pref, rootNode + "msgBeep", "beep beep");
    client.QUERY_BEEP =
        getCharPref (pref, rootNode + "queryBeep", "beep");
    
    client.munger.enabled =
        getBoolPref (pref, rootNode + "munger", client.munger.enabled);

    client.enableColors =
        getBoolPref (pref, rootNode + "munger.colorCodes", true);

    client.smileyText =
        getBoolPref (pref, rootNode + "munger.smileyText", false);

    for (var entry in client.munger.entries)
    {
        if (entry[0] != ".")
        {
            client.munger.entries[entry].enabled =
                getBoolPref (pref, rootNode + "munger." + entry,
                             client.munger.entries[entry].enabled);
        }
    }

    client.FLASH_WINDOW =
        getBoolPref (pref, rootNode + "notify.aggressive", true);

    client.SAVE_SETTINGS =
        getBoolPref (pref, rootNode + "settings.autoSave", true);

    client.DEFAULT_STYLE =
        getCharPref (pref, rootNode + "style.default",
                     "chrome://chatzilla/skin/output-default.css");
    
    client.COLLAPSE_MSGS = 
        getBoolPref (pref, rootNode + "views.collapseMsgs", false);

    client.MAX_MESSAGES = 
        getIntPref (pref, rootNode + "views.client.maxlines",
                    client.MAX_MESSAGES);

    CIRCNetwork.prototype.MAX_MESSAGES =
        getIntPref (pref, rootNode + "views.network.maxlines",
                    CIRCChanUser.prototype.MAX_MESSAGES);

    CIRCChannel.prototype.MAX_MESSAGES =
        getIntPref (pref, rootNode + "views.channel.maxlines",
                    CIRCChannel.prototype.MAX_MESSAGES);

    CIRCUser.prototype.MAX_MESSAGES =
        getIntPref (pref, rootNode + "views.chanuser.maxlines",
                    CIRCChanUser.prototype.MAX_MESSAGES);
    
    var h = client.eventPump.getHook ("event-tracer");
    h.enabled = client.debugMode =
        getBoolPref (pref, rootNode + "debug.tracer", h.enabled);
    
}

function writeIRCPrefs (rootNode)
{
    var pref =
        Components.classes["@mozilla.org/preferences;1"].createInstance();
    if(!pref)
        throw ("Can't find pref component.");

    if (!rootNode)
        rootNode = "extensions.irc.";

    if (!rootNode.match(/\.$/))
        rootNode += ".";
    
    pref = pref.QueryInterface(Components.interfaces.nsIPref);

    pref.SetCharPref (rootNode + "nickname",
                      CIRCNetwork.prototype.INITIAL_NICK);
    pref.SetCharPref (rootNode + "username",
                      CIRCNetwork.prototype.INITIAL_NAME);
    pref.SetCharPref (rootNode + "desc", CIRCNetwork.prototype.INITIAL_DESC);
    pref.SetCharPref (rootNode + "nickCompleteStr", client.ADDRESSED_NICK_SEP);
    pref.SetCharPref (rootNode + "initialURLs", client.INITIAL_URLS);
    pref.SetCharPref (rootNode + "initialScripts", client.INITIAL_SCRIPTS);
    pref.SetCharPref (rootNode + "newTabThreshold", client.NEW_TAB_THRESHOLD);
    pref.SetCharPref (rootNode + "focusNewTab", client.FOCUS_NEW_TAB);
    pref.SetCharPref (rootNode + "style.default", client.DEFAULT_STYLE);
    pref.SetCharPref (rootNode + "stalkWords",
                      client.stalkingVictims.join ("; "));
    pref.SetCharPref (rootNode + "stalkBeep", client.STALK_BEEP);
    pref.SetCharPref (rootNode + "msgBeep", client.MSG_BEEP);
    pref.SetCharPref (rootNode + "queryBeep", client.QUERY_BEEP);    
    pref.SetBoolPref (rootNode + "munger", client.munger.enabled);
    pref.SetBoolPref (rootNode + "munger.colorCodes", client.enableColors);
    pref.SetBoolPref (rootNode + "munger.smileyText", client.smileyText);
    for (var entry in client.munger.entries)
    {
        if (entry[0] != ".")
        {
            pref.SetBoolPref (rootNode + "munger." + entry,
                              client.munger.entries[entry].enabled);
        }
    }
    pref.SetBoolPref (rootNode + "notify.aggressive", client.FLASH_WINDOW);
    pref.SetBoolPref (rootNode + "views.collapseMsgs", client.COLLAPSE_MSGS);
    pref.SetIntPref (rootNode + "views.client.maxlines", client.MAX_MESSAGES);
    pref.SetIntPref (rootNode + "views.network.maxlines",
                     CIRCChanUser.prototype.MAX_MESSAGES);
    pref.SetIntPref (rootNode + "views.channel.maxlines",
                     CIRCChannel.prototype.MAX_MESSAGES);
    pref.SetIntPref (rootNode + "views.chanuser.maxlines",
                     CIRCChanUser.prototype.MAX_MESSAGES);
    
    var h = client.eventPump.getHook ("event-tracer");
    pref.SetBoolPref (rootNode + "debug.tracer", h.enabled);
    
}

function getCharPref (prefObj, prefName, defaultValue)
{
    var e, rv;
    
    try
    {
        rv = prefObj.CopyCharPref (prefName);
    }
    catch (e)
    {
        rv = defaultValue;
    }

    //dd ("getCharPref: returning '" + rv + "' for " + prefName);
    return rv;
    
}

function getIntPref (prefObj, prefName, defaultValue)
{
    var e;

    try
    {
        return prefObj.GetIntPref (prefName);
    }
    catch (e)
    {
        return defaultValue;
    }
    
}

function getBoolPref (prefObj, prefName, defaultValue)
{
    var e;

    try
    {
        return prefObj.GetBoolPref (prefName);
    }
    catch (e)
    {
        return defaultValue;
    }
    
}
