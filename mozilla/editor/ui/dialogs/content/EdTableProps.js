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
 *    Ben Goodger
 */

//Cancel() is in EdDialogCommon.js
var tagname = "table"
var TableElement;
var CellElement;
var TableCaptionElement;
var TabPanel;
var dialog;
var globalCellElement;
var globalTableElement
var TablePanel = 0;
var CellPanel = 1;
var currentPanel = TablePanel;
var validatePanel;
var defHAlign =   "left";
var centerStr =   "center";  //Index=1
var rightStr =    "right";   // 2
var justifyStr =  "justify"; // 3
var charStr =     "char";    // 4
var defVAlign =   "middle";
var topStr =      "top";
var bottomStr =   "bottom";
var bgcolor = "bgcolor";

var rowCount = 1;
var colCount = 1;
var lastRowIndex;
var lastColIndex;
var newRowCount;
var newColCount;
var curRowIndex;
var curColIndex;
var SelectedCellsType = 1;
var SELECT_CELL = 1;
var SELECT_ROW = 2;
var SELECT_COLUMN = 3;
/*
From C++:
 0 TABLESELECTION_TABLE
 1 TABLESELECTION_CELL   There are 1 or more cells selected
                          but complete rows or columns are not selected
 2 TABLESELECTION_ROW    All cells are in 1 or more rows
                          and in each row, all cells selected
                          Note: This is the value if all rows (thus all cells) are selected
 3 TABLESELECTION_COLUMN All cells are in 1 or more columns
*/

var selectedCellCount = 0;
var error = 0;
var ApplyUsed = false;
// What should these be?
var maxRows    = 10000;
var maxColumns = 10000;
var selection;
var CellDataChanged = false;

