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

#ifndef nsIWidget_h__
#define nsIWidget_h__

#include "nsISupports.h"
#include "nsColor.h"
#include "nsIMouseListener.h"
#include "nsIMenuListener.h"
#include "nsIImage.h"

#include "prthread.h"
#include "nsGUIEvent.h"

// forward declarations
class   nsIAppShell;
class   nsIToolkit;
class   nsIFontMetrics;
class   nsIToolkit;
class   nsIRenderingContext;
class   nsIEnumerator;
class   nsIDeviceContext;
struct  nsRect;
struct  nsFont;
class   nsIMenuBar;
class   nsIEventListener;
class   nsIRollupListener;

/**
 * Callback function that processes events.
 * The argument is actually a subtype (subclass) of nsEvent which carries
 * platform specific information about the event. Platform specific code knows
 * how to deal with it.
 * The return value determines whether or not the default action should take place.
 */

typedef nsEventStatus (*PR_CALLBACK EVENT_CALLBACK)(nsGUIEvent *event);

/**
 * Flags for the getNativeData function.
 * See getNativeData()
 */
#define NS_NATIVE_WINDOW    0
#define NS_NATIVE_GRAPHIC   1
#define NS_NATIVE_COLORMAP  2
#define NS_NATIVE_WIDGET    3
#define NS_NATIVE_DISPLAY   4
#define NS_NATIVE_REGION		5
#define NS_NATIVE_OFFSETX		6
#define NS_NATIVE_OFFSETY		7
#define NS_NATIVE_PLUGIN_PORT	8
#define NS_NATIVE_SCREEN      9

// {18032AD5-B265-11d1-AA2A-000000000000}
#define NS_IWIDGET_IID \
{ 0x18032ad5, 0xb265, 0x11d1, \
{ 0xaa, 0x2a, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 } }


// Hide the native window systems real window type so as to avoid
// including native window system types and api's. This is necessary
// to ensure cross-platform code.
typedef void* nsNativeWidget;

/**
 * Border styles
 */

enum nsWindowType {
  // default top level window
  eWindowType_toplevel,
  // top level window but usually handled differently by the OS
  eWindowType_dialog,
  // used for combo boxes, etc
  eWindowType_popup,
  // child windows (contained inside a window on the desktop (has no border))
  eWindowType_child
};


enum nsBorderStyle
{
  // no border, titlebar, etc.. opposite of all
  eBorderStyle_none     = 0,

  // all window decorations
  eBorderStyle_all      = 1 << 0,

  // enables the border on the window.  these are only for decoration and are not resize hadles
  eBorderStyle_border   = 1 << 1,

  // enables the resize handles for the window.  if this is set, border is implied to also be set
  eBorderStyle_resizeh  = 1 << 2,

  // enables the titlebar for the window
  eBorderStyle_title    = 1 << 3,

  // enables the window menu button on the title bar.  this being on should force the title bar to display
  eBorderStyle_menu     = 1 << 4,

  // enables the minimize button so the user can minimize the window.
  //   turned off for tranient windows since they can not be minimized seperate from their parent
  eBorderStyle_minimize = 1 << 5,

  // enables the maxmize button so the user can maximize the window
  eBorderStyle_maximize = 1 << 6,

  // show the close button
  eBorderStyle_close    = 1 << 7,

  // whatever the OS wants... i.e. don't do anything
  eBorderStyle_default  = -1
};

/**
 * Cursor types.
 */

enum nsCursor {   ///(normal cursor,       usually rendered as an arrow)
                eCursor_standard, 
                  ///(system is busy,      usually rendered as a hourglass or watch)
                eCursor_wait, 
                  ///(Selecting something, usually rendered as an IBeam)
                eCursor_select, 
                  ///(can hyper-link,      usually rendered as a human hand)
                eCursor_hyperlink, 
                  ///(west/east sizing,    usually rendered as ->||<-)
                eCursor_sizeWE,
                  ///(north/south sizing,  usually rendered as sizeWE rotated 90 degrees)
                eCursor_sizeNS,
                eCursor_arrow_north,
                eCursor_arrow_north_plus,
                eCursor_arrow_south,
                eCursor_arrow_south_plus,
                eCursor_arrow_west,
                eCursor_arrow_west_plus,
                eCursor_arrow_east,
                eCursor_arrow_east_plus,
                eCursor_crosshair,
                //Don't know what 'move' cursor should be.  See CSS2.
                eCursor_move,
                eCursor_help
                }; 


