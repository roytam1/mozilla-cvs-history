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

#define NS_IMPL_IDS
#include "nsIUnicharInputStream.h"
#include "nsIByteBuffer.h"
#include "nsIUnicharBuffer.h"
#include "nsIServiceManager.h"
#include "nsICharsetConverterManager.h"
#include "nsIUnicodeDecoder.h"
#include "nsString.h"
#include "nsCRT.h"
#include <fcntl.h>
#ifdef NS_WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

static NS_DEFINE_CID(kCharsetConverterManagerCID, NS_ICHARSETCONVERTERMANAGER_CID);

class StringUnicharInputStream : public nsIUnicharInputStream {
public:
  StringUnicharInputStream(nsString* aString);
  virtual ~StringUnicharInputStream();

  NS_DECL_ISUPPORTS

  NS_IMETHOD Read(PRUnichar* aBuf,
                  PRUint32 aOffset,
                  PRUint32 aCount,
                  PRUint32 *aReadCount);
  NS_IMETHOD Close();

  nsString* mString;
  PRUint32 mPos;
  PRUint32 mLen;
};

StringUnicharInputStream::StringUnicharInputStream(nsString* aString)
{
  NS_INIT_REFCNT();
  mString = aString;
  mPos = 0;
  mLen = aString->Length();
}

StringUnicharInputStream::~StringUnicharInputStream()
{
  if (nsnull != mString) {
    delete mString;
  }
}

nsresult StringUnicharInputStream::Read(PRUnichar* aBuf,
                                        PRUint32 aOffset,
                                        PRUint32 aCount,
                                        PRUint32 *aReadCount)
{
  if (mPos >= mLen) {
    *aReadCount = 0;
    return (nsresult)-1;
  }
  const PRUnichar* us = mString->GetUnicode();
  NS_ASSERTION(mLen >= mPos, "unsigned madness");
  PRUint32 amount = mLen - mPos;
  if (amount > aCount) {
    amount = aCount;
  }
  nsCRT::memcpy(aBuf + aOffset, us + mPos, sizeof(PRUnichar) * amount);
  mPos += amount;
  *aReadCount = amount;
  return NS_OK;
}

nsresult StringUnicharInputStream::Close()
{
  mPos = mLen;
  if (nsnull != mString) {
    delete mString;
  }
  return NS_OK;
}

NS_IMPL_ISUPPORTS1(StringUnicharInputStream, nsIUnicharInputStream)

NS_COM nsresult
NS_NewStringUnicharInputStream(nsIUnicharInputStream** aInstancePtrResult,
                               nsString* aString)
{
  NS_PRECONDITION(nsnull != aString, "null ptr");
  NS_PRECONDITION(nsnull != aInstancePtrResult, "null ptr");
  if ((nsnull == aString) || (nsnull == aInstancePtrResult)) {
    return NS_ERROR_NULL_POINTER;
  }

  StringUnicharInputStream* it = new StringUnicharInputStream(aString);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return it->QueryInterface(NS_GET_IID(nsIUnicharInputStream),
                            (void**) aInstancePtrResult);
}

//----------------------------------------------------------------------

/**
 * This function used to be public, with the NS_COM declaration. I am 
 * changing it right now into a module private visibility because there are
 * better and more xpcom-like ways to get a Converter.
 */

nsresult NS_NewB2UConverter(nsIUnicodeDecoder** aInstancePtrResult, nsISupports* aOuter, nsString* aCharSet);
nsresult
NS_NewB2UConverter(nsIUnicodeDecoder** aInstancePtrResult,
                   nsISupports* aOuter,
                   nsString* aCharSet)
{
  if (nsnull != aOuter) {
    return NS_ERROR_NO_AGGREGATION;
  }

  // Create converter
  nsresult res;
  nsAutoString defaultCharset;
  defaultCharset.AssignWithConversion("ISO-8859-1");

  if (aCharSet == nsnull) aCharSet = &defaultCharset;

  NS_WITH_SERVICE(nsICharsetConverterManager, ccm, kCharsetConverterManagerCID, &res);
  if (NS_FAILED(res)) return res;

  return ccm->GetUnicodeDecoder(aCharSet, aInstancePtrResult);
}

//----------------------------------------------------------------------

class ConverterInputStream : public nsIUnicharInputStream {
public:
  ConverterInputStream(nsIInputStream* aStream,
                       nsIUnicodeDecoder* aConverter,
                       PRUint32 aBufSize);
  virtual ~ConverterInputStream();

  NS_DECL_ISUPPORTS
  NS_IMETHOD Read(PRUnichar* aBuf,
                  PRUint32 aOffset,
                  PRUint32 aCount,
                  PRUint32 *aReadCount);
  NS_IMETHOD Close();

protected:
  PRInt32 Fill(nsresult * aErrorCode);

