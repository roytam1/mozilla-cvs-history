/* -*- Mode: Java; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998-1999
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *    Sammy Ford (sford@swbell.net)
 *    Dan Haddix (dan6992@hotmail.com)
 *    John Ratke (jratke@owc.net)
 *    Ryan Cassin (rcassin@supernova.org)
 *    Daniel Glazman (glazman@netscape.com)
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

/* Main Composer window UI control */

var gEditor;
var editorShell;   // XXX THIS NEEDS TO DIE
var gComposerWindowControllerID = -1;
var documentModified;
var prefAuthorString = "";

const kDisplayModeNormal = 0;
const kDisplayModeAllTags = 1;
const kDisplayModeSource = 2;
const kDisplayModePreview = 3;
const kDisplayModeMenuIDs = ["viewNormalMode", "viewAllTagsMode", "viewSourceMode", "viewPreviewMode"];
const kDisplayModeTabIDS = ["NormalModeButton", "TagModeButton", "SourceModeButton", "PreviewModeButton"];
const kBaseEditorStyleSheet = "chrome://editor/content/EditorOverride.css";
const kNormalStyleSheet = "chrome://editor/content/EditorContent.css";
const kAllTagsStyleSheet = "chrome://editor/content/EditorAllTags.css";
const kParagraphMarksStyleSheet = "chrome://editor/content/EditorParagraphMarks.css";
const kTextMimeType = "text/plain";
const kHTMLMimeType = "text/html";

var gPreviousNonSourceDisplayMode = 1;
var gEditorDisplayMode = -1;
var docWasModified = false;  // Check if clean document, if clean then unload when user "Opens"
var gContentWindow = 0;
var gSourceContentWindow = 0;
var gHTMLSourceChanged = false;
var gContentWindowDeck;
var gFormatToolbar;
var gFormatToolbarHidden = false;
var gViewFormatToolbar;
var gColorObj = { LastTextColor:"", LastBackgroundColor:"", LastHighlightColor:"",
                  Type:"", SelectedType:"", NoDefault:false, Cancel:false,
                  HighlightColor:"", BackgroundColor:"", PageColor:"",
                  TextColor:"", TableColor:"", CellColor:""
                };
var gDefaultTextColor = "";
var gDefaultBackgroundColor = "";
var gCSSPrefListener;
var gPrefs;

var gLastFocusNode = null;
var gLastFocusNodeWasSelected = false;

// These must be kept in synch with the XUL <options> lists
var gFontSizeNames = ["xx-small","x-small","small","medium","large","x-large","xx-large"];

const nsIFilePicker = Components.interfaces.nsIFilePicker;

function nsButtonPrefListener()
{
  try {
    var pbi = pref.QueryInterface(Components.interfaces.nsIPrefBranchInternal);
    pbi.addObserver(this.domain, this, false);
  } catch(ex) {
    dump("Failed to observe prefs: " + ex + "\n");
  }
}

// implements nsIObserver
nsButtonPrefListener.prototype =
{
  domain: "editor.use_css",
  observe: function(subject, topic, prefName)
  {
    if (!isHTMLEditor())
      return;
    // verify that we're changing a button pref
    if (topic != "nsPref:changed") return;
    if (prefName.substr(0, this.domain.length) != this.domain) return;

    var cmd = document.getElementById("cmd_highlight");
    if (cmd) {
      var prefs = GetPrefs();
      var useCSS = prefs.getBoolPref(prefName);
      if (useCSS && gEditor) {
        var mixedObj = {};
        var state = gEditor.getHighlightColorState(mixedObj);
        cmd.setAttribute("state", state);
        cmd.removeAttribute("collapsed");
      }      
      else {
        cmd.setAttribute("state", "transparent");
        cmd.setAttribute("collapsed", "true");
      }

      if (gEditor)
        gEditor.isCSSEnabled = useCSS;
    }
  }
}

function AfterHighlightColorChange()
{
  if (!isHTMLEditor())
    return;

  var button = document.getElementById("cmd_highlight");
  if (button) {
    var mixedObj = {};
    var state = gEditor.getHighlightColorState(mixedObj);
    button.setAttribute("state", state);
    onHighlightColorChange();
  }      
}

function EditorOnLoad()
{
    // See if argument was passed.
    if ( window.arguments && window.arguments[0] ) {
        // Opened via window.openDialog with URL as argument.
        // Put argument where EditorStartup expects it.
        document.getElementById( "args" ).setAttribute( "value", window.arguments[0] );
    }

    // get default character set if provided
    if ("arguments" in window && window.arguments.length > 1 && window.arguments[1]) {
      if (window.arguments[1].indexOf("charset=") != -1) {
        var arrayArgComponents = window.arguments[1].split("=");
        if (arrayArgComponents) {
          // Put argument where EditorStartup expects it.
          document.getElementById( "args" ).setAttribute("charset", arrayArgComponents[1]);
        }
      }
    }

    window.tryToClose = EditorCanClose;

    // Continue with normal startup.
    EditorStartup('html', document.getElementById("content-frame"));
}

function TextEditorOnLoad()
{
    // See if argument was passed.
    if ( window.arguments && window.arguments[0] ) {
        // Opened via window.openDialog with URL as argument.
        // Put argument where EditorStartup expects it.
        document.getElementById( "args" ).setAttribute( "value", window.arguments[0] );
    }
    // Continue with normal startup.
    EditorStartup('text', document.getElementById("content-frame"));
}

// This should be called by all editor users when they close their window
//  or other similar "done with editor" actions, like recycling a Mail Composer window.
function EditorCleanup()
{
  SwitchInsertCharToAnotherEditorOrClose();
}

function PageIsEmptyAndUntouched()
{
  return (gEditor != null) && gEditor.documentIsEmpty && 
         !docWasModified && !gHTMLSourceChanged;
}

function IsInHTMLSourceMode()
{
  return (gEditorDisplayMode == kDisplayModeSource);
}

// are we editing HTML (i.e. neither in HTML source mode, nor editing a text file)
function IsEditingRenderedHTML()
{
  return isHTMLEditor() && !IsInHTMLSourceMode();
}

function IsWebComposer()
{
  return document.documentElement.id == "editorWindow";
}

function IsDocumentEditable()
{
  try {
    return GetCurrentEditor().isDocumentEditable;
  } catch (e) {}
  return false;
}

function IsDocumentModified()
{
  try {
    return GetCurrentEditor().documentModified;
  } catch(e) {}
  return false;
}

var DocumentReloadListener =
{
  NotifyDocumentCreated: function() {},
  NotifyDocumentWillBeDestroyed: function() {},

  NotifyDocumentStateChanged:function( isNowDirty )
  {
    var charset = gEditor.documentCharacterSet;

    // unregister the listener to prevent multiple callbacks
    gEditor.removeDocumentStateListener( DocumentReloadListener );

    // update the META charset with the current presentation charset
    gEditor.documentCharacterSet = charset;
  }
};

function addEditorClickEventListener()
{
  try {
    var bodyelement = GetBodyElement();
    if (bodyelement)
      bodyelement.addEventListener("click", EditorClick, false);
  } catch (e) {}
}

function DoAllQueryInterfaceOnEditor()
{
  // do QI here so the interfaces will be accessible on gEditor
  try {
    gEditor.QueryInterface(Components.interfaces.nsIHTMLEditor);
  } catch(e) {}
  try {
    gEditor.QueryInterface(Components.interfaces.nsIPlaintextEditor);
  } catch(e) {}
  try {
    gEditor.QueryInterface(Components.interfaces.nsITableEditor);
  } catch(e) {}
  try {
    gEditor.QueryInterface(Components.interfaces.nsIEditorStyleSheets);
  } catch(e) {}
}

// This is called when the real editor document is created,
// before it's loaded.
// IMPORTANT: This is used by ALL Composers, so be careful!
var DocumentStateListener =
{
  NotifyDocumentCreated: function()
  {
    gEditor = editorShell.editor;

    // Just for convenience
    gContentWindow = window._content;

    // do all of our QI'ing here so we don't need to do it elsewhere
    DoAllQueryInterfaceOnEditor();

    if (editorShell.editorType == "htmlmail"
        || editorShell.editorType == "textmail")
    {
      gEditor.QueryInterface(Components.interfaces.nsIEditorMailSupport);
    }
    else
    {
      // Mail Composers start focus in address area
      gContentWindow.focus();
    }

    if (!("InsertCharWindow" in window))
      window.InsertCharWindow = null;

    try {
      // Add the base sheets for editor cursor etc.
      gEditor.addOverrideStyleSheet(kBaseEditorStyleSheet);

      //  and extra styles for showing anchors, table borders, smileys, etc
      gEditor.addOverrideStyleSheet(kNormalStyleSheet);
    } catch (e) {}

    // XXX Temporary: This should be set during startup 
    //  once we finish transition from nsIEditorShell is to nsIEditorSession
    try {
      gEditor.contentsMIMEType = gIsHTMLEditor ? kHTMLMimeType : kTextMimeType;
    } catch(e) {}
    
    // Add mouse click watcher if right type of editor
    if (isHTMLEditor())
      addEditorClickEventListener();

    // udpate menu items now that we have an editor to play with
    window.updateCommands("create");

    // Do the rest only if a Web Composer application
    if (IsWebComposer())
    {
      // Get the window command controller created in SetupComposerWindowCommands()
      if (gComposerWindowControllerID != -1)
      {
        try { 
          var controller = window.controllers.getControllerById(gComposerWindowControllerID);
          controller.SetCommandRefCon(gEditor.QueryInterface(Components.interfaces.nsISupports));
        } catch (e) {}
      }

      // Call EditorSetDefaultPrefsAndDoctype first so it gets the default author before initing toolbars
      EditorSetDefaultPrefsAndDoctype();
      EditorInitToolbars();
    
      // Set window title and build "Recent Files" menu  
      // (to detect empty menu and disable it)
      UpdateWindowTitle();
      BuildRecentMenu();

      // We must wait until document is created to get proper Url
      // (Windows may load with local file paths)
      SetSaveAndPublishUI(GetDocumentUrl());

      // Start in "Normal" edit mode
      SetDisplayMode(kDisplayModeNormal);
    }
  },

    // note that the editor seems to be gone at this point 
    // so we don't have a way to remove the click listener.
    // hopefully it is being cleaned up with all listeners
  NotifyDocumentWillBeDestroyed: function() {},

  NotifyDocumentStateChanged:function( isNowDirty )
  {
    /* Notify our dirty detector so this window won't be closed if
       another document is opened */
    if (isNowDirty)
      docWasModified = true;

    // hack! Should not need this updateCommands, but there is some controller
    //  bug that this works around. ??
    // comment out the following line because it cause 41573 IME problem on Mac
    //gContentWindow.focus();
    window.updateCommands("create");
    window.updateCommands("save");
  }
};

