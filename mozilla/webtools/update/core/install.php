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
//      Chris "Wolf" Crews <psychoticwolf@carolina.rr.com>
//      Mike Morgan <morgamic@gmail.com>
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
require_once('../core/init.php');

//Get Full Information for the file requested.
$uri = escape_string(str_replace(" ","+",$_GET["uri"]));
$sql = "SELECT `vID`, TM.ID, `URI` FROM `version` TV INNER JOIN `main` TM ON TM.ID=TV.ID WHERE `URI`='$uri' LIMIT 1";
$sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);

if (mysql_num_rows($sql_result)=="0") {
    exit("Invalid URI cannot Continue");
} else {

    // Get our info.
    $row = mysql_fetch_array($sql_result);
    $uri=$row["URI"]; 
    $id = $row["ID"];
    $vid = $row["vID"];
}

//Are we behind a proxy and given the IP via an alternate enviroment variable? If so, use it.
if (!empty($_SERVER["HTTP_X_FORWARDED_FOR"])) {
    $remote_addr = mysql_real_escape_string($_SERVER["HTTP_X_FORWARDED_FOR"]);
} else {
    $remote_addr = mysql_real_escape_string($_SERVER["REMOTE_ADDR"]);
}

// Clean the user agent string.
$http_user_agent = mysql_real_escape_string($_SERVER['HTTP_USER_AGENT']);

// Rate limit set to 10 minutes.
$sql = "
    SELECT
        `dID`
    FROM
        `downloads`
    WHERE
        `ID`='$id' AND
        `vID`='$vid' AND
        `user_ip`='$remote_addr' AND
        `user_agent`='$http_user_agent' AND
        `date`>DATE_SUB(NOW(), INTERVAL 10 MINUTE)
    LIMIT 
        1
";
$sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);

if (mysql_num_rows($sql_result)==0) {
    $sql = "INSERT INTO `downloads` (`ID`,`date`,`vID`, `user_ip`, `user_agent`) VALUES ('$id',NOW(),'$vid', '$remote_addr', '$http_user_agent');";
    $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
}

// Send User on their way to the file, if requested...
if ($_GET["passthrough"]=="yes") {
    header("Location: $uri");
}
?>
