<?php
require_once('../core/inc_version_comparison.php');

function tshirtEligible() {
    $userid = $_SESSION['uid'];
    
    if ($_SESSION['level'] == "admin" || $_SESSION['level'] == "editor") {
        return true;
    }

    $addons = mysql_query("SELECT * FROM authorxref WHERE UserID='{$userid}'");
    while($row = mysql_fetch_array($addons)) {
        $latestversionqry = mysql_query("SELECT * FROM version WHERE ID='".$row['ID']."' AND AppID=1 AND DateAdded<'2006-10-01 00:00:00' AND MaxAppVer!='' ORDER BY DateAdded DESC LIMIT 1");
        $latestversion = mysql_fetch_array($latestversionqry);

        if (mysql_num_rows($latestversionqry) > 0 && NS_CompareVersions($latestversion['MaxAppVer'], '2.0') == 1) {
            return true;
        }
    }
    
    return false;
}

?>