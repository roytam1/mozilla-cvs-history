/*
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
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation. Portions created by Netscape are
 * Copyright (C) 2001 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s):
 */

var gPublishSiteData;
var gPublishDataChanged = false;
var gDefaultSiteIndex = -1;
var gDefaultSiteName;
var gPreviousDefaultSite;
var gPreviousTitle;
var gSettingsChanged = false;
var gSiteDataChanged = false;
var gNewSite = false;

// Dialog initialization code
function Startup()
{
  if (!InitEditorShell()) return;

  gDialog.SiteList            = document.getElementById("SiteList");
  gDialog.SiteNameInput       = document.getElementById("SiteNameInput");
  gDialog.PublishUrlInput     = document.getElementById("PublishUrlInput");
  gDialog.BrowseUrlInput      = document.getElementById("BrowseUrlInput");
  gDialog.UsernameInput       = document.getElementById("UsernameInput");
  gDialog.PasswordInput       = document.getElementById("PasswordInput");
  gDialog.SavePassword        = document.getElementById("SavePassword");
  gDialog.SetDefaultButton    = document.getElementById("SetDefaultButton");
  gDialog.RemoveSiteButton    = document.getElementById("RemoveSiteButton");
  gDialog.OkButton            = document.documentElement.getButton("accept");

  gPublishSiteData = GetPublishSiteData();
  gDefaultSiteName = GetDefaultPublishSiteName();
  gPreviousDefaultSite = gDefaultSiteName;

  InitDialog();

  SetWindowLocation();
}

function InitDialog()
{
  // If there's no current site data, start a new item in the Settings panel
  if (!gPublishSiteData)
  {
    AddNewSite();
  }
  else
  {
    FillSiteList();
    InitSiteSettings(gDefaultSiteIndex);
    SetTextboxFocus(gDialog.SiteNameInput);
  }
}

function FillSiteList()
{
  ClearListbox(gDialog.SiteList);
  gDefaultSiteIndex = -1;

  // Fill the site list
  var count = gPublishSiteData.length;
  for (var i = 0; i < count; i++)
  {
    var name = gPublishSiteData[i].siteName;
    var item = gDialog.SiteList.appendItem(name);
    SetPublishItemStyle(item);
    if (name == gDefaultSiteName)
      gDefaultSiteIndex = i;
  }
}

function SetPublishItemStyle(item)
{
  // Add a cell before the text to display a check for default site
  if (item)
  {
    if (item.label == gDefaultSiteName)
      item.setAttribute("class", "bold");
    else
      item.removeAttribute("class");
  }
}

function AddNewSite()
{
  // Save any pending changes locally first
  if (gSettingsChanged && !UpdateSettings())
    return;

  // Initialize Setting widgets to none of the selected sites
  InitSiteSettings(-1);
  gNewSite = true;

  SetTextboxFocus(gDialog.SiteNameInput);
}

function RemoveSite()
{
  if (!gPublishSiteData)
    return;

  var index = gDialog.SiteList.selectedIndex;
  var item;
  if (index != -1)
  {
    item = gDialog.SiteList.selectedItems[0];

    // Remove one item from site data array
    gPublishSiteData.splice(index, 1);
    // Remove item from site list
    gDialog.SiteList.clearSelection();
    gDialog.SiteList.removeItemAt(index);

    // Adjust if we removed last item and reselect a site
    if (index >= gPublishSiteData.length)
      index--;
    InitSiteSettings(index);

    gSiteDataChanged = true;
  }
}

function SetDefault()
{
  if (!gPublishSiteData)
    return;

  var index = gDialog.SiteList.selectedIndex;
  if (index != -1)
  {
    gDefaultSiteIndex = index;
    gDefaultSiteName = gPublishSiteData[index].siteName;
    
    // Set bold style on new default
    var item = gDialog.SiteList.firstChild;
    while (item)
    {
      SetPublishItemStyle(item);
      item = item.nextSibling;
    }
  }
}

// Recursion prevention: InitSiteSettings() changes selected item
var gIsSelecting = false;

function SelectSiteList()
{
  if (gIsSelecting)
    return;

  gIsSelecting = true;

  // Save any pending changes locally first
  if (gSettingsChanged && !UpdateSettings())
    return;

  InitSiteSettings(gDialog.SiteList.selectedIndex);

  gIsSelecting = false;
}

