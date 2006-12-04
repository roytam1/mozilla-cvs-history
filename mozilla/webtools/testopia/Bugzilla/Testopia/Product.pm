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

package Bugzilla::Testopia::Product;

use strict;

# Extends Bugzilla::Product;
use base "Bugzilla::Product";

use Bugzilla;
use Bugzilla::Testopia::Environment;

sub environments {
    my $self = shift;
    my $dbh = Bugzilla->dbh;
    
    my $ref = $dbh->selectcol_arrayref("SELECT environment_id 
                                        FROM test_environments
                                        WHERE product_id = ?",
                                        undef, $self->{'id'});
    my @objs;
    foreach my $id (@{$ref}){
        push @objs, Bugzilla::Testopia::Environment->new($id);
    }
    $self->{'environments'} = \@objs;
    return $self->{'environments'};
}

sub builds {
    my $self = shift;
    my $dbh = Bugzilla->dbh;
    
    my $ref = $dbh->selectcol_arrayref(
                   "SELECT build_id 
                      FROM test_builds 
                     WHERE product_id = ?
                  ORDER BY name",
                    undef, $self->{'id'});
    my @objs;
    foreach my $id (@{$ref}){
        push @objs, Bugzilla::Testopia::Build->new($id);
    }
    $self->{'builds'} = \@objs;
    return $self->{'builds'};
}

sub categories {
    my $self = shift;
    my $dbh = Bugzilla->dbh;
    
    my $ref = $dbh->selectcol_arrayref(
                   "SELECT category_id 
                    FROM test_case_categories 
                    WHERE product_id = ?
                 ORDER BY name",
                    undef, $self->{'id'});
    my @objs;
    foreach my $id (@{$ref}){
        push @objs, Bugzilla::Testopia::Category->new($id);
    }
    $self->{'categories'} = \@objs;
    return $self->{'categories'};
}

sub plans {
    my $self = shift;
    my $dbh = Bugzilla->dbh;
    
    my $ref = $dbh->selectcol_arrayref(
                   "SELECT plan_id 
                    FROM test_plans 
                    WHERE product_id = ?
                 ORDER BY name",
                    undef, $self->{'id'});
    my @objs;
    foreach my $id (@{$ref}){
        push @objs, Bugzilla::Testopia::TestPlan->new($id);
    }
    $self->{'plans'} = \@objs;
    return $self->{'plans'};
}

sub environment_categories {
    my $self = shift;
    my $dbh = Bugzilla->dbh;
    
    my $ref = $dbh->selectcol_arrayref(
                   "SELECT env_category_id 
                    FROM test_environment_category 
                    WHERE product_id = ?",
                    undef, $self->id);
    my @objs;
    foreach my $id (@{$ref}){
        push @objs, Bugzilla::Testopia::Environment::Category->new($id);
    }
    $self->{'environment_categories'} = \@objs;
    return $self->{'environment_categories'};
}

sub check_product_by_name {
    my $self = shift;
    my ($name) = @_;
    my $dbh = Bugzilla->dbh;
    my ($used) = $dbh->selectrow_array(qq{
    	SELECT id 
		  FROM products
		  WHERE name = ?},undef,$name);
    return $used;  
}

1;
