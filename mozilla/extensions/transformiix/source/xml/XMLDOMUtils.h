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
 * The Original Code is TransforMiiX XSLT processor.
 * 
 * The Initial Developer of the Original Code is The MITRE Corporation.
 * Portions created by MITRE are Copyright (C) 1999 The MITRE Corporation.
 *
 * Portions created by Keith Visco as a Non MITRE employee,
 * (C) 1999 Keith Visco. All Rights Reserved.
 * 
 * Contributor(s): 
 * Keith Visco 
 *    -- original author.
 *
 */

#ifndef TRANSFRMX_XMLDOMUTILS_H
#define TRANSFRMX_XMLDOMUTILS_H

#include "baseutils.h"

class Node;
class String;

/*
 * A utility class for use with XML DOM implementations
 */
class XMLDOMUtils
{
public:
    /*
     *  Appends the value of the given node to aResult.
     */
    static void getNodeValue(Node* aNode, String& aResult);
};

#endif
