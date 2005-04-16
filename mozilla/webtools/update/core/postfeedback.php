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

//Submit Review/Rating Feedback to Table
require_once('../core/init.php');

// Minimum number of seconds since the last comment.
define("COMMENTS_MIN_BREAK", 90);
/**
 * Check if client has posted recently.
 *
 * @param string name - Name that they User Submitted
 * @param string addr - IPv4 Address of the client to check
 * @return bool - True if the client has posted too often, false if they have not
 *                posted recently.
 */
function comment_rate_limited($name, $addr) 
{
   global $connection;

   $sql = "SELECT (UNIX_TIMESTAMP(CURRENT_TIMESTAMP()) - UNIX_TIMESTAMP(CommentDate)) as since_last_post
           FROM `feedback` 
           WHERE commentip = '".$addr."' OR
                 CommentName = '".$name."'
           ORDER BY CommentDate DESC LIMIT 1";
   
   $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_ERROR);
   
   $count = mysql_num_rows($sql_result);
   if($count == 0) {
       return false;
   }
   else if ($count == 1) {
       $row = mysql_fetch_array($sql_result);
       if ($row['since_last_post'] > COMMENTS_MIN_BREAK) {
           return false;
       }
   }
   return true;
}

//Check and see if the ID/vID is valid.
$sql = "SELECT TM.ID, TV.vID 
        FROM `main` TM 
        INNER JOIN `version` TV ON TM.ID=TV.ID 
        WHERE TM.ID = '".escape_string($_POST['id'])."' AND `vID`='".escape_string($_POST["vid"])."' LIMIT 1";

$sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_ERROR);
    if(mysql_num_rows($sql_result)=="0") {
        unset($_POST["id"],$_POST["vid"],$id,$vid);
    } else {
        $id = escape_string($_POST["id"]);
        $vid = escape_string($_POST["vid"]);
    }

    $name = escape_string(strip_tags($_POST["name"]));
    $title = escape_string(strip_tags($_POST["title"]));
    $rating = escape_string($_POST["rating"]);
    $comments = nl2br(strip_tags(escape_string($_POST["comments"])));
    $email = escape_string($_POST["email"]);
    if (!$name) {
        $name="Anonymous";
    }
    if (!$title) {
        $title="No Title";
    }

    //Make Sure Rating is as expected.
    if (is_numeric($rating) and $rating<=5 and $rating>=0) {
    } else {
        unset($rating);
    }

    if (!$rating or !$comments ) {
    //No Rating or Comment Defined, throw an error.
        page_error("3","Comment is Blank or Rating is Null.");
        exit;
    }


//Compile Info about What Version of the item this comment is about.
$sql = "SELECT TV.Version, `OSName`, `AppName` FROM `version` TV
        INNER JOIN `os` TOS ON TOS.OSID=TV.OSID
        INNER JOIN `applications` TA ON TA.AppID=TV.AppID
        WHERE TV.ID = '$id' AND TV.vID='$vid' LIMIT 1";
$sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_ERROR);
    $row = mysql_fetch_array($sql_result);
    $version = $row["Version"];
    $os = $row["OSName"];
    $appname = $row["AppName"];
    $versiontagline = "version $version for $appname";
    if ($os !=="ALL") {$versiontagline .=" on $os"; }

//Are we behind a proxy and given the IP via an alternate enviroment variable? If so, use it.
    if (isset($_SERVER["HTTP_X_FORWARDED_FOR"])) {
        $remote_addr = $_SERVER["HTTP_X_FORWARDED_FOR"];
    } else {
        $remote_addr = $_SERVER["REMOTE_ADDR"];
    }

//Check the Formkey against the DB, and see if this has already been posted...
$formkey = escape_string($_POST["formkey"]);
$date = date("Y-m-d H:i:s", mktime(0, 0, 0, date("m"), date("d")-1, date("Y")));
$sql = "SELECT `CommentID` FROM  `feedback` WHERE `formkey` = '$formkey' AND `CommentDate`>='$date'";
$sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_ERROR);
if (mysql_num_rows($sql_result)=="0" && comment_rate_limited($name, $remote_addr) === false) {

    //FormKey check passed, now let's see if this IP is banned...
    $sql = "SELECT `bID` from `feedback_ipbans` WHERE `beginip` <= '$remote_addr' AND `endip` >='$remote_addr' LIMIT 1";
    $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_ERROR);
    if (mysql_num_rows($sql_result)=="0") {
    //No Bans Returned, Proceed...

        //FormKey doesn't exist, go ahead and add their comment.
        $sql = "INSERT INTO `feedback` (`ID`, `CommentName`, `CommentVote`, `CommentTitle`, `CommentNote`, `CommentDate`, `commentip`, `email`, `formkey`, `VersionTagline`) VALUES ('$id', '$name', '$rating', '$title', '$comments', NOW(NULL), '$remote_addr', '$email', '$formkey', '$versiontagline');";
        $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);


        //Get Rating Data and Create $ratingarray
        $date = date("Y-m-d H:i:s", mktime(0, 0, 0, date("m"), date("d")-30, date("Y")));
        $sql = "SELECT ID, CommentVote FROM  `feedback` WHERE `ID` = '$id' AND `CommentDate`>='$date' AND `CommentVote` IS NOT NULL";
        $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
        while ($row = mysql_fetch_array($sql_result)) {
            $ratingarray[$row['ID']][] = $row["CommentVote"];
        }

        //Compile Rating Average
        if (!$ratingarray[$id]) {
            $ratingarray[$id] = array();
        }
        $numratings = count($ratingarray[$id]);
        $sumratings = array_sum($ratingarray[$id]);

        if ($numratings>0) {
            $rating = round($sumratings/$numratings, 1);
        } else {
            $rating="2.5"; //Default Rating
        }


        $sql = "UPDATE `main` SET `Rating`='$rating' WHERE `ID`='$id' LIMIT 1";
        $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
    } else {
        //User is Banned, Add Param to URI to throw an error about this...
        $action="ipbanned";
    }
}


if ($_POST["type"]=="E") {
    $type="extensions";
} else if ($_POST["type"]=="T") {
    $type="themes";
}

if (!isset($action)) {
    $action="successful";
}

$return_path="$type/moreinfo.php?id=$id&vid=$vid&page=comments&action=$action";
header('Location: http://'.HOST_NAME.WEB_PATH.'/'.$return_path);
exit;
?>
