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

package PLIF::Service::Components::Login;
use strict;
use vars qw(@ISA);
use PLIF::Service;
@ISA = qw(PLIF::Service);
1;

sub provides {
    my $class = shift;
    my($service) = @_;
    return ($service eq 'input.verify' or 
            $service eq 'input.verify.user.generic' or 
            $service eq 'user.login' or
            $service eq 'component.userLogin' or
            $service eq 'dispatcher.commands' or 
            $service eq 'dispatcher.output.generic' or 
            $service eq 'dispatcher.output' or 
            $class->SUPER::provides($service));
}

# input.verify
sub verifyInput {
    my $self = shift;
    my($app) = @_;
    # let's see if there are any protocol-specific user authenticators
    my @result = $app->getSelectingServiceList('input.verify.user.'.$app->input->defaultOutputProtocol)->authenticateUser($app);
    if (not @result) { 
        # ok, let's try the generic authenticators...
        @result = $app->getSelectingServiceList('input.verify.user.generic')->authenticateUser($app);
    }
    # now let's see what that gave us
    if (@result) {
        # horrah, somebody knew what to do!
        if ((defined($result[0])) and ($result[0]->checkLogin())) {
            $app->addObject($result[0]); # they will have returned a user object
        } else {
            # hmm, so apparently user is not authentic
            $self->errorState(\@result);
            return $self; # supports user.login (reportInputVerificationError)
        }
    }
    return; # nope, nothing to see here... (no error, anyway)
}

# input.verify.user.generic
sub authenticateUser {
    my $self = shift;
    my($app) = @_;
    if (defined($app->input->username)) {
        return $app->getService('user.factory')->getUserByCredentials($app, $app->input->username, $app->input->password);
    } else {
        return; # return nothing (not undef)
    }
}

# input.verify
sub reportInputVerificationError {
    my $self = shift;
    my($app) = @_;
    my $message = '';
    if (defined($self->errorState) and defined($self->errorState->[0])) {
        $message = $self->errorState->[0]->adminMessage;
    }
    $self->errorState(undef);
    $app->output->loginFailed(1, $message); # 1 means 'unknown username/password'
}

# dispatcher.commands
sub cmdLoginRequestAccount {
    my $self = shift;
    my($app) = @_;
    $app->output->loginRequestAccount();
}

# dispatcher.commands
sub cmdLoginLogout {
    my $self = shift;
    my($app) = @_;
    # mark the user as logged out and then return to the main index page
    my $user = $app->getObject('user');
    if (defined($user)) {
        $user->logout();
        $app->removeObject($user);
    }
    $app->noCommand();
}

# cmdLoginSendPassword could also be called 'cmdLoginNewUser'
# dispatcher.commands
sub cmdLoginSendPassword {
    my $self = shift;
    my($app) = @_;
    my $protocol = $app->input->getArgument('protocol');
    my $address = $app->input->getArgument('address');
    if (defined($protocol) and defined($address)) {
        my $user = $app->getService('user.factory')->getUserByContactDetails($app, $protocol, $address);
        my $password;
        if (defined($user)) {
            $password = $self->changePassword($app, $user);
        } else {
            ($user, $password) = $self->createUser($app, $protocol, $address);
            if (not defined($user)) {
                $app->output->loginFailed(2, ''); # 2 means 'invalid protocol/username'
                return;
            }
        }
        $self->sendPassword($app, $user, $protocol, $password);
    } else {
        $app->output->loginFailed(0, ''); # 0 means 'no username/password'
    }
}

# user.login
# if this returns undef, don't do anything!
# XXX need a quieter version of this to enable/disable UI elements
sub hasRight {
    my $self = shift;
    my($app, $right) = @_;
    my $user = $app->getObject('user');
    if (defined($user)) {
        if ($user->hasRight($right)) {
            return $user;
        } else {
            $app->output->loginInsufficient($right);
        }
    } else {
        $self->requireLogin($app);
    }
    return undef;
}

# user.login
# this assumes user is not logged in
sub requireLogin {
    my $self = shift;
    my($app) = @_;
    my $address = $app->input->address;
    if (defined($address) and not defined($app->getService('user.factory')->getUserByContactDetails($app, $app->input->protocol, $address))) {
        my($user, $password) = $self->createUser($app, $app->input->protocol, $address);
        $self->sendPassword($app, $user, $app->input->protocol, $password);
    } else {
        $app->output->loginFailed(0, '');
    }
}

# dispatcher.output.generic
sub outputLoginInsufficient {
    my $self = shift;
    my($app, $output, $right) = @_;
    $output->output('login.accessDenied', {
        'right' => $right,
    });   
}

# dispatcher.output.generic
sub outputLoginFailed {
    my $self = shift;
    my($app, $output, $tried, $message) = @_;
    $output->output('login.failed', {
        'tried' => $tried, # 0 = no username; 1 = unknown username; 2 = invalid username
        'contacts' => [$app->getService('dataSource.user')->getFieldNamesByCategory($app, 'contact')],
        'message' => $message,
    });   
}

# dispatcher.output.generic
sub outputLoginRequestAccount {
    my $self = shift;
    my($app, $output, $tried) = @_;
    $output->output('login.requestAccount', {
        'contacts' => [$app->getService('dataSource.user')->getFieldNamesByCategory($app, 'contact')],
    });   
}

# dispatcher.output.generic
sub outputLoginDetailsSent {
    my $self = shift;
    my($app, $output, $address, $protocol) = @_;
    $output->output('login.detailsSent', {
        'address' => $address,
        'protocol' => $protocol,
    });   
}

# dispatcher.output.generic
sub outputLoginDetails {
    my $self = shift;
    my($app, $output, $username, $password) = @_;
    $output->output('login.details', {
        'username' => $username,
        'password' => $password,
    });   
}

# dispatcher.output
sub strings {
    return (
            'login.accessDenied' => 'Displayed when the user does not have the requisite right (namely, data.right).',
            'login.failed' => 'Displayed when the user has not logged in (data.tried is false) or when the credentials were wrong (data.tried is true). A message may be given in data.message.',
            'login.requestAccount' => 'Displayed when the user requests the form to enter a new account (should display the same form as login.failed, basically).',
            'login.detailsSent' => 'The password was sent to data.address using data.protocol.',
            'login.details' => 'The message containing the data.username and data.password of a new account or when the user has forgotten his password (only required for contact protocols, e.g. e-mail).',
            );
}


# internal routines

sub changePassword {
    my $self = shift;
    my($app, $user) = @_;
    my($crypt, $password) = $app->getService('service.passwords')->newPassword();
    $user->password($crypt);
    return $password;
}

sub createUser {
    my $self = shift;
    my($app, $protocol, $address) = @_;
    my $validator = $app->getService('protocol.'.$protocol);
    if ((defined($validator)) and (not $validator->checkAddress($address))) {
        return (undef, undef);
    }
    my($crypt, $password) = $app->getService('service.passwords')->newPassword();
    my $user = $app->getService('user.factory')->getNewUser($app, $crypt);
    $user->getField('contact', $protocol)->data($address);
    return ($user, $password);
}

sub sendPassword {
    my $self = shift;
    my($app, $user, $protocol, $password) = @_;
    my $field = $user->hasField('contact', $protocol);
    $self->assert(defined($field), 1, 'Tried to send a password using a protocol that the user has no mention of!'); # XXX grammar... :-)
    $app->output($protocol, $user)->loginDetails($field->username, $password);
    $app->output->loginDetailsSent($field->address, $protocol);
}
