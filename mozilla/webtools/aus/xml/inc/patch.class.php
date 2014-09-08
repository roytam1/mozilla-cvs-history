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

define('DEFAULT_SNIPPET_SCHEMA_VER', 0);
define('SNIPPET_SCHEMA_VER_1', 1);
define('SNIPPET_SCHEMA_VER_2', 2);

/**
 * AUS Patch class.
 * @package aus
 * @subpackage inc
 * @author Mike Morgan
 *
 * This class is for handling patch objects.
 * These carry relevant information about partial or complete patches.
 */
class Patch extends AUS_Object {

    // Patch metadata.
    var $type;
    var $url;
    var $hashFunction;
    var $hashValue;
    var $size;
    var $build;

    // Array that maps versions onto their respective branches.
    var $productBranchVersions;

    // Array the defines which channels are flagged as 'nightly' channels.
    var $nightlyChannels;

    // Array that defines which releases are considered the latest, for channel-changing purposes.
    var $latestRelease;

    // Valid patch flag.
    var $isPatch;

    // Is this patch a complete or partial patch?
    var $patchType;

    // Update metadata, read from patch file.
    var $updateType;
    var $updateVersion;
    var $updateExtensionVersion;
    var $licenseUrl;
    var $billboardUrl;
    var $showPrompt;
    var $showNeverForVersion;
    var $showSurvey;
    var $actions;
    var $open;
    var $notification;
    var $alert;
    var $promptWaitTime;
    var $displayVersion;
    var $appVersion;
    var $platformVersion;
    var $snippetSchemaVersion;
    
    // Do we have Update metadata information?
    // FIXME hasUpdateInfo not used by snippet schema v2, needs refactoring
    var $hasUpdateInfo;
    var $hasDetailsUrl;
    var $hasBillboardUrl;
    var $hasShowPrompt;
    var $hasShowNeverForVersion;
    var $hasShowSurvey;
    var $hasActions;
    var $hasOpenUrl;
    var $hasNotificationUrl;
    var $hasAlertUrl;
    var $hasPromptWaitTime;
    var $hasDisplayVersion;
    var $hasAppVersion;
    var $hasPlatformVersion;
    var $hasSnippetSchemaVersion;

    // Is this a channel-changing request?
    var $isChangingChannel;

    /**
     * Constructor.
     */
    function Patch($productBranchVersions=array(),$nightlyChannels,$type='complete',$latestRelease=null) {
        $this->setProductBranchVersions($productBranchVersions);
        $this->setNightlyChannels($nightlyChannels);
        $this->setVar('latestRelease',$latestRelease);
        $this->setVar('isPatch',false);
        $this->setVar('isChangingChannel',false);
        $this->setVar('patchType',$type);
        $this->setVar('updateType','minor');
        // FIXME hasUpdateInfo not used by snippet schema v2, needs refactoring
        $this->setVar('hasUpdateInfo',false);
        $this->setVar('hasDetailsUrl',false);
    }

    /**
     * Set the filepath for the snippet based on product/platform/locale and
     * SOURCE_DIR, which is set in config.
     *
     * @param string $product
     * @param string $platform
     * @param string $locale
     * @param string $version
     * @param string $build
     * @param string $buildSource
     * @param string $channel
     *
     * @return boolean
     */
    function setPath ($product,$platform,$locale,$version=null,$build,$buildSource,$channel) {
        switch($buildSource) {
            case 3:
                return $this->setVar('path',OVERRIDE_DIR.'/'.$product.'/'.$version.'/'.$platform.'/'.$build.'/'.$locale.'/'.$channel.'/'.$this->patchType.'.txt',true);
                break;
            case 2:
                return $this->setVar('path',SOURCE_DIR.'/'.$buildSource.'/'.$product.'/'.$version.'/'.$platform.'/'.$build.'/'.$locale.'/'.$this->patchType.'.txt',true);
                break;
        }
        return false;
    }

