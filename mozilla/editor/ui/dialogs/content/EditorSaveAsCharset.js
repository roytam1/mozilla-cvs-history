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
 * Frank Tang ftang@netscape.com
 */


var charsetList = new Array();
var charsetDict = new Array();
var title="";
var charset="";
var titleWasEdited = false;
var charsetWasChanged = false;
var insertNewContentType = false;
var contenttypeElement;
var initDone = false;


//Cancel() is in EdDialogCommon.js

// dialog initialization code
function Startup()
{
  if (!InitEditorShell())
    return;

  doSetOKCancel(onOK, null);

  // Create dialog object to store controls for easy access
  dialog = new Object;
  // GET EACH CONTROL -- E.G.:
  dialog.TitleInput = document.getElementById("TitleInput");
  dialog.charsetTree      = document.getElementById('CharsetTree'); 
  //dialog.charsetRoot  = document.getElementById('CharsetRoot'); 

 contenttypeElement = GetHTTPEquivMetaElement("content-type");
 if(! contenttypeElement )
 {
    contenttypeElement = CreateHTTPEquivMetaElement("content-type");
    if( ! contenttypeElement )
      window.close();
    insertNewContentType = true;
 }

  InitDialog();
  
  // SET FOCUS TO FIRST CONTROL
  //dialog.editBox.focus();
  //dialog.TitleInput.focus();
  dialog.charsetTree.focus();
  LoadAvailableCharSets();
  initDone = true;
}

function InitDialog() {
  
  dialog.TitleInput.value = editorShell.GetDocumentTitle();
  charset = editorShell.GetDocumentCharacterSet();
}

function onOK()
{
 if(ValidateData())
 {
   if(titleWasEdited) {
      editorShell.SetDocumentTitle(title);
   }

   if(charsetWasChanged) 
   {
      SetMetaElementContent(contenttypeElement, "text/html; charset=" + charset, insertNewContentType);     
      editorShell.SetDocumentCharacterSet(charset);
   }
   window.opener.ok = true;
   return true;
 }
 return false; 
}
function LoadAvailableCharSets()
{
  try {
    var ccm	= Components.classes['component://netscape/charset-converter-manager'];

    if (ccm) {
      ccm = ccm.getService();
      ccm = ccm.QueryInterface(Components.interfaces.nsICharsetConverterManager2);
      charsetList = ccm.GetDecoderList();
      charsetList = charsetList.QueryInterface(Components.interfaces.nsISupportsArray);
      charsetList.sort;
    }
  } catch(ex)
  {
    dump("failed to get charset mgr\n");
  }
  if (charsetList) 
  {
    var j=0;
    for (i = 0; i < charsetList.Count(); i++) 
    {
      atom = charsetList.GetElementAt(i);
      atom = atom.QueryInterface(Components.interfaces.nsIAtom);
  
      if (atom) {
        str = atom.GetUnicode();
        try {
          tit = ccm.GetCharsetTitle(atom);
        } catch (ex) {
          tit = str; //don't ignore charset detectors without a title
        }
      
        try {                                  
          visible = ccm.GetCharsetData(atom,'.notForBrowser');
          visible = false;
        } catch (ex) {
          visible = true;
          charsetDict[j] = new Array(2);
          charsetDict[j][0]  = tit;  
          charsetDict[j][1]  = str;
          j++;
          //dump('Getting invisible for:' + str + ' failed!\n');
        }
      } //atom
  
    } //for

    ClearTreelist(dialog.charsetTree);
    charsetDict.sort();
    var selItem;
    if (charsetDict) 
    {
      for (i = 0; i < charsetDict.length; i++) 
      {
        try {  //let's beef up our error handling for charsets without label / title

//dump("add " + charsetDict[i][0] + charsetDict[i][1] + "\n");
          var item = AppendStringToTreelist(dialog.charsetTree, charsetDict[i][0]);
          if(item) {
             var row= item.firstChild;
             if(row) {
                var cell= row.firstChild;
                if(cell) {
                   cell.setAttribute("data", charsetDict[i][1]);
                }
             }
             if(charset == charsetDict[i][1] ) 
             {
               selItem = item;
//dump("hit default " + charset + "\n");
             }
          }
        } //try
        catch (ex) {
          dump("*** Failed to add charset: " + tit + ex + "\n");
        } //catch

      } //for
    } // if
    if(selItem) {
        try {
        dialog.charsetTree.selectItem(selItem);
        dialog.charsetTree.ensureElementIsVisible(selItem);
        } catch (ex) {
          dump("*** Failed to select and ensure : " + ex + "\n");
        }
    }
  } // if
}
function SelectCharset()
{
  if(initDone) {
    try {
      charset = GetSelectedTreelistAttribute(dialog.charsetTree, "data");
      //dump("charset = " + charset + "\n");
      if(charset != "") {
         charsetWasChanged = true;
      }
    } catch(ex) {
      dump("failed to get selected data" + ex + "\n");
    }
  }
}
function ValidateData()
{
  title=dialog.TitleInput.value.trimString();
  return true;
}
function TitleChanged()
{
  titleWasEdited = true; 
}
// copy from EdPageProps.js
function GetMetaElement(name)
{
  if (name)
  {
    name = name.toLowerCase();
    if (name != "")
    {
      var metaNodes = editorShell.editorDocument.getElementsByTagName("meta");
      if (metaNodes && metaNodes.length > 0)
      {
        for (var i = 0; i < metaNodes.length; i++)
        {
          var metaNode = metaNodes.item(i);
          if (metaNode && metaNode.getAttribute("name") == name)
            return metaNode;
        }
      }
    }
  }
  return null;
}
function GetHTTPEquivMetaElement(name)
{
  if (name)
  {
    name = name.toLowerCase();
    if (name != "")
    {
      var metaNodes = editorShell.editorDocument.getElementsByTagName("meta");
      if (metaNodes && metaNodes.length > 0)
      {
        for (var i = 0; i < metaNodes.length; i++)
        {
          var metaNode = metaNodes.item(i);
          if (metaNode && metaNode.getAttribute("http-equiv") == name)
            return metaNode;
        }
      }
    }
  }
  return null;
}

