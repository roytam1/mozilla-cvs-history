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

var title = "";
var author = "";
var description = "";
var authorElement;
var descriptionElement;
var insertNewAuthor = false;
var insertNewDescription = false;
var titleWasEdited = false;
var authorWasEdited = false;
var descWasEdited = false;

//Cancel() is in EdDialogCommon.js
// dialog initialization code
function Startup()
{
  if (!InitEditorShell())
    return;

  dialog = new Object;
  if (!dialog)
  {
    dump("Failed to create dialog object!!!\n");
    window.close();
  }
  dialog.PageLocation     = document.getElementById("PageLocation");
  dialog.TitleInput       = document.getElementById("TitleInput");
  dialog.AuthorInput      = document.getElementById("AuthorInput");
  dialog.DescriptionInput = document.getElementById("DescriptionInput");
  doSetOKCancel(onOK, null);
  
  // Default string for new page is set from DTD string in XUL,
  //   so set only if not new doc URL
  var location = editorShell.editorDocument.location;
  if (location != "about:blank")
  {
    dialog.PageLocation.setAttribute("value", editorShell.editorDocument.location);

    // Get last-modified file date+time
    // TODO: Convert this to local time?
    var lastmod = editorShell.editorDocument.lastModified;  // get string of last modified date
    var lastmoddate = Date.parse(lastmod);                  // convert modified string to date
    if(lastmoddate == 0)                                    // unknown date (or January 1, 1970 GMT)
      lastmod = GetString("Unknown");

    document.getElementById("PageModDate").setAttribute("value",lastmod);
  }

  authorElement = GetMetaElement("author");
  if (!authorElement)
  {
    authorElement = CreateMetaElement("author");
    if (!authorElement)
      window.close();

    insertNewAuthor = true;
  }

  descriptionElement = GetMetaElement("description");
  if (!descriptionElement)
  {
    descriptionElement = CreateMetaElement("description");
    if (!descriptionElement)
      window.close();

    insertNewDescription = true;
  }
  
  InitDialog();

  SetTextfieldFocus(dialog.TitleInput);
}

function InitDialog()
{
  dialog.TitleInput.value = editorShell.GetDocumentTitle();
  dialog.AuthorInput.value = authorElement.getAttribute("content");
  dialog.DescriptionInput.value = descriptionElement.getAttribute("content");
}

function TextfieldChanged(ID)
{
  switch(ID)
  {
    case "TitleInput":
      titleWasEdited = true;
      break;
    case "AuthorInput":
      authorWasEdited = true;
      break;
    case "DescriptionInput":
      descWasEdited = true;
      break;
  }
}

function ValidateData()
{
  title = dialog.TitleInput.value.trimString();
  author = dialog.AuthorInput.value.trimString();
  description = dialog.DescriptionInput.value.trimString();
  return true;
}

function onOK()
{
  if (ValidateData())
  {
    if (titleWasEdited)
    {
      // Set title contents even if string is empty
      //  because TITLE is a required HTML element
      editorShell.SetDocumentTitle(title);
    }
    
    if (authorWasEdited)
      SetMetaElementContent(authorElement, author, insertNewAuthor);
    if (descWasEdited)
      SetMetaElementContent(descriptionElement, description, insertNewDescription);

    return true; // do close the window
  }
  return false;
}

