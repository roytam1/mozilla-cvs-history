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
 * The Original Code is Mozilla Communicator client code.
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
#include "nsNativeAppSupport.h"
#include "nsString.h"

#include <Gestalt.h>
#include <Dialogs.h>
#include <Resources.h>
#include <TextUtils.h>
#include <ControlDefinitions.h>

#include "nsAppShellCIDs.h"
#include "nsCOMPtr.h"
#include "nsCommandLineServiceMac.h"
#include "nsNativeAppSupportBase.h"

#include "nsIAppShellService.h"
#include "nsIBaseWindow.h"
#include "nsICmdLineService.h"
#include "nsIDOMWindowInternal.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDocShellTreeOwner.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIObserver.h"
#include "nsIServiceManager.h"
#include "nsIWebNavigation.h"
#include "nsIWidget.h"
#include "nsIWindowMediator.h"

static NS_DEFINE_CID(kCmdLineServiceCID,    NS_COMMANDLINE_SERVICE_CID);
static NS_DEFINE_CID(kAppShellServiceCID,   NS_APPSHELL_SERVICE_CID);

#define rSplashDialog 512

static Boolean VersGreaterThan4(const FSSpec *fSpec);

const OSType kNSCreator = 'MOSS';
const OSType kMozCreator = 'MOZZ';
const SInt16 kNSCanRunStrArrayID = 1000;
const SInt16 kAnotherVersionStrIndex = 1;

nsresult
GetNativeWindowPointerFromDOMWindow(nsIDOMWindowInternal *window, WindowRef *nativeWindow);

const SInt16 kNSOSVersErrsStrArrayID = 1001;

enum {
        eOSXVersTooOldErrIndex = 1,
        eOSXVersTooOldExplanationIndex,
        eContinueButtonTextIndex,
        eQuitButtonTextIndex,
        eCarbonLibVersTooOldIndex,
        eCarbonLibVersTooOldExplanationIndex
     };

class nsNativeAppSupportMac : public nsNativeAppSupportBase,
                              public nsIObserver
{
public:

    // dialog items
    enum {
      eSplashPictureItem = 1,
      eSplashStatusTextItem    
    };
    
            nsNativeAppSupportMac();    
    virtual ~nsNativeAppSupportMac();

    NS_DECL_ISUPPORTS
    NS_DECL_NSINATIVEAPPSUPPORT
    NS_DECL_NSIOBSERVER

protected:

    DialogPtr mDialog;

}; // class nsSplashScreenMac


nsNativeAppSupportMac::nsNativeAppSupportMac()
: mDialog(nsnull)
{
  NS_INIT_ISUPPORTS();
}


nsNativeAppSupportMac::~nsNativeAppSupportMac()
{
  HideSplashScreen();
}

NS_IMPL_ISUPPORTS2(nsNativeAppSupportMac, nsINativeAppSupport, nsIObserver);

