/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or 
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "nsToolkit.h"
#include "nsIWidget.h"
#include "nsGUIEvent.h"
#include "nsWidgetAtoms.h"

#include <Gestalt.h>
#include <Appearance.h>

#include "nsIEventSink.h"

#include "nsIEventQueue.h"
#include "nsIEventQueueService.h"
#include "nsIServiceManager.h"
#include "nsGfxCIID.h"
#include "nsIPref.h"


// Class IDs...
static NS_DEFINE_CID(kEventQueueCID,  NS_EVENTQUEUE_CID);
static NS_DEFINE_CID(kEventQueueServiceCID,  NS_EVENTQUEUESERVICE_CID);

static nsMacNSPREventQueueHandler*  gEventQueueHandler = nsnull;

//
// Static thread local storage index of the Toolkit 
// object associated with a given thread...
//
static PRUintn gToolkitTLSIndex = 0;


static CFBundleRef getBundle(CFStringRef frameworkPath)
{
  CFBundleRef bundle = NULL;
 
  //	Make a CFURLRef from the CFString representation of the bundle's path.
  //	See the Core Foundation URL Services chapter for details.
  CFURLRef bundleURL = CFURLCreateWithFileSystemPath(NULL, frameworkPath, kCFURLPOSIXPathStyle, true);
  if (bundleURL != NULL) {
    bundle = CFBundleCreate(NULL, bundleURL);
    if (bundle != NULL)
      CFBundleLoadExecutable(bundle);
    CFRelease(bundleURL);
  }

  return bundle;
}

static void* getQDFunction(CFStringRef functionName)
{
  static CFBundleRef systemBundle = getBundle(CFSTR("/System/Library/Frameworks/ApplicationServices.framework"));
  if (systemBundle)
    return CFBundleGetFunctionPointerForName(systemBundle, functionName);
  return NULL;
}

//-------------------------------------------------------------------------
//
//-------------------------------------------------------------------------
nsMacNSPREventQueueHandler::nsMacNSPREventQueueHandler(): Repeater()
{
  mRefCnt = 0;
  mEventQueueService = do_GetService(kEventQueueServiceCID);
}

//-------------------------------------------------------------------------
//
//-------------------------------------------------------------------------
nsMacNSPREventQueueHandler::~nsMacNSPREventQueueHandler()
{
  StopRepeating();
}

//-------------------------------------------------------------------------
//
//-------------------------------------------------------------------------
void nsMacNSPREventQueueHandler::StartPumping()
{
  ++mRefCnt;
  NS_LOG_ADDREF(this, mRefCnt, "nsMacNSPREventQueueHandler", sizeof(*this));
  
#if !TARGET_CARBON
  StartRepeating();
#endif
}

//-------------------------------------------------------------------------
//
//-------------------------------------------------------------------------
PRBool nsMacNSPREventQueueHandler::StopPumping()
{
  if (mRefCnt > 0) {
    --mRefCnt;
    NS_LOG_RELEASE(this, mRefCnt, "nsMacNSPREventQueueHandler");
    if (mRefCnt == 0) {
#if !TARGET_CARBON
      StopRepeating();
#endif
      return PR_TRUE;
    }
  }

  return PR_FALSE;
}

//-------------------------------------------------------------------------
//
//-------------------------------------------------------------------------
void nsMacNSPREventQueueHandler::RepeatAction(const EventRecord& inMacEvent)
{
  ProcessPLEventQueue();
}


//-------------------------------------------------------------------------
//
//-------------------------------------------------------------------------
PRBool nsMacNSPREventQueueHandler::EventsArePending()
{
  PRBool   pendingEvents = PR_FALSE;
  
  if (mEventQueueService)
  {
    nsCOMPtr<nsIEventQueue> queue;
    mEventQueueService->GetThreadEventQueue(NS_CURRENT_THREAD, getter_AddRefs(queue));

    if (queue)
      queue->PendingEvents(&pendingEvents);
  }

  return pendingEvents;
}


