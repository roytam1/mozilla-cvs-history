/* -*- Mode: C++;    tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- 
 * 
 * The contents of this file are subject to the Netscape Public License 
 * Version 1.0 (the "NPL"); you may not use this file except in 
 * compliance with the NPL. You may obtain a copy of the NPL at  
 * http://www.mozilla.org/NPL/ 
 * 
 * Software distributed under the NPL is distributed on an "AS IS" basis, 
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL 
 * for the specific language governing rights and limitations under the 
 * NPL. 
 * 
 * The Initial Developer of this code under the NPL is Netscape 
 * Communications Corporation. Portions created by Netscape are 
 * Copyright (C) 1998 Netscape Communications Corporation. All Rights    
 * Reserved. */

#include <MacMemory.h>

#include <string.h>
#include <stdlib.h>


#ifdef __cplusplus
extern "C" {
#endif

void *malloc(size_t blockSize);
void free(void *deadBlock);
void* realloc(void* block, size_t newSize);
void *calloc(size_t nele, size_t elesize);

#ifdef __cplusplus
}
#endif


// NewPtr implementation of malloc/free, for testing purposes

//--------------------------------------------------------------------
void *malloc(size_t blockSize)
//--------------------------------------------------------------------
{
	return (void *)::NewPtr(blockSize);
}


//--------------------------------------------------------------------
void free(void *deadBlock)
//--------------------------------------------------------------------
{
	if (deadBlock)
		::DisposePtr((Ptr)deadBlock);
}


//--------------------------------------------------------------------
void* realloc(void* block, size_t newSize)
//--------------------------------------------------------------------
{
	::SetPtrSize((Ptr)block, newSize);
	if (MemError() == noErr)
		return block;
	
	void*	newBlock = ::NewPtr(newSize);
	if (!newBlock) return nil;
	
	BlockMoveData(block, newBlock, newSize);	// might copy off the end of block,
												// but who cares?
	
	return newBlock;
}

//--------------------------------------------------------------------
void *calloc(size_t nele, size_t elesize)
//--------------------------------------------------------------------
{
	size_t	space = nele * elesize;
	void	*newBlock = ::malloc(space);
	if (newBlock)
		memset(newBlock, 0, space);
	return newBlock;
}

