/*
 * dynamic_allocator.c
 *
 *  Created on: Sep 21, 2023
 *      Author: HP
 */
#include <inc/assert.h>
#include <inc/string.h>
#include "../inc/dynamic_allocator.h"


//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//

//=====================================================
// 1) GET BLOCK SIZE (including size of its meta data):
//=====================================================
__inline__ uint32 get_block_size(void* va)
{
	uint32 *curBlkMetaData = ((uint32 *)va - 1) ;
	return (*curBlkMetaData) & ~(0x1);
}

//===========================
// 2) GET BLOCK STATUS:
//===========================
__inline__ int8 is_free_block(void* va)
{
	uint32 *curBlkMetaData = ((uint32 *)va - 1) ;
	return (~(*curBlkMetaData) & 0x1) ;
}

//===========================
// 3) ALLOCATE BLOCK:
//===========================

void *alloc_block(uint32 size, int ALLOC_STRATEGY)
{
	void *va = NULL;
	switch (ALLOC_STRATEGY)
	{
	case DA_FF:
		va = alloc_block_FF(size);
		break;
	case DA_NF:
		va = alloc_block_NF(size);
		break;
	case DA_BF:
		va = alloc_block_BF(size);
		break;
	case DA_WF:
		va = alloc_block_WF(size);
		break;
	default:
		cprintf("Invalid allocation strategy\n");
		break;
	}
	return va;
}

//===========================
// 4) PRINT BLOCKS LIST:
//===========================

void print_blocks_list(struct MemBlock_LIST list)
{
	cprintf("=========================================\n");
	struct BlockElement* blk ;
	cprintf("\nDynAlloc Blocks List:\n");
	LIST_FOREACH(blk, &list)
	{
		cprintf("(size: %d, isFree: %d)\n", get_block_size(blk), is_free_block(blk)) ;
	}
	cprintf("=========================================\n");

}
//
////********************************************************************************//
////********************************************************************************//

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//

struct MemBlock_LIST myHeapDA;
bool is_initialized = 0;
uint32* daEnd;
//==================================
// [1] INITIALIZE DYNAMIC ALLOCATOR:
//==================================
void initialize_dynamic_allocator(uint32 daStart, uint32 initSizeOfAllocatedSpace)
{
	//DON'T CHANGE THESE LINES==========================================================
	{
		if (initSizeOfAllocatedSpace % 2 != 0) initSizeOfAllocatedSpace++; //ensure it's multiple of 2
		// because size must be even => min block size is 16 B
		if (initSizeOfAllocatedSpace == 0)
			return ;
		is_initialized = 1;
	}

	//TODO: [PROJECT'24.MS1 - #04] [3] DYNAMIC ALLOCATOR - initialize_dynamic_allocator
	uint32* daBeg = (uint32*)  daStart;
	struct BlockElement* ff_blck = (struct BlockElement*) (daStart + 2*sizeof(int));
	daEnd = (uint32*) (/*heap start*/ daStart +  initSizeOfAllocatedSpace - sizeof(int));

	LIST_INIT(&myHeapDA);
	*daBeg = 1; // 0000000 0001
	*daEnd = 1;

	uint32* blkHeader = (uint32*) (daStart + sizeof(int));
	uint32* blkFooter = (uint32*) (daStart + initSizeOfAllocatedSpace - 2*sizeof(int));

	*blkHeader = (initSizeOfAllocatedSpace - 2 * sizeof(int)) & ~(0x1);

    *blkFooter = (initSizeOfAllocatedSpace - 2 * sizeof(int)) & ~(0x1);

	LIST_INIT(&freeBlocksList);

	LIST_INSERT_HEAD(&myHeapDA, ff_blck);
	LIST_INSERT_HEAD(&freeBlocksList, ff_blck);
}

//==================================
// [2] SET BLOCK HEADER & FOOTER:
//==================================

void set_block_data(void* va, uint32 totalSize, bool isAllocated)
{
	uint32 *header = (uint32*)va - 1;

	*header = (totalSize | isAllocated);
	void *ft = va + totalSize - 2*sizeof(int);
	uint32 *footer = (uint32 *)ft;
	// make footer eq header
	*footer = *header;
}

