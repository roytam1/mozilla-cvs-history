#
## hostname: bm-xserve12
## uname: Darwin bm-xserve12 8.8.4 Darwin Kernel Version 8.8.4: Sun Oct 29 15:26:54 PST 2006; root:xnu-792.16.4.obj~1/RELEASE_I386 i386 i386
#

#- tinder-config.pl - Tinderbox configuration file.
#-    Uncomment the variables you need to set.
#-    The default values are the same as the commented variables.

$ENV{NO_EM_RESTART} = "1";
$ENV{DYLD_NO_FIX_PREBINDING} = "1";
$ENV{LD_PREBIND_ALLOW_OVERLAP} = "1";
$ENV{CVS_RSH} = "ssh";
$ENV{MOZ_CRASHREPORTER_NO_REPORT} = '1';

$MacUniversalBinary = 0;

# $ENV{MOZ_PACKAGE_MSI}
#-----------------------------------------------------------------------------
#  Default: 0
#   Values: 0 | 1
#  Purpose: Controls whether a MSI package is made.
# Requires: Windows and a local MakeMSI installation.
#$ENV{MOZ_PACKAGE_MSI} = 0;

# $ENV{MOZ_SYMBOLS_TRANSFER_TYPE}
#-----------------------------------------------------------------------------
#  Default: scp
#   Values: scp | rsync
#  Purpose: Use scp or rsync to transfer symbols to the Talkback server.
# Requires: The selected type requires the command be available both locally
#           and on the Talkback server.
#$ENV{MOZ_SYMBOLS_TRANSFER_TYPE} = "scp";

#- PLEASE FILL THIS IN WITH YOUR PROPER EMAIL ADDRESS
$BuildAdministrator = 'build@mozilla.org';
#$BuildAdministrator = "$ENV{USER}\@$ENV{HOST}";
#$BuildAdministrator = ($ENV{USER} || "cltbld") . "\@" . ($ENV{HOST} || "dhcp");

#- You'll need to change these to suit your machine's needs
#$DisplayServer = ':0.0';

#- Default values of command-line opts
#-
#$BuildDepend       = 1;      # Depend or Clobber
#$BuildDebug        = 0;      # Debug or Opt (Darwin)
#$ReportStatus      = 1;      # Send results to server, or not
#$ReportFinalStatus = 1;      # Finer control over $ReportStatus.
$UseTimeStamp      = 0;      # Use the CVS 'pull-by-timestamp' option, or not
#$BuildOnce         = 0;      # Build once, don't send results to server
#$TestOnly          = 0;      # Only run tests, don't pull/build
#$BuildEmbed        = 0;      # After building seamonkey, go build embed app.
#$SkipMozilla       = 0;      # Use to debug post-mozilla.pl scripts.
$BuildLocales      = 1;      # Do l10n packaging?

# Tests
$CleanProfile             = 1;
#$ResetHomeDirForTests     = 1;
$ProductName              = "Firefox";
$VendorName               = 'Mozilla';

$RunMozillaTests          = 0;  # Allow turning off of all tests if needed.
$RegxpcomTest             = 1;
$AliveTest                = 1;
#$JavaTest                 = 0;
#$ViewerTest               = 0;
#$BloatTest                = 0;  # warren memory bloat test
#$BloatTest2               = 0;  # dbaron memory bloat test, require tracemalloc
#$DomToTextConversionTest  = 0;  
#$XpcomGlueTest            = 0;
$CodesizeTest             = 1;  # Z,  require mozilla/tools/codesighs
$EmbedCodesizeTest        = 0;  # mZ, require mozilla/tools/codesigns
#$MailBloatTest            = 0;
#$EmbedTest                = 0;  # Assumes you wanted $BuildEmbed=1
$LayoutPerformanceTest    = 1;  # Tp
$LayoutPerformanceLocalTest   = 1;  # Tp2
$DHTMLPerformanceTest     = 1;  # Tdhtml
#$QATest                   = 0;  
$XULWindowOpenTest        = 1;  # Txul
$StartupPerformanceTest   = 1;  # Ts

$TestsPhoneHome           = 0;  # Should test report back to server?

$GraphNameOverride        = 'xserve08.build.mozilla.org_Fx-Trunk';

# $results_server
#----------------------------------------------------------------------------
# Server on which test results will be accessible.  This was originally tegu,
# then became axolotl.  Once we moved services from axolotl, it was time
# to give this service its own hostname to make future transitions easier.
# - cmp@mozilla.org
#$results_server           = "build-graphs.mozilla.org";

