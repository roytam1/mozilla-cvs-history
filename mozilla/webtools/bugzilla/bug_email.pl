#!/usr/bonsaitools/bin/perl -w
# Contributor(s): Terry Weissman <terry@mozilla.org>
#                 Gregor Fischer <fischer@suse.de>
#                 Klaas Freitag  <freitag@suse.de>
###############################################################
# Bugzilla: Create a new bug via email
###############################################################
# The email needs to be feeded to this program on STDIN. 
# This is usually done by having an entry like this in your 
# .procmailrc:
# 
#     BUGZILLA_HOME=/usr/local/httpd/htdocs/bugzilla
#     :0 c
#     |(cd $BUGZILLA_HOME; ./bug_email.pl)
# 
# $Id$
###############################################################

use diagnostics;
use strict;

#require "CGI.pl";
require "globals.pl";

my @mailerrors = ();       # Buffer for Errors in the mail
my @mailwarnings = ();     # Buffer for Warnings found in the mail
my $critical_err = 0; # Counter for critical errors - must be zero for success
my %Control;
my $Header = "";
my @RequiredLabels = ();
my @AllowedLabels = ();
my $Body = "";

my $product_valid = 0;
my $test = 0;


###############################################################
# Beautification
sub horLine( )
{
    return( "-----------------------------------------------------------------------\n" ); 
}


###############################################################
# Check if $Name is in $GroupName
sub CheckPermissions {
    my ($GroupName, $Name) = @_;
    
    SendSQL("select login_name from profiles,groups where groups.name='$GroupName' and profiles.groupset & groups.bit = groups.bit and profiles.login_name=\'$Name\'");
    my $NewName = FetchOneColumn();
    if ( $NewName eq $Name ) {
	return $Name;
    } else {
	return;
    }
}

###############################################################
# Check if product is valid.
sub CheckProduct {
    my $Product = shift;
    
    SendSQL("select product from products where product='$Product'");
    my $Result = FetchOneColumn();
    if (lc($Result) eq lc($Product)) {
	return $Result;
    } else {
	return "";
    }
}

###############################################################
# Check if component is valid for product.
sub CheckComponent {
    my $Product = shift;
    my $Component = shift;
    
    SendSQL("select value from components where program=" . SqlQuote($Product) . " and value=" . SqlQuote($Component) . "");
    my $Result = FetchOneColumn();
    if (lc($Result) eq lc($Component)) {
	return $Result;
    } else {
	return "";
    }
}

###############################################################
# Check if component is valid for product.
sub CheckVersion {
    my $Product = shift;
    my $Version = shift;
    
    SendSQL("select value from versions where program=" . SqlQuote($Product) . " and value=" . SqlQuote($Version) . "");
    my $Result = FetchOneColumn();
    if (lc($Result) eq lc($Version)) {
	return $Result;
    } else {
	return "";
    }
}

###############################################################
# Reply to a mail.
sub Reply {
    my ($OrgHeader, $Subject, $Text) = @_;
    my $Sender = "";
    my $MessageID= "";
    
    if ( $OrgHeader =~ /^Reply-To:\s+(.*?)\s*$/m ) {
	$Sender = $1;
    } elsif ( $OrgHeader =~ /^From:\s+(.*?)\s*$/m ) {
	$Sender = $1;
    } else {
	die "Cannot find sender-email-address";
    }

    if ( $OrgHeader =~ /^Message-Id:\s+(.*?)\s*$/m ) {
	$MessageID = $1;
    }
    if( $test ) {
	open( MAIL, ">>data/bug_email_test.log" );
    }
    else {
	open( MAIL, "| /usr/sbin/sendmail -t" );
    }

    print MAIL "To: $Sender\n";
    print MAIL "From: SuSE Bugzilla <bugmail\@suse.de>\n";
    print MAIL "Subject: $Subject\n";
    print MAIL "In-Reply-To: $MessageID\n" if ( $MessageID );
    print MAIL "\n";
    print MAIL "$Text";
    close( MAIL );

}


