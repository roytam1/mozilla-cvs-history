<?php
require_once('../core/init.php');
require_once('./core/sessionconfig.php');

session_unset();
session_destroy();

header('Location: https://'.HOST_NAME.WEB_PATH.'/developers/');
exit;
?>
