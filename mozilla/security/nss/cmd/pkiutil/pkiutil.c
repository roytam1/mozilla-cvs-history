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
#include "nspr.h"
#include "prtypes.h"
#include "prtime.h"
#include "prlong.h"
#include "nss.h"
#include "nsspki.h"
#include "nsspkix.h"

/* XXX */
#include "dev.h"

#include "pkiutil.h"

char *progName;

static PRStatus pkiutil_command_dispatcher(cmdCommand *, int);

/*  pkiutil commands  */
enum {
    cmd_ChangePassword = 0,
    cmd_Delete,
    cmd_Export,
    cmd_GenerateKeyPair,
    cmd_Import,
    cmd_Interactive,
    cmd_List,
    cmd_ListChain,
    cmd_ModifyTrust,
    cmd_NewDBs,
    cmd_Print,
    cmd_ScriptFile,
    cmd_Validate,
    cmd_Version,
    pkiutil_num_commands
};

/*  pkiutil options */
enum {
    opt_Help = 0,
    opt_Ascii,
    opt_ProfileDir,
    opt_TokenName,
    opt_InputFile,
    opt_Info,
    opt_KeyType,
    opt_Nickname,
    opt_OutputFile,
    opt_Orphans,
    opt_Password,
    opt_Binary,
    opt_Serial,
    opt_KeySize,
    opt_Trust,
    opt_Type,
    opt_Usages,
    opt_KeyPassword,
    pkiutil_num_options
};

