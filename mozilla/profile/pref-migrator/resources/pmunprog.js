/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 * 
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
 * License for the specific language governing rights and limitations
 * under the License.
 * 
 * The Original Code is Mozilla Communicator client code, released March
 * 31, 1998.
 * 
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation. Portions created by Netscape are Copyright (C) 1998
 * Netscape Communications Corporation. All Rights Reserved.
 */

var migrator;
var browser;
var dialog;

function onLoad(oldProfilePath, newProfilePath) {
  //dialog = new Object;
  //dialog.title     = document.getElementByID( "xpi.process" );
  //dialog.current   = document.getElementByID( "xpi.currentlyprocessing");
  //dialog.newWindow   = document.getElementById( "dialog.newWindow" );

  var retval;
	
  var prefmigrator = Components.classes['component://netscape/profile/migration'].createInstance(Components.interfaces.nsIPrefMigration);
  if (prefmigrator) 
  {
	 dump("-----  Migrating prefs\n");
     retval = prefmigrator.ProcessPrefsFromJS();
	 dump("-----  Migrating prefs done " + retval + "\n" );
  }
  else
  {
	  dump("-----  ERROR Migrating prefs failed create instances failed\n");
  }
  return retval;
}


function open() {
   if ( dialog.ok.disabled ) {
      return;
   }

	var url = dialog.input.value;

	browser.loadUrl( url );

	/* Close dialog. */
        window.close();
}

function cancel() {
        window.close();
}

