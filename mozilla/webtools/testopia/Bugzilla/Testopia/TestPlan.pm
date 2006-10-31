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

=head1 NAME

Bugzilla::Testopia::TestPlan - Testopia Test Plan object

=head1 DESCRIPTION

This module represents a test plan in Testopia. The test plan
is the glue of testopia. Virtually all other objects associate 
to a plan.

=head1 SYNOPSIS

use Bugzilla::Testopia::TestPlan;

 $plan = Bugzilla::Testopia::TestPlan->new($plan_id);
 $plan = Bugzilla::Testopia::TestPlan->new(\%plan_hash);

=cut

package Bugzilla::Testopia::TestPlan;

use strict;

use Bugzilla::User;
use Bugzilla::Util;
use Bugzilla::Error;
use Bugzilla::Config;
use Bugzilla::Testopia::Util;
use Bugzilla::Testopia::TestRun;
use Bugzilla::Testopia::TestCase;
use Bugzilla::Testopia::Category;
use Bugzilla::Testopia::Build;
use Bugzilla::Testopia::TestTag;
use Bugzilla::Bug;

#TODO: Add this to checksetup
use Text::Diff;

use base qw(Exporter);

###############################
####    Initialization     ####
###############################

=head1 FIELDS

    plan_id
    product_id
    author_id
    type_id
    default_product_version
    name
    creation_date
    isactive

=cut

use constant DB_COLUMNS => qw(
    test_plans.plan_id
    test_plans.product_id
    test_plans.author_id
    test_plans.type_id
    test_plans.default_product_version
    test_plans.name
    test_plans.creation_date
    test_plans.isactive
);

our $columns = join(", ", DB_COLUMNS);

sub report_columns {
    my $self = shift;
    my %columns;
    # Changes here need to match Report.pm
    $columns{'Type'}          = "plan_type";        
    $columns{'Version'}       = "default_product_version";
    $columns{'Product'}       = "product";
    $columns{'Archived'}      = "archived";
    $columns{'Tags'}          = "tags";
    $columns{'Author'}        = "author";
    my @result;
    push @result, {'name' => $_, 'id' => $columns{$_}} foreach (sort(keys %columns));
    unshift @result, {'name' => '<none>', 'id'=> ''};
    return \@result;     
        
}

###############################
####       Methods         ####
###############################

=head2 new

Instantiate a new test plan. This takes a single argument 
either a test plan ID or a reference to a hash containing keys 
identical to a test plan's fields and desired values.

=cut

sub new {
    my $invocant = shift;
    my $class = ref($invocant) || $invocant;
    my $self = {};
    bless($self, $class);
    return $self->_init(@_);
}

=head2 _init

Private constructor for this object 

=cut

sub _init {
    my $self = shift;
    my ($param) = (@_);
    my $dbh = Bugzilla->dbh;

    my $id = $param unless (ref $param eq 'HASH');
    my $obj;

    if (defined $id && detaint_natural($id)) {

        $obj = $dbh->selectrow_hashref(qq{
            SELECT $columns FROM test_plans
            WHERE plan_id = ?}, undef, $id);
    } elsif (ref $param eq 'HASH'){
         $obj = $param;   

    } else {
        ThrowCodeError('bad_arg',
            {argument => 'param',
             function => 'Testopia::TestRun::_init'});
    }

    return undef unless (defined $obj);

    foreach my $field (keys %$obj) {
        $self->{$field} = $obj->{$field};
    }
    return $self;
}

=head2 store

Stores a test plan object in the database. This method is used to store a 
newly created test plan. It returns the new ID.

=cut

sub store {
    my $self = shift;
    my $dbh = Bugzilla->dbh;
    my $timestamp = Bugzilla::Testopia::Util::get_time_stamp();
    $dbh->do("INSERT INTO test_plans ($columns)
              VALUES (?,?,?,?,?,?,?,?)",
              undef, (undef, $self->{'product_id'}, $self->{'author_id'}, 
              $self->{'type_id'}, $self->{'default_product_version'}, $self->{'name'},
              $timestamp, 1));
    my $key = $dbh->bz_last_key( 'test_plans', 'plan_id' );
    $self->store_text($key, $self->{'author_id'}, $self->text, $timestamp);
    $self->{'plan_id'} = $key;
    return $key;
}

