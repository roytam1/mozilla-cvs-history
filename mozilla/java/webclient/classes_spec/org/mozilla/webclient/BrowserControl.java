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
 * The Original Code is RaptorCanvas.
 *
 * The Initial Developer of the Original Code is Kirk Baker and
 * Ian Wilkinson. Portions created by Kirk Baker and Ian Wilkinson are
 * Copyright (C) 1999 Kirk Baker and Ian Wilkinson. All
 * Rights Reserved.
 *
 * Contributor(s): Kirk Baker <kbaker@eb.com>
 *               Ian Wilkinson <iw@ennoble.com>
 *               Ann Sunhachawee
 */

package org.mozilla.webclient;

// BrowserControl.java

/**
 *

 * Provides a common access point for other webclient interfaces.  There
 * is one instance of this class per application-level browser window.
 * Instances must be created using BrowserControlFactory.


 *
 * @version $Id$
 * 
 * @see org.mozilla.webclient.BrowserControlFactory
 * */

public interface BrowserControl 
{

public static String BOOKMARKS_NAME = "webclient.Bookmarks";
public static String BROWSER_CONTROL_CANVAS_NAME = "webclient.BrowserControlCanvas";
public static String CURRENT_PAGE_NAME = "webclient.CurrentPage";
public static String EVENT_REGISTRATION_NAME = "webclient.EventRegistration";
public static String EXTENDED_EVENT_REGISTRATION_NAME = "webclient.ExtendedEventRegistration";
public static String HISTORY_NAME = "webclient.History";
public static String NAVIGATION_NAME = "webclient.Navigation";
public static String CACHE_MANAGER_NAME = "webclient.cache.NetDataCacheManager";
public static String PREFERENCES_NAME = "webclient.Preferences";
public static String PRINT_NAME = "webclient.Print";
public static String WINDOW_CONTROL_NAME = "webclient.WindowControl";



    /**

     * @param interfaceName valid values for <code>interfaceName</code>
     * are any of the *_NAME class variables in this interface.

     */

public Object queryInterface(String interfaceName) throws ClassNotFoundException;

} // end of interface BrowserControl
