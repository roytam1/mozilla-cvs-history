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
 *   IBM Corporation 
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
#include "nsICharsetConverterManager.h"
#include "nsILanguageAtomService.h"
#include "nsIURL.h"
#include "nsIEventStateManager.h"
#include "nsIObserver.h"
#include "nsILookAndFeel.h"
#ifdef IBMBIDI
#include "nsIUBidiUtils.h"
#endif

// Base class for concrete presentation context classes
class nsPresContext : public nsIPresContext, public nsIObserver {
public:
  NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

  // nsISupports methods
  NS_DECL_ISUPPORTS

  // nsIPresContext methods
  NS_IMETHOD Init(nsIDeviceContext* aDeviceContext);
  NS_IMETHOD Stop(PRBool aStopChrome = PR_TRUE);
  NS_IMETHOD SetShell(nsIPresShell* aShell);
  NS_IMETHOD GetShell(nsIPresShell** aResult);
  NS_IMETHOD GetCompatibilityMode(nsCompatibility* aModeResult);
  NS_IMETHOD SetCompatibilityMode(nsCompatibility aMode);
  NS_IMETHOD GetWidgetRenderingMode(nsWidgetRendering* aModeResult);
  NS_IMETHOD SetWidgetRenderingMode(nsWidgetRendering aMode);
  NS_IMETHOD GetImageAnimationMode(nsImageAnimation* aModeResult);
  NS_IMETHOD SetImageAnimationMode(nsImageAnimation aMode);
  NS_IMETHOD GetLookAndFeel(nsILookAndFeel** aLookAndFeel);
  NS_IMETHOD GetBaseURL(nsIURI** aURLResult);
  NS_IMETHOD GetMedium(nsIAtom** aMediumResult) = 0;
  NS_IMETHOD RemapStyleAndReflow(void);
  NS_IMETHOD ResolveStyleContextFor(nsIContent* aContent,
                                    nsIStyleContext* aParentContext,
                                    PRBool aForceUnique,
                                    nsIStyleContext** aResult);
  NS_IMETHOD ResolvePseudoStyleContextFor(nsIContent* aParentContent,
                                          nsIAtom* aPseudoTag,
                                          nsIStyleContext* aParentContext,
                                          PRBool aForceUnique,
                                          nsIStyleContext** aResult);
  NS_IMETHOD ResolvePseudoStyleWithComparator(nsIContent* aParentContent,
                                          nsIAtom* aPseudoTag,
                                          nsIStyleContext* aParentContext,
                                          PRBool aForceUnique,
                                          nsICSSPseudoComparator* aComparator,
                                          nsIStyleContext** aResult);
  NS_IMETHOD ProbePseudoStyleContextFor(nsIContent* aParentContent,
                                        nsIAtom* aPseudoTag,
                                        nsIStyleContext* aParentContext,
                                        PRBool aForceUnique,
                                        nsIStyleContext** aResult);
  NS_IMETHOD ReParentStyleContext(nsIFrame* aFrame, 
                                  nsIStyleContext* aNewParentContext);
  NS_IMETHOD GetMetricsFor(const nsFont& aFont, nsIFontMetrics** aResult);
  NS_IMETHOD AllocateFromShell(size_t aSize, void** aResult);
  NS_IMETHOD FreeToShell(size_t aSize, void* aFreeChunk);
  NS_IMETHOD GetDefaultFont(nsFont& aResult);
  NS_IMETHOD SetDefaultFont(const nsFont& aFont);
  virtual const nsFont& GetDefaultFontDeprecated();
  NS_IMETHOD GetDefaultFixedFont(nsFont& aResult);
  NS_IMETHOD SetDefaultFixedFont(const nsFont& aFont);
  virtual const nsFont& GetDefaultFixedFontDeprecated();
  NS_IMETHOD GetCachedBoolPref(PRUint32 prefType, PRBool &aValue);

  NS_IMETHOD GetFontScaler(PRInt32* aResult);
  NS_IMETHOD SetFontScaler(PRInt32 aScaler);
  NS_IMETHOD GetDefaultColor(nscolor* aColor);
  NS_IMETHOD GetDefaultBackgroundColor(nscolor* aColor);
  NS_IMETHOD GetDefaultBackgroundImage(nsString& aImage);
  NS_IMETHOD GetDefaultBackgroundImageRepeat(PRUint8* aRepeat);
  NS_IMETHOD GetDefaultBackgroundImageOffset(nscoord* aX, nscoord* aY);
  NS_IMETHOD GetDefaultBackgroundImageAttachment(PRUint8* aRepeat);
  NS_IMETHOD GetDefaultLinkColor(nscolor* aColor);
  NS_IMETHOD GetDefaultVisitedLinkColor(nscolor* aColor);