function EditorStartup(editorType, editorElement)
{
  gIsHTMLEditor = (editorType == "html");
  if (gIsHTMLEditor)
  {
    gSourceContentWindow = document.getElementById("content-source");

    // XUL elements we use when switching from normal editor to edit source
    gContentWindowDeck = document.getElementById("ContentWindowDeck");
    gFormatToolbar = document.getElementById("FormatToolbar");
    gViewFormatToolbar = document.getElementById("viewFormatToolbar");
  }

  // store the editor shell in the window, so that child windows can get to it.
  // XXX This needs to go, but first, all the dialogs need to be changed
  // not to require it.
  editorShell = editorElement.editorShell;        // this pattern exposes a JS/XBL bug that causes leaks
  editorShell.editorType = editorType;

  editorShell.webShellWindow = window;
  editorShell.contentWindow = window._content;

  // set up our global prefs object
  GetPrefsService();

  // Startup also used by other editor users, such as Message Composer
  EditorSharedStartup();

  // Commands specific to the Composer Application window,
  //  (i.e., not embedded editors)
  //  such as file-related commands, HTML Source editing, Edit Modes...
  SetupComposerWindowCommands();

  gCSSPrefListener = new nsButtonPrefListener();

  // hide Highlight button if we are in an HTML editor with CSS mode off
  var cmd = document.getElementById("cmd_highlight");
  if (cmd) {
    var prefs = GetPrefs();
    var useCSS = prefs.getBoolPref("editor.use_css");
    if (!useCSS && gIsHTMLEditor) {
      cmd.setAttribute("collapsed", "true");
    }
  }

  // Get url for editor content and load it.
  // the editor gets instantiated by the editor shell when the URL has finished loading.
  var url = document.getElementById("args").getAttribute("value");
  var charset = document.getElementById("args").getAttribute("charset");
  // XXX We can't call gEditor.documentCharacterSet = charset
  // XXX because gEditor isn't set until after the document loads.
  if (charset) editorShell.SetDocumentCharacterSet(charset);

  // We need the docshell to do a loadurl!
  // editorshell gets it from the window in PrepareDocumentForEditing.
  editorShell.LoadUrl(url);
}

// This is also called by Message Composer
function EditorSharedStartup()
{
  // Just for convenience
  gContentWindow = window._content;

  switch (editorShell.editorType)
  {
      case "html":
      case "htmlmail":
        gIsHTMLEditor = true;
        break;

      case "text":
      case "textmail":
        gIsHTMLEditor = false;
        break;

      default:
        dump("INVALID EDITOR TYPE: " + editorShell.editorType + "\n");
        gIsHTMLEditor = false;
        break;
  }

  // Set up the mime type and register the commands.
  // We don't have an editor yet -- in fact, this is the listener which
  // will tell us when it's time to create the editor.
  // So we can't use gEditor.addDocumentStateListener here.
  if (gIsHTMLEditor)
  {
    //XXX this is replaced by nsIEditor::contentsMIMEType
    editorShell.contentsMIMEType = kHTMLMimeType;
    SetupHTMLEditorCommands();
  }
  else
  {
    editorShell.contentsMIMEType = kTextMimeType;
    SetupTextEditorCommands();
  }

  // add a listener to be called when document is really done loading
  editorShell.RegisterDocumentStateListener( DocumentStateListener );

  var isMac = (GetOS() == gMac);

  // Set platform-specific hints for how to select cells
  // Mac uses "Cmd", all others use "Ctrl"
  var tableKey = GetString(isMac ? "XulKeyMac" : "TableSelectKey");
  var dragStr = tableKey+GetString("Drag");
  var clickStr = tableKey+GetString("Click");

  var delStr = GetString(isMac ? "Clear" : "Del");

  SafeSetAttribute("menu_SelectCell", "acceltext", clickStr);
  SafeSetAttribute("menu_SelectRow", "acceltext", dragStr);
  SafeSetAttribute("menu_SelectColumn", "acceltext", dragStr);
  SafeSetAttribute("menu_SelectAllCells", "acceltext", dragStr);
  // And add "Del" or "Clear"
  SafeSetAttribute("menu_DeleteCellContents", "acceltext", delStr);

  // Set text for indent, outdent keybinding

  // hide UI that we don't have components for
  RemoveInapplicableUIElements();

  gPrefs = GetPrefs();

  // Use browser colors as initial values for editor's default colors
  var BrowserColors = GetDefaultBrowserColors();
  if (BrowserColors)
  {
    gDefaultTextColor = BrowserColors.TextColor;
    gDefaultBackgroundColor = BrowserColors.BackgroundColor;
  }

  // For new window, no default last-picked colors
  gColorObj.LastTextColor = "";
  gColorObj.LastBackgroundColor = "";
  gColorObj.LastHighlightColor = "";
}

// This method is only called by Message composer when recycling a compose window
function EditorResetFontAndColorAttributes()
{
  document.getElementById("cmd_fontFace").setAttribute("state", "");
  EditorRemoveTextProperty("font", "color");
  EditorRemoveTextProperty("font", "bgcolor");
  EditorRemoveTextProperty("font", "size");
  EditorRemoveTextProperty("small", "");
  EditorRemoveTextProperty("big", "");
  var bodyelement = GetBodyElement();
  if (bodyelement)
  {
    gEditor.removeAttributeOrEquivalent(bodyelement, "text", true);
    gEditor.removeAttributeOrEquivalent(bodyelement, "bgcolor", true);
    bodyelement.removeAttribute("link");
    bodyelement.removeAttribute("alink");
    bodyelement.removeAttribute("vlink");
    gEditor.removeAttributeOrEquivalent(bodyelement, "background", true);
  }
  gColorObj.LastTextColor = "";
  gColorObj.LastBackgroundColor = "";
  gColorObj.LastHighlightColor = "";
  document.getElementById("cmd_fontColor").setAttribute("state", "");
  document.getElementById("cmd_backgroundColor").setAttribute("state", "");
  UpdateDefaultColors();
}

function _EditorNotImplemented()
{
  dump("Function not implemented\n");
}

function EditorShutdown()
{
    // nothing to do. Shutdown is called by the nsEditorBoxObject
}

function SafeSetAttribute(nodeID, attributeName, attributeValue)
{
    var theNode = document.getElementById(nodeID);
    if (theNode)
        theNode.setAttribute(attributeName, attributeValue);
}

function DocumentHasBeenSaved()
{
  var fileurl = "";
  try {
    fileurl = GetDocumentUrl();
  } catch (e) {
    return false;
  }

  if (!fileurl || IsUrlAboutBlank(fileurl))
    return false;

  // We have a file URL already
  return true;
}

function CheckAndSaveDocument(command, allowDontSave)
{
  var document;
  try {
    // if we don't have an editor or an document, bail
    if (!gEditor)
      return true;
    document = gEditor.document;
    if (!document)
      return true;
  } catch (e) { return true; }

  if (!gEditor.documentModified && !gHTMLSourceChanged)
    return true;

  // call window.focus, since we need to pop up a dialog
  // and therefore need to be visible (to prevent user confusion)
  window.focus();  

  var scheme = GetScheme(GetDocumentUrl());
  var doPublish = (scheme && scheme != "file");

  var strID;
  switch (command)
  {
    case "cmd_close":
      strID = "BeforeClosing";
      break;
    case "cmd_preview":
      strID = "BeforePreview";
      break;
    case "cmd_editSendPage":
      strID = "SendPageReason";
      break;
    case "cmd_validate":
      strID = "BeforeValidate";
      break;
  }
    
  var reasonToSave = strID ? GetString(strID) : "";

  var title = document.title;
  if (!title)
    title = GetString("untitled");

  var dialogTitle = GetString(doPublish ? "PublishPage" : "SaveDocument");
  var dialogMsg = GetString(doPublish ? "PublishPrompt" : "SaveFilePrompt");
  dialogMsg = (dialogMsg.replace(/%title%/,title)).replace(/%reason%/,reasonToSave);

  var promptService = GetPromptService();
  if (!promptService)
    return false;

  var result = {value:0};
  var promptFlags = promptService.BUTTON_TITLE_CANCEL * promptService.BUTTON_POS_1;
  var button1Title = null;
  var button3Title = null;

  if (doPublish)
  {
    promptFlags += promptService.BUTTON_TITLE_IS_STRING * promptService.BUTTON_POS_0;
    button1Title = GetString("Publish");
    button3Title = GetString("DontPublish");    
  }
  else
  {
    promptFlags += promptService.BUTTON_TITLE_SAVE * promptService.BUTTON_POS_0;
  }

  // If allowing "Don't..." button, add that
  if (allowDontSave)
    promptFlags += doPublish ?
        (promptService.BUTTON_TITLE_IS_STRING * promptService.BUTTON_POS_2)
        : (promptService.BUTTON_TITLE_DONT_SAVE * promptService.BUTTON_POS_2);
  
  result = promptService.confirmEx(window, dialogTitle, dialogMsg, promptFlags,
                          button1Title, null, button3Title, null, {value:0});

  if (result == 0)
  {
    // Save, but first finish HTML source mode
    if (gHTMLSourceChanged) {
      try {
        FinishHTMLSource();
      } catch (e) { return false;}
    }

    if (doPublish)
    {
      // We save the command the user wanted to do in a global
      // and return as if user canceled because publishing is asynchronous
      // This command will be fired when publishing finishes
      gCommandAfterPublishing = command;
      goDoCommand("cmd_publish");
      return false;
    }

    // Save to local disk
    var contentsMIMEType;
    if (isHTMLEditor())
      contentsMIMEType = kHTMLMimeType;
    else
      contentsMIMEType = kTextMimeType;
    var success = SaveDocument(false, false, contentsMIMEType);
    return success;
  }

  if (result == 2) // "Don't Save"
    return true;

  // Default or result == 1 (Cancel)
  return false;
}

// --------------------------- File menu ---------------------------


// used by openLocation. see openLocation.js for additional notes.
function delayedOpenWindow(chrome, flags, url)
{
  dump("setting timeout\n");
  setTimeout("window.openDialog('"+chrome+"','_blank','"+flags+"','"+url+"')", 10);
}

function EditorNewPlaintext()
{
  window.openDialog( "chrome://editor/content/TextEditorAppShell.xul",
                     "_blank",
                     "chrome,dialog=no,all",
                     "about:blank");
}

// Check for changes to document and allow saving before closing
// This is hooked up to the OS's window close widget (e.g., "X" for Windows)
function EditorCanClose()
{
  // Returns FALSE only if user cancels save action

  // "true" means allow "Don't Save" button
  var canClose = CheckAndSaveDocument("cmd_close", true);

  // This is our only hook into closing via the "X" in the caption
  //   or "Quit" (or other paths?)
  //   so we must shift association to another
  //   editor or close any non-modal windows now
  if (canClose && "InsertCharWindow" in window && window.InsertCharWindow)
    SwitchInsertCharToAnotherEditorOrClose();

  return canClose;
}

// --------------------------- View menu ---------------------------

function EditorSetDocumentCharacterSet(aCharset)
{
  if (gEditor)
  {
    gEditor.documentCharacterSet = aCharset;
    var docUrl = GetDocumentUrl();
    if( !IsUrlAboutBlank(docUrl))
    {
      // reloading the document will reverse any changes to the META charset, 
      // we need to put them back in, which is achieved by a dedicated listener
      gEditor.addDocumentStateListener( DocumentReloadListener );
      editorShell.LoadUrl(docUrl);
    }
  }
}

