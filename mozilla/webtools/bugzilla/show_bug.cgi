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

use diagnostics;
use strict;
use CGI;

$::cgi = new CGI;

require "CGI.pl";

confirm_login();

if ($::cgi->param('id') eq "") {
    PutHeader("Search By Bug Number"); 
    print $::cgi->startform .
          "You may find a single bug by entering its bug id here: \n" .
          $::cgi->textfield(-name=>"id", -default=>'') .
          $::cgi->submit(-name=>"submit", -value=>"Show Me This Bug") .
          $::cgi->endform;
    exit;
}

ConnectToDatabase();

GetVersionTable();

# get rid of those damn 'used only once errors'
$::header = $::h1 = $::h2 = "";  

$::bug_id = $::cgi->param("id");
$::header = "Bugzilla Bug $::bug_id";
$::h1 = "Bugzilla Bug";
$::h2 = "$::bug_id";

do "bug_form.pl";
