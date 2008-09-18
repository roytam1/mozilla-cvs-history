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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Sun Microsystems,
 * Inc. Portions created by Sun are
 * Copyright (C) 1999 Sun Microsystems, Inc. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */
package org.mozilla.pluglet;

import java.io.InputStream;
import org.mozilla.pluglet.mozilla.*;

public class PlugletStreamListenerAdaptor implements PlugletStreamListener { 
    public void onStartBinding(PlugletStreamInfo streamInfo) {
    }
    public void onDataAvailable(PlugletStreamInfo streamInfo, InputStream input,int  length) {
    }
    public void onFileAvailable(PlugletStreamInfo streamInfo, String fileName) {
    }
    public void onStopBinding(PlugletStreamInfo streamInfo,int status) {
    }
    public int  getStreamType() {
	return PlugletStreamListener.STREAM_TYPE_NORMAL;
    }
}




