#!perl -w
package			NGLayoutBuildList;

require 5.004;
require Exporter;

use strict;
use vars qw( @ISA @EXPORT );

# perl includes
use Mac::StandardFile;
use Mac::Processes;
use Cwd;
use File::Path;

# homegrown
use Moz;
use MacCVS;

@ISA			= qw(Exporter);
@EXPORT			= qw( Checkout BuildDist BuildProjects BuildCommonProjects BuildLayoutProjects);

# NGLayoutBuildList builds the nglayout project
# it is configured by setting the following variables in the caller:
# Usage:
# caller variables that affect behaviour:
# DEBUG		: 1 if we are building a debug version
# 3-part build process: checkout, dist, and build_projects
# Hack alert:
# NGLayout defines are located in :mozilla:config:mac:NGLayoutConfigInclude.h
# An alias "MacConfigInclude.h" to this file is created inside dist:config
# Note that the name of alias is different than the name of the file. This
# is to trick CW into including NGLayout defines 


#//--------------------------------------------------------------------------------------------------
#// Utility routines
#//--------------------------------------------------------------------------------------------------

# pickWithMemoryFile stores the information about the user pick inside
# the file $session_storage
sub _pickWithMemoryFile($)
{
	my ($sessionStorage) = @_;
	my $cvsfile;

	if (( -e $sessionStorage) &&
		 open( SESSIONFILE, $sessionStorage ))
	{
	# Read in the path if available
	  $cvsfile = <SESSIONFILE>;
	  chomp $cvsfile;
		close SESSIONFILE;
		if ( ! -e $cvsfile )
		{
			print STDERR "$cvsfile has disappeared\n";
			undef $cvsfile;
		}
	}
	unless (defined ($cvsfile))
	{
		print "Choose a CVS session file in file dialog box:\n";	# no way to display a prompt?
# make sure that MacPerl is a front process
  while (GetFrontProcess	() !=  GetCurrentProcess())
  {
	   SetFrontProcess( GetCurrentProcess() );
  }
	# prompt user for the file name, and store it
		my $macFile = StandardGetFile( 0, "McvD");	
		if ( $macFile->sfGood() )
		{
			$cvsfile = $macFile->sfFile();
		# save the choice if we can
			if ( open (SESSIONFILE, ">" . $sessionStorage))
			{
				printf SESSIONFILE $cvsfile, "\n";
				close SESSIONFILE;
			}
			else
			{
				print STDERR "Could not open storage file\n";
			}
		}
	}
	return $cvsfile;
}

# assert that we are in the correct directory for the build
sub _assertRightDirectory()
{
	unless (-e ":mozilla")
	{
		my($dir) = cwd();
		print STDERR "NGLayoutBuildList called from incorrect directory: $dir";
	} 
}

sub _getDistDirectory()
{
	return $main::DEBUG ? ":mozilla:dist:viewer_debug:" : ":mozilla:dist:viewer:";
}


#//--------------------------------------------------------------------------------------------------
#// Checkout everything
#//--------------------------------------------------------------------------------------------------

sub Checkout()
{
	_assertRightDirectory();
	my($cvsfile) = _pickWithMemoryFile("::nglayout.cvsloc");
	my($session) = MacCVS->new( $cvsfile );
	unless (defined($session)) { die "Checkout aborted. Cannot create session file: $session" }

	#//
	#//	Checkout commands
	#//
	if ($main::pull{all})
	{
		$session->checkout("RaptorMac")					|| die "checkout failure";
		$session->checkout("mozilla/modules/libpref")	|| die "checkout failure";
	}
}


#//--------------------------------------------------------------------------------------------------
#// Build the 'dist' directory
#//--------------------------------------------------------------------------------------------------

