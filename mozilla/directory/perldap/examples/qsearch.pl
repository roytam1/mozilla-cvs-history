#!/usr/bin/perl5
#############################################################################
# $Id$
#
# The contents of this file are subject to the Mozilla Public License
# Version 1.0 (the "License"); you may not use this file except in
# compliance with the License. You may obtain a copy of the License at
# http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an "AS IS"
# basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
# License for the specific language governing rights and limitations
# under the License.
#
# The Original Code is PerLDAP. The Initial Developer of the Original
# Code is Leif Hedstrom and Netscape Communications. Portions created
# by Leif are Copyright (C) Leif Hedstrom, portions created by Netscape
# are Copyright (C) Netscape Communications Corp. All Rights Reserved.
#
# Contributor(s):
#
# DESCRIPTION
#    Quick Search, like ldapsearch, but in Perl. Look how simple it is.
#
#############################################################################

use Getopt::Std;			# To parse command line arguments.
use Mozilla::LDAP::Conn;		# Main "OO" layer for LDAP
use Mozilla::LDAP::Utils;		# LULU, utilities.

use strict;
no strict "vars";


#################################################################################
# Constants, shouldn't have to edit these...
#
$APPNAM	= "qsearch";
$USAGE	= "$APPNAM -b base -h host -D bind -w pswd -P cert filter [attr...]";


#################################################################################
# Check arguments, and configure some parameters accordingly..
#
if (!getopts('b:h:D:p:s:vw:P:'))
{
   print "usage: $APPNAM $USAGE\n";
   exit;
}
%ld = Mozilla::LDAP::Utils::ldapArgs();


#################################################################################
# Now do all the searches, one by one.
#
$conn = new Mozilla::LDAP::Conn(\%ld);
die "Could't connect to LDAP server $ld{host}" unless $conn;

foreach (@ARGV)
{
  if (/\=/)
    {
      push(@srch, $_);
    }
  else
    {
      push(@attr, $_);
    }
}

foreach $search (@srch)
{
  if ($#attr >= $[)
    {
      $entry = $conn->search(basedn	=> $ld{base},
			     scope	=> $ld{scope},
			     filter	=> $search,
			     attributes	=> [ @attr ],
			     verbose	=> $opt_v);
    }
  else
    {
      $entry = $conn->search(basedn	=> $ld{base},
			     scope	=> $ld{scope},
			     filter	=> $search,
			     verbose	=> $opt_v);
    }

  print "Searched for `$search':\n\n";
  $conn->printError() if $conn->getErrorCode();

  while (defined($entry))
    {
      print "$entry";
      $entry = $conn->nextEntry;
    }
  print "\n";
}


#################################################################################
# Close the connection.
#
$conn->close if $conn;
