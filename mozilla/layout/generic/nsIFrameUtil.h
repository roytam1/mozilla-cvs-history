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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */
#ifndef nsIFrameUtil_h___
#define nsIFrameUtil_h___

#include "nsIXMLContent.h"
class nsIURI;

/* a6cf90d4-15b3-11d2-932e-00805f8add32 */
#define NS_IFRAME_UTIL_IID \
 { 0xa6cf90d6, 0x15b3, 0x11d2,{0x93, 0x2e, 0x00, 0x80, 0x5f, 0x8a, 0xdd, 0x32}}

/**
 * Frame utility interface
 */
class nsIFrameUtil : public nsISupports {
public:
  NS_DEFINE_STATIC_IID_ACCESSOR(NS_IFRAME_UTIL_IID)
  /**
   * Compare two regression data dumps. The return status will be NS_OK
   * if the trees compare favoribly, otherwise the return will indicate
   * NS_ERROR_FAILURE. Other return status's will indicate some other
   * type of failure. The files, aFile1 and aFile2 are closed before
   * returning. 
   * aRegressionOutput will vary output, 0 is full output, 1 is brief
   */
  NS_IMETHOD CompareRegressionData(FILE* aFile1, FILE* aFile2,PRInt32 aRegressionOuput) = 0;

  /**
   * Display the regression dump data stored in aInputFile1 to
   * aOutputFile . The file is closed before returning. If the
   * regression data is in error somehow then NS_ERROR_FAILURE will be
   * returned.
   */
  NS_IMETHOD DumpRegressionData(FILE* aInputFile, FILE* aOutputFile) = 0;
};

#endif /* nsIFrameUtil_h___ */