    /**
     * Read the given file and store its contents in our Patch object.
     *
     * @param string $path
     *
     * @return boolean
     */
    function setSnippet ($path) {

        // Default to the old Schema type
        $snippetSchemaVersion = DEFAULT_SNIPPET_SCHEMA_VER;

        // Attempt to read the file.  Return false on failure, since there's nothing left to do.
        if (!($file = explode("\n",file_get_contents($path,true)))) {
            return false;
        }

        if (preg_match('/^version=(\d+)$/', $file[0], $matches)) {
            $snippetSchemaVersion = $matches[1];
        }

        $this->setVar('snippetSchemaVersion',$snippetSchemaVersion,true);
        if (isset($this->snippetSchemaVersion)) {
            $this->setVar('hasSnippetSchemaVersion', true, true);
        }

        /**
         * This is the "legacy" way of reading snippet files.  This will be phased out.
         */
        if (DEFAULT_SNIPPET_SCHEMA_VER == $snippetSchemaVersion) {

            $this->setVar('version', DEFAULT_SNIPPET_SCHEMA_VER);
            $this->setVar('type',$file[0]);
            $this->setVar('url',$file[1]);
            $this->setVar('hashFunction',$file[2]);
            $this->setVar('hashValue',$file[3]);
            $this->setVar('size',$file[4]);
            $this->setVar('build',$file[5]);

            // Attempt to read update information.
            if ($this->isComplete() && isset($file[6]) && isset($file[7])) {
                $this->setVar('updateVersion',$file[6],true);
                $this->setVar('updateExtensionVersion',$file[7],true);
                // FIXME hasUpdateInfo not used by snippet schema v2, needs refactoring
                $this->setVar('hasUpdateInfo',true,true);
            }
            
            // Pull update metadata if it exists, and the patch is a complete patch.
            if ($this->isComplete() && isset($file[8])) {
                $this->setVar('detailsUrl',$file[8],true);
                $this->setVar('hasDetailsUrl',true,true);
            }
            
            $this->setVar('isPatch',true,true);
            return true;
        } 
        
        /**
         * This is an attribute-driven build snippet schema.  Data will be in the form:
         *      key=value
         *      foo=bar
         *      bar=foo
         */
        elseif (SNIPPET_SCHEMA_VER_1 == $snippetSchemaVersion) {

            // For each line in the file, read the key=value pairing.
            //
            // If the pairing is matches our expected schema format, set them appropriately.
            foreach ($file as $row) {
                if (preg_match('/^(\w+)=(.+)$/', $row, $matches)) {
                    $snippetKey = $matches[1];
                    $snippetValue = $matches[2];
                    $this->setVar($snippetKey, $snippetValue, true);
                }
            }

            // Store information found only in complete snippets.
            // This information is tied to the <update> element.
            if ($this->isComplete()) {
                if (isset($this->appv) && isset($this->extv)) {
                    $this->setVar('updateVersion', $this->appv, true);
                    $this->setVar('updateExtensionVersion', $this->extv, true);
                    // FIXME hasUpdateInfo not used by snippet schema v2, needs refactoring
                    $this->setVar('hasUpdateInfo', true, true);
                }

                if (isset($this->detailsUrl)) {
                    $this->setVar('hasDetailsUrl', true, true);
                }
            }

            $this->setVar('isPatch',true,true);
            return true;
        }
        elseif (SNIPPET_SCHEMA_VER_2 == $snippetSchemaVersion) {

            // For each line in the file, read the key=value pairing.
            //
            // If the pairing is matches our expected schema format, set them appropriately.
            foreach ($file as $row) {
                if (preg_match('/^(\w+)=(.+)$/', $row, $matches)) {
                    $snippetKey = $matches[1];
                    $snippetValue = $matches[2];
                    $this->setVar($snippetKey, $snippetValue, true);
                }
            }

            // Store information found only in complete snippets.
            // This information is tied to the <update> element.
            if ($this->isComplete()) {
                # so that isSupported keeps working
                if (isset($this->appVersion)) {
                    $this->setVar('updateExtensionVersion', $this->appVersion, true);
                }

                if (isset($this->detailsUrl)) {
                    $this->setVar('hasDetailsUrl', true, true);
                }

                if (isset($this->billboardURL)) {
                    $this->setVar('hasBillboardUrl', true, true);
                }

                if (isset($this->showPrompt)) {
                    $this->setVar('hasShowPrompt', true, true);
                }

                if (isset($this->showNeverForVersion)) {
                    $this->setVar('hasShowNeverForVersion', true, true);
                }

                if (isset($this->showSurvey)) {
                    $this->setVar('hasShowSurvey', true, true);
                }

                if (isset($this->actions)) {
                    $this->setVar('hasActions', true, true);
                }

                if (isset($this->openURL)) {
                    $this->setVar('hasOpenUrl', true, true);
                }

                if (isset($this->notificationURL)) {
                    $this->setVar('hasNotificationUrl', true, true);
                }

                if (isset($this->alertURL)) {
                    $this->setVar('hasAlertUrl', true, true);
                }

                if (isset($this->promptWaitTime)) {
                    $this->setVar('hasPromptWaitTime', true, true);
                }

                if (isset($this->displayVersion)) {
                    $this->setVar('hasDisplayVersion', true, true);
                }

                if (isset($this->appVersion)) {
                    $this->setVar('hasAppVersion', true, true);
                }

                if (isset($this->platformVersion)) {
                    $this->setVar('hasPlatformVersion', true, true);
                }
            }

            $this->setVar('isPatch',true,true);
            return true;
        } else {
            error_log("Unknown snippet schema version: $snippetSchemaVersion");
            return false;
        }
    }

