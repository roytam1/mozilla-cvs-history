/*
 * $Id$
 */

/* 
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
 * The Initial Developer of the Original Code is Sun
 * Microsystems, Inc. Portions created by Sun are
 * Copyright (C) 1999 Sun Microsystems, Inc. All
 * Rights Reserved.
 *
 * Contributor(s): Ed Burns &lt;edburns@acm.org&gt;
 */
package org.mozilla.mcp;

import java.awt.AWTException;
import java.awt.BorderLayout;
import java.awt.Frame;
import java.awt.Robot;
import java.awt.event.InputEvent;
import java.awt.event.KeyListener;
import java.awt.event.MouseListener;
import java.io.FileNotFoundException;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.concurrent.CountDownLatch;
import java.util.logging.Level;
import java.util.logging.Logger;
import org.mozilla.dom.util.DOMTreeDumper;
import org.mozilla.dom.util.DOMTreeDumper.AbortTraversalException;
import org.mozilla.dom.util.DOMTreeDumper.TreeTraversalCallBack;
import org.mozilla.webclient.BrowserControl;
import org.mozilla.webclient.BrowserControlCanvas;
import org.mozilla.webclient.BrowserControlFactory;
import org.mozilla.webclient.CurrentPage2;
import org.mozilla.webclient.DocumentLoadEvent;
import org.mozilla.webclient.EventRegistration2;
import org.mozilla.webclient.Navigation2;
import org.mozilla.webclient.PageInfoListener;
import org.mozilla.webclient.WebclientEvent;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;

/**
 * <p>The main class for the Mozilla Control Program.  Please see <a
 * href="package-summary.html">the package description</a> for an
 * overview.</p>

 * @author edburns
 */
public class MCP {
    
    /** Creates a new instance of MCP */
    public MCP() {
    }
    
    private static final String MCP_LOG = "org.mozilla.mcp";
    private static final String MCP_LOG_STRINGS = "org.mozilla.mcp.MCPLogStrings";

    static final Logger LOGGER = getLogger(MCP_LOG);
    
    private static Logger getLogger( String loggerName ) {
        return Logger.getLogger(loggerName, MCP_LOG_STRINGS );
    }
    
    private BrowserControl browserControl = null;
    private Navigation2 navigation = null;
    private EventRegistration2 eventRegistration = null;
    private BrowserControlCanvas canvas = null;
    private PageInfoListener pageInfoListener = null;
    private Frame frame = null;
    private int x = 0;
    private int y = 0;
    private int width = 1280;
    private int height = 960;
    private boolean initialized = false;
    private Robot robot;
    private DOMTreeDumper treeDumper = null;
    private CountDownLatch latch = null;
    private TimeoutHandler timeoutHandler = null;
    private long Timeout = 30000L;
    
    private void createLatch() {
        if (null != latch) {
            IllegalStateException ise = new IllegalStateException("Trying to set latch when latch is already set!");
            if (LOGGER.isLoggable(Level.WARNING)) {
                LOGGER.throwing("org.mozilla.mcp.MCP", "createLatch", ise);
            }
            throw ise;
        }
        latch = new CountDownLatch(1);
    }
    
    private void lockLatch() throws InterruptedException {
        if (null == latch) {
            IllegalStateException ise = new IllegalStateException("Trying to lock latch before it has been created!");
            if (LOGGER.isLoggable(Level.WARNING)) {
                LOGGER.throwing("org.mozilla.mcp.MCP", "lockLatch", ise);
            }
            throw ise;
        }
        latch.await();
    }

    private void openLatch() {
        if (null == latch) {
            return;
        }
        if (null != latch || 1 != latch.getCount()) {
            latch.countDown();
            latch = null;
        }
    }    

    
    public void setAppData(String absolutePathToNativeBrowserBinDir)
    throws FileNotFoundException,
            ClassNotFoundException {
        BrowserControlFactory.setAppData(absolutePathToNativeBrowserBinDir);
        initialized = true;
    }
    
    private DOMTreeDumper getDOMTreeDumper() {
        if (null == treeDumper) {
            treeDumper = new DOMTreeDumper("MCP", false);
        }
        return treeDumper;
    }
    