=head2 store_text

Stores the test plan document in the test_plan_texts 
table. Used by both store and copy. Accepts the the test plan id,
author id, text, and a an optional timestamp. 

=cut

sub store_text {
    my $self = shift;
    my $dbh = Bugzilla->dbh;
    my ($key, $author, $text, $timestamp) = @_;
    if (!defined $timestamp){
        ($timestamp) = Bugzilla::Testopia::Util::get_time_stamp();
    }
    $text ||= '';
    trick_taint($text);
    my $version = $self->version || 0;
    $dbh->do("INSERT INTO test_plan_texts 
              (plan_id, plan_text_version, who, creation_ts, plan_text)
              VALUES(?,?,?,?,?)",
              undef, $key, ++$version, $author, 
              $timestamp, $text);
    $self->{'version'} = $version;
    return $self->{'version'};

}

=head2 clone

Creates a copy of this test plan. Accepts the name of the new plan
and a boolean representing whether to copy the plan document as well.

=cut

sub clone {
    my $self = shift;
    my ($name, $store_doc) = @_;
    $store_doc = 1 unless defined($store_doc);
    my $dbh = Bugzilla->dbh;
    my ($timestamp) = Bugzilla::Testopia::Util::get_time_stamp();
    $dbh->do("INSERT INTO test_plans ($columns)
              VALUES (?,?,?,?,?,?,?,?)",
              undef, (undef, $self->{'product_id'}, Bugzilla->user->id, 
              $self->{'type_id'}, $self->{'default_product_version'}, $name,
              $timestamp, 1));
    my $key = $dbh->bz_last_key( 'test_plans', 'plan_id' );
    if ($store_doc){
        $self->store_text($key, $self->{'author_id'}, $self->text, $timestamp);
    }
    return $key;
    
}

=head2 toggle_archive

Toggles the archive bit on the plan.

=cut

