<?php
require_once('../core/init.php');
require_once('./core/sessionconfig.php');
$page_title = 'Mozilla Update :: Developer Control Panel :: Item Previews';
require_once(HEADER);
require_once('inc_sidebar.php');

//Kill access to items this user doesn't own...
if ($_SESSION["level"] !=="admin" and $_SESSION["level"] !=="editor") {

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

//List Page Preview Variables
// defaults
$preview_width = "200";
$preview_height = "150";
// limits
$preview_width_max = "690";
$preview_height_max = "520";
?>
<?php
$id = escape_string($_GET["id"]);
$sql = "SELECT  TM.ID, TM.Type, TM.Name, TM.Description, TM.downloadcount, TM.TotalDownloads, TM.Rating, TU.UserEmail FROM  `main`  TM 
LEFT JOIN authorxref TAX ON TM.ID = TAX.ID
INNER JOIN userprofiles TU ON TAX.UserID = TU.UserID
WHERE TM.ID = '$id' ORDER  BY  `Type` , `Name` ";
 $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
  $numresults = mysql_num_rows($sql_result);
  $row = mysql_fetch_array($sql_result);
$v++;
    $id = $row["ID"];
    $type = $row["Type"];
    $name = $row["Name"];
    $dateadded = $row["DateAdded"];
    $dateupdated = $row["DateUpdated"];
    $homepage = $row["Homepage"];
    $description = nl2br($row["Description"]);
    $authors = $row["UserEmail"];
    $downloadcount = $row["downloadcount"];
    $totaldownloads = $row["TotalDownloads"];
    $rating = $row["Rating"];
?>

<h2>Item Previews :: <?php echo"$name"; ?></h2>
<?php
if ($_POST["submit"]=="Update Previews") {
for ($i = 1; $i <= $_POST["maxid"]; $i++) {
$previewid = escape_string($_POST["previewid_$i"]);
$caption = escape_string($_POST["caption_$i"]);
$delete = escape_string($_POST["delete_$i"]);



$sql = "SELECT `PreviewURI` from `previews` WHERE `PreviewID`='$previewid' LIMIT 1";
  $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
    $row = mysql_fetch_array($sql_result);
    $file = $row["PreviewURI"];
    $file = FILE_PATH.'/'.$file;
    
    $imagesize = @getimagesize($file);
if ($_POST["preview"]==$previewid AND $imagesize[0]<="$preview_width" AND $imagesize[1]<="$preview_height") {$preview="YES"; } else {$preview="NO";}
    
if ($delete==$previewid) {
  if (file_exists($file)) {  unlink($file); }

$sql = "DELETE FROM `previews` WHERE `PreviewID`='$previewid'";
} else {
$sql = "UPDATE `previews` SET `caption`='$caption', `preview`='$preview' WHERE `PreviewID`='$previewid'";
}
  if (checkFormKey()) {
    $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
  }
}

echo"Previews successfully updated. The new values for the preview records should be shown below.<br>\n";

}


unset($i);
$sql = "SELECT * FROM `previews` TP WHERE `ID`='$id' ORDER BY `PreviewID`";
 $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);
  $num_results = mysql_num_rows($sql_result);

