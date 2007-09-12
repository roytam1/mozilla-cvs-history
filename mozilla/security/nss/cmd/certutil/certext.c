/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Netscape security libraries.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1994-2000
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Dr Vipul Gupta <vipul.gupta@sun.com>, Sun Microsystems Laboratories
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

/*
** certext.c
**
** part of certutil for managing certificates extensions
**
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#if defined(WIN32)
#include "fcntl.h"
#include "io.h"
#endif

#include "secutil.h"

#if defined(XP_UNIX)
#include <unistd.h>
#endif

#include "cert.h"

#define GEN_BREAK(e) rv=e; break;

extern char *progName;

static char *
Gets_s(char *buff, size_t size) {
    char *str;
    
    if (buff == NULL || size < 1) {
        PORT_Assert(0);
        return NULL;
    }
    if ((str = fgets(buff, size, stdin)) != NULL) {
        int len = PORT_Strlen(str);
        /*
         * fgets() automatically converts native text file
         * line endings to '\n'.  As defensive programming
         * (just in case fgets has a bug or we put stdin in
         * binary mode by mistake), we handle three native 
         * text file line endings here:
         *   '\n'      Unix (including Linux and Mac OS X)
         *   '\r''\n'  DOS/Windows & OS/2
         *   '\r'      Mac OS Classic
         * len can not be less then 1, since in case with
         * empty string it has at least '\n' in the buffer
         */
        if (buff[len - 1] == '\n' || buff[len - 1] == '\r') {
            buff[len - 1] = '\0';
            if (len > 1 && buff[len - 2] == '\r')
                buff[len - 2] = '\0';
        }
    } else {
        buff[0] = '\0';
    }
    return str;
}

static CERTGeneralName *
GetGeneralName (PRArenaPool *arena)
{
    CERTGeneralName *namesList = NULL;
    CERTGeneralName *current;
    CERTGeneralName *tail = NULL;
    SECStatus rv = SECSuccess;
    int intValue;
    char buffer[512];
    void *mark;

    PORT_Assert (arena);
    mark = PORT_ArenaMark (arena);
    do {
        puts ("\nSelect one of the following general name type: \n");
        puts ("\t1 - instance of other name\n\t2 - rfc822Name\n\t3 - "
              "dnsName\n");
        puts ("\t4 - x400Address\n\t5 - directoryName\n\t6 - ediPartyName\n");
        puts ("\t7 - uniformResourceidentifier\n\t8 - ipAddress\n\t9 - "
              "registerID\n");
        puts ("\tAny other number to finish\n\t\tChoice:");
        if (Gets_s (buffer, sizeof(buffer)) == NULL) {
            PORT_SetError(SEC_ERROR_INPUT_LEN);
            GEN_BREAK (SECFailure);
        }
        intValue = PORT_Atoi (buffer);
        /*
         * Should use ZAlloc instead of Alloc to avoid problem with garbage
         * initialized pointers in CERT_CopyName
         */
        if (intValue >= certOtherName && intValue <= certRegisterID) {
            if (namesList == NULL) {
                namesList = current = tail =
                    PORT_ArenaZNew(arena, CERTGeneralName);
            } else {
                current = PORT_ArenaZNew(arena, CERTGeneralName);
            }
            if (current == NULL) {
                GEN_BREAK (SECFailure);
            }
        } else {
            break;
        }
        current->type = intValue;
        puts ("\nEnter data:");
        fflush (stdout);
        if (Gets_s (buffer, sizeof(buffer)) == NULL) {
            PORT_SetError(SEC_ERROR_INPUT_LEN);
            GEN_BREAK (SECFailure);
        }
        switch (current->type) {
        case certURI:
        case certDNSName:
        case certRFC822Name:
            current->name.other.data =
                PORT_ArenaAlloc (arena, strlen (buffer));
            if (current->name.other.data == NULL) {
                GEN_BREAK (SECFailure);
            }
            PORT_Memcpy(current->name.other.data, buffer,
                        current->name.other.len = strlen(buffer));
            break;

        case certEDIPartyName:
        case certIPAddress:
        case certOtherName:
        case certRegisterID:
        case certX400Address: {

            current->name.other.data =
                PORT_ArenaAlloc (arena, strlen (buffer) + 2);
            if (current->name.other.data == NULL) {
                GEN_BREAK (SECFailure);
            }
            
            PORT_Memcpy (current->name.other.data + 2, buffer,
                         strlen (buffer));
            /* This may not be accurate for all cases.  For now,
             * use this tag type */
            current->name.other.data[0] =
                (char)(((current->type - 1) & 0x1f)| 0x80);
            current->name.other.data[1] = (char)strlen (buffer);
            current->name.other.len = strlen (buffer) + 2;
            break;
        }

        case certDirectoryName: {
            CERTName *directoryName = NULL;
            
            directoryName = CERT_AsciiToName (buffer);
            if (!directoryName) {
                fprintf(stderr, "certutil: improperly formatted name: "
                        "\"%s\"\n", buffer);
                break;
            }
            
            rv = CERT_CopyName (arena, &current->name.directoryName,
                                directoryName);
            CERT_DestroyName (directoryName);
            
            break;
        }
        }
        if (rv != SECSuccess)
            break;
        current->l.next = &(namesList->l);
        current->l.prev = &(tail->l);
        tail->l.next = &(current->l);
        tail = current;
        
    }while (1);

    if (rv != SECSuccess) {
        PORT_ArenaRelease (arena, mark);
        namesList = NULL;
    }
    return (namesList);
}

