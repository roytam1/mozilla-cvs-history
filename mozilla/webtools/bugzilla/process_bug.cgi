#!/usr/bonsaitools/bin/perl -wT
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
#                 Dan Mosedale <dmose@mozilla.org>
#                 Dave Miller <justdave@syndicomm.com>
#                 Christopher Aillon <christopher@aillon.com>
#                 Myk Melez <myk@mozilla.org>

use diagnostics;
use strict;

my $UserInEditGroupSet = -1;
my $UserInCanConfirmGroupSet = -1;

use lib qw(.);

require "CGI.pl";
require "bug_form.pl";

use RelationSet;

# Shut up misguided -w warnings about "used only once":

use vars qw(%versions
          %components
          %COOKIE
          %MFORM
          %legal_opsys
          %legal_platform
          %legal_priority
          %settable_resolution
          %target_milestone
          %legal_severity
          %superusergroupset
          $next_bug);

ConnectToDatabase();
my $whoid = confirm_login();

my $requiremilestone = 0;

use vars qw($template $vars);

######################################################################
# Begin Data/Security Validation
######################################################################

# Create a list of IDs of all bugs being modified in this request.
# This list will either consist of a single bug number from the "id"
# form/URL field or a series of numbers from multiple form/URL fields
# named "id_x" where "x" is the bug number.
# For each bug being modified, make sure its ID is a valid bug number 
# representing an existing bug that the user is authorized to access.
my @idlist;
if (defined $::FORM{'id'}) {
    ValidateBugID($::FORM{'id'});
    push @idlist, $::FORM{'id'};
} else {
    foreach my $i (keys %::FORM) {
        if ($i =~ /^id_([1-9][0-9]*)/) {
            my $id = $1;
            ValidateBugID($id);
            push @idlist, $id;
        }
    }
}

# Make sure there are bugs to process.
scalar(@idlist)
  || DisplayError("You did not select any bugs to modify.")
  && exit;

# If we are duping bugs, let's also make sure that we can change 
# the original.  This takes care of issue A on bug 96085.
if (defined $::FORM{'dup_id'} && $::FORM{'knob'} eq "duplicate") {
    ValidateBugID($::FORM{'dup_id'});

    # Also, let's see if the reporter has authorization to see the bug
    # to which we are duping.  If not we need to prompt.
    DuplicateUserConfirm();
}

ValidateComment($::FORM{'comment'});

# If the bug(s) being modified have dependencies, validate them
# and rebuild the list with the validated values.  This is important
# because there are situations where validation changes the value
# instead of throwing an error, f.e. when one or more of the values
# is a bug alias that gets converted to its corresponding bug ID
# during validation.
foreach my $field ("dependson", "blocked") {
    if (defined($::FORM{$field}) && $::FORM{$field} ne "") {
        my @validvalues;
        foreach my $id (split(/[\s,]+/, $::FORM{$field})) {
            next unless $id;
            ValidateBugID($id, 1);
            push(@validvalues, $id);
        }
        $::FORM{$field} = join(",", @validvalues);
    }
}

######################################################################
# End Data/Security Validation
######################################################################

print "Content-type: text/html\n\n";

# Start displaying the response page.
$vars->{'title'} = "Bug processed";
$template->process("global/header.html.tmpl", $vars)
  || ThrowTemplateError($template->error());

$vars->{'header_done'} = 1;

GetVersionTable();

CheckFormFieldDefined(\%::FORM, 'product');
CheckFormFieldDefined(\%::FORM, 'version');
CheckFormFieldDefined(\%::FORM, 'component');

# check if target milestone is defined - matthew@zeroknowledge.com
if ( Param("usetargetmilestone") ) {
  CheckFormFieldDefined(\%::FORM, 'target_milestone');
}

#
# This function checks if there is a comment required for a specific
# function and tests, if the comment was given.
# If comments are required for functions  is defined by params.
#
sub CheckonComment( $ ) {
    my ($function) = (@_);
    
    # Param is 1 if comment should be added !
    my $ret = Param( "commenton" . $function );

    # Allow without comment in case of undefined Params.
    $ret = 0 unless ( defined( $ret ));

    if( $ret ) {
        if (!defined $::FORM{'comment'} || $::FORM{'comment'} =~ /^\s*$/) {
            # No comment - sorry, action not allowed !
            ThrowUserError("comment_required");
        } else {
            $ret = 0;
        }
    }
    return( ! $ret ); # Return val has to be inverted
}

# Figure out whether or not the user is trying to change the product
# (either the "product" variable is not set to "don't change" or the
# user is changing a single bug and has changed the bug's product),
# and make the user verify the version, component, target milestone,
# and bug groups if so.
if ( $::FORM{'id'} ) {
    SendSQL("SELECT name FROM products, bugs " .
            "WHERE products.id = bugs.product_id AND bug_id = $::FORM{'id'}");
    $::oldproduct = FetchSQLData();
}
if ((($::FORM{'id'} && $::FORM{'product'} ne $::oldproduct) 
     || (!$::FORM{'id'} && $::FORM{'product'} ne $::dontchange))
    && CheckonComment( "reassignbycomponent" ))
{
    CheckFormField(\%::FORM, 'product', \@::legal_product);
    my $prod = $::FORM{'product'};

    # note that when this script is called from buglist.cgi (rather
    # than show_bug.cgi), it's possible that the product will be changed
    # but that the version and/or component will be set to 
    # "--dont_change--" but still happen to be correct.  in this case,
    # the if statement will incorrectly trigger anyway.  this is a 
    # pretty weird case, and not terribly unreasonable behavior, but 
    # worthy of a comment, perhaps.
    #
    my $vok = lsearch($::versions{$prod}, $::FORM{'version'}) >= 0;
    my $cok = lsearch($::components{$prod}, $::FORM{'component'}) >= 0;

    my $mok = 1;   # so it won't affect the 'if' statement if milestones aren't used
    if ( Param("usetargetmilestone") ) {
       $mok = lsearch($::target_milestone{$prod}, $::FORM{'target_milestone'}) >= 0;
    }

    # If the product-specific fields need to be verified, or we need to verify
    # whether or not to add the bugs to their new product's group, display
    # a verification form.
    if (!$vok || !$cok || !$mok || (Param('usebuggroups') && !defined($::FORM{'addtonewgroup'}))) {
        $vars->{'form'} = \%::FORM;
        
        if (!$vok || !$cok || !$mok) {
            $vars->{'verify_fields'} = 1;
            my %defaults;
            # We set the defaults to these fields to the old value,
            # if its a valid option, otherwise we use the default where
            # thats appropriate
            $vars->{'versions'} = $::versions{$prod};
            if (lsearch($::versions{$prod}, $::FORM{'version'}) != -1) {
                $defaults{'version'} = $::FORM{'version'};
            }
            $vars->{'components'} = $::components{$prod};
            if (lsearch($::components{$prod}, $::FORM{'component'}) != -1) {
                $defaults{'component'} = $::FORM{'component'};
            }

            if (Param("usetargetmilestone")) {
                $vars->{'use_target_milestone'} = 1;
                $vars->{'milestones'} = $::target_milestone{$prod};
                if (lsearch($::target_milestone{$prod},
                            $::FORM{'target_milestone'}) != -1) {
                    $defaults{'target_milestone'} = $::FORM{'target_milestone'};
                } else {
                    SendSQL("SELECT defaultmilestone FROM products WHERE " .
                            "name = " . SqlQuote($prod));
                    $defaults{'target_milestone'} = FetchOneColumn();
                }
            }
            else {
                $vars->{'use_target_milestone'} = 0;
            }
            $vars->{'defaults'} = \%defaults;
        }
        else {
            $vars->{"verify_fields"} = 0;
        }
        
        $vars->{'verify_bug_group'} = (Param('usebuggroups') 
                                       && !defined($::FORM{'addtonewgroup'}));
        
        $template->process("bug/process/verify-new-product.html.tmpl", $vars)
          || ThrowTemplateError($template->error());
        exit;
    }
}


