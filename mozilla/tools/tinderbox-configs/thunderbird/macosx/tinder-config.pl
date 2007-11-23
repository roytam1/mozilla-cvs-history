#
## hostname: xserve04.build.mozilla.org
## uname:  Darwin xserve04.build.mozilla.org 8.7.0 Darwin Kernel Version 8.7.0: Fri May 26 15:20:53 PDT 2006; root:xnu-792.6.76.obj~1/RELEASE_PPC Power Macintosh powerpc
#

#- tinder-config.pl - Tinderbox configuration file.
#-    Uncomment the variables you need to set.
#-    The default values are the same as the commented variables.

$ENV{CVS_RSH} = "ssh";
$ENV{MOZ_SYMBOLS_TRANSFER_TYPE} = "rsync";

#- PLEASE FILL THIS IN WITH YOUR PROPER EMAIL ADDRESS
$BuildAdministrator = 'build@mozilla.org';

#$BuildAdministrator = ($ENV{USER} || "cltbld") . "\@" . ($ENV{HOST} || "dhcp");

#- You'll need to change these to suit your machine's needs
#$DisplayServer = ':0.0';

#- Default values of command-line opts
#-
#$BuildDepend       = 1;      # Depend or Clobber
#$BuildDebug        = 0;      # Debug or Opt (Darwin)
#$ReportStatus      = 1;      # Send results to server, or not
#$ReportFinalStatus = 1;      # Finer control over $ReportStatus.
$UseTimeStamp      = 1;      # Use the CVS 'pull-by-timestamp' option, or not
#$BuildOnce         = 0;      # Build once, don't send results to server
#$TestOnly          = 0;      # Only run tests, don't pull/build
#$BuildEmbed        = 0;      # After building seamonkey, go build embed app.
#$SkipMozilla       = 1;      # Use to debug post-mozilla.pl scripts.
$BuildLocales      = 1;

# Tests
$CleanProfile             = 1;
#$ResetHomeDirForTests     = 1;
$ProductName              = "Thunderbird";
$VendorName               = 'Mozilla';

$RunMozillaTests          = 1;  # Allow turning off of all tests if needed.
$RegxpcomTest             = 1;
$AliveTest                = 1;
#$JavaTest                 = 0;
#$ViewerTest               = 0;
#$BloatTest                = 0;  # warren memory bloat test
#$BloatTest2               = 0;  # dbaron memory bloat test, require tracemalloc
#$DomToTextConversionTest  = 0;  
#$XpcomGlueTest            = 0;
#$CodesizeTest             = 0;  # Z,  require mozilla/tools/codesighs
#$EmbedCodesizeTest        = 0;  # mZ, require mozilla/tools/codesigns
#$MailBloatTest            = 0;
#$EmbedTest                = 0;  # Assumes you wanted $BuildEmbed=1
#$LayoutPerformanceTest    = 0;  # Tp
#$DHTMLPerformanceTest     = 0;  # Tdhtml
#$QATest                   = 0;  
#$XULWindowOpenTest        = 0;  # Txul
#$StartupPerformanceTest   = 0;  # Ts
#@CompareLocaleDirs        = (); # Run compare-locales test on these directories
@CompareLocaleDirs = (
  "netwerk",
  "dom",
  "toolkit",
  "security/manager",
  "other-licenses/branding/thunderbird",
  "editor/ui",
  "mail",
);
#$CompareLocalesAviary     = 0;  # Should the compare-locales commands use the
#                                # aviary directory structure?

#$TestsPhoneHome           = 0;  # Should test report back to server?
#$results_server           = "axolotl.mozilla.org"; # was tegu
#$pageload_server          = "spider";  # localhost

#
# Timeouts, values are in seconds.
#
#$CVSCheckoutTimeout               = 3600;
#$CreateProfileTimeout             = 45;
#$RegxpcomTestTimeout              = 120;

