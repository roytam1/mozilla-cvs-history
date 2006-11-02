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
//   Mike Morgan <morgamic@gmail.com>
//   Justin Scott <fligtar@gmail.com>
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
$page_title = 'Mozilla Update :: Resend Confirmation';
require_once(HEADER);
?>
<hr class="hide">
<div id="mBody">

<?php
if ($_POST['submit']=='Request New Confirmation Email') {
    echo "<h1>Resending confirmation email Please Wait...</h1>\n";
    //Gather and Filter Data from the Submission Form
    $email = escape_string($_POST['email']);
    //Generate Confirmation Code
    $confirmationcode = md5(mt_rand());

    $sql = "SELECT `UserEmail`, `UserMode` FROM `userprofiles` WHERE `UserEmail`='$email' LIMIT 1";
    $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
    if (mysql_num_rows($sql_result) == 1) {
        $row = mysql_fetch_array($sql_result);
        if ($row['UserMode'] !== 'D') {
            echo "<p>Your account has already been confirmed, so new new confirmation code has been generated.</p>";
        } else {
           $newpassword = substr(md5(mt_rand()),0,14);
		   $password_plain = $newpassword;
           $password = md5($newpassword);
           $sql = "UPDATE `userprofiles` SET `ConfirmationCode`='$confirmationcode', `UserPass`='$password' WHERE `UserEmail`='$email' LIMIT 1";
           $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
           if ($sql_result) {
               require_once('./mail_newaccount.php');
               echo"Your confirmation code has been reset successfully. However, as a consequence of this your password has changed. An e-mail has been sent to you to confirm your account and inform you of the new password.<br>\n";
               echo"<br><br><a href=\"./index.php\">&#171;&#171; Login to Mozilla Update's Developer Control Panel &#187;&#187;</a>";
           }
        }
    } else {
       echo"An error was encountered when trying to resend your confirmation email, verify the e-mail you entered is correct and try again.<br>\n";
    }
    require_once(FOOTER);
    exit;
}
?>
<h2>Resend Confirmation</h2>
<P>If you have deleted the confirmation email, then you can have a new one sent to you by filling in the form below. Just put your e-mail address in the form below,
and a new password and confirmation code will be e-mailed to the e-mail address you have on file.</P>

<form name="resetpassword" method="post" action="?post=getconfirmation">
<table border=0 cellpadding=0 cellspacing=0 style="width: 50%; margin: auto;">
<tr>
<td style="height: 30px;">E-Mail Address:</td>
<td><input name="email" type="text" size=30></td>
</tr>
<tr>
<td colspan=2 align=center><input name="submit" type="submit" value="Request New Confirmation Email"><input name="reset" type="reset" value="Clear Form"></td>
</tr>
</table>

</form>

</div>

<?php
require_once(FOOTER);
?>
