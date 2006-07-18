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
# Contributor(s): Tiago R. Mello <timello@async.com.br>

use strict;

package Bugzilla::Product;

use Bugzilla::Version;
use Bugzilla::Milestone;

use Bugzilla::Util;
use Bugzilla::Group;
use Bugzilla::Error;

use base qw(Bugzilla::Object);

use constant DEFAULT_CLASSIFICATION_ID => 1;

###############################
####    Initialization     ####
###############################

use constant DB_TABLE => 'products';

use constant DB_COLUMNS => qw(
   products.id
   products.name
   products.classification_id
   products.description
   products.milestoneurl
   products.disallownew
   products.votesperuser
   products.maxvotesperbug
   products.votestoconfirm
   products.defaultmilestone
);


###############################
####       Methods         ####
###############################

sub components {
    my $self = shift;
    my $dbh = Bugzilla->dbh;

    if (!defined $self->{components}) {
        my $ids = $dbh->selectcol_arrayref(q{
            SELECT id FROM components
            WHERE product_id = ?
            ORDER BY name}, undef, $self->id);

        my @components;
        require Bugzilla::Component;
        foreach my $id (@$ids) {
            push @components, new Bugzilla::Component($id);
        }
        $self->{components} = \@components;
    }
    return $self->{components};
}

sub group_controls {
    my $self = shift;
    my $dbh = Bugzilla->dbh;

    if (!defined $self->{group_controls}) {
        my $query = qq{SELECT
                       groups.id,
                       group_control_map.entry,
                       group_control_map.membercontrol,
                       group_control_map.othercontrol,
                       group_control_map.canedit
                  FROM groups
                  LEFT JOIN group_control_map
                        ON groups.id = group_control_map.group_id
                  WHERE group_control_map.product_id = ?
                  AND   groups.isbuggroup != 0
                  ORDER BY groups.name};
        $self->{group_controls} = 
            $dbh->selectall_hashref($query, 'id', undef, $self->id);
        foreach my $group (keys(%{$self->{group_controls}})) {
            $self->{group_controls}->{$group}->{'group'} = 
                new Bugzilla::Group($group);
        }
    }
    return $self->{group_controls};
}

sub versions {
    my $self = shift;
    my $dbh = Bugzilla->dbh;

    if (!defined $self->{versions}) {
        my $values = $dbh->selectcol_arrayref(q{
            SELECT value FROM versions
            WHERE product_id = ?
            ORDER BY value}, undef, $self->id);

        my @versions;
        foreach my $value (@$values) {
            push @versions, new Bugzilla::Version($self->id, $value);
        }
        $self->{versions} = \@versions;
    }
    return $self->{versions};
}

sub milestones {
    my $self = shift;
    my $dbh = Bugzilla->dbh;

    if (!defined $self->{milestones}) {
        my $values = $dbh->selectcol_arrayref(q{
            SELECT value FROM milestones
            WHERE product_id = ?
            ORDER BY sortkey}, undef, $self->id);
 
        my @milestones;
        foreach my $value (@$values) {
            push @milestones, new Bugzilla::Milestone($self->id, $value);
        }
        $self->{milestones} = \@milestones;
    }
    return $self->{milestones};
}

sub bug_count {
    my $self = shift;
    my $dbh = Bugzilla->dbh;

    if (!defined $self->{'bug_count'}) {
        $self->{'bug_count'} = $dbh->selectrow_array(qq{
            SELECT COUNT(bug_id) FROM bugs
            WHERE product_id = ?}, undef, $self->id);

    }
    return $self->{'bug_count'};
}

sub bug_ids {
    my $self = shift;
    my $dbh = Bugzilla->dbh;

    if (!defined $self->{'bug_ids'}) {
        $self->{'bug_ids'} = 
            $dbh->selectcol_arrayref(q{SELECT bug_id FROM bugs
                                       WHERE product_id = ?},
                                     undef, $self->id);
    }
    return $self->{'bug_ids'};
}


###############################
####      Accessors      ######
###############################

sub description       { return $_[0]->{'description'};       }
sub milestone_url     { return $_[0]->{'milestoneurl'};      }
sub disallow_new      { return $_[0]->{'disallownew'};       }
sub votes_per_user    { return $_[0]->{'votesperuser'};      }
sub max_votes_per_bug { return $_[0]->{'maxvotesperbug'};    }
sub votes_to_confirm  { return $_[0]->{'votestoconfirm'};    }
sub default_milestone { return $_[0]->{'defaultmilestone'};  }
sub classification_id { return $_[0]->{'classification_id'}; }

###############################
####      Subroutines    ######
###############################

sub check_product {
    my ($product_name) = @_;

    unless ($product_name) {
        ThrowUserError('product_not_specified');
    }
    my $product = new Bugzilla::Product({name => $product_name});
    unless ($product) {
        ThrowUserError('product_doesnt_exist',
                       {'product' => $product_name});
    }
    return $product;
}

1;

__END__

=head1 NAME

Bugzilla::Product - Bugzilla product class.

=head1 SYNOPSIS

    use Bugzilla::Product;

    my $product = new Bugzilla::Product(1);
    my $product = new Bugzilla::Product({ name => 'AcmeProduct' });

    my @components      = $product->components();
    my $groups_controls = $product->group_controls();
    my @milestones      = $product->milestones();
    my @versions        = $product->versions();
    my $bugcount        = $product->bug_count();
    my $bug_ids         = $product->bug_ids();

    my $id               = $product->id;
    my $name             = $product->name;
    my $description      = $product->description;
    my $milestoneurl     = $product->milestone_url;
    my disallownew       = $product->disallow_new;
    my votesperuser      = $product->votes_per_user;
    my maxvotesperbug    = $product->max_votes_per_bug;
    my votestoconfirm    = $product->votes_to_confirm;
    my $defaultmilestone = $product->default_milestone;
    my $classificationid = $product->classification_id;

=head1 DESCRIPTION

Product.pm represents a product object. It is an implementation
of L<Bugzilla::Object>, and thus provides all methods that
L<Bugzilla::Object> provides.

The methods that are specific to C<Bugzilla::Product> are listed 
below.

=head1 METHODS

=over

=item C<components()>

 Description: Returns an array of component objects belonging to
              the product.

 Params:      none.

 Returns:     An array of Bugzilla::Component object.

=item C<group_controls()>

 Description: Returns a hash (group id as key) with all product
              group controls.

 Params:      none.

 Returns:     A hash with group id as key and hash containing 
              a Bugzilla::Group object and the properties of group
              relative to the product.

=item C<versions()>

 Description: Returns all valid versions for that product.

 Params:      none.

 Returns:     An array of Bugzilla::Version objects.

=item C<milestones()>

 Description: Returns all valid milestones for that product.

 Params:      none.

 Returns:     An array of Bugzilla::Milestone objects.

=item C<bug_count()>

 Description: Returns the total of bugs that belong to the product.

 Params:      none.

 Returns:     Integer with the number of bugs.

=item C<bug_ids()>

 Description: Returns the IDs of bugs that belong to the product.

 Params:      none.

 Returns:     An array of integer.

=back

=head1 SUBROUTINES

=over

=item C<check_product($product_name)>

 Description: Checks if the product name was passed in and if is a valid
              product.

 Params:      $product_name - String with a product name.

 Returns:     Bugzilla::Product object.

=back

=head1 SEE ALSO

L<Bugzilla::Object>

=cut
