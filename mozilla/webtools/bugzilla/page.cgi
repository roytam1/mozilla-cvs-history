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
# Contributor(s): Gervase Markham <gerv@gerv.net>
#

###############################################################################
# This CGI is a general template display engine. To display templates using it,
# put them in the "pages" subdirectory of template/en/default, call them
# "foo.<ctype>.tmpl" and use the URL page.cgi?id=foo.<ctype> , where <ctype> is
# a content-type, e.g. html.
###############################################################################

use strict;

use lib ".";
require "CGI.pl";

use vars qw($template $vars);

ConnectToDatabase();

quietly_check_login();

if ($::FORM{'id'}) {
    # Remove all dodgy chars, and split into name and ctype.
    $::FORM{'id'} =~ s/[^\w\-\.]//g;
    $::FORM{'id'} =~ /(.*)\.(.*)/;

    my $format = GetFormat($1, undef, $2);
    
    $vars->{'form'} = \%::FORM; 
    
    print "Content-Type: $format->{'ctype'}\n\n";

    $template->process("pages/$format->{'template'}", $vars)
      || ThrowTemplateError($template->error());
}
else {
    ThrowUserError("no_page_specified");
}
