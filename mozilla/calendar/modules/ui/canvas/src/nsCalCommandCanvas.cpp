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

#include "nsCalCommandCanvas.h"
#include "nsCalUICIID.h"
#include "nsITextWidget.h"
#include "nsWidgetsCID.h"
#include "nsCalToolkit.h"
#include "nsXPFCMethodInvokerCommand.h"
#include "nsIXPFCObserver.h"

#include "nsIDeviceContext.h"
#include "nsFont.h"
#include "nsIFontMetrics.h"

static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
static NS_DEFINE_IID(kCalCommandCanvasCID, NS_CAL_COMMANDCANVAS_CID);
static NS_DEFINE_IID(kITextWidgetIID, NS_ITEXTWIDGET_IID);
static NS_DEFINE_IID(kCTextFieldCID, NS_TEXTFIELD_CID);
static NS_DEFINE_IID(kIWidgetIID, NS_IWIDGET_IID);

#define kNotFound -1

#define DEFAULT_WIDTH  25
#define DEFAULT_HEIGHT 25

nsEventStatus PR_CALLBACK HandleEventTextField(nsGUIEvent *aEvent);

nsCalCommandCanvas :: nsCalCommandCanvas(nsISupports* outer) : nsXPFCCanvas(outer)
{
  NS_INIT_REFCNT();
  mStaticTextField = nsnull;
  mTextField = nsnull;
}

nsCalCommandCanvas :: ~nsCalCommandCanvas()
{

  gXPFCToolkit->GetCanvasManager()->Unregister(this);
  
  NS_IF_RELEASE(mStaticTextField);
  NS_IF_RELEASE(mTextField);
}

nsresult nsCalCommandCanvas::QueryInterface(REFNSIID aIID, void** aInstancePtr)      
{                                                                        
  if (NULL == aInstancePtr) {                                            
    return NS_ERROR_NULL_POINTER;                                        
  }                                                                      
  static NS_DEFINE_IID(kISupportsIID,  NS_ISUPPORTS_IID);                 
  static NS_DEFINE_IID(kXPFCSubjectIID, NS_IXPFC_SUBJECT_IID);                 
  static NS_DEFINE_IID(kClassIID, kCalCommandCanvasCID);
  if (aIID.Equals(kClassIID)) {                                          
    *aInstancePtr = (void*) (nsCalCommandCanvas *)this;                                        
    AddRef();                                                            
    return NS_OK;                                                        
  }                                                                      
  if (aIID.Equals(kXPFCSubjectIID)) {                                          
    *aInstancePtr = (void*) (nsIXPFCSubject *)this;                                        
    AddRef();                                                            
    return NS_OK;                                                        
  }                                                                      
  if (aIID.Equals(kISupportsIID)) {                                      
    *aInstancePtr = (void*) (this);                        
    AddRef();                                                            
    return NS_OK;                                                        
  }                                                                      
  return (nsXPFCCanvas::QueryInterface(aIID, aInstancePtr));
}

NS_IMPL_ADDREF(nsCalCommandCanvas)
NS_IMPL_RELEASE(nsCalCommandCanvas)