/**
 * Basic struct for widget initialization data.
 * @see Create member function of nsIWidget
 */

struct nsWidgetInitData {
  nsWidgetInitData()
    : clipChildren(PR_FALSE), clipSiblings(PR_FALSE),
      mWindowType(eWindowType_child),
      mBorderStyle(eBorderStyle_default)
  {
  }

  // when painting exclude area occupied by child windows and sibling windows
  PRPackedBool  clipChildren, clipSiblings;
  nsWindowType mWindowType;
  nsBorderStyle mBorderStyle;
};

/**
 * The base class for all the widgets. It provides the interface for
 * all basic and necessary functionality.
 */
class nsIWidget : public nsISupports {

  public:

    NS_DEFINE_STATIC_IID_ACCESSOR(NS_IWIDGET_IID)

    /**
     * Create and initialize a widget. 
     *
     * The widget represents a window that can be drawn into. It also is the 
     * base class for user-interface widgets such as buttons and text boxes.
     *
     * All the arguments can be NULL in which case a top level window
     * with size 0 is created. The event callback function has to be
     * provided only if the caller wants to deal with the events this
     * widget receives.  The event callback is basically a preprocess
     * hook called synchronously. The return value determines whether
     * the event goes to the default window procedure or it is hidden
     * to the os. The assumption is that if the event handler returns
     * false the widget does not see the event. The widget should not 
     * automatically clear the window to the background color. The 
     * calling code must handle paint messages and clear the background 
     * itself. 
     *
     * @param     parent or null if it's a top level window
     * @param     aRect     the widget dimension
     * @param     aHandleEventFunction the event handler callback function
     * @param     aContext
     * @param     aAppShell the parent application shell. If nsnull,
     *                      the parent window's application shell will be used.
     * @param     aToolkit
     * @param     aInitData data that is used for widget initialization
     *
     */
    NS_IMETHOD Create(nsIWidget        *aParent,
                        const nsRect     &aRect,
                        EVENT_CALLBACK   aHandleEventFunction,
                        nsIDeviceContext *aContext,
                        nsIAppShell      *aAppShell = nsnull,
                        nsIToolkit       *aToolkit = nsnull,
                        nsWidgetInitData *aInitData = nsnull) = 0;

    /**
     * Create and initialize a widget with a native window parent
     *
     * The widget represents a window that can be drawn into. It also is the 
     * base class for user-interface widgets such as buttons and text boxes.
     *
     * All the arguments can be NULL in which case a top level window
     * with size 0 is created. The event callback function has to be
     * provided only if the caller wants to deal with the events this
     * widget receives.  The event callback is basically a preprocess
     * hook called synchronously. The return value determines whether
     * the event goes to the default window procedure or it is hidden
     * to the os. The assumption is that if the event handler returns
     * false the widget does not see the event.
     *
     * @param     aParent   native window.
     * @param     aRect     the widget dimension
     * @param     aHandleEventFunction the event handler callback function
     */
    NS_IMETHOD Create(nsNativeWidget aParent,
                        const nsRect     &aRect,
                        EVENT_CALLBACK   aHandleEventFunction,
                        nsIDeviceContext *aContext,
                        nsIAppShell      *aAppShell = nsnull,
                        nsIToolkit       *aToolkit = nsnull,
                        nsWidgetInitData *aInitData = nsnull) = 0;


    /**
     * Accessor functions to get and set the client data associated with the
     * widget.
     */
    //@{
    NS_IMETHOD  GetClientData(void*& aClientData) = 0;
    NS_IMETHOD  SetClientData(void* aClientData) = 0;
    //@}

    /**
     * Close and destroy the internal native window. 
     * This method does not delete the widget.
     */

    NS_IMETHOD Destroy(void) = 0;

    /**
     * Return the parent Widget of this Widget or nsnull if this is a 
     * top level window
     *
     * @return the parent widget or nsnull if it does not have a parent
     *
     */
    virtual nsIWidget* GetParent(void) = 0;

