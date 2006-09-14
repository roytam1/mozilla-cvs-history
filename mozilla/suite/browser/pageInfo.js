/* -*- Mode: Java; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): smorrison@gte.com
 *   Terry Hayes <thayes@netscape.com>
 *   Daniel Brooks <db48x@yahoo.com>
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
 * ***** END LICENSE BLOCK *****
*/

// mmm, yummy. global variables.
var theWindow = null;
var theDocument = null;

var linkList = new Array();
var formList = new Array();
var imageList = new Array();

var linkIndex = 0;
var formIndex = 0;
var imageIndex = 0;
var frameCount = 0;

// a number of services I'll need later
// the cache services
const nsICacheService = Components.interfaces.nsICacheService;
const cacheService = Components.classes["@mozilla.org/network/cache-service;1"].getService(nsICacheService);
var httpCacheSession = cacheService.createSession("HTTP", 0, true);
var ftpCacheSession = cacheService.createSession("FTP", 0, true);

// scriptable date formater, for pretty printing dates
const nsIScriptableDateFormat = Components.interfaces.nsIScriptableDateFormat;
var dateService = Components.classes["@mozilla.org/intl/scriptabledateformat;1"].getService(nsIScriptableDateFormat);

// namespaces, don't need all of these yet...
const XLinkNS = "http://www.w3.org/1999/xlink";
const XULNS   = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
const XMLNS   = "http://www.w3.org/XML/1998/namespace";
const XHTMLNS = "http://www.w3.org/1999/xhtml";

/* Overlays register init functions here.
 *   Add functions to call by invoking "onLoadRegistry.append(XXXLoadFunc);"
 *   The XXXLoadFunc should be unique to the overlay module, and will be
 *   invoked as "XXXLoadFunc();"
 */
var onLoadRegistry = [ ];
 
/* Called when PageInfo window is loaded.  Arguments are:
 *  window.arguments[0] - document to use for source (null=Page Info, otherwise Frame Info)
 *  window.arguments[1] - tab name to display first (may be null)
*/
function onLoadPageInfo()
{
  //dump("===============================================================================\n");
  var theBundle = document.getElementById("pageinfobundle");
  var unknown = theBundle.getString("unknown");

  var docTitle = "";
  if("arguments" in window && window.arguments.length >= 1 && window.arguments[0])
  {
    theWindow = null;
    theDocument = window.arguments[0];
    docTitle = theBundle.getString("frameInfo.title");
  } 
  else 
  {
    if ("gBrowser" in window.opener)
      theWindow = window.opener.gBrowser.contentWindow;
    else
      theWindow = window.opener.frames[0];
    theDocument = theWindow.document;
    docTitle = theBundle.getString("pageInfo.title");
  }

  document.title = docTitle;

  // do the easy stuff first
  makeGeneralTab();

  /* Call registered overlay init functions */
  for (x in onLoadRegistry)
  {
    onLoadRegistry[x]();
  }

  /* Select the requested tab, if the name is specified */
  if ("arguments" in window && window.arguments.length > 1)
  {
    var tabName = window.arguments[1];

    if (tabName)
    {
      var tabControl = document.getElementById("tabbox");
      var tab = document.getElementById(tabName);

      if (tabControl && tab)
      {
        tabControl.selectedTab = tab;
      }
    }
  }
}

