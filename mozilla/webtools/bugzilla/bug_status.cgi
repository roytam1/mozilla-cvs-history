#!/usr/bin/perl -w

use diagnostics;
use strict;
 
require 'CGI.pl';

print "Content-type:text/html\n\n";

PutHeader("Form Help");

print "
<TABLE WIDTH=800 ALIGN=center><TR><TD ALIGN=left>
<H1>A Bug's Life Cycle</H1>

The <B>status</B> and <B>resolution</B> field define and track the
life cycle of a bug.

<P>
<TABLE BORDER=1 CELLPADDING=4 CELLSPACING=0 WIDTH=100%>
  <TR ALIGN=center VALIGN=top>
    <TD WIDTH=\"50%\"><H1>STATUS</H1></TD>
    <TD><H1>RESOLUTION</H1></TD>
  </TR>
  <TR VALIGN=top>
    <TD>The <A NAME=\"status\"><B>status</B></A> field indicates the general
        health of a bug.  Only certain status transitions are allowed.</TD>
    <TD>The <A NAME=\"resolution\"><B>resolution</B></A> field indicates what
        happened to this bug.</TD>
  </TR>
  <TR VALIGN=top>
    <TD>
      <DL>
        <DT><A NAME=\"new\"><B>NEW</B></A>
          <DD> This bug has recently been added to the list of bugs and must
               be processed. Bugs in this state may be <B>VERIFIED</B>,
               remain <B>NEW</B>, or resolved and marked <B>RESOLVED</B>.
        <DT><A NAME=\"verified\"><B>VERIFIED</B></A>
          <DD> This bug has been verified as a bug.  Bugs in this state may
               be <B>ASSIGNED</B>, or remain <B>VERIFIED</B>.
        <DT><A NAME=\"assigned\"><B>ASSIGNED</B></A>
          <DD> This bug is not yet resolved, but is assigned to the proper
               person. From here bugs can be given to another person, or
               resolved and become <B>RESOLVED</B>.
        <DT><A NAME=\"reopened\"><B>REOPENED</B></A>
          <DD>This bug was once resolved, but the resolution was deemed
              incorrect.  For example, a <B>WORKSFORME</B> bug is
              <B>REOPENED</B> when more information shows up and the bug is now
              reproducible.  From here bugs are either marked <B>ASSIGNED</B>
              or <B>RESOLVED</B>.
      </DL>
    </TD>
    <TD>
      <DL>
             No resolution yet. All bugs which are <B>NEW</B>,
             <B>VERIFIED</B>, <B>ASSIGNED</B>, or <B>REOPENED</B> have the
             resolution set to blank. All other bugs will be marked with
             one of the following resolutions.
      </DL>
    </TD>
  </TR>
  <TR VALIGN=top>
    <TD>
      <DL>
        <DT><A NAME=\"resolved\"><B>RESOLVED</B></A>
          <DD> A resolution has been found.  From here bugs can be
               <B>REOPENED</B>.
      </DL>
    </TD>
    <TD>
      <DL>
        <DT><A NAME=\"notabug\"><B>NOTABUG</B></A>
          <DD> The problem described is not a bug.
        <DT><A NAME=\"wontfix\"><B>WONTFIX</B></A>
          <DD> The problem described is a bug which will never be fixed.
        <DT><A NAME=\"deferred\"><B>DEFERRED</B></A>
          <DD> The problem described is a bug which will not be fixed in this
               version of the product.
        <DT><A NAME=\"duplicate\"><B>DUPLICATE</B></A>
          <DD> The problem is a duplicate of an existing bug. Marking a bug
               duplicate requires the bug# of the duplicating bug and will at
               least put that bug number in the description field.
        <DT><A NAME=\"worksforme\"><B>WORKSFORME</B></A>
          <DD> All attempts at reproducing this bug were futile, reading the
               code produces no clues as to why this behavior would occur. If
               more information appears later, please re-assign the bug, for
               now, file it.
        <DT><A NAME=\"currentrelease\"><B>CURRENTRELEASE</B></A>
          <DD> The problem described has already been fixed and can be obtained
               in the latest version of our product.
        <DT><A NAME=\"rawhide\"><B>RAWHIDE</B></A>
          <DD> The problem describe has been fixed in the latest development
               release of our product obtainable from our ftp site.
        <DT><A NAME=\"errata\"><B>ERRATA</B></A>
          <DD> The problem described has been fixed and will be available as
               an errata update from our support web site. Please check the site
               to see if it is currently available for download.
      </DL>
    </TD>
  </TR>
</TABLE>

<H1>Other Fields</H1>

<TABLE BORDER=1 CELLPADDING=4 CELLSPACING=0 WIDTH=100%>
  <TR>
    <TH ALIGN=center VALIGN=center><A NAME=\"severity\">
      <H2>Severity</H2></A>
    </TH>
    <TH ALIGN=center VALIGN=center><a name=\"priority\">
      <h2>Priority</h2></a>
    </TH>
  </TR>
  <TR>
    <TD ALIGN=center VALIGN=center>This field describes the impact of a bug.</TD>
    <TD ALIGN=center VALIGN=center>This field describes the importance and order in which
        a bug should be fixed.  The available priorities are:
    </TD>
  </TR>
  <TR>
    <TD VALIGN=top>
	<TABLE CELLPADDING=4>
	<TR>
		<TD><B>Security</B></TD>
    	<TD>security issue</TD>
	</TR><TR>
		<TD><B>High</B></TD>
    	<TD>crashes, loss of data, severe memory leak</TD>
	</TR><TR>
		<TD><B>Low</B></TD>
    	<TD>minor loss of function, or other problem where
        easy workaround is present</TD>
	</TR><TR>
    	<TD><B>Enhancement</B></TD>
    	<TD>some feature or change you would like to see in any future releases of the product</TD>
	</TR>
	</TABLE>
	</TD>
  	<TD VALIGN=top>
	<TABLE CELLPADDING=4>
	<TR>
		<TD><B>High</B></TD>
	    <TD>Most important</TD>
	</TR><TR>
	    <TD><B>Normal</B></TD>
	    <TD>Average Importance</TD>
	</TR><TR>
	    <TD><B>Low</B></TD>
	    <TD>Least important</TD>
  	</TR>
	</TABLE>
	</TD>
  </TR>
</TABLE>
	
<A NAME=\"rep_platform\"><H2>Platform</H2></a>
This is the platform against which the bug was reported.
Legal platforms include:

<UL>
  <LI> All (happens on all platforms; cross-platform bug)
  <LI> i386
  <LI> sparc
  <LI> alpha
</UL>

<B>Note:</B> Selecting the option \"All\" does not select bugs assigned
against all platforms.  It merely selects bugs that <B>occur</B> on
all platforms.

<A NAME=\"assigned_to\"><H2>Assigned To</H2></a>

This is the person in charge of resolving the bug.

<A NAME=\"reporter\"><H2>Reporter</H2></a>

This is the person who reported the bug.

<p>

The default status for queries is set to NEW, ASSIGNED and REOPENED. When
searching for bugs that have been resolved or verified, remember to set the
status field appropriately.

<P>
<A NAME=\"description\"><H2>Description and Additional Comments</H2></a>
Markup tags entered in these fields will appear as plain text. Numbers preceded by the word
'bug' will be converted to hyperlinks<BR>pointing to the proper bug reports. Any email addresses
will be automatically converted to mailto: hyperlinks. Also any urls starting<BR>with http://
will be made automatically into hyperlinks.
</TD></TR></TABLE>
";

PutFooter();


