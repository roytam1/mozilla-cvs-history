#
## hostname: sea-win32-tbox
## uname: CYGWIN_NT-5.2 sea-win32-tbox 1.5.19(0.150/4/2) 2006-01-20 13:28 i686 Cygwin
#

# disable sending of crash reports locally until bug 379290 (autoreporting) gets fixed
$ENV{MOZ_CRASHREPORTER_NO_REPORT} = '1';

# package NSIS installer using 7zip
$ENV{MOZ_INSTALLER_USE_7ZIP} = '1';

# Ship the MSVC8 runtime libs
$ENV{WIN32_REDIST_DIR} = 'C:\Program Files\Microsoft Visual Studio 8\VC\Redist\x86\Microsoft.VC80.CRT';

# Enable Building of the Palm Sync extension.
$ENV{BUILD_PALMSYNC} = '1';
$ENV{PALM_CDK_DIR} = 'C:\PALMCDK403';

#- tinder-config.pl - Tinderbox configuration file.
#-    Uncomment the variables you need to set.
#-    The default values are the same as the commented variables.

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
#$BuildAdministrator = "$ENV{USER}\@$ENV{HOST}";
#$BuildAdministrator = ($ENV{USER} || "seabld") . "\@" . ($ENV{HOST} || "dhcp");
$BuildAdministrator = 'ccooper@deadsquid.com';

#- You'll need to change these to suit your machine's needs
$DisplayServer = ':0.0';

#- Default values of command-line opts
#-
$BuildDepend       = 1;      # Depend or Clobber
#$BuildDebug        = 0;      # Debug or Opt (Darwin)
$ReportStatus      = 1;      # Send results to server, or not
$ReportFinalStatus = 1;      # Finer control over $ReportStatus.
#$UseTimeStamp      = 1;      # Use the CVS 'pull-by-timestamp' option, or not
#$BuildOnce         = 0;      # Build once, don't send results to server
#$TestOnly          = 1;      # Only run tests, don't pull/build
#$BuildEmbed        = 0;      # After building seamonkey, go build embed app.
#$SkipMozilla       = 0;      # Use to debug post-mozilla.pl scripts.
$BuildLocales      = 1;      # Do l10n packaging?

# Tests
$CleanProfile             = 1;
#$ResetHomeDirForTests     = 1;
$ProductName              = "SeaMonkey";
$VendorName               = 'Mozilla';

#$RunMozillaTests          = 1;  # Allow turning off of all tests if needed.
#$RegxpcomTest             = 1;
$AliveTest                = 1;
#$JavaTest                 = 0;
#$ViewerTest               = 0;
#$BloatTest                = 0;  # warren memory bloat test
#$BloatTest2               = 0;  # dbaron memory bloat test, require tracemalloc
#$DomToTextConversionTest  = 1;  
#$XpcomGlueTest            = 0;
#$CodesizeTest             = 0;  # Z,  require mozilla/tools/codesighs
#$EmbedCodesizeTest        = 0;  # mZ, require mozilla/tools/codesigns
#$MailBloatTest            = 0;
#$EmbedTest                = 0;  # Assumes you wanted $BuildEmbed=1
$LayoutPerformanceTest    = 1;  # Tp
#$DHTMLPerformanceTest     = 0;  # Tdhtml
#$QATest                   = 0;  
#$XULWindowOpenTest        = 0;  # Txul
$StartupPerformanceTest   = 1;  # Ts
$LocaleProduct = "suite";
@CompareLocaleDirs = (
  "netwerk",
  "dom",
  "toolkit",
  "security/manager",
  "extensions/reporter",
  "editor/ui",
  "suite",
);

$TestsPhoneHome           = 1;  # Should test report back to server?

# $results_server
#----------------------------------------------------------------------------
# Server on which test results will be accessible.  This was originally tegu,
# then became axolotl.  Once we moved services from axolotl, it was time
# to give this service its own hostname to make future transitions easier.
# - cmp@mozilla.org
#$results_server           = "build-graphs.mozilla.org";

$pageload_server          = "pageload.build.mozilla.org";

#
# Timeouts, values are in seconds.
#
$CVSCheckoutTimeout               = 1200; # seconds (20 minutes)
#$CreateProfileTimeout             = 45;
#$RegxpcomTestTimeout              = 120;

