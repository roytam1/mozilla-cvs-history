/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is the Netscape security libraries.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are 
 * Copyright (C) 1994-2000 Netscape Communications Corporation.  All
 * Rights Reserved.
 * 
 * Contributor(s):
 * 
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 or later (the
 * "GPL"), in which case the provisions of the GPL are applicable 
 * instead of those above.  If you wish to allow use of your 
 * version of this file only under the terms of the GPL and not to
 * allow others to use your version of this file under the MPL,
 * indicate your decision by deleting the provisions above and
 * replace them with the notice and other provisions required by
 * the GPL.  If you do not delete the provisions above, a recipient
 * may use your version of this file under either the MPL or the
 * GPL.
 */

#include <string.h>
#include <ctype.h>

#include "cmdutil.h"

/* XXX */
#define ZERO 0

static int s_indent_size = 4;

void
CMD_SetIndentSize(int size) 
{
    s_indent_size = size;
}

#if 0
static void
indent(PRFileDesc *out, int level)
{
    int i, j;
    for (i=0; i<level; i++)
	for (j=0; j<s_indent_size; j++)	
	    PR_fprintf(out, " ");
}
#endif

struct cmdPrintStateStr {
    PRFileDesc *file;
    int width;
    int indent;
    int linepos;
};

static void
init_print_ps(cmdPrintState *ps, PRFileDesc *outfile, int width, int indent)
{
    ps->file = (outfile) ? outfile : PR_STDOUT;
    ps->width = (width > 0) ? width : 80;
    ps->indent = (indent > 0) ? indent : 0;
    ps->linepos = 0;
}

static void
print_ps_indent(cmdPrintState *ps)
{
    int j;
    if (ps->linepos != 0) {
	PR_fprintf(ps->file, "\n");
	ps->linepos = 0;
    }
    for (j=0; j<ps->indent; j++) PR_fprintf(ps->file, " ");
    ps->linepos = ps->indent;
}

static void
print_ps_to_indent(cmdPrintState *ps)
{
    if (ps->linepos > ps->indent)
	PR_fprintf(ps->file, "\n");
    while (ps->linepos <= ps->indent) {
	PR_fprintf(ps->file, " ");
	ps->linepos++;
    }
}

static void
nprintbuf(cmdPrintState *ps, char *buf, int start, int len)
{
    int j;
    for (j=start; j<start + len; j++) {
	if (buf[j] == '\n') {
	    PR_fprintf(ps->file, "\n");
	    ps->linepos = 0;
	    print_ps_indent(ps);
	} else {
	    PR_fprintf(ps->file, "%c", buf[j]);
	    ps->linepos++;
	}
    }
}

static void 
nprintf(cmdPrintState *ps, char *msg, ...)
{
    char buf[256];
    int i, len, grouplen;
    PRBool openquote, openbracket, openparen, openangle, itsaword;
    va_list args;
    va_start(args, msg);
    vsprintf(buf, msg, args);
    len = strlen(buf);
    /* print_ps_indent(ps); */
    if (len < ps->width - ps->linepos) {
	nprintbuf(ps, buf, 0, len + 1);
	return;
    }
    /* group in this order: " [ ( < word > ) ] " */
    i=0;
    openquote=openbracket=openparen=openangle=itsaword=PR_FALSE;
    while (i<len) {
	grouplen = 0;
	if (buf[i] == '\"') { openquote = PR_TRUE; grouplen = 1; }
	else if (buf[i] == '[') { openbracket = PR_TRUE; grouplen = 1; }
	else if (buf[i] == '(') { openparen = PR_TRUE; grouplen = 1; }
	else if (buf[i] == '<') { openangle = PR_TRUE; grouplen = 1; }
	else itsaword = PR_TRUE;
	while (grouplen < len && buf[i+grouplen] != '\0' &&
	       ((openquote && buf[i+grouplen] != '\"') ||
	        (openbracket && buf[i+grouplen] != ']') ||
	        (openparen && buf[i+grouplen] != ')') ||
	        (openangle && buf[i+grouplen] != '>') ||
	        (itsaword && !isspace(buf[i+grouplen]))))
	    grouplen++;
	grouplen++; /* grab the terminator (whitespace for word) */
	if (!itsaword && isspace(buf[i+grouplen])) grouplen++;
	if (grouplen < ps->width - ps->linepos) {
	    nprintbuf(ps, buf, i, grouplen);
	} else if (grouplen < ps->width - ps->indent) {
	    print_ps_indent(ps);
	    nprintbuf(ps, buf, i, grouplen);
	} else {
	    /* it's just too darn long.  what to do? */
	}
	i += grouplen;
	openquote=openbracket=openparen=openangle=itsaword=PR_FALSE;
    }
    va_end(args);
}

