#!/usr/bonsaitools/bin/perl -w
# -*- Mode: perl; indent-tabs-mode: nil -*-
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
# The Original Code is the Bugzilla Bug Tracking System.
# 
# The Initial Developer of the Original Code is Netscape Communications
# Corporation. Portions created by Netscape are Copyright (C) 1998
# Netscape Communications Corporation. All Rights Reserved.
# 
# $Id$
#
# Contributor(s): Sam Ziegler <sam@ziegler.org>
#                 Andrew Anderson <andrew@redhat.com>

# Code derived from editparams.cgi

use diagnostics;
use strict;
use CGI;

$::cgi = new CGI;

require "CGI.pl";

confirm_login();

if (Param("maintainer") ne $::cgi->cookie('Bugzilla_login')) {
    print $::cgi->h1("Sorry, you aren't the maintainer of this system");
    print "And so, you aren't allowed to edit the parameters of it.\n";
    exit;
}

my $rowbreak = $::cgi->TR($::cgi->td({-colspan=>"2"}, $::cgi->hr));

SendSQL("select program, value, initialowner from components order by program, value");

my @line;
my @table_array;
my $table_line;
my $curProgram = "";

while (@line = FetchSQLData()) {
    if ($line[0] ne "$curProgram") {
	push(@table_array, $rowbreak);
	$table_line = $::cgi->TR(
	                 $::cgi->th({-align=>"RIGHT", -valign=>"TOP"}, 
			    "$line[0]")
		      );
	push(@table_array, $table_line);
        $curProgram = $line[0];
    }
    $table_line = $::cgi->TR(
                     $::cgi->td({-valign=>"TOP"}, "$line[1]"),
                     $::cgi->td(
		        $::cgi->textfield(-size=>"80",
			                 -name=>"$line[0]_$line[1]",
			                 -value=>"$line[2]"))
		  );
    push(@table_array, $table_line);
}

PutHeader("Edit Component Owners");

print "This lets you edit the owners of the program components of bugzilla.\n",
      $::cgi->start_form(-action=>"doeditowners.cgi"),
      $::cgi->table(@table_array),
      $::cgi->submit(-name=>"submit", -value=>"Submit changes"),
      $::cgi->endform,
      $::cgi->p, 
      $::cgi->a({-href=>"query.cgi"}, 
              "Skip all this, and go back to the query page"),
      $::cgi->end_html;
