/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

//---- Factory Includes & Stuff -----// 
#include "nsIFactory.h" 
#include "nsIComponentManager.h" 
#include "nsIServiceManager.h"
#include "nsIEventQueueService.h"
#include "nsIEventQueue.h"
#include "nsGfxCIID.h" 

#include "nsWidgetsCID.h" 
#include "nsIWidget.h"
#include "nsGUIEvent.h"
#include "nsString.h"
#include "nsRect.h"
#include "nsIRenderingContext.h"

#include "nsIDeviceContext.h"
#include "nsFont.h"
#include "nsIComponentManager.h"
#include "nsWidgetsCID.h"
#include "nsIAppShell.h"
#include "nslog.h"

NS_IMPL_LOG(nsCoverageLog)
#define PRINTF(args) NS_LOG_PRINTF(nsCoverageLog, args)
#define FLUSH()      NS_LOG_FLUSH(nsCoverageLog)

//------------------------------------------------------------------------------

nsIEventQueueService *gEventQService = nsnull;

nsIWidget         *gWindow = NULL;

#ifdef XP_PC
#define XPCOM_DLL "xpcom32.dll"
#define WIDGET_DLL "gkwidget.dll"
#define GFXWIN_DLL "gkgfxwin.dll"
#define TEXT_HEIGHT 25
#endif

#if defined(XP_UNIX) || defined(XP_BEOS)
#define XPCOM_DLL "libxpcom"MOZ_DLL_SUFFIX
#ifndef WIDGET_DLL
#define WIDGET_DLL "libwidget_gtk"MOZ_DLL_SUFFIX
#endif
#ifndef GFXWIN_DLL
#define GFXWIN_DLL "libgfx_gtk"MOZ_DLL_SUFFIX
#endif
#define TEXT_HEIGHT 30
#endif

#ifdef XP_MAC
#define XPCOM_DLL "XPCOM_DLL"
#define WIDGET_DLL "WIDGET_DLL"
#define GFXWIN_DLL "GFXWIN_DLL"
#define TEXT_HEIGHT 30
#endif

// class ids
static NS_DEFINE_IID(kCWindowCID, NS_WINDOW_CID);
static NS_DEFINE_IID(kLookAndFeelCID, NS_LOOKANDFEEL_CID);

static NS_DEFINE_IID(kEventQueueServiceCID, NS_EVENTQUEUESERVICE_CID);
static NS_DEFINE_IID(kEventQueueCID, NS_EVENTQUEUE_CID);
static NS_DEFINE_IID(kCAppShellCID, NS_APPSHELL_CID);
static NS_DEFINE_IID(kCToolkitCID, NS_TOOLKIT_CID);

// interface ids
static NS_DEFINE_IID(kISupportsIID,       NS_ISUPPORTS_IID);
static NS_DEFINE_IID(kIWidgetIID,         NS_IWIDGET_IID);
static NS_DEFINE_IID(kIEventQueueServiceIID, NS_IEVENTQUEUESERVICE_IID);
static NS_DEFINE_IID(kIAppShellIID,       NS_IAPPSHELL_IID);

// Scroll offsets
nscoord gOffsetX = 0;
nscoord gOffsetY = 0;

nscoord pixelLoc(nscoord aPixelValue, float aP2t, nscoord aOffset)
{
  return NSToIntRound((aPixelValue + aOffset) * aP2t);
}


