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
// Justin Scott <fligtar@gmail.com>.
// Portions created by the Initial Developer are Copyright (C) 2004
// the Initial Developer. All Rights Reserved.
//
// Contributor(s):
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
require_once('./core/sessionconfig.php');
$page_title = 'Mozilla Update :: Developer Control Panel :: Mass Disable Add-ons';
require_once(HEADER);
require_once('./inc_sidebar.php');

if ($_SESSION['level'] != 'admin') {
    echo "<h1>Access Denied</h1>\n";
    echo "You do not have permission to access this page.";
    require_once(FOOTER);
    exit;
}

if (!empty($_POST['id'])) {
    $addon_ids = explode(',', str_replace(' ', '', mysql_real_escape_string($_POST['id'])));

    if (!empty($_POST['disable']) && !empty($addon_ids)) {
        foreach ($addon_ids as $addon_id) {
            $authors = array();

            $addon_qry = mysql_query("SELECT * FROM main WHERE ID='{$addon_id}'");
            $addon = mysql_fetch_array($addon_qry);
    
            $version_qry = mysql_query("SELECT version.*, os.* FROM version LEFT JOIN os ON version.OSID=os.OSID WHERE version.ID='{$addon_id}' GROUP BY version.URI ORDER BY version.DateAdded DESC");

            $author_qry = mysql_query("SELECT userprofiles.UserName, userprofiles.UserEmail, userprofiles.UserID FROM authorxref LEFT JOIN userprofiles ON authorxref.UserID = userprofiles.UserID WHERE authorxref.ID='{$addon_id}'");


            $email = $_POST['email'];
    
            echo '<h1>Disable '.$addon['Name'].'</h1>';

            if (empty($email)) {
                echo 'Error: Please enter text for the e-mail body.';
                exit;
            }
    
            //Disable all versions
            mysql_query("UPDATE version SET approved='DISABLED' WHERE ID='{$addon_id}'");
            echo 'Versions disabled...<br>';
    
            //Set only author to disabled@addons.mozilla.org account
            mysql_query("DELETE FROM authorxref WHERE ID='{$addon_id}'");
            mysql_query("INSERT INTO authorxref (ID, UserID) VALUES('{$addon_id}', '69324')");
            echo 'Author changed to disabled@addons.mozilla.org...<br>';
    
            //Add notice to beginning of description
            $disabled_notice = "********** NOTICE **********\nThis add-on has been disabled by an administrator.\n****************************\n\n";
            mysql_query("UPDATE main SET description=CONCAT('{$disabled_notice}', description) WHERE ID='{$addon_id}'");
            echo 'Description updated...<br>';
    
            //email authors
            $headers = "From: AMO Administrators <umo-admins@mozilla.org>\r\n";
            $subject = $addon['Name'].' disabled';
            $body = $email."\n\nMozilla Add-ons\nhttps://".HOST_NAME.WEB_PATH."\n";
    
            while ($author = mysql_fetch_array($author_qry)) {
                $authors[] = $author['UserName'].' ('.$author['UserEmail'].' - ID '.$author['UserID'].')';
                mail($author['UserEmail'], $subject, $body, $headers);
                echo $author['UserEmail'].' emailed...<br>';
            }
    
            echo '<b>Add-on disabled successfully.</b><br>';

            $disabled_addons[$addon_id]['name'] = $addon['Name'];
            $disabled_addons[$addon_id]['authors'] = implode(', ', $authors);
            $disabled_addons[$addon_id]['guid'] = $addon['GUID'];

        }
        echo '<hr>';

        //email admins
        if (!empty($disabled_addons)) {
            $body = "Administrator: {$_SESSION['name']} ({$_SESSION['email']})\n\n";
            $body .= "Notes to author:\n";
            $body .= "{$email}\n\n";

            $body .= "The following add-ons were disabled:\n\n";
            foreach ($disabled_addons as $addon_id => $info) {
                $body .= "{$info['name']} ({$info['guid']} - https://".HOST_NAME.WEB_PATH."/disabled/{$addon_id}/)\n";
            $body .= "Authors: ".$info['authors']."\n\n";
            }

            mail('umo-admins@mozilla.org', count($disabled_addons).' add-ons disabled', $body, $headers);
            echo 'umo-admins@mozilla.org emailed...<br>';
        }

        echo '<b>Mass Disable complete.</b><br>';
    }
    elseif (!empty($_POST['preview'])) {
    ?>
        <form method="post">
        <input type="hidden" name="id" value="<?=$_POST['id']?>">
        <h1>Mass Disable Preview</h1>
        The following add-ons and all of their versions will be disabled if you continue:<br>
        <ul>
        <?php
            if (!empty($addon_ids)) {
                foreach ($addon_ids as $addon_id) {
                    $addon_qry = mysql_query("SELECT * FROM main WHERE ID='{$addon_id}'");
                    $addon = mysql_fetch_array($addon_qry);

                    echo "<li><a href=\"../addon.php?id={$addon['ID']}\">{$addon['Name']}</a> (ID {$addon['ID']})</li>";
                }
            }
        ?>
        </ul>
        <br><br>
        The following e-mail will be sent to the authors:<br>
        <blockquote>
            <?=nl2br($_POST['email'])?>
        </blockquote>
        <input type="hidden" name="email" value="<?=$_POST['email']?>">
        <div style="text-align: center;">
            <input type="submit" name="disable" value="Disable Add-ons">
            <input type="button" value="Cancel" onClick="window.location='main.php';">
            <br><br><a href="../author.php?id=69324">See all disabled add-ons</a>
        </div>
    <?php
    }
}
else {
?>
<form method="post">
<h1>Mass Disable</h1>
Enter a comma-separated list of add-on IDs to disable:&nbsp;&nbsp;
<textarea name="id" cols=70 rows=5></textarea>
<br><br>
The following e-mail will be sent to the authors [REQUIRED]:
<textarea name="email" cols=70 rows=10></textarea>
<br>
<div style="text-align: center;">
    <input type="submit" name="preview" value="Preview Mass Disable">
    <input type="button" value="Cancel" onClick="window.location='main.php';">
    <br><br><a href="../author.php?id=69324">See all disabled add-ons</a>
</div>
</form>
<?php
}
?>

<!-- close #mBody-->
</div>

<?php
require_once(FOOTER);
?>