static SECStatus 
GetString(PRArenaPool *arena, char *prompt, SECItem *value)
{
    char buffer[251];
    char *buffPrt;

    buffer[0] = '\0';
    value->data = NULL;
    value->len = 0;
    
    puts (prompt);
    buffPrt = Gets_s (buffer, sizeof(buffer));
    /* returned NULL here treated the same way as empty string */
    if (buffPrt && strlen (buffer) > 0) {
        value->data = PORT_ArenaAlloc (arena, strlen (buffer));
        if (value->data == NULL) {
            PORT_SetError (SEC_ERROR_NO_MEMORY);
            return (SECFailure);
        }
        PORT_Memcpy (value->data, buffer, value->len = strlen(buffer));
    }
    return (SECSuccess);
}

static PRBool 
GetYesNo(char *prompt) 
{
    char buf[3];
    char *buffPrt;

    buf[0] = 'n';
    puts(prompt);
    buffPrt = Gets_s(buf, sizeof(buf));
    return (buffPrt && (buf[0] == 'y' || buf[0] == 'Y')) ? PR_TRUE : PR_FALSE;
}

static SECStatus 
AddKeyUsage (void *extHandle)
{
    SECItem bitStringValue;
    unsigned char keyUsage = 0x0;
    char buffer[5];
    int value;
    PRBool yesNoAns;

    while (1) {
        fprintf(stdout, "%-25s 0 - Digital Signature\n", "");
        fprintf(stdout, "%-25s 1 - Non-repudiation\n", "");
        fprintf(stdout, "%-25s 2 - Key encipherment\n", "");
        fprintf(stdout, "%-25s 3 - Data encipherment\n", "");   
        fprintf(stdout, "%-25s 4 - Key agreement\n", "");
        fprintf(stdout, "%-25s 5 - Cert signing key\n", "");   
        fprintf(stdout, "%-25s 6 - CRL signing key\n", "");
        fprintf(stdout, "%-25s Other to finish\n", "");
        if (Gets_s (buffer, sizeof(buffer))) {
            value = PORT_Atoi (buffer);
            if (value < 0 || value > 6)
                break;
            if (value == 0) {
                /* Checking that zero value of variable 'value'
                 * corresponds to '0' input made by user */
                char *chPtr = strchr(buffer, '0');
                if (chPtr == NULL) {
                    continue;
                }
            }
            keyUsage |= (0x80 >> value);
        }
        else {/* gets() returns NULL on EOF or error */
            break;
        }
    }

    bitStringValue.data = &keyUsage;
    bitStringValue.len = 1;
    yesNoAns = GetYesNo("Is this a critical extension [y/N]?");

    return (CERT_EncodeAndAddBitStrExtension
            (extHandle, SEC_OID_X509_KEY_USAGE, &bitStringValue,
             yesNoAns));

}


