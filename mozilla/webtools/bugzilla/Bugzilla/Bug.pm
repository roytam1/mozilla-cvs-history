#!/usr/bonsaitools/bin/perl -w
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

use diagnostics;
use strict;

use DBI;
use RelationSet;
require "globals.pl";
require "CGI.pl";
package Bug;
use CGI::Carp qw(fatalsToBrowser);
my %ok_fields;

# ok_fields is a quick hash of all the fields that can be touched by outsiders.
# 
for my $key (qw (product version rep_platform op_sys bug_status resolution
                priority bug_serverity component assigned_to reporter short_desc
                target_milestone qa_contact status_whiteboard dependson blocking
                attachments keywords comment bug_file_loc) ) {
    $ok_fields{$key}++;
    }


# create a new empty bug
#
sub new {
  my $type = shift();
  my $class = ref($type) || $type;
  my %bug;
  my $self = {};

  # create a ref to an empty hash and bless it
  #
     %bug = GetBug(@_);
     if (defined($bug{'error'})) {
         confess($bug{'error'});
     }
     $self = {%bug};
     bless $self, $type;

  # bless as a Bug
  #
  return $self;
}

# dump info about bug into hash unless user doesn't have permission
# user_id 0 is used when person is not logged in.
#
sub GetBug  {
  my ($bug_id, $user_id) = (@_);
  my %bug;

  if ( (! defined $bug_id) || (!$bug_id) ) {
    # no bug number given
    $bug{'error'} = 'No bug number given!';
    return %bug;
  }

# default userid 0, or get DBID if you used an email address
  unless (defined $user_id) {
    $user_id = 0;
  }
  else {
     if ($user_id =~ /^\@/) {
	$user_id = &::DBname_to_id($user_id); 
     }
  }
     
  &::ConnectToDatabase();
  &::GetVersionTable();

  $bug{'whoid'} = $user_id;
  &::SendSQL("SELECT groupset FROM profiles WHERE userid=$bug{'whoid'}");
  my $usergroupset = &::FetchOneColumn();
  $bug{'usergroupset'} = $usergroupset;

  my $query = "
    select
      bugs.bug_id, product, version, rep_platform, op_sys, bug_status,
      resolution, priority, bug_severity, component, assigned_to, reporter,
      bug_file_loc, short_desc, target_milestone, qa_contact,
      status_whiteboard, date_format(creation_ts,'%Y-%m-%d %H:%i'),
      groupset, delta_ts, sum(votes.count)
    from bugs left join votes using(bug_id)
    where bugs.bug_id = $bug_id
    and bugs.groupset & $usergroupset = bugs.groupset
    group by bugs.bug_id";

  &::SendSQL($query);
  my @row;

  if (@row = &::FetchSQLData()) {
    my $count = 0;
    my %fields;
    foreach my $field ("bug_id", "product", "version", "rep_platform",
                       "op_sys", "bug_status", "resolution", "priority",
                       "bug_severity", "component", "assigned_to", "reporter",
                       "bug_file_loc", "short_desc", "target_milestone",
                       "qa_contact", "status_whiteboard", "creation_ts",
                       "groupset", "delta_ts", "votes") {
	$fields{$field} = shift @row;
	if ($fields{$field}) {
	    $bug{$field} = $fields{$field};
	}
	$count++;
    }
  } else {
    &::SendSQL("select groupset from bugs where bug_id = $bug_id");
    if (@row = &::FetchSQLData()) {
      $bug{'bug_id'} = $bug_id;
      $bug{'error'} = "NotPermitted";
      return %bug;
    } else {
      $bug{'bug_id'} = $bug_id;
      $bug{'error'} = "Bug #$bug_id not found in database.";
      return %bug;
    }
  }

  $bug{'assigned_to'} = &::DBID_to_name($bug{'assigned_to'});
  $bug{'reporter'} = &::DBID_to_name($bug{'reporter'});

  my $ccSet = new RelationSet;
  $ccSet->mergeFromDB("select who from cc where bug_id=$bug_id");
  my @cc = $ccSet->toArrayOfStrings();
  if (@cc) {
    $bug{'cc'} = \@cc;
  }

  if (&::Param("useqacontact") && (defined $bug{'qa_contact'}) ) {
    my $name = $bug{'qa_contact'} > 0 ? &::DBID_to_name($bug{'qa_contact'}) :"";
    if ($name) {
      $bug{'qa_contact'} = $name;
    }
  }

  if (@::legal_keywords) {
    &::SendSQL("SELECT keyworddefs.name 
              FROM keyworddefs, keywords
             WHERE keywords.bug_id = $bug_id 
               AND keyworddefs.id = keywords.keywordid
          ORDER BY keyworddefs.name");
    my @keywordlist;
    while (&::MoreSQLData()) {
        push(@keywordlist, &::FetchOneColumn());
    }
    if (@keywordlist) {
      $bug{'keywords'} = \@keywordlist;
    }
  }

  &::SendSQL("select attach_id, creation_ts, description 
           from attachments 
           where bug_id = $bug_id");
  my @attachments;
  while (&::MoreSQLData()) {
    my ($attachid, $date, $desc) = (&::FetchSQLData());
    if ($date =~ /^(\d\d)(\d\d)(\d\d)(\d\d)(\d\d)(\d\d)(\d\d)$/) {
        $date = "$3/$4/$2 $5:$6";
      my %attach;
      $attach{'attachid'} = $attachid;
      $attach{'date'} = $date;
      $attach{'desc'} = $desc;
      push @attachments, \%attach;
    }
  }
  if (@attachments) {
    $bug{'attachments'} = \@attachments;
  }

  &::SendSQL("select bug_id, who, bug_when
           from longdescs 
           where bug_id = $bug_id order by bug_when");
  my @longdescs;
  while (&::MoreSQLData()) {
    my ($bug_id, $who, $bug_when) = (&::FetchSQLData());
    my %longdesc;
    $longdesc{'who'} = $who;
    $longdesc{'bug_when'} = $bug_when;
    push @longdescs, \%longdesc;
  }
  if (@longdescs) {
    $bug{'longdescs'} = \@longdescs;
  }

  if (&::Param("usedependencies")) {
    my @depends = EmitDependList("blocked", "dependson", $bug_id);
    if ( @depends ) {
      $bug{'dependson'} = \@depends;
    }
    my @blocks = EmitDependList("dependson", "blocked", $bug_id);
    if ( @blocks ) {
      $bug{'blocks'} = \@blocks;
    }
  }
  return %bug;
}

# given a bug hash, emit xml for it. with file header provided by caller
#
sub emitXML {
  ( $#_ == 0 ) || confess("invalid number of arguments");
  my $self = shift();
  my $xml;


  if (exists $self->{'error'}) {
    $xml .= "<bug error=\"$self->{'error'}\">\n";
    $xml .= "  <bug_id>$self->{'bug_id'}</bug_id>\n";
    $xml .= "</bug>\n";
    return $xml;
  }

  $xml .= "<bug>\n";

  foreach my $field ("bug_id", "urlbase", "bug_status", "product",
      "priority", "version", "rep_platform", "assigned_to", "delta_ts", 
      "component", "reporter", "target_milestone", "bug_severity", 
      "creation_ts", "qa_contact", "op_sys", "resolution", "bug_file_loc",
      "short_desc", "keywords", "status_whiteboard") {
    if ($self->{$field}) {
      $xml .= "  <$field>" . $self->{$field} . "</$field>\n";
    }
  }

  foreach my $field ("dependson", "blocks", "cc") {
    if (defined $self->{$field}) {
      for (my $i=0 ; $i < @{$self->{$field}} ; $i++) {
        $xml .= "  <$field>" . $self->{$field}[$i] . "</$field>\n";
      }
    }
  }

  if (defined $self->{'longdescs'}) {
    for (my $i=0 ; $i < @{$self->{'longdescs'}} ; $i++) {
      $xml .= "  <long_desc>\n"; 
      $xml .= "   <who>" . &::DBID_to_name($self->{'longdescs'}[$i]->{'who'}) 
                         . "</who>\n"; 
      $xml .= "   <bug_when>" . $self->{'longdescs'}[$i]->{'bug_when'} 
                              . "</bug_when>\n"; 
      $xml .= "   <thetext>" . QuoteXMLChars($self->{'longdescs'}[$i]->{'thetext'})
                             . "</thetext>\n"; 
      $xml .= "  </long_desc>\n"; 
    }
  }

  if (defined $self->{'attachments'}) {
    for (my $i=0 ; $i < @{$self->{'attachments'}} ; $i++) {
      $xml .= "  <attachment>\n"; 
      $xml .= "    <attachid>" . $self->{'attachments'}[$i]->{'attachid'}
                              . "</attachid>\n"; 
      $xml .= "    <date>" . $self->{'attachments'}[$i]->{'date'} . "</date>\n"; 
      $xml .= "    <desc>" . $self->{'attachments'}[$i]->{'desc'} . "</desc>\n"; 
    # $xml .= "    <type>" . $self->{'attachments'}[$i]->{'type'} . "</type>\n"; 
    # $xml .= "    <data>" . $self->{'attachments'}[$i]->{'data'} . "</data>\n"; 
      $xml .= "  </attachment>\n"; 
    }
  }

  $xml .= "</bug>\n";
  return $xml;
}

sub EmitDependList {
  my ($myfield, $targetfield, $bug_id) = (@_);
  my @list;
  &::SendSQL("select dependencies.$targetfield
           from dependencies
           where dependencies.$myfield = $bug_id
           order by dependencies.$targetfield;");
  while (&::MoreSQLData()) {
    my ($i, $stat) = (&::FetchSQLData());
    push @list, $i;
  }
  return @list;
}

sub QuoteXMLChars {
  $_[0] =~ s/</&lt;/g;
  $_[0] =~ s/>/&gt;/g;
  $_[0] =~ s/'/&apos;/g;
  $_[0] =~ s/"/&quot;/g;
  $_[0] =~ s/&/&amp;/g;
# $_[0] =~ s/([\x80-\xFF])/&XmlUtf8Encode(ord($1))/ge;
  return($_[0]);
}

sub XML_Header {
  my ($urlbase, $version, $maintainer, $exporter) = (@_);

  my $xml;
  $xml = "<?xml version=\"1.0\" standalone=\"no\"?>\n";
  $xml .= "<!DOCTYPE bugzilla SYSTEM \"$urlbase";
  if (! ($urlbase =~ /.+\/$/)) {
    $xml .= "/";
  }
  $xml .= "bugzilla.dtd\">\n";
  $xml .= "<bugzilla";
  if (defined $exporter) {
    $xml .= " exporter=\"$exporter\"";
  }
  $xml .= " version=\"$version\"";
  $xml .= " urlbase=\"$urlbase\"";
  $xml .= " maintainer=\"$maintainer\">\n";
  return ($xml);
}


sub XML_Footer {
  return ("</bugzilla>\n");
}

sub UserInGroup {
    my $self = shift();
    my ($groupname) = (@_);
    if ($self->{'usergroupset'} eq "0") {
        return 0;
    }
    &::ConnectToDatabase();
    &::SendSQL("select (bit & $self->{'usergroupset'}) != 0 from groups where name = " 
           . &::SqlQuote($groupname));
    my $bit = &::FetchOneColumn();
    if ($bit) {
        return 1;
    }
    return 0;
}

sub CheckCanChangeField {
   my $self = shift;
   my ($f, $oldvalue, $newvalue) = @_;
   my $UserInEditGroupSet = -1;
   my $UserInCanConfirmGroupSet = -1;
   my $ownerid;
   my $reporterid;
   my $qacontactid;

   if (!defined($oldvalue)) {
      $oldvalue = '';
   }

   print "$f old=$oldvalue new=$newvalue\n";
    if ($f eq "assigned_to" || $f eq "reporter" || $f eq "qa_contact") {
        if ($oldvalue =~ /^\d+$/) {
            if ($oldvalue == 0) {
                $oldvalue = "";
            } else {
                $oldvalue = &::DBID_to_name($oldvalue);
            }
        }
    }
    if ($oldvalue eq $newvalue) {
        return 1;
    }
    if (&::trim($oldvalue) eq &::trim($newvalue)) {
        return 1;
    }
    if ($f =~ /^longdesc/) {
        return 1;
    }
    if ($UserInEditGroupSet < 0) {
        $UserInEditGroupSet = UserInGroup($self, "editbugs");
    }
    if ($UserInEditGroupSet) {
        return 1;
    }
    &::SendSQL("SELECT reporter, assigned_to, qa_contact FROM bugs " .
                "WHERE bug_id = $self->{'bug_id'}");
    ($reporterid, $ownerid, $qacontactid) = (&::FetchSQLData());

    # Let reporter change bug status, even if they can't edit bugs.
    # If reporter can't re-open their bug they will just file a duplicate.
    # While we're at it, let them close their own bugs as well.
    if ( ($f eq "bug_status") && ($self->{'whoid'} eq $reporterid) ) {
        return 1;
    }
    if ($f eq "bug_status" && $newvalue ne $::unconfirmedstate &&
        &::IsOpenedState($newvalue)) {

        # Hmm.  They are trying to set this bug to some opened state
        # that isn't the UNCONFIRMED state.  Are they in the right
        # group?  Or, has it ever been confirmed?  If not, then this
        # isn't legal.

        if ($UserInCanConfirmGroupSet < 0) {
            $UserInCanConfirmGroupSet = &::UserInGroup("canconfirm");
        }
        if ($UserInCanConfirmGroupSet) {
            return 1;
        }
        &::SendSQL("SELECT everconfirmed FROM bugs WHERE bug_id = $self->{'bug_id'}");
        my $everconfirmed = FetchOneColumn();
        if ($everconfirmed) {
            return 1;
        }
    } elsif ($reporterid eq $self->{'whoid'} || $ownerid eq $self->{'whoid'} ||
             $qacontactid eq $self->{'whoid'}) {
        return 1;
    }
    PushError($self, "
Only the owner or submitter of the bug, or a sufficiently
empowered user, may make that change to the $f field.");
    return 0;
}

sub LockDatabase {
    my $write = "WRITE";        # Might want to make a param to control
                                # whether we do LOW_PRIORITY ...
    &::SendSQL("LOCK TABLES bugs $write, bugs_activity $write, cc $write, " .
            "profiles $write, dependencies $write, votes $write, " .
            "keywords $write, longdescs $write, fielddefs $write, " .
            "keyworddefs READ, groups READ, attachments READ, products READ");
}

sub UnlockDatabase {
    &::SendSQL("UNLOCK TABLES");
}

sub PushError {
   my $self = shift;
   my ($error) = @_;
   my @emptylist = ();
  
   if (!defined($self->{'error'})) {
       $self->{'error'} = \@emptylist;
   }
   push (@{$self->{'error'}}, $error);
}

sub PrintErrors {
   my $self = shift;

   foreach my $err (@{$self->{'error'}}) {
       print $err . "\n";
   }
}

sub DeleteErrors {
   my $self = shift;

   delete $self->{'errors'};
   return 1;
}

sub SetDirtyFlag {
   my $self = shift;

   $self->{'dirty'} = 1;
   return 1;
}

sub SetComment {
   my $self = shift;
   my ($comment) = (@_);
   $comment =~ s/\r\n/\n/g;     # Get rid of windows-style line endings.
   $comment =~ s/\r/\n/g;       # Get rid of mac-style line endings.
   if ($comment =~ /^\s*$/) {  # Nothin' but whitespace.
       $self->{'comment'} = "";
       return;
   }
   $self->{'comment'} = $comment;
   SetDirtyFlag($self);
} 
   

#from o'reilley's Programming Perl
sub display {
    my $self = shift;
    my @keys;
    if (@_ == 0) {                  # no further arguments
        @keys = sort keys(%$self);
    }  else {
        @keys = @_;                 # use the ones given
    }
    foreach my $key (@keys) {
        print "\t$key => $self->{$key}\n";
    }
}

sub ShowQuery {
    my $self = shift;
    print $self->{'query'} . "\n";
}


# check and see if a given field exists, is non-empty, and is set to a
# legal value. 
# if $legalsRef is not passed, just check to make sure the value exists and
# is non-NULL
#
sub CheckField {
    my ($newvalue,                # the value to check
        $fieldname,              # the fieldname to check
        $legalsRef               # (optional) ref to a list of legal values
       ) = @_;

    if (defined($legalsRef) &&
          (&::lsearch($legalsRef, $newvalue)<0)) {
        return 0;
    }
    else {
        return 1;
    }
}

sub SnapShotBugInDB {
    my $self = shift;
    my $bugid;
    my $who;
    my %snapshot;

    $bugid = $self->{'bug_id'};
    $who = $self->{'whoid'};
    %snapshot = GetBug($bugid, $who);
    return %snapshot;
}

sub DoConfirm {
    my $self = shift;
    my $UserInEditGroupSet = -1;
    my $UserInCanConfirmGroupSet = -1;
    if ($UserInEditGroupSet < 0) {
        $UserInEditGroupSet = &::UserInGroup($self, "editbugs");
    }
    if ($UserInCanConfirmGroupSet < 0) {
        $UserInCanConfirmGroupSet = &::UserInGroup($self, "canconfirm");
    }
    if ($UserInEditGroupSet || $UserInCanConfirmGroupSet) {
        $self->{'everconfirmed'} = "everconfirmed = 1";
    }
}


sub BugExists {
   my ($bugid) = (@_);

   &::SendSQL("SELECT bug_id FROM bugs WHERE bug_id=$bugid");
   return $::FetchOneColumn();
}


sub SetDependsOn {
   my $self = shift;
   my (@list) = (@_);

   foreach my $bug (@list) {
      unless (BugExists($bug)) {
          PushError($self, "Bug number $bug doesn\'t exist!"); 
          return 0;
      }
   }
   $self->{'dependson'} = \@list;
   SetDirtyFlag($self);
   return 1;
}


sub SetBlocking {
   my $self = shift;
   my (@list) = (@_);

   foreach my $bug (@list) {
      unless (BugExists($bug)) {
          $self->{'error'} = "Bug number $bug doesn\'t exist!";
          return 0;
      }
   }
   $self->{'blocking'} = \@list;
   SetDirtyFlag($self);
   return 1;
}

sub SetKeywords {
   my $self = shift;
   my (@keywords) = (@_);
   my $okay;

   foreach my $keyword (@keywords) {
      $okay = CheckField($self, 'keyword', \@::legal_keywords);
      if (!$okay) {
          PushError($self,"Invalid keyword \'$keyword!\'");
          return 0; 
      }
   }
   $self->{'keywords'} = \@keywords;
   SetDirtyFlag($self);
   return 1;
}

sub SetProductComponent {
   my $self = shift;
   my ($product, $component) = (@_);
   my $okay;
   
   unless (CheckField($self, 'product', @::legal_product)) {
     $self->{'error'} = "Invalid product \'$product\'";
     return 0;
   }
   &::SendSQL("SELECT value FROM components WHERE value=$component AND program=$product");
   $okay = &::FetchOneColumn();
   if ($okay) {
      $self->{'product'} = $product;
      $self->{'component'} = $component;
      SetDirtyFlag($self);
      return 1;
   }
   else {
     PushError($self,"Invalid component \'$component\'");
     return 0;
   }
}

sub SetVersion {
   my $self = shift;
   my ($version) = (@_);
   
   unless (CheckField('version', \@::legal_versions)) {
     PushError($self, "Invalid version \'$version\'");
     return 0;
   }
   unless(CheckCanChangeField($self, 'version', $self->{'version'},
                              $version)) {
      return 0;
   }
   $self->{'version'} = $version;
   SetDirtyFlag($self);
   return 1;
}

sub SetPlatform {
   my $self = shift;
   my ($platform) = (@_);
  
   unless (CheckField($platform, 'rep_platform', \@::legal_platform)) {
     PushError($self, "Invalid platform \'$platform\'");
     return 0;
   }
   unless(CheckCanChangeField($self, 'rep_platform', $self->{'rep_platform'},
                              $platform)) {
      return 0;
   }
   $self->{'rep_platform'} = "$platform";
   return 1;
}

sub SetOperatingSystem {
   my $self = shift;
   my ($os) = @_;
   
   unless (CheckField($os, 'op_sys', \@::legal_opsys)) {
     PushError($self, "Invalid operating system \'$os\'");
     return 0;
   }
   unless(CheckCanChangeField($self, 'op_sys', $self->{'op_sys'},
                              $os)) {
      return 0;
   }
   $self->{'op_sys'} = $os;
   SetDirtyFlag($self);
   return 1;
}

sub SetPriority {
   my $self = shift;
   my ($priority) = @_;

   unless (CheckField($priority, 'priority', \@::legal_priority)) {
     PushError($self,"Invalid priority \'$priority\'");
     return 0;
   }
   unless(CheckCanChangeField($self, 'priority', $self->{'priority'},
                              $priority)) {
      return 0;
   }
   $self->{'priority'} = $priority;
   SetDirtyFlag($self);
   return 1;
}

sub SetSeverity {
   my $self = shift;
   my ($severity) = (@_);
   
   unless (CheckField('bug_severity', \@::legal_severity)) {
      PushError($self, "Invalid bug severity \'$severity\'");
      return 0;
   }
   unless(CheckCanChangeField($self, 'bug_severity', $self->{'bug_severity'},
                              $severity)) {
      return 0;
   }
   $self->{'bug_severity'} = $severity;
   SetDirtyFlag($self);
   return 1;
}

sub CheckUserExists {
  my $self = shift;
  my ($user) = (@_);
  my $user_id;

     if ($user =~ /^\@/) {
        $user_id = &::DBname_to_id($user);
         unless ($user_id) {
           PushError($self, "No such user \'$user\'");
           return 0;
        }
        return $user_id;
     }
     else {
       &::SendSQL("SELECT userid from profiles where userid=$user");
       my $okay = &::FetchOneColumn();
       unless ($okay) {
           PushError($self, "No such user \'$user\'");
           return 0;
       } 
       return $user_id;
     }
}

sub SetAssignedTo {
   my $self = shift;
   my ($assignee) = (@_);
    
   my $id = CheckUserExists($assignee);
   unless(CheckCanChangeField($self, 'assigned_to', $self->{'assigned_to'},
                              $assignee)) {
      return 0;
   }
   if ($id) {
      $self->{'assigned_to'} = $assignee;
   }
   else {
      PushError($self, "No such userid \'$assignee\'");
      return 0;
   }
}

sub SetShortDescription {
   my $self = shift;
   my ($desc) = (@_);

   unless(CheckCanChangeField($self, 'short_desc', $self->{'short_desc'}, $desc)) {
      return 0;
   }
   $self->{'short_desc'} = $desc;
   SetDirtyFlag($self);
   return 1;
}


sub SetTargetMilestone {
   my $self = shift;
   my ($stone) = @_;

   if (&::Param('usetargetmilestone')) {
       unless (CheckField($self, 'target_milestone', @::legal_target_milestone)) {
          PushError($self, "Invalid target milestone \'$stone\'");
          return 0;
       }
       unless(CheckCanChangeField($self, 'target_milestone', 
              $self->{'target_milestone'}, $stone)) {
          return 0;
       }    
      $self->{'target_milestone'} = $stone;
      SetDirtyFlag($self);
      return 1;
   }
   else {
       PushError($self, "Target Milestones not enabled at this installation.");
       return 0;
   }
}

sub SetQAContact {
   my $self = shift;
   my ($contact) = (@_);

   if (&::Param('useqacontact')) {
       my $id = CheckUserExists($contact);
       unless(CheckCanChangeField($self, 'qa_contact', $self->{'qa_contact'},
                                  $contact)) {
          return 0;
       }
       if ($id) {
          $self->{'qa_contact'} = $contact;
          SetDirtyFlag($self);
       }
       else {
           PushError($self, "No such userid \'$contact\'");
           return 0;
       }
   }
   else {
       PushError($self, "useqacontact not enabled at this installation.");
       return 0;
   }
}

sub SetStatusWhiteboard {
   my $self = shift;
   my ($white) = @_;

   if (&::Param('usestatuswhiteboard')) {
       unless(CheckCanChangeField($self, 'status_whiteboard',
                  $self->{'status_whiteboard'}, $white)) {
           return 0;
       }
       $self->{'status_whiteboard'} = $white;
       SetDirtyFlag($self);
       return 1;
   }
   else {
       PushError($self, "usestatuswhiteboard not enabled at this installation.");
       return 0;
   }

}

sub SetURL {
   my $self = shift;
   my ($url) = @_;

   unless(CheckCanChangeField($self, 'bug_file_loc', $self->{'bug_file_loc'},
                               $url)) {
      return 0;
   }
   $self->{'bug_file_loc'} = $url;
   SetDirtyFlag($self);
   return 1;
}

sub SetStatus {
   my $self = shift;
   my ($status) = @_;

   unless(CheckField($status, 'bug_status', \@::legal_bug_status)) {
      PushError($self, "Invalid status \'$status\'");
      return 0;
   }
   $self->{'bug_status'} = $status;
   SetDirtyFlag($self);
   return 1; 
}

sub SetResolution {
   my $self = shift;
   my ($resolution) = @_;

   unless(CheckField($self, 'resolution', @::legal_resolution)) {
      PushError($self,"Invalid resolution \'$resolution\'");
      return 0;
   }
   $self->{'resolution'} = $resolution;
   SetDirtyFlag($self);
   return 1;
}

sub CheckonComment {
    my $self = shift;
    my ($function) = @_;

    # Param is 1 if comment should be added !
    my $ret = &::Param( "commenton" . $function );

    # Allow without comment in case of undefined Params.
    $ret = 0 unless ( defined( $ret ));

    if( $ret ) {
        if (!defined $self->{'comment'} || $self->{'comment'} =~ /^\s*$/) {
            # No comment - sorry, action not allowed !
            $ret = 1;
           PushError($self, "You have to specify a comment on this " .
                         "change.  Please give some words " .
                         "on the reason for your change.");
        } else {
            $ret = 0;
        }
    }
    return( ! $ret ); # Return val has to be inverted
}

sub MarkResolvedFixed {
    my $self = shift;

    if (CheckonCommment('resolve')) { 
      $self->{'bug_status'} = 'RESOLVED';
      $self->{'resolution'} = 'FIXED';
    }
    else {
       return 0;
    }

}

sub MarkResolvedInvalid {
    my $self = shift;
    
    $self->{'bug_status'} = 'RESOLVED';
    $self->{'resolution'} = 'INVALID';

}

sub MarkResolvedWontFix {
    my $self = shift;
    
    $self->{'bug_status'} =  'RESOLVED';
    $self->{'resolution'} = 'WONTFIX';

}

sub MarkResolvedLater {
    my $self = shift;
    
    $self->{'bug_status'} = 'RESOLVED';
    $self->{'resolution'} = 'LATER';

}

sub MarkResolvedRemind {
    my $self = shift;
    
    $self->{'bug_status'} = 'RESOLVED';
    $self->{'resolution'} = 'REMIND';

}

sub MarkResolvedWorksForMe {
    my $self = shift;
    
    $self->{'bug_status'} = 'RESOLVED';
    $self->{'resolution'} = 'WORKSFORME';

}

# stupid subroutine for checking if lists are equal.
sub ListDiff {
    my (@source, @dest, $type) = @_;

    if (@source != @dest) {
      return 1;
    }

# must be a list of strings or numbers. have at you!
    @source = sort(@source);
    @dest   = sort(@dest);

    my $i;
    for ($i = 0; $i < @source; $i++) {
      if ($type eq 'str') {
          if ($source[$i] ne $dest[$i]) {
              return 1;
          }
      }
      else {
          if ($source[$i] != $dest[$i]) {
              return 1;
          }
      }
    }
    return 0;
}

sub ChangedFields {
    my $self = shift;
    my (%snapshot) = @_;
    my @changed = ();

# first we find if any new hash key/values have been added to
# $self. if we do, we push them onto the changed list...

    foreach (keys %{$self}) {
       unless (exists $snapshot{$_}) {
           if ($ok_fields{$_}) {
               push(@changed, $_);
           } 
       }
    }

# next, we see if any of the common fields have changed...
    foreach my $field (keys(%snapshot)) {
        # we just punt for longdescs and attachments, since if a user
        # we currently don't allow for new attachments, and we use
        # the comment field for new comments.
        unless (($field eq 'longdescs') || ($field eq 'attachments')) { 
            if (&::Param("usedependencies")) {
                if (($field eq 'dependson') || ($field eq 'blocking')) {
                    if (ListDiff($self->{$field}, $snapshot{$field})) {
                        push(@changed, $field);
                    }
                }
            }
            if ($field eq 'keywords') {
                if (ListDiff($self->{$field}, $snapshot{$field}, 'str')) {
                   push(@changed, $field);
                }
            }
            if ($self->{$field} ne $snapshot{$field}) {
                push (@changed, $field);
            }
        }
    }
    return @changed;
}

sub TestChanged {
   my $self = shift();
   my %snappy;
   my @changed;

   %snappy = SnapShotBugInDB($self);
   @changed = ChangedFields($self, %snappy);

   foreach my $field (@changed) {
       print $field . " has changed\n";
   }
}

sub Collision {
   my $self = shift;
   my $delta_ts;
   my $id;

   $id = $self->{'bug_id'};

   &::SendSQL("SELECT delta_ts FROM bugs WHERE bug_id=$id");
   $delta_ts = &::FetchOneColumn;

   if ($delta_ts > $self->{'delta_ts'}) {
       PushError($self, "A collision has occurred: someone has made changes " .
                         "to bug $self->{'bug_id'} already.");
       return 1;   
   }
   return 0; 

}


sub WriteChanges {
   my $self = shift;
   my (@fields) = @_;
   my $sql;
   my $comma = "";

   $sql = "UPDATE bugs set\n";
#if it's something on the bug table, build onto a query
#else 
   foreach my $field (@fields) {
       if (($field eq 'dependson') || ($field eq 'blocks')) {
            #go into dependency hell
       }
       elsif ($field eq 'comment') {
         &::AppendComment($self->{'bug_id'}, $self->{'whoid'}, $self->{'comment'});
       }
       elsif ($field eq 'keywords') {



       }
       else {
          $sql .= "$comma $field=\'$self->{$field}\'";
          if ($comma eq "") {
              $comma = ", ";
          }
       }
   }
   print $sql . "\n";
   &::SendSQL($sql);
}

sub Commit {
   my $self = shift;
   my %snapshot;
   my @changed;

   print "committing\n"; 
   if (($self->{'dirty'}) && (defined($self->{'error'}))) {
       print "locking\n";
       LockDatabase();
       print "locked\n";
       unless (Collision($self)) {
           %snapshot = SnapShotBugInDB($self);
           @changed = ChangedFields($self, %snapshot); 
           if (@changed > 0) {
               print "changed fields\n";
               WriteChanges($self, @changed);
               UnlockDatabase();
#           DoMailNotification();
               delete $self->{'dirty'};
               return 1;
           }
       }
       else {
           UnlockDatabase();
           return 0;
       }
   }
}



sub AUTOLOAD {
  use vars qw($AUTOLOAD);
  my $self = shift;
  my $type = ref($self) || $self;
  my $attr = $AUTOLOAD;

  $attr =~ s/.*:://;
  return unless $attr=~ /[^A-Z]/;
  if (@_) {
    confess ("invalid bug attribute $attr") unless $ok_fields{$attr};
    $self->{$attr} = shift;
    return;
  }
  confess ("invalid bug attribute $attr") unless $ok_fields{$attr};
  if (defined $self->{$attr}) {
    return $self->{$attr};
  } else {
    return '';
  }
}
1;
