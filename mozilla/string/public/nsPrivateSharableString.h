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
 * The Original Code is Mozilla strings.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 2000 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s):
 *   Scott Collins <scc@mozilla.org> (original author)
 *
 */

#ifndef nsPrivateSharableString_h___
#define nsPrivateSharableString_h___

#ifndef nsBufferHandle_h___
#include "nsBufferHandle.h"
#endif

  /**
   * This class is part of the machinery that makes
   * most string implementations in this family share their underlying buffers
   * when convenient.  It is _not_ part of the abstract string interface,
   * though other machinery interested in sharing buffers will know about it.
   *
   * Normal string clients must _never_ call routines from this interface.
   */
template <class CharT>
class nsPrivateSharableString
  {
    public:
      virtual ~nsPrivateSharableString() {}

      virtual PRUint32                            GetImplementationFlags() const;
      virtual const nsBufferHandle<CharT>*        GetFlatBufferHandle() const;
      virtual const nsBufferHandle<CharT>*        GetBufferHandle() const;
      virtual const nsSharedBufferHandle<CharT>*  GetSharedBufferHandle() const;

        /**
         * |GetBufferHandle()| will return either |0|, or a reasonable pointer.
         * The meaning of |0| is that the string points to a non-contiguous or else empty representation.
         * Otherwise |GetBufferHandle()| returns a handle that points to the single contiguous hunk of characters
         * that make up this string.
         */
  };

template <class CharT>
const nsSharedBufferHandle<CharT>*
nsPrivateSharableString<CharT>::GetSharedBufferHandle() const
  {
    return 0;
  }

template <class CharT>
const nsBufferHandle<CharT>*
nsPrivateSharableString<CharT>::GetFlatBufferHandle() const
  {
    return GetSharedBufferHandle();
  }

template <class CharT>
const nsBufferHandle<CharT>*
nsPrivateSharableString<CharT>::GetBufferHandle() const
  {
    return GetSharedBufferHandle();
  }

template <class CharT>
PRUint32
nsPrivateSharableString<CharT>::GetImplementationFlags() const
  {
    PRUint32 flags = 0;
    const nsSharedBufferHandle<CharT>* handle = GetSharedBufferHandle();
    if ( handle )
      flags = handle->GetImplementationFlags();
    return flags;
  }

#endif // !defined(nsPrivateSharableString_h___)
