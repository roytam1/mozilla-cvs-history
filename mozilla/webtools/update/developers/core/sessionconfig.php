<?php
//session_name('sid');
session_name();
session_start();

if ($_SESSION["logoncheck"] !=="YES" && strpos($_SERVER["SCRIPT_NAME"],"login.php")===false && strpos($_SERVER["SCRIPT_NAME"],"index.php")===false) {
header('Location: http://'.HOST_NAME.WEB_PATH.'/developers/');
exit;
}
?>
