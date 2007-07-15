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
# Contributor(s): Terry Weissman <terry@mozilla.org>
#                 Dan Mosedale <dmose@mozilla.org>
#                 Dave Miller <justdave@syndicomm.com>
#                 Christopher Aillon <christopher@aillon.com>
#                 Myk Melez <myk@mozilla.org>
#                 Jeff Hedlund <jeff.hedlund@matrixsi.com>
#                 Frédéric Buclin <LpSolit@gmail.com>
#                 Lance Larsh <lance.larsh@oracle.com>
#                 Akamai Technologies <bugzilla-dev@akamai.com>
#                 Max Kanat-Alexander <mkanat@bugzilla.org>

# Implementation notes for this file:
#
# 1) the 'id' form parameter is validated early on, and if it is not a valid
# bugid an error will be reported, so it is OK for later code to simply check
# for a defined form 'id' value, and it can assume a valid bugid.
#
# 2) If the 'id' form parameter is not defined (after the initial validation),
# then we are processing multiple bugs, and @idlist will contain the ids.
#
# 3) If we are processing just the one id, then it is stored in @idlist for
# later processing.

use strict;

use lib qw(.);

use Bugzilla;
use Bugzilla::Constants;
use Bugzilla::Bug;
use Bugzilla::BugMail;
use Bugzilla::Mailer;
use Bugzilla::User;
use Bugzilla::Util;
use Bugzilla::Error;
use Bugzilla::Field;
use Bugzilla::Product;
use Bugzilla::Component;
use Bugzilla::Keyword;
use Bugzilla::Flag;

use Storable qw(dclone);

my $user = Bugzilla->login(LOGIN_REQUIRED);
local our $whoid = $user->id;
my $grouplist = $user->groups_as_string;

my $cgi = Bugzilla->cgi;
my $dbh = Bugzilla->dbh;
my $template = Bugzilla->template;
local our $vars = {};
$vars->{'valid_keywords'} = [map($_->name, Bugzilla::Keyword->get_all)];
$vars->{'use_keywords'} = 1 if Bugzilla::Keyword::keyword_count();

my @editable_bug_fields = editable_bug_fields();

my $requiremilestone = 0;
local our $PrivilegesRequired = 0;

######################################################################
# Subroutines
######################################################################

# Used to send email when an update is done.
sub send_results {
    my ($bug_id, $vars) = @_;
    my $template = Bugzilla->template;
    if (Bugzilla->usage_mode == USAGE_MODE_EMAIL) {
         Bugzilla::BugMail::Send($bug_id, $vars->{'mailrecipients'});
    }
    else {
        $template->process("bug/process/results.html.tmpl", $vars)
            || ThrowTemplateError($template->error());
    }
    $vars->{'header_done'} = 1;
}

sub comment_exists {
    my $cgi = Bugzilla->cgi;
    return ($cgi->param('comment') && $cgi->param('comment') =~ /\S+/) ? 1 : 0;
}

######################################################################
# Begin Data/Security Validation
######################################################################

# Create a list of IDs of all bugs being modified in this request.
# This list will either consist of a single bug number from the "id"
# form/URL field or a series of numbers from multiple form/URL fields
# named "id_x" where "x" is the bug number.
# For each bug being modified, make sure its ID is a valid bug number 
# representing an existing bug that the user is authorized to access.
my (@idlist, @bug_objects);
if (defined $cgi->param('id')) {
  my $id = $cgi->param('id');
  ValidateBugID($id);

  # Store the validated, and detainted id back in the cgi data, as
  # lots of later code will need it, and will obtain it from there
  $cgi->param('id', $id);
  push @idlist, $id;
  push(@bug_objects, new Bugzilla::Bug($id));
} else {
    foreach my $i ($cgi->param()) {
        if ($i =~ /^id_([1-9][0-9]*)/) {
            my $id = $1;
            ValidateBugID($id);
            push @idlist, $id;
            # We do this until we have Bugzilla::Bug->new_from_list.
            push(@bug_objects, new Bugzilla::Bug($id));
        }
    }
}

# Make sure there are bugs to process.
scalar(@idlist) || ThrowUserError("no_bugs_chosen");

# Build a bug object using the first bug id, for validations.
my $bug = $bug_objects[0];

# Make sure form param 'dontchange' is defined so it can be compared to easily.
$cgi->param('dontchange','') unless defined $cgi->param('dontchange');

# Make sure the 'knob' param is defined; else set it to 'none'.
$cgi->param('knob', 'none') unless defined $cgi->param('knob');

# Validate all timetracking fields
foreach my $field ("estimated_time", "work_time", "remaining_time") {
    if (defined $cgi->param($field) 
        && $cgi->param($field) ne $cgi->param('dontchange')) 
    {
        $cgi->param($field, $bug->_check_time($cgi->param($field), $field));
    }
}

if (Bugzilla->user->in_group(Bugzilla->params->{'timetrackinggroup'})) {
    my $wk_time = $cgi->param('work_time');
    if ($cgi->param('comment') =~ /^\s*$/ && $wk_time && $wk_time != 0) {
        ThrowUserError('comment_required');
    }
}

$cgi->param('comment', $bug->_check_comment($cgi->param('comment')));

# If the bug(s) being modified have dependencies, validate them
# and rebuild the list with the validated values.  This is important
# because there are situations where validation changes the value
# instead of throwing an error, f.e. when one or more of the values
# is a bug alias that gets converted to its corresponding bug ID
# during validation.
if ($cgi->param('id') && (defined $cgi->param('dependson')
                          || defined $cgi->param('blocked')) )
{
    $bug->set_dependencies(scalar $cgi->param('dependson'),
                           scalar $cgi->param('blocked'));
}
# Right now, you can't modify dependencies on a mass change.
else {
    $cgi->delete('dependson');
    $cgi->delete('blocked');
}

# do a match on the fields if applicable

# The order of these function calls is important, as Flag::validate
# assumes User::match_field has ensured that the values
# in the requestee fields are legitimate user email addresses.
&Bugzilla::User::match_field($cgi, {
    'qa_contact'                => { 'type' => 'single' },
    'newcc'                     => { 'type' => 'multi'  },
    'masscc'                    => { 'type' => 'multi'  },
    'assigned_to'               => { 'type' => 'single' },
    '^requestee(_type)?-(\d+)$' => { 'type' => 'multi'  },
});

# Validate flags in all cases. validate() should not detect any
# reference to flags if $cgi->param('id') is undefined.
Bugzilla::Flag::validate($cgi, $cgi->param('id'));

######################################################################
# End Data/Security Validation
######################################################################

print $cgi->header() unless Bugzilla->usage_mode == USAGE_MODE_EMAIL;
$vars->{'title_tag'} = "bug_processed";

# Set the title if we can see a mid-air coming. This test may have false
# negatives, but never false positives, and should catch the majority of cases.
# It only works at all in the single bug case.
if (defined $cgi->param('id')) {
    if (defined $cgi->param('delta_ts')
        && $cgi->param('delta_ts') ne $bug->delta_ts)
    {
        $vars->{'title_tag'} = "mid_air";
        ThrowCodeError('undefined_field', {field => 'longdesclength'})
          if !defined $cgi->param('longdesclength');
    }
}

