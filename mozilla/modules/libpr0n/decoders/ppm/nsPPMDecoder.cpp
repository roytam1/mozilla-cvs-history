/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
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
 * The Original Code is mozilla.org code.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 2001 Netscape Communications Corporation.
 * All Rights Reserved.
 * 
 * Contributor(s):
 *   Stuart Parmenter <pavlov@netscape.com>
 *
 *
 * the ppm decoding function is from Tim Rowley <tor@cs.brown.edu>
 *  i dunno its license
 *
 */

#include "nsPPMDecoder.h"

#include "nsIInputStream.h"
#include "nsIImageContainer.h"

#include "nspr.h"

#include "nsUnitConverters.h"

#include "nsIComponentManager.h"

NS_IMPL_ISUPPORTS2(nsPPMDecoder, nsIImageDecoder, nsIOutputStream)


nsPPMDecoder::nsPPMDecoder()
{
  NS_INIT_ISUPPORTS();
  mDataReceived = 0;
  mDataWritten = 0;

  mDataLeft = 0;
  mPrevData = nsnull;
}

nsPPMDecoder::~nsPPMDecoder()
{

}


/** nsIImageDecoder methods **/

/* void init (in nsIImageRequest aRequest); */
NS_IMETHODIMP nsPPMDecoder::Init(nsIImageRequest *aRequest)
{
  mRequest = aRequest;

  nsCOMPtr<nsIImageContainer> container;
  aRequest->GetImage(getter_AddRefs(container));

  mImage = do_CreateInstance("@mozilla.org/gfx/image/frame;2");
  if (!mImage)
    return NS_ERROR_FAILURE;

  container->AppendFrame(mImage);

  return NS_OK;
}

/* readonly attribute nsIImageRequest request; */
NS_IMETHODIMP nsPPMDecoder::GetRequest(nsIImageRequest * *aRequest)
{
  *aRequest = mRequest;
  NS_ADDREF(*aRequest);
  return NS_OK;
}






/** nsIOutputStream methods **/

/* void close (); */
NS_IMETHODIMP nsPPMDecoder::Close()
{
  printf("nsPPMDecoder::Close()\n");

  // XXX hack
  gfx_format format;
  mImage->GetFormat(&format);
  
  return NS_OK;
}

