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


#include "nsPolygonFrame.h"

#include "nsIDOMElement.h"
#include "nsIContent.h"
#include "prtypes.h"
#include "nsIAtom.h"
#include "nsHTMLAtoms.h"
#include "nsIPresContext.h"
#include "nsIStyleContext.h"
#include "nsCSSRendering.h"
#include "nsINameSpaceManager.h"
#include "nsColor.h"
#include "nsIServiceManager.h"
#include "nsPoint.h"
#include "nsSVGAtoms.h"
#include "nsIDeviceContext.h"
//#include "nsPolygonCID.h"
//
// NS_NewPolygonFrame
//
// Wrapper for creating a new color picker
//
nsresult
NS_NewPolygonFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame)
{
  NS_PRECONDITION(aNewFrame, "null OUT ptr");
  if (nsnull == aNewFrame) {
    return NS_ERROR_NULL_POINTER;
  }
  nsPolygonFrame* it = new (aPresShell) nsPolygonFrame;
  if ( !it )
    return NS_ERROR_OUT_OF_MEMORY;
  *aNewFrame = it;
  return NS_OK;
}

// static NS_DEFINE_IID(kDefPolygonCID, NS_DEFCOLORPICKER_CID);

//
// nsPolygonFrame cntr
//
nsPolygonFrame::nsPolygonFrame() :
  mPnts(nsnull),
  mNumPnts(0),
  mX(0),
  mY(0)
{

}

nsPolygonFrame::~nsPolygonFrame()
{
  if (mPnts) {
    delete [] mPnts;
  }
}


NS_IMETHODIMP
nsPolygonFrame::Init(nsIPresContext*  aPresContext,
                         nsIContent*      aContent,
                         nsIFrame*        aParent,
                         nsIStyleContext* aContext,
                         nsIFrame*        aPrevInFlow)
{
 
  nsresult rv = nsLeafFrame::Init(aPresContext, aContent, aParent, aContext,
                                  aPrevInFlow);


  nsAutoString type;
  mContent->GetAttribute(kNameSpaceID_None, nsHTMLAtoms::type, type);

  if (type.EqualsIgnoreCase(NS_ConvertASCIItoUCS2("swatch")) || type.IsEmpty())
  {
    //mPolygon = new nsStdPolygon();
    //mPolygon->Init(mContent);
  }

  return rv;
}

//--------------------------------------------------------------
// Frames are not refcounted, no need to AddRef
NS_IMETHODIMP
nsPolygonFrame::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{
  NS_PRECONDITION(0 != aInstancePtr, "null ptr");
  if (NULL == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }

  if (aIID.Equals(NS_GET_IID(nsISVGFrame))) {
    *aInstancePtr = (void*) ((nsISVGFrame*) this);
    return NS_OK;
  }
  return nsLeafFrame::QueryInterface(aIID, aInstancePtr);
}