function makeGeneralTab()
{
  var theBundle = document.getElementById("pageinfobundle");
  var unknown = theBundle.getString("unknown");
  var notSet = theBundle.getString("notset");
  
  var title = (theDocument.title) ? theBundle.getFormattedString("pageTitle", [theDocument.title]) : theBundle.getString("noPageTitle");
  document.getElementById("titletext").value = title;

  var url = theDocument.location;
  document.getElementById("urltext").value = url;

  var mode = ("compatMode" in theDocument && theDocument.compatMode == "BackCompat") ? theBundle.getString("generalQuirksMode") : theBundle.getString("generalStrictMode");
  document.getElementById("modetext").value = mode;

  // find out the mime type
  var mimeType = theDocument.contentType || unknown;
  document.getElementById("typetext").value = mimeType;
  
  // get the meta tags
  var metaNodes = theDocument.getElementsByTagName("meta");
  var metaOutliner = document.getElementById("metaoutliner");
  var metaView = new pageInfoOutlinerView(["meta-name","meta-content"]);

  metaOutliner.outlinerBoxObject.view = metaView;

  var length = metaNodes.length;
  for (var i = 0; i < length; i++)
  {    
    metaView.addRow([metaNodes[i].name || metaNodes[i].httpEquiv, metaNodes[i].content]);
  }
  metaView.rowCountChanged(0, length);
  
  // get the document characterset
  var encoding = theDocument.characterSet;
  document.getElementById("encodingtext").value = encoding;

  // get the date of last modification
  var modifiedText = formatDate(theDocument.lastModified, notSet);
  document.getElementById("modifiedtext").value = modifiedText;
  
  // get cache info
  var sourceText = theBundle.getString("generalNotCached");
  var expirationText = theBundle.getString("generalNoExpiration");
  var sizeText = unknown;

  var pageSize = 0; 
  var kbSize = 0;
  var expirationTime = 0;

  try
  {
    var cacheEntryDescriptor = httpCacheSession.openCacheEntry(url, Components.interfaces.nsICache.ACCESS_READ, true);
    if(cacheEntryDescriptor)
    { 
      switch(cacheEntryDescriptor.deviceID)
      {
        case "disk":
          sourceText = theBundle.getString("generalDiskCache");
          break;
        case "memory":
          sourceText = theBundle.getString("generalMemoryCache");
          break;
        default:
          sourceText = cacheEntryDescriptor.deviceID;
          break;
      }

      pageSize = cacheEntryDescriptor.dataSize;
      kbSize = pageSize / 1024;
      sizeText = pageSize; // + " (" + 0 + "Kb)";

      expirationText = formatDate(cacheEntryDescriptor.expirationTime*1000, notSet);
    }
  }
  catch(ex)
  {
    try
    {
      cacheEntryDescriptor = ftpCacheSession.openCacheEntry(url, Components.interfaces.nsICache.ACCESS_READ, true);
      if (cacheEntryDescriptor)
      {
        switch(cacheEntryDescriptor.deviceID)
        {
          case "disk":
            sourceText = theBundle.getString("generalDiskCache");
            break;
          case "memory":
            sourceText = theBundle.getString("generalMemoryCache");
            break;
          default:
            sourceText = cacheEntryDescriptor.deviceID;
            break;
        }

        pageSize = cacheEntryDescriptor.dataSize;
        kbSize = pageSize / 1024;
        sizeText = pageSize; // + " (" + 0 + "Kb)";

        expirationText = formatDate(cacheEntryDescriptor.expirationTime*1000, notSet);
      }
    }
    catch(ex2)
    {
      sourceText = theBundle.getString("generalNotCached");
    }
  }
  document.getElementById("sourcetext").value = sourceText;
  document.getElementById("expirestext").value = expirationText;
  document.getElementById("sizetext").value = sizeText;
}

//******** Form Stuff
function makeFormTab()
{
  var formOutliner = document.getElementById("formoutliner");
  var formPreview = document.getElementById("formpreview");
  
  var formView = new pageInfoOutlinerView(["form-number","form-name","form-action","form-method"]);
  var fieldView = new pageInfoOutlinerView(["field-number","field-label","field-field","field-type","field-value"]);
  formOutliner.outlinerBoxObject.view = formView;
  formPreview.outlinerBoxObject.view = fieldView;

  formList = grabAllForms(theWindow, theDocument);
  formIndex = 0;

  var length = formList.length;
  for (var i = 0; i < length; i++)
  {
    var elem = formList[i];
    formView.addRow([++formIndex, elem.name, elem.method, elem.action]);
  }
  formView.rowCountChanged(0, length);

  formView.selection.select(0);
}

function grabAllForms(aWindow, aDocument)
{
  var theList = [];

  if (aWindow && aWindow.frames.length > 0)
  {
    var length = aWindow.frames.length;
    for (var i = 0; i < length; i++)
    {
      var frame = aWindow.frames[i];
      theList = theList.concat(grabAllForms(frame, frame.document));
    }
  }

  if ("forms" in aDocument)
    return theList.concat(aDocument.forms);
  else
    return theList.concat(aDocument.getElementsByTagNameNS(XHTMLNS, "form"));
}

