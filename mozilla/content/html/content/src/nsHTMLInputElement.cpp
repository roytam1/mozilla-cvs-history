/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Pierre Phaneuf <pp@ludusdesign.com>
 *
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
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */
#include "nsCOMPtr.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsIDOMNSHTMLInputElement.h"
#include "nsITextControlElement.h"
#include "nsIRadioControlElement.h"
#include "nsIRadioVisitor.h"

#include "nsIControllers.h"
#include "nsIEditorController.h"
#include "nsIFocusController.h"
#include "nsPIDOMWindow.h"
#include "nsIScriptGlobalObject.h"
#include "nsContentCID.h"
#include "nsIComponentManager.h"
#include "nsIDOMHTMLFormElement.h"
#include "nsIDOMEventReceiver.h"
#include "nsIHTMLContent.h"
#include "nsGenericHTMLElement.h"
#include "nsHTMLAtoms.h"
#include "nsHTMLIIDs.h"
#include "nsIStyleContext.h"
#include "nsStyleConsts.h"
#include "nsIPresContext.h"
#include "nsIHTMLAttributes.h"
#include "nsIFormControl.h"
#include "nsIForm.h"
#include "nsIFormSubmission.h"
#include "nsIGfxTextControlFrame.h"
#include "nsIRadioControlFrame.h"
#include "nsIDocument.h"
#include "nsIPresShell.h"
#include "nsIFormControlFrame.h"
#include "nsIFrame.h"
#include "nsIEventStateManager.h"
#include "nsISizeOfHandler.h"
#include "nsIServiceManager.h"
#include "nsIScriptSecurityManager.h"
#include "nsDOMError.h"
#include "nsIPrivateDOMEvent.h"
#include "nsIEditor.h"
#include "nsGUIEvent.h"

#include "nsIPresState.h"
#include "nsIDOMEvent.h"
#include "nsIDOMNSEvent.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMHTMLCollection.h"
#include "nsICheckboxControlFrame.h"
#include "nsIFormManager.h"
#include "nsIImageControlFrame.h"
#include "nsLinebreakConverter.h" //to strip out carriage returns
#include "nsReadableUtils.h"

#include "nsIDOMMutationEvent.h"
#include "nsIDOMEventReceiver.h"
#include "nsMutationEvent.h"

#include "nsRuleNode.h"

// input type=file
#include "nsIMIMEService.h"
#include "nsCExternalHandlerService.h"
#include "nsIFile.h"
#include "nsILocalFile.h"
#include "nsIFileStreams.h"


// XXX align=left, hspace, vspace, border? other nav4 attrs

static NS_DEFINE_CID(kXULControllersCID,  NS_XULCONTROLLERS_CID);

typedef nsIGfxTextControlFrame2 textControlPlace;

//
// Accessors for mBitField
//
#define BF_SKIP_FOCUS_EVENT 0
#define BF_HANDLING_CLICK 1
#define BF_VALUE_CHANGED 2
#define BF_CHECKED_CHANGED 3
#define BF_CHECKED 4
#define BF_HANDLING_SELECT_EVENT 5

#define GET_BOOLBIT(bitfield, field) (((bitfield) & (0x01 << (field))) \
                                        ? PR_TRUE : PR_FALSE)
#define SET_BOOLBIT(bitfield, field, b) ((b) \
                                        ? ((bitfield) |=  (0x01 << (field))) \
                                        : ((bitfield) &= ~(0x01 << (field))))

class nsHTMLInputElement : public nsGenericHTMLLeafFormElement,
                           public nsIDOMHTMLInputElement,
                           public nsIDOMNSHTMLInputElement,
                           public nsITextControlElement,
                           public nsIRadioControlElement
{
public:
  nsHTMLInputElement();
  virtual ~nsHTMLInputElement();

  // nsISupports
  NS_DECL_ISUPPORTS_INHERITED

  // nsIDOMNode
  NS_FORWARD_NSIDOMNODE_NO_CLONENODE(nsGenericHTMLLeafFormElement::)

  // nsIDOMElement
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLLeafFormElement::)

  // nsIDOMHTMLElement
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLLeafFormElement::)

  // nsIDOMHTMLInputElement
  NS_DECL_NSIDOMHTMLINPUTELEMENT

  // nsIDOMNSHTMLInputElement
  NS_DECL_NSIDOMNSHTMLINPUTELEMENT

  // Overriden nsIFormControl methods
  NS_IMETHOD SetForm(nsIDOMHTMLFormElement* aForm,
                     PRBool aRemoveFromForm = PR_TRUE);
  NS_IMETHOD GetType(PRInt32* aType);
  NS_IMETHOD Reset();
  NS_IMETHOD SubmitNamesValues(nsIFormSubmission* aFormSubmission,
                               nsIContent* aSubmitElement);
  NS_IMETHOD SaveState(nsIPresContext* aPresContext, nsIPresState** aState);
  NS_IMETHOD RestoreState(nsIPresContext* aPresContext, nsIPresState* aState);

  // nsIContent
  NS_IMETHOD SetFocus(nsIPresContext* aPresContext);
  NS_IMETHOD RemoveFocus(nsIPresContext* aPresContext);

  NS_IMETHOD StringToAttribute(nsIAtom* aAttribute,
                               const nsAReadableString& aValue,
                               nsHTMLValue& aResult);
  NS_IMETHOD AttributeToString(nsIAtom* aAttribute,
                               const nsHTMLValue& aValue,
                               nsAWritableString& aResult) const;
  NS_IMETHOD GetMappedAttributeImpact(const nsIAtom* aAttribute, PRInt32 aModType,
                                      PRInt32& aHint) const;
  NS_IMETHOD GetAttributeMappingFunction(nsMapRuleToAttributesFunc& aMapRuleFunc) const;
  NS_IMETHOD HandleDOMEvent(nsIPresContext* aPresContext, nsEvent* aEvent,
                            nsIDOMEvent** aDOMEvent, PRUint32 aFlags,
                            nsEventStatus* aEventStatus);
#ifdef DEBUG
  NS_IMETHOD SizeOf(nsISizeOfHandler* aSizer, PRUint32* aResult) const;
#endif

  NS_IMETHOD SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                     const nsAReadableString& aValue, PRBool aNotify) {
    BeforeSetAttr(aNameSpaceID, aName, &aValue, aNotify);

    nsresult rv = nsGenericHTMLLeafFormElement::SetAttr(aNameSpaceID, aName,
                                                        aValue, aNotify);

    AfterSetAttr(aNameSpaceID, aName, &aValue, aNotify);
    return rv;
  }
  NS_IMETHOD SetAttr(nsINodeInfo* aNodeInfo, const nsAReadableString& aValue,
                     PRBool aNotify) {
    // This will end up calling the other SetAttr().
    return nsGenericHTMLLeafFormElement::SetAttr(aNodeInfo, aValue, aNotify);
  }

  NS_IMETHOD UnsetAttr(PRInt32 aNameSpaceID, nsIAtom* aAttribute,
                       PRBool aNotify) {
    BeforeSetAttr(aNameSpaceID, aAttribute, nsnull, aNotify);

    nsresult rv = nsGenericHTMLLeafElement::UnsetAttr(aNameSpaceID,
                                                      aAttribute,
                                                      aNotify);

    AfterSetAttr(aNameSpaceID, aAttribute, nsnull, aNotify);
    return rv;
  }

  // nsITextControlElement
  NS_IMETHOD SetValueGuaranteed(const nsAString& aValue, nsIGfxTextControlFrame2* aFrame);
  NS_IMETHOD SetValueChanged(PRBool aValueChanged);

  // nsIRadioControlElement
  NS_IMETHOD RadioSetChecked();
  NS_IMETHOD SetCheckedChanged(PRBool aCheckedChanged);
  NS_IMETHOD SetCheckedChangedInternal(PRBool aCheckedChanged);
  NS_IMETHOD GetCheckedChanged(PRBool* aCheckedChanged);
  NS_IMETHOD AddedToRadioGroup();
  NS_IMETHOD RemovedFromRadioGroup(nsIForm* aForm, nsAString* aName);

protected:
  // Helper method
  void SetPresStateChecked(nsIHTMLContent * aHTMLContent, PRBool aValue);
  NS_IMETHOD SetValueSecure(const nsAReadableString& aValue,
                            nsIGfxTextControlFrame2* aFrame,
                            PRBool aCheckSecurity);

  nsresult GetSelectionRange(PRInt32* aSelectionStart, PRInt32* aSelectionEnd);
  nsresult MouseClickForAltText(nsIPresContext* aPresContext);
  //Helper method
#ifdef ACCESSIBILITY
  nsresult FireEventForAccessibility(nsIPresContext* aPresContext,
			    		                       const nsAReadableString& aEventType);
#endif

  /**
   * Called when an attribute is about to be changed
   */
  void BeforeSetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                     const nsAReadableString* aValue, PRBool aNotify);
  /**
   * Called when an attribute has just been changed
   */
  void AfterSetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                    const nsAReadableString* aValue, PRBool aNotify);

  void SelectAll(nsIPresContext* aPresContext);
  PRBool IsImage() const
  {
    nsAutoString tmp;

    GetAttr(kNameSpaceID_HTML, nsHTMLAtoms::type, tmp);

    return tmp.EqualsIgnoreCase("image");
  }

  void FireOnChange();

  nsresult VisitGroup(nsIRadioVisitor* aVisitor);

  nsresult SetCheckedInternal(PRBool aValue);

  static nsresult GetContentType(const char* aPathName, char** aContentType);

  nsCOMPtr<nsIControllers> mControllers;

  PRInt8                   mType;
  // See GET_BOOLBIT / SET_BOOLBIT macros and BF_* field identifiers
  PRInt8                   mBitField;
  char*                    mValue;
};

//
// Visitor declarations
//

/**
 * This visitor sets CheckedChanged on all elements it finds.
 *
 * @param aCheckedChanged the value of CheckedChanged to set on all elements
 * @param aVisitor the visitor (out param)
 */
NS_METHOD NS_GetRadioSetCheckedChangedVisitor(PRBool aCheckedChanged,
                                              nsIRadioVisitor** aVisitor);

/**
 * This visitor will take the boolean you're pointing at and put
 * aCheckedChanged into it.  If the visitor is never called, aCheckedChanged
 * will of course not change.
 *
 * @param aCheckedChanged the boolean to put CheckedChanged into
 * @param aExcludeElement an element to exclude--i.e. don't get checked changed
 *        on this element
 * @param aVisitor the visitor (out param)
 */
NS_METHOD NS_GetRadioGetCheckedChangedVisitor(PRBool* aCheckedChanged,
                                              nsIFormControl* aExcludeElement,
                                              nsIRadioVisitor** aVisitor);


//
// construction, destruction
//

