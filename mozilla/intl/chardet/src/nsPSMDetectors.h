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
#ifndef nsPSMDetectors_h__
#define nsPSMDetectors_h__

#include "nsIFactory.h"
#include "nsVerifier.h"
//---- for verifiers
#include "nsSJISVerifier.h"
#include "nsEUCJPVerifier.h"
#include "nsCP1252Verifier.h"
#include "nsUTF8Verifier.h"
#include "nsISO2022JPVerifier.h"
#include "nsISO2022KRVerifier.h"
#include "nsISO2022CNVerifier.h"
#include "nsHZVerifier.h"
#include "nsUCS2BEVerifier.h"
#include "nsUCS2LEVerifier.h"
#include "nsBIG5Verifier.h"
#include "nsGB2312Verifier.h"
#include "nsEUCTWVerifier.h"
#include "nsEUCKRVerifier.h"
//---- end verifiers

#define DETECTOR_DEBUG

#define MAX_VERIFIERS 16

// {12BB8F1B-2389-11d3-B3BF-00805F8A6670}
#define NS_JA_PSMDETECTOR_CID \
{ 0x12bb8f1b, 0x2389, 0x11d3, { 0xb3, 0xbf, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70 } }

// {12BB8F1C-2389-11d3-B3BF-00805F8A6670}
#define NS_JA_STRING_PSMDETECTOR_CID \
{ 0x12bb8f1c, 0x2389, 0x11d3, { 0xb3, 0xbf, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70 } }

// {EA06D4E1-2B3D-11d3-B3BF-00805F8A6670}
#define NS_KO_PSMDETECTOR_CID \
{ 0xea06d4e1, 0x2b3d, 0x11d3, { 0xb3, 0xbf, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70 } }

// {EA06D4E2-2B3D-11d3-B3BF-00805F8A6670}
#define NS_ZHCN_PSMDETECTOR_CID \
{ 0xea06d4e2, 0x2b3d, 0x11d3, { 0xb3, 0xbf, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70 } }

// {EA06D4E3-2B3D-11d3-B3BF-00805F8A6670}
#define NS_ZHTW_PSMDETECTOR_CID \
{ 0xea06d4e3, 0x2b3d, 0x11d3, { 0xb3, 0xbf, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70 } }


// {EA06D4E4-2B3D-11d3-B3BF-00805F8A6670}
#define NS_KO_STRING_PSMDETECTOR_CID \
{ 0xea06d4e4, 0x2b3d, 0x11d3, { 0xb3, 0xbf, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70 } }

// {EA06D4E5-2B3D-11d3-B3BF-00805F8A6670}
#define NS_ZHCN_STRING_PSMDETECTOR_CID \
{ 0xea06d4e5, 0x2b3d, 0x11d3, { 0xb3, 0xbf, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70 } }

// {EA06D4E6-2B3D-11d3-B3BF-00805F8A6670}
#define NS_ZHTW_STRING_PSMDETECTOR_CID \
{ 0xea06d4e6, 0x2b3d, 0x11d3, { 0xb3, 0xbf, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70 } }


// {FCACEF21-2B40-11d3-B3BF-00805F8A6670}
#define NS_ZH_STRING_PSMDETECTOR_CID \
{ 0xfcacef21, 0x2b40, 0x11d3, { 0xb3, 0xbf, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70 } }

// {FCACEF22-2B40-11d3-B3BF-00805F8A6670}
#define NS_CJK_STRING_PSMDETECTOR_CID \
{ 0xfcacef22, 0x2b40, 0x11d3, { 0xb3, 0xbf, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70 } }


// {FCACEF23-2B40-11d3-B3BF-00805F8A6670}
#define NS_ZH_PSMDETECTOR_CID \
{ 0xfcacef23, 0x2b40, 0x11d3, { 0xb3, 0xbf, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70 } }

// {FCACEF24-2B40-11d3-B3BF-00805F8A6670}
#define NS_CJK_PSMDETECTOR_CID \
{ 0xfcacef24, 0x2b40, 0x11d3, { 0xb3, 0xbf, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70 } }

