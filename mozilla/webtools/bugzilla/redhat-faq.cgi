#!/usr/bin/perl -w
#
#     The contents of this file are subject to the Mozilla Public License
#     Version 1.0 (the "License"); you may not use this file except in
#     compliance with the License. You may obtain a copy of the License at
#     http://www.mozilla.org/MPL/
#
#     Software distributed under the License is distributed on an "AS IS"
#     basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
#     License for the specific language governing rights and limitations
#     under the License.
#
#     The Original Code is the Bugzilla Bug Tracking System.
#
#     The Initial Developer of the Original Code is Netscape Communications
#     Corporation. Portions created by Netscape are Copyright (C) 1998
#     Netscape Communications Corporation. All Rights Reserved.
#
#     Contributor(s): Terry Weissman <terry@mozilla.org>
#                     David Lawrence <dkl@redhat.com>
push(@INC, '../');

use diagnostics;
use strict;

require 'CGI.pl';

print "Content-type:text/html\n\n";

PutHeader("Bugzilla FAQ");

print "
<TABLE WIDTH=800 ALIGN=center> 
<TR>
	<TD>
<H1>Red Hat Bugzilla FAQ</H1>

<UL>
<LI><A HREF=\"#whatisit\">What is it?</A>
<LI><A HREF=\"#howdoienter\">How do I enter a bug?</A>
<LI><A HREF=\"#whathappens\">What happens once I enter a bug?</A>
<LI><A HREF=\"#howdoisearch\">How do I search for a bug?</A>
<LI><A HREF=\"#howdoipatch\">How do I submit a patch?</A>
<LI><A HREF=\"#cookies\">Are cookies required?</A>
</UL>

<A NAME=\"whatisit\"><H2>What is it?</H2></A>

Bugzilla is a bug tracking system developed at 
<A HREF=\"http://www.mozilla.org/\">mozilla.org</A>.
It was originally used to track the bugs in the 
Mozilla web browser. Red Hat has customized it to 
track bugs in our Linux products.

<A NAME=\"howdoienter\"><H2>How do I enter a bug?</H2></A>

To enter a bug, select <I><A HREF=\"enter_bug.cgi\">Enter a new bug</A></I>
from the <A HREF=\"/bugzilla/\">main bugzilla page</A>.  This will 
take you to a product selection screen.
<P>
From this screen you select the product that you wish to enter a bug
for, by selecting the hightlighted product name.  This will take you 
to a bug entry screen.
<P>
<B>Example:</B> <I>Red Hat Linux</I>
<P>
From the bug entry screen, you need to select the version of the 
product that you are entering a bug on, along with which component
of the product that is having a problem.
<P>
<B>Example:</B> <I>5.2</I> and <I>acm</I>
<P>
Next, select the <A HREF=\"bug_status.cgi#severity\">Severity</A> of your bug.
<P>
<B>Example:</B> <I>Normal</I>. 
<P>
Then you will select which 
<A HREF=\"bug_status.cgi#rep_platform\">Architecture</A> your bug
occurs on.
<P>
<B>Example:</B> If you are using an Intel x86 platform, 
you will choose <I>i386</I>.
<P>
The <I>Cc:</I> field can be used to add someone to the carbon-copy
list for all email related to this bug.  As the reporter of the bug,
you will automatically be copied on any mail, so you do not need to
add yourself to this.
<P>
Enter a one line description of the bug into the <I>Summary</I> field, 
and the full description of the bug into the <I>Description</I> field.

<A NAME=\"whathappens\"><H2>What happens once I enter a bug?</H2></A>

After you enter a bug, mail is sent both to you and the QA department
of Red Hat.  A member of the QA department will verify that they can
reproduce your bug, and may contact you to get additional information.
<P>
After the bug has been <A HREF=\"bug_status.cgi#verified\">verified</A>, 
it will be assigned to a developer to look into a resolution for the bug.
<P>
Once a resolution has been found, the bug will be marked 
<A HREF=\"bug_status.cgi#resolved\">RESOLVED</A> with a 
<A HREF=\"bug_status.cgi#fixed\">resolution status</A>.
<P>
At each step of the process, you will get an email message that will
tell you what has been updated on your bug report.

<A NAME=\"howdoisearch\"><H2>How do I search for a bug?</H2></A>

To search for a bug, select <A HREF=\"query.cgi\"><I>Go to the query page</I></A>
from the <A HREF=\"/bugzilla/\">main bugzilla page</A>.
<P>
The bugzilla search uses an \"OR\" within each field, 
with an \"AND\" between fields.   So, if you were to
select <A HREF=\"bug_status.cgi#new\"><B>NEW</B></A> and 
<A HREF=\"bug_status.cgi#verified\"><B>VERIFIED</B></A> from the 
<A HREF=\"bug_status.cgi#status\"><I>Status</I></A> field and 
<B>normal</B> from the <A HREF=\"bug_status.cgi#priority\"><I>Priority</I></A>
field, you would be asking for all normal priority bugs that are
new or verified.

<A NAME=\"#howdoipatch\"><H2>How do I submit a patch?</H2></A>

The new Bugzilla system supports the attachment of patches, test cases, and
various other forms of file types directly from the bug report screen. Just
click on \"Create an attachment\". You will then be asked for the file name, a
one line description, and the file type. After that it will be uploaded to 
server so that it can be displayed later from the same bug report. 

<A NAME=\"#cookies\"><H2>Are cookies required?</H2></A>

Yes.  If you do not have cookies enabled or are using a browser that
does not support cookies, you will be prompted to re-login for each
screen. It also allows for special features like stored queries and
keeping track of your last query result list.
	</TD>
</TR>
</TABLE>
";

PutFooter();

