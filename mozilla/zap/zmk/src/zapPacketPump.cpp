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

#include "zapPacketPump.h"
#include "nsIPropertyBag2.h"
#include "stdio.h"
#include "zapIMediaFrame.h"
#include "prmem.h"
#include "nsAutoPtr.h"
#include "nsString.h"
#include "nsIComponentManager.h"

////////////////////////////////////////////////////////////////////////
// zapPacketPumpClock

class zapPacketPumpClock : public zapIMediaSink
{
public:
  zapPacketPumpClock();
  ~zapPacketPumpClock();

  void Init(zapPacketPump* pump);

  NS_DECL_ISUPPORTS
  NS_DECL_ZAPIMEDIASINK

private:
  nsRefPtr<zapPacketPump> mPump;
  nsCOMPtr<zapIMediaSource> mInput;
};

//----------------------------------------------------------------------

zapPacketPumpClock::zapPacketPumpClock()
{
}

zapPacketPumpClock::~zapPacketPumpClock()
{
  NS_ASSERTION(mPump, "Never initialized");
  mPump->mClock = nsnull;
}

void zapPacketPumpClock::Init(zapPacketPump* pump) {
  mPump = pump;
  mPump->mClock = this;
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_ADDREF(zapPacketPumpClock)
NS_IMPL_RELEASE(zapPacketPumpClock)

NS_INTERFACE_MAP_BEGIN(zapPacketPumpClock)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, zapIMediaSink)
  NS_INTERFACE_MAP_ENTRY(zapIMediaSink)
NS_INTERFACE_MAP_END

//----------------------------------------------------------------------
// zapIMediaSink methods:

/* void connectSource (in zapIMediaSource source); */
NS_IMETHODIMP
zapPacketPumpClock::ConnectSource(zapIMediaSource *source)
{
  NS_ASSERTION(!mInput, "already connected");
  mInput = source;
  
  return NS_OK;
}

/* void disconnectSource (in zapIMediaSource source); */
NS_IMETHODIMP
zapPacketPumpClock::DisconnectSource(zapIMediaSource *source)
{
  mInput = nsnull;
  return NS_OK;
}

/* void consumeFrame (in zapIMediaFrame frame); */
NS_IMETHODIMP
zapPacketPumpClock::ConsumeFrame(zapIMediaFrame * frame)
{
  if (!mPump) return NS_OK;

  return mPump->Pump();
}


////////////////////////////////////////////////////////////////////////
// zapPacketPump

zapPacketPump::zapPacketPump()
    : mClock(nsnull)
{
}

zapPacketPump::~zapPacketPump()
{
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_ADDREF(zapPacketPump)
NS_IMPL_RELEASE(zapPacketPump)

NS_INTERFACE_MAP_BEGIN(zapPacketPump)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, zapIMediaNode)
  NS_INTERFACE_MAP_ENTRY(zapIMediaNode)
  NS_INTERFACE_MAP_ENTRY(zapIMediaSink)
  NS_INTERFACE_MAP_ENTRY(zapIMediaSource)
  NS_INTERFACE_MAP_ENTRY(zapIPacketPump)
NS_INTERFACE_MAP_END

//----------------------------------------------------------------------
// zapIMediaNode methods:

/* void insertedIntoContainer (in zapIMediaNodeContainer container, in nsIPropertyBag2 node_pars); */
NS_IMETHODIMP
zapPacketPump::InsertedIntoContainer(zapIMediaNodeContainer *container,
                                     nsIPropertyBag2* node_pars)
{
  mContainer = container;

  return NS_OK;
}

/* void removedFromContainer (); */
NS_IMETHODIMP
zapPacketPump::RemovedFromContainer()
{
  return NS_OK;
}

/* zapIMediaSource getSource (in nsIPropertyBag2 source_pars); */
NS_IMETHODIMP
zapPacketPump::GetSource(nsIPropertyBag2 *source_pars,
                         zapIMediaSource **_retval)
{
  if (mOutput) {
    NS_ERROR("output end already connected");
    return NS_ERROR_FAILURE;
  }

  *_retval = this;
  NS_ADDREF(*_retval);
  return NS_OK;
}