// ------------------------------------------------------------------
function updateCharsetPopupMenu(menuPopup)
{
  if (gEditor.documentModified && !gEditor.documentIsEmpty)
  {
    for (var i = 0; i < menuPopup.childNodes.length; i++)
    {
      var menuItem = menuPopup.childNodes[i];
      menuItem.setAttribute('disabled', 'true');
    }
  }
}

// --------------------------- Text style ---------------------------

function onParagraphFormatChange(paraMenuList, commandID)
{
  if (!paraMenuList)
    return;

  var commandNode = document.getElementById(commandID);
  var state = commandNode.getAttribute("state");

  // force match with "normal"
  if (state == "body")
    state = "";

  if (state == "mixed")
  {
    //Selection is the "mixed" ( > 1 style) state
    paraMenuList.selectedItem = null;
    paraMenuList.setAttribute("label",GetString('Mixed'));
  }
  else
  {
    var menuPopup = document.getElementById("ParagraphPopup");
    var menuItems = menuPopup.childNodes;
    for (var i=0; i < menuItems.length; i++)
    {
      var menuItem = menuItems.item(i);
      if ("value" in menuItem && menuItem.value == state)
      {
        paraMenuList.selectedItem = menuItem;
        break;
      }
    }
  }
}

function onFontFaceChange(fontFaceMenuList, commandID)
{
  var commandNode = document.getElementById(commandID);
  var state = commandNode.getAttribute("state");

  if (state == "mixed")
  {
    //Selection is the "mixed" ( > 1 style) state
    fontFaceMenuList.selectedItem = null;
    fontFaceMenuList.setAttribute("label",GetString('Mixed'));
  }
  else
  {
    var menuPopup = document.getElementById("FontFacePopup");
    var menuItems = menuPopup.childNodes;
    for (var i=0; i < menuItems.length; i++)
    {
      var menuItem = menuItems.item(i);
      if (menuItem.getAttribute("label") && ("value" in menuItem && menuItem.value.toLowerCase() == state.toLowerCase()))
      {
        fontFaceMenuList.selectedItem = menuItem;
        break;
      }
    }
  }
}

function EditorSelectFontSize()
{
  var select = document.getElementById("FontSizeSelect");
  if (select)
  {
    if (select.selectedIndex == -1)
      return;

    EditorSetFontSize(gFontSizeNames[select.selectedIndex]);
  }
}

function onFontSizeChange(fontSizeMenulist, commandID)
{
  // If we don't match anything, set to "0 (normal)"
  var newIndex = 2;
  var size = fontSizeMenulist.getAttribute("size");
  if ( size == "mixed")
  {
    // No single type selected
    newIndex = -1;
  }
  else
  {
    for (var i = 0; i < gFontSizeNames.length; i++)
    {
      if( gFontSizeNames[i] == size )
      {
        newIndex = i;
        break;
      }
    }
  }
  if (fontSizeMenulist.selectedIndex != newIndex)
    fontSizeMenulist.selectedIndex = newIndex;
}

function EditorSetFontSize(size)
{
  if( size == "0" || size == "normal" ||
      size == "medium" )
  {
    EditorRemoveTextProperty("font", "size");
    // Also remove big and small,
    //  else it will seem like size isn't changing correctly
    EditorRemoveTextProperty("small", "");
    EditorRemoveTextProperty("big", "");
  } else {
    // Temp: convert from new CSS size strings to old HTML size strings
    switch (size)
    {
      case "xx-small":
      case "x-small":
        size = "-2";
        break;
      case "small":
        size = "-1";
        break;
      case "large":
        size = "+1";
        break;
      case "x-large":
        size = "+2";
        break;
      case "xx-large":
        size = "+3";
        break;
    }
    EditorSetTextProperty("font", "size", size);
  }
  gContentWindow.focus();
}

function initFontFaceMenu(menuPopup)
{
  if (menuPopup)
  {
    var children = menuPopup.childNodes;
    if (!children) return;

    var firstHas = { value: false };
    var anyHas = { value: false };
    var allHas = { value: false };

    // we need to set or clear the checkmark for each menu item since the selection
    // may be in a new location from where it was when the menu was previously opened

    // Fixed width (second menu item) is special case: old TT ("teletype") attribute
    EditorGetTextProperty("tt", "", "", firstHas, anyHas, allHas);
    children[1].setAttribute("checked", allHas.value);
    var fontWasFound = anyHas.value;

    // Skip over default, TT, and separator
    for (var i = 3; i < children.length; i++)
    {
      var menuItem = children[i];
      var faceType = menuItem.getAttribute("value");

      if (faceType)
      {
        EditorGetTextProperty("font", "face", faceType, firstHas, anyHas, allHas);

        // Check the item only if all of selection has the face...
        menuItem.setAttribute("checked", allHas.value);
        // ...but remember if ANY part of the selection has it
        fontWasFound |= anyHas.value;
      }
    }
    // Check the default item if no other item was checked
    // note that no item is checked in the case of "mixed" selection
    children[0].setAttribute("checked", !fontWasFound);
  }
}

function initFontSizeMenu(menuPopup)
{
  if (menuPopup)
  {
    var children = menuPopup.childNodes;
    if (!children) return;

    var firstHas = { value: false };
    var anyHas = { value: false };
    var allHas = { value: false };

    var sizeWasFound = false;

    // we need to set or clear the checkmark for each menu item since the selection
    // may be in a new location from where it was when the menu was previously opened

    // First 2 items add <small> and <big> tags
    // While it would be better to show the number of levels,
    //  at least this tells user if either of them are set
    var menuItem = children[0];
    if (menuItem)
    {
      EditorGetTextProperty("small", "", "", firstHas, anyHas, allHas);
      menuItem.setAttribute("checked", allHas.value);
      sizeWasFound = anyHas.value;
    }

    menuItem = children[1];
    if (menuItem)
    {
      EditorGetTextProperty("big", "", "", firstHas, anyHas, allHas);
      menuItem.setAttribute("checked", allHas.value);
      sizeWasFound |= anyHas.value;
    }

    // Fixed size items start after menu separator
    var menuIndex = 3;
    // Index of the medium (default) item
    var mediumIndex = 5;

    // Scan through all supported "font size" attribute values
    for (var i = -2; i <= 3; i++)
    {
      menuItem = children[menuIndex];

      // Skip over medium since it'll be set below.
      // If font size=0 is actually set, we'll toggle it off below if
      // we enter this loop in this case.
      if (menuItem && (i != 0))
      {
        var sizeString = (i <= 0) ? String(i) : ("+" + String(i));
        EditorGetTextProperty("font", "size", sizeString, firstHas, anyHas, allHas);
        // Check the item only if all of selection has the size...
        menuItem.setAttribute("checked", allHas.value);
        // ...but remember if ANY of of selection had size set
        sizeWasFound |= anyHas.value;
      }
      menuIndex++;
    }

    // if no size was found, then check default (medium)
    // note that no item is checked in the case of "mixed" selection
    children[mediumIndex].setAttribute("checked", !sizeWasFound);
  }
 }

function onHighlightColorChange()
{
  var commandNode = document.getElementById("cmd_highlight");
  if (commandNode)
  {
    var color = commandNode.getAttribute("state");
    var button = document.getElementById("HighlightColorButton");
    if (button)
    {
      // No color set - get color set on page or other defaults
      if (!color)
        color = "transparent" ;

      button.setAttribute("style", "background-color:"+color+" !important");
    }
  }
}

function onFontColorChange()
{
  var commandNode = document.getElementById("cmd_fontColor");
  if (commandNode)
  {
    var color = commandNode.getAttribute("state");
    var button = document.getElementById("TextColorButton");
    if (button)
    {
      // No color set - get color set on page or other defaults
      if (!color)
        color = gDefaultTextColor;

      button.setAttribute("style", "background-color:"+color);
    }
  }
}

function onBackgroundColorChange()
{
  var commandNode = document.getElementById("cmd_backgroundColor");
  if (commandNode)
  {
    var color = commandNode.getAttribute("state");
    var button = document.getElementById("BackgroundColorButton");
    if (button)
    {
      if (!color)
        color = gDefaultBackgroundColor;

      button.setAttribute("style", "background-color:"+color);
    }
  }
}

// Call this when user changes text and/or background colors of the page
function UpdateDefaultColors()
{
  var BrowserColors = GetDefaultBrowserColors();
  var bodyelement = GetBodyElement();
  var defTextColor = gDefaultTextColor;
  var defBackColor = gDefaultBackgroundColor;

  if (bodyelement)
  {
    var color = bodyelement.getAttribute("text");
    if (color)
      gDefaultTextColor = color;
    else if (BrowserColors)
      gDefaultTextColor = BrowserColors.TextColor;

    color = bodyelement.getAttribute("bgcolor");
    if (color)
      gDefaultBackgroundColor = color;
    else if (BrowserColors)
      gDefaultBackgroundColor = BrowserColors.BackgroundColor;
  }

  // Trigger update on toolbar
  if (defTextColor != gDefaultTextColor)
  {
    goUpdateCommand("cmd_fontColor");
    onFontColorChange();
  }
  if (defBackColor != gDefaultBackgroundColor)
  {
    goUpdateCommand("cmd_backgroundColor");
    onBackgroundColorChange();
  }
}

function GetBackgroundElementWithColor()
{
  gColorObj.Type = "";
  gColorObj.PageColor = "";
  gColorObj.TableColor = "";
  gColorObj.CellColor = "";
  gColorObj.BackgroundColor = "";
  gColorObj.SelectedType = "";

  var tagNameObj = { value: "" };
  var element;
  try {
    element = gEditor.getSelectedOrParentTableElement(tagNameObj, {value:0});
  }
  catch(e) {}

  if (element && tagNameObj && tagNameObj.value)
  {
    gColorObj.BackgroundColor = GetHTMLOrCSSStyleValue(element, "bgcolor", "background-color");
    gColorObj.BackgroundColor = ConvertRGBColorIntoHEXColor(gColorObj.BackgroundColor);
    if (tagNameObj.value.toLowerCase() == "td")
    {
      gColorObj.Type = "Cell";
      gColorObj.CellColor = gColorObj.BackgroundColor;

      // Get any color that might be on parent table
      var table = GetParentTable(element);
      gColorObj.TableColor = GetHTMLOrCSSStyleValue(table, "bgcolor", "background-color");
      gColorObj.TableColor = ConvertRGBColorIntoHEXColor(gColorObj.TableColor);
    }
    else
    {
      gColorObj.Type = "Table";
      gColorObj.TableColor = gColorObj.BackgroundColor;
    }
    gColorObj.SelectedType = gColorObj.Type;
  }
  else
  {
    var prefs = GetPrefs();
    var IsCSSPrefChecked = prefs.getBoolPref("editor.use_css");
    if (IsCSSPrefChecked && isHTMLEditor())
    {
      var selection = gEditor.selection;
      if (selection)
      {
        element = selection.focusNode;
        while (!gEditor.nodeIsBlock(element))
          element = element.parentNode;
      }
      else
      {
        element = GetBodyElement();
      }
    }
    else
    {
      element = GetBodyElement();
    }
    if (element)
    {
      gColorObj.Type = "Page";
      gColorObj.BackgroundColor = GetHTMLOrCSSStyleValue(element, "bgcolor", "background-color");
      if (gColorObj.BackgroundColor == "")
      {
        gColorObj.BackgroundColor = "transparent";
      }
      else
      {
        gColorObj.BackgroundColor = ConvertRGBColorIntoHEXColor(gColorObj.BackgroundColor);
      }
      gColorObj.PageColor = gColorObj.BackgroundColor;
    }
  }
  return element;
}