  nsIInputStream* mInput;
  nsIUnicodeDecoder* mConverter;
  nsIByteBuffer* mByteData;
  PRUint32 mByteDataOffset;
  nsIUnicharBuffer* mUnicharData;
  PRUint32 mUnicharDataOffset;
  PRUint32 mUnicharDataLength;
};

ConverterInputStream::ConverterInputStream(nsIInputStream* aStream,
                                           nsIUnicodeDecoder* aConverter,
                                           PRUint32 aBufferSize)
{
  NS_INIT_REFCNT();
  mInput = aStream; aStream->AddRef();
  mConverter = aConverter; aConverter->AddRef();
  if (aBufferSize == 0) {
    aBufferSize = 8192;
  }

  // XXX what if these fail?
  NS_NewByteBuffer(&mByteData, nsnull, aBufferSize);
  NS_NewUnicharBuffer(&mUnicharData, nsnull, aBufferSize);

  mByteDataOffset = 0;
  mUnicharDataOffset = 0;
  mUnicharDataLength = 0;
}

NS_IMPL_ISUPPORTS1(ConverterInputStream,nsIUnicharInputStream)

ConverterInputStream::~ConverterInputStream()
{
  Close();
}

nsresult ConverterInputStream::Close()
{
  if (nsnull != mInput) {
    mInput->Release();
    mInput = nsnull;
  }
  if (nsnull != mConverter) {
    mConverter->Release();
    mConverter = nsnull;
  }
  if (nsnull != mByteData) {
    mByteData->Release();
    mByteData = nsnull;
  }
  if (nsnull != mUnicharData) {
    mUnicharData->Release();
    mUnicharData = nsnull;
  }

  return NS_OK;
}

nsresult ConverterInputStream::Read(PRUnichar* aBuf,
                                    PRUint32 aOffset,
                                    PRUint32 aCount,
                                    PRUint32 *aReadCount)
{
  NS_ASSERTION(mUnicharDataLength >= mUnicharDataOffset, "unsigned madness");
  PRUint32 rv = mUnicharDataLength - mUnicharDataOffset;
  nsresult errorCode;
  if (0 == rv) {
    // Fill the unichar buffer
    rv = Fill(&errorCode);
    if (rv <= 0) {
      *aReadCount = 0;
      return errorCode;
    }
  }
  if (rv > aCount) {
    rv = aCount;
  }
  nsCRT::memcpy(aBuf + aOffset, mUnicharData->GetBuffer() + mUnicharDataOffset,
                rv * sizeof(PRUnichar));
  mUnicharDataOffset += rv;
  *aReadCount = rv;
  return NS_OK;
}

PRInt32 ConverterInputStream::Fill(nsresult * aErrorCode)
{
  if (nsnull == mInput) {
    // We already closed the stream!
    *aErrorCode = NS_BASE_STREAM_CLOSED;
    return -1;
  }

  NS_ASSERTION(mByteData->GetLength() >= mByteDataOffset, "unsigned madness");
  PRUint32 remainder = mByteData->GetLength() - mByteDataOffset;
  mByteDataOffset = remainder;
  PRInt32 nb = mByteData->Fill(aErrorCode, mInput, remainder);
  if (nb <= 0) {
    // Because we assume a many to one conversion, the lingering data
    // in the byte buffer must be a partial conversion
    // fragment. Because we know that we have recieved no more new
    // data to add to it, we can't convert it. Therefore, we discard
    // it.
    return nb;
  }
  NS_ASSERTION(remainder + nb == mByteData->GetLength(), "bad nb");

  // Now convert as much of the byte buffer to unicode as possible
  PRInt32 dstLen = mUnicharData->GetBufferSize();
  PRInt32 srcLen = remainder + nb;
  *aErrorCode = mConverter->Convert(mByteData->GetBuffer(), &srcLen,
                                    mUnicharData->GetBuffer(), &dstLen);
  mUnicharDataOffset = 0;
  mUnicharDataLength = dstLen;
  mByteDataOffset += srcLen;
  return dstLen;
}

// XXX hook up auto-detect here (do we need more info, like the url?)
NS_COM nsresult
NS_NewConverterStream(nsIUnicharInputStream** aInstancePtrResult,
                      nsISupports* aOuter,
                      nsIInputStream* aStreamToWrap,
                      PRInt32 aBufferSize,
                      nsString* aCharSet)
{
  if (nsnull != aOuter) {
    return NS_ERROR_NO_AGGREGATION;
  }

  // Create converter
  nsIUnicodeDecoder* converter;
  nsresult rv = NS_NewB2UConverter(&converter, nsnull, aCharSet);
  if (NS_OK != rv) {
    return rv;
  }

  // Create converter input stream
  ConverterInputStream* it =
    new ConverterInputStream(aStreamToWrap, converter, aBufferSize);
  NS_RELEASE(converter);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return it->QueryInterface(NS_GET_IID(nsIUnicharInputStream), 
                            (void **) aInstancePtrResult);
}
