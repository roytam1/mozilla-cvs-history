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

//Various Sample User_Agents, uncomment to debug detection for one. :-)
//$_SERVER["HTTP_USER_AGENT"] = "Mozilla/5.0 (Photon; U; QNX x86pc; en-US; rv:1.6a) Gecko/20030122";
//$_SERVER["HTTP_USER_AGENT"] = "Mozilla/5.0 (BeOS; U; BeOS BePC; en-US; rv:1.4a) Gecko/20030305";
//$_SERVER["HTTP_USER_AGENT"] = "Mozilla/5.0 (X11; U; SunOS i86pc; en-US; rv:1.7b) Gecko/20040302";
//$_SERVER["HTTP_USER_AGENT"] = "Mozilla/5.0 (Macintosh; U; PPC Mac OS X Mach-O; en-US; rv:1.7) Gecko/20040803 Firefox/0.9.3";
//$_SERVER["HTTP_USER_AGENT"] = "Mozilla/5.0 (Windows; U; Windows NT 5.1; en-US; rv:1.7.3) Gecko/20041001 Firefox/0.10.1";


if (isset($_GET["version"]) && $_GET["version"]=="auto-detect") {$_GET["version"]="";}//Clear Version For AutoDetect 

//Change OS Support for showlist.php
if (isset($_GET["os"])) {
  switch ( $_GET["os"] )
  {
    case 'windows':
      $_GET["os"] = 'Windows';
      break;
    case 'linux':
      $_GET["os"] = 'Linux';
      break;
    case 'solaris':
      $_GET["os"] = 'Solaris';
      break;
    case 'bsd':
      $_GET["os"] = 'BSD';
      break;
    case 'macosx':
      $_GET["os"] = 'MacOSX';
      break;
    case 'all':
      $_GET["os"] = 'ALL';
      break;
    default:
      unset($_GET["os"]);
      break;
  }
}

if (isset($_GET["application"])) {
    $application = escape_string($_GET["application"]);
}
if (isset($_GET["version"])) {
    $app_version = escape_string($_GET["version"]);
}
if (isset($_GET["os"])) {
    $OS = escape_string($_GET["os"]);
}
//print("$application, $app_version, $OS<br>\n");


include"browser_detection.php"; //Script that defines the browser_detection() function

if (!isset($OS)) {$OS = browser_detection('os');}
if (!isset($application) or !isset($app_version)) {$moz_array = browser_detection('moz_version');}

//Turn $OS into something usable.
if ( isset($moz_array) && $moz_array[0] !== '' )
{
  switch ( $OS )
  {
    case 'win':
    case 'nt':
      $OS = 'Windows';
      break;
    case 'lin':
      $OS = 'Linux';
      break;
    case 'solaris':
    case 'sunos':
      $OS = 'Solaris';
      break;
    case 'unix':
    case 'bsd':
      $OS = 'BSD';
      break;
    case 'mac':
      $OS = 'MacOSX';
      break;
    default:
      break;
  }

//Print what it's found, debug item.
//echo ( 'Your Mozilla product is ' . $moz_array[0] . ' ' . $moz_array[1] . ' running on '. $OS . '<br>');

if (!isset($application)) {$application = $moz_array[0];}
if (!isset($app_version)) {$app_version = $moz_array[1];}

//If the applicatin is user-defined and not the same as what was detected, ignore the detected version and use the user-defined.
if (isset($_GET["application"]) and $_GET["application"] !==$moz_array[0]) {
    $app_version = escape_string($_GET["version"]); 
}


} else {
//If it's not a Mozilla product, then return nothing and let the default app code work..
}

//----------------------------
//Browser & OS Detection (Default Code)
//----------------------------

//Application
if (!isset($application) || !$application) { $application="firefox"; } //Default App is Firefox

//App_Version
//Get Max Version for Application Specified
if (!isset($app_version) || !$app_version) {
    $sql = "SELECT `Version` FROM `applications` WHERE `AppName` = '$application' ORDER BY `Version` DESC LIMIT 1";
    $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
    $row = mysql_fetch_array($sql_result);
         $app_version = $row['Version'];
}

//So, at this point we gracefully leave, feeling happy and sending 3 variables on.
//$application
//$app_version
//$OS
if (!isset($OS)) {
    $OS = "unknown";
}

?>
