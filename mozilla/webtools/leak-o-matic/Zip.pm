#!/usr/bin/perl -w
#
# The contents of this file are subject to the Netscape Public
# License Version 1.1 (the "License"); you may not use this file
# except in compliance with the License. You may obtain a copy of
# the License at http://www.mozilla.org/NPL/
#
# Software distributed under the License is distributed on an "AS
# IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
# implied. See the License for the specific language governing
# rights and limitations under the License.
#
# The Original Code is Mozilla Communicator client code.
#
# The Initial Developer of the Original Code is Netscape Communications
# Corporation.  Portions created by Netscape are
# Copyright (C) 1998 Netscape Communications Corporation. All
# Rights Reserved.
#
# Contributor(s): 
#
# $Id$
#

#
# Simple, incomplete interface to .zip files.
#


package Zip;

require 5.000;

use strict;

use Carp;
use IO;
use Symbol;
use Time::Local;

my $zipname;

sub new($$) {
    @_ == 2 || croak 'usage: new Zip [FILENAME]';
    my $class = shift;
    my $zip = gensym;
    if (@_) {
      Zip::open($zip, $_[0]) || return undef;
    }
    bless $zip, $class;
}

sub DESTROY($) {
}

sub open($$) {
    @_ == 2 || croak 'usage: $zip->open(FILENAME)';
    my ($zip, $filename) = @_;
    $zipname = $filename;
}

sub close($) {
    @_ == 1 || croak 'usage: $zip->close()';
    $zipname = undef;
}

sub dir($) {
    @_ == 1 || croak 'usage: $zip->dir()';
    $zipname || croak 'no open zipfile';


    my @result = ();

    my @list = qx/unzip -l $zipname/;
    ENTRY: foreach (@list) {
        # Entry expected to be in the format
        #   size mm-dd-yy hh:mm name
        next ENTRY unless (/ *(\d+) +(\d+)-(\d+)-(\d+) +(\d+):(\d+) +(.+)$/);

        my $mtime = Time::Local::timelocal(0, $6, $5, $3, $2, ($4 < 1900) ? ($4 + 1900) : $4);

        push(@result, { name  => $7,
                        size  => $1,
                        mtime => $mtime });
    }

    return @result;
}

sub expand($$) {
    @_ == 2 || croak 'usage: $zip->expand(FILENAME)';
    $zipname || croak 'no open zipfile';

    my $filename = $_[1];

    my $result = new IO::Handle;
    CORE::open($result, "unzip -p $zipname $filename |");
    return $result;
}

1;
