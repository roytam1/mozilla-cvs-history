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
 * The Original Code is the Mozilla SVG project.
 *
 * The Initial Developer of the Original Code is 
 * Crocodile Clips Ltd..
 * Portions created by the Initial Developer are Copyright (C) 2001
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *    Alex Fritze <alex.fritze@crocodile-clips.com> (original author)
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

#include "nsSVGGraphicElement.h"
#include "nsSVGAtoms.h"
#include "nsIDOMSVGForeignObjectElem.h"
#include "nsSVGLength.h"
#include "nsSVGAnimatedLength.h"
#include "nsCOMPtr.h"

typedef nsSVGGraphicElement nsSVGForeignObjectElementBase;

class nsSVGForeignObjectElement : public nsSVGForeignObjectElementBase,
                                  public nsIDOMSVGForeignObjectElement
{
protected:
  friend nsresult NS_NewSVGForeignObjectElement(nsIContent **aResult,
                                                nsINodeInfo *aNodeInfo);
  nsSVGForeignObjectElement();
  virtual ~nsSVGForeignObjectElement();
  virtual nsresult Init();
  
public:
  // interfaces:
  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGFOREIGNOBJECTELEMENT

  // xxx I wish we could use virtual inheritance
  NS_FORWARD_NSIDOMNODE_NO_CLONENODE(nsSVGForeignObjectElementBase::)
  NS_FORWARD_NSIDOMELEMENT(nsSVGForeignObjectElementBase::)
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGForeignObjectElementBase::)
  
protected:
  nsCOMPtr<nsIDOMSVGAnimatedLength> mX;
  nsCOMPtr<nsIDOMSVGAnimatedLength> mY;
  nsCOMPtr<nsIDOMSVGAnimatedLength> mWidth;
  nsCOMPtr<nsIDOMSVGAnimatedLength> mHeight;
};


nsresult NS_NewSVGForeignObjectElement(nsIContent **aResult, nsINodeInfo *aNodeInfo)
{
  *aResult = nsnull;
  nsSVGForeignObjectElement* it = new nsSVGForeignObjectElement();

  if (!it) return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(it);

  nsresult rv = NS_STATIC_CAST(nsGenericElement*,it)->Init(aNodeInfo);

  if (NS_FAILED(rv)) {
    it->Release();
    return rv;
  }

  rv = it->Init();

  if (NS_FAILED(rv)) {
    it->Release();
    return rv;
  }
  
  *aResult = NS_STATIC_CAST(nsIContent *, it);

  return NS_OK;
}

//----------------------------------------------------------------------
// nsISupports methods

NS_IMPL_ADDREF_INHERITED(nsSVGForeignObjectElement,nsSVGForeignObjectElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGForeignObjectElement,nsSVGForeignObjectElementBase)

NS_INTERFACE_MAP_BEGIN(nsSVGForeignObjectElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGForeignObjectElement)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGForeignObjectElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGForeignObjectElementBase)

//----------------------------------------------------------------------
// Implementation

nsSVGForeignObjectElement::nsSVGForeignObjectElement()
{

}