static CERTOidSequence *
CreateOidSequence(void)
{
    CERTOidSequence *rv = (CERTOidSequence *)NULL;
    PRArenaPool *arena = (PRArenaPool *)NULL;

    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if( (PRArenaPool *)NULL == arena ) {
        goto loser;
    }

    rv = (CERTOidSequence *)PORT_ArenaZAlloc(arena, sizeof(CERTOidSequence));
    if( (CERTOidSequence *)NULL == rv ) {
        goto loser;
    }

    rv->oids = (SECItem **)PORT_ArenaZAlloc(arena, sizeof(SECItem *));
    if( (SECItem **)NULL == rv->oids ) {
        goto loser;
    }

    rv->arena = arena;
    return rv;

loser:
    if( (PRArenaPool *)NULL != arena ) {
        PORT_FreeArena(arena, PR_FALSE);
    }

    return (CERTOidSequence *)NULL;
}

static void
DestroyOidSequence(CERTOidSequence *os)
{
    if (os->arena) {
        PORT_FreeArena(os->arena, PR_FALSE);
    }
}

static SECStatus
AddOidToSequence(CERTOidSequence *os, SECOidTag oidTag)
{
    SECItem **oids;
    PRUint32 count = 0;
    SECOidData *od;

    od = SECOID_FindOIDByTag(oidTag);
    if( (SECOidData *)NULL == od ) {
        return SECFailure;
    }

    for( oids = os->oids; (SECItem *)NULL != *oids; oids++ ) {
        count++;
    }

    /* ArenaZRealloc */

    {
        PRUint32 i;

        oids = (SECItem **)PORT_ArenaZAlloc(os->arena,
                                            sizeof(SECItem *) * (count+2));
        if( (SECItem **)NULL == oids ) {
            return SECFailure;
        }
    
        for( i = 0; i < count; i++ ) {
            oids[i] = os->oids[i];
        }

        /* ArenaZFree(os->oids); */
    }

    os->oids = oids;
    os->oids[count] = &od->oid;

    return SECSuccess;
}

SEC_ASN1_MKSUB(SEC_ObjectIDTemplate)

  const SEC_ASN1Template CERT_OidSeqTemplate[] = {
      { SEC_ASN1_SEQUENCE_OF | SEC_ASN1_XTRN,
        offsetof(CERTOidSequence, oids),
        SEC_ASN1_SUB(SEC_ObjectIDTemplate) }
  };


static SECItem *
EncodeOidSequence(CERTOidSequence *os)
{
    SECItem *rv;

    rv = (SECItem *)PORT_ArenaZAlloc(os->arena, sizeof(SECItem));
    if( (SECItem *)NULL == rv ) {
        goto loser;
    }

    if( !SEC_ASN1EncodeItem(os->arena, rv, os, CERT_OidSeqTemplate) ) {
        goto loser;
    }

    return rv;

loser:
    return (SECItem *)NULL;
}