function onFormSelect()
{
  var theBundle = document.getElementById("pageinfobundle");
  var formOutliner = document.getElementById("formoutliner");
  var formView = formOutliner.outlinerBoxObject.view;
  if (!formView.rowCount) return;

  if (formView.selection.count == 1)
  {
    var formPreview = document.getElementById("formpreview");
    var fieldView = new pageInfoOutlinerView(["field-number","field-label","field-field","field-type","field-value"]);
    formPreview.outlinerBoxObject.view = fieldView;

    var clickedRow = formView.selection.currentIndex;
    var formnum = formView.getCellText(clickedRow, "form-number");
    var form = formList[formnum-1];
    var ft = null;
    if (form.name)
      ft = theBundle.getFormattedString("formTitle", [form.name]);
    else
      ft = theBundle.getString("formUntitled");

    document.getElementById("formname").value = ft || theBundle.getString("formUntitled");
    document.getElementById("formenctype").value = form.encoding || theBundle.getString("default");
    document.getElementById("formtarget").value = form.target || theBundle.getString("formDefaultTarget");

    var formfields = form.elements;

    var length = formfields.length;
    var i = 0;

    var checked = theBundle.getString("formChecked");
    var unchecked = theBundle.getString("formUnchecked");    

    for (i = 0; i < length; i++)
    {
      var elem = formfields[i];

      if(elem.nodeName.toLowerCase() == "button")
        fieldView.addRow([i+1, "", elem.name, elem.type, getValueText(elem)]);
      else
      {
        var val = (elem.type == "password") ? theBundle.getString("formPassword") : elem.value;
        fieldView.addRow([i+1, "", elem.name, elem.type, val]);
      }
    }

    var labels = form.getElementsByTagName("label");
    var llength = labels.length

    for (i = 0; i < llength; i++)
    {
      var whatfor = labels[i].getAttribute("for") || findFirstControl(labels[i]);
      var labeltext = getValueText(labels[i]);

      for(var j = 0; j < length; j++)
        if (formfields[j] == whatfor || formfields[j].name == whatfor)
          fieldView.setCellText(j, "field-label", labeltext);
    }

    fieldView.rowCountChanged(0, length);
  }
}

function findFirstControl(node)
{
  function FormControlFilter() 
  {
    this.acceptNode = function(node)
    {
      switch (node.nodeName.toLowerCase())
      {
        case "input":
        case "select":
        case "button":
        case "textarea":
        case "object":
          return NodeFilter.FILTER_ACCEPT;
          break;
        default:
          return NodeFilter.FILTER_SKIP;
          break;
      }
      return NodeFilter.FILTER_SKIP;   // placate the js compiler
    }     
  }

  var nodeFilter = new FormControlFilter;
  var iterator = theDocument.createTreeWalker(node, NodeFilter.SHOW_ELEMENT, nodeFilter, true);

  return iterator.nextNode();
}

//******** Link Stuff
function makeLinkTab()
{
  //var start = new Date();
  var theBundle = document.getElementById("pageinfobundle");
  var linkOutliner = document.getElementById("linkoutliner");

  var linkView = new pageInfoOutlinerView(["link-number","link-name","link-address","link-type"]);
  linkOutliner.outlinerBoxObject.view = linkView;

  linkList = grabAllLinks(theWindow, theDocument);

  var linkAnchor = theBundle.getString("linkAnchor");
  var linkArea = theBundle.getString("linkArea");
  var linkSubmit = theBundle.getString("linkSubmit");
  var linkSubmission = theBundle.getString("linkSubmission");
  var linkRel = theBundle.getString("linkRel");
  var linkStylesheet = theBundle.getString("linkStylesheet");
  var linkRev = theBundle.getString("linkRev");

  var linktext = null;
  linkIndex = 0;

  var length = linkList.length;
  for (var i = 0; i < length; i++)
  {
    var elem = linkList[i];
    switch (elem.nodeName.toLowerCase())
    {
      case "a":
        linktext = getValueText(elem);
        linkView.addRow([++linkIndex, linktext, elem.href, linkAnchor]);
        break;
      case "area":
        linkView.addRow([++linkIndex, elem.alt, elem.href, linkArea]);
        break;
      case "input":
        linkView.addRow([++linkIndex, elem.value || linkSubmit, elem.form.action, linkSubmission]);
        break;
      case "link":
        if (elem.rel)
        {
          // should this test use regexes to be a little more lenient wrt whitespace?
          if (elem.rel.toLowerCase() == "stylesheet" || elem.rel.toLowerCase() == "alternate stylesheet")
            linktext = linkStylesheet;
          else
            linktext = linkRel;
        }
        else
          linktext = linkRev;
        linkView.addRow([++linkIndex, elem.rel || elem.rev, elem.href, linktext]);
        break;
      default:
        dump("Page Info - makeLinkTab(): Hey, that's an odd one! ("+elem+")");
        break;
    }
  }
  linkView.rowCountChanged(0, length);

  //var end = new Date();
  //dump("links tab took "+(end-start)+"ms to build.\n");
}

