#!/usr/bin/perl -w
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
#                 David Gardiner <david.gardiner@unisa.edu.au>

use diagnostics;
use strict;

require "CGI.pl";

use vars %::COOKIE, %::FILENAME;

sub Punt {
    my ($str) = (@_);
    print "<CENTER>$str<P>Please hit <b>Back</b> and try again.</CENTER>\n";
    exit;
}

confirm_login();

my $id = $::FORM{'id'};

print "Content-type: text/html\n\n";
PutHeader("Create an attachment", "Create attachment", "Bug $id");

if (!defined($::FORM{'data'})) {
    print qq{
<TABLE ALIGN=center><TR><TD ALIGN=left>
<FORM METHOD=post ENCTYPE="multipart/form-data">
<INPUT TYPE=hidden NAME=id value=$id>
To attach a file to <A HREF="show_bug.cgi?id=$id">bug $id</A>, place it in a
file on your local machine, and enter the path to that file here:<BR>
<INPUT TYPE=file NAME=data SIZE=60>
<P>
Please also provide a one-line description of this attachment:<BR>
<INPUT NAME=description SIZE=60>
<BR>
What kind of file is this?
<BR><INPUT TYPE=radio NAME=type VALUE=patch>Patch file (text/plain, diffs)
<BR><INPUT TYPE=radio NAME=type VALUE="text/plain">Plain text (text/plain)
<BR><INPUT TYPE=radio NAME=type VALUE="text/html">HTML source (text/html)
<BR><INPUT TYPE=radio NAME=type VALUE="image/gif">GIF Image (image/gif)
<BR><INPUT TYPE=radio NAME=type VALUE="image/jpeg">JPEG Image (image/jpeg)
<BR><INPUT TYPE=radio NAME=type VALUE="image/png">PNG Image (image/png)
<BR><INPUT TYPE=radio NAME=type VALUE="application/octet-stream">Binary file (application/octet-stream)
<BR><INPUT TYPE=radio NAME=type VALUE="other">Other (enter mime type: <INPUT NAME=othertype size=30>)
<P>
<CENTER><INPUT TYPE=submit VALUE="Submit"></CENTER>
</FORM>
</TD></TR></TABLE>
<P>
};
} else {
    if ($::FORM{'data'} eq "" || !defined $::FILENAME{'data'}) {
        Punt("No file was provided, or it was empty.");
    }
    my $desc = trim($::FORM{'description'});
    if ($desc eq "") {
        Punt("You must provide a description of your attachment.");
    }
    my $ispatch = 0;
    my $mimetype = $::FORM{'type'};
    if (!defined $mimetype) {
        Punt("You must select which kind of file you have.");
    }
    $mimetype = trim($mimetype);
    if ($mimetype eq "patch") {
        $mimetype = "text/plain";
        $ispatch = 1;
    }
    if ($mimetype eq "other") {
        $mimetype = $::FORM{'othertype'};
    }
    if ($mimetype !~ m@^(\w|-)+/(\w|-)+$@) {
        Punt("You must select a legal mime type.  '<tt>$mimetype</tt>' simply will not do.");
    }

	if ($::driver eq "mysql") {
	    SendSQL("insert into attachments (bug_id, filename, description, mimetype, " .
				"ispatch, submitter_id, thedata) values ($id," .
            	SqlQuote($::FILENAME{'data'}) . ", " . SqlQuote($desc) . ", " .
            	SqlQuote($mimetype) . ", $ispatch, " .
            	DBNameToIdAndCheck($::COOKIE{'Bugzilla_login'}) . ", " .
            	SqlQuote($::FORM{'data'}) . ")");
    	SendSQL("select LAST_INSERT_ID()");
	} else {
		# all this is necessary to allow inserting large fields into Oracle
		use DBD::Oracle qw{:ora_types};

		my $query = "insert into attachments (attach_id, bug_id, filename, description, mimetype, " .
                	"ispatch, submitter_id, creation_ts, thedata) values (attachments_seq.nextval, $id," .
                	SqlQuote($::FILENAME{'data'}) . ", " . SqlQuote($desc) . ", " .
                	SqlQuote($mimetype) . ", $ispatch, " .
                	DBNameToIdAndCheck($::COOKIE{'Bugzilla_login'}) . ", sysdate, :1)";

		my $oracle_sth = $::db->prepare($query);
		$oracle_sth->bind_param(1, $::FORM{'data'}, { ora_field => 'thedata', ora_type => ORA_BLOB });
    	$oracle_sth->execute || die "$query: " . $DBI::errstr;

		# get the attach_id we just created
        SendSQL("select attachments_seq.currval from dual");
	}

    my $attachid = FetchOneColumn();
    AppendComment($id, $::COOKIE{"Bugzilla_login"},
                  "Created an attachment (id=$attachid)\n$desc\n");

    print "<TABLE BORDER=1 CELLSPACING=0 ALIGN=center><TD><H2>Attachment to bug $id created</H2>\n";
    system("./processmail", $id, $::COOKIE{'Bugzilla_login'});
    print "<TD><A HREF=\"show_bug.cgi?id=$id\">Go Back to BUG# $id</A></TABLE>\n";
}

PutFooter();
