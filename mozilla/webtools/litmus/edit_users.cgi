#!/usr/bin/perl -w
# -*- Mode: perl; indent-tabs-mode: nil -*-

# ***** BEGIN LICENSE BLOCK *****
# Version: MPL 1.1
#
# The contents of this file are subject to the Mozilla Public License Version
# 1.1 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
# http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
# for the specific language governing rights and limitations under the
# License.
#
# The Original Code is Litmus.
#
# The Initial Developer of the Original Code is
# the Mozilla Corporation.
# Portions created by the Initial Developer are Copyright (C) 2006
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#   Chris Cooper <ccooper@deadsquid.com>
#   Zach Lipton <zach@zachlipton.com>
#
# ***** END LICENSE BLOCK *****

use strict;

use Litmus;
use Litmus::Error;
use Litmus::DB::Product;
use Litmus::Auth;
use Litmus::Utils;

use CGI;
use Time::Piece::MySQL;

Litmus->init();
my $c = Litmus->cgi(); 

Litmus::Auth::requireLogin("edit_users.cgi");

# Only trusted users can edit other users.
my $cookie = undef;
$cookie = Litmus::Auth::getCookie();
if (Litmus::Auth::istrusted($cookie)) {
  
  if ($c->param('submit')) {
    # search for users:
    
    # gather group membership bits:
    my @groups = Litmus::DB::SecurityGroup->retrieve_all(order_by => "grouptype, group_id");
    my @group_search;
    my %checked;
    foreach my $cur (@groups) {
    	if ($c->param("group_".$cur->group_id())) {
    		push(@group_search, $cur);
    		$checked{$cur->group_id()} = 1;
    	}
    }
    
    
    my @users = Litmus::DB::User->search_full_text(
                                                         $c->param('search_string'), 
                                                         $c->param('search_string'),
                                                         $c->param('search_string'),
                                                         @group_search);
                                                        
    
    my $vars = {
		users => \@users,
                search_string => $c->param('search_string'),
                groups => \@groups,
                checked => \%checked,
               };
    print $c->header();
    Litmus->template()->process("admin/edit_users/search_results.html.tmpl", $vars) || 
      internalError(Litmus->template()->error()); 
  } elsif ($c->param('id')) {
    # lookup a given user
    my $uid = $c->param('id');
    my $user = Litmus::DB::User->retrieve($uid);
    print $c->header();
    if (! $user) {
      invalidInputError("Invalid user ID: $uid");
    }
    my @groups = Litmus::DB::SecurityGroup->retrieve_all();
    my $vars = {
		user => $user,
		groups => \@groups,
    };
    Litmus->template()->process("admin/edit_users/edit_user.html.tmpl", $vars) || 
      internalError(Litmus->template()->error()); 
  } elsif ($c->param('user_id')) {
    # process changes to a user:
    my $user = Litmus::DB::User->retrieve($c->param('user_id'));
    print $c->header();
    if (! $user) {
      invalidInputError("Invalid user ID: " . $c->param('user_id'));
    }
    $user->bugzilla_uid($c->param('bugzilla_uid'));
    $user->email($c->param('edit_email'));
    
    if ($c->param('edit_password') ne '' and
        $c->param('edit_password') eq $c->param('edit_confirm_password')) {
      # they changed the password, so let the auth folks know:
      Litmus::Auth::changePassword($user, $c->param('edit_password'));
    }
    $user->realname($c->param('realname'));
    $user->irc_nickname($c->param('irc_nickname'));

    my $revoke_sessions = 0;
    # Check to see whether we are enabling this user.    
    if ($c->param('enabled')) {
      $user->enabled(1);
    } else {
      if ($user->enabled) {
        $user->enabled(0);
        $revoke_sessions = 1;
      }
    }
    
    $user->authtoken($c->param('authtoken'));
    
    # process changes to group permissions:
    my @allgroups = Litmus::DB::SecurityGroup->retrieve_all();
    foreach my $group (@allgroups) {
    	if ($c->param("group_".$group->group_id())) {
    		# we're blessing this user
    		Litmus::DB::UserGroupMap->find_or_create(user=>$user, group=>$group);
    	} else {
    		# unblesing (if previously blessed) the user
    		Litmus::DB::UserGroupMap->remove($user, $group);
    	}
    }
    

    $user->authtoken($c->param('authtoken'));
    $user->update();

   if ($revoke_sessions) {
     Litmus::DB::Session->search(user_id => $user->{'user_id'})->delete_all; 
   }

    my $vars = {
		user => $user,
                onload => "toggleMessage('success','User information updated successfully.');",
                groups => \@allgroups,
               };
    Litmus->template()->process("admin/edit_users/search_users.html.tmpl", $vars) || 
      internalError(Litmus->template()->error()); 
  } else {
    # we're here for the first time, so display the search form
    my @groups = Litmus::DB::SecurityGroup->retrieve_all();
    my $vars = {
       			groups => \@groups,
               };
    print $c->header();
    Litmus->template()->process("admin/edit_users/search_users.html.tmpl", $vars) || 
      internalError(Litmus->template()->error()); 
  }

} else {
  my $uid = $cookie->user_id;

  # Process user-submited changes.
  if ($c->param('user_id')) {
    # Check for the user_id param, but don't trust its contents.
    my $user = Litmus::DB::User->retrieve($uid);

    print $c->header();

    if (! $user) {
      invalidInputError("Invalid user ID: $uid");
    }

    if (!Litmus::Auth::checkPassword($user,$c->param('current_password'))) {
      invalidInputError("The current password you supplied was invalid.");
    }      

    $user->email($c->param('edit_email'));    
    $user->realname($c->param('realname'));
    $user->irc_nickname($c->param('irc_nickname'));
    $user->update();

    my $template_file = "admin/edit_users/edit_user.html.tmpl";
    if ($c->param('edit_password') ne '' and
        $c->param('edit_password') eq $c->param('edit_confirm_password')) {
      # they changed the password, so let the auth folks know:
      Litmus::Auth::changePassword($user, $c->param('edit_password'));
      $template_file = "auth/login.html.tmpl";
    }

    my $vars = {
		user => $user,
                onload => "toggleMessage('success','User information updated successfully.');"
               };
    Litmus->template()->process($template_file, $vars) ||
      internalError(Litmus->template()->error());

  } else {
    # Lookup details for non-admin user.
    my $user = Litmus::DB::User->retrieve($uid);
    print $c->header();
    if (! $user) {
      invalidInputError("Invalid user ID: $uid");
    }
    my $vars = {
                user => $user,
               };
    Litmus->template()->process("admin/edit_users/edit_user.html.tmpl", $vars) || 
      internalError(Litmus->template()->error());
  }
}




