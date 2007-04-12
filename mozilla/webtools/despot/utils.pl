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
# The Original Code is the Despot Account Administration System.
#
# The Initial Developer of the Original Code is Netscape Communications
# Corporation. Portions created by Netscape are
# Copyright (C) 1998 Netscape Communications Corporation. All
# Rights Reserved.
#
# Contributor(s): Terry Weissman <terry@mozilla.org>

sub cryptit {
    my ($plain,$s) = (@_);
    srand (time ());
    my $sc = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789./";
    if ((!defined $s) || $s eq "") {
        $s = "";
        for (my $i=0 ; $i<2 ; $i++) {
            $s  .= substr($sc, int (rand () * 100000) % (length ($sc) + 1), 1);
        }
    }

    return crypt($plain, $s);
}

sub checkpassword {
    my ($plain, $encrypted) = (@_);
    return $encrypted eq crypt($plain, $encrypted);
}


# Trim whitespace from front and back.

sub trim {
    ($_) = (@_);
    s/^\s*//g;
    s/\s*$//g;
    return $_;
}


# Turn a partition name into a valid id.

sub name_to_id_token {
    ($_) = (@_);
    tr/A-Z/a-z/;
    s/[^A-Za-z0-9.]+/\-/g;
    return $_;
}

# Quotify a string, suitable for output as an html document
sub html_quote {
    my ($var) = (@_);
    $var =~ s/\&/\&amp;/g;
    $var =~ s/</\&lt;/g;
    $var =~ s/>/\&gt;/g;
    return $var;
}

1;