static SECStatus 
AddExtKeyUsage (void *extHandle)
{
    char buffer[5];
    int value;
    CERTOidSequence *os;
    SECStatus rv;
    SECItem *item;
    PRBool yesNoAns;

    os = CreateOidSequence();
    if( (CERTOidSequence *)NULL == os ) {
        return SECFailure;
    }

    while (1) {
        fprintf(stdout, "%-25s 0 - Server Auth\n", "");
        fprintf(stdout, "%-25s 1 - Client Auth\n", "");
        fprintf(stdout, "%-25s 2 - Code Signing\n", "");
        fprintf(stdout, "%-25s 3 - Email Protection\n", "");
        fprintf(stdout, "%-25s 4 - Timestamp\n", "");
        fprintf(stdout, "%-25s 5 - OCSP Responder\n", "");
        fprintf(stdout, "%-25s 6 - Step-up\n", "");
        fprintf(stdout, "%-25s Other to finish\n", "");

        if (Gets_s(buffer, sizeof(buffer)) == NULL) {
            PORT_SetError(SEC_ERROR_INPUT_LEN);
            rv = SECFailure;
            goto loser;
        }
        value = PORT_Atoi(buffer);

        if (value == 0) {
            /* Checking that zero value of variable 'value'
             * corresponds to '0' input made by user */
            char *chPtr = strchr(buffer, '0');
            if (chPtr == NULL) {
                continue;
            }
        }

        switch( value ) {
        case 0:
            rv = AddOidToSequence(os, SEC_OID_EXT_KEY_USAGE_SERVER_AUTH);
            break;
        case 1:
            rv = AddOidToSequence(os, SEC_OID_EXT_KEY_USAGE_CLIENT_AUTH);
            break;
        case 2:
            rv = AddOidToSequence(os, SEC_OID_EXT_KEY_USAGE_CODE_SIGN);
            break;
        case 3:
            rv = AddOidToSequence(os, SEC_OID_EXT_KEY_USAGE_EMAIL_PROTECT);
            break;
        case 4:
            rv = AddOidToSequence(os, SEC_OID_EXT_KEY_USAGE_TIME_STAMP);
            break;
        case 5:
            rv = AddOidToSequence(os, SEC_OID_OCSP_RESPONDER);
            break;
        case 6:
            rv = AddOidToSequence(os, SEC_OID_NS_KEY_USAGE_GOVT_APPROVED);
            break;
        default:
            goto endloop;
        }

        if( SECSuccess != rv ) goto loser;
    }

endloop:
    item = EncodeOidSequence(os);

    yesNoAns = GetYesNo("Is this a critical extension [y/N]?");

    rv = CERT_AddExtension(extHandle, SEC_OID_X509_EXT_KEY_USAGE, item,
                           yesNoAns, PR_TRUE);
    /*FALLTHROUGH*/
loser:
    DestroyOidSequence(os);
    return rv;
}

static SECStatus 
AddNscpCertType (void *extHandle)
{
    SECItem bitStringValue;
    unsigned char keyUsage = 0x0;
    char buffer[5];
    int value;
    PRBool yesNoAns;

    while (1) {
        fprintf(stdout, "%-25s 0 - SSL Client\n", "");
        fprintf(stdout, "%-25s 1 - SSL Server\n", "");
        fprintf(stdout, "%-25s 2 - S/MIME\n", "");
        fprintf(stdout, "%-25s 3 - Object Signing\n", "");   
        fprintf(stdout, "%-25s 4 - Reserved for future use\n", "");
        fprintf(stdout, "%-25s 5 - SSL CA\n", "");   
        fprintf(stdout, "%-25s 6 - S/MIME CA\n", "");
        fprintf(stdout, "%-25s 7 - Object Signing CA\n", "");
        fprintf(stdout, "%-25s Other to finish\n", "");
        if (Gets_s (buffer, sizeof(buffer)) == NULL) {
            PORT_SetError(SEC_ERROR_INPUT_LEN);
            return SECFailure;
        }
        value = PORT_Atoi (buffer);
        if (value < 0 || value > 7)
            break;
        if (value == 0) {
            /* Checking that zero value of variable 'value'
             * corresponds to '0' input made by user */
            char *chPtr = strchr(buffer, '0');
            if (chPtr == NULL) {
                continue;
            }
        }
        keyUsage |= (0x80 >> value);
    }

    bitStringValue.data = &keyUsage;
    bitStringValue.len = 1;
    yesNoAns = GetYesNo("Is this a critical extension [y/N]?");

    return (CERT_EncodeAndAddBitStrExtension
            (extHandle, SEC_OID_NS_CERT_EXT_CERT_TYPE, &bitStringValue,
             yesNoAns));

}