#$AliveTestTimeout                 = 45;
#$ViewerTestTimeout                = 45;
#$EmbedTestTimeout                 = 45;
#$BloatTestTimeout                 = 120;   # seconds
#$MailBloatTestTimeout             = 120;   # seconds
#$JavaTestTimeout                  = 45;
#$DomTestTimeout	                  = 45;    # seconds
#$XpcomGlueTestTimeout             = 15;
#$CodesizeTestTimeout              = 900;     # seconds
#$CodesizeTestType                 = "auto";  # {"auto"|"base"}
#$LayoutPerformanceTestTimeout     = 1200;  # entire test, seconds
#$DHTMLPerformanceTestTimeout      = 1200;  # entire test, seconds
#$QATestTimeout                    = 1200;   # entire test, seconds
#$LayoutPerformanceTestPageTimeout = 30000; # each page, ms
#$StartupPerformanceTestTimeout    = 60;    # seconds
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

$moz_cvsroot   = ":ext:cltbld\@cvs.mozilla.org:/cvsroot";

#- Set these proper values for your tinderbox server
#$Tinderbox_server = 'tinderbox-daemon@tinderbox.mozilla.org';

# Allow for non-client builds, e.g. camino.
#$moz_client_mk = 'client.mk';

#- Set if you want to build in a separate object tree
#$ObjDir = '../build/unifox';

# Extra build name, if needed.
$BuildNameExtra = 'Tb-Universal-l10n';

# User comment, eg. ip address for dhcp builds.
# ex: $UserComment = "ip = 208.12.36.108";
#$UserComment = 0;

# Configure only, don't build.
$ConfigureOnly = 1;

$LocalizationVersionFile = 'mail/config/version.txt';
%WGetFiles = (
	     "http://stage.mozilla.org/pub/mozilla.org/thunderbird/nightly/latest-mozilla1.8.0/thunderbird-%version%.en-US.mac.dmg" =>
	     "/builds/tinderbox/Tb-Mozilla1.8.0-l10n/Darwin_8.7.0_Depend/thunderbird.dmg"
	     );
$BuildLocalesArgs = "ZIP_IN=/builds/tinderbox/Tb-Mozilla1.8.0-l10n/Darwin_8.7.0_Depend/thunderbird.dmg";


#-
#- The rest should not need to be changed
#-

#- Minimum wait period from start of build to start of next build in minutes.
#$BuildSleep = 10;

#- Until you get the script working. When it works,
#- change to the tree you're actually building
$BuildTree  = 'Mozilla1.8.0-l10n';
#$BuildName = '';
$BuildTag = 'MOZILLA_1_8_0_BRANCH';
#$BuildConfigDir = 'mozilla/config';
#$Topsrcdir = 'mozilla';

$BinaryName = 'thunderbird-bin';

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

# allow override of timezone value (for win32 POSIX::strftime)
#$Timezone = '';

# Release build options
$ReleaseBuild  = 1;
$LocaleProduct = "mail";
$shiptalkback  = 0;
$ReleaseToLatest = 1; # Push the release to latest-<milestone>?
$ReleaseToDated = 0; # Push the release to YYYY-MM-DD-HH-<milestone>?
$build_hour    = "9";
$package_creation_path = "/mail/installer";
# needs setting for mac + talkback: $mac_bundle_path = "/browser/app";
$mac_bundle_path = "/mail/app";
$ssh_version   = "2";
$ssh_user      = "tbirdbld";
$ssh_key       = "'$ENV{HOME}/.ssh/tbirdbld_dsa'";
$ssh_server    = "stage-new.mozilla.org";
$ReleaseGroup  = "thunderbird";
$ftp_path      = "/home/ftp/pub/thunderbird/nightly";
$url_path      = "http://ftp.mozilla.org/pub/mozilla.org/thunderbird/nightly";
$tbox_ftp_path = "/home/ftp/pub/thunderbird/tinderbox-builds";
$tbox_url_path = "http://ftp.mozilla.org/pub/mozilla.org/thunderbird/tinderbox-builds";
$milestone     = "mozilla1.8.0-l10n";
$notify_list   = "build-announce\@mozilla.org";
$stub_installer = 0;
$sea_installer = 0;
$archive       = 1;
$push_raw_xpis = 0;
$update_package = 1;
$update_product = "Thunderbird";
$update_version = "1.5.0.4";
$update_platform = "Darwin_ppc-gcc3";
$update_hash = "md5";
$update_filehost = "ftp.mozilla.org";
$update_appv = "1.5.0.4";
$update_extv = "1.5.0.4";

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
$MacUniversalBinary = 1;
