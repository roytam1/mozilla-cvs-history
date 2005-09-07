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
 * Mozilla Update configuration.
 *
 * Define central configuration variables.  Use CAPS for constants to flag them
 * as being defined in this document.
 *
 * Copy this document (config-dist.php) to (config.php).  This prevents
 * committing database or working directory information back to CVS.
 *
 * @package umo
 * @subpackage core
 * @author Mike Morgan
 */

// Path information.  No trailing slashes. (old variable)
define('FILE_PATH',''); // Root filepath of application. ($websitepath)
define('WEB_PATH',''); // Relative webpath. ('' if at root).
define('REPO_PATH',FILE_PATH.'/files'); // XPI repo path. ($repositorypath)
define('FTP_URL','http://ftp.mozilla.org/pub/mozilla.org'); // FTP. ($ftpurl)
define('HOST_NAME',$_SERVER['SERVER_NAME']); // Host (*.*.*) ($sitehostname)

// Site template includes. 
define('HEADER',FILE_PATH.'/core/inc_header.php');
define('FOOTER',FILE_PATH.'/core/inc_footer.php');

// DB config.
// This is accessed for general read/write requests.
define('DB_HOST',''); // MySQL host
define('DB_USER',''); // MySQL username
define('DB_PASS',''); // MySQL password
define('DB_NAME',''); // MySQL database

// Shadow DB config.
// This is accessed by high-load read-only requests.
define('SHADOW_DB_HOST',''); // Shadow MySQL host
define('SHADOW_DB_USER',''); // Shadow MySQL username
define('SHADOW_DB_PASS',''); // Shadow MySQL password
define('SHADOW_DB_NAME',''); // Shadow MySQL database
?>
