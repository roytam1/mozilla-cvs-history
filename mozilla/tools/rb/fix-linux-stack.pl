#!/usr/bin/perl
# ***** BEGIN LICENSE BLOCK *****
# Version: MPL 1.1/GPL 2.0/LGPL 2.1
#
# The contents of this file are subject to the Mozilla Public License Version
# 1.1 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
# http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
# for the specific language governing rights and limitations under the
# License.
#
# The Original Code is fix-linux-stack.pl.
#
# The Initial Developer of the Original Code is L. David Baron.
# Portions created by the Initial Developer are Copyright (C) 2003
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#   L. David Baron <dbaron@dbaron.org> (original author)
#
# Alternatively, the contents of this file may be used under the terms of
# either the GNU General Public License Version 2 or later (the "GPL"), or
# the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
# in which case the provisions of the GPL or the LGPL are applicable instead
# of those above. If you wish to allow use of your version of this file only
# under the terms of either the GPL or the LGPL, and not to allow others to
# use your version of this file under the terms of the MPL, indicate your
# decision by deleting the provisions above and replace them with the notice
# and other provisions required by the GPL or the LGPL. If you do not delete
# the provisions above, a recipient may use your version of this file under
# the terms of any one of the MPL, the GPL or the LGPL.
#
# ***** END LICENSE BLOCK *****

# This script uses addr2line (part of binutils) to process the output of
# nsTraceRefcnt's Linux stack walking code.  This is useful for two
# things:
#  (1) Getting line number information out of
#      |nsTraceRefcntImpl::WalkTheStack|'s output in debug builds.
#  (2) Getting function names out of |nsTraceRefcntImpl::WalkTheStack|'s
#      output on optimized builds (where it mostly prints UNKNOWN
#      because only a handful of symbols are exported from component
#      libraries).
#
# Use the script by piping output containing stacks (such as raw stacks
# or make-tree.pl balance trees) through this script.

use strict;
use IPC::Open2;

my %pipes;
while (<>) {
    my $line = $_;
    if ($line =~ /^([ \|0-9-]*)(.*) ?\[([^ ]*) \+(0x[0-9A-F]{1,8})\](.*)$/) {
        my $before = $1; # allow preservation of balance trees
        my $badsymbol = $2;
        my $file = $3;
        my $address = $4;
        my $after = $5; # allow preservation of counts

        my $pipe;
        unless (exists $pipes{$file}) {
            my $pid = open2($pipe->{read}, $pipe->{write},
                            '/usr/bin/addr2line', '-C', '-f', '-e', "$file");
            $pipes{$file} = $pipe;
        } else {
            $pipe = $pipes{$file};
        }

        my $out = $pipe->{write};
        my $in = $pipe->{read};
        print {$out} "$address\n";
        chomp(my $symbol = <$in>);
        chomp(my $fileandline = <$in>);
        if ($symbol eq "??") { $symbol = $badsymbol; }
        if ($fileandline eq "??:0") { $fileandline = $file; }
        print "$before$symbol ($fileandline)$after\n";
    } else {
        print $line;
    }
}