function grabAllLinks(aWindow,aDocument)
{
  var theList = [];

  if (aWindow && aWindow.frames.length > 0)
  {
    var num = aWindow.frames.length;
    for (var i = 0; i < num; i++)
    {
      var frame = aWindow.frames[i];
      theList = theList.concat(grabAllLinks(frame, frame.document));
    }
  }

  theList = theList.concat(aDocument.getElementsByTagName("link"));

  var inputList = aDocument.getElementsByTagName("input");
  var length = inputList.length;
  for (i = 0; i < length; i++)
    if (inputList[i].type.toLowerCase() == "submit")
      theList = theList.concat(inputList[i]);

  if ("links" in aDocument)
    return theList.concat(aDocument.links);
  else
    return theList.concat(aDocument.getElementsByTagNameNS(XHTMLNS, "a"));
}

function openURL(target)
{
  var url = target.parentNode.childNodes[2].value;
  window.open(url, "_blank", "chrome");
}

//******** Image Stuff
function makeMediaTab()
{
  var theBundle = document.getElementById("pageinfobundle");
  var imageOutliner = document.getElementById("imageoutliner");

  var imageView = new pageInfoOutlinerView(["image-number","image-address","image-type"]);
  imageOutliner.outlinerBoxObject.view = imageView;

  imageList = grabAllMedia(theWindow, theDocument);

  var mediaImg = theBundle.getString("mediaImg");
  var mediaApplet = theBundle.getString("mediaApplet");
  var mediaObject = theBundle.getString("mediaObject");
  var mediaEmbed = theBundle.getString("mediaEmbed");
  var mediaLink = theBundle.getString("mediaLink");
  var mediaInput = theBundle.getString("mediaInput");

  var row = null;
  var length = imageList.length;
  imageIndex = 0;

  for (var i = 0; i < length; i++)
  {
    var elem = imageList[i];
    switch (elem.nodeName.toLowerCase())
    {
      case "img":
        imageView.addRow([++imageIndex, elem.src, mediaImg]);
        break;
      case "input":
        imageView.addRow([++imageIndex, elem.src, mediaInput]);
        break;
      case "applet":
        imageView.addRow([++imageIndex, elem.code || elem.object, mediaApplet]);
        break;
      case "object":
        imageView.addRow([++imageIndex, elem.data, mediaObject]);
        break;
      case "embed":
        imageView.addRow([++imageIndex, elem.src, mediaEmbed]);
        break;
      case "link":
        imageView.addRow([++imageIndex, elem.href, mediaLink]);
        break;
      default:
        dump("Page Info - makeMediaTab(): hey, that's an odd one! ("+elem+")");;
        break;
    }
  }
  imageView.rowCountChanged(0, length);
  
  imageView.selection.select(0);
}

