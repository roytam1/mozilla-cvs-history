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


#include <new.h>		// for placement new
#include <MacMemory.h>

#include "nsMemAllocator.h"
#include "nsSmallHeapAllocator.h"

const UInt32 SmallHeapBlock::kBlockOverhead = sizeof(SmallHeapBlock) + MEMORY_BLOCK_TAILER_SIZE;


//--------------------------------------------------------------------
nsSmallHeapAllocator::nsSmallHeapAllocator(THz heapZone)
:	nsMemAllocator(heapZone)
//--------------------------------------------------------------------
{
	mBaseChunkSize = mTempChunkSize = (nsMemAllocator::kChunkSizeMultiple);
}

//--------------------------------------------------------------------
nsSmallHeapAllocator::~nsSmallHeapAllocator()
//--------------------------------------------------------------------
{
}


//--------------------------------------------------------------------
void *nsSmallHeapAllocator::AllocatorMakeBlock(size_t blockSize)
//--------------------------------------------------------------------
{
	nsSmallHeapChunk		*chunk = (nsSmallHeapChunk *)mFirstChunk;
	
	// walk through all of our chunks, trying to allocate memory from somewhere
	while (chunk)
	{
		void	*theBlock = chunk->GetSpaceForBlock(blockSize);
		
		if (theBlock)
			return theBlock;
		
		chunk = (nsSmallHeapChunk *)chunk->GetNextChunk();
	}       // while (chunk != nil)

	chunk = (nsSmallHeapChunk *)AllocateChunk(blockSize);
	if (!chunk) return nil;
	
	return chunk->GetSpaceForBlock(blockSize);
}


//--------------------------------------------------------------------
void nsSmallHeapAllocator::AllocatorFreeBlock(void *freeBlock)
//--------------------------------------------------------------------
{
	SmallHeapBlock		*deadBlock = SmallHeapBlock::GetBlockHeader(freeBlock);

#if DEBUG_HEAP_INTEGRITY
	MEM_ASSERT(deadBlock->HasHeaderTag(kUsedBlockHeaderTag), "Bad block header on free");
	MEM_ASSERT(deadBlock->HasTrailerTag(deadBlock->GetBlockSize(), kUsedBlockTrailerTag), "Bad block trailer on free");
	MEM_ASSERT(deadBlock->CheckPaddingBytes(), "Block has overwritten its bounds");
#endif
	
	nsSmallHeapChunk	*chunk = deadBlock->GetOwningChunk();
	
#if DEBUG_HEAP_INTEGRITY
	deadBlock->SetHeaderTag(kFreeBlockHeaderTag);
	deadBlock->SetTrailerTag(deadBlock->GetBlockSize(), kFreeBlockTrailerTag);
#endif

	chunk->ReturnBlock(deadBlock);
	
	// if this chunk is completely empty and it's not the first chunk then free it
	if (chunk->IsEmpty() && chunk!= mFirstChunk)
		FreeChunk(chunk);
}


//--------------------------------------------------------------------
void *nsSmallHeapAllocator::AllocatorResizeBlock(void *block, size_t newSize)
//--------------------------------------------------------------------
{
	SmallHeapBlock		*blockHeader = SmallHeapBlock::GetBlockHeader(block);
	nsSmallHeapChunk	*chunk = blockHeader->GetOwningChunk();
	
#if DEBUG_HEAP_INTEGRITY
	MEM_ASSERT(blockHeader->HasHeaderTag(kUsedBlockHeaderTag), "Bad block header on realloc");
	MEM_ASSERT(blockHeader->HasTrailerTag(blockHeader->GetBlockSize(), kUsedBlockTrailerTag), "Bad block trailer on realloc");
	MEM_ASSERT(blockHeader->CheckPaddingBytes(), "Block has overwritten its bounds");
#endif

	UInt32		newAllocSize = (newSize + 3) & ~3;

	// we can resize this block to any size, provided it fits.
	
	if (newAllocSize < blockHeader->GetBlockSize())					// shrinking
	{
		return chunk->ShrinkBlock(blockHeader, newSize);
	}
	else if (newAllocSize > blockHeader->GetBlockSize())			// growing
	{
		return chunk->GrowBlock(blockHeader, newSize);
	}
	else
	{
		return chunk->ResizeBlockInPlace(blockHeader, newSize);
	}
	
	return nil;
}


