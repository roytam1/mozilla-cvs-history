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
//   Mike Morgan <morgamic@gmail.com>
//   Justin Scott <fligtar@gmail.com>
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

require_once('inc_tshirt.php');

if (($_SESSION["level"] == "admin" or $_SESSION["level"] == "editor")) {
    $sql ="SELECT TM.ID FROM `main` TM INNER JOIN `version` TV ON TM.ID = TV.ID  WHERE `approved` = '?' GROUP BY `URI`";
    $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
        $queuenum = mysql_num_rows($sql_result);

    $sql = "SELECT `CommentID` FROM `feedback` WHERE `flag`='YES'";
    $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
        $commentsnum = mysql_num_rows($sql_result);
}
?>
<div id="mBody">

<div id="side">
<ul id="nav">
<li><A HREF="<?=WEB_PATH?>/developers/main.php">Overview</A></li>
<?php

if ($_SESSION['level'] == 'user') {
?>
<li><A HREF="<?=WEB_PATH?>/developers/usermanager.php">Your Profile</A></li>

<?php
}

if (tshirtEligible()) {
?>

<li><A HREF="<?=WEB_PATH?>/developers/tshirt.php">T-Shirt Request</A></li>

<?php
}

if ($_SESSION['level'] == 'editor') {
?>
<li><A HREF="<?=WEB_PATH?>/developers/usermanager.php?function=edituser&amp;userid=<?php echo"$_SESSION[uid]"; ?>">Your Profile</A></li>
<li><A HREF="<?=WEB_PATH?>/developers/approval.php">Approval Queue <?=$queuenum?></A></li>
<li><a href="<?=WEB_PATH?>/developers/commentsmanager.php?function=flaggedcomments">Comments Manager <?=$commentsnum?></a></li>
<?php
} 

if ($_SESSION['level'] == 'admin') {
?>
<li><A HREF="<?=WEB_PATH?>/developers/usermanager.php?function=edituser&amp;userid=<?php echo"$_SESSION[uid]"; ?>">Your Profile</A></li>
<li><A HREF="<?=WEB_PATH?>/developers/approval.php">Approval Queue <?="($queuenum)"?></A></li>
<li><A HREF="<?=WEB_PATH?>/developers/listmanager.php?type=T">Themes list</A></li>
<li><A HREF="<?=WEB_PATH?>/developers/listmanager.php?type=E">Extensions list</A></li>
<li><A HREF="<?=WEB_PATH?>/developers/usermanager.php">Users Manager</A></li>
<li><a href="<?=WEB_PATH?>/developers/appmanager.php">Application Manager</a></li>
<li><a href="<?=WEB_PATH?>/developers/categorymanager.php">Category Manager</A></li>
<li><a href="<?=WEB_PATH?>/developers/faqmanager.php">FAQ Manager</A></li>
<li><a href="<?=WEB_PATH?>/developers/commentsmanager.php?function=flaggedcomments">Comments Manager <?="($commentsnum)"?></a></li>
<li><a href="<?=WEB_PATH?>/developers/reviewsmanager.php">Reviews Manager</a></li>
<?php } ?>
<li><a href="logout.php">Logout</A></li>
</ul>

	</div>
	<div id="mainContent">