function SetSmiley(smileyText)
{
  try {
    gEditor.InsertText(smileyText);
    gContentWindow.focus();
  }
  catch(e) {}
}

function EditorSelectColor(colorType, mouseEvent)
{
  if (!gColorObj)
    return;

  // Shift + mouse click automatically applies last color, if available
  var useLastColor = mouseEvent ? ( mouseEvent.button == 0 && mouseEvent.shiftKey ) : false;
  var element;
  var table;
  var currentColor = "";
  var commandNode;

  if (!colorType)
    colorType = "";

  if (colorType == "Text")
  {
    gColorObj.Type = colorType;

    // Get color from command node state
    commandNode = document.getElementById("cmd_fontColor");
    currentColor = commandNode.getAttribute("state");
    gColorObj.TextColor = currentColor;

    if (useLastColor && gColorObj.LastTextColor )
      gColorObj.TextColor = gColorObj.LastTextColor;
    else
      useLastColor = false;
  }
  else if (colorType == "Highlight")
  {
    gColorObj.Type = colorType;

    // Get color from command node state
    commandNode = document.getElementById("cmd_highlight");
    currentColor = commandNode.getAttribute("state");
    gColorObj.HighlightColor = currentColor;

    if (useLastColor && gColorObj.LastHighlightColor )
      gColorObj.HighlightColor = gColorObj.LastHighlightColor;
    else
      useLastColor = false;
  }
  else
  {
    element = GetBackgroundElementWithColor();
    if (!element)
      return;

    // Get the table if we found a cell
    if (gColorObj.Type == "Table")
      table = element;
    else if (gColorObj.Type == "Cell")
      table = GetParentTable(element);

    // Save to avoid resetting if not necessary
    currentColor = gColorObj.BackgroundColor;

    if (colorType == "TableOrCell" || colorType == "Cell")
    {
      if (gColorObj.Type == "Cell")
        gColorObj.Type = colorType;
      else if (gColorObj.Type != "Table")
        return;
    }
    else if (colorType == "Table" && gColorObj.Type == "Page")
      return;

    if (colorType == "" && gColorObj.Type == "Cell")
    {
      // Using empty string for requested type means
      //  we can let user select cell or table
      gColorObj.Type = "TableOrCell";
    }

    if (useLastColor && gColorObj.LastBackgroundColor )
      gColorObj.BackgroundColor = gColorObj.LastBackgroundColor;
    else
      useLastColor = false;
  }
  // Save the type we are really requesting
  colorType = gColorObj.Type;

  if (!useLastColor)
  {
    // Avoid the JS warning
    gColorObj.NoDefault = false;

    // Launch the ColorPicker dialog
    // TODO: Figure out how to position this under the color buttons on the toolbar
    window.openDialog("chrome://editor/content/EdColorPicker.xul", "_blank", "chrome,close,titlebar,modal", "", gColorObj);

    // User canceled the dialog
    if (gColorObj.Cancel)
      return;
  }

  if (gColorObj.Type == "Text")
  {
    if (currentColor != gColorObj.TextColor)
    {
      if (gColorObj.TextColor)
        EditorSetTextProperty("font", "color", gColorObj.TextColor);
      else
        EditorRemoveTextProperty("font", "color");
    }
    // Update the command state (this will trigger color button update)
    goUpdateCommand("cmd_fontColor");
  }
  else if (gColorObj.Type == "Highlight")
  {
    if (currentColor != gColorObj.HighlightColor)
    {
      if (gColorObj.HighlightColor)
        EditorSetTextProperty("font", "bgcolor", gColorObj.HighlightColor);
      else
        EditorRemoveTextProperty("font", "bgcolor");
    }
    // Update the command state (this will trigger color button update)
    goUpdateCommand("cmd_highlight");
  }
  else if (element)
  {
    if (gColorObj.Type == "Table")
    {
      // Set background on a table
      // Note that we shouldn't trust "currentColor" because of "TableOrCell" behavior
      if (table)
      {
        var bgcolor = table.getAttribute("bgcolor");
        if (bgcolor != gColorObj.BackgroundColor)
        {
          if (gColorObj.BackgroundColor)
            gEditor.setAttributeOrEquivalent(table, "bgcolor", gColorObj.BackgroundColor, false);
          else
            gEditor.removeAttributeOrEquivalent(table, "bgcolor", false);
        }
      }
    }
    else if (currentColor != gColorObj.BackgroundColor && isHTMLEditor())
    {
      gEditor.beginTransaction();
      try
      {
        gEditor.setBackgroundColor(gColorObj.BackgroundColor);

        if (gColorObj.Type == "Page" && gColorObj.BackgroundColor)
        {
          // Set all page colors not explicitly set,
          //  else you can end up with unreadable pages
          //  because viewer's default colors may not be same as page author's
          var bodyelement = GetBodyElement();
          if (bodyelement)
          {
            var defColors = GetDefaultBrowserColors();
            if (defColors)
            {
              if (!bodyelement.getAttribute("text"))
                gEditor.setAttributeOrEquivalent(bodyelement, "text", defColors.TextColor, false);

              // The following attributes have no individual CSS declaration counterparts
              // Getting rid of them in favor of CSS implies CSS rules management
              if (!bodyelement.getAttribute("link"))
                gEditor.setAttribute(bodyelement, "link", defColors.LinkColor);

              if (!bodyelement.getAttribute("alink"))
                gEditor.setAttribute(bodyelement, "alink", defColors.LinkColor);

              if (!bodyelement.getAttribute("vlink"))
                gEditor.setAttribute(bodyelement, "vlink", defColors.VisitedLinkColor);
            }
          }
        }
      }
      catch(e) {}

      gEditor.endTransaction();
    }

    goUpdateCommand("cmd_backgroundColor");
  }
  gContentWindow.focus();
}

function GetParentTable(element)
{
  var node = element;
  while (node)
  {
    if (node.nodeName.toLowerCase() == "table")
      return node;

    node = node.parentNode;
  }
  return node;
}

function GetParentTableCell(element)
{
  var node = element;
  while (node)
  {
    if (node.nodeName.toLowerCase() == "td" || node.nodeName.toLowerCase() == "th")
      return node;

    node = node.parentNode;
  }
  return node;
}

function EditorDblClick(event)
{
  if (event.target)
  {
    // Only bring up properties if clicked on an element or selected link
    var element;
    try {
      element = event.target.QueryInterface(Components.interfaces.nsIDOMElement);
    } catch (e) {}

     //  We use "href" instead of "a" to not be fooled by named anchor
    if (!element)
      element = gEditor.getSelectedElement("href");

    if (element)
    {
      goDoCommand("cmd_objectProperties");  
      event.preventDefault();
    }
  }
}

function EditorClick(event)
{
  if (!event)
    return;

  if (event.detail == 2)
  {
    EditorDblClick(event);
    return;
  }

  // For Web Composer: In Show All Tags Mode,
  // single click selects entire element,
  //  except for body and table elements
  if (event.target && IsWebComposer()
      && isHTMLEditor() && gEditorDisplayMode == kDisplayModeAllTags)
  {
    try
    {
      var element = event.target.QueryInterface( Components.interfaces.nsIDOMElement);
      var name = element.localName.toLowerCase();
      if (name != "body" && name != "table" &&
          name != "td" && name != "th" && name != "caption" && name != "tr")
      {          
        gEditor.selectElement(event.target);
        event.preventDefault();
      }
    } catch (e) {}
  }
}

/*TODO: We need an oncreate hook to do enabling/disabling for the
        Format menu. There should be code like this for the
        object-specific "Properties" item
*/
// For property dialogs, we want the selected element,
//  but will accept a parent link, list, or table cell if inside one
function GetObjectForProperties()
{
  if (!gEditor || !isHTMLEditor())
    return null;

  var element = gEditor.getSelectedElement("");
  if (element)
    return element;

  // Find nearest parent of selection anchor node
  //   that is a link, list, table cell, or table

  var anchorNode = gEditor.selection.anchorNode;
  if (!anchorNode) return null;
  var node;
  if (anchorNode.firstChild)
  {
    // Start at actual selected node
    var offset = gEditor.selection.anchorOffset;
    // Note: If collapsed, offset points to element AFTER caret,
    //  thus node may be null
    node = anchorNode.childNodes.item(offset);
  }
  if (!node)
    node = anchorNode;

  while (node)
  {
    if (node.nodeName)
    {
      var nodeName = node.nodeName.toLowerCase();

      // Done when we hit the body
      if (nodeName == "body") break;

      if ((nodeName == "a" && node.href) ||
          nodeName == "ol" || nodeName == "ul" || nodeName == "dl" ||
          nodeName == "td" || nodeName == "th" ||
          nodeName == "table")
      {
        return node;
      }
    }
    node = node.parentNode;
  }
  return null;
}

function SetEditMode(mode)
{
  if (!isHTMLEditor())
    return;

  var bodyNode = gEditor.document.getElementsByTagName("body").item(0);
  if (!bodyNode)
  {
    dump("SetEditMode: We don't have a body node!\n");
    return;
  }

  // Switch the UI mode before inserting contents
  //   so user can't type in source window while new window is being filled
  var previousMode = gEditorDisplayMode;
  if (!SetDisplayMode(mode))
    return;

  if (mode == kDisplayModeSource)
  {
    // Display the DOCTYPE as a non-editable string above edit area
    var domdoc;
    try { domdoc = gEditor.document; } catch (e) { dump( e + "\n");}
    if (domdoc)
    {
      var doctypeNode = document.getElementById("doctype-text");
      var dt = domdoc.doctype;
      if (doctypeNode)
      {
        if (dt)
        {
          doctypeNode.removeAttribute("collapsed");
          var doctypeText = "<!DOCTYPE " + domdoc.doctype.name;
          if (dt.publicId)
            doctypeText += " PUBLIC \"" + domdoc.doctype.publicId;
          if (dt.systemId)
            doctypeText += " "+"\"" + dt.systemId;
          doctypeText += "\">"
          doctypeNode.setAttribute("value", doctypeText);
        }
        else
          doctypeNode.setAttribute("collapsed", "true");
      }
    }
    // Get the entire document's source string

    var flags = 256; // OutputEncodeEntities;

    try { 
      var prettyPrint = gPrefs.getBoolPref("editor.prettyprint");
      if (prettyPrint)
        flags |= 2; // OutputFormatted

    } catch (e) {}

    var source = gEditor.outputToString(kHTMLMimeType, flags);
    var start = source.search(/<html/i);
    if (start == -1) start = 0;
    gSourceContentWindow.value = source.slice(start);
    gSourceContentWindow.focus();

    // Add input handler so we know if user made any changes
    gSourceContentWindow.addEventListener("input", oninputHTMLSource, false);
    gHTMLSourceChanged = false;
  }
  else if (previousMode == kDisplayModeSource)
  {
    // Only rebuild document if a change was made in source window
    if (gHTMLSourceChanged)
    {
      // Reduce the undo count so we don't use too much memory
      //   during multiple uses of source window 
      //   (reinserting entire doc caches all nodes)
      try {
        gEditor.transactionManager.maxTransactionCount = 1;
      } catch (e) {}

      gEditor.beginTransaction();
      try {
        // We are comming from edit source mode,
        //   so transfer that back into the document
        source = gSourceContentWindow.value;
        gEditor.rebuildDocumentFromSource(source);

        // Get the text for the <title> from the newly-parsed document
        // (must do this for proper conversion of "escaped" characters)
        var title = "";
        var titlenodelist = gEditor.document.getElementsByTagName("title");
        if (titlenodelist)
        {
          var titleNode = titlenodelist.item(0);
          if (titleNode && titleNode.firstChild && titleNode.firstChild.data)
            title = titleNode.firstChild.data;
        }
        if (gEditor.document.title != title)
          SetDocumentTitle(title);

      } catch (ex) {
        dump(ex);
      }
      gEditor.endTransaction();

      // Restore unlimited undo count
      try {
        gEditor.transactionManager.maxTransactionCount = -1;
      } catch (e) {}
    } else {
      // We don't need to call this again, so remove handler
      gSourceContentWindow.removeEventListener("input", oninputHTMLSource, false);
    }
    gHTMLSourceChanged = false;

    // Clear out the string buffers
    gSourceContentWindow.value = null;

    gContentWindow.focus();
  }
}