static SECStatus 
AddSubjectAltNames(PRArenaPool *arena, CERTGeneralName **existingListp,
                   const char *names, CERTGeneralNameType type)
{
    CERTGeneralName *nameList = NULL;
    CERTGeneralName *current = NULL;
    PRCList *prev = NULL;
    const char *cp;
    char *tbuf;
    SECStatus rv = SECSuccess;


    /*
     * walk down the comma separated list of names. NOTE: there is
     * no sanity checks to see if the email address look like
     * email addresses.
     */
    for (cp=names; cp; cp = PORT_Strchr(cp,',')) {
        int len;
        char *end;

        if (*cp == ',') {
            cp++;
        }
        end = PORT_Strchr(cp,',');
        len = end ? end-cp : PORT_Strlen(cp);
        if (len <= 0) {
            continue;
        }
        tbuf = PORT_ArenaAlloc(arena,len+1);
        PORT_Memcpy(tbuf,cp,len);
        tbuf[len] = 0;
        current = (CERTGeneralName *) PORT_ZAlloc(sizeof(CERTGeneralName));
        if (!current) {
            rv = SECFailure;
            break;
        }
        if (prev) {
            current->l.prev = prev;
            prev->next = &(current->l);
        } else {
            nameList = current;
        }
        current->type = type;
        current->name.other.data = (unsigned char *)tbuf;
        current->name.other.len = PORT_Strlen(tbuf);
        prev = &(current->l);
    }
    /* at this point nameList points to the head of a doubly linked,
     * but not yet circular, list and current points to its tail. */
    if (rv == SECSuccess && nameList) {
        if (*existingListp != NULL) {
            PRCList *existingprev;
            /* add nameList to the end of the existing list */
            existingprev = (*existingListp)->l.prev;
            (*existingListp)->l.prev = &(current->l);
            nameList->l.prev = existingprev;
            existingprev->next = &(nameList->l);
            current->l.next = &((*existingListp)->l);
        }
        else {
            /* make nameList circular and set it as the new existingList */
            nameList->l.prev = prev;
            current->l.next = &(nameList->l);
            *existingListp = nameList;
        }
    }
    return rv;
}

static SECStatus 
AddEmailSubjectAlt(PRArenaPool *arena, CERTGeneralName **existingListp,
                   const char *emailAddrs)
{
    return AddSubjectAltNames(arena, existingListp, emailAddrs, 
                              certRFC822Name);
}

static SECStatus 
AddDNSSubjectAlt(PRArenaPool *arena, CERTGeneralName **existingListp,
                 const char *dnsNames)
{
    return AddSubjectAltNames(arena, existingListp, dnsNames, certDNSName);
}


static SECStatus 
AddBasicConstraint(void *extHandle)
{
    CERTBasicConstraints basicConstraint;    
    SECItem encodedValue;
    SECStatus rv;
    char buffer[10];
    PRBool yesNoAns;

    encodedValue.data = NULL;
    encodedValue.len = 0;
    do {
        basicConstraint.pathLenConstraint = CERT_UNLIMITED_PATH_CONSTRAINT;
        basicConstraint.isCA = GetYesNo ("Is this a CA certificate [y/N]?");

        buffer[0] = '\0';
        puts ("Enter the path length constraint, enter to skip "
              "[<0 for unlimited path]:");
        Gets_s (buffer, sizeof(buffer));
        if (PORT_Strlen (buffer) > 0)
            basicConstraint.pathLenConstraint = PORT_Atoi (buffer);

        rv = CERT_EncodeBasicConstraintValue (NULL, &basicConstraint,
                                              &encodedValue);
        if (rv)
            return (rv);

        yesNoAns = GetYesNo ("Is this a critical extension [y/N]?");

        rv = CERT_AddExtension(extHandle, SEC_OID_X509_BASIC_CONSTRAINTS,
                               &encodedValue, yesNoAns, PR_TRUE);
    } while (0);
    PORT_Free (encodedValue.data);
    return (rv);
}

