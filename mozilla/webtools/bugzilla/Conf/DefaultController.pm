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

package Conf::DefaultController;
no warnings;

use Exporter;
@ISA = qw(Exporter);
@EXPORT = qw(_ask _holduntilkey _output);


# If you are creating a custom version of the controller, you can fill
# in this array with questions where you just want the default used 
# (see below for a way to change the default) and the user should not
# be asked for an answer (intended for debian-style configuration systems).
# just add the question-name into this array.
@ignorequestions = qw(

);

# Or, perhaps you are creating a custom version of the controller and you 
# want to change the default answers to reflect your distro. Just add 
# setConf lines below to change default values: 
# example: _setConf('questionName','answer');

sub _ask($$$) {
	my ($name,$question, $default) = @_;
	
	foreach $cur (@ignorequestions) { #check to see if we should ignore it
		if ($cur eq $name) { # the question should be ignored
			print "$question [$default]\n";
			print "AUTOMATICALLY ANSWERED\n";
			return $default;
		}
	}
	
	print "$question [$default]";
    $answer = <STDIN>;
    print "\n";
    chomp($answer);
    return $answer;
}

sub _holduntilkey() {
	if (Conf::getParam('quiet') ne 1) { # if they want us to be quiet, don't pause
		my $input = <STDIN>;
	}
}

sub _output($$) {
	my ($output, $loud) = @_;
    print $output;
}

1;

