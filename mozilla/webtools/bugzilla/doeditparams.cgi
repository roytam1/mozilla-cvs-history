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
# Contributor(s): Terry Weissman <terry@mozilla.org>

use diagnostics;
use strict;

require "CGI.pl";
require "defparams.pl";

# Shut up misguided -w warnings about "used only once":
use vars %::param,
    %::param_default,
    @::param_list,
    %::COOKIE;


confirm_login();

print "Content-type: text/html\n\n";

if (Param("maintainer") ne $::COOKIE{'Bugzilla_login'}) {
    print "<H1>Sorry, you aren't the maintainer of this system.</H1>\n";
    print "And so, you aren't allowed to edit the parameters of it.\n";
    exit;
}


PutHeader("Saving new parameters");

foreach my $i (@::param_list) {
#    print "Processing $i...<BR>\n";
    if (exists $::FORM{"reset-$i"}) {
        $::FORM{$i} = $::param_default{$i};
    }
    $::FORM{$i} =~ s/\r\n/\n/;     # Get rid of windows-style line endings.
    if ($::FORM{$i} ne Param($i)) {
        if (defined $::param_checker{$i}) {
            my $ref = $::param_checker{$i};
            my $ok = &$ref($::FORM{$i});
            if ($ok ne "") {
                print "New value for $i is invalid: $ok<p>\n";
                print "Please hit <b>Back</b> and try again.\n";
                exit;
            }
        }
        print "Changed $i.<br>\n";
        $::param{$i} = $::FORM{$i}
    }
}


WriteParams();

print "OK, done.<p>\n";
print "<a href=editparams.cgi>Edit the params some more.</a><p>\n";
print "<a href=query.cgi>Go back to the query page.</a>\n";
    