nsresult nsCalCommandCanvas :: Init()
{
  nsRect rect;

  GetBounds(rect);

  /*
   * Static Field with read-only string
   */

  nsString text("COMMAND: ");

  nsRepository::CreateInstance(kCTextFieldCID, 
                               nsnull, 
                               kITextWidgetIID, 
                               (void **)&mStaticTextField);

  
  nsFont font("Times", NS_FONT_STYLE_NORMAL,
		  NS_FONT_VARIANT_NORMAL,
		  NS_FONT_WEIGHT_BOLD,
		  0,
		  8);

  nsIFontMetrics * fm ;
  
  GetWidget()->GetDeviceContext()->GetMetricsFor(font, fm);

  nscoord width ;
  
  fm->GetWidth(text,width);

  rect.width = width;

  mStaticTextField->SetReadOnly(PR_TRUE);

  nsIWidget * widget = nsnull;
  nsresult res = mStaticTextField->QueryInterface(kIWidgetIID,(void**)&widget);
  gXPFCToolkit->GetCanvasManager()->Register(this, widget);
  NS_RELEASE(widget);

  mStaticTextField->Create(GetWidget(), 
                           rect, 
                           HandleEventTextField, 
                           nsnull, nsnull, nsnull);

  mStaticTextField->SetText(text);
  mStaticTextField->Show(PR_TRUE);


  /*
   * Writeable Command field
   */

  GetBounds(rect);

  rect.x += width;
  rect.width -= width;
  
  nsRepository::CreateInstance(kCTextFieldCID, 
                               nsnull, 
                               kITextWidgetIID, 
                               (void **)&mTextField);

  widget = nsnull;
  res = mTextField->QueryInterface(kIWidgetIID,(void**)&widget);
  gXPFCToolkit->GetCanvasManager()->Register(this, widget);
  NS_RELEASE(widget);
  
  mTextField->Create(GetWidget(), 
                     rect, 
                     HandleEventTextField, 
                     NULL);

  text = "TimebarScale setbackgroundcolor #FF0000";
  mTextField->SetText(text);
  mTextField->Show(PR_TRUE);

  return NS_OK;
}

// XXX: todo: we should be passing commands off rather than calling 
//      directly, and we need to put this code in the topmost event
//      loop and call a method of the canvas containing the widget.
//      ........

nsEventStatus PR_CALLBACK HandleEventTextField(nsGUIEvent *aEvent)
{

  if (gXPFCToolkit == nsnull)
    return nsEventStatus_eIgnore;

  nsIXPFCCanvas * canvas = gXPFCToolkit->GetCanvasManager()->CanvasFromWidget(aEvent->widget);

  if (canvas)
    return (canvas->HandleEvent(aEvent));

  return nsEventStatus_eIgnore;
}

nsEventStatus nsCalCommandCanvas :: OnResize(nsGUIEvent *aEvent)
{
  SetBounds(*((nsSizeEvent*)aEvent)->windowSize);
  return (nsXPFCCanvas::OnResize(aEvent));
}

nsresult nsCalCommandCanvas :: SetBounds(const nsRect &aBounds)
{
  nsXPFCCanvas::SetBounds(aBounds);
  nsRect rect = aBounds;
  nscoord width = 0;

  if (mStaticTextField) {

    nsString text("COMMAND: ");

    nsFont font("Times", NS_FONT_STYLE_NORMAL,
		    NS_FONT_VARIANT_NORMAL,
		    NS_FONT_WEIGHT_BOLD,
		    0,
		    8);

    nsIFontMetrics * fm ;
    
    mStaticTextField->GetDeviceContext()->GetMetricsFor(font,fm);

    fm->GetWidth(text,width);

    rect.width = width;

    mStaticTextField->Invalidate(PR_FALSE);

    mStaticTextField->Resize(rect.x, rect.y, rect.width, rect.height, PR_FALSE);


  }

  if (mTextField) {

    rect = aBounds;

    rect.x += width;
    rect.width -= width;

    mTextField->Invalidate(PR_FALSE);

    mTextField->Resize(rect.x, rect.y, rect.width, rect.height, PR_FALSE);
  }

  return NS_OK;
}


nsEventStatus nsCalCommandCanvas :: OnPaint(nsGUIEvent *aEvent)
{
  mTextField->Invalidate(PR_FALSE);
  mStaticTextField->Invalidate(PR_FALSE);

  return nsEventStatus_eConsumeNoDefault;  
}

nsEventStatus nsCalCommandCanvas :: HandleEvent(nsGUIEvent *aEvent)
{
 switch (aEvent->message) 
  {
    case NS_KEY_UP:
  
      if (NS_VK_RETURN == ((nsKeyEvent*)aEvent)->keyCode) 
      {
        nsString text,reply;
        nsITextWidget * text_widget = nsnull;

        nsresult res = aEvent->widget->QueryInterface(kITextWidgetIID,(void**)&text_widget);

        if (res != NS_OK)
          return nsEventStatus_eIgnore;        

        text_widget->GetText(text, 1000);

        SendCommand(text,reply);

        NS_RELEASE(text_widget);
      }
      break;
  }
  return (nsEventStatus_eIgnore);
}

