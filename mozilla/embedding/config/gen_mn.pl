#!/usr/bin/perl

#################################################################
# Rest of program

use Getopt::Long;
use GenerateManifest;

# Configuration
$win32 = ($^O eq "MSWin32") ? 1 : 0; # ActiveState Perl
if ($win32) {
  $moz = "$ENV{'MOZ_SRC'}/mozilla";
  if ($ENV{'MOZ_DEBUG'}) {
    $chrome = "$moz/dist/WIN32_D.OBJ/Embed/tmpchrome";
  }
  else
  {
    $chrome = "$moz/dist/WIN32_O.OBJ/Embed/tmpchrome";
  }
}
else {
  $moz = "~/mozilla";
  $chrome = "$moz/dist/Embed/tmpchrome";
}

$verbose = 0;
$locale = "en-US";

GetOptions('verbose!' => \$verbose,
           'mozpath=s' => \$moz,
           'manifest=s' => \$manifest,
	   'chrome=s' => \$chrome,
	   'locale=s' => \$locale,
           'help' => \$showhelp);

if ($win32) {
  # Fix those pesky backslashes
  $moz =~ s/\\/\//g;
  $chrome =~ s/\\/\//g;
}

if ($showhelp) {
  print STDERR "Embedding manifest generator.\n",
               "Usage:\n",
               "  -help            Show this help.\n",
               "  -manifest <file> Specify the input manifest.\n",
               "  -mozpath <path>  Specify the path to Mozilla.\n",
               "  -chrome <path>   Specify the path to the chrome.\n",
               "  -locale <value>  Specify the locale to use. (e.g. en-US)\n",
               "  -verbose         Print extra information\n";
  exit 1;
}

if ($manifest eq "") {
  die("Error: No input manifest file was specified.\n");
}

if ($verbose) {
  print STDERR "Mozilla path  = \"$moz\"\n",
               "Chrome path   = \"$chrome\"\n",
               "Manifest file = \"$manifest\"\n";
}

GenerateManifest($moz, $manifest, $chrome, $locale, *STDOUT, "/", $verbose);

exit 0;