# Set up the vars for navigational <link> elements
my @bug_list;
if ($cgi->cookie("BUGLIST") && defined $cgi->param('id')) {
    @bug_list = split(/:/, $cgi->cookie("BUGLIST"));
    $vars->{'bug_list'} = \@bug_list;
}

# Figure out whether or not the user is trying to change the product
# (either the "product" variable is not set to "don't change" or the
# user is changing a single bug and has changed the bug's product),
# and make the user verify the version, component, target milestone,
# and bug groups if so.
# At this point, the product must be defined, even if set to "dontchange".
defined($cgi->param('product'))
  || ThrowCodeError('undefined_field', { field => 'product' });

my $product_change = 0;
if ((defined $cgi->param('id') && $cgi->param('product') ne $bug->product)
     || (!$cgi->param('id')
         && $cgi->param('product') ne $cgi->param('dontchange')))
{
    if (Bugzilla->params->{'commentonreassignbycomponent'} && !comment_exists()) {
        ThrowUserError('comment_required');
    }
    # Check to make sure they actually have the right to change the product
    my $oldproduct = (defined $cgi->param('id')) ? $bug->product : '';
    if (!$bug->check_can_change_field('product', $oldproduct, $cgi->param('product'),
                                      \$PrivilegesRequired))
    {
        $vars->{'oldvalue'} = $oldproduct;
        $vars->{'newvalue'} = $cgi->param('product');
        $vars->{'field'} = 'product';
        $vars->{'privs'} = $PrivilegesRequired;
        ThrowUserError("illegal_change", $vars);
    }

    my $product_name = $cgi->param('product');
    my $product = new Bugzilla::Product({name => $product_name});

    # If at least one bug does not belong to the product we are
    # moving to, we have to check whether or not the user is
    # allowed to enter bugs into that product.
    # Note that this check must be done early to avoid the leakage
    # of component, version and target milestone names.
    my $check_can_enter = 1;
    if ($product) {
        $check_can_enter =
          $dbh->selectrow_array("SELECT 1 FROM bugs
                                 WHERE product_id != ?
                                 AND bugs.bug_id IN
                                 (" . join(',', @idlist) . ") " .
                                 $dbh->sql_limit(1),
                                 undef, $product->id);
    }
    if ($check_can_enter) { $user->can_enter_product($product_name, 1) }

    # note that when this script is called from buglist.cgi (rather
    # than show_bug.cgi), it's possible that the product will be changed
    # but that the version and/or component will be set to 
    # "--dont_change--" but still happen to be correct.  in this case,
    # the if statement will incorrectly trigger anyway.  this is a 
    # pretty weird case, and not terribly unreasonable behavior, but 
    # worthy of a comment, perhaps.
    #
    my @version_names = map($_->name, @{$product->versions});
    my @component_names = map($_->name, @{$product->components});
    my $vok = 0;
    if (defined $cgi->param('version')) {
        $vok = lsearch(\@version_names, $cgi->param('version')) >= 0;
    }
    my $cok = 0;
    if (defined $cgi->param('component')) {
        $cok = lsearch(\@component_names, $cgi->param('component')) >= 0;
    }

    my $mok = 1;   # so it won't affect the 'if' statement if milestones aren't used
    my @milestone_names = ();
    if ( Bugzilla->params->{"usetargetmilestone"} ) {
       @milestone_names = map($_->name, @{$product->milestones});
       $mok = 0;
       if (defined $cgi->param('target_milestone')) {
           $mok = lsearch(\@milestone_names, $cgi->param('target_milestone')) >= 0;
       }
    }

    # We cannot be sure if the component is the same by only checking $cok; the
    # current component name could exist in the new product. So always display
    # the form and use the confirm_product_change param to check if that was
    # shown. Also show the verification form if the product-specific fields
    # somehow still need to be verified, or if we need to verify whether or not
    # to add the bugs to their new product's group.
    if (!$vok || !$cok || !$mok || !defined $cgi->param('confirm_product_change')) {

        if (Bugzilla->usage_mode == USAGE_MODE_EMAIL) {
            if (!$vok) {
                ThrowUserError('version_not_valid', {
                    version => $cgi->param('version'),
                    product => $product->name});
            }
            if (!$cok) {
                ThrowUserError('component_not_valid', {
                    product => $product->name,
                    name    => $cgi->param('component')});
            }
            if (!$mok) {
                ThrowUserError('milestone_not_valid', {
                    product   => $product->name,
                    milestone => $cgi->param('target_milestone')});
            }
        }

        $vars->{'product'} = $product;
        my %defaults;
        # We set the defaults to these fields to the old value,
        # if it's a valid option, otherwise we use the default where
        # that's appropriate
        $vars->{'versions'} = \@version_names;
        if ($vok) {
            $defaults{'version'} = $cgi->param('version');
        }
        elsif (scalar(@version_names) == 1) {
            $defaults{'version'} = $version_names[0];
        }

        $vars->{'components'} = \@component_names;
        if ($cok) {
            $defaults{'component'} = $cgi->param('component');
        }
        elsif (scalar(@component_names) == 1) {
            $defaults{'component'} = $component_names[0];
        }

        if (Bugzilla->params->{"usetargetmilestone"}) {
            $vars->{'milestones'} = \@milestone_names;
            if ($mok) {
                $defaults{'target_milestone'} = $cgi->param('target_milestone');
            } else {
                $defaults{'target_milestone'} = $product->default_milestone;
            }
        }
        $vars->{'defaults'} = \%defaults;

        # Get the ID of groups which are no longer valid in the new product.
        my $gids =
          $dbh->selectcol_arrayref('SELECT bgm.group_id
                                      FROM bug_group_map AS bgm
                                     WHERE bgm.bug_id IN (' . join(', ', @idlist) . ')
                                       AND bgm.group_id NOT IN
                                           (SELECT gcm.group_id
                                              FROM group_control_map AS gcm
                                             WHERE gcm.product_id = ?
                                               AND ((gcm.membercontrol != ?
                                                    AND gcm.group_id IN (' . $grouplist . '))
                                                    OR gcm.othercontrol != ?))',
                                     undef, ($product->id, CONTROLMAPNA, CONTROLMAPNA));
        $vars->{'old_groups'} = Bugzilla::Group->new_from_list($gids);

        $template->process("bug/process/verify-new-product.html.tmpl", $vars)
          || ThrowTemplateError($template->error());
        exit;
    }
    $product_change = 1;
}

# At this point, the component must be defined, even if set to "dontchange".
defined($cgi->param('component'))
  || ThrowCodeError('undefined_field', { field => 'component' });

