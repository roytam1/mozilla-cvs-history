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
 * Copyright (C) 1998-1999 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s):
 *   Pete Collins
 *   Brian King
 */
/*
  if we ever need to use a different string bundle, use srGetStrBundle
  by including
  <script type="application/x-javascript" src="chrome://global/content/strres.js"/>
  e.g.:
  var bundle = srGetStrBundle("chrome://global/locale/filepicker.properties");
*/

// Each editor window must include this file
// Variables  shared by all dialogs:
var editorShell;

// Object to attach commonly-used widgets (all dialogs should use this)
var gDialog = {};

var gValidationError = false;

// Use for 'defaultIndex' param in InitPixelOrPercentMenulist
var gPixel = 0;
var gPercent = 1;

var maxPixels  = 10000;
// For dialogs that expand in size. Default is smaller size see "onMoreFewer()" below
var SeeMore = false;

// A XUL element with id="location" for managing
// dialog location relative to parent window
var gLocation;

// The element being edited - so AdvancedEdit can have access to it
var globalElement;

function InitEditorShell()
{
    // get the editor shell from the parent window

  editorShell = window.opener.editorShell;
  if (editorShell) {
    editorShell = editorShell.QueryInterface(Components.interfaces.nsIEditorShell);
  }
  if (!editorShell) {
    dump("EditorShell not found!!!\n");
    window.close();
    return false;
  }

  // Save as a property of the window so it can be used by child dialogs

  window.editorShell = editorShell;

  return true;
}

/* Validate contents of an input field 
 *
 *  inputWidget    The 'textbox' XUL element for text input of the attribute's value
 *  listWidget     The 'menulist' XUL element for choosing "pixel" or "percent"
 *                  May be null when no pixel/percent is used.
 *  minVal         minimum allowed for input widget's value
 *  maxVal         maximum allowed for input widget's value
 *                 (when "listWidget" is used, maxVal is used for "pixel" maximum,
 *                  100% is assumed if "percent" is the user's choice)
 *  element        The DOM element that we set the attribute on. May be null.
 *  attName        Name of the attribute to set.  May be null or ignored if "element" is null
 *  mustHaveValue  If true, error dialog is displayed if "value" is empty string
 *
 *  This calls "ValidateNumberRange()", which puts up an error dialog to inform the user. 
 *    If error, we also: 
 *      Shift focus and select contents of the inputWidget,
 *      Switch to appropriate panel of tabbed dialog if user implements "SwitchToValidate()",
 *      and/or will expand the dialog to full size if "More / Fewer" feature is implemented
 *
 *  Returns the "value" as a string, or "" if error or input contents are empty
 *  The global "gValidationError" variable is set true if error was found
 */
function ValidateNumber(inputWidget, listWidget, minVal, maxVal, element, attName, mustHaveValue, mustShowMoreSection)
{
  if (!inputWidget)
  {
    gValidationError = true;
    return "";
  }

  // Global error return value
  gValidationError = false;
  var maxLimit = maxVal;
  var isPercent = false;

  var numString = TrimString(inputWidget.value);
  if (numString || mustHaveValue)
  {
    if (listWidget)
      isPercent = (listWidget.selectedIndex == 1);
    if (isPercent)
      maxLimit = 100;

    // This method puts up the error message
    numString = ValidateNumberRange(numString, minVal, maxLimit, mustHaveValue);
    if(!numString)
    {
      // Switch to appropriate panel for error reporting
      SwitchToValidatePanel();

      // or expand dialog for users of "More / Fewer" button
      if ("dialog" in window && dialog && 
           "MoreSection" in gDialog && gDialog.MoreSection)
      {
        if ( !SeeMore )
          onMoreFewer();
      }

      // Error - shift to offending input widget
      SetTextboxFocus(inputWidget);
      gValidationError = true;
    }
    else
    {
      if (isPercent)
        numString += "%";
      if (element)
        element.setAttribute(attName, numString);
    }
  } else if (element) {
    element.removeAttribute(attName);
  }
  return numString;
}

/* Validate contents of an input field 
 *
 *  value          number to validate
 *  minVal         minimum allowed for input widget's value
 *  maxVal         maximum allowed for input widget's value
 *                 (when "listWidget" is used, maxVal is used for "pixel" maximum,
 *                  100% is assumed if "percent" is the user's choice)
 *  mustHaveValue  If true, error dialog is displayed if "value" is empty string
 *
 *  If inputWidget's value is outside of range, or is empty when "mustHaveValue" = true,
 *      an error dialog is popuped up to inform the user. The focus is shifted
 *      to the inputWidget.
 *
 *  Returns the "value" as a string, or "" if error or input contents are empty
 *  The global "gValidationError" variable is set true if error was found
 */
