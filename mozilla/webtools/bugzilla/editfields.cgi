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
# The Original Code is the Bugzilla Bug Tracking System.
#
# Contributor(s): Frédéric Buclin <LpSolit@gmail.com>

use strict;
use lib ".";

use Bugzilla;
use Bugzilla::Constants;
use Bugzilla::Error;
use Bugzilla::Util;
use Bugzilla::Field;
use Bugzilla::Token;

my $cgi = Bugzilla->cgi;
my $template = Bugzilla->template;
my $vars = {};

# Make sure the user is logged in and is an administrator.
my $user = Bugzilla->login(LOGIN_REQUIRED);
$user->in_group('admin')
  || ThrowUserError('auth_failure', {group  => 'admin',
                                     action => 'edit',
                                     object => 'custom_fields'});

my $action = trim($cgi->param('action') || '');
my $token  = $cgi->param('token');

print $cgi->header();

# List all existing custom fields if no action is given.
if (!$action) {
    $template->process('admin/custom_fields/list.html.tmpl', $vars)
        || ThrowTemplateError($template->error());
}
# Interface to add a new custom field.
elsif ($action eq 'add') {
    $vars->{'token'} = issue_session_token('add_field');

    $template->process('admin/custom_fields/create.html.tmpl', $vars)
        || ThrowTemplateError($template->error());
}
elsif ($action eq 'new') {
    check_token_data($token, 'add_field');
    my $name = clean_text($cgi->param('name') || '');
    my $desc = clean_text($cgi->param('desc') || '');
    my $type = trim($cgi->param('type') || FIELD_TYPE_FREETEXT);
    my $sortkey = $cgi->param('sortkey') || 0;

    # Validate these fields.
    $name || ThrowUserError('customfield_missing_name');
    # Don't want to allow a name that might mess up SQL.
    $name =~ /^\w+$/ || ThrowUserError('customfield_invalid_name',
                                       { name => $name });
    # Prepend cf_ to the custom field name to distinguish it from standard fields.
    if ($name !~ /^cf_/) {
        $name = 'cf_' . $name;
    }
    my $field = new Bugzilla::Field({'name' => $name});
    ThrowUserError('customfield_already_exists', {'field' => $field }) if $field;

    $desc || ThrowUserError('customfield_missing_description', {'name' => $name});

    # We hardcode valid values for $type. This doesn't matter.
    my $typ = $type;
    (detaint_natural($type) && $type < 3)
      || ThrowCodeError('invalid_customfield_type', {'type' => $typ});

    my $skey = $sortkey;
    detaint_natural($sortkey)
      || ThrowUserError('customfield_invalid_sortkey', {'name'    => $name,
                                                        'sortkey' => $skey});

    # All fields have been validated. We can create this new custom field.
    trick_taint($name);
    trick_taint($desc);

    $vars->{'name'} = $name;
    $vars->{'desc'} = $desc;
    $vars->{'sortkey'} = $sortkey;
    $vars->{'type'} = $type;
    $vars->{'custom'} = 1;
    $vars->{'in_new_bugmail'} = $cgi->param('new_bugmail') ? 1 : 0;
    $vars->{'editable_on_enter_bug'} = $cgi->param('enter_bug') ? 1 : 0;
    $vars->{'is_obsolete'} = $cgi->param('obsolete') ? 1 : 0;

    Bugzilla::Field::create_or_update($vars);
    delete_token($token);

    $vars->{'message'} = 'custom_field_created';

    $template->process('admin/custom_fields/list.html.tmpl', $vars)
        || ThrowTemplateError($template->error());
}
elsif ($action eq 'edit') {
    my $name = $cgi->param('name') || ThrowUserError('customfield_missing_name');
    # Custom field names must start with "cf_".
    if ($name !~ /^cf_/) {
        $name = 'cf_' . $name;
    }
    my $field = new Bugzilla::Field({'name' => $name});
    $field || ThrowUserError('customfield_nonexistent', {'name' => $name});

    $vars->{'field'} = $field;
    $vars->{'token'} = issue_session_token('edit_field');

    $template->process('admin/custom_fields/edit.html.tmpl', $vars)
        || ThrowTemplateError($template->error());
}
elsif ($action eq 'update') {
    check_token_data($token, 'edit_field');
    my $name = $cgi->param('name');
    my $desc = clean_text($cgi->param('desc') || '');
    my $sortkey = $cgi->param('sortkey') || 0;

    # Validate fields.
    $name || ThrowUserError('customfield_missing_name');
    # Custom field names must start with "cf_".
    if ($name !~ /^cf_/) {
        $name = 'cf_' . $name;
    }
    my $field = new Bugzilla::Field({'name' => $name});
    $field || ThrowUserError('customfield_nonexistent', {'name' => $name});

    $desc || ThrowUserError('customfield_missing_description', {'name' => $name});
    trick_taint($desc);

    my $skey = $sortkey;
    detaint_natural($sortkey)
      || ThrowUserError('customfield_invalid_sortkey', {'name'    => $name,
                                                        'sortkey' => $skey});

    $vars->{'name'} = $field->name;
    $vars->{'desc'} = $desc;
    $vars->{'sortkey'} = $sortkey;
    $vars->{'custom'} = 1;
    $vars->{'in_new_bugmail'} = $cgi->param('new_bugmail') ? 1 : 0;
    $vars->{'editable_on_enter_bug'} = $cgi->param('enter_bug') ? 1 : 0;
    $vars->{'is_obsolete'} = $cgi->param('obsolete') ? 1 : 0;

    Bugzilla::Field::create_or_update($vars);
    delete_token($token);

    $vars->{'message'} = 'custom_field_updated';

    $template->process('admin/custom_fields/list.html.tmpl', $vars)
        || ThrowTemplateError($template->error());
}
else {
    ThrowUserError('no_valid_action', {'field' => 'custom_field'});
}
