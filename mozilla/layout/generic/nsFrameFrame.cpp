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
#include "nsCOMPtr.h"
#include "nsLeafFrame.h"
#include "nsHTMLContainerFrame.h"
#include "nsIHTMLContent.h"
#include "nsIWebShell.h"
#include "nsIPresContext.h"
#include "nsIPresShell.h"
#include "nsHTMLIIDs.h"
#include "nsIComponentManager.h"
#include "nsIStreamListener.h"
#include "nsIURL.h"
#ifdef NECKO
#include "nsIIOService.h"
#include "nsIURL.h"
#include "nsIServiceManager.h"
static NS_DEFINE_CID(kIOServiceCID, NS_IOSERVICE_CID);
#endif // NECKO
#include "nsIDocument.h"
#include "nsIView.h"
#include "nsIViewManager.h"
#include "nsWidgetsCID.h"
#include "nsViewsCID.h"
#include "nsHTMLAtoms.h"
#include "nsIScrollableView.h"
#include "nsStyleCoord.h"
#include "nsIStyleContext.h"
#include "nsStyleConsts.h"
#include "nsIDocumentLoader.h"
#include "nsIPref.h"
#include "nsFrameSetFrame.h"
#include "nsIDOMHTMLFrameElement.h"
#include "nsIDOMHTMLIFrameElement.h"
#include "nsGenericHTMLElement.h"
#include "nsLayoutAtoms.h"

class nsHTMLFrame;

static NS_DEFINE_IID(kIWebShellContainerIID, NS_IWEB_SHELL_CONTAINER_IID);
static NS_DEFINE_IID(kIStreamObserverIID, NS_ISTREAMOBSERVER_IID);
static NS_DEFINE_IID(kIWebShellIID, NS_IWEB_SHELL_IID);
static NS_DEFINE_IID(kWebShellCID, NS_WEB_SHELL_CID);
static NS_DEFINE_IID(kIViewIID, NS_IVIEW_IID);
static NS_DEFINE_IID(kCViewCID, NS_VIEW_CID);
static NS_DEFINE_IID(kCChildCID, NS_CHILD_CID);
static NS_DEFINE_IID(kIDOMHTMLFrameElementIID, NS_IDOMHTMLFRAMEELEMENT_IID);
static NS_DEFINE_IID(kIDOMHTMLIFrameElementIID, NS_IDOMHTMLIFRAMEELEMENT_IID);

/*******************************************************************************
 * TempObserver XXX temporary until doc manager/loader is in place
 ******************************************************************************/
class TempObserver : public nsIStreamObserver
{
public:
  TempObserver() { NS_INIT_REFCNT(); }

  virtual ~TempObserver() {}

  // nsISupports
  NS_DECL_ISUPPORTS

#ifdef NECKO
  // nsIStreamObserver methods:
  NS_IMETHOD OnStartRequest(nsISupports *ctxt);
  NS_IMETHOD OnStopRequest(nsISupports *ctxt, nsresult status, const PRUnichar *errorMsg);
#else
  // nsIStreamObserver
  NS_IMETHOD OnStartRequest(nsIURI* aURL, const char *aContentType);
  NS_IMETHOD OnProgress(nsIURI* aURL, PRUint32 aProgress, PRUint32 aProgressMax);
  NS_IMETHOD OnStatus(nsIURI* aURL, const PRUnichar* aMsg);
  NS_IMETHOD OnStopRequest(nsIURI* aURL, nsresult status, const PRUnichar* aMsg);
#endif

protected:

  nsString mURL;
  nsString mOverURL;
  nsString mOverTarget;
};

/*******************************************************************************
 * FrameLoadingInfo 
 ******************************************************************************/
class FrameLoadingInfo : public nsISupports
{
public:
  FrameLoadingInfo(const nsSize& aSize);

  // nsISupports interface...
  NS_DECL_ISUPPORTS

protected:
  virtual ~FrameLoadingInfo() {}

public:
  nsSize mFrameSize;
};


/*******************************************************************************
 * nsHTMLFrameOuterFrame
 ******************************************************************************/
#define nsHTMLFrameOuterFrameSuper nsHTMLContainerFrame

class nsHTMLFrameOuterFrame : public nsHTMLFrameOuterFrameSuper {

public:
  nsHTMLFrameOuterFrame();

  NS_IMETHOD GetFrameName(nsString& aResult) const;

  NS_IMETHOD GetFrameType(nsIAtom** aType) const;

  NS_IMETHOD Paint(nsIPresContext& aPresContext,
                   nsIRenderingContext& aRenderingContext,
                   const nsRect& aDirtyRect,
                   nsFramePaintLayer aWhichLayer);

  NS_IMETHOD Init(nsIPresContext&  aPresContext,
                  nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIStyleContext* aContext,
                  nsIFrame*        aPrevInFlow);

  NS_IMETHOD Reflow(nsIPresContext&          aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);

  NS_IMETHOD AttributeChanged(nsIPresContext* aPresContext,
                              nsIContent* aChild,
                              nsIAtom* aAttribute,
                              PRInt32 aHint);
  NS_IMETHOD  VerifyTree() const;
  PRBool HasBorder();
  PRBool IsInline();

protected:
  virtual ~nsHTMLFrameOuterFrame();
  virtual void GetDesiredSize(nsIPresContext* aPresContext,
                              const nsHTMLReflowState& aReflowState,
                              nsHTMLReflowMetrics& aDesiredSize);
  virtual PRIntn GetSkipSides() const;
  PRBool mIsInline;
};