# Confirm that the reporter of the current bug can access the bug we are duping to.
sub DuplicateUserConfirm {
    my ($dupe, $original) = @_;
    my $cgi = Bugzilla->cgi;
    my $dbh = Bugzilla->dbh;
    my $template = Bugzilla->template;

    # if we've already been through here, then exit
    if (defined $cgi->param('confirm_add_duplicate')) {
        return;
    }

    if ($dupe->reporter->can_see_bug($original)) {
        $cgi->param('confirm_add_duplicate', '1');
        return;
    }
    elsif (Bugzilla->usage_mode == USAGE_MODE_EMAIL) {
        # The email interface defaults to the safe alternative, which is
        # not CC'ing the user.
        $cgi->param('confirm_add_duplicate', 0);
        return;
    }

    $vars->{'cclist_accessible'} = $dbh->selectrow_array(
        q{SELECT cclist_accessible FROM bugs WHERE bug_id = ?},
        undef, $original);
    
    # Once in this part of the subroutine, the user has not been auto-validated
    # and the duper has not chosen whether or not to add to CC list, so let's
    # ask the duper what he/she wants to do.
    
    $vars->{'original_bug_id'} = $original;
    $vars->{'duplicate_bug_id'} = $dupe->bug_id;
    
    # Confirm whether or not to add the reporter to the cc: list
    # of the original bug (the one this bug is being duped against).
    print Bugzilla->cgi->header();
    $template->process("bug/process/confirm-duplicate.html.tmpl", $vars)
      || ThrowTemplateError($template->error());
    exit;
}

if (defined $cgi->param('id')) {
    # since this means that we were called from show_bug.cgi, now is a good
    # time to do a whole bunch of error checking that can't easily happen when
    # we've been called from buglist.cgi, because buglist.cgi only tweaks
    # values that have been changed instead of submitting all the new values.
    # (XXX those error checks need to happen too, but implementing them 
    # is more work in the current architecture of this script...)

    my $prod = $bug->_check_product($cgi->param('product'));
    $cgi->param('product', $prod->name);

    my $comp = $bug->_check_component($prod, 
                                      scalar $cgi->param('component'));
    $cgi->param('component', $comp->name);

    $cgi->param('version', $bug->_check_version($prod,
                                                scalar $cgi->param('version')));
    $cgi->param('target_milestone', $bug->_check_target_milestone($prod,
        scalar $cgi->param('target_milestone')));
}

my %methods = (
    bug_severity => 'set_severity',
    rep_platform => 'set_platform',
    short_desc   => 'set_summary',
    bug_file_loc => 'set_url',
);
foreach my $b (@bug_objects) {
    foreach my $field_name (qw(op_sys rep_platform priority bug_severity
                               bug_file_loc status_whiteboard short_desc))
    {
        # We only update the field if it's defined and it's not set
        # to dontchange.
        if ( defined $cgi->param($field_name)
             && (!$cgi->param('dontchange')
                 || $cgi->param($field_name) ne $cgi->param('dontchange')) )
        {
            my $method = $methods{$field_name};
            $method ||= "set_" . $field_name;
            $b->$method($cgi->param($field_name));
        }
    }
}

my $action = trim($cgi->param('action') || '');

if ($action eq Bugzilla->params->{'move-button-text'}) {
    Bugzilla->params->{'move-enabled'} || ThrowUserError("move_bugs_disabled");

    $user->is_mover || ThrowUserError("auth_failure", {action => 'move',
                                                       object => 'bugs'});

    $dbh->bz_lock_tables('bugs WRITE', 'bugs_activity WRITE', 'duplicates WRITE',
                         'longdescs WRITE', 'profiles READ', 'groups READ',
                         'bug_group_map READ', 'group_group_map READ',
                         'user_group_map READ', 'classifications READ',
                         'products READ', 'components READ', 'votes READ',
                         'cc READ', 'fielddefs READ', 'bug_status READ',
                         'status_workflow READ', 'resolution READ');

    # First update all moved bugs.
    foreach my $bug (@bug_objects) {
        $bug->add_comment(scalar $cgi->param('comment'),
                          { type => CMT_MOVED_TO, extra_data => $user->login });
    }
    # Don't export the new status and resolution. We want the current ones.
    local $Storable::forgive_me = 1;
    my $bugs = dclone(\@bug_objects);
    foreach my $bug (@bug_objects) {
        my ($status, $resolution) = $bug->get_new_status_and_resolution('move');
        $bug->set_status($status);
        $bug->set_resolution($resolution);
    }
    $_->update() foreach @bug_objects;
    $dbh->bz_unlock_tables();

    # Now send emails.
    foreach my $id (@idlist) {
        $vars->{'mailrecipients'} = { 'changer' => $user->login };
        $vars->{'id'} = $id;
        $vars->{'type'} = "move";
        send_results($id, $vars);
    }
    # Prepare and send all data about these bugs to the new database
    my $to = Bugzilla->params->{'move-to-address'};
    $to =~ s/@/\@/;
    my $from = Bugzilla->params->{'moved-from-address'};
    $from =~ s/@/\@/;
    my $msg = "To: $to\n";
    $msg .= "From: Bugzilla <" . $from . ">\n";
    $msg .= "Subject: Moving bug(s) " . join(', ', @idlist) . "\n\n";

    my @fieldlist = (Bugzilla::Bug->fields, 'group', 'long_desc',
                     'attachment', 'attachmentdata');
    my %displayfields;
    foreach (@fieldlist) {
        $displayfields{$_} = 1;
    }

    $template->process("bug/show.xml.tmpl", { bugs => $bugs,
                                              displayfields => \%displayfields,
                                            }, \$msg)
      || ThrowTemplateError($template->error());

    $msg .= "\n";
    MessageToMTA($msg);

    # End the response page.
    unless (Bugzilla->usage_mode == USAGE_MODE_EMAIL) {
        $template->process("bug/navigate.html.tmpl", $vars)
            || ThrowTemplateError($template->error());
        $template->process("global/footer.html.tmpl", $vars)
            || ThrowTemplateError($template->error());
    }
    exit;
}


$::query = "UPDATE bugs SET";
$::comma = "";
local our @values;
umask(0);

sub _remove_remaining_time {
    my $cgi = Bugzilla->cgi;
    if (Bugzilla->user->in_group(Bugzilla->params->{'timetrackinggroup'})) {
        if ( defined $cgi->param('remaining_time') 
             && $cgi->param('remaining_time') > 0 )
        {
            $cgi->param('remaining_time', 0);
            $vars->{'message'} = "remaining_time_zeroed";
        }
    }
    else {
        DoComma();
        $::query .= "remaining_time = 0";
    }
}

sub DoComma {
    $::query .= "$::comma\n    ";
    $::comma = ",";
}

foreach my $field ("version", "target_milestone") {
    if (defined $cgi->param($field)) {
        if (!$cgi->param('dontchange')
            || $cgi->param($field) ne $cgi->param('dontchange')) {
            DoComma();
            $::query .= "$field = ?";
            my $value = trim($cgi->param($field));
            trick_taint($value);
            push(@values, $value);
        }
    }
}

# Add custom fields data to the query that will update the database.
foreach my $field (Bugzilla->get_fields({custom => 1, obsolete => 0})) {
    my $fname = $field->name;
    if (defined $cgi->param($fname)
        && (!$cgi->param('dontchange')
            || $cgi->param($fname) ne $cgi->param('dontchange')))
    {
        $_->set_custom_field($field, $cgi->param($fname)) foreach @bug_objects;
    }
}