/* boolean start (); */
NS_IMETHODIMP nsNativeAppSupportMac::Start(PRBool *_retval)
{
  Str255 str1;
  Str255 str2;
  SInt16 outItemHit;
  long   response = 0;
  OSErr  err = ::Gestalt (gestaltSystemVersion, &response);
  // check for at least MacOS 8.5
  if ( err || response < 0x850)
  {
    ::StopAlert (5000, NULL);
    *_retval = PR_FALSE;
    return NS_ERROR_FAILURE;
  }
  
#if TARGET_CARBON
  // If we're running under Mac OS X check for at least Mac OS X 10.1
  // If that fails display a StandardAlert giving the user the option
  // to continue running the app or quitting
  if (response >= 0x00001000 && response < 0x00001010)
  {
    // put up error dialog
    Str255 continueButtonLabel;
    Str255 quitButtonLabel;
    ::GetIndString(str1, kNSOSVersErrsStrArrayID, eOSXVersTooOldErrIndex);
    ::GetIndString(str2, kNSOSVersErrsStrArrayID, eOSXVersTooOldExplanationIndex);
    ::GetIndString(continueButtonLabel, kNSOSVersErrsStrArrayID, eContinueButtonTextIndex);
    ::GetIndString(quitButtonLabel, kNSOSVersErrsStrArrayID, eQuitButtonTextIndex);
    if (StrLength(str1) && StrLength(str1) && StrLength(continueButtonLabel) && StrLength(quitButtonLabel))
    {
      AlertStdAlertParamRec pRec;
      
      pRec.movable      = nil;
      pRec.filterProc 	= nil;
      pRec.defaultText  = continueButtonLabel;
      pRec.cancelText   = quitButtonLabel;
      pRec.otherText    = nil;
      pRec.helpButton   = nil;
      pRec.defaultButton = kAlertStdAlertOKButton;
      pRec.cancelButton  = kAlertStdAlertCancelButton;
      pRec.position      = 0;
      
      ::StandardAlert(kAlertNoteAlert, str1, str2, &pRec, &outItemHit);
      if (outItemHit == kAlertStdAlertCancelButton)
        return PR_FALSE;
    }
    else
      return PR_FALSE;
  }
  
  // We also check for CarbonLib version >= 1.4 if OS vers < 10.0
  // which is always cause for the app to quit
  if (response < 0x00001000)
  {
    err = ::Gestalt (gestaltCarbonVersion, &response);
    if (err || response < 0x00000140)
    {
      // put up error dialog
      ::GetIndString(str1, kNSOSVersErrsStrArrayID, eCarbonLibVersTooOldIndex);
      ::GetIndString(str2, kNSOSVersErrsStrArrayID, eCarbonLibVersTooOldExplanationIndex);
      if (StrLength(str1) && StrLength(str1))
      {
        ::StandardAlert(kAlertStopAlert, str1, str2, nil, &outItemHit);
      }
      return PR_FALSE;
    }
  }
#endif

  // Check for running instances of Mozilla or Netscape. The real issue
  // is having more than one app use the same profile directory. That would
  // be REAL BAD!!!!!!!!

  // The code below is a copy of nsLocalFile::FindRunningAppBySignature which is
  // a non-static method. Sounds silly that I have to create an nsILocalFile
  // just to call that method. At any rate, don't think we want to go through
  // all that rigmarole during startup anyway.

  ProcessInfoRec  info;
  FSSpec          tempFSSpec;
  ProcessSerialNumber psn, nextProcessPsn;

  nextProcessPsn.highLongOfPSN = 0;
  nextProcessPsn.lowLongOfPSN  = kNoProcess;

  // first, get our psn so that we can exclude ourselves when searching
  err = ::GetCurrentProcess(&psn);

  if (err != noErr)
  {
    *_retval = PR_FALSE;
    return NS_ERROR_FAILURE;
  }

  // We loop while err == noErr, which should mean all our calls are OK
  // The ways of 'break'-ing out of the loop are:
  //   GetNextProcess() fails (this includes getting procNotFound, meaning we're at the end of the process list),
  //   GetProcessInformation() fails
  // The ways we should fall out of the while loop are:
  //   GetIndString() fails, err == resNotFound,
  //   we found a running mozilla process or running Netscape > 4.x process, err == fnfErr
  while (err == noErr)
  {
    err = ::GetNextProcess(&nextProcessPsn);
    if (err != noErr)
      break; // most likely, end of process list
    info.processInfoLength = sizeof(ProcessInfoRec);
    info.processName = nil;
    info.processAppSpec = &tempFSSpec;
    err = ::GetProcessInformation(&nextProcessPsn, &info);
    if (err != noErr)
      break; // aww crap, GetProcessInfo() failed, we're outta here

    if (info.processSignature == kNSCreator || info.processSignature == kMozCreator)
    {
      // if the found process is us, obviously, it's okay if WE'RE running,
      if (!(info.processNumber.lowLongOfPSN == psn.lowLongOfPSN &&
            info.processNumber.highLongOfPSN == psn.highLongOfPSN))
      {
        // check to see if Netscape process is greater than Netscape 4.x or
        // if process is Mozilla
        if ((info.processSignature == kNSCreator && VersGreaterThan4(&tempFSSpec)) ||
             info.processSignature == kMozCreator)
        {
          // put up error dialog
          Str255 str;
          ::GetIndString(str, kNSCanRunStrArrayID, kAnotherVersionStrIndex);
          if (StrLength(str) == 0)
            err = resNotFound; // set err to something so that we return false
          else
          {
            SInt16 outItemHit;
            if (str)
              ::StandardAlert(kAlertStopAlert, str, nil, nil, &outItemHit);
            err = fnfErr; // set err to something so that we return false
          }
        }
      }
    }
  }

  if (err == noErr || err == procNotFound)
  {
    *_retval = PR_TRUE;
    return NS_OK;
  }
  
  *_retval = PR_FALSE;
  return NS_ERROR_FAILURE;
}

