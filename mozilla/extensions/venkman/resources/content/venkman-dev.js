/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/ 
 * 
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License. 
 *
 * The Original Code is The JavaScript Debugger
 * 
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation
 * Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.
 *
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU Public License (the "GPL"), in which case the
 * provisions of the GPL are applicable instead of those above.
 * If you wish to allow use of your version of this file only
 * under the terms of the GPL and not to allow others to use your
 * version of this file under the MPL, indicate your decision by
 * deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL.  If you do not delete
 * the provisions above, a recipient may use your version of this
 * file under either the MPL or the GPL.
 *
 * Contributor(s):
 *  Robert Ginda, <rginda@netscape.com>, original author
 *
 */

function initDev()
{
    var cmdary =
        [["dumpcontexts",  cmdDumpContexts,  CMD_CONSOLE | CMD_NO_HELP],
         ["dumpfilters",   cmdDumpFilters,   CMD_CONSOLE | CMD_NO_HELP],
         ["dumptree",      cmdDumpTree,      CMD_CONSOLE | CMD_NO_HELP],
         ["reloadui",      cmdReloadUI,      CMD_CONSOLE | CMD_NO_HELP],
         ["sync-debug",    cmdSyncDebug,     CMD_CONSOLE | CMD_NO_HELP],
         ["testargs",      cmdTestArgs,      CMD_CONSOLE | CMD_NO_HELP],
         ["testargs1",     cmdTestArgs,      CMD_CONSOLE | CMD_NO_HELP],
         ["testfilters",   cmdTestFilters,   CMD_CONSOLE | CMD_NO_HELP],
         ["treetest",      cmdTreeTest,      CMD_CONSOLE | CMD_NO_HELP],

         ["multialias",    "help pref; help props", CMD_CONSOLE | CMD_NO_HELP]];
         
    
    defineVenkmanCommands (cmdary);

    console.addPref ("dbgContexts", false);
    console.addPref ("dbgDispatch", false);
    console.addPref ("dbgRealize", false);
    dispatch ("sync-debug");
    
    return "Venkman development functions loaded OK.";
}

function cmdDumpContexts()
{
    console.contexts = new Array();
    var i = 0;
    
    var enumer = {
        enumerateContext: function _ec (context) {
                              if (!context.isValid)
                              {
                                  dd("enumerate got invalid context");
                              }
                              else
                              {
                                  var v = context.globalObject.getWrappedValue();
                                  var title = "<n/a>";
                                  if (v instanceof Object &&
                                      "document" in v)
                                  {
                                      title = v.document.title;
                                  }
                              
                                  dd ("enumerateContext: Index " + i +
                                      ", Version " + context.version +
                                      ", Tag " + context.tag +
                                      ", Title " + title +
                                      ", Scripts Enabled " +
                                      context.scriptsEnabled);
                              }
                              console.contexts[i++] = context;
                          }
    };
    
    console.jsds.enumerateContexts(enumer);
    return true;
}
    
function cmdDumpFilters()
{
    var i = 0;
    
    var enumer = {
        enumerateFilter: function enum_f (filter) {
                             dd ((i++) + ": " + filter.glob +
                                 " '" + filter.urlPattern + "'");
                         }
    };
    
    console.jsds.enumerateFilters (enumer);
}

function cmdDumpTree(e)
{
    if (!e.depth)
        e.depth = 0;
    dd(e.tree + ":\n" + tov_formatBranch (eval(e.tree), "", e.depth));
}

function cmdReloadUI()
{
    var bs = Components.classes["@mozilla.org/intl/stringbundle;1"];
    bs = bs.createInstance(Components.interfaces.nsIStringBundleService);
    bs.flushBundles();
    window.location.href = window.location.href;
}    

function cmdSyncDebug()
{
    if (console.prefs["dbgContexts"])
        console.dbgContexts = true;
    else
        delete console.dbgContexts;

    if (console.prefs["dbgDispatch"])
        console.dbgDispatch = true;
    else
        delete console.dbgDispatch;

    if (console.prefs["dbgRealize"])
        console.dbgRealize = true;
    else
        delete console.dbgRealize;
}

function cmdTestArgs (e)
{
    display (dumpObjectTree(e));
}

function cmdTestFilters ()
{
    var filter1 = {
        globalObject: null,
        flags: jsdIFilter.FLAG_ENABLED,
        urlPattern: "1",
        startLine: 0,
        endLine: 0
    };
    var filter2 = {
        globalObject: null,
        flags: jsdIFilter.FLAG_ENABLED,
        urlPattern: "*2",
        startLine: 0,
        endLine: 0
    };
    var filter3 = {
        globalObject: null,
        flags: jsdIFilter.FLAG_ENABLED,
        urlPattern: "3*",
        startLine: 0,
        endLine: 0
    };
    var filter4 = {
        globalObject: null,
        flags: jsdIFilter.FLAG_ENABLED,
        urlPattern: "*4*",
        startLine: 0,
        endLine: 0
    };

    dd ("append f3 into an empty list.");
    console.jsds.appendFilter (filter3);
    console.jsds.GC();
    dd("insert f1 at the top");
    console.jsds.insertFilter (filter1, null);
    console.jsds.GC();
    dd("insert f2 after f1");
    console.jsds.insertFilter (filter2, filter1);
    console.jsds.GC();
    dd("swap f4 in, f3 out");
    console.jsds.swapFilters (filter3, filter4);
    console.jsds.GC();
    dd("append f3");
    console.jsds.appendFilter (filter3);
    console.jsds.GC();
    dd("swap f4 and f3");
    console.jsds.swapFilters (filter3, filter4);
    console.jsds.GC();

    dumpFilters();
}


function cmdTreeTest()
{
    var w = openDialog("chrome://venkman/content/tests/tree.xul", "", "");
    var testsFilter = {
        globalObject: w,
        flags: jsdIFilter.FLAG_ENABLED | jsdIFilter.FLAG_PASS,
        urlPattern: null,
        startLine: 0,
        endLine: 0
    };
    /* make sure this filter goes at the top, so the system
     * "chrome://venkman/ *" filter doesn't get to it first.
     */
    console.jsds.insertFilter (testsFilter, null);
}

initDev();
