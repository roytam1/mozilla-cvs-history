// Each editor window must include this file
// Variables  shared by all dialogs:
var editorShell;
var SelectionOnly=1; 
var FormatedWithDoctype=2; 
var FormatedWithoutDoctype=6; 

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

function ClearList(list)
{
  for( i = (list.length-1); i >= 0; i-- ) {
    list.remove(i);
  }
}

function ReplaceStringInList(list, index, string)
{
  //Hmmm... We should be able to simply set the "text" and "value"
  //  properties of the option element, but setting "text" doesn't work.
  //  Replace with a new node instead:
  if (index < list.options.length)
  {
/*
    node = list.options[index];
    dump("BEFORE Option node text: "+node.text+" Value: "+node.value+"\n");
    node.text = string;
    node.value = string;
    dump("AFTER Option node text: "+node.text+" Value: "+node.value+"\n");
*/
    // Save and remove selection else we have trouble below!
    //  (This must be a bug!)
    selIndex = list.selectedIndex;
    list.selectedIndex = -1;

    optionNode = new Option(string, string);
    // Remove existing option node
    //list.remove(index);
    list.options[index] = null;
    // Insert the new node
    list.options[index] = optionNode;
    // NOTE: If we insert, then remove, we crash!
    // Reset the selected item
    list.selectedIndex = selIndex;
  }
}

function AppendStringToList(list, string)
{
  
  // THIS DOESN'T WORK! Result is a XULElement -- namespace problem
  //optionNode1 = document.createElement("option");
  // "Unsanctioned method from Vidur:
  // createElementWithNamespace("http://... [the HTML4 URL], "option);

  // This works - Thanks to Vidur! Params = name, value
  optionNode = new Option(string, string);
  if (optionNode) {
    list.add(optionNode, null);    
  } else {
    dump("Failed to create OPTION node. String content="+string+"\n");
  }
}

// "value" may be a number or string type
function ValidateNumberString(value, minValue, maxValue)
{
  // Get the number version
  number = value - 0;
  if ((value+"") != "") {
    if (number && number >= minValue && number <= maxValue ){
      // Return string version of the number
      return number + "";
    }
  }
  message = "The number you entered ("+number+") is outside of allowed range.\nPlease enter a number between "+minValue+" and "+maxValue;

  // Initialize where we place result from generic EdMessage dialog
  window.msgResult = 0;
  // This is NOT MODAL!
  window.openDialog("chrome://editordlgs/content/EdMessage.xul", "MsgDlg", "chrome", "", message, "Input Error", "OK");
  // We could do something like this if we could call
  //  a method that pumps the message system
  //while (window.msgResult == 0);

  dump("Message button pressed: "+window.msgResult+"\n");
  // Return an empty string to indicate error
  return "";
}


function TrimStringLeft(string)
{
  firstCharIndex = 0;
  len = string.length;
  var result;

  while (firstCharIndex < len) {
    if (!IsWhitespace(string.charAt(firstCharIndex))) break;
    firstCharIndex = firstCharIndex + 1;
  }
  if (firstCharIndex > len) {
    string = "";
  } else {
    string = string.slice(firstCharIndex);
  }
  return string;
}

function TrimStringRight(string)
{
  len = string.length;
  if (len > 0 ) {
    lastCharIndex = string.length-1;
    var result;

    while (lastCharIndex > 0) {
      // Find the last non-whitespace char
      if (!IsWhitespace(string.charAt(lastCharIndex))) break;
      lastCharIndex = lastCharIndex - 1;
    }
    if (lastCharIndex == 0) {
      string = "";
    } else {
      string = string.slice(0, lastCharIndex+1);
    }
  }
  return string;
}

// Remove whitespace from both ends of a string
function TrimString(string)
{
  return TrimStringRight(TrimStringLeft(string));
}

function IsWhitespace(character)
{
  result = character.match(/\s/);
  if (result == null)
    return false;
  return true;
}

