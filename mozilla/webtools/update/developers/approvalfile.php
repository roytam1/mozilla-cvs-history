<?php
require_once('../core/init.php');
require_once('./core/sessionconfig.php');

if ($_SESSION["level"]=="admin" or $_SESSION["level"]=="editor") {
    //Do Nothing, they're good. :-)
} else {
    echo"<h1>Access Denied</h1>\n";
    echo"You do not have access to the Approval Queue.";
    exit;
}

$filename = basename($_SERVER["PATH_INFO"]);
$file = REPO_PATH . "/approval/$filename";
header('Content-Description: File Transfer');
header('Content-Type: application/octet-stream');
header('Content-Length: ' . filesize($file));
header('Content-Disposition: attachment; filename=' . basename($file));
readfile($file);
?>
