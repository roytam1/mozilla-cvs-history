/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */
#include "jdefines.h"
#include "julnstr.h"
#include "nsCalShellFactory.h"
#include "nsString.h"
#include "nsFont.h"
#include "nsCalShellCIID.h"
#include "nsXPFCObserverManager.h"
#include "nsCRT.h"
#include "plstr.h"
#include "prmem.h"
#include "prenv.h"
#include "icalfrdr.h"
#include "nsIPref.h"
#include "nsCurlParser.h"
#include "nsCalendarShell.h"
#include "nscalstrings.h"
#include "nsxpfcCIID.h"
#include "nsIAppShell.h"
#include "nsICommandServer.h"
#include "nsCalUICIID.h"
#include "nsCalCommandCanvas.h"
#include "nlsloc.h"
#include "nsCapiCIID.h"
#include "nspr.h"
#include "prcvar.h"
#include "nsXPFCToolkit.h"
#include "nsX400Parser.h"
#include "nsxpfcCIID.h"

#include "capi.h"
#include "nsICapi.h"

#include "nsCoreCIID.h"
#include "nsLayer.h"
#include "nsLayerCollection.h"
#include "nsCalUser.h"
#include "nsCalendarUser.h"
#include "nsCalendarModel.h"
#include "nsIServiceManager.h"


/* for CAPI to work in general form */
#include "nsCapiCallbackReader.h"
#include "nsCalStreamReader.h"

#ifdef NS_WIN32
#include "windows.h"
#endif

#ifdef NS_UNIX
#include "Xm/Xm.h"
#include "Xm/MainW.h"
#include "Xm/Frame.h"
#include "Xm/XmStrDefs.h"
#include "Xm/DrawingA.h"

extern XtAppContext app_context;
extern Widget topLevel;

#endif

// All Applications must specify this *special* application CID
// to their own unique IID.
nsIID kIXPCOMApplicationShellCID = NS_CAL_SHELL_CID ; 

static NS_DEFINE_IID(kIXPFCObserverManagerIID, NS_IXPFC_OBSERVERMANAGER_IID);
static NS_DEFINE_IID(kCXPFCObserverManagerCID, NS_XPFC_OBSERVERMANAGER_CID);

// hardcode names of dll's
#ifdef NS_WIN32
  #define CAPI_DLL    "calcapi10.dll"
  #define CORE_DLL    "calcore10.dll"
#else
  #define CAPI_DLL   "libcalcapi10.so"
  #define CORE_DLL   "libcalcore10.so"
#endif

static NS_DEFINE_IID(kCCapiLocalCID,        NS_CAPI_LOCAL_CID);
static NS_DEFINE_IID(kCCapiCSTCID,          NS_CAPI_CST_CID);
static NS_DEFINE_IID(kCLayerCID,            NS_LAYER_CID);
static NS_DEFINE_IID(kILayerIID,            NS_ILAYER_IID);
static NS_DEFINE_IID(kCLayerCollectionCID,  NS_LAYER_COLLECTION_CID);
static NS_DEFINE_IID(kILayerCollectionIID,  NS_ILAYER_COLLECTION_IID);
static NS_DEFINE_IID(kCCalendarModelCID,    NS_CALENDAR_MODEL_CID);
static NS_DEFINE_IID(kICalendarUserIID,     NS_ICALENDAR_USER_IID); 
static NS_DEFINE_IID(kCCalendarUserCID,     NS_CALENDAR_USER_CID);
static NS_DEFINE_IID(kIUserIID,             NS_IUSER_IID);
static NS_DEFINE_IID(kISupportsIID,         NS_ISUPPORTS_IID);
static NS_DEFINE_IID(kCUserCID,             NS_USER_CID);

// All Application Must implement this function
nsresult NS_RegisterApplicationShellFactory()
{

  nsresult res = nsRepository::RegisterFactory(kIXPCOMApplicationShellCID,
                                               new nsCalShellFactory(kIXPCOMApplicationShellCID),
                                               PR_FALSE) ;

  return res;
}

/*
 * nsCalendarShell Definition
 */