// dialog initialization code
function Startup()
{
  if (!InitEditorShell()) return;

  selection = editorShell.editorSelection;
  if (!selection) return;

  dialog = new Object;
  if (!dialog)
  {
    dump("Failed to create dialog object!!!\n");
    window.close();
  }
  // Get dialog widgets - Table Panel
  dialog.TableRowsInput = document.getElementById("TableRowsInput");
  dialog.TableColumnsInput = document.getElementById("TableColumnsInput");
  dialog.TableHeightInput = document.getElementById("TableHeightInput");
  dialog.TableHeightUnits = document.getElementById("TableHeightUnits");
  dialog.TableWidthInput = document.getElementById("TableWidthInput");
  dialog.TableWidthUnits = document.getElementById("TableWidthUnits");
  dialog.BorderWidthInput = document.getElementById("BorderWidthInput");
  dialog.SpacingInput = document.getElementById("SpacingInput");
  dialog.PaddingInput = document.getElementById("PaddingInput");
  dialog.TableAlignList = document.getElementById("TableAlignList");
  dialog.TableCaptionList = document.getElementById("TableCaptionList");
  dialog.TableInheritColor = document.getElementById("TableInheritColor");
  dialog.TableImageInput = document.getElementById("TableImageInput");
  dialog.TableImageButton = document.getElementById("TableImageButton");

  dialog.RowsCheckbox = document.getElementById("RowsCheckbox");
  dialog.ColumnsCheckbox = document.getElementById("ColumnsCheckbox");
  dialog.TableHeightCheckbox = document.getElementById("TableHeightCheckbox");
  dialog.TableWidthCheckbox = document.getElementById("TableWidthCheckbox");
  dialog.TableBorderCheckbox = document.getElementById("TableBorderCheckbox");
  dialog.CellSpacingCheckbox = document.getElementById("CellSpacingCheckbox");
  dialog.CellPaddingCheckbox = document.getElementById("CellPaddingCheckbox");
  dialog.TableHAlignCheckbox = document.getElementById("TableHAlignCheckbox");
  dialog.TableCaptionCheckbox = document.getElementById("TableCaptionCheckbox");
  dialog.TableColorCheckbox = document.getElementById("TableColorCheckbox");
  dialog.TableImageCheckbox = document.getElementById("TableImageCheckbox");

  // Cell Panel
  dialog.SelectionList = document.getElementById("SelectionList");
  dialog.PreviousButton = document.getElementById("PreviousButton");
  dialog.NextButton = document.getElementById("NextButton");
  dialog.ApplyBeforeMove =  document.getElementById("ApplyBeforeMove");
  dialog.KeepCurrentData = document.getElementById("KeepCurrentData");

  dialog.CellHeightInput = document.getElementById("CellHeightInput");
  dialog.CellHeightUnits = document.getElementById("CellHeightUnits");
  dialog.CellWidthInput = document.getElementById("CellWidthInput");
  dialog.CellWidthUnits = document.getElementById("CellWidthUnits");
  dialog.RowSpanInput = document.getElementById("RowSpanInput");
  dialog.ColSpanInput = document.getElementById("ColSpanInput");
  dialog.CellHAlignList = document.getElementById("CellHAlignList");
  dialog.CellAlignCharInput = document.getElementById("CellAlignCharInput");
  dialog.CellVAlignList = document.getElementById("CellVAlignList");
  dialog.CellStyleList = document.getElementById("CellStyleList");
  dialog.TextWrapList = document.getElementById("TextWrapList");
  dialog.CellInheritColor = document.getElementById("CellInheritColor");
  dialog.CellImageInput = document.getElementById("CellImageInput");
  dialog.CellImageButton = document.getElementById("CellImageButton");

  dialog.CellHeightCheckbox = document.getElementById("CellHeightCheckbox");
  dialog.CellWidthCheckbox = document.getElementById("CellWidthCheckbox");
  dialog.SpanCheckbox = document.getElementById("SpanCheckbox");
  dialog.CellHAlignCheckbox = document.getElementById("CellHAlignCheckbox");
  dialog.CellVAlignCheckbox = document.getElementById("CellVAlignCheckbox");
  dialog.CellStyleCheckbox = document.getElementById("CellStyleCheckbox");
  dialog.TextWrapCheckbox = document.getElementById("TextWrapCheckbox");
  dialog.CellColorCheckbox = document.getElementById("CellColorCheckbox");
  dialog.CellImageCheckbox = document.getElementById("CellImageCheckbox");

  TabPanel = document.getElementById("TabPanel");
  var TableTab = document.getElementById("TableTab");
  var CellTab = document.getElementById("CellTab");

  TableElement = editorShell.GetElementOrParentByTagName("table", null);
  if(!TableElement)
  {
    dump("Failed to get table element!\n");
    window.close();
  }
  globalTableElement = TableElement.cloneNode(false);

  var tagNameObj = new Object;
  var countObj = new Object;
  var tableOrCellElement = editorShell.GetSelectedOrParentTableElement(tagNameObj, countObj);
  selectedCellCount = countObj.value;

  if (tagNameObj.value == "td")
  {
    // We are in a cell
    CellElement = tableOrCellElement;
    globalCellElement = CellElement.cloneNode(false);

    // Tells us whether cell, row, or column is selected
    SelectedCellsType = editorShell.GetSelectedCellsType(TableElement);
    
    // Ignore types except Cell, Row, and Column
    if (SelectedCellsType < SELECT_CELL || SelectedCellsType > SELECT_COLUMN)
      SelectedCellsType = SELECT_CELL;

    // Be sure at least 1 cell is selected.
    // (If the count is 0, then we were inside the cell.)
    if (selectedCellCount == 0)
      editorShell.SelectTableCell();

    // Get location in the cell map
    curRowIndex = editorShell.GetRowIndex(CellElement);
    curColIndex = editorShell.GetColumnIndex(CellElement);

    // Get actual rowspan and colspan
    var startRowIndexObj = new Object;
    var startColIndexObj = new Object;
    var rowSpanObj = new Object;
    var colSpanObj = new Object;
    var actualRowSpanObj = new Object;
    var actualColSpanObj = new Object;
    var isSelectedObj = new Object;
    editorShell.GetCellDataAt(TableElement, curRowIndex, curColIndex, 
                              startRowIndexObj, startColIndexObj,
                              rowSpanObj, colSpanObj, 
                              actualRowSpanObj, actualColSpanObj, isSelectedObj);

    curRowSpan = actualRowSpanObj.value;
    curColSpan = actualColSpanObj.value;

    // Set appropriate icons in the Previous/Next buttons
    SetSelectionButtons();

    // Starting TabPanel name is passed in
    if (window.arguments[1] == "CellPanel") 
    {
      currentPanel = CellPanel;

      //Set index for starting panel on the <tabpanel> element
      TabPanel.setAttribute("index", CellPanel);
  
      // Trigger setting of style for the tab widgets
      CellTab.setAttribute("selected", "true");
      TableTab.removeAttribute("selected");

      // Use cell element for Advanced Edit dialog
      globalElement = globalCellElement;
    }
  }

  if (currentPanel == TablePanel)
  {
    // Use table element for Advanced Edit dialog
    globalElement = globalTableElement;
    
    // We may call this with table selected, but no cell,
    //  so disable the Cell Properties tab
    if(!CellElement)
      CellTab.setAttribute("disabled", "true");
  }

  doSetOKCancel(onOK, null);

  // Note: we must use TableElement, not globalTableElement for these,
  //  thus we should not put this in InitDialog.
  // Instead, monitor desired counts with separate globals
  rowCount = editorShell.GetTableRowCount(TableElement);
  lastRowIndex = rowCount-1;
  colCount = editorShell.GetTableColumnCount(TableElement);
  lastColIndex = colCount-1;

  // User can change these via textfields  
  newRowCount = rowCount;
  newColCount = colCount;

  InitDialog();

  if (currentPanel == CellPanel)
    dialog.SelectionList.focus(); 
  else
    dialog.TableRowsInput.focus();
}


