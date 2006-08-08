<?php
require_once('../core/init.php');
require_once('./core/sessionconfig.php');
$page_title = 'Mozilla Update :: Developer Control Panel :: Item Overview';
require_once(HEADER);
require_once('./inc_sidebar.php');

//Kill access to items this user doesn't own...
if ($_SESSION["level"] !=="admin") {

    $id = escape_string($_GET["id"]);
    if (!$id) {$id = escape_string($_POST["id"]); }
    $sql = "SELECT `UserID` from `authorxref` TAX WHERE `ID` = '$id' AND `UserID` = '$_SESSION[uid]' LIMIT 1";
    $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
    if (mysql_num_rows($sql_result)=="0") {
    echo"<h1>Access Denied</h1>\n";
    echo"You do not have access to this item.";
    require_once(FOOTER);
    exit;
    }
}
?>
<?php
$id = escape_string($_GET["id"]);
$sql = "SELECT  TM.ID, TM.Type, TM.GUID, TM.Name, TM.Homepage, TM.Description, TM.downloadcount, TM.TotalDownloads, TM.Rating, TU.UserEmail FROM  `main`  TM 
LEFT JOIN authorxref TAX ON TM.ID = TAX.ID
INNER JOIN userprofiles TU ON TAX.UserID = TU.UserID
WHERE TM.ID = '$id' LIMIT 1";
 $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
  $numresults = mysql_num_rows($sql_result);
  $row = mysql_fetch_array($sql_result);
$v++;
    $id = $row["ID"];
    $type = $row["Type"];
    $guid = $row["GUID"];
    $name = $row["Name"];
    $dateadded = $row["DateAdded"];
    $dateupdated = $row["DateUpdated"];
    $homepage = $row["Homepage"];
    $description = nl2br($row["Description"]);
    $downloadcount = $row["downloadcount"];
    $totaldownloads = $row["TotalDownloads"];
    $rating = $row["Rating"];

$categories="";
$sql = "SELECT  TC.CatName FROM  `categoryxref`  TCX 
INNER JOIN categories TC ON TCX.CategoryID = TC.CategoryID
WHERE TCX.ID = '$id' GROUP BY TC.CatName";
 $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
  while ($row = mysql_fetch_array($sql_result)) {
    if($categories == "")
   $categories = $row["CatName"];
    else
   $categories .= ", ".$row["CatName"];
  }
?>
<div id="mainContent" class="right">
<h2>Item Overview :: <?php echo"$name"; ?></h2>
<?php
echo"<a href=\"listmanager.php?function=editmain&id=$id\">Edit $name</a><br>\n";
echo"$description<br>\n";
if ($guid) {echo"GUID: $guid<br>\n"; }
if ($homepage) {echo"Homepage: <a href=\"$homepage\">$homepage</a><br>\n";}
echo"Categories: $categories<br>\n";

?>

<h2>Add New Version of <?php echo"$name"; ?></h2>
<TABLE BORDER=0 CELLPADDING=2 CELLSPACING=2 ALIGN=CENTER STYLE="border: solid 0px #000000; width: 100%">
<FORM NAME="additem" METHOD="POST" ACTION="additem.php?function=additem2" enctype="multipart/form-data">
<INPUT NAME="type" TYPE="hidden" VALUE="<?php echo"$type"; ?>">
<TR><TD style="padding-left: 20px">
Your <?php echo"$typename"?> File:<BR>
<INPUT NAME="file" SIZE=40 TYPE="FILE"><BR>
<BR>
<INPUT NAME="button" TYPE="BUTTON" VALUE="Cancel" onclick="javascript:history.back()"> <INPUT NAME="submit" TYPE="SUBMIT" VALUE="Next &#187;"> 
</TD></TR>
</FORM>
</TABLE>