# Checks that the user is allowed to change the given field.  Actually, right
# now, the rules are pretty simple, and don't look at the field itself very
# much, but that could be enhanced.

my $lastbugid = 0;
my $ownerid;
my $reporterid;
my $qacontactid;

sub CheckCanChangeField {
    my ($f, $bugid, $oldvalue, $newvalue) = (@_);
    if ($f eq "assigned_to" || $f eq "reporter" || $f eq "qa_contact") {
        if ($oldvalue =~ /^\d+$/) {
            if ($oldvalue == 0) {
                $oldvalue = "";
            } else {
                $oldvalue = DBID_to_name($oldvalue);
            }
        }
    }
    if ($oldvalue eq $newvalue) {
        return 1;
    }
    if (trim($oldvalue) eq trim($newvalue)) {
        return 1;
    }
    if ($f =~ /^longdesc/) {
        return 1;
    }
    if ($f eq "resolution") { # always OK this.  if they really can't,
        return 1;             # it'll flag it when "status" is checked.
    }
    if ($UserInEditGroupSet < 0) {
        $UserInEditGroupSet = UserInGroup("editbugs");
    }
    if ($UserInEditGroupSet) {
        return 1;
    }
    if ($lastbugid != $bugid) {
        SendSQL("SELECT reporter, assigned_to, qa_contact FROM bugs " .
                "WHERE bug_id = $bugid");
        ($reporterid, $ownerid, $qacontactid) = (FetchSQLData());
    }
    # Let reporter change bug status, even if they can't edit bugs.
    # If reporter can't re-open their bug they will just file a duplicate.
    # While we're at it, let them close their own bugs as well.
    if ( ($f eq "bug_status") && ($whoid eq $reporterid) ) {
        return 1;
    }
    if ($f eq "bug_status" && $newvalue ne $::unconfirmedstate &&
        IsOpenedState($newvalue)) {

        # Hmm.  They are trying to set this bug to some opened state
        # that isn't the UNCONFIRMED state.  Are they in the right
        # group?  Or, has it ever been confirmed?  If not, then this
        # isn't legal.

        if ($UserInCanConfirmGroupSet < 0) {
            $UserInCanConfirmGroupSet = UserInGroup("canconfirm");
        }
        if ($UserInCanConfirmGroupSet) {
            return 1;
        }
        SendSQL("SELECT everconfirmed FROM bugs WHERE bug_id = $bugid");
        my $everconfirmed = FetchOneColumn();
        if ($everconfirmed) {
            return 1;
        }
    } elsif ($reporterid eq $whoid || $ownerid eq $whoid ||
             $qacontactid eq $whoid) {
        return 1;
    }
    
    # The user doesn't have the necessary permissions to change this field.
    $vars->{'oldvalue'} = $oldvalue;
    $vars->{'newvalue'} = $newvalue;
    $vars->{'field'} = $f;
    ThrowUserError("illegal_change", "abort");
}

# Confirm that the reporter of the current bug can access the bug we are duping to.
sub DuplicateUserConfirm {
    # if we've already been through here, then exit
    if (defined $::FORM{'confirm_add_duplicate'}) {
        return;
    }

    my $dupe = trim($::FORM{'id'});
    my $original = trim($::FORM{'dup_id'});
    
    SendSQL("SELECT reporter FROM bugs WHERE bug_id = " . SqlQuote($dupe));
    my $reporter = FetchOneColumn();
    SendSQL("SELECT profiles.groupset FROM profiles WHERE profiles.userid =".SqlQuote($reporter));
    my $reportergroupset = FetchOneColumn();

    if (CanSeeBug($original, $reporter, $reportergroupset)) {
        $::FORM{'confirm_add_duplicate'} = "1";
        return;
    }

    SendSQL("SELECT cclist_accessible FROM bugs WHERE bug_id = $original");
    $vars->{'cclist_accessible'} = FetchOneColumn();
    
    # Once in this part of the subroutine, the user has not been auto-validated
    # and the duper has not chosen whether or not to add to CC list, so let's
    # ask the duper what he/she wants to do.
    
    $vars->{'form'} = \%::FORM;
    $vars->{'original_bug_id'} = $original;
    $vars->{'duplicate_bug_id'} = $dupe;
    
    # Confirm whether or not to add the reporter to the cc: list
    # of the original bug (the one this bug is being duped against).
    print "Content-type: text/html\n\n";
    $template->process("bug/process/confirm-duplicate.html.tmpl", $vars)
      || ThrowTemplateError($template->error());
    exit;
} # end DuplicateUserConfirm()