function InitDialog()
{
  // Get Table attributes
  dialog.TableRowsInput.value = rowCount;
  dialog.TableColumnsInput.value = colCount;
  dialog.TableHeightInput.value = InitPixelOrPercentMenulist(globalTableElement, TableElement, "height", "TableHeightUnits");
  dialog.TableWidthInput.value = InitPixelOrPercentMenulist(globalTableElement, TableElement, "width", "TableWidthUnits");
  dialog.BorderWidthInput.value = globalTableElement.border;
  dialog.SpacingInput.value = globalTableElement.cellSpacing;
  dialog.PaddingInput.value = globalTableElement.cellPadding;

  //BUG: The align strings are converted: e.g., "center" becomes "Center";
  var halign = globalTableElement.align.toLowerCase();
  if (halign == centerStr)
    dialog.TableAlignList.selectedIndex = 1;
  else if (halign == bottomStr)
    dialog.TableAlignList.selectedIndex = 2;
  else // Default = left
    dialog.TableAlignList.selectedIndex = 0;
  
  TableCaptionElement = globalTableElement.caption;
dump("Caption Element = "+TableCaptionElement+"\n");
  var index = 0;
  if (TableCaptionElement)
  {
    // Note: Other possible values are "left" and "right",
    //  but "align" is deprecated, so should we even support "botton"?
    if (TableCaptionElement.vAlign == "bottom")
      index = 2;
    else
      index = 1;
  }
  dialog.TableCaptionList.selectedIndex = index;
  
  if (globalTableElement.background)
    dialog.TableImageInput.value = globalTableElement.background;
  
  SetColor("tableBackgroundCW", globalTableElement.bgColor); 

  InitCellPanel(SelectedCellsType);
}

function InitCellPanel()
{
  // Get cell attributes
  if (globalCellElement)
  {
    // This assumes order of items is Cell, Row, Column
    dialog.SelectionList.selectedIndex = SelectedCellsType-1;

dump("*****globalCellElement="+globalCellElement+", CellElement="+CellElement+"\n");
    dialog.CellHeightInput.value = InitPixelOrPercentMenulist(globalCellElement, CellElement, "height", "CellHeightUnits");
    dialog.CellWidthInput.value = InitPixelOrPercentMenulist(globalCellElement, CellElement, "width", "CellWidthUnits");

    dialog.RowSpanInput.value = globalCellElement.getAttribute("rowspan");
    dialog.ColSpanInput.value = globalCellElement.getAttribute("colspan");
    
    var valign = globalCellElement.vAlign.toLowerCase();
    if (valign == topStr)
      dialog.CellVAlignList.selectedIndex = 0;
    else if (valign == bottomStr)
      dialog.CellVAlignList.selectedIndex = 2;
    else // Default = middle
      dialog.CellVAlignList.selectedIndex = 1;
    
    var halign = globalCellElement.align.toLowerCase();
    switch (halign)
    {
      case centerStr:
        dialog.CellHAlignList.selectedIndex = 1;
        break;
      case rightStr:
        dialog.CellHAlignList.selectedIndex = 2;
        break;
      case justifyStr:
        dialog.CellHAlignList.selectedIndex = 3;
        break;
      case charStr:
        var alignChar = globalCellElement.getAttribute(charStr);
        if (alignChar && alignChar.length == 1)
        {
          dialog.CellAlignCharInput.value = alignChar;
          dialog.CellHAlignList.selectedIndex = 4;
          dialog.CellAlignCharInput.removeAttribute("collapsed");

        } else {
          // "char" align set but no alignment char value
          dialog.CellHAlignList.selectedIndex = 0;
        }
        break;
      default:  // left
        dialog.CellHAlignList.selectedIndex = 0;
        break;
    }

    // Hide align char input if not that align type
    dialog.CellHAlignList.selectedIndex != 4
      dialog.CellAlignCharInput.setAttribute("collapsed","true");

    dialog.CellStyleList.selectedIndex = (globalCellElement.nodeName == "TH") ? 1 : 0;
    dialog.TextWrapList.selectedIndex = globalCellElement.noWrap ? 1 : 0;
    
    SetColor("cellBackgroundCW", globalCellElement.bgColor); 
    
    if (globalCellElement.background)
      dialog.CellImageInput.value = globalCellElement.background;
  }
}

