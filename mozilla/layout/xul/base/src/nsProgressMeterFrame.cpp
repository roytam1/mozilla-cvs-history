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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

//
// Eric Vaughan
// Netscape Communications
//
// See documentation in associated header file
//

static float STRIPE_SKEW  = 1.0;      // pixels
static int   STRIPE_WIDTH = 20;       // pixels
static int   ANIMATION_INCREMENT = 4; // pixels
static int   ANIMATION_SPEED = 50;    // miliseconds

#include "nsINameSpaceManager.h"
#include "nsProgressMeterFrame.h"
#include "nsIStyleContext.h"
#include "nsCSSRendering.h"
#include "nsIContent.h"
#include "nsIPresContext.h"
#include "nsHTMLAtoms.h"
#include "nsXULAtoms.h"
#include "nsINameSpaceManager.h"
#include "nsITimerCallback.h"
#include "nsITimer.h"
#include "nsIView.h"
#include "nsIViewManager.h"
#include "nsIReflowCommand.h"
#include "nsHTMLParts.h"
#include "nsIPresShell.h"

class StripeTimer : public nsITimerCallback {
public:
  StripeTimer();
  virtual ~StripeTimer();

  NS_DECL_ISUPPORTS

  void AddFrame(nsIPresContext* aPresContext, nsProgressMeterFrame* aFrame);

  PRBool RemoveFrame(nsProgressMeterFrame* aFrame);

  PRInt32 FrameCount();

  void Start();

  void Stop();

  virtual void Notify(nsITimer *timer);

  PRInt32 GetFrameData(nsProgressMeterFrame* aFrame);

  struct FrameData {
    nsIPresContext*       mPresContext;  // pres context associated with the frame
    nsProgressMeterFrame* mFrame;


    FrameData(nsIPresContext*       aPresContext,
              nsProgressMeterFrame* aFrame)
      : mPresContext(aPresContext), mFrame(aFrame) {}
  };

  nsITimer* mTimer;
  nsVoidArray mFrames;
};

static StripeTimer* gStripeAnimator;

StripeTimer::StripeTimer()
{
  NS_INIT_REFCNT();
  mTimer = nsnull;
}

StripeTimer::~StripeTimer()
{
  Stop();
}

void StripeTimer::Start()
{
  nsresult rv = NS_NewTimer(&mTimer);
  if (NS_OK == rv) {
    mTimer->Init(this, ANIMATION_SPEED);
  }
}

void StripeTimer::Stop()
{
  if (nsnull != mTimer) {
    mTimer->Cancel();
    NS_RELEASE(mTimer);
  }
}

static NS_DEFINE_IID(kITimerCallbackIID, NS_ITIMERCALLBACK_IID);
NS_IMPL_ISUPPORTS(StripeTimer, kITimerCallbackIID);

PRInt32 StripeTimer::GetFrameData(nsProgressMeterFrame* aFrame)
{
  PRInt32 i, n = mFrames.Count();
  for (i = 0; i < n; i++) {
    FrameData* frameData = (FrameData*) mFrames.ElementAt(i);

    if (frameData->mFrame == aFrame) {
      return i;
    }
  }

  return -1;
}

void StripeTimer::AddFrame(nsIPresContext* aPresContext, nsProgressMeterFrame* aFrame) {

  // see if the frame is already here.
  if (GetFrameData(aFrame) > -1)
	  return;

  // if not add it.
  FrameData* frameData = new FrameData(aPresContext, aFrame);
  mFrames.AppendElement(frameData);
  if (1 == mFrames.Count()) {
    Start();
  }
}

PRBool StripeTimer::RemoveFrame(nsProgressMeterFrame* aFrame) {
  PRBool  rv = PR_FALSE;
  PRInt32 i = GetFrameData(aFrame);

  if (i > -1) {
    FrameData*  frameData = (FrameData*)mFrames.ElementAt(i);
    rv = mFrames.RemoveElementAt(i);
    delete frameData;
  }
  if (0 == mFrames.Count()) {
    Stop();
  }
  return rv;
}

