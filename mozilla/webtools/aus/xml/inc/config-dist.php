<?php
// ***** BEGIN LICENSE BLOCK *****
//
// Version: MPL 1.1/GPL 2.0/LGPL 2.1
//
// The contents of this file are subject to the Mozilla Public License Version
// 1.1 (the "License"); you may not use this file except in compliance with
// the License. You may obtain a copy of the License at
// http://www.mozilla.org/MPL/
//
// Software distributed under the License is distributed on an "AS IS" basis,
// WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
// for the specific language governing rights and limitations under the
// License.
//
// The Original Code is AUS.
//
// The Initial Developer of the Original Code is Mike Morgan.
// 
// Portions created by the Initial Developer are Copyright (C) 2006 
// the Initial Developer. All Rights Reserved.
//
// Contributor(s):
//   Mike Morgan <morgamic@mozilla.com>
//
// Alternatively, the contents of this file may be used under the terms of
// either the GNU General Public License Version 2 or later (the "GPL"), or
// the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
// in which case the provisions of the GPL or the LGPL are applicable instead
// of those above. If you wish to allow use of your version of this file only
// under the terms of either the GPL or the LGPL, and not to allow others to
// use your version of this file under the terms of the MPL, indicate your
// decision by deleting the provisions above and replace them with the notice
// and other provisions required by the GPL or the LGPL. If you do not delete
// the provisions above, a recipient may use your version of this file under
// the terms of any one of the MPL, the GPL or the LGPL.
//
// ***** END LICENSE BLOCK *****

/**
 * Configuration file.
 * @package aus
 * @subpackage inc
 * @author Mike Morgan
 */
// define('SOURCE_DIR','/home/morgamic/public_html/auslite/source');
define('SOURCE_DIR',getcwd().'/data');

// This is the directory containin channel-specific updates.
// Snippets in this directory override normal updates.
define('OVERRIDE_DIR',getcwd().'/data/3');

// Uncomment this line in order to echo text debug information.
define('DEBUG',false);

// Define default for Update blocks.
define('UPDATE_TYPE','minor');
define('UPDATE_VERSION','1.0+');
define('UPDATE_EXTENSION_VERSION','1.0+');

// Define cookie parameters
define('COOKIE_NAME', 'aus2a');
define('COOKIE_DOMAIN', 'aus2.mozilla.org');

// Turns global throttling on and off.
define('THROTTLE_GLOBAL',false);

// Define the throttle -- think gas pedal.  This determines how much the AUS application will
// stagger updates.  Pedal to the floor means we're serving all updates.  Examples:
//  10  = Updates offered only 10% of the time.
//  80  = Updates offered 80% of the time.
//  100 = Updates always offered.
define('THROTTLE_LEVEL',100);

// Turns logging throttled hits on and off.
define('THROTTLE_LOGGING',false);

// This defines explicit throttling levels.  If global throttling is on, these
// override global levels.  If it is off, this still works.  For example, this
// is 10% throttling (only 10% of the time updates are offered):
//
// $productThrottling = array(
//     'Firefox' => array(
//         '3.0' => 10,
//         '3.1' => 10
//     )
// );
// NOTE: "FULLY THROTTLED" == 0
//       "UNTHROTTLED"     == 100 (or remove the entry)
$productThrottling = array(
     'Firefox' => array(
     ),
     'Thunderbird' => array(
        '10.0' => 0,
        '10.0.1' => 0,
        '10.0.2' => 0,
        '11.0' => 0,
        '11.0.1' => 0,
        '12.0' => 0,
        '12.0.1' => 0,
        '13.0' => 0,
        '13.0.1' => 0,
        '13.0.2' => 0,
        '14.0' => 0,
     )
);

