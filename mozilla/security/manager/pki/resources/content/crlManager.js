/*
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 2001 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s):
 *  David Drinan <ddrinan@netscape.com>
 */

const nsIX509CertDB = Components.interfaces.nsIX509CertDB;
const nsX509CertDB = "@mozilla.org/security/x509certdb;1";
const nsICrlEntry = Components.interfaces.nsICrlEntry;
const nsISupportsArray = Components.interfaces.nsISupportsArray;
const nsIPKIParamBlock    = Components.interfaces.nsIPKIParamBlock;
const nsPKIParamBlock    = "@mozilla.org/security/pkiparamblock;1";
const nsIPref             = Components.interfaces.nsIPref;

var certdb;
var crls;
var prefs;

var autoupdateEnabledBaseString   = "security.crl.autoupdate.enable.";
var autoupdateTimeTypeBaseString  = "security.crl.autoupdate.timingType.";
var autoupdateTimeBaseString      = "security.crl.autoupdate.nextInstant.";
var autoupdateURLBaseString       = "security.crl.autoupdate.url.";
var autoupdateErrCntBaseString    = "security.crl.autoupdate.errCount.";
var autoupdateErrDetailBaseString = "security.crl.autoupdate.errDetail.";
var autoupdateDayCntString        = "security.crl.autoupdate.dayCnt.";
var autoupdateFreqCntString       = "security.crl.autoupdate.freqCnt.";

function onLoad()
{
  var crlEntry;
  var i;

  certdb = Components.classes[nsX509CertDB].getService(nsIX509CertDB);
  prefs = Components.classes["@mozilla.org/preferences;1"].getService(nsIPref);
  crls = certdb.getCrls();
  var bundle = srGetStrBundle("chrome://pippki/locale/pippki.properties");
  var autoupdateEnabledString;
  var autoupdateErrCntString;

  for (i=0; i<crls.Count(); i++) {
    crlEntry = crls.GetElementAt(i).QueryInterface(nsICrlEntry);
    var org = crlEntry.org;
    var orgUnit = crlEntry.orgUnit;
    var lastUpdate = crlEntry.lastUpdateLocale;
    var nextUpdate = crlEntry.nextUpdateLocale;
    autoupdateEnabledString    = autoupdateEnabledBaseString + crlEntry.nameInDb;
    autoupdateErrCntString    = autoupdateErrCntBaseString + crlEntry.nameInDb;
    var enabled = false;
    var enabledStr = bundle.GetStringFromName("crlAutoupdateNotEnabled");
    var status = "";
    try{
      enabled = prefs.GetBoolPref(autoupdateEnabledString)
      if(enabled){
        enabledStr = bundle.GetStringFromName("crlAutoupdateEnabled");
      }
      var cnt;
      cnt = prefs.GetIntPref(autoupdateErrCntString);
      if(cnt > 0){
        status = bundle.GetStringFromName("crlAutoupdateFailed");
      } else {
        status = bundle.GetStringFromName("crlAutoupdateOk");
      }
    }catch(exception){}
    
    AddItem("crlList", [org, orgUnit, lastUpdate, nextUpdate, enabledStr, status], "crltree_", i);
  }
}

function AddItem(children,cells,prefix,idfier)
{
  var kids = document.getElementById(children);
  var item  = document.createElement("treeitem");
  var row   = document.createElement("treerow");
  for(var i = 0; i < cells.length; i++)
  {
    var cell  = document.createElement("treecell");
    cell.setAttribute("class", "propertylist");
    cell.setAttribute("label", cells[i])
    row.appendChild(cell);
  }
  item.appendChild(row);
  item.setAttribute("id",prefix + idfier);
  kids.appendChild(item);
}