function ValidateNumberRange(value, minValue, maxValue, mustHaveValue)
{
  // Initialize global error flag
  gValidationError = false;
  value = TrimString(String(value));

  // We don't show error for empty string unless caller wants to
  if (!value && !mustHaveValue)
    return "";

  var numberStr = "";

  if (value.length > 0)
  {
    // Extract just numeric characters
    var number = Number(value.replace(/\D+/g, ""));
    if (number >= minValue && number <= maxValue )
    {
      // Return string version of the number
      return String(number);
    }
    numberStr = String(number);
  }

  var message = "";

  if (numberStr.length > 0)
  {
    // We have a number from user outside of allowed range
    message = editorShell.GetString( "ValidateRangeMsg");
    message = message.replace(/%n%/, numberStr);
    message += "\n ";
  }
  message += editorShell.GetString( "ValidateNumberMsg");

  // Replace variable placeholders in message with number values
  message = message.replace(/%min%/, minValue).replace(/%max%/, maxValue);
  ShowInputErrorMessage(message);

  // Return an empty string to indicate error
  gValidationError = true;
  return "";
}

function SetTextboxFocusById(id)
{
  SetTextboxFocus(document.getElementById(id));
}

function SetTextboxFocus(textbox)
{
  if (textbox)
  {
    // Select entire contents
    if (textbox.value.length > 0)
      textbox.select();
    else
      textbox.focus();
  }
}

function ShowInputErrorMessage(message)
{
  AlertWithTitle(GetString("InputError"), message);
  window.focus();
}

// Get the text appropriate to parent container
//  to determine what a "%" value is refering to.
// elementForAtt is element we are actually setting attributes on
//  (a temporary copy of element in the doc to allow canceling),
//  but elementInDoc is needed to find parent context in document
function GetAppropriatePercentString(elementForAtt, elementInDoc)
{
  var name = elementForAtt.nodeName.toLowerCase();
  if ( name == "td" || name == "th")
    return GetString("PercentOfTable");

  // Check if element is within a table cell
  if (editorShell.GetElementOrParentByTagName("td", elementInDoc))
    return GetString("PercentOfCell");
  else
    return GetString("PercentOfWindow");
}

function AppendStringToMenulistById(menulist, stringID)
{
  return AppendStringToMenulist(menulist, editorShell.GetString(stringID));
}

function AppendStringToMenulist(menulist, string)
{
  if (menulist)
  {
    var menupopup = menulist.firstChild;
    // May not have any popup yet -- so create one
    if (!menupopup)
    {
      menupopup = document.createElementNS(XUL_NS, "menupopup");
      if (menupopup)
        menulist.appendChild(menupopup);
      else
      {
        dump("Failed to create menupoup\n");
        return null;
      }
    }
    var menuItem = document.createElementNS(XUL_NS, "menuitem");
    if (menuItem)
    {
      menuItem.setAttribute("label", string);
      menupopup.appendChild(menuItem);
      return menuItem;
    }
  }
  return null;
}

function AppendLabelAndValueToMenulist(menulist, labelStr, valueStr)
{
  if (menulist)
  {
    var menupopup = menulist.firstChild;
    // May not have any popup yet -- so create one
    if (!menupopup)
    {
      menupopup = document.createElementNS(XUL_NS, "menupopup");
      if (menupopup)
        menulist.appendChild(menupopup);
      else
      {
        dump("Failed to create menupoup\n");
        return null;
      }
    }
    var menuItem = document.createElementNS(XUL_NS, "menuitem");
    if (menuItem)
    {
      menuItem.setAttribute("label", labelStr);
      menuItem.setAttribute("value", valueStr);
      menupopup.appendChild(menuItem);
      return menuItem;
    }
  }
  return null;
}

function ClearMenulist(menulist)
{
  // Always use "AppendStringToMenulist" so we know there's 
  //  just one <menupopup> as 1st child of <menulist>
  if (menulist) {
    menulist.selectedItem = null;
    var popup = menulist.firstChild;
    if (popup)
      while (popup.firstChild)
        popup.removeChild(popup.firstChild);
  }
}

