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
 *   Daniel Glazman <glazman@netscape.com>
 */

// build attribute list in tree form from element attributes
function BuildCSSAttributeTable()
{
  // we can't trust DOM 2 ElementCSSInlineStyle because gElement can be
  // outside of the document's tree
  var styleAttr = gElement.getAttribute("style");
  var editor = editorShell.editor.QueryInterface(Components.interfaces.nsIHTMLEditor);
  var styleRule = editor.ParseStyleAttrIntoCSSRule(styleAttr);
  
  if (styleRule == undefined)
  {
    dump("Inline styles undefined\n");
    return;
  }

  var style = styleRule.style;
  var declLength = style.length;

  if (declLength == undefined || declLength == 0)
  {
    if (declLength == undefined) {
      dump("Failed to query the number of inline style declarations\n");
    }

    return;
  }

  if (declLength > 0)
  {
    var added = false;
    for (i = 0; i < declLength; i++)
    {
      name = style.item(i);
      value = style.getPropertyValue(name);
      AddTreeItem( name, value, "CSSAList", CSSAttrs );
    }
  }

  ClearCSSInputWidgets();
}

function onChangeCSSAttribute()
{
  var name = TrimString(gDialog.AddCSSAttributeNameInput.value);
  if ( !name )
    return;

  var value = TrimString(gDialog.AddCSSAttributeValueInput.value);

  // First try to update existing attribute
  // If not found, add new attribute
  if ( !UpdateExistingAttribute( name, value, "CSSAList" ) && value)
    AddTreeItem( name, value, "CSSAList", CSSAttrs );
}

function ClearCSSInputWidgets()
{
  gDialog.AddCSSAttributeTree.clearSelection();
  gDialog.AddCSSAttributeNameInput.value ="";
  gDialog.AddCSSAttributeValueInput.value = "";
  gDialog.AddCSSAttributeNameInput.inputField.focus();
}

function onSelectCSSTreeItem()
{
  if (!gDoOnSelectTree)
    return;

  var tree = gDialog.AddCSSAttributeTree;
  if (tree && tree.selectedItems && tree.selectedItems.length)
  {
    gDialog.AddCSSAttributeNameInput.value = GetTreeItemAttributeStr(tree.selectedItems[0]);
    gDialog.AddCSSAttributeValueInput.value = GetTreeItemValueStr(tree.selectedItems[0]);
  }
}

function onInputCSSAttributeName()
{
  var attName = TrimString(gDialog.AddCSSAttributeNameInput.value).toLowerCase();
  var newValue = "";

  var existingValue = GetAndSelectExistingAttributeValue(attName, "CSSAList");
  if (existingValue)
    newValue = existingValue;

  gDialog.AddCSSAttributeValueInput.value = newValue;
}

function editCSSAttributeValue(targetCell)
{
  if (IsNotTreeHeader(targetCell))
    gDialog.AddCSSAttributeValueInput.inputField.select();
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
  if (styleString)
  {
    // Use editor transactions if modifying the element directly in the document
    doRemoveAttribute("style");
    doSetAttribute("style", styleString);  // NOTE BUG 18894!!!
  } 
  else if (gElement.getAttribute("style"))
    doRemoveAttribute("style");
}

function RemoveCSSAttribute()
{
  var treechildren = gDialog.AddCSSAttributeTree.lastChild;

  // We only allow 1 selected item
  if (gDialog.AddCSSAttributeTree.selectedItems.length)
  {
    var item = gDialog.AddCSSAttributeTree.selectedItems[0];

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
    gDialog.AddCSSAttributeTree.selectedIndex = index;
  } catch (e) {}
  gDoOnSelectTree = true;
}