/*
 * The format of the string is:
 *
 * "CanvasName MethodName [Parameter1 Parameter2 .... ParameterN]"
 */

nsresult nsCalCommandCanvas :: SendCommand(nsString& aCommand, nsString& aReply)
{

  /*
   * Extract the CanvasName, method and params out
   */

  nsString name, method, param;

  aCommand.Trim(" \r\n\t");

  PRInt32 offset = aCommand.Find(' ');

  if (offset == kNotFound)
    return NS_OK;
    
  aCommand.Left(name,offset);
  aCommand.Cut(0,offset);
  aCommand.Trim(" \r\n\t",PR_TRUE,PR_FALSE);

  offset = aCommand.Find(' ');

  if (offset == kNotFound)
  {
    method = aCommand;
    param = "";
  } else
  {
    aCommand.Left(method,offset);
    aCommand.Cut(0,offset);
    aCommand.Trim(" \r\n\t",PR_TRUE,PR_FALSE);

    param = aCommand;
  }

  /*
   * Fint the canvas by this name
   */

  nsIXPFCCanvas * root = nsnull;
  nsIXPFCCanvas * canvas = nsnull;
  
  gXPFCToolkit->GetRootCanvas(&root);
  
  canvas = root->CanvasFromName(name);

  NS_RELEASE(root);

  if (canvas == nsnull)
    return NS_OK;

  /*
   * Send this command directly to the the canvas.
   */

  static NS_DEFINE_IID(kCXPFCMethodInvokerCommandCID, NS_XPFC_METHODINVOKER_COMMAND_CID);
  static NS_DEFINE_IID(kXPFCCommandIID, NS_IXPFC_COMMAND_IID);
  static NS_DEFINE_IID(kCXPFCObserverIID, NS_IXPFC_OBSERVER_IID);
  static NS_DEFINE_IID(kCXPFCSubjectIID, NS_IXPFC_SUBJECT_IID);

  nsXPFCMethodInvokerCommand * command;

  nsresult res = nsRepository::CreateInstance(kCXPFCMethodInvokerCommandCID, 
                                              nsnull, 
                                              kXPFCCommandIID, 
                                              (void **)&command);

  if (NS_OK != res)
    return res ;

  command->Init();

  command->mMethod = method;
  command->mParams = param;

  /*
   * Pass this Command onto the Observer interface of the target canvas directly.
   * There is no need to go through the ObserverManager since we have the
   * necessary info
   */

  nsIXPFCObserver * observer = nsnull;
  nsIXPFCSubject * subject = nsnull;

  res = canvas->QueryInterface(kCXPFCObserverIID, (void **)&observer);
  if (res == NS_OK)
    res = QueryInterface(kCXPFCSubjectIID, (void **)&subject);

  if (res == NS_OK)
    observer->Update(subject,command);

  aReply = command->mReply;

  NS_IF_RELEASE(command);
  NS_IF_RELEASE(observer);
  NS_IF_RELEASE(subject);
  
  return NS_OK;  
}

nsresult nsCalCommandCanvas::Attach(nsIXPFCObserver * aObserver)
{  
  return NS_OK;
}

nsresult nsCalCommandCanvas::Detach(nsIXPFCObserver * aObserver)
{  
  return NS_OK;
}

nsresult nsCalCommandCanvas::Notify(nsIXPFCCommand * aCommand)
{  
  return NS_OK;
}

nsresult nsCalCommandCanvas :: GetClassPreferredSize(nsSize& aSize)
{
  aSize.width  = DEFAULT_WIDTH;
  aSize.height = DEFAULT_HEIGHT;
  return (NS_OK);
}