/* These help using a <tree> for simple lists
  Assumes this simple structure:
  <tree>
    <treecolgroup><treecol flex="1"/></treecolgroup>
    <treechildren>
      <treeitem>
        <treerow>
          <treecell label="the text the user sees"/>
*/

function AppendStringToTreelistById(tree, stringID)
{
  return AppendStringToTreelist(tree, editorShell.GetString(stringID));
}

function AppendStringToTreelist(tree, string)
{
  if (tree)
  {
    var treecols = tree.firstChild;
    if (!treecols)
    {
      dump("Bad XUL: Must have <treecolgroup> as first child\n");
    }
    var treechildren = treecols.nextSibling;
    if (!treechildren)
    {
      treechildren = document.createElementNS(XUL_NS, "treechildren");
      if (treechildren)
      {
        treechildren.setAttribute("flex","1");
        tree.appendChild(treechildren);
      }
      else
      {
        dump("Failed to create <treechildren>\n");
        return null;
      }
    }
    var treeitem = document.createElementNS(XUL_NS, "treeitem");
    var treerow = document.createElementNS(XUL_NS, "treerow");
    var treecell = document.createElementNS(XUL_NS, "treecell");
    if (treeitem && treerow && treecell)
    {
      treerow.appendChild(treecell);
      treeitem.appendChild(treerow);
      treechildren.appendChild(treeitem)
      treecell.setAttribute("label", string);
      //var len = Number(tree.getAttribute("length"));
      //if (!len) len = -1;
      tree.setAttribute("length", treechildren.childNodes.length);
      return treeitem;
    }
  }
  return null;
}

function ReplaceStringInTreeList(tree, index, string)
{
  if (tree)
  {
    var treecols = tree.firstChild;
    if (!treecols)
    {
      dump("Bad XUL: Must have <treecolgroup> as first child\n");
      return;
    }
    var treechildren = treecols.nextSibling;
    if (!treechildren)
      return;

    // Each list item is a <treeitem><treerow><treecell> under <treechildren> node
    var childNodes = treechildren.childNodes;
    if (!childNodes || index >= childNodes.length)
      return;

    var row = childNodes.item(index).firstChild;
    if (row && row.firstChild)
    {
      row.firstChild.setAttribute("label", string);
    }
  }
}

function ClearTreelist(tree)
{
  if (tree)
  {
    tree.clearItemSelection();
    // Skip over the first <treecolgroup> child
    if (tree.firstChild)
    {
      var nextChild = tree.firstChild.nextSibling;
      while (nextChild)
      {
        var nextTmp = nextChild.nextSibling;
        tree.removeChild(nextChild);
        nextChild = nextTmp;
      }
    }
    // Count list items
    tree.setAttribute("length", 0);
  }
}

function GetSelectedTreelistAttribute(tree, attr)
{
  if (tree)
  {
    if (tree.selectedIndex >= 0 &&
        tree.selectedItems.length > 0 &&
        tree.selectedItems[0] &&
        tree.selectedItems[0].firstChild &&
        tree.selectedItems[0].firstChild.firstChild)
    {
      return tree.selectedItems[0].firstChild.firstChild.getAttribute(attr);
    }
  }
  return "";
}

function GetSelectedTreelistValue(tree)
{
  return GetSelectedTreelistAttribute(tree,"label")
}

function RemoveSelectedTreelistItem(tree)
{
  if (tree)
  {
    if (tree.selectedIndex >= 0 &&
        tree.selectedItems.length > 0)
    {
      // Get the node to delete
      var treeItem = tree.selectedItems[0];
      if (treeItem)
      {
        tree.clearItemSelection();
        var parent = treeItem.parentNode;
        if (parent)
        {
          parent.removeChild(treeItem);
          tree.setAttribute("length", parent.childNodes.length);
        }
      }
    }
  }
}

function GetTreelistValueAt(tree, index)
{
  if (tree)
  {
    var item = tree.getItemAtIndex(index);
    if (item && item.firstChild && item.firstChild.firstChild)
      return item.firstChild.firstChild.getAttribute("label");
  }
  return "";
}

function forceInteger(elementID)
{
  var editField = document.getElementById( elementID );
  if ( !editField )
    return;

  var stringIn = editField.value;
  if (stringIn && stringIn.length > 0)
  {
    // Strip out all nonnumeric characters
    stringIn = stringIn.replace(/\D+/g,"");
    if (!stringIn) stringIn = "";

    // Write back only if changed
    if (stringIn != editField.value)
      editField.value = stringIn;
  }
}

