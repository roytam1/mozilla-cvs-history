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
#include "nspr.h"
#include "net.h"

#include <stdio.h>
#include "nscore.h"

#ifdef NS_WIN32
#include "windows.h"
#elif NS_UNIX
#include <Xm/Xm.h>
#endif

#include "nsISupports.h"
#include "nsIShellInstance.h"
#include "nsShellInstance.h"
#include "nsITimer.h"

#include "nsWidgetsCID.h"
#include "nsGfxCIID.h"
#include "nsParserCIID.h"

static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
static NS_DEFINE_IID(kIFactoryIID, NS_IFACTORY_IID);
static NS_DEFINE_IID(kCShellInstance, NS_SHELLINSTANCE_CID);

class nsShellInstanceFactory : public nsIFactory {

public:
  nsShellInstanceFactory(const nsCID &aClass);
  ~nsShellInstanceFactory();

  // nsISupports methods   
  NS_IMETHOD QueryInterface(const nsIID &aIID,    
                            void **aResult);   
  NS_IMETHOD_(nsrefcnt) AddRef(void);   
  NS_IMETHOD_(nsrefcnt) Release(void);   

  NS_IMETHOD CreateInstance(nsISupports * aOuter,
                            const nsIID &aIID,
                            void ** aResult);

  NS_IMETHOD LockFactory(PRBool aLock);

private:   
  nsrefcnt  mRefCnt;   
  nsCID     mClassID;

};

nsShellInstance::nsShellInstance()
{
  mApplicationWindow = NULL;
}

nsShellInstance::~nsShellInstance()
{
}

NS_DEFINE_IID(kIShellInstanceIID, NS_ISHELLINSTANCE_IID);
NS_IMPL_ISUPPORTS(nsShellInstance,kIShellInstanceIID);

nsresult nsShellInstance::Init()
{
  nsresult res = NS_OK;

  RegisterFactories() ;

  return res;
}

static nsITimer* gNetTimer;

static void
PollNet(nsITimer *aTimer, void *aClosure)
{
  NET_PollSockets();
  NS_IF_RELEASE(gNetTimer);
  if (NS_OK == NS_NewTimer(&gNetTimer)) {
    gNetTimer->Init(PollNet, nsnull, 1000 / 50);
  }
}

nsresult nsShellInstance::Run()
{

#ifdef NS_WIN32
  MSG msg;
  PollNet(0, 0);
  while (GetMessage(&msg, NULL, 0, 0)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
      NET_PollSockets();
  }
  return ((nsresult)msg.wParam);
#elif NS_UNIX
  extern   XtAppContext app_context ;

  XtAppMainLoop(app_context) ;
  return NS_OK;
#else
  return NS_OK;
#endif
}

void * nsShellInstance::GetNativeInstance()
{
  return mNativeInstance ;
}

void nsShellInstance::SetNativeInstance(void * aNativeInstance)
{
  mNativeInstance = aNativeInstance;
  return ;
}

nsIApplicationShell * nsShellInstance::GetApplicationShell()
{
  return mApplicationShell ;
}

void nsShellInstance::SetApplicationShell(nsIApplicationShell * aApplicationShell)
{
  mApplicationShell = aApplicationShell;
  return ;
}

