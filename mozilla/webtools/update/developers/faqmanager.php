<?php
require_once('../core/init.php');
require_once('./core/sessionconfig.php');
$function = $_GET['function'];
$page_title = 'Mozilla Update :: Developer Control Panel :: FAQ Manager';
require_once(HEADER);
require_once('inc_sidebar.php');

if ($_SESSION["level"]=="admin") {
    //Do Nothing, they're good. :-)
} else {
    echo"<h1>Access Denied</h1>\n";
    echo"You do not have access to the FAQ Manager";
    require_once(FOOTER);
    exit;
}
?>

<?php
if (!$function) {
?>

<h1>Manage FAQs:</h1>
<TABLE CELLPADDING=1 CELLSPACING=1 ALIGN=CENTER STYLE="border: 0px; width: 100%">
<TR>
<tr>
<th></th>
<th>FAQ Entry</th>
<th>Updated</th>
<th>Active</th>
</tr>

<?php
  $sql = "SELECT id, title, active, UNIX_TIMESTAMP(lastupdated) as lastupdated FROM `faq` ORDER BY `index` ASC, `title` ASC";
  $sql_result = mysql_query($sql, $connection) or trigger_error("<div class=\"error\">MySQL Error ".mysql_errno().": ".mysql_error()."</div>", E_USER_NOTICE);
  while ($row = mysql_fetch_array($sql_result)) {
    $lastupdated = gmdate("F d, Y g:i:sa", $row['lastupdated']);
    echo"<tr>\n";
    echo"<td>".++$i."</td>\n";
    echo"<td>&nbsp;<a href=\"?function=edit&id=".$row["id"]."\">".$row["title"]."</a></td>\n";
    echo"<td>$lastupdated</td>\n";
    echo"<td>".$row['active']."</td>\n";
    echo"</tr>\n";

}
?>
</table>

<h2><a href="?function=addentry">New FAQ Entry</A></h2>
<form name="addapplication" method="post" action="?function=addentry">
<?writeFormKey();?>
  Title: <input name="title" type="text" size="30" maxlength="150" value="">
<input name="submit" type="submit" value="Next &#187;&#187;"></SPAN>
</form>
</div>

<?php
} else if ($function=="edit") {
  $id = escape_string($_GET["id"]);
  //Post Functions
  if ($_POST["submit"] == "Update Entry") {
    echo"<h2>Processing your update, please wait...</h2>\n";
    $title = escape_string($_POST["title"]);
    $index = escape_string($_POST["index"]);
    $alias = escape_string($_POST["alias"]);
    $text = escape_string($_POST["text"]);
    $active = escape_string($_POST["active"]);
    $id = escape_string($_POST["id"]);
    if (checkFormKey()) {
      $sql = "UPDATE `faq` SET `title`='$title', `index`='$index', `alias`='$alias', `text`='$text', `active`='$active' WHERE `id`='$id'";
      $sql_result = mysql_query($sql, $connection) or trigger_error("<div class=\"error\">MySQL Error ".mysql_errno().": ".mysql_error()."</div>", E_USER_NOTICE);
      if ($sql_result) {
          echo"Your update to '$title', has been successful.<br>";
      }
    }

  } else if ($_POST["submit"] == "Delete Entry") {
    echo"<h2>Processing, please wait...</h2>\n";
    $id = escape_string($_POST["id"]);
    $title = escape_string($_POST["title"]);
    if (checkFormKey()) {
      $sql = "DELETE FROM `faq` WHERE `id`='$id'";
      $sql_result = mysql_query($sql, $connection) or trigger_error("<div class=\"error\">MySQL Error ".mysql_errno().": ".mysql_error()."</div>", E_USER_NOTICE);
      if ($sql_result) {
          echo"You've successfully deleted the FAQ Entry '$title'.";
          require_once(FOOTER);
          exit;
      }
    }
}

// Show Edit Form
  $sql = "SELECT * FROM `faq` WHERE `id` = '$id' LIMIT 1";
  $sql_result = mysql_query($sql, $connection) or trigger_error("<div class=\"error\">MySQL Error ".mysql_errno().": ".mysql_error()."</div>", E_USER_NOTICE);
  $row = mysql_fetch_array($sql_result);
?>

<h3>Edit FAQ Entry:</h3>
<form name="editfaq" method="post" action="?function=edit">
<?writeFormKey();?>
<?php
  echo"<input name=\"id\" type=\"hidden\" value=\"".$row["id"]."\" />\n";
  echo"Title: <input name=\"title\" type=\"text\" size=\"40\" maxlength=\"150\" value=\"".$row["title"]."\"> ";
  echo"Alias: <input name=\"alias\" type=\"text\" size=\"8\" maxlength=\"20\" value=\"".$row["alias"]."\"><br>\n";

//List of Entry Index for User Convienience
 echo"Existing Index Reference: <SELECT name=\"titleindex\">\n";
 $sql = "SELECT `id`,`title`, `index` FROM `faq` ORDER BY `index` ASC";
  $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
    while ($row2 = mysql_fetch_array($sql_result)) {
    echo"<OPTION value=\"$row2[index]\"";
    if ($row2[id]==$id) {echo" SELECTED";}
    echo">$row2[title] (Index: $row2[index])</OPTION>\n";
    }
 echo"</SELECT><BR>\n";

  echo"Index: <input name=\"index\" type=\"text\" size=\"5\" maxlength=\"5\" value=\"".$row["index"]."\"><BR>\n";
  echo"<br>\n";

  echo"Entry Text:<BR><TEXTAREA NAME=\"text\" ROWS=10 COLS=60>$row[text]</TEXTAREA><BR>";
$active = $row["active"];
echo"Show Entry on FAQ Page: ";
if ($active=="YES") {
 echo"Yes: <INPUT NAME=\"active\" TYPE=\"RADIO\" VALUE=\"YES\" CHECKED> No: <INPUT NAME=\"active\" TYPE=\"RADIO\" VALUE=\"NO\">";
 } else if ($active=="NO") {
 echo"Yes: <INPUT NAME=\"active\" TYPE=\"RADIO\" VALUE=\"YES\"> No: <INPUT NAME=\"active\" TYPE=\"RADIO\" VALUE=\"NO\" CHECKED>";
 } else {
 echo"Yes: <INPUT NAME=\"active\" TYPE=\"RADIO\" VALUE=\"YES\"> No: <INPUT NAME=\"active\" TYPE=\"RADIO\" VALUE=\"NO\">";
 }
?>
<BR><BR>
<input name="submit" type="submit" value="Update Entry">
<input name="reset"  type="reset"  value="Reset Form">
<input name="submit" type="submit" value="Delete Entry" onclick="return confirm('Are you sure you want to delete <?php echo $row["title"]; ?>?');" />
</form>
<BR><BR>
<A HREF="?function=">&#171;&#171; Return to FAQ Manager</A>

<?php
} else if ($function=="addentry") {

 //Add Category to MySQL Table
  if ($_POST["submit"]=="Add FAQ Entry") {

    echo"<h2>Adding Entry, please wait...</h2>\n";
    $title = escape_string($_POST["title"]);
    $index = escape_string($_POST["index"]);
    $alias = escape_string($_POST["alias"]);
    $text = escape_string($_POST["text"]);
    $active = escape_string($_POST["active"]);
    $id = escape_string($_POST["id"]);
    if (checkFormKey()) {
      $sql = "INSERT INTO `faq` (`title`,`index`,`alias`, `text`, `active`) VALUES ('$title','$index','$alias', '$text', '$active')";
      $sql_result = mysql_query($sql, $connection) or trigger_error("<div class=\"error\">MySQL Error ".mysql_errno().": ".mysql_error()."</div>", E_USER_NOTICE);
      if ($sql_result) {
        echo"The entry '$title' has been successfully added.<br>\n";
      }
    }
  }
?>

<h2>Add FAQ Entry:</h2>
<form name="addfaq" method="post" action="?function=addentry">
<?writeFormKey();?>
<?php
$title = escape_string($_POST["title"]);

  echo"Title: <input name=\"title\" type=\"text\" size=\"40\" maxlength=\"150\" value=\"$title\">&nbsp;\n";
  echo"Alias: <input name=\"alias\" type=\"text\" size=\"8\" maxlength=\"20\" value=\"\"><br>";

//List of Entry Index for User Convienience
 echo"<BR>Existing Index Reference: <SELECT name=\"titleindex\">\n";
 $sql = "SELECT `id`,`title`, `index` FROM `faq` ORDER BY `index` ASC";
  $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
    while ($row2 = mysql_fetch_array($sql_result)) {
    echo"<OPTION value=\"$row2[index]\"";
    if ($row2[id]==$id) {echo" SELECTED";}
    echo">$row2[title] (Index: $row2[index])</OPTION>\n";
    }
 echo"</SELECT><BR>\n";
  echo"Index: <input name=\"index\" type=\"text\" size=\"5\" maxlength=\"5\" value=\"\"> (used for FAQ page sort order)<br><br>\n";

  echo"Entry Text:<BR><TEXTAREA NAME=\"text\" ROWS=10 COLS=60></TEXTAREA><BR>";
  echo"Show Entry on FAQ Page: ";
  echo"Yes: <INPUT NAME=\"active\" TYPE=\"RADIO\" VALUE=\"YES\" CHECKED>/ No: <INPUT NAME=\"active\" TYPE=\"RADIO\" VALUE=\"NO\">";
?>
<BR><BR>

<input name="submit" type="submit" value="Add FAQ Entry" />
<input name="reset"  type="reset"  value="Reset Form" />
</form>
<BR><BR>
<A HREF="?function=">&#171;&#171; Return to FAQ Manager</A>
</div>

<?php
} else {}
?>


<!-- close #mBody-->
</div>

<?php
require_once(FOOTER);
?>
