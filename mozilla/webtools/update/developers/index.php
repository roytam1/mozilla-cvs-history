<?php
require"../core/config.php";
require"core/sessionconfig.php";

//If already logged in, we don't need to show the prompt... redirect the user in.
if ($_SESSION["logoncheck"]=="YES") {
$return_path="developers/main.php";
header("Location: http://$sitehostname/$return_path");
exit;
}
?>
<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html401/strict.dtd">
<html>
<head>
<link rel="stylesheet" type="text/css" href="/admin/core/mozupdates.css">
<title>Mozilla Update :: Developer Control Panel</title>

<?php
include"$page_header";
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

<?php if ($_GET[login]=="failed") { ?>
<strong>You were not successfully logged in. Check your e-mail address and password and try again.</strong>
<?php } else if ($_GET[logout]=="true") { ?>
<strong>You've been successfully logged out.</strong>
<?php } else {} ?>

<form name="login" method="post" action="login.php">
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
<a href="./passwordreset.php">Forgot your password?</a>

<h2>Create an Account</h2>
<P>You need an account to access the features of the Developer Control Panel and add your extension or themes to Mozilla Update.</P>

<a href="./createaccount.php">Join Mozilla Update!</a>

</div>

</div>

<?php
include"$page_footer";
?>
</body>
</html>
