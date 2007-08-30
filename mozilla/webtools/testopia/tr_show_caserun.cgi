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
use Bugzilla::Bug;
use Bugzilla::Constants;
use Bugzilla::Error;
use Bugzilla::Util;
use Bugzilla::User;
use Bugzilla::Testopia::Util;
use Bugzilla::Testopia::Search;
use Bugzilla::Testopia::Table;
use Bugzilla::Testopia::TestRun;
use Bugzilla::Testopia::TestCase;
use Bugzilla::Testopia::TestCaseRun;

local our $vars = {};
local our $template = Bugzilla->template;
local our $query_limit = 15000;

Bugzilla->login(LOGIN_REQUIRED);
   
local our $cgi = Bugzilla->cgi;

print $cgi->header;

my $caserun_id = $cgi->param('caserun_id');
validate_test_id($caserun_id, 'case_run');

ThrowUserError('testopia-missing-parameter', {'param' => 'caserun_id'}) unless ($caserun_id);
$vars->{'fullwidth'} = 1;
my $action = $cgi->param('action') || '';

# For use on the classic form

if ($action eq 'Commit'){
    my $caserun = Bugzilla::Testopia::TestCaseRun->new($caserun_id);
    $caserun = do_update($caserun);
    display(Bugzilla::Testopia::TestCaseRun->new($caserun->id));
}
elsif ($action eq 'Attach'){
    my $caserun = do_update(Bugzilla::Testopia::TestCaseRun->new($caserun_id));

    defined $cgi->upload('data')
        || ThrowUserError("file_not_specified");
    
    my $fh = $cgi->upload('data');
    my $data;
    # enable 'slurp' mode
    local $/;
    $data = <$fh>;
    $data || ThrowUserError("zero_length_file");
    
    my $attachment = Bugzilla::Testopia::Attachment->create({
                        caserun_id   => $caserun_id,
                        case_id      => $caserun->case->id,
                        submitter_id => Bugzilla->user->id,
                        description  => $cgi->param('description'),
                        filename     => $cgi->upload('data'),
                        mime_type    => $cgi->uploadInfo($cgi->param('data'))->{'Content-Type'},
                        contents     => $data
    });

    $vars->{'tr_message'} = "File attached.";
    $vars->{'backlink'} = $caserun;
    display(Bugzilla::Testopia::TestCaseRun->new($caserun->id));
}