//-------------------------------------------------------------------
//-- Main Reflow for the Polygon
//-------------------------------------------------------------------
NS_IMETHODIMP 
nsPolygonFrame::Reflow(nsIPresContext*          aPresContext, 
                       nsHTMLReflowMetrics&     aDesiredSize,
                       const nsHTMLReflowState& aReflowState, 
                       nsReflowStatus&          aStatus)
{
  aStatus = NS_FRAME_COMPLETE;

  nsCOMPtr<nsIDeviceContext> dx;
  aPresContext->GetDeviceContext(getter_AddRefs(dx));
  float p2t   = 1.0;
  float scale = 1.0;
  if (dx) { 
    aPresContext->GetPixelsToTwips(&p2t);
    dx->GetCanonicalPixelScale(scale); 
  }  
  
  nsAutoString coordStr;
  nsresult res = mContent->GetAttribute(kNameSpaceID_None, nsSVGAtoms::x, coordStr);
  if (NS_SUCCEEDED(res)) {
    char * s = coordStr.ToNewCString();
    mX = NSIntPixelsToTwips(atoi(s), p2t*scale);
    delete [] s;
  }

  res = mContent->GetAttribute(kNameSpaceID_None, nsSVGAtoms::y, coordStr);
  if (NS_SUCCEEDED(res)) {
    char * s = coordStr.ToNewCString();
    mY = NSIntPixelsToTwips(atoi(s), p2t*scale);
    delete [] s;
  }

  if (mPoints.Count() == 0) {
    GetPoints();
  }
  nscoord maxWidth  = 0;
  nscoord maxHeight = 0;
  for (PRInt32 i=0;i<mNumPnts;i++) {
    maxWidth = PR_MAX(maxWidth, NSIntPixelsToTwips(mPnts[i].x, p2t*scale));
    maxHeight = PR_MAX(maxHeight, NSIntPixelsToTwips(mPnts[i].y, p2t*scale));
  }

  aDesiredSize.width  = maxWidth  + nscoord(p2t*scale);
  aDesiredSize.height = maxHeight + nscoord(p2t*scale);
  aDesiredSize.ascent = aDesiredSize.height;
  aDesiredSize.descent = 0;

  if (nsnull != aDesiredSize.maxElementSize) {
    aDesiredSize.maxElementSize->width  = aDesiredSize.width;
    aDesiredSize.maxElementSize->height = aDesiredSize.height;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsPolygonFrame::HandleEvent(nsIPresContext* aPresContext, 
                                nsGUIEvent*     aEvent,
                                nsEventStatus*  aEventStatus)
{
  NS_ENSURE_ARG_POINTER(aEventStatus);
  *aEventStatus = nsEventStatus_eConsumeDoDefault;
	if (aEvent->message == NS_MOUSE_LEFT_BUTTON_DOWN)
		HandleMouseDownEvent(aPresContext, aEvent, aEventStatus);

  return NS_OK;
}

nsresult
nsPolygonFrame::HandleMouseDownEvent(nsIPresContext* aPresContext, 
                                         nsGUIEvent*     aEvent,
                                         nsEventStatus*  aEventStatus)
{

  return NS_OK;
}

// XXX - Quick and Dirty impl to get somethinf working
// this should be rewritten
NS_METHOD 
nsPolygonFrame::GetPoints()
{
  nsAutoString pointsStr;
  nsresult res = mContent->GetAttribute(kNameSpaceID_None, nsSVGAtoms::points, pointsStr);

  char * ps = pointsStr.ToNewCString();
  char seps[]   = " ";
  char *token  = strtok(ps, seps);
  PRInt32 cnt = 0;
  nsPoint * pnt = nsnull;
  while (token != NULL) {
    if (cnt % 2 == 0) {
      pnt = new nsPoint;
      mPoints.AppendElement((void*)pnt);
      pnt->x = atoi(token);
    } else {
      pnt->y = atoi(token);
    }
    token = strtok( NULL, seps );
    cnt++;
  }

  delete [] ps;

  mNumPnts = mPoints.Count()+1;
  mPnts    = new nsPoint[mNumPnts];
  for (cnt=0;cnt<mNumPnts-1;cnt++) {
    nsPoint * pnt = (nsPoint*)mPoints.ElementAt(cnt);
    mPnts[cnt] = *pnt;
    delete pnt;
  }
  mPnts[mNumPnts-1] = mPnts[0];

  return NS_OK;
}

NS_IMETHODIMP nsPolygonFrame::SetProperty(nsIPresContext* aPresContext, 
                                          nsIAtom* aName, 
                                          const nsString& aValue)
{
  if (aName == nsSVGAtoms::points) {
  } else if (aName == nsSVGAtoms::x) {
  } else if (aName == nsSVGAtoms::y) {
  }
  return NS_OK;
}

NS_IMETHODIMP
nsPolygonFrame::AttributeChanged(nsIPresContext* aPresContext,
                                          nsIContent*     aChild,
                                          PRInt32         aNameSpaceID,
                                          nsIAtom*        aAttribute,
                                          PRInt32         aHint)
{
  return nsLeafFrame::AttributeChanged(aPresContext, aChild, aNameSpaceID, aAttribute, aHint);
}
  
NS_METHOD nsPolygonFrame::RenderPoints(nsIRenderingContext& aRenderingContext,
                                       const nsPoint aPoints[], PRInt32 aNumPoints)
{
  //nsAutoString fillStr;
  //nsresult res = mContent->GetAttribute(kNameSpaceID_None, nsSVGAtoms::fill, fillStr);
  //if (NS_SUCCEEDED(res)) {
    aRenderingContext.FillPolygon(aPoints, aNumPoints);
  //} else {
  //  aRenderingContext.DrawPolygon(aPoints, aNumPoints);
  //}
  return NS_OK;
}

//
// Paint
//
//
NS_METHOD 
nsPolygonFrame::Paint(nsIPresContext* aPresContext,
                          nsIRenderingContext& aRenderingContext,
                          const nsRect& aDirtyRect,
                          nsFramePaintLayer aWhichLayer)
{
  const nsStyleDisplay* disp = (const nsStyleDisplay*)
  mStyleContext->GetStyleData(eStyleStruct_Display);

  // if we aren't visible then we are done.
  if (!disp->IsVisibleOrCollapsed()) 
	   return NS_OK;  

  // if we are visible then tell our superclass to paint
  nsLeafFrame::Paint(aPresContext, aRenderingContext, aDirtyRect,
                     aWhichLayer);

  // get our border
	const nsStyleSpacing* spacing = (const nsStyleSpacing*)mStyleContext->GetStyleData(eStyleStruct_Spacing);
	nsMargin border(0,0,0,0);
	spacing->CalcBorderFor(this, border);

  
  // XXX - Color needs to comes from new style property fill
  // and not mColor
  const nsStyleColor* colorStyle = (const nsStyleColor*)mStyleContext->GetStyleData(eStyleStruct_Color);
  nscolor color = colorStyle->mColor;
  

  aRenderingContext.PushState();

  // set the clip region
  nsRect rect;

  PRBool clipState;
  GetRect(rect);

  // Clip so we don't render outside the inner rect
  aRenderingContext.SetClipRect(rect, nsClipCombine_kReplace, clipState);
  aRenderingContext.SetColor(color);
  
  ///////////////////////////////////////////
  // XXX - This is all just a quick hack
  // needs to be rewritten
  nsCOMPtr<nsIDeviceContext> dx;
  aPresContext->GetDeviceContext(getter_AddRefs(dx));
  float p2t   = 1.0;
  float scale = 1.0;
  if (dx) { 
    aPresContext->GetPixelsToTwips(&p2t);
    dx->GetCanonicalPixelScale(scale); 
  }  
  
  nsPoint points[256];
  for (PRInt32 i=0;i<mNumPnts;i++) {
    points[i] = mPnts[i];
    points[i].x = NSIntPixelsToTwips(points[i].x, p2t*scale)+mX;
    points[i].y = NSIntPixelsToTwips(points[i].y, p2t*scale)+mY;
    //printf("%p   Draw Poly: %d,%d\n", this, points[i].x, points[i].y);
  }
  // XXX - down to here

  RenderPoints(aRenderingContext, points, mNumPnts);

  aRenderingContext.PopState(clipState);

  return NS_OK;
}


//
// GetDesiredSize
//
// For now, be as big as CSS wants us to be, or some small default size.
//
void
nsPolygonFrame::GetDesiredSize(nsIPresContext* aPresContext,
                               const nsHTMLReflowState& aReflowState,
                               nsHTMLReflowMetrics& aDesiredSize)
{
  NS_ASSERTION(0, "Who called this? and Why?");
} // GetDesiredSize
