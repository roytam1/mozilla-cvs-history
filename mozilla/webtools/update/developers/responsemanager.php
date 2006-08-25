<?php
require_once('../core/init.php');
require_once('./core/sessionconfig.php');
$function = $_GET['function'];
$page_title = 'Mozilla Update :: Developer Control Panel :: Canned Response Manager';
require_once(HEADER);
require_once('inc_sidebar.php');

if ($_SESSION["level"] == "admin") {
    //Do Nothing, they're good. :-)
} else {
    echo"<h1>Access Denied</h1>\n";
    echo"You do not have access to the Canned Response Manager";
    require_once(FOOTER);
    exit;
}

if (!$function) {
?>
<h1>Manage Canned Responses:</h1>
<TABLE CELLPADDING=1 CELLSPACING=1 ALIGN=CENTER STYLE="border: 0px; width: 100%">
    <TR>
        <TD></TD>
        <TD>Name</TD>
        <TD>Action</TD>
    </TR>
<?php
    $sql = "SELECT `CannedID`, `CannedName`, `CannedAction`, `CannedResponse` FROM `canned_responses` ORDER BY `CannedAction`, `CannedName`";
    $sql_result = mysql_query($sql, $connection) or trigger_error("<div class=\"error\">MySQL Error ".mysql_errno().": ".mysql_error()."</div>", E_USER_NOTICE);
    while ($row = mysql_fetch_array($sql_result)) {
        echo "<TR>\n";
        echo "<TD>".++$i."</TD>\n";
        echo "<TD>&nbsp;<a href=\"?function=edit&id=".$row["CannedID"]."\">".$row["CannedName"]."</a></TD>\n";
        echo "<TD>Approval".$row["CannedAction"]."</TD>\n";
        echo "</TR>\n";
    }
?>
</table>

<h2><a href="?function=addentry">New Canned Response</a></h2>
<form method="post" action="?function=addentry">
<?writeFormKey();?>
  Name: <input name="name" type="text" size="30" maxlength="50">
<input name="submit" type="submit" value="Next &#187;"></SPAN>
</form>
</div>

<?php
} elseif ($function == "edit") {
    $id = escape_string($_GET["id"]);
    //Post Functions
    if ($_POST["submit"] == "Update Response") {
        echo "<h2>Processing your update, please wait...</h2>\n";
        $name = escape_string($_POST["name"]);
        $action = escape_string($_POST["action"]);
        $response = escape_string($_POST["response"]);
        $id = escape_string($_POST["id"]);
        if (checkFormKey()) {
            $sql = "UPDATE `canned_responses` SET `CannedName`='$name', `CannedAction`='$action', `CannedResponse`='$response' WHERE `CannedID`='$id'";
            $sql_result = mysql_query($sql, $connection) or trigger_error("<div class=\"error\">MySQL Error ".mysql_errno().": ".mysql_error()."</div>", E_USER_NOTICE);
          if ($sql_result) {
              echo "Your update to '$name' has been successful.<br>";
          }
        }
    } elseif ($_POST["submit"] == "Delete Response") {
        echo "<h2>Processing, please wait...</h2>\n";
        $id = escape_string($_POST["id"]);
        $name = escape_string($_POST["name"]);
        if (checkFormKey()) {
            $sql = "DELETE FROM `canned_responses` WHERE `CannedID`='$id'";
            $sql_result = mysql_query($sql, $connection) or trigger_error("<div class=\"error\">MySQL Error ".mysql_errno().": ".mysql_error()."</div>", E_USER_NOTICE);
            if ($sql_result) {
                echo "You've successfully deleted the Canned Response '$name'.";
                echo "<br><br><a href=\"?function=\">&laquo; Back to Canned Response Manager</a>";
                require_once(FOOTER);
                exit;
            }
        }
    }

    // Show Edit Form
    $sql = "SELECT * FROM `canned_responses` WHERE `CannedID` = '$id' LIMIT 1";
    $sql_result = mysql_query($sql, $connection) or trigger_error("<div class=\"error\">MySQL Error ".mysql_errno().": ".mysql_error()."</div>", E_USER_NOTICE);
    $row = mysql_fetch_array($sql_result);
?>

<h3>Edit Canned Response:</h3>
<form  method="post" action="?function=edit">
<?php
    writeFormKey();
    echo "<br><input name=\"id\" type=\"hidden\" value=\"".$row["CannedID"]."\" />\n";
    echo "Name: <input name=\"name\" type=\"text\" size=\"40\" maxlength=\"50\" value=\"".$row["CannedName"]."\"><br>\n";
    echo "Approval Action: <input type=\"radio\" name=\"action\" value=\"+\"".($row["CannedAction"] == "+" ? " checked" : "")."> Approve&nbsp;&nbsp;\n";
    echo "<input type=\"radio\" name=\"action\" value=\"-\"".($row["CannedAction"] == "-" ? " checked" : "")."> Deny<br><br>\n";

    echo "Canned Response:<BR><TEXTAREA NAME=\"response\" ROWS=5 COLS=60>{$row['CannedResponse']}</TEXTAREA>";

?>
<BR><BR>
<input name="submit" type="submit" value="Update Response">
<input name="reset"  type="reset"  value="Reset Form">
<input name="submit" type="submit" value="Delete Response" onclick="return confirm('Are you sure you want to delete <?=$row["CannedName"]?>?');" />
</form>
<BR><BR>
<A HREF="?function=">&#171; Return to Canned Response Manager</A>

<?php
} elseif ($function == "addentry") {
    //Add response to MySQL Table
    if ($_POST["add"] != "") {
        echo "<h2>Adding Canned Response, please wait...</h2>\n";
        $name = escape_string($_POST["name"]);
        $action = escape_string($_POST["action"]);
        $response = escape_string($_POST["response"]);
        $id = escape_string($_POST["id"]);
        if (checkFormKey()) {
            $sql = "INSERT INTO `canned_responses` (`CannedName`, `CannedAction`, `CannedResponse`) VALUES ('$name','$action','$response')";
            $sql_result = mysql_query($sql, $connection) or trigger_error("<div class=\"error\">MySQL Error ".mysql_errno().": ".mysql_error()."</div>", E_USER_NOTICE);
            if ($sql_result) {
                echo "The response '$name' has been successfully added.<br>\n";
            }
        }
    }
?>

<h2>Add Canned Response:</h2>
<form method="post" action="?function=addentry">
<?writeFormKey();?>
<?php
    $name = escape_string($_POST["name"]);
?>
<br>
Name: <input name="name" type="text" size="40" maxlength="50" value="<?=$name?>"><br>
Approval Action: <input type="radio" name="action" value="+"> Approve&nbsp;&nbsp;
<input type="radio" name="action" value="-" checked> Deny<br><br>
Canned Response:<BR><TEXTAREA NAME="response" ROWS=5 COLS=60></TEXTAREA>

<BR><BR>
<input name="add" type="submit" value="Add Cannned Response" />
<input name="reset"  type="reset"  value="Reset Form" />
</form>
<BR><BR>
<A HREF="?function=">&#171; Return to Canned Response Manager</A>
</div>
<?php
}
?>
<!-- close #mBody-->
</div>

<?php
require_once(FOOTER);
?>