    public void setBounds(int x, int y, int width, int height) {
        this.x = x;
        this.y = y;
        this.width = width;
        this.height = height;
    }
    
    private Navigation2 getNavigation() {
        if (null == navigation) {
            try {
                navigation = (Navigation2)
                getBrowserControl().queryInterface(BrowserControl.NAVIGATION_NAME);
            }
            catch (Throwable th) {
                if (LOGGER.isLoggable(Level.SEVERE)) {
                    LOGGER.throwing(this.getClass().getName(), "getNavigation", 
                            th);
                    LOGGER.severe("Unable to obtain Navigation2 reference from BrowserControl");
                }
            }
        }
        return navigation;
    }
    
    private EventRegistration2 getEventRegistration() {
        if (null == eventRegistration) {
            try {
                eventRegistration = (EventRegistration2)
                getBrowserControl().queryInterface(BrowserControl.EVENT_REGISTRATION_NAME);
            }
            catch (Throwable th) {
                if (LOGGER.isLoggable(Level.SEVERE)) {
                    LOGGER.throwing(this.getClass().getName(), "getEventRegistration", 
                            th);
                    LOGGER.severe("Unable to obtain EventRegistration2 reference from BrowserControl");
                }
            }
            
        }
        return eventRegistration;
    }

    private BrowserControlCanvas getBrowserControlCanvas() {
        if (null == canvas) {
            try {
                canvas = (BrowserControlCanvas)
                getBrowserControl().queryInterface(BrowserControl.BROWSER_CONTROL_CANVAS_NAME);
            }
            catch (Throwable th) {
                if (LOGGER.isLoggable(Level.SEVERE)) {
                    LOGGER.throwing(this.getClass().getName(), "getBrowserControlCanvas", 
                            th);
                    LOGGER.severe("Unable to obtain BrowserControlCanvas reference from BrowserControl");
                }
            }
            
        }
        return canvas;
    }
    
    private CurrentPage2 getCurrentPage() {
        CurrentPage2 currentPage = null;

        try {
            currentPage = (CurrentPage2)
            getBrowserControl().queryInterface(BrowserControl.CURRENT_PAGE_NAME);
        }
        catch (Throwable th) {
            if (LOGGER.isLoggable(Level.SEVERE)) {
                LOGGER.throwing(this.getClass().getName(), "getCurrentPage",
                        th);
                LOGGER.severe("Unable to obtain CurrentPage2 reference from BrowserControl");
            }
        }

        return currentPage;
    }
    
    private PageInfoListener getPageInfoListener() {
        if (null == pageInfoListener) {
            pageInfoListener = new PageInfoListenerImpl(this);
        }
        return pageInfoListener;
    }

    /**

    * <p>Add the argument <code>AjaxListener</code> to this MCP
    * instance.</p>

    */
    
    public void addAjaxListener(AjaxListener listener) {
        getEventRegistration().addDocumentLoadListener(listener);
    }
    
    /**

    * <p>Remove the argument <code>AjaxListener</code> to this MCP
    * instance.</p>

    */
    public void removeAjaxListener(AjaxListener listener) {
        getEventRegistration().removeDocumentLoadListener(listener);
    }

    /**

    * <p>Add the argument <code>MouseListener</code> to the
    * <code>Canvas</code> for this MCP instance.</p>

    */
    public void addMouseListener(MouseListener listener) {
        getBrowserControlCanvas().addMouseListener(listener);
    }
    
    /**

    * <p>Remove the argument <code>MouseListener</code> from the
    * <code>Canvas</code> for this MCP instance.</p>

    */
    public void removeMouseListener(MouseListener listener) {
        getBrowserControlCanvas().removeMouseListener(listener);
    }

    /**

    * <p>Add the argument <code>KeyListener</code> to the
    * <code>Canvas</code> for this MCP instance.</p>

    */
    public void addKeyListener(KeyListener listener) {
        getBrowserControlCanvas().addKeyListener(listener);
    }
    
    /**

    * <p>Remove the argument <code>KeyListener</code> from the
    * <code>Canvas</code> for this MCP instance.</p>

    */
    public void removeKeyListener(KeyListener listener) {
        getBrowserControlCanvas().removeKeyListener(listener);
    }

