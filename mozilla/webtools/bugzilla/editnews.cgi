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
# David Lawrence <dkl@redhat.com>

use diagnostics;
use strict;

require "CGI.pl";
require "globals.pl";

#
# Displays the form to edit news articles 
#
sub EmitFormElements {
    my ($id, $add_date, $headline, $story) = (@_);

	print qq{
	<TH ALIGN="right">Headline</TH>
	<TD><INPUT SIZE=64 MAXLENGTH=64 NAME=headline VALUE="$headline"></TD>
</TR><TR>
	<TH ALIGN="right">Story</TH>
	<TD><TEXTAREA ROWS=4 COLS=64 WRAP=VIRTUAL NAME=story>$$story</TEXTAREA></TD>
</TR>
};

}


#
# Displays a text like "a.", "a or b.", "a, b or c.", "a, b, c or d."
#
sub PutTrailer {
    my (@links) = ("Back to the <A HREF=\"query.cgi\">query page</A>", @_);

    my $count = $#links;
    my $num = 0;
    print "<P>\n";
	print "<CENTER>";
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

unless (UserInGroup("addnews")) {
    PutHeader("Not allowed");
    print "Sorry, you aren't a member of the 'addnews' group.\n";
    print "And so, you aren't allowed to add, modify or delete news items.\n";
    PutTrailer();
    exit;
}


#
# often used variables
#
my $id = trim($::FORM{'id'} || '');
my $action  = trim($::FORM{'action'}  || '');
my $localtrailer = "<A HREF=\"editnews.cgi\">edit</A> more news items";


#
# action='' -> Show nice list of groups 
#
unless ($action) {
    PutHeader("Select News Item");

	if ($::driver eq 'mysql') {
		SendSQL("SELECT id, DATE_FORMAT(add_date, '\%b \%d, \%Y \%l:\%i'), headline " .
                "FROM news " .
                "ORDER BY id");
	} else {
    	SendSQL("SELECT id, TO_CHAR(add_date, 'MON DD, YYYY HH:MI'), headline, story " . 
        	    "FROM news " .
             	"ORDER BY id");
	}

    print "<P>\n<TABLE BORDER=1 CELLPADDING=4 CELLSPACING=0 ALIGN=center><TR BGCOLOR=\"#BFBFBF\">\n";
    print "  <TH ALIGN=left>ID</TH>\n";
	print "  <TH ALIGN=left>Date</TH>\n";
    print "  <TH ALIGN=left>Headline</TH>\n";
    print "  <TH ALIGN=left>Action</TH>\n";
    print "</TR>";
    while ( MoreSQLData() ) {
        my ($id, $add_date, $headline) = FetchSQLData();
        print "<TR BGCOLOR=\"#ECECEC\">\n";
        print "  <TD ALIGN=left><A HREF=\"editnews.cgi?id=$id&action=edit\">$id</A></TD>\n";
        print "  <TD ALIGN=left>$add_date</TD>\n";
        print "  <TD ALIGN=left>$headline</TD>\n";
        print "  <TD ALIGN=left><A HREF=\"editnews.cgi?id=$id&action=del\">Delete</A></TD>\n";
        print "</TR>";
    }
    print "<TR BGCOLOR=\"#ECECEC\">\n";
    print "  <TH ALIGN=left COLSPAN=3>Add a new item</TH>\n";
    print "  <TD ALIGN=left><A HREF=\"editnews.cgi?action=add\">Add</A></TD>\n";
    print "</TR></TABLE>\n";

    PutTrailer();
    PutFooter();
	exit;
}


#
# action='add' -> present form for parameters for new item
#
# (next action will be 'new')
#
if ($action eq 'add') {
    PutHeader("Add News Item");

    #print "This page lets you add a news item to bugzilla.\n";
    print "<FORM METHOD=POST ACTION=editnews.cgi>\n";
    print "<TABLE BORDER=0 CELLPADDING=4 CELLSPACING=0 ALIGN=center><TR>\n";

    EmitFormElements('', '', '<Place headline here>', \'<Place story here>');

    print "</TR>\n</TABLE>\n";
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
# action='new' -> add news item  entered in the 'action=add' screen
#
if ($action eq 'new') {
    PutHeader("Adding News Item");
	my $headline  = trim($::FORM{'headline'}  || '');
	my $story = trim($::FORM{'story'} || '');

    # Cleanups and valididy checks

    unless ($headline) {
        print "You must enter a headline for the news item. Please press\n";
        print "<b>Back</b> and try again.\n";
        PutTrailer($localtrailer);
        exit;
    }

	SendSQL("select id from news where headline = " . SqlQuote($headline));
	my $result = FetchOneColumn();
	if ($result) {
        print "The news item '$headline' already exists. Please press\n";
        print "<b>Back</b> and try again.\n";
        PutTrailer($localtrailer);
        exit;
    }

    
    # Add the new group.
	if ($::driver eq 'mysql') {
		SendSQL("INSERT INTO news ( " .
          		"add_date, headline, story" .
          		") VALUES ( " .
          		"now(), " .
          		SqlQuote($headline) . "," .
          		SqlQuote($story) . ")");
	} else {
    	SendSQL("INSERT INTO news ( " .
          		"id, add_date, headline, story" .
          		") VALUES ( " .
		  		"news_seq.nextval, sysdate, " .
          		SqlQuote($headline) . "," .
          		SqlQuote($story) . ")");
	}

    print "<CENTER>OK, done.</CENTER><P>\n";
    PutTrailer($localtrailer);
    exit;
}


#
# action='del' -> ask if user really wants to delete
#
# (next action would be 'delete')
#
if ($action eq 'del') {
    PutHeader("Delete News Item");

	# display some data about the news item
	if ($::driver eq 'mysql') { 
		SendSQL("SELECT id, DATE_FORMAT(add_date, '\%b \%d, \%Y \%l:\%i'), headline, story " .
            	"FROM news " .
            	"WHERE id = " . $::FORM{'id'});
	} else {
		SendSQL("SELECT id, TO_CHAR(add_date, 'MON DD, YYYY HH:MI'), headline, story " .
                "FROM news " .
                "WHERE id = " . $::FORM{'id'});
	}

    my ($id, $add_date, $headline, $story) = FetchSQLData();
	 
    print qq{
<P>
<TABLE ALIGN=center WIDTH=700 BORDER=1 CELLSPACING=0 CELLPADDING=3>
<TR BGCOLOR="#BFBFBF">
    <TD ALIGN=left>
    <FONT SIZE=+2><B>$headline</B></FONT><BR>
    <FONT SIZE=-1>Added on</FONT> <I>$add_date</I>
    </TD>
</TR><TR BGCOLOR="#ECECEC">
    <TD ALIGN=left>
    $story
    </TD>
</TR>
</TABLE>
<P>
};

    print "<CENTER><H2>Confirmation</H2>\n";
    print "</TD>\n</TR></TABLE>";
    print "<P>Do you really want to delete this news item?<P>\n";
    print "<FORM METHOD=POST ACTION=editnews.cgi>\n";
    print "<INPUT TYPE=SUBMIT VALUE=\"Yes, delete\">\n";
    print "<INPUT TYPE=HIDDEN NAME=action VALUE=delete>\n";
    print "<INPUT TYPE=HIDDEN NAME=id VALUE=$id>\n";
    print "</FORM></CENTER>";

    PutTrailer($localtrailer);
	PutFooter();
    exit;
}


#
# action='delete' -> really delete the news item 
#
if ($action eq 'delete') {
    PutHeader("Deleting News Item");

    SendSQL("DELETE FROM news " . 
            "WHERE id = " . $::FORM{'id'});
    print "<CENTER>Item deleted.<BR></CENTER>\n";

    PutTrailer($localtrailer);
    PutFooter();
	exit;
}



#
# action='edit' -> present the edit news item
#
# (next action would be 'update')
#
if ($action eq 'edit') {
    PutHeader("Edit News Item");

    # get data of group 
    SendSQL("SELECT id, add_date, headline, story " .
            "FROM news " .
            "WHERE id = " . $::FORM{'id'});
    my ($id, $add_date, $headline, $story) = FetchSQLData();

    print "<FORM METHOD=POST ACTION=editnews.cgi>\n";
    print "<TABLE BORDER=0 CELLPADDING=4 CELLSPACING=0 ALIGN=center><TR>\n";

    EmitFormElements($id, $add_date, $headline, \$story);
    
    print "</TR>\n";
    print "</TABLE>\n";

	print "<INPUT TYPE=hidden NAME=id VALUE=$id>\n";
    print "<INPUT TYPE=hidden NAME=headlineold VALUE=\"$headline\">\n";
    print "<INPUT TYPE=hidden NAME=storyold VALUE=\"$story\">\n";
    print "<INPUT TYPE=hidden NAME=action VALUE=update>\n";
    print "<CENTER><INPUT TYPE=submit VALUE=\"Update\"></CENTER>\n";
    print "</FORM>";

    my $x = $localtrailer;
    $x =~ s/more/other/;
    PutTrailer($x);
	PutFooter();
    exit;
}


#
# action='update' -> update the news item
#
if ($action eq 'update') {
    PutHeader("Update News Item");

	my $id 				= trim($::FORM{'id'});
    my $headlineold 	= trim($::FORM{'headlineold'}      	|| '');
    my $storyold	    = trim($::FORM{'storyold'}     		|| '');
    my $headline	  	= trim($::FORM{'headline'}  		|| '');
    my $story	 		= trim($::FORM{'story'}     		|| '');

    if ($headline ne $headlineold) {
        unless ($headline ne "") {
            print "Sorry, I can't delete the headline.";
            PutTrailer($localtrailer);
            exit;
        }
        SendSQL("UPDATE news " . 
                "SET headline = " . SqlQuote($headline) . 
                " WHERE id = $id");
        print "Updated headline.<BR>\n";
    }

    if ($story ne $storyold) {
        unless ($story ne "") {
            print "Sorry, I can't delete the story.";
            PutTrailer($localtrailer);
            exit;
        }

        SendSQL("UPDATE news " . 
                "SET story = " . SqlQuote($story) . 
                " WHERE id = $id");
        print "Updated story.<BR>\n";
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