function oninputHTMLSource()
{
  gHTMLSourceChanged = true;

  // Trigger update of "Save" and "Publish" buttons
  goUpdateCommand("cmd_save");
  goUpdateCommand("cmd_publish");

  // We don't need to call this again, so remove handler
  gSourceContentWindow.removeEventListener("input", oninputHTMLSource, false);
}

function CancelHTMLSource()
{
  // Don't convert source text back into the DOM document
  gSourceContentWindow.value = "";
  gHTMLSourceChanged = false;
  SetDisplayMode(gPreviousNonSourceDisplayMode);
}

function FinishHTMLSource()
{
  //Here we need to check whether the HTML source contains <head> and <body> tags
  //Or RebuildDocumentFromSource() will fail.
  if (IsInHTMLSourceMode())
  {
    var htmlSource = gSourceContentWindow.value;
    if (htmlSource.length > 0)
    {
      var beginHead = htmlSource.indexOf("<head");
      if (beginHead == -1)
      {
        AlertWithTitle(GetString("Alert"), GetString("NoHeadTag"));
        //cheat to force back to Source Mode
        gEditorDisplayMode = kDisplayModePreview;
        SetDisplayMode(kDisplayModeSource);
        throw Components.results.NS_ERROR_FAILURE;
      }

      var beginBody = htmlSource.indexOf("<body");
      if (beginBody == -1)
      {
        AlertWithTitle(GetString("Alert"), GetString("NoBodyTag"));
        //cheat to force back to Source Mode
        gEditorDisplayMode = kDisplayModePreview;
        SetDisplayMode(kDisplayModeSource);
        throw Components.results.NS_ERROR_FAILURE;
      }
    }
  }

  // Switch edit modes -- converts source back into DOM document
  SetEditMode(gPreviousNonSourceDisplayMode);
}

function CollapseItem(id, collapse)
{
  var item = document.getElementById(id);
  if (item)
  {
    if(collapse != (item.getAttribute("collapsed") == "true"))
      item.setAttribute("collapsed", collapse ? "true" : "");
  }
}

function SetDisplayMode(mode)
{
  if (!isHTMLEditor())
    return false;

  // Already in requested mode:
  //  return false to indicate we didn't switch
  if (mode == gEditorDisplayMode)
    return false;

  var previousMode = gEditorDisplayMode;
  gEditorDisplayMode = mode;

  ResetStructToolbar();
  if (mode == kDisplayModeSource)
  {
    // Switch to the sourceWindow (second in the deck)
    gContentWindowDeck.selectedIndex = 1;

    //Hide the formatting toolbar if not already hidden
    gFormatToolbarHidden = gFormatToolbar.hidden;
    gFormatToolbar.hidden = true;
    gViewFormatToolbar.hidden = true;

    gSourceContentWindow.focus();
  }
  else
  {
    // Save the last non-source mode so we can cancel source editing easily
    gPreviousNonSourceDisplayMode = mode;

    // Load/unload appropriate override style sheet
    try {
      var editor = GetCurrentEditor();

      switch (mode)
      {
        case kDisplayModePreview:
          // Disable all extra "edit mode" style sheets 
          editor.enableStyleSheet(kNormalStyleSheet, false);
          editor.enableStyleSheet(kAllTagsStyleSheet, false);
          break;

        case kDisplayModeNormal:
          editor.addOverrideStyleSheet(kNormalStyleSheet);
          // Disable ShowAllTags mode
          editor.enableStyleSheet(kAllTagsStyleSheet, false);
          break;

        case kDisplayModeAllTags:
          editor.addOverrideStyleSheet(kNormalStyleSheet);
          editor.addOverrideStyleSheet(kAllTagsStyleSheet);
          break;
      }
    } catch(e) {}

    // Switch to the normal editor (first in the deck)
    gContentWindowDeck.selectedIndex = 0;

    // Restore menus and toolbars
    gFormatToolbar.hidden = gFormatToolbarHidden;
    gViewFormatToolbar.hidden = false;

    gContentWindow.focus();
  }

  // update commands to disable or re-enable stuff
  window.updateCommands("mode_switch");

  // Set the selected tab at bottom of window:
  // (Note: Setting "selectedIndex = mode" won't redraw tabs when menu is used.)
  document.getElementById("EditModeTabs").selectedItem = document.getElementById(kDisplayModeTabIDS[mode]);

  // Uncheck previous menuitem and set new check since toolbar may have been used
  if (previousMode >= 0)
    document.getElementById(kDisplayModeMenuIDs[previousMode]).setAttribute("checked","false");
  document.getElementById(kDisplayModeMenuIDs[mode]).setAttribute("checked","true");
  

  return true;
}

function EditorToggleParagraphMarks()
{
  var menuItem = document.getElementById("viewParagraphMarks");
  if (menuItem)
  {
    // Note that the 'type="checbox"' mechanism automatically
    //  toggles the "checked" state before the oncommand is called,
    //  so if "checked" is true now, it was just switched to that mode
    var checked = menuItem.getAttribute("checked");
    try {
      if (checked == "true")
        GetCurrentEditor().addOverrideStyleSheet(kParagraphMarksStyleSheet);
      else
        GetCurrentEditor().enableStyleSheet(kParagraphMarksStyleSheet, false);
    }
    catch(e) { return; }
  }
}

function InitPasteAsMenu()
{
  var menuItem = document.getElementById("menu_pasteTable")
  if(menuItem)
  {
    menuItem.IsInTable  
    menuItem.setAttribute("label", GetString(IsInTable() ? "NestedTable" : "Table"));
   // menuItem.setAttribute("accesskey",GetString("ObjectPropertiesAccessKey"));
  }
  // TODO: Do enabling based on what is in the clipboard
}

function UpdateWindowTitle()
{
  try {
    var windowTitle = GetDocumentTitle();
    if (!windowTitle)
      windowTitle = GetString("untitled");

    // Append just the 'leaf' filename to the Doc. Title for the window caption
    var docUrl = GetDocumentUrl();
    if (!IsUrlAboutBlank(docUrl))
    {
      var scheme = GetScheme(docUrl);
      var filename = GetFilename(docUrl);
      if (filename)
        windowTitle += " [" + scheme + ":/.../" + filename + "]";
    }
    // Set window title with " - Composer" appended
    var xulWin = document.documentElement;
    window.title = windowTitle + xulWin.getAttribute("titlemenuseparator") + xulWin.getAttribute("titlemodifier");

    // Save changed title in the recent pages data in prefs
    SaveRecentFilesPrefs();
  } catch (e) {}
}

function BuildRecentMenu()
{
  // Can't do anything if no prefs
  if (!gPrefs) return;

  var popup = document.getElementById("menupopup_RecentFiles");
  if (!popup || !gEditor || !gEditor.document)
    return;

  // Delete existing menu
  while (popup.firstChild)
    popup.removeChild(popup.firstChild);

  // Current page is the "0" item in the list we save in prefs,
  //  but we don't include it in the menu.
  var curTitle = gEditor.document.title;
  var curUrl = StripPassword(GetDocumentUrl());
  var historyCount = 10;
  try { historyCount = gPrefs.getIntPref("editor.history.url_maximum"); } catch(e) {}
  var menuIndex = 1;
  var i;
  var disableMenu = true;

  for (i = 0; i < historyCount; i++)
  {
    var title = GetUnicharPref("editor.history_title_"+i);
    var url = GetUnicharPref("editor.history_url_"+i);

    // Continue if URL pref is missing because 
    //  a URL not found during loading may have been removed
    // Also skip "data:" URL
    if (!url || GetScheme(url) == "data")
      continue;

    // Skip over current URL
    if (url != curUrl)
    {

      // Build the menu
      AppendRecentMenuitem(popup, title, url, menuIndex);
      menuIndex++;
      disableMenu = false;

    }
  }

  // Disable menu item if no entries
  SetElementEnabledById("menu_RecentFiles", !disableMenu);
}

function SaveRecentFilesPrefs()
{
  // Can't do anything if no prefs
  if (!gPrefs) return;

  // Nothing will change, so don't bother doing the work
  if(IsUrlAboutBlank(curUrl))
    return;

  var curTitle = gEditor.document.title;
  var curUrl = StripPassword(GetDocumentUrl());
  var historyCount = 10;
  try { historyCount = gPrefs.getIntPref("editor.history.url_maximum"); } catch(e) {}
  var titleArray = new Array(historyCount);
  var urlArray   = new Array(historyCount);
  var arrayIndex = 0;
  var i;


  // Always put latest-opened URL at start of array
  titleArray[arrayIndex] = curTitle;
  urlArray[arrayIndex] = curUrl;
  arrayIndex++;

  for (i = 0; i < historyCount; i++)
  {
    var title = GetUnicharPref("editor.history_title_"+i);
    var url = GetUnicharPref("editor.history_url_"+i);

    // Continue if URL pref is missing because 
    //  a URL not found during loading may have been removed
    // Also skip "data:" URL
    if (!url || GetScheme(url) == "data")
      continue;

    // Skip over current URL
    if (url != curUrl)
    {
      // Save in array for prefs
      if (arrayIndex < historyCount)
      {
        titleArray[arrayIndex] = title;
        urlArray[arrayIndex] = url;
        arrayIndex++;
      }
    }
  }

  // Resave the list back to prefs in the new order
  for (i = 0; i < historyCount; i++)
  {
    if (!urlArray[i])
      break;
    SetUnicharPref("editor.history_title_"+i, titleArray[i]);
    SetUnicharPref("editor.history_url_"+i, urlArray[i]);
  }
/*
  // Force saving to file so next file opened finds these values
  var prefsService = Components.classes["@mozilla.org/preferences-service;1"]
                               .getService(Components.interfaces.nsIPrefService);
  prefsService.savePrefFile(null);
*/
}