void set_virtual_addrx(struct BlockElement* new_one)
{
	struct BlockElement* cur_blck = NULL;  // cur block ptr
	struct BlockElement* prv_one = NULL; // prev block ptr

	LIST_FOREACH(cur_blck, &freeBlocksList) {
		if ((uint32)cur_blck > (uint32)new_one) {
			break;
		}
		prv_one = cur_blck;
	}
	// add new block to fb list at its pos
	if (prv_one == NULL)
	{
		LIST_INSERT_HEAD(&freeBlocksList, new_one);
	}
	else if (cur_blck == NULL)
	{
		LIST_INSERT_TAIL(&freeBlocksList, new_one);
	}
	else
	{
		LIST_INSERT_AFTER(&freeBlocksList, prv_one, new_one);
	}
}

//=========================================
// [3] ALLOCATE BLOCK BY FIRST FIT:
//=========================================

void *alloc_block_FF(uint32 size)
{
	//cprintf("\nInside ff\n");
	if (size == 0) return NULL;
	//==================================================================================
	//DON'T CHANGE THESE LINES==========================================================
	//==================================================================================
	{
		if (size % 2 != 0) size++;	//ensure that the size is even (to use LSB as allocation flag)
		if (size < DYN_ALLOC_MIN_BLOCK_SIZE)
			size = DYN_ALLOC_MIN_BLOCK_SIZE ;
		if (!is_initialized)
		{
			uint32 required_size = size + 2*sizeof(int) /*header & footer*/ + 2*sizeof(int) /*da begin & end*/ ;
			uint32 da_start = (uint32)sbrk(ROUNDUP(required_size, PAGE_SIZE)/PAGE_SIZE);
			uint32 da_break = (uint32)sbrk(0);
			initialize_dynamic_allocator(da_start, da_break - da_start);
		}
	}

	//TODO: [PROJECT'24.MS1 - #06] [3] DYNAMIC ALLOCATOR - alloc_block_FF
	//COMMENT THE FOLLOWING LINE BEFORE START CODING0
	// Cases or Senario:
	//	1] if size of block is equaled to size of block
	//	2] if size of block is so large => split to 2 blocks
	//	3] if size of block is a bit large => make the remaining internal frag.
	//	4] if size of block is not enough => call sbrk()

	struct BlockElement* ptr_belo2;
	uint32 req_size = 2*sizeof(int) + size; // size of block we need to allocate + its (h & f)
	LIST_FOREACH(ptr_belo2, &freeBlocksList)
	{
		// cache block info (size & allocation flag)
		uint32 size_of_blck = get_block_size((void*)ptr_belo2);
		uint8 is_blck_free = is_free_block((void*)ptr_belo2); // if 1 then free, else allocated

		if(ptr_belo2 == NULL) {
			//cprintf("\nel ptr_belo2 bynulllllllll\n");
			return NULL;
		}
		// check for case >
		if(size_of_blck >= req_size && is_blck_free == 1)
		{
			//cprintf("inside 1st if\n");
			struct BlockElement* new_one = (struct BlockElement*)((void*)ptr_belo2 + req_size);
			//cprintf("after make new_block DONE\n");
			uint32 size_newblck = size_of_blck - req_size;
			set_block_data(ptr_belo2, req_size, 1);
			//cprintf("after set block data DONE\n");
			LIST_REMOVE(&freeBlocksList, ptr_belo2); // we will remove at anyway
			//cprintf("after remove from freeBlockList DONE\n");

			// case of remaining is equal or greater than min size of block
			if(size_newblck >= 4*sizeof(int) /*16B*/)
			{
				//cprintf("inside if DONE\n");
			    set_block_data(new_one, size_newblck, 0);
				//cprintf("after set block data DONE\n");
			    set_virtual_addrx(new_one);
				//cprintf("after add in fbl DONE\n");
			}
			// case of internal fragmentation "a bit large"
			else
			{
				// reset data of actual block => ptr_blk
				//cprintf("inside else DONE\n");
				set_block_data(ptr_belo2, size_of_blck, 1);
				//cprintf("after el else DONE\n");
			}
			return ptr_belo2;
		}
		// check for case =
		else if(size_of_blck == req_size && is_blck_free == 1)
		{
			//cprintf("inside 2nd if\n");
			set_block_data((void*)ptr_belo2, req_size, 1);
			LIST_REMOVE(&freeBlocksList, ptr_belo2);
			return ptr_belo2;
		}
	}
	// must call sbrk() to create more space on heap
	// must increase block size with the required size

	/* Cases after call sbrk
	 * case 1: if -1 then must ret null
	 * 		   else:
	 * 		   		1] get req size
	 * 		   		2] move end block
	 * 		   		3] check if there's free block at da
	 * 		   		4] if true must coalesce then allocate req size
	 * 		   		5] else just allocate
	 * */

	//cprintf("call sbrk\n");
	uint32 seg_brck = (uint32)sbrk(0);
	uint32 rounded_size = ROUNDUP(req_size, PAGE_SIZE);
	uint32 numOfPages = rounded_size/PAGE_SIZE;
	void *ret = sbrk(numOfPages);
	//cprintf("after return from sbrk\n");

	if (ret == (void*)-1) return NULL;
	//cprintf("before check if returned value != null\n");
	if (ret != NULL)
	{
		uint32 *last_end = daEnd; // cache last end block before we move it
		//cprintf("daEnd before updates = %d\n",daEnd);

		daEnd = (uint32*)(ret + rounded_size - sizeof(int));
		*daEnd = 1;
		/*cprintf("daEnd after updates= %d\n",daEnd);
		cprintf("*daEnd data = %d\n",*daEnd);
		cprintf("\nlast end = %d\n", last_end);
		cprintf("last end data = %d\n", *last_end);*/

		uint32 *footer_last = last_end - 1; // get footer of last block to check if its free or not
		/*cprintf("last footer = %d\n", footer_last);
		cprintf("last footer data = %d\n", *footer_last);*/

		int8 is_last_free = *footer_last & 1; //000000001 if 1 => a else f
		uint32 block_size = *footer_last & ~1; // 111111111110

		if(is_last_free == 0) // then coalesce
		{
			//cprintf("\n1\n");
			struct BlockElement *belo2 = (struct BlockElement*)((void*)last_end - block_size + sizeof(int));

			uint32 total_size = block_size + rounded_size;
			uint32 remain_size = total_size - req_size;

			/*cprintf("is_last_free = %d\n", is_last_free);
			cprintf("block_size = %d\n", block_size);
			cprintf("req_size = %d\n", req_size);
			cprintf("remain = %d\n", remain_size);
			cprintf("rounded_size = %d\n", rounded_size);
			cprintf("total size = %d\n", total_size);*/

			set_block_data((void*)belo2, req_size, 1);
			LIST_REMOVE(&freeBlocksList, belo2);

			struct BlockElement* new_one = (struct BlockElement*)((void*)belo2 + req_size);
			struct BlockElement* new_one_seg = (struct BlockElement*)((void*)seg_brck);
			struct BlockElement* new_one_sbrk = (struct BlockElement*)(ret);

			/*cprintf("new_one = %x\n", new_one);
			cprintf("new_one_seg = %x\n", new_one_seg);
			cprintf("new_one_sbrk = %x\n", new_one_sbrk);*/

			if(remain_size >= 4*sizeof(int) /*16B*/)
			{
				//cprintf("inside if\n");
				set_block_data(new_one, remain_size, 0);
				set_virtual_addrx(new_one);
			}
			else
			{
				//cprintf("inside else\n");
				set_block_data((void*)belo2, total_size, 1);
			}
			return belo2;
		}
		else // then directly allocate
		{
			//cprintf("2\n");
			if(rounded_size > req_size)
			{
				struct BlockElement* cur_one = (struct BlockElement*)((void*)last_end + sizeof(int));
				uint32 size_newblck = rounded_size - req_size;
				set_block_data((void*)cur_one, req_size, 1);

				if(size_newblck >= 4*sizeof(int) /*16B*/) {
					struct BlockElement* new_one = (struct BlockElement*)((void*)cur_one + req_size);
					set_block_data(new_one, size_newblck, 0);
					set_virtual_addrx(new_one);
				}
				else
				{
					set_block_data((void*)cur_one, rounded_size, 1);
				}
				return cur_one;
			}
		}
	}
	return NULL;
}

