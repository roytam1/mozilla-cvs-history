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
 *   Pierre Phaneuf <pp@ludusdesign.com>
 */



#define NS_IMPL_IDS
#include "nsICharsetConverterManager.h"
#undef NS_IMPL_IDS 

#include "nsCOMPtr.h"
#include "nsXPIDLString.h"

#include "nsID.h"


#include "nsFormFrame.h"
#include "nsIFormControlFrame.h"
#include "nsFormControlFrame.h"
#include "nsFileControlFrame.h"
#include "nsRadioControlGroup.h"

#include "nsIForm.h"
#include "nsIFormControl.h"
#include "nsIAtom.h"
#include "nsHTMLIIDs.h"
#include "nsIRenderingContext.h"
#include "nsIPresShell.h"
#include "nsIPresContext.h"
#include "nsIStyleContext.h"
#include "nsCSSRendering.h"
#include "nsHTMLIIDs.h"
#include "nsDebug.h"
#include "nsIWidget.h"
#include "nsVoidArray.h"
#include "nsHTMLAtoms.h"
#include "nsIHTMLAttributes.h"
#include "nsCRT.h"
#include "nsIURL.h"
#include "nsIHTMLDocument.h"

#include "nsIFormProcessor.h"

#include "nsIIOService.h"
#include "nsIURL.h"
#include "nsNetUtil.h"
static NS_DEFINE_CID(kIOServiceCID, NS_IOSERVICE_CID);
#include "nsIDocument.h"
#include "nsILinkHandler.h"
#include "nsGfxRadioControlFrame.h"
#include "nsDocument.h"
#include "nsIDOMHTMLFormElement.h"
#include "nsIDOMNSHTMLFormElement.h"
#include "nsDOMError.h"
#include "nsHTMLParts.h"
#include "nsIReflowCommand.h"

#include "nsIUnicodeEncoder.h"

// FormSubmit observer notification
#include "nsIFormSubmitObserver.h"
#include "nsIObserverService.h"
#include "nsIServiceManager.h"

// Get base target for submission
#include "nsIHTMLContent.h"
#include "nsIDOMHTMLInputElement.h"

#include "net.h"
#include "xp_file.h"
#include "prio.h"
#include "prmem.h"
#include "prenv.h"
#include "prlong.h"

// Rewrite of Multipart form posting
#include "nsSpecialSystemDirectory.h"
#include "nsIFileSpec.h"
#include "nsFileSpec.h"
#include "nsIProtocolHandler.h"
#include "nsIHTTPProtocolHandler.h"
#include "nsIServiceManager.h"
#include "nsEscape.h"
#include "nsLinebreakConverter.h"
#include "nsIMIMEService.h"

#include "nsILocalFile.h"		// Using nsILocalFile to get file size

// Security
#include "nsIScriptSecurityManager.h"

static NS_DEFINE_CID(kCharsetConverterManagerCID, NS_ICHARSETCONVERTERMANAGER_CID);
static NS_DEFINE_CID(kMIMEServiceCID, NS_MIMESERVICE_CID);

//----------------------------------------------------------------------

static NS_DEFINE_IID(kIFormManagerIID, NS_IFORMMANAGER_IID);
static NS_DEFINE_IID(kIFormIID, NS_IFORM_IID);
static NS_DEFINE_IID(kIFormControlIID, NS_IFORMCONTROL_IID);
static NS_DEFINE_IID(kIFormControlFrameIID, NS_IFORMCONTROLFRAME_IID);
static NS_DEFINE_IID(kIDOMNodeIID, NS_IDOMNODE_IID);
static NS_DEFINE_IID(kIDOMElementIID, NS_IDOMELEMENT_IID);
static NS_DEFINE_IID(kIDOMHTMLFormElementIID, NS_IDOMHTMLFORMELEMENT_IID);
static NS_DEFINE_IID(kIDOMNSHTMLFormElementIID, NS_IDOMNSHTMLFORMELEMENT_IID);
static NS_DEFINE_IID(kIContentIID, NS_ICONTENT_IID);
static NS_DEFINE_IID(kIFrameIID, NS_IFRAME_IID);
static NS_DEFINE_IID(kIHTMLDocumentIID, NS_IHTMLDOCUMENT_IID);
static NS_DEFINE_IID(kIDOMHTMLElementIID, NS_IDOMHTMLELEMENT_IID);
static NS_DEFINE_IID(kFormProcessorCID, NS_IFORMPROCESSOR_CID);

NS_IMETHODIMP
nsFormFrame::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  if (aIID.Equals(kIFormManagerIID)) {
    *aInstancePtr = (void*)(nsIFormManager*)this;
    return NS_OK;
  }
  return nsBlockFrame::QueryInterface(aIID, aInstancePtr);
}

nsrefcnt nsFormFrame::AddRef(void)
{
  NS_ERROR("not supported");
  return 0;
}

nsrefcnt nsFormFrame::Release(void)
{
  NS_ERROR("not supported");
  return 0;
}

nsFormFrame::nsFormFrame()
  : nsBlockFrame()
{
  mTextSubmitter = nsnull;
  mTextSubmitterSet = PR_FALSE;
}

nsFormFrame::~nsFormFrame()
{
  RemoveRadioGroups();
  PRInt32 numControls = mFormControls.Count();
  PRInt32 i;
  // Traverse list from end to 0 -> void array remove method does less work
  for (i = (numControls-1); i>=0; i--) {
    nsIFormControlFrame* fcFrame = (nsIFormControlFrame*) mFormControls.ElementAt(i);
    fcFrame->SetFormFrame(nsnull);
    mFormControls.RemoveElement(fcFrame);
  }
}

PRBool 
nsFormFrame::CanSubmit(nsFormControlFrame& aFrame)
{ 
  if (mTextSubmitter == &aFrame) {
    return PR_TRUE;
  }
  PRInt32 type;
  aFrame.GetType(&type);
  if ((NS_FORM_INPUT_SUBMIT == type) || (NS_FORM_INPUT_IMAGE == type)) {
    return PR_TRUE;
  }
  return PR_FALSE;
}

NS_IMETHODIMP
nsFormFrame::GetAction(nsString* aAction)
{
  nsresult result = NS_OK;
  if (mContent) {
    nsIDOMHTMLFormElement* form = nsnull;
    result = mContent->QueryInterface(kIDOMHTMLFormElementIID, (void**)&form);
    if ((NS_OK == result) && form) {
      form->GetAction(*aAction);
      NS_RELEASE(form);
    }
  }
  return result;
}

NS_IMETHODIMP
nsFormFrame::GetTarget(nsString* aTarget)
{
  nsresult result = NS_OK;
  if (mContent) {
    nsIDOMHTMLFormElement* form = nsnull;
    result = mContent->QueryInterface(kIDOMHTMLFormElementIID, (void**)&form);
    if ((NS_OK == result) && form) {
      form->GetTarget(*aTarget);
      if ((*aTarget).Length() == 0) {
        nsIHTMLContent* content = nsnull;
	result = form->QueryInterface(kIHTMLContentIID, (void**)&content);
	if ((NS_OK == result) && content) {
	  content->GetBaseTarget(*aTarget);
	  NS_RELEASE(content);
	}
      }
      NS_RELEASE(form);
    }
  }
  return result;
}

