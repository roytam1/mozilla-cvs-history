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
 *   Ben Goodger
 */

var gInsertNewImage = true;
var gInsertNewIMap = true;
var constrainOn = false;
// Note used in current version, but these are set correctly
//  and could be used to reset width and height used for constrain ratio
var constrainWidth  = 0;
var constrainHeight = 0;
var imageElement;
var gImageMap = 0;
var gCanRemoveImageMap = false;
var gRemoveImageMap = false;
var gImageMapDisabled = false;
var firstTimeOkUsed = true;
var doAltTextError = false;
var actualWidth = "";
var gOriginalSrc = "";
var actualHeight = "";
var gHaveDocumentUrl = false;
var gTimerID;

// These must correspond to values in EditorDialog.css for each theme
// (unfortunately, setting "style" attribute here doesn't work!)
var gPreviewImageWidth = 80;
var gPreviewImageHeight = 50;
var StartupCalled = false;
var gOkButton;

// dialog initialization code

function Startup()
{
  //XXX Very weird! When calling this with an existing image,
  //    we get called twice. That causes dialog layout
  //    to explode to fullscreen!
  if (StartupCalled)
  {
    dump("*** CALLING IMAGE DIALOG Startup() AGAIN! ***\n");
    return;
  }
  StartupCalled = true;

  if (!InitEditorShell())
    return;

  gDialog.srcInput          = document.getElementById( "srcInput" );
  gDialog.altTextInput      = document.getElementById( "altTextInput" );
  gDialog.MoreFewerButton   = document.getElementById( "MoreFewerButton" );
  gDialog.MoreSection       = document.getElementById( "MoreSection" );
  gDialog.customSizeRadio   = document.getElementById( "customSizeRadio" );
  gDialog.actualSizeRadio   = document.getElementById( "actualSizeRadio" );
  gDialog.constrainCheckbox = document.getElementById( "constrainCheckbox" );
  gDialog.widthInput        = document.getElementById( "widthInput" );
  gDialog.heightInput       = document.getElementById( "heightInput" );
  gDialog.widthUnitsMenulist   = document.getElementById( "widthUnitsMenulist" );
  gDialog.heightUnitsMenulist  = document.getElementById( "heightUnitsMenulist" );
  gDialog.imagelrInput      = document.getElementById( "imageleftrightInput" );
  gDialog.imagetbInput      = document.getElementById( "imagetopbottomInput" );
  gDialog.border            = document.getElementById( "border" );
  gDialog.alignTypeSelect   = document.getElementById( "alignTypeSelect" );
  gDialog.ImageHolder       = document.getElementById( "preview-image-holder" );
  gDialog.PreviewWidth      = document.getElementById( "PreviewWidth" );
  gDialog.PreviewHeight     = document.getElementById( "PreviewHeight" );
  gDialog.PreviewSize       = document.getElementById( "PreviewSize" );
  gDialog.PreviewImage      = null;
  gDialog.OkButton          = document.documentElement.getButton("accept");

  // Get a single selected image element
  var tagName = "img"
  imageElement = editorShell.GetSelectedElement(tagName);

  if (imageElement)
  {
    // We found an element and don't need to insert one
    gInsertNewImage = false;
    actualWidth  = imageElement.naturalWidth;
    actualHeight = imageElement.naturalHeight;
  }
  else
  {
    gInsertNewImage = true;

    // We don't have an element selected,
    //  so create one with default attributes

    imageElement = editorShell.CreateElementWithDefaults(tagName);
    if (!imageElement)
    {
      dump("Failed to get selected element or create a new one!\n");
      window.close();
      return;
    }
  }

  // Make a copy to use for AdvancedEdit
  globalElement = imageElement.cloneNode(false);

  // We only need to test for this once per dialog load
  gHaveDocumentUrl = GetDocumentBaseUrl();

  InitDialog();

  // Save initial source URL
  gOriginalSrc = gDialog.srcInput.value;

  // By default turn constrain on, but both width and height must be in pixels
  gDialog.constrainCheckbox.checked =
    gDialog.widthUnitsMenulist.selectedIndex == 0 &&
    gDialog.heightUnitsMenulist.selectedIndex == 0;

  // Set SeeMore bool to the OPPOSITE of the current state,
  //   which is automatically saved by using the 'persist="more"'
  //   attribute on the MoreFewerButton button
  //   onMoreFewer will toggle the state and redraw the dialog
  SeeMore = (gDialog.MoreFewerButton.getAttribute("more") != "1");

  // Initialize widgets with image attributes in the case where the entire dialog isn't visible
  onMoreFewer();

  SetTextboxFocus(gDialog.srcInput);

  SetWindowLocation();
}

