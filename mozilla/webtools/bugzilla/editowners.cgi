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

# Code derived from editparams.cgi

use diagnostics;
use strict;

require "CGI.pl";

# Shut up misguided -w warnings about "used only once":
use vars %::COOKIE;

confirm_login();

print "Content-type: text/html\n\n";

if (Param("maintainer") ne $::COOKIE{Bugzilla_login}) {
    print "<H1>Sorry, you aren't the maintainer of this system.</H1>\n";
    print "And so, you aren't allowed to edit the parameters of it.\n";
    exit;
}


PutHeader("Edit Component Owners");

print "This lets you edit the owners of the program components of bugzilla.\n";

print "<form method=post action=doeditowners.cgi><table>\n";

my $rowbreak = "<tr><td colspan=2><hr></td></tr>";

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

my @line;
my $curProgram = "";

while (@line = FetchSQLData()) {
    if ($line[0] ne $curProgram) {
        print $rowbreak;
        print "<tr><th align=right valign=top>$line[0]:</th><td></td></tr>\n";
        $curProgram = $line[0];
    }
    print "<tr><td valign = top>$line[1]</td><td>\n";
    print "<input size=80 name=\"$line[3]\" value=\"$line[2]\">\n";
    print "</td></tr>\n";
}

print "</table>\n";

print "<input type=submit value=\"Submit changes\">\n";

print "</form>\n";

print "<p><a href=query.cgi>Skip all this, and go back to the query page</a>\n";