//TODO: Should we validate the panel before leaving it? We don't now
function SelectTableTab()
{
  globalElement = globalTableElement;
  currentPanel = TablePanel;
}

function SelectCellTab()
{
  globalElement = globalCellElement;
  currentPanel = CellPanel;
}

function SelectCellHAlign()
{
  if (dialog.CellHAlignList.selectedIndex == 4)
    dialog.CellAlignCharInput.removeAttribute("collapsed");
  else
    dialog.CellAlignCharInput.setAttribute("collapsed","true");

  SetCheckbox("CellHAlignCheckbox");
}

function GetColorAndUpdate(ColorPickerID, ColorWellID, CheckboxID, popup)
{
  // Close the colorpicker
  popup.closePopup();
  var color = null;
  if (ColorPickerID)
    color = getColor(ColorPickerID);

  SetColor(ColorWellID, color);
  
  SetCheckbox(CheckboxID);
}

function SetColor(ColorWellID, color)
{
  // Save the color
  if (ColorWellID == "cellBackgroundCW")
  {
    if (color)
    {
      globalCellElement.setAttribute(bgcolor, color);
      dialog.CellInheritColor.setAttribute("collapsed","true");
    }
    else
    {
      globalCellElement.removeAttribute(bgcolor);
      // Reveal addition message explaining "default" color
      dialog.CellInheritColor.removeAttribute("collapsed");
    }
  }
  else
  {
    if (color)
    {
      globalTableElement.setAttribute(bgcolor, color);
      dialog.TableInheritColor.setAttribute("collapsed","true");
    }
    else
    {
      globalTableElement.removeAttribute(bgcolor);
      dialog.TableInheritColor.removeAttribute("collapsed");
    }
  }    
  setColorWell(ColorWellID, color); 
}

function ChangeSelection(newType)
{
dump("ChangeSelection: newType="+newType+"\n");
  newType = Number(newType);

  if (SelectedCellsType != newType)
  {
    SelectedCellsType = newType;
    // Keep the same focus CellElement, just change the type
    DoCellSelection();
    SetSelectionButtons();

    // Note: globalCellElement should still be a clone of CellElement
  }
}

