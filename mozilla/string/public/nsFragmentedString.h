/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
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
 * Original Author:
 *   Scott Collins <scc@mozilla.org>
 *
 * Contributor(s):
 */

#ifndef nsFragmentedString_h___
#define nsFragmentedString_h___

  // WORK IN PROGRESS

#ifndef nsAWritableString_h___
#include "nsAWritableString.h"
#endif

template <class CharT>
class basic_nsFragmentedString
      : public basic_nsAWritableString<CharT>
    /*
      ...
    */
  {
    public:
      // ...
  };

#endif // !defined(nsFragmentedString_h___)
