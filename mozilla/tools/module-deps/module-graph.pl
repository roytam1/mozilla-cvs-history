#!/usr/bin/perl -w

# Usage:
# module-graph.pl [directory [ directory ..] ] > foo.dot
#
# Description:
# Outputs a Graphviz-compatible graph description file for use
# with the utilities dot, sccmap, and so forth.
# Graphviz is available from:
# http://www.research.att.com/sw/tools/graphviz/
#
# Reccomendations:
# View the graphs by creating graphs with dot:
# > dot -Tpng foo.dot -o foo.png
#
# Note to Linux users: graphviz needs TrueType fonts installed
# http://support.pa.msu.edu/Help/FAQs/Linux/truetype.html
#

# Todo:
# - eliminate arcs implied by transitive dependancies
#   (i.e. in a -> b -> c; a->c;, eliminate a->c;
#   (discovered that "tred" will do this, but isn't super-helpful)
# - group together strongly-connected components, where strongly connected
#   means there exists a cycle, and all dependancies off the cycle.
#   in the graph "a -> b <-> c -> d", b and c are strongly connected, and
#   they depend on d, so b, c, and d should be grouped together.

use strict;

# For --option1, --option2, ...
use Getopt::Long;
Getopt::Long::Configure("bundling_override");
Getopt::Long::Configure("auto_abbrev");

sub PrintUsage {
  die <<END_USAGE
  Prints out required modules for specified directories.
  usage: module-graph.pl [--list-only] [--start-module <mod> ] [--file <file> | <dir1> <dir2> ...]
END_USAGE
}

my %clustered;        # strongly connected components.
my %deps;
my %toplevel_modules; # visited components.

my $debug = 0;

my $makecommand;

if ($^O eq "linux") {
  $makecommand = "make";
} elsif ($^O eq "MSWin32") {
  $makecommand = "make";
}

use Cwd;
my @dirs;
my $curdir = getcwd();

my $list_only_mode = 0;  # --list-only argument, only print out module names
my $opt_list_only;

my $load_file = 0;       # --file
my $opt_start_module;    # --start-module optionally print out dependencies    
                         # for a given module.


# Parse commandline input.
sub parse_args() {
  # Stuff arguments into variables.
  # Print usage if we get an unknown argument.
  PrintUsage() if !GetOptions('list-only' => \$opt_list_only,
							  'start-module=s' => \$opt_start_module,
							  'file=s' => \$load_file);

  # Pick up arguments, if any.
  if($opt_list_only) {
	$list_only_mode = 1;
  }

  # Last args are directories, if not in load-from-file mode.
  unless($load_file) {
	if (!@ARGV) {
	  @dirs = (getcwd());
	} else {
	  @dirs = @ARGV;
	  # XXX does them in reverse order..
	  my $arg;
	  foreach $arg (@ARGV) {
		push @dirs, "$curdir/$arg";
	  }
	}
  }
}


#  Build up the %deps matrix.
sub build_deps_matrix() {
MFILE:
  while ($#dirs != -1) {
	my ($current_dirs, $current_module, $current_requires);
	# pop the curdir
	$curdir = pop @dirs;
	
	if(!$list_only_mode) {
	  print STDERR "Entering $curdir..                 \r";
	}
	chdir "$curdir" || next;
	if ($^O eq "linux") {
      next if (! -e "$curdir/Makefile");
	} elsif ($^O eq "MSWin32") {
      next if (! -e "$curdir/makefile.win");
	}
	
	$current_dirs = "";
	open(MAKEOUT, "$makecommand echo-dirs echo-module echo-requires|") || die "Can't make: $!\n";
	
	$current_dirs = <MAKEOUT>; $current_dirs && chop $current_dirs;
	$current_module = <MAKEOUT>; $current_module && chop $current_module;
	$current_requires = <MAKEOUT>; $current_requires && chop $current_requires;
	close MAKEOUT;
	
	if ($current_module) {
	  #
	  # now keep a list of all dependencies of the module
	  #
	  my @require_list = split(/\s+/,$current_requires);
	  my $req;
	  foreach $req (@require_list) {
		$deps{$current_module}{$req}++;
	  }
	
	  $toplevel_modules{$current_module}++;
	}
	
	next if !$current_dirs;
	
	# now push all child directories onto the list
	my @local_dirs = split(/\s+/,$current_dirs);
	for (@local_dirs) {
	  push @dirs,"$curdir/$_" if $_;
	}
  }

  if(!$list_only_mode) {
	print STDERR "\n";
  }
}


#
# 
#
sub build_deps_matrix_from_file {
  my ($filename) = @_;

  open DEPS_FILE, $filename or print "can't open $filename, $?\n";
  my @line;
  while (<DEPS_FILE>) {
	if(/->/) {
	  chomp;
	  s/\;//;  # Strip off ';'

	  # Pick off module, and dependency from -> line.
	  @line = split(' -> ', $_);
	  $deps{$line[0]}{$line[1]}++;

	  # Add the module to the list of modules.
	  $toplevel_modules{$line[0]}++;
	}
  }
  close DEPS_FILE;

}

# Print out %deps.
sub print_deps_matrix() {
  my $module;
  if(!$list_only_mode) {
	print "digraph G {\n";
	print "    concentrate=true;\n";
	
	# figure out the internal nodes, and place them in a cluster
	
	#print "    subgraph cluster0 {\n";
	#print "        color=blue;\n"; # blue outline around cluster
	
	# ** new method: just list all modules that came from MODULE=foo
	foreach $module (sort keys %toplevel_modules) {
	  print "        $module [style=filled];\n"
	}
  }

  print_dependency_list();

  if(!$list_only_mode) {
	print "}\n";
  }
}