static SECStatus 
AddAuthKeyID (void *extHandle)
{
    CERTAuthKeyID *authKeyID = NULL;    
    PRArenaPool *arena = NULL;
    SECStatus rv = SECSuccess;
    PRBool yesNoAns;

    do {
        arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
        if ( !arena ) {
            SECU_PrintError(progName, "out of memory");
            GEN_BREAK (SECFailure);
        }

        if (GetYesNo ("Enter value for the authKeyID extension [y/N]?") == 0)
            break;

        authKeyID = PORT_ArenaZAlloc (arena, sizeof (CERTAuthKeyID));
        if (authKeyID == NULL) {
            GEN_BREAK (SECFailure);
        }

        rv = GetString (arena, "Enter value for the key identifier fields,"
                        "enter to omit:", &authKeyID->keyID);
        if (rv != SECSuccess)
            break;
        authKeyID->authCertIssuer = GetGeneralName (arena);
        if (authKeyID->authCertIssuer == NULL && 
            SECFailure == PORT_GetError ())
            break;


        rv = GetString (arena, "Enter value for the authCertSerial field, "
                        "enter to omit:", &authKeyID->authCertSerialNumber);

        yesNoAns = GetYesNo ("Is this a critical extension [y/N]?");

        rv = SECU_EncodeAndAddExtensionValue(arena, extHandle,
                                             authKeyID, yesNoAns,
                                             SEC_OID_X509_AUTH_KEY_ID, 
                                             (EXTEN_EXT_VALUE_ENCODER)
                                             CERT_EncodeAuthKeyID);
        if (rv)
            break;

    } while (0);
    if (arena)
        PORT_FreeArena (arena, PR_FALSE);
    return (rv);
}   
    
static SECStatus 
AddCrlDistPoint(void *extHandle)
{
    PRArenaPool *arena = NULL;
    CERTCrlDistributionPoints *crlDistPoints = NULL;
    CRLDistributionPoint *current;
    SECStatus rv = SECSuccess;
    int count = 0, intValue;
    char buffer[512];

    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( !arena )
        return (SECFailure);

    do {
        current = NULL;

        current = PORT_ArenaZAlloc (arena, sizeof (*current));
        if (current == NULL) {
            GEN_BREAK (SECFailure);
        }   

        /* Get the distributionPointName fields - this field is optional */
        puts ("Enter the type of the distribution point name:\n");
        puts ("\t1 - Full Name\n\t2 - Relative Name\n\tAny other "
              "number to finish\n\t\tChoice: ");
        if (Gets_s (buffer, sizeof(buffer)) == NULL) {
            PORT_SetError(SEC_ERROR_INPUT_LEN);
            GEN_BREAK (SECFailure);
        }
        intValue = PORT_Atoi (buffer);
        switch (intValue) {
        case generalName:
            current->distPointType = intValue;
            current->distPoint.fullName = GetGeneralName (arena);
            rv = PORT_GetError();
            break;

        case relativeDistinguishedName: {
            CERTName *name;

            current->distPointType = intValue;
            puts ("Enter the relative name: ");
            fflush (stdout);
            if (Gets_s (buffer, sizeof(buffer)) == NULL) {
                GEN_BREAK (SECFailure);
            }
            /* For simplicity, use CERT_AsciiToName to converse from a string
               to NAME, but we only interest in the first RDN */
            name = CERT_AsciiToName (buffer);
            if (!name) {
                GEN_BREAK (SECFailure);
            }
            rv = CERT_CopyRDN (arena, &current->distPoint.relativeName,
                               name->rdns[0]);
            CERT_DestroyName (name);
            break;
          }
        }
        if (rv != SECSuccess)
            break;

        /* Get the reason flags */
        puts ("\nSelect one of the following for the reason flags\n");
        puts ("\t0 - unused\n\t1 - keyCompromise\n\t2 - caCompromise\n"
              "\t3 - affiliationChanged\n");
        puts ("\t4 - superseded\n\t5 - cessationOfOperation\n"
              "\t6 - certificateHold\n");
        puts ("\tAny other number to finish\t\tChoice: ");

        if (Gets_s (buffer, sizeof(buffer)) == NULL) {
            PORT_SetError(SEC_ERROR_INPUT_LEN);
            GEN_BREAK (SECFailure);
        }
        intValue = PORT_Atoi (buffer);
        if (intValue == 0) {
            /* Checking that zero value of variable 'value'
             * corresponds to '0' input made by user */
            char *chPtr = strchr(buffer, '0');
            if (chPtr == NULL) {
                intValue = -1;
            }
        }
        if (intValue >= 0 && intValue <8) {
            current->reasons.data = PORT_ArenaAlloc (arena, sizeof(char));
            if (current->reasons.data == NULL) {
                GEN_BREAK (SECFailure);
            }
            *current->reasons.data = (char)(0x80 >> intValue);
            current->reasons.len = 1;
        }
        puts ("Enter value for the CRL Issuer name:\n");
        current->crlIssuer = GetGeneralName (arena);
        if (current->crlIssuer == NULL && (rv = PORT_GetError()) == SECFailure)
            break;

        if (crlDistPoints == NULL) {
            crlDistPoints = PORT_ArenaZAlloc (arena, sizeof (*crlDistPoints));
            if (crlDistPoints == NULL) {
                GEN_BREAK (SECFailure);
            }
        }
    
        crlDistPoints->distPoints =
            PORT_ArenaGrow (arena, crlDistPoints->distPoints,
                            sizeof (*crlDistPoints->distPoints) * count,
                            sizeof (*crlDistPoints->distPoints) *(count + 1));
        if (crlDistPoints->distPoints == NULL) {
            GEN_BREAK (SECFailure);
        }

        crlDistPoints->distPoints[count] = current;
        ++count;
        if (GetYesNo ("Enter more value for the CRL distribution "
                      "point extension [y/N]") == 0) {
            /* Add null to the end of the crlDistPoints to mark end of data */
            crlDistPoints->distPoints =
                PORT_ArenaGrow(arena, crlDistPoints->distPoints,
                               sizeof (*crlDistPoints->distPoints) * count,
                               sizeof (*crlDistPoints->distPoints) *(count + 1));
            crlDistPoints->distPoints[count] = NULL;    
            break;
        }


    } while (1);
    
    if (rv == SECSuccess) {
        PRBool yesNoAns = GetYesNo ("Is this a critical extension [y/N]?");

        rv = SECU_EncodeAndAddExtensionValue(arena, extHandle,
                                             crlDistPoints, yesNoAns, 
                                             SEC_OID_X509_CRL_DIST_POINTS,
                                             (EXTEN_EXT_VALUE_ENCODER)
                                             CERT_EncodeCRLDistributionPoints);
    }
    if (arena)
        PORT_FreeArena (arena, PR_FALSE);
    return (rv);
}

