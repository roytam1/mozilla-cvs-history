<?php
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