nsCalendarShell::nsCalendarShell()
{
  NS_INIT_REFCNT();

  mShellInstance  = nsnull ;
  mDocumentContainer = nsnull ;
  mObserverManager = nsnull;
  mCAPIPassword = nsnull;
  mpLoggedInUser = nsnull;
  mCommandServer = nsnull;
  mCAPISession = nsnull;
  mCAPIHandle = nsnull;
}

nsCalendarShell::~nsCalendarShell()
{
  nsCalSession * session = mSessionMgr.GetAt(0L);

  if (session != nsnull)
    session->mCapi->CAPI_DestroyHandles(mCAPISession, &mCAPIHandle, 1, 0L);

  Logoff();

  nsServiceManager::ReleaseService(kCXPFCObserverManagerCID, mObserverManager);

  if (mCAPIPassword)
    PR_Free(mCAPIPassword);

  NS_IF_RELEASE(mpLoggedInUser);
  NS_IF_RELEASE(mDocumentContainer);
  NS_IF_RELEASE(mShellInstance);
  NS_IF_RELEASE(mCommandServer);
}

static NS_DEFINE_IID(kIAppShellIID, NS_IAPPSHELL_IID);


nsresult nsCalendarShell::QueryInterface(const nsIID& aIID, void** aInstancePtr)  
{                                                                        
  if (NULL == aInstancePtr) {                                            
    return NS_ERROR_NULL_POINTER;                                        
  }                                                                      

  if(aIID.Equals(kISupportsIID))    {  //do IUnknown...
    *aInstancePtr = (nsISupports*)(nsIApplicationShell *)(this);                                        
  }
  else if(aIID.Equals(kIXPCOMApplicationShellCID)) {  //do nsIContentSink base class...
    *aInstancePtr = (nsICalendarShell*)(this);                                        
  }
  else if(aIID.Equals(kIAppShellIID)) {  //do nsIContentSink base class...
    *aInstancePtr = (nsIAppShell*)(this);                                        
  }
  else {
    *aInstancePtr=0;
    return NS_NOINTERFACE;
  }
  ((nsISupports*) *aInstancePtr)->AddRef();
  return NS_OK;                                                        
}

NS_IMPL_ADDREF(nsCalendarShell)
NS_IMPL_RELEASE(nsCalendarShell)

nsresult nsCalendarShell::Init()
{
  /*
   * Register class factrories needed for application
   */
  RegisterFactories() ;

  InitFactoryObjs();

  /*
   * Load Application Prefs
   */
  LoadPreferences();

  /*
   * Logon to the system
   */
  Logon();

  /*
   * Create the UI
   */
  LoadUI();

  return NS_OK;
}

/**
 * Create factory objects...
 * @return NS_OK on success
 */
nsresult nsCalendarShell::InitFactoryObjs()
{
  nsresult res;

  res = nsRepository::CreateInstance(kCCalendarUserCID,  // class id that we want to create
                                     nsnull,             // not aggregating anything  (this is the aggregatable interface)
                                     kICalendarUserIID,  // interface id of the object we want to get back
                                     (void**)&mpLoggedInUser);

  if (NS_OK == res) 
  {
    mpLoggedInUser->Init();
  }

  return res;
}

/**
 * This method establishes a logged in user and opens a connection to 
 * the calendar server (or local database file if they're working offline).
 *
 * @return NS_OK on success
 *         1 = could not create or write to a local capi directory
 */