if ($num_results>"0") {
?>
<form name="updatepreviews" method="post" action="?id=<?php echo"$id"; 
?>&amp;function=updatepreviews">
<?writeFormKey();?>
<?php
}

  while($row = mysql_fetch_array($sql_result)) {
    $i++;
    $previewid = $row["PreviewID"];
    $uri = $row["PreviewURI"];
    $filename = basename($row["PreviewURI"]);
    $filename_array[$i] = $filename;
    $caption = $row["caption"];
    $preview = $row["preview"];
    list($src_width, $src_height, $type, $attr) = getimagesize(FILE_PATH.'/'.$uri);
    $filesize = round(filesize(FILE_PATH.$uri)/1024,1);
    $popup_width = $src_width+25;
    $popup_height = $src_height+25;

    //Scale Image Dimensions
    $dest_width="200"; // Destination Width /$tn_size_width
    $dest_height_fixed="150"; // Destination Height / $tn_size_height (Fixed)
    if ($src_width<=$dest_width AND $src_height<=$dest_width) {
    $dest_width = $src_width;
    $dest_height = $src_height;
    $nopopup = true;
    } else {
        $dest_height= ($src_height * $dest_width) / $src_width; // (Aspect Ratio Variable Height
        if ($dest_height>$dest_height_fixed) {
            $dest_height = $dest_height_fixed;
            $dest_width = ($src_width * $dest_height) / $src_height;
        }
    }

echo"<h3>Preview #$i - $caption </h3>\n";
echo"$filename ($src_width x $src_height) $filesize"."Kb"; if ($preview=="YES") {echo"  (List Page Preview)";}echo"<br>\n";
if ($nopopup != "true") {echo"<a href=\"javascript:void(window.open('".WEB_PATH."$uri', '', 'scrollbars=yes,resizable=yes,width=$popup_width,height=$popup_height,left=30,top=20'));\">";}
echo"<img src=\"".WEB_PATH."$uri\" border=\"0\" width=\"$dest_width\" height=\"$dest_height\" alt=\"$caption\" title=\"$caption\">";
if ($nopopup != "true") {echo"</a>";}
echo"<br>\n";
echo"<input name=\"previewid_$i\" type=\"hidden\" value=\"$previewid\">\n";
echo"Edit Caption: <input name=\"caption_$i\" type=\"text\" value=\"$caption\" size=30>";
if ($preview == "YES" OR ( $src_width<="$preview_width" AND $src_height <="$preview_height")) {
echo"&nbsp;&nbsp;<input name=\"preview\" type=\"radio\" value=\"$previewid\""; if ($preview=="YES") {echo" checked"; } echo">List Page Preview?<br>\n";
} else { echo"<input name=\"preview_null\" type=\"radio\" value=\"\" disabled=\"true\"><span class=\"tooltip\" TITLE=\"Option Disabled. Max Size for Previews is $preview_width x $preview_height\">List Item Preview?</span><br>\n"; }
echo"Delete File? <input name=\"delete_$i\" type=\"checkbox\" value=\"$previewid\"><br>\n";
}
?>
<?php if ($num_results>"0") { ?>
<input name="maxid" type="hidden" value="<?php echo"$i"; ?>">
<br><input name="submit" type="submit" value="Update Previews"><input name="reset" type="reset" value="Reset">
</form>
<?php } ?>

