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


//--- Send Request via E-Mail Message ---

$from_name = "Mozilla Update";
$from_address = "update-daemon@mozilla.org";


//Send To Address
$to_address = "$email";

$headers .= "MIME-Version: 1.0\r\n";
$headers .= "Content-type: text/plain; charset=iso-8859-1\r\n";
$headers .= "From: ".$from_name." <".$from_address.">\r\n";
//$headers .= "Reply-To: ".$from_name." <".$from_address.">\r\n";
$headers .= "X-Priority: 3\r\n";
$headers .= "X-MSMail-Priority: Normal\r\n";
$headers .= "X-Mailer: UMO Mail System 1.0";

$subject = "Activate your new Mozilla Update account\n";

	$message = "Welcome to Mozilla Update.\n";
    $message .= "Before you can use your new account you must activate it, this ensures the e-mail address you used is valid and belongs to you.\n";
    $message .= "To activate your account, click the link below or copy and paste the whole thing into your browsers location bar:\n";
    $message .=
    'https://'.HOST_NAME.WEB_PATH.'/developers/createaccount.php?function=confirmaccount&email='.urlencode($email).'&confirmationcode='.$confirmationcode."\n\n";
    $message .= "Keep this e-mail in a safe-place for your records, below is your account details you used when registering for your account.\n\n";
	$message .= "E-Mail: $email\n";
    $message .= "Password: $password_plain\n\n";
    $message .= "Thanks for joining Mozilla Update\n";
    $message .= "-- Mozilla Update Staff\n";

mail($to_address, $subject, $message, $headers);

?>
