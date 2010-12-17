#!/usr/bin/perl --
# -*- Mode: perl; indent-tabs-mode: nil -*-
#
# The contents of this file are subject to the Netscape Public
# License Version 1.1 (the "License"); you may not use this file
# except in compliance with the License. You may obtain a copy of
# the License at http://www.mozilla.org/NPL/
#
# Software distributed under the License is distributed on an "AS
# IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
# implied. See the License for the specific language governing
# rights and limitations under the License.
#
# The Original Code is the Tinderbox build tool.
#
# The Initial Developer of the Original Code is Netscape Communications
# Corporation. Portions created by Netscape are
# Copyright (C) 1998 Netscape Communications Corporation. All
# Rights Reserved.
#
# Contributor(s): 

use strict;
use Socket;

require 'header.pl';

print "Content-type: text/html\n\n";

EmitHtmlTitleAndHeader("tinderbox: add images", "add images");

$| = 1;

require "tbglobals.pl";
require "imagelog.pl";

my %form = &split_cgi_args();


sub Error {
    my ($msg) = @_;
    print "<BR><BR><BR>";
    print "<UL><FONT SIZE='+1'><B>Something went wrong:</B><P>";
    print "<UL>";
    print $msg;
    print "</UL>";
    print "<P>";
    print "Hit <B>\`Back'</B> and try again.";
    print "</UL>";
    exit 1;
}


my ($url, $safe_url, $quote, $safe_quote, $width, $height, $size );

