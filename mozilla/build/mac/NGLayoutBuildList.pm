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
use MANIFESTO;

@ISA			= qw(Exporter);
@EXPORT			= qw(Checkout BuildDist BuildProjects BuildCommonProjects BuildLayoutProjects);

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
		$session->checkout("SeaMonkeyEditor")						|| die "checkout failure";
  
		#// beard:  additional libraries needed to make shared libraries link.
		#//$session->checkout("mozilla/lib/mac/PowerPlant")	|| die "checkout failure";
		#//$session->checkout("mozilla/lib/xlate")				|| die "checkout failure";
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
	
	if ($main::MOZ_FULLCIRCLE)
	{
		if ( $main::DEBUG )
		{
			mkpath([ "$distdirectory:viewer_debug:TalkBack"]);
		} 
		else
		{
			mkpath([ "$distdirectory:viewer:TalkBack"]);
		}
	}
	
	#MAC_COMMON
	InstallFromManifest(":mozilla:build:mac:MANIFEST",								"$distdirectory:mac:common:");
	InstallFromManifest(":mozilla:lib:mac:NSStdLib:include:MANIFEST",				"$distdirectory:mac:common:");
	InstallFromManifest(":mozilla:lib:mac:MacMemoryAllocator:include:MANIFEST",		"$distdirectory:mac:common:");
	InstallFromManifest(":mozilla:lib:mac:Misc:MANIFEST",							"$distdirectory:mac:common:");
	InstallFromManifest(":mozilla:lib:mac:MoreFiles:MANIFEST",						"$distdirectory:mac:common:morefiles:");

	#INCLUDE
	InstallFromManifest(":mozilla:config:mac:MANIFEST",								"$distdirectory:config:");
	InstallFromManifest(":mozilla:config:mac:MANIFEST_config",						"$distdirectory:config:");
	InstallFromManifest(":mozilla:include:MANIFEST",								"$distdirectory:include:");		
	InstallFromManifest(":mozilla:cmd:macfe:pch:MANIFEST",							"$distdirectory:include:");
	InstallFromManifest(":mozilla:cmd:macfe:utility:MANIFEST",						"$distdirectory:include:");

	#NSPR	
    InstallFromManifest(":mozilla:nsprpub:pr:include:MANIFEST",						"$distdirectory:nspr:");		
    InstallFromManifest(":mozilla:nsprpub:pr:src:md:mac:MANIFEST",					"$distdirectory:nspr:mac:");		
    InstallFromManifest(":mozilla:nsprpub:lib:ds:MANIFEST",							"$distdirectory:nspr:");		
    InstallFromManifest(":mozilla:nsprpub:lib:libc:include:MANIFEST",				"$distdirectory:nspr:");		
    InstallFromManifest(":mozilla:nsprpub:lib:msgc:include:MANIFEST",				"$distdirectory:nspr:");

	#INTL
	#UCONV
	InstallFromManifest(":mozilla:intl:uconv:public:MANIFEST",						"$distdirectory:uconv:");
	InstallFromManifest(":mozilla:intl:uconv:ucvlatin:MANIFEST",					"$distdirectory:uconv:");
	InstallFromManifest(":mozilla:intl:uconv:ucvja:MANIFEST",						"$distdirectory:uconv:");
	InstallFromManifest(":mozilla:intl:uconv:ucvja2:MANIFEST",						"$distdirectory:uconv:");


	#UNICHARUTIL
	InstallFromManifest(":mozilla:intl:unicharutil:public:MANIFEST",				"$distdirectory:unicharutil");

	#LOCALE
	InstallFromManifest(":mozilla:intl:locale:public:MANIFEST",						"$distdirectory:locale:");

	#LWBRK
	InstallFromManifest(":mozilla:intl:lwbrk:public:MANIFEST",						"$distdirectory:lwbrk:");

	#STRRES
	InstallFromManifest(":mozilla:intl:strres:public:MANIFEST",						"$distdirectory:strres:");

	#JPEG
    InstallFromManifest(":mozilla:jpeg:MANIFEST",									"$distdirectory:jpeg:");

	#LIBREG
    InstallFromManifest(":mozilla:modules:libreg:include:MANIFEST",					"$distdirectory:libreg:");

	#XPCOM
    InstallFromManifest(":mozilla:xpcom:public:MANIFEST",							"$distdirectory:xpcom:");

	#ZLIB
    InstallFromManifest(":mozilla:modules:zlib:src:MANIFEST",						"$distdirectory:zlib:");

	#LIBUTIL
    InstallFromManifest(":mozilla:modules:libutil:public:MANIFEST",					"$distdirectory:libutil:");

	#SUN_JAVA
    InstallFromManifest(":mozilla:sun-java:stubs:include:MANIFEST",					"$distdirectory:sun-java:");
    InstallFromManifest(":mozilla:sun-java:stubs:macjri:MANIFEST",					"$distdirectory:sun-java:");

	#NAV_JAVA
    InstallFromManifest(":mozilla:nav-java:stubs:include:MANIFEST",					"$distdirectory:nav-java:");
    InstallFromManifest(":mozilla:nav-java:stubs:macjri:MANIFEST",					"$distdirectory:nav-java:");

	#JS
    InstallFromManifest(":mozilla:js:src:MANIFEST",									"$distdirectory:js:");

	#LIVECONNECT
	InstallFromManifest(":mozilla:js:src:liveconnect:MANIFEST",						"$distdirectory:liveconnect:");
	
	#CAPS
	InstallFromManifest(":mozilla:caps:public:MANIFEST",							"$distdirectory:caps:");
	InstallFromManifest(":mozilla:caps:include:MANIFEST",							"$distdirectory:caps:");

	#SECURITY_freenav
    InstallFromManifest(":mozilla:modules:security:freenav:MANIFEST",				"$distdirectory:security:");

	#LIBPREF
    InstallFromManifest(":mozilla:modules:libpref:public:MANIFEST",					"$distdirectory:libpref:");

	#LIBIMAGE
    InstallFromManifest(":mozilla:modules:libimg:png:MANIFEST",						"$distdirectory:libimg:");
    InstallFromManifest(":mozilla:modules:libimg:src:MANIFEST",						"$distdirectory:libimg:");
    InstallFromManifest(":mozilla:modules:libimg:public:MANIFEST",					"$distdirectory:libimg:");

	#PLUGIN
    InstallFromManifest(":mozilla:modules:plugin:nglsrc:MANIFEST",					"$distdirectory:plugin:");
    InstallFromManifest(":mozilla:modules:plugin:public:MANIFEST",					"$distdirectory:plugin:");
    InstallFromManifest(":mozilla:modules:plugin:src:MANIFEST",						"$distdirectory:plugin:");
    InstallFromManifest(":mozilla:modules:oji:src:MANIFEST",						"$distdirectory:oji:");
    InstallFromManifest(":mozilla:modules:oji:public:MANIFEST",						"$distdirectory:oji:");

	#LAYERS (IS THIS STILL NEEDED)
	InstallFromManifest(":mozilla:lib:liblayer:include:MANIFEST",					"$distdirectory:layers:");

	#NETWORK
    InstallFromManifest(":mozilla:network:public:MANIFEST",							"$distdirectory:network:");
    InstallFromManifest(":mozilla:network:cache:MANIFEST",							"$distdirectory:network:");
    InstallFromManifest(":mozilla:network:client:MANIFEST",							"$distdirectory:network:");
    InstallFromManifest(":mozilla:network:cnvts:MANIFEST",							"$distdirectory:network:");
    InstallFromManifest(":mozilla:network:cstream:MANIFEST",						"$distdirectory:network:");
    InstallFromManifest(":mozilla:network:main:MANIFEST",							"$distdirectory:network:");
    InstallFromManifest(":mozilla:network:mimetype:MANIFEST",						"$distdirectory:network:");
    InstallFromManifest(":mozilla:network:util:MANIFEST",							"$distdirectory:network:");
    InstallFromManifest(":mozilla:network:protocol:about:MANIFEST",					"$distdirectory:network:");
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
    InstallFromManifest(":mozilla:network:protocol:sockstub:MANIFEST",				"$distdirectory:network:");
    InstallFromManifest(":mozilla:network:module:MANIFEST",							"$distdirectory:network:module");

	#BASE
    InstallFromManifest(":mozilla:base:src:MANIFEST",								"$distdirectory:base:");
    InstallFromManifest(":mozilla:base:public:MANIFEST",							"$distdirectory:base:");

	#WEBSHELL
    InstallFromManifest(":mozilla:webshell:public:MANIFEST",						"$distdirectory:webshell:");
    InstallFromManifest(":mozilla:webshell:tests:viewer:public:MANIFEST",   "$distdirectory:webshell:");

	#LAYOUT
    InstallFromManifest(":mozilla:layout:build:MANIFEST",							"$distdirectory:layout:");
    InstallFromManifest(":mozilla:layout:base:public:MANIFEST",						"$distdirectory:layout:");
        InstallFromManifest(":mozilla:layout:html:document:src:MANIFEST",                               "$distdirectory:layout:");
    InstallFromManifest(":mozilla:layout:html:style:public:MANIFEST",				"$distdirectory:layout:");
    InstallFromManifest(":mozilla:layout:html:style:src:MANIFEST",					"$distdirectory:layout:");
    InstallFromManifest(":mozilla:layout:html:base:src:MANIFEST",					"$distdirectory:layout:");
	InstallFromManifest(":mozilla:layout:html:forms:public:MANIFEST",				"$distdirectory:layout:");
    InstallFromManifest(":mozilla:layout:html:table:public:MANIFEST",		"$distdirectory:layout:");
    InstallFromManifest(":mozilla:layout:base:src:MANIFEST",						"$distdirectory:layout:");
	InstallFromManifest(":mozilla:layout:events:public:MANIFEST",					"$distdirectory:layout:");
	InstallFromManifest(":mozilla:layout:events:src:MANIFEST",						"$distdirectory:layout:");
	InstallFromManifest(":mozilla:layout:xml:document:public:MANIFEST",				"$distdirectory:layout:");
	InstallFromManifest(":mozilla:layout:xml:content:public:MANIFEST",				"$distdirectory:layout:");
	

	#WIDGET
    InstallFromManifest(":mozilla:widget:public:MANIFEST",							"$distdirectory:widget:");
    InstallFromManifest(":mozilla:widget:src:mac:MANIFEST",							"$distdirectory:widget:");

	#GFX
    InstallFromManifest(":mozilla:gfx:public:MANIFEST",								"$distdirectory:gfx:");

	#VIEW
    InstallFromManifest(":mozilla:view:public:MANIFEST",							"$distdirectory:view:");

	#DOM
   InstallFromManifest(":mozilla:dom:public:MANIFEST",								"$distdirectory:dom:");
   InstallFromManifest(":mozilla:dom:public:base:MANIFEST",							"$distdirectory:dom:");
   InstallFromManifest(":mozilla:dom:public:coreDom:MANIFEST",						"$distdirectory:dom:");
   InstallFromManifest(":mozilla:dom:public:coreEvents:MANIFEST",					"$distdirectory:dom:");
   InstallFromManifest(":mozilla:dom:public:events:MANIFEST",						"$distdirectory:dom:");
   InstallFromManifest(":mozilla:dom:public:html:MANIFEST",							"$distdirectory:dom:");
   InstallFromManifest(":mozilla:dom:public:css:MANIFEST",							"$distdirectory:dom:");
   InstallFromManifest(":mozilla:dom:src:jsurl:MANIFEST",							"$distdirectory:dom:");
   InstallFromManifest(":mozilla:dom:src:base:MANIFEST",							"$distdirectory:dom:");

	#HTMLPARSER
   InstallFromManifest(":mozilla:htmlparser:src:MANIFEST",							"$distdirectory:htmlparser:");
   
    #RDF
    InstallFromManifest(":mozilla:rdf:base:public:MANIFEST",						"$distdirectory:rdf:");
    InstallFromManifest(":mozilla:rdf:content:public:MANIFEST",						"$distdirectory:rdf:");
    InstallFromManifest(":mozilla:rdf:datasource:public:MANIFEST",					"$distdirectory:rdf:");
    InstallFromManifest(":mozilla:rdf:build:MANIFEST",								"$distdirectory:rdf:");

	#EDITOR
   InstallFromManifest(":mozilla:editor:public:MANIFEST",							"$distdirectory:editor:");
   InstallFromManifest(":mozilla:editor:txmgr:public:MANIFEST",						"$distdirectory:editor:txmgr");
   
    #SILENTDL
    InstallFromManifest(":mozilla:silentdl:MANIFEST",								"$distdirectory:silentdl:");

   #FULL CIRCLE    
   if ($main::MOZ_FULLCIRCLE)
   {
		InstallFromManifest(":ns:fullsoft:public:MANIFEST",								"$distdirectory");
	
		if ($main::DEBUG)
		{
			#InstallFromManifest(":ns:fullsoft:public:MANIFEST",						"$distdirectory:viewer_debug:");
		} 
		else
		{
			#InstallFromManifest(":ns:fullsoft:public:MANIFEST",						"$distdirectory:viewer:");
			InstallFromManifest(":ns:fullsoft:public:MANIFEST",							"$distdirectory");
		}
	}

	# XPAPPS
   InstallFromManifest(":mozilla:xpfe:AppCores:public:MANIFEST",						"$distdirectory:xpfe:");
   InstallFromManifest(":mozilla:xpfe:appshell:public:MANIFEST",						"$distdirectory:xpfe:");

	#// To get out defines in all the project, dummy alias NGLayoutConfigInclude.h into MacConfigInclude.h
	MakeAlias(":mozilla:config:mac:NGLayoutConfigInclude.h",	":mozilla:dist:config:MacConfigInclude.h");
}