    /**

    * <p>Return the Webclient <code>BrowserControl</code> instance used
    * by this <code>MCP</code> instance.  This is useful for operations
    * that require more complex browser control than that offered by
    * <code>MCP</code>.</p>

    */
    public BrowserControl getBrowserControl() {
        if (!initialized) {
            IllegalStateException ise = new IllegalStateException("Not initialized.  Call setAppData()");
            if (LOGGER.isLoggable(Level.SEVERE)) {
                LOGGER.throwing(this.getClass().getName(), "getBrowserControl", ise);
            }
            throw ise;
        }
        if (null == browserControl) {
            try {
                browserControl = BrowserControlFactory.newBrowserControl();
            } catch (IllegalStateException ex) {
                if (LOGGER.isLoggable(Level.SEVERE)) {
                    LOGGER.throwing(this.getClass().getName(), "getBrowserControl", ex);
                }
            } catch (InstantiationException ex) {
                if (LOGGER.isLoggable(Level.SEVERE)) {
                    LOGGER.throwing(this.getClass().getName(), "getBrowserControl", ex);
                }
            } catch (IllegalAccessException ex) {
                if (LOGGER.isLoggable(Level.SEVERE)) {
                    LOGGER.throwing(this.getClass().getName(), "getBrowserControl", ex);
                }
            }
        }
        return browserControl;
    }

    /**

    * <p>Return the realized and visible <code>java.awt.Frame</code>
    * containing the actual browser window.  There is no need to put
    * this Frame inside of any surrounding Swing or AWT windows.  It is
    * sufficient to stand alone.</p>

    */
    
    public Frame getRealizedVisibleBrowserWindow() {
        if (null == frame) {
            BrowserControlCanvas canvas = null;
            try {
                canvas = (BrowserControlCanvas) getBrowserControl().queryInterface(BrowserControl.BROWSER_CONTROL_CANVAS_NAME);
            } catch (ClassNotFoundException ex) {
                if (LOGGER.isLoggable(Level.SEVERE)) {
                    LOGGER.throwing(this.getClass().getName(), "createRealizedVisibleBrowserWindow", ex);
                }
            }
            if (null != canvas) {
                frame = new Frame();
                frame.setUndecorated(true);
                frame.setBounds(x, y, width, height);
                frame.add(canvas, BorderLayout.CENTER);
                frame.setVisible(true);
                canvas.setVisible(true);
                
                getEventRegistration().addDocumentLoadListener(getPageInfoListener());
            }
        }
        return frame;
    }

    /**

    * <p>Make invisible, and delete the <code>BrowserControl</code>
    * instance for this MCP instance.  Reset internal state of the
    * instance so that a subsequent call to {@link
    * #getRealizedVisibleBrowserWindow} will create a new
    * <code>Frame</code>.</p>

    */
    
    public void deleteBrowserControl() {
        getRealizedVisibleBrowserWindow().setVisible(false);
        BrowserControlFactory.deleteBrowserControl(getBrowserControl());
	browserControl = null;
	navigation = null;
	eventRegistration = null;
	canvas = null;
	pageInfoListener = null;
	frame = null;
	x = 0;
	y = 0;
	width = 1280;
	height = 960;
	initialized = false;
	robot = null;
	treeDumper = null;
	if (null != latch) {
	    latch.countDown();
	}
	latch = null;
    }

    /**

    * <p>Return the DOM <code>Element</code> with the given id or name.
    * First, <code>Document.getElementById()</code> is called, passing
    * the argument <code>id</code>.  If an element is found, it is
    * returned.  Otherwise, the document is traversed and the first
    * element encountered with a <code>name</code> equal to the argument
    * <code>id</code> is returned.  If no such element exists,
    * <code>null</code> is returned.</p>

    */
    
    public Element findElement(String id) {
        Element result = null;
        Document dom = getCurrentPage().getDOM();
        try {
            result = dom.getElementById(id);
        }
        catch (Exception e) {
            
        }
        if (null == result) {
            result = getDOMTreeDumper().findFirstElementWithName(dom, id);
        }
        
        return result;
    }
    