nsresult
NS_NewHTMLInputElement(nsIHTMLContent** aInstancePtrResult,
                       nsINodeInfo *aNodeInfo)
{
  NS_ENSURE_ARG_POINTER(aInstancePtrResult);

  nsHTMLInputElement* it = new nsHTMLInputElement();

  if (!it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  nsresult rv = NS_STATIC_CAST(nsGenericHTMLElement *, it)->Init(aNodeInfo);

  if (NS_FAILED(rv)) {
    delete it;

    return rv;
  }

  *aInstancePtrResult = NS_STATIC_CAST(nsIHTMLContent *, it);
  NS_ADDREF(*aInstancePtrResult);

  return NS_OK;
}


nsHTMLInputElement::nsHTMLInputElement()
{
  mType = NS_FORM_INPUT_TEXT; // default value
  mBitField = 0;
  mValue = nsnull;
}

nsHTMLInputElement::~nsHTMLInputElement()
{
  // Null out form's pointer to us - no ref counting here!
  SetForm(nsnull);

  if (mValue) {
    nsMemory::Free(mValue);
  }
}


// nsISupports

NS_IMPL_ADDREF_INHERITED(nsHTMLInputElement, nsGenericElement) 
NS_IMPL_RELEASE_INHERITED(nsHTMLInputElement, nsGenericElement) 


// QueryInterface implementation for nsHTMLInputElement
NS_HTML_CONTENT_INTERFACE_MAP_BEGIN(nsHTMLInputElement,
                                    nsGenericHTMLLeafFormElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMHTMLInputElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNSHTMLInputElement)
  NS_INTERFACE_MAP_ENTRY(nsITextControlElement)
  NS_INTERFACE_MAP_ENTRY(nsIRadioControlElement)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(HTMLInputElement)
NS_HTML_CONTENT_INTERFACE_MAP_END


// nsIDOMNode

nsresult
nsHTMLInputElement::CloneNode(PRBool aDeep, nsIDOMNode** aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);
  *aReturn = nsnull;

  nsHTMLInputElement* it = new nsHTMLInputElement();

  if (!it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  nsCOMPtr<nsIDOMNode> kungFuDeathGrip(it);

  nsresult rv = NS_STATIC_CAST(nsGenericHTMLElement *, it)->Init(mNodeInfo);

  if (NS_FAILED(rv))
    return rv;

  CopyInnerTo(this, it, aDeep);

  *aReturn = NS_STATIC_CAST(nsIDOMNode *, it);

  NS_ADDREF(*aReturn);

  return NS_OK;
}


void
nsHTMLInputElement::BeforeSetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                                  const nsAReadableString* aValue,
                                  PRBool aNotify)
{
  if (aName == nsHTMLAtoms::name &&
      mType == NS_FORM_INPUT_RADIO) {
    // XXX Because we are not doing anything that assumes the radio button has
    // actually had its name changed at this point, we can get away with
    // calling this before the name has changed (and gain in performance by not
    // grabbing the old name every time the name is changed).
    RemovedFromRadioGroup(mForm, nsnull);
  }
}

void
nsHTMLInputElement::AfterSetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                                 const nsAReadableString* aValue,
                                 PRBool aNotify)
{
  //
  // When name changes, radio moves to a different group
  // (type changes are handled in the form itself currently)
  //
  if (aName == nsHTMLAtoms::name &&
      mType == NS_FORM_INPUT_RADIO) {
    AddedToRadioGroup();
  }

  //
  // Some elements have to change their value when the value and checked
  // attributes change (but they only do so when ValueChanged() and
  // CheckedChanged() are false--i.e. the value has not been changed by the
  // user or by JS)
  //
  // We only really need to call reset for the value so that the text control
  // knows the new value.  No other reason.
  //
  if (aName == nsHTMLAtoms::value &&
      !GET_BOOLBIT(mBitField, BF_VALUE_CHANGED) &&
      (mType == NS_FORM_INPUT_TEXT ||
       mType == NS_FORM_INPUT_PASSWORD ||
       mType == NS_FORM_INPUT_FILE)) {
    Reset();
  }
  //
  // Checked must be set no matter what type of control it is, since
  // GetChecked() must reflect the new value
  //
  if (aName == nsHTMLAtoms::checked &&
      !GET_BOOLBIT(mBitField, BF_CHECKED_CHANGED)) {
    PRBool resetVal;
    GetDefaultChecked(&resetVal);
    SetChecked(resetVal);
    SetCheckedChanged(PR_FALSE);
  }
}

NS_IMETHODIMP
nsHTMLInputElement::GetForm(nsIDOMHTMLFormElement** aForm)
{
  return nsGenericHTMLLeafFormElement::GetForm(aForm);
}

NS_IMETHODIMP 
nsHTMLInputElement::GetDefaultValue(nsAWritableString& aDefaultValue)
{
  return GetAttr(kNameSpaceID_HTML, nsHTMLAtoms::value, aDefaultValue);
}

NS_IMETHODIMP 
nsHTMLInputElement::SetDefaultValue(const nsAReadableString& aDefaultValue)
{
  return SetAttr(kNameSpaceID_HTML, nsHTMLAtoms::value, aDefaultValue,
                 PR_TRUE); 
}

