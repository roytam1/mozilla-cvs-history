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
# Contributor(s): Dawn Endico    <endico@mozilla.org>
#                 Terry Weissman <terry@mozilla.org>
#                 Chris Yeh      <cyeh@bluemartini.com>
#                 Bradley Baetz  <bbaetz@acm.org>
#                 Dave Miller    <justdave@bugzilla.org>
#                 Max Kanat-Alexander <mkanat@bugzilla.org>
#                 Frédéric Buclin <LpSolit@gmail.com>
#                 Lance Larsh <lance.larsh@oracle.com>

package Bugzilla::Bug;

use strict;

use Bugzilla::Attachment;
use Bugzilla::Constants;
use Bugzilla::Field;
use Bugzilla::Flag;
use Bugzilla::FlagType;
use Bugzilla::User;
use Bugzilla::Util;
use Bugzilla::Error;
use Bugzilla::Product;

use List::Util qw(min);

use base qw(Exporter);
@Bugzilla::Bug::EXPORT = qw(
    AppendComment ValidateComment
    bug_alias_to_id ValidateBugAlias ValidateBugID
    RemoveVotes CheckIfVotedConfirmed
    LogActivityEntry
    is_open_state
    editable_bug_fields
);

#####################################################################
# Constants
#####################################################################

# Used in LogActivityEntry(). Gives the max length of lines in the
# activity table.
use constant MAX_LINE_LENGTH => 254;

# Used in ValidateComment(). Gives the max length allowed for a comment.
use constant MAX_COMMENT_LENGTH => 65535;

#####################################################################

sub new {
  my $invocant = shift;
  my $class = ref($invocant) || $invocant;
  my $self = {};
  bless $self, $class;
  $self->_init(@_);
  return $self;
}

sub _init {
  my $self = shift();
  my ($bug_id) = (@_);
  my $dbh = Bugzilla->dbh;

  $bug_id = trim($bug_id || 0);

  my $old_bug_id = $bug_id;

  # If the bug ID isn't numeric, it might be an alias, so try to convert it.
  $bug_id = bug_alias_to_id($bug_id) if $bug_id !~ /^0*[1-9][0-9]*$/;

  unless ($bug_id && detaint_natural($bug_id)) {
      # no bug number given or the alias didn't match a bug
      $self->{'bug_id'} = $old_bug_id;
      $self->{'error'} = "InvalidBugId";
      return $self;
  }

    my $custom_fields = "";
    if (scalar(Bugzilla->custom_field_names) > 0) {
        $custom_fields = ", " . join(", ", Bugzilla->custom_field_names);
    }

  my $query = "
    SELECT
      bugs.bug_id, alias, products.classification_id, classifications.name,
      bugs.product_id, products.name, version,
      rep_platform, op_sys, bug_status, resolution, priority,
      bug_severity, bugs.component_id, components.name, 
      assigned_to AS assigned_to_id, reporter AS reporter_id,
      bug_file_loc, short_desc, target_milestone,
      qa_contact AS qa_contact_id, status_whiteboard, " .
      $dbh->sql_date_format('creation_ts', '%Y.%m.%d %H:%i') . ",
      delta_ts, everconfirmed, reporter_accessible, cclist_accessible,
      estimated_time, remaining_time, " .
      $dbh->sql_date_format('deadline', '%Y-%m-%d') .
      $custom_fields . "
    FROM bugs
      INNER JOIN components
              ON components.id = bugs.component_id
      INNER JOIN products
              ON products.id = bugs.product_id
      INNER JOIN classifications
              ON classifications.id = products.classification_id
      WHERE bugs.bug_id = ?";

  my $bug_sth = $dbh->prepare($query);
  $bug_sth->execute($bug_id);
  my @row;

  if (@row = $bug_sth->fetchrow_array) {
    my $count = 0;
    my %fields;
    foreach my $field ("bug_id", "alias", "classification_id", "classification",
                       "product_id", "product", "version", 
                       "rep_platform", "op_sys", "bug_status", "resolution", 
                       "priority", "bug_severity", "component_id", "component",
                       "assigned_to_id", "reporter_id", 
                       "bug_file_loc", "short_desc",
                       "target_milestone", "qa_contact_id", "status_whiteboard",
                       "creation_ts", "delta_ts", "everconfirmed",
                       "reporter_accessible", "cclist_accessible",
                       "estimated_time", "remaining_time", "deadline",
                       Bugzilla->custom_field_names)
      {
        $fields{$field} = shift @row;
        if (defined $fields{$field}) {
            $self->{$field} = $fields{$field};
        }
        $count++;
    }
  } else {
      $self->{'bug_id'} = $bug_id;
      $self->{'error'} = "NotFound";
      return $self;
  }

  $self->{'isunconfirmed'} = ($self->{bug_status} eq 'UNCONFIRMED');
  $self->{'isopened'} = is_open_state($self->{bug_status});
  
  return $self;
}