function TruncateStringAtWordEnd(string, maxLength, addEllipses)
{
  // We assume they probably don't want whitespace at the beginning
  string = TrimStringLeft(string);

  len = string.length;
  if (len > maxLength) {
    // We need to truncate the string
    var max;
    if (addEllipses) {
      // Make room for ellipses
      max = maxLength - 3;
    } else {
      max = maxLength;
    }
    var lastCharIndex = 0;
    
    // Start search just past max if there's enough characters
    if (len >= (max+1)) {
      lastCharIndex = max;
    } else {
      lastCharIndex = len-1;
    }
    dump("Len="+len+" lastCharIndex="+lastCharIndex+" max="+max+"\n");

    // Find the last whitespace char from the end
    dump("Skip to first whitspace from end: ");

    while (lastCharIndex > 0) {
      lastChar = string.charAt(lastCharIndex);
      dump(lastChar);
      if (IsWhitespace(lastChar)) break;
      lastCharIndex = lastCharIndex -1;
    }
    dump("[space found]\nlastCharIndex="+lastCharIndex+"\nSkip over whitespace:");

    while (lastCharIndex > 0) {
      // Find the last non-whitespace char
      lastChar = string.charAt(lastCharIndex);
      dump(lastChar);
      if (!IsWhitespace(lastChar)) break;
      lastCharIndex = lastCharIndex -1;
    }
    dump("[non-space found]\nlastCharIndex="+lastCharIndex+"\n");
    
    string = string.slice(0, lastCharIndex+1);
    if (addEllipses) {
      string = string+"...";
      dump(string+"\n");
    }
  }
  return string;
}

// Replace all whitespace characters with supplied character
// E.g.: Use charReplace = " ", to "unwrap" the string by removing line-end chars
//       Use charReplace = "_" when you don't want spaces (like in a URL)
function ReplaceWhitespace(string, charReplace) {
  if (string.length > 0 )
  {
    string = TrimString(string);
    // This replaces a run of whitespace with just one character
    string = string.replace(/\s+/g, charReplace);
  }
  dump(string+"\n");
  return string;
}

// this function takes an elementID and a flag
// if the element can be found by ID, then it is either enabled (by removing "disabled" attr)
// or disabled (setAttribute) as specified in the "doEnable" parameter
function SetElementEnabledByID( elementID, doEnable )
{
  element = document.getElementById(elementID);
  if ( element )
  {
    if ( doEnable )
    {
      element.removeAttribute( "disabled" );
    }
    else
    {
      element.setAttribute( "disabled", "true" );
    }
  }
}

// this function takes an ID for a label and a flag
// if an element can be found by its ID, then it is either enabled or disabled
// The class is set to either "enabled" or "disabled" depending on the flag passed in.
// This function relies on css having a special appearance for these two classes.
function SetLabelEnabledByID( labelID, doEnable )
{
  label = document.getElementById(labelID);
  if ( label )
  {
    if ( doEnable )
    {
     label.setAttribute( "class", "enabled" );
    }
    else
    {
     label.setAttribute( "class", "disabled" );
    }
  }
  else
  {
    dump( "not changing label"+labelID+"\n" );
  }
}

// Next two methods assume caller has a "percentChar" variable 
//  to hold an empty string (no % used) or "%" (percent is used)
function InitPixelOrPercentPopupButton(element, attribute, buttonID)
{
  size = element.getAttribute(attribute);
  btn = document.getElementById(buttonID);

  // Search for a "%" character
  percentIndex = size.search(/%/);
  if (percentIndex > 0) {
    percentChar = "%";
    // Strip out the %
    size = size.substr(0, percentIndex);
    // TODO: USE ENTITIES FOR TEXT VALUES
    if (btn)
      btn.setAttribute("value","percent");
  } else {
    if (btn)
      btn.setAttribute("value","pixels");
  }
  return size;
}

// Input string is "" for pixel, or "%" for percent
function SetPixelOrPercentByID(elementID, percentString)
{
  percentChar = percentString;
  dump("SetPixelOrPercent. PercentChar="+percentChar+"\n");

  btn = document.getElementById( elementID );
  if ( btn )
  {
    if ( percentChar == "%" )
    {
      btn.setAttribute( "value", "percent" );
    }
    else
    {
      btn.setAttribute( "value", "pixels" );
    }
  }
}

// forceInteger by petejc (pete@postpagan.com)
function forceInteger(elementID)
{
  editfield = document.getElementById( elementID );
  if ( !editfield )
    return;
  
  var stringIn = editfield.value;
  var pat = /\D/g;
  
  editfield.value = stringIn.replace(pat, "");
}

// All dialogs share this simple method
function onCancel()
{
  dump("Cancel button clicked: closing window\n");
  window.close();
}

function GetSelectionAsText()
{
  return editorShell.GetContentsAs("text/plain", SelectionOnly);
}