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
# Contributor(s): Terry Weissman <terry@mozilla.org>
#                 Matthew Tuck <matty@chariot.net.au>
#                 Myk Melez <myk@mozilla.org>

# Make it harder for us to do dangerous things in Perl.
use diagnostics;
use strict;
use lib ".";

# Include the Bugzilla CGI and general utility library.
require "CGI.pl";

# Use the template toolkit (http://www.template-toolkit.org/) to generate
# the user interface (HTML pages and mail messages) using templates in the
# "template/" subdirectory.
use Template;

# Create the global template object that processes templates and specify
# configuration parameters that apply to all templates processed in this script.
my $template = Template->new( {

    # Colon-separated list of directories containing templates.
    INCLUDE_PATH => "template/custom:template/default" ,

    # Allow templates to be specified with relative paths.
    RELATIVE => 1

} );

################################################################################
# Some constants callers might want to use
################################################################################

$::tryagain = 'Please press <b>Back</b> and try again.';
$::wentwrong = 'Something went wrong.';

################################################################################
# The following must be set up by the caller.
################################################################################

# What we're editing eg ('resolution', 'Resolution', 'resolutions').
($::valuetype, $::valuetypeicap, $::valuetypeplural) = ();

# What group you have to be in to edit these.
$::grouprestrict = undef;

# The name of the CGI calling the editor.
$::thiscgi = undef;

# The name of the table that stores what we're editing.
$::tablename = undef;

# The name of the table that has references to what we're editing.
# This currently doesn't support multiple references.
$::bugsreftablename = undef;

# The full field name of the field that has references to what we're editing,
# eg 'bugs.resolution_id'.
$::bugsreffieldref = undef;

# The maximum number of characters allowed in the name of what we're editing.
# Get this information from the schema.
$::maxnamesize = undef;

# Whether to use sortkeys.
$::usesortkeys = undef;

################################################################################
# The following may be changed by the caller.
################################################################################

# Whether we can delete this thing when it is referred to in the DB.
$::candeleterefsref = sub ($) { return 0; };
# If so, a subroutine to do it.
$::deleterefsref = sub ($) { die 'Shouldn\'t be here! (admineditor.pl/deleterefsref)'; };

# This allows us to add extra vars to the template, which is passed in as a hashref.
$::extravarsref = sub ($) {};

# "Rest" allows us to extend the code in this file to handle extra things, called
# "the rest".

# These check extra errors and warnings.
$::extraerrorsref = sub ($) {};
$::extrawarningsref = sub ($) { return (); };

# This takes the rest, and does anything necessary for use in SQL, eg
# SqlQuoting strings.
$::preparerestforsqlref = sub ($) {};

# This is the default rest.
%::defaultrest = ();

################################################################################
# Begin admin editor code
################################################################################

sub ValidateName ($) {

    my ($fieldsref) = @_;

    my $name = $::FORM{name};
    my $id = $$fieldsref{id};

    if (!defined($name)) {
        ThatDoesntValidate("name");
        exit;
    }

    $name = trim($name);

    if ($name eq "") {
        DisplayError("You must enter a non-blank name for the $::valuetype. $::tryagain");
        exit;
    }
    if ($name =~ /[\s,]/) {
        DisplayError("You may not use commas or whitespace in a $::valuetype name. $::tryagain");
        exit;
    }
    if ($::maxnamesize < length($name)) {
        DisplayError("Names can't have more than $::maxnamesize characters. $::tryagain");
        exit;
    }

    my $sqlcondition;
    my $sqlname = SqlQuote($name);

    if (defined $id) {
        $sqlcondition = "name = $sqlname AND id != $id";
    }
    else {
        $sqlcondition = "name = $sqlname";
    }

    if (RecordExists($::tablename, $sqlcondition)) {
        DisplayError("The $::valuetype $name already exists. $::tryagain");
        exit;
    }

    $$fieldsref{name} = $name;

}

sub ValidateDesc ($) {

    my ($fieldsref) = @_;

    my $description = $::FORM{description};

    if (!defined($description)) {
        ThatDoesntValidate("description");
        exit;
    }

    $description = trim($description);

    if ($description eq "") {
        DisplayError("You must enter a non-blank description of the $::valuetype. $::tryagain");
        exit;
    }

    $$fieldsref{description} = $description;

}

sub ValidateID ($) {

    my ($fieldsref) = @_;

    $::FORM{id} = trim($::FORM{id});

    if (detaint_natural($::FORM{id})) {
        $$fieldsref{id} = $::FORM{id};
    }
    else {
        ThatDoesntValidate("id");
        exit;
    }

}

sub ValidateIsActive ($) {

    my ($fieldsref) = @_;

    if (!defined $::FORM{isactive}) {
        $$fieldsref{isactive} = 0;
    }
    elsif ($::FORM{isactive} eq "1") {
        $$fieldsref{isactive} = 1;
    }
    else {
        ThatDoesntValidate("isactive");
        exit;
    }

}