/* boolean stop (); */
NS_IMETHODIMP nsNativeAppSupportMac::Stop(PRBool *_retval)
{
  *_retval = PR_TRUE;
  return NS_OK;
}

/* void quit (); */
NS_IMETHODIMP nsNativeAppSupportMac::Quit()
{
  return NS_OK;
}

/* [noscript] void ensureProfile (in nsICmdLineService aCmdService); */
NS_IMETHODIMP nsNativeAppSupportMac::EnsureProfile(nsICmdLineService *aCmdService)
{
  return NS_OK;
}

/* void showSplashScreen (); */
NS_IMETHODIMP nsNativeAppSupportMac::ShowSplashScreen()
{
  mDialog = ::GetNewDialog(rSplashDialog, nil, (WindowPtr)-1L);
  if (!mDialog) return NS_ERROR_FAILURE;

#if TARGET_CARBON
  ::ShowWindow(GetDialogWindow(mDialog));
  ::SetPortDialogPort(mDialog);
#else 
  ::ShowWindow(mDialog);
  ::SetPort(mDialog);
#endif

  ::DrawDialog(mDialog);    // we don't handle events for this dialog, so we
                            // need to draw explicitly. Yuck.
  return NS_OK;
}

/* void hideSplashScreen (); */
NS_IMETHODIMP nsNativeAppSupportMac::HideSplashScreen()
{
  if (mDialog)
  {
    ::DisposeDialog( mDialog );
    mDialog = nsnull;
  }
  return NS_OK;
}

/* attribute boolean isServerMode; */
NS_IMETHODIMP nsNativeAppSupportMac::GetIsServerMode(PRBool *aIsServerMode)
{
  *aIsServerMode = PR_FALSE;
  return NS_OK;;
}
NS_IMETHODIMP nsNativeAppSupportMac::SetIsServerMode(PRBool aIsServerMode)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute boolean shouldShowUI; */
NS_IMETHODIMP nsNativeAppSupportMac::GetShouldShowUI(PRBool *aShouldShowUI)
{
  *aShouldShowUI = PR_TRUE;
  return NS_OK;;
}

NS_IMETHODIMP nsNativeAppSupportMac::SetShouldShowUI(PRBool aShouldShowUI)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* void startServerMode (); */
NS_IMETHODIMP nsNativeAppSupportMac::StartServerMode()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* void onLastWindowClosing (in nsIXULWindow aWindow); */
NS_IMETHODIMP nsNativeAppSupportMac::OnLastWindowClosing()
{
  return NS_OK;
}

NS_IMETHODIMP
nsNativeAppSupportMac::Observe(nsISupports *aSubject, const char *aTopic, const PRUnichar *someData)
{
  // update a string in the dialog
  
  nsCAutoString statusString;
  statusString.AssignWithConversion(someData);

  ControlHandle   staticTextControl;
  OSErr err = ::GetDialogItemAsControl(mDialog, eSplashStatusTextItem, &staticTextControl);
  if (err != noErr) return NS_OK;
  
  PRInt32   maxLen = statusString.Length();
  if (maxLen > 254)
    maxLen = 254;
  
  ::SetControlData(staticTextControl, 0, kControlStaticTextTextTag, maxLen, statusString.get());
  ::DrawOneControl(staticTextControl);
      
  //::DrawDialog(mDialog);
  return NS_OK;
}

