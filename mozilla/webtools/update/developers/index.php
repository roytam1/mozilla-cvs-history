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

<h2>Developer Control Panel Locked</h2>
<p class="first">The Developer Control Panel has been locked so we can clear the review queue and migrate our database in preparation for our new site.  For more information, please visit <a href="http://blog.mozilla.com/webdev/">our blog</a>.</p>

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
