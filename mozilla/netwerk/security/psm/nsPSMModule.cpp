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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s):
*/

#include "nsIModule.h"
#include "nsIGenericFactory.h"

#include "nsPSMUICallbacks.h"
#include "nsPSMComponent.h"

static nsModuleComponentInfo components[] =
{
    { PSM_COMPONENT_CLASSNAME,  
      NS_PSMCOMPONENT_CID, 
      PSM_COMPONENT_PROGID,   
      nsPSMComponent::CreatePSMComponent},

    { PSM_UI_HANLDER_CLASSNAME, 
      NS_PSMUIHANDLER_CID, 
      PSM_UI_HANLDER_PROGID,  
      nsPSMUIHandlerImpl::CreatePSMUIHandler}
};

NS_IMPL_NSGETMODULE("PSMComponent", components);
