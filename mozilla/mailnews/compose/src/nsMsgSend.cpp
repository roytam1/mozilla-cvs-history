/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
#include "nsCOMPtr.h"
#include "nsCRT.h"
#include "msgCore.h"
#include "rosetta_mailnews.h"
#include "nsMsgLocalFolderHdrs.h"
#include "nsMsgSendPart.h"
#include "nsMsgSendFact.h"
#include "nsMsgBaseCID.h"
#include "nsMsgNewsCID.h"
#include "nsIMsgHeaderParser.h"
#include "nsINetService.h"
#include "nsISmtpService.h"  // for actually sending the message...
#include "nsINntpService.h"  // for actually posting the message...
#include "nsMsgCompPrefs.h"
#include "nsIMsgMailSession.h"
#include "nsIMsgIdentity.h"
#include "nsMsgSend.h"
#include "nsEscape.h"
#include "nsIPref.h"
#include "nsIMsgMailNewsUrl.h"
#include "nsMsgDeliveryListener.h"
#include "nsIMimeURLUtils.h"
#include "nsMsgComposeStringBundle.h"
#include "nsMsgEncoders.h"
#include "nsMsgCompUtils.h"
#include "nsMsgI18N.h"
#include "nsIMsgSendListener.h"
#include "nsIMimeURLUtils.h"
#include "nsIMsgCopyServiceListener.h"
#include "nsIFileSpec.h"

#include "nsMsgPrompts.h"

// RICHIE - StringBundle Converstion Place Holder ////////////////////////////
//
#define   MK_MSG_MIME_OBJECT_NOT_AVAILABLE    -9999
#define   MK_ATTACHMENT_LOAD_FAILED           -9998
#define   MK_MAIL_FAILED_COPY_OPERATION       -9997
#define   MK_NEWS_FAILED_COPY_OPERATION       -9996

#define   MIME_MULTIPART_BLURB     "This is a multi-part message in MIME format."
//
// RICHIE - StringBundle Converstion Place Holder ////////////////////////////


/* use these macros to define a class IID for our component. Our object currently supports two interfaces 
   (nsISupports and nsIMsgCompose) so we want to define constants for these two interfaces */
static NS_DEFINE_IID(kIMsgSend, NS_IMSGSEND_IID);
static NS_DEFINE_CID(kSmtpServiceCID, NS_SMTPSERVICE_CID);
static NS_DEFINE_CID(kNntpServiceCID, NS_NNTPSERVICE_CID);
static NS_DEFINE_CID(kNetServiceCID, NS_NETSERVICE_CID); 
static NS_DEFINE_IID(kIPrefIID, NS_IPREF_IID);
static NS_DEFINE_CID(kPrefCID, NS_PREF_CID);
static NS_DEFINE_CID(kMimeURLUtilsCID, NS_IMIME_URLUTILS_CID);


#ifdef XP_MAC
#include "errors.h"
#include "m_cvstrm.h"

XP_BEGIN_PROTOS
extern OSErr my_FSSpecFromPathname(char* src_filename, FSSpec* fspec);
XP_END_PROTOS

static char* NET_GetLocalFileFromURL(char *url)
{
	char * finalPath;
	NS_ASSERTION(PL_strncasecmp(url, "file://", 7) == 0, "invalid url");
	finalPath = (char*)PR_Malloc(strlen(url));
	if (finalPath == nsnull)
		return nsnull;
	strcpy(finalPath, url+6+1);
	return finalPath;
}

static char* NET_GetURLFromLocalFile(char *filename)
{
    /*  file:///<path>0 */
	char * finalPath = (char*)PR_Malloc(strlen(filename) + 8 + 1);
	if (finalPath == nsnull)
		return nsnull;
	finalPath[0] = 0;
	strcat(finalPath, "file://");
	strcat(finalPath, filename);
	return finalPath;
}

#endif /* XP_MAC */

static PRBool mime_use_quoted_printable_p = PR_TRUE;

//
// Ugh, we need to do this currently to access this boolean.
//
PRBool
UseQuotedPrintable(void)
{
  return mime_use_quoted_printable_p;
}

// 
// This function will be used by the factory to generate the 
// nsMsgComposeAndSend Object....
//
nsresult NS_NewMsgSend(const nsIID &aIID, void ** aInstancePtrResult)
{
	/* note this new macro for assertions...they can take a string describing the assertion */
	NS_PRECONDITION(nsnull != aInstancePtrResult, "nsnull ptr");
	if (nsnull != aInstancePtrResult)
	{
		nsMsgComposeAndSend *pSend = new nsMsgComposeAndSend();
		if (pSend)
			return pSend->QueryInterface(kIMsgSend, aInstancePtrResult);
		else
			return NS_ERROR_OUT_OF_MEMORY; /* we couldn't allocate the object */
	}
	else
		return NS_ERROR_NULL_POINTER; /* aInstancePtrResult was nsnull....*/
}



static char *mime_mailto_stream_read_buffer = 0;
static char *mime_mailto_stream_write_buffer = 0;


char * mime_get_stream_write_buffer(void)
{
	if (!mime_mailto_stream_write_buffer)
		mime_mailto_stream_write_buffer = (char *) PR_Malloc(MIME_BUFFER_SIZE);
	return mime_mailto_stream_write_buffer;
}

/* All of the desired attachments have been written to individual temp files,
   and we know what's in them.  Now we need to make a final temp file of the
   actual mail message, containing all of the other files after having been
   encoded as appropriate.
 */