NS_IMETHODIMP 
nsHTMLInputElement::GetDefaultChecked(PRBool* aDefaultChecked)
{
  nsHTMLValue val;
  nsresult rv;
  rv = GetHTMLAttribute(nsHTMLAtoms::checked, val);       

  *aDefaultChecked = (NS_CONTENT_ATTR_NOT_THERE != rv);                        

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLInputElement::SetDefaultChecked(PRBool aDefaultChecked)
{
  nsresult rv;

  if (aDefaultChecked) {
    rv = SetAttr(kNameSpaceID_HTML, nsHTMLAtoms::checked,
                 NS_LITERAL_STRING(""), PR_TRUE);
  } else {
    rv = UnsetAttr(kNameSpaceID_HTML, nsHTMLAtoms::checked, PR_TRUE);
  }
  
  return rv;
}
  
//NS_IMPL_STRING_ATTR(nsHTMLInputElement, DefaultValue, defaultvalue)
//NS_IMPL_BOOL_ATTR(nsHTMLInputElement, DefaultChecked, defaultchecked)
NS_IMPL_STRING_ATTR(nsHTMLInputElement, Accept, accept)
NS_IMPL_STRING_ATTR(nsHTMLInputElement, AccessKey, accesskey)
NS_IMPL_STRING_ATTR(nsHTMLInputElement, Align, align)
NS_IMPL_STRING_ATTR(nsHTMLInputElement, Alt, alt)
//NS_IMPL_BOOL_ATTR(nsHTMLInputElement, Checked, checked)
NS_IMPL_BOOL_ATTR(nsHTMLInputElement, Disabled, disabled)
NS_IMPL_INT_ATTR(nsHTMLInputElement, MaxLength, maxlength)
NS_IMPL_STRING_ATTR(nsHTMLInputElement, Name, name)
NS_IMPL_BOOL_ATTR(nsHTMLInputElement, ReadOnly, readonly)
NS_IMPL_STRING_ATTR(nsHTMLInputElement, Size, size)
NS_IMPL_STRING_ATTR(nsHTMLInputElement, Src, src)
NS_IMPL_INT_ATTR(nsHTMLInputElement, TabIndex, tabindex)
NS_IMPL_STRING_ATTR(nsHTMLInputElement, UseMap, usemap)
//NS_IMPL_STRING_ATTR(nsHTMLInputElement, Value, value)

NS_IMETHODIMP
nsHTMLInputElement::GetType(nsAWritableString& aValue)
{
  nsresult rv = GetAttr(kNameSpaceID_HTML, nsHTMLAtoms::type, aValue);

  if (rv == NS_CONTENT_ATTR_NOT_THERE)
    aValue.Assign(NS_LITERAL_STRING("text"));

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLInputElement::SetType(const nsAReadableString& aValue)
{
  return nsGenericHTMLLeafFormElement::SetAttr(kNameSpaceID_HTML,
                                               nsHTMLAtoms::type, aValue,
                                               PR_TRUE);
}

NS_IMETHODIMP 
nsHTMLInputElement::GetValue(nsAWritableString& aValue)
{
  PRInt32 type;
  GetType(&type);

  if (type == NS_FORM_INPUT_TEXT || type == NS_FORM_INPUT_PASSWORD ||
      type == NS_FORM_INPUT_FILE) {
    // No need to flush here, if there's no frame created for this
    // input yet, there won't be a value in it (that we don't already
    // have) even if we force it to be created
    nsIFormControlFrame* formControlFrame = GetFormControlFrame(PR_FALSE);

    PRBool frameOwnsValue = PR_FALSE;
    if (formControlFrame) {
      nsIGfxTextControlFrame2* textControlFrame = nsnull;
      CallQueryInterface(formControlFrame, &textControlFrame);

      if (textControlFrame) {
        textControlFrame->OwnsValue(&frameOwnsValue);
      } else {
        // We assume if it's not a text control frame that it owns the value
        frameOwnsValue = PR_TRUE;
      }
    }

    if (frameOwnsValue) {
      formControlFrame->GetProperty(nsHTMLAtoms::value, aValue);
    } else {
      if (!GET_BOOLBIT(mBitField, BF_VALUE_CHANGED) || !mValue) {
        GetDefaultValue(aValue);
      } else {
        aValue = NS_ConvertUTF8toUCS2(mValue);
      }
    }

    return NS_OK;
  }

  // Treat value == defaultValue for other input elements
  nsresult rv = GetAttr(kNameSpaceID_HTML, nsHTMLAtoms::value, aValue);

  if (rv == NS_CONTENT_ATTR_NOT_THERE &&
      (type == NS_FORM_INPUT_RADIO || type == NS_FORM_INPUT_CHECKBOX)) {
    // The default value of a radio or checkbox input is "on".
    aValue.Assign(NS_LITERAL_STRING("on"));

    return NS_OK;
  }

  return rv;
}

NS_IMETHODIMP 
nsHTMLInputElement::SetValue(const nsAReadableString& aValue)
{
  return SetValueSecure(aValue, nsnull, PR_TRUE);
}

NS_IMETHODIMP
nsHTMLInputElement::SetValueGuaranteed(const nsAString& aValue,
                                       nsIGfxTextControlFrame2* aFrame)
{
  return SetValueSecure(aValue, aFrame, PR_FALSE);
}

NS_IMETHODIMP
nsHTMLInputElement::SetValueSecure(const nsAReadableString& aValue,
                                   nsIGfxTextControlFrame2* aFrame,
                                   PRBool aCheckSecurity)
{
  PRInt32 type;
  GetType(&type);
  if (NS_FORM_INPUT_TEXT == type || NS_FORM_INPUT_PASSWORD == type ||
      NS_FORM_INPUT_FILE == type) {
    if (aCheckSecurity && NS_FORM_INPUT_FILE == type) {
      nsresult rv;
      nsCOMPtr<nsIScriptSecurityManager> securityManager = 
               do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv);
      if (NS_FAILED(rv)) {
        return rv;
      }

      PRBool enabled;
      rv = securityManager->IsCapabilityEnabled("UniversalFileRead", &enabled);
      if (NS_FAILED(rv)) {
        return rv;
      }

      if (!enabled) {
        // setting the value of a "FILE" input widget requires the
        // UniversalFileRead privilege
        return NS_ERROR_DOM_SECURITY_ERR;
      }
    }


    nsIGfxTextControlFrame2* textControlFrame = aFrame;
    nsIFormControlFrame* formControlFrame = textControlFrame;
    if (!textControlFrame) {
      // No need to flush here, if there's no frame at this point we
      // don't need to force creation of one just to tell it about this
      // new value.
      formControlFrame = GetFormControlFrame(PR_FALSE);

      if (formControlFrame) {
        CallQueryInterface(formControlFrame, &textControlFrame);
      }
    }

    PRBool frameOwnsValue = PR_FALSE;
    if (textControlFrame) {
      textControlFrame->OwnsValue(&frameOwnsValue);
    }
    if (frameOwnsValue) {
      nsCOMPtr<nsIPresContext> presContext;
      GetPresContext(this, getter_AddRefs(presContext));
      formControlFrame->SetProperty(presContext, nsHTMLAtoms::value, aValue);
    } else {
      if (mValue) {
        nsMemory::Free(mValue);
      }

      mValue = ToNewUTF8String(aValue);

      SetValueChanged(PR_TRUE);
      return mValue ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
    }

    return NS_OK;
  }

  // Treat value == defaultValue for other input elements.
  return nsGenericHTMLLeafFormElement::SetAttr(kNameSpaceID_HTML,
                                               nsHTMLAtoms::value,
                                               aValue, PR_TRUE);
}

NS_IMETHODIMP
nsHTMLInputElement::SetValueChanged(PRBool aValueChanged)
{
  SET_BOOLBIT(mBitField, BF_VALUE_CHANGED, aValueChanged);
  if (!aValueChanged) {
    if (mValue) {
      nsMemory::Free(mValue);
      mValue = nsnull;
    }
  }
  return NS_OK;
}

NS_IMETHODIMP 
nsHTMLInputElement::GetChecked(PRBool* aChecked)
{
  *aChecked = GET_BOOLBIT(mBitField, BF_CHECKED);
  return NS_OK;
}

void
nsHTMLInputElement::SetPresStateChecked(nsIHTMLContent * aHTMLContent, 
                                        PRBool aValue)
{
  nsCOMPtr<nsIPresState> presState;
  GetPrimaryPresState(aHTMLContent, getter_AddRefs(presState));

  // Obtain the value property from the presentation state.
  if (presState) {
    nsAutoString value; value.AssignWithConversion( aValue ? "1" : "0" );
    presState->SetStateProperty(NS_LITERAL_STRING("checked"), value);
  }
}

NS_IMETHODIMP
nsHTMLInputElement::SetCheckedChanged(PRBool aCheckedChanged)
{
  if (mType == NS_FORM_INPUT_RADIO) {
    if (GET_BOOLBIT(mBitField, BF_CHECKED_CHANGED) != aCheckedChanged) {
      nsCOMPtr<nsIRadioVisitor> visitor;
      NS_GetRadioSetCheckedChangedVisitor(aCheckedChanged,
                                          getter_AddRefs(visitor));
      VisitGroup(visitor);
    }
  } else {
    SetCheckedChangedInternal(aCheckedChanged);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLInputElement::SetCheckedChangedInternal(PRBool aCheckedChanged)
{
  SET_BOOLBIT(mBitField, BF_CHECKED_CHANGED, aCheckedChanged);
  return NS_OK;
}


NS_IMETHODIMP
nsHTMLInputElement::GetCheckedChanged(PRBool* aCheckedChanged)
{
  *aCheckedChanged = GET_BOOLBIT(mBitField, BF_CHECKED_CHANGED);
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLInputElement::SetChecked(PRBool aChecked)
{
  nsresult rv = NS_OK;

  //
  // If the user or JS attempts to set checked, whether it actually changes the
  // value or not, we say the value was changed so that defaultValue don't
  // affect it no more.
  //
  SetCheckedChanged(PR_TRUE);

  //
  // Don't do anything if we're not changing whether it's checked (it would
  // screw up state actually, especially when you are setting radio button to
  // false)
  //
  PRBool checked = PR_FALSE;
  GetChecked(&checked);
  if (checked == aChecked) {
    return NS_OK;
  }

  //
  // Set checked
  //
  PRInt32 type;
  GetType(&type);
  if (type == NS_FORM_INPUT_RADIO) {
    //
    // For radio button, we need to do some extra fun stuff
    //
    if (aChecked) {
      rv = RadioSetChecked();
    } else {
      rv = SetCheckedInternal(PR_FALSE);
      if (mForm) {
        nsAutoString name;
        GetName(name);
        mForm->SetCurrentRadioButton(name, nsnull);
      }
    }
  } else {
    rv = SetCheckedInternal(aChecked);
  }

  return rv;
}

NS_IMETHODIMP
nsHTMLInputElement::RadioSetChecked()
{
  nsresult rv = NS_OK;

  //
  // Find the selected radio button so we can deselect it
  //
  nsCOMPtr<nsIDOMHTMLInputElement> currentlySelected;
  nsAutoString name;
  if (mForm) {
    // This is only initialized in if (mForm) for use in later if (mForm)s
    GetName(name);
    mForm->GetCurrentRadioButton(name, getter_AddRefs(currentlySelected));
  }

  //
  // Deselect the currently selected radio button
  //
  if (currentlySelected) {
    rv = NS_STATIC_CAST(nsHTMLInputElement*,
                        NS_STATIC_CAST(nsIDOMHTMLInputElement*, currentlySelected)
         )->SetCheckedInternal(PR_FALSE);
  }

  //
  // Actually select this one
  //
  if (NS_SUCCEEDED(rv)) {
    rv = SetCheckedInternal(PR_TRUE);
  }

  //
  // Let the form know that we are now the One True Radio Button
  //
  if (mForm && NS_SUCCEEDED(rv)) {
    rv = mForm->SetCurrentRadioButton(name, NS_STATIC_CAST(nsIDOMHTMLInputElement*, this));
  }

  return rv;
}

NS_IMETHODIMP
nsHTMLInputElement::SetForm(nsIDOMHTMLFormElement* aForm,
                            PRBool aRemoveFromForm)
{
  return nsGenericHTMLLeafFormElement::SetForm(aForm, aRemoveFromForm);
}


nsresult
nsHTMLInputElement::SetCheckedInternal(PRBool aChecked)
{
  //
  // Set the value
  //
  SET_BOOLBIT(mBitField, BF_CHECKED, aChecked);

  //
  // Notify the frame
  //
  // No need to flush here since if there's no frame for this input at
  // this point we don't care about creating one, once it's created
  // the frame will do the right thing.
  //
  PRInt32 type;
  GetType(&type);

  nsIFormControlFrame* formControlFrame = GetFormControlFrame(PR_FALSE);
  if (formControlFrame) {
    nsCOMPtr<nsIPresContext> presContext;
    GetPresContext(this, getter_AddRefs(presContext));

    if (type == NS_FORM_INPUT_CHECKBOX) {
      nsICheckboxControlFrame* checkboxFrame = nsnull;
      CallQueryInterface(formControlFrame, &checkboxFrame);
      if (checkboxFrame) {
        checkboxFrame->OnChecked(presContext, aChecked);
      }
      // These are frames.  You don't refcount frames.  Silly wabbit.
    } else if (type == NS_FORM_INPUT_RADIO) {
      nsIRadioControlFrame* radioFrame = nsnull;
      CallQueryInterface(formControlFrame, &radioFrame);
      if (radioFrame) {
        radioFrame->OnChecked(presContext, aChecked);
      }
      // You don't refcount frames.
    }
  }
  // special attr for XBL form controls
  else if (type == NS_FORM_INPUT_CHECKBOX) {
    if (aChecked)
      SetAttr(kNameSpaceID_None, nsHTMLAtoms::inputCheckedPseudo,
              NS_LITERAL_STRING(""), PR_TRUE);
    else
      UnsetAttr(kNameSpaceID_None, nsHTMLAtoms::inputCheckedPseudo, PR_TRUE);
  }

  return NS_OK;
}

void
nsHTMLInputElement::FireOnChange()
{
  //
  // Since the value is changing, send out an onchange event (bug 23571)
  //
  nsCOMPtr<nsIPresContext> presContext;
  GetPresContext(this, getter_AddRefs(presContext));
  nsEventStatus status = nsEventStatus_eIgnore;
  nsEvent event;
  event.eventStructType = NS_EVENT;
  event.message = NS_FORM_CHANGE;
  HandleDOMEvent(presContext, &event, nsnull, NS_EVENT_FLAG_INIT, &status);
}

NS_IMETHODIMP
nsHTMLInputElement::Blur()
{
  return SetElementFocus(PR_FALSE);
}

NS_IMETHODIMP
nsHTMLInputElement::Focus()
{
  return SetElementFocus(PR_TRUE);
}

NS_IMETHODIMP
nsHTMLInputElement::SetFocus(nsIPresContext* aPresContext)
{
  NS_ENSURE_ARG_POINTER(aPresContext);

  // We can't be focus'd if we don't have a document
  if (! mDocument)
    return NS_OK;

  // first see if we are disabled or not. If disabled then do nothing.
  nsAutoString disabled;
  if (NS_CONTENT_ATTR_HAS_VALUE == GetAttr(kNameSpaceID_HTML,
                                           nsHTMLAtoms::disabled, disabled)) {
    return NS_OK;
  }
 
  // If the window is not active, do not allow the focus to bring the
  // window to the front.  We update the focus controller, but do
  // nothing else.
  nsCOMPtr<nsIFocusController> focusController;
  nsCOMPtr<nsIScriptGlobalObject> globalObj;
  mDocument->GetScriptGlobalObject(getter_AddRefs(globalObj));
  nsCOMPtr<nsPIDOMWindow> win(do_QueryInterface(globalObj));
  win->GetRootFocusController(getter_AddRefs(focusController));
  PRBool isActive = PR_FALSE;
  focusController->GetActive(&isActive);
  if (!isActive) {
    focusController->SetFocusedElement(this);
    return NS_OK;
  }

  nsCOMPtr<nsIEventStateManager> esm;

  aPresContext->GetEventStateManager(getter_AddRefs(esm));

  if (esm) {
    esm->SetContentState(this, NS_EVENT_STATE_FOCUS);
  }

  nsIFormControlFrame* formControlFrame = GetFormControlFrame(PR_TRUE);

  if (formControlFrame) {
    formControlFrame->SetFocus(PR_TRUE, PR_TRUE);
    formControlFrame->ScrollIntoView(aPresContext);
    // Could call SelectAll(aPresContext) here to automatically
    // select text when we receive focus - only for text and password!
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLInputElement::RemoveFocus(nsIPresContext* aPresContext)
{
  NS_ENSURE_ARG_POINTER(aPresContext);
  // If we are disabled, we probably shouldn't have focus in the
  // first place, so allow it to be removed.
  nsresult rv = NS_OK;

  nsIFormControlFrame* formControlFrame = GetFormControlFrame(PR_TRUE);

  if (formControlFrame) {
    formControlFrame->SetFocus(PR_FALSE, PR_FALSE);
  }

  nsCOMPtr<nsIEventStateManager> esm;
  aPresContext->GetEventStateManager(getter_AddRefs(esm));

  if (esm) {
    nsCOMPtr<nsIDocument> doc;

    GetDocument(*getter_AddRefs(doc));

    if (!doc)
      return NS_ERROR_NULL_POINTER;

    nsCOMPtr<nsIContent> rootContent;

    doc->GetRootContent(getter_AddRefs(rootContent));

    rv = esm->SetContentState(rootContent, NS_EVENT_STATE_FOCUS);
  }

  return rv;
}

NS_IMETHODIMP
nsHTMLInputElement::Select()
{
  nsresult rv = NS_OK;

  // first see if we are disabled or not. If disabled then do nothing.
  nsAutoString disabled;
  if (NS_CONTENT_ATTR_HAS_VALUE == GetAttr(kNameSpaceID_HTML,
                                           nsHTMLAtoms::disabled, disabled)) {
    return rv;
  }

  // see what type of input we are.  Only select for texts and passwords
  PRInt32 type;
  GetType(&type);

  if (NS_FORM_INPUT_PASSWORD == type ||
      NS_FORM_INPUT_TEXT == type) {
    // XXX Bug?  We have to give the input focus before contents can be
    // selected

    nsCOMPtr<nsIPresContext> presContext;
    GetPresContext(this, getter_AddRefs(presContext)); 

    // If the window is not active, do not allow the select to bring the
    // window to the front.  We update the focus controller, but do
    // nothing else.
    nsCOMPtr<nsIScriptGlobalObject> globalObj;
    mDocument->GetScriptGlobalObject(getter_AddRefs(globalObj));
    nsCOMPtr<nsPIDOMWindow> win(do_QueryInterface(globalObj));
    nsCOMPtr<nsIFocusController> focusController;
    win->GetRootFocusController(getter_AddRefs(focusController));
    PRBool isActive = PR_FALSE;
    focusController->GetActive(&isActive);
    if (!isActive) {
      focusController->SetFocusedElement(this);
      SelectAll(presContext);
      return NS_OK;
    }

    // Just like SetFocus() but without the ScrollIntoView()!
    nsEventStatus status = nsEventStatus_eIgnore;
    
    //If already handling select event, don't dispatch a second.
    if (!GET_BOOLBIT(mBitField, BF_HANDLING_SELECT_EVENT)) {
      nsEvent event;
      event.eventStructType = NS_EVENT;
      event.message = NS_FORM_SELECTED;
  
      SET_BOOLBIT(mBitField, BF_HANDLING_SELECT_EVENT, PR_TRUE);
      rv = HandleDOMEvent(presContext, &event, nsnull, NS_EVENT_FLAG_INIT,
                          &status);
      SET_BOOLBIT(mBitField, BF_HANDLING_SELECT_EVENT, PR_FALSE);
    }

    // If the DOM event was not canceled (e.g. by a JS event handler
    // returning false)
    if (status == nsEventStatus_eIgnore) {
      nsCOMPtr<nsIEventStateManager> esm;
      if (NS_OK == presContext->GetEventStateManager(getter_AddRefs(esm))) {
        esm->SetContentState(this, NS_EVENT_STATE_FOCUS);
      }

      nsIFormControlFrame* formControlFrame = GetFormControlFrame(PR_TRUE);

      if (formControlFrame) {
        formControlFrame->SetFocus(PR_TRUE, PR_TRUE);

        // Now Select all the text!
        SelectAll(presContext);
      }
    }
  }

  return rv;
}

void
nsHTMLInputElement::SelectAll(nsIPresContext* aPresContext)
{
  nsIFormControlFrame* formControlFrame = GetFormControlFrame(PR_TRUE);

  if (formControlFrame) {
    formControlFrame->SetProperty(aPresContext, nsHTMLAtoms::select,
                                  nsAutoString());
  }
}

NS_IMETHODIMP
nsHTMLInputElement::Click()
{
  nsresult rv = NS_OK;

  if (GET_BOOLBIT(mBitField, BF_HANDLING_CLICK)) // Fixes crash as in bug 41599
      return rv;                      // --heikki@netscape.com

  // first see if we are disabled or not. If disabled then do nothing.
  nsAutoString disabled;
  if (NS_CONTENT_ATTR_HAS_VALUE == GetAttr(kNameSpaceID_HTML,
                                           nsHTMLAtoms::disabled, disabled)) {
    return rv;
  }

  // see what type of input we are.  Only click button, checkbox, radio,
  // reset, submit, & image
  PRInt32 type;
  GetType(&type);
  if (NS_FORM_INPUT_BUTTON == type ||
      NS_FORM_INPUT_CHECKBOX == type ||
      NS_FORM_INPUT_RADIO == type ||
      NS_FORM_INPUT_RESET == type ||
      NS_FORM_INPUT_SUBMIT == type) {

    nsCOMPtr<nsIDocument> doc; // Strong
    rv = GetDocument(*getter_AddRefs(doc));

    if (doc) {
      PRInt32 numShells = doc->GetNumberOfShells();
      nsCOMPtr<nsIPresContext> context;
      for (PRInt32 i=0; i<numShells; i++) {
        nsCOMPtr<nsIPresShell> shell;
        doc->GetShellAt(i, getter_AddRefs(shell));
        if (shell) {
          rv = shell->GetPresContext(getter_AddRefs(context));
          if (context) {
            nsEventStatus status = nsEventStatus_eIgnore;
            nsMouseEvent event;
            event.eventStructType = NS_MOUSE_EVENT;
            event.message = NS_MOUSE_LEFT_CLICK;
            event.isShift = PR_FALSE;
            event.isControl = PR_FALSE;
            event.isAlt = PR_FALSE;
            event.isMeta = PR_FALSE;
            event.clickCount = 0;
            event.widget = nsnull;
            
            SET_BOOLBIT(mBitField, BF_HANDLING_CLICK, PR_TRUE);

            rv = HandleDOMEvent(context, &event, nsnull, NS_EVENT_FLAG_INIT,
                                &status);

            SET_BOOLBIT(mBitField, BF_HANDLING_CLICK, PR_FALSE);
          }
        }
      }
    }
  }
  
  return NS_OK;
}

nsresult
nsHTMLInputElement::MouseClickForAltText(nsIPresContext* aPresContext)
{
  NS_ENSURE_ARG_POINTER(aPresContext);
  PRBool disabled;

  nsresult rv = GetDisabled(&disabled);
  if (NS_FAILED(rv) || disabled) {
    return rv;
  }

  // Generate a submit event targeted at the form content
  nsCOMPtr<nsIContent> form(do_QueryInterface(mForm));

  if (form) {
    nsCOMPtr<nsIPresShell> shell;
    aPresContext->GetShell(getter_AddRefs(shell));
    if (shell) {
      nsCOMPtr<nsIContent> formControl = this; // kungFuDeathGrip

      nsFormEvent event;
      event.eventStructType = NS_FORM_EVENT;
      event.message         = NS_FORM_SUBMIT;
      event.originator      = formControl;
      nsEventStatus status  = nsEventStatus_eIgnore;
      shell->HandleDOMEventWithTarget(form, &event, &status);
    }
  }

  return rv;
}

NS_IMETHODIMP
nsHTMLInputElement::HandleDOMEvent(nsIPresContext* aPresContext,
                                   nsEvent* aEvent,
                                   nsIDOMEvent** aDOMEvent,
                                   PRUint32 aFlags,
                                   nsEventStatus* aEventStatus)
{
  NS_ENSURE_ARG_POINTER(aEventStatus);
  if ((aEvent->message == NS_FOCUS_CONTENT
       && GET_BOOLBIT(mBitField, BF_SKIP_FOCUS_EVENT))
      || (aEvent->message == NS_BLUR_CONTENT
          && GET_BOOLBIT(mBitField, BF_SKIP_FOCUS_EVENT))) {
    return NS_OK;
  }

  nsCOMPtr<nsIDOMEventTarget> oldTarget;
      
  // Do not process any DOM events if the element is disabled
  PRBool disabled;

  nsresult rv = GetDisabled(&disabled);
  if (NS_FAILED(rv) || disabled) {
    return rv;
  }
  
  // Bugscape 2369: Type might change during JS event handler
  //                This pointer is only valid until
  //                nsGenericHTMLLeafFormElement::HandleDOMEvent
  nsIFormControlFrame* formControlFrame = GetFormControlFrame(PR_FALSE);

  if (formControlFrame) {
    nsIFrame* formFrame = nsnull;
    CallQueryInterface(formControlFrame, &formFrame);
    if (formFrame) {
      const nsStyleUserInterface* uiStyle;
      formFrame->GetStyleData(eStyleStruct_UserInterface,
                              (const nsStyleStruct *&)uiStyle);

      if (uiStyle->mUserInput == NS_STYLE_USER_INPUT_NONE ||
          uiStyle->mUserInput == NS_STYLE_USER_INPUT_DISABLED) {
        return NS_OK;
      }
    }
  }

  // If we're a file input we have anonymous content underneath
  // that we need to hide.  We need to set the event target now
  // to ourselves
  //
  // Bugscape 2369: Type might change during JS event handler
  //                This is only valid until
  //                nsGenericHTMLLeafFormElement::HandleDOMEvent
  //
  PRInt32 type;
  GetType(&type);

  if (type == NS_FORM_INPUT_FILE ||
      type == NS_FORM_INPUT_TEXT ) {
    // If the event is starting here that's fine.  If it's not
    // init'ing here it started beneath us and needs modification.
    if (!(NS_EVENT_FLAG_INIT & aFlags)) {
      if (!*aDOMEvent) {
        // We haven't made a DOMEvent yet.  Force making one now.
        nsCOMPtr<nsIEventListenerManager> listenerManager;
        rv = GetListenerManager(getter_AddRefs(listenerManager));

        if (NS_FAILED(rv)) {
          return rv;
        }

        nsAutoString empty;
        rv = listenerManager->CreateEvent(aPresContext, aEvent, empty,
                                          aDOMEvent);

        if (NS_FAILED(rv)) {
          return rv;
        }
      }

      if (!*aDOMEvent) {
        return NS_ERROR_FAILURE;
      }

      nsCOMPtr<nsIPrivateDOMEvent> privateEvent(do_QueryInterface(*aDOMEvent));

      if (!privateEvent) {
        return NS_ERROR_FAILURE;
      }

      (*aDOMEvent)->GetTarget(getter_AddRefs(oldTarget));

      nsCOMPtr<nsIDOMEventTarget> originalTarget;

      nsCOMPtr<nsIDOMNSEvent> nsevent(do_QueryInterface(*aDOMEvent));

      if (nsevent) {
        nsevent->GetOriginalTarget(getter_AddRefs(originalTarget));
      }

      if (!originalTarget) {
        privateEvent->SetOriginalTarget(oldTarget);
      }

      nsCOMPtr<nsIDOMEventTarget>
        target(do_QueryInterface((nsIDOMHTMLInputElement*)this));

      privateEvent->SetTarget(target);
    }
  }

  // Preset the the value of the checkbox or the radiobutton before
  // calling into script If the event gets "cancelled" then we have to
  // "back out" this change, but the odds of that are slimmer than it
  // being set each time
  //
  // Start by remember the original value and for radio buttons we
  // must get the currently selected radiobtn. So we go get the
  // content instead of the frame
  //
  PRBool originalCheckedValue = PR_FALSE;
  PRBool checkWasSet         = PR_FALSE;

  nsCOMPtr<nsIDOMHTMLInputElement> selectedRadioButton;

  if (!(aFlags & NS_EVENT_FLAG_CAPTURE) &&
      aEvent->message == NS_MOUSE_LEFT_CLICK) {
    GetChecked(&originalCheckedValue);
    checkWasSet = PR_TRUE;
    switch(type) {
      case NS_FORM_INPUT_CHECKBOX:
        {
          PRBool checked;
          GetChecked(&checked);
          SetChecked(!checked);
          FireOnChange();
          // Fire an event to notify accessibility
#ifdef ACCESSIBILITY
          FireEventForAccessibility(aPresContext,
                                    NS_LITERAL_STRING("CheckboxStateChange"));
#endif
        }
        break;

      case NS_FORM_INPUT_RADIO:
        {
          if (mForm) {
            nsAutoString name;
            GetName(name);
            mForm->GetCurrentRadioButton(name,
                                         getter_AddRefs(selectedRadioButton));
          }

          PRBool checked;
          GetChecked(&checked);
          if (!checked) {
            SetChecked(PR_TRUE);
            FireOnChange();
          }
          // Fire an event to notify accessibility
#ifdef ACCESSIBILITY
          if (selectedRadioButton != this) {
            FireEventForAccessibility(aPresContext,
                                      NS_LITERAL_STRING("RadioStateChange"));
          }
#endif
        }
        break;

      default:
        break;
    } //switch
  }

  // If NS_EVENT_FLAG_NO_CONTENT_DISPATCH is set we will not allow content to handle
  // this event.  But to allow middle mouse button paste to work we must allow 
  // middle clicks to go to text fields anyway.
  PRBool noContentDispatch = aEvent->flags & NS_EVENT_FLAG_NO_CONTENT_DISPATCH;
  if ((type == NS_FORM_INPUT_TEXT || type == NS_FORM_INPUT_PASSWORD) &&
      aEvent->message == NS_MOUSE_MIDDLE_CLICK) {
    aEvent->flags &= ~NS_EVENT_FLAG_NO_CONTENT_DISPATCH;
  }

  // Try script event handlers first if its not a focus/blur event
  //we dont want the doc to get these
  rv = nsGenericHTMLLeafFormElement::HandleDOMEvent(aPresContext,
                                                    aEvent,
                                                    aDOMEvent,
                                                    aFlags,
                                                    aEventStatus);

  // Reset the flag for other content besides this text field
  aEvent->flags |= noContentDispatch ? NS_EVENT_FLAG_NO_CONTENT_DISPATCH : NS_EVENT_FLAG_NONE;

  // now check to see if the event was "cancelled"
  if (nsEventStatus_eConsumeNoDefault == *aEventStatus && checkWasSet
      && (type == NS_FORM_INPUT_CHECKBOX || type == NS_FORM_INPUT_RADIO)) {
    // if it was cancelled and a radio button, then set the old
    // selected btn to TRUE. if it is a checkbox then set it to its
    // original value
    if (selectedRadioButton) {
      selectedRadioButton->SetChecked(PR_TRUE);
    } else if (type == NS_FORM_INPUT_CHECKBOX) {
      SetChecked(originalCheckedValue);
    }
  }

  // Bugscape 2369: Frame might have changed during event handler
  formControlFrame = GetFormControlFrame(PR_FALSE);

  // Finish the special file control processing...
  if (oldTarget) {
    // If the event is starting here that's fine.  If it's not
    // init'ing here it started beneath us and needs modification.
    if (!(NS_EVENT_FLAG_INIT & aFlags)) {
      if (!*aDOMEvent) {
        return NS_ERROR_FAILURE;
      }
      nsCOMPtr<nsIPrivateDOMEvent> privateEvent =
        do_QueryInterface(*aDOMEvent);

      if (!privateEvent) {
        return NS_ERROR_FAILURE;
      }

      // This will reset the target to its original value
      privateEvent->SetTarget(oldTarget);
    }
  }

  // Bugscape 2369: type might have changed during event handler
  GetType(&type);

  if (type == NS_FORM_INPUT_IMAGE && 
      aEvent->message == NS_MOUSE_LEFT_BUTTON_UP && 
      nsEventStatus_eIgnore == *aEventStatus &&
      aFlags & NS_EVENT_FLAG_BUBBLE) {
    // Tell the frame about the click
    return MouseClickForAltText(aPresContext);
  }

  if ((NS_OK == rv) && (nsEventStatus_eIgnore == *aEventStatus) &&
      !(aFlags & NS_EVENT_FLAG_CAPTURE)) {
    switch (aEvent->message) {

      case NS_FOCUS_CONTENT:
      {
        if (formControlFrame) {
          SET_BOOLBIT(mBitField, BF_SKIP_FOCUS_EVENT, PR_TRUE);
          formControlFrame->SetFocus(PR_TRUE, PR_TRUE);
          SET_BOOLBIT(mBitField, BF_SKIP_FOCUS_EVENT, PR_FALSE);
          return NS_OK;
        }
      }                                                                         
      break; // NS_FOCUS_CONTENT

      case NS_KEY_PRESS:
      case NS_KEY_UP:
      {
        // For backwards compat, trigger checks/radios/buttons with
        // space or enter (bug 25300)
        nsKeyEvent * keyEvent = (nsKeyEvent *)aEvent;

        if ((aEvent->message == NS_KEY_PRESS &&
             keyEvent->keyCode == NS_VK_RETURN) ||
            (aEvent->message == NS_KEY_UP &&
             keyEvent->keyCode == NS_VK_SPACE)) {
          switch(type) {
            case NS_FORM_INPUT_CHECKBOX:
            case NS_FORM_INPUT_RADIO:
            {
              // Checkbox and Radio try to submit on Enter press
              if (keyEvent->keyCode != NS_VK_SPACE) {
                // Generate a submit event targeted at the form content
                nsCOMPtr<nsIContent> form(do_QueryInterface(mForm));

                if (form) {
                  nsCOMPtr<nsIPresShell> shell;
                  aPresContext->GetShell(getter_AddRefs(shell));
                  if (shell) {
                    nsCOMPtr<nsIContent> formControl = this; // kungFuDeathGrip

                    nsFormEvent event;
                    event.eventStructType = NS_FORM_EVENT;
                    event.message         = NS_FORM_SUBMIT;
                    event.originator      = formControl;
                    nsEventStatus status  = nsEventStatus_eIgnore;
                    shell->HandleDOMEventWithTarget(form, &event, &status);
                  }
                }
                break;  // If we are submitting, do not send click event
              }
              // else fall through and treat Space like click...
            }
            case NS_FORM_INPUT_BUTTON:
            case NS_FORM_INPUT_RESET:
            case NS_FORM_INPUT_SUBMIT:
            case NS_FORM_INPUT_IMAGE: // Bug 34418
            {
              nsEventStatus status = nsEventStatus_eIgnore;
              nsMouseEvent event;
              event.eventStructType = NS_MOUSE_EVENT;
              event.message = NS_MOUSE_LEFT_CLICK;
              event.isShift = PR_FALSE;
              event.isControl = PR_FALSE;
              event.isAlt = PR_FALSE;
              event.isMeta = PR_FALSE;
              event.clickCount = 0;
              event.widget = nsnull;
              rv = HandleDOMEvent(aPresContext, &event, nsnull,
                                  NS_EVENT_FLAG_INIT, &status);
            } // case
          } // switch
        }
      } break;// NS_KEY_PRESS || NS_KEY_UP

      // cancel all of these events for buttons
      case NS_MOUSE_MIDDLE_BUTTON_DOWN:
      case NS_MOUSE_MIDDLE_BUTTON_UP:
      case NS_MOUSE_MIDDLE_DOUBLECLICK:
      case NS_MOUSE_RIGHT_DOUBLECLICK:
      case NS_MOUSE_RIGHT_BUTTON_DOWN:
      case NS_MOUSE_RIGHT_BUTTON_UP:
      {
        if (type == NS_FORM_INPUT_BUTTON || 
            type == NS_FORM_INPUT_RESET || 
            type == NS_FORM_INPUT_SUBMIT ) {
          nsCOMPtr<nsIDOMNSEvent> nsevent;

          if (aDOMEvent) {
            nsevent = do_QueryInterface(*aDOMEvent);
          }

          if (nsevent) {
            nsevent->PreventBubble();
          } else {
            rv = NS_ERROR_FAILURE;
          }
        }

        break;
      }
      case NS_MOUSE_LEFT_CLICK:
      {
        switch(type) {
          case NS_FORM_INPUT_RESET:
          case NS_FORM_INPUT_SUBMIT:
          case NS_FORM_INPUT_IMAGE:
            {
              nsFormEvent event;
              event.eventStructType = NS_FORM_EVENT;
              event.message         = (type == NS_FORM_INPUT_RESET) ? NS_FORM_RESET : NS_FORM_SUBMIT;
              event.originator      = this;
              nsEventStatus status  = nsEventStatus_eIgnore;

              nsCOMPtr<nsIPresShell> presShell;
              aPresContext->GetShell(getter_AddRefs(presShell));
              // If |nsIPresShell::Destroy| has been called due to
              // handling the event (base class HandleDOMEvent, above),
              // the pres context will return a null pres shell.  See
              // bug 125624.
              if (presShell) {
                nsCOMPtr<nsIContent> form(do_QueryInterface(mForm));
                presShell->HandleEventWithTarget(&event, nsnull, form, 
                                                 NS_EVENT_FLAG_INIT, &status);
              }
            }
            break;

          default:
            break;
        } //switch 
      } break;// NS_MOUSE_LEFT_BUTTON_DOWN
    } //switch 
  } // if

  return rv;
}


// nsIHTMLContent

static nsGenericHTMLElement::EnumTable kInputTypeTable[] = {
  { "browse", NS_FORM_BROWSE }, // XXX not valid html, but it is convenient
  { "button", NS_FORM_INPUT_BUTTON },
  { "checkbox", NS_FORM_INPUT_CHECKBOX },
  { "file", NS_FORM_INPUT_FILE },
  { "hidden", NS_FORM_INPUT_HIDDEN },
  { "reset", NS_FORM_INPUT_RESET },
  { "image", NS_FORM_INPUT_IMAGE },
  { "password", NS_FORM_INPUT_PASSWORD },
  { "radio", NS_FORM_INPUT_RADIO },
  { "submit", NS_FORM_INPUT_SUBMIT },
  { "text", NS_FORM_INPUT_TEXT },
  { 0 }
};

NS_IMETHODIMP
nsHTMLInputElement::StringToAttribute(nsIAtom* aAttribute,
                                      const nsAReadableString& aValue,
                                      nsHTMLValue& aResult)
{
  if (aAttribute == nsHTMLAtoms::type) {
    nsGenericHTMLElement::EnumTable *table = kInputTypeTable;
    nsAutoString valueStr(aValue);
    while (nsnull != table->tag) { 
      if (valueStr.EqualsIgnoreCase(table->tag)) {
        // If the type is being changed to file, set the element value
        // to the empty string. This is for security.
        if (table->value == NS_FORM_INPUT_FILE)
          SetValue(NS_LITERAL_STRING(""));
        aResult.SetIntValue(table->value, eHTMLUnit_Enumerated);
        mType = table->value;  // set the type of this input
        return NS_CONTENT_ATTR_HAS_VALUE;
      }
      table++;
    }
  }
  else if (aAttribute == nsHTMLAtoms::checked) {
    aResult.SetEmptyValue();
    return NS_CONTENT_ATTR_HAS_VALUE;
  }
  else if (aAttribute == nsHTMLAtoms::disabled) {
    aResult.SetEmptyValue();
    return NS_CONTENT_ATTR_HAS_VALUE;
  }
  else if (aAttribute == nsHTMLAtoms::readonly) {
    aResult.SetEmptyValue();
    return NS_CONTENT_ATTR_HAS_VALUE;
  }
  else if (aAttribute == nsHTMLAtoms::width) {
    if (ParseValueOrPercent(aValue, aResult, eHTMLUnit_Pixel)) {
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }
  else if (aAttribute == nsHTMLAtoms::height) {
    if (ParseValueOrPercent(aValue, aResult, eHTMLUnit_Pixel)) {
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }
  else if (aAttribute == nsHTMLAtoms::maxlength) {
    if (ParseValue(aValue, 0, aResult, eHTMLUnit_Integer)) {
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }
  else if (aAttribute == nsHTMLAtoms::size) {
    if (ParseValue(aValue, 0, aResult, eHTMLUnit_Integer)) {
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }
  else if (aAttribute == nsHTMLAtoms::tabindex) {
    if (ParseValue(aValue, 0, aResult, eHTMLUnit_Integer)) {
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }
  else if (aAttribute == nsHTMLAtoms::border) {
    if (ParseValue(aValue, 0, aResult, eHTMLUnit_Pixel)) {
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }
  else if (aAttribute == nsHTMLAtoms::align) {
    if (ParseAlignValue(aValue, aResult)) {
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }
  else if (IsImage()) {
    if (ParseImageAttribute(aAttribute, aValue, aResult)) {
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }

  return NS_CONTENT_ATTR_NOT_THERE;
}

NS_IMETHODIMP
nsHTMLInputElement::AttributeToString(nsIAtom* aAttribute,
                                      const nsHTMLValue& aValue,
                                      nsAWritableString& aResult) const
{
  if (aAttribute == nsHTMLAtoms::type) {
    if (eHTMLUnit_Enumerated == aValue.GetUnit()) {
      // The DOM spec says that input types should be capitalized but
      // AFAIK all existing browsers return the type in lower case,
      // http://bugzilla.mozilla.org/show_bug.cgi?id=32369 is the bug
      // about this problem, we pass PR_FALSE as the last argument
      // here to avoid capitalizing the input type (this is required for
      // backwards compatibility). -- jst@netscape.com
      // Update.  The DOM spec will be changed to have input types be
      // all-lowercase.  See
      // http://bugzilla.mozilla.org/show_bug.cgi?id=113174#c12
      // -- bzbarsky@mit.edu

      EnumValueToString(aValue, kInputTypeTable, aResult);

      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }
  else if (aAttribute == nsHTMLAtoms::align) {
    if (eHTMLUnit_Enumerated == aValue.GetUnit()) {
      VAlignValueToString(aValue, aResult);
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }
  else if (aAttribute == nsHTMLAtoms::checked) {
    aResult.Assign(NS_LITERAL_STRING("checked"));
    return NS_CONTENT_ATTR_HAS_VALUE;
  }
  else if (IsImage() && ImageAttributeToString(aAttribute, aValue,
                                               aResult)) {
    return NS_CONTENT_ATTR_HAS_VALUE;
  }

  return nsGenericHTMLLeafFormElement::AttributeToString(aAttribute, aValue,
                                                         aResult);
}

static void
MapAttributesIntoRule(const nsIHTMLMappedAttributes* aAttributes,
                      nsRuleData* aData)
{
  if (!aData)
    return;

  nsHTMLValue value;
  aAttributes->GetAttribute(nsHTMLAtoms::type, value);
  if (eHTMLUnit_Enumerated == value.GetUnit()) {  
    switch (value.GetIntValue()) {
      case NS_FORM_INPUT_IMAGE: {
        nsGenericHTMLElement::MapImageBorderAttributeInto(aAttributes, aData);
        nsGenericHTMLElement::MapImageMarginAttributeInto(aAttributes, aData);
        nsGenericHTMLElement::MapImagePositionAttributeInto(aAttributes, aData);
        break;
      }
    }
  }

  nsGenericHTMLElement::MapAlignAttributeInto(aAttributes, aData);
  nsGenericHTMLElement::MapCommonAttributesInto(aAttributes, aData);
}

NS_IMETHODIMP
nsHTMLInputElement::GetMappedAttributeImpact(const nsIAtom* aAttribute, PRInt32 aModType,
                                             PRInt32& aHint) const
{
  if (aAttribute == nsHTMLAtoms::value) {
    aHint = NS_STYLE_HINT_REFLOW;
  }
  else if ((aAttribute == nsHTMLAtoms::align) ||
           (aAttribute == nsHTMLAtoms::type)) {
    aHint = NS_STYLE_HINT_FRAMECHANGE;
  }
  else if (!GetCommonMappedAttributesImpact(aAttribute, aHint)) {
    if (!GetImageMappedAttributesImpact(aAttribute, aHint)) {
      if (!GetImageBorderAttributeImpact(aAttribute, aHint)) {
        aHint = NS_STYLE_HINT_CONTENT;
      }
    }
  }

  return NS_OK;
}


NS_IMETHODIMP
nsHTMLInputElement::GetAttributeMappingFunction(nsMapRuleToAttributesFunc& aMapRuleFunc) const
{
  aMapRuleFunc = &MapAttributesIntoRule;
  return NS_OK;
}


// nsIFormControl

NS_IMETHODIMP
nsHTMLInputElement::GetType(PRInt32* aType)
{
  NS_ASSERTION(aType, "aType must not be null!");
  *aType = mType;
  return NS_OK;
}

#ifdef DEBUG
NS_IMETHODIMP
nsHTMLInputElement::SizeOf(nsISizeOfHandler* aSizer, PRUint32* aResult) const
{
  *aResult = sizeof(*this) + BaseSizeOf(aSizer);

  return NS_OK;
}
#endif


// Controllers Methods

NS_IMETHODIMP
nsHTMLInputElement::GetControllers(nsIControllers** aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);

  PRInt32 type;
  GetType(&type);

  //XXX: what about type "file"?
  if (NS_FORM_INPUT_TEXT == type || NS_FORM_INPUT_PASSWORD == type)
  {
    if (!mControllers)
    {
      NS_ENSURE_SUCCESS (
        nsComponentManager::CreateInstance(kXULControllersCID,
                                           nsnull,
                                           NS_GET_IID(nsIControllers),
                                           getter_AddRefs(mControllers)),
        NS_ERROR_FAILURE);
      if (!mControllers) { return NS_ERROR_NULL_POINTER; }

      nsresult rv;
      nsCOMPtr<nsIController>
        controller(do_CreateInstance("@mozilla.org/editor/editorcontroller;1",
                                     &rv));
      if (NS_FAILED(rv)) return rv;

      nsCOMPtr<nsIEditorController>
        editorController = do_QueryInterface(controller, &rv);

      if (NS_FAILED(rv))
        return rv;

      rv = editorController->Init(nsnull);
      if (NS_FAILED(rv))
        return rv;

      mControllers->AppendController(controller);
    }
  }

  *aResult = mControllers;
  NS_IF_ADDREF(*aResult);

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLInputElement::GetTextLength(PRInt32* aTextLength)
{
  nsAutoString val;

  nsresult rv = GetValue(val);

  *aTextLength = val.Length();

  return rv;
}

NS_IMETHODIMP
nsHTMLInputElement::SetSelectionRange(PRInt32 aSelectionStart,
                                      PRInt32 aSelectionEnd)
{
  nsIFormControlFrame* formControlFrame = GetFormControlFrame(PR_TRUE);

  if (formControlFrame) {
    nsCOMPtr<textControlPlace>
      textControlFrame(do_QueryInterface(formControlFrame));

    if (textControlFrame)
      textControlFrame->SetSelectionRange(aSelectionStart, aSelectionEnd);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLInputElement::GetSelectionStart(PRInt32* aSelectionStart)
{
  NS_ENSURE_ARG_POINTER(aSelectionStart);
  
  PRInt32 selEnd;
  return GetSelectionRange(aSelectionStart, &selEnd);
}

NS_IMETHODIMP
nsHTMLInputElement::SetSelectionStart(PRInt32 aSelectionStart)
{
  nsIFormControlFrame* formControlFrame = GetFormControlFrame(PR_TRUE);

  if (formControlFrame) {
    nsCOMPtr<textControlPlace>
      textControlFrame(do_QueryInterface(formControlFrame));

    if (textControlFrame)
      textControlFrame->SetSelectionStart(aSelectionStart);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLInputElement::GetSelectionEnd(PRInt32* aSelectionEnd)
{
  NS_ENSURE_ARG_POINTER(aSelectionEnd);
  
  PRInt32 selStart;
  return GetSelectionRange(&selStart, aSelectionEnd);
}


NS_IMETHODIMP
nsHTMLInputElement::SetSelectionEnd(PRInt32 aSelectionEnd)
{
  nsIFormControlFrame* formControlFrame = GetFormControlFrame(PR_TRUE);

  if (formControlFrame) {
    nsCOMPtr<textControlPlace>
      textControlFrame(do_QueryInterface(formControlFrame));

    if (textControlFrame)
      textControlFrame->SetSelectionEnd(aSelectionEnd);
  }

  return NS_OK;
}

nsresult
nsHTMLInputElement::GetSelectionRange(PRInt32* aSelectionStart,
                                      PRInt32* aSelectionEnd)
{
  nsIFormControlFrame* formControlFrame = GetFormControlFrame(PR_TRUE);

  if (formControlFrame) {
    nsCOMPtr<textControlPlace>
      textControlFrame(do_QueryInterface(formControlFrame));

    if (textControlFrame)
      textControlFrame->GetSelectionRange(aSelectionStart, aSelectionEnd);
  }

  return NS_OK;
}

#ifdef ACCESSIBILITY
nsresult
nsHTMLInputElement::FireEventForAccessibility(nsIPresContext* aPresContext,
                                              const nsAReadableString& aEventType)
{
  nsCOMPtr<nsIEventListenerManager> listenerManager;

  nsresult rv = GetListenerManager(getter_AddRefs(listenerManager));
  if ( !listenerManager )
    return rv;

  // Create the DOM event
  nsCOMPtr<nsIDOMEvent> domEvent;
  rv = listenerManager->CreateEvent(aPresContext,
                                    nsnull, 
                                    NS_LITERAL_STRING("MutationEvents"),
                                    getter_AddRefs(domEvent) );
  if ( !domEvent )
    return NS_ERROR_FAILURE;

  // Initialize the mutation event
  nsCOMPtr<nsIDOMMutationEvent> mutEvent(do_QueryInterface(domEvent));
  if ( !mutEvent )
    return NS_ERROR_FAILURE;
  nsAutoString empty;
  mutEvent->InitMutationEvent( aEventType, PR_TRUE, PR_TRUE, nsnull, empty, empty, empty, nsIDOMMutationEvent::MODIFICATION);

  // Set the target of the event to this nsHTMLInputElement, which should be checkbox content??
  nsCOMPtr<nsIPrivateDOMEvent> privEvent(do_QueryInterface(domEvent));
  if ( ! privEvent )
    return NS_ERROR_FAILURE;
  nsCOMPtr<nsIDOMEventTarget> targ(do_QueryInterface(NS_STATIC_CAST(nsIDOMHTMLInputElement *, this)));
  if ( ! targ )
    return NS_ERROR_FAILURE;
  privEvent->SetTarget(targ);

  // Dispatch the event
  nsCOMPtr<nsIDOMEventReceiver> eventReceiver(do_QueryInterface(listenerManager));
  if ( ! eventReceiver )
    return NS_ERROR_FAILURE;
  PRBool noDefault;
  eventReceiver->DispatchEvent(domEvent, &noDefault);

  return NS_OK;
}
#endif

nsresult
nsHTMLInputElement::Reset()
{
  nsresult rv = NS_OK;
  PRInt32 type;
  GetType(&type);

  nsIFormControlFrame* formControlFrame = GetFormControlFrame(PR_FALSE);

  switch (type) {
    case NS_FORM_INPUT_CHECKBOX:
    case NS_FORM_INPUT_RADIO:
    {
      PRBool resetVal;
      GetDefaultChecked(&resetVal);
      rv = SetChecked(resetVal);
      SetCheckedChanged(PR_FALSE);
      break;
    }
    case NS_FORM_INPUT_HIDDEN:
    case NS_FORM_INPUT_PASSWORD:
    case NS_FORM_INPUT_TEXT:
    {
      // If the frame is there, we have to set the value so that it will show
      // up.
      if (formControlFrame) {
        nsAutoString resetVal;
        GetDefaultValue(resetVal);
        rv = SetValue(resetVal);
      }
      SetValueChanged(PR_FALSE);
      break;
    }
    case NS_FORM_INPUT_FILE:
    {
      // Resetting it to blank should not perform security check
      rv = SetValueSecure(NS_LITERAL_STRING(""), nsnull, PR_FALSE);
      break;
    }
    default:
      break;
  }

  // Notify frame that it has been reset
  if (formControlFrame) {
    formControlFrame->OnContentReset();
  }
  return rv;
}

NS_IMETHODIMP
nsHTMLInputElement::SubmitNamesValues(nsIFormSubmission* aFormSubmission,
                                      nsIContent* aSubmitElement)
{
  nsresult rv = NS_OK;

  //
  // Disabled elements don't submit
  //
  PRBool disabled;
  rv = GetDisabled(&disabled);
  if (NS_FAILED(rv) || disabled) {
    return rv;
  }

  //
  // Get the type (many ops depend on the type)
  //
  PRInt32 type;
  rv = GetType(&type);
  if (NS_FAILED(rv)) {
    return rv;
  }

  //
  // For type=reset, and type=button, we just never submit, period.
  //
  if (type == NS_FORM_INPUT_RESET || type == NS_FORM_INPUT_BUTTON) {
    return rv;
  }

  //
  // For type=image and type=button, we only submit if we were the button
  // pressed
  //
  if ((type == NS_FORM_INPUT_SUBMIT || type == NS_FORM_INPUT_IMAGE)
      && aSubmitElement != this) {
    return rv;
  }

  //
  // For type=radio and type=checkbox, we only submit if checked=true
  //
  if (type == NS_FORM_INPUT_RADIO || type == NS_FORM_INPUT_CHECKBOX) {
    PRBool checked;
    rv = GetChecked(&checked);
    if (NS_FAILED(rv) || !checked) {
      return rv;
    }
  }

  //
  // Get the name
  //
  nsAutoString name;
  rv = GetAttr(kNameSpaceID_None, nsHTMLAtoms::name, name);
  if (NS_FAILED(rv)) {
    return rv;
  }
  PRBool nameThere = (rv != NS_CONTENT_ATTR_NOT_THERE);

  //
  // Submit .x, .y for input type=image
  //
  if (type == NS_FORM_INPUT_IMAGE) {
    // Go to the frame to find out where it was clicked.  This is the only
    // case where I can actually see using the frame, because you're talking
    // about a value--mouse click--that is rightfully the domain of the frame.
    //
    // If the frame isn't there or isn't an ImageControlFrame, then we're not
    // submitting these values no matter *how* nicely you ask.
    PRInt32 clickedX;
    PRInt32 clickedY;
    nsIFormControlFrame* formControlFrame = GetFormControlFrame(PR_TRUE);

    nsCOMPtr<nsIImageControlFrame> imageControlFrame(
        do_QueryInterface(formControlFrame));
    if (imageControlFrame) {
      imageControlFrame->GetClickedX(&clickedX);
      imageControlFrame->GetClickedY(&clickedY);

      // Convert the values to strings for submission
      char buf[20];
      sprintf(&buf[0], "%d", clickedX);
      nsAutoString xVal = NS_ConvertASCIItoUCS2(buf);
      sprintf(&buf[0], "%d", clickedY);
      nsAutoString yVal = NS_ConvertASCIItoUCS2(buf);

      if (!name.IsEmpty()) {
        aFormSubmission->AddNameValuePair(this,
                                          name + NS_LITERAL_STRING(".x"), xVal);
        aFormSubmission->AddNameValuePair(this,
                                          name + NS_LITERAL_STRING(".y"), yVal);
      } else {
        // If the Image Element has no name, simply return x and y
        // to Nav and IE compatability.
        aFormSubmission->AddNameValuePair(this, NS_LITERAL_STRING("x"), xVal);
        aFormSubmission->AddNameValuePair(this, NS_LITERAL_STRING("y"), yVal);
      }
    }
  }

  //
  // Submit name=value
  //

  // If name not there, don't submit
  if (!nameThere) {
    return rv;
  }

  // Get the value
  nsAutoString value;
  rv = GetValue(value);
  if (NS_FAILED(rv)) {
    return rv;
  }

  //
  // Submit file if it's input type=file and this encoding method accepts files
  //
  PRBool submittedValue = PR_FALSE;
  if (type == NS_FORM_INPUT_FILE) {

    //
    // Open the file
    //
    nsCOMPtr<nsILocalFile> file(do_CreateInstance(NS_LOCAL_FILE_CONTRACTID,
                                                  &rv));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = file->InitWithUnicodePath(value.get());
    if (NS_SUCCEEDED(rv)) {

      //
      // Get the leaf path name (to be submitted as the value)
      //
      PRUnichar* leafNameChars = nsnull;
      rv = file->GetUnicodeLeafName(&leafNameChars);
      NS_ENSURE_SUCCESS(rv, rv);

      if (leafNameChars) {
        nsAutoString filename;
        filename.Adopt(leafNameChars);

        PRBool acceptsFiles = PR_FALSE;
        aFormSubmission->AcceptsFiles(&acceptsFiles);

        if (acceptsFiles) {
          //
          // Get content type
          //
          nsCOMPtr<nsIMIMEService> MIMEService =
            do_GetService(NS_MIMESERVICE_CONTRACTID, &rv);
          NS_ENSURE_SUCCESS(rv, rv);

          char * contentTypeChars = nsnull;
          rv = MIMEService->GetTypeFromFile(file, &contentTypeChars);

          nsCAutoString contentType;
          if (contentTypeChars) {
            contentType.Adopt(contentTypeChars);
          } else {
            contentType = NS_LITERAL_CSTRING("application/octet-stream");
          }

          //
          // Get input stream
          //
          nsCOMPtr<nsIInputStream> rawStream;
          rv = NS_NewLocalFileInputStream(getter_AddRefs(rawStream), file);
          if (rawStream) {
            nsCOMPtr<nsIInputStream> bufferedStream;
            rv = NS_NewBufferedInputStream(getter_AddRefs(bufferedStream),
                                           rawStream, 8192);
            NS_ENSURE_SUCCESS(rv, rv);

            //
            // Submit
            //
            aFormSubmission->AddNameFilePair(this, name, filename,
                                             bufferedStream, contentType,
                                             PR_FALSE);
            submittedValue = PR_TRUE;
          }
        }

        //
        // If we don't submit as a file, at least submit the truncated filename.
        //
        if (!submittedValue) {
          aFormSubmission->AddNameValuePair(this, name, filename);
          submittedValue = PR_TRUE;
        }
      }
    }
  }

  if (!submittedValue) {
    // Submit
    // (for type=image, only submit if value is non-null)
    if (type != NS_FORM_INPUT_IMAGE || !value.IsEmpty()) {
      rv = aFormSubmission->AddNameValuePair(this, name, value);
    }
  }

  return rv;
}


NS_IMETHODIMP
nsHTMLInputElement::SaveState(nsIPresContext* aPresContext,
                              nsIPresState** aState)
{
  nsresult rv = NS_OK;

  PRInt32 type;
  GetType(&type);
  
  switch (type) {
    case NS_FORM_INPUT_CHECKBOX:
    case NS_FORM_INPUT_RADIO:
      {
        PRBool checked = PR_FALSE;
        GetChecked(&checked);
        rv = GetPrimaryPresState(this, aState);
        if (*aState) {
          if (checked) {
            rv = (*aState)->SetStateProperty(NS_LITERAL_STRING("checked"),
                                             NS_LITERAL_STRING("t"));
          } else {
            rv = (*aState)->SetStateProperty(NS_LITERAL_STRING("checked"),
                                             NS_LITERAL_STRING("f"));
          }
          NS_ASSERTION(NS_SUCCEEDED(rv), "checked save failed!");
        }
        break;
      }

    // Never save passwords in session history
    case NS_FORM_INPUT_PASSWORD:
      break;
    case NS_FORM_INPUT_TEXT:
    case NS_FORM_INPUT_FILE:
      {
        nsresult rv = GetPrimaryPresState(this, aState);
        if (*aState) {
          nsString value;
          GetValue(value);
          // XXX Should use nsAutoString above but ConvertStringLineBreaks
          // requires mOwnsBuffer!
          rv = nsLinebreakConverter::ConvertStringLineBreaks(
                   value,
                   nsLinebreakConverter::eLinebreakPlatform,
                   nsLinebreakConverter::eLinebreakContent);
          NS_ASSERTION(NS_SUCCEEDED(rv), "Converting linebreaks failed!");
          rv = (*aState)->SetStateProperty(NS_LITERAL_STRING("value"), value);
          NS_ASSERTION(NS_SUCCEEDED(rv), "value save failed!");
        }
        break;
      }
  }

  return rv;
}

NS_IMETHODIMP
nsHTMLInputElement::RestoreState(nsIPresContext* aPresContext,
                                 nsIPresState* aState)
{
  nsresult rv = NS_OK;

  PRInt32 type;
  GetType(&type);
  
  switch (type) {
    case NS_FORM_INPUT_CHECKBOX:
    case NS_FORM_INPUT_RADIO:
      {
        nsAutoString checked;
        rv = aState->GetStateProperty(NS_LITERAL_STRING("checked"), checked);
        // We assume that we are the only ones who saved the state.  Thus we
        // know the exact value that would have been saved.
        if (checked.Equals(NS_LITERAL_STRING("t"))) {
          SetChecked(PR_TRUE);
        } else {
          SetChecked(PR_FALSE);
        }

        break;
      }

    // Never save passwords in session history
    case NS_FORM_INPUT_PASSWORD:
      break;
    case NS_FORM_INPUT_TEXT:
    case NS_FORM_INPUT_FILE:
      {
        nsAutoString value;
        rv = aState->GetStateProperty(NS_LITERAL_STRING("value"), value);
        NS_ASSERTION(NS_SUCCEEDED(rv), "value restore failed!");
        SetValue(value);
        break;
      }
  }

  return rv;
}


/*
 * Radio group stuff
 */

NS_IMETHODIMP
nsHTMLInputElement::AddedToRadioGroup()
{
  //
  // Currently the only time radio groups really happen is in forms
  //
  if (!mForm) {
    return NS_OK;
  }

  //
  // If the input element is checked, and we add it to the group, it will
  // deselect whatever is currently selected in that group
  //
  PRBool checked;
  GetChecked(&checked);
  if (checked) {
    //
    // If it is checked, call "RadioSetChecked" to perform the selection/
    // deselection ritual.  This has the side effect of repainting the
    // radio button, but as adding a checked radio button into the group
    // should not be that common an occurrence, I think we can live with
    // that.
    //
    RadioSetChecked();
  }

  //
  // For integrity purposes, we have to ensure that "checkedChanged" is
  // the same for this new element as for all the others in the group
  //
  PRBool checkedChanged = PR_FALSE;
  nsCOMPtr<nsIRadioVisitor> visitor;
  nsresult rv = NS_GetRadioGetCheckedChangedVisitor(&checkedChanged, this,
                                           getter_AddRefs(visitor));
  NS_ENSURE_SUCCESS(rv, rv);
  nsAutoString name;
  GetName(name);
  rv = mForm->WalkRadioGroup(name, visitor);
  SetCheckedChangedInternal(checkedChanged);

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLInputElement::RemovedFromRadioGroup(nsIForm* aForm, nsAString* aName)
{
  //
  // Currently radio groups only happen in forms
  //
  if (!aForm) {
    return NS_OK;
  }

  //
  // If this button was checked, we need to notify the group that there is no
  // longer a selected radio button
  //
  PRBool checked = PR_FALSE;
  GetChecked(&checked);

  if (checked) {
    if (aName) {
      aForm->SetCurrentRadioButton(*aName, (nsIDOMHTMLInputElement*)nsnull);
    } else {
      nsAutoString name;
      GetName(name);
      aForm->SetCurrentRadioButton(name, (nsIDOMHTMLInputElement*)nsnull);
    }
  }

  return NS_OK;
}

nsresult
nsHTMLInputElement::VisitGroup(nsIRadioVisitor* aVisitor)
{
  nsresult rv;
  if (mForm) {
    nsAutoString name;
    GetName(name);
    rv = mForm->WalkRadioGroup(name, aVisitor);
  } else {
    PRBool stop = PR_FALSE;
    rv = aVisitor->Visit(this, &stop);
  }
  return rv;
}


//
// Visitor classes
//
//
// CLASS nsRadioVisitor
//
// (this is the superclass of the others)
//
class nsRadioVisitor : public nsIRadioVisitor {
public:
  nsRadioVisitor() { NS_INIT_ISUPPORTS(); }
  virtual ~nsRadioVisitor() { };

  NS_DECL_ISUPPORTS

  NS_IMETHOD Visit(nsIFormControl* aRadio, PRBool* aStop) = 0;
};

NS_IMPL_ADDREF(nsRadioVisitor)
NS_IMPL_RELEASE(nsRadioVisitor)

NS_INTERFACE_MAP_BEGIN(nsRadioVisitor)
  NS_INTERFACE_MAP_ENTRY(nsIRadioVisitor)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END


//
// CLASS nsRadioSetCheckedChangedVisitor
//
class nsRadioSetCheckedChangedVisitor : public nsRadioVisitor {
public:
  nsRadioSetCheckedChangedVisitor(PRBool aCheckedChanged) :
    nsRadioVisitor(), mCheckedChanged(aCheckedChanged)
    { }

  virtual ~nsRadioSetCheckedChangedVisitor() { }

  NS_IMETHOD Visit(nsIFormControl* aRadio, PRBool* aStop)
  {
    nsCOMPtr<nsIRadioControlElement> radio(do_QueryInterface(aRadio));
    NS_ASSERTION(radio, "Visit() passed a null button (or non-radio)!");
    radio->SetCheckedChangedInternal(mCheckedChanged);
    *aStop = PR_TRUE;
    return NS_OK;
  }

protected:
  PRPackedBool mCheckedChanged;
};

//
// CLASS nsRadioGetCheckedChangedVisitor
//
class nsRadioGetCheckedChangedVisitor : public nsRadioVisitor {
public:
  nsRadioGetCheckedChangedVisitor(PRBool* aCheckedChanged,
                                  nsIFormControl* aExcludeElement) :
    nsRadioVisitor(),
    mCheckedChanged(aCheckedChanged),
    mExcludeElement(aExcludeElement)
    { }

  virtual ~nsRadioGetCheckedChangedVisitor() { }

  NS_IMETHOD Visit(nsIFormControl* aRadio, PRBool* aStop)
  {
    if (aRadio == mExcludeElement) {
      return NS_OK;
    }
    nsCOMPtr<nsIRadioControlElement> radio(do_QueryInterface(aRadio));
    NS_ASSERTION(radio, "Visit() passed a null button (or non-radio)!");
    radio->GetCheckedChanged(mCheckedChanged);
    *aStop = PR_TRUE;
    return NS_OK;
  }

protected:
  PRBool* mCheckedChanged;
  nsIFormControl* mExcludeElement;
};

NS_METHOD
NS_GetRadioSetCheckedChangedVisitor(PRBool aCheckedChanged,
                                    nsIRadioVisitor** aVisitor)
{
  //
  // These are static so that we don't have to keep creating new visitors for
  // such an ordinary process all the time.  There are only two possibilities
  // for this visitor: set to true, and set to false.
  //
  static nsIRadioVisitor* visitorTrue = nsnull;
  static nsIRadioVisitor* visitorFalse = nsnull;

  //
  // Get the visitor that sets them to true
  //
  if (aCheckedChanged) {
    if (!visitorTrue) {
      visitorTrue = new nsRadioSetCheckedChangedVisitor(PR_TRUE);
      if (!visitorTrue) {
        return NS_ERROR_OUT_OF_MEMORY;
      }
      NS_ADDREF(visitorTrue);
    }
    *aVisitor = visitorTrue;
  }

  //
  // Get the visitor that sets them to false
  //
  if (!aCheckedChanged) {
    if (!visitorFalse) {
      visitorFalse = new nsRadioSetCheckedChangedVisitor(PR_FALSE);
      if (!visitorFalse) {
        return NS_ERROR_OUT_OF_MEMORY;
      }
      NS_ADDREF(visitorFalse);
    }
    *aVisitor = visitorFalse;
  }

  NS_ADDREF(*aVisitor);
  return NS_OK;
}

NS_METHOD
NS_GetRadioGetCheckedChangedVisitor(PRBool* aCheckedChanged,
                                    nsIFormControl* aExcludeElement,
                                    nsIRadioVisitor** aVisitor)
{
  *aVisitor = new nsRadioGetCheckedChangedVisitor(aCheckedChanged,
                                                  aExcludeElement);
  if (!*aVisitor) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  NS_ADDREF(*aVisitor);

  return NS_OK;
}
