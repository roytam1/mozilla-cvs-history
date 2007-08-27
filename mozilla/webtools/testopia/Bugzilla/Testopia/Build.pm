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
# Portions created by Maciej Maczynski are Copyright (C) 2006
# Novell. All Rights Reserved.
#
# Contributor(s): Greg Hendricks <ghendricks@novell.com>

=head1 NAME

Bugzilla::Testopia::Build - An object representing a Testopia build number

=head1 DESCRIPTION

Builds are used to classify test runs. A build represents the results of 
a period of work.

=head1 SYNOPSIS

 $build = Bugzilla::Testopia::Build->new($build_id);
 $build = Bugzilla::Testopia::Build->new(\%build_hash);

=cut

package Bugzilla::Testopia::Build;

use strict;

use Bugzilla::Util;
use Bugzilla::Error;
use Bugzilla::Testopia::TestPlan;
use Bugzilla::Testopia::TestCase;

use base qw(Exporter Bugzilla::Object);
@Bugzilla::Bug::EXPORT = qw(check_build);

###############################
####    Initialization     ####
###############################

=head1 FIELDS

    build_id
    product_id
    name
    description
    milestone
    isactive

=cut
use constant DB_TABLE   => "test_builds";
use constant NAME_FIELD => "name";
use constant ID_FIELD   => "build_id";
use constant DB_COLUMNS => qw(
    build_id
    product_id
    name
    description
    milestone
    isactive
);

use constant REQUIRED_CREATE_FIELDS => qw(product_id name milestone isactive);
use constant UPDATE_COLUMNS         => qw(name description milestone isactive);

use constant VALIDATORS => {
    product_id  => \&_check_product,
    isactive    => \&_check_isactive,
};

###############################
####       Validators      ####
###############################
sub _check_product {
    my ($invocant, $product_id) = @_;
    $product_id = trim($product_id);
    
    ThrowUserError("testopia-create-denied", {'object' => 'build'}) unless Bugzilla->user->in_group('Testers');
    
    my $product = Bugzilla::Testopia::Product->new($product_id);
    
    if (ref $invocant){
        $invocant->{'product'} = $product; 
        return $product->id;
    } 
    return $product;
}

sub _check_name {
    my ($invocant, $name, $product_id) = @_;
    $name = clean_text($name) if $name;
    trick_taint($name);
    if (!defined $name || $name eq '') {
        ThrowUserError('testopia-missing-required-field', {'field' => 'name'});
    }

    # Check that we don't already have a build with that name in this product.    
    my $orig_id = check_build($name, $product_id);
    my $notunique;

    if (ref $invocant){
        # If updating, we have matched ourself at least
        $notunique = 1 if (($orig_id && $orig_id != $invocant->id))
    }
    else {
        # In new build any match is one too many
        $notunique = 1 if $orig_id;
    }

    ThrowUserError('testopia-name-not-unique', 
                  {'object' => 'Build', 
                   'name' => $name}) if $notunique;
               
    return $name;
}

sub _check_milestone {
    my ($invocant, $milestone, $product) = @_;
    if (ref $invocant){
        $product = $invocant->product;
    }
    $milestone = trim($milestone);
    $milestone = Bugzilla::Milestone::check_milestone($product, $milestone);
    return $milestone->name;
}

sub _check_isactive {
    my ($invocant, $isactive) = @_;
    ThrowCodeError('bad_arg', {argument => 'isactive', function => 'set_isactive'}) unless ($isactive =~ /(1|0)/);
    return $isactive;
}

##############################
####       Mutators        ####
###############################
sub set_description { $_[0]->set('description', $_[1]); }
sub set_isactive    { $_[0]->set('isactive', $_[1]); }
sub set_milestone { 
    my ($self, $value) = @_;
    $value = $self->_check_milestone($value);
    $self->set('milestone', $value); 
}
sub set_name { 
    my ($self, $value) = @_;
    $value = $self->_check_name($value, $self->product_id);
    $self->set('name', $value); 
}

sub new {
    my $invocant = shift;
    my $class = ref($invocant) || $invocant;
    my $param = shift;
    
    # We want to be able to supply an empty object to the templates for numerous
    # lists etc. This is much cleaner than exporting a bunch of subroutines and
    # adding them to $vars one by one. Probably just Laziness shining through.
    if (ref $param eq 'HASH'){
        if (keys %$param){
            bless($param, $class);
            return $param;
        }
        bless($param, $class);
        return $param;
    }
    
    unshift @_, $param;
    my $self = $class->SUPER::new(@_);
    
    return $self; 
}

