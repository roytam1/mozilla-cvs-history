/* 
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
 *                  Ashutosh Kulkarni <ashuk@eng.sun.com>
 *      Jason Mawdsley <jason@macadamian.com>
 *      Louis-Philippe Gagnon <louisphilippe@macadamian.com>
 */

package org.mozilla.webclient.impl.wrapper_native;

import java.util.Queue;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.logging.Level;
import java.util.logging.Logger;
import org.mozilla.util.Assert;
import org.mozilla.util.Log;
import org.mozilla.util.ParameterCheck;

import org.mozilla.webclient.impl.WrapperFactory;

/**
 * <p>This is a singleton class.  All native events pass thru this class
 * by virtue of the {@link #pushRunnable} or {@link pushBlockingWCRunnable}
 * methods.</p>
 */

public class NativeEventThread extends Thread {

    //
    // Class variables
    //
    
    public static final String LOG = "org.mozilla.webclient.impl.wrapper_native.NativeEventThread";

    public static final Logger LOGGER = Log.getLogger(LOG);

    static NativeEventThread instance = null;
    
    //
    // Attribute ivars
    //

    //
    // Relationship ivars
    //
    
    private WrapperFactory wrapperFactory;
    private int nativeWrapperFactory;
    
    private Queue<WCRunnable> blockingRunnables;
    private Queue<Runnable> runnables;
    
    
    //
    // Attribute ivars
    //
    
    //
    // Constructors
    //
    
    public NativeEventThread(String threadName, 
			     WrapperFactory yourFactory,
			     int yourNativeWrapperFactory) {
	super(threadName);
	// Don't do this check for subclasses
	if (this.getClass() == NativeEventThread.class) {
	    instance = this;
	}
	ParameterCheck.nonNull(yourFactory);
	
	wrapperFactory = yourFactory;
	nativeWrapperFactory = yourNativeWrapperFactory;
	
	blockingRunnables = new ConcurrentLinkedQueue<WCRunnable>();
	runnables = new ConcurrentLinkedQueue<Runnable>();
    }
    
    /**
     *
     * This is a very delicate method, and possibly subject to race
     * condition problems.  To combat this, our first step is to set our
     * wrapperFactory to null, within a synchronized block which
     * synchronizes on the same object used in the run() method's event
     * loop.  By setting the wrapperFactory ivar to null, we cause
     * the run method to return.
     */
    
    public void delete() {
	synchronized(this) {
	    // this has to be inside the synchronized block!
	    wrapperFactory = null;
	    try {
                if (LOGGER.isLoggable(Level.FINEST)) {
                    LOGGER.finest("NativeEventThread.delete: About to wait during delete()");
                }
		wait();
                if (LOGGER.isLoggable(Level.FINEST)) {
                    LOGGER.finest("NativeEventThread.delete: Returned from wait during delete()");
                }
	    }
	    catch (Exception e) {
		System.out.println("NativeEventThread.delete: interrupted while waiting\n\t for NativeEventThread to notifyAll() after destruction of initContext: " + e +
				   " " + e.getMessage());
	    }
	}
	wrapperFactory = null;
    }