# ** old method: find only internal nodes
# (nodes with both parents and children)
sub print_internal_nodes() {
  my $module;
  my $depmod;
  foreach $module (sort { scalar keys %{$deps{$b}} <=> scalar keys %{$deps{$a}} } keys %deps) {
	foreach $depmod ( keys %deps ) {
	  # only in cluster if they are a child too
	  if ($deps{$depmod}{$module}) {
		print "        $module;\n";
		$clustered{$module}++;
		last;
	  }
	}
  }
}

# Run over dependency array to generate raw component list.
# This is the "a -> b" lines.
sub print_dependency_list() {
  my @raw_list;
  my @unique_list;
  my $module;

  foreach $module (sort sortby_deps keys %deps) {
	my $req;
	foreach $req ( sort { $deps{$module}{$b} <=> $deps{$module}{$a} }
				   keys %{ $deps{$module} } ) {
	  #    print "    $module -> $req [weight=$deps{$module}{$req}];\n";
	  if(!$list_only_mode) {
		print "$module -> $req;\n";
	  } else {
		# print "$req ";
		push(@raw_list, $req);
	  }
	}
  }

  # generate unique list, print it out.
  if($list_only_mode) {
	my %saw;
	undef %saw;
	@unique_list = grep(!$saw{$_}++, @raw_list);
	
	my $i;
	for ($i=0;$i <= $#unique_list; $i++) {
	  print $unique_list[$i], " ";
	}
	print "\n";
  }
}


# we're sorting based on clustering
# order:
#   - unclustered, with dependencies
#   - clustered
#   - unclustered, with no dependencies
# However, the last group will probably never come in $a or $b, because we're
# probably only being called from the keys in $deps
# We'll keep all the logic here, in case we come up with a better scheme later
sub sortby_deps() {

  my $keys_a = scalar keys %{$deps{$a}};
  my $keys_b = scalar keys %{$deps{$b}};

  # determine if they are the same or not
  if ($clustered{$a} && $clustered{$b}) {
    # both in "clustered" group
    return $keys_a <=> $keys_b;
  }

  elsif (!$clustered{$a} && !$clustered{$b}) {
    # not clustered. Do they both have dependencies or both
    # have no dependencies?

    if (($keys_a && $keys_b) ||
        (!$keys_a && !$keys_b)) {
      # both unclustered, and either both have dependencies,
      # or both don't have dependencies
      return $keys_a <=> $keys_b;
    }
  }

  # if we get here, then they are in different "groups"
  if ($clustered{$a}) {
    # b must be unclustered
    if ($keys_b) {
      return 1;
    } else {
      return -1;
    }
  } elsif ($clustered{$b}) {
    # a must be unclustered
    if ($keys_a) {
      return -1;
    } else {
      return 1;
    }
  } else {
    # both are unclustered, so the with-dependencies one comes first
    if ($keys_a) {
      return -1;
    } else {
      return 1;
    }
  }
}

#
# Recursively traverse the deps matrix.
#
my %visited_nodes;

sub walk_module_digraph {
  my ($module, $level) = @_;

  # Remember that we visited this node.
  $visited_nodes{$module}++;

  # Print this node.
  if (!$list_only_mode) {
	my $i;
	for ($i=0; $i<$level; $i++) {
	  print "  ";
	}
	print "$module\n";
  }

  # If we haven't visited this node, search again
  # from this node.
  my $depmod;
  foreach $depmod ( keys %{ $deps{$module} } ) {
	my $visited = $visited_nodes{$depmod};

	if(!$visited) {    	# test recursion: if($level < 5)
	  walk_module_digraph($depmod, $level + 1);
	}
  }

  if (!$list_only_mode) {
	if($level == 1) {
	  print "\n";
	}
  }
}


sub print_module_deps {
  # Recursively hunt down dependencies for $opt_start_module
  walk_module_digraph($opt_start_module, 1);

  my $visited_mod;
  foreach $visited_mod (sort keys %visited_nodes ) {
	print "$visited_mod ";
  }
  print "\n";

  if($debug) {
	my @total_visited = (sort keys %visited_nodes);
	my $total = $#total_visited + 1;
	print "\ntotal = $total\n";
  }
}


sub get_matrix_size {
    my (%matrix) = @_;

  	my $i;
	my $j;

	$i = 0;
    $j = 0;
	foreach $i ( keys %matrix ) {
	  $j++;
	}

	return $j;
}


# main
{
  parse_args();

  if($load_file) {
	build_deps_matrix_from_file($load_file);
  } else {
	build_deps_matrix();
  }

  # Print out deps matrix.
  # --list-only and --start-module together mean to
  # print out the module deps, not the matrix.
  if (not ($list_only_mode and $opt_start_module)) {
    print_deps_matrix();
  }

  # If we specified a --start-module option, print out
  # the required modules for that module.
  if($opt_start_module) {
	print_module_deps();
  }

  if($debug) {
	print STDERR "----- sizes -----\n";
	print STDERR "            deps: " . get_matrix_size(%deps) . "\n";
	print STDERR "toplevel_modules: " . get_matrix_size(%toplevel_modules) . "\n";
	print STDERR "       clustered: " . get_matrix_size(%clustered) . "\n";
  }
}