  NS_IMETHOD GetFocusBackgroundColor(nscolor* aColor);
  NS_IMETHOD GetFocusTextColor(nscolor* aColor);
  NS_IMETHOD GetUseFocusColors(PRBool& useFocusColors);
  NS_IMETHOD GetFocusRingWidth(PRUint8 *focusRingWidth);
  NS_IMETHOD GetFocusRingOnAnything(PRBool& focusRingOnAnything);
  NS_IMETHOD SetDefaultColor(nscolor aColor);
  NS_IMETHOD SetDefaultBackgroundColor(nscolor aColor);
  NS_IMETHOD SetDefaultBackgroundImage(const nsString& aImage);
  NS_IMETHOD SetDefaultBackgroundImageRepeat(PRUint8 aRepeat);
  NS_IMETHOD SetDefaultBackgroundImageOffset(nscoord aX, nscoord aY);
  NS_IMETHOD SetDefaultBackgroundImageAttachment(PRUint8 aRepeat);
  NS_IMETHOD SetDefaultLinkColor(nscolor aColor);
  NS_IMETHOD SetDefaultVisitedLinkColor(nscolor aColor);

  NS_IMETHOD GetImageGroup(nsIImageGroup** aGroupResult);
  NS_IMETHOD StartLoadImage(const nsString& aURL,
                            const nscolor* aBackgroundColor,
                            const nsSize* aDesiredSize,
                            nsIFrame* aTargetFrame,
                            nsIFrameImageLoaderCB aCallBack,
                            void* aClosure,
                            void* aKey,
                            nsIFrameImageLoader** aResult);
  NS_IMETHOD StopLoadImage(void* aKey, nsIFrameImageLoader* aLoader);
  NS_IMETHOD StopAllLoadImagesFor(nsIFrame* aTargetFrame, void* aKey);
  NS_IMETHOD SetContainer(nsISupports* aContainer);
  NS_IMETHOD GetContainer(nsISupports** aResult);
  NS_IMETHOD SetLinkHandler(nsILinkHandler* aHandler);
  NS_IMETHOD GetLinkHandler(nsILinkHandler** aResult);
  NS_IMETHOD GetVisibleArea(nsRect& aResult);
  NS_IMETHOD SetVisibleArea(const nsRect& r);
  NS_IMETHOD IsPaginated(PRBool* aResult) = 0;
  NS_IMETHOD GetPageDim(nsRect* aActualRect, nsRect* aAdjRect) = 0;
  NS_IMETHOD SetPageDim(nsRect* aRect) = 0;
  NS_IMETHOD GetPixelsToTwips(float* aResult) const;
  NS_IMETHOD GetTwipsToPixels(float* aResult) const;
  NS_IMETHOD GetScaledPixelsToTwips(float* aScale) const;
  NS_IMETHOD GetDeviceContext(nsIDeviceContext** aResult) const;
  NS_IMETHOD GetEventStateManager(nsIEventStateManager** aManager);
  NS_IMETHOD GetLanguage(nsILanguageAtom** aLanguage);
  NS_IMETHOD GetLanguageSpecificTransformType(
              nsLanguageSpecificTransformType* aType);

  NS_IMETHOD SetIsRenderingOnlySelection(PRBool aVal) { mIsRenderingOnlySelection = aVal; return NS_OK; }
  NS_IMETHOD IsRenderingOnlySelection(PRBool* aResult);

#ifdef MOZ_REFLOW_PERF
  NS_IMETHOD CountReflows(const char * aName, PRUint32 aType, nsIFrame * aFrame);
  NS_IMETHOD PaintCount(const char * aName, nsIRenderingContext* aRenderingContext, nsIFrame * aFrame, PRUint32 aColor);
#endif

