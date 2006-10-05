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
# Contributor(s):  Greg Hendricks <ghendricks@novell.com>

use strict;
use lib ".";

use Bugzilla;
use Bugzilla::Util;
use Bugzilla::Error;
use Bugzilla::Constants;
use Bugzilla::Testopia::Search;
use Bugzilla::Testopia::Util;
use Bugzilla::Testopia::TestCase;
use Bugzilla::Testopia::TestCaseRun;
use Bugzilla::Testopia::TestPlan;
use Bugzilla::Testopia::TestTag;
use Bugzilla::Testopia::Table;

use vars qw($vars $template);
require "globals.pl";

my $dbh = Bugzilla->dbh;
my $cgi = Bugzilla->cgi;
my $template = Bugzilla->template;

push @{$::vars->{'style_urls'}}, 'testopia/css/default.css';

$cgi->send_cookie(-name => "TEST_LAST_ORDER",
                  -value => $cgi->param('order'),
                  -expires => "Fri, 01-Jan-2038 00:00:00 GMT");
Bugzilla->login();
print $cgi->header;
my $action = $cgi->param('action') || '';

if ($action eq 'Commit'){
    Bugzilla->login(LOGIN_REQUIRED);
    # Get the list of checked items. This way we don't have to cycle through 
    # every test case, only the ones that are checked.
    my $reg = qr/c_([\d]+)/;
    my @cases;
    foreach my $p ($cgi->param()){
        push @cases, Bugzilla::Testopia::TestCase->new($1) if $p =~ $reg;
    }
    ThrowUserError('testopia-none-selected', {'object' => 'case'}) if (scalar @cases < 1);
    foreach my $case (@cases){
        ThowUserError('insufficient-perms') unless $case->canedit;
        my $requirement = $cgi->param('requirement') eq '--Do Not Change--' ? $case->requirement : $cgi->param('requirement');
        my $arguments = $cgi->param('arguments') eq '--Do Not Change--' ? $case->arguments : $cgi->param('requirement');
        my $script = $cgi->param('script') eq '--Do Not Change--' ? $case->script : $cgi->param('requirement');
        my $status = $cgi->param('status')     == -1 ? $case->status_id : $cgi->param('status');
        my $priority = $cgi->param('priority') == -1 ? $case->{'priority_id'} : $cgi->param('priority');
        my $category = $cgi->param('category') == -1 ? $case->{'category_id'} : $cgi->param('category');
        my $isautomated = $cgi->param('isautomated') == -1 ? $case->isautomated : $cgi->param('isautomated');
        my $tester = $cgi->param('tester') || ''; 
        if ($tester && $tester ne '--Do Not Change--'){
            $tester = DBNameToIdAndCheck(trim($cgi->param('tester')));
        }
        else {
            $tester = $case->default_tester->id;
        }
            
        # We use placeholders so trick_taint is ok.
        trick_taint($requirement) if $requirement;
        trick_taint($arguments) if $arguments;
        trick_taint($script) if $script;
        
        detaint_natural($status);
        detaint_natural($priority);
        detaint_natural($category);
        detaint_natural($isautomated);
        
        my %newvalues = ( 
            'case_status_id' => $status,
            'category_id'    => $category,
            'priority_id'    => $priority,
            'isautomated'    => $isautomated,
            'requirement'    => $requirement,
            'script'         => $script,
            'arguments'      => $arguments,
            'default_tester_id' => $tester  || $case->default_tester->id,
        );
      
        $case->update(\%newvalues);
        if ($cgi->param('addtags')){
            foreach my $name (split(/[\s,]+/, $cgi->param('addtags'))){
                trick_taint($name);
                my $tag = Bugzilla::Testopia::TestTag->new({'tag_name' => $name});
                my $tag_id = $tag->store;
                $case->add_tag($tag_id);
            }
        }
        my @runs;
        foreach my $runid (split(/[\s,]+/, $cgi->param('addruns'))){
            validate_test_id($runid, 'run');
            push @runs, Bugzilla::Testopia::TestRun->new($runid);
        }
        foreach my $run (@runs){
            $run->add_case_run($case->id);
        }
        
        foreach my $planid (split(",", $cgi->param('linkplans'))){
            validate_test_id($planid, 'plan');
            $case->link_plan($planid);
        }
    }
    my @runlist = split(/[\s,]+/, $cgi->param('addruns'));
    if (scalar @runlist == 1){
        my $run_id = $cgi->param('addruns');
        validate_test_id($run_id, 'run');
        $cgi->delete_all;
        $cgi->param('run_id', $run_id);
        $cgi->param('current_tab', 'case_run');
        my $search = Bugzilla::Testopia::Search->new($cgi);
        my $table = Bugzilla::Testopia::Table->new('case_run', 'tr_show_run.cgi', $cgi, undef, $search->query);
        
        my @case_list;
        foreach my $caserun (@{$table->list}){
            push @case_list, $caserun->case_id;
        }
        $vars->{'run'} = Bugzilla::Testopia::TestRun->new($run_id);
        $vars->{'table'} = $table;
        $vars->{'case_list'} = join(",", @case_list);
        $vars->{'action'} = 'Commit';
        $template->process("testopia/run/show.html.tmpl", $vars)
            || ThrowTemplateError($template->error());
        exit;
    } 
    my $case = Bugzilla::Testopia::TestCase->new({ 'case_id' => 0 });
    $vars->{'case'} = $case;
    $vars->{'title'} = "Update Successful";
    $vars->{'tr_message'} = scalar @cases . " Test Cases Updated";
    $vars->{'current_tab'} = 'case';
    $template->process("testopia/search/advanced.html.tmpl", $vars)
        || ThrowTemplateError($template->error()); 
    exit;
}