typedef struct {
  float mFirstByteFreq[94];
  float mFirstByteStdDev;
  float mFirstByteMean;
  float mFirstByteWeight;
  float mSecoundByteFreq[94];
  float mSecoundByteStdDev;
  float mSecoundByteMean;
  float mSecoundByteWeight;
} nsEUCStatistics;

static nsEUCStatistics gBig5Statistics = 
#include "Big5Statistics.h"
// end of UECTWStatistics.h include

static nsEUCStatistics gEUCTWStatistics = 
#include "EUCTWStatistics.h"
// end of UECTWStatistics.h include

static nsEUCStatistics gGB2312Statistics = 
#include "GB2312Statistics.h"
// end of GB2312Statistics.h include

static nsEUCStatistics gEUCJPStatistics = 
#include "EUCJPStatistics.h"
// end of EUCJPStatistics.h include

static nsEUCStatistics gEUCKRStatistics = 
#include "EUCKRStatistics.h"
// end of EUCKRStatistics.h include

//==========================================================
/*
   This class won't detect x-euc-tw for now. It can  only 
   tell a Big5 document is not x-euc-tw , but cannot tell
   a x-euc-tw docuement is not Big5 unless we hit characters
   defined in CNS 11643 plane 2.
   
   May need improvement ....
 */

#define ZHTW_DETECTOR_NUM_VERIFIERS 7

static nsVerifier *gZhTwVerifierSet[ZHTW_DETECTOR_NUM_VERIFIERS] = {
      &nsUTF8Verifier,
      &nsBIG5Verifier,
      &nsISO2022CNVerifier,
      &nsEUCTWVerifier,
      &nsCP1252Verifier,
      &nsUCS2BEVerifier,
      &nsUCS2LEVerifier
};

static nsEUCStatistics *gZhTwStatisticsSet[ZHTW_DETECTOR_NUM_VERIFIERS] = {
      nsnull,
      &gBig5Statistics,
      nsnull,
      &gEUCTWStatistics,
      nsnull,
      nsnull,
      nsnull
};

//==========================================================
#define KO_DETECTOR_NUM_VERIFIERS 6

static nsVerifier *gKoVerifierSet[KO_DETECTOR_NUM_VERIFIERS] = {
      &nsUTF8Verifier,
      &nsEUCKRVerifier,
      &nsISO2022KRVerifier,
      &nsCP1252Verifier,
      &nsUCS2BEVerifier,
      &nsUCS2LEVerifier
};

//==========================================================
#define ZHCN_DETECTOR_NUM_VERIFIERS 7

static nsVerifier *gZhCnVerifierSet[ZHCN_DETECTOR_NUM_VERIFIERS] = {
      &nsUTF8Verifier,
      &nsGB2312Verifier,
      &nsISO2022CNVerifier,
      &nsHZVerifier,
      &nsCP1252Verifier,
      &nsUCS2BEVerifier,
      &nsUCS2LEVerifier
};

//==========================================================
#define JA_DETECTOR_NUM_VERIFIERS 7

static nsVerifier *gJaVerifierSet[JA_DETECTOR_NUM_VERIFIERS] = {
      &nsUTF8Verifier,
      &nsSJISVerifier,
      &nsEUCJPVerifier,
      &nsISO2022JPVerifier,
      &nsCP1252Verifier,
      &nsUCS2BEVerifier,
      &nsUCS2LEVerifier
};

//==========================================================
#define ZH_DETECTOR_NUM_VERIFIERS 9

static nsVerifier *gZhVerifierSet[ZH_DETECTOR_NUM_VERIFIERS] = {
      &nsUTF8Verifier,
      &nsGB2312Verifier,
      &nsBIG5Verifier,
      &nsISO2022CNVerifier,
      &nsHZVerifier,
      &nsEUCTWVerifier,
      &nsCP1252Verifier,
      &nsUCS2BEVerifier,
      &nsUCS2LEVerifier
};

static nsEUCStatistics *gZhStatisticsSet[ZH_DETECTOR_NUM_VERIFIERS] = {
      nsnull,
      &gGB2312Statistics,
      &gBig5Statistics,
      nsnull,
      nsnull,
      &gEUCTWStatistics,
      nsnull,
      nsnull,
      nsnull
};

