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

#ifndef nsGfxRadioControlFrame_h___
#define nsGfxRadioControlFrame_h___

#include "nsFormControlFrame.h"
#include "nsIStatefulFrame.h"
#include "nsIRadioControlFrame.h"

// nsGfxRadioControlFrame

#define NS_GFX_RADIO_CONTROL_FRAME_FACE_CONTEXT_INDEX   0 // for additional style contexts
#define NS_GFX_RADIO_CONTROL_FRAME_LAST_CONTEXT_INDEX   0

class nsGfxRadioControlFrame : public nsFormControlFrame,
                               public nsIStatefulFrame,
                               public nsIRadioControlFrame

{
private:

public:
  nsGfxRadioControlFrame();
  ~nsGfxRadioControlFrame();

   //nsIRadioControlFrame methods
  NS_IMETHOD QueryInterface(const nsIID& aIID, void** aInstancePtr);
  NS_IMETHOD SetRadioButtonFaceStyleContext(nsIStyleContext *aRadioButtonFaceStyleContext);


  virtual PRBool GetChecked(PRBool aGetInitialValue);
  virtual void   SetChecked(nsIPresContext* aPresContext, PRBool aValue, PRBool aSetInitialValue);

  void InitializeControl(nsIPresContext* aPresContext);

  NS_IMETHOD GetAdditionalStyleContext(PRInt32 aIndex, 
                                       nsIStyleContext** aStyleContext) const;
  NS_IMETHOD SetAdditionalStyleContext(PRInt32 aIndex, 
                                       nsIStyleContext* aStyleContext);

  // overrides base class HandleEvent to do nothing
  // events are handled by the DOM
  NS_IMETHOD HandleEvent(nsIPresContext* aPresContext, 
                         nsGUIEvent* aEvent,
                         nsEventStatus* aEventStatus) { return NS_OK; }
  //
  // XXX: The following paint methods are TEMPORARY. It is being used to get printing working
  // under windows. Later it may be used to GFX-render the controls to the display. 
  // Expect this code to repackaged and moved to a new location in the future.
  //

  NS_IMETHOD Paint(nsIPresContext* aPresContext,
                   nsIRenderingContext& aRenderingContext,
                   const nsRect& aDirtyRect,
                   nsFramePaintLayer aWhichLayer);

  virtual void PaintRadioButton(nsIPresContext* aPresContext,
                        nsIRenderingContext& aRenderingContext,
                        const nsRect& aDirtyRect);

  virtual PRInt32 GetMaxNumValues() { return 1; }
  
  virtual PRBool GetNamesValues(PRInt32 aMaxNumValues, PRInt32& aNumValues,
                                nsString* aValues, nsString* aNames);
  virtual void Reset(nsIPresContext* aPresContext);

       // nsIFormControlFrame
  NS_IMETHOD SetProperty(nsIPresContext* aPresContext, nsIAtom* aName, const nsString& aValue);
  NS_IMETHOD GetProperty(nsIAtom* aName, nsString& aValue); 

  //nsIStatefulFrame
  NS_IMETHOD GetStateType(nsIPresContext* aPresContext, nsIStatefulFrame::StateType* aStateType);
  NS_IMETHOD SaveState(nsIPresContext* aPresContext, nsIPresState** aState);
  NS_IMETHOD RestoreState(nsIPresContext* aPresContext, nsIPresState* aState);

  ///XXX: End o the temporary methods
#ifdef DEBUG_rodsXXX
  NS_IMETHOD Reflow(nsIPresContext*          aCX,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);
#endif

protected:

	virtual PRBool	GetRadioState();
	virtual void 		SetRadioState(nsIPresContext* aPresContext, PRBool aValue);

    //GFX-rendered state variables
  PRBool							mChecked;
  nsIStyleContext* 		mRadioButtonFaceStyle;

private:
  NS_IMETHOD_(nsrefcnt) AddRef() { return NS_OK; }
  NS_IMETHOD_(nsrefcnt) Release() { return NS_OK; }
};



#endif