PRInt32
drawtest(nsIRenderingContext *aSurface)
{
nsFont  *font;
nsPoint *pointlist;

   // Get the size of a pixel.
  nsIDeviceContext* deviceContext;
  aSurface->GetDeviceContext(deviceContext);
  float p2t; // pixel to twips conversion
  deviceContext->GetDevUnitsToAppUnits(p2t);
  NS_RELEASE(deviceContext);

  font = new nsFont("Times", NS_FONT_STYLE_NORMAL,NS_FONT_VARIANT_NORMAL,NS_FONT_WEIGHT_BOLD,0,12);
  aSurface->SetFont(*font);

   // Clear surface.
  nsRect rect;
  gWindow->GetClientBounds(rect);
  aSurface->SetColor(NS_RGB(255, 255, 255));
  aSurface->FillRect(0,0,rect.width,rect.height);

  aSurface->SetColor(NS_RGB(255, 0, 0));
  aSurface->DrawString("GFX - Pixel coverage test\0",150,30);
  aSurface->DrawString("(Use (u, d, r, l) keys to scroll the window contents)\0",150,50);
            
  aSurface->Translate(gOffsetX, gOffsetY);

   // Starting coordinates
  nscoord ox = 90; // pixels
  nscoord oy = 100; // pixels
   // Spacing between tests
  nscoord yspacing = 50; // pixels
 
   //--------------
   // DrawLine TEST
   //--------------

  aSurface->SetColor(NS_RGB(255, 0, 0));
  aSurface->DrawLine(pixelLoc(12, p2t, ox),
                     pixelLoc(0,  p2t, oy), 
                     pixelLoc(12, p2t, ox), 
                     pixelLoc(10, p2t, oy));

  aSurface->SetColor(NS_RGB(0, 0, 0));
  aSurface->DrawLine(pixelLoc(0, p2t, ox),
                     pixelLoc(10,p2t, oy), 
                     pixelLoc(12,p2t, ox), 
                     pixelLoc(10, p2t, oy));
  aSurface->DrawString("DrawLine - There should be a one pixel gap where the red and black lines meet\0",ox + 30, oy);

  oy += yspacing;

 //------------------
  // DrawPolyline TEST
  //------------------


  pointlist = new nsPoint[5];
  pointlist[0].x = pixelLoc(0, p2t, ox);
  pointlist[0].y = pixelLoc(0, p2t, oy);

  pointlist[1].x = pixelLoc(10, p2t, ox);
  pointlist[1].y = pixelLoc(0, p2t, oy);

  pointlist[2].x = pixelLoc(10, p2t, ox);
  pointlist[2].y = pixelLoc(10, p2t, oy);

  pointlist[3].x = pixelLoc(0, p2t, ox);
  pointlist[3].y = pixelLoc(10, p2t, oy);

  pointlist[4].x = pixelLoc(0, p2t, ox);
  pointlist[4].y = pixelLoc(1, p2t, oy);

  aSurface->DrawPolyline(pointlist,5);
  aSurface->DrawString("DrawPolyline - There should be a one pixel gap in the rectangle\0", ox + 30, oy);
  delete [] pointlist;

  oy += yspacing;


   //--------------
   // FillRect TEST
   //--------------
   
  aSurface->SetColor(NS_RGB(255, 0, 0));
  aSurface->DrawLine(pixelLoc(9,p2t, ox),
                     pixelLoc(0,p2t, oy), 
                     pixelLoc(9,p2t,  ox), 
                     pixelLoc(30,p2t, oy));

  aSurface->SetColor(NS_RGB(0, 0, 0));
  aSurface->FillRect(pixelLoc(0,  p2t, ox),
                     pixelLoc(10,  p2t, oy),
                     pixelLoc(10, p2t,  0),
                     pixelLoc(10, p2t,  0));


  aSurface->DrawString("FillRect - The red line should be at the right edge under the rectangle\0", ox + 30, oy);

  oy += yspacing; 

  //--------------
  // DrawRect TEST
  //--------------

  aSurface->SetColor(NS_RGB(255, 0, 0));
  aSurface->DrawLine(pixelLoc(9,p2t, ox),
                     pixelLoc(0,p2t, oy), 
                     pixelLoc(9,p2t,  ox), 
                     pixelLoc(30,p2t, oy));

  aSurface->SetColor(NS_RGB(0, 0, 0));
  aSurface->DrawRect(pixelLoc(0,  p2t, ox),
                     pixelLoc(10,  p2t, oy),
                     pixelLoc(10, p2t,  0),
                     pixelLoc(10, p2t,  0));


  aSurface->DrawString("DrawRect - The red line should be at the right edge under the rectangle\0", ox + 30, oy);
  oy += yspacing;

 

 /*
#ifdef WINDOWSBROKEN
  pointlist = new nsPoint[5];
  pointlist[0].x = 200;pointlist[0].y = 200;
  pointlist[1].x = 250;pointlist[1].y = 200;
  pointlist[2].x = 240;pointlist[2].y = 220;
  pointlist[3].x = 260;pointlist[3].y = 240;
  pointlist[4].x = 225;pointlist[4].y = 225;
  aSurface->DrawPolygon(pointlist,6);
  aSurface->DrawString("This is an open Polygon\0",250,200);
  delete [] pointlist;
#endif  

  aSurface->DrawEllipse(30, 150,50,100);
  aSurface->DrawString("This is an Ellipse\0",30,140);
*/

  return(30);
}




/**--------------------------------------------------------------------------------
 * Main Handler
 *--------------------------------------------------------------------------------
 */
nsEventStatus PR_CALLBACK HandleEvent(nsGUIEvent *aEvent)
{ 
   nsEventStatus result = nsEventStatus_eIgnore;

   switch(aEvent->message) {

        case NS_PAINT: 
           
              // paint the background
            if (aEvent->widget == gWindow) {
                nsIRenderingContext *drawCtx = ((nsPaintEvent*)aEvent)->renderingContext;
                drawCtx->SetColor(aEvent->widget->GetBackgroundColor());
                drawCtx->FillRect(*(((nsPaintEvent*)aEvent)->rect));
                drawtest(drawCtx);

                return nsEventStatus_eIgnore;
            }

            break;

        case NS_KEY_UP: {
            nsKeyEvent * ke = (nsKeyEvent*)aEvent;
            char str[256];
            sprintf(str, "Key Event Key Code[%d] Key [%c] Shift [%s] Control [%s] Alt [%s]",
              ke->keyCode, ke->keyCode, 
              (ke->isShift?"Pressed":"Released"),
              (ke->isControl?"Pressed":"Released"),
              (ke->isAlt?"Pressed":"Released"));
            PRINTF(("%s\n", str));
            switch(ke->keyCode) {
               case 'U':
                 gOffsetY -= 9;
                 gWindow->Invalidate(PR_FALSE);
               break;

               case 'D':
                 gOffsetY += 10;
                 gWindow->Invalidate(PR_FALSE);
                break;

               case 'R':
                 gOffsetX += 9;
                 gWindow->Invalidate(PR_FALSE);
               break;

               case 'L':
                 gOffsetX -= 10;
                 gWindow->Invalidate(PR_FALSE);
               break;
            }
            }
            break;
        
        case NS_DESTROY:
			      exit(0); // for now
            break;

        default:
            result = nsEventStatus_eIgnore;
    }


    return result;
}


