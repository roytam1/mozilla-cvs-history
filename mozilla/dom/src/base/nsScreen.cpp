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
 *    Travis Bogard <travis@netscape.com> 
 */

#include "nscore.h"
#include "nsScreen.h"
#include "nsIDOMWindow.h"
#include "nsIScriptGlobalObject.h"
#include "nsIDocShell.h"
#include "nsIDeviceContext.h"
#include "nsIPresContext.h"
#include "nsCOMPtr.h"
#include "nsIDocumentViewer.h"
#include "nsIDocumentLoader.h"
#include "nsDOMClassInfo.h"


//
//  Screen class implementation 
//
ScreenImpl::ScreenImpl(nsIDocShell* aDocShell) : mDocShell(aDocShell)
{
  NS_INIT_REFCNT();
}

ScreenImpl::~ScreenImpl()
{
}


// XPConnect interface list for ScreenImpl
NS_CLASSINFO_MAP_BEGIN(Screen)
  NS_CLASSINFO_MAP_ENTRY(nsIDOMScreen)
NS_CLASSINFO_MAP_END


// QueryInterface implementation for ScreenImpl
NS_INTERFACE_MAP_BEGIN(ScreenImpl)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY(nsIDOMScreen)
  NS_INTERFACE_MAP_ENTRY_DOM_CLASSINFO(Screen)
NS_INTERFACE_MAP_END


NS_IMPL_ADDREF(ScreenImpl)
NS_IMPL_RELEASE(ScreenImpl)


NS_IMETHODIMP ScreenImpl::SetDocShell(nsIDocShell* aDocShell)
{
   mDocShell = aDocShell; // Weak Reference
   return NS_OK;
}

NS_IMETHODIMP
ScreenImpl::GetTop(PRInt32* aTop)
{
	nsCOMPtr<nsIDeviceContext> context ( getter_AddRefs(GetDeviceContext()) );
	if ( context )
	{
		nsRect rect;
		context->GetRect( rect );
		float devUnits;
		context->GetDevUnitsToAppUnits(devUnits);
		*aTop = NSToIntRound(float(rect.y) / devUnits );
		return NS_OK;
	}
  *aTop = -1;
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
ScreenImpl::GetLeft(PRInt32* aLeft)
{
	nsCOMPtr<nsIDeviceContext> context ( getter_AddRefs(GetDeviceContext()) );
	if ( context )
	{
		nsRect rect;
		context->GetRect( rect );
		float devUnits;
		context->GetDevUnitsToAppUnits(devUnits);
		*aLeft = NSToIntRound(float(rect.x) / devUnits );
		return NS_OK;
	}
  *aLeft = -1;
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
ScreenImpl::GetWidth(PRInt32* aWidth)
{
	nsCOMPtr<nsIDeviceContext> context ( getter_AddRefs(GetDeviceContext()) );
	if ( context )
	{
		PRInt32 height;
		context->GetDeviceSurfaceDimensions( *aWidth, height  );
		float devUnits;
		context->GetDevUnitsToAppUnits(devUnits);
		*aWidth = NSToIntRound(float( *aWidth) / devUnits );
		return NS_OK;
	}

  *aWidth = -1;
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
ScreenImpl::GetHeight(PRInt32* aHeight)
{
	nsCOMPtr<nsIDeviceContext> context ( getter_AddRefs(GetDeviceContext()) );
	if ( context )
	{
		PRInt32 width;
		context->GetDeviceSurfaceDimensions( width, *aHeight  );
		float devUnits;
		context->GetDevUnitsToAppUnits(devUnits);
		*aHeight = NSToIntRound(float( *aHeight) / devUnits );
		return NS_OK;
	}
	
  *aHeight = -1;
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
ScreenImpl::GetPixelDepth(PRInt32* aPixelDepth)
{
	nsCOMPtr<nsIDeviceContext> context ( getter_AddRefs(GetDeviceContext()) );
	if ( context )
	{
		PRUint32 depth;
		context->GetDepth( depth  );
		*aPixelDepth = depth;
		return NS_OK;
	}
  //XXX not implmented
  *aPixelDepth = -1;
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
ScreenImpl::GetColorDepth(PRInt32* aColorDepth)
{
	nsCOMPtr<nsIDeviceContext> context ( getter_AddRefs(GetDeviceContext()) );
	if ( context )
	{
		PRUint32 depth;
		context->GetDepth( depth  );
		*aColorDepth = depth;
		return NS_OK;
	}
  
  *aColorDepth = -1;
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
ScreenImpl::GetAvailWidth(PRInt32* aAvailWidth)
{
	nsCOMPtr<nsIDeviceContext> context ( getter_AddRefs(GetDeviceContext()) );
  if ( context )
  {
	  nsRect rect;
	  context->GetClientRect( rect  );
	  float devUnits;
	  context->GetDevUnitsToAppUnits(devUnits);
	  *aAvailWidth = NSToIntRound(float( rect.width) / devUnits );
	  return NS_OK;
  }

  *aAvailWidth = -1;
  return NS_ERROR_FAILURE;

}

NS_IMETHODIMP
ScreenImpl::GetAvailHeight(PRInt32* aAvailHeight)
{
	nsCOMPtr<nsIDeviceContext> context ( getter_AddRefs(GetDeviceContext()) );
  if ( context )
  {
	  nsRect rect;
	  context->GetClientRect( rect  );
	  float devUnits;
	  context->GetDevUnitsToAppUnits(devUnits);
	  *aAvailHeight = NSToIntRound(float( rect.height) / devUnits );
	  return NS_OK;
  }

  *aAvailHeight = -1;
  return NS_ERROR_FAILURE;

}

NS_IMETHODIMP
ScreenImpl::GetAvailLeft(PRInt32* aAvailLeft)
{
	nsCOMPtr<nsIDeviceContext> context ( getter_AddRefs(GetDeviceContext()) );
  if ( context )
  {
	  nsRect rect;
	  context->GetClientRect( rect  );
	  float devUnits;
	  context->GetDevUnitsToAppUnits(devUnits);
	  *aAvailLeft = NSToIntRound(float( rect.x) / devUnits );
	  return NS_OK;
 }

  *aAvailLeft = -1;
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
ScreenImpl::GetAvailTop(PRInt32* aAvailTop)
{
	nsCOMPtr<nsIDeviceContext> context ( getter_AddRefs(GetDeviceContext()) );
  if ( context )
  {
	  nsRect rect;
	  context->GetClientRect( rect  );
	  float devUnits;
	  context->GetDevUnitsToAppUnits(devUnits);
	  *aAvailTop = NSToIntRound(float( rect.y) / devUnits );
	  return NS_OK;
  }

  *aAvailTop = -1;
  return NS_ERROR_FAILURE;
}

nsIDeviceContext* ScreenImpl::GetDeviceContext()
{
  
   if(!mDocShell)
      return nsnull;

	nsCOMPtr<nsIContentViewer> contentViewer;
   mDocShell->GetContentViewer(getter_AddRefs(contentViewer));

   nsCOMPtr<nsIDocumentViewer> docViewer(do_QueryInterface(contentViewer));
   if(!docViewer)
      return nsnull;

   nsCOMPtr<nsIPresContext> presContext;
   docViewer->GetPresContext(*getter_AddRefs(presContext));

	nsIDeviceContext* context = nsnull;
   if(presContext)
      presContext->GetDeviceContext(&context);

   return context;
}


