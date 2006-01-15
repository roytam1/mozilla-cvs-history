<?php
require_once('../core/init.php');
$page_title = 'Mozilla Update :: Create An Account';
require_once(HEADER);
$function = $_GET['function'];

if (!$function or $function=="step1") {
?>
<hr class="hide">
<div id="mBody">
<div id="mainContent" class="right">

<h2>Create an Account</h2>
<p>Joining Mozilla Update is easy!  Just fill out the form below and click the join button.</p>

<form name="createaccount" method="post" action="createaccount.php?function=step2">
<table border=0 cellpadding=0 cellspacing=0>
<tr><td colspan="2">Your e-mail address is used as your username to login. You'll also receive a confirmation e-mail to this address. In order for your account to be activated succesfully, you must specify a valid e-mail address.</td></tr>
<tr>
<td><strong>E-Mail Address:</strong></td>
<td><input name="email" type="text" size=30></td>
</tr>
<tr>
<td><strong>Confirm E-Mail:</strong></td>
<td><input name="emailconfirm" type="text" size=30></td>
</tr>
<tr><td colspan="2">How do you want to be known to visitors of Mozilla Update? This is your "author name" and it will be shown with your extension/theme listings on the Mozilla Update web site.</td></tr>
<tr>
<td><strong>Your Name</strong></td>
<td><input name="name" type="text" size=30></td>
</tr>
<tr><td colspan="2">If you have a website, enter the URL here. (including the http:// ) Your website will be shown to site visitors on your author profile page. This field is optional; if you don't have a website or don't want it linked to from Mozilla Update, leave this box blank. </td></tr>
<tr>
<td><strong>Your Website</strong></td>
<td><input name="website" type="text" size=30></td>
</tr>
<tr><td colspan="2">Your desired password. This along with your e-mail will allow you to login, so make it something memorable but not easy to guess. Type it in both fields below.  The two fields must match.</td></tr>
<tr>
<td><strong>Password:</strong></td>
<td><input name="password" type="password" size=30></td>
</tr>
<tr>
<td><strong>Confirm Password:</strong></td>
<td><input name="passwordconfirm" type="password" size=30></td>
</tr>
<tr><td colspan="2">Review what you entered above. If everything's correct, click the "Join Mozilla Update" button. If you want to start over, click "Clear Form".</td></tr>
<tr>
<td colspan="2"><input name="submit" type="submit" value="Join Mozilla Update"><input name="reset" type="reset" value="Clear Form"></td>
</tr>
</table>

</form>

</div>
<div id="side" class="right">
<h2>Already Have an Account?</h2>
<P>If you already have signed up for an account, you don't need to sign-up again. Just use your e-mail address and password and <a href="index.php">login</a>.</P>
<P>If you don't remember the password for your account, you can <a href="./passwordreset.php">recover a forgotten password</a>.</P>
</div>

<?php
} else if ($function=="step2") {
echo"<h1>Processing New Account Request, Please Wait...</h1>\n";
//Gather and Filter Data from the Submission Form
if ($_POST["email"]==$_POST["emailconfirm"]) {$email = escape_string($_POST["email"]);} else { $errors="true"; $emailvalid="no";}
if ($_POST["password"]==$_POST["passwordconfirm"]) {$password = escape_string($_POST["password"]);} else { $errors="true"; $passwordvalid="no"; }
if ($_POST["name"]) { $name = escape_string($_POST["name"]); } else { $errors="true"; $namevalid="no"; }
$website = escape_string($_POST["website"]);

// Before doing an unneccessary query for dupes, check to see that the email has a valid format.
if (!preg_match('/^[a-z0-9][-_.a-z0-9]*[@][a-z0-9][-_a-z0-9]*(\.[-_a-z0-9]+)*(\.[a-z]{2,6})$/i',$email)) {
    $errors = 'true';
    $emailvalid = 'bademail';
}

//Check e-mail address and see if its already in use.
if ($emailvalid !="no") {
$sql = "SELECT `UserEmail` from `userprofiles` WHERE `UserEmail`='$email' LIMIT 1";
 $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
  if (mysql_num_rows($sql_result)>"0") {$errors="true"; $emailvalid="no"; }
}

if ($errors == "true") {
echo"<p>Errors have been found in your submission:</p>\n";
echo '<ul>';
if ($emailvalid=="no") {echo"<li>Your e-mail addresses didn't match, or your e-mail is already in use.</li>\n"; }
if ($passwordvalid=="no") {echo"<li>The passwords you entered did not match.</li>\n"; }
if ($namevalid=="no") {echo"<li>The name field cannot be left blank.</li>\n"; }
if ($emailvalid=='bademail') {echo"<li>The email you entered cannot possibly be a valid email.  Please try again.</li>\n";}
echo '</ul>';

require_once(FOOTER);
exit;
}

//We've got good data here, valid password & e-mail. 

//Generate Confirmation Code
$confirmationcode = md5(mt_rand());
$password_plain = $password;
$password = md5($password);

$sql = "INSERT INTO `userprofiles` (`UserName`,`UserEmail`,`UserWebsite`,`UserPass`,`UserMode`,`ConfirmationCode`) VALUES ('$name','$email','$website','$password','D','$confirmationcode');";
 $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
  if ($sql_result) {
    include"mail_newaccount.php";
    echo"Your account has been created successfully. An e-mail has been sent to you with instructions on how to activate your account so you can begin using it.<br>\n";
    echo"<br><br><a href=\"./index.php\">&#171;&#171; Login to Mozilla Update's Developer Control Panel &#187;&#187;</a>";
  }

} else if ($function=="confirmaccount") {
?>
<h2>Activate Your Mozilla Update Account</h2>
<?php
//Get the two URI variables from the query_string..
$email = escape_string($_GET["email"]);
$confirmationcode = escape_string($_GET["confirmationcode"]);

//Check DB to see if those two values match a record.. if it does, activate the account, if not throw error.
$sql = "SELECT `UserID` from `userprofiles` WHERE `UserEmail`='$email' and `ConfirmationCode`='$confirmationcode' LIMIT 1";
  $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
   if (mysql_num_rows($sql_result)=="1") {
     $row = mysql_fetch_array($sql_result);
      $userid = $row["UserID"];
      $sql = "UPDATE `userprofiles` SET `UserMode`='U', `ConfirmationCode`=NULL WHERE `UserID`='$userid' LIMIT 1";
        $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
        if ($sql_result) {
        echo"Thanks! Your account has been activated successfully, you may now login and being using Mozilla Update's Developer Control Panel.";
        echo"<br><br><a href=\"./index.php\">&#171;&#171; Login to Mozilla Update's Developer Control Panel &#187;&#187;</a>";
        }
   } else {
     echo"Sorry, the e-mail and confirmation code do not match, please make sure you've copied the entire URL, if you copy/pasted it from your e-mail client, and try again.";
     echo"<br><br><a href=\"./index.php\">&#171;&#171; Back to Mozilla Update Developer Control Panel Home &#187;&#187;</a>";

   }

?>


<?php
} else {}
?>


</div>

<?php
require_once(FOOTER);
?>