function LimitStringLength(elementID, length)
{
  var editField = document.getElementById( elementID );
  if ( !editField )
    return;

  var stringIn = editField.value;
  if (stringIn && stringIn.length > length)
    editField.value = stringIn.slice(0,length);
}

function InitPixelOrPercentMenulist(elementForAtt, elementInDoc, attribute, menulistID, defaultIndex)
{
  if (!defaultIndex) defaultIndex = gPixel;

  var size  = elementForAtt.getAttribute(attribute);
  var menulist = document.getElementById(menulistID);
  var pixelItem;
  var percentItem;

  if (!menulist)
  {
    dump("NO MENULIST found for ID="+menulistID+"\n");
    return size;
  }

  ClearMenulist(menulist);
  pixelItem = AppendStringToMenulist(menulist, GetString("Pixels"));

  if (!pixelItem) return 0;

  percentItem = AppendStringToMenulist(menulist, GetAppropriatePercentString(elementForAtt, elementInDoc));
  if (size && size.length > 0)
  {
    // Search for a "%" character
    var percentIndex = size.search(/%/);
    if (percentIndex > 0)
    {
      // Strip out the %
      size = size.substr(0, percentIndex);
      if (percentItem)
        menulist.selectedItem = percentItem;
    }
    else
      menulist.selectedItem = pixelItem;
  }
  else
    menulist.selectedIndex = defaultIndex;

  return size;
}

function onAdvancedEdit()
{
  // First validate data from widgets in the "simpler" property dialog
  if (ValidateData())
  {
    // Set true if OK is clicked in the Advanced Edit dialog
    window.AdvancedEditOK = false;
    // Open the AdvancedEdit dialog, passing in the element to be edited
    //  (the copy named "globalElement")
    window.openDialog("chrome://editor/content/EdAdvancedEdit.xul", "_blank", "chrome,close,titlebar,modal,resizable=yes", "", globalElement);
    window.focus();
    if (window.AdvancedEditOK)
    {
      // Copy edited attributes to the dialog widgets:
      InitDialog();
    }
  }
}

function getColor(ColorPickerID)
{
  var colorPicker = document.getElementById(ColorPickerID);
  var color;
  if (colorPicker)
  {
    // Extract color from colorPicker and assign to colorWell.
    color = colorPicker.getAttribute("color");
    if (color && color == "")
      return null;
    // Clear color so next if it's called again before
    //  color picker is actually used, we dedect the "don't set color" state
    colorPicker.setAttribute("color","");
  }

  return color;
}

function setColorWell(ColorWellID, color)
{
  var colorWell = document.getElementById(ColorWellID);
  if (colorWell)
  {
    if (!color || color == "")
    {
      // Don't set color (use default)
      // Trigger change to not show color swatch
      colorWell.setAttribute("default","true");
      // Style in CSS sets "background-color",
      //   but color won't clear unless we do this:
      colorWell.removeAttribute("style");
    }
    else
    {
      colorWell.removeAttribute("default");
      // Use setAttribute so colorwell can be a XUL element, such as button
      colorWell.setAttribute("style", "background-color:"+color);
    }
  }
}

function getColorAndSetColorWell(ColorPickerID, ColorWellID)
{
  var color = getColor(ColorPickerID);
  setColorWell(ColorWellID, color);
  return color;
}

// Test for valid image by sniffing out the extension
function IsValidImage(imageName)
{
  var image = TrimString(imageName);
  if ( !image )
    return false;

  /* look for an extension */
  var tailindex = image.lastIndexOf(".");
  if ( tailindex == 0 || tailindex == -1 ) /* -1 is not found */
    return false;

  /* move past period, get the substring from the first character after the '.' to the last character (length) */
  tailindex = tailindex + 1;
  var type = image.substring(tailindex,image.length);

  /* convert extension to lower case */
  if (type)
    type = type.toLowerCase();

  // TODO: Will we convert .BMPs to a web format?
  switch( type )
  {
    case "gif":
    case "jpg":
    case "jpeg":
    case "png":
      return true;
      break;
  }
  return false;
}

function InitMoreFewer()
{
  // Set SeeMore bool to the OPPOSITE of the current state,
  //   which is automatically saved by using the 'persist="more"'
  //   attribute on the gDialog.MoreFewerButton button
  //   onMoreFewer will toggle it and redraw the dialog
  SeeMore = (gDialog.MoreFewerButton.getAttribute("more") != "1");
  onMoreFewer();
}

