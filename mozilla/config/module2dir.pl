#!/usr/bin/perl -w

#
# Create a mapping from symbolic component name to directory name(s).
#
# Tue Oct 16 16:48:36 PDT 2001
# <mcafee@netscape.com>

use strict;

# For --option1, --option2, ...
use Getopt::Long;
Getopt::Long::Configure("bundling_override");
Getopt::Long::Configure("auto_abbrev");


sub PrintUsage {
  die <<END_USAGE
  Prints out directories needed for a given list of components.
  usage: module2dir.pl [--list-only] <component-name1> <component-name2> ...
END_USAGE
}

my %map = (
  'accessibility',      'accessible',
  'addrbook',           'mailnews/addrbook',
  'appcomps',           'xpfe/components',
  'appshell',           'xpfe/appshell',
  'appstartup',         'embedding/components/appstartup',
  'browser',            'xpfe/browser',
  'caps',               'caps',
  'chardet',            'intl/chardet',
  'chrome',             'rdf/chrome',
  'commandhandler',     'embedding/components/commandhandler',
  'content',            'content',
  'content_xsl',        'content/xsl/document',
  'cookie',             'extensions/cookie',
  'dbm',                'dbm',
  'docshell',           'docshell',
  'dom',                'dom',
  'downloadmanager',    'xpfe/components/download-manager',
  'editor',             'editor',
  'embed_base',         'embedding/base',
  'embedcomponents',    'embedding/components/appstartup',
  'expat',              'expat',
  'exthandler',         'uriloader/exthandler',
  'find',               'xpfe/components/find embedding/components/find',
  'gfx',                'gfx',
  'gfx2',               'gfx2',
  'gtkxtbin',           'widget/src/gtkxtbin',
  'helperAppDlg',       'embedding/components/ui/helperAppDlg',
  'htmlparser',         'htmlparser',
  'imgbmp',             'modules/libpr0n/decoders/bmp',
  'imggif',             'modules/libpr0n/decoders/gif',
  'imgicon',            'modules/libpr0n/decoders/icon',
  'imgjpeg',            'modules/libpr0n/decoders/jpeg',
  'imglib2',            'modules/libpr0n',
  'imgmng',             'modules/libpr0n/decoders/mng',
  'imgpng',             'modules/libpr0n/decoders/png',
  'imgppm',             'modules/libpr0n/decoders/ppm',
  'import',             'mailnews/import',
  'intl',               'intl',
  'jar',                'modules/libjar',
  'java',               'sun-java/stubs',
  'jprof',              'tools/jprof',
  'js',                 'js',
  'jsconsole',          'embedding/components/jsconsole',
  'layout',             'layout',
  'libreg',             'modules/libreg',
  'locale',             'intl/locale',
  'lwbrk',              'intl/lwbrk',
  'mailnews',           'mailnews',
  'mime',               'mailnews/mime',
  'mimetype',           'netwerk/mime',
  'mork',               'db/mork db/mdb',
  'mozcomps',           'xpfe/components',
  'mozldap',            'directory/xpcom/base',
  'mpfilelocprovider',  'modules/mpfilelocprovider',
  'msgbase',            'mailnews/base',
  'msgbaseutil',        'mailnews/base/util',
  'msgcompose',         'mailnews/compose',
  'msgdb',              'mailnews/db/msgdb',
  'msgimap',            'mailnews/imap',
  'msglocal',           'mailnews/local',
  'msgnews',            'mailnews/news',
  'necko',              'netwerk',
  'necko2',             'netwerk/protocol',
  'nkcache',            'netwerk/cache',
  'oji',                'modules/oji',
  'plugin',             'modules/plugin',
  'png',                'modules/libimg/png',
  'pref',               'modules/libpref',
  'prefmigr',           'profile/pref-migrator',
  'profile',            'profile',
  'progressDlg',        'embedding/components/ui/progressDlg',
  'rdf',                'rdf',
  'rdfutil',            'rdf/util',
  'shistory',           'xpfe/components/shistory',
  'sidebar',            'xpfe/components/sidebar',
  'string',             'string',
  'transformiix',       'extensions/transformiix',
  'txmgr',              'editor/txmgr',
  'txtsvc',             'editor/txtsvc',
  'uconv',              'intl/uconv',
  'unicharutil',        'intl/unicharutil',
  'uriloader',          'uriloader',
  'util',               'modules/libutil',
  'view',               'view',
  'wallet',             'extensions/wallet',
  'webbrwsr',           'embedding/browser/webBrowser',
  'webbrowserpersist',  'embedding/components/webbrowserpersist',
  'webshell',           'webshell',
  'widget',             'widget',
  'windowwatcher',      'embedding/components/windowwatcher',
  'xlibrgb',            'gfx/src/xlibrgb',
  'xmlextras',          'extensions/xmlextras',
  'xpcom',              'xpcom',
  'xpconnect',          'js/src/xpconnect',
  'xpconnect_tests',    'js/src/xpconnect/tests',
  'xpinstall',          'xpinstall',
  'xremoteservice',     'xpfe/components/xremote',
  'xul',                'content/xul/content',
  'xuldoc',             'content/xul/document',
  'xultmpl',            'content/xul/templates',
  'zlib',               'modules/zlib',
);


sub dir_for_required_component {
  my ($component) = @_;
  my $rv;
  my $dir;

  $dir = $map{$component};
  if($dir) {
	# prepend "mozilla/" in front of directory names.
	$rv = "mozilla/$dir";
	$rv =~ s/\s+/ mozilla\//g;  # Hack for 2 or more directories.
  } else {
	$rv = 0;
  }
  return $rv;
}

my $list_only_mode = 0;
my $opt_list_only;
{

  # Add stdin to the commandline.  This makes commandline-only mode hang,
  # call it a bug.  Not sure how to get around this.
  push (@ARGV, split(' ',<STDIN>));

  PrintUsage() if !GetOptions('list-only' => \$opt_list_only);

  # Pick up arguments, if any.
  if($opt_list_only) {
  	$list_only_mode = 1;
  }

  my $arg;
  my $dir;
  while ($arg = shift @ARGV) {
	$dir = dir_for_required_component($arg);
	if($dir) {
      if($list_only_mode) {
		print $dir, " ";
	  } else {
		print "$arg: ", $dir, "\n";
	  }
	} else {
	  # do nothing
	}
  }
  if($dir && $list_only_mode) {
	print "\n";
  }
}
