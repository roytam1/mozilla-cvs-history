<?php
/**
 * Home page for extensions, switchable on application.
 *
 * @package amo
 * @subpackage docs
 * @todo make this dynamic based on an SQL field (recommended)
 */

startProcessing('recommended-new.tpl', 'recommended', $compileId, 'rustico');
require_once('includes.php');

// If app is not set or empty, set it to null for our switch.
$_GET['app'] = (!empty($_GET['app'])) ? $_GET['app'] : null;

// Determine our application.
switch( $_GET['app'] ) {
    case 'mozilla':
        $clean['app'] = 'Mozilla';
        break;
    case 'thunderbird':
        $clean['app'] = 'Thunderbird';
        break;
    case 'firefox':
    default:
        $clean['app'] = 'Firefox';
        break;
}

// $sql['app'] can equal $clean['app'] since it was assigned in a switch().
// We have to ucfirst() it because the DB has caps.
$sql['app'] = $clean['app'];

$amo = new AMO_Object();

// Assign template variables.
$tpl->assign(
    array(  'recommended'       => $amo->getRecommendedAddons($sql['app'],'E',100),
            'title'             => 'Recommended Firefox Add-ons',
            'content'           => 'recommended.tpl')
);
?>
