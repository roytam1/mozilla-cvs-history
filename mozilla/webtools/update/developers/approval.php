<?php
require_once('../core/init.php');
require_once('./core/sessionconfig.php');
$function = $_GET['function'];
$page_title = 'Mozilla Update :: Developer Control Panel :: Manage Approval Queue';
require_once(HEADER);
$skipqueue='true';
require_once('./inc_sidebar.php');

if ($_SESSION["level"]=="admin" or $_SESSION["level"]=="editor") {
    //Do Nothing, they're good. :-)
} else {
    echo"<h1>Access Denied</h1>\n";
    echo"You do not have access to the Approval Queue.";
    require_once(FOOTER);
    exit;
}

if (!$function || $function == "approvalqueue") {
//Overview page for admins/editors to see all the waiting approval queue items...

  if ($_POST["submit"] == "Submit") {
    include "inc_approval.php"; //Get the resuable process_approval() function.

    echo "<h2>Processing changes to approval queue, please wait...</h2>\n";
    //echo"<pre>"; print_r($_POST); echo"</pre>\n";

    for ($i = 1; $_POST["maxvid"] >= $i; $i++) {
      $type = escape_string($_POST["type_$i"]);
      $comments = escape_string($_POST["comments_$i"]);
      $approval = escape_string($_POST["approval_$i"]);
      $file = escape_string($_POST["file_$i"]);
      $testos = escape_string($_POST["testos_$i"]);
      $testbuild = escape_string($_POST["testbuild_$i"]);

      $installation = $_POST["installation_$i"] ? escape_string($_POST["installation_$i"]) : "NO";
      $uninstallation = $_POST["uninstallation_$i"] ? escape_string($_POST["uninstallation_$i"]) : "NO";
      $appworks = $_POST["appworks_$i"] ? escape_string($_POST["appworks_$i"]) : "NO";
      $cleanprofile = $_POST["cleanprofile_$i"] ? escape_string($_POST["cleanprofile_$i"]) : "NO";

      if ($type == "E") {
        $newchrome = $_POST["newchrome_$i"] ? escape_string($_POST["newchrome_$i"]) : "NO";
        $worksasdescribed = $_POST["worksasdescribed_$i"] ? escape_string($_POST["worksasdescribed_$i"]) : "NO";
      } elseif ($type == "T") {
        $visualerrors = $_POST["visualerrors_$i"] ? escape_string($_POST["visualerrors_$i"]) : "NO";
        $allelementsthemed = $_POST["allelementsthemed_$i"] ? escape_string($_POST["allelementsthemed_$i"]) : "NO";
      }

      if ($approval == "YES" || $approval == "NO") {
        $name = escape_string($_POST["name_$i"]);
        $version = escape_string($_POST["version_$i"]);
        if ($type == "T") {
          if ($approval == "YES") {
            if ($installation == "YES" && $uninstallation == "YES" && $appworks == "YES" && $cleanprofile == "YES" && $visualerrors == "YES" && $allelementsthemed == "YES" && $testos && $testbuild) {
              $approval_result = process_approval($type, $file, "approve");
            } else {
              echo "Error: Approval of $name $version cannot be processed because of missing data. Fill in the required fields and try again.<br>\n";
            }
          } elseif ($comments) {
            $approval_result = process_approval($type, $file, "deny");
          } else {
            echo"Error: Denial of $name $version cannot be processed because of missing data. Fill in the required fields and try again.<br>\n";
          }
        } else if ($type == "E") {
          if ($approval == "YES") {
            if ($installation == "YES" && $uninstallation == "YES" && $appworks == "YES" && $cleanprofile == "YES" && $newchrome == "YES" && $worksasdescribed == "YES" && $testos && $testbuild) {
              $approval_result = process_approval($type, $file, "approve");
            } else {
              echo "Error: Approval of $name $version cannot be processed because of missing data. Fill in the required fields and try again.<br>\n";
            }
          } elseif ($comments) {
            $approval_result = process_approval($type, $file, "deny");    
          } else {
            echo"Error: Denial of $name $version cannot be processed because of missing data. Fill in the required fields and try again.<br>\n";
          }
        }
      }

      //Approval for this file was successful, print the output message.
      if ($approval_result) {
        if ($approval == "YES") {
          echo "$name $version was granted approval<br>\n";
        } elseif ($approval == "NO") {
          echo "$name $version was denied approval<br>\n";
        }    
      }
    }  
  }

?>

<h1>Extensions/Themes Awaiting Approval</h1>
<div style="margin-left: 20px; font-size: 8pt;">
    <a href="?function=approvalhistory">Approval Log History</a>
    <?=($_SESSION["level"] == "admin") ? " | <a href=\"responsemanager.php\">Canned Response Manager</a>" : ""?>
</div>
<form name="approvalqueue" method="post" action="?">
<?php
// Get canned responses available for all addons
$sql = "SELECT `CannedAction`, `CannedName`, `CannedResponse` FROM `canned_responses` ORDER BY `CannedAction`, `CannedName`";
$sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
while($row = mysql_fetch_array($sql_result)) {
     $cannedResponses[] = $row;
}

$i = 0;
// Get main info about extensions pending approval
$sql = "SELECT TM.ID, `Type`, `vID`, `Name`, `Description`, `ReviewNotes`, `Homepage`, TV.Version, `OSName`, `URI` FROM `main` TM
INNER JOIN `version` TV ON TM.ID = TV.ID
INNER JOIN `os` TOS ON TV.OSID=TOS.OSID
WHERE `approved` = '?' GROUP BY TV.URI ORDER BY TV.DateUpdated ASC";
$sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
$num_results = mysql_num_rows($sql_result);
while ($row = mysql_fetch_array($sql_result)) {
    $i++;
    $id = $row["ID"];
    $type = $row["Type"];
    $uri = $row["URI"];
    $reviewnotes = $row["ReviewNotes"];
    
    // Get author information
    $authors = ""; $j = ""; $authorWebsites = "";
    $authid = array();
    $sql2 = "SELECT `UserName`, `UserWebsite`, TAX.`UserID` from `authorxref` TAX INNER JOIN `userprofiles` TU ON TAX.UserID = TU.UserID WHERE TAX.ID='$row[ID]' ORDER BY `UserName` ASC";
    $sql_result2 = mysql_query($sql2, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
    $num_results2 = mysql_num_rows($sql_result2);
    while ($row2 = mysql_fetch_array($sql_result2)) {
        $j++;
        $authid[] = $row2["UserID"];
        $authors .= $row2["UserName"];
        if ($num_results2 > $j) { $authors .= ", "; }
        if($row2["UserWebsite"] != "") {
            $authorWebsite = $row2["UserWebsite"];
            if(strpos($authorWebsite, "://") === false) { $authorWebsite = "http://".$authorWebsite; }
            if($num_results2 == 1) {
                $authorWebsites = " | <a href=\"$authorWebsite\">Author Homepage</a>";
            } else {
                $authorWebsites .= " | <a href=\"$authorWebsite\">".$row2["UserName"]." Homepage</a>";
            }
        }
    }

    // Get category information
    $categories = ""; $j = "";
    $sql2 = "SELECT `CatName` from `categoryxref` TCX INNER JOIN `categories` TC ON TCX.CategoryID = TC.CategoryID WHERE TCX.ID='$row[ID]' GROUP BY `CatName` ORDER BY `CatName` ASC";
    $sql_result2 = mysql_query($sql2, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
    if (mysql_num_rows($sql_result2) == 1) {$categories = "<strong>Category:</strong> "; } else { $categories = "<strong>Categories:</strong> "; }
    while ($row2 = mysql_fetch_array($sql_result2)) {
        $j++;
        $categories .= $row2["CatName"];
        if (mysql_num_rows($sql_result2) > $j) { $categories .= ", "; } 
    }

    // Get preview information
    $sql2 = "SELECT `PreviewID` FROM `previews` WHERE `ID`='$id' AND `preview`='YES' LIMIT 1";
    $sql_result2 = mysql_query($sql2, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
    if (mysql_num_rows($sql_result2) == 1) {
        $listpreview = " | <a href=\"previews.php?id=$id\">View Previews</a>";
    } else {
        $listpreview = " | <span class=\"tooltip\" title=\"Previews are REQUIRED for Themes and recommended for Extensions\">No Previews</span>";
    }

    // Get requester information
    $sql2 = "SELECT `UserName`,`UserEmail`,`date` FROM `approvallog` TA INNER JOIN `userprofiles` TU ON TA.UserID = TU.UserID WHERE `ID`='$row[ID]' AND `vID`='$row[vID]' and `action`='Approval?' ORDER BY `date` DESC LIMIT 1";
    $sql_result2 = mysql_query($sql2, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
    $row2 = mysql_fetch_array($sql_result2);
    if ($row2["date"]) { $date = $row2["date"]; } else { $date = $row["DateUpdated"]; } 
    $date = date("l, F d, Y, g:i:sa", strtotime($date));

    // Previous versions?
    $sql3 = "SELECT `ID` FROM `version` WHERE `ID`='{$id}' AND `URI`!='" . escape_string($row['URI']) . "'";
    $sql_result3 = mysql_query($sql3, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
    if(mysql_num_rows($sql_result3) > 0) {
        $updatingAddon = true;
    } else {
        $updatingAddon = false;
    }

    // Start of extension listing  
    echo "<TABLE BORDER=0 CELLPADDING=1 CELLSPACING=0 ALIGN=CENTER STYLE=\"border: 0px; width: 100%;\">\n";

    echo "<TR><TD><h2><a href=\"../addon.php?id=$id\">$row[Name] $row[Version]</a> by $authors&nbsp;\n";

    // Icons
    echo "&nbsp;&nbsp;";
    if($type == "E") {
        echo "<img src=\"../images/icons/extension.png\" title=\"This is an Extension\" style=\"vertical-align: middle;\">\n";
    } elseif($type == "T") {
        echo "<img src=\"../images/icons/theme.png\" title=\"This is a Theme\" style=\"vertical-align: middle;\">\n";
    }

    if($updatingAddon == true) {
        echo "<img src=\"../images/icons/update.png\" title=\"This is an update for an existing add-on\" style=\"vertical-align: middle;\">\n";
    } else {
        echo "<img src=\"../images/icons/new.png\" title=\"This is a new add-on\" style=\"vertical-align: middle;\">\n";
    }

    // Get OS information
    $sql3 = "SELECT `AppName`, `shortname`, `MinAppVer`, `MaxAppVer` FROM `version` TV INNER JOIN `applications` TA ON TV.AppID = TA.AppID WHERE `URI`='" . escape_string($row['URI']) . "' ORDER BY `AppName` ASC";
    $sql_result3 = mysql_query($sql3, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);

    // Array of applications this item is associated with.
    $apps = array(); $compatability = ""; $j = 0;

    while ($row3 = mysql_fetch_array($sql_result3)) {
        // Built compatability string with version numbers for use later
        $appstring = "<span class=\"tooltip\" title=\"{$row3['AppName']}\">".ucwords(strtolower($row3['shortname']))."</span> {$row3['MinAppVer']}-{$row3['MaxAppVer']} \n";

        // Output application's icon
        if(strpos($compatability, $row3["AppName"]) === false) {
            echo "<img src=\"../images/icons/".strtolower($row3["AppName"]).".png\" title=\"This add-on works with ".$row3["AppName"]."\" style=\"vertical-align: middle;\">\n";
        }

        // Throw application compatibility into array.
        $apps[$j]["display"] = $appstring;
        $apps[$j]["value"] = $row3["AppName"]." ".$row3["MinAppVer"]."-".$row3["MaxAppVer"];
        $compatability .= $appstring;
        $j++;
    }
    
    echo "</h2></TD><TR>\n";


    // Links TR
    echo "<TR><TD>";
    echo "<span style=\"font-size: 8pt;\">";

     // Escape each instance of name/uri for javascript.
    if ($type == "T") {
        // Show Themes install link.
        echo "<a href=\"javascript:void(InstallTrigger.installChrome(InstallTrigger.SKIN,'".addslashes($row['URI'])."','".addslashes($row['Name']." ".$row['Version'])."'))\">Install Now</a>\n";
    } else {
        // Show extensions install link.
        echo "<a href=\"javascript:void(InstallTrigger.install({'".addslashes($row['Name'].$row['Version'])."':'".addslashes($row['URI'])."'}))\">Install Now</a>\n";
    }

    // Show a download now link.
    echo " | <a href=\"".$row['URI']."\">Download Now</a>";

    // Show previews link
    echo $listpreview;

    // Only admins can edit versions
    if($_SESSION["level"] == "admin") {
        echo " | <a href=\"listmanager.php?function=editversion&id=$id&vid={$row[vID]}\">Edit Version</a>";
    }

    // Show these links only if the addon has other versions
    if($updatingAddon == true) {
        echo " | <a href=\"itemhistory.php?id=$id\">Item History</a>";
    }
    echo "<br>\n";
    if($updatingAddon == true) {
        echo "<a href=\"../addon.php?id=$id\">Item Overview</a> | ";
    }

    if($row["Homepage"] != "") {
        echo "<a href=\"".$row["Homepage"]."\">Item Homepage</a>";
    }
    echo $authorWebsites;

    echo "</span>\n";
    echo "</TD></TR>\n";

    echo "<TR><TD style=\"font-size: 8pt;\"><strong>Works with:</strong> $compatability";

    if($row["OSName"] != "ALL") { echo " (<strong>".$row["OSName"]."</strong>)"; }
  
    echo "</TD></TR>";

    echo "<TR><TD style=\"font-size: 8pt;\">".nl2br($row[Description])."</TD></TR>\n";

    echo "<TR><TD style=\"font-size: 8pt;\">$categories</TD></TR>\n";

    echo "<TR>";
    if ($row2[UserName]) {
        echo "<TD COLSPAN=2 style=\"font-size: 8pt;\"><strong>Requested by:</strong> <a href=\"mailto:$row2[UserEmail]\">$row2[UserName]</a> on $date</TD>";
    } else {
        echo "<TD COLSPAN=2></TD>";
    }
    echo "</TR>\n";
  
    if($reviewnotes != "") {
        echo "<TR><TD COLSPAN=2 style=\"font-size: 8pt;\"><strong>Notes to Reviewer:</strong> ".nl2br($reviewnotes)."</TD></TR>\n";
    }

    // Author cannot approve their own work unless they are an admin
    if (!in_array($_SESSION['uid'], $authid) || $_SESSION["level"] == "admin") {

        // Approval Form for this item
        echo "<TR id=\"showform_$i\"><TD><a href=\"javascript:void(0);\" onClick=\"showForm('$i');\">Show Approval Form &raquo;</a></TD></TR>";
        echo "</TABLE>";

        echo "<TABLE BORDER=0 CELLPADDING=1 CELLSPACING=0 id=\"form_$i\" ALIGN=CENTER STYLE=\"display: none; border: 1px solid #CCC; width: 100%;\">";

        echo "<TR BGCOLOR=\"#DDDDFF\"><TD WIDTH=\"75%\" style=\"border-bottom: 1px solid #CCC;\">Add-on Review Information:</TD><TD style=\"border-bottom: 1px solid #CCC; text-align: right; font-size: 8pt;\"><a href=\"#submitjump\">Jump to Submit</a></TD></TR>\n";

        echo "<input name=\"type_$i\" type=\"hidden\" value=\"$type\">\n";
        echo "<input name=\"file_$i\" type=\"hidden\" value=\"$uri\">\n";
        echo "<input name=\"name_$i\" type=\"hidden\" value=\"{$row['Name']}\">\n";
        echo "<input name=\"version_$i\" type=\"hidden\" value=\"{$row['Version']}\">\n";

        // Action
        echo "<TR><TD COLSPAN=4 style=\"font-size: 8pt\"><strong>Action:</strong> \n";
        echo "<input name=\"approval_$i\" type=\"radio\" value=\"YES\">Approve&nbsp;&nbsp;";
        echo "<input name=\"approval_$i\" type=\"radio\" value=\"NO\">Deny&nbsp;&nbsp;";
        echo "<input name=\"approval_$i\" type=\"radio\" checked=\"checked\" VALUE=\"noaction\">No Action\n";
        echo "</TD></TR>\n";

        // Comments
        echo "<TR><TD COLSPAN=2 style=\"font-size: 8pt;\">";
        echo "<strong>Comments:</strong> <textarea name=\"comments_$i\" id=\"comments_$i\" rows=3 cols=50 style=\"vertical-align: top;\"></textarea>"; 
        echo "</TD></TR>\n";

        echo "<TR><TD COLSPAN=2 style=\"font-size: 8pt;\">";
        echo "or select a canned response: ";
        echo "<select onchange=\"document.getElementById('comments_$i').value=this.value;\">";
        echo "<option value=\"\"></option>";
        for($j = 0; $j < count($cannedResponses); $j++) {
            echo "<option value=\"".$cannedResponses[$j]["CannedResponse"]."\">[".(($cannedResponses[$j]["CannedAction"] == "+") ? "APPROVE" : "DENY")."] ".$cannedResponses[$j]["CannedName"]."</option>";
        }
        echo "</select>";
        echo "</TD></TR>";

        echo "<TR><TD COLSPAN=2 style=\"font-size: 8pt;\">";
        echo "<strong>Add-on Testing: </strong>&nbsp;&nbsp;<a href=\"javascript:void(0);\" id=\"checkall_$i\" onClick=\"checkAll('$i');\">Check All</a>";
        echo "</TD></TR>";

        echo "<TR><TD COLSPAN=2 style=\"font-size: 8pt\">\n";
        echo "<label for=\"installation_$i\"><input name=\"installation_$i\" id=\"installation_$i\" type=\"checkbox\" value=\"YES\" style=\"vertical-align: middle;\"><span class=\"tooltip\" TITLE=\"Installs OK?\">Install?</span></label>\n";
        echo "<label for=\"uninstallation_$i\"><input name=\"uninstallation_$i\" id=\"uninstallation_$i\" type=\"checkbox\" value=\"YES\" style=\"vertical-align: middle;\"><span class=\"tooltip\" TITLE=\"Uninstalls OK?\">Uninstall?</span></label>\n";
        echo "<label for=\"appworks_$i\"><input name=\"appworks_$i\" id=\"appworks_$i\" type=\"checkbox\" value=\"YES\" style=\"vertical-align: middle;\"><span class=\"tooltip\" TITLE=\"App works OK? (Loading pages/messages, Tabs, Back/Forward)\">App Works?</span></label>\n";
        echo "<label for=\"cleanprofile_$i\"><input name=\"cleanprofile_$i\" id=\"cleanprofile_$i\" type=\"checkbox\" value=\"YES\" style=\"vertical-align: middle;\"><span class=\"tooltip\" TITLE=\"Using a clean profile? (I.E. Works with no major extensions installed, like TBE)\">Clean Profile?</span></label>\n";
        if ($type == "E") {
            echo "<label for=\"newchrome_$i\"><input name=\"newchrome_$i\" id=\"newchrome_$i\" type=\"checkbox\" value=\"YES\" style=\"vertical-align: middle;\"><span class=\"tooltip\" TITLE=\"Extension added chrome to the UI?\">New Chrome?</span></label>\n";
            echo "<label for=\"worksasdescribed_$i\"><input name=\"worksasdescribed_$i\" id=\"worksasdescribed_$i\" type=\"checkbox\" value=\"YES\" style=\"vertical-align: middle;\"><span class=\"tooltip\" TITLE=\"Item works as author describes\">Works?</span></label>\n";
        } elseif ($type == "T") {
            echo "<label for=\"visualerrors_$i\"><input name=\"visualerrors_$i\" id=\"visualerrors_$i\" type=\"checkbox\" value=\"YES\" style=\"vertical-align: middle;\"><span class=\"tooltip\" TITLE=\"No visual errors/rendering problems\">Visual Errors?</span></label>\n";
            echo "<label for=\"allelementsthemed_$i\"><input name=\"allelementsthemed_$i\" id=\"allelementsthemed_$i\" type=\"checkbox\" value=\"YES\" style=\"vertical-align: middle;\"><span class=\"tooltip\" TITLE=\"All components themed? (Including no missing icons?)\">Theme Complete?</span></label>\n";
        }
        echo "</TD></TR>\n";

        // Operating Systems
        echo "<TR><TD COLSPAN=2 style=\"font-size: 8pt;\">\n";
        echo "<strong>Operating Systems:</strong> <input type=\"text\" name=\"testos_$i\" size=\"40\"></TD></TR>\n";

        // Applications
        echo "<TR><TD COLSPAN=2 style=\"font-size: 8pt;\">";
        echo "<strong>Applications:</strong> <input type=\"text\" name=\"testbuild_$i\" size=\"40\"></TD></TR>\n";

        echo "</TABLE>";
    }
}

echo "<input name=\"maxvid\" id=\"maxvid\" type=\"hidden\" value=\"$i\">\n";


echo "<TABLE>";
if ($num_results > 0) {
    echo "<TR><TD COLSPAN=4 style=\"height: 8px\"></td></tr>";
    echo "<TR><TD COLSPAN=4><img src=\"../images/faq_small.png\" border=0 height=16 width=16 alt=\"\"> Before pressing submit, please make sure all the information you entered above is complete and correct. For themes, a preview screenshot is required for approval. A preview image is recommended for extensions.</TD></TR>";
    echo "<TR><TD COLSPAN=4 ALIGN=\"center\"><a name=\"submitjump\"><input name=\"submit\" type=\"submit\" value=\"Submit\">&nbsp;&nbsp;<input name=\"reset\" type=\"reset\" value=\"Reset\"></a></TD></TR>";
} else {
    echo "<TR><TD COLSPAN=4 ALIGN=\"center\">No items are pending approval at this time</TD></TR>";
}
?>
</TABLE>
</form>
<?php 
} elseif ($function == "approvalhistory") {
?>
<style type="text/css">
TD { font-size: 8pt }
</style>
<h1>Approval History Log</h2>
<span style="font-size: 8pt;">Incomplete Basic UI for the Approval History Log. <a href="https://bugzilla.mozilla.org/show_bug.cgi?id=255305">Bug 255305</a>.</span>
<table border=0 cellpadding=2 cellspacing=0 align="Center">
<tr style="background-color:#ccc;font-weight:bold;">
<td>&nbsp;</td>
<td style="font-size: 8pt">ID</td>
<td style="font-size: 8pt">vID</td>
<td style="font-size: 8pt">uID</td>
<td style="font-size: 8pt">Action</td>
<td style="font-size: 8pt">Date</td>
<td style="font-size: 6pt">Install?</td>
<td style="font-size: 6pt">Unistall?</td>
<td style="font-size: 6pt">New Chrome?</td>
<td style="font-size: 6pt">App Works?</td>
<td style="font-size: 6pt">Visual Errors?</td>
<td style="font-size: 6pt">All Elements Themed?</td>
<td style="font-size: 6pt">Clean Profile?</td>
<td style="font-size: 6pt">Works As Described?</td>
<td style="font-size: 6pt">Test Build:</td>
<td style="font-size: 7pt">Test OS:</td>
<td style="font-size: 7pt">Comments:</td>
</tr>
<?php

$_start_date = mysql_real_escape_string($_GET['start']);
$_end_date = mysql_real_escape_string($_GET['end']);

$sql ="SELECT * FROM `approvallog` WHERE 1=1";

if (!empty($_start_date)) {
    $sql .= " AND date >= '{$_start_date}'";
} else {
    // If we don't have a start date, throw in the beginning of the current month
    $_start_date = date('Y-m-01');
    $sql .= " AND date >= '{$_start_date}'";
}
if (!empty($_end_date)) {
    $sql .= " AND date <= '{$_end_date} 23:59:59'";
} else {
    $_end_date = date('Y-m-t');
    $sql .= " AND date <= '{$_end_date} 23:59:59'";
}
$sql .= " ORDER BY `date` DESC";

$_start_date = htmlentities($_start_date);
$_end_date = htmlentities($_end_date);

echo "<p>Showing results between {$_start_date} and {$_end_date}</p>";

// Basically, we're grabbing the start and end dates
// they gave us, and adding or subtracting a week to exaggerate the month they are
// in.  This means they land into the previous or next month for us, and we just have
// to make the start the first, and then figure out the length of the month.  I
// assure you this is temporary and this whole thing is getting rewritten.
echo '
<p>
<a href="?function=approvalhistory&start='.date('Y-m-01',strtotime($_start_date)-604800).'&end='.date('Y-m-t',strtotime($_start_date)-604800).'">&laquo; Prev Month</a> 
<a href="?function=approvalhistory&start='.date('Y-m-01',strtotime($_end_date)+604800).'&end='.date('Y-m-t',strtotime($_end_date)+604800).'">Next Month &raquo;</a>
</p>
';



 $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
 $num_results = mysql_num_rows($sql_result);
  while ($row = mysql_fetch_array($sql_result)) {
    $i++;
    $logid = $row["LogID"];
    $id = $row["ID"];
    $vid = $row["vID"];
    $userid = $row["UserID"];
    $action = $row["action"];
    $date = $row["date"];
    $installation = $row["Installation"];
    $uninstallation = $row["Uninstallation"];
    $newchrome = $row["NewChrome"];
    $appworks = $row["AppWorks"];
    $visualerrors = $row["VisualErrors"];
    $allelementsthemed = $row["AllElementsThemed"];
    $cleanprofile = $row["CleanProfile"];
    $worksasdescribed = $row["WorksAsDescribed"];
    $testbuild = $row["TestBuild"];
    $testos = $row["TestOS"];
    $comments = $row["comments"];

    $class = ($i % 2) ? 'rowa' : 'rowb';

    echo"<tr class=\"{$class}\">
    <td>$i.</td>
    <td>$id</td>
    <td>$vid</td>
    <td>$userid</td>
    <td>$action</td>
    <td>$date</td>
    <td>$installation</td>
    <td>$uninstallation</td>
    <td>$newchrome</td>
    <td>$appworks</td>
    <td>$visualerrors</td>
    <td>$allelementsthemed</td>
    <td>$cleanprofile</td>
    <td>$worksasdescribed</td>
    <td>$testbuild</td>
    <td>$testos</td>
    <td>$comments</td>
    </tr>\n";
  }
?>
</table>
<?php
}
?>
<!-- close #mBody-->
</div>

<?php
require_once(FOOTER);
?>
