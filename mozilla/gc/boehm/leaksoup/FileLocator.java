/* -*- Mode: Java; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express oqr
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s):
 *
 * Patrick C. Beard <beard@netscape.com>
 *
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU Public License (the "GPL"), in which case the
 * provisions of the GPL are applicable instead of those above.
 * If you wish to allow use of your version of this file only
 * under the terms of the GPL and not to allow others to use your
 * version of this file under the NPL, indicate your decision by
 * deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL.  If you do not delete
 * the provisions above, a recipient may use your version of this
 * file under either the NPL or the GPL.
 */

import java.io.*;
import java.util.*;

public class FileLocator {
	static boolean USE_BLAME = false;
	static boolean ASSIGN_BLAME = false;

	static final String MOZILLA_BASE = "mozilla/";
	static final String LXR_BASE = "http://lxr.mozilla.org/seamonkey/source/";
	static final String BONSAI_BASE = "http://bonsai.mozilla.org/cvsblame.cgi?file=";

	static final Hashtable fileTables = new Hashtable();
	static final RevisionTable revisionTable = new RevisionTable();
	static final BlameTable blameTable = new BlameTable();

	public static String getFileLocation(String line) throws IOException {
		int leftBracket = line.indexOf('[');
		if (leftBracket == -1)
			return line;
		int rightBracket = line.indexOf(']', leftBracket + 1);
		if (rightBracket == -1)
			return line;
		int comma = line.indexOf(',', leftBracket + 1);
		String macPath = line.substring(leftBracket + 1, comma);
		String fullPath = "/" + macPath.replace(':', '/');
		
		// compute the line number in the file.
		int offset = 0;
		try {
			offset = Integer.parseInt(line.substring(comma + 1, rightBracket));
		} catch (NumberFormatException nfe) {
			return line;
		}
		FileTable table = (FileTable) fileTables.get(fullPath);
		if (table == null) {
			table = new FileTable(fullPath);
			fileTables.put(fullPath, table);
		}
		int lineNumber = 1 + table.getLine(offset);
		int lineAnchor = lineNumber;

		// compute the URL of the file.
		int mozillaIndex = fullPath.indexOf(MOZILLA_BASE);
		String locationURL = null, blameInfo = "";
		if (mozillaIndex > -1) {
			// if using blame, hilite the line number of the call, and include the revision.
			String mozillaPath = fullPath.substring(mozillaIndex);
			String revision = revisionTable.getRevision(fullPath);
			String bonsaiPath = mozillaPath + (revision.length() > 0 ? "&rev=" + revision : "");
			if (USE_BLAME) {
				locationURL = BONSAI_BASE + bonsaiPath + "&mark=" + lineNumber;
				if (lineAnchor > 10)
					lineAnchor -= 10;
			} else {
				locationURL = LXR_BASE + mozillaPath.substring(MOZILLA_BASE.length());
			}
			if (ASSIGN_BLAME)
				blameInfo = " (" + blameTable.getBlame(bonsaiPath, lineNumber) + ")";
		} else {
			locationURL = "file://" + fullPath;
		}
		
		return "<A HREF=\"" + locationURL + "#" + lineAnchor + "\"TARGET=\"SOURCE\">" + line.substring(0, leftBracket) + "</A>" + blameInfo;
	}
}
