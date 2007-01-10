<?php
/**
 * Plugin Finder Service
 *
 * The purpose of this script is to determine a matching plugin based on mime-type
 * for an embedded HTML object, then return the correct information for a corresponding plugin.
 * 
 * @package amo
 * @subpackage docs
 * @TODO clean this ____ up
 *
 * @author Vladimir Vukicevic <vladimir@pobox.com>
 * @author Doron Rosenberg <doronr@us.ibm.com>
 * @author Johnny Stenback <jst@mozilla.org>
 * @author Mike Morgan <morgamic@gmail.com> (reindented everything 08/06)
 */

startProcessing('pfs.tpl',$memcacheId,$compileId,'xml');

/**
 * Set variables.  We are allowing these to come in since they are compared to regular
 * expressions eventually anyway.  So yes, we are aware they are coming from $_GET.
 */
$mimetype = isset($_GET['mimetype']) ? $_GET['mimetype'] : null;
$reqTargetAppGuid = isset($_GET['appID']) ? $_GET['appID'] : null;
$reqTargetAppVersion = isset($_GET['appVersion']) ? $_GET['appVersion'] : null;
$clientOS = isset($_GET['clientOS']) ? $_GET['clientOS'] : null;
$chromeLocale = isset($_GET['chromeLocale']) ? $_GET['chromeLocale'] : null;

/**
 * Only execute if we have proper variables passed from the client.
 */
