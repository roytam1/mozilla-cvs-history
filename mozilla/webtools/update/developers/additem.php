<?php
require_once('../core/init.php');
require_once('./core/sessionconfig.php');
$function = $_GET['function'];
$page_title = 'Mozilla Update :: Developer Control Panel :: Add Item';
require_once(HEADER);
require_once('./inc_sidebar.php');
require_once('./parse_install_manifest.php');
require_once('../core/inc_version_comparison.php');

if (!$function or $function=="additem") {
    if (!$_GET["type"]) {
        $_GET["type"] = "E"; 
    }
    $typearray = array("E"=>"Extension","T"=>"Theme");
    $typename = $typearray[$_GET["type"]];
?>

<h1>Add New <?php echo"$typename"; ?></h1>
<TABLE BORDER=0 CELLPADDING=2 CELLSPACING=2 ALIGN=CENTER STYLE="border: solid 0px #000000; width: 100%">
<FORM NAME="additem" METHOD="POST" ACTION="?function=additem2" enctype="multipart/form-data">
<INPUT NAME="type" TYPE="hidden" VALUE="<?php echo"$_GET[type]"; ?>">
<TR><TD style="padding-left: 20px">
Your <?php echo"$typename"?> File:<BR>
<INPUT NAME="file" SIZE=40 TYPE="FILE"><BR>
<BR>
<INPUT NAME="button" TYPE="BUTTON" VALUE="Cancel" onclick="javascript:history.back()"> <INPUT NAME="submit" TYPE="SUBMIT" VALUE="Next &#187;"> 
</TD></TR>
</FORM>
</TABLE>

<?php
} else if ($function=="additem2") {
    $filename=check_filename($_FILES['file']['name']);
    $filetype=$_FILES['file']['type'];
    $filesize=$_FILES['file']['size'];
    $uploadedfile=$_FILES['file']['tmp_name'];
    $status=$_FILES['file']['error'];

    //Convert File-Size to Kilobytes
    $filesize = round($filesize/1024, 1);

    //Status
    // TODO: refactor this nonsense code to make some use of messages
    // (and perhaps die early)

    if ($status==0) {$statusresult="Success!";
    } else if ($status==1) {$statusresult="Error: File Exceeds upload_max_filesize (PHP)";
    } else if ($status==2) {$statusresult="Error: File Exceeds max_file_size (HTML)";
    } else if ($status==3) {$statusresult="Error: File Incomplete, Partial File Received";
    } else if ($status==4) {$statusresult="Error: No File Was Uploaded";
    }

    $manifest_exists = "FALSE";
    $destination = REPO_PATH."/temp/$filename";

    if (move_uploaded_file($uploadedfile, $destination)) {
        $uploadedfile = $destination;
        $chmod_result = chmod("$uploadedfile", 0644); //Make the file world readable. prevent nasty permissions issues.
    }

    $zip = @zip_open("$uploadedfile");

    if ($zip) {

        while ($zip_entry = zip_read($zip)) {
            if (zip_entry_name($zip_entry)=="install.rdf") {
                $manifest_exists = "TRUE";
                // echo "Name:              " . zip_entry_name($zip_entry) . "\n";
                // echo "Actual Filesize:    " . zip_entry_filesize($zip_entry) . "\n";
                // echo "Compressed Size:    " . zip_entry_compressedsize($zip_entry) . "\n";
                // echo "Compression Method: " . zip_entry_compressionmethod($zip_entry) . "\n";

                if (zip_entry_open($zip, $zip_entry, "r")) {
                    //    echo "File Contents:\n";
                    $buf = zip_entry_read($zip_entry, zip_entry_filesize($zip_entry));
                    // echo "$buf\n";

                    zip_entry_close($zip_entry);
                }
                echo "\n";
            }
        }

   zip_close($zip);
   
   }

}
if ($manifest_exists=='TRUE') {

//------------------
//  Construct $manifestdata[] array from install.rdf info.
//-------------------
$manifestdata = parse_install_manifest($buf);

if(is_null($manifestdata)) {
  echo"Errors were encountered during install.rdf parsing...<br>\n";
  die("Aborting...");
}

//echo"<h1>Adding Extension... Checking file...</h1>\n";
//echo"<pre>"; print_r($manifestdata); echo"</pre>\n";
//Populate Form Variables from manifestdata.
$id = $manifestdata["id"];
$version = $manifestdata["version"];
$homepage = $manifestdata["homepageURL"];

// Do we have an updateURL?  If so, error out.
if (isset($manifestdata['updateURL'])) {
    echo '<h2>updateUrl not allowed</h2>';
    echo '<p>Addons cannot have an external updateURL value.   Please remove this from your install.rdf and try again.</p>';
    echo '</div>';
    require_once(FOOTER);
    exit;
}

if (isset($id) && !preg_match('/^(\{[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}\}|[a-z0-9-\._]*\@[a-z0-9-\._]+)$/i',$id)) {
    echo '<h2>Invalid id</h2>';
    echo '<p>Your id is invalid.  Please update your install.rdf and try again.  For more information on valid id\'s, please see <a href="http://developer.mozilla.org/en/docs/Install_Manifests#id">developer.mozilla.org\'s page on id\'s</a>.</p>';
    echo '</div>';
    require_once(FOOTER);
    exit;
}

// $names, $descriptions are arrays keyed by locale
$names = $manifestdata["name"];
$descriptions = $manifestdata["description"];
//TODO: support multiple locale names/descriptions
// right now we just use en-US or the first one
$name = trim(default_l10n($names));
$description = default_l10n($descriptions);

//Check GUID for validity/existance, if it exists, check the logged in author for permission
$sql = "SELECT ID, GUID from `main` WHERE `GUID` = '".escape_string($manifestdata[id])."' LIMIT 1";
 $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
  if (mysql_num_rows($sql_result)=="1") {
//    echo"This is a updated extension... Checking author data...<br>\n";
    $mode = "update";
    $row = mysql_fetch_array($sql_result);
    $item_id = $row["ID"];

    $sql = "SELECT `UserID` from `authorxref` WHERE `ID`='$item_id' AND `UserID` = '$_SESSION[uid]' LIMIT 1";
      $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
      if (mysql_num_rows($sql_result)=="1" or ($_SESSION["level"]=="admin" or $_SESSION["level"]=="editor")) {
//        echo"This extension belongs to the author logged in<br>\n";
      } else {
        echo"ERROR!! This extension does NOT belong to the author logged in.<br>\n";
        die("Terminating...");
      }

  } else {
  $mode = "new";
//  echo"This is a new extension...<br>\n";
  }

/**
 * For each targetApplication, verify that the min/max app versions are
 * correctly formatted.
 * @TODO Rewrite this entire page one weekend instead of hacking on it.
 * @TODO Fix references to non-existent variables and array indeces.
 * @TODO Rethink how we're storing versions, and clean up new versions as they come in.
 */

// We need a marker to say whether or not we have a valid GUID at all.
// We are looking for at least one valid Mozilla application.
// If it has none, it will error out, as it is a conflict of interest.
$oneValidGuidFound = false;

$versioncheck = array();

// For each of our specified targetApplications, we iterate to find a matching
// result.  Once we find a matching result, we set the flag to true.  Once we
// have to successful matches (one for maxVersion, one for minVersion), we break
// the loop and move on.
foreach ($manifestdata['targetApplication'] as $key=>$val) {
    $esckey = escape_string($key);

    // Query to attempt to grab valid application records.
    $app_sql = "
        SELECT 
            `AppName`,
            `major`,
            `minor`,
            `release`,
            `SubVer`
        FROM
            `applications`
        WHERE
            `GUID`='$esckey' AND
            `public_ver`='YES'
        ORDER BY
            `major` DESC,
            `minor` DESC,
            `release` DESC,
            `SubVer` DESC
    ";

    $app_sql_result = mysql_query($app_sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);

    // If we have results, set our min/max versions to false by default so we can check them later.
    if (mysql_num_rows($app_sql_result) == 0) {
        continue;
    } else {
        // If we get here, we found at least one valid GUID.
        // This doesn't mean, however, that the min/max verion values for that GUID are valid.
        // This will still be tested.
        $oneValidGuidFound = true;

        $versioncheck[$key]['minVersion_valid'] = false;
        $versioncheck[$key]['maxVersion_valid'] = false;

        while ($row = mysql_fetch_array($app_sql_result, MYSQL_ASSOC)) {

            // Set up our variables.
            $appname = $row['AppName'];  // Name of the application.

            // Build our app version string.
            $appVersion = buildAppVersion($row['major'],$row['minor'],$row['release'],$row['SubVer']);

            // If we have a match, set our valid minVersion flag to true.
            if ($appVersion == $val['minVersion'] && preg_match('/^[\d\.+]*$/',$val['minVersion'])) {
                $versioncheck[$key]['minVersion_valid'] = true;
            }

            // If we have a match, set our valid maxVersion flag to true.
            if ($appVersion == $val['maxVersion']) {
                $versioncheck[$key]['maxVersion_valid'] = true; 
            }

            /**
             * Use this to debug app versions.
            echo '<pre>';
            echo 'App: '.$appname."\n";
            echo 'Release from DB: '.$row['major'].' '.$row['minor'].' '.$row['release'].' '.$row['subver']."\n";
            echo 'Version we put together: '.$appVersion."\n";
            echo 'MinVersion from RDF (match): '.$val['minVersion'].' ('.$versioncheck[$key]['minVersion_valid'].') '."\n";
            echo 'MaxVersion from RDF (match): '.$val['maxVersion'].' ('.$versioncheck[$key]['maxVersion_valid'].') '."\n\n";
            print_r($versioncheck);
            echo "\n\n\n";
            echo '</pre>';
             */

            // If we have valid matches for both max/minVersions, we don't need to
            // keep checking.  Break this loop and continue to the next application.
            if ($versioncheck[$key]['minVersion_valid'] == true && $versioncheck[$key]['maxVersion_valid'] == true) {
                break;
            }
        }

        // If we never found a valid minVersion, report the error.
        if ($versioncheck[$key]['minVersion_valid'] == false) {
            echo "Error! The MinAppVer for $appname of " . $val['minVersion'] . " in install.rdf is invalid.<br>\n";
            $versioncheck['errors'] = true;
        }

        // If we never found a valid maxVersion, report the error.
        if ($versioncheck[$key]['maxVersion_valid'] == false) {
            echo "Error! The MaxAppVer for $appname of ". $val['maxVersion'] . " in install.rdf is invalid.<br>\n";
            $versioncheck['errors'] = true;
        }
    }
}

// If they don't have at least one valid GUID, tell them that is not allowed.
if ($oneValidGuidFound == false) {
    echo "Sorry, your add-on must have at least one valid Mozilla application to use this site.<br>";
    die('Aborting...');

// Even if we have a valid GUID, it still has to have valid min/max version values.
// If these don't exist, we need to error out and say why.
} elseif (!empty($versioncheck['errors']) && $versioncheck['errors'] == true) {
    echo "Errors were encountered during install.rdf checking...<br>\n";
    echo "<p>How to fix this:</p>";
    echo "<ul>";
    echo "<li><a href=\"".WEB_PATH."/faq.php\">See the list of valid version numbers</a></li>";
    echo "<li>minVersion (MinAppVer) values may only contain values 0-9 and '.' because they have to be an absolute version.  minVersions like 1.0+ or 1.5.0.* are not allowed.</li>";
    echo "<li>Your version has not been found in the addons database but it should be.  See #umo@mozilla.org in IRC if you think this is in error.</li>";
    echo "</ul>";
    die('Aborting...');
}

$typearray = array("E"=>"Extension","T"=>"Theme");
$type = escape_string($_POST["type"]);
$typename = $typearray[$type];

if ($mode=="update") {
 $sql = "SELECT  `Name`, `Homepage`, `Description` FROM  `main` WHERE `ID` = '$item_id' LIMIT 1";
 $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
   $row = mysql_fetch_array($sql_result);
    if (!$name) { $name=$row["Name"]; }
    $homepage = $row["Homepage"];
    $description = $row["Description"];

 $authors = ""; $i="";
 $sql = "SELECT TU.UserEmail FROM  `authorxref` TAX INNER JOIN userprofiles TU ON TAX.UserID = TU.UserID WHERE `ID` = '$item_id'";
 $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
   $numresults = mysql_num_rows($sql_result);
   while ($row = mysql_fetch_array($sql_result)) {
    $i++;
    $email = $row["UserEmail"];
   $authors .= "$email";
   if ($i < $numresults) { $authors .=", "; }
   }

//Get Currently Set Categories for this Object...
 $sql = "SELECT  TCX.CategoryID, TC.CatName FROM  `categoryxref`  TCX 
INNER JOIN categories TC ON TCX.CategoryID = TC.CategoryID
WHERE TCX.ID = '$item_id'
ORDER  BY  `CatName`  ASC ";
 $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
  while ($row = mysql_fetch_array($sql_result)) {
   $n++;
   $catid = $row["CategoryID"];
   $categories[$n] = $catid;
  }
unset($n);
}

if (!$categories) {$categories = array(); }
?>
<h1>Add New <?php echo"$typename"; ?> &#187;&#187; Step 2:</h2>
<TABLE BORDER=0 CELLPADDING=2 CELLSPACING=2 ALIGN=CENTER STYLE="border: solid 0px #000000; width: 100%">
<FORM NAME="addstep2" METHOD="POST" ACTION="?function=additem3">
<INPUT NAME="mode" TYPE="HIDDEN" VALUE="<?php echo"$mode"; ?>">
<?php if ($mode=="update") { ?>
<INPUT NAME="item_id" TYPE="HIDDEN" VALUE="<?php echo"$item_id"; ?>">
<?php } ?>
<INPUT NAME="guid" TYPE="HIDDEN" VALUE="<?php echo"$id"; ?>">
<INPUT NAME="type" TYPE="HIDDEN" VALUE="<?php echo"$type"; ?>">
<TR><TD><SPAN class="global">Name*</SPAN></TD> <TD><INPUT NAME="name" TYPE="TEXT" VALUE="<?php echo"$name"; ?>" SIZE=45 MAXLENGTH=100></TD>

<?php
//Get the Category Table Data for the Select Box
 $sql = "SELECT  `CategoryID`, `CatName` FROM  `categories` WHERE `CatType` = '$type' GROUP BY `Catname` ORDER  BY  `CatName` ASC";
 $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
?>
<TD ROWSPAN=8 VALIGN=TOP><SPAN class="global">Categories:</SPAN><BR>&nbsp;&nbsp;&nbsp;&nbsp;<SELECT NAME="categories[]" MULTIPLE="YES" SIZE="10">
<?php
  while ($row = mysql_fetch_array($sql_result)) {
    $catid = $row["CategoryID"];
    $catname = $row["CatName"];

    echo"<OPTION value=\"$catname\"";
    foreach ($categories as $validcat) {
    if ($validcat==$catid) { echo" SELECTED"; }
    }
    echo">$catname</OPTION>\n";

  }
?>
</SELECT></TD></TR>

<?php
if (!$authors) {$authors="$_SESSION[email]"; }
?>
<TR><TD><SPAN class="global">Author(s):*</SPAN></TD><TD><INPUT NAME="authors" TYPE="TEXT" VALUE="<?php echo"$authors"; ?>" SIZE=45></TD></TR>
<?php
if ($version) {
    echo"<TR><TD><SPAN class=\"file\">Version:*</SPAN></TD><TD>$version<INPUT NAME=\"version\" TYPE=\"HIDDEN\" VALUE=\"$version\"></TD></TR>\n";
} else {
    echo"<TR><TD><SPAN class=\"file\">Version:*</SPAN></TD><TD><INPUT NAME=\"version\" TYPE=\"TEXT\" VALUE=\"$version\"></TD></TR>\n";
}
    echo"<TR><TD><SPAN class=\"file\">OS*</SPAN></TD><TD><SELECT NAME=\"osid\">";
 $sql = "SELECT * FROM `os` ORDER BY `OSName` ASC";
  $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
  while ($row = mysql_fetch_array($sql_result)) {
  $osid = $row["OSID"];
  $osname = $row["OSName"];
   echo"<OPTION value=\"$osid\">$osname</OPTION>\n";
  }
    echo"</SELECT></TD></TR>\n";
    echo"<TR><TD><SPAN class=\"file\">Filename:</SPAN></TD><TD>$filename ($filesize"."kb) <INPUT name=\"filename\" type=\"hidden\" value=\"$filename\"><INPUT name=\"filesize\" type=\"hidden\" value=\"$filesize\"></TD></TR>\n";

echo"<TR><TD COLSPAN=2><SPAN class=\"file\">Target Application(s):</SPAN></TD></TR>\n";
 $sql2 = "SELECT `AppName`,`GUID` FROM `applications` GROUP BY `AppName` ORDER BY `AppName` ASC";
 $sql_result2 = mysql_query($sql2, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
  while ($row2 = mysql_fetch_array($sql_result2)) {
   $appname = $row2["AppName"];
   $guid = $row2["GUID"];
   if ($appname == "Mozilla") { $mozguid = $guid; }
   $minappver = $manifestdata["targetApplication"]["$guid"]["minVersion"];
   $maxappver = $manifestdata["targetApplication"]["$guid"]["maxVersion"];
    echo"<TR><TD></TD><TD>$appname ";

if (($mode=="new" or $mode=="update") and (strtolower($appname) !="mozilla" or $manifestdata["targetApplication"]["$mozguid"])) {
 //Based on Extension Manifest (New Mode)
    if ($minappver and $maxappver) {
      echo"$minappver - $maxappver\n";
      echo"<INPUT name=\"$appname-minappver\" TYPE=\"HIDDEN\" VALUE=\"$minappver\">\n";
      echo"<INPUT name=\"$appname-maxappver\" TYPE=\"HIDDEN\" VALUE=\"$maxappver\">\n";
    } else {
      echo"N/A";
    }
} else {
 //Legacy Mode Code...
    if ($appname =="Firefox" or $appname == "Thunderbird") { 
    echo"<br><SPAN style=\"font-size: 8pt; font-weight: bold\">Incompatable with Legacy Extensions (Requires install.rdf)</SPAN>";
    } else {

$sql = "SELECT `version`,`major`,`minor`,`release`,`SubVer` FROM `applications` WHERE `AppName` = '$appname' ORDER BY `major` ASC, `minor` ASC, `release` ASC, `SubVer` ASC";
 $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
echo"<SELECT name=\"$appname-minappver\" TITLE=\"Minimum Version* (Required)\">";
echo"<OPTION value\"\"> - </OPTION>\n";
  while ($row = mysql_fetch_array($sql_result)) {
  $release = "$row[major].$row[minor]";
  if ($row["release"]) {$release = "$release.$row[release]";}
    $subver = $row["SubVer"];
    if ($subver !=="final") {$release="$release$subver";}
    echo"<OPTION value=\"$release\">$release</OPTION>\n";
  }
echo"</select>\n";

   echo"&nbsp;-&nbsp;";

$sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
echo"<SELECT name=\"$appname-maxappver\" TITLE=\"Maximum Version* (Required)\">";
echo"<OPTION value\"\"> - </OPTION>\n";
  while ($row = mysql_fetch_array($sql_result)) {
  $release = "$row[major].$row[minor]";
  if ($row["release"]) {$release = "$release.$row[release]";}
    $subver = $row["SubVer"];
    if ($subver !=="final") {$release="$release$subver";}
    echo"<OPTION value=\"$release\">$release</OPTION>\n";
  }
echo"</select>\n";
    echo"</TD></TR>\n";
 }   }
}
?>

<TR><TD><SPAN class="global">Homepage</SPAN></TD> <TD COLSPAN=2><INPUT NAME="homepage" TYPE="TEXT" VALUE="<?php echo"$homepage"; ?>" SIZE=60 MAXLENGTH=200></TD></TR>
<TR><TD><SPAN class="global">Description*</SPAN></TD> <TD COLSPAN=2><TEXTAREA NAME="description" ROWS=3 COLS=55><?php echo"$description"; ?></TEXTAREA></TD></TR>
<?php
    echo"<TR><TD><SPAN class=\"file\">Version Notes:</SPAN></TD><TD COLSPAN=2><TEXTAREA NAME=\"notes\" ROWS=4 COLS=55>$notes</TEXTAREA></TD></TR>\n";
?>
<TR><TD COLSPAN="3" ALIGN="CENTER"><INPUT NAME="submit" TYPE="SUBMIT" VALUE="Next &#187;">&nbsp;&nbsp;<INPUT NAME="reset" TYPE="RESET" VALUE="Reset Form"></TD></TR>
</FORM>


</TABLE>

<?php
} else if ($function=="additem3") {
//print_r($_POST);
//exit;

//Verify that there's at least one min/max app value pair...
 $sql = "SELECT `AppName`,`AppID` FROM `applications` GROUP BY `AppName` ORDER BY `AppName` ASC";
 $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
  while ($row = mysql_fetch_array($sql_result)) {
   $appname = $row["AppName"];
   $appid = $row["AppID"];
   if (!$minappver AND $_POST["$appname-minappver"]) {$minappver="true";}
   if (!$maxappver AND  $_POST["$appname-maxappver"]) {$maxappver="true";}

  }

//Author List -- Autocomplete and Verify, if no valid authors, kill add.. otherwise, autocomplete/prompt
  $authors = escape_string($_POST["authors"]);
  $authors = explode(", ","$authors");
foreach ($authors as $author) {
if (strlen($author)<2) {continue;} //Kills all values that're too short.. 
$a++;
 $sql = "SELECT `UserID`,`UserEmail` FROM `userprofiles` WHERE `UserEmail` LIKE '$author%' ORDER BY `UserMode`, `UserName` ASC";
 $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
 $numresults = mysql_num_rows($sql_result);
   while ($row = mysql_fetch_array($sql_result)) {
    $userid = $row["UserID"];
    $useremail = $row["UserEmail"];
  if ($numresults>1) {
   //Too many e-mails match, store individual data for error block.
    $r++;
     $emailerrors[$a]["foundemails"][$r] = $useremail;
  }
 $authorids[] = $userid;
 $authoremails[] = $useremail;
 }
 if ($numresults !="1") {
  //No Valid Entry Found for this E-Mail or too many, kill and store data for error block.
  $emailerrors[$a]["author"] = "$author";
  $updateauthors = "false"; // Just takes one of these to kill the author update.
  }
}
unset($a,$r);



if ($_POST["name"] AND $_POST["type"] AND $_POST["authors"] AND $updateauthors !="false" AND $_POST["version"] AND $_POST["osid"] AND $_POST["filename"] AND $_POST["filesize"] AND $_POST["description"] AND $minappver AND $maxappver) {
//All Needed Info is in the arrays, procceed with inserting...

//Create DIV for Box around the output...
 echo"<h1>Adding Item... Please Wait...</h1>\n";
 echo"<DIV>\n";

//Phase One, Main Data
$name = escape_string($_POST["name"]);
$homepage = escape_string($_POST["homepage"]);
$description = escape_string($_POST["description"]);
$item_id = escape_string($_POST["item_id"]);
$guid = escape_string($_POST["guid"]);
$type = escape_string($_POST["type"]);

//Check to ensure tha the name isn't already taken, if it is, throw an error and halt.
$sql = "SELECT `Name` from `main` WHERE `Name`='$name' and `GUID` != '$guid'";
$sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);

if (mysql_num_rows($sql_result)=="0") {

    if ($_POST["mode"]=="update") { 
        $sql = "UPDATE `main` SET `Name`='$name', `Homepage`='$homepage', `Description`='$description', `DateUpdated`=NOW(NULL) WHERE `ID`='$item_id' LIMIT 1";
    } else {
        $sql = "INSERT INTO `main` (`GUID`, `Name`, `Type`, `Homepage`,`Description`,`DateAdded`,`DateUpdated`) VALUES ('$guid', '$name', '$type', '$homepage', '$description', NOW(NULL), NOW(NULL));";
    }

    $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
    if ($sql_result) {
        echo"Updating/Adding record for $name...<br>\n";
    } else {
        //Handle Error Case and Abort
        $failure = "true";
        echo"Failure to successfully add/update main record. Unrecoverable Error, aborting.<br>\n";
        require_once(FOOTER);
        exit;
    }
} else {
    //Name wasn't unique, error time. :-)
    //Handle Error Case and Abort
    $failure = "true";
    echo"<p><strong>Error!</strong> The Name for your extension or theme already exists in the Update database.  Please make sure that:</p>\n";
    echo <<<OPTIONS
    <ul>
        <li>Your GUIDs match -- the most common cause for this error is mismatched GUIDs (please make sure you also have the {}).</li>
        <li>You do not have a duplicate entry in the database.  If you do, you should update that entry, or delete it and try again.</li>
    </ul>
OPTIONS;
    require_once(FOOTER);
    exit;
}




//Get ID for inserted row... if we don't know it already
if (!$_POST[item_id] and $_POST["mode"] !=="update") {
$name = escape_string($_POST["name"]);
$guid = escape_string($_POST["guid"]);

 $sql = "SELECT `ID` FROM `main` WHERE `GUID`='$guid' AND `Name`='$name' LIMIT 1";
 $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
  $row = mysql_fetch_array($sql_result);
   $id = $row["ID"];
  } else {
   $id = escape_string($_POST["item_id"]);
  }


//Phase 2 -- Commit Updates to AuthorXref tables.. with the ID and UserID.
if ($updateauthors != "false") {
 //Remove Current Authors
 $sql = "DELETE FROM `authorxref` WHERE `ID` = '$id'";
 $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);

 //Add New Authors based on $authorids
 sort($authorids);
  foreach ($authorids as $authorid) {
   	$sql = "INSERT INTO `authorxref` (`ID`, `UserID`) VALUES ('$id', '$authorid');";
    $result = mysql_query($sql) or trigger_error("<FONT COLOR=\"#FF0000\"><B>MySQL Error ".mysql_errno().": ".mysql_error()."</B></FONT>", E_USER_NOTICE);
  }
   if ($result) { echo"Authors added...<br>\n"; }
} else {
   echo"ERROR: Could not update Authors list, please fix the errors printed below and try again...<br>\n"; 
}

unset($authors); //Clear from Post.. 


// Phase 3, categoryxref

if (!$_POST["categories"]) {
//No Categories defined, need to grab one to prevent errors...
 $sql = "SELECT `CategoryID` FROM `categories` WHERE `CatType`='$type' AND `CatName`='Miscellaneous' LIMIT 1";
   $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
   while ($row = mysql_fetch_array($sql_result)) {
   $_POST["categories"] = array("$row[CategoryID]");
  }

}

 //Delete Current Category Linkages...
   $sql = "DELETE FROM `categoryxref` WHERE `ID` = '$id'";
   $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);

 //Add New Categories from $_POST["categories"]
   foreach ($_POST["categories"] as $categoryname) {

    $sql2 = "SELECT `CategoryID` FROM `categories` WHERE `CatType` = '$type' AND `CatName` = '$categoryname' ORDER BY `CategoryID` ASC";
    $sql_result2 = mysql_query($sql2, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
    while ($row2 = mysql_fetch_array($sql_result2)) {
        $categoryid = $row2["CategoryID"];

   	    $sql = "INSERT INTO `categoryxref` (`ID`, `CategoryID`) VALUES ('$id', '$categoryid');";
        $result = mysql_query($sql) or trigger_error("<FONT COLOR=\"#FF0000\"><B>MySQL Error ".mysql_errno().": ".mysql_error()."</B></FONT>", E_USER_NOTICE);
    }

  }
   if ($result) {echo"Categories added...<br>\n"; }


//Phase 4, version rows

//Construct Internal App_Version Arrays
$i=0;
$sql = "SELECT `AppName`, `int_version`, `major`, `minor`, `release`, `SubVer`, `shortname` FROM `applications` ORDER BY `AppName`, `major` DESC, `minor` DESC, `release` DESC, `SubVer` DESC";
 $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
  while ($row = mysql_fetch_array($sql_result)) {
  $i++;
  $appname = $row["AppName"];
  $int_version = $row["int_version"];
  $subver = $row["SubVer"];
  $release = "$row[major].$row[minor]";
  if ($row["release"]) {$release = "$release.$row[release]";}
  if ($subver !=="final") {$release="$release$subver";}
    $app_internal_array[$release] = $int_version;
    $app_shortname[strtolower($appname)] = $row["shortname"];
  }

 $sql2 = "SELECT `AppName`,`AppID` FROM `applications` GROUP BY `AppName` ORDER BY `AppName` ASC";
 $sql_result2 = mysql_query($sql2, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
  while ($row2 = mysql_fetch_array($sql_result2)) {
   unset($minappver_int,$maxappver_int);
   $appname = $row2["AppName"];
   $appid = $row2["AppID"];
   $minappver = $_POST["$appname-minappver"];
   $maxappver = $_POST["$appname-maxappver"];

if ($minappver and $maxappver) {

if ($app_internal_array["$minappver"]) {$minappver_int = $app_internal_array["$minappver"]; }
if ($app_internal_array["$maxappver"]) {$maxappver_int = $app_internal_array["$maxappver"]; }
if (!$minappver_int) {$minappver_int = $minappver;}
if (!$maxappver_int) {$maxappver_int = $maxappver;}


$version = escape_string($_POST["version"]);
$osid = escape_string($_POST["osid"]);
$filesize = escape_string($_POST["filesize"]);
$uri = ""; //we don't have all the parts to set a uri, leave blank and fix when we do.
$notes = escape_string($_POST["notes"]);

//If a record for this item's exact version, OS, and app already exists, find it and delete it, before inserting
  $sql3 = "SELECT `vID` from `version` TV INNER JOIN `applications` TA ON TA.AppID=TV.AppID WHERE TV.ID = '$id' AND `OSID`='$osid' AND `AppName` = '$appname' AND TV.Version='$version' ORDER BY `vID` ASC";
    $sql_result3 = mysql_query($sql3, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
      while ($row = mysql_fetch_array($sql_result3)) {
        $sql = "DELETE FROM `version` WHERE `vID`='$row[vID]' LIMIT 1";
        $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
          if ($sql_result) { echo"<strong>Warning!</strong> A version Record already exists for this item's Application/OS/Version combination. Deleting.<br>\n"; }
    }

$sql = "INSERT INTO `version` (`ID`, `Version`, `OSID`, `AppID`, `MinAppVer`, `MinAppVer_int`, `MaxAppVer`, `MaxAppVer_int`, `Size`, `URI`, `Notes`, `DateAdded`, `DateUpdated`) VALUES ('$id', '$version', '$osid', '$appid', '$minappver', '$minappver_int', '$maxappver', '$maxappver_int', '$filesize', '$uri', '$notes', NOW(NULL), NOW(NULL));";
 $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
 if ($sql_result) {echo"Added $name version $version for $appname<br>\n"; $apps_array[]=$app_shortname[strtolower($appname)];}

$sql = "SELECT `vID` from `version` WHERE `id` = '$id' ORDER BY `vID` DESC LIMIT 1";
 $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
  $row = mysql_fetch_array($sql_result);
   $vid_array[] = $row["vID"];
}
}

$sql = "SELECT `OSName` FROM `os` WHERE `OSID`='$osid' LIMIT 1";
 $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
   $row = mysql_fetch_array($sql_result);
   $osname = $row["OSName"];


//Construct the New Filename
$filename = check_filename($_POST['filename']);
$filename_array = explode(".",$filename);
$filename_count = count($filename_array)-1;
$fileext = $filename_array[$filename_count];

$itemname = preg_replace('/(^\.+|[^\w\-\.]+)/','_',$name); // if you modify this, update inc_approval.php as well
$j=0; $app="";
$app_count = count($apps_array);
foreach ($apps_array as $app_val) {
$j++;
$apps .="$app_val";
if ($j<$app_count) {$apps .="+"; }
}
$newfilename = "$itemname-$version-$apps";
if (strtolower($osname) !=="all") {$newfilename .="-".strtolower($osname).""; }
$newfilename .=".$fileext";
$newfilename=check_filename(strtolower($newfilename));

//Move temp XPI to home for approval queue items...
$oldpath = REPO_PATH.'/temp/'.$filename;
$newpath = REPO_PATH.'/approval/'.$newfilename;
if (file_exists($oldpath)) { 
  rename($oldpath,$newpath) or die("Can't save $newpath to disk");
  echo"File $newfilename saved to disk...<br>\n";
}
$uri = str_replace(REPO_PATH.'/approval/','http://'.HOST_NAME.'/developers/approvalfile.php/',$newpath);
//echo"$newfilename ($oldpath) ($newpath) ($uri)<br>\n";

foreach ($vid_array as $vid) {
  $sql = "UPDATE `version` SET `URI`='$uri' WHERE `vID`='$vid'";
  $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
}

//Approval Queue
  //Check if the item belongs to the user, (special case for where admins are trusted, the trust only applies to their own work.)
  $sql = "SELECT `UserID` from `authorxref` WHERE `ID`='$id' AND `UserID` = '$_SESSION[uid]' LIMIT 1";
    $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
    if (mysql_num_rows($sql_result)=="1" AND $_SESSION["trusted"]=="TRUE") {
    //User is trusted and the item they're modifying inheirits that trust.
    include"inc_approval.php"; //Get the resuable process_approval() function.
    $action = "Approval+";
    $file = $uri;
    $comments = "Auto-Approval for Trusted User";
       $approval_result = process_approval($type, $file, "approve");

    } else {
    $action="Approval?";
    $comments="";
    }


//Firstly, log the comments and action taken..
$userid = $_SESSION["uid"];

if (!$vid_array) { $vid_array = array(); }
foreach ($vid_array as $vid) {
$sql = "INSERT INTO `approvallog` (`ID`, `vID`, `UserID`, `action`, `date`, `comments`) VALUES ('$id', '$vid', '$userid', '$action', NOW(NULL), '$comments');";
  $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
}


echo"Process Complete...<br><br>\n";
echo"$name version $version has been added to the Mozilla Update database";
if ($_SESSION["trusted"]=="FALSE") { echo" and is awaiting review by an editor, you will be notified when an editor reviews it.";
} else if ($_SESSION["trusted"]=="TRUE") {
echo" and has been auto-approved. It should be up on the website within the next half-hour.";
}
echo"<br>\n";
echo"To review or make changes to your submission, visit the <A HREF=\"itemoverview.php?id=$id\">Item Details page</A>...<br>\n";

echo"<br><br>\n";
echo"<A HREF=\"/developers/\">&#171;&#171; Back to Home</A>";
echo"</div>\n";

}


//Author Error Handling/Display Block for Form Post...
if ($emailerrors) {

echo"
<h1>Adding Item... Error Found while processing authors</h1>\n
<TABLE BORDER=0 CELLPADDING=2 CELLSPACING=2 ALIGN=CENTER STYLE=\"border: 0px; width: 100%\">
<FORM NAME=\"addstep2b\" METHOD=\"POST\" ACTION=\"?function=additem3\">";

foreach ($_POST as $key => $val) {
if ($key=="authors" or $key=="submit") {continue; }
if ($key=="categories") {
foreach ($_POST["categories"] as $val) {
echo"<INPUT name=\"categories[]\" type=\"hidden\" value=\"$val\">\n";
}
continue;
}
echo"<INPUT name=\"$key\" type=\"hidden\" value=\"$val\">\n";
}


echo"<TR><TD COLSPAN=2 STYLE=\"\">\n";
echo"<DIV style=\"margin-left 2px; border: 1px dotted #CCC;\">";
foreach ($emailerrors as $authorerror) {
$author = $authorerror["author"];
$count = count($authorerror["foundemails"]);

if ($count=="0") {
//Error for No Entry Found
echo"<SPAN STYLE=\"color: #FF0000;\"><strong>Error! Entry '$author': No Matches Found.</strong></SPAN> Please check your entry and try again.<BR>\n";
} else {
//Error for Too Many Entries Found
echo"<SPAN STYLE=\"color: #FF0000;\"><strong>Error! Entry '$author': Too Many Matches.</strong></SPAN> Please make your entry more specific.<BR>\n";
}
if ($count>0 AND $count<6) {
echo"&nbsp;&nbsp;&nbsp;&nbsp;Possible Addresses found: ";
foreach ($authorerror["foundemails"] as $foundemails) {
$a++;
echo"$foundemails";

if ($a != $count) {echo", "; } else {echo"<br>\n";}
}
}

}
echo"</font></DIV></TD></TR>\n";
$authors = $_POST["authors"];
?>

<TR><TD><SPAN class="global">Author(s):*</SPAN></TD><TD><INPUT NAME="authors" TYPE="TEXT" VALUE="<?php echo"$authors"; ?>" SIZE=70><INPUT NAME="submit" TYPE="SUBMIT" VALUE="Next &#187;"></TD></TR>
</FORM></TABLE>
<?php
}


} else {}
?>


<!-- close #mBody-->
</div>

<?php
require_once(FOOTER);
?>
