/***********************************************************
 *	shmsdos - unix shell for msdos
 **********************************************************/
#include <process.h>    
#include <direct.h>    
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <io.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <windows.h>
#pragma hdrstop

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE !FALSE
#endif

#define	index(s, c)	strchr((s), (c))
#define streq(a, b) (strcmp ((a), (b)) == 0)

#define SH_CMD_CD		0
#define SH_CMD_EVAL		1
#define SH_CMD_EXEC		2
#define SH_CMD_EXIT		3
#define SH_CMD_LOGIN	4
#define SH_CMD_LOGOUT	5
#define SH_CMD_SET		6
#define SH_CMD_UMASK	7
#define SH_CMD_WAIT		8
#define SH_CMD_WHILE	9
#define SH_CMD_FOR		10
#define SH_CMD_CASE		11
#define SH_CMD_IF		12
#define SH_CMD_COLON	13
#define SH_CMD_PERIOD	14
#define SH_CMD_BREAK	15
#define SH_CMD_CONTINUE	16
#define SH_CMD_EXPORT	17
#define SH_CMD_READ		18
#define SH_CMD_READONLY	19
#define SH_CMD_SHIFT	20
#define SH_CMD_TIMES	21
#define SH_CMD_TRAP		22
#define SH_CMD_SWITCH	23
#define SH_CMD_MKDIR	24
#define SH_CMD_ECHO		25
#define SH_CMD_CP		26
#define SH_CMD_RM		27
#define SH_CMD_TOUCH	28
#define SH_CMD_NSINSTALL 29
#define SH_CMD_TEST		30
#define SH_CMD_CAT      31
#define SH_CMD_TRUE	32
#define SH_CMD_MV       33
	
#define COND_WHILE		0
#define COND_FOR		1
#define COND_CASE		2
#define COND_IF			3
#define COND_BREAK		4
#define COND_CONTINUE	5
#define COND_SHIFT		6
#define COND_TIMES		7
#define COND_SWITCH		8
#define COND_FI			9
#define COND_THEN		10
#define COND_ELSE		11
#define COND_DONE		12
#define COND_DO			13

/*
 * sh_FileFcn --
 *
 * A function that operates on a file.  The pathname is either
 * absolute or relative to the current directory, and contains
 * no wildcard characters such as * and ?.   Additional arguments
 * can be passed to the function via the arg pointer.
 */

typedef BOOL (*sh_FileFcn)(
        char *pathName,
        WIN32_FIND_DATA *fileData,
        void *arg);

//static char sh_chars[] = "#;\"*?[]&|<>(){}$`^";
static char sh_chars[] = ";";
static char *conditional_cmds[] = { "while", "for",
		     "case", "if", "break", "continue",
		     "shift", "times", "switch", "fi", "then", "else", "done", "do", 0 };

static char *sh_cmds[] = { "cd", "eval", "exec", "exit", "login",
		     "logout", "set", "umask", "wait", "while", "for",
		     "case", "if", ":", ".", "break", "continue",
		     "export", "read", "readonly", "shift", "times",
		     "trap", "switch", "mkdir", "echo", "cp", "rm",
		      "touch", "nsinstall", "test", "cat", "true", "mv", 0 };

static char ** getNextCommandArgv (char **line, char * pMacro, char * pSubstitution);
static int doBuiltinShellCommand (char *argv[ ], char **envp);
static char ** getlistOfOptions (char **pArgv, char * pOptions);
static int parseShellLine ( char** pointerIntoLine, char *argv[ ], char *envp[ ],
							char * pMacro, char * pSubstitution );
static void freeArgv ( char *argv[ ] ); 
static int shellCp (char **pArgv); 
static int shellEcho (char **pArgv);
static int shellRm (char **pArgv);
static int shellNsinstall (char **pArgv);
static int shellMkdir (char **pArgv); 
static int shellTest (char **pArgv); 
static int shellCat (char **pArgv); 
static int shellMv (char **pArgv);
static BOOL sh_EnumerateFiles(const char *pattern, const char *where,
        sh_FileFcn fileFcn, void *arg, int *nFiles);
static const char *sh_GetLastErrorMessage(void);
static BOOL sh_DoCopy(char *srcFileName, DWORD srcFileAttributes,
        char *dstFileName, DWORD dstFileAttributes,
        int force, int recursive);

static int waitForDebug = 1;

char * next_token( char *p)
{
	while ( *p == ' ' || *p == '\t' ) 
		++p;
	return p;
}

static char* convertArgvIntoCommandLine( int argc, char *argv[ ] )
{
	char *pLine;
	char *p;
	int argi;
	int totalLength = 0;

	/* get total # of characters in command line */
	argi = 2;
	while ( argi < argc ) {
		if ( argv[argi] )
			totalLength += strlen (	argv[argi] ) + 1;
		argi++;
	}
	
	/* allocate memory for the command line  */
  	pLine = (char *) malloc (totalLength * sizeof (char *));
	if ( pLine == NULL ) {
		printf ("Not enough memory in SHMSDOS\n");
		exit(1);
	}	
	/* compress all tokens into single command line */
	argi = 2;
	p = pLine;
	while ( argi < argc ) {
		char *pArgv = argv[argi];
		while ( *pArgv )
			*p++ = *pArgv++;
		argi++;
		*p++ = ' ';
	}

	/* remove trailing blanks */
	while ( (p > pLine) &&  (*(p-1) == ' ' ) )
		*(--p) = '\0';

	return pLine;
}

main( int argc, char *argv[ ], char *envp[ ] )
{
	int status;
	char* pointerIntoLine;
	char **newArgv;

	char * pLine = convertArgvIntoCommandLine( argc, argv );
	pointerIntoLine = pLine;

	while ( TRUE ) {
		newArgv = getNextCommandArgv ( &pointerIntoLine, NULL, NULL );
		if ( newArgv == 0 )
			break;

		status = parseShellLine ( &pointerIntoLine, newArgv, envp, NULL, NULL );
		freeArgv ( newArgv ); 
		if ( status != 0 )
			break;
	}

	/* process each shell command */
	free ( pLine );
	return status;
}

	/* process each shell command */
int getConditionalCommand ( char * shellName )
{		
	int shellCommand = -1;
	int j;

		/* search for shell name */
		for (j = 0; conditional_cmds[j] != 0; ++j) {
			if ( streq (conditional_cmds[j], shellName) ) {
				shellCommand = j;
				break;
			}
		}
	return shellCommand;
}

/*     
  if list then list [ elif list then list ] ... [ else list ] fi
	The list following if is executed and, if it returns a zero exit
	status, the list following the first then is executed.  Otherwise,
	the list following elif is executed and, if its value is zero, the
	list following the next then is executed.  Failing that, the else
	list is executed.  If no else list or then list is executed, then
	the if command returns a zero exit status.
 */
int parseIfCommand ( char** pointerIntoLine, char *argv[ ], char *envp[ ],
					 char * pMacro, char * pSubstitution )
{		
	int ifTestedTrue;
	int status = 0;
	int shellCommand;
	char **pArgv;

	/* check test condition */
//	while ( waitForDebug );
	ifTestedTrue = doBuiltinShellCommand ( argv, envp );
	if ( (ifTestedTrue != 0) && (ifTestedTrue != 1)	) {
		printf ( "Error executing command %s while processing 'if'\n", argv[0] );
		return ifTestedTrue;
	}
	ifTestedTrue = !ifTestedTrue; 

	while ( TRUE ) {

		char **newArgv = getNextCommandArgv ( pointerIntoLine, pMacro, pSubstitution );
		if ( newArgv == 0 ) {
			status = -1;
			break;
		}
		pArgv = newArgv;

		/* get next shell command */
		shellCommand = getConditionalCommand ( newArgv[0] );
		if ( shellCommand == COND_FI ) {
			status = 0;
			freeArgv ( newArgv ); 
			break;
	 	} else if ( shellCommand == COND_THEN ) {
			pArgv++;
	 	} else if ( shellCommand == COND_ELSE ) {
			pArgv++;
			ifTestedTrue = !ifTestedTrue;
	 	}
		if ( ifTestedTrue )
			status = parseShellLine ( pointerIntoLine, pArgv, envp, pMacro, pSubstitution );
		else
			status = 0;
		freeArgv ( newArgv ); 
		if ( status != 0 )
			break;
	}

	return status;
}
/*
	for name [ in word ... ] do list done
	  Each time a for command is executed, name is set to the next word
	  taken from the in word list.  If in word ...  is omitted, then the
	  for command executes the do list once for each positional parameter
	  that is set (see Parameter Substitution below).  Execution ends when
	  there are no more words in the list.
 */