nsresult nsCalendarShell::Logon()
{
  CAPIStatus s;
  nsIUser* pUser;
  nsresult res;

  if (NS_OK != (res = mpLoggedInUser->QueryInterface(kIUserIID,(void**)&pUser)))
    return res ;


  /*
   * THIS WHOLE AREA NEEDS TO BE MOVED INTO USERMGR. Have it 
   * create the user, etc based on a curl or curllist.
   */


  /*
   *  Getting the first calendar by user name should be reviewed.
   */
  nsString nsstrUserName;
  pUser->GetUserName(nsstrUserName);
  JulianString sUserName(256);
  nsstrUserName.ToCString(sUserName.GetBuffer(),256);
  sUserName.DoneWithBuffer();

  nsCurlParser theURL(sUserName);
  nsCurlParser sessionURL(msCalURL);
  theURL |= sessionURL;

  /*
   *  Ask the session manager for a session...
   */
  res = mSessionMgr.GetSession(
        msCalURL.GetBuffer(),  // may contain a password, if so it will be used
        0L, 
        GetCAPIPassword(), 
        mCAPISession);
  
  pUser->GetUserName(nsstrUserName);
  JulianString sHandle(256);
  nsstrUserName.ToCString(sHandle.GetBuffer(),256);
  sHandle.DoneWithBuffer();

  
  NS_RELEASE(pUser);

  if (nsCurlParser::eCAPI == theURL.GetProtocol())
  {
    nsX400Parser x(theURL.GetExtra());
    x.Delete("ND");
    x.GetValue(sHandle);
    sHandle.Prepend(":");
  }

  s = mSessionMgr.GetAt(0L)->mCapi->CAPI_GetHandle(mCAPISession,sHandle.GetBuffer(),0,&mCAPIHandle);

  if (CAPI_ERR_OK != s)
    return NS_OK;

  /*
   * Begin a calendar for the logged in user...
   */

 switch(theURL.GetProtocol())
  {
    case nsCurlParser::eFILE:
    case nsCurlParser::eCAPI:
    {
      DateTime d;
      DateTime d1;
      JulianPtrArray EventList;
      nsILayer *pLayer;
      d.prevDay(14);
      d1.nextDay(14);
      mpLoggedInUser->GetLayer(pLayer);
      NS_ASSERTION(0 != pLayer,"null pLayer");
      {
        char sBuf[2048];
        int iBufSize = sizeof(sBuf);
        JulianString sTmp;
        mShellInstance->GetPreferences()->GetCharPref(CAL_STRING_PREF_PREFERRED_ADDR,sBuf, &iBufSize );
        sTmp = sBuf;
        EnvVarsToValues(sTmp);
      pLayer->SetCurl(sTmp);
      }
      pLayer->SetShell(this);
      pLayer->FetchEventsByRange(&d,&d1,&EventList);
    }
    break;

    default:
    {
      /* need to report that this is an unhandled curl type */
    }
    break;
  }

  return NS_OK;
}

nsresult nsCalendarShell::Logoff()
{
  /*
   * Shut down any open CAPI sessions.
   */
  mSessionMgr.Shutdown();

  return NS_OK;
}


/*
 *  DEFAULT PREFERENCES
 *  If you have a new preference for calendar, here's what to do:
 *  1. Add the defines for the key name and a default value in nscalstrings.h
 *  2. Add the key name and default value defines in the table below.
 *  3. you're all set
 */
typedef struct
{
  char* psKey;
  char* psVal;
} JULIAN_KEY_VAL;

static JULIAN_KEY_VAL KVPairs[] =
{
  CAL_STRING_PREF_JULIAN_UI_XML_MENUBAR,  CAL_STRING_RESOURCE_UI_MENUBAR,
  CAL_STRING_PREF_JULIAN_UI_XML_TOOLBAR,  CAL_STRING_RESOURCE_UI_TOOLBAR,
  CAL_STRING_PREF_JULIAN_UI_XML_CALENDAR, CAL_STRING_RESOURCE_UI_CALENDAR,
  CAL_STRING_PREF_LOCAL_ADDRESS,          CAL_STRING_PREF_LOCAL_ADDRESS_DEFAULT,
  CAL_STRING_PREF_PREFERRED_ADDR,         CAL_STRING_PREF_PREFERRED_ADDR_DEFAULT,
  CAL_STRING_PREF_USERNAME,               CAL_STRING_PREF_USERNAME_DEFAULT,
  CAL_STRING_PREF_USER_DISPLAY_NAME,      CAL_STRING_PREF_USER_DISPLAY_NAME_DEFAULT,
  
  /* Enter new key, default-value pairs above this line */
  0, 0
};

/**
 * Load all the preferences listed in the table above. Set any
 * preferences that do not exist to their default value. Save the
 * preferences upon completion.
 * @return NS_OK
 */