static cmdCommandLineArg pkiutil_commands[] =
{
 { /* cmd_ChangePassword */
    0 , "change-password",
   CMDNoArg, 0, PR_FALSE,
   { 0, 0, 0, 0 },
   { 
     CMDBIT(opt_TokenName) |
     CMDBIT(opt_ProfileDir) |
     CMDBIT(opt_Password),
     0, 0, 0 
   },
   "Change the password of a token"
 },
 { /* cmd_Delete */
   'D', "delete", 
   CMDNoArg, 0, PR_FALSE, 
   { 
     CMDBIT(opt_Nickname) |
     0, 0, 0 
   },
   {
     CMDBIT(opt_ProfileDir) | 
     CMDBIT(opt_Orphans) | 
     CMDBIT(opt_Password) |
     CMDBIT(opt_TokenName),
     0, 0, 0
   },
   "Delete an object from the profile/token"
 },
 { /* cmd_Export */  
   'E', "export", 
   CMDNoArg, 0, PR_FALSE, 
   {
     CMDBIT(opt_Nickname) |
     CMDBIT(opt_Type),
     0, 0, 0
   },
   {
     CMDBIT(opt_Ascii) | 
     CMDBIT(opt_ProfileDir) | 
     CMDBIT(opt_TokenName) | 
     CMDBIT(opt_OutputFile) | 
     CMDBIT(opt_Password) |
     CMDBIT(opt_Binary) |
     CMDBIT(opt_KeyPassword),
     0, 0, 0
   },
   "Export an object from the profile/token\n"
   "  private-key ==> PKCS#8 Encrypted Private Key Info\n"
   "  certificate ==> DER-encoded Certificate"
 },
 { /* cmd_GenerateKeyPair */  
   'G', "generate-key-pair", 
   CMDNoArg, 0, PR_FALSE, 
   { 0, 0, 0, 0 },
   {
     CMDBIT(opt_KeyType) |
     CMDBIT(opt_Nickname) |
     CMDBIT(opt_ProfileDir) | 
     CMDBIT(opt_KeySize) | 
     CMDBIT(opt_Password) |
     CMDBIT(opt_TokenName) | 
     0, 0, 0
   },
   "Generate a public/private key pair on the token"
 },
 { /* cmd_Import */  
   'I', "import", 
   CMDNoArg, 0, PR_FALSE, 
   {
     CMDBIT(opt_Nickname),
     0, 0, 0
   },
   {
     CMDBIT(opt_Ascii) | 
     CMDBIT(opt_ProfileDir) | 
     CMDBIT(opt_TokenName) | 
     CMDBIT(opt_InputFile) | 
     CMDBIT(opt_KeyType) |
     CMDBIT(opt_Password) |
     CMDBIT(opt_Binary) | 
     CMDBIT(opt_Type) |
     CMDBIT(opt_KeyPassword),
     0, 0, 0
   },
   "Import an object into the profile/token"
 },
 { /* cmd_Interactive */
    0 , "interactive", 
   CMDNoArg, 0, PR_FALSE, 
   { 0, 0, 0, 0 },
   { 
     CMDBIT(opt_ProfileDir),
     0, 0, 0 
   },
   "Use interactive mode"
 },
 { /* cmd_List */  
   'L', "list", 
   CMDNoArg, 0, PR_FALSE, 
   { 
     0, 0, 0, 0 
   },
   {
     CMDBIT(opt_Ascii) | 
     CMDBIT(opt_ProfileDir) | 
     CMDBIT(opt_TokenName) | 
     CMDBIT(opt_Password) |
     CMDBIT(opt_Binary) | 
     CMDBIT(opt_Nickname) | 
     CMDBIT(opt_Type),
     0, 0, 0
   },
   "List objects on the token"
 },
 { /* cmd_ListChain */  
    0 , "list-chain", 
   CMDNoArg, 0, PR_FALSE, 
   { 
     CMDBIT(opt_Nickname),
     0, 0, 0 
   },
   {
     CMDBIT(opt_ProfileDir) |
     CMDBIT(opt_Serial),
     0, 0, 0
   },
   "List a certificate chain"
 },
 { /* cmd_ModifyTrust */
   'M', "modify-trust", 
   CMDNoArg, 0, PR_FALSE,
   {
     CMDBIT(opt_Nickname) |
     CMDBIT(opt_Usages),
     0, 0, 0
   },
   {
     CMDBIT(opt_ProfileDir) | 
     CMDBIT(opt_Serial),
     0, 0, 0
   },
   "Modify the trusted usages of a certificate"
 },
 { /* cmd_NewDBs */
   'N', "newdbs",
   CMDNoArg, 0, PR_FALSE,
   { 0, 0, 0, 0 },
   {
     CMDBIT(opt_ProfileDir),
     0, 0, 0
   },
   "Create new security databases"
 },
 { /* cmd_Print */
   'P', "print", 
   CMDNoArg, 0, PR_FALSE,
   {
     CMDBIT(opt_Nickname),
     0, 0, 0
   },
   {
     CMDBIT(opt_Info) | 
     CMDBIT(opt_ProfileDir) | 
     CMDBIT(opt_OutputFile) | 
     CMDBIT(opt_Password) |
     CMDBIT(opt_Serial) | 
     CMDBIT(opt_Type),
     0, 0, 0
   },
   "Print or dump a single object"
 },
 { /* cmd_ScriptFile */
    0 , "script-file", 
   CMDArgReq, 0, PR_FALSE,
   { 0, 0, 0, 0 },
   {
     CMDBIT(opt_ProfileDir),
     0, 0, 0
   },
   "Run a script file containing a set of commands"
 },
 { /* cmd_Validate */
   'V', "validate", 
   CMDNoArg, 0, PR_FALSE,
   {
     CMDBIT(opt_Nickname),
     0, 0, 0
   },
   {
     CMDBIT(opt_Info) | 
     CMDBIT(opt_ProfileDir) | 
     CMDBIT(opt_Serial) | 
     CMDBIT(opt_Usages) | 
     0, 0, 0
   },
   "Validate a certificate"
 },
 { /* cmd_Version */  
   0, "version", 
   CMDNoArg, 0, PR_FALSE, 
   { 0, 0, 0, 0 }, 
   { 0, 0, 0, 0 },
   "Get version information"
 }
};

