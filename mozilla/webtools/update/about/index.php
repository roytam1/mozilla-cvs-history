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

require_once('../core/init.php');
$page_title = 'Mozilla Update :: Frequently Asked Questions';
require_once(HEADER);
?>

<div id="mBody">

<h1>Frequently Asked Questions</h1>

<h2>How do I get involved?</h2>
<p class="first">We are looking for volunteers to help us with UMO. We are in need of PHP
developers to help with redesigning the site, and people to review extensions
and themes that get submitted to UMO. We especially need Mac and Thunderbird
users. If you are interested in being a part of this exciting project, please
join us in <kbd>#umo</kbd> on <kbd>irc.mozilla.org</kbd> to start getting a feeling for what's up or for a more informal chat.
</p>

<h2>What is Mozilla Update?</h2>
<p>Mozilla Update is the place to get updates and extras for
your <a href="http://www.mozilla.org/">Mozilla</a> products.  This service
has undergone <a href="./update.php">several changes</a> that we hope
will make the site better.  We have re-enabled access to the developers area
and look forward to serving the extension and theme developer community in the
future!  We will be posting frequent 
<a href="./update.php">status updates</a> as to our progress with the 
UMO service.  The best is yet to come!</p>

<?php
$sql = "SELECT `title`, `text` FROM  `faq` WHERE `active` = 'YES' ORDER  BY  `index` ASC, `title` ASC";
$sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);

while ($row = mysql_fetch_array($sql_result)) {
    $title = $row['title'];
    $text = nl2br($row['text']);

    echo "<h2>$title</h2>\n";
    echo "<p>$text</p>\n";
}
?>
</div>

<?php
require_once(FOOTER);
?>
