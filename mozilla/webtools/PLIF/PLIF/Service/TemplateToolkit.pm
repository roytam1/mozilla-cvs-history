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

package PLIF::Service::TemplateToolkit;
use strict;
use vars qw(@ISA);
use PLIF::Service;
@ISA = qw(PLIF::Service);
1;

sub provides {
    my $class = shift;
    my($service) = @_;
    return ($service eq 'string.expander.TemplateToolkit' or
            $service eq 'string.expander.TemplateToolkitCompiled' or
            $class->SUPER::provides($service));
}

sub init {
    my $self = shift;
    my($app) = @_;
    $self->SUPER::init(@_);
    require Template; import Template; # DEPENDENCY
}

sub expand {
    my $self = shift;
    my($app, $output, $session, $protocol, $string, $data, $type) = @_;
    local $Template::Config::STASH = 'Template::Stash::Context'; # Don't use Template::Stash::XS as we can't currently get a hash back out of it
    my $template = Template->new({
        'CONTEXT' => PLIF::Service::TemplateToolkit::Context->new($app, $output, $session, $protocol),
    });
    my $document;
    if ($type eq 'TemplateToolkitCompiled') {
        # what we have here is a potential Template::Document
        # let's try to evaluate it
        $document = eval $string;
        $self->assert((not defined($@)), 1, "Error loading compiled template: $@");
    } else { # $type eq 'TemplateToolkit'
        # what we have is a raw string
        $document = \$string;
    }
    # ok, let's try to process it
    my $result = '';
    if (not $template->process($document, $data, \$result)) {
        $self->error(1, 'Error processing template: '.$template->error());
    }
    return $result;
}


package PLIF::Service::TemplateToolkit::Context;
use strict;
use vars qw(@ISA);
use Template::Context;
@ISA = qw(Template::Context);
1;

# subclass the real Context so that we go through PLIF for everything
sub new {
    my $class = shift;
    my($app, $output, $session, $protocol) = @_;
    my $self = $class->SUPER::new({});
    if (defined($self)) {
        $self->{'__PLIF__app'} = $app;
        $self->{'__PLIF__output'} = $output; # unused (it's a handle to the dataSource.strings service but we look one up instead of using it directly)
        $self->{'__PLIF__session'} = $session;
        $self->{'__PLIF__protocol'} = $protocol;
    } # else failed
    return $self;
}

# compile a template
sub template {
    my $self = shift;
    my($text) = @_;
    $self->{'__PLIF__app'}->assert(ref($text), 1, 'Internal error: tried to compile a template by name instead of going through normal channels (security risk)');
    return $self->SUPER::template(@_);
}

# insert another template
sub process {
    my $self = shift;
    my($strings, $params) = @_;

    # support multiple files for compatability with Template::Context->process()
    if (ref($strings) ne 'ARRAY') {
        $strings = [$strings];
    }

    # get data source
    my $app = $self->{'__PLIF__app'};
    my $session = $self->{'__PLIF__session'};
    my $protocol = $self->{'__PLIF__protocol'};
    my $dataSource = $self->{'__PLIF__app'}->getService('dataSource.strings');

    # expand strings and append
    my $result = '';
    foreach my $string (@$strings) {
        if (ref($string)) {
            # probably already compiled
            $result .= $self->SUPER::process($string, $params);
        } else {
            $result .= $dataSource->getExpandedString($app, $session, $protocol, $string, $self->{'STASH'});
        }
    }

    return $result;
}

# insert another template but protecting the stash
sub include {
    my $self = shift;
    my($strings, $params) = @_;

    # localise the variable stash with any parameters passed
    my $stash = $self->{'STASH'};
    $self->{'STASH'} = $self->{'STASH'}->clone($params);

    # defer to process(), above
    my $result = eval { $self->process($strings, $params); };
    my $error = $@;

    # delocalise the variable stash
    $self->{'STASH'} = $self->{'STASH'}->declone();

    # propagate any error
    if (ref($error) or ($error ne '')) {
        die $error;
    }

    return $result;
}

# insert a string regardless of what it is without expanding it
sub insert {
    my $self = shift;
    my($strings) = @_;

    # support multiple files for compatability with Template::Context->insert()
    if (ref($strings) ne 'ARRAY') {
        $strings = [$strings];
    }

    # get data source
    my $app = $self->{'__PLIF__app'};
    my $session = $self->{'__PLIF__session'};
    my $protocol = $self->{'__PLIF__protocol'};
    my $dataSource = $self->{'__PLIF__app'}->getService('dataSource.strings');

    # concatenate the files
    my $result = '';
    foreach my $string (@$strings) {
        my($type, $version, $data) = $dataSource->getString($app, $session, $protocol, $string);
        $result .= $data;
    }

    return $result;
}