function AppendRecentMenuitem(menupopup, title, url, menuIndex)
{
  if (menupopup)
  {
    var menuItem = document.createElementNS(XUL_NS, "menuitem");
    if (menuItem)
    {
      var accessKey;
      if (menuIndex <= 9)
        accessKey = String(menuIndex);
      else if (menuIndex == 10)
        accessKey = "0";
      else
        accessKey = " ";

      var itemString = accessKey+" ";

      // Show "title [url]" or just the URL
      if (title)
      {
       itemString += title;
       itemString += " [";
      }
      itemString += url;
      if (title)
        itemString += "]";

      menuItem.setAttribute("label", itemString);
      menuItem.setAttribute("value", url);
      if (accessKey != " ")
        menuItem.setAttribute("accesskey", accessKey);
      menupopup.appendChild(menuItem);
    }
  }
}

function EditorInitFileMenu()
{
  // Disable "Save" menuitem when editing remote url. User should use "Save As"
  var scheme = GetScheme(GetDocumentUrl());
  if (scheme && scheme != "file")
    SetElementEnabledById("saveMenuitem", false);
}

function EditorInitFormatMenu()
{
  try {
    InitObjectPropertiesMenuitem("objectProperties");
    InitRemoveStylesMenuitems("removeStylesMenuitem", "removeLinksMenuitem", "removeNamedAnchorsMenuitem");
  } catch(ex) {}
  // Set alignment check
}

function InitObjectPropertiesMenuitem(id)
{
  // Set strings and enable for the [Object] Properties item
  // Note that we directly do the enabling instead of
  //  using goSetCommandEnabled since we already have the menuitem
  var menuItem = document.getElementById(id);
  if (!menuItem) return null;

  var element;
  var menuStr = GetString("AdvancedProperties");
  var name;

  if (IsEditingRenderedHTML())
    element = GetObjectForProperties();

  if (element && element.nodeName)
  {
    var objStr = "";
    menuItem.setAttribute("disabled", "");
    name = element.nodeName.toLowerCase();
    switch (name)
    {
      case "img":
        // Check if img is enclosed in link
        //  (use "href" to not be fooled by named anchor)
        try
        {
          if (gEditor.getElementOrParentByTagName("href", element))
            objStr = GetString("ImageAndLink");
        } catch(e) {}
        
        if (objStr == "")
          objStr = GetString("Image");
        break;
      case "hr":
        objStr = GetString("HLine");
        break;
      case "table":
        objStr = GetString("Table");
        break;
      case "th":
        name = "td";
      case "td":
        objStr = GetString("TableCell");
        break;
      case "ol":
      case "ul":
      case "dl":
        objStr = GetString("List");
        break;
      case "li":
        objStr = GetString("ListItem");
        break;
      case "form":
        objStr = GetString("Form");
        break;
      case "input":
        var type = element.getAttribute("type");
        if (type && type.toLowerCase() == "image")
          objStr = GetString("InputImage");
        else
          objStr = GetString("InputTag");
        break;
      case "textarea":
        objStr = GetString("TextArea");
        break;
      case "select":
        objStr = GetString("Select");
        break;
      case "button":
        objStr = GetString("Button");
        break;
      case "label":
        objStr = GetString("Label");
        break;
      case "fieldset":
        objStr = GetString("FieldSet");
        break;
      case "a":
        if (element.name)
        {
          objStr = GetString("NamedAnchor");
          name = "anchor";
        }
        else if(element.href)
        {
          objStr = GetString("Link");
          name = "href";
        }
        break;
    }
    if (objStr)
      menuStr = GetString("ObjectProperties").replace(/%obj%/,objStr);
  }
  else
  {
    // We show generic "Properties" string, but disable menu item
    menuItem.setAttribute("disabled","true");
  }
  menuItem.setAttribute("label", menuStr);
  menuItem.setAttribute("accesskey",GetString("ObjectPropertiesAccessKey"));
  return name;
}

function InitParagraphMenu()
{
  var mixedObj = { value: null };
  var state;
  try {
    state = gEditor.getParagraphState(mixedObj);
  }
  catch(e) {}
  var IDSuffix;

  // PROBLEM: When we get blockquote, it masks other styles contained by it
  // We need a separate method to get blockquote state

  // We use "x" as uninitialized paragraph state
  if (!state || state == "x")
    IDSuffix = "bodyText" // No paragraph container
  else
    IDSuffix = state;

  // Set "radio" check on one item, but...
  var menuItem = document.getElementById("menu_"+IDSuffix);
  menuItem.setAttribute("checked", "true");

  // ..."bodyText" is returned if mixed selection, so remove checkmark
  if (mixedObj.value)
    menuItem.setAttribute("checked", "false");
}

function GetListStateString()
{
  var mixedObj = { value: null };
  var hasOL = { value: false };
  var hasUL = { value: false };
  var hasDL = { value: false };
  gEditor.getListState(mixedObj, hasOL, hasUL, hasDL);

  if (mixedObj.value)
    return "mixed";
  if (hasOL.value)
    return "ol";
  if (hasUL.value)
    return "ul";

  if (hasDL.value)
  {
    var hasLI = { value: false };
    var hasDT = { value: false };
    var hasDD = { value: false };
    gEditor.getListItemState(mixedObj, hasLI, hasDT, hasDD);
    if (mixedObj.value)
      return "mixed";
    if (hasLI.value)
      return "li";
    if (hasDT.value)
      return "dt";
    if (hasDD.value)
      return "dd";
  }

  // return "noList" if we aren't in a list at all
  return "noList";
}

function InitListMenu()
{
  if (!isHTMLEditor())
    return;

  var IDSuffix = GetListStateString();

  // Set enable state for the "None" menuitem
  goSetCommandEnabled("cmd_removeList", IDSuffix != "noList");

  // Set "radio" check on one item, but...
  // we won't find a match if it's "mixed"
  var menuItem = document.getElementById("menu_"+IDSuffix);
  if (menuItem)
    menuItem.setAttribute("checked", "true");
}

function GetAlignmentString()
{
  var mixedObj = { value: null };
  var alignObj = { value: null };
  gEditor.getAlignment(mixedObj, alignObj);

  if (mixedObj.value)
    return "mixed";
  if (alignObj.value == nsIHTMLEditor.eLeft)
    return "left";
  if (alignObj.value == nsIHTMLEditor.eCenter)
    return "center";
  if (alignObj.value == nsIHTMLEditor.eRight)
    return "right";
  if (alignObj.value == nsIHTMLEditor.eJustify)
    return "justify";

  // return "left" if we got here
  return "left";
}

function InitAlignMenu()
{
  if (!isHTMLEditor())
    return;

  var IDSuffix = GetAlignmentString();

  // we won't find a match if it's "mixed"
  var menuItem = document.getElementById("menu_"+IDSuffix);
  if (menuItem)
    menuItem.setAttribute("checked", "true");
}

function EditorInitToolbars()
{
  if (!isHTMLEditor())
  {
    //Hide the formating toolbar
    HideItem("FormatToolbar");

    //Hide the edit mode toolbar
    HideItem("EditModeToolbar");

    SetElementEnabledById("cmd_viewFormatToolbar", false);
    SetElementEnabledById("cmd_viewEditModeToolbar", false);
  }
}

function EditorSetDefaultPrefsAndDoctype()
{
  var domdoc;
  try { domdoc = gEditor.document; } catch (e) { dump( e + "\n"); }
  if ( !domdoc )
  {
    dump("EditorSetDefaultPrefsAndDoctype: EDITOR DOCUMENT NOT FOUND\n");
    return;
  }

  // Insert a doctype element 
  // if it is missing from existing doc
  if (!domdoc.doctype)
  {
    var newdoctype = domdoc.implementation.createDocumentType("html", "-//W3C//DTD HTML 4.01 Transitional//EN","");
    if (newdoctype)
      domdoc.insertBefore(newdoctype, domdoc.firstChild);
  }
  
  // search for head; we'll need this for meta tag additions
  var headelement = 0;
  var headnodelist = domdoc.getElementsByTagName("head");
  if (headnodelist)
  {
    var sz = headnodelist.length;
    if ( sz >= 1 )
      headelement = headnodelist.item(0);
  }
  else
  {
    headelement = domdoc.createElement("head");
    if (headelement)
      domdoc.insertAfter(headelement, domdoc.firstChild);
  }

  /* only set default prefs for new documents */
  if (!IsUrlAboutBlank(GetDocumentUrl()))
    return;

  // search for author meta tag.
  // if one is found, don't do anything.
  // if not, create one and make it a child of the head tag
  //   and set its content attribute to the value of the editor.author preference.

  var nodelist = domdoc.getElementsByTagName("meta");
  if ( nodelist )
  {
    // we should do charset first since we need to have charset before
    // hitting other 8-bit char in other meta tags
    // grab charset pref and make it the default charset
    var element;
    var prefCharsetString = 0;
    try
    {
      prefCharsetString = gPrefs.getComplexValue("intl.charset.default",
                                                 Components.interfaces.nsIPrefLocalizedString).data;
    }
    catch (ex) {}
    if ( prefCharsetString && prefCharsetString != 0)
    {
        element = domdoc.createElement("meta");
        if ( element )
        {
          element.setAttribute("http-equiv", "content-type");
          element.setAttribute("content", "text/html; charset=" + prefCharsetString);
          headelement.insertBefore( element, headelement.firstChild );
        }
    }

    var node = 0;
    var listlength = nodelist.length;

    // let's start by assuming we have an author in case we don't have the pref
    var authorFound = false;
    for (var i = 0; i < listlength && !authorFound; i++)
    {
      node = nodelist.item(i);
      if ( node )
      {
        var value = node.getAttribute("name");
        if (value && value.toLowerCase() == "author")
        {
          authorFound = true;
        }
      }
    }

    var prefAuthorString = 0;
    try
    {
      prefAuthorString = gPrefs.getComplexValue("editor.author",
                                                Components.interfaces.nsISupportsString).data;
    }
    catch (ex) {}
    if ( prefAuthorString && prefAuthorString != 0)
    {
      if ( !authorFound && headelement)
      {
        /* create meta tag with 2 attributes */
        element = domdoc.createElement("meta");
        if ( element )
        {
          element.setAttribute("name", "author");
          element.setAttribute("content", prefAuthorString);
          headelement.appendChild( element );
        }
      }
    }
  }

  // add title tag if not present
  var titlenodelist = gEditor.document.getElementsByTagName("title");
  if (headelement && titlenodelist && titlenodelist.length == 0)
  {
     titleElement = domdoc.createElement("title");
     if (titleElement)
       headelement.appendChild(titleElement);
  }

  // Get editor color prefs
  var use_custom_colors = false;
  try {
    use_custom_colors = gPrefs.getBoolPref("editor.use_custom_colors");
  }
  catch (ex) {}

  // find body node
  var bodyelement = GetBodyElement();
  if (bodyelement)
  {
    if ( use_custom_colors )
    {
      // try to get the default color values.  ignore them if we don't have them.
      var text_color;
      var link_color;
      var active_link_color;
      var followed_link_color;
      var background_color;

      try { text_color = gPrefs.getCharPref("editor.text_color"); } catch (e) {}
      try { link_color = gPrefs.getCharPref("editor.link_color"); } catch (e) {}
      try { active_link_color = gPrefs.getCharPref("editor.active_link_color"); } catch (e) {}
      try { followed_link_color = gPrefs.getCharPref("editor.followed_link_color"); } catch (e) {}
      try { background_color = gPrefs.getCharPref("editor.background_color"); } catch(e) {}

      // add the color attributes to the body tag.
      // and use them for the default text and background colors if not empty
      if (text_color)
      {
        gEditor.setAttributeOrEquivalent(bodyelement, "text", text_color, true);
        gDefaultTextColor = text_color;
      }
      if (background_color)
      {
        gEditor.setAttributeOrEquivalent(bodyelement, "bgcolor", background_color, true);
        gDefaultBackgroundColor = background_color
      }

      if (link_color)
        bodyelement.setAttribute("link", link_color);
      if (active_link_color)
        bodyelement.setAttribute("alink", active_link_color);
      if (followed_link_color)
        bodyelement.setAttribute("vlink", followed_link_color);
    }
    // Default image is independent of Custom colors???
    var background_image;
    try { background_image = gPrefs.getCharPref("editor.default_background_image"); } catch(e) {}

    if (background_image)
      gEditor.setAttributeOrEquivalent(bodyelement, "background", background_image, true);
  }
  // auto-save???
}