    /**
     * Return an nsEnumerator over the children of this widget.
     *
     * @return an enumerator over the list of children or nsnull if it does not
     * have any children
     *
     */
    virtual nsIEnumerator*  GetChildren(void) = 0;

    /**
     * Show or hide this widget
     *
     * @param aState PR_TRUE to show the Widget, PR_FALSE to hide it
     *
     */
    NS_IMETHOD Show(PRBool aState) = 0;

    /**
     * Make the window modal
     *
     */
    NS_IMETHOD SetModal(PRBool aModal) = 0;

    /**
     * Returns whether the window is visible
     *
     */
    NS_IMETHOD IsVisible(PRBool & aState) = 0;

    /**
     * Move this widget.
     *
     * @param aX the new x position expressed in the parent's coordinate system
     * @param aY the new y position expressed in the parent's coordinate system
     *
     **/
    NS_IMETHOD Move(PRInt32 aX, PRInt32 aY) = 0;

    /**
     * Resize this widget. 
     *
     * @param aWidth  the new width expressed in the parent's coordinate system
     * @param aHeight the new height expressed in the parent's coordinate system
     * @param aRepaint whether the widget should be repainted
     *
     */
    NS_IMETHOD Resize(PRInt32 aWidth,
                      PRInt32 aHeight,
                      PRBool   aRepaint) = 0;

    /**
     * Move or resize this widget.
     *
     * @param aX       the new x position expressed in the parent's coordinate system
     * @param aY       the new y position expressed in the parent's coordinate system
     * @param aWidth   the new width expressed in the parent's coordinate system
     * @param aHeight  the new height expressed in the parent's coordinate system
     * @param aRepaint whether the widget should be repainted if the size changes
     *
     */
    NS_IMETHOD Resize(PRInt32 aX,
                      PRInt32 aY,
                      PRInt32 aWidth,
                      PRInt32 aHeight,
                      PRBool   aRepaint) = 0;

    /**
     * Set's the widget's z-index.
     */
    NS_IMETHOD SetZIndex(PRInt32 aZIndex) = 0;

    /**
     * Get's the widget's z-index. 
     */
    NS_IMETHOD GetZIndex(PRInt32* aZIndex) = 0;

    /**
     * Enable or disable this Widget
     *
     * @param aState PR_TRUE to enable the Widget, PR_FALSE to disable it.
     *
     */
    NS_IMETHOD Enable(PRBool aState) = 0;

    /**
     * Give focus to this widget.
     */
    NS_IMETHOD SetFocus(void) = 0;

    /**
     * Get this widget's outside dimensions relative to it's parent widget
     *
     * @param aRect on return it holds the  x. y, width and height of this widget
     *
     */
    NS_IMETHOD GetBounds(nsRect &aRect) = 0;
  
    
    /**
     * Get this widget's client area dimensions, if the window has a 3D border appearance
     * this returns the area inside the border, The x and y are always zero
     *
     * @param aRect on return it holds the  x. y, width and height of the client area of this widget
     *
     */
    NS_IMETHOD GetClientBounds(nsRect &aRect) = 0;

    /**
     * Gets the width and height of the borders
     * @param aWidth the width of the border
     * @param aHeight the height of the border
     *
     */
    NS_IMETHOD GetBorderSize(PRInt32 &aWidth, PRInt32 &aHeight) = 0;

    /**
     * Get the foreground color for this widget
     *
     * @return this widget's foreground color
     *
     */
    virtual nscolor GetForegroundColor(void) = 0;

    /**
     * Set the foreground color for this widget
     *
     * @param aColor the new foreground color
     *
     */

    NS_IMETHOD SetForegroundColor(const nscolor &aColor) = 0;

    /**
     * Get the background color for this widget
     *
     * @return this widget's background color
     *
     */

    virtual nscolor GetBackgroundColor(void) = 0;

    /**
     * Set the background color for this widget
     *
     * @param aColor the new background color
     *
     */

    NS_IMETHOD SetBackgroundColor(const nscolor &aColor) = 0;

    /**
     * Get the font for this widget
     *
     * @return the font metrics 
     */

    virtual nsIFontMetrics* GetFont(void) = 0;

    /**
     * Set the font for this widget 
     *
     * @param aFont font to display. See nsFont for allowable fonts
     */

    NS_IMETHOD SetFont(const nsFont &aFont) = 0;

