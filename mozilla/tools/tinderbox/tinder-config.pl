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
#$BuildAdministrator = ($ENV{USER} || "cltbld") . "\@" . ($ENV{HOST} || "dhcp");
#$FailedBuildAdministrator = $BuildAdministrator;

#- You'll need to change these to suit your machine's needs
#$DisplayServer = ':0.0';

#- Default values of command-line opts
#-
#$BuildDepend       = 1;      # Depend or Clobber
#$BuildDebug        = 0;      # Debug or Opt (Darwin)
#$ReportStatus      = 1;      # Send results to server, or not
#$ReportFailedStatus= 1;      # Send email report of initial build failure
#$ReportFinalStatus = 1;      # Finer control over $ReportStatus.
#$UseTimeStamp      = 1;      # Use the CVS 'pull-by-timestamp' option, or not
#$BuildOnce         = 0;      # Build once, don't send results to server
#$TestOnly          = 0;      # Only run tests, don't pull/build
#$BuildEmbed        = 0;      # After building seamonkey, go build embed app.
#$SkipMozilla       = 0;      # Use to debug post-mozilla.pl scripts.
#$SkipCheckout      = 0;      # Use to debug build process without checking out new source.
#$BuildLocales      = 0;      # Do l10n packaging?

# Tests
#$CleanProfile             = 0;
#$ResetHomeDirForTests     = 1;
#$ProductName              = "Mozilla";
#$VendorName               = '';

#$RunMozillaTests          = 1;  # Allow turning off of all tests if needed.
#$RegxpcomTest             = 1;
#$AliveTest                = 1;
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

#$TestsPhoneHome           = 0;  # Should test report back to server?
#$GraphNameOverride        = ''; # Override name built from ::hostname() and $BuildTag

# $results_server
#----------------------------------------------------------------------------
# Server on which test results will be accessible.  This was originally tegu,
# then became axolotl.  Once we moved services from axolotl, it was time
# to give this service its own hostname to make future transitions easier.
# - cmp@mozilla.org
#$results_server           = "build-graphs.mozilla.org";

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

#$moz_cvsroot   = $ENV{CVSROOT};

# Used for checking out sources (e.g. Talkback) from the internal
# Mozilla repository. If you don't have a CVS account with access,
# just leave this set to 0.
#$MofoRoot = 0

#- Set these proper values for your tinderbox server
#$Tinderbox_server = 'tinderbox-daemon@tinderbox.mozilla.org';

# Allow for non-client builds, e.g. camino.
#$moz_client_mk = 'client.mk';

# Set if you're building an app on top of XULRunner.
#$XULRunnerApp = 0;

#- Set if you want to build in a separate object tree
#$ObjDir = '';

# If the build is a combined xulrunner+something, set the "something"
# subdirectory: example "firefox/" - NOTE: need trailing slash!
#$SubObjDir = '';

# Extra build name, if needed.
#$BuildNameExtra = '';

# User comment, eg. ip address for dhcp builds.
# ex: $UserComment = "ip = 208.12.36.108";
#$UserComment = 0;

#-
#- The rest should not need to be changed
#-

#- Minimum wait period from start of build to start of next build in minutes.
#$BuildSleep = 10;

#- Until you get the script working. When it works,
#- change to the tree you're actually building
#$BuildTree  = 'MozillaTest';

#$BuildName = '';
#$BuildTag = '';
#$BuildConfigDir = 'mozilla/config';
#$Topsrcdir = 'mozilla';

#$BinaryName = 'mozilla-bin';

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

# UsePrebuiltTalkback:
# If set to a filepath, tinderbox will use the file contents (presumed to be a bz2
# archive of a compatible Talkback extension) rather than compiling Talkback
# from source. 
#$UsePrebuiltTalkback = 0;

# Release build options
#$ReleaseBuild  = 1;
#$clean_objdir = 1; # remove objdir when starting release cycle?
#$clean_srcdir = 1; # remove srcdir when starting release cycle?
#$shiptalkback  = 1;
#$ReleaseToLatest = 1; # Push the release to latest-<milestone>?
#$ReleaseToDated = 1; # Push the release to YYYY-MM-DD-HH-<milestone>?
#$ReleaseGroup = ''; # group to set uploaded files to
#$build_hour    = "8";
#$package_creation_path = "/xpinstall/packager";
# needs setting for mac + talkback: $mac_bundle_path = "/browser/app";
#$ssh_version   = "2";
#$ssh_user      = "cltbld";
#$ssh_server    = "stage.mozilla.org";
#$ftp_path      = "/home/ftp/pub/mozilla/nightly/experimental";
#$url_path      = "http://ftp.mozilla.org/pub/mozilla.org/mozilla/nightly/experimental";
#$tbox_ftp_path = $ftp_path;
#$tbox_url_path = $url_path;
#$milestone     = "trunk";
#$notify_list   = "cmp\@mozilla.org";
#$stub_installer = 1;
#$sea_installer = 1;
#$archive       = 0;
#$push_raw_xpis = 1;

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

# Any extensions that are built can be uploaded to the stage server using these
# config options. Wildcards can be used. These variables are mainly used by
# post-mozilla-rel.pl
#@ReleaseExtensions = (); # set to e.g. ('lightning*.xpi', 'gdata-provider.xpi')
#$ReleaseExtensionSubdir = "" # set to e.g. "xpi"; if not specified, <os>-xpi will be used.