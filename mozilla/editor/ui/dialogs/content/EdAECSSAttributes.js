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

// build attribute list in tree form from element attributes
function BuildCSSAttributeTable()
{
  // get the CSS declaration from DOM 2 ElementCSSInlineStyle
  var style = gElement.style;

  if (style == undefined)
  {
    dump("Inline styles undefined\n");
    return;
  }

  var l = style.length;
  if (l == undefined || l == 0)
  {
    if (l == undefined) {
      dump("Failed to query the number of inline style declarations\n");
    }
    return;
  }

  if (l > 0)
  {
    var added = false;
    for (i = 0; i < l; i++)
    {
      name = style.item(i);
      value = style.getPropertyValue(name);
      AddTreeItem( name, value, "CSSAList", CSSAttrs );
    }
  }

  ClearCSSInputWidgets();
}

// add or select attribute in the tree widget
function onChangeCSSAttribute()
{
  var name = TrimString(dialog.AddCSSAttributeNameInput.value);
  if ( !name )
    return;

  var value = TrimString(dialog.AddCSSAttributeValueInput.value);

  // First try to update existing attribute
  // If not found, add new attribute
  if ( !UpdateExistingAttribute( name, value, "CSSAList" ) )
    AddTreeItem( name, value, "CSSAList", CSSAttrs );
}

function ClearCSSInputWidgets()
{
  dialog.AddCSSAttributeTree.clearItemSelection();
  dialog.AddCSSAttributeNameInput.value ="";
  dialog.AddCSSAttributeValueInput.value = "";
  dialog.AddCSSAttributeNameInput.inputField.focus();
}

function onSelectCSSTreeItem()
{
  if (!gDoOnSelectTree)
    return;

  var tree = dialog.AddCSSAttributeTree;
  if (tree && tree.selectedItems && tree.selectedItems.length)
  {
    dialog.AddCSSAttributeNameInput.value = GetTreeItemAttributeStr(tree.selectedItems[0]);
    dialog.AddCSSAttributeValueInput.value = GetTreeItemValueStr(tree.selectedItems[0]);
  }
}

function onInputCSSAttributeName()
{
  var attName = TrimString(dialog.AddCSSAttributeNameInput.value).toLowerCase();
  var newValue = "";

  var existingValue = GetAndSelectExistingAttributeValue(attName, "CSSAList");
  if (existingValue)
    newValue = existingValue;

  dialog.AddCSSAttributeValueInput.value = newValue;
}

function onInputCSSAttributeValue()
{
  // Update value in the tree list
  UpdateExistingAttribute( dialog.AddCSSAttributeNameInput.value,
                           dialog.AddCSSAttributeValueInput.value,
                           "CSSAList" );
}

function editCSSAttributeValue(targetCell)
{
  if (IsNotTreeHeader(targetCell))
    dialog.AddCSSAttributeValueInput.inputField.select();
}

function UpdateCSSAttributes()
{
  var CSSAList = document.getElementById("CSSAList");
  var styleString = "";
  for(var i = 0; i < CSSAList.childNodes.length; i++)
  {
    var item = CSSAList.childNodes[i];
    var name = GetTreeItemAttributeStr(item);
    var value = GetTreeItemValueStr(item);
    // this code allows users to be sloppy in typing in values, and enter
    // things like "foo: " and "bar;". This will trim off everything after the
    // respective character.
    if (name.indexOf(":") != -1)
      name = name.substring(0,name.lastIndexOf(":"));
    if (value.indexOf(";") != -1)
      value = value.substring(0,value.lastIndexOf(";"));
    if (i == (CSSAList.childNodes.length - 1))
      styleString += name + ": " + value + ";";   // last property
    else
      styleString += name + ": " + value + "; ";
  }
  if (styleString.length > 0)
  {
    gElement.removeAttribute("style");
    gElement.setAttribute("style",styleString);  // NOTE BUG 18894!!!
  } 
  else if (gElement.getAttribute("style"))
    gElement.removeAttribute("style");
}

function RemoveCSSAttribute()
{
  var treechildren = dialog.AddCSSAttributeTree.lastChild;

  // We only allow 1 selected item
  if (dialog.AddCSSAttributeTree.selectedItems.length)
  {
    var item = dialog.AddCSSAttributeTree.selectedItems[0];

    // Remove the item from the tree
    // We always rebuild complete "style" string,
    //  so no list of "removed" items 
    treechildren.removeChild (item);

    ClearCSSInputWidgets();
  }
}

function SelectCSSTree( index )
{
  gDoOnSelectTree = false;
  try {
    dialog.AddCSSAttributeTree.selectedIndex = index;
  } catch (e) {}
  gDoOnSelectTree = true;
}