/*******************************************************************************
 * nsHTMLFrameInnerFrame
 ******************************************************************************/
class nsHTMLFrameInnerFrame : public nsLeafFrame {

public:

  nsHTMLFrameInnerFrame();

  NS_IMETHOD GetFrameName(nsString& aResult) const;

  NS_IMETHOD GetFrameType(nsIAtom** aType) const;

  /**
    * @see nsIFrame::Paint
    */
  NS_IMETHOD Paint(nsIPresContext& aPresContext,
                   nsIRenderingContext& aRenderingContext,
                   const nsRect& aDirtyRect,
                   nsFramePaintLayer aWhichLayer);

  /**
    * @see nsIFrame::Reflow
    */
  NS_IMETHOD Reflow(nsIPresContext&          aCX,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);

  NS_IMETHOD DidReflow(nsIPresContext& aPresContext,
                       nsDidReflowStatus aStatus);

  NS_IMETHOD MoveTo(nscoord aX, nscoord aY);
  NS_IMETHOD SizeTo(nscoord aWidth, nscoord aHeight);

  NS_IMETHOD GetParentContent(nsIContent*& aContent);
  PRBool GetURL(nsIContent* aContent, nsString& aResult);
  PRBool GetName(nsIContent* aContent, nsString& aResult);
  PRInt32 GetScrolling(nsIContent* aContent, PRBool aStandardMode);
  nsFrameborder GetFrameBorder(PRBool aStandardMode);
  PRInt32 GetMarginWidth(nsIPresContext* aPresContext, nsIContent* aContent);
  PRInt32 GetMarginHeight(nsIPresContext* aPresContext, nsIContent* aContent);

  nsresult ReloadURL();

protected:
  nsresult CreateWebShell(nsIPresContext& aPresContext, const nsSize& aSize);

  virtual ~nsHTMLFrameInnerFrame();

  virtual void GetDesiredSize(nsIPresContext* aPresContext,
                              const nsHTMLReflowState& aReflowState,
                              nsHTMLReflowMetrics& aDesiredSize);

  nsIWebShell* mWebShell;
  PRBool mCreatingViewer;

  // XXX fix these
  TempObserver* mTempObserver;
};


/*******************************************************************************
 * nsHTMLFrameOuterFrame
 ******************************************************************************/
nsHTMLFrameOuterFrame::nsHTMLFrameOuterFrame()
  : nsHTMLContainerFrame()
{
  mIsInline = PR_FALSE;
}

nsHTMLFrameOuterFrame::~nsHTMLFrameOuterFrame()
{
  //printf("nsHTMLFrameOuterFrame destructor %X \n", this);
}

NS_IMETHODIMP
nsHTMLFrameOuterFrame::Init(nsIPresContext&  aPresContext,
                            nsIContent*      aContent,
                            nsIFrame*        aParent,
                            nsIStyleContext* aContext,
                            nsIFrame*        aPrevInFlow)
{
  // determine if we are a <frame> or <iframe>
  nsIDOMHTMLFrameElement* frameElem = nsnull;
  if (aContent) {
    aContent->QueryInterface(kIDOMHTMLIFrameElementIID, (void**) &frameElem);
    mIsInline = (frameElem) ? PR_TRUE : PR_FALSE;
    NS_IF_RELEASE(frameElem);
  }
  return nsHTMLFrameOuterFrameSuper::Init(aPresContext, aContent, aParent,
                                          aContext, aPrevInFlow);
}


PRBool
nsHTMLFrameOuterFrame::HasBorder()
{
  if (IsInline()) {
    nsIFrame* firstChild = mFrames.FirstChild();
    if (nsnull != firstChild) {
      if (eFrameborder_No != ((nsHTMLFrameInnerFrame*)firstChild)->GetFrameBorder(eCompatibility_Standard)) {
        return PR_TRUE;
      }
    }
  }
  return PR_FALSE;
}


PRIntn
nsHTMLFrameOuterFrame::GetSkipSides() const
{
  return 0;
}

void 
nsHTMLFrameOuterFrame::GetDesiredSize(nsIPresContext* aPresContext,
                                      const nsHTMLReflowState& aReflowState,
                                      nsHTMLReflowMetrics& aDesiredSize)
{
  // <frame> processing does not use this routine, only <iframe>
  float p2t;
  aPresContext->GetScaledPixelsToTwips(&p2t);

  // XXX this needs to be changed from (200,200) to a better default
  // for inline frames
  if (NS_UNCONSTRAINEDSIZE != aReflowState.computedWidth) {
    aDesiredSize.width = aReflowState.computedWidth;
  }
  else {
    aDesiredSize.width = NSIntPixelsToTwips(200, p2t);
  }
  if (NS_UNCONSTRAINEDSIZE != aReflowState.computedHeight) {
    aDesiredSize.height = aReflowState.computedHeight;
  }
  else {
    aDesiredSize.height = NSIntPixelsToTwips(200, p2t);
  }
  aDesiredSize.ascent = aDesiredSize.height;
  aDesiredSize.descent = 0;
}