if (defined $::FORM{'id'}) {
    # since this means that we were called from show_bug.cgi, now is a good
    # time to do a whole bunch of error checking that can't easily happen when
    # we've been called from buglist.cgi, because buglist.cgi only tweaks
    # values that have been changed instead of submitting all the new values.
    # (XXX those error checks need to happen too, but implementing them 
    # is more work in the current architecture of this script...)
    #
    CheckFormField(\%::FORM, 'rep_platform', \@::legal_platform);
    CheckFormField(\%::FORM, 'priority', \@::legal_priority);
    CheckFormField(\%::FORM, 'bug_severity', \@::legal_severity);
    CheckFormField(\%::FORM, 'component', 
                   \@{$::components{$::FORM{'product'}}});
    CheckFormFieldDefined(\%::FORM, 'bug_file_loc');
    CheckFormFieldDefined(\%::FORM, 'short_desc');
    CheckFormField(\%::FORM, 'product', \@::legal_product);
    CheckFormField(\%::FORM, 'version', 
                   \@{$::versions{$::FORM{'product'}}});
    CheckFormField(\%::FORM, 'op_sys', \@::legal_opsys);
    CheckFormFieldDefined(\%::FORM, 'longdesclength');
}

my $action  = '';
if (defined $::FORM{action}) {
  $action  = trim($::FORM{action});
}
if (Param("move-enabled") && $action eq Param("move-button-text")) {
  $::FORM{'buglist'} = join (":", @idlist);
  do "move.pl" || die "Error executing move.cgi: $!";
  PutFooter();
  exit;
}


$::query = "update bugs\nset";
$::comma = "";
umask(0);

sub DoComma {
    $::query .= "$::comma\n    ";
    $::comma = ",";
}

sub DoConfirm {
    if ($UserInEditGroupSet < 0) {
        $UserInEditGroupSet = UserInGroup("editbugs");
    }
    if ($UserInCanConfirmGroupSet < 0) {
        $UserInCanConfirmGroupSet = UserInGroup("canconfirm");
    }
    if ($UserInEditGroupSet || $UserInCanConfirmGroupSet) {
        DoComma();
        $::query .= "everconfirmed = 1";
    }
}


sub ChangeStatus {
    my ($str) = (@_);
    if ($str ne $::dontchange) {
        DoComma();
        # Ugly, but functional.  We don't want to change Status if we are
        # reasigning non-open bugs via the mass change form.
        if ( ($::FORM{knob} eq 'reassign' || $::FORM{knob} eq 'reassignbycomponent') &&
             ! defined $::FORM{id} && $str eq 'NEW' ) {
            # If we got to here, we're dealing with a reassign from the mass
            # change page.  We don't know (and can't easily figure out) if this
            # bug is open or closed.  If it's closed, we don't want to change
            # its status to NEW.  We have to put some logic into the SQL itself
            # to handle that.
            my @open_state = map(SqlQuote($_), OpenStates());
            my $open_state = join(", ", @open_state);
            $::query .= "bug_status = IF(bug_status IN($open_state), '$str', bug_status)";
        } elsif (IsOpenedState($str)) {
            $::query .= "bug_status = IF(everconfirmed = 1, '$str', '$::unconfirmedstate')";
        } else {
            $::query .= "bug_status = '$str'";
        }
        $::FORM{'bug_status'} = $str; # Used later for call to
                                      # CheckCanChangeField to make sure this
                                      # is really kosher.
    }
}

sub ChangeResolution {
    my ($str) = (@_);
    if ($str ne $::dontchange) {
        DoComma();
        $::query .= "resolution = " . SqlQuote($str);
    }
}

# Changing this so that it will process groups from checkboxes instead of
# select lists.  This means that instead of looking for the bit-X values in
# the form, we need to loop through all the bug groups this user has access
# to, and for each one, see if it's selected.
# In order to make mass changes work correctly, keep a sum of bits for groups
# added, and another one for groups removed, and then let mysql do the bit
# operations
# If the form element isn't present, or the user isn't in the group, leave
# it as-is
if($::usergroupset ne '0') {
    my $groupAdd = "0";
    my $groupDel = "0";

    SendSQL("SELECT bit, isactive FROM groups WHERE " .
            "isbuggroup != 0 AND bit & $::usergroupset != 0 ORDER BY bit");
    while (my ($b, $isactive) = FetchSQLData()) {
        # The multiple change page may not show all groups a bug is in
        # (eg product groups when listing more than one product)
        # Only consider groups which were present on the form. We can't do this
        # for single bug changes because non-checked checkboxes aren't present.
        # All the checkboxes should be shown in that case, though, so its not
        # an issue there
        if ($::FORM{'id'} || exists $::FORM{"bit-$b"}) {
            if (!$::FORM{"bit-$b"}) {
                $groupDel .= "+$b";
            } elsif ($::FORM{"bit-$b"} == 1 && $isactive) {
                $groupAdd .= "+$b";
            }
        }
    }
    if ($groupAdd ne "0" || $groupDel ne "0") {
        DoComma();
        # mysql < 3.23.5 doesn't support the ~ operator, even though
        # the docs say that it does
        $::query .= "groupset = ((groupset & ($::superusergroupset - ($groupDel))) | ($groupAdd))";
    }
}

foreach my $field ("rep_platform", "priority", "bug_severity",          
                   "summary", "bug_file_loc", "short_desc",
                   "version", "op_sys",
                   "target_milestone", "status_whiteboard") {
    if (defined $::FORM{$field}) {
        if ($::FORM{$field} ne $::dontchange) {
            DoComma();
            $::query .= "$field = " . SqlQuote(trim($::FORM{$field}));
        }
    }
}

my $prod_id; # Remember, can't use this for mass changes
if ($::FORM{'product'} ne $::dontchange) {
    $prod_id = get_product_id($::FORM{'product'});
    if (! $prod_id) {
        DisplayError("The <tt>" . html_quote($::FORM{'product'}) .
                     "</tt> product doesn't exist.");
        exit;
    }
    DoComma();
    $::query .= "product_id = $prod_id";
} else {
    SendSQL("SELECT DISTINCT product_id FROM bugs WHERE bug_id IN (" .
            join(',', @idlist) . ") LIMIT 2");
    $prod_id = FetchOneColumn();
    $prod_id = undef if (FetchOneColumn());
}

my $comp_id; # Remember, can't use this for mass changes
if ($::FORM{'component'} ne $::dontchange) {
    if (!defined $prod_id) {
        ThrowUserError("You cannot change the component from a list of bugs " .
                       "covering more than one product");
    }
    $comp_id = get_component_id($prod_id,
                                $::FORM{'component'});
    if (! $comp_id) {
        DisplayError("The <tt>" . html_quote($::FORM{'component'}) .
                     "</tt> component doesn't exist in the <tt>" .
                     html_quote($::FORM{'product'}) . "</tt> product");
        exit;
    }
    DoComma();
    $::query .= "component_id = $comp_id";
}

