/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
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
 * The Original Code is Mozilla Communicator client code, released March
 * 31, 1998.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation. Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *   Ben "Count XULula" Goodger
 */

/*** =================== INITIALISATION CODE =================== ***/

// globals 
var signonviewer        = null;
var signonList          = [];
var rejectList          = [];
var nopreviewList       = [];
var nocaptureList       = [];
var goneSS              = ""; // signon
var goneIS              = ""; // ignored site
var goneNP              = ""; // nopreview
var goneNC              = ""; // nocapture

// function : <SignonViewer.js>::Startup();
// purpose  : initialises interface, calls init functions for each page
function Startup()
{
  signonviewer = Components.classes["component://netscape/signonviewer/signonviewer-world"].createInstance();
  signonviewer = signonviewer.QueryInterface(Components.interfaces.nsISignonViewer);

  doSetOKCancel(onOK, null);  // init ok event handler

  // remove wallet functions (unless overruled by the "wallet.enabled" pref)
  try {
    pref = Components.classes['component://netscape/preferences'];
    pref = pref.getService();
    pref = pref.QueryInterface(Components.interfaces.nsIPref);
    try {
      if (!pref.GetBoolPref("wallet.enabled")) {
        var element;
        element = document.getElementById("nopreview");
        element.setAttribute("style","display: none;" );
        element = document.getElementById("nocapture");
        element.setAttribute("style","display: none;" );
      }
    } catch(e) {
      dump("wallet.enabled pref is missing from all.js");
    }
  } catch (ex) {
    dump("failed to get prefs service!\n");
    pref = null;
  }

  LoadSignons();
  LoadReject();
  LoadNopreview();
  LoadNocapture();
}

/*** =================== SAVED SIGNONS CODE =================== ***/

// function : <SignonViewer.js>::LoadSignons();
// purpose  : reads signons from interface and loads into tree
function LoadSignons()
{
  signonList = signonviewer.GetSignonValue();
  var delim = signonList[0];
  signonList = signonList.split(delim);
  for(var i = 1; i < signonList.length; i++)
  {
    var currSignon = TrimString(signonList[i]);
    // TEMP HACK until morse fixes signon viewer functions
    currSignon = RemoveHTMLFormatting(currSignon);
    var site = currSignon.substring(0,currSignon.lastIndexOf(":"));
    var user = currSignon.substring(currSignon.lastIndexOf(":")+1,currSignon.length);
    AddItem("savesignonlist",[site,user],"signon_",i-1);
  }
}

// function : <SignonViewer.js>::DeleteSignon();
// purpose  : deletes a particular signon
function DeleteSignon()
{
  goneSS += DeleteItemSelected('signonstree','signon_','savesignonlist');
  DoButtonEnabling("signonstree");
}

/*** =================== IGNORED SIGNONS CODE =================== ***/

// function : <SignonViewer.js>::LoadReject();
// purpose  : reads rejected sites from interface and loads into tree
function LoadReject()
{
  rejectList = signonviewer.GetRejectValue();
  var delim = rejectList[0];
  rejectList = rejectList.split(delim);
  for(var i = 1; i < rejectList.length; i++)
  {
    var currSignon = TrimString(rejectList[i]);
    // TEMP HACK until morse fixes signon viewer functions
    currSignon = RemoveHTMLFormatting(currSignon);
    var site = currSignon.substring(0,currSignon.lastIndexOf(":"));
    var user = currSignon.substring(currSignon.lastIndexOf(":")+1,currSignon.length);
    AddItem("ignoredlist",[site],"reject_",i-1);
  }
}

// function : <SignonViewer.js>::DeleteIgnoredSite();
// purpose  : deletes ignored site(s)
function DeleteIgnoredSite()
{
  goneIS += DeleteItemSelected('ignoretree','reject_','ignoredlist');
  DoButtonEnabling("ignoretree");
}

/*** =================== NO PREVIEW FORMS CODE =================== ***/

// function : <SignonViewer.js>::LoadNopreview();
// purpose  : reads non-previewed forms from interface and loads into tree
function LoadNopreview()
{
  nopreviewList = signonviewer.GetNopreviewValue();
  var delim = nopreviewList[0];
  nopreviewList = nopreviewList.split(delim);
  for(var i = 1; i < nopreviewList.length; i++)
  {
    var currSignon = TrimString(nopreviewList[i]);
    // TEMP HACK until morse fixes signon viewer functions
    currSignon = RemoveHTMLFormatting(currSignon);
    var form = currSignon.substring(currSignon.lastIndexOf(":")+1,currSignon.length);
    AddItem("nopreviewlist",[form],"nopreview_",i-1);
  }
}

// function : <SignonViewer>::DeleteNoPreviewForm()
// purpose  : deletes no-preview entry(s)
function DeleteNoPreviewForm()
{
  goneNP += DeleteItemSelected('nopreviewtree','nopreview_','nopreviewlist');
  DoButtonEnabling("nopreviewtree");
}

/*** =================== NO CAPTURE FORMS CODE =================== ***/