// Set dialog widgets with attribute data
// We get them from globalElement copy so this can be used
//   by AdvancedEdit(), which is shared by all property dialogs
function InitDialog()
{
  // Set the controls to the image's attributes
  gDialog.srcInput.value = globalElement.getAttribute("src");

  // Set "Relativize" checkbox according to current URL state
  SetRelativeCheckbox();

  // Force loading of image from its source and show preview image
  LoadPreviewImage();

  gDialog.altTextInput.value = globalElement.getAttribute("alt");

  // setup the height and width widgets
  var width = InitPixelOrPercentMenulist(globalElement,
                    gInsertNewImage ? null : imageElement,
                    "width", "widthUnitsMenulist", gPixel);
  var height = InitPixelOrPercentMenulist(globalElement,
                    gInsertNewImage ? null : imageElement,
                    "height", "heightUnitsMenulist", gPixel);

  var rg = gDialog.actualSizeRadio.radioGroup;
  // Set actual radio button if both set values are the same as actual
  if (actualWidth && actualHeight) {
    if ((width == actualWidth && height == actualHeight) || !(width || height))
      rg.selectedItem = gDialog.actualSizeRadio;
  }
  if (!gDialog.actualSizeRadio.selected)
    rg.selectedItem = gDialog.customSizeRadio;

  gDialog.widthInput.value  = constrainWidth = width ? width : (actualWidth ? actualWidth : "");
  gDialog.heightInput.value = constrainHeight = height ? height : (actualHeight ? actualHeight : "");

  // set spacing editfields
  gDialog.imagelrInput.value = globalElement.getAttribute("hspace");
  gDialog.imagetbInput.value = globalElement.getAttribute("vspace");

  // dialog.border.value       = globalElement.getAttribute("border");
  var bv = GetHTMLOrCSSStyleValue(globalElement, "border", "border-top-width");
  var pxIndex = bv.search(/px/);
  if (pxIndex  > 0)
  {
    // Strip out the px
    bv = bv.substr(0, pxIndex);
  }
  else if (bv == "thin")
  {
    bv = "1";
  }
  else if (bv == "medium")
  {
    bv = "3";
  }
  else if (bv == "thick")
  {
    bv = "5";
  }
  gDialog.border.value = bv;

  // Get alignment setting
  var align = globalElement.getAttribute("align");
  if (align) {
    align = align.toLowerCase();
  }
  var imgClass;
  var textID;
  switch ( align )
  {
    case "top":
    case "middle":
    case "right":
    case "left":
      gDialog.alignTypeSelect.value = align;
      break;
    default:  // Default or "bottom"
      gDialog.alignTypeSelect.value = "bottom";
  }

  // Get image map for image
  gImageMap = GetImageMap();

  doOverallEnabling();
  doDimensionEnabling();
}

function GetImageMap()
{
  var usemap = globalElement.getAttribute("usemap");
  if (usemap)
  {
    gCanRemoveImageMap = true;
    var mapname = usemap.substring(1, usemap.length);
    var mapCollection = editorShell.editorDocument.getElementsByName(mapname);
    if (mapCollection[0] != null)
    {
      gInsertNewIMap = false;
      return mapCollection[0];
    }
  }
  else
  {
    gCanRemoveImageMap = false;
  }

  gInsertNewIMap = true;
  return null;
}

function chooseFile()
{
  // Get a local file, converted into URL format
  var fileName = GetLocalFileURL("img");
  if (fileName)
  {
    // Always try to relativize local file URLs
    if (gHaveDocumentUrl)
      fileName = MakeRelativeUrl(fileName);

    gDialog.srcInput.value = fileName;

    SetRelativeCheckbox();
    doOverallEnabling();
  }
  LoadPreviewImage();

  // Put focus into the input field
  SetTextboxFocus(gDialog.srcInput);
}