int 
nsMsgComposeAndSend::GatherMimeAttachments()
{
	PRBool shouldDeleteDeliveryState = PR_TRUE;
	PRInt32 status, i;
	char *headers = 0;
	char *separator = 0;
	PRFileDesc  *in_file = 0;
	PRBool multipart_p = PR_FALSE;
	PRBool plaintext_is_mainbody_p = PR_FALSE; // only using text converted from HTML?
	char *buffer = 0;
	char *buffer_tail = 0;
	char* error_msg = nsnull;
 
  // to news is true if we have a m_field and we have a Newsgroup and it is not empty
	PRBool tonews = PR_FALSE;
	if (mCompFields) 
  {
		const char* pstrzNewsgroup = mCompFields->GetNewsgroups();
		if (pstrzNewsgroup && *pstrzNewsgroup)
			tonews = PR_TRUE;
	}

	nsMsgI18NMessageSendToNews(tonews);			// hack to make Korean Mail/News work correctly 
											// Look at libi18n/doc_ccc.c for details 
											// temp solution for bug 30725

	nsMsgSendPart* toppart = nsnull;			// The very top most container of the message
											// that we are going to send.

	nsMsgSendPart* mainbody = nsnull;			// The leaf node that contains the text of the
											// message we're going to contain.

	nsMsgSendPart* maincontainer = nsnull;	// The direct child of toppart that will
											// contain the mainbody.  If mainbody is
											// the same as toppart, then this is
											// also the same.  But if mainbody is
											// to end up somewhere inside of a
											// multipart/alternative or a
											// multipart/related, then this is that
											// multipart object.

	nsMsgSendPart* plainpart = nsnull;		// If we converted HTML into plaintext,
											// the message or child containing the plaintext
											// goes here. (Need to use this to determine
											// what headers to append/set to the main 
											// message body.)
	char *hdrs = 0;
	PRBool maincontainerISrelatedpart = PR_FALSE;

	status = m_status;
	if (status < 0)
		goto FAIL;

	if (m_attachments_only_p)
	{
		/* If we get here, we should be fetching attachments only! */
		if (m_attachments_done_callback) 
    {
			struct nsMsgAttachedFile *attachments;

			NS_ASSERTION(m_attachment_count > 0, "not more attachment");
			if (m_attachment_count <= 0) 
      {
				m_attachments_done_callback (nsnull, nsnull, nsnull);
				m_attachments_done_callback = nsnull;
				// SHERRY: for now, we are going to let the destructor do the Clear();
				goto FAIL;
			}

			attachments = (struct nsMsgAttachedFile *)PR_Malloc((m_attachment_count + 1) * sizeof(*attachments));

			if (!attachments)
				goto FAILMEM;
			memset(attachments, 0, ((m_attachment_count + 1) * sizeof(*attachments)));
			for (i = 0; i < m_attachment_count; i++)
			{
				nsMsgAttachmentHandler *ma = &m_attachments[i];

#undef SNARF
#define SNARF(x,y) do { if((y) && *(y) && !(x)) { ((x) = (y)); ((y) = 0); }} \
				   while(0)
				/* Rather than copying the strings and dealing with allocation
				 failures, we'll just "move" them into the other struct (this
				 should be ok since this file uses PR_FREEIF when discarding
				 the mime_attachment objects.) */

        // RICHIE - not sure on this...but we'll try
				attachments[i].orig_url = ma->mURL;
				attachments[i].file_spec = ma->mFileSpec;

				SNARF(attachments[i].type, ma->m_type);
				SNARF(attachments[i].encoding, ma->m_encoding);
				SNARF(attachments[i].description, ma->m_description);
				SNARF(attachments[i].x_mac_type, ma->m_x_mac_type);
				SNARF(attachments[i].x_mac_creator, ma->m_x_mac_creator);

#undef SNARF
				attachments[i].size = ma->m_size;
				attachments[i].unprintable_count = ma->m_unprintable_count;
				attachments[i].highbit_count = ma->m_highbit_count;
				attachments[i].ctl_count = ma->m_ctl_count;
				attachments[i].null_count = ma->m_null_count;
				attachments[i].max_line_length = ma->m_max_column;
				HJ08239

				/* Doesn't really matter, but let's not lie about encoding
				   in case it does someday. */
				if (attachments[i].highbit_count > 0 && attachments[i].encoding &&
						!PL_strcasecmp(attachments[i].encoding, ENCODING_7BIT))
					attachments[i].encoding = ENCODING_8BIT;
			}

			m_attachments_done_callback(nsnull, nsnull, attachments);
			PR_FREEIF(attachments);
			m_attachments_done_callback = nsnull;
			// SHERRY: for now, we are going to let the destructor do the Clear();
		}
		goto FAIL;
	}


	/* If we get here, we're generating a message, so there shouldn't be an
	   attachments callback. */
	NS_ASSERTION(!m_attachments_done_callback, "shouldn't have an attachement callback!");

	if (!m_attachment1_type) {
		m_attachment1_type = PL_strdup(TEXT_PLAIN);
		if (!m_attachment1_type)
			goto FAILMEM;
	}

	// If we have a text/html main part, and we need a plaintext attachment, then
	// we'll do so now.  This is an asynchronous thing, so we'll kick it off and
	// count on getting back here when it finishes.

	if (m_plaintext == nsnull &&
			(mCompFields->GetForcePlainText() ||
			 mCompFields->GetUseMultipartAlternative()) &&
			 m_attachment1_body && PL_strcmp(m_attachment1_type, TEXT_HTML) == 0)
	{
    //
    // If we get here, we have an HTML body, but we really need to send
    // a text/plain message, so we need to run this through html -> text
    // conversion and write it out to the nsHTMLFileSpec file. This will
    // be sent instead as the primary message body.
    //
    // RICHIE - we need that plain text converter!
    //
    mHTMLFileSpec = nsMsgCreateTempFileSpec("nsmail.tmp");
		if (!mHTMLFileSpec)
			goto FAILMEM;

		nsOutputFileStream tempfile(*mHTMLFileSpec);
		if (! tempfile.is_open()) 
    {
			status = MK_UNABLE_TO_OPEN_TMP_FILE;
			goto FAIL;
		}

		status = tempfile.write(m_attachment1_body, m_attachment1_body_length);
		if (status < int(m_attachment1_body_length)) 
    {
			if (status >= 0)
				status = MK_MIME_ERROR_WRITING_FILE;
			goto FAIL;
		}

		if (tempfile.failed()) 
      goto FAIL;

		m_plaintext = new nsMsgAttachmentHandler;
		if (!m_plaintext)
			goto FAILMEM;
		m_plaintext->m_mime_delivery_state = this;

    char *tempURL = nsMsgPlatformFileToURL (mHTMLFileSpec->GetCString());
    if (!tempURL || NS_FAILED(nsMsgNewURL(&(m_plaintext->mURL), nsString(tempURL))))
    {
      delete m_plaintext;
      m_plaintext = nsnull;
      goto FAILMEM;
    }
  
    NS_ADDREF(m_plaintext->mURL);
    PR_FREEIF(tempURL);

		PR_FREEIF(m_plaintext->m_type);
		m_plaintext->m_type = PL_strdup(TEXT_HTML);
		PR_FREEIF(m_plaintext->m_charset);
		m_plaintext->m_charset = PL_strdup(mCompFields->GetCharacterSet());
		PR_FREEIF(m_plaintext->m_desired_type);
		m_plaintext->m_desired_type = PL_strdup(TEXT_PLAIN);
		m_attachment_pending_count ++;
		status = m_plaintext->SnarfAttachment(mCompFields);
		if (status < 0)
			goto FAIL;
		if (m_attachment_pending_count > 0)
			return NS_OK;
	}
	  
	/* Kludge to avoid having to allocate memory on the toy computers... */
	buffer = mime_get_stream_write_buffer();
	if (! buffer)
		goto FAILMEM;

	buffer_tail = buffer;

	NS_ASSERTION (m_attachment_pending_count == 0, "m_attachment_pending_count != 0");

#ifdef UNREADY_CODE
	FE_Progress(GetContext(), XP_GetString(MK_MSG_ASSEMBLING_MSG));
#endif

	/* First, open the message file.
	*/
	mTempFileSpec = nsMsgCreateTempFileSpec("nsmail.eml");
	if (! mTempFileSpec)
		goto FAILMEM;

  mOutputFile = new nsOutputFileStream(*mTempFileSpec);
	if (! mOutputFile->is_open()) 
  {
		status = MK_UNABLE_TO_OPEN_TMP_FILE;
    nsMsgDisplayMessageByID(MK_UNABLE_TO_OPEN_TMP_FILE);
		goto FAIL;
	}
  
	if (mCompFields->GetMessageId() == nsnull || *mCompFields->GetMessageId() == 0)
	{
		char * msgID = msg_generate_message_id(mUserIdentity);
		mCompFields->SetMessageId(msgID, nsnull);
		PR_FREEIF(msgID);
	}

	mainbody = new nsMsgSendPart(this, mCompFields->GetCharacterSet());
	if (!mainbody)
		goto FAILMEM;
  
	mainbody->SetMainPart(PR_TRUE);
	mainbody->SetType(m_attachment1_type ? m_attachment1_type : TEXT_PLAIN);

	NS_ASSERTION(mainbody->GetBuffer() == nsnull, "not-null buffer");
	status = mainbody->SetBuffer(m_attachment1_body ? m_attachment1_body : " ");
	if (status < 0)
		goto FAIL;

	/*	### mwelch 
		Determine the encoding of the main message body before we free it.
		The proper way to do this should be to test whatever text is in mainbody
		just before writing it out, but that will require a fix that is less safe
		and takes more memory. */
	PR_FREEIF(m_attachment1_encoding);

  // Check if the part contains 7 bit only. Re-label charset if necessary.
  PRBool body_is_us_ascii;
  if (nsMsgI18Nstateful_charset(mCompFields->GetCharacterSet())) {
    body_is_us_ascii = nsMsgI18N7bit_data_part(mCompFields->GetCharacterSet(), m_attachment1_body, m_attachment1_body_length); 
  }
  else {
    body_is_us_ascii = mime_7bit_data_p (m_attachment1_body, m_attachment1_body_length);
  }

	if (nsMsgI18Nstateful_charset(mCompFields->GetCharacterSet()) ||
		  body_is_us_ascii)
		m_attachment1_encoding = PL_strdup (ENCODING_7BIT);
	else if (mime_use_quoted_printable_p)
		m_attachment1_encoding = PL_strdup (ENCODING_QUOTED_PRINTABLE);
	else
		m_attachment1_encoding = PL_strdup (ENCODING_8BIT);

	PR_FREEIF (m_attachment1_body);

	maincontainer = mainbody;

	// If we were given a pre-saved collection of HTML and contained images,
	// then we want mainbody to point to the HTML lump therein.
	if (m_related_part)
	{
		// If m_related_part is of type text/html, set both maincontainer
		// and mainbody to point to it. If m_related_part is multipart/related,
		// however, set mainbody to be the first child within m_related_part.

		// To plug a memory leak, delete the original maincontainer/mainbody.
		// 
		// NOTE: We DO NOT want to do this to the HTML or multipart/related 
		// nsMsgSendParts because they are deleted at the end of message 
		// delivery by the TapeFileSystem object 
		// (MSG_MimeRelatedSaver / mhtmlstm.cpp).
		delete mainbody;

		// No matter what, maincontainer points to the outermost related part.
		maincontainer = m_related_part;
		maincontainerISrelatedpart = PR_TRUE;

		const char *relPartType = m_related_part->GetType();
		if (relPartType && !strcmp(relPartType, MULTIPART_RELATED))
			// outer shell is m/related,
			// mainbody wants to be the HTML lump within
			mainbody = m_related_part->GetChild(0);
		else
			// outer shell is text/html, 
			// so mainbody wants to be the same lump
			mainbody = maincontainer;

		mainbody->SetMainPart(PR_TRUE);
	}

	if (m_plaintext) 
  {
    //
		// OK.  We have a plaintext version of the main body that we want to
		// send instead of or with the text/html.  Shove it in.
    //
		plainpart = new nsMsgSendPart(this, mCompFields->GetCharacterSet());
		if (!plainpart)
			goto FAILMEM;
		status = plainpart->SetType(TEXT_PLAIN);
		if (status < 0)
			goto FAIL;
		status = plainpart->SetFile(m_plaintext->mFileSpec);
		if (status < 0)
			goto FAIL;
		m_plaintext->AnalyzeSnarfedFile(); // look for 8 bit text, long lines, etc.
		m_plaintext->PickEncoding(mCompFields->GetCharacterSet());
		hdrs = mime_generate_attachment_headers(m_plaintext->m_type,
											  m_plaintext->m_encoding,
											  m_plaintext->m_description,
											  m_plaintext->m_x_mac_type,
											  m_plaintext->m_x_mac_creator,
											  m_plaintext->m_real_name, 0,
											  m_digest_p,
											  m_plaintext,
											  mCompFields->GetCharacterSet());
		if (!hdrs)
			goto FAILMEM;
		status = plainpart->SetOtherHeaders(hdrs);
		PR_Free(hdrs);
		hdrs = nsnull;
		if (status < 0)
			goto FAIL;

		if (mCompFields->GetUseMultipartAlternative()) {
			nsMsgSendPart* htmlpart = maincontainer;
			maincontainer = new nsMsgSendPart(this);
			if (!maincontainer)
				goto FAILMEM;
			status = maincontainer->SetType(MULTIPART_ALTERNATIVE);
			if (status < 0)
				goto FAIL;
			status = maincontainer->AddChild(plainpart);
			if (status < 0)
				goto FAIL;
			status = maincontainer->AddChild(htmlpart);
			if (status < 0)
				goto FAIL;

			// Create the encoder for the plaintext part here,
			// because we aren't the main part (attachment1).
			// (This, along with the rest of the routine, should really
			// be restructured so that no special treatment is given to
			// the main body text that came in. Best to put attachment1_text
			// etc. into a nsMsgSendPart, then reshuffle the parts. Sigh.)
			if (!PL_strcasecmp(m_plaintext->m_encoding, ENCODING_QUOTED_PRINTABLE))
			{
				MimeEncoderData *plaintext_enc = MIME_QPEncoderInit(mime_encoder_output_fn, this);
				if (!plaintext_enc)
				{
					status = MK_OUT_OF_MEMORY;
					goto FAIL;
				}
				plainpart->SetEncoderData(plaintext_enc);
			}
		}
		else {
			delete maincontainer; //### mwelch ??? doesn't this cause a crash?!
			if (maincontainerISrelatedpart)
				m_related_part = nsnull;
			maincontainer = plainpart;
			mainbody = maincontainer;
			PR_FREEIF(m_attachment1_type);
			m_attachment1_type = PL_strdup(TEXT_PLAIN);
			if (!m_attachment1_type)
				goto FAILMEM;

			/* Override attachment1_encoding here. */
			PR_FREEIF(m_attachment1_encoding);
			m_attachment1_encoding = PL_strdup(m_plaintext->m_encoding);

			plaintext_is_mainbody_p = PR_TRUE; // converted plaintext is mainbody
		}
	}

	// ###tw  This used to be this more complicated thing, but for now, if it we
	// have any attachments, we generate multipart.
	// multipart_p = (m_attachment_count > 1 ||
	//				 (m_attachment_count == 1 &&
	//				  m_attachment1_body_length > 0));
	multipart_p = (m_attachment_count > 0);

	if (multipart_p) 
  {
		toppart = new nsMsgSendPart(this);
		if (!toppart)
			goto FAILMEM;
		status = toppart->SetType(m_digest_p ? MULTIPART_DIGEST : MULTIPART_MIXED);
		if (status < 0)
			goto FAIL;
		status = toppart->AddChild(maincontainer);
		if (status < 0)
			goto FAIL;
		if (!m_crypto_closure) 
    {
      status = toppart->SetBuffer(MIME_MULTIPART_BLURB);
			if (status < 0)
				goto FAIL;
		}
	}
	else
		toppart = maincontainer;

  /* Write out the message headers.
   */
	headers = mime_generate_headers (mCompFields,
								   mCompFields->GetCharacterSet(),
								   m_deliver_mode);
	if (!headers)
		goto FAILMEM;

	// ### mwelch
	// If we converted HTML into plaintext, the plaintext part (plainpart)
	// already has its content-type and content-transfer-encoding
	// ("other") headers set. 
	// 
	// In the specific case where such a plaintext part is the 
	// top level message part (iff an HTML message is being sent
	// as text only and no other attachments exist) we want to 
	// preserve the original plainpart headers, since they
	// contain accurate transfer encoding and Mac type/creator 
	// information.
	// 
	// So, in the above case we append the main message headers, 
	// otherwise we overwrite whatever headers may have existed.
	// 
	/* reordering of headers will happen in nsMsgSendPart::Write */
	if ((plainpart) && (plainpart == toppart))
		status = toppart->AppendOtherHeaders(headers);
	else
		status = toppart->SetOtherHeaders(headers);
	PR_Free(headers);
	headers = nsnull;
	if (status < 0)
		goto FAIL;

	/* Set up the first part (user-typed.)  For now, do it even if the first
	* part is empty; we need to add things to skip it if this part is empty.
	* ###tw
	*/


	/* Set up encoder for the first part (message body.)
	*/
	NS_ASSERTION(!m_attachment1_encoder_data, "not-null m_attachment1_encoder_data");
	if (!PL_strcasecmp(m_attachment1_encoding, ENCODING_BASE64))
	{
		m_attachment1_encoder_data = MIME_B64EncoderInit(mime_encoder_output_fn, this);
		if (!m_attachment1_encoder_data) goto FAILMEM;
	}
	else
		if (!PL_strcasecmp(m_attachment1_encoding, ENCODING_QUOTED_PRINTABLE)) {
			m_attachment1_encoder_data =
			MIME_QPEncoderInit(mime_encoder_output_fn, this);
#if 0
			if (!m_attachment1_encoder_data)
				goto FAILMEM;
#endif
		}

	// ### mwelch
	// If we converted HTML into plaintext, the plaintext part
	// already has its type/encoding headers set. So, in the specific
	// case where such a plaintext part is the main message body
	// (iff an HTML message is being sent as text only)
	// we want to avoid generating type/encoding/digest headers;
	// in all other cases, generate such headers here.
	//
	// We really want to set up headers as a dictionary of some sort
	// so that we need not worry about duplicate header lines.
	//
	if ((!plainpart) || (plainpart != mainbody))
	{
		hdrs = mime_generate_attachment_headers (m_attachment1_type,
											   m_attachment1_encoding,
											   0, 0, 0, 0, 0,
											   m_digest_p,
											   nsnull, /* no "ma"! */
											   mCompFields->GetCharacterSet());
		if (!hdrs)
			goto FAILMEM;
		status = mainbody->AppendOtherHeaders(hdrs);
		if (status < 0)
			goto FAIL;
	}

	PR_FREEIF(hdrs);

	status = mainbody->SetEncoderData(m_attachment1_encoder_data);
	m_attachment1_encoder_data = nsnull;
	if (status < 0)
		goto FAIL;


	/* Set up the subsequent parts.
	*/
	if (m_attachment_count > 0)
	{
		/* Kludge to avoid having to allocate memory on the toy computers... */
		if (! mime_mailto_stream_read_buffer)
			mime_mailto_stream_read_buffer = (char *) PR_Malloc (MIME_BUFFER_SIZE);
		buffer = mime_mailto_stream_read_buffer;
		if (! buffer)
			goto FAILMEM;
		buffer_tail = buffer;

		for (i = 0; i < m_attachment_count; i++)
		{
			nsMsgAttachmentHandler *ma = &m_attachments[i];
			hdrs = 0;

			nsMsgSendPart* part = nsnull;

			// If at this point we *still* don't have an content-type, then
			// we're never going to get one.
			if (ma->m_type == nsnull) {
				ma->m_type = PL_strdup(UNKNOWN_CONTENT_TYPE);
				if (ma->m_type == nsnull)
					goto FAILMEM;
			}

			ma->PickEncoding (mCompFields->GetCharacterSet());

			part = new nsMsgSendPart(this);
			if (!part)
				goto FAILMEM;
			status = toppart->AddChild(part);
			if (status < 0)
				goto FAIL;
			status = part->SetType(ma->m_type);
			if (status < 0)
				goto FAIL;

      const char *turl;
      ma->mURL->GetSpec(&turl);
			hdrs = mime_generate_attachment_headers (ma->m_type, ma->m_encoding,
												   ma->m_description,
												   ma->m_x_mac_type,
												   ma->m_x_mac_creator,
												   ma->m_real_name,
												   turl,
												   m_digest_p,
												   ma,
												   mCompFields->GetCharacterSet());
			if (!hdrs)
				goto FAILMEM;

			status = part->SetOtherHeaders(hdrs);
			PR_FREEIF(hdrs);
			if (status < 0)
				goto FAIL;
			status = part->SetFile(ma->mFileSpec);
			if (status < 0)
				goto FAIL;
			if (ma->m_encoder_data) {
				status = part->SetEncoderData(ma->m_encoder_data);
				if (status < 0)
					goto FAIL;
				ma->m_encoder_data = nsnull;
			}

			ma->m_current_column = 0;

			if (ma->m_type &&
					(!PL_strcasecmp (ma->m_type, MESSAGE_RFC822) ||
					!PL_strcasecmp (ma->m_type, MESSAGE_NEWS))) {
				status = part->SetStripSensitiveHeaders(PR_TRUE);
				if (status < 0)
					goto FAIL;
			}
		}
	}

	// OK, now actually write the structure we've carefully built up.
	status = toppart->Write();
	if (status < 0)
		goto FAIL;

	HJ45609

	if (mOutputFile) {
		/* If we don't do this check...ZERO length files can be sent */
		if (mOutputFile->failed()) {
			status = MK_MIME_ERROR_WRITING_FILE;
			goto FAIL;
		}

    mOutputFile->close();
		delete mOutputFile;
	}
	mOutputFile = nsnull;

#ifdef UNREADY_CODE
	FE_Progress(GetContext(), XP_GetString(MK_MSG_ASSEMB_DONE_MSG));
#endif

  if (m_dont_deliver_p && mListenerArrayCount > 0)
	{
    //
		// Need to ditch the file spec here so that we don't delete the
		// file, since in this case, the caller wants the file
    //
    NS_NewFileSpecWithSpec(*mTempFileSpec, &mReturnFileSpec);
    delete mTempFileSpec;
    mTempFileSpec = nsnull;
	  if (!mReturnFileSpec)
      NotifyListenersOnStopSending(nsnull, NS_ERROR_FAILURE, nsnull, nsnull);
    else
      NotifyListenersOnStopSending(nsnull, NS_OK, nsnull, mReturnFileSpec);

		// SHERRY: for now, we are going to let the destructor do the Clear();
	}
	else 
  {
		DeliverMessage();
		shouldDeleteDeliveryState = PR_FALSE;
	}

FAIL:
	if (toppart)
		delete toppart;
	toppart = nsnull;
	mainbody = nsnull;
	maincontainer = nsnull;

	PR_FREEIF(headers);
	PR_FREEIF(separator);
	if (in_file) {
		PR_Close (in_file);
		in_file = nsnull;
	}

	if (shouldDeleteDeliveryState)
	{
		if (status < 0) 
    {
			m_status = status;
			Fail (status, error_msg);
      PR_FREEIF(error_msg);
		}
	}

	return status;

FAILMEM:
	status = MK_OUT_OF_MEMORY;
	goto FAIL;
}


