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
#ifndef nsImageBoxFrame_h___
#define nsImageBoxFrame_h___

#include "nsLeafBoxFrame.h"

#include "imgILoader.h"
#include "imgIRequest.h"
#include "imgIContainer.h"
#include "imgIDecoderObserver.h"

class nsImageBoxFrame;

class nsImageBoxListener : imgIDecoderObserver
{
public:
  nsImageBoxListener();
  virtual ~nsImageBoxListener();

  NS_DECL_ISUPPORTS
  NS_DECL_IMGIDECODEROBSERVER
  NS_DECL_IMGICONTAINEROBSERVER

  void SetFrame(nsImageBoxFrame *frame) { mFrame = frame; }

private:
  nsImageBoxFrame *mFrame;
};

class nsImageBoxFrame : public nsLeafBoxFrame
{
public:

  // nsIBox
  NS_IMETHOD GetPrefSize(nsBoxLayoutState& aBoxLayoutState, nsSize& aSize);
  NS_IMETHOD GetMinSize(nsBoxLayoutState& aBoxLayoutState, nsSize& aSize);
  NS_IMETHOD GetAscent(nsBoxLayoutState& aBoxLayoutState, nscoord& aAscent);
  NS_IMETHOD DoLayout(nsBoxLayoutState& aBoxLayoutState);
  NS_IMETHOD NeedsRecalc();

  friend nsresult NS_NewImageBoxFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame);

  // nsIBox frame interface

  NS_IMETHOD  Init(nsIPresContext*  aPresContext,
                   nsIContent*      aContent,
                   nsIFrame*        aParent,
                   nsIStyleContext* aContext,
                   nsIFrame*        asPrevInFlow);

  NS_IMETHOD AttributeChanged(nsIPresContext* aPresContext,
                              nsIContent* aChild,
                              PRInt32 aNameSpaceID,
                              nsIAtom* aAttribute,
                              PRInt32 aModType, 
                              PRInt32 aHint);

  NS_IMETHOD  DidSetStyleContext (nsIPresContext* aPresContext);

  NS_IMETHOD Destroy(nsIPresContext* aPresContext);

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsString& aResult) const;
#endif

  virtual void UpdateAttributes(nsIPresContext*  aPresContext, nsIAtom* aAttribute, PRBool& aResize, PRBool& aRedraw);
  virtual void UpdateImage(nsIPresContext*  aPresContext, PRBool& aResize);


  NS_IMETHOD  Paint(nsIPresContext*      aPresContext,
                    nsIRenderingContext& aRenderingContext,
                    const nsRect&        aDirtyRect,
                    nsFramePaintLayer    aWhichLayer,
                    PRUint32             aFlags = 0);

  NS_IMETHOD OnStartDecode(imgIRequest *request, nsIPresContext *cx);
  NS_IMETHOD OnStartContainer(imgIRequest *request, nsIPresContext *cx, imgIContainer *image);
  NS_IMETHOD OnStartFrame(imgIRequest *request, nsIPresContext *cx, gfxIImageFrame *frame);
  NS_IMETHOD OnDataAvailable(imgIRequest *request, nsIPresContext *cx, gfxIImageFrame *frame, const nsRect * rect);
  NS_IMETHOD OnStopFrame(imgIRequest *request, nsIPresContext *cx, gfxIImageFrame *frame);
  NS_IMETHOD OnStopContainer(imgIRequest *request, nsIPresContext *cx, imgIContainer *image);
  NS_IMETHOD OnStopDecode(imgIRequest *request, nsIPresContext *cx, nsresult status, const PRUnichar *statusArg);
  NS_IMETHOD FrameChanged(imgIContainer *container, nsIPresContext *cx, gfxIImageFrame *newframe, nsRect * dirtyRect);

  virtual ~nsImageBoxFrame();
protected:

  void CacheImageSize(nsBoxLayoutState& aBoxLayoutState);


  NS_IMETHOD  PaintImage(nsIPresContext* aPresContext,
                         nsIRenderingContext& aRenderingContext,
                         const nsRect& aDirtyRect,
                         nsFramePaintLayer aWhichLayer);

  nsImageBoxFrame(nsIPresShell* aShell);

  void GetImageSource(nsString& aResult);

  void GetBaseURI(nsIURI **uri);

  void GetLoadGroup(nsIPresContext *aPresContext, nsILoadGroup **group);

  virtual void GetImageSize(nsIPresContext* aPresContext);

private:

  nsCOMPtr<imgIRequest> mImageRequest;
  nsCOMPtr<imgIDecoderObserver> mListener;

  nsSize mIntrinsicSize;

  PRBool mSizeFrozen;
  nsSize mImageSize;
  PRBool mHasImage;
  
}; // class nsImageBoxFrame

#endif /* nsImageBoxFrame_h___ */
