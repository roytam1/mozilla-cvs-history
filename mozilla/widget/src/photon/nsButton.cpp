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

#include "nsButton.h"
#include "nsToolkit.h"
#include "nsColor.h"
#include "nsGUIEvent.h"
#include "nsString.h"
#include "nsStringUtil.h"
#include <Pt.h>

#include "nsILookAndFeel.h"
#include "nsWidgetsCID.h"
#include "nsIComponentManager.h"

#include "nsIDeviceContext.h"
#include "nsIFontMetrics.h"

#include "nsPhWidgetLog.h"

static NS_DEFINE_IID(kLookAndFeelCID, NS_LOOKANDFEEL_CID);
static NS_DEFINE_IID(kILookAndFeelIID, NS_ILOOKANDFEEL_IID);


NS_IMPL_ADDREF_INHERITED(nsButton, nsWidget)
NS_IMPL_RELEASE_INHERITED(nsButton, nsWidget)
NS_IMPL_QUERY_INTERFACE_INHERITED(nsButton, nsWidget, nsIButton)
//-------------------------------------------------------------------------
//
// nsButton constructor
//
//-------------------------------------------------------------------------
nsButton::nsButton() : nsWidget(), nsIButton()
{
  NS_INIT_REFCNT();
}

//-------------------------------------------------------------------------
//
// nsButton destructor
//
//-------------------------------------------------------------------------
nsButton::~nsButton()
{
}

//-------------------------------------------------------------------------
//
// Set this button label
//
//-------------------------------------------------------------------------
NS_METHOD nsButton::SetLabel(const nsString& aText)
{
  mLabel = aText;

  if( mWidget )
  {
    PtArg_t arg;
    
    NS_ALLOC_STR_BUF(label, aText, aText.Length());

    PtSetArg( &arg, Pt_ARG_TEXT_STRING, label, 0 );
    PtSetResources( mWidget, 1, &arg );

    NS_FREE_STR_BUF(label);
  }

  return NS_OK;
}

//-------------------------------------------------------------------------
//
// Get this button label
//
//-------------------------------------------------------------------------
NS_METHOD nsButton::GetLabel(nsString& aBuffer)
{
  aBuffer = mLabel;
  return NS_OK;
}

//-------------------------------------------------------------------------
//
// move, paint, resizes message - ignore
//
//-------------------------------------------------------------------------
// Not Implemented
PRBool nsButton::OnMove(PRInt32, PRInt32)
{
  return PR_FALSE;
}

// Not Implemented
PRBool nsButton::OnPaint()
{
  return PR_FALSE;
}

// Not Implemented
PRBool nsButton::OnResize(nsRect &aWindowRect)
{
  return PR_FALSE;
}


/**
 * Renders the Button for Printing
 *
 **/
// Not Implemented
NS_METHOD nsButton::Paint(nsIRenderingContext& aRenderingContext,
                          const nsRect& aDirtyRect)
{
  return NS_OK;
}


NS_METHOD nsButton::CreateNative( PtWidget_t* aParent )
{
  nsresult  res = NS_ERROR_FAILURE;
  PtArg_t   arg[5];
  PhPoint_t pos;
  PhDim_t   dim;
  const unsigned short BorderWidth = 2;
  
  NS_PRECONDITION(aParent, "nsButton::CreateNative aParent is NULL");

  pos.x = mBounds.x;
  pos.y = mBounds.y;
  dim.w = mBounds.width - (2*BorderWidth); // Correct for border width
  dim.h = mBounds.height - (2*BorderWidth);

  PtSetArg( &arg[0], Pt_ARG_POS, &pos, 0 );
  PtSetArg( &arg[1], Pt_ARG_DIM, &dim, 0 );
  PtSetArg( &arg[2], Pt_ARG_BORDER_WIDTH, BorderWidth, 0 );

  mWidget = PtCreateWidget( PtButton, aParent, 3, arg );
  if( mWidget )
  {
    PtAddEventHandler( mWidget,
      Ph_EV_BUT_PRESS | Ph_EV_BUT_RELEASE |Ph_EV_BOUNDARY,
      RawEventHandler, this );

    res = NS_OK;
  }

  NS_POSTCONDITION(mWidget, "nsButton::CreateNative Failed to create Native Button");

  return res;  
}
  