int parseForCommand ( char** pointerIntoLine, char *argv[ ], char *envp[ ] )
{		
	int status = 0;
	int shellCommand;
	char **pArgv = argv;
	char * pMacro;
	char ** pWord;
	char* doPointerIntoLine;

	// get name of macro
	pMacro = *pArgv++;
	
	if ( strcmp ( "in" , *pArgv ) != 0 ) {
		exit (1);
	}
	pArgv++;
	pWord = pArgv;
	
		

	/* check test condition */
//	while ( waitForDebug );
	doPointerIntoLine = *pointerIntoLine;
	while ( *pWord ) {

		while ( TRUE ) {
			char **newArgv = getNextCommandArgv ( pointerIntoLine, pMacro, *pWord );
			if ( newArgv == 0 ) {
				status = -1;
				break;
			}
			pArgv = newArgv;

			shellCommand = getConditionalCommand ( *pArgv );
			if ( shellCommand == COND_DO ) {
				pArgv++;
			}

			/* get next shell command */
			if ( shellCommand == COND_DONE ) {
				status = 0;
				freeArgv ( newArgv ); 
				break;
		 	}
			status = parseShellLine ( pointerIntoLine, pArgv, envp, pMacro, *pWord );
			freeArgv ( newArgv ); 
			if ( status != 0 )
				break;
		}
		pWord++;
		if ( status != 0 )
			break;

		/* if another word exists in list, then repeat */
		if ( *pWord )
			*pointerIntoLine = doPointerIntoLine;
	}

	return status;
}
/* process each shell command */
int parseShellLine ( char** pointerIntoLine, char *argv[ ], char *envp[ ],
					char * pMacro, char * pSubstitution )
{		
	int shellCommand;
	char **pArgv;
	int status;

	/* search for shell name */
	shellCommand = getConditionalCommand ( argv[0] );

	/* process shell command */

	if ( shellCommand == -1 ) {
		status = doBuiltinShellCommand ( argv, envp );
	} else {
		pArgv = &argv[1];
		switch ( shellCommand ) {
		case COND_IF:
			status = parseIfCommand ( pointerIntoLine, pArgv, envp, pMacro, pSubstitution );
			break;
		case COND_FOR:
			status = parseForCommand ( pointerIntoLine, pArgv, envp );
			break;
		default:
			printf ( "Unexpected conditional command: -%s-\n", argv[0] );
			status = -1;
			break;
		}
	}
	return status;
}

/* Figure out the argument list necessary to run LINE as a command.  Try to
   avoid using a shell.  This routine handles only ' quoting, and " quoting
   when no backslash, $ or ` characters are seen in the quotes.  Starting
   quotes may be escaped with a backslash.  If any of the characters in
   sh_chars[] is seen, or any of the builtin commands listed in sh_cmds[]
   is the first word of a line, the shell is used.

   SHELL is the shell to use, or nil to use the default shell.
   IFS is the value of $IFS, or nil (meaning the default).  */

static char **
getNextCommandArgv (char **pLine, char * pMacro, char * pSubstitution )
{
  register int i;
  register int lineLength;
  register char *pp;
  register char *p;
  register char *ap;
  char *line = *pLine;
  char *end;
  int instring, word_has_equals, seen_nonequals;
  char **new_argv = 0;
  int foundStartOfLine;
  int iSubstitutionLen;

  /* Make sure not to bother processing an empty line.  */
	foundStartOfLine = FALSE;
 	while ( !foundStartOfLine ) {
	  	switch ( *line ) {
		case ';':
		case '\t':
		case ' ':
	    	++line;
			break;

		case '\\':
			if ( *(line+1) == 't') {
		    	line += 2;
				break;
			}

		default:
			foundStartOfLine = TRUE;
			break;
		}
	}												

  if (*line == '\0')
    return 0;

	/* get maximum length of length with arguments substituted */
  	i = strlen (line) + 1;
	lineLength = i;


  /* More than 1 arg per character is impossible.  */
  new_argv = (char **) malloc (i * sizeof (char *));
  if ( new_argv == NULL ) {
  	printf ("Not enough memory in SHMSDOS\n");
	exit(1);
  }	
 
  /* increase size of parameters by macro substitution */
  pp = line;
  if ( pSubstitution ) {
	iSubstitutionLen = strlen ( pSubstitution ) - strlen ( pMacro ) - 1;
	while ( *pp ) {
		if ( *pp++ == '$' )
			lineLength += iSubstitutionLen;
	}
  }

  /* All the args can fit in a buffer as big as LINE is.   */
  ap = new_argv[0] = (char *) malloc (lineLength);
  if ( ap == NULL ) {
  	printf ("Not enough memory in SHMSDOS\n");
	exit(1);
  }	
  end = ap + lineLength;

  /* I is how many complete arguments have been found.  */
  i = 0;
  instring = word_has_equals = seen_nonequals = 0;
  for (p = line; *p != '\0'; ++p)
    {
      if (ap > end)
        {
          fprintf(stderr, "Internal error: stepping past the end of buffer, "
              "at %s, line %d\n", __FILE__, __LINE__);
          abort ();
        }

      if (instring)
	{
	string_char:
	  /* Inside a string, just copy any char except a closing quote
	     or a backslash-newline combination.  */
	  if (*p == instring)
	    instring = 0;
	  else if (*p == '\\' && p[1] == '\n')
	    goto swallow_escaped_newline;
	  /* Backslash, $, and ` are special inside double quotes.
	     If we see any of those, punt.  */
	  else if (instring == '"' && *p == '\\'
	      && strchr("\\$`\"", p[1]) != NULL)
	  /* The backslash can be used to escape another special
	     character inside double quotes.  */
	    *ap++ = *++p;
	  else if (instring == '"' && *p == '$')
	    goto macroSubstitution;
	  else
	    *ap++ = *p;
	}
      else if (index (sh_chars, *p) != 0)
	/* Not inside a string, but it's a special char.  */
	goto slow;
      else
	/* Not a special char.  */
	switch (*p)
	  {
	  case '=':
	    /* Equals is a special character in leading words before the
	       first word with no equals sign in it.  This is not the case
	       with sh -k, but we never get here when using nonstandard
	       shell flags.  */
	    if (! seen_nonequals)
	      goto slow;
	    word_has_equals = 1;
	    *ap++ = '=';
	    break;

	  case '\\':
	    /* Backslash-newline combinations are eaten.  */
	    if (p[1] == '\n')
	      {
	      swallow_escaped_newline:

		/* Eat the backslash, the newline, and following whitespace,
		   replacing it all with a single space.  */
		p += 2;

		/* If there is a tab after a backslash-newline,
		   remove it from the source line which will be echoed,
		   since it was most likely used to line
		   up the continued line with the previous one.  */
		if (*p == '\t')
		  strcpy (p, p + 1);

		if (instring)
		  goto string_char;
		else
		  {
		    if (ap != new_argv[i])
		      /* Treat this as a space, ending the arg.
			 But if it's at the beginning of the arg, it should
			 just get eaten, rather than becoming an empty arg. */
		      goto end_of_arg;
		    else
		      p = next_token (p) - 1;
		  }
	      }
	    else if (p[1] != '\0')
	      /* Copy and skip the following char.  */
	      *ap++ = *++p;
//	      *ap++ = *p;
	    break;

	  case '\'':
	  case '"':
	    instring = *p;
	    break;

	  case '\n':
	      /* Newlines are not special.  */
	      *ap++ = '\n';
	    break;

	  case '$':
macroSubstitution:
			if ( pMacro ) {
				int len = strlen ( pMacro );
				if ( strncmp ( pMacro, p+1, len ) == 0 ) {
					p += len;
					strcpy ( ap, pSubstitution );
					ap += strlen ( pSubstitution );
					break;
				}
			}
	  		
	      /* Newlines are not special.  */
	    *ap++ = *p;
	    break;

	  case ' ':
	  case '\t':
	  end_of_arg:
	    /* We have the end of an argument.
	       Terminate the text of the argument.  */
	    *ap++ = '\0';
	    new_argv[++i] = ap;

	    /* Update SEEN_NONEQUALS, which tells us if every word
	       heretofore has contained an `='.  */
	    seen_nonequals |= ! word_has_equals;
	    if (word_has_equals && ! seen_nonequals)
	      /* An `=' in a word before the first
		 word without one is magical.  */
	      goto slow;
	    word_has_equals = 0; /* Prepare for the next word.  */

	    /* Ignore multiple whitespace chars.  */
	    p = next_token (p);
	    /* Next iteration should examine the first nonwhite char.  */
	    --p;
	    break;

	  default:
	    *ap++ = *p;
	    break;
	  }
    }

  /* Terminate the last argument and the argument list.  */

slow:;
  *ap = '\0';
  if (new_argv[i][0] != '\0')
    ++i;
  new_argv[i] = 0;

	*pLine = p;		/* return pointer into line */
	return new_argv;

}

static void
freeArgv ( char *argv[ ] ) 
{
	free (argv[0]);
	free (argv);
}

/* changes all forward slashes in token to back slashes */
void changeForwardSlashesTpBackSlashes ( char *arg )
{
	if ( arg == NULL )
		return;

	/* get options for command */
	while ( *arg ) {
		if ( *arg == '/' )
			*arg = '\\';
		arg++;			
	}
}

/*
 * The various spawn functions in Microsoft's runtime library
 * may fail if the Path environment variable contains nonexistent
 * directories.  The spawn functions expect to get ENOENT in
 * this situation, but on some Compaq PCs I've seen EACCES
 * ("Permission denied") being returned.  So I wrote our own
 * process-spawning function ns_spawn().  My code is inspired
 * by Microsoft's implementation of spawnvpe().
 */