sub BuildDist()
{
	unless ( $main::build{dist} ) { return;}
	_assertRightDirectory();
	
	# we really do not need all these paths, but many client projects include them
	mkpath([ ":mozilla:dist:", ":mozilla:dist:client:", ":mozilla:dist:client_debug:", ":mozilla:dist:client_stubs:" ]);
	mkpath([ ":mozilla:dist:viewer:", ":mozilla:dist:viewer_debug:" ]);

	my($distdirectory) = ":mozilla:dist";

	#MAC_COMMON
	InstallFromManifest(":mozilla:build:mac:MANIFEST",								"$distdirectory:mac:common:");
	InstallFromManifest(":mozilla:lib:mac:NSStdLib:include:MANIFEST",				"$distdirectory:mac:common:");
	InstallFromManifest(":mozilla:lib:mac:MacMemoryAllocator:include:MANIFEST",	"$distdirectory:mac:common:");
	InstallFromManifest(":mozilla:lib:mac:Misc:MANIFEST",							"$distdirectory:mac:common:");
	InstallFromManifest(":mozilla:lib:mac:MoreFiles:MANIFEST",						"$distdirectory:mac:common:morefiles:");

	#INCLUDE
	InstallFromManifest(":mozilla:config:mac:MANIFEST",							"$distdirectory:config:");
	InstallFromManifest(":mozilla:config:mac:MANIFEST_config",						"$distdirectory:config:");
	InstallFromManifest(":mozilla:include:MANIFEST",								"$distdirectory:include:");		
	InstallFromManifest(":mozilla:cmd:macfe:pch:MANIFEST",							"$distdirectory:include:");
	InstallFromManifest(":mozilla:cmd:macfe:utility:MANIFEST",						"$distdirectory:include:");

	#NSPR	
    InstallFromManifest(":mozilla:nsprpub:pr:include:MANIFEST",					"$distdirectory:nspr:");		
    InstallFromManifest(":mozilla:nsprpub:pr:src:md:mac:MANIFEST",					"$distdirectory:nspr:mac:");		
    InstallFromManifest(":mozilla:nsprpub:lib:ds:MANIFEST",						"$distdirectory:nspr:");		
    InstallFromManifest(":mozilla:nsprpub:lib:libc:include:MANIFEST",				"$distdirectory:nspr:");		
    InstallFromManifest(":mozilla:nsprpub:lib:msgc:include:MANIFEST",				"$distdirectory:nspr:");

	#JPEG
    InstallFromManifest(":mozilla:jpeg:MANIFEST",									"$distdirectory:jpeg:");

	#LIBREG
    InstallFromManifest(":mozilla:modules:libreg:include:MANIFEST",				"$distdirectory:libreg:");

	#XPCOM
    InstallFromManifest(":mozilla:xpcom:public:MANIFEST",								"$distdirectory:xpcom:");

	#ZLIB
    InstallFromManifest(":mozilla:modules:zlib:src:MANIFEST",						"$distdirectory:zlib:");

	#LIBUTIL
    InstallFromManifest(":mozilla:modules:libutil:public:MANIFEST",				"$distdirectory:libutil:");

	#SUN_JAVA
    InstallFromManifest(":mozilla:sun-java:stubs:include:MANIFEST",				"$distdirectory:sun-java:");
    InstallFromManifest(":mozilla:sun-java:stubs:macjri:MANIFEST",					"$distdirectory:sun-java:");

	#NAV_JAVA
    InstallFromManifest(":mozilla:nav-java:stubs:include:MANIFEST",				"$distdirectory:nav-java:");
    InstallFromManifest(":mozilla:nav-java:stubs:macjri:MANIFEST",					"$distdirectory:nav-java:");

	#JS
    InstallFromManifest(":mozilla:js:src:MANIFEST",								"$distdirectory:js:");

	#SECURITY_freenav
    InstallFromManifest(":mozilla:modules:security:freenav:MANIFEST",				"$distdirectory:security:");

	#LIBPREF
    InstallFromManifest(":mozilla:modules:libpref:public:MANIFEST",				"$distdirectory:libpref:");

	#LIBIMAGE
    InstallFromManifest(":mozilla:modules:libimg:png:MANIFEST",					"$distdirectory:libimg:");
    InstallFromManifest(":mozilla:modules:libimg:src:MANIFEST",					"$distdirectory:libimg:");
    InstallFromManifest(":mozilla:modules:libimg:public:MANIFEST",					"$distdirectory:libimg:");

	#PLUGIN
    InstallFromManifest(":mozilla:modules:plugin:nglsrc:MANIFEST",					"$distdirectory:plugin:");
    InstallFromManifest(":mozilla:modules:plugin:public:MANIFEST",					"$distdirectory:plugin:");
    InstallFromManifest(":mozilla:modules:plugin:src:MANIFEST",					"$distdirectory:plugin:");
    InstallFromManifest(":mozilla:modules:oji:src:MANIFEST",						"$distdirectory:oji:");
    InstallFromManifest(":mozilla:modules:oji:public:MANIFEST",					"$distdirectory:oji:");

	#LAYERS (IS THIS STILL NEEDED)
	InstallFromManifest(":mozilla:lib:liblayer:include:MANIFEST",					"$distdirectory:layers:");

	#NETWORK
    InstallFromManifest(":mozilla:network:cache:MANIFEST",							"$distdirectory:network:");
    InstallFromManifest(":mozilla:network:client:MANIFEST",						"$distdirectory:network:");
    InstallFromManifest(":mozilla:network:cnvts:MANIFEST",							"$distdirectory:network:");
    InstallFromManifest(":mozilla:network:cstream:MANIFEST",						"$distdirectory:network:");
    InstallFromManifest(":mozilla:network:main:MANIFEST",							"$distdirectory:network:");
    InstallFromManifest(":mozilla:network:mimetype:MANIFEST",						"$distdirectory:network:");
    InstallFromManifest(":mozilla:network:util:MANIFEST",							"$distdirectory:network:");
    InstallFromManifest(":mozilla:network:protocol:about:MANIFEST",				"$distdirectory:network:");
    InstallFromManifest(":mozilla:network:protocol:certld:MANIFEST",				"$distdirectory:network:");
    InstallFromManifest(":mozilla:network:protocol:dataurl:MANIFEST",				"$distdirectory:network:");
    InstallFromManifest(":mozilla:network:protocol:file:MANIFEST",					"$distdirectory:network:");
    InstallFromManifest(":mozilla:network:protocol:ftp:MANIFEST",					"$distdirectory:network:");
    InstallFromManifest(":mozilla:network:protocol:gopher:MANIFEST",				"$distdirectory:network:");
    InstallFromManifest(":mozilla:network:protocol:http:MANIFEST",					"$distdirectory:network:");
    InstallFromManifest(":mozilla:network:protocol:js:MANIFEST",					"$distdirectory:network:");
    InstallFromManifest(":mozilla:network:protocol:mailbox:MANIFEST",				"$distdirectory:network:");
    InstallFromManifest(":mozilla:network:protocol:marimba:MANIFEST",				"$distdirectory:network:");
    InstallFromManifest(":mozilla:network:protocol:nntp:MANIFEST",					"$distdirectory:network:");
    InstallFromManifest(":mozilla:network:protocol:pop3:MANIFEST",					"$distdirectory:network:");
    InstallFromManifest(":mozilla:network:protocol:remote:MANIFEST",				"$distdirectory:network:");
    InstallFromManifest(":mozilla:network:protocol:smtp:MANIFEST",					"$distdirectory:network:");
    InstallFromManifest(":mozilla:network:module:MANIFEST",						"$distdirectory:network:module");

	#BASE
    InstallFromManifest(":mozilla:base:src:MANIFEST",								"$distdirectory:base:");
    InstallFromManifest(":mozilla:base:public:MANIFEST",							"$distdirectory:base:");

	#WEBSHELL
    InstallFromManifest(":mozilla:webshell:public:MANIFEST",						"$distdirectory:webshell:");

	#LAYOUT
    InstallFromManifest(":mozilla:layout:build:MANIFEST",							"$distdirectory:layout:");
    InstallFromManifest(":mozilla:layout:base:public:MANIFEST",					"$distdirectory:layout:");
    InstallFromManifest(":mozilla:layout:html:style:public:MANIFEST",				"$distdirectory:layout:");
    InstallFromManifest(":mozilla:layout:html:base:src:MANIFEST",					"$distdirectory:layout:");
    InstallFromManifest(":mozilla:layout:base:src:MANIFEST",						"$distdirectory:layout:");
	InstallFromManifest(":mozilla:layout:events:public:MANIFEST",					"$distdirectory:layout:");
	InstallFromManifest(":mozilla:layout:events:src:MANIFEST",						"$distdirectory:layout:");

	#WIDGET
    InstallFromManifest(":mozilla:widget:public:MANIFEST",							"$distdirectory:widget:");
    InstallFromManifest(":mozilla:widget:src:mac:MANIFEST",						"$distdirectory:widget:");

	#GFX
    InstallFromManifest(":mozilla:gfx:src:MANIFEST",										"$distdirectory:gfx:");
    InstallFromManifest(":mozilla:gfx:public:MANIFEST",								"$distdirectory:gfx:");

	#VIEW
    InstallFromManifest(":mozilla:view:public:MANIFEST",							"$distdirectory:view:");

	#DOM
   InstallFromManifest(":mozilla:dom:public:MANIFEST",								"$distdirectory:dom:");
   InstallFromManifest(":mozilla:dom:public:coreDom:MANIFEST",						"$distdirectory:dom:");
   InstallFromManifest(":mozilla:dom:public:coreEvents:MANIFEST",					"$distdirectory:dom:");
   InstallFromManifest(":mozilla:dom:public:events:MANIFEST",						"$distdirectory:dom:");
   InstallFromManifest(":mozilla:dom:public:html:MANIFEST",						"$distdirectory:dom:");
   InstallFromManifest(":mozilla:dom:public:css:MANIFEST",							"$distdirectory:dom:");
   InstallFromManifest(":mozilla:dom:src:jsurl:MANIFEST",							"$distdirectory:dom:");

	#HTMLPARSER
   InstallFromManifest(":mozilla:htmlparser:src:MANIFEST",							"$distdirectory:htmlparser:");

	#// To get out defines in all the project, dummy alias NGLayoutConfigInclude.h into MacConfigInclude.h
	MakeAlias(":mozilla:config:mac:NGLayoutConfigInclude.h",	":mozilla:dist:config:MacConfigInclude.h");
}


