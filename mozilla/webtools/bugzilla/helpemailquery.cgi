#!/usr/bonsaitools/bin/perl -w
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

use diagnostics;
use strict;

require 'CGI.pl';

print "Content-type:text/html\n\n";

PutHeader("Email Help");

print "
<TABLE WIDTH=800>
<TR>
	<TD>
<h1>Help on searching by email address.</h1>

This used to be simpler, but not very powerful.  Now it's really
powerful and useful, but it may not be obvious how to use it...

<p>

To search for bugs associated with an email address:

<ul>
  <li> Type a portion of an email address into the text field.
  <li> Select which fields of the bug you expect that address to be in
       the bugs you're looking for.
</ul>

<p>

You can look for up to two different email addresses; if you specify
both, then only bugs which match both will show up.  This is useful to
find bugs that were, for example, created by Ralph and assigned to
Fred.

<p>

You can also use the drop down menus to specify whether you want to
match addresses by doing a substring match, by using regular
expressions, or by exactly matching a fully specified email address.
	</TD>
</TR>
</TABLE>
";

PutFooter();