#//--------------------------------------------------------------------------------------------------
#// Build stub projects
#//--------------------------------------------------------------------------------------------------

sub BuildStubs()
{

	unless( $main::build{stubs} ) { return; }
	_assertRightDirectory();

	#//
	#// Clean projects
	#//
	BuildProjectClean(":mozilla:lib:mac:MacMemoryAllocator:MemAllocator.mcp",	"Stubs");
	BuildProjectClean(":mozilla:lib:mac:NSStdLib:NSStdLib.mcp",              	"Stubs");
	BuildProjectClean(":mozilla:lib:mac:NSRuntime:NSRuntime.mcp",				"Stubs");
#	BuildProjectClean(":mozilla:cmd:macfe:projects:client:Client.mcp",    		"Stubs");
}


#//--------------------------------------------------------------------------------------------------
#// Build one project, and make the alias. Parameters
#// are project path, target name, make shlb alias (boolean), make xSYM alias (boolean)
#// 
#// Note that this routine assumes that the target name and the shared libary name
#// are the same.
#//--------------------------------------------------------------------------------------------------

sub BuildOneProject($$$$$)
{
	my ($project_path, $target_name, $toc_file, $alias_shlb, $alias_xSYM) = @_;

	# $D becomes a suffix to target names for selecting either the debug or non-debug target of a project
	my($D) = $main::DEBUG ? "Debug" : "";
	my($dist_dir) = _getDistDirectory();

	my($project_dir) = $project_path;
	$project_dir =~ s/:[^:]+$/:/;			# chop off leaf name

	if ($main::CLOBBER_LIBS)
	{
		unlink "$project_dir$target_name";				# it's OK if these fail
		unlink "$project_dir$target_name.xSYM";
	}
	
	if ($toc_file ne "")
	{
		ReconcileProject("$project_path", "$project_dir$toc_file");
	}
	
	BuildProject($project_path, $target_name);
	
	$alias_shlb ? MakeAlias("$project_dir$target_name", "$dist_dir") : 0;
	$alias_xSYM ? MakeAlias("$project_dir$target_name.xSYM", "$dist_dir") : 0;
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
	#// Stub libraries
	#//
	BuildProject(":mozilla:modules:security:freenav:macbuild:NoSecurity.mcp",			"Security.o");

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
	
	BuildOneProject(":mozilla:lib:mac:NSRuntime:NSRuntime.mcp",					"NSRuntime$D.shlb", "", 1, $main::ALIAS_SYM_FILES);

	BuildOneProject(":mozilla:lib:mac:MoreFiles:build:MoreFilesPPC.mcp",		"MoreFiles$D.shlb", "", 1, $main::ALIAS_SYM_FILES);

	BuildOneProject(":mozilla:nsprpub:macbuild:NSPR20PPC.mcp",					"NSPR20$D.shlb", "NSPR20.toc", 1, $main::ALIAS_SYM_FILES);

	BuildOneProject(":mozilla:lib:mac:MacMemoryAllocator:MemAllocator.mcp",		"MemAllocator$D.shlb", "", 1, $main::ALIAS_SYM_FILES);

	BuildOneProject(":mozilla:lib:mac:NSStdLib:NSStdLib.mcp",					"NSStdLib$D.shlb", "", 1, $main::ALIAS_SYM_FILES);

	BuildOneProject(":mozilla:jpeg:macbuild:JPEG.mcp",							"JPEG$D.shlb", "JPEG.toc", 1, $main::ALIAS_SYM_FILES);

	BuildOneProject(":mozilla:xpcom:macbuild:xpcomPPC.mcp",						"xpcom$D.shlb", "xpcom.toc", 1, $main::ALIAS_SYM_FILES);

	BuildOneProject(":mozilla:js:macbuild:JavaScript.mcp",						"JavaScript$D.shlb", "JavaScript.toc", 1, $main::ALIAS_SYM_FILES);

	BuildOneProject(":mozilla:js:macbuild:LiveConnect.mcp",						"LiveConnect$D.shlb", "", 1, $main::ALIAS_SYM_FILES);

	BuildOneProject(":mozilla:modules:zlib:macbuild:zlib.mcp",					"zlib$D.shlb", "zlib.toc", 1, $main::ALIAS_SYM_FILES);
	
	BuildOneProject(":mozilla:caps:macbuild:Caps.mcp",						 	"Caps$D.shlb", "", 1, $main::ALIAS_SYM_FILES);
	
	BuildOneProject(":mozilla:modules:oji:macbuild:oji.mcp",					"oji$D.shlb", "", 1, $main::ALIAS_SYM_FILES);

	BuildOneProject(":mozilla:modules:libpref:macbuild:libpref.mcp",			"libpref$D.shlb", "libpref.toc", 1, $main::ALIAS_SYM_FILES);

	BuildOneProject(":mozilla:base:macbuild:base.mcp",							"base$D.shlb", "base.toc", 1, $main::ALIAS_SYM_FILES);

	BuildOneProject(":mozilla:intl:uconv:macbuild:uconv.mcp",					"uconv$D.shlb", "uconv.toc", 1, $main::ALIAS_SYM_FILES);

	BuildOneProject(":mozilla:intl:uconv:macbuild:ucvlatin.mcp",				"ucvlatin$D.shlb", "uconvlatin.toc", 1, $main::ALIAS_SYM_FILES);

	BuildOneProject(":mozilla:intl:uconv:macbuild:ucvja.mcp",					"ucvja$D.shlb", "ucvja.toc", 1, $main::ALIAS_SYM_FILES);
		
	BuildOneProject(":mozilla:intl:unicharutil:macbuild:unicharutil.mcp",		"unicharutil$D.shlb", "unicharutil.toc", 1, $main::ALIAS_SYM_FILES);

	BuildOneProject(":mozilla:intl:locale:macbuild:locale.mcp",					"nslocale$D.shlb", "nslocale.toc", 1, $main::ALIAS_SYM_FILES);
	
	BuildOneProject(":mozilla:intl:lwbrk:macbuild:lwbrk.mcp",					"lwbrk$D.shlb", "lwbrk.toc", 1, $main::ALIAS_SYM_FILES);
	

    BuildOneProject(":mozilla:intl:strres:macbuild:strres.mcp",					"strres$D.shlb", "strres.toc", 1, $main::ALIAS_SYM_FILES);



#// removing powerplant - long live powerplant
#	BuildOneProject(":mozilla:lib:mac:PowerPlant:PowerPlant.mcp",				"PowerPlant$D.shlb", "", 1, $main::ALIAS_SYM_FILES);

	BuildOneProject(":mozilla:modules:libutil:macbuild:libutil.mcp",			"libutil$D.shlb", "libutil.toc", 1, $main::ALIAS_SYM_FILES);

	ReconcileProject(":mozilla:modules:libimg:macbuild:png.mcp", 				":mozilla:modules:libimg:macbuild:png.toc");
	BuildProject(":mozilla:modules:libimg:macbuild:png.mcp",					"png$D.o");

	BuildOneProject(":mozilla:modules:libimg:macbuild:libimg.mcp",				"libimg$D.shlb", "libimg.toc", 1, $main::ALIAS_SYM_FILES);

	BuildOneProject(":mozilla:network:macbuild:network.mcp",					"NetworkModular$D.shlb", "network.toc", 1, $main::ALIAS_SYM_FILES);

#// XXX moved this TEMPORARILY to layout while we sort out a dependency
#	BuildOneProject(":mozilla:rdf:macbuild:rdf.mcp",							"rdf$D.shlb", "rdf.toc", 1, $main::ALIAS_SYM_FILES);
}