// XXX We really need a standard way to enumerate 
//     a set of libraries and call their self
//     registration routines... when that code is 
//     XP of course.
nsresult nsShellInstance::RegisterFactories()
{
  // hardcode names of dll's
#ifdef NS_WIN32
  #define GFXWIN_DLL "raptorgfxwin.dll"
  #define WIDGET_DLL "raptorwidget.dll"
  #define PARSER_DLL "raptorhtmlpars.dll"
#else
  #define GFXWIN_DLL "libgfxunix.so"
  #define WIDGET_DLL "libwidgetunix.so"
  #define PARSER_DLL "libraptorhtmlpars.so"
#endif


  static NS_DEFINE_IID(kIWidgetIID, NS_IWIDGET_IID);
  NSRepository::RegisterFactory(kIWidgetIID, WIDGET_DLL, PR_FALSE, PR_FALSE);

  // register graphics classes
  static NS_DEFINE_IID(kCRenderingContextIID, NS_RENDERING_CONTEXT_CID);
  static NS_DEFINE_IID(kCDeviceContextIID, NS_DEVICE_CONTEXT_CID);
  static NS_DEFINE_IID(kCFontMetricsIID, NS_FONT_METRICS_CID);
  static NS_DEFINE_IID(kCImageIID, NS_IMAGE_CID);

  NSRepository::RegisterFactory(kCRenderingContextIID, GFXWIN_DLL, PR_FALSE, PR_FALSE);
  NSRepository::RegisterFactory(kCDeviceContextIID, GFXWIN_DLL, PR_FALSE, PR_FALSE);
  NSRepository::RegisterFactory(kCFontMetricsIID, GFXWIN_DLL, PR_FALSE, PR_FALSE);
  NSRepository::RegisterFactory(kCImageIID, GFXWIN_DLL, PR_FALSE, PR_FALSE);

  // register widget classes
  static NS_DEFINE_IID(kCWindowCID, NS_WINDOW_CID);
  static NS_DEFINE_IID(kCChildCID, NS_CHILD_CID);
  static NS_DEFINE_IID(kCButtonCID, NS_BUTTON_CID);
  static NS_DEFINE_IID(kCCheckButtonCID, NS_CHECKBUTTON_CID);
  static NS_DEFINE_IID(kCComboBoxCID, NS_COMBOBOX_CID);
  static NS_DEFINE_IID(kCFileWidgetCID, NS_FILEWIDGET_CID);
  static NS_DEFINE_IID(kCListBoxCID, NS_LISTBOX_CID);
  static NS_DEFINE_IID(kCRadioButtonCID, NS_RADIOBUTTON_CID);
  static NS_DEFINE_IID(kCRadioGroupCID, NS_RADIOGROUP_CID);
  static NS_DEFINE_IID(kCHorzScrollbarCID, NS_HORZSCROLLBAR_CID);
  static NS_DEFINE_IID(kCVertScrollbarCID, NS_VERTSCROLLBAR_CID);
  static NS_DEFINE_IID(kCTextAreaCID, NS_TEXTAREA_CID);
  static NS_DEFINE_IID(kCTextFieldCID, NS_TEXTFIELD_CID);
  static NS_DEFINE_IID(kCParserCID, NS_PARSER_IID);
  static NS_DEFINE_IID(kCParserNodeCID, NS_PARSER_NODE_IID);

  NSRepository::RegisterFactory(kCWindowCID, WIDGET_DLL, PR_FALSE, PR_FALSE);
  NSRepository::RegisterFactory(kCChildCID, WIDGET_DLL, PR_FALSE, PR_FALSE);
  NSRepository::RegisterFactory(kCButtonCID, WIDGET_DLL, PR_FALSE, PR_FALSE);
  NSRepository::RegisterFactory(kCCheckButtonCID, WIDGET_DLL, PR_FALSE, PR_FALSE);
  NSRepository::RegisterFactory(kCComboBoxCID, WIDGET_DLL, PR_FALSE, PR_FALSE);
  NSRepository::RegisterFactory(kCFileWidgetCID, WIDGET_DLL, PR_FALSE, PR_FALSE);
  NSRepository::RegisterFactory(kCListBoxCID, WIDGET_DLL, PR_FALSE, PR_FALSE);
  NSRepository::RegisterFactory(kCRadioButtonCID, WIDGET_DLL, PR_FALSE, PR_FALSE);
  NSRepository::RegisterFactory(kCRadioGroupCID, WIDGET_DLL, PR_FALSE, PR_FALSE);
  NSRepository::RegisterFactory(kCHorzScrollbarCID, WIDGET_DLL, PR_FALSE, PR_FALSE);
  NSRepository::RegisterFactory(kCVertScrollbarCID, WIDGET_DLL, PR_FALSE, PR_FALSE);
  NSRepository::RegisterFactory(kCTextAreaCID, WIDGET_DLL, PR_FALSE, PR_FALSE);
  NSRepository::RegisterFactory(kCTextFieldCID, WIDGET_DLL, PR_FALSE, PR_FALSE);
  NSRepository::RegisterFactory(kCParserCID, PARSER_DLL, PR_FALSE, PR_FALSE);
  NSRepository::RegisterFactory(kCParserNodeCID, PARSER_DLL, PR_FALSE, PR_FALSE);
  

  return NS_OK;
}

