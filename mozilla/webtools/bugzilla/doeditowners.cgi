#!@PERL5@ -w
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
# Contributor(s): Sam Ziegler <sam@ziegler.org>

use diagnostics;
use strict;

require "CGI.pl";

# Shut up misguided -w warnings about "used only once":
use vars %::COOKIE;

confirm_login();

print "Content-type: text/html\n\n";

if (Param("maintainer") ne $::COOKIE{'Bugzilla_login'}) {
    print "<H1>Sorry, you aren't the maintainer of this system.</H1>\n";
    print "And so, you aren't allowed to edit the parameters of it.\n";
    exit;
}

PutHeader("Saving new owners");

my %users;
my @line;

SendSQL("select userid, login_name from profiles");

while (@line = FetchSQLData()) {
    $users{$line[1]} = $line[0];
}

SendSQL("
select a.name,
       b.name,
       c.login_name,
       b.component_id
from   products   a,
       components b,
       profiles   c
where  b.product_id = a.product_id and
       c.userid     = b.owner_id
order by a.name, b.name
");

my @updates;
my $curIndex = 0;

while (@line = FetchSQLData()) {
    if (exists $::FORM{$line[3]}) {
        $::FORM{$line[3]} =~ s/\r\n/\n/;
        if ($::FORM{$line[3]} ne $line[2]) {
            if ($users{$::FORM{$line[3]}} eq "") {
                print "<b>$line[0] : $line[1] unchanged, $::FORM{$line[3]} not listed in profiles table</b><BR>\n";
                next;
            }
            print "$line[0] : $line[1] is now owned by $::FORM{$line[3]}.<BR>\n";
            $updates[$curIndex++] = "update components set owner_id = $users{$::FORM{$line[3]}} where component_id = $line[3]";
        }
    }
}

foreach my $update (@updates) {
    SendSQL($update);
}

print "OK, done.<p>\n";
print "<a href=editowners.cgi>Edit the owners some more.</a><p>\n";
print "<a href=query.cgi>Go back to the query page.</a>\n";
