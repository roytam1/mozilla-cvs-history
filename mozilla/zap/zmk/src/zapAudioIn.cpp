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

#include "zapAudioIn.h"
#include "stdio.h"
#include "nsStringAPI.h"
#include "zapMediaFrame.h"
#include "zapAudioDevice.h"
#include "nsIProxyObjectManager.h"
#include "nsAutoLock.h"
#include "nsThreadUtils.h"

///////////////////////////////////////////////////////////////////////
// zapAudioInEvent

class zapAudioInEvent : public nsRunnable
{
public:
  zapAudioInEvent(zapAudioIn* audioin,
                  const nsACString &data,
                  double timestamp)
      : mAudioIn(audioin),
        mData(data),
        mTimestamp(timestamp)
  {
  }

  NS_IMETHODIMP Run()
  {
    if (!mAudioIn) return NS_OK;
    
    mAudioIn->CreateFrame(mData, mTimestamp);
    return NS_OK;
  }
  
private:
  nsRefPtr<zapAudioIn> mAudioIn; 
  nsCString mData;
  double mTimestamp;
};

////////////////////////////////////////////////////////////////////////
// zapAudioIn

zapAudioIn::zapAudioIn()
    : mStream(nsnull)
{
  PaError err;
  if ((err = Pa_Initialize()) != paNoError) {
    NS_WARNING("Failed to initialize portaudio");
  }  
}

zapAudioIn::~zapAudioIn()
{
  NS_ASSERTION(!mStream, "stream still running");

  Pa_Terminate();
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_THREADSAFE_ADDREF(zapAudioIn)
NS_IMPL_THREADSAFE_RELEASE(zapAudioIn)

NS_INTERFACE_MAP_BEGIN(zapAudioIn)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, zapIMediaNode)
  NS_INTERFACE_MAP_ENTRY(zapIMediaNode)
  NS_INTERFACE_MAP_ENTRY(zapIMediaSource)
  NS_INTERFACE_MAP_ENTRY(zapIAudioIn)
NS_INTERFACE_MAP_END

//----------------------------------------------------------------------
// zapIMediaNode methods:

/* void insertedIntoContainer (in zapIMediaNodeContainer container, in nsIPropertyBag2 node_pars); */
NS_IMETHODIMP
zapAudioIn::InsertedIntoContainer(zapIMediaNodeContainer *container,
                                  nsIPropertyBag2* node_pars)
{
  //XXX needed to prevent premature destruction of the container thread
  mContainer = container;
  
  container->GetEventTarget(getter_AddRefs(mEventTarget));
  
  // node parameter defaults:
  mInputDevice = Pa_GetDefaultInputDeviceID();
  mBuffers = 4;
  // unpack node parameters:
  if (node_pars) {
    nsCOMPtr<zapIPortaudioDevice> device;
    node_pars->GetPropertyAsInterface(NS_LITERAL_STRING("device"),
                                      NS_GET_IID(zapIPortaudioDevice),
                                      getter_AddRefs(device));
    if (device) {
      device->GetDeviceID(&mInputDevice);
    }
    
    node_pars->GetPropertyAsUint32(NS_LITERAL_STRING("buffers"),
                                   &mBuffers);
  }
  
  if (mBuffers < 2) mBuffers = 2;
  
  if (NS_FAILED(mStreamParameters.InitWithProperties(node_pars)))
    return NS_ERROR_FAILURE;

  // XXX shall we start playing now or later??
  Play();
  
  return NS_OK;
}

/* void removedFromContainer (); */
NS_IMETHODIMP
zapAudioIn::RemovedFromContainer()
{
  Stop();
  mEventTarget = nsnull;
  return NS_OK;
}

/* zapIMediaSource getSource (in nsIPropertyBag2 source_pars); */
NS_IMETHODIMP
zapAudioIn::GetSource(nsIPropertyBag2 *source_pars, zapIMediaSource **_retval)
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
zapAudioIn::GetSink(nsIPropertyBag2 *sink_pars, zapIMediaSink **_retval)
{
  NS_ERROR("audioin is a source-only node");
  return NS_ERROR_FAILURE;
}

//----------------------------------------------------------------------
// zapIMediaSource methods:

/* void connectSink (in zapIMediaSink sink); */
NS_IMETHODIMP
zapAudioIn::ConnectSink(zapIMediaSink *sink)
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
zapAudioIn::DisconnectSink(zapIMediaSink *sink)
{
  mOutput = nsnull;
  return NS_OK;
}