sub run_create_validators {
    my $class  = shift;
    my $params = $class->SUPER::run_create_validators(@_);
    my $product = $params->{product_id};
    
    $params->{milestone} = $class->_check_milestone($params->{milestone}, $product);
    $params->{name}      = $class->_check_name($params->{name}, $product);
    
    return $params;
}

sub create {
    my ($class, $params) = @_;

    $class->SUPER::check_required_create_fields($params);
    my $field_values = $class->run_create_validators($params);
    
    $field_values->{isactive}  = 1;
    $field_values->{product_id} = $field_values->{product_id}->id;
    my $self = $class->SUPER::insert_create_data($field_values);
    
    return $self;
}
###############################
####      Functions        ####
###############################
sub check_build {
    my ($name, $product_id) = @_;
    my $dbh = Bugzilla->dbh;
    my $is = $dbh->selectrow_array(
        "SELECT build_id FROM test_builds 
         WHERE name = ? AND product_id = ?",
         undef, $name, $product_id);
 
    return $is;
}

###############################
####       Methods         ####
###############################

=head1 METHODS

=head2 new

Instantiates a new Build object

=cut
=head2 store

Serializes this build to the database

=cut

sub store {
    my $self = shift;
    my $dbh = Bugzilla->dbh;
    # Exclude the auto-incremented field from the column list.
    my $columns = join(", ", grep {$_ ne 'build_id'} DB_COLUMNS);

    $dbh->do("INSERT INTO test_builds ($columns) VALUES (?,?,?,?,?)",
              undef, ($self->{'product_id'}, $self->{'name'},
              $self->{'description'}, $self->{'milestone'}, $self->{'isactive'}));
    my $key = $dbh->bz_last_key( 'test_builds', 'build_id' );
    return $key;
}

=head2 check_name

Returns true if a build of the specified name exists in the database
for a product.

=cut


=head2 check_build_by_name

Returns id of a build of the specified name

=cut

sub check_build_by_name {
    my $self = shift;
    my ($name) = @_;
    my $dbh = Bugzilla->dbh;
    my $id = $dbh->selectrow_array(
        "SELECT build_id FROM test_builds 
         WHERE name = ?", undef, $name);
 
    return $id;
}

=head2 toggle_hidden

Toggles the archive bit on the build.

=cut

sub toggle_hidden {
    my $self = shift;
    my $dbh = Bugzilla->dbh;
    $dbh->do("UPDATE test_builds SET isactive = ? 
               WHERE build_id = ?", undef, $self->isactive ? 0 : 1, $self->id);
    
}

###############################
####      Accessors        ####
###############################

=head2 id

Returns the ID of this object

=head2 product_id

Returns the product_id of this object

=head2 name

Returns the name of this object

=head2 description

Returns the description of this object

=head2 milestone

Returns the Bugzilla target milestone associated with this build

=cut

sub id              { return $_[0]->{'build_id'};   }
sub product_id      { return $_[0]->{'product_id'}; }
sub name            { return $_[0]->{'name'};       }
sub description     { return $_[0]->{'description'};}
sub milestone       { return $_[0]->{'milestone'};}
sub isactive        { return $_[0]->{'isactive'};}

sub product {
    my ($self) = @_;
    
    return $self->{'product'} if exists $self->{'product'};

    $self->{'product'} = Bugzilla::Testopia::Product->new($self->product_id);
    return $self->{'product'};
}

=head2 run_count

Returns the number of test runs using this build

=cut

sub run_count {
    my ($self) = @_;
    my $dbh = Bugzilla->dbh;
    return $self->{'run_count'} if exists $self->{'run_count'};

    $self->{'run_count'} = $dbh->selectrow_array(
          "SELECT COUNT(run_id) FROM test_runs 
           WHERE build_id = ?", undef, $self->{'build_id'});
          
    return $self->{'run_count'};
}

=head2 case_run_count

Returns the number of test case runs against this build

=cut

sub case_run_count {
    my ($self,$status_id) = @_;
    my $dbh = Bugzilla->dbh;
    
    my $query = "SELECT COUNT(case_run_id) FROM test_case_runs 
           WHERE build_id = ?";
    $query .= " AND case_run_status_id = ?" if $status_id;
    
    my $count;
    if ($status_id){
        $count = $dbh->selectrow_array($query, undef, ($self->{'build_id'},$status_id));
    }
    else {
        $count = $dbh->selectrow_array($query, undef, $self->{'build_id'});
    }
          
    return $count;
}

=head1 SEE ALSO

TestPlan TestRun TestCaseRun

=head1 AUTHOR

Greg Hendricks <ghendricks@novell.com>

=cut

1;
