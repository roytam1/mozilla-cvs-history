#!perl

#
# The contents of this file are subject to the Netscape Public
# License Version 1.1 (the "License"); you may not use this file
# except in compliance with the License. You may obtain a copy of
# the License at http://www.mozilla.org/NPL/
#
# Software distributed under the License is distributed on an "AS
# IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
# implied. See the License for the specific language governing
# rights and limitations under the License.
#
# The Original Code is mozilla.org code.
#
# The Initial Developer of the Original Code is Netscape
# Communications Corporation.  Portions created by Netscape are
# Copyright (C) 1998 Netscape Communications Corporation. All
# Rights Reserved.
#
# Contributor(s): 
#

#
# nglayout pull script 
#
use Cwd;

# Make sure we add the config dir to search
BEGIN
{
  my ($inc_path) = cwd();
  $inc_path =~ s/:build:mac$/:config:/;
  push(@INC, $inc_path);
}
use Mac::Processes;
use NGLayoutBuildList;
use Moz;

# configuration variables
$pull{all} = 1;
$pull{moz} = 0;
$pull{runtime} = 0;

if ($pull{all})
{
    foreach $k (keys(%pull))
    {
        $pull{$k} = 1;
    }
}

# you should not have to edit anything bellow

chdir("::::");

Moz::StopForErrors();
#Moz::DontStopForErrors();

OpenErrorLog("NGLayoutPullLog");

Checkout();