//==========================================================
#define CJK_DETECTOR_NUM_VERIFIERS 14

static nsVerifier *gCJKVerifierSet[CJK_DETECTOR_NUM_VERIFIERS] = {
      &nsUTF8Verifier,
      &nsSJISVerifier,
      &nsEUCJPVerifier,
      &nsISO2022JPVerifier,
      &nsEUCKRVerifier,
      &nsISO2022KRVerifier,
      &nsBIG5Verifier,
      &nsEUCTWVerifier,
      &nsGB2312Verifier,
      &nsISO2022CNVerifier,
      &nsHZVerifier,
      &nsCP1252Verifier,
      &nsUCS2BEVerifier,
      &nsUCS2LEVerifier
};

static nsEUCStatistics *gCJKStatisticsSet[CJK_DETECTOR_NUM_VERIFIERS] = {
      nsnull,
      nsnull,
      &gEUCJPStatistics,
      nsnull,
      &gEUCKRStatistics,
      nsnull,
      &gBig5Statistics,
      &gEUCTWStatistics,
      &gGB2312Statistics,
      nsnull,
      nsnull,
      nsnull,
      nsnull,
      nsnull
};

class nsEUCSampler {
  public:
    nsEUCSampler() {
      mTotal =0;
      mThreshold = 200;
	  mState = 0;
      PRInt32 i;
      for(i=0;i<94;i++)
          mFirstByteCnt[i] = mSecondByteCnt[i]=0;
    }
    PRBool EnoughData()  { return mTotal > mThreshold; }
    PRBool GetSomeData() { return mTotal > 1; }
    PRBool Sample(const char* aIn, PRUint32 aLen);
    void   CalFreq();
    float   GetScore(const float* aFirstByteFreq, float aFirstByteWeight,
                     const float* aSecondByteFreq, float aSecondByteWeight);
    float   GetScore(const float* array1, const float* array2);
  private:
    PRUint32 mTotal;
    PRUint32 mThreshold;
    PRInt8 mState;
    PRUint32 mFirstByteCnt[94];
    PRUint32 mSecondByteCnt[94];
    float mFirstByteFreq[94];
    float mSecondByteFreq[94];
   
};

/*
 In the current design, we know the following combination of verifiers 
 are not good-

 1. Two or more of the following verifier in one detector:
      nsEUCJPVerifer 
      nsGB2312Verifier
      nsEUCKRVerifer 
      nsEUCTWVerifier
      nsBIG5Verifer 

 */
//----------------------------------------------------------
class nsPSMDetector {
public :
   nsPSMDetector(PRUint8 aItems, nsVerifier** aVerifierSet, nsEUCStatistics** aStatisticsSet);
   virtual ~nsPSMDetector() {};

   virtual PRBool HandleData(const char* aBuf, PRUint32 aLen);
   virtual void   DataEnd();
 
protected:
   virtual void Report(const char* charset) = 0;

   PRUint8 mItems;
   PRUint8 mClassItems;
   PRUint8 mState[MAX_VERIFIERS];
   PRUint8 mItemIdx[MAX_VERIFIERS];
   nsVerifier** mVerifier;
   nsEUCStatistics** mStatisticsData;
   PRBool mDone;

   PRBool mRunSampler;
   PRBool mClassRunSampler;
protected:
   void Reset();
   void Sample(const char* aBuf, PRUint32 aLen, PRBool aLastChance=PR_FALSE);
private:
#ifdef DETECTOR_DEBUG
   PRUint32 mDbgTest;
   PRUint32 mDbgLen;
#endif
   nsEUCSampler mSampler;

};

class nsXPCOMDetector : 
      private nsPSMDetector,
      public nsICharsetDetector // Implement the interface 
{
  NS_DECL_ISUPPORTS
public:
    nsXPCOMDetector(PRUint8 aItems, nsVerifier** aVer, nsEUCStatistics** aStatisticsSet);
    virtual ~nsXPCOMDetector();
  NS_IMETHOD Init(nsICharsetDetectionObserver* aObserver);
  NS_IMETHOD DoIt(const char* aBuf, PRUint32 aLen, PRBool* oDontFeedMe);
  NS_IMETHOD Done();

protected:
  virtual void Report(const char* charset);

private:
  nsCOMPtr<nsICharsetDetectionObserver> mObserver;
};