# This is the correct way to delete bugs from the DB.
# No bug should be deleted from anywhere else except from here.
#
sub remove_from_db {
    my ($self) = @_;
    my $dbh = Bugzilla->dbh;

    if ($self->{'error'}) {
        ThrowCodeError("bug_error", { bug => $self });
    }

    my $bug_id = $self->{'bug_id'};

    # tables having 'bugs.bug_id' as a foreign key:
    # - attachments
    # - bug_group_map
    # - bugs
    # - bugs_activity
    # - cc
    # - dependencies
    # - duplicates
    # - flags
    # - keywords
    # - longdescs
    # - votes

    # Also, the attach_data table uses attachments.attach_id as a foreign
    # key, and so indirectly depends on a bug deletion too.

    $dbh->bz_lock_tables('attachments WRITE', 'bug_group_map WRITE',
                         'bugs WRITE', 'bugs_activity WRITE', 'cc WRITE',
                         'dependencies WRITE', 'duplicates WRITE',
                         'flags WRITE', 'keywords WRITE',
                         'longdescs WRITE', 'votes WRITE',
                         'attach_data WRITE');

    $dbh->do("DELETE FROM bug_group_map WHERE bug_id = ?", undef, $bug_id);
    $dbh->do("DELETE FROM bugs_activity WHERE bug_id = ?", undef, $bug_id);
    $dbh->do("DELETE FROM cc WHERE bug_id = ?", undef, $bug_id);
    $dbh->do("DELETE FROM dependencies WHERE blocked = ? OR dependson = ?",
             undef, ($bug_id, $bug_id));
    $dbh->do("DELETE FROM duplicates WHERE dupe = ? OR dupe_of = ?",
             undef, ($bug_id, $bug_id));
    $dbh->do("DELETE FROM flags WHERE bug_id = ?", undef, $bug_id);
    $dbh->do("DELETE FROM keywords WHERE bug_id = ?", undef, $bug_id);
    $dbh->do("DELETE FROM longdescs WHERE bug_id = ?", undef, $bug_id);
    $dbh->do("DELETE FROM votes WHERE bug_id = ?", undef, $bug_id);

    # The attach_data table doesn't depend on bugs.bug_id directly.
    my $attach_ids =
        $dbh->selectcol_arrayref("SELECT attach_id FROM attachments
                                  WHERE bug_id = ?", undef, $bug_id);

    if (scalar(@$attach_ids)) {
        $dbh->do("DELETE FROM attach_data WHERE id IN (" .
                 join(",", @$attach_ids) . ")");
    }

    # Several of the previous tables also depend on attach_id.
    $dbh->do("DELETE FROM attachments WHERE bug_id = ?", undef, $bug_id);
    $dbh->do("DELETE FROM bugs WHERE bug_id = ?", undef, $bug_id);

    $dbh->bz_unlock_tables();

    # Now this bug no longer exists
    $self->DESTROY;
    return $self;
}


#####################################################################
# Class Accessors
#####################################################################

sub fields {
    my $class = shift;

    return (
        # Standard Fields
        # Keep this ordering in sync with bugzilla.dtd.
        qw(bug_id alias creation_ts short_desc delta_ts
           reporter_accessible cclist_accessible
           classification_id classification
           product component version rep_platform op_sys
           bug_status resolution dup_id
           bug_file_loc status_whiteboard keywords
           priority bug_severity target_milestone
           dependson blocked votes
           reporter assigned_to cc),
    
        # Conditional Fields
        Bugzilla->params->{'useqacontact'} ? "qa_contact" : (),
        Bugzilla->params->{'timetrackinggroup'} ? 
            qw(estimated_time remaining_time actual_time deadline) : (),
    
        # Custom Fields
        Bugzilla->custom_field_names
    );
}


#####################################################################
# Instance Accessors
#####################################################################

# These subs are in alphabetical order, as much as possible.
# If you add a new sub, please try to keep it in alphabetical order
# with the other ones.

# Note: If you add a new method, remember that you must check the error
# state of the bug before returning any data. If $self->{error} is
# defined, then return something empty. Otherwise you risk potential
# security holes.

sub dup_id {
    my ($self) = @_;
    return $self->{'dup_id'} if exists $self->{'dup_id'};

    $self->{'dup_id'} = undef;
    return if $self->{'error'};

    if ($self->{'resolution'} eq 'DUPLICATE') { 
        my $dbh = Bugzilla->dbh;
        $self->{'dup_id'} =
          $dbh->selectrow_array(q{SELECT dupe_of 
                                  FROM duplicates
                                  WHERE dupe = ?},
                                undef,
                                $self->{'bug_id'});
    }
    return $self->{'dup_id'};
}

sub actual_time {
    my ($self) = @_;
    return $self->{'actual_time'} if exists $self->{'actual_time'};

    if ( $self->{'error'} || 
         !Bugzilla->user->in_group(Bugzilla->params->{"timetrackinggroup"}) ) {
        $self->{'actual_time'} = undef;
        return $self->{'actual_time'};
    }

    my $sth = Bugzilla->dbh->prepare("SELECT SUM(work_time)
                                      FROM longdescs 
                                      WHERE longdescs.bug_id=?");
    $sth->execute($self->{bug_id});
    $self->{'actual_time'} = $sth->fetchrow_array();
    return $self->{'actual_time'};
}

sub any_flags_requesteeble {
    my ($self) = @_;
    return $self->{'any_flags_requesteeble'} 
        if exists $self->{'any_flags_requesteeble'};
    return 0 if $self->{'error'};

    $self->{'any_flags_requesteeble'} = 
        grep($_->{'is_requesteeble'}, @{$self->flag_types});

    return $self->{'any_flags_requesteeble'};
}

sub attachments {
    my ($self) = @_;
    return $self->{'attachments'} if exists $self->{'attachments'};
    return [] if $self->{'error'};

    $self->{'attachments'} =
        Bugzilla::Attachment->get_attachments_by_bug($self->bug_id);
    return $self->{'attachments'};
}

sub assigned_to {
    my ($self) = @_;
    return $self->{'assigned_to'} if exists $self->{'assigned_to'};
    $self->{'assigned_to_id'} = 0 if $self->{'error'};
    $self->{'assigned_to'} = new Bugzilla::User($self->{'assigned_to_id'});
    return $self->{'assigned_to'};
}

sub blocked {
    my ($self) = @_;
    return $self->{'blocked'} if exists $self->{'blocked'};
    return [] if $self->{'error'};
    $self->{'blocked'} = EmitDependList("dependson", "blocked", $self->bug_id);
    return $self->{'blocked'};
}

# Even bugs in an error state always have a bug_id.
sub bug_id { $_[0]->{'bug_id'}; }

sub cc {
    my ($self) = @_;
    return $self->{'cc'} if exists $self->{'cc'};
    return [] if $self->{'error'};

    my $dbh = Bugzilla->dbh;
    $self->{'cc'} = $dbh->selectcol_arrayref(
        q{SELECT profiles.login_name FROM cc, profiles
           WHERE bug_id = ?
             AND cc.who = profiles.userid
        ORDER BY profiles.login_name},
      undef, $self->bug_id);

    $self->{'cc'} = undef if !scalar(@{$self->{'cc'}});

    return $self->{'cc'};
}

sub dependson {
    my ($self) = @_;
    return $self->{'dependson'} if exists $self->{'dependson'};
    return [] if $self->{'error'};
    $self->{'dependson'} = 
        EmitDependList("blocked", "dependson", $self->bug_id);
    return $self->{'dependson'};
}

sub flag_types {
    my ($self) = @_;
    return $self->{'flag_types'} if exists $self->{'flag_types'};
    return [] if $self->{'error'};

    # The types of flags that can be set on this bug.
    # If none, no UI for setting flags will be displayed.
    my $flag_types = Bugzilla::FlagType::match(
        {'target_type'  => 'bug',
         'product_id'   => $self->{'product_id'}, 
         'component_id' => $self->{'component_id'} });

    foreach my $flag_type (@$flag_types) {
        $flag_type->{'flags'} = Bugzilla::Flag::match(
            { 'bug_id'      => $self->bug_id,
              'type_id'     => $flag_type->{'id'},
              'target_type' => 'bug' });
    }

    $self->{'flag_types'} = $flag_types;

    return $self->{'flag_types'};
}

sub keywords {
    my ($self) = @_;
    return $self->{'keywords'} if exists $self->{'keywords'};
    return () if $self->{'error'};

    my $dbh = Bugzilla->dbh;
    my $list_ref = $dbh->selectcol_arrayref(
         "SELECT keyworddefs.name
            FROM keyworddefs, keywords
           WHERE keywords.bug_id = ?
             AND keyworddefs.id = keywords.keywordid
        ORDER BY keyworddefs.name",
        undef, ($self->bug_id));

    $self->{'keywords'} = join(', ', @$list_ref);
    return $self->{'keywords'};
}

sub longdescs {
    my ($self) = @_;
    return $self->{'longdescs'} if exists $self->{'longdescs'};
    return [] if $self->{'error'};
    $self->{'longdescs'} = GetComments($self->{bug_id});
    return $self->{'longdescs'};
}

sub milestoneurl {
    my ($self) = @_;
    return $self->{'milestoneurl'} if exists $self->{'milestoneurl'};
    return '' if $self->{'error'};

    $self->{'prod_obj'} ||= new Bugzilla::Product({name => $self->{'product'}});
    $self->{'milestoneurl'} = $self->{'prod_obj'}->milestone_url;
    return $self->{'milestoneurl'};
}

sub qa_contact {
    my ($self) = @_;
    return $self->{'qa_contact'} if exists $self->{'qa_contact'};
    return undef if $self->{'error'};

    if (Bugzilla->params->{'useqacontact'} && $self->{'qa_contact_id'}) {
        $self->{'qa_contact'} = new Bugzilla::User($self->{'qa_contact_id'});
    } else {
        # XXX - This is somewhat inconsistent with the assignee/reporter 
        # methods, which will return an empty User if they get a 0. 
        # However, we're keeping it this way now, for backwards-compatibility.
        $self->{'qa_contact'} = undef;
    }
    return $self->{'qa_contact'};
}

sub reporter {
    my ($self) = @_;
    return $self->{'reporter'} if exists $self->{'reporter'};
    $self->{'reporter_id'} = 0 if $self->{'error'};
    $self->{'reporter'} = new Bugzilla::User($self->{'reporter_id'});
    return $self->{'reporter'};
}


sub show_attachment_flags {
    my ($self) = @_;
    return $self->{'show_attachment_flags'} 
        if exists $self->{'show_attachment_flags'};
    return 0 if $self->{'error'};

    # The number of types of flags that can be set on attachments to this bug
    # and the number of flags on those attachments.  One of these counts must be
    # greater than zero in order for the "flags" column to appear in the table
    # of attachments.
    my $num_attachment_flag_types = Bugzilla::FlagType::count(
        { 'target_type'  => 'attachment',
          'product_id'   => $self->{'product_id'},
          'component_id' => $self->{'component_id'} });
    my $num_attachment_flags = Bugzilla::Flag::count(
        { 'target_type'  => 'attachment',
          'bug_id'       => $self->bug_id });

    $self->{'show_attachment_flags'} =
        ($num_attachment_flag_types || $num_attachment_flags);

    return $self->{'show_attachment_flags'};
}

sub use_votes {
    my ($self) = @_;
    return 0 if $self->{'error'};

    $self->{'prod_obj'} ||= new Bugzilla::Product({name => $self->{'product'}});

    return Bugzilla->params->{'usevotes'} 
           && $self->{'prod_obj'}->votes_per_user > 0;
}

sub groups {
    my $self = shift;
    return $self->{'groups'} if exists $self->{'groups'};
    return [] if $self->{'error'};

    my $dbh = Bugzilla->dbh;
    my @groups;

    # Some of this stuff needs to go into Bugzilla::User

    # For every group, we need to know if there is ANY bug_group_map
    # record putting the current bug in that group and if there is ANY
    # user_group_map record putting the user in that group.
    # The LEFT JOINs are checking for record existence.
    #
    my $grouplist = Bugzilla->user->groups_as_string;
    my $sth = $dbh->prepare(
             "SELECT DISTINCT groups.id, name, description," .
             " CASE WHEN bug_group_map.group_id IS NOT NULL" .
             " THEN 1 ELSE 0 END," .
             " CASE WHEN groups.id IN($grouplist) THEN 1 ELSE 0 END," .
             " isactive, membercontrol, othercontrol" .
             " FROM groups" . 
             " LEFT JOIN bug_group_map" .
             " ON bug_group_map.group_id = groups.id" .
             " AND bug_id = ?" .
             " LEFT JOIN group_control_map" .
             " ON group_control_map.group_id = groups.id" .
             " AND group_control_map.product_id = ? " .
             " WHERE isbuggroup = 1" .
             " ORDER BY description");
    $sth->execute($self->{'bug_id'},
                  $self->{'product_id'});

    while (my ($groupid, $name, $description, $ison, $ingroup, $isactive,
            $membercontrol, $othercontrol) = $sth->fetchrow_array()) {

        $membercontrol ||= 0;

        # For product groups, we only want to use the group if either
        # (1) The bit is set and not required, or
        # (2) The group is Shown or Default for members and
        #     the user is a member of the group.
        if ($ison ||
            ($isactive && $ingroup
                       && (($membercontrol == CONTROLMAPDEFAULT)
                           || ($membercontrol == CONTROLMAPSHOWN))
            ))
        {
            my $ismandatory = $isactive
              && ($membercontrol == CONTROLMAPMANDATORY);

            push (@groups, { "bit" => $groupid,
                             "name" => $name,
                             "ison" => $ison,
                             "ingroup" => $ingroup,
                             "mandatory" => $ismandatory,
                             "description" => $description });
        }
    }

    $self->{'groups'} = \@groups;

    return $self->{'groups'};
}

sub user {
    my $self = shift;
    return $self->{'user'} if exists $self->{'user'};
    return {} if $self->{'error'};

    my $user = Bugzilla->user;
    my $canmove = Bugzilla->params->{'move-enabled'} && $user->is_mover;

    # In the below, if the person hasn't logged in, then we treat them
    # as if they can do anything.  That's because we don't know why they
    # haven't logged in; it may just be because they don't use cookies.
    # Display everything as if they have all the permissions in the
    # world; their permissions will get checked when they log in and
    # actually try to make the change.
    my $unknown_privileges = !$user->id
                             || $user->in_group("editbugs");
    my $canedit = $unknown_privileges
                  || $user->id == $self->{assigned_to_id}
                  || (Bugzilla->params->{'useqacontact'}
                      && $self->{'qa_contact_id'}
                      && $user->id == $self->{qa_contact_id});
    my $canconfirm = $unknown_privileges
                     || $user->in_group("canconfirm");
    my $isreporter = $user->id
                     && $user->id == $self->{reporter_id};

    $self->{'user'} = {canmove    => $canmove,
                       canconfirm => $canconfirm,
                       canedit    => $canedit,
                       isreporter => $isreporter};
    return $self->{'user'};
}

sub choices {
    my $self = shift;
    return $self->{'choices'} if exists $self->{'choices'};
    return {} if $self->{'error'};

    $self->{'choices'} = {};
    $self->{prod_obj} ||= new Bugzilla::Product({name => $self->{product}});

    my @prodlist = map {$_->name} @{Bugzilla->user->get_enterable_products};
    # The current product is part of the popup, even if new bugs are no longer
    # allowed for that product
    if (lsearch(\@prodlist, $self->{'product'}) < 0) {
        push(@prodlist, $self->{'product'});
        @prodlist = sort @prodlist;
    }

    # Hack - this array contains "". See bug 106589.
    my @res = grep ($_, @{settable_resolutions()});

    $self->{'choices'} =
      {
       'product' => \@prodlist,
       'rep_platform' => get_legal_field_values('rep_platform'),
       'priority'     => get_legal_field_values('priority'),
       'bug_severity' => get_legal_field_values('bug_severity'),
       'op_sys'       => get_legal_field_values('op_sys'),
       'bug_status'   => get_legal_field_values('bug_status'),
       'resolution'   => \@res,
       'component'    => [map($_->name, @{$self->{prod_obj}->components})],
       'version'      => [map($_->name, @{$self->{prod_obj}->versions})],
       'target_milestone' => [map($_->name, @{$self->{prod_obj}->milestones})],
      };

    return $self->{'choices'};
}

# List of resolutions that may be set directly by hand in the bug form.
# 'MOVED' and 'DUPLICATE' are excluded from the list because setting
# bugs to those resolutions requires a special process.
sub settable_resolutions {
    my $resolutions = get_legal_field_values('resolution');
    my $pos = lsearch($resolutions, 'DUPLICATE');
    if ($pos >= 0) {
        splice(@$resolutions, $pos, 1);
    }
    $pos = lsearch($resolutions, 'MOVED');
    if ($pos >= 0) {
        splice(@$resolutions, $pos, 1);
    }
    return $resolutions;
}

sub votes {
    my ($self) = @_;
    return 0 if $self->{error};
    return $self->{votes} if defined $self->{votes};

    my $dbh = Bugzilla->dbh;
    $self->{votes} = $dbh->selectrow_array(
        'SELECT SUM(vote_count) FROM votes
          WHERE bug_id = ? ' . $dbh->sql_group_by('bug_id'),
        undef, $self->bug_id);
    $self->{votes} ||= 0;
    return $self->{votes};
}

# Convenience Function. If you need speed, use this. If you need
# other Bug fields in addition to this, just create a new Bug with
# the alias.
# Queries the database for the bug with a given alias, and returns
# the ID of the bug if it exists or the undefined value if it doesn't.
sub bug_alias_to_id {
    my ($alias) = @_;
    return undef unless Bugzilla->params->{"usebugaliases"};
    my $dbh = Bugzilla->dbh;
    trick_taint($alias);
    return $dbh->selectrow_array(
        "SELECT bug_id FROM bugs WHERE alias = ?", undef, $alias);
}

#####################################################################
# Subroutines
#####################################################################

sub AppendComment {
    my ($bugid, $whoid, $comment, $isprivate, $timestamp, $work_time) = @_;
    $work_time ||= 0;
    my $dbh = Bugzilla->dbh;

    ValidateTime($work_time, "work_time") if $work_time;
    trick_taint($work_time);

    # Use the date/time we were given if possible (allowing calling code
    # to synchronize the comment's timestamp with those of other records).
    $timestamp ||= $dbh->selectrow_array('SELECT NOW()');

    $comment =~ s/\r\n/\n/g;     # Handle Windows-style line endings.
    $comment =~ s/\r/\n/g;       # Handle Mac-style line endings.

    if ($comment =~ /^\s*$/) {  # Nothin' but whitespace
        return;
    }

    # Comments are always safe, because we always display their raw contents,
    # and we use them in a placeholder below.
    trick_taint($comment); 
    my $privacyval = $isprivate ? 1 : 0 ;
    $dbh->do(q{INSERT INTO longdescs
                      (bug_id, who, bug_when, thetext, isprivate, work_time)
               VALUES (?,?,?,?,?,?)}, undef,
             ($bugid, $whoid, $timestamp, $comment, $privacyval, $work_time));
    $dbh->do("UPDATE bugs SET delta_ts = ? WHERE bug_id = ?",
             undef, $timestamp, $bugid);
}

# Represents which fields from the bugs table are handled by process_bug.cgi.
sub editable_bug_fields {
    my @fields = Bugzilla->dbh->bz_table_columns('bugs');
    foreach my $remove ("bug_id", "creation_ts", "delta_ts", "lastdiffed") {
        my $location = lsearch(\@fields, $remove);
        splice(@fields, $location, 1);
    }
    # Sorted because the old @::log_columns variable, which this replaces,
    # was sorted.
    return sort(@fields);
}

# This method is private and is not to be used outside of the Bug class.
sub EmitDependList {
    my ($myfield, $targetfield, $bug_id) = (@_);
    my $dbh = Bugzilla->dbh;
    my $list_ref =
        $dbh->selectcol_arrayref(
          "SELECT dependencies.$targetfield
             FROM dependencies, bugs
            WHERE dependencies.$myfield = ?
              AND bugs.bug_id = dependencies.$targetfield
         ORDER BY dependencies.$targetfield",
         undef, ($bug_id));
    return $list_ref;
}

# Tells you whether or not the argument is a valid "open" state.
sub is_open_state {
    my ($state) = @_;
    return (grep($_ eq $state, BUG_STATE_OPEN) ? 1 : 0);
}

sub ValidateTime {
    my ($time, $field) = @_;

    # regexp verifies one or more digits, optionally followed by a period and
    # zero or more digits, OR we have a period followed by one or more digits
    # (allow negatives, though, so people can back out errors in time reporting)
    if ($time !~ /^-?(?:\d+(?:\.\d*)?|\.\d+)$/) {
        ThrowUserError("number_not_numeric",
                       {field => "$field", num => "$time"});
    }

    # Only the "work_time" field is allowed to contain a negative value.
    if ( ($time < 0) && ($field ne "work_time") ) {
        ThrowUserError("number_too_small",
                       {field => "$field", num => "$time", min_num => "0"});
    }

    if ($time > 99999.99) {
        ThrowUserError("number_too_large",
                       {field => "$field", num => "$time", max_num => "99999.99"});
    }
}

sub GetComments {
    my ($id, $comment_sort_order) = (@_);
    $comment_sort_order = $comment_sort_order ||
        Bugzilla->user->settings->{'comment_sort_order'}->{'value'};

    my $sort_order = ($comment_sort_order eq "oldest_to_newest") ? 'asc' : 'desc';
    my $dbh = Bugzilla->dbh;
    my @comments;
    my $sth = $dbh->prepare(
            "SELECT  profiles.realname AS name, profiles.login_name AS email,
            " . $dbh->sql_date_format('longdescs.bug_when', '%Y.%m.%d %H:%i:%s') . "
               AS time, longdescs.thetext AS body, longdescs.work_time,
                     isprivate, already_wrapped
             FROM    longdescs, profiles
            WHERE    profiles.userid = longdescs.who
              AND    longdescs.bug_id = ?
            ORDER BY longdescs.bug_when $sort_order");
    $sth->execute($id);

    while (my $comment_ref = $sth->fetchrow_hashref()) {
        my %comment = %$comment_ref;

        $comment{'email'} .= Bugzilla->params->{'emailsuffix'};
        $comment{'name'} = $comment{'name'} || $comment{'email'};

        push (@comments, \%comment);
    }
   
    if ($comment_sort_order eq "newest_to_oldest_desc_first") {
        unshift(@comments, pop @comments);
    }

    return \@comments;
}

# Get the activity of a bug, starting from $starttime (if given).
# This routine assumes ValidateBugID has been previously called.
sub GetBugActivity {
    my ($id, $starttime) = @_;
    my $dbh = Bugzilla->dbh;

    # Arguments passed to the SQL query.
    my @args = ($id);

    # Only consider changes since $starttime, if given.
    my $datepart = "";
    if (defined $starttime) {
        trick_taint($starttime);
        push (@args, $starttime);
        $datepart = "AND bugs_activity.bug_when > ?";
    }

    # Only includes attachments the user is allowed to see.
    my $suppjoins = "";
    my $suppwhere = "";
    if (Bugzilla->params->{"insidergroup"} 
        && !UserInGroup(Bugzilla->params->{'insidergroup'})) 
    {
        $suppjoins = "LEFT JOIN attachments 
                   ON attachments.attach_id = bugs_activity.attach_id";
        $suppwhere = "AND COALESCE(attachments.isprivate, 0) = 0";
    }

    my $query = "
        SELECT COALESCE(fielddefs.description, " 
               # This is a hack - PostgreSQL requires both COALESCE
               # arguments to be of the same type, and this is the only
               # way supported by both MySQL 3 and PostgreSQL to convert
               # an integer to a string. MySQL 4 supports CAST.
               . $dbh->sql_string_concat('bugs_activity.fieldid', q{''}) .
               "), fielddefs.name, bugs_activity.attach_id, " .
        $dbh->sql_date_format('bugs_activity.bug_when', '%Y.%m.%d %H:%i:%s') .
            ", bugs_activity.removed, bugs_activity.added, profiles.login_name
          FROM bugs_activity
               $suppjoins
     LEFT JOIN fielddefs
            ON bugs_activity.fieldid = fielddefs.id
    INNER JOIN profiles
            ON profiles.userid = bugs_activity.who
         WHERE bugs_activity.bug_id = ?
               $datepart
               $suppwhere
      ORDER BY bugs_activity.bug_when";

    my $list = $dbh->selectall_arrayref($query, undef, @args);

    my @operations;
    my $operation = {};
    my $changes = [];
    my $incomplete_data = 0;

    foreach my $entry (@$list) {
        my ($field, $fieldname, $attachid, $when, $removed, $added, $who) = @$entry;
        my %change;
        my $activity_visible = 1;

        # check if the user should see this field's activity
        if ($fieldname eq 'remaining_time'
            || $fieldname eq 'estimated_time'
            || $fieldname eq 'work_time'
            || $fieldname eq 'deadline')
        {
            $activity_visible = 
                UserInGroup(Bugzilla->params->{'timetrackinggroup'}) ? 1 : 0;
        } else {
            $activity_visible = 1;
        }

        if ($activity_visible) {
            # This gets replaced with a hyperlink in the template.
            $field =~ s/^Attachment// if $attachid;

            # Check for the results of an old Bugzilla data corruption bug
            $incomplete_data = 1 if ($added =~ /^\?/ || $removed =~ /^\?/);

            # An operation, done by 'who' at time 'when', has a number of
            # 'changes' associated with it.
            # If this is the start of a new operation, store the data from the
            # previous one, and set up the new one.
            if ($operation->{'who'}
                && ($who ne $operation->{'who'}
                    || $when ne $operation->{'when'}))
            {
                $operation->{'changes'} = $changes;
                push (@operations, $operation);

                # Create new empty anonymous data structures.
                $operation = {};
                $changes = [];
            }

            $operation->{'who'} = $who;
            $operation->{'when'} = $when;

            $change{'field'} = $field;
            $change{'fieldname'} = $fieldname;
            $change{'attachid'} = $attachid;
            $change{'removed'} = $removed;
            $change{'added'} = $added;
            push (@$changes, \%change);
        }
    }

    if ($operation->{'who'}) {
        $operation->{'changes'} = $changes;
        push (@operations, $operation);
    }

    return(\@operations, $incomplete_data);
}

# Update the bugs_activity table to reflect changes made in bugs.
sub LogActivityEntry {
    my ($i, $col, $removed, $added, $whoid, $timestamp) = @_;
    my $dbh = Bugzilla->dbh;
    # in the case of CCs, deps, and keywords, there's a possibility that someone
    # might try to add or remove a lot of them at once, which might take more
    # space than the activity table allows.  We'll solve this by splitting it
    # into multiple entries if it's too long.
    while ($removed || $added) {
        my ($removestr, $addstr) = ($removed, $added);
        if (length($removestr) > MAX_LINE_LENGTH) {
            my $commaposition = find_wrap_point($removed, MAX_LINE_LENGTH);
            $removestr = substr($removed, 0, $commaposition);
            $removed = substr($removed, $commaposition);
            $removed =~ s/^[,\s]+//; # remove any comma or space
        } else {
            $removed = ""; # no more entries
        }
        if (length($addstr) > MAX_LINE_LENGTH) {
            my $commaposition = find_wrap_point($added, MAX_LINE_LENGTH);
            $addstr = substr($added, 0, $commaposition);
            $added = substr($added, $commaposition);
            $added =~ s/^[,\s]+//; # remove any comma or space
        } else {
            $added = ""; # no more entries
        }
        trick_taint($addstr);
        trick_taint($removestr);
        my $fieldid = get_field_id($col);
        $dbh->do("INSERT INTO bugs_activity
                  (bug_id, who, bug_when, fieldid, removed, added)
                  VALUES (?, ?, ?, ?, ?, ?)",
                  undef, ($i, $whoid, $timestamp, $fieldid, $removestr, $addstr));
    }
}

# CountOpenDependencies counts the number of open dependent bugs for a
# list of bugs and returns a list of bug_id's and their dependency count
# It takes one parameter:
#  - A list of bug numbers whose dependencies are to be checked
sub CountOpenDependencies {
    my (@bug_list) = @_;
    my @dependencies;
    my $dbh = Bugzilla->dbh;

    my $sth = $dbh->prepare(
          "SELECT blocked, COUNT(bug_status) " .
            "FROM bugs, dependencies " .
           "WHERE blocked IN (" . (join "," , @bug_list) . ") " .
             "AND bug_id = dependson " .
             "AND bug_status IN ('" . (join "','", BUG_STATE_OPEN)  . "') " .
          $dbh->sql_group_by('blocked'));
    $sth->execute();

    while (my ($bug_id, $dependencies) = $sth->fetchrow_array()) {
        push(@dependencies, { bug_id       => $bug_id,
                              dependencies => $dependencies });
    }

    return @dependencies;
}

sub ValidateComment {
    my ($comment) = @_;

    if (defined($comment) && length($comment) > MAX_COMMENT_LENGTH) {
        ThrowUserError("comment_too_long");
    }
}

# If a bug is moved to a product which allows less votes per bug
# compared to the previous product, extra votes need to be removed.
sub RemoveVotes {
    my ($id, $who, $reason) = (@_);
    my $dbh = Bugzilla->dbh;

    my $whopart = ($who) ? " AND votes.who = $who" : "";

    my $sth = $dbh->prepare("SELECT profiles.login_name, " .
                            "profiles.userid, votes.vote_count, " .
                            "products.votesperuser, products.maxvotesperbug " .
                            "FROM profiles " . 
                            "LEFT JOIN votes ON profiles.userid = votes.who " .
                            "LEFT JOIN bugs ON votes.bug_id = bugs.bug_id " .
                            "LEFT JOIN products ON products.id = bugs.product_id " .
                            "WHERE votes.bug_id = ? " . $whopart);
    $sth->execute($id);
    my @list;
    while (my ($name, $userid, $oldvotes, $votesperuser, $maxvotesperbug) = $sth->fetchrow_array()) {
        push(@list, [$name, $userid, $oldvotes, $votesperuser, $maxvotesperbug]);
    }

    # @messages stores all emails which have to be sent, if any.
    # This array is passed to the caller which will send these emails itself.
    my @messages = ();

    if (scalar(@list)) {
        foreach my $ref (@list) {
            my ($name, $userid, $oldvotes, $votesperuser, $maxvotesperbug) = (@$ref);
            my $s;

            $maxvotesperbug = min($votesperuser, $maxvotesperbug);

            # If this product allows voting and the user's votes are in
            # the acceptable range, then don't do anything.
            next if $votesperuser && $oldvotes <= $maxvotesperbug;

            # If the user has more votes on this bug than this product
            # allows, then reduce the number of votes so it fits
            my $newvotes = $maxvotesperbug;

            my $removedvotes = $oldvotes - $newvotes;

            $s = ($oldvotes == 1) ? "" : "s";
            my $oldvotestext = "You had $oldvotes vote$s on this bug.";

            $s = ($removedvotes == 1) ? "" : "s";
            my $removedvotestext = "You had $removedvotes vote$s removed from this bug.";

            my $newvotestext;
            if ($newvotes) {
                $dbh->do("UPDATE votes SET vote_count = ? " .
                         "WHERE bug_id = ? AND who = ?",
                         undef, ($newvotes, $id, $userid));
                $s = $newvotes == 1 ? "" : "s";
                $newvotestext = "You still have $newvotes vote$s on this bug."
            } else {
                $dbh->do("DELETE FROM votes WHERE bug_id = ? AND who = ?",
                         undef, ($id, $userid));
                $newvotestext = "You have no more votes remaining on this bug.";
            }

            # Notice that we did not make sure that the user fit within the $votesperuser
            # range.  This is considered to be an acceptable alternative to losing votes
            # during product moves.  Then next time the user attempts to change their votes,
            # they will be forced to fit within the $votesperuser limit.

            # Now lets send the e-mail to alert the user to the fact that their votes have
            # been reduced or removed.
            my $vars = {

                'to' => $name . Bugzilla->params->{'emailsuffix'},
                'bugid' => $id,
                'reason' => $reason,

                'votesremoved' => $removedvotes,
                'votesold' => $oldvotes,
                'votesnew' => $newvotes,

                'votesremovedtext' => $removedvotestext,
                'votesoldtext' => $oldvotestext,
                'votesnewtext' => $newvotestext,

                'count' => $removedvotes . "\n    " . $newvotestext
            };

            my $msg;
            my $template = Bugzilla->template;
            $template->process("email/votes-removed.txt.tmpl", $vars, \$msg);
            push(@messages, $msg);
        }
        my $votes = $dbh->selectrow_array("SELECT SUM(vote_count) " .
                                          "FROM votes WHERE bug_id = ?",
                                          undef, $id) || 0;
        $dbh->do("UPDATE bugs SET votes = ? WHERE bug_id = ?",
                 undef, ($votes, $id));
    }
    # Now return the array containing emails to be sent.
    return \@messages;
}

# If a user votes for a bug, or the number of votes required to
# confirm a bug has been reduced, check if the bug is now confirmed.
sub CheckIfVotedConfirmed {
    my ($id, $who) = (@_);
    my $dbh = Bugzilla->dbh;

    my ($votes, $status, $everconfirmed, $votestoconfirm, $timestamp) =
        $dbh->selectrow_array("SELECT votes, bug_status, everconfirmed, " .
                              "       votestoconfirm, NOW() " .
                              "FROM bugs INNER JOIN products " .
                              "                  ON products.id = bugs.product_id " .
                              "WHERE bugs.bug_id = ?",
                              undef, $id);

    my $ret = 0;
    if ($votes >= $votestoconfirm && !$everconfirmed) {
        if ($status eq 'UNCONFIRMED') {
            my $fieldid = get_field_id("bug_status");
            $dbh->do("UPDATE bugs SET bug_status = 'NEW', everconfirmed = 1, " .
                     "delta_ts = ? WHERE bug_id = ?",
                     undef, ($timestamp, $id));
            $dbh->do("INSERT INTO bugs_activity " .
                     "(bug_id, who, bug_when, fieldid, removed, added) " .
                     "VALUES (?, ?, ?, ?, ?, ?)",
                     undef, ($id, $who, $timestamp, $fieldid, 'UNCONFIRMED', 'NEW'));
        }
        else {
            $dbh->do("UPDATE bugs SET everconfirmed = 1, delta_ts = ? " .
                     "WHERE bug_id = ?", undef, ($timestamp, $id));
        }

        my $fieldid = get_field_id("everconfirmed");
        $dbh->do("INSERT INTO bugs_activity " .
                 "(bug_id, who, bug_when, fieldid, removed, added) " .
                 "VALUES (?, ?, ?, ?, ?, ?)",
                 undef, ($id, $who, $timestamp, $fieldid, '0', '1'));

        AppendComment($id, $who,
                      "*** This bug has been confirmed by popular vote. ***",
                      0, $timestamp);

        $ret = 1;
    }
    return $ret;
}

################################################################################
# check_can_change_field() defines what users are allowed to change. You
# can add code here for site-specific policy changes, according to the
# instructions given in the Bugzilla Guide and below. Note that you may also
# have to update the Bugzilla::Bug::user() function to give people access to the
# options that they are permitted to change.
#
# check_can_change_field() returns true if the user is allowed to change this
# field, and false if they are not.
#
# The parameters to this method are as follows:
# $field    - name of the field in the bugs table the user is trying to change
# $oldvalue - what they are changing it from
# $newvalue - what they are changing it to
# $PrivilegesRequired - return the reason of the failure, if any
# $data     - hash containing relevant parameters, e.g. from the CGI object
################################################################################
sub check_can_change_field {
    my $self = shift;
    my ($field, $oldvalue, $newvalue, $PrivilegesRequired, $data) = (@_);
    my $user = Bugzilla->user;

    $oldvalue = defined($oldvalue) ? $oldvalue : '';
    $newvalue = defined($newvalue) ? $newvalue : '';

    # Return true if they haven't changed this field at all.
    if ($oldvalue eq $newvalue) {
        return 1;
    } elsif (trim($oldvalue) eq trim($newvalue)) {
        return 1;
    # numeric fields need to be compared using ==
    } elsif (($field eq 'estimated_time' || $field eq 'remaining_time')
             && $newvalue ne $data->{'dontchange'}
             && $oldvalue == $newvalue)
    {
        return 1;
    }

    # Allow anyone to change comments.
    if ($field =~ /^longdesc/) {
        return 1;
    }

    # Ignore the assigned_to field if the bug is not being reassigned
    if ($field eq 'assigned_to'
        && $data->{'knob'} ne 'reassignbycomponent'
        && $data->{'knob'} ne 'reassign')
    {
        return 1;
    }

    # If the user isn't allowed to change a field, we must tell him who can.
    # We store the required permission set into the $PrivilegesRequired
    # variable which gets passed to the error template.
    #
    # $PrivilegesRequired = 0 : no privileges required;
    # $PrivilegesRequired = 1 : the reporter, assignee or an empowered user;
    # $PrivilegesRequired = 2 : the assignee or an empowered user;
    # $PrivilegesRequired = 3 : an empowered user.

    # Allow anyone with "editbugs" privs to change anything.
    if ($user->in_group('editbugs')) {
        return 1;
    }

    # *Only* users with "canconfirm" privs can confirm bugs.
    if ($field eq 'canconfirm'
        || ($field eq 'bug_status'
            && $oldvalue eq 'UNCONFIRMED'
            && is_open_state($newvalue)))
    {
        $PrivilegesRequired = 3;
        return $user->in_group('canconfirm');
    }

    # Make sure that a valid bug ID has been given.
    if (!$self->{'error'}) {
        # Allow the assignee to change anything else.
        if ($self->{'assigned_to_id'} == $user->id) {
            return 1;
        }

        # Allow the QA contact to change anything else.
        if (Bugzilla->params->{'useqacontact'}
            && $self->{'qa_contact_id'}
            && ($self->{'qa_contact_id'} == $user->id))
        {
            return 1;
        }
    }

    # At this point, the user is either the reporter or an
    # unprivileged user. We first check for fields the reporter
    # is not allowed to change.

    # The reporter may not:
    # - reassign bugs, unless the bugs are assigned to him;
    #   in that case we will have already returned 1 above
    #   when checking for the assignee of the bug.
    if ($field eq 'assigned_to') {
        $PrivilegesRequired = 2;
        return 0;
    }
    # - change the QA contact
    if ($field eq 'qa_contact') {
        $PrivilegesRequired = 2;
        return 0;
    }
    # - change the target milestone
    if ($field eq 'target_milestone') {
        $PrivilegesRequired = 2;
        return 0;
    }
    # - change the priority (unless he could have set it originally)
    if ($field eq 'priority'
        && !Bugzilla->params->{'letsubmitterchoosepriority'})
    {
        $PrivilegesRequired = 2;
        return 0;
    }

    # The reporter is allowed to change anything else.
    if (!$self->{'error'} && $self->{'reporter_id'} == $user->id) {
        return 1;
    }

    # If we haven't returned by this point, then the user doesn't
    # have the necessary permissions to change this field.
    $PrivilegesRequired = 1;
    return 0;
}

#
# Field Validation
#

# Validates and verifies a bug ID, making sure the number is a 
# positive integer, that it represents an existing bug in the
# database, and that the user is authorized to access that bug.
# We detaint the number here, too.
sub ValidateBugID {
    my ($id, $field) = @_;
    my $dbh = Bugzilla->dbh;
    my $user = Bugzilla->user;

    # Get rid of leading '#' (number) mark, if present.
    $id =~ s/^\s*#//;
    # Remove whitespace
    $id = trim($id);

    # If the ID isn't a number, it might be an alias, so try to convert it.
    my $alias = $id;
    if (!detaint_natural($id)) {
        $id = bug_alias_to_id($alias);
        $id || ThrowUserError("invalid_bug_id_or_alias",
                              {'bug_id' => $alias,
                               'field'  => $field });
    }
    
    # Modify the calling code's original variable to contain the trimmed,
    # converted-from-alias ID.
    $_[0] = $id;
    
    # First check that the bug exists
    $dbh->selectrow_array("SELECT bug_id FROM bugs WHERE bug_id = ?", undef, $id)
      || ThrowUserError("invalid_bug_id_non_existent", {'bug_id' => $id});

    return if (defined $field && ($field eq "dependson" || $field eq "blocked"));
    
    return if $user->can_see_bug($id);

    # The user did not pass any of the authorization tests, which means they
    # are not authorized to see the bug.  Display an error and stop execution.
    # The error the user sees depends on whether or not they are logged in
    # (i.e. $user->id contains the user's positive integer ID).
    if ($user->id) {
        ThrowUserError("bug_access_denied", {'bug_id' => $id});
    } else {
        ThrowUserError("bug_access_query", {'bug_id' => $id});
    }
}

# ValidateBugAlias:
#   Check that the bug alias is valid and not used by another bug.  If 
#   curr_id is specified, verify the alias is not used for any other
#   bug id.  
sub ValidateBugAlias {
    my ($alias, $curr_id) = @_;
    my $dbh = Bugzilla->dbh;

    $alias = trim($alias || "");
    trick_taint($alias);

    if ($alias eq "") {
        ThrowUserError("alias_not_defined");
    }

    # Make sure the alias isn't too long.
    if (length($alias) > 20) {
        ThrowUserError("alias_too_long");
    }

    # Make sure the alias is unique.
    my $query = "SELECT bug_id FROM bugs WHERE alias = ?";
    if ($curr_id && detaint_natural($curr_id)) {
        $query .= " AND bug_id != $curr_id";
    }
    my $id = $dbh->selectrow_array($query, undef, $alias); 

    my $vars = {};
    $vars->{'alias'} = $alias;
    if ($id) {
        $vars->{'bug_id'} = $id;
        ThrowUserError("alias_in_use", $vars);
    }

    # Make sure the alias isn't just a number.
    if ($alias =~ /^\d+$/) {
        ThrowUserError("alias_is_numeric", $vars);
    }

    # Make sure the alias has no commas or spaces.
    if ($alias =~ /[, ]/) {
        ThrowUserError("alias_has_comma_or_space", $vars);
    }

    $_[0] = $alias;
}

# Validate and return a hash of dependencies
sub ValidateDependencies {
    my $fields = {};
    $fields->{'dependson'} = shift;
    $fields->{'blocked'} = shift;
    my $id = shift || 0;

    unless (defined($fields->{'dependson'})
            || defined($fields->{'blocked'}))
    {
        return;
    }

    my $dbh = Bugzilla->dbh;
    my %deps;
    my %deptree;
    foreach my $pair (["blocked", "dependson"], ["dependson", "blocked"]) {
        my ($me, $target) = @{$pair};
        $deptree{$target} = [];
        $deps{$target} = [];
        next unless $fields->{$target};

        my %seen;
        foreach my $i (split('[\s,]+', $fields->{$target})) {
            if ($id == $i) {
                ThrowUserError("dependency_loop_single");
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
            my $dep_list =
                $dbh->selectcol_arrayref("SELECT $target
                                          FROM dependencies
                                          WHERE $me = ?", undef, $i);
            foreach my $t (@$dep_list) {
                # ignore any _current_ dependencies involving this bug,
                # as they will be overwritten with data from the form.
                if ($t != $id && !exists $seen{$t}) {
                    push(@{$deptree{$target}}, $t);
                    push @stack, $t;
                    $seen{$t} = 1;
                }
            }
        }
    }

    my @deps   = @{$deptree{'dependson'}};
    my @blocks = @{$deptree{'blocked'}};
    my %union = ();
    my %isect = ();
    foreach my $b (@deps, @blocks) { $union{$b}++ && $isect{$b}++ }
    my @isect = keys %isect;
    if (scalar(@isect) > 0) {
        ThrowUserError("dependency_loop_multi", {'deps' => \@isect});
    }
    return %deps;
}


#####################################################################
# Autoloaded Accessors
#####################################################################

# Determines whether an attribute access trapped by the AUTOLOAD function
# is for a valid bug attribute.  Bug attributes are properties and methods
# predefined by this module as well as bug fields for which an accessor
# can be defined by AUTOLOAD at runtime when the accessor is first accessed.
#
# XXX Strangely, some predefined attributes are on the list, but others aren't,
# and the original code didn't specify why that is.  Presumably the only
# attributes that need to be on this list are those that aren't predefined;
# we should verify that and update the list accordingly.
#
sub _validate_attribute {
    my ($attribute) = @_;

    my @valid_attributes = (
        # Miscellaneous properties and methods.
        qw(error groups
           longdescs milestoneurl attachments
           isopened isunconfirmed
           flag_types num_attachment_flag_types
           show_attachment_flags any_flags_requesteeble),

        # Bug fields.
        Bugzilla::Bug->fields
    );

    return grep($attribute eq $_, @valid_attributes) ? 1 : 0;
}

sub AUTOLOAD {
  use vars qw($AUTOLOAD);
  my $attr = $AUTOLOAD;

  $attr =~ s/.*:://;
  return unless $attr=~ /[^A-Z]/;
  confess("invalid bug attribute $attr") unless _validate_attribute($attr);

  no strict 'refs';
  *$AUTOLOAD = sub {
      my $self = shift;
      if (defined $self->{$attr}) {
          return $self->{$attr};
      } else {
          return '';
      }
  };

  goto &$AUTOLOAD;
}

1;