//--------------------------------------------------------------------
size_t nsSmallHeapAllocator::AllocatorGetBlockSize(void *thisBlock)
//--------------------------------------------------------------------
{
	SmallHeapBlock	*allocBlock = SmallHeapBlock::GetBlockHeader(thisBlock);
#if DEBUG_HEAP_INTEGRITY
	MEM_ASSERT(allocBlock->HasHeaderTag(kUsedBlockHeaderTag), "Bad block header on get block size");
#endif
	return allocBlock->GetBlockSize();
}

//--------------------------------------------------------------------
nsHeapChunk *nsSmallHeapAllocator::AllocateChunk(size_t blockSize)
//--------------------------------------------------------------------
{
	UInt32	minChunkSize = ((blockSize + 3) & ~3) + sizeof(nsSmallHeapChunk) + 3 * SmallHeapBlock::kBlockOverhead;
	
	if (minChunkSize < mBaseChunkSize)
		minChunkSize = mBaseChunkSize;
		
	Size	actualChunkSize;
	Handle	tempMemHandle;
	Ptr		chunkMemory = DoMacMemoryAllocation(minChunkSize, actualChunkSize, &tempMemHandle);
	
	// use placement new to initialize the chunk in the memory block
	nsHeapChunk		*newHeapChunk = new (chunkMemory) nsSmallHeapChunk(this, actualChunkSize, tempMemHandle);
	
	if (newHeapChunk)
		AddToChunkList(newHeapChunk);
	
	return newHeapChunk;
}


//--------------------------------------------------------------------
void nsSmallHeapAllocator::FreeChunk(nsHeapChunk *chunkToFree)
//--------------------------------------------------------------------
{
	RemoveFromChunkList(chunkToFree);
	// we used placement new to make it, so we have to delete like this
	nsSmallHeapChunk	*thisChunk = (nsSmallHeapChunk *)chunkToFree;
	thisChunk->~nsSmallHeapChunk();
	
	Handle	tempMemHandle = thisChunk->GetMemHandle();
	if (tempMemHandle)
		DisposeHandle(tempMemHandle);
	else
		DisposePtr((Ptr)thisChunk);
}


#pragma mark -

//--------------------------------------------------------------------
nsSmallHeapChunk::nsSmallHeapChunk(
			nsMemAllocator 	*inOwningAllocator,
			Size 			heapSize,
			Handle 			tempMemHandle) :
	nsHeapChunk(inOwningAllocator, heapSize, tempMemHandle),
	mOverflow(nil)
//--------------------------------------------------------------------
{
	// init the bin ptrs
	for (UInt32	count = 0; count < kDefaultSmallHeapBins; ++count )
		mBins[count] = nil;
	
	// mark how much we can actually store in the heap
	heapSize -= sizeof(nsSmallHeapChunk);		// subtract heap overhead
	mHeapSize = heapSize - 3 * SmallHeapBlock::kBlockOverhead;
	
	SmallHeapBlock	*newRawBlockHeader = mMemory;

	//	The first few bytes of the block are a dummy header
	//	which is a block of size zero that is always allocated.  
	//	This allows our coalesce code to work without modification
	//	on the edge case of coalescing the first real block.

	newRawBlockHeader->SetPrevBlock(nil);
	newRawBlockHeader->SetBlockSize(0);
	newRawBlockHeader->SetBlockUsed();
	newRawBlockHeader->SetOwningChunk(nil);
	
	SmallHeapBlock	*newFreeOverflowBlock = newRawBlockHeader->GetNextBlock();
	
	newFreeOverflowBlock->SetPrevBlock(newRawBlockHeader);
	newFreeOverflowBlock->SetBlockSize(heapSize - 3 * SmallHeapBlock::kBlockOverhead);

	//	The same is true for the last few bytes in the block as well.
	
	SmallHeapBlock	*newRawBlockTrailer = (SmallHeapBlock *)(((Ptr)newRawBlockHeader) + heapSize - SmallHeapBlock::kBlockOverhead);
	newRawBlockTrailer->SetPrevBlock(newFreeOverflowBlock);
	newRawBlockTrailer->SetBlockSize(0xFFFFFFFF);
	newRawBlockTrailer->SetBlockUsed();
	newRawBlockTrailer->SetOwningChunk(nil);
	
	AddBlockToFreeList(newFreeOverflowBlock);
}


