#!/usr/bonsaitools/bin/perl -w
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
# The Original Code is mozilla.org code.
#
# The Initial Developer of the Original Code is Holger
# Schurig. Portions created by Holger Schurig are
# Copyright (C) 1999 Holger Schurig. All
# Rights Reserved.
#
# Contributor(s): Holger Schurig <holgerschurig@nikocity.de>
#               Terry Weissman <terry@mozilla.org>
#               Dawn Endico <endico@mozilla.org>
#               Joe Robins <jmrobins@tgix.com>
#
# Direct any questions on this source code to
#
# Holger Schurig <holgerschurig@nikocity.de>

use diagnostics;
use strict;

require "CGI.pl";
require "globals.pl";

# Shut up misguided -w warnings about "used only once".  "use vars" just
# doesn't work for me.

sub sillyness {
    my $zz;
    $zz = $::unconfirmedstate;
}

my $userid = 0;

# TestProduct:  just returns if the specified product does exists
# CheckProduct: same check, optionally  emit an error text

sub TestProduct ($)
{
    my $prod = shift;

    # does the product exist?
    SendSQL("SELECT product
             FROM products
             WHERE product=" . SqlQuote($prod));
    return FetchOneColumn();
}

sub CheckProduct ($)
{
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

    unless (CanSeeProduct($userid, $prod)) {
        print "Sorry, You do not have permission to modify product '$prod'.";
        PutTrailer();
        exit;
    }
}


#
# Displays the form to edit a products parameters
#

sub EmitFormElements ($$$$$$$$)
{
    my ($product, $description, $milestoneurl, $disallownew,
        $votesperuser, $maxvotesperbug, $votestoconfirm, $defaultmilestone)
        = @_;

    $product = value_quote($product);
    $description = value_quote($description);

    print "  <TH ALIGN=\"right\">Product:</TH>\n";
    print "  <TD><INPUT SIZE=64 MAXLENGTH=64 NAME=\"product\" VALUE=\"$product\"></TD>\n";
    print "</TR><TR>\n";

    print "  <TH ALIGN=\"right\">Description:</TH>\n";
    print "  <TD><TEXTAREA ROWS=4 COLS=64 WRAP=VIRTUAL NAME=\"description\">$description</TEXTAREA></TD>\n";

    $defaultmilestone = value_quote($defaultmilestone);
    if (Param('usetargetmilestone')) {
        $milestoneurl = value_quote($milestoneurl);
        print "</TR><TR>\n";
        print "  <TH ALIGN=\"right\">URL describing milestones for this product:</TH>\n";
        print "  <TD><INPUT TYPE=TEXT SIZE=64 MAXLENGTH=255 NAME=\"milestoneurl\" VALUE=\"$milestoneurl\"></TD>\n";

        print "</TR><TR>\n";
        print "  <TH ALIGN=\"right\">Default milestone:</TH>\n";
        
        print "  <TD><INPUT TYPE=TEXT SIZE=20 MAXLENGTH=20 NAME=\"defaultmilestone\" VALUE=\"$defaultmilestone\"></TD>\n";
    } else {
        print qq{<INPUT TYPE=HIDDEN NAME="defaultmilestone" VALUE="$defaultmilestone">\n};
    }

    print "</TR><TR>\n";
    print "  <TH ALIGN=\"right\">Closed for bug entry:</TH>\n";
    my $closed = $disallownew ? "CHECKED" : "";
    print "  <TD><INPUT TYPE=CHECKBOX NAME=\"disallownew\" $closed VALUE=\"1\"></TD>\n";

    print "</TR><TR>\n";
    print "  <TH ALIGN=\"right\">Maximum votes per person:</TH>\n";
    print "  <TD><INPUT SIZE=5 MAXLENGTH=5 NAME=\"votesperuser\" VALUE=\"$votesperuser\"></TD>\n";

    print "</TR><TR>\n";
    print "  <TH ALIGN=\"right\">Maximum votes a person can put on a single bug:</TH>\n";
    print "  <TD><INPUT SIZE=5 MAXLENGTH=5 NAME=\"maxvotesperbug\" VALUE=\"$maxvotesperbug\"></TD>\n";

    print "</TR><TR>\n";
    print "  <TH ALIGN=\"right\">Number of votes a bug in this product needs to automatically get out of the <A HREF=\"bug_status.html#status\">UNCONFIRMED</A> state:</TH>\n";
    print "  <TD><INPUT SIZE=5 MAXLENGTH=5 NAME=\"votestoconfirm\" VALUE=\"$votestoconfirm\"></TD>\n";

    # Find list of groups that this product can be marked private to
    print "</TR><TR>\n";
    print "  <TH ALIGN=\"right\">Group Access:</TH>\n";
    print "  <TD><TABLE>\n";
    my $productid = 0;
    my %productbelongs = ();
    if ($product) {
        SendSQL("select product_id from products where product = " . SqlQuote($product));
        $productid = FetchOneColumn();
        SendSQL("select groups.group_id from groups, product_group_map " . 
                "where groups.group_id = product_group_map.group_id ". 
                "and product_group_map.product_id = $productid");
        while (my ($groupid) = FetchSQLData()) {
            $productbelongs{$groupid} = 1;
        }
    }

    my %groupsbelong = ();
    my %groupsbelongname = ();
    my %groupsbelongdesc = (); 
    SendSQL("select groups.group_id, groups.name, groups.description " .
            "from groups, user_group_map " .
            "where groups.group_id = user_group_map.group_id " .
            "and groups.isbuggroup = 1 " .
            "and user_group_map.user_id = $userid");
    while (my ($groupid, $name, $desc) = FetchSQLData()) {
        $groupsbelong{$groupid} = 1;
        $groupsbelongname{$groupid} = $name;
        $groupsbelongdesc{$groupid} = $desc;
    }

    foreach my $groupid (keys %groupsbelong) {
        my $checked = defined ($productbelongs{$groupid}) ? "CHECKED" : "";
        print "<TR>\n<TD><INPUT TYPE=\"checkbox\" NAME=\"group_$groupid\" VALUE=\"$groupid\" $checked></TD>\n";
        print "<TD><B>" . ucfirst($groupsbelongname{$groupid}) . ":</B> ";
        print "$groupsbelongdesc{$groupid}</TD>\n</TR>\n";
    }
    print "</TABLE>\n</TD>\n";
    print "<INPUT TYPE=\"hidden\" NAME=\"productid\" VALUE=\"$productid\">\n";
}


#
# Displays a text like "a.", "a or b.", "a, b or c.", "a, b, c or d."
#

sub PutTrailer (@)
{
    my (@links) = ("Back to the <A HREF=\"query.cgi\">query page</A>", @_);

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
    PutFooter();
}







#
# Preliminary checks:
#

$userid = confirm_login();

print "Content-type: text/html\n\n";

unless (UserInGroup($userid, "editcomponents")) {
    PutHeader("Not allowed");
    print "Sorry, you aren't a member of the 'editcomponents' group.\n";
    print "And so, you aren't allowed to add, modify or delete products.\n";
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

    SendSQL("SELECT products.product,description,disallownew,
                    votesperuser,maxvotesperbug,votestoconfirm,COUNT(bug_id)
             FROM products LEFT JOIN bugs
               ON products.product=bugs.product
             GROUP BY products.product
             ORDER BY products.product");
    print "<TABLE BORDER=1 CELLPADDING=4 CELLSPACING=0><TR BGCOLOR=\"#6666FF\">\n";
    print "  <TH ALIGN=\"left\">Edit product ...</TH>\n";
    print "  <TH ALIGN=\"left\">Description</TH>\n";
    print "  <TH ALIGN=\"left\">Status</TH>\n";
    print "  <TH ALIGN=\"left\">Votes<br>per<br>user</TH>\n";
    print "  <TH ALIGN=\"left\">Max<br>Votes<br>per<br>bug</TH>\n";
    print "  <TH ALIGN=\"left\">Votes<br>to<br>confirm</TH>\n";
    print "  <TH ALIGN=\"left\">Bugs</TH>\n";
    print "  <TH ALIGN=\"left\">Action</TH>\n";
    print "</TR>";
    while ( MoreSQLData() ) {
        my ($product, $description, $disallownew, $votesperuser,
            $maxvotesperbug, $votestoconfirm, $bugs) = FetchSQLData();

        # Skip this product if user cannot see it
        next if !CanSeeProduct($userid, $product);

        $description ||= "<FONT COLOR=\"red\">missing</FONT>";
        $disallownew = $disallownew ? 'closed' : 'open';
        $bugs        ||= 'none';
        print "<TR>\n";
        print "  <TD VALIGN=\"top\"><A HREF=\"editproducts.cgi?action=edit&product=", url_quote($product), "\"><B>$product</B></A></TD>\n";
        print "  <TD VALIGN=\"top\">$description</TD>\n";
        print "  <TD VALIGN=\"top\">$disallownew</TD>\n";
        print "  <TD VALIGN=\"top\" ALIGN=\"right\">$votesperuser</TD>\n";
        print "  <TD VALIGN=\"top\" ALIGN=\"right\">$maxvotesperbug</TD>\n";
        print "  <TD VALIGN=\"top\" ALIGN=\"right\">$votestoconfirm</TD>\n";
        print "  <TD VALIGN=\"top\" ALIGN=\"right\">$bugs</TD>\n";
        print "  <TD VALIGN=\"top\"><A HREF=\"editproducts.cgi?action=del&product=", url_quote($product), "\">Delete</A></TD>\n";
        print "</TR>";
    }
    print "<TR>\n";
    print "  <TD VALIGN=\"top\" COLSPAN=7>Add a new product</TD>\n";
    print "  <TD VALIGN=\"top\" ALIGN=\"middle\"><FONT SIZE =-1><A HREF=\"editproducts.cgi?action=add\">Add</A></FONT></TD>\n";
    print "</TR></TABLE>\n";

    PutTrailer();
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
    print "<TABLE BORDER=0 CELLPADDING=4 CELLSPACING=0><TR>\n";

    EmitFormElements('', '', '', 0, 0, 10000, 0, "---");

    print "</TR><TR>\n";
    print "  <TH ALIGN=\"right\">Version:</TH>\n";
    print "  <TD><INPUT SIZE=64 MAXLENGTH=255 NAME=\"version\" VALUE=\"unspecified\"></TD>\n";

    print "</TABLE>\n<HR>\n";
    print "<INPUT TYPE=SUBMIT VALUE=\"Add\">\n";
    print "<INPUT TYPE=HIDDEN NAME=\"action\" VALUE=\"new\">\n";
    print "</FORM>";

    my $other = $localtrailer;
    $other =~ s/more/other/;
    PutTrailer($other);
    exit;
}



#
# action='new' -> add product entered in the 'action=add' screen
#

if ($action eq 'new') {
    PutHeader("Adding new product");

    # Cleanups and validity checks

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
    my $votesperuser = $::FORM{votesperuser};
    $votesperuser ||= 0;
    my $maxvotesperbug = $::FORM{maxvotesperbug};
    $maxvotesperbug = 10000 if !defined $maxvotesperbug;
    my $votestoconfirm = $::FORM{votestoconfirm};
    $votestoconfirm ||= 0;
    my $defaultmilestone = $::FORM{defaultmilestone} || "---";

    # Add the new product.
    SendSQL("INSERT INTO products ( " .
            "product, description, milestoneurl, disallownew, votesperuser, " .
            "maxvotesperbug, votestoconfirm, defaultmilestone" .
            " ) VALUES ( " .
            SqlQuote($product) . "," .
            SqlQuote($description) . "," .
            SqlQuote($milestoneurl) . "," .
            $disallownew . "," .
            "$votesperuser, $maxvotesperbug, $votestoconfirm, " .
            SqlQuote($defaultmilestone) . ")");

    SendSQL("SELECT LAST_INSERT_ID()");
    my $productid = FetchOneColumn();

    SendSQL("INSERT INTO versions ( " .
          "value, program" .
          " ) VALUES ( " .
          SqlQuote($version) . "," .
          SqlQuote($product) . ")" );

    SendSQL("INSERT INTO milestones (product, value) VALUES (" .
            SqlQuote($product) . ", " . SqlQuote($defaultmilestone) . ")");

    # Update group permissions for this product
    
    my %newgroups = ();
    foreach (keys %::FORM) {
        next unless /^group_/;
        detaint_natural($::FORM{$_});
        $newgroups{$::FORM{$_}} = 1;
    }

    my %groupsbelong = ();
    SendSQL("select groups.group_id from groups, user_group_map " .
            "where groups.group_id = user_group_map.group_id " .
            "and groups.isbuggroup = 1 " .
            "and user_group_map.user_id = $userid");
    while (my ($groupid) = FetchSQLData()) {
        $groupsbelong{$groupid} = 1;
    }

    foreach my $groupid (keys %groupsbelong) {
        if ($newgroups{$groupid}) {
            SendSQL("INSERT INTO product_group_map (product_id, group_id) VALUES ($productid, $groupid)");
        }
    }

    # Make versioncache flush
    unlink "data/versioncache";

    print "OK, done.<p>\n";
    PutTrailer($localtrailer, "<a href=\"editcomponents.cgi?action=add&product=" . url_quote($product) . "\">add</a> components to this new product.");
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
    my $milestonelink = $milestoneurl ? "<a href=\"$milestoneurl\">$milestoneurl</a>"
                                      : "<font color=\"red\">missing</font>";
    $description ||= "<FONT COLOR=\"red\">description missing</FONT>";
    $disallownew = $disallownew ? 'closed' : 'open';
    
    print "<TABLE BORDER=1 CELLPADDING=4 CELLSPACING=0>\n";
    print "<TR BGCOLOR=\"#6666FF\">\n";
    print "  <TH VALIGN=\"top\" ALIGN=\"left\">Part</TH>\n";
    print "  <TH VALIGN=\"top\" ALIGN=\"left\">Value</TH>\n";

    print "</TR><TR>\n";
    print "  <TD VALIGN=\"top\">Product:</TD>\n";
    print "  <TD VALIGN=\"top\">$product</TD>\n";

    print "</TR><TR>\n";
    print "  <TD VALIGN=\"top\">Description:</TD>\n";
    print "  <TD VALIGN=\"top\">$description</TD>\n";

    if (Param('usetargetmilestone')) {
        print "</TR><TR>\n";
        print "  <TD VALIGN=\"top\">Milestone URL:</TD>\n";
        print "  <TD VALIGN=\"top\">$milestonelink</TD>\n";
    }

    print "</TR><TR>\n";
    print "  <TD VALIGN=\"top\">Closed for bugs:</TD>\n";
    print "  <TD VALIGN=\"top\">$disallownew</TD>\n";

    print "</TR><TR>\n";
    print "  <TD VALIGN=\"top\">Components:</TD>\n";
    print "  <TD VALIGN=\"top\">";
    SendSQL("SELECT value,description
             FROM components
             WHERE program=" . SqlQuote($product));
    if (MoreSQLData()) {
        print "<table>";
        while ( MoreSQLData() ) {
            my ($component, $description) = FetchSQLData();
            $description ||= "<FONT COLOR=\"red\">description missing</FONT>";
            print "<tr><th align=right valign=top>$component:</th>";
            print "<td valign=top>$description</td></tr>\n";
        }
        print "</table>\n";
    } else {
        print "<FONT COLOR=\"red\">missing</FONT>";
    }

    print "</TD>\n</TR><TR>\n";
    print "  <TD VALIGN=\"top\">Versions:</TD>\n";
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

    #
    # Adding listing for associated target milestones - matthew@zeroknowledge.com
    #
    if (Param('usetargetmilestone')) {
        print "</TD>\n</TR><TR>\n";
        print "  <TH ALIGN=\"right\" VALIGN=\"top\"><A HREF=\"editmilestones.cgi?product=", url_quote($product), "\">Edit milestones:</A></TH>\n";
        print "  <TD>";
        SendSQL("SELECT value
                 FROM milestones
                 WHERE product=" . SqlQuote($product) . "
                 ORDER BY sortkey,value");
        if(MoreSQLData()) {
            my $br = 0;
            while ( MoreSQLData() ) {
                my ($milestone) = FetchSQLData();
                print "<BR>" if $br;
                print $milestone;
                $br = 1;
            }
        } else {
            print "<FONT COLOR=\"red\">missing</FONT>";
        }
    }

    print "</TD>\n</TR><TR>\n";
    print "  <TD VALIGN=\"top\">Bugs:</TD>\n";
    print "  <TD VALIGN=\"top\">";
    SendSQL("SELECT count(bug_id),product
             FROM bugs
             GROUP BY product
             HAVING product=" . SqlQuote($product));
    my $bugs = FetchOneColumn();
    print $bugs || 'none';


    print "</TD>\n</TR></TABLE>";

    print "<H2>Confirmation</H2>\n";

    if ($bugs) {
        if (!Param("allowbugdeletion")) {
            print "Sorry, there are $bugs bugs outstanding for this product.
You must reassign those bugs to another product before you can delete this
one.";
            PutTrailer($localtrailer);
            exit;
        }
        print "<TABLE BORDER=0 CELLPADDING=20 WIDTH=\"70%\" BGCOLOR=\"red\"><TR><TD>\n",
              "There are bugs entered for this product!  When you delete this ",
              "product, <B><BLINK>all</BLINK><B> stored bugs will be deleted, too. ",
              "You could not even see a bug history anymore!\n",
              "</TD></TR></TABLE>\n";
    }

    print "<P>Do you really want to delete this product?<P>\n";
    print "<FORM METHOD=POST ACTION=editproducts.cgi>\n";
    print "<INPUT TYPE=SUBMIT VALUE=\"Yes, delete\">\n";
    print "<INPUT TYPE=HIDDEN NAME=\"action\" VALUE=\"delete\">\n";
    print "<INPUT TYPE=HIDDEN NAME=\"product\" VALUE=\"" .
        value_quote($product) . "\">\n";
    print "</FORM>";

    PutTrailer($localtrailer);
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
                         products WRITE,
                         groups WRITE,
                         profiles WRITE,
                         milestones WRITE,
                         product_group_map WRITE");

    SendSQL("SELECT product_id FROM products WHERE product = " . SqlQuote($product));
    my $productid = FetchOneColumn();

    # According to MySQL doc I cannot do a DELETE x.* FROM x JOIN Y,
    # so I have to iterate over bugs and delete all the indivial entries
    # in bugs_activies and attachments.

    if (Param("allowbugdeletion")) {
        SendSQL("SELECT bug_id
             FROM bugs
             WHERE product=" . SqlQuote($product));
        while (MoreSQLData()) {
            my $bugid = FetchOneColumn();

            PushGlobalSQLState();
            SendSQL("DELETE FROM attachments WHERE bug_id=$bugid");
            SendSQL("DELETE FROM bugs_activity WHERE bug_id=$bugid");
            SendSQL("DELETE FROM dependencies WHERE blocked=$bugid");
            PopGlobalSQLState();
        }
        print "Attachments, bug activity and dependencies deleted.<BR>\n";


        # Deleting the rest is easier:

        SendSQL("DELETE FROM bugs
             WHERE product=" . SqlQuote($product));
        print "Bugs deleted.<BR>\n";
    }

    SendSQL("DELETE FROM components
             WHERE program=" . SqlQuote($product));
    print "Components deleted.<BR>\n";

    SendSQL("DELETE FROM versions
             WHERE program=" . SqlQuote($product));
    print "Versions deleted.<P>\n";

    # deleting associated target milestones - matthew@zeroknowledge.com
    SendSQL("DELETE FROM milestones
             WHERE product=" . SqlQuote($product));
    print "Milestones deleted.<BR>\n";

    # Deleting any product groups
    SendSQL("DELETE FROM product_group_map WHERE product_id = $productid");
    print "Product groups deleted.<BR>\n";

    SendSQL("DELETE FROM products
             WHERE product=" . SqlQuote($product));
    print "Product '$product' deleted.<BR>\n";

    SendSQL("UNLOCK TABLES");

    unlink "data/versioncache";
    PutTrailer($localtrailer);
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
    SendSQL("SELECT description,milestoneurl,disallownew,
                    votesperuser,maxvotesperbug,votestoconfirm,defaultmilestone
             FROM products
             WHERE product=" . SqlQuote($product));
    my ($description, $milestoneurl, $disallownew,
        $votesperuser, $maxvotesperbug, $votestoconfirm, $defaultmilestone) =
        FetchSQLData();

    print "<FORM METHOD=POST ACTION=editproducts.cgi>\n";
    print "<TABLE  BORDER=0 CELLPADDING=4 CELLSPACING=0><TR>\n";

    EmitFormElements($product, $description, $milestoneurl, 
                     $disallownew, $votesperuser, $maxvotesperbug,
                     $votestoconfirm, $defaultmilestone);
    
    print "</TR><TR VALIGN=top>\n";
    print "  <TH ALIGN=\"right\"><A HREF=\"editcomponents.cgi?product=", url_quote($product), "\">Edit components:</A></TH>\n";
    print "  <TD>";
    SendSQL("SELECT value,description
             FROM components
             WHERE program=" . SqlQuote($product));
    if (MoreSQLData()) {
        print "<table>";
        while ( MoreSQLData() ) {
            my ($component, $description) = FetchSQLData();
            $description ||= "<FONT COLOR=\"red\">description missing</FONT>";
            print "<tr><th align=right valign=top>$component:</th>";
            print "<td valign=top>$description</td></tr>\n";
        }
        print "</table>\n";
    } else {
        print "<FONT COLOR=\"red\">missing</FONT>";
    }


    print "</TD>\n</TR><TR>\n";
    print "  <TH ALIGN=\"right\" VALIGN=\"top\"><A HREF=\"editversions.cgi?product=", url_quote($product), "\">Edit versions:</A></TH>\n";
    print "  <TD>";
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

    #
    # Adding listing for associated target milestones - matthew@zeroknowledge.com
    #
    if (Param('usetargetmilestone')) {
        print "</TD>\n</TR><TR>\n";
        print "  <TH ALIGN=\"right\" VALIGN=\"top\"><A HREF=\"editmilestones.cgi?product=", url_quote($product), "\">Edit milestones:</A></TH>\n";
        print "  <TD>";
        SendSQL("SELECT value
                 FROM milestones
                 WHERE product=" . SqlQuote($product) . "
                 ORDER BY sortkey,value");
        if(MoreSQLData()) {
            my $br = 0;
            while ( MoreSQLData() ) {
                my ($milestone) = FetchSQLData();
                print "<BR>" if $br;
                print $milestone;
                $br = 1;
            }
        } else {
            print "<FONT COLOR=\"red\">missing</FONT>";
        }
    }

    print "</TD>\n</TR><TR>\n";
    print "  <TH ALIGN=\"right\">Bugs:</TH>\n";
    print "  <TD>";
    SendSQL("SELECT count(bug_id),product
             FROM bugs
             GROUP BY product
             HAVING product=" . SqlQuote($product));
    my $bugs = '';
    $bugs = FetchOneColumn() if MoreSQLData();
    print $bugs || 'none';

    print "</TD>\n</TR></TABLE>\n";

    print "<INPUT TYPE=HIDDEN NAME=\"productold\" VALUE=\"" .
        value_quote($product) . "\">\n";
    print "<INPUT TYPE=HIDDEN NAME=\"descriptionold\" VALUE=\"" .
        value_quote($description) . "\">\n";
    print "<INPUT TYPE=HIDDEN NAME=\"milestoneurlold\" VALUE=\"" .
        value_quote($milestoneurl) . "\">\n";
    print "<INPUT TYPE=HIDDEN NAME=\"disallownewold\" VALUE=\"$disallownew\">\n";
    print "<INPUT TYPE=HIDDEN NAME=\"votesperuserold\" VALUE=\"$votesperuser\">\n";
    print "<INPUT TYPE=HIDDEN NAME=\"maxvotesperbugold\" VALUE=\"$maxvotesperbug\">\n";
    print "<INPUT TYPE=HIDDEN NAME=\"votestoconfirmold\" VALUE=\"$votestoconfirm\">\n";
    $defaultmilestone = value_quote($defaultmilestone);
    print "<INPUT TYPE=HIDDEN NAME=\"defaultmilestoneold\" VALUE=\"$defaultmilestone\">\n";
    print "<INPUT TYPE=HIDDEN NAME=\"action\" VALUE=\"update\">\n";
    print "<INPUT TYPE=SUBMIT VALUE=\"Update\">\n";

    print "</FORM>";

    my $x = $localtrailer;
    $x =~ s/more/other/;
    PutTrailer($x);
    exit;
}



#
# action='update' -> update the product
#

if ($action eq 'update') {
    PutHeader("Update product");
    
    my $productid           = trim($::FORM{productid}           || 0);
    my $productold          = trim($::FORM{productold}          || '');
    my $description         = trim($::FORM{description}         || '');
    my $descriptionold      = trim($::FORM{descriptionold}      || '');
    my $disallownew         = trim($::FORM{disallownew}         || '');
    my $disallownewold      = trim($::FORM{disallownewold}      || '');
    my $milestoneurl        = trim($::FORM{milestoneurl}        || '');
    my $milestoneurlold     = trim($::FORM{milestoneurlold}     || '');
    my $votesperuser        = trim($::FORM{votesperuser}        || 0);
    my $votesperuserold     = trim($::FORM{votesperuserold}     || 0);
    my $maxvotesperbug      = trim($::FORM{maxvotesperbug}      || 0);
    my $maxvotesperbugold   = trim($::FORM{maxvotesperbugold}   || 0);
    my $votestoconfirm      = trim($::FORM{votestoconfirm}      || 0);
    my $votestoconfirmold   = trim($::FORM{votestoconfirmold}   || 0);
    my $defaultmilestone    = trim($::FORM{defaultmilestone}    || '---');
    my $defaultmilestoneold = trim($::FORM{defaultmilestoneold} || '---');

    my $checkvotes = 0;

    CheckProduct($productold);

    if ($maxvotesperbug !~ /^\d+$/ || $maxvotesperbug <= 0) {
        print "Sorry, the max votes per bug must be a positive integer.";
        PutTrailer($localtrailer);
        exit;
    }

    # Note that the order of this tests is important. If you change
    # them, be sure to test for WHERE='$product' or WHERE='$productold'

    SendSQL("LOCK TABLES bugs WRITE,
                         components WRITE,
                         products WRITE,
                         versions WRITE,
                         groups WRITE,
                         profiles WRITE,
                         milestones WRITE,
                         user_group_map WRITE,
                         product_group_map WRITE");

    if ($disallownew ne $disallownewold) {
        $disallownew ||= 0;
        SendSQL("UPDATE products
                 SET disallownew=$disallownew
                 WHERE product=" . SqlQuote($productold));
        print "Updated bug submit status.<BR>\n";
    }

    if ($description ne $descriptionold) {
        unless ($description) {
            print "Sorry, I can't delete the description.";
            SendSQL("UNLOCK TABLES");
            PutTrailer($localtrailer);
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

    if ($votesperuser ne $votesperuserold) {
        SendSQL("UPDATE products
                 SET votesperuser=$votesperuser
                 WHERE product=" . SqlQuote($productold));
        print "Updated votes per user.<BR>\n";
        $checkvotes = 1;
    }


    if ($maxvotesperbug ne $maxvotesperbugold) {
        SendSQL("UPDATE products
                 SET maxvotesperbug=$maxvotesperbug
                 WHERE product=" . SqlQuote($productold));
        print "Updated max votes per bug.<BR>\n";
        $checkvotes = 1;
    }


    if ($votestoconfirm ne $votestoconfirmold) {
        SendSQL("UPDATE products
                 SET votestoconfirm=$votestoconfirm
                 WHERE product=" . SqlQuote($productold));
        print "Updated votes to confirm.<BR>\n";
        $checkvotes = 1;
    }


    if ($defaultmilestone ne $defaultmilestoneold) {
        SendSQL("SELECT value FROM milestones " .
                "WHERE value = " . SqlQuote($defaultmilestone) .
                "  AND product = " . SqlQuote($productold));
        if (!FetchOneColumn()) {
            print "Sorry, the milestone $defaultmilestone must be defined first.";
            SendSQL("UNLOCK TABLES");
            PutTrailer($localtrailer);
            exit;
        }
        SendSQL("UPDATE products " .
                "SET defaultmilestone = " . SqlQuote($defaultmilestone) .
                "WHERE product=" . SqlQuote($productold));
        print "Updated default milestone.<BR>\n";
    }

    my $qp = SqlQuote($product);
    my $qpold = SqlQuote($productold);

    if ($product ne $productold) {
        unless ($product) {
            print "Sorry, I can't delete the product name.";
            SendSQL("UNLOCK TABLES");
            PutTrailer($localtrailer);
            exit;
        }
        if (TestProduct($product)) {
            print "Sorry, product name '$product' is already in use.";
            SendSQL("UNLOCK TABLES");
            PutTrailer($localtrailer);
            exit;
        }

        SendSQL("UPDATE bugs SET product=$qp, delta_ts=delta_ts WHERE product=$qpold");
        SendSQL("UPDATE components SET program=$qp WHERE program=$qpold");
        SendSQL("UPDATE products SET product=$qp WHERE product=$qpold");
        SendSQL("UPDATE versions SET program=$qp WHERE program=$qpold");
        SendSQL("UPDATE milestones SET product=$qp WHERE product=$qpold");
        
        print "Updated product name.<BR>\n";
    }

    # Update group permissions for this product
    my %newgroups = ();
    foreach (keys %::FORM) {
        next unless /^group_/;
        detaint_natural($::FORM{$_});
        $newgroups{$::FORM{$_}} = 1;
    }

    my %groupsbelong = ();
    SendSQL("select groups.group_id from groups, user_group_map " .
            "where groups.group_id = user_group_map.group_id " .
            "and groups.isbuggroup = 1 " .
            "and user_group_map.user_id = $userid");
    while (my ($groupid) = FetchSQLData()) {
        $groupsbelong{$groupid} = 1;
    }

    my %oldgroups = ();
    SendSQL("select groups.group_id from groups, product_group_map " .
            "where groups.group_id = product_group_map.group_id " .
            "and product_group_map.product_id = $productid");
    while (my ($groupid) = FetchSQLData()) {
        $oldgroups{$groupid} = 1;
    }

    foreach my $groupid (keys %groupsbelong) {
        if (!$oldgroups{$groupid} && $newgroups{$groupid}) {
            SendSQL("INSERT INTO product_group_map (product_id, group_id) VALUES ($productid, $groupid)");
        }
        if ($oldgroups{$groupid} && !$newgroups{$groupid}) {
            SendSQL("DELETE FROM product_group_map WHERE product_id = $productid AND group_id = $groupid");
        }
    }
    print "Updated product permissions<br>\n";

    unlink "data/versioncache";
    SendSQL("UNLOCK TABLES");

    if ($checkvotes) {
        print "Checking existing votes in this product for anybody who now has too many votes.";
        if ($maxvotesperbug < $votesperuser) {
            SendSQL("SELECT votes.who, votes.bug_id " .
                    "FROM votes, bugs " .
                    "WHERE bugs.bug_id = votes.bug_id " .
                    " AND bugs.product = $qp " .
                    " AND votes.count > $maxvotesperbug");
            my @list;
            while (MoreSQLData()) {
                my ($who, $id) = (FetchSQLData());
                push(@list, [$who, $id]);
            }
            foreach my $ref (@list) {
                my ($who, $id) = (@$ref);
                RemoveVotes($id, $who, "The rules for voting on this product has changed;\nyou had too many votes for a single bug.");
                my $name = DBID_to_name($who);
                print qq{<br>Removed votes for bug <A HREF="show_bug.cgi?id=$id">$id</A> from $name\n};
            }
        }
        SendSQL("SELECT votes.who, votes.count FROM votes, bugs " .
                "WHERE bugs.bug_id = votes.bug_id " .
                " AND bugs.product = $qp");
        my %counts;
        while (MoreSQLData()) {
            my ($who, $count) = (FetchSQLData());
            if (!defined $counts{$who}) {
                $counts{$who} = $count;
            } else {
                $counts{$who} += $count;
            }
        }
        foreach my $who (keys(%counts)) {
            if ($counts{$who} > $votesperuser) {
                SendSQL("SELECT votes.bug_id FROM votes, bugs " .
                        "WHERE bugs.bug_id = votes.bug_id " .
                        " AND bugs.product = $qp " .
                        " AND votes.who = $who");
                while (MoreSQLData()) {
                    my $id = FetchSQLData();
                    RemoveVotes($id, $who,
                                "The rules for voting on this product has changed; you had too many\ntotal votes, so all votes have been removed.");
                    my $name = DBID_to_name($who);
                    print qq{<br>Removed votes for bug <A HREF="show_bug.cgi?id=$id">$id</A> from $name\n};
                }
            }
        }
        SendSQL("SELECT bug_id FROM bugs " .
                "WHERE product = $qp " .
                "  AND bug_status = '$::unconfirmedstate' " .
                "  AND votes >= $votestoconfirm");
        my @list;
        while (MoreSQLData()) {
            push(@list, FetchOneColumn());
        }
        foreach my $id (@list) {
            SendSQL("SELECT who FROM votes WHERE bug_id = $id");
            my $who = FetchOneColumn();
            CheckIfVotedConfirmed($id, $who);
        }

    }

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
