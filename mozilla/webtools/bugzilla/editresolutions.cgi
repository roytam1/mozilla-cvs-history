#!/usr/bin/perl -Tw
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
# The Initial Developer of the Original Code is Terry Weissman.
# Portions created by Terry Weissman are
# Copyright (C) 2000 Terry Weissman. All
# Rights Reserved.
#
# Contributor(s): Matthew Tuck <matty@chariot.net.au>

use diagnostics;
use strict;
use lib ".";

require 'admineditor.pl';

################################################################################
# Silliness
################################################################################

sub edit_resolutions_sillyness {
    my $zz;
    $zz = $::valuetype;
    $zz = $::valuetypeicap;
    $zz = $::valuetypeplural;
    $zz = $::grouprestrict;
    $zz = $::thiscgi;
    $zz = $::tablename;
    $zz = $::bugsreftablename;
    $zz = $::bugsreffieldref;
    $zz = $::maxnamesize;
    $zz = $::maxrestype;
    $zz = @::defaultrest;
    $zz = $::validaterestref;
    $zz = $::extravarsref;
    $zz = $::successfulrestype;
    $zz = $::unsuccessfulrestype;
    $zz = $::duperestype;
    $zz = $::movedrestype;
    $zz = $::tryagain;
    $zz = $::usesortkeys;
}

################################################################################
# Set up variables to use admin editor.
################################################################################

$::valuetype = "resolution";
$::valuetypeicap = "Resolution";
$::valuetypeplural = "resolutions";
$::grouprestrict = "tweakparams";
$::thiscgi = "editresolutions.cgi";

$::tablename = "resolutions";
$::bugsreftablename = "bugs";
$::bugsreffieldref = "$::bugsreftablename.resolution_id";
$::maxnamesize = 64;
$::usesortkeys = 1;

$::validaterestref = sub ($) {

    my ($fieldsref) = @_;

    # Validate restype
    if (!defined $::FORM{restype}) {
        ThatDoesntValidate("restype");
        exit;
    } elsif ( ($::FORM{restype} =~ /^\s*(\d)+\s*$/) && (0 <= $1) && ($1 <= $::maxrestype) ) {
        $$fieldsref{restype} = $1;
    } else {
        ThatDoesntValidate("restype");
        exit;
    }

    # Extra name check
    my $uniqlength = 4;

    my $namestart = SqlQuote(substr($$fieldsref{name}, 0, $uniqlength));

    if (defined $$fieldsref{id}) {
        SendSQL("SELECT name " .
                "FROM   $::tablename " .
                "WHERE  id != $$fieldsref{id} " .
                "  AND  SUBSTRING(name, 1, $uniqlength) = $namestart");
    } else {
        SendSQL("SELECT name " .
                "FROM   $::tablename " .
                "WHERE  SUBSTRING(name, 1, $uniqlength) = $namestart");
    }

    my $htmlname = html_quote($$fieldsref{name});

    my ($dupename) = FetchSQLData();

    if (defined $dupename) {
        my $htmldupename = html_quote($dupename);
        DisplayError("The $::valuetype $htmlname has the same first " .
                     "$uniqlength characters as least one other resolution, " .
                     "eg $htmldupename. $::tryagain\n");
        exit;
    }

};

$::extravarsref = sub ($) {
    my ($varsref) = @_;

    my @restypes = (
        { id => $::successfulrestype, name => 'Successful',
          description => 'These resolutions are for when the bug report has been closed successfully. ' .
                         'This isn\'t used for anything at the moment, but probably will be shortly.' },
        { id => $::unsuccessfulrestype, name => 'Unsuccessful',
          description => 'These resolutions are the most common - when the bug report has been closed ' .
                         'unsuccessfully for some reason.' },
        { id => $::duperestype, name => 'Duplicate',
          description => 'These resolutions are for marking a bug a duplicate of another bug.' },
        { id => $::movedrestype, name => 'Moved',
          description => 'These resolutions are for when a bug has been moved to another Bugzilla installation.' }
    );

    %$varsref->{'restypes'} = \@restypes;
};

%::defaultrest = ( 'restype' => 1 );

################################################################################
# Call the admin editor code
################################################################################

AdminEditor();