//--------------------------------------------------------------------
nsSmallHeapChunk::~nsSmallHeapChunk()
//--------------------------------------------------------------------
{
}


//--------------------------------------------------------------------
void *nsSmallHeapChunk::GetSpaceForBlock(size_t blockSize)
//--------------------------------------------------------------------
{
	//	Round up allocation to nearest 4 bytes.
	UInt32 roundedBlockSize = (blockSize + 3) & ~3;
	MEM_ASSERT(roundedBlockSize <= nsSmallHeapChunk::kMaximumBlockSize, "Block is too big for this allocator!");

	//	Try to find the best fit in one of the bins.
	UInt32	startingBinNum = (roundedBlockSize - kDefaultSmallHeadMinSize) >> 2;
	
	SmallHeapBlock	**currentBin = GetBins(startingBinNum);
	SmallHeapBlock	*currentBinValue = *currentBin;
	SmallHeapBlock	*allocatedBlock = nil;
	
	//	If the current bin has something in it,
	//	then use it for our allocation.
	
	if (currentBinValue)
	{	
		RemoveBlockFromFreeList(currentBinValue);
		MEM_ASSERT(currentBinValue->GetBlockSize() >= roundedBlockSize, "Got a smaller block than requested");
		allocatedBlock = currentBinValue;
		goto done;
	}

	//	Otherwise, try to carve up an existing larger block.

	UInt32	remainingBins = kDefaultSmallHeapBins - startingBinNum - 1;
	SmallHeapBlock	*blockToCarve = nil;
	
	while (remainingBins--)
	{
		currentBin++;
		currentBinValue = *currentBin;
		
		if (currentBinValue != nil)
		{
			blockToCarve = currentBinValue;
			break;
		}
	}

	//	Try carving up up a block from the overflow bin.
	
	if (blockToCarve == nil)
		blockToCarve = GetOverflow();
	
	while (blockToCarve != nil)
	{
		SInt32	blockToCarveSize = blockToCarve->GetBlockSize();
	
		//	If the owerflow block is big enough to house the
		//	allocation, then use it.
	
		if (blockToCarveSize >= roundedBlockSize)
		{
			//	Remove the block from the free list... we will
			//	be using it for the allocation...
			
			RemoveBlockFromFreeList(blockToCarve);

			//	If taking our current allocation out of
			//	the overflow block would still leave enough
			//	room for another allocation out of the 
			//	block, then split the block up.
			
			//blockToCarve->SetUnusedBlockSize(roundedBlockSize);
			//leftovers = blockToCarveSize - blockToCarve->GetBlockHeapUsage();
			SInt32	leftovers = blockToCarveSize - roundedBlockSize - SmallHeapBlock::kBlockOverhead;

			if (leftovers >= kDefaultSmallHeadMinSize)
			{
				SmallHeapBlock	*nextBlock = blockToCarve->GetNextBlock();
				
				//	Create a new block out of the leftovers
				
				SmallHeapBlock	*leftoverBlock = (SmallHeapBlock *)((char *)blockToCarve + roundedBlockSize + SmallHeapBlock::kBlockOverhead); 
			
				//	Add the block to linked list of blocks in this raw
				//	allocation chunk.
			
				nextBlock->SetPrevBlock(leftoverBlock);
				
				blockToCarve->SetBlockSize(roundedBlockSize);
				blockToCarve->SetBlockUsed();
					
				leftoverBlock->SetPrevBlock(blockToCarve);
				leftoverBlock->SetBlockSize(leftovers);
				
				//	And add the block to a free list, which will either be
				//	one of the sized bins or the overflow list, depending
				//	on its size.
			
				AddBlockToFreeList(leftoverBlock);
			}
			// else...
			// if we didn't carve up the block, then we are returning a block that is bigger
			// than the requested size. That's fine, we just need to be careful about where
			// to put the trailer tag, because we only know the allocated size, and not the
			// requested size.
			
			allocatedBlock = blockToCarve;
			goto done;
		} 
	
		blockToCarve = blockToCarve->GetNextFree();
	}

	return nil;

done:

	allocatedBlock->SetOwningChunk(this);

#if DEBUG_HEAP_INTEGRITY
	allocatedBlock->SetHeaderTag(kUsedBlockHeaderTag);
	allocatedBlock->SetTrailerTag(allocatedBlock->GetBlockSize(), kUsedBlockTrailerTag);
	allocatedBlock->SetPaddingBytes(roundedBlockSize - blockSize);
	allocatedBlock->FillPaddingBytes();
#endif

#if STATS_MAC_MEMORY
	allocatedBlock->SetLogicalBlockSize(blockSize);
	GetOwningAllocator()->AccountForNewBlock(blockSize);
#endif

	IncrementUsedBlocks();
	return &allocatedBlock->memory;
}