    //
    // Methods from Thread
    //
    
/**

 * This method is the heart of webclient.  It is called indirectly from
 * {@link WrapperFactoryImpl#initialize}.  It calls nativeStartup, which
 * does the per-window initialization, including creating the native
 * event queue which corresponds to this instance, then enters into an
 * infinite loop where processes native events, then checks to see if
 * there are any listeners to add, and adds them if necessary.

 * @see nativeProcessEvents

 */

public void run()
{
    Runnable runnable;
    WCRunnable wcRunnable;
    // our owner must have put an event in the queue 
    Assert.assert_it(!runnables.isEmpty());
    ((Runnable)runnables.poll()).run();
    synchronized (wrapperFactory) {
	try {
	    wrapperFactory.notifyAll();
	}
	catch (Exception e) {
	    System.out.println("NativeEventThread.run: exception trying to send notifyAll() to WrapperFactoryImpl on startup:" + e + " " + e.getMessage());
	}
    }

    //
    // Execute the event-loop.
    // 
    
    while (true) {
        try {
            Thread.sleep(1);
        }
        catch (Exception e) {
            System.out.println("NativeEventThread.run(): Exception: " + e +
                               " while sleeping: " + e.getMessage());
        }
        runnable = runnables.poll();
        wcRunnable = blockingRunnables.poll();
        synchronized (this) {
	    // if we are have been told to delete ourselves
            if (null == this.wrapperFactory) {
                try {
                    notifyAll();
                }
                catch (Exception e) {
                    System.out.println("NativeEventThread.run: Exception: trying to send notifyAll() to this during delete: " + e + " " + e.getMessage());
                }
                return;
            }
	    
	    if (null!= runnable) {
                if (LOGGER.isLoggable(Level.FINEST)) {
                    LOGGER.finest("NativeEventThread.run: About to run " + 
                            runnable.toString());
                }
		runnable.run();
                if (LOGGER.isLoggable(Level.FINEST)) {
                    LOGGER.finest("NativeEventThread.run: Return from run " + 
                            runnable.toString());
                }
	    }
	    if (null != wcRunnable) {
                if (LOGGER.isLoggable(Level.FINEST)) {
                    LOGGER.finest("NativeEventThread.run: About to run " +
                            wcRunnable.toString());
                }
                wcRunnable.setResult(wcRunnable.run());
                if (LOGGER.isLoggable(Level.FINEST)) {
                    LOGGER.finest("NativeEventThread.run: Return from run " +
                            wcRunnable.toString());
                }
		// notify the pushBlockingWCRunnable() method.
                synchronized (wcRunnable) {
                    try {
                        wcRunnable.notifyAll();
                    }
                    catch (Exception e) {
                        System.out.println("NativeEventThread.run: Exception: trying to notify for blocking result:" + e + " " + e.getMessage());
                    }
                }
	    }
	    nativeProcessEvents(nativeWrapperFactory);
        }
    }
}

//
// private methods
//

//
// Package methods
//

    void pushRunnable(Runnable toInvoke) {
	synchronized (this) {
	    runnables.add(toInvoke);
	}
    }
    
    Object pushBlockingWCRunnable(WCRunnable toInvoke) throws RuntimeException {
	Object result = null;

	if (Thread.currentThread().getName().equals(instance.getName())){
	    toInvoke.run();
            result = toInvoke.getResult();
            if (result instanceof RuntimeException) {
                throw ((RuntimeException) result);
            }
	    return result;
	}

        blockingRunnables.add(toInvoke);
        synchronized (toInvoke) {
	    try {
                if (LOGGER.isLoggable(Level.FINEST)) {
                    LOGGER.finest("NativeEventThread.pushBlockingWCRunnable: " +
                            " About to wait for NativeEventThread to run " +
                            toInvoke.toString());
                }
		toInvoke.wait();
                if (LOGGER.isLoggable(Level.FINEST)) {
                    LOGGER.finest("NativeEventThread.pushBlockingWCRunnable: " +
                            " Return from wait for NativeEventThread to run " +
                            toInvoke.toString());
                }
	    }
	    catch (Exception se) {
		System.out.println("NativeEventThread.pushBlockingWCRunnable: Exception: while waiting for blocking result: " + se + "  " + se.getMessage());
	    }
        }
        result = toInvoke.getResult();
        if (result instanceof RuntimeException) {
            throw ((RuntimeException) result);
        }

	return result;
    }

//
// Native methods
//


/**

 * Called from java to allow the native code to process any pending
 * events, such as: painting the UI, processing mouse and key events,
 * etc.

 */

public native void nativeProcessEvents(int webShellPtr);

} // end of class NativeEventThread
