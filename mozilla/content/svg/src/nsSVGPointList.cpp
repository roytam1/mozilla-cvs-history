/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is the Mozilla SVG project.
 *
 * The Initial Developer of the Original Code is Crocodile Clips Ltd.
 * Portions created by Crocodile Clips are 
 * Copyright (C) 2001 Crocodile Clips Ltd. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *
 *          Alex Fritze <alex.fritze@crocodile-clips.com>
 *
 */

#include "nsSVGPointList.h"
#include "nsSVGPoint.h"
#include "nsDOMError.h"
#include "prdtoa.h"

nsresult
nsSVGPointList::Create(const nsAReadableString& aValue,
                       nsISVGValue** aResult)
{
  *aResult = (nsISVGValue*) new nsSVGPointList();
  if(!*aResult) return NS_ERROR_OUT_OF_MEMORY;
  
  NS_ADDREF(*aResult);

  (*aResult)->SetValueString(aValue);  
  return NS_OK;
}

nsresult
nsSVGPointList::Create(nsIDOMSVGPointList** aResult)
{
  *aResult = (nsIDOMSVGPointList*) new nsSVGPointList();
  if(!*aResult) return NS_ERROR_OUT_OF_MEMORY;
  
  NS_ADDREF(*aResult);
  return NS_OK;
}

nsSVGPointList::nsSVGPointList()
{
  NS_INIT_ISUPPORTS();
}

nsSVGPointList::~nsSVGPointList()
{
  ReleasePoints();
}

void
nsSVGPointList::ReleasePoints()
{
  WillModify();
  PRInt32 count = mPoints.Count();
  for (PRInt32 i = 0; i < count; ++i) {
    nsIDOMSVGPoint* point = ElementAt(i);
    nsCOMPtr<nsISVGValue> val = do_QueryInterface(point);
    if (val)
      val->RemoveObserver(this);
    NS_RELEASE(point);
  }
  mPoints.Clear();
  DidModify();
}

nsIDOMSVGPoint*
nsSVGPointList::ElementAt(PRInt32 index)
{
  return (nsIDOMSVGPoint*)mPoints.ElementAt(index);
}

void
nsSVGPointList::AppendElement(nsIDOMSVGPoint* aElement)
{
  WillModify();
  NS_ADDREF(aElement);
  mPoints.AppendElement((void*)aElement);
  nsCOMPtr<nsISVGValue> val = do_QueryInterface(aElement);
  if (val)
    val->AddObserver(this);
  DidModify();
}

void
nsSVGPointList::RemoveElementAt(PRInt32 index)
{
  WillModify();
  nsIDOMSVGPoint* point = ElementAt(index);
  NS_ASSERTION(point, "null point");
  nsCOMPtr<nsISVGValue> val = do_QueryInterface(point);
  if (val)
    val->RemoveObserver(this);
  mPoints.RemoveElementAt(index);
  NS_RELEASE(point);
  DidModify();
}

void
nsSVGPointList::InsertElementAt(nsIDOMSVGPoint* aElement, PRInt32 index)
{
  WillModify();
  NS_ADDREF(aElement);
  mPoints.InsertElementAt((void*)aElement, index);
  nsCOMPtr<nsISVGValue> val = do_QueryInterface(aElement);
  if (val)
    val->AddObserver(this);
  DidModify();
}

//----------------------------------------------------------------------
// XPConnect interface list
NS_CLASSINFO_MAP_BEGIN(SVGPointList)
  NS_CLASSINFO_MAP_ENTRY(nsIDOMSVGPointList)
NS_CLASSINFO_MAP_END

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_ADDREF(nsSVGPointList)
NS_IMPL_RELEASE(nsSVGPointList)

NS_INTERFACE_MAP_BEGIN(nsSVGPointList)
  NS_INTERFACE_MAP_ENTRY(nsISVGValue)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGPointList)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY(nsISVGValueObserver)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGPointList)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsISVGValue)
NS_INTERFACE_MAP_END


//----------------------------------------------------------------------
// nsISVGValue methods:

NS_IMETHODIMP
nsSVGPointList::SetValueString(const nsAReadableString& aValue)
{
  WillModify();
  
  ReleasePoints();

  nsresult rv = NS_OK;

  // XXX how am I supposed to do this ??? 
  // char* str  = aValue.ToNewCString();
  char* str;
  {
    nsAutoString temp(aValue);
    str = temp.ToNewCString();
  }
  
  char* rest = str;
  char* token1;
  char* token2;
  const char* delimiters = ",\x20\x9\xD\xA";

  while ( (token1 = nsCRT::strtok(rest, delimiters, &rest)) &&
          (token2 = nsCRT::strtok(rest, delimiters, &rest)) ) {

    char *end;
    
    double x = PR_strtod(token1, &end);
    if (*end != '\0') break; // parse error
    
    double y = PR_strtod(token2, &end);
    if (*end != '\0') break; // parse error

    nsCOMPtr<nsIDOMSVGPoint> point;
    nsSVGPoint::Create((float)x, (float)y, getter_AddRefs(point));
    if (!point) {
      rv = NS_ERROR_OUT_OF_MEMORY;
      break;
    }
    AppendElement(point);
  }

  if (token1) {
    // there was a parse error. should we return an error?
    // rv = NS_ERROR_???;
  }

  nsMemory::Free(str);
  
  DidModify();
  return rv;
}