# Take the search from the URL params and convert it to SQL
$cgi->param('current_tab', 'case');
my $search = Bugzilla::Testopia::Search->new($cgi);
my $table = Bugzilla::Testopia::Table->new('case', 'tr_list_cases.cgi', $cgi, undef, $search->query);

# Check that all of the test cases returned only belong to one product.
if ($table->list_count > 0){
    my %case_prods;
    my $prod_id;
    foreach my $case (@{$table->list}){
        $case_prods{$case->id} = $case->get_product_ids;
        $prod_id = @{$case_prods{$case->id}}[0];
        if (scalar(@{$case_prods{$case->id}} > 1)){
            $vars->{'multiprod'} = 1 ;
            last;
        }
    }
    # Check that all of them are the same product
    if (!$vars->{'multiprod'}){
        foreach my $c (keys %case_prods){
            if ($case_prods{$c}->[0] != $prod_id){
                $vars->{'multiprod'} = 1;
                last;
            }
        }
    }
    if (!$vars->{'multiprod'}) {
        my $category_list = $table->list->[0]->get_category_list;
        unshift @{$category_list},  {'id' => -1, 'name' => "--Do Not Change--"};
        $vars->{'category_list'} = $category_list;
    }
}
# create an empty case to use for getting status and priority lists
my $c = Bugzilla::Testopia::TestCase->new({'case_id' => 0 });
my $status_list   = $c->get_status_list;
my $priority_list = $c->get_priority_list;

# add the "do not change" option to each list
# we use unshift so they show at the top of the list
unshift @{$status_list},   {'id' => -1, 'name' => "--Do Not Change--"};
unshift @{$priority_list}, {'id' => -1, 'name' => "--Do Not Change--"};

my $addrun = $cgi->param('addrun');
if ($addrun){
    validate_test_id($addrun, 'run');
    my $run = Bugzilla::Testopia::TestRun->new($addrun);
    $vars->{'addruns'} = $addrun;
    $vars->{'plan'} = $run->plan;
}
    
$vars->{'addrun'} = $cgi->param('addrun');
$vars->{'fullwidth'} = 1; #novellonly
$vars->{'case'} = $c;
$vars->{'status_list'} = $status_list;
$vars->{'priority_list'} = $priority_list;
$vars->{'dotweak'} = UserInGroup('edittestcases');
$vars->{'table'} = $table;

$template->process("testopia/case/list.html.tmpl", $vars)
    || ThrowTemplateError($template->error());
 