nsresult nsCalendarShell::SetDefaultPreferences()
{
  char pref[1024];
  int i = 1024;

  for (int j = 0; 0 != KVPairs[j].psKey ; j++)
  {
    if (0 != mShellInstance->GetPreferences()->GetCharPref(KVPairs[j].psKey, pref, &i))
      mShellInstance->GetPreferences()->SetCharPref(KVPairs[j].psKey, KVPairs[j].psVal);
  }

  return mShellInstance->GetPreferences()->SavePrefFile();
}

/*
 * the list of environment variables we're interested in...
 */
static char *gtVars[] = 
{
  "$USERNAME",
  "$USERPROFILE",
  0
};

/**
 *  Preferences may contain references to variables that are filled
 *  in at run time.  The current list is:
 *
 *  <PRE>
 *  variable         meaning
 *  ---------------  ----------------------------------------------
 *  $USERNAME        the OS username or other default user name
 *  $USERPROFILE     the file path to where the user's local data
 *                   should be stored.
 *  </PRE>
 *
 *  This routine replaces the variables with their current values.
 *  The results are returned in the supplied buffer.
 *
 *  @param s    the calendar url preference
 *  @param iBufSize  the amount of space available in psCurl
 *  @return 0 on success
 */
nsresult nsCalendarShell::EnvVarsToValues(JulianString& s)
{
  PRInt32 iIndex;
  PRInt32 i;
  char* p;
  char* psEnv;

  /*
   * Replace all callouts of the env vars with their values...
   */
  for (i = 0; 0 != gtVars[i]; i++)
  {
    p = gtVars[i];
    if (-1 != s.Find(p))
    {
      psEnv = PR_GetEnv(&p[1]);
      if (0 != psEnv)
      {
        while (-1 != (iIndex = s.Find(p)))
        {
            s.Replace(iIndex, iIndex + PL_strlen(p) - 1, psEnv);
        }
      }
    }
  }
  return 0;
}


/**
 * Load the preferences that help us get bootstrapped. Specifically,
 * determine the cal url for the user.
 * @return NS_OK on success. Otherwise errors from retrieving preferences.
 */
nsresult nsCalendarShell::LoadPreferences()
{
  nsresult res = nsApplicationManager::GetShellInstance(this, &mShellInstance);
  nsCurlParser curl;

  if (NS_OK != res)
    return res ;

  /*
   * Load the overriding user preferences
   */
  mShellInstance->GetPreferences()->Startup("prefs.js");

  /*
   * Set the default preferences
   */
  SetDefaultPreferences();
  char sBuf[1024];
  int iBufSize = 1024;
  int iSavePrefs = 0;
  JulianString s;

  sBuf[0]=0;

  nsIUser* pUser;
  if (NS_OK != (res = mpLoggedInUser->QueryInterface(kIUserIID,(void**)&pUser)))
    return res ;

  /*
   * fill in info for the user...
   */
  mShellInstance->GetPreferences()->GetCharPref(CAL_STRING_PREF_USERNAME,sBuf, &iBufSize );
  s = sBuf;
  EnvVarsToValues(s);
  nsString nsstrUserName = s.GetBuffer();
  pUser->SetUserName(nsstrUserName);

  /*
   * Add the logged in user to the user list...
   */
  mUserList.Add( mpLoggedInUser );

  /*****************************************************************/
  
  /*
   *  This section of code needs to be revamped after a review.
   *  Currently, it expects to load a local and preferred cal url.
   *  We need to rethink this.
   */

  /*
   *  Get the local cal address.
   */
  mShellInstance->GetPreferences()->GetCharPref(CAL_STRING_PREF_LOCAL_ADDRESS,sBuf, &iBufSize );
  s = sBuf;
  EnvVarsToValues(s);
  curl = s;   
  curl.ResolveFileURL();

  /* XXX for now, we're ignoring this value... */

  /*
   *  Load the preferred cal address. For now, the local cal address is the default too...
   */
  mShellInstance->GetPreferences()->GetCharPref(CAL_STRING_PREF_PREFERRED_ADDR,sBuf, &iBufSize );
  s = sBuf;
  EnvVarsToValues(s);
  curl = s;   
  curl.ResolveFileURL();
  // mpLoggedInUser->AddCalAddr(new JulianString(curl.GetCurl().GetBuffer()));

  /*
   * create a layer for the preferred address
   */
  nsILayer* pLayer;
  if (NS_OK != (res = nsRepository::CreateInstance(
          kCLayerCollectionCID,  // class id that we want to create
          nsnull,                // not aggregating anything  (this is the aggregatable interface)
          kILayerIID,            // interface id of the object we want to get back
          (void**)&pLayer)))
    return 1;  // XXX fix this
  pLayer->Init();
  pLayer->SetCurl(curl.GetCurl());
  mpLoggedInUser->SetLayer(pLayer);

  /*****************************************************************/

  /*
   *  Get the user's display name
   */
  mShellInstance->GetPreferences()->GetCharPref(CAL_STRING_PREF_USER_DISPLAY_NAME,sBuf, &iBufSize );
  // mpLoggedInUser->SetDisplayName(sBuf); // XXX fix this

  /*
   *  Set the curl to use for the logged in user's calendar...
   */
  // msCalURL = * mpLoggedInUser->GetPreferredCalAddr();
  msCalURL = curl.GetCurl();

  /*
   *  XXX: We gotta ask for this or save a secure password pref or something...
   */
  SetCAPIPassword("HeyBaby");

  NS_RELEASE(pUser);

  return NS_OK;
}

