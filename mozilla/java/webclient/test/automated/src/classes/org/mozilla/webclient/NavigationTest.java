/*
 * $Id$
 */

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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Sun
 * Microsystems, Inc. Portions created by Sun are
 * Copyright (C) 1999 Sun Microsystems, Inc. All
 * Rights Reserved.
 *
 * Contributor(s): Ed Burns &lt;edburns@acm.org&gt;
 */

package org.mozilla.webclient;

import junit.framework.TestSuite;
import junit.framework.Test;

import java.util.Enumeration;

import java.awt.Frame;
import java.awt.BorderLayout;

import java.io.File;
import java.io.FileInputStream;

// NavigationTest.java

public class NavigationTest extends WebclientTestCase {

    public NavigationTest(String name) {
	super(name);
    }

    public static Test suite() {
	return (new TestSuite(NavigationTest.class));
    }

    static EventRegistration2 eventRegistration;

    static boolean keepWaiting;

    //
    // Constants
    // 

    //
    // Testcases
    // 

    public void testNavigation() throws Exception {
	BrowserControl firstBrowserControl = null;
	Selection selection = null;
	BrowserControlFactory.setAppData(getBrowserBinDir());
	firstBrowserControl = BrowserControlFactory.newBrowserControl();
	assertNotNull(firstBrowserControl);
	BrowserControlCanvas canvas = (BrowserControlCanvas)
	    firstBrowserControl.queryInterface(BrowserControl.BROWSER_CONTROL_CANVAS_NAME);
	eventRegistration = (EventRegistration2)
	    firstBrowserControl.queryInterface(BrowserControl.EVENT_REGISTRATION_NAME);

	assertNotNull(canvas);
	Frame frame = new Frame();
	frame.setUndecorated(true);
	frame.setBounds(0, 0, 640, 480);
	frame.add(canvas, BorderLayout.CENTER);
	frame.setVisible(true);
	canvas.setVisible(true);
	
	Navigation2 nav = (Navigation2) 
	    firstBrowserControl.queryInterface(BrowserControl.NAVIGATION_NAME);
	assertNotNull(nav);
	final CurrentPage2 currentPage = (CurrentPage2) 
          firstBrowserControl.queryInterface(BrowserControl.CURRENT_PAGE_NAME);
	
	assertNotNull(currentPage);

	File testPage = new File(getBrowserBinDir(), 
				 "../../java/webclient/test/automated/src/test/NavigationTest.txt");
	
	//
	// try loading a file: url
	//
	
	NavigationTest.keepWaiting = true;
	
	System.out.println("Loading url: " + testPage.toURL().toString());
	eventRegistration.addDocumentLoadListener(new EndDocumentSelectionVerifier() {
		public void doEndCheck() {
		    currentPage.selectAll();
		    Selection selection = currentPage.getSelection();
		    assertTrue(-1 != selection.toString().indexOf("This test file is for the NavigationTest."));
		    System.out.println("Selection is: " + 
				       selection.toString());
		    NavigationTest.keepWaiting = false;
		}
	    });
	nav.loadURL(testPage.toURL().toString());

	// keep waiting until the previous load completes
	while (NavigationTest.keepWaiting) {
	    Thread.currentThread().sleep(1000);
	}

	/*******************

	NavigationTest.keepWaiting = true;
	//
	// try loading from the dreaded RandomHTMLInputStream
	//
	RandomHTMLInputStream rhis = new RandomHTMLInputStream(10, false);
	
	eventRegistration.addDocumentLoadListener(new EndDocumentSelectionVerifier() {
		public void doEndCheck() {
		    currentPage.selectAll();
		    Selection selection = currentPage.getSelection();
		    assertTrue(-1 != selection.toString().indexOf("START Random Data"));
		    assertTrue(-1 != selection.toString().indexOf("END Random Data"));
		    System.out.println("Selection is: " + selection.toString());
		    NavigationTest.keepWaiting = false; 
		}
	    });
	nav.loadFromStream(rhis, "http://randomstream.com/",
			   "text/html", -1, null);

	// keep waiting until the previous load completes
	while (NavigationTest.keepWaiting) {
	    Thread.currentThread().sleep(1000);
	}
	
	//
	// try loading from a FileInputStream
	//
	NavigationTest.keepWaiting = true;

	FileInputStream fis = new FileInputStream(testPage);
	eventRegistration.addDocumentLoadListener(new EndDocumentSelectionVerifier() {
		public void doEndCheck() {
		    currentPage.selectAll();
		    Selection selection = currentPage.getSelection();
		    assertTrue(-1 != selection.toString().indexOf("This test file is for the NavigationTest."));
		    System.out.println("Selection is: " + 
				       selection.toString());
		    NavigationTest.keepWaiting = false;
		}
	    });
	nav.loadFromStream(fis, "http://somefile.com/",
			   "text/html", -1, null);
	// keep waiting until the previous load completes
	while (NavigationTest.keepWaiting) {
	    Thread.currentThread().sleep(1000);
	}
	*******************/

	frame.setVisible(false);
	BrowserControlFactory.deleteBrowserControl(firstBrowserControl);
	BrowserControlFactory.appTerminate();
    }

    public static abstract class EndDocumentSelectionVerifier implements DocumentLoadListener {

	public void eventDispatched(WebclientEvent event) {
	    if (event instanceof DocumentLoadEvent) {
		switch ((int) event.getType()) {
		case ((int) DocumentLoadEvent.END_DOCUMENT_LOAD_EVENT_MASK):
		    NavigationTest.eventRegistration.removeDocumentLoadListener(this);
		    doEndCheck();
		    break;
		}
	    }
	}	
	
	public abstract void doEndCheck();
    }

}
