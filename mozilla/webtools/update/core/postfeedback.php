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
define("COMMENTS_MAX_LENGTH", 2000);

/**
 * Check if client has posted recently.
 *
 * @param string name - Name that they User Submitted
 * @param string addr - IPv4 Address of the client to check
 * @return bool - True if the client has posted too often, false if they have not
 *                posted recently.
 */
function client_rate_limited($name, $addr) 
{
    global $connection;

    $sql = "SELECT (UNIX_TIMESTAMP(CURRENT_TIMESTAMP()) - UNIX_TIMESTAMP(CommentDate)) as since_last_post
            FROM `feedback` 
            WHERE commentip = '".$addr."' OR
                  CommentName = '".$name."'
            ORDER BY CommentDate DESC LIMIT 1";
   
    $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_ERROR);
   
    $count = mysql_num_rows($sql_result);
    if ($count == 0) {
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

/**
 * @param string key - formkey to test for
 * @return bool True if the formkey is valid, false if invalid.
 */
function valid_form_key($key) 
{
    global $connection;
    //Check the Formkey against the DB, and see if this has already been posted...
    $formkey = escape_string($key);
    $date = date("Y-m-d H:i:s", mktime(0, 0, 0, date("m"), date("d")-1, date("Y")));
    $sql = "SELECT `CommentID` FROM  `feedback` WHERE `formkey` = '$formkey' AND `CommentDate`>='$date'";
    $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_ERROR);
    if (mysql_num_rows($sql_result) == 0) {
        return true;
    }
    return false;
}

/**
 * Check if a client is banned from posting comments
 * @param string address - IPv4 Client IP Address to check
 * @return bool - True if the client is banned, false if they are not.
 */
function client_ip_banned($address) 
{
    global $connection;
    $sql = "SELECT `bID` from `feedback_ipbans` WHERE `beginip` <= '$address' AND `endip` >='$address' LIMIT 1";
    $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_ERROR);
    if (mysql_num_rows($sql_result)== 0) {
        return false;
    }
    return true;
}

/**
 * Determine the Client IP Address as Logged
 * @return string IP Address of the client
 */
function get_client_ip() 
{
    // Are we behind a proxy and given the IP via an alternate enviroment variable? If so, use it.
    if (isset($_SERVER["HTTP_X_FORWARDED_FOR"])) {
        return $_SERVER["HTTP_X_FORWARDED_FOR"];
    } 
    else {
        return $_SERVER["REMOTE_ADDR"];
    }
}


//Check and see if the ID/vID is valid.
$sql = "SELECT TM.ID, TV.vID 
        FROM `main` TM 
        INNER JOIN `version` TV ON TM.ID=TV.ID 
        WHERE TM.ID = '".escape_string($_POST['id'])."' AND `vID`='".escape_string($_POST["vid"])."' LIMIT 1";

$sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_ERROR);
if (mysql_num_rows($sql_result)=="0") {
    unset($_POST["id"],$_POST["vid"],$id,$vid);
} 
else {
    $id = escape_string($_POST["id"]);
    $vid = escape_string($_POST["vid"]);
}

$comments = nl2br(strip_tags(escape_string($_POST["comments"])));
$email = escape_string($_POST["email"]);

$name="Anonymous";

if (strlen($comments) > COMMENTS_MAX_LENGTH) {
    page_error("29912312", "Your comment was longer than the maximum allowed length of ".COMMENTS_MAX_LENGTH." characters.");
    exit;
}

if (isset($_POST["name"])) {
    $name = escape_string(strip_tags($_POST["name"]));
}

$title="No Title";
if (isset($_POST["title"])) {
    $title = escape_string(strip_tags($_POST["title"]));
}

$rating = escape_string($_POST["rating"]);
// Make Sure Rating is as expected.
if (!(is_numeric($rating) and $rating<=5 and $rating>=0)) {
    unset($rating);
} 

if (!isset($rating) or !$comments ) {
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

if ($os !=="ALL") {
    $versiontagline .=" on $os";
}

$remote_addr = get_client_ip();

$form_key = "";
if (!valid_form_key($_POST['formkey'])) {
    page_error("5","Invalid formkey.");
    exit;
}
else {
    $formkey = escape_string($_POST['formkey']);
}

if (client_rate_limited($name, $remote_addr)) {
    page_error("6","You may only post one comment every ". COMMENTS_MIN_BREAK ." seconds.");
    exit;
}

if (client_ip_banned($remote_addr)) {
    page_error("7","Your IP Address is Banned from Making Comments.");
    exit;
}

$sql = "INSERT INTO `feedback` (`ID`, `CommentName`, `CommentVote`, `CommentTitle`, `CommentNote`, `CommentDate`, `commentip`, `email`, `formkey`, `VersionTagline`) VALUES ('$id', '$name', '$rating', '$title', '$comments', NOW(NULL), '$remote_addr', '$email', '$formkey', '$versiontagline');";
$sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);

update_rating($id);

if ($_POST["type"]=="E") {
    $type="extensions";
} else if ($_POST["type"]=="T") {
    $type="themes";
}

$return_path="$type/moreinfo.php?id=$id&vid=$vid&page=comments&action=successful";
header('Location: http://'.HOST_NAME.WEB_PATH.'/'.$return_path);
exit;

?>