function grabAllMedia(aWindow, aDocument)
{
  var theList = [];

  if (aWindow && aWindow.frames.length > 0)
  {
    var num = aWindow.frames.length;
    for (var i = 0; i < num; i++)
    {
      var frame = aWindow.frames[i];
      theList = theList.concat(grabAllMedia(frame, frame.document));
    }
  }

  theList = theList.concat(aDocument.getElementsByTagName("embed"), aDocument.applets, aDocument.getElementsByTagName("object"));

  var inputList = aDocument.getElementsByTagName("input");
  var length = inputList.length
  for (i = 0; i < length; i++)
    if(inputList[i].type.toLowerCase() == "image")
      theList = theList.concat(inputList[i]);

  var linkList = aDocument.getElementsByTagName("link");
  length = linkList.length;
  for (i = 0; i < length; i++)
    if(linkList[i].rel.match(/\bicon\b/i))
      theList = theList.concat(linkList[i]);

  if ("images" in aDocument)
    return theList.concat(aDocument.images);
  else
    return theList.concat(aDocument.getElementsByTagNameNS(XHTMLNS, "img"));
}  

function getSource( item )
{
  // Return the correct source without strict warnings
  if (item.href != null) {
    return item.href;
  } else if (item.src != null) {
    return item.src;
  }
  return null;
}

function getSelectedItem(outliner)
{
  var view = outliner.outlinerBoxObject.view;
  if (!view.rowCount) return null;

  // Only works if only one item is selected
  var clickedRow = outliner.outlinerBoxObject.selection.currentIndex;
  var lineNum = view.getCellText(clickedRow, "image-number");
  return imageList[lineNum - 1];
}

function saveMedia()
{
  var outliner = document.getElementById("imageoutliner");
  var item = getSelectedItem(outliner);
  var url = getAbsoluteURL(getSource(item), item);

  if (url) {
    saveURL(url, null, 'SaveImageTitle', false );
  }
}

function onImageSelect()
{
  var outliner = document.getElementById("imageoutliner");
  var saveAsButton = document.getElementById("imagesaveasbutton");

  if (outliner.outlinerBoxObject.selection.count == 1)
  {
    makePreview(getSelectedItem(outliner));
    saveAsButton.setAttribute("disabled", "false");
  } else {
    saveAsButton.setAttribute("disabled", "true");
  }
}

function makePreview(item)
{
  var theBundle = document.getElementById("pageinfobundle");
  var unknown = theBundle.getString("unknown");
  var notSet = theBundle.getString("notset");

  var url = ("src" in item && item.src) || ("code" in item && item.code) || ("data" in item && item.data) || ("href" in item && item.href) || unknown;  // it better have at least one of those...
  document.getElementById("imageurltext").value = url;
  document.getElementById("imagetitletext").value = item.title || notSet;
  document.getElementById("imagealttext").value = ("alt" in item && item.alt) || getValueText(item) || notSet;
  document.getElementById("imagelongdesctext").value = ("longDesc" in item && item.longDesc) || notSet;

  // find out the mime type
  var mimeType = unknown;
  if (item.nodeName.toLowerCase() != "input")
    mimeType = ("type" in item && item.type) || ("codeType" in item && item.codeType) || ("contentType" in item && item.contentType) || unknown;
  document.getElementById("imagetypetext").value = mimeType;

  // get cache info
  var sourceText = theBundle.getString("generalNotCached");
  var expirationText = theBundle.getString("unknown");
  var sizeText = theBundle.getString("unknown");

  var expirationTime = 0;
  var expirationDate = null;

  try
  {
    var cacheEntryDescriptor = httpCacheSession.openCacheEntry(url, Components.interfaces.nsICache.ACCESS_READ, true);   // open for READ, in blocking mode
    if (cacheEntryDescriptor)
    {
      switch(cacheEntryDescriptor.deviceID)
      {
        case "disk":
          sourceText = theBundle.getString("generalDiskCache");
          break;
        case "memory":
          sourceText = theBundle.getString("generalMemoryCache");
          break;
        default:
          sourceText = cacheEntryDescriptor.deviceID;
          break;
      }

      sizeText = cacheEntryDescriptor.dataSize;

      expirationText = formatDate(cacheEntryDescriptor.expirationTime*1000, notSet);
    }
  }
  catch(ex)
  {
    try
    {
      cacheEntryDescriptor = ftpCacheSession.openCacheEntry(url, Components.interfaces.nsICache.ACCESS_READ, true);   // open for READ, in blocking mode
      if (cacheEntryDescriptor)
      {
        switch(cacheEntryDescriptor.deviceID)
        {
          case "disk":
            sourceText = theBundle.getString("generalDiskCache");
            break;
          case "memory":
            sourceText = theBundle.getString("generalMemoryCache");
            break;
          default:
            sourceText = cacheEntryDescriptor.deviceID;
            break;
        }

        sizeText = cacheEntryDescriptor.dataSize;

        expirationText = formatDate(cacheEntryDescriptor.expirationTime*1000, notSet);
      }
    }
    catch(ex2)
    {
      sourceText = theBundle.getString("generalNotCached");
    }
  }
  document.getElementById("imagesourcetext").value = sourceText;
  document.getElementById("imageexpirestext").value = expirationText;
  document.getElementById("imagesizetext").value = sizeText;

  // perhaps these can be done in the future
  //document.getElementById("imageplugintext").value = "--";
  //document.getElementById("imagecharsettext").value = "--";

  var width = ("width" in item && item.width) || "";
  var height = ("height" in item && item.height) || "";

  document.getElementById("imagewidth").value = theBundle.getFormattedString("mediaWidth", [width]);
  document.getElementById("imageheight").value = theBundle.getFormattedString("mediaHeight", [height]);

  // also can't be done at the moment
  //document.getElementById("imageencryptiontext").value = "--";

  var imageContainer = document.getElementById("theimagecontainer");
  var oldImage = document.getElementById("thepreviewimage");

  var newImage = null;
  var nn = item.nodeName.toLowerCase();
  if (nn == "link" || nn == "input")
  {
    newImage = new Image();
    newImage.src = getAbsoluteURL(getSource(item), item);
  }
  else
  {
    newImage = item.cloneNode(true);
    newImage.src = ("src" in item && item.src) || ("href" in item && item.href);  // weird funky hack, I know :P
  }

  newImage.setAttribute("id", "thepreviewimage");
  if ("width" in item && item.width)
    newImage.width = item.width;
  if ("height" in item && item.height)
    newImage.height = item.height;
  newImage.removeAttribute("align"); // just in case.

  imageContainer.removeChild(oldImage);
  imageContainer.appendChild(newImage);
}