sub ValidateSortKey ($) {

    my ($fieldsref) = @_;

    $::FORM{sortkey} = trim($::FORM{sortkey});

    if (detaint_natural($::FORM{sortkey})) {
        $$fieldsref{sortkey} = $::FORM{sortkey};
    }
    else {
        ThatDoesntValidate('sortkey');
        exit;
    }

}

my $vars;

sub CheckWarnings (%) {

    my (%fields) = @_;

    my @warnings = &$::extrawarningsref(%fields);

    if (@warnings && !$::FORM{reallychange}) {
        $vars->{warnings} = @warnings;
        EmitTemplate("admin/$::valuetypeplural/warnings.atml");
        exit;
    }

}

sub EmitTemplate($) {

    my ($templatename) = @_;

    # Return the appropriate HTTP response headers.
    print "Content-type: text/html\n\n";

    # Generate and return the UI (HTML page) from the appropriate template.
    $template->process($templatename, $vars)
        || DisplayError("Template process failed: " . $template->error())
            && exit;

}

sub ExtraFields() {

    my %defaults = %::defaultrest;
    return keys %defaults;

}

################################################################################
# Main Body Execution
################################################################################

sub AdminEditor() {
    # Preliminary checks.

    confirm_login();

    $::db->{Taint} = 1;
    $::db->{Taint} = 1; # Silliness

    unless (UserInGroup($::grouprestrict)) {
        DisplayError("Sorry, you aren't a member of the $::grouprestrict group. " .
                     "And so, you aren't allowed to add, modify or delete $::valuetypeplural.",
                     "Not allowed");
        exit;
    }

    # Define the global variables and functions that will be passed to the UI 
    # template.  Individual functions add their own values to this hash before
    # sending them to the templates they process.
    $vars = {
        # Function for retrieving global parameters.
        'Param' => \&Param,

        # Function for processing global parameters that contain references
        # to other global parameters.
        'PerformSubsts' => \&PerformSubsts,
        
        # Uses for links that point back to this script.
        'thiscgi' => $::thiscgi,
        
        # What we are actually editing.
        'valuetype' => $::valuetype,
        'valuetypeicap' => $::valuetypeicap,
        
        # Maximum size allowed for a name of this value.
        'maxnamesize' => $::maxnamesize,

        # Whether we're using sortkeys for this value.
        'usesortkeys' => $::usesortkeys
    };

    &$::extravarsref($vars);

    # All calls to this script should contain an "action" variable whose value
    # determines what the user wants to do.  The code below checks the value of
    # that variable and runs the appropriate code.

    # Determine whether to use the action specified by the user or the default.
    my $action = $::FORM{'action'} || 'list';

    my %fields;

    if ($action eq "list") {

        ListScreen("");

    }
    elsif ($action eq "add") {
        
        CreateScreen();
        
    }
    elsif ($action eq "new") {
        
        ValidateName(\%fields);
        ValidateDesc(\%fields);
        ValidateSortKey(\%fields) if ($::usesortkeys);
        &$::extraerrorsref(\%fields);

        CheckWarnings(%fields);

        InsertNew(%fields);
        
    }
    elsif ($action eq "edit") {
        
        ValidateID(\%fields);
        EditScreen(%fields);
        
    }
    elsif ($action eq "update") {
        
        ValidateID(\%fields);
        ValidateName(\%fields);
        ValidateDesc(\%fields);
        ValidateIsActive(\%fields);
        ValidateSortKey(\%fields) if ($::usesortkeys);

        &$::extraerrorsref(\%fields);

        CheckWarnings(%fields);

        UpdateExisting(%fields);
        
    }
    elsif ($action eq "delete") {
        
        ValidateID(\%fields);

        DeleteExisting(%fields);
        
    }
    else {
        
        print "Content-type: text/html\n\n";
        ThatDoesntValidate("action");
        
    }

}

################################################################################
# The Actions
################################################################################

# Screen to present values to user to determine what to do.
# Next action would be CreateScreen, EditScreen or DeleteExisting.
sub ListScreen ($) {

    my ($message) = (@_);

    my $ordering = $::usesortkeys
        ? "$::tablename.sortkey, $::tablename.name"
        : "$::tablename.name";

    SendSQL("SELECT $::tablename.id, $::tablename.name, " .
            "$::tablename.description, $::tablename.isactive, " .
            "COUNT($::bugsreffieldref) " .
            "FROM $::tablename LEFT JOIN $::bugsreftablename ON " .
            "$::tablename.id = $::bugsreffieldref " .
            "GROUP BY $::tablename.id " .
            "ORDER BY $ordering");        

    my @values;

    while (MoreSQLData()) {
        my ($id, $name, $description, $isactive, $bugcount) =
            FetchSQLData();
        $bugcount ||= 0;

        push( @values, { 'id' => $id, 'name' => $name, 'description' => $description,
                         'isactive' => $isactive, 'bugcount' => $bugcount } );
    }

    # Define the variables and functions that will be passed to the UI template.
    $vars->{'values'} = \@values;
    $vars->{'message'} = $message;

    # Generate the template.
    EmitTemplate("admin/$::valuetypeplural/list.atml");

}