//--------------------------------------------------------------------
void *nsSmallHeapChunk::GrowBlock(SmallHeapBlock *growBlock, size_t newSize)
//--------------------------------------------------------------------
{
	UInt32			newAllocSize = (newSize + 3) & ~3;
	
	SmallHeapBlock	*blockToCarve = growBlock->GetNextBlock();

	if (! blockToCarve->IsBlockUsed())
	{
		UInt32		oldPhysicalSize = growBlock->GetBlockHeapUsage();
		UInt32		newPhysicalSize = SmallHeapBlock::kBlockOverhead + newAllocSize;

		UInt32		blockToCarveSize = blockToCarve->GetBlockSize();
		UInt32		blockToCarvePhysicalSize = blockToCarve->GetBlockHeapUsage();
		
		SInt32		leftovers = (SInt32)oldPhysicalSize + blockToCarvePhysicalSize - newPhysicalSize;
					
		// enough space to grow into the free block?
		if (leftovers < 0)
			return nil;

		//	Remove the block from the free list... we will
		//	be using it for the allocation...
		
		RemoveBlockFromFreeList(blockToCarve);

		//	If taking our current allocation out of
		//	the overflow block would still leave enough
		//	room for another allocation out of the 
		//	block, then split the block up.
			
		if (leftovers >= kDefaultSmallHeadMinSize + SmallHeapBlock::kBlockOverhead)
		{
			//	Create a new block out of the leftovers
			SmallHeapBlock* leftoverBlock = (SmallHeapBlock *)((char *)growBlock + newPhysicalSize);
			SmallHeapBlock* nextBlock = blockToCarve->GetNextBlock();
		
			//	Add the block to linked list of blocks in this raw allocation chunk.
		
			nextBlock->SetPrevBlock(leftoverBlock);	
			growBlock->SetBlockSize(newAllocSize);
			leftoverBlock->SetPrevBlock(growBlock);
			leftoverBlock->SetBlockSize(leftovers - SmallHeapBlock::kBlockOverhead);
			
			//	And add the block to a free list, which will either be
			//	one of the sized bins or the overflow list, depending
			//	on its size.
		
			AddBlockToFreeList(leftoverBlock);					
		}
		else
		{
			SmallHeapBlock* nextBlock = blockToCarve->GetNextBlock();
			nextBlock->SetPrevBlock(growBlock);
			// If we're using the entire free block, then because growBlock->blockSize
			// is used to calculate the start of the next block, it must be
			// adjusted so that still does this.
			growBlock->SetBlockSize(growBlock->GetBlockSize() + blockToCarvePhysicalSize);
		}
	
#if DEBUG_HEAP_INTEGRITY
		growBlock->SetPaddingBytes(newAllocSize - newSize);
		growBlock->FillPaddingBytes();
		growBlock->SetTrailerTag(growBlock->GetBlockSize(), kUsedBlockTrailerTag);
#endif

#if STATS_MAC_MEMORY
		GetOwningAllocator()->AccountForResizedBlock(growBlock->GetLogicalBlockSize(), newSize);
		growBlock->SetLogicalBlockSize(newSize);
#endif

		return (void *)(&growBlock->memory);
	}
	
	return nil;
}