###############################################################
# getEnumList
# Queries the Database for the table description and figures the
# enum-settings out - usefull for checking fields for enums like
# prios 
sub getEnumList( $ )
{
    my ($fieldname) = @_;
    SendSQL( "describe bugs $fieldname" );
    my ($f, $type) = FetchSQLData();

    # delete unneeded stuff
    $type =~ s/enum\(|\)//g;
    $type =~ s/\',//g;
    # print "$type\n";

    my @all_prios = split( /\'/, $type );
    return( @all_prios );
}

###############################################################
# CheckPriority
# Checks, if the priority setting is one of the enums defined
# in the data base
# Uses the global var. $Control{ 'priority' }
sub CheckPriority
{
    my $prio = ($Control{'priority'} ||= "");
    my @all_prios = getEnumList( "priority" );

    if( (lsearch( \@all_prios, $prio ) == -1) || $prio eq "" ) {
	# OK, Prio was not defined - create Answer
	my $Text = "You sent wrong priority-setting, valid values are:" .
	    join( "\n\t", @all_prios ) . "\n\n";
	$Text .= "*  The priority is set to the default value ". 
	    SqlQuote( Param('defaultpriority')) . "\n";

	BugMailError( 0, $Text );

	# set default value from param-file
	$Control{'priority'} = Param( 'defaultpriority' );
    } else {
	# Nothing to do
    }
}

###############################################################
# CheckSeverity
# checks the bug_severity
sub CheckSeverity
{
    my $sever = ($Control{'bug_severity'} ||= "" );
    my @all_sever = getEnumList( "bug_severity" );

    if( (lsearch( \@all_sever, $sever ) == -1) || $sever eq "" ) {
	# OK, Prio was not defined - create Answer
	my $Text = "You sent wrong bug_severity-setting, valid values are:" .
	    join( "\n\t", @all_sever ) . "\n\n";
	$Text .= "*  The bug_severity is set to the default value ". 
	    SqlQuote( "normal" ) . "\n";

	BugMailError( 0, $Text );

	# set default value from param-file
	$Control{'bug_severity'} = "normal";
    } 
}

###############################################################
# CheckArea
# checks the area-field
sub CheckArea
{
    my $area = ($Control{'area'} ||= "" );
    my @all= getEnumList( "area" );

    if( (lsearch( \@all, $area ) == -1) || $area eq "" ) {
	# OK, Area was not defined - create Answer
	my $Text = "You sent wrong area-setting, valid values are:" .
	    join( "\n\t", @all ) . "\n\n";
	$Text .= "*  The area is set to the default value ". 
	    SqlQuote( "BUILD" ) . "\n";

	BugMailError( 0, $Text );

	# set default value from param-file
	$Control{'area'} = "BUILD";
    } 
}

###############################################################
# CheckPlatform
# checks the given Platform and corrects it
sub CheckPlatform
{
    my $platform = ($Control{'rep_platform'} ||= "" );
    my @all = getEnumList( "rep_platform" );

    if( (lsearch( \@all, $platform ) == -1) ||  $platform eq "" ) {
	# OK, Prio was not defined - create Answer
	my $Text = "You sent wrong platform-setting, valid values are:" .
	    join( "\n\t", @all ) . "\n\n";
	$Text .= "*  The rep_platform is set to the default value ". 
	    SqlQuote( "All" ) . "\n";

	BugMailError( 0, $Text );

	# set default value from param-file
	$Control{'rep_platform'} = "All";
    } 
}

###############################################################
# CheckSystem
# checks the given Op-Sys and corrects it
sub CheckSystem
{
    my $sys = ($Control{'op_sys'} ||= "" );
    my @all = getEnumList( "op_sys" );

    if(  (lsearch( \@all, $sys ) == -1) || $sys eq "" ) {
	# OK, Prio was not defined - create Answer
	my $Text = "You sent wrong OS-setting, valid values are:" .
	    join( "\n\t", @all ) . "\n\n";
	$Text .= "*  The op_sys is set to the default value ". 
	    SqlQuote( "Linux" ) . "\n";

	BugMailError( 0, $Text );

	# set default value from param-file
	$Control{'op_sys'} = "Linux";
    } 
}


###############################################################
# Fetches all lines of a query with a single column selected and
# returns it as an array
# 
sub FetchAllSQLData( )
{
    my @res = ();

    while( MoreSQLData() ){
	push( @res, FetchOneColumn() );
    }
    return( @res );
}

###############################################################
# Error Handler for Errors in the mail
# 
# This function can be called multiple within processing one mail and
# stores the errors found in the Mail. Errors are for example empty
# required tags, missing required tags and so on.
# 
# The benefit is, that the mail users get a reply, where all mail errors
# are reported. The reply mail includes all messages what was wrong and
# the second mail the user sends can be ok, cause all his faults where
# reported.
# 
# BugMailError takes two arguments: The first one is a flag, how heavy
# the error is:
# 
# 0 - Its an error, but bugzilla can process the bug. The user should
#     handle that as a warning.
# 
# 1 - Its a real bug. Bugzilla cant store the bug. The mail has to be
#     resent.
# 
# 2 - Permission error: The user does not have the permission to send
#     a bug.
# 
# The second argument is a Text which describs the bug.
# 
# 
# #
sub BugMailError($ $ )
{
    my ( $errflag, $text ) = @_;

    # On permission error, dont sent all other Errors back -> just quit !
    if( $errflag == 2 ) {            # Permission-Error
	Reply( $Header, "Bugzilla Error", "Permission denied.\n\n" .
	       "You do not have the permissions to create a new bug. Sorry.\n" );
	exit;
    }


    # Warnings - store for the reply mail
    if( $errflag == 0 ) {
	push( @mailwarnings, $text );
    }

    # Critical Error
    if( $errflag == 1 ) {
	$critical_err += 1;
	push( @mailerrors, $text );
    }
}

###############################################################
# getWarningText()
# 
# getWarningText() returns a reply-ready Textline of all the
# Warnings in the Mail
sub getWarningText()
{
    my $anz = @mailwarnings;

    my $ret = <<END
  
The Bugzilla Mail Interface found warnings (JFYI):

END
    ;

    # Handshake if no warnings at all
    return( "\n\n Your mail was processed without Warnings !\n" ) if( $anz == 0 );

    # build a text
    $ret .= join( "\n     ", @mailwarnings );
    return( horLine() . $ret );
}

sub getErrorText()
{
    my $anz = @mailerrors;

    my $ret = <<END

**************************  ERROR  **************************
 
Your request to the SuSE-bugzilla mail interface could not be met
due to errors in the mail. We will find it !


END
    ;
    return( "\n\n Your mail was processed without errors !\n") if( $anz == 0 );
    # build a text
    $ret .= join( "\n     ", @mailerrors );
    return( $ret );
}

###############################################################
# generateTemplate
# 
# This functiuon generates a mail-Template with the 
sub generateTemplate()
{
    my $w;
    my $ret;

    # Required Labels
    $ret =<<EOF


You may want to use this template to resend your mail. Please fill in the missing
keys.

_____ snip _______________________________________________________________________

EOF
    ;
    foreach ( @RequiredLabels ) {
	$w = "";
	$w = $Control{$_} if defined( $Control{ $_ } );
	$ret .= sprintf( "    \@%-15s:  %s\n", $_, $w );
    }

    $ret .= "\n";
    # Allowed Labels
    foreach( @AllowedLabels ) {
	next if( /reporter/    );  # Reporter is not a valid label
	next if( /assigned_to/ );  # Assigned to is just a number 
	if( defined( $Control{ $_ } ) && lsearch( \@RequiredLabels, $_ ) == -1 ) {
	    $ret .=  sprintf( "    \@%-15s:  %s\n", $_,  $Control{ $_ } );
	}
    }

    if( $Body eq "" ) {
    $ret .= <<END
	
   < the bug-description follows here >

_____ snip _______________________________________________________________________

END
    ; } else {
	$ret .= "\n" . $Body;
    }
	
    return( $ret );

}
###############################################################
# groupBitToString( $ )
# converts a given number back to the groupsetting-names
# This function accepts single numbers as added bits or 
# Strings with +-Signs
sub groupBitToString( $ )
{
    my ($bits) = @_;
    my $type;
    my @bitlist = ();
    my $ret = "";

    if( $bits =~ /^\d+$/ ) {  # only numbers
	$type = 1;

    } elsif( $bits =~ /^(\s*\d+\s*\+\s*)+/ ) { 
	$type = 2;
    } else {
	# Error: unknown format !
	$type = 0;
    }

    $bits =~ s/\s*//g;
    #
    # Query for groupset-Information
    SendSQL( "Select Bit,Name, Description from groups where isbuggroup=1" );
    my @line;
    while( MoreSQLData() ){
	@line = FetchSQLData();
	
	if( $type == 1 ) { 
	    if( ((0+$bits) & (0+$line[0])) == 0+$line[0] ) {
		$ret .= sprintf( "%s ", $line[1] );
	    }
	} elsif( $type == 2 ) {
	    if( $bits =~ /$line[0]/ ) {
		$ret .= sprintf( "%s ", $line[1] );
	    }
	}
    }

    return( $ret );
}



###############################################################
# Main starts here
###############################################################
foreach( @ARGV ) {
    $test = 1 if ( /-t/ );

}


while (<STDIN>) {
    last if /^$/;
    $Header .= $_;
}
while (<STDIN>) {
    if ( /^\s*\@description/i ) {
	while (<STDIN>) {
	    $Body .= $_;
	}

    } elsif ( /^\s*\@(.*?)(?:\s*=\s*|\s*:\s*|\s+)(.*?)\s*$/ ) {
	$Control{lc($1)} = $2;
    } else {
	$Body .= $_;
    }

}

# Join Header Lines:
while ( $Header =~ s/^(\S+:.*?)\n(\s+\S+.*?)$/$1$2/gm ) {}

# Find Sender:
my $Sender = "";
if ( $Header =~ /^Reply-To:\s+(.*?)\s*$/m ) {
    $Sender = $1;
} elsif ( $Header =~ /^From:\s+(.*?)\s*$/m ) {
    $Sender = $1;
} else {
    die "Cannot find sender-email-address";
}
my $SenderShort = $Sender;
$SenderShort =~ s/^.*?([a-zA-Z0-9_.-]+?\@[a-zA-Z0-9_.-]+\.[a-zA-Z0-9_.-]+).*$/$1/;



# Find Subject:
my $Subject = "";
if ( $Header =~ /^Subject:\s+(.*?)\s*$/m ) {
    $Subject = $1;
}
$Control{'short_desc'} ||= $Subject;


# Check Control-Labels
# not: reporter !
@AllowedLabels = ("product", "version", "rep_platform",
		  "bug_severity", "priority", "op_sys", "assigned_to",
		  "bug_status", "bug_file_loc", "short_desc", "component",
		  "status_whiteboard", "target_milestone", "groupset",
		  "qa_contact");
#my @AllowedLabels = qw{Summary priority platform assign};
foreach (keys %Control) {
    if ( lsearch( \@AllowedLabels, $_) < 0 ) {
	BugMailError( 0, "You sent a unknown label: " . $_ );
    }
}

push( @AllowedLabels, "reporter" );
$Control{'reporter'} = $SenderShort;

# Check required Labels - not all labels are required, because they could be generated
# from the given information
# Just send a warning- the error-Flag will be set later
@RequiredLabels = qw{product version component short_desc};
foreach my $Label (@RequiredLabels) {
    if ( ! defined $Control{$Label} ) {
	BugMailError( 0, "You were missing a required label: \@$Label\n" );
	next;
    }

    if( $Control{$Label} =~ /^\s*$/  ) {
	BugMailError( 0, "One of your required labels is empty: $Label" );
	next;
    }
}

if ( $Body =~ /^\s*$/s ) {
    BugMailError( 1, "You sent a completely empty body !" );
}


# umask 0;
ConnectToDatabase();

# Check Permissions ...
if (! CheckPermissions("CreateBugs", $SenderShort ) ) {
    BugMailError( 2, "Permission denied.\n\n"  .
		  "You do not have the permissions to create a new bug. Sorry.\n" );
}

# Set QA
SendSQL("select initialqacontact from components where program=" .
	SqlQuote($Control{'product'}) .
	" and value=" . SqlQuote($Control{'component'}));
my $qacontact = FetchOneColumn();
if (defined $qacontact && $qacontact !~ /^\s*$/) {
    #$Control{'qa_contact'} = DBNameToIdAndCheck($qacontact, 1);
    $Control{'qa_contact'} = DBname_to_id($qacontact);

    if ( ! $Control{'qa_contact'} ) {
	BugMailError( 0,  "Could not resolve qa_contact !\n" );
    }
    
    #push(@bug_fields, "qa_contact");
}

# Set Assigned - assigned_to depends on the product, cause initialowner 
#                depends on the product !
#                => first check product !
# Product
my @all_products = ();
my $Product = "";
$Product = CheckProduct( $Control{'product'} ) if( defined( $Control{ 'product'} ));

if ( $Product eq "" ) {
    my $Text = "You didnt send a value for the required key \@product !\n\n";

    $Text = "You sent the invalid product \"$Control{'product'}\"!\n\n"
	if( defined( $Control{ 'product'} ));

    $Text .= "Valid products are:\n\t";

    SendSQL("select product from products");
    @all_products = FetchAllSQLData();
    $Text .= join( "\n\t", @all_products ) . "\n\n";
    $Text .= horLine();

    BugMailError( 1, $Text );
} else {
    # Fill list @all_products, which is needed in case of component-help
    @all_products = ( $Product );
    $product_valid = 1;
}
$Control{'product'} = $Product;

#
# Check the Component:
#
my $Component = "";

if( defined( $Control{'component' } )) {
    $Component = CheckComponent( $Control{'product'}, $Control{'component'} );
}
    
if ( $Component eq "" ) {

    my $Text = "You did not send a value for the required key \@component!\n\n"; 

    if( defined( $Control{ 'component' } )) {
	$Text = "You sent the invalid component \"$Control{'component'}\" !\n";
    }

    #
    # Attention: If no product was sent, the user needs info for all components of all
    #            products -> big reply mail :)
    #            if a product was sent, only reply the components of the sent product
    my @val_components = ();
    foreach my $prod ( @all_products ) {
	$Text .= "\nValid components for product `$prod' are: \n\t";

	SendSQL("select value from components where program=" . SqlQuote( $prod ) . "");
	@val_components = FetchAllSQLData();

	$Text .= join( "\n\t", @val_components ) . "\n";
    }
    
    # Special: if there is a valid product, maybe it has only one component -> use it !
    # 
    my $amount_of_comps = @val_components;
    if( $product_valid  && $amount_of_comps == 1 ) {
	$Component = $val_components[0];
	
	$Text .= " * You did not send a component, but a valid product " . SqlQuote( $Product ) . ".\n";
	$Text .= " * This product has has only the one component ". SqlQuote(  $Component ) .".\n" .
		" * This component was set by bugzilla for submitting the bug.\n\n";
	BugMailError( 0, $Text ); # No blocker

    } else { # The component is really buggy :(
	$Text  .= horLine();
	BugMailError( 1, $Text );
    }
}
$Control{'component'} = $Component;


#
# Check assigned_to
# if no assigned_to was given, generate it from the product-DB
my $forceAssignedOK = 0;
if ( (! defined($Control{'assigned_to'}) ) 
     || $Control{'assigned_to'} =~ /^\s*$/ ) {
    SendSQL("select initialowner from components where program=" .
            SqlQuote($Control{'product'}) .
            " and value=" . SqlQuote($Control{'component'}));
    $Control{'assigned_to'} = FetchOneColumn();
    $forceAssignedOK = 1;
}


# Recode Names
$Control{'assigned_to'} = DBname_to_id($Control{'assigned_to'}, $forceAssignedOK);

if ( $Control{'assigned_to'} == 0 ) {
    my $Text = "Could not resolve key \@assigned_to !\n" .
	"If you do NOT send a value for assigned_to, the bug will be assigned to\n" .
	    "the qa-contact for the product and component.\n";
    $Text .= "This works only if product and component are OK. \n" 
	. horLine();

    BugMailError( 1, $Text );
}


$Control{'reporter'} = DBname_to_id($Control{'reporter'});
if ( ! $Control{'reporter'} ) {
    BugMailError( 1, "Could not resolve reporter !\n" );
}

### Set default values
CheckPriority( );
CheckSeverity( );
CheckPlatform( );
CheckSystem( );
# CheckArea();

### Check values ...
# Version
my $Version = "";
$Version = CheckVersion( $Control{'product'}, $Control{'version'} ) if( defined( $Control{'version'}));
if ( $Version eq "" ) {
    my $Text = "You did not send a value for the required key \@version!\n\n";

    if( defined( $Control{'version'})) {
	my $Text = "You sent the invalid version \"$Control{'version'}\"!\n";
    }

    my $anz_versions;
    my @all_versions;
    # Assemble help text
    foreach my $prod ( @all_products ) {
	$Text .= "Valid versions for product " . SqlQuote( $prod ) . " are: \n\t";

	SendSQL("select value from versions where program=" . SqlQuote( $prod ) . "");
	@all_versions = FetchAllSQLData();
	$anz_versions = @all_versions;
	$Text .= join( "\n\t", @all_versions ) . "\n" ; 

    }

    # Check if we could use the only version
    if( $anz_versions == 1 && $product_valid ) {
	$Version = $all_versions[0];
	# Fine, there is only one version string
	$Text .= " * You did not send a version, but a valid product " . SqlQuote( $Product ) . ".\n";
	$Text .= " * This product has has only the one version ". SqlQuote(  $Version) .".\n" .
	    " * This version was set by bugzilla for submitting the bug.\n\n";
	$Text .= horLine();
	BugMailError( 0, $Text ); # No blocker
    } else {
	$Text .= horLine();
	BugMailError( 1, $Text );
    }

}

$Control{'version'} = $Version;

# GroupsSet: Protections for Bug info. This paramter controls the visiblility of the 
# given bug. An Error in the given Buggroup is not a blocker, a default is taken.
#
# The GroupSet is accepted in three ways: As single number like 65536
# As added numbers like 65536 + 6 +8
# As literals linked with whitespaces, plus-signs or kommas
#
my $GroupSet = "";
$GroupSet = $Control{'groupset'} if( defined( $Control{ 'groupset' }));
#
# Fetch the default value for groupsetting
SendSQL("select bit from groups where name=" . SqlQuote( "ReadInternal" ));
my $default_group = FetchOneColumn();

if( $GroupSet eq "" ) {
    # To bad: Groupset does not contain anything -> set to default
    $GroupSet = $default_group;
    #
    # Give the user a hint
    my $Text = "You did not send a value for optional key \@groupset, which controls\n";
    $Text .= "the Permissions of the bug. It was set to a default value 'Internal Bug'\n";
    $Text .= "Probably the QA will change that if desired.\n";
    
    BugMailError( 0, $Text );
} elsif( $GroupSet =~ /^\d+$/ ) {
    # Numerical Groups (no +-signs), the GroupSet must be the sum of the bits
    # 
    my $grp_num = $GroupSet;
    # print "Numeric: $GroupSet\n";
    SendSQL("select bit from groups where isbuggroup=1 order by bit");
    my @Groups = FetchAllSQLData();
    
    # DANGEROUS: This code implies, that perl *CAN* cope with large numbers
    # Its probably better to allow only one default-group when mailing !
    my $Val = "0";
    foreach ( @Groups ) {
	# print 0+$grp_num & 0+$_ , "\n";
	if ( ( (0+$grp_num) & (0+$_) ) == $_ ) {
	    $Val .= sprintf( "+%d", $_ );
	}
    }

    if( $Val eq "0" ) { 
	# No valid group found
	my $Text = "The number you sent for the groupset of the bug was wrong.\n" .
	    "It was not the sum of valid bits, which are:\n\t";
	$Text .= join( "\n\t", @Groups ) . "\n";
	$Text .= "The groupset for your bug is set to default $default_group, which\n" .
	    "means 'ReadInternal'";
	
	BugMailError( 0, $Text );
	$GroupSet = $default_group;
    } else {
	$GroupSet = $Val;
    }
	
} elsif( $GroupSet =~ /^(\s*\d+\s*\+\s*)+/ ) {
    #
    # Groupset given as String with added numbers like 65536+131072
    # The strings are splitted and checked if the numbers are in the DB
    my @bits = split( /\s*\+\s*/, $GroupSet );
    my $new_groupset = "0";
    # Get all bits for groupsetting
    SendSQL("select bit from groups where isbuggroup=1" );
    my @db_bits = FetchAllSQLData();

    # ... and check, if the given bits and the one in the DB fit together
    foreach my $bit ( @bits ) {
	# print "The Bit is: $bit \n";
	if( lsearch( \@db_bits, $bit ) == -1 ) {
	    # Bit not found !
	    my $Text = "Checking the Group-Settings: You sent the Groupset-Bit $bit\n" .
		"which is not a valid Groupset-Bit. It will be scipped !\n\n";
	    BugMailError( 0, $Text );
	} else {
	    # Cool bit, add to the result-String
	    $new_groupset .= "+" . $bit;
	}
    }

    # Is the new-String larger than 0
    if( $new_groupset eq "0" ) {
	$new_groupset = $default_group;
	
	my $Text = "All given Groupsetting-Bits are invalid. Setting Groupsetting to\n" .
	    "default-Value $new_groupset, what means 'ReadInternal'\n\n";
	
	BugMailError( 0, $Text );
    }
    # Restore to Groupset-Variable
    $GroupSet = $new_groupset;

} else {
    # literal  e.g. 'ReadInternal'
    my $Value = "0";
    my $gserr = 0;
    my $Text = "";

    #
    # Split literal Groupsettings either on Whitespaces, +-Signs or ,
    # Then search for every Literal in the DB - col name
    foreach ( split /\s+|\s*\+\s*|\s*,\s*/, $GroupSet ) {
	SendSQL("select bit, Name from groups where name=" . SqlQuote($_));
	my( $bval, $bname ) = FetchSQLData();

	if( defined( $bname ) && $_ eq $bname ) {
	    $Value .= sprintf( "+%d", $bval );
	} else {
	    $Text .= "You sent the wrong GroupSet-String $_\n";
	    $gserr = 1;
	}
    }
    #
    # Give help if wrong GroupSet-String came
    if( $gserr > 0 ) {
	# There happend errors 
	$Text .= "Here are all valid literal Groupsetting-strings:\n\t";
	SendSQL( "select name from groups where isbuggroup=1" );
	$Text .= join( "\n\t", FetchAllSQLData()) . "\n";
	BugMailError( 0, $Text );
    }
		
    #
    # Check if anything was right, if not -> set default
    if( $Value eq "0" ) {
	$Value = $default_group;
	$Text .= "\nThe group will be set to $default_group, what means 'ReadInternal'\n\n";
    }
    
    $GroupSet = $Value;
} # End of checking groupsets

$Control{'groupset'} = $GroupSet;

# ###################################################################################
# Checking is finished
#

# Check used fields
my @used_fields;
foreach my $f (@AllowedLabels) {
    if ((exists $Control{$f}) && ($Control{$f} !~ /^\s*$/ )) {
        push (@used_fields, $f);
    }
}

#
# Creating the query for inserting the bug
# -> this should only be done, if there was no critical error before
if( $critical_err == 0 )
{
    
    my $reply = <<END
  
  +---------------------------------------------------------------------------+
           B U G Z I L L A -  M A I L -  I N T E R F A C E 
  +---------------------------------------------------------------------------+

  Your Bugzilla Mail Interface request was successfull.

END
;

    $reply .= "Your Bug-ID is ";

    my $query = "insert into bugs (\n" . join(",\n", @used_fields ) . 
	", bug_status, creation_ts, long_desc ) values ( ";
    
    my $tmp_reply = "These values were stored by bugzilla:\n";
    my $val;
    foreach my $field (@used_fields) {
	$query .= SqlQuote($Control{$field}) . ",\n";
	
	$val = $Control{ $field };
	
	$val = DBID_to_name( $val ) if( $field =~ /reporter|assigned_to|qa_contact/ );
	$val = groupBitToString( $val ) if( $field =~ /groupset/ );

        # print "Bitfield: $val\n";

	$tmp_reply .= sprintf( "     \@%-15s = %-15s\n", $field, $val );
    }

    $tmp_reply .= "      ... and your error-description !\n";

    my $comment = $Body;
    $comment =~ s/\r\n/\n/g;     # Get rid of windows-style line endings.
    $comment =~ s/\r/\n/g;       # Get rid of mac-style line endings.
    $comment = trim($comment);

    $query .=  SqlQuote( "NEW" ) . ", now(), " . SqlQuote($comment) . " )\n";


     #my %ccid3s;
     #if (defined $::FORM{'cc'}) {
     #    foreach my $person (split(/[ ,]/, $::FORM{'cc'})) {
     #        if ($person ne "") {
     #            $ccids{DBNameToIdAndCheck($person)} = 1;
     #        }
     #    }
     #}
     #foreach my $person (keys %ccids) {
     #    SendSQL("insert into cc (bug_id, who) values ($id, $person)");
     #}
    my $id;

    if( ! $test ) {
	SendSQL($query);

	SendSQL("select LAST_INSERT_ID()");
	$id = FetchOneColumn();


	# Cool, the mail was successfull
	system("./processmail", $id, "$Sender");
    } else {
	$id = "<just testing>";
	print "\n-------------------------------------------------------------------------\n";
	print "$query\n";
    }
    $reply .= $id . "\n\n" . $tmp_reply . "\n" . getWarningText();

    # print $reply;
    # print getWarningText();

    Reply( $Header,"Bugzilla success (ID $id)", $reply );
    
} else {
    # There were critical errors in the mail - the bug couldnt be inserted. !
my $errreply = <<END
  
  +---------------------------------------------------------------------------+
          B U G Z I L L A -  M A I L -  I N T E R F A C E             
  +---------------------------------------------------------------------------+

END
    ;

    # print "These are the errors: \n ===============================================================\n";
    $errreply .= getErrorText() . getWarningText() . generateTemplate();

    Reply( $Header, "Bugzilla Error", $errreply );

    # print getErrorText();
    # print getWarningText();
    # print generateTemplate();
}





exit;

