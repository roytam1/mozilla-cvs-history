# -*- Mode: perl; indent-tabs-mode: nil -*-
#
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
# The Original Code are the Bugzilla Tests.
# 
# The Initial Developer of the Original Code is Zach Lipton
# Portions created by Zach Lipton are 
# Copyright (C) 2001 Zach Lipton.  All
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


#################
#Bugzilla Test 1#
###Compilation###
BEGIN { use lib 't/'; }
BEGIN { use Support::Files; }
BEGIN { $tests = @Support::Files::testitems; }
BEGIN { use Test::More tests => $tests; }

use strict;

# First now we test the scripts                                                   
my @testitems = @Support::Files::testitems; 
# Capture the TESTERR from Test::More for printing errors.
# This will handle verbosity for us automatically
*TESTOUT = \*Test::More::TESTOUT;
my $perlapp = $^X;

foreach my $file (@testitems) {
        $file =~ s/\s.*$//; # nuke everything after the first space (#comment)
        next if (!$file); # skip null entries
        open (FILE,$file);
        my $bang = <FILE>;
        close (FILE);
        my $T = "";
        if ($bang =~ m/#!\S*perl\s+-.*T/) {
            $T = "T";
        }
        my $command = "$perlapp"." -c$T $file 2>&1";
        my $loginfo=`$command`;
        #print '@@'.$loginfo.'##';
        if ($loginfo =~ /syntax ok$/im) {
                if ($loginfo ne "$file syntax OK\n") {
                        print TESTOUT $loginfo;
                        ok(0,$file."--WARNING");
                } else {
                        ok(1,$file);
                }
        } else {
                print TESTOUT $loginfo;
                ok(0,$file."--ERROR");
        }
}      

# Remove the lib testing from here since it is now done 
# in Files.pm








