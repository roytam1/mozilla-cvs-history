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

confirm_login();

my $platform = $::cgi->param('product');
my $version = $::cgi->param('version');

my $platform_cookie = $::cgi->cookie(-name=>"PLATFORM",
                                     -value=>"$platform",
                                     #-path=>'/bugzilla/',
                                     -expires=>"Sun, 30-Jun-2029 00:00:00 GMT");
my $version_cookie = $::cgi->cookie(-name=>"VERSION-$platform",
                                     -value=>"$version",
                                     #-path=>'/bugzilla/',
                                     -expires=>"Sun, 30-Jun-2029 00:00:00 GMT");

if ($::cgi->param('maketemplate') ne "") {
    PutHeader("Bookmarks are your friend.", "Template constructed.");
    
    my $url = "enter_bug.cgi?$::cgi->query_string";

    print "If you put a bookmark <A HREF=\"$url\">to this link</A>, it will\n";
    print "bring up the submit-a-new-bug page with the fields initialized\n";
    print "as you've requested.\n";
    exit;
}

PutHeader("Posting Bug -- Please wait", "Posting Bug", "One moment please...");

umask 0;
ConnectToDatabase();

if ($::cgi->param('component') eq "") {
    print "You must choose a component that corresponds to this bug.  If\n";
    print "necessary, just guess.  But please hit the <B>Back</B> button\n";
    print "and choose a component.\n";
    print $::cgi->end_html;
    exit 0
}
    

my $forceAssignedOK = 0;
if ($::cgi->param('assigned_to') eq "") {
    SendSQL("select initialowner from components where program=" .
            SqlQuote($::cgi->param('product')) .
            " and value=" . SqlQuote($::cgi->param('component')));
    my $data = FetchOneColumn();
    $::cgi->param(-name=>'assigned_to', -value=>$data, -override=>"1");
    $forceAssignedOK = 1;
}

my $assign = $::cgi->param('assigned_to');
$::cgi->param(-name=>'assigned_to', 
              -value=>DBNameToIdAndCheck($assign, $forceAssignedOK),
              -override=>"1");
my $report = ($::cgi->param('Bugzilla_login') ne "") ? $::cgi->param('Bugzilla_login') 
                                                     : $::cgi->cookie('Bugzilla_login');;
$::cgi->param(-name=>'reporter',
              -value=>DBNameToIdAndCheck($report),
              -override=>"1");


my @bug_fields = ("reporter", "product", "version", "rep_platform",
                  "bug_severity", "priority", "op_sys", "assigned_to",
                  "bug_status", "short_desc", "component", "bug_file_loc", 
                  "view");
my $query = "insert into bugs (\n" . join(",\n", @bug_fields) . ",
group_id, creation_ts, long_desc )
values (
";

foreach my $field (@bug_fields) {
    $query .= SqlQuote($::cgi->param($field)) . ",\n";
}

my $gid_query = "SELECT groupid FROM profiles " . 
	"WHERE userid = '" . $::cgi->param('reporter') . "'";
#print $::cgi->pre("$gid_query");
SendSQL($gid_query);
my $gid = FetchOneColumn();
#print $::cgi->pre("gid = $gid) . $cgi->br . "\n";
$query .= "$gid, ";

my $view_query;
my $view_id;
my $view_name;

#if ($::cgi->param('view') ne "") {
#    $view_query = "SELECT type_id FROM type WHERE name = '" . 
#	$::cgi->param('view') . "'";
#    SendSQL($view_query);
#    ($view_id, $view_name) = FetchSQLData();
#    $query .= "'$view_id' ,";
#} else {
#    $view_query = "SELECT type_id FROM type WHERE name = 'public'";
#    SendSQL($view_query);
#    ($view_id, $view_name) = FetchSQLData();
#    $query .= "'$view_id', ";
#}

$query .= "now(), " . SqlQuote($::cgi->param('comment')) . " )\n";

#print $::cgi->param("$query") . "\n";

my %ccids;

if ($::cgi->param('cc') ne "") {
    foreach my $person (split(/[ ,]/, $::cgi->param('cc'))) {
        if ($person ne "") {
            $ccids{DBNameToIdAndCheck($person)} = 1;
        }
    }
}

#print $::cgi->param("$query") . "\n";

SendSQL($query);

SendSQL("select LAST_INSERT_ID()");
my $id = FetchOneColumn();

foreach my $person (keys %ccids) {
    SendSQL("insert into cc (bug_id, who) values ($id, $person)");
}

print $::cgi->h2("Changes Submitted"),
      $::cgi->a({-href=>"show_bug.cgi?id=$id"}, "Show BUG# $id"),
      $::cgi->br,
      $::cgi->a({-href=>"query.cgi"}, "Back To Query Page"),
      $::cgi->br,
      $::cgi->a({-href=>"enter_bug.cgi?product=" . $::cgi->param('product')}, 
           "Enter a new bug");

system("./processmail $id < /dev/null > /dev/null 2> /dev/null &");
exit;
