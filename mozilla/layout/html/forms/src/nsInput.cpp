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
#include "nsInput.h"
#include "nsIFormManager.h"
#include "nsInputFrame.h"
#include "nsHTMLParts.h"
#include "nsHTMLContainer.h"
#include "nsIRenderingContext.h"
#include "nsIPresShell.h"
#include "nsIPresContext.h"
#include "nsIStyleContext.h"
#include "nsLeafFrame.h"
#include "nsCSSRendering.h"
#include "nsHTMLIIDs.h"
#include "nsHTMLAtoms.h"
#include "nsIView.h"
#include "nsIViewManager.h"
#include "nsCoord.h"
#include "nsDebug.h"
#include "nsIWidget.h"
#include "nsHTMLForms.h"
#include "nsStyleConsts.h"

#define ALIGN_UNSET PRUint8(-1)

static NS_DEFINE_IID(kSupportsIID, NS_ISUPPORTS_IID);

// Note: we inherit a base class operator new that zeros our memory
nsInput::nsInput(nsIAtom* aTag, nsIFormManager* aManager)
  : nsHTMLContainer(aTag), mControl()
{
  mFormMan = aManager;
  if (nsnull != mFormMan) {
    NS_ADDREF(mFormMan);
    mFormMan->AddFormControl(&mControl);
  }
  mSize  = ATTR_NOTSET; 
  mAlign = ALIGN_UNSET;
  mLastClickPoint.x = -1;
  mLastClickPoint.y = -1;
  mCanSubmit = PR_FALSE;
}

nsInput::~nsInput()
{
  NS_IF_RELEASE(mWidget);
  NS_IF_RELEASE(mWidgetSupports);
  if (nsnull != mName) {
    delete mName;
  }
  if (nsnull != mValue) {
    delete mName;
  }
  if (nsnull != mFormMan) {
    // prevent mFormMan from decrementing its ref count on us
    mFormMan->RemoveFormControl(&mControl, PR_FALSE); 
    NS_RELEASE(mFormMan);
  }
}

void nsInput::SetClickPoint(nscoord aX, nscoord aY)
{
  mLastClickPoint.x = aX;
  mLastClickPoint.y = aY;
}

PRBool nsInput::GetCanSubmit() const
{
  return mCanSubmit;
}

void nsInput::SetCanSubmit(PRBool aFlag)
{
  mCanSubmit = aFlag;
}

void nsInput::SetContent(const nsString& aValue)
{
  if (nsnull == mValue) {
    mValue = new nsString(aValue);
  } else {
    *mValue = aValue;
  }
}

void nsInput::MapAttributesInto(nsIStyleContext* aContext, 
                                nsIPresContext* aPresContext)
{
  if (ALIGN_UNSET != mAlign) {
    nsStyleDisplay* display = (nsStyleDisplay*)
      aContext->GetMutableStyleData(eStyleStruct_Display);
    nsStyleText* text = (nsStyleText*)
      aContext->GetMutableStyleData(eStyleStruct_Text);
    switch (mAlign) {
    case NS_STYLE_TEXT_ALIGN_LEFT:
      display->mFloats = NS_STYLE_FLOAT_LEFT;
      break;
    case NS_STYLE_TEXT_ALIGN_RIGHT:
      display->mFloats = NS_STYLE_FLOAT_RIGHT;
      break;
    default:
      text->mVerticalAlign.SetIntValue(mAlign, eStyleUnit_Enumerated);
      break;
    }
  }
  MapImagePropertiesInto(aContext, aPresContext);
}


static NS_DEFINE_IID(kIFormControlIID, NS_IFORMCONTROL_IID);

nsresult nsInput::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  if (aIID.Equals(kIFormControlIID)) {
    AddRef();
    *aInstancePtr = (void**) &mControl;
    return NS_OK;
  }
  return nsHTMLContainer::QueryInterface(aIID, aInstancePtr);
}

PRBool nsInput::IsSuccessful(nsIFormControl* aSubmitter) const
{
  if (nsnull == mName) {
    return PR_FALSE;
  }
  else {
    return PR_TRUE;
  }
}

nsrefcnt nsInput::Release()
{
  --mRefCnt;
	if (mRefCnt == 0) {
    delete this;                                       
    return 0;                                          
  }
  if ((mFormMan == nsnull) || (mRefCnt > 1)) {
    return mRefCnt;
  }

 	int numSiblings = mFormMan->GetFormControlCount();
  PRBool externalRefs = PR_FALSE;  // are there external refs to dad or any siblings
  if ((int)mFormMan->GetRefCount() > numSiblings) {
    externalRefs = PR_TRUE;
  } else {
    for (int i = 0; i < numSiblings; i++) {
      nsIFormControl* sibling = mFormMan->GetFormControlAt(i);
      if (sibling->GetRefCount() > 1) {
        externalRefs = PR_TRUE;
        break;
      }
    }
  }

  if (!externalRefs) {
    mRefCnt = 0;
    delete this;                                       
    return 0;                                          
  } 
  
  return mRefCnt;
}
  