static cmdCommandLineOpt pkiutil_options[] =
{
 { /* opt_Help        */  '?', "help",     CMDNoArg  },
 { /* opt_Ascii       */  'a', "ascii",    CMDNoArg  },
 { /* opt_ProfileDir  */  'd', "dbdir",    CMDArgReq },
 { /* opt_TokenName   */  'h', "token",    CMDArgReq },
 { /* opt_InputFile   */  'i', "infile",   CMDArgReq },
 { /* opt_Info        */   0 , "info",     CMDNoArg  },
 { /* opt_KeyType     */  'k', "key-type", CMDArgReq },
 { /* opt_Nickname    */  'n', "nickname", CMDArgReq },
 { /* opt_OutputFile  */  'o', "outfile",  CMDArgReq },
 { /* opt_Orphans     */   0 , "orphans",  CMDNoArg  },
 { /* opt_Password    */  'p', "password", CMDArgReq },
 { /* opt_Binary      */  'r', "raw",      CMDNoArg  },
 { /* opt_Serial      */   0 , "serial",   CMDArgReq },
 { /* opt_KeySize     */   0 , "size",     CMDArgReq },
 { /* opt_Trust       */  't', "trust",    CMDArgReq },
 { /* opt_Type        */   0 , "type",     CMDArgReq },
 { /* opt_Usages      */  'u', "usages",   CMDArgReq },
 { /* opt_KeyPassword */  'w', "keypass",  CMDArgReq },
};

static char * pkiutil_options_help[] =
{
 "get help for command",
 "use ascii (base-64 encoded) mode for I/O",
 "directory containing security databases (default: \"./\")",
 "name of PKCS#11 token to use (default: internal or all)",
 "file for input (default: stdin)",
 "print object-specific information (token instances, etc.)",
 "type of key [rsa|dsa|dh] (default: rsa)",
 "nickname of object",
 "file for output (default: stdout)",
 "delete orphaned key pairs (keys not associated with a cert)",
 "specify a slot password at the command line",
 "use raw (binary der-encoded) mode for I/O",
 "specify a certificate serial number",
 "size of key pair in bits [rsa modulus, etc.] (default: 1024)",
 "trust level for certificate",
 "specify type of object"
  "\n certificate (default)"
  "\n public-key"
  "\n private-key"
  "\n all",
 "specify a set of certificate usages"
  "\n c - SSL client"
  "\n v - SSL server"
  "\n r - Email recipient"
  "\n s - Email signer"
  "\n o - Code signer"
  "\n t - Status responder"
  "\n u - SSL server with step-up"
  "\n  (capital letters specify CA equivalents)",
 "passphrase that protects encoded private key"
};

static char pkiutil_description[] =
"utility for managing PKI objects";

