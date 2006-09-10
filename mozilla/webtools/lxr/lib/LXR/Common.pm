# $Id$

package LXR::Common;

use DB_File;
use lib '../..';
use Local;

require Exporter;
@ISA = qw(Exporter);
@EXPORT = qw($Path &warning &fatal &abortall &fflush &urlargs 
	     &fileref &idref &htmlquote &freetextmarkup &markupfile
	     &markspecials &htmlquote &statustextmarkup &markupstring
	     &init &glimpse_init &makeheader &makefooter &expandtemplate);


$wwwdebug = 1;

$SIG{__WARN__} = 'warning';
$SIG{__DIE__}  = 'fatal';


@term = (
  'atom',	'\\\\.',	'',
  'comment',	'/\*',		'\*/',
  'comment',	'//',		'[\r\n]',
  'string',	'"',		'"',
  'string',	"'",		"'",
  'verb',	'\\b(?:for|do|while|else|if|throw|return)\\b',	'[\s(]',
  'verb',	'\\b(?:true|false|void|unsigned|int|double|float|short|long|bool|char)\\b',	'[\s();]',
  'verb',	
'^(?:const|static|switch|case|default|break|next|continue|class|struct|union|enum)\b',	
'\s',
  'include',	'#\\s*include\\b',	'[\r\n\b]',
  'include',	'#\\s*import\\b',	'[\r\n]',
  #'include',    '\\s*interface\\b,      ';'
);

my @javaterm = @term;
push @javaterm, (
  'verb',
  '\\b(?:public|protected|private|package|implements|interface|extends|final|import|throws|abstract)\\b',
  '[\s]',
  'verb',       '\\b(?:try|catch|finally)\\b', '[\s{(]',
  'verb',
  '\\b(?:new|delete|instanceof|null)',
  '[\s(]',
);

my @cterm = @term;
push @cterm, (
  'verb',  '\\b(?:typedef)\\b', '[\s]',
  'verb',
'^#\\s*(?:if|(?:ifn?|un)def|else|elif|define|endif|pragma|warn|error)\\b',
'(?:\s|$)',
  'verb',  '\\b(?:sizeof)\\b', '[\s(]',
);

my @cppterm = @cterm;
push @cppterm, (
  'verb',  '\\b(?:template)\\b', '[\s<]',
  'verb',  '\\b(?:inline|extern|explicit|new)\\b', '[\s]',
  'verb',
  '\\b(?:public|protected|private|interface|virtual)\\b',
  '[\s:(]',
  'verb',       '\\b(?:try|catch|finally|operator)\\b', '[\s{(]',
  'verb',
  '\\b(?:new|delete|null)',
  '[\s(]',
);

my @jsterm = @javaterm;
push @jsterm, (
  'verb',  '\\b(?:var|function|[gs]et\s|typeof)\\b', '[\s(]',
);

@pterm = (
  'atom',       '\\\\.',        '',
  'atom',       '(?:[\$\@\&\%\=]?\w+)', '\\W',
  'comment',    '#',            '[\r\n]',
  'comment',    '^=(?:begin|pod|head)', '=cut',
  #'comment',    '^=cut',   '^=back',
  'string',     "'",            "'",
  'string',     '"',            '"',
  'string',     '\\b(?:qq?|m)\|', '\|',
  'string',     '\\b(?:qq?|m)#',  '#',
  'string',     '\\b(?:qq?|m)\(', '\)',
  'string',     '\\b(?:qq?|m){',  '}',
  'string',     '\\b(?:qq?|m)<',  '>',
  #'string',     '^/',           '/',
  'verb',       '\\bsub\\b',      '\s',
  'verb',
'^\s*(?:for|foreach|while|else|elsif|if|unless|my|local|shift)\b',
'[ \(\{]',
  'verb',
'^\s*(?:exit|return|break|next|last|package)\\b',
'[ ;\(]',
  'include',    '\\brequire\\b',';',
  'use',        '\\buse\\b',      ';',
);

my @tterm = (
  'atom',       '\\\\.',        '',
  'comment',    '\[\%#',        '\%\]',
  'include',    '\\bPROCESS\\b',      '\%\]|\s$',
  'include',    '\\bINCLUDE\\b',      '\%\]|\s$',
  'use',        '\\bUSE\\b',          '\%\]',
  'string',     '"',            '"',
  'string',     "'",            "'",
);

my %alreadywarned = ();

sub warning {
    return if defined $_[1] && $_[1] && defined $alreadywarned{$_[1]};
    print(STDERR "[",scalar(localtime),"] warning: $_[0]\n");
    print("<h4 align=\"center\"><i>** Warning: $_[0]</i></h4>\n") if $wwwdebug;
    $alreadywarned{$_[1]} = 1;
}


sub fatal {
    print(STDERR "[",scalar(localtime),"] fatal: $_[0]\n");
    print("<h4 align=\"center\"><i>** Fatal: $_[0]</i></h4>\n") if $wwwdebug;
    exit(1);
}


sub abortall {
    print(STDERR "[",scalar(localtime),"] abortall: $_[0]\n");
    print("Content-Type: text/html\n\n",
	  "<html>\n<head>\n<title>Abort</title>\n</head>\n",
	  "<body><h1>Abort!</h1>\n",
	  "<b><i>** Aborting: $_[0]</i></b>\n",
	  "</body>\n</html>\n") if $wwwdebug;
    exit(1);
}


sub fflush {
    $| = 1; print('');
}


