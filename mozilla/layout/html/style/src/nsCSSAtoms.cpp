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
#include "nsCSSAtoms.h"

// define storage for all atoms
#define CSS_ATOM(_name, _value) nsIAtom* nsCSSAtoms::_name;
#include "nsCSSAtomList.h"
#undef CSS_ATOM


static nsrefcnt gRefCnt;

void nsCSSAtoms::AddRefAtoms()
{
  if (0 == gRefCnt++) {
    // create atoms
#define CSS_ATOM(_name, _value) _name = NS_NewAtom(_value);
#include "nsCSSAtomList.h"
#undef CSS_ATOM
  }
}

void nsCSSAtoms::ReleaseAtoms()
{
  NS_PRECONDITION(gRefCnt != 0, "bad release atoms");
  if (--gRefCnt == 0) {
    // release atoms
#define CSS_ATOM(_name, _value) NS_RELEASE(_name);
#include "nsCSSAtomList.h"
#undef CSS_ATOM
  }
}

