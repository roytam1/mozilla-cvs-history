/* Insert copyright and license here 1997 */

/*
   composec.c --- the crypto interface to MIME generation (see compose.c.)
 */

typedef enum {
  mime_crypto_none,				/* normal unencapsulated MIME message */
  mime_crypto_clear_signed,		/* multipart/signed encapsulation */
  mime_crypto_opaque_signed,	/* application/x-pkcs7-mime (signedData) */
  mime_crypto_encrypted,		/* application/x-pkcs7-mime */
  mime_crypto_signed_encrypted	/* application/x-pkcs7-mime */
} mime_delivery_crypto_state;


typedef struct mime_crypto_closure {
  mime_delivery_crypto_state crypto_state;
  nsOutputFileStream *file;

  /* Slots used when using multipart/signed
	 (clearsigning, or signing and encrypting)
   */
  PRInt16 hash_type;
  nsCOMPtr<nsIHash> data_hash_context;

  MimeEncoderData *sig_encoder_data;
  char *multipart_signed_boundary;


  /* Slots used when encrypting (or signing and encrypting)
   */
  nsCOMPtr<nsIX509Cert> self_signing_cert;
  nsCOMPtr<nsIX509Cert> self_encryption_cert;
  nsCOMPtr<nsISupportsArray> certs;	/* ist of the certs to which this (encrypted) message is being addressed. */

  nsCOMPtr<nsICMSMessage> encryption_cinfo;
  nsCOMPtr<nsICMSEncoder> encryption_context;

  MimeEncoderData *crypto_encoder_data;

  PRBool isDraft;

} mime_crypto_closure;


static void
free_crypto_closure (mime_crypto_closure *state)
{
  if (state->encryption_context) {
    state->encryption_context->Finish(nsnull, nsnull);
    state->encryption_context = 0;
  }
  if (state->encryption_cinfo) {
    state->encryption_cinfo = 0;
  }
  if (state->sig_encoder_data) {
	  MimeEncoderDestroy (state->sig_encoder_data, PR_TRUE);
  }
  if (state->crypto_encoder_data) {
	  MimeEncoderDestroy (state->crypto_encoder_data, PR_TRUE);
  }

  // XXX Free the 'certs' array. Fix Me XXX //
  PR_FREEIF(state->multipart_signed_boundary);
  PR_FREE(state);
}


extern void msg_SetCompositionSecurityError(MWContext *, int status);

static void
msg_remember_security_error(mime_crypto_closure *state, int status)
{
  if (status >= 0) return;
  PR_ASSERT(state && state->context->type == MWContextMessageComposition);
  if (!state) return;
  msg_SetCompositionSecurityError(state->context, status);
}


extern char *mime_make_separator(const char *prefix);


/* Returns a string consisting of a Content-Type header, and a boundary
   string, suitable for moving from the header block, down into the body
   of a multipart object.  The boundary itself is also returned (so that
   the caller knows what to write to close it off.)
 */
static int
make_multipart_signed_header_string(PRBool outer_p,
									char **header_return,
									char **boundary_return)
{
  const char *crypto_multipart_blurb = 0;
  
  *header_return = 0;
  *boundary_return = mime_make_separator("ms");

  if (!*boundary_return)
	return MIME_OUT_OF_MEMORY;

  if (outer_p)
	crypto_multipart_blurb = XP_GetString (MK_MIME_MULTIPART_SIGNED_BLURB);

  *header_return =
	PR_smprintf("Content-Type: " MULTIPART_SIGNED "; "
				"protocol=\"" APPLICATION_XPKCS7_SIGNATURE "\"; "
				"micalg=" PARAM_MICALG_SHA1 "; "
				"boundary=\"%s\"" CRLF
				CRLF
				"%s%s"
				"--%s" CRLF,

				*boundary_return,
				(crypto_multipart_blurb ? crypto_multipart_blurb : ""),
				(crypto_multipart_blurb ? CRLF CRLF : ""),
				*boundary_return);

  if (!*header_return)
	{
	  PR_Free(*boundary_return);
	  *boundary_return = 0;
	  return MIME_OUT_OF_MEMORY;
	}

  return 0;
}


static int mime_init_multipart_signed(mime_crypto_closure *, PRBool outer_p);
static int mime_init_encryption(mime_crypto_closure *state, PRBool sign_p);
static int mime_crypto_hack_certs(mime_crypto_closure *,
				  const char *recipients,
				  PRBool encrypt_p,
				  PRBool sign_p);


