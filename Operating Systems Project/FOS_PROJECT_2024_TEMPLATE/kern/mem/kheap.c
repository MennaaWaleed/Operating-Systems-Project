#include "kheap.h"

#include <inc/memlayout.h>
#include <inc/dynamic_allocator.h>
#include "memory_manager.h"

struct spinlock lock;

//Initialize the dynamic allocator of kernel heap with the given start address, size & limit
//All pages in the given range should be allocated
//Remember: call the initialize_dynamic_allocator(..) to complete the initialization
//Return:
//	On success: 0
//	Otherwise (if no memory OR initial size exceed the given limit): PANIC
int initialize_kheap_dynamic_allocator(uint32 daStart, uint32 initSizeToAllocate, uint32 daLimit)
{
	init_spinlock(&lock, "Locked");

	//TODO: [PROJECT'24.MS2 - #01] [1] KERNEL HEAP - initialize_kheap_dynamic_allocator
	// Write your code here, remove the panic and write your code
	//panic("initialize_kheap_dynamic_allocator() is not implemented yet...!!");
	// 1] must initialize 3 variables @ .h
	kh_start = ROUNDDOWN(daStart, PAGE_SIZE);
	sgm_brk = ROUNDUP((kh_start + initSizeToAllocate), PAGE_SIZE);
	rlmt = ROUNDUP(daLimit, PAGE_SIZE);

	initialize_paging();
	int numOfPages = ROUNDUP(initSizeToAllocate, PAGE_SIZE) / PAGE_SIZE;

	if(sgm_brk > rlmt) return E_NO_MEM;

	// 2] alloc all pages in "initSizeToAllocate" & map to pa
	for(int i = 0; i < numOfPages; i++)
	{
	   struct FrameInfo *ptr = NULL;
	   allocate_frame(&ptr);
	   if(ptr != NULL)
	   {
		   map_frame(ptr_page_directory, ptr, kh_start + (i * PAGE_SIZE), (PERM_WRITEABLE | PERM_PRESENT));
		   ptr->my_own_va = kh_start + (i * PAGE_SIZE);
	   }
	   else
		   return E_NO_MEM;
	}
	initialize_dynamic_allocator(kh_start, initSizeToAllocate);
	return 0;
}

void* sbrk(int numOfPages)
{
	/* numOfPages > 0: move the segment break of the kernel to increase the size of its heap by the given numOfPages,
	 * 				you should allocate pages and map them into the kernel virtual address space,
	 * 				and returns the address of the previous break (i.e. the beginning of newly mapped memory).
	 * numOfPages = 0: just return the current position of the segment break
	 *
	 * NOTES:
	 * 	1) Allocating additional pages for a kernel dynamic allocator will fail if the free frames are exhausted
	 * 		or the break exceed the limit of the dynamic allocator. If sbrk fails, return -1
	 */
	// Me
	/* 1] first should conv #pages to its size
	 * 2] case 1: if inc > 0 then
	 * 		- must move sbrk by #pages
	 * 		- then allocate the req pages
	 * 		- at last map them to pa
	 * 		- & ret address of last sbrk => begin of new mapped mem
	 *
	 * 3] case 2: if inc == 0 then
	 * 		- must ret the cur sbrk
	 *
	 * SOME Checks:
	 * 		- if no mem enough to req pages or exceed rlimit => return -1
	 * */
	//MS2: COMMENT THIS LINE BEFORE START CODING ==========
	//====================================================
	uint32 size = numOfPages * PAGE_SIZE;  // to get size "we consider size of metadata" we need to add
	//TODO: [PROJECT'24.MS2 - #02] [1] KERNEL HEAP - sbrk
	if(size == 0 && numOfPages == 0) return (void*) sgm_brk;
	else if (size > 0 && (size + sgm_brk) <= rlmt && numOfPages != 0)
	{
		/*for(int iter = sgm_brk; iter < sgm_brk + size; iter += PAGE_SIZE)
		{
			struct FrameInfo *ptr = NULL;
			allocate_frame(&ptr);
			if(ptr != NULL)
			{
				map_frame(ptr_page_directory, &ptr, iter, (PERM_WRITEABLE | PERM_PRESENT));
			}
			else
				return (void*)-1;
		}*/
		for(int iter = 0; iter < numOfPages; iter++)
		{
			struct FrameInfo *ptr = NULL;
			allocate_frame(&ptr);
			if(ptr != NULL)
			{
				map_frame(ptr_page_directory, ptr, sgm_brk + (iter * PAGE_SIZE), (PERM_WRITEABLE | PERM_PRESENT));
				ptr->my_own_va = sgm_brk + (iter * PAGE_SIZE);
			}
			else
				return (void*) -1;
		}
		uint32 last_s = sgm_brk;
		// update sbrk
		sgm_brk += size;
		return (void*)last_s;
	}
	return (void*) -1;
}

