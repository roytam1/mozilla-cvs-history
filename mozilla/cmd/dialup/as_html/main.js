/* -*- Mode: Java; tab-width: 4; indent-tabs-mode: nil -*-
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
 * Contributor(s): 
 */
var captureString="";
var	thePath = "";

function go( msg )
{
	if ( msg == thePath )
		return checkData();
	return false;
}



function doGo()
{
	parent.controls.go( "Next" );
}



function setPath( msg )
{
	thePath = msg;
	setTimeout( "doGo()", 1 );
}



function checkData()
{
	return true;
}

function doAbout()
{
	parent.controls.go( "About" );
}

function captureKeys( e )
{
	captureString = captureString + String.fromCharCode( e.which );
	if ( captureString.length > 5 )
		captureString = captureString.substring( captureString.length - 5,captureString.length );
	if ( captureString == "about" )
	{
		captureString="";
		setTimeout( "doAbout()", 1 );
	}
	return true;
}

function loadData()
{
	var cleanFlag = false;
	var configuredFlag = false;
	var newProfileFlag = false;
	var configFilename = "";

	netscape.security.PrivilegeManager.enablePrivilege( "AccountSetup" );

	var acctSetupFile = parent.parent.globals.getAcctSetupFilename( self );
	var editMode = parent.parent.globals.document.vars.editMode.value;
	if ( editMode != null && editMode != "" )
		editMode = editMode.toLowerCase();

	if ( editMode != "yes" )
	{
		document.onkeydown = captureKeys;
	
		// get platform
	
		var thePlatform = parent.parent.globals.getPlatform();
	
		//var profileDir = parent.parent.globals.document.setupPlugin.GetCurrentProfileDirectory();
		var profileDir = parent.parent.globals.currentProfileDirectory;
		parent.parent.globals.debug("****Priofile Dir = " + profileDir);
		if ( profileDir != null && profileDir != "" )
		{
			if ( thePlatform == "Macintosh" )				// Macintosh support
				configFilename = profileDir + "Configuration";
			else										// Windows support
				configFilename = profileDir + "CONFIG.INI";
	
			// if a MUC file exists:  re-using Account Setup against a profile that's been configured
			// else, if "account_setup.upgraded" is true, but no MUC:  upgrade from 3.x or earlier
	
			var accountName = parent.parent.globals.document.setupPlugin.GetNameValuePair( configFilename, "Account", "Account" );
			if ( accountName != null && accountName != "" && accountName != "Untitled" && accountName != "None" )
				configuredFlag = true;
			else
			{
				var profileName = parent.parent.globals.document.setupPlugin.GetCurrentProfileName();
				if ( profileName != null )
				{
					profileName = profileName.toUpperCase();
					if ( profileName == '911' || profileName == 'USER1' )
						cleanFlag=true;
					else
						newProfileFlag=true;
				}
			}
		}
	
		// ensure that some descriptive layer is shown
	
		if ( cleanFlag == false && configuredFlag == false && newProfileFlag == false )
			cleanFlag = true;
	
		if ( document && document.layers )
		{
			document.layers[ "Clean_Installation" ].visibility = ( ( cleanFlag == true ) ? "show" : "hide" );
			document.layers[ "Profile_Configured" ].visibility = ( ( configuredFlag == true ) ? "show" : "hide" );
			document.layers[ "Profile_Manager_Entrance" ].visibility = ( ( newProfileFlag == true ) ? "show" : "hide" );			
		}
	
		parent.parent.globals.loadUserInput();
	}
	
	
	if ( document && document.layers )
	{
		document.layers[ "buttontext" ].visibility = "show";
			
		var showIntroScreens = parent.parent.globals.document.setupPlugin.GetNameValuePair( acctSetupFile, "Mode Selection", "Show_Intro_Screens" );
		document.layers[ "Intro_Screens" ].visibility = ( ( showIntroScreens == "yes" ) ? "show" : "hide" );
	}

	if ( parent && parent.controls && parent.controls.generateControls )
		parent.controls.generateControls();
}



function saveData()
{
}
