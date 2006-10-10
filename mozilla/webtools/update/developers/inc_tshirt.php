<?php
require_once('../core/inc_version_comparison.php');

/**
 * Returns the eligibility of the current user to get a tshirt
 * If $showDetails is false, the function will return true or false
 * If $showDetails is true, the function will return an array
 * @param boolean $showDetails
 */
function tshirtEligible($showDetails = false) {
    $userid = mysql_real_escape_string($_SESSION['uid']);
    $details = array();

    if ($_SESSION['level'] == "admin") {
        if ($showDetails == true) {
            $details[] = 'AMO Administrator';
        }
        else {
            return true;
        }
    }

    if ($_SESSION['level'] == "editor") {
        $checkqry = mysql_query("SELECT COUNT(*) AS `num` FROM `approvallog` WHERE `approvallog`.`action`!='Approval?' AND `approvallog`.`date` > '2006-07-09 00:00:00' AND `approvallog`.`UserID`='{$userid}'"); 
        $check = mysql_fetch_array($checkqry);

        if ($check['num'] >= 20) {
            if ($showDetails == true) {
                $details[] = 'Active AMO Reviewer ('.$check['num'].')';
            }
            else {
                return true;
            }
        }
    }

/*    $addons = mysql_query("SELECT * FROM `main` LEFT JOIN `authorxref` ON `main`.`ID`=`authorxref`.`ID` WHERE `authorxref`.`UserID`='{$userid}'");
    while($row = mysql_fetch_array($addons)) {
        $latestversionqry = mysql_query("SELECT * FROM version WHERE ID='".$row['ID']."' AND AppID=1 AND DateAdded<'2006-10-01 00:00:00' AND MaxAppVer!='' ORDER BY DateAdded DESC LIMIT 1");
        $latestversion = mysql_fetch_array($latestversionqry);

        if (mysql_num_rows($latestversionqry) > 0 && NS_CompareVersions($latestversion['MaxAppVer'], '2.0') == 1) {
            if ($showDetails == false) {
                return true;
            }
            else {
                $details[] = $row['Name'].' ('.$latestversion['MaxAppVer'].' - '.$latestversion['DateAdded'].')';
            }
        }
    }*/
    
    if ($showDetails == true) {
        return $details;
    }
    else {
        return false;
    }
}

?>