void 
CMD_PrintUsageString(cmdPrintState *ps, char *str)
{
    nprintf(ps, "%s", str);
}

static int
command_line_okay(cmdCommand *cmd, char *progName)
{
    int i, c = -1;
    /* user asked for help.  hope somebody gives it to them. */
    if (cmd->opt[0].on) return 0;
    /* check that the command got all of its needed options */
    for (i=0; i<cmd->ncmd; i++) {
	if (cmd->cmd[i].on) {
	    if (c > 0) {
		fprintf(stderr, 
		        "%s: only one command can be given at a time.\n",
		        progName);
		CMD_Usage(progName, cmd);
		return -1;
	    } else {
		c = i;
	    }
	}
    }
    if (cmd->cmd[c].argUse == CMDArgReq && cmd->cmd[c].arg == NULL) {
	/* where's the arg when you need it... */
	fprintf(stderr, "%s: command --%s requires an argument.\n",
	                 progName, cmd->cmd[c].s);
	fprintf(stderr, "type \"%s --%s --help\" for help.\n",
	                 progName, cmd->cmd[c].s);
	CMD_Usage(progName, cmd);
	return -1;
    }
    for (i=0; i<cmd->nopt; i++) {
	if (cmd->cmd[c].req[ZERO] & CMDBIT(i)) {
	    /* command requires this option */
	    if (!cmd->opt[i].on) {
		/* but it ain't there */
		fprintf(stderr, "%s: command --%s requires option --%s.\n",
		                 progName, cmd->cmd[c].s, cmd->opt[i].s);
		CMD_Usage(progName, cmd);
		return -1;
	    } else {
		/* okay, its there, but does it have an arg? */
		if (cmd->opt[i].argUse == CMDArgReq && !cmd->opt[i].arg) {
		    fprintf(stderr, "%s: option --%s requires an argument.\n",
		                     progName, cmd->opt[i].s);
		    CMD_Usage(progName, cmd);
		    return -1;
		}
	    }
	} else if (cmd->cmd[c].opt[ZERO] & CMDBIT(i)) {
	   /* this option is optional */
	    if (cmd->opt[i].on) {
		/* okay, its there, but does it have an arg? */
		if (cmd->opt[i].argUse == CMDArgReq && !cmd->opt[i].arg) {
		    fprintf(stderr, "%s: option --%s requires an argument.\n",
		                     progName, cmd->opt[i].s);
		    CMD_Usage(progName, cmd);
		    return -1;
		}
	    }
	} else {
	   /* command knows nothing about it */
	   if (cmd->opt[i].on) {
		/* so why is it on? */
		fprintf(stderr, "%s: option --%s not used with command --%s.\n",
		                 progName, cmd->opt[i].s, cmd->cmd[c].s);
		CMD_Usage(progName, cmd);
		return -1;
	   }
	}
    }
    return 0;
}

static char *
get_arg(char *curopt, char **nextopt, int argc, int *index)
{
    char *str;
    if (curopt) {
	str = curopt;
    } else {
	if (*index + 1 >= argc) return NULL;
	/* not really an argument but another flag */
	if (nextopt[*index+1][0] == '-') return NULL;
	str = nextopt[++(*index)];
    }
    /* parse the option */
    return strdup(str);
}

