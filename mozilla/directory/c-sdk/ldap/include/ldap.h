/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */
/* ldap.h - general header file for libldap */
#ifndef _LDAP_H
#define _LDAP_H

#ifdef __cplusplus
extern "C" {
#endif

#if defined( XP_OS2 ) 
#include "os2sock.h"
#elif defined (WIN32) || defined (_WIN32) || defined( _CONSOLE ) 
#include <windows.h>
#  if defined( _WINDOWS )
#  include <winsock.h>
#  endif
#elif defined(macintosh)
#ifndef LDAP_TYPE_TIMEVAL_DEFINED
#include <utime.h>
#endif
#ifndef LDAP_TYPE_SOCKET_DEFINED	/* API extension */
#include "macsock.h"
#endif
#else /* everything else, e.g., Unix */
#ifndef LDAP_TYPE_TIMEVAL_DEFINED
#include <sys/time.h>
#endif
#ifndef LDAP_TYPE_SOCKET_DEFINED	/* API extension */
#include <sys/types.h>
#include <sys/socket.h>
#endif
#endif

#ifdef _AIX
#include <sys/select.h>
#endif /* _AIX */

#include "lber.h"

#define LDAP_PORT       	389
#define LDAPS_PORT      	636
#define LDAP_PORT_MAX		65535		/* API extension */
#define LDAP_VERSION1   	1		/* API extension */
#define LDAP_VERSION2   	2
#define LDAP_VERSION3   	3
#define LDAP_VERSION    	LDAP_VERSION2	/* API extension */
#define LDAP_VERSION_MIN	LDAP_VERSION1
#define LDAP_VERSION_MAX	LDAP_VERSION3

#define LDAP_VENDOR_VERSION	410	/* version # * 100 */
#define LDAP_VENDOR_NAME	"Netscape Communications Corp."
/*
 * The following will be an RFC number once the LDAP C API Internet Draft
 * is published as a Proposed Standard RFC.  For now we use 2000 + the
 * draft revision number (currently 4) since we are close to compliance
 * with revision 4 of the draft.
 */
#define LDAP_API_VERSION	2004

/*
 * C LDAP features we support that are not (yet) part of the LDAP C API
 * Internet Draft.  Use the ldap_get_option() call with an option value of
 * LDAP_OPT_API_FEATURE_INFO to retrieve information about a feature.
 *
 * Note that this list is incomplete; it includes only the most widely
 * used extensions.  Also, the version is 1 for all of these for now.
 */
#define LDAP_API_FEATURE_SERVER_SIDE_SORT	1
#define LDAP_API_FEATURE_VIRTUAL_LIST_VIEW	1
#define LDAP_API_FEATURE_PERSISTENT_SEARCH	1
#define LDAP_API_FEATURE_PROXY_AUTHORIZATION	1
#define LDAP_API_FEATURE_X_LDERRNO		1
#define LDAP_API_FEATURE_X_MEMCACHE		1
#define LDAP_API_FEATURE_X_IO_FUNCTIONS		1
#define LDAP_API_FEATURE_X_EXTIO_FUNCTIONS	1
#define LDAP_API_FEATURE_X_DNS_FUNCTIONS	1
#define LDAP_API_FEATURE_X_MEMALLOC_FUNCTIONS	1
#define LDAP_API_FEATURE_X_THREAD_FUNCTIONS	1
#define LDAP_API_FEATURE_X_EXTHREAD_FUNCTIONS	1
#define LDAP_API_FEATURE_X_GETLANGVALUES	1
#define LDAP_API_FEATURE_X_CLIENT_SIDE_SORT	1
#define LDAP_API_FEATURE_X_URL_FUNCTIONS	1
#define LDAP_API_FEATURE_X_FILTER_FUNCTIONS	1

#define LDAP_ROOT_DSE		""		/* API extension */
#define LDAP_NO_ATTRS		"1.1"
#define LDAP_ALL_USER_ATTRS	"*"

/*
 * Standard options (used with ldap_set_option() and ldap_get_option):
 */
#define LDAP_OPT_API_INFO               0x00	/*  0 */
#define LDAP_OPT_DESC                   0x01	/*  1 */
#define LDAP_OPT_DEREF                  0x02	/*  2 */
#define LDAP_OPT_SIZELIMIT              0x03	/*  3 */
#define LDAP_OPT_TIMELIMIT              0x04	/*  4 */
#define LDAP_OPT_REFERRALS              0x08	/*  8 */
#define LDAP_OPT_RESTART                0x09	/*  9 */
#define LDAP_OPT_PROTOCOL_VERSION	0x11	/* 17 */
#define LDAP_OPT_SERVER_CONTROLS	0x12	/* 18 */
#define LDAP_OPT_CLIENT_CONTROLS	0x13	/* 19 */
#define LDAP_OPT_API_FEATURE_INFO	0x15	/* 21 */
#define LDAP_OPT_HOST_NAME		0x30	/* 48 */
#define LDAP_OPT_ERROR_NUMBER		0x31	/* 49 */
#define LDAP_OPT_ERROR_STRING		0x32	/* 50 */
#define LDAP_OPT_MATCHED_DN		0x33	/* 51 */

/*
 * Well-behaved private and experimental extensions will use option values
 * between 0x4000 (16384) and 0x7FFF (32767) inclusive.
 */
#define LDAP_OPT_PRIVATE_EXTENSION_BASE	0x4000	/* to 0x7FFF inclusive */

/* for on/off options */
#define LDAP_OPT_ON     ((void *)1)
#define LDAP_OPT_OFF    ((void *)0)

typedef struct ldap     LDAP;           /* opaque connection handle */
typedef struct ldapmsg  LDAPMessage;    /* opaque result/entry handle */

#define NULLMSG ((LDAPMessage *)0)

/* structure representing an LDAP modification */
typedef struct ldapmod {
	int             mod_op;         /* kind of mod + form of values*/
#define LDAP_MOD_ADD            0x00
#define LDAP_MOD_DELETE         0x01
#define LDAP_MOD_REPLACE        0x02
#define LDAP_MOD_BVALUES        0x80
	char            *mod_type;      /* attribute name to modify */
	union mod_vals_u {
		char            **modv_strvals;
		struct berval   **modv_bvals;
	} mod_vals;                     /* values to add/delete/replace */
#define mod_values      mod_vals.modv_strvals
#define mod_bvalues     mod_vals.modv_bvals
} LDAPMod;


/*
 * structure for holding ldapv3 controls
 */
typedef struct ldapcontrol {
    char            *ldctl_oid;
    struct berval   ldctl_value;
    char            ldctl_iscritical;
} LDAPControl;


/*
 * LDAP API information.  Can be retrieved by using a sequence like:
 *
 *    LDAPAPIInfo ldai;
 *    ldai.ldapai_info_version = LDAP_API_INFO_VERSION;
 *    if ( ldap_get_option( NULL, LDAP_OPT_API_INFO, &ldia ) == 0 ) ...
 */
#define LDAP_API_INFO_VERSION		1
typedef struct ldapapiinfo {
    int  ldapai_info_version;     /* version of this struct (1) */
    int  ldapai_api_version;      /* revision of API supported */
    int  ldapai_protocol_version; /* highest LDAP version supported */
    char **ldapai_extensions;     /* names of API extensions */
    char *ldapai_vendor_name;     /* name of supplier */
    int  ldapai_vendor_version;   /* supplier-specific version times 100 */
} LDAPAPIInfo;


/*
 * LDAP API extended features info.  Can be retrieved by using a sequence like:
 *
 *    LDAPAPIFeatureInfo ldfi;
 *    ldfi.ldapaif_info_version = LDAP_FEATURE_INFO_VERSION;
 *    ldfi.ldapaif_name = "VIRTUAL_LIST_VIEW";
 *    if ( ldap_get_option( NULL, LDAP_OPT_API_FEATURE_INFO, &ldfi ) == 0 ) ...
 */
#define LDAP_FEATURE_INFO_VERSION	1
typedef struct ldap_apifeature_info {
    int   ldapaif_info_version;	/* version of this struct (1) */
    char  *ldapaif_name;	/* name of supported feature */
    int   ldapaif_version;	/* revision of supported feature */
} LDAPAPIFeatureInfo;


/* possible result types a server can return */
#define LDAP_RES_BIND                   0x61L	/* 97 */
#define LDAP_RES_SEARCH_ENTRY           0x64L	/* 100 */
#define LDAP_RES_SEARCH_RESULT          0x65L	/* 101 */
#define LDAP_RES_MODIFY                 0x67L	/* 103 */
#define LDAP_RES_ADD                    0x69L	/* 105 */
#define LDAP_RES_DELETE                 0x6bL	/* 107 */
#define LDAP_RES_MODDN			0x6dL	/* 109 */
#define LDAP_RES_COMPARE                0x6fL	/* 111 */
#define LDAP_RES_SEARCH_REFERENCE       0x73L	/* 115 */
#define LDAP_RES_EXTENDED               0x78L	/* 120 */

/* Special values for ldap_result() "msgid" parameter */
#define LDAP_RES_ANY                (-1)
#define LDAP_RES_UNSOLICITED		0

/* built-in SASL methods */
#define LDAP_SASL_SIMPLE	0	/* special value used for simple bind */
#define LDAP_SASL_EXTERNAL	"EXTERNAL"	/* TLS/SSL extension */

/* search scopes */
#define LDAP_SCOPE_BASE         0x00
#define LDAP_SCOPE_ONELEVEL     0x01
#define LDAP_SCOPE_SUBTREE      0x02

/* alias dereferencing */
#define LDAP_DEREF_NEVER        0
#define LDAP_DEREF_SEARCHING    1
#define LDAP_DEREF_FINDING      2
#define LDAP_DEREF_ALWAYS       3

/* predefined size/time limits */
#define LDAP_NO_LIMIT           0

/* allowed values for "all" ldap_result() parameter */
#define LDAP_MSG_ONE		0
#define LDAP_MSG_ALL		1
#define LDAP_MSG_RECEIVED	2

/* possible error codes we can be returned */
#define LDAP_SUCCESS                    0x00	/* 0 */
#define LDAP_OPERATIONS_ERROR           0x01	/* 1 */
#define LDAP_PROTOCOL_ERROR             0x02	/* 2 */
#define LDAP_TIMELIMIT_EXCEEDED         0x03	/* 3 */
#define LDAP_SIZELIMIT_EXCEEDED         0x04	/* 4 */
#define LDAP_COMPARE_FALSE              0x05	/* 5 */
#define LDAP_COMPARE_TRUE               0x06	/* 6 */
#define LDAP_STRONG_AUTH_NOT_SUPPORTED  0x07	/* 7 */
#define LDAP_STRONG_AUTH_REQUIRED       0x08	/* 8 */
#define LDAP_PARTIAL_RESULTS            0x09	/* 9 (UMich LDAPv2 extn) */
#define LDAP_REFERRAL                   0x0a	/* 10 - LDAPv3 */
#define LDAP_ADMINLIMIT_EXCEEDED	0x0b	/* 11 - LDAPv3 */
#define LDAP_UNAVAILABLE_CRITICAL_EXTENSION  0x0c /* 12 - LDAPv3 */
#define LDAP_CONFIDENTIALITY_REQUIRED	0x0d	/* 13 */
#define LDAP_SASL_BIND_IN_PROGRESS	0x0e	/* 14 - LDAPv3 */

#define LDAP_NO_SUCH_ATTRIBUTE          0x10	/* 16 */
#define LDAP_UNDEFINED_TYPE             0x11	/* 17 */
#define LDAP_INAPPROPRIATE_MATCHING     0x12	/* 18 */
#define LDAP_CONSTRAINT_VIOLATION       0x13	/* 19 */
#define LDAP_TYPE_OR_VALUE_EXISTS       0x14	/* 20 */
#define LDAP_INVALID_SYNTAX             0x15	/* 21 */

#define LDAP_NO_SUCH_OBJECT             0x20	/* 32 */
#define LDAP_ALIAS_PROBLEM              0x21	/* 33 */
#define LDAP_INVALID_DN_SYNTAX          0x22	/* 34 */
#define LDAP_IS_LEAF                    0x23	/* 35 (not used in LDAPv3) */
#define LDAP_ALIAS_DEREF_PROBLEM        0x24	/* 36 */

#define NAME_ERROR(n)   ((n & 0xf0) == 0x20)

#define LDAP_INAPPROPRIATE_AUTH         0x30	/* 48 */
#define LDAP_INVALID_CREDENTIALS        0x31	/* 49 */
#define LDAP_INSUFFICIENT_ACCESS        0x32	/* 50 */
#define LDAP_BUSY                       0x33	/* 51 */
#define LDAP_UNAVAILABLE                0x34	/* 52 */
#define LDAP_UNWILLING_TO_PERFORM       0x35	/* 53 */
#define LDAP_LOOP_DETECT                0x36	/* 54 */

#define LDAP_SORT_CONTROL_MISSING       0x3C	/* 60 (server side sort extn) */
#define LDAP_INDEX_RANGE_ERROR          0x3D    /* 61 (VLV extn) */

#define LDAP_NAMING_VIOLATION           0x40	/* 64 */
#define LDAP_OBJECT_CLASS_VIOLATION     0x41	/* 65 */
#define LDAP_NOT_ALLOWED_ON_NONLEAF     0x42	/* 66 */
#define LDAP_NOT_ALLOWED_ON_RDN         0x43	/* 67 */
#define LDAP_ALREADY_EXISTS             0x44	/* 68 */
#define LDAP_NO_OBJECT_CLASS_MODS       0x45	/* 69 */
#define LDAP_RESULTS_TOO_LARGE          0x46	/* 70 - CLDAP */
#define LDAP_AFFECTS_MULTIPLE_DSAS      0x47	/* 71 */

#define LDAP_OTHER                      0x50	/* 80 */
#define LDAP_SERVER_DOWN                0x51	/* 81 */
#define LDAP_LOCAL_ERROR                0x52	/* 82 */
#define LDAP_ENCODING_ERROR             0x53	/* 83 */
#define LDAP_DECODING_ERROR             0x54	/* 84 */
#define LDAP_TIMEOUT                    0x55	/* 85 */
#define LDAP_AUTH_UNKNOWN               0x56	/* 86 */
#define LDAP_FILTER_ERROR               0x57	/* 87 */
#define LDAP_USER_CANCELLED             0x58	/* 88 */
#define LDAP_PARAM_ERROR                0x59	/* 89 */
#define LDAP_NO_MEMORY                  0x5a	/* 90 */
#define LDAP_CONNECT_ERROR              0x5b	/* 91 */
#define LDAP_NOT_SUPPORTED              0x5c	/* 92 - LDAPv3 */
#define LDAP_CONTROL_NOT_FOUND		0x5d	/* 93 - LDAPv3 */
#define LDAP_NO_RESULTS_RETURNED	0x5e	/* 94 - LDAPv3 */
#define LDAP_MORE_RESULTS_TO_RETURN	0x5f	/* 95 - LDAPv3 */
#define LDAP_CLIENT_LOOP		0x60	/* 96 - LDAPv3 */
#define LDAP_REFERRAL_LIMIT_EXCEEDED	0x61	/* 97 - LDAPv3 */

/*
 * LDAPv3 unsolicited notification messages we know about
 */
#define LDAP_NOTICE_OF_DISCONNECTION	"1.3.6.1.4.1.1466.20036"

/*
 * LDAPv3 server controls we know about
 */
#define LDAP_CONTROL_MANAGEDSAIT	"2.16.840.1.113730.3.4.2"
#define LDAP_CONTROL_SORTREQUEST	"1.2.840.113556.1.4.473"
#define LDAP_CONTROL_SORTRESPONSE	"1.2.840.113556.1.4.474"
#define LDAP_CONTROL_PERSISTENTSEARCH	"2.16.840.1.113730.3.4.3"
#define LDAP_CONTROL_ENTRYCHANGE	"2.16.840.1.113730.3.4.7"
#define LDAP_CONTROL_VLVREQUEST    	"2.16.840.1.113730.3.4.9"
#define LDAP_CONTROL_VLVRESPONSE	"2.16.840.1.113730.3.4.10"
#define LDAP_CONTROL_PROXYAUTH		"2.16.840.1.113730.3.4.12"

/* Authentication request and response controls */
#define LDAP_CONTROL_AUTH_REQUEST	"2.16.840.1.113730.3.4.16"
#define LDAP_CONTROL_AUTH_RESPONSE	"2.16.840.1.113730.3.4.15"

/* Password information sent back to client */
#define LDAP_CONTROL_PWEXPIRED		"2.16.840.1.113730.3.4.4"
#define LDAP_CONTROL_PWEXPIRING		"2.16.840.1.113730.3.4.5"


/*
 * Client controls we know about
 */
#define LDAP_CONTROL_REFERRALS		"1.2.840.113556.1.4.616"


/*
 * LDAP_API macro definition:
 */
#ifndef LDAP_API
#if defined( _WINDOWS ) || defined( _WIN32 )
#define LDAP_API(rt) rt
#else /* _WINDOWS */
#define LDAP_API(rt) rt
#endif /* _WINDOWS */
#endif /* LDAP_API */


LDAP_API(LDAP *) LDAP_CALL ldap_open( const char *host, int port );
LDAP_API(LDAP *) LDAP_CALL ldap_init( const char *defhost, int defport );
LDAP_API(int) LDAP_CALL ldap_set_option( LDAP *ld, int option,
	const void *optdata );
LDAP_API(int) LDAP_CALL ldap_get_option( LDAP *ld, int option, void *optdata );
LDAP_API(int) LDAP_CALL ldap_unbind( LDAP *ld );
LDAP_API(int) LDAP_CALL ldap_unbind_s( LDAP *ld );

/*
 * perform ldap operations and obtain results
 */
LDAP_API(int) LDAP_CALL ldap_abandon( LDAP *ld, int msgid );
LDAP_API(int) LDAP_CALL ldap_add( LDAP *ld, const char *dn, LDAPMod **attrs );
LDAP_API(int) LDAP_CALL ldap_add_s( LDAP *ld, const char *dn, LDAPMod **attrs );
LDAP_API(int) LDAP_CALL ldap_simple_bind( LDAP *ld, const char *who,
	const char *passwd );
LDAP_API(int) LDAP_CALL ldap_simple_bind_s( LDAP *ld, const char *who,
	const char *passwd );
LDAP_API(int) LDAP_CALL ldap_modify( LDAP *ld, const char *dn, LDAPMod **mods );
LDAP_API(int) LDAP_CALL ldap_modify_s( LDAP *ld, const char *dn, 
	LDAPMod **mods );
LDAP_API(int) LDAP_CALL ldap_modrdn( LDAP *ld, const char *dn, 
	const char *newrdn );
LDAP_API(int) LDAP_CALL ldap_modrdn_s( LDAP *ld, const char *dn, 
	const char *newrdn );
LDAP_API(int) LDAP_CALL ldap_modrdn2( LDAP *ld, const char *dn, 
	const char *newrdn, int deleteoldrdn );
LDAP_API(int) LDAP_CALL ldap_modrdn2_s( LDAP *ld, const char *dn, 
	const char *newrdn, int deleteoldrdn);
LDAP_API(int) LDAP_CALL ldap_compare( LDAP *ld, const char *dn,
	const char *attr, const char *value );
LDAP_API(int) LDAP_CALL ldap_compare_s( LDAP *ld, const char *dn, 
	const char *attr, const char *value );
LDAP_API(int) LDAP_CALL ldap_delete( LDAP *ld, const char *dn );
LDAP_API(int) LDAP_CALL ldap_delete_s( LDAP *ld, const char *dn );
LDAP_API(int) LDAP_CALL ldap_search( LDAP *ld, const char *base, int scope,
	const char *filter, char **attrs, int attrsonly );
LDAP_API(int) LDAP_CALL ldap_search_s( LDAP *ld, const char *base, int scope,
	const char *filter, char **attrs, int attrsonly, LDAPMessage **res );
LDAP_API(int) LDAP_CALL ldap_search_st( LDAP *ld, const char *base, int scope,
	const char *filter, char **attrs, int attrsonly,
	struct timeval *timeout, LDAPMessage **res );
LDAP_API(int) LDAP_CALL ldap_result( LDAP *ld, int msgid, int all,
	struct timeval *timeout, LDAPMessage **result );
LDAP_API(int) LDAP_CALL ldap_msgfree( LDAPMessage *lm );
LDAP_API(int) LDAP_CALL ldap_msgid( LDAPMessage *lm );
LDAP_API(int) LDAP_CALL ldap_msgtype( LDAPMessage *lm );


/*
 * Routines to parse/deal with results and errors returned
 */
LDAP_API(int) LDAP_CALL ldap_result2error( LDAP *ld, LDAPMessage *r, 
	int freeit );
LDAP_API(char *) LDAP_CALL ldap_err2string( int err );
LDAP_API(void) LDAP_CALL ldap_perror( LDAP *ld, const char *s );
LDAP_API(LDAPMessage *) LDAP_CALL ldap_first_entry( LDAP *ld, 
	LDAPMessage *chain );
LDAP_API(LDAPMessage *) LDAP_CALL ldap_next_entry( LDAP *ld, 
	LDAPMessage *entry );
LDAP_API(int) LDAP_CALL ldap_count_entries( LDAP *ld, LDAPMessage *chain );
LDAP_API(char *) LDAP_CALL ldap_get_dn( LDAP *ld, LDAPMessage *entry );
LDAP_API(char *) LDAP_CALL ldap_dn2ufn( const char *dn );
LDAP_API(char **) LDAP_CALL ldap_explode_dn( const char *dn, 
	const int notypes );
LDAP_API(char **) LDAP_CALL ldap_explode_rdn( const char *rdn, 
	const int notypes );
LDAP_API(char *) LDAP_CALL ldap_first_attribute( LDAP *ld, LDAPMessage *entry,
	BerElement **ber );
LDAP_API(char *) LDAP_CALL ldap_next_attribute( LDAP *ld, LDAPMessage *entry,
	BerElement *ber );
LDAP_API(void) LDAP_CALL ldap_ber_free( BerElement *ber, int freebuf );
LDAP_API(char **) LDAP_CALL ldap_get_values( LDAP *ld, LDAPMessage *entry,
	const char *target );
LDAP_API(struct berval **) LDAP_CALL ldap_get_values_len( LDAP *ld,
	LDAPMessage *entry, const char *target );
LDAP_API(int) LDAP_CALL ldap_count_values( char **vals );
LDAP_API(int) LDAP_CALL ldap_count_values_len( struct berval **vals );
LDAP_API(void) LDAP_CALL ldap_value_free( char **vals );
LDAP_API(void) LDAP_CALL ldap_value_free_len( struct berval **vals );
LDAP_API(void) LDAP_CALL ldap_memfree( void *p );


/*
 * LDAPv3 extended operation calls
 */
/*
 * Note: all of the new asynchronous calls return an LDAP error code,
 * not a message id.  A message id is returned via the int *msgidp
 * parameter (usually the last parameter) if appropriate.
 */
LDAP_API(int) LDAP_CALL ldap_abandon_ext( LDAP *ld, int msgid,
	LDAPControl **serverctrls, LDAPControl **clientctrls );
LDAP_API(int) LDAP_CALL ldap_add_ext( LDAP *ld, const char *dn, LDAPMod **attrs,
	LDAPControl **serverctrls, LDAPControl **clientctrls, int *msgidp );
LDAP_API(int) LDAP_CALL ldap_add_ext_s( LDAP *ld, const char *dn,
	LDAPMod **attrs, LDAPControl **serverctrls, LDAPControl **clientctrls );
LDAP_API(int) LDAP_CALL ldap_sasl_bind( LDAP *ld, const char *dn,
	const char *mechanism, const struct berval *cred,
	LDAPControl **serverctrls, LDAPControl **clientctrls, int *msgidp );
LDAP_API(int) LDAP_CALL ldap_sasl_bind_s( LDAP *ld, const char *dn,
	const char *mechanism, const struct berval *cred,
	LDAPControl **serverctrls, LDAPControl **clientctrls,
	struct berval **servercredp );
LDAP_API(int) LDAP_CALL ldap_modify_ext( LDAP *ld, const char *dn,
	LDAPMod **mods, LDAPControl **serverctrls, LDAPControl **clientctrls,
	int *msgidp );
LDAP_API(int) LDAP_CALL ldap_modify_ext_s( LDAP *ld, const char *dn,
	LDAPMod **mods, LDAPControl **serverctrls, LDAPControl **clientctrls );
LDAP_API(int) LDAP_CALL ldap_rename( LDAP *ld, const char *dn,
	const char *newrdn, const char *newparent, int deleteoldrdn,
	LDAPControl **serverctrls, LDAPControl **clientctrls, int *msgidp );
LDAP_API(int) LDAP_CALL ldap_rename_s( LDAP *ld, const char *dn,
	const char *newrdn, const char *newparent, int deleteoldrdn,
	LDAPControl **serverctrls, LDAPControl **clientctrls );
LDAP_API(int) LDAP_CALL ldap_compare_ext( LDAP *ld, const char *dn,
	const char *attr, const struct berval *bvalue,
	LDAPControl **serverctrls, LDAPControl **clientctrls, int *msgidp );
LDAP_API(int) LDAP_CALL ldap_compare_ext_s( LDAP *ld, const char *dn,
	const char *attr, const struct berval *bvalue,
	LDAPControl **serverctrls, LDAPControl **clientctrls );
LDAP_API(int) LDAP_CALL ldap_delete_ext( LDAP *ld, const char *dn,
	LDAPControl **serverctrls, LDAPControl **clientctrls, int *msgidp );
LDAP_API(int) LDAP_CALL ldap_delete_ext_s( LDAP *ld, const char *dn,
	LDAPControl **serverctrls, LDAPControl **clientctrls );
LDAP_API(int) LDAP_CALL ldap_search_ext( LDAP *ld, const char *base,
	int scope, const char *filter, char **attrs, int attrsonly,
	LDAPControl **serverctrls, LDAPControl **clientctrls,
	struct timeval *timeoutp, int sizelimit, int *msgidp );
LDAP_API(int) LDAP_CALL ldap_search_ext_s( LDAP *ld, const char *base,
	int scope, const char *filter, char **attrs, int attrsonly,
	LDAPControl **serverctrls, LDAPControl **clientctrls,
	struct timeval *timeoutp, int sizelimit, LDAPMessage **res );
LDAP_API(int) LDAP_CALL ldap_extended_operation( LDAP *ld,
	const char *requestoid, const struct berval *requestdata,
	LDAPControl **serverctrls, LDAPControl **clientctrls, int *msgidp );
LDAP_API(int) LDAP_CALL ldap_extended_operation_s( LDAP *ld,
	const char *requestoid, const struct berval *requestdata,
	LDAPControl **serverctrls, LDAPControl **clientctrls,
	char **retoidp, struct berval **retdatap );
LDAP_API(int) LDAP_CALL ldap_unbind_ext( LDAP *ld, LDAPControl **serverctrls,
	LDAPControl **clientctrls );


/*
 * LDAPv3 extended parsing / result handling calls
 */
LDAP_API(int) LDAP_CALL ldap_parse_sasl_bind_result( LDAP *ld,
	LDAPMessage *res, struct berval **servercredp, int freeit );
LDAP_API(int) LDAP_CALL ldap_parse_result( LDAP *ld, LDAPMessage *res,
	int *errcodep, char **matcheddnp, char **errmsgp, char ***referralsp,
	LDAPControl ***serverctrlsp, int freeit );
LDAP_API(int) LDAP_CALL ldap_parse_extended_result( LDAP *ld, LDAPMessage *res,
	char **retoidp, struct berval **retdatap, int freeit );
LDAP_API(LDAPMessage *) LDAP_CALL ldap_first_message( LDAP *ld,
	LDAPMessage *res );
LDAP_API(LDAPMessage *) LDAP_CALL ldap_next_message( LDAP *ld,	
	LDAPMessage *msg );
LDAP_API(int) LDAP_CALL ldap_count_messages( LDAP *ld, LDAPMessage *res );
LDAP_API(LDAPMessage *) LDAP_CALL ldap_first_reference( LDAP *ld,
	LDAPMessage *res );
LDAP_API(LDAPMessage *) LDAP_CALL ldap_next_reference( LDAP *ld,
	LDAPMessage *ref );
LDAP_API(int) LDAP_CALL ldap_count_references( LDAP *ld, LDAPMessage *res );
LDAP_API(int) LDAP_CALL ldap_parse_reference( LDAP *ld, LDAPMessage *ref,
	char ***referralsp, LDAPControl ***serverctrlsp, int freeit );
LDAP_API(int) LDAP_CALL ldap_get_entry_controls( LDAP *ld, LDAPMessage *entry,
	LDAPControl ***serverctrlsp );
LDAP_API(void) LDAP_CALL ldap_control_free( LDAPControl *ctrl );
LDAP_API(void) LDAP_CALL ldap_controls_free( LDAPControl **ctrls );

/*************** End of core standard C LDAP API definitions *****************/


/*
 * Server side sorting of search results (an LDAPv3 extension --
 * LDAP_API_FEATURE_SERVER_SIDE_SORT)
 */
typedef struct LDAPsortkey {	/* structure for a sort-key */
	char *  sk_attrtype;
	char *  sk_matchruleoid;
	int     sk_reverseorder;
} LDAPsortkey;

LDAP_API(int) LDAP_CALL ldap_create_sort_control( LDAP *ld,
	LDAPsortkey **sortKeyList, const char ctl_iscritical,
	LDAPControl **ctrlp );
LDAP_API(int) LDAP_CALL ldap_parse_sort_control( LDAP *ld,
	LDAPControl **ctrls, unsigned long *result, char **attribute );

LDAP_API(void) LDAP_CALL ldap_free_sort_keylist( LDAPsortkey **sortKeyList );
LDAP_API(int) LDAP_CALL ldap_create_sort_keylist( LDAPsortkey ***sortKeyList,
	const char *string_rep );


/*
 * Virtual list view (an LDAPv3 extension -- LDAP_API_FEATURE_VIRTUAL_LIST_VIEW)
 */
/*
 * structure that describes a VirtualListViewRequest control.
 * note that ldvlist_index and ldvlist_size are only relevant to
 * ldap_create_virtuallist_control() if ldvlist_attrvalue is NULL.
 */
typedef struct ldapvirtuallist {
    unsigned long   ldvlist_before_count;       /* # entries before target */
    unsigned long   ldvlist_after_count;        /* # entries after target */
    char            *ldvlist_attrvalue;         /* jump to this value */
    unsigned long   ldvlist_index;              /* list offset */
    unsigned long   ldvlist_size;               /* number of items in vlist */
    void            *ldvlist_extradata;         /* for use by application */
} LDAPVirtualList;

/*
 * VLV functions:
 */
LDAP_API(int) LDAP_CALL ldap_create_virtuallist_control( LDAP *ld, 
        LDAPVirtualList *ldvlistp, LDAPControl **ctrlp );

LDAP_API(int) LDAP_CALL ldap_parse_virtuallist_control( LDAP *ld,
        LDAPControl **ctrls, unsigned long *target_posp, 
	unsigned long *list_sizep, int *errcodep );


/*
 * Routines for creating persistent search controls and for handling
 * "entry changed notification" controls (an LDAPv3 extension --
 * LDAP_API_FEATURE_PERSISTENT_SEARCH)
 */
#define LDAP_CHANGETYPE_ADD		1
#define LDAP_CHANGETYPE_DELETE		2
#define LDAP_CHANGETYPE_MODIFY		4
#define LDAP_CHANGETYPE_MODDN		8
#define LDAP_CHANGETYPE_ANY		(1|2|4|8)
LDAP_API(int) LDAP_CALL ldap_create_persistentsearch_control( LDAP *ld, 
	int changetypes, int changesonly, int return_echg_ctls,
	char ctl_iscritical, LDAPControl **ctrlp );
LDAP_API(int) LDAP_CALL ldap_parse_entrychange_control( LDAP *ld,
	LDAPControl **ctrls, int *chgtypep, char **prevdnp,
	int *chgnumpresentp, long *chgnump );


/*
 * Routine for creating the Proxied Authorization control (an LDAPv3
 * extension -- LDAP_API_FEATURE_PROXY_AUTHORIZATION)
 */
LDAP_API(int) LDAP_CALL ldap_create_proxyauth_control( LDAP *ld,
	const char *dn, const char ctl_iscritical, LDAPControl **ctrlp );


/*
 * Functions to get and set LDAP error information (API extension --
 * LDAP_API_FEATURE_X_LDERRNO )
 *
 * By using LDAP_OPT_THREAD_FN_PTRS, you can arrange for the error info. to
 * be thread-specific.
 */
LDAP_API(int) LDAP_CALL ldap_get_lderrno( LDAP *ld, char **m, char **s );
LDAP_API(int) LDAP_CALL ldap_set_lderrno( LDAP *ld, int e, char *m, char *s );


/*
 * LDAP URL functions and definitions (an API extension --
 * LDAP_API_FEATURE_X_URL_FUNCTIONS)
 */
/*
 * types for ldap URL handling
 */
typedef struct ldap_url_desc {
    char                *lud_host;
    int                 lud_port;
    char                *lud_dn;
    char                **lud_attrs;
    int                 lud_scope;
    char                *lud_filter;
    unsigned long       lud_options;
#define LDAP_URL_OPT_SECURE     0x01
    char        *lud_string;    /* for internal use only */
} LDAPURLDesc;

#define NULLLDAPURLDESC ((LDAPURLDesc *)NULL)

/*
 * possible errors returned by ldap_url_parse()
 */
#define LDAP_URL_ERR_NOTLDAP    1       /* URL doesn't begin with "ldap://" */
#define LDAP_URL_ERR_NODN       2       /* URL has no DN (required) */
#define LDAP_URL_ERR_BADSCOPE   3       /* URL scope string is invalid */
#define LDAP_URL_ERR_MEM        4       /* can't allocate memory space */
#define LDAP_URL_ERR_PARAM	5	/* bad parameter to an URL function */

/*
 * URL functions:
 */
LDAP_API(int) LDAP_CALL ldap_is_ldap_url( const char *url );
LDAP_API(int) LDAP_CALL ldap_url_parse( const char *url, LDAPURLDesc **ludpp );
LDAP_API(void) LDAP_CALL ldap_free_urldesc( LDAPURLDesc *ludp );
LDAP_API(int) LDAP_CALL ldap_url_search( LDAP *ld, const char *url,
	int attrsonly );
LDAP_API(int) LDAP_CALL ldap_url_search_s( LDAP *ld, const char *url,
	int attrsonly, LDAPMessage **res );
LDAP_API(int) LDAP_CALL ldap_url_search_st( LDAP *ld, const char *url,
	int attrsonly, struct timeval *timeout, LDAPMessage **res );

/*
 * Function to dispose of an array of LDAPMod structures (an API extension).
 * Warning: don't use this unless the mods array was allocated using the
 * same memory allocator as is being used by libldap.
 */
LDAP_API(void) LDAP_CALL ldap_mods_free( LDAPMod **mods, int freemods );

/*
 * SSL option (an API extension):
 */
#define LDAP_OPT_SSL			0x0A	/* 10 - API extension */

/*
 * Referral hop limit (an API extension):
 */
#define LDAP_OPT_REFERRAL_HOP_LIMIT	0x10	/* 16 - API extension */


/*
 * Preferred language and get_lang_values (an API extension --
 * LDAP_API_FEATURE_X_GETLANGVALUES)
 */
#define LDAP_OPT_PREFERRED_LANGUAGE	0x14	/* 20 - API extension */
LDAP_API(char **) LDAP_CALL ldap_get_lang_values( LDAP *ld, LDAPMessage *entry,
	const char *target, char **type );
LDAP_API(struct berval **) LDAP_CALL ldap_get_lang_values_len( LDAP *ld,
	LDAPMessage *entry, const char *target, char **type );


/*
 * Rebind callback function (an API extension)
 */
#define LDAP_OPT_REBIND_FN              0x06	/* 6 - API extension */
#define LDAP_OPT_REBIND_ARG             0x07	/* 7 - API extension */
typedef int (LDAP_CALL LDAP_CALLBACK LDAP_REBINDPROC_CALLBACK)( LDAP *ld, 
	char **dnp, char **passwdp, int *authmethodp, int freeit, void *arg);
LDAP_API(void) LDAP_CALL ldap_set_rebind_proc( LDAP *ld, 
	LDAP_REBINDPROC_CALLBACK *rebindproc, void *arg );


/*
 * Thread function callbacks (an API extension --
 * LDAP_API_FEATURE_X_THREAD_FUNCTIONS).
 */
#define LDAP_OPT_THREAD_FN_PTRS         0x05	/* 5 - API extension */

/*
 * Thread callback functions:
 */
typedef void *(LDAP_C LDAP_CALLBACK LDAP_TF_MUTEX_ALLOC_CALLBACK)( void );
typedef void (LDAP_C LDAP_CALLBACK LDAP_TF_MUTEX_FREE_CALLBACK)( void *m );
typedef int (LDAP_C LDAP_CALLBACK LDAP_TF_MUTEX_LOCK_CALLBACK)( void *m );
typedef int (LDAP_C LDAP_CALLBACK LDAP_TF_MUTEX_UNLOCK_CALLBACK)( void *m );
typedef int (LDAP_C LDAP_CALLBACK LDAP_TF_GET_ERRNO_CALLBACK)( void );
typedef void (LDAP_C LDAP_CALLBACK LDAP_TF_SET_ERRNO_CALLBACK)( int e );
typedef int (LDAP_C LDAP_CALLBACK LDAP_TF_GET_LDERRNO_CALLBACK)(
	char **matchedp, char **errmsgp, void *arg );
typedef void    (LDAP_C LDAP_CALLBACK LDAP_TF_SET_LDERRNO_CALLBACK)( int err, 
	char *matched, char *errmsg, void *arg );

/*
 * Structure to hold thread function pointers:
 */
struct ldap_thread_fns {
	LDAP_TF_MUTEX_ALLOC_CALLBACK *ltf_mutex_alloc;
	LDAP_TF_MUTEX_FREE_CALLBACK *ltf_mutex_free;
	LDAP_TF_MUTEX_LOCK_CALLBACK *ltf_mutex_lock;
	LDAP_TF_MUTEX_UNLOCK_CALLBACK *ltf_mutex_unlock;
	LDAP_TF_GET_ERRNO_CALLBACK *ltf_get_errno;
	LDAP_TF_SET_ERRNO_CALLBACK *ltf_set_errno;
	LDAP_TF_GET_LDERRNO_CALLBACK *ltf_get_lderrno;
	LDAP_TF_SET_LDERRNO_CALLBACK *ltf_set_lderrno;
	void    *ltf_lderrno_arg;
};


/*
 * Extended I/O function callbacks option (an API extension --
 * LDAP_API_FEATURE_X_EXTIO_FUNCTIONS).
 */
#define LDAP_X_OPT_EXTIO_FN_PTRS   (LDAP_OPT_PRIVATE_EXTENSION_BASE + 0x0F00)		/* 0x4000 + 0x0F00 = 0x4F00 = 20224 - API extension */

/*
 * These extended I/O function callbacks echo the BSD socket API but accept
 * an extra pointer parameter at the end of their argument list that can
 * be used by client applications for their own needs.  For some of the calls,
 * the pointer is a session argument of type struct lextiof_session_private *
 * that is associated with the LDAP session handle (LDAP *).  For others, the
 * pointer is a socket specific struct lextiof_socket_private * argument that
 * is associated with a particular socket (a TCP connection).
 *
 * The lextiof_session_private and lextiof_socket_private structures are not
 * defined by the LDAP C API; users of this extended I/O interface should
 * define these themselves.
 *
 * The combination of the integer socket number (i.e., lpoll_fd, which is
 * the value returned by the CONNECT callback) and the application specific
 * socket argument (i.e., lpoll_socketarg, which is the value set in *sockargpp
 * by the CONNECT callback) must be unique.
 *
 * The types for the extended READ and WRITE callbacks are actually in lber.h.
 *
 * The CONNECT callback gets passed both the session argument (sessionarg)
 * and a pointer to a socket argument (socketargp) so it has the
 * opportunity to set the socket-specific argument.  The CONNECT callback
 * also takes a timeout parameter whose value can be set by calling
 * ldap_set_option( ld, LDAP_X_OPT_..., &val ).  The units used for the
 * timeout parameter are milliseconds.
 *
 * A POLL interface is provided instead of a select() one.  The timeout is
 * in milliseconds.

 * A NEWHANDLE callback function is also provided.  It is called right
 * after the LDAP session handle is created, e.g., during ldap_init().
 * If the NEWHANDLE callback returns anything other than LDAP_SUCCESS,
 * the session handle allocation fails.
 *
 * A DISPOSEHANDLE callback function is also provided.  It is called right
 * before the LDAP session handle and its contents are destroyed, e.g.,
 * during ldap_unbind().
 */

/*
 * Special timeout values for poll and connect:
 */
#define LDAP_X_IO_TIMEOUT_NO_WAIT	0	/* return immediately */
#define LDAP_X_IO_TIMEOUT_NO_TIMEOUT	(-1)	/* block indefinitely */

/* LDAP poll()-like descriptor:
 */
typedef struct ldap_x_pollfd {	   /* used by LDAP_X_EXTIOF_POLL_CALLBACK */
    int		lpoll_fd;	   /* integer file descriptor / socket */
    struct lextiof_socket_private
		*lpoll_socketarg;
				   /* pointer socket and for use by */
				   /* application */
    short	lpoll_events;      /* requested event */
    short	lpoll_revents;     /* returned event */
} LDAP_X_PollFD;

/* Event flags for lpoll_events and lpoll_revents:
 */
#define LDAP_X_POLLIN    0x01  /* regular data ready for reading */
#define LDAP_X_POLLPRI   0x02  /* high priority data available */
#define LDAP_X_POLLOUT   0x04  /* ready for writing */
#define LDAP_X_POLLERR   0x08  /* error occurred -- only in lpoll_revents */
#define LDAP_X_POLLHUP   0x10  /* connection closed -- only in lpoll_revents */
#define LDAP_X_POLLNVAL  0x20  /* invalid lpoll_fd -- only in lpoll_revents */

/* Options passed to LDAP_X_EXTIOF_CONNECT_CALLBACK to modify socket behavior:
 */
#define LDAP_X_EXTIOF_OPT_NONBLOCKING	0x01  /* turn on non-blocking mode */
#define LDAP_X_EXTIOF_OPT_SECURE	0x02  /* turn on 'secure' mode */


/* extended I/O callback function prototypes:
 */
typedef int	(LDAP_C LDAP_CALLBACK LDAP_X_EXTIOF_CONNECT_CALLBACK )(
	    const char *hostlist, int port, /* host byte order */
	    int timeout /* milliseconds */,
	    unsigned long options, /* bitmapped options */
	    struct lextiof_session_private *sessionarg,
	    struct lextiof_socket_private **socketargp );
typedef int	(LDAP_C LDAP_CALLBACK LDAP_X_EXTIOF_CLOSE_CALLBACK )(
	    int s, struct lextiof_socket_private *socketarg );
typedef int	(LDAP_C LDAP_CALLBACK LDAP_X_EXTIOF_POLL_CALLBACK)(
	    LDAP_X_PollFD fds[], int nfds, int timeout /* milliseconds */,
	    struct lextiof_session_private *sessionarg );
typedef int	(LDAP_C LDAP_CALLBACK LDAP_X_EXTIOF_NEWHANDLE_CALLBACK)(
	    LDAP *ld, struct lextiof_session_private *sessionarg );
typedef void	(LDAP_C LDAP_CALLBACK LDAP_X_EXTIOF_DISPOSEHANDLE_CALLBACK)(
	    LDAP *ld, struct lextiof_session_private *sessionarg );


/* Structure to hold extended I/O function pointers:
 */
struct ldap_x_ext_io_fns {
	/* lextiof_size should always be set to LDAP_X_EXTIO_FNS_SIZE */
	int					lextiof_size;
	LDAP_X_EXTIOF_CONNECT_CALLBACK		*lextiof_connect;
	LDAP_X_EXTIOF_CLOSE_CALLBACK		*lextiof_close;
	LDAP_X_EXTIOF_READ_CALLBACK		*lextiof_read;
	LDAP_X_EXTIOF_WRITE_CALLBACK		*lextiof_write;
	LDAP_X_EXTIOF_POLL_CALLBACK		*lextiof_poll;
	LDAP_X_EXTIOF_NEWHANDLE_CALLBACK	*lextiof_newhandle;
	LDAP_X_EXTIOF_DISPOSEHANDLE_CALLBACK	*lextiof_disposehandle;
	void					*lextiof_session_arg;
};
#define LDAP_X_EXTIO_FNS_SIZE	sizeof(struct ldap_x_ext_io_fns)


/*
 * Utility functions for parsing space-separated host lists (useful for
 * implementing an extended I/O CONNECT callback function).
 */
struct ldap_x_hostlist_status;
LDAP_API(int) LDAP_CALL ldap_x_hostlist_first( const char *hostlist,
	int defport, char **hostp, int *portp /* host byte order */,
	struct ldap_x_hostlist_status **statusp );
LDAP_API(int) LDAP_CALL ldap_x_hostlist_next( char **hostp,
	int *portp /* host byte order */, struct ldap_x_hostlist_status *status );
LDAP_API(void) LDAP_CALL ldap_x_hostlist_statusfree(
	struct ldap_x_hostlist_status *status );


/*
 * I/O function callbacks option (an API extension --
 * LDAP_API_FEATURE_X_IO_FUNCTIONS).
 * Use of the extended I/O functions instead is recommended; see above.
 */
#define LDAP_OPT_IO_FN_PTRS		0x0B	/* 11 - API extension */

/*
 * I/O callback functions (note that types for the read and write callbacks
 * are actually in lber.h):
 */
typedef int	(LDAP_C LDAP_CALLBACK LDAP_IOF_SELECT_CALLBACK)( int nfds,
	fd_set *readfds, fd_set *writefds, fd_set *errorfds,
	struct timeval *timeout );
typedef LBER_SOCKET (LDAP_C LDAP_CALLBACK LDAP_IOF_SOCKET_CALLBACK)(
	int domain, int type, int protocol );
typedef int	(LDAP_C LDAP_CALLBACK LDAP_IOF_IOCTL_CALLBACK)( LBER_SOCKET s, 
	int option, ... );
typedef int	(LDAP_C LDAP_CALLBACK LDAP_IOF_CONNECT_CALLBACK )(
	LBER_SOCKET s, struct sockaddr *name, int namelen );
typedef int	(LDAP_C LDAP_CALLBACK LDAP_IOF_CLOSE_CALLBACK )(
	LBER_SOCKET s );
typedef int	(LDAP_C LDAP_CALLBACK LDAP_IOF_SSL_ENABLE_CALLBACK )(
	LBER_SOCKET s );

/*
 * Structure to hold I/O function pointers:
 */
struct ldap_io_fns {
	LDAP_IOF_READ_CALLBACK *liof_read;
	LDAP_IOF_WRITE_CALLBACK *liof_write;
	LDAP_IOF_SELECT_CALLBACK *liof_select;
	LDAP_IOF_SOCKET_CALLBACK *liof_socket;
	LDAP_IOF_IOCTL_CALLBACK *liof_ioctl;
	LDAP_IOF_CONNECT_CALLBACK *liof_connect;
	LDAP_IOF_CLOSE_CALLBACK *liof_close;
	LDAP_IOF_SSL_ENABLE_CALLBACK *liof_ssl_enable;
};


/*
 * Client side sorting of entries (an API extension --
 * LDAP_API_FEATURE_X_CLIENT_SIDE_SORT)
 */
/*
 * Client side sorting callback functions:
 */
typedef const struct berval* (LDAP_C LDAP_CALLBACK
	LDAP_KEYGEN_CALLBACK)( void *arg, LDAP *ld, LDAPMessage *entry );
typedef int (LDAP_C LDAP_CALLBACK
	LDAP_KEYCMP_CALLBACK)( void *arg, const struct berval*,
	const struct berval* );
typedef void (LDAP_C LDAP_CALLBACK
	LDAP_KEYFREE_CALLBACK)( void *arg, const struct berval* );
typedef int (LDAP_C LDAP_CALLBACK
	LDAP_CMP_CALLBACK)(const char *val1, const char *val2);
typedef int (LDAP_C LDAP_CALLBACK
	LDAP_VALCMP_CALLBACK)(const char **val1p, const char **val2p);

/*
 * Client side sorting functions:
 */
LDAP_API(int) LDAP_CALL ldap_keysort_entries( LDAP *ld, LDAPMessage **chain,
	void *arg, LDAP_KEYGEN_CALLBACK *gen, LDAP_KEYCMP_CALLBACK *cmp,
	LDAP_KEYFREE_CALLBACK *fre );
LDAP_API(int) LDAP_CALL ldap_multisort_entries( LDAP *ld, LDAPMessage **chain,
	char **attr, LDAP_CMP_CALLBACK *cmp );
LDAP_API(int) LDAP_CALL ldap_sort_entries( LDAP *ld, LDAPMessage **chain, 
	char *attr, LDAP_CMP_CALLBACK *cmp );
LDAP_API(int) LDAP_CALL ldap_sort_values( LDAP *ld, char **vals, 
	LDAP_VALCMP_CALLBACK *cmp );
LDAP_API(int) LDAP_C LDAP_CALLBACK ldap_sort_strcasecmp( const char **a, 
	const char **b );


/*
 * Filter functions and definitions (an API extension --
 * LDAP_API_FEATURE_X_FILTER_FUNCTIONS)
 */
/*
 * Structures, constants, and types for filter utility routines:
 */
typedef struct ldap_filt_info {
	char                    *lfi_filter;
	char                    *lfi_desc;
	int                     lfi_scope;      /* LDAP_SCOPE_BASE, etc */
	int                     lfi_isexact;    /* exact match filter? */
	struct ldap_filt_info   *lfi_next;
} LDAPFiltInfo;

#define LDAP_FILT_MAXSIZ        1024

typedef struct ldap_filt_list LDAPFiltList; /* opaque filter list handle */
typedef struct ldap_filt_desc LDAPFiltDesc; /* opaque filter desc handle */

/*
 * Filter utility functions:
 */
LDAP_API(LDAPFiltDesc *) LDAP_CALL ldap_init_getfilter( char *fname );
LDAP_API(LDAPFiltDesc *) LDAP_CALL ldap_init_getfilter_buf( char *buf, 
	long buflen );
LDAP_API(LDAPFiltInfo *) LDAP_CALL ldap_getfirstfilter( LDAPFiltDesc *lfdp,
	char *tagpat, char *value );
LDAP_API(LDAPFiltInfo *) LDAP_CALL ldap_getnextfilter( LDAPFiltDesc *lfdp );
LDAP_API(int) LDAP_CALL ldap_set_filter_additions( LDAPFiltDesc *lfdp, 
	char *prefix, char *suffix );
LDAP_API(int) LDAP_CALL ldap_create_filter( char *buf, unsigned long buflen,
	char *pattern, char *prefix, char *suffix, char *attr,
	char *value, char **valwords );
LDAP_API(void) LDAP_CALL ldap_getfilter_free( LDAPFiltDesc *lfdp );


/*
 * Friendly mapping structure and routines (an API extension)
 */
typedef struct friendly {
	char    *f_unfriendly;
	char    *f_friendly;
} *FriendlyMap;
LDAP_API(char *) LDAP_CALL ldap_friendly_name( char *filename, char *name,
	FriendlyMap *map );
LDAP_API(void) LDAP_CALL ldap_free_friendlymap( FriendlyMap *map );


/*
 * In Memory Cache (an API extension -- LDAP_API_FEATURE_X_MEMCACHE)
 */
typedef struct ldapmemcache  LDAPMemCache;  /* opaque in-memory cache handle */

LDAP_API(int) LDAP_CALL ldap_memcache_init( unsigned long ttl,
	unsigned long size, char **baseDNs, struct ldap_thread_fns *thread_fns, 
	LDAPMemCache **cachep );
LDAP_API(int) LDAP_CALL ldap_memcache_set( LDAP *ld, LDAPMemCache *cache );
LDAP_API(int) LDAP_CALL ldap_memcache_get( LDAP *ld, LDAPMemCache **cachep );
LDAP_API(void) LDAP_CALL ldap_memcache_flush( LDAPMemCache *cache, char *dn,
	int scope );
LDAP_API(void) LDAP_CALL ldap_memcache_destroy( LDAPMemCache *cache );
LDAP_API(void) LDAP_CALL ldap_memcache_update( LDAPMemCache *cache );


/*
 * DNS resolver callbacks (an API extension --LDAP_API_FEATURE_X_DNS_FUNCTIONS).
 * Note that gethostbyaddr() is not currently used.
 */
#define LDAP_OPT_DNS_FN_PTRS		0x60	/* 96 - API extension */

typedef struct LDAPHostEnt {
    char	*ldaphe_name;		/* official name of host */
    char	**ldaphe_aliases;	/* alias list */
    int		ldaphe_addrtype;	/* host address type */
    int		ldaphe_length;		/* length of address */
    char	**ldaphe_addr_list;	/* list of addresses from name server */
} LDAPHostEnt;

typedef LDAPHostEnt * (LDAP_C LDAP_CALLBACK LDAP_DNSFN_GETHOSTBYNAME)(
	const char *name, LDAPHostEnt *result, char *buffer,
	int buflen, int *statusp, void *extradata );
typedef LDAPHostEnt * (LDAP_C LDAP_CALLBACK LDAP_DNSFN_GETHOSTBYADDR)(
	const char *addr, int length, int type, LDAPHostEnt *result,
	char *buffer, int buflen, int *statusp, void *extradata );

struct ldap_dns_fns {
	void				*lddnsfn_extradata;
	int				lddnsfn_bufsize;
	LDAP_DNSFN_GETHOSTBYNAME	*lddnsfn_gethostbyname;
	LDAP_DNSFN_GETHOSTBYADDR	*lddnsfn_gethostbyaddr;
};


/*
 * Timeout value for nonblocking connect call
 */
#define	LDAP_X_OPT_CONNECT_TIMEOUT    (LDAP_OPT_PRIVATE_EXTENSION_BASE + 0x0F01)
        /* 0x4000 + 0x0F01 = 0x4F01 = 20225 - API extension */

/********* the functions in the following section are experimental ***********/

/*
 * Memory allocation callback functions (an API extension --
 * LDAP_API_FEATURE_X_MEMALLOC_FUNCTIONS).  These are global and can
 * not be set on a per-LDAP session handle basis.  Install your own
 * functions by making a call like this:
 *    ldap_set_option( NULL, LDAP_OPT_MEMALLOC_FN_PTRS, &memalloc_fns );
 *
 * look in lber.h for the function typedefs themselves.
 */
#define LDAP_OPT_MEMALLOC_FN_PTRS	0x61	/* 97 - API extension */

struct ldap_memalloc_fns {
	LDAP_MALLOC_CALLBACK	*ldapmem_malloc;
	LDAP_CALLOC_CALLBACK	*ldapmem_calloc;
	LDAP_REALLOC_CALLBACK	*ldapmem_realloc;
	LDAP_FREE_CALLBACK	*ldapmem_free;
};


/*
 * Server reconnect (an API extension).
 */
#define LDAP_OPT_RECONNECT		0x62	/* 98 - API extension */


/*
 * Extra thread callback functions (an API extension --
 * LDAP_API_FEATURE_X_EXTHREAD_FUNCTIONS)
 */
#define LDAP_OPT_EXTRA_THREAD_FN_PTRS  0x65	/* 101 - API extension */

typedef int (LDAP_C LDAP_CALLBACK LDAP_TF_MUTEX_TRYLOCK_CALLBACK)( void *m );
typedef void *(LDAP_C LDAP_CALLBACK LDAP_TF_SEMA_ALLOC_CALLBACK)( void );
typedef void (LDAP_C LDAP_CALLBACK LDAP_TF_SEMA_FREE_CALLBACK)( void *s );
typedef int (LDAP_C LDAP_CALLBACK LDAP_TF_SEMA_WAIT_CALLBACK)( void *s );
typedef int (LDAP_C LDAP_CALLBACK LDAP_TF_SEMA_POST_CALLBACK)( void *s );
typedef void *(LDAP_C LDAP_CALLBACK LDAP_TF_THREADID_CALLBACK)(void);

struct ldap_extra_thread_fns {
        LDAP_TF_MUTEX_TRYLOCK_CALLBACK *ltf_mutex_trylock;
        LDAP_TF_SEMA_ALLOC_CALLBACK *ltf_sema_alloc;
        LDAP_TF_SEMA_FREE_CALLBACK *ltf_sema_free;
        LDAP_TF_SEMA_WAIT_CALLBACK *ltf_sema_wait;
        LDAP_TF_SEMA_POST_CALLBACK *ltf_sema_post;
	LDAP_TF_THREADID_CALLBACK *ltf_threadid_fn;
};


/*
 * Asynchronous I/O (an API extension).
 */
/*
 * This option enables completely asynchronous IO.  It works by using ioctl()
 * on the fd, (or tlook()) 
 */
#define LDAP_OPT_ASYNC_CONNECT          0x63	/* 99 - API extension */

/*
 * Debugging level (an API extension)
 */
#define LDAP_OPT_DEBUG_LEVEL		0x6E	/* 110 - API extension */
/* On UNIX, there's only one copy of ldap_debug */
/* On NT, each dll keeps its own module_ldap_debug, which */
/* points to the process' ldap_debug and needs initializing after load */
#ifdef _WIN32
extern int		*module_ldap_debug;
typedef void (*set_debug_level_fn_t)(int*);
#endif
 
/************************ end of experimental section ************************/


/********** the functions, etc. below are unsupported at this time ***********/
#ifdef LDAP_DNS
#define LDAP_OPT_DNS			0x0C	/* 12 - API extension */
#endif


/*
 * generalized bind
 */
/*
 * Authentication methods:
 */
#define LDAP_AUTH_NONE          0x00L
#define LDAP_AUTH_SIMPLE        0x80L
#define LDAP_AUTH_SASL		0xa3L
LDAP_API(int) LDAP_CALL ldap_bind( LDAP *ld, const char *who, 
	const char *passwd, int authmethod );
LDAP_API(int) LDAP_CALL ldap_bind_s( LDAP *ld, const char *who, 
	const char *cred, int method );

/*
 * experimental DN format support
 */
LDAP_API(char **) LDAP_CALL ldap_explode_dns( const char *dn );
LDAP_API(int) LDAP_CALL ldap_is_dns_dn( const char *dn );


/*
 * user friendly naming/searching routines
 */
typedef int (LDAP_C LDAP_CALLBACK LDAP_CANCELPROC_CALLBACK)( void *cl );
LDAP_API(int) LDAP_CALL ldap_ufn_search_c( LDAP *ld, char *ufn,
	char **attrs, int attrsonly, LDAPMessage **res,
	LDAP_CANCELPROC_CALLBACK *cancelproc, void *cancelparm );
LDAP_API(int) LDAP_CALL ldap_ufn_search_ct( LDAP *ld, char *ufn,
	char **attrs, int attrsonly, LDAPMessage **res,
	LDAP_CANCELPROC_CALLBACK *cancelproc, void *cancelparm, 
	char *tag1, char *tag2, char *tag3 );
LDAP_API(int) LDAP_CALL ldap_ufn_search_s( LDAP *ld, char *ufn,
	char **attrs, int attrsonly, LDAPMessage **res );
LDAP_API(LDAPFiltDesc *) LDAP_CALL ldap_ufn_setfilter( LDAP *ld, char *fname );
LDAP_API(void) LDAP_CALL ldap_ufn_setprefix( LDAP *ld, char *prefix );
LDAP_API(int) LDAP_C ldap_ufn_timeout( void *tvparam );

/*
 * utility routines
 */
LDAP_API(int) LDAP_CALL ldap_charray_add( char ***a, char *s );
LDAP_API(int) LDAP_CALL ldap_charray_merge( char ***a, char **s );
LDAP_API(void) LDAP_CALL ldap_charray_free( char **array );
LDAP_API(int) LDAP_CALL ldap_charray_inlist( char **a, char *s );
LDAP_API(char **) LDAP_CALL ldap_charray_dup( char **a );
LDAP_API(char **) LDAP_CALL ldap_str2charray( char *str, char *brkstr );
LDAP_API(int) LDAP_CALL ldap_charray_position( char **a, char *s );

/*
 * UTF-8 routines (should these move into libnls?)
 */
/* number of bytes in character */
LDAP_API(int) LDAP_CALL ldap_utf8len( const char* );
/* find next character */
LDAP_API(char*) LDAP_CALL ldap_utf8next( char* );
/* find previous character */
LDAP_API(char*) LDAP_CALL ldap_utf8prev( char* );
/* copy one character */
LDAP_API(int) LDAP_CALL ldap_utf8copy( char* dst, const char* src );
/* total number of characters */
LDAP_API(size_t) LDAP_CALL ldap_utf8characters( const char* );
/* get one UCS-4 character, and move *src to the next character */
LDAP_API(unsigned long) LDAP_CALL ldap_utf8getcc( const char** src );
/* UTF-8 aware strtok_r() */
LDAP_API(char*) LDAP_CALL ldap_utf8strtok_r( char* src, const char* brk, char** next);

/* like isalnum(*s) in the C locale */
LDAP_API(int) LDAP_CALL ldap_utf8isalnum( char* s );
/* like isalpha(*s) in the C locale */
LDAP_API(int) LDAP_CALL ldap_utf8isalpha( char* s );
/* like isdigit(*s) in the C locale */
LDAP_API(int) LDAP_CALL ldap_utf8isdigit( char* s );
/* like isxdigit(*s) in the C locale */
LDAP_API(int) LDAP_CALL ldap_utf8isxdigit(char* s );
/* like isspace(*s) in the C locale */
LDAP_API(int) LDAP_CALL ldap_utf8isspace( char* s );

#define LDAP_UTF8LEN(s)  ((0x80 & *(unsigned char*)(s)) ?   ldap_utf8len (s) : 1)
#define LDAP_UTF8NEXT(s) ((0x80 & *(unsigned char*)(s)) ?   ldap_utf8next(s) : (s)+1)
#define LDAP_UTF8INC(s)  ((0x80 & *(unsigned char*)(s)) ? s=ldap_utf8next(s) : ++s)

#define LDAP_UTF8PREV(s)   ldap_utf8prev(s)
#define LDAP_UTF8DEC(s) (s=ldap_utf8prev(s))

#define LDAP_UTF8COPY(d,s) ((0x80 & *(unsigned char*)(s)) ? ldap_utf8copy(d,s) : ((*(d) = *(s)), 1))
#define LDAP_UTF8GETCC(s) ((0x80 & *(unsigned char*)(s)) ? ldap_utf8getcc (&s) : *s++)
#define LDAP_UTF8GETC(s) ((0x80 & *(unsigned char*)(s)) ? ldap_utf8getcc ((const char**)&s) : *s++)

/*
 * functions and definitions that have been replaced by new improved ones
 */
/*
 * Use ldap_get_option() with LDAP_OPT_API_INFO and an LDAPAPIInfo structure
 * instead of ldap_version().
 */
typedef struct _LDAPVersion {
	int sdk_version;      /* Version of the SDK, * 100 */
	int protocol_version; /* Highest protocol version supported, * 100 */
	int SSL_version;      /* SSL version if this SDK supports it, * 100 */
	int security_level;   /* highest level available */
	int reserved[4];
} LDAPVersion;
#define LDAP_SECURITY_NONE      0
LDAP_API(int) LDAP_CALL ldap_version( LDAPVersion *ver );

/* use ldap_create_filter() instead of ldap_build_filter() */
LDAP_API(void) LDAP_CALL ldap_build_filter( char *buf, unsigned long buflen,
	char *pattern, char *prefix, char *suffix, char *attr,
	char *value, char **valwords );
/* use ldap_set_filter_additions() instead of ldap_setfilteraffixes() */
LDAP_API(void) LDAP_CALL ldap_setfilteraffixes( LDAPFiltDesc *lfdp, 
	char *prefix, char *suffix );

/* older result types a server can return -- use LDAP_RES_MODDN instead */
#define LDAP_RES_MODRDN                 LDAP_RES_MODDN
#define LDAP_RES_RENAME			LDAP_RES_MODDN

/* older error messages */
#define LDAP_AUTH_METHOD_NOT_SUPPORTED  LDAP_STRONG_AUTH_NOT_SUPPORTED

/*
 * Generalized cache callback interface:
 */
#define LDAP_OPT_CACHE_FN_PTRS          0x0D	/* 13 - API extension */
#define LDAP_OPT_CACHE_STRATEGY         0x0E	/* 14 - API extension */
#define LDAP_OPT_CACHE_ENABLE           0x0F	/* 15 - API extension */

/* cache strategies */
#define LDAP_CACHE_CHECK                0
#define LDAP_CACHE_POPULATE             1
#define LDAP_CACHE_LOCALDB              2

typedef int (LDAP_C LDAP_CALLBACK LDAP_CF_BIND_CALLBACK)( LDAP *ld, int msgid,
	unsigned long tag, const char *dn, const struct berval *creds,
	int method);
typedef int (LDAP_C LDAP_CALLBACK LDAP_CF_UNBIND_CALLBACK)( LDAP *ld,
	int unused0, unsigned long unused1 );
typedef int (LDAP_C LDAP_CALLBACK LDAP_CF_SEARCH_CALLBACK)( LDAP *ld,
	int msgid, unsigned long tag, const char *base, int scope,
	const char LDAP_CALLBACK *filter, char **attrs, int attrsonly );
typedef int (LDAP_C LDAP_CALLBACK LDAP_CF_COMPARE_CALLBACK)( LDAP *ld,
	int msgid, unsigned long tag, const char *dn, const char *attr,
	const struct berval *value );
typedef int (LDAP_C LDAP_CALLBACK LDAP_CF_ADD_CALLBACK)( LDAP *ld,
	int msgid, unsigned long tag, const char *dn, LDAPMod **attrs );
typedef int (LDAP_C LDAP_CALLBACK LDAP_CF_DELETE_CALLBACK)( LDAP *ld,
	int msgid, unsigned long tag, const char *dn );
typedef int (LDAP_C LDAP_CALLBACK LDAP_CF_MODIFY_CALLBACK)( LDAP *ld,
	int msgid, unsigned long tag, const char *dn, LDAPMod **mods );
typedef int (LDAP_C LDAP_CALLBACK LDAP_CF_MODRDN_CALLBACK)( LDAP *ld,
	int msgid, unsigned long tag, const char *dn, const char *newrdn,
	int deleteoldrdn );
typedef int (LDAP_C LDAP_CALLBACK LDAP_CF_RESULT_CALLBACK)( LDAP *ld,
	int msgid, int all, struct timeval *timeout, LDAPMessage **result );
typedef int (LDAP_C LDAP_CALLBACK LDAP_CF_FLUSH_CALLBACK)( LDAP *ld,
	const char *dn, const char *filter );

struct ldap_cache_fns {
	void    *lcf_private;
	LDAP_CF_BIND_CALLBACK *lcf_bind;
	LDAP_CF_UNBIND_CALLBACK *lcf_unbind;
	LDAP_CF_SEARCH_CALLBACK *lcf_search;
	LDAP_CF_COMPARE_CALLBACK *lcf_compare;
	LDAP_CF_ADD_CALLBACK *lcf_add;
	LDAP_CF_DELETE_CALLBACK *lcf_delete;
	LDAP_CF_MODIFY_CALLBACK *lcf_modify;
	LDAP_CF_MODRDN_CALLBACK *lcf_modrdn;
	LDAP_CF_RESULT_CALLBACK *lcf_result;
	LDAP_CF_FLUSH_CALLBACK *lcf_flush;
};

LDAP_API(int) LDAP_CALL ldap_cache_flush( LDAP *ld, const char *dn,
	const char *filter );

/*********************** end of unsupported functions ************************/

#ifdef __cplusplus
}
#endif
#endif /* _LDAP_H */