    /**
     * Attempt to read and parse the designated source file.
     * How and where the file is read depends on the client version.
     *
     * For more information on why this is a little complicated, see:
     * https://intranet.mozilla.org/AUS:Version2:Roadmap:Multibranch
     *
     * @param string $product
     * @param string $platform
     * @param string $locale
     * @param string $version
     * @param string $build
     * @param string $channel
     *
     * @return boolean
     */
    function findPatch($product,$platform,$locale,$version,$build,$channel=null) {

        // If a specific update exists for the specified channel, it takes priority over the branch update.
        if (!$this->isChangingChannel() && !empty($channel) && $this->setPath($product,$platform,$locale,$version,$build,3,$channel) && file_exists($this->path) && filesize($this->path) > 0) {
            return $this->setSnippet($this->path); 
        } 

        // If our channel matches our regexp for fallback channels, let's try to fallback.
        //
        // In our condition below, we check to see that the normal update path doesn't have a file that exists.
        //
        // If the file does exist, we don't ever fall back, which is the hacky way to stop the fallback behavior,
        // but the only way we have so far.
        if (!$this->isChangingChannel() && !empty($channel) && $this->setPath($product,$platform,$locale,$version,$build,3,$channel) && !file_exists($this->path) && preg_match('/^[\w\-]+\-cck\-.[\w\-\.]+$/',$channel)) {

            // Partner fallback channel to be used if the partner-specific update doesn't exist or work.
            $buf = array();
            $buf = split('-cck-',$channel);
            $fallbackChannel = $buf[0];
        
            // Do a check for the fallback update.  If we find a valid fallback update, we can offer it. 
            if (!empty($fallbackChannel) && $this->setPath($product,$platform,$locale,$version,$build,3,$fallbackChannel) && file_exists($this->path) && filesize($this->path) > 0) {
                return $this->setSnippet($this->path);
            }
        } 

        // Determine the branch of the client's version.
        $branchVersion = $this->getBranch($product,$version,$channel);

        if ($this->isComplete()) {
            // If it is a complete patch and a nightly channel, force the complete update to take the user to the latest build.
            if ($this->isNightlyChannel($channel) || $this->isChangingChannel()) {
                $buildSource = 3;
                if ($this->isNightlyChannel($channel)) {
                    $buildSource = 2;
                } else {
                   if (isset($this->latestRelease[$product][$channel])) {
                       $branchVersion = $this->latestRelease[$product][$channel];
                       if (!$branchVersion) {
                           return false;
                       }
                   }
                }

                // Get the latest build that has an update for this branch.
                $latestCompleteBuild = $this->getLatestCompleteBuild($product,$branchVersion,$platform,$locale,$buildSource,$channel);
    
                // If we have the latest complete build, the path is valid, the file exists, and the filesize is greater than zero, we have a valid complete patch.
                if ($latestCompleteBuild && $this->setPath($product,$platform,$locale,$branchVersion,$latestCompleteBuild,$buildSource,$channel) && file_exists($this->path) && filesize($this->path) > 0) {
                    $this->setSnippet($this->path); 
                    if ($this->isChangingChannel()) {
                        return true;
                    } elseif ($this->isNightlyChannel($channel)) {
                        return $this->isNewerBuild($build);
                    }
                }
            }
        }

        // Otherwise, check for the partial snippet info.  If an update exists, pass it along.
        if ($this->isPartial() && $this->isNightlyChannel($channel) && $this->setPath($product,$platform,$locale,$branchVersion,$build,2,$channel) && file_exists($this->path) && filesize($this->path) > 0) {
                $this->setSnippet($this->path); 
                return $this->isNewerBuild($build);
        } 

        // Note: Other data sets were made obsolete in 0.6.  May incoming/0,1 rest in peace.

        // If we get here, we know for sure that no updates exist for the current request..
        // Return false by default, which prompts the "no updates" XML output.
        return false;
    }