SECStatus
AddExtensions(void *extHandle, const char *emailAddrs, const char *dnsNames,
              PRBool  keyUsage, 
              PRBool  extKeyUsage,
              PRBool  basicConstraint, 
              PRBool  authKeyID,
              PRBool  crlDistPoints, 
              PRBool  nscpCertType)
{
    SECStatus rv = SECSuccess;
    do {
        /* Add key usage extension */
        if (keyUsage) {
            rv = AddKeyUsage(extHandle);
            if (rv)
                break;
        }

        /* Add extended key usage extension */
        if (extKeyUsage) {
            rv = AddExtKeyUsage(extHandle);
            if (rv)
                break;
        }

        /* Add basic constraint extension */
        if (basicConstraint) {
            rv = AddBasicConstraint(extHandle);
            if (rv)
                break;
        }

        if (authKeyID) {
            rv = AddAuthKeyID (extHandle);
            if (rv)
                break;
        }    

        if (crlDistPoints) {
            rv = AddCrlDistPoint (extHandle);
            if (rv)
                break;
        }

        if (nscpCertType) {
            rv = AddNscpCertType(extHandle);
            if (rv)
                break;
        }

        if (emailAddrs || dnsNames) {
            PRArenaPool *arena;
            CERTGeneralName *namelist = NULL;
            SECItem item = { 0, NULL, 0 };
            
            arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
            if (arena == NULL) {
                rv = SECFailure;
                break;
            }

            rv = AddEmailSubjectAlt(arena, &namelist, emailAddrs);

            rv |= AddDNSSubjectAlt(arena, &namelist, dnsNames);

            if (rv == SECSuccess) {
                rv = CERT_EncodeAltNameExtension(arena, namelist, &item);
                if (rv == SECSuccess) {
                    rv = CERT_AddExtension(extHandle,
                                           SEC_OID_X509_SUBJECT_ALT_NAME,
                                           &item, PR_FALSE, PR_TRUE);
                }
            }
            PORT_FreeArena(arena, PR_FALSE);
        }
    } while (0);
    return rv;
}