/* Called by compose.c / msgsend.cpp, after the main headers have been written,
   but before the Content-Type or message body have been written.  This returns
   a mime_crypto_closure object, which will signal the caller to write data
   into the message via mime_crypto_write_block() rather than writing it
   directly to the file.
 */
int
mime_begin_crypto_encapsulation (XP_File file, void **closure_return,
								 PRBool encrypt_p, PRBool sign_p,
								 const char *recipients,
								 PRBool saveAsDraft)
{
  int status = 0;
  mime_crypto_closure *state;

  *closure_return = 0;

  PR_ASSERT(encrypt_p || sign_p);
  if (!encrypt_p && !sign_p) return 0;

  state = (mime_crypto_closure *) PR_MALLOC(sizeof(*state));
  if (!state) return MIME_OUT_OF_MEMORY;

  nsCRT::memset(state, 0, sizeof(*state));
  state->context = context;
  state->file = file;

  state->isDraft = saveAsDraft;

  if (encrypt_p && sign_p)
	  state->crypto_state = mime_crypto_signed_encrypted;
  else if (encrypt_p)
	  state->crypto_state = mime_crypto_encrypted;
  else if (sign_p)
	  state->crypto_state = mime_crypto_clear_signed;
  else
	  PR_ASSERT(0);

  status = mime_crypto_hack_certs(state, recipients, encrypt_p, sign_p);
  if (status < 0) goto FAIL;

  if (status > 0) status = 0;  /* We use >0 to mean "send with no crypto." */

  switch (state->crypto_state)
	{
	case mime_crypto_clear_signed:
	  status = mime_init_multipart_signed(state, PR_TRUE);
	  break;
	case mime_crypto_opaque_signed:
	  PR_ASSERT(0);    /* #### no api for this yet */
	  status = -1;
	  break;
	case mime_crypto_signed_encrypted:
	  status = mime_init_encryption(state, PR_TRUE);
	  break;
	case mime_crypto_encrypted:
	  status = mime_init_encryption(state, PR_FALSE);
	  break;
	case mime_crypto_none:
	  /* This can happen if mime_crypto_hack_certs() decided to turn off
		 encryption (by asking the user.) */
	  status = 1;
	  break;
	default:
	  PR_ASSERT(0);
	  status = -1;
	  break;
	}

FAIL:
  msg_remember_security_error(state, status);

  if (status != 0) {
	  free_crypto_closure(state);
  } else {
	  *closure_return = state;
  }

  return status;
}


/* Helper function for mime_begin_crypto_encapsulation() to start a
   multipart/signed object (for signed or signed-and-encrypted messages.)
 */
static int
mime_init_multipart_signed(mime_crypto_closure *state, PRBool outer_p)
{

  /* First, construct and write out the multipart/signed MIME header data.
   */

  char *header = 0;
  PRInt32 L;
  int status =
	make_multipart_signed_header_string(outer_p, &header,
										&state->multipart_signed_boundary);
  if (status < 0) goto FAIL;

  L = nsCRT::strlen(header);

  if (outer_p){
	  /* If this is the outer block, write it to the file. */
    if (PRInt32(file->write(header, L)) < L) {
		  status = MK_MIME_ERROR_WRITING_FILE;
    }
  } else {
	  /* If this is an inner block, feed it through the crypto stream. */
	  status = mime_crypto_write_block (state, header, L);
	}

  PR_Free(header);
  if (status < 0) goto FAIL;

  /* Now initialize the crypto library, so that we can compute a hash
	 on the object which we are signing.
   */

  state->hash_type = nsIHash.HASH_AlgSHA1;

  PR_SetError(0,0);
  state->data_hash_context = do_CreateInstance(NS_HASH_CONTRACTID, &rv);
  if (NS_FAILED(rv)) return 0;

  rv = state->data_hash_context->Create(state->hash_type));
  if (NS_FAILED(rv))
	{
	  status = PR_GetError();
	  PR_ASSERT(status < 0);
	  if (status >= 0) status = -1;
	  goto FAIL;
	}


  PR_SetError(0,0);
  rv = state->data_hash_context->Begin();
  status = PR_GetError();
 FAIL:
  msg_remember_security_error(state, status);
  return status;
}


