
#include "cmdutil.h"

#define PKIUTIL_VERSION_STRING "pkiutil version 0.1"

extern char *progName;

typedef enum 
{
    PKIUnknown = -1,
    PKICertificate,
    PKIPublicKey,
    PKIPrivateKey,
    PKIAny
} PKIObjectType;

PRStatus
ImportObject
(
  NSSTrustDomain *td,
  NSSToken *tokenOpt,
  char *objectTypeOpt,
  char *nickname,
  char *keyTypeOpt,
  char *keypass,
  CMDRunTimeData *rtData
);

PRStatus
ExportObject
(
  NSSTrustDomain *td,
  NSSToken *tokenOpt,
  char *objectTypeOpt,
  char *nickname,
  char *keypass,
  CMDRunTimeData *rtData
);

PRStatus
GenerateKeyPair
(
  NSSTrustDomain *td,
  NSSToken *tokenOpt,
  char *keyTypeOpt,
  char *keySizeOpt,
  char *nickname,
  CMDRunTimeData *rtData
);

/* XXX need to be more specific (serial number?) */
PRStatus
DeleteObject
(
  NSSTrustDomain *td,
  NSSToken *tokenOpt,
  char *objectTypeOpt,
  char *nickname
);

PRStatus
ListObjects
(
  NSSTrustDomain *td,
  NSSToken *tokenOpt,
  char *objectTypeOpt,
  char *nicknameOpt,
  PRUint32 maximumOpt,
  CMDRunTimeData *rtData
);

PRStatus
ListChain
(
  NSSTrustDomain *td,
  char *nickname,
  char *serial,
  PRUint32 maximumOpt,
  CMDRunTimeData *rtData
);

PRStatus
DumpObject
(
  NSSTrustDomain *td,
  char *objectType,
  char *nickname,
  char *serialOpt,
  PRBool info,
  CMDRunTimeData *rtData
);

PRStatus
ValidateCert
(
  NSSTrustDomain *td,
  char *nickname,
  char *serial,
  char *usages,
  PRBool info,
  CMDRunTimeData *rtData
);

PRStatus
SetCertTrust
(
  NSSTrustDomain *td,
  char *nickname,
  char *serial,
  char *trustedUsages
);

PRStatus
DeleteOrphanedKeyPairs
(
  NSSTrustDomain *td,
  NSSToken *tokenOpt,
  CMDRunTimeData *rtData
);