/*
 * Load the UI for this instance
 */

nsresult nsCalendarShell::LoadUI()
{

  /*
   * First, create the ObserverManager
   */

  nsServiceManager::GetService(kCXPFCObserverManagerCID, kIXPFCObserverManagerIID, (nsISupports**)&mObserverManager);
  
  /*
   * Now create an actual window into the world
   */

  nsRect aRect(20,20,800,600) ;

  /*
   * Get our nsIAppShell interface and pass it down
   */

  nsIAppShell * appshell = nsnull;

  nsresult res = QueryInterface(kIAppShellIID,(void**)&appshell);

  if (NS_OK != res)
    return res ;

  mShellInstance->CreateApplicationWindow(appshell, aRect);

  NS_RELEASE(appshell);

  /* 
   * Load a Container
   */

  static NS_DEFINE_IID(kCCalContainerCID, NS_CALENDAR_CONTAINER_CID);

  res = NS_NewCalendarContainer((nsICalendarContainer **)&(mDocumentContainer)) ;

  if (NS_OK != res)
    return res ;

  mDocumentContainer->SetApplicationShell((nsIApplicationShell*)this);

  mDocumentContainer->SetToolbarManager(mShellInstance->GetToolbarManager());

  // Start the widget from upper left corner
  aRect.x = aRect.y = 0;

  mDocumentContainer->Init(mShellInstance->GetApplicationWidget(),
                           aRect, 
                           this) ;

  char pUI[1024];
  int i = 1024;

  /*
   * Now Load the Canvas UI
   */

  mShellInstance->GetPreferences()->GetCharPref(CAL_STRING_PREF_JULIAN_UI_XML_CALENDAR,pUI,&i);
  res = mDocumentContainer->LoadURL(pUI,nsnull);

  mShellInstance->ShowApplicationWindow(PR_TRUE) ;

  return res ;
}

nsresult nsCalendarShell::SetCAPIPassword(char * aCAPIPassword)
{
  if (mCAPIPassword)
    PR_Free(mCAPIPassword);

  mCAPIPassword = (char *) PR_Malloc(nsCRT::strlen(aCAPIPassword)+1);

  nsCRT::memcpy(mCAPIPassword, aCAPIPassword, nsCRT::strlen(aCAPIPassword)+1);

  return NS_OK;
}

char * nsCalendarShell::GetCAPIPassword()
{
  return (mCAPIPassword);
}


nsresult nsCalendarShell::SetCAPIHandle(CAPIHandle aCAPIHandle)
{
  mCAPIHandle = aCAPIHandle;
  return NS_OK;
}

CAPIHandle nsCalendarShell::GetCAPIHandle()
{
  return (mCAPIHandle);
}

nsresult nsCalendarShell::SetCAPISession(CAPISession aCAPISession)
{
  mCAPISession = aCAPISession;
  return NS_OK;
}

