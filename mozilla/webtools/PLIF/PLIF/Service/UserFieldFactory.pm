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

package PLIF::Service::UserFieldFactory;
use strict;
use vars qw(@ISA);
use PLIF::Service;
@ISA = qw(PLIF::Service);
1;

sub provides {
    my $class = shift;
    my($service) = @_;
    return ($service eq 'user.fieldFactory' or $class->SUPER::provides($service));
}

# Field Factory
#
# The factory methods below should return service instances (not
# objects or pointers to services in the controller's service
# list!). These service instances should provide the 'user.field.X'
# service where 'X' is a field type. The field type should be
# determined from the fieldID or fieldCategory.fieldName identifiers
# passed to the factory methods.

# typically used when the data comes from the database
sub createFieldByID {
    my $self = shift;
    my($app, $user, $fieldID, $fieldData) = @_;
    return undef; # XXX
}

# typically used when the field is being created
sub createFieldByName {
    my $self = shift;
    my($app, $user, $fieldCategory, $fieldName, $fieldData) = @_;
    # fieldData is likely to be undefined, as the field is unlikely to
    # exist for this user.
    return undef; # XXX
}
