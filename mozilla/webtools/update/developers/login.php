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

require_once('../core/init.php');
require_once('./core/sessionconfig.php');

$password = md5($_POST[password]);
$email = escape_string($_POST["email"]);
$sql = "SELECT DISTINCT `UserID`, `UserEmail`,`UserName`,`UserMode`,`UserTrusted` FROM `userprofiles` WHERE `UserEmail` = '$email' && `UserPass` = '$password' LIMIT 1";
$sql_result = mysql_query($sql, $connection) or trigger_error("<FONT COLOR=\"#FF0000\"><B>MySQL Error ".mysql_errno().": ".mysql_error()."</B></FONT>", E_USER_NOTICE);
  $num = mysql_num_rows($sql_result);

if ($num == 1) {

$row = mysql_fetch_array($sql_result);

if ($row['UserMode'] != 'A' && $row['UserMode'] != 'E')
{
   $return_path ="";
   header('Location: https://'.HOST_NAME.WEB_PATH.'/developers/index.php');
   exit;
}

if ($row['UserMode'] == 'D')
{
	$return_path ="";
	header('Location: https://'.HOST_NAME.WEB_PATH.'/developers/index.php?login=unconfirmed');
	exit;
}
$userid=$row["UserID"];
$useremail=$row["UserEmail"];
$username=$row["UserName"];
$usermode=$row["UserMode"];
$usertrusted=$row["UserTrusted"];

$logoncheck="YES";

//Update LastLogin Time to current.
$sql = "UPDATE `userprofiles` SET `UserLastLogin`=NOW(NULL) WHERE `UserID`='$userid' LIMIT 1";
  $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);

//User Role to Session Level variable...
$rolearray = array("A"=>"admin", "E"=>"editor","U"=>"user");
$usermode = $rolearray[$usermode];

//Register Session Variables
$_SESSION["uid"] = "$userid";
$_SESSION["email"] = "$useremail";
$_SESSION["name"] = "$username";
$_SESSION["level"] = "$usermode";
$_SESSION["trusted"] = "$usertrusted";
$_SESSION["logoncheck"] = "$logoncheck";

header('Location: https://'.HOST_NAME.WEB_PATH.'/developers/main.php');
exit;


} else {
$return_path ="";
header('Location: https://'.HOST_NAME.WEB_PATH.'/developers/index.php?login=failed');
exit;
}
?>
