/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
#include <stdio.h>
#include "prmem.h"

#include "nsMBCSGroupProber.h"

#ifdef DEBUG_chardet
char *ProberName[] = 
{
  "UTF8",
  "SJIS",
  "EUCJP",
  "GB2312",
  "EUCKR",
  "Big5",
  "EUCTW",
};

#endif

nsMBCSGroupProber::nsMBCSGroupProber()
{
  mProbers[0] = new nsUTF8Prober();
  mProbers[1] = new nsSJISProber();
  mProbers[2] = new nsEUCJPProber();
  mProbers[3] = new nsGB2312Prober();
  mProbers[4] = new nsEUCKRProber();
  mProbers[5] = new nsBig5Prober();
  mProbers[6] = new nsEUCTWProber();
  Reset();
}

nsMBCSGroupProber::~nsMBCSGroupProber()
{
  for (PRUint32 i = 0; i < NUM_OF_PROBERS; i++)
  {
    delete mProbers[i];
  }
}

const char* nsMBCSGroupProber::GetCharSetName()
{
  if (mBestGuess == -1)
  {
    GetConfidence();
    if (mBestGuess == -1)
      mBestGuess = 0;
  }
  return mProbers[mBestGuess]->GetCharSetName();
}

void  nsMBCSGroupProber::Reset(void)
{
  for (PRUint32 i = 0; i < NUM_OF_PROBERS; i++)
  {
    mProbers[i]->Reset();
    mIsActive[i] = PR_TRUE;
  }
  mActiveNum = NUM_OF_PROBERS;
  mBestGuess = -1;
  mState = eDetecting;
}

nsProbingState nsMBCSGroupProber::HandleData(const char* aBuf, PRUint32 aLen)
{
  nsProbingState st;
  PRUint32 i;

  //do filtering to reduce load to probers
  char *highbyteBuf;
  char *hptr;
  PRBool keepNext = PR_TRUE;   //assume previous is not ascii, it will do not harm except add some noise
  hptr = highbyteBuf = (char*)PR_MALLOC(aLen);
  for (i = 0; i < aLen; i++)
  {
    if (aBuf[i] & 0x80)
    {
      *hptr++ = aBuf[i];
      keepNext = PR_TRUE;
    }
    else
    {
      //if previous is highbyte, keep this even it is a ASCII
      if (keepNext)
      {
          *hptr++ = aBuf[i];
          keepNext = PR_FALSE;
      }
    }
  }

  for (i = 0; i < NUM_OF_PROBERS; i++)
  {
     if (!mIsActive[i])
       continue;
     st = mProbers[i]->HandleData(highbyteBuf, hptr - highbyteBuf);
     if (st == eFoundIt)
     {
       mBestGuess = i;
       mState = eFoundIt;
#ifdef DEBUG_chardet
       printf("MBCS Prober found charset %d in HandleData. \r\n", i);
#endif
       break;
     }
     else if (st == eNotMe)
     {
       mIsActive[i] = PR_FALSE;
       mActiveNum--;
       if (mActiveNum <= 0)
       {
         mState = eNotMe;
         break;
       }
     }
  }

  PR_FREEIF(highbyteBuf);

  return mState;
}

float nsMBCSGroupProber::GetConfidence(void)
{
  PRUint32 i;
  float bestConf = 0.0, cf;

  switch (mState)
  {
  case eFoundIt:
    return (float)0.99;
  case eNotMe:
    return (float)0.01;
  default:
    for (i = 0; i < NUM_OF_PROBERS; i++)
    {
      if (!mIsActive[i])
        continue;
      cf = mProbers[i]->GetConfidence();
      if (bestConf < cf)
      {
        bestConf = cf;
        mBestGuess = i;
      }
    }
  }
#ifdef DEBUG_chardet
       printf("MBCS Prober confidence is %f in charset %d . \r\n", bestConf, mBestGuess);
       for (i = 0; i < NUM_OF_PROBERS; i++)
       {
          if (!mIsActive[i])
            printf("[%s] is inactive\r\n", ProberName[i], i);
          else
          {
            cf = mProbers[i]->GetConfidence();
            printf("[%s] detector has confidence %f\r\n", ProberName[i], cf);
          }
       }
#endif
  return bestConf;
}

