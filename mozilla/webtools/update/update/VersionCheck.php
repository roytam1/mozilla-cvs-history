<?php
/* -*- Mode: php; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Extension Update Service.
 *
 * The Initial Developer of the Original Code is Vladimir Vukicevic.
 * Portions created by the Initial Developer are Copyright (C) 2004
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Vladimir Vukicevic <vladimir@pobox.com>
 *  Ted Mielczarek <ted.mielczarek@gmail.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

/**
 * VersionCheck.php is a dynamic RDF that compares version information for
 * extensions and determines whether or not an update is needed.  If an update
 * is needed, the correct update file is referenced based on the UMO database
 * and repository.  The script is set to die silently instead of echoing errors
 * clients don't use anyway.  For testing, if you would like to debug, supply
 * the script with ?debug=true
 *
 * @package umo
 * @subpackage pub
 */



/*
 *  VARIABLES
 *
 *  Initialize, set up and clean variables.
 */



// Map the mysql main.type enum into the right type.
$ext_typemap = array('T' => 'theme',
                     'E' => 'extension',
                     'P' => 'plugin');

// Required variables that we need to run the script.
$required_vars = array('reqVersion',
                       'id',
                       'version',
                       'appID',
                       'appVersion');

// Array to hold errors for debugging.
$errors = array();

// Check for existence of required variables.
foreach ($required_vars as $var) {
    if (empty($_GET[$var])) {
        $errors[] = 'Required variable '.$var.' not set.'; // set debug error
    }
}

// If we have all of our data, clean it up for our queries.
if (empty($errors)) {

    // Config, connect to DB, select DB.
    require_once("../core/config.php");

    $connection = @mysql_connect(DB_HOST,DB_USER,DB_PASS);
    if (!is_resource($connection)) {
        $errors[] = 'MySQL connection failed.';
    } elseif (!@mysql_select_db(DB_NAME, $connection)) {
        $errors[] = 'Could not select MySQL database.';
    }

    // $reqVersion dictates the behavior of this script.
    // Default should be 1 until the expected behavior of this script is changed.
    // This will be apparent in a switch() statement later in this script.
    $reqVersion = intval($_GET['reqVersion']);

    // Main id for the item.
    $reqItemGuid = mysql_real_escape_string($_GET['id']);

    // Item version.
    $reqItemVersion = mysql_real_escape_string($_GET['version']);

    // ID of the client app (firefox, thunderbird, etc.).
    $reqTargetAppGuid = mysql_real_escape_string($_GET['appID']);

    // Version of that app.
    $reqTargetAppVersion = mysql_real_escape_string($_GET['appVersion']);

    // Get the os_id based on _GET; fall back  _SERVER (UA string).
    $os_id = get_os_id();



    /*
     *  QUERIES  
     *  
     *  All of our variables are cleaned.
     *  Now attempt to retrieve update information.
     */ 



    // Set up OSID piece of query based on $os_id.
    $os_query = ($os_id) ? " OR version.OSID = '{$os_id}' " : '';

    // Query for possible updates.
    $query = "
        SELECT
            main.guid AS extguid,
            main.type AS exttype,
            version.version AS extversion,
            version.uri AS exturi,
            version.minappver AS appminver,
            version.maxappver AS appmaxver,
            applications.guid AS appguid
        FROM
            main
        INNER JOIN
            version
        ON
            main.id = version.id
        INNER JOIN
            applications
        ON
            version.appid = applications.appid
        WHERE
            main.guid = '{$reqItemGuid}' AND
            applications.guid = '{$reqTargetAppGuid}' AND
            (version.OSID = 1 {$os_query} ) AND
            version.approved = 'YES' AND
            '{$reqTargetAppVersion}+' >= version.minappver AND
            '{$reqItemVersion}' <= version.version
        ORDER BY
            extversion DESC,
            version.MaxAppVer_int DESC,
            version.OSID DESC 
        LIMIT 1       
    ";

    $result = mysql_query($query);

    // Did our query execute?  Did we get any information?
    if (!is_resource($result)) {
        $errors[] = 'MySQL query for item information failed.'; 
    } elseif (mysql_num_rows($result) < 1) {
        $errors[] = 'No matching update for given item/GUID.'; 
    } else {

        // An update exists.  Retrieve it.
        $update = mysql_fetch_array($result, MYSQL_ASSOC);
        $update['exttype'] = $ext_typemap[$update['exttype']];



        /*
         *  GENERATE OUTPUT
         *
         *  If we get to this point, we're ready to output the RDF.
         *  We want to store the string then echo it if we are not debugging.
         */



        // VersionCheck.php output depends on $reqVersion.
        // Future cases could be added for future client versions.
        // Case 1: Firefox versions up to 1.0.1
        switch ($reqVersion) {
            case '1':
            default:
                // Build RDF output.
                $output = <<<OUT
<?xml version="1.0"?>
<RDF:RDF xmlns:RDF="http://www.w3.org/1999/02/22-rdf-syntax-ns#" xmlns:em="http://www.mozilla.org/2004/em-rdf#">

<RDF:Description about="urn:mozilla:{$update['exttype']}:{$reqItemGuid}">
 <em:updates><RDF:Seq>
  <RDF:li resource="urn:mozilla:{$update['exttype']}:{$reqItemGuid}:{$update['extversion']}"/>
 </RDF:Seq></em:updates>
</RDF:Description>

<RDF:Description about="urn:mozilla:{$update['exttype']}:{$update['extguid']}:{$update['extversion']}">
 <em:version>{$update['extversion']}</em:version>
 <em:targetApplication>
  <RDF:Description>
   <em:id>{$update['appguid']}</em:id>
   <em:minVersion>{$update['appminver']}</em:minVersion>
   <em:maxVersion>{$update['appmaxver']}</em:maxVersion>
   <em:updateLink>{$update['exturi']}</em:updateLink>
  </RDF:Description>
 </em:targetApplication>
</RDF:Description>

</RDF:RDF>
OUT;
                break;
        }



        /*
         *  ECHO OUTPUT
         *
         *  If we have valid RDF output set, now stream it to the client.
         */



        if (!empty($output) && $_GET['debug']!=true) {
            header("Content-type: text/xml");
            echo $output;
            exit;
        }
    }
} 