//******** Other Misc Stuff
// Modified from the Links Panel v2.3, http://segment7.net/mozilla/links/links.html
// parse a node to extract the contents of the node
// linkNode doesn't really _have_ to be link
function getValueText(linkNode)
{
  var valueText = "";
  
  var length = linkNode.childNodes.length;
  for (var i = 0; i < length; i++)
  {
    var childNode = linkNode.childNodes[i];
    var nodeType = childNode.nodeType;
    if (nodeType == Node.TEXT_NODE)
      valueText += " " + childNode.nodeValue;
    else if (nodeType == Node.ELEMENT_NODE)
    {
      if (childNode.nodeName.toLowerCase() == "img")
        valueText += " " + getAltText(childNode);
      else
        valueText += " " + getValueText(childNode);
    }
  }

  return stripWS(valueText);
}

// Copied from the Links Panel v2.3, http://segment7.net/mozilla/links/links.html
// traverse the tree in search of an img or area element and grab its alt tag
function getAltText(node)
{
  var altText = "";
  
  if (node.alt)
    return node.alt;
  var length = node.childNodes.length;
  for (var i = 0; i < length; i++)
    if ((altText = getAltText(node.childNodes[i]) != undefined))  // stupid js warning...
      return altText;
  return "";
}

// Copied from the Links Panel v2.3, http://segment7.net/mozilla/links/links.html
// strip leading and trailing whitespace, and replace multiple consecutive whitespace characters with a single space
function stripWS(text)
{
  var middleRE = /\s+/g;
  var endRE = /(^\s+)|(\s+$)/g;

  text = text.replace(middleRE, " ");
  return text.replace(endRE, "");
}

function formatDate(datestr, unknown)
{
  var date = new Date(datestr);
  return (date.valueOf()) ? dateService.FormatDateTime("", dateService.dateFormatLong, dateService.timeFormatSeconds, date.getFullYear(), date.getMonth()+1, date.getDate(), date.getHours(), date.getMinutes(), date.getSeconds()) : unknown;
}

