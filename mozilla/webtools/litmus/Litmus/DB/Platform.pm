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
# The Original Code is Litmus.
#
# The Initial Developer of the Original Code is Netscape Communications
# Corporation. Portions created by Netscape are
# Copyright (C) 1998 Netscape Communications Corporation. All
# Rights Reserved.
#
# Contributor(s): Zach Lipton <zach@zachlipton.com>

package Litmus::DB::Platform;

use strict;
use base 'Litmus::DBI';
use CGI;

Litmus::DB::Platform->table('platforms');

Litmus::DB::Platform->columns(All => qw/platformid product name detect_regexp iconpath/);

Litmus::DB::Platform->has_a(product => "Litmus::DB::Product");
Litmus::DB::Platform->has_many(testresults => "Litmus::DB::Testresult");
Litmus::DB::Platform->has_many(opsyses => "Litmus::DB::Opsys");

sub autodetect {
    my $self = shift; 
    my $product = shift;
    
    my $ua = $ENV{"HTTP_USER_AGENT"};
    
    my @plats = $self->search(product => $product);
    
    foreach my $cur (@plats) {
        my $regexp = $cur->detect_regexp();
        if ($ua =~ /$regexp/) {
            return $cur;
        }
    }
}

1;