/*
 * Assemble the command line by concatenating the argv array.
 * On success, this function returns 0 and the resulting command
 * line is returned in *cmdLine.  On failure, it returns -1.
 */
static int assembleCmdLine(char **argv, char **cmdLine)
{
    char **arg;
    char *p, *q;
    int cmdLineSize;
    int numBackslashes;
    int i;
    int argNeedQuotes;

    /*
     * Find out how large the command line buffer should be.
     */
    cmdLineSize = 0;
    for (arg = argv; *arg; arg++) {
        /*
         * \ and " need to be escaped by a \.  In the worst case,
         * every character is a \ or ", so the string of length
         * may double.  If we quote an argument, that needs two ".
         * Finally, we need a space between arguments, and
         * a null byte at the end of command line.
         */
        cmdLineSize += 2 * strlen(*arg)  /* \ and " need to be escaped */
                + 2                      /* we quote every argument */
                + 1;                     /* space in between, or final null */
    }
    p = *cmdLine = malloc(cmdLineSize);
    if (p == NULL) {
        return -1;
    }

    for (arg = argv; *arg; arg++) {
        /* Add a space to separates the arguments */
        if (arg != argv) {
            *p++ = ' '; 
        }
        q = *arg;
        numBackslashes = 0;
        argNeedQuotes = 0;

        /* If the argument contains white space, it needs to be quoted. */
        if (strpbrk(*arg, " \f\n\r\t\v")) {
            argNeedQuotes = 1;
        }

        if (argNeedQuotes) {
            *p++ = '"';
        }
        while (*q) {
            if (*q == '\\') {
                numBackslashes++;
                q++;
            } else if (*q == '"') {
                if (numBackslashes) {
                    /*
                     * Double the backslashes since they are followed
                     * by a quote
                     */
                    for (i = 0; i < 2 * numBackslashes; i++) {
                        *p++ = '\\';
                    }
                    numBackslashes = 0;
                }
                /* To escape the quote */
                *p++ = '\\';
                *p++ = *q++;
            } else {
                if (numBackslashes) {
                    /*
                     * Backslashes are not followed by a quote, so
                     * don't need to double the backslashes.
                     */
                    for (i = 0; i < numBackslashes; i++) {
                        *p++ = '\\';
                    }
                    numBackslashes = 0;
                }
                *p++ = *q++;
            }
        }

        /* Now we are at the end of this argument */
        if (numBackslashes) {
            /*
             * Double the backslashes if we have a quote string
             * delimiter at the end.
             */
            if (argNeedQuotes) {
                numBackslashes *= 2;
            }
            for (i = 0; i < numBackslashes; i++) {
                *p++ = '\\';
            }
        }
        if (argNeedQuotes) {
            *p++ = '"';
        }
    } 

    *p = '\0';
    return 0;
}

/*
 * Assemble the environment block by concatenating the envp array
 * (preserving the terminating null byte in each array element)
 * and adding a null byte at the end.
 *
 * Returns 0 on success.  The resulting environment block is returned
 * in *envBlock.  Note that if envp is NULL, a NULL pointer is returned
 * in *envBlock.  Returns -1 on failure.
 */
static int assembleEnvBlock(char **envp, char **envBlock)
{
    char *p;
    char *q;
    char **env;
    char *curEnv;
    char *cwdStart, *cwdEnd;
    int envBlockSize;

    if (envp == NULL) {
        *envBlock = NULL;
        return 0;
    }

    curEnv = GetEnvironmentStrings();

    cwdStart = curEnv;
    while (*cwdStart) {
        if (cwdStart[0] == '=' && cwdStart[1] != '\0'
                && cwdStart[2] == ':' && cwdStart[3] == '=') {
            break;
        }
        cwdStart += strlen(cwdStart) + 1;
    }
    cwdEnd = cwdStart;
    if (*cwdEnd) {
        cwdEnd += strlen(cwdEnd) + 1;
        while (*cwdEnd) {
            if (cwdEnd[0] != '=' || cwdEnd[1] == '\0'
                    || cwdEnd[2] != ':' || cwdEnd[3] != '=') {
                break;
            }
            cwdEnd += strlen(cwdEnd) + 1;
        }
    }
    envBlockSize = cwdEnd - cwdStart;

    for (env = envp; *env; env++) {
        envBlockSize += strlen(*env) + 1;
    }
    envBlockSize++;

    p = *envBlock = malloc(envBlockSize);
    if (p == NULL) {
        FreeEnvironmentStrings(curEnv);
        return -1;
    }

    q = cwdStart;
    while (q < cwdEnd) {
        *p++ = *q++;
    }
    FreeEnvironmentStrings(curEnv);

    for (env = envp; *env; env++) {
        q = *env;
        while (*q) {
            *p++ = *q++;
        }
        *p++ = '\0';
    }
    *p = '\0';
    return 0;
}

/*
 * For qsort.  We sort (case-insensitive) the environment strings
 * before generating the environment block.
 */
static int compare(const void *arg1, const void *arg2)
{
    return _stricmp(* (char**)arg1, * (char**)arg2);
}

/*
 * Spawn a new process with arguments argv and environment envp,
 * wait until it terminates, and return its exit code in *exitCode.
 * Return 0 on success.  Return -1 on error.  Call GetLastError()
 * (note: not errno) to get the error code.
 */
static int ns_spawn(char **argv, char **envp, int *exitCode)
{
    char *cmdLine;
    char *envBlock;
    char **newEnvp;
    BOOL rv;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    changeForwardSlashesTpBackSlashes(argv[0]);
    if (assembleCmdLine(argv, &cmdLine) == -1) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return -1;
    }

    if (envp == NULL) {
        newEnvp = NULL;
    } else {
        int i;
        int numEnv = 0;
        while (envp[numEnv]) {
            numEnv++;
        }
        newEnvp = (char **) malloc((numEnv+1) * sizeof(char *));
        for (i = 0; i <= numEnv; i++) {
            newEnvp[i] = envp[i];
        }
        qsort((void *) newEnvp, (size_t) numEnv, sizeof(char *), compare);
    }
    if (assembleEnvBlock(newEnvp, &envBlock) == -1) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        free(cmdLine);
        return -1;
    }

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    rv = CreateProcess(
            NULL,
            cmdLine,
            NULL,
            NULL,
            TRUE,
            0,
            envBlock,
            NULL,
            &si,
            &pi);
    free(cmdLine);
    free(envBlock);
    if (!rv) {
        return -1;
    }
    CloseHandle(pi.hThread);
    WaitForSingleObject(pi.hProcess, INFINITE);
    if (exitCode) {
        GetExitCodeProcess(pi.hProcess, exitCode);
    }
    CloseHandle(pi.hProcess);
    return 0;
}

