// -*- Mode: Java -*-

var sidebar_name    = '';   // Name for preferences (e.g. 'sidebar.<name>.foo')
var sidebar_uri     = '';   // Content to load in sidebar frame
var sidebar_width   = 0;    // Desired width of sidebar
var sidebar_pref    = '';   // Base for preferences (e.g. 'sidebar.browser')
var is_sidebar_open = false; 
var prefs           = null; // Handle to preference interface

function init_sidebar(name, uri, width) {
  sidebar_name  = name;
  sidebar_uri   = uri;
  sidebar_width = width;
  sidebar_pref  = 'sidebar.' + name;

  // Open/close sidebar based on saved pref.
  // This may be replaced by another system by hyatt.
  prefs = Components.classes['component://netscape/preferences'];
  if (prefs) {
    prefs = prefs.getService();
  }
  if (prefs) {
    prefs = prefs.QueryInterface(Components.interfaces.nsIPref);
  }
  if (prefs) {
    prefs.SetDefaultBoolPref(sidebar_pref + '.open', false);

    // The sidebar is closed by default, so open it only if the
    //    preference is set to true.
    if (prefs.GetBoolPref(sidebar_pref + '.open')) {
      toggle_open_close();
    }
  }
}

function toggle_open_close() {

  var sidebar = document.getElementById('sidebarframe');
  var grippy = document.getElementById('grippy');

  if (is_sidebar_open)
  {
    // Close it
    sidebar.setAttribute('style','width: 0px');
    sidebar.setAttribute('src','about:blank');

    grippy.setAttribute('open','');

    is_sidebar_open = false;
  }
  else
  {
    // Open it
    sidebar.setAttribute('style', 'width:' + sidebar_width + 'px');
    sidebar.setAttribute('src',   sidebar_uri);

    grippy.setAttribute('open','true');

    is_sidebar_open = true;
  }  

  // Save new open/close state in prefs
  if (prefs) {
    prefs.SetBoolPref(sidebar_pref + '.open', is_sidebar_open);
  }
}
