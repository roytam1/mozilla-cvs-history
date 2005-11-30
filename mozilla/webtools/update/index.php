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
$page_title = 'Mozilla Update';
$page_headers = "\n".'
    <link rel="alternate" type="application/rss+xml" 
          title="New '.$titleCaseApp.' Additions"
          href="./rss/?application='.$application.'&amp;list=newest">
'."\n";

$currentTab = 'home';
require_once(HEADER);
installtrigger('extensions');
?>

<div class="split-feature">
    <div class="split-feature-one">
        <div class="feature-download">
            <!-- Feature Image must be 200px wide... any height is fine, but around 170-200 is preferred -->
            <a href="./extensions/moreinfo.php?id=1532"><img src="./images/feature/del.icio.us.png" width="200" height="213" alt="del.icio.us Extension"></a>
            <h3><a href="http://releases.mozilla.org/pub/mozilla.org/extensions/del.icio.us/del.icio.us-1.0.2-fx.xpi" onclick="return install(event,'del.icio.us 1.0.2', '<?=WEB_PATH?>/images/default.png');" title="Install del.icio.us 1.0.2 (Right-Click to Download)">Install Extension (62 KB)</a> </h3>

        </div>
        <h2>Featured Extension</h2>
        <h2><a href="./extensions/moreinfo.php?id=1532">del.icio.us</a></h2>
        <p class="first">Harness the power of social bookmarking right in your browser with the del.icio.us Firefox extension! The del.icio.us extension for Firefox offers everything you need to seamlessly integrate the del.icio.us service with your Firefox browser. <a href="./extensions/moreinfo.php?id=1532">Learn more...</a></p>
    </div>
    <a class="top-feature" href="./recommended.php"><img src="./images/feature-recommend.png" width="213" height="128" style="padding-left: 12px;" alt="We Recommend: See some of our favorite extensions to get you started."></a>
    <div class="split-feature-two">
    <h2><img src="images/title-topdownloads.gif" width="150" height="24" alt="Top 10 Downloads"></h2>

<?php
// Get our most popular list.
$sql = "
    SELECT DISTINCT
        TM.ID,
        TM.Name,
        TM.Description, 
        TM.Rating,
        TM.downloadcount,
        IF(TM.Type='E', 'extensions', 'themes') as Type
    FROM
        `main` TM
    INNER JOIN 
        version TV 
    ON 
        TM.ID = TV.ID
    INNER JOIN 
        applications TA 
    ON 
        TV.AppID = TA.AppID
    INNER JOIN 
	os TOS 
    ON TV.OSID = TOS.OSID
    WHERE
        `AppName` = '{$application}' AND
        `approved` = 'YES'
    ORDER BY
        `downloadcount` DESC,
        `Rating` DESC, 
        `Name` ASC
    LIMIT 5
";

$sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);

if (mysql_num_rows($sql_result)) {
    $top10count = 1;
    echo '<ol class="top-10">';
    while ($row = mysql_fetch_array($sql_result)) {
        echo <<<MP
        <li class="top-10-{$top10count}"><a href="./{$row['Type']}/moreinfo.php?id={$row['ID']}&amp;application={$application}"><strong>{$row['Name']}</strong> {$row['downloadcount']}</a></li>
MP;
        $top10count++;
    }
    echo '</ol>';
}
?>
    </div>
</div>

<form id="front-search" method="get" action="<?=WEB_PATH?>/quicksearch.php" title="Search Mozilla Update">
    <div>
    <label for="q2" title="Search mozilla.org&quot;s sites">search:</label>
    <input type="hidden" name="cof" value="">
    <input type="hidden" name="domains" value="mozilla.org">
    <input type="hidden" name="sitesearch" value="mozilla.org">
    <input id="q2" type="text" name="q" accesskey="s" size="40">
    <select name="section">
          <option value="A">Entire Site</option>
          <option value="E">Extensions</option>
          <option value="T">Themes</option>
    </select>
    <input type="submit" value="Go">
    </div>
</form>

<div class="front-section-left">
    <h2><img src="images/title-browse.gif" width="168" height="22" alt="Browse By Category"></h2>
    <ul>
    <li><a href="./extensions/showlist.php?application=<?=$application?>&amp;category=Popular">Most Popular Add-ons</a></li>
    <li><a href="./extensions/showlist.php?application=<?=$application?>&amp;category=Newest">Recently Added</a></li>
    <li><a href="./extensions/showlist.php?application=<?=$application?>&amp;category=All">All Categories</a></li>
    </ul>
</div>

<?php
/*
<div class="front-section">
    <h2><img src="images/title-recommends.gif" width="181" height="22" alt="Mozilla Recommends"></h2>
    <ul>
    <li><a href="./asa.php">Asa's Picks</a></li>
    <li><a href="./webdeveloper.php">Web Developer Kit</a></li>
    <li><a href="./newsjunkie.php">News Junkie Kit</a></li>
    </ul>
</div>
*/
?>

<div class="front-section-right">
    <h2><img src="images/title-develop.gif" width="152" height="22" alt="Develop Your Own"></h2>
    <ul>
    <li><a href="./developers/">Login to Submit</a></li>
    <li><a href="http://developer.mozilla.org/en/docs/Extensions">Documentation</a></li>
    <li><a href="http://developer.mozilla.org/en/docs/Building_an_Extension">Develop Your Own</a></li>
    </ul>
</div>

<?php
require_once(FOOTER);
?>
