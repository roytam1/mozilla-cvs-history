/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

#ifndef nsIDocumentEncoder_h__
#define nsIDocumentEncoder_h__

#include "nsISupports.h"
#include "nsString.h"

class nsIDocumentEncoder;
class nsIDocument;
class nsIDOMSelection;
class nsIOutputStream;
class nsISupportsArray;


#define NS_IDOCUMENT_ENCODER_IID                     \
{ /* a6cf9103-15b3-11d2-932e-00805f8add32 */         \
    0xa6cf9103,                                      \
    0x15b3,                                          \
    0x11d2,                                          \
    {0x93, 0x2e, 0x00, 0x80, 0x5f, 0x8a, 0xdd, 0x32} \
  }

#define NS_TEXT_ENCODER_CID                          \
{ /* e7ba1480-1dea-11d3-830f-00104bed045e */         \
    0xe7ba1480,                                      \
    0x1dea,                                          \
    0x11d3,                                          \
    {0x83, 0x0f, 0x00, 0x10, 0x4b, 0xed, 0x04, 0x5e} \
}

#define NS_DOC_ENCODER_PROGID_BASE "component://netscape/layout/documentEncoder?type="

class nsIDocumentEncoder : public nsISupports
{
public:

  /**
   * Output methods flag bits.
   *
   * There are a frightening number of these,
   * because everyone wants something a little bit different!
   *
   * These should move to an idl file so that Javascript can
   * have access to the symbols, not just the constants.
   */
  enum {
    // Output only the selection.
    OutputSelectionOnly = 1,

    // Convert html to plaintext that looks like the html.
    // Implies wrap (except inside <pre>), since html wraps.
    OutputFormatted     = 2,

    // Don't output the html doctype and gecko output system comment headers
    OutputNoDoctype     = 4,

    // No html head tags
    OutputBodyOnly      = 8,

    // Wrap even if we're not doing formatted output (e.g. for text fields)
    OutputPreformatted  = 16,

    // Output as though the content is preformatted
    // (e.g. maybe it's wrapped in a MOZ_PRE or MOZ_PRE_WRAP style tag)
    OutputWrap          = 32,

    // Output for format flowed (RFC 2646). This is used when converting
    // to text for mail sending. This differs just slightly
    // but in an important way from normal formatted, and that is that
    // lines are space stuffed. This can't (correctly) be done later.
    OutputFormatFlowed  = 64
  };
  
  static const nsIID& GetIID() { static nsIID iid = NS_IDOCUMENT_ENCODER_IID; return iid; }

  /**
   *  Initialize with a pointer to the document and the mime type.
   */
  NS_IMETHOD Init(nsIDocument* aDocument, const nsString& aMimeType, PRUint32 flags) = 0;

  /**
   *  If the selection is set to a non-null value, then the
   *  selection is used for encoding, otherwise the entire
   *  document is encoded.
   */
  NS_IMETHOD SetSelection(nsIDOMSelection* aSelection) = 0;

  /**
   *  Documents typically have an intrinsic character set.
   *  If no intrinsic value is found, the platform character set
   *  is used.
   *  aCharset overrides the both the intrinsic or platform
   *  character set when encoding the document.
   *
   *  Possible result codes: NS_ERROR_NO_CHARSET_CONVERTER
   */
  NS_IMETHOD SetCharset(const nsString& aCharset) = 0;

  /**
   *  Set a wrap column.  This may have no effect in some types of encoders.
   */
  NS_IMETHOD SetWrapColumn(PRUint32 aWC) = 0;

  /**
   *  The document is encoded, the result is sent to the 
   *  to nsIOutputStream.
   * 
   *  Possible result codes are passing along whatever stream errors
   *  might have been encountered.
   */
  NS_IMETHOD EncodeToStream(nsIOutputStream* aStream) = 0;
  NS_IMETHOD EncodeToString(nsString& aOutputString) = 0;
};

// XXXXXXXXXXXXXXXX nsITextEncoder is going away! XXXXXXXXXXXXXXXXXXXXXX
#ifdef USE_OBSOLETE_TEXT_ENCODER
// Example of a output service for a particular encoder.
// The text encoder handles XIF, HTML, and plaintext.
class nsITextEncoder : public nsIDocumentEncoder
{
public:
  static const nsIID& GetIID() { static nsIID iid = NS_TEXT_ENCODER_CID; return iid; }

  // Get embedded objects -- images, links, etc.
  // NOTE: we may want to use an enumerator
  NS_IMETHOD PrettyPrint(PRBool aYes) = 0;
  NS_IMETHOD SetWrapColumn(PRUint32 aWC) = 0;
  NS_IMETHOD AddHeader(PRBool aYes) = 0;
};
#endif /* USE_OBSOLETE_TEXT_ENCODER */


#endif /* nsIDocumentEncoder_h__ */

