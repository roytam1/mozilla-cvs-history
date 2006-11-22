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
use Bugzilla::Testopia::TestRun;
use Bugzilla::Testopia::TestCaseRun;
use Bugzilla::Testopia::TestTag;
use Bugzilla::Testopia::Environment;
use Bugzilla::Testopia::Search;
use Bugzilla::Testopia::Table;

use vars qw($template $vars);
my $template = Bugzilla->template;
my $query_limit = 15000;

require "globals.pl";

Bugzilla->login();
   
my $dbh = Bugzilla->dbh;
my $cgi = Bugzilla->cgi;

my $run_id = trim($cgi->param('run_id') || '');

unless ($run_id){
  print $cgi->header;
  $template->process("testopia/run/choose.html.tmpl", $vars) 
      || ThrowTemplateError($template->error());
  exit;
}
validate_test_id($run_id, 'run');
push @{$::vars->{'style_urls'}}, 'testopia/css/default.css';

my $serverpush = support_server_push($cgi);

my $action = $cgi->param('action') || '';

####################
### Edit Actions ###
####################
if ($action eq 'Commit'){
    Bugzilla->login(LOGIN_REQUIRED);
    my $run = Bugzilla::Testopia::TestRun->new($run_id);
    ThrowUserError("testopia-read-only", {'object' => 'run'}) unless $run->canedit;
    do_update($run);
    $vars->{'tr_message'} = "Test run updated";
    $vars->{'backlink'} = $run;
    print $cgi->header;
    display($run);    
}

elsif ($action eq 'History'){
    my $run = Bugzilla::Testopia::TestRun->new($run_id);
    $vars->{'run'} = $run; 
    print $cgi->header;
    $template->process("testopia/run/history.html.tmpl", $vars)
      || ThrowTemplateError($template->error());
       
}