static void mime_crypto_write_base64 (void *closure, const char *buf,
									  unsigned long size);
static int mime_encoder_output_fn (const char *buf, int32 size, void *closure);
static int mime_nested_encoder_output_fn (const char *, int32, void *closure);


/* Helper function for mime_begin_crypto_encapsulation() to start an
   an opaque crypto object (for encrypted or signed-and-encrypted messages.)
 */
static int
mime_init_encryption(mime_crypto_closure *state, PRBool sign_p)
{
  int status = 0;

  /* First, construct and write out the opaque-crypto-blob MIME header data.
   */

  char *s =
	PR_smprintf("Content-Type: " APPLICATION_XPKCS7_MIME
				  "; name=\"smime.p7m\"" CRLF
				"Content-Transfer-Encoding: " ENCODING_BASE64 CRLF
				"Content-Disposition: attachment"
				  "; filename=\"smime.p7m\"" CRLF
				"Content-Description: %s" CRLF
				CRLF,
				XP_GetString(MK_MIME_SMIME_ENCRYPTED_CONTENT_DESCRIPTION));
  PRInt32 L;
  if (!s) return MIME_OUT_OF_MEMORY;
  L = nsCRT::strlen(s);
  if (PRInt32(state->file->write(s, L)) < L) {
	  return MK_MIME_ERROR_WRITING_FILE;
  }
  PR_Free(s);
  s = 0;

  /* Now initialize the crypto library, so that we can filter the object
	 to be encrypted through it.
   */

  if (! state->isDraft) {
	  PR_ASSERT(state->certs[0]);
	  if (!state->certs[0]) return -1;
  }

  /* Initialize the base64 encoder. */
  PR_ASSERT(!state->crypto_encoder_data);
  state->crypto_encoder_data = MimeB64EncoderInit(mime_encoder_output_fn,
												  state);
  if (!state->crypto_encoder_data)
	return MIME_OUT_OF_MEMORY;

  /* Initialize the encrypter (and add the sender's cert.) */
  PR_ASSERT(state->self_encryption_cert);
  PR_SetError(0,0);
  state->encryption_cinfo = do_CreateInstance(NS_CMSMESSAGE_CONTRACTID, &rv);
  if (NS_FAILED(rv)) return 0;
  rv = state->encryption_cinfo->CreateEncrypted(); // XXX Fix this later XXX //
  if (NS_FAILED(rv)) {
	  status = PR_GetError();
	  PR_ASSERT(status < 0);
	  if (status >= 0) status = -1;
	  goto FAIL;
	}

  PR_SetError(0,0);
  state->encryption_context = do_CreateInstance(NS_CMSENCODER_CONTRACTID, &rv);
  if (NS_FAILED(rv)) return 0;

  rv = state->encryption_context->Start(); // XXX Fix this later XXX //
  if (NS_FAILED(rv)) {
	  status = PR_GetError();
	  PR_ASSERT(status < 0);
	  if (status >= 0) status = -1;
	  goto FAIL;
	}

  /* If we're signing, tack a multipart/signed header onto the front of
	 the data to be encrypted, and initialize the sign-hashing code too.
   */
  if (sign_p) {
	  status = mime_init_multipart_signed(state, PR_FALSE);
	  if (status < 0) goto FAIL;
	}

 FAIL:
  msg_remember_security_error(state, status);
  return status;
}

static int mime_finish_multipart_signed (mime_crypto_closure *state,
										 PRBool outer_p);
static int mime_finish_encryption (mime_crypto_closure *state, PRBool sign_p);


/* Called by compose.c / msgsend.cpp, when the whole body has been processed.
 */
int
mime_finish_crypto_encapsulation (void *closure, PRBool abort_p)
{
  mime_crypto_closure *state = (mime_crypto_closure *) closure;
  int status = 0;

  if (!abort_p)
	{
	  switch (state->crypto_state)
		{
		case mime_crypto_clear_signed:
		  status = mime_finish_multipart_signed (state, PR_TRUE);
		  break;
		case mime_crypto_opaque_signed:
		  PR_ASSERT(0);    /* #### no api for this yet */
		  status -1;
		  break;
		case mime_crypto_signed_encrypted:
		  status = mime_finish_encryption (state, PR_TRUE);
		  break;
		case mime_crypto_encrypted:
		  status = mime_finish_encryption (state, PR_FALSE);
		  break;
		default:
		  PR_ASSERT(0);
		  status = -1;
		  break;
		}
	}

  msg_remember_security_error(state, status);
  free_crypto_closure(state);
  return status;
}