#$pageload_server          = "spider";  # localhost
$pageload_server          = "pageload.build.mozilla.org";  # localhost

#
# Timeouts, values are in seconds.
#
#$CVSCheckoutTimeout               = 3600;
#$CreateProfileTimeout             = 45;
#$RegxpcomTestTimeout              = 120;

$AliveTestTimeout                 = 10;
#$ViewerTestTimeout                = 45;
#$EmbedTestTimeout                 = 45;
#$BloatTestTimeout                 = 120;   # seconds
#$MailBloatTestTimeout             = 120;   # seconds
#$JavaTestTimeout                  = 45;
#$DomTestTimeout	                  = 45;    # seconds
#$XpcomGlueTestTimeout             = 15;
#$CodesizeTestTimeout              = 900;     # seconds
#$CodesizeTestType                 = "auto";  # {"auto"|"base"}
$LayoutPerformanceTestTimeout     = 300;  # entire test, seconds
$LayoutPerformanceLocalTestTimeout     = 180;  # entire test, seconds
$DHTMLPerformanceTestTimeout      = 180;  # entire test, seconds
#$QATestTimeout                    = 1200;   # entire test, seconds
#$LayoutPerformanceTestPageTimeout = 30000; # each page, ms
#$StartupPerformanceTestTimeout    = 15;    # seconds
#$XULWindowOpenTestTimeout	      = 150;   # seconds


#$MozConfigFileName = 'mozconfig';

#$UseMozillaProfile = 1;
#$MozProfileName = 'default';

#- Set these to what makes sense for your system
#$Make          = 'gmake';       # Must be GNU make
#$MakeOverrides = '';
#$mail          = '/bin/mail';
#$CVS           = 'cvs -q';
#$CVSCO         = 'checkout -P';

# win32 usually doesn't have /bin/mail
#$blat           = 'c:/nstools/bin/blat';
#$use_blat       = 0;

# Set moz_cvsroot to something like:
# :pserver:$ENV{USER}%netscape.com\@cvs.mozilla.org:/cvsroot
# :pserver:anonymous\@cvs-mirror.mozilla.org:/cvsroot
#
# Note that win32 may not need \@, depends on ' or ".
# :pserver:$ENV{USER}%netscape.com@cvs.mozilla.org:/cvsroot
# CONFIG: $moz_cvsroot   = '%mozillaCvsroot%';
$moz_cvsroot   = ":ext:ffxbld\@cvs.mozilla.org:/cvsroot";

#- Set these proper values for your tinderbox server
#$Tinderbox_server = 'tinderbox-daemon@tinderbox.mozilla.org';

# Allow for non-client builds, e.g. camino.
#$moz_client_mk = 'client.mk';

#- Set if you want to build in a separate object tree
#$ObjDir = '';

# Extra build name, if needed.
$BuildNameExtra = 'Fx-Trunk-l10n-Release';

# User comment, eg. ip address for dhcp builds.
# ex: $UserComment = "ip = 208.12.36.108";
#$UserComment = 0;

# l10n settings
$ConfigureOnly = 1;             # Configure only, don't build.
$LocaleProduct = "browser";
$LocalizationVersionFile = 'browser/config/version.txt';
%WGetFiles = (
# CONFIG:             "http://%stagingServer%/pub/mozilla.org/firefox/nightly/%version%-candidates/rc%rc%/firefox-%version%.en-US.mac.dmg" => 
	      "http://stage.mozilla.org/pub/mozilla.org/firefox/nightly/1.9b1-candidates/rc1/firefox-%version%.en-US.mac.dmg" =>
# CONFIG:             "%l10n_buildDir%/%l10n_buildPlatform%/firefox.dmg"
	      "/builds/tinderbox/Fx-Trunk-l10n-Release/Darwin_8.8.4_Depend/firefox.dmg"
	      );

# CONFIG: $BuildLocalesArgs = "ZIP_IN=%l10n_buildDir%/%l10n_buildPlatform%/firefox.dmg";
$BuildLocalesArgs = "ZIP_IN=/builds/tinderbox/Fx-Trunk-l10n-Release/Darwin_8.8.4_Depend/firefox.dmg";
#@CompareLocaleDirs        = (); # Run compare-locales test on these directories
@CompareLocaleDirs = (
  "netwerk",
  "browser",
  "dom",
  "toolkit",
  "security/manager",
  "other-licenses/branding/firefox",
  "extensions/reporter",
);