function MoveSelection(forward)
{
  var newRowIndex = curRowIndex;
  var newColIndex = curColIndex;
  var focusCell;
  var inRow = false;

  if (SelectedCellsType == SELECT_ROW)
  {
    newRowIndex += (forward ? 1 : -1);

    // Wrap around if before first or after last row
    if (newRowIndex < 0)
      newRowIndex = lastRowIndex;
    else if (newRowIndex > lastRowIndex)
      newRowIndex = 0;
    inRow = true;

    // Use first cell in row for focus cell
    newColIndex = 0;
  }
  else
  {
    // Cell or column:
    if (!forward)
      newColIndex--;
      
    if (SelectedCellsType == SELECT_CELL)
    {
      // Skip to next cell
      if (forward)
        newColIndex += curColSpan;
    }
    else  // SELECT_COLUMN
    {
      // Use first cell in column for focus cell
      newRowIndex = 0;

      // Don't skip by colspan,
      //  but find first cell in next cellmap column
      if (forward)
        newColIndex++;
    }

    if (newColIndex < 0)
    {
      // Request is before the first cell in column

      // Wrap to last cell in column
      newColIndex = lastColIndex;

      if (SelectedCellsType == SELECT_CELL)
      {
        // If moving by cell, also wrap to previous...
        if (newRowIndex > 0)
          newRowIndex -= 1;
        else
          // ...or the last row
          newRowIndex = lastRowIndex;
        
        inRow = true;  
      }
    }
    else if (newColIndex > lastColIndex)
    {
      // Request is after the last cell in column

      // Wrap to first cell in column
      newColIndex = 0;

      if (SelectedCellsType == SELECT_CELL)
      {
        // If moving by cell, also wrap to next...
        if (newRowIndex < lastRowIndex)
          newRowIndex++;
        else
          // ...or the first row
          newRowIndex = 0;

        inRow = true;  
      }
    }
  }  

  // Get the cell at the new location
  var startRowIndexObj = new Object;
  var startColIndexObj = new Object;
  var rowSpanObj = new Object;
  var colSpanObj = new Object;
  var actualRowSpanObj = new Object;
  var actualColSpanObj = new Object;
  var isSelectedObj = new Object;

dump("*** Move from row="+curRowIndex+", col="+curColIndex+" to NewRow="+newRowIndex+", NewCol="+newColIndex+"\n");
  
  do {
    focusCell = editorShell.GetCellDataAt(TableElement, newRowIndex, newColIndex, 
                                     startRowIndexObj, startColIndexObj,
                                     rowSpanObj, colSpanObj, 
                                     actualRowSpanObj, actualColSpanObj, isSelectedObj);
    if (!focusCell)
    {
      dump("MoveSelection: CELL NOT FOUND\n");
      return null;
    }
    if (inRow)
    {
      if (startRowIndexObj.value == newRowIndex)
        break;
      else
        // Cell spans from a row above, look for the next cell in row
        newRowIndex += actualRowSpanObj.value;
    }
    else
    {
      if (startColIndexObj.value == newColIndex)
        break;
      else
        // Cell spans from a Col above, look for the next cell in column
        newColIndex += actualColSpanObj.value;
    }
  }
  while(true);

  // Set cell and other data
  CellElement = focusCell;

  globalTableCell = CellElement.cloneNode(false);
  globalElement = globalCellElement;

  curRowIndex = startRowIndexObj.value;
  curColIndex = startColIndexObj.value;
  curRowSpan = actualRowSpanObj.value;
  curColSpan = actualColSpanObj.value;

  if (CellDataChanged && dialog.ApplyBeforeMove.checked)
  {
    if (!ValidateCellData())
      return;

    editorShell.BeginBatchChanges();
    // Apply changes to all selected cells
    ApplyCellAttributes();
    editorShell.EndBatchChanges();

    SetCloseButton();
  }

  // Reinitialize using new cell only if checkbox is not checked
  if (!dialog.KeepCurrentData.checked)
    InitCellPanel();

  // Change the selection
  DoCellSelection();
}


function DoCellSelection()
{
  // Collapse selection into to the focus cell
  //  so editor uses that as start cell
  selection.collapse(CellElement, 0);

  switch (SelectedCellsType)
  {
    case SELECT_CELL:
      editorShell.SelectTableCell();
      break
    case SELECT_ROW:
      editorShell.SelectTableRow();
      break;
    default:
      editorShell.SelectTableColumn();
      break;
  }
}

function SetSelectionButtons()
{
  if (SelectedCellsType == SELECT_ROW)
  {
dump("SetSelectionButtons to ROW\n");
    // Trigger CSS to set images of up and down arrows
    dialog.PreviousButton.setAttribute("type","row");
    dialog.NextButton.setAttribute("type","row");
  }
  else
  {
    // or images of left and right arrows
    dialog.PreviousButton.setAttribute("type","col");
    dialog.NextButton.setAttribute("type","col");
  }
}

function ChooseTableImage()
{
  // Get a local file, converted into URL format
  fileName = GetLocalFileURL("img");
  if (fileName && fileName.length > 0)
  {
    dialog.TableImageInput.setAttribute("value",fileName);

    SetCheckbox("TableImageCheckbox");
  }
  // Put focus into the input field
  dialog.TableImageInput.focus();
}

function ChooseCellImage()
{
  fileName = GetLocalFileURL("img");
  if (fileName && fileName.length > 0)
  {
    dialog.CellImageInput.setAttribute("value",fileName);
    SetCheckbox("CellImageCheckbox");
  }
  // Put focus into the input field
  dialog.CellImageInput.focus();
}

