/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 *      Gagan Saksena <gagan@netscape.com>
 */
#ifndef nsHTTPAtoms_h___
#define nsHTTPAtoms_h___

#include "nsIAtom.h"

/**
 * This class wraps up the creation (and destruction) of the standard
 * set of HTTP atoms used during normal HTTP handling. This objects
 * are created when the first HTTP handler is created
 * are destroyed when the last HTTP handler is destroyed.
 */
class nsHTTPAtoms {
public:

  static void AddRefAtoms();
  static void ReleaseAtoms();

  /* Declare all atoms

     The atom names and values are stored in nsHTTPAtomList.h and
     are brought to you by the magic of C preprocessing

     Add new atoms to nsHTPAtomList and all support logic will be auto-generated
   */
#define HTTP_ATOM(_name, _value) static nsIAtom* _name;
#include "nsHTTPAtomList.h"
#undef HTTP_ATOM
};

#endif /* nsHTTPAtoms_h___ */
