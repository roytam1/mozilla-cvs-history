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
# The Original Code is the Bugzilla Bug Tracking System.
# 
# The Initial Developer of the Original Code is Netscape Communications
# Corporation. Portions created by Netscape are Copyright (C) 1998
# Netscape Communications Corporation. All Rights Reserved.
# 
# Contributor(s): Terry Weissman <terry@mozilla.org>
# 				  David Lawrence <dkl@redhat.com>

use diagnostics;
use strict;
use XML::Generator;
use XML::Parser;
use CGI::XMLForm;
use Net::FTP;

require 'CGI.pl';
require 'globals.pl';

############### Global variables and other stuff
use vars qw{$cgi};
my $xml_dir = '/home/httpd/html/bugzilla/data/errata_xml/';

# HACK: This should be done automatically
my @final_releases = ('4.2', '5.2', '6.1'); 

# update server information
my $server = "wallace.redhat.com";
my $login = "anonymous";
my $password = "bugzilla\@redhat.com";
my $tmp_dir = "/tmp/errata";

# todays date
my ($day, $mon, $year) = (localtime)[3,4,5];
$year = $year+1900;
$mon = $mon+1;

umask 0000;

############### Start Security
confirm_login();
if (!UserInGroup("errata")) {
	print "<P><FONT COLOR=red><B>Sorry: </B></FONT>\n";
	print "<B>You are not a member of the 'Errata' group.</B>\n";
	exit;
}
############### End Security

GetVersionTable();

############### Start XML/HTML subroutines

################################################################################
# 
# Subroutine:  gen_xml
# 
# Description: Creates a xml file with errata information contained within 
# 
# Exceptions:  NONE
#
# Parameters:   
#
# Return Value: exits upon successful completetion 

sub gen_xml {
	my $update = $cgi->param('update');
	$cgi->delete('update');
	$cgi->delete('action');

	my $filename = $cgi->param("/errata/metadata/id") . ".xml";

	open(OUT, ">$xml_dir/$filename") or die "Could not open $filename for writing: $!";
	if ($cgi->param()) {
		print OUT $cgi->toXML;
	}
	close(OUT);
	print "<A HREF=\"createerrata.cgi?load_file=$filename&action=Update+Errata&update=1\">$filename</A> saved!<P>\n";
	insert_db($cgi->param('/errata/metadata/type'),
			  $cgi->param('/errata/metadata/issued_on'),
			  $cgi->param('/errata/metadata/updated_on'),
			  $cgi->param('/errata/metadata/id'));
}

################################################################################
# 
# Subroutine:  load_xml 
# 
# Description: load in xml file for editing and resubmitting
# 
# Exceptions:  NONE
#
# Parameters:  advisory string 
#
# Return Value: NONE

sub load_xml {
	my $filename = $::FORM{'load_file'};

	my %form_vars    = ("/errata/metadata/type" => "type",
				   		"/errata/metadata/issued_on" => "issued_on",
						"/errata/metadata/updated_on" => "updated_on",
						"/errata/metadata/id" => "advisory_id",
						"/errata/metadata/component" => "component",
						"/errata/metadata/synopsis" => "synopsis",
						"/errata/metadata/keywords" => "keywords",
						"/errata/metadata/cross_ref" => "cross_ref",
						"/errata/content/obsoleted" => "obsoleted",
						"/errata/content/conflicts" => "conflicts",
						"/errata/content/topic" => "topic",
						"/errata/content/description" => "description",
						"/errata/content/solution" => "solution",
						"/errata/content/id_fixed" => "id_fixed",
						"/errata/content/references" => "references",
						"/errata/content/release_arch"	=> "release_arch",
						"/errata/content/rpms" => "packages" );
	my @queries = keys %form_vars;

#### 
#   foreach my $query (@queries) {
#        print "$query <BR>\n"; 
#   }
#	print "<P>\n";
	
	open(inXML, "<$xml_dir/$filename") or die "Could not open $filename for reading: $!";
    my @results = $cgi->readXML(*inXML, @queries);
	close(inXML);

####	
#	foreach my $result (@results) {
#		print "$result <BR>\n";
#	}	
#	print "<P>\n";
	
	my %hash;
	my $count = 0;
	while($count <= $#results) {
		my $temp = $results[$count];
		$count = $count + 1;
		$hash{$temp} = $results[$count];
		$count = $count + 1;
	}
	
	foreach my $k (keys %hash) {
		if (!defined($hash{$k})) { 
			next; 
		}
		$::FORM{$form_vars{$k}} = $hash{$k};
	}

	# make updated_on today's date
	$::FORM{'updated_on'} = sprintf("%s-%s-%s", $year, $mon, $day);

	# fix the advisory_id
	my @temp = split(':', $::FORM{'advisory_id'});
	$::FORM{'advisory_id'} = $temp[1];

#	if ($::FORM{'update'} eq "1") {
#		@temp = split('-', $::FORM{'advisory_id'});
#		$temp[1] = sprintf("%.2d", $temp[1]+1);
#		$::FORM{'advisory_id'} = "$temp[0]-$temp[1]";
#	}
	
	# fix the release and arch form stuff
	my @form_items;
	foreach my $form (split ("\n", $::FORM{'release_arch'})) {
		my @form_items = split (' ', $form);
		$::FORM{"rel_arch-$form_items[0]-$form_items[1]"} = "$form_items[0]-$form_items[1]";
	}

####
#	foreach my $i (keys %::FORM) {
#		print "$i = $::FORM{$i}<BR>\n";
#	}

	# call form generation and exit
	gen_form();
	PutFooter();
	exit;
}