elsif ($action eq 'delete'){
    my $caserun = Bugzilla::Testopia::TestCaseRun->new($caserun_id);
    ThrowUserError("testopia-no-delete", {'object' => $caserun}) unless $caserun->candelete;
    $vars->{'title'} = 'Remove Test Case '. $caserun->case->id .' from Run: ' . $caserun->run->summary;
    $vars->{'bugcount'} = scalar @{$caserun->bugs};
    $vars->{'form_action'} = 'tr_show_caserun.cgi';
    $vars->{'caserun'} = $caserun;
    $template->process("testopia/caserun/delete.html.tmpl", $vars) ||
        ThrowTemplateError($template->error());
}
elsif ($action eq 'do_delete'){
    my $caserun = Bugzilla::Testopia::TestCaseRun->new($caserun_id);
    ThrowUserError("testopia-no-delete", {'object' => $caserun}) unless $caserun->candelete;
    $caserun->obliterate($cgi->param('single'));

    # See if there is a saved filter
    if ($cgi->cookie('TESTOPIA-FILTER-RUN-' . $caserun->run->id) && $action ne 'Filter' && $action ne 'clear_filter'){
        $cgi = Bugzilla::CGI->new($cgi->cookie('TESTOPIA-FILTER-RUN-' . $caserun->run->id));
        $vars->{'filtered'} = 1;
    }
    else {
        $cgi->delete_all;
    }
    $cgi->param('current_tab', 'case_run');
    $cgi->param('run_id', $caserun->run->id);
    my $search = Bugzilla::Testopia::Search->new($cgi);
    my $table = Bugzilla::Testopia::Table->new('case_run', 'tr_show_run.cgi', $cgi, undef, $search->query);
    ThrowUserError('testopia-query-too-large', {'limit' => $query_limit}) if $table->view_count > $query_limit;
    
    my @case_list;
    foreach my $cr (@{$table->list}){
        push @case_list, $cr->case_id;
    }
    my $case = Bugzilla::Testopia::TestCase->new({'case_id' => 0});
    $vars->{'run'} = $caserun->run;
    $vars->{'table'} = $table;
    $vars->{'case_list'} = join(",", @case_list);
    $vars->{'action'} = 'Commit';
    $vars->{'tr_message'} = "Case removed";
    $vars->{'backlink'} = $caserun->run;
    
    # We need these to provide the filter values. 
    $vars->{'caserun'} = Bugzilla::Testopia::TestCaseRun->new({});
    $vars->{'case'} = Bugzilla::Testopia::TestCase->new({});

    $template->process("testopia/run/show.html.tmpl", $vars) ||
        ThrowTemplateError($template->error());

}
####################
### Ajax Actions ###
####################
elsif ($action eq 'update_build'){
    my $caserun = Bugzilla::Testopia::TestCaseRun->new($caserun_id);
    if (!$caserun->canedit) { 
        print "Error - You don't have permission";
        exit;
    }
    my $build_id = $cgi->param('build_id');
    detaint_natural($build_id);
    validate_test_id($build_id, 'build');
    
    $caserun = $caserun->switch($build_id, $caserun->environment->id);
    
    my $body_data;
    my $head_data;
    
    $vars->{'caserun'} = $caserun;
    $vars->{'index'}   = $cgi->param('index');
    $vars->{'updating'} = 1;
    $template->process("testopia/caserun/short-form-header.html.tmpl", $vars, \$head_data) ||
        ThrowTemplateError($template->error());
    $template->process("testopia/caserun/short-form.html.tmpl", $vars, \$body_data) ||
        ThrowTemplateError($template->error());
    
    print $head_data . "|~+" . $body_data;
}
elsif ($action eq 'update_environment'){
    my $caserun = Bugzilla::Testopia::TestCaseRun->new($caserun_id);
    if (!$caserun->canedit) { 
        print "Error - You don't have permission";
        exit;
    }
    my $environment_id = $cgi->param('caserun_env');
    detaint_natural($environment_id);
    validate_test_id($environment_id, 'environment');
    
    $caserun = $caserun->switch($caserun->build->id, $environment_id);
    
    my $body_data;
    my $head_data;
    $vars->{'caserun'} = $caserun;
    $vars->{'index'}   = $cgi->param('index');
    $vars->{'updating'} = 1;
    $template->process("testopia/caserun/short-form-header.html.tmpl", $vars, \$head_data) ||
        ThrowTemplateError($template->error());
    $template->process("testopia/caserun/short-form.html.tmpl", $vars, \$body_data) ||
        ThrowTemplateError($template->error());
    print $head_data . "|~+" . $body_data;
}
elsif ($action eq 'update_status'){
    my $caserun = Bugzilla::Testopia::TestCaseRun->new($caserun_id);
    if (!$caserun->canedit){
        print "Error - You don't have permission";
        exit;
    }
    my $status_id = $cgi->param('status_id');
    my $note = $cgi->param('note');

    detaint_natural($status_id);
    trick_taint($note) if $note;

    $caserun->set_status($status_id, $cgi->param('update_bug'));
    $caserun->append_note($note) if $note;

    my $percent_data;
    my $body_data;
    my $head_data;
    $vars->{'caserun'} = $caserun;
    $vars->{'index'}   = $cgi->param('index');
    $vars->{'run'} = $caserun->run;
    $vars->{'updating'} = 1; 
    $template->process("testopia/percent_bar.html.tmpl", $vars, \$percent_data) ||
        ThrowTemplateError($template->error());
    $template->process("testopia/caserun/short-form-header.html.tmpl", $vars, \$head_data) ||
        ThrowTemplateError($template->error());
    $template->process("testopia/caserun/short-form.html.tmpl", $vars, \$body_data) ||
        ThrowTemplateError($template->error());
    print $percent_data . "|~+" . $head_data . "|~+" . $body_data;
    if ($caserun->updated_deps) {
        print "|~+". join(',', @{$caserun->updated_deps});
    }
}
elsif ($action eq 'update_note'){
    my $caserun = Bugzilla::Testopia::TestCaseRun->new($caserun_id);
    if (!$caserun->canedit){
        print "Error - You don't have permission";
        exit;
    }
    my $note = $cgi->param('note');
    trick_taint($note);
    $caserun->append_note($note);
    
    my $body_data;
    my $head_data;
    $vars->{'caserun'} = $caserun;
    $vars->{'index'}   = $cgi->param('index');
    $vars->{'updating'} = 1;
    $template->process("testopia/caserun/short-form-header.html.tmpl", $vars, \$head_data) ||
        ThrowTemplateError($template->error());
    $template->process("testopia/caserun/short-form.html.tmpl", $vars, \$body_data) ||
        ThrowTemplateError($template->error());
    print $head_data . "|~+" . $body_data;

}
elsif ($action eq 'update_assignee'){
    my $caserun = Bugzilla::Testopia::TestCaseRun->new($caserun_id);
    if (!$caserun->canedit){
        print "Error - You don't have permission";
        exit;
    }
    
    my $assignee_id = login_to_id(trim($cgi->param('assignee')));
    if ($assignee_id == 0){
        print "Error - Invalid assignee";
        exit;
    }
    trick_taint($assignee_id);
    $caserun->set_assignee($assignee_id);
}
elsif ($action eq 'update_sortkey'){
    my $caserun = Bugzilla::Testopia::TestCaseRun->new($caserun_id);
    if (!$caserun->canedit){
        print "Error - You don't have permission";
        exit;
    }
    my $sortkey = $cgi->param('sortkey');
    unless (detaint_natural($sortkey)){
        print "Error - Please enter a number";
        exit;
    }
    $caserun->set_sortkey($sortkey);
}
elsif ($action eq 'get_notes'){
    my $caserun = Bugzilla::Testopia::TestCaseRun->new($caserun_id);
    if (!$caserun->canedit){
        print "Error - You don't have permission";
        exit;
    }
    print '<pre>' .  $caserun->notes . '</pre>';
}
elsif ($action eq 'attach_bug'){
    my $caserun = Bugzilla::Testopia::TestCaseRun->new($caserun_id);
    if (!$caserun->canedit){
        print "Error - You don't have permission";
        exit;
    }
    my @buglist;
    foreach my $bug (split(/[\s,]+/, $cgi->param('bugs'))){
        
        my $error_mode_cache = Bugzilla->error_mode;
        Bugzilla->error_mode(ERROR_MODE_DIE);
        eval{
            ValidateBugID($bug);
        };
        Bugzilla->error_mode($error_mode_cache);
        if ($@){
            print "<span style='font-weight:bold; color:#FF0000;'>Error - Invalid bug id or alias</span>";
            exit;
        }
        push @buglist, $bug;
    }
    foreach my $bug (@buglist){
        $caserun->attach_bug($bug);
    }
    foreach my $bug (@{$caserun->case->bugs}){
        print Bugzilla::Template::get_bug_link($bug->bug_id, $bug->bug_id) ." ";
    }
}
elsif ($action eq 'detach_bug'){
    my $caserun = Bugzilla::Testopia::TestCaseRun->new($caserun_id);
    if (!$caserun->canedit){
        print "Error - You don't have permission";
        exit;
    }
    my @buglist;
    foreach my $bug (split(/[\s,]+/, $cgi->param('bug_id'))){
        ValidateBugID($bug);
        push @buglist, $bug;
    }
    foreach my $bug (@buglist){
        $caserun->detach_bug($bug);
    }
    display(Bugzilla::Testopia::TestCaseRun->new($caserun_id));
}
else {
    display(Bugzilla::Testopia::TestCaseRun->new($caserun_id));
}

