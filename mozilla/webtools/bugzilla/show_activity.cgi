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
use CGI;

$::cgi = new CGI;

require "CGI.pl";
require "security.pl";

print $::cgi->header('text/html');

my $bug_id = $::cgi->param("id");

PutHeader("Changes made to bug $bug_id", "Activity log",
          "Bug $bug_id");

my $query = "
        select bugs_activity.field, bugs_activity.when,
                bugs_activity.oldvalue, bugs_activity.newvalue,
                profiles.login_name
        from bugs_activity,profiles
        where bugs_activity.bug_id = '" . $bug_id . "'
        and profiles.userid = bugs_activity.who";

ConnectToDatabase();

my $view_query = "SELECT type_id FROM type where name = 'public'";
SendSQL($view_query);
my $type = FetchOneColumn();

if (CanIView("type")){
        $query .= " and bugs.type = " . $type;
}


$query .= " order by bugs_activity.when";

SendSQL($query);

my @row;
my @table_data;
while (@row = FetchSQLData()) {
    my ($field,$when,$old,$new,$who) = (@row);
    $old = value_quote($old);
    $new = value_quote($new);
    my $line = $::cgi->TR($::cgi->td($who), $::cgi->td($field), 
               $::cgi->td($old), $::cgi->td($new), $::cgi->td($when));
    push(@table_data, $line);
}

print $::cgi->table({-border=>"0", -cellpadding=>"4"},
         $::cgi->TR(
           $::cgi->th("Who"), $::cgi->th("What"), 
           $::cgi->th("Old Value"), $::cgi->th("New Value"),
           $::cgi->th("When")
         ),
         @table_data
      ),
      $::cgi->hr,
      $::cgi->a({-href=>"show_bug.cgi?id=$bug_id"}, "Back to bug $bug_id")