CAPISession nsCalendarShell::GetCAPISession()
{
  return (mCAPISession);
}

nsresult nsCalendarShell::Create(int* argc, char ** argv)
{
  return NS_OK;
}
nsresult nsCalendarShell::Exit()
{
  NS_IF_RELEASE(mDocumentContainer);

  NLS_Terminate();

  return NS_OK;
}

nsresult nsCalendarShell::Run()
{
  mShellInstance->Run();
  return NS_OK;
}

nsresult nsCalendarShell::SetDispatchListener(nsDispatchListener* aDispatchListener)
{
  return NS_OK;
}
void* nsCalendarShell::GetNativeData(PRUint32 aDataType)
{
#ifdef XP_UNIX
  if (aDataType == NS_NATIVE_SHELL)
    return topLevel;

  return nsnull;
#else
  return (mShellInstance->GetApplicationWidget());
#endif

}

nsresult nsCalendarShell::RegisterFactories()
{
  /*
    Until an installer does this for us, we do it here
  */

  // hardcode names of dll's
#ifdef NS_WIN32
  #define CALUI_DLL "calui10.dll"
  #define XPFC_DLL  "xpfc10.dll"
#else
  #define CALUI_DLL "libcalui10.so"
  #define XPFC_DLL "libxpfc10.so"
#endif

  // register graphics classes
  static NS_DEFINE_IID(kCXPFCCanvasCID, NS_XPFC_CANVAS_CID);
  static NS_DEFINE_IID(kCCalTimeContextCID, NS_CAL_TIME_CONTEXT_CID);
  static NS_DEFINE_IID(kCCalContextControllerCID, NS_CAL_CONTEXT_CONTROLLER_CID);

  nsRepository::RegisterFactory(kCXPFCCanvasCID, XPFC_DLL, PR_FALSE, PR_FALSE);
  nsRepository::RegisterFactory(kCCalTimeContextCID, CALUI_DLL, PR_FALSE, PR_FALSE);
  nsRepository::RegisterFactory(kCCalContextControllerCID, CALUI_DLL, PR_FALSE, PR_FALSE);

  static NS_DEFINE_IID(kCVectorCID, NS_VECTOR_CID);
  static NS_DEFINE_IID(kCVectorIteratorCID, NS_VECTOR_ITERATOR_CID);

  nsRepository::RegisterFactory(kCVectorCID, XPFC_DLL, PR_FALSE, PR_FALSE);
  nsRepository::RegisterFactory(kCVectorIteratorCID, XPFC_DLL, PR_FALSE, PR_FALSE);

  static NS_DEFINE_IID(kCXPFCObserverManagerCID,   NS_XPFC_OBSERVERMANAGER_CID);
  nsRepository::RegisterFactory(kCXPFCObserverManagerCID, XPFC_DLL, PR_FALSE, PR_FALSE);

  // Register the CAPI implementations
  nsRepository::RegisterFactory(kCCapiLocalCID, CAPI_DLL, PR_FALSE, PR_FALSE);
  nsRepository::RegisterFactory(kCCapiCSTCID,   CAPI_DLL, PR_FALSE, PR_FALSE);

  // Register the Core Implementations
  nsRepository::RegisterFactory(kCCalendarUserCID, CORE_DLL, PR_FALSE, PR_FALSE);
  nsRepository::RegisterFactory(kCCalendarModelCID, CORE_DLL, PR_FALSE, PR_FALSE);
  nsRepository::RegisterFactory(kCLayerCID, CORE_DLL, PR_FALSE, PR_FALSE);
  nsRepository::RegisterFactory(kCLayerCollectionCID, CORE_DLL, PR_FALSE, PR_FALSE);

  return NS_OK;
}

nsresult nsCalendarShell::GetWebViewerContainer(nsIWebViewerContainer ** aWebViewerContainer)
{
  *aWebViewerContainer = (nsIWebViewerContainer*) mDocumentContainer;
  NS_ADDREF(*aWebViewerContainer);
  return NS_OK;
}

