/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

package netscape.jsdebug;

/**
* Exception to indicate bad thread state etc...
*
* @author  John Bandhauer
* @author  Nick Thompson
* @version 1.0
* @since   1.0
*/
public class InvalidInfoException extends Exception {
    /**
     * Constructs a InvalidInfoException without a detail message.
     * A detail message is a String that describes this particular exception.
     */
    public InvalidInfoException() {
	super();
    }

    /**
     * Constructs a InvalidInfoException with a detail message.
     * A detail message is a String that describes this particular exception.
     * @param s the detail message
     */
    public InvalidInfoException(String s) {
	super(s);
    }
}
