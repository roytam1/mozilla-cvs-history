/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * CONFIDENTIAL AND PROPRIETARY SOURCE CODE
 * OF NETSCAPE COMMUNICATIONS CORPORATION
 *
 * Copyright (c) 2000 Netscape Communications Corporation.
 * All Rights Reserved.
 *
 * Use of this Source Code is subject to the terms of the applicable
 * license agreement from Netscape Communications Corporation.
 *
 * The copyright notice(s) in this Source Code does not indicate actual
 * or intended publication of this Source Code.
 */

#ifndef nsUniversalDetector_h__
#define nsUniversalDetector_h__

#include "nsISupports.h"
#include "nsICharsetDetector.h"
#include "nsIStringCharsetDetector.h"
#include "nsICharsetDetectionObserver.h"

#include "nsIFactory.h"

// {374E0CDE-F605-4259-8C92-E639C6C2EEEF}
#define NS_UNIVERSAL_DETECTOR_CID \
{ 0x374e0cde, 0xf605, 0x4259, { 0x8c, 0x92, 0xe6, 0x39, 0xc6, 0xc2, 0xee, 0xef } }

// {6EE5301A-3981-49bd-85F8-1A2CC228CF3E}
#define NS_UNIVERSAL_STRING_DETECTOR_CID \
{ 0x6ee5301a, 0x3981, 0x49bd, { 0x85, 0xf8, 0x1a, 0x2c, 0xc2, 0x28, 0xcf, 0x3e } }

class nsCharSetProber;

#define NUM_OF_CHARSET_PROBERS  2

typedef enum {
  ePureAscii = 0,
  eEscAscii  = 1,
  eHighbyte  = 2
} nsInputState;

class nsUniversalDetector {
public:
   nsUniversalDetector();
   virtual ~nsUniversalDetector();
   virtual void HandleData(const char* aBuf, PRUint32 aLen);
   virtual void DataEnd(void);

protected:
   virtual void Report(const char* aCharset) = 0;
   nsInputState  mInputState;
   PRBool  mAvailable;		//the model is not available for this instance
   PRBool  mDone;
   PRBool  mInTag;
   PRBool  mStart;
   PRBool  mGotData;
   char    mLastChar;
   const char *  mDetectedCharset;
   PRInt32 mBestGuess;

   nsCharSetProber  *mCharSetProbers[NUM_OF_CHARSET_PROBERS];
   nsCharSetProber  *mEscCharSetProber;
};

//=====================================================================
class nsUniversalXPCOMDetector :  
      public nsUniversalDetector,
      public nsICharsetDetector
{
  NS_DECL_ISUPPORTS
  public:
     nsUniversalXPCOMDetector();
     virtual ~nsUniversalXPCOMDetector();
     NS_IMETHOD Init(nsICharsetDetectionObserver* aObserver);
     NS_IMETHOD DoIt(const char* aBuf, PRUint32 aLen, PRBool *oDontFeedMe);
     NS_IMETHOD Done();
  protected:
     virtual void Report(const char* aCharset);
  private:
     nsICharsetDetectionObserver* mObserver;
};

//=====================================================================
class nsUniversalXPCOMStringDetector :  
      public nsUniversalDetector,
      public nsIStringCharsetDetector
{
  NS_DECL_ISUPPORTS
  public:
     nsUniversalXPCOMStringDetector();
     virtual ~nsUniversalXPCOMStringDetector();
     NS_IMETHOD DoIt(const char* aBuf, PRUint32 aLen, 
                     const char** oCharset, nsDetectionConfident &oConf);
  protected:
     virtual void Report(const char* aCharset);
  private:
     nsICharsetDetectionObserver* mObserver;
     const char* mResult;
};

#endif