$AliveTestTimeout                 = 15;
$ViewerTestTimeout                = 45;
$EmbedTestTimeout                 = 45;
$BloatTestTimeout                 = 120;   # seconds
$MailBloatTestTimeout             = 120;   # seconds
$JavaTestTimeout                  = 45;
$DomTestTimeout	                  = 45;    # seconds
$XpcomGlueTestTimeout             = 15;
$CodesizeTestTimeout              = 900;     # seconds
$CodesizeTestType                 = "auto";  # {"auto"|"base"}
$LayoutPerformanceTestTimeout     = 1200;  # entire test, seconds
$DHTMLPerformanceTestTimeout      = 1200;  # entire test, seconds
$QATestTimeout                    = 1200;   # entire test, seconds
$LayoutPerformanceTestPageTimeout = 30000; # each page, ms
$StartupPerformanceTestTimeout    = 5;    # seconds
$XULWindowOpenTestTimeout	      = 150;   # seconds


#$MozConfigFileName = 'mozconfig';

#$UseMozillaProfile = 1;
$MozProfileName = 'default';

#- Set these to what makes sense for your system
$Make          = 'make';       # Must be GNU make
#$MakeOverrides = '';
#$mail          = '/bin/mail';
#$CVS           = 'cvs -q';
#$CVSCO         = 'checkout -P';

# win32 usually doesn't have /bin/mail
$blat           = 'd:/moztools/bin/blat.exe';
$use_blat       = 1;

# Set moz_cvsroot to something like:
# :pserver:$ENV{USER}%netscape.com\@cvs.mozilla.org:/cvsroot
# :pserver:anonymous\@cvs-mirror.mozilla.org:/cvsroot
#
# Note that win32 may not need \@, depends on ' or ".
# :pserver:$ENV{USER}%netscape.com@cvs.mozilla.org:/cvsroot

$moz_cvsroot   = ':ext:seabld@cvs.mozilla.org:/cvsroot';

# Set these proper values for your tinderbox server
#$Tinderbox_server = 'tinderbox-daemon@tinderbox.mozilla.org';

# Allow for non-client builds, e.g. camino.
#$moz_client_mk = 'client.mk';

#- Set if you want to build in a separate object tree
$ObjDir = 'objdir';

# Extra build name, if needed.
$BuildNameExtra = 'Nightly';

# User comment, eg. ip address for dhcp builds.
# ex: $UserComment = "ip = 208.12.36.108";
$UserComment = 0;

#-
#- The rest should not need to be changed
#-

#- Minimum wait period from start of build to start of next build in minutes.
#$BuildSleep = 10;

#- Until you get the script working. When it works,
#- change to the tree you're actually building
$BuildTree  = 'SeaMonkey';
$BuildTreeLocale  = 'Mozilla-l10n-%s';

#$BuildName = '';
#$BuildTag = '';
#$BuildConfigDir = 'mozilla/config';
#$Topsrcdir = 'mozilla';

$BinaryName = 'seamonkey.exe';
$Timezone = 'PDT';

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
#$clean_objdir = 1; # remove objdir when starting release cycle?
#$clean_srcdir = 1; # remove srcdir when starting release cycle?
$shiptalkback  = 0;
#$ReleaseToLatest = 1; # Push the release to latest-<milestone>?
#$ReleaseToDated = 1; # Push the release to YYYY-MM-DD-HH-<milestone>?
$ReleaseGroup = 'seamonkey'; # group to set uploaded files to
$build_hour    = "1";
$package_creation_path = "/suite/installer";
# needs setting for mac + talkback: $mac_bundle_path = "/browser/app";
$ssh_version   = "2";
$ssh_user      = "seabld";
#$ssh_server    = "stage.mozilla.org";
$ftp_path      = "/home/ftp/pub/seamonkey/nightly";
$url_path      = "http://ftp.mozilla.org/pub/mozilla.org/seamonkey/nightly";
$tbox_ftp_path = "/home/ftp/pub/seamonkey/tinderbox-builds";
$tbox_url_path = "http://ftp.mozilla.org/pub/mozilla.org/seamonkey/tinderbox-builds";
$milestone     = "trunk";
$notify_list   = "build-announce\@mozilla.org";
$stub_installer = 0;
$sea_installer = 1;
$archive       = 1;
#$push_raw_xpis = 1;

$crashreporter_buildsymbols = 1;
$crashreporter_pushsymbols = 1;
$ENV{SYMBOL_SERVER_HOST} = 'stage.mozilla.org';
$ENV{SYMBOL_SERVER_USER}   = 'seabld';
$ENV{SYMBOL_SERVER_PATH}   = '/mnt/netapp/breakpad/symbols_sea/';
$ENV{SYMBOL_SERVER_SSH_KEY}   = "$ENV{HOME}/.ssh/seabld_dsa";

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