//=========================================
// [4] ALLOCATE BLOCK BY BEST FIT:
//=========================================

void *alloc_block_BF(uint32 size)
{
	//==================================================================================
	//DON'T CHANGE THESE LINES==========================================================
	//==================================================================================
		{
			if (size % 2 != 0) size++;	//ensure that the size is even (to use LSB as allocation flag)
			if (size < DYN_ALLOC_MIN_BLOCK_SIZE)
				size = DYN_ALLOC_MIN_BLOCK_SIZE ;
			if (!is_initialized)
			{
				uint32 required_size = size + 2*sizeof(int) /*header & footer*/ + 2*sizeof(int) /*da begin & end*/ ;
				uint32 da_start = (uint32)sbrk(ROUNDUP(required_size, PAGE_SIZE)/PAGE_SIZE);
				uint32 da_break = (uint32)sbrk(0);
				initialize_dynamic_allocator(da_start, da_break - da_start);
			}
		}
		//TODO: [PROJECT'24.MS1 - BONUS] [3] DYNAMIC ALLOCATOR - alloc_block_BF
		//COMMENT THE FOLLOWING LINE BEFORE START CODING
		//panic("alloc_block_BF is not implemented yet");
		//Your Code is Here...
		if (size == 0) return NULL;
		struct BlockElement* ptr_belo2; // iterator

		struct BlockElement* min_belo2 ; // temp ptr that catch block of suitable min size
		uint32 min_size_of_blck = UINT_MAX;

		uint32 req_size = 2*sizeof(int) + size; // size of block we need to allocate + its (h & f)
		LIST_FOREACH(ptr_belo2, &freeBlocksList)
		{
			// cache block info (size & allocation flag)
			uint32 size_of_blck = get_block_size((void*)ptr_belo2);

			// case 1: if required size is equal to size of accessed block
			if(req_size == size_of_blck){
					min_belo2 = ptr_belo2;
					min_size_of_blck = size_of_blck;
				break;
			}
			// case 2: if required size is less than size of accessed block
			else if(req_size < size_of_blck)
			{
				if(size_of_blck < min_size_of_blck)
				{
					// make min_belo2 point to where ptr_belo2 points to => to catch min block size
					min_belo2 = ptr_belo2;
					min_size_of_blck = size_of_blck;
				}
			}
		}

		if(min_size_of_blck ==UINT_MAX)
		{
			uint32 dabrk_old = (uint32)sbrk(0);
			uint32* new_brk = sbrk(dabrk_old + size + 2*sizeof(int)/*metadata*/ /*+ sizeof(int)*/);
			return NULL;
		}
		if((min_size_of_blck-req_size) >= 16){
		set_block_data((void*)min_belo2, req_size, 1);
		LIST_REMOVE(&freeBlocksList, min_belo2);
		uint32 req= (uint32)min_belo2+req_size;
		set_block_data((void*)req,min_size_of_blck-req_size, 1);
		free_block((void*)req);
		return (void*)min_belo2;
		}else{
		set_block_data((void*)min_belo2, min_size_of_blck, 1);
		LIST_REMOVE(&freeBlocksList, min_belo2);
		return (void*)min_belo2;
		}
}

