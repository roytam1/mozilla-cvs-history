<?php
if (($_SESSION["level"] == "admin" or $_SESSION["level"] == "editor")) {
    $sql ="SELECT TM.ID FROM `main` TM INNER JOIN `version` TV ON TM.ID = TV.ID  WHERE `approved` = '?' GROUP BY `URI`";
    $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
        $queuenum = mysql_num_rows($sql_result);

    $sql = "SELECT `CommentID` FROM `feedback` WHERE `flag`='YES'";
    $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
        $commentsnum = mysql_num_rows($sql_result);
}
?>
<div id="mBody">

<div id="side">
<ul id="nav">
<li><A HREF="<?=WEB_PATH?>/developers/main.php">Overview</A></li>
<?php

if ($_SESSION['level'] == 'user') {
?>
<li><A HREF="<?=WEB_PATH?>/developers/usermanager.php">Your Profile</A></li>

<?php
} 

if ($_SESSION['level'] == 'editor') {
?>
<li><A HREF="<?=WEB_PATH?>/developers/usermanager.php?function=edituser&amp;userid=<?php echo"$_SESSION[uid]"; ?>">Your Profile</A></li>
<li><A HREF="<?=WEB_PATH?>/developers/approval.php">Approval Queue <?=$queuenum?></A></li>
<li><a href="<?=WEB_PATH?>/developers/commentsmanager.php?function=flaggedcomments">Comments Manager <?=$commentsnum?></a></li>
<li><a href="<?=WEB_PATH?>/developers/reviewsmanager.php">Reviews Manager</a></li>
<?php
} 

if ($_SESSION['level'] == 'admin') {
?>
<li><A HREF="<?=WEB_PATH?>/developers/usermanager.php?function=edituser&amp;userid=<?php echo"$_SESSION[uid]"; ?>">Your Profile</A></li>
<li><A HREF="<?=WEB_PATH?>/developers/approval.php">Approval Queue <?="($queuenum)"?></A></li>
<li><A HREF="<?=WEB_PATH?>/developers/listmanager.php?type=T">Themes list</A></li>
<li><A HREF="<?=WEB_PATH?>/developers/listmanager.php?type=E">Extensions list</A></li>
<li><A HREF="<?=WEB_PATH?>/developers/usermanager.php">Users Manager</A></li>
<li><a href="<?=WEB_PATH?>/developers/appmanager.php">Application Manager</a></li>
<li><a href="<?=WEB_PATH?>/developers/categorymanager.php">Category Manager</A></li>
<li><a href="<?=WEB_PATH?>/developers/faqmanager.php">FAQ Manager</A></li>
<li><a href="<?=WEB_PATH?>/developers/commentsmanager.php?function=flaggedcomments">Comments Manager <?="($commentsnum)"?></a></li>
<li><a href="<?=WEB_PATH?>/developers/reviewsmanager.php">Reviews Manager</a></li>
<?php } ?>
<li><a href="logout.php">Logout</A></li>
</ul>

	</div>
	<div id="mainContent">