//-------------------------------------------------------------------------
//
//-------------------------------------------------------------------------
void nsMacNSPREventQueueHandler::ProcessPLEventQueue()
{
  NS_ASSERTION(mEventQueueService, "Need event queue service here");
  
  nsCOMPtr<nsIEventQueue> queue;
  mEventQueueService->GetThreadEventQueue(NS_CURRENT_THREAD, getter_AddRefs(queue));
  if (queue)
  {
    nsresult rv = queue->ProcessPendingEvents();
    NS_ASSERTION(NS_SUCCEEDED(rv), "Error processing PLEvents");
  }
}


#pragma mark -

NS_IMPL_THREADSAFE_ISUPPORTS1(nsToolkit, nsIToolkit);


// assume we begin as the fg app
bool nsToolkit::sInForeground = true;

static const char* gQuartzRenderingPref = "browser.quartz.enable";

//-------------------------------------------------------------------------
//
//-------------------------------------------------------------------------
nsToolkit::nsToolkit() : mInited(false)
{
  if (gEventQueueHandler == nsnull)
    gEventQueueHandler = new nsMacNSPREventQueueHandler;
}

//-------------------------------------------------------------------------
//
//-------------------------------------------------------------------------
nsToolkit::~nsToolkit()
{ 
  /* StopPumping decrements a refcount on gEventQueueHandler; a prelude toward
     stopping event handling. This is not something you want to do unless you've
     bloody well started event handling and incremented the refcount. That's
     done in the Init method, not the constructor, and that's what mInited is about.
  */
  if (mInited && gEventQueueHandler) {
    if (gEventQueueHandler->StopPumping()) {
      delete gEventQueueHandler;
      gEventQueueHandler = nsnull;
    }
  }
    // Remove the TLS reference to the toolkit...
    PR_SetThreadPrivate(gToolkitTLSIndex, nsnull);
}


//-------------------------------------------------------------------------
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsToolkit::Init(PRThread */*aThread*/)
{
  if (gEventQueueHandler)
    gEventQueueHandler->StartPumping();

  nsWidgetAtoms::RegisterAtoms();

  mInited = true;
  
#if TARGET_CARBON
  SetupQuartzRendering();
  nsCOMPtr<nsIPref> prefs = do_GetService(NS_PREF_CONTRACTID);
  if ( prefs )
    prefs->RegisterCallback(gQuartzRenderingPref, QuartzChangedCallback, nsnull);
#endif

  return NS_OK;
}

#if TARGET_CARBON

//
// QuartzChangedCallback
//
// The pref changed, reset the app to use quartz rendering as dictated by the pref
//
int nsToolkit::QuartzChangedCallback(const char* pref, void* data)
{
  SetupQuartzRendering();
  return NS_OK;
}


//
// SetupQuartzRendering
//
// Use apple's technote for 10.1.5 to turn on quartz rendering with CG metrics. This
// slows us down about 12% when turned on.
//
void nsToolkit::SetupQuartzRendering()
{
  // from Apple's technote, yet un-numbered.
#if UNIVERSAL_INTERFACES_VERSION <= 0x0400
  enum {
    kQDDontChangeFlags = 0xFFFFFFFF,         // don't change anything
    kQDUseDefaultTextRendering = 0,          // bit 0
    kQDUseTrueTypeScalerGlyphs = (1 << 0),   // bit 1
    kQDUseCGTextRendering = (1 << 1),        // bit 2
    kQDUseCGTextMetrics = (1 << 2)
  };
#endif
  const int kFlagsWeUse = kQDUseCGTextRendering | kQDUseCGTextMetrics;
  
  // turn on quartz rendering if we find the symbol in the app framework. Just turn
  // on the bits that we need, don't turn off what someone else might have wanted. If
  // the pref isn't found, assume we want it on. That way, we have to explicitly put
  // in a pref to disable it, rather than force everyone who wants it to carry around
  // an extra pref.
  typedef UInt32 (*qd_procptr)(UInt32);  
  static qd_procptr SwapQDTextFlags = (qd_procptr) getQDFunction(CFSTR("SwapQDTextFlags"));
  if ( SwapQDTextFlags ) {
    nsCOMPtr<nsIPref> prefs = do_GetService(NS_PREF_CONTRACTID);
    if (!prefs)
      return;
    PRBool enableQuartz = PR_TRUE;
    nsresult rv = prefs->GetBoolPref(gQuartzRenderingPref, &enableQuartz);
    UInt32 oldFlags = SwapQDTextFlags(kQDDontChangeFlags);
    if ( NS_FAILED(rv) || enableQuartz )
      SwapQDTextFlags(oldFlags | kFlagsWeUse);
    else 
      SwapQDTextFlags(oldFlags & !kFlagsWeUse);
  }
}
#endif