# If this installation uses bug aliases, and the user is changing the alias,
# add this change to the query.
if (Param("usebugaliases") && defined($::FORM{'alias'})) {
    my $alias = trim($::FORM{'alias'});
    
    # Since aliases are unique (like bug numbers), they can only be changed
    # for one bug at a time, so ignore the alias change unless only a single
    # bug is being changed.
    if (scalar(@idlist) == 1) {
        # Validate the alias if the user entered one.
        if ($alias ne "") {
            # Make sure the alias isn't too long.
            if (length($alias) > 20) {
                ThrowUserError("alias_too_long");
            }

            # Make sure the alias is unique.
            my $escaped_alias = SqlQuote($alias);
            $vars->{'alias'} = $alias;
            
            SendSQL("SELECT bug_id FROM bugs WHERE alias = $escaped_alias " . 
                    "AND bug_id != $idlist[0]");
            my $id = FetchOneColumn();
            
            if ($id) {
                $vars->{'bug_link'} = GetBugLink($id, "Bug $id");
                ThrowUserError("alias_in_use");
            }

            # Make sure the alias isn't just a number.
            if ($alias =~ /^\d+$/) {
                ThrowUserError("alias_is_numeric");
            }

            # Make sure the alias has no commas or spaces.
            if ($alias =~ /[, ]/) {
                ThrowUserError("alias_has_comma_or_space");
            }
        }
        
        # Add the alias change to the query.  If the field contains the blank 
        # value, make the field be NULL to indicate that the bug has no alias.
        # Otherwise, if the field contains a value, update the record 
        # with that value.
        DoComma();
        $::query .= "alias = ";
        $::query .= ($alias eq "") ? "NULL" : SqlQuote($alias);
    }
}

if (defined $::FORM{'qa_contact'}) {
    my $name = trim($::FORM{'qa_contact'});
    if ($name ne $::dontchange) {
        my $id = 0;
        if ($name ne "") {
            $id = DBNameToIdAndCheck($name);
        }
        DoComma();
        $::query .= "qa_contact = $id";
    }
}


# If the user is submitting changes from show_bug.cgi for a single bug,
# and that bug is restricted to a group, process the checkboxes that
# allowed the user to set whether or not the reporter
# and cc list can see the bug even if they are not members of all groups 
# to which the bug is restricted.
if ( $::FORM{'id'} ) {
    SendSQL("SELECT groupset FROM bugs WHERE bug_id = $::FORM{'id'}");
    my ($groupset) = FetchSQLData();
    if ( $groupset ) {
        DoComma();
        $::FORM{'reporter_accessible'} = $::FORM{'reporter_accessible'} ? '1' : '0';
        $::query .= "reporter_accessible = $::FORM{'reporter_accessible'}";

        DoComma();
        $::FORM{'cclist_accessible'} = $::FORM{'cclist_accessible'} ? '1' : '0';
        $::query .= "cclist_accessible = $::FORM{'cclist_accessible'}";
    }
}


my $duplicate = 0;

# We need to check the addresses involved in a CC change before we touch any bugs.
# What we'll do here is formulate the CC data into two hashes of ID's involved
# in this CC change.  Then those hashes can be used later on for the actual change.
my (%cc_add, %cc_remove);
if (defined $::FORM{newcc} || defined $::FORM{removecc} || defined $::FORM{masscc}) {
    # If masscc is defined, then we came from buglist and need to either add or
    # remove cc's... otherwise, we came from bugform and may need to do both.
    my ($cc_add, $cc_remove) = "";
    if (defined $::FORM{masscc}) {
        if ($::FORM{ccaction} eq 'add') {
            $cc_add = $::FORM{masscc};
        } elsif ($::FORM{ccaction} eq 'remove') {
            $cc_remove = $::FORM{masscc};
        }
    } else {
        $cc_add = $::FORM{newcc};
        # We came from bug_form which uses a select box to determine what cc's
        # need to be removed...
        if (defined $::FORM{removecc} && $::FORM{cc}) {
            $cc_remove = join (",", @{$::MFORM{cc}});
        }
    }

    if ($cc_add) {
        $cc_add =~ s/[\s,]+/ /g; # Change all delimiters to a single space
        foreach my $person ( split(" ", $cc_add) ) {
            my $pid = DBNameToIdAndCheck($person);
            $cc_add{$pid} = $person;
        }
    }
    if ($cc_remove) {
        $cc_remove =~ s/[\s,]+/ /g; # Change all delimiters to a single space
        foreach my $person ( split(" ", $cc_remove) ) {
            my $pid = DBNameToIdAndCheck($person);
            $cc_remove{$pid} = $person;
        }
    }
}