    /**

     * <p>Given an argument <code>id</code>, return a List of DOM 
     * <code>Element</code> instances that are HTML anchors that are 
     * direct or indirect children of the element with that <code>id</code>.
     */
    public List<Element> getAnchors(String id) {
        List<Element> result = getChildElementsWithTagName(id, "a");
        return result;
    }

    /**

     * <p>Return a List of DOM 
     * <code>Element</code> instances that are HTML anchors that are 
     * present in the document.
     * If no such elements exist,
     * <code>null</code> is returned.</p>
     */
    public List<Element> getAnchors() {
        Element root = getCurrentPage().getDOM().getDocumentElement();
        List<Element> result = getChildElementsWithTagName(root, "a");
        return result;
    }
    
    /**

     * <p>Return a List of DOM <code>Element</code> instances whose
     * tagName attribute is equal to the argument <code>tagName</code>.
     * If no such elements exist,
     * <code>null</code> is returned.</p>
     */
    
    public List<Element> getChildElementsWithTagName(Element root,
            final String tagName) {
        List<Element> results = new ArrayList<Element>();
        if (null != root) {
            TreeTraversalCallBack callback = new TreeTraversalCallBack() {
                public void takeActionOnNode(Node node, Object closure)
                throws AbortTraversalException {
                    List<Element> list = (List<Element>) closure;
                    Element element = null;
                    if (node instanceof Element) {
                        element = (Element) node;
                        String nodeTagName = element.getTagName();
                        if (null != nodeTagName && nodeTagName.equalsIgnoreCase(tagName)){
                            list.add(element);
                        }
                    }
                }
            };
            getDOMTreeDumper().preorderTreeWalk(root, callback, results);
        }
        
        return results;
    }
    /**

     * <p>Return a List of DOM <code>Element</code> instances whose
     * tagName attribute is equal to the argument <code>tagName</code>.
     * If no such elements exist,
     * <code>null</code> is returned.</p>
     */
    
    public List<Element> getChildElementsWithTagName(String id,
            final String tagName) {
        List<Element> results = null;
        Element root = findElement(id);
        results = getChildElementsWithTagName(root, tagName);
        
        return results;
    }

    /**

    * <p>Return <code>true</code> if and only if the argument
    * <code>toFind</code> occurs within the current page.  Case is not
    * significant.  Users desiring more detailed find behavior should
    * use {@link #getBrowserControl} to obtain a reference to the
    * <code>CurrentPage</code> interface, which has more advanced
    * methods pertaining to the current page. </p>

    */
    
    public boolean findInPage(String toFind) {
        boolean found = false;
        found = getCurrentPage().find(toFind, true, false);
        return found;
    }

    /**

    * <p>Find the DOM element within the current page matching the
    * argument <code>id</code> using {@link #findElement}.  Use
    * <code>java.awt.Robot</code> to click the element.  Return
    * immediately after clicking the element.</p>

    */
    
    public void clickElement(Element element) {
        String clientX = null, clientY = null;
        String id = element.getAttribute("id");
        if (null != element) {
            clientX = element.getAttribute("clientX");
            clientY = element.getAttribute("clientY");
            int x,y;
            if (null != clientX && null != clientY) {
                try {
                    x = Integer.valueOf(clientX).intValue();
                    y = Integer.valueOf(clientY).intValue();
                    Robot robot = getRobot();
                    robot.mouseMove(x, y);
                    robot.mousePress(InputEvent.BUTTON1_MASK);
                    robot.mouseRelease(InputEvent.BUTTON1_MASK);
                    startTimeoutCheckIfNecessary();

                } catch (NumberFormatException ex) {
                    LOGGER.throwing(this.getClass().getName(), "clickElementGivenId",
                        ex);
                    ex.printStackTrace();
                    throw new IllegalStateException(ex);
                }
            }
        }
        if (null == element || null == clientX || null == clientY) {
            throw new IllegalStateException("Unable to click element " + id);
        }
    }

    /**

    * <p>Find the DOM element within the current page matching the
    * argument <code>id</code> using {@link #findElement}.  Use
    * <code>java.awt.Robot</code> to click the element.  Return
    * immediately after clicking the element.</p>

    */
    
