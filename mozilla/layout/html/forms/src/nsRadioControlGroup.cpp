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

#include "nsRadioControlGroup.h"

nsRadioControlGroup::nsRadioControlGroup(nsString& aName)
:mName(aName), mCheckedRadio(nsnull)
{
}

nsRadioControlGroup::~nsRadioControlGroup()
{
  mCheckedRadio = nsnull;
}
  
PRInt32 
nsRadioControlGroup::GetRadioCount() const 
{ 
  return mRadios.Count(); 
}

nsGfxRadioControlFrame*
nsRadioControlGroup::GetRadioAt(PRInt32 aIndex) const 
{ 
  return (nsGfxRadioControlFrame*) mRadios.ElementAt(aIndex);
}

PRBool 
nsRadioControlGroup::AddRadio(nsGfxRadioControlFrame* aRadio) 
{ 
  return mRadios.AppendElement(aRadio);
}

PRBool 
nsRadioControlGroup::RemoveRadio(nsGfxRadioControlFrame* aRadio) 
{ 
  if (aRadio == mCheckedRadio) {
    mCheckedRadio = nsnull;
  }
  return mRadios.RemoveElement(aRadio);
}

nsGfxRadioControlFrame*
nsRadioControlGroup::GetCheckedRadio()
{
  return mCheckedRadio;
}

void    
nsRadioControlGroup::SetCheckedRadio(nsGfxRadioControlFrame* aRadio)
{
  mCheckedRadio = aRadio;
}

void
nsRadioControlGroup::GetName(nsString& aNameResult) const
{
  aNameResult = mName;
}
