/***********************************************************
 *	uname - returns name of OS
 **********************************************************/
#include <windows.h>
#include <stdlib.h> 
#include <stdio.h>

#define	index(s, c)	strchr((s), (c))
#define streq(a, b) (strcmp ((a), (b)) == 0)

static char ** getlistOfOptions (char **pArgv, char * pOptions);
static void getUnameOption ( char option );

/***********************************************************************
 * -a    Print all information.
 *
 * -m    Print the machine hardware name.
 *
 * -n    Print the nodename (the nodename is the name by  which
 *      the system is known to a communications network).
 *
 * -p    Print the current host's processor type.
 *
 * -r    Print the operating system release.
 *
 * -s    Print the name of the operating system.  This  is  the
 *      default.
 *
 * -v    Print the operating system version.
 *
 * -S systemname
 *      The nodename may be changed by specifying a system name
 *      argument.   The  system  name argument is restricted to
 *      SYS_NMLN characters.   SYS_NMLN  is  an  implementation
 *      specific  value  defined  in <sys/utsname.h>.  Only the
 *      super-user is allowed this capability.
 ***********************************************************************/

main( int argc, char *argv[ ], char *envp[ ] )
{
	char options[1024];
	char *pOptions;
	char **pArgv;
 	char c;

	pArgv = &argv[1];
	pArgv = getlistOfOptions (pArgv, options);

	/* if no options specified, then return operating system */
	pOptions = options;
	if ( *pOptions == '\0' ) {
		options[0] = 's';
		options[1] = '\0';
	}
	while (c = *pOptions++ ) {
		if ( c == 'a' )	{
			getUnameOption ( 's' );
			printf (" " );
			getUnameOption ( 'n' );
			printf (" " );
			getUnameOption ( 'r' );
			printf (" " );
			getUnameOption ( 'v' );
			printf (" " );
			getUnameOption ( 'm' );
			printf (" " );
			getUnameOption ( 'p' );
		} else
			getUnameOption ( c );
	}
	return 0;
}

static char **
getlistOfOptions (char **pArgv, char * pOptions)
{
	char *pArg;
	while ( *pArgv && (**pArgv == '-') ) {
		pArg = *pArgv;
		pArg++;		/* skip over '-' */
		while ( *pArg )
			*pOptions++ = *pArg++;
		pArgv++;
	}
	*pOptions = '\0';
	return pArgv;
}
void getUnameOption ( char option ) 
{
	char computerName[128];
	SYSTEM_INFO systemInfo;
	OSVERSIONINFO versionInfo;
	DWORD computerNameLength = sizeof(computerName); 

	GetSystemInfo( &systemInfo);
	GetComputerName( (LPTSTR)&computerName, &computerNameLength );
	versionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx( &versionInfo );

	switch ( option ) {
	case 'a':
		printf ( getenv	("OS") );
		break;
	case 'm':
		printf ( "xx" );
		break;
	case 'n':
		printf ( computerName );
		break;
	case 'p':
		switch ( systemInfo.wProcessorArchitecture ) {
		case PROCESSOR_ARCHITECTURE_INTEL:
			printf("I386");
			break;
		case PROCESSOR_ARCHITECTURE_MIPS:
			printf("MIPS");
			break;
		case PROCESSOR_ARCHITECTURE_ALPHA:
			printf("ALPHA");
			break;
		case PROCESSOR_ARCHITECTURE_PPC:
			printf("PPC");
			break;
		default:
			printf("UNKNOWN_PROCESSOR");
			break;
		}
		break;
	case 'r':
		printf ("%d.%d", versionInfo.dwMajorVersion, versionInfo.dwMinorVersion );
		break;
	case 's':
		switch ( versionInfo.dwPlatformId ) {
		case VER_PLATFORM_WIN32s:
			printf("WIN32S");
			break;
		case VER_PLATFORM_WIN32_WINDOWS:
			printf("WIN95");
			break;
		case VER_PLATFORM_WIN32_NT:
			printf("WINNT");
			break;
		default:
			printf("UNKNOWN_OS");
			break;
		} 
		break;
	case 'v':
		printf ("%d", versionInfo.dwBuildNumber );
		break;
	default:
		printf ( "uname -%c is unimplemented", option );
		break;
	}
}