#if defined(XP_MAC) && defined(DEBUG)
// Compiler runs out of registers for the debug build.
#pragma global_optimizer on
#pragma optimization_level 4
#endif // XP_MAC && DEBUG

# define FROB(X) \
	  if (X && *X) \
		{ \
		  if (*recipients) PL_strcat(recipients, ","); \
		  PL_strcat(recipients, X); \
	  }

HJ91531

#if defined(XP_MAC) && defined(DEBUG)
#pragma global_optimizer reset
#endif // XP_MAC && DEBUG


int
mime_write_message_body (nsMsgComposeAndSend *state,
						 char *buf, PRInt32 size)
{
  HJ62011

  if (PRInt32(state->mOutputFile->write(buf, size)) < size) 
  {
    return MK_MIME_ERROR_WRITING_FILE;
  } 
  else 
  {
    return 0;
  }
}

int
mime_encoder_output_fn (const char *buf, PRInt32 size, void *closure)
{
  nsMsgComposeAndSend *state = (nsMsgComposeAndSend *) closure;
  return mime_write_message_body (state, (char *) buf, size);
}

int nsMsgComposeAndSend::HackAttachments(
						  const struct nsMsgAttachmentData *attachments,
						  const struct nsMsgAttachedFile *preloaded_attachments)
{
  MWContext *x = NULL;
	INTL_CharSetInfo c = LO_GetDocumentCharacterSetInfo(x);
  if (preloaded_attachments)
		NS_ASSERTION(!attachments, "not-null attachments");
	if (attachments)
		NS_ASSERTION(!preloaded_attachments, "not-null preloaded attachments");

  if (preloaded_attachments && preloaded_attachments[0].orig_url) 
  {
		/* These are attachments which have already been downloaded to tmp files.
		We merely need to point the internal attachment data at those tmp
		files.
		*/
		PRInt32 i;

		m_pre_snarfed_attachments_p = PR_TRUE;

		m_attachment_count = 0;
		while (preloaded_attachments[m_attachment_count].orig_url)
			m_attachment_count++;

    //
    // Create the class to deal with attachments...
    //
		m_attachments = (nsMsgAttachmentHandler *) new nsMsgAttachmentHandler[m_attachment_count];
		if (! m_attachments)
			return MK_OUT_OF_MEMORY;

		for (i = 0; i < m_attachment_count; i++) 
    {
			m_attachments[i].m_mime_delivery_state = this;

			/* These attachments are already "snarfed". */
			m_attachments[i].m_done = PR_TRUE;
			NS_ASSERTION (preloaded_attachments[i].orig_url, "null url");


      if (m_attachments[i].mURL)
        NS_RELEASE(m_attachments[i].mURL);
			m_attachments[i].mURL = preloaded_attachments[i].orig_url;

			PR_FREEIF(m_attachments[i].m_type);
			m_attachments[i].m_type = PL_strdup (preloaded_attachments[i].type);
			PR_FREEIF(m_attachments[i].m_charset);
			m_attachments[i].m_charset = PL_strdup (mCompFields->GetCharacterSet());
			PR_FREEIF(m_attachments[i].m_description);
			m_attachments[i].m_description = PL_strdup (preloaded_attachments[i].description);
			PR_FREEIF(m_attachments[i].m_real_name);
			m_attachments[i].m_real_name = PL_strdup (preloaded_attachments[i].real_name);
			PR_FREEIF(m_attachments[i].m_x_mac_type);
			m_attachments[i].m_x_mac_type = PL_strdup (preloaded_attachments[i].x_mac_type);
			PR_FREEIF(m_attachments[i].m_x_mac_creator);
			m_attachments[i].m_x_mac_creator = PL_strdup (preloaded_attachments[i].x_mac_creator);
			PR_FREEIF(m_attachments[i].m_encoding);
			m_attachments[i].m_encoding = PL_strdup (preloaded_attachments[i].encoding);

      if (m_attachments[i].mFileSpec)
        delete (m_attachments[i].mFileSpec);
			m_attachments[i].mFileSpec = new nsFileSpec(*(preloaded_attachments[i].file_spec));

			m_attachments[i].m_size = preloaded_attachments[i].size;
			m_attachments[i].m_unprintable_count =
			preloaded_attachments[i].unprintable_count;
			m_attachments[i].m_highbit_count =
			preloaded_attachments[i].highbit_count;
			m_attachments[i].m_ctl_count = preloaded_attachments[i].ctl_count;
			m_attachments[i].m_null_count =
			preloaded_attachments[i].null_count;
			m_attachments[i].m_max_column =
			preloaded_attachments[i].max_line_length;

			/* If the attachment has an encoding, and it's not one of
			the "null" encodings, then keep it. */
			if (m_attachments[i].m_encoding &&
					PL_strcasecmp (m_attachments[i].m_encoding, ENCODING_7BIT) &&
					PL_strcasecmp (m_attachments[i].m_encoding, ENCODING_8BIT) &&
					PL_strcasecmp (m_attachments[i].m_encoding, ENCODING_BINARY))
				m_attachments[i].m_already_encoded_p = PR_TRUE;

			msg_pick_real_name(&m_attachments[i], mCompFields->GetCharacterSet());
		}
	}
	else 
		if (attachments && attachments[0].url) {
		/* These are attachments which have already been downloaded to tmp files.
		We merely need to point the internal attachment data at those tmp
		files.  We will delete the tmp files as we attach them.
		*/
		PRInt32 i;
		int mailbox_count = 0, news_count = 0;

		m_attachment_count = 0;
		while (attachments[m_attachment_count].url)
			m_attachment_count++;

		m_attachments = (nsMsgAttachmentHandler *)
		new nsMsgAttachmentHandler[m_attachment_count];

		if (! m_attachments)
			return MK_OUT_OF_MEMORY;

		for (i = 0; i < m_attachment_count; i++) {
			m_attachments[i].m_mime_delivery_state = this;
			NS_ASSERTION (attachments[i].url, "null url");

      if (m_attachments[i].mURL)
        NS_RELEASE(m_attachments[i].mURL);
      m_attachments[i].mURL = attachments[i].url;

			PR_FREEIF(m_attachments[i].m_override_type);
			m_attachments[i].m_override_type = PL_strdup (attachments[i].real_type);
			PR_FREEIF(m_attachments[i].m_charset);
			m_attachments[i].m_charset = PL_strdup (mCompFields->GetCharacterSet());
			PR_FREEIF(m_attachments[i].m_override_encoding);
			m_attachments[i].m_override_encoding = PL_strdup (attachments[i].real_encoding);
			PR_FREEIF(m_attachments[i].m_desired_type);
			m_attachments[i].m_desired_type = PL_strdup (attachments[i].desired_type);
			PR_FREEIF(m_attachments[i].m_description);
			m_attachments[i].m_description = PL_strdup (attachments[i].description);
			PR_FREEIF(m_attachments[i].m_real_name);
			m_attachments[i].m_real_name = PL_strdup (attachments[i].real_name);
			PR_FREEIF(m_attachments[i].m_x_mac_type);
			m_attachments[i].m_x_mac_type = PL_strdup (attachments[i].x_mac_type);
			PR_FREEIF(m_attachments[i].m_x_mac_creator);
			m_attachments[i].m_x_mac_creator = PL_strdup (attachments[i].x_mac_creator);
			PR_FREEIF(m_attachments[i].m_encoding);
			m_attachments[i].m_encoding = PL_strdup ("7bit");

			// real name is set in the case of vcard so don't change it.
			// m_attachments[i].m_real_name = 0;

			/* Count up attachments which are going to come from mail folders
			and from NNTP servers. */
      const char *turl;
      m_attachments[i].mURL->GetSpec(&turl);
			if (PL_strncasecmp(turl, "mailbox:",8) ||
					PL_strncasecmp(turl, "IMAP:",5))
				mailbox_count++;
			else
				if (PL_strncasecmp(turl, "news:",5) ||
						PL_strncasecmp(turl, "snews:",6))
					news_count++;

			msg_pick_real_name(&m_attachments[i], mCompFields->GetCharacterSet());
		}

		/* If there is more than one mailbox URL, or more than one NNTP url,
		do the load in serial rather than parallel, for efficiency.
		*/
		if (mailbox_count > 1 || news_count > 1)
			m_be_synchronous_p = PR_TRUE;

		m_attachment_pending_count = m_attachment_count;

		/* Start the URL attachments loading (eventually, an exit routine will
		call the done_callback). */

#ifdef UNREADY_CODE
		if (m_attachment_count == 1)
			FE_Progress(GetContext(), XP_GetString(MK_MSG_LOAD_ATTACHMNT));
		else
			FE_Progress(GetContext(), XP_GetString(MK_MSG_LOAD_ATTACHMNTS));
#endif

		for (i = 0; i < m_attachment_count; i++) {
			/* This only returns a failure code if NET_GetURL was not called
			(and thus no exit routine was or will be called.) */
			int status = m_attachments [i].SnarfAttachment(mCompFields);
			if (status < 0)
				return status;

			if (m_be_synchronous_p)
				break;
		}
	}

	if (m_attachment_pending_count <= 0)
		/* No attachments - finish now (this will call the done_callback). */
		GatherMimeAttachments();

	return 0;
}

