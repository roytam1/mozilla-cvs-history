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
//   Alan Starr <alanjstarr@yahoo.com>
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

// Set this page to read from the SHADOW_DB.
define('USE_SHADOW_DB',true);

require_once('../core/init.php');

$app = strtolower($_GET["application"]);  // Firefox, Thunderbird, Mozilla
$type = escape_string($_GET["type"]); //E, T, [P]
$list = ucwords(strtolower($_GET["list"])); // Newest, Updated, [Editors], Popular

$sitetitle = "Mozilla Update";
$siteurl = 'https://' . HOST_NAME . WEB_PATH;
$siteicon = $siteurl . '/favicon.ico';
$sitedescription = "the way to keep your mozilla software up-to-date";
$sitelanguage = "en-US";
$sitecopyright = "Copyright 2004-2005 The Mozilla Organization";
$currenttime = gmdate(r);// GMT 
$rssttl = "120"; //Life of feed in minutes

header("Content-Type: text/xml; charset=utf-8");

// Firefox, extensions, by date added

$select = "
    SELECT DISTINCT
        m.ID,
        m.name as Title,
        m.Type,
        m.Description,
        v.Version,
        v.vID,
        v.dateupdated as DateStamp,
        a.AppName
";

$from = "
    FROM
        main m
    INNER JOIN version v ON m.id = v.id
    INNER JOIN (
        SELECT v.id, v.appid, v.osid, max(v.vid) as mxvid 
        FROM version v       
        WHERE approved = 'YES' group by v.id, v.appid, v.osid) as vv 
    ON vv.mxvid = v.vid AND vv.id = v.id
    INNER JOIN applications a ON a.appid = v.appid
";

$where = "v.approved = 'YES'"; // Always have a WHERE

if ($app == 'firefox' || $app == 'thunderbird' || $app == 'mozilla') {
  $where .= " AND a.AppName = '$app'";
}

if ($type == 'E' || $type == 'T' || $type == 'P') {
  $where .= " AND m.Type = '$type'";
}

switch ($list) {
   case "Popular":
     $orderby = "m.downloadcount DESC, m.rating DESC, m.name ASC";
     break;
   case "Updated":
     $orderby = "m.dateupdated DESC";
     break;
   case "Rated":
     $orderby = "m.rating DESC";
     break;
   case "Newest":
   default:
     $orderby = "v.dateupdated DESC";
     break;
}

$sql = $select . " " . $from . " WHERE " . $where . " GROUP BY m.id ORDER BY " . $orderby . " LIMIT 0, 10";

require_once('inc_rssfeed.php');
?>