//TODO: [PROJECT'24.MS2 - BONUS#2] [1] KERNEL HEAP - Fast Page Allocator
struct mapped_vas
{
	int no_of_pages;
	uint32 va;
};

void remove_pages(uint32 va_start, uint32 va_end)
{
    for (uint32 va = va_start; va < va_end; va += PAGE_SIZE)
    	unmap_frame(ptr_page_directory, va);
}

struct mapped_vas mapped_pages[(KERNEL_HEAP_MAX - KERNEL_HEAP_START) / PAGE_SIZE];
int arr_size = 0;
uint32 i;
void *kmalloc(unsigned int size)
{
	acquire_spinlock(&lock);
    if (size == 0 || size > DYN_ALLOC_MAX_SIZE || size > (KERNEL_HEAP_MAX - (rlmt + PAGE_SIZE)))
    {
    	release_spinlock(&lock);
        return NULL;
    }

    // Block Area
    if (size <= DYN_ALLOC_MAX_BLOCK_SIZE)
    {
    	void *ret = alloc_block_FF(size);
    	release_spinlock(&lock);
        return ret;
    }

    // Page Area
    if (size > DYN_ALLOC_MAX_BLOCK_SIZE)
    {
    	uint32 rounded_size =  ROUNDUP(size, PAGE_SIZE);
        int no_of_pages = rounded_size / PAGE_SIZE;
        int orig_pages = no_of_pages;

        if (orig_pages == 0) {
        	release_spinlock(&lock);
        	return NULL;
        }

        if (isKHeapPlacementStrategyFIRSTFIT())
        {
            int isApplicable = 0;
            uint32 availVa = 0;
            for (uint32 va = rlmt + PAGE_SIZE; va < KERNEL_HEAP_MAX; va += PAGE_SIZE)
            {
                uint32 *ptr_page_table = NULL;
                struct FrameInfo *framePtr = get_frame_info(ptr_page_directory, va, &ptr_page_table);

                if (framePtr == NULL)
                {
                    uint32 temp = va;
                    int remaining_pages = no_of_pages;
                    isApplicable = 1;

                    while (remaining_pages > 0 && temp < KERNEL_HEAP_MAX)
                    {
                        framePtr = get_frame_info(ptr_page_directory, temp, &ptr_page_table);
                        if (framePtr != NULL)
                        {
                            isApplicable = 0;
                            break;
                        }
                        temp += PAGE_SIZE;
                        remaining_pages--;
                    }

                    if (isApplicable && remaining_pages == 0)
                    {
                        availVa = va;
                        break;
                    }
                }
            }
			if (availVa == 0 || !isApplicable)
			{
		    	release_spinlock(&lock);
				return NULL;
			}

			uint32 currVa = availVa;
			int allocation_failed = 0;

			while (orig_pages-- > 0)
			{
				struct FrameInfo *frame_ptr = NULL;

				int ret = allocate_frame(&frame_ptr);
				if (ret == E_NO_MEM)
				{
					allocation_failed = 1;
					break;
				}

				ret = map_frame(ptr_page_directory, frame_ptr, currVa, (PERM_WRITEABLE | PERM_PRESENT));
				if (ret == E_NO_MEM)
				{
					allocation_failed = 1;
					break;
				}

				frame_ptr->my_own_va = currVa;
				currVa += PAGE_SIZE;
			}

			if (allocation_failed)
			{
				remove_pages(availVa, currVa);
		    	release_spinlock(&lock);
				return NULL;
			}

			// kfree
			for (i = 0; i < ((KERNEL_HEAP_MAX - KERNEL_HEAP_START) / PAGE_SIZE); i++)
			{
				if (mapped_pages[i].no_of_pages == 0 && mapped_pages[i].va == 0)
				{
					mapped_pages[i].va = availVa;
					mapped_pages[i].no_of_pages = no_of_pages;
					arr_size++;
					break;
				}
			}

	    	release_spinlock(&lock);
			return (void*)availVa;
		}
    }
	release_spinlock(&lock);
    return NULL;
}