<h2>Listed Versions</h2>
<?php
$approved_array = array("?"=>"Pending Approval", "YES"=>"Approved", "NO"=>"Denied", "DISABLED"=>"Disabled");
$sql = "SELECT vID, TV.Version, URI, OSName, approved FROM `version` TV
INNER JOIN os TOS ON TOS.OSID = TV.OSID
WHERE `ID`='$id' GROUP BY `URI` ORDER BY `Version` DESC";
 $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
  while($row = mysql_fetch_array($sql_result)) {
    $vid = $row["vID"];
    $version = $row["Version"];
    $uri = $row["URI"];
    $filename = basename($row["URI"]);
    $os = $row["OSName"];
    $approved = $row["approved"];
    $approved = $approved_array["$approved"];

echo"<h4><a href=\"listmanager.php?function=editversion&id=$id&vid=$vid\">Version $version</a> - $approved (<small>$filename</small>)</h4>\n";

if ($os !="ALL") {echo" - for $os"; }
 }
?>


	</div>
    <div id="side" class="right">
    <h2>Statistics</h2>
        <img src="/images/download.png" border=0 height=32 width=32 alt="" class="iconbar">Downloads this Week: <?php echo"$downloadcount"; ?><br>
        Total Downloads: <?php echo"$totaldownloads"; ?><BR>
    <BR>
        <img src="/images/ratings.png" border=0 height=34 width=34 alt="" class="iconbar">Rated: <?php echo"$rating"; ?> of 5<BR>&nbsp;<br>
    <BR>
<?php

$sql = "SELECT CommentID FROM  `feedback` WHERE ID = '$id'";
$sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
    $num_comments = mysql_num_rows($sql_result);
?>
        <img src="/images/edit.png" border=0 height=34 width=34 alt="" class="iconbar"><a href="commentsmanager.php?id=<?php echo"$id"; ?>">Comments: <?php echo"$num_comments"; ?></a><BR>&nbsp;<br>

    <h2>Developer Comments</h2>
<?php
if ($_POST["submit"]=="Post Comments") {
  $id = escape_string($_POST["id"]);
  $comments = escape_string($_POST["comments"]);
  if (checkFormKey()) {
    $sql = "UPDATE `main` SET `devcomments`='$comments' WHERE `id`='$id'";
    $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
    if ($sql_result) { echo"Developer Comments Updated...<br>\n"; }
  }
}

$sql = "SELECT `devcomments` FROM `main` WHERE `id`='$id' LIMIT 1";
 $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
  $row = mysql_fetch_array($sql_result);
   $devcomments = $row["devcomments"];
 
?>
<form name="devcomments" method="post" action="itemoverview.php?id=<?php echo"$id"; ?>">
<?writeFormKey();?>
<input name="id" type="hidden" value="<?php echo"$id"; ?>">
<textarea name="comments" rows=10 cols=26><?php echo"$devcomments"; ?></textarea><br>
<input name="submit" type="submit" value="Post Comments">&nbsp;<input name="reset" type="reset" value="Reset">
</form>

    <h2><a href="previews.php?id=<?php echo"$id"; ?>">Previews</a></h2>
<?php
$sql = "SELECT * FROM `previews` TP WHERE `ID`='$id' AND `preview`='YES' ORDER BY `PreviewID` LIMIT 1";
 $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
  while ($row = mysql_fetch_array($sql_result)) {
    $i++;
    $previewid = $row["PreviewID"];
    $uri = $row["PreviewURI"];
    $filename = basename($row["PreviewURI"]);
    $filename_array[$i] = $filename;
    $caption = $row["caption"];
    $preview = $row["preview"];
    list($src_width, $src_height, $type, $attr) = getimagesize(FILE_PATH.'/'.$uri);

    echo"<a href=\"previews.php?id=$id\"><img src=\"$uri\" border=0 $attr alt=\"$caption\"></a><br>$caption\n";
   }
   if (mysql_num_rows($sql_result)=="0") {echo"<a href=\"previews.php?id=$id\">Add a Preview</a>...<br>\n"; }
?>



    </div>


<!-- close #mBody-->
</div>

<?php
require_once(FOOTER);
?>
