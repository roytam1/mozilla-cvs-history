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
require"../core/config.php";
?>
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd">
<html lang="en">

<head>

<?php
//----------------------------
//Global $_GET variables
//----------------------------
if ($_GET["numpg"]) {$items_per_page = escape_string($_GET["numpg"]); } else {$items_per_page="10";}//Default Num per Page is 10
if ($_GET["category"]) { $category = escape_string($_GET["category"]); }
if ($category=="All") {$category="";}


if (!$_GET["pageid"]) {$pageid="1"; } else { $pageid = escape_string($_GET["pageid"]); } //Default PageID is 1
$type="T"; //Default Type is T

unset($typename);
$types = array("E"=>"Extensions","T"=>"Themes","U"=>"Updates");
$typename = $types["$type"];

//RSS Autodiscovery Link Stuff
switch ($_SESSION["category"]) {
  case "Newest":
    $rsslist = "newest";
    break;
  case "Popular":
    $rsslist = "popular";
    break;
  case "Top Rated":
    $rsslist = "rated";
    break;
}

$rssfeed = "rss/?application=" . $application . "&type=" . $type . "&list=" . $rsslist;

if (!$category) {$categoryname = "All $typename"; } else {$categoryname = $category; }
?>

<TITLE>Mozilla Update :: Themes - List - <?php echo"$categoryname"; if ($pageid) {echo" - Page $pageid"; } ?></TITLE>

<?php
if ($rsslist) {
echo"<link rel=\"alternate\" type=\"application/rss+xml\" title=\"RSS\" href=\"http://$_SERVER[HTTP_HOST]/$rssfeed\">";
}
?>

<?php
installtrigger("themes");

include"$page_header";

?>
<div id="mBody">
    <?php
    include"inc_sidebar.php";
    ?>

	<div id="mainContent">

<?php

//Query for List Creation
$s = "0";
$startpoint = ($pageid-1)*$items_per_page;
if ($category=="Editors Pick" or $category=="Newest" or $category=="Popular" or $category=="Top Rated") {
if ($category =="Editors Pick") {
$editorpick="true";
} else if ($category =="Newest") {
$orderby = "TV.DateAdded DESC, `Name` ASC";
} else if ($category =="Popular") {
$orderby = "TM.downloadcount DESC, `Name` ASC";
} else if ($category =="Top Rated") {
$orderby = "TM.Rating DESC, `Name` ASC";
}
$catname = $category;
$category = "%";
}