#//--------------------------------------------------------------------------------------------------
#// Make resource aliases for one directory
#//--------------------------------------------------------------------------------------------------

sub BuildFolderResourceAliases($$)
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
#// Make resource aliases
#//--------------------------------------------------------------------------------------------------

sub MakeResouceAliases()
{
	unless( $main::build{resources} ) { return; }
	_assertRightDirectory();


	# $D becomes a suffix to target names for selecting either the debug or non-debug target of a project
	my($D) = $main::DEBUG ? "Debug" : "";
	my($dist_dir) = _getDistDirectory();

	#//
	#// Make aliases of resource files
	#//
	my($resource_dir) = "$dist_dir" . "res:";
	MakeAlias(":mozilla:layout:html:document:src:ua.css",								"$resource_dir");
	MakeAlias(":mozilla:webshell:tests:viewer:resources:viewer.properties",								"$resource_dir");

	my($html_dir) = "$resource_dir" . "html:";
	MakeAlias(":mozilla:layout:html:base:src:broken-image.gif",							"$html_dir");

	my($throbber_dir) = "$resource_dir" . "throbber:";
#	BuildFolderResourceAliases(":mozilla:xpfe:xpviewer:src:resources:throbber:",		"$throbber_dir");
	BuildFolderResourceAliases(":mozilla:webshell:tests:viewer:throbber:",				"$throbber_dir");
	
	my($samples_dir) = "$resource_dir" . "samples:";
	BuildFolderResourceAliases(":mozilla:webshell:tests:viewer:samples:",				"$samples_dir");
	BuildFolderResourceAliases(":mozilla:webshell:tests:viewer:resources:",				"$samples_dir");

	my($chrome_dir) = "$resource_dir" . "chrome:";
	BuildFolderResourceAliases(":mozilla:xpfe:xpviewer:src:resources:chrome:",			"$chrome_dir");
	
	my($toolbar_dir) = "$resource_dir" . "toolbar:";
	BuildFolderResourceAliases(":mozilla:xpfe:xpviewer:src:resources:toolbar:",			"$toolbar_dir");

	my($rdf_dir) = "$resource_dir" . "rdf:";
	BuildFolderResourceAliases(":mozilla:rdf:resources:",								"$rdf_dir");
	
	# NOTE: this will change as we move the toolbar/appshell chrome files to a real place
	BuildFolderResourceAliases(":mozilla:xpfe:browser:src:", 							"$samples_dir");
	BuildFolderResourceAliases(":mozilla:xpfe:AppCores:xul:",							"$samples_dir");
	BuildFolderResourceAliases(":mozilla:xpfe:AppCores:xul:resources:",					"$toolbar_dir");
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
	#// Make WasteLib alias
	#//
	local(*F);
	my($filepath, $appath, $psi) = (':mozilla:build:mac:idepath.txt');
	if (open(F, $filepath)) {
		$appath = <F>;
		close(F);
		#// beard: use pattern substitution to generate path to WasteLib. (thanks gordon!)
		$appath =~ s/[^:]*$/MacOS Support:WASTE 1.3 Distribution:WASTELib/;
		my($wastelibpath) = $appath;
		MakeAlias("$wastelibpath", "$dist_dir");
	}
	else {
		print STDERR "Can't find $filepath\n";
	}

	
	#//
	#// Build Layout projects
	#//

	BuildOneProject(":mozilla:htmlparser:macbuild:htmlparser.mcp",				"htmlparser$D.shlb", "htmlparser.toc", 1, $main::ALIAS_SYM_FILES);

	BuildOneProject(":mozilla:dom:macbuild:dom.mcp",							"dom$D.shlb", "dom.toc", 1, $main::ALIAS_SYM_FILES);

	BuildOneProject(":mozilla:gfx:macbuild:gfx.mcp",							"gfx$D.shlb", "gfx.toc", 1, $main::ALIAS_SYM_FILES);

	BuildOneProject(":mozilla:modules:plugin:macbuild:plugin.mcp",							"plugin$D.shlb", "plugin.toc", 1, $main::ALIAS_SYM_FILES);

	BuildOneProject(":mozilla:layout:macbuild:layout.mcp",						"layout$D.shlb", "layout.toc", 1, $main::ALIAS_SYM_FILES);
	
	BuildOneProject(":mozilla:view:macbuild:view.mcp",							"view$D.shlb", "view.toc", 1, $main::ALIAS_SYM_FILES);
	
	BuildOneProject(":mozilla:widget:macbuild:widget.mcp",						"widget$D.shlb", "widget.toc", 1, $main::ALIAS_SYM_FILES);

	BuildOneProject(":mozilla:webshell:macbuild:webshell.mcp",					"webshell$D.shlb", "webshell.toc", 1, $main::ALIAS_SYM_FILES);

	BuildOneProject(":mozilla:webshell:embed:mac:RaptorShell.mcp",					"RaptorShell$D.shlb", "RaptorShell.toc", 1, $main::ALIAS_SYM_FILES);

	#// XXX this is here because of a very TEMPORARY dependency
	BuildOneProject(":mozilla:rdf:macbuild:rdf.mcp",							"rdf$D.shlb", "rdf.toc", 1, $main::ALIAS_SYM_FILES);
}


