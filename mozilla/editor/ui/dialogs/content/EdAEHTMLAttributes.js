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
 *   Ben "Count XULula" Goodger
 */

function BuildHTMLAttributeNameList()
{
  ClearMenulist(dialog.AddHTMLAttributeNameInput);
  
  var elementName = gElement.localName.toLowerCase();
  var attNames = gHTMLAttr[elementName];

  if (attNames && attNames.length)
  {
    var forceOneChar = false;
    var forceInteger = false;

    for (var i = 0; i < attNames.length; i++)
    {
      var name = attNames[i];
      if (name == "_core")
      {
        // Signal to append the common 'core' attributes.
        // Note: These currently don't have any filtering
        for (var j = 0; j < gCoreHTMLAttr.length; j++)
          AppendStringToMenulist(dialog.AddHTMLAttributeNameInput, gCoreHTMLAttr[j]);

      }
      else if (name == "-")
      {
        // Signal for separator
        var popup = dialog.AddHTMLAttributeNameInput.firstChild;
        if (popup)
        {
          sep = document.createElementNS("http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul", "menuseparator");
          if (sep)
            popup.appendChild(sep);
        }        
      }
      else
      {
        // Get information about value filtering
        forceOneChar = name.indexOf("!") >= 0;
        forceInteger = name.indexOf("#") >= 0;
        forceIntOrPercent = name.indexOf("%") >= 0;
        //var required = name.indexOf("$") >= 0;

        // Strip flag characters ("_" is used when attribute name is reserved JS word)
        name = name.replace(/[!#%$_]/g, "");

        var menuitem = AppendStringToMenulist(dialog.AddHTMLAttributeNameInput, name);
        if (menuitem)
        {
          // Signify "required" attributes by special style
          //TODO: Don't do this until next version, when we add
          //      explanatory text and an 'Autofill Required Attributes' button
          //if (required)
          //  menuitem.setAttribute("class", "menuitem-highlight-1");

          // Set flags to filter value input
          menuitem.setAttribute("forceOneChar", forceOneChar ? "true" : "");
          menuitem.setAttribute("forceInteger", forceInteger ? "true" : "");
          menuitem.setAttribute("forceIntOrPercent", forceIntOrPercent ? "true" : "");
        }
      }
    }
  }

  // Always start with empty input fields
  ClearHTMLInputWidgets();
}

// build attribute list in tree form from element attributes
function BuildHTMLAttributeTable()
{
  var nodeMap = gElement.attributes;
  var i;
  if (nodeMap.length > 0) 
  {
    var added = false;
    for(i = 0; i < nodeMap.length; i++)
    {
      if ( CheckAttributeNameSimilarity( nodeMap[i].nodeName, HTMLAttrs ) ||
          IsEventHandler( nodeMap[i].nodeName ) ||
          TrimString( nodeMap[i].nodeName.toLowerCase() ) == "style" ) {
        continue;   // repeated or non-HTML attribute, ignore this one and go to next
      }
      var name  = nodeMap[i].nodeName.toLowerCase();
      var value = gElement.getAttribute ( nodeMap[i].nodeName );
      if ( name.indexOf("_moz") != 0 &&
           AddTreeItem(name, value, "HTMLAList", HTMLAttrs) )
        added = true;
    }

    if (added)
      SelectHTMLTree(0);
  }
}

// add or select an attribute in the tree widget
function onChangeHTMLAttribute()
{
  var name  = TrimString(dialog.AddHTMLAttributeNameInput.value);
  if (!name)
    return;

  var value = TrimString(dialog.AddHTMLAttributeValueInput.value);

  // First try to update existing attribute
  // If not found, add new attribute
  if ( !UpdateExistingAttribute( name, value, "HTMLAList" ) )
    AddTreeItem ( name, value, "HTMLAList", HTMLAttrs );
}

function ClearHTMLInputWidgets()
{
  dialog.AddHTMLAttributeTree.clearItemSelection();
  dialog.AddHTMLAttributeNameInput.value ="";
  dialog.AddHTMLAttributeValueInput.value = "";
  dialog.AddHTMLAttributeNameInput.inputField.focus();
}

function onSelectHTMLTreeItem()
{
  if (!gDoOnSelectTree)
    return;

  var tree = dialog.AddHTMLAttributeTree;
  if (tree && tree.selectedItems && tree.selectedItems.length)
  {
    var inputName = TrimString(dialog.AddHTMLAttributeNameInput.value).toLowerCase();
    var selectedName = tree.selectedItems[0].firstChild.firstChild.getAttribute("label");

    if (inputName == selectedName)
    {
      // Already editing selected name - just update the value input
      dialog.AddHTMLAttributeValueInput.value =  GetTreeItemValueStr(tree.selectedItems[0]);
    }
    else
    {
      dialog.AddHTMLAttributeNameInput.value =  selectedName;

      // Change value input based on new selected name
      onInputHTMLAttributeName();
    }
  }
}

function onInputHTMLAttributeName()
{
  var attName = TrimString(dialog.AddHTMLAttributeNameInput.value).toLowerCase();

  // Clear value widget, but prevent triggering update in tree
  gUpdateTreeValue = false;
  dialog.AddHTMLAttributeValueInput.value = "";
  gUpdateTreeValue = true; 

  if (attName)
  {
    // Get value list for current attribute name
    var valueList = gHTMLAttr[gElement.localName.toLowerCase() + "_" + attName];

    // Index to which widget we were using to edit the value
    var deckIndex = dialog.AddHTMLAttributeValueDeck.getAttribute("index");
    var newValue = "";
    var listLen = 0;
    if (valueList)
    {
      listLen = valueList.length;
      if (listLen > 0)
        newValue = valueList[0];

      // Note: For case where "value list" is actually just 
      // one (default) item, don't use menulist for that
      if (listLen > 1)
      {
        ClearMenulist(dialog.AddHTMLAttributeValueMenulist);

        if (deckIndex != "1")
        {
          // Switch to using editable menulist
          dialog.AddHTMLAttributeValueInput = dialog.AddHTMLAttributeValueMenulist;
          dialog.AddHTMLAttributeValueDeck.setAttribute("index", "1");
        }
        // Rebuild the list
        for (var i = 0; i < listLen; i++)
          AppendStringToMenulist(dialog.AddHTMLAttributeValueMenulist, valueList[i]);
      }
    }
    
    if (listLen <= 1 && deckIndex != "0")
    {
      // No list: Use textbox for input instead
      dialog.AddHTMLAttributeValueInput = dialog.AddHTMLAttributeValueTextbox;
      dialog.AddHTMLAttributeValueDeck.setAttribute("index", "0");
    }

    // If attribute already exists in tree, use associated value,
    //  else use default found above
    var existingValue = GetAndSelectExistingAttributeValue(attName, "HTMLAList");
    if (existingValue)
      newValue = existingValue;
      
    dialog.AddHTMLAttributeValueInput.value = newValue;
  }
}

function onInputHTMLAttributeValue()
{
  if (!gUpdateTreeValue)
    return;

  // Trim spaces only from left since we must allow spaces within the string
  //  (we always reset the input field's value below)
  var value = TrimStringLeft(dialog.AddHTMLAttributeValueInput.value);
  if (value)
  {
    // Do value filtering based on type of attribute
    // (Do not use "LimitStringLength()" and "forceInteger()"
    //  to avoid multiple reseting of input's value and flickering)
    var selectedItem = dialog.AddHTMLAttributeNameInput.selectedItem;
    if (selectedItem)
    {
      if ( selectedItem.getAttribute("forceOneChar") == "true" &&
           value.length > 1 )
        value = value.slice(0, 1);

      if ( selectedItem.getAttribute("forceIntOrPercent") == "true" )
      {
        // Allow integer with optional "%" as last character
        var percent = TrimStringRight(value).slice(-1);
        value = value.replace(/\D+/g,"");
        if (percent == "%")
          value += percent;
      }
      else if ( selectedItem.getAttribute("forceInteger") == "true" )
      {
        value = value.replace(/\D+/g,"");
      }

      // Update once only if it changed
      if ( value != dialog.AddHTMLAttributeValueInput.value )
        dialog.AddHTMLAttributeValueInput.value = value;
    }
  }

  // Always update value in the tree list
  UpdateExistingAttribute( dialog.AddHTMLAttributeNameInput.value, value, "HTMLAList" );
}

function editHTMLAttributeValue(targetCell)
{
  if (IsNotTreeHeader(targetCell))
    dialog.AddHTMLAttributeValueInput.inputField.select();
}


// update the object with added and removed attributes
function UpdateHTMLAttributes()
{
  var HTMLAList = document.getElementById("HTMLAList");
  var i;

  // remove removed attributes
  for( i = 0; i < HTMLRAttrs.length; i++ )
  {
    var name = HTMLRAttrs[i];

    // We can't use getAttribute to figure out if attribute already
    //  exists for attributes that don't require a value
    // This gets the attribute NODE from the attributes NamedNodeMap
    if (gElement.attributes.getNamedItem(name))
      gElement.removeAttribute(name);
  }

  // Set added or changed attributes
  for( i = 0; i < HTMLAList.childNodes.length; i++)
  {
    var item = HTMLAList.childNodes[i];
    gElement.setAttribute( GetTreeItemAttributeStr(item), GetTreeItemValueStr(item) );
  }
}

function RemoveHTMLAttribute()
{
  var treechildren = dialog.AddHTMLAttributeTree.lastChild;

  // We only allow 1 selected item
  if (dialog.AddHTMLAttributeTree.selectedItems.length)
  {
    var item = dialog.AddHTMLAttributeTree.selectedItems[0];
    var attr = GetTreeItemAttributeStr(item);

    // remove the item from the attribute array
    HTMLRAttrs[HTMLRAttrs.length] = attr;
    RemoveNameFromAttArray(attr, HTMLAttrs);

    // Remove the item from the tree
    treechildren.removeChild(item);

    // Clear inputs and selected item in tree
    ClearHTMLInputWidgets();
  }
}

function SelectHTMLTree( index )
{

  gDoOnSelectTree = false;
  try {
    dialog.AddHTMLAttributeTree.selectedIndex = index;
  } catch (e) {}
  gDoOnSelectTree = true;
}