PRBool 
nsInput::IsHidden()
{
  return PR_FALSE;
}

void 
nsInput::SetWidget(nsIWidget* aWidget)
{
  if (aWidget != mWidget) {
	  NS_IF_RELEASE(mWidget);
    NS_IF_ADDREF(aWidget);
    mWidget = aWidget;

    NS_IF_RELEASE(mWidgetSupports);
    mWidget->QueryInterface(kSupportsIID, (void**)&mWidgetSupports);
  }
}

nsrefcnt nsInput::GetRefCount() const 
{
  return mRefCnt;
}

// these do not AddRef
nsIWidget* nsInput::GetWidget()
{
  return mWidget;
}
nsISupports* nsInput::GetWidgetSupports()
{
  return mWidgetSupports;
}

PRBool nsInput::GetContent(nsString& aResult) const
{
  if (nsnull == mValue) {
    aResult.SetLength(0);
    return PR_FALSE;
  } else {
    aResult = *mValue;
    return PR_TRUE;
  }
}

void 
nsInput::SetFormManager(nsIFormManager* aFormMan, PRBool aDecrementRef)
{
	if (aDecrementRef) {
    NS_IF_RELEASE(mFormMan);
	}
  mFormMan = aFormMan;
  NS_IF_ADDREF(aFormMan);
}

nsIFormManager* 
nsInput::GetFormManager() const
{
  NS_IF_ADDREF(mFormMan);
  return mFormMan;
}

/**
 * Get the name associated with this form element. If there is no name
 * then return PR_FALSE (form elements without names are not submitable).
 */
PRBool
nsInput::GetName(nsString& aName) const
{
  if ((nsnull != mName) && (0 != mName->Length())) {
    aName = *mName;
    return PR_TRUE;
  }
  return PR_FALSE;
}

void
nsInput::Reset()
{
}

PRInt32
nsInput::GetMaxNumValues()
{
  return 0;
}

PRBool
nsInput::GetNamesValues(PRInt32 aMaxNumValues, PRInt32& aNumValues, 
                        nsString* aValues, nsString* aNames)
{
  aNumValues = 0;
  return PR_FALSE;
}

void nsInput::CacheAttribute(const nsString& aValue, nsString*& aLoc)
{
  if (nsnull == aLoc) {
    aLoc = new nsString(aValue);
  } else {
    aLoc->SetLength(0);
    aLoc->Append(aValue);
  }
}

void nsInput::CacheAttribute(const nsString& aValue, PRInt32 aMinValue, PRInt32& aLoc)
{
  PRInt32 status;
  PRInt32 intVal = aValue.ToInteger(&status);
  aLoc = ((NS_OK == status) && (intVal >= aMinValue)) ? intVal : aMinValue;
}

void nsInput::SetAttribute(nsIAtom* aAttribute, const nsString& aValue)
{
  if (aAttribute == nsHTMLAtoms::type) { // You cannot set the type of a form element
    ;
  } 
  else if (aAttribute == nsHTMLAtoms::name) {
    CacheAttribute(aValue, mName);
  } 
  else if (aAttribute == nsHTMLAtoms::size) {
    CacheAttribute(aValue, ATTR_NOTSET, mSize);
  } 
  else if (aAttribute == nsHTMLAtoms::value) {
    CacheAttribute(aValue, mValue);
  } 
  else if (aAttribute == nsHTMLAtoms::align) {
    nsHTMLValue val;
    if (ParseAlignParam(aValue, val)) {
      mAlign = val.GetIntValue();
      // Reflect the attribute into the syle system
      nsHTMLTagContent::SetAttribute(aAttribute, val);  // is this needed?
    } else {
      mAlign = ALIGN_UNSET;
    }
    return;
  }
  // XXX the following is necessary so that MapAttributesInto gets called
  nsInputSuper::SetAttribute(aAttribute, aValue); 
}

nsContentAttr nsInput::GetCacheAttribute(nsString* const& aLoc, nsHTMLValue& aValue) const
{
  aValue.Reset();
  if (nsnull == aLoc) {
    return eContentAttr_NotThere;
  } 
  else {
    aValue.SetStringValue(*aLoc);
    return eContentAttr_HasValue;
  }
}

nsContentAttr nsInput::GetCacheAttribute(PRInt32 aLoc, nsHTMLValue& aValue, nsHTMLUnit aUnit) const
{
  aValue.Reset();
  if (aLoc <= ATTR_NOTSET) {
    return eContentAttr_NotThere;
  } 
  else {
    if (eHTMLUnit_Pixel == aUnit) {
      aValue.SetPixelValue(aLoc);
    }
    else if (eHTMLUnit_Empty == aUnit) {
      if (PRBool(aLoc)) {
        aValue.SetEmptyValue();
      }
      else {
        return eContentAttr_NotThere;
      }
    }
    else {
      aValue.SetIntValue(aLoc, aUnit);
    }

    return eContentAttr_HasValue;
  }
}