#-
#- The rest should not need to be changed
#-

#- Minimum wait period from start of build to start of next build in minutes.
#$BuildSleep = 10;

#- Until you get the script working. When it works,
#- change to the tree you're actually building
# CONFIG: $BuildTree  = '%buildTree%';
$BuildTree  = 'Mozilla-l10n';

#$BuildName = '';
# CONFIG: $BuildTag = '%productTag%_RELEASE';
$BuildTag = 'FIREFOX_3_0b1_RELEASE';
#$BuildConfigDir = 'mozilla/config';
#$Topsrcdir = 'mozilla';

$BinaryName = 'firefox-bin';

#
# For embedding app, use:
#$EmbedBinaryName = 'TestGtkEmbed';
#$EmbedDistDir    = 'dist/bin'


#$ShellOverride = ''; # Only used if the default shell is too stupid
#$ConfigureArgs = '';
#$ConfigureEnvArgs = '';
#$Compiler = 'gcc';
#$NSPRArgs = '';
#$ShellOverride = '';

# Release build options
$ReleaseBuild  = 1;
$shiptalkback  = 0;
$ReleaseToLatest = 0; # Push the release to latest-<milestone>?
$ReleaseToDated = 1; # Push the release to YYYY-MM-DD-HH-<milestone>?
$build_hour    = "9";
$package_creation_path = "/browser/installer";
# needs setting for mac + talkback: $mac_bundle_path = "/browser/app";
$mac_bundle_path = "/browser/app";
# CONFIG: $ssh_user      = "%sshUser%";
$ssh_user      = "ffxbld";
# CONFIG: $ssh_key       = "'$ENV{HOME}/.ssh/%sshUser%_dsa'";
$ssh_key       = "'$ENV{HOME}/.ssh/ffxbld_dsa'";
# CONFIG: $ssh_server    = "%sshServer%";
$ssh_server    = "stage.mozilla.org";
$ReleaseGroup  = "firefox";
$ftp_path      = "/home/ftp/pub/firefox/nightly";
$url_path      = "http://ftp.mozilla.org/pub/mozilla.org/firefox/nightly";
$tbox_ftp_path = "/home/ftp/pub/firefox/tinderbox-builds";
$tbox_url_path = "http://ftp.mozilla.org/pub/mozilla.org/firefox/tinderbox-builds";
# CONFIG: $milestone     = "firefox%version%-l10n";
$milestone     = "firefox3.0b1-l10n";
$notify_list   = "build-announce\@mozilla.org";
$stub_installer = 0;
$sea_installer = 0;
$archive       = 1;
$push_raw_xpis = 0;
$update_package = 1;
$update_product = "Firefox";
$update_version = "trunk";
$update_platform = "Darwin_Universal-gcc3";
$update_hash = "md5";
$update_filehost = "ftp.mozilla.org";
$update_ver_file = 'browser/config/version.txt';
$update_pushinfo = 0;
$crashreporter_buildsymbols = 1;
$crashreporter_pushsymbols = 1;
$ENV{SYMBOL_SERVER_HOST} = 'stage.mozilla.org';
$ENV{SYMBOL_SERVER_USER}   = 'ffxbld';
$ENV{SYMBOL_SERVER_PATH}   = '/mnt/netapp/breakpad/symbols_ffx/';
$ENV{SYMBOL_SERVER_SSH_KEY}   = "$ENV{HOME}/.ssh/ffxbld_dsa";

# Reboot the OS at the end of build-and-test cycle. This is primarily
# intended for Win9x, which can't last more than a few cycles before
# locking up (and testing would be suspect even after a couple of cycles).
# Right now, there is only code to force the reboot for Win9x, so even
# setting this to 1, will not have an effect on other platforms. Setting
# up win9x to automatically logon and begin running tinderbox is left 
# as an exercise to the reader. 
#$RebootSystem = 0;

# LogCompression specifies the type of compression used on the log file.
# Valid options are 'gzip', and 'bzip2'. Please make sure the binaries
# for 'gzip' or 'bzip2' are in the user's path before setting this
# option.
#$LogCompression = '';

# LogEncoding specifies the encoding format used for the logs. Valid
# options are 'base64', and 'uuencode'. If $LogCompression is set above,
# this needs to be set to 'base64' or 'uuencode' to ensure that the
# binary data is transferred properly.
#$LogEncoding = '';

# Prevent Extension Manager from spawning child processes during tests
# - processes that tbox scripts cannot kill. 
#$ENV{NO_EM_RESTART} = '1';
