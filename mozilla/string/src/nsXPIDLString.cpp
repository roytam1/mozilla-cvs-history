/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 * The Original Code is Mozilla.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications.  Portions created by Netscape Communications are
 * Copyright (C) 2001 by Netscape Communications.  All
 * Rights Reserved.
 * 
 * Contributor(s): 
 *   Scott Collins <scc@mozilla.org> (original author)
 */

#include "nsXPIDLString.h"

#if DEBUG_STRING_STATS
size_t nsXPIDLString::sCreatedCount     = 0;
size_t nsXPIDLString::sAliveCount       = 0;
size_t nsXPIDLString::sHighWaterCount   = 0;
size_t nsXPIDLString::sAssignCount      = 0;
size_t nsXPIDLString::sShareCount       = 0;

size_t nsXPIDLCString::sCreatedCount    = 0;
size_t nsXPIDLCString::sAliveCount      = 0;
size_t nsXPIDLCString::sHighWaterCount  = 0;
size_t nsXPIDLCString::sAssignCount     = 0;
size_t nsXPIDLCString::sShareCount      = 0;
#endif


template <class CharT>
class nsImportedStringHandle
    : public nsFlexBufferHandle<CharT>
  {
    public:
      nsImportedStringHandle() : nsFlexBufferHandle<CharT>(0, 0, 0, 0) { }

      CharT** AddressOfDataStart() { return &(this->mDataStart); }
      void RecalculateBoundaries() const;
  };


template <class CharT>
void
nsImportedStringHandle<CharT>::RecalculateBoundaries() const
  {
    size_t data_length = 0;
    size_t storage_length = 0;

    CharT* data_start = NS_CONST_CAST(CharT*, this->DataStart());
    if ( data_start )
      {
        data_length = nsCharTraits<CharT>::length(data_start);
        storage_length = data_length + 1;
      }

    nsImportedStringHandle<CharT>* mutable_this = NS_CONST_CAST(nsImportedStringHandle<CharT>*, this);
    mutable_this->DataEnd(data_start+data_length);

    mutable_this->StorageStart(data_start);
    mutable_this->StorageEnd(data_start+storage_length);
  }


#if DEBUG_STRING_STATS
const nsBufferHandle<PRUnichar>*
nsXPIDLString::GetFlatBufferHandle() const
  {
    --sShareCount;
    return GetSharedBufferHandle();
  }


const nsBufferHandle<PRUnichar>*
nsXPIDLString::GetBufferHandle() const
  {
    --sShareCount;
    return GetSharedBufferHandle();
  }


void
nsXPIDLString::DebugPrintStats( FILE* aOutFile )
  {
    fprintf(aOutFile, "nsXPIDLString stats: %ld alive now [%ld max] of %ld created; %ld getter_Copies, %ld attempts to share\n",
                       sAliveCount, sHighWaterCount, sCreatedCount, sAssignCount, sShareCount);
  }
#endif


const nsSharedBufferHandle<PRUnichar>*
nsXPIDLString::GetSharedBufferHandle() const
  {
    const nsImportedStringHandle<PRUnichar>* answer = NS_STATIC_CAST(const nsImportedStringHandle<PRUnichar>*, mBuffer.get());
    
    if ( answer && !answer->DataEnd() )
      answer->RecalculateBoundaries();

#if DEBUG_STRING_STATS
    ++sShareCount;
#endif
    return answer;
  }


PRUnichar**
nsXPIDLString::PrepareForUseAsOutParam()
  {
    nsImportedStringHandle<PRUnichar>* handle = new nsImportedStringHandle<PRUnichar>();
    NS_ASSERTION(handle, "Trouble!  We couldn't get a new handle during |getter_Copies|.");

    mBuffer = handle;
#if DEBUG_STRING_STATS
    ++sAssignCount;
#endif
    return handle->AddressOfDataStart();
  }


#if DEBUG_STRING_STATS
const nsBufferHandle<char>*
nsXPIDLCString::GetFlatBufferHandle() const
  {
    --sShareCount;
    return GetSharedBufferHandle();
  }


const nsBufferHandle<char>*
nsXPIDLCString::GetBufferHandle() const
  {
    --sShareCount;
    return GetSharedBufferHandle();
  }


void
nsXPIDLCString::DebugPrintStats( FILE* aOutFile )
  {
    fprintf(aOutFile, "nsXPIDLCString stats: %ld alive now [%ld max] of %ld created; %ld getter_Copies, %ld attempts to share\n",
                       sAliveCount, sHighWaterCount, sCreatedCount, sAssignCount, sShareCount);
  }
#endif


const nsSharedBufferHandle<char>*
nsXPIDLCString::GetSharedBufferHandle() const
  {
    const nsImportedStringHandle<char>* answer = NS_STATIC_CAST(const nsImportedStringHandle<char>*, mBuffer.get());
    
    if ( answer && !answer->DataEnd() )
      answer->RecalculateBoundaries();

#if DEBUG_STRING_STATS
    ++sShareCount;
#endif
    return answer;
  }


char**
nsXPIDLCString::PrepareForUseAsOutParam()
  {
    nsImportedStringHandle<char>* handle = new nsImportedStringHandle<char>();
    NS_ASSERTION(handle, "Trouble!  We couldn't get a new handle during |getter_Copies|.");

    mBuffer = handle;
#if DEBUG_STRING_STATS
    ++sAssignCount;
#endif
    return handle->AddressOfDataStart();
  }