/* zapIMediaSink getSink (in nsIPropertyBag2 sink_pars); */
NS_IMETHODIMP
zapPacketPump::GetSink(nsIPropertyBag2 *sink_pars, zapIMediaSink **_retval)
{
  if (!sink_pars) return NS_ERROR_FAILURE;

  nsCString sinkName;
  NS_ENSURE_SUCCESS(sink_pars->GetPropertyAsACString(NS_LITERAL_STRING("name"),
                                                     sinkName),
                    NS_ERROR_FAILURE);

  if (sinkName == NS_LITERAL_CSTRING("input")) {
    if (mInput) {
      NS_ERROR("input already connected");
      return NS_ERROR_FAILURE;
    }

    *_retval = this;
    NS_ADDREF(*_retval);
    return NS_OK;
  }
  else if (sinkName == NS_LITERAL_CSTRING("clock")) {
    if (mClock) {
      NS_ERROR("clock already connected");
      return NS_ERROR_FAILURE;
    }

    zapPacketPumpClock* clockSink = new zapPacketPumpClock();
    clockSink->AddRef();
    clockSink->Init(this);
    *_retval = clockSink;
    return NS_OK;
  }

  // ... else
  NS_ERROR("unknown sink");
  return NS_ERROR_FAILURE;
}

//----------------------------------------------------------------------
// zapIMediaSink methods:

/* void connectSource (in zapIMediaSource source); */
NS_IMETHODIMP
zapPacketPump::ConnectSource(zapIMediaSource *source)
{
  NS_ASSERTION(!mInput, "already connected");
  mInput = source;

  return NS_OK;
}

/* void disconnectSource (in zapIMediaSource source); */
NS_IMETHODIMP
zapPacketPump::DisconnectSource(zapIMediaSource *source)
{
  mInput = nsnull;

  return NS_OK;
}

/* void consumeFrame (in zapIMediaFrame frame); */
NS_IMETHODIMP
zapPacketPump::ConsumeFrame(zapIMediaFrame * frame)
{
  NS_ERROR("Not a passive sink - maybe you need some buffering?");
  return NS_ERROR_FAILURE;
}

//----------------------------------------------------------------------
// zapIMediaSource methods:

/* void connectSink (in zapIMediaSink sink); */
NS_IMETHODIMP
zapPacketPump::ConnectSink(zapIMediaSink *sink)
{
  if (mOutput) {
    NS_ERROR("output end already connected");
    return NS_ERROR_FAILURE;
  }
  mOutput = sink;
  return NS_OK;
}

/* void disconnectSink (in zapIMediaSink sink); */
NS_IMETHODIMP
zapPacketPump::DisconnectSink(zapIMediaSink *sink)
{
  mOutput = nsnull;
  return NS_OK;
}

/* zapIMediaFrame produceFrame (); */
NS_IMETHODIMP
zapPacketPump::ProduceFrame(zapIMediaFrame ** frame)
{
  NS_ERROR("Not a passive source - maybe you need some buffering?");
  *frame = nsnull;
  return NS_ERROR_FAILURE;
}

//----------------------------------------------------------------------
// zapIPacketPump implementation:

/* void pump (); */
NS_IMETHODIMP
zapPacketPump::Pump()
{
  if (!mInput || !mOutput) return NS_OK;

  nsRefPtr<zapPacketPump> deathGrip(this);

  nsCOMPtr<zapIMediaFrame> frame;
  if (NS_SUCCEEDED(mInput->ProduceFrame(getter_AddRefs(frame)))) {
    // the following check is important, since the graph might have
    // been reconfigured by the ProduceFrame() call:
    if (mOutput)
      mOutput->ConsumeFrame(frame);
  }
  
  return NS_OK;
}