nsSVGForeignObjectElement::~nsSVGForeignObjectElement()
{
  if (mX) {
    nsCOMPtr<nsISVGValue> value = do_QueryInterface(mX);
    value->RemoveObserver(this);
  }
  if (mY) {
    nsCOMPtr<nsISVGValue> value = do_QueryInterface(mY);
    value->RemoveObserver(this);
  }
  if (mWidth) {
    nsCOMPtr<nsISVGValue> value = do_QueryInterface(mWidth);
    value->RemoveObserver(this);
  }
  if (mHeight) {
    nsCOMPtr<nsISVGValue> value = do_QueryInterface(mHeight);
    value->RemoveObserver(this);
  }
}

  
nsresult
nsSVGForeignObjectElement::Init()
{
  nsresult rv;
  rv = nsSVGForeignObjectElementBase::Init();
  NS_ENSURE_SUCCESS(rv,rv);

  // Create mapped properties:
  

  // DOM property: x ,  #IMPLIED attrib: x
  {
    nsCOMPtr<nsIDOMSVGLength> length;
    rv = NS_NewSVGLength(getter_AddRefs(length),
                         (nsSVGElement*)this, eXDirection,
                         0.0f);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = NS_NewSVGAnimatedLength(getter_AddRefs(mX), length);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = mAttributes->AddMappedSVGValue(nsSVGAtoms::x, mX);
    NS_ENSURE_SUCCESS(rv,rv);
  }

  // DOM property: y ,  #IMPLIED attrib: y
  {
    nsCOMPtr<nsIDOMSVGLength> length;
    rv = NS_NewSVGLength(getter_AddRefs(length),
                         (nsSVGElement*)this, eYDirection,
                         0.0f);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = NS_NewSVGAnimatedLength(getter_AddRefs(mY), length);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = mAttributes->AddMappedSVGValue(nsSVGAtoms::y, mY);
    NS_ENSURE_SUCCESS(rv,rv);
  }

  // DOM property: width ,  #REQUIRED  attrib: width
  // XXX: enforce requiredness
  {
    nsCOMPtr<nsIDOMSVGLength> length;
    rv = NS_NewSVGLength(getter_AddRefs(length),
                         (nsSVGElement*)this, eXDirection,
                         100.0f);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = NS_NewSVGAnimatedLength(getter_AddRefs(mWidth), length);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = mAttributes->AddMappedSVGValue(nsSVGAtoms::width, mWidth);
    NS_ENSURE_SUCCESS(rv,rv);
  }

  // DOM property: height ,  #REQUIRED  attrib: height
  // XXX: enforce requiredness
  {
    nsCOMPtr<nsIDOMSVGLength> length;
    rv = NS_NewSVGLength(getter_AddRefs(length),
                         (nsSVGElement*)this, eYDirection,
                         100.0f);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = NS_NewSVGAnimatedLength(getter_AddRefs(mHeight), length);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = mAttributes->AddMappedSVGValue(nsSVGAtoms::height, mHeight);
    NS_ENSURE_SUCCESS(rv,rv);
  }
  
    
  return NS_OK;
}

//----------------------------------------------------------------------
// nsIDOMNode methods

NS_IMETHODIMP
nsSVGForeignObjectElement::CloneNode(PRBool aDeep, nsIDOMNode** aReturn)
{
  *aReturn = nsnull;
  nsSVGForeignObjectElement* it = new nsSVGForeignObjectElement();

  if (!it) return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(it);

  nsresult rv = NS_STATIC_CAST(nsGenericElement*,it)->Init(mNodeInfo);

  if (NS_FAILED(rv)) {
    it->Release();
    return rv;
  }

  rv = it->Init();

  if (NS_FAILED(rv)) {
    it->Release();
    return rv;
  }

  rv = CopyNode(it, aDeep);

  if (NS_FAILED(rv)) {
    it->Release();
    return rv;
  }
 
  *aReturn = NS_STATIC_CAST(nsSVGForeignObjectElementBase*, it);

  return NS_OK; 
}


//----------------------------------------------------------------------
// nsIDOMSVGForeignObjectElement methods:

/* readonly attribute nsIDOMSVGAnimatedLength x; */
NS_IMETHODIMP nsSVGForeignObjectElement::GetX(nsIDOMSVGAnimatedLength * *aX)
{
  *aX = mX;
  NS_IF_ADDREF(*aX);
  return NS_OK;
}

/* readonly attribute nsIDOMSVGAnimatedLength y; */
NS_IMETHODIMP nsSVGForeignObjectElement::GetY(nsIDOMSVGAnimatedLength * *aY)
{
  *aY = mY;
  NS_IF_ADDREF(*aY);
  return NS_OK;
}

/* readonly attribute nsIDOMSVGAnimatedLength width; */
NS_IMETHODIMP nsSVGForeignObjectElement::GetWidth(nsIDOMSVGAnimatedLength * *aWidth)
{
  *aWidth = mWidth;
  NS_IF_ADDREF(*aWidth);
  return NS_OK;
}

/* readonly attribute nsIDOMSVGAnimatedLength height; */
NS_IMETHODIMP nsSVGForeignObjectElement::GetHeight(nsIDOMSVGAnimatedLength * *aHeight)
{
  *aHeight = mHeight;
  NS_IF_ADDREF(*aHeight);
  return NS_OK;
}