$sql = "SELECT TM.ID, TM.Name, TM.DateAdded, TM.DateUpdated, TM.Homepage, TM.Description, TM.Rating, TM.TotalDownloads, TM.downloadcount, TV.vID,
SUBSTRING(MAX(CONCAT(LPAD(TV.Version, 6, '0'), TV.vID)), 7)  AS MAXvID,
MAX(TV.Version) AS Version,
TA.AppName, TOS.OSName
FROM `main` TM 
INNER  JOIN version TV ON TM.ID = TV.ID
INNER  JOIN applications TA ON TV.AppID = TA.AppID
INNER  JOIN os TOS ON TV.OSID = TOS.OSID ";
if ($category && $category !=="%") { $sql .="INNER  JOIN categoryxref TCX ON TM.ID = TCX.ID
INNER JOIN categories TC ON TCX.CategoryID = TC.CategoryID "; }
if ($editorpick=="true") { $sql .="INNER JOIN reviews TR ON TM.ID = TR.ID "; }
$sql .="WHERE Type = '$type' AND AppName = '$application' AND `approved` = 'YES' ";
if ($editorpick=="true") { $sql .="AND TR.Pick = 'YES' "; }
if ($category && $category !=="%") {$sql .="AND CatName LIKE '$category' ";}
if ($app_version) { $sql .=" AND TV.MinAppVer_int <= '".strtolower($app_version)."' AND TV.MaxAppVer_int >= '".strtolower($app_version)."' ";}
if ($OS) { $sql .=" AND (TOS.OSName = '$OS' OR TOS.OSName = 'All') "; }
if ($catname == "Popular") { $sql .=" AND TM.downloadcount > '5'"; }
$sql .="GROUP BY `Name` ";
if ($orderby) {
$sql .="ORDER BY $orderby";
} else {
$sql .="ORDER  BY  `Name` , `Version` DESC ";
}

$resultsquery = $sql;
unset($sql);

//Get Total Results from Result Query & Populate Page Control Vars.
 $sql_result = mysql_query($resultsquery, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
  $totalresults = mysql_num_rows($sql_result);

  $num_pages = ceil($totalresults/$items_per_page); //Total # of Pages
  if ($pageid>$num_pages) {$pageid=$num_pages;} //Check PageId for Validity
  $startpoint = ($pageid-1)*$items_per_page;
if ($startpoint<0) {$startpoint=0; $startitem=0;}
  $startitem = $startpoint+1;
  $enditem = $startpoint+$items_per_page;
 if ($totalresults=="0") {$startitem = "0"; }
 if ($enditem>$totalresults) {$enditem=$totalresults;} //Verify EndItem


if ($_GET[nextnum]) {$startpoint = $_GET["nextnum"]; }
//$resultsquery = str_replace("GROUP BY `Name` ", "", $resultsquery);
$resultsquery .= " LIMIT $startpoint , $items_per_page"; //Append LIMIT clause to result query

if ($category=="%") {$category = $catname; unset($catname); }

//Now Showing Box
if (!$category) {$categoryname="All"; } else {$categoryname = $category;}
echo"<H2>".ucwords("$application $typename &#187; $categoryname ")."</H2>\n";

if (!$OS) {$OS="all";}
if (!$category) {$categoryname="All"; } else {$categoryname = $category;}

echo"".ucwords("$typename")." $startitem - $enditem of $totalresults&nbsp;&nbsp;|&nbsp;&nbsp;";

$previd=$pageid-1;
if ($previd >"0") {
echo"<a href=\"?".uriparams()."&amp;pageid=$previd\">&#171; Previous</A> &bull; ";
}
echo"Page $pageid of $num_pages";

$nextid=$pageid+1;
if ($pageid <$num_pages) {
echo" &bull; <a href=\"?".uriparams()."&amp;pageid=$nextid\">Next &#187;</a>";
}

echo"<br>\n";

$sql = "SELECT `CatDesc` FROM `categories` WHERE `CatName`='$category' and `CatType`='$type' LIMIT 1";
 $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
   $row = mysql_fetch_array($sql_result);
     $categorydescription = $row["CatDesc"];
if ($category=="All") {$categorydescription="All listed themes for $application";}

if ($categorydescription) {echo"$categorydescription<br>\n";}



// Modify List Form

echo"<DIV class=\"key-point\">";
echo"<FORM NAME=\"listviews\" METHOD=\"GET\" ACTION=\"showlist.php\">\n";
echo"<div>";
echo"<INPUT NAME=\"application\" TYPE=\"hidden\" VALUE=\"$application\">\n";
echo"<INPUT NAME=\"category\" TYPE=\"hidden\" VALUE=\"$category\">\n";
//Items-Per-Page
echo"Show/Page: ";
$perpage = array("5","10","20","50");
echo"<SELECT name=\"numpg\">";
foreach ($perpage as $value) {
echo"<OPTION value=\"$value\"";
if ($items_per_page==$value) {echo" SELECTED"; }
echo">$value</OPTION>";
}
echo"</SELECT>\n";


// Operating Systems
echo" OS: ";
echo"<SELECT name=\"os\">\n";
$sql = "SELECT `OSName` FROM `os` ORDER BY `OSName`";
 $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
  while ($row = mysql_fetch_array($sql_result)) {
  $osname = $row["OSName"];
  echo"<OPTION value=\"".strtolower($osname)."\"";
  if (strtolower($OS) == strtolower($osname)) {echo" SELECTED";}
  echo">$osname</OPTION>";
  }
echo"</SELECT>\n";


//Versions of Application
echo"Versions: ";
echo"<SELECT name=\"version\">";
if ($application != "thunderbird") {echo"<OPTION value=\"auto-detect\">Auto-Detect</OPTION>";}
$app_orig = $application; //Store original to protect against possible corruption
$sql = "SELECT `Version`, `major`, `minor`, `release`, `SubVer` FROM `applications` WHERE `AppName` = '$application' AND `public_ver` = 'YES'  ORDER BY `major` DESC, `minor` DESC, `release` DESC, `SubVer` DESC";
 $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
  while ($row = mysql_fetch_array($sql_result)) {
  $version = $row["Version"];
  $subver = $row["SubVer"];
  $release = "$row[major].$row[minor]";
  if ($row["release"]) {$release = ".$release$row[release]";}
if ($app_version=="0.95") {$app_version="0.10"; }
//Firesomething Support
if ($application=="firefox") { if ($release == "0.7") {$application="firebird";} else {$application="firefox";} }

if ($subver !=="final") {$release="$release$subver";}
echo"<OPTION value=\"$release\"";
if ($app_version == $release) {echo" SELECTED"; }
echo">".ucwords($application)." $version</OPTION>";

if ($app_version=="0.10") {$app_version="0.95"; }
  }
$application = $app_orig; unset($app_orig);


echo"</SELECT>\n";
echo"<INPUT NAME=\"submit\" TYPE=\"SUBMIT\" VALUE=\"Update\">";
echo"</div></FORM>";
echo"</DIV>\n";
echo"&nbsp;<br>\n";

//---------------------------------
// Begin List
//---------------------------------
//Get Author Data and Create $authorarray and $authorids
$sql = "SELECT  TM.Name, TU.UserName, TU.UserID, TU.UserEmail FROM  `main`  TM 
LEFT JOIN authorxref TAX ON TM.ID = TAX.ID
INNER JOIN userprofiles TU ON TAX.UserID = TU.UserID
ORDER  BY  `Type` , `Name`  ASC ";
 $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
  while ($row = mysql_fetch_array($sql_result)) {
     $authorarray[$row[Name]][] = $row["UserName"];
     $authorids[$row[UserName]] = $row["UserID"];
   }

//Assemble a display application version array
$sql = "SELECT `Version`, `major`, `minor`, `release`, `SubVer` FROM `applications` WHERE `AppName`='$application' ORDER BY `major`,`minor`";
 $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
  while ($row = mysql_fetch_array($sql_result)) {
  $version = $row["Version"];
  $subver = $row["SubVer"];
  $release = "$row[major].$row[minor]";
  if ($row["release"]) {$release = ".$release$row[release]";}
  if ($subver !=="final") {$release="$release$subver";}

  $appvernames[$release] = $version;
  }

//Query to Generate List..
$sql = "$resultsquery";
 $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
  while ($row = mysql_fetch_array($sql_result)) {

    $id = $row["ID"];
    $type = $row["Type"];
    $name = $row["Name"];
    $dateadded = $row["DateAdded"];
    $dateupdated = $row["DateUpdated"];
    $homepage = $row["Homepage"];
    $description = nl2br($row["Description"]);
    $rating = $row["Rating"];
    $authors = $authorarray[$name];
    $osname = $row["OSName"];
    $appname = $row["AppName"];
    $downloadcount = $row["TotalDownloads"];
    $populardownloads = $row["downloadcount"];

//Get Version Record for Referenced MAXvID from list query
$sql2 = "SELECT TV.vID, TV.Version, TV.MinAppVer, TV.MaxAppVer, TV.Size, TV.DateAdded AS VerDateAdded, TV.DateUpdated AS VerDateUpdated, TV.URI, TV.Notes FROM  `version` TV
INNER  JOIN applications TA ON TV.AppID = TA.AppID
INNER  JOIN os TOS ON TV.OSID = TOS.OSID
WHERE TV.ID = '$id' AND TV.Version = '$row[Version]' AND TA.AppName = '$appname' AND TOS.OSName = '$osname' LIMIT 1";
 $sql_result2 = mysql_query($sql2, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
$vid = $row[MAXvID];
  $row = mysql_fetch_array($sql_result2);

   $vid = $row["vID"];
if ($appvernames[$row["MinAppVer"]]) {$minappver = $appvernames[$row["MinAppVer"]]; } else { $minappver = $row["MinAppVer"]; }
if ($appvernames[$row["MaxAppVer"]]) {$maxappver = $appvernames[$row["MaxAppVer"]]; } else { $maxappver = $row["MaxAppVer"]; }
   $VerDateAdded = $row["VerDateAdded"];
   $VerDateUpdated = $row["VerDateUpdated"];
   $filesize = $row["Size"];
   $notes = $row["Notes"];
   $version = $row["Version"];
   $uri = $row["URI"];
   $filename = basename($uri);

$sql3 = "SELECT `PreviewURI`, `caption` from `previews` WHERE `ID` = '$id' AND `preview`='YES' LIMIT 1";
 $sql_result3 = mysql_query($sql3, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
   $row3 = mysql_fetch_array($sql_result3);
   $previewuri = $row3["PreviewURI"];
   $caption = $row3["caption"];


if ($VerDateAdded > $dateadded) {$dateadded = $VerDateAdded; }
if ($VerDateUpdated > $dateupdated) {$dateupdated = $VerDateUpdated; }

//Turn Authors Array into readable string...
$authorcount = count($authors);
foreach ($authors as $author) {
$userid = $authorids[$author];
$n++;
$authorstring .= "<A HREF=\"authorprofiles.php?".uriparams()."&amp;id=$userid\">$author</A>";
if ($authorcount != $n) {$authorstring .=", "; }

}
$authors = $authorstring;
unset($authorstring, $n); // Clear used Vars.. 

if ($dateupdated > $dateadded) {
    $timestamp = $dateupdated;
    $datetitle = "Last Update: ";
  } else {
    $timestamp = $dateadded;
    $datetitle = "Added on: ";
  }

  $date = date("F d, Y g:i:sa",  strtotime("$timestamp"));


$datestring = "$datetitle $date";


echo"<DIV class=\"item\">\n";

echo"<div class=\"rating\">Rating: <img src=\"/images/stars-3-8.gif\" width=\"90\" height=\"20\" title=\"3.8 Stars out of 5\" alt=\"3.8 Stars\"></div>\n";

echo"<h2 class=\"first\"><A HREF=\"moreinfo.php?".uriparams()."&amp;id=$id\">$name $version</A></h2>";

        if ($previewuri) {

        echo"<p class=\"screenshot\">\n";

            list($width, $height, $attr) = getimagesize("$websitepath"."$previewuri");
            echo"<a href=\"moreinfo.php?".uriparams()."&amp;id=$id&amp;page=previews\"><img src=\"$previewuri\" height=$height width=$width alt=\"$name preview - $caption\" title=\"$caption\"></a>\n";

        echo"</p>\n";
        $preview_minheight=$height-50;
        }


echo"<P class=\"first\">By $authors</P>";

//Description & Version Notes
if ($previewuri AND $preview_minheight) {
    echo"<p class=\"first\" style=\"min-height: 80px\">";
} else {
    echo"<p class=\"first\">";
}
echo"$description<BR>";
if ($notes) {echo"<BR>$notes"; }
echo"</p>";



//Icon Bar Modules
echo"<DIV style=\"margin-top: 30px; height: 34px\">";
echo"<DIV class=\"iconbar\">";
if ($appname=="Thunderbird") {
    echo"<A HREF=\"moreinfo.php?".uriparams()."&amp;id=$id\"><IMG SRC=\"/images/download.png\" HEIGHT=32 WIDTH=32 TITLE=\"More Info about $name\" ALT=\"\">More Info</A>";
} else {
    echo"<a href=\"$uri\" onclick=\"return installTheme(event,'$name $version');\"><IMG SRC=\"/images/download.png\" HEIGHT=32 WIDTH=32 TITLE=\"Install $name\" ALT=\"\">Install</A>";
}
echo"<BR><SPAN class=\"filesize\">&nbsp;&nbsp;$filesize kb</SPAN></DIV>";
echo"<DIV class=\"iconbar\"><IMG SRC=\"/images/".strtolower($appname)."_icon.png\" HEIGHT=34 WIDTH=34 ALT=\"\">&nbsp;For $appname:<BR>&nbsp;&nbsp;$minappver - $maxappver</DIV>";
if($osname !=="ALL") { echo"<DIV class=\"iconbar\"><IMG SRC=\"/images/".strtolower($osname)."_icon.png\" HEIGHT=34 WIDTH=34 ALT=\"\">For&nbsp;$osname<BR>only</DIV>"; }
//if ($homepage) {echo"<DIV class=\"iconbar\"><A HREF=\"$homepage\"><IMG SRC=\"/images/home.png\" HEIGHT=34 WIDTH=34 TITLE=\"$name Homepage\" ALT=\"\">Homepage</A></DIV>";}
echo"</DIV>";

echo"<DIV class=\"baseline\">$datestring";
if ($populardownloads > 5 ) {echo" | Downloads Last 7 Days: $populardownloads";}
if ($downloadcount) { echo" | Total Downloads: $downloadcount"; }
echo"</DIV>\n";
echo"</DIV>\n";

} //End While Loop
if ($totalresults=="0") {
echo"<DIV class=\"item noitems\">\n";
echo"<P>No themes found in this category for ".ucwords($application).".</P>\n";
echo"</DIV>\n";

}
?>




<?php
echo"<H2>".ucwords("$application $typename &#187; $categoryname ")."</H2>";
echo"".ucwords("$typename")." $startitem - $enditem of $totalresults";
echo"&nbsp;&nbsp;|&nbsp;&nbsp;";

// Begin Code for Dynamic Navbars
if ($pageid <=$num_pages) {


$previd=$pageid-1;
if ($previd >"0") {
echo"<a href=\"?".uriparams()."&amp;pageid=$previd\">&#171; Previous</A> &bull; ";
}
echo"Page $pageid of $num_pages";


$nextid=$pageid+1;
if ($pageid <$num_pages) {
echo" &bull; <a href=\"?".uriparams()."&amp;pageid=$nextid\">Next &#187;</a>";
}
echo"<BR>\n";


//Skip to Page...
if ($num_pages>1) {
echo"Jump to Page: ";
$pagesperpage=9; //Plus 1 by default..
$i = 01;
//Dynamic Starting Point
if ($pageid>11) {
$nextpage=$pageid-10;
}
$i=$nextpage;

//Dynamic Ending Point
$maxpagesonpage=$pageid+$pagesperpage;
//Page #s
while ($i <= $maxpagesonpage && $i <= $num_pages) {

if ($i==$pageid) { 
    echo"<SPAN style=\"color: #FF0000\">$i</SPAN>&nbsp;";
  } else {
    echo"<A HREF=\"?".uriparams()."&amp;pageid=$i\">$i</A>&nbsp;";

}

    $i++;
}
}



}

?>
</div>
<?php
include"$page_footer";
?>
</div>
</BODY>
</HTML>