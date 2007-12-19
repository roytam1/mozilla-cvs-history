#! /usr/bin/perl
# -*- Mode: perl; indent-tabs-mode: nil -*-
#
# The contents of this file are subject to the Mozilla Public License
# Version 1.1 (the "License"); you may not use this file except in
# compliance with the License. You may obtain a copy of the License at
# http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an "AS IS"
# basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
# License for the specific language governing rights and limitations
# under the License.
#
# The Original Code is Tinderbox
#
# The Initial Developer of the Original Code is Netscape Communications
# Corporation. Portions created by Netscape are Copyright (C) 1999
# Netscape Communications Corporation. All Rights Reserved.
#
# Contributor(s): Chris McAfee <mcafee@netscape.com>

#
# scrape.pl - Process log-scraped data into scrape.dat
#   Write data to $tree/scrape.dat in the following format,
#
#  <logfilename>|blurb1|blurb2|blurb3 ...
#

use strict;

sub usage {
  warn "./scrape.pl <tree> <logfile>";
}

use Compress::Zlib;
use lib "@TINDERBOX_DIR@";
require "tbglobals.pl";
my $debug = 0;

$ENV{PATH} = "@SETUID_PATH@";

unless ($#ARGV == 1) {
  &usage;
  die "Error: Wrong number of arguments\n";
}

my ($tree, $logfile) = @ARGV;

print "scrape.pl($tree, $logfile)\n" if ($debug);

$tree = &trick_taint($tree);
$logfile = &trick_taint($logfile);

die "Error: No tree named $tree" unless -r "$::tree_dir/$tree/treedata.pl";
require "$::tree_dir/$tree/treedata.pl";

# Search the build log for the scrape data
#
my $gz = gzopen("$::tree_dir/$tree/$logfile", "rb")
  or die "gzopen($::tree_dir/$tree/$logfile): $!\n";
my @scrape_data = find_scrape_data($gz);
$gz->gzclose();

if (!defined(@scrape_data)) {
    print "No scrape data found in log.\n" if ($debug);
    exit(0);
}

# Save the scrape data to 'scrape.dat'
#
my $lockfile = "$::tree_dir/$tree/scrape.sem";
my $lock = &lock_datafile($lockfile);
open(SCRAPE, ">>", "$::tree_dir/$tree/scrape.dat") or die "Unable to open $::tree_dir/$tree/scrape.dat";
print SCRAPE "$logfile|".join('|', @scrape_data)."\n";
close SCRAPE;
&unlock_datafile($lock);
unlink($lockfile);

#print "scrape_data = ";
#my $i;
#foreach $i (@scrape_data) {
#  print "$i ";
#}
#print "\n";


# end of main
#============================================================

sub find_scrape_data {
  my ($gz) = $_[0];
  local $_;
  my @rv;
  my @line;
  my ($bytesread, $gzline);
  while (defined($gz) && (($bytesread = $gz->gzreadline($gzline)) > 0)) {
      if ($gzline =~ m/TinderboxPrint:/) {
          # Line format:
          #  TinderboxPrint:<general html>
          
          # Strip off the TinderboxPrint: part of the line
          chomp($gzline);
          $gzline =~ s/.*TinderboxPrint://;
          
          # No longer use ; to create separate lines.
          #@line = split(';', $_);
          
          push(@rv, $gzline);
      }
  }
  return @rv;
}