PRBool nsHTMLFrameOuterFrame::IsInline()
{ 
  return mIsInline;
}

NS_IMETHODIMP
nsHTMLFrameOuterFrame::Paint(nsIPresContext& aPresContext,
                             nsIRenderingContext& aRenderingContext,
                             const nsRect& aDirtyRect,
                             nsFramePaintLayer aWhichLayer)
{
  //printf("outer paint %X (%d,%d,%d,%d) \n", this, aDirtyRect.x, aDirtyRect.y, aDirtyRect.width, aDirtyRect.height);
  nsIFrame* firstChild = mFrames.FirstChild();
  if (nsnull != firstChild) {
    firstChild->Paint(aPresContext, aRenderingContext, aDirtyRect,
                      aWhichLayer);
  }
  if (IsInline()) {
    return nsHTMLContainerFrame::Paint(aPresContext, aRenderingContext,
                                       aDirtyRect, aWhichLayer);
  } else {
    return NS_OK;
  }
}

NS_IMETHODIMP nsHTMLFrameOuterFrame::GetFrameName(nsString& aResult) const
{
  return MakeFrameName("FrameOuter", aResult);
}

NS_IMETHODIMP
nsHTMLFrameOuterFrame::GetFrameType(nsIAtom** aType) const
{
  NS_PRECONDITION(nsnull != aType, "null OUT parameter pointer");
  *aType = nsLayoutAtoms::htmlFrameOuterFrame; 
  NS_ADDREF(*aType);
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLFrameOuterFrame::Reflow(nsIPresContext&          aPresContext,
                              nsHTMLReflowMetrics&     aDesiredSize,
                              const nsHTMLReflowState& aReflowState,
                              nsReflowStatus&          aStatus)
{
  //printf("OuterFrame::Reflow %X (%d,%d) \n", this, aReflowState.availableWidth, aReflowState.availableHeight); 
  NS_FRAME_TRACE(NS_FRAME_TRACE_CALLS,
     ("enter nsHTMLFrameOuterFrame::Reflow: maxSize=%d,%d reason=%d",
      aReflowState.availableWidth, aReflowState.availableHeight, aReflowState.reason));

  if (IsInline()) {
    GetDesiredSize(&aPresContext, aReflowState, aDesiredSize);
  } else {
    aDesiredSize.width  = aReflowState.availableWidth;
    aDesiredSize.height = aReflowState.availableHeight;
  }

  nsIFrame* firstChild = mFrames.FirstChild();
  if (nsnull == firstChild) {
    firstChild = new nsHTMLFrameInnerFrame;
    mFrames.SetFrames(firstChild);
    // XXX temporary! use style system to get correct style!
    firstChild->Init(aPresContext, mContent, this, mStyleContext, nsnull);
  }
 

  nsSize innerSize(aDesiredSize.width, aDesiredSize.height);
  nsPoint offset(0,0);
  if (IsInline() && HasBorder()) {
    const nsStyleSpacing* spacing =
      (const nsStyleSpacing*)mStyleContext->GetStyleData(eStyleStruct_Spacing);
    nsMargin border;
    spacing->CalcBorderFor(this, border);
    offset.x = border.left;
    offset.y = border.right;
    // XXX Don't subtract the border!!! The size we are given does not include our
    // border! -EDV
    //innerSize.width  -= border.left + border.right;
    //innerSize.height -= border.top  + border.bottom;

    // we now need to add our border in. -EDV
    aDesiredSize.width += border.left + border.right;
    aDesiredSize.height += border.top + border.bottom;
  }

  // Reflow the child and get its desired size
  nsHTMLReflowMetrics kidMetrics(aDesiredSize.maxElementSize);
  nsHTMLReflowState   kidReflowState(aPresContext, aReflowState, firstChild,
                                     innerSize);
  nsIHTMLReflow*      htmlReflow;

  if (NS_OK == firstChild->QueryInterface(kIHTMLReflowIID, (void**)&htmlReflow)) {
    ReflowChild(firstChild, aPresContext, kidMetrics, kidReflowState, aStatus);
    NS_ASSERTION(NS_FRAME_IS_COMPLETE(aStatus), "bad status");
  
    // Place and size the child
    nsRect rect(offset.x, offset.y, innerSize.width, innerSize.height);
    firstChild->SetRect(rect);
  }

  // XXX what should the max-element-size of an iframe be? Shouldn't
  // iframe's normally shrink wrap around their content when they
  // don't have a specified width/height?
  if (nsnull != aDesiredSize.maxElementSize) {
    aDesiredSize.maxElementSize->width = aDesiredSize.width;
    aDesiredSize.maxElementSize->height = aDesiredSize.height;
  }

  NS_FRAME_TRACE(NS_FRAME_TRACE_CALLS,
     ("exit nsHTMLFrameOuterFrame::Reflow: size=%d,%d status=%x",
      aDesiredSize.width, aDesiredSize.height, aStatus));

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLFrameOuterFrame::VerifyTree() const
{
  // XXX Completely disabled for now; once pseud-frames are reworked
  // then we can turn it back on.
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLFrameOuterFrame::AttributeChanged(nsIPresContext* aPresContext,
                                        nsIContent* aChild,
                                        nsIAtom* aAttribute,
                                        PRInt32 aHint)
{
  if (nsHTMLAtoms::src == aAttribute) {
    printf("got a request\n");
    nsIFrame* firstChild = mFrames.FirstChild();
    if (nsnull != firstChild) {
      ((nsHTMLFrameInnerFrame*)firstChild)->ReloadURL();
    }
  }
  return NS_OK;
}

nsresult
NS_NewHTMLFrameOuterFrame(nsIFrame** aNewFrame)
{
  NS_PRECONDITION(aNewFrame, "null OUT ptr");
  if (nsnull == aNewFrame) {
    return NS_ERROR_NULL_POINTER;
  }
  nsHTMLFrameOuterFrame* it = new nsHTMLFrameOuterFrame;
  if (!it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  *aNewFrame = it;
  return NS_OK;
}

/*******************************************************************************
 * nsHTMLFrameInnerFrame
 ******************************************************************************/
nsHTMLFrameInnerFrame::nsHTMLFrameInnerFrame()
  : nsLeafFrame()
{
  mWebShell       = nsnull;
  mCreatingViewer = PR_FALSE;
  mTempObserver   = new TempObserver();
  NS_ADDREF(mTempObserver);
}

nsHTMLFrameInnerFrame::~nsHTMLFrameInnerFrame()
{
  //printf("nsHTMLFrameInnerFrame destructor %X \n", this);
  if (nsnull != mWebShell) {
    mWebShell->Destroy();    
    NS_RELEASE(mWebShell);
  }
  NS_RELEASE(mTempObserver);
}

PRBool nsHTMLFrameInnerFrame::GetURL(nsIContent* aContent, nsString& aResult)
{
  PRBool result = PR_FALSE;
  nsIHTMLContent* content = nsnull;
  aContent->QueryInterface(kIHTMLContentIID, (void**) &content);
  if (nsnull != content) {
    nsHTMLValue value;
    if (NS_CONTENT_ATTR_HAS_VALUE == (content->GetHTMLAttribute(nsHTMLAtoms::src, value))) {
      if (eHTMLUnit_String == value.GetUnit()) {
        value.GetStringValue(aResult);
        if (aResult.Length() > 0) {
          result = PR_TRUE;
        }
      }
    }
    NS_RELEASE(content);
  }
  if (PR_FALSE == result) {
    aResult.SetLength(0);
  }
  return result;
}

PRBool nsHTMLFrameInnerFrame::GetName(nsIContent* aContent, nsString& aResult)
{
  PRBool result = PR_FALSE;
  nsIHTMLContent* content = nsnull;
  aContent->QueryInterface(kIHTMLContentIID, (void**) &content);
  if (nsnull != content) {
    nsHTMLValue value;
    if (NS_CONTENT_ATTR_HAS_VALUE == (content->GetHTMLAttribute(nsHTMLAtoms::name, value))) {
      if (eHTMLUnit_String == value.GetUnit()) {
        value.GetStringValue(aResult);
        result = PR_TRUE;
      }
    } 
    NS_RELEASE(content);
  }
  if (PR_FALSE == result) {
    aResult.SetLength(0);
  }
  return result;
}

PRInt32 nsHTMLFrameInnerFrame::GetScrolling(nsIContent* aContent, PRBool aStandardMode)
{
  nsIHTMLContent* content = nsnull;
  aContent->QueryInterface(kIHTMLContentIID, (void**) &content);
  if (nsnull != content) {
    nsHTMLValue value;
    if (NS_CONTENT_ATTR_HAS_VALUE == (content->GetHTMLAttribute(nsHTMLAtoms::scrolling, value))) {
      if (eHTMLUnit_Enumerated == value.GetUnit()) {
        PRInt32 returnValue;
        PRInt32 intValue;
        intValue = value.GetIntValue();
        if (!aStandardMode) {
          if ((NS_STYLE_FRAME_ON == intValue) || (NS_STYLE_FRAME_SCROLL == intValue)) {
            intValue = NS_STYLE_FRAME_YES;
          } else if ((NS_STYLE_FRAME_OFF == intValue) || (NS_STYLE_FRAME_NOSCROLL == intValue)) {
            intValue = NS_STYLE_FRAME_NO;
          }
        }
        if (NS_STYLE_FRAME_YES == intValue) {
          returnValue = NS_STYLE_OVERFLOW_SCROLL;
        } else if (NS_STYLE_FRAME_NO == intValue) {
          returnValue = NS_STYLE_OVERFLOW_HIDDEN;
        } else if (NS_STYLE_FRAME_AUTO == intValue) {
          returnValue = NS_STYLE_OVERFLOW_AUTO;
        }
        NS_RELEASE(content);
        return returnValue;
      }      
    }
    NS_RELEASE(content);
  }
  return -1;
}

nsFrameborder nsHTMLFrameInnerFrame::GetFrameBorder(PRBool aStandardMode)
{
  nsIHTMLContent* content = nsnull;
  mContent->QueryInterface(kIHTMLContentIID, (void**) &content);
  if (nsnull != content) {
    nsHTMLValue value;
    if (NS_CONTENT_ATTR_HAS_VALUE == (content->GetHTMLAttribute(nsHTMLAtoms::frameborder, value))) {
      if (eHTMLUnit_Enumerated == value.GetUnit()) {
        PRInt32 intValue;
        intValue = value.GetIntValue();
        if (!aStandardMode) {
          if (NS_STYLE_FRAME_YES == intValue) {
            intValue = NS_STYLE_FRAME_0;
          } 
          else if (NS_STYLE_FRAME_NO == intValue) {
            intValue = NS_STYLE_FRAME_1;
          }
        }
        if (NS_STYLE_FRAME_0 == intValue) {
          NS_RELEASE(content);
          return eFrameborder_No;
        } 
        else if (NS_STYLE_FRAME_1 == intValue) {
          NS_RELEASE(content);
          return eFrameborder_Yes;
        }
      }      
    }
    NS_RELEASE(content);
  }
  // XXX if we get here, check for nsIDOMFRAMESETElement interface
  return eFrameborder_Notset;
}


PRInt32 nsHTMLFrameInnerFrame::GetMarginWidth(nsIPresContext* aPresContext, nsIContent* aContent)
{
  PRInt32 marginWidth = -1;
  nsIHTMLContent* content = nsnull;
  if (NS_SUCCEEDED(mContent->QueryInterface(kIHTMLContentIID, (void**) &content))) {
    float p2t;
    aPresContext->GetScaledPixelsToTwips(&p2t);
    nsHTMLValue value;
    content->GetHTMLAttribute(nsHTMLAtoms::marginwidth, value);
    if (eHTMLUnit_Pixel == value.GetUnit()) { 
      marginWidth = NSIntPixelsToTwips(value.GetPixelValue(), p2t);
      if (marginWidth < 0) {
        marginWidth = 0;
      }
    }
    NS_RELEASE(content);
  }
  return marginWidth;
}

PRInt32 nsHTMLFrameInnerFrame::GetMarginHeight(nsIPresContext* aPresContext, nsIContent* aContent)
{
  PRInt32 marginHeight = -1;
  nsIHTMLContent* content = nsnull;
  if (NS_SUCCEEDED(mContent->QueryInterface(kIHTMLContentIID, (void**) &content))) {
    float p2t;
    aPresContext->GetScaledPixelsToTwips(&p2t);
    nsHTMLValue value;
    content->GetHTMLAttribute(nsHTMLAtoms::marginheight, value);
    if (eHTMLUnit_Pixel == value.GetUnit()) { 
      marginHeight = NSIntPixelsToTwips(value.GetPixelValue(), p2t);
      if (marginHeight < 0) {
        marginHeight = 0;
      }
    }
    NS_RELEASE(content);
  }
  return marginHeight;
}

NS_IMETHODIMP nsHTMLFrameInnerFrame::GetFrameName(nsString& aResult) const
{
  return MakeFrameName("FrameInner", aResult);
}

NS_IMETHODIMP
nsHTMLFrameInnerFrame::GetFrameType(nsIAtom** aType) const
{
  NS_PRECONDITION(nsnull != aType, "null OUT parameter pointer");
  *aType = nsLayoutAtoms::htmlFrameInnerFrame; 
  NS_ADDREF(*aType);
  return NS_OK;
}

NS_METHOD
nsHTMLFrameInnerFrame::MoveTo(nscoord aX, nscoord aY)
{
  return nsLeafFrame::MoveTo(aX, aY);
}

NS_METHOD
nsHTMLFrameInnerFrame::SizeTo(nscoord aWidth, nscoord aHeight)
{
  return nsLeafFrame::SizeTo(aWidth, aHeight);
}

NS_IMETHODIMP
nsHTMLFrameInnerFrame::Paint(nsIPresContext&      aPresContext,
                             nsIRenderingContext& aRenderingContext,
                             const nsRect&        aDirtyRect,
                             nsFramePaintLayer    aWhichLayer)
{
  //printf("inner paint %X (%d,%d,%d,%d) \n", this, aDirtyRect.x, aDirtyRect.y, aDirtyRect.width, aDirtyRect.height);
  // if there is not web shell paint based on our background color, 
  // otherwise let the web shell paint the sub document 
  if (!mWebShell) {
    const nsStyleColor* color =
      (const nsStyleColor*)mStyleContext->GetStyleData(eStyleStruct_Color);
    aRenderingContext.SetColor(color->mBackgroundColor);
    aRenderingContext.FillRect(mRect);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLFrameInnerFrame::GetParentContent(nsIContent*& aContent)
{
  nsHTMLFrameOuterFrame* parent;
  GetParent((nsIFrame**)&parent);

  nsIContent* content;
  nsresult    rv = parent->GetContent(&content);
  aContent = content;
  return rv;
}


static
void TempMakeAbsURL(nsIContent* aContent, nsString& aRelURL, nsString& aAbsURL)
{
  nsIURI* baseURL = nsnull;
  nsIHTMLContent* htmlContent;
  if (NS_SUCCEEDED(aContent->QueryInterface(kIHTMLContentIID, (void**)&htmlContent))) {
    htmlContent->GetBaseURL(baseURL);
    NS_RELEASE(htmlContent);
  }
  else {
    nsIDocument* doc;
    if (NS_SUCCEEDED(aContent->GetDocument(doc))) {
      doc->GetBaseURL(baseURL);
      NS_RELEASE(doc);
    }
  }

  nsString empty;
#ifndef NECKO
  NS_MakeAbsoluteURL(baseURL, empty, aRelURL, aAbsURL);
#else
  nsresult result;
  NS_WITH_SERVICE(nsIIOService, service, kIOServiceCID, &result);
  if (NS_FAILED(result)) return;

  nsIURI *baseUri = nsnull;
  result = baseURL->QueryInterface(nsIURI::GetIID(), (void**)&baseUri);
  if (NS_FAILED(result)) return;

  char *absUrlStr = nsnull;
  char *urlSpec = aRelURL.ToNewCString();
  if (!urlSpec) return NS_ERROR_OUT_OF_MEMORY;
  result = service->MakeAbsolute(urlSpec, baseUri, &absUrlStr);
  NS_RELEASE(baseUri);
  aAbsURL= absUrlStr;
  nsCRT::free(urlSpec);
  delete [] absUrlStr;
#endif // NECKO
  NS_IF_RELEASE(baseURL);
}

NS_IMETHODIMP
nsHTMLFrameInnerFrame::DidReflow(nsIPresContext& aPresContext,
                        nsDidReflowStatus aStatus)
{
  nsresult rv = nsLeafFrame::DidReflow(aPresContext, aStatus);


  // The view is created hidden; once we have reflowed it and it has been
  // positioned then we show it.
  if (NS_FRAME_REFLOW_FINISHED == aStatus) {
    nsIView* view = nsnull;
    GetView(&view);
    if (view) {
      const nsStyleDisplay* display;
      GetStyleData(eStyleStruct_Display, ((const nsStyleStruct *&)display));
      nsViewVisibility newVis = NS_STYLE_VISIBILITY_HIDDEN == display->mVisible ? nsViewVisibility_kHide : nsViewVisibility_kShow;
      nsViewVisibility oldVis;
      // only change if different.
      view->GetVisibility(oldVis);
      if (newVis != oldVis) 
        view->SetVisibility(newVis);
    }
  }
  
  return rv;
}

nsresult
nsHTMLFrameInnerFrame::CreateWebShell(nsIPresContext& aPresContext,
                                      const nsSize& aSize)
{
  nsresult rv;
  nsIContent* content;
  GetParentContent(content);

  rv = nsComponentManager::CreateInstance(kWebShellCID, nsnull, kIWebShellIID,
                                    (void**)&mWebShell);
  if (NS_OK != rv) {
    NS_ASSERTION(0, "could not create web widget");
    return rv;
  }
  
  // pass along marginwidth, marginheight, scrolling so sub document can use it
  mWebShell->SetMarginWidth(GetMarginWidth(&aPresContext, content));
  mWebShell->SetMarginHeight(GetMarginHeight(&aPresContext, content));
  nsCompatibility mode;
  aPresContext.GetCompatibilityMode(&mode);
  mWebShell->SetScrolling(GetScrolling(content, mode));
  mWebShell->SetIsFrame(PR_TRUE);
  
  nsString frameName;
  if (GetName(content, frameName)) {
    mWebShell->SetName(frameName.GetUnicode());
  }

  // If our container is a web-shell, inform it that it has a new
  // child. If it's not a web-shell then some things will not operate
  // properly.
  nsISupports* container;
  aPresContext.GetContainer(&container);
  if (nsnull != container) {
    nsIWebShell* outerShell = nsnull;
    container->QueryInterface(kIWebShellIID, (void**) &outerShell);
    if (nsnull != outerShell) {
      outerShell->AddChild(mWebShell);

      // connect the container...
      nsIWebShellContainer* outerContainer = nsnull;
      container->QueryInterface(kIWebShellContainerIID, (void**) &outerContainer);
      if (nsnull != outerContainer) {
        mWebShell->SetContainer(outerContainer);
        NS_RELEASE(outerContainer);
      }

#ifdef INCLUDE_XUL
      nsWebShellType parentType;
      outerShell->GetWebShellType(parentType);
      nsIAtom* typeAtom = NS_NewAtom("type");
      nsAutoString value, valuePiece;
      PRBool isContent;

      isContent = PR_FALSE;
      if (NS_SUCCEEDED(content->GetAttribute(kNameSpaceID_None, typeAtom, value))) {

        // we accept "content" and "content-xxx" values.
        // at time of writing, we expect "xxx" to be "primary", but
        // someday it might be an integer expressing priority
        value.Left(valuePiece, 7);
        if (valuePiece.EqualsIgnoreCase("content") &&
           (value.Length() == 7 ||
              value.Mid(valuePiece, 7, 1) == 1 && valuePiece.Equals("-")))
            isContent = PR_TRUE;
      }
      if (isContent) {
        // The web shell's type is content.
        mWebShell->SetWebShellType(nsWebShellContent);
        nsCOMPtr<nsIWebShellContainer> shellAsContainer;
        shellAsContainer = do_QueryInterface(mWebShell);
        shellAsContainer->ContentShellAdded(mWebShell, content);
      } else {
        // Inherit our type from our parent webshell.  If it is
        // chrome, we'll be chrome.  If it is content, we'll be
        // content.
        mWebShell->SetWebShellType(parentType);
      }

      // Make sure all shells have links back to the nearest enclosing chrome
      // shell.
      
      nsCOMPtr<nsIWebShell> chromeShell;
      nsWebShellType chromeShellType;
      outerShell->GetWebShellType(chromeShellType);
      if (chromeShellType == nsWebShellChrome)
        chromeShell = dont_QueryInterface(outerShell);
      else
        outerShell->GetContainingChromeShell(getter_AddRefs(chromeShell));

      mWebShell->SetContainingChromeShell(chromeShell);
      
#endif // INCLUDE_XUL 

      nsIPref*  outerPrefs = nsnull;  // connect the prefs
      outerShell->GetPrefs(outerPrefs);
      if (nsnull != outerPrefs) {
        mWebShell->SetPrefs(outerPrefs);
        NS_RELEASE(outerPrefs);
      } 
      NS_RELEASE(outerShell);
    }
    NS_RELEASE(container);
  }

  float t2p;
  aPresContext.GetTwipsToPixels(&t2p);
  nsCOMPtr<nsIPresShell> presShell;
  aPresContext.GetShell(getter_AddRefs(presShell));     

  // create, init, set the parent of the view
  nsIView* view;
  rv = nsComponentManager::CreateInstance(kCViewCID, nsnull, kIViewIID,
                                        (void **)&view);
  if (NS_OK != rv) {
    NS_ASSERTION(0, "Could not create view for nsHTMLFrame");
    return rv;
  }

  nsIView* parView;
  nsPoint origin;
  GetOffsetFromView(origin, &parView);  
  nsRect viewBounds(origin.x, origin.y, aSize.width, aSize.height);

  nsCOMPtr<nsIViewManager> viewMan;
  presShell->GetViewManager(getter_AddRefs(viewMan));  
  rv = view->Init(viewMan, viewBounds, parView);
  viewMan->InsertChild(parView, view, 0);
  rv = view->CreateWidget(kCChildCID);
  SetView(view);

  // if the visibility is hidden, reflect that in the view
  const nsStyleDisplay* display;
  GetStyleData(eStyleStruct_Display, ((const nsStyleStruct *&)display));
  if (NS_STYLE_VISIBILITY_HIDDEN == display->mVisible) {
    view->SetVisibility(nsViewVisibility_kHide);
  }

  nsIWidget* widget;
  view->GetWidget(widget);
  nsRect webBounds(0, 0, NSToCoordRound(aSize.width * t2p), 
                   NSToCoordRound(aSize.height * t2p));

  mWebShell->Init(widget->GetNativeData(NS_NATIVE_WIDGET), 
                  webBounds.x, webBounds.y,
                  webBounds.width, webBounds.height);
                  //GetScrolling(content, PR_FALSE));
  NS_RELEASE(content);
  NS_RELEASE(widget);

  mWebShell->SetObserver(mTempObserver);
  mWebShell->Show();

  return NS_OK;
}


NS_IMETHODIMP
nsHTMLFrameInnerFrame::Reflow(nsIPresContext&          aPresContext,
                              nsHTMLReflowMetrics&     aDesiredSize,
                              const nsHTMLReflowState& aReflowState,
                              nsReflowStatus&          aStatus)
{
  NS_FRAME_TRACE(NS_FRAME_TRACE_CALLS,
     ("enter nsHTMLFrameInnerFrame::Reflow: maxSize=%d,%d reason=%d",
      aReflowState.availableWidth,
      aReflowState.availableHeight,
      aReflowState.reason));

  nsresult rv = NS_OK;

  // use the max size set in aReflowState by the nsHTMLFrameOuterFrame as our size
  if (!mCreatingViewer) {
    nsIContent* content;
    GetParentContent(content);

    nsAutoString url;
    PRBool hasURL = GetURL(content, url);

    // if the size is not 0 and there is a src, create the web shell
    if ((aReflowState.availableWidth >= 0) && (aReflowState.availableHeight >= 0) && hasURL) {
      if (nsnull == mWebShell) {
        nsSize  maxSize(aReflowState.availableWidth, aReflowState.availableHeight);
        rv = CreateWebShell(aPresContext, maxSize);
#ifdef INCLUDE_XUL
        // The URL can be destructively altered when a content shell is made.
        // Refetch it to ensure we have the actual URL to load.
        hasURL = GetURL(content, url);
#endif // INCLUDE_XUL
      }

      if (nsnull != mWebShell) {
        mCreatingViewer=PR_TRUE;

        // load the document
        nsString absURL;
        TempMakeAbsURL(content, url, absURL);

        rv = mWebShell->LoadURL(absURL.GetUnicode());  // URL string with a default nsnull value for post Data
      }
    } else {
      mCreatingViewer = PR_TRUE;
    }
    NS_RELEASE(content);
  }

  aDesiredSize.width  = aReflowState.availableWidth;
  aDesiredSize.height = aReflowState.availableHeight;
  aDesiredSize.ascent = aDesiredSize.height;
  aDesiredSize.descent = 0;
  if (nsnull != aDesiredSize.maxElementSize) {
    aDesiredSize.maxElementSize->width = aDesiredSize.width;
    aDesiredSize.maxElementSize->height = aDesiredSize.height;
  }

  aStatus = NS_FRAME_COMPLETE;

  // resize the sub document
  if (mWebShell) {
    float t2p;
    aPresContext.GetTwipsToPixels(&t2p);
    nsRect subBounds;

    mWebShell->GetBounds(subBounds.x, subBounds.y,
                       subBounds.width, subBounds.height);
    subBounds.width  = NSToCoordRound(aDesiredSize.width * t2p);
    subBounds.height = NSToCoordRound(aDesiredSize.height * t2p);
    mWebShell->SetBounds(subBounds.x, subBounds.y,
                       subBounds.width, subBounds.height);
    mWebShell->Repaint(PR_TRUE); 

    NS_FRAME_TRACE(NS_FRAME_TRACE_CALLS,
      ("exit nsHTMLFrameInnerFrame::Reflow: size=%d,%d rv=%x",
      aDesiredSize.width, aDesiredSize.height, aStatus));
  }
    
  return rv;
}

nsresult
nsHTMLFrameInnerFrame::ReloadURL()
{
  nsresult rv = NS_OK;
  nsIContent* content;
  GetParentContent(content);
  if (nsnull != content) {

    nsAutoString url;
    GetURL(content, url);

    // load a new url if the size is not 0
    if ((mRect.width > 0) && (mRect.height > 0)) {
      if (nsnull != mWebShell) {
        mCreatingViewer=PR_TRUE;

        // load the document
        nsString absURL;
        TempMakeAbsURL(content, url, absURL);

        rv = mWebShell->LoadURL(absURL.GetUnicode());  // URL string with a default nsnull value for post Data
      }
    } else {
      mCreatingViewer = PR_TRUE;
    }
    NS_RELEASE(content);
  }
  return rv;
}

void 
nsHTMLFrameInnerFrame::GetDesiredSize(nsIPresContext* aPresContext,
                                      const nsHTMLReflowState& aReflowState,
                                      nsHTMLReflowMetrics& aDesiredSize)
{
  // it must be defined, but not called
  NS_ASSERTION(0, "this should never be called");
  aDesiredSize.width   = 0;
  aDesiredSize.height  = 0;
  aDesiredSize.ascent  = 0;
  aDesiredSize.descent = 0;
}

/*******************************************************************************
 * FrameLoadingInfo
 ******************************************************************************/
FrameLoadingInfo::FrameLoadingInfo(const nsSize& aSize)
{
  NS_INIT_REFCNT();

  mFrameSize = aSize;
}

/*
 * Implementation of ISupports methods...
 */
NS_IMPL_ISUPPORTS(FrameLoadingInfo,kISupportsIID);

// XXX temp implementation

NS_IMPL_ADDREF(TempObserver);
NS_IMPL_RELEASE(TempObserver);

/*******************************************************************************
 * TempObserver
 ******************************************************************************/
nsresult
TempObserver::QueryInterface(const nsIID& aIID,
                            void** aInstancePtrResult)
{
  NS_PRECONDITION(nsnull != aInstancePtrResult, "null pointer");
  if (nsnull == aInstancePtrResult) {
    return NS_ERROR_NULL_POINTER;
  }
  if (aIID.Equals(kIStreamObserverIID)) {
    *aInstancePtrResult = (void*) ((nsIStreamObserver*)this);
    AddRef();
    return NS_OK;
  }
  if (aIID.Equals(kISupportsIID)) {
    *aInstancePtrResult = (void*) ((nsISupports*)((nsIDocumentObserver*)this));
    AddRef();
    return NS_OK;
  }
  return NS_NOINTERFACE;
}

#ifndef NECKO
NS_IMETHODIMP
TempObserver::OnProgress(nsIURI* aURL, PRUint32 aProgress, PRUint32 aProgressMax)
{
#if 0
  fputs("[progress ", stdout);
  fputs(mURL, stdout);
  printf(" %d %d ", aProgress, aProgressMax);
  fputs("]\n", stdout);
#endif
  return NS_OK;
}

NS_IMETHODIMP
TempObserver::OnStatus(nsIURI* aURL, const PRUnichar* aMsg)
{
#if 0
  fputs("[status ", stdout);
  fputs(mURL, stdout);
  fputs(aMsg, stdout);
  fputs("]\n", stdout);
#endif
  return NS_OK;
}
#endif

NS_IMETHODIMP
#ifdef NECKO
TempObserver::OnStartRequest(nsISupports *ctxt)
#else
TempObserver::OnStartRequest(nsIURI* aURL, const char *aContentType)
#endif
{
#if 0
  fputs("Loading ", stdout);
  fputs(mURL, stdout);
  fputs("\n", stdout);
#endif
  return NS_OK;
}

NS_IMETHODIMP
#ifdef NECKO
TempObserver::OnStopRequest(nsISupports *ctxt, nsresult status, const PRUnichar *errorMsg)
#else
TempObserver::OnStopRequest(nsIURI* aURL, nsresult status, const PRUnichar* aMsg)
#endif
{
#if 0
  fputs("Done loading ", stdout);
  fputs(mURL, stdout);
  fputs("\n", stdout);
#endif
  return NS_OK;
}


