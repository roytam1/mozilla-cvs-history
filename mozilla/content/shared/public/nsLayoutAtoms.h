/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */
#ifndef nsLayoutAtoms_h___
#define nsLayoutAtoms_h___

#include "nsIAtom.h"

/**
 * This class wraps up the creation (and destruction) of the standard
 * set of atoms used during layout processing. These objects
 * are created when the first presentation context is created and they
 * are destroyed when the last presentation context object is destroyed.
 */

class nsLayoutAtoms {
public:

  static void AddrefAtoms();
  static void ReleaseAtoms();

  // Alphabetical list of atoms
  static nsIAtom* all;
  static nsIAtom* aural;

  static nsIAtom* braille;

  static nsIAtom* embossed;

  static nsIAtom* handheld;

  static nsIAtom* print;
  static nsIAtom* projection;

  static nsIAtom* screen;

  static nsIAtom* tty;
  static nsIAtom* tv;
};

#endif /* nsLayoutAtoms_h___ */
