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
var tagName = "table"
var tableElement = null;
var rowElement = null;
var cellElement = null;
var maxRows = 10000;
var maxColumns = 10000;
var maxPixels = 10000;
var rows;
var columns;

// dialog initialization code
function Startup()
{
  if (!InitEditorShell())
    return;

  doSetOKCancel(onOK, null);

  dump(tagName+" = InsertTable tagName\n");
  tableElement = editorShell.CreateElementWithDefaults(tagName);
  if(!tableElement)
  {
    dump("Failed to create a new table!\n");
    window.close();
  }
  dump("Rows editbox:"+document.getElementById("rows")+"\n");
  dump("Columns editbox:"+document.getElementById("columns")+"\n");
  dump("width editbox:"+document.getElementById("width")+"\n");
  dump("pixelOrPercentSelect:"+document.getElementById("pixelOrPercentSelect")+"\n");
  dump("borderInput editbox:"+document.getElementById("borderInput")+"\n");

  // Create dialog object to store controls for easy access
  dialog = new Object;
  dialog.rowsInput = document.getElementById("rows");
  dialog.columnsInput = document.getElementById("columns");
  dialog.widthInput = document.getElementById("width");
  dialog.borderInput = document.getElementById("borderInput");
  dialog.pixelOrPercentSelect = document.getElementById("pixelOrPercentSelect");

  // Make a copy to use for AdvancedEdit
  globalElement = tableElement.cloneNode(false);
  
  // Initialize all widgets with image attributes
  InitDialog();

  // Set initial number to 1 row, 2 columns:
  // Note, these are not attributes on the table,
  //  so don't put them in InitDialog(),
  //  else the user's values will be trashed when they use 
  //  the Advanced Edit dialog
  dialog.rowsInput.value = 1;
  dialog.columnsInput.value = 2;

  dialog.rowsInput.focus();

  // Resize window
  window.sizeToContent();
}

// Set dialog widgets with attribute data
// We get them from globalElement copy so this can be used
//   by AdvancedEdit(), which is shared by all property dialogs
function InitDialog()
{  
  // Get default attributes set on the created table:
  // Get the width attribute of the element, stripping out "%"
  // This sets contents of select combobox list
  dialog.widthInput.value = InitPixelOrPercentCombobox(globalElement,"width","pixelOrPercentSelect");
  dialog.borderInput.value = globalElement.getAttribute("border");
}

// Get and validate data from widgets.
// Set attributes on globalElement so they can be accessed by AdvancedEdit()
function ValidateData()
{
  rows = ValidateNumberString(dialog.rowsInput.value, 1, maxRows);
  if (rows == "") {
    // Set focus to the offending control
    dialog.rowsInput.focus();
    return false;
  }

  columns = ValidateNumberString(dialog.columnsInput.value, 1, maxColumns);
  if (columns == "") {
    // Set focus to the offending control
    dialog.columnsInput.focus();
    return false;
  }

  // Set attributes: NOTE: These may be empty strings
  borderText = TrimString(dialog.borderInput.value);
  if (borderText) {
    // Set the other attributes on the table
    if (ValidateNumberString(borderText, 0, maxPixels))
      globalElement.setAttribute("border", borderText);
  }

  var isPercent = (dialog.pixelOrPercentSelect.selectedIndex == 1);
  widthText = TrimString(dialog.widthInput.value);
  if (widthText) {
    var maxLimit;
    if (isPercent) {
      maxLimit = 100;
    } else {
      // Upper limit when using pixels
      maxLimit = maxPixels;
    }

    widthText = ValidateNumberString(dialog.widthInput.value, 1, maxLimit);
    if (widthText) {
      if (isPercent)
        widthText += "%";
      globalElement.setAttribute("width", widthText);
    }
  }
  return true;
}


function onOK()
{
  if (ValidateData())
  {
    editorShell.CloneAttributes(tableElement, globalElement);

    var tableBody = editorShell.CreateElementWithDefaults("tbody");
    if (tableBody)
    {
      tableElement.appendChild(tableBody);

      // Create necessary rows and cells for the table
      dump("Rows = "+rows+"  Columns = "+columns+"\n");
      for (var i = 0; i < rows; i++)
      {
        var newRow = editorShell.CreateElementWithDefaults("tr");
        if (newRow)
        {
          tableBody.appendChild(newRow);
          for (var j = 0; j < columns; j++)
          {
            newCell = editorShell.CreateElementWithDefaults("td");
            if (newCell)
            {
              newRow.appendChild(newCell);
            }
          }
        }
      }
    }
    try {
      // Don't delete selected text when inserting
      editorShell.InsertElementAtSelection(tableElement, false);
    } catch (e) {
      dump("Exception occured in InsertElementAtSelection\n");
    }
    return true;
  }
  return false;
}