// List of exceptions for throttling.
//
// $throttleExceptions = array(
//    '3.0.11' => array(
//       'betatest',
//       'releasetest',
//       'beta'
//    )
// );
//
// In this example, 3.0.11 with channel names that match these channels will not
// be throttled unless there is a global throttle enabled.
$throttleExceptions = array(
     '10.0' => array(
         'betatest',
         'releasetest',
         'beta',
         'esr',
         'esrtest',
         'esrreleasetest',
     ),
     '10.0.1' => array(
         'betatest',
         'releasetest',
         'beta',
         'esr',
         'esrtest',
         'esrreleasetest',
     ),
     '10.0.2' => array(
         'betatest',
         'releasetest',
         'beta',
         'esr',
         'esrtest',
         'esrreleasetest',
     ),
     '11.0' => array(
         'betatest',
         'releasetest',
         'beta',
     ),
     '11.0.1' => array(
         'betatest',
         'releasetest',
         'beta',
     ),
     '12.0' => array(
         'betatest',
         'releasetest',
         'beta',
     ),
     '12.0.1' => array(
         'betatest',
         'releasetest',
         'beta',
     ),
     '13.0' => array(
         'betatest',
         'releasetest',
         'beta',
     ),
     '13.0.1' => array(
         'betatest',
         'releasetest',
         'beta',
     ),
     '13.0.2' => array(
         'betatest',
         'releasetest',
         'beta',
     ),
     '14.0' => array(
         'betatest',
         'releasetest',
         'beta',
     ),
);

// These are channels that have access to nightly updates.
// All other channels only have access to the OVERRIDE_DIR for update info.
$nightlyChannels = array(
    'nightly',
    'nightlytest',
    'nightly-tracemonkey',
    'nightly-electrolysis',
    'nightly-mozilla-2.1',
    'nightly-jaegermonkey',
    'nightly-ux',
    'nightly-maple',
    'nightly-birch',
    'nightly-ash',
    'nightly-elm',
    'nightly-ionmonkey',
    'nightly-oak',
    'nightly-profiling',
    'nightly-esr10',
    'aurora',
    'auroratest'
);

// This hash defines the product->version->patch relationships for nightlies
// It determines which patches are associated to which incoming client versions.
// @todo replace this with a better datasource that can be easily managed via a GUI.
// The ordering is !important!, given the wildcard block at the bottom.
$productBranchVersions = array(
    'Firefox'     =>  array(
        '*'       => array(
           'nightly'                 => 'mozilla-central',
           'nightlytest'             => 'mozilla-central-test',
           'nightly-tracemonkey'     => 'tracemonkey',
           'nightly-electrolysis'    => 'electrolysis',
           'nightly-jaegermonkey'    => 'jaegermonkey',
           'nightly-ux'              => 'ux',
           'nightly-maple'           => 'maple',
           'nightly-birch'           => 'birch',
           'nightly-ash'             => 'ash',
           'nightly-elm'             => 'elm',
           'nightly-ionmonkey'       => 'ionmonkey',
           'nightly-oak'             => 'oak',
           'nightly-profiling'       => 'mozilla-central',
           'nightly-esr10'           => 'mozilla-esr10',
           'aurora'                  => 'mozilla-aurora',
           'auroratest'              => 'mozilla-aurora-test'
        )
    ),
    'Fennec'      =>  array(
        '4.0*'    =>  array(
           'nightly-mozilla-2.1'     => 'mozilla-2.1'
        ),
        '*'       => array(
           'nightly'                 => 'mozilla-central',
           'aurora'                  => 'mozilla-aurora',
           'nightly-birch'           => 'mozilla-central',
           'nightly-ash'             => 'ash',
           'nightly-maple'           => 'maple',
           'nightly-oak'             => 'oak'
        )
    ),
    'Thunderbird' =>  array(
        '*'       =>   array(
           'nightly'        => 'comm-central',
           'nightlytest'    => 'comm-central-test',
           'nightly-esr10'  => 'comm-esr10',
           'aurora'         => 'comm-aurora',
           'auroratest'     => 'comm-aurora-test'
        ),
    ),
);

// Specify which release should be used for channel-changers wanting to go to 
// release or beta channels.

$latestRelease = array(
    'Firefox' => array(
        'beta' => '5.0',
        'release' => '5.0'
    )
);

// Config for memcache.
define('MEMCACHE_NAMESPACE', 'aus'); // set memcache namespace.  Keep this string as short and simple as possible.
define('MEMCACHE_EXPIRE', 1800); // how long items are stored in memcache
define('MEMCACHE_ON', false); // whether or not to cache ever