if (!empty($mimetype) &&
    !empty($reqTargetAppGuid) &&
    !empty($reqTargetAppVersion) &&
    !empty($clientOS) &&
    !empty($chromeLocale)) {

    /**
     * Figure out what plugins we've got, and what plugins we know where
     * to get.
     */
    $name = '';
    $guid = '-1';
    $version = '';
    $iconUrl = '';
    $XPILocation = '';
    $installerShowsUI = 'true';
    $manualInstallationURL = '';
    $licenseURL = '';
    $needsRestart = 'false';

    /**
     * Begin our huge and embarrassing if-else statement.
     */
    if (($mimetype == 'application/x-shockwave-flash' ||
         $mimetype == 'application/futuresplash') &&
         preg_match('/^(Win|(PPC|Intel) Mac OS X|Linux.+i\d86)/', $clientOS)) {
        // We really want the regexp for Linux to be /Linux(?! x86_64)/ but
        // for now we can't tell 32-bit linux appart from 64-bit linux, so
        // feed all x86_64 users the flash player, even if it's a 32-bit
        // plugin

        // We've got flash plugin installers for Win and Linux (x86),
        // present those to the user, and for Mac users, tell them where
        // they can go to get the installer.

        $name = 'Adobe Flash Player';
        $manualInstallationURL = 'http://www.adobe.com/go/getflashplayer';

        // Don't use a https URL for the license here, per request from
        // Macromedia.
        if ($chromeLocale != 'ja-JP') {
            $licenseURL = 'http://www.adobe.com/go/eula_flashplayer';
        } else {
            $licenseURL = 'http://www.adobe.com/go/eula_flashplayer_jp';
        }

        if (preg_match('/^Win/', $clientOS)) {
            $guid = '{4cfaef8a-a6c9-41a0-8e6f-967eb8f49143}';
            $XPILocation = 'http://fpdownload.macromedia.com/get/flashplayer/xpi/current/flashplayer-win.xpi';
            $installerShowsUI = 'false';
        } else if (preg_match('/^Linux/', $clientOS)) {
            $guid = '{7a646d7b-0202-4491-9151-cf66fa0722b2}';
            $XPILocation = 'http://fpdownload.macromedia.com/get/flashplayer/xpi/current/flashplayer-linux.xpi';
            $installerShowsUI = 'false';
        } else if (preg_match('/^PPC Mac OS X/', $clientOS)) {
            $guid = '{89977581-9028-4be0-b151-7c4f9bcd3211}';
            $XPILocation = 'http://fpdownload.macromedia.com/get/flashplayer/xpi/current/flashplayer-mac.xpi';
        }
    } elseif ($mimetype == 'application/x-director' &&
              preg_match('/^(Win|PPC Mac OS X)/', $clientOS)) {
        $name = 'Macromedia Shockwave Player';
        $manualInstallationURL = 'http://www.adobe.com/go/getshockwave/';
        $XPILocation = getUriForNoXPI();
        $version = '10.1';
        $needsRestart = 'true';

        // Even though the shockwave installer is not a silent installer, we
        // need to show its EULA here since we've got a slimmed down
        // installer that doesn't do that itself.
        if ($chromeLocale != 'ja-JP') {
            $licenseURL = 'http://www.adobe.com/go/eula_shockwaveplayer';
        } else {
            $licenseURL = 'http://www.adobe.com/go/eula_shockwaveplayer_jp';
        }

        if (preg_match('/^Win/', $clientOS)) {
            $guid = '{45f2a22c-4029-4209-8b3d-1421b989633f}';

            if ($chromeLocale == 'ja-JP') {
                $XPILocation = 'https://www.macromedia.com/go/xpi_shockwaveplayerj_win';
            } else {
                $XPILocation = 'https://www.macromedia.com/go/xpi_shockwaveplayer_win';
            }
        } else if (preg_match('/^PPC Mac OS X/', $clientOS)) {
            $guid = '{49141640-b629-4d57-a539-b521c4a99eff}';

            if ($chromeLocale == 'ja-JP') {
                $XPILocation = 'https://www.macromedia.com/go/xpi_shockwaveplayerj_macosx';
            } else {
                $XPILocation = 'https://www.macromedia.com/go/xpi_shockwaveplayer_macosx';
            }
        }
    } elseif (($mimetype == 'audio/x-pn-realaudio-plugin' ||
	           $mimetype == 'audio/x-pn-realaudio') &&
               preg_match('/^(Win|Linux|PPC Mac OS X)/', $clientOS)) {
        $name = 'Real Player';
        $version = '10.5';
        $manualInstallationURL = 'http://www.real.com';
        $needsRestart = 'true';

        if (preg_match('/^Win/', $clientOS)) {
            $XPILocation = 'http://forms.real.com/real/player/download.html?type=firefox';
            $guid = '{d586351c-cb55-41a7-8e7b-4aaac5172d39}';
        } else {
            $XPILocation = getUriForNoXPI();
            $guid = '{269eb771-59de-4702-9209-ca97ce522f6d}';
        }
    } elseif (preg_match('/^(application\/(sdp|x-(mpeg|rtsp|sdp))|audio\/(3gpp(2)?|AMR|aiff|basic|mid(i)?|mp4|mpeg|vnd\.qcelp|wav|x-(aiff|m4(a|b|p)|midi|mpeg|wav))|image\/(pict|png|tiff|x-(macpaint|pict|png|quicktime|sgi|targa|tiff))|video\/(3gpp(2)?|flc|mp4|mpeg|quicktime|sd-video|x-mpeg))$/', $mimetype) &&
	          preg_match('/^(Win|PPC Mac OS X)/', $clientOS)) {
        //
        // Well, we don't have a plugin that can handle any of those
        // mimetypes, but the Apple Quicktime plugin can. Point the user to
        // the Quicktime download page.
        //

        $name = 'Apple Quicktime';
        $guid = '{a42bb825-7eee-420f-8ee7-834062b6fefd}';
        $XPILocation = getUriForNoXPI();
        $installerShowsUI = 'true';
        $manualInstallationURL = 'http://www.apple.com/quicktime/download/';
    } elseif (preg_match('/^application\/x-java-((applet|bean)(;jpi-version=1\.5|;version=(1\.(1(\.[1-3])?|(2|4)(\.[1-2])?|3(\.1)?|5)))?|vm)$/', $mimetype) &&
	          preg_match('/^(Win|Linux|PPC Mac OS X)/', $clientOS)) {
        // We serve up the Java plugin for the following mimetypes:
        //
        // application/x-java-vm
        // application/x-java-applet;jpi-version=1.5
        // application/x-java-bean;jpi-version=1.5
        // application/x-java-applet;version=1.3
        // application/x-java-bean;version=1.3
        // application/x-java-applet;version=1.2.2
        // application/x-java-bean;version=1.2.2
        // application/x-java-applet;version=1.2.1
        // application/x-java-bean;version=1.2.1
        // application/x-java-applet;version=1.4.2
        // application/x-java-bean;version=1.4.2
        // application/x-java-applet;version=1.5
        // application/x-java-bean;version=1.5
        // application/x-java-applet;version=1.3.1
        // application/x-java-bean;version=1.3.1
        // application/x-java-applet;version=1.4
        // application/x-java-bean;version=1.4
        // application/x-java-applet;version=1.4.1
        // application/x-java-bean;version=1.4.1
        // application/x-java-applet;version=1.2
        // application/x-java-bean;version=1.2
        // application/x-java-applet;version=1.1.3
        // application/x-java-bean;version=1.1.3
        // application/x-java-applet;version=1.1.2
        // application/x-java-bean;version=1.1.2
        // application/x-java-applet;version=1.1.1
        // application/x-java-bean;version=1.1.1
        // application/x-java-applet;version=1.1
        // application/x-java-bean;version=1.1
        // application/x-java-applet
        // application/x-java-bean
        //
        //
        // We don't have a Java plugin to offer here, but Sun's got one for
        // Windows. For other platforms we know where to get one, point the
        // user to the JRE download page.
        //

        $name = 'Java Runtime Environment';
        $version = '';
        $manualInstallationURL = 'http://java.com/download/manual.jsp';
        $installerShowsUI = 'true';
        $needsRestart = 'true';

        // For now, send Vista users to a manual download page.
        //
        // This is a temp fix for bug 366129 until vista has a non-manual
        // solution.
        if (preg_match('/^Windows NT 6\.0/', $clientOS)) {
            $guid = '{fbe640ef-4375-4f45-8d79-767d60bf75b8}';
            $XPILocation = getUriForNoXPI();
        } elseif (preg_match('/^Win/', $clientOS)) {
            $guid = '{92a550f2-dfd2-4d2f-a35d-a98cfda73595}';
            $XPILocation = 'http://java.com/jre-install.xpi';
        } else {
            $guid = '{fbe640ef-4375-4f45-8d79-767d60bf75b8}';
            $XPILocation = getUriForNoXPI();
        }
    } elseif (($mimetype == 'application/pdf' ||
               $mimetype == 'application/vnd.fdf' ||
               $mimetype == 'application/vnd.adobe.xfdf' ||
               $mimetype == 'application/vnd.adobe.xdp+xml' ||
               $mimetype == 'application/vnd.adobe.xfd+xml') &&
               preg_match('/^(Win|PPC Mac OS X|Linux(?! x86_64))/', $clientOS)) {
        $name = 'Adobe Acrobat Plug-In';
        $guid = '{d87cd824-67cb-4547-8587-616c70318095}';
        $XPILocation = getUriForNoXPI();
        $manualInstallationURL = 'http://www.adobe.com/products/acrobat/readstep.html';
    } elseif ($mimetype == 'application/x-mtx' &&
              preg_match('/^(Win|PPC Mac OS X)/', $clientOS)) {
        $name = 'Viewpoint Media Player';
        $guid = '{03f998b2-0e00-11d3-a498-00104b6eb52e}';
        $XPILocation = getUriForNoXPI();
        $manualInstallationURL = 'http://www.viewpoint.com/pub/products/vmp.html';
    } elseif (preg_match('/^PPC Mac OS X/', $clientOS) &&
	          preg_match('/^(application\/(asx|x-(mplayer2|ms-wmd))|video\/x-ms-(asf(-plugin)?|wm(p|v|x)?|wvx)|audio\/x-ms-w(ax|ma))$/', $mimetype)) {
        // We serve up the Windows Media Player plugin for the following mimetypes:
        //
        // application/asx
        // application/x-mplayer2
        // application/x-ms-wmd
        // audio/x-ms-wax
        // audio/x-ms-wma
        // video/x-ms-asf
        // video/x-ms-asf-plugin
        // video/x-ms-wm
        // video/x-ms-wmp
        // video/x-ms-wmv
        // video/x-ms-wmx
        // video/x-ms-wvx
        //
        $name = 'Windows Media Player';
        $version = '9';
        $guid = '{cff1240a-fd24-4b9f-8183-ccd96e5300d0}';
        $XPILocation = getUriForNoXPI();
        $manualInstallationURL = 'http://www.microsoft.com/windows/windowsmedia/software/Macintosh/osx/default.aspx';
        $needsRestart = 'true';
    } elseif ($mimetype == 'application/x-xstandard' && preg_match('/^(Win|PPC Mac OS X)/', $clientOS)) {
        $name = 'XStandard XHTML WYSIWYG Editor';
        $guid = '{3563d917-2f44-4e05-8769-47e655e92361}';
        $iconUrl = 'http://xstandard.com/images/xicon32x32.gif';
        $XPILocation = 'http://xstandard.com/download/xstandard.xpi';
        $installerShowsUI = 'false';
        $manualInstallationURL = 'http://xstandard.com/download/';
        $licenseURL = 'http://xstandard.com/license/';
    } elseif ($mimetype == 'application/x-dnl' && preg_match('/^Win/', $clientOS)) {
        $name = 'DNL Reader';
        $guid = '{ce9317a3-e2f8-49b9-9b3b-a7fb5ec55161}';
        $version = '5.5';
        $iconUrl = 'http://digitalwebbooks.com/reader/dwb16.gif';
        $XPILocation = 'http://digitalwebbooks.com/reader/xpinst.xpi';
        $installerShowsUI = 'false';
        $manualInstallationURL = 'http://digitalwebbooks.com/reader/';
    } elseif ($mimetype == 'application/x-videoegg-loader' && preg_match('/^Win/', $clientOS)) {
        $name = 'VideoEgg Publisher';
        $guid = '{b8b881f0-2e07-11db-a98b-0800200c9a66}';
        $iconUrl = 'http://videoegg.com/favicon.ico';
        $XPILocation = 'http://update.videoegg.com/Install/Windows/Initial/VideoEggPublisher.xpi';
        $installerShowsUI = 'true';
        $manualInstallationURL = 'http://www.videoegg.com/';
    } elseif ($mimetype == 'video/divx' && preg_match('/^Win/', $clientOS)) {
        $name = 'DivX Web Player';
        $guid = '{a8b771f0-2e07-11db-a98b-0800200c9a66}';
        $iconUrl = 'http://images.divx.com/divx/player/webplayer.png';
        $XPILocation = 'http://download.divx.com/player/DivXWebPlayer.xpi';
        $installerShowsUI = 'false';
        $licenseURL = 'http://go.divx.com/plugin/license/';
        $manualInstallationURL = 'http://go.divx.com/plugin/download/';
    } elseif ($mimetype == 'video/divx' && preg_match('/^(PPC|Intel) Mac OS X/', $clientOS)) {
        $name = 'DivX Web Player';
        $guid = '{a8b771f0-2e07-11db-a98b-0800200c9a66}';
        $iconUrl = 'http://images.divx.com/divx/player/webplayer.png';
        $XPILocation = 'http://download.divx.com/player/DivXWebPlayerMac.xpi';
        $installerShowsUI = 'false';
        $licenseURL = 'http://go.divx.com/plugin/license/';
        $manualInstallationURL = 'http://go.divx.com/plugin/download/';
    }
    // End ridiculously huge and embarrassing if-else block.

}
// End our PFS block.



