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
    var $branchVersions;

    // Array the defines which channels are flagged as 'nightly' channels.
    var $nightlyChannels;

    // Valid patch flag.
    var $isPatch;

    // Is this patch a complete or partial patch?
    var $patchType;

    // Update metadata, read from patch file.
    var $updateType;
    var $updateVersion;
    var $updateExtensionVersion;
    
    // Do we have Update metadata information?
    var $hasUpdateInfo;
    var $hasDetailsUrl;

    /**
     * Constructor.
     */
    function Patch($branchVersions=array(),$nightlyChannels,$type='complete') {
        $this->setBranchVersions($branchVersions);
        $this->setNightlyChannels($nightlyChannels);
        $this->setVar('isPatch',false);
        $this->setVar('patchType',$type); 
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
        if ($file = explode("\n",file_get_contents($path,true))) {
            $this->setVar('type',$file[0]);
            $this->setVar('url',$file[1]);
            $this->setVar('hashFunction',$file[2]);
            $this->setVar('hashValue',$file[3]);
            $this->setVar('size',$file[4]);
            $this->setVar('build',$file[5]);

            // Attempt to read update information.
            // @TODO Add ability to set updateType, once it exists in the build snippet.
            if ($this->isComplete() && isset($file[6]) && isset($file[7])) {
                $this->setVar('updateVersion',$file[6],true);
                $this->setVar('updateExtensionVersion',$file[7],true);
                $this->setVar('hasUpdateInfo',true,true);
            }
            
            if ($this->isComplete() && isset($file[8])) {
                $this->setVar('detailsUrl',$file[8],true);
                $this->setVar('hasDetailsUrl',true,true);
            }
            
            return true;
        }

        return false;
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
        if (!empty($channel) && $this->setPath($product,$platform,$locale,$version,$build,3,$channel) && file_exists($this->path) && filesize($this->path) > 0) {
            $this->setSnippet($this->path); 
            $this->setVar('isPatch',true,true);
            return true;
        } 

        // If our channel matches our regexp for fallback channels, let's try to fallback.
        if (preg_match('/^(release|beta)(test)?\-cck\-\w+$/',$channel)) {

            // Partner fallback channel to be used if the partner-specific update doesn't exist or work.
            $buf = array();
            $buf = split('-cck-',$channel);
            $fallbackChannel = $buf[0];
        
            // Do a check for the fallback update.  If we find a valid fallback update, we can offer it. 
            if (!empty($fallbackChannel) && $this->setPath($product,$platform,$locale,$version,$build,3,$fallbackChannel) && file_exists($this->path) && filesize($this->path) > 0) {
                $this->setSnippet($this->path); 
                $this->setVar('isPatch',true,true);
                return true;
            }
        }

        // Determine the branch of the client's version.
        $branchVersion = $this->getBranch($version);


        // Otherwise, if it is a complete patch and a nightly channel, force the complete update to take the user to the latest build.
        if ($this->isComplete() && $this->isNightlyChannel($channel)) {

            // Get the latest build that has an update for this branch.
            $latestCompleteBuild = $this->getLatestCompleteBuild($product,$branchVersion,$platform);

            // If we have the latest complete build, the path is valid, the file exists, and the filesize is greater than zero, we have a valid complete patch.
            if ($latestCompleteBuild && $this->setPath($product,$platform,$locale,$branchVersion,$latestCompleteBuild,2,$channel) && file_exists($this->path) && filesize($this->path) > 0) {
                $this->setSnippet($this->path); 
                $this->setVar('isPatch',true,true);
                return true;
            }
        } 


        // Otherwise, check for the partial snippet info.  If an update exists, pass it along.
        elseif ($this->isPartial() && $this->isNightlyChannel($channel) && $this->setPath($product,$platform,$locale,$branchVersion,$build,2,$channel) && file_exists($this->path) && filesize($this->path) > 0) {
                $this->setSnippet($this->path); 
                $this->setVar('isPatch',true,true);
                return true;
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
    function isNewBuild($build) {
        return ($this->build>$build) ? true : false;
    }

    /**
     * Set the branch versions array.
     *
     * @param array $branchVersions
     * @return boolean
     */
    function setBranchVersions($branchVersions) {
        return $this->setVar('branchVersions',$branchVersions);
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
     * Determine whether or not the incoming version is a product BRANCH.
     *
     * @param string $version
     * @return string|false
     */
    function getBranch($version) {
       return (isset($this->branchVersions[$version])) ? $this->branchVersions[$version] : false;
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
     */
    function hasDetailsUrl() {
       return $this->hasDetailsUrl;
    }

    /**
     * Determine whether or not this patch has update information.
     */
    function hasUpdateInfo() {
       return $this->hasUpdateInfo;
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
    function getLatestCompleteBuild($product,$branchVersion,$platform) {
        $files = array();

        $dir = SOURCE_DIR.'/2/'.$product.'/'.$branchVersion.'/'.$platform;

        // Find the build ids for the given product/branchVersion/platform.  The last complete update
        // is found in the second-to-last build directory.
        //
        // To get this build id, we sort the directory listing by number and retrieve $files[1].
        if (is_dir($dir)) {
            $fp = opendir($dir);
            while (false !== ($filename = readdir($fp))) {
                if ($filename!='.' && $filename!='..') {
                    $files[] = $filename;
                }       
            }
            closedir($fp);
            
            rsort($files,SORT_NUMERIC); 

            // Only return the build id if it is not empty and not numeric.
            if (!empty($files[1]) && is_numeric($files[1])) {
                return $files[1];
            }
        }

        // By default we return false, meaning that there is no latest complete update.
        // Doing so means no updates for the nightly channel.
        return false;
    }
}
?>
