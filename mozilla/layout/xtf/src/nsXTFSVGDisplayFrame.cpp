/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ----- BEGIN LICENSE BLOCK -----
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Mozilla XTF project.
 *
 * The Initial Developer of the Original Code is 
 * Alex Fritze.
 * Portions created by the Initial Developer are Copyright (C) 2004
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *    Alex Fritze <alex@croczilla.com> (original author)
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or 
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ----- END LICENSE BLOCK ----- */

#include "nsCOMPtr.h"
#include "nsContainerFrame.h"
#include "nsPresContext.h"
#include "nsISupportsArray.h"
#include "nsISVGChildFrame.h"
#include "nsISVGContainerFrame.h"
#include "nsISVGOuterSVGFrame.h"
#include "nsIXTFVisualWrapperPrivate.h"
#include "nsIAnonymousContentCreator.h"

typedef nsContainerFrame nsXTFSVGDisplayFrameBase;

class nsXTFSVGDisplayFrame : public nsXTFSVGDisplayFrameBase,
                             public nsISVGChildFrame,
                             public nsISVGContainerFrame,
                             public nsIAnonymousContentCreator
{
  friend nsresult
  NS_NewXTFSVGDisplayFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsIFrame** aNewFrame);
protected:
  nsXTFSVGDisplayFrame();
  virtual ~nsXTFSVGDisplayFrame();
  nsresult Init();
  
  // nsISupports interface:
  NS_IMETHOD QueryInterface(const nsIID& aIID, void** aInstancePtr);
private:
  NS_IMETHOD_(nsrefcnt) AddRef() { return NS_OK; }
  NS_IMETHOD_(nsrefcnt) Release() { return NS_OK; }  
public:
  // nsIFrame:
  
  NS_IMETHOD  AppendFrames(nsPresContext* aPresContext,
                           nsIPresShell&   aPresShell,
                           nsIAtom*        aListName,
                           nsIFrame*       aFrameList);
  NS_IMETHOD  InsertFrames(nsPresContext* aPresContext,
                           nsIPresShell&   aPresShell,
                           nsIAtom*        aListName,
                           nsIFrame*       aPrevFrame,
                           nsIFrame*       aFrameList);
  NS_IMETHOD  RemoveFrame(nsPresContext* aPresContext,
                          nsIPresShell&   aPresShell,
                          nsIAtom*        aListName,
                          nsIFrame*       aOldFrame);
  NS_IMETHOD  ReplaceFrame(nsPresContext* aPresContext,
                           nsIPresShell&   aPresShell,
                           nsIAtom*        aListName,
                           nsIFrame*       aOldFrame,
                           nsIFrame*       aNewFrame);
  NS_IMETHOD Init(nsPresContext*  aPresContext,
                  nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsStyleContext*  aContext,
                  nsIFrame*        aPrevInFlow);

  NS_IMETHOD  AttributeChanged(nsPresContext* aPresContext,
                               nsIContent*     aChild,
                               PRInt32         aNameSpaceID,
                               nsIAtom*        aAttribute,
                               PRInt32         aModType);

  // nsISVGChildFrame interface:
  NS_IMETHOD Paint(nsISVGRendererCanvas* canvas, const nsRect& dirtyRectTwips);
  NS_IMETHOD GetFrameForPoint(float x, float y, nsIFrame** hit);  
  NS_IMETHOD_(already_AddRefed<nsISVGRendererRegion>) GetCoveredRegion();
  NS_IMETHOD InitialUpdate();
  NS_IMETHOD NotifyCanvasTMChanged();
  NS_IMETHOD NotifyRedrawSuspended();
  NS_IMETHOD NotifyRedrawUnsuspended();
  NS_IMETHOD GetBBox(nsIDOMSVGRect **_retval);
  
  // nsISVGContainerFrame interface:
  nsISVGOuterSVGFrame*GetOuterSVGFrame();
  already_AddRefed<nsIDOMSVGMatrix> GetCanvasTM();
  already_AddRefed<nsSVGCoordCtxProvider> GetCoordContextProvider();

  // nsIAnonymousContentCreator
  NS_IMETHOD CreateAnonymousContent(nsPresContext* aPresContext,
                                    nsISupportsArray& aAnonymousItems);
  
  // If the creator doesn't want to create special frame for frame hierarchy
  // then it should null out the style content arg and return NS_ERROR_FAILURE
  NS_IMETHOD CreateFrameFor(nsPresContext*   aPresContext,
                            nsIContent *      aContent,
                            nsIFrame**        aFrame) {
    if (aFrame) *aFrame = nsnull; return NS_ERROR_FAILURE; }
  
