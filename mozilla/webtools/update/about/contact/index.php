<?php
// ***** BEGIN LICENSE BLOCK *****
// Version: MPL 1.1/GPL 2.0/LGPL 2.1
//
// The contents of this file are subject to the Mozilla Public License Version
// 1.1 (the "License"); you may not use this file except in compliance with
// the License. You may obtain a copy of the License at
// http://www.mozilla.org/MPL/
//
// Software distributed under the License is distributed on an "AS IS" basis,
// WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
// for the specific language governing rights and limitations under the
// License.
//
// The Original Code is Mozilla Update.
//
// The Initial Developer of the Original Code is
// Chris "Wolf" Crews.
// Portions created by the Initial Developer are Copyright (C) 2004
// the Initial Developer. All Rights Reserved.
//
// Contributor(s):
//   Chris "Wolf" Crews <psychoticwolf@carolina.rr.com>
//
// Alternatively, the contents of this file may be used under the terms of
// either the GNU General Public License Version 2 or later (the "GPL"), or
// the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
// in which case the provisions of the GPL or the LGPL are applicable instead
// of those above. If you wish to allow use of your version of this file only
// under the terms of either the GPL or the LGPL, and not to allow others to
// use your version of this file under the terms of the MPL, indicate your
// decision by deleting the provisions above and replace them with the notice
// and other provisions required by the GPL or the LGPL. If you do not delete
// the provisions above, a recipient may use your version of this file under
// the terms of any one of the MPL, the GPL or the LGPL.
//
// ***** END LICENSE BLOCK *****
?>
<?php

// Set this page to read from the SHADOW_DB.
define('USE_SHADOW_DB',true);

require_once('../../core/init.php');
$page_title = 'Mozilla Update :: Contact Information';
require_once(HEADER);
?>

<div id="mBody">

<h1>Contact Mozilla Update</h1>
<p>Are you looking for contact information for the Mozilla Update staff to report a bug?  
Mozilla Update bugs should be filed in Bugzilla, the mozilla organizations bug tracking tool.
For more information, visit our <a href="../#bugs">frequently asked
questions</a>.</p>

<h2>IRC</h2>
<p>For support questions, please visit <a href="irc://irc.mozilla.org/firefox">#firefox</a> 
and <a href="irc://irc.mozilla.org/thunderbird">#thunderbird</a>.</p>

<p>Some of the Mozilla Update staff can be found on IRC. In the 
<a href="irc://irc.mozilla.org/umo">#umo</a> channel on 
<a href="irc://irc.mozilla.org">irc.mozilla.org</a>. Note: this channel is not
for end-user support.</p>

</div>
<?php
require_once(FOOTER);
?>