static int
doBuiltinShellCommand (char *argv[ ], char **envp)
{
	int retVal = 0;			/* assume valid return */
	char path[_MAX_PATH];
	char **pArgv;
	char *arg1;
//	char options[1024];
//	char *pOptions;
	int j;
	int fileHandle;
	char * shellName = *argv;
	int shellCommand = - 1;

	/* search for shell name */
	for (j = 0; sh_cmds[j] != 0; ++j) {
		if ( streq (sh_cmds[j], shellName) ) {
			shellCommand = j;
			break;
		}
	}

	pArgv = &argv[1];
	switch ( shellCommand ) {
	case SH_CMD_CD:
		while ( **pArgv == '-' )
			pArgv++;
		arg1 = *pArgv;
		changeForwardSlashesTpBackSlashes ( arg1 );
		_getcwd ( path, sizeof (path) );
		if ( _chdir ( arg1 ) == -1 ) {
			perror ( "Path not found" );
			retVal = 2;
			break;
		}
		_getcwd ( path, sizeof (path) );
		break;

	case SH_CMD_NSINSTALL:
//		while ( waitForDebug );
		retVal = shellNsinstall ( pArgv );
		break;


	case SH_CMD_MKDIR:
		retVal = shellMkdir ( pArgv );
		break;

	case SH_CMD_CP:
		retVal = shellCp ( pArgv );
		break;

	case SH_CMD_ECHO:
		retVal = shellEcho ( pArgv );
		break;

	case SH_CMD_RM:
		retVal = shellRm ( pArgv );
		break;

	case SH_CMD_TOUCH:
		while ( **pArgv == '-' )
			pArgv++;
		arg1 = *pArgv;
		changeForwardSlashesTpBackSlashes ( arg1 );
		fileHandle = _open( arg1, _O_CREAT );
		if( fileHandle == -1 ) {
			perror( "Open failed on input file" );
			retVal = 5;
		} else {
			_close( fileHandle );
		}
		break;

		/*
         * set [ --aefhkntuvx [ arg ... ] ]
         * -a   Mark variables which are modified or created for export.
         * -e   Exit immediately if a command exits with a non-zero exit
         *      status.
         * -f   Disable file name generation
         * -h   Locate and remember function commands as functions are defined
         *      (function commands are normally located when the function is
         *      executed).
         * -k   All keyword arguments are placed in the environment for a
         *      command, not just those that precede the command name.
         * -n   Read commands but do not execute them.
         * -t   Exit after reading and executing one command.
         * -u   Treat unset variables as an error when substituting.
         * -v   Print shell input lines as they are read.
         * -x   Print commands and their arguments as they are executed.
         * --   Do not change any of the flags; useful in setting $1 to -.
         * Using + rather than - causes these flags to be turned off.  These
         * flags can also be used upon invocation of the shell.  The current
         * set of flags may be found in $-.  The remaining arguments are
         * positional parameters and are assigned, in order, to $1, $2, ....
         * If no arguments are given the values of all names are printed.
		 */

	case SH_CMD_SET:
		arg1 = *pArgv++;
		//printf ( "set %s\n", arg1 );
		break;

	case SH_CMD_TEST:
		retVal = shellTest ( pArgv );
		break;

	case SH_CMD_CAT:
		retVal = shellCat ( pArgv );
		break;

	case SH_CMD_TRUE:
		break;

	case SH_CMD_MV:
		retVal = shellMv ( pArgv );
		break;

	default:
//		retVal = spawnvp (P_WAIT, argv[0], argv );
		if ( ns_spawn (argv, envp, &retVal) == -1 ) {
			fprintf (stderr, "SHMSDOS: %s: %s\n", argv[0],
					sh_GetLastErrorMessage ());
			retVal = -1;
		}
		break;
	}
	return retVal;
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

static int
shellNsinstall (char **pArgv)
{
	int retVal = 0;		/* exit status */
	int dirOnly = 0;	/* 1 if and only if -D is specified */
	char **pSrc;
	char **pDst;

	/*
	 * Process the command-line options.  We ignore the
	 * options except for -D.  Some options, such as -m,
	 * are followed by an argument.  We need to skip the
	 * argument too.
	 */
	while ( *pArgv && **pArgv == '-' ) {
		char c = (*pArgv)[1];  /* The char after '-' */

		if ( c == 'D' ) {
			dirOnly = 1;
		} else if ( c == 'm' ) {
			pArgv++;  /* skip the next argument */
		}
		pArgv++;
	}

	if ( !dirOnly ) {
		/* There are files to install.  Get source files */
		if ( *pArgv ) {
			pSrc = pArgv++;
		} else {
			fprintf( stderr, "SHMSDOS: nsinstall: not enough arguments\n");
			return 3;
		}
	}

	/* Get to last token to find destination directory */
	if ( *pArgv ) {
		pDst = pArgv++;
		if ( dirOnly && *pArgv ) {
			fprintf( stderr, "SHMSDOS: nsinstall: too many arguments with -D\n");
			return 3;
		}
	} else {
		fprintf( stderr, "SHMSDOS: nsinstall: not enough arguments\n");
		return 3;
	}
	while ( *pArgv ) 
		pDst = pArgv++;

  	retVal = shellMkdir ( pDst );
	if ( retVal )
		return retVal;
	if ( !dirOnly )
		retVal = shellCp ( pSrc );
	return retVal;
}

static int
shellMkdir (char **pArgv) 
{
	int retVal = 0;			/* assume valid return */
	char *arg;
	char *pArg;
	char path[_MAX_PATH];
	char tmpPath[_MAX_PATH];
	char *pTmpPath = tmpPath;

	/* All the options are simply ignored in this implementation */
	while ( *pArgv && **pArgv == '-' ) {
		if ( (*pArgv)[1] == 'm' ) {
			pArgv++;  /* skip the next argument (mode) */
		}
		pArgv++;
	}

	while ( *pArgv ) {
		arg = *pArgv;
		changeForwardSlashesTpBackSlashes ( arg );
		pArg = arg;
		pTmpPath = tmpPath;
		while ( 1 ) {
			/* create part of path */
			while ( *pArg ) {
				*pTmpPath++ = *pArg++;
				if ( *pArg == '\\' )
					break;
			}
			*pTmpPath = '\0';

			/* check if directory alreay exists */
			_getcwd ( path, sizeof (path) );
			if ( _chdir ( tmpPath ) != -1 ) {
				_chdir ( path );
			} else {
				if ( _mkdir ( tmpPath ) == -1 ) {
//				while ( waitForDebug );
					printf ( "%s: ", tmpPath );
					perror ( "Could not create the directory" );
					retVal = 3;
					break;
				}
			}
			if ( *pArg == '\0' )	/* complete path? */
				break;
			/* loop for next directory */
		}

		pArgv++;
	}
	return retVal;
}

static const char *
sh_GetLastErrorMessage()
{
    static char buf[128];

    FormatMessage(
            FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            GetLastError(),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),  /* default language */
            buf,
            sizeof(buf),
            NULL
    );
    return buf;
}

/*
 * struct sh_FileData --
 *
 * A pointer to the sh_FileData structure is passed into sh_RecordFileData,
 * which will fill in the fields.
 */

struct sh_FileData {
    char pathName[_MAX_PATH];
    DWORD dwFileAttributes;
};

/*
 * sh_RecordFileData --
 *
 * Record the pathname and attributes of the file in
 * the sh_FileData structure pointed to by arg.
 *
 * Always return TRUE (successful completion).
 *
 * This function is intended to be passed into sh_EnumerateFiles
 * to see if a certain pattern expands to exactly one file/directory,
 * and if so, record its pathname and attributes.
 */

static BOOL
sh_RecordFileData(char *pathName, WIN32_FIND_DATA *findData, void *arg)
{
    struct sh_FileData *fData = (struct sh_FileData *) arg;

    strcpy(fData->pathName, pathName);
    fData->dwFileAttributes = findData->dwFileAttributes;
    return TRUE;
}

/*
 * CopyDirRecursive --
 * 
 *     First create a new directory, then recursively copy
 *     the contents of the specified directory to the new directory.
 *
 *     This routine is called only by CopyDir and by itself
 *     (recursion).
 */

