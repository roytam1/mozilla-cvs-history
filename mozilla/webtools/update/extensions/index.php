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

require_once('../core/init.php');
$page_title = 'Mozilla Update :: Extensions - Add Features to Mozilla Software';
$page_headers = '<link rel="alternate" type="application/rss+xml"
   title="New '.ucwords($application).' Extensions Additions"
   href="../rss/?application='.$application.'&amp;type=E&amp;list=newest">';
require_once(HEADER);
?>

<div id="mBody">

    <?php
    $index = 'yes';
    require_once('./inc_sidebar.php');
    ?>

	<div id="mainContent">
	<h2><?php print(ucwords($application)); ?> Extensions</h2>

	<p class="first">Extensions are small add-ons that add new functionality to
<?php print(ucwords($application)); ?>. They can add anything from a toolbar
button to a completely new feature. They allow the application to be customized
to fit the personal needs of each user if they need additional features<?php if
($application !=="mozilla") { ?>, while keeping <?php
print(ucwords($application)); ?> small to download<?php } ?>.</p>

    <?php
    //Get Current Version for Detected Application
    $sql = "SELECT `Version`, `major`, `minor`, `release`, `SubVer` FROM `applications` WHERE `AppName` = '$application' AND `public_ver` = 'YES'  ORDER BY `major` DESC, `minor` DESC, `release` DESC, `SubVer` DESC LIMIT 1";
    $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
        $row = mysql_fetch_array($sql_result);
            $version = $row["Version"];
            $subver = $row["SubVer"];
            $release = "$row[major].$row[minor]";
            if ($row["release"]) {
                $release = "$release.$row[release]";
            }
        $currentver = $release;
        $currentver_display = $version;
        unset($version,$subver,$release);

/**
 * Turn off Top Rated until comment spam is better regulated.
 * See: https://bugzilla.mozilla.org/show_bug.cgi?id=278016
 * @TODO Fix comment spam.
 */

/* TURN OFF TOP RATED

<h2>Top Rated <?php print(ucwords($application)); ?> Extensions</h2>
<p class="first">Ratings are based on feedback from people who use these extensions.</p>

<?php
$sql = "
    SELECT DISTINCT
        TM.ID,
        TM.Name,
        TM.Description, 
        TM.Rating
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
    WHERE
        `Type` = '{$type}' AND
        `AppName` = '{$application}' AND
        `approved` = 'YES' 
    ORDER BY
        `Rating` DESC, 
        `downloadcount` DESC,
        `Name` ASC
    LIMIT 5
";

$sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);

if (!mysql_num_rows($sql_result)) {
    echo '<p>No featured extensions were found.  Please check back later.</p>';
} else {
    echo '<ol>';
    while ($row = mysql_fetch_array($sql_result)) {
echo <<<TR
        <li>
        <a href="./moreinfo.php?id={$row['ID']}&amp;application={$application}"><strong>{$row['Name']}</strong></a>, {$row['Rating']} stars<br>
        {$row['Description']}
        </li>
TR;
    }
    echo '</ol>';
}
*/
?>
<h2>
<a href="../rss/?application=<?php echo"$application"; ?>&amp;type=E&amp;list=popular"><img src="../images/rss.png" width="16" height="16" class="rss" alt="Most Popular Additions in RSS"></a>
<?php $catname = "Popular"; echo "<a href=\"showlist.php?".uriparams()."&amp;category=$catname&amp;numpg=10&amp;pageid=2\" title=\"$catdesc\">"; ?>
Most Popular <?php print(ucwords($application)); ?> Extensions</a></h2>
<p class="first">The most popular downloads over the last week.</p>

<?php
$sql = "
    SELECT DISTINCT
        TM.ID,
        TM.Name,
        TM.Description, 
        TM.Rating,
        TM.downloadcount
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
        `Type` = '{$type}' AND
        `AppName` = '{$application}' AND
        (`OSName` = '$OS' OR `OSName` = 'ALL') AND
        `approved` = 'YES'
    ORDER BY
        `downloadcount` DESC,
        `Rating` DESC, 
        `Name` ASC
    LIMIT 10
";

$sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);

if (!mysql_num_rows($sql_result)) {
    echo '<p>No featured extensions were found.  Please check back later.</p>';
} else {
    echo '<ol>';
    while ($row = mysql_fetch_array($sql_result)) {
echo <<<MP
        <li>
        <a href="./moreinfo.php?id={$row['ID']}&amp;application={$application}"><strong>{$row['Name']}</strong></a>,
        ({$row['Rating']} stars, {$row['downloadcount']} downloads)<br>
        {$row['Description']}
        </li>
MP;
    }
    echo '</ol>';
}
?>
<h2>
<a href="../rss/?application=<?php echo"$application"; ?>&amp;type=E&amp;list=newest"><img src="../images/rss.png" width="16" height="16" class="rss" alt="News Additions in RSS"></a>
<?php $catname = "Newest"; echo "<a href=\"showlist.php?".uriparams()."&amp;category=$catname&amp;numpg=10&amp;pageid=2\" title=\"$catdesc\">"; ?>
Newest <?php print(ucwords($application)); ?> Extensions</a></h2>
<p class="first">New and updated extensions. Subscribe to <a href="../rss/?application=<?php echo"$application"; ?>&amp;type=E&amp;list=newest">our RSS feed</a> to be notified when new extensions are added.</p>

<?php
$sql = "
    SELECT DISTINCT
        TM.ID, 
        TM.Type, 
        TM.Description, 
        TM.Name, 
        TV.Version, 
        TV.DateAdded
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
    INNER
        JOIN os TOS
    ON
        TV.OSID = TOS.OSID
    WHERE  
        `Type`='E' AND 
        `AppName` = '$application' AND 
        (`OSName` = '$OS' OR `OSName` = 'ALL') AND 
        `approved` = 'YES'
    ORDER BY
        DateAdded DESC
    LIMIT 10
";

$sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);

if (!mysql_num_rows($sql_result)) {
    echo '<p>Nothing recently added.  Please try again later.</p>';
} else {
    echo '<ol>';
    while ($row = mysql_fetch_array($sql_result)) {
        $row['DateAdded'] = gmdate('F d, Y', strtotime($row['DateAdded']));
echo <<<MP
        <li>
        <a href="./moreinfo.php?id={$row['ID']}&amp;application={$application}"><strong>{$row['Name']} {$row['Version']}</strong></a>,
        {$row['DateAdded']}<br>
        {$row['Description']}
        </li>
MP;
    }
    echo '</ol>';
}
?>

	</div>
</div>

<!-- closes #mBody-->

<?php
require_once(FOOTER);
?>
