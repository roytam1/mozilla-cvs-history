# -*- Mode: perl; indent-tabs-mode: nil -*-

# VCDisplay::ViewCVS - This is the implemenation to use if you have
# installed ViewCVS to let your webserver render html pages of your
# CVS repository.

# Currently ViewCVS does not understand all the standard options for
# Bonsai. It is missing: modules and min/max dates options on the
# query.  I would add these features into ViewCVS but I can not tell
# if the project is abandoned or not.

# $Revision$ 
# $Date$ 
# $Author$ 
# $Source$ 
# $Name$ 



# The contents of this file are subject to the Mozilla Public
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

# complete rewrite by Ken Estes:
#	 kestes@staff.mail.com Old work.
#	 kestes@reefedge.com New work.
#	 kestes@walrus.com Home.
# Contributor(s): 





package VCDisplay::ViewCVS;


# Load standard perl libraries


# Load Tinderbox libraries

use lib '#tinder_libdir#';
use HTMLPopUp;
use TreeData;

# Remember the HTMLPopUp.pm function Link accepts these arguments:

# Link(
#      "statuslinetxt"=>"", 
#      "windowtxt"=>"", 
#      "linktxt"=>"", 
#      "name"=>"", 
#      "href"=>"",
#     );

# We will pass these values through to Link (except for href which is
# created using additional arguments as needed by the specific
# function).

$VERSION = '0.09';



# this is how we create a URL to the various CGI programs.

# If you your using VCDisplay:ViewCVS we need to know how to make HTML
# to point the ViewCVS CGI programs.

$VIEWCVS_URL = ( $TinderConfig::VIEWCVS_URL || 
                "../viewcvs");

$QUERY = $VIEWCVS_URL."/query.cgi";
$GUESS = $VIEWCVS_URL."/viewcvs.cgi";

# ViewCVS wants the CVS root to be a local root.  i.e. use '/cvsroot'
# not ':pserver:anonymous@cvs-mirror.mozilla.org:/cvsroot'.  Sometimes
# it is convienent to keep the remote server information in the
# tinderbox config so that we can run BOTH the CVS and Bosai TinderDB
# modules at the same time.

# return the local cvsroot portion of the cvsroot.

sub remote_cvsroot2local_cvsroot {
  my ($root) = @_;
  
  # remove everything up till the last colon.
  $root =~ s!^.*\:!!g;

  return $root;
}


# convert time() format to human readable bonsai format
sub time2bonsai {
  my ($time) = @_;

  my ($sec,$min,$hour,$mday,$mon,
      $year,$wday,$yday,$isdst) =
        localtime($time);
  $mon++;
  $year += 1900;
  my ($bonsai_date_str) = sprintf("%02u/%02u/%04u %02u:%02u:%02u",
                                  $mon, $mday, $year, $hour, $min, $sec);


  $bonsai_date_str = HTMLPopUp::escapeURL($bonsai_date_str);

  return $bonsai_date_str;
}


sub prepare_bonsai_args {
    my (%args) = @_;

    my @url_args = ();

    # bonsai has certain conventions for what we would normally
    # consider empty variables.
    
    my ($tree) = ($args{'tree'} ||
                  'default');  
    
    my ($module) = (TreeData::TreeName2Module($tree) ||
                    'all');
    
    my ($root) = TreeData::TreeName2Root($tree);
    $root = remote_cvsroot2local_cvsroot($root);
        
    my ($branch) = TreeData::TreeName2Branch($tree);
    
    ($tree) &&
        (push @url_args, "treeid=".HTMLPopUp::escapeURL($tree));
    
    ($root) &&
        (push @url_args, "repository=".HTMLPopUp::escapeURL($root),
	 "cvsroot=".HTMLPopUp::escapeURL($root) );
    
    # Bonsai wants to see the branch name 'HEAD' but ViewCVS does not.

    ($branch) &&
	($branch ne 'HEAD') &&
	    (push @url_args, "branch=".HTMLPopUp::escapeURL($branch));

    ($args{'who'}) &&
        (push @url_args, "who=".HTMLPopUp::escapeURL($args{'who'}) );
    
    ($args{'file'}) &&
        (push @url_args, "file=".HTMLPopUp::escapeURL($args{'file'}) );

    ($args{'mindate'} || $args{'maxdate'}) && 
        (push @url_args, "date=explicit" );
    
    ($module) &&
        (push @url_args, "module=".HTMLPopUp::escapeURL($module));
    
    ($args{'mindate'} && $args{'maxdate'}) && 
    ($args{'mindate'} > $args{'maxdate'}) && 
        (die (
              "Bonsai mindate is GREATER than maxdate.\n".
              "Bonsai requires that mindate be an earlier time than maxdate.\n".
              "mindate: $args{'mindate'}: ".time2bonsai($args{'mindate'})."\n".
              "maxdate: $args{'maxdate'}: ".time2bonsai($args{'maxdate'})."\n".
              ""));

    # Convert times to a human readable date for the convience of the
    # users.  Bonsai excepts data of form: 'mm/dd/yyyy hh:mm:ss' in
    # addition to time format.
    
    ($args{'mindate'}) &&
        (push @url_args, "mindate=".time2bonsai($args{'mindate'}) );
    
    ($args{'maxdate'}) && 
        (push @url_args, "maxdate=".time2bonsai($args{'maxdate'}) );

    return @url_args;
}