function onMoreFewer()
{
  if (SeeMore)
  {
    gDialog.MoreSection.setAttribute("collapsed","true");
    window.sizeToContent();
    gDialog.MoreFewerButton.setAttribute("more","0");
    gDialog.MoreFewerButton.setAttribute("label",GetString("MoreProperties"));
    SeeMore = false;
  }
  else
  {
    gDialog.MoreSection.removeAttribute("collapsed");
    window.sizeToContent();
    gDialog.MoreFewerButton.setAttribute("more","1");
    gDialog.MoreFewerButton.setAttribute("label",GetString("FewerProperties"));
    SeeMore = true;
  }
}

function SwitchToValidatePanel()
{
  // no default implementation
  // Only EdTableProps.js currently implements this
}

const nsIFilePicker = Components.interfaces.nsIFilePicker;

function GetLocalFileURL(filterType)
{
  var fp = Components.classes["@mozilla.org/filepicker;1"].createInstance(nsIFilePicker);

  if (filterType == "img")
  {
    fp.init(window, GetString("SelectImageFile"), nsIFilePicker.modeOpen);
    fp.appendFilters(nsIFilePicker.filterImages);
  }
  // Current usage of this is in Link dialog,
  //  where we always want HTML first
  else if (filterType.indexOf("html") == 0)
  {
    fp.init(window, GetString("OpenHTMLFile"), nsIFilePicker.modeOpen);

    // When loading into Composer, direct user to prefer HTML files and text files,
    //   so we call separately to control the order of the filter list
    fp.appendFilters(nsIFilePicker.filterHTML);
    fp.appendFilters(nsIFilePicker.filterText);

    // Link dialog also allows linking to images
    if (filterType.indexOf("img") > 0)
      fp.appendFilters(nsIFilePicker.filterImages);

  }
  // Default or last filter is "All Files"
  fp.appendFilters(nsIFilePicker.filterAll);

  /* doesn't handle *.shtml files */
  try {
    var ret = fp.show();
    if (ret == nsIFilePicker.returnCancel)
      return null;
  }
  catch (ex) {
    dump("filePicker.chooseInputFile threw an exception\n");
    return null;
  }

  return fp.fileURL.spec;
}

function GetMetaElement(name)
{
  if (name)
  {
    name = name.toLowerCase();
    if (name != "")
    {
      var metaNodes = editorShell.editorDocument.getElementsByTagName("meta");
      if (metaNodes && metaNodes.length > 0)
      {
        for (var i = 0; i < metaNodes.length; i++)
        {
          var metaNode = metaNodes.item(i);
          if (metaNode && metaNode.getAttribute("name") == name)
            return metaNode;
        }
      }
    }
  }
  return null;
}

function CreateMetaElement(name)
{
  var metaElement = editorShell.CreateElementWithDefaults("meta");
  if (metaElement)
    metaElement.setAttribute("name", name);
  else
    dump("Failed to create metaElement!\n");

  return metaElement;
}

function GetHTTPEquivMetaElement(name)
{
  if (name)
  {
    name = name.toLowerCase();
    if (name != "")
    {
      var metaNodes = editorShell.editorDocument.getElementsByTagName("meta");
      if (metaNodes && metaNodes.length > 0)
      {
        for (var i = 0; i < metaNodes.length; i++)
        {
          var metaNode = metaNodes.item(i);
          if (metaNode)
          {
            var httpEquiv = metaNode.getAttribute("http-equiv");
            if (httpEquiv && httpEquiv.toLowerCase() == name)
              return metaNode;
          }
        }
      }
    }
  }
  return null;
}

function CreateHTTPEquivMetaElement(name)
{
  metaElement = editorShell.CreateElementWithDefaults("meta");
  if (metaElement)
    metaElement.setAttribute("http-equiv", name);
  else
    dump("Failed to create httpequivMetaElement!\n");

  return metaElement;
}

function CreateHTTPEquivElement(name)
{
  metaElement = editorShell.CreateElementWithDefaults("meta");
  if (metaElement)
    metaElement.setAttribute("http-equiv", name);
  else
    dump("Failed to create metaElement for http-equiv!\n");

  return metaElement;
}

