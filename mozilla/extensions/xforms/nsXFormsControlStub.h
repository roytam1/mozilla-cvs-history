/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla XForms support.
 *
 * The Initial Developer of the Original Code is
 * Novell, Inc.
 * Portions created by the Initial Developer are Copyright (C) 2004
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Allan Beaufour <abeaufour@novell.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef nsXFormsControlStub_h_
#define nsXFormsControlStub_h_

#include "nsArray.h"
#include "nsCOMPtr.h"
#include "nsString.h"

#include "nsIDOMElement.h"
#include "nsIDOMNode.h"
#include "nsIXTFElement.h"
#include "nsIDOMEventListener.h"

#include "nsIModelElementPrivate.h"
#include "nsIXFormsControl.h"
#include "nsXFormsStubElement.h"
#include "nsXFormsUtils.h"

class nsIDOMEvent;
class nsIDOMXPathResult;
class nsIXTFXMLVisualWrapper;

/**
 * Common stub for all XForms controls that inherit from nsIXFormsControl and
 * is bound to an instance node.
 *
 * It also inherits from nsXFormsXMLVisualStub, and overrides a couple of its
 * functions.
 *
 * @bug If a control has a model attribute, but no binding attributes we fail
 *      to set this as the context for children. We need to return the contextnode
 *      from EvaluateNodeBinding in that case, and return that in GetContext(). (XXX)
 *      @see http://bugzilla.mozilla.org/show_bug.cgi?id=280366
 */
class nsXFormsControlStub : public nsIXFormsControl,
                            public nsXFormsXMLVisualStub
{
public:
  NS_DECL_ISUPPORTS_INHERITED

  /** The standard notification flags set on nsIXTFElement */
  const PRUint32 kStandardNotificationMask;
  /**
   * The element flags for the controls passed to
   * nsXFormsUtils:EvaluateNodeBinding()
   */
  const PRUint32 kElementFlags;

  // nsIXFormsControl
  NS_IMETHOD GetBoundNode(nsIDOMNode **aBoundNode);
  NS_IMETHOD GetDependencies(nsIArray **aDependencies);
  NS_IMETHOD GetElement(nsIDOMElement **aElement);
  NS_IMETHOD ResetBoundNode();
  NS_IMETHOD Bind();
  NS_IMETHOD TryFocus(PRBool* aOK);

  // nsIXTFXMLVisual overrides
  /** This sets the notification mask and initializes mElement */
  NS_IMETHOD OnCreated(nsIXTFXMLVisualWrapper *aWrapper);

  // nsIXTFElement overrides
  NS_IMETHOD HandleDefault(nsIDOMEvent *aEvent,
                           PRBool      *aHandled);
  NS_IMETHOD OnDestroyed();
  NS_IMETHOD DocumentChanged(nsIDOMDocument *aNewDocument);
  NS_IMETHOD ParentChanged(nsIDOMElement *aNewParent);
  NS_IMETHOD WillSetAttribute(nsIAtom *aName, const nsAString &aValue);
  NS_IMETHOD AttributeSet(nsIAtom *aName, const nsAString &aValue);

  // nsIXFormsContextControl
  NS_DECL_NSIXFORMSCONTEXTCONTROL
  
  /** Constructor */
  nsXFormsControlStub() :
    kStandardNotificationMask(nsIXTFElement::NOTIFY_WILL_SET_ATTRIBUTE |
                              nsIXTFElement::NOTIFY_ATTRIBUTE_SET |
                              nsIXTFElement::NOTIFY_DOCUMENT_CHANGED |
                              nsIXTFElement::NOTIFY_PARENT_CHANGED |
                              nsIXTFElement::NOTIFY_HANDLE_DEFAULT),
    kElementFlags(nsXFormsUtils::ELEMENT_WITH_MODEL_ATTR)
    {};

protected:
  /** The nsIXTFXMLVisualWrapper */
  nsCOMPtr<nsIDOMElement>   mElement;

  /**
   * The node that the controls is bound to.
   *
   * @note This needs to be set by the control. nsXFormsControlStub does not
   * set it, it just uses it.
   */
  nsCOMPtr<nsIDOMNode>      mBoundNode;

  /**
   * Array of nsIDOMNodes that the controls depends on.
   *
   * @note This needs to be set by the control. nsXFormsControlStub does not
   * set it, it just uses it.
   */
  nsCOMPtr<nsIMutableArray> mDependencies;

  /** The model for the control */
  nsCOMPtr<nsIModelElementPrivate> mModel;

  /** This event listener is used to create xforms-hint and xforms-help events. */
  nsCOMPtr<nsIDOMEventListener> mEventListener;

  /** Returns the read only state of the control (ie. mBoundNode) */
  PRBool GetReadOnlyState();
  
  /** Returns the relevant state of the control */
  PRBool GetRelevantState();

  /**
   * Processes the node binding of a control, get the current MDG (mMDG) and
   * binds the control to its model.
   */
  nsresult
    ProcessNodeBinding(const nsString          &aBindingAttr,
                       PRUint16                 aResultType,
                       nsIDOMXPathResult      **aResult,
                       nsIModelElementPrivate **aModel = nsnull);
  
  /**
   * Toggles a property on the control (readonly, relevant, etc.)
   *
   * @todo This needs to be implemented using pseudo-classes instead of
   *       attributes (XXX)
   */
  void ToggleProperty(const nsAString &aOn,
                      const nsAString &aOff);

  /**
   *  Reset (and reinitialize) the event listener, which is used to create 
   *  xforms-hint and xforms-help events.
   */
  void ResetHelpAndHint(PRBool aInitialize);
};

#endif
