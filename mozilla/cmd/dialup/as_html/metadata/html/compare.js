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
<!--  to hide script contents from old browsers

var				ispRadio = null;
//var				out = java.lang.System.out;

//window.captureEvents( Event.MOUSEUP| Event.MOUSEDOWN| Event.MOUSEDRAG );
function loadData()
{
	radioValue = document.layers[ "allisp" ].layers[ "level1isp" ].
					layers[ "levelDisplay" ].layers[ 0 ].
					layers[ "moreInfoButton" ].document.
					forms[ 0 ].elements[ 0 ].value;
	radioClick( radioValue );
}

function tabIndex( tab )
{
	allisp = document.layers[ "allisp" ];

	for ( i = 0; i < allisp.layers.length; i++ )
	{
		if ( tab.name == allisp.layers[ i ].name )
			return i;
	}
	return -1;
}

function syncTabs()
{
	allisp = document.layers[ "allisp" ];
	for ( i = 0; i < allisp.layers.length; i++ )
	{
		levelLayer = allisp.layers[ i ];
		controls = levelLayer.layers[ "control" ];
		showLayer = controls.layers[ "show" ];
		hideLayer = controls.layers[ "hide" ];
		displayLayer = levelLayer.layers[ "levelDisplay" ];
		
		if ( hideLayer.visibility == "show" )
		{
			hideLayer.moveAbove( showLayer );
			showLayer.visibility = "hide";
			displayLayer.visibility = "show";
		//	displayLayer.moveAbove( hideLayer );
			levelLayer.toggleState = true;
		}
		else if ( showLayer.visibility == "show" )
		{
			showLayer.moveAbove( hideLayer );
			hideLayer.visibility = "hide";
			displayLayer.visibility = "hide";
		//	displayLayer.moveBelow( hideLayer );
			levelLayer.toggleState = false;
		}
	}
}

function toggleTab( tab )
{
//	out.println( "toggleTab" );
	allisp = document.layers[ "allisp" ];
	
	for ( i = tabIndex( tab ); i < allisp.layers.length; i++ )
	{
		levelLayer = allisp.layers[ i ];
		displayLayer = levelLayer.layers[ "levelDisplay" ];
		controls = levelLayer.layers[ "control" ];
		showLayer = controls.layers[ "show" ];
		hideLayer = controls.layers[ "hide" ];

//		out.println( "pageY: " + levelLayer.pageY );
		
		if ( tab == levelLayer )
		{
			// toggleState == false is closed 
			if ( levelLayer.toggleState == false )
			{
//				out.println( "OPENING" );
				// open "tab"
				showLayer.visibility = "hide";
				hideLayer.visibility = "show";
				hideLayer.moveAbove( showLayer );
				levelLayer.toggleState = true;
				moveBy = levelLayer.clip.height - controls.clip.height;
				displayLayer.visibility = "show";
			}
			else
			{
//				out.println( "CLOSING" );
				// close "tab"
				hideLayer.visibility = "hide";
				showLayer.visibility = "show";
				showLayer.moveAbove( hideLayer );
				levelLayer.toggleState = false;
				moveBy = -displayLayer.clip.height;
				displayLayer.visibility = "hide";
			}		
				
		}
		else
			levelLayer.moveBy( 0, moveBy );
	}
	levelLayer = allisp.layers[ allisp.layers.length - 1 ];
	displayLayer = levelLayer.layers[ "levelDisplay" ];
	controls = levelLayer.layers[ "control" ];
	
	if ( levelLayer.toggleState == false )
	{
		displayLayer.visibility = "hide";
		allisp.resizeBy( 0, -displayLayer.clip.height );
	}
	else
	{
		displayLayer.visibility = "show";
		allisp.resizeBy( 0, displayLayer.clip.height );
	}
}

function toggle( tabName )
{
	allisp = document.layers[ "allisp" ];
	toggleTab( allisp.layers[ tabName ] );
}

function radioClick( radioValue )
{
//	out.println( "radioClick: " + radioValue );
	
	if ( radioValue != ispRadio )
	{
		allisp = document.layers[ "allisp" ];
		for ( i = 0; i < allisp.layers.length; i++ )
		{
			levellayer = allisp.layers[ i ];
			displaylayer = levellayer.layers[ "levelDisplay" ];
			for ( j = 0; j < displaylayer.layers.length; j++ )
			{
				isplayer = displaylayer.layers[ j ];
				buttonlayer = isplayer.layers[ "moreInfoButton" ];
				form = buttonlayer.document.forms[ 0 ];
				radio = form.elements[ 0 ];
				if ( radio.value == radioValue )
					radio.checked=true;
				else
					form.reset();
			}
		}
		ispRadio = radioValue;
	}
}		

	
// end hiding contents from old browsers  -->
