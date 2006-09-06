<?php
require_once('../core/init.php');
require_once('./core/sessionconfig.php');

//If already logged in, we don't need to show the prompt... redirect the user in.
if ($_SESSION["logoncheck"]=="YES") {
header('Location: https://'.HOST_NAME.WEB_PATH.'/developers/main.php');
exit;
}
$page_title = 'Mozilla Update :: Developer Control Panel';
require_once(HEADER);
?>

<hr class="hide">
<div id="mBody">
<div id="mainContent" class="right">

<h2>About the Developer Control Panel</h2>
<P class="first">The Mozilla Update Developer Control Panel allows Extension and Theme authors full access to manage the
listings of their items on Mozilla Update. This includes the ability to add/remove new versions or whole new items,
add/remove screenshots, update the compatibility information and even to report questionable comments left in the item's feedback
to the Mozilla Update Staff.</P>

</div>
<div id="side" class="right">
<h2>Developers Login</h2>

<?php if ($_GET['login']=="failed") { ?>
<strong>You were not successfully logged in. Check your e-mail address and password and try again.</strong>
<?php } else if ($_GET['login']=="unconfirmed") { ?>
<strong>Please check your email for instructions on how to confirm your account.<br><a href="resendconfirmation.php">Click to request a new email confirmation.</a></strong>
<?php }else if ($_GET['logout']=="true") { ?>
<strong>You've been successfully logged out.</strong>
<?php } else {} ?>

<form name="login" method="post" action="./login.php">
<table cellpadding="1" cellspacing="1" style="margin: auto">
<tr><td style="margin-top: 4px"></td></tr>
<tr>
<td><strong>E-Mail:</strong></td><td><input name="email" type="text" size="30"
    maxlength="200"></TD>
</tr>
<tr>
<td><strong>Password:</strong></td><td><input name="password" type="password"
    size="30" maxlength="100"></td>
</tr>
<tr>
<td align="center" colspan="2"><input name="submit" type="submit" value="Login">
    <input type="reset" value="Reset"></td>
</tr>
</table>
</form>
<p>
<a href="./passwordreset.php">Forgot your password?</a><br>
<a href="./resendconfirmation.php">Resend Confirmation Email</a>
</p>

<h2>Create an Account</h2>
<P>You need an account to access the features of the Developer Control Panel and add your extension or themes to Mozilla Update.</P>

<a href="./createaccount.php">Join Mozilla Update!</a>

</div>

</div>

<?php
require_once(FOOTER);
?>
