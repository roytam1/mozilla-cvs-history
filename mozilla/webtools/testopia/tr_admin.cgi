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
# Portions created by Greg Hendricks are Copyright (C) 2001
# Greg Hendricks. All Rights Reserved.
#
# Contributor(s): Greg Hendricks <ghendricks@novell.com>

use strict;
use lib ".";

use strict;
use lib ".";

use Bugzilla;
use Bugzilla::Constants;
use Bugzilla::Error;
use Bugzilla::Util;
use Bugzilla::Testopia::TestPlan;
use Bugzilla::Testopia::Util;

use vars qw($vars);
my $template = Bugzilla->template;

Bugzilla->login(LOGIN_REQUIRED);

print Bugzilla->cgi->header();
ThrowUserError("testopia-read-only", {'object' => 'plan type'}) unless Bugzilla->user->in_group('admin');   

my $dbh = Bugzilla->dbh;
my $cgi = Bugzilla->cgi;

my $plan = Bugzilla::Testopia::TestPlan->new({'plan_id' => 0});
my $action = $cgi->param('action') || '';
my $item = $cgi->param('item') || '';

if ($item eq 'plan_type'){
    if( $action eq 'edit'){
        my $type_id = $cgi->param('type_id');
        detaint_natural($type_id);
        $vars->{'type'} = $plan->lookup_plan_type($type_id) 
            || ThrowUserError("invalid-test-id-non-existent", {'id' => $type_id, 'type' => 'Plan Type'});
        $template->process("testopia/admin/plantypes/edit.html.tmpl", $vars) 
            || ThrowTemplateError($template->error());
    }
    elsif ($action eq 'doedit'){
        my $type_id = $cgi->param('type_id');
        my $type_name = $cgi->param('name') || '';
        
        ThrowUserError('testopia-missing-required-field', {'field' => 'name'})  if $type_name  eq '';
        detaint_natural($type_id);
        ThrowUserError("invalid-test-id-non-existent", 
            {'id' => $type_id, 'type' => 'Plan Type'}) unless $plan->lookup_plan_type($type_id);
        trick_taint($type_name);
        
        $plan->update_plan_type($type_id, $type_name);
        display();
    }
    elsif ($action eq 'add'){
        $template->process("testopia/admin/plantypes/add.html.tmpl", $vars) 
            || ThrowTemplateError($template->error());
    }
    elsif ($action eq 'doadd'){
        my $type_name = $cgi->param('name') || '';
        
        ThrowUserError('testopia-missing-required-field', {'field' => 'name'})  if $type_name  eq '';
        trick_taint($type_name);
        ThrowUserError('testopia-name-not-unique', 
            {'object' => 'Plan Type', 'name' => $type_name}) if $plan->check_plan_type($type_name);
        
        $plan->add_plan_type($type_name);
        display();
    }
    else{
        display();     
    }
    
}

else {
    $template->process("testopia/admin/show.html.tmpl", $vars) 
        || ThrowTemplateError($template->error());
}

sub display {
       $vars->{'plan'} = $plan;
        $template->process("testopia/admin/plantypes/show.html.tmpl", $vars) 
            || ThrowTemplateError($template->error());
}
