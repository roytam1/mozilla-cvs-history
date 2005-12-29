<?php
/**
 * FAQ page.
 *
 * @package amo
 * @subpackage docs
 *
 * @todo FAQ search?
 */

startProcessing('faq.tpl',null,$compileId);
require_once('includes.php');

$db->query("
    SELECT
        `title`,
        `text`
    FROM
        `faq`
    WHERE
        `active` = 'YES'
    ORDER BY
        `index` ASC,
        `title` ASC
",SQL_ALL, SQL_ASSOC);

$faq = $db->record;

$links = array(
    array(  'href'  => './faq.php',
            'title' => 'Frequently Asked Questions', 
            'text'  => 'FAQ'),

    array(  'href'  => './policy.php',
            'title' => 'Addons Policies', 
            'text'  => 'Policy')
);

// Send FAQ data to Smarty object.
$tpl->assign(
   array(   'faq'       => $faq,
            'links'     => $links,
            'sidebar'   => 'inc/nav.tpl',
            'content'   => 'faq.tpl',
            'title'     => 'Frequently Asked Questions')
);
?>