PRInt32 StripeTimer::FrameCount() {
  return mFrames.Count();
}

void StripeTimer::Notify(nsITimer *timer)
{
  // XXX hack to get auto-repeating timers; restart before doing
  // expensive work so that time between ticks is more even
  Stop();
  Start();

  PRInt32 i, n = mFrames.Count();
  for (i = 0; i < n; i++) {
    FrameData*  frameData = (FrameData*) mFrames.ElementAt(i);
    frameData->mFrame->animate();

    // Determine damaged area and tell view manager to redraw it
    nsPoint offset;
    nsRect bounds;
    frameData->mFrame->GetRect(bounds);
    nsIView* view;
    frameData->mFrame->GetOffsetFromView(frameData->mPresContext, offset, &view);
    nsIViewManager* vm;
    view->GetViewManager(vm);
    bounds.x = offset.x;
    bounds.y = offset.y;
    vm->UpdateView(view, bounds, 0);
    NS_RELEASE(vm);
  }
}

//
// NS_NewToolbarFrame
//
// Creates a new Toolbar frame and returns it in |aNewFrame|
//
nsresult
NS_NewProgressMeterFrame ( nsIPresShell* aPresShell, nsIFrame** aNewFrame )
{
  NS_PRECONDITION(aNewFrame, "null OUT ptr");
  if (nsnull == aNewFrame) {
    return NS_ERROR_NULL_POINTER;
  }
  nsProgressMeterFrame* it = new (aPresShell) nsProgressMeterFrame;
  if (nsnull == it)
    return NS_ERROR_OUT_OF_MEMORY;

  // it->SetFlags(aFlags);
  *aNewFrame = it;
  return NS_OK;
  
} // NS_NewProgressMeterFrame

//
// nsProgressMeterFrame cntr
//
// Init, if necessary
//
nsProgressMeterFrame :: nsProgressMeterFrame ( )
{
	// if we haven't created the timer create it.
	if (nsnull == gStripeAnimator) {
	gStripeAnimator = new StripeTimer();
	}

	NS_ADDREF(gStripeAnimator);

	mProgress = float(0.0);
	mHorizontal = PR_TRUE;
	mUndetermined = PR_FALSE;
	mStripeOffset = STRIPE_WIDTH;
}

//
// nsProgressMeterFrame dstr
//
// Cleanup, if necessary
//
nsProgressMeterFrame :: ~nsProgressMeterFrame ( )
{

    gStripeAnimator->RemoveFrame(this);
}

NS_IMETHODIMP
nsProgressMeterFrame::Init(nsIPresContext*  aPresContext,
                   nsIContent*      aContent,
                   nsIFrame*        aParent,
                   nsIStyleContext* aContext,
                   nsIFrame*        aPrevInFlow)
{
 
  nsresult  rv = nsLeafFrame::Init(aPresContext, aContent, aParent, aContext,
                                   aPrevInFlow);

  // get the value
  nsAutoString value;
  if ((NS_CONTENT_ATTR_HAS_VALUE == mContent->GetAttribute(kNameSpaceID_None, nsHTMLAtoms::value, value)) &&
      (value.Length() > 0)) {
	  setProgress(value);
  }


   // get the alignment
  nsAutoString align;
  mContent->GetAttribute(kNameSpaceID_None, nsHTMLAtoms::align, align);
  setAlignment(align); 

  // get the mode
  nsAutoString mode;
  mContent->GetAttribute(kNameSpaceID_None, nsXULAtoms::mode, mode);
  setMode(mode); 

  nsCOMPtr<nsIAtom> barPseudo ( dont_AddRef(NS_NewAtom(":progressmeter-stripe")) );
  nsIStyleContext* barStyle = nsnull;
  aPresContext->ProbePseudoStyleContextFor(aContent, barPseudo, aContext, 
                                          PR_FALSE, &barStyle);
  mBarStyle = barStyle;

  return rv;
}