//--------------------------------------------------------------------
void *nsSmallHeapChunk::ShrinkBlock(SmallHeapBlock *shrinkBlock, size_t newSize)
//--------------------------------------------------------------------
{
	UInt32			newAllocSize = (newSize + 3) & ~3;
	UInt32			oldSize = shrinkBlock->GetBlockSize();
	UInt32			oldPhysicalSize = shrinkBlock->GetBlockHeapUsage();
	UInt32			newPhysicalSize = SmallHeapBlock::kBlockOverhead + newAllocSize;
	
	SInt32			leftovers = (SInt32)oldPhysicalSize - newPhysicalSize;

	MEM_ASSERT(leftovers > 0, "Should have some leftovers here");

	SmallHeapBlock		*nextBlock = shrinkBlock->GetNextBlock();

	if (! nextBlock->IsBlockUsed())
	{
		// coalesce
		leftovers += nextBlock->GetBlockHeapUsage(); // augmented leftovers.
		RemoveBlockFromFreeList(nextBlock);
		nextBlock = nextBlock->GetNextBlock();
	}
	else
	{			
		// enough space to turn into a free block?
		if (leftovers < SmallHeapBlock::kBlockOverhead + kDefaultSmallHeadMinSize)
			return (void *)(&shrinkBlock->memory);
	}	

	//	Create a new block out of the leftovers (or augmented leftovers)
	SmallHeapBlock		*leftoverBlock = (SmallHeapBlock *)((char *)shrinkBlock + newPhysicalSize);

	//	Add the block to linked list of blocks in this raw
	//	allocation chunk.

	nextBlock->SetPrevBlock(leftoverBlock);	
	shrinkBlock->SetBlockSize(newAllocSize);
	leftoverBlock->SetPrevBlock(shrinkBlock);
	leftoverBlock->SetBlockSize(leftovers -  SmallHeapBlock::kBlockOverhead);
	
	//	And add the block to a free list, which will either be
	//	one of the sized bins or the overflow list, depending
	//	on its size.

	AddBlockToFreeList(leftoverBlock);

#if DEBUG_HEAP_INTEGRITY
	shrinkBlock->SetPaddingBytes(newAllocSize - newSize);
	shrinkBlock->FillPaddingBytes();
	shrinkBlock->SetTrailerTag(shrinkBlock->GetBlockSize(), kUsedBlockTrailerTag);
#endif

#if STATS_MAC_MEMORY
	GetOwningAllocator()->AccountForResizedBlock(shrinkBlock->GetLogicalBlockSize(), newSize);
	shrinkBlock->SetLogicalBlockSize(newSize);
#endif

	return (void *)(&shrinkBlock->memory);
}


//--------------------------------------------------------------------
void* nsSmallHeapChunk::ResizeBlockInPlace(SmallHeapBlock *theBlock, size_t newSize)
//--------------------------------------------------------------------
{
#if DEBUG_HEAP_INTEGRITY
	UInt32		newAllocSize = (newSize + 3) & ~3;

	theBlock->SetPaddingBytes(newAllocSize - newSize);
	theBlock->FillPaddingBytes();
#endif

#if STATS_MAC_MEMORY
	GetOwningAllocator()->AccountForResizedBlock(theBlock->GetLogicalBlockSize(), newSize);
	theBlock->SetLogicalBlockSize(newSize);
#endif

	return (void *)(&theBlock->memory);
}


