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
$item = escape_string($_GET['id']);
$page_title = 'Mozilla Update :: Developer Control Panel :: Item History';
require_once(HEADER);
require_once('./inc_sidebar.php');

//Kill access to items this user doesn't own...
if ($_SESSION["level"] !== "admin" && $_SESSION["level"] !== "editor") {
    $id = escape_string($_GET["id"] != "" ? $_GET["id"] : $_POST["id"]);
    $sql = "SELECT `UserID` from `authorxref` TAX WHERE `ID` = '$id' AND `UserID` = '$_SESSION[uid]' LIMIT 1";
    $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
    if (mysql_num_rows($sql_result) == 0) {
        echo "<h1>Access Denied</h1>\n";
        echo "You do not have access to this item.";
        require_once(FOOTER);
        exit;
    }
}


$sql = "SELECT `Name` FROM `main` WHERE `ID`={$item}";
$sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
$row = mysql_fetch_array($sql_result);

?>
<style type="text/css">
TD { font-size: 8pt }
</style>
<h1>Item History :: <?=$row["Name"]?></h1><br>
<table border=0 cellpadding=2 cellspacing=0 width="100%" align="center">
<tr style="background-color:#ccc;font-weight:bold;">
<td style="font-size: 8pt">ID</td>
<td style="font-size: 8pt">Version</td>
<td style="font-size: 8pt">Date</td>
<td style="font-size: 8pt">Reviewer</td>
<td style="font-size: 8pt">Action</td>
<td style="font-size: 8pt">Comments</td>
</tr>
<?php

$sql ="SELECT TV.Version, TU.UserName, TU.UserEmail, `action`, `date`, `comments` FROM `approvallog` TA
INNER JOIN `version` TV ON TA.vID=TV.vID
INNER JOIN `userprofiles` TU ON TA.UserID=TU.UserID
WHERE TA.ID={$item} AND `action`!='Approval?' GROUP BY `date` ORDER BY `date` DESC";

$sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
 $num_results = mysql_num_rows($sql_result);
 if($num_results > 0) { 
  while ($row = mysql_fetch_array($sql_result)) {
    $i++;

    $class = ($i % 2) ? 'rowa' : 'rowb';
?>
<tr class="<?=$class?>">
    <td><?=$i?></td>
    <td><?=$row["Version"]?></td>
    <td><?=$row["date"]?></td>
<?
    if($_SESSION["level"] == "admin" || $_SESSION["level"] == "editor") {
        echo "<td><a href=\"mailto:".$row["UserEmail"]."\">".$row["UserName"]."</a></td>";
    } else {
        echo "<td>".$row["UserName"]."</td>";
    }
?>
    <td><?=(($row["action"] == "Approval+") ? "Approved" : "Denied")?></td>
    <td><?=$row["comments"]?></td>
</tr>
<?
  }
 } 
 else {
?>
<tr><td colspan="6" align="center">No review entries could be found.</td></tr>
<?
 }
?>
</table>


<!-- close #mBody-->
</div>

<?php
require_once(FOOTER);
?>