/* Helper function for mime_finish_crypto_encapsulation() to close off
   a multipart/signed object (for signed or signed-and-encrypted messages.)
 */
static int
mime_finish_multipart_signed (mime_crypto_closure *state, PRBool outer_p)
{
  int status;
  PRUint32 sec_item_len;
  unsigned char * sec_item_data;
  nsCOMPtr<nsIHash> hash = do_CreateInstance(NS_HASH_CONTRACTID,&rv);
  nsCOMPtr<nsICMSMessage> cinfo = do_CreateInstance(NS_CMSMESSAGE_CONTRACTID, &rv);
  nsCOMPtr<nsICMSEncoder> encoder = do_CreateInstance(NS_CMSENCODER_CONTRACTID, &rv);
  PRStatus ds_status = PR_FAILURE;
  nsresult rv;

  /* Compute the hash...
   */
  sec_item_len = state->data_hash_context->ResultLen(state->hash_type);
  sec_item_data = (unsigned char *) PR_MALLOC(sec_item.len);
  if (!sec_item_data)	{
	  status = MIME_OUT_OF_MEMORY;
	  goto FAIL;
	}

  PR_SetError(0,0);
  state->data_hash_context->End(sec_item.data, &sec_item_len, sec_item_len);
  status = PR_GetError();
  if (status < 0) {
    goto FAIL;
  }

  PR_SetError(0,0);
  state->data_hash_context = 0;

  status = PR_GetError();
  if (status < 0) goto FAIL;

  /* Write out the headers for the signature.
   */
  {
	PRInt32 L;
	char *header =
	  PR_smprintf(CRLF
				  "--%s" CRLF
				  "Content-Type: " APPLICATION_XPKCS7_SIGNATURE
				    "; name=\"smime.p7s\"" CRLF
				  "Content-Transfer-Encoding: " ENCODING_BASE64 CRLF
				  "Content-Disposition: attachment; "
				    "filename=\"smime.p7s\"" CRLF
				  "Content-Description: %s" CRLF
				  CRLF,
				  state->multipart_signed_boundary,
				  XP_GetString(MK_MIME_SMIME_SIGNATURE_CONTENT_DESCRIPTION));
	if (!header) {
		status = MIME_OUT_OF_MEMORY;
		goto FAIL;
	}

	L = nsCRT::strlen(header);
	if (outer_p) {
		/* If this is the outer block, write it to the file. */
		if (PRInt32(state->file->write(header, L)) < L)
		  status = MK_MIME_ERROR_WRITING_FILE;
    } else {
		  /* If this is an inner block, feed it through the crypto stream. */
		  status = mime_crypto_write_block (state, header, L);
	  }
  }

	PR_Free(header);
	if (status < 0) goto FAIL;
  }

  /* Create the signature...
   */

  PR_ASSERT(state->hash_type == nsIHash.HASH_AlgSHA1);

  PR_ASSERT (state->self_signing_cert);
  PR_SetError(0,0);
  rv = cinfo->CreateSigned();
  if (NS_FAILED(rv))	{
	  status = PR_GetError();
	  PR_ASSERT(status < 0);
	  if (status >= 0) status = -1;
	  goto FAIL;
	}

  /* Initialize the base64 encoder for the signature data.
   */
  PR_ASSERT(!state->sig_encoder_data);
  state->sig_encoder_data =
	MimeB64EncoderInit((outer_p
						? mime_encoder_output_fn
						: mime_nested_encoder_output_fn),
					   state);
  if (!state->sig_encoder_data) {
	  status = MIME_OUT_OF_MEMORY;
	  goto FAIL;
	}

  /* Write out the signature.
   */
  PR_SetError(0,0);
  rv = encoder->Encode (cinfo);
  if (ds_status != SECSuccess) {
	  status = PR_GetError();
	  PR_ASSERT(status < 0);
	  if (status >= 0) status = -1;
	  goto FAIL;
	}

  /* Shut down the sig's base64 encoder.
   */
  status = MimeEncoderDestroy(state->sig_encoder_data, PR_FALSE);
  state->sig_encoder_data = 0;
  if (status < 0) goto FAIL;

  /* Now write out the terminating boundary.
   */
  {
	PRInt32 L;
	char *header = PR_smprintf(CRLF "--%s--" CRLF,
							   state->multipart_signed_boundary);
	PR_Free(state->multipart_signed_boundary);
	state->multipart_signed_boundary = 0;

	if (!header) {
		status = MIME_OUT_OF_MEMORY;
		goto FAIL;
	}
	L = nsCRT::strlen(header);
	if (outer_p) {
		/* If this is the outer block, write it to the file. */
		if (PRInt32(state->file->write(header, L)) < L)
		  status = MK_MIME_ERROR_WRITING_FILE;
  } else {
		/* If this is an inner block, feed it through the crypto stream. */
		status = mime_crypto_write_block (state, header, L);
	}
  }

