/* -*- Mode: java; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License.  You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is the Grendel mail/news client.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are Copyright (C) 1997
 * Netscape Communications Corporation.  All Rights Reserved.
 */

package grendel.composition;

import java.awt.*;
import com.sun.java.swing.JTabbedPane;
import com.sun.java.swing.basic.BasicTabbedPaneUI;
import com.sun.java.swing.*;
import com.sun.java.swing.plaf.*;

import netscape.orion.toolbars.*;
import pk.core.*;

public class NSTabbedPane extends BaseDelegate implements INSToolbar, IFloatingToolbar {
    protected JTabbedPane   fTabbedPane;
    protected Object        fID = null;
    protected String        fName = null;
    protected String        fFloatingTitle = "";

    public NSTabbedPane() {
        fTabbedPane = new JTabbedPane();
        fTabbedPane.setUI(new MyTabbedPaneUI());
    }

    /**
    * the toolbar is about to be docked
    */
    public void dock() {
    }

    /**
    * tells the toolbar it is about to float
    */
    public void floatToolbar() {
    }

    /**
    * sets the title to be used when the toolbar is floating
    */
    public void setFloatingTitle(String aTitle) {
        fFloatingTitle = aTitle;
    }

    /**
    * gets the title used when the toolbar is floating
    */
    public String getFloatingTitle() {
        return fFloatingTitle;
    }

    /**
     * adds a new tab.
     */
    public void addTab(String aLabel, ImageIcon aIcon, Component mAddressList) {
        fTabbedPane.addTab(aLabel, aIcon, mAddressList);
    }

    /**
     * set tab selection
     */
    public void setSelectedIndex(int aIndex) {
        fTabbedPane.setSelectedIndex (aIndex);
    }

    /**
    * sets an ID for this toolbar
    */
    public void setID(Object anID) {
        fID = anID;
    }

    /**
    * gets the toolbar's id
    */
    public Object getID() {
        return fID;
    }

    /**
    * sets the name of the toolbar
    */
    public void setName(String aName) {
        fName = aName;
    }

    /**
    * gets the name of the toolbar
    */
    public String getName() {
        return fName;
    }

    /**
    * gets the component associated with the toolbar
    */
    public Component getComponent() {
        return fTabbedPane;
    }

    //***************************
    public class MyTabbedPaneUI extends BasicTabbedPaneUI {
        public Dimension getPreferredSize(JComponent container) {
            int widest = 0;
            int tallest = 0;

            //return the component with the largst preferred size.
            Component[] comps = fTabbedPane.getComponents();
            for (int i = 0; i < comps.length; i++) {
                Dimension dim = comps[i].getPreferredSize();

                if (dim.width > widest)
                    widest = dim.width;

                if (dim.height > tallest)
                    tallest = dim.height;
            }

            return new Dimension(widest, tallest);
        }
    }
}

