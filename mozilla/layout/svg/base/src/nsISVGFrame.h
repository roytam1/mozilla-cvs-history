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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
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

#ifndef nsISVGFrame_h__
#define nsISVGFrame_h__

#include "nsISupports.h"

#include "libart-incs.h"

class nsSVGRenderingContext;
class nsIFrame;

#define NS_ISVGFRAME_IID \
  {0x84957983, 0x712, 0x11d4, \
    { 0x97, 0x7, 0x0, 0x60, 0xb0, 0xfb, 0x99, 0x56 }}

class nsISVGFrame : public nsISupports {
public:

  NS_DEFINE_STATIC_IID_ACCESSOR(NS_ISVGFRAME_IID)

  // down the tree
    
  NS_IMETHOD Paint(nsSVGRenderingContext* renderingContext)=0;
  NS_IMETHOD GetFrameForPoint(float x, float y, nsIFrame** hit)=0;
  NS_IMETHOD NotifyCTMChanged()=0;
  NS_IMETHOD NotifyRedrawSuspended()=0;
  NS_IMETHOD NotifyRedrawUnsuspended()=0;

  // up the tree. these should really go into a separate nsISVGContainerFrame interface  
  NS_IMETHOD InvalidateRegion(ArtUta* uta, PRBool bRedraw)=0;
  NS_IMETHOD IsRedrawSuspended(PRBool* isSuspended)=0;
};

#endif