CheckFormFieldDefined(\%::FORM, 'knob');
SWITCH: for ($::FORM{'knob'}) {
    /^none$/ && do {
        last SWITCH;
    };
    /^confirm$/ && CheckonComment( "confirm" ) && do {
        DoConfirm();
        ChangeStatus('NEW');
        last SWITCH;
    };
    /^accept$/ && CheckonComment( "accept" ) && do {
        DoConfirm();
        ChangeStatus('ASSIGNED');
        if (Param("musthavemilestoneonaccept") &&
                scalar(@{$::target_milestone{$::FORM{'product'}}}) > 1) {
            if (Param("usetargetmilestone")) {
                $requiremilestone = 1;
            }
        }
        last SWITCH;
    };
    /^clearresolution$/ && CheckonComment( "clearresolution" ) && do {
        ChangeResolution('');
        last SWITCH;
    };
    /^resolve$/ && CheckonComment( "resolve" ) && do {
        # Check here, because its the only place we require the resolution
        CheckFormField(\%::FORM, 'resolution', \@::settable_resolution);
        ChangeStatus('RESOLVED');
        ChangeResolution($::FORM{'resolution'});
        last SWITCH;
    };
    /^reassign$/ && CheckonComment( "reassign" ) && do {
        if ($::FORM{'andconfirm'}) {
            DoConfirm();
        }
        ChangeStatus('NEW');
        DoComma();
        if (!defined$::FORM{'assigned_to'} ||
            trim($::FORM{'assigned_to'}) eq "") {
            ThrowUserError("reassign_to_empty");
        }
        my $newid = DBNameToIdAndCheck(trim($::FORM{'assigned_to'}));
        $::query .= "assigned_to = $newid";
        last SWITCH;
    };
    /^reassignbycomponent$/  && CheckonComment( "reassignbycomponent" ) && do {
        if ($::FORM{'product'} eq $::dontchange) {
            ThrowUserError("need_product");
        }
        if ($::FORM{'component'} eq $::dontchange) {
            ThrowUserError("need_component");
        }
        if ($::FORM{'compconfirm'}) {
            DoConfirm();
        }
        ChangeStatus('NEW');
        SendSQL("SELECT initialowner FROM components " .
                "WHERE components.id = $comp_id");
        my $newid = FetchOneColumn();
        my $newname = DBID_to_name($newid);
        DoComma();
        $::query .= "assigned_to = $newid";
        if (Param("useqacontact")) {
            SendSQL("SELECT initialqacontact FROM components " .
                    "WHERE components.id = $comp_id");
            my $qacontact = FetchOneColumn();
            if (defined $qacontact && $qacontact != 0) {
                DoComma();
                $::query .= "qa_contact = $qacontact";
            }
        }
        last SWITCH;
    };   
    /^reopen$/  && CheckonComment( "reopen" ) && do {
                SendSQL("SELECT resolution FROM bugs WHERE bug_id = $::FORM{'id'}");
        ChangeStatus('REOPENED');
        ChangeResolution('');
                if (FetchOneColumn() eq 'DUPLICATE') {
                        SendSQL("DELETE FROM duplicates WHERE dupe = $::FORM{'id'}");
                }
        last SWITCH;
    };
    /^verify$/ && CheckonComment( "verify" ) && do {
        ChangeStatus('VERIFIED');
        last SWITCH;
    };
    /^close$/ && CheckonComment( "close" ) && do {
        ChangeStatus('CLOSED');
        last SWITCH;
    };
    /^duplicate$/ && CheckonComment( "duplicate" ) && do {
        ChangeStatus('RESOLVED');
        ChangeResolution('DUPLICATE');
        CheckFormFieldDefined(\%::FORM,'dup_id');
        my $num = trim($::FORM{'dup_id'});
        SendSQL("SELECT bug_id FROM bugs WHERE bug_id = " . SqlQuote($num));
        $num = FetchOneColumn();
        if (!$num) {
            ThrowUserError("dupe_invalid_bug_id")
        }
        if (!defined($::FORM{'id'}) || $num == $::FORM{'id'}) {
            ThrowUserError("dupe_of_self_disallowed");
        }
        my $checkid = trim($::FORM{'id'});
        SendSQL("SELECT bug_id FROM bugs where bug_id = " .  SqlQuote($checkid));
        $checkid = FetchOneColumn();
        if (!$checkid) {
            $vars->{'bug_id'} = $checkid;
            ThrowUserError("invalid_bug_id");
        }
        $::FORM{'comment'} .= "\n\n*** This bug has been marked as a duplicate of $num ***";
        $duplicate = $num;

        last SWITCH;
    };
    
    $vars->{'action'} = $::FORM{'knob'};
    ThrowCodeError("unknown_action");
}


if ($#idlist < 0) {
    ThrowUserError("no_bugs_chosen");
}


my @keywordlist;
my %keywordseen;

if ($::FORM{'keywords'}) {
    foreach my $keyword (split(/[\s,]+/, $::FORM{'keywords'})) {
        if ($keyword eq '') {
            next;
        }
        my $i = GetKeywordIdFromName($keyword);
        if (!$i) {
            ThrowUserError("unknown_keyword");
        }
        if (!$keywordseen{$i}) {
            push(@keywordlist, $i);
            $keywordseen{$i} = 1;
        }
    }
}

my $keywordaction = $::FORM{'keywordaction'} || "makeexact";

if ($::comma eq ""
    && (! @::legal_keywords || (0 == @keywordlist && $keywordaction ne "makeexact"))
    && defined $::FORM{'masscc'} && ! $::FORM{'masscc'}
    ) {
    if (!defined $::FORM{'comment'} || $::FORM{'comment'} =~ /^\s*$/) {
        ThrowUserError("bugs_not_changed");
    }
}

my $basequery = $::query;
my $delta_ts;


sub SnapShotBug {
    my ($id) = (@_);
    SendSQL("select delta_ts, " . join(',', @::log_columns) .
            " from bugs where bug_id = $id");
    my @row = FetchSQLData();
    $delta_ts = shift @row;

    return @row;
}


sub SnapShotDeps {
    my ($i, $target, $me) = (@_);
    SendSQL("select $target from dependencies where $me = $i order by $target");
    my @list;
    while (MoreSQLData()) {
        push(@list, FetchOneColumn());
    }
    return join(',', @list);
}


my $timestamp;
my $bug_changed;

sub FindWrapPoint {
    my ($string, $startpos) = @_;
    if (!$string) { return 0 }
    if (length($string) < $startpos) { return length($string) }
    my $wrappoint = rindex($string, ",", $startpos); # look for comma
    if ($wrappoint < 0) {  # can't find comma
        $wrappoint = rindex($string, " ", $startpos); # look for space
        if ($wrappoint < 0) {  # can't find space
            $wrappoint = rindex($string, "-", $startpos); # look for hyphen
            if ($wrappoint < 0) {  # can't find hyphen
                $wrappoint = $startpos;  # just truncate it
            } else {
                $wrappoint++; # leave hyphen on the left side
            }
        }
    }
    return $wrappoint;
}

sub LogActivityEntry {
    my ($i,$col,$removed,$added) = @_;
    # in the case of CCs, deps, and keywords, there's a possibility that someone
    # might try to add or remove a lot of them at once, which might take more
    # space than the activity table allows.  We'll solve this by splitting it
    # into multiple entries if it's too long.
    while ($removed || $added) {
        my ($removestr, $addstr) = ($removed, $added);
        if (length($removestr) > 254) {
            my $commaposition = FindWrapPoint($removed, 254);
            $removestr = substr($removed,0,$commaposition);
            $removed = substr($removed,$commaposition);
            $removed =~ s/^[,\s]+//; # remove any comma or space
        } else {
            $removed = ""; # no more entries
        }
        if (length($addstr) > 254) {
            my $commaposition = FindWrapPoint($added, 254);
            $addstr = substr($added,0,$commaposition);
            $added = substr($added,$commaposition);
            $added =~ s/^[,\s]+//; # remove any comma or space
        } else {
            $added = ""; # no more entries
        }
        $addstr = SqlQuote($addstr);
        $removestr = SqlQuote($removestr);
        my $fieldid = GetFieldID($col);
        SendSQL("INSERT INTO bugs_activity " .
                "(bug_id,who,bug_when,fieldid,removed,added) VALUES " .
                "($i,$whoid," . SqlQuote($timestamp) . ",$fieldid,$removestr,$addstr)");
        $bug_changed = 1;
    }
}

