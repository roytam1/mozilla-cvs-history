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

function go( msg )
{
	//parent.parent.globals.debug( "compwrap go" );
	
	if ( compare.ispRadio != null && compare.ispRadio != "" )
	{
		parent.parent.globals.debug( compare.ispRadio );
		parent.parent.globals.selectedISP = compare.ispRadio;
		return true;
	}
	alert( "You haven't selected an ISP" );
	return false;
}

function checkData()
{
	return true;
}

// end hiding contents from old browers -->
