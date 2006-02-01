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
//   Chris "Wolf" Crews <psychoticwolf@carolina.rr.com> (not)
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
	
// Set this page to read from the SHADOW_DB.
define('USE_SHADOW_DB',true);

require_once('./core/init.php');

$page_title = 'Support :: Mozilla Update';
require_once(HEADER);
?>

<div id="mBody">
<h1>Support</h1>

<p>This is a list of different ways to get support for Mozilla Add-ons, which includes Themes and Extensions.</p>

<dl>
<dt><a href="http://forums.mozillazine.org/">MozillaZine Forums</a></dt>
<dd>People in the MozillaZine forums are often helpful and know a lot about Mozilla Add-ons.</dd>

<dt>IRC Chat</dt>
<dd>IRC is probably the best way to find support.  You should find people who can help in <b>#umo</b> or <b>#firefox</b> on <b>irc.mozilla.org</b>.  If you don't have an IRC client, you can use <a href="<?=WEB_PATH?>/extensions/moreinfo.php?id=16&application=firefox">ChatZilla</a>.</dd>

<dt><a href="http://developer.mozilla.org/en/docs/Extensions">developer.mozilla.org (DevMo) Extensions Page</a></dt>
<dd>Extensions developers can look for documentation on how to create their own extensions here.</dd>

<dt><a href="http://developer.mozilla.org/en/docs/Themes">DevMo Themes Page</a></dt>
<dd>Theme designers interesting in making themes for Mozilla applications should read this document.</dd>

<dt><a href="http://mozilla.org/support/">Mozilla Product Support</a></dt>
<dd>For support on one of the Mozilla applications themeselves, please visit this page.</dd>

<dt><a href="https://bugzilla.mozilla.org/enter_bug.cgi?product=Update">File a Bug in Bugzilla</a></dt>
<dd>To file a bug regarding an major error in an extension or theme, or to report a problem with this website, please file a bug in Bugzilla to make sure it gets tracked properly.</dd>
</dl>

</div>

<?php
require_once(FOOTER);
?>