    /**
     * Compare passed build to build in snippet.
     * Returns true if the snippet build is newer than the client build.
     *
     * @param string $build
     * @return boolean
     */
    function isNewerBuild($build) {
        return ($this->build>$build);
    }

    /**
     * Set the product & branch versions array.
     *
     * @param array $productBranchVersions
     * @return boolean
     */
    function setProductBranchVersions($productBranchVersions) {
        return $this->setVar('productBranchVersions',$productBranchVersions);
    }

    /**
     * Set the nightly channels array.
     *
     * @param array $branchVersions
     * @return boolean
     */
    function setNightlyChannels($nightlyChannels) {
       return $this->setVar('nightlyChannels',$nightlyChannels);
    }

    /**
     * Determine whether or not the given channel is flagged as nightly.
     * 
     * @param string $channel
     *
     * @return bool
     */
    function isNightlyChannel($channel) {
        return in_array($channel,$this->nightlyChannels);
    }

    /**
     * Set whether this is a channel-changing request.
     *
     * @param boolean $option
     * @return boolean
     */
    function setChangingChannel($option) {
       return $this->setVar('isChangingChannel', $option, true);
    }

    /**
     * Determine whether or not this is a channel-changing request.
     * 
     * @return boolean
     */
    function isChangingChannel() {
        return $this->isChangingChannel;
    }

    /**
     * Determine whether or not the incoming version matches a (wildcard) version pattern
     *
     * @param string $version
     * @param string $versionPattern
     * @return boolean
     */
    function isMatchingVersion($version,$versionPattern) {
        // 1.0b3+ style, everything from 1.0b3 upwards
        if (substr($versionPattern, -1) === '+' &&  
            version_compare($version, $versionPattern, '>=')) {
            return true;
        // '1.0b3' (plain) style, only that version
        } elseif (strpos($versionPattern, '*') === false &&
                  $versionPattern == $version) {
            return true;
        // * wildcard style, eg '1.0*' is anything starting with 1.0 
        } elseif (preg_match('/^'. str_replace('\\*', '.*', preg_quote($versionPattern, '/')) .'$/', $version)) {
             return true;
        } 
        return false;
    }

    /**
     * Determine whether or not the incoming version is a product BRANCH.
     *
     * @param string $product
     * @param string $version
     * @param string $channel
     * @return string|false
     */
    function getBranch($product,$version,$channel) {
        if (!isset ($this->productBranchVersions[$product])) {
            return false;
        }
        foreach ($this->productBranchVersions[$product] as $versionPattern => $branches) {
            if ($this->isMatchingVersion($version,$versionPattern)) {
                if (is_string($branches) && ($channel == 'nightly')) {
                    return $branches;
                } elseif (is_array($branches) && isset($branches[$channel])) {
                    return $branches[$channel];
                }
            }
        }
        return false;
    }

    /**
     * Determine whether or not something is Trunk.
     *
     * @param string $version
     * @return boolean
     */
    function isTrunk($version) {
       return ($version == 'trunk') ? true : false;
    }

    /**
     * Does this object contain a valid patch file?
     * @return boolean
     */
    function isPatch() {
        return $this->isPatch;
    }

    /**
     * Determine whether or not this patch is a complete patch.
     */
    function isComplete() {
       return $this->patchType === 'complete';
    }

    /**
     * Determine whether or not this patch is a partial patch.
     */
    function isPartial() {
       return $this->patchType === 'partial';
    }


    /**
     * Determine whether or not this patch has a details URL.
     * @TODO Make this a bit more graceful, possibly join it with hasUpdateInfo, or find a better way.
     */
    function hasDetailsUrl() {
       return $this->hasDetailsUrl;
    }

    /**
     * Determine whether or not this patch has a billboard URL.
     */
    function hasBillboardUrl() {
       return $this->hasBillboardUrl;
    }

    /**
     * Determine whether or not this patch has showPrompt set.
     */
    function hasShowPrompt() {
       return $this->hasShowPrompt;
    }

    /**
     * Determine whether or not this patch has showNeverForVersion set.
     */
    function hasShowNeverForVersion() {
       return $this->hasShowNeverForVersion;
    }

    /**
     * Determine whether or not this patch has showSurvey set.
     */
    function hasShowSurvey() {
       return $this->hasShowSurvey;
    }

    /**
     * Determine whether or not this patch has actions set.
     */
    function hasActions() {
       return $this->hasActions;
    }

    /**
     * Determine whether or not this patch has openUrl set.
     */
    function hasOpenUrl() {
       return $this->hasOpenUrl;
    }

