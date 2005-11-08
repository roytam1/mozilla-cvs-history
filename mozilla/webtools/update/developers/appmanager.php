<?php
require_once('../core/init.php');
require_once('./core/sessionconfig.php');
$function = $_GET['function'];
$page_title = 'Mozilla Update :: Developer Control Panel :: Application Manager';
require_once(HEADER);
require_once('./inc_sidebar.php');

if ($_SESSION["level"]!="admin") {
    echo"<h1>Access Denied</h1>\n";
    echo"You do not have access to the Application Manager";
    require_once(FOOTER);
    exit;
}
?>

<?php
if (!$function) {

//Add Category to MySQL Table
  if ($_POST["submit"]=="Add Application" or $_POST["submit"]=="Add Version") {
     echo"<h2>Processing Add Request, please wait...</h2>\n";

     $appname = escape_string($_POST["appname"]);
     $guid = escape_string($_POST["guid"]);
     $shortname = escape_string($_POST["shortname"]);
     $version = escape_string($_POST["version"]);
     $major = escape_string($_POST["Major"]);
     $minor = escape_string($_POST["Minor"]);
     $release = escape_string($_POST["Release"]);
     $subver = escape_string($_POST["SubVer"]);
     $public_ver = escape_string($_POST["public_ver"]);

     if (checkFormKey()) {
       $sql = "INSERT INTO `applications` (`AppName`, `GUID`, `shortname`, `Version`, `major`, `minor`, `release`,`SubVer`,`public_ver`) VALUES ('$appname','$guid','$shortname','$version', '$major','$minor','$release','$subver','$public_ver')";
       $sql_result = mysql_query($sql, $connection) or trigger_error("<div class=\"error\">MySQL Error ".mysql_errno().": ".mysql_error()."</div>", E_USER_NOTICE);
       if ($sql_result) {
           echo"The application $appname $version has been successfully added.";
       }
     }
}
?>

<h1>Manage Application List</h1>
<SPAN style="font-size:8pt">&nbsp;&nbsp;&nbsp;&nbsp; Show Versions for: <?php $i=0;
$sql = "SELECT `AppName` from `applications` GROUP BY `AppName` ORDER BY `AppName` ASC";
 $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
   while ($row = mysql_fetch_array($sql_result)) {
   $appname = $row["AppName"];
   echo"<a href=\"?application=".strtolower($appname)."\">$appname</a> / ";
   }
   echo"<a href=\"?function=addnewapp\">Add New Application...</a>";
?></span>

<h2>Application Versions for <?php echo ucwords($application); ?></h2>
<TABLE CELLPADDING=1 CELLSPACING=1 STYLE="border: 0px;">
<TR>
<tr>
<th></th>
<th style="width: 200px">Release</th>
<th>Public Version?</th>
</tr>

<?php
  $i=0;
  $sql = "SELECT * FROM `applications` WHERE `AppName`='$application' ORDER BY `AppName` ASC, `major` ASC, `minor` ASC, `release` ASC, `SubVer` ASC";
  $sql_result = mysql_query($sql, $connection) or trigger_error("<div class=\"error\">MySQL Error ".mysql_errno().": ".mysql_error()."</div>", E_USER_NOTICE);
  while ($row = mysql_fetch_array($sql_result)) {
      
      echo"<tr>\n";
      echo"<td>".++$i."</td>\n";
      echo"<td>&nbsp;<a href=\"?function=editversion&appid=".$row["AppID"]."\">$row[AppName] $row[Version]</a></td>";
      echo"<td>$row[public_ver]</td>";
      echo"</tr>\n";

  }

?>
</table>

<h2>New Version of <?php echo ucwords($application); ?></h2>
<form name="addapplication" method="post" action="?function=&action=addnewapp">
<?writeFormKey();?>
<?php
  $sql = "SELECT `AppName`, `GUID`, `shortname` FROM `applications` WHERE `AppName`='$application' LIMIT 1";
  $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
  $row = mysql_fetch_array($sql_result);
  $application = $row["AppName"];
  $guid = $row["GUID"];
  $shortname = $row["shortname"];
?>
<input name="appname" type="hidden" value="<?php echo ucwords($application); ?>">
<input name="guid" type="hidden" value="<?php echo"$guid"; ?>">
<input name="shortname" type="hidden" value="<?php echo"$shortname"; ?>">

Display Version (e.g. 1.0PR): <input name="version" size=8 maxlength=15 title="User Friendly Version (Ex. 1.0PR instead of 0.10)"><BR>

Major:  <input name="Major" type="text" size="5" maxlength="3" value="" title="Major Version #. (Ex. 1 if 1.0)">
Minor:  <input name="Minor" type="text" size="5" maxlength="3" value="" title="Minor Version #. (Ex. 0 if 1.0)">
Release:  <input name="Release" type="text" size="5" maxlength="3" value="" title="Release Version. (Ex. 2 if 1.0.2)">
SubVer: <input name="SubVer" size="5" maxlength="5" title="SubVersion Value (Ex. a4 if 1.8a4 or + if 0.10+)">

<br><br>
Public Version: Yes: <input name="public_ver" type="radio" value="YES" checked> No: <input name="public_ver" type="radio" value="NO"><br>

<p>Please note that a 'final' SubVer causes the version comparison in additem.php to ignore the release value:</p>
<ul>
<li>1 0 4 final --> 1.0</li>
<li>1 0 0 final --> 1.0</li>
</ul>
<p>In all other cases, release will be counted!</p>
<ul>
<li>1 0 4 0 --> 1.0.4.0</li>
<li>1 0 0 + --> 1.0.0+</li>
<li>1 0 0 a1+ --> 1.0.0.a1+</li>
</ul>
<input name="submit" type="submit" value="Add Version">&nbsp;<input name="reset" type="reset" value="Reset Form">
</form>

<?php
} else if ($function=="editversion") {

  $appid = escape_string($_GET["appid"]);
  //Post Functions
  if ($_POST["submit"] == "Update") {

     $appname = escape_string($_POST["AppName"]);
     $version = escape_string($_POST["version"]);
     $major = escape_string($_POST["Major"]);
     $minor = escape_string($_POST["Minor"]);
     $release = escape_string($_POST["Release"]);
     $subver = escape_string($_POST["SubVer"]);
     $public_ver = escape_string($_POST["public_ver"]);
     $appid= escape_string($_POST["appid"]);

    if (checkFormKey()) {
      echo"<h2>Processing update request, please wait...</h2>\n";

      $sql = "UPDATE `applications` SET `AppName`='$appname', `major`='$major', `minor`='$minor', `release`='$release', `SubVer`='$subver',`Version`='$version', `public_ver`='$public_ver' WHERE `appid`='$appid'";
      $sql_result = mysql_query($sql, $connection) or trigger_error("<div class=\"error\">MySQL Error ".mysql_errno().": ".mysql_error()."</div>", E_USER_NOTICE);
      if ($sql_result) {
          echo"Your update to $appname $version has been successful.<br>";
      }
    }
  } else if ($_POST["submit"] == "Delete Version") {
     $appid= escape_string($_POST["appid"]);
     $appname = escape_string($_POST["AppName"]);
     $version = escape_string($_POST["version"]);
    echo"<h2>Processing delete request, please wait...</h2>\n";
    $sql = "DELETE FROM `applications` WHERE `appid`='$appid'";
    $sql_result = mysql_query($sql, $connection) or trigger_error("<div class=\"error\">MySQL Error ".mysql_errno().": ".mysql_error()."</div>", E_USER_NOTICE);
    if ($sql_result) {
        echo"You've successfully deleted the application '$appname $version'<br>";
        require_once(FOOTER);
        exit;
    }
}

if (!$appid) { $appid = escape_string($_POST["appid"]); }
// Show Edit Form
  $sql = "SELECT * FROM `applications` WHERE `appid` = '$appid' LIMIT 1";
  $sql_result = mysql_query($sql, $connection) or trigger_error("<div class=\"error\">MySQL Error ".mysql_errno().": ".mysql_error()."</div>", E_USER_NOTICE);

  $row = mysql_fetch_array($sql_result);
?>


<h3>Edit Application:</h3>
<form name="editcategory" method="post" action="?function=editversion">
<?writeFormKey();?>
<?php
  echo"Name:  <input name=\"AppName\" type=\"text\" size=\"30\" maxlength=\"30\" value=\"".$row["AppName"]."\"><br>\n";
  echo"Display Version: <input name=\"version\" type=\"text\" size=\"10\" maxlength=\"15\" value=\"$row[Version]\" title=\"User Friendly Version (Ex. 1.0PR instead of 0.10)\"><br>\n";
  echo"Major:  <input name=\"Major\" type=\"text\" size=\"5\" maxlength=\"3\" value=\"".$row["major"]."\" title=\"Major Version #. (Ex. 1 if 1.0)\">";
  echo"Minor:  <input name=\"Minor\" type=\"text\" size=\"5\" maxlength=\"3\" value=\"".$row["minor"]."\" title=\"Minor Version #. (Ex. 0 if 1.0)\">";
  echo"Release:  <input name=\"Release\" type=\"text\" size=\"5\" maxlength=\"3\" value=\"".$row["release"]."\" title=\"Release Version. (Ex. 2 if 1.0.2)\">";
  echo"SubVer:  <input name=\"SubVer\" type=\"text\" size=\"8\" maxlength=\"5\" value=\"$row[SubVer]\" title=\"SubVersion Value (Ex. a4 if 1.8a4 or + if 0.10+)\"><br>\n";

?>
<div style="margin-top: 10px; font-size: 8pt">Should this version be exposed to end-users of the website, or just allowed for extension authors' install.rdf files? In general only release milestones should be exposed.</DIV>
<?php
$public_ver = $row["public_ver"];
 echo"Public Version: ";
 if ($public_ver=="YES") {
   echo"Yes: <input name=\"public_ver\" type=\"radio\" value=\"YES\" checked> No: <input name=\"public_ver\" type=\"radio\" value=\"NO\">\n";
   } else if ($public_ver=="NO") {
   echo"Yes: <input name=\"public_ver\" type=\"radio\" value=\"YES\"> No: <input name=\"public_ver\" type=\"radio\" value=\"NO\" checked>\n";
   }


  echo"<input name=\"appid\" type=\"hidden\" value=\"".$row["AppID"]."\">\n";
?>
<BR><BR>
<input name="submit" type="submit" value="Update">
<input name="reset"  type="reset"  value="Reset Form">
<input name="submit" type="submit" value="Delete Version" onclick="return confirm('Are you sure you want to delete <?php echo"$row[AppName] $row[Version]"; ?>?');">
</form>
<BR><BR>
<A HREF="?function=">&#171;&#171; Return to Application Manager</A>
</div>

<?php
} else if ($function=="addnewapp") {
?>

<h1>Add New Application</h1>
<form name="addapplication" method="post" action="?function=&action=addnewapp">
<?writeFormKey();?>
Application Name: <input name="appname" type="text" title="Name of Application (e.g. Firefox)"><BR>
GUID of App: <input name="guid" type="text" size=35 title="Application Identifier"><BR>
Shortname: <input name="shortname" type="text" size="5" maxlength=2 title="two char abbrieviation of appname. (Eg fx for Firefox, or tb for Thunderbird)"><BR>
<BR>
Display Version (e.g. 1.0PR): <input name="version" size=8 maxlength=15 title="User Friendly Version (Ex. 1.0PR instead of 0.10)"><BR>

Major:  <input name="Major" type="text" size="5" maxlength="3" value="" title="Major Version #. (Ex. 1 if 1.0)">
Minor:  <input name="Minor" type="text" size="5" maxlength="3" value="" title="Minor Version #. (Ex. 0 if 1.0)">
Release:  <input name="Release" type="text" size="5" maxlength="3" value="" title="Release Version. (Ex. 2 if 1.0.2)">
SubVer: <input name="SubVer" size="5" maxlength="5" title="SubVersion Value (Ex. a4 if 1.8a4 or + if 0.10+)">
<BR>
<input name="public_ver" type="hidden" value="YES">
<BR>
<input name="submit" type="submit" value="Add Application">&nbsp;<input name="reset" type="reset" value="Reset Form">
</form>
<?php
} 
?>


<!-- close #mBody-->
</div>

<?php
require_once(FOOTER);
?>