# Screen to create a new value.
# Next action would be InsertNew.
sub CreateScreen() {

    $vars->{name} = '';
    $vars->{description} = '';
    $vars->{sortkey} = 0;

    # Defaults for the rest
    %$vars = ( %$vars, %::defaultrest );

    # Generate the template.
    EmitTemplate("admin/$::valuetypeplural/create.atml");

}

# Add value entered on the creation screen.
sub InsertNew(%) {
    
    my (%fields) = @_;

    my $htmlname = html_quote($fields{name});
    my $sqlname = SqlQuote($fields{name});
    my $sqldescription = SqlQuote($fields{description});

    # Pick an unused number.  Be sure to recycle numbers that may have been
    # deleted in the past.  This code is potentially slow, but it happens
    # rarely enough.

    SendSQL("SELECT id FROM $::tablename ORDER BY id");

    my $newid = 1;

    while (MoreSQLData()) {
        my $oldid = FetchOneColumn();

        detaint_natural($oldid) || die "Failed to detaint next seqnum.";

        if ($oldid > $newid) {
            last;
        }
        $newid = $oldid + 1;
    }

    # Do proper conversion for inclusion in SQL
    $fields{id} = $newid;
    $fields{name} = $sqlname;
    $fields{description} = SqlQuote($fields{description});
    &$::preparerestforsqlref(\%fields);

    # Add the new record.
    my $fieldnames = join(', ', keys(%fields));
    my $fieldvalues = join(', ', values(%fields));

    SendSQL("INSERT INTO $::tablename ($fieldnames) VALUES ($fieldvalues)");

    # Make versioncache flush
    unlink "data/versioncache";

    # Display list with message.
    ListScreen( "$::valuetypeicap $htmlname added." );

}

# Screen to edit existing value.
# Next action would be UpdateExisting.
sub EditScreen (%) {

    my (%fields) = @_;
    my $id = $fields{id};

    my %defaults = %::defaultrest;

    my @fieldnames = ('name', 'description', 'isactive', keys %defaults);
    @fieldnames = (@fieldnames, 'sortkey') if ($::usesortkeys);

    my $fieldnames = join(', ', @fieldnames);

    # get data of record
    SendSQL("SELECT $fieldnames FROM $::tablename WHERE id = $id");

    if (!MoreSQLData()) {
        DisplayError("$::wentwrong I can't find the $::valuetype ID $id.");
        exit;
    }

    my @data = FetchSQLData();

    my $bugcount = GetCount( $::bugsreftablename, "$::bugsreffieldref = $id" );

    # Define the variables and functions that will be passed to the UI template.
    $vars->{'id'} = $id;
    $vars->{'bugcount'} = $bugcount;

    foreach my $fieldname (@fieldnames) {
        my $datum = shift @data;
        $vars->{$fieldname} = $datum;
    }

    # Generate the template.
    EmitTemplate("admin/$::valuetypeplural/edit.atml");

}

# Update the value edited on the edit screen.
sub UpdateExisting (%) {

    my (%fields) = @_;

    my $id = $fields{id};

    my $htmlname = html_quote($fields{name});
    my $sqlname = SqlQuote($fields{name});

    # Do proper conversion for inclusion in SQL
    delete $fields{id};
    $fields{name} = $sqlname;
    $fields{description} = SqlQuote($fields{description});
    &$::preparerestforsqlref(\%fields);

    # Generate the SET SQL
    my $assignments = GenerateUpdateSQL(%fields);

    # Send the SQL
    SendSQL("UPDATE $::tablename SET $assignments WHERE id = $id");

    # Make versioncache flush
    unlink "data/versioncache";

    # Display list with message.
    ListScreen( "$::valuetypeicap $htmlname updated." );

}

# Delete the value selected on the list screen.
sub DeleteExisting (%) {

    my (%fields) = @_;
    my $id = $fields{id};

    SendSQL("SELECT name FROM $::tablename WHERE id = $id");

    if (!MoreSQLData()) {
        DisplayError("$::wentwrong That $::valuetype does not exist!");
        exit;
    }

    my $name = FetchOneColumn();
    my $htmlname = html_quote($name);

    my $bugcount = GetCount($::bugsreftablename, "$::bugsreffieldref = $id");

    if (!$::FORM{reallydelete} or !(&$::candeleterefsref($id))) {

        if (0 < $bugcount) {
            if (&$::candeleterefsref($id)) {
                $vars->{id} = $id;
                $vars->{name} = $name;
                $vars->{bugcount} = $bugcount;

                EmitTemplate("admin/$::valuetypeplural/confirmdelete.atml");
                exit;
            }
            else {
                DisplayError("There are $bugcount bug(s) which have " .
                             "the $::valuetype $htmlname.  You " .
                             "can't delete the $::valuetype while " .
                             "it is on one or more bugs.");
                exit;
            }

        }

    }

    if ($bugcount > 0) {
        &$::deleterefsref($id);
    }

    SendSQL("DELETE FROM $::tablename WHERE id = $id");

    # Make versioncache flush
    unlink "data/versioncache";

    # Display list with message.
    ListScreen( "$::valuetypeicap $htmlname deleted." );

}

1;