/* zapIMediaFrame produceFrame (); */
NS_IMETHODIMP
zapAudioIn::ProduceFrame(zapIMediaFrame ** frame)
{
  NS_ERROR("Not a passive source - maybe you need some buffering?");
  *frame = nsnull;
  return NS_ERROR_FAILURE;
}

//----------------------------------------------------------------------
// zapIAudioIn methods:

/* void play (); */
NS_IMETHODIMP
zapAudioIn::Play()
{
  if (!mEventTarget)
    return NS_ERROR_FAILURE;
  
  if (!mStream)
    return StartStream();
  return NS_OK;
}

/* void stop (); */
NS_IMETHODIMP
zapAudioIn::Stop()
{
  if (mStream)
    CloseStream();
  return NS_OK;
}

/* readonly attribute zapIAudioDevice defaultInputDevice; */
NS_IMETHODIMP
zapAudioIn::GetDefaultInputDevice(zapIAudioDevice * *aDefaultInputDevice)
{
  PaDeviceID id = Pa_GetDefaultInputDeviceID();
  if (id == paNoDevice) {
    *aDefaultInputDevice = nsnull;
    return NS_ERROR_FAILURE;
  }

  *aDefaultInputDevice = CreateAudioDevice(id);
  return NS_OK;
}

//----------------------------------------------------------------------
// portaudio audio source callback:

int AudioInCallback(void* inputBuffer, void* outputBuffer,
                    unsigned long framesPerBuffer,
                    PaTimestamp outTime, void* userData)
{
  zapAudioIn* audioin = (zapAudioIn*)userData;
  
  // Asynchronously post the frame.
  nsCString data;
  PRUint32 bufferLength = 
      audioin->mStreamParameters.samples *
      audioin->mStreamParameters.channels *
      GetPortAudioSampleSize(audioin->mStreamParameters.sample_format);
  data.SetLength(bufferLength);
  memcpy(data.BeginWriting(), inputBuffer, bufferLength);

  nsCOMPtr<nsIRunnable> ev = new zapAudioInEvent(audioin, data, outTime);
    
  audioin->mEventTarget->Dispatch(ev, NS_DISPATCH_NORMAL);
  
  return 0;
}

//----------------------------------------------------------------------
// Implementation helpers:

nsresult zapAudioIn::StartStream()
{
  NS_ASSERTION(!mStream, "stream still running");

  // Create a new streaminfo. We can't just implement nsIPropertyBag2
  // on zapAudioIn, because packets and their associated streaminfo
  // might be buffered elsewhere in the graph while we are already
  // busy on the next stream.
  mStreamInfo = mStreamParameters.CreateStreamInfo();
    
  PaError err = Pa_OpenStream(&mStream,
                              mInputDevice, mStreamParameters.channels,
                              ZapAudioSampleFormatToPaFormat(mStreamParameters.sample_format),
                              nsnull,
                              paNoDevice, 0, 0, nsnull,
                              mStreamParameters.sample_rate,
                              mStreamParameters.samples,
                              mBuffers,
                              paNoFlag,
                              AudioInCallback, this);

  if (err != paNoError) {
    NS_WARNING("Failed to open portaudio input stream");
    return NS_ERROR_FAILURE;
  }

  Pa_StartStream(mStream);
  return NS_OK;
}

void zapAudioIn::CloseStream()
{
  NS_ASSERTION(mStream, "stream not running");

  Pa_CloseStream(mStream);
  mStream = nsnull;
}

void zapAudioIn::CreateFrame(const nsACString& data, double timestamp)
{
  if (!mOutput) return;
  
  // Create frame:
  nsRefPtr<zapMediaFrame> frame = new zapMediaFrame();

  if (mStreamParameters.sample_format == sf_float32_32768) {
    // convert from int16 to float:
    PRUint32 samplesPerFrame = mStreamParameters.samples * mStreamParameters.channels;
    frame->mData.SetLength( samplesPerFrame * GetZapAudioSampleSize(mStreamParameters.sample_format));
    float* d = (float*)frame->mData.BeginWriting();
    const PRInt16* s = (const PRInt16*)data.BeginReading();
    for (PRUint32 i=0; i<samplesPerFrame; ++i)
      *d++ = (float)*s++;
  }
  else {
    // portaudio format maps directly onto zmk audio stream format
    frame->mData = data;
  }
  frame->mStreamInfo = mStreamInfo;
  frame->mTimestamp = (PRUint64)timestamp;

  mOutput->ConsumeFrame(frame);
}