/**
 * Set up our plugin array based on what we've found.
 */
$plugin = array();
$plugin['mimetype'] = !empty($mimetype) ? $mimetype : '-1';
$plugin['name'] = !empty($name) ? $name : '-1';
$plugin['guid'] = !empty($guid) ? $guid : '-1';
$plugin['version'] = !empty($version) ? $version : null;
$plugin['iconUrl'] = !empty($iconUrl) ? $iconUrl : null;
$plugin['XPILocation'] = !empty($XPILocation) ? $XPILocation : null;
$plugin['installerShowsUI'] = !empty($installerShowsUI) ? $installerShowsUI : null;
$plugin['manualInstallationURL'] = !empty($manualInstallationURL) ? $manualInstallationURL : null;
$plugin['licenseURL'] = !empty($licenseURL) ? $licenseURL : null;
$plugin['needsRestart'] = !empty($needsRestart) ? $needsRestart : 'false';

$tpl->assign('plugin',$plugin);

function getUriForNoXPI()
{
  // Firefox 1.0PR1 (0.10 and 0.10.1, anything built in September,
  // actually) is broken and need an XPI in all cases, other versions
  // can deal with getting no XPI URI.

  if (preg_match("/Gecko\/200409\d\d Firefox\/0\.10/",
		 $_SERVER['HTTP_USER_AGENT'])) {
    return 'https://update.mozilla.org/plugins/installers/invalid.xpi';
  }

  return "";
}
?>