//===================================================
// [5] FREE BLOCK WITH COALESCING:
//===================================================
void free_block(void *va)
{
	//TODO: [PROJECT'24.MS1 - #07] [3] DYNAMIC ALLOCATOR - free_block
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("free_block is not implemented yet");
	//Your Code is Here...

		uint32 check_is_free = is_free_block(va);

		if(check_is_free || va == NULL) return ;

		uint32 block_size = get_block_size(va);
		set_block_data(va, block_size, 0);

		uint32 *nxt_block_va = (uint32*)((uint32)va + get_block_size(va));

		int8 is_next_blck_allocated = !is_free_block(nxt_block_va);

		uint32 *prev_footer = (uint32*)va - 2;

		int is_prev_blck_allocated = (*prev_footer%10) & 1; // to get the last bit
	    if(*(nxt_block_va-1)==1){
	    	is_next_blck_allocated=1;
	    }
	    if(*prev_footer==1){
	    	is_prev_blck_allocated=1;
	        }
		// check if the prev block & next block is allocated
		if(is_prev_blck_allocated && is_next_blck_allocated){

			struct BlockElement* new_blck =(struct BlockElement*) (va);
			// must be inserted @ right place in ffl
			set_virtual_addrx(new_blck);
		}
		else if(is_prev_blck_allocated == 1 && is_next_blck_allocated == 0)
		{
			struct BlockElement *next_block =(struct BlockElement*) nxt_block_va;
			uint32 size_of_nxt = get_block_size((uint32 *)nxt_block_va);
			uint32 total_size = block_size + size_of_nxt;

			struct BlockElement* new_block =(struct BlockElement*) va; // merged
			set_block_data(va, total_size, 0);
			LIST_REMOVE(&freeBlocksList, next_block);

			// must be inserted @ right place in ffl
			set_virtual_addrx(new_block);
		}
		else if(is_prev_blck_allocated == 0 && is_next_blck_allocated == 1)
		{
			struct BlockElement* prev_block = (struct BlockElement*)((uint32)va - *prev_footer);
			uint32 size_of_prev = get_block_size((uint32 *)((uint32)va - *prev_footer));

			// set data of merged (prev + cur)
			struct BlockElement* new_block =(struct BlockElement*) (prev_block);
			uint32 total_size = block_size + size_of_prev;
			set_block_data(prev_block, total_size, 0);

			LIST_REMOVE(&freeBlocksList, prev_block);

			// must be inserted @ right place in ffl
			set_virtual_addrx(new_block);
		}
		else if(!(is_prev_blck_allocated && is_next_blck_allocated))
		{
			struct BlockElement *next_block = (struct BlockElement*) nxt_block_va;
			struct BlockElement *prev_block = (struct BlockElement*)((uint32)va - *prev_footer);

			uint32 size_of_prev = get_block_size((uint32 *)((uint32)va - *prev_footer));
			uint32 size_of_nxt = get_block_size(nxt_block_va);

			struct BlockElement* new_block =(struct BlockElement*) prev_block;
			uint32 total_size = block_size + size_of_prev + size_of_nxt;
			set_block_data(prev_block,total_size,0);

			LIST_REMOVE(&freeBlocksList,next_block);
			LIST_REMOVE(&freeBlocksList,prev_block);

			// must be inserted @ right place in ffl
			set_virtual_addrx(new_block);
		}


}

