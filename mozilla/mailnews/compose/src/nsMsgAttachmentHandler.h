/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#ifndef _nsMsgAttachment_H_
#define _nsMsgAttachment_H_

#include "nsIURL.h"
#include "nsURLFetcher.h"
#include "nsIMimeConverter.h"
#include "nsMsgCompFields.h"
#include "nsIMsgStatusFeedback.h"

// Forward declarations...
class    nsMsgComposeAndSend;

#ifdef RICHIE_XP_MAC

#include "nsFileStream.h"
#include "nsMsgAppleDouble.h"

typedef struct _AppledoubleEncodeObject
{
	appledouble_encode_object   ap_encode_obj;
	char                        *buff;					// the working buff
	PRInt32	                    s_buff;         // the working buff size
  nsIOFileStream              *fileStream;    // file to hold the encoding
} AppleDoubleEncodeObject;

#endif  // XP_MAC

//
// This is a class that deals with processing remote attachments. It implements
// an nsIStreamListener interface to deal with incoming data
//
class nsMsgAttachmentHandler // : public nsMsgZapIt
{
public:
  nsMsgAttachmentHandler();
  ~nsMsgAttachmentHandler();

  //////////////////////////////////////////////////////////////////////
  // Object methods...
  //////////////////////////////////////////////////////////////////////
  //
  nsresult              SnarfAttachment(nsMsgCompFields *compFields);
  nsresult              SnarfMsgAttachment(nsMsgCompFields *compFields);
  nsresult              UrlExit(nsresult status, const PRUnichar* aMsg);

  PRBool                UseUUEncode_p(void);
  int	                  PickEncoding (const char *charset);
  void                  AnalyzeDataChunk (const char *chunk, PRInt32 chunkSize);
  void                  AnalyzeSnarfedFile ();      // Analyze a previously-snarfed file.
  									                                // (Currently only used for plaintext
  									                                // converted from HTML.) 

  nsresult              LoadDataFromFile(nsFileSpec& fSpec, nsString &sigData);

  //////////////////////////////////////////////////////////////////////
  // Member vars...
  //////////////////////////////////////////////////////////////////////
  //
  nsIURI                *mURL;
  nsFileSpec            *mFileSpec;					// The temp file to which we save it 
  nsOutputFileStream    *mOutFile;          // The temp file stream pointer
  nsURLFetcher          *mFetcher;          // The URL Fetcher
  nsMsgCompFields       *mCompFields;       // Message composition fields for the sender

#ifdef XP_MAC
  nsFileSpec            *mAppleFileSpec;    // The temp file holds the appledouble
									                          // encoding of the file we want to send.
#endif
  char                  *m_x_mac_type;      // Mac file type
  char                  *m_x_mac_creator;   // Mac file creator
  
  PRBool                m_done;
  nsMsgComposeAndSend		*m_mime_delivery_state;
  char                  *m_charset;         // charset name 
  char                  *m_content_id;      // This is for mutipart/related Content-ID's
  char                  *m_type;            // The real type, once we know it.
  char                  *m_override_type;   // The type we should assume it to be
									                          // or 0, if we should get it from the
									                          // server)
  char                  *m_override_encoding; // Goes along with override_type 

  char                  *m_desired_type;		// The type it should be converted to. 
  char                  *m_description;     // For Content-Description header
  char                  *m_real_name;				// The name for the headers, if different
									                          // from the URL. 
  char                  *m_encoding;				// The encoding, once we've decided. */
  PRBool                m_already_encoded_p; // If we attach a document that is already
									                           // encoded, we just pass it through.

  PRBool                m_decrypted_p;	/* S/MIME -- when attaching a message that was
							                             encrypted, it's necessary to decrypt it first
							                             (since nobody but the original recipient can
							                             read it -- if you forward it to someone in the
							                             raw, it will be useless to them.)  This flag
							                             indicates whether decryption occurred, so that
							                             libmsg can issue appropriate warnings about
							                             doing a cleartext forward of a message that was
							                             originally encrypted. */

  PRBool                mDeleteFile;      // If this is true, Delete the file...its 
                                          // NOT the original file!

  PRBool                mMHTMLPart;           // This is true if its an MHTML part, otherwise, PR_FALSE
  PRBool                mPartUserOmissionOverride;  // This is true if the user send send the email without this part

  //
  // Vars for analyzing file data...
  //
  PRUint32              m_size;					/* Some state used while filtering it */
  PRUint32              m_unprintable_count;
  PRUint32              m_highbit_count;
  PRUint32              m_ctl_count;
  PRUint32              m_null_count;
  PRUint32              m_current_column;
  PRUint32              m_max_column;
  PRUint32              m_lines;
  PRBool                m_file_analyzed;

  MimeEncoderData *m_encoder_data;  /* Opaque state for base64/qp encoder. */
  char *                m_uri; // original uri string
};


#endif /* _nsMsgAttachment_H_ */