int
CMD_ParseCommandLine(int argc, char **argv, char *progName, cmdCommand *cmd)
{
    int i, j, k;
    int cmdToRun = -1;
    char *flag;
    i=1;
    if (argc <= 1) return -2; /* gross hack for cmdless things like atob */
    do {
	flag = argv[i];
	if (strlen(flag) < 2) /* huh? */
	    return -1;
	if (flag[0] != '-')
	    return -1;
	/* ignore everything after lone "--" (app-specific weirdness there) */
	if (strcmp(flag, "--") == 0)
	    return cmdToRun;
	/* single hyphen means short alias (single-char) */
	if (flag[1] != '-') {
	    j=1;
	    /* collect a set of opts, ex. -abc */
	    while (flag[j] != '\0') {
		PRBool found = PR_FALSE;
		/* walk the command set looking for match */
		for (k=0; k<cmd->ncmd; k++) {
		    if (flag[j] == cmd->cmd[k].c) {
			/* done - only take one command at a time */
			if (j > 1) return -1;
			cmd->cmd[k].on = found = PR_TRUE;
			cmdToRun = k;
			if (cmd->cmd[k].argUse != CMDNoArg)
			    cmd->cmd[k].arg = get_arg(NULL, argv, argc, &i);
			goto next_flag;
		    }
		}
		/* wasn't found in commands, try options */
		for (k=0; k<cmd->nopt; k++) {
		    if (flag[j] == cmd->opt[k].c) {
			/* collect this option and keep going */
			cmd->opt[k].on = found = PR_TRUE;
			if (flag[j+1] == '\0') {
			    if (cmd->opt[k].argUse != CMDNoArg)
				cmd->opt[k].arg = get_arg(NULL, argv, argc, &i);
			    goto next_flag;
			}
		    }
		}
		j++;
		if (!found) return -1;
	    }
	} else { /* long alias, ex. --list */
	    char *fl = NULL, *arg = NULL;
	    PRBool hyphened = PR_FALSE;
	    fl = &flag[2];
	    arg = strchr(fl, '=');
	    if (arg) {
		*arg++ = '\0';
#if 0
	    } else {
		arg = strchr(fl, '-');
		if (arg) {
		    hyphened = PR_TRUE; /* watch this, see below */
		    *arg++ = '\0';
		}
#endif
	    }
	    for (k=0; k<cmd->ncmd; k++) {
		if (strcmp(fl, cmd->cmd[k].s) == 0) {
		    cmd->cmd[k].on = PR_TRUE;
		    cmdToRun = k;
		    if (cmd->cmd[k].argUse != CMDNoArg || hyphened) {
			cmd->cmd[k].arg = get_arg(arg, argv, argc, &i);
		    }
		    if (arg) arg[-1] = '=';
		    goto next_flag;
		}
	    }
	    for (k=0; k<cmd->nopt; k++) {
		if (strcmp(fl, cmd->opt[k].s) == 0) {
		    cmd->opt[k].on = PR_TRUE;
		    if (cmd->opt[k].argUse != CMDNoArg || hyphened) {
			cmd->opt[k].arg = get_arg(arg, argv, argc, &i);
		    }
		    if (arg) arg[-1] = '=';
		    goto next_flag;
		}
	    }
	    return -1;
	}
next_flag:
	i++;
    } while (i < argc);
    if (command_line_okay(cmd, progName) < 0) {
	return -1;
    }
    return cmdToRun;
}

void
CMD_LongUsage(char *progName, cmdCommand *cmd)
{
    int i, j;
    PRBool oneCommand = PR_FALSE;
    cmdPrintState ps;
    init_print_ps(&ps, PR_STDERR, 80, 0);
    nprintf(&ps, "\n%s:   ", progName);
    /* prints app-specific header */
    ps.indent = strlen(progName) + 4;
    nprintf(&ps, "%s\n", cmd->description);
    for (i=0; i<cmd->ncmd; i++) if (cmd->cmd[i].on) oneCommand = PR_TRUE;
    for (i=0; i<cmd->ncmd; i++) {
	if ((oneCommand  && cmd->cmd[i].on) || !oneCommand) {
	    ps.indent = 1;
	    print_ps_indent(&ps);
	    if (cmd->cmd[i].c != 0) {
		nprintf(&ps, "-%c, ", cmd->cmd[i].c);
		nprintf(&ps, "--%-16s ", cmd->cmd[i].s);
	    } else {
		nprintf(&ps, "--%-20s ", cmd->cmd[i].s);
	    }
	    ps.indent += 20;
	    nprintf(&ps, "%s\n", cmd->cmd[i].help);
	    for (j=0; j<cmd->nopt; j++) {
		if (cmd->cmd[i].req[ZERO] & CMDBIT(j)) {
		    ps.indent = 0;
		    print_ps_indent(&ps);
		    nprintf(&ps, "%3s* ", "");
		    if (cmd->opt[j].c != 0) {
			nprintf(&ps, "-%c, ", cmd->opt[j].c);
			nprintf(&ps, "--%-16s  ", cmd->opt[j].s);
		    } else {
			nprintf(&ps, "--%-20s  ", cmd->opt[j].s);
		    }
		    ps.indent += 29;
		    nprintf(&ps, "%s", cmd->optHelp[j]);
		}
	    }
	    for (j=0; j<cmd->nopt; j++) {
		if (cmd->cmd[i].opt[ZERO] & CMDBIT(j)) {
		    ps.indent = 0;
		    print_ps_indent(&ps);
		    nprintf(&ps, "%5s", "");
		    if (cmd->opt[j].c != 0) {
			nprintf(&ps, "-%c, ", cmd->opt[j].c);
			nprintf(&ps, "--%-16s  ", cmd->opt[j].s);
		    } else {
			nprintf(&ps, "--%-20s  ", cmd->opt[j].s);
		    }
		    ps.indent += 29;
		    nprintf(&ps, "%s", cmd->optHelp[j]);
		}
	    }
	}
	nprintf(&ps, "\n");
    }
    ps.indent = 0;
    nprintf(&ps, "\n* - required flag for command\n\n");
    /*nprintf(&ps, "\n\n");*/
    exit(1);
}