void
nsProgressMeterFrame::setProgress(nsAutoString progress)
{
	// convert to and integer
	PRInt32 error;
	PRInt32 v = progress.ToInteger(&error);
 
	// adjust to 0 and 100
	if (v < 0)
		v = 0;
	else if (v > 100)
		v = 100;

//	printf("ProgressMeter value=%d\n", v);
    mProgress = float(v)/float(100);
}

void
nsProgressMeterFrame::setSize(nsAutoString sizeString, int& size, PRBool& isPercent)
{
	// -1 means unset
	size = -1;

	int length = sizeString.Length();
	if (length == 0)
		return;

	char w[100];
	sizeString.ToCString(w,100);
    
	if (w[length-1] == '%')
		isPercent = PR_TRUE;
	else
		isPercent = PR_FALSE;

	// convert to and integer
	PRInt32 error;
	PRInt32 v = sizeString.ToInteger(&error);
 
	// adjust to 0 and 100
	if (isPercent) {
		if (v < 0)
			v = 0;
		else if (v > 100)
			v = 100;
	}

	// printf("size=%d\n", v);

  size = v;
}



void
nsProgressMeterFrame::setAlignment(nsAutoString progress)
{
    if (progress.EqualsIgnoreCase("vertical"))
		  mHorizontal = PR_FALSE;
    else
		  mHorizontal = PR_TRUE;
}

void
nsProgressMeterFrame::setMode(nsAutoString mode)
{
    if (mode.EqualsIgnoreCase("undetermined"))
		  mUndetermined = PR_TRUE;
    else
		  mUndetermined = PR_FALSE;
}


//
// Paint
//
// Paint our background and border like normal frames, but before we draw the
// children, draw our grippies for each toolbar.
//
NS_IMETHODIMP
nsProgressMeterFrame :: Paint ( nsIPresContext* aPresContext,
                            nsIRenderingContext& aRenderingContext,
                            const nsRect& aDirtyRect,
                            nsFramePaintLayer aWhichLayer)
{
  const nsStyleDisplay* disp = (const nsStyleDisplay*)
  mStyleContext->GetStyleData(eStyleStruct_Display);

  // if we aren't visible then we are done.
  if (!disp->mVisible) 
	   return NS_OK;  

  // if we are visible then tell our superclass to paint
  nsLeafFrame::Paint(aPresContext, aRenderingContext, aDirtyRect,
                       aWhichLayer);
  
  if (aWhichLayer == NS_FRAME_PAINT_LAYER_FOREGROUND)
  {
    // get our border
    const nsStyleSpacing* spacing =
      (const nsStyleSpacing*)mStyleContext->GetStyleData(eStyleStruct_Spacing);
    nsMargin border(0,0,0,0);
    spacing->CalcBorderFor(this, border);

    const nsStyleColor* colorStyle =
      (const nsStyleColor*)mStyleContext->GetStyleData(eStyleStruct_Color);

    nscolor color = colorStyle->mColor;
      
    // figure our twips convertion ratio
      //  float p2t;
      //  aPresContext->GetScaledPixelsToTwips(p2t);
      //  nscoord onePixel = NSIntPixelsToTwips(1, p2t);

    // figure out our rectangle
    nsRect rect(0,0,mRect.width, mRect.height);

    // if its vertical then transform the coords to the X coordinate system
    // and do our calculations there.
    if (!mHorizontal)
        rect = TransformYtoX(rect);

    //CalcSize(aPresContext,rect.width,rect.height);
    rect.x = border.left;
    rect.y = border.top;
    rect.width -= border.left*2;
    rect.height -= border.top*2;

    // paint the current progress in blue
    PaintBar(aPresContext, aRenderingContext, rect, mProgress, color);
	}
 
  return NS_OK;  
} // Paint

