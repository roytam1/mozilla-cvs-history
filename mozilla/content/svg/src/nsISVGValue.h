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


#ifndef __NS_ISVGVALUE_H__
#define __NS_ISVGVALUE_H__

#include "nsISupports.h"
#include "nsString.h"

class nsISVGValueObserver;

////////////////////////////////////////////////////////////////////////
// nsSVGValue: private interface for svg values

/* This interface is implemented by all value-types (e.g. coords,
  pointlists, matrices) that can be parsed from/to strings. This is
  used for element-properties that are also XML attributes. E.g. the
  'polyline'-element has a 'points'-attribute and a property
  'animatedPoints' in the DOM.

  XXX Observers
*/

// {79293232-F695-4bda-9FC7-C2679647B790}
#define NS_ISVGVALUE_IID \
{ 0x79293232, 0xf695, 0x4bda, { 0x9f, 0xc7, 0xc2, 0x67, 0x96, 0x47, 0xb7, 0x90 } }


class nsISVGValue : public nsISupports
{
public:
  static const nsIID& GetIID() { static nsIID iid = NS_ISVGVALUE_IID; return iid; }
  
  NS_IMETHOD SetValueString(const nsAReadableString& aValue)=0;
  NS_IMETHOD GetValueString(nsAWritableString& aValue)=0;

  NS_IMETHOD AddObserver(nsISVGValueObserver* observer)=0;
  NS_IMETHOD RemoveObserver(nsISVGValueObserver* observer)=0;
};

extern nsresult
NS_CreateSVGGenericStringValue(const nsAReadableString& aValue, nsISVGValue** aResult);

#endif // __NS_ISVGVALUE_H__