function PreviewImageLoaded()
{
  if (gDialog.PreviewImage)
  {
    // Image loading has completed -- we can get actual width
    actualWidth  = gDialog.PreviewImage.naturalWidth;
    actualHeight = gDialog.PreviewImage.naturalHeight;

    if (actualWidth && actualHeight)
    {
      // Use actual size or scale to fit preview if either dimension is too large
      var width = actualWidth;
      var height = actualHeight;
      if (actualWidth > gPreviewImageWidth)
      {
          width = gPreviewImageWidth;
          height = actualHeight * (gPreviewImageWidth / actualWidth);
      }
      if (height > gPreviewImageHeight)
      {
        height = gPreviewImageHeight;
        width = actualWidth * (gPreviewImageHeight / actualHeight);
      }
      gDialog.PreviewImage.width = width;
      gDialog.PreviewImage.height = height;

      gDialog.PreviewWidth.setAttribute("value", actualWidth);
      gDialog.PreviewHeight.setAttribute("value", actualHeight);

      gDialog.PreviewSize.setAttribute("collapsed", "false");
      gDialog.ImageHolder.setAttribute("collapsed", "false");

      // Use values as start for constrain proportions
    }

    if (gDialog.actualSizeRadio.selected)
      SetActualSize();
  }
}

function LoadPreviewImage()
{
  gDialog.PreviewSize.setAttribute("collapsed", "true");

  var imageSrc = TrimString(gDialog.srcInput.value);
  if (!imageSrc)
    return;

  try {
    // Remove the image URL from image cache so it loads fresh
    //  (if we don't do this, loads after the first will always use image cache
    //   and we won't see image edit changes or be able to get actual width and height)
    
    var IOService = GetIOService();
    if (IOService)
    {
      // We must have an absolute URL to preview it or remove it from the cache
      imageSrc = MakeAbsoluteUrl(imageSrc);

      if (GetScheme(imageSrc))
      {
        var uri = IOService.newURI(imageSrc, null);
        if (uri)
        {
          var imgCacheService = Components.classes["@mozilla.org/image/cache;1"].getService();
          var imgCache = imgCacheService.QueryInterface(Components.interfaces.imgICache);

          // This returns error if image wasn't in the cache; ignore that
          imgCache.removeEntry(uri);
        }
      }
    }
  } catch(e) {}

  if (gDialog.PreviewImage)
    removeEventListener("load", PreviewImageLoaded, true);

  if (gDialog.ImageHolder.firstChild)
    gDialog.ImageHolder.removeChild(gDialog.ImageHolder.firstChild);
    
  gDialog.PreviewImage = document.createElementNS("http://www.w3.org/1999/xhtml", "html:img");
  if (gDialog.PreviewImage)
  {
    gDialog.ImageHolder.appendChild(gDialog.PreviewImage);
    gDialog.PreviewImage.addEventListener("load", PreviewImageLoaded, true);
    gDialog.PreviewImage.src = imageSrc;
  }
}

function SetActualSize()
{
  gDialog.widthInput.value = actualWidth ? actualWidth : "";
  gDialog.widthUnitsMenulist.selectedIndex = 0;
  gDialog.heightInput.value = actualHeight ? actualHeight : "";
  gDialog.heightUnitsMenulist.selectedIndex = 0;
  doDimensionEnabling();
}

function ChangeImageSrc()
{
  if (gTimerID)
    clearTimeout(gTimerID);

  gTimerID = setTimeout("LoadPreviewImage()", 800);

  SetRelativeCheckbox();
  doOverallEnabling();
}

function doDimensionEnabling()
{
  // Enabled only if "Custom" is selected
  var enable = (gDialog.customSizeRadio.selected);

  // BUG 74145: After input field is disabled,
  //   setting it enabled causes blinking caret to appear
  //   even though focus isn't set to it.
  SetElementEnabledById( "heightInput", enable );
  SetElementEnabledById( "heightLabel", enable );
  SetElementEnabledById( "heightUnitsMenulist", enable );

  SetElementEnabledById( "widthInput", enable );
  SetElementEnabledById( "widthLabel", enable);
  SetElementEnabledById( "widthUnitsMenulist", enable );

  var constrainEnable = enable
         && ( gDialog.widthUnitsMenulist.selectedIndex == 0 )
         && ( gDialog.heightUnitsMenulist.selectedIndex == 0 );

  SetElementEnabledById( "constrainCheckbox", constrainEnable );
}