/**
 * Memcache configuration.
 * See http://php.oregonstate.edu/memcache for info.
 */
$memcache_config = array(
    'localhost' => array(
       'port' => '11211',
       'persistent' => true,
       'weight' => '1',
       'timeout' => '1',
       'retry_interval' => 15
    )
);

/*
 * Array that defines which %OS_VERSION% values are no longer supported.
 * Applies to all updates and uses the version of the update to be 
 * served to determine blocking (bug 666735). Previously only applied to
 * major updates, and used the version from the incoming URI (bug 418129)
 * Use of this array is in inc/patch.class.php.  
 *
 * The Array format is considered to be:
 * array(
 *      $Product => array(
 *          $Version => array(
 *              $OS_VERSION
 *          )
 *      )
 *  )
 *
 * $Product is the product name (Firefox, Thunderbird, etc. - %PRODUCT%).
 *
 * $Version is a string which identifies some set of releases
 *   '1.0.2'  - an exact version
 *   '1.0*'   - all versions starting '1.0' (via a regexp)
 *   '1.0b3+' - all versions from 1.0b3 onwards (via php's version_compare)
 *
 * $OS_VERSION is used in a string match (existence anywhere in passed
 * %OS_VERSION% triggers blocklisting of that OS).
 */
$unsupportedPlatforms = array(
    'Firefox'     =>  array(
        // Mac 10.2/10.3, Win < 2k, GTK < 2.10 - bug 418129
        '3.0b1+' => array(
            'Darwin 6',
            'Darwin 7',
            'Windows_95',
            'Windows_98',
            'Windows_NT 4',
            'GTK 2.0.',
            'GTK 2.1.',
            'GTK 2.2.',
            'GTK 2.3.',
            'GTK 2.4.',
            'GTK 2.5.',
            'GTK 2.6.',
            'GTK 2.7.',
            'GTK 2.8.',
            'GTK 2.9.'
        ),
        // Mac 10.4 - bug 640044
        // See index.php for PPC
        '4.0b1+' => array(
            'Darwin 8'
        ),
        // RHEL5 has too old libstdc++ - bug 655917
        // Fx6 will ship with --enable-stdcxx-compat
        '4.0*' => array(
            '.el5'
        ),
        '5.0*' => array(
            '.el5'
        ),
        // Too old freetype - bug 666735
        '7.0a2+' => array(
            'GTK 2.10.'
        ),
        // Block Win2000, XP RTM & SP1 after switching to MSVC2010
        '13.0a1+' => array (
            'Windows_NT 5.0',
            'Windows_NT 5.1.0',
            'Windows_NT 5.1.1',
        ),
        // Mac 10.5 - bug 772735
        '17.0a1+' => array (
            'Darwin 9'
        ),
    ),
    'Thunderbird'     =>  array(
        // Mac 10.2/10.3, Win < 2k, GTK < 2.10 - bug 418129
        '3.0a1+' => array(
            'Darwin 6',
            'Darwin 7',
            'Windows_95',
            'Windows_98',
            'Windows_NT 4',
            'GTK 2.0.',
            'GTK 2.1.',
            'GTK 2.2.',
            'GTK 2.3.',
            'GTK 2.4.',
            'GTK 2.5.',
            'GTK 2.6.',
            'GTK 2.7.',
            'GTK 2.8.',
            'GTK 2.9.'
        ),
        // Mac 10.4 - bug 640044
        // See index.php for PPC
        '4.0b1+' => array(
            'Darwin 8'
        ),
        // RHEL5 has too old libstdc++ - bug 655917
        '4.0*' => array(
            '.el5'
        ),
        '5.0*' => array(
            '.el5'
        ),
        // Too old freetype - bug 666735
        '7.0a1+' => array(
            'GTK 2.10.'
        ),
        // Block Win2000, XP RTM & SP1 after switching to MSVC2010
        '13.0a1+' => array (
            'Windows_NT 5.0',
            'Windows_NT 5.1.0',
            'Windows_NT 5.1.1',
        ),
        // Mac 10.5 - bug 772735
        '17.0a1+' => array (
            'Darwin 9'
        ),
    )
);
?>