function SwitchPanel(newPanel)
{
  if (currentPanel != newPanel)
  {
    //Set index for starting panel on the <tabpanel> element
    TabPanel.setAttribute("index", newPanel);
    if (newPanel == CellPanel)
    {    
      // Trigger setting of style for the tab widgets
      CellTab.setAttribute("selected", "true");
      TableTab.removeAttribute("selected");
    } else {
      TableTab.setAttribute("selected", "true");
      CellTab.removeAttribute("selected");
    }
    currentPanel = newPanel;
  }
}

// More weirdness: Can't pass in "inputWidget" -- becomes undefined?!
// Must pass in just ID and use getElementById
function ValidateNumber(inputWidgetID, listWidget, minVal, maxVal, element, attName)
{
  var inputWidget = document.getElementById(inputWidgetID);
  // Global error return value
  error = false;
  var maxLimit = maxVal;
  var isPercent = false;

  var numString = inputWidget.value.trimString();
  if (numString && numString != "")
  {
    if (listWidget)
      isPercent = (listWidget.selectedIndex == 1);
    if (isPercent)
      maxLimit = 100;

    numString = ValidateNumberString(numString, minVal, maxLimit);
    if(numString == "")
    {
      dump("Error returned from ValidateNumberString\n");

      // Switch to appropriate panel for error reporting
      SwitchPanel(validatePanel);

      // Error - shift to offending input widget
      inputWidget.focus();
      error = true;
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

function SetAlign(listID, defaultValue, element, attName)
{
  var value = document.getElementById(listID).selectedItem.data;
dump("SetAlign value = "+value+"\n");
  if (value == defaultValue)
    element.removeAttribute(attName);
  else
    element.setAttribute(attName, value);
}

function ValidateTableData()
{
  validatePanel = TablePanel;
  if (dialog.RowsCheckbox.checked)
  {
    newRowCount = Number(ValidateNumber("TableRowsInput", null, 1, maxRows, null, null));
    if (error) return false;
  }

  if (dialog.ColumnsCheckbox.checked)
  {
    newColCount = Number(ValidateNumber("TableColumnsInput", null, 1, maxColumns, null, null));
    if (error) return false;
  }

  if (dialog.TableHeightCheckbox.checked)
  {
    ValidateNumber("TableHeightInput", dialog.TableHeightUnits, 
                    1, maxPixels, globalTableElement, "height");
    if (error) return false;
  }

  if (dialog.TableWidthCheckbox.checked)
  {
    ValidateNumber("TableWidthInput", dialog.TableWidthUnits, 
                   1, maxPixels, globalTableElement, "width");
    if (error) return false;
  }

  if (dialog.TableBorderCheckbox.checked)
  {
    var border = ValidateNumber("BorderWidthInput", null, 0, maxPixels, globalTableElement, "border");
    // TODO: Deal with "BORDER" without value issue
    if (error) return false;
  }

  if (dialog.CellSpacingCheckbox.checked)
  {
    ValidateNumber("SpacingInput", null, 0, maxPixels, globalTableElement, "cellspacing");
    if (error) return false;
  }

  if (dialog.CellPaddingCheckbox.checked)
  {
    ValidateNumber("PaddingInput", null, 0, maxPixels, globalTableElement, "cellpadding");
    if (error) return false;
  }

  if (dialog.TableHAlignCheckbox.checked)
    SetAlign("TableAlignList", defHAlign, globalTableElement, "align");

  // Color is set on globalCellElement immediately

  // Background Image
  if (dialog.TableImageCheckbox.checked)
  {
    var imageName = dialog.TableImageInput.value.trimString();
    if (imageName && imageName != "")
    {
      if (IsValidImage(imageName))
        globalTableElement.background = imageName;
      else
      {
        dialog.TableImageInput.focus();
        // Switch to appropriate panel for error reporting
        SwitchPanel(validatePanel);
        ShowInputErrorMessage(GetString("MissingImageError"));
        return false;
      }
    } else {
      globalTableElement.removeAttribute("background");
    }
  }
  return true;
}

function ValidateCellData()
{

  validatePanel = CellPanel;

  if (dialog.CellHeightCheckbox.checked)
  {
    ValidateNumber("CellHeightInput", dialog.TableHeightUnits, 
                    1, maxPixels, globalCellElement, "height");
    if (error) return false;
  }

  if (dialog.CellWidthCheckbox.checked)
  {
    ValidateNumber("CellWidthInput", dialog.TableWidthUnits, 
                   1, maxPixels, globalCellElement, "width");
    if (error) return false;
  }
  
  if (dialog.SpanCheckbox.checked)
  {
    // Note that span = 0 is allowed and means "span entire row/col"
    ValidateNumber("ColSpanInput", null,
                   0, colCount, globalCellElement, "colspan");
    if (error) return false;

    ValidateNumber("RowSpanInput", null,
                   0, rowCount, globalCellElement, "rowspan");
    if (error) return false;
  }

  if (dialog.CellHAlignCheckbox.checked)
  {
    // Vertical alignment is complicated by "char" type
    var hAlign = dialog.CellHAlignList.selectedItem.data;
dump("Cell hAlign = "+hAlign+"\n");

    if (hAlign != charStr)
      globalCellElement.removeAttribute(charStr);
    
    if (hAlign == "left")
    {
      globalCellElement.removeAttribute("align");
    }
    else
    {
      if (hAlign == charStr)
      {
        var alignChar = dialog.CellAlignCharInput.value.trimString().charAt(0);
        globalCellElement.setAttribute(charStr, alignChar);
dump("Alignment char="+alignChar+"\n");
      }

      globalCellElement.setAttribute("align", hAlign);
    }
  }

  if (dialog.CellVAlignCheckbox.checked)
    SetAlign("CellVAlignList", defVAlign, globalCellElement, "valign");

  if (dialog.TextWrapCheckbox.checked)
  {
    if (dialog.TextWrapList.selectedIndex == 1)
      globalCellElement.setAttribute("nowrap","true");
    else
      globalCellElement.removeAttribute("nowrap");
  }

  // Background Image
  if (dialog.CellImageCheckbox.checked)
  {
    var imageName = dialog.TableImageInput.value.trimString();
    if (imageName && imageName != "")
    {
      if (IsValidImage(imageName))
        globalCellElement.background = imageName;
      else
      {
        dialog.CellImageInput.focus();
        // Switch to appropriate panel for error reporting
        SwitchPanel(validatePanel);
        ShowInputErrorMessage(GetString("MissingImageError"));
      }
    } else {
      globalCellElement.removeAttribute("background");
    }
  }  

  return true;
}

function ValidateData()
{
  var result;
  var savePanel = currentPanel;
  
  // Validate current panel first
  if (currentPanel == TablePanel)
  {
    result = ValidateTableData();
    if (result)
      result = ValidateCellData();
  } else {
    result = ValidateCellData();
    if (result)
      result = ValidateTableData();
  }
  if(!result) return false;

  // If we passed, restore former currentPanel
  currentPanel = savePanel;

  // Set global element for AdvancedEdit
  if(currentPanel == TablePanel)
    globalElement = globalTableElement;
  else
    globalElement = globalCellElement;

  return true;
}

// Call this when a textfield or menulist is changed
//   so the checkbox is automatically set
function SetCheckbox(checkboxID)
{
dump("SetCheckbox: id="+checkboxID+"\n");

  // Set associated checkbox
  document.getElementById(checkboxID).checked = true;

  if (currentPanel == CellPanel)
    CellDataChanged = true;
}

function ChangeIntTextfield(textfieldID, checkboxID)
{
  // Filter input for just integers
  forceInteger(textfieldID);

  // Set associated checkbox
  SetCheckbox(checkboxID);
}

function CloneAttribute(destElement, srcElement, attr)
{
  var value = srcElement.getAttribute(attr);
  
  // Use editorShell methods since we are always
  //  modifying a table in the document and 
  //  we need transaction system for undo
  if (!value || value.length == 0)
    editorShell.RemoveAttribute(destElement, attr);
  else
    editorShell.SetAttribute(destElement, attr, value);
}

function ApplyTableAttributes()
{
  if (dialog.TableCaptionCheckbox.checked)
  {
    var newAlign = dialog.TableCaptionList.selectedItem.data;
    if (!newAlign) newAlign = "";

    if (TableCaptionElement)
    {
      // Get current alignment
      var align = TableCaptionElement.align.toLowerCase();
      // This is the default
      if (!align) align = "top";

      if (newAlign == "")
      {
        // Remove existing caption
        editorShell.DeleteElement(TableCaptionElement);
        TableCaptionElement = null;
      } 
      else if( align != newAlign)
      {
        if (align == "top") // This is default, so don't explicitly set it
          editorShell.RemoveAttribute(TableCaptionElement, "align");
        else
          editorShell.SetAttribute(TableCaptionElement, "align", newAlign);
      }
    } 
    else if (newAlign != "")
    {
dump("Insert a table caption...\n");
      // Create and insert a caption:
      TableCaptionElement = editorShell.CreateElementWithDefaults("caption");
      if (TableCaptionElement)
      {
        if (newAlign != "top")
          TableCaptionElement.setAttribute("align", newAlign);
        
        // Insert it into the table - caption is always inserted as first child
        editorShell.InsertElement(TableCaptionElement, TableElement, 0);
      }
    }
  }

  //TODO: DOM manipulation to add/remove table rows/columns
  if (dialog.RowsCheckbox.checked && newRowCount != rowCount)
  {
  }

  if (dialog.ColumnsCheckbox.checked && newColCount != colCount)
  {
  }

  if (dialog.TableHeightCheckbox.checked)
    CloneAttribute(TableElement, globalTableElement, "height");

  if (dialog.TableWidthCheckbox.checked)
    CloneAttribute(TableElement, globalTableElement, "width");

  if (dialog.TableBorderCheckbox.checked)
    CloneAttribute(TableElement, globalTableElement, "border");

  if (dialog.CellSpacingCheckbox.checked)
    CloneAttribute(TableElement, globalTableElement, "cellspacing");

  if (dialog.CellPaddingCheckbox.checked)
    CloneAttribute(TableElement, globalTableElement, "cellpadding");

  if (dialog.TableHAlignCheckbox.checked)
    CloneAttribute(TableElement, globalTableElement, "align");

  if (dialog.TableColorCheckbox.checked)
    CloneAttribute(TableElement, globalTableElement, "bgcolor");

  if (dialog.TableImageCheckbox.checked)
    CloneAttribute(TableElement, globalTableElement, "background");
  
}

function ApplyCellAttributes()
{
  // Apply changes to all selected cells
  var selectedCell = editorShell.GetFirstSelectedCell();
  while (selectedCell)
  {
    ApplyAttributesToOneCell(selectedCell); 
    selectedCell = editorShell.GetNextSelectedCell();
  }
  CellDataChanged = false;
}

function ApplyAttributesToOneCell(destElement)
{
  if (dialog.CellHeightCheckbox.checked)
    CloneAttribute(destElement, globalCellElement, "height");

  if (dialog.CellWidthCheckbox.checked)
    CloneAttribute(destElement, globalCellElement, "width");

  if (dialog.SpanCheckbox.checked)
  {
    CloneAttribute(destElement, globalCellElement, "rowspan");
    CloneAttribute(destElement, globalCellElement, "colspan");
  }

  if (dialog.CellHAlignCheckbox.checked)
  {
    CloneAttribute(destElement, globalCellElement, "align");
    CloneAttribute(destElement, globalCellElement, charStr);
  }

  if (dialog.CellVAlignCheckbox.checked)
    CloneAttribute(destElement, globalCellElement, "valign");

  if (dialog.TextWrapCheckbox.checked)
    CloneAttribute(destElement, globalCellElement, "nowrap");

  if (dialog.CellColorCheckbox.checked)
    CloneAttribute(destElement, globalCellElement, "bgcolor");

  if (dialog.CellImageCheckbox.checked)
    CloneAttribute(destElement, globalCellElement, "background");

  if (dialog.CellStyleCheckbox.checked)
  {
    var newStyleIndex = dialog.CellStyleList.selectedIndex;
    var currentStyleIndex = (destElement.nodeName == "TH") ? 1 : 0;

    if (newStyleIndex != currentStyleIndex)
    {
      // Switch cell types 
      // (replaces with new cell and copies attributes and contents)
      destElement = editorShell.SwitchTableCellHeaderType(destElement);
      CurrentStyleIndex = newStyleIndex;
    }
  }
}

function SetCloseButton()
{
  // Change text on "Cancel" button after Apply is used
  if (!ApplyUsed)
  {
    document.getElementById("cancel").setAttribute("value",GetString("Close"));
    ApplyUsed = true;
  }
}

function Apply()
{
  if (ValidateData())
  {
    editorShell.BeginBatchChanges();

    // Can't use editorShell.CloneAttributes -- we must change only
    //   attributes that are checked!
    ApplyTableAttributes();

    // We may have just a table, so check for cell element
    if (globalCellElement)
      ApplyCellAttributes();

    editorShell.EndBatchChanges();

    SetCloseButton();
    return true;
  }
  return false;
}

function onOK()
{
  // Do same as Apply and close window if ValidateData succeeded
  return Apply();
}