function doOverallEnabling()
{
  var enabled = TrimString(gDialog.srcInput.value) != "";

  SetElementEnabled(gDialog.OkButton, enabled);
  SetElementEnabledById("AdvancedEditButton1", enabled);
  SetElementEnabledById("imagemapLabel", enabled);

  //TODO: Restore when Image Map editor is finished
  //SetElementEnabledById("editImageMap", enabled);
  SetElementEnabledById("removeImageMap", gCanRemoveImageMap);
}

function ToggleConstrain()
{
  // If just turned on, save the current width and height as basis for constrain ratio
  // Thus clicking on/off lets user say "Use these values as aspect ration"
  if (gDialog.constrainCheckbox.checked && !gDialog.constrainCheckbox.disabled
     && (gDialog.widthUnitsMenulist.selectedIndex == 0)
     && (gDialog.heightUnitsMenulist.selectedIndex == 0))
  {
    constrainWidth = Number(TrimString(gDialog.widthInput.value));
    constrainHeight = Number(TrimString(gDialog.heightInput.value));
  }
}

function constrainProportions( srcID, destID )
{
  var srcElement = document.getElementById(srcID);
  if (!srcElement)
    return;

  var destElement = document.getElementById(destID);
  if (!destElement)
    return;

  // always force an integer (whether we are constraining or not)
  forceInteger(srcID);

  if (!actualWidth || !actualHeight ||
      !(gDialog.constrainCheckbox.checked && !gDialog.constrainCheckbox.disabled))
    return;

  // double-check that neither width nor height is in percent mode; bail if so!
  if ( (gDialog.widthUnitsMenulist.selectedIndex != 0)
     || (gDialog.heightUnitsMenulist.selectedIndex != 0) )
    return;

  // This always uses the actual width and height ratios
  // which is kind of funky if you change one number without the constrain
  // and then turn constrain on and change a number
  // I prefer the old strategy (below) but I can see some merit to this solution
  if (srcID == "widthInput")
    destElement.value = Math.round( srcElement.value * actualHeight / actualWidth );
  else
    destElement.value = Math.round( srcElement.value * actualWidth / actualHeight );

/*
  // With this strategy, the width and height ratio
  //   can be reset to whatever the user entered.
  if (srcID == "widthInput")
    destElement.value = Math.round( srcElement.value * constrainHeight / constrainWidth );
  else
    destElement.value = Math.round( srcElement.value * constrainWidth / constrainHeight );
*/
}

function editImageMap()
{
  // Create an imagemap for image map editor
  if (gInsertNewIMap)
    gImageMap = editorShell.CreateElementWithDefaults("map");

  // Note: We no longer pass in a copy of the global ImageMap. ImageMap editor should create a copy and manage onOk and onCancel behavior
  window.openDialog("chrome://editor/content/EdImageMap.xul", "_blank", "chrome,close,titlebar,modal", globalElement, gImageMap);
}

function removeImageMap()
{
  gRemoveImageMap = true;
  gCanRemoveImageMap = false;
  SetElementEnabledById("removeImageMap", false);
}

