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

package org.mozilla.webclient;

// BrowserControlCore.java

import java.awt.Canvas;
import java.awt.Rectangle;

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

public void createWindow(int windowPtr, Rectangle bounds) throws Exception;

public Canvas getCanvas();

} // end of interface BrowserControlCore