/*
 * Takes care of XMLBase and <base>
 * url is the possibly relative url.
 * node is the node where the url was given (needed for XMLBase)
 *
 * This function is called in many places as a workaround for bug 72524
 * Once bug 72522 is fixed this code should use the Node.baseURI attribute
 *
 * for node==null or url=="", empty string is returned
 *
 * This is basically just copied from http://lxr.mozilla.org/seamonkey/source/xpfe/browser/resources/content/metadata.js
 */

function getAbsoluteURL(url, node)
{
  if (!url || !node)
    return "";
  var urlArr = new Array(url);

  var doc = node.ownerDocument;
  if (node.nodeType == Node.ATTRIBUTE_NODE)
    node = node.ownerElement;

  while (node && node.nodeType == Node.ELEMENT_NODE) 
  {
    var att = node.getAttributeNS(XMLNS, "base");
    if (att != "")
      urlArr.unshift(att);

    node = node.parentNode;
  }

  // Look for a <base>.
  var baseTags = doc.getElementsByTagNameNS(XHTMLNS, "base");

  if (baseTags && baseTags.length) 
  {
    urlArr.unshift(baseTags[baseTags.length - 1].getAttribute("href"));
  }

  // resolve everything from bottom up, starting with document location
  var ioService = Components.classes["@mozilla.org/network/io-service;1"].getService(Components.interfaces.nsIIOService);
  var URL = ioService.newURI(doc.location.href, null, null);

  for (var i=0; i<urlArr.length; i++) 
  {
    URL.spec = URL.resolve(urlArr[i]);
  }

  return URL.spec;
}

//******** define a js object to implement nsIOutlinerView
function pageInfoOutlinerView(columnids)
{
  // columnids is an array of strings indicating the names of the columns, in order
  this.columnids = columnids;
  this.colcount = columnids.length
  this.rows = 0;
  this.outliner = null;
  this.data = new Array;
  this.selection = null;
}

pageInfoOutlinerView.prototype = {
  set rowCount(c) { throw "rowCount is a readonly property"; },
  get rowCount() { return this.rows; },

  setOutliner: function(outliner) 
  {
    this.outliner = outliner;
  },

  getCellText: function(row, column)
  {
    var colidx = 0;
    // loop through the list of column names to find the index to the column the should be worrying about. very much a hack, but what can you do?
    while(colidx < this.colcount && column != this.columnids[colidx])
      colidx++;
    return this.data[row][colidx] || "";
  },

  setCellText: function(row, column, value) 
  {
    var colidx = 0;
    // loop through the list of column names to find the index to the column the should be worrying about. very much a hack, but what can you do?
    while(colidx < this.colcount && column != this.columnids[colidx])
      colidx++;
    this.data[row][colidx] = value;
  },

  addRow: function(row)
  {
    var oldrowcount = this.rows;
    this.rows = this.data.push(row);
  },

  addRows: function(rows)
  {
    var oldrowcount = this.rowCount;

    var length = rows.length;
    for(var i = 0; i < length; i++)
      this.rows = this.data.push(rows[i]);
  },

  rowCountChanged: function(index, count)
  {
    this.outliner.rowCountChanged(index, count);
  },

  invalidate: function()
  {
    this.outliner.invalidate();
  },

  clear: function()
  {
    this.data = null;
  },

  getRowProperties: function(row, column, prop) { },
  getCellProperties: function(row, prop) { },
  getColumnProperties: function(column, elem, prop) { },
  isContainer: function(index) { return false; },
  isContainerOpen: function(index) { return false; },
  isSeparator: function(index) { return false; },
  isSorted: function() { return false; },
  canDropOn: function(index) { return false; },
  canDropBeforeAfter: function(index, before) { return false; },
  drop: function(row, orientation) { return false; },
  getParentIndex: function(index) { return 0; },
  hasNextSibling: function(index, after) { return false; },
  getLevel: function(index) { return 0; },
  toggleOpenState: function(index) { },
  cycleHeader: function(col, elem) { },
  selectionChanged: function() { },
  cycleCell: function(row, column) { },
  isEditable: function(row, column) { return false; },
  performAction: function(action) { },
  performActionOnRow: function(action, row) { },
  performActionOnCell: function(action, row, column) { }
};
