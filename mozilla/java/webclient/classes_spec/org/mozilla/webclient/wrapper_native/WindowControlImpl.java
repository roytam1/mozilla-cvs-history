/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * The Original Code is RaptorCanvas.
 *
 * The Initial Developer of the Original Code is Kirk Baker and
 * Ian Wilkinson. Portions created by Kirk Baker and Ian Wilkinson are
 * Copyright (C) 1999 Kirk Baker and Ian Wilkinson. All
 * Rights Reserved.
 *
 * Contributor(s):  Ed Burns <edburns@acm.org>
 */

package org.mozilla.webclient.wrapper_native;

import org.mozilla.util.Assert;
import org.mozilla.util.Log;
import org.mozilla.util.ParameterCheck;

import org.mozilla.webclient.BrowserControl;
import org.mozilla.webclient.WindowControl;
import org.mozilla.webclient.WrapperFactory;

import java.awt.Rectangle;

public class WindowControlImpl extends ImplObjectNative implements WindowControl
{
//
// Protected Constants
//

//
// Class Variables
//

//
// Instance Variables
//

// Attribute Instance Variables

// Relationship Instance Variables

protected NativeEventThread eventThread = null;

//
// Constructors and Initializers    
//

public WindowControlImpl(WrapperFactory yourFactory, 
			 BrowserControl yourBrowserControl)
{
    super(yourFactory, yourBrowserControl, false);
}

/**

 * First, we delete our eventThread, which causes the eventThread to
 * stop running, which causes the initContext to be deleted.  
 * deallocates native resources for this window.

 */

public void delete()
{
    Assert.assert(null != eventThread, "eventThread shouldn't be null at delete time");
    eventThread.delete();
    eventThread = null;
    nativeWebShell = -1;
}

//
// Class methods
//

//
// General Methods
//


//
// Package Methods
//

NativeEventThread getNativeEventThread()
{
    return eventThread;
}


//
// Methods from WindowControl    
//

public void setBounds(Rectangle newBounds)
{
    ParameterCheck.nonNull(newBounds);
    myFactory.throwExceptionIfNotInitialized();
    Assert.assert(-1 != nativeWebShell);
    
    synchronized(myBrowserControl) {
        nativeSetBounds(nativeWebShell, newBounds.x, newBounds.y,
                        newBounds.width, newBounds.height);
    }
}

public void createWindow(int nativeWindow, Rectangle bounds)
{
    ParameterCheck.greaterThan(nativeWindow, 0);
    ParameterCheck.nonNull(bounds);
    myFactory.throwExceptionIfNotInitialized();

    synchronized(myBrowserControl) {
        synchronized(this) {
            nativeWebShell = nativeCreateInitContext(nativeWindow, bounds.x, 
                                                     bounds.y, bounds.width, 
                                                     bounds.height, myBrowserControl);
            eventThread = new NativeEventThread("EventThread-" +
                                                nativeWebShell,
                                                myBrowserControl);

            // IMPORTANT: the nativeEventThread initializes all the
            // native browser stuff, then sends us notify().
            eventThread.start();
            try {
                wait();
            }
            catch (Exception e) {
                System.out.println("WindowControlImpl.createWindow: interrupted while waiting\n\t for NativeEventThread to notify(): " + e + 
                                   " " + e.getMessage());
            }
        }
    }
}

public int getNativeWebShell()
{
    myFactory.throwExceptionIfNotInitialized();

    return nativeWebShell;
}

public void moveWindowTo(int x, int y)
{
    myFactory.throwExceptionIfNotInitialized();
    
    synchronized(myBrowserControl) {
        nativeMoveWindowTo(nativeWebShell, x, y);
    }
}

public void removeFocus()
{
    myFactory.throwExceptionIfNotInitialized();
    
    synchronized(myBrowserControl) {
        nativeRemoveFocus(nativeWebShell);
    }
}
    
public void repaint(boolean forceRepaint)
{
    myFactory.throwExceptionIfNotInitialized();
    
    synchronized(myBrowserControl) {
        nativeRepaint(nativeWebShell, forceRepaint);
    }
}

public void setVisible(boolean newState)
{
    myFactory.throwExceptionIfNotInitialized();
    
    synchronized(myBrowserControl) {
        nativeSetVisible(nativeWebShell, newState);
    }
}

public void setFocus()
{
    myFactory.throwExceptionIfNotInitialized();

    synchronized(myBrowserControl) {
        nativeSetFocus(nativeWebShell);
    }
}


// 
// Native methods
//

public native void nativeSetBounds(int webShellPtr, int x, int y, 
                                   int w, int h);

/**

 * This method allows the native code to create a "context object" that
 * is passed to all subsequent native methods.  This context object is
 * opaque to Java and should just be a pointer cast to a jint.  For
 * example, this method could create a struct that encapsulates the
 * native window (from the nativeWindow argument), the x, y, width,
 * height parameters for the window, and any other per browser window
 * information.  <P>

 * Subsequent native methods would know how to turn this jint into the
 * struct.

 */

public native int nativeCreateInitContext(int nativeWindow, 
                                          int x, int y, int width, int height, BrowserControl myBrowserControlImpl);

public native void nativeDestroyInitContext(int nativeWindow);

public native void nativeMoveWindowTo(int webShellPtr, int x, int y);

public native void nativeRemoveFocus(int webShellPtr);

public native void nativeRepaint(int webShellPtr, boolean forceRepaint);

public native void nativeSetVisible(int webShellPtr, boolean newState);

public native void nativeSetFocus(int webShellPtr);


// ----VERTIGO_TEST_START

//
// Test methods
//

public static void main(String [] args)
{
    Assert.setEnabled(true);

    Log.setApplicationName("WindowControlImpl");
    Log.setApplicationVersion("0.0");
    Log.setApplicationVersionDate("$Id$");

    try {
        org.mozilla.webclient.BrowserControlFactory.setAppData(args[0]);
	org.mozilla.webclient.BrowserControl control = 
	    org.mozilla.webclient.BrowserControlFactory.newBrowserControl();
        Assert.assert(control != null);
	
	WindowControl wc = (WindowControl)
	    control.queryInterface(org.mozilla.webclient.BrowserControl.WINDOW_CONTROL_NAME);
	Assert.assert(wc != null);
    }
    catch (Exception e) {
	System.out.println("got exception: " + e.getMessage());
    }
}

// ----VERTIGO_TEST_END

} // end of class WindowControlImpl
