/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code mozilla.org code.
 *
 * The Initial Developer of the Original Code Christopher Blizzard
 * <blizzard@mozilla.org>.  Portions created by the Initial Developer
 * are Copyright (C) 2001 the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef __nsWindow_h__

#include "nsCommonWidget.h"

#include "mozcontainer.h"
#include "mozdrawingarea.h"

class nsWindow : public nsCommonWidget {
 public:
  nsWindow();
  virtual ~nsWindow();

  // nsIWidget
  NS_IMETHOD         Create(nsIWidget        *aParent,
			    const nsRect     &aRect,
			    EVENT_CALLBACK   aHandleEventFunction,
			    nsIDeviceContext *aContext,
			    nsIAppShell      *aAppShell,
			    nsIToolkit       *aToolkit,
			    nsWidgetInitData *aInitData);
  NS_IMETHOD         Create(nsNativeWidget aParent,
			    const nsRect     &aRect,
			    EVENT_CALLBACK   aHandleEventFunction,
			    nsIDeviceContext *aContext,
			    nsIAppShell      *aAppShell,
			    nsIToolkit       *aToolkit,
			    nsWidgetInitData *aInitData);
  NS_IMETHOD         Destroy(void);
  NS_IMETHOD         Show(PRBool aState);
  NS_IMETHOD         SetModal(PRBool aModal);
  NS_IMETHOD         IsVisible(PRBool & aState);
  NS_IMETHOD         ConstrainPosition(PRInt32 *aX,
				       PRInt32 *aY);
  NS_IMETHOD         Move(PRInt32 aX,
			   PRInt32 aY);
  NS_IMETHOD         Resize(PRInt32 aWidth,
			    PRInt32 aHeight,
			    PRBool  aRepaint);
  NS_IMETHOD         Resize(PRInt32 aX,
			    PRInt32 aY,
			    PRInt32 aWidth,
			    PRInt32 aHeight,
			    PRBool   aRepaint);
  NS_IMETHOD         PlaceBehind(nsIWidget *aWidget,
				 PRBool     aActivate);
  NS_IMETHOD         Enable(PRBool aState);
  NS_IMETHOD         SetFocus(PRBool aRaise = PR_FALSE);
  NS_IMETHOD         GetScreenBounds(nsRect &aRect);
  NS_IMETHOD         SetForegroundColor(const nscolor &aColor);
  NS_IMETHOD         SetBackgroundColor(const nscolor &aColor);
  virtual            nsIFontMetrics* GetFont(void);
  NS_IMETHOD         SetFont(const nsFont &aFont);
  NS_IMETHOD         SetCursor(nsCursor aCursor);
  NS_IMETHOD         Validate();
  NS_IMETHOD         Invalidate(PRBool aIsSynchronous);
  NS_IMETHOD         Invalidate(const nsRect &aRect,
				PRBool        aIsSynchronous);
  NS_IMETHOD         InvalidateRegion(const nsIRegion *aRegion,
				      PRBool           aIsSynchronous);
  NS_IMETHOD         Update();
  NS_IMETHOD         SetColorMap(nsColorMap *aColorMap);
  NS_IMETHOD         Scroll(PRInt32  aDx,
			    PRInt32  aDy,
			    nsRect  *aClipRect);
  NS_IMETHOD         ScrollWidgets(PRInt32 aDx,
				   PRInt32 aDy);
  NS_IMETHOD         ScrollRect(nsRect  &aSrcRect,
				PRInt32  aDx,
				PRInt32  aDy);
  virtual void*      GetNativeData(PRUint32 aDataType);
  NS_IMETHOD         SetBorderStyle(nsBorderStyle aBorderStyle);
  NS_IMETHOD         SetTitle(const nsString& aTitle);
  NS_IMETHOD         SetIcon(const nsAReadableString& anIconSpec);
  NS_IMETHOD         SetMenuBar(nsIMenuBar * aMenuBar);
  NS_IMETHOD         ShowMenuBar(PRBool aShow);
  NS_IMETHOD         WidgetToScreen(const nsRect& aOldRect, nsRect& aNewRect);
  NS_IMETHOD         ScreenToWidget(const nsRect& aOldRect, nsRect& aNewRect);
  NS_IMETHOD         BeginResizingChildren(void);
  NS_IMETHOD         EndResizingChildren(void);
  NS_IMETHOD         GetPreferredSize(PRInt32 &aWidth,
				      PRInt32 &aHeight);
  NS_IMETHOD         SetPreferredSize(PRInt32 aWidth,
				      PRInt32 aHeight);
  NS_IMETHOD         Paint(nsIRenderingContext &aRenderingContext,
			   const nsRect        &aDirtyRect);
  NS_IMETHOD         EnableDragDrop(PRBool aEnable);
  virtual void       ConvertToDeviceCoordinates(nscoord &aX,
						nscoord &aY);
  NS_IMETHOD         PreCreateWidget(nsWidgetInitData *aWidgetInitData);
  NS_IMETHOD         CaptureMouse(PRBool aCapture);
  NS_IMETHOD         CaptureRollupEvents(nsIRollupListener *aListener,
					 PRBool           aDoCapture,
					 PRBool           aConsumeRollupEvent);
  NS_IMETHOD         ModalEventFilter(PRBool  aRealEvent,
				      void   *aEvent,
				      PRBool *aForWindow);
  NS_IMETHOD         GetAttention();

  // event callbacks
  gboolean           OnExposeEvent(GtkWidget *aWidget,
				   GdkEventExpose *aEvent);
  gboolean           OnConfigureEvent(GtkWidget *aWidget,
				      GdkEventConfigure *aEvent);
  void               OnSizeAllocate(GtkWidget *aWidget,
				    GtkAllocation *aAllocation);
  void               OnDeleteEvent(GtkWidget *aWidget,
				   GdkEventAny *aEvent);
  void               OnEnterNotifyEvent(GtkWidget *aWidget,
					GdkEventCrossing *aEvent);
  void               OnLeaveNotifyEvent(GtkWidget *aWidget,
					GdkEventCrossing *aEvent);
  void               OnMotionNotifyEvent(GtkWidget *aWidget,
					 GdkEventMotion *aEvent);
  void               OnButtonPressEvent(GtkWidget *aWidget,
					GdkEventButton *aEvent);
  void               OnButtonReleaseEvent(GtkWidget *aWidget,
					  GdkEventButton *aEvent);
  void               SendResizeEvent(nsRect &aRect,
				     nsEventStatus &aStatus);

 private:

  nsresult NativeCreate(nsIWidget        *aParent,
		        nsNativeWidget    aNativeParent,
		        const nsRect     &aRect,
		        EVENT_CALLBACK    aHandleEventFunction,
		        nsIDeviceContext *aContext,
		        nsIAppShell      *aAppShell,
		        nsIToolkit       *aToolkit,
		        nsWidgetInitData *aInitData);

  GtkWidget          *mShell;
  MozContainer       *mContainer;
  MozDrawingarea     *mDrawingarea;

  PRPackedBool        mIsTopLevel;
  PRPackedBool        mIsDestroyed;

  PRUint32            mPreferredWidth;
  PRUint32            mPreferredHeight;

};

class nsChildWindow : public nsWindow {
 public:
  nsChildWindow();
  ~nsChildWindow();
};

#endif /* __nsWindow_h__ */
