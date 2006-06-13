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
# Portions taken from processbug.cgi and Bug.pm 
# which are copyrighted by
#                 Terry Weissman <terry@mozilla.org>
#                 Dan Mosedale <dmose@mozilla.org>
#                 Dave Miller <justdave@syndicomm.com>
#                 Christopher Aillon <christopher@aillon.com>
#                 Myk Melez <myk@mozilla.org>
#                 Jeff Hedlund <jeff.hedlund@matrixsi.com>
#                 Fr��ic Buclin <LpSolit@gmail.com>
#
# Contributor(s): Greg Hendricks <ghendricks@novell.com>

=head1 NAME

Bugzilla::Testopia::TestCase - Testopia Test Case object

=head1 DESCRIPTION

This module represents a test case in Testopia. Each test case must 
be linked to one or more test plans.

=head1 SYNOPSIS

use Bugzilla::Testopia::TestCase;

 $case = Bugzilla::Testopia::TestCase->new($case_id);
 $case = Bugzilla::Testopia::TestCase->new(\%case_hash);

=cut

package Bugzilla::Testopia::TestCase;

use strict;

use Bugzilla::Util;
use Bugzilla::Bug;
use Bugzilla::User;
use Bugzilla::Config;
use Bugzilla::Error;
use Bugzilla::Constants;
use Bugzilla::Testopia::Util;
use Bugzilla::Testopia::TestPlan;
use Bugzilla::Testopia::TestRun;
use Bugzilla::Testopia::TestCaseRun;
use Bugzilla::Testopia::Category;

use Text::Diff;

###############################
####    Initialization     ####
###############################

=head1 FIELDS

    case_id
    case_status_id
    category_id
    priority_id
    author_id
    default_tester_id  
    creation_date
    isautomated
    sortkey
    script
    arguments
    summary
    requirement
    alias

=cut

use constant DB_COLUMNS => qw(
    test_cases.case_id
    test_cases.case_status_id
    test_cases.category_id
    test_cases.priority_id
    test_cases.author_id
    test_cases.default_tester_id  
    test_cases.creation_date
    test_cases.isautomated
    test_cases.sortkey
    test_cases.script
    test_cases.arguments
    test_cases.summary
    test_cases.requirement
    test_cases.alias
);

our $columns = join(", ", DB_COLUMNS);

sub display_columns {
my $self = shift;
my @columns = 
   [{column => 'case_id',        desc => 'ID'          },
    {column => 'case_status_id', desc => 'Status'      },
    {column => 'category_id',    desc => 'Category'    },
    {column => 'priority_id',    desc => 'Priority'    },
    {column => 'summary',        desc => 'Summary'     },
    {column => 'requirement',    desc => 'Requirement' },
    {column => 'alias',          desc => 'Alias'       }];

$self->{'display_columns'} = \@columns;
return $self->{'display_columns'};
}

=head1 METHODS

=cut

###############################
####       Methods         ####
###############################

=head2 new

Instantiate a new Test Case. This takes a single argument 
either a test case ID or a reference to a hash containing keys 
identical to a test case's fields and desired values.

=cut

sub new {
    my $invocant = shift;
    my $class = ref($invocant) || $invocant;
    my $self = {};
    bless($self, $class);
    return $self->_init(@_);
}

=head2 _init

Private Test Case constructor. This does the actual work of building 
a test case object which is passed to new to be blessed

=cut

sub _init {
    my $self = shift;
    my ($param) = (@_);
    my $dbh = Bugzilla->dbh;

    my $id = $param unless (ref $param eq 'HASH');
    my $obj;

    if (defined $id && detaint_natural($id)) {

        $obj = $dbh->selectrow_hashref(qq{
            SELECT $columns FROM test_cases
            WHERE case_id = ?}, undef, $id);
    } elsif (ref $param eq 'HASH'){
         $obj = $param;   
    } else {
        ThrowCodeError('bad_arg',
            {argument => 'param',
             function => 'Testopia::TestCase::_init'});
    }

    return undef unless (defined $obj);

    foreach my $field (keys %$obj) {
        $self->{$field} = $obj->{$field};
    }
    return $self;
}

=head2 get_selectable_components

Returns a reference to a list of selectable components not already
associated with this case. 

=cut