//-------------------------------------------------------------------------
//
//-------------------------------------------------------------------------
PRBool nsToolkit::ToolkitBusy()
{
  return (gEventQueueHandler) ? gEventQueueHandler->EventsArePending() : PR_FALSE;
}


//-------------------------------------------------------------------------
//
//-------------------------------------------------------------------------
bool nsToolkit::HasAppearanceManager()
{

#define APPEARANCE_MIN_VERSION  0x0110    // we require version 1.1
  
  static bool inited = false;
  static bool hasAppearanceManager = false;

  if (inited)
    return hasAppearanceManager;
  inited = true;

  SInt32 result;
  if (::Gestalt(gestaltAppearanceAttr, &result) != noErr)
    return false;   // no Appearance Mgr

  if (::Gestalt(gestaltAppearanceVersion, &result) != noErr)
    return false;   // still version 1.0

  hasAppearanceManager = (result >= APPEARANCE_MIN_VERSION);

  return hasAppearanceManager;
}


void 
nsToolkit :: AppInForeground ( )
{
  sInForeground = true;
}


void 
nsToolkit :: AppInBackground ( )
{
  sInForeground = false;
} 


bool
nsToolkit :: IsAppInForeground ( )
{
  return sInForeground;
}


//
// Return true if we are on Mac OS X, caching the result after the first call
//
bool
nsToolkit :: OnMacOSX()
{
  static PRBool gInitVer = PR_FALSE;
  static PRBool gOnMacOSX = PR_FALSE;
  if(! gInitVer) {
    long version;
    OSErr err = ::Gestalt(gestaltSystemVersion, &version);
    gOnMacOSX = (err == noErr && version >= 0x00001000);
    gInitVer = PR_TRUE;
  }
  return gOnMacOSX;
}


//
// GetTopWidget
//
// We've stashed the nsIWidget for the given windowPtr in the data 
// properties of the window. Fetch it.
//
void
nsToolkit::GetTopWidget ( WindowPtr aWindow, nsIWidget** outWidget )
{
  nsIWidget* topLevelWidget = nsnull;
	::GetWindowProperty ( aWindow, 'MOSS', 'GEKO', sizeof(nsIWidget*), nsnull, (void*)&topLevelWidget);
  if ( topLevelWidget ) {
    *outWidget = topLevelWidget;
    NS_ADDREF(*outWidget);
  }
}


//
// GetWindowEventSink
//
// We've stashed the nsIEventSink for the given windowPtr in the data 
// properties of the window. Fetch it.
//
void
nsToolkit::GetWindowEventSink ( WindowPtr aWindow, nsIEventSink** outSink )
{
  *outSink = nsnull;
  
  nsCOMPtr<nsIWidget> topWidget;
  GetTopWidget ( aWindow, getter_AddRefs(topWidget) );
  nsCOMPtr<nsIEventSink> sink ( do_QueryInterface(topWidget) );
  if ( sink ) {
    *outSink = sink;
    NS_ADDREF(*outSink);
  }
}


//-------------------------------------------------------------------------
//
// Return the nsIToolkit for the current thread.  If a toolkit does not
// yet exist, then one will be created...
//
//-------------------------------------------------------------------------
NS_METHOD NS_GetCurrentToolkit(nsIToolkit* *aResult)
{
  nsIToolkit* toolkit = nsnull;
  nsresult rv = NS_OK;
  PRStatus status;

  // Create the TLS index the first time through...
  if (0 == gToolkitTLSIndex) {
    status = PR_NewThreadPrivateIndex(&gToolkitTLSIndex, NULL);
    if (PR_FAILURE == status) {
      rv = NS_ERROR_FAILURE;
    }
  }

  if (NS_SUCCEEDED(rv)) {
    toolkit = (nsIToolkit*)PR_GetThreadPrivate(gToolkitTLSIndex);

    //
    // Create a new toolkit for this thread...
    //
    if (!toolkit) {
      toolkit = new nsToolkit();

      if (!toolkit) {
        rv = NS_ERROR_OUT_OF_MEMORY;
      } else {
        NS_ADDREF(toolkit);
        toolkit->Init(PR_GetCurrentThread());
        //
        // The reference stored in the TLS is weak.  It is removed in the
        // nsToolkit destructor...
        //
        PR_SetThreadPrivate(gToolkitTLSIndex, (void*)toolkit);
      }
    } else {
      NS_ADDREF(toolkit);
    }
    *aResult = toolkit;
  }

  return rv;
}

