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
// Portions created by the Initial Developer are Copyright (C) 2004
// the Initial Developer. All Rights Reserved.
//
// Contributor(s):
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
/**
 * Maintenance script for addons.mozilla.org.
 *
 * The purpose of this document is to perform periodic tasks that should not be
 * done everytime a download occurs in install.php.  This should reduce
 * unnecessary DELETE and UPDATE queries and lighten the load on the database
 * backend.
 *
 * This script should not ever be accessed over HTTP.
 *
 * @package umo
 * @subpackage core
 * @TODO list this in v2 requirements
 * @TODO consider creating DB wrappers for v1 updates...
 * @TODO explore using hashes to reduce DB load
 */


// Before doing anything, test to see if we are calling this from the command
// line.  If this is being called from the web, HTTP environment variables will
// be automatically set by Apache.  If these are found, exit immediately.
if (isset($_SERVER['HTTP_HOST'])) {
    exit;
}

// If we get here, CRON is calling this so we can continue.
require_once('../core/init.php');

/**
 * Update addon counts and statistics.
 */

// Get our addon IDs.
$addons_sql = 'SELECT ID FROM main';
$addons_result = mysql_query($addons_sql, $connection) 
    or trigger_error('MySQL Error '.mysql_errno().': '.mysql_error()."", 
                     E_USER_NOTICE);

// Store our addon IDs in an array.
$addons = array();
while ($addon = mysql_fetch_array($addons_result)) {
    $addons[] = $addon['ID'];
}

// For each addon, update the weekly, daily and total download counts.
foreach ($addons as $id) {

    // Get today's uncounted hits from the download table.
    $today_hits_sql = "
        SELECT
            COUNT(*) as count
        FROM
            `downloads`
        WHERE
            `ID`='{$id}' AND
            `counted`=0 AND
            `type`='download' AND
            `date` > DATE_SUB(NOW(), INTERVAL 24 HOUR)
    ";
    $today_hits_result = mysql_query($today_hits_sql, $connection) 
        or trigger_error('MySQL Error '.mysql_errno().': '.mysql_error()."", 
                         E_USER_NOTICE);
    if (mysql_num_rows($today_hits_result) > 0) {
        $row = mysql_fetch_array($today_hits_result);
        $today_hits = ($row['count'] > 0) ? $row['count'] : 0;
        unset($row);
    } else {
        $today_hits = 0;
    }


    // Update today's count.  Create a record if one doesn't exist yet.
    $today_check_sql = "SELECT `dID` FROM `downloads` WHERE `ID`='{$id}' AND `date`=CURDATE() AND `type`='count' LIMIT 1";
    $today_check_sql_result = mysql_query($today_check_sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);

    if (mysql_num_rows($today_check_sql_result)=="0" && $today_hits > 0) {
        $update_today_sql = "INSERT INTO `downloads` (`ID`,`date`,`downloadcount`,`type`) VALUES ('{$id}',CURDATE(),'{$today_hits}','count');";
    } else {
        $row = mysql_fetch_array($today_check_sql_result);
        $update_today_sql = "UPDATE `downloads` SET `downloadcount`=downloadcount+{$today_hits} WHERE `dID`='{$row['dID']}' LIMIT 1";
    }

    $update_today_sql_result = mysql_query($update_today_sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);


    // Get our 7 day counts from the download table.
    $seven_day_count_sql = "
        SELECT
            SUM(`downloadcount`) as seven_day_count
        FROM
            `downloads`
        WHERE
            `ID`='{$id}' AND
            `date`>= DATE_SUB(CURDATE(), INTERVAL 7 DAY) AND
            `type`='count'
    ";
    $seven_day_count_result = mysql_query($seven_day_count_sql, $connection) 
        or trigger_error('MySQL Error '.mysql_errno().': '.mysql_error()."", 
                         E_USER_NOTICE);
    if (mysql_num_rows($seven_day_count_result) > 0 ) {
        $row = mysql_fetch_array($seven_day_count_result);
        $seven_day_count = ($row['seven_day_count']>0) ? $row['seven_day_count'] : 0;
        unset($row);
    } else {
        $seven_day_count = 0; 
    }

    // Update the 7 day count in the main record.
    $seven_day_count_update_sql = "
        UPDATE
            `main`
        SET
            `downloadcount`='{$seven_day_count}'
        WHERE
            `ID`='{$id}'
        LIMIT
            1
    ";
    $seven_day_count_update_result = mysql_query($seven_day_count_update_sql, 
                                                 $connection) 
        or trigger_error('MySQL Error '.mysql_errno().': '.mysql_error()."", 
                         E_USER_NOTICE);

    // Get all uncounted hits from the download table.
    $uncounted_hits_sql = "
        SELECT
            COUNT(*) as count
        FROM
            `downloads`
        WHERE
            `ID`='{$id}' AND
            `counted`=0 AND
            `type`='download'
    ";
    $uncounted_hits_result = mysql_query($uncounted_hits_sql, $connection) 
        or trigger_error('MySQL Error '.mysql_errno().': '.mysql_error()."", 
                         E_USER_NOTICE);
    if (mysql_num_rows($uncounted_hits_result) > 0) {
        $row = mysql_fetch_array($uncounted_hits_result);
        $uncounted_hits = ($row['count'] > 0) ? $row['count'] : 0;
        unset($row);
    } else {
        $uncounted_hits = 0;
    }

    // Update the total downloadcount in the main record based on the uncounted
    // records.
    $uncounted_update_sql = "
        UPDATE
            `main`
        SET
            `TotalDownloads`=`TotalDownloads`+{$uncounted_hits}
        WHERE
            `ID`='{$id}'
    ";
    $uncounted_update_result = mysql_query($uncounted_update_sql, $connection) 
        or trigger_error('MySQL Error '.mysql_errno().': '.mysql_error()."", 
                         E_USER_NOTICE);


}

// If we get here, we've counted everything and we can mark stuff for
// deletion.
//
// Mark the downloads we just counted as counted if:
//      a) it is a day count that is more than 8 days old
//      b) it is a download log that has not been counted
$counted_update_sql = "
    UPDATE
        `downloads`
    SET
        `counted`=1
    WHERE
        ((`date` < DATE_SUB(CURDATE(), INTERVAL 8 DAY) AND `type`='count') OR
        (`counted`=0 AND `type`='download'))
";
$counted_update_result = mysql_query($counted_update_sql, $connection) 
    or trigger_error('MySQL Error '.mysql_errno().': '.mysql_error()."", 
                     E_USER_NOTICE);
?>