//--------------------------------------------------------------------
void nsSmallHeapChunk::ReturnBlock(SmallHeapBlock *deadBlock)
//--------------------------------------------------------------------
{
	SmallHeapBlock		*nextBlock = deadBlock->GetNextBlock();
	SmallHeapBlock		*prevBlock = deadBlock->GetPrevBlock();
	
#if STATS_MAC_MEMORY
	GetOwningAllocator()->AccountForFreedBlock(deadBlock->GetLogicalBlockSize());
#endif

	//	If the block after us is free, then coalesce with it.
	if (! nextBlock->IsBlockUsed())
	{	
		RemoveBlockFromFreeList(nextBlock);
		deadBlock->SetBlockSize( deadBlock->GetBlockSize() + nextBlock->GetBlockHeapUsage());
		nextBlock = nextBlock->GetNextBlock();
	}
	
	//	If the block before us is free, then coalesce with it.
	if (! prevBlock->IsBlockUsed())
	{
		RemoveBlockFromFreeList(prevBlock);
		prevBlock->SetBlockSize( ((Ptr)nextBlock - (Ptr)prevBlock) - SmallHeapBlock::kBlockOverhead);
		AddBlockToFreeList(prevBlock);
		deadBlock = prevBlock;
	}
	else
	{
		AddBlockToFreeList(deadBlock);
	}
	
	nextBlock->SetPrevBlock(deadBlock);

	DecrementUsedBlocks();
}


//--------------------------------------------------------------------
void nsSmallHeapChunk::RemoveBlockFromFreeList(SmallHeapBlock *removeBlock)
//--------------------------------------------------------------------
{
	UInt32	blockSize = removeBlock->GetBlockSize();
	
	SmallHeapBlock	*nextFree = removeBlock->GetNextFree();
	SmallHeapBlock	*prevFree = removeBlock->GetPrevFree();
	
	if (nextFree != nil)
		nextFree->SetPrevFree(prevFree);
	if (prevFree != nil)
		prevFree->SetNextFree(nextFree);
	else
	{
		if (blockSize > kMaximumBinBlockSize)
			mOverflow = nextFree;
		else
		{
			UInt32	nextBlockBin = (blockSize - kDefaultSmallHeadMinSize) >> 2;
#if DEBUG_HEAP_INTEGRITY
			if (blockSize < kDefaultSmallHeadMinSize)
				DebugStr("\pBad block size");
			if (nextBlockBin >= kDefaultSmallHeapBins)
				DebugStr("\pBad bin index");
#endif
			mBins[nextBlockBin] = nextFree;
		}
	}
	
	removeBlock->SetBlockUsed();
}



//--------------------------------------------------------------------
void nsSmallHeapChunk::AddBlockToFreeList(SmallHeapBlock *addBlock)
//--------------------------------------------------------------------
{
	addBlock->SetBlockUnused();

	UInt32	blockSize = addBlock->GetBlockSize();
	
	addBlock->SetPrevFree(nil);
	
	if (blockSize > kMaximumBinBlockSize)
	{
		SmallHeapBlock	*tempBlock = mOverflow;
		addBlock->SetNextFree(tempBlock);
		if (tempBlock) tempBlock->SetPrevFree(addBlock);
		mOverflow = addBlock;
	}
	else
	{
		UInt32	nextBlockBin = (blockSize - kDefaultSmallHeadMinSize) >> 2;
#if DEBUG_HEAP_INTEGRITY
		if (blockSize < kDefaultSmallHeadMinSize)
			DebugStr("\pBad block size");
		if (nextBlockBin >= kDefaultSmallHeapBins)
			DebugStr("\pBad bin index");
#endif
		SmallHeapBlock	*tempBlock = mBins[nextBlockBin];
		addBlock->SetNextFree(tempBlock);
		if (tempBlock) tempBlock->SetPrevFree(addBlock);
		mBins[nextBlockBin] = addBlock;
	}
}

