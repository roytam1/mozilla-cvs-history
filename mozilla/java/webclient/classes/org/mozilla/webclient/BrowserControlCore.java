/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * 
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The original code is RaptorCanvas
 * 
 * The Initial Developer of the Original Code is Kirk Baker <kbaker@eb.com> and * Ian Wilkinson <iw@ennoble.com
 */

package org.mozilla.webclient;

// BrowserControlCore.java

/**
 *
 *  <B>BrowserControlCore</B> Defines the core methods for browsing
 *

 * @version $Id$
 * 

 * @see	org.mozilla.webclient.BrowserControlExtended
 * @see	org.mozilla.webclient.BrowserControl
 *
 */

public interface BrowserControlCore
{

public void loadURL(String urlString) throws Exception;

public void stop() throws Exception;

public boolean canBack() throws Exception;

public boolean canForward() throws Exception;

public boolean back() throws Exception;

public boolean forward() throws Exception;

// added by Mark Goddard OTMP 9/2/1999
public boolean refresh() throws Exception;

public int getNativeWebShell();

} // end of interface BrowserControlCore


