/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

/*   gnomestr.c --- gnome fe handling of string id's */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define RESOURCE_STR
#include "gnomefe-strings.h"

#include "ntypes.h"

extern char *XP_GetBuiltinString(int16 i);

char *
XP_GetString(int16 i)
{
  char		*ret;

  if ((ret = mcom_cmd_gnome_gnome_err_h_strings (i + RES_OFFSET)))
    {
      return ret;
    }

  return XP_GetBuiltinString(i);
}