int nsMsgComposeAndSend::SetMimeHeader(MSG_HEADER_SET header, const char *value)
{
	char * dupHeader = nsnull;
	PRInt32	ret = MK_OUT_OF_MEMORY;

	if (header & (MSG_FROM_HEADER_MASK | MSG_TO_HEADER_MASK | MSG_REPLY_TO_HEADER_MASK | MSG_CC_HEADER_MASK | MSG_BCC_HEADER_MASK))
		dupHeader = mime_fix_addr_header(value);
	else if (header &  (MSG_NEWSGROUPS_HEADER_MASK| MSG_FOLLOWUP_TO_HEADER_MASK))
		dupHeader = mime_fix_news_header(value);
	else  if (header & (MSG_FCC_HEADER_MASK | MSG_ORGANIZATION_HEADER_MASK |  MSG_SUBJECT_HEADER_MASK | MSG_REFERENCES_HEADER_MASK | MSG_X_TEMPLATE_HEADER_MASK))
		dupHeader = mime_fix_header(value);
	else
		NS_ASSERTION(PR_FALSE, "invalid header");	// unhandled header mask - bad boy.

	if (dupHeader) 
  {
		mCompFields->SetHeader(header, dupHeader, &ret);
		PR_Free(dupHeader);
	}
	return ret;
}

nsresult
nsMsgComposeAndSend::InitCompositionFields(nsMsgCompFields *fields)
{
  nsresult        rv = NS_OK;
	const char      *pStr = nsnull;

	if (mCompFields)
		mCompFields->Release();

	mCompFields = new nsMsgCompFields;
	if (mCompFields)
		mCompFields->AddRef();
	else
		return MK_OUT_OF_MEMORY;

  char *cset = nsnull;
  nsresult lrv = fields->GetCharacterSet(&cset);
  // Make sure charset is sane...
  if (NS_FAILED(lrv) || !cset || !*cset)
  {
    mCompFields->SetCharacterSet("us-ascii", nsnull);
  }
  else
  {
    mCompFields->SetCharacterSet(fields->GetCharacterSet(), nsnull);
  }

	pStr = fields->GetMessageId();
	if (pStr)
	{
		mCompFields->SetMessageId((char *) pStr, nsnull);
		/* Don't bother checking for out of memory; if it fails, then we'll just
		   let the server generate the message-id, and suffer with the
		   possibility of duplicate messages.*/
	}

	pStr = fields->GetNewspostUrl();
	if (!pStr || !*pStr) 
  {
		HJ41792
	}

  
  // Now, we will look for a URI defined as the default FCC pref. If this is set,
  // then SetFcc will use this value. The FCC field is a URI for the server that 
  // will hold the "Sent" folder...the 
  //
  // First, look at what was passed in via the "fields" structure...if that was
  // set then use it, otherwise, fall back to what is set in the prefs...
  //
  const char *fieldsFCC = fields->GetFcc();
  if (fieldsFCC && *fieldsFCC)
  {
    if (PL_strcasecmp(fieldsFCC, "nocopy://") == 0)
      mCompFields->SetFcc("", nsnull);
    else
      mCompFields->SetFcc(fieldsFCC, nsnull);
  }
  else
  {
    char *uri = "mailbox://";
    NS_WITH_SERVICE(nsIPref, prefs, kPrefCID, &rv); 
    if (NS_SUCCEEDED(rv) && prefs) 
      rv = prefs->CopyCharPref("mail.default_fcc_server", &uri);

    if ( (uri) || (*uri) )
    {
      if (PL_strcasecmp(uri, "nocopy://") == 0)
        mCompFields->SetFcc("", nsnull);
      else
        mCompFields->SetFcc(uri, nsnull);
    }
    else
      mCompFields->SetFcc("", nsnull);
  }

	mCompFields->SetNewspostUrl((char *) fields->GetNewspostUrl(), nsnull);
	mCompFields->SetDefaultBody((char *) fields->GetDefaultBody(), nsnull);

	/* strip whitespace from and duplicate header fields. */
	SetMimeHeader(MSG_FROM_HEADER_MASK, fields->GetFrom());
	SetMimeHeader(MSG_REPLY_TO_HEADER_MASK, fields->GetReplyTo());
	SetMimeHeader(MSG_TO_HEADER_MASK, fields->GetTo());
	SetMimeHeader(MSG_CC_HEADER_MASK, fields->GetCc());
	SetMimeHeader(MSG_FCC_HEADER_MASK, fields->GetFcc());
	SetMimeHeader(MSG_BCC_HEADER_MASK, fields->GetBcc());
	SetMimeHeader(MSG_NEWSGROUPS_HEADER_MASK, fields->GetNewsgroups());
	SetMimeHeader(MSG_FOLLOWUP_TO_HEADER_MASK, fields->GetFollowupTo());
	SetMimeHeader(MSG_ORGANIZATION_HEADER_MASK, fields->GetOrganization());
	SetMimeHeader(MSG_SUBJECT_HEADER_MASK, fields->GetSubject());
	SetMimeHeader(MSG_REFERENCES_HEADER_MASK, fields->GetReferences());
	SetMimeHeader(MSG_X_TEMPLATE_HEADER_MASK, fields->GetTemplateName());

	pStr = fields->GetOtherRandomHeaders();
	if (pStr)
		mCompFields->SetOtherRandomHeaders((char *) pStr, nsnull);

	pStr = fields->GetPriority();
	if (pStr)
		mCompFields->SetPriority((char *) pStr, nsnull);

	int i, j = (int) MSG_LAST_BOOL_HEADER_MASK;
	for (i = 0; i < j; i++) 
  {
		mCompFields->SetBoolHeader((MSG_BOOL_HEADER_SET) i,
		fields->GetBoolHeader((MSG_BOOL_HEADER_SET) i), nsnull);
	}

	mCompFields->SetForcePlainText(fields->GetForcePlainText());
	mCompFields->SetUseMultipartAlternative(fields->GetUseMultipartAlternative());

	//
  // Check the fields for legitimacy...
  //
	if ( m_deliver_mode != nsMsgSaveAsDraft && m_deliver_mode != nsMsgSaveAsTemplate ) 
  {
		rv = mime_sanity_check_fields (mCompFields->GetFrom(), mCompFields->GetReplyTo(),
										mCompFields->GetTo(), mCompFields->GetCc(),
										mCompFields->GetBcc(), mCompFields->GetFcc(),
										mCompFields->GetNewsgroups(), mCompFields->GetFollowupTo(),
										mCompFields->GetSubject(), mCompFields->GetReferences(),
										mCompFields->GetOrganization(),
										mCompFields->GetOtherRandomHeaders());
	}

	return rv;
}