nsIWidget * nsShellInstance::CreateApplicationWindow(const nsRect &aRect,
                                                     EVENT_CALLBACK aHandleEventFunction)
{

  nsRect windowRect ;

  if (aRect.IsEmpty()) {
    windowRect.SetRect(100,100,320,480);
  } else {
    windowRect.SetRect(aRect.x, aRect.y, aRect.width, aRect.height);
  }

  static NS_DEFINE_IID(kIWidgetIID, NS_IWIDGET_IID);
  static NS_DEFINE_IID(kCWindowCID, NS_WINDOW_CID);

  NSRepository::CreateInstance(kCWindowCID, 
                               nsnull, 
                               kIWidgetIID, 
                               (void **)&(mApplicationWindow));

  mApplicationWindow->Create((nsIWidget*)NULL, 
                  aRect, 
                  aHandleEventFunction, 
                  nsnull, nsnull, nsnull, (nsWidgetInitData *) GetNativeInstance());

  return (mApplicationWindow);
}


nsresult nsShellInstance::ShowApplicationWindow(PRBool show)
{
  mApplicationWindow->Show(show);

#ifdef NS_UNIX
  XtRealizeWidget((Widget)GetNativeInstance());
#endif

  return NS_OK;
}

nsresult nsShellInstance::ExitApplication()
{

#ifdef NS_WIN32
  PostQuitMessage(0);
#endif
  return NS_OK;
}

void * nsShellInstance::GetApplicationWindowNativeInstance()
{
  return (mApplicationWindow->GetNativeData(NS_NATIVE_WINDOW));
}

nsIWidget * nsShellInstance::GetApplicationWidget()
{
  return (mApplicationWindow);
}

nsShellInstanceFactory::nsShellInstanceFactory(const nsCID &aClass)
{    
  mRefCnt = 0;
  mClassID = aClass;
}

nsShellInstanceFactory::~nsShellInstanceFactory()
{    
  NS_ASSERTION(mRefCnt == 0, "non-zero refcnt at destruction");   
}

nsresult nsShellInstanceFactory::QueryInterface(const nsIID &aIID,   
                                      void **aResult)   
{   
  if (aResult == NULL) {   
    return NS_ERROR_NULL_POINTER;   
  }   

  // Always NULL result, in case of failure   
  *aResult = NULL;   

  if (aIID.Equals(kISupportsIID)) {   
    *aResult = (void *)(nsISupports*)this;   
  } else if (aIID.Equals(kIFactoryIID)) {   
    *aResult = (void *)(nsIFactory*)this;   
  }   

  if (*aResult == NULL) {   
    return NS_NOINTERFACE;   
  }   

  AddRef(); // Increase reference count for caller   
  return NS_OK;   
}   

nsrefcnt nsShellInstanceFactory::AddRef()   
{   
  return ++mRefCnt;   
}   

nsrefcnt nsShellInstanceFactory::Release()   
{   
  if (--mRefCnt == 0) {   
    delete this;   
    return 0; // Don't access mRefCnt after deleting!   
  }   
  return mRefCnt;   
}  

nsresult nsShellInstanceFactory::CreateInstance(nsISupports * aOuter,
                                                   const nsIID &aIID,
                                                   void ** aResult)
{
  if (aResult == NULL) {  
    return NS_ERROR_NULL_POINTER;  
  }  

  *aResult = NULL;  
  
  nsISupports *inst = nsnull;

  if (mClassID.Equals(kCShellInstance)) {
    inst = (nsISupports *)new nsShellInstance();
  }

  if (inst == NULL) {  
    return NS_ERROR_OUT_OF_MEMORY;  
  }  

  nsresult res = inst->QueryInterface(aIID, aResult);

  if (res != NS_OK) {  
    // We didn't get the right interface, so clean up  
    delete inst;  
  }  

  return res;  

}

nsresult nsShellInstanceFactory::LockFactory(PRBool aLock)
{
  return NS_OK;
}

// return the proper factory to the caller
extern "C" NS_SHELL nsresult NSGetFactory(const nsCID &aClass, nsIFactory **aFactory)
{
  if (nsnull == aFactory) {
    return NS_ERROR_NULL_POINTER;
  }

  *aFactory = new nsShellInstanceFactory(aClass);

  if (nsnull == aFactory) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return (*aFactory)->QueryInterface(kIFactoryIID, (void**)aFactory);
}


