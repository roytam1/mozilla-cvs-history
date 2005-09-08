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

/**
 * Mozilla Update Initialization Script
 * 
 * Configuration, libraries and includes are processed here.
 *
 * @package umo
 * @subpackage core
 * @author Mike Morgan
 */

// Process configuration file.
require_once('config.php');

// Connect to DB.
//
// The core includes depend on this, so taking this out for use in static pages
// is not possible.
//
// For completely static pages, do not require init.php, require only config.php.
// This prevents unnecessary database connections.
//

// If we have the SHADOW_DB flag set, use the SHADOW_DB - otherwise use the regular db.
if (defined('USE_SHADOW_DB')) {
    // SHADOW_DB_HOST, SHADOW_DB_USER, SHADOW_DB_PASS, SHADOW_DB_NAME are set in ./config.php
    $connection = mysql_connect(SHADOW_DB_HOST,SHADOW_DB_USER,SHADOW_DB_PASS) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_ERROR);
    $db = mysql_select_db(SHADOW_DB_NAME, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_ERROR);
} else { 
    // DB_HOST, DB_USER, DB_PASS, DB_NAME are set in ./config.php
    $connection = mysql_connect(DB_HOST,DB_USER,DB_PASS) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_ERROR);
    $db = mysql_select_db(DB_NAME, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_ERROR);
}

// Includes.
require_once('inc_guids.php'); // GUID --> AppName Handler
require_once('inc_global.php'); // Global Functions - Variable Cleanup
require_once('inc_browserdetection.php'); // Browser Detection - App Variable Handling

// Start timer.
$time_start = getmicrotime();
?>