/**--------------------------------------------------------------------------------
 *
 */
nsresult CoverageTest(int *argc, char **argv)
{
    // register xpcom classes
    nsComponentManager::RegisterComponentLib(kEventQueueServiceCID, NULL, NULL, XPCOM_DLL, PR_FALSE, PR_FALSE);
    nsComponentManager::RegisterComponentLib(kEventQueueCID, NULL, NULL, XPCOM_DLL, PR_FALSE, PR_FALSE);
    
    // register widget classes
    nsComponentManager::RegisterComponentLib(kLookAndFeelCID, NULL, NULL, WIDGET_DLL, PR_FALSE, PR_FALSE);
    nsComponentManager::RegisterComponentLib(kCWindowCID, NULL, NULL, WIDGET_DLL, PR_FALSE, PR_FALSE);
    
    nsComponentManager::RegisterComponentLib(kCAppShellCID, NULL, NULL, WIDGET_DLL, PR_FALSE, PR_FALSE);
    nsComponentManager::RegisterComponentLib(kCToolkitCID, NULL, NULL, WIDGET_DLL, PR_FALSE, PR_FALSE);
    static NS_DEFINE_IID(kCRenderingContextIID, NS_RENDERING_CONTEXT_CID); 
    static NS_DEFINE_IID(kCDeviceContextIID, NS_DEVICE_CONTEXT_CID); 
    static NS_DEFINE_IID(kCFontMetricsIID, NS_FONT_METRICS_CID); 
    static NS_DEFINE_IID(kCImageIID, NS_IMAGE_CID); 


    nsComponentManager::RegisterComponentLib(kCRenderingContextIID, NULL, NULL, GFXWIN_DLL, PR_FALSE, PR_FALSE); 
    nsComponentManager::RegisterComponentLib(kCDeviceContextIID, NULL, NULL, GFXWIN_DLL, PR_FALSE, PR_FALSE); 
    nsComponentManager::RegisterComponentLib(kCFontMetricsIID, NULL, NULL, GFXWIN_DLL, PR_FALSE, PR_FALSE); 
    nsComponentManager::RegisterComponentLib(kCImageIID, NULL, NULL, GFXWIN_DLL, PR_FALSE, PR_FALSE); 

    nsresult  res;

    // Create the Event Queue for the UI thread...
    res = nsServiceManager::GetService(kEventQueueServiceCID,
                                       kIEventQueueServiceIID,
                                       (nsISupports **)&gEventQService);

    if (NS_OK != res) {
        NS_ASSERTION(PR_FALSE, "Could not obtain the event queue service");
        return res;
    }

    res = gEventQService->CreateThreadEventQueue();
    if (NS_OK != res) {
        NS_ASSERTION(PR_FALSE, "Could not create the event queue for the thread");
	      return res;
    }

      // Create a application shell
    nsIAppShell *appShell;
    nsComponentManager::CreateInstance(kCAppShellCID, nsnull, kIAppShellIID, (void**)&appShell);
    if (appShell != nsnull) {
      fputs("Created AppShell\n", stderr);
      appShell->Create(argc, argv);
    } else {
      PRINTF(("AppShell is null!\n"));
    }

    nsIDeviceContext* deviceContext = 0;

    // Create a device context for the widgets

    static NS_DEFINE_IID(kDeviceContextCID, NS_DEVICE_CONTEXT_CID);
    static NS_DEFINE_IID(kDeviceContextIID, NS_IDEVICE_CONTEXT_IID);

    //
    // create the main window
    //
    nsComponentManager::CreateInstance(kCWindowCID, nsnull, kIWidgetIID, (void**)&gWindow);
    nsRect rect(100, 100, 600, 700);
    gWindow->Create((nsIWidget*) nsnull, rect, HandleEvent, 
                   (nsIDeviceContext *) nsnull,
                   appShell);
    gWindow->SetTitle("Pixel coverage test");

    //
    // Create Device Context based on main window
    //
    res = nsComponentManager::CreateInstance(kDeviceContextCID, nsnull, kDeviceContextIID, (void **)&deviceContext);

    if (NS_OK == res)
    {
      deviceContext->Init(gWindow->GetNativeData(NS_NATIVE_WIDGET));
      NS_ADDREF(deviceContext);
    }

    gWindow->Show(PR_TRUE);

 
    return(appShell->Run());
}


