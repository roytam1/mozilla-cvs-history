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
 *		John C. Griggs <johng@corel.com>
 *
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
#ifndef nsDeviceContextQT_h___
#define nsDeviceContextQT_h___

#include "nsDeviceContext.h"
#include "nsUnitConversion.h"
#include "nsIWidget.h"
#include "nsIView.h"
#include "nsIRenderingContext.h"

class QWidget;

class nsDeviceContextQT : public DeviceContextImpl
{
public:
    nsDeviceContextQT();
    virtual ~nsDeviceContextQT();

    NS_IMETHOD  Init(nsNativeWidget aNativeWidget);

    NS_IMETHOD  CreateRenderingContext(nsIRenderingContext *&aContext);
    NS_IMETHOD  CreateRenderingContext(nsIView *aView,nsIRenderingContext *&aContext)
    {
      return(DeviceContextImpl::CreateRenderingContext(aView,aContext));
    }
    NS_IMETHOD  CreateRenderingContext(nsIWidget *aWidget,nsIRenderingContext *&aContext)
    {
      return (DeviceContextImpl::CreateRenderingContext(aWidget,aContext));
    }

    NS_IMETHOD  SupportsNativeWidgets(PRBool &aSupportsWidgets);

    NS_IMETHOD  GetScrollBarDimensions(float &aWidth,float &aHeight) const;
    NS_IMETHOD  GetSystemFont(nsSystemFontID anID, nsFont *aFont) const;

    //get a low level drawing surface for rendering. the rendering context
    //that is passed in is used to create the drawing surface if there isn't
    //already one in the device context. the drawing surface is then cached
    //in the device context for re-use.
    NS_IMETHOD GetDrawingSurface(nsIRenderingContext &aContext, 
                                 nsDrawingSurface &aSurface);

    NS_IMETHOD ConvertPixel(nscolor aColor,PRUint32 &aPixel);
    NS_IMETHOD CheckFontExistence(const nsString &aFontName);

    NS_IMETHOD GetDeviceSurfaceDimensions(PRInt32 &aWidth,PRInt32 &aHeight);
    NS_IMETHOD GetClientRect(nsRect &aRect);
    NS_IMETHOD GetRect(nsRect &aRect);

    NS_IMETHOD GetDeviceContextFor(nsIDeviceContextSpec *aDevice,
                                   nsIDeviceContext *&aContext);

    NS_IMETHOD BeginDocument(PRUnichar * aTitle, PRUnichar* aPrintToFileName, PRInt32 aStartPage, PRInt32 aEndPage);
    NS_IMETHOD EndDocument(void);

    NS_IMETHOD BeginPage(void);
    NS_IMETHOD EndPage(void);

    // Overridden DeviceContextImpl functions.
    NS_IMETHOD GetDepth(PRUint32 &aDepth);

    static int prefChanged(const char *aPref,void *aClosure);
    nsresult   SetDPI(PRInt32 dpi);

private:
    PRUint32      mDepth;
    PRBool        mWriteable;
    PRUint32      mNumCells;
    PRInt16       mScrollbarHeight;
    PRInt16       mScrollbarWidth;
    PRInt16       mWindowBorderWidth;
    PRInt16       mWindowBorderHeight;
    QWidget 	  *mWidget;    
    PRInt32       mWidth;
    PRInt32       mHeight; 
    float         mWidthFloat;
    float         mHeightFloat; 

    static nscoord mDpi;

    nsresult GetSystemFontInfo(nsFont *aFont) const;

    PRUint32 mID;
};

#endif /* nsDeviceContextQT_h___ */

