#!/usr/bin/perl -w
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
# The Original Code is mozilla.org code.
# 
# The Initial Developer of the Original Code is Christopher Blizzard.
# Portions created by Christopher Blizzard are Copyright (C)
# Christopher Blizzard.  All Rights Reserved.
#
# Contributor(s):

# This script will read one of the mozilla packages- file on unix and
# copy it to a target directory.  It's for unix only and is really
# designed for use in building rpms or other packages.

use Getopt::Long;
use File::Find;

use strict;

# global vars
my $srcdir       = "";
my $package_name = "";
my $package_file = "";
my $output_file  = "";
my $shared_pass;
my $retval;

# std return val

$retval = GetOptions('source=s',       \$srcdir,
		     'package=s',      \$package_name,
		     'package-file=s', \$package_file,
		     'output-file=s',  \$output_file,
		     'shared!',        \$shared_pass);

# make sure that all of the values are specific on the command line
if (!$retval || !$srcdir || !$package_name || 
    !$package_file || !$output_file) {
    print_usage();
    exit 1;
}

# try to open the packages file

open (PACKAGE_FILE, $package_file) || die("$0: Failed to open file $package_file for reading.");

print "chdir to $srcdir\n";
chdir($srcdir);

my @file_list;
my @exclude_list;
my @final_file_list;
my $reading_package = 0;

LINE: while (<PACKAGE_FILE>) {
    s/\;.*//;			# it's a comment, kill it.
    s/^\s+//;			# nuke leading whitespace
    s/\s+$//;			# nuke trailing whitespace
    
    # it's a blank line, skip it.
    if (/^$/) {
	next LINE;
    }

    # it's a new component
    if (/^\[/) {
	my $this_package;
	( $this_package ) = /^\[(.+)\]$/;
	if ($this_package eq $package_name) {
	    $reading_package = 1;
	}
	else {
	    $reading_package = 0;
	}
	next LINE;
    }

    # read this line
    if ($reading_package) {
	# see if it's a deletion
	if (/^-/) {
	    my $this_file;
	    ( $this_file ) = /^-(.+)$/;
	    push (@exclude_list, $this_file);
	}
	else {
	    push (@file_list, $_);
	}
    }
}

close PACKAGE_FILE;

# Expand our file list

expand_file_list(\@file_list, \@exclude_list, \@final_file_list);

print "final file list\n";
foreach (@final_file_list) {
    print $_ . "\n";
}

open (OUTPUT_FILE, ">>$output_file") || die("Failed to open output file\n");
foreach (@final_file_list) {
    # strip off the bin/
    s/^bin\///;

    # if it's a shared library and we're doing a shared pass print it.
    # otherwise ignore it.
    my $is_shared_library = 0;
    $is_shared_library = /^[a-zA-Z0-9]+\.so$/;
    if ($shared_pass && $is_shared_library) {
	print ("Adding $_\n");
	print (OUTPUT_FILE $_ . "\n");
    }
    elsif (!$shared_pass && !$is_shared_library) {
	print ("Adding $_\n");
	print (OUTPUT_FILE $_ . "\n");
    }
    else {
	print("Ignoring $_\n");
    }
}
close OUTPUT_FILE;

#print "\nexlude list\n";
#foreach (@exclude_list) {
#    print $_ . "\n";
#}

# this function expands a list of files

sub expand_file_list {
    my $file_list_ref = shift;
    my $exclude_list_ref = shift;
    my $final_file_list_ref = shift;
    my $this_file;
    foreach $this_file (@{$file_list_ref}) {
	# is it a wild card?
	if ($this_file =~ /\*$/) {
	    print "Wild card $this_file\n";
	    # expand that wild card, removing anything in the exclude
	    # list
	    my @temp_list;
	    @temp_list = glob($this_file);
	    foreach $this_file (@temp_list) {
		if (!in_exclude_list($this_file, $exclude_list_ref)) {
		    push (@{$final_file_list_ref}, $this_file);
		}
	    }
	}
	else {
	    if (!in_exclude_list($this_file, $exclude_list_ref)) {
		push (@{$final_file_list_ref}, $this_file);
	    }
	}
    }
}

# is this file in the exlude list?

sub in_exclude_list {
    my $file = shift;
    my $exclude_list_ref = shift;
    my $this_file;
    foreach $this_file (@{$exclude_list_ref}) {
	if ($file eq $this_file) {
	    return 1;
	}
    }
    return 0;
}

# print out a usage message

sub print_usage {
    print ("$0: --source dir --package name --package-file file --output-file file [--shared]\n");
    print ("\t source is the source directory where the files can be found.\n");
    print ("\t package is the name of the package to list\n");
    print ("\t package-file is the file that contains the list of packages\n");
    print ("\t output-file is the file which will contain the list of files\n");
    print ("\t shared pulls out only the shared libraries\n");
}
