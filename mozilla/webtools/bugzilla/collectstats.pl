#!/usr/bin/perl -w
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
# The Original Code is the Bugzilla Bug Tracking System.
#
# The Initial Developer of the Original Code is Netscape Communications
# Corporation. Portions created by Netscape are
# Copyright (C) 1998 Netscape Communications Corporation. All
# Rights Reserved.
#
# Contributor(s): Terry Weissman <terry@mozilla.org>,
#                 Harrison Page <harrison@netscape.com>
#                 Gervase Markham <gerv@gerv.net>
#                 Richard Walters <rwalters@qualcomm.com>
#                 Jean-Sebastien Guay <jean_seb@hybride.com>

# Run me out of cron at midnight to collect Bugzilla statistics.


use AnyDBM_File;
use strict;
use IO::Handle;
use vars @::legal_product;

require "globals.pl";

use Bugzilla;

# Turn off output buffering (probably needed when displaying output feedback
# in the regenerate mode.)
$| = 1;

# Tidy up after graphing module
if (chdir("graphs")) {
    unlink <./*.gif>;
    unlink <./*.png>;
    chdir("..");
}

ConnectToDatabase();
GetVersionTable();

Bugzilla->switch_to_shadow_db();

# To recreate the daily statistics,  run "collectstats.pl --regenerate" .
my $regenerate = 0;
if ($#ARGV >= 0 && $ARGV[0] eq "--regenerate") {
    $regenerate = 1;
}

my @myproducts;
push( @myproducts, "-All-", @::legal_product );

my $tstart = time;
foreach (@myproducts) {
    my $dir = "data/mining";

    &check_data_dir ($dir);
    
    if ($regenerate) {
        &regenerate_stats($dir, $_);
    } else {
        &collect_stats($dir, $_);
    }
}
my $tend = time;
print "Total time taken " . delta_time($tstart, $tend) . "\n";

&calculate_dupes();

# Generate a static RDF file containing the default view of the duplicates data.
open(CGI, "GATEWAY_INTERFACE=cmdline REQUEST_METHOD=GET QUERY_STRING=ctype=rdf ./duplicates.cgi |")
  || die "can't fork duplicates.cgi: $!";
open(RDF, ">data/duplicates.tmp")
  || die "can't write to data/duplicates.tmp: $!";
my $headers_done = 0;
while (<CGI>) {
  print RDF if $headers_done;
  $headers_done = 1 if $_ eq "\n";
}
close CGI;
close RDF;
if (-s "data/duplicates.tmp") {
    rename("data/duplicates.rdf", "data/duplicates-old.rdf");
    rename("data/duplicates.tmp", "data/duplicates.rdf");
}

sub check_data_dir {
    my $dir = shift;

    if (! -d $dir) {
        mkdir $dir, 0755;
        chmod 0755, $dir;
    }
}

sub collect_stats {
    my $dir = shift;
    my $product = shift;
    my $when = localtime (time);
    my $product_id = get_product_id($product) unless $product eq '-All-';

    die "Unknown product $product" unless ($product_id or $product eq '-All-');

    # NB: Need to mangle the product for the filename, but use the real
    # product name in the query
    my $file_product = $product;
    $file_product =~ s/\//-/gs;
    my $file = join '/', $dir, $file_product;
    my $exists = -f $file;

    if (open DATA, ">>$file") {
        push my @row, &today;

        foreach my $status ('NEW', 'ASSIGNED', 'REOPENED', 'UNCONFIRMED', 'RESOLVED', 'VERIFIED', 'CLOSED') {
            if( $product eq "-All-" ) {
                SendSQL("select count(bug_status) from bugs where bug_status='$status'");
            } else {
                SendSQL("select count(bug_status) from bugs where bug_status='$status' and product_id=$product_id");
            }

            push @row, FetchOneColumn();
        }

        foreach my $resolution ('FIXED', 'INVALID', 'WONTFIX', 'LATER', 'REMIND', 'DUPLICATE', 'WORKSFORME', 'MOVED') {
            if( $product eq "-All-" ) {
                SendSQL("select count(resolution) from bugs where resolution='$resolution'");
            } else {
                SendSQL("select count(resolution) from bugs where resolution='$resolution' and product_id=$product_id");
            }

            push @row, FetchOneColumn();
        }

        if (! $exists) {
            print DATA <<FIN;
# Bugzilla Daily Bug Stats
#
# Do not edit me! This file is generated.
#
# fields: DATE|NEW|ASSIGNED|REOPENED|UNCONFIRMED|RESOLVED|VERIFIED|CLOSED|FIXED|INVALID|WONTFIX|LATER|REMIND|DUPLICATE|WORKSFORME|MOVED
# Product: $product
# Created: $when
FIN
        }

        print DATA (join '|', @row) . "\n";
        close DATA;
        chmod 0644, $file;
    } else {
        print "$0: $file, $!";
    }
}

sub calculate_dupes {
    SendSQL("SELECT * FROM duplicates");

    my %dupes;
    my %count;
    my @row;
    my $key;
    my $changed = 1;

    my $today = &today_dash;

    # Save % count here in a date-named file
    # so we can read it back in to do changed counters
    # First, delete it if it exists, so we don't add to the contents of an old file
    if (my @files = <data/duplicates/dupes$today*>) {
        unlink @files;
    }
   
    dbmopen(%count, "data/duplicates/dupes$today", 0644) || die "Can't open DBM dupes file: $!";

    # Create a hash with key "a bug number", value "bug which that bug is a
    # direct dupe of" - straight from the duplicates table.
    while (@row = FetchSQLData()) {
        my $dupe_of = shift @row;
        my $dupe = shift @row;
        $dupes{$dupe} = $dupe_of;
    }

    # Total up the number of bugs which are dupes of a given bug
    # count will then have key = "bug number", 
    # value = "number of immediate dupes of that bug".
    foreach $key (keys(%dupes)) 
    {
        my $dupe_of = $dupes{$key};

        if (!defined($count{$dupe_of})) {
            $count{$dupe_of} = 0;
        }

        $count{$dupe_of}++;
    }   

    # Now we collapse the dupe tree by iterating over %count until
    # there is no further change.
    while ($changed == 1)
    {
        $changed = 0;
        foreach $key (keys(%count)) {
            # if this bug is actually itself a dupe, and has a count...
            if (defined($dupes{$key}) && $count{$key} > 0) {
                # add that count onto the bug it is a dupe of,
                # and zero the count; the check is to avoid
                # loops
                if ($count{$dupes{$key}} != 0) {
                    $count{$dupes{$key}} += $count{$key};
                    $count{$key} = 0;
                    $changed = 1;
                }
            }
        }
    }

    # Remove the values for which the count is zero
    foreach $key (keys(%count))
    {
        if ($count{$key} == 0) {
            delete $count{$key};
        }
    }
   
    dbmclose(%count);
}

# This regenerates all statistics from the database.
sub regenerate_stats {
    my $dir = shift;
    my $product = shift;
    my $when = localtime(time());

    my $tstart = time();

    # NB: Need to mangle the product for the filename, but use the real
    # product name in the query
    my $file_product = $product;
    $file_product =~ s/\//-/gs;
    my $file = join '/', $dir, $file_product;

    my @bugs;

    my $and_product = "";
    my $from_product = "";
                    
    if ($product ne '-All-') {
        $and_product = "AND bugs.product_id = products.id " .
                       "AND products.name = " . SqlQuote($product) . " ";
        $from_product = ", products";                       
    }          
              
    # Determine the start date from the date the first bug in the
    # database was created, and the end date from the current day.
    # If there were no bugs in the search, return early.
    SendSQL("SELECT to_days(creation_ts) AS start, " .
            "to_days(current_date) AS end, " .
            "to_days('1970-01-01') " . 
            "FROM bugs $from_product WHERE to_days(creation_ts) != 'NULL' " .
            $and_product .
            "ORDER BY start LIMIT 1");
    
    my ($start, $end, $base) = FetchSQLData();
    if (!defined $start) {
        return;
    }
 
    if (open DATA, ">$file") {
        DATA->autoflush(1);
        print DATA <<FIN;
# Bugzilla Daily Bug Stats
#
# Do not edit me! This file is generated.
#
# fields: DATE|NEW|ASSIGNED|REOPENED|UNCONFIRMED|RESOLVED|VERIFIED|CLOSED|FIXED|INVALID|WONTFIX|LATER|REMIND|DUPLICATE|WORKSFORME|MOVED
# Product: $product
# Created: $when
FIN
        # For each day, generate a line of statistics.
        my $total_days = $end - $start;
        for (my $day = $start + 1; $day <= $end; $day++) {
            # Some output feedback
            my $percent_done = ($day - $start - 1) * 100 / $total_days;
            printf "\rRegenerating $product \[\%.1f\%\%]", $percent_done;

            # Get a list of bugs that were created the previous day, and
            # add those bugs to the list of bugs for this product.
            SendSQL("SELECT bug_id FROM bugs $from_product " .
                    "WHERE bugs.creation_ts < from_days(" . ($day - 1) . ") " . 
                    "AND bugs.creation_ts >= from_days(" . ($day - 2) . ") " .
                    $and_product .
                    " ORDER BY bug_id");
            
            my @row;        
            while (@row = FetchSQLData()) {
                push @bugs, $row[0];
            }

            # For each bug that existed on that day, determine its status
            # at the beginning of the day.  If there were no status
            # changes on or after that day, the status was the same as it
            # is today, which can be found in the bugs table.  Otherwise,
            # the status was equal to the first "previous value" entry in
            # the bugs_activity table for that bug made on or after that
            # day.
            my %bugcount;
            my @logstates = qw(NEW ASSIGNED REOPENED UNCONFIRMED RESOLVED 
                               VERIFIED CLOSED);
            my @logresolutions = qw(FIXED INVALID WONTFIX LATER REMIND 
                                    DUPLICATE WORKSFORME MOVED);
            foreach (@logstates) {
                $bugcount{$_} = 0;
            }
            
            foreach (@logresolutions) {
                $bugcount{$_} = 0;
            }
            
            for my $bug (@bugs) {
                # First, get information on various bug states.
                SendSQL("SELECT bugs_activity.removed " .
                        "FROM bugs_activity,fielddefs " .
                        "WHERE bugs_activity.fieldid = fielddefs.fieldid " .
                        "AND fielddefs.name = 'bug_status' " .
                        "AND bugs_activity.bug_id = $bug " .
                        "AND bugs_activity.bug_when >= from_days($day) " .
                        "ORDER BY bugs_activity.bug_when LIMIT 1");
                
                my $status;
                if (@row = FetchSQLData()) {
                    $status = $row[0];
                } else {
                    SendSQL("SELECT bug_status FROM bugs WHERE bug_id = $bug");
                    $status = FetchOneColumn();
                }
                
                if (defined $bugcount{$status}) {
                    $bugcount{$status}++;
                }

                # Next, get information on various bug resolutions.
                SendSQL("SELECT bugs_activity.removed " .
                        "FROM bugs_activity,fielddefs " .
                        "WHERE bugs_activity.fieldid = fielddefs.fieldid " .
                        "AND fielddefs.name = 'resolution' " .
                        "AND bugs_activity.bug_id = $bug " .
                        "AND bugs_activity.bug_when >= from_days($day) " .
                        "ORDER BY bugs_activity.bug_when LIMIT 1");
                        
                if (@row = FetchSQLData()) {
                    $status = $row[0];
                } else {
                    SendSQL("SELECT resolution FROM bugs WHERE bug_id = $bug");
                    $status = FetchOneColumn();
                }
                
                if (defined $bugcount{$status}) {
                    $bugcount{$status}++;
                }
            }

            # Generate a line of output containing the date and counts
            # of bugs in each state.
            my $date = sqlday($day, $base);
            print DATA "$date";
            foreach (@logstates) {
                print DATA "|$bugcount{$_}";
            }
            
            foreach (@logresolutions) {
                print DATA "|$bugcount{$_}";
            }
            
            print DATA "\n";
        }
        
        # Finish up output feedback for this product.
        my $tend = time;
        print "\rRegenerating $product \[100.0\%] - " .
            delta_time($tstart, $tend) . "\n";
            
        close DATA;
        chmod 0640, $file;
    }
}

sub today {
    my ($dom, $mon, $year) = (localtime(time))[3, 4, 5];
    return sprintf "%04d%02d%02d", 1900 + $year, ++$mon, $dom;
}

sub today_dash {
    my ($dom, $mon, $year) = (localtime(time))[3, 4, 5];
    return sprintf "%04d-%02d-%02d", 1900 + $year, ++$mon, $dom;
}

sub sqlday {
    my ($day, $base) = @_;
    $day = ($day - $base) * 86400;
    my ($dom, $mon, $year) = (gmtime($day))[3, 4, 5];
    return sprintf "%04d%02d%02d", 1900 + $year, ++$mon, $dom;
}

sub delta_time {
    my $tstart = shift;
    my $tend = shift;
    my $delta = $tend - $tstart;
    my $hours = int($delta/3600);
    my $minutes = int($delta/60) - ($hours * 60);
    my $seconds = $delta - ($minutes * 60) - ($hours * 3600);
    return sprintf("%02d:%02d:%02d" , $hours, $minutes, $seconds);
}