nsresult
nsMsgComposeAndSend::Init(
              nsIMsgIdentity  *aUserIdentity,
						  nsMsgCompFields *fields,
              nsFileSpec      *sendFileSpec,
						  PRBool digest_p,
						  PRBool dont_deliver_p,
						  nsMsgDeliverMode mode,
              nsIMessage      *msgToReplace,
						  const char *attachment1_type,
						  const char *attachment1_body,
						  PRUint32 attachment1_body_length,
						  const struct nsMsgAttachmentData *attachments,
						  const struct nsMsgAttachedFile *preloaded_attachments,
						  nsMsgSendPart *relatedPart)
{
	nsresult      rv = NS_OK;

  // 
  // The Init() method should initialize a send operation for full
  // blown create and send operations as well as just the "send a file"
  // operations. 
  //
	m_dont_deliver_p = dont_deliver_p;
	m_deliver_mode = mode;
  mMsgToReplace = msgToReplace;

  mUserIdentity = aUserIdentity;
  NS_ASSERTION(mUserIdentity, "Got null identity!\n");
  if (!mUserIdentity) return NS_ERROR_UNEXPECTED;

  //
  // First sanity check the composition fields parameter and
  // see if we should continue
  //
	if (!fields)
		return MK_OUT_OF_MEMORY;

  rv = InitCompositionFields(fields);
  if (NS_FAILED(rv))
    return rv;

  //
  // At this point, if we are only creating this object to do
  // send operations on externally created RFC822 disk files, 
  // make sure we have setup the appropriate nsFileSpec and
  // move on with life.
  //
  //
  // First check to see if we are doing a send operation on an external file
  // or creating the file itself.
  //
  if (sendFileSpec)
  {
  	mTempFileSpec = sendFileSpec;
    return NS_OK;
  }

  // Setup related part information
	m_related_part = relatedPart;
	if (m_related_part)
		m_related_part->SetMimeDeliveryState(this);

  //
  // Needed for mime encoding!
  //
  PRBool strictly_mime = PR_FALSE; 
  NS_WITH_SERVICE(nsIPref, prefs, kPrefCID, &rv); 
  if (NS_SUCCEEDED(rv) && prefs) 
  {
    rv = prefs->GetBoolPref("mail.strictly_mime", &strictly_mime);
  }

  nsMsgMIMESetConformToStandard(strictly_mime);
	mime_use_quoted_printable_p = strictly_mime;

	/* Strip whitespace from beginning and end of body. */
	if (attachment1_body)
	{
		/* BUG 115202 -- we used to strip out whitespaces from
		   beginning of body. We are not doing that anymore. */
		while (attachment1_body_length > 0 &&
			IS_SPACE (attachment1_body [attachment1_body_length - 1]))
		{
			attachment1_body_length--;
		}
		if (attachment1_body_length <= 0)
			attachment1_body = 0;

		if (attachment1_body)
		{
			char *newb = (char *) PR_Malloc (attachment1_body_length + 1);
			if (! newb)
				return MK_OUT_OF_MEMORY;
      nsCRT::memcpy (newb, attachment1_body, attachment1_body_length);
			newb [attachment1_body_length] = 0;
			m_attachment1_body = newb;
			m_attachment1_body_length = attachment1_body_length;
		}
	}

	PR_FREEIF(m_attachment1_type);
	m_attachment1_type = PL_strdup (attachment1_type);
	PR_FREEIF(m_attachment1_encoding);
	m_attachment1_encoding = PL_strdup ("8bit");
  m_digest_p = digest_p;

	return HackAttachments(attachments, preloaded_attachments);
}

nsresult
MailDeliveryCallback(nsIURI *aUrl, nsresult aExitCode, void *tagData)
{
  if (tagData)
  {
    nsMsgComposeAndSend *ptr = (nsMsgComposeAndSend *) tagData;
    ptr->DeliverAsMailExit(aUrl, aExitCode);
    NS_RELEASE(ptr);
  }

  return aExitCode;
}

nsresult
NewsDeliveryCallback(nsIURI *aUrl, nsresult aExitCode, void *tagData)
{
  if (tagData)
  {
    nsMsgComposeAndSend *ptr = (nsMsgComposeAndSend *) tagData;
    ptr->DeliverAsNewsExit(aUrl, aExitCode);
    NS_RELEASE(ptr);
  }

  return aExitCode;
}

HJ70669

nsresult
nsMsgComposeAndSend::DeliverMessage()
{
	PRBool mail_p = ((mCompFields->GetTo() && *mCompFields->GetTo()) || 
					(mCompFields->GetCc() && *mCompFields->GetCc()) || 
					(mCompFields->GetBcc() && *mCompFields->GetBcc()));
	PRBool news_p = (mCompFields->GetNewsgroups() && 
				    *(mCompFields->GetNewsgroups()) ? PR_TRUE : PR_FALSE);

	if ( m_deliver_mode != nsMsgSaveAsDraft &&
			m_deliver_mode != nsMsgSaveAsTemplate )
		NS_ASSERTION(mail_p || news_p, "message without destination");

	if (m_deliver_mode == nsMsgQueueForLater) 
  {
		return QueueForLater();
	}
	else if (m_deliver_mode == nsMsgSaveAsDraft) 
  {
		return SaveAsDraft();
	}
	else if (m_deliver_mode == nsMsgSaveAsTemplate) 
  {
		return SaveAsTemplate();
	}

	if (news_p)
		DeliverFileAsNews();   /* May call ...as_mail() next. */
	else if (mail_p)
		DeliverFileAsMail();
	else
		abort();

  return NS_OK;
}

nsresult
nsMsgComposeAndSend::DeliverFileAsMail()
{
	char *buf, *buf2;
	buf = (char *) PR_Malloc ((mCompFields->GetTo() ? PL_strlen (mCompFields->GetTo())  + 10 : 0) +
						   (mCompFields->GetCc() ? PL_strlen (mCompFields->GetCc())  + 10 : 0) +
						   (mCompFields->GetBcc() ? PL_strlen (mCompFields->GetBcc()) + 10 : 0) +
						   10);
	if (!buf) 
  {
    // RICHIE_TODO: message loss here
    char    *eMsg = ComposeBEGetStringByID(MK_OUT_OF_MEMORY);
    Fail(MK_OUT_OF_MEMORY, eMsg);
    NotifyListenersOnStopSending(nsnull, MK_OUT_OF_MEMORY, nsnull, nsnull);
    PR_FREEIF(eMsg);
    return MK_OUT_OF_MEMORY;
	}

	PL_strcpy (buf, "");
	buf2 = buf + PL_strlen (buf);
	if (mCompFields->GetTo() && *mCompFields->GetTo())
		PL_strcat (buf2, mCompFields->GetTo());
	if (mCompFields->GetCc() && *mCompFields->GetCc()) {
		if (*buf2) PL_strcat (buf2, ",");
			PL_strcat (buf2, mCompFields->GetCc());
	}
	if (mCompFields->GetBcc() && *mCompFields->GetBcc()) {
		if (*buf2) PL_strcat (buf2, ",");
			PL_strcat (buf2, mCompFields->GetBcc());
	}

  nsresult rv = NS_OK;
  NS_WITH_SERVICE(nsISmtpService, smtpService, kSmtpServiceCID, &rv);
  if (NS_SUCCEEDED(rv) && smtpService)
  {
  	NS_ADDREF(this);
    nsMsgDeliveryListener *sendListener = new nsMsgDeliveryListener(MailDeliveryCallback, nsMailDelivery, this);
    if (!sendListener)
    {
      // RICHIE_TODO - message loss here?
      nsMsgDisplayMessageByString("Unable to create SMTP listener service. Send failed.");
      return MK_OUT_OF_MEMORY;
    }

    sendListener->SetMsgComposeAndSendObject(this);

    nsFilePath    filePath(*mTempFileSpec);
    rv = smtpService->SendMailMessage(filePath, buf, sendListener, nsnull);
  }
  
  PR_FREEIF(buf); // free the buf because we are done with it....
  return rv;
}

