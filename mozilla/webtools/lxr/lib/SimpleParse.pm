# $Id$

use strict;

package SimpleParse;

require Exporter;

use vars qw(@ISA @EXPORT);

@ISA = qw(Exporter);
@EXPORT = qw(&doparse &untabify &init &nextfrag);

my $INFILE;			# Input file handle
my @frags;			# Fragments in queue
my @bodyid;			# Array of body type ids
my @open;			# Fragment opening delimiters
my @term;			# Fragment closing delimiters
my $split;			# Fragmentation regexp
my $open;			# Fragment opening regexp
my $tabwidth;			# Tab width

sub init {
    my @blksep;

    ($INFILE, @blksep) = @_;

    while (@_ = splice(@blksep,0,3)) {
	push(@bodyid, $_[0]);
	push(@open, $_[1]);
	push(@term, $_[2]);
    }

    foreach (@open) {
	$open .= "($_)|";
	$split .= "$_|";
    }
    chop($open);
    
    foreach (@term) {
	next if $_ eq '';
	$split .= "$_|";
    }
    chop($split);

    $tabwidth = 8;
}


sub untabify {
    my $t = $_[1] || 8;

    $_[0] =~ s/([^\t]*)\t/$1.(' ' x ($t - (length($1) % $t)))/ge;
    return($_[0]);
}

my $frag_debug = undef;

sub nextfrag {
    my $btype = undef;
print "<!-- no btype -->
" if $frag_debug;
    my $frag = undef;

    while (1) {
	if ($#frags < 0) {
	    my $line = <$INFILE>;
#$frag_debug = 1 if (!defined $frag_debug && $line =~ /ARGV/);
	    
	    if ($. == 1 &&
		$line =~ /^.*-[*]-.*?[ \t;]tab-width:[ \t]*([0-9]+).*-[*]-/) {
		$tabwidth = $1;
	    }
		
	    &untabify($line, $tabwidth);
#	    $line =~ s/([^\t]*)\t/
#		$1.(' ' x ($tabwidth - (length($1) % $tabwidth)))/ge;

	    @frags = split(/($split)/o, $line);
if ($frag_debug) {
local $, = "\n";
print "<!--
";
print @frags;
print "
-->";
}
	}

	last if $#frags < 0;
	
print "<!-- not last $#frags -->
" if $frag_debug;
	unless (length $frags[0]) {
print "<!-- shifting !length frags 0 -->
" if $frag_debug;
	    shift(@frags);

	} elsif (defined($frag)) {
print "<!-- defined frag $frag -->
" if $frag_debug;
	    if (defined($btype)) {
print "<!-- defined btype $btype -->
" if $frag_debug;
		my $next = shift(@frags);
print "<!-- frag: $frag -->
" if $frag_debug;
		
		$frag .= $next;
		last if $next =~ /^$term[$btype]$/;
print "<!-- !last $next -->
" if $frag_debug;
	    } else {
print "<!-- !defined btype -->
" if $frag_debug;
		last if $frags[0] =~ /^$open$/o;
		$frag .= shift(@frags);
	    }
	} else {
print "<!-- !defined frag -->
" if $frag_debug;
	    $frag = shift(@frags);
	    if (defined($frag) && (@_ = $frag =~ /^$open$/o)) {
		my $i = 1;
		$btype = grep { $i = ($i && !defined($_)) } @_;
print "<!-- new btype $btype @_ -->
" if $frag_debug;
	    }
	}
    }
    $btype = $bodyid[$btype] if $btype;
print "<!-- New btype $btype -->
" if $btype && $frag_debug;
    
    return($btype, $frag);
}

1;