nsEventStatus nsCalendarShell::HandleEvent(nsGUIEvent *aEvent)
{
  nsEventStatus result = nsEventStatus_eConsumeNoDefault;

  switch(aEvent->message) {

      case NS_CREATE:
      {
        return nsEventStatus_eConsumeNoDefault;
      }
      break ;

      case NS_DESTROY:
      {
        if (mShellInstance->GetApplicationWidget() == aEvent->widget)
        {
          mShellInstance->ExitApplication() ;
          result = mDocumentContainer->HandleEvent(aEvent);
          Exit();
          return result;
        }
        if (mDocumentContainer != nsnull)
          return (mDocumentContainer->HandleEvent(aEvent));
      }
      break ;

      case NS_PAINT:
      {
        ((nsPaintEvent*)aEvent)->rect->x = 0 ;
        ((nsPaintEvent*)aEvent)->rect->y = 0 ;
        if (mDocumentContainer != nsnull)
          return (mDocumentContainer->HandleEvent(aEvent));
      }
      break;

      case NS_SIZE:
      {
        ((nsSizeEvent*)aEvent)->windowSize->x = 0 ;
        ((nsSizeEvent*)aEvent)->windowSize->y = 0 ;
      }
      default:
      {
        if (mDocumentContainer != nsnull)
          return (mDocumentContainer->HandleEvent(aEvent));
      }
      break;

  }

  return nsEventStatus_eIgnore; 

}

nsresult nsCalendarShell::ReceiveCommand(nsString& aCommand, nsString& aReply)
{
  /*
   * Call SendCommand on the CommandCanvas!
   */
  nsIXPFCCanvas * root = nsnull ;
  nsIXPFCCanvas * canvas = nsnull;

  nsString name("CommandCanvas");

  gXPFCToolkit->GetRootCanvas(&root);

  canvas = root->CanvasFromName(name);

  if (canvas != nsnull)
  {
    nsCalCommandCanvas * cc ;

    static NS_DEFINE_IID(kCalCommandCanvasCID, NS_CAL_COMMANDCANVAS_CID);
    nsresult res = canvas->QueryInterface(kCalCommandCanvasCID,(void**)&cc);

    if (res == NS_OK)
    {
      cc->SendCommand(aCommand, aReply);
      NS_RELEASE(cc);
    }
  }

  NS_RELEASE(root);
  return NS_OK;
}

nsresult nsCalendarShell::StartCommandServer()
{

  if (mCommandServer)
    return NS_OK;

  static NS_DEFINE_IID(kICommandServerIID, NS_IXPFC_COMMAND_SERVER_IID);
  static NS_DEFINE_IID(kCCommandServerCID, NS_XPFC_COMMAND_SERVER_CID);
  static NS_DEFINE_IID(kIApplicationShellIID, NS_IAPPLICATIONSHELL_IID);

  nsresult res = nsRepository::CreateInstance(kCCommandServerCID, 
                                              nsnull, 
                                              kICommandServerIID, 
                                              (void **)&mCommandServer);

  if (NS_OK != res)
    return res ;

  mCommandServer->Init((nsIApplicationShell *)this);
 
  return NS_OK;
}



// XXX BAD BAD BAD GROSSNESS ... Here so Linux will link ... ARGH!


#include "xp_mcom.h"
#include "net.h"

extern "C" XP_Bool ValidateDocData(MWContext *window_id) 
{ 
  printf("ValidateDocData not implemented, stubbed in CalendarShell.cpp\n"); 
  return PR_TRUE; 
}

/* dist/public/xp/xp_linebuf.h */
int XP_ReBuffer (const char *net_buffer, int32 net_buffer_size,
                        uint32 desired_buffer_size,
                        char **bufferP, uint32 *buffer_sizeP,
                        uint32 *buffer_fpP,
                        int32 (*per_buffer_fn) (char *buffer,
                                                uint32 buffer_size,
                                                void *closure),
                        void *closure) 
{ 

  printf("XP_ReBuffer not implemented, stubbed in CalendarShell.cpp\n"); 
  return(0); 
}



/* mozilla/include/xp_trace.h */

extern "C" void XP_Trace( const char *, ... ) 
{ 
  printf("XP_Trace not implemented, stubbed in CalendarShell.cpp\n"); 


} 


