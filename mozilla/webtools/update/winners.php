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
	
// Set this page to read from the SHADOW_DB.
define('USE_SHADOW_DB',true);

require_once('./core/init.php');

$titleCaseApp=ucwords($application); // cache results!
$uriparams=uriparams(); // cache results!
$page_title = 'Extend Firefox Contest Winners :: Mozilla Update';
require_once(HEADER);
installtrigger('extensions');

echo <<<pagetop
<div id="mBody">
<h1>Extend Firefox Contest Winners</h1>
<p>We are happy to announce the winners in our <a href="http://extendfirefox.com/">Extend Firefox Contest</a>, a contest held to award the best and brightest extension developers in the Firefox community.  The contest asked entrants to create Firefox Extensions that are innovative, useful, and integrate with today's Web services. Over 200 Extensions were submitted to the contest.  Many thanks to everyone who entered and everyone who helped spread the word about the contest.</p>
pagetop;

/**
 * Setting up variables.
 */
$guids = array(
'Reveal@sourmilk.net'=>'Grand Prize Winner: New Extension, Best in Class: Best User Experience New Extension',
'{c45c406e-ab73-11d8-be73-000a95be3b12}'=>'Grand Prize Winner: Upgraded Extension',
'{89506680-e3f4-484c-a2c0-ed711d481eda}'=>'Grand Prize Winner: Best Use of Firefox 1.5 Features',
'xpose@viamatic.com'=>'Best in Class: Most Innovative New Extension',
'{a6ca9b3b-5e52-4f47-85d8-cca35bb57596}'=>'Best in Class: Most Innovative Upgraded Extension',
'separe@m4ng0.lilik.it'=>'Best in Class Most Useful New Extension',
'{53A03D43-5363-4669-8190-99061B2DEBA5}'=>'Best in Class: Most Useful Upgraded Extension',
'{097d3191-e6fa-4728-9826-b533d755359d}'=>'Best in Class: Best User Experience Upgraded Extension',
'{bbc21d30-1cff-11da-8cd6-0800200c9a66}'=>'Best in Class: Best Integration with a Web Service New Extension',
'{3CE993BF-A3D9-4fd2-B3B6-768CBBC337F8}'=>'Best in Class: Best Integration with a Web Service Upgraded Extension'
);

$guids_tmp = array();
foreach ($guids as $guid=>$msg) {
    $guids_tmp[] = "'".$guid."'";
}
$guids_imploded = implode(',',$guids_tmp);

$descriptions = array(
'{097d3191-e6fa-4728-9826-b533d755359d}'=>"Manage Extensions, Themes, Downloads, and more including Web content via Firefox’s sidebar.",
'{89506680-e3f4-484c-a2c0-ed711d481eda}'=>"View open Tabs and Windows with Showcase.  You can use it in two ways: global mode (F12) or local mode (Shift + F12). In global mode, a new window will be opened with thumbnails of the pages you've opened in all windows. In local mode, only content in tabs of your current window will be shown.

You can also right click in those thumbnails to perform the most usual operations on them. Mouse middle button can be used to zoom a thumbnail, although other actions can be assigned to it.",
'{3CE993BF-A3D9-4fd2-B3B6-768CBBC337F8}'=>"Get international weather forecasts and display it in any toolbar or status bar.",
'{bbc21d30-1cff-11da-8cd6-0800200c9a66}'=>"Allows sticky notes to be added to any web page, and viewed upon visiting the Web page again.  You can also share sticky notes.  Requires account.",
'Reveal@sourmilk.net'=>"Reveal allows you to see thumbnails of pages in your history by mousing over the back and forward buttons.  With many tabs open, quickly find the page you want, by pressing F2. Reveal also has a rectangular magnifying glass you can use to zoom in on areas of any web page. Comes  with a quick tour of all the features. ",
'{a6ca9b3b-5e52-4f47-85d8-cca35bb57596}'=>"A lightweight RSS and Atom feed aggregator.  Alt+S to open Sage in the Sidebar to start reading feed content.",
'{53A03D43-5363-4669-8190-99061B2DEBA5}'=>"Highlight text, create sticky notes, and more to Web pages and Web sites that are saved to your desktop.  Scrapbook Includes full text search and quick filtering of saved pages.",
'xpose@viamatic.com'=>"Click on the icon in the status bar to view all Web pages in Tabbed windows as thumbnail images.  Press F8 to activate foXpose.",
'{c45c406e-ab73-11d8-be73-000a95be3b12}'=>"Web developer toolbar includes various development tools such as window resizing, form and image debugging, links to page validation and optimization tools and much more.",
'separe@m4ng0.lilik.it'=>"Manage tabs by creating a tab separator.  Right click on a Tab to add a new Tab separator.  Click on the Tab separator to view thumbnail images of web sites that are to the left and right of the Tab separator.",
);