void
nsProgressMeterFrame :: PaintBar ( nsIPresContext* aPresContext,
                            nsIRenderingContext& aRenderingContext,
                            const nsRect& rect,
							float progress,
							nscolor color) {

	// if the bar is undetermined then use the whole progress area.
	// if the bar is determined then figure out the current progress and make
	// the bar only that percent of the full progress meter.

	nsRect bar(rect);

  if (!mUndetermined) 
	{
	  nscoord p = (nscoord)(bar.width*progress);
	  bar.width = p;
	}

	// fill the bar first then we will do the shading over it.
  aRenderingContext.SetColor(color);

	if (mHorizontal)
		aRenderingContext.FillRect(bar);
	else { // if we are vert then transfrom to the y cood system.
		nsRect nbar = TransformXtoY(bar);
		aRenderingContext.FillRect(nbar);
	}

	// draw the stripped barber shop if undetermined.
  if (mUndetermined) 
    PaintBarStripped(aPresContext,aRenderingContext,bar, color);
  else 
	  PaintBarSolid(aPresContext,aRenderingContext,bar, color, 0);	

}

nsRect 
nsProgressMeterFrame::TransformXtoY(const nsRect& rect)
{
   return nsRect(rect.y, mRect.height - (rect.x + rect.width), rect.height, rect.width);
}

nsRect 
nsProgressMeterFrame::TransformYtoX(const nsRect& rect)
{
   return nsRect(mRect.width - (rect.y + rect.height), rect.x, rect.height, rect.width);
}

nscolor 
nsProgressMeterFrame::BrightenBy(nscolor c, PRUint8 amount)
{
	PRUint8 r = NS_GET_R(c);
	PRUint8 g = NS_GET_G(c);
	PRUint8 b = NS_GET_B(c);

	return NS_RGB(r+amount, g+amount, b+amount);
}

PRUint8 
nsProgressMeterFrame::GetBrightness(nscolor c)
{
     
	// get the biggest rgb component;
  PRUint8 r = NS_GET_R(c);
  PRUint8 g = NS_GET_G(c);
  PRUint8 b = NS_GET_B(c);

	PRUint8 biggest = r;

	if (r > g && r > b)
		biggest = r;
	else if (g > r && g > b)
		biggest = g;
	else if (b > r && b > g)
		biggest = b;

  return biggest;
}

void 
nsProgressMeterFrame::PaintBarSolid(nsIPresContext* aPresContext, nsIRenderingContext& aRenderingContext, 
                                    const nsRect& rect, nscolor color, float skew)
{

  // figure out a pixel size
	float p2t;
	aPresContext->GetScaledPixelsToTwips(&p2t);
	nscoord onePixel = NSIntPixelsToTwips(1, p2t);

	// how many pixel lines will fit?
  int segments = 0;
  if(onePixel) {
    segments = (rect.height/2) / onePixel;
  } else {
    // Zero-height rect?  Bail, don't paint.
    return;
  }

	// get the skew in pixels;
	int skewedPixels = int(skew * onePixel);

	// we will draw from the top to center and from the bottom to center at the same time
	// so we need 2 rects one for the top and one for the bottom

	// top.

	nsRect tr(rect);
	tr.height= onePixel;

	// bottom
	nsRect br(rect);
	br.height = onePixel;
	br.y = rect.y + 2*segments*onePixel;
    br.x = rect.x + 2*segments*skewedPixels;

	// get the brightness of the color
	PRUint8 brightness = GetBrightness(color);

	// we need to figure out how bright we can get.
  PRUint8 units = 0;
  if(segments) {
    units = (255 - brightness)/segments;
  } else {
    // Divide-by-zero case, zero-height rect?
    units = 0;
  }

	// get a color we can set
	nscolor c(color);
  
  for (int i=0; i <= segments; i++)
  {
    // set the color and fill the top and bottom lines
    aRenderingContext.SetColor(c);
    
    if (mHorizontal) {
      aRenderingContext.FillRect(tr);
      aRenderingContext.FillRect(br);
    } else {
      aRenderingContext.FillRect(TransformXtoY(tr));
      aRenderingContext.FillRect(TransformXtoY(br));
    }
    // brighten the color
    c = BrightenBy(c, units);
    
    // move one line down
    tr.x += skewedPixels;
    tr.y += onePixel;
    
    // move one line up
    br.y -= onePixel;
    br.x -= skewedPixels;
  }
}