/*
 *  DEBUG
 *
 *  If we get here, something went wrong.  For testing purposes, we can
 *  optionally display errors based on $_GET['debug'].
 *
 *  By default, no errors are ever displayed because humans do not read this
 *  script.
 *
 *  Until there is some sort of API for how clients handle errors, 
 *  things should remain this way.
 */



// If we have the debug flag set and errors exist, show them.
if ($_GET['debug'] == true) {
    echo '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd">';
    echo '<html lang="en">';

    echo '<head>';
    echo '<title>VersionCheck.php Debug Information</title>';
    echo '<meta http-equiv="Content-Type" content="text/html; charset=UTF-8">';
    echo '</head>';

    echo '<body>';

    echo '<h1>Parameters</h1>';
    echo '<pre>';
    print_r($_GET);
    echo '</pre>';

    echo '<h1>Query</h1>';
    echo '<pre>';
    echo $query;
    echo '</pre>';

    if (!empty($update)) {
        echo '<h1>Result</h1>';
        echo '<pre>';
        print_r($update);
        echo '</pre>';
    }

    if (!empty($errors) && is_array($errors)) {
        echo '<h1>Errors Found</h1>';
        echo '<pre>';
        print_r($errors);
        echo '</pre>';
    } else {
        echo '<h1>No Errors Found (output below)</h1>';
        echo '<pre>';
        echo str_replace('<','&lt;',str_replace('>','&gt;',$output));
        echo '</pre>';
    }

    echo '</body>';

    echo '</html>';
}



/*
 *  FUNCTIONS
 */



/**
 * Determine the os_id based on passed OS or guess based on UA string.
 * @return int|bool $id ID of the OS in the UMO database
 */
function get_os_id()
{
    /* OS from UMO database
    2 	Linux
    3 	MacOSX
    4 	BSD
    5 	Windows
    6 	Solaris
    */

   // possible matches
    $os = array(
        'linux'=>2,
        'mac'=>3,
        'bsd'=>4,
        'win'=>5,
        'solaris'=>6
    );

    // Check UA string for a match
    foreach ($os as $string=>$id) {
        if (preg_match("/^.*{$string}.*$/i",$_SERVER['HTTP_USER_AGENT'])) {
            return $id;
        }
    }

    // If we get here, there is no defined OS
    // This OS is undetermined, and the query will instead rely on "ALL" (1) in the OR
    return false;
}
?>