nsContentAttr nsInput::GetAttribute(nsIAtom* aAttribute, PRInt32& aValue) const
{
  nsHTMLValue htmlValue;
  nsContentAttr result = GetAttribute(aAttribute, htmlValue);
  if (eContentAttr_HasValue == result) {
    if (eHTMLUnit_Empty == htmlValue.GetUnit()) {
      aValue = 1;
    }
    else if (eHTMLUnit_Pixel == htmlValue.GetUnit()) {
      aValue = htmlValue.GetPixelValue();
    }
    else {
      aValue = htmlValue.GetIntValue();
    }
    return eContentAttr_HasValue;
  }
  else {
    aValue = ATTR_NOTSET;
    // XXX for bool values, this should return 0
    return eContentAttr_NoValue;
  }
}

nsContentAttr nsInput::GetAttribute(nsIAtom* aAttribute,
                                    nsHTMLValue& aValue) const
{
  if (aAttribute == nsHTMLAtoms::type) {
    nsAutoString tmp;
    GetType(tmp);
    if (tmp.Length() == 0) {    // derivatives that don't support type return zero length string
      return eContentAttr_NotThere;
    }
    else {
      aValue.SetStringValue(tmp);
      return eContentAttr_HasValue;
    }
  } 
  else if (aAttribute == nsHTMLAtoms::name) {
    return GetCacheAttribute(mName, aValue);
  } 
  else if (aAttribute == nsHTMLAtoms::size) {
    return GetCacheAttribute(mSize, aValue, eHTMLUnit_Pixel); // XXX pixel or percent??
  }
  else if (aAttribute == nsHTMLAtoms::value) {
    return GetCacheAttribute(mValue, aValue);
  }
  else if (aAttribute == nsHTMLAtoms::align) {
    return GetCacheAttribute(mAlign, aValue, eHTMLUnit_Enumerated);
  }
  else {
    return nsInputSuper::GetAttribute(aAttribute, aValue);
  }
}

PRBool nsInput::GetChecked(PRBool aGetInitialValue) const
{
  return PR_FALSE;
}

void nsInput::SetChecked(PRBool aState, PRBool aSetInitialValue)
{
}


//----------------------------------------------------------------------

#define GET_OUTER() ((nsInput*) ((char*)this - nsInput::GetOuterOffset()))

nsInput::AggInputControl::AggInputControl()
{
}

nsInput::AggInputControl::~AggInputControl()
{
}

nsrefcnt nsInput::AggInputControl::AddRef()
{
  return GET_OUTER()->AddRef();
}

nsrefcnt nsInput::AggInputControl::Release()
{
  return GET_OUTER()->Release();
}

nsresult nsInput::AggInputControl::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  return GET_OUTER()->QueryInterface(aIID, aInstancePtr);
}

PRBool nsInput::AggInputControl::GetName(nsString& aName) const
{
  return GET_OUTER()->GetName(aName);
}

PRInt32 nsInput::AggInputControl::GetMaxNumValues()
{
  return GET_OUTER()->GetMaxNumValues();
}

PRBool nsInput::AggInputControl::GetNamesValues(PRInt32 aMaxNumValues, PRInt32& aNumValues,
                                                nsString* aValues, nsString* aNames)
{
  return GET_OUTER()->GetNamesValues(aMaxNumValues, aNumValues, aValues, aNames);
}

void nsInput::AggInputControl::Reset()
{
  GET_OUTER()->Reset();
}

void nsInput::AggInputControl::SetFormManager(nsIFormManager* aFormMan, PRBool aDecrementRef)
{
  GET_OUTER()->SetFormManager(aFormMan, aDecrementRef);
}

nsIFormManager* nsInput::AggInputControl::GetFormManager() const
{
  return GET_OUTER()->GetFormManager();
}

nsrefcnt nsInput::AggInputControl::GetRefCount() const
{
  return GET_OUTER()->GetRefCount();
}

PRBool nsInput::AggInputControl::GetChecked(PRBool aGetInitialValue) const
{
  return GET_OUTER()->GetChecked(aGetInitialValue);
}

void nsInput::AggInputControl::SetChecked(PRBool aState, PRBool aSetInitialValue)
{
  GET_OUTER()->SetChecked(aState, aSetInitialValue);
}

void nsInput::AggInputControl::GetType(nsString& aName) const
{
  GET_OUTER()->GetType(aName);
}

PRBool nsInput::AggInputControl::IsSuccessful(nsIFormControl* aSubmitter) const
{
  return GET_OUTER()->IsSuccessful(aSubmitter);
}

void nsInput::AggInputControl::SetContent(const nsString& aValue)
{
  GET_OUTER()->SetContent(aValue);
}

PRBool nsInput::AggInputControl::GetContent(nsString& aResult) const
{
  return GET_OUTER()->GetContent(aResult);
}

void nsInput::AggInputControl::SetCanSubmit(PRBool aFlag)
{
  GET_OUTER()->SetCanSubmit(aFlag);
}

PRBool nsInput::AggInputControl::GetCanSubmit() const
{
  return GET_OUTER()->GetCanSubmit();
}
