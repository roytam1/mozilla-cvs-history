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

package PLIF::Output::Generic;
use strict;
use vars qw(@ISA);
use PLIF::Output;
@ISA = qw(PLIF::Output);
1;

sub protocol {
    return 'generic';
}

sub init {
    my $self = shift;
    $self->SUPER::init(@_);
    my($app, $session, $protocol) = @_;
    $self->propertySet('actualSession', $session);
    $self->propertySet('actualProtocol', $protocol);
}

sub output {
    my $self = shift;
    my($session, $string, $data) = @_;
    if (not defined($session)) {
        $session = $self->actualSession;
    }
    my $expander = $self->app->getService("string.expander.$string");
    if (not defined($expander)) {
        $expander = $self->app->getService('string.expander');
        $self->assert($expander, 1, 'Could not find a string expander.');
    }
    $self->app->getService('output.generic.'.$self->actualProtocol)->output($self->app, $session, 
         $expander->expand($self->app, $session, $self->actualProtocol, $string, $data));
}
