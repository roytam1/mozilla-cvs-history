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

// Set this page to read from the SHADOW_DB.
define('USE_SHADOW_DB',true);

require_once('./core/init.php');
$page_title = 'Mozilla Update :: Frequently Asked Questions';
require_once(HEADER);
?>

<div id="mBody">

<h1>Frequently Asked Questions</h1>

<h2>What is Mozilla Update?</h2>
<p>Mozilla Update is the place to get updates and extras for
your <a href="http://www.mozilla.org/">Mozilla</a> products.  This service
has undergone <a href="./update.php">several changes</a> that we hope
will make the site better.  We have re-enabled access to the developers area
and look forward to serving the extension and theme developer community in the
future!  We will be posting frequent 
<a href="./update.php">status updates</a> as to our progress with the 
UMO service.  The best is yet to come!</p>

<h2>How do I get involved?</h2>
<p class="first">We are looking for volunteers to help us with UMO. We are in need of PHP
developers to help with redesigning the site, and people to review extensions
and themes that get submitted to UMO. We especially need Mac and Thunderbird
users. If you are interested in being a part of this exciting project, please
join us in <kbd>#umo</kbd> on <kbd>irc.mozilla.org</kbd> to start getting a feeling for what's up or for a more informal chat.
</p>

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

	</dl>

<?php
$sql = "SELECT `title`, `text` FROM  `faq` WHERE `active` = 'YES' ORDER  BY  `index` ASC, `title` ASC";
$sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);

while ($row = mysql_fetch_array($sql_result)) {
    $title = $row['title'];
    $text = nl2br($row['text']);

    echo "<h2>$title</h2>\n";
    echo "<p>$text</p>\n";
}
?>

<h2>Valid App Versions for Addon Developers</h2>

<table class="appversions">
<tr>
    <th>Application Name {GUID}</th>
    <th>Display Version</th>
    <th>install.rdf Version</th>
</tr>

<?php
// Let's display our valid app versions to make the lives of our appliation
// developers a lot easier.
$appVersions = array();
$guids = array();

$sql = "
    SELECT 
        `AppName`,
        `GUID`,
        `Version`,
        `major`,
        `minor`,
        `release`,
        `SubVer`
    FROM
        `applications`
    WHERE
        `public_ver`='YES'
    ORDER BY
        AppID
";

$sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);

while ($row = mysql_fetch_array($sql_result)) {
    $appVersions[$row['AppName']][] = array(
        'displayVersion' => $row['Version'],
        'versionNumber' => buildAppVersion($row['major'],$row['minor'],$row['release'],$row['SubVer'])
    );
    $guids[$row['AppName']] = $row['GUID'];
}

if (is_array($appVersions)) {
    foreach ($appVersions as $app=>$versions) {
        echo <<<ROWHEADER
            <tr><td colspan="3"><h3>{$app} {$guids[$app]}</h3></td></tr>

ROWHEADER;
        if (is_array($versions)) { 
            $class = 0;
            foreach ($versions as $row) {
                $rowClass = $class%2;
                echo <<<ROW
                <tr class="row{$rowClass}"><td></td><td>{$row['displayVersion']}</td><td>{$row['versionNumber']}</td></tr>

ROW;
                $class++;
            }
        }
    }
}
echo '</table>'

?>

<h2>I see this error when trying to upload my extension or theme: "The Name for your extension or theme already exists in the Update database."</h2>
<p>This is typically caused by mismatching GUIDs or a duplicate record.  If there is a duplicate record, chances are you should submit an update instead of trying to create a new extension or theme.  If you cannot see the existing record, then it is owned by another author, and you should consider renaming your extension/theme.</p>

</div>

<?php
require_once(FOOTER);
?>
