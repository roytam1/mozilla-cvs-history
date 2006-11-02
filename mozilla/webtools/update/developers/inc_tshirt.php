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
// Justin "Kickstand" Scott.
// Portions created by the Initial Developer are Copyright (C) 2004
// the Initial Developer. All Rights Reserved.
//
// Contributor(s):
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

    $addons = mysql_query("SELECT * FROM `main` LEFT JOIN `authorxref` ON `main`.`ID`=`authorxref`.`ID` WHERE `authorxref`.`UserID`='{$userid}'");
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
    }
    
    if ($showDetails == true) {
        return $details;
    }
    else {
        return false;
    }
}

?>
