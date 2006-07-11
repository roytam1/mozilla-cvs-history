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
 * Portions created by the Initial Developer are Copyright (C) 2005
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

#include "zapUDPPacketizer.h"
#include "zapIMediaFrame.h"
#include "zapIMediaGraph.h"
#include "nsString.h"
#include "nsIPropertyBag2.h"
#include "zapDatagramFrame.h"
#include "nsHashPropertyBag.h"
#include "zapMediaUtils.h"

////////////////////////////////////////////////////////////////////////
// zapUDPPacketizer

zapUDPPacketizer::zapUDPPacketizer()
{
#ifdef DEBUG_afri_zmk
  printf("zapUDPPacketizer::zapUDPPacketizer()\n");
#endif
}

zapUDPPacketizer::~zapUDPPacketizer()
{
#ifdef DEBUG_afri_zmk
  printf("zapUDPPacketizer::~zapUDPPacketizer()\n");
#endif
}

NS_IMETHODIMP
zapUDPPacketizer::AddedToGraph(zapIMediaGraph *graph, const nsACString & id,
                               nsIPropertyBag2 *node_pars)
{
  // unpack node parameters:
  if (!node_pars) {
    NS_ERROR("no node parameters");
    return NS_ERROR_FAILURE;
  }

  NS_ENSURE_SUCCESS(node_pars->GetPropertyAsACString(
                      NS_LITERAL_STRING("address"), mAddress),
                    NS_ERROR_FAILURE);
  NS_ENSURE_SUCCESS(node_pars->GetPropertyAsUint16(
                      NS_LITERAL_STRING("port"), &mPort),
                    NS_ERROR_FAILURE);
  
  return NS_OK;
}

NS_IMETHODIMP
zapUDPPacketizer::RemovedFromGraph(zapIMediaGraph *graph)
{
  return NS_OK;
}

nsresult
zapUDPPacketizer::ValidateNewStream(nsIPropertyBag2* streamInfo)
{
  // We accept any stream, but we still need to feed streambreak
  // downstream:
  ZMK_CREATE_STREAM_INFO(mStreamInfo, "datagram");

  return NS_OK;
}

nsresult
zapUDPPacketizer::Filter(zapIMediaFrame* input, zapIMediaFrame** output)
{
  // create datagram frame:
  zapDatagramFrame* frame = new zapDatagramFrame();
  frame->AddRef();
  frame->mStreamInfo = mStreamInfo;
  input->GetData(frame->mData);
  frame->mAddress = mAddress;
  frame->mPort = mPort;
  *output = frame;
  return NS_OK;
}
