#!/usr/bin/perl -w
# 
# news.cgi
# 
#	Contributor(s) David Lawrence <dkl@redhat.com>

require 'CGI.pl';
require 'globals.pl';

print "Content-type: text/html\n\n";
PutHeader("Bugzilla News");


# subroutine: 	PutStory
# description:	outputs story in table html format
# params:		$headline = headline of news article (scalar)
#				$date = date news article was created (scalar)
#				$story = actual large text of the story (scalar)
# returns:		none

sub PutStory {
    my ($add_date, $headline, $story) = (@_);
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
    $$story
    </TD>
</TR>
</TABLE>
<P>
};

}


if (defined ($::FORM{'id'}) && $::FORM{'id'} ne '') {
	# Show an individual news article
	my $query = "";
	if ($::driver eq 'mysql') {
		$query = "select add_date, headline, story from news where id = " . $::FORM{'id'};
	} else {
		$query = "select TO_CHAR(add_date, 'YYYY-MM-DD HH:MI:SS'), " .
				 "headline, story from news " .
				 "where id = " . $::FORM{'id'}; 	
	}
	SendSQL($query);
	my ($add_date, $headline, $story) = FetchSQLData();
	PutStory($add_date, $headline, \$story);

} else {
	# Show all the news
	print "<CENTER><H2>All the News...</H2></CENTER>\n";
	my $query = "";
	if ($::driver eq 'mysql') {
		$query = "select id, add_date, headline, story from news order by id";
	} else {
		$query = "select id, TO_CHAR(add_date, 'YYYY-MM-DD HH:MI:SS'), " .
				 "headline, story from news order by id";
	}
	SendSQL($query);
	while (my @row = FetchSQLData()) {
		my ($id, $add_date, $headline, $story) = (@row);
		PutStory($add_date, $headline, \$story);
	}
}

PutFooter();

exit;