#//--------------------------------------------------------------------------------------------------
#// Build common projects
#//--------------------------------------------------------------------------------------------------

sub BuildCommonProjects()
{
	unless( $main::build{common} ) { return; }
	_assertRightDirectory();

	# $D becomes a suffix to target names for selecting either the debug or non-debug target of a project
	my($D) = $main::DEBUG ? "Debug" : "";
	my($dist_dir) = _getDistDirectory();

	#//
	#// Clean projects
	#//
	Moz::BuildProjectClean(":mozilla:lib:mac:MacMemoryAllocator:MemAllocator.mcp",	"Stubs");
	Moz::BuildProjectClean(":mozilla:lib:mac:NSStdLib:NSStdLib.mcp",              	"Stubs");
	Moz::BuildProjectClean(":mozilla:lib:mac:NSRuntime:NSRuntime.mcp",				"Stubs");
	Moz::BuildProjectClean(":mozilla:cmd:macfe:projects:client:Client.mcp",    		"Stubs");

	#//
	#// Shared libraries
	#//
	if ( $main::CARBON )
	{
		BuildProject(":mozilla:cmd:macfe:projects:interfaceLib:Interface.mcp",			"Carbon Interfaces");
	}
	else
	{
		BuildProject(":mozilla:cmd:macfe:projects:interfaceLib:Interface.mcp",			"MacOS Interfaces");
	}
		
	Moz::BuildProject(":mozilla:lib:mac:NSRuntime:NSRuntime.mcp",						"NSRuntime$D.shlb");
	MakeAlias(":mozilla:lib:mac:NSRuntime:NSRuntime$D.shlb",							"$dist_dir");
	$main::DEBUG ? MakeAlias(":mozilla:lib:mac:NSRuntime:NSRuntime$D.shlb.xSYM",		"$dist_dir") : 0;
	
	Moz::BuildProject(":mozilla:lib:mac:MoreFiles:build:MoreFilesPPC.mcp",				"MoreFiles$D.shlb");
	MakeAlias(":mozilla:lib:mac:MoreFiles:build:MoreFiles$D.shlb",						"$dist_dir");
	$main::DEBUG ? MakeAlias(":mozilla:lib:mac:MoreFiles:build:MoreFiles$D.shlb.xSYM",	"$dist_dir") : 0;

	BuildProject(":mozilla:nsprpub:macbuild:NSPR20PPC.mcp",								"NSPR20$D.shlb");
	MakeAlias(":mozilla:nsprpub:macbuild:NSPR20$D.shlb",								"$dist_dir");
	$main::DEBUG ? MakeAlias(":mozilla:nsprpub:macbuild:NSPR20$D.shlb.xSYM",			"$dist_dir") : 0;

	BuildProject(":mozilla:lib:mac:MacMemoryAllocator:MemAllocator.mcp",					"MemAllocator$D.shlb");
	MakeAlias(":mozilla:lib:mac:MacMemoryAllocator:MemAllocator$D.shlb",					"$dist_dir");
	$main::DEBUG ? MakeAlias(":mozilla:lib:mac:MacMemoryAllocator:MemAllocator$D.shlb.xSYM","$dist_dir") : 0;
	
	BuildProject(":mozilla:lib:mac:NSStdLib:NSStdLib.mcp",								"NSStdLib$D.shlb");
	MakeAlias(":mozilla:lib:mac:NSStdLib:NSStdLib$D.shlb",								"$dist_dir");
	$main::DEBUG ? MakeAlias(":mozilla:lib:mac:NSStdLib:NSStdLib$D.shlb.xSYM",			"$dist_dir") : 0;

	BuildProject(":mozilla:jpeg:macbuild:JPEG.mcp",										"JPEG$D.shlb");
	MakeAlias(":mozilla:jpeg:macbuild:JPEG$D.shlb",										"$dist_dir");
	$main::DEBUG ? MakeAlias(":mozilla:jpeg:macbuild:JPEG$D.shlb.xSYM",					"$dist_dir") : 0;

	BuildProject(":mozilla:js:macbuild:JavaScriptPPC.mcp",								"JavaScriptNoJSJ$D.shlb");
	MakeAlias(":mozilla:js:macbuild:JavaScript$D.shlb",									"$dist_dir");
	$main::DEBUG ? MakeAlias(":mozilla:js:macbuild:JavaScript$D.shlb.xSYM",				"$dist_dir") : 0;

	BuildProject(":mozilla:modules:zlib:macbuild:zlib.mcp",								"zlib$D.shlb");
	MakeAlias(":mozilla:modules:zlib:macbuild:zlib$D.shlb",								"$dist_dir");
	$main::DEBUG ? MakeAlias(":mozilla:modules:zlib:macbuild:zlib$D.shlb.xSYM",			"$dist_dir") : 0;
	
	#//
	#// Static libraries
	#//
	BuildProject(":mozilla:xpcom:macbuild:xpcomPPC.mcp",								"xpcom$D.o");
	BuildProject(":mozilla:modules:security:freenav:macbuild:NoSecurity.mcp",			"Security.o");
	BuildProject(":mozilla:modules:libimg:macbuild:png.mcp",							"png$D.o");
	BuildProject(":mozilla:modules:libimg:macbuild:libimg.mcp",							"libimg$D.o (standalone)");
	BuildProject(":mozilla:network:macbuild:network.mcp",								"NetworkModular$D.o");
}


