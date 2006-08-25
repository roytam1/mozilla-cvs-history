<?php
require_once('../core/init.php');
require_once('./core/sessionconfig.php');

if ($_SESSION["level"]=="admin" or $_SESSION["level"]=="editor") {
    //Do Nothing, they're good. :-)
} else {
    require_once(HEADER);
    echo"<h1>Access Denied</h1>\n";
    echo"You do not have access to the Approval Queue.";
    require_once(FOOTER);
    exit;
}

$filename = stripslashes($_GET['file']);
$file = REPO_PATH . "/approval/$filename";
if (file_exists($file)) {
    header('Content-Description: File Transfer');
    header('Content-Type: application/octet-stream');
    header('Content-Length: ' . filesize($file));
    header('Content-Disposition: attachment; filename=' . basename($file));
    readfile($file);
} else {
    require_once(HEADER);
    echo"<h1>File Not Found</h1>\n";
    echo"The file you requested could not be found.";
    require_once(FOOTER);
    exit;
}
?>