// function : <SignonViewer.js>::LoadNocapture();
// purpose  : reads non-captured forms from interface and loads into tree
function LoadNocapture()
{
  nocaptureList = signonviewer.GetNocaptureValue();
  var delim = nocaptureList[0];
  nocaptureList = nocaptureList.split(delim);
  for(var i = 1; i < nocaptureList.length; i++)
  {
    var currSignon = TrimString(nocaptureList[i]);
    // TEMP HACK until morse fixes signon viewer functions
    currSignon = RemoveHTMLFormatting(currSignon);
    var form = currSignon.substring(currSignon.lastIndexOf(":")+1,currSignon.length);
    AddItem("nocapturelist",[form],"nocapture_",i-1);
  }
}

// function : <SignonViewer>::DeleteNoCaptureForm()
// purpose  : deletes no-capture entry(s)
function DeleteNoCaptureForm()
{
  goneNC += DeleteItemSelected('nocapturetree','nocapture_','nocapturelist');
  DoButtonEnabling("nocapturetree");
}

/*** =================== GENERAL CODE =================== ***/

// function : <SignonViewer>::onOK()
// purpose  : dialog confirm & tidy up.
function onOK()
{
  var result = "|goneS|"+goneSS+"|goneR|"+goneIS;
  result += "|goneC|"+goneNC+"|goneP|"+goneNP+"|";
  signonviewer.SetValue(result, window);
  return true;
}

/*** =================== UTILITY FUNCTIONS =================== ***/

// function : <SignonViewer.js>::RemoveHTMLFormatting();
// purpose  : removes HTML formatting from input stream      
function RemoveHTMLFormatting(which)
{
  var ignoreon = false;
  var rv = "";
  for(var i = 0; i < which.length; i++)
  {
    if(which.charAt(i) == "<")
      ignoreon = true;
    if(which.charAt(i) == ">") {
      ignoreon = false;
      continue;
    }
    if(ignoreon)
      continue;
    rv += which.charAt(i);
  }
  return rv;
}

/*** =================== TREE MANAGEMENT CODE =================== ***/

// function : <SignonViewer.js>::AddItem();
// purpose  : utility function for adding items to a tree.
function AddItem(children,cells,prefix,idfier)
{
  var kids = document.getElementById(children);
  var item  = document.createElement("treeitem");
  var row   = document.createElement("treerow");
  for(var i = 0; i < cells.length; i++)
  {
    var cell  = document.createElement("treecell");
    cell.setAttribute("class", "propertylist");
    cell.setAttribute("value", cells[i])
    row.appendChild(cell);
  }
  item.appendChild(row);
  item.setAttribute("id",prefix + idfier);
  kids.appendChild(item);
}

// function : <SignonViewer.js>::DeleteItemSelected();
// purpose  : deletes all the signons that are selected
function DeleteItemSelected(tree, prefix, kids) {
  var delnarray = [];
  var rv = "";
  var cookietree = document.getElementById(tree);
  selitems = cookietree.selectedItems;
  for(var i = 0; i < selitems.length; i++) 
  { 
    delnarray[i] = document.getElementById(selitems[i].getAttribute("id"));
    var itemid = parseInt(selitems[i].getAttribute("id").substring(prefix.length,selitems[i].getAttribute("id").length));
    rv += (itemid + ",");
  }
  for(var i = 0; i < delnarray.length; i++) 
  { 
    document.getElementById(kids).removeChild(delnarray[i]);
  }
  return rv;
}

// function: <SignonViewer.js>::HandleKeyPress();
// purpose : handles keypress events 
function HandleEvent( event, page )
{
  // click event
  if( event.type == "click" ) {
    var node = event.target;
    while ( node.nodeName != "tree" )
      node = node.parentNode;
    var tree = node;
    DoButtonEnabling( node.id );
  }
  
  // keypress event
  if( event.type == "keypress" && event.which == 46 ) {
    switch( page ) {
    case 0:
      DeleteSignon();
      break;
    case 1:
      DeleteIgnoredSite();
      break;
    case 2:
      DeleteNoPreviewForm();
      break;
    case 2:
      DeleteNoCaptureForm();
      break;
    default:
      break;
    }
  }
}

function DoButtonEnabling( treeid )
{
  var tree = document.getElementById( treeid );
  switch( treeid ) {
  case "signonstree":
    var button = document.getElementById("removeSignon");
    break;
  case "ignoretree":
    var button = document.getElementById("removeIgnoredSite");
    break;
  case "nopreviewtree":
    var button = document.getElementById("removeNoPreview");
    break;
  case "nocapturetree":
    var button = document.getElementById("removeNoCapture");
    break;
  default:
    break;
  }
  if( button.getAttribute("disabled") && tree.selectedItems.length )
    button.removeAttribute("disabled", "true");
  else if( !button.getAttribute("disabled") && !tree.selectedItems.length )
    button.setAttribute("disabled","true");
}

// Remove whitespace from both ends of a string
function TrimString(string)
{
  if (!string) return "";
  return string.replace(/(^\s+)|(\s+$)/g, '')
}
