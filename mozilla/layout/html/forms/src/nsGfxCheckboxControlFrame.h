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
#ifndef nsGfxCheckboxControlFrame_h___
#define nsGfxCheckboxControlFrame_h___

#include "nsFormControlFrame.h"
#include "nsIStatefulFrame.h"
#include "nsICheckboxControlFrame.h"


#define NS_GFX_CHECKBOX_CONTROL_FRAME_FACE_CONTEXT_INDEX   0 // for additional style contexts
#define NS_GFX_CHECKBOX_CONTROL_FRAME_LAST_CONTEXT_INDEX   0

class nsGfxCheckboxControlFrame : public nsFormControlFrame,
                                  public nsIStatefulFrame,
                                  public nsICheckboxControlFrame
{
public:
  nsGfxCheckboxControlFrame();
  virtual ~nsGfxCheckboxControlFrame();
  
#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsString& aResult) const {
    return MakeFrameName("CheckboxControl", aResult);
  }
#endif
   // this should be protected, but VC6 is lame.
  enum CheckState { eOff, eOn, eMixed } ;

  NS_IMETHOD Init(nsIPresContext*  aPresContext,
                  nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIStyleContext* aContext,
                  nsIFrame*        aPrevInFlow) ;

  NS_IMETHOD AttributeChanged(nsIPresContext* aPresContext,
                              nsIContent*     aChild,
                              PRInt32         aNameSpaceID,
                              nsIAtom*        aAttribute,
                              PRInt32         aHint) ;

  NS_IMETHOD Paint(nsIPresContext* aPresContext,
                   nsIRenderingContext& aRenderingContext,
                   const nsRect& aDirtyRect,
                   nsFramePaintLayer aWhichLayer);

   //nsIRadioControlFrame methods
  NS_IMETHOD SetCheckboxFaceStyleContext(nsIStyleContext *aCheckboxFaceStyleContext);

  void InitializeControl(nsIPresContext* aPresContext);

  NS_IMETHOD GetAdditionalStyleContext(PRInt32 aIndex, 
                                       nsIStyleContext** aStyleContext) const;
  NS_IMETHOD SetAdditionalStyleContext(PRInt32 aIndex, 
                                       nsIStyleContext* aStyleContext);

    // nsIFormControlFrame
  NS_IMETHOD SetProperty(nsIPresContext* aPresContext, nsIAtom* aName, const nsString& aValue);
  NS_IMETHOD GetProperty(nsIAtom* aName, nsString& aValue); 
  virtual void Reset(nsIPresContext* aPresContext);
  virtual PRInt32 GetMaxNumValues();
  virtual PRBool GetNamesValues(PRInt32 aMaxNumValues, PRInt32& aNumValues,
                                nsString* aValues, nsString* aNames);

   // nsIStatefulFrame
  NS_IMETHOD QueryInterface(const nsIID& aIID, void** aInstancePtr);
  NS_IMETHOD GetStateType(nsIPresContext* aPresContext, nsIStatefulFrame::StateType* aStateType);
  NS_IMETHOD SaveState(nsIPresContext* aPresContext, nsIPresState** aState);
  NS_IMETHOD RestoreState(nsIPresContext* aPresContext, nsIPresState* aState);


#ifdef DEBUG_rodsXXX
  NS_IMETHOD Reflow(nsIPresContext*          aCX,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);
#endif

protected:

    // native/gfx implementations need to implement needs.
  CheckState GetCheckboxState();
  void SetCheckboxState(nsIPresContext* aPresContext, CheckState aValue);

   // Utility methods for implementing SetProperty/GetProperty
  void SetCheckboxControlFrameState(nsIPresContext* aPresContext,
                                    const nsString& aValue);
  void GetCheckboxControlFrameState(nsString& aValue);  

    // utility routine for converting from DOM values to internal enum
  void CheckStateToString ( CheckState inState, nsString& outStateAsString ) ;
  CheckState StringToCheckState ( const nsString & aStateAsString ) ;

    // figure out if we're a tri-state checkbox.
  PRBool IsTristateCheckbox ( ) const { return mIsTristate; }

    // we just became a tri-state, or we just lost tri-state status. fix up
    // the attributes for the new mode.
  void SwitchModesWithEmergencyBrake ( nsIPresContext* aPresContext,
                                       PRBool inIsNowTristate ) ;
  
    // for tri-state checkbox. meaningless for normal HTML
  PRBool mIsTristate;
  
  static nsIAtom* GetTristateAtom() ;
  static nsIAtom* GetTristateValueAtom() ;

protected:

  virtual void PaintCheckBox(nsIPresContext* aPresContext,
                             nsIRenderingContext& aRenderingContext,
                             const nsRect& aDirtyRect,
                             nsFramePaintLayer aWhichLayer);
  virtual void PaintMixedMark(nsIRenderingContext& aRenderingContext,
                               float aPixelsToTwips, const nsRect& aRect) ;

    //GFX-rendered state variables
  CheckState mChecked;
  nsIStyleContext* mCheckButtonFaceStyle;

private:
  NS_IMETHOD_(nsrefcnt) AddRef() { return NS_OK; }
  NS_IMETHOD_(nsrefcnt) Release() { return NS_OK; }
 
};

#endif