    public void clickElement(String id) {
        Element element = findElement(id);
        if (null != element) {
            clickElement(element);
        }
    }
    
    
    /**

    * <p>Find the DOM element within the current page matching the
    * argument <code>id</code> using {@link #findElement}.  Use
    * <code>java.awt.Robot</code> to click the element.  Block until the
    * document load triggered by the click has completed.</p>

    */

    
    public void blockingClickElement(Element element) {
        synchronized (this) {
            try {
                createLatch();
                clickElement(element);
                lockLatch();
            }
            catch (InterruptedException ie) {
                if (LOGGER.isLoggable(Level.WARNING)) {
                    LOGGER.log(Level.WARNING, "blockingClickElementGivenId", ie);
                }
            }
        }
    }

    /**

    * <p>Find the DOM element within the current page matching the
    * argument <code>id</code> using {@link #findElement}.  Use
    * <code>java.awt.Robot</code> to click the element.  Block until the
    * document load triggered by the click has completed.</p>

    */

    
    public void blockingClickElement(String idOrName) {
        Element element = findElement(idOrName);
        if (null != element) {
            blockingClickElement(element);
        }
    }

    /**

    * <p>Load the url, blocking until the load has completed.</p>

    */

    
    public void blockingLoad(String url) {
        Navigation2 nav = getNavigation();
        synchronized (this) {
            nav.loadURL(url);
            startTimeoutCheckIfNecessary();
            createLatch();
            try {
                lockLatch();
            } catch (InterruptedException ex) {
                if (LOGGER.isLoggable(Level.WARNING)) {
                    LOGGER.log(Level.WARNING, "blockingLoad", ex);
                }
            }
        }
    }
    
    private Robot getRobot() {
        if (null == robot) {
            try {
                robot = new Robot();
            } catch (AWTException ex) {
                LOGGER.throwing(this.getClass().getName(), "getRobot",
                        ex);
                ex.printStackTrace();
            }
        }
        return robot;
    }
    
    private class PageInfoListenerImpl implements PageInfoListener {
        private MCP owner = null;
        PageInfoListenerImpl(MCP owner) {
            this.owner = owner;
        }

        public void eventDispatched(WebclientEvent webclientEvent) {
            boolean logInfo = LOGGER.isLoggable(Level.INFO);
            Map eventData = 
                    (Map) webclientEvent.getEventData();
            long type = webclientEvent.getType();
            
            switch ((int)type) {
                case ((int) DocumentLoadEvent.END_AJAX_EVENT_MASK):
                case ((int) DocumentLoadEvent.END_DOCUMENT_LOAD_EVENT_MASK):
                    openLatch();
		    if (null != MCP.this.timeoutThread) {
			MCP.this.abortTimeoutCheck();
		    }

                    break;
                case ((int) DocumentLoadEvent.START_URL_LOAD_EVENT_MASK):
                    String method = (String) eventData.get("method");
                    method = (null != method) ? method : "no method given";
                    String uri = (String) eventData.get("URI");
                    uri = (null != uri) ? uri : "no URI given";
                    Map<String,String> requestHeaders = (Map<String,String>)
                        eventData.get("headers");
                    StringBuffer headerValues = null;
                    if (null != requestHeaders) {
                        headerValues = new StringBuffer();
                        for (String cur : requestHeaders.keySet()) {
                            headerValues.append(cur + ": " + 
                                    requestHeaders.get(cur) + "\n");
                        }
                    }
                    headerValues = (null != headerValues) ? headerValues :
                        new StringBuffer("no headers given: ");
                    if (logInfo) {
                        LOGGER.info("HTTP REQUEST:\n" + 
                                method + " " + uri + "\n" + 
                                headerValues);
                    }
                    break;
                default:
                    break;
            }
        }
        
    }
    
    
    private void startTimeoutCheckIfNecessary() {
        // If we are already tracking a timeout...
        if (null != timeoutThread) {
            abortTimeoutCheck();
        }
        if (null != getTimeoutHandler()) {
            timeoutRunnable = new TimeoutRunnable();
            timeoutThread = new Thread(timeoutRunnable,
                    "TimeoutThread-" + getTimeoutHandler().toString());
            timeoutThread.start();
        }
    }
    
