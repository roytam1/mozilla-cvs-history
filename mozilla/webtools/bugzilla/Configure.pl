#!/usr/bonsaitools/bin/perl -w

# The contents of this file are subject to the Mozilla Public
# License Version 1.1 (the "License"); you may not use this file
# except in compliance with the License. You may obtain a copy of
# the License at http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an "AS
# IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
# implied. See the License for the specific language governing
# rights and limitations under the License.
#
# The Original Code is the Bugzilla Installer.
#
# The Initial Developer of the Original Code is Zach Lipton
# Portions created by Zach Lipton are
# Copyright (C) 2002 Zach Lipton.  All
# Rights Reserved.
#
# Contributor(s): Zach Lipton <zach@zachlipton.com>
#
# Alternatively, the contents of this file may be used under the
# terms of the GNU General Public License Version 2 or later (the
# "GPL"), in which case the provisions of the GPL are applicable
# instead of those above.  If you wish to allow use of your
# version of this file only under the terms of the GPL and not to
# allow others to use your version of this file under the MPL,
# indicate your decision by deleting the provisions above and
# replace them with the notice and other provisions required by
# the GPL.  If you do not delete the provisions above, a recipient
# may use your version of this file under either the MPL or the
# GPL.
#
use File::Spec;
use Conf;

$args = join(' ',@ARGV);

unless ($] >= 5.006) {
    die "Sorry, you need at least Perl 5.6\n";
}

# Insert lines in this space to run .cm files like:
# runconf('Conf/Foo.cm',"Title of section"); 
# runconf() will handle the path mapping for XP purposes
# XXX: NOT SURE IF WE REALLY NEED TO ADJUST THE PATH

if (-e 'Conf/Supplies/config.pl') {
	output <<EOF;
I see you have settings saved from a pervious partial installation. 
Would you like me to restore them and skip ahead to the 
configuration section, using your previous settings?
EOF
	ask('skipahead',"Yes or no?","yes");
	if (getConf('skipahead') =~ /(y|yes)/i) {
		do("Conf/Supplies/config.pl");
		output "\n";
		$args .= "--perl=".getConf('perl');
	}
}

if ($args =~ /--perl\=([\/A-Za-z0-9]+|[\/A-Za-z0-9]+ )/) { # they passed us a perl arg
	setConf('perl',$1); # set the path to perl and move on...
} else {
	# they haven't run configure.pl before so run Welcome and PerlCheck now...
	runconf('Conf/Begin.cm',"Welcome");
	runconf('Conf/PerlCheck.cm',"Checking perl"); 
}
runconf('Conf/ModuleCheck.cm',"Checking required modules");
runconf('Conf/UpgradeCheck.cm',"Installation Type");

# alas, a junction in our plans:
if (getConf('installtype') eq 'convert') {
	runconf('Conf/UpgradeConvert.cm',"checksetup conversion");
	runconf('Conf/Upgrade.cm',"Upgrade");
} elsif (getConf('installtype') eq 'upgrade') {
	runconf('Conf/Upgrade.cm',"Upgrade");
} elsif (getConf('installtype') eq 'new') {
	runconf('Conf/Location.cm',"Installation Location");
	runconf('Conf/NewConfiguration.cm',"Installation Configuration");
}


sub runconf {
    my ($path,$text) = @_;
    my @pathlist = split('/',$path);
    output "\n";
    # oh, isn't ascii art cute? ;)
    for (split(//, $text)) { output "=" }
    output "\n";
    output $text."\n";
    for (split(//, $text)) { output "=" }
    output "\n";
    require File::Spec->catfile(@pathlist);
}

