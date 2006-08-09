<?php
require_once('../core/init.php');
require_once('./core/sessionconfig.php');
$item = escape_string($_GET['id']);
$page_title = 'Mozilla Update :: Developer Control Panel :: Item History';
require_once(HEADER);
require_once('./inc_sidebar.php');

if ($_SESSION["level"]=="admin" or $_SESSION["level"]=="editor") {
    //Do Nothing, they're good. :-)
} else {
    echo"<h1>Access Denied</h1>\n";
    echo"You do not have access to the Item History.";
    require_once(FOOTER);
    exit;
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
    <td><a href="mailto:<?=$row["UserEmail"]?>"><?=$row["UserName"]?></a></td>
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
