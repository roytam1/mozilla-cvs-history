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
# Contributor(s): Terry Weissman <terry@mozilla.org>
#                 Andrew Anderson <andrew@redhat.com>


use diagnostics;
use strict;
use CGI;

$::cgi = new CGI;

require "CGI.pl";
require "defparams.pl";

# Shut up misguided -w warnings about "used only once":
use vars @::param_desc,
    @::param_list;

confirm_login();

if (Param("maintainer") ne $::cgi->cookie('Bugzilla_login')) {
    PutHeader("Denied!");
    print $::cgi->h1("Sorry, you aren't the maintainer of this system.");
    print "And so, you aren't allowed to edit the parameters of it.\n";
    print $::cgi->end_html;
    exit;
}

PutHeader("Edit parameters");

print "This lets you edit the basic operating parameters of bugzilla.\n";
print "Be careful!\n";
print "<P>\n";
print "Any item you check Reset on will get reset to its default value.\n";

print $::cgi->start_form(-method=>"POST", -action=>"doeditparams.cgi");

my @table_array;
my $table_row;

my $rowbreak = $::cgi->TR($::cgi->td({-colspan=>"2"}, $::cgi->hr));
push(@table_array, $rowbreak);

foreach my $i (@::param_list) {
    $table_row = $::cgi->TR(
                      $::cgi->th({-align=>"RIGHT", -valign=>"TOP"}, "$i:"),
		      $::cgi->td($::param_desc{$i}));
    push(@table_array, $table_row);
    $table_row = $::cgi->TR(
                      $::cgi->td({-valign=>"TOP"}, 
		         $::cgi->checkbox(-name=>"reset-$i", -label=>"Reset")));
    push(@table_array, $table_row);
    my $value = Param($i);
    SWITCH: for ($::param_type{$i}) {
	/^t$/ && do {
	    $table_row = $::cgi->textfield(-name=>"$i",
	                                   -size=>"80",
					   '-value'=>"$value",
					   -override=>"1");
            push(@table_array, $table_row);
            last SWITCH;
	};
	/^l$/ && do {
	    $table_row = $::cgi->textarea({-wrap=>"HARD"},
	                                 -name=>"$i",
					 -rows=>"10",
					 -cols=>"80",
					 '-value'=>"$value");
            push(@table_array, $table_row);
            last SWITCH;
	};
        /^b$/ && do {
	    my $default;
            if ($value) {
	        $default = 'on';
            } else {
	        $default = 'off';
            }
	    $table_row = $::cgi->radio_group(-name=>"$i",
	                                     '-values'=>['on', 'off'],
					     -default=>$default);
            push(@table_array, $table_row);
            last SWITCH;
        };
        # DEFAULT
	$table_row = $::cgi->font({-color=>"RED"}, 
	               $::cgi->blink("Unknown param type $::param_type{$i}!"));
        push(@table_array, $table_row);
    }
    push(@table_array, $rowbreak);
}

$table_row = $::cgi->TR(
                $::cgi->th({-align=>"RIGHT", -valign=>"TOP"}, "version:"),
		$::cgi->td("What version of Bugzilla this is.  This can't " .
		           "be modified here, but " . $::cgi->tt("%version%") .
			   " can be used as a parameter in places that " .
			   "understand such parameters.")
	     ),
	     $::cgi->TR(
	        $::cgi->td("&nbsp;"),
	        $::cgi->td(Param('version'))
	     );
push(@table_array, $rowbreak);

print 
      $::cgi->table(@table_array),
      $::cgi->reset,
      $::cgi->submit(-name=>"submit", -value=>"Submit changes"),
      $::cgi->endform;
      $::cgi->p,
      $::cgi->a({-href=>"query.cgi"}, 
         "Skip all this, and go back to the query page.");