sub BuildResourceAliases
{
	my($src_dir, $dest_dir) = @_;
	
	# get a list of all the resource files
	opendir(SRCDIR, $src_dir) || die("can't open $src_dir");
	my(@resource_files) = readdir(SRCDIR);
	closedir(SRCDIR);
	
	# make aliases for each one into the dest directory
	for ( @resource_files ) {
		next if $_ eq "CVS";
		
		my($file_name) = $src_dir . $_;	
		print("Placing alias to file $file_name in $dest_dir\n");
		MakeAlias($file_name, $dest_dir);
	}
}


#//--------------------------------------------------------------------------------------------------
#// Build NGLayout
#//--------------------------------------------------------------------------------------------------

sub BuildLayoutProjects()
{
	unless( $main::build{nglayout} ) { return; }
	_assertRightDirectory();

	# $D becomes a suffix to target names for selecting either the debug or non-debug target of a project
	my($D) = $main::DEBUG ? "Debug" : "";
	my($dist_dir) = _getDistDirectory();

	#//
	#// Make aliases of resource files
	#//
	my($resource_dir) = "$dist_dir" . "res:";
	MakeAlias(":mozilla:layout:html:document:src:ua.css",				"$resource_dir");

	my($html_dir) = "$resource_dir" . "html:";
	MakeAlias(":mozilla:layout:html:base:src:broken-image.gif",			"$html_dir");

	my($throbber_dir) = "$resource_dir" . "throbber:";
	BuildResourceAliases(":mozilla:webshell:tests:viewer:throbber:", "$throbber_dir");
	
	my($samples_dir) = "$resource_dir" . "samples:";
	BuildResourceAliases(":mozilla:webshell:tests:viewer:samples:", "$samples_dir");

	my($chrome_dir) = "$resource_dir" . "chrome:";
	BuildResourceAliases(":mozilla:xpfe:xpviewer:src:resources:chrome:", "$chrome_dir");
	
	my($toolbar_dir) = "$resource_dir" . "toolbar:";
	BuildResourceAliases(":mozilla:xpfe:xpviewer:src:resources:toolbar:", "$toolbar_dir");
	
	#//
	#// Build Layout projects
	#//
	BuildProject(":mozilla:base:macbuild:base.mcp",						"base$D.o");
	BuildProject(":mozilla:htmlparser:macbuild:htmlparser.mcp",			"htmlparser$D.o");
	BuildProject(":mozilla:dom:macbuild:dom.mcp",						"dom$D.o");
	BuildProject(":mozilla:gfx:macbuild:gfx.mcp",						"gfx$D.o");
	BuildProject(":mozilla:layout:macbuild:layout.mcp",					"layout$D.o");
	BuildProject(":mozilla:widget:macbuild:widget.mcp",					"widget$D.o");
	BuildProject(":mozilla:webshell:macbuild:webshell.mcp",				"webshell$D.o");
	BuildProject(":mozilla:webshell:tests:viewer:mac:viewer.mcp",		"viewer$D");
	BuildProject(":mozilla:xpfe:macbuild:xpfeviewer.mcp",				"xpfeViewer$D");
}


#//--------------------------------------------------------------------------------------------------
#// Build everything
#//--------------------------------------------------------------------------------------------------

sub BuildProjects()
{
	BuildCommonProjects();
	BuildLayoutProjects();
}
