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
use vars %::param,
    %::param_default,
    @::param_list;

confirm_login();

if (Param("maintainer") ne $::cgi->cookie('Bugzilla_login')) {
    PutHeader("Denied!");
    print $::cgi->h1("Sorry, you aren't the maintainer of this system.");
    print "And so, you aren't allowed to edit the parameters of it.\n";
    print $::cgi->end_html;
    exit;
}

PutHeader("Saving new parameters");

my $cgiItem;

foreach my $i (@::param_list) {
#    print "Processing $i...<BR>\n";
    if ($::cgi->param("reset-$i") ne "") {
        $::cgi->param(-name=>$i, -value=>$::param_default{$i}, -override=>"1");
    }
    $cgiItem = $::cgi->param($i);
    $cgiItem =~ s/\r\n/\n/;     # Get rid of windows-style line endings.
    if ($cgiItem ne Param($i)) {
        if (defined $::param_checker{$i}) {
            my $ref = $::param_checker{$i};
            my $ok = &$ref($cgiItem);
            if ($ok ne "") {
                print "New value for $i is invalid: $ok<p>\n";
                print "Please hit <b>Back</b> and try again.\n";
                exit;
            }
        }
        print "Changed $i.<BR>\n";
        $::param{$i} = $::cgi->param($i);
    }
}

WriteParams();

print "OK, done.<P>\n",
      $::cgi->a({-href=>"editparams.cgi"}, "Edit the params some more.");
      $::cgi->a({-href=>"query.cgi"}, "Go back to the query page.");
