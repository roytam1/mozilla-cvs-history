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
 *   Ben Goodger (30/09/99)
 */

// note: offloading this into a separate JS file where it should be.
// also got rid of seemingly useless function "foo". If it is needed, please
// feel free to reimplement ;)
 
var bundle = srGetStrBundle("chrome://profile/locale/newProfile1_2.properties");
var detect = false;

function InitializeValues()
{
	element = document.getElementById("ProfileName");
	element.value = parent.profName;
	
	element = document.getElementById("ProfileDir");
	element.value = parent.profDir;
}

function commit()
{
	var element;
	element = document.getElementById("ProfileName");
	parent.SetValue(element.id, element.value);
	parent.profName = element.value;

	element = document.getElementById("ProfileDir");
	parent.SetValue(element.id, element.value);
	parent.profDir = element.value;
}

// the getting procedure is unique to each page, since each page can different
// types of elements (not necessarily form elements). So each page must provide
// its own GetFields function
function GetFields()
{
  var profName = document.getElementById("ProfileName").value;
  var profDir  = document.getElementById("ProfileDir");
  for(var i = 0; i < profDir.attributes.length; i++)
  {
    // this is just an awful, bad, terrible hack. I shouldn't have to do this
    // to get the value. but all other ways return "undefined".
    if(profDir.attributes[i].nodeName == "value")
      profDirContent = profDir.attributes[i].nodeValue;
  }
  return [["ProfileName",profName],["ProfileDir",profDirContent]];
}

// the setting procedure is unique to each page, and as such each page
// must provide its own SetFields function
function SetFields(element,set)
{
  element = document.getElementById(element);
  //dump("In SetFields(" + element + "," + set + ");\n");
  if(element.id == "ProfileDir" && set != "")
    getProfileDir(set,false);
  else if(element.id == "ProfileName")
    element.value = set;
}  

// function createProfileWizard.js::chooseFolder();
// utility function responsible for displaying a folder selection dialog.
function chooseFolder(string)
{
	try {
    var fileSpec = Components.classes["component://netscape/filespecwithui"].createInstance();
    if(fileSpec) {
      fileSpec = fileSpec.QueryInterface(Components.interfaces.nsIFileSpecWithUI);
      return fileSpec.chooseDirectory(string);
    } else 
      return false;
  } catch(e) {
    return false;
  }
}

function removeChildren(which)
{
  if(which.hasChildNodes()) {
    for(var i = 0; i < which.childNodes.length; i++)
    {
      which.removeChild(which.lastChild);
    }
  }
}

// function createProfileWizard.js::getProfileDir();
// invoke a folder selection dialog for choosing the directory of profile storage.
function getProfileDir(folder, showPopup)
{
  if(showPopup)
    folder = chooseFolder("Choose Profile Directory");
  //dump("folder = " + folder + "\n");
  if(folder != undefined && folder ) {
    var folderText = document.getElementById("ProfileDir");
    oldText = document.getElementById("deffoldername");
    removeChildren(oldText);
    // covert the file URL to a native file path.
    // only need to do this if we called chooseFolder()
    // otherwise, it's already a native file path.   
    // this can happen when the user goes back and next after
    // selecting a folder
    if (showPopup) {
	    try {
		var spec = Components.classes["component://netscape/filespec"].createInstance();
		spec = spec.QueryInterface(Components.interfaces.nsIFileSpec);
		spec.URLString = folder;
		folder = spec.nativePath;
	    }
	    catch (ex) {
		dump("failed to convert URL to native path\n");
	    }
    }
    folderText.setAttribute("value",folder);
    if(!detect) {
      var useDefault = document.createElement("titledbutton");
      useDefault.setAttribute("value",bundle.GetStringFromName("useDefaultFolder"));
      useDefault.setAttribute("id","useDefaultButton");
      useDefault.setAttribute("onclick","UseDefaultFolder();");
      document.getElementById("folderbuttons").appendChild(useDefault);
      detect = true;
    }
  }

  //resize the parent window, because the native file path
  //may require a window resize.
  //comment this out for now, see bug #15825
  //parent.sizeToContent();
}

function UseDefaultFolder()
{
  var ProfileDir = document.getElementById("ProfileDir");
  ProfileDir.setAttribute("value","");
  var FolderButtons = document.getElementById("folderbuttons");
  if(FolderButtons.childNodes.length > 1)
    FolderButtons.removeChild(FolderButtons.lastChild);
  var span = document.getElementById("deffoldername")
  var text = document.createTextNode(bundle.GetStringFromName("defaultString"));
  span.appendChild(text);
  detect = false;

  //resize the parent window, because switching to use the default
  //may require a window resize.
  //comment this out for now, see bug #15825
  //parent.sizeToContent(); 
}