void kfree(void* virtual_address)
{
	acquire_spinlock(&lock);
	//TODO: [PROJECT'24.MS2 - #04] [1] KERNEL HEAP - kfree
	// Write your code here, remove the panic and write your code
	//panic("kfree() is not implemented yet...!!");

	//you need to get the size of the given allocation using its address
	//refer to the project presentation and documentation for details

	/*test case
	//1] If virtual address inside the [BLOCK ALLOCATOR] range
	//Use dynamic allocator to free the given address MS#1
	//2]If virtual address inside the [PAGE ALLOCATOR] range
	//FREE the space of the given address from RAM & number of pages & free all pages
	//3]Else (i.e. invalid address): should panic(…)
    // if va null return*/

	uint32 p_start = rlmt + PAGE_SIZE;
	if(virtual_address == NULL)
	{
    	release_spinlock(&lock);
		return;
	}
	if((uint32)virtual_address >= KERNEL_HEAP_MAX || (uint32)virtual_address < KERNEL_HEAP_START)
	{
		release_spinlock(&lock);
		return;
	}
	if((uint32)virtual_address >= p_start && (uint32)virtual_address < KERNEL_HEAP_MAX)
	{
		uint32 no_of_pages = 0;
		int found = 0;
		// hna bnloop on arr elly feh all va & its pages
		for(int c = 0; c < arr_size; c++)
		{
			if((uint32)virtual_address == mapped_pages[c].va)
			{
				found = 1;
				no_of_pages = mapped_pages[c].no_of_pages;
				mapped_pages[c].va = 0;
				mapped_pages[c].no_of_pages = 0;
				break;
			}

		}
		if(found)
		{
			for(int i = 0; i < no_of_pages; i++)
			{
				uint32 *ptr_page_table = NULL;
				struct FrameInfo* ptr_frame_info = get_frame_info(ptr_page_directory, (uint32)virtual_address, &ptr_page_table);
				ptr_frame_info->my_own_va = 0;
				unmap_frame(ptr_page_directory,(uint32)virtual_address);
				virtual_address += PAGE_SIZE;
			}
		}
	}
	else if((uint32)virtual_address >= kh_start && (uint32)virtual_address < rlmt)
	{
		free_block(virtual_address);
    	release_spinlock(&lock);
    	return;
	}
	release_spinlock(&lock);
}

unsigned int kheap_physical_address(unsigned int virtual_address)
{
	//TODO: [PROJECT'24.MS2 - #05] [1] KERNEL HEAP - kheap_physical_address
	// Write your code here, remove the panic and write your code
	//panic("kheap_physical_address() is not implemented yet...!!");

	//return the physical address corresponding to given virtual_address
	//refer to the project presentation and documentation for details

	//EFFICIENT IMPLEMENTATION ~O(1) IS REQUIRED ==================
	//if(virtual_address<KERNEL_HEAP_MAX && virtual_address>=KERNEL_HEAP_START){
	uint32 *ptr_page_table = NULL;
	uint32 t_ent = PTX((uint32)virtual_address);

	get_page_table(ptr_page_directory,virtual_address, &ptr_page_table);
	if(ptr_page_table != NULL)
	{
		struct FrameInfo* ptr_frame_info = get_frame_info(ptr_page_directory, (uint32)virtual_address, &ptr_page_table);
		if(ptr_frame_info!=NULL){
			uint32 ent = ptr_page_table[t_ent];
			uint32 pa = (ent & (0xFFFFF000))+(virtual_address &(0x00000FFF));
			return pa;
		}
	}
	return 0;
}