    /**
     * Get the cursor for this widget.
     *
     * @return this widget's cursor.
     */

    virtual nsCursor GetCursor(void) = 0;

    /**
     * Set the cursor for this widget
     *
     * @param aCursor the new cursor for this widget
     */

    NS_IMETHOD SetCursor(nsCursor aCursor) = 0;

    /**
     * Invalidate the widget and repaint it.
     *
     * @param aIsSynchronouse PR_TRUE then repaint synchronously. If PR_FALSE repaint later.
     * @see #Update()
     */

    NS_IMETHOD Invalidate(PRBool aIsSynchronous) = 0;

    /**
     * Invalidate a specified rect for a widget and repaints it.
     *
     * @param aIsSynchronouse PR_TRUE then repaint synchronously. If PR_FALSE repaint later.
     * @see #Update()
     */

    NS_IMETHOD Invalidate(const nsRect & aRect, PRBool aIsSynchronous) = 0;

    /**
     * Invalidate a specified region for a widget and repaints it.
     *
     * @param aIsSynchronouse PR_TRUE then repaint synchronously. If PR_FALSE repaint later.
     * @see #Update()
     */

    NS_IMETHOD InvalidateRegion(const nsIRegion* aRegion, PRBool aIsSynchronous) = 0;

    /**
     * Force a synchronous repaint of the window if there are dirty rects.
     *
     * @see Invalidate()
     */

     NS_IMETHOD Update() = 0;

    /**
     * Adds a mouse listener to this widget
     * Any existing mouse listener is replaced
     *
     * @param aListener mouse listener to add to this widget.
     */

    NS_IMETHOD AddMouseListener(nsIMouseListener * aListener) = 0;

    /**
     * Adds an event listener to this widget
     * Any existing event listener is replaced
     *
     * @param aListener event listener to add to this widget.
     */

    NS_IMETHOD AddEventListener(nsIEventListener * aListener) = 0;

    /**
     * Adds a menu listener to this widget
     * Any existing menu listener is replaced
     *
     * @param aListener menu listener to add to this widget.
     */

    NS_IMETHOD AddMenuListener(nsIMenuListener * aListener) = 0;
    
    /**
     * Return the widget's toolkit
     *
     * @return the toolkit this widget was created in. See nsToolkit.
     */

    virtual nsIToolkit* GetToolkit() = 0;    

    /**
     * Set the color map for this widget
     *
     * @param aColorMap color map for displaying this widget
     *
     */

    NS_IMETHOD SetColorMap(nsColorMap *aColorMap) = 0;

    /**
     * Scroll this widget.
     *
     * @param aDx amount to scroll along the x-axis
     * @param aDy amount to scroll along the y-axis.
     * @param aClipRect clipping rectangle to limit the scroll to.
     *
     */

    NS_IMETHOD Scroll(PRInt32 aDx, PRInt32 aDy, nsRect *aClipRect) = 0;

    /**
     * Scroll an area of this widget.
     *
     * @param aRect source rectangle to scroll in the widget
     * @param aDx x offset from the source
     * @param aDy y offset from the source
     *
     */

    NS_IMETHOD ScrollRect(nsRect &aSrcRect, PRInt32 aDx, PRInt32 aDy) = 0;

    /** 
     * Internal methods
     */

    //@{
    virtual void AddChild(nsIWidget* aChild) = 0;
    virtual void RemoveChild(nsIWidget* aChild) = 0;
    virtual void* GetNativeData(PRUint32 aDataType) = 0;
    virtual void FreeNativeData(void * data, PRUint32 aDataType) = 0;//~~~
    virtual nsIRenderingContext* GetRenderingContext() = 0;
    virtual nsIDeviceContext* GetDeviceContext() = 0;
    virtual nsIAppShell *GetAppShell() = 0;
    //@}

    /**
     * Set border style
     * Must be called before Create.
     * @param aBorderStyle @see nsBorderStyle
     */

    NS_IMETHOD SetBorderStyle(nsBorderStyle aBorderStyle) = 0;

    /**
     * Set the widget's title.
     * Must be called after Create.
     *
     * @param aTitle string displayed as the title of the widget
     */

    NS_IMETHOD SetTitle(const nsString& aTitle) = 0;

    /**
     * Set the widget's MenuBar.
     * Must be called after Create.
     *
     * @param aMenuBar the menubar
     */

