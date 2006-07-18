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
# The Original Code is the Bugzilla Test Runner System.
#
# The Initial Developer of the Original Code is Maciej Maczynski.
# Portions created by Maciej Maczynski are Copyright (C) 2001
# Maciej Maczynski. All Rights Reserved.
#
# Contributor(s): Greg Hendricks <ghendricks@novell.com>

use strict;
use lib ".";

use Bugzilla;
use Bugzilla::Constants;
use Bugzilla::Error;
use Bugzilla::Util;

use Bugzilla::Testopia::Util;
use Bugzilla::Testopia::TestCase;

require "globals.pl";

use vars qw($template $vars);

Bugzilla->login(LOGIN_REQUIRED);
ThrowUserError("testopia-create-denied", {'object' => 'Test Case'}) unless UserInGroup('edittestcases');

print Bugzilla->cgi->header();
   
my $dbh = Bugzilla->dbh;
my $cgi = Bugzilla->cgi;

push @{$::vars->{'style_urls'}}, 'testopia/css/default.css';

my $action = $cgi->param('action') || '';
my @plan_id = $cgi->param('plan_id');

unless (@plan_id){
  $vars->{'form_action'} = 'tr_new_case.cgi';
  $template->process("testopia/plan/choose.html.tmpl", $vars) 
      || ThrowTemplateError($template->error());
  exit;
}
my %seen;
my @plans;
my @plan_ids;
my @categories;
foreach my $entry (@plan_id){
    foreach my $id (split(/[\s,]+/, $entry)){
        detaint_natural($id);
        validate_test_id($id, 'plan');
        $seen{$id} = 1;
    }
}
foreach my $id (keys %seen){
    my $plan = Bugzilla::Testopia::TestPlan->new($id);
    push @plan_ids, $id;
    push @plans, $plan;
    push @categories, @{$plan->categories};
}

ThrowUserError('testopia-create-category', {'plan' => $plans[0] }) if scalar @categories < 1;
if ($action eq 'Add'){
    my $alias       = $cgi->param('alias')|| '';
    my $category    = $cgi->param('category');
    my $status      = $cgi->param('status');
    my $priority    = $cgi->param('priority');
    my $isautomated = $cgi->param("isautomated");
    my $script      = $cgi->param("script")|| '';
    my $arguments   = $cgi->param("arguments")|| '';    
    my $summary     = $cgi->param("summary")|| '';
    my $requirement = $cgi->param("requirement")|| '';
    my $tcaction    = $cgi->param("tcaction");
    my $tceffect    = $cgi->param("tceffect");
    my $tcdependson = $cgi->param("tcdependson")|| '';
    my $tcblocks    = $cgi->param("tcblocks")|| '';
    my $tester      = $cgi->param("tester") || '';
    my @comps       = $cgi->param("components");
    if ($tester){
        $tester = DBNameToIdAndCheck($cgi->param('tester'));
    }
    
    ThrowUserError('testopia-missing-required-field', {'field' => 'summary'})  if $summary  eq '';
    ThrowUserError('testopia-missing-required-field', {'field' => 'Case Action'}) if $tcaction eq '';
    ThrowUserError('testopia-missing-required-field', {'field' => 'Case Expected Results'}) if $tceffect eq '';
    
    detaint_natural($status);
    detaint_natural($category);
    detaint_natural($priority);
    detaint_natural($isautomated);

    # All inserts are done with placeholders so this is OK
    trick_taint($alias);
    trick_taint($script);
    trick_taint($arguments);
    trick_taint($summary);
    trick_taint($requirement);
    trick_taint($tcaction);
    trick_taint($tceffect);
    trick_taint($tcdependson);
    trick_taint($tcblocks);
    
    validate_selection($category, 'category_id', 'test_case_categories');
    validate_selection($status, 'case_status_id', 'test_case_status');
    
    my @components;
    foreach my $id (@comps){
        detaint_natural($id);
        validate_selection($id, 'id', 'components');
        push @components, $id;
    }
    
    my $case = Bugzilla::Testopia::TestCase->new({
            'alias'          => $alias || undef,
            'case_status_id' => $status,
            'category_id'    => $category,
            'priority_id'    => $priority,
            'isautomated'    => $isautomated,
            'script'         => $script,
            'arguments'      => $arguments,
            'summary'        => $summary,
            'requirement'    => $requirement,
            'default_tester_id' => $tester,
            'author_id'  => Bugzilla->user->id,
            'action'     => $tcaction,
            'effect'     => $tceffect,
            'dependson'  => $tcdependson,
            'blocks'     => $tcblocks,
            'plans'      => \@plans,
    });

    ThrowUserError('testiopia-alias-exists', 
        {'alias' => $alias}) if $case->check_alias($alias);
    ThrowUserError('testiopia-invalid-data', 
        {'field' => 'isautomated', 'value' => $isautomated }) 
            if ($isautomated !~ /^[01]$/);
            
    my $case_id = $case->store;
    my $case = Bugzilla::Testopia::TestCase->new($case_id);
    
    $case->add_component($_) foreach (@components);
    if ($cgi->param('addtags')){
        foreach my $name (split(/,/, $cgi->param('addtags'))){
            trick_taint($name);
            my $tag = Bugzilla::Testopia::TestTag->new({'tag_name' => $name});
            my $tag_id = $tag->store;
            $case->add_tag($tag_id);
        }
    }

    $vars->{'action'} = "update";
    $vars->{'form_action'} = "tr_show_case.cgi";
    $vars->{'case'} = $case;
    $vars->{'tr_message'} = "Case $case_id Created. 
        <a href=\"tr_new_case.cgi?plan_id=" . join(",", @plan_ids) . "\">Add another</a>";
        
    $template->process("testopia/case/show.html.tmpl", $vars) ||
        ThrowTemplateError($template->error());
    
}
####################
### Display Form ###
####################
else {
    $vars->{'action'} = "Add";
    $vars->{'form_action'} = "tr_new_case.cgi";
    $vars->{'case'} = Bugzilla::Testopia::TestCase->new(
                        {'case_id' => 0, 
                         'plans' => \@plans, 
                         'category' => {'name' => 'Default'},
    });
    $template->process("testopia/case/add.html.tmpl", $vars) ||
        ThrowTemplateError($template->error());
}
