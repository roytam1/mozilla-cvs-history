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

use strict;
use diagnostics;

use CGI;
$::cgi = new CGI;

require "CGI.pl";

# The master list not only says what fields are possible, but what order
# they get displayed in.

my @masterlist = ("opendate", "changeddate", "severity", "priority",
                  "platform", "owner", "reporter", "status", "resolution",
                  "component", "product", "version", "summary", "summaryfull");


my @collist;
if ($::cgi->param('rememberedquery') ne "") {
    if ($::cgi->param('resetit') ne "") {
        @collist = @::default_column_list;
    } else {
        foreach my $i (@masterlist) {
            if ($::cgi->param("column_$i") ne "") {
                push @collist, $i;
            }
        }
    }
    my $list = join(" ", @collist);
    # FIXME: make path a config option
    my $cookie = $::cgi->cookie(-name=>"COLUMNLIST",
                     -value=>$list,
                     -path=>'/bugzilla/',
                     -expires=>"Sun, 30-Jun-2029 00:00:00 GMT");
    my $rememberedquery = $::cgi->param('rememberedquery');
    print $::cgi->redirect(-uri=>"buglist.cgi?$rememberedquery",
                         -cookie=>$cookie);
    exit;
}

if ($::cgi->cookie('COLUMNLIST') ne "") {
    @collist = split(/ /, $::cgi->cookie('COLUMNLIST'));
} else {
    @collist = @::default_column_list;
}

print $::cgi->header('text/html');

PutHeader("Column Change");

my %desc;
foreach my $i (@masterlist) {
    $desc{$i} = $i;
}

$desc{'summary'} = "Summary (first 60 characters)";
$desc{'summaryfull'} = "Full Summary";


my $query_string = $::cgi->query_string;
print "Check which columns you wish to appear on the list, and then click\n";
print "on submit.\n<P>\n";
print $::cgi->startform(-method=>"POST");
#print $::cgi->hidden(-name=>"rememberedquery", 
#                   -value=>$query_string) . "\n";
print $::cgi->hidden(-name=>"rememberedquery", 
                   -value=>$::cgi->param('querystring'), 
                   -override=>"1") . "\n";

foreach my $i (@masterlist) {
    if (lsearch(\@collist, $i) >= 0) {
        print $::cgi->checkbox(-name=>"column_$i", 
                             -label=>"$desc{$i}",
                             -checked=>'checked') . "<BR>\n";
    } else {
        print $::cgi->checkbox(-name=>"column_$i",
                             -label=>"$desc{$i}") . "<BR>\n";
    }
}
print "<P>\n" . $::cgi->submit(-name=>"submit", -value=>"Submit") . "\n";
print $::cgi->endform . "\n";

print $::cgi->startform;
print $::cgi->hidden(-name=>"resetit", 
                   -value=>"1", 
                   -override=>"1") . "\n";
print $::cgi->submit(-name=>"submit", -value=>"Reset to Bugzilla default") . "\n";
print $::cgi->endform . "\n";