unsigned int kheap_virtual_address(unsigned int physical_address)
{
	//TODO: [PROJECT'24.MS2 - #06] [1] KERNEL HEAP - kheap_virtual_address
	// Write your code here, remove the panic and write your code
	//panic("kheap_virtual_address() is not implemented yet...!!");

	//return the virtual address corresponding to given physical_address
	//refer to the project presentation and documentation for details

	//EFFICIENT IMPLEMENTATION ~O(1) IS REQUIRED ==================

	struct FrameInfo* ptr = NULL;
	ptr = to_frame_info(physical_address);
	if(ptr != NULL)
	{
	   if(ptr->references == 1)
	   {
		   if(ptr->my_own_va!=0){
			uint32 va = (ptr->my_own_va & 0xFFFFF000) + (physical_address & 0x00000FFF);
			return va;
		   }
	   }
	   return 0;
	}
	return 0;
}

//=================================================================================//
//============================== BONUS FUNCTION ===================================//
//=================================================================================//
// krealloc():
/*	Attempts to resize the allocated space at "virtual_address" to "new_size" bytes,
	possibly moving it in the heap.
	If successful, returns the new virtual_address, if moved to another loc: the old virtual_address must no longer be accessed.
	On failure, returns a null pointer, and the old virtual_address remains valid.

	A call with virtual_address = null is equivalent to kmalloc().
	A call with new_size = zero is equivalent to kfree().*/

void *krealloc(void *virtual_address, uint32 new_size)
{
	//TODO: [PROJECT'24.MS2 - BONUS#1] [1] KERNEL HEAP - krealloc
	// Write your code here, remove the panic and write your code
	//return NULL;
	//panic("krealloc() is not implemented yet...!!");
	if(new_size == 0 && virtual_address != NULL) {kfree(virtual_address); return NULL;}

	if(new_size != 0 && virtual_address == NULL) return kmalloc(new_size);

	if(new_size == 0 && virtual_address == NULL) return NULL;

	/* check on the "va" if found inside "block" allocator
	 * if true:
	 * 			if new_size > 2K => will move to page allocator 'kmalloc(new_size)'
	 * 			- free with => kfree(va)
	 *
	 * 			if new_size < 2K => call reallocate "deal with block area"
	 *
	 * if false "if va found inside "page" allocator":
	 * 			if new_size > 2K => kreallocate for this va
	 * 			- kmalloc(new_size)
	 * 			- kfree(va)
	 *
	 * 			if new_size < 2K => kmalloc(new_size)
	 * 			- free with => kfree(va)
	 *
	 * copy el data???
	 * */

	if(virtual_address != NULL)
	{
		// va on block area
		if((uint32)virtual_address >= KERNEL_HEAP_START && (uint32)virtual_address < rlmt)
		{
			if(new_size > DYN_ALLOC_MAX_SIZE) // new_size > 2K
			{
				void* new_allocation = kmalloc(new_size);
				if (new_allocation != NULL)
				{
					uint32 size = get_block_size(virtual_address);
					memcpy(new_allocation, virtual_address, size - 2*sizeof(int));

					kfree(virtual_address);
					return new_allocation;
				}
				else return NULL; // ???????
			}
			else // new_size < 2K => inside 'block area'
			{
				return realloc_block_FF(virtual_address, new_size);
			}
		}
		// va on page area
		else if((uint32)virtual_address >= (rlmt + PAGE_SIZE) && (uint32)virtual_address < KERNEL_HEAP_MAX)
		{
			if(new_size > DYN_ALLOC_MAX_SIZE) // new_size > 2K
			{
				void* new_allocation = kmalloc(new_size);
				if (new_allocation != NULL)
				{
					uint32 no_of_pages = 0;

					for(int c = 0; c < arr_size; c++)
					{
						if((uint32)virtual_address == mapped_pages[c].va)
						{
							no_of_pages = mapped_pages[c].no_of_pages;
							break;
						}
					}
					if(no_of_pages > ROUNDUP(new_size, PAGE_SIZE)/PAGE_SIZE)
					{
						memcpy(new_allocation, virtual_address, new_size);
					}
					else{
						memcpy(new_allocation, virtual_address, no_of_pages*PAGE_SIZE);
					}

					kfree(virtual_address);
					return new_allocation;
				}
				else return NULL; // ???????
			}
			else // new_size < 2K => inside 'block area'
			{
				void* new_allocation = kmalloc(new_size);
				if (new_allocation != NULL)
				{
					memcpy(new_allocation, virtual_address, new_size);
					kfree(virtual_address);
					return new_allocation;
				}
				else return NULL; // ???????
			}
		}
	}
	return NULL;
}