<h3>Add Preview</h3>
<?php
//Add Photo Function
if ($_POST["submit"]=="Add Preview") {

//Compute the maxval for the filename for the new file.
if (!$filename_array) { $filename_array=array(); }
foreach ( $filename_array as $filename) {
$exploded_filename = explode("-", $filename);
$count = count($exploded_filename)-1;
$val = explode(".",$exploded_filename[$count]);
$val = $val[0];
if (!$maxval or $maxval<$val) {$maxval=$val;}
}
$maxval = $maxval+1;
$i=$maxval;
unset($count,$val,$exploded_filename,$filename,$maxval);

$width = $_POST["width"];
$height = $_POST["height"];
$preview = escape_string($_POST["preview"]);
$caption = escape_string($_POST["caption"]);

$name = str_replace(" ","_",$name);
$previewpath = strtolower("images/previews/$name-$i.jpg");

if ($preview=="YES") {
$width = "$preview_width";
$height = "$preview_height";
} else {
$preview="NO";
}

$filename=$_FILES['file']['name'];
$filetype=$_FILES['file']['type'];
$filesize=$_FILES['file']['size'];
$uploadedfile=$_FILES['file']['tmp_name'];
$status=$_FILES['file']['error'];

//Status
if ($status==0) {$statusresult="Success!";
} else if ($status==1) {$statusresult="Error: File Exceeds upload_max_filesize (PHP)";
} else if ($status==2) {$statusresult="Error: File Exceeds max_file_size (HTML)";
} else if ($status==3) {$statusresult="Error: File Incomplete, Partial File Received";
} else if ($status==4) {$statusresult="Error: No File Was Uploaded";
}


//Now to make the Image...
$sourcepath="$uploadedfile"; // Source Image Path -- Destination from FileUpload()
$srcimagehw = @GetImageSize($sourcepath); //GetImage Info -- Source Image
$src_width="$srcimagehw[0]"; // Source Image Width
$src_height="$srcimagehw[1]"; // Source Image Height
$type = $srcimagehw[2];

if (!$width) {$width=$src_width;}
if (!$height) {$height=$src_height;}

if ($type=="2" or $type=="3") {

//Destination Properties for the Display Image
//Output Image Dimensions

//Limit Max.
if ($width > $preview_width_max) {$width=$preview_width_max;}
if ($height > $preview_height_max) {$height=$preview_height_max;}

$dest_width="$width"; // Destination Width /$tn_size_width
$dest_height_fixed="$height"; // Destination Height / $tn_size_height (Fixed)
$dest_height= ($src_height * $dest_width) / $src_width; // (Aspect Ratio Variable Height
if ($dest_height>$dest_height_fixed) {
$dest_height = $dest_height_fixed;
$dest_width = ($src_width * $dest_height) / $src_height;
}
$quality="80"; // JPEG Image Quality
$outputpath=FILE_PATH.'/'.$previewpath; //path of output image ;-)

if ($type=="2") {
$src_img = imagecreatefromjpeg("$sourcepath");
} else if ($type=="3") {
$src_img = imagecreatefrompng("$sourcepath");
$quality="95";
}
$dst_img = imagecreatetruecolor($dest_width,$dest_height);
imagecopyresampled($dst_img, $src_img, 0, 0, 0, 0, $dest_width, $dest_height, $src_width, $src_height);
imageinterlace($dst_img, 1);
$white = ImageColorAllocate($dst_img, 255, 255, 255);
//Make a couple of size corrections for the auto-border feature..
$dest_width = $dest_width-1;
$dest_height = $dest_height-1;
imagerectangle ($dst_img, 0, 0, $dest_width, $dest_height, $white);
$status = imagejpeg($dst_img, "$outputpath", $quality);
imagedestroy($src_img);
imagedestroy($dst_img);

if ($status=="1") {
//Lets attempt to add the record to the DB.
 if (checkFormKey()) {
  $sql = "INSERT INTO `previews` (`PreviewURI`,`ID`,`caption`,`preview`) VALUES ('/$previewpath','$id','$caption','$preview');";
  $sql_result = mysql_query($sql, $connection) or trigger_error("MySQL Error ".mysql_errno().": ".mysql_error()."", E_USER_NOTICE);

  if ($sql_result=="1") {
   echo"Your File $filename ($filesize bytes) has been successfully uploaded and added to the database. <a href=\"?id=$id\">Click here</a> to refresh this page to show the added entry for editing.<BR><BR>";
 }
}

}
} else {
echo"<span class=error>The image you uploaded has errors and could not be processed successfully. The image may be corrupt or not in the correct format. This tool only supports jpeg and png images. Please try your operation again.</span><br><br>\n";

}

}

?>

<form name="addpreview" method="post" action="?id=<?php echo"$id"; 
?>&amp;function=addpreview" enctype="multipart/form-data">
<?writeFormKey();?>
Only PNG or JPG images are supported for addition to the preview screenshots page for your item. Check the "List Page Preview" box
if you'd like the image to be featured on the list and the top of the item details pages. To just have it added to your screenshots
page, leave the box unchecked<br>
<input name="file" SIZE=30 type="file"> <input name="preview" type="checkbox" value="YES">List Page Preview?<br>

Image Width:<input name="width" type="text" value="0" size=5> x Height: <input name="height" type="text" value="0" size=5><br>
Fill in the image width and height fields above to have the site resize 
your image for you. Otherwise, leave Width and Height as 0 (the default) 
for full-size (limited to <?php echo $preview_width_max . " x " 
. $preview_height_max; ?>, anything larger will be 
auto-resized). This setting is ignored for preview images, which have 
strict size requirements of <?php echo $preview_width . " x " . 
$preview_height; ?>.<br><br> 
Image Caption: <input name="caption" type="text" size="30"><br>

<input name="submit" type="submit" value="Add Preview"><input name="reset" type="reset" value="Reset">
</form>



<!-- close #mBody-->
</div>

<?php
require_once(FOOTER);
?>
