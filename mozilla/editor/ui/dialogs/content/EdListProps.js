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
 */


//Cancel() is in EdDialogCommon.js
var insertNew = true;
var tagname = "TAG NAME"
var ListTypeList;
var BulletStyleList;
var BulletStyleLabel;
var StartingNumberInput;
var StartingNumberLabel;
var BulletStyleIndex = 0;
var NumberStyleIndex = 0;
var ListElement = 0;
var ListType = "";
var AdvancedEditButton;

// dialog initialization code
function Startup()
{
  if (!InitEditorShell())
    return;

  doSetOKCancel(onOK, null);

  ListTypeList = document.getElementById("ListType");
  BulletStyleList = document.getElementById("BulletStyle");
  BulletStyleLabel = document.getElementById("BulletStyleLabel");
  StartingNumberInput = document.getElementById("StartingNumber");
  StartingNumberLabel = document.getElementById("StartingNumberLabel");
  AdvancedEditButton = document.getElementById("AdvancedEditButton");
  
  // Try to get an existing list
  ListElement = editorShell.GetElementOrParentByTagName("list",null);
  
  // The copy to use in AdvancedEdit
  if (ListElement)
    globalElement = ListElement.cloneNode(false);

  //dump("List and global elements: "+ListElement+globalElement+"\n");

  InitDialog();

  ListTypeList.focus();
}

function InitDialog()
{
  if (ListElement)
    ListType = ListElement.nodeName.toLowerCase();
  else
    ListType = "";

  BuildBulletStyleList();
}

function BuildBulletStyleList()
{
  ClearList(BulletStyleList);
  var label = "";
  var selectedIndex = -1;

  //dump("List Type: "+ListType+" globalElement: "+globalElement+"\n");

  if (ListType == "ul")
  {
    BulletStyleList.removeAttribute("disabled");
    BulletStyleLabel.removeAttribute("disabled");
    StartingNumberInput.setAttribute("disabled", "true");
    StartingNumberLabel.setAttribute("disabled", "true");
    label = GetString("BulletStyle");

    AppendStringToListByID(BulletStyleList,"SolidCircle");
    AppendStringToListByID(BulletStyleList,"OpenCircle");
    AppendStringToListByID(BulletStyleList,"SolidSquare");

    BulletStyleList.selectedIndex = BulletStyleIndex;
    ListTypeList.selectedIndex = 1;
  }
  else if (ListType == "ol")
  {
    BulletStyleList.removeAttribute("disabled");
    BulletStyleLabel.removeAttribute("disabled");
    StartingNumberInput.removeAttribute("disabled");
    StartingNumberLabel.removeAttribute("disabled");
    label = GetString("NumberStyle");

    AppendStringToListByID(BulletStyleList,"Style_1");
    AppendStringToListByID(BulletStyleList,"Style_I");
    AppendStringToListByID(BulletStyleList,"Style_i");
    AppendStringToListByID(BulletStyleList,"Style_A");
    AppendStringToListByID(BulletStyleList,"Style_a");

    BulletStyleList.selectedIndex = NumberStyleIndex;
    ListTypeList.selectedIndex = 2;
  } 
  else 
  {
    BulletStyleList.setAttribute("disabled", "true");
    BulletStyleLabel.setAttribute("disabled", "true");
    StartingNumberInput.setAttribute("disabled", "true");
    StartingNumberLabel.setAttribute("disabled", "true");

    if (ListType == "dl")
      ListTypeList.selectedIndex = 3;
    else
      ListTypeList.selectedIndex = 0;
  }
  
  // Disable advanced edit button if changing to "normal"
  if (ListType == "")
    AdvancedEditButton.setAttribute("disabled","true");
  else
    AdvancedEditButton.removeAttribute("disabled");

  if (label)
  {
    BulletStyleLabel.value = label;
    if (BulletStyleLabel.hasChildNodes())
    {
      //dump("BulletStyleLabel.firstChild: "+BulletStyleLabel.firstChild+"\n");
      BulletStyleLabel.removeChild(BulletStyleLabel.firstChild);
    }
    
    var textNode = document.createTextNode(label);
    BulletStyleLabel.appendChild(textNode);
  }
}

function SelectListType()
{
  switch (ListTypeList.selectedIndex)
  {
    case 1:
      NewType = "ul";
      break;
    case 2:
      NewType = "ol";
      break;
    case 3:
      NewType = "dl";
      break;
    default:
      NewType = "";
      break;
  }
  if (ListType != NewType)
  {
    ListType = NewType;
    
    // Create a newlist object for Advanced Editing
    if (ListType != "")
      globalElement = editorShell.CreateElementWithDefaults(ListType);

    BuildBulletStyleList();
  }
}

function SelectBulletStyle()
{
  // Save the selected index so when user changes
  //   list style, restore index to associated list
  if (ListType == "ul")
    BulletStyleIndex = BulletStyleList.selectedIndex;
  else if (ListType == "ol")
    NumberStyleIndex = BulletStyleList.selectedIndex;
}

function ValidateData()
{
  var type = 0;
  // globalElement should already be of the correct type 
  //dump("Global List element="+globalElement+"  should be type: "+ListType+"\n");

  if (globalElement)
  {
    if (ListType == "ul")
    {
      switch (BulletStyleList.selectedIndex)
      {
        // Index 0 = "disc", the default, so we don't set it explicitly
        case 1:
          type = "circle";
          break;
        case 2:
          type = "square";
          break;
      }
      if (type)
        globalElement.setAttribute("type",type);
      else
        globalElement.removeAttribute("type");

    } else if (ListType == "ol")
    {
      switch (BulletStyleList.selectedIndex)
      {
        // Index 0 = "1", the default, so we don't set it explicitly
        case 1:
          type = "I";
          break;
        case 2:
          type = "i";
          break;
        case 3:
          type = "A";
          break;
        case 4:
          type = "a";
          break;
      }
      if (type)
        globalElement.setAttribute("type",type);
      else
        globalElement.removeAttribute("type");
        
      var startingNumber = StartingNumberInput.value.trimString();
      if (startingNumber)
        globalElement.setAttribute("start",startingNumber);
      else
        globalElement.removeAttribute("start");
    }
  }
  return true;
}

function onOK()
{
  if (ValidateData())
  {
    // Coalesce into one undo transaction
    editorShell.BeginBatchChanges();

    // Making a list is tricky!
    // First, make the list
    editorShell.MakeOrChangeList(ListType);

    // Now we need to get ALL of the list nodes in the current selection
    // For now, let's get just the one at the anchor
    listElement = editorShell.GetElementOrParentByTagName("list",null);
    // Set the list attributes
    if (listElement)
      editorShell.CloneAttributes(listElement, globalElement);

    editorShell.EndBatchChanges();
    return true;
  }
  return false;
}