/* void flush (); */
NS_IMETHODIMP nsPPMDecoder::Flush()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* unsigned long write (in string buf, in unsigned long count); */
NS_IMETHODIMP nsPPMDecoder::Write(const char *buf, PRUint32 count, PRUint32 *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

static char *__itoa(int n)
{
	char *s;
	int i, j, sign, tmp;
	
	/* check sign and convert to positive to stringify numbers */
	if ( (sign = n) < 0)
		n = -n;
	i = 0;
	s = (char*) malloc(sizeof(char));
	
	/* grow string as needed to add numbers from powers of 10 
     * down till none left 
     */
	do
	{
		s = (char*) realloc(s, (i+1)*sizeof(char));
		s[i++] = n % 10 + '0';  /* '0' or 30 is where ASCII numbers start */
		s[i] = '\0';
	}
	while( (n /= 10) > 0);	
	
	/* tack on minus sign if we found earlier that this was negative */
	if (sign < 0)
	{
		s = (char*) realloc(s, (i+1)*sizeof(char));
		s[i++] = '-';
	}
	s[i] = '\0';
	
	/* pop numbers (and sign) off of string to push back into right direction */
	for (i = 0, j = strlen(s) - 1; i < j; i++, j--)
	{
		tmp = s[i];
		s[i] = s[j];
		s[j] = tmp;
	}
	
	return s;
}


/* unsigned long writeFrom (in nsIInputStream inStr, in unsigned long count); */
NS_IMETHODIMP nsPPMDecoder::WriteFrom(nsIInputStream *inStr, PRUint32 count, PRUint32 *_retval)
{
  nsresult rv;

  char *buf = (char *)PR_Malloc(count + mDataLeft);
  if (!buf)
    return NS_ERROR_OUT_OF_MEMORY; /* we couldn't allocate the object */

  
  // read the data from the input stram...
  PRUint32 readLen;
  rv = inStr->Read(buf+mDataLeft, count, &readLen);

  PRUint32 dataLen = readLen + mDataLeft;

  if (mPrevData) {
    strncpy(buf, mPrevData, mDataLeft);
    PR_Free(mPrevData);
    mPrevData = nsnull;
    mDataLeft = 0;
  }

  char *data = buf;

  if (NS_FAILED(rv)) return rv;

  if (mDataReceived == 0) {

    // Check the magic number
    char type;
    if ((sscanf(data, "P%c\n", &type) !=1) || (type != '6')) {
      return NS_ERROR_FAILURE;
    }
    int i = 3;
    data += i;

#if 0
    // XXX
    // Ignore comments
    while ((input = fgetc(f)) == '#')
      fgets(junk, 512, f);
    ungetc(input, f);
#endif

    // Read size
    int w, h, mcv;

    if (sscanf(data, "%d %d\n%d\n", &w, &h, &mcv) != 3) {
      return NS_ERROR_FAILURE;
    }
    char *ws = __itoa(w), *hs = __itoa(h), *mcvs = __itoa(mcv);
    int j = strlen(ws) + strlen(hs) + strlen(mcvs) + 3;
    data += j;
//    free(ws);
//    free(hs);
//    free(mcvs);

    readLen -= i + j;
    dataLen = readLen; // since this is the first pass, we don't have any data waiting that we need to keep track of

    // XXX this isn't the width/height that was actually requested.. get that from mRequest.
    mImage->Init(0, 0, w, h, nsIGFXFormat::RGB);

  }

  PRUint32 bpr;
  gfx_dimension width;
  mImage->GetBytesPerRow(&bpr);
  mImage->GetWidth(&width);

  PRUint32 real_bpr = GFXCoordToIntCeil(width) * 3;
  
  PRUint32 i = 0;
  PRUint32 rownum = mDataWritten / real_bpr;  // XXX this better not have a decimal
  
  PRUint32 wroteLen = 0;

  if (readLen > real_bpr) {

    do {
      PRUint8 *line = (PRUint8*)data + i*real_bpr;
      mImage->SetBits(line, real_bpr, (rownum++)*bpr);
      wroteLen += real_bpr ;
      i++;
    } while(dataLen >= real_bpr * (i+1));

  }
  
  mDataReceived += readLen;  // don't double count previous data that is in 'dataLen'
  mDataWritten += wroteLen;

  PRUint32 dataLeft = dataLen - wroteLen;

  if (dataLeft > 0) {
    if (mPrevData) {
      mPrevData = (char *)PR_Realloc(mPrevData, mDataLeft + dataLeft);
      strncpy(mPrevData + mDataLeft, data+wroteLen, dataLeft);
      mDataLeft += dataLeft;

    } else {
      mDataLeft = dataLeft;
      mPrevData = (char *)PR_Malloc(mDataLeft);
      strncpy(mPrevData, data+wroteLen, mDataLeft);
    }
  }

  PR_FREEIF(buf);

  return NS_OK;
}

/* [noscript] unsigned long writeSegments (in nsReadSegmentFun reader, in voidPtr closure, in unsigned long count); */
NS_IMETHODIMP nsPPMDecoder::WriteSegments(nsReadSegmentFun reader, void * closure, PRUint32 count, PRUint32 *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute boolean nonBlocking; */
NS_IMETHODIMP nsPPMDecoder::GetNonBlocking(PRBool *aNonBlocking)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsPPMDecoder::SetNonBlocking(PRBool aNonBlocking)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute nsIOutputStreamObserver observer; */
NS_IMETHODIMP nsPPMDecoder::GetObserver(nsIOutputStreamObserver * *aObserver)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsPPMDecoder::SetObserver(nsIOutputStreamObserver * aObserver)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