#//--------------------------------------------------------------------------------------------------
#// Build Editor Projects
#//--------------------------------------------------------------------------------------------------

sub BuildEditorProjects()
{
	unless( $main::build{editor} ) { return; }
	_assertRightDirectory();
	
	# $D becomes a suffix to target names for selecting either the debug or non-debug target of a project
	my($D) = $main::DEBUG ? "Debug" : "";
	my($dist_dir) = _getDistDirectory();

	BuildOneProject(":mozilla:editor:txmgr:macbuild:txmgr.mcp",					"EditorTxmgr$D.shlb", "txmgr.toc", 1, $main::ALIAS_SYM_FILES);

	BuildOneProject(":mozilla:editor:guimgr:macbuild:EditorGuiManager.mcp",		"EditorGuiManager$D.shlb", "EditorGuiManager.toc", 1, $main::ALIAS_SYM_FILES);

	BuildOneProject(":mozilla:editor:macbuild:editor.mcp",						"EditorCore$D.shlb", "EditorCore.toc", 1, $main::ALIAS_SYM_FILES);

}


#//--------------------------------------------------------------------------------------------------
#// Build Viewer Projects
#//--------------------------------------------------------------------------------------------------

sub BuildViewerProjects()
{
	unless( $main::build{viewer} ) { return; }
	_assertRightDirectory();
	
	# $D becomes a suffix to target names for selecting either the debug or non-debug target of a project
	my($D) = $main::DEBUG ? "Debug" : "";
	my($dist_dir) = _getDistDirectory();

	BuildOneProject(":mozilla:webshell:tests:viewer:mac:viewer.mcp",			"viewer$D", "viewer.toc", 0, 0);

#	BuildOneProject(":mozilla:xpfe:macbuild:xpfeviewer.mcp",					"xpfeviewer$D.shlb", "xpfeviewer.toc", 0, 0);
}