  // nsIObserver method
  NS_IMETHOD Observe(nsISupports* aSubject, const PRUnichar* aTopic,
                     const PRUnichar* aData);

#ifdef IBMBIDI
  NS_IMETHOD GetBidiEnabled(PRBool* aBidiEnabled) const;
  NS_IMETHOD SetBidiEnabled(PRBool aBidiEnabled) const;
  NS_IMETHOD IsVisualMode(PRBool& aIsVisual) const;
  NS_IMETHOD SetVisualMode(PRBool aIsVisual);
  NS_IMETHOD GetBidiUtils(nsBidiPresUtils** aBidiUtils);
  NS_IMETHOD SetBidi(PRUint32 aSource, PRBool aForceReflow = PR_FALSE);
  NS_IMETHOD GetBidi(PRUint32* aDest);
 //ahmed
  NS_IMETHOD IsVisRTL(PRBool &aResult);
  NS_IMETHOD IsArabicEncoding(PRBool &aResult);

//Mohamed  17-1-01
  NS_IMETHOD SetIsBidiSystem(PRBool aIsBidi);
  NS_IMETHOD GetIsBidiSystem(PRBool &aResult) const;
  NS_IMETHOD GetBidiCharset(nsAutoString &aCharSet);
//Mohamed End
#endif // IBMBIDI

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
  nsCOMPtr<nsILanguageAtomService> mLangService;
  nsCOMPtr<nsILanguageAtom> mLanguage;
  nsLanguageSpecificTransformType mLanguageSpecificTransformType;
  nsCOMPtr<nsIImageGroup> mImageGroup;
  nsILinkHandler*       mLinkHandler;   // [WEAK]
  nsISupports*          mContainer;     // [WEAK]
  nsCOMPtr<nsILookAndFeel> mLookAndFeel;
  nsFont                mDefaultFont;
  nsFont                mDefaultFixedFont;
  PRInt32               mFontScaler;
  PRPackedBool          mUseDocumentFonts;        // set in GetUserPrefs
  nscolor               mDefaultColor;            // set in GetUserPrefs
  nscolor               mDefaultBackgroundColor;  // set in GetUserPrefs
  PRPackedBool          mUseDocumentColors;       // set in GetUserPrefs
  nscolor               mLinkColor;               // set in GetUserPrefs
  nscolor               mVisitedLinkColor;        // set in GetUserPrefs
  PRPackedBool          mUnderlineLinks;          // set in GetUserPrefs
  PRPackedBool          mUseFocusColors;          // set in GetUserPrefs
  nscolor               mFocusTextColor;          // set in GetUserPrefs
  nscolor               mFocusBackgroundColor;    // set in GetUserPrefs
  PRUint8               mFocusRingWidth;          // set in GetUserPrefs
  PRPackedBool          mFocusRingOnAnything;     // set in GetUserPrefs
  nsString              mDefaultBackgroundImage;
  PRUint8               mDefaultBackgroundImageRepeat;
  nscoord               mDefaultBackgroundImageOffsetX;
  nscoord               mDefaultBackgroundImageOffsetY;
  PRUint8               mDefaultBackgroundImageAttachment;
  nsVoidArray           mImageLoaders;
  nsCOMPtr<nsIEventStateManager> mEventManager;
  nsCOMPtr<nsIURI>      mBaseURL;
  nsCompatibility       mCompatibilityMode;
  PRPackedBool          mCompatibilityLocked;
  nsWidgetRendering     mWidgetRenderingMode;
  nsImageAnimation      mImageAnimationMode;
  PRPackedBool          mImageAnimationStopped;   // image animation stopped
  PRPackedBool          mStopped;                 // loading stopped
  PRPackedBool          mStopChrome;              // should we stop chrome?
#ifdef IBMBIDI
  PRPackedBool          mIsVisual;                // is the Bidi text mode visual
  PRPackedBool          mIsBidiSystem;            // is the system capable of doing Bidi reordering
#endif // IBMBIDI
  PRPackedBool          mIsRenderingOnlySelection;

#ifdef IBMBIDI
  nsBidiPresUtils*      mBidiUtils;
  PRUint32              mBidi;
  nsAutoString          mCharset;                 // the charset we are using
#endif // IBMBIDI

#ifdef DEBUG
  PRBool                mInitialized;
#endif

  nsImageAnimation      mImageAnimationModePref;

protected:
  void   GetUserPreferences();
  void   GetFontPreferences();
  void   UpdateCharSet(const PRUnichar* aCharSet);

private:
  friend int PR_CALLBACK PrefChangedCallback(const char*, void*);
  void   PreferenceChanged(const char* aPrefName);
};

#endif /* nsPresContext_h___ */
