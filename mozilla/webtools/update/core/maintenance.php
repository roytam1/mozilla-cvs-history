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
 */


// Before doing anything, test to see if we are calling this from the command
// line.  If this is being called from the web, HTTP environment variables will
// be automatically set by Apache.  If these are found, exit immediately.
if (isset($_SERVER['HTTP_HOST'])) {
    exit;
}

// If we get here, we're on the command line, which means we can continue.
require_once('init.php');

// Start our timer.
$start = getmicrotime();

// Get our action.
$action = isset($_SERVER['argv'][1]) ? $_SERVER['argv'][1] : '';

// Perform specified task.  If a task is not properly defined, exit.
switch ($action) {

    /**
     * Update weekly addon counts.
     */
    case 'weekly':
        // Get 7 day counts from the download table.
        $seven_day_count_sql = "
            SELECT
                downloads.ID as ID,
                COUNT(downloads.ID) as seven_day_count
            FROM
                `downloads`
            WHERE
                `date` >= DATE_SUB(NOW(), INTERVAL 7 DAY)
            GROUP BY
                downloads.ID
            ORDER BY
                downloads.ID
        ";

        echo 'Retrieving seven-day counts from `downloads` ...'."\n";
        $seven_day_count_result = mysql_query($seven_day_count_sql, $connection) 
            or trigger_error('MySQL Error '.mysql_errno().': '.mysql_error()."", 
                             E_USER_NOTICE);

        $affected_rows = mysql_num_rows($seven_day_count_result);
    
        if ($affected_rows > 0 ) {
            $seven_day_counts = array();
            while ($row = mysql_fetch_array($seven_day_count_result)) {
                $seven_day_counts[$row['ID']] = ($row['seven_day_count']>0) ? $row['seven_day_count'] : 0;
            }

            echo 'Updating seven day counts in `main` ...'."\n";

            foreach ($seven_day_counts as $id=>$seven_day_count) {
                $seven_day_count_update_sql = "
                    UPDATE `main` SET `downloadcount`='{$seven_day_count}' WHERE `id`='{$id}'
                ";

                $seven_day_count_update_result = mysql_query($seven_day_count_update_sql, $connection) 
                    or trigger_error('mysql error '.mysql_errno().': '.mysql_error()."", 
                                     E_USER_NOTICE);
            }
        }
    break;



    /**
     * Update all total download counts.
     */
    case 'total':

        // Get the max dID from downloads so we don't miscount hits 
        // that occur while the update query is running.
        $max_sql = "
            SELECT
                MAX(dID) as max_id
            FROM
                downloads
        ";

        $max_result = mysql_query($max_sql, $connection)
            or trigger_error('MySQL Error '.mysql_errno().': '.mysql_error()."", 
                             E_USER_NOTICE);

        $max_row = mysql_fetch_array($max_result,MYSQL_ASSOC);

        $max_id = $max_row['max_id'];

        // Get uncounted hits from the download table.
        // We only select counts for dID < max_id for accuracy.
        $uncounted_hits_sql = "
            SELECT
                downloads.ID as ID,
                COUNT(downloads.ID) as count
            FROM
                downloads
            WHERE
                `counted`=0 AND
                dID <= '{$max_id}'
            GROUP BY
                downloads.ID
            ORDER BY
                downloads.ID
        ";

        echo 'Retrieving uncounted downloads ...'."\n";

        $uncounted_hits_result = mysql_query($uncounted_hits_sql, $connection) 
            or trigger_error('MySQL Error '.mysql_errno().': '.mysql_error()."", 
                             E_USER_NOTICE);
        $affected_rows = mysql_num_rows($uncounted_hits_result);

        if ($affected_rows > 0) {
            $uncounted_hits = array();
            while ($row = mysql_fetch_array($uncounted_hits_result)) {
                $uncounted_hits[$row['ID']] = ($row['count'] > 0) ? $row['count'] : 0;
            }

            echo 'Updating download totals ...'."\n";

            foreach ($uncounted_hits as $id=>$hits) {
                $uncounted_update_sql = "
                    UPDATE `main` SET `TotalDownloads`=`TotalDownloads`+{$hits} WHERE `ID`='{$id}'
                ";
                $uncounted_update_result = mysql_query($uncounted_update_sql, $connection) 
                    or trigger_error('MySQL Error '.mysql_errno().': '.mysql_error()."", 
                                     E_USER_NOTICE);
            }


            // If we get here, we've counted everything and we can mark stuff for
            // deletion.
            //
            // Mark the downloads we just counted as counted if:
            //      c) it has a key lower than max_id, because 
            //         all keys lower than max_id have been counted above
            //
            $counted_update_sql = "
                UPDATE
                    `downloads`
                SET
                    `counted`=1
                WHERE
                    dID <= '{$max_id}'
            ";
            $counted_update_result = mysql_query($counted_update_sql, $connection) 
                or trigger_error('MySQL Error '.mysql_errno().': '.mysql_error()."", 
                                 E_USER_NOTICE);
        } else {
            $affected_rows = 0;
        }
    break;



    /**
     * Garbage collection for all records that are older than 8 days.
     */
    case 'gc':
        echo 'Starting garbage collection ...'."\n";
        $gc_sql = "
            DELETE FROM
                `downloads`
            WHERE
                `date` < DATE_SUB(NOW(), INTERVAL 8 DAY)
        ";
        $gc_result = mysql_query($gc_sql, $connection) 
            or trigger_error('MySQL Error '.mysql_errno().': '.mysql_error()."", 
                             E_USER_NOTICE);

        // This is unreliable, but it's not a big deal.
        $affected_rows = mysql_affected_rows();
    break;



    /**
     * Unknown command.
     */
    default:
        echo 'Command not found. Exiting ...'."\n";
        exit;
    break;
}
// End switch.



// How long did it take to run?
$exectime = getmicrotime() - $start;



// Display script output.
echo 'Affected rows: '.$affected_rows.'    ';
echo 'Time: '.$exectime."\n";
echo 'Exiting ...'."\n";



exit;
?>