#TODO: This can be done with a single query
sub get_selectable_components {
    my $self = shift;
    my $dbh = Bugzilla->dbh;
    my @compids;
    my @exclusions;
    foreach my $e (@{$self->components}){
        push @exclusions, $e->{'id'};
    }
    my $query = "SELECT id FROM components
                 WHERE product_id = ?";
    if (@exclusions){
        $query .= " AND id NOT IN(". join(",", @exclusions) .")";
    }
    foreach my $p (@{$self->plans}){ 
        my $ref = $dbh->selectcol_arrayref(
                $query, undef, $p->{'product_id'});
        push @compids, @{$ref};
    }
    my %compseen;
    foreach (@compids){
        $compseen{$_} = 1;
    }
    @compids = keys %compseen;
    #TODO: 2.22 use Bugzilla::Component
    my @comps;
    push @comps, {'id' => '0', 'name' => '--Please Select--'};
    foreach my $id (@compids){
        my $ref = $dbh->selectrow_hashref(
            "SELECT id, name FROM components
            WHERE id = ?", undef, $id); 
        
        push @comps, $ref;
        #push @comps, Bugzilla::Component->new($id);
    }
    return \@comps;
    
}

=head2 get_available_components

Returns a list of all user visible components for use in searches

=cut

#TODO: This can be done with a single query
sub get_available_components{
    my $self = shift;
    my $dbh = Bugzilla->dbh;
    #TODO: 2.22 use Product.pm
    my $products = $dbh->selectcol_arrayref(
            "SELECT id FROM products");
    my @selectable;
    foreach my $p (@{$products}){
        if (Bugzilla::Testopia::Util::can_view_product($p)){
            my $ref = $dbh->selectcol_arrayref(
                "SELECT name
                 FROM components WHERE product_id=?", 
                 undef, $p);
            push @selectable, @{$ref};
        }
    }
    # weed out duplicate names
    my %comps;
    foreach my $c (@selectable){
        $comps{$c} = 1;
    }
    @selectable = ();
    foreach my $k (sort(keys %comps)){
        push @selectable, {'id' => $k, 'name' => $k};
    }
    return \@selectable;             
}

=head2 get_category_list

Returns a list of categories associated with products in all
plans referenced by this case.

=cut

#TODO: Move to Testopia::Product.pm in 2.22
sub get_category_list{
    my $self = shift;
    my $dbh = Bugzilla->dbh;
    my @categories;
    foreach my $p (@{$self->plans}){
        if (Bugzilla::Testopia::Util::can_view_product($p->product_id)){
            push @categories, @{$p->categories};
        }
    }
    my %seen;
    foreach my $c (@categories){
        $seen{$c->id} = $c;
    }
    @categories = values %seen;
    @categories = sort @categories;
    return \@categories;    
}

=head2 get_distinct_categories

Returns a list of all user viewable categories for use in searches

=cut

#TODO: Move to Testopia::Product.pm in 2.22
sub get_distinct_categories{
    my $self = shift;
    my $dbh = Bugzilla->dbh;
    my $query = "SELECT DISTINCT cat.name AS id, cat.name " .
                  "FROM test_case_categories AS cat " .
                  "JOIN products ON cat.product_id = products.id " .
             "LEFT JOIN group_control_map " .
              "ON group_control_map.product_id = products.id ";
    if (Param('useentrygroupdefault')) {
        $query .= "AND group_control_map.entry != 0 ";
    } else {
        $query .= "AND group_control_map.membercontrol = " .
              CONTROLMAPMANDATORY . " ";
    }
    if (%{Bugzilla->user->groups}) {
        $query .= "AND group_id NOT IN(" . 
              join(',', values(%{Bugzilla->user->groups})) . ") ";
    }
    $query .= "WHERE group_id IS NULL ORDER BY cat.name";
    
    my $ref = $dbh->selectall_arrayref($query, {'Slice'=>{}});

    return $ref;             
}

=head2 get_status_list

Returns the list of legal statuses for a test case

=cut

sub get_status_list {
    my $self = shift;
    my $dbh = Bugzilla->dbh;
    my $ref = $dbh->selectall_arrayref("
        SELECT case_status_id AS id, name 
        FROM test_case_status", {"Slice"=>{}});
    return $ref
}

=head2 get_priority_list

Returns a list of legal priorities

=cut

sub get_priority_list {
    my $self = shift;
    my $dbh = Bugzilla->dbh;
    my $ref = $dbh->selectall_arrayref("
        SELECT id, value AS name FROM priority
        ORDER BY sortkey", {"Slice"=>{}});
    return $ref
}

=head2 add_tag

Associates a tag with this test case

=cut

sub add_tag {
    my $self = shift;
    my ($tag_id) = @_;
    my $dbh = Bugzilla->dbh;

    $dbh->bz_lock_tables('test_case_tags WRITE');
    my $tagged = $dbh->selectrow_array(
             "SELECT 1 FROM test_case_tags 
              WHERE tag_id = ? AND case_id = ?",
              undef, $tag_id, $self->{'case_id'});
    if ($tagged) {
        $dbh->bz_unlock_tables();
        return 1;
    }
    $dbh->do("INSERT INTO test_case_tags(tag_id, case_id) VALUES(?,?)",
              undef, $tag_id, $self->{'case_id'});
    $dbh->bz_unlock_tables();

    return 0;
}

=head2 remove_tag

Disassociates a tag from this test case

=cut

sub remove_tag {    
    my $self = shift;
    my ($tag_id) = @_;
    my $dbh = Bugzilla->dbh;
    $dbh->do("DELETE FROM test_case_tags 
              WHERE tag_id=? AND case_id=?",
              undef, $tag_id, $self->{'case_id'});
    return;
}

=head2 add_component

Associates a component with this test case

=cut

sub add_component {
    my $self = shift;
    my ($comp_id) = @_;
    my $dbh = Bugzilla->dbh;
    #TODO: Check for existing component
    $dbh->do("INSERT INTO test_case_components (case_id, component_id)
              VALUES (?,?)",undef,  $self->{'case_id'}, $comp_id);    
}

=head2 remove_component

Disassociates a component with this test case

=cut

sub remove_component {
    my $self = shift;
    my ($comp_id) = @_;
    my $dbh = Bugzilla->dbh;
    $dbh->do("DELETE FROM test_case_components
              WHERE case_id = ? AND component_id = ?",
              undef,  $self->{'case_id'}, $comp_id);    
}

=head2 compare_doc_versions

Returns a unified diff of two versions of a case document (action 
and effect). It takes two arguments both integers representing
the first and second versions to compare.

=cut

sub compare_doc_versions {
    my $self = shift;
    my ($newversion, $oldversion) = @_;
    detaint_natural($newversion);
    detaint_natural($oldversion);
    my $dbh = Bugzilla->dbh;
    my %diff;
    my ($newaction, $neweffect) = $dbh->selectrow_array(
            "SELECT action, effect FROM test_case_texts
             WHERE case_id = ? AND case_text_version = ?",
             undef, $self->{'case_id'}, $newversion);
    
    my ($oldaction, $oldeffect) = $dbh->selectrow_array(
            "SELECT action, effect FROM test_case_texts
             WHERE case_id = ? AND case_text_version = ?",
             undef, $self->{'case_id'}, $oldversion);
    $diff{'action'} = diff(\$newaction, \$oldaction);
    $diff{'effect'} = diff(\$neweffect, \$oldeffect);
    return \%diff;
}

=head2 diff_case_doc

Returns the diff of the latest case document (action and effect) 
with some text passed as an argument. Used to determine if the 
text has changed and thus requiring a new version be created.

=cut

sub diff_case_doc {
    my $self = shift;
    my ($newaction, $neweffect) = @_;
    my $dbh = Bugzilla->dbh;
    my ($oldaction, $oldeffect) = $dbh->selectrow_array(
            "SELECT action, effect FROM test_case_texts
             WHERE case_id = ? AND case_text_version = ?",
             undef, $self->{'case_id'}, $self->version);
    my $diff = diff(\$newaction, \$oldaction);
    $diff .= diff(\$neweffect, \$oldeffect);
    return $diff
}

=head2 get_fields

Returns a reference to a list of test case field descriptions from 
the test_fielddefs table. 

=cut

sub get_fields {
    my $self = shift;
    my $dbh = Bugzilla->dbh;    

    my $types = $dbh->selectall_arrayref(
            "SELECT fieldid AS id, description AS name 
             FROM test_fielddefs 
             WHERE table_name=?", 
             {"Slice"=>{}}, "test_cases");
    return $types;
}

=head2 store

Stores a test case object in the database. This method is used to store a 
newly created test case. It returns the new ID.

=cut

sub store {
    my $self = shift;
    my $dbh = Bugzilla->dbh;    
    my ($timestamp) = Bugzilla::Testopia::Util::get_time_stamp();
    $dbh->do("INSERT INTO test_cases ($columns)
              VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?)",
              undef, (undef, $self->{'case_status_id'}, $self->{'category_id'}, 
              $self->{'priority_id'}, $self->{'author_id'}, $self->{'default_tester_id'},
              $timestamp, $self->{'isautomated'}, $self->sortkey, $self->{'script'},
              $self->{'arguments'}, $self->{'summary'}, $self->{'requirement'},
              $self->{'alias'}));

    my $key = $dbh->bz_last_key( 'test_cases', 'case_id' );

    $self->store_text($key, $self->{'author_id'}, $self->{'action'}, $self->{'effect'}, $timestamp);
    $self->update_deps($self->{'dependson'}, $self->{'blocks'}, $key);    
    foreach my $p (@{$self->{'plans'}}){
        $self->link_plan($p->id, $key);
    }
    return $key;
}

=head2 store_text

Stores the test case document (action and effect) in the test_case_texts 
table. Used by both store and copy. Accepts the the test case id,
author id, action text, effect text, and an optional timestamp. 

=cut

sub store_text {
    my $self = shift;
    my $dbh = Bugzilla->dbh;
    my ($key, $author, $action, $effect, $timestamp) = @_;
    if (!defined $timestamp){
        ($timestamp) = Bugzilla::Testopia::Util::get_time_stamp();
    }
    trick_taint($action);
    trick_taint($effect);
    my $version = $self->version || 0;
    $dbh->do("INSERT INTO test_case_texts VALUES(?,?,?,?,?,?)",
              undef, $key, ++$version, $author, 
              $timestamp, $action, $effect);
    $self->{'version'} = $version;
    return $self->{'version'};

}

=head2 link_plan

Creates a link to the specified plan id. Optionally can create a link
for an arbitrary test case, not just this one.

=cut

sub link_plan {
    my $self = shift;
    my $dbh = Bugzilla->dbh;
    my ($plan_id, $case_id) = @_;
    $case_id = $self->{'case_id'} unless defined $case_id;
    
    #Check that it isn't linked already

    $dbh->bz_lock_tables('test_case_plans WRITE');
    my ($is) = $dbh->selectrow_array(
            "SELECT 1 
               FROM test_case_plans
              WHERE case_id = ?
                AND plan_id = ?",
               undef, ($case_id, $plan_id));
    if ($is) {
        $dbh->bz_unlock_tables();
        return;
    }
    $dbh->do("INSERT INTO test_case_plans (plan_id, case_id) 
              VALUES (?,?)", undef, $plan_id, $case_id);
    $dbh->bz_unlock_tables();
}

=head2 copy

Creates a copy of this test case. Accepts the plan id to link to and 
a boolean representing whether to copy the case document as well.

=cut

sub copy {
    my $self = shift;
    my $dbh = Bugzilla->dbh;
    my ($planid, $copydoc) = @_;
    my ($timestamp) = Bugzilla::Testopia::Util::get_time_stamp();
    $dbh->do("INSERT INTO test_cases ($columns)
              VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?)",
              undef, (undef, $self->{'case_status_id'}, $self->{'category_id'}, 
              $self->{'priority_id'}, Bugzilla->user->id, $self->{'default_tester_id'}, 
              $timestamp, $self->{'isautomated'}, $self->sortkey, $self->{'script'},
              $self->{'arguments'}, $self->{'summary'}, $self->{'requirement'},
              undef));

    my $key = $dbh->bz_last_key( 'test_cases', 'case_id' );
    
    if ($copydoc){
        $self->store_text($key, Bugzilla->user->id, $self->text->{'action'}, $self->text->{'effect'}, $timestamp);
    }
    return $key;
    
}

=head2 check_alias

Checks if the given alias exists already. Returns the case_id of
the matching case if it does.

=cut

sub check_alias {
    my $self = shift;
    my ($alias) = @_;
    
    return unless $alias;
    
    my $dbh = Bugzilla->dbh;
    my ($id) = $dbh->selectrow_array(
            "SELECT case_id 
               FROM test_cases 
              WHERE alias = ?
                AND case_id != ?",
              undef, ($alias, $self->{'case_id'}));

    return $id;
}

=head2 update

Updates this test case with new values supplied by the user.
Accepts a reference to a hash with keys identical to a test cases
fields and values representing the new values entered.
Validation tests should be performed on the values 
before calling this method. If a field is changed, a history 
of that change is logged in the test_case_activity table.

=cut

sub update {
    my $self = shift;
    my ($newvalues) = @_;
    my $dbh = Bugzilla->dbh;
    my $timestamp = Bugzilla::Testopia::Util::get_time_stamp();

    $dbh->bz_lock_tables('test_cases WRITE', 'test_case_activity WRITE',
        'test_fielddefs READ');
    foreach my $field (keys %{$newvalues}){
        if ($self->{$field} ne $newvalues->{$field}){
            trick_taint($newvalues->{$field});
            $dbh->do("UPDATE test_cases 
                      SET $field = ? WHERE case_id = ?",
                      undef, $newvalues->{$field}, $self->{'case_id'});
            # Update the history
            my $field_id = Bugzilla::Testopia::Util::get_field_id($field, "test_cases");
            $dbh->do("INSERT INTO test_case_activity 
                      VALUES(?,?,?,?,?,?)",
                      undef, $self->{'case_id'}, $field_id, Bugzilla->user->id,
                      $timestamp, $self->{$field}, $newvalues->{$field});
            $self->{$field} = $newvalues->{$field};
        }
    }
    $dbh->bz_unlock_tables();
}

=head2 history

Returns a reference to a list of history entries from the 
test_case_activity table.

=cut

sub history {
    my $self = shift;
    my $dbh = Bugzilla->dbh;
    my $ref = $dbh->selectall_arrayref(
            "SELECT defs.description AS what, 
                    p.login_name AS who, a.changed, a.oldvalue, a.newvalue
               FROM test_case_activity AS a
               JOIN test_fielddefs AS defs ON a.fieldid = defs.fieldid
               JOIN profiles AS p ON a.who = p.userid
              WHERE a.case_id = ?",
              {'Slice'=>{}}, $self->{'case_id'});

    foreach my $row (@$ref){
        if ($row->{'what'} eq 'Case Status'){
            $row->{'oldvalue'} = $self->lookup_status($row->{'oldvalue'});
            $row->{'newvalue'} = $self->lookup_status($row->{'newvalue'});
        }
        elsif ($row->{'what'} eq 'Category'){
            $row->{'oldvalue'} = $self->lookup_category($row->{'oldvalue'});
            $row->{'newvalue'} = $self->lookup_category($row->{'newvalue'});
        }
        elsif ($row->{'what'} eq 'Priority'){
            $row->{'oldvalue'} = $self->lookup_priority($row->{'oldvalue'});
            $row->{'newvalue'} = $self->lookup_priority($row->{'newvalue'});
        }
        elsif ($row->{'what'} eq 'Default Tester'){
            $row->{'oldvalue'} = $self->lookup_default_tester($row->{'oldvalue'});
            $row->{'newvalue'} = $self->lookup_default_tester($row->{'newvalue'});
        }
    }        
    return $ref;
}

=head2 lookup_status

Takes an ID of the status field and returns the value

=cut

sub lookup_status {
    my $self = shift;
    my ($id) = @_;
    my $dbh = Bugzilla->dbh;
    my ($value) = $dbh->selectrow_array(
            "SELECT name 
               FROM test_case_status
              WHERE case_status_id = ?",
              undef, $id);
    return $value;
}

=head2 lookup_category

Takes an ID of the category field and returns the value

=cut

sub lookup_category {
    my $self = shift;
    my ($id) = @_;
    my $dbh = Bugzilla->dbh;
    my ($value) = $dbh->selectrow_array(
            "SELECT name 
               FROM test_case_categories
              WHERE category_id = ?",
              undef, $id);
    return $value;
}

=head2 lookup_priority

Takes an ID of the priority field and returns the value

=cut

sub lookup_priority {
    my $self = shift;
    my ($id) = @_;
    my $dbh = Bugzilla->dbh;
    my ($value) = $dbh->selectrow_array(
            "SELECT value 
               FROM priority
              WHERE id = ?",
              undef, $id);
    return $value;
}

=head2 lookup_default_tester

Takes an ID of the default_tester field and returns the value

=cut

sub lookup_default_tester {
    my $self = shift;
    my ($id) = @_;
    my $dbh = Bugzilla->dbh;
    my ($value) = $dbh->selectrow_array(
            "SELECT login_name 
               FROM profiles
              WHERE userid = ?",
              undef, $id);
    return $value;
}

# From process bug
sub _snap_shot_deps {
    my ($i, $target, $me) = (@_);
    my $dbh = Bugzilla->dbh;
    my $ref = $dbh->selectcol_arrayref(
        "SELECT $target 
         FROM test_case_dependencies 
         WHERE $me = ? ORDER BY $target", undef, $i);
    
    return join(',', @{$ref});
}

# Taken from Bugzilla::Bug
sub _get_dep_lists {
    my ($myfield, $targetfield, $case_id) = (@_);
    my $dbh = Bugzilla->dbh;
    my $list_ref =
        $dbh->selectcol_arrayref(
          "SELECT test_case_dependencies.$targetfield
             FROM test_case_dependencies
             JOIN test_cases ON test_cases.case_id = test_case_dependencies.$targetfield
            WHERE test_case_dependencies.$myfield = ?
         ORDER BY test_case_dependencies.$targetfield",
         undef, ($case_id));
    return $list_ref;
}

sub update_deps {
    my $self = shift;
    my ($dependson, $blocks, $case_id) = @_;
    $case_id = $self->{'case_id'} unless $case_id;
    my $dbh = Bugzilla->dbh;
    my $fields = {};
    $fields->{'dependson'} = $dependson;
    $fields->{'blocked'} = $blocks;
# From process bug
    foreach my $field ("dependson", "blocked") {
        if ($fields->{$field}) {
            my @validvalues;
            foreach my $id (split(/[\s,]+/, $fields->{$field})) {
                next unless $id;
                Bugzilla::Testopia::Util::validate_test_id($id, 'case');
                push(@validvalues, $id);
            }
            $fields->{$field} = join(",", @validvalues);
        }
    }
#From Bug.pm sub ValidateDependencies($$$)
#    my $fields = {};
#    $fields->{'dependson'} = shift;
#    $fields->{'blocked'} = shift;
    my $id = $case_id || 0;

    unless (defined($fields->{'dependson'})
            || defined($fields->{'blocked'}))
    {
        return;
    }

    my %deps;
    my %deptree;
    foreach my $pair (["blocked", "dependson"], ["dependson", "blocked"]) {
        my ($me, $target) = @{$pair};
        $deptree{$target} = [];
        $deps{$target} = [];
        next unless $fields->{$target};

        my %seen;
        foreach my $i (split('[\s,]+', $fields->{$target})) {
            if ($id == $i) {
                ThrowUserError("dependency_loop_single");
            }
            if (!exists $seen{$i}) {
                push(@{$deptree{$target}}, $i);
                $seen{$i} = 1;
            }
        }
        # populate $deps{$target} as first-level deps only.
        # and find remainder of dependency tree in $deptree{$target}
        @{$deps{$target}} = @{$deptree{$target}};
        my @stack = @{$deps{$target}};
        while (@stack) {
            my $i = shift @stack;
            my $dep_list =
                $dbh->selectcol_arrayref("SELECT $target
                                          FROM test_case_dependencies
                                          WHERE $me = ?", undef, $i);
            foreach my $t (@$dep_list) {
                # ignore any _current_ dependencies involving this test_case,
                # as they will be overwritten with data from the form.
                if ($t != $id && !exists $seen{$t}) {
                    push(@{$deptree{$target}}, $t);
                    push @stack, $t;
                    $seen{$t} = 1;
                }
            }
        }
    }

    my @deps   = @{$deptree{'dependson'}};
    my @blocks = @{$deptree{'blocked'}};
    my @union = ();
    my @isect = ();
    my %union = ();
    my %isect = ();
    foreach my $b (@deps, @blocks) { $union{$b}++ && $isect{$b}++ }
    @union = keys %union;
    @isect = keys %isect;
    if (scalar(@isect) > 0) {
        my $both = "";
        foreach my $i (@isect) {
           $both .= "<a href=\"tr_show_case.cgi?case_id=$i\">$i</a> " ;
        }
        ThrowUserError("dependency_loop_multi", { both => $both });
    }

#from process_bug
    foreach my $pair ("blocked/dependson", "dependson/blocked") {
        my ($me, $target) = split("/", $pair);

        my @oldlist = @{$dbh->selectcol_arrayref("SELECT $target FROM test_case_dependencies
                                                  WHERE $me = ? ORDER BY $target",
                                                  undef, $id)};
#        @dependencychanged{@oldlist} = 1;

        if (defined $fields->{$target}) {
            my %snapshot;
            my @newlist = sort {$a <=> $b} @{$deps{$target}};
#            @dependencychanged{@newlist} = 1;

            while (0 < @oldlist || 0 < @newlist) {
                if (@oldlist == 0 || (@newlist > 0 &&
                                      $oldlist[0] > $newlist[0])) {
                    $snapshot{$newlist[0]} = _snap_shot_deps($newlist[0], $me,
                                                          $target);
                    shift @newlist;
                } elsif (@newlist == 0 || (@oldlist > 0 &&
                                           $newlist[0] > $oldlist[0])) {
                    $snapshot{$oldlist[0]} = _snap_shot_deps($oldlist[0], $me,
                                                          $target);
                    shift @oldlist;
                } else {
                    if ($oldlist[0] != $newlist[0]) {
                        die "Error in list comparing code";
                    }
                    shift @oldlist;
                    shift @newlist;
                }
            }
            my @keys = keys(%snapshot);
            if (@keys) {
                my $oldsnap = _snap_shot_deps($id, $target, $me);
                $dbh->do("DELETE FROM test_case_dependencies WHERE $me = ?", undef, $id);
                foreach my $i (@{$deps{$target}}) {
                    $dbh->do("INSERT INTO test_case_dependencies ($me, $target) VALUES (?,?)", undef, $id, $i);
                }
#                foreach my $k (@keys) {
#                    LogDependencyActivity($k, $snapshot{$k}, $me, $target, $timestamp);
#                }
#                LogDependencyActivity($id, $oldsnap, $target, $me, $timestamp);
#                $check_dep_bugs = 1;
            }
        }
    }  
}

=head2 canedit

Returns true if the logged in user has rights to edit this test case.

=cut

sub canedit {
    my $self = shift;
    return $self->canview 
      && UserInGroup('managetestplans') 
        || UserInGroup('edittestcases');
}

=head2 canview

Returns true if the logged in user has rights to view this test case.

=cut

sub canview {
    my $self = shift;
    return $self->{'canview'} if exists $self->{'canview'};
    my $ret = 1;
    foreach my $p (@{$self->plans}){
        $ret = 0 unless Bugzilla::Testopia::Util::can_view_product($p->product_id);
    }
    $self->{'canview'} = $ret;
    return $self->{'canview'};
}

=head2 candelete

Returns true if the logged in user has rights to delete this test case.

=cut

sub candelete {
    my $self = shift;
    return $self->canedit && Param("allow-test-deletion");
}

###############################
####      Accessors        ####
###############################

=head1 ACCESSOR METHODS

=head2 id

Returns the ID of this object

=head2 author

Returns a Bugzilla::User object representing the Author of this case

=head2 default_tester

Returns a Bugzilla::User object representing the run's default tester

=head2 creation_date

Returns the creation time stamp of this object

=head2 isautomated

Returns true if this is an automatic test case

=head2 script

Returns the script of this object

=head2 status_id

Returns the status_id of this object

=head2 arguments

Returns the arguments for the script of this object

=head2 summary

Returns the summary of this object

=head2 requirements

Returns the requirements of this object

=head2 alias

Returns the alias of this object

=cut

sub id              { return $_[0]->{'case_id'};       }
sub author          { return Bugzilla::User->new($_[0]->{'author_id'});  }
sub default_tester  { return Bugzilla::User->new($_[0]->{'default_tester_id'});  }
sub creation_date   { return $_[0]->{'creation_date'}; }
sub isautomated     { return $_[0]->{'isautomated'};   }
sub script          { return $_[0]->{'script'};        }
sub status_id       { return $_[0]->{'case_status_id'};}
sub arguments       { return $_[0]->{'arguments'};     }
sub summary         { return $_[0]->{'summary'};       }
sub requirement     { return $_[0]->{'requirement'};   }
sub alias           { return $_[0]->{'alias'};         }

=head2 type

Returns 'case'

=cut

sub type {
    my $self = shift;
    $self->{'type'} = 'case';
    return $self->{'type'};
}

=head2 attachments

Returns a reference to a list of attachments associated with this
case.

=cut

sub attachments {
    my ($self) = @_;
    my $dbh = Bugzilla->dbh;
    return $self->{'attachments'} if exists $self->{'attachments'};

    my $attachments = $dbh->selectcol_arrayref(
        "SELECT attachment_id
         FROM test_attachments
         WHERE case_id = ?", 
         undef, $self->{'case_id'});
    
    my @attachments;
    foreach my $a (@{$attachments}){
        push @attachments, Bugzilla::Testopia::Attachment->new($a);
    }
    $self->{'attachments'} = \@attachments;
    return $self->{'attachments'};
    
}

=head2 version

Returns the case text version. This number is incremented any time
changes are made to the case docs (action and effect).

=cut

sub version { 
    my $self = shift;
    my $dbh = Bugzilla->dbh;
    return $self->{'version'} if exists $self->{'version'};
    my ($ver) = $dbh->selectrow_array("SELECT MAX(case_text_version)
                                       FROM test_case_texts
                                       WHERE case_id = ?", 
                                       undef, $self->{'case_id'});
    $self->{'version'} = $ver;
    return $self->{'version'};
}

=head2 status

Looks up the case status based on the case_status_id of this case.

=cut

sub status { 
    my $self = shift;
    my $dbh = Bugzilla->dbh;
    return $self->{'status'} if exists $self->{'status'};
    my ($res) = $dbh->selectrow_array("SELECT name
                                       FROM test_case_status
                                       WHERE case_status_id = ?", 
                                       undef, $self->{'case_status_id'});
    $self->{'status'} = $res;
    return $self->{'status'};
}

=head2 priority

Looks up the Bugzilla priority value based on the priority_id 
of this case.

=cut

sub priority { 
    my $self = shift;
    my $dbh = Bugzilla->dbh;
    return $self->{'priority'} if exists $self->{'priority'};
    my ($res) = $dbh->selectrow_array("SELECT value
                                       FROM priority
                                       WHERE id = ?", 
                                       undef, $self->{'priority_id'});
    $self->{'priority'} = $res;
    return $self->{'priority'};
}

=head2 category

Returns the category name based on the category_id of this case

=cut

sub category {
    my $self = shift;
    return $self->{'category'} if exists $self->{'category'};
    $self->{'category'} = Bugzilla::Testopia::Category->new($self->{'category_id'});
    return $self->{'category'};
}

=head2 components

Returns a reference to a list of bugzilla components assoicated with 
this test case.

=cut

sub components {
    my $self = shift;
    my $dbh = Bugzilla->dbh;
    return $self->{'components'} if exists $self->{'components'};
    #TODO: 2.22 use Bugzilla::Component
    my $comps = $dbh->selectall_arrayref(
        "SELECT comp.id, comp.name 
           FROM components AS comp
           JOIN test_case_components AS tcc ON tcc.component_id = comp.id
           JOIN test_cases ON tcc.case_id = test_cases.case_id
          WHERE test_cases.case_id = ?", 
         {'Slice' => {}},  $self->{'case_id'}); 
    
    
    $self->{'components'} = $comps;
    return $self->{'components'};
}

=head2 tags

Returns a reference to a list of Bugzilla::Testopia::TestTag objects 
associated with this case.

=cut

sub tags {
    my $self = shift;
    my $dbh = Bugzilla->dbh;
    return $self->{'tags'} if exists $self->{'tags'};
    my $tagids = $dbh->selectcol_arrayref("SELECT tag_id 
                                          FROM test_case_tags
                                          WHERE case_id = ?", 
                                          undef, $self->{'case_id'});
    my @tags;
    foreach my $id (@{$tagids}){
        push @tags, Bugzilla::Testopia::TestTag->new($id);
    }
    $self->{'tags'} = \@tags;
    return $self->{'tags'};
}

=head2 plans

Returns a reference to a list of Bugzilla::Testopia::TestPlan objects 
associated with this case.

=cut

sub plans {
    my $self = shift;
    my $dbh = Bugzilla->dbh;
    return $self->{'plans'} if exists $self->{'plans'};
    my $ref = $dbh->selectcol_arrayref("SELECT plan_id
                                       FROM test_case_plans
                                       WHERE case_id = ?", 
                                       undef, $self->{'case_id'});
    my @plans;
    foreach my $id (@{$ref}){
        push @plans, Bugzilla::Testopia::TestPlan->new($id);
    }
    $self->{'plans'} = \@plans;
    return $self->{'plans'};
}

=head2 plans

Returns a reference to a list of Bugzilla::Bug objects 
associated with this case.

=cut

sub bugs {
    my $self = shift;
    my $dbh = Bugzilla->dbh;
    return $self->{'bugs'} if exists $self->{'bugs'};
    my $ref = $dbh->selectcol_arrayref("SELECT bug_id
                                       FROM test_case_bugs b
                                       JOIN  test_case_runs r
                                       ON r.case_run_id = b.case_run_id
                                       WHERE case_id = ?", 
                                       undef, $self->{'case_id'});
    my @bugs;
    foreach my $id (@{$ref}){
        push @bugs, Bugzilla::Bug->new($id, Bugzilla->user->id);
    }
    $self->{'bugs'} = \@bugs;
    return $self->{'bugs'};
}

=head2 text

Returns a hash reference representing the action and effect of this
case.

=cut


sub text {
    my $self = shift;
    my $dbh = Bugzilla->dbh;
    return $self->{'text'} if exists $self->{'text'};
    my $text = $dbh->selectrow_hashref("SELECT action, effect
                                        FROM test_case_texts
                                        WHERE case_id=? AND case_text_version=?",
                                        undef, $self->{'case_id'}, 
                                        $self->version);
    $self->{'text'} = $text;
    return $self->{'text'};
}

=head2 runs

Returns a reference to a list of Bugzilla::Testopia::TestRun objects 
associated with this case.

=cut

sub runs {
    my $self = shift;
    my $dbh = Bugzilla->dbh;
    return $self->{'runs'} if exists $self->{'runs'};
    my $ref = $dbh->selectcol_arrayref("SELECT t.run_id
                                       FROM test_runs t
                                       INNER JOIN  test_case_runs r
                                       ON r.run_id = t.run_id
                                       WHERE case_id = ?", 
                                       undef, $self->{'case_id'});
    my @runs;
    foreach my $id (@{$ref}){
        push @runs, Bugzilla::Testopia::TestRun->new($id);
    }
    $self->{'runs'} = \@runs;
    return $self->{'runs'};
}

=head2 caseruns

Returns a reference to a list of Bugzilla::Testopia::TestCaseRun objects 
associated with this case.

=cut

sub caseruns {
    my $self = shift;
    my $dbh = Bugzilla->dbh;
    return $self->{'caseruns'} if exists $self->{'caseruns'};
    my $ref = $dbh->selectcol_arrayref("SELECT case_run_id
                                        FROM test_case_runs 
                                        WHERE case_id = ?", 
                                        undef, $self->{'case_id'});
    my @runs;
    foreach my $id (@{$ref}){
        push @runs, Bugzilla::Testopia::TestCaseRun->new($id);
    }
    $self->{'caseruns'} = \@runs;
    return $self->{'caseruns'};
}

sub sortkey {
    my $self = shift;
    return $self->{'sortkey'} if exists $self->{'sortkey'};
    my $dbh = Bugzilla->dbh;
    my ($sortkey) = $dbh->selectrow_array("SELECT MAX(sortkey) FROM test_cases");
    $self->{'sortkey'} = ++$sortkey;
    return $self->{'sortkey'};
}

=head2 blocked

Returns a reference to a list of Bugzilla::Testopia::TestCase objects 
which are blocked by this test case.

=cut

sub blocked {
    my ($self) = @_;
    return $self->{'blocked'} if exists $self->{'blocked'};
    my @deps;
    my $ref = _get_dep_lists("dependson", "blocked", $self->{'case_id'});
    foreach my $id (@{$ref}){
        push @deps, Bugzilla::Testopia::TestCase->new($id);
    }
    $self->{'blocked'} = \@deps;
    return $self->{'blocked'};
}

sub blocked_list {
    my ($self) = @_;
    return $self->{'blocked_list'} if exists $self->{'blocked_list'};
    my @deps;
    my $ref = _get_dep_lists("dependson", "blocked", $self->{'case_id'});
    $self->{'blocked_list'} = join(",", @$ref);
    return $self->{'blocked_list'};
} 

=head2 dependson

Returns a reference to a list of Bugzilla::Testopia::TestCase objects 
which depend on this test case.

=cut

sub dependson {
    my ($self) = @_;
    return $self->{'dependson'} if exists $self->{'dependson'};
    my @deps;
    my $ref = _get_dep_lists("blocked", "dependson", $self->{'case_id'});
    foreach my $id (@{$ref}){
        push @deps, Bugzilla::Testopia::TestCase->new($id);
    }
    $self->{'dependson'} = \@deps;
    return $self->{'dependson'};
}    

sub dependson_list {
    my ($self) = @_;
    return $self->{'dependson_list'} if exists $self->{'dependson_list'};
    my @deps;
    my $ref = _get_dep_lists("blocked", "dependson", $self->{'case_id'});
    $self->{'dependson_list'} = join(",", @$ref);
    return $self->{'dependson_list'};
}    


=head1 TODO

Move components to use Bugzilla::Components with 2.22

=head1 SEE ALSO

TestPlan, TestRun, TestCaseRun

=head1 AUTHOR

Greg Hendricks <ghendricks@novell.com>

=cut

1;
