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
 * Roland Mainz <roland.mainz@informatik.med.uni-giessen.de>
 *
 */

#ifndef nsIDeviceContextSpecXP_h___
#define nsIDeviceContextSpecXP_h___

#include "nsISupports.h"

/* make Xprint the default print system if user/admin has set the XPSERVERLIST"
 * env var. See Xprt config README (/usr/openwin/server/etc/XpConfig/README) 
 * for details.
 */
#ifdef DEBUG_gisburn
#define NS_DEFAULT_PRINT_METHOD ((PR_GetEnv("XPSERVERLIST")!=nsnull)?(puts("** using printing=Xprint"),1):(puts("** using printing=PostScript"),0))
#else 
#define NS_DEFAULT_PRINT_METHOD ((PR_GetEnv("XPSERVERLIST")!=nsnull)?(1):(0))
#endif /* DEBUG_gisburn */

#define NS_IDEVICE_CONTEXT_SPEC_XP_IID { 0xa4ef8910, 0xdd65, 0x11d2, { 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 } }

class nsIDeviceContextSpecXp : public nsISupports
{

public:
  NS_DEFINE_STATIC_IID_ACCESSOR(NS_IDEVICE_CONTEXT_SPEC_XP_IID);

  /*
   * If PR_TRUE, print to printer  
   * @update 
   * @param aToPrinter --  
   * @return 
   **/
   NS_IMETHOD GetToPrinter( PRBool &aToPrinter ) = 0; 

  /*
   * If PR_TRUE, first page first 
   * @update 
   * @param aFpf -- 
   * @return 
   **/
   NS_IMETHOD GetFirstPageFirst (  PRBool &aFpf ) = 0;     

  /*
   * If PR_TRUE, print grayscale 
   * @update 
   * @param aGrayscale --
   * @return 
   **/
   NS_IMETHOD GetGrayscale(  PRBool &aGrayscale ) = 0;   

  /*
   * Paper size e.g., NS_LETTER_SIZE 
   * @update 
   * @param aSize --
   * @return 
   **/
   NS_IMETHOD GetSize (  int &aSize ) = 0; 

  /*
   * Top margin 
   * @update 
   * @param aValue --
   * @return 
   **/
   NS_IMETHOD GetTopMargin (  float &aValue ) = 0; 

  /*
   * Bottom margin 
   * @update 
   * @param aValue --
   * @return 
   **/
   NS_IMETHOD GetBottomMargin (  float &aValue ) = 0; 

  /*
   * Left margin 
   * @update 
   * @param aValue --
   * @return 
   **/
   NS_IMETHOD GetLeftMargin (  float &aValue ) = 0; 

  /*
   * Right margin 
   * @update 
   * @param aValue --
   * @return 
   **/
   NS_IMETHOD GetRightMargin (  float &aValue ) = 0; 

  /*
   * Print command e.g., lpr 
   * @update 
   * @param aCommand --
   * @return 
   **/
   NS_IMETHOD GetCommand (  char **aCommand ) = 0;   

  /*
   * Get width and height based on user page size choice, e.g., 8.5 x 11.0 
   * @update 
   * @param aWidth, aHeight 
   * @return 
   **/
   NS_IMETHOD GetPageDimensions ( float &aWidth, float &aHeight ) = 0;   

  /*
   * If toPrinter = PR_FALSE, dest file 
   * @update 
   * @param aPath --
   * @return 
   **/
   NS_IMETHOD GetPath (  char **aPath ) = 0;    

  /*
   * If PR_TRUE, user cancelled 
   * @update 
   * @param aCancel -- 
   * @return 
   **/
   NS_IMETHOD GetUserCancelled(  PRBool &aCancel ) = 0;      
};

#endif