//=========================================
// [6] REALLOCATE BLOCK BY FIRST FIT:
//=========================================
void *realloc_block_FF(void* va, uint32 new_size)
{
	// Q1? if new_size included its metadata?
	// Q2? at special conditions must we return function or call it

	if (new_size % 2 != 0) new_size++; // to make it even

	uint32 size_req = 2*sizeof(int) + new_size; // the total size (included its metadata)

	//TODO: [PROJECT'24.MS1 - #08] [3] DYNAMIC ALLOCATOR - realloc_block_FF
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("realloc_block_FF is not implemented yet");

	//Your Code is Here...
	// Case 1: if size equal to zero
	if(new_size == 0 && va != NULL) {free_block(va); return NULL;}

	// Case 2: if size not equal to zero but va points to null
	if(new_size != 0 && va == NULL) return alloc_block_FF(new_size);

	// Case 3: if size equal to zero but va points to null
	if(new_size == 0 && va == NULL) return alloc_block_FF(new_size);

	// cache block info (size & allocation flag)
	struct BlockElement* cur_one = (struct BlockElement*)(va);
	uint32 size_of_blck = get_block_size(va); // get size of block we need to reallocate with new size
	int8 is_blck_free = is_free_block(va); // to check if block is free or already allocated



	if (size_req == size_of_blck) return va;

	if(va != NULL){
		struct BlockElement *nxt_blck = (struct BlockElement*)((void*)va + size_of_blck);
		uint32 size_nxt_blck = get_block_size((void*)nxt_blck);
		int8 is_nxt_free = is_free_block((void*)nxt_blck);

		// Case 4: if required size is less than the block of va
		if(size_req < size_of_blck){ // @ same block

			uint32 remaining_size = size_of_blck - size_req;

			// must check the next block if isfree???
			// case of remaining is equal or greater than min size of block
			if(remaining_size >= 4*sizeof(int) /*16B*/) {
				//cprintf("Case 4: first if condition\n");
				set_block_data(va, size_req, 1);
				if(is_nxt_free){
					struct BlockElement* new_one = (struct BlockElement*)((void*)va + size_req);
					set_block_data((void*)new_one, remaining_size + size_nxt_blck, 0);
					//cprintf("is_nxt_free\n");
					free_block(va+size_req);
				}
				else{
					//cprintf("else is_nxt_free\n");
					struct BlockElement* new_one = (struct BlockElement*)((void*)va + size_req);
					set_block_data((void*)new_one, remaining_size, 0);
					set_virtual_addrx(new_one);
				}

				//cprintf("hena\n");
			}
			// case of internal fragmentation "a bit large"
			else
			{
				//cprintf("inside else of first if condition\n");

				// reset data of actual block => ptr_blk
				set_block_data((void*)cur_one, size_of_blck, 1);
			}
			return va;
		}
		// Case 5: if required size is greater than the block of va
		else if(size_req > size_of_blck){
			//cprintf("Case 5: 2nd if condition\n");
			struct BlockElement *returned_blck;
			// 1] must check if there is free block after this block
			// 2] if true then take from
			// 3] if false then must search about "suitable" free one @ ffl
			// 4] if found => one must allocate it, free old_block, set new_block
			// 4] if not found then must call sbrk()
			uint32 *end = va + size_of_blck - sizeof(int); // check if it the last block so we get
			                                               // the next block of va and check if it is the end block

			if(*end == 1){ // then it is the last block
				returned_blck = alloc_block_FF(new_size);
				//cprintf("after call allocff\n");

				if(returned_blck != NULL) // allocation is done correctly
				{
					//cprintf(" if (*end == 1)\n");
					uint32 size_returned = get_block_size(returned_blck);
					memcpy(returned_blck, va, size_of_blck - 2*sizeof(int)); // is taken size included its md?
					//set_block_data(va, size_of_blck, 0); // make it free
					//set_virtual_addrx(va); // add it to freeList
					free_block(va);
					return returned_blck;
				}
				else return NULL;
			}

			// must be take from next
			if(is_nxt_free == 1 && nxt_blck != NULL)
			{
				//cprintf("inside case of next is free \n");
				if (size_of_blck + size_nxt_blck >= size_req) {
					//cprintf("inside case of size_of_blck + size_nxt_blck >= size_req \n");
					LIST_REMOVE(&freeBlocksList, nxt_blck);
					uint32 total_size = size_of_blck + size_nxt_blck;
					uint32 remaining_size = total_size - size_req;

					if (remaining_size >= 4 * sizeof(int))
					{
						struct BlockElement* new_free_blck = (struct BlockElement*)((void*)va + size_req);
						set_block_data(va, size_req, 1);
						set_block_data(new_free_blck, remaining_size, 0); // indicate eno free
						set_virtual_addrx(new_free_blck); // add it to freeList
					}
					else
					{
						set_block_data(va, total_size, 1);
					}
					return cur_one;
				}
				else // in case (cur + nxt) not suitable
				{
					//cprintf("inside case of size_of_blck + size_nxt_blck < size_req \n");

					returned_blck = alloc_block_FF(new_size); // allocation is done correctly
					if(returned_blck != NULL){
						memcpy(returned_blck, va, size_of_blck - 2*sizeof(int));
						free_block(va);
						return returned_blck;
					}
					else return NULL;
				}
			}
			else
			{
				// case if must search on fbl about suitable free block "ykfeny"
				returned_blck = alloc_block_FF(new_size);
				if(returned_blck != NULL){
					memcpy(returned_blck, va, size_of_blck - 2*sizeof(int));
					free_block(va);
					return returned_blck;
				}
				else return NULL;
			}
		}
	}
	return NULL;
}

/*********************************************************************************************/
/*********************************************************************************************/
/*********************************************************************************************/

//=========================================
// [7] ALLOCATE BLOCK BY WORST FIT:
//=========================================
void *alloc_block_WF(uint32 size)
{
	panic("alloc_block_WF is not implemented yet");
	return NULL;
}

//=========================================
// [8] ALLOCATE BLOCK BY NEXT FIT:
//=========================================
void *alloc_block_NF(uint32 size)
{
	panic("alloc_block_NF is not implemented yet");
	return NULL;
}

