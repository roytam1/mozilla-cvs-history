<?php
require_once('../core/init.php');
require_once('./core/sessionconfig.php');
$page_title = 'Mozilla Update :: Developer Control Panel :: Overview';
require_once(HEADER);
require_once('./inc_sidebar.php');
?>
<h2>Welcome <?php echo"$_SESSION[name]";?>!</h2>


<?php
$sql ="SELECT TM.ID FROM `main` TM
INNER JOIN `version` TV ON TM.ID = TV.ID
WHERE `approved` = '?' GROUP BY `URI` ORDER BY TV.DateUpdated ASC";
 $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
  $num_result = mysql_num_rows($sql_result);
?>
<p>
<strong>Approval Queue Status: There are currently <?php echo"$num_result"; ?> add-ons awaiting review</strong>
</p>

<h3>My Extensions</h3>

<?php
$sql = "SELECT  TM.ID, TM.Type, TM.Name, TM.Description, TM.downloadcount, TM.TotalDownloads, TM.Rating, TU.UserEmail FROM  `main`  TM 
LEFT JOIN authorxref TAX ON TM.ID = TAX.ID
INNER JOIN userprofiles TU ON TAX.UserID = TU.UserID
WHERE TU.UserID = '$_SESSION[uid]' AND TM.Type ='E'
ORDER  BY  `Type` , `Name` ";
 $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
  $numresults = mysql_num_rows($sql_result);
  while ($row = mysql_fetch_array($sql_result)) {

$v++;
    $id = $row["ID"];
    $type = $row["Type"];
    $name = $row["Name"];
    $dateadded = $row["DateAdded"];
    $dateupdated = $row["DateUpdated"];
    $homepage = $row["Homepage"];
    $description = nl2br($row["Description"]);
    $authors = $row["UserEmail"];
    $downloadcount = $row["downloadcount"];
    $totaldownloads = $row["TotalDownloads"];
    $rating = $row["Rating"];

echo"<h4><A HREF=\"./itemoverview.php?id=$id\">$name</A></h4>\n";
echo"<p>$description</p>\n";
//Icon Bar
echo"<DIV style=\"margin-top: 10px; height: 34px\">";
echo"<DIV class=\"iconbar\"><A HREF=\"./itemoverview.php?id=$id\"><IMG SRC=\"/images/edit.png\" HEIGHT=\"34\" WIDTH=\"34\" ALT=\"\">&nbsp;Edit Item</A></DIV>";
echo"<DIV class=\"iconbar\"><IMG SRC=\"/images/download.png\" HEIGHT=\"34\" WIDTH=\"34\" ALT=\"\">Downloads: $downloadcount<BR>&nbsp;&nbsp;$totaldownloads total</DIV>";
echo"<DIV class=\"iconbar\" title=\"$rating of 5 stars\"><A HREF=\"../extensions/moreinfo.php?id=$id&amp;page=comments\"><IMG SRC=\"/images/ratings.png\" HEIGHT=\"34\" WIDTH=\"34\" ALT=\"\">Rated<br>&nbsp;&nbsp;$rating of 5</A></DIV>";
echo"</DIV>";

}
?>
<p>
&nbsp;&nbsp;&nbsp;&nbsp;<a href="additem.php?type=E">Add New Extension...</a>
</p>


<h3>My Themes</h3>

<?php
$sql = "SELECT  TM.ID, TM.Type, TM.Name, TM.DateAdded, TM.Description, TM.downloadcount, TM.TotalDownloads, TM.Rating, TU.UserEmail FROM  `main`  TM 
LEFT JOIN authorxref TAX ON TM.ID = TAX.ID
INNER JOIN userprofiles TU ON TAX.UserID = TU.UserID
WHERE TU.UserID = '$_SESSION[uid]' AND TM.Type ='T'
ORDER  BY  `Type` , `Name` ";
 $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
  $numresults = mysql_num_rows($sql_result);
  while ($row = mysql_fetch_array($sql_result)) {
    $id = $row["ID"];
    $type = $row["Type"];
    $name = $row["Name"];
    $dateadded = $row["DateAdded"];
    $dateupdated = $row["DateUpdated"];
    $homepage = $row["Homepage"];
    $description = substr($row["Description"],0,75);
    $authors = $row["UserEmail"];
    $downloadcount = $row["downloadcount"];
    $totaldownloads = $row["TotalDownloads"];
    $rating = $row["Rating"];

echo"<h4><A HREF=\"./itemoverview.php?id=$id\">$name</A></h4>\n";
echo"<p>$description</p>\n";
//Icon Bar
echo"<DIV style=\"margin-top: 10px; height: 34px\">";
echo"<DIV class=\"iconbar\"><A HREF=\"./itemoverview.php?id=$id\"><IMG SRC=\"/images/edit.png\" HEIGHT=\"34\" WIDTH=\"34\" ALT=\"\">&nbsp;Edit Item</A></DIV>";
echo"<DIV class=\"iconbar\"><IMG SRC=\"../images/download.png\" HEIGHT=\"34\" WIDTH=\"34\" ALT=\"\">Downloads: $downloadcount<BR>&nbsp;&nbsp;$totaldownloads total</DIV>";
echo"<DIV class=\"iconbar\" title=\"$rating of 5 stars\"><A HREF=\"../themes/moreinfo.php?id=$id&amp;page=comments\"><IMG SRC=\"/images/ratings.png\" HEIGHT=\"34\" WIDTH=\"34\" ALT=\"\">Rated<br>&nbsp;&nbsp;$rating of 5</A></DIV>";
echo"</DIV>";


}
?>
<p>&nbsp;&nbsp;&nbsp;&nbsp;<a href="additem.php?type=T">Add New Theme...</a></p>

</DIV>

<!-- close #mBody-->
</div>

<?php
require_once(FOOTER);
?>