class nsXPCOMStringDetector : 
      private nsPSMDetector,
      public nsIStringCharsetDetector // Implement the interface 
{
  NS_DECL_ISUPPORTS
public:
    nsXPCOMStringDetector(PRUint8 aItems, nsVerifier** aVer, nsEUCStatistics** aStatisticsSet);
    virtual ~nsXPCOMStringDetector();
    NS_IMETHOD DoIt(const char* aBuf, PRUint32 aLen, 
                   const char** oCharset, 
                   nsDetectionConfident &oConfident);
protected:
  virtual void Report(const char* charset);
private:
  const char* mResult;
};

class nsJAPSMDetector : public nsXPCOMDetector
{
public:
  nsJAPSMDetector() 
    : nsXPCOMDetector(JA_DETECTOR_NUM_VERIFIERS, gJaVerifierSet, nsnull) {};
};

class nsJAStringPSMDetector : public nsXPCOMStringDetector
{
public:
  nsJAStringPSMDetector() 
    : nsXPCOMStringDetector(JA_DETECTOR_NUM_VERIFIERS - 3, gJaVerifierSet, nsnull) {};
};

class nsKOPSMDetector : public nsXPCOMDetector
{
public:
  nsKOPSMDetector() 
    : nsXPCOMDetector(KO_DETECTOR_NUM_VERIFIERS, gKoVerifierSet, nsnull){};
};

class nsKOStringPSMDetector : public nsXPCOMStringDetector
{
public:
  nsKOStringPSMDetector() 
    : nsXPCOMStringDetector(KO_DETECTOR_NUM_VERIFIERS - 3, gKoVerifierSet, nsnull) {};
};

class nsZHTWPSMDetector : public nsXPCOMDetector
{
public:
  nsZHTWPSMDetector() 
    : nsXPCOMDetector(ZHTW_DETECTOR_NUM_VERIFIERS, gZhTwVerifierSet, gZhTwStatisticsSet) {};
};

class nsZHTWStringPSMDetector : public nsXPCOMStringDetector
{
public:
  nsZHTWStringPSMDetector() 
    : nsXPCOMStringDetector(ZHTW_DETECTOR_NUM_VERIFIERS - 3, gZhTwVerifierSet, gZhTwStatisticsSet) {};
};

class nsZHCNPSMDetector : public nsXPCOMDetector
{
public:
  nsZHCNPSMDetector() 
    : nsXPCOMDetector(ZHCN_DETECTOR_NUM_VERIFIERS, gZhCnVerifierSet, nsnull) {};
};

class nsZHCNStringPSMDetector : public nsXPCOMStringDetector
{
public:
  nsZHCNStringPSMDetector() 
    : nsXPCOMStringDetector(ZHCN_DETECTOR_NUM_VERIFIERS - 3, gZhCnVerifierSet, nsnull) {};
};

class nsZHPSMDetector : public nsXPCOMDetector
{
public:
  nsZHPSMDetector() 
    : nsXPCOMDetector(ZH_DETECTOR_NUM_VERIFIERS, gZhVerifierSet, gZhStatisticsSet) {};
};

class nsZHStringPSMDetector : public nsXPCOMStringDetector
{
public:
  nsZHStringPSMDetector() 
    : nsXPCOMStringDetector(ZH_DETECTOR_NUM_VERIFIERS - 3, gZhVerifierSet, gZhStatisticsSet) {};
};

class nsCJKPSMDetector : public nsXPCOMDetector
{
public:
  nsCJKPSMDetector() 
    : nsXPCOMDetector(CJK_DETECTOR_NUM_VERIFIERS, gCJKVerifierSet, gCJKStatisticsSet) {};
};

class nsCJKStringPSMDetector : public nsXPCOMStringDetector
{
public:
  nsCJKStringPSMDetector() 
    : nsXPCOMStringDetector(CJK_DETECTOR_NUM_VERIFIERS - 3, gCJKVerifierSet, gCJKStatisticsSet) {};
};

#endif // nsPSMDetectors_h__