int 
main(int argc, char **argv)
{
    char     *profiledir = "./";
    PRStatus  rv         = PR_SUCCESS;

    int cmdToRun;
    cmdCommand pkiutil;
    pkiutil.ncmd = pkiutil_num_commands;
    pkiutil.nopt = pkiutil_num_options;
    pkiutil.cmd = pkiutil_commands;
    pkiutil.opt = pkiutil_options;
    pkiutil.optHelp = pkiutil_options_help;
    pkiutil.description = pkiutil_description;

    progName = strrchr(argv[0], '/');
    if (!progName) {
	progName = strrchr(argv[0], '\\');
    }
    progName = progName ? progName+1 : argv[0];

    cmdToRun = CMD_ParseCommandLine(argc, argv, progName, &pkiutil);

#if 0
    { int i, nc;
    for (i=0; i<pkiutil.ncmd; i++)
	printf("%s: %s <%s>\n", pkiutil.cmd[i].s, 
	                        (pkiutil.cmd[i].on) ? "on" : "off",
				pkiutil.cmd[i].arg);
    for (i=0; i<pkiutil.nopt; i++)
	printf("%s: %s <%s>\n", pkiutil.opt[i].s, 
	                        (pkiutil.opt[i].on) ? "on" : "off",
				pkiutil.opt[i].arg);
    }
#endif

    if (pkiutil.opt[opt_Help].on)
	CMD_LongUsage(progName, &pkiutil);

    if (cmdToRun < 0) {
	CMD_Usage(progName, &pkiutil);
	exit(1);
    }

    /* Display version info and exit */
    if (cmdToRun == cmd_Version) {
	printf("%s\nNSS Version %s\n", PKIUTIL_VERSION_STRING, NSS_VERSION);
	return PR_SUCCESS;
    }

    /* -d */
    if (pkiutil.opt[opt_ProfileDir].on) {
	profiledir = strdup(pkiutil.opt[opt_ProfileDir].arg);
    }

    /* initialize */
    PR_Init(PR_SYSTEM_THREAD, PR_PRIORITY_NORMAL, 1);

    /* XXX allow for read-only and no-db */
    rv = NSS_InitReadWrite(profiledir);
    if (rv == PR_FAILURE) {
	CMD_PrintError("Failed to initialize NSS");
	exit(1);
    }

    if (cmdToRun == cmd_NewDBs) {
	/* just creating new dbs, done */
	goto shutdown;
    }

    /* XXX */
    rv = NSS_EnablePKIXCertificates();
    if (rv == PR_FAILURE) {
	CMD_PrintError("Failed to load PKIX module");
	goto shutdown;
    }

    if (cmdToRun == cmd_Interactive) {
	while (PR_TRUE) {
	    cmdToRun = CMD_Interactive(&pkiutil);
	    if (cmdToRun == -1 || pkiutil.opt[opt_Help].on) {
		CMD_InteractiveUsage(progName, &pkiutil);
		continue;
	    } else if (cmdToRun == -2) {
		break;
	    }
	    rv = pkiutil_command_dispatcher(&pkiutil, cmdToRun);
	}
    } else if (cmdToRun == cmd_ScriptFile) {
	PRFileDesc *scriptFile;
	scriptFile = PR_Open(pkiutil.cmd[cmd_ScriptFile].arg, PR_RDONLY, 0);
	if (!scriptFile) {
	    PR_fprintf(PR_STDERR, "Failed to open script file\n");
	    rv = PR_FAILURE;
	    goto shutdown;
	}
	while (PR_TRUE) {
	    cmdToRun = CMD_GetNextScriptCommand(&pkiutil, scriptFile);
	    if (cmdToRun < 0) break;
	    rv = pkiutil_command_dispatcher(&pkiutil, cmdToRun);
	    if (rv == PR_FAILURE) break;
	}
	PR_Close(scriptFile);
    } else {
	rv = pkiutil_command_dispatcher(&pkiutil, cmdToRun);
    }

shutdown:
    NSS_Shutdown();

    if (pkiutil.opt[opt_ProfileDir].on) {
	free(profiledir);
    }

    return rv;
}

