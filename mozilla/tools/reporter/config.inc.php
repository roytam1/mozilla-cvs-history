<?php
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla Reporter (r.m.o).
 *
 * The Initial Developer of the Original Code is
 *      Robert Accettura <robert@accettura.com>.
 *
 * Portions created by the Initial Developer are Copyright (C) 2004
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

// Debug
$config['debug'] = false;

// Paths
$config['app_url']                      = 'http://reporter.host.tld/app';
$config['app_path']                     = '/opt/webtools/reporter';
$config['pear_path']                    = ''; // empty if use default system path
                                          ini_set('include_path', $config['pear_path'] . PATH_SEPARATOR . ini_get('include_path'));
$config['nusoap_path']                  = $config['app_path'].'/contrib/nusoap';

// Database
$config['db_type']                      = 'mysql';
$config['db_server']                    = 'localhost';
$config['db_user']                      = 'USERNAME';
$config['db_pass']                      = 'PASSWORD';
$config['db_database']                  = 'DATABASE_NAME';
$config['db_dsn']                       = $config['db_type'].'://'.$config['db_user'].':'.$config['db_pass'].'@'.$config['db_server'].'/'.$config['db_database'];
$config['db_options']                   = array(
                                                'debug'       => 2,
                                                'portability' => DB_PORTABILITY_ALL,
                                          );

$config['gzip']                         = false; // should we gzip encode (more cpu, less bandwidth) XX-> Not implemented

$config['min_vers']                     = '0.2';

// products to show in the pull down on the query page
$config['products'][0]['id']            = 'Firefox/1.0';
$config['products'][0]['title']         = 'Firefox 1.0';

$config['show'] = 25;

/*****************************/
// Shouldn't need to touch this
/*****************************/
$config['self']                         = 'http://' . $_SERVER['SERVER_NAME'] . substr($_SERVER['PHP_SELF'], 0, strrpos($_SERVER['PHP_SELF'], '/')) . '/';

$boolTypes[-1] = '--';
$boolTypes[0] = 'No';
$boolTypes[1] = 'Yes';


// ProblemTypes
$problemTypes['0_0'] = '--';
$problemTypes['Crash']['1_1'] = 'Freeze';
$problemTypes['Crash']['1_2'] = 'Crashes Mozilla';
$problemTypes['Performance']['2_1'] = 'Slows Down Mozilla';

$problemTypes['Compatibility']['3_1'] = 'Appears Wrong';
$problemTypes['Compatibility']['3_2'] = 'Doesn\'t Appear';
$problemTypes['Compatibility']['3_3'] = 'Doesn\'t Function Properly';
$problemTypes['Compatibility']['3_4'] = 'Doesn\'t Load';
$problemTypes['Compatibility']['3_5'] = 'Mozilla Blocked / ("Netscape Not Supported")';
$problemTypes['Compatibility']['3_6'] = 'Appears Wrong';



// DB Debug code
function handleErrors($error) {
	echo "<hr />";
	echo "An error occurred while trying to run your query.<br>\n";
	echo "Error message: " . $error->getMessage() . "<br>\n";
	echo "A more detailed error description: " . $error->getDebugInfo() . "<br>\n";
	echo "<hr />";
	exit;
}

?>
