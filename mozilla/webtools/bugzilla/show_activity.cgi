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
my $pre = Param("prefix");
require "$pre-security.pl";

print "Content-type: text/html\n\n";

PutHeader("Changes made to bug $::FORM{'id'}", "Activity log",
          "Bug $::FORM{'id'}");

my $query = "
        select bugs_activity.field, bugs_activity.when,
                bugs_activity.oldvalue, bugs_activity.newvalue,
                profiles.login_name
        from bugs_activity,profiles
        where bugs_activity.bug_id = '" . $::FORM{'id'} . "'
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

print "<TABLE BORDER CELLPADDING=\"4\">\n";
print "<TR>\n";
print "    <th>Who</th><th>What</th><th>Old value</th><th>New value</th><th>When</th>\n";
print "</TR>\n";

my @row;
while (@row = FetchSQLData()) {
    my ($field,$when,$old,$new,$who) = (@row);
    $old = value_quote($old);
    $new = value_quote($new);
    print "<TR>\n";
    print "<TD>$who</TD>\n";
    print "<TD>$field</TD>\n";
    print "<TD>$old</TD>\n";
    print "<TD>$new</TD>\n";
    print "<TD>$when</TD>\n";
    print "</TR>\n";
}
print "</TABLE>\n";
print "<HR><A HREF=\"show_bug.cgi?id=$::FORM{'id'}\">Back to bug $::FORM{'id'}</a>\n";
