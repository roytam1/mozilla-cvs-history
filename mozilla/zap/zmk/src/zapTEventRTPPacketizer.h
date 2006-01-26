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

#ifndef __ZAP_TEVENTRTPPACKETIZER_H__
#define __ZAP_TEVENTRTPPACKETIZER_H__

#include "zapFilterNode.h"
#include "nsIWritablePropertyBag2.h"

////////////////////////////////////////////////////////////////////////
// zapTEventRTPPacketizer

// {0038F151-0F9C-44A8-909E-A135C80F4E3F}
#define ZAP_TEVENTRTPPACKETIZER_CID                             \
  { 0x0038f151, 0x0f9c, 0x44a8, { 0x90, 0x9e, 0xa1, 0x35, 0xc8, 0x0f, 0x4e, 0x3f } }

#define ZAP_TEVENTRTPPACKETIZER_CONTRACTID ZAP_MEDIANODE_CONTRACTID_PREFIX "tevent-rtp-packetizer"

class zapTEventRTPPacketizer : public zapFilterNode
{
public:
  zapTEventRTPPacketizer();
  ~zapTEventRTPPacketizer();
  
  NS_IMETHOD AddedToGraph(zapIMediaGraph *graph,
                          const nsACString & id,
                          nsIPropertyBag2 *node_pars);
  NS_IMETHOD RemovedFromGraph(zapIMediaGraph *graph);
  virtual nsresult ValidateNewStream(nsIPropertyBag2* streamInfo);
  virtual nsresult Filter(zapIMediaFrame* input, zapIMediaFrame** output);

private:
  nsCOMPtr<nsIWritablePropertyBag2> mStreamInfo;
  PRUint16 mPayloadType;
};

#endif // __ZAP_TEVENTRTPPACKETIZER_H__
