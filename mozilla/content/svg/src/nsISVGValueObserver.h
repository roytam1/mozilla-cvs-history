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


#ifndef __NS_ISVGVALUEOBSERVER_H__
#define __NS_ISVGVALUEOBSERVER_H__

#include "nsISupports.h"

class nsISVGValue;

////////////////////////////////////////////////////////////////////////
// nsISVGValueObserver

/*
  Implementors of this interface also need to implement
  nsISupportsWeakReference so that svg-values can store safe owning
  refs.
*/

// {33E46ADB-9AA4-4903-9EDE-699FAE1107D8}
#define NS_ISVGVALUEOBSERVER_IID \
{ 0x33e46adb, 0x9aa4, 0x4903, { 0x9e, 0xde, 0x69, 0x9f, 0xae, 0x11, 0x7, 0xd8 } }


class nsISVGValueObserver : public nsISupports
{
public:
  static const nsIID& GetIID() { static nsIID iid = NS_ISVGVALUEOBSERVER_IID; return iid; }
  
  NS_IMETHOD WillModifySVGObservable(nsISVGValue* observable)=0;
  NS_IMETHOD DidModifySVGObservable (nsISVGValue* observable)=0;
};

#endif // __NS_ISVGVALUEOBSERVER_H__

