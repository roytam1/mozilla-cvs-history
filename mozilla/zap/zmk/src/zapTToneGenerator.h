/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Mozilla SIP client project.
 *
 * The Initial Developer of the Original Code is 8x8 Inc.
 * Portions created by the Initial Developer are Copyright (C) 2006
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Alex Fritze <alex@croczilla.com> (original author)
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef __ZAP_TTONEGENERATOR_H__
#define __ZAP_TTONEGENERATOR_H__

#include "zapIMediaNode.h"
#include "zapIMediaSource.h"
#include "zapIMediaSink.h"
#include "zapIMediaFrame.h"
#include "nsCOMPtr.h"
#include "nsIWritablePropertyBag2.h"
#include "zapITToneGenerator.h"
#include "nsDeque.h"

////////////////////////////////////////////////////////////////////////
// zapTToneGenerator

// {E239A6DF-C6D2-4B17-B2C6-C013D3BEEA8D}
#define ZAP_TTONEGENERATOR_CID \
  { 0xe239a6df, 0xc6d2, 0x4b17, { 0xb2, 0xc6, 0xc0, 0x13, 0xd3, 0xbe, 0xea, 0x8d } }

#define ZAP_TTONEGENERATOR_CONTRACTID ZAP_MEDIANODE_CONTRACTID_PREFIX "ttone-generator"

class zapTToneGenerator : public zapIMediaNode,
                          public zapIMediaSource,
                          public zapITToneGenerator
{
public:
  zapTToneGenerator();
  ~zapTToneGenerator();
  
  NS_DECL_ISUPPORTS
  NS_DECL_ZAPIMEDIANODE
  NS_DECL_ZAPIMEDIASOURCE
  NS_DECL_ZAPITTONEGENERATOR

private:
  void ClearState();
  nsresult ParseTonesData(const char *buf);
  nsresult ParseTone(const char **bufp);
  
  PRUint16 mMaxSamplesPerFrame;

  nsDeque mBuffer; // tone buffer
  PRUint32 mToneSamplesPlayed; // samples played for current tone
  PRBool mLoop; // true: loop the buffer
  PRUint64 mSampleClock;
  
  nsCOMPtr<nsIWritablePropertyBag2> mStreamInfo;

  nsCOMPtr<zapIMediaSink> mOutput;
};

#endif // __ZAP_TTONEGENERATOR_H__