// Get data from widgets, validate, and set for the global element
//   accessible to AdvancedEdit() [in EdDialogCommon.js]
function ValidateData()
{
  //TODO: WE NEED TO DO SOME URL VALIDATION HERE, E.G.:
  // We must convert to "file:///" or "http://" format else image doesn't load!
  var src = TrimString(gDialog.srcInput.value);
  globalElement.setAttribute("src", src);

  // Note: allow typing spaces,
  // Warn user if empty string just once per dialog session
  //   but don't consider this a failure
  var alt = gDialog.altTextInput.value;
  if (doAltTextError && !alt)
  {
    AlertWithTitle(null, GetString("NoAltText"));
    SetTextboxFocus(gDialog.altTextInput);
    doAltTextError = false;
    return false;
  }
  globalElement.setAttribute("alt", alt);

  var width = "";
  var height = "";

  if (!gDialog.actualSizeRadio.selected)
  {
    // Get user values for width and height
    width = ValidateNumber(gDialog.widthInput, gDialog.widthUnitsMenulist, 1, maxPixels, 
                           globalElement, "width", false, true);
    if (gValidationError)
      return false;

    height = ValidateNumber(gDialog.heightInput, gDialog.heightUnitsMenulist, 1, maxPixels, 
                            globalElement, "height", false, true);
    if (gValidationError)
      return false;
  }

  // We always set the width and height attributes, even if same as actual.
  //  This speeds up layout of pages since sizes are known before image is loaded
  if (!width)
    width = actualWidth;
  if (!height)
    height = actualHeight;

  // Remove existing width and height only if source changed
  //  and we couldn't obtain actual dimensions
  var srcChanged = (src != gOriginalSrc);
  if (width)
    globalElement.setAttribute("width", width);
  else if (srcChanged)
    globalElement.removeAttribute("width");

  if (height)
    globalElement.setAttribute("height", height);
  else if (srcChanged) 
    globalElement.removeAttribute("height");

  // spacing attributes

  ValidateNumber(gDialog.imagelrInput, null, 0, maxPixels, 
                 globalElement, "hspace", false, true, true);
  if (gValidationError)
    return false;

  ValidateNumber(gDialog.imagetbInput, null, 0, maxPixels, 
                 globalElement, "vspace", false, true);
  if (gValidationError)
    return false;

  // note this is deprecated and should be converted to stylesheets
  ValidateNumber(gDialog.border, null, 0, maxPixels, 
                 globalElement, "border", false, true);
  if (gValidationError)
    return false;

  // Default or setting "bottom" means don't set the attribute
  // Note that the attributes "left" and "right" are opposite
  //  of what we use in the UI, which describes where the TEXT wraps,
  //  not the image location (which is what the HTML describes)
  switch ( gDialog.alignTypeSelect.value )
  {
    case "top":
    case "middle":
    case "right":
    case "left":
      globalElement.setAttribute( "align", gDialog.alignTypeSelect.value );
      break;
    default:
      globalElement.removeAttribute( "align" );
  }

  return true;
}

function doHelpButton()
{
  openHelp("chrome://help/content/help.xul?image_properties");
  return true;
}

function onAccept()
{
  // Show alt text error only once
  // (we don't initialize doAltTextError=true
  //  so Advanced edit button dialog doesn't trigger that error message)
  doAltTextError = firstTimeOkUsed;
  firstTimeOkUsed = false;


  if (ValidateData())
  {
    editorShell.BeginBatchChanges();

    if (gRemoveImageMap)
    {
      globalElement.removeAttribute("usemap");
      if (gImageMap)
      {
        editorShell.DeleteElement(gImageMap);
        gInsertNewIMap = true;
        gImageMap = null;
      }
    }
    else if (gImageMap)
    {
      // Assign to map if there is one
      var mapName = gImageMap.getAttribute("name");
      if (mapName != "")
      {
        globalElement.setAttribute("usemap", ("#"+mapName));
        if (globalElement.getAttribute("border") == "")
          globalElement.setAttribute("border", 0);
      }

      if (gInsertNewIMap)
      {
        try
        {
          editorShell.editorDocument.body.appendChild(gImageMap);
        //editorShell.InsertElementAtSelection(gImageMap, false);
        }
        catch (e)
        {
          dump("Exception occured in InsertElementAtSelection\n");
        }
      }
    }

    // All values are valid - copy to actual element in doc or
    //   element created to insert
    editorShell.CloneAttributes(imageElement, globalElement);
    if (gInsertNewImage)
    {
      try {
        // 'true' means delete the selection before inserting
        editorShell.InsertElementAtSelection(imageElement, true);
      } catch (e) {
        dump(e);
      }
    }

    // un-comment to see that inserting image maps does not work!
    /*test = editorShell.CreateElementWithDefaults("map");
    test.setAttribute("name", "testing");
    testArea = editorShell.CreateElementWithDefaults("area");
    testArea.setAttribute("shape", "circle");
    testArea.setAttribute("coords", "86,102,52");
    testArea.setAttribute("href", "test");
    test.appendChild(testArea);
    editorShell.InsertElementAtSelection(test, false);*/

    editorShell.EndBatchChanges();

    SaveWindowLocation();
    return true;
  }
  return false;
}
