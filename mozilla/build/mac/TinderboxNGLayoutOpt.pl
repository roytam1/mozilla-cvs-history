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
use NGLayoutBuildList;
use Cwd;
use Moz;


# configuration variables

$DEBUG = 0;
$ALIAS_SYM_FILES = 0;
$CLOBBER_LIBS = 1;
$MOZ_FULLCIRCLE = 0;

# The following two options will delete all files, but leave the directory structure intact.
$CLOBBER_DIST_ALL = 0;      # turn on to clobber all files inside dist (headers, xsym and libs)
$CLOBBER_DIST_LIBS = 0;     # turn on to clobber the aliases to libraries and sym files in dist
$USE_XPIDL = 1;             # turn on to use the XPIDL plugin to generate files.

$pull{all} = 1;
$pull{lizard} = 0;
$pull{xpcom} = 0;
$pull{imglib} = 0;
$pull{netlib} = 0;
$pull{nglayout} = 0;
$pull{mac} = 0;

$build{all} = 1;
$build{dist} = 0;
$build{runtime}		= 0;
$build{stubs} = 0;
$build{common} = 0;
$build{intl} = 0;
$build{nglayout} = 0;
$build{resources} = 0;
$build{editor} = 0;
$build{mailnews} = 0;
$build{viewer} = 0;
$build{xpapp} = 0;


# script


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

# do the work

OpenErrorLog("::::Mozilla.BuildLog");		# Tinderbox requires that name

chdir("::::");

Checkout();

BuildDist();

BuildProjects();

print "Build NGLayout complete\n";
