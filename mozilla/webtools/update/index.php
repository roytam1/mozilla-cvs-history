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
?>
<?php
function renderPopularList($typename) {
  global $titleCaseApp, $application, $uriparams, $connection, $OS;
  $titleCaseType=ucwords($typename);
  $type=$titleCaseType{0};
  echo <<<EOS
    
	<h2><a href="./rss/?application={$application}&amp;type={$type}&amp;list=popular"><img src="./images/rss.png" width="16" height="16" class="rss" alt="Most Popular Additions in RSS"></a><a href="./{$typename}/showlist.php?application={$application}&amp;category=Popular">Most Popular $titleCaseApp $titleCaseType</a></h2>
	<ol class="popularlist">
EOS;
  // Took out the compatibility stuff to avoid a blank front page.
  // `minAppVer_int` <='$currentver' AND `maxAppVer_int` >= '$currentver' AND 
 $sql = "SELECT DISTINCT TM.ID id, TM.Name name, TM.downloadcount dc
         FROM  main TM
         INNER  JOIN version TV ON TM.ID = TV.ID
         INNER  JOIN applications TA ON TV.AppID = TA.AppID
         INNER  JOIN os TOS ON TV.OSID = TOS.OSID
         WHERE  Type  = '$type' AND AppName = '$application' 
         AND (`OSName` = '$OS' OR OSName = 'ALL')
         AND downloadcount > '0' AND approved = 'YES' 
         ORDER BY downloadcount DESC LIMIT 5";
 $sql_result = mysql_query($sql, $connection) 
   or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error(), 
        E_USER_NOTICE);
 if (mysql_num_rows($sql_result)=="0") {
   echo "        <li>No Popular $titleCaseType<li>\n";
 }
 else {
 while ($row = mysql_fetch_array($sql_result)) {
  echo <<<EOS

          <li><a href="{$typename}/moreinfo.php?{$uriparams}&amp;id={$row['id']}"
              >{$row['name']}</a><span class="downloads"> ({$row['dc']} downloads)</span></li>
EOS;
            }
  }
  echo "
        </ol>\n";
}
	
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

require_once(HEADER);

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

// security update
$securitywarning=false;
if ($securitywarning=="true") {
?>

<!-- Don't display if no urgent security updates -->
<div class="key-point">
<p class="security-update"><strong>Important Firefox
Security Update:</strong><br>Lorem ipsum dolor sit amet, 
<a href="#securitydownload">consectetuer adipiscing</a> elit. Curabitur 
viverra ultrices ante. Aliquam nec lectus. Praesent vitae risus. Aenean 
vulputate sapien et leo. Nullam euismod tortor id wisi.
</p>
</div>

<hr class="hide">

<!-- close security update -->
<?php } ?>