NS_IMETHODIMP
nsFormFrame::GetMethod(PRInt32* aMethod)
{
  nsresult result = NS_OK;
  if (mContent) {
    nsIHTMLContent* form = nsnull;
    result = mContent->QueryInterface(kIHTMLContentIID, (void**)&form);
    if ((NS_OK == result) && form) {
      nsHTMLValue value;
      if (NS_CONTENT_ATTR_HAS_VALUE == (form->GetHTMLAttribute(nsHTMLAtoms::method, value))) {
        if (eHTMLUnit_Enumerated == value.GetUnit()) {
          *aMethod = value.GetIntValue();
          NS_RELEASE(form);
          return NS_OK;
        }
      }
      NS_RELEASE(form);
    }
  }
  *aMethod = NS_FORM_METHOD_GET;
  return result;
}

NS_IMETHODIMP
nsFormFrame::GetEnctype(PRInt32* aEnctype)
{
  nsresult result = NS_OK;
  if (mContent) {
    nsIHTMLContent* form = nsnull;
    result = mContent->QueryInterface(kIHTMLContentIID, (void**)&form);
    if ((NS_OK == result) && form) {
      nsHTMLValue value;
      if (NS_CONTENT_ATTR_HAS_VALUE == (form->GetHTMLAttribute(nsHTMLAtoms::enctype, value))) {
        if (eHTMLUnit_Enumerated == value.GetUnit()) {
          *aEnctype = value.GetIntValue();
          NS_RELEASE(form);
          return NS_OK;
        }
      }
      NS_RELEASE(form);
    }
  }
  *aEnctype = NS_FORM_ENCTYPE_URLENCODED;
  return result;
}

NS_IMETHODIMP 
nsFormFrame::OnReset(nsIPresContext* aPresContext)
{
  PRInt32 numControls = mFormControls.Count();
  for (int i = 0; i < numControls; i++) {
    nsIFormControlFrame* fcFrame = (nsIFormControlFrame*) mFormControls.ElementAt(i);
    fcFrame->Reset(aPresContext);
  }
  return NS_OK;
}

void nsFormFrame::RemoveRadioGroups()
{
  int numRadioGroups = mRadioGroups.Count();
  for (int i = 0; i < numRadioGroups; i++) {
    nsRadioControlGroup* radioGroup = (nsRadioControlGroup *) mRadioGroups.ElementAt(i);
    delete radioGroup;
  }
  mRadioGroups.Clear();
}

void nsFormFrame::AddFormControlFrame(nsIPresContext* aPresContext, nsIFrame& aFrame)
{
  // Make sure we have a form control
  nsIFormControlFrame* fcFrame = nsnull;
  nsresult result = aFrame.QueryInterface(kIFormControlFrameIID, (void**)&fcFrame);
  if ((NS_OK != result) || (nsnull == fcFrame)) {
    return;
  }

  // Get this control's form frame and add this control to it
  nsCOMPtr<nsIContent> iContent;
  result = aFrame.GetContent(getter_AddRefs(iContent));
  if (NS_SUCCEEDED(result) && iContent) {
    nsCOMPtr<nsIFormControl> formControl;
    result = iContent->QueryInterface(kIFormControlIID, getter_AddRefs(formControl));
    if (NS_SUCCEEDED(result) && formControl) {
      nsCOMPtr<nsIDOMHTMLFormElement> formElem;
      result = formControl->GetForm(getter_AddRefs(formElem));
      if (NS_SUCCEEDED(result) && formElem) {
        nsCOMPtr<nsIPresShell> presShell;
        result = aPresContext->GetShell(getter_AddRefs(presShell));
        if (NS_SUCCEEDED(result) && presShell) {
          nsIContent* formContent;
          result = formElem->QueryInterface(kIContentIID, (void**)&formContent);
          if (NS_SUCCEEDED(result) && formContent) {
            nsFormFrame* formFrame = nsnull;
            result = presShell->GetPrimaryFrameFor(formContent, (nsIFrame**)&formFrame);
            if (NS_SUCCEEDED(result) && formFrame) {
              formFrame->AddFormControlFrame(aPresContext, *fcFrame);
              fcFrame->SetFormFrame(formFrame);
            }
            NS_RELEASE(formContent);
          }
        }
      }
    }
  }
}

void nsFormFrame::RemoveFormControlFrame(nsIFormControlFrame& aFrame)
{
  // Remove form control from array
  mFormControls.RemoveElement(&aFrame);
}

NS_IMETHODIMP
nsFormFrame::RemoveFrame(nsIPresContext* aPresContext,
                         nsIPresShell&   aPresShell,
                         nsIAtom*        aListName,
                         nsIFrame*       aOldFrame)
{
  nsresult rv = NS_OK;

  nsIFormControlFrame* fcFrame = nsnull;
  nsresult result = aOldFrame->QueryInterface(kIFormControlFrameIID, (void**)&fcFrame);
  if ((NS_OK == result) || (nsnull != fcFrame)) {
    PRInt32 type;
    fcFrame->GetType(&type);
    if (NS_FORM_INPUT_RADIO == type) {
      nsRadioControlGroup * group;
      nsAutoString name;
      nsresult rv = GetRadioInfo(fcFrame, name, group);
      if (NS_SUCCEEDED(rv) && nsnull != group) {
        DoDefaultSelection(aPresContext, group, (nsGfxRadioControlFrame*)fcFrame);
      }
    }
  }

  return nsBlockFrame::RemoveFrame(aPresContext, aPresShell, aListName, aOldFrame);
}

nsresult nsFormFrame::GetRadioInfo(nsIFormControlFrame* aFrame,
                                   nsString& aName,
                                   nsRadioControlGroup *& aGroup)
{
  aGroup = nsnull;
  aName.SetLength(0);
  aFrame->GetName(&aName);
  PRBool hasName = aName.Length() > 0;

  // radio group processing
  if (hasName) { 
    int numGroups = mRadioGroups.Count();
    nsRadioControlGroup* group;
    for (int j = 0; j < numGroups; j++) {
      group = (nsRadioControlGroup *) mRadioGroups.ElementAt(j);
      nsString groupName;
      group->GetName(groupName);
      if (groupName.Equals(aName)) {
        aGroup = group;
        return NS_OK;
      }
    }
  }
  return NS_ERROR_FAILURE;
}

