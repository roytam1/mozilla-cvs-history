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

// Force references to all of the symbols that we want exported from
// the dll that are located in the .lib files we link with

#include "nsVoidArray.h"
#include "nsIAtom.h"
#include "nsFileSpec.h"
//#include "nsIBuffer.h"
//#include "nsIByteBufferInputStream.h"
#include "nsFileStream.h"
#include "nsFileSpecStreaming.h"
#include "nsSpecialSystemDirectory.h"
#include "nsIThread.h"
#include "nsDeque.h"
#include "nsObserver.h"
#include "nsTraceRefcnt.h"
#include "nsXPIDLString.h"
#include "nsIEnumerator.h"
#include "nsEnumeratorUtils.h"
#include "nsQuickSort.h"
#include "nsString2.h"
#include "nsProxyEventPrivate.h"
#include "xpt_xdr.h"
#include "nsInterfaceInfo.h"
#include "xptcall.h"
#include "nsIFileSpec.h"
#include "nsIGenericFactory.h"
#include "nsAVLTree.h"
#include "nsHashtableEnumerator.h"
#include "nsPipe2.h"
#include "nsCWeakReference.h"
#include "nsWeakReference.h"
#include "nsISizeOfHandler.h"
#include "nsTextFormater.h"

class dummyComparitor: public nsAVLNodeComparitor {
public:
  virtual PRInt32 operator()(void* anItem1,void* anItem2)
  {
    return 0;
  }
}; 

#ifdef DEBUG
extern NS_COM void
TestSegmentedBuffer();
#endif

void XXXNeverCalled()
{
    nsTextFormater::snprintf(nsnull,0,nsnull);
    dummyComparitor dummy;
    nsVoidArray();
    nsAVLTree(dummy, nsnull);
    NS_GetNumberOfAtoms();
    nsFileURL(NULL);
//    NS_NewPipe(NULL, NULL, 0, 0, 0, NULL);
    NS_NewPipe(NULL, NULL, NULL, 0, 0);
    nsFileSpec s;
    NS_NewIOFileStream(NULL, s, 0, 0);
    nsInputFileStream(s, 0, 0);
    nsPersistentFileDescriptor d;
    ReadDescriptor(NULL, d);
    new nsSpecialSystemDirectory(nsSpecialSystemDirectory::OS_DriveDirectory);
    nsIThread::GetCurrent(NULL);
    nsDeque(NULL);
    NS_NewObserver(NULL, NULL);
    nsTraceRefcnt::DumpAllStatistics();
    nsXPIDLCString::Copy(NULL);
    NS_NewEmptyEnumerator(NULL);
    nsArrayEnumerator(NULL);
    NS_NewIntersectionEnumerator(NULL, NULL, NULL);
    NS_QuickSort(NULL, 0, 0, NULL, NULL);
    nsString2();
    nsProxyObject();
    XPT_DoString(NULL, NULL);
    XPT_DoHeader(NULL, NULL);
    nsInterfaceInfo* info = NULL;
    info->GetName(NULL);
#ifdef DEBUG
    info->print(NULL);
#endif
    XPTC_InvokeByIndex(NULL, 0, 0, NULL);
    NS_NewFileSpec(NULL);
    xptc_dummy();
    xptc_dummy2();
    XPTI_GetInterfaceInfoManager();
    NS_NewGenericFactory(NULL, NULL, NULL);
    NS_NewHashtableEnumerator(NULL, NULL, NULL, NULL);
    nsCWeakProxy(0, 0);
    nsCWeakReferent(0);
    NS_GetWeakReference(NULL);
#ifdef DEBUG
    TestSegmentedBuffer();
#endif
    NS_NewSizeOfHandler(0);
}
