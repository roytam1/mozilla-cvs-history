/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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
 * The Original Code is mozilla.org code.
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
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "nsIModule.h"
#include "nsIGenericFactory.h"

#include "nsAboutProtocolHandler.h"
#include "nsAboutBlank.h"
#include "nsAboutBloat.h"
#include "nsAboutCredits.h"
#include "mzAboutMozilla.h"

static const nsModuleComponentInfo components[] = 
{
   { "About Protocol Handler", 
     NS_ABOUTPROTOCOLHANDLER_CID,
     NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX "about", 
     nsAboutProtocolHandler::Create
   },

   { "about:blank", 
     NS_ABOUT_BLANK_MODULE_CID,
     NS_ABOUT_MODULE_CONTRACTID_PREFIX "blank", 
     nsAboutBlank::Create
   },
   
   { "about:bloat", 
     NS_ABOUT_BLOAT_MODULE_CID,
     NS_ABOUT_MODULE_CONTRACTID_PREFIX "bloat", 
     nsAboutBloat::Create
   },

   { "about:config",
     NS_ABOUT_CONFIG_MODULE_CID,
     NS_ABOUT_MODULE_CONTRACTID_PREFIX "config",
     nsAboutConfig::Create
   },

   { "about:credits",
     NS_ABOUT_CREDITS_MODULE_CID,
     NS_ABOUT_MODULE_CONTRACTID_PREFIX "credits",
     nsAboutCredits::Create
   },

   { "about:mozilla",
     MZ_ABOUT_MOZILLA_MODULE_CID,
     NS_ABOUT_MODULE_CONTRACTID_PREFIX "mozilla",
     mzAboutMozilla::Create
   },	     

   { "about:cache", 
     NS_ABOUT_CACHE_MODULE_CID,
     NS_ABOUT_MODULE_CONTRACTID_PREFIX "cache", 
     nsAboutCache::Create
   },
};

NS_IMPL_NSGETMODULE(nsAboutProtocolModule, components);