    private void abortTimeoutCheck() {
        assert(null != timeoutThread);
        assert(null != timeoutRunnable);
        timeoutRunnable.setRunning(false);
        timeoutThread.interrupt();
        timeoutRunnable = null;
        timeoutThread = null;
    }
    
    private Thread timeoutThread = null;
    private TimeoutRunnable timeoutRunnable = null;

    /**
     * <p>Return the currently installed {@link TimeoutHandler}, if
     * any.</p>
     */

    public TimeoutHandler getTimeoutHandler() {
        return timeoutHandler;
    }

    /**
     * <p>Install an instance of {@link TimeoutHandler} that will be
     * used for all subsequent browser interactions (clicks, loads, Ajax
     * transactions, etc).  To remove the <code>TimeoutHandler</code>,
     * pass <code>null</code> to this method.  If a handler is
     * installed, the timer is automatically started when a browser
     * interaction commences.  If the browser interaction does not
     * complete within {@link #getTimeout} milliseconds, the {@link
     * TimeoutHandler#timeout} method is called on the argument
     * <code>TimeoutHandler</code> instance.</p>
     */ 

    public void setTimeoutHandler(TimeoutHandler timeoutHandler) {
        if (null != timeoutThread) {
            abortTimeoutCheck();
        }
        this.timeoutHandler = timeoutHandler;
    }

    /**
     * <p>Return the number of milliseconds that must elapse before the
     * {@link TimeoutHandler} is called.  Note that a
     * <code>TimeoutHandler</code> is only called if the user has
     * installed one by calling {@link #setTimeoutHandler}.</p>
     */

    public long getTimeout() {
        return Timeout;
    }

    /**
     * <p>Set the number of milliseconds that must elapse before the
     * {@link TimeoutHandler} is called, if such an instance has been
     * installed via a previous call to {@link @setTimeoutHandler}.</p>
     */

    public void setTimeout(long Timeout) {
        this.Timeout = Timeout;
    }
    
        private long waitInterval = 5000L;

        /**
         * <p>Return the number of milliseconds to wait between timeout
         * checks.</p>
         */

        public long getTimeoutWaitInterval() {
            return waitInterval;
        }

        /**
         * <p>Set the number of milliseconds to wait between timeout
         * checks.</p>
         */

        public void setTimeoutWaitInterval(long waitInterval) {
            this.waitInterval = waitInterval;
        }
    
    
    
    private class TimeoutRunnable implements Runnable {
        
        public void run() {
            setRunning(true);
            
            while (isRunning()) {
                try {
                    Thread.currentThread().sleep(MCP.this.getTimeoutWaitInterval());
                } catch (InterruptedException ex) {
                    if (LOGGER.isLoggable(Level.WARNING)) {
                        LOGGER.log(Level.WARNING, "Thread " + 
                                Thread.currentThread().getName() + 
                                " interrupted while sleeping.", ex);
                    }
                    setRunning(false);
                    return;
                }
                if (isTimedout()) {
                    setRunning(false);
                    if (null != MCP.this.getTimeoutHandler()) {
                        MCP.this.getTimeoutHandler().timeout();
                    }
                }
            }
            
        }
        
        private long startTime = -1L;

        private boolean running = false;

        public boolean isRunning() {
            return running;
        }

        public void setRunning(boolean newValue) {
            if (newValue) {
                startTime = System.currentTimeMillis();
            }
            else {
                startTime = -1L;
            }
            
            this.running = newValue;
        }
        
        /**
         * <p>Returns <code>true</code> if {@link isTiming} returns <code>true</code>
         * and the elapsed time between when timing commenced and
         * the current time is greater than the value returned by {@link getTimeout}.
         * Otherwise, returns <code>false</code>.  This method need not be called
         * by the user.
         */

        public boolean isTimedout() {
            boolean result = false;
            if (isRunning()) {
                result = (MCP.this.getTimeout() < (System.currentTimeMillis() - startTime));
            }
            return result;
        }

    }
    
}
