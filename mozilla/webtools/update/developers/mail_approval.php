<?php
// This message is called to inform the user of action being taken on a queued item listing.

//--- Send via E-Mail Message ---

$from_name = "Mozilla Update";
$from_address = "update-daemon@mozilla.org";


//Send To Address
//Get E-Mail Addresses of the Authors of this item.

$sql = "SELECT `UserEmail` FROM `authorxref` TAX INNER JOIN `userprofiles` TU ON TAX.UserID=TU.UserID WHERE TAX.ID='$id'";
$sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
    while ($row = mysql_fetch_array($sql_result)) {


    $to_address = $row['UserEmail'];

    $headers .= "MIME-Version: 1.0\r\n";
    $headers .= "Content-type: text/plain; charset=iso-8859-1\r\n";
    $headers .= "From: ".$from_name." <".$from_address.">\r\n";
    $headers .= "X-Priority: 3\r\n";
    $headers .= "X-MSMail-Priority: Normal\r\n";
    $headers .= "X-Mailer: UMO Mail System 1.0";

    $subject = "[$name $version] $action_email \n";

    $message = "$name $version - $action_email\n";
    $message .= "Your item, $name $version, has been reviewed by a Mozilla Update editor who took the following action:\n";
    $message .= "$action_email\n\n";

    if ($action == 'approve')
    {
       $message .= "Please Note: It may take up to 30 minutes for your extension to be available for download.\n\n";
    }

    $message .= "Your item was tested by " . $_SESSION['name'] ;
    $message .= " using $testbuild on $testos.\n";
    if ($comments != "") {
      $message .= "Editor's Comments:\n ".stripslashes(html_entity_decode($comments))."\n";
    }
    $message .= "----\n";
    $message .= 'Mozilla Update: https://'.HOST_NAME.WEB_PATH.'/'."\n";

    mail($to_address, $subject, $message, $headers);

}
?>
