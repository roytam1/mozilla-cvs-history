#!/usr/bin/perl -w
# -*- mode: cperl; c-basic-offset: 8; indent-tabs-mode: nil; -*-

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
use Litmus::Auth;
use Litmus::Error;
use Litmus::FormWidget;
use Litmus::Utils;

use CGI;
use Date::Manip;
use JSON;

Litmus->init();
my $c = Litmus->cgi(); 

my $vars;

my $testgroup_id;
my $message;
my $status;
my $rv;

if ($c->param("searchTestgroupList")) {
  print $c->header('text/plain');
  my $product_id = $c->param("product");
  my $branch_id = $c->param("branch");

  my $testgroups;
  if ($branch_id) {
    $testgroups = Litmus::DB::Testgroup->search(branch => $branch_id);
  } elsif ($product_id) {
    $testgroups = Litmus::DB::Testgroup->search(product => $product_id);
  } else {
    $testgroups = Litmus::DB::Testgroup->retrieve_all;
  }
  while (my $tg = $testgroups->next) {
    print $tg->testgroup_id()."\n";
  }
  exit;
}

# anyone can use this script for its searching capabilities, but if we 
# get here, then you need to be an admin:
Litmus::Auth::requireProductAdmin('manage_testgroups.cgi');

my $product_persist = $c->param('product_persist') ? $c->param('product_persist') : 0;
my $branch_persist = $c->param('branch_persist') ? $c->param('branch_persist') : 0;
$vars->{'product_persist'} = $product_persist;
$vars->{'branch_persist'} = $branch_persist;

if ($c->param("testgroup_id")) {
  $testgroup_id = $c->param("testgroup_id");
}

my $defaults;
if ($c->param("delete_testgroup_button")) {
  my $testgroup = Litmus::DB::Testgroup->retrieve($testgroup_id);
  if ($testgroup) {
    Litmus::Auth::requireProductAdmin("manage_testgroups.cgi", $testgroup->product());
    $rv = $testgroup->delete_with_refs();
    if ($rv) {
      $status = "success";
      $message = "Testgroup ID# $testgroup_id deleted successfully.";
    } else {
      $status = "failure";
      $message = "Failed to delete Testgroup ID# $testgroup_id.";
    }
  } else { 
    $status = "failure";
    $message = "Testgroup ID# $testgroup_id does not exist. (Already deleted?)";
  }

} elsif ($c->param("clone_testgroup_button") and
         $c->param("testgroup_id")) {
 
  my $testgroup_id = $c->param("testgroup_id");
  my $testgroup = Litmus::DB::Testgroup->retrieve($testgroup_id);
  if ($testgroup) {
    Litmus::Auth::requireAdmin('manage_testgroups.cgi');

    # Do we already have our cloning info? Then get cloning!
    if ($c->param("clone_type")) {
      my $clone_type = sanitize($c->param("clone_type"));
      my $new_name = $c->param("new_testgroup_name");
      my $new_testgroup;
      if ($clone_type eq "simple") {
        $new_testgroup = $testgroup->clone($new_name);
      } elsif ($clone_type eq "preserve") {
        $new_testgroup = $testgroup->clone_preserve($new_name);

      } elsif ($clone_type eq "recursive") {
        my $old_name_regexp = sanitize($c->param("old_name_regexp"));
        my $new_name_regexp = sanitize($c->param("new_name_regexp"));
        $new_testgroup = $testgroup->clone_recursive($new_name,
                                                     undef,
                                                     $old_name_regexp,
                                                     $new_name_regexp
                                                    );
      }
   
      if ($new_testgroup) {
        $status = "success";
        $message = "Testgroup ID# $testgroup_id cloned successfully as testgroup ID# " . $new_testgroup->testgroup_id . ".";
        $defaults->{'testgroup_id'} = $new_testgroup->testgroup_id;
      } else {
        $status = "failure";
        $message = "Failed to clone testgroup ID# $testgroup_id.";
      }

    } else {
      my $vars = {
                  title => 'Manage Testgroups - Clone',
                  testgroup => $testgroup,
                 };

      print $c->header();
      Litmus->template()->process("admin/clone_testgroup.tmpl", $vars) ||
        internalError(Litmus->template()->error());

      exit;
    }
  } else {
    $status = "failure";
    $message = "Failed to lookup testgroup ID# $testgroup_id.";
  }

} elsif ($c->param("mode")) {
  requireField('product', $c->param('product'));
  requireField('branch', $c->param('branch'));
  my $enabled = $c->param('enabled') ? 1 : 0;
  my $now = &Date::Manip::UnixDate("now","%q");
  my $user_id = Litmus::Auth::getCurrentUser();

  if ($c->param("mode") eq "add") {
    Litmus::Auth::requireProductAdmin("manage_testgroups.cgi", $c->param('product'));
    my %hash = (
                name => $c->param('name'),
                product_id => $c->param('product'),
                enabled => $enabled,
                branch_id => $c->param('branch'),
                creation_date => $now,
                creator_id => $c->param('created_by'),
               );
    my $new_testgroup = 
      Litmus::DB::Testgroup->create(\%hash);

    if ($new_testgroup) {
      my @selected_subgroups = $c->param("testgroup_subgroups");
      $new_testgroup->update_subgroups(\@selected_subgroups);
      $status = "success";
      $message = "Testgroup added successfully. New testgroup ID# is " . $new_testgroup->testgroup_id;
      $defaults->{'testgroup_id'} = $new_testgroup->testgroup_id;
    } else {
      $status = "failure";
      $message = "Failed to add testgroup.";
    }
  } elsif ($c->param("mode") eq "edit") {
    requireField('testgroup_id', $c->param("editform_testgroup_id"));
    $testgroup_id = $c->param("editform_testgroup_id");
    my $testgroup = Litmus::DB::Testgroup->retrieve($testgroup_id);
    if ($testgroup) {
      Litmus::Auth::requireProductAdmin("manage_testgroups.cgi", $testgroup->product());
      $testgroup->product_id($c->param('product'));
      $testgroup->branch_id($c->param('branch'));
      $testgroup->enabled($enabled);
      $testgroup->name($c->param('name'));
      $testgroup->creator_id($c->param('created_by'));
      $rv = $testgroup->update();
      if ($rv) {
        my @selected_subgroups = $c->param("testgroup_subgroups");
        $testgroup->update_subgroups(\@selected_subgroups);
        $status = "success";
	$message = "Testgroup ID# $testgroup_id updated successfully.";
        $defaults->{'testgroup_id'} = $testgroup_id;
      } else {
	$status = "failure";
	$message = "Failed to update testgroup ID# $testgroup_id.";
      }
    } else {
      $status = "failure";
      $message = "Testgroup ID# $testgroup_id not found.";
    }
  } 
} else {
  $defaults->{'testgroup_id'} = $c->param("testgroup_id");
}