if( $url = $form{"url"} ){
    $quote = $form{"quote"};

    $quote =~ s/[\r\n]/ /g;
    $url =~ s/[\r\n]/ /g;
    $safe_url = value_encode($url);
    $safe_quote = value_encode($quote);

    $width = "";
    $height = "";
# I think we don't want to allow this --jwz
#    $width = $form{"width"};
#    $height = $form{"height"};

#    if ($width eq "" || $height eq "") {
        $size = &URLsize($url);
        if ($size =~ /WIDTH=([0-9]*)/) {
            $width = $1;
        }
        if ($size =~ /HEIGHT=([0-9]*)/) {
            $height = $1;
        }
        if ($width eq "" || $height eq "") {
            Error "Couldn't get image size for \"$safe_url\".\n";
        }
#    }

    print " 
    
<P><center><img border=2 src='$safe_url' width=$width height=$height><br>
<i>$safe_quote</i><br><br>
";

    if( $form{"submit"} ne "Yes" ){
        print "
<form action='addimage.cgi' METHOD='get'>
<input type=hidden name=url value=\"$safe_url\">
<input type=hidden name=quote value=\"$safe_quote\">
<HR>
<TABLE>
 <TR>
  <TH ALIGN=RIGHT NOWRAP>Image URL:</TH>
  <TD><TT><B>$safe_url</B></TT></TD>
 </TR><TR>
  <TH ALIGN=RIGHT>Caption:</TH>
  <TD><TT><B>$safe_quote</B></TT></TD>
 </TR>
 <TR>
  <TD></TD>
  <TD>
   <FONT SIZE=+2><B>
   Does that look right?
   <SPACER SIZE=10>
   <INPUT Type='submit' name='submit' value='Yes'>
   </B><BR>(If not, hit \`Back' and fix it.)
   </FONT>
 </TD>
</TABLE>
</form>
";
    }
    else {
        &add_imagelog( $url, $quote, $width, $height );
        print "<br><br>
        <font size=+2>Has been added</font><br><br>
        <a href=showbuilds.cgi>Return to Log</a>";
    }

}
else {
    print "
<h2>Add an image and a funny caption.</h2>

<ul>
<p>This is about fun, and making your daily excursion to 
<A HREF=http://www.mozilla.org/tinderbox.html>Tinderbox</A> a
novel experience.  Engineers spend a lot of time here; it might as well
have some entertainment value.

<p>Please play nice. We don't have the time or inclination to look at
everything you people submit, but if we get nastygrams or legalgrams
and have to take something down, we will curse your IP address, and you
might even make it so the whole thing goes away forever. Please don't
make us go there. You might also avoid links to big images or slow
servers.  

<p><ul><B>Thank you for playing nice.</B></UL>

<p>If you really find an image offensive, please
<A HREF=mailto:terry\@netscape.com?Subject=offensive%20tinderbox%20image>tell us</A>
nicely before someone causes a stink.  Be sure to include the URL of
the image.  Remember, we don't screen these submissions and may not
have even seen it.

<p><ul><B>P.S. Please, no more pictures of Bill Gates.</B></UL>
</ul>

<p><form action='addimage.cgi' METHOD='get'>
<TABLE>
 <TR>
  <TH ALIGN=RIGHT NOWRAP>Image URL:</TH>
  <TD><INPUT NAME='url' SIZE=60></TD>
 </TR><TR>
  <TH ALIGN=RIGHT>Caption:</TH>
  <TD><INPUT NAME='quote' SIZE=60></TD>
 </TR><TR>
  <TD></TD>
  <TD><B>
   <INPUT Type='submit' name='submit' value='Test'>
   <SPACER SIZE=25>
   <INPUT Type='reset' name='reset' value='Reset'>
  </B></TD>
 </TR>
</TABLE>

</form>
<br><br>
";
}

#sub imgsize {
#    local($file)= @_;
#
#    #first try to open the file
#    if( !open(STREAM, "<", $file) ){
#        Error "Can't open IMG $file"; 
##        $size="";
#    } else {
#        if ($file =~ /.jpg/i || $file =~ /.jpeg/i) {
#            $size = &jpegsize(STREAM);
#        } elsif($file =~ /.gif/i) {
#            $size = &gifsize(STREAM);
#        } elsif($file =~ /.xbm/i) {
#            $size = &xbmsize(STREAM);
#        } else {
#           return "";
#        }
#        $_ = $size;
#        if(  /\s*width\s*=\s*([0-9]*)\s*/i ){
#            ($newwidth)= /\s*width\s*=\s*(\d*)\s*/i;
#        }
#        if(  /\s*height\s*=\s*([0-9]*)\s*/i ){
#            ($newheight)=/\s*height\s*=\s*(\d*)\s*/i;
#        }
#        close(STREAM);
#    }
#    return $size;
#}

###########################################################################
# Subroutine gets the size of the specified GIF
###########################################################################

# bug: it thinks that 
# http://cvs1.mozilla.org/webtools/tinderbox/data/knotts.gif
# is 640x400, but it's really 200x245.
# giftrans says of that image:
#
#   Header: "GIF87a"
#   Logical Screen Descriptor:
#       Logical Screen Width: 640 pixels
#       Logical Screen Height: 480 pixels
#   Image Descriptor:
#       Image Width: 200 pixels
#       Image Height: 245 pixels


sub gifsize {
    my ($GIF) = @_;
    my ($size, $type, $s);
    my ($a,$b,$c,$d);
    read($GIF, $type, 6); 
    if(!($type =~ /GIF8[7,9]a/) || 
       !(read($GIF, $s, 4) == 4) ){
        Error "Invalid or Corrupted GIF"; 
        $size="";
    } else {
        ($a,$b,$c,$d)=unpack("C"x4,$s);
        $size=join ("", 'WIDTH=', $b<<8|$a, ' HEIGHT=', $d<<8|$c);
    }
    return $size;
}

sub xbmsize {
    my ($XBM) = @_;
    my $input ="";
    my ($size, $a, $b);

    $input .= <$XBM>;
    $input .= <$XBM>;
    $_ = $input;
    if( /#define\s*\S*\s*\d*\s*\n#define\s*\S*\s*\d*\s*\n/i ){
       ($a,$b)=/#define\s*\S*\s*(\d*)\s*\n#define\s*\S*\s*(\d*)\s*\n/i;
       $size=join ("", 'WIDTH=', $a, ' HEIGHT=', $b );
   } else {
       Error "Doesn't look like an XBM file";
   }
    return $size;
}

# jpegsize : gets the width and height (in pixels) of a jpeg file
# Andrew Tong, werdna@ugcs.caltech.edu           February 14, 1995
# modified slightly by alex@ed.ac.uk
sub jpegsize {
    my ($JPEG) = @_;
    my $done=0;
    my $size="";
    my ($c1, $c2, $ch, $s, $length, $marker, $junk);
    my ($a,$b,$c,$d);

    read($JPEG, $c1, 1); read($JPEG, $c2, 1);
    if( !((ord($c1) == 0xFF) && (ord($c2) == 0xD8))){
        my $s = sprintf "This is not a JPEG! (Codes %02X %02X)\n", ord($c1), ord($c2);
        Error $s;
        $done=1;
    }
    while (ord($ch) != 0xDA && !$done) {
        # Find next marker (JPEG markers begin with 0xFF)
        # This can hang the program!!
        while (ord($ch) != 0xFF) {  read($JPEG, $ch, 1); }
        # JPEG markers can be padded with unlimited 0xFF's
        while (ord($ch) == 0xFF) { read($JPEG, $ch, 1); }
        # Now, $ch contains the value of the marker.
            $marker=ord($ch);

            if (($marker >= 0xC0) && ($marker <= 0xCF) &&
            ($marker != 0xC4) && ($marker != 0xCC)) {  # it's a SOFn marker
            read ($JPEG, $junk, 3); read($JPEG, $s, 4);
            ($a,$b,$c,$d)=unpack("C"x4,$s);
            $size=join("", 'HEIGHT=',$a<<8|$b,' WIDTH=',$c<<8|$d );
            $done=1;
        } else {
            # We **MUST** skip variables, since FF's within variable 
            # names are NOT valid JPEG markers
            read ($JPEG, $s, 2); 
            ($c1, $c2) = unpack("C"x2,$s); 
            $length = $c1<<8|$c2;
            if( ($length < 2) ){
                Error "Bad JPEG file: erroneous marker length";
                $done=1;
            } else {
                read($JPEG, $junk, $length-2);
            }
        }
    }
    return $size;
}

###########################################################################
# Subroutine grabs a gif from another server and gets its size
###########################################################################


sub URLsize {
    my ($fullurl) = @_;
    my $S = new IO::Handle;

    $_ = value_quote($fullurl);
    if ( ! m@^http://@ ) {
        Error "HTTP URLs only, please: \"$_\" is no good.";
    }

    my($dummy, $dummy2, $serverstring, $url) = split(/\//, $fullurl, 4);
    my($them,$port) = split(/:/, $serverstring);
    $port = 80 unless $port;
    my $size="";
    my ($newheight, $newwidth);

    $_ = value_quote($them);
    if ( m@^[^.]*$@ ) {
        Error "Fully-qualified host names only, please: \"$_\" is no good.";
    }

    $_=$url;
    my ($remote, $safe_remote, $iaddr, $paddr, $proto, $line);
    $remote = $them;
    $safe_remote = value_encode($them);
    if ($port =~ /\D/) { $port = getservbyname($port, 'tcp') }
    die "No port" unless $port;
    $iaddr   = inet_aton($remote)               || die "no host: $safe_remote";
    $paddr   = sockaddr_in($port, $iaddr);

    $proto   = getprotobyname('tcp');
    socket($S, PF_INET, SOCK_STREAM, $proto)  || die "socket: $!";
    connect($S, $paddr)    || die "connect: $!";
    select($S); $| = 1; select(STDOUT);

    print $S "GET /$url HTTP/1.0\r\n";
    print $S "Host: $them\r\n";
    print $S "User-Agent: Tinderbox/0.0\r\n";
    print $S "\r\n";

    $_ = <$S>;
    if (! m@^HTTP/[0-9.]+ 200@ ) {
        Error "$them responded badly";
    }

    my $ctype = "";
    while (<$S>) {
        # print "read: $_<br>\n";
        if ( m@^Content-Type:[ \t]*([^ \t\r\n]+)@io ) {
            $ctype = $1;
        }
        last if (/^[\r\n]/);
    }

    $_ = $ctype;
    if ( $_ eq "" ) {
        Error "Server returned no content-type for url?";
    } elsif ( m@image/jpeg@i || m@image/pjpeg@i ) {
        $size = &jpegsize($S);
    } elsif ( m@image/gif@i ) {
        $size = &gifsize($S);
    } elsif ( m@image/xbm@i || m@image/x-xbm@i || m@image/x-xbitmap@i ) {
        $size = &xbmsize($S);
    } else {
        Error "Not a GIF, JPEG, or XBM.";
    }               

    $_ = $size;
    if(  /\s*width\s*=\s*([0-9]*)\s*/i ){
        ($newwidth)= /\s*width\s*=\s*(\d*)\s*/i;
    }
    if(  /\s*height\s*=\s*([0-9]*)\s*/i ){
        ($newheight)=/\s*height\s*=\s*(\d*)\s*/i;
    }

    if ( $newwidth eq "" || $newheight eq "" ) {
        return "";
    } else {
        if ( $newwidth <= 5 || $newheight <= 5 ) {
            Error "${newwidth}x${newheight} seems small, don't you think?";
        } elsif ( $newwidth >= 400 || $newheight >= 400 ) {
            Error "${newwidth}x${newheight} is too big; please" .
                " keep it under 400x400."
        }
        return $size;
    }
}

#sub dokill {
#    kill 9,$child if $child;
#}