void 
nsProgressMeterFrame::PaintBarStripped(nsIPresContext* aPresContext, nsIRenderingContext& aRenderingContext, 
                                       const nsRect& r, nscolor color)
{
	// get stripe color from the style system
	nsCOMPtr<nsIStyleContext> style (mBarStyle) ;

	nscolor altColor = NS_RGB(128,128,128);

	// if we got a style then get the color from it
	if (style != 0)
	{
		const nsStyleColor*   barColor   = (const nsStyleColor*)style->GetStyleData(eStyleStruct_Color);
		altColor = barColor->mColor;
	}

  float skew = STRIPE_SKEW;
  float stripeWidth = float(STRIPE_WIDTH);

  nsRect rect(r);

  PRBool clipState;

  // Clip so we don't render outside the inner rect
  aRenderingContext.PushState();
  if (mHorizontal) 
	aRenderingContext.SetClipRect(rect, nsClipCombine_kIntersect, clipState);
  else
	aRenderingContext.SetClipRect(TransformXtoY(rect), nsClipCombine_kIntersect, clipState);
 
 
  float p2t;
  aPresContext->GetScaledPixelsToTwips(&p2t);
  // nscoord onePixel = NSIntPixelsToTwips(1, p2t);

  int stripeWidthInTwips = (int)(stripeWidth * p2t);
 
  int offset = int(float(r.height) * skew);

  //make things a little bigger and just clip them
    rect.width += offset*2;
  rect.x -= (offset + int(float(mStripeOffset)*p2t));
 
  int stripes = rect.width / (stripeWidthInTwips/2) + 2;

  nsRect sr(rect.x,rect.y,stripeWidthInTwips,rect.height);

  PRBool onoff = PR_FALSE;
  nscolor c;


  for (int i=0; i < stripes; i++)
  {
	 if (onoff)
		 c = color;
	 else
		 c = altColor;

	 PaintBarSolid(aPresContext,aRenderingContext,sr, c, skew);
	 sr.x += (stripeWidthInTwips/2);

	 onoff = !onoff;
  }

  aRenderingContext.PopState(clipState);
}

void
nsProgressMeterFrame::animate()
{
    mStripeOffset -= ANIMATION_INCREMENT;
  // 	printf("animate=%d\n", mStripeOffset);
	if (mStripeOffset < 0)
		mStripeOffset = STRIPE_WIDTH;
}


//
// Reflow
//
// Handle moving children around.
//
NS_IMETHODIMP
nsProgressMeterFrame :: Reflow ( nsIPresContext*          aPresContext,
                            nsHTMLReflowMetrics&     aDesiredSize,
                            const nsHTMLReflowState& aReflowState,
                            nsReflowStatus&          aStatus)
{	

  if (eReflowReason_Incremental == aReflowState.reason) {
    nsIFrame* targetFrame;
  
    // See if it's targeted at us
    aReflowState.reflowCommand->GetTarget(targetFrame);
    if (this == targetFrame) {
      Invalidate(aPresContext, nsRect(0,0,mRect.width,mRect.height), PR_FALSE);
    }
  }

  if (mUndetermined)
    gStripeAnimator->AddFrame(aPresContext, this);
  else 
	  gStripeAnimator->RemoveFrame(this);

  return nsLeafFrame::Reflow ( aPresContext, aDesiredSize, aReflowState, aStatus );

} // Reflow 

void
nsProgressMeterFrame::GetDesiredSize(nsIPresContext* aPresContext,
                             const nsHTMLReflowState& aReflowState,
                             nsHTMLReflowMetrics& aDesiredSize)
{

  CalcSize(aPresContext,aDesiredSize.width,aDesiredSize.height);

   // if the width is set use it
	if (NS_INTRINSICSIZE != aReflowState.mComputedWidth) 
	  aDesiredSize.width = aReflowState.mComputedWidth;

	// if the height is set use it
 	if (NS_INTRINSICSIZE != aReflowState.mComputedHeight) 
		aDesiredSize.height = aReflowState.mComputedHeight;
}