static PRStatus 
pkiutil_command_dispatcher(cmdCommand *pkiutil, int cmdToRun)
{
    PRStatus status;
    CMDRunTimeData rtData;
    NSSTrustDomain *td = NSS_GetDefaultTrustDomain();
    NSSSlot *slot = NULL;
    NSSToken *token = NULL;
    NSSCallback *pwcb;
    char *inMode;
    char *outMode;

    if (pkiutil->opt[opt_Ascii].on) {
	inMode = outMode = "ascii";
    } else if (pkiutil->opt[opt_Binary].on) {
	inMode = outMode = "binary";
    } else {
	/* default I/O is binary input, pretty-print output */
	inMode = "binary";
	outMode = "pretty-print";
    }

    pwcb = CMD_GetDefaultPasswordCallback(pkiutil->opt[opt_Password].arg, 
                                          NULL);
    if (!pwcb) {
	return PR_FAILURE;
    }
    status = NSSTrustDomain_SetDefaultCallback(td, pwcb, NULL);
    if (status != PR_SUCCESS) {
	return status;
    }
    status = CMD_SetRunTimeData(pkiutil->opt[opt_InputFile].arg, NULL, inMode,
                                pkiutil->opt[opt_OutputFile].arg, outMode,
                                &rtData);
    if (status != PR_SUCCESS) {
	return status;
    }
    if (pkiutil->opt[opt_TokenName].on) {
	NSSUTF8 *tokenOrSlotName = pkiutil->opt[opt_TokenName].arg;
	/* First try by slot name */
	slot = NSSTrustDomain_FindSlotByName(td, tokenOrSlotName);
	if (slot) {
	    token = NSSSlot_GetToken(slot);
	} else {
	    token = NSSTrustDomain_FindTokenByName(td, tokenOrSlotName);
	    if (token) {
		slot = NSSToken_GetSlot(token);
	    }
	}
    }
    switch (cmdToRun) {
    case cmd_ChangePassword:
	if (!slot) {
	    /* XXX */
	    token = nss_GetDefaultDatabaseToken();
	    slot = NSSToken_GetSlot(token);
	}
	if (pkiutil->opt[opt_Password].on) {
	    status = NSSSlot_SetPassword(slot, NULL, 
	                                 pkiutil->opt[opt_Password].arg);
	} else {
	    status = CMD_ChangeSlotPassword(slot);
	}
	break;
    case cmd_Delete:
	if (pkiutil->opt[opt_Orphans].on) {
	    status = DeleteOrphanedKeyPairs(td, NULL, &rtData);
	    break;
	}
	status = DeleteObject(td,
	                      NULL,
	                      NULL,
	                      pkiutil->opt[opt_Nickname].arg);
	break;
    case cmd_Export:
	status = ExportObject(td,
	                      token,
	                      pkiutil->opt[opt_Type].arg,
	                      pkiutil->opt[opt_Nickname].arg,
	                      pkiutil->opt[opt_KeyPassword].arg,
	                      &rtData);
	break;
    case cmd_GenerateKeyPair:
	status = GenerateKeyPair(td,
	                         token,
	                         pkiutil->opt[opt_KeyType].arg,
	                         pkiutil->opt[opt_KeySize].arg,
	                         pkiutil->opt[opt_Nickname].arg,
	                         &rtData);
	break;
    case cmd_Import:
	status = ImportObject(td,
	                      token,
	                      pkiutil->opt[opt_Type].arg,
	                      pkiutil->opt[opt_Nickname].arg,
	                      pkiutil->opt[opt_KeyType].arg,
	                      pkiutil->opt[opt_KeyPassword].arg,
	                      &rtData);
	break;
    case cmd_List:
	status = ListObjects(td,
	                     NULL,
	                     pkiutil->opt[opt_Type].arg,
	                     pkiutil->opt[opt_Nickname].arg,
	                     0,
	                     &rtData);
	break;
    case cmd_ListChain:
	status = ListChain(td,
	                   pkiutil->opt[opt_Nickname].arg,
	                   pkiutil->opt[opt_Serial].arg,
	                   0,
	                   &rtData);
	break;
    case cmd_ModifyTrust:
	status = SetCertTrust(td,
	                      pkiutil->opt[opt_Nickname].arg,
	                      pkiutil->opt[opt_Serial].arg,
			      pkiutil->opt[opt_Usages].arg);
	break;
    case cmd_Print:
	status = DumpObject(td,
	                    pkiutil->opt[opt_Type].arg,
	                    pkiutil->opt[opt_Nickname].arg,
	                    pkiutil->opt[opt_Serial].arg,
			    pkiutil->opt[opt_Info].on,
	                    &rtData);
	break;
    case cmd_Validate:
	status = ValidateCert(td,
	                      pkiutil->opt[opt_Nickname].arg,
	                      pkiutil->opt[opt_Serial].arg,
			      pkiutil->opt[opt_Usages].arg,
			      pkiutil->opt[opt_Info].on,
	                      &rtData);
	break;
    default:
	status = PR_FAILURE;
	break;
    }
    CMD_FinishRunTimeData(&rtData);
    CMD_DestroyCallback(pwcb);
    if (slot) {
	NSSSlot_Destroy(slot);
    }
    if (token) {
	NSSToken_Destroy(token);
    }
    return status;
}