sub LogDependencyActivity {
    my ($i, $oldstr, $target, $me) = (@_);
    my $newstr = SnapShotDeps($i, $target, $me);
    if ($oldstr ne $newstr) {
        # Figure out what's really different...
        my ($removed, $added) = DiffStrings($oldstr, $newstr);
        LogActivityEntry($i,$target,$removed,$added);
        # update timestamp on target bug so midairs will be triggered
        SendSQL("UPDATE bugs SET delta_ts=NOW() WHERE bug_id=$i");
        return 1;
    }
    return 0;
}

# this loop iterates once for each bug to be processed (eg when this script
# is called with multiple bugs selected from buglist.cgi instead of
# show_bug.cgi).
#
foreach my $id (@idlist) {
    my %dependencychanged;
    $bug_changed = 0;
    my $write = "WRITE";        # Might want to make a param to control
                                # whether we do LOW_PRIORITY ...
    SendSQL("LOCK TABLES bugs $write, bugs_activity $write, cc $write, " .
            "cc AS selectVisible_cc $write, " .
            "profiles $write, dependencies $write, votes $write, " .
            "products READ, components READ, " .
            "keywords $write, longdescs $write, fielddefs $write, " .
            "keyworddefs READ, groups READ, attachments READ");
    my @oldvalues = SnapShotBug($id);
    my %oldhash;
    my $i = 0;
    foreach my $col (@::log_columns) {
        # Consider NULL db entries to be equivalent to the empty string
        $oldvalues[$i] ||= '';
        $oldhash{$col} = $oldvalues[$i];
        if (exists $::FORM{$col}) {
            CheckCanChangeField($col, $id, $oldvalues[$i], $::FORM{$col});
        }
        $i++;
    }
    if ($requiremilestone) {
        my $value = $::FORM{'target_milestone'};
        if (!defined $value || $value eq $::dontchange) {
            $value = $oldhash{'target_milestone'};
        }
        SendSQL("SELECT defaultmilestone FROM products WHERE name = " .
                SqlQuote($oldhash{'product'}));
        if ($value eq FetchOneColumn()) {
            SendSQL("UNLOCK TABLES");
            $vars->{'bug_id'} = $id;
            ThrowUserError("milestone_required", "abort");
        }
    }   
    if (defined $::FORM{'delta_ts'} && $::FORM{'delta_ts'} ne $delta_ts) {
        ($vars->{'operations'}) = GetBugActivity($::FORM{'id'}, $::FORM{'delta_ts'});

        $vars->{'start_at'} = $::FORM{'longdesclength'};
        $vars->{'comments'} = GetComments($id);

        $::FORM{'delta_ts'} = $delta_ts;
        $vars->{'form'} = \%::FORM;
        
        $vars->{'bug_id'} = $id;
        $vars->{'quoteUrls'} = \&quoteUrls;
        
        SendSQL("UNLOCK TABLES");
        
        # Warn the user about the mid-air collision and ask them what to do.
        $template->process("bug/process/midair.html.tmpl", $vars)
          || ThrowTemplateError($template->error());
        exit;
    }
        
    my %deps;
    if (defined $::FORM{'dependson'}) {
        my $me = "blocked";
        my $target = "dependson";
        my %deptree;
        for (1..2) {
            $deptree{$target} = [];
            my %seen;
            foreach my $i (split('[\s,]+', $::FORM{$target})) {
                next if $i eq "";
                
                if ($id eq $i) {
                    ThrowUserError("dependency_loop_single", "abort");
                }
                if (!exists $seen{$i}) {
                    push(@{$deptree{$target}}, $i);
                    $seen{$i} = 1;
                }
            }
            # populate $deps{$target} as first-level deps only.
            # and find remainder of dependency tree in $deptree{$target}
            @{$deps{$target}} = @{$deptree{$target}};
            my @stack = @{$deps{$target}};
            while (@stack) {
                my $i = shift @stack;
                SendSQL("select $target from dependencies where $me = " .
                        SqlQuote($i));
                while (MoreSQLData()) {
                    my $t = FetchOneColumn();
                    # ignore any _current_ dependencies involving this bug,
                    # as they will be overwritten with data from the form.
                    if ($t != $id && !exists $seen{$t}) {
                        push(@{$deptree{$target}}, $t);
                        push @stack, $t;
                        $seen{$t} = 1;
                    }
                }
            }

            if ($me eq 'dependson') {
                my @deps   =  @{$deptree{'dependson'}};
                my @blocks =  @{$deptree{'blocked'}};
                my @union = ();
                my @isect = ();
                my %union = ();
                my %isect = ();
                foreach my $b (@deps, @blocks) { $union{$b}++ && $isect{$b}++ }
                @union = keys %union;
                @isect = keys %isect;
                if (@isect > 0) {
                    my $both;
                    foreach my $i (@isect) {
                       $both = $both . GetBugLink($i, "#" . $i) . " ";
                    }
                    
                    $vars->{'both'} = $both;
                    ThrowUserError("dependency_loop_multi", "abort");
                }
            }
            my $tmp = $me;
            $me = $target;
            $target = $tmp;
        }
    }

    if (@::legal_keywords) {
        # There are three kinds of "keywordsaction": makeexact, add, delete.
        # For makeexact, we delete everything, and then add our things.
        # For add, we delete things we're adding (to make sure we don't
        # end up having them twice), and then we add them.
        # For delete, we just delete things on the list.
        my $changed = 0;
        if ($keywordaction eq "makeexact") {
            SendSQL("DELETE FROM keywords WHERE bug_id = $id");
            $changed = 1;
        }
        foreach my $keyword (@keywordlist) {
            if ($keywordaction ne "makeexact") {
                SendSQL("DELETE FROM keywords
                         WHERE bug_id = $id AND keywordid = $keyword");
                $changed = 1;
            }
            if ($keywordaction ne "delete") {
                SendSQL("INSERT INTO keywords 
                         (bug_id, keywordid) VALUES ($id, $keyword)");
                $changed = 1;
            }
        }
        if ($changed) {
            SendSQL("SELECT keyworddefs.name 
                     FROM keyworddefs, keywords
                     WHERE keywords.bug_id = $id
                         AND keyworddefs.id = keywords.keywordid
                     ORDER BY keyworddefs.name");
            my @list;
            while (MoreSQLData()) {
                push(@list, FetchOneColumn());
            }
            SendSQL("UPDATE bugs SET keywords = " .
                    SqlQuote(join(', ', @list)) .
                    " WHERE bug_id = $id");
        }
    }
    my $query = "$basequery\nwhere bug_id = $id";
    
    if ($::comma ne "") {
        SendSQL($query);
    }
    SendSQL("select now()");
    $timestamp = FetchOneColumn();
    
    if (defined $::FORM{'comment'}) {
        AppendComment($id, $::COOKIE{'Bugzilla_login'}, $::FORM{'comment'});
    }
    
    my $removedCcString = "";
    if (defined $::FORM{newcc} || defined $::FORM{removecc} || defined $::FORM{masscc}) {
        # Get the current CC list for this bug
        my %oncc;
        SendSQL("SELECT who FROM cc WHERE bug_id = $id");
        while (MoreSQLData()) {
            $oncc{FetchOneColumn()} = 1;
        }

        my (@added, @removed) = ();
        foreach my $pid (keys %cc_add) {
            # If this person isn't already on the cc list, add them
            if (! $oncc{$pid}) {
                SendSQL("INSERT INTO cc (bug_id, who) VALUES ($id, $pid)");
                push (@added, $cc_add{$pid});
                $oncc{$pid} = 1;
            }
        }
        foreach my $pid (keys %cc_remove) {
            # If the person is on the cc list, remove them
            if ($oncc{$pid}) {
                SendSQL("DELETE FROM cc WHERE bug_id = $id AND who = $pid");
                push (@removed, $cc_remove{$pid});
                $oncc{$pid} = 0;
            }
        }
        # Save off the removedCcString so it can be fed to processmail
        $removedCcString = join (",", @removed);

        # If any changes were found, record it in the activity log
        if (scalar(@removed) || scalar(@added)) {
            my $removed = join(", ", @removed);
            my $added = join(", ", @added);
            LogActivityEntry($id,"cc",$removed,$added);
        }
    }

    if (defined $::FORM{'dependson'}) {
        my $me = "blocked";
        my $target = "dependson";
        for (1..2) {
            SendSQL("select $target from dependencies where $me = $id order by $target");
            my %snapshot;
            my @oldlist;
            while (MoreSQLData()) {
                push(@oldlist, FetchOneColumn());
            }
            my @newlist = sort {$a <=> $b} @{$deps{$target}};
            @dependencychanged{@oldlist} = 1;
            @dependencychanged{@newlist} = 1;

            while (0 < @oldlist || 0 < @newlist) {
                if (@oldlist == 0 || (@newlist > 0 &&
                                      $oldlist[0] > $newlist[0])) {
                    $snapshot{$newlist[0]} = SnapShotDeps($newlist[0], $me,
                                                          $target);
                    shift @newlist;
                } elsif (@newlist == 0 || (@oldlist > 0 &&
                                           $newlist[0] > $oldlist[0])) {
                    $snapshot{$oldlist[0]} = SnapShotDeps($oldlist[0], $me,
                                                          $target);
                    shift @oldlist;
                } else {
                    if ($oldlist[0] != $newlist[0]) {
                        die "Error in list comparing code";
                    }
                    shift @oldlist;
                    shift @newlist;
                }
            }
            my @keys = keys(%snapshot);
            if (@keys) {
                my $oldsnap = SnapShotDeps($id, $target, $me);
                SendSQL("delete from dependencies where $me = $id");
                foreach my $i (@{$deps{$target}}) {
                    SendSQL("insert into dependencies ($me, $target) values ($id, $i)");
                }
                foreach my $k (@keys) {
                    LogDependencyActivity($k, $snapshot{$k}, $me, $target);
                }
                LogDependencyActivity($id, $oldsnap, $target, $me);
            }

            my $tmp = $me;
            $me = $target;
            $target = $tmp;
        }
    }

    # When a bug changes products and the old or new product is associated
    # with a bug group, it may be necessary to remove the bug from the old
    # group or add it to the new one.  There are a very specific series of
    # conditions under which these activities take place, more information
    # about which can be found in comments within the conditionals below.
    if ( 
      # the "usebuggroups" parameter is on, indicating that products
      # are associated with groups of the same name;
      Param('usebuggroups')

      # the user has changed the product to which the bug belongs;
      && defined $::FORM{'product'} 
        && $::FORM{'product'} ne $::dontchange 
          && $::FORM{'product'} ne $oldhash{'product'} 
    ) {
        if (
          # the user wants to add the bug to the new product's group;
          ($::FORM{'addtonewgroup'} eq 'yes' 
            || ($::FORM{'addtonewgroup'} eq 'yesifinold' 
                  && GroupNameToBit($oldhash{'product'}) & $oldhash{'groupset'})) 

          # the new product is associated with a group;
          && GroupExists($::FORM{'product'})

          # the bug is not already in the group; (This can happen when the user
          # goes to the "edit multiple bugs" form with a list of bugs at least
          # one of which is in the new group.  In this situation, the user can
          # simultaneously change the bugs to a new product and move the bugs
          # into that product's group, which happens earlier in this script
          # and thus is already done.  If we didn't check for this, then this
          # situation would cause us to add the bug to the group twice, which
          # would result in the bug being added to a totally different group.)
          && !BugInGroup($id, $::FORM{'product'})

          # the user is a member of the associated group, indicating they
          # are authorized to add bugs to that group, *or* the "usebuggroupsentry"
          # parameter is off, indicating that users can add bugs to a product 
          # regardless of whether or not they belong to its associated group;
          && (UserInGroup($::FORM{'product'}) || !Param('usebuggroupsentry'))

          # the associated group is active, indicating it can accept new bugs;
          && GroupIsActive(GroupNameToBit($::FORM{'product'}))
        ) { 
            # Add the bug to the group associated with its new product.
            my $groupbit = GroupNameToBit($::FORM{'product'});
            SendSQL("UPDATE bugs SET groupset = groupset + $groupbit WHERE bug_id = $id");
        }

        if ( 
          # the old product is associated with a group;
          GroupExists($oldhash{'product'})

          # the bug is a member of that group;
          && BugInGroup($id, $oldhash{'product'}) 
        ) { 
            # Remove the bug from the group associated with its old product.
            my $groupbit = GroupNameToBit($oldhash{'product'});
            SendSQL("UPDATE bugs SET groupset = groupset - $groupbit WHERE bug_id = $id");
        }

    }
  
    # get a snapshot of the newly set values out of the database, 
    # and then generate any necessary bug activity entries by seeing 
    # what has changed since before we wrote out the new values.
    #
    my @newvalues = SnapShotBug($id);

    # for passing to processmail to ensure that when someone is removed
    # from one of these fields, they get notified of that fact (if desired)
    #
    my $origOwner = "";
    my $origQaContact = "";

    foreach my $c (@::log_columns) {
        my $col = $c;           # We modify it, don't want to modify array
                                # values in place.
        my $old = shift @oldvalues;
        my $new = shift @newvalues;
        if (!defined $old) {
            $old = "";
        }
        if (!defined $new) {
            $new = "";
        }
        if ($old ne $new) {

            # Products and components are now stored in the DB using ID's
            # We need to translate this to English before logging it
            if ($col eq 'product_id') {
                $old = get_product_name($old);
                $new = get_product_name($new);
                $col = 'product';
            }
            if ($col eq 'component_id') {
                $old = get_component_name($old);
                $new = get_component_name($new);
                $col = 'component';
            }

            # save off the old value for passing to processmail so the old
            # owner can be notified
            #
            if ($col eq 'assigned_to') {
                $old = ($old) ? DBID_to_name($old) : "";
                $new = ($new) ? DBID_to_name($new) : "";
                $origOwner = $old;
            }

            # ditto for the old qa contact
            #
            if ($col eq 'qa_contact') {
                $old = ($old) ? DBID_to_name($old) : "";
                $new = ($new) ? DBID_to_name($new) : "";
                $origQaContact = $old;
            }

            # If this is the keyword field, only record the changes, not everything.
            if ($col eq 'keywords') {
                ($old, $new) = DiffStrings($old, $new);
            }

            if ($col eq 'product') {
                RemoveVotes($id, 0,
                            "This bug has been moved to a different product");
            }
            LogActivityEntry($id,$col,$old,$new);
        }
    }
    if ($bug_changed) {
        SendSQL("UPDATE bugs SET delta_ts = " . SqlQuote($timestamp) . " WHERE bug_id = $id");
    }
    SendSQL("UNLOCK TABLES");

    my @ARGLIST = ();
    if ( $removedCcString ne "" ) {
        push @ARGLIST, ("-forcecc", $removedCcString);
    }
    if ( $origOwner ne "" ) {
        push @ARGLIST, ("-forceowner", $origOwner);
    }
    if ( $origQaContact ne "") { 
        push @ARGLIST, ( "-forceqacontact", $origQaContact);
    }
    push @ARGLIST, ($id, $::COOKIE{'Bugzilla_login'});
  
    # Send mail to let people know the bug has been changed.  Uses 
    # a special syntax of the "open" and "exec" commands to capture 
    # the output "processmail", which "system" doesn't allow 
    # (i.e. "system ('./processmail', $bugid , $::userid);"), without 
    # the insecurity of running the command through a shell via backticks
    # (i.e. "my $mailresults = `./processmail $bugid $::userid`;").
    $vars->{'mail'} = "";
    open(PMAIL, "-|") or exec('./processmail', @ARGLIST);
    $vars->{'mail'} .= $_ while <PMAIL>;
    close(PMAIL);

    $vars->{'id'} = $id;
    
    # Let the user know the bug was changed and who did and didn't
    # receive email about the change.
    $template->process("bug/process/results.html.tmpl", $vars)
      || ThrowTemplateError($template->error());
    
    if ($duplicate) {
        # Check to see if Reporter of this bug is reporter of Dupe 
        SendSQL("SELECT reporter FROM bugs WHERE bug_id = " . SqlQuote($::FORM{'id'}));
        my $reporter = FetchOneColumn();
        SendSQL("SELECT reporter FROM bugs WHERE bug_id = " . SqlQuote($duplicate) . " and reporter = $reporter");
        my $isreporter = FetchOneColumn();
        SendSQL("SELECT who FROM cc WHERE bug_id = " . SqlQuote($duplicate) . " and who = $reporter");
        my $isoncc = FetchOneColumn();
        unless ($isreporter || $isoncc || ! $::FORM{'confirm_add_duplicate'}) {
            # The reporter is oblivious to the existance of the new bug and is permitted access
            # ... add 'em to the cc (and record activity)
            LogActivityEntry($duplicate,"cc","",DBID_to_name($reporter));
            SendSQL("INSERT INTO cc (who, bug_id) VALUES ($reporter, " . SqlQuote($duplicate) . ")");
        }
        AppendComment($duplicate, $::COOKIE{'Bugzilla_login'}, "*** Bug $::FORM{'id'} has been marked as a duplicate of this bug. ***");
        CheckFormFieldDefined(\%::FORM,'comment');
        SendSQL("INSERT INTO duplicates VALUES ($duplicate, $::FORM{'id'})");
        
        $vars->{'mail'} = "";
        open(PMAIL, "-|") or exec('./processmail', $duplicate, $::COOKIE{'Bugzilla_login'});
        $vars->{'mail'} .= $_ while <PMAIL>;
        close(PMAIL);
        
        $vars->{'id'} = $duplicate;
        $vars->{'type'} = "dupe";
        
        # Let the user know a duplication notation was added to the original bug.
        $template->process("bug/process/results.html.tmpl", $vars)
          || ThrowTemplateError($template->error());
    }

    foreach my $k (keys(%dependencychanged)) {
        $vars->{'mail'} = "";
        open(PMAIL, "-|") or exec('./processmail', $k, $::COOKIE{'Bugzilla_login'});
        $vars->{'mail'} .= $_ while <PMAIL>;
        close(PMAIL);
        
        $vars->{'id'} = $k;
        $vars->{'type'} = "dep";
        
        # Let the user know we checked to see if we should email notice
        # of this change to users with a relationship to the dependent
        # bug and who did and didn't receive email about it.
        $template->process("bug/process/results.html.tmpl", $vars)
          || ThrowTemplateError($template->error());
    }

}

# Show next bug, if it exists.
if ($::COOKIE{"BUGLIST"} && $::FORM{'id'}) {
    my @bugs = split(/:/, $::COOKIE{"BUGLIST"});
    $vars->{'bug_list'} = \@bugs;
    my $cur = lsearch(\@bugs, $::FORM{"id"});
    if ($cur >= 0 && $cur < $#bugs) {
        my $next_bug = $bugs[$cur + 1];
        if (detaint_natural($next_bug) && CanSeeBug($next_bug)) {
            $::FORM{'id'} = $next_bug;
            
            $vars->{'next_id'} = $next_bug;
            
            # Let the user know we are about to display the next bug in their list.
            $template->process("bug/process/next.html.tmpl", $vars)
              || ThrowTemplateError($template->error());

            show_bug("header is already done");

            exit;
        }
    }
}

# End the response page.
$template->process("bug/navigate.html.tmpl", $vars)
  || ThrowTemplateError($template->error());
$template->process("global/footer.html.tmpl", $vars)
  || ThrowTemplateError($template->error());