$screenshots = array(
'{097d3191-e6fa-4728-9826-b533d755359d}'=>'all-in-one-mini.png',
'{89506680-e3f4-484c-a2c0-ed711d481eda}'=>'firefox-showcase.png',
'{3CE993BF-A3D9-4fd2-B3B6-768CBBC337F8}'=>'forecastfoxenhanced-small.png',
'{bbc21d30-1cff-11da-8cd6-0800200c9a66}'=>'stickies-small.png',
'Reveal@sourmilk.net'=>'reveal.png',
'{a6ca9b3b-5e52-4f47-85d8-cca35bb57596}'=>'sage.png',
'{53A03D43-5363-4669-8190-99061B2DEBA5}'=>'scrapbook-final.png',
'separe@m4ng0.lilik.it'=>'separe.png',
'xpose@viamatic.com'=>'xpose-small.png',
'{c45c406e-ab73-11d8-be73-000a95be3b12}'=>'web-developer-toolbar-small.png'
);

$authors = array(
'{53A03D43-5363-4669-8190-99061B2DEBA5}'=>'Taiga Gomibuchi',
'separe@m4ng0.lilik.it'=>'Massimo Mangoni',
);

$webpath = WEB_PATH;
$finalists = array();


// Get data for GUIDs.
$finalists_sql = "
            SELECT
                m.guid,
                m.id, 
                m.name, 
                m.downloadcount,
                m.homepage,
                v.dateupdated,
                v.uri,
                v.size,
                v.version,
                (   
                    SELECT u.username          
                    FROM userprofiles u
                    JOIN authorxref a ON u.userid = a.userid
                    WHERE a.id = m.id
                    ORDER BY u.userid DESC
                    LIMIT 1
                ) as username
            FROM
                main m
            JOIN version v ON m.id = v.id
            WHERE
                v.vid = (SELECT max(vid) FROM version WHERE id=m.id AND approved='YES') AND
                type = 'E' AND
                m.guid IN({$guids_imploded})
            ORDER BY
                LTRIM(m.name)
";

$finalists_sql_result = mysql_query($finalists_sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);

while ($row = mysql_fetch_array($finalists_sql_result,MYSQL_ASSOC)) {
    $finalists[$row['guid']] = $row;
}


// Display output.
foreach ($guids as $guid=>$prize) {

$author = !empty($authors[$guid]) ? $authors[$guid] : $finalists[$guid]['username'];

echo <<<finalist
<div class="recommended">
<br class="clear-both"/>
<strong>{$prize}</strong>
<img class="recommended-img" alt="" src="{$webpath}/images/finalists/{$screenshots[$guid]}"/>
<h2><a href="./extensions/moreinfo.php?id={$finalists[$guid]['id']}">{$finalists[$guid]['name']}</a> <small>by {$author}</small></h2>
<div class="recommended-download">
<h3><a href="{$finalists[$guid]['uri']}" onclick="return install(event,'{$finalists[$guid]['name']} {$finalists[$guid]['version']}', '{$webpath}/images/default.png');" title="Install {$finalists[$guid]['name']} {$finalists[$guid]['version']} (Right-Click to Download)">Install Extension ({$finalists[$guid]['size']}KB)</a></h3>
</div>
<p>{$descriptions[$guid]}</p>
</div>
finalist;

}
?>

<br class="clear-both"/>

</div>

<?php
require_once(FOOTER);
?>
