#!/usr/bin/perl -w
# -*- Mode: perl; indent-tabs-mode: nil -*-
#
# The contents of this file are subject to the Mozilla Public License
# Version 1.0 (the "License"); you may not use this file except in
# compliance with the License. You may obtain a copy of the License at
# http://www.mozilla.org/MPL/
# 
# Software distributed under the License is distributed on an "AS IS"
# basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
# License for the specific language governing rights and limitations
# under the License.
#
#
# Direct any questions on this source code to
#
# Holger Schurig <holgerschurig@nikocity.de>

use diagnostics;
use strict;

require "CGI.pl";
require "globals.pl";

# TestProduct:  just returns if the specified product does exists
# CheckProduct: same check, optionally  emit an error text

sub TestProduct ($) {
    my $prod = shift;

    # does the product exist?
    SendSQL("SELECT product
             FROM products
             WHERE product=" . SqlQuote($prod));
    return FetchOneColumn();
}

sub CheckProduct ($) {
    my $prod = shift;

    # do we have a product?
    unless ($prod) {
        print "Sorry, you haven't specified a product.";
        PutTrailer();
        exit;
    }

    unless (TestProduct $prod) {
        print "Sorry, product '$prod' does not exist.";
        PutTrailer();
        exit;
    }
}


#
# Displays the form to edit a products parameters
#
sub EmitFormElements ($$$$) {
    my ($product, $description, $milestoneurl, $disallownew) = @_;

    print "  <TH ALIGN=\"right\">Product:</TH>\n";
    print "  <TD><INPUT SIZE=64 MAXLENGTH=64 NAME=\"product\" VALUE=\"$product\"></TD>\n";
    print "</TR><TR>\n";

    print "  <TH ALIGN=\"right\">Description:</TH>\n";
    print "  <TD><TEXTAREA ROWS=4 COLS=64 WRAP=VIRTUAL NAME=\"description\">$description</TEXTAREA></TD>\n";

    if (Param('usetargetmilestone')) {
        print "</TR><TR>\n";
        print "  <TH ALIGN=\"right\">Milestone URL:</TH>\n";
        print "  <TD><INPUT TYPE=TEXT SIZE=64 MAXLENGTH=255 NAME=\"milestoneurl\" VALUE=\"$milestoneurl\"></TD>\n";
    }

    print "</TR><TR>\n";
    print "  <TH ALIGN=\"right\">Closed for bug entry:</TH>\n";
    my $closed = $disallownew ? "CHECKED" : "";
    print "  <TD><INPUT TYPE=CHECKBOX NAME=\"disallownew\" $closed VALUE=\"1\"></TD>\n";
}


#
# Displays a text like "a.", "a or b.", "a, b or c.", "a, b, c or d."
#
sub PutTrailer (@) {
    my (@links) = ("Back to the <A HREF=\"query.cgi\">query page</A>", @_);
	print "<CENTER>\n";
    my $count = $#links;
    my $num = 0;
    print "<P>\n";
    foreach (@links) {
        print $_;
        if ($num == $count) {
            print ".\n";
        }
        elsif ($num == $count-1) {
            print " or ";
        }
        else {
            print ", ";
        }
        $num++;
    }
	print "</CENTER>\n";
}


#
# Preliminary checks:
#

confirm_login();

print "Content-type: text/html\n\n";

unless (UserInGroup("editcomponents")) {
    PutHeader("Not allowed");
    PutError("Sorry, you aren't a member of the 'editcomponents' group.<BR>\n" . 
    		 "And so, you aren't allowed to add, modify or delete products.<BR>\n");
    PutTrailer();
    exit;
}


#
# often used variables
#
my $product = trim($::FORM{product} || '');
my $action  = trim($::FORM{action}  || '');
my $localtrailer = "<A HREF=\"editproducts.cgi\">edit</A> more products";


#
# action='' -> Show nice list of products
#
unless ($action) {
    PutHeader("Select product");

    SendSQL("SELECT product, description, disallownew 
             FROM products 
             ORDER BY product");
    print "<P>\n<TABLE BORDER=1 CELLPADDING=3 CELLSPACING=0 WIDTH=800 ALIGN=center><TR BGCOLOR=\"#BFBFBF\">\n";
    print "  <TH ALIGN=\"left\">Product</TH>\n";
    print "  <TH ALIGN=\"left\">Description</TH>\n";
    print "  <TH ALIGN=\"left\">Status</TH>\n";
    print "  <TH ALIGN=\"left\">Action</TH>\n";
    print "</TR>";
    while ( MoreSQLData() ) {
        my ($product, $description, $disallownew) = FetchSQLData();
        $description ||= "<FONT COLOR=\"red\">missing</FONT>";
        $disallownew = $disallownew ? 'closed' : 'open';
        print "<TR BGCOLOR=\"#ECECEC\">\n";
        print "  <TD VALIGN=\"top\"><A HREF=\"editproducts.cgi?action=edit&product=", url_quote($product), "\"><B>$product</B></A></TD>\n";
        print "  <TD VALIGN=\"top\">$description</TD>\n";
        print "  <TD VALIGN=\"top\">$disallownew</TD>\n";
        print "  <TD VALIGN=\"top\"><A HREF=\"editproducts.cgi?action=del&product=", url_quote($product), "\">Delete</A></TD>\n";
        print "</TR>";
    }
    print "<TR BGCOLOR=\"#ECECEC\">\n";
    print "  <TH VALIGN=\"top\" COLSPAN=4><A HREF=\"editproducts.cgi?action=add\">Add a new product</A></TH>\n";
    print "</TR></TABLE>\n";

    PutTrailer();
    PutFooter();
	exit;
}

#
# action='add' -> present form for parameters for new product
#
# (next action will be 'new')
#
if ($action eq 'add') {
    PutHeader("Add product");

    #print "This page lets you add a new product to bugzilla.\n";

    print "<FORM METHOD=POST ACTION=editproducts.cgi>\n";
    print "<TABLE BORDER=0 CELLPADDING=4 CELLSPACING=0 ALIGN=center><TR>\n";

    EmitFormElements('', '', '', 0);

    print "</TR><TR>\n";
    print "  <TH ALIGN=\"right\">Version:</TH>\n";
    print "  <TD><INPUT SIZE=64 MAXLENGTH=255 NAME=\"version\" VALUE=\"unspecified\"></TD>\n";

    print "</TABLE>\n";
    print "<CENTER><INPUT TYPE=SUBMIT VALUE=\"Add\"></CENTER>\n";
    print "<INPUT TYPE=HIDDEN NAME=\"action\" VALUE=\"new\">\n";
    print "</FORM>";

    my $other = $localtrailer;
    $other =~ s/more/other/;
    PutTrailer($other);
	PutFooter();
    exit;
}

#
# action='new' -> add product entered in the 'action=add' screen
#
if ($action eq 'new') {
    PutHeader("Adding new product");

    # Cleanups and valididy checks

    unless ($product) {
        print "You must enter a name for the new product. Please press\n";
        print "<b>Back</b> and try again.\n";
        PutTrailer($localtrailer);
        exit;
    }
    if (TestProduct($product)) {
        print "The product '$product' already exists. Please press\n";
        print "<b>Back</b> and try again.\n";
        PutTrailer($localtrailer);
        exit;
    }

    my $version = trim($::FORM{version} || '');

    if ($version eq '') {
        print "You must enter a version for product '$product'. Please press\n";
        print "<b>Back</b> and try again.\n";
        PutTrailer($localtrailer);
        exit;
    }

    my $description  = trim($::FORM{description}  || '');
    my $milestoneurl = trim($::FORM{milestoneurl} || '');
    my $disallownew = 0;
    $disallownew = 1 if $::FORM{disallownew};

    # Add the new product.
	if ($::driver eq "mysql") {
    	SendSQL("INSERT INTO products ( " .
        	    "product, description, milestoneurl, disallownew" .
          		" ) VALUES ( " .
          		SqlQuote($product) . "," .
          		SqlQuote($description) . "," .
          		SqlQuote($milestoneurl) . "," .
          		$disallownew . ")" );
	} else {
		SendSQL("INSERT INTO products ( " .
                "product, description, milestoneurl, disallownew, id" .
                " ) VALUES ( " .
                SqlQuote($product) . "," .
                SqlQuote($description) . "," .
                SqlQuote($milestoneurl) . "," .
                $disallownew . ", products_seq.nextval)" );    
	}
	SendSQL("INSERT INTO versions ( " .
          "value, program" .
          " ) VALUES ( " .
          SqlQuote($version) . "," .
          SqlQuote($product) . ")" );

    # Make versioncache flush
    unlink "data/versioncache";

    print "<CENTER>OK, done.</CENTER><p>\n";
    PutTrailer($localtrailer, "<a href=\"editcomponents.cgi?action=add&product=" . url_quote($product) . "\">add</a> components to this new product.");
	PutFooter();
    exit;
}

#
# action='del' -> ask if user really wants to delete
#
# (next action would be 'delete')
#
if ($action eq 'del') {
    PutHeader("Delete product");
    CheckProduct($product);

    # display some data about the product
    SendSQL("SELECT description, milestoneurl, disallownew
             FROM products
             WHERE product=" . SqlQuote($product));
    my ($description, $milestoneurl, $disallownew) = FetchSQLData();
    $description ||= "<FONT COLOR=\"red\">description missing</FONT>";
    $disallownew = $disallownew ? 'closed' : 'open';
    
    print "<TABLE BORDER=1 CELLPADDING=4 CELLSPACING=0 ALIGN=center BGCOLOR=\"#ECECEC\">\n";
    print "<TR BGCOLOR=\"#BFBFBF\">\n";
    print "  <TH VALIGN=\"top\" ALIGN=\"left\">Part</TH>\n";
    print "  <TH VALIGN=\"top\" ALIGN=\"left\">Value</TH>\n";

    print "</TR><TR>\n";
    print "  <TH VALIGN=\"top\" ALIGN=left>Product</TH>\n";
    print "  <TD VALIGN=\"top\">$product</TD>\n";

    print "</TR><TR>\n";
    print "  <TH VALIGN=\"top\" ALIGN=left>Description</TH>\n";
    print "  <TD VALIGN=\"top\">$description</TD>\n";

    if (Param('usetargetmilestone')) {
        print "</TR><TR>\n";
        print "  <TH VALIGN=\"top\" ALIGN=left>Milestone URL</TH>\n";
        print "  <TD VALIGN=\"top\"><A HREF=\"$milestoneurl\">$milestoneurl</A></TD>\n";
    }

    print "</TR><TR>\n";
    print "  <TH VALIGN=\"top\" ALIGN=left>Closed for bugs</TH>\n";
    print "  <TD VALIGN=\"top\">$disallownew</TD>\n";

    print "</TR><TR>\n";
    print "  <TH VALIGN=\"top\" ALIGN=left>Components</TH>\n";
    print "  <TD VALIGN=\"top\">";
    SendSQL("SELECT value,description
             FROM components
             WHERE program=" . SqlQuote($product));
    if (MoreSQLData()) {
        print "<TABLE>";
        while ( MoreSQLData() ) {
            my ($component, $description) = FetchSQLData();
            $description ||= "<FONT COLOR=\"red\">description missing</FONT>";
            print "<TR><TH ALIGN=right VALIGN=top>$component:</TH>";
            print "<TD VALIGN=top>$description</TD></TR>\n";
        }
        print "</TABLE>\n";
    } else {
        print "<FONT COLOR=\"red\">missing</FONT>";
    }

    print "</TD>\n</TR><TR>\n";
    print "  <TH VALIGN=\"top\" ALIGN=left>Versions</TH>\n";
    print "  <TD VALIGN=\"top\">";
    SendSQL("SELECT value
             FROM versions
             WHERE program=" . SqlQuote($product) . "
             ORDER BY value");
    if (MoreSQLData()) {
        my $br = 0;
        while ( MoreSQLData() ) {
            my ($version) = FetchSQLData();
            print "<BR>" if $br;
            print $version;
            $br = 1;
        }
    } else {
        print "<FONT COLOR=\"red\">missing</FONT>";
    }


    print "</TD>\n</TR><TR>\n";
    print "  <TH VALIGN=\"top\" ALIGN=left>Bugs</TH>\n";
    print "  <TD VALIGN=\"top\">";
    SendSQL("SELECT count(bug_id),product
             FROM bugs
             GROUP BY product
             HAVING product=" . SqlQuote($product));
    my $bugs = FetchOneColumn();
    print $bugs || 'none';


    print "</TD>\n</TR></TABLE>";

    print "<CENTER><H2>Confirmation</H2>\n";

    if ($bugs) {
        if (!Param("allowbugdeletion")) {
            print "Sorry, there are $bugs bugs outstanding for this product.
You must reassign those bugs to another product before you can delete this
one.</CENTER><P>\n";
            PutTrailer($localtrailer);
            exit;
        }
        print "<TABLE BORDER=0 CELLPADDING=20 WIDTH=\"70%\" BGCOLOR=\"red\" ALIGN=center><TR><TD>\n",
              "There are bugs entered for this product!  When you delete this ",
              "product, <B><BLINK>all</BLINK><B> stored bugs will be deleted, too. ",
              "You could not even see a bug history anymore!\n",
              "</TD></TR></TABLE>\n";
    }

    print "<P>Do you really want to delete this product?<P>\n";
    print "<FORM METHOD=POST ACTION=editproducts.cgi>\n";
    print "<INPUT TYPE=SUBMIT VALUE=\"Yes, delete\">\n";
    print "<INPUT TYPE=HIDDEN NAME=\"action\" VALUE=\"delete\">\n";
    print "<INPUT TYPE=HIDDEN NAME=\"product\" VALUE=\"$product\">\n";
    print "</FORM>\n</CENTER>\n";

    PutTrailer($localtrailer);
	PutFooter();
    exit;
}


#
# action='delete' -> really delete the product
#
if ($action eq 'delete') {
    PutHeader("Deleting product");
    CheckProduct($product);

    # lock the tables before we start to change everything:

    SendSQL("LOCK TABLES attachments WRITE,
                         bugs WRITE,
                         bugs_activity WRITE,
                         components WRITE,
                         dependencies WRITE,
                         versions WRITE,
                         products WRITE");

    # According to MySQL doc I cannot do a DELETE x.* FROM x JOIN Y,
    # so I have to iterate over bugs and delete all the indivial entries
    # in bugs_activies and attachments.

    SendSQL("SELECT bug_id
             FROM bugs
             WHERE product=" . SqlQuote($product));
    while (MoreSQLData()) {
        my $bugid = FetchOneColumn();

        my $query = $::db->query("DELETE FROM attachments WHERE bug_id=$bugid")
                or die "$::db_errstr";
        $query = $::db->query("DELETE FROM bugs_activity WHERE bug_id=$bugid")
                or die "$::db_errstr";
        $query = $::db->query("DELETE FROM dependencies WHERE blocked=$bugid")
                or die "$::db_errstr";
    }
    print "Attachments, bug activity and dependencies deleted.<BR>\n";


    # Deleting the rest is easier:

    SendSQL("DELETE FROM bugs
             WHERE product=" . SqlQuote($product));
    print "Bugs deleted.<BR>\n";

    SendSQL("DELETE FROM components
             WHERE program=" . SqlQuote($product));
    print "Components deleted.<BR>\n";

    SendSQL("DELETE FROM versions
             WHERE program=" . SqlQuote($product));
    print "Versions deleted.<P>\n";

    SendSQL("DELETE FROM products
             WHERE product=" . SqlQuote($product));
    print "Product '$product' deleted.<BR>\n";
    SendSQL("UNLOCK TABLES");

    unlink "data/versioncache";
    PutTrailer($localtrailer);
    PutFooter();
	exit;
}

#
# action='edit' -> present the edit products from
#
# (next action would be 'update')
#
if ($action eq 'edit') {
    PutHeader("Edit product");
    CheckProduct($product);

    # get data of product
    SendSQL("SELECT description,milestoneurl,disallownew
             FROM products
             WHERE product=" . SqlQuote($product));
    my ($description, $milestoneurl, $disallownew) = FetchSQLData();

    print "<FORM METHOD=POST ACTION=editproducts.cgi>\n";
    print "<TABLE BORDER=0 CELLPADDING=4 CELLSPACING=0 ALIGN=center><TR>\n";

    EmitFormElements($product, $description, $milestoneurl, $disallownew);
    
    print "</TR><TR>\n";
	print "<TH ALIGN=right VALIGN=center>Other:</TH>\n";
    print "<TH ALIGN=left><A HREF=\"editcomponents.cgi?product=", url_quote($product), "\">";
	print "Edit components for $product</A><BR>\n";
    print "<A HREF=\"editversions.cgi?product=", url_quote($product), "\">Edit versions for $product</A></TH>\n";
    print "</TR><TR>\n";
    print "  <TH ALIGN=\"right\">Bugs:</TH>\n";
    print "  <TD>";
    SendSQL("SELECT count(bug_id), product " . 
             "FROM bugs " .
             "GROUP BY product " .
             "HAVING product = " . SqlQuote($product));
    my $bugs = '';
    $bugs = FetchOneColumn() if MoreSQLData();
    print $bugs || 'none';
	print "</TD>\n</TR>\n";

	# begin new group stuff
	SendSQL("select id from products where product = " . SqlQuote($product));
	my $productid = FetchOneColumn();

    SendSQL("select groupid, description from groups order by groupid");
    my %grouplist;
    my @row;
    while (@row = FetchSQLData()) {
        $grouplist{$row[0]} = $row[1];
    }

    SendSQL("select groupid from product_group where productid = $productid");
    my @groupbelong;
    while (@row = FetchSQLData()) {
        push (@groupbelong, $row[0]);
    }

	print qq{
	<TR>
		<TH ALIGN=right VALIGN=center>Product can<BR>seen by:</TH>
		<TD ALIGN=left>
		<U>All <B>unchecked</B> same as <B>everyone</B></U><BR>
<INPUT TYPE=HIDDEN NAME=productid VALUE="$productid">
};

    my $count = 0;
    foreach my $i (keys %grouplist) {
        my $c;
        if (lsearch(\@groupbelong, $i) >= 0) {
            $c = 'CHECKED';
        } else {
            $c = '';
        }
        print qq{
<INPUT TYPE=checkbox NAME=group-$i $c> $grouplist{$i}<br>
};
    }

    print "</TD>\n</TR></TABLE>\n";
	print "<CENTER>\n";
    print "<INPUT TYPE=HIDDEN NAME=\"productold\" VALUE=\"$product\">\n";
    print "<INPUT TYPE=HIDDEN NAME=\"descriptionold\" VALUE=\"$description\">\n";
    print "<INPUT TYPE=HIDDEN NAME=\"milestoneurlold\" VALUE=\"$milestoneurl\">\n";
    print "<INPUT TYPE=HIDDEN NAME=\"disallownewold\" VALUE=\"$disallownew\">\n";
    print "<INPUT TYPE=HIDDEN NAME=\"action\" VALUE=\"update\">\n";
    print "<INPUT TYPE=SUBMIT VALUE=\"Update\">\n";
    print "</FORM>\n</CENTER>\n";

    my $x = $localtrailer;
    $x =~ s/more/other/;
    PutTrailer($x);
	PutFooter();
    exit;
}

#
# action='update' -> update the product
#
if ($action eq 'update') {
    PutHeader("Update product");

    my $productold      = trim($::FORM{productold}      || '');
    my $description     = trim($::FORM{description}     || '');
    my $descriptionold  = trim($::FORM{descriptionold}  || '');
    my $disallownew     = trim($::FORM{disallownew}     || '');
    my $disallownewold  = trim($::FORM{disallownewold}  || '');
    my $milestoneurl    = trim($::FORM{milestoneurl}    || '');
    my $milestoneurlold = trim($::FORM{milestoneurlold} || '');

    CheckProduct($productold);

    # Note that the order of this tests is important. If you change
    # them, be sure to test for WHERE='$product' or WHERE='$productold'

    SendSQL("LOCK TABLES bugs WRITE,
                         components WRITE,
                         products WRITE,
                         versions WRITE,
						 product_group WRITE");

    if ($disallownew != $disallownewold) {
        $disallownew ||= 0;
        SendSQL("UPDATE products
                 SET disallownew=$disallownew
                 WHERE product=" . SqlQuote($productold));
        print "Updated bug submit status.<BR>\n";
    }

    if ($description ne $descriptionold) {
        unless ($description) {
            print "Sorry, I can't delete the description.";
            PutTrailer($localtrailer);
            SendSQL("UNLOCK TABLES");
            exit;
        }
        SendSQL("UPDATE products
                 SET description=" . SqlQuote($description) . "
                 WHERE product=" . SqlQuote($productold));
        print "Updated description.<BR>\n";
    }

    if (Param('usetargetmilestone') && $milestoneurl ne $milestoneurlold) {
        SendSQL("UPDATE products
                 SET milestoneurl=" . SqlQuote($milestoneurl) . "
                 WHERE product=" . SqlQuote($productold));
        print "Updated mile stone URL.<BR>\n";
    }


    if ($product ne $productold) {
        unless ($product) {
            print "Sorry, I can't delete the product name.";
            PutTrailer($localtrailer);
            SendSQL("UNLOCK TABLES");
            exit;
        }
        if (TestProduct($product)) {
            print "Sorry, product name '$product' is already in use.";
            PutTrailer($localtrailer);
            SendSQL("UNLOCK TABLES");
            exit;
        }

        SendSQL("UPDATE bugs
                 SET product=" . SqlQuote($product) . "
                 WHERE product=" . SqlQuote($productold));
        SendSQL("UPDATE components
                 SET program=" . SqlQuote($product) . "
                 WHERE program=" . SqlQuote($productold));
        SendSQL("UPDATE products
                 SET product=" . SqlQuote($product) . "
                 WHERE product=" . SqlQuote($productold));
        SendSQL("UPDATE versions
                 SET program='$product'
                 WHERE program=" . SqlQuote($productold));

        unlink "data/versioncache";
        print "Updated product name.<BR>\n";
    }

	# Update product_group table
	my $productid = $::FORM{'productid'};
	my $flag = 0;
    SendSQL("delete from product_group where productid = $productid");
    foreach my $groupid (grep(/^group-.*$/, keys %::FORM)) {
    	if ($::FORM{$groupid}) {
    		$groupid =~ s/^group-//;
    		SendSQL("insert into product_group values ($productid, $groupid)");
			$flag = 1;
    	}
    }
	if ($flag) { print "Updated group permissions.<BR>\n"; }

    SendSQL("UNLOCK TABLES");

    PutTrailer($localtrailer);
    exit;
}

#
# No valid action found
#
PutHeader("Error");
print "I don't have a clue what you want.<BR>\n";

foreach ( sort keys %::FORM) {
    print "$_: $::FORM{$_}<BR>\n";
}