sub toggle_archive {
    my $self = shift;
    my $dbh = Bugzilla->dbh;
    
    my $oldvalue = $self->isactive;
    my $newvalue = $oldvalue == 1 ? 0 : 1;
    my $timestamp = Bugzilla::Testopia::Util::get_time_stamp();

    $dbh->bz_lock_tables('test_plans WRITE', 'test_plan_activity WRITE',
            'test_fielddefs READ');
    $dbh->do("UPDATE test_plans SET isactive = ? 
               WHERE plan_id = ?", undef, $newvalue, $self->{'plan_id'});
    my $field_id = Bugzilla::Testopia::Util::get_field_id("isactive", "test_plans");
    $dbh->do("INSERT INTO test_plan_activity 
              (plan_id, fieldid, who, changed, oldvalue, newvalue)
              VALUES(?,?,?,?,?,?)",
              undef, ($self->{'plan_id'}, $field_id, Bugzilla->user->id,
              $timestamp, $oldvalue, $newvalue));
    $dbh->bz_unlock_tables();

    $self->{'isactive'} = $newvalue;
}

=head2 add_tag

Associates a tag with this test plan. Takes the tag_id of the tag
to link.

=cut

sub add_tag {
    my $self = shift;
    my ($tag_id) = @_;
    my $dbh = Bugzilla->dbh;

    $dbh->bz_lock_tables('test_plan_tags WRITE');
    my $tagged = $dbh->selectrow_array(
             "SELECT 1 FROM test_plan_tags 
               WHERE tag_id = ? AND plan_id = ?",
              undef, ($tag_id, $self->{'plan_id'}));
    if ($tagged) {
        $dbh->bz_unlock_tables();
        return 1;
    }
    $dbh->do("INSERT INTO test_plan_tags(tag_id, plan_id, userid) VALUES(?,?,?)",
              undef, ($tag_id, $self->{'plan_id'}), Bugzilla->user->id);
    $dbh->bz_unlock_tables();

}

=head2 remove_tag

Removes a tag from this plan. Takes the tag_id of the tag to remove.

=cut

sub remove_tag {    
    my $self = shift;
    my ($tag_id) = @_;
    my $dbh = Bugzilla->dbh;
    $dbh->do("DELETE FROM test_plan_tags 
               WHERE tag_id=? AND plan_id=?",
              undef, ($tag_id, $self->{'plan_id'}));
}

=head2 get_available_products

Returns a list of user visible products

=cut

sub get_available_products {
    my $self = shift;
    my $dbh = Bugzilla->dbh;
    
    # TODO 2.22 use Product.pm
    my $products = $dbh->selectall_arrayref(
            "SELECT id, name 
               FROM products
           ORDER BY name", 
             {"Slice"=>{}});
    my @selectable;
    foreach my $p (@{$products}){
        if (Bugzilla::Testopia::Util::can_view_product($p->{'id'})){
            push @selectable, $p;
        }
    }
    return \@selectable;
}

=head2 get_product_versions

Returns a list of versions for the given product. If one is not
specified, use the plan product

=cut

sub get_product_versions {
    my $self = shift;
    my ($product) = @_;
    $product = $self->{'product_id'} unless $product;
    $product ||= 0;
    my $dbh = Bugzilla->dbh;
    #TODO: 2.22 use product->versions
    # Can't use placeholders as this could be a single id (integer)
    # or a comma separated list (string)
    my $versions = $dbh->selectall_arrayref(
            "SELECT DISTINCT value AS id, value AS name
               FROM versions
              WHERE product_id IN($product)
           ORDER BY name", 
             {'Slice'=>{}});
             
    return $versions;
}

=head2 get_product_milestones

Returns al list of product milestones for the given product. If one
is not specified, use the plan product.

=cut

sub get_product_milestones {
#TODO: 2.22 use product.pm
    my $self = shift;
    my ($product_id) = @_;
    $product_id ||= $self->{'product_id'};
    my $dbh = Bugzilla->dbh;
    my $ref = $dbh->selectall_arrayref(
            "SELECT DISTINCT value AS id, value AS name 
               FROM milestones
              WHERE product_id IN($product_id)
           ORDER BY sortkey",
           {'Slice'=>{}});
           
    return $ref;
}

=head2 get_product_builds

Returns al list of builds for the given product. If one
is not specified, use the plan product.

=cut

sub get_product_builds {
#TODO: 2.22 use product.pm
    my $self = shift;
    my ($product_id) = @_;
    $product_id ||= $self->{'product_id'};
    my $dbh = Bugzilla->dbh;
    my $ref = $dbh->selectall_arrayref(
            "SELECT DISTINCT name AS id, name 
               FROM test_builds
              WHERE product_id IN($product_id)
           ORDER BY name",
           {'Slice'=>{}});
           
    return $ref;
}

=head2 get_product_categories

Returns a list of product categories for the given product. If one
is not specified, use the plan product.

=cut

sub get_product_categories {
#TODO: 2.22 use product.pm
    my $self = shift;
    my ($product_id) = @_;
    $product_id ||= $self->{'product_id'};
    my $dbh = Bugzilla->dbh;
    my $ref = $dbh->selectall_arrayref(
            "SELECT DISTINCT name AS id, name 
               FROM test_case_categories
              WHERE product_id IN($product_id)
           ORDER BY name",
           {'Slice'=>{}});
           
    return $ref;
}

=head2 get_product_components

Returns a list of product components for the given product. If one
is not specified, use the plan product.

=cut

sub get_product_components {
#TODO: 2.22 use product.pm
    my $self = shift;
    my ($product_id) = @_;
    $product_id ||= $self->{'product_id'};
    my $dbh = Bugzilla->dbh;
    my $ref = $dbh->selectall_arrayref(
            "SELECT DISTINCT name AS id, name 
               FROM components
              WHERE product_id IN($product_id)
           ORDER BY name",
           {'Slice'=>{}});
           
    return $ref;
}

=head2 get_product_environments

Returns al list of environments for the given product. If one
is not specified, use the plan product.

=cut

sub get_product_environments {
#TODO: 2.22 use product.pm
    my $self = shift;
    my ($product_id) = @_;
    $product_id ||= $self->{'product_id'};
    my $dbh = Bugzilla->dbh;
    my $ref = $dbh->selectall_arrayref(
            "SELECT DISTINCT name AS id, name 
               FROM test_environments
              WHERE product_id IN($product_id)
           ORDER BY name",
           {'Slice'=>{}});
           
    return $ref;
}


=head2 get_case_ids_by_category

Returns a list of case_ids in this plan from the selected categories.

=cut

sub get_case_ids_by_category {
    my $self = shift;
    my ($categories) = @_;
    my $dbh = Bugzilla->dbh;
    
    my $ref = $dbh->selectcol_arrayref(
        "SELECT DISTINCT test_cases.case_id from test_cases
           JOIN test_case_plans AS tcp ON tcp.case_id = test_cases.case_id
          WHERE tcp.plan_id = ?
            AND category_id in (". join(',',@$categories) . ")",
           undef, $self->id);
    
    return $ref; 
}

=head2 get_plan_types

Returns a list of types from the test_plan_types table

=cut

sub get_plan_types {
    my $self = shift;
    my $dbh = Bugzilla->dbh;    

    my $types = $dbh->selectall_arrayref(
            "SELECT type_id AS id, name 
               FROM test_plan_types
           ORDER BY name", 
            {"Slice"=>{}});
    return $types;

}

=head2 last_changed

Returns the date of the last change in the history table

=cut

sub last_changed {
    my $self = shift;
    my $dbh = Bugzilla->dbh;
    
    my ($date) = $dbh->selectrow_array(
            "SELECT MAX(changed)
               FROM test_plan_activity 
              WHERE plan_id = ?",
              undef, $self->id);

    return $self->{'creation_date'} unless $date;
    return $date;
}

=head2 lookup_plan_type

Returns a type name matching the given type id

=cut

sub lookup_plan_type {
    my $self = shift;
    my $type_id = shift;
    my $dbh = Bugzilla->dbh;    

    my $type = $dbh->selectrow_hashref(
            "SELECT type_id AS id, name 
               FROM test_plan_types
              WHERE type_id = ?", 
            undef, $type_id);
    
    return $type;
}

=head2 check_plan_type

Returns true if a type with the given name exists

=cut

sub check_plan_type {
    my $self = shift;
    my $name = shift;
    my $dbh = Bugzilla->dbh;    

    my $type = $dbh->selectrow_hashref(
            "SELECT 1 
               FROM test_plan_types
              WHERE name = ?", 
            undef, $name);
    
    return $type;
}

=head2 update_plan_type

Update the given type

=cut

sub update_plan_type {
    my $self = shift;
    my ($type_id, $name) = @_;
    my $dbh = Bugzilla->dbh;    

    my $type = $dbh->do(
            "UPDATE test_plan_types
                SET name = ? 
              WHERE type_id = ?", 
            undef, ($name,$type_id));
    
}

=head2 add_plan_type

Add the given type

=cut

sub add_plan_type {
    my $self = shift;
    my ($name) = @_;
    my $dbh = Bugzilla->dbh;    

    my $type = $dbh->do(
            "INSERT INTO test_plan_types (type_id, name)
                VALUES(?,?)", 
            undef, (undef, $name));
}

=head2 get_fields

Returns a list of fields from the fielddefs table associated with
a plan

=cut

sub get_fields {
    my $self = shift;
    my $dbh = Bugzilla->dbh;    

    my $types = $dbh->selectall_arrayref(
            "SELECT fieldid AS id, description AS name 
               FROM test_fielddefs 
              WHERE table_name=?", 
            {"Slice"=>{}}, "test_plans");
    return $types;
}

=head2 get_plan_versions

Returns the list of versions of the plan document.

=cut

sub get_plan_versions {
    my $self = shift;
    my $dbh = Bugzilla->dbh;    

    my $versions = $dbh->selectall_arrayref(
            "SELECT plan_text_version AS id, plan_text_version AS name 
               FROM test_plan_texts
              WHERE plan_id = ?", 
            {'Slice' =>{}}, $self->id);
    return $versions;
}

=head2 diff_plan_doc

Returns either the diff of the latest version with a new text
or two numerical versions.

=cut

sub diff_plan_doc {
    my $self = shift;
    my ($new, $old) = @_;
    $old ||= $self->version;
    my $dbh = Bugzilla->dbh;
    my $newdoc;
    my $text = $new;
    if (detaint_natural($new)){
        # we are looking for a version 
        $newdoc = $dbh->selectrow_array(
            "SELECT plan_text FROM test_plan_texts
              WHERE plan_id = ? AND plan_text_version = ?",
            undef, ($self->{'plan_id'}, $new));
    }
    else {
        $newdoc = $text;
    }
    detaint_natural($old);
    my $olddoc = $dbh->selectrow_array(
            "SELECT plan_text FROM test_plan_texts
              WHERE plan_id = ? AND plan_text_version = ?",
            undef, ($self->{'plan_id'}, $old));
    my $diff = diff(\$newdoc, \$olddoc);
    return $diff
}

=head2 update

Updates this test plan with new values supplied by the user.
Accepts a reference to a hash with keys identical to a test plan's
fields and values representing the new values entered.
Validation tests should be performed on the values 
before calling this method. If a field is changed, a history 
of that change is logged in the test_plan_activity table.

=cut

sub update {
    my $self = shift;
    my ($newvalues) = @_;
    my $dbh = Bugzilla->dbh;
    my $timestamp = Bugzilla::Testopia::Util::get_time_stamp();

    $dbh->bz_lock_tables('test_plans WRITE', 'test_plan_activity WRITE',
            'test_fielddefs READ');
    foreach my $field (keys %{$newvalues}){
        if ($self->{$field} ne $newvalues->{$field}){
            trick_taint($newvalues->{$field});
            $dbh->do("UPDATE test_plans 
                         SET $field = ? 
                       WHERE plan_id = ?",
                      undef, ($newvalues->{$field}, $self->{'plan_id'}));
            # Update the history
            my $field_id = Bugzilla::Testopia::Util::get_field_id($field, "test_plans");
            $dbh->do("INSERT INTO test_plan_activity 
                      (plan_id, fieldid, who, changed, oldvalue, newvalue)
                           VALUES(?,?,?,?,?,?)",
                      undef, ($self->{'plan_id'}, $field_id, Bugzilla->user->id,
                      $timestamp, $self->{$field}, $newvalues->{$field}));
            $self->{$field} = $newvalues->{$field};
            
        }
    }
    $dbh->bz_unlock_tables();
}

=head2 history

Returns a reference to a list of history entries from the 
test_plan_activity table.

=cut

sub history {
    my $self = shift;
    my $dbh = Bugzilla->dbh;
    my $ref = $dbh->selectall_arrayref(
            "SELECT defs.description AS what, 
                    p.login_name AS who, a.changed, a.oldvalue, a.newvalue
               FROM test_plan_activity AS a
               JOIN test_fielddefs AS defs ON a.fieldid = defs.fieldid
               JOIN profiles AS p ON a.who = p.userid
              WHERE a.plan_id = ?",
              {'Slice'=>{}}, $self->{'plan_id'});
    foreach my $row (@$ref){
        if ($row->{'what'} eq 'Product'){
            $row->{'oldvalue'} = $self->lookup_product($row->{'oldvalue'});
            $row->{'newvalue'} = $self->lookup_product($row->{'newvalue'});
        }
        elsif ($row->{'what'} eq 'Plan Type'){
            $row->{'oldvalue'} = $self->lookup_type($row->{'oldvalue'});
            $row->{'newvalue'} = $self->lookup_type($row->{'newvalue'});
        }
    }        
    return $ref;
}

=head2 lookup_type

Takes an ID of the type field and returns the value

=cut

sub lookup_type {
    my $self = shift;
    my ($id) = @_;
    my $dbh = Bugzilla->dbh;
    my ($value) = $dbh->selectrow_array(
            "SELECT name 
               FROM test_plan_types
              WHERE type_id = ?",
              undef, $id);
    return $value;
}

=head2 lookup_type_by_name

Returns the id of the type name passed.

=cut

sub lookup_type_by_name {
    my ($name) = @_;
    my $dbh = Bugzilla->dbh;
    
    my ($value) = $dbh->selectrow_array(
            "SELECT type_id
			 FROM test_plan_types
			 WHERE name = ?",
			 undef, $name);
    return $value;
}

=head2 lookup_status

Takes an ID of the status field and returns the value

=cut

sub lookup_product {
    my $self = shift;
    my ($id) = @_;
    my $dbh = Bugzilla->dbh;
    my ($value) = $dbh->selectrow_array(
            "SELECT name 
               FROM products
              WHERE id = ?",
              undef, $id);
    return $value;
}

=head2 lookup_product_by_name

Returns the id of the product name passed.

=cut

sub lookup_product_by_name {
    my ($name) = @_;
    my $dbh = Bugzilla->dbh;
    
    # TODO 2.22 use Product.pm
    my ($value) = $dbh->selectrow_array(
            "SELECT id
			 FROM products
			 WHERE name = ?",
			 undef, $name);
    return $value;
}



=head2 obliterate

Removes this plan and all things that reference it.

=cut

sub obliterate {
    my $self = shift;
    return 0 unless $self->candelete;
    my $dbh = Bugzilla->dbh;

    $dbh->do("DELETE FROM test_plan_texts WHERE plan_id = ?", undef, $self->id);
    $dbh->do("DELETE FROM test_plan_tags WHERE plan_id = ?", undef, $self->id);
    $dbh->do("DELETE FROM test_plan_group_map WHERE plan_id = ?", undef, $self->id);
    $dbh->do("DELETE FROM test_plan_activity WHERE plan_id = ?", undef, $self->id);
    $dbh->do("DELETE FROM test_case_plans WHERE plan_id = ?", undef, $self->id);
    
    foreach my $obj (@{$self->attachments}){
        $obj->obliterate;
    }
    foreach my $obj (@{$self->test_runs}){
        $obj->obliterate;
    }
    foreach my $obj (@{$self->test_cases}){
        $obj->obliterate if (scalar @{$obj->plans} == 1);
    }
    
    $dbh->do("DELETE FROM test_plans WHERE plan_id = ?", undef, $self->id);
    return 1;
}

=head2 canedit

Returns true if the logged in user has rights to edit this plan

=cut

sub canedit {
    my $self = shift;
    return $self->canview && UserInGroup("managetestplans");
}

=head2 canview

Returns true if the logged in user has rights to view this plan

=cut

sub canview {
    my $self = shift;
    return 1 if (Bugzilla->user->id == $self->author->id); 
    return Bugzilla::Testopia::Util::can_view_product($self->product_id);
}

=head2 candelete

Returns true if the logged in user has rights to delete this plan

=cut

sub candelete {
    my $self = shift;
    return $self->canedit && Param("allow-test-deletion")
      && (Bugzilla->user->id == $self->author->id || Bugzilla->user->in_group('admin'));    
}
  
    
###############################
####      Accessors        ####
###############################
=head1 ACCESSOR METHODS

=head2 id

Returns the ID for this object

=head2 creation_date

Returns the creation timestamp for this object

=head2 product_version

Returns the product version for this object

=head2 product_id

Returns the product id for this object

=head2 author

Returns a Bugzilla::User object representing the plan author

=head2 name

Returns the name of this plan

=head2 type_id

Returns the type id of this plan

=head2 isactive

Returns true if this plan is not archived

=cut

sub id              { return $_[0]->{'plan_id'};          }
sub creation_date   { return $_[0]->{'creation_date'};    }
sub product_version { return $_[0]->{'default_product_version'};  }
sub product_id      { return $_[0]->{'product_id'};       }
sub author          { return Bugzilla::User->new($_[0]->{'author_id'});  }
sub name            { return $_[0]->{'name'};    }
sub type_id         { return $_[0]->{'type_id'};    }
sub isactive        { return $_[0]->{'isactive'};  }

=head2 type

Returns 'case'

=cut

sub type {
    my $self = shift;
    $self->{'type'} = 'plan';
    return $self->{'type'};
}

=head2 attachments

Returns a reference to a list of attachments on this plan

=cut

sub attachments {
    my ($self) = @_;
    my $dbh = Bugzilla->dbh;
    return $self->{'attachments'} if exists $self->{'attachments'};

    my $attachments = $dbh->selectcol_arrayref(
            "SELECT attachment_id
               FROM test_attachments
              WHERE plan_id = ?", 
             undef, $self->{'plan_id'});
    
    my @attachments;
    foreach my $a (@{$attachments}){
        push @attachments, Bugzilla::Testopia::Attachment->new($a);
    }
    $self->{'attachments'} = \@attachments;
    return $self->{'attachments'};
    
}

=head2 builds

Returns a reference to a list of Testopia::Build objects associated 
with this plan

=cut

sub builds {
    my ($self) = @_;
    my $dbh = Bugzilla->dbh;
    return $self->{'builds'} if exists $self->{'builds'};

    my $builds = 
      $dbh->selectcol_arrayref(
              "SELECT build_id
                 FROM test_builds
                WHERE product_id = ?", 
               undef, $self->{'product_id'});
    
    my @builds;
    foreach my $id (@{$builds}){
        push @builds, Bugzilla::Testopia::Build->new($id);
    }
    $self->{'builds'} = \@builds;
    return $self->{'builds'};
    
}

=head2 bugs

Returns a reference to a list of Bugzilla::Bug objects associated
with this plan

=cut

sub bugs {
    my $self = shift;
    my $dbh = Bugzilla->dbh;
    return $self->{'bugs'} if exists $self->{'bugs'};
    my $ref = $dbh->selectcol_arrayref(
          "SELECT DISTINCT bug_id
             FROM test_case_bugs 
             JOIN test_cases ON test_case_bugs.case_id = test_cases.case_id
             JOIN test_case_plans ON test_case_plans.case_id = test_cases.case_id 
            WHERE test_case_plans.plan_id = ?", 
           undef, $self->id);
    my @bugs;
    foreach my $id (@{$ref}){
        push @bugs, Bugzilla::Bug->new($id, Bugzilla->user->id);
    }
    $self->{'bugs'} = \@bugs if @bugs;
    $self->{'bug_list'} = join(',', @$ref);
    return $self->{'bugs'};
}

=head2 product_name

Returns the name of the product this plan is associated with

=cut

sub product_name {
    my ($self) = @_;
    my $dbh = Bugzilla->dbh;
    return $self->{'product_name'} if exists $self->{'product_name'};

    $self->{'product_name'} = undef;
    $self->{'product_name'} = $dbh->selectrow_array(
            "SELECT name FROM products
              WHERE id = ?", 
             undef, $self->{'product_id'});
    return $self->{'product_name'};
}

=head2 categories

Returns a reference to a list of Testopia::Category objects 
associated with this plan

=cut

sub categories {
    my ($self) = @_;
    my $dbh = Bugzilla->dbh;
    return $self->{'categories'} if exists $self->{'categories'};

    my $categories = 
      $dbh->selectcol_arrayref(
             "SELECT category_id
                FROM test_case_categories
               WHERE product_id = ?", 
              undef, $self->{'product_id'});
        
    my @categories;
    foreach my $c (@{$categories}){
        push @categories, Bugzilla::Testopia::Category->new($c);
    }
    $self->{'categories'} = \@categories;
    return $self->{'categories'};
    
}

=head2 test_cases

Returns a reference to a list of Testopia::TestCase objects linked
to this plan

=cut

sub test_cases {
    my ($self) = @_;
    my $dbh = Bugzilla->dbh;
    return $self->{'test_cases'} if exists $self->{'test_cases'};
    my $caseids = $dbh->selectcol_arrayref(
            "SELECT case_id FROM test_case_plans
              WHERE plan_id = ?", 
             undef, $self->{'plan_id'});
    my @cases;
    foreach my $id (@{$caseids}){
        push @cases, Bugzilla::Testopia::TestCase->new($id);
    }

    $self->{'test_cases'} = \@cases;
    return $self->{'test_cases'};
}

=head2 test_case_count

Returns a count of the test cases linked to this plan

=cut

sub test_case_count {
    my ($self) = @_;
    my $dbh = Bugzilla->dbh;
    return $self->{'test_case_count'} if exists $self->{'test_case_count'};
    $self->{'test_case_count'} = $dbh->selectrow_array(
                                      "SELECT COUNT(case_id) FROM test_case_plans
                                       WHERE plan_id = ?", 
                                       undef, $self->{'plan_id'}) || 0;
    return $self->{'test_case_count'};
}

=head2 test_runs

Returns a reference to a list of test runs in this plan

=cut

sub test_runs {
    my ($self) = @_;
    my $dbh = Bugzilla->dbh;
    return $self->{'test_runs'} if exists $self->{'test_runs'};

    my $runids = $dbh->selectcol_arrayref("SELECT run_id FROM test_runs
                                          WHERE plan_id = ?", 
                                          undef, $self->{'plan_id'});
    my @runs;
    foreach my $id (@{$runids}){
        push @runs, Bugzilla::Testopia::TestRun->new($id);
    }
    
    $self->{'test_runs'} = \@runs;
    return $self->{'test_runs'};
}

=head2 test_run_count

Returns a count of the test cases linked to this plan

=cut

sub test_run_count {
    my ($self) = @_;
    my $dbh = Bugzilla->dbh;
    return $self->{'test_run_count'} if exists $self->{'test_run_count'};
    $self->{'test_run_count'} = $dbh->selectrow_array(
                                      "SELECT COUNT(run_id) FROM test_runs
                                       WHERE plan_id = ?", 
                                       undef, $self->{'plan_id'}) || 0;
    return $self->{'test_run_count'};
}

=head2 tags

Returns a reference to a list of Testopia::TestTag objects 
associated with this plan

=cut

sub tags {
    my ($self) = @_;
    my $dbh = Bugzilla->dbh;
    return $self->{'tags'} if exists $self->{'tags'};
    my $tagids = $dbh->selectcol_arrayref("SELECT tag_id 
                                          FROM test_plan_tags
                                          WHERE plan_id = ?", 
                                          undef, $self->{'plan_id'});
    my @plan_tags;
    foreach my $t (@{$tagids}){
        push @plan_tags, Bugzilla::Testopia::TestTag->new($t);
    }
    $self->{'tags'} = \@plan_tags;
    return $self->{'tags'};
}

sub plan_testers {
    my ($self) = @_;
    my $dbh = Bugzilla->dbh;
    return $self->{'plan_testers'} if exists $self->{'plan_testers'};
    my $testers = $dbh->selectcol_arrayref("SELECT p.login_name 
                                           FROM profiles p, testers t
                                           WHERE p.userid = t.userid
                                           AND t.plan_id = ?", 
                                           undef, $self->{'plan_id'});

    $self->{'plan_testers'} = $testers;
    return $self->{'plan_testers'};
}

=head2 text

Returns the text of the plan document from the latest version 
in the test_plan_texts table

=cut

sub text {
    my ($self) = @_;
    my $dbh = Bugzilla->dbh;
    return $self->{'text'} if exists $self->{'text'};
    my ($text) = $dbh->selectrow_array("SELECT plan_text
                                       FROM test_plan_texts
                                       WHERE plan_id = ? AND plan_text_version = ?", 
                                       undef, $self->{'plan_id'}, $self->version);
    $self->{'text'} = $text;
    return $self->{'text'};
}


=head2 version

Returns the plan text version. This number is incremented any time
changes are made to the plan document.

=cut

sub version { 
    my ($self) = @_;
    my $dbh = Bugzilla->dbh;
    return $self->{'version'} if exists $self->{'version'};
    my ($ver) = $dbh->selectrow_array("SELECT MAX(plan_text_version)
                                       FROM test_plan_texts
                                       WHERE plan_id = ?", 
                                       undef, $self->{'plan_id'});

    $self->{'version'} = $ver;
    return $self->{'version'};


}

=head2 type

Returns the type of this plan

=cut

sub plan_type {
    my ($self) = @_;
    my $dbh = Bugzilla->dbh;
    return $self->{'plan_type'} if exists $self->{'plan_type'};
    my ($type) = $dbh->selectrow_array("SELECT name
                                       FROM test_plan_types
                                       WHERE type_id = ?", 
                                       undef, $self->{'type_id'});

    $self->{'plan_type'} = $type;
    return $self->{'plan_type'};
}

=head1 TODO

Use Bugzilla::Product and Version in 2.22

=head1 SEE ALSO

Testopia::(TestRun, TestCase, Category, Build, Util)

=head1 AUTHOR

Greg Hendricks <ghendricks@novell.com>

=cut

1;
