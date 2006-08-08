<?php
require_once('../core/init.php');
require_once('./core/sessionconfig.php');

$function = $_GET["function"];
//Kill access to flagged comments for users.
if ($_SESSION["level"] !=="admin" and $_SESSION["level"] !=="editor") {
    if ($function=="flaggedcomments") {
        unset($function);
    }

    $id = escape_string($_GET["id"]);
    if (!$id) {$id = escape_string($_POST["id"]); }
    $sql = "SELECT `UserID` from `authorxref` TAX WHERE `ID` = '$id' AND `UserID` = '$_SESSION[uid]' LIMIT 1";
    $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
    if (mysql_num_rows($sql_result)=="0") {
        echo"<h1>Access Denied</h1>\n";
        echo"You do not have access to this item.";
        require_once(FOOTER);
        exit;
    }
}

$page_title = 'Mozilla Update :: Developer Control Panel :: Comments Manager';
require_once(HEADER);

if ($function=="flaggedcomments") {
    $skipcomments = true;
}

require_once('./inc_sidebar.php');

?>
<?php
if (!$function) {
?>

<?php
if ($_POST["submit"]=="Flag Selected" or $_POST["submit"]=="Delete Selected") {
?>
<h1>Updating comments list, please wait...</h1>
<?php

    $errors = false;

    //Process Post Data, Make Changes to Feedback Table.
    //Begin General Updating
    for ($i=1; $i<=$_POST[maxid]; $i++) {
      if (!$_POST["selected_$i"]) {
        continue;
      } else {
        $selected = escape_string($_POST["selected_$i"]);
      }

        //Admins/Editors can delete from here. Regular Users Can't.
        if ($_SESSION["level"] !=="admin" and $_SESSION["level"] !=="editor") {
            if ($_POST["submit"]=="Delete Selected") {
                $_POST["submit"]="Flag Selected";
            }
        }


     if (checkFormKey()) {
        if ($_POST["submit"]=="Delete Selected" && !empty($_POST["note_$i"])) {
	    $note = escape_string($_POST["note_$i"]);

            // Get the ID of the addon whose comment is being deleted
            // so we can recompute its rating after deleting the comment.
            $sql = "SELECT ID FROM  `feedback` WHERE `CommentID`='$selected'";
            $sql_result = mysql_query($sql, $connection) or trigger_error("<FONT COLOR=\"#FF0000\"><B>MySQL Error ".mysql_errno().": ".mysql_error()."</B></FONT>", E_USER_NOTICE);
            $id = mysql_result($sql_result, 0);

            //$sql = "DELETE FROM `feedback` WHERE `CommentID`='$selected'";
            $sql = "UPDATE `feedback` SET `CommentTitle` = '<i>Removed by Staff</i>', `CommentNote` = 'Deleted because: $note', `flag`='', `CommentVote` = NULL WHERE `CommentID='$selected'";
            $sql_result = mysql_query($sql, $connection) or trigger_error("<FONT COLOR=\"#FF0000\"><B>MySQL Error ".mysql_errno().": ".mysql_error()."</B></FONT>", E_USER_NOTICE);

            if ($sql_result) {
                echo"Comment $selected removed.<br>\n";
                // Update the rating for this item, since it has potentially changed
                update_rating($id);
            } else {
                $errors = true;
            }
        } else if ($_POST["submit"]=="Flag Selected") {
            $sql = "UPDATE `feedback` SET `flag`= 'YES' WHERE `CommentID`='$selected'";
            $sql_result = mysql_query($sql, $connection) or trigger_error("<FONT COLOR=\"#FF0000\"><B>MySQL Error ".mysql_errno().": ".mysql_error()."</B></FONT>", E_USER_NOTICE);
            if ($sql_result) {
                echo"Comment $selected flagged for editor review.<br>\n";
            } else {
                $errors = true;
            }

        }
     }

    }

  unset($i);
  if ($errors==true) {
  echo"Your changes to the comment list have been succesfully completed<BR>\n";
  } else {
  echo "There were some errors processing your request.\n";
  }

 }
?>
<?php
if ($_GET["numpg"]) {$items_per_page=escape_string($_GET["numpg"]); } else {$items_per_page="50";} //Default Num per Page is 50
if (!$_GET["pageid"]) {$pageid="1"; } else { $pageid = escape_string($_GET["pageid"]); } //Default PageID is 1
$startpoint = ($pageid-1)*$items_per_page;

$id = escape_string($_GET["id"]);

$sql = "SELECT `Type`,`Name` FROM `main` WHERE `ID`='$id' LIMIT 1";
$sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
    $row = mysql_fetch_array($sql_result);
    $type = $row["Type"];
    $name = $row["Name"];

$sql = "SELECT CommentID FROM  `feedback` WHERE ID = '$id'";
$sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
    $num_pages = ceil(mysql_num_rows($sql_result)/$items_per_page);
?>

<h1>Manage Comments for <?php echo"$name :: Page $pageid of $num_pages"; ?></h1>
<?php
//Flagged Comments Queue Link for Admins/Editors
if ($_SESSION["level"] =="admin" or $_SESSION["level"]=="editor") {
    echo"<a href=\"?function=flaggedcomments\">View Flagged Comments Queue</a> | \n";
}

// Begin Code for Dynamic Navbars
if ($pageid <=$num_pages) {
    $previd=$pageid-1;
    if ($previd >"0") {
        echo"<a href=\"?".uriparams()."&id=$id&page=$page&pageid=$previd\">&#171; Previous</A> &bull; ";
    }
}

echo"Page $pageid of $num_pages";
$nextid=$pageid+1;

if ($pageid <$num_pages) {
    echo" &bull; <a href=\"?".uriparams()."&id=$id&page=$page&pageid=$nextid\">Next &#187;</a>";
}
echo"<BR>\n";
?>

<TABLE BORDER=0 CELLPADDING=1 CELLSPACING=1 ALIGN=CENTER STYLE="border: 0px; width: 100%">
<FORM NAME="updateusers" METHOD="POST" ACTION="?id=<?php echo"$id&pageid=$pageid&numpg=$items_per_page"; ?>&action=update">
<?writeFormKey();?>
<?php

 $sql = "SELECT * FROM `feedback` WHERE `ID`='$id' AND CommentVote IS NOT NULL ORDER BY `CommentDate`DESC LIMIT $startpoint,$items_per_page";
 $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
    while ($row = mysql_fetch_array($sql_result)) {
        $commentid = $row["CommentID"];
        $name = $row["CommentName"];
        $email = $row["email"];
        $title = $row["CommentTitle"];
        $notes = $row["CommentNote"];
        $helpful_yes = $row["helpful-yes"];
        $helpful_no = $row["helpful-no"];
        $helpful_total = $helpful_yes+$helpful_no;
        $date = date("l, F j Y g:i:sa", strtotime($row["CommentDate"]));
        $rating = $row["CommentVote"];
        if (!$title) {$title = "No Title"; }
        if (!$name) {$name = "Anonymous"; }
        if ($rating==NULL) {$rating="N/A"; }
        if ($row["flag"]=="YES") {$title .= " (flagged)"; }

   
   
   $i++;
    echo"<TR><TD COLSPAN=4><h2>$i.&nbsp;&nbsp;$title</h2></TD></TR>\n";
    echo"<TR>\n";
    echo"<TD COLSPAN=4>$notes";
    if ($helpful_total>0) {echo" ($helpful_yes of $helpful_total found this comment helpful)"; }
    echo"</TD>\n";
    echo"</TR>\n";
    echo"<TR>";
    if ($email) {
        echo"<TD>Posted by <A HREF=\"mailto:$email\">$name</A></TD>\n";
    } else {
        echo"<TD>Posted by $name</TD>\n";
    }
    echo"<TD NOWRAP>$date</TD>\n";
    echo"<TD NOWRAP>Rated $rating of 5</TD>\n";
    echo"<TD ALIGN=CENTER><INPUT NAME=\"selected_$i\" TYPE=\"CHECKBOX\" VALUE=\"$commentid\" TITLE=\"Selected User\"></TD>";
    echo"</TR>\n";
    echo"<TR>\n";
    echo"<TD colspan=3>Reason for deletion (if deleting): <INPUT NAME=\"note_$i\" TYPE=\"TEXT\" TITLE=\"Note on comment\"></TD>\n";
    echo"</TR>\n";

}

echo"<INPUT NAME=\"maxid\" TYPE=\"HIDDEN\" VALUE=\"$i\">\n";

?>
<TR>
<TD COLSPAN=4>
<h3></h3>
Found a duplicate or inappropriate comment? To Flag comments for review by Mozilla Update Staff for review, select the comment and choose "Flag Selected".<BR>
</TD>
</TR>
<TR><TD COLSPAN=4 ALIGN=RIGHT>
<?php
if ($_SESSION["level"] =="admin" or $_SESSION["level"]=="editor") {
//This user is an Admin or Editor, show the delete button.
?>
<INPUT NAME="submit" TYPE="SUBMIT" VALUE="Delete Selected" ONCLICK="return confirm('Are you sure you want to delete all selected comments?');">
<?php
}
?>
<INPUT NAME="submit" TYPE="SUBMIT" VALUE="Flag Selected" ONCLICK="return confirm('Are you sure you want to flag all selected comments for editor review?');">
</TD>
<TD>
</TR>
</FORM>
</TABLE>
<h3></h3>
<?php
// Begin Code for Dynamic Navbars
if ($pageid <=$num_pages) {
    $previd=$pageid-1;
    if ($previd >"0") {
        echo"<a href=\"?".uriparams()."&id=$id&page=$page&pageid=$previd\">&#171; Previous</A> &bull; ";
    }
}

echo"Page $pageid of $num_pages";
$nextid=$pageid+1;

if ($pageid <$num_pages) {
    echo" &bull; <a href=\"?".uriparams()."&id=$id&page=$page&pageid=$nextid\">Next &#187;</a>";
}
echo"<BR>\n";

//Skip to Page...
if ($num_pages>1) {
    echo"Jump to Page: ";
    $pagesperpage=9; //Plus 1 by default..
    $i = 01;

    //Dynamic Starting Point
    if ($pageid>11) {
        $nextpage=$pageid-10;
    }

    $i=$nextpage;

    //Dynamic Ending Point
    $maxpagesonpage=$pageid+$pagesperpage;
    //Page #s
    while ($i <= $maxpagesonpage && $i <= $num_pages) {
        if ($i==$pageid) { 
            echo"<SPAN style=\"color: #FF0000\">$i</SPAN>&nbsp;";
        } else {
            echo"<A HREF=\"?".uriparams()."&id=$id&page=$page&pageid=$i\">$i</A>&nbsp;";
        }
        $i++;
    }
}

if ($num_pages>1) {
    echo"<br>\nComments per page: \n";
    $perpagearray = array("25","50","100");
    foreach ($perpagearray as $items_per_page) {
       echo"<A HREF=\"?".uriparams()."&id=$id&page=$page&pageid=1\">$items_per_page</A>&nbsp;";
    }
}
?>

<?php
if ($_POST["submit"]=="Add Comment") {
echo"<a name=\"addcomment\"></a>\n";
echo"<h2>Submitting Comment, please wait...</h2>\n";


  if (checkFormKey()) {

    $id = escape_string($_POST["id"]);
    $name = escape_string($_POST["name"]);
    $title = escape_string($_POST["title"]);
    $comments = escape_string($_POST["notes"]);

    if ($_POST["type"]=="E") {
        $type="extensions";
    } else if ($_POST["type"]=="T") {
        $type="themes";
    }
    
    $name = "<a href=\"/$type/authorprofiles.php?id=$_SESSION[uid]\">$_SESSION[name]</a>";

    $sql = "INSERT INTO `feedback` (`ID`, `CommentName`, `CommentVote`, `CommentTitle`, `CommentNote`, `CommentDate`, `commentip`) VALUES ('$id', '$name', NULL, '$title', '$comments', NOW(NULL), '$_SERVER[REMOTE_ADDR]');";
    $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
    if ($sql_result) {
      echo"Your comment has been added successfully...<br>\n";
    } else {
      echo"There was a problem adding your comment, please try again.<br>\n";
    }
  }



}
?>

<?php /* This Section commented out until bug 333335 is fixed - https://bugzilla.mozilla.org/show_bug.cgi?id=333335

<h2>Add Comment with No Rating</h2>
Need to make a reply comment or answer a question somebody left who didn't provide an e-mail address? Use the form below. No rating is supplied and it will not affect your item's overall rating.

<form name="addcoment" method="post" action="?id=<?php echo"$id"; ?>&action=addcomment#addcomment">
<?writeFormKey();?>
  <input name="id" type="hidden" value="<?php echo"$id"; ?>">
  <input name="type" type="hidden" value="<?php echo"$type"; ?>">
  <input name="name" type="hidden" value="<?php echo"$_SESSION[name]"; ?>">
  <strong>Title:</strong> <input name="title" type="text" size="30" maxlength="150" value=""><br>
  <strong>Comment:</strong><br>
  <textarea name="notes" rows=5 cols=50></textarea><br>
<input name="submit" type="submit" value="Add Comment"></SPAN>
</form>

*/ ?>

</div>

<?php
} else if ($function=="flaggedcomments") {
?>

<?php
if ($_POST["submit"]=="Process Queue") {
echo"<h2>Processing Changes to the Flagged Comments List, please wait...</h2>\n";
    
    for ($i=1; $i<=$_POST['maxid']; $i++) {
        $action = $_POST["action_$i"];
        $commentid = escape_string($_POST["selected_$i"]); 
        if ($action=="skip") {continue;}

        if ($action=="delete" && !empty($_POST["note_$i"])) {
            $note = escape_string($_POST["note_$i"]);

            $sql = "SELECT ID FROM  `feedback` WHERE `CommentID`='$commentid'";
            $sql_result = mysql_query($sql, $connection) or trigger_error("<FONT COLOR=\"#FF0000\"><B>MySQL Error ".mysql_errno().": ".mysql_error()."</B></FONT>", E_USER_NOTICE);
            $id = mysql_result($sql_result, 0);

            //$sql = "DELETE FROM `feedback` WHERE `CommentID`='$commentid'";
            $sql = "UPDATE `feedback` SET `CommentTitle` = '<i>Removed by Staff</i>', `CommentNote` = 'Deleted because: $note', `flag`='', `CommentVote` = NULL WHERE `CommentID`='$commentid'";
            $sql_result = mysql_query($sql, $connection) or trigger_error("<FONT COLOR=\"#FF0000\"><B>MySQL Error ".mysql_errno().": ".mysql_error()."</B></FONT>", E_USER_NOTICE);
            if ($sql_result) {
                echo"Comment $commentid removed.<br>\n";
                // Update the rating for this item since it has potentially changed
                update_rating($id);
            }
	} else if ($action=="delete") {
	   echo "You must provide a reason for deleting comment $commentid<br>\n";
	} else if ($action=="clear") {
            $sql = "UPDATE `feedback` SET `flag`= '' WHERE `CommentID`='$commentid'";
            $sql_result = mysql_query($sql, $connection) or trigger_error("<FONT COLOR=\"#FF0000\"><B>MySQL Error ".mysql_errno().": ".mysql_error()."</B></FONT>", E_USER_NOTICE);
            if ($sql_result) {
                echo"Flag cleared for comment $commentid.<br>\n";
            }

        }
    }
}
unset($i);
?>

<h1>Comments Flagged for Editor Review</h1>
<TABLE BORDER=0 CELLPADDING=1 CELLSPACING=1 ALIGN=CENTER STYLE="border: 0px; width: 100%">
<?php
 $sql = "SELECT `CommentID`,`CommentName`,`email`,`CommentTitle`,`CommentNote`,`CommentDate`,`CommentVote`,`commentip`, TM.Name, TM.ID, TM.Type FROM `feedback` INNER JOIN `main` TM ON feedback.ID=TM.ID WHERE `flag`='YES' ORDER BY `CommentDate`DESC";
 $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
    $num_results = mysql_num_rows($sql_result);
    if ($num_results>"0") {
?>
<FORM NAME="updateusers" METHOD="POST" ACTION="?function=flaggedcomments&action=update">
<?writeFormKey();?>

<?php
    }
$i=0;
    while ($row = mysql_fetch_array($sql_result)) {
        $itemname = $row["Name"];
        $commentid = $row["CommentID"];
        $name = $row["CommentName"];
        $email = $row["email"];
        $title = $row["CommentTitle"];
        $notes = $row["CommentNote"];
        $type = $row["Type"];
        $theid = $row["ID"];
        $date = date("l, F j Y g:i:sa", strtotime($row["CommentDate"]));
        $rating = $row["CommentVote"];
        $commentip = $row["commentip"];
        if (!$title) {$title = "No Title"; }
        if (!$name) {$name = "Anonymous"; }
        if ($rating==NULL) {$rating="N/A"; }

   
   
   $i++;
    echo"<TR><TD COLSPAN=4><h2><a href=\"../";
    echo ($type=="E")?"extensions":"themes";
    echo "/moreinfo.php?id=$theid\">$i.&nbsp;&nbsp;$itemname :: $title</a></h2></TD></TR>\n";
    echo"<TR>\n";
    echo"<TD COLSPAN=4>$notes";
    if ($commentip) {echo"<BR>(Posted from IP: $commentip)"; }
    echo"</TD>\n";
    echo"</TR>\n";
    echo"<TR>";
    if ($email) {
        echo"<TD>Posted by <A HREF=\"mailto:$email\">$name</A></TD>\n";
    } else {
        echo"<TD>Posted by $name</TD>\n";
    }
    echo"<TD NOWRAP>$date</TD>\n";
    echo"<TD NOWRAP>Rated $rating of 5</TD>\n";
    echo"<TD>&nbsp;<INPUT NAME=\"selected_$i\" TYPE=\"hidden\" VALUE=\"$commentid\"></TD>";
    echo"</TR>\n";

    echo"<TR>\n";
    echo"<TD COLSPAN=4><input name=\"action_$i\" type=\"radio\" value=\"delete\"> Delete Comment  <input name=\"action_$i\" type=\"radio\" value=\"clear\"> Clear Flag <input name=\"action_$i\" type=\"radio\" value=\"skip\" checked> No Action</TD>\n";
    echo"</TR>\n";
    echo"<TR>\n";
    echo"<TD colspan=3>Reason for deletion (if deleting): <INPUT NAME=\"note_$i\" TYPE=\"TEXT\" TITLE=\"Note on comment\"></TD>\n";
    echo"</TR>\n";

}
if ($num_results>"0") {
echo"<INPUT NAME=\"maxid\" TYPE=\"HIDDEN\" VALUE=\"$i\">\n";

?>
<TR><TD COLSPAN=4 ALIGN=RIGHT>
<h3></h3>
<INPUT NAME="submit" TYPE="SUBMIT" VALUE="Process Queue">&nbsp;&nbsp;<INPUT name="reset" type="reset" value="Reset Form">
</TD>
<TD>
</TR>
<?php
} else {
echo"<TR><TD COLSPAN=4 align=center>No Comments are Currently Flagged for Editor Review</TD></TR>\n";
}
?>
</FORM>
</TABLE>

<?php
} else {}
?>


<!-- close #mBody-->
</div>

<?php
require_once(FOOTER);
?>
