/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

package com.netscape.jsdebugging.api.corba;

import com.netscape.jsdebugging.api.*;

public class JSErrorReporterCorba 
    extends com.netscape.jsdebugging.remote.corba._sk_IJSErrorReporter
{
    private static int _objectCounter = 0;

    public JSErrorReporterCorba(JSErrorReporter er)
    {
        super( "JSErrorReporterCorba_"+_objectCounter++);
        _er = er;
    }

    public int reportError( String msg,
                            String filename,
                            int    lineno,
                            String linebuf,
                            int    tokenOffset )
    {
        return _er.reportError(msg, filename, lineno, linebuf, tokenOffset);
    }

    public JSErrorReporter getWrappedHook() {return _er;}

    private JSErrorReporter _er;
}    