# create a Link to a VC file and its line number

sub source {
  my ($self, %args) = @_;

  if ($DEBUG) {
    ($args{'tree'} && $args{'file'} && $args{'line'} && $args{'linktxt'}) ||
      die("function VCDisplay::source, not called with enough arguments ");
    
    (TreeData::tree_exists($args{'tree'})) ||
      die("function VCDisplay::source, tree: $args{'tree'} does not exist\n");
  }

  my $line = $args{'line'} || 1;
  my ($goto_line) = ($line > 10 ? $line - 10 : 1 );
  
  my ($tree) = ($args{'tree'} ||
                'default');

  my ($branch) = TreeData::TreeName2Module($tree);


  my @url_args = prepare_bonsai_args(%args);

  ($branch) &&
      (push @url_args, "rev=".HTMLPopUp::escapeURL($branch) );

  ($args{'line'}) &&
      (push @url_args, "mark=".HTMLPopUp::escapeURL($args{'line'}."\#$goto_line") );

  $args{'annotate'} = "HEAD";

  $args{'href'} = ("$GUESS?".join('&', @url_args));

  my $output = HTMLPopUp::Link(%args);
  
  return $output;
}


# Create a Link to a VC file and its line number.

# This function is used when only the basename of the file is provided
# and some additional work must be done to figure out the file which
# was meant.  Most compilers only give the basename in their error
# messages and leave the determination of the directory to the user
# who is looking at the error log.

sub guess {
  my ($self, %args) = @_;

  if ($DEBUG) {
    ($args{'tree'} && $args{'file'} && $args{'line'} && $args{'linktxt'}) ||
      die("function VCDisplay::guess, not called with enough arguments ");
    
    (TreeData::tree_exists($args{'tree'})) ||
      die("function VCDisplay::guess, tree: $args{'tree'} does not exist\n");
  }

  my $line = $args{'line'} || 1;
  my ($goto_line) = ($line > 10 ? $line : 1 );
  
  my @url_args = prepare_bonsai_args(%args);

  my ($tree) = ($args{'tree'} ||
                'default');

  my ($branch) = TreeData::TreeName2Module($tree);


  ($branch) &&
      (push @url_args, "rev=".HTMLPopUp::escapeURL($branch) );

  ($args{'line'}) &&
      (push @url_args, "mark=".HTMLPopUp::escapeURL($args{'line'}."\#$goto_line") );

  # we must use the annotate here since there are no line number names
  # in the 'rev' output and we may need to point a particular line.

  $args{'annotate'} = "HEAD";

  $args{'href'} = ("$GUESS?".join('&', @url_args));

  my $output = HTMLPopUp::Link(%args);
  
  return $output;
}


# Query VC about checkins by a user or during a particular time or
# with a particular file.


sub query {
  my ($self, %args) = @_;

  if ($DEBUG) {
    ($args{'tree'} && $args{'linktxt'}) ||
      die("function VCDisplay::query, not called with enough arguments ");
    ($args{'mindate'} || $args{'who'}) || 
      die("function VCDisplay::query, not called with enough arguments ");
    
    (TreeData::tree_exists($args{'tree'})) ||
      die("function VCDisplay::query, tree: $args{'tree'} does not exist\n");
  }

  my @url_args = prepare_bonsai_args(%args);

  $args{'href'} = ("$QUERY?".join('&', @url_args));

  my ($output) = HTMLPopUp::Link(%args);

  return $output;
}


1;