FAIL:
  PR_FREEIF(sec_item_data);

  msg_remember_security_error(state, status);
  return status;
}


/* Helper function for mime_finish_crypto_encapsulation() to close off
   an opaque crypto object (for encrypted or signed-and-encrypted messages.)
 */
static int
mime_finish_encryption (mime_crypto_closure *state, PRBool sign_p)
{
  int status = 0;

  /* If this object is both encrypted and signed, close off the
	 signature first (since it's inside.) */
  if (sign_p)
	{
	  status = mime_finish_multipart_signed (state, PR_FALSE);
	  if (status < 0) goto FAIL;
	}

  /* Close off the opaque encrypted blob.
   */
  PR_ASSERT(state->encryption_context);
  if (!state->encryption_context) status = -1;
  PR_SetError(0,0);
  rv = state->encryption_context->Finish();
  if (NS_FAILED(rv)) {
	  status = PR_GetError();
	  PR_ASSERT(status < 0);
	  if (status >= 0) status = -1;
	}

  state->encryption_context = 0;
  if (status < 0) goto FAIL;

  PR_ASSERT(state->encryption_cinfo);
  if (!state->encryption_cinfo) {
    status = -1;
  }
  if (state->encryption_cinfo) {
    state->encryption_cinfo = 0;
  }
  if (status < 0) goto FAIL;

  /* Shut down the base64 encoder. */
  status = MimeEncoderDestroy(state->crypto_encoder_data, PR_FALSE);
  state->crypto_encoder_data = 0;

  if (status < 0) goto FAIL;

 FAIL:
  msg_remember_security_error(state, status);
  return status;
}




extern char *MSG_ExtractRFC822AddressMailboxes (const char *line);
extern char *MSG_RemoveDuplicateAddresses (const char *addrs,
										   const char *other_addrs,
										   PRBool removeAliasesToMe);
extern int MSG_ParseRFC822Addresses (const char *line,
									 char **names, char **addresses);


static CERTCertificate *
msg_get_cert(const char *email_addr)
{
  CERTCertificate * cert = NULL;
  CERTCertList *certList = NULL;
  SECStatus rv;
  
  certList = CERT_CreateEmailAddrCertList(NULL, CERT_GetDefaultCertDB(),
										  (char *)email_addr, PR_Now(),
										  PR_PR_TRUE);

  if ( certList == NULL ) {
	return NULL;
  }

  rv = CERT_FilterCertListByUsage(certList, certUsageEmailRecipient, PR_PR_FALSE);
  
  if ( ! CERT_LIST_END(CERT_LIST_HEAD(certList), certList) ) {
	  cert = CERT_DupCertificate(CERT_LIST_HEAD(certList)->cert);
  }

  CERT_DestroyCertList(certList);
  
  return cert;   /* cert may point to a cert or may be NULL */
}


/* Used to figure out what certs should be used when encrypting this message.
 */
