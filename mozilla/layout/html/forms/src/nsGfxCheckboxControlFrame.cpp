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

#include "nsGfxCheckboxControlFrame.h"
#include "nsICheckButton.h"
#include "nsHTMLAtoms.h"
#include "nsHTMLParts.h"
#include "nsFormFrame.h"
#include "nsIFormControl.h"
#include "nsIContent.h"
#include "nsIComponentManager.h"
#include "nsHTMLAtoms.h"
#include "nsINameSpaceManager.h"
#include "nsIPresState.h"
#include "nsCSSRendering.h"

//------------------------------------------------------------
nsresult
NS_NewGfxCheckboxControlFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame)
{
  NS_PRECONDITION(aNewFrame, "null OUT ptr");
  if (nsnull == aNewFrame) {
    return NS_ERROR_NULL_POINTER;
  }
  nsGfxCheckboxControlFrame* it = new (aPresShell) nsGfxCheckboxControlFrame;
  if (!it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  *aNewFrame = it;
  return NS_OK;
}


//------------------------------------------------------------
// Initialize GFX-rendered state
nsGfxCheckboxControlFrame::nsGfxCheckboxControlFrame()
: mChecked(eOff),
  mCheckButtonFaceStyle(nsnull),
  mInClickEvent(PR_FALSE)
{
}

nsGfxCheckboxControlFrame::~nsGfxCheckboxControlFrame()
{
  NS_IF_RELEASE(mCheckButtonFaceStyle);
}


//----------------------------------------------------------------------
// nsISupports
//----------------------------------------------------------------------
// Frames are not refcounted, no need to AddRef
NS_IMETHODIMP
nsGfxCheckboxControlFrame::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{
  NS_ASSERTION(aInstancePtr, "QueryInterface requires a non-NULL destination!");
  if ( !aInstancePtr )
    return NS_ERROR_NULL_POINTER;
  if (aIID.Equals(NS_GET_IID(nsIStatefulFrame))) {
    *aInstancePtr = (void*)(nsIStatefulFrame*) this;
    return NS_OK;
  }
  if (aIID.Equals(NS_GET_IID(nsICheckboxControlFrame))) {
    *aInstancePtr = (void*) ((nsICheckboxControlFrame*) this);
    return NS_OK;
  }
  return nsFormControlFrame::QueryInterface(aIID, aInstancePtr);
}

//--------------------------------------------------------------
NS_IMETHODIMP
nsGfxCheckboxControlFrame::SetCheckboxFaceStyleContext(nsIStyleContext *aCheckboxFaceStyleContext)
{
  mCheckButtonFaceStyle = aCheckboxFaceStyleContext;
  NS_ADDREF(mCheckButtonFaceStyle);
  return NS_OK;
}

//--------------------------------------------------------------
NS_IMETHODIMP
nsGfxCheckboxControlFrame::SetIsInClickEvent(PRBool aVal)
{
  mInClickEvent = aVal;
  return NS_OK;
}

//------------------------------------------------------------
//
// Init
//
// We need to override this in order to see if we're a tristate checkbox.
//
NS_IMETHODIMP
nsGfxCheckboxControlFrame::Init(nsIPresContext*  aPresContext,
              nsIContent*      aContent,
              nsIFrame*        aParent,
              nsIStyleContext* aContext,
              nsIFrame*        aPrevInFlow)
{
  nsFormControlFrame::Init ( aPresContext, aContent, aParent, aContext, aPrevInFlow );
  
  // figure out if we're a tristate at the start. This may change later on once
  // we've been running for a while, so more code is in AttributeChanged() to pick
  // that up. Regardless, we need this check when initializing.
  nsAutoString value;
  nsresult res = mContent->GetAttribute ( kNameSpaceID_None, GetTristateAtom(), value );
  if ( res == NS_CONTENT_ATTR_HAS_VALUE )
    mIsTristate = PR_TRUE;

  // give the attribute a default value so it's always present, if we're a tristate
  if ( IsTristateCheckbox() )
    mContent->SetAttribute ( kNameSpaceID_None, GetTristateValueAtom(), NS_ConvertASCIItoUCS2("0"), PR_FALSE );
  
  return NS_OK;
}

//--------------------------------------------------------------
NS_IMETHODIMP
nsGfxCheckboxControlFrame::GetAdditionalStyleContext(PRInt32 aIndex, 
                                                     nsIStyleContext** aStyleContext) const
{
  NS_PRECONDITION(nsnull != aStyleContext, "null OUT parameter pointer");
  if (aIndex < 0) {
    return NS_ERROR_INVALID_ARG;
  }
  *aStyleContext = nsnull;
  switch (aIndex) {
  case NS_GFX_CHECKBOX_CONTROL_FRAME_FACE_CONTEXT_INDEX:
    *aStyleContext = mCheckButtonFaceStyle;
    NS_IF_ADDREF(*aStyleContext);
    break;
  default:
    return NS_ERROR_INVALID_ARG;
  }
  return NS_OK;
}



//--------------------------------------------------------------
NS_IMETHODIMP
nsGfxCheckboxControlFrame::SetAdditionalStyleContext(PRInt32 aIndex, 
                                                     nsIStyleContext* aStyleContext)
{
  if (aIndex < 0) {
    return NS_ERROR_INVALID_ARG;
  }
  switch (aIndex) {
  case NS_GFX_CHECKBOX_CONTROL_FRAME_FACE_CONTEXT_INDEX:
    NS_IF_RELEASE(mCheckButtonFaceStyle);
    mCheckButtonFaceStyle = aStyleContext;
    NS_IF_ADDREF(aStyleContext);
    break;
  }
  return NS_OK;
}


//------------------------------------------------------------
//
// GetTristateAtom [static]
//
// Use a lazily instantiated static initialization scheme to create an atom that
// represents the attribute set when this should be a tri-state checkbox.
//
// Does NOT addref!
//
nsIAtom*
nsGfxCheckboxControlFrame :: GetTristateAtom ( )
{
  return nsHTMLAtoms::moz_tristate;
}

//------------------------------------------------------------
//
// GetTristateValueAtom [static]
//
// Use a lazily instantiated static initialization scheme to create an atom that
// represents the attribute that holds the value when the button is a tri-state (since
// we can't use "checked").
//
// Does NOT addref!
//
nsIAtom*
nsGfxCheckboxControlFrame :: GetTristateValueAtom ( )
{
  return nsHTMLAtoms::moz_tristatevalue;
}

//------------------------------------------------------------
//
// AttributeChanged
//
// Override to check for the attribute that determines if we're a normal or a 
// tristate checkbox. If we notice a switch from one to the other, we need
// to adjust the proper attributes in the content model accordingly.
//
// Also, since the value of a tri-state is kept in a separate attribute (we
// can't use "checked" because it's a boolean), we have to notice it changing
// here.
//
NS_IMETHODIMP
nsGfxCheckboxControlFrame::AttributeChanged(nsIPresContext* aPresContext,
                                          nsIContent*     aChild,
                                          PRInt32         aNameSpaceID,
                                          nsIAtom*        aAttribute,
                                          PRInt32         aHint)
{
  if ( aAttribute == GetTristateAtom() ) {    
    nsAutoString value;
    nsresult res = mContent->GetAttribute ( kNameSpaceID_None, GetTristateAtom(), value );
    PRBool isNowTristate = (res == NS_CONTENT_ATTR_HAS_VALUE);
    if ( isNowTristate != mIsTristate )
      SwitchModesWithEmergencyBrake(aPresContext, isNowTristate);
  }
  else if ( aAttribute == GetTristateValueAtom() ) {
    // ignore this change if we're not a tri-state checkbox
    if ( IsTristateCheckbox() ) {      
      nsAutoString value;
      nsresult res = mContent->GetAttribute ( kNameSpaceID_None, GetTristateValueAtom(), value );
      if ( res == NS_CONTENT_ATTR_HAS_VALUE )
        SetCheckboxControlFrameState(aPresContext, value);
    }
  }
  else
    return nsFormControlFrame::AttributeChanged(aPresContext, aChild, aNameSpaceID, aAttribute, aHint);

  return NS_OK;
}

//------------------------------------------------------------
//
// InitializeControl
//
// Set the default checked state of the checkbox.
// 
void 
nsGfxCheckboxControlFrame::InitializeControl(nsIPresContext* aPresContext)
{
  nsFormControlFrame::InitializeControl(aPresContext);

  PRBool checked = PR_FALSE;
  nsresult result = GetDefaultCheckState(&checked);
  if (NS_CONTENT_ATTR_HAS_VALUE == result) {
    SetCheckboxState (aPresContext, checked ? eOn : eOff );
  }
}

//------------------------------------------------------------
void
nsGfxCheckboxControlFrame::PaintCheckBox(nsIPresContext* aPresContext,
                                         nsIRenderingContext& aRenderingContext,
                                         const nsRect& aDirtyRect,
                                         nsFramePaintLayer aWhichLayer)
{
  aRenderingContext.PushState();

  float p2t;
  aPresContext->GetScaledPixelsToTwips(&p2t);

  const nsStyleSpacing* spacing =
    (const nsStyleSpacing*)mStyleContext->GetStyleData(eStyleStruct_Spacing);

  nsRect checkRect(0,0, mRect.width, mRect.height);

  nsMargin borderPadding;
  spacing->CalcBorderPaddingFor(this, borderPadding);
  checkRect.Deflate(borderPadding);

  const nsStyleColor* color = (const nsStyleColor*)
                                  mStyleContext->GetStyleData(eStyleStruct_Color);
  aRenderingContext.SetColor(color->mColor);

  if ( IsTristateCheckbox() ) {
    // Get current checked state through content model.
    // XXX this won't work under printing. does that matter???
    CheckState checked = GetCheckboxState();
    switch ( checked ) {   
      case eOn:
        nsFormControlHelper::PaintCheckMark(aRenderingContext, p2t, checkRect);
        break;

      case eMixed:
        PaintMixedMark(aRenderingContext, p2t, checkRect);
        break;

			  // no special drawing otherwise
			default:
				break;
    } // case of value of checkbox
  } else {
    // Get current checked state through content model.
    // XXX: This is very inefficient, but it is necessary in the case of printing.
    // During printing the Paint is called but the actual state of the checkbox
    // is in a frame in presentation shell 0.
    PRBool checked = PR_FALSE;
    GetCurrentCheckState(&checked);
    if ( checked ) {
      nsFormControlHelper::PaintCheckMark(aRenderingContext, p2t, checkRect);
    }
  }
  
  PRBool clip;
  aRenderingContext.PopState(clip);
}


//------------------------------------------------------------
//
// PaintMixedMark
//
// Like nsFormControlHelper::PaintCheckMark(), but paints the horizontal "mixed"
// bar inside the box. Only used for tri-state checkboxes.
//
void
nsGfxCheckboxControlFrame::PaintMixedMark ( nsIRenderingContext& aRenderingContext,
                                             float aPixelsToTwips, const nsRect& aRect)
{
  const PRUint32 checkpoints = 4;
  const PRUint32 checksize   = 6; //This is value is determined by added 2 units to the end
                                //of the 7X& pixel rectangle below to provide some white space
                                //around the checkmark when it is rendered.

  // Points come from the coordinates on a 7X7 pixels 
  // box with 0,0 at the lower left. 
  nscoord checkedPolygonDef[] = { 1,2,  5,2,  5,4, 1,4 };
  // Location of the center point of the checkmark
  const PRUint32 centerx = 3;
  const PRUint32 centery = 3;
  
  nsPoint checkedPolygon[checkpoints];
  PRUint32 defIndex = 0;
  PRUint32 polyIndex = 0;

  // Scale the checkmark based on the smallest dimension
  PRUint32 size = aRect.width / checksize;
  if (aRect.height < aRect.width)
   size = aRect.height / checksize;
  
  // Center and offset each point in the polygon definition.
  for (defIndex = 0; defIndex < (checkpoints * 2); defIndex++) {
    checkedPolygon[polyIndex].x = nscoord((((checkedPolygonDef[defIndex]) - centerx) * (size)) + (aRect.width / 2) + aRect.x);
    defIndex++;
    checkedPolygon[polyIndex].y = nscoord((((checkedPolygonDef[defIndex]) - centery) * (size)) + (aRect.height / 2) + aRect.y);
    polyIndex++;
  }
  
  aRenderingContext.FillPolygon(checkedPolygon, checkpoints);

} // PaintMixedMark


//------------------------------------------------------------
NS_METHOD 
nsGfxCheckboxControlFrame::Paint(nsIPresContext* aPresContext,
                              nsIRenderingContext& aRenderingContext,
                              const nsRect& aDirtyRect,
                              nsFramePaintLayer aWhichLayer)
{
  const nsStyleDisplay* disp = (const nsStyleDisplay*)
  mStyleContext->GetStyleData(eStyleStruct_Display);
  if (!disp->IsVisible())
    return NS_OK;

  // Paint the background
  nsFormControlFrame::Paint(aPresContext, aRenderingContext, aDirtyRect, aWhichLayer);
  if (NS_FRAME_PAINT_LAYER_FOREGROUND == aWhichLayer) {
    PRBool doDefaultPainting = PR_TRUE;
    // Paint the checkmark 
    if (nsnull != mCheckButtonFaceStyle  && GetCheckboxState() == eOn) {
      const nsStyleColor* myColor = (const nsStyleColor*)
          mCheckButtonFaceStyle->GetStyleData(eStyleStruct_Color);

      if (myColor->mBackgroundImage.Length() > 0) {
        const nsStyleSpacing* mySpacing = (const nsStyleSpacing*)
            mCheckButtonFaceStyle->GetStyleData(eStyleStruct_Spacing);
        const nsStylePosition* myPosition = (const nsStylePosition*)
            mCheckButtonFaceStyle->GetStyleData(eStyleStruct_Position);

        nscoord width = myPosition->mWidth.GetCoordValue();
        nscoord height = myPosition->mHeight.GetCoordValue();
         // Position the button centered within the radio control's rectangle.
        nscoord x = (mRect.width - width) / 2;
        nscoord y = (mRect.height - height) / 2;
        nsRect rect(x, y, width, height); 

        nsCSSRendering::PaintBackground(aPresContext, aRenderingContext, this,
                                          aDirtyRect, rect, *myColor, *mySpacing, 0, 0);
        nsCSSRendering::PaintBorder(aPresContext, aRenderingContext, this,
                                    aDirtyRect, rect, *mySpacing, mCheckButtonFaceStyle, 0);
        doDefaultPainting = PR_FALSE;
      }
    } 

    if (doDefaultPainting) {
      PaintCheckBox(aPresContext, aRenderingContext, aDirtyRect, aWhichLayer);
    }
  }
  
  // Call to the base class to draw selection borders when appropriate
  return nsFrame::Paint(aPresContext, aRenderingContext, aDirtyRect, aWhichLayer);
}

//------------------------------------------------------------
nsGfxCheckboxControlFrame::CheckState 
nsGfxCheckboxControlFrame::GetCheckboxState ( )
{
  // If we are processing an onclick event then
  // always return the opposite value
  // additional explanantion is in nsICheckboxControlFrame or nsHTMLInputElement.cpp
  if (mInClickEvent) {
    if (!IsTristateCheckbox()) {
      return mChecked == eOn? eOff : eOn;
    } else {
      switch (mChecked) {
        case eOff:   return eOn;
        case eOn:    return eMixed;
        case eMixed: return eOff;
      default:
        break;
      }
    }
  }
  return mChecked;
}

//------------------------------------------------------------
void 
nsGfxCheckboxControlFrame::SetCheckboxState (nsIPresContext* aPresContext,
                                               nsGfxCheckboxControlFrame::CheckState aValue )
{
  mChecked = aValue;
  nsFormControlHelper::ForceDrawFrame(aPresContext, this);
}

//------------------------------------------------------------
void nsGfxCheckboxControlFrame::GetCheckboxControlFrameState(nsString& aValue)
{
  CheckStateToString(GetCheckboxState(), aValue);
}       


//------------------------------------------------------------
void nsGfxCheckboxControlFrame::SetCheckboxControlFrameState(nsIPresContext* aPresContext,
                                                          const nsString& aValue)
{
  CheckState state = StringToCheckState(aValue);
  SetCheckboxState(aPresContext, state);
}         

//------------------------------------------------------------
NS_IMETHODIMP nsGfxCheckboxControlFrame::SetProperty(nsIPresContext* aPresContext,
                                                  nsIAtom* aName,
                                                  const nsString& aValue)
{
  if (nsHTMLAtoms::checked == aName)
    SetCheckboxControlFrameState(aPresContext, aValue);
  else
    return nsFormControlFrame::SetProperty(aPresContext, aName, aValue);

  return NS_OK;     
}


//------------------------------------------------------------
NS_IMETHODIMP nsGfxCheckboxControlFrame::GetProperty(nsIAtom* aName, nsString& aValue)
{
  if (nsHTMLAtoms::checked == aName)
    GetCheckboxControlFrameState(aValue);
  else
    return nsFormControlFrame::GetProperty(aName, aValue);

  return NS_OK;     
}

//------------------------------------------------------------
PRInt32 
nsGfxCheckboxControlFrame::GetMaxNumValues()
{
  return 1;
}
  
//------------------------------------------------------------
PRBool
nsGfxCheckboxControlFrame::GetNamesValues(PRInt32 aMaxNumValues, PRInt32& aNumValues,
                                       nsString* aValues, nsString* aNames)
{
  nsAutoString name;
  nsresult nameResult = GetName(&name);
  if ((aMaxNumValues <= 0) || (NS_CONTENT_ATTR_HAS_VALUE != nameResult)) {
    return PR_FALSE;
  }

  PRBool result = PR_TRUE;
  CheckState state = GetCheckboxState();

  nsAutoString value;
  nsresult valueResult = GetValue(&value);
   
   if (eOn != state) {
      result = PR_FALSE;
   } else {
     if (NS_CONTENT_ATTR_HAS_VALUE != valueResult) {
       aValues[0].AssignWithConversion("on");
     } else {
       aValues[0] = value;
     }
     aNames[0] = name;
     aNumValues = 1;
  }
 
  return result;
}

//------------------------------------------------------------
void 
nsGfxCheckboxControlFrame::Reset(nsIPresContext* aPresContext) 
{
  PRBool checked;
  GetDefaultCheckState(&checked);
  SetCheckboxState (aPresContext, checked ? eOn : eOff );
}  


//------------------------------------------------------------
//
// CheckStateToString
//
// Converts from a CheckState to a string
//
void
nsGfxCheckboxControlFrame::CheckStateToString ( CheckState inState, nsString& outStateAsString )
{
  switch ( inState ) {
    case eOn:
      outStateAsString.Assign(NS_STRING_TRUE);
	  break;

    case eOff:
      outStateAsString.Assign(NS_STRING_FALSE);
      break;
 
    case eMixed:
      outStateAsString.AssignWithConversion("2");
      break;
  }
} // CheckStateToString


//------------------------------------------------------------
//
// StringToCheckState
//
// Converts from a string to a CheckState enum
//
nsGfxCheckboxControlFrame::CheckState 
nsGfxCheckboxControlFrame::StringToCheckState ( const nsString & aStateAsString )
{
  if ( aStateAsString.EqualsWithConversion(NS_STRING_TRUE) )
    return eOn;
  else if ( aStateAsString.EqualsWithConversion(NS_STRING_FALSE) )
    return eOff;

  // not true and not false means mixed
  return eMixed;
  
} // StringToCheckState


//------------------------------------------------------------
//
// SwitchModesWithEmergencyBrake
//
// Since we use an attribute to decide if we're a tristate box or not, this can change
// at any time. Since we have to use separate attributes to store the values depending
// on the mode, we have to convert from one to the other.
//
void
nsGfxCheckboxControlFrame::SwitchModesWithEmergencyBrake ( nsIPresContext* aPresContext,
                                                          PRBool inIsNowTristate )
{
  if ( inIsNowTristate ) {
    // we were a normal checkbox, and now we're a tristate. That means that the
    // state of the checkbox was in "checked" and needs to be copied over into
    // our parallel attribute.
    nsAutoString value;
    CheckStateToString ( GetCheckboxState(), value );
    mContent->SetAttribute ( kNameSpaceID_None, GetTristateValueAtom(), value, PR_FALSE );
  }
  else {
    // we were a tri-state checkbox, and now we're a normal checkbox. The current
    // state is already up to date (because it's always up to date). We just have
    // to make sure it's not mixed. If it is, just set it to checked. Remove our
    // parallel attribute so that we're nice and HTML4 compliant.
    if ( GetCheckboxState() == eMixed )
      SetCheckboxState(aPresContext, eOn);
    mContent->UnsetAttribute ( kNameSpaceID_None, GetTristateValueAtom(), PR_FALSE );
  }

  // switch!
  mIsTristate = inIsNowTristate;
  
} // SwitchModesWithEmergencyBrake

//----------------------------------------------------------------------
// nsIStatefulFrame
//----------------------------------------------------------------------
NS_IMETHODIMP nsGfxCheckboxControlFrame::GetStateType(nsIPresContext* aPresContext,
                                                   nsIStatefulFrame::StateType* aStateType)
{
  *aStateType=nsIStatefulFrame::eCheckboxType;
  return NS_OK;
}

NS_IMETHODIMP nsGfxCheckboxControlFrame::SaveState(nsIPresContext* aPresContext,
                                                nsIPresState** aState)
{
  // Construct a pres state.
  NS_NewPresState(aState); // The addref happens here.
  
  // This string will hold a single item, whether or not we're checked.
  nsAutoString stateString;
  GetCheckboxControlFrameState(stateString);
  (*aState)->SetStateProperty(NS_ConvertASCIItoUCS2("checked"), stateString);

  return NS_OK;
}

NS_IMETHODIMP nsGfxCheckboxControlFrame::RestoreState(nsIPresContext* aPresContext,
                                                   nsIPresState* aState)
{
  if (!mDidInit) {
    mPresContext = aPresContext;
    InitializeControl(aPresContext);
    mDidInit = PR_TRUE;
  }
  nsAutoString string;
  aState->GetStateProperty(NS_ConvertASCIItoUCS2("checked"), string);
  SetCheckboxControlFrameState(aPresContext, string);
  return NS_OK;
}

//------------------------------------------------------------
// Extra Debug Methods
//------------------------------------------------------------
#ifdef DEBUG_rodsXXX
NS_IMETHODIMP 
nsGfxCheckboxControlFrame::Reflow(nsIPresContext*          aPresContext, 
                                  nsHTMLReflowMetrics&     aDesiredSize,
                                  const nsHTMLReflowState& aReflowState, 
                                  nsReflowStatus&          aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsGfxCheckboxControlFrame", aReflowState.reason);
  nsresult rv = nsFormControlFrame::Reflow(aPresContext, aDesiredSize, aReflowState, aStatus);

  COMPARE_QUIRK_SIZE("nsGfxCheckboxControlFrame", 13, 13) 
  return rv;
}
#endif