function GetBodyElement()
{
  try {
    var bodyNodelist = gEditor.document.getElementsByTagName("body");
    if (bodyNodelist)
      return bodyNodelist.item(0);
  }
  catch (ex) {
    dump("no body tag found?!\n");
    //  better have one, how can we blow things up here?
  }
  return null;
}

// --------------------------- Logging stuff ---------------------------

function EditorGetNodeFromOffsets(offsets)
{
  var node = null;
  node = gEditor.document;

  for (var i = 0; i < offsets.length; i++)
  {
    node = node.childNodes[offsets[i]];
  }

  return node;
}

function EditorSetSelectionFromOffsets(selRanges)
{
  var rangeArr, start, end, node, offset;
  var selection = gEditor.selection;

  selection.removeAllRanges();

  for (var i = 0; i < selRanges.length; i++)
  {
    rangeArr = selRanges[i];
    start    = rangeArr[0];
    end      = rangeArr[1];

    var range = gEditor.document.createRange();

    node   = EditorGetNodeFromOffsets(start[0]);
    offset = start[1];

    range.setStart(node, offset);

    node   = EditorGetNodeFromOffsets(end[0]);
    offset = end[1];

    range.setEnd(node, offset);

    selection.addRange(range);
  }
}

//--------------------------------------------------------------------
function initFontStyleMenu(menuPopup)
{
  for (var i = 0; i < menuPopup.childNodes.length; i++)
  {
    var menuItem = menuPopup.childNodes[i];
    var theStyle = menuItem.getAttribute("state");
    if (theStyle)
    {
      menuItem.setAttribute("checked", theStyle);
    }
  }
}

//--------------------------------------------------------------------
function onButtonUpdate(button, commmandID)
{
  var commandNode = document.getElementById(commmandID);
  var state = commandNode.getAttribute("state");
  button.checked = state == "true";
}

//--------------------------------------------------------------------
function onStateButtonUpdate(button, commmandID, onState)
{
  var commandNode = document.getElementById(commmandID);
  var state = commandNode.getAttribute("state");

  button.checked = state == onState;
}

// --------------------------- Status calls ---------------------------
function getColorAndSetColorWell(ColorPickerID, ColorWellID)
{
  var colorWell;
  if (ColorWellID)
    colorWell = document.getElementById(ColorWellID);

  var colorPicker = document.getElementById(ColorPickerID);
  if (colorPicker)
  {
    // Extract color from colorPicker and assign to colorWell.
    var color = colorPicker.getAttribute("color");

    if (colorWell && color)
    {
      // Use setAttribute so colorwell can be a XUL element, such as button
      colorWell.setAttribute("style", "background-color: " + color);
    }
  }
  return color;
}

//-----------------------------------------------------------------------------------
function IsSpellCheckerInstalled()
{
  return "@mozilla.org/spellchecker;1" in Components.classes;
}

//-----------------------------------------------------------------------------------
function IsFindInstalled()
{
  return "@mozilla.org/embedcomp/rangefind;1" in Components.classes
          && "@mozilla.org/find/find_service;1" in Components.classes;
}

//-----------------------------------------------------------------------------------
function RemoveInapplicableUIElements()
{
  // For items that are in their own menu block, remove associated separator
  // (we can't use "hidden" since class="hide-in-IM" CSS rule interferes)

   // if no find, remove find ui
  if (!IsFindInstalled())
  {
    HideItem("menu_find");
    HideItem("menu_findnext");
    HideItem("menu_replace");
    HideItem("menu_find");
    RemoveItem("sep_find");
  }

   // if no spell checker, remove spell checker ui
  if (!IsSpellCheckerInstalled())
  {
    HideItem("spellingButton");
    HideItem("menu_checkspelling");
    RemoveItem("sep_checkspelling");
  }
  else
  {
    SetElementEnabled(document.getElementById("menu_checkspelling"), true);
    SetElementEnabled(document.getElementById("spellingButton"), true);
    SetElementEnabled(document.getElementById("checkspellingkb"), true);
  }

  // Remove menu items (from overlay shared with HTML editor) in non-HTML.
  if (!isHTMLEditor())
  {
    HideItem("insertAnchor");
    HideItem("insertImage");
    HideItem("insertHline");
    HideItem("insertTable");
    HideItem("insertHTML");
    HideItem("insertFormMenu");
    HideItem("fileExportToText");
    HideItem("viewFormatToolbar");
    HideItem("viewEditModeToolbar");
  }
}

function HideItem(id)
{
  var item = document.getElementById(id);
  if (item)
    item.setAttribute("hidden", "true");
}

function RemoveItem(id)
{
  var item = document.getElementById(id);
  if (item)
    item.parentNode.removeChild(item);
}

// Command Updating Strategy:
//   Don't update on on selection change, only when menu is displayed,
//   with this "oncreate" hander:
function EditorInitTableMenu()
{
  try {
    InitJoinCellMenuitem("menu_JoinTableCells");
  } catch (ex) {}

  // Set enable states for all table commands
  goUpdateTableMenuItems(document.getElementById("composerTableMenuItems"));
}

function InitJoinCellMenuitem(id)
{
  // Change text on the "Join..." item depending if we
  //   are joining selected cells or just cell to right
  // TODO: What to do about normal selection that crosses
  //       table border? Try to figure out all cells
  //       included in the selection?
  var menuText;
  var menuItem = document.getElementById(id);
  if (!menuItem) return;

  // Use "Join selected cells if there's more than 1 cell selected
  var numSelected;
  var foundElement;
  
  try {
    var tagNameObj = {};
    var countObj = {value:0}
    foundElement = gEditor.getSelectedOrParentTableElement(tagNameObj, countObj);
    numSelected = countObj.value
  }
  catch(e) {}
  if (foundElement && numSelected > 1)
    menuText = GetString("JoinSelectedCells");
  else
    menuText = GetString("JoinCellToRight");

  menuItem.setAttribute("label",menuText);
  menuItem.setAttribute("accesskey",GetString("JoinCellAccesskey"));
}

function InitRemoveStylesMenuitems(removeStylesId, removeLinksId, removeNamedAnchorsId)
{
  // Change wording of menuitems depending on selection
  var stylesItem = document.getElementById(removeStylesId);
  var linkItem = document.getElementById(removeLinksId);

  var isCollapsed = gEditor.selection.isCollapsed;
  if (stylesItem)
  {
    stylesItem.setAttribute("label", isCollapsed ? GetString("StopTextStyles") : GetString("RemoveTextStyles"));
    stylesItem.setAttribute("accesskey", GetString("RemoveTextStylesAccesskey"));
  }
  if (linkItem)
  {
    linkItem.setAttribute("label", isCollapsed ? GetString("StopLinks") : GetString("RemoveLinks"));
    linkItem.setAttribute("accesskey", GetString("RemoveLinksAccesskey"));
    // Note: disabling text style is a pain since there are so many - forget it!

    // Disable if not in a link, but always allow "Remove"
    //  if selection isn't collapsed since we only look at anchor node
    try {
      SetElementEnabled(linkItem, !isCollapsed ||
                      gEditor.getElementOrParentByTagName("href", null));
    } catch(e) {}      
  }
  // Disable if selection is collapsed
  SetElementEnabledById(removeNamedAnchorsId, !isCollapsed);
}

function goUpdateTableMenuItems(commandset)
{
  var enabled = false;
  var enabledIfTable = false;

  if (!gEditor)
  {
    dump("goUpdateTableMenuItems: too early, not initialized\n");
    return;
  }

  var flags = gEditor.flags;
  if (!(flags & nsIPlaintextEditor.eEditorReadonlyMask) &&
      IsEditingRenderedHTML())
  {
    var tagNameObj = { value: "" };
    var element;
    try {
      element = gEditor.getSelectedOrParentTableElement(tagNameObj, {value:0});
    }
    catch(e) {}

    if (element)
    {
      // Value when we need to have a selected table or inside a table
      enabledIfTable = true;

      // All others require being inside a cell or selected cell
      enabled = (tagNameObj.value == "td");
    }
  }

  // Loop through command nodes
  for (var i = 0; i < commandset.childNodes.length; i++)
  {
    var commandID = commandset.childNodes[i].getAttribute("id");
    if (commandID)
    {
      if (commandID == "cmd_InsertTable" ||
          commandID == "cmd_JoinTableCells" ||
          commandID == "cmd_SplitTableCell" ||
          commandID == "cmd_ConvertToTable")
      {
        // Call the update method in the command class
        goUpdateCommand(commandID);
      }
      // Directly set with the values calculated here
      else if (commandID == "cmd_DeleteTable" ||
               commandID == "cmd_NormalizeTable" ||
               commandID == "cmd_editTable" ||
               commandID == "cmd_TableOrCellColor" ||
               commandID == "cmd_SelectTable")
      {
        goSetCommandEnabled(commandID, enabledIfTable);
      } else {
        goSetCommandEnabled(commandID, enabled);
      }
    }
  }
}

//-----------------------------------------------------------------------------------
// Helpers for inserting and editing tables:

function IsInTable()
{
  if (!gEditor) return false;
  var flags = gEditor.flags;
  return (isHTMLEditor() &&
          !(flags & nsIPlaintextEditor.eEditorReadonlyMask) &&
          IsEditingRenderedHTML() &&
          null != gEditor.getElementOrParentByTagName("table", null));
}

function IsInTableCell()
{
  if (!gEditor) return false;
  var flags = gEditor.flags;
  return (isHTMLEditor() &&
          !(flags & nsIPlaintextEditor.eEditorReadonlyMask) && 
          IsEditingRenderedHTML() &&
          null != gEditor.getElementOrParentByTagName("td", null));
}

function IsSelectionInOneCell()
{
  if (!gEditor || !isHTMLEditor()) 
    return false;

  var selection = gEditor.selection;

  if (selection && selection.rangeCount == 1)
  {
    // We have a "normal" single-range selection
    if (!selection.isCollapsed &&
       selection.anchorNode != selection.focusNode)
    {
      // Check if both nodes are within the same cell
      var anchorCell = gEditor.getElementOrParentByTagName("td", selection.anchorNode);
      var focusCell = gEditor.getElementOrParentByTagName("td", selection.focusNode);
      return (focusCell != null && anchorCell != null && (focusCell == anchorCell));
    }
    // Collapsed selection or anchor == focus (thus must be in 1 cell)
    return true;
  }
  return false;
}