if ($defaults) {
  $vars->{'defaults'} = $defaults;
}

if ($status and $message) {
  $vars->{'onload'} = "toggleMessage('$status','$message');";
}

my $products = Litmus::FormWidget->getProducts();
my $branches = Litmus::FormWidget->getBranches();
my $testgroups = Litmus::FormWidget->getTestgroups;
my $subgroups = Litmus::FormWidget->getSubgroups(0,'name');
my $authors = Litmus::FormWidget->getAuthors();

# only allow the user access to the products they are product admins for
my %authorized_products;
my @tmp_products;
foreach my $b (@{$products}) {
	my %cur = %{$b};
	if (Litmus::Auth::getCurrentUser()->isProductAdmin($cur{product_id})) {
		push(@tmp_products, $b);
		$authorized_products{$cur{product_id}} = 1;
	}
}
$products = \@tmp_products;

# likewise for branches:
my %authorized_branches;
my @tmp_branches;
foreach my $b (@{$branches}) {
	my %cur = %{$b};
	if ($authorized_products{$cur{product_id}}) {
		push(@tmp_branches, $b);
		$authorized_branches{$cur{branch_id}} = 1;
	}
}
$branches = \@tmp_branches;

# and of course limit the testgroups
my @tmp_testgroups;
foreach my $b (@{$testgroups}) {
	my %cur = %{$b};
	if ($authorized_products{$cur{product_id}}) {
		push(@tmp_testgroups, $b);
	}
}
$testgroups = \@tmp_testgroups;


my $json = JSON->new(skipinvalid => 1, convblessed => 1);
my $products_js = $json->objToJson($products);
my $branches_js = $json->objToJson($branches);
my $subgroups_js = $json->objToJson($subgroups);

$vars->{'title'} = "Manage Testgroups";
$vars->{'products'} = $products;
$vars->{'products_js'} = $products_js;
$vars->{'branches'} = $branches;
$vars->{'branches_js'} = $branches_js;
$vars->{'testgroups'} = $testgroups;
$vars->{'subgroups_js'} = $subgroups_js;
$vars->{'authors'} = $authors;
$vars->{'user'} = Litmus::Auth::getCurrentUser();

my $cookie =  Litmus::Auth::getCookie();
$vars->{"defaultemail"} = $cookie;
$vars->{"show_admin"} = Litmus::Auth::istrusted($cookie);

print $c->header();

Litmus->template()->process("admin/manage_testgroups.tmpl", $vars) || 
  internalError("Error loading template.");