sub urlargs {
    my @args = @_;
    my %args = ();
    my $val;

    foreach (@args) {
	$args{$1} = $2 if /(\S+)=(\S*)/;
    }
    @args = ();

    foreach ($Conf->allvariables) {
	$val = $args{$_} || $Conf->variable($_);
	push(@args, "$_=$val") unless ($val eq $Conf->vardefault($_));
	delete($args{$_});
    }

    foreach (keys(%args)) {
	push(@args, "$_=$args{$_}");
    }

    return($#args < 0 ? '' : '?'.join(';',@args));
}    


sub fileref {
    my ($desc, $path, $line, @args) = @_;
$desc =~ s/\n//g;
$path =~ s/\n//g;
    #$path =~ s/\+/ /;

    # jwz: URL-quote any special characters.
    # endico: except plus. plus signs are normally used to represent spaces
    # but here we need to allow plus signs in file names for gtk+
    # hopefully this doesn't break anything else
    $path =~ s|([^-a-zA-Z0-9.+\@/_\r\n])|sprintf("%%%02X", ord($1))|ge;

    return("<a href=\"$Conf->{virtroot}/source$path".
	   &urlargs(@args).
	   ($line > 0 ? "#$line" : "").
	   "\"\>$desc</a>");
}


sub diffref {
    my ($desc, $path, $darg) = @_;

    ($darg,$dval) = $darg =~ /(.*?)=(.*)/;
    return("<a href=\"$Conf->{virtroot}/diff$path".
	   &urlargs(($darg ? "diffvar=$darg" : ""),
		    ($dval ? "diffval=$dval" : ""),
		    @args).
	   "\"\>$desc</a>");
}


sub idref {
    my ($desc, $id, @args) = @_;
    return("<a class='ident' href=\"$Conf->{virtroot}/ident".
	   &urlargs(($id ? "i=$id" : ""),
		    @args).
	   "\"\>$desc</a>");
}


sub atomref {
    my ($atom) = @_;
    return "<span class='atom'>$atom</span>";
}

sub http_wash {
    my $t = shift;
    # $t =~ s/\+/%2B/g;

    $t =~ s/\%2b/\%252b/gi;

    #endico: don't use plus signs to represent spaces as is the normal
    #case. we need to use them in file names for gtk+

    $t =~ s/\%([\da-f][\da-f])/pack("C", hex($1))/gie;

    # Paranoia check. Regexp-searches in Glimpse won't work.
    # if ($t =~ tr/;<>*|\`&$!#()[]{}:\'\"//) {

    # Should be sufficient to keep "open" from doing unexpected stuff.
    if ($t =~ tr/<>|\"\'\`//) {
	&abortall("Illegal characters in HTTP-parameters.");

    }
    
    return($t);
}


sub markspecials {
    $_[0] =~ s/([\&\<\>])/\0$1/g;
}


sub htmlquote {
    $_[0] =~ s/\0&/&amp;/g;
    $_[0] =~ s/\0</&lt;/g;
    $_[0] =~ s/\0>/&gt;/g;
#$_[0] = "<i>TEST</i>".$_[0]."<i>ING</i>" if $_[0] =~ /overview/;
    $_[0] =~ s#\b(href=)("([^"]*)")\b#$1<a href="$3">$2</a>#gi;
}

sub freetextmarkup {
    $_[0] =~ s#((?:ftp|http)://\S*[^\s."')>])#<a href=\"$1\">$1</a>#gi;
    $_[0] =~ s#(&amp;lt;(?:[Mm][Aa][Ii][Ll][Tt][Oo]:|)([^\s"']*?@[^\s"']*?)&amp;gt;)#<a href=\"mailto:$2\">$1</a>#g;
    $_[0] =~ s#(\((?:[Mm][Aa][Ii][Ll][Tt][Oo]:|)([^\s"']*?@[^\s"']*?)\))#<a href=\"mailto:$2\">$1</a>#g;
    $_[0] =~ s#(\0<(?:[Mm][Aa][Ii][Ll][Tt][Oo]:|)([^\s"']*?@[^\s"']*?)\0>)#<a href=\"mailto:$2\">$1</a>#g;
}

sub statustextmarkup {
    return unless $_[0] =~ /\@status/;
    $_[0] =~ s#(\@status\s+)(FROZEN|UNDER_REVIEW|DEPRECATED)\b#<span class="idl_$2">$1$2</span>#gi;
}

sub linetag {
#$frag =~ s/\n/"\n".&linetag($virtp.$fname, $line)/ge;
#    my $tag = '<a href="'.$_[0].'#L'.$_[1].
#	'" name="L'.$_[1].'">'.$_[1].' </a>';
    my $tag;
    $tag = '<span class=line>';
    $tag .= ' ' if $_[1] < 10;
    $tag .= ' ' if $_[1] < 100;
    $tag .= ' ' if $_[1] < 1000;
    $tag .= &fileref($_[1], $_[0], $_[1]).' ';
    $tag .= '</span>';
    $tag =~ s/<a/<a name=$_[1]/;
#    $_[1]++;
    return($tag);
}

# dme: Smaller version of the markupfile function meant for marking up 
# the descriptions in source directory listings.
sub markupstring {
    my ($string, $virtp) = @_;

    # Mark special characters so they don't get processed just yet.
    markspecials($string);

    # Look for identifiers and create links with identifier search query.
    tie (%xref, "DB_File", $Conf->dbdir."/xref", O_RDONLY, 0664, $DB_HASH)
        || &warning("Cannot open xref database.", 'xref-db');
    $string =~ s#(^|\s)([a-zA-Z_~][a-zA-Z0-9_]*)\b#
                $1.(is_linkworthy($2) ? &idref($2,$2) : $2)#ge;
    untie(%xref);

    # HTMLify the special characters we marked earlier,
    # but not the ones in the recently added xref html links.
    $string=~ s/\0&/&amp;/g;
    $string=~ s/\0</&lt;/g;
    $string=~ s/\0>/&gt;/g;

    # HTMLify email addresses and urls.
    $string =~ s#((ftp|http|nntp|snews|news)://(\w|\w\.\w|\~|\-|\/|\#)+(?!\.\b))#<a href=\"$1\">$1</a>#g;
    # htmlify certain addresses which aren't surrounded by <>
    $string =~ s/([\w\-\_]*\@(?:netscape\.com|mozilla\.(?:com|org)|gnome\.org|linux\.no))(?!&gt;)/<a href=\"mailto:$1\">$1<\/a>/g;
    $string =~ s/(&lt;)(.*@.*)(&gt;)/$1<a href=\"mailto:$2\">$2<\/a>$3/g;

    # HTMLify file names, assuming file is in the current directory.
    $string =~ s#\b(([\w-_\/]+\.(c|h|cc|cpp?|idl|java))|README(?:\.(?:txt|html?)|))\b#<a href=\"$Conf->{virtroot}/source$virtp$1\">$1</a>#g;

    return($string);
}

# dme: Return true if string is in the identifier db and it seems like its
# use in the sentence is as an identifier and it isn't just some word that
# happens to have been used as a variable name somewhere. We don't want
# words like "of", "to" and "a" to get links. The string must be long 
# enough, and  either contain "_" or have some letter other than the first 
# which is capitalized
sub is_linkworthy{
    my ($string) = @_;

    if  ( ($string =~ /....../) &&
	  ( ($string =~ /_/) ||
	    ($string =~ /.[A-Z]/)
	  ) &&
	  (defined($xref{$string}))
        ){
	return (1);
    }else{
	return (0);
    }
}

my @file_listing;
my $file_iterator;
my $file_length;

sub getnext_fileentry{
    my ($filematch) = @_;
    unless (defined @file_listing)
    {
        my $indexname = $indexname || $Conf->dbdir."/.glimpse_filenames";
        $sourceroot = $sourceroot || $Conf->sourceroot;

        return undef unless open(FILELLISTING,$indexname);
        $file_length = <FILELLISTING>;
        $file_length =~ s/[\r\n]//g;
        @file_listing = ();
    }
    my $fileentry;
    if ($file_length == scalar @file_listing) {
        while ($file_iterator < $file_length) {
            $fileentry = $file_listing[$file_iterator++];
            if ($fileentry =~ /$filematch/) {
                return $fileentry;
            }
        }
    }
    my $fileentry;
    while (scalar @file_listing < $file_length) {
        $fileentry = <FILELLISTING>;
        chomp $fileentry;
        $fileentry =~ s/^$sourceroot//;
        $fileentry =~ s/\n//;
        push @file_listing, $fileentry;
        if ($fileentry =~ /$filematch/) {
            $file_iterator = scalar @file_listing;
            return $fileentry;
        }
        #return $fileentry;
    }
    $file_iterator = scalar @file_listing;
    if ($file_iterator == $file_length) {
        close FILELLISTING;
        return undef;
    }

    return undef;
}

sub filelookup{
    my ($filename,$bestguess,$prettyname)=@_;
    $prettyname = $filename unless defined $prettyname;
    my $idlfile;
    $idlfile = $1 if ($filename =~ /(^.*)\.h$/);
    return &fileref($prettyname, $bestguess) if -e $Conf->sourceroot.$bestguess;
    my $baseurl = $Conf->{virtroot}; # &baseurl;
    my ($pfile_ref,$gfile_ref,$ifile_ref,$jfile_ref,$kfile_ref,$loosefile,$basefile,$p,$g,$i,$j,$k);
    $filename =~ s|([(){}^\$.*?\&\@\\+])|\\$1|g;
    if ($filename =~ m|/|) {
        $basefile = $loosefile = $filename;
        $basefile =~ s|^.*/|/|g;
        $loosefile =~ s|/|/.*/|g;
    }
    $filename = '/' . $filename . '$';
    $file_iterator = 0;
    while ($fileentry = &getnext_fileentry($idlfile || $filename)) {
        if ($fileentry =~ m|/\Q$bestguess\E$|i) {
	    $pfile_ref=&fileref($prettyname, $fileentry);
	    $p++;
        }
        if ($fileentry =~ m|$filename|i) {
            $gfile_ref=&fileref($prettyname, $fileentry);
            $g++;
        }
        if ($idlfile && $fileentry =~ m|/\Q$idlfile.idl\E$|i) {
            $ifile_ref=&fileref($prettyname, $fileentry);
            $i++;
        }
        if ($loosefile && $fileentry =~ m|$loosefile|i) {
            $jfile_ref=&fileref($prettyname, $fileentry);
            $j++;
        }
        if ($basefile && $fileentry =~ m|$basefile$|i) {
            $kfile_ref=&fileref($prettyname, $fileentry);
            $k++;
        }
	# Short circuiting:
	# If there's more than one idl file then just give a find for all stems
	# If there's an idl file and a header file then just give a find for all stems
	return "<a href='$baseurl/find?string=$idlfile'>$prettyname</a>" if ($p || $g || $i > 1) && $i;
    }
    return $pfile_ref if $p == 1;
    if  ($p == 0) {
        return $gfile_ref if $g == 1;
        if ($g == 0) {
            return $ifile_ref if $i == 1;
            if ($i == 0) {
                return $jfile_ref if $j == 1;
                return $kfile_ref if $j == 0 && $k == 1;
            }
        }
    }
    return "<a href='$baseurl/find?string=$idlfile'>$prettyname</a><!-- i: $p/$g/$i/$j/$k -->" if $i;
    return "<a href='$baseurl/find?string=$filename'>$prettyname</a><!-- f: $p/$g/$i/$j/$k -->" if $p || $g || !$loosefile;
    return "<a href='$baseurl/find?string=$loosefile'>$prettyname</a><!-- l: $p/$g/$i/$j/$k -->" if $j;
    return "<a href='$baseurl/find?string=$basefile'>$prettyname</a><!-- b: $p/$g/$i/$j/$k -->";
}

sub notvcalled {
 my ($entryrev,$entrybranch,$keywords);
 my ($entriespath, $entryname) = split m|/(?!.*/)|, $Path->{'realf'};
 if (open(CVSENTRIES, "$entriespath/CVS/Entries")) {
  while (<CVSENTRIES>) {
   next unless m|^/\Q$entryname\E/([^/]*)/[^/]*/([^/]*)/(.*)|;
   ($entryrev,$keywords,$entrybranch)=($1,$2,$3);
   last;
  }
  close(CVSENTRIES);
 }
 return ($entryrev,$entrybranch,$keywords);
}
sub notycalled {
 my $entrybranch = 'HEAD';
 if (open(CVSTAG, " $Path->{'real'}/CVS/Tag")) {
  while (<CVSTAG>) {
   next unless m|^T(.*)$|;
   $entrybranch=$1;
   last;
  }
  close(CVSTAG);
 }
 return $entrybranch;
}

sub markupfile {
    my ($INFILE, $virtp, $fname, $outfun) = @_;
    my @terms;
    $line = 1;

    # A C/C++ file 
    if ($fname =~ /\.(?:java|idl)(?:.in|)$/i) {
        @terms = @javaterm;
    } elsif ($fname =~ /\.(?:[ch]|cpp?|cc|mm?|pch\+?\+?)(?:.in|)$/i) { # Duplicated in genxref.
        @terms = @cppterm;
    } elsif ($fname =~ /\.(?:js)(?:.in|)$/) {
        @terms = @jsterm;
    } elsif ($fname =~ /\.(?:pl|pm|cgi)$/) {
        @terms = @pterm;
    } elsif ($fname =~ /\.(?:tm?pl)$/) {
        @terms = @tterm;
    } else {
        open HEAD_HANDLE, $fname;
        my $file_head = <HEAD_HANDLE>;
        @terms = @pterm if $file_head =~ /^#!.*perl/;
        close HEAD_HANDLE;
    }
    if (defined @terms) {
	&SimpleParse::init($INFILE, @terms);

	tie (%xref, "DB_File", $Conf->dbdir."/xref", O_RDONLY, 0664, $DB_HASH)
	    || &warning("Cannot open xref database.", 'xref-db');

	&$outfun(# "<pre>\n".
		 #"<a name=\"".$line++.'"></a>');
		 &linetag($virtp.$fname, $line++));

	($btype, $frag) = &SimpleParse::nextfrag;
	
	while (defined($frag)) {
#print "<!--$btype-->";
	    &markspecials($frag);

	    if ($btype eq 'comment') {
		# Comment
		# Convert mail addresses to mailto:
		&freetextmarkup($frag);
		&statustextmarkup($frag);
		$frag = "<span class='comment'>$frag</span>";
		$frag =~ s#\n#</span>\n<span class='comment'>#g;
	    } elsif ($btype eq 'string') {
		# String
		$frag = "<span class='string'>$frag</span>";
		
	    } elsif ($btype eq 'include') { 
		# Include directive
                if ($frag =~ s#\0(<)(.*?)\0(>)#
                    '&lt;'.
                    &filelookup($2, $Conf->mappath($Conf->incprefix.'/'.$2)).
                    '&gt;'#e) {
                } else {
                    my ($inc_head, $inc_file, $inc_tail, $prettyfile);
                        if ($frag =~ s#(\s*[\"\'])(.*?)([\"\'])#
                                ($1)."\0$2\0".($3)#e) {
                            ($inc_head, $inc_file, $inc_tail, $prettyfile) = ($1, $2, $3, $2);
                        } elsif ($frag =~ s#((?:\s*require|)\s+)([^\s;]+)#
                                ($1)."\0$2\0"#e) {
                        ($inc_head, $inc_file, $inc_tail, $prettyfile) = ($1, $2, undef, $2);
                    }
                    unless (length $inc_tail) {
                        $inc_file .= '.pm' if $inc_file =~ s|::|/|g;
                    }
                    $frag =~ s#\0.*?\0#
                        &filelookup($inc_file, $virtp.$inc_file,$prettyfile)#e;
                }
                $frag =~ s/('[^'+]*)\+(.*?')/$1\%2b$2/ while $frag =~ /(?:'[^'+]*)\+(?:.*?')/;
                $frag =~ s|(#?\s*[^\s"'<]+)|<span class='include'>$1</span>|;
            } elsif ($btype eq 'use') {
                # perl use directive
                $frag =~ s#(use|USE)(\s+)([^\s;]*)#<span class='include'>$1</span>$2$3#;
                my $module = $3;
                my $modulefile = "$module.pm";
                $modulefile =~ s|::|/|g; 
                $module = (&filelookup($modulefile, $modulefile, $module));
                $frag =~ s|(</span>\s+)([^\s;]*)|$1$module|;
            } elsif ($btype eq 'verb') {
                $frag =~ s/^/<span class='verb'>/;
                $frag =~ s|$|</span>|;
	    } else {
		# Code
		$frag =~ s#(^|[^a-zA-Z_\#0-9])([a-zA-Z_~][a-zA-Z0-9_]*)\b#
                    $1.(defined($xref{$2}) ? &idref($2,$2) : &atomref($2))#ge;
	    }

	    &htmlquote($frag);
	    $frag =~ s/(?:\r?\n|\r)/"\n".&linetag($virtp.$fname, $line++)/ge;
	    &$outfun($frag);
	    
	    ($btype, $frag) = &SimpleParse::nextfrag;
	}
	    
#	&$outfun("</pre>\n");
	untie(%xref);

    } elsif ($fname =~ /\.(gif|p?jpe?g|xbm|bmp|[jmp]ng)$/i) {

	&$outfun("</PRE>");
	&$outfun("<UL><TABLE><TR><TH VALIGN=MIDDLE><B>Image: </B></TH>");
	&$outfun("<TD VALIGN=MIDDLE>");

	&$outfun("<img src=\"$Conf->{virtroot}/source".$virtp.$fname.
		 &urlargs("raw=1")."\" border=\"0\" alt=\"$fname\">");

	&$outfun("</TR></TD></TABLE></UL><PRE>");

    } elsif ($fname eq 'CREDITS') {
	while (<$INFILE>) {
	    &SimpleParse::untabify($_);
	    &markspecials($_);
	    &htmlquote($_);
            s/^N:\s+(.*)/<strong>$1<\/strong>/gm;
	    s/^(E:\s+)(\S+@\S+)/$1<a href=\"mailto:$2\">$2<\/a>/gm;
	    s/^(W:\s+)(.*)/$1<a href=\"$2\">$2<\/a>/gm;
#	    &$outfun("<a name=\"L$.\"><\/a>".$_);
	    &$outfun(&linetag($virtp.$fname, $.).$_);
	}
    } else {
	my $is_binary = -1;
	my $keywords;
	(undef,undef,$keywords) = &notvcalled;
	READFILE:
	my $first_line = <$INFILE>;

	$_ = $first_line;
	if ($keywords =~ /-kb/) {
	    $is_binary = 1;
	    print "CVS Says this is binary<br>";
	} elsif ( m/^\#!/ ) {			# it's a script
	    $is_binary = 0;
	} elsif ( m/-\*-.*mode:/i ) {		# has an emacs mode spec
	    $is_binary = 0;
	} elsif (length($_) > 132) {		# no linebreaks
	    my $macline = $_;
	    if ($is_binary == -1 && $macline =~ s/\r//g > 5) {	# mac linebreaks?
		seek $INFILE, 0, 0;		# restart at the beginning of the file
		$. = 0;				# reset the line counter to the beginning
		$/ = "\r";			# set the input record to macnewline
		$is_binary = -2;		# make sure not to loop infinitely
		goto READFILE;
	    }
	    $is_binary = 1;
	} elsif ( m/[\000-\010\013\014\016-\037\200-Ÿ]/ ) {	# ctrl or ctrl+
	    $is_binary = 1;
	} else {				# no idea, but assume text.
	    $is_binary = 0;
	}

	if ( $is_binary ) {

	    &$outfun("</PRE>");
	    &$outfun("<UL><B>Binary File: ");

            # jwz: URL-quote any special characters.
            my $uname = $fname;
            $uname =~ s|([^-a-zA-Z0-9.\@/_\r\n])|sprintf("%%%02X", ord($1))|ge;

	    &$outfun("<A HREF=\"$Conf->{virtroot}/source".$virtp.$uname.
		     &urlargs("raw=1")."\">");
	    &$outfun("$fname</A></B>");
	    &$outfun("</UL><PRE>");

	} else {
	    $_ = $first_line;
	    do {
		&SimpleParse::untabify($_);
		&markspecials($_);
		&htmlquote($_);
		&freetextmarkup($_);
#	    &$outfun("<a name=\"L$.\"><\/a>".$_);
		&$outfun(&linetag($virtp.$fname, $.).$_);
	    } while (<$INFILE>);
	}
    }
}


sub fixpaths {
    my $virtf = '/'.shift;
    $Path->{'root'} = $Conf->sourceroot;
    
    while ($virtf =~ s#/[^/]+/\.\./#/#g) {
       }
    $virtf =~ s#/\.\./#/#g;
	   
    $virtf .= '/' if (-d $Path->{'root'}.$virtf);
    $virtf =~ s#//+#/#g;
    
    my ($virt, $file) = $virtf =~ m#^(.*/)([^/]*)$#;
    ($Path->{'virtf'}, $Path->{'virt'}, $Path->{'file'}) = ($virtf, $virt, $file);

    my $real = $Path->{'real'} = $Path->{'root'}.$virt;
    my $realf = $Path->{'realf'} = $Path->{'root'}.$virtf;

    my $svndirprop = $real . ".svn/dir-wcprops";
    if (-f $svndirprop) {
      if (open (SVN, $svndirprop))
      {
        my $svnpath;
        $svnpath = <SVN> while $svnpath !~ /^V \d+$/;
        $svnpath = <SVN>;
        $svnpath =~ m|^/svn/([^/]*)/!svn/ver/\d+/(.*)|;
        my $svntree = $1;
        $svnpath = $2;
        $svnpath =~ s/[\n\r]//g;

        $Path->{'svnvirt'} = $svnpath;
        $Path->{'svntree'} = $svntree;
        close SVN;
      }
    }
    my $svnentries = $real . ".svn/entries";
    if (-f $svnentries) {
      if (open (SVN, $svnentries))
      {
        my $svnrepo;
        while ($svnrepo = <SVN>) {
          if ($svnrepo =~ /repos="(.*)"/) {
            $Path->{'svnrepo'} = $1;
            last;
          }
        }
        close SVN;
      }
    }

    @pathelem = $Path->{'virtf'} =~ /([^\/]+$|[^\/]+\/)/g;
    
    $fpath = '';
    foreach (@pathelem) {
	$fpath .= $_;
	push(@addrelem, $fpath);
    }
    my $fix = '';
    if (defined $Conf->prefix) {
        $fix = $Conf->prefix.'/';
        unshift(@pathelem, $fix);
        unshift(@addrelem, "");
        $fix =~ s#[^/]##g;
        $fix =~ s#/#../#g;
    }
    unshift(@pathelem, $Conf->sourcerootname.'/');
    unshift(@addrelem, $fix);
    
    foreach (1..$#pathelem) {
	if (defined($addrelem[$_])) {

	    # jwz: put a space after each / in the banner so that it's possible
	    # for the pathnames to wrap.  The <WBR> tag ought to do this, but
	    # it is ignored when sizing table cells, so we have to use a real
	    # space.  It's somewhat ugly to have these spaces be visible, but
	    # not as ugly as getting a horizontal scrollbar...
	    #
	    $Path->{'xref'} .= &fileref($pathelem[$_], "/$addrelem[$_]") . " ";
	} else {
	    $Path->{'xref'} .= $pathelem[$_];
	}
    }
    $Path->{'xref'} =~ s#/</a>#</a>/#gi;
}

sub glimpse_init {

    $HTTP->{'this_url'} = join('', 'http://',
					  $ENV{'SERVER_NAME'},
					  ':', $ENV{'SERVER_PORT'},
					  $ENV{'SCRIPT_NAME'},
					  $ENV{'PATH_INFO'},
					  '?', $ENV{'QUERY_STRING'});
    my @a;

    foreach ($ENV{'QUERY_STRING'} =~ /([^;&=]+)(?:=([^;&]+)|)/g) {
	push(@a, $_);
        }
    $HTTP->{'param'} = {@a};
    my $head = init_all();

    if ($ENV{'QUERY_STRING'} =~ s/\&regexp=on//) {
        $Conf->{'regexp'} = 'on';
    } else {
        $ENV{'QUERY_STRING'} =~ s/\&regexp=off//;
        $Conf->{'regexp'} = 'off';
    }

    return($Conf, $HTTP, $Path, $head);
    }


sub init {

    my @a;
    $HTTP->{'this_url'} = &http_wash(join('', 'http://',
					  $ENV{'SERVER_NAME'},
					  ':', $ENV{'SERVER_PORT'},
					  $ENV{'SCRIPT_NAME'},
					  $ENV{'PATH_INFO'},
					  '?', $ENV{'QUERY_STRING'}));
    foreach ($ENV{'QUERY_STRING'} =~ /([^;&=]+)(?:=([^;&]+)|)/g) {
	push(@a, &http_wash($_));
        }
    $HTTP->{'param'} = {@a};
    my $head = init_all();
    return($Conf, $HTTP, $Path, $head);
    }

sub pretty_date
{
    my $time = shift;
    my @t = gmtime($time);
    my ($sec, $min, $hour, $mday, $mon, $year,$wday) = @t;
    my @days = ("Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat");
    my @months = ("Jan", "Feb", "Mar", "Apr", "May", "Jun",
		  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec");
    $year += 1900;
    $wday = $days[$wday];
    $mon = $months[$mon];
    return sprintf("%s, %02d %s %d %02d:%02d:%02d GMT",
		  $wday, $mday, $mon, $year, $hour, $min, $sec);
}

sub init_all {
    my ($argv_0) = @_;

    $HTTP->{'path_info'} = &http_wash($ENV{'PATH_INFO'});
    $HTTP->{'param'}->{'v'} ||= $HTTP->{'param'}->{'version'};
    $HTTP->{'param'}->{'a'} ||= $HTTP->{'param'}->{'arch'};
    $HTTP->{'param'}->{'i'} ||= $HTTP->{'param'}->{'identifier'};


    $identifier = $HTTP->{'param'}->{'i'};
    $readraw    = $HTTP->{'param'}->{'raw'};
    $ctype      = $HTTP->{'param'}->{'ctype'};
    $Conf = new LXR::Config;

    foreach ($Conf->allvariables) {
	$Conf->variable($_, $HTTP->{'param'}->{$_}) if $HTTP->{'param'}->{$_};
    }
    
    &fixpaths($HTTP->{'path_info'} || $HTTP->{'param'}->{'file'});

    my $head = '';
    if (defined($readraw)) {
	$ctype = ($ctype =~ m|([\w\d\.;/+-]+)|) ? $1 : undef;
	$head .= ("Content-Type: $ctype\n") if defined $ctype;
    } else {
        $head .= ("Content-Type: text/html\n");

	#
	# Print out a Last-Modified date that is the larger of: the
	# underlying file that we are presenting; and the "source" script
	# itself (passed in as an argument to this function.)  If we can't
	# stat either of them, don't print out a L-M header.  (Note that this
	# stats lxr/source but not lxr/lib/LXR/Common.pm.  Oh well, I can
	# live with that I guess...)    -- jwz, 16-Jun-98
	#
	my $file1 = $Path->{'realf'};
	my $file2 = $argv_0;

	# make sure the thing we call stat with doesn't end in /.
	if ($file1) { $file1 =~ s@/$@@; }
	if ($file2) { $file2 =~ s@/$@@; }

	my $time1 = 0, $time2 = 0;
	if ($file1) { $time1 = (stat($file1))[9]; }
	if ($file2) { $time2 = (stat($file2))[9]; }

	my $mod_time = ($time1 > $time2 ? $time1 : $time2);
	if ($mod_time > 0) {
	    # Last-Modified: Wed, 10 Dec 1997 00:55:32 GMT
	    $head .= ("Last-Modified: ".(pretty_date($mod_time))."\n");
	    # Expires: Thu, 11 Dec 1997 00:55:32 GMT
	    $head .= ("Expires: ".(pretty_date(time+1200))."\n");
	}
    }
    
    
    if (defined($readraw)) {
        print "$head
"; 
	open(RAW, "<", $Path->{'realf'}) || die "couldn't open $Path->{'realf'}";
	while (<RAW>) {
	    print;
	}
	close(RAW);
	exit;
    }

#exit;
    return($Conf, $HTTP, $Path, $head);
}


sub expandtemplate {
    my ($templ, %expfunc) = @_;
    my ($expfun, $exppar);

    while ($templ =~ s/(\{[^\{\}]*)\{([^\{\}]*)\}/$1\01$2\02/s) {}
    
    $templ =~ s/(\$(\w+)(\{([^\}]*)\}|))/{
	if (defined($expfun = $expfunc{$2})) {
	    if ($3 eq '') {
		&$expfun(undef, @expfunc);
	    } else {
		$exppar = $4;
		$exppar =~ s#\01#\{#gs;
		$exppar =~ s#\02#\}#gs;
		&$expfun($exppar, @expfunc);
	    }
	} else {
	    $1;
	}
    }/ges;

    $templ =~ s/\01/\{/gs;
    $templ =~ s/\02/\}/gs;
    return($templ);
}


# What follows is a pretty hairy way of expanding nested templates.
# State information is passed via localized variables.

# The first one is simple, the "banner" template is empty, so we
# simply return an appropriate value.
sub bannerexpand {
    if ($who eq 'source' || $who eq 'sourcedir' || $who eq 'diff') {
	return($Path->{'xref'});
    } else {
	return('');
    }
}

sub pathname {
    return $Path->{'virtf'};
}

sub cvsentriesexpand {
    my ($entryrev, $entrybranch);
    local $,=" | ";
    my ($entriespath, $entryname) = split m|/(?!.*/)|, $Path->{'realf'};
    if (open(CVSENTRIES, "$entriespath/CVS/Entries")) {
        while (<CVSENTRIES>) {
            next unless m|^/\Q$entryname\E/([^/]*)/[^/]*/[^/]*/(.*)|;
            ($entryrev,$entrybranch)=($1,$2);
            $entrybranch =~ s/^T//;
            $entrybranch ||= 'HEAD';
        }
        close(CVSENTRIES);
    }
    return ($entryrev, $entrybranch);
}

sub cvstagexpand {
    my $entrybranch;
    if (open(CVSTAG, " $Path->{'real'}/CVS/Tag")) {
        while (<CVSTAG>) {
            next unless m|^T(.*)$|;
            $entrybranch = $1;
        }
        close(CVSTAG);
    }
    return $entrybranch || 'HEAD';
}

sub cvsversionexpand {
    if ($who eq 'source') {
        my ($entryrev,undef) = cvsentriesexpand();
        return $entryrev;
    }
    if ($who eq 'sourcedir') {
        return cvstagexpand();
    }
    return('');
}

sub cvsbranchexpand {
    if ($who eq 'source') {
        my (undef,$entrybranch) = cvsentriesexpand();
        return $entrybranch;
    }
    if ($who eq 'sourcedir') {
       return cvstagexpand();
    }
    return('');
}

sub pathname {
    my $prefix = '';
    $prefix = '/' . $Conf->prefix if defined $Conf->prefix;
    return url_quote ($prefix . $Path->{'virtf'});
}

sub urlpath {
    return $Path->{'virtf'};
}

sub pathname_unquoted {
    return urlpath();
}

sub filename {
    return url_quote ($Path->{'file'});
}

sub virtfold {
    return url_quote ($Path->{'svnvirt'});
}

sub virttree {
    return url_quote ($Path->{'svntree'});
}

sub treename {
    return $Conf->{'treename'};
}

sub bonsaicvsroot {
    return $Conf->{'bonsaicvsroot'};
}

sub titleexpand {
    if ($who eq 'source' || $who eq 'sourcedir' || $who eq 'diff') {
	return(&treename.' '.$Conf->sourcerootname.$Path->{'virtf'});

    } elsif ($who eq 'ident') {
	my $i = $HTTP->{'param'}->{'i'};
	return(&treename.' identifier search'.
	       ($i ? " \"$i\"" : ''));

    } elsif ($who eq 'search') {
	my $s = $HTTP->{'param'}->{'string'};
        $s =~ tr/+/ /;
        $s =~ s/%(\w\w)/chr(hex $1)/ge;
        $s =~ s/&/&amp;/g;
        $s =~ s/</&lt;/g;
        $s =~ s/>/&gt;/g;


	return(&treename.' freetext search'.
	       ($s ? " \"$s\"" : ''));

    } elsif ($who eq 'find') {
	my $s = $HTTP->{'param'}->{'string'};
	return(&treename.' file search'.
	       ($s ? " \"$s\"" : ''));
    }
}


sub thisurl {
    my $url = $HTTP->{'this_url'};
    $url =~ s/\?$//;
    $url =~ s/([\&\;\=])/sprintf('%%%02x',(unpack('c',$1)))/ge;
    return($url);
}


sub baseurl {
    return($Conf->baseurl);
}

sub rooturl {
    my $root = $Conf->baseurl;
    $root = $1 if $root =~ m{^(.*[^/])/[^/]*$};
    return $root;
}

sub stylesheet {
    my $alt = shift;
    my $pre = "alt$alt-" if defined $alt;
    my $kind;
    $kind = 'style' if -f ('style/' . $pre . "style.css");
    my $ext;
    if ($Path->{'file'}) {
        $ext = $1 if $Path->{'file'} =~ /\.(\w+)$/;
    } else {
        $ext = 'dir' unless $Path->{'file'};
    }
    $kind = $ext if -f ("style/$pre$ext.css");
    return '' unless $kind;
    return "$pre$kind.css";
}

sub stylesheets {
    my $stylesheet;
    my $baseurl = baseurl();
    $stylesheet = stylesheet();
    my $type = 'stylesheet';
    my $style = 'Screen Look';
    my $index = 0 ;
    my $stylesheets;
    while ($stylesheet) {
        $stylesheets .= "<link rel='$type' title='$title' href='$baseurl/style/$stylesheet' type='text/css'>\n" if $stylesheet;
        $type = 'alternate stylesheet';
        return $stylesheets unless $stylesheet = stylesheet(++$index);
        $title = "Alt-$index";
    }
}

sub dotdoturl {
    my $url = $Conf->baseurl;
    $url =~ s@/$@@;
    $url =~ s@/[^/]*$@@;
    return($url);
}

# This one isn't too bad either.  We just expand the "modes" template
# by filling in all the relevant values in the nested "modelink"
# template.
sub modeexpand {
    my $templ = shift;
    my $modex = '';
    my @mlist = ();
    local $mode;
    
    if ($who eq 'source' || $who eq 'sourcedir') {
	push(@mlist, "<b><i>source navigation</i></b>");
    } else {
	push(@mlist, &fileref("source navigation", $Path->{'virtf'}));
    }
    
    if ($who eq 'diff') {
	push(@mlist, "<b><i>diff markup</i></b>");
	
    } elsif (($who eq 'source' || $who eq 'sourcedir') && $Path->{'file'}) {
	push(@mlist, &diffref("diff markup", $Path->{'virtf'}));
    }
    
    if ($who eq 'ident') {
	push(@mlist, "<b><i>identifier search</i></b>");
    } else {
	push(@mlist, &idref("identifier search", ""));
    }

    if ($who eq 'search') {
	push(@mlist, "<b><i>freetext search</i></b>");
    } else {
	push(@mlist, "<a href=\"$Conf->{virtroot}/search".
	     &urlargs."\">freetext search</a>");
    }
    
    if ($who eq 'find') {
	push(@mlist, "<b><i>file search</i></b>");
    } else {
	push(@mlist, "<a href=\"$Conf->{virtroot}/find".
	     &urlargs."\">file search</a>");
    }
    
    foreach $mode (@mlist) {
	$modex .= &expandtemplate($templ,
				  ('modelink', sub { return($mode) }));
    }
    
    return($modex);
}

# This is where it gets a bit tricky.  varexpand expands the
# "variables" template using varname and varlinks, the latter in turn
# expands the nested "varlinks" template using varval.
sub varlinks {
    my $templ = shift;
    my $vlex = '';
    my ($val, $oldval);
    local $vallink;
    
    $oldval = $Conf->variable($var);
    foreach $val ($Conf->varrange($var)) {
	if ($val eq $oldval) {
	    $vallink = "<b><i>$val</i></b>";
	} else {
	    if ($who eq 'source' || $who eq 'sourcedir') {
		$vallink = &fileref($val, 
				    $Conf->mappath($Path->{'virtf'},
						   "$var=$val"),
				    0,
				    "$var=$val");

	    } elsif ($who eq 'diff') {
		$vallink = &diffref($val, $Path->{'virtf'}, "$var=$val");
		
	    } elsif ($who eq 'ident') {
		$vallink = &idref($val, $identifier, "$var=$val");
		
	    } elsif ($who eq 'search') {
		$vallink = "<a href=\"$Conf->{virtroot}/search".
		    &urlargs("$var=$val",
			     "string=".$HTTP->{'param'}->{'string'}).
				 "\">$val</a>";
		
	    } elsif ($who eq 'find') {
		$vallink = "<a href=\"$Conf->{virtroot}/find".
		    &urlargs("$var=$val",
			     "string=".$HTTP->{'param'}->{'string'}).
				 "\">$val</a>";
	    }
	}
	$vlex .= &expandtemplate($templ,
				 ('varvalue', sub { return($vallink) }));

    }
    return($vlex);
}


sub varexpand {
    my $templ = shift;
    my $varex = '';
    local $var;
    
    foreach $var ($Conf->allvariables) {
	$varex .= &expandtemplate($templ,
				  ('varname',  sub { 
				      return($Conf->vardescription($var))}),
				  ('varlinks', \&varlinks));
    }
    return($varex);
}

sub makeheader {
    local $who = shift;
    $template = undef;
    my $def_templ = "<html><title>(".&treename.")</title><body>\n<hr>\n";

    if ($who eq "sourcedir" && $Conf->sourcedirhead) {
	if (!open(TEMPL, $Conf->sourcedirhead)) {
	    &warning("Template ".$Conf->sourcedirhead." does not exist.", 'sourcedirhead');
	    $template = $def_templ;
	}
    } elsif (($who eq "source" || $who eq 'sourcedir') && $Conf->sourcehead) {
	if (!open(TEMPL, $Conf->sourcehead)) {
	    &warning("Template ".$Conf->sourcehead." does not exist.", 'sourcehead');
	    $template = $def_templ;
	}
    } elsif ($who eq "find" && $Conf->findhead) {
	if (!open(TEMPL, $Conf->findhead)) {
	    &warning("Template ".$Conf->findhead." does not exist.", 'findhead');
	    $template = $def_templ;
	}
    } elsif ($who eq "ident" && $Conf->identhead) {
	if (!open(TEMPL, $Conf->identhead)) {
	    &warning("Template ".$Conf->identhead." does not exist.", 'identhead');
	    $template = $def_templ;
	}
    } elsif ($who eq "search" && $Conf->searchhead) {
	if (!open(TEMPL, $Conf->searchhead)) {
	    &warning("Template ".$Conf->searchhead." does not exist.", 'searchhead');
	    $template = $def_templ;
	}
    } elsif ($Conf->htmlhead) {
	if (!open(TEMPL, $Conf->htmlhead)) {
	    &warning("Template ".$Conf->htmlhead." does not exist.", 'htmlhead');
	    $template = $def_templ;
	}
    }

    if (!$template) {
	$save = $/; undef($/);
	$template = <TEMPL>;
	$/ = $save;
	close(TEMPL);
    }
    
    print(
#"<!doctype html public \"-//W3C//DTD HTML 3.2//EN\">\n",
#	  "<html>\n",
#	  "<head>\n",
#	  "<title>",$Conf->sourcerootname," Cross Reference</title>\n",
#	  "<base href=\"",$Conf->baseurl,"\">\n",
#	  "</head>\n",

    	  &expandtemplate($template,
			  ('title',		\&titleexpand),
			  ('banner',		\&bannerexpand),
			  ('baseurl',		\&baseurl),
			  ('stylesheet',	\&stylesheet),
			  ('stylesheets',	\&stylesheets),
			  ('dotdoturl',		\&dotdoturl),
			  ('thisurl',		\&thisurl),
			  ('pathname',		\&pathname),
			  ('filename',          \&filename),
			  ('virtfold',          \&virtfold),
			  ('virttree',          \&virttree),
			  ('beginbonsai',	\&Local::beginbonsai),
			  ('endbonsai',		\&Local::endbonsai),
			  ('begintrac',		\&Local::begintrac),
			  ('endtrac',		\&Local::endtrac),
			  ('beginviewcvs',	\&Local::beginviewcvs),
			  ('endviewcvs',	\&Local::endviewcvs),
			  ('urlpath',		\&urlpath),
			  ('treename',		\&treename),
    			  ('modes',		\&modeexpand),
    			  ('bonsaicvsroot',	\&bonsaicvsroot),
    			  ('cvsversion',	\&cvsversionexpand),
    			  ('cvsbranch',		\&cvsbranchexpand),
    			  ('variables',		\&varexpand)));
}


sub makefooter {
    local $who = shift;
    $template = undef;
    my $def_templ = "<hr>\n</body>\n";

    if ($who eq "sourcedir" && $Conf->sourcedirtail) {
	if (!open(TEMPL, $Conf->sourcedirtail)) {
	    &warning("Template ".$Conf->sourcedirtail." does not exist.", 'sourcedirtail');
	    $template = $def_templ;
	}
    } elsif (($who eq "source" || $who eq 'sourcedir') && $Conf->sourcetail) {
	if (!open(TEMPL, $Conf->sourcetail)) {
	    &warning("Template ".$Conf->sourcetail." does not exist.", 'sourcetail');
	    $template = $def_templ;
	}
    } elsif ($who eq "find" && $Conf->findtail) {
	if (!open(TEMPL, $Conf->findtail)) {
	    &warning("Template ".$Conf->findtail." does not exist.", 'findtail');
	    $template = $def_templ;
	}
    } elsif ($who eq "ident" && $Conf->identtail) {
	if (!open(TEMPL, $Conf->identtail)) {
	    &warning("Template ".$Conf->identtail." does not exist.", 'identtail');
	    $template = $def_templ;
	}
    } elsif ($who eq "search" && $Conf->searchtail) {
	if (!open(TEMPL, $Conf->searchtail)) {
	    &warning("Template ".$Conf->searchtail." does not exist.", 'searchtail');
	    $template = $def_templ;
	}
    } elsif ($Conf->htmltail) {
	if (!open(TEMPL, $Conf->htmltail)) {
	    &warning("Template ".$Conf->htmltail." does not exist.", 'htmltail');
	    $template = $def_templ;
	}
    }

    if (!$template) {
	$save = $/; undef($/);
	$template = <TEMPL>;
	$/ = $save;
	close(TEMPL);
    }
    
    print(&expandtemplate($template,
			  ('banner',	\&bannerexpand),
			  ('thisurl',	\&thisurl),
    			  ('modes',	\&modeexpand),
    			  ('variables',	\&varexpand),
    			  ('baseurl',	\&baseurl),
			  ('dotdoturl',	\&dotdoturl),
                         ),
	  "</html>\n");
}

sub url_quote {
    my($toencode) = (@_);
# don't escape / 
    $toencode=~s|([^a-zA-Z0-9_/\-.])|uc sprintf("%%%02x",ord($1))|eg;
    return $toencode;
}

1;