void nsFormFrame::DoDefaultSelection(nsIPresContext*          aPresContext, 
                                     nsRadioControlGroup *    aGroup,
                                     nsGfxRadioControlFrame * aRadioToIgnore)
{
#if 0
  // If in standard mode, then a radio group MUST default 
  // to the first item in the group (it must be selected)
  nsCompatibility mode;
  aPresContext->GetCompatibilityMode(&mode);
  if (eCompatibility_Standard == mode) {
    // first find out if any have a default selection
    PRInt32 i;
    PRInt32 numItems = aGroup->GetNumRadios();
    PRBool changed          = PR_FALSE;
    PRBool oneIsDefSelected = PR_FALSE;
    for (i=0;i<numItems && !oneIsDefSelected;i++) {
      nsGfxRadioControlFrame * radioBtn = (nsGfxRadioControlFrame*) aGroup->GetRadioAt(i);
      nsCOMPtr<nsIContent> content;
      radioBtn->GetContent(getter_AddRefs(content));
      if (content) {
        nsCOMPtr<nsIDOMHTMLInputElement> input(do_QueryInterface(content));
        if (input) {
          input->GetDefaultChecked(&oneIsDefSelected);
          PRBool currentValue = radioBtn->GetChecked(PR_FALSE);
          if (currentValue != oneIsDefSelected) {
            changed = PR_TRUE;
          }
        }
      }
    }

    // if there isn't a default selcted radio then 
    // select the firdst one in the group.
    // if aRadioToIgnore is not null then it is being deleted
    // so don't select that item, select the next one if there is one.
    if (!changed && !oneIsDefSelected && numItems > 0) {
      nsGfxRadioControlFrame * radioBtn = (nsGfxRadioControlFrame*) aGroup->GetRadioAt(0);
      if (aRadioToIgnore != nsnull && aRadioToIgnore == radioBtn) {
        if (numItems == 1) {
          return;
        }
        radioBtn = (nsGfxRadioControlFrame*) aGroup->GetRadioAt(1);
      }
      nsCOMPtr<nsIContent> content;
      radioBtn->GetContent(getter_AddRefs(content));
      if (content) {
        nsCOMPtr<nsIDOMHTMLInputElement> input(do_QueryInterface(content));
        if (input) {
          input->SetChecked(PR_TRUE);
          OnRadioChecked(aPresContext, *radioBtn, PR_TRUE);
        }
      }
    }
  }
#endif
}


void nsFormFrame::AddFormControlFrame(nsIPresContext* aPresContext, nsIFormControlFrame& aFrame)
{
  mFormControls.AppendElement(&aFrame);

  // determine which radio buttons belong to which radio groups, unnamed radio buttons
  // don't go into any group since they can't be submitted. Determine which controls
  // are capable of form submission.
  PRInt32 type;
  aFrame.GetType(&type);

  // a solo text control can be a submitter (if return is hit)
  if ((NS_FORM_INPUT_TEXT == type) || (NS_FORM_INPUT_PASSWORD == type)) {
    // Only set if this is the first text input found.  Otherwise, set to null.
    if (!mTextSubmitterSet) {
      mTextSubmitter = &aFrame;
      mTextSubmitterSet = PR_TRUE;
    } else mTextSubmitter = nsnull;
    return;
  }

  // radio group processing
  if (NS_FORM_INPUT_RADIO == type) { 
    nsGfxRadioControlFrame* radioFrame = (nsGfxRadioControlFrame*)&aFrame;
    // gets the name of the radio group and the group
    nsRadioControlGroup * group;
    nsAutoString name;
    nsresult rv = GetRadioInfo(&aFrame, name, group);
    if (NS_SUCCEEDED(rv) && nsnull != group) {
      group->AddRadio(radioFrame);
    } else {
      group = new nsRadioControlGroup(name);
      mRadioGroups.AppendElement(group);
      group->AddRadio(radioFrame);
    }
    // allow only one checked radio button
    if (radioFrame->GetChecked(PR_TRUE)) {
	    if (nsnull == group->GetCheckedRadio()) {
	      group->SetCheckedRadio(radioFrame);
	    } else {
	      radioFrame->SetChecked(aPresContext, PR_FALSE, PR_TRUE);
	    }
    }
    DoDefaultSelection(aPresContext, group);
  }
}

void nsFormFrame::RemoveRadioControlFrame(nsIFormControlFrame * aFrame)
{
  PRInt32 type;
  aFrame->GetType(&type);

  // radio group processing
  if (NS_FORM_INPUT_RADIO == type) { 
    nsGfxRadioControlFrame* radioFrame = (nsGfxRadioControlFrame*)aFrame;
    // gets the name of the radio group and the group
    nsRadioControlGroup * group;
    nsAutoString name;
    nsresult rv = GetRadioInfo(aFrame, name, group);
    if (NS_SUCCEEDED(rv) && nsnull != group) {
      group->RemoveRadio(radioFrame);
    }
  }
}
  
void
nsFormFrame::OnRadioChecked(nsIPresContext* aPresContext, nsGfxRadioControlFrame& aControl, PRBool aChecked)
{
  nsString radioName;
  aControl.GetName(&radioName);
  if (0 == radioName.Length()) { // don't consider a radio without a name 
    return;
  }
 
  // locate the radio group with the name of aRadio
  int numGroups = mRadioGroups.Count();
  for (int j = 0; j < numGroups; j++) {
    nsRadioControlGroup* group = (nsRadioControlGroup *) mRadioGroups.ElementAt(j);
    nsString groupName;
    group->GetName(groupName);
    nsGfxRadioControlFrame* checkedRadio = group->GetCheckedRadio();
    if (groupName.Equals(radioName)) {
      if (aChecked) {
        if (&aControl != checkedRadio) {
          if (checkedRadio) {
            checkedRadio->SetChecked(aPresContext, PR_FALSE, PR_FALSE);
          }
          group->SetCheckedRadio(&aControl);
        }
      } else if (&aControl == checkedRadio) {
        checkedRadio->SetChecked(aPresContext, PR_FALSE, PR_FALSE);
      }
    }
  }
}