function CreateMetaElement(name)
{
  metaElement = editorShell.CreateElementWithDefaults("meta");
  if (metaElement)
    metaElement.setAttribute("name", name);
  else
    dump("Failed to create metaElement!\n");
  
  return metaElement;
}
function CreateHTTPEquivMetaElement(name)
{
  metaElement = editorShell.CreateElementWithDefaults("meta");
  if (metaElement)
    metaElement.setAttribute("http-equiv", name);
  else
    dump("Failed to create httpequivMetaElement!\n");
  
  return metaElement;
}

function CreateHTTPEquivElement(name)
{
  metaElement = editorShell.CreateElementWithDefaults("meta");
  if (metaElement)
    metaElement.setAttribute("http-equiv", name);
  else
    dump("Failed to create metaElement for http-equiv!\n");
  
  return metaElement;
}

// Change "content" attribute on a META element,
//   or delete entire element it if content is empty
// This uses undoable editor transactions 
function SetMetaElementContent(metaElement, content, insertNew)
{
  if (metaElement)
  {
    if(!content || content == "")
    {
      if (!insertNew)
        editorShell.DeleteElement(metaElement);
    }
    else
    {
      if (insertNew)
      {
        // Don't need undo for set attribute, just for InsertElement
        metaElement.setAttribute("content", content);
        AppendHeadElement(metaElement);
      }
      else
        editorShell.SetAttribute(metaElement, "content", content);
    }
  }
}

function GetHeadElement()
{
  var headList = editorShell.editorDocument.getElementsByTagName("head");
  if (headList)
    return headList.item(0);
  
  return null;
}

function AppendHeadElement(element)
{
  var head = GetHeadElement();
  if (head)
    head.appendChild(element);
}