void
CMD_Usage(char *progName, cmdCommand *cmd)
{
    int i, j, inc;
    PRBool first;
    cmdPrintState ps;
    init_print_ps(&ps, PR_STDERR, 80, 0);
    nprintf(&ps, "%s", progName);
    ps.indent = strlen(progName) + 1;
    print_ps_to_indent(&ps);
    for (i=0; i<cmd->ncmd; i++) {
	if (cmd->cmd[i].c != 0) {
	    nprintf(&ps, "-%c", cmd->cmd[i].c);
	    inc = 4;
	} else {
	    nprintf(&ps, "--%s", cmd->cmd[i].s);
	    inc = 4 + strlen(cmd->cmd[i].s);
	}
	first = PR_TRUE;
	ps.indent += inc;
	print_ps_to_indent(&ps);
	for (j=0; j<cmd->nopt; j++) {
	    if (cmd->cmd[i].req[ZERO] & CMDBIT(j)) {
		if (cmd->opt[j].c != 0 && cmd->opt[j].argUse == CMDNoArg) {
		    if (first) {
			nprintf(&ps, "-");
			first = !first;
		    }
		    nprintf(&ps, "%c", cmd->opt[j].c);
		}
	    }
	}
	for (j=0; j<cmd->nopt; j++) {
	    if (cmd->cmd[i].req[ZERO] & CMDBIT(j)) {
		if (cmd->opt[j].c != 0)
		    nprintf(&ps, "-%c ", cmd->opt[j].c);
		else
		    nprintf(&ps, "--%s ", cmd->opt[j].s);
		if (cmd->opt[j].argUse != CMDNoArg)
		    nprintf(&ps, "%s ", cmd->opt[j].s);
	    }
	}
	first = PR_TRUE;
	for (j=0; j<cmd->nopt; j++) {
	    if (cmd->cmd[i].opt[ZERO] & CMDBIT(j)) {
		if (cmd->opt[j].c != 0 && cmd->opt[j].argUse == CMDNoArg) {
		    if (first) {
			nprintf(&ps, "[-");
			first = !first;
		    }
		    nprintf(&ps, "%c", cmd->opt[j].c);
		}
	    }
	}
	if (!first) nprintf(&ps, "] ");
	for (j=0; j<cmd->nopt; j++) {
	    if (cmd->cmd[i].opt[ZERO] & CMDBIT(j) && 
	         cmd->opt[j].argUse != CMDNoArg) {
		if (cmd->opt[j].c != 0)
		    nprintf(&ps, "[-%c %s] ", cmd->opt[j].c, cmd->opt[j].s);
		else
		    nprintf(&ps, "[--%s %s] ", cmd->opt[j].s, cmd->opt[j].s);
	    }
	}
	ps.indent -= inc;
	print_ps_indent(&ps);
    }
    ps.indent = 0;
    nprintf(&ps, "\n");
}