nsresult
NS_NewFormFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame, PRUint32 aFlags)
{
  NS_PRECONDITION(aNewFrame, "null OUT ptr");
  if (nsnull == aNewFrame) {
    return NS_ERROR_NULL_POINTER;
  }
  nsFormFrame* it = new (aPresShell) nsFormFrame;
  if (!it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  it->SetFlags(aFlags);
  *aNewFrame = it;
  return NS_OK;
}


PRIVATE
void DebugPrint(char* aLabel, nsString aString)
{
  char* out = aString.ToNewCString();
  printf("\n %s=%s\n", aLabel, out);
  delete [] out;
}



// Give the form process observer a chance to modify the value passed for form submission
nsresult
nsFormFrame::ProcessValue(nsIFormProcessor& aFormProcessor, nsIFormControlFrame* aFrameControl, const nsString& aName, nsString& aValue)
{
  nsresult res = NS_OK;

  nsIFrame *frame = nsnull;
  res = aFrameControl->QueryInterface(kIFrameIID, (void **)&frame);
  if (NS_SUCCEEDED(res) && (frame)) {
    nsCOMPtr<nsIContent> content;
    nsresult rv = frame->GetContent(getter_AddRefs(content));
    if (NS_SUCCEEDED(rv) && content) {
      nsCOMPtr<nsIDOMHTMLElement> formElement;
      res = content->QueryInterface(kIDOMHTMLElementIID, getter_AddRefs(formElement));
      if (NS_SUCCEEDED(res) && formElement) {
	 	    res = aFormProcessor.ProcessValue(formElement, aName, aValue);
		    NS_ASSERTION(NS_SUCCEEDED(res), "unable Notify form process observer"); 
      }
    }
  }

  return res;
}


// submission

NS_IMETHODIMP
nsFormFrame::OnSubmit(nsIPresContext* aPresContext, nsIFrame* aFrame)
{
  if (!mContent) {
    return NS_FORM_NOTOK;
  }

   // Get a service to process the value part of the form data
   // If one doesn't exist, that fine. It's not required.
  nsresult result = NS_OK;
  NS_WITH_SERVICE(nsIFormProcessor, formProcessor, kFormProcessorCID, &result)

  nsString data; // this could be more efficient, by allocating a larger buffer

  PRInt32 method, enctype;
  GetMethod(&method);
  GetEnctype(&enctype);

  PRBool isURLEncoded = (NS_FORM_ENCTYPE_MULTIPART != enctype);

  // for enctype=multipart/form-data, force it to be post
  // if method is "" (not specified) use "get" as default
  PRBool isPost = (NS_FORM_METHOD_POST == method) || !isURLEncoded; 

  nsIFormControlFrame* fcFrame = nsnull;

  // Since JS Submit() calls are not linked to an element, aFrame is null.
  // fcframe will remain null, but IsSuccess will return succes in this case.
  if (aFrame != nsnull) {
    aFrame->QueryInterface(kIFormControlFrameIID, (void**)&fcFrame);
  }

  // Notify observers that the form is being submitted.
  result = NS_OK;
  NS_WITH_SERVICE(nsIObserverService, service, NS_OBSERVERSERVICE_PROGID, &result);
  if (NS_FAILED(result)) return result;

  nsString  theTopic(NS_FORMSUBMIT_SUBJECT);
  nsIEnumerator* theEnum;
  result = service->EnumerateObserverList(theTopic.GetUnicode(), &theEnum);
  if (NS_SUCCEEDED(result) && theEnum){
    nsCOMPtr<nsISupports> inst;
      
    for (theEnum->First(); theEnum->IsDone() != NS_OK; theEnum->Next()) {
      result = theEnum->CurrentItem(getter_AddRefs(inst));
      if (NS_SUCCEEDED(result) && inst) {
        nsCOMPtr<nsIFormSubmitObserver> formSubmitObserver = do_QueryInterface(inst, &result);
        if (NS_SUCCEEDED(result) && formSubmitObserver) {
          formSubmitObserver->Notify(mContent);
        }
      }
    }
    NS_RELEASE(theEnum);
  }

  nsIFileSpec* multipartDataFile = nsnull;
  if (isURLEncoded) {
    result = ProcessAsURLEncoded(formProcessor, isPost, data, fcFrame);
  }
  else {
    result = ProcessAsMultipart(formProcessor, multipartDataFile, fcFrame);
  }

  // Don't bother submitting form if we failed to generate a valid submission
  if (NS_FAILED(result)) {
    NS_IF_RELEASE(multipartDataFile);
    return result;
  }

  // make the url string
  nsILinkHandler* handler;
  if (NS_OK == aPresContext->GetLinkHandler(&handler)) {
    nsAutoString href;
    GetAction(&href);

    // Resolve url to an absolute url
    nsCOMPtr<nsIURI> docURL;
    nsCOMPtr<nsIDocument> doc;
    mContent->GetDocument(*getter_AddRefs(doc));
    NS_ASSERTION(doc, "No document found in Form Submit!\n");

    doc->GetBaseURL(*getter_AddRefs(docURL));
    NS_ASSERTION(docURL, "No Base URL found in Form Submit!\n");

      // If an action is not specified and we are inside 
      // a HTML document then reload the URL. This makes us
      // compatible with 4.x browsers.
      // If we are in some other type of document such as XML or
      // XUL, do nothing. This prevents undesirable reloading of
      // a document inside XUL.

    if (href.Equals("")) {
      nsCOMPtr<nsIHTMLDocument> htmlDoc;
      if (PR_FALSE == NS_SUCCEEDED(doc->QueryInterface(kIHTMLDocumentIID,
                                             getter_AddRefs(htmlDoc)))) {   
        // Must be a XML, XUL or other non-HTML document type
        // so do nothing.
        return NS_OK;
      } 

      // Necko's MakeAbsoluteURI doesn't reuse the baseURL's rel path if it is
      // passed a zero length rel path.
      char* relPath = nsnull;
      docURL->GetSpec(&relPath);
      NS_ASSERTION(relPath, "Rel path couldn't be formed in form submit!\n");
      if (relPath) {
        href.Append(relPath);
        nsCRT::free(relPath);
      } else {
        result = NS_ERROR_OUT_OF_MEMORY;
      }
    } else {
      // Get security manager, check to see if access to action URI is allowed.
      NS_WITH_SERVICE(nsIScriptSecurityManager, securityManager,
                      NS_SCRIPTSECURITYMANAGER_PROGID, &result);
      nsCOMPtr<nsIURI> actionURL;
      if (NS_FAILED(result) ||
          NS_FAILED(result = NS_NewURI(getter_AddRefs(actionURL), href, docURL)) ||
          NS_FAILED(result = securityManager->CheckLoadURI(docURL, actionURL))) 
      {
        return result;
      }

      nsXPIDLCString scheme;
      if (NS_FAILED(result = actionURL->GetScheme(getter_Copies(scheme))))
        return result;
      if (nsCRT::strcmp(scheme, "mailto") == 0) {
        PRBool enabled;
        if (NS_FAILED(result = securityManager->IsCapabilityEnabled("UniversalSendMail", 
                                                                    &enabled)))
        {
          return result;
        }
        if (!enabled) {
          // Form submit to a mailto: URI requires the UniversalSendMail privilege
          return NS_ERROR_DOM_SECURITY_ERR;
        }
      }
    }

    nsAutoString target;
    GetTarget(&target);

    if (!isPost) {
      if (href.FindChar('?', PR_FALSE, 0) == kNotFound) { // Add a ? if needed
        href.Append('?');
      } else {                              // Adding to existing query string
        if (href.Last() != '&' && href.Last() != '?') {   // Add a & if needed
          href.Append('&');
        }
      }
      href.Append(data);
    }
    nsAutoString absURLSpec;
    result = NS_MakeAbsoluteURI(href, docURL, absURLSpec);
    if (NS_FAILED(result)) return result;

    // Now pass on absolute url to the click handler
    nsIInputStream* postDataStream = nsnull;
    if (isPost) {
      nsresult rv;
      char* postBuffer = data.ToNewCString();

      if (isURLEncoded) {
        rv = NS_NewPostDataStream(!isURLEncoded, postBuffer, 0, &postDataStream);
      } else {
// Cut-and-paste of NS_NewPostDataStream
        NS_WITH_SERVICE(nsIIOService, serv, kIOServiceCID, &rv);
        if (NS_FAILED(rv)) return rv;
	nsCOMPtr<nsIProtocolHandler> pHandler;
	rv = serv->GetProtocolHandler("http", getter_AddRefs(pHandler));
	if (NS_FAILED(rv)) return rv;
	
	nsCOMPtr<nsIHTTPProtocolHandler> http = do_QueryInterface(pHandler, &rv);
	if (NS_FAILED(rv)) return rv;
	
// Cut-and-paste of nsHTTPHandler::NewPostDataStream inside if(isFile)
        nsIInputStream* rawStream = nsnull; // Strong
        rv = multipartDataFile->GetInputStream(&rawStream); // AddRef
	if (NS_FAILED(rv)) return rv;
	
	rv = http->NewEncodeStream(rawStream, nsIHTTPProtocolHandler::ENCODE_NORMAL, &postDataStream);
        NS_RELEASE(rawStream);
// End Cut-and-pastisms
      }

      if (NS_OK != rv) {
        delete [] postBuffer;
      }

      /* The postBuffer is now owned by the IPostData instance */
    }    
    if (handler) {
      handler->OnLinkClick(mContent, eLinkVerb_Replace,
                           absURLSpec.GetUnicode(),
                           target.GetUnicode(), postDataStream);
    }
// We need to delete the data file somewhere...
//    if (!isURLEncoded) {
//      nsFileSpec mdf = nsnull;
//      result = multipartDataFile->GetFileSpec(&mdf);
//      if (NS_SUCCEEDED(result) && mdf) {
//        mdf.Delete(PR_FALSE);
//      }
//    }
// XXX DON'T NS_IF_RELEASE(postDataStream), this happens in Necko!
    NS_IF_RELEASE(handler);

// If you need these for debugging...
// wrap them in DEBUG_<username>
// Printing the data and url prints the contents of passwords
//DebugPrint("url", absURLSpec);
//DebugPrint("data", data);
  }
  return result;
}

// XXX i18n helper routines
char*
nsFormFrame::UnicodeToNewBytes(const PRUnichar* aSrc, PRUint32 aLen, nsIUnicodeEncoder* encoder)
{
   char* res = nsnull;
   if(NS_SUCCEEDED(encoder->Reset()))
   {
      PRInt32 maxByteLen = 0;
      if(NS_SUCCEEDED(encoder->GetMaxLength(aSrc, (PRInt32) aLen, &maxByteLen))) 
      {
          res = new char[maxByteLen+1];
          if(nsnull != res) 
          {
             PRInt32 reslen = maxByteLen;
             PRInt32 reslen2 ;
             PRInt32 srclen = aLen;
             encoder->Convert(aSrc, &srclen, res, &reslen);
             reslen2 = maxByteLen-reslen;
             encoder->Finish(res+reslen, &reslen2);
             res[reslen+reslen2] = '\0';
          }
      }

   }
   return res;
}

// XXX i18n helper routines
nsString*
nsFormFrame::URLEncode(const nsString& aString, nsIUnicodeEncoder* encoder) 
{
  char* inBuf = nsnull;
  if(encoder)
    inBuf  = UnicodeToNewBytes(aString.GetUnicode(), aString.Length(), encoder);

  if(nsnull == inBuf)
    inBuf  = aString.ToNewCString();

  // convert to CRLF breaks
  char* convertedBuf = nsLinebreakConverter::ConvertLineBreaks(inBuf,
                           nsLinebreakConverter::eLinebreakPlatform, nsLinebreakConverter::eLinebreakNet);
  delete [] inBuf;
  
  char* outBuf = nsEscape(convertedBuf, url_XPAlphas);
  nsString* result = new nsString(outBuf);
  nsCRT::free(outBuf);
  nsAllocator::Free(convertedBuf);
  return result;
}

void nsFormFrame::GetSubmitCharset(nsString& oCharset)
{
  oCharset = "UTF-8"; // default to utf-8
  nsresult rv;
  // XXX
  // We may want to get it from the HTML 4 Accept-Charset attribute first
  // see 17.3 The FORM element in HTML 4 for details

  // Get the charset from document
  nsIDocument* doc = nsnull;
  mContent->GetDocument(doc);
  if( nsnull != doc ) {
    rv = doc->GetDocumentCharacterSet(oCharset);
    NS_RELEASE(doc);
  }

}

NS_IMETHODIMP nsFormFrame::GetEncoder(nsIUnicodeEncoder** encoder)
{
  *encoder = nsnull;
  nsAutoString charset;
  nsresult rv = NS_OK;
  GetSubmitCharset(charset);
  
  // Get Charset, get the encoder.
  nsICharsetConverterManager * ccm = nsnull;
  rv = nsServiceManager::GetService(kCharsetConverterManagerCID ,
                                    NS_GET_IID(nsICharsetConverterManager),
                                    (nsISupports**)&ccm);
  if(NS_SUCCEEDED(rv) && (nsnull != ccm)) {
     rv = ccm->GetUnicodeEncoder(&charset, encoder);
     nsServiceManager::ReleaseService( kCharsetConverterManagerCID, ccm);
     if (nsnull == encoder) {
       rv = NS_ERROR_FAILURE;
     }
     if (NS_SUCCEEDED(rv)) {
       rv = (*encoder)->SetOutputErrorBehavior(nsIUnicodeEncoder::kOnError_Replace, nsnull, (PRUnichar)'?');
     }
  }
  return NS_OK;
}

#define CRLF "\015\012"   
nsresult nsFormFrame::ProcessAsURLEncoded(nsIFormProcessor* aFormProcessor, PRBool isPost, nsString& aData, nsIFormControlFrame* aFrame)
{
  nsresult rv = NS_OK;
  nsString buf;
  PRBool firstTime = PR_TRUE;
  PRUint32 numChildren = mFormControls.Count();

  nsIUnicodeEncoder *encoder = nsnull;
  if(NS_FAILED(GetEncoder(&encoder)))  // Non-fatal error
     encoder = nsnull;

  // collect and encode the data from the children controls
  for (PRUint32 childX = 0; childX < numChildren; childX++) {
    nsIFormControlFrame* child = (nsIFormControlFrame*) mFormControls.ElementAt(childX);
    if (child && child->IsSuccessful(aFrame)) {
      PRInt32 numValues = 0;
      PRInt32 maxNumValues = child->GetMaxNumValues();
      if (0 >= maxNumValues) {
        continue;
      }
      nsString* names = new nsString[maxNumValues];
      if (!names) {
        rv = NS_ERROR_OUT_OF_MEMORY;
      } else {
        nsString* values = new nsString[maxNumValues];
        if (!values) {
          rv = NS_ERROR_OUT_OF_MEMORY;
        } else {
          if (PR_TRUE == child->GetNamesValues(maxNumValues, numValues, values, names)) {
            for (int valueX = 0; valueX < numValues; valueX++){
              if (PR_TRUE == firstTime) {
                firstTime = PR_FALSE;
              } else {
                buf += "&";
              }
              nsString* convName = URLEncode(names[valueX], encoder);
              buf += *convName;
              delete convName;
              buf += "=";
              nsAutoString newValue;
              newValue = values[valueX];
              if (aFormProcessor) {
                ProcessValue(*aFormProcessor, child, names[valueX], newValue);
              }
              nsString* convValue = URLEncode(newValue, encoder);
              buf += *convValue;
              delete convValue;
            }
          }
          delete [] values;
        }
        delete [] names;
      }
    }
  }
    aData.SetLength(0);
    if (isPost) {
      char size[16];
      sprintf(size, "%d", buf.Length());
      aData = "Content-type: application/x-www-form-urlencoded";
#ifdef SPECIFY_CHARSET_IN_CONTENT_TYPE
      nsString charset;
      GetSubmitCharset(charset);
      aData += "; charset=";
      aData += charset;
#endif
      aData += CRLF;
      aData += "Content-Length: ";
      aData += size;
      aData += CRLF;
      aData += CRLF;
    } 
  aData += buf;
  // Need to append CRLF to end of stream for compatability with Nav and IE
  if (isPost) {
    aData += CRLF;
  }
  NS_IF_RELEASE(encoder);
  return rv;
}

// include the file name without the directory
const char*
nsFormFrame::GetFileNameWithinPath(char* aPathName)
{
#ifdef XP_MAC
  // On a Mac the only invalid character in a file name is a : so we have to avoid
  // the test for '\'
  char* fileNameStart = PL_strrchr(aPathName, ':');
#else
  char* fileNameStart = PL_strrchr(aPathName, '\\'); // windows
  if (!fileNameStart) { // try unix
    fileNameStart = PL_strrchr(aPathName, '/');
  }
#endif
  if (fileNameStart) { 
    return fileNameStart+1;
  }
  else {
    return aPathName;
  }
}

nsresult
nsFormFrame::GetContentType(char* aPathName, char** aContentType)
{
  nsresult rv = NS_OK;
  NS_ASSERTION(aContentType, "null pointer");

  if (aPathName && *aPathName) {
    // Get file extension and mimetype from that.g936
    char* fileExt = &aPathName[nsCRT::strlen(aPathName)-1];
    while (fileExt && (*fileExt != '.')) {
      fileExt--;
    }
    if (fileExt) {
      NS_WITH_SERVICE(nsIMIMEService, MIMEService, kMIMEServiceCID, &rv);
      if (NS_FAILED(rv)) return rv;
      if (NS_SUCCEEDED(MIMEService->GetTypeFromExtension(++fileExt, aContentType)))
          return NS_OK;
    }
  }
  *aContentType = nsCRT::strdup("unknown");
  if (!*aContentType) return NS_ERROR_OUT_OF_MEMORY;
  return NS_OK;
}

#define CONTENT_DISP "Content-Disposition: form-data; name=\""
#define FILENAME "\"; filename=\""
#define CONTENT_TYPE "Content-Type: "
#define CONTENT_ENCODING "Content-Encoding: "
#define BUFSIZE 1024
#define MULTIPART "multipart/form-data"
#define SEP "--"

nsresult nsFormFrame::ProcessAsMultipart(nsIFormProcessor* aFormProcessor,nsIFileSpec*& aMultipartDataFile, nsIFormControlFrame* aFrame)
{
  char buffer[BUFSIZE];
  PRInt32 numChildren = mFormControls.Count();

  // Create a temporary file to write the form post data to
  nsSpecialSystemDirectory tempDir(nsSpecialSystemDirectory::OS_TemporaryDirectory);
  tempDir += "formpost";
  tempDir.MakeUnique();
  nsIFileSpec* postDataFile = nsnull;
  nsresult rv = NS_NewFileSpecWithSpec(tempDir, &postDataFile);
  NS_ASSERTION(NS_SUCCEEDED(rv), "Post data file couldn't be created!");
  if (NS_FAILED(rv)) return rv;

  // write the content-type, boundary to the tmp file
  char boundary[80];
  sprintf(boundary, "---------------------------%d%d%d", 
          rand(), rand(), rand());
  sprintf(buffer, "Content-type: %s; boundary=%s" CRLF, MULTIPART, boundary);
  PRInt32 wantbytes = 0, gotbytes = 0;
  rv = postDataFile->Write(buffer, wantbytes = PL_strlen(buffer), &gotbytes);
  if (NS_FAILED(rv) || (wantbytes != gotbytes)) return rv;

  nsIUnicodeEncoder *encoder=nsnull;
  if(NS_FAILED( GetEncoder(&encoder) ) )
     encoder = nsnull;

  PRInt32 boundaryLen = PL_strlen(boundary);
  PRInt32 contDispLen = PL_strlen(CONTENT_DISP);
  PRInt32 crlfLen     = PL_strlen(CRLF);
  PRInt32 sepLen      = PL_strlen(SEP);

  // compute the content length
  /////////////////////////////

  PRInt32 contentLen = 0; // extra crlf after content-length header not counted
  PRInt32 childX;  // stupid compiler
  for (childX = 0; childX < numChildren; childX++) {
    nsIFormControlFrame* child = (nsIFormControlFrame*) mFormControls.ElementAt(childX);
    if (child) {
      PRInt32 type;
      child->GetType(&type);
      if (child->IsSuccessful(aFrame)) {
        PRInt32 numValues = 0;
        PRInt32 maxNumValues = child->GetMaxNumValues();
        if (maxNumValues <= 0) {
          continue;
        }
        nsString* names  = new nsString[maxNumValues];
        nsString* values = new nsString[maxNumValues];
        if (PR_FALSE == child->GetNamesValues(maxNumValues, numValues, values, names)) {
          continue;
        }
        for (int valueX = 0; valueX < numValues; valueX++) {
          char* name = nsnull;
          char* value = nsnull;

          nsString valueStr = values[valueX];
          if (aFormProcessor) {
            ProcessValue(*aFormProcessor, child, names[valueX], valueStr);
          }
   
          if(encoder) {
              name  = UnicodeToNewBytes(names[valueX].GetUnicode(), names[valueX].Length(), encoder);
              value  = UnicodeToNewBytes(valueStr.GetUnicode(), valueStr.Length(), encoder);
          }

          if(nsnull == name)
            name  = names[valueX].ToNewCString();
          if(nsnull == value)
            value = valueStr.ToNewCString();

          if ((0 == names[valueX].Length()) || (0 == valueStr.Length())) {
            continue;
          }

          // convert value to CRLF line breaks
          char* newValue = nsLinebreakConverter::ConvertLineBreaks(value,
                           nsLinebreakConverter::eLinebreakPlatform, nsLinebreakConverter::eLinebreakNet);
          delete [] value;
          value = newValue;
          
          // Add boundary line
          contentLen += sepLen + boundaryLen + crlfLen;

          // Add Content-Disp line
          contentLen += contDispLen;
          contentLen += PL_strlen(name);

          // File inputs also list filename on Content-Disp line
          if (NS_FORM_INPUT_FILE == type) { 
            contentLen += PL_strlen(FILENAME);
            const char* fileNameStart = GetFileNameWithinPath(value);
	          contentLen += PL_strlen(fileNameStart);
          }
          // End Content-Disp Line (quote plus CRLF)
          contentLen += 1 + crlfLen;  // ending name quote plus CRLF

          // File inputs add Content-Type line
          if (NS_FORM_INPUT_FILE == type) {
            char* contentType = nsnull;
            rv = GetContentType(value, &contentType);
            if (NS_FAILED(rv)) break; // Need to free up anything here?
            contentLen += PL_strlen(CONTENT_TYPE);
            contentLen += PL_strlen(contentType) + crlfLen;
            nsCRT::free(contentType);
          }
	  
          // Blank line before value
          contentLen += crlfLen;

          // File inputs add file contents next
          if (NS_FORM_INPUT_FILE == type) {
            do {
              // Because we have a native path to the file we can't use PR_GetFileInfo
              // on the Mac as it expects a Unix style path.  Instead we'll use our
              // spiffy new nsILocalFile
              nsILocalFile* tempFile = nsnull;
              rv = NS_NewLocalFile(value, &tempFile);
              NS_ASSERTION(tempFile, "Couldn't create nsIFileSpec to get file size!");
              if (NS_FAILED(rv) || !tempFile)
                break; // NS_ERROR_OUT_OF_MEMORY
              PRUint32 fileSize32 = 0;
              PRInt64  fileSize = LL_Zero();
              rv = tempFile->GetFileSize(&fileSize);
              if (NS_FAILED(rv)) {
                NS_RELEASE(tempFile);
                break;
              }
              LL_L2UI(fileSize32, fileSize);
              contentLen += fileSize32;
              NS_RELEASE(tempFile);
            } while (PR_FALSE);

            // Add CRLF after file
            contentLen += crlfLen;
          } else {
            // Non-file inputs add value line
            contentLen += PL_strlen(value) + crlfLen;
          }
          delete [] name;
          nsAllocator::Free(value);
        }
        delete [] names;
        delete [] values;
      }

      aMultipartDataFile = postDataFile;
    }
  }

  // Add the post file boundary line
  contentLen += sepLen + boundaryLen + sepLen + crlfLen;

  // write the content 
  ////////////////////

  sprintf(buffer, "Content-Length: %d" CRLF CRLF, contentLen);
  rv = postDataFile->Write(buffer, wantbytes = PL_strlen(buffer), &gotbytes);
  if (NS_SUCCEEDED(rv) && (wantbytes == gotbytes)) {

    // write the content passing through all of the form controls a 2nd time
    for (childX = 0; childX < numChildren; childX++) {
      nsIFormControlFrame* child = (nsIFormControlFrame*) mFormControls.ElementAt(childX);
      if (child) {
        PRInt32 type;
        child->GetType(&type);
        if (child->IsSuccessful(aFrame)) {
          PRInt32 numValues = 0;
          PRInt32 maxNumValues = child->GetMaxNumValues();
          if (maxNumValues <= 0) {
            continue;
          }
          nsString* names  = new nsString[maxNumValues];
          nsString* values = new nsString[maxNumValues];
            if (PR_FALSE == child->GetNamesValues(maxNumValues, numValues, values, names)) {
            continue;
          }
          for (int valueX = 0; valueX < numValues; valueX++) {
            char* name = nsnull;
            char* value = nsnull;

            if(encoder) {
              name  = UnicodeToNewBytes(names[valueX].GetUnicode(), names[valueX].Length(), encoder);
              value = UnicodeToNewBytes(values[valueX].GetUnicode(), values[valueX].Length(), encoder);
            }

            if(nsnull == name)
              name  = names[valueX].ToNewCString();
            if(nsnull == value)
              value = values[valueX].ToNewCString();

            if ((0 == names[valueX].Length()) || (0 == values[valueX].Length())) {
              continue;
            }

            // convert value to CRLF line breaks
          // convert value to CRLF line breaks
            char* newValue = nsLinebreakConverter::ConvertLineBreaks(value,
                             nsLinebreakConverter::eLinebreakPlatform, nsLinebreakConverter::eLinebreakNet);
            delete [] value;
            value = newValue;

	    // Print boundary line
            sprintf(buffer, SEP "%s" CRLF, boundary);
            rv = postDataFile->Write(buffer, wantbytes = PL_strlen(buffer), &gotbytes);
            if (NS_FAILED(rv) || (wantbytes != gotbytes)) break;

            // Print Content-Disp line
            rv = postDataFile->Write(CONTENT_DISP, wantbytes = contDispLen, &gotbytes);
            if (NS_FAILED(rv) || (wantbytes != gotbytes)) break;	  
            rv = postDataFile->Write(name, wantbytes = PL_strlen(name), &gotbytes);
            if (NS_FAILED(rv) || (wantbytes != gotbytes)) break;

            // File inputs also list filename on Content-Disp line
            if (NS_FORM_INPUT_FILE == type) {
              rv = postDataFile->Write(FILENAME, wantbytes = PL_strlen(FILENAME), &gotbytes);
              if (NS_FAILED(rv) || (wantbytes != gotbytes)) break;
              const char* fileNameStart = GetFileNameWithinPath(value);
              rv = postDataFile->Write(fileNameStart, wantbytes = PL_strlen(fileNameStart), &gotbytes);
              if (NS_FAILED(rv) || (wantbytes != gotbytes)) break;
            }

            // End Content Disp
            rv = postDataFile->Write("\"" CRLF , wantbytes = PL_strlen("\"" CRLF), &gotbytes);
            if (NS_FAILED(rv) || (wantbytes != gotbytes)) break;

            // File inputs write Content-Type line
            if (NS_FORM_INPUT_FILE == type) {
              char* contentType = nsnull;
              rv = GetContentType(value, &contentType);
              if (NS_FAILED(rv)) break;
              rv = postDataFile->Write(CONTENT_TYPE, wantbytes = PL_strlen(CONTENT_TYPE), &gotbytes);
              if (NS_FAILED(rv) || (wantbytes != gotbytes)) break;
              rv = postDataFile->Write(contentType, wantbytes = PL_strlen(contentType), &gotbytes);
              nsCRT::free(contentType);
              if (NS_FAILED(rv) || (wantbytes != gotbytes)) break;
              rv = postDataFile->Write(CRLF, wantbytes = PL_strlen(CRLF), &gotbytes);
              if (NS_FAILED(rv) || (wantbytes != gotbytes)) break;
              // end content-type header
	          }

            // Blank line before value
            rv = postDataFile->Write(CRLF, wantbytes = PL_strlen(CRLF), &gotbytes);
            if (NS_FAILED(rv) || (wantbytes != gotbytes)) break;

            // File inputs print file contents next
            if (NS_FORM_INPUT_FILE == type) {
              nsIFileSpec* contentFile = nsnull;
              rv = NS_NewFileSpec(&contentFile);
              NS_ASSERTION(contentFile, "Post content file couldn't be created!");
              if (NS_FAILED(rv) || !contentFile) break; // NS_ERROR_OUT_OF_MEMORY
              rv = contentFile->SetNativePath(value);
              NS_ASSERTION(contentFile, "Post content file path couldn't be set!");
              if (NS_FAILED(rv)) {
                NS_RELEASE(contentFile);
                break;
              }
              // Print file contents
              PRInt32 size = 1;
              while (1) {
                char* readbuffer = nsnull;
                rv = contentFile->Read(&readbuffer, BUFSIZE, &size);
                if (NS_FAILED(rv) || 0 >= size) break;
                rv = postDataFile->Write(readbuffer, wantbytes = size, &gotbytes);
                if (NS_FAILED(rv) || (wantbytes != gotbytes)) break;
              }
              NS_RELEASE(contentFile);
              // Print CRLF after file
              rv = postDataFile->Write(CRLF, wantbytes = PL_strlen(CRLF), &gotbytes);
              if (NS_FAILED(rv) || (wantbytes != gotbytes)) break;
            }

            // Non-file inputs print value line
            else {
              rv = postDataFile->Write(value, wantbytes = PL_strlen(value), &gotbytes);
              if (NS_FAILED(rv) || (wantbytes != gotbytes)) break;
              rv = postDataFile->Write(CRLF, wantbytes = PL_strlen(CRLF), &gotbytes);
              if (NS_FAILED(rv) || (wantbytes != gotbytes)) break;
            }
            delete [] name;
            nsAllocator::Free(value);
          }
          delete [] names;
          delete [] values;
        }
      }
    }
  }

  if (NS_SUCCEEDED(rv)) {
    sprintf(buffer, SEP "%s" SEP CRLF, boundary);
    rv = postDataFile->Write(buffer, wantbytes = PL_strlen(buffer), &gotbytes);
    if (NS_SUCCEEDED(rv) && (wantbytes == gotbytes)) {
      rv = postDataFile->CloseStream();
    }
  }

  NS_ASSERTION(NS_SUCCEEDED(rv), "Generating the form post temp file failed.\n");
  NS_IF_RELEASE(encoder);
  return rv;
}

// static helper functions for nsIFormControls

PRBool
nsFormFrame::GetDisabled(nsIFrame* aChildFrame, nsIContent* aContent) 
{
  PRBool result = PR_FALSE;

  nsIContent* content = aContent;
  if (nsnull == content) {
    aChildFrame->GetContent(&content);
  }
  if (nsnull != content) {
    nsIHTMLContent* htmlContent = nsnull;
    content->QueryInterface(kIHTMLContentIID, (void**)&htmlContent);
    if (nsnull != htmlContent) {
      nsHTMLValue value;
      if (NS_CONTENT_ATTR_HAS_VALUE == htmlContent->GetHTMLAttribute(nsHTMLAtoms::disabled, value)) {
        result = PR_TRUE;
      }
      NS_RELEASE(htmlContent);
    }
    if (nsnull == aContent) {
      NS_RELEASE(content);
    }
  }
  return result;
}

PRBool
nsFormFrame::GetReadonly(nsIFrame* aChildFrame, nsIContent* aContent) 
{
  PRBool result = PR_FALSE;

  nsIContent* content = aContent;
  if (nsnull == content) {
    aChildFrame->GetContent(&content);
  }
  if (nsnull != content) {
    nsIHTMLContent* htmlContent = nsnull;
    content->QueryInterface(kIHTMLContentIID, (void**)&htmlContent);
    if (nsnull != htmlContent) {
      nsHTMLValue value;
      if (NS_CONTENT_ATTR_HAS_VALUE == htmlContent->GetHTMLAttribute(nsHTMLAtoms::readonly, value)) {
        result = PR_TRUE;
      }
      NS_RELEASE(htmlContent);
    }
    if (nsnull == aContent) {
      NS_RELEASE(content);
    }
  }
  return result;
}

nsresult
nsFormFrame::GetName(nsIFrame* aChildFrame, nsString& aName, nsIContent* aContent) 
{
  nsresult result = NS_FORM_NOTOK;

  nsIContent* content = aContent;
  if (nsnull == content) {
    aChildFrame->GetContent(&content);
  }
  if (nsnull != content) {
    nsIHTMLContent* htmlContent = nsnull;
    result = content->QueryInterface(kIHTMLContentIID, (void**)&htmlContent);
    if (NS_SUCCEEDED(result) && (nsnull != htmlContent)) {
      nsHTMLValue value;
      result = htmlContent->GetHTMLAttribute(nsHTMLAtoms::name, value);
      if (NS_CONTENT_ATTR_HAS_VALUE == result) {
        if (eHTMLUnit_String == value.GetUnit()) {
          value.GetStringValue(aName);
        }
      }
      NS_RELEASE(htmlContent);
    }
    if (nsnull == aContent) {
      NS_RELEASE(content);
    }
  }
  return result;
}

nsresult
nsFormFrame::GetValue(nsIFrame* aChildFrame, nsString& aValue, nsIContent* aContent) 
{
  nsresult result = NS_FORM_NOTOK;

  nsIContent* content = aContent;
  if (nsnull == content) {
    aChildFrame->GetContent(&content);
  }
  if (nsnull != content) {
    nsIHTMLContent* htmlContent = nsnull;
    result = content->QueryInterface(kIHTMLContentIID, (void**)&htmlContent);
    if (NS_SUCCEEDED(result) && (nsnull != htmlContent)) {
      nsHTMLValue value;
      result = htmlContent->GetHTMLAttribute(nsHTMLAtoms::value, value);
      if (NS_CONTENT_ATTR_HAS_VALUE == result) {
        if (eHTMLUnit_String == value.GetUnit()) {
          value.GetStringValue(aValue);
        }
      }
      NS_RELEASE(htmlContent);
    }
    if (nsnull == aContent) {
      NS_RELEASE(content);
    }
  }
  return result;
}

void
nsFormFrame::StyleChangeReflow(nsIPresContext* aPresContext,
                               nsIFrame* aFrame)
{
  nsCOMPtr<nsIPresShell> shell;
  aPresContext->GetShell(getter_AddRefs(shell));
    
  nsIReflowCommand* reflowCmd;
  nsresult rv = NS_NewHTMLReflowCommand(&reflowCmd, aFrame,
                                        nsIReflowCommand::StyleChanged);
  if (NS_SUCCEEDED(rv)) {
    shell->AppendReflowCommand(reflowCmd);
    NS_RELEASE(reflowCmd);
  }
}
