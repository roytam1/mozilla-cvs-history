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

  doSetOKCancel(onOK, onCancel);

  tableElement = editorShell.CreateElementWithDefaults(tagName);
  if(!tableElement)
  {
    dump("Failed to create a new table!\n");
    window.close();
    return;
  }
  gDialog.rowsInput    = document.getElementById("rowsInput");
  gDialog.columnsInput = document.getElementById("columnsInput");
  gDialog.widthInput = document.getElementById("widthInput");
  gDialog.borderInput = document.getElementById("borderInput");
  gDialog.widthPixelOrPercentMenulist = document.getElementById("widthPixelOrPercentMenulist");

  // Make a copy to use for AdvancedEdit
  globalElement = tableElement.cloneNode(false);
  
  // Initialize all widgets with image attributes
  InitDialog();

  // Set initial number to 2 rows, 2 columns:
  // Note, these are not attributes on the table,
  //  so don't put them in InitDialog(),
  //  else the user's values will be trashed when they use 
  //  the Advanced Edit dialog
  gDialog.rowsInput.value = 2;
  gDialog.columnsInput.value = 2;

  // If no default value on the width, set to 100%
  if (gDialog.widthInput.value.length == 0)
  {
    gDialog.widthInput.value = "100";
    gDialog.widthPixelOrPercentMenulist.selectedIndex = 1;
  }

  SetTextboxFocusById("rowsInput");

  SetWindowLocation();
}

// Set dialog widgets with attribute data
// We get them from globalElement copy so this can be used
//   by AdvancedEdit(), which is shared by all property dialogs
function InitDialog()
{  
  // Get default attributes set on the created table:
  // Get the width attribute of the element, stripping out "%"
  // This sets contents of menu combobox list
  // 2nd param = null: Use current selection to find if parent is table cell or window
  gDialog.widthInput.value = InitPixelOrPercentMenulist(globalElement, null, "width", "widthPixelOrPercentMenulist", gPercent);
  gDialog.borderInput.value = globalElement.getAttribute("border");
}

function ChangeRowOrColumn(id)
{
  // Allow only integers
  forceInteger(id);

  // Enable OK only if both rows and columns have a value > 0
  var enable = gDialog.rowsInput.value.length > 0 && 
                              gDialog.rowsInput.value > 0 &&
                              gDialog.columnsInput.value.length > 0 &&
                              gDialog.columnsInput.value > 0;

  SetElementEnabledById("ok", enable);
  SetElementEnabledById("AdvancedEditButton1", enable);
}


// Get and validate data from widgets.
// Set attributes on globalElement so they can be accessed by AdvancedEdit()
function ValidateData()
{
  rows = ValidateNumber(gDialog.rowsInput, null, 1, maxRows, null, null, true)
  if (gValidationError)
    return false;

  columns = ValidateNumber(gDialog.columnsInput, null, 1, maxColumns, null, null, true)
  if (gValidationError)
    return false;

  // Set attributes: NOTE: These may be empty strings (last param = false)
  ValidateNumber(gDialog.borderInput, null, 0, maxPixels, globalElement, "border", false);
  // TODO: Deal with "BORDER" without value issue
  if (gValidationError) return false;

  ValidateNumber(gDialog.widthInput, gDialog.widthPixelOrPercentMenulist,
                 1, maxPixels, globalElement, "width", false);
  if (gValidationError)
    return false;

  return true;
}


function onOK()
{
  if (ValidateData())
  {
    editorShell.BeginBatchChanges();
    editorShell.CloneAttributes(tableElement, globalElement);

    // Create necessary rows and cells for the table
    var tableBody = editorShell.CreateElementWithDefaults("tbody");
    if (tableBody)
    {
      tableElement.appendChild(tableBody);

      // Create necessary rows and cells for the table
      for (var i = 0; i < rows; i++)
      {
        var newRow = editorShell.CreateElementWithDefaults("tr");
        if (newRow)
        {
          tableBody.appendChild(newRow);
          for (var j = 0; j < columns; j++)
          {
            var newCell = editorShell.CreateElementWithDefaults("td");
            if (newCell)
            {
              newRow.appendChild(newCell);
            }
          }
        }
      }
    }
    // Detect when entire cells are selected:
      // Get number of cells selected
    var tagNameObj = new Object;
    var countObj = new Object;
    tagNameObj.value = "";
    var element = editorShell.GetSelectedOrParentTableElement(tagNameObj, countObj);
    var deletePlaceholder = false;

    if (tagNameObj.value == "table")
    {
      //Replace entire selected table with new table, so delete the table
      editorShell.DeleteTable();
    }
    else if (tagNameObj.value == "td")
    {
      if (countObj.value >= 1)
      {
        if (countObj.value > 1)
        {
          // Assume user wants to replace a block of
          //  contiguous cells with a table, so
          //  join the selected cells
          editorShell.JoinTableCells(false);
          
          // Get the cell everything was merged into
          element = editorShell.GetFirstSelectedCell();
          
          // Collapse selection into just that cell
          editorShell.editorSelection.collapse(element,0);
        }

        if (element)
        {
          // Empty just the contents of the cell
          editorShell.DeleteTableCellContents();
          
          // Collapse selection to start of empty cell...
          editorShell.editorSelection.collapse(element,0);
          // ...but it will contain a <br> placeholder
          deletePlaceholder = true;
        }
      }
    }

    try {
      // true means delete selection when inserting
      editorShell.InsertElementAtSelection(tableElement, true);
    } catch (e) {
      dump("Exception occured in InsertElementAtSelection\n");
    }
    if (deletePlaceholder && tableElement && tableElement.nextSibling)
    {
      // Delete the placeholder <br>
      editorShell.DeleteElement(tableElement.nextSibling);
    }

    editorShell.EndBatchChanges();

    SaveWindowLocation();
    return true;
  }
  return false;
}
