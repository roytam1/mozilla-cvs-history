#!perl

#
# The contents of this file are subject to the Netscape Public License
# Version 1.0 (the "NPL"); you may not use this file except in
# compliance with the NPL.  You may obtain a copy of the NPL at
# http://www.mozilla.org/NPL/
#
# Software distributed under the NPL is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
# for the specific language governing rights and limitations under the
# NPL.
#
# The Initial Developer of this code under the NPL is Netscape
# Communications Corporation.  Portions created by Netscape are
# Copyright (C) 1998 Netscape Communications Corporation.  All Rights
# Reserved.
#

#
# nglayout build script (debug)
#
use Mac::Processes;
use NGLayoutBuildList;
use Cwd;
use Moz;

# configuration variables
$DEBUG = 1;
$ALIAS_SYM_FILES = $DEBUG;
$CLOBBER_LIBS = 1;			# turn on to clobber existing libs and .xSYM files before
							# building each project
							
# The following two options will delete all files, but leave the directory structure intact.
$CLOBBER_DIST_ALL = 0;      # turn on to clobber all aliases/files inside dist (headers/xsym/libs)
$CLOBBER_DIST_LIBS = 0;     # turn on to clobber only aliases/files for libraries/sym files in dist
$USE_XPIDL = 0;             # turn on to use the XPIDL plugin to generate files.

$CodeWarriorLib::CLOSE_PROJECTS_FIRST = 0;
							#1 = close then make (for development), 0 = make then close (for tinderbox).

$MOZ_FULLCIRCLE = 0;

$pull{all} = 0;
$pull{lizard} = 0;
$pull{xpcom} = 0;
$pull{imglib} = 0;
$pull{netlib} = 0;
$pull{nglayout} = 0;
$pull{mac} = 0;

$build{all} = 1;			# turn off to do individual builds, or to do "most"
$build{most} = 1;			# turn off to do individual builds
$build{dist} = 0;
$build{stubs} = 0;
$build{common} = 0;
$build{intl} = 0;
$build{nglayout} = 0;
$build{resources} = 0;
$build{editor} = 0;
$build{mailnews} = 0;
$build{viewer} = 0;
$build{xpapp} = 0;

if ($pull{all})
{
	foreach $k (keys(%pull))
	{
		$pull{$k} = 1;
	}
}
if ($build{all})
{
	foreach $k (keys(%build))
	{
		$build{$k} = 1;
	}
}
if ($build{most})
{
### Just uncomment/comment to get the ones you want (if "most" is selected).
#	$build{dist} = 1;
#    $build{stubs} = 1;
#	$build{common} = 1; # Requires intl
#   $build{intl} = 1; 
#	$build{nglayout} = 1;
#	$build{resources} = 1;
#	$build{editor} = 1;
	$build{mailnews} = 1;
#	$build{viewer} = 1;
#	$build{xpapp} = 1;
}

# do the work
# you should not have to edit anything below

chdir("::::");
$MOZ_SRC = cwd();

if ($MOZ_FULLCIRCLE)
{
	#// Get the Build Number for the Master.ini(Full Circle) n'stuff
	$buildnum = Moz::SetBuildNumber();
}

$USE_TIMESTAMPED_LOGS = 1;
if ($USE_TIMESTAMPED_LOGS)
{
	#Use time-stamped names so that you don't clobber your previous log file!
	my $now = localtime();
	while ($now =~ s@:@.@) {} # replace all colons by periods
	my $logdir = ":Build Logs:";
	if (!stat($logdir))
	{
	        print "Creating directory $logdir\n";
	        mkdir $logdir, 0777 || die "Couldn't create directory $logdir";
	}
	OpenErrorLog("$logdir$now");
}
else
{
	OpenErrorLog("NGLayoutBuildLog");		# Release build
	#OpenErrorLog("Mozilla.BuildLog");		# Tinderbox requires that name
}

Moz::StopForErrors();
#Moz::DontStopForErrors();

if ($pull{all}) { 
   Checkout();
}

if ($build{dist}) {
	chdir($MOZ_SRC);
	BuildDist();
}

chdir($MOZ_SRC);
BuildProjects();

print "Build layout complete\n";