    NS_IMETHOD SetMenuBar(nsIMenuBar * aMenuBar) = 0;

    /**
     * Set the widget's MenuBar's visibility
     *
     * @param aShow PR_TRUE to show, PR_FALSE to hide
     */

    NS_IMETHOD ShowMenuBar(PRBool aShow) = 0;

     /**
     * Convert from this widget coordinates to screen coordinates.
     *
     * @param  aOldRect  widget coordinates stored in the x,y members
     * @param  aNewRect  screen coordinates stored in the x,y members
     */

    NS_IMETHOD WidgetToScreen(const nsRect& aOldRect, nsRect& aNewRect) = 0;

    /**
     * Convert from screen coordinates to this widget's coordinates.
     *
     * @param  aOldRect  screen coordinates stored in the x,y members
     * @param  aNewRect  widget's coordinates stored in the x,y members
     */

    NS_IMETHOD ScreenToWidget(const nsRect& aOldRect, nsRect& aNewRect) = 0;

    /**
     * When adjustments are to made to a whole set of child widgets, call this
     * before resizing/positioning the child windows to minimize repaints. Must
     * be followed by EndResizingChildren() after child windows have been
     * adjusted.
     *
     */

    NS_IMETHOD BeginResizingChildren(void) = 0;

    /**
     * Call this when finished adjusting child windows. Must be preceded by
     * BeginResizingChildren().
     *
     */

    NS_IMETHOD EndResizingChildren(void) = 0;

    /**
     * Returns the preferred width and height for the widget
     *
     */
    NS_IMETHOD GetPreferredSize(PRInt32& aWidth, PRInt32& aHeight) = 0;

    /**
     * Set the preferred width and height for the widget
     *
     */
    NS_IMETHOD SetPreferredSize(PRInt32 aWidth, PRInt32 aHeight) = 0;

    /**
     * Dispatches and event to the widget
     *
     */
    NS_IMETHOD DispatchEvent(nsGUIEvent* event, nsEventStatus & aStatus) = 0;


#ifdef LOSER
    /**
     * FSets the vertical scrollbar widget
     *
     */
    NS_IMETHOD SetVerticalScrollbar(nsIWidget * aScrollbar) = 0;
#endif

    /**
     * For printing and lightweight widgets
     *
     */
    NS_IMETHOD Paint(nsIRenderingContext& aRenderingContext,
                     const nsRect& aDirtyRect) = 0;
   
    /**
     * Enables the dropping of files to a widget (XXX this is temporary)
     *
     */
    NS_IMETHOD EnableDragDrop(PRBool aEnable) = 0;
   
    virtual void  ConvertToDeviceCoordinates(nscoord	&aX,nscoord	&aY) = 0;

    /**
     * Enables/Disables system mouse capture.
     * @param aCapture PR_TRUE enables mouse capture, PR_FALSE disables mouse capture 
     *
     */
    NS_IMETHOD CaptureMouse(PRBool aCapture) = 0;

	/**
	 * Enables/Disables system capture of any and all events that would cause a
	 * dropdown to be rolled up, This method ignores the aConsumeRollupEvent 
   * parameter when aDoCapture is FALSE
	 * @param aCapture PR_TRUE enables capture, PR_FALSE disables capture 
	 * @param aConsumeRollupEvent PR_TRUE consumes the rollup event, PR_FALSE dispatches rollup event
	 *
	 */
    NS_IMETHOD CaptureRollupEvents(nsIRollupListener * aListener, PRBool aDoCapture, PRBool aConsumeRollupEvent) = 0;

  /**
   *   Determine whether a given event should be processed assuming we are
   * the currently active modal window.
   *   Note that the exact semantics of this method are platform-dependent.
   * The Macintosh, for instance, cares deeply that this method do exactly
   * as advertised. Gtk, for instance, handles modality in a completely
   * different fashion and does little if anything with this method.
   * @param aRealEvent event is real or a null placeholder (Macintosh)
   * @param aEvent void pointer to native event structure
   * @param aForWindow return value. PR_TRUE iff event should be processed.
   */
  NS_IMETHOD ModalEventFilter(PRBool aRealEvent, void *aEvent, PRBool *aForWindow) = 0;

};

#endif // nsIWidget_h__