function InitSiteSettings(selectedSiteIndex)
{
  var savePassord = false;

  var haveData = (gPublishSiteData && selectedSiteIndex != -1);
  gDialog.SiteList.selectedIndex = selectedSiteIndex;

  gDialog.SiteNameInput.value = haveData ? gPublishSiteData[selectedSiteIndex].siteName : "";
  gDialog.PublishUrlInput.value = haveData ? gPublishSiteData[selectedSiteIndex].publishUrl : "";
  gDialog.BrowseUrlInput.value = haveData ? gPublishSiteData[selectedSiteIndex].browseUrl : "";
  gDialog.UsernameInput.value = haveData ? gPublishSiteData[selectedSiteIndex].username : "";
  gDialog.PasswordInput.value = haveData ? gPublishSiteData[selectedSiteIndex].password : "";
  gDialog.SavePassword.checked = haveData ? gPublishSiteData[selectedSiteIndex].savePassword : false;

  gDialog.SetDefaultButton.disabled = !haveData;
  gDialog.RemoveSiteButton.disabled = !haveData;
  gSettingsChanged = false;
}

function onInputSettings()
{
  // TODO: Save current data during SelectSite1 and compare here
  //       to detect if real change has occurred?
  gSettingsChanged = true;
}

function UpdateSettings()
{
  // Validate and add new site
  var newName = TrimString(gDialog.SiteNameInput.value);
  if (!newName)
  {
    ShowInputErrorMessage(GetString("MissingSiteNameError"), gDialog.SiteNameInput);
    return false;
  }
  var newUrl = FormatUrlForPublishing(gDialog.PublishUrlInput.value);
  if (!newUrl)
  {
    ShowInputErrorMessage(GetString("MissingPublishUrlError"), gDialog.PublishUrlInput);
    return false;
  }

  var siteIndex = -1;
  if (!gPublishSiteData)
  {
    // Create the first site profile
    gPublishSiteData = new Array(1);
    siteIndex = 0;
    gNewSite = true;
  }
  else
  {
    // Add new data at the end of list
    siteIndex = gPublishSiteData.length;
  }
    
  if (gNewSite || gDialog.SiteList.selectedIndex == -1)
  {
    // Init new site object
    gPublishSiteData[siteIndex] = {};
    gPublishSiteData[siteIndex].docDir = "/";
    gPublishSiteData[siteIndex].otherDir = "/";
    gPublishSiteData[siteIndex].dirList = ["/"];
  }
  else
  {
    // Update existing site profile
    siteIndex = gDialog.SiteList.selectedIndex;
  }

  gPublishSiteData[siteIndex].siteName = newName;
  gPublishSiteData[siteIndex].publishUrl = newUrl;
  gPublishSiteData[siteIndex].browseUrl = FormatUrlForPublishing(gDialog.BrowseUrlInput.value);
  gPublishSiteData[siteIndex].username = TrimString(gDialog.UsernameInput.value);
  gPublishSiteData[siteIndex].password= gDialog.PasswordInput.value;
  gPublishSiteData[siteIndex].savePassword = gDialog.SavePassword.checked;

  if (siteIndex == gDefaultSiteIndex)
    gDefaultSiteName = newName;

  var count = gPublishSiteData.length;
  if (count > 1)
  {
    // XXX Ascii sort, not locale-aware
    gPublishSiteData.sort();

    //Find previous items in sorted list
    for (var i = 0; i < count; i++)
    {
      if (gPublishSiteData[i].siteName == newName)
      {
        siteIndex = i;
        break;
      }
    }
  }

  // When adding the very first site, assume that's the default
  if (count == 1 && !gDefaultSiteName)
  {
    gDefaultSiteName = gPublishSiteData[0].siteName;
    gDefaultSiteIndex = 0;
  }

  FillSiteList();
  gDialog.SiteList.selectedIndex = siteIndex;

  // Signal saving data to prefs
  gSiteDataChanged = true;
  
  // Clear current site flags
  gSettingsChanged = false;
  gNewSite = false;

  return true;
}

function doHelpButton()
{
  openHelp("chrome://help/content/help.xul?site_settings");
}

function onAccept()
{
  // Save any pending changes locally first
  if (gSettingsChanged && !UpdateSettings())
    return false;

  if (gSiteDataChanged)
  {
    // Save all local data to prefs
    SavePublishSiteDataToPrefs(gPublishSiteData, gDefaultSiteName);
  }
  else if (gPreviousDefaultSite != gDefaultSiteName)
  {
    // only the default site was changed
    SetDefaultSiteName(gDefaultSiteName);
  }

  SaveWindowLocation();

  return true;
}
