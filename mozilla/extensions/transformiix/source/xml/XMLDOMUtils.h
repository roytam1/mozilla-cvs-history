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
 * $Id$
 */


#include "dom.h"
#include "NamespaceResolver.h"
#include "baseutils.h"

#ifndef TRANSFRMX_XMLDOMUTILS_H
#define TRANSFRMX_XMLDOMUTILS_H


/**
 * A utility class for use with XML DOM implementations
**/
class XMLDOMUtils {

public:

    /**
     *  Copies the given Node, using the owner Document to create all
     *  necessary new Node(s)
    **/
   static Node* copyNode(Node* node, Document* owner, NamespaceResolver* resolver);

    /**
     *  Appends the value of the given Node to the target String
    **/
   static void getNodeValue(Node* node, String* target);

   /**
    * Returns the Namespace associated with the given prefix
   **/
   static MBool getNameSpace(const String& prefix, Element* element, String& dest);

}; //-- XMLDOMUtils

#endif

