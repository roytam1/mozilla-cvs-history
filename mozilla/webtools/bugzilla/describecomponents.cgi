#!/usr/bonsaitools/bin/perl -wT
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
# The Original Code is the Bugzilla Bug Tracking System.
#
# The Initial Developer of the Original Code is Netscape Communications
# Corporation. Portions created by Netscape are
# Copyright (C) 1998 Netscape Communications Corporation. All
# Rights Reserved.
#
# Contributor(s): Terry Weissman <terry@mozilla.org>
#                 Bradley Baetz <bbaetz@student.usyd.edu.au>

use vars qw(
  %FORM
  %legal_product
  $userid
);

use strict;

use lib qw(.);

require "CGI.pl";

ConnectToDatabase();
quietly_check_login();

GetVersionTable();

if (!defined $::FORM{'product'}) {
    # Reference to a subset of %::proddesc, which the user is allowed to see
    my %products;

    if (Param("usebuggroups")) {
        # OK, now only add products the user can see
        confirm_login() unless $::userid;
        foreach my $p (@::legal_product) {
            if (!GroupExists($p) || UserInGroup($p)) {
                $products{$p} = $::proddesc{$p};
            }
        }
    }
    else {
          %products = %::proddesc;
    }

    my $prodsize = scalar(keys %products);
    if ($prodsize == 0) {
        DisplayError("Either no products have been defined ".
                     "or you have not been given access to any.\n");
        exit;
    }
    elsif ($prodsize > 1) {
        $::vars->{'proddesc'} = \%products;
        $::vars->{'target'} = "describecomponents.cgi";
        $::vars->{'title'} = "Bugzilla component description";
        $::vars->{'h2'} = 
          "Please specify the product whose components you want described.";

        print "Content-type: text/html\n\n";
        $::template->process("global/choose-product.html.tmpl", $::vars)
          || ThrowTemplateError($::template->error());
        exit;
    }

    $::FORM{'product'} = (keys %products)[0];
}

my $product = $::FORM{'product'};

# Make sure the user specified a valid product name.  Note that
# if the user specifies a valid product name but is not authorized
# to access that product, they will receive a different error message
# which could enable people guessing product names to determine
# whether or not certain products exist in Bugzilla, even if they
# cannot get any other information about that product.
my $product_id = get_product_id($product);

ThrowUserError("The product name is invalid.") unless $product_id;

# Make sure the user is authorized to access this product.
if (Param("usebuggroups") && GroupExists($product)) {
    confirm_login() unless $::userid;
    UserInGroup($product)
      || DisplayError("You are not authorized to access that product.")
        && exit;
}

######################################################################
# End Data/Security Validation
######################################################################

my @components;
SendSQL("SELECT name, initialowner, initialqacontact, description FROM " .
        "components WHERE product_id = $product_id ORDER BY " .
        "name");
while (MoreSQLData()) {
    my ($name, $initialowner, $initialqacontact, $description) =
      FetchSQLData();

    my %component;

    $component{'name'} = $name;
    $component{'initialowner'} = $initialowner ?
      DBID_to_name($initialowner) : '';
    $component{'initialqacontact'} = $initialqacontact ?
      DBID_to_name($initialqacontact) : '';
    $component{'description'} = $description;

    push @components, \%component;
}

$::vars->{'product'} = $product;
$::vars->{'components'} = \@components;

print "Content-type: text/html\n\n";
$::template->process("reports/components.html.tmpl", $::vars)
  || ThrowTemplateError($::template->error());

