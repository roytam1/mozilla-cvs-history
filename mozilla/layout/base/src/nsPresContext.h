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
#ifndef nsPresContext_h___
#define nsPresContext_h___

#include "nsIPresContext.h"
#include "nsIDeviceContext.h"
#include "nsVoidArray.h"
#include "nsFont.h"
#include "nsCRT.h"
#include "nsCOMPtr.h"
#include "nsIImageGroup.h"
#include "nsIPref.h"
#include "nsIURL.h"
#include "nsIEventStateManager.h"

// Base class for concrete presentation context classes
class nsPresContext : public nsIPresContext {
public:
  NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

  // nsISupports methods
  NS_DECL_ISUPPORTS

  // nsIPresContext methods
  NS_IMETHOD Init(nsIDeviceContext* aDeviceContext, nsIPref* aPrefs);
  NS_IMETHOD Stop(void);
  NS_IMETHOD SetShell(nsIPresShell* aShell);
  NS_IMETHOD GetShell(nsIPresShell** aResult);
  NS_IMETHOD GetPrefs(nsIPref** aPrefsResult);
  NS_IMETHOD GetCompatibilityMode(nsCompatibility* aModeResult);
  NS_IMETHOD SetCompatibilityMode(nsCompatibility aMode);
  NS_IMETHOD GetWidgetRenderingMode(nsWidgetRendering* aModeResult);
  NS_IMETHOD SetWidgetRenderingMode(nsWidgetRendering aMode);
  NS_IMETHOD GetLookAndFeel(nsILookAndFeel** aLookAndFeel);
  NS_IMETHOD GetBaseURL(nsIURI** aURLResult);
  NS_IMETHOD GetMedium(nsIAtom** aMediumResult) = 0;
  NS_IMETHOD ResolveStyleContextFor(nsIContent* aContent,
                                    nsIStyleContext* aParentContext,
                                    PRBool aForceUnique,
                                    nsIStyleContext** aResult);
  NS_IMETHOD ResolvePseudoStyleContextFor(nsIContent* aParentContent,
                                          nsIAtom* aPseudoTag,
                                          nsIStyleContext* aParentContext,
                                          PRBool aForceUnique,
                                          nsIStyleContext** aResult);
  NS_IMETHOD ProbePseudoStyleContextFor(nsIContent* aParentContent,
                                        nsIAtom* aPseudoTag,
                                        nsIStyleContext* aParentContext,
                                        PRBool aForceUnique,
                                        nsIStyleContext** aResult);
  NS_IMETHOD ReParentStyleContext(nsIFrame* aFrame, 
                                  nsIStyleContext* aNewParentContext);
  NS_IMETHOD GetMetricsFor(const nsFont& aFont, nsIFontMetrics** aResult);
  NS_IMETHOD GetDefaultFont(nsFont& aResult);
  NS_IMETHOD SetDefaultFont(nsFont& aFont);
  virtual const nsFont& GetDefaultFontDeprecated();
  NS_IMETHOD GetDefaultFixedFont(nsFont& aResult);
  NS_IMETHOD SetDefaultFixedFont(nsFont& aFont);
  virtual const nsFont& GetDefaultFixedFontDeprecated();
  NS_IMETHOD GetFontScaler(PRInt32* aResult);
  NS_IMETHOD SetFontScaler(PRInt32 aScaler);
  NS_IMETHOD GetDefaultColor(nscolor* aColor);
  NS_IMETHOD GetDefaultBackgroundColor(nscolor* aColor);
  NS_IMETHOD GetDefaultBackgroundImage(nsString& aImage);
  NS_IMETHOD GetDefaultBackgroundImageRepeat(PRUint8* aRepeat);
  NS_IMETHOD GetDefaultBackgroundImageOffset(nscoord* aX, nscoord* aY);
  NS_IMETHOD GetDefaultBackgroundImageAttachment(PRUint8* aRepeat);

