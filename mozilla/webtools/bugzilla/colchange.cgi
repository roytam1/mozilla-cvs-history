#!/usr/bin/perl -wT
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
# Contributor(s): Terry Weissman <terry@mozilla.org>
#                 Gervase Markham <gerv@gerv.net>

use strict;

use lib qw(.);

use vars qw(
  @legal_keywords
  $buffer
  $template
  $vars
);

use Bugzilla;

require "CGI.pl";

ConnectToDatabase();
quietly_check_login();

GetVersionTable();

my $cgi = Bugzilla->cgi;

# The master list not only says what fields are possible, but what order
# they get displayed in.
my @masterlist = ("opendate", "changeddate", "bug_severity", "priority",
                  "rep_platform", "assigned_to", "assigned_to_realname",
                  "reporter", "reporter_realname", "bug_status",
                  "resolution", "product", "component", "version", "op_sys",
                  "votes");

if (Param("usebugaliases")) {
    unshift(@masterlist, "alias");
}
if (Param("usetargetmilestone")) {
    push(@masterlist, "target_milestone");
}
if (Param("useqacontact")) {
    push(@masterlist, "qa_contact");
    push(@masterlist, "qa_contact_realname");
}
if (Param("usestatuswhiteboard")) {
    push(@masterlist, "status_whiteboard");
}
if (@::legal_keywords) {
    push(@masterlist, "keywords");
}

if (UserInGroup(Param("timetrackinggroup"))) {
    push(@masterlist, ("estimated_time", "remaining_time", "actual_time",
                       "percentage_complete")); 
}

push(@masterlist, ("short_desc", "short_short_desc"));

$vars->{'masterlist'} = \@masterlist;

my @collist;
if (defined $::FORM{'rememberedquery'}) {
    my $splitheader = 0;
    if (defined $::FORM{'resetit'}) {
        @collist = @::default_column_list;
    } else {
        foreach my $i (@masterlist) {
            if (defined $::FORM{"column_$i"}) {
                push @collist, $i;
            }
        }
        if (exists $::FORM{'splitheader'}) {
            $splitheader = $::FORM{'splitheader'};
        }
    }
    my $list = join(" ", @collist);
    my $urlbase = Param("urlbase");

    $cgi->send_cookie(-name => 'COLUMNLIST',
                      -value => $list,
                      -expires => 'Fri, 01-Jan-2038 00:00:00 GMT');
    $cgi->send_cookie(-name => 'SPLITHEADER',
                      -value => $::FORM{'splitheader'},
                      -expires => 'Fri, 01-Jan-2038 00:00:00 GMT');

    $vars->{'message'} = "change_columns";
    $vars->{'redirect_url'} = "buglist.cgi?$::FORM{'rememberedquery'}";
    print $cgi->redirect($vars->{'redirect_url'});
    $template->process("global/message.html.tmpl", $vars)
      || ThrowTemplateError($template->error());
    exit;
}

if (defined $::COOKIE{'COLUMNLIST'}) {
    @collist = split(/ /, $::COOKIE{'COLUMNLIST'});
} else {
    @collist = @::default_column_list;
}

$vars->{'collist'} = \@collist;
$vars->{'splitheader'} = $::COOKIE{'SPLITHEADER'} ? 1 : 0;

$vars->{'buffer'} = $::buffer;

# Generate and return the UI (HTML page) from the appropriate template.
print $cgi->header();
$template->process("list/change-columns.html.tmpl", $vars)
  || ThrowTemplateError($template->error());
