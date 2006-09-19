<?php
require_once('../core/init.php');
require_once('./core/sessionconfig.php');

if ($_SESSION["level"]!="admin" && $_SESSION["level"]!="editor") {
    require_once(HEADER);
    echo '<h1>Access Denied</h1>';
    echo '<p>You do not have access to the Approval Queue.</p>';
    require_once(FOOTER);
    exit;
}

$filename = basename($_SERVER['PATH_INFO']);
$file = REPO_PATH . "/approval/$filename";
header('Content-Description: File Transfer');
header('Content-Type: application/octet-stream');
header('Content-Length: ' . filesize($file));
header('Content-Disposition: attachment; filename=' . $filename);
readfile($file);
?>
