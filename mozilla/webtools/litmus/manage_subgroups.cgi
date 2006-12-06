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
use Litmus::Cache;
use Litmus::Error;
use Litmus::FormWidget;
use Litmus::Utils;

use CGI;
use Date::Manip;
use JSON;

Litmus->init();
my $c = Litmus->cgi(); 

my $vars;

my $subgroup_id;
my $message;
my $status;
my $rv;


if ($c->param("searchSubgroupList")) {
  print $c->header('text/plain');
  my $product_id = $c->param("product");
  my $testgroup_id = $c->param("testgroup");
  
  my $subgroups;
  
  if ($testgroup_id) {
    $subgroups = Litmus::DB::Subgroup->search_ByTestgroup($testgroup_id);
  } elsif ($product_id) {
    $subgroups = Litmus::DB::Subgroup->search(product => $product_id);
  }
  while (my $sg = $subgroups->next) {
    print $sg->subgroup_id()."\n";
  }
  exit;
}

# anyone can use this script for its searching capabilities, but if we 
# get here, then you need to be an admin:
Litmus::Auth::requireAdmin('manage_subgroups.cgi');

if ($c->param("subgroup_id")) {
  $subgroup_id = $c->param("subgroup_id");
}
my $rebuild_cache = 0;
my $defaults;
if ($c->param("delete_subgroup_button")) {
  my $subgroup = Litmus::DB::Subgroup->retrieve($subgroup_id);
  if ($subgroup) {
    $rv = $subgroup->delete_with_refs();
    if ($rv) {
      $status = "success";
      $message = "Subgroup ID# $subgroup_id deleted successfully.";
      $rebuild_cache = 1;
    } else {
      $status = "failure";
      $message = "Failed to delete Subgroup ID# $subgroup_id.";
    }
  } else { 
    $status = "failure";
    $message = "Subgroup ID# $subgroup_id does not exist. (Already deleted?)";
  }
} elsif ($c->param("clone_subgroup_button")) {
  my $subgroup = Litmus::DB::Subgroup->retrieve($subgroup_id);
  my $new_subgroup = $subgroup->clone;
  if ($new_subgroup) {
    $status = "success";
    $message = "Subgroup cloned successfully. New subgroup ID# is " . $new_subgroup->subgroup_id;
    $defaults->{'subgroup_id'} = $new_subgroup->subgroup_id;
    $rebuild_cache = 1;
  } else {
    $status = "failure";
    $message = "Failed to clone Subgroup ID# $subgroup_id.";
  }
} elsif ($c->param("editform_mode")) {
  requireField('product', $c->param('editform_product'));
  requireField('branch', $c->param('editform_branch'));
  my $enabled = $c->param('editform_enabled') ? 1 : 0;
  if ($c->param("editform_mode") eq "add") {
    my %hash = (
                name => $c->param('editform_name'),
                product_id => $c->param('editform_product'),
                branch_id => $c->param('editform_branch'),
                enabled => $enabled,
               );
    my $new_subgroup = 
      Litmus::DB::Subgroup->create(\%hash);

    if ($new_subgroup) {      
      my @selected_testcases = $c->param("editform_subgroup_testcases");
      $new_subgroup->update_testcases(\@selected_testcases);
      $status = "success";
      $message = "Subgroup added successfully. New subgroup ID# is " . $new_subgroup->subgroup_id;
      $defaults->{'subgroup_id'} = $new_subgroup->subgroup_id;
      $rebuild_cache = 1;
    } else {
      $status = "failure";
      $message = "Failed to add subgroup.";        
    }
    
  } elsif ($c->param("editform_mode") eq "edit") {
    requireField('subgroup_id', $c->param("editform_subgroup_id"));
    $subgroup_id = $c->param("editform_subgroup_id");
    my $subgroup = Litmus::DB::Subgroup->retrieve($subgroup_id);
    if ($subgroup) {
      $subgroup->product_id($c->param('editform_product'));
      $subgroup->branch_id($c->param('editform_branch'));
      $subgroup->enabled($enabled);
      $subgroup->name($c->param('editform_name'));
      $rv = $subgroup->update();
      if ($rv) {
        my @selected_testcases = $c->param("editform_subgroup_testcases");
        $subgroup->update_testcases(\@selected_testcases);
        $status = "success";
	$message = "Subgroup ID# $subgroup_id updated successfully.";
        $defaults->{'subgroup_id'} = $subgroup_id;
        $rebuild_cache = 1;
      } else {
	$status = "failure";
	$message = "Failed to update subgroup ID# $subgroup_id.";        
      }
    } else {
      $status = "failure";
      $message = "Subgroup ID# $subgroup_id not found.";        
    }
  } 
} else {
  $defaults->{'subgroup_id'} = $c->param("subgroup_id");
}

if ($defaults) {
  $vars->{'defaults'} = $defaults;  
}

if ($status and $message) {
  $vars->{'onload'} = "toggleMessage('$status','$message');";
}

if ($rebuild_cache) {
  Litmus::Cache::rebuildCache();
}

my $subgroups = Litmus::FormWidget->getSubgroups(0,'name');
my $products = Litmus::FormWidget->getProducts();
my $testcases = Litmus::FormWidget->getTestcases(0,'name');

my $json = JSON->new(skipinvalid => 1, convblessed => 1);
my $testcases_js = $json->objToJson($testcases);

$vars->{'title'} = "Manage Subgroups";
$vars->{'subgroups'} = $subgroups;
$vars->{'products'} = $products;
$vars->{'all_testcases'} = $testcases_js;
$vars->{'user'} = Litmus::Auth::getCurrentUser();

my $cookie =  Litmus::Auth::getCookie();
$vars->{"defaultemail"} = $cookie;
$vars->{"show_admin"} = Litmus::Auth::istrusted($cookie);

print $c->header();

Litmus->template()->process("admin/manage_subgroups.tmpl", $vars) || 
  internalError("Error loading template.");