nsresult
nsMsgComposeAndSend::DeliverFileAsNews()
{
  if (mCompFields->GetNewsgroups() == nsnull) 
    return NS_OK;

  nsresult rv = NS_OK;
  NS_WITH_SERVICE(nsINntpService, nntpService, kNntpServiceCID, &rv);

  if (NS_SUCCEEDED(rv) && nntpService) 
  {	
  	NS_ADDREF(this);
    nsMsgDeliveryListener *sendListener = new nsMsgDeliveryListener(NewsDeliveryCallback, nsNewsDelivery, this);
    if (!sendListener)
    {
      // RICHIE_TODO - message loss here?
      nsMsgDisplayMessageByString("Unable to create NNTP listener service. News Delivery failed.");
      return MK_OUT_OF_MEMORY;
    }

    sendListener->SetMsgComposeAndSendObject(this);
    nsFilePath    filePath (*mTempFileSpec);

    rv = nntpService->PostMessage(filePath, mCompFields->GetNewsgroups(), sendListener, nsnull);
  }

  return rv;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
// RICHIE CLASS INIT STUFF
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
//

/* the following macro actually implement addref, release and query interface for our component. */
NS_IMPL_ISUPPORTS(nsMsgComposeAndSend, nsCOMTypeInfo<nsIMsgSend>::GetIID());

void 
nsMsgComposeAndSend::Fail(nsresult failure_code, char *error_msg)
{
  if (NS_FAILED(failure_code))
  {
    if (!error_msg)
      nsMsgDisplayMessageByID(failure_code);
    else
      nsMsgDisplayMessageByString(error_msg);
  }

  if (m_attachments_done_callback)
	{
	  /* mime_free_message_state will take care of cleaning up the
		   attachment files and attachment structures */
	  m_attachments_done_callback (failure_code, error_msg, nsnull);
    m_attachments_done_callback = nsnull;
	}
  
  // SHERRY: for now, we are going to let the destructor do the Clear();
}

void
nsMsgComposeAndSend::DoDeliveryExitProcessing(nsresult aExitCode, PRBool aCheckForMail)
{
  // If we fail on the news delivery, not sense in going on so just notify
  // the user and exit.
  if (NS_FAILED(aExitCode))
  {
#ifdef NS_DEBUG
  printf("\nMessage Delivery Failed!\n");
#endif

    // RICHIE_TODO - message lost here!
    char    *eMsg = ComposeBEGetStringByID(aExitCode);
    
    Fail(aExitCode, eMsg);
    NotifyListenersOnStopSending(nsnull, aExitCode, nsnull, nsnull);
    PR_FREEIF(eMsg);
    return;
  }
#ifdef NS_DEBUG
  else
    printf("\nMessage Delivery SUCCEEDED!\n");
#endif

  
  if (aCheckForMail)
  {
    if ((mCompFields->GetTo() && *mCompFields->GetTo()) || 
		    (mCompFields->GetCc() && *mCompFields->GetCc()) || 
        (mCompFields->GetBcc() && *mCompFields->GetBcc()))
    {
      // If we're sending this news message to mail as well, start it now.
      // Completion and further errors will be handled there.
      DeliverFileAsMail();
      return;
    }
  }

  //
  // Tell the listeners that we are done with the sending operation...
  //
  NotifyListenersOnStopSending(nsnull, aExitCode, nsnull, nsnull);

  // If we hit here, we are done with delivery!
  //
  // Just call the DoFCC() method and if that fails, then we should just 
  // cleanup and get out. If DoFCC "succeeds", then all that means is the
  // async copy operation has been started and we will be notified later 
  // when it is done. DON'T cleanup until the copy is complete and don't
  // notify the listeners with OnStop() until we are done.
  //
  // For now, we don't need to do anything here, but the code will stay this 
  // way until later...
  //
  nsresult retCode = DoFcc();
  if (NS_FAILED(retCode))
  {
#ifdef NS_DEBUG
  printf("\nDoDeliveryExitProcessing(): DoFcc() call Failed!\n");
#endif
    // Everything already cleaned up...just return.
    return;
  } 
  else
  {
    // Either we started the copy...cleanup happens later...or cleanup was
    // already taken care of for us.
    return;
  }
}

void
nsMsgComposeAndSend::DeliverAsMailExit(nsIURI *aUrl, nsresult aExitCode)
{
  DoDeliveryExitProcessing(aExitCode, PR_FALSE);
  return;
}

void
nsMsgComposeAndSend::DeliverAsNewsExit(nsIURI *aUrl, nsresult aExitCode)
{
  DoDeliveryExitProcessing(aExitCode, PR_FALSE);
  return;
}

// 
// Now, start the appropriate copy operation.
//
nsresult
nsMsgComposeAndSend::DoFcc()
{
  //
  // Just cleanup and return success if no FCC is necessary
  //
  if (!mCompFields->GetFcc() || !*mCompFields->GetFcc())
  {
#ifdef NS_DEBUG
  printf("\nCopy operation disabled by user!\n");
#endif

    NotifyListenersOnStopSending(nsnull, NS_OK, nsnull, nsnull);
    // SHERRY: for now, we are going to let the destructor do the Clear();
    return NS_OK;
  }

  //
  // If we are here, then we need to save off the FCC file to save and
  // start the copy operation. MimeDoFCC() will take care of all of this
  // for us.
  //
  nsresult rv = MimeDoFCC(mTempFileSpec,
                          nsMsgDeliverNow,
                          mCompFields->GetBcc(),
							            mCompFields->GetFcc(), 
                          mCompFields->GetNewspostUrl());
  if (NS_FAILED(rv))
  {
    //
    // If we hit here, the copy operation FAILED and we should at least tell the
    // user that it did fail but the send operation has already succeeded.
    //
    char    *eMsg = ComposeBEGetStringByID(rv);
    Fail(rv, eMsg);
    NotifyListenersOnStopCopy(rv, nsnull);
    PR_FREEIF(eMsg);
  }

  return rv;
}

void 
nsMsgComposeAndSend::Clear()
{
#ifdef NS_DEBUG
  printf("\nTHE CLEANUP ROUTINE FOR nsMsgComposeAndSend() WAS CALLED\n");
#endif
	PR_FREEIF (m_attachment1_type);
	PR_FREEIF (m_attachment1_encoding);
	PR_FREEIF (m_attachment1_body);

	if (mCompFields) 
  {
		mCompFields->Release();
		mCompFields = nsnull;
	}

	if (m_attachment1_encoder_data) 
  {
		MIME_EncoderDestroy(m_attachment1_encoder_data, PR_TRUE);
		m_attachment1_encoder_data = 0;
	}

	if (m_plaintext) 
  {
		if (m_plaintext->mOutFile)
			m_plaintext->mOutFile->close();

		if (m_plaintext->mFileSpec)
		  delete m_plaintext->mFileSpec;
		delete m_plaintext;
		m_plaintext = nsnull;
	}

	if (mHTMLFileSpec) 
  {
		mHTMLFileSpec->Delete(PR_FALSE);
		delete mHTMLFileSpec;
		mHTMLFileSpec= nsnull;
	}

	if (mOutputFile) 
  {
		delete mOutputFile;
		mOutputFile = 0;
	}

  if (mCopyFileSpec)
  {
    nsFileSpec aFileSpec;
    mCopyFileSpec->GetFileSpec(&aFileSpec);
    if (aFileSpec.Valid())
      aFileSpec.Delete(PR_FALSE);

    delete mCopyFileSpec;
    mCopyFileSpec = nsnull;
  }

	if (mTempFileSpec) 
  {
    if (mReturnFileSpec == nsnull)
    {
      mTempFileSpec->Delete(PR_FALSE);
      delete mTempFileSpec;
	    mTempFileSpec = nsnull;
    }
	}

	HJ82388

	if (m_attachments)
	{
		int i;
		for (i = 0; i < m_attachment_count; i++) 
    {
			if (m_attachments [i].m_encoder_data) 
      {
				MIME_EncoderDestroy(m_attachments[i].m_encoder_data, PR_TRUE);
				m_attachments [i].m_encoder_data = 0;
			}

			if (m_attachments[i].mURL)
      {
        NS_RELEASE(m_attachments[i].mURL);
        m_attachments[i].mURL = nsnull;
      }

			PR_FREEIF (m_attachments [i].m_type);
			PR_FREEIF (m_attachments [i].m_override_type);
			PR_FREEIF (m_attachments [i].m_override_encoding);
			PR_FREEIF (m_attachments [i].m_desired_type);
			PR_FREEIF (m_attachments [i].m_description);
			PR_FREEIF (m_attachments [i].m_x_mac_type);
			PR_FREEIF (m_attachments [i].m_x_mac_creator);
			PR_FREEIF (m_attachments [i].m_real_name);
			PR_FREEIF (m_attachments [i].m_encoding);
			if (m_attachments [i].mOutFile)
          m_attachments[i].mOutFile->close();
			if (m_attachments[i].mFileSpec) 
      {
				m_attachments[i].mFileSpec->Delete(PR_FALSE);
				delete m_attachments[i].mFileSpec;
        m_attachments[i].mFileSpec = nsnull;
			}

#ifdef XP_MAC
		  //
      // remove the appledoubled intermediate file after we done all.
		  //
			if (m_attachments[i].mAppleFileSpec) 
      {
				m_attachments[i].mAppleFileSpec->Delete(PR_FALSE);
				delete m_attachments[i].mAppleFileSpec;
			}
#endif /* XP_MAC */
		}

		delete[] m_attachments;
		m_attachment_count = m_attachment_pending_count = 0;
		m_attachments = 0;
	}

  // Cleanup listener array...
  DeleteListeners();

  if (mCopyObj)
    delete mCopyObj;
}

nsMsgComposeAndSend::nsMsgComposeAndSend()
{
	mCompFields = nsnull;			/* Where to send the message once it's done */
  mListenerArray = nsnull;
  mListenerArrayCount = 0;

	mOutputFile = nsnull;

	m_dont_deliver_p = PR_FALSE;
	m_deliver_mode = nsMsgDeliverNow;

	m_attachments_only_p = PR_FALSE;
	m_pre_snarfed_attachments_p = PR_FALSE;
	m_digest_p = PR_FALSE;
	m_be_synchronous_p = PR_FALSE;
	m_crypto_closure = nsnull;
	m_attachment1_type = 0;
	m_attachment1_encoding = 0;
	m_attachment1_encoder_data = nsnull;
	m_attachment1_body = 0;
	m_attachment1_body_length = 0;
	m_attachment_count = 0;
	m_attachment_pending_count = 0;
	m_attachments = nsnull;
	m_status = 0;
	m_attachments_done_callback = nsnull;
	m_plaintext = nsnull;
	m_related_part = nsnull;
	m_pane = nsnull;		/* Pane to use when loading the URLs */

  // These are for temp file creation and return
  mReturnFileSpec = nsnull;
  mTempFileSpec = nsnull;
	mHTMLFileSpec = nsnull;
  mCopyFileSpec = nsnull;
  mCopyObj = nsnull;

	NS_INIT_REFCNT();
}

nsMsgComposeAndSend::~nsMsgComposeAndSend()
{
	Clear();
}

nsresult
nsMsgComposeAndSend::SetListenerArray(nsIMsgSendListener **aListenerArray)
{
  nsIMsgSendListener **ptr = aListenerArray;

  if ( (!aListenerArray) || (!*aListenerArray) )
    return NS_OK;

  // First, count the listeners passed in...
  mListenerArrayCount = 0;
  while (*ptr != nsnull)
  {
    mListenerArrayCount++;
    ++ptr;
  }

  // now allocate an array to hold the number of entries.
  mListenerArray = (nsIMsgSendListener **) PR_Malloc(sizeof(nsIMsgSendListener *) * mListenerArrayCount);
  if (!mListenerArray)
    return NS_ERROR_FAILURE;

  nsCRT::memset(mListenerArray, 0, (sizeof(nsIMsgSendListener *) * mListenerArrayCount));
  
  // Now assign the listeners...
  PRInt32 i;
  for (i=0; i<mListenerArrayCount; i++)
  {
    mListenerArray[i] = aListenerArray[i];
    NS_ADDREF(mListenerArray[i]);
  }

  return NS_OK;
}

nsresult
nsMsgComposeAndSend::AddListener(nsIMsgSendListener *aListener)
{
  if ( (mListenerArrayCount > 0) || mListenerArray )
  {
    mListenerArrayCount = 1;
    mListenerArray = (nsIMsgSendListener **) 
                  PR_Realloc(*mListenerArray, sizeof(nsIMsgSendListener *) * mListenerArrayCount);
    if (!mListenerArray)
      return NS_ERROR_FAILURE;
    else
      return NS_OK;
  }
  else
  {
    mListenerArrayCount = 1;
    mListenerArray = (nsIMsgSendListener **) PR_Malloc(sizeof(nsIMsgSendListener *) * mListenerArrayCount);
    if (!mListenerArray)
      return NS_ERROR_FAILURE;

    nsCRT::memset(mListenerArray, 0, (sizeof(nsIMsgSendListener *) * mListenerArrayCount));
  
    mListenerArray[0] = aListener;
    NS_ADDREF(mListenerArray[0]);
    return NS_OK;
  }
}

nsresult
nsMsgComposeAndSend::RemoveListener(nsIMsgSendListener *aListener)
{
  PRInt32 i;
  for (i=0; i<mListenerArrayCount; i++)
    if (mListenerArray[i] == aListener)
    {
      NS_RELEASE(mListenerArray[i]);
      mListenerArray[i] = nsnull;
      return NS_OK;
    }

  return NS_ERROR_FAILURE;
}

nsresult
nsMsgComposeAndSend::DeleteListeners()
{
  if ( (mListenerArray) && (*mListenerArray) )
  {
    PRInt32 i;
    for (i=0; i<mListenerArrayCount; i++)
    {
      NS_RELEASE(mListenerArray[i]);
    }
    
    PR_FREEIF(mListenerArray);
  }

  mListenerArrayCount = 0;
  return NS_OK;
}

nsresult
nsMsgComposeAndSend::NotifyListenersOnStartSending(const char *aMsgID, PRUint32 aMsgSize)
{
  PRInt32 i;
  for (i=0; i<mListenerArrayCount; i++)
    if (mListenerArray[i] != nsnull)
      mListenerArray[i]->OnStartSending(aMsgID, aMsgSize);

  return NS_OK;
}

nsresult
nsMsgComposeAndSend::NotifyListenersOnProgress(const char *aMsgID, PRUint32 aProgress, PRUint32 aProgressMax)
{
  PRInt32 i;
  for (i=0; i<mListenerArrayCount; i++)
    if (mListenerArray[i] != nsnull)
      mListenerArray[i]->OnProgress(aMsgID, aProgress, aProgressMax);

  return NS_OK;
}

nsresult
nsMsgComposeAndSend::NotifyListenersOnStatus(const char *aMsgID, const PRUnichar *aMsg)
{
  PRInt32 i;
  for (i=0; i<mListenerArrayCount; i++)
    if (mListenerArray[i] != nsnull)
      mListenerArray[i]->OnStatus(aMsgID, aMsg);

  return NS_OK;
}

nsresult
nsMsgComposeAndSend::NotifyListenersOnStopSending(const char *aMsgID, nsresult aStatus, const PRUnichar *aMsg, 
                                                  nsIFileSpec *returnFileSpec)
{
  PRInt32 i;
  for (i=0; i<mListenerArrayCount; i++)
    if (mListenerArray[i] != nsnull)
      mListenerArray[i]->OnStopSending(aMsgID, aStatus, aMsg, returnFileSpec);

  return NS_OK;
}

nsresult
nsMsgComposeAndSend::NotifyListenersOnStartCopy(nsISupports *listenerData)
{
  nsCOMPtr<nsIMsgCopyServiceListener> copyListener;

  PRInt32 i;
  for (i=0; i<mListenerArrayCount; i++)
  {
    if (mListenerArray[i] != nsnull)
    {
      copyListener = do_QueryInterface(mListenerArray[i]);
      if (copyListener)
        copyListener->OnStartCopy(listenerData);      
    }
  }

  return NS_OK;
}

nsresult
nsMsgComposeAndSend::NotifyListenersOnProgressCopy(PRUint32 aProgress, PRUint32 aProgressMax, 
                                               nsISupports *listenerData)
{
  nsCOMPtr<nsIMsgCopyServiceListener> copyListener;

  PRInt32 i;
  for (i=0; i<mListenerArrayCount; i++)
  {
    if (mListenerArray[i] != nsnull)
    {
      copyListener = do_QueryInterface(mListenerArray[i]);
      if (copyListener)
        copyListener->OnProgress(aProgress, aProgressMax, listenerData);
    }
  }

  return NS_OK;  
}

nsresult
nsMsgComposeAndSend::NotifyListenersOnStopCopy(nsresult aStatus, nsISupports *listenerData)
{
  nsCOMPtr<nsIMsgCopyServiceListener> copyListener;

  PRInt32 i;
  for (i=0; i<mListenerArrayCount; i++)
  {
    if (mListenerArray[i] != nsnull)
    {
      copyListener = do_QueryInterface(mListenerArray[i]);
      if (copyListener)
        copyListener->OnStopCopy(aStatus, listenerData);
    }
  }

  return NS_OK;
}

/* This is the main driving function of this module.  It generates a
   document of type message/rfc822, which contains the stuff provided.
   The first few arguments are the standard header fields that the
   generated document should have.

   `other_random_headers' is a string of additional headers that should
   be inserted beyond the standard ones.  If provided, it is just tacked
   on to the end of the header block, so it should have newlines at the
   end of each line, shouldn't have blank lines, multi-line headers
   should be properly continued, etc.

   `digest_p' says that most of the documents we are attaching are
   themselves messages, and so we should generate a multipart/digest
   container instead of multipart/mixed.  (It's a minor difference.)

   The full text of the first attachment is provided via `attachment1_type',
   `attachment1_body' and `attachment1_body_length'.  These may all be 0
   if all attachments are provided externally.

   Subsequent attachments are provided as URLs to load, described in the
   nsMsgAttachmentData structures.

   If `dont_deliver_p' is false, then we actually deliver the message to the
   SMTP and/or NNTP server, and the message_delivery_done_callback will be
   invoked with the status.

   If `dont_deliver_p' is true, then we just generate the message, we don't
   actually deliver it, and the message_delivery_done_callback will be called
   with the name of the generated file.  The callback is responsible for both
   freeing the file name string, and deleting the file when it is done with
   it.  If an error occurred, then `status' will be negative and
   `error_message' may be an error message to display.  If status is non-
   negative, then `error_message' contains the file name (this is kind of
   a kludge...)
 */
nsresult 
nsMsgComposeAndSend::CreateAndSendMessage(
              nsIMsgIdentity                    *aUserIdentity,
 						  nsIMsgCompFields                  *fields,
						  PRBool                            digest_p,
						  PRBool                            dont_deliver_p,
						  nsMsgDeliverMode                  mode,
              nsIMessage                        *msgToReplace,
						  const char                        *attachment1_type,
						  const char                        *attachment1_body,
						  PRUint32                          attachment1_body_length,
						  const struct nsMsgAttachmentData  *attachments,
						  const struct nsMsgAttachedFile    *preloaded_attachments,
						  void                              *relatedPart,
              nsIMsgSendListener                **aListenerArray)
{
  nsresult      rv;

  SetListenerArray(aListenerArray);

  if (!attachment1_body || !*attachment1_body)
		attachment1_type = attachment1_body = 0;

  rv = Init(aUserIdentity, (nsMsgCompFields *)fields, nsnull,
					digest_p, dont_deliver_p, mode, msgToReplace,
					attachment1_type, attachment1_body,
					attachment1_body_length,
					attachments, preloaded_attachments,
					(nsMsgSendPart *)relatedPart);

	if (NS_SUCCEEDED(rv))
		return NS_OK;
  else
    return rv;
}

nsresult
nsMsgComposeAndSend::SendMessageFile(
              nsIMsgIdentity                    *aUserIndentity,
 						  nsIMsgCompFields                  *fields,
              nsFileSpec                        *sendFileSpec,
              PRBool                            deleteSendFileOnCompletion,
						  PRBool                            digest_p,
						  nsMsgDeliverMode                  mode,
              nsIMessage                        *msgToReplace,
              nsIMsgSendListener                **aListenerArray)
{
  nsresult      rv;

  if (!fields)
    return NS_ERROR_FAILURE;

  //
  // First check to see if the external file we are sending is a valid file.
  //
  if (!sendFileSpec)
    return NS_ERROR_FAILURE;
  
  if (!sendFileSpec->Exists())
    return NS_ERROR_FAILURE;

  // Setup the listeners...
  SetListenerArray(aListenerArray);

  // Should we delete the temp file when done?
  if (!deleteSendFileOnCompletion)
  {
    NS_NewFileSpecWithSpec(*sendFileSpec, &mReturnFileSpec);
	  if (!mReturnFileSpec)
      return NS_ERROR_FAILURE;
  }

  rv = Init(aUserIndentity, (nsMsgCompFields *)fields, sendFileSpec,
					    digest_p, PR_FALSE, mode, msgToReplace, 
					    nsnull, nsnull, nsnull,
					    nsnull, nsnull, nsnull);
	if (NS_SUCCEEDED(rv))
  { 
    DeliverMessage();
		return NS_OK;
  }
  else
    return rv;
}

nsMsgAttachmentData *
BuildURLAttachmentData(nsIURI *url)
{
  int                 attachCount = 2;  // one entry and one empty entry
  nsMsgAttachmentData *attachments = nsnull;
  char                *theName = nsnull;
  const char          *spec = nsnull;

  if (!url)
    return nsnull;    

  attachments = (nsMsgAttachmentData *) PR_Malloc(sizeof(nsMsgAttachmentData) * attachCount);
  if (!attachments)
    return nsnull;

  // Now get a readable name...
  url->GetSpec(&spec);
  if (spec)
  {
    theName = PL_strrchr(spec, '/');
  }

  if (!theName)
    theName = "Unknown"; // Don't I18N this string...should never happen...
  else
    theName++;

  nsCRT::memset(attachments, 0, sizeof(nsMsgAttachmentData) * attachCount);
  attachments[0].url = url; // The URL to attach. This should be 0 to signify "end of list".
  attachments[0].real_name = (char *)PL_strdup(theName);	// The original name of this document, which will eventually show up in the 

  NS_ADDREF(url);
  return attachments;
}

nsresult
nsMsgComposeAndSend::SendWebPage(nsIMsgIdentity                    *aUserIndentity,
                                 nsIMsgCompFields                  *fields,
                                 nsIURI                            *url,
                                 nsMsgDeliverMode                  mode,
                                 nsIMsgSendListener                **aListenerArray)
{
  nsresult            rv;
  nsMsgAttachmentData *tmpPageData = nsnull;
  
  //
  // First check to see if the fields are valid...
  //
  if ((!fields) || (!url) )
    return NS_ERROR_FAILURE;

  tmpPageData = BuildURLAttachmentData(url);

  // Setup the listeners...
  SetListenerArray(aListenerArray);

  /* string GetBody(); */
  char          *msgBody = nsnull;
  PRInt32       bodyLen;

  rv = fields->GetBody(&msgBody);
  if (NS_FAILED(rv) || (!msgBody))
  {
    const char *body = nsnull;
    url->GetSpec(&body);
    msgBody = (char *)body;
  }

  bodyLen = PL_strlen(msgBody);
  rv = CreateAndSendMessage(
              aUserIndentity,
 						  fields, //nsIMsgCompFields                  *fields,
						  PR_FALSE, //PRBool                            digest_p,
						  PR_FALSE, //PRBool                            dont_deliver_p,
						  mode,   //nsMsgDeliverMode                  mode,
              nsnull, //  nsIMessage      *msgToReplace,
						  TEXT_PLAIN, //const char                        *attachment1_type,
              msgBody, //const char                        *attachment1_body,
						  bodyLen, // PRUint32                          attachment1_body_length,
						  tmpPageData, // const struct nsMsgAttachmentData  *attachments,
						  nsnull,  // const struct nsMsgAttachedFile    *preloaded_attachments,
						  nsnull, // void                              *relatedPart,
              aListenerArray);  
  return rv;
}

//
// Send the message to the magic folder, and runs the completion/failure
// callback.
//
nsresult
nsMsgComposeAndSend::SendToMagicFolder(nsMsgDeliverMode mode)
{
    nsresult rv = MimeDoFCC(mTempFileSpec,
                            mode,
                            mCompFields->GetBcc(),
							              mCompFields->GetFcc(), 
                            mCompFields->GetNewspostUrl());
    //
    // The caller of MimeDoFCC needs to deal with failure.
    //
    if (NS_FAILED(rv))
    {
      // RICHIE_TODO: message loss here
      char    *eMsg = ComposeBEGetStringByID(rv);
      Fail(MK_OUT_OF_MEMORY, eMsg);
      NotifyListenersOnStopCopy(rv, nsnull);
      PR_FREEIF(eMsg);
    }
    
    return rv;
}

//
// Queues the message for later delivery, and runs the completion/failure
// callback.
//
nsresult
nsMsgComposeAndSend::QueueForLater()
{
  return SendToMagicFolder(nsMsgQueueForLater);
}

//
// Save the message to the Drafts folder, and runs the completion/failure
// callback.
//
nsresult 
nsMsgComposeAndSend::SaveAsDraft()
{
  return SendToMagicFolder(nsMsgSaveAsDraft);
}

//
// Save the message to the Template folder, and runs the completion/failure
// callback.
//
nsresult
nsMsgComposeAndSend::SaveAsTemplate()
{
  return SendToMagicFolder(nsMsgSaveAsTemplate);
}

nsresult
nsMsgComposeAndSend::SaveInSentFolder()
{
  return SendToMagicFolder(nsMsgDeliverNow);
}

char*
nsMsgGetEnvelopeLine(void)
{
  static char       result[75] = "";
	PRExplodedTime    now;
  char              buffer[128] = "";

  // Generate envelope line in format of:  From - Sat Apr 18 20:01:49 1998
  //
  // Use PR_FormatTimeUSEnglish() to format the date in US English format,
	// then figure out what our local GMT offset is, and append it (since
	// PR_FormatTimeUSEnglish() can't do that.) Generate four digit years as
	// per RFC 1123 (superceding RFC 822.)
  //
  PR_ExplodeTime(PR_Now(), PR_LocalTimeParameters, &now);
	PR_FormatTimeUSEnglish(buffer, sizeof(buffer),
						   "%a %b %d %H:%M:%S %Y",
						   &now);
  
  // This value must be in ctime() format, with English abbreviations.
	// PL_strftime("... %c ...") is no good, because it is localized.
  //
  PL_strcpy(result, "From - ");
  PL_strcpy(result + 7, buffer);
  PL_strcpy(result + 7 + 24, MSG_LINEBREAK);
  return result;
}

nsresult 
nsMsgComposeAndSend::MimeDoFCC(nsFileSpec       *input_file, 
			                         nsMsgDeliverMode mode,
			                         const char       *bcc_header,
			                         const char       *fcc_header,
			                         const char       *news_url)
{
  nsresult      status = NS_OK;
  char          *ibuffer = 0;
  PRInt32       ibuffer_size = TEN_K;
  char          *obuffer = 0;
  PRInt32       obuffer_size = 0, obuffer_fp = 0;
  PRInt32       n;
  char          *envelopeLine = nsMsgGetEnvelopeLine();

  //
  // Create the file that will be used for the copy service!
  //
  nsFileSpec *tFileSpec = nsMsgCreateTempFileSpec("nscopy.tmp"); 
	if (!tFileSpec)
    return NS_ERROR_FAILURE;

  NS_NewFileSpecWithSpec(*tFileSpec, &mCopyFileSpec);
	if (!mCopyFileSpec)
  {
    delete tFileSpec;
    return NS_ERROR_FAILURE;
  }

  nsOutputFileStream tempOutfile(mCopyFileSpec);
  if (! tempOutfile.is_open()) 
  {	  
    // Need to determine what type of operation failed and set status accordingly. 
    switch (mode)
    {
    case nsMsgSaveAsDraft:
      status = MK_MSG_UNABLE_TO_SAVE_DRAFT;
      break;
    case nsMsgSaveAsTemplate:
      status = MK_MSG_UNABLE_TO_SAVE_TEMPLATE;
      break;
    case nsMsgDeliverNow:
    default:
      status = MK_MSG_COULDNT_OPEN_FCC_FILE;
      break;
    }
    delete tFileSpec;
    NS_RELEASE(mCopyFileSpec);
    return status;
  }

  //
  // Get our files ready...
  //
  nsInputFileStream inputFile(*input_file);
  if (!inputFile.is_open())
	{
	  status = MK_UNABLE_TO_OPEN_FILE;
	  goto FAIL;
	}

  // now the buffers...
  ibuffer = nsnull;
  while (!ibuffer && (ibuffer_size >= 1024))
  {
	  ibuffer = (char *) PR_Malloc(ibuffer_size);
	  if (!ibuffer)
		  ibuffer_size /= 2;
  }

  if (!ibuffer)
	{
	  status = MK_OUT_OF_MEMORY;
	  goto FAIL;
	}

  //
  // First, we we need to put a Berkely "From - " delimiter at the head of 
  // the file for parsing...
  //
  if (envelopeLine)
  {
    PRInt32   len = PL_strlen(envelopeLine);
    
    n = tempOutfile.write(envelopeLine, len);
    if (n != len)
    {
      status = NS_ERROR_FAILURE;
      goto FAIL;
    }
  }

  //
  // Write out an X-Mozilla-Status header.
  //
  // This is required for the queue file, so that we can overwrite it once
	// the messages have been delivered, and so that the MSG_FLAG_QUEUED bit
	// is set.
  //
	// For FCC files, we don't necessarily need one, but we might as well put
	// one in so that it's marked as read already.
  //
  if (mode == nsMsgQueueForLater  || mode == nsMsgSaveAsDraft ||
	    mode == nsMsgSaveAsTemplate || mode == nsMsgDeliverNow)
	{
	  char       *buf = 0;
	  PRUint16   flags = 0;

	  flags |= MSG_FLAG_READ;
	  if (mode == nsMsgQueueForLater)
  		flags |= MSG_FLAG_QUEUED;
	  buf = PR_smprintf(X_MOZILLA_STATUS_FORMAT MSG_LINEBREAK, flags);
	  if (buf)
	  {
      PRInt32   len = PL_strlen(buf);
      n = tempOutfile.write(buf, len);
		  PR_FREEIF(buf);
		  if (n != len)
      {
        status = NS_ERROR_FAILURE;
			  goto FAIL;
      }
	  }
	  
	  PRUint32 flags2 = 0;
	  if (mode == nsMsgSaveAsTemplate)
		  flags2 |= MSG_FLAG_TEMPLATE;
	  buf = PR_smprintf(X_MOZILLA_STATUS2_FORMAT MSG_LINEBREAK, flags2);
	  if (buf)
	  {
      PRInt32   len = PL_strlen(buf);
      n = tempOutfile.write(buf, len);
		  PR_FREEIF(buf);
		  if (n != len)
      {
        status = NS_ERROR_FAILURE;
			  goto FAIL;
      }
	  }
	}

  // Write out the FCC and BCC headers.
	// When writing to the Queue file, we *must* write the FCC and BCC
	// headers, or else that information would be lost.  Because, when actually
	// delivering the message (with "deliver now") we do FCC/BCC right away;
	// but when queueing for later delivery, we do FCC/BCC at delivery-time.
  //
	// The question remains of whether FCC and BCC should be written into normal
	// BCC folders (like the Sent Mail folder.)
  //
	// For FCC, there seems no point to do that; it's not information that one
	// would want to refer back to.
  //
	// For BCC, the question isn't as clear.  On the one hand, if I send someone
	// a BCC'ed copy of the message, and save a copy of it for myself (with FCC)
	// I might want to be able to look at that message later and see the list of
	// people to whom I had BCC'ed it.
  //
	// On the other hand, the contents of the BCC header is sensitive
	// information, and should perhaps not be stored at all.
  //
	// Thus the consultation of the #define SAVE_BCC_IN_FCC_FILE.
  //
	// (Note that, if there is a BCC header present in a message in some random
	// folder, and that message is forwarded to someone, then the attachment
	// code will strip out the BCC header before forwarding it.)
  //
  if ((mode == nsMsgQueueForLater || mode == nsMsgSaveAsDraft ||
	     mode == nsMsgSaveAsTemplate) && fcc_header && *fcc_header)
	{
	  PRInt32 L = PL_strlen(fcc_header) + 20;
	  char  *buf = (char *) PR_Malloc (L);
	  if (!buf)
		{
		  status = MK_OUT_OF_MEMORY;
		  goto FAIL;
		}

	  PR_snprintf(buf, L-1, "FCC: %s" MSG_LINEBREAK, fcc_header);

    PRInt32   len = PL_strlen(buf);
    n = tempOutfile.write(buf, len);
		if (n != len)
    {
      status = NS_ERROR_FAILURE;
      goto FAIL;
    }
	}

  if (bcc_header && *bcc_header
#ifndef SAVE_BCC_IN_FCC_FILE
	    && (mode == MSG_QueueForLater || mode == MSG_SaveAsDraft ||
		      mode == MSG_SaveAsTemplate)
#endif
	  )
	{
	  PRInt32 L = PL_strlen(bcc_header) + 20;
	  char *buf = (char *) PR_Malloc (L);
	  if (!buf)
		{
		  status = MK_OUT_OF_MEMORY;
		  goto FAIL;
		}

	  PR_snprintf(buf, L-1, "BCC: %s" MSG_LINEBREAK, bcc_header);
    PRInt32   len = PL_strlen(buf);
    n = tempOutfile.write(buf, len);
		if (n != len)
    {
      status = NS_ERROR_FAILURE;
      goto FAIL;
    }
	}

  //
  // Write out the X-Mozilla-News-Host header.
	// This is done only when writing to the queue file, not the FCC file.
	// We need this to complement the "Newsgroups" header for the case of
	// queueing a message for a non-default news host.
  //
	// Convert a URL like "snews://host:123/" to the form "host:123/secure"
	// or "news://user@host:222" to simply "host:222".
  //
  if ((mode == nsMsgQueueForLater || mode == nsMsgSaveAsDraft ||
	     mode == nsMsgSaveAsTemplate) && news_url && *news_url)
	{
	  PRBool secure_p = (news_url[0] == 's' || news_url[0] == 'S');
	  char *orig_hap = nsMsgParseURL (news_url, GET_HOST_PART);
	  char *host_and_port = orig_hap;
	  if (host_and_port)
		{
		  // There may be authinfo at the front of the host - it could be of
			// the form "user:password@host:port", so take off everything before
			// the first at-sign.  We don't want to store authinfo in the queue
			// folder, I guess, but would want it to be re-prompted-for at
			// delivery-time.
		  //
		  char *at = PL_strchr (host_and_port, '@');
		  if (at)
  			host_and_port = at + 1;
		}

	  if ((host_and_port && *host_and_port) || !secure_p)
		{
		  char *line = PR_smprintf(X_MOZILLA_NEWSHOST ": %s%s" MSG_LINEBREAK,
								   host_and_port ? host_and_port : "",
								   secure_p ? "/secure" : "");
		  PR_FREEIF(orig_hap);
		  orig_hap = 0;
		  if (!line)
			{
			  status = MK_OUT_OF_MEMORY;
			  goto FAIL;
			}

      PRInt32   len = PL_strlen(line);
      n = tempOutfile.write(line, len);
		  PR_FREEIF(line);
		  if (n != len)
      {
        status = NS_ERROR_FAILURE;
        goto FAIL;
      }
		}

	  PR_FREEIF(orig_hap);
	  orig_hap = 0;
	}

  //
  // Read from the message file, and write to the FCC or Queue file.
	// There are two tricky parts: the first is that the message file
	// uses CRLF, and the FCC file should use LINEBREAK.  The second
	// is that the message file may have lines beginning with "From "
	// but the FCC file must have those lines mangled.
  //
	// It's unfortunate that we end up writing the FCC file a line
	// at a time, but it's the easiest way...
  //
  while (! inputFile.eof())
	{
    if (!inputFile.readline(ibuffer, ibuffer_size))
    {
      status = NS_ERROR_FAILURE;
      goto FAIL;
    }

    n =  tempOutfile.write(ibuffer, PL_strlen(ibuffer));
    n += tempOutfile.write(NS_LINEBREAK, NS_LINEBREAK_LEN);
	  if (n != (PRInt32) (PL_strlen(ibuffer) + NS_LINEBREAK_LEN)) // write failed 
		{
		  status = MK_MIME_ERROR_WRITING_FILE;
		  goto FAIL;
		}
	}

  //
  // Terminate with a final newline. 
  //
  n = tempOutfile.write(NS_LINEBREAK, NS_LINEBREAK_LEN);
  if (n != NS_LINEBREAK_LEN) // write failed 
	{
		status = MK_MIME_ERROR_WRITING_FILE;
    goto FAIL;
	}

FAIL:
  if (ibuffer)
  	PR_FREEIF(ibuffer);
  if (obuffer && obuffer != ibuffer)
  	PR_FREEIF(obuffer);


  if (tempOutfile.is_open()) 
  {
    tempOutfile.close();
    if (mCopyFileSpec)
      mCopyFileSpec->closeStream();
  }

  if (inputFile.is_open()) 
    inputFile.close();

  // 
  // When we get here, we have to see if we have been successful so far.
  // If we have, then we should start up the async copy service operation.
  // If we weren't successful, then we should just return the error and 
  // bail out.
  //
  if (NS_FAILED(status))
	{
	  // Fail, and return...
	  return status;
	}
  else
	{
	  //
    // If we are here, time to start the async copy service operation!
    //
    return StartMessageCopyOperation(mCopyFileSpec, mode);
	}
}

//
// This is pretty much a wrapper to the functionality that lives in the 
// nsMsgCopy class
//
nsresult 
nsMsgComposeAndSend::StartMessageCopyOperation(nsIFileSpec        *aFileSpec, 
                                               nsMsgDeliverMode   mode)
{
  mCopyObj = new nsMsgCopy();
  if (!mCopyObj)
    return MK_OUT_OF_MEMORY;

  if (!mCompFields->GetFcc() || !*mCompFields->GetFcc())
    return mCopyObj->StartCopyOperation(mUserIdentity, aFileSpec, mode, 
                                        this, nsnull);
  else
    return mCopyObj->StartCopyOperation(mUserIdentity, aFileSpec, mode, 
                                        this, mCompFields->GetFcc());
}