// Change "content" attribute on a META element,
//   or delete entire element it if content is empty
// This uses undoable editor transactions
function SetMetaElementContent(metaElement, content, insertNew)
{
  if (metaElement)
  {
    if(!content || content == "")
    {
      if (!insertNew)
        editorShell.DeleteElement(metaElement);
    }
    else
    {
      if (insertNew)
      {
        metaElement.setAttribute("content", content);
        AppendHeadElement(metaElement);
      }
      else
        editorShell.SetAttribute(metaElement, "content", content);
    }
  }
}

function GetHeadElement()
{
  var headList = editorShell.editorDocument.getElementsByTagName("head");
  if (headList)
    return headList.item(0);

  return null;
}

function AppendHeadElement(element)
{
  var head = GetHeadElement();
  if (head)
  {
    var position = 0;
    if (head.hasChildNodes())
      position = head.childNodes.length;

    // Use editor's undoable transaction
    // Last param "true" says "don't change the selection"
    editorShell.InsertElement(element, head, position, true);
  }
}

function SetWindowLocation()
{
  gLocation = document.getElementById("location");
  if (gLocation)
  {
    window.screenX = Math.max(0, Math.min(window.opener.screenX + Number(gLocation.getAttribute("offsetX")),
                                          screen.availWidth - window.outerWidth));
    window.screenY = Math.max(0, Math.min(window.opener.screenY + Number(gLocation.getAttribute("offsetY")),
                                          screen.availHeight - window.outerHeight));
  }
}

function SaveWindowLocation()
{
  if (gLocation)
  {
    var newOffsetX = window.screenX - window.opener.screenX;
    var newOffsetY = window.screenY - window.opener.screenY;
    gLocation.setAttribute("offsetX", window.screenX - window.opener.screenX);
    gLocation.setAttribute("offsetY", window.screenY - window.opener.screenY);
  }
}

function onCancel()
{
  SaveWindowLocation();
  // Close dialog by returning true
  return true;
}

function SetRelativeCheckbox()
{
  var checkbox = document.getElementById("MakeRelativeCheckbox");
  if (!checkbox)
    return;

  // Mail never allows relative URLs, so hide the checkbox
  if (editorShell.editorType == "htmlmail")
  {
    checkbox.setAttribute("collapsed", "true");
    return;
  }

  var input =  document.getElementById(checkbox.getAttribute("for"));
  if (!input)
    return;

  var url = TrimString(input.value);
  var urlScheme = GetScheme(url);

  // Check it if url is relative (no scheme).
  checkbox.checked = url.length > 0 && !urlScheme;

  // Now do checkbox enabling:
  var enable = false;

  var docUrl = GetDocumentBaseUrl();
  var docScheme = GetScheme(docUrl);

  if (url && docUrl && docScheme)
  {
    if (urlScheme)
    {
      // Url is absolute
      // If we can make a relative URL, then enable must be true!
      // (this lets the smarts of MakeRelativeUrl do all the hard work)
      enable = (GetScheme(MakeRelativeUrl(url)).length == 0);
    }
    else
    {
      // Url is relative
      // Check if url is a named anchor
      //  but document doesn't have a filename
      // (it's probably "index.html" or "index.htm",
      //  but we don't want to allow a malformed URL)
      if (url[0] == "#")
      {
        var docFilename = GetFilename(docUrl);
        enable = docFilename.length > 0;
      }
      else
      {
        // Any other url is assumed 
        //  to be ok to try to make absolute
        enable = true;
      }
    }
  }

  SetElementEnabledById("MakeRelativeCheckbox", enable);
}

// oncommand handler for the Relativize checkbox in EditorOverlay.xul
function MakeInputValueRelativeOrAbsolute()
{
  var checkbox = document.getElementById("MakeRelativeCheckbox");
  if (!checkbox)
    return;

  var input =  document.getElementById(checkbox.getAttribute("for"));
  if (!input)
    return;

  var docUrl = GetDocumentBaseUrl();
  if (!docUrl)
  {
    // Checkbox should be disabled if not saved,
    //  but keep this error message in case we change that
    AlertWithTitle("", GetString("SaveToUseRelativeUrl"));
    window.focus();
  }
  else 
  {
    // Note that "checked" is opposite of its last state,
    //  which determines what we want to do here
    if (checkbox.checked)
      input.value = MakeRelativeUrl(input.value);
    else
      input.value = MakeAbsoluteUrl(input.value);

    // Reset checkbox to reflect url state
    SetRelativeCheckbox(checkbox, input.value);
  }
}