NS_IMETHODIMP
nsSVGPointList::GetValueString(nsAWritableString& aValue)
{
  aValue.Truncate();

  PRInt32 count = mPoints.Count();

  if (count<=0) return NS_OK;

  PRInt32 i = 0;
  char buf[80];
  
  while (1) {
    nsIDOMSVGPoint* point = ElementAt(i);
    float x, y;
    point->GetX(&x);
    point->GetY(&y);
    
    sprintf(buf, "%g,%g", (double)x, (double)y);
    aValue.Append(NS_ConvertASCIItoUCS2(buf));

    if (++i >= count) break;

    aValue.Append(NS_LITERAL_STRING(" "));
  }
  
  return NS_OK;
}

//----------------------------------------------------------------------
// nsIDOMSVGPointList methods:

/* readonly attribute unsigned long numberOfItems; */
NS_IMETHODIMP nsSVGPointList::GetNumberOfItems(PRUint32 *aNumberOfItems)
{
  *aNumberOfItems = mPoints.Count();
  return NS_OK;
}

/* void clear (); */
NS_IMETHODIMP nsSVGPointList::Clear()
{
  WillModify();
  ReleasePoints();
  DidModify();
  return NS_OK;
}

/* nsIDOMSVGPoint initialize (in nsIDOMSVGPoint newItem); */
NS_IMETHODIMP nsSVGPointList::Initialize(nsIDOMSVGPoint *newItem, nsIDOMSVGPoint **_retval)
{
  Clear();
  return AppendItem(newItem, _retval);
}

/* nsIDOMSVGPoint getItem (in unsigned long index); */
NS_IMETHODIMP nsSVGPointList::GetItem(PRUint32 index, nsIDOMSVGPoint **_retval)
{
  if ((PRInt32)index >= mPoints.Count()) {
    *_retval = nsnull;
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  *_retval  = ElementAt(index);
  NS_ADDREF(*_retval);
  return NS_OK;
}

/* nsIDOMSVGPoint insertItemBefore (in nsIDOMSVGPoint newItem, in unsigned long index); */
NS_IMETHODIMP nsSVGPointList::InsertItemBefore(nsIDOMSVGPoint *newItem, PRUint32 index, nsIDOMSVGPoint **_retval)
{
  NS_NOTYETIMPLEMENTED("write me");
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* nsIDOMSVGPoint replaceItem (in nsIDOMSVGPoint newItem, in unsigned long index); */
NS_IMETHODIMP nsSVGPointList::ReplaceItem(nsIDOMSVGPoint *newItem, PRUint32 index, nsIDOMSVGPoint **_retval)
{
  NS_NOTYETIMPLEMENTED("write me");
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* nsIDOMSVGPoint removeItem (in unsigned long index); */
NS_IMETHODIMP nsSVGPointList::RemoveItem(PRUint32 index, nsIDOMSVGPoint **_retval)
{
  if ((PRInt32)index >= mPoints.Count()) {
    *_retval = nsnull;
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  *_retval = ElementAt(index);
  NS_ADDREF(*_retval);
  WillModify();
  RemoveElementAt(index);
  DidModify();
  return NS_OK;
}

/* nsIDOMSVGPoint appendItem (in nsIDOMSVGPoint newItem); */
NS_IMETHODIMP nsSVGPointList::AppendItem(nsIDOMSVGPoint *newItem, nsIDOMSVGPoint **_retval)
{
  // XXX The SVG specs state that 'if newItem is already in a list, it
  // is removed from its previous list before it is inserted into this
  // list'. We don't do that. Should we?
  
  *_retval = newItem;
  NS_ADDREF(*_retval);
  AppendElement(newItem);
  return NS_OK;
}

//----------------------------------------------------------------------
// nsISVGValueObserver methods

NS_IMETHODIMP
nsSVGPointList::WillModifySVGObservable(nsISVGValue* observable)
{
  WillModify();
  return NS_OK;
}

NS_IMETHODIMP
nsSVGPointList::DidModifySVGObservable (nsISVGValue* observable)
{
  DidModify();
  return NS_OK;
}