static int
CopyDirRecursive(char *srcDir, char *dstDir, int force)
{
    DWORD dwFileAttributes;
    WIN32_FIND_DATA fileData;
    HANDLE hSearch;
    char *srcDirMarker;
    char *dstDirMarker;
    int retVal = 0;  /* The return value of this routine.  0 means success.
                      * Nonzero means failure.  */

    dwFileAttributes = GetFileAttributes(dstDir);
    if (dwFileAttributes == 0xFFFFFFFF) {
        if (_mkdir(dstDir) == -1) {
            fprintf(stderr, "SHMSDOS: cp: cannot create directory %s: %s\n",
                    dstDir, strerror(errno));
            return 8;
        }
    } else {
        if (!(dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            fprintf(stderr, "SHMSDOS: cp: target %s is an existing file\n",
                    dstDir);
            return 8;
        }
        if (force && (dwFileAttributes & FILE_ATTRIBUTE_READONLY)) {
            dwFileAttributes &= ~FILE_ATTRIBUTE_READONLY;
            SetFileAttributes(dstDir, dwFileAttributes);
        }
    }

    dstDirMarker = dstDir + strlen(dstDir);
    *(dstDirMarker++) = '\\';

    srcDirMarker = srcDir + strlen(srcDir);
    *(srcDirMarker++) = '\\';
    strcpy(srcDirMarker, "*");
    hSearch = FindFirstFile(srcDir, &fileData);
    if (hSearch == INVALID_HANDLE_VALUE) {
        /* Directory is empty.  We are done. */

        return 0;
    }

    do {
        if (!strcmp(fileData.cFileName, ".")
                || !strcmp(fileData.cFileName, "..")) {
            /*
             * Skip over . and ..
             */

            continue;
        }

        strcpy(srcDirMarker, fileData.cFileName);
        strcpy(dstDirMarker, fileData.cFileName);
        if (sh_DoCopy(srcDir, fileData.dwFileAttributes,
                dstDir, GetFileAttributes(dstDir),
                force, TRUE) == FALSE) {
            retVal = 5;
        }
    } while (FindNextFile(hSearch, &fileData));
    FindClose(hSearch);
    return retVal;
}

static BOOL
sh_DoCopy(char *srcFileName,
          DWORD srcFileAttributes,
          char *dstFileName,
          DWORD dstFileAttributes,
          int force,
          int recursive
)
{
    if (dstFileAttributes != 0xFFFFFFFF) {
        if ((dstFileAttributes & FILE_ATTRIBUTE_READONLY) && force) {
            dstFileAttributes &= ~FILE_ATTRIBUTE_READONLY;
            SetFileAttributes(dstFileName, dstFileAttributes);
        }
    }

    if (srcFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        if (!recursive) {
            fprintf(stderr, "SHMSDOS: cp: %s is a directory\n",
                    srcFileName);
            return FALSE;
        } else if (CopyDirRecursive(srcFileName, dstFileName, force) != 0) {
            return FALSE;
        }
    } else {
        if (!CopyFile(srcFileName, dstFileName, FALSE)) {
            fprintf(stderr, "SHMSDOS: cp: cannot copy %s to %s: %s\n",
                    srcFileName, dstFileName, sh_GetLastErrorMessage());
            return FALSE;
        }
    }
    return TRUE;
}

/*
 * struct sh_CpCmdArg --
 *
 * A pointer to the sh_CpCmdArg structure is passed into sh_CpFileCmd.
 * The sh_CpCmdArg contains information about the cp command, and
 * provide a buffer for constructing the destination file name.
 */

struct sh_CpCmdArg {
    int force;                /* -f option, ok to overwrite an existing
                               * read-only destination file */
    int recursive;            /* -r or -R option, recursively copy
                               * directories. */
    char *dstFileName;        /* a buffer for constructing the destination
                               * file name */
    char *dstFileNameMarker;  /* points to where in the dstFileName buffer
                               * we should write the file component of the
                               * destination file */
};

/*
 * sh_CpFileCmd --
 *
 * Copy a file to the destination directory
 * 
 * This function is intended to be passed into sh_EnumerateFiles to
 * copy all the files specified by the pattern to the destination
 * directory.
 *
 * Return TRUE if the file is successfully copied, and FALSE otherwise.
 */

static BOOL
sh_CpFileCmd(char *pathName, WIN32_FIND_DATA *findData, void *cpArg)
{
    BOOL retVal = TRUE;
    struct sh_CpCmdArg *arg = (struct sh_CpCmdArg *) cpArg;

    strcpy(arg->dstFileNameMarker, findData->cFileName);
    return sh_DoCopy(pathName, findData->dwFileAttributes,
            arg->dstFileName, GetFileAttributes(arg->dstFileName),
            arg->force, arg->recursive);
}

static int
shellCp (char **pArgv) 
{
    int retVal = 0;
    char **pSrc;
    char **pDst;
    struct sh_CpCmdArg arg;
    struct sh_FileData dstData;
    int dstIsDir = 0;
    int n;

    arg.force = 0;
    arg.recursive = 0;
    arg.dstFileName = dstData.pathName;
    arg.dstFileNameMarker = 0;

    while (*pArgv && **pArgv == '-') {
        char *p = *pArgv;

        while (*(++p)) {
            if (*p == 'f') {
                arg.force = 1;
            } else if (*p == 'r' || *p == 'R') {
                arg.recursive = 1;
            }
        }
        pArgv++;
    }

    /* the first source file */
    if (*pArgv) {
        pSrc = pArgv++;
    } else {
        fprintf(stderr, "SHMSDOS: cp: not enough arguments\n");
        return 3;
    }

    /* get to the last token to find destination */
    if (*pArgv) {
        pDst = pArgv++;
    } else {
        fprintf(stderr, "SHMSDOS: cp: not enough arguments\n");
        return 3;
    }
    while (*pArgv) {
        pDst = pArgv++;
    }

    /*
     * The destination pattern must unambiguously expand to exactly
     * one file or directory.
     */

    changeForwardSlashesTpBackSlashes(*pDst);
    sh_EnumerateFiles(*pDst, *pDst, sh_RecordFileData, &dstData, &n);
    assert(n >= 0);
    if (n == 1) {
        /*
         * Is the destination a file or directory?
         */

        if (dstData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            dstIsDir = 1;
        }
    } else if (n > 1) {
        fprintf(stderr, "SHMSDOS: cp: %s: ambiguous destination file "
                "or directory\n", *pDst);
        return 3;
    } else {
        /*
         * n == 0, meaning that destination file or directory does
         * not exist.  In this case the destination file directory
         * name must be fully specified.
         */

        char *p;

        for (p = *pDst; *p; p++) {
            if (*p == '*' || *p == '?') {
                fprintf(stderr, "SHMSDOS: cp: %s: No such file or directory\n",
                        *pDst);
                return 3;
            }
        }

        /*
         * Do not include the trailing \, if any, unless it is a root
         * directory (\ or X:\).
         */

        if (p > *pDst && p[-1] == '\\' && p != *pDst + 1 && p[-2] != ':') {
            p[-1] = '\0';
        }
        strcpy(dstData.pathName, *pDst);
        dstData.dwFileAttributes = 0xFFFFFFFF;
    }

    /*
     * If there are two or more source files, the destination has
     * to be a directory.
     */

    if (pDst - pSrc > 1 && !dstIsDir) {
        fprintf(stderr, "SHMSDOS: cp: cannot copy more than"
                " one file to the same destination file\n");
        return 3;
    }

    if (dstIsDir) {
        arg.dstFileNameMarker = arg.dstFileName + strlen(arg.dstFileName);

        /*
         * Now arg.dstFileNameMarker is pointing to the null byte at the
         * end of string.  We want to make sure that there is a \ at the
         * end of string, and arg.dstFileNameMarker should point right
         * after that \. 
         */

        if (arg.dstFileNameMarker[-1] != '\\') {
            *(arg.dstFileNameMarker++) = '\\';
        }
    }
	
    if (!dstIsDir) {
        struct sh_FileData srcData;

        assert(pDst - pSrc == 1);
        changeForwardSlashesTpBackSlashes(*pSrc);
        sh_EnumerateFiles(*pSrc, *pSrc, sh_RecordFileData, &srcData, &n);
        if (n == 0) {
            fprintf(stderr, "SHMSDOS: cp: %s: No such file or directory\n",
                    *pSrc);
            retVal = 3;
        } else if (n > 1) {
            fprintf(stderr, "SHMSDOS: cp: cannot copy more than one file or "
                    "directory to the same destination\n");
            retVal = 3;
        } else {
            assert(n == 1);
            if (sh_DoCopy(srcData.pathName, srcData.dwFileAttributes,
                    dstData.pathName, dstData.dwFileAttributes,
                    arg.force, arg.recursive) == FALSE) {
                retVal = 3;
            }
        }
        return retVal;
    }

    for ( ; *pSrc != *pDst; pSrc++) {
        BOOL rv;

        changeForwardSlashesTpBackSlashes(*pSrc);
        rv = sh_EnumerateFiles(*pSrc, *pSrc, sh_CpFileCmd, &arg, &n);
        if (rv == FALSE) {
            retVal = 3;
        } else {
            if (n == 0) {
                fprintf(stderr, "SHMSDOS: cp: %s: No such file or directory\n",
                        *pSrc);
                retVal = 3;
            }
        }
    }

    return retVal;
}

static int
shellEcho(char **pArgv)
{
    int retVal = 0;
    FILE *file = stdout;
    char *fileName = NULL;
    int redirectOutput = 0;
    char **argv;
    char *arrow = NULL;
    int skipTwoArgs = 0;
    int firstArg;
    const char *mode = "w";

    if (*pArgv == NULL) {
        return 0;
    }

    /*
     * See if output is redirected to a file
     */

    for (argv = pArgv; *argv; argv++) {
        char *p = *argv;

        if (*p == '>') {
            if (redirectOutput) {
                fprintf(stderr, "SHMSDOS: echo: ambiguous output "
                        "redirection\n");
                retVal = 8;
                goto done;
            }
            arrow = p;
            if (*(p + 1) == '>') {
                /*
                 * append to redirected output file
                 */

                mode = "a";
                p++;
            }
            if (*(p + 1) == '\0') {
                /*
                 * The output file name is the next argument.
                 * So when we print the arguments later, we have
                 * one more argument to skip (in addition to the
                 * arrow ">").
                 */

                fileName = *(argv + 1);
                skipTwoArgs = 2;
            } else {
                fileName = p + 1;
            }
            if (fileName == NULL || *fileName == '\0') {
                fprintf(stderr, "SHMSDOS: echo: missing output file\n");
                assert(file == stdout);
                return 8;
            }
            file = fopen(fileName, mode);
            if (file == NULL) {
                fprintf(stderr, "SHMSDOS: echo: cannot open output file %s: %s\n",
                        fileName, strerror(errno));
                assert(file == stdout);
		return 8;
            }
            redirectOutput = 1;
        }
    }

    argv = pArgv;
    firstArg = 1;
    while (*argv) {
        if (*argv != arrow) {
            if (firstArg) {
                firstArg = 0;
            } else {
                fprintf(file, " ");
            }
            fprintf(file, *argv);
        } else if (skipTwoArgs) {
            argv++;
        }
        argv++;
    }
    fprintf(file, "\n");

done:
    if (file != stdout) {
        fclose(file);
    }
    return retVal;
}

/*
 ****************************************************************
 *
 * The following routines implement the 'rm' command.
 *
 ****************************************************************
 */

/*
 * RemoveDirRecursive --
 * 
 *     First recursively remove the contents of the specified
 *     directory, then remove the directory itself.
 *     It is assumed that dirName is just the name of the
 *     directory itself, i.e., it is not a full or relative
 *     pathname.
 *
 *     This routine is called only by RemoveDir and by itself
 *     (recursion).
 */

static int
RemoveDirRecursive(const char *dirName, int forceRm)
{
    WIN32_FIND_DATA fileData;
    HANDLE hSearch;
    BOOL rv;
    int retVal = 0;  /* The return value of this routine.  0 means success.
                      * Nonzero means failure.  */

    _chdir(dirName);

    hSearch = FindFirstFile("*", &fileData);
    if (hSearch == INVALID_HANDLE_VALUE) {
        /* Good, it's already empty */
        goto done;
    }

    do {
        /*
         * Skip over . and ..
         */

        if (strcmp(fileData.cFileName, ".")
                && strcmp(fileData.cFileName, "..")) {
            if (fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                if (RemoveDirRecursive(fileData.cFileName, forceRm) != 0) {
                    retVal = 5;
                }
            } else {
                if (fileData.dwFileAttributes & FILE_ATTRIBUTE_READONLY) {
                    if (forceRm) {
                        /*
                         * Change mode to writable, in preparation for
                         * removal.
                         */

                        _chmod(fileData.cFileName, _S_IWRITE);
                    } else {
                        fprintf(stderr, "SHMSDOS: rm: file %s is read-only\n",
                                fileData.cFileName);
                        retVal = 5;
                    }
                }
                if (!DeleteFile(fileData.cFileName)) {
                    fprintf(stderr, "SHMSDOS: rm: cannot delete file %s: %s\n",
                            fileData.cFileName, sh_GetLastErrorMessage());
                    retVal = 5;
                }
            }
        }
        rv = FindNextFile(hSearch, &fileData);
    } while (rv);
    FindClose(hSearch);

    _chdir("..");
done:
    if (!RemoveDirectory(dirName) ) {
        fprintf(stderr, "SHMSDOS: rm: cannot remove directory %s: %s\n",
                dirName, sh_GetLastErrorMessage());
        retVal = 5;
    }
    return retVal;
}

/*
 * RemoveDir --
 *     Remove the contents of a directory and the directory itself.
 *     This routine is called only by shellRm.
 */

static int
RemoveDir(const char *dirName, int forceRm)
{
    char cwd[_MAX_PATH];
    char *cPtr;
    int retVal;

    /*
     * Before we call RemoveDirRecursive, which actually deletes things,
     * we want to find the directory name proper, not including the
     * path.
     */

    cPtr = strrchr(dirName, '\\');
    if (cPtr != NULL && *(cPtr + 1) != '\0') {
        _getcwd(cwd, sizeof(cwd));
        *cPtr = '\0';
        _chdir(dirName);
        retVal = RemoveDirRecursive(cPtr + 1, forceRm);
        _chdir(cwd);
    } else {
        retVal = RemoveDirRecursive(dirName, forceRm);
    }
    return retVal;
}

/*
 * struct sh_RmCmdArg --
 *
 * A pointer to the sh_RmCmdArg structure is passed into sh_RmFileCmd.
 */

struct sh_RmCmdArg {
    int force;      /* -f option, ok to remove read-only files and
                     * directories */
    int recursive;  /* -r or -R option, recursively remove directories */
};

/*
 * sh_RmFileCmd --
 *
 * Remove the file or directory.  Return TRUE if the file or directory
 * is successfully removed, FALSE otherwise.
 */

static BOOL
sh_RmFileCmd(char *pathName, WIN32_FIND_DATA *fileData, void *rmArg)
{
    BOOL rv = TRUE;
    struct sh_RmCmdArg *arg = (struct sh_RmCmdArg *) rmArg;

    if (fileData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        if (arg->recursive) {
            if (RemoveDir(pathName, arg->force) != 0) {
                rv = FALSE;
            }
        } else {
            fprintf(stderr, "SHMSDOS: rm: %s is a directory\n",
                    fileData->cFileName);
            rv = FALSE;
        }
    } else {
        if (fileData->dwFileAttributes & FILE_ATTRIBUTE_READONLY) {
            if (arg->force) {
                /* 
                 * Change mode to writable, in preparation for
                 * removal.
                 */

                _chmod(pathName, _S_IWRITE);
            } else {
                fprintf(stderr,
                        "SHMSDOS: rm: file %s is read-only\n",
                        pathName);
                rv = FALSE;
            }
        }
        if (!DeleteFile(pathName)) {
            fprintf(stderr,
                    "SHMSDOS: rm: cannot delete file %s: %s\n",
                    pathName, sh_GetLastErrorMessage());
            rv = FALSE;
        }
    }
    return rv;
}

/*
 * sh_EnumerateFiles --
 *
 * Enumerate all the files in the specified pattern, which is a pathname
 * containing possibly wildcard characters such as * and ?.  fileFcn
 * is called on each file, passing the expanded file name, a pointer
 * to the file's WIN32_FILE_DATA, and the arg pointer.
 * 
 * It is assumed that there are no wildcard characters before the
 * character pointed to by 'where'.
 *
 * On return, *nFiles stores the number of files enumerated.  *nFiles is
 * set to this number whether sh_EnumerateFiles or 'fileFcn' succeeds
 * or not.
 *
 * Return TRUE if the files are successfully enumerated and all
 * 'fileFcn' invocations succeeded.  Return FALSE if something went
 * wrong.
 */

static BOOL sh_EnumerateFiles(
        const char *pattern,
        const char *where,
        sh_FileFcn fileFcn,
        void *arg,
        int *nFiles
        )
{
    WIN32_FIND_DATA fileData;
    HANDLE hSearch;
    const char *src;
    char *dst;
    char fileName[_MAX_PATH];
    char *fileNameMarker = fileName;
    char *oldFileNameMarker;
    BOOL hasWildcard = FALSE;
    BOOL retVal = TRUE;
    BOOL patternEndsInDotStar = FALSE;
    BOOL patternEndsInDot = FALSE;  /* a special case of
                                     * patternEndsInDotStar */
    int numDotsInPattern;
    int len;
    
    /*
     * Windows expands patterns ending in ".", ".*", ".**", etc.
     * differently from the glob expansion on Unix.  For example,
     * both "foo." and "foo.*" match "foo", and "*.*" matches
     * everything, including filenames with no dots.  So we need
     * to throw away extra files returned by the FindNextFile()
     * function.  We require that a matched filename have at least
     * the number of dots in the pattern.
     */
    len = strlen(pattern);
    if (len >= 2) {
        /* Start from the end of pattern and go backward */
        const char *p = &pattern[len - 1];

        /* We can have zero or more *'s */
        while (p >= pattern && *p == '*') {
            p--;
        }
        if (p >= pattern && *p == '.') {
            patternEndsInDotStar = TRUE;
            if (p == &pattern[len - 1]) {
                patternEndsInDot = TRUE;
            }
            p--;
            numDotsInPattern = 1;
            while (p >= pattern && *p != '\\') {
                if (*p == '.') {
                    numDotsInPattern++;
                }
                p--;
            }
        }
    }

    *nFiles = 0;

    /*
     * Copy pattern to fileName, but only up to and not including
     * the first \ after the first wildcard letter.
     *
     * Make fileNameMarker point to one of the following:
     * - the start of fileName, if fileName does not contain any \.
     * - right after the \ before the first wildcard letter, if there is
     *   a wildcard character.
     * - right after the last \, if there is no wildcard character.
     */

    dst = fileName;
    src = pattern;
    while (src < where) {
        if (*src == '\\') {
            oldFileNameMarker = fileNameMarker;
            fileNameMarker = dst + 1;
        }
        *(dst++) = *(src++);
    }

    while (*src && *src != '*' && *src != '?') {
        if (*src == '\\') {
            oldFileNameMarker = fileNameMarker;
            fileNameMarker = dst + 1;
        }
        *(dst++) = *(src++);
    }

    if (*src) {
        /*
         * Must have seen the first wildcard letter
         */

        hasWildcard = TRUE;
        while (*src && *src != '\\') {
            *(dst++) = *(src++);
        }
    }
    
    /* Now src points to either null or \ */

    assert(*src == '\0' || *src == '\\');
    assert(hasWildcard || *src == '\0');
    *dst = '\0';

    /*
     * If the pattern does not contain any wildcard characters, then
     * we don't need to go the FindFirstFile route.
     */

    if (!hasWildcard) {
        /*
         * See if it is the root directory, \, or X:\.
         */

        assert(!strcmp(fileName, pattern));
        assert(strlen(fileName) >= 1);
        if (dst[-1] == '\\' && (dst == fileName + 1 || dst[-2] == ':')) {
            fileData.cFileName[0] = '\0';
        } else {
            /*
             * Do not include the trailing \, if any
             */

            if (dst[-1] == '\\') {
                assert(*fileNameMarker == '\0');
                dst[-1] = '\0';
                fileNameMarker = oldFileNameMarker;
            } 
            strcpy(fileData.cFileName, fileNameMarker);
        }
        fileData.dwFileAttributes = GetFileAttributes(fileName);
        if (fileData.dwFileAttributes == 0xFFFFFFFF) {
            return TRUE;
        }
        *nFiles = 1;
        return (*fileFcn)(fileName, &fileData, arg);
    }

    hSearch = FindFirstFile(fileName, &fileData);
    if (hSearch == INVALID_HANDLE_VALUE) {
        return retVal;
    }

    do {
        if (!strcmp(fileData.cFileName, ".")
                || !strcmp(fileData.cFileName, "..")) {
            /* 
             * Skip over . and ..
             */

            continue;
        }

        if (patternEndsInDotStar) {
            int nDots = 0;
            char *p = fileData.cFileName;
            while (*p) {
                if (*p == '.') {
                    nDots++;
                }
                p++;
            }
            /* Now p points to the null byte at the end of file name */
            if (patternEndsInDot && (p == fileData.cFileName
                    || p[-1] != '.')) {
                /*
                 * File name does not end in dot.  Skip this file.
                 * Note: windows file name probably cannot end in dot,
                 * but we do this check anyway.
                 */
                continue;
            }
            if (nDots < numDotsInPattern) {
                /*
                 * Not enough dots in file name.  Must be an extra
                 * file in matching .* pattern.  Skip this file.
                 */
                continue;
            }
        }

        strcpy(fileNameMarker, fileData.cFileName);
        if (*src && *(src + 1)) {
            /*
             * More to go.  Recurse.
             */

            int n;

            assert(*src == '\\');
            where = fileName + strlen(fileName);
            strcat(fileName, src);
            sh_EnumerateFiles(fileName, where, fileFcn, arg, &n);
            *nFiles += n;
        } else {
            assert(strchr(fileName, '*') == NULL);
            assert(strchr(fileName, '?') == NULL);
            (*nFiles)++;
            if ((*fileFcn)(fileName, &fileData, arg) == FALSE) {
                retVal = FALSE;
            }
        }
    } while (FindNextFile(hSearch, &fileData));

    FindClose(hSearch);
    return retVal;
}
        

/*
 * shellRm --
 * 
 * Implement the Unix 'rm' command.
 *
 * We require that the option flags of 'rm', if any,
 * be the first arguments and precede all the other
 * arguments.  Right now the -f, -r, and -R flags are
 * recognized.  We change / to \ in file pathname arguments.
 */

static int
shellRm (char **pArgv)
{
    int retVal = 0;  /* The return value of this function.  0 means
                      * success.  Nonzero means failure. */
    struct sh_RmCmdArg arg;

    arg.force = 0;
    arg.recursive = 0;

    /* Convert option flags */
    while (*pArgv && (**pArgv == '-')) {
        char *p = *pArgv;
        p++;
        while (*p) {
            if (!arg.recursive && (*p == 'r' || *p == 'R')) {
                arg.recursive = 1;
            }
            if (!arg.force && *p == 'f') {
                arg.force = 1;
            }
            p++;
        }
        pArgv++;
    }

    /*
     * If -f is not specified and there are no paths specified,
     * we complain.
     */

    if (!arg.force && !*pArgv) {
        fprintf(stderr, "SHMSDOS: usage: rm [-fRr] file ...\n");
        retVal = 5;
    }

    /* Delete each file/directory */
    for ( ; *pArgv; pArgv++) {
        int n;
        BOOL rv;

        changeForwardSlashesTpBackSlashes(*pArgv);
        rv = sh_EnumerateFiles(*pArgv, *pArgv, sh_RmFileCmd, &arg, &n);
        if (rv == FALSE) {
            retVal = 5;
        } else {
            if (n == 0 && !arg.force) {
                fprintf(stderr, "SHMSDOS: rm: %s: No such file or directory\n",
                        *pArgv);
                retVal = 5;
            }
        }
    }
    return retVal;
}

static int
shellTest (char **pArgv) 
{
	int retVal = 0;			/* assume valid return */
	char * arg1 = NULL;
	char * arg2 = NULL;
	char * arg3 = NULL;
	struct _stat buf;
	int result;
	int	true = 0;
	int false = 1;
	unsigned short fileStatus;

	if ( strcmp ( *pArgv, "!" ) == 0 ) {
		pArgv++;
		true = !true;
		false = !false;
	}
	arg1 = *pArgv++;
				
	if ( *arg1 == '-' ) {
		arg2 = *pArgv++;

		// process string tests
		switch ( *(arg1+1) ) {
		case 'z':
			return (!arg2 || strlen ( arg2 ) == 0) ? true : false;		// return zero if length of string is zero
		case 'n':
			return (arg2 && strlen ( arg2 ) != 0) ? true : false;	   // return zero if length of string is nonzero
		}

		// process file tests
		if ( !arg2 )
			return false;
		changeForwardSlashesTpBackSlashes ( arg2 );

		/* Check if statistics are valid: */
		result = _stat( arg2, &buf );
		if( result != 0 )
			return false;					// file or directory does not exist

		fileStatus = buf.st_mode;
		if ( (*(arg1+1) != 'd') && (*(arg1+1) != 'r')
				&& (*(arg1+1) != 'f') ) 
			printf ("!!!!!! never been tested: test -%c\n", *(arg1+1) );

		switch ( *(arg1+1) ) {
		case 'c':			// character special file?
			if ( fileStatus & _S_IFCHR )
				return true;	// return directory found
			return false; 		// return directory not found 
		case 'd':			// directory?
			if ( fileStatus & _S_IFDIR )
				return true;	// return directory found
			return false; 		// return directory not found 
		case 'f':			// regular file?
			if ( fileStatus & _S_IFREG )
				return true;	// return regular file found
			return false; 		// return regular file not found 
		case 'g':			// set group ID bit set?
			return false; 		// return not set 
		case 'h':			// symbolic link?
		case 'l':			// symbolic link?
		case 'L':			// symbolic link?
			return false; 		// return symbolic link not found 
		case 'k':			// sticky bit set?
			return false; 		// return not set 
		case 'p':			// named pipe?
			if ( fileStatus & _S_IFIFO )
				return true;	// return named pipe found
			return false; 		// return named pipe not found 
		case 'r':			// readable file?
			if ( fileStatus & _S_IREAD )
				return true;	// return read permission found
			return false; 		// return read permission not found 
		case 's':			// size greater than zero?
			if ( buf.st_size > 0 )
				return true;	// return file has greater than zero size
			return false; 		// return file size is zero 
		case 't':			// set for terminal device?
			return -1; 		// return not implemented 
		case 'u':			// set user ID bit set?
			return false; 		// return not set 
		case 'w':			// writeable file?
			if ( fileStatus & _S_IWRITE )
				return true;	// return write permission found
			return false; 		// return write permission not found 
		case 'x':			// executable file?
			if ( fileStatus & _S_IEXEC )
				return true;	// return execute permission found
			return false; 		// return execute permission not found 
		default:
			return -1; // not implemented
		}
	} else {			// string compares
		printf ("!!!!!! never been tested: test %s\n", arg1 );
		if ( *pArgv )
			arg2 = *pArgv++;
		if ( *pArgv )
			arg3 = *pArgv++;

		// check if strings compare s1 = s2 or not compare s1 != s2
		if ( arg2 && arg3 ) {
			if ( strcmp ( arg2, "=" ) == 0 )
				return strcmp ( arg1, arg3 ) ? false : true;
			if ( strcmp ( arg2, "!=" ) == 0 )
				return strcmp ( arg1, arg3 ) ? true : false;
			if ( strcmp ( arg2, "-eq" ) == 0 )
				return ( atol(arg1) == atol(arg3) ) ? true : false;
			if ( strcmp ( arg2, "-ne" ) == 0 )
				return ( atol(arg1) != atol(arg3) ) ? true : false;
			if ( strcmp ( arg2, "-gt" ) == 0 )
				return ( atol(arg1) > atol(arg3) ) ? true : false;
			if ( strcmp ( arg2, "-ge" ) == 0 )
				return ( atol(arg1) >= atol(arg3) ) ? true : false;
			if ( strcmp ( arg2, "-lt" ) == 0 )
				return ( atol(arg1) < atol(arg3) ) ? true : false;
			if ( strcmp ( arg2, "-le" ) == 0 )
				return ( atol(arg1) <= atol(arg3) ) ? true : false;
		}
		// check for empty string
		if ( (arg2 == NULL ) && ( arg3 == NULL)	)
			return (strlen(arg1) > 0 ) ? true : false;
	}

	return -1; // not implemented																								 
}

#define COPY_BUFFER_SIZE        4096

static int
shellCat (char **pArgv) 
{
	int retVal = 0;			/* assume valid return */
	char **pSrc;
	char **pDst;

    int sfd, dfd, len;
    struct _stat fi;
    char copy_buffer[COPY_BUFFER_SIZE];
    unsigned long read_len;

	while ( **pArgv == '-' )
		pArgv++;

	/* get source files */
	pSrc = pArgv++;

	/* get to last token to find destination */
	pDst = pArgv++;
	while ( *pArgv ) 
		pDst = pArgv++;
	
    if( (dfd = _open(*pDst, O_RDWR | O_CREAT | O_BINARY, 0666)) == -1) {
		perror ( "Could not open the destination file ");
        fprintf(stdout, "Destination file %s\n", *pDst);
		retVal = (4);
    }
 
	while ( *pSrc != *pDst ) { 

        if (!(strcmp(*pSrc, ">"))) {
           *pSrc++;
            continue;
        }
        if (!strcmp(*pSrc, *pDst)) {
           *pSrc++;
            continue;
        }
        if( (sfd = _open(*pSrc, O_RDONLY | O_BINARY, 0666)) == -1) {
			perror ( "Could not open the source file" );
            fprintf(stdout, "Source file %s\n", *pSrc);
			retVal = (4);
            return retVal;
        }
 
        _stat(*pSrc, &fi);
        if(!(_S_IFREG & fi.st_mode)) {
			perror ( "Could not stat the source file" );
            fprintf(stdout, "Source file %s\n", *pSrc);
			retVal = (4);
            return retVal;
        }
        len = fi.st_size;
 
        while(len) {
            unsigned long result;
            read_len = len>COPY_BUFFER_SIZE?COPY_BUFFER_SIZE:len;
 
            if ( (read_len = _read(sfd, copy_buffer, read_len)) == -1) {
    			perror ( "Could not read the source file" );
                fprintf(stdout, "Source file %s\n", *pSrc);
       			retVal = (4);
                return retVal;
            }
 
            if ( (result = (unsigned long)_write(dfd, copy_buffer, read_len)) != read_len) {
    			perror ( "Could not write to the destination file" );
                fprintf(stdout, "Destination file %s\n", *pDst);
    			retVal = (4);
                return retVal;
            }
 
            len -= read_len;
        }
        close(sfd);
        *pSrc++;
    }
    close(dfd);

	return retVal;
}

/*
 * struct sh_MvCmdArg --
 *
 * A pointer to the sh_MvCmdArg structure is passed into sh_MvFileCmd.
 * The sh_MvCmdArg contains information about the mv command, and
 * provide a buffer for constructing the destination file name.
 */

struct sh_MvCmdArg {
    int force;                /* -f option, ok to overwrite an existing
                               * read-only destination file */
    char *dstFileName;        /* a buffer for constructing the destination
                               * file name */
    char *dstFileNameMarker;  /* points to where in the dstFileName buffer
                               * we should write the file component of the
                               * destination file */
};

static BOOL
sh_DoMove(const char *srcFileName,
          DWORD srcFileAttributes,
          const char *dstFileName,
          DWORD dstFileAttributes,
          int force
)
{
    if (dstFileAttributes != 0xFFFFFFFF) {
        /*
         * The destination exists.
         * - If the destination is a file, then the source must be a file.
         * - If the destination is a directory, then it must be empty,
         *   and the source must be a directory.
         * This is really an arbitrary choice.  You may have other
         * preference of the behavior.
         *
         * Because we need to remove the existing destination file
         * or directory, we also require that either it is writable
         * or 'force' is TRUE.
         */

        int srcIsDir = srcFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
        int dstIsDir = dstFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
        BOOL rv;

        if (!dstIsDir && srcIsDir) {
            fprintf(stderr, "SHMSDOS: mv: cannot rename %s (a directory) "
                    "to %s (a file)\n", srcFileName, dstFileName);
            return FALSE;
        }
        if (dstIsDir && !srcIsDir) {
            fprintf(stderr, "SHMSDOS: mv: cannot rename %s (a file) "
                    "to %s (a directory)\n", srcFileName, dstFileName);
            return FALSE;
        }
        if ((dstFileAttributes & FILE_ATTRIBUTE_READONLY) && force) {
            dstFileAttributes &= ~FILE_ATTRIBUTE_READONLY;
            SetFileAttributes(dstFileName, dstFileAttributes);
        }
        if (dstIsDir) {
            rv = RemoveDirectory(dstFileName);
        } else {
            rv = DeleteFile(dstFileName);
        }
        if (rv == FALSE) {
            fprintf(stderr, "SHMSDOS: mv: cannot overwrite %s: %s\n",
                    dstFileName, sh_GetLastErrorMessage());
            return FALSE;
        }
    }

    if (MoveFile(srcFileName, dstFileName) == FALSE) {
        fprintf(stderr, "SHMSDOS: mv: cannot move %s to %s: %s\n",
                srcFileName, dstFileName, sh_GetLastErrorMessage());
        return FALSE;
    }
    return TRUE;
}

/*
 * sh_MvFileCmd --
 *
 * Move a file to the destination directory
 * 
 * This function is intended to be passed into sh_EnumerateFiles to
 * move all the files specified by the pattern to the destination
 * directory.
 *
 * Return TRUE if the file is successfully moved, and FALSE otherwise.
 */

static BOOL
sh_MvFileCmd(char *pathName, WIN32_FIND_DATA *findData, void *mvArg)
{
    struct sh_MvCmdArg *arg = (struct sh_MvCmdArg *) mvArg;
    DWORD dwFileAttributes;

    strcpy(arg->dstFileNameMarker, findData->cFileName);
    dwFileAttributes = GetFileAttributes(arg->dstFileName);
    return sh_DoMove(pathName, findData->dwFileAttributes,
            arg->dstFileName, dwFileAttributes, arg->force);
}

static int
shellMv (char **pArgv) 
{
    int retVal = 0;
    char **pSrc;
    char **pDst;
    struct sh_MvCmdArg arg;
    struct sh_FileData dstData;
    int dstIsDir = 0;
    int n;

    arg.force = 0;
    arg.dstFileName = dstData.pathName;
    arg.dstFileNameMarker = 0;

    while (*pArgv && **pArgv == '-') {
        char *p = *pArgv;

        while (*(++p)) {
            if (*p == 'f') {
                arg.force = 1;
            }
        }
        pArgv++;
    }

    /* the first source file */
    if (*pArgv) {
        pSrc = pArgv++;
    } else {
        fprintf(stderr, "SHMSDOS: mv: not enough arguments\n");
        return 3;
    }

    /* get to the last token to find destination */
    if (*pArgv) {
        pDst = pArgv++;
    } else {
        fprintf(stderr, "SHMSDOS: mv: not enough arguments\n");
        return 3;
    }
    while (*pArgv) {
        pDst = pArgv++;
    }

    /*
     * The destination pattern must unambiguously expand to exactly
     * one file or directory.
     */

    changeForwardSlashesTpBackSlashes(*pDst);
    sh_EnumerateFiles(*pDst, *pDst, sh_RecordFileData, &dstData, &n);
    assert(n >= 0);
    if (n == 1) {
        /*
         * Is the destination a file or directory?
         */

        if (dstData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            dstIsDir = 1;
        }
    } else if (n > 1) {
        fprintf(stderr, "SHMSDOS: mv: %s: ambiguous destination file "
                "or directory\n", *pDst);
        return 3;
    } else {
        /*
         * n == 0, meaning that destination file or directory does
         * not exist.  In this case the destination file directory
         * name must be fully specified.
         */

        char *p;

        for (p = *pDst; *p; p++) {
            if (*p == '*' || *p == '?') {
                fprintf(stderr, "SHMSDOS: mv: %s: No such file or directory\n",
                        *pDst);
                return 3;
            }
        }

        /*
         * Do not include the trailing \, if any, unless it is a root
         * directory (\ or X:\).
         */

        if (p > *pDst && p[-1] == '\\' && p != *pDst + 1 && p[-2] != ':') {
            p[-1] = '\0';
        }
        strcpy(dstData.pathName, *pDst);
        dstData.dwFileAttributes = 0xFFFFFFFF;
    }

    /*
     * If there are two or more source files, the destination has
     * to be a directory.
     */

    if (pDst - pSrc > 1 && !dstIsDir) {
        fprintf(stderr, "SHMSDOS: mv: cannot move more than"
                " one file to the same destination file\n");
        return 3;
    }

    if (dstIsDir) {
        arg.dstFileNameMarker = arg.dstFileName + strlen(arg.dstFileName);

        /*
         * Now arg.dstFileNameMarker is pointing to the null byte at the
         * end of string.  We want to make sure that there is a \ at the
         * end of string, and arg.dstFileNameMarker should point right
         * after that \. 
         */

        if (arg.dstFileNameMarker[-1] != '\\') {
            *(arg.dstFileNameMarker++) = '\\';
        }
    }
	
    if (!dstIsDir) {
        struct sh_FileData srcData;

        assert(pDst - pSrc == 1);
        changeForwardSlashesTpBackSlashes(*pSrc);
        sh_EnumerateFiles(*pSrc, *pSrc, sh_RecordFileData, &srcData, &n);
        if (n == 0) {
            fprintf(stderr, "SHMSDOS: mv: %s: No such file or directory\n",
                    *pSrc);
            retVal = 3;
        } else if (n > 1) {
            fprintf(stderr, "SHMSDOS: mv: cannot move more than one file or "
                    "directory to the same destination\n");
            retVal = 3;
        } else {
            assert(n == 1);
            if (sh_DoMove(srcData.pathName, srcData.dwFileAttributes,
                    dstData.pathName, dstData.dwFileAttributes, 
                    arg.force) == FALSE) {
                retVal = 3;
            }
        }
        return retVal;
    }

    for ( ; *pSrc != *pDst; pSrc++) {
        BOOL rv;

        changeForwardSlashesTpBackSlashes(*pSrc);
        rv = sh_EnumerateFiles(*pSrc, *pSrc, sh_MvFileCmd, &arg, &n);
        if (rv == FALSE) {
            retVal = 3;
        } else {
            if (n == 0) {
                fprintf(stderr, "SHMSDOS: mv: %s: No such file or directory\n",
                        *pSrc);
                retVal = 3;
            }
        }
    }

    return retVal;
}