################################################################################
# 
# Subroutine:  insert_db 
# 
# Description: insert new errata identifier into the database 
# 
# Exceptions:  NONE
#
# Parameters:  global advisory string parameters 
#
# Return Value: 

sub insert_db {
	my $type = shift;
	my $issue = shift;
	my $updated = shift;
	my $id = shift;
	my ($next_id, $rev) = ("", "");
	my @temp;

	if ($type eq "Security Advisory") {
        $type = 'RHSA';
    }
    if ($type eq "Bug Fix Advisory") {
        $type = 'RHBA';
    }
    if ($type eq "Product Enhancement Advisory") {
        $type = 'RHEA'; 
    }

	@temp = split(':', $id);
	($next_id, $rev) = split('-', $temp[1]);

	my $query ="";
	if ($::FORM{'update'} eq "1") { 
		$query = "update errata set revision = $rev" .
				 ", updated_on = " . SqlQuote($updated) . 
				 " where id = $next_id";
		
	} else {
		$query = "insert into errata (id, revision, issue_date, type) " . 
					" values ($next_id, $rev, " . SqlQuote($issue) .
            		", " . SqlQuote($type) . ")";
	}
	SendSQL($query);
    print "Database updated.\n";
}

################################################################################
# 
# Subroutine:   gen_advisory_string
# 
# Description:  Generate an unique advisory identifier
# 
# Exceptions:  NONE
#
# Parameters:  
#
# Return Value:  Returns an unique advisory identifier string 

sub gen_advisory_string {
	if ($::FORM{'type'} eq "Security Advisory") {
		return "RHSA-$year:" . $::FORM{'advisory_id'};
	}
	if ($::FORM{'type'} eq "Bug Fix Advisory") {
		return "RHBA-$year:" . $::FORM{'advisory_id'};
	}
	if ($::FORM{'type'} eq "Product Enhancement Advisory") {
		return "RHEA-$year:" . $::FORM{'advisory_id'};
	}
}

################################################################################
# 
# Subroutine:   gen_preview
# 
# Description:  Generate a formatted view of how the Errata will look.
# 
# Exceptions:  NONE
#
# Parameters:  
#
# Return Value:  

