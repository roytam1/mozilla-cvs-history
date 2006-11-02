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

require"../core/init.php";
require"./core/sessionconfig.php";
$function = $_GET['function'];
$page_title = 'Mozilla Update :: Developer Control Panel :: List Manager';
require_once(HEADER);
require_once('./inc_sidebar.php');

//this screen is only for admins
if ($_SESSION["level"] !=="admin") {

  $id = escape_string($_GET["id"]);                                           
  if (!$id) {
    $id = escape_string($_POST["id"]); 
  }                             
  $sql = "SELECT `UserID` from `authorxref` TAX WHERE `ID` = '$id' AND `UserID` = '$_SESSION[uid]' LIMIT 1";
  $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
  if (mysql_num_rows($sql_result)=="0") {              
    echo"<h1>Access Denied</h1>\n";
    echo"You do not have access to this item.";
    include(FOOTER);
    exit;
  }
  if(!$function) {
    $function = "editmain";
  }
}
?>

<?php
if (!$function) {
$typearray = array("E"=>"Extensions","T"=>"Themes");
if (!$_GET["type"]) {$_GET["type"]="E";}
?>

<h1>Manage <?php $typename = $typearray[$_GET[type]]; echo"$typename"; ?>:</h1>
<SPAN style="font-size: 10pt"><a href="additem.php?function=additem&type=<?php echo"$_GET[type]"; ?>" style="font-weight: bold;">Add New Item</a> | Show: <?php

$count = count($typearray);
$i = 0;
foreach ($typearray as $type =>$typename) {
$i++;
echo"<a href=\"?type=$type\">$typename</a>";
if ($i !== $count) {echo" / "; }

}
unset($i);
?></SPAN>

<TABLE BORDER=0 CELLPADDING=1 CELLSPACING=0 ALIGN=CENTER STYLE="border: solid 0px #000000; width: 95%" class="listing">
<TR>
<TH><!-- Counter --></TH>
<TH>Name</TH>
<TH>Description</TH>
<TH>Last Updated</TH>
</TR>

<?php
$type = escape_string($_GET["type"]);
$sql = "SELECT  TM.ID, TM.Name, TM.Description, TM.DateUpdated FROM  `main`  TM ";
if ($_SESSION[level]=="user") { $sql .= "LEFT JOIN authorxref TAX ON TM.ID = TAX.ID
INNER JOIN userprofiles TU ON TAX.UserID = TU.UserID "; }
$sql .= "WHERE TM.Type = '$type'";
if ($_SESSION[level]=="user") {$sql .=" AND TU.UserEmail = '$_SESSION[email]'"; }
$sql .=" ORDER  BY  `Type` , `Name`  ASC ";
 $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
   $numresults = mysql_num_rows($sql_result);
   while ($row = mysql_fetch_array($sql_result)) {
    $id = $row["ID"];
    $name = $row["Name"];
    $description = substr($row["Description"],0,75);
    $dateupdated = gmdate("F d, Y", strtotime($row["DateUpdated"]));
    $i++;
    echo"<tr>\n";
    echo"<td align=\"center\" width=\"20\">$i</td>\n";
    echo"<td><a href=\"itemoverview.php?id=$id\">$name</a></td>\n";
    echo"<td>$description</td>\n";
    echo"<td nowrap>$dateupdated</td>\n";
    echo"</tr>\n";
}
?>
</table>
<div style="width: 580px; border: 1px dotted #AAA; margin-top: 2px; margin-left: 50px; padding: 5px; font-size: 10pt; font-weight: bold">
<form name="addapplication" method="post" ACTION="?function=additem2" enctype="multipart/form-data">
<?writeFormKey();?>
<a href="additem.php?function=additem">New <?php $typename = $typearray[$_GET[type]]; echo"$typename"; ?></A>
  Input File: <input name="file" size=30 type="file">
<input name="type" type="hidden" value="<?php echo"$_GET[type]"; ?>">
<input name="submit" type="submit" value="Next &#187;"></SPAN>
</form>
</div>
<?php
} else if ($function=="editmain") {

//Process Submitted Values if this is a return with $_POST data for the parent objects...
if ($_POST["submit"]=="Update") {
unset($sql_result);
 echo"<DIV style=\"width: 95%; text-align:left; margin: 0px; margin-bottom: 10px\">";
 echo"<h2>Updating $name, please wait...</h2>";
if ($_POST["name"] && $_POST["authors"] && $_POST["categories"] && $_POST["description"]) {
//Everything We *must* have is present... Begin....

//Phase One, update main values...
if (checkFormKey()) {
  $sql = "UPDATE `main` SET `Name`= '".escape_string($_POST[name])."', `Homepage`='".escape_string($_POST[homepage])."', `Description`='".escape_string($_POST[description])."', `DateUpdated`=NOW(NULL) WHERE `ID`='".escape_string($_POST[id])."' LIMIT 1";
   //echo"$sql<br>\n"; //Debug
   $sql_result = mysql_query($sql, $connection) or trigger_error("<FONT COLOR=\"#FF0000\"><B>MySQL Error ".mysql_errno().": ".mysql_error()."</B></FONT>", E_USER_NOTICE);
if ($sql_result) {
   echo"Your update to $_POST[name] has been submitted successfully...<br>\n";
}
}
echo"<SPAN style=\"font-size 10pt;\">";

//Phase Two, Author List -- Autocomplete and Verify
  $authors = escape_string($_POST["authors"]);
  $authors = explode(", ","$authors");
foreach ($authors as $author) {
if (strlen($author)<2) {continue;} //Kills all values that're too short.. 
$a++;
 $sql = "SELECT `UserID`,`UserEmail` FROM `userprofiles` WHERE `UserEmail` LIKE '$author%' ORDER BY `UserMode`, `UserName` ASC";
 $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
 $numresults = mysql_num_rows($sql_result);
   while ($row = mysql_fetch_array($sql_result)) {
    $userid = $row["UserID"];
    $useremail = $row["UserEmail"];
  if ($numresults>1) {
   //Too many e-mails match, store individual data for error block.
    $r++;
     $emailerrors[$a]["foundemails"][$r] = $useremail;
  }
 $authorids[] = $userid;
 }
 if ($numresults !="1") {
  //No Valid Entry Found for this E-Mail or too many, kill and store data for error block.
  $emailerrors[$a]["author"] = "$author";
  $updateauthors = "false"; // Just takes one of these to kill the author update.
  }
}
unset($a,$r);

//Commit Updates to AuthorXref tables.. with the ID and UserID.
if ($updateauthors != "false") {
 //Remove Current Authors
 if (checkFormKey()) {
  $id = escape_string($_POST["id"]);
  $sql = "DELETE FROM `authorxref` WHERE `ID` = '$id'";
  $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
 }

 //Add New Authors based on $authorids
 sort($authorids);
  foreach ($authorids as $authorid) {
    if (checkFormKey()) {
      $id = escape_string($_POST["id"]);
      $sql = "INSERT INTO `authorxref` (`ID`, `UserID`) VALUES ('$id', '$authorid');";
      $result = mysql_query($sql) or trigger_error("<FONT COLOR=\"#FF0000\"><B>MySQL Error ".mysql_errno().": ".mysql_error()."</B></FONT>", E_USER_NOTICE);
    }
  }
   echo"Authors for $_POST[name]'s updated...<br>\n";
} else {
   echo"ERROR: Could not update Authors list, please fix the errors printed below and try again...<br>\n"; 
}

unset($authors); //Clear from Post.. 

//Phase Three, Category List, update linkages.
//print_r($_POST["categories"]);
 //Delete Current Category Linkages...
  if (checkFormKey()) {
   $id = escape_string($_POST["id"]);
   $sql = "DELETE FROM `categoryxref` WHERE `ID` = '$id'";
   //echo"$sql<br>\n"; //Debug
   $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
  
 //Add New Categories from $_POST["categories"]
  $sql = "SELECT `Type` from `main` WHERE `ID` = '$id' LIMIT 1";
  $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
    $row = mysql_fetch_array($sql_result);
    $type = $row["Type"];

  foreach ($_POST["categories"] as $categoryname) {
    $sql2 = "SELECT `CategoryID` FROM `categories` WHERE `CatType` = '$type' AND `CatName` = '$categoryname' ORDER BY `CategoryID` ASC";
    $sql_result2 = mysql_query($sql2, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
    while ($row2 = mysql_fetch_array($sql_result2)) {
        $categoryid = $row2["CategoryID"];

        $sql = "INSERT INTO `categoryxref` (`ID`, `CategoryID`) VALUES ('$id', '$categoryid');";
        $result = mysql_query($sql) or trigger_error("<FONT COLOR=\"#FF0000\"><B>MySQL Error ".mysql_errno().": ".mysql_error()."</B></FONT>", E_USER_NOTICE);
   
    }

  }
   echo"Categories for $_POST[name]'s updated...<br>\n";
  }
//End _POST if.

} else {
//Something Isnt Present.. Throw the main error and details...

echo"You're missing one or more required fields, please return to the previous page and correct the errors and try again...<br>\n";
if (!$_POST["name"]) {echo"Name is required<br>\n";}
if (!$_POST["authors"]) {echo"Authors is required<br>\n";}
if (!$_POST["categories"]) {echo"Categories is required<br>\n";}
if (!$_POST["description"]) {echo"Description is required<br>\n";}
}   

echo"</SPAN></DIV>";



} else if ($_POST["submit"]=="Delete") {
$name = escape_string($_POST["name"]);
$id = escape_string($_POST["id"]);

echo"<h1>Deleting $name, please wait...</h1>\n";
  $sql = "SELECT `Version`, `URI` FROM `version` WHERE `ID`='$id' GROUP BY `URI` ORDER BY `Version` ASC";
  $sql_result = mysql_query($sql, $connection) or trigger_error("<FONT COLOR=\"#FF0000\"><B>MySQL Error ".mysql_errno().": ".mysql_error()."</B></FONT>", E_USER_NOTICE);
    while ($row = mysql_fetch_array($sql_result)) {
     $version = $row["Version"];
     $uri = $row["URI"];

    //Delete File(s) from server
      if (strpos("$uri","approvalfile.php/")) {
        $file = str_replace('http://'.HOST_NAME.'/developers/approvalfile.php/',REPO_PATH.'/approval/',$uri);
      } else if (strpos("$uri","ftp.mozilla.org")) {
        $ftppath = $uri;
        $file = str_replace("http://ftp.mozilla.org/pub/mozilla.org/",REPO_PATH.'/ftp/',$uri);
      }

      if (file_exists($file)) {
        $unlink = unlink($file);
      } else {
        $file_exists = "false";
      }

      if ($file_exists == "false" or !$unlink) {
        echo"<strong>Error!</strong> Encountered a problem when trying to delete associated files, files were not deleted successfully<br>\n";
      } else {
        echo"Deleted $name $version (".basename($uri).") successfully...<br>\n";
      }

    }
    if ($ftppath) {
    $basename = basename($ftppath);
    $ftppath = str_replace("/$basename","",$ftppath);
    $ftppath = str_replace("http://ftp.mozilla.org/pub/mozilla.org",REPO_PATH."/ftp",$ftppath);
    if (@rmdir($ftppath)) {
    echo"Removed Directory for $name...<br>\n";
    } else {
    echo"<strong>Warning!</strong> Unable to remove directory for $name. Directory may not be empty.<br>\n";
    }
    }

  if (checkFormKey()) {
   $sql = "DELETE FROM `main` WHERE `ID`='$id'";
   $sql_result = mysql_query($sql, $connection) or trigger_error("<FONT COLOR=\"#FF0000\"><B>MySQL Error ".mysql_errno().": ".mysql_error()."</B></FONT>", E_USER_NOTICE);
    if ($sql_result) {
    echo"$name has been deleted...<br>\n";
    echo"<a href=\"./\">&#171;&#171; Back to Main Page...</a><br>\n";
    require_once(FOOTER);
    exit;
    }
   }
}


//----------------------
// END
//----------------------



//Get Parent Item Information
$id = escape_string($_GET["id"]);
if (!$id) {$id = escape_string($_POST["id"]); }
$sql = "SELECT  TM.ID, TM.Type, TM.GUID, TM.Name, TM.Description, TM.DateAdded, TM.DateUpdated, TM.Homepage, TU.UserEmail FROM  `main`  TM 
LEFT JOIN authorxref TAX ON TM.ID = TAX.ID
INNER JOIN userprofiles TU ON TAX.UserID = TU.UserID
WHERE TM.ID = '$id'
ORDER  BY  `Type` , `Name`  ASC ";
 $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
  while ($row = mysql_fetch_array($sql_result)) {
    $v++;
    $id = $row["ID"];
    $type = $row["Type"];
    $guid = $row["GUID"];
    $name = $row["Name"];
    $dateadded = date("l, F, j  Y g:i:s",strtotime($row["DateAdded"]));
    $dateupdated = date("l, F, j  Y g:i:s",strtotime($row["DateUpdated"]));
    $homepage = $row["Homepage"];
    $description = $row["Description"];
    $downloadcount = $row["DownloadCount"];
    $authors[$v] = $row["UserEmail"];
 }
 unset($v);
    if (!$guid) { $guid = "None"; }

//Make Authors E-Mail Array into a Nice String
$num_authors = count($authors);
foreach($authors as $author) {
$v++;
$authorstring .= "$author";
if ($v != $num_authors) { $authorstring .=", "; }
}
$authors = $authorstring;
unset($v);
?>
<h1><?php echo"Edit $name"; ?></h1>
<TABLE CELLPADDING=1 CELLSPACING=1 STYLE="border: solid 0px #000000;">
<FORM NAME="editmain" METHOD="POST" ACTION="?function=editmain&<?php echo"id=$id"; ?>">
<?writeFormKey();?>
<INPUT NAME="id" TYPE="HIDDEN" VALUE="<?php echo"$id"; ?>">
<TR><TD>GUID:</TD><TD><?php echo"$guid"; ?></TD>

<?php
//Get Currently Set Categories for this Object...
 $sql = "SELECT  TCX.CategoryID, TC.CatName FROM  `categoryxref`  TCX 
INNER JOIN categories TC ON TCX.CategoryID = TC.CategoryID
WHERE TCX.ID = '$id'
ORDER  BY  `CatName`  ASC ";
 $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
  while ($row = mysql_fetch_array($sql_result)) {
   $n++;
   $catid = $row["CategoryID"];
   $categories[$n] = $catid;
  }
unset($n);
if (!$categories) {$categories = array(); }

//Get the Category Table Data for the Select Box
 $sql = "SELECT  `CategoryID`, `CatName` FROM  `categories` WHERE `CatType` = '$type' GROUP BY `CatName` ORDER  BY  `CatName` ASC";
 $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
?>
<TD ROWSPAN=8 VALIGN=TOP><SPAN class="global">Categories:</SPAN><BR>&nbsp;&nbsp;&nbsp;&nbsp;<SELECT NAME="categories[]" MULTIPLE="YES" SIZE="10">
<?php
  while ($row = mysql_fetch_array($sql_result)) {
    $catid = $row["CategoryID"];
    $catname = $row["CatName"];

    echo"<OPTION value=\"$catname\"";
    foreach ($categories as $validcat) {
    if ($validcat==$catid) { echo" selected=\"selected\" "; }
    }
    echo">$catname</OPTION>\n";
  }
?>
</SELECT></TD></TR>

<tr><td>Date Added:</td><td><?php echo"$dateadded"; ?></td></tr>
<tr><td>Last Updated:</td><td><?php echo"$dateupdated"; ?></td></tr>
<TR><TD><SPAN class="global">Name*</SPAN></TD> <TD><INPUT NAME="name" TYPE="TEXT" VALUE="<?php echo"$name"; ?>" SIZE=50 MAXLENGTH=100></TD></TR>

<?php
//Author Error Handling/Display Block for Form Post...
if ($emailerrors) {
echo"<TR><TD COLSPAN=2 STYLE=\"\">\n";
echo"<DIV style=\"margin-left 2px; border: 1px dotted #CCC; width: 550px; font-weight: bold\">";
echo"<FONT STYLE=\"color: #FF0000; font-weight: bolder\">Errors Found in Authors</FONT><BR>\n";
echo"<FONT STYLE=\"font-size: 10pt\">\n";
foreach ($emailerrors as $authorerror) {
$author = $authorerror["author"];
$count = count($authorerror["foundemails"]);

if ($count=="0") {
//Error for No Entry Found
echo"Entry '$author': No Matches Found, Please check your entry and try again.<BR>\n";
} else {
//Error for Too Many Entries Found
echo"Entry '$author': Too Many Matches, Please make your entry more specific.<BR>\n";
}
if ($count>0 AND $count<6) {
echo"&nbsp;&nbsp;&nbsp;&nbsp;Possible Addresses found: ";
foreach ($authorerror["foundemails"] as $foundemails) {
$a++;
echo"$foundemails";

if ($a != $count) {echo", "; } else {echo"<br>\n";}
}
}

}
echo"</font></DIV></TD></TR>\n";
$authors = $_POST["authors"];
}
?>
<TR><TD><SPAN class="global">Author(s):*</SPAN></TD><TD><INPUT NAME="authors" TYPE="TEXT" VALUE="<?php echo"$authors"; ?>" SIZE=50></TD></TR>
<TR><TD><SPAN class="global">Homepage</SPAN></TD> <TD><INPUT NAME="homepage" TYPE="TEXT" VALUE="<?php echo"$homepage"; ?>" SIZE=50 MAXLENGTH=200></TD></TR>
<TR><TD><SPAN class="global">Description*</SPAN></TD> <TD><TEXTAREA NAME="description" ROWS=4 COLS=45><?php echo"$description"; ?></TEXTAREA></TD></TR>
<TR><TD COLSPAN="2" ALIGN="CENTER"><INPUT NAME="submit" TYPE="SUBMIT" VALUE="Update">&nbsp;&nbsp;<INPUT NAME="reset" TYPE="RESET" VALUE="Reset Form">&nbsp;&nbsp;<INPUT NAME="submit" TYPE="SUBMIT" VALUE="Delete" ONCLICK="return confirm('Warning! Are you sure you want to delete <?php echo"$name"; ?>? This action is not reversable and will remove all files and listed versions.');"></TD></TR>
</FORM>
</TABLE>
&nbsp;&nbsp;&nbsp;<a href="itemoverview.php?id=<?php echo"$id"; ?>">&#171;&#171; Back to Item Overview</a>

<?php
} else if ($function=="editversion") {

//Process Submitted Values if this is a return with $_POST data for the parent objects...
if ($_POST["submit"]=="Update") {

unset($sql_result);
 echo"<DIV style=\"width: 100%; margin: auto; margin-bottom: 10px\">";
 echo"<h2>Updating Version, please wait...</h2>\n";
//print_r($_POST);
//echo"<br><Br>\n";

echo"<SPAN style=\"font-size 10pt;\">";
//Phase One-Part 1 -- Version (per-file record update)
$notes = escape_string($_POST["notes"]);
$reviewnotes = escape_string($_POST["reviewnotes"]);
$id = escape_string($_POST["id"]);
$uri = escape_string($_POST["uri"]);
$osid = escape_string($_POST["osid"]);
 if (checkFormKey()) {
  $sql = "UPDATE `version` SET `OSID`='$osid', `Notes`='$notes', `ReviewNotes`='$reviewnotes' WHERE `ID`='$id' AND `URI`='$uri'";
  $sql_result = mysql_query($sql, $connection) or trigger_error("<FONT COLOR=\"#FF0000\"><B>MySQL Error ".mysql_errno().": ".mysql_error()."</B></FONT>", E_USER_NOTICE);
   echo"Version Notes and OS for $_POST[name] $_POST[version] updated...<br>\n";
 }
//Phase Two -- Update Min/Max Versions

//Construct Internal App_Version Arrays
$i=0;
$sql = "SELECT `Version` FROM `applications` ORDER BY `AppName`, `Version` DESC";
 $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
  while ($row = mysql_fetch_array($sql_result)) {
  $i++;

    $app_internal_array[$row['Version']] = $row['Version'];
  }

//print_r($app_internal_array);

for ($i = 1; $i <= $_POST[maxval]; $i++) {
unset($minappver_int,$maxappver_int);
$minappver = escape_string($_POST["minappver_$i"]);
$maxappver = escape_string($_POST["maxappver_$i"]);
$vid = escape_string($_POST["appvid_$i"]);

if ($minappver && $maxappver) {
 if (checkFormKey()) {
  $sql = "UPDATE `version` SET `MinAppVer`='$minappver', `MaxAppVer`='$maxappver' WHERE `vID`='$vid'";
  //echo"$sql<br>\n";
  $sql_result = mysql_query($sql, $connection) or trigger_error("<FONT COLOR=\"#FF0000\"><B>MySQL Error ".mysql_errno().": ".mysql_error()."</B></FONT>", E_USER_NOTICE);
    echo"Updated Target Application Values for Application $i...<br>\n";
 }
} else {
if (!$minappver) { echo"<SPAN class=\"error\">Error: Minimum Version is not specified or invalid</SPAN><BR>\n"; }
if (!$maxappver) { echo"<SPAN class=\"error\">Error: Maximum Version is not specified or invalid</SPAN><BR>\n"; }
}
}

echo"</SPAN></DIV>";



} else if ($_POST["submit"]=="Delete") {
$id = escape_string($_POST["id"]);
$uri = escape_string($_POST["uri"]);
$version = escape_string($_POST["version"]);
echo"<h1>Deleting Version... Please wait...</h1>\n";
$sql = "SELECT `Name` FROM `main` WHERE `ID` = '$id'";
 $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
  $row = mysql_fetch_array($sql_result);
    $name = $row["Name"];

  $sql_result = false;
  if (checkFormKey()) {
   $sql = "DELETE FROM `version` WHERE `ID`='$id' AND `URI`='$uri'";
   $sql_result = mysql_query($sql, $connection) or trigger_error("<FONT COLOR=\"#FF0000\"><B>MySQL Error ".mysql_errno().": ".mysql_error()."</B></FONT>", E_USER_NOTICE);
  }
    if ($sql_result) {
    //Delete File from server
    if (strpos("$uri","approvalfile.php/")) {
    $file = str_replace('http://'.HOST_NAME.'/developers/approvalfile.php/',REPO_PATH.'/approval/',$uri);
    } else if (strpos("$uri","ftp.mozilla.org")) {
    $file = str_replace('http://ftp.mozilla.org/pub/mozilla.org/',REPO_PATH.'/ftp/',$uri);
    }

    if (file_exists($file)) {
    $unlink = unlink($file);
    } else {
    $file_exists = "false";
    }

    if ($file_exists == "false" or !$unlink) {
    echo"<strong>Error!</strong> Encountered a problem when trying to delete associated files, files were not deleted successfully<br>\n";
    }
    
    echo"<DIV>$name $version has been deleted...<br>
    <a href=\"itemoverview.php?id=$id\">&#171;&#171; Back to the $name overview page...</a></DIV>";
    require_once(FOOTER);
    echo"</BODY></HTML>";
    exit;
    }
}


//----------------------
// END
//----------------------

//Get Parent Item Information
$id = escape_string($_GET["id"]);
if (!$id) {$id = escape_string($_POST["id"]); }
$sql = "SELECT  TM.ID, TM.Name FROM  `main` TM WHERE TM.ID = '$id' LIMIT 1";
 $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
  $row = mysql_fetch_array($sql_result);
    $id = $row["ID"];
    $name = $row["Name"];
 

//-----------------------
//  Version Table
//-----------------------
$vid = escape_string($_GET["vid"]);
if (!$vid) {$vid = escape_string($_POST["vid"]); }
//Get Data for Form Population
//INNER JOIN main TM ON TV.ID = TM.ID 
 $sql = "SELECT `Version`, TV.OSID, `OSName`, `URI`, `Notes`, `ReviewNotes`, `Size`,`DateAdded`, `DateUpdated` FROM `version` TV INNER JOIN `os` TOS ON TOS.OSID=TV.OSID WHERE `vID` = '$vid' ORDER BY `Version` ASC LIMIT 1";
 $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
  $row = mysql_fetch_array($sql_result);
    $v++;
    $version=$row["Version"];
    $osid=$row["OSID"];
    $osname=$row["OSName"];
    $minappver=$row["MinAppVer"];
    $maxappver=$row["MaxAppVer"];
    $uri=$row["URI"];
    $filename=basename($row["URI"]);
    $notes=$row["Notes"];
    $reviewnotes=$row["ReviewNotes"];
    $size=$row["Size"];

?>
<h1><?php echo"Edit $name Version $version"; ?></h1>
<TABLE CELLPADDING=1 CELLSPACING=1 STYLE="border: solid 0px #000000;">
<FORM NAME="editversion" METHOD="POST" ACTION="?function=editversion&<?php echo"id=$id&vid=$vid"; ?>">
<?writeFormKey();?>
<?php
    echo"<INPUT NAME=\"id\" TYPE=\"HIDDEN\" VALUE=\"$id\">\n";
    echo"<INPUT NAME=\"uri\" TYPE=\"HIDDEN\" VALUE=\"$uri\">\n";
    echo"<INPUT NAME=\"version\" TYPE=\"HIDDEN\" VALUE=\"$version\">\n";
    echo"<TR><TD colspan=2>$filename ($size"."kb)</TD></TR>\n";



//Construct App_Versions Arrays
$i=0;
$sql = "SELECT `AppName`, `Version` FROM `applications` ORDER BY `AppName`, `Version` DESC";
 $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
  while ($row = mysql_fetch_array($sql_result)) {
  $i++;
    $AppName = strtolower($row['AppName']);
    $builtAppVersion = $row['Version'];
    $app_ver_array[$AppName][] = $builtAppVersion;
    $app_display_array[$AppName][$builtAppVersion] = $row['Version'];
  }

  
$i=0;
echo"<TR><TD COLSPAN=2><SPAN class=\"file\">Target Application(s):</SPAN></TD></TR>\n";
 $sql = "SELECT vID, TV.AppID,`AppName`,`MinAppVer`,`MaxAppVer` FROM  `version` TV INNER JOIN `applications` TA ON TA.AppID=TV.AppID WHERE `ID` = '$id' && TV.URI = '".escape_string($uri)."' ORDER BY `AppName` ASC";
 $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
  while ($row = mysql_fetch_array($sql_result)) {
    $i++;
    $appname = $row["AppName"];
    $appid=$row["AppID"];
    $minappver=$row["MinAppVer"];
    $maxappver=$row["MaxAppVer"];

    echo"<TR><TD></TD><TD>$appname ";
    echo"<INPUT NAME=\"appvid_$i\" TYPE=\"hidden\" VALUE=\"$row[vID]\">";
    echo"<SELECT name=\"minappver_$i\" TITLE=\"Minimum Version* (Required)\">\n";
foreach ($app_ver_array[strtolower($appname)] as $app_version) {
    $display_version = $app_display_array[strtolower($appname)][$app_version];
    echo"<OPTION value=\"$app_version\""; if ($app_version==$minappver) {echo" selected=\"selected\" ";} echo">$display_version</OPTION>\n";
}
    echo"</SELECT>\n";
    echo"&nbsp;-&nbsp;";

    echo"<SELECT name=\"maxappver_$i\" TITLE=\"Maximum Version* (Required)\">\n";
foreach ($app_ver_array[strtolower($appname)] as $app_version) {
    $display_version = $app_display_array[strtolower($appname)][$app_version];
    echo"<OPTION value=\"$app_version\""; if ($app_version==$maxappver) {echo" selected=\"selected\" ";} echo">$display_version</OPTION>\n";
}
    echo"</SELECT>\n";
    echo"</TD></TR>\n";

}
    echo"<INPUT type=\"hidden\" name=\"maxval\" value=\"$i\">";
    echo"<TR><TD><SPAN class=\"file\">OS*</SPAN></TD><TD>";
    echo"<SELECT name=\"osid\">";
$os = $osid;
$sql = "SELECT * FROM `os` ORDER BY `OSName` ASC";
 $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
  while ($row = mysql_fetch_array($sql_result)) {
    $osname = $row["OSName"];
    $osid = $row["OSID"];
    echo"<OPTION value=\"$osid\""; if ($osid == $os) {echo" selected=\"selected\" "; } echo">$osname</OPTION>\n";
  }
    echo"</SELECT>\n";
    echo"</TD></TR>\n";
?>
<TR><TD style="vertical-align: top;"><SPAN class="global">Version Notes:</SPAN></TD><TD COLSPAN=2><TEXTAREA NAME="notes" ROWS=4 COLS=55><?=$notes?></TEXTAREA></TD></TR>
<TR><TD style="vertical-align: top;"><SPAN class="global">Notes to Reviewer:</SPAN></TD><TD COLSPAN=2><TEXTAREA NAME="reviewnotes" ROWS=3 COLS=55><?=$reviewnotes?></TEXTAREA></TD></TR>


<TR><TD COLSPAN="3" ALIGN="CENTER"><INPUT NAME="submit" TYPE="SUBMIT" VALUE="Update">&nbsp;&nbsp;<INPUT NAME="reset" TYPE="RESET" VALUE="Reset Form">&nbsp;&nbsp;<INPUT NAME="submit" TYPE="SUBMIT" VALUE="Delete" ONCLICK="return confirm('Are you sure you want to delete <?php echo"$name $version"; ?>?');"></TD></TR>
</FORM>
</TABLE>
&nbsp;&nbsp;&nbsp;<a href="itemoverview.php?id=<?php echo"$id"; ?>">&#171;&#171; Back to Item Overview</a>

<?php
} else {}
?>


<!-- close #mBody-->
</div>

<?php
require_once(FOOTER);
?>