function DeleteCrlSelected() {
  var crlEntry;

  // delete selected item
  var crltree = document.getElementById("crltree");
  var i = crltree.selectedIndex;
  if(i<0){
    return;
  }
  crlEntry = crls.GetElementAt(i).QueryInterface(nsICrlEntry);
    
  var autoupdateEnabled = false;
  var autoupdateParamAvailable = false;
  var id = crlEntry.nameInDb;
  
  //First, check if autoupdate was enabled for this crl
  try {
    autoupdateEnabled = prefs.GetBoolPref(autoupdateEnabledBaseString + id);
    //Note, if the pref is not present, we get an exception right here,
    //and autoupdateEnabled remains false
    autoupdateParamAvailable = true;
    prefs.ClearUserPref(autoupdateEnabledBaseString + id);
    prefs.ClearUserPref(autoupdateTimeTypeBaseString + id);
    prefs.ClearUserPref(autoupdateTimeBaseString + id);
    prefs.ClearUserPref(autoupdateURLBaseString + id);
    prefs.ClearUserPref(autoupdateDayCntString + id);
    prefs.ClearUserPref(autoupdateFreqCntString + id);
    prefs.ClearUserPref(autoupdateErrCntBaseString + id);
    prefs.ClearUserPref(autoupdateErrDetailBaseString + id);
  } catch(Exception){}

  //Once we have deleted the prefs that can be deleted, we save the 
  //file if relevant, restart the scheduler, and once we are successful 
  //in doind that, we try to delete the crl 
  try{
    if(autoupdateParamAvailable){
      prefs.savePrefFile(null);
    }

    if(autoupdateEnabled){
      certdb.rescheduleCRLAutoUpdate();
    }
          
    // Now, try to delete it
    certdb.deleteCrl(i);
    DeleteItemSelected("crltree", "crltree_", "crlList");
    //To do: If delete fails, we should be able to retrieve the deleted
    //settings
    //XXXXXXXXXXXXXXXXXXXXX
  
  }catch(exception) {
    //To Do: Possibly show an error ...
    //XXXXXXXXXXXX
  }

  if( !crltree.selectedItems.length ) {
    if( !document.getElementById("deleteCrl").disabled ) {
      document.getElementById("deleteCrl").disabled = true;
      document.getElementById("editPrefs").disabled = true;
      document.getElementById("updateCRL").disabled = true;
    }
  }
}

function EnableCrlActions() {
  document.getElementById("deleteCrl").removeAttribute("disabled", "true");
  document.getElementById("editPrefs").removeAttribute("disabled", "true");
  document.getElementById("updateCRL").removeAttribute("disabled", "true");
}

function DeleteItemSelected(tree, prefix, kids) {
  var i;
  var delnarray = [];
  var rv = "";
  var cookietree = document.getElementById(tree);
  var selitems = cookietree.selectedItems;
  for(i = 0; i < selitems.length; i++) 
  { 
    delnarray[i] = document.getElementById(selitems[i].getAttribute("id"));
    var itemid = parseInt(selitems[i].getAttribute("id").substring(prefix.length,selitems[i].getAttribute("id").length));
    rv += (itemid + ",");
  }
  for(i = 0; i < delnarray.length; i++) 
  { 
    document.getElementById(kids).removeChild(delnarray[i]);
  }
  return rv;
}

function EditAutoUpdatePrefs() {
  var crlEntry;

  // delete selected item
  var crltree = document.getElementById("crltree");
  var i = crltree.selectedIndex;
  if(i<0){
    return;
  }
  crlEntry = crls.GetElementAt(i).QueryInterface(nsICrlEntry);
  var params = Components.classes[nsPKIParamBlock].createInstance(nsIPKIParamBlock);
  params.setISupportAtIndex(1, crlEntry);
  window.openDialog("chrome://pippki/content/pref-crlupdate.xul","",
                                   "chrome,resizable=1,modal=1,dialog=1",params);
}

function UpdateCRL()
{
  var crltree = document.getElementById("crltree");
  var i = crltree.selectedIndex;
  if(i<0){
    return;
  }
  crlEntry = crls.GetElementAt(i).QueryInterface(nsICrlEntry);
  certdb.updateCRLFromURL(crlEntry.lastFetchURL, crlEntry.nameInDb);
}