void
nsProgressMeterFrame::CalcSize(nsIPresContext* aPresContext, int& width, int& height)
{
	// make sure we convert to twips.
	float p2t;
  aPresContext->GetScaledPixelsToTwips(&p2t);

	if (mHorizontal) {
		width = (int)(100 * p2t);
		height = (int)(16 * p2t);
	} else {
		height = (int)(100 * p2t);
		width = (int)(16 * p2t);
	}
}

NS_IMETHODIMP
nsProgressMeterFrame::AttributeChanged(nsIPresContext* aPresContext,
                               nsIContent* aChild,
                               PRInt32 aNameSpaceID,
                               nsIAtom* aAttribute,
                               PRInt32 aHint)
{
  nsresult rv = nsLeafFrame::AttributeChanged(aPresContext, aChild,
                                              aNameSpaceID, aAttribute, aHint);
  if (NS_OK != rv) {
    return rv;
  }

  // did the progress change?
  if (nsHTMLAtoms::value == aAttribute) {
    nsAutoString newValue;

	// get attribute and set it
    aChild->GetAttribute(kNameSpaceID_None, nsHTMLAtoms::value, newValue);
    setProgress(newValue);

    Redraw(aPresContext);

  } else if (nsXULAtoms::mode == aAttribute) {
    nsAutoString newValue;

    aChild->GetAttribute(kNameSpaceID_None, nsXULAtoms::mode, newValue);
    setMode(newValue);

    // needs to reflow so we start the timer.
    if (aHint != NS_STYLE_HINT_REFLOW)
      Reflow(aPresContext);

  } else if (nsHTMLAtoms::align == aAttribute) {
    nsAutoString newValue;

	// get attribute and set it
    aChild->GetAttribute(kNameSpaceID_None, nsHTMLAtoms::align, newValue);
    setAlignment(newValue);

    if (aHint != NS_STYLE_HINT_REFLOW)
      Reflow(aPresContext);
  }

  return NS_OK;
}

void
nsProgressMeterFrame::Reflow(nsIPresContext* aPresContext)
{
   // reflow
    nsCOMPtr<nsIPresShell> shell;
    aPresContext->GetShell(getter_AddRefs(shell));
    
    nsCOMPtr<nsIReflowCommand> reflowCmd;
    nsresult rv = NS_NewHTMLReflowCommand(getter_AddRefs(reflowCmd), this,
                                          nsIReflowCommand::StyleChanged);
    if (NS_SUCCEEDED(rv)) 
      shell->AppendReflowCommand(reflowCmd);
}

void
nsProgressMeterFrame::Redraw(nsIPresContext* aPresContext)
{
   	nsRect frameRect;
	  GetRect(frameRect);
	  nsRect rect(0, 0, frameRect.width, frameRect.height);
    Invalidate(aPresContext, rect, PR_TRUE);
}


NS_IMETHODIMP
nsProgressMeterFrame::GetAdditionalStyleContext(PRInt32 aIndex, 
                                                nsIStyleContext** aStyleContext) const
{
  NS_PRECONDITION(nsnull != aStyleContext, "null OUT parameter pointer");
  if (aIndex < 0) {
    return NS_ERROR_INVALID_ARG;
  }
  *aStyleContext = nsnull;
  switch (aIndex) {
  case NS_PROGRESS_METER_STRIPE_CONTEXT_INDEX:
    *aStyleContext = mBarStyle;
    NS_IF_ADDREF(*aStyleContext);
    break;
  default:
    return NS_ERROR_INVALID_ARG;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsProgressMeterFrame::SetAdditionalStyleContext(PRInt32 aIndex, 
                                                nsIStyleContext* aStyleContext)
{
  if (aIndex < 0) {
    return NS_ERROR_INVALID_ARG;
  }
  switch (aIndex) {
  case NS_PROGRESS_METER_STRIPE_CONTEXT_INDEX:
    mBarStyle = aStyleContext;
    break;
  }
  return NS_OK;
}






