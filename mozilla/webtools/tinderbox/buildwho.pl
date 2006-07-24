#!/usr/bonsaitools/bin/perl --
# -*- Mode: perl; indent-tabs-mode: nil -*-
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
# The Original Code is the Tinderbox build tool.
#
# The Initial Developer of the Original Code is Netscape Communications
# Corporation. Portions created by Netscape are
# Copyright (C) 1998 Netscape Communications Corporation. All
# Rights Reserved.
#
# Contributor(s): 

use FileHandle;
use File::Copy 'move';
use Fcntl qw(:DEFAULT :flock);

use lib "@TINDERBOX_DIR@";
require 'tbglobals.pl';
$F_DEBUG=1;

$ENV{'PATH'} = "@SETUID_PATH@";

# Process args:
#   $days: How many days of data to process.
#   $tree: Which tree to use.
$days = $tree = undef;

if ($ARGV[0] eq '-days') {
    shift;
    $days = &trick_taint(shift);
}
$tree = &trick_taint(shift);

&usage() if (!defined($tree));

# Grab globals for this tree:
#   $who_days:    Number of days of commit info to make available
#   $cvs_module:  The checkout module
#   $cvs_branch:  The current branch
#   $cvs_root:    The path to the cvs root
#   $bonsai_tree: The data directory for this tree in ../bonsai
#   $viewvc_repository: Repository path used by viewvc for this tree
require "$tree/treedata.pl";

$days = $who_days if (!defined($days));

# Exit early if no query system is enabled
exit 0 if (!$use_bonsai && !$use_viewvc);

# Only allow one process at a time to re-write "who.dat".
#
my $lock = lock_datafile($tree);

if ($use_bonsai) {
    # Setup global variables for bonsai query
    #
    if ($cvs_root eq '') {
        $CVS_ROOT = '/m/src';
    } else {
        $CVS_ROOT = $cvs_root;
    }

    $CVS_REPOS_SUFIX = $CVS_ROOT;
    $CVS_REPOS_SUFIX =~ s/\//_/g;
    
    $CHECKIN_DATA_FILE = "/d/webdocs/projects/bonsai/data/checkinlog${CVS_REPOS_SUFIX}";
    $CHECKIN_INDEX_FILE = "/d/webdocs/projects/bonsai/data/index${CVS_REPOS_SUFIX}";

    use lib "/d/webdocs/projects/bonsai";
    require 'cvsquery.pl';

    print "cvsroot='$CVS_ROOT'\n" if $F_DEBUG;
} elsif ($use_viewvc) {
    require 'viewvc.pl';
}


build_who($tree);

unlock_datafile($lock);

# End of main
##################################################################
sub usage() {
    print "Usage: $0 [-days days_of_commitinfo] treename\n";
    exit 1;
}

sub lock_datafile {
    my ($tree) = @_;

    my $lock_fh = new FileHandle ">>$tree/buildwho.sem"
      or die "Couldn't open semaphore file!";

    # Get an exclusive lock with a non-blocking request
    unless (flock($lock_fh, LOCK_EX|LOCK_NB)) {
        die "buildwho.pl: Lock unavailable: $!";
    }
    return $lock_fh;
}

sub unlock_datafile {
    my ($lock_fh) = @_;

    flock $lock_fh, LOCK_UN;  # Free the lock
    close $lock_fh;
}

sub build_who {
    my ($tree) = @_;
    $query_date_min = time - (60 * 60 * 24 * $days);

    print "Minimum date: $query_date_min\n" if $F_DEBUG;

    if ($use_viewvc) {
        $query_module=$viewvc_repository;
    } elsif ($use_bonsai) {
        $query_module=$cvs_module;
        $query_branch=$cvs_branch;
    } else {
        # Should never reach this
        return;
    }

    $query_branchtype='regexp' if $query_branch =~ /\*|\?|\+/;
    $::query_branch_head=1 if $::query_branch eq 'HEAD';

    my $who_file = "$tree/who.dat";
    my $temp_who_file = "$who_file.$$";
    open(WHOLOG, ">", "$temp_who_file");

    if ($use_bonsai) {
        chdir "../bonsai";
        $::TreeID = $bonsai_tree;
    }
    my $result = &query_checkins(%mod_map);
        
    $last_who='';
    $last_date=0;
    for $ci (@$result) {
        if ($ci->[$CI_DATE] != $last_date or $ci->[$CI_WHO] ne $last_who) {
            print WHOLOG "$ci->[$CI_DATE]|$ci->[$CI_WHO]\n";
        }
        $last_who=$ci->[$CI_WHO];
        $last_date=$ci->[$CI_DATE];
    }
    close (WHOLOG);
    if ($use_bonsai) {
        chdir "../tinderbox";
    }
    move($temp_who_file, $who_file);
}