protected:
};

//----------------------------------------------------------------------
// Implementation

nsresult
NS_NewXTFSVGDisplayFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsIFrame** aNewFrame)
{
  nsXTFSVGDisplayFrame* it = new (aPresShell) nsXTFSVGDisplayFrame;
  if (nsnull == it)
    return NS_ERROR_OUT_OF_MEMORY;

  *aNewFrame = it;

  return NS_OK;
}

nsXTFSVGDisplayFrame::nsXTFSVGDisplayFrame()
{
#ifdef DEBUG
//  printf("nsXTFSVGDisplayFrame CTOR\n");
#endif
}

nsXTFSVGDisplayFrame::~nsXTFSVGDisplayFrame()
{
#ifdef DEBUG
//  printf("~nsXTFSVGDisplayFrame\n");
#endif
}

nsresult nsXTFSVGDisplayFrame::Init()
{
  return NS_OK;
}

//----------------------------------------------------------------------
// nsISupports methods

NS_INTERFACE_MAP_BEGIN(nsXTFSVGDisplayFrame)
  NS_INTERFACE_MAP_ENTRY(nsISVGChildFrame)
  NS_INTERFACE_MAP_ENTRY(nsISVGContainerFrame)
  NS_INTERFACE_MAP_ENTRY(nsIAnonymousContentCreator)
NS_INTERFACE_MAP_END_INHERITING(nsXTFSVGDisplayFrameBase)


//----------------------------------------------------------------------
// nsIFrame methods
  
NS_IMETHODIMP
nsXTFSVGDisplayFrame::Init(nsPresContext*  aPresContext,
                           nsIContent*      aContent,
                           nsIFrame*        aParent,
                           nsStyleContext*  aContext,
                           nsIFrame*        aPrevInFlow)
{
  nsresult rv;
  rv = nsXTFSVGDisplayFrameBase::Init(aPresContext, aContent, aParent,
                                      aContext, aPrevInFlow);

  Init();
  
  return rv;
}


NS_IMETHODIMP
nsXTFSVGDisplayFrame::AppendFrames(nsPresContext* aPresContext,
                                   nsIPresShell&   aPresShell,
                                   nsIAtom*        aListName,
                                   nsIFrame*       aFrameList)
{
  // append == insert at end:
  return InsertFrames(aPresContext, aPresShell, aListName,
                      mFrames.LastChild(), aFrameList);  
}

NS_IMETHODIMP
nsXTFSVGDisplayFrame::InsertFrames(nsPresContext* aPresContext,
                                   nsIPresShell&   aPresShell,
                                   nsIAtom*        aListName,
                                   nsIFrame*       aPrevFrame,
                                   nsIFrame*       aFrameList)
{
  nsIFrame* lastNewFrame = nsnull;
  {
    nsFrameList tmpList(aFrameList);
    lastNewFrame = tmpList.LastChild();
  }
  
  // Insert the new frames
  mFrames.InsertFrames(nsnull, aPrevFrame, aFrameList);

  // call InitialUpdate() on all new frames:
  nsIFrame* end = nsnull;
  if (lastNewFrame)
    end = lastNewFrame->GetNextSibling();

  for (nsIFrame*kid=aFrameList; kid != end; kid = kid->GetNextSibling()) {
    nsISVGChildFrame* SVGFrame=nsnull;
    kid->QueryInterface(NS_GET_IID(nsISVGChildFrame),(void**)&SVGFrame);
    if (SVGFrame) {
      SVGFrame->InitialUpdate(); 
    }
  }
  
  return NS_OK;
}