<div id="mBody">
	<div id="mainContent" class="right">

	<h2>What can I find here?</h2>
	<dl>
    <dt>Extensions</dt>
    <dd>Extensions are small add-ons that add new functionality to your Firefox
    web browser or Thunderbird email client.  They can add anything from
    toolbars to completely new features.  Browse extensions for:
    <a href="./extensions/?application=firefox">Firefox</a>, 
    <a href="./extensions/?application=thunderbird">Thunderbird</a>,
    <a href="./extensions/?application=mozilla">Mozilla Suite</a>
    </dd>

    <dt>Themes</dt>
    <dd>Themes allow you to change the way your Mozilla program looks. 
    New graphics and colors. Browse themes for: 
    <a href="./themes/?application=firefox">Firefox</a>,
    <a href="./themes/?application=thunderbird">Thunderbird</a>,
    <a href="./themes/?application=mozilla">Mozilla Suite</a>
    </dd>

    <dt>Plugins</dt>
    <dd>Plugins are programs that also add funtionality to your browser to
    deliver specific content like videos, games, and music.  Examples of Plugins
    are Macromedia Flash Player, Adobe Acrobat, and Sun Microsystem's Java
    Software.  Browse plug-ins for:
    <a href="./plugins/">Firefox &amp; Mozilla Suite</a>
    </dd>

    <?php /*
    <dt>Search Engines</dt>
    <dd>In Firefox, you can add search engines that will be available in 
    the search in the top of the browser. Browse search engines for: 
    <a href="/searchengines/">Firefox</a></dd>
    */ ?>

	</dl>

    <?php
    $featuredate = date("Ym");
    $sql = "SELECT TM.ID, TM.Type, TM.Name, TR.Title, TR.Body, TR.ExtendedBody, TP.PreviewURI FROM `main` TM
            INNER JOIN version TV ON TM.ID = TV.ID
            INNER JOIN applications TA ON TV.AppID = TA.AppID
            INNER JOIN os TOS ON TV.OSID = TOS.OSID
            INNER JOIN `reviews` TR ON TR.ID = TM.ID
            INNER JOIN `previews` TP ON TP.ID = TM.ID
            WHERE  `AppName` = '$application' AND `minAppVer_int` <='$currentver' AND `maxAppVer_int` >= '$currentver' AND (`OSName` = '$OS' OR `OSName` = 'ALL') AND `approved` = 'YES' AND TR.featured = 'YES' AND TR.featuredate = '$featuredate' AND TP.preview='YES' LIMIT 1";
        $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
        while ($row = mysql_fetch_array($sql_result)) {
        $id = $row["ID"];
        $type = $row["Type"];
        if ($type=="E") {$typename = "extensions"; } else if ($type=="T") {$typename="themes"; }
        $name = $row["Name"];
        $title = $row["Title"];
        $body = nl2br($row["Body"]);
        $extendedbody = $row["ExtendedBody"];
        $previewuri = $row["PreviewURI"];
        $attr = getimagesize(FILE_PATH.'/'.$previewuri);
        $attr = $attr[3];

    ?>
	<h2>Currently Featuring... <?php echo"$name"; ?></a></h2>
	<a href="./<?php echo"./$typename/moreinfo.php?$uriparams&amp;id=$id"; ?>"><img src="<?php echo"$previewuri"; ?>" <?php echo"$attr"; ?> alt="<?php echo"$name for $application"; ?>" class="imgright"></a>
    <p class="first">
    <strong><a href="./<?php echo"/$typename/moreinfo.php?$uriparams&amp;id=$id"; ?>" style="text-decoration: none"><?php echo"$title"; ?></a></strong><br>
    <?php
    echo"$body";  
    if ($extendedbody) {
        echo" <a href=\"./$typename/moreinfo.php?$uriparams&amp;id=$id&amp;page=staffreview#more\">More...</a>";
    }
    ?></p>
    <?php } ?>
	</div>
	<div id="side" class="right">
    <?php
      renderPopularList("extensions");
      renderPopularList("themes");
    ?>
	
	<h2><a href="./rss/?application=<?php echo $application; ?>&amp;list=newest"><img src="images/rss.png" width="16" height="16" class="rss" alt="News Additions in RSS"></a>New Additions</h2>
	<ol class="popularlist">

        <?php
        // Took out the compatibility stuff to avoid a blank front page.
        // `minAppVer_int` <='$currentver' AND `maxAppVer_int` >= '$currentver' AND 
        $sql = "SELECT TM.ID, TM.Type, TM.Name, MAX(TV.Version) Version, MAX(TV.DateAdded) DateAdded
            FROM  `main` TM
            INNER  JOIN version TV ON TM.ID = TV.ID
            INNER  JOIN applications TA ON TV.AppID = TA.AppID
            INNER  JOIN os TOS ON TV.OSID = TOS.OSID
            WHERE  `AppName` = '$application' AND 
            (`OSName` = '$OS' OR `OSName` = 'ALL')
            AND `approved` = 'YES' 
            GROUP BY TM.ID
            ORDER BY DateAdded DESC LIMIT 8";
        $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error());
        if (mysql_num_rows($sql_result)==0) {
          echo"        <li>Nothing Recently Added</li>\n";
        }
        while ($row = mysql_fetch_array($sql_result)) {
          $id = $row['ID'];
          $typename = $row['Type']=='T'?'themes':'extensions';
          $name = $row['Name'];
          $version = $row['Version'];
          $dateadded = gmdate('M d, Y', strtotime($row['DateAdded'])); 

          echo "		<li>";
          echo "<a href=\"./$typename/moreinfo.php?$uriparams&amp;id=$id\">$name $version</a>";
          echo "<span class=\"downloads\"> ($dateadded)</span>";
          echo "</li>\n";

        }
        ?>
	</ol>
	</div>
</div>
<!-- closes #mBody-->

<?php
require_once(FOOTER);
?>
