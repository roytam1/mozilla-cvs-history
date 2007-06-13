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
# The Original Code is the Bugzilla Testopia System.
#
# The Initial Developer of the Original Code is Greg Hendricks.
# Portions created by Greg Hendricks are Copyright (C) 2006
# Novell. All Rights Reserved.
#
# Contributor(s): Greg Hendricks <ghendricks@novell.com>

use strict;
use lib ".";

use Bugzilla;
use Bugzilla::Config;
use Bugzilla::Error;
use Bugzilla::Constants;
use Bugzilla::Util;
use Bugzilla::User;
use Bugzilla::Testopia::Util;
use Bugzilla::Testopia::Search;
use Bugzilla::Testopia::Table;
use Bugzilla::Testopia::TestRun;

my $vars = {};
my $cgi = Bugzilla->cgi;
my $template = Bugzilla->template;
my $query_limit = 5000;

Bugzilla->login(LOGIN_REQUIRED);
my $serverpush = support_server_push($cgi);
if ($serverpush) {
    print $cgi->multipart_init;
    print $cgi->multipart_start;

    # Under mod_perl, flush stdout so that the page actually shows up.
    if ($ENV{MOD_PERL}) {
        require Apache2::RequestUtil;
        Apache2::RequestUtil->request->rflush();
    }

    $template->process("list/server-push.html.tmpl", $vars)
      || ThrowTemplateError($template->error());
}
else {
    print $cgi->header;
}
# prevent DOS attacks from multiple refreshes of large data
$::SIG{TERM} = 'DEFAULT';
$::SIG{PIPE} = 'DEFAULT';

my $action = $cgi->param('action') || '';
if ($action eq 'Commit'){
    # Get the list of checked items. This way we don't have to cycle through 
    # every test case, only the ones that are checked.
    my $reg = qr/r_([\d]+)/;
    my $params = join(" ", $cgi->param());
    my @params = $cgi->param();
    unless ($params =~ $reg){
        print $cgi->multipart_end if $serverpush;
        ThrowUserError('testopia-none-selected', {'object' => 'run'});
    }
    if ($cgi->param('environment') eq ''){
        print $cgi->multipart_end if $serverpush;
        ThrowUserError('testopia-missing-required-field', {'field' => 'environment'});
    }
    
    my $progress_interval = 250;
    my $i = 0;
    my $total = scalar @params;
    my @uneditable;
    
    foreach my $p ($cgi->param()){
        my $run = Bugzilla::Testopia::TestRun->new($1) if $p =~ $reg;
        next unless $run;
        
        $i++;
        if ($i % $progress_interval == 0 && $serverpush){
            print $cgi->multipart_end;
            print $cgi->multipart_start;
            $vars->{'complete'} = $i;
            $vars->{'total'} = $total;
            $template->process("testopia/progress.html.tmpl", $vars)
              || ThrowTemplateError($template->error());
        }
        unless ($run->canedit){
            push @uneditable, $run;
            next;
        }
        my $manager = login_to_id(trim($cgi->param('manager')));

        if ($cgi->param('manager') && !$manager){
            print $cgi->multipart_end if $serverpush;
            ThrowUserError("invalid_username", { name => $cgi->param('manager') }) if $cgi->param('manager');
        }
        my $stop_date;
        if ($cgi->param('run_status')){
            if ($cgi->param('run_status') == -1 || $run->stop_date){
                $stop_date = $run->stop_date;
            }
            else {
                $stop_date = get_time_stamp();
            }
        }

        my $enviro    = $cgi->param('environment')   eq '--Do Not Change--' ? $run->environment->id : $cgi->param('environment');
        my $build     = $cgi->param('build') == -1 ? $run->build->id : $cgi->param('build');

        validate_test_id($enviro, 'environment');
        validate_test_id($build, 'build');
        my %newvalues = ( 
            'manager_id'        => $manager || $run->manager->id,
            'stop_date'         => $stop_date,
            'environment_id'    => $enviro,
            'build_id'          => $build
        );
        $run->update(\%newvalues);
        if ($cgi->param('addtags')){
            foreach my $name (split(/[,]+/, $cgi->param('addtags'))){
                trick_taint($name);
                my $tag = Bugzilla::Testopia::TestTag->new({'tag_name' => $name});
                my $tag_id = $tag->store;
                if ($cgi->param('tag_action') eq 'add'){
                    $run->add_tag($tag_id);
                }
                else {
                    $run->remove_tag($tag_id);
                }
            }
        }     
    }
    if ($serverpush && !$cgi->param('debug')) {
        print $cgi->multipart_end;
        print $cgi->multipart_start;
    }
    my $run = Bugzilla::Testopia::TestRun->new({});
    my $updated = $i - scalar @uneditable;
    $vars->{'run'} = $run;
    $vars->{'title'} = $i ? "Update Successful" : "Nothing Updated";
    $vars->{'tr_error'} = "You did not have rights to edit ". scalar @uneditable . "runs" if scalar @uneditable > 0;
    $vars->{'tr_message'} = "$updated Test Runs Updated" if $updated;
    $vars->{'current_tab'} = 'run';
    $vars->{'build_list'} = $run->get_distinct_builds();
    $template->process("testopia/search/advanced.html.tmpl", $vars)
        || ThrowTemplateError($template->error()); 
    print $cgi->multipart_final if $serverpush;
    exit;
    
}
else {
    $vars->{'qname'} = $cgi->param('qname') if $cgi->param('qname');
    $cgi->param('current_tab', 'run');
    my $search = Bugzilla::Testopia::Search->new($cgi);
    my $table = Bugzilla::Testopia::Table->new('run', 'tr_list_runs.cgi', $cgi, undef, $search->query);
    if ($table->view_count > $query_limit){
        print $cgi->multipart_end if $serverpush;
        ThrowUserError('testopia-query-too-large', {'limit' => $query_limit});
    }
    
    if ($table->list_count > 0){
        my $plan_id = $table->list->[0]->plan->product_id;
        foreach my $run (@{$table->list}){
            if ($run->plan->product_id != $plan_id){
                $vars->{'multiprod'} = 1;
                last;
            }
        }
        if (!$vars->{'multiprod'}) {
            my $p = $table->list->[0]->plan;
            my $build_list  = $p->product->builds;
            unshift @{$build_list},  {'id' => -1, 'name' => "--Do Not Change--"};
            $vars->{'build_list'} = $build_list;
        }
        
    }        
    my $r = Bugzilla::Testopia::TestRun->new({'run_id' => 0 });

    my $status_list = $r->get_status_list;
    
    unshift @{$status_list}, {'id' => -1, 'name' => "--Do Not Change--"};

    $vars->{'status_list'} = $status_list;
    
    $vars->{'fullwidth'} = 1; #novellonly
    $vars->{'dotweak'} = Bugzilla->user->in_group('Testers');
    $vars->{'table'} = $table;
    if ($serverpush && !$cgi->param('debug')) {
        print $cgi->multipart_end;
        print $cgi->multipart_start;
    }
    $template->process("testopia/run/list.html.tmpl", $vars)
      || ThrowTemplateError($template->error()); 
    print $cgi->multipart_final if $serverpush;
}
