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

package PLIF::Output::Generic::Email;
use strict;
use vars qw(@ISA);
use PLIF::Service;
use Net::SMTP; # DEPENDENCY
@ISA = qw(PLIF::Service);
1;

sub provides {
    my $class = shift;
    my($service) = @_;
    return ($service eq 'output.generic.email' or
            $service eq 'protocol.smtp' or
            $class->SUPER::provides($service));
}

sub init {
    my $self = shift;
    my($app) = @_;
    $self->SUPER::init(@_);
    $self->handle(Net::SMTP->new('localhost')); # XXX
}

# output.generic.email
sub output {
    my $self = shift;
    my($app, $session, $string) = @_;
    $self->assert(defined($self->handle), 1, 'No SMTP handle, can\'t send mail');
    $self->handle->mail('XXX@spam.hixie.ch');
    $self->handle->to($session->getAddress('email'));
    $self->handle->data($string);
}

# protocol.smtp
sub checkAddress {
    my $self = shift;
    my($app, $username) = @_;
    $self->assert(defined($self->handle), 1, 'No SMTP handle, can\'t check address');
    my $result = $self->handle->verify($username);
    return $result;
}

sub DESTROY {
    my $self = shift;
    if (defined($self->handle)) {
        $self->handle->quit();
    }
    $self->SUPER::DESTROY(@_);
}
