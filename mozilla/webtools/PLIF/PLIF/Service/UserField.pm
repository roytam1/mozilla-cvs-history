# -*- Mode: perl; tab-width: 4; indent-tabs-mode: nil; -*-
#
# This file is MPL/GPL dual-licensed under the following terms:
# 
# The contents of this file are subject to the Mozilla Public License
# Version 1.1 (the "License"); you may not use this file except in
# compliance with the License. You may obtain a copy of the License at
# http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an "AS IS"
# basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
# the License for the specific language governing rights and
# limitations under the License.
#
# The Original Code is PLIF 1.0.
# The Initial Developer of the Original Code is Ian Hickson.
#
# Alternatively, the contents of this file may be used under the terms
# of the GNU General Public License Version 2 or later (the "GPL"), in
# which case the provisions of the GPL are applicable instead of those
# above. If you wish to allow use of your version of this file only
# under the terms of the GPL and not to allow others to use your
# version of this file under the MPL, indicate your decision by
# deleting the provisions above and replace them with the notice and
# other provisions required by the GPL. If you do not delete the
# provisions above, a recipient may use your version of this file
# under either the MPL or the GPL.

package PLIF::Service::UserField;
use strict;
use vars qw(@ISA);
use PLIF::Service;
@ISA = qw(PLIF::Service);
1;

# This class implements a service instance -- you should never call
# getService() to obtain a copy of this class or its descendants.

sub provides {
    my $class = shift;
    my($service) = @_;
    return ($service eq 'user.field.'.$class->type or $class->SUPER::provides($service));
}

# the 'data' field of field descriptions means different things
# depending on the category. For 'contact' category user fields, it
# represents a string that is prefixed to the data of the user field
# to obtain the username that should be used for this user field.

sub init {
    my $self = shift;
    my($app, $user, $fieldID, $fieldTypeData, $fieldCategory, $fieldName, $fieldData) = @_;
    # do not hold on to $user!
    $self->app($app);
    $self->userID($user->userID); # change this at your peril
    $self->fieldID($fieldID); # change this at your peril
    $self->typeData($fieldTypeData); # change this at your peril
    $self->category($fieldCategory); # change this at your peril
    $self->name($fieldName); # change this at your peril
    $self->data($fieldData); # this is the only thing you should be changing
    # don't forget to update the user's 'hash' function if you add more fields
    $self->{'_DELETE'} = 0;
    $self->{'_DIRTY'} = 0;
}

sub type {
    my $self = shift;
    $self->notImplemented();
}

sub validate {
    return 1;
}

# contact fields have usernames made of the field type data part
# followed by the field data itself
sub username {
    my $self = shift;
    $self->assert($self->category eq 'contact', 1, 'Tried to get the username from the non-contact field \''.($self->fieldID).'\'');
    return $self->typeData.$self->data;
}

# deletes this field from the database
sub remove {
    my $self = shift;
    $self->{'_DELETE'} = 1;
    $self->{'_DIRTY'} = 1;
}

sub propertySet {
    my $self = shift;
    my $result = $self->SUPER::propertySet(@_);
    $self->{'_DIRTY'} = 1;
    return $result;
}

sub DESTROY {
    my $self = shift;
    if ($self->{'_DIRTY'}) {
        $self->write();
    }
    $self->SUPER::DESTROY(@_);
}

sub write {
    my $self = shift;
    if ($self->{'_DELETE'}) {
        $self->app->getService('dataSource.user')->removeUserField($elf->app, $self->userID, $self->fieldID);
    } else {
        $self->app->getService('dataSource.user')->setUserField($elf->app, $self->userID, $self->fieldID, $self->data);
    }
}
