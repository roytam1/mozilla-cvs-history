#!/usr/bonsaitools/bin/perl -w
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
#                 David Gardiner <david.gardiner@unisa.edu.au>

use diagnostics;
use strict;

require "CGI.pl";

# Shut up misguided -w warnings about "used only once":

use vars %::FORM;

ConnectToDatabase();

# Clear out the login cookies.  Make people log in again if they create an
# account; otherwise, they'll probably get confused.

print "Set-Cookie: Bugzilla_login= ; path=/bugzilla2/; expires=Sun, 30-Jun-80 00:00:00 GMT
Set-Cookie: Bugzilla_logincookie= ; path=/bugzilla2/; expires=Sun, 30-Jun-80 00:00:00 GMT
Set-Cookie: Bugzilla_password= ; path=/bugzilla2/; expires=Sun, 30-Jun-80 00:00:00 GMT
Content-type: text/html

";


my $login = $::FORM{'login'};
my $realname = $::FORM{'realname'};
if (defined $login) {
    CheckEmailSyntax($login);
    if (DBname_to_id($login) != 0) {
	PutHeader("Account exists");
	print "<CENTER>A bugzilla account for the name <B><tt>$login</tt></B> already\n";
	print "exists.<P>If you have forgotten the password for it, then\n";
	print "<a href=query.cgi?GoAheadAndLogIn>click here</a> and use\n";
	print "the <b>E-mail me a password</b> button.</CENTER>\n";
        PutFooter();
	exit;
    }
    PutHeader("Account created");
    my $password = InsertNewUser($login, $realname);
    MailPassword($login, $password);
    print "<CENTER>A bugzilla account for <B><tt>$login</tt></B> has been created.  The\n";
    print "password has been e-mailed to that address.<P>When it is\n";
    print "received, you may <a href=query.cgi?GoAheadAndLogIn>click\n";
    print "here</a> and log in.  Or, you can just <a href=\"\">go back to\n";
    print "the top</a>.</CENTER>\n";
    PutFooter();
    exit;
}

PutHeader("Create a new bugzilla account");
print qq{
<CENTER><B>
To create a bugzilla account, all that you need to do is to enter a<BR>
legitimate e-mail address. The account will be created, and its<BR>
password will be mailed to you. Optionally you may enter your<BR>real name 
as well.

<FORM method=get>
<TABLE>
<TR>
	<TH ALIGN=right>E-mail address:</TH>
	<TD><INPUT SIZE=35 NAME=login></TD>
</TR><TR>
	<TD ALIGN=right><B>Real name:</B></TD>
	<TD><INPUT SIZE=35 NAME=realname></TD>
</TR>
</TABLE>
<INPUT TYPE=submit NAME=submit VALUE="Submit">
</CENTER>
};

PutFooter();
