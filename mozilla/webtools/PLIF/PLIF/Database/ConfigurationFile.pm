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

package PLIF::Database::ConfigurationFile;
use strict;
use vars qw(@ISA);
use PLIF::Database;
use PLIF::Exception;
@ISA = qw(PLIF::Database);
1;

sub class {
    return 'configuration';
}

sub type {
    return 'property';
}

__DATA__

sub init {
    my $self = shift;
    my($app) = @_;
    $self->SUPER::init(@_);
    require Data::Dumper; import Data::Dumper; # DEPENDENCY
    # This next line isn't recursive thinking. The configuration
    # details for all the various databases, including this one, come
    # from the dataSource.configuration data source.
    $self->{'_FILENAME'} = $app->getService('dataSource.configuration')->configurationFilename;
}

sub filename {
    my $self = shift;
    return $self->{'_FILENAME'};
}

# typically you won't call this directly, but will use ensureRead below.
sub read {
    my $self = shift;
    my $settings;
    try {
        $settings = $self->doRead($self->filename);
    } except {
        # if we weren't successful, warn and abort
        $self->warn(3, @_);
    } otherwise {
        # else, we were successful, so go ahead and process the data
        # make sure ensureRead doesn't call us again -- the configuration file calls propertySet, which calls ensureRead:
        $self->{'_DIRTY'} = undef;
        if ($settings) {
            $settings =~ /^(.*)$/so;
            eval($1); # untaint the configuration file
            $self->assert(defined($@), 1, 'Error processing configuration file \''.($self->filename).'\': '.$@);
        }
        $self->{'_DIRTY'} = 0;
    }
}

# reads the database unless that was already done
sub ensureRead {
    my $self = shift;
    if (not exists($self->{'_DIRTY'})) {
        # not yet read configuration
        $self->read();
    }
}

# don't call this unless you know very well what you are doing
# it basically results in the file being overwritten (if you 
# call it before using propertyGet, anyway)
sub assumeRead {
    my $self = shift;
    $self->{'_DIRTY'} = 0;
}

# typically you won't call this directly, but will just rely on the
# DESTROY handler below.
sub write {
    my $self = shift;
    my $settings = "# This is the configuration file.\n# You may edit this file, so long as it remains valid Perl.\n";
    local $Data::Dumper::Terse = 1;
    foreach my $variable (sort(keys(%$self))) {
        if ($variable !~ /^_/o) { # we skip the internal variables (prefixed with '_')
            my $contents = Data::Dumper->Dump([$self->{$variable}]);
            chop($contents); # remove the newline (newline is guarenteed so no need to chomp)
            $settings .= "\$self->propertySet('$variable', $contents);\n";
        }
    }
    $self->doWrite($self->filename, $settings);
    $self->{'_DIRTY'} = 0;
}

sub propertySet {
    my $self = shift;
    my($name, $value) = @_;
    $self->ensureRead();
    $self->{'_DIRTY'} = 1;
    return $self->{$name} = $value;
}

sub propertyGet {
    my $self = shift;
    my($name) = @_;
    $self->ensureRead();
    return $self->{$name};
}

sub DESTROY {
    my $self = shift;
    if ($self->{'_DIRTY'}) {
        $self->write();
    }
    $self->SUPER::DESTROY(@_);
}


# internal low-level implementation routines

sub doRead {
    my $self = shift;
    my($filename) = @_;
    if (-e $filename) {
        $self->assert($self->doPermissionsCheck($filename), 1, "Configuration file '$filename' has the wrong permissions: it has to be only accessible by this user since it can contain passwords. Running without configuration file");
        local *FILE; # ugh
        $self->assert(open(FILE, "<$filename"), 1, "Could not open configuration file '$filename' for reading: $!");
        local $/ = undef; # slurp entire file (no record delimiter)
        my $settings = <FILE>;
        $self->assert(close(FILE), 3, "Could not close configuration file '$filename': $!");
        return $settings;
    } else {
        # file doesn't exist, so no configuration to read in
        return '';
    }
}

sub doWrite {
    my $self = shift;
    my($filename, $contents) = @_;
    local *FILE; # ugh
    my $umask = umask(0077); # XXX this might be UNIX-specific # XXX THIS IS PLATFORM SPECIFIC CODE XXX
    $self->assert(open(FILE, ">$filename"), 1, "Could not open configuration file '$filename' for writing: $!");
    $self->assert(FILE->print($contents), 1, "Could not dump settings to configuration file '$filename': $!");
    $self->assert(close(FILE), 1, "Could not close configuration file '$filename': $!");
    umask($umask); # XXX this might be UNIX-specific # XXX THIS IS PLATFORM SPECIFIC CODE XXX
}

sub doPermissionsCheck {
    my $self = shift;
    my($filename) = @_;
    # XXX this might be UNIX-specific # XXX THIS IS PLATFORM SPECIFIC CODE XXX
    my($dev, $ino, $mode, $nlink, $uid, $gid, $rdev, $size, $atime, $mtime, $ctime, $blksize, $blocks) = stat($filename);
    return (($mode & 07077) == 0 and # checks that the permissions are at most -xrw------
            $uid == $>); # checks that the file's owner is the same as the user running the script
}