NS_IMETHODIMP
nsNativeAppSupportMac::ReOpen()
{

  PRBool haveUncollapsed = PR_FALSE;
  PRBool haveOpenWindows = PR_FALSE;
  PRBool done = PR_FALSE;
  
  nsCOMPtr<nsIWindowMediator> 
    wm(do_GetService(NS_WINDOWMEDIATOR_CONTRACTID));
  if (!wm)
  {
    return NS_ERROR_FAILURE;
  } 
  else
  {
    nsCOMPtr<nsISimpleEnumerator> windowList;
    wm->GetXULWindowEnumerator(nsnull, getter_AddRefs(windowList));
    PRBool more;
    windowList->HasMoreElements(&more);
    while (more)
    {
      nsCOMPtr<nsISupports> nextWindow = nsnull;
      windowList->GetNext(getter_AddRefs(nextWindow));
      nsCOMPtr<nsIBaseWindow> baseWindow(do_QueryInterface(nextWindow));
		  if (!baseWindow)
		  {
        windowList->HasMoreElements(&more);
        continue;
      }
      else
      {
        haveOpenWindows = PR_TRUE;
      }

      nsCOMPtr<nsIWidget> widget = nsnull;
      baseWindow->GetMainWidget(getter_AddRefs(widget));
      if (!widget)
      {
        windowList->HasMoreElements(&more);
        continue;
      }
      WindowRef windowRef = (WindowRef)widget->GetNativeData(NS_NATIVE_DISPLAY);
      if (!::IsWindowCollapsed(windowRef))
      {
        haveUncollapsed = PR_TRUE;
        break;  //have un-minimized windows, nothing to do
      } 
      windowList->HasMoreElements(&more);
    } // end while
        
    if (!haveUncollapsed)
    {
      //uncollapse the most recenty used window
      nsCOMPtr<nsIDOMWindowInternal> mru = nsnull;
      wm->GetMostRecentWindow(nsnull, getter_AddRefs(mru));
            
      if (mru) 
      {        
        WindowRef mruRef = nil;
        GetNativeWindowPointerFromDOMWindow(mru, &mruRef);
        if (mruRef)
        {
          ::CollapseWindow(mruRef, FALSE);
          ::SelectWindow(mruRef);
          done = PR_TRUE;
        }
      }
      
    } // end if have uncollapsed 
    
    if (!haveOpenWindows && !done)
    {    
    
      NS_WARNING("trying to open new window");
      //use the bootstrap helpers to make the right kind(s) of window open        
      nsresult rv = PR_FALSE;
      nsCOMPtr<nsICmdLineService> cmdLineArgs(do_GetService(kCmdLineServiceCID, &rv));
      if (!rv)
      {
        nsCOMPtr<nsIAppShellService> appShell(do_GetService(kAppShellServiceCID, &rv));
        if (!rv)
          {
            PRBool openedAWindow;
            // uncomment when bug 109811 is fixed
            //appShell->DoCommandLines(cmdLineArgs, true, &openedAWindow);
          }
        }        
    }
    
  } // got window mediator
  return NS_OK;
}

nsresult
GetNativeWindowPointerFromDOMWindow(nsIDOMWindowInternal *a_window, WindowRef *a_nativeWindow)
{
    *a_nativeWindow = nil;
    if (!a_window) return NS_ERROR_INVALID_ARG;
    
    nsCOMPtr<nsIWebNavigation> mruWebNav(do_GetInterface(a_window));
    if (mruWebNav)
    {
      nsCOMPtr<nsIDocShellTreeItem> mruTreeItem(do_QueryInterface(mruWebNav));
      nsCOMPtr<nsIDocShellTreeOwner> mruTreeOwner = nsnull;
      mruTreeItem->GetTreeOwner(getter_AddRefs(mruTreeOwner));
      if(mruTreeOwner)
      {
        nsCOMPtr<nsIBaseWindow> mruBaseWindow(do_QueryInterface(mruTreeOwner));
        if (mruBaseWindow)
        {
          nsCOMPtr<nsIWidget> mruWidget = nsnull;
          mruBaseWindow->GetMainWidget(getter_AddRefs(mruWidget));
          if (mruWidget)
          {
            *a_nativeWindow = (WindowRef)mruWidget->GetNativeData(NS_NATIVE_DISPLAY);
          }
        }
      }
    }
    return NS_OK;
}

#pragma mark -

// Create and return an instance of class nsNativeAppSupportMac.
nsresult NS_CreateNativeAppSupport(nsINativeAppSupport**aResult)
{
  if ( aResult )
  {  
      *aResult = new nsNativeAppSupportMac;
      if ( *aResult )
      {
          NS_ADDREF( *aResult );
          return NS_OK;
      } 
      else
      {
          return NS_ERROR_OUT_OF_MEMORY;
      }
  } 
  else
  {
      return NS_ERROR_NULL_POINTER;
  }
}

// Snagged from mozilla/xpinstall/wizrd/mac/src/SetupTypeWin.c
// VersGreaterThan4 - utility function to test if it's >4.x running
static Boolean VersGreaterThan4(const FSSpec *fSpec)
{
  Boolean result = false;
  short fRefNum = 0;
  
  ::SetResLoad(false);
  fRefNum = ::FSpOpenResFile(fSpec, fsRdPerm);
  ::SetResLoad(true);
  if (fRefNum != -1)
  {
    Handle  h;
    h = ::Get1Resource('vers', 2);
    if (h && **(unsigned short**)h >= 0x0500)
      result = true;
    ::CloseResFile(fRefNum);
  }
    
  return result;
}