my $product;
my $prod_changed = 0;
my @newprod_ids;
if ($cgi->param('product') ne $cgi->param('dontchange')) {
    $product = Bugzilla::Product::check_product(scalar $cgi->param('product'));

    DoComma();
    $::query .= "product_id = ?";
    push(@values, $product->id);
    @newprod_ids = ($product->id);
    # If the bug remains in the same product, leave $prod_changed set to 0.
    # Even with 'strict_isolation' turned on, we ignore users who already
    # play a role for the bug; else you would never be able to edit it.
    # If you want to move the bug to another product, then you first have to
    # remove these users from the bug.
    unless (defined $cgi->param('id') && $bug->product_id == $product->id) {
        $prod_changed = 1;
    }
} else {
    @newprod_ids = @{$dbh->selectcol_arrayref("SELECT DISTINCT product_id
                                               FROM bugs 
                                               WHERE bug_id IN (" .
                                                   join(',', @idlist) . 
                                               ")")};
    if (scalar(@newprod_ids) == 1) {
        $product = new Bugzilla::Product($newprod_ids[0]);
    }
}

my $component;
my (@cc_add, @cc_remove);

if ($cgi->param('component') ne $cgi->param('dontchange')) {
    if (scalar(@newprod_ids) > 1) {
        ThrowUserError("no_component_change_for_multiple_products");
    }
    $component =
        Bugzilla::Component::check_component($product, scalar $cgi->param('component'));

    # This parameter is required later when checking fields the user can change.
    $cgi->param('component_id', $component->id);
    DoComma();
    $::query .= "component_id = ?";
    push(@values, $component->id);

    # Add in the default CC list for the component if we are moving bugs.
    if (!$cgi->param('id') || $component->id != $bug->component_id) {
        foreach my $cc (@{$component->initial_cc}) {
            # NewCC must be defined or the code below won't insert
            # any CCs.
            $cgi->param('newcc') || $cgi->param('newcc', "");
            push(@cc_add, $cc->login);
        }
    }
}

# Certain changes can only happen on individual bugs, never on mass-changes.
if (defined $cgi->param('id')) {
    my $bug = $bug_objects[0];
    
    # Since aliases are unique (like bug numbers), they can only be changed
    # for one bug at a time.
    if (Bugzilla->params->{"usebugaliases"} && defined $cgi->param('alias')) {
        $bug->set_alias($cgi->param('alias'));
    }

    # reporter_accessible and cclist_accessible--these are only set if
    # the user can change them and there are groups on the bug.
    # (If the user can't change the field, the checkboxes don't appear
    #  on show_bug, thus it would look like the user was trying to
    #  uncheck them, which would then be denied by the set_ functions,
    #  throwing a confusing error.)
    if (scalar @{$bug->groups_in}) {
        $bug->set_cclist_accessible($cgi->param('cclist_accessible'))
            if $bug->check_can_change_field('cclist_accessible', 0, 1);
        $bug->set_reporter_accessible($cgi->param('reporter_accessible'))
            if $bug->check_can_change_field('reporter_accessible', 0, 1);
    }
}

if ( defined $cgi->param('id') &&
     (Bugzilla->params->{"insidergroup"} 
      && Bugzilla->user->in_group(Bugzilla->params->{"insidergroup"})) ) 
{

    my $sth = $dbh->prepare('UPDATE longdescs SET isprivate = ?
                             WHERE bug_id = ? AND bug_when = ?');

    foreach my $field ($cgi->param()) {
        if ($field =~ /when-([0-9]+)/) {
            my $sequence = $1;
            my $private = $cgi->param("isprivate-$sequence") ? 1 : 0 ;
            if ($private != $cgi->param("oisprivate-$sequence")) {
                my $field_data = $cgi->param("$field");
                # Make sure a valid date is given.
                $field_data = format_time($field_data, '%Y-%m-%d %T');
                $sth->execute($private, $cgi->param('id'), $field_data);
            }
        }

    }
}

# We need to check the addresses involved in a CC change before we touch 
# any bugs. What we'll do here is formulate the CC data into two arrays of
# users involved in this CC change.  Then those arrays can be used later 
# on for the actual change.
if (defined $cgi->param('newcc')
    || defined $cgi->param('addselfcc')
    || defined $cgi->param('removecc')
    || defined $cgi->param('masscc')) {
        
    # If masscc is defined, then we came from buglist and need to either add or
    # remove cc's... otherwise, we came from bugform and may need to do both.
    my ($cc_add, $cc_remove) = "";
    if (defined $cgi->param('masscc')) {
        if ($cgi->param('ccaction') eq 'add') {
            $cc_add = join(' ',$cgi->param('masscc'));
        } elsif ($cgi->param('ccaction') eq 'remove') {
            $cc_remove = join(' ',$cgi->param('masscc'));
        }
    } else {
        $cc_add = join(' ',$cgi->param('newcc'));
        # We came from bug_form which uses a select box to determine what cc's
        # need to be removed...
        if (defined $cgi->param('removecc') && $cgi->param('cc')) {
            $cc_remove = join (",", $cgi->param('cc'));
        }
    }

    push(@cc_add, split(/[\s,]+/, $cc_add)) if $cc_add;
    push(@cc_add, Bugzilla->user) if $cgi->param('addselfcc');

    push(@cc_remove, split(/[\s,]+/, $cc_remove)) if $cc_remove;
}

foreach my $b (@bug_objects) {
    $b->remove_cc($_) foreach @cc_remove;
    $b->add_cc($_, $product) foreach @cc_add;
}

# Store the new assignee and QA contact IDs (if any). This is the
# only way to keep these informations when bugs are reassigned by
# component as $cgi->param('assigned_to') and $cgi->param('qa_contact')
# are not the right fields to look at.
# If the assignee or qacontact is changed, the new one is checked when
# changed information is validated.  If not, then the unchanged assignee
# or qacontact may have to be validated later.

my $assignee;
my $qacontact;
my $qacontact_checked = 0;
my $assignee_checked = 0;

my %usercache = ();

if (defined $cgi->param('assigned_to')
    && !$cgi->param('set_default_assignee')
    && trim($cgi->param('assigned_to')) ne $cgi->param('dontchange'))
{
    my $name = trim($cgi->param('assigned_to'));
    if ($name ne "") {
        $assignee = login_to_id($name, THROW_ERROR);
        if (Bugzilla->params->{"strict_isolation"}) {
            $usercache{$assignee} ||= Bugzilla::User->new($assignee);
            my $assign_user = $usercache{$assignee};
            foreach my $product_id (@newprod_ids) {
                if (!$assign_user->can_edit_product($product_id)) {
                    my $product_name = Bugzilla::Product->new($product_id)->name;
                    ThrowUserError('invalid_user_group',
                                      {'users'   => $assign_user->login,
                                       'product' => $product_name,
                                       'bug_id' => (scalar(@idlist) > 1)
                                                     ? undef : $idlist[0]
                                      });
                }
            }
        }
    } else {
        ThrowUserError("reassign_to_empty");
    }
    DoComma();
    $::query .= "assigned_to = ?";
    push(@values, $assignee);
    $assignee_checked = 1;
};

if (defined $cgi->param('qa_contact') && !$cgi->param('set_default_qa_contact')) {
    my $name = trim($cgi->param('qa_contact'));
    # The QA contact cannot be deleted from show_bug.cgi for a single bug!
    if ($name ne $cgi->param('dontchange')) {
        $qacontact = login_to_id($name, THROW_ERROR) if ($name ne "");
        if ($qacontact && Bugzilla->params->{"strict_isolation"}
            && !(defined $cgi->param('id') && $bug->qa_contact
                 && $qacontact == $bug->qa_contact->id))
        {
                $usercache{$qacontact} ||= Bugzilla::User->new($qacontact);
                my $qa_user = $usercache{$qacontact};
                foreach my $product_id (@newprod_ids) {
                    if (!$qa_user->can_edit_product($product_id)) {
                        my $product_name = Bugzilla::Product->new($product_id)->name;
                        ThrowUserError('invalid_user_group',
                                          {'users'   => $qa_user->login,
                                           'product' => $product_name,
                                           'bug_id' => (scalar(@idlist) > 1)
                                                         ? undef : $idlist[0]
                                          });
                    }
                }
        }
        $qacontact_checked = 1;
        DoComma();
        if($qacontact) {
            $::query .= "qa_contact = ?";
            push(@values, $qacontact);
        }
        else {
            $::query .= "qa_contact = NULL";
        }
    }
}

if (($cgi->param('set_default_assignee') || $cgi->param('set_default_qa_contact'))
    && Bugzilla->params->{'commentonreassignbycomponent'} && !comment_exists())
{
        ThrowUserError('comment_required');
}

my $duplicate; # It will store the ID of the bug we are pointing to, if any.

# Make sure the bug status transition is legal for all bugs.
my $knob = scalar $cgi->param('knob');
# Special actions (duplicate, change_resolution and clearresolution) are outside
# the workflow.
if (!grep { $knob eq $_ } SPECIAL_STATUS_WORKFLOW_ACTIONS) {
    Bugzilla::Bug->check_status_transition($knob, \@idlist);
    my $bug_status = new Bugzilla::Status({name => $knob});
    # Fill the resolution field with the correct value (e.g. in case the
    # workflow allows several open -> closed transitions).
    if ($bug_status->is_open) {
        $cgi->delete('resolution');
    }
    else {
        $cgi->param('resolution', $cgi->param('resolution_knob_' . $bug_status->id));
    }
}
elsif ($knob eq 'change_resolution') {
    # Fill the resolution field with the correct value.
    $cgi->param('resolution', $cgi->param('resolution_knob_change_resolution'));
}
else {
    # The resolution field is not in use.
    $cgi->delete('resolution');
}

# The action is a valid one.
trick_taint($knob);
# Some information is required for checks.
$vars->{comment_exists} = comment_exists();
$vars->{bug_id} = $cgi->param('id');
$vars->{dup_id} = $cgi->param('dup_id');
$vars->{resolution} = $cgi->param('resolution') || '';
Bugzilla::Bug->check_status_change_triggers($knob, \@idlist, $vars);

# Some triggers require extra actions.
$duplicate = $vars->{dup_id} if ($knob eq 'duplicate');
$requiremilestone = $vars->{requiremilestone};
# $vars->{DuplicateUserConfirm} is true only if a single bug is being edited.
DuplicateUserConfirm($bug, $duplicate) if $vars->{DuplicateUserConfirm};
_remove_remaining_time() if $vars->{remove_remaining_time};

my $any_keyword_changes;
if (defined $cgi->param('keywords')) {
    foreach my $b (@bug_objects) {
        my $return =
            $b->modify_keywords(scalar $cgi->param('keywords'),
                                scalar $cgi->param('keywordaction'));
        $any_keyword_changes ||= $return;
    }
}

if ($::comma eq ""
    && !$any_keyword_changes
    && defined $cgi->param('masscc') && ! $cgi->param('masscc')
    ) {
    if (!defined $cgi->param('comment') || $cgi->param('comment') =~ /^\s*$/) {
        ThrowUserError("bugs_not_changed");
    }
}

# Process data for Time Tracking fields
if (Bugzilla->user->in_group(Bugzilla->params->{'timetrackinggroup'})) {
    foreach my $field ("estimated_time", "remaining_time") {
        if (defined $cgi->param($field)) {
            my $er_time = trim($cgi->param($field));
            if ($er_time ne $cgi->param('dontchange')) {
                DoComma();
                $::query .= "$field = ?";
                trick_taint($er_time);
                push(@values, $er_time);
            }
        }
    }

    if (defined $cgi->param('deadline')) {
        DoComma();
        if ($cgi->param('deadline')) {
            validate_date($cgi->param('deadline'))
              || ThrowUserError('illegal_date', {date => $cgi->param('deadline'),
                                                 format => 'YYYY-MM-DD'});
            $::query .= "deadline = ?";
            my $deadline = $cgi->param('deadline');
            trick_taint($deadline);
            push(@values, $deadline);
        } else {
            $::query .= "deadline = NULL";
        }
    }
}

my $basequery = $::query;

local our $delta_ts;
sub SnapShotBug {
    my ($id) = (@_);
    my $dbh = Bugzilla->dbh;
    my @row = $dbh->selectrow_array(q{SELECT delta_ts, } .
                join(',', editable_bug_fields()).q{ FROM bugs WHERE bug_id = ?},
                undef, $id);
    $delta_ts = shift @row;

    return @row;
}

my $timestamp;

if ($prod_changed && Bugzilla->params->{"strict_isolation"}) {
    my $sth_cc = $dbh->prepare("SELECT who
                                FROM cc
                                WHERE bug_id = ?");
    my $sth_bug = $dbh->prepare("SELECT assigned_to, qa_contact
                                 FROM bugs
                                 WHERE bug_id = ?");

    foreach my $id (@idlist) {
        $sth_cc->execute($id);
        my @blocked_cc = ();
        while (my ($pid) = $sth_cc->fetchrow_array) {
            # Ignore deleted accounts. They will never get notification.
            $usercache{$pid} ||= Bugzilla::User->new($pid) || next;
            my $cc_user = $usercache{$pid};
            if (!$cc_user->can_edit_product($product->id)) {
                push (@blocked_cc, $cc_user->login);
            }
        }
        if (scalar(@blocked_cc)) {
            ThrowUserError('invalid_user_group',
                              {'users'   => \@blocked_cc,
                               'bug_id' => $id,
                               'product' => $product->name});
        }
        $sth_bug->execute($id);
        my ($assignee, $qacontact) = $sth_bug->fetchrow_array;
        if (!$assignee_checked) {
            $usercache{$assignee} ||= Bugzilla::User->new($assignee) || next;
            my $assign_user = $usercache{$assignee};
            if (!$assign_user->can_edit_product($product->id)) {
                    ThrowUserError('invalid_user_group',
                                      {'users'   => $assign_user->login,
                                       'bug_id' => $id,
                                       'product' => $product->name});
            }
        }
        if (!$qacontact_checked && $qacontact) {
            $usercache{$qacontact} ||= Bugzilla::User->new($qacontact) || next;
            my $qa_user = $usercache{$qacontact};
            if (!$qa_user->can_edit_product($product->id)) {
                    ThrowUserError('invalid_user_group',
                                      {'users'   => $qa_user->login,
                                       'bug_id' => $id,
                                       'product' => $product->name});
            }
        }
    }
}


my %bug_objects = map {$_->id => $_} @bug_objects;

# This loop iterates once for each bug to be processed (i.e. all the
# bugs selected when this script is called with multiple bugs selected
# from buglist.cgi, or just the one bug when called from
# show_bug.cgi).
#
foreach my $id (@idlist) {
    my $query = $basequery;
    my @bug_values = @values;
    # XXX We really have to get rid of $::comma.
    my $comma = $::comma;
    my $old_bug_obj = new Bugzilla::Bug($id);

    my ($status, $everconfirmed);
    my $resolution = $old_bug_obj->resolution;
    # We only care about the resolution field if the user explicitly edits it
    # or if he closes the bug.
    if ($knob eq 'change_resolution' || $cgi->param('resolution')) {
        $resolution = $cgi->param('resolution');
    }
    ($status, $resolution, $everconfirmed) =
      $old_bug_obj->get_new_status_and_resolution($knob, $resolution);

    if ($status ne $old_bug_obj->bug_status) {
        $query .= "$comma bug_status = ?";
        push(@bug_values, $status);
        $comma = ',';
    }
    if ($resolution ne $old_bug_obj->resolution) {
        $query .= "$comma resolution = ?";
        push(@bug_values, $resolution);
        $comma = ',';
    }
    if ($everconfirmed ne $old_bug_obj->everconfirmed) {
        $query .= "$comma everconfirmed = ?";
        push(@bug_values, $everconfirmed);
        $comma = ',';
    }

    # We have to check whether the bug is moved to another product
    # and/or component before reassigning. If $component is defined,
    # use it; else use the product/component the bug is already in.
    if ($cgi->param('set_default_assignee')) {
        my $new_comp_id = $component ? $component->id : $old_bug_obj->component_id;
        $assignee = $dbh->selectrow_array('SELECT initialowner
                                           FROM components
                                           WHERE components.id = ?',
                                           undef, $new_comp_id);
        $query .= "$comma assigned_to = ?";
        push(@bug_values, $assignee);
        $comma = ',';
    }

    if (Bugzilla->params->{'useqacontact'} && $cgi->param('set_default_qa_contact')) {
        my $new_comp_id = $component ? $component->id : $old_bug_obj->component_id;
        $qacontact = $dbh->selectrow_array('SELECT initialqacontact
                                            FROM components
                                            WHERE components.id = ?',
                                            undef, $new_comp_id);
        if ($qacontact) {
            $query .= "$comma qa_contact = ?";
            push(@bug_values, $qacontact);
        }
        else {
            $query .= "$comma qa_contact = NULL";
        }
        $comma = ',';
    }

    my $bug_changed = 0;
    my $write = "WRITE";        # Might want to make a param to control
                                # whether we do LOW_PRIORITY ...
    $dbh->bz_lock_tables("bugs $write", "bugs_activity $write", "cc $write",
            "profiles READ", "dependencies $write", "votes $write",
            "products READ", "components READ", "milestones READ",
            "keywords $write", "longdescs $write", "fielddefs READ",
            "bug_group_map $write", "flags $write", "duplicates $write",
            "user_group_map READ", "group_group_map READ", "flagtypes READ",
            "flaginclusions AS i READ", "flagexclusions AS e READ",
            "keyworddefs READ", "groups READ", "attachments READ",
            "bug_status READ", "group_control_map AS oldcontrolmap READ",
            "group_control_map AS newcontrolmap READ",
            "group_control_map READ", "email_setting READ", "classifications READ");

    # It may sound crazy to set %formhash for each bug as $cgi->param()
    # will not change, but %formhash is modified below and we prefer
    # to set it again.
    my $i = 0;
    my @oldvalues = SnapShotBug($id);
    my %oldhash;
    my %formhash;
    foreach my $col (@editable_bug_fields) {
        # Consider NULL db entries to be equivalent to the empty string
        $oldvalues[$i] = defined($oldvalues[$i]) ? $oldvalues[$i] : '';
        # Convert the deadline taken from the DB into the YYYY-MM-DD format
        # for consistency with the deadline provided by the user, if any.
        # Else Bug::check_can_change_field() would see them as different
        # in all cases.
        if ($col eq 'deadline') {
            $oldvalues[$i] = format_time($oldvalues[$i], "%Y-%m-%d");
        }
        $oldhash{$col} = $oldvalues[$i];
        $formhash{$col} = $cgi->param($col) if defined $cgi->param($col);
        $i++;
    }
    # The status and resolution are defined by the workflow.
    $formhash{'bug_status'} = $status;
    $formhash{'resolution'} = $resolution;

    # We need to convert $newhash{'assigned_to'} and $newhash{'qa_contact'}
    # email addresses into their corresponding IDs;
    $formhash{'qa_contact'} = $qacontact if Bugzilla->params->{'useqacontact'};
    $formhash{'assigned_to'} = $assignee;

    # This hash is required by Bug::check_can_change_field().
    my $cgi_hash = {'dontchange' => scalar $cgi->param('dontchange')};
    foreach my $col (@editable_bug_fields) {
        if (exists $formhash{$col}
            && !$old_bug_obj->check_can_change_field($col, $oldhash{$col}, $formhash{$col},
                                                     \$PrivilegesRequired, $cgi_hash))
        {
            my $vars;
            if ($col eq 'component_id') {
                # Display the component name
                $vars->{'oldvalue'} = $old_bug_obj->component;
                $vars->{'newvalue'} = $cgi->param('component');
                $vars->{'field'} = 'component';
            } elsif ($col eq 'assigned_to' || $col eq 'qa_contact') {
                # Display the assignee or QA contact email address
                $vars->{'oldvalue'} = user_id_to_login($oldhash{$col});
                $vars->{'newvalue'} = user_id_to_login($formhash{$col});
                $vars->{'field'} = $col;
            } else {
                $vars->{'oldvalue'} = $oldhash{$col};
                $vars->{'newvalue'} = $formhash{$col};
                $vars->{'field'} = $col;
            }
            $vars->{'privs'} = $PrivilegesRequired;
            ThrowUserError("illegal_change", $vars);
        }
    }
    
    $oldhash{'product'} = $old_bug_obj->product;
    if (!Bugzilla->user->can_edit_product($oldhash{'product_id'})) {
        ThrowUserError("product_edit_denied",
                      { product => $oldhash{'product'} });
    }

    # If we are editing a single bug or if bugs are being moved into
    # a specific product, $product is defined. If $product is undefined,
    # then we don't move bugs, so we can use their current product.
    my $new_product = $product || new Bugzilla::Product({name => $oldhash{'product'}});
    if ($requiremilestone) {
        # musthavemilestoneonaccept applies only if at least two
        # target milestones are defined for the product.
        if (scalar(@{$new_product->milestones}) > 1) {
            my $value = $cgi->param('target_milestone');
            if (!defined $value || $value eq $cgi->param('dontchange')) {
                $value = $oldhash{'target_milestone'};
            }
            # if musthavemilestoneonaccept == 1, then the target
            # milestone must be different from the default one.
            if ($value eq $new_product->default_milestone) {
                ThrowUserError("milestone_required", { bug_id => $id });
            }
        }
    }   
    if (defined $cgi->param('delta_ts') && $cgi->param('delta_ts') ne $delta_ts)
    {
        ($vars->{'operations'}) =
            Bugzilla::Bug::GetBugActivity($id, $cgi->param('delta_ts'));

        $vars->{'start_at'} = $cgi->param('longdesclength');

        # Always sort midair collision comments oldest to newest,
        # regardless of the user's personal preference.
        $vars->{'comments'} = Bugzilla::Bug::GetComments($id, "oldest_to_newest");

        $cgi->param('delta_ts', $delta_ts);
        
        $vars->{'bug_id'} = $id;
        
        $dbh->bz_unlock_tables(UNLOCK_ABORT);
        
        # Warn the user about the mid-air collision and ask them what to do.
        $template->process("bug/process/midair.html.tmpl", $vars)
          || ThrowTemplateError($template->error());
        exit;
    }

    my $work_time;
    if (Bugzilla->user->in_group(Bugzilla->params->{'timetrackinggroup'})) {
        $work_time = $cgi->param('work_time');
    }

    if ($cgi->param('comment') || $work_time || $duplicate) {
        my $type = $duplicate ? CMT_DUPE_OF : CMT_NORMAL;

        $bug_objects{$id}->add_comment(scalar($cgi->param('comment')),
            { isprivate => scalar($cgi->param('commentprivacy')),
              work_time => $work_time, type => $type, 
              extra_data => $duplicate});
        $bug_changed = 1;
    }
    
    #################################
    # Start Actual Database Updates #
    #################################
    
    $timestamp = $dbh->selectrow_array(q{SELECT NOW()});

    if ($work_time) {
        LogActivityEntry($id, "work_time", "", $work_time,
                         $whoid, $timestamp);
    }

    $bug_objects{$id}->update($timestamp);

    $bug_objects{$id}->update_keywords($timestamp);
    
    $query .= " WHERE bug_id = ?";
    push(@bug_values, $id);

    if ($comma ne '') {
        $dbh->do($query, undef, @bug_values);
    }

    # Check for duplicates if the bug is [re]open or its resolution is changed.
    if ($resolution ne 'DUPLICATE') {
        $dbh->do(q{DELETE FROM duplicates WHERE dupe = ?}, undef, $id);
    }

    # First of all, get all groups the bug is currently restricted to.
    my $initial_groups =
      $dbh->selectcol_arrayref('SELECT group_id, name
                                  FROM bug_group_map
                            INNER JOIN groups
                                    ON groups.id = bug_group_map.group_id
                                 WHERE bug_id = ?', {Columns=>[1,2]}, $id);
    my %original_groups = @$initial_groups;
    my %updated_groups = %original_groups;

    # Now let's see which groups have to be added or removed.
    foreach my $gid (keys %{$new_product->group_controls}) {
        my $group = $new_product->group_controls->{$gid};
        # Leave inactive groups alone.
        next unless $group->{group}->is_active;

        # Only members of a group can add/remove the bug to/from it,
        # unless the bug is being moved to another product in which case
        # non-members can also edit group restrictions.
        if ($group->{membercontrol} == CONTROLMAPMANDATORY
            || ($product_change && $group->{othercontrol} == CONTROLMAPMANDATORY
                && !$user->in_group_id($gid)))
        {
            $updated_groups{$gid} = $group->{group}->name;
        }
        elsif ($group->{membercontrol} == CONTROLMAPNA
               || ($product_change && $group->{othercontrol} == CONTROLMAPNA
                   && !$user->in_group_id($gid)))
        {
            delete $updated_groups{$gid};
        }
        # When editing several bugs at once, only consider groups which
        # have been displayed.
        elsif (($user->in_group_id($gid) || $product_change)
               && (defined $cgi->param('id') || defined $cgi->param("bit-$gid")))
        {
            if (!$cgi->param("bit-$gid")) {
                delete $updated_groups{$gid};
            }
            # Note that == 1 is important, because == -1 means "ignore this group".
            elsif ($cgi->param("bit-$gid") == 1) {
                $updated_groups{$gid} = $group->{group}->name;
            }
        }
    }
    # We also have to remove groups which are not legal in the new product.
    foreach my $gid (keys %updated_groups) {
        delete $updated_groups{$gid}
          unless exists $new_product->group_controls->{$gid};
    }
    my ($removed, $added) = diff_arrays([keys %original_groups], [keys %updated_groups]);

    # We can now update the DB.
    if (scalar(@$removed)) {
        $dbh->do('DELETE FROM bug_group_map WHERE bug_id = ?
                  AND group_id IN (' . join(', ', @$removed) . ')',
                  undef, $id);
    }
    if (scalar(@$added)) {
        my $sth = $dbh->prepare('INSERT INTO bug_group_map
                                 (bug_id, group_id) VALUES (?, ?)');
        $sth->execute($id, $_) foreach @$added;
    }

    # Add the changes to the bug_activity table.
    if (scalar(@$removed) || scalar(@$added)) {
        my @removed_names = map { $original_groups{$_} } @$removed;
        my @added_names = map { $updated_groups{$_} } @$added;
        LogActivityEntry($id, 'bug_group', join(',', @removed_names),
                         join(',', @added_names), $whoid, $timestamp);
        $bug_changed = 1;
    }

    my ($cc_removed) = $bug_objects{$id}->update_cc($timestamp);
    $cc_removed = [map {$_->login} @$cc_removed];

    my ($dep_changes) = $bug_objects{$id}->update_dependencies($timestamp);
    
    # Convert the "changes" hash into a list of all the bug ids, then
    # convert that into a hash to eliminate duplicates. ("map {@$_}" collapses
    # an array of arrays.)
    my @all_changed_deps = map { @$_ } @{$dep_changes->{'dependson'}};
    push(@all_changed_deps, map { @$_ } @{$dep_changes->{'blocked'}});
    my %changed_deps = map { $_ => 1 } @all_changed_deps;

    # get a snapshot of the newly set values out of the database,
    # and then generate any necessary bug activity entries by seeing 
    # what has changed since before we wrote out the new values.
    #
    my $new_bug_obj = new Bugzilla::Bug($id);
    my @newvalues = SnapShotBug($id);
    my %newhash;
    $i = 0;
    foreach my $col (@editable_bug_fields) {
        # Consider NULL db entries to be equivalent to the empty string
        $newvalues[$i] = defined($newvalues[$i]) ? $newvalues[$i] : '';
        # Convert the deadline to the YYYY-MM-DD format.
        if ($col eq 'deadline') {
            $newvalues[$i] = format_time($newvalues[$i], "%Y-%m-%d");
        }
        $newhash{$col} = $newvalues[$i];
        $i++;
    }
    # for passing to Bugzilla::BugMail to ensure that when someone is removed
    # from one of these fields, they get notified of that fact (if desired)
    #
    my $origOwner = "";
    my $origQaContact = "";

    # $msgs will store emails which have to be sent to voters, if any.
    my $msgs;
    my %notify_deps;
    
    foreach my $c (@editable_bug_fields) {
        my $col = $c;           # We modify it, don't want to modify array
                                # values in place.
        my $old = shift @oldvalues;
        my $new = shift @newvalues;
        if ($old ne $new) {

            # Products and components are now stored in the DB using ID's
            # We need to translate this to English before logging it
            if ($col eq 'product_id') {
                $old = $old_bug_obj->product;
                $new = $new_bug_obj->product;
                $col = 'product';
            }
            if ($col eq 'component_id') {
                $old = $old_bug_obj->component;
                $new = $new_bug_obj->component;
                $col = 'component';
            }

            # save off the old value for passing to Bugzilla::BugMail so
            # the old assignee can be notified
            #
            if ($col eq 'assigned_to') {
                $old = ($old) ? user_id_to_login($old) : "";
                $new = ($new) ? user_id_to_login($new) : "";
                $origOwner = $old;
            }

            # ditto for the old qa contact
            #
            if ($col eq 'qa_contact') {
                $old = ($old) ? user_id_to_login($old) : "";
                $new = ($new) ? user_id_to_login($new) : "";
                $origQaContact = $old;
            }

            # Bugzilla::Bug does these for us already.
            next if grep($_ eq $col, qw(keywords op_sys rep_platform priority
                                        bug_severity short_desc alias
                                        reporter_accessible cclist_accessible
                                        status_whiteboard bug_file_loc),
                                     Bugzilla->custom_field_names);

            if ($col eq 'product') {
                # If some votes have been removed, RemoveVotes() returns
                # a list of messages to send to voters.
                # We delay the sending of these messages till tables are unlocked.
                $msgs = RemoveVotes($id, 0,
                          "This bug has been moved to a different product");

                CheckIfVotedConfirmed($id, $whoid);
            }

            # If this bug has changed from opened to closed or vice-versa,
            # then all of the bugs we block need to be notified.
            if ($col eq 'bug_status' 
                && is_open_state($old) ne is_open_state($new))
            {
                $notify_deps{$_} = 1 foreach (@{$bug_objects{$id}->blocked});
            }

            LogActivityEntry($id,$col,$old,$new,$whoid,$timestamp);
            $bug_changed = 1;
        }
    }
    # Set and update flags.
    Bugzilla::Flag::process($new_bug_obj, undef, $timestamp, $cgi);

    if ($bug_changed) {
        $dbh->do(q{UPDATE bugs SET delta_ts = ? WHERE bug_id = ?},
                 undef, $timestamp, $id);
    }
    $dbh->bz_unlock_tables();

    # Now is a good time to send email to voters.
    foreach my $msg (@$msgs) {
        MessageToMTA($msg);
    }

    if ($duplicate) {
        # If the bug was already marked as a duplicate, remove
        # the existing entry.
        $dbh->do('DELETE FROM duplicates WHERE dupe = ?',
                  undef, $cgi->param('id'));

        my $dup = new Bugzilla::Bug($duplicate);
        my $reporter = $new_bug_obj->reporter;
        my $isoncc = $dbh->selectrow_array(q{SELECT who FROM cc
                                           WHERE bug_id = ? AND who = ?},
                                           undef, $duplicate, $reporter->id);
        unless (($reporter->id == $dup->reporter->id) || $isoncc
                || !$cgi->param('confirm_add_duplicate')) {
            # The reporter is oblivious to the existence of the original bug
            # and is permitted access. Add him to the cc (and record activity).
            LogActivityEntry($duplicate,"cc","",$reporter->name,
                             $whoid,$timestamp);
            $dbh->do(q{INSERT INTO cc (who, bug_id) VALUES (?, ?)},
                     undef, $reporter->id, $duplicate);
        }
        # Bug 171639 - Duplicate notifications do not need to be private.
        $dup->add_comment("", { type => CMT_HAS_DUPE,
                                extra_data => $new_bug_obj->bug_id });
        $dup->update($timestamp);

        $dbh->do(q{INSERT INTO duplicates VALUES (?, ?)}, undef,
                 $duplicate, $cgi->param('id'));
    }

    # Now all changes to the DB have been made. It's time to email
    # all concerned users, including the bug itself, but also the
    # duplicated bug and dependent bugs, if any.

    $vars->{'mailrecipients'} = { 'cc' => $cc_removed,
                                  'owner' => $origOwner,
                                  'qacontact' => $origQaContact,
                                  'changer' => Bugzilla->user->login };

    $vars->{'id'} = $id;
    $vars->{'type'} = "bug";
    
    # Let the user know the bug was changed and who did and didn't
    # receive email about the change.
    send_results($id, $vars);
 
    if ($duplicate) {
        $vars->{'mailrecipients'} = { 'changer' => Bugzilla->user->login }; 

        $vars->{'id'} = $duplicate;
        $vars->{'type'} = "dupe";
        
        # Let the user know a duplication notation was added to the 
        # original bug.
        send_results($duplicate, $vars);
    }

    my %all_dep_changes = (%notify_deps, %changed_deps);
    foreach my $id (sort { $a <=> $b } (keys %all_dep_changes)) {
        $vars->{'mailrecipients'} = { 'changer' => Bugzilla->user->login };
        $vars->{'id'} = $id;
        $vars->{'type'} = "dep";

        # Let the user (if he is able to see the bug) know we checked to
        # see if we should email notice of this change to users with a 
        # relationship to the dependent bug and who did and didn't 
        # receive email about it.
        send_results($id, $vars);
    }
}

# Determine if Patch Viewer is installed, for Diff link
# (NB: Duplicate code with show_bug.cgi.)
eval {
    require PatchReader;
    $vars->{'patchviewerinstalled'} = 1;
};

if (defined $cgi->param('id')) {
    $action = Bugzilla->user->settings->{'post_bug_submit_action'}->{'value'};
} else {
    # param('id') is not defined when changing multiple bugs
    $action = 'nothing';
}

if (Bugzilla->usage_mode == USAGE_MODE_EMAIL) {
    # Do nothing.
}
elsif ($action eq 'next_bug') {
    my $next_bug;
    my $cur = lsearch(\@bug_list, $cgi->param("id"));
    if ($cur >= 0 && $cur < $#bug_list) {
        $next_bug = $bug_list[$cur + 1];
    }
    if ($next_bug) {
        if (detaint_natural($next_bug) && Bugzilla->user->can_see_bug($next_bug)) {
            my $bug = new Bugzilla::Bug($next_bug);
            ThrowCodeError("bug_error", { bug => $bug }) if $bug->error;

            $vars->{'bugs'} = [$bug];
            $vars->{'nextbug'} = $bug->bug_id;

            $template->process("bug/show.html.tmpl", $vars)
              || ThrowTemplateError($template->error());

            exit;
        }
    }
} elsif ($action eq 'same_bug') {
    if (Bugzilla->user->can_see_bug($cgi->param('id'))) {
        my $bug = new Bugzilla::Bug($cgi->param('id'));
        ThrowCodeError("bug_error", { bug => $bug }) if $bug->error;

        $vars->{'bugs'} = [$bug];

        $template->process("bug/show.html.tmpl", $vars)
          || ThrowTemplateError($template->error());

        exit;
    }
} elsif ($action ne 'nothing') {
    ThrowCodeError("invalid_post_bug_submit_action");
}

# End the response page.
unless (Bugzilla->usage_mode == USAGE_MODE_EMAIL) {
    # The user pref is 'Do nothing', so all we need is the current bug ID.
    $vars->{'bug'} = {bug_id => scalar $cgi->param('id')};

    $template->process("bug/navigate.html.tmpl", $vars)
        || ThrowTemplateError($template->error());
    $template->process("global/footer.html.tmpl", $vars)
        || ThrowTemplateError($template->error());
}

1;