void
CMD_InteractiveUsage(char *progName, cmdCommand *cmd)
{
    int i, j;
    cmdPrintState ps;
    int cmdToHelp = -1;
    init_print_ps(&ps, PR_STDERR, 80, 0);
    ps.indent = 1;
    print_ps_to_indent(&ps);
    for (i=0; i<cmd->ncmd; i++) {
	if (cmd->cmd[i].on) {
	    cmdToHelp = i;
	    break;
	}
    }
    if (cmdToHelp > 0) {
	nprintf(&ps, "%s\n", cmd->cmd[cmdToHelp].help);
	for (j=0; j<cmd->nopt; j++) {
	    if (cmd->cmd[cmdToHelp].req[ZERO] & CMDBIT(j)) {
		nprintf(&ps, "  %s", cmd->opt[j].s);
		if (cmd->opt[j].argUse == CMDArgReq) {
		    nprintf(&ps, "=%s", cmd->opt[j].s);
		} else if (cmd->opt[j].argUse == CMDArgOpt) {
		    nprintf(&ps, "(=%s)", cmd->opt[j].s);
		}
		nprintf(&ps, " %s\n", cmd->optHelp[j]);
	    }
	}
	for (j=0; j<cmd->nopt; j++) {
	    if (cmd->cmd[cmdToHelp].opt[ZERO] & CMDBIT(j)) {
		nprintf(&ps, "  [%s", cmd->opt[j].s);
		if (cmd->opt[j].argUse == CMDArgReq) {
		    nprintf(&ps, "=%s]", cmd->opt[j].s);
		} else if (cmd->opt[j].argUse == CMDArgOpt) {
		    nprintf(&ps, "(=%s)]", cmd->opt[j].s);
		} else {
		    nprintf(&ps, "]", cmd->opt[j].s);
		}
		nprintf(&ps, " %s\n", cmd->optHelp[j]);
	    }
	}
	goto finish;
    }
    for (i=0; i<cmd->ncmd; i++) {
	nprintf(&ps, "%s", cmd->cmd[i].s);
	print_ps_indent(&ps);
    }
finish:
    ps.indent = 0;
    nprintf(&ps, "\n");
}

int
CMD_Interactive(cmdCommand *cmd)
{
    int i, clen, remaining;
    int cmdToRun = -1;
    char commandBuf[256];
    char *mark, *token, *opt;
    PRBool quoted = PR_FALSE;
    PRBool hasValue;
    /* clear the state */
    for (i=0; i<cmd->ncmd; i++) {
	cmd->cmd[i].on = PR_FALSE;
    }
    for (i=0; i<cmd->nopt; i++) {
	cmd->opt[i].on = PR_FALSE;
	cmd->opt[i].arg = NULL;
    }
    PR_fprintf(PR_STDOUT, "\n> ");
read_command:
    /* read command from user */
    clen = PR_Read(PR_STDIN, commandBuf, sizeof commandBuf);
    if (clen < 0) {
	/* error occurred */
	return -1;
    } else if (clen == 0) {
	/* empty line, try again */
	goto read_command;
    }
    mark = token = commandBuf;
    remaining = clen;
    /* skip to whitespace or EOL */
    while (!isspace(*mark) && (mark - token < remaining)) mark++;
    *mark++ = '\0';
    /* find the command */
    for (i=0; i<cmd->ncmd; i++) {
	if (strcmp(token, cmd->cmd[i].s) == 0) {
	    cmdToRun = i;
/*PR_fprintf(PR_STDERR, "doing command %s\n", token);*/
	    cmd->cmd[i].on = PR_TRUE;
	    break;
	}
    }
    if (cmdToRun < 0) { 
	if (strcmp(token, "quit") == 0 || strcmp(token, "q") == 0) {
	    /* this one applies to all */
	    return -2;
	} else {
	    return -1;
	}
    }
    remaining -= mark - token;
    token = mark;
    /* read options */
    while (remaining > 0) {
	quoted = PR_FALSE;
	/* skip to end of whitespace */
	while (isspace(*mark) && (mark - token < remaining)) mark++;
	/* skip to = or whitespace */
	while (*mark != '=' && !isspace(*mark) &&
	       (mark - token < remaining)) mark++;
	opt = token;
	hasValue = (*mark == '=');
	*mark++ = '\0';
	if (hasValue) {
	    token = mark;
	    if (*mark == '\"') {
		quoted = PR_TRUE;
		token = ++mark;
	    }
	    /* skip to whitespace or EOL */
	    if (quoted) {
		while (*mark != '\"' && (mark - token < remaining)) mark++;
		*mark++ = '\0';
	    } else {
		while (!isspace(*mark) && (mark - token < remaining)) mark++;
	    }
	    *mark++ = '\0';
	}
	/* find the option */
	for (i=0; i<cmd->nopt; i++) {
	    if (strcmp(opt, cmd->opt[i].s) == 0) {
		cmd->opt[i].on = PR_TRUE;
		if (hasValue) {
		    cmd->opt[i].arg = strdup(token); /* XXX */
		}
/*PR_fprintf(PR_STDERR, " with option %s = \"%s\"\n", opt, token);*/
	    }
	}
	remaining -= mark - opt;
	token = mark;
    }
    return cmdToRun;
}