  NS_IMETHOD SetDefaultColor(nscolor aColor);
  NS_IMETHOD SetDefaultBackgroundColor(nscolor aColor);
  NS_IMETHOD SetDefaultBackgroundImage(const nsString& aImage);
  NS_IMETHOD SetDefaultBackgroundImageRepeat(PRUint8 aRepeat);
  NS_IMETHOD SetDefaultBackgroundImageOffset(nscoord aX, nscoord aY);
  NS_IMETHOD SetDefaultBackgroundImageAttachment(PRUint8 aRepeat);

  NS_IMETHOD GetImageGroup(nsIImageGroup** aGroupResult);
  NS_IMETHOD StartLoadImage(const nsString& aURL,
                            const nscolor* aBackgroundColor,
                            const nsSize* aDesiredSize,
                            nsIFrame* aTargetFrame,
                            nsIFrameImageLoaderCB aCallBack,
                            void* aClosure,
                            nsIFrameImageLoader** aResult);
  NS_IMETHOD StopLoadImage(nsIFrame* aForFrame, nsIFrameImageLoader* aLoader);
  NS_IMETHOD StopAllLoadImagesFor(nsIFrame* aForFrame);
  NS_IMETHOD SetContainer(nsISupports* aContainer);
  NS_IMETHOD GetContainer(nsISupports** aResult);
  NS_IMETHOD SetLinkHandler(nsILinkHandler* aHandler);
  NS_IMETHOD GetLinkHandler(nsILinkHandler** aResult);
  NS_IMETHOD GetVisibleArea(nsRect& aResult);
  NS_IMETHOD SetVisibleArea(const nsRect& r);
  NS_IMETHOD IsPaginated(PRBool* aResult) = 0;
  NS_IMETHOD GetPageWidth(nscoord* aResult) = 0;
  NS_IMETHOD GetPageHeight(nscoord* aResult) = 0;
  NS_IMETHOD GetPixelsToTwips(float* aResult) const;
  NS_IMETHOD GetTwipsToPixels(float* aResult) const;
  NS_IMETHOD GetScaledPixelsToTwips(float* aScale) const;
  NS_IMETHOD GetDeviceContext(nsIDeviceContext** aResult) const;
  NS_IMETHOD GetEventStateManager(nsIEventStateManager** aManager);

protected:
  nsPresContext();
  virtual ~nsPresContext();

  // IMPORTANT: The ownership implicit in the following member variables has been 
  // explicitly checked and set using nsCOMPtr for owning pointers and raw COM interface 
  // pointers for weak (ie, non owning) references. If you add any members to this
  // class, please make the ownership explicit (pinkerton, scc).
  
  nsIPresShell*         mShell;         // [WEAK]
  nsCOMPtr<nsIPref>     mPrefs;
  nsRect                mVisibleArea;
  nsCOMPtr<nsIDeviceContext>  mDeviceContext; // could be weak, but better safe than sorry. Cannot reintroduce cycles
                                              // since there is no dependency from gfx back to layout.
  nsCOMPtr<nsIImageGroup> mImageGroup;
  nsILinkHandler*       mLinkHandler;   // [WEAK]
  nsISupports*          mContainer;     // [WEAK]
  nsILookAndFeel*       mLookAndFeel;
  nsFont                mDefaultFont;
  nsFont                mDefaultFixedFont;
  PRInt32               mFontScaler;
  nscolor               mDefaultColor;
  nscolor               mDefaultBackgroundColor;
  nsString              mDefaultBackgroundImage;
  PRUint8               mDefaultBackgroundImageRepeat;
  nscoord               mDefaultBackgroundImageOffsetX;
  nscoord               mDefaultBackgroundImageOffsetY;
  PRUint8               mDefaultBackgroundImageAttachment;
  nsVoidArray           mImageLoaders;
  nsCOMPtr<nsIEventStateManager> mEventManager;
  nsCompatibility       mCompatibilityMode;
  PRBool                mCompatibilityLocked;
  nsWidgetRendering     mWidgetRenderingMode;
  nsCOMPtr<nsIURI>      mBaseURL;
  PRBool                mStopped;

#ifdef DEBUG
  PRBool                mInitialized;
#endif

protected:
  void   GetUserPreferences();

private:
  friend int PrefChangedCallback(const char*, void*);
  void   PreferenceChanged(const char* aPrefName);
};

#endif /* nsPresContext_h___ */
