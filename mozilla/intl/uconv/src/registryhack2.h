/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License.  You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are Copyright (C) 1998
 * Netscape Communications Corporation.  All Rights Reserved.
 */

  mDecSize           = 6;
  mDecArray          = new ConverterInfo [mDecSize];

  mDecArray[0].mCID  = &kLatin1ToUnicodeCID;
  mDecArray[1].mCID  = &kSJIS2UnicodeCID;
  mDecArray[2].mCID  = &kISO88597ToUnicodeCID;
  mDecArray[3].mCID  = &kCP1253ToUnicodeCID;
  mDecArray[4].mCID  = &kISO2022JPToUnicodeCID;
  mDecArray[5].mCID  = &kEUCJPToUnicodeCID;
  
  mEncSize           = 1;
  mEncArray          = new ConverterInfo [mEncSize];

  mEncArray[0].mCID  = &kUnicodeToLatin1CID;
