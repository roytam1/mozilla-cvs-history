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
//   Colin Ogilvie <colin.ogilvie@gmail.com>
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

//Inappropriate Comment Reporting Tool
require_once('../core/init.php');

if (strtolower($_SERVER['REQUEST_METHOD']) == 'post')
{
    $id = escape_string($_POST['id']);
    $commentid = escape_string($_POST['commentid']);
    
    //Check and see if the CommentID/ID is valid.
    $sql = "SELECT `ID`, `CommentID` FROM `feedback` WHERE `ID` = '".$id."' AND `CommentID`='" .$commentid."' LIMIT 1";
    $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_ERROR);
    if(mysql_num_rows($sql_result)==0) {
        page_error("4","Comment could not be found. Please go back and try again.");
    } else {
        if ($_POST['action'] == 'reportconfirm')
        {
            $sql = "UPDATE `feedback` SET `flag`='YES' WHERE `CommentID`='".$commentid."' LIMIT 1";
            $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
            if (mysql_affected_rows()==0)
            {
                page_error("5", "Comment could not be flagged for review. Please go back and try again.");
            } else {
                $page_title = 'Mozilla Update :: Report a Comment';
                require_once(HEADER);
                echo '<h1>Mozilla Update :: Report a Comment</h1>
                <p>You have sucessfully reported this comment to Mozilla Update staff.</p>
                <p>A staff member will review your submission and take the appropriate action.</p>
                <p>Thank you for your assistance.</p>';
                require_once(FOOTER);
            }
        }
    }
}
else
{
    $page_title = 'Mozilla Update :: Report a Comment';
    require_once(HEADER);
    $id = escape_string($_GET['id']);
    $commentid = escape_string($_GET['commentid']);
    // Check to see if Comment ID is valid
    $sql = "SELECT `ID`, `CommentID` FROM `feedback` WHERE `ID` = '".$id."' AND `CommentID`='" .$commentid."' LIMIT 1";
    $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_ERROR);
    if (mysql_num_rows($sql_result)==0)
    {
        page_error("4","Comment could not be found. Please go back and try again.");
        // page_error automatically exists for us.
    }
?>
    <h1>Mozilla Update :: Report a Comment</h1>
    <p>You have asked for a comment to be reviewed by the Mozilla Update staff. To confirm this action, please click 'Review this Comment' below.</p>
    <form action="reportcomment.php" method="post">
        <input type="hidden" name="id" value="<?=$id?>">
        <input type="hidden" name="commentid" value="<?=$commentid?>">
        <input type="hidden" name="action" value="reportconfirm">
        <input type="submit" name="submit" value="Review this Comment">
    </form>
<?
    require_once(FOOTER);
}
?>