static int
mime_crypto_hack_certs(mime_crypto_closure *state, const char *recipients,
					   PRBool encrypt_p, PRBool sign_p)
{
  int status = 0;
  char *all_mailboxes = 0, *mailboxes = 0, *mailbox_list = 0;
  const char *mailbox = 0;
  int count = 0;
/*  char **nocerts = 0; */
  int32 nocert_count = 0;
  PRBool no_clearsigning_p = PR_FALSE;
  CERTCertificate **certs_tail;

  PR_ASSERT(encrypt_p || sign_p);

  {
	state->self_encryption_cert =
	  SECNAV_GetDefaultEMailCert(certUsageEmailRecipient, state->context);
	state->self_signing_cert =
	  SECNAV_GetDefaultEMailCert(certUsageEmailSigner, state->context);

	if ((state->self_signing_cert == NULL) && sign_p)
	  {
		status = SEC_ERROR_NO_EMAIL_CERT;
		goto FAIL;
	  }
	if ((state->self_encryption_cert == NULL) && encrypt_p)
	  {
		status = SEC_ERROR_NO_EMAIL_CERT;
		goto FAIL;
	  }
  }

  all_mailboxes = MSG_ExtractRFC822AddressMailboxes(recipients);
  mailboxes = MSG_RemoveDuplicateAddresses(all_mailboxes, 0, PR_FALSE /*removeAliasesToMe*/);
  FREEIF(all_mailboxes);

  if (mailboxes)
	count = MSG_ParseRFC822Addresses (mailboxes, 0, &mailbox_list);
  FREEIF(mailboxes);
  if (count < 0) return count;

  PR_ASSERT(!state->certs);
  state->certs = (CERTCertificate **)
	PR_MALLOC(sizeof(*state->certs) * (count + 1));
  if (!state->certs)
	{
	  status = MIME_OUT_OF_MEMORY;
	  goto FAIL;
	}

/*  nocerts = (char **) PR_MALLOC(sizeof(char *) * (count+1));
  if (!nocerts)
	{
	  status = MIME_OUT_OF_MEMORY;
	  goto FAIL;
	}
 */

  XP_MEMSET(state->certs, 0, sizeof(*state->certs) * (count + 1));
  certs_tail = state->certs;

  /* If the message is to be encrypted, then get the recipient certs */
  if (encrypt_p) {
	  mailbox = mailbox_list;
	  for (; count > 0; count--)
	  {
		  CERTCertificate *cert = msg_get_cert(mailbox);

		  if (!cert)
		  {
	/*		  nocerts[nocert_count] = (char *) mailbox; */
			  nocert_count++;
			  mailbox += nsCRT::strlen(mailbox) + 1;
			  continue;
		  }

		  *certs_tail++ = cert;

	  /* #### see if recipient requests `signedData'.
		 if (...) no_clearsigning_p = PR_TRUE;
		 (This is the only reason we even bother looking up the certs
		 of the recipients if we're sending a signed-but-not-encrypted
		 message.)
	   */

		  mailbox += nsCRT::strlen(mailbox) + 1;
	  }

	  /* null terminate the list of cert */
	  *certs_tail = NULL;	
 
	  if (nocert_count)
		{
		/* If we wanted to encrypt but couldn't, ask the user whether they'd
		 rather just turn off encryption and send anyway. */
		if (FE_Confirm(state->context,
					 XP_GetString(SEC_ERROR_NO_RECIPIENT_CERTS_QUERY)))
			{
			status = 1;   /* this means "send unencrypted" */

			if (state->crypto_state == mime_crypto_signed_encrypted)
				state->crypto_state = mime_crypto_clear_signed;
			else
				{
				PR_ASSERT(state->crypto_state == mime_crypto_encrypted);
				state->crypto_state = mime_crypto_none;
				}
			}
		else
			{
			status = MK_INTERRUPTED;	/* "fail" without an error dialog */
			}

		goto FAIL;
		}
	}
FAIL:
  FREEIF(mailbox_list);
/*  FREEIF(nocerts); */

  if (status >= 0 &&
	  no_clearsigning_p &&
	  state->crypto_state == mime_crypto_clear_signed)
	state->crypto_state = mime_crypto_opaque_signed;

  if (status < 0)
	msg_remember_security_error(state, status);
  return status;
}



/* Used as the output function of a SEC_PKCS7EncoderContext -- we feed
   plaintext into the crypto engine, and it calls this function with encrypted
   data; then this function writes a base64-encoded representation of that
   data to the file (by filtering it through the given MimeEncoderData object.)

   Also used as the output function of SEC_PKCS7Encode() -- but in that case,
   it's used to write the encoded representation of the signature.  The only
   difference is which MimeEncoderData object is used.
 */
