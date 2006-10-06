<?php
require_once('../core/init.php');
require_once('./core/sessionconfig.php');
$page_title = 'Mozilla Update :: Developer Control Panel :: T-Shirt Request';
require_once(HEADER);
require_once('./inc_sidebar.php');
require_once('./inc_tshirt.php');

if (!tshirtEligible()) {
    echo "<h1>Access Denied</h1>\n";
    echo "You are not eligible to receive a t-shirt.";
    require_once(FOOTER);
    exit;
}

$userid = mysql_real_escape_string($_SESSION['uid']);

if (isset($_POST['submit'])) {
    $name = mysql_real_escape_string($_POST['name']);
    $address1 = mysql_real_escape_string($_POST['address1']);
    $address2 = mysql_real_escape_string($_POST['address2']);
    $address3 = mysql_real_escape_string($_POST['address3']);
    $size = mysql_real_escape_string($_POST['size']);
    
    if (!empty($address1) && !empty($size)) {
        mysql_query("UPDATE userprofiles SET UserName='{$name}', UserAddress1='{$address1}', UserAddress2='{$address2}', UserAddress3='{$address3}', UserTShirtSize='{$size}' WHERE UserID='{$userid}'");
    
        $message = 'Your request has been submitted! Please be patient for delivery.';
    }
    else {
        $message = 'Error: Please fill in your address and t-shirt size.';
    }
}

$userqry = mysql_query("SELECT * FROM userprofiles WHERE UserID='{$userid}'");
$user = mysql_fetch_array($userqry);
?>
<h1>T-Shirt Request Form</h1>
<?=!empty($message) ? '<div style="font-weight: bold;">'.$message.'</div><br>' : ''?>
<div style="font-size: 12px;">You are eligible to receive a free Bon Echo Extension Team t-shirt for your contributions to Firefox Add-ons. Please complete the following form to receive your t-shirt, making sure to include your full address and country if not in the United States. Please be patient, as requests will not be processed immediately.</div><br>
<div>Current Request Status: <strong><?=!empty($user['UserTShirtSize']) ? 'Submitted' : 'Not Submitted'?></strong></div><br>
<form method="post">
<table cellpadding=2 cellspacing=0 align="center">
    <tr>
        <td>Name</td>
        <td><input type="text" name="name" size="30" value="<?=$user['UserName']?>"></td>
    </tr>
    <tr>
        <td>Address</td>
        <td><input type="text" name="address1" size="40" value="<?=$user['UserAddress1']?>"></td>
    </tr>
    <tr>
        <td>&nbsp;</td>
        <td><input type="text" name="address2" size="40" value="<?=$user['UserAddress2']?>"></td>
    </tr>
    <tr>
        <td>&nbsp;</td>
        <td><input type="text" name="address3" size="40" value="<?=$user['UserAddress3']?>"></td>
    </tr>
    <tr>
        <td>T-Shirt Size</td>
        <td>
            <select name="size">
                <option></option>
                <option value="S"<?=($user['UserTShirtSize'] == 'S') ? ' selected' : ''?>>Small</option>
                <option value="M"<?=($user['UserTShirtSize'] == 'M') ? ' selected' : ''?>>Medium</option>
                <option value="L"<?=($user['UserTShirtSize'] == 'L') ? ' selected' : ''?>>Large</option>
                <option value="2X"<?=($user['UserTShirtSize'] == '2X') ? ' selected' : ''?>>Extra Large</option>
            </select>
        </td>
    </tr>
    <tr>
        <td colspan="2" align="center"><input type="submit" name="submit" value="Submit Request"></td>
    </tr>
</table>
</form>

<!-- close #mBody-->
</div>

<?php
require_once(FOOTER);
?>