    /**
     * Determine whether or not this patch has notificationUrl set.
     */
    function hasNotificationUrl() {
       return $this->hasNotificationUrl;
    }

    /**
     * Determine whether or not this patch has alertUrl set.
     */
    function hasAlertUrl() {
       return $this->hasAlertUrl;
    }

    /**
     * Determine whether or not this patch has displayVersion set.
     */
    function hasDisplayVersion() {
       return $this->hasDisplayVersion;
    }

    /**
     * Determine whether or not this patch has appVersion set.
     */
    function hasAppVersion() {
       return $this->hasAppVersion;
    }

    /**
     * Determine whether or not this patch has platformVersion set.
     */
    function hasPlatformVersion() {
       return $this->hasPlatformVersion;
    }

    /**
     * Determine whether or not this patch has snippetSchemaVersion set.
     */
    function hasSnippetSchemaVersion() {
       return $this->hasSnippetSchemaVersion;
    }

    /**
     * Determine whether or not this patch has promptWaitTime set.
     */
    function hasPromptWaitTime() {
       return $this->hasPromptWaitTime;
    }

    /**
     * FIXME hasUpdateInfo not used by snippet schema v2, needs refactoring
     * Determine whether or not this patch has update information.
     * @TODO Make this a bit more graceful.
     */
    function hasUpdateInfo() {
       return $this->hasUpdateInfo;
    }

    /**
     * Determine whether or not this patch has update type information.
     */
    function hasUpdateType() {
        return isset($this->updateType);
    }

    /**
     * Determine whether or not we have a license URL and our patch is a major update.
     */
    function hasLicenseUrl() {
        return (isset($this->licenseUrl) && $this->isComplete());
    }

    /**
     * Determine whether or not the to_build matches the latest build (taken from the complete build) for a partial patch.
     * @param string $completeBuild
     *
     * @return bool
     */
    function isOneStepFromLatest($completeBuild) {
        return ($this->build == $completeBuild) ? true : false;
    }

    /**
     * Get the latest build with and update for this branch.  The purpose of this function is to find the last build that contains an update.
     * @param string $product
     * @param string $branchVersion
     * @param string $platform
     * @param return string|false false if there is no matching build
     */
    function getLatestCompleteBuild($product,$branchVersion,$platform,$locale,$buildSource,$channel) {
        $files = array();

        $dir = SOURCE_DIR.'/'.$buildSource.'/'.$product.'/'.$branchVersion.'/'.$platform;

        // Find the build ids for the given product/branchVersion/platform.  The last complete update
        // is normally found in the second-to-last build directory.
        //
        // To get this build id, we sort the directory listing by number and search for non-empty
        // complete.txt file
        if (is_dir($dir)) {
            $fp = opendir($dir);
            while (false !== ($filename = readdir($fp))) {
                if ($filename!='.' && $filename!='..') {
                    $files[] = $filename;
                }       
            }
            closedir($fp);
            
            rsort($files,SORT_NUMERIC); 

            // Return the directory with the non-empty complete.txt, which is the latest available build
            foreach ($files as $buildID) {
                if (!empty($buildID) && is_numeric($buildID)) {
                    $testPath = $dir.'/'.$buildID.'/'.$locale.'/'.$channel.'/complete.txt';
                    if ($this->isNightlyChannel($channel)) {  
                        $testPath = $dir.'/'.$buildID.'/'.$locale.'/complete.txt';
                    }
                    if (file_exists($testPath) && filesize($testPath) > 0) {
                        return $buildID;
                    }
                }
            }
        }

        // By default we return false, meaning that there is no latest complete update.
        // Doing so means no updates for this channel.
        return false;
    }

    /**
     * Determine if the current update should be blocked because of its
     * OS_VERSION.
     *
     * @param string updateType type of update (major|minor)
     * @param string product name of the product
     * @param string version of the update found
     * @param string os OS_VERSION
     * @param array unsupportedPlatforms - array of unsupported platforms
     * @return boolean
     */
    function isSupported($updateType, $product, $newVersion, $os, $unsupportedPlatforms) {

        foreach ($unsupportedPlatforms[$product] as $versionPattern => $badPlatforms) {
            if ($this->isMatchingVersion($newVersion,$versionPattern)) {
                foreach ($badPlatforms as $badPlatform) {
                    if (strpos($os, $badPlatform) !== false) {
                        return false;
                    }
                }
            }
        }

        return true;
    }
}
?>