sub gen_preview {
	check_form();

	# if we are updating then increment the advisory id
    if ($::FORM{'update'} eq "1") {
        my @temp = split('-', $::FORM{'advisory_id'});
		$temp[0] = sprintf("%.3d", $temp[0]);
        $temp[1] = sprintf("%.2d", $temp[1]+1);
        $::FORM{'advisory_id'} = "$temp[0]-$temp[1]";
    }

	my $advisory = gen_advisory_string();
	
    print qq{
<P>
<TABLE WIDTH=800 BORDER=1 CELLPADDING=3 CELLSPACING=0>
<TR BGCOLOR="#ECECEC">
	<TD>
    <PRE>
---------------------------------------------------------------------
                   Red Hat, Inc. $::FORM{'type'}

Synopsis:           $::FORM{'synopsis'}
Advisory ID:        $advisory
Issue date:         $::FORM{'issued_on'}
Updated on:         $::FORM{'updated_on'}
Keywords:           $::FORM{'keywords'}
Cross references:   $::FORM{'cross_ref'}
---------------------------------------------------------------------

1. Topic:

$::FORM{'topic'}

2. Relevant releases/architectures:

};

	my %rel_arch;
	foreach my $rel (grep(/^rel_arch-.*$/, keys %::FORM)) {
		$rel =~ s/^rel_arch-//g;
		my @temp = split('-', $rel);
		push (@{$rel_arch{$temp[0]}}, $temp[1]);
	}

	foreach my $line (sort(keys %rel_arch)) {
		my @reverse = reverse @{$rel_arch{$line}};
		@{$rel_arch{$line}} = @reverse;
		print "Red Hat Linux " . int($line) . ".x - @{$rel_arch{$line}}\n";
	}

	print qq{
3. Problem description:

$::FORM{'description'}

4. Solution:

$::FORM{'solution'}

5. Bug IDs fixed (http://bugzilla.redhat.com/bugzilla for more info):

};

	# Generate summarys for each of the bugs listed
	foreach my $bug (split(' ', $::FORM{'id_fixed'})) {
		SendSQL("select short_desc from bugs where bug_id = $bug");
		while(my $desc = FetchOneColumn()) {
			print "$bug  -  $desc\n";
		}
	}

        print qq{
6. Obsoleted by:

$::FORM{'obsoleted'}

7. Conflicts with:

$::FORM{'conflicts'}

8. RPMs required:
};

	# Generate rpms lists based on release-arch info and rpm names
	my @full_paths;
	my @releases = keys (%rel_arch);
	foreach my $release (sort(@releases)) {
		print "\nRed Hat Linux " . int($release) . ".x:\n";
		my @rpms;
		my @names;
		foreach my $line (split ('\n', $::FORM{'packages'})) {
			chomp($line);
			$line =~ s/\s+//g;
			if ($line eq "") { 
				next; 
			}
			push (@names, $line);
		}
		foreach my $arch (@{$rel_arch{$release}}) {
			print "\n$arch:\n";
			foreach my $name (sort(@names)) {
				my $full_path = "ftp://ftp.redhat.com/redhat/updates/$release/$arch/$name.$arch.rpm";
				print "$full_path\n";
				push (@full_paths, $full_path);
			}
		}
		print "\nSRPMS:\n";
		foreach my $name (@names) {
			my $full_path = "ftp://ftp.redhat.com/redhat/updates/$release/SRPMS/$name.src.rpm";
			print "$full_path\n";
			push (@full_paths, );
		}
	}
 
	print qq{
9. Verification:

MD5 sum                           Package Name
--------------------------------------------------------------------------
};

	# This will eventually print md5sums automatically
	my $md5sum = gen_md5sum(\@full_paths);
	print $md5sum;
	
	print qq{
These packages are GPG signed by Red Hat, Inc. for security.  Our key
is available at:
    http://www.redhat.com/corp/contact.html

You can verify each package with the following command:
    rpm --checksig  <filename>

If you only wish to verify that each package has not been corrupted or
tampered with, examine only the md5sum with the following command:
    rpm --checksig --nogpg <filename>

10. References:

$::FORM{'references'}

    </PRE>
    </TD>
</TR>
</TABLE>
};

	####################### XML Stuff #########################
	# resend all the form elements formatted for xml creation #
	print 	$cgi->startform(-METHOD=>"GET"),
    		$cgi->hidden(-NAME=>"/errata/metadata/type", -VALUE=>$::FORM{'type'}),
			$cgi->hidden(-NAME=>"/errata/metadata/component", -VALUE=>$::FORM{'component'}),
			$cgi->hidden(-NAME=>"/errata/metadata/issued_on", -VALUE=>$::FORM{'issued_on'}),
			$cgi->hidden(-NAME=>"/errata/metadata/updated_on", -VALUE=>$::FORM{'updated_on'}),
			$cgi->hidden(-NAME=>"/errata/metadata/id", -VALUE=>$advisory),
    		$cgi->hidden(-NAME=>"/errata/metadata/synopsis", -VALUE=>$::FORM{'synopsis'}),
    		$cgi->hidden(-NAME=>"/errata/metadata/keywords", -VALUE=>$::FORM{'keywords'}),
    		$cgi->hidden(-NAME=>"/errata/metadata/cross_ref", -VALUE=>$::FORM{'cross_ref'}),
			$cgi->hidden(-NAME=>"/errata/content/id_fixed", -VALUE=>$::FORM{'id_fixed'}),
    		$cgi->hidden(-NAME=>"/errata/content/obsoleted", -VALUE=>$::FORM{'obsoleted'}),
            $cgi->hidden(-NAME=>"/errata/content/conflicts", -VALUE=>$::FORM{'conflicts'}),
    		$cgi->hidden(-NAME=>"/errata/content/topic", -VALUE=>$::FORM{'topic'}),
    		$cgi->hidden(-NAME=>"/errata/content/solution", -VALUE=>$::FORM{'solution'}),
    		$cgi->hidden(-NAME=>"/errata/content/description", -VALUE=>$::FORM{'description'}),
			$cgi->hidden(-NAME=>"/errata/content/references", -VALUE=>$::FORM{'references'});

	# Release per architecture information
	my $rel_arch_string;
	foreach my $line (sort(keys %rel_arch)) {
		foreach my $arch (@{$rel_arch{$line}}) {
			$rel_arch_string .= "$line $arch\n";
		}
    }
	print $cgi->hidden(-NAME=>"/errata/content/release_arch", -VALUE=>$rel_arch_string);
	
	# If this is an update then pass it along
	if ($::FORM{'update'}) {	
		print $cgi->hidden(-NAME=>'update', -VALUE=>'1');
	}
		
	print	$cgi->hidden(-NAME=>"/errata/content/rpms", -VALUE=>$::FORM{'packages'}),
			$cgi->submit(-NAME=>'action', -VALUE=>'Generate Errata'), "&nbsp;",
			$cgi->a({-HREF=>'createerrata.cgi?action=New+Errata'}, "[Start Over]"),
			$cgi->endform();

	#														  #
	##################### End XML Stuff ####################### 
}

################################################################################
# 
# Subroutine:   gen_form
# 
# Description:  Generate the form for entering in additional errata information
# 
# Exceptions:  None
#
# Parameters:  None
#
# Return Value: None

sub gen_form {
	# lets set some sane defaults
	if (!$::FORM{'type'}) {
		$::FORM{'type'} = "Security Advisory";
	}

	print qq{
<FORM METHOD="GET" ACTION="createerrata.cgi">

<!-- Begin Meta Data -->
<TABLE WIDTH=800 CELLPADDING=3 CELLSPACING=0 BORDER=1>
<TR BGCOLOR="#BFBFBF">
	<TH ALIGN=left><B>Meta Data</B></TH>
</TR><TR BGCOLOR="#ECECEC">
	<TD ALIGN=left>
	<TABLE WIDTH="100%" BORDER=0 CELLSPACING=0 CELLPADDING=3>
	<TR>
    	<TD ALIGN=right><B>Errata Type</B></TD>
    	<TD ALIGN=left>
};
		
		foreach my $i ("Security Advisory", "Bug Fix Advisory", "Product Enhancement Advisory") {
			if ($::FORM{'type'} eq $i) {
            	print qq{\t\t<INPUT TYPE="radio" NAME="type" VALUE="$i" CHECKED>$i<BR>\n};
        	} else {
            	print qq{\t\t<INPUT TYPE="radio" NAME="type" VALUE="$i">$i<BR>\n};
        	}
		}

	print qq{
        </TD>
        <TD ALIGN=right><B>Advisory&nbsp;ID</B></TD>
        <TD ALIGN=left><INPUT TYPE="text" NAME="advisory_id" VALUE="$::FORM{'advisory_id'}" SIZE=20 MAXLENGTH=20><BR>
		(Incremented automatically for updates)</TD>
    </TR><TR>
    	<TD ALIGN=right><B>Synopsis</B></TD>
    	<TD  ALIGN=left COLSPAN="3">
    	<INPUT TYPE="text" NAME="synopsis" VALUE="$::FORM{'synopsis'}" SIZE=60 MAXLENGTH=78>
    	</TD>
	</TR><TR>
    	<TD ALIGN=right><B>Issue date</B></TD>
    	<TD ALIGN=left>
    	<INPUT TYPE="text" NAME="issued_on" VALUE="$::FORM{'issued_on'}" SIZE=15 MAXLENGTH=15>
    	</TD>
    	<TD ALIGN=right><B>Updated On</B></TD>
    	<TD ALIGN=left>
    	<INPUT TYPE="text" NAME="updated_on" VALUE="$::FORM{'updated_on'}" SIZE=15 MAXLENGTH=15>
    	</TD>
	</TR><tr>
    	<TD ALIGN=right><B>Keywords</B></TD>
    	<TD ALIGN=left COLSPAN="3"><INPUT TYPE="text" NAME="keywords" VALUE="$::FORM{'keywords'}" SIZE=60 MAXLENGTH=78></TD>
	</TR><TR>
    	<TD ALIGN=right><B>Cross References</B></TD>
    	<TD ALIGN=left COLSPAN="3"><INPUT TYPE="text" NAME="cross_ref" VALUE="$::FORM{'cross_ref'}" SIZE=60 MAXLENGTH=78></TD>
	</TR>
	</TABLE>
	</TD>
</TR>
</TABLE>
<P>

<!-- Begin Content Data -->
<TABLE WIDTH=800 CELLPADDING=3 CELLSPACING=0 BORDER=1>
<TR BGCOLOR="#BFBFBF">
    <TH ALIGN=left><B>Content Data</B></TH>
</TR><TR BGCOLOR="#ECECEC">
    <TD ALIGN=left>
	<TABLE WIDTH="100%" CELLSPACING=0 CELLPADDING=3>
	<TR>
    	<TD ALIGN=right><B>Topic</B></TD>
    	<TD ALIGN=left COLSPAN=3>
    	<TEXTAREA NAME="topic" ROWS=10 COLS=70 WRAP=hard>$::FORM{'topic'}</TEXTAREA>
    	</TD>
	</TR><TR>	
    	<TD ALIGN=right><B>Bug IDs fixed</B></TD>
    	<TD ALIGN=left COLSPAN="3"><INPUT TYPE="text" NAME="id_fixed" VALUE="$::FORM{'id_fixed'}" SIZE=60></TD>
	</TR><TR>
    	<TD ALIGN=right><B>Releases</B></TD>
    	<TD ALIGN=left>
		<TABLE BORDER=0 CELLPADDING=3 CELLSPACING=2>
		<TR>
			<TH></TH>
			<TH ALIGN=center COLSPAN=4>Architectures</TH>
		</TR><TR>
			<TD ALIGN=center></TD>
			<TD ALIGN=center>i386</TD>
			<TD ALIGN=center>Alpha</TD><TD ALIGN=center>Sparc</TD>
		</TR>
};

	foreach my $i (@final_releases) {
		if ($i eq "src") { next; }
		print "\t\t<TR>\n\t\t\t<TD ALIGN=center WIDTH=30>$i</TD>\n\t\t\t";
		foreach my $j (@::legal_platform) {
			if ($j eq "All") { next; }
			print "<TD ALIGN=center WIDTH=30>";
			if ($::FORM{"release-$i"} and $::FORM{"rep_platform-$j"}) {
				print qq{<INPUT TYPE="checkbox" NAME="rel_arch-$i-$j" VALUE="$i-$j" CHECKED>};
			} elsif ($::FORM{"rel_arch-$i-$j"}) {
				print qq{<INPUT TYPE="checkbox" NAME="rel_arch-$i-$j" VALUE="$i-$j" CHECKED>};
			} else {
				print qq{<INPUT TYPE="checkbox" NAME="rel_arch-$i-$j" VALUE="$i-$j">};
			}
			print "</TD>\n";
		}
		print "</TR>\n";
    }

    print qq{
		</TABLE>
    	</TD>
};
    
	print qq{
	</TR><TR>
    	<TD ALIGN=right><B>Obsoleted&nbsp;by</B></TD>
    	<TD ALIGN=left><INPUT TYPE="text" NAME="obsoleted" SIZE=24 MAXSIZE=40 VALUE="$::FORM{'obsoleted'}"></TD>
    	<TD ALIGN=right><B>Conflicts&nbsp;with</B></TD>
    	<TD ALIGN=left><INPUT TYPE="text" NAME="conflicts" SIZE=24 MAXSIZE=40 VALUE="$::FORM{'conflicts'}"></TD>
	</TR><TR>
    	<TD ALIGN=right><B>Problem Description</B></TD>
    	<TD ALIGN=left COLSPAN=3>
    	<TEXTAREA NAME="description" ROWS=10 COLS=70 WRAP=hard>$::FORM{'description'}</TEXTAREA>
    	</TD>
	</TR><TR>
    	<TD ALIGN=right><B>Solution</B></TD>
    	<TD ALIGN=left COLSPAN=3>
    	<TEXTAREA NAME="solution" ROWS=10 COLS=70 WRAP=hard>$::FORM{'solution'}
    	</TEXTAREA>
    	</TD>
	</TR><TR>
    	<TD ALIGN=right><B>Name-Version-Rev</B><BR>(package links get<BR>automatically generated)</TD>
    	<TD ALIGN=left COLSPAN=3>
    	<TEXTAREA NAME="packages" ROWS=10 COLS=70 WRAP=hard>$::FORM{'packages'}</TEXTAREA>
    	</TD>
	</TR><TR>
		<TD ALIGN=right><B>References</B></TD>
        <TD ALIGN=left COLSPAN=3>
        <TEXTAREA NAME="references" ROWS=10 COLS=70 WRAP=hard>$::FORM{'references'}</TEXTAREA>
	</TABLE>
    </TD>
</TR>
</TABLE>
<P>
<TABLE>
<TR>
	<TD><B><INPUT TYPE="submit" NAME=action VALUE="Preview Errata"></B></TD>
	<TD><B><INPUT TYPE="submit" NAME=action VALUE="Past Errata"></B></TD>
</TR>
</TABLE>
};

	if($::FORM{'update'}) {
		print qq{<INPUT TYPE=hidden NAME=update VALUE=1>};
	}

	print qq{
</FORM>
};

}

################################################################################
# 
# Subroutine:   gen_list
# 
# Description:  Generate a link to past errata and link them for loading
# 
# Exceptions:  None
#
# Parameters:  None
#
# Return Value: None

sub gen_list {
	print qq{
<P>
(Click to edit or view old errata announcement)
<TABLE CELLPADDING=3 CELLSPACING=0 BORDER=1 WIDTH=600>
<TR BGCOLOR="#BFBFBF">
	<TH ALIGN=left>Full Name</TH>
	<TH ALIGN=left>ID/Rev</TH>
	<TH ALIGN=left>Errata Type</TH>
	<TH ALIGN=left>Issue Date</TH>
	<TH ALIGN=left>Updated On</TH>
</TR>
};
	
	my @row;
	SendSQL("select * from errata");
	while (@row = FetchSQLData()) {
		my $id = sprintf("%.3d", $row[0]);
		my $rev = sprintf("%.2d", $row[1]);
		my $errata_type = $row[2];
		my $date = $row[3]; 
		my ($year, $mon, $day) = ($date =~ /(\d+)-(\d+)-(\d+)/);
		print "<TR BGCOLOR=\"#ECECEC\">\n";
		print "<TD ALIGN=left><A HREF=\"createerrata.cgi?action=Update+Errata&update=1&load_file=$errata_type-$year:$id-$rev.xml\">";
		print "$errata_type-$year:$id-$rev</A></TD>\n";
		print "<TD ALIGN=left>$id-$rev</TD>\n";
		print "<TD ALIGN=left>";
		if ($errata_type eq "RHSA") {
			print "Security Advisory";
		} elsif ($errata_type eq "RHBA") {
			print "Bug Fix Advisory";
		} elsif ($errata_type eq "RHEA") {
			print "Product Enhancement Advisory";
		}
		print "</TD>\n";
		print "<TD ALIGN=left>$date</TD>\n";
		print "<TD ALIGN=left>$row[4]&nbsp;</TD>\n";
		print "</TD>\n";
	}

print qq{
</TABLE>
};

}

################################################################################
# 
# Subroutine:  check_form 
# 
# Description:  sanity checking of some form variables
# 
# Exceptions:  None
#
# Parameters:  None
#
# Return Value: None

sub check_form {
    # A little sanity checking
    # Advisory ID #
    if ($::FORM{'advisory_id'} =~ /\s/) {
        print "Advisory ID can not contain spaces!";
        exit;
    }

    # Issue Date #
    if ($::FORM{'issued_on'} !~ m/\d{4}-\d{1,2}-\d{1,2}/) {
        print "Issue Date must be in the form YYYY-MM-DD!";
        exit;
    }

    # Updated On #
	if ($::FORM{'update'} eq "1") {
    	if ($::FORM{'updated_on'} !~ m/\d{4}-\d{1,2}-\d{1,2}/) {
        	print "Updated Date must be in the form YYYY-MM-DD!";
        	exit;
    	}
	}

    # Packages #
#    if ($::FORM{'packages'} !~ m!\S+\s+[^-\s]+-[^-\s]+-\S+!) {
#        print "Package entries must match 'RHL N-R-V md5sum'!";
#        exit;
#    }

	# Topic #
	if ($::FORM{'topic'} eq "") {
		$::FORM{'topic'} = "N-A";
	}

	# Synopsis #
	if ($::FORM{'synopsis'} eq "") {
        $::FORM{'synopsis'} = "N-A";
    }

	# Cross references #
	if ($::FORM{'cross_ref'} eq "") {
        $::FORM{'cross_ref'} = "N-A";
    }

	# Keywords #
	if ($::FORM{'keywords'} eq "") {
        $::FORM{'keywords'} = "N-A";
    }

	# Description #
	if ($::FORM{'description'} eq "") {
        $::FORM{'description'} = "N-A";
    }

	# Obsoleted by #
	if ($::FORM{'obsoleted'} eq "") {
        $::FORM{'obsoleted'} = "N-A";
    }

	# Conflicts with #
	if ($::FORM{'conflicts'} eq "") {
        $::FORM{'conflicts'} = "N-A";
    }

	# References #
	if ($::FORM{'references'} eq "") {
        $::FORM{'references'} = "N-A";
    }
}

################################################################################
# 
# Subroutine:  check_form 
# 
# Description:  sanity checking of some form variables
# 
# Exceptions:  None
#
# Parameters:  None
#
# Return Value: None

sub gen_md5sum {
	my @full_paths = @{ shift; };
	my $md5sums;
	my $local;

#	if (! -d $tmp_dir) {
#		mkdir($tmp_dir);
#	}

	my $ftp = new Net::FTP($server);
	$ftp->login($login, $password);
	$ftp->binary;

	foreach my $path (@full_paths) {
		$path =~ s/ftp:\/\/ftp.redhat.com\///g;
		$local = rindex($path, '/');
    	$local = substr($path, $local+1);
		my $result = $ftp->get($path, "$tmp_dir/$local");
		if ($result) {
			$md5sums .= "*insert md5sum here* $path \n";
		} else {
			$md5sums .= "*insert md5sum here* $path \n";
		}
	}

#	foreach my $foo (@full_paths) {
#		$foo =~ s/ftp:\/\/ftp.redhat.com\/redhat/updates\///g;
#		$md5sums .= "*insert md5sum here* $foo\n";	
#	}

	$ftp->quit;
	
	return $md5sums;
}

############## Start Main 

print "Content-type: text/html\n\n";

$cgi = new CGI::XMLForm;

# Uncomment the next line for debugging purposes
# print $cgi->dump;

if ($::FORM{'action'} eq "Generate Errata") {
	PutHeader("Generate Errata");
	$cgi->delete('action');
	gen_xml();

} elsif ($::FORM{'action'} eq "Preview Errata") {
	PutHeader("Preview Errata");
	delete $::FORM{'action'};
	gen_preview();

} elsif ($::FORM{'action'} eq "Past Errata") {
	PutHeader("Past Errata");
	gen_list();

} elsif ($::FORM{'action'} eq "Update Errata") {
	PutHeader("Update Errata");
	delete $::FORM{'action'};
	load_xml();
	gen_form();

} elsif ($::FORM{'action'} eq "New Errata") {
	PutHeader("New Errata");
	delete $::FORM{'action'};

	# select the next id information from database
	SendSQL("select max(id) from errata");
	my $next_id = FetchOneColumn() + 1;
	my $rev = 1;
	$next_id = sprintf("%.3d", $next_id);
	$rev = sprintf("%.2d", $rev);
	$::FORM{'advisory_id'} = "$next_id-$rev";
	
	# issue date for new errata's is todays date
	$::FORM{'issued_on'} = sprintf("%s-%s-%s", $year, $mon, $day);
	$::FORM{'updated_on'} = sprintf("%s-%s-%s", $year, $mon, $day);

    gen_form();

} else {
	PutHeader("Illegal Errata Action");
}

PutFooter();
exit;

############### End Main 