#//--------------------------------------------------------------------------------------------------
#// Build XPApp Projects
#//--------------------------------------------------------------------------------------------------

sub BuildXPAppProjects()
{
	unless( $main::build{xpapp} ) { return; }
	_assertRightDirectory();

	# $D becomes a suffix to target names for selecting either the debug or non-debug target of a project
	my($D) = $main::DEBUG ? "Debug" : "";
	my($dist_dir) = _getDistDirectory();

	BuildOneProject(":mozilla:xpfe:appshell:macbuild:AppShell.mcp",				"AppShell$D.shlb", "AppShell.toc", 1, $main::ALIAS_SYM_FILES);

	BuildOneProject(":mozilla:xpfe:AppCores:macbuild:AppCores.mcp",				"AppCores$D.shlb", "AppCores.toc", 1, $main::ALIAS_SYM_FILES);

	BuildOneProject(":mozilla:xpfe:bootstrap:macbuild:apprunner.mcp",			"apprunner$D", "apprunner.toc", 0, 0);

}



#//--------------------------------------------------------------------------------------------------
#// Build everything
#//--------------------------------------------------------------------------------------------------

sub BuildProjects()
{
	BuildStubs();
	BuildCommonProjects();
	BuildLayoutProjects();
	BuildEditorProjects();
	MakeResouceAliases();
	BuildViewerProjects();
	BuildXPAppProjects();
}