static void
mime_crypto_write_base64 (void *closure, const char *buf,
						  unsigned long size)
{
  MimeEncoderData *data = (MimeEncoderData *) closure;
  int status = MimeEncoderWrite (data, buf, size);
  XP_SetError(status < 0 ? status : 0);
}


/* Used as the output function of MimeEncoderData -- when we have generated
   the signature for a multipart/signed object, this is used to write the
   base64-encoded representation of the signature to the file.
 */
static int
mime_encoder_output_fn (const char *buf, int32 size, void *closure)
{
  mime_crypto_closure *state = (mime_crypto_closure *) closure;
  if ((int32) XP_FileWrite((char *) buf, size, state->file) < size)
	return MK_MIME_ERROR_WRITING_FILE;
  else
	return 0;
}


/* Like mime_encoder_output_fn, except this is used for the case where we
   are both signing and encrypting -- the base64-encoded output of the
   signature should be fed into the crypto engine, rather than being written
   directly to the file.
 */
static int
mime_nested_encoder_output_fn (const char *buf, int32 size, void *closure)
{
  mime_crypto_closure *state = (mime_crypto_closure *) closure;
  return mime_crypto_write_block (state, (char *) buf, size);
}



/* Called by compose.c / msgsend.cpp to write the body of the message.
 */
int
mime_crypto_write_block (void *closure, char *buf, int32 size)
{
  mime_crypto_closure *state = (mime_crypto_closure *) closure;
  int status = 0;

  PR_ASSERT(state);
  if (!state) return -1;

  /* If this is a From line, mangle it before signing it.  You just know
	 that something somewhere is going to mangle it later, and that's
	 going to cause the signature check to fail.

	 (This assumes that, in the cases where From-mangling must happen,
	 this function is called a line at a time.  That happens to be the
	 case.)
  */
  if (size >= 5 && buf[0] == 'F' && !XP_STRNCMP(buf, "From ", 5))
	{
	  char mangle[] = ">";
	  status = mime_crypto_write_block (closure, mangle, 1);
	  if (status < 0)
		return status;
	}


  /* If we're signing, or signing-and-encrypting, feed this data into
	 the computation of the hash. */
  if (state->data_hash_context)
	{
	  PR_SetError(0,0);
	  HASH_Update(state->data_hash_context, (unsigned char *) buf, size);
	  status = PR_GetError();
	  if (status < 0) goto FAIL;
	}

  PR_SetError(0,0);
  if (state->encryption_context)
	{
	  /* If we're encrypting, or signing-and-encrypting, write this data
		 by filtering it through the crypto library. */

	  SECStatus sec_status = SEC_PKCS7EncoderUpdate(state->encryption_context,
													buf, size);
	  if (sec_status != SECSuccess)
		{
		  status = PR_GetError();
		  PR_ASSERT(status < 0);
		  if (status >= 0) status = -1;
		  goto FAIL;
		}
	}
  else
	{
	  /* If we're not encrypting (presumably just signing) then write this
		 data directly to the file. */

	  status = XP_FileWrite (buf, size, state->file);
	  if (status != size)
		return MK_MIME_ERROR_WRITING_FILE;
	}

 FAIL:
  msg_remember_security_error(state, status);
  return status;
}


#ifdef XP_MAC
#include <ConditionalMacros.h>

#pragma global_optimizer on
#pragma optimization_level 4

#endif


/* Crypto preferences
 */


/* Returns PR_TRUE if the user has selected the preference that says that new
   mail messages should be encrypted by default. 
 */
PRBool
MSG_GetMailEncryptionPreference(void)
{
  PRBool result = PR_FALSE;
  PREF_GetBoolPref("mail.encrypt_outgoing_mail", &result);
  return result;
}

/* Returns PR_TRUE if the user has selected the preference that says that new
   mail messages should be cryptographically signed by default. 
 */
PRBool
MSG_GetMailSigningPreference(void)
{
  PRBool result = PR_FALSE;
  PREF_GetBoolPref("mail.crypto_sign_outgoing_mail", &result);
  return result;
}

/* Returns PR_TRUE if the user has selected the preference that says that new
   news messages should be cryptographically signed by default. 
 */
PRBool
MSG_GetNewsSigningPreference(void)
{
  PRBool result = PR_FALSE;
  PREF_GetBoolPref("mail.crypto_sign_outgoing_news", &result);
  return result;
}


#ifdef XP_MAC
#pragma global_optimizer reset
#endif

