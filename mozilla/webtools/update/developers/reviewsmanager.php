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
$function = $_GET['function'];
$page_title = 'Mozilla Update :: Developer Control Panel :: Reviews Manager';
require_once(HEADER);
require_once('./inc_sidebar.php');

if ($_SESSION["level"] !=="admin") {
    echo"<h1>Access Denied</h1>\n";
    echo"You do not have access to the Editor Reviews Manager";
    require_once(FOOTER);
    exit;
}
?>
<?php
if (!$function) {
$typearray = array("E"=>"Extensions","T"=>"Themes");
if (!$_GET["type"]) {$_GET["type"]="E";}
?>

<h1>Manage Reviews for <?php $typename = $typearray[$_GET[type]]; echo"$typename"; ?>:</h1>
<SPAN style="font-size: 10pt">Show: <?php

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
<TH>Review...</TH>
<TH>Review Posted</TH>
</TR>

<?php
$type = escape_string($_GET["type"]);

$sql = "SELECT  TM.ID, TM.Name, TR.Body as Description, TR.DateAdded  FROM  `main` TM LEFT JOIN `reviews` TR ON TR.ID=TM.ID WHERE TM.Type = '$type' ORDER  BY  `Type` , ltrim(`Name`)  ASC ";
 $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
   $numresults = mysql_num_rows($sql_result);
   while ($row = mysql_fetch_array($sql_result)) {
    $id = $row["ID"];
    $name = $row["Name"];
    $description = substr($row["Description"],0,75);
    if ($description) {$description .="..."; }
    if ($row["DateAdded"]) {
        $dateadded = date("F d, Y", strtotime($row["DateAdded"]));
    } else {
        $dateadded = "N/A";
    }
    $i++;
    echo"<tr>\n";
    echo"<td align=\"center\" width=\"20\">$i.</td>\n";
    echo"<td><a href=\"?function=editreview&id=$id\">$name</a></td>\n";
    echo"<td>$description</td>\n";
    echo"<td nowrap>$dateadded</td>\n";
    echo"</tr>\n";
}
?>
</table>

<?php
} else if ($function=="editreview") {

    //Process Submitted Values if this is a return with $_POST data for the parent objects...
    if ($_POST["submit"]=="Add Review") {
        $name = escape_string($_POST["name"]);
        echo"<h2>Adding review for $name, please wait...</h2>";

        if ($_POST["title"] && $_POST["body"] && $_POST["id"] && $_POST["method"]=="add") {
        //Everything We *must* have is present... Begin....

            if (checkFormKey()) {
                $sql = "INSERT INTO `reviews` (`ID`,`DateAdded`,`AuthorID`,`Title`,`Body`,`ExtendedBody`,`pick`,`featured`,`featuredate`) VALUES ('".escape_string($_POST[id])."', NOW(NULL), '".escape_string($_SESSION[uid])."','".escape_string($_POST[title])."','".escape_string($_POST[body])."','".escape_string($_POST[extendedbody])."','".escape_string($_POST[pick])."','".escape_string($_POST[featured])."','".escape_string($_POST[featuredate])."');";
                $sql_result = mysql_query($sql, $connection) or trigger_error("<FONT COLOR=\"#FF0000\"><B>MySQL Error ".mysql_errno().": ".mysql_error()."</B></FONT>", E_USER_NOTICE);
                if ($sql_result) {
                    echo"Your review of $name has been submitted successfully...<br>\n";
                }
            }
        }

    } else if ($_POST["submit"]=="Update Review") {
        $name = escape_string($_POST["name"]);
        echo"<h2>Updating review for $name, please wait...</h2>";

        if ($_POST["title"] && $_POST["body"] && $_POST["rid"] && $_POST["method"]=="edit") {
        //Everything We *must* have is present... Begin....

            if (checkFormKey()) {
                $sql = "UPDATE `reviews` SET `Title`= '".escape_string($_POST[title])."', `Body`='".escape_string($_POST[body])."', `ExtendedBody`='".escape_string($_POST[extendedbody])."', `pick`='".escape_string($_POST[pick])."', `featured`='".escape_string($_POST[featured])."', `featuredate`='".escape_string($_POST[featuredate])."' WHERE `rID`='".escape_string($_POST[rid])."' LIMIT 1";
                $sql_result = mysql_query($sql, $connection) or trigger_error("<FONT COLOR=\"#FF0000\"><B>MySQL Error ".mysql_errno().": ".mysql_error()."</B></FONT>", E_USER_NOTICE);
                if ($sql_result) {
                    echo"Your update to the review for $name has been submitted successfully...<br>\n";
                }
            }
        }

    } else if ($_POST["submit"]=="Delete") {
        $name = escape_string($_POST["name"]);
        $rid = escape_string($_POST["rid"]);

        echo"<h1>Deleting $name, please wait...</h1>\n";

        if (checkFormKey()) {
            $sql = "DELETE FROM `reviews` WHERE `rID`='$rid' LIMIT 1";
            $sql_result = mysql_query($sql, $connection) or trigger_error("<FONT COLOR=\"#FF0000\"><B>MySQL Error ".mysql_errno().": ".mysql_error()."</B></FONT>", E_USER_NOTICE);
            if ($sql_result) {
                echo"The review for $name has been deleted...<br>\n";
                echo"<a href=\"?type=$type\">&#171;&#171; Back to Main Page...</a><br>\n";
                require_once(FOOTER);
                exit;
            }

        }

    }

//Get Parent Item Information
$id = escape_string($_GET["id"]);
if (!$id) {$id = escape_string($_POST["id"]); }

$sql = "SELECT  TM.ID, TM.Type, TM.Name FROM  `main` TM  WHERE TM.ID = '$id' LIMIT 1";
 $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
  $row = mysql_fetch_array($sql_result);
    $id = $row["ID"];
    $type = $row["Type"];
    $name = $row["Name"];

$sql = "SELECT `rID`,TU.UserName as AuthorName, `DateAdded`, `Title`, `Body`, `ExtendedBody`, `pick`, `featured`, `featuredate`  FROM `reviews` INNER JOIN `userprofiles` TU ON reviews.AuthorID=TU.UserID WHERE `ID` = '$id' LIMIT 1";
    $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
    $sql_num = mysql_num_rows($sql_result);
    if ($sql_num=="0") { $method="Add"; $action="Add"; } else { $method="Edit"; $action="Update"; }
    $row = mysql_fetch_array($sql_result);
      $rid = $row["rID"];
      $authorname = $row["AuthorName"];
      $dateadded = date("F d, Y", strtotime($row["DateAdded"]));
      $title = $row["Title"];
      $body = $row["Body"];
      $extendedbody = $row["ExtendedBody"];
      $pick = $row["pick"];
      $featured = $row["featured"];
      $featuredate = $row["featuredate"];
      if (!$featuredate) {$featuredate = date("Ym"); }
      if (!$authorname) { $authorname = $_SESSION["name"]; }
?>
<h1><?php echo"$method Review for $name"; ?></h1>
<?php echo"Review written by $authorname on $dateadded<br>\n"; ?>

<TABLE CELLPADDING=1 CELLSPACING=1 STYLE="border: solid 0px #000000;">
<FORM NAME="editmain" METHOD="POST" ACTION="?function=editreview&<?php echo"id=$id"; ?>">
<?writeFormKey();?>
<INPUT NAME="rid" TYPE="HIDDEN" VALUE="<?php echo"$rid"; ?>">
<INPUT NAME="id" TYPE="HIDDEN" VALUE="<?php echo"$id"; ?>">
<INPUT NAME="name" TYPE="HIDDEN" VALUE="<?php echo"$name"; ?>">
<INPUT NAME="method" TYPE="HIDDEN" VALUE="<?php echo strtolower($method); ?>">
<TR><TD><SPAN class="global">Title*</SPAN></TD> <TD><INPUT NAME="title" TYPE="TEXT" VALUE="<?php echo"$title"; ?>" SIZE=50 MAXLENGTH=100></TD></TR>
<TR><TD><SPAN class="global">Body*</SPAN></TD> <TD><TEXTAREA NAME="body" ROWS=4 COLS=60><?php echo"$body"; ?></TEXTAREA></TD></TR>
<TR><TD><SPAN class="global">Extended Body</SPAN></TD> <TD><TEXTAREA NAME="extendedbody" ROWS=8 COLS=60><?php echo"$extendedbody"; ?></TEXTAREA></TD></TR>

<TR><TD>
<?php
echo"Editor's Pick:</TD>\n<TD>";
if ($pick=="YES") {
 echo"<INPUT NAME=\"pick\" TYPE=\"RADIO\" VALUE=\"YES\" CHECKED>Yes  <INPUT NAME=\"pick\" TYPE=\"RADIO\" VALUE=\"NO\">No";
 } else if ($pick=="NO") {
 echo"<INPUT NAME=\"pick\" TYPE=\"RADIO\" VALUE=\"YES\">Yes <INPUT NAME=\"pick\" TYPE=\"RADIO\" VALUE=\"NO\" CHECKED>No";
 } else {
 echo"<INPUT NAME=\"pick\" TYPE=\"RADIO\" VALUE=\"YES\">Yes <INPUT NAME=\"pick\" TYPE=\"RADIO\" VALUE=\"NO\">No";
 }
?>
</TD></TR>

<TR><TD>
<?php
echo"Featured:</TD>\n<TD>";
if ($featured=="YES") {
 echo"<INPUT NAME=\"featured\" TYPE=\"RADIO\" VALUE=\"YES\" CHECKED>Yes  <INPUT NAME=\"featured\" TYPE=\"RADIO\" VALUE=\"NO\">No";
 } else if ($featured=="NO") {
 echo"<INPUT NAME=\"featured\" TYPE=\"RADIO\" VALUE=\"YES\">Yes <INPUT NAME=\"featured\" TYPE=\"RADIO\" VALUE=\"NO\" CHECKED>No";
 } else {
 echo"<INPUT NAME=\"featured\" TYPE=\"RADIO\" VALUE=\"YES\">Yes <INPUT NAME=\"featured\" TYPE=\"RADIO\" VALUE=\"NO\">No";
 }
?>
&nbsp;&nbsp;&nbsp;<span class="tooltip" title="Month this item should appear on the frontpage, Format: YYYYMM">Feature Month</span>: <input name="featuredate" type="text" size=6 value="<?php echo"$featuredate"; ?>">
</TD></TR>

<TR><TD COLSPAN="2" ALIGN="CENTER"><INPUT NAME="submit" TYPE="SUBMIT" VALUE="<?php echo"$action"; ?> Review">&nbsp;&nbsp;<INPUT NAME="reset" TYPE="RESET" VALUE="Reset Form">&nbsp;&nbsp;<?php if ($method=="Add") {} else { ?><INPUT NAME="submit" TYPE="SUBMIT" VALUE="Delete" ONCLICK="return confirm('Warning! Are you sure you want to delete the review for <?php echo"$name"; ?>?');"><?php } ?></TD></TR>
</FORM>
</TABLE>
&nbsp;&nbsp;&nbsp;<a href="?type=<?php echo"$type"; ?>">&#171;&#171; Back to Reviews Manager</a>

<?php
} else {}
?>


<!-- close #mBody-->
</div>

<?php
require_once(FOOTER);
?>