#############
### Clone ###
#############
elsif ($action =~ /^Clone/){
    Bugzilla->login(LOGIN_REQUIRED);
    my $run = Bugzilla::Testopia::TestRun->new($run_id);
    ThrowUserError("testopia-read-only", {'object' => 'run'}) unless $run->canedit;
    my $case_list = $cgi->param('case_list');
    do_update($run);
    my @ids;
    foreach my $id (split(",", $case_list)){
        detaint_natural($id);
        push @ids, $id;
    }
    
    my $dbh = Bugzilla->dbh;
    my $ref;
    if ($case_list){ 
        $ref = $dbh->selectcol_arrayref(
            "SELECT DISTINCT case_id 
               FROM test_case_runs
              WHERE case_run_id IN (" . join(",",@ids) . ")");
    }
    
    $vars->{'run'} = $run;
    $vars->{'case_list'} = join(",", @$ref) if ($action =~/These Cases/ && $ref);
    $vars->{'caserun'} = Bugzilla::Testopia::TestCaseRun->new({'case_run_id' => 0});
    print $cgi->header;
    $template->process("testopia/run/clone.html.tmpl", $vars) 
      || ThrowTemplateError($template->error());
    
}
elsif ($action eq 'do_clone'){
    Bugzilla->login(LOGIN_REQUIRED);
    my $run = Bugzilla::Testopia::TestRun->new($run_id);
    if ($serverpush) {
        print $cgi->multipart_init();
        print $cgi->multipart_start();
    
        $template->process("list/server-push.html.tmpl", $vars)
          || ThrowTemplateError($template->error());

    }
    
    ThrowUserError("testopia-read-only", {'object' => 'run'}) unless $run->canedit;
    my $summary = $cgi->param('summary');
    my $build = $cgi->param('build');
    trick_taint($summary);
    detaint_natural($build);
    my $newrun = Bugzilla::Testopia::TestRun->new($run->clone($summary, $build));

    if($cgi->param('copy_tags')){
        foreach my $tag (@{$run->tags}){
            my $newtag = Bugzilla::Testopia::TestTag->new({
                           tag_name  => $tag->name
                         });
# Store will return the id of the existing tag if it exists 
# or create it if it does not
            my $newtagid = $newtag->store;
            $newrun->add_tag($newtagid);
        }
    }
    my $progress_interval = 250;
        
    if ($cgi->param('case_list')){
        my @case_ids;
        foreach my $id (split(",", $cgi->param('case_list'))){
            
            detaint_natural($id);
            my $case = Bugzilla::Testopia::TestCase->new($id);
            ThrowUserError('testopia-permission-denied', {'object' => 'Test Case'})
                unless $case->canview;
            push @case_ids, $id
        }

        my $i = 0;
        my $total = scalar @case_ids;

        foreach my $id (@case_ids){
            $i++;
            if ($i % $progress_interval == 0 && $serverpush){
                print $cgi->multipart_end;
                print $cgi->multipart_start;
                $vars->{'complete'} = $i;
                $vars->{'total'} = $total;
                $template->process("testopia/progress.html.tmpl", $vars)
                  || ThrowTemplateError($template->error());
            } 
            
            $newrun->add_case_run($id);
        }
    }
    if ($cgi->param('copy_test_cases')){
        if ($cgi->param('status')){
            my @status = $cgi->param('status');
            foreach my $s (@status){
                detaint_natural($s);
            }
            my $ref = $dbh->selectcol_arrayref(
                "SELECT case_id
                   FROM test_case_runs
                  WHERE run_id = ?
                    AND case_run_status_id IN (". join(",", @status) .")
                    AND iscurrent = 1", undef, $run->id);

            my $i = 0;
            my $total = scalar @$ref;

            foreach my $case_id (@{$ref}){
                $i++;
                if ($i % $progress_interval == 0 && $serverpush){
                    print $cgi->multipart_end;
                    print $cgi->multipart_start;
                    $vars->{'complete'} = $i;
                    $vars->{'total'} = $total;
                    $template->process("testopia/progress.html.tmpl", $vars)
                      || ThrowTemplateError($template->error());
                } 

                $newrun->add_case_run($case_id);
            }
        }
        else {
            my $i = 0;
            my $total = scalar @{$run->cases};
            
            foreach my $case (@{$run->cases}){
                $i++;
                if ($i % $progress_interval == 0 && $serverpush){
                    print $cgi->multipart_end;
                    print $cgi->multipart_start;
                    $vars->{'complete'} = $i;
                    $vars->{'total'} = $total;
                    $template->process("testopia/progress.html.tmpl", $vars)
                      || ThrowTemplateError($template->error());
                }
                 
                $newrun->add_case_run($case->id);
            }
        }
    }
    if ($serverpush) {
        print $cgi->multipart_end;
        print $cgi->multipart_start;
    } else {
        print $cgi->header;
    }
    
    $cgi->delete_all;
    $cgi->param('run_id', $newrun->id);
    $vars->{'tr_message'} = "Test run cloned";
    $vars->{'backlink'} = $run;
    display($newrun);
    print $cgi->multipart_final if $serverpush; 
}
####################
### Ajax Actions ###
####################
elsif ($action eq 'addcc'){
    Bugzilla->login(LOGIN_REQUIRED);
    my $run = Bugzilla::Testopia::TestRun->new($run_id);
    ThrowUserError("testopia-read-only", {'object' => 'run'}) unless $run->canedit;
    my @cclist = split(/[\s,]+/, $cgi->param('cc'));
    my %ccids;
    foreach my $email (@cclist){
        my $ccid = DBNameToIdAndCheck($email);
        if ($ccid && !$ccids{$ccid}) {
           $ccids{$ccid} = 1;
        }
    }
    foreach my $ccid (keys(%ccids)) {
        $run->add_cc($ccid);
    }
    my $cc = get_cc_xml($run);
    print $cc;
}
elsif ($action eq 'removecc'){
    Bugzilla->login(LOGIN_REQUIRED);
    my $run = Bugzilla::Testopia::TestRun->new($run_id);
    ThrowUserError('insufficient-case-perms') unless $run->canedit;
    foreach my $ccid (split(",", $cgi->param('cc'))){
        detaint_natural($ccid);
        $run->remove_cc($ccid);
    }
    my $cc = get_cc_xml($run);
    print $cc;
}
elsif ($action eq 'Delete'){
    Bugzilla->login(LOGIN_REQUIRED);
    my $run = Bugzilla::Testopia::TestRun->new($run_id);
    ThrowUserError("testopia-read-only", {'object' => 'run'}) unless $run->candelete;
    $vars->{'run'} = $run;
    print $cgi->header;
    
    $template->process("testopia/run/delete.html.tmpl", $vars) ||
        ThrowTemplateError($template->error());
    
}
elsif ($action eq 'do_delete'){
    Bugzilla->login(LOGIN_REQUIRED);
    my $run = Bugzilla::Testopia::TestRun->new($run_id);
    ThrowUserError("testopia-read-only", {'object' => 'run'}) unless $run->candelete;
    if ($serverpush) {
        print $cgi->multipart_init();
        print $cgi->multipart_start();
        $vars->{'complete'} = 1;
        $vars->{'total'} = 250;
        $template->process("testopia/progress.html.tmpl", $vars)
          || ThrowTemplateError($template->error());

        $run->obliterate($cgi,$template);
    }
    else{
        $run->obliterate;
    }
    if ($serverpush) {
        print $cgi->multipart_end;
        print $cgi->multipart_start;
    } else {
        print $cgi->header;
    }
    
    $vars->{'deleted'} = 1;
    $template->process("testopia/run/delete.html.tmpl", $vars) ||
        ThrowTemplateError($template->error());
    print $cgi->multipart_final if $serverpush;
}

####################
### Just show it ###
####################
else {
    print $cgi->header;
    display(Bugzilla::Testopia::TestRun->new($run_id));
}
###################
### Helper Subs ###
###################
sub get_cc_xml {
    my ($run) = @_;
    my $ret = "<cclist>";
    foreach my $c (@{$run->cc}){
        $ret .= "<user>";
        $ret .= "<id>". $c->{'id'} ."</id>";
        $ret .= "<name>". $c->{'login'} ."</name>";
        $ret .= "</user>";
    }
    $ret .= "</cclist>";
    return $ret;
} 

sub do_update {
    my ($run) = @_;
    
    my $timestamp;
    $timestamp = $run->stop_date;
    $timestamp = undef if $cgi->param('status') && $run->stop_date;
    $timestamp = get_time_stamp() if !$cgi->param('status') && !$run->stop_date;
    my $summary = $cgi->param('summary') || '';
    my $prodver = $cgi->param('product_version');
    my $planver = $cgi->param('plan_version');
    my $build = $cgi->param('build');
    my $env      = $cgi->param('environment') ? $cgi->param('environment') : $cgi->param('env_pick');
    my $manager = DBNameToIdAndCheck(trim($cgi->param('manager')));
    my $notes = trim($cgi->param('notes'));

    ThrowUserError('testopia-missing-required-field', {'field' => 'summary'}) if ($cgi->param('summary') eq '');
    ThrowUserError('testopia-missing-required-field', {'field' => 'environment'}) if ($env eq '');
    
    trick_taint($summary);
    trick_taint($prodver);
    trick_taint($planver);
    trick_taint($notes);
    
    detaint_natural($build);
    detaint_natural($env);
    
    validate_test_id($build, 'build');
    validate_test_id($env, 'environment');

    my $check = validate_version($prodver, $run->plan->product_id);
    if ($check ne '1'){
        $vars->{'tr_error'} = "Version mismatch. Please update the product version";
        $prodver = $check;
    }
    #TODO: Are notes something we want in the history?
    #$run->update_notes($notes);
    
    my %newvalues = ( 
        'summary'           => $summary,
        'product_version'   => $prodver,
        'plan_text_version' => $planver,
        'build_id'          => $build,
        'environment_id'    => $env,
        'stop_date'         => $timestamp,
        'manager_id'        => $manager,
        'notes'             => $notes
    );
    $run->update(\%newvalues);
    
    # Add new tags 
    foreach my $tag_name (split(/[,]+/, $cgi->param('newtag'))){
        trick_taint($tag_name);
        my $tag = Bugzilla::Testopia::TestTag->new({tag_name => $tag_name});
        my $tag_id = $tag->store;
        $run->add_tag($tag_id);
    }
    
    $cgi->delete_all;
    $cgi->param('run_id', $run->id);

}

sub display {
    my $run = shift;

    $cgi->param('current_tab', 'case_run');
    my $search = Bugzilla::Testopia::Search->new($cgi);
    my $table = Bugzilla::Testopia::Table->new('case_run', 'tr_show_run.cgi', $cgi, undef, $search->query);
    ThrowUserError('testopia-query-too-large', {'limit' => $query_limit}) if $table->view_count > $query_limit;
    
    my $case = Bugzilla::Testopia::TestCase->new({'case_id' => 0});
    $vars->{'expand_report'} = $cgi->param('expand_report') || 0;
    $vars->{'expand_filter'} = $cgi->param('expand_filter') || 0;
    $vars->{'run'} = $run;
    $vars->{'table'} = $table;
    $vars->{'action'} = 'Commit';
    $template->process("testopia/run/show.html.tmpl", $vars) ||
        ThrowTemplateError($template->error());
}
