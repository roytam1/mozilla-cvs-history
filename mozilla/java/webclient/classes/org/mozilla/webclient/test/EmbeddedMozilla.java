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
 * Contributor(s): Kirk Baker <kbaker@eb.com>
 *               Ian Wilkinson <iw@ennoble.com>
 *               Mark Goddard
 *               Ed Burns <edburns@acm.org>
 */

package org.mozilla.webclient.test;

/*
 * EmbeddedMozilla.java
 */

import java.awt.*;
import java.awt.event.*;

import org.mozilla.webclient.*;
import org.mozilla.util.Assert;

/**
 *

 * This is a test application for using the BrowserControl.

 *
 * @version $Id$
 * 
 * @see	org.mozilla.webclient.BrowserControlFactory

 */

public class EmbeddedMozilla extends Frame implements ActionListener {
    static final int defaultWidth = 640;
    static final int defaultHeight = 480;

    private TextField		urlField;
	private BrowserControl	browserControl;
	private Panel			controlPanel;
	private Panel			buttonsPanel;

public static void printUsage()
{
    System.out.println("usage: java org.mozilla.webclient.test.EmbeddedMozilla <path> [url]");
    System.out.println("       <path> is the absolute path to the native browser bin directory, ");
    System.out.println("       including the bin.");
}
	
	
    public static void main (String[] arg) {
        if (1 > arg.length) {
            printUsage();
            System.exit(-1);
        }
		String urlArg =(2 == arg.length) ? arg[1] : "http://www.mozilla.org/";
		
		EmbeddedMozilla gecko = 
			new EmbeddedMozilla("Embedded Mozilla", arg[0], urlArg);
    } // main()
    
    public EmbeddedMozilla (String title, String binDir, String url) {
		super(title);
        System.out.println("constructed with binDir: " + binDir + " url: " + 
                           url);
	
		addWindowListener(new WindowAdapter() {
		    public void windowClosing(WindowEvent e) {
				dispose();
				// should close the BrowserControlCanvas
		    }
		    
		    public void windowClosed(WindowEvent e) { 
				System.exit(0);
		    }
		});
	 
		setSize(defaultWidth, defaultHeight);
	
		// Create the URL field
		urlField = new TextField("", 30);
        urlField.addActionListener(this);        	

		// Create the buttons sub panel
		buttonsPanel = new Panel();
        buttonsPanel.setLayout(new GridBagLayout());

		// Add the buttons
		makeItem(buttonsPanel, "Back",    0, 0, 1, 1, 0.0, 0.0);
		makeItem(buttonsPanel, "Forward", 1, 0, 1, 1, 0.0, 0.0);
		makeItem(buttonsPanel, "Stop",    2, 0, 1, 1, 0.0, 0.0);
		makeItem(buttonsPanel, "Refresh", 3, 0, 1, 1, 0.0, 0.0);

		// Create the control panel
		controlPanel = new Panel();
        controlPanel.setLayout(new BorderLayout());
        
        // Add the URL field, and the buttons panel
		controlPanel.add(urlField,     BorderLayout.CENTER);
		controlPanel.add(buttonsPanel, BorderLayout.WEST);

		// Create the browser
        Canvas browserCanvas = null;

        try {
            BrowserControlFactory.setAppData(binDir);
			browserControl = BrowserControlFactory.newBrowserControl();
        }
        catch(Exception e) {
            System.out.println("Can't create BrowserControl: " + 
                               e.getMessage());
        }
        browserCanvas = browserControl.getCanvas();
        Assert.assert(null != browserCanvas);
		browserCanvas.setSize(defaultWidth, defaultHeight);
	
		// Add the control panel and the browserCanvas
		add(controlPanel, BorderLayout.NORTH);
		add(browserCanvas,      BorderLayout.CENTER);
		
		pack();
		show();
		toFront();
	
		try {
	        browserControl.loadURL(url);
			urlField.setText(url);
		}
		catch (Exception e) {
		    System.out.println(e.toString());
		}
    } // EmbeddedMozilla() ctor


    public void actionPerformed (ActionEvent evt) {
    	String command = evt.getActionCommand();
    	
    	try {
	    	if (command.equals("Back")) {
	    		if (browserControl.canBack()) {
		    		browserControl.back();
		    		int index = browserControl.getHistoryIndex();
		    		String newURL = browserControl.getURL(index);
		    		
				    System.out.println(newURL);
		    		
		    		urlField.setText(newURL);
		    	}
	    	}
	    	else if (command.equals("Forward")) {
	    		if (browserControl.canForward()) {
	    			browserControl.forward();
		    		int index = browserControl.getHistoryIndex();
		    		String newURL = browserControl.getURL(index);
		    		
				    System.out.println(newURL);
		    		
		    		urlField.setText(newURL);
	    		}
	    	}
	    	else if (command.equals("Stop")) {
	    		browserControl.stop();
	    	}
            else if (command.equals("Refresh")) {
                browserControl.refresh();
            }
	    	else {
		        browserControl.loadURL(urlField.getText());
		    }
		}
		catch (Exception e) {
		    System.out.println(e.toString());
		}
    } // actionPerformed()


    private void makeItem (Panel p, Object arg, int x, int y, int w, int h, double weightx, double weighty) {
        GridBagLayout gbl = (GridBagLayout) p.getLayout();
        GridBagConstraints c = new GridBagConstraints();
        Component comp;

        c.fill = GridBagConstraints.BOTH;
        c.gridx = x;
        c.gridy = y;
        c.gridwidth = w;
        c.gridheight = h;
        c.weightx = weightx;
        c.weighty = weighty;
        if (arg instanceof String) {
        	Button b;
        	
            comp = b = new Button((String) arg);
	        b.addActionListener(this);

		    p.add(comp);
		    gbl.setConstraints(comp, c);
        }
    } // makeItem()

}

// EOF