sub do_update {
    my $caserun = shift;
    
    ThrowUserError("testopia-read-only", {'object' => $caserun}) unless $caserun->canedit;
    
    my $status   = $cgi->param('status');
    my $notes    = $cgi->param('notes');
    my $build    = $cgi->param('caserun_build');
    my $env      = $cgi->param('caserun_env');
    my $assignee;
    if ($cgi->param('assignee')) {
        $assignee = login_to_id(trim($cgi->param('assignee')));
    }
    ThrowUserError("invalid_username", { name => $cgi->param('assignee') }) unless $assignee;
    
    ThrowUserError('testopia-missing-required-field', {field => 'Status'}) unless defined $status;
    ThrowUserError('testopia-missing-required-field', {field => 'build'}) unless defined $build;
    ThrowUserError('testopia-missing-required-field', {field => 'environment'}) unless defined $env;
    
    validate_test_id($build, 'build');
    validate_test_id($env, 'environment');
    my @buglist;
    foreach my $bug (split(/[\s,]+/, $cgi->param('bugs'))){
        ValidateBugID($bug);
        push @buglist, $bug;
    }
    
    detaint_natural($env);
    detaint_natural($build);
    detaint_natural($status);
    trick_taint($notes);
    trick_taint($assignee);

    # Switch to the record representing this build and environment combo.
    # If there is not one, it will create it and switch to that.
    $caserun = $caserun->switch($build,$env);
    
    $caserun->set_status($status, $cgi->param('update_bug'))     if ($caserun->status_id != $status);
    $caserun->set_assignee($assignee) if ($caserun->assignee && $caserun->assignee->id != $assignee);
    $caserun->append_note($notes)     if ($notes && $caserun->notes !~ /$notes/);

    foreach my $bug (@buglist){
        $caserun->attach_bug($bug);
    }
    
    $vars->{'tr_message'} = "Case-run updated.";
    
    return $caserun;
}

sub display {
    my $caserun = shift;
    ThrowUserError("testopia-permission-denied", {'object' => $caserun}) if !$caserun->canview;
    my $table = Bugzilla::Testopia::Table->new('case_run');
    ThrowUserError('testopia-query-too-large', {'limit' => $query_limit}) if $table->list_count > $query_limit;
    $vars->{'table'} = $table;
    $vars->{'caserun'} = $caserun;
    $vars->{'action'} = 'tr_show_caserun.cgi';
    $vars->{'run'} = $caserun->run;
    $vars->{'single'} = 1;
    $template->process("testopia/caserun/show.html.tmpl", $vars) ||
        ThrowTemplateError($template->error());
}