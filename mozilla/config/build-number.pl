#
# The contents of this file are subject to the Netscape Public License
# Version 1.0 (the "License"); you may not use this file except in
# compliance with the License.  You may obtain a copy of the License at
# http://www.mozilla.org/NPL/
#
# Software distributed under the License is distributed on an "AS IS"
# basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
# the License for the specific language governing rights and limitations
# under the License.
#
# The Original Code is Mozilla Communicator client code.
#
# The Initial Developer of the Original Code is Netscape Communications
# Corporation.  Portions created by Netscape are Copyright (C) 1998
# Netscape Communications Corporation.  All Rights Reserved.

my $progname = $0;
my $contents;

# this script needs to be run in config
my $numberfile = "build_number";
my $fcsutil = "$ENV{MOZ_TOOLS}\\bin\\fcsutil.pl";

$fcsutil =~ s/\\/\\\\/g;
# This is the preferences file that gets read and written.

open(NUMBER, "<$numberfile") || die "no build_number file\n";

while ( <NUMBER> ) {
    $build_number = $_
}
close (NUMBER);

chop($build_number);

print "$fcsutil\n";
open(OUTPUT, ">master.pl");

print OUTPUT "system(\"perl $fcsutil -newbuild -deployment Netscape/Communicator5.0/Win32/1 -output  master.ini $build_number -server cyclone.mcom.com\")";

close(OUTPUT);

