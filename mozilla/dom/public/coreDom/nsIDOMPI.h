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

#ifndef nsIDOMPI_h__
#define nsIDOMPI_h__

#include "nsDOM.h"
#include "nsIDOMNode.h"


class nsIDOMPI : public nsIDOMNode {
public:
  //attribute UniString        name;
  virtual UniString           GetName() = 0;
  virtual void                SetName(UniString name) = 0;
  //attribute UniString        data;
  virtual UniString           GetData() = 0;
  virtual void                SetData(UniString data) = 0;
};

#endif // nsIDOMPI_h__