// Call this with insertAllowed = true to allow inserting if not in existing table,
//   else use false to do nothing if not in a table
function EditorInsertOrEditTable(insertAllowed)
{
  if (IsInTable()) {
    // Edit properties of existing table
    window.openDialog("chrome://editor/content/EdTableProps.xul", "_blank", "chrome,close,titlebar,modal", "","TablePanel");
    gContentWindow.focus();
  } else if(insertAllowed) {
    if (gEditor.selection.isCollapsed)
      // If we have a caret, insert a blank table...
      EditorInsertTable();
    else
      // else convert the selection into a table
      goDoCommand("cmd_ConvertToTable");
  }
}

function EditorInsertTable()
{
  // Insert a new table
  window.openDialog("chrome://editor/content/EdInsertTable.xul", "_blank", "chrome,close,titlebar,modal", "");
  gContentWindow.focus();
}

function EditorTableCellProperties()
{
  if (!isHTMLEditor())
    return;

  var cell = gEditor.getElementOrParentByTagName("td", null);
  if (cell) {
    // Start Table Properties dialog on the "Cell" panel
    window.openDialog("chrome://editor/content/EdTableProps.xul", "_blank", "chrome,close,titlebar,modal", "", "CellPanel");
    gContentWindow.focus();
  }
}

function GetNumberOfContiguousSelectedRows()
{
  if (!isHTMLEditor())
    return 0;

  var rowObj = { value: 0 };
  var colObj = { value: 0 };
  
  var cell = gEditor.getFirstSelectedCellInTable(rowObj, colObj);
  if (!cell)
    return 0;

  var rows = 1;
  var lastIndex = rowObj.value;

  do {
    cell = gEditor.getNextSelectedCell({value:0});
    if (cell)
    {
      gEditor.getCellIndexes(cell, rowObj, colObj);
      var index = rowObj.value;
      if (index == lastIndex + 1)
      {
        lastIndex = index;
        rows++;
      }
    }
  }
  while (cell);

  return rows;
}

function GetNumberOfContiguousSelectedColumns()
{
  if (!isHTMLEditor())
    return 0;

  var colObj = { value: 0 };
  var rowObj = { value: 0 };
  var cell = gEditor.getFirstSelectedCellInTable(rowObj, colObj);
  if (!cell)
    return 0;

  var columns = 1;
  var lastIndex = colObj.value;

  do {
    cell = gEditor.getNextSelectedCell({value:0});
    if (cell)
    {
      gEditor.getCellIndexes(cell, rowObj, colObj);
      var index = colObj.value;
      if (index == lastIndex +1)
      {
        lastIndex = index;
        columns++;
      }
    }
  }
  while (cell);

  return columns;
}

function EditorOnFocus()
{
  // Current window already has the InsertCharWindow
  if ("InsertCharWindow" in window && window.InsertCharWindow) return;

  // Find window with an InsertCharsWindow and switch association to this one
  var windowWithDialog = FindEditorWithInsertCharDialog();
  if (windowWithDialog)
  {
    // Switch the dialog to current window
    // this sets focus to dialog, so bring focus back to editor window
    if (SwitchInsertCharToThisWindow(windowWithDialog))
      window.focus();
  }
}

function SwitchInsertCharToThisWindow(windowWithDialog)
{
  if (windowWithDialog && "InsertCharWindow" in windowWithDialog &&
      windowWithDialog.InsertCharWindow)
  {
    // Move dialog association to the current window
    window.InsertCharWindow = windowWithDialog.InsertCharWindow;
    windowWithDialog.InsertCharWindow = null;

    // Switch the dialog's editorShell and opener to current window's
    window.InsertCharWindow.editorShell = window.editorShell;
    window.InsertCharWindow.opener = window;

    // Bring dialog to the forground
    window.InsertCharWindow.focus();
    return true;
  }
  return false;
}

function FindEditorWithInsertCharDialog()
{
  try {
    // Find window with an InsertCharsWindow and switch association to this one
    var windowManager = Components.classes['@mozilla.org/appshell/window-mediator;1'].getService();
    var windowManagerInterface = windowManager.QueryInterface( Components.interfaces.nsIWindowMediator);
    var enumerator = windowManagerInterface.getEnumerator( null );

    while ( enumerator.hasMoreElements()  )
    {
      var tempWindow = enumerator.getNext();

      if (tempWindow != window && "InsertCharWindow" in tempWindow &&
          tempWindow.InsertCharWindow)
      {
        return tempWindow;
      }
    }
  }
  catch(e) {}
  return null;
}

function EditorFindOrCreateInsertCharWindow()
{
  if ("InsertCharWindow" in window && window.InsertCharWindow)
    window.InsertCharWindow.focus();
  else
  {
    // Since we switch the dialog during EditorOnFocus(),
    //   this should really never be found, but it's good to be sure
    var windowWithDialog = FindEditorWithInsertCharDialog();
    if (windowWithDialog)
    {
      SwitchInsertCharToThisWindow(windowWithDialog);
    }
    else
    {
      // The dialog will set window.InsertCharWindow to itself
      window.openDialog("chrome://editor/content/EdInsertChars.xul", "_blank", "chrome,close,titlebar", "");
    }
  }
}

// Find another HTML editor window to associate with the InsertChar dialog
//   or close it if none found  (May be a mail composer)
function SwitchInsertCharToAnotherEditorOrClose()
{
  if ("InsertCharWindow" in window && window.InsertCharWindow)
  {
    var windowManager = Components.classes['@mozilla.org/appshell/window-mediator;1'].getService();
    var enumerator;
    try {
      var windowManagerInterface = windowManager.QueryInterface( Components.interfaces.nsIWindowMediator);
      enumerator = windowManagerInterface.getEnumerator( null );
    }
    catch(e) {}
    if (!enumerator) return;

    // TODO: Fix this to search for command controllers and look for "cmd_InsertChars"
    // For now, detect just Web Composer and HTML Mail Composer
    while ( enumerator.hasMoreElements()  )
    {
      var  tempWindow = enumerator.getNext();
      if (tempWindow != window && tempWindow != window.InsertCharWindow && 
          ("editorShell" in tempWindow) && tempWindow.editorShell)
      {
        if (gEditor)
        {
          tempWindow.InsertCharWindow = window.InsertCharWindow;
          window.InsertCharWindow = null;

          tempWindow.InsertCharWindow.editorShell = tempWindow.editorShell;
          tempWindow.InsertCharWindow.opener = tempWindow;
          return;
        }
      }
    }
    // Didn't find another editor - close the dialog
    window.InsertCharWindow.close();
  }
}

function ResetStructToolbar()
{
  gLastFocusNode = null;
  UpdateStructToolbar();
}

function newCommandListener(element)
{
  return function() { return SelectFocusNodeAncestor(element); };
}

function newContextmenuListener(button, element)
{
  return function() { return InitStructBarContextMenu(button, element); };
}

function UpdateStructToolbar()
{
  var editor = GetCurrentEditor();
  if (!editor) return;

  var mixed = GetSelectionContainer();
  if (!mixed) return;
  var element = mixed.node;
  var oneElementSelected = mixed.oneElementSelected;

  if (!element) return;

  if (element == gLastFocusNode &&
      oneElementSelected == gLastFocusNodeWasSelected)
    return;

  gLastFocusNode = element;
  gLastFocusNodeWasSelected = mixed.oneElementSelected;

  var toolbar = document.getElementById("structToolbar");
  if (!toolbar) return;
  var childNodes = toolbar.childNodes;
  var childNodesLength = childNodes.length;
  // We need to leave the <label> to flex the buttons to the left
  // so, don't remove the last child at position length - 1
  for (var i = childNodesLength - 2; i >= 0; i--) {
    toolbar.removeChild(childNodes.item(i));
  }

  toolbar.removeAttribute("label");

  if ( IsInHTMLSourceMode() ) {
    // we have destroyed the contents of the status bar and are
    // about to recreate it ; but we don't want to do that in
    // Source mode
    return;
  }

  var tag, button;
  var bodyElement = GetBodyElement();
  var isFocusNode = true;
  var tmp;
  do {
    tag = element.nodeName.toLowerCase();

    button = document.createElementNS(XUL_NS, "toolbarbutton");
    button.setAttribute("label",   "<" + tag + ">");
    button.setAttribute("value",   tag);
    button.setAttribute("context", "structToolbarContext");
    button.className = "struct-button";

    toolbar.insertBefore(button, toolbar.firstChild);

    button.addEventListener("command", newCommandListener(element), false);

    button.addEventListener("contextmenu", newContextmenuListener(button, element), false);

    if (isFocusNode && oneElementSelected) {
      button.setAttribute("checked", "true");
      isFocusNode = false;
    }

    tmp = element;
    element = element.parentNode;

  } while (tmp != bodyElement);
}

function SelectFocusNodeAncestor(element)
{
  var editor = GetCurrentEditor();
  if (editor) {
    if (element == GetBodyElement())
      editor.selectAll();
    else
      editor.selectElement(element);
  }
  ResetStructToolbar();
}

function GetSelectionContainer()
{
  var editor = GetCurrentEditor();
  if (!editor) return null;

  var selection = editor.selection;
  if (!selection) return null;

  var result = { oneElementSelected:false };

  if (selection.isCollapsed) {
    result.node = selection.focusNode;
  }
  else {
    var rangeCount = selection.rangeCount;
    if (rangeCount == 1) {
      result.node = editor.getSelectedElement("");
      var range = selection.getRangeAt(0);

      // check for a weird case : when we select a piece of text inside
      // a text node and apply an inline style to it, the selection starts
      // at the end of the text node preceding the style and ends after the
      // last char of the style. Assume the style element is selected for
      // user's pleasure
      if (!result.node &&
          range.startContainer.nodeType == Node.TEXT_NODE &&
          range.startOffset == range.startContainer.length &&
          range.endContainer.nodeType == Node.TEXT_NODE &&
          range.endOffset == range.endContainer.length &&
          range.endContainer.nextSibling == null &&
          range.startContainer.nextSibling == range.endContainer.parentNode)
        result.node = range.endContainer.parentNode;

      if (!result.node) {
        // let's rely on the common ancestor of the selection
        result.node = range.commonAncestorContainer;
      }
      else {
        result.oneElementSelected = true;
      }
    }
    else {
      // assume table cells !
      var i, container = null;
      for (i = 0; i < rangeCount; i++) {
        range = selection.getRangeAt(i);
        if (!container) {
          container = range.startContainer;
        }
        else if (container != range.startContainer) {
          // all table cells don't belong to same row so let's
          // select the parent of all rows
          result.node = container.parentNode;
          break;
        }
        result.node = container;
      }
    }
  }

  // make sure we have an element here
  while (result.node.nodeType != Node.ELEMENT_NODE)
    result.node = result.node.parentNode;

  // and make sure the element is not a special editor node like
  // the <br> we insert in blank lines
  while (result.node.hasAttribute("_moz_editor_bogus_node"))
    result.node = result.node.parentNode;

  return result;
}