NS_IMETHODIMP
nsXTFSVGDisplayFrame::RemoveFrame(nsPresContext* aPresContext,
                                  nsIPresShell&   aPresShell,
                                  nsIAtom*        aListName,
                                  nsIFrame*       aOldFrame)
{
  nsCOMPtr<nsISVGRendererRegion> dirty_region;
  
  nsISVGChildFrame* SVGFrame=nsnull;
  aOldFrame->QueryInterface(NS_GET_IID(nsISVGChildFrame),(void**)&SVGFrame);

  if (SVGFrame)
    dirty_region = SVGFrame->GetCoveredRegion();

  PRBool result = mFrames.DestroyFrame(aPresContext, aOldFrame);

  nsISVGOuterSVGFrame* outerSVGFrame = GetOuterSVGFrame();
  NS_ASSERTION(outerSVGFrame, "no outer svg frame");
  if (dirty_region && outerSVGFrame)
    outerSVGFrame->InvalidateRegion(dirty_region, PR_TRUE);

  NS_ASSERTION(result, "didn't find frame to delete");
  return result ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsXTFSVGDisplayFrame::ReplaceFrame(nsPresContext* aPresContext,
                                   nsIPresShell&   aPresShell,
                                   nsIAtom*        aListName,
                                   nsIFrame*       aOldFrame,
                                   nsIFrame*       aNewFrame)
{
  NS_NOTYETIMPLEMENTED("write me!");
  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsXTFSVGDisplayFrame::AttributeChanged(nsPresContext* aPresContext,
                                       nsIContent*     aChild,
                                       PRInt32         aNameSpaceID,
                                       nsIAtom*        aAttribute,
                                       PRInt32         aModType)
{
#ifdef DEBUG
//     printf("** nsXTFSVGDisplayFrame::AttributeChanged(");
//     nsAutoString str;
//     aAttribute->ToString(str);
//     nsCAutoString cstr;
//     cstr.AssignWithConversion(str);
//     printf(cstr.get());
//     printf("\n");
#endif

  return NS_OK;
}

//----------------------------------------------------------------------
// nsISVGChildFrame methods

NS_IMETHODIMP
nsXTFSVGDisplayFrame::Paint(nsISVGRendererCanvas* canvas, const nsRect& dirtyRectTwips)
{
#ifdef DEBUG
//  printf("nsXTFSVGDisplayFrame(%p)::Paint\n", this);
#endif
  for (nsIFrame* kid = mFrames.FirstChild(); kid;
       kid = kid->GetNextSibling()) {
    nsISVGChildFrame* SVGFrame=nsnull;
    kid->QueryInterface(NS_GET_IID(nsISVGChildFrame),(void**)&SVGFrame);
    if (SVGFrame)
      SVGFrame->Paint(canvas, dirtyRectTwips);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsXTFSVGDisplayFrame::GetFrameForPoint(float x, float y, nsIFrame** hit)
{
#ifdef DEBUG
//  printf("nsXTFSVGDisplayFrame(%p)::GetFrameForPoint\n", this);
#endif
  *hit = nsnull;
  for (nsIFrame* kid = mFrames.FirstChild(); kid;
       kid = kid->GetNextSibling()) {
    nsISVGChildFrame* SVGFrame=nsnull;
    kid->QueryInterface(NS_GET_IID(nsISVGChildFrame),(void**)&SVGFrame);
    if (SVGFrame) {
      nsIFrame* temp=nsnull;
      nsresult rv = SVGFrame->GetFrameForPoint(x, y, &temp);
      if (NS_SUCCEEDED(rv) && temp) {
        *hit = temp;
        // return NS_OK; can't return. we need reverse order but only
        // have a singly linked list...
      }
    }
  }
  
  return *hit ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP_(already_AddRefed<nsISVGRendererRegion>)
nsXTFSVGDisplayFrame::GetCoveredRegion()
{
  nsISVGRendererRegion *accu_region=nsnull;
  
  nsIFrame* kid = mFrames.FirstChild();
  while (kid) {
    nsISVGChildFrame* SVGFrame=0;
    kid->QueryInterface(NS_GET_IID(nsISVGChildFrame),(void**)&SVGFrame);
    if (SVGFrame) {
      nsCOMPtr<nsISVGRendererRegion> dirty_region = SVGFrame->GetCoveredRegion();
      if (dirty_region) {
        if (accu_region) {
          nsCOMPtr<nsISVGRendererRegion> temp = dont_AddRef(accu_region);
          dirty_region->Combine(temp, &accu_region);
        }
        else {
          accu_region = dirty_region;
          NS_IF_ADDREF(accu_region);
        }
      }
    }
    kid = kid->GetNextSibling();
  }
  
  return accu_region;
}

NS_IMETHODIMP
nsXTFSVGDisplayFrame::InitialUpdate()
{
  nsIFrame* kid = mFrames.FirstChild();
  while (kid) {
    nsISVGChildFrame* SVGFrame=0;
    kid->QueryInterface(NS_GET_IID(nsISVGChildFrame),(void**)&SVGFrame);
    if (SVGFrame) {
      SVGFrame->InitialUpdate();
    }
    kid = kid->GetNextSibling();
  }
  return NS_OK;
}  

NS_IMETHODIMP
nsXTFSVGDisplayFrame::NotifyCanvasTMChanged()
{
  for (nsIFrame* kid = mFrames.FirstChild(); kid;
       kid = kid->GetNextSibling()) {
    nsISVGChildFrame* SVGFrame=nsnull;
    kid->QueryInterface(NS_GET_IID(nsISVGChildFrame),(void**)&SVGFrame);
    if (SVGFrame) {
      SVGFrame->NotifyCanvasTMChanged();
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsXTFSVGDisplayFrame::NotifyRedrawSuspended()
{
  for (nsIFrame* kid = mFrames.FirstChild(); kid;
       kid = kid->GetNextSibling()) {
    nsISVGChildFrame* SVGFrame=0;
    kid->QueryInterface(NS_GET_IID(nsISVGChildFrame),(void**)&SVGFrame);
    if (SVGFrame) {
      SVGFrame->NotifyRedrawSuspended();
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsXTFSVGDisplayFrame::NotifyRedrawUnsuspended()
{
  for (nsIFrame* kid = mFrames.FirstChild(); kid;
       kid = kid->GetNextSibling()) {
    nsISVGChildFrame* SVGFrame=nsnull;
    kid->QueryInterface(NS_GET_IID(nsISVGChildFrame),(void**)&SVGFrame);
    if (SVGFrame) {
      SVGFrame->NotifyRedrawUnsuspended();
    }
  }
 return NS_OK;
}

NS_IMETHODIMP
nsXTFSVGDisplayFrame::GetBBox(nsIDOMSVGRect **_retval)
{
  *_retval = nsnull;
  return NS_ERROR_FAILURE;
}

//----------------------------------------------------------------------
// nsISVGContainerFrame methods:

nsISVGOuterSVGFrame *
nsXTFSVGDisplayFrame::GetOuterSVGFrame()
{
  NS_ASSERTION(mParent, "null parent");
  
  nsISVGContainerFrame *containerFrame;
  mParent->QueryInterface(NS_GET_IID(nsISVGContainerFrame), (void**)&containerFrame);
  if (!containerFrame) {
    NS_ERROR("invalid container");
    return nsnull;
  }

  return containerFrame->GetOuterSVGFrame();  
}

already_AddRefed<nsIDOMSVGMatrix>
nsXTFSVGDisplayFrame::GetCanvasTM()
{
  NS_ASSERTION(mParent, "null parent");
  
  nsISVGContainerFrame *containerFrame;
  mParent->QueryInterface(NS_GET_IID(nsISVGContainerFrame), (void**)&containerFrame);
  if (!containerFrame) {
    NS_ERROR("invalid container");
    return nsnull;
  }

  return containerFrame->GetCanvasTM();  
}

already_AddRefed<nsSVGCoordCtxProvider>
nsXTFSVGDisplayFrame::GetCoordContextProvider()
{
  NS_ASSERTION(mParent, "null parent");
  
  nsISVGContainerFrame *containerFrame;
  mParent->QueryInterface(NS_GET_IID(nsISVGContainerFrame), (void**)&containerFrame);
  if (!containerFrame) {
    NS_ERROR("invalid container");
    return nsnull;
  }

  return containerFrame->GetCoordContextProvider();  
}

//----------------------------------------------------------------------
// nsIAnonymousContentCreator methods:

NS_IMETHODIMP
nsXTFSVGDisplayFrame::CreateAnonymousContent(nsPresContext* aPresContext,
                                             nsISupportsArray& aAnonymousItems)
{
  nsCOMPtr<nsIXTFVisualWrapperPrivate> visual = do_QueryInterface(mContent);
  NS_ASSERTION(visual, "huh? associated content not implementing nsIXTFVisualWrapperPrivate");
  return visual->CreateAnonymousContent(aPresContext, aAnonymousItems);
}
