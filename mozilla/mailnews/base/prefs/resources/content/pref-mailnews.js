function Startup()
{
  var startupFunc;
  try {
    startupFunc = document.getElementById("mailnewsEnableMapi").getAttribute('startFunc');
  }
  catch (ex) {
    startupFunc = null;
  }
  if (startupFunc)
    eval(startupFunc);
}

function setColorWell(menu) 
{
	// Find the colorWell and colorPicker in the hierarchy.
	var colorWell = menu.firstChild.nextSibling;
	var colorPicker = menu.firstChild.nextSibling.nextSibling.firstChild;

	// Extract color from colorPicker and assign to colorWell.
	var color = colorPicker.getAttribute('color');
	colorWell.style.backgroundColor = color;
}

function setHomePageToDefaultPage(folderFieldId)
{
  var homePageField = document.getElementById(folderFieldId);
  var prefs = Components.classes["@mozilla.org/preferences;1"].getService(Components.interfaces.nsIPref);
  var url = prefs.getDefaultLocalizedUnicharPref("mailnews.start_page.url");
  homePageField.value = url;
}