#pragma mark -

Handle nsMacMemoryCushion::sMemoryReserve;

nsMacMemoryCushion::nsMacMemoryCushion()
: mBufferHandle(nsnull)
{
}

nsMacMemoryCushion::~nsMacMemoryCushion()
{
  StopRepeating();
  ::SetGrowZone(nsnull);

  NS_ASSERTION(sMemoryReserve, "Memory reserve was nil");
  if (sMemoryReserve)
  {
    ::DisposeHandle(sMemoryReserve);
    sMemoryReserve = nil;
  }

  if (mBufferHandle)
    ::DisposeHandle(mBufferHandle);
}


OSErr nsMacMemoryCushion::Init(Size bufferSize, Size reserveSize)
{
  sMemoryReserve = ::NewHandle(reserveSize);
  if (sMemoryReserve == nsnull)
    return ::MemError();
  
  mBufferHandle = ::NewHandle(bufferSize);
  if (mBufferHandle == nsnull)
    return ::MemError();

  // make this purgable
  ::HPurge(mBufferHandle);

#if !TARGET_CARBON
  ::SetGrowZone(NewGrowZoneProc(GrowZoneProc));
#endif

  return noErr;
}


void nsMacMemoryCushion::RepeatAction(const EventRecord &aMacEvent)
{
  if (!RecoverMemoryReserve(kMemoryReserveSize))
  {
    // NS_ASSERTION(0, "Failed to recallocate memory reserve. Flushing caches");
    nsMemory::HeapMinimize(PR_TRUE);
  }
}


Boolean nsMacMemoryCushion::RecoverMemoryReserve(Size reserveSize)
{
  if (!sMemoryReserve) return true;     // not initted yet
  if (*sMemoryReserve != nsnull) return true;   // everything is OK
  
  ::ReallocateHandle(sMemoryReserve, reserveSize);
  if (::MemError() != noErr || !*sMemoryReserve) return false;
  return true;
}

Boolean nsMacMemoryCushion::RecoverMemoryBuffer(Size bufferSize)
{
  if (!mBufferHandle) return true;     // not initted yet
  if (*mBufferHandle != nsnull) return true;   // everything is OK
    
  ::ReallocateHandle(mBufferHandle, bufferSize);
  if (::MemError() != noErr || !*mBufferHandle) return false;

  // make this purgable
  ::HPurge(mBufferHandle);
  return true;
}

pascal long nsMacMemoryCushion::GrowZoneProc(Size amountNeeded)
{
  long    freedMem = 0;
  
  if (sMemoryReserve && *sMemoryReserve && sMemoryReserve != ::GZSaveHnd())
  {
    freedMem = ::GetHandleSize(sMemoryReserve);
    ::EmptyHandle(sMemoryReserve);
  }
  
  return freedMem;
}

std::auto_ptr<nsMacMemoryCushion>    gMemoryCushion;

Boolean nsMacMemoryCushion::EnsureMemoryCushion()
{
    if (!gMemoryCushion.get())
    {
        nsMacMemoryCushion* softFluffyCushion = new nsMacMemoryCushion();
        if (!softFluffyCushion) return false;
        
        OSErr   err = softFluffyCushion->Init(nsMacMemoryCushion::kMemoryBufferSize, nsMacMemoryCushion::kMemoryReserveSize);
        if (err != noErr)
        {
            delete softFluffyCushion;
            return false;
        }
        
        gMemoryCushion.reset(softFluffyCushion);
        softFluffyCushion->StartRepeating();
    }
    
    return true;
}

