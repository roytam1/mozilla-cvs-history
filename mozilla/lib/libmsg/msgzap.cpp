/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
#include "msg.h"
#include "xp_mcom.h"
#include "msgzap.h"

#if defined(XP_OS2) && defined(__DEBUG_ALLOC__)
void* MSG_ZapIt::operator new(size_t size, const char *file, size_t line) {
  void* rv = ::operator new(size, file, line);
  if (rv) {
    XP_MEMSET(rv, 0, size);
  }
  return rv;
}
#else
void* MSG_ZapIt::operator new(size_t size) {
  void* rv = ::operator new(size);
  if (rv) {
    XP_MEMSET(rv, 0, size);
  }
  return rv;
}

#endif
