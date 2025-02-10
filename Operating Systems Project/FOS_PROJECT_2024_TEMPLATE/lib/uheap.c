#include <inc/lib.h>

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//

//=============================================
// [1] CHANGE THE BREAK LIMIT OF THE USER HEAP:
//=============================================
/*2023*/
void* sbrk(int increment)
{
	return (void*) sys_sbrk(increment);
}

//=================================
// [2] ALLOCATE SPACE IN USER HEAP:
//=================================
struct marked_vas
{
	int no_of_pages;
	uint32 va;
};

struct marked_vas marked_pages[(USER_HEAP_MAX - USER_HEAP_START) / PAGE_SIZE];
int arr_size = 0;

void* malloc(uint32 size)
{
	 //cprintf("\n\nInside malloc\n");

	//==============================================================
	//DON'T CHANGE THIS CODE========================================
	if (size == 0 || size > DYN_ALLOC_MAX_SIZE || size > (USER_HEAP_MAX - (myEnv->u_rlmt + PAGE_SIZE)))
	        return NULL;

	// Block Area
	if (size <= DYN_ALLOC_MAX_BLOCK_SIZE)
		return alloc_block_FF(size);
	//==============================================================
	//TODO: [PROJECT'24.MS2 - #12] [3] USER HEAP [USER SIDE] - malloc()
	// Write your code here, remove the panic and write your code
	// panic("malloc() is not implemented yet...!!");
	// Use sys_isUHeapPlacementStrategyFIRSTFIT() and	sys_isUHeapPlacementStrategyBESTFIT()
	// to check the current strategy
	//cprintf("Before Page Allocator\n");
	if (size > DYN_ALLOC_MAX_BLOCK_SIZE)
	{
		uint32 rounded_size =  ROUNDUP(size, PAGE_SIZE);
		int no_of_pages = rounded_size / PAGE_SIZE;

		int orig_pages = no_of_pages;
		if(sys_isUHeapPlacementStrategyFIRSTFIT())
		{
			//cprintf("inside sys_isUHeapPlacementStrategyFIRSTFIT\n");
			uint32 pageVa = 0;
			int isPageNMarked = 1;
			 for (uint32 va = myEnv->u_rlmt + PAGE_SIZE; va < USER_HEAP_MAX; va += PAGE_SIZE)
			 {
				 uint32 perms = sysfor_perms(va);
				 //cprintf("Perms = %x\n", perms);
				 if((perms & M_BIT) == 0x000) // 1st page not marked
				 {
					 uint32 temp = va;
					 int remaining_pages = no_of_pages;
					 isPageNMarked = 1;
					 while (remaining_pages > 0 && temp < USER_HEAP_MAX)
					 {
						 perms = sysfor_perms(temp);
						 if ((perms & M_BIT) != 0x000) // page is marked
						 {
							 isPageNMarked = 0;
							 break;
						 }
						 temp += PAGE_SIZE;
						 remaining_pages--;
					 }

					 if (isPageNMarked && remaining_pages == 0)
					 {
						 pageVa = va;
						 break;
					 }
				 }
			 }
			 // must check on pageVa
			 if(pageVa == 0 || !isPageNMarked) return NULL;

			 //cprintf("before call sys_allocate_user_mem\tva = %x\n", pageVa);
			 sys_allocate_user_mem(pageVa, size);
			 //cprintf("after call sys_allocate_user_mem\tva = %x\n", pageVa);

			 // ufree
			 for (int i = 0; i < ((USER_HEAP_MAX - USER_HEAP_START) / PAGE_SIZE); i++)
			 {
				if (marked_pages[i].no_of_pages == 0 && marked_pages[i].va == 0)
				{
					marked_pages[i].va = pageVa;
					marked_pages[i].no_of_pages = no_of_pages;
					arr_size++;
					break;
				}
			 }

			 //cprintf("before return pageVa\tva = %x\n", pageVa);
			 return (void*)pageVa;
		}
	}
	return NULL;
}


//=================================
// [3] FREE SPACE FROM USER HEAP:
//=================================
void free(void* virtual_address)
{
	//TODO: [PROJECT'24.MS2 - #14] [3] USER HEAP [USER SIDE] - free()
	// Write your code here, remove the panic and write your code
	//panic("free() is not implemented yet...!!");
	/*test case
	// 1]If virtual address inside the [BLOCK ALLOCATOR] range call free_block()
	// 2]If virtual address inside the [PAGE ALLOCATOR] range
		// a) Find the allocated size of the given virtual_address
		// b) Free this allocation from the page allocator of the user heap
		// c) Call “sys_free_user_mem()” to free the allocation from the memory & page file
	//3]panic(…)*/

	//struct Env* e = (struct Env*)virtual_address;
	uint32 upage_start = myEnv->u_rlmt + PAGE_SIZE;
	if(virtual_address == NULL) return;
	if((uint32)virtual_address >= USER_HEAP_MAX || (uint32)virtual_address < USER_HEAP_START) return;
	if((uint32)virtual_address >= upage_start && (uint32)virtual_address < USER_HEAP_MAX)
	{
		uint32 no_of_pages = 0;
		int found = 0;
		// hna bnloop on arr elly feh all va & its pages
		for(int c = 0; c < arr_size; c++)
		{
			if((uint32)virtual_address == marked_pages[c].va)
			{
				//cprintf("index = %d\n", c);
				found = 1;
				//cprintf("va = %d\n", marked_pages[c].va);
				//cprintf("va = %d\tpages = %d\n", virtual_address, marked_pages[c].no_of_pages);
				no_of_pages = marked_pages[c].no_of_pages;
				marked_pages[c].va = 0;
				marked_pages[c].no_of_pages = 0;
				break;
			}
		}
		if(found)
		{
			uint32 size = no_of_pages * PAGE_SIZE;
			sys_free_user_mem((uint32)virtual_address, size);
		}
	}
	else if((uint32)virtual_address >= myEnv->u_start && (uint32)virtual_address < myEnv->u_rlmt)
	{
		free_block(virtual_address);
	}
}

//=================================
// [4] ALLOCATE SHARED VARIABLE:
//=================================
void* smalloc(char *sharedVarName, uint32 size, uint8 isWritable)
{
	// ==============================================================
	// DON'T CHANGE THIS CODE ========================================
	if (size == 0 || size > DYN_ALLOC_MAX_SIZE || size > (USER_HEAP_MAX - (myEnv->u_rlmt + PAGE_SIZE)))
		return NULL;

	// ==============================================================
	// TODO: [PROJECT'24.MS2 - #18] [4] SHARED MEMORY [USER SIDE] - smalloc()
	// Write your code here, remove the panic and write your code
	// panic("smalloc() is not implemented yet...!!");

	size = ROUNDUP(size, PAGE_SIZE);
	uint32 no_of_pages = ROUNDUP(size, PAGE_SIZE)/ PAGE_SIZE;

	uint32 pageVa = 0;
	int isPageNMarked = 1;
	for (uint32 va = myEnv->u_rlmt + PAGE_SIZE; va < USER_HEAP_MAX; va += PAGE_SIZE)
	{
		uint32 perms = sysfor_perms(va);
		 if(((perms & M_BIT) == 0x000))  // 1st page not marked
		{
			uint32 temp = va;
			int remaining_pages = no_of_pages;
			isPageNMarked = 1;
			while (remaining_pages > 0 && temp < USER_HEAP_MAX)
			{
				perms = sysfor_perms(temp);
				 if ((perms & M_BIT) != 0x000) // page is marked
				{
				   isPageNMarked = 0;
				   //va = temp;
				   break;
				}
				temp += PAGE_SIZE;
				remaining_pages--;
			}
			if (isPageNMarked && remaining_pages == 0)
			{
				pageVa = va;
				break;
			}
		}
	}
	// must check on pageVa
	if(pageVa == 0 || !isPageNMarked)
		return NULL;

    if(pageVa != 0 || isPageNMarked)
    {
        // for testing
		//cprintf("\n\npageVa = %u\n\n", pageVa);
        uint32 ret = sys_createSharedObject(sharedVarName, size, isWritable, (void*)pageVa);
        //cprintf("return %d\n",ret);
        if(ret == E_NO_SHARE || ret == E_SHARED_MEM_EXISTS){
            return NULL;
        }
        return (void*)pageVa;
    }
    else
    {
        return NULL;
    }
}

//========================================
// [5] SHARE ON ALLOCATED SHARED VARIABLE:
//========================================
void* sget(int32 ownerEnvID, char *sharedVarName)
{
	//cprintf("hello");
	//TODO: [PROJECT'24.MS2 - #20] [4] SHARED MEMORY [USER SIDE] - sget()
	// Write your code here, remove the panic and write your code
//	panic("sget() is not implemented yet...!!");
	int sizeofshard=sys_getSizeOfSharedObject(ownerEnvID,sharedVarName);
//    if(sizeofshard==E_SHARED_MEM_NOT_EXISTS||sizeofshard==0){
//		return NULL;
//	}
	if (sizeofshard == 0 || sizeofshard==E_SHARED_MEM_NOT_EXISTS || sizeofshard > DYN_ALLOC_MAX_SIZE || sizeofshard > (USER_HEAP_MAX - (myEnv->u_rlmt + PAGE_SIZE)))
	                return NULL;

	     //cprintf("hello1");
		uint32 no_of_pages = ROUNDUP(sizeofshard, PAGE_SIZE)/ PAGE_SIZE;
		if(sys_isUHeapPlacementStrategyFIRSTFIT())
		{  //cprintf("hello2");
		    uint32 pageVa = 0;
			int isPageNMarked = 1;
			for (uint32 va = myEnv->u_rlmt + PAGE_SIZE; va < USER_HEAP_MAX; va += PAGE_SIZE)
			{  //cprintf("hello3");
				uint32 perms = sysfor_perms(va);
				 if(((perms & M_BIT) == 0x000))  // 1st page not marked
				{  //cprintf("hello4");
				    uint32 temp = va;
					int remaining_pages = no_of_pages;
					isPageNMarked = 1;
					//cprintf("hello5");
				while (remaining_pages > 0 && temp < USER_HEAP_MAX)
				{//cprintf("hello7");
					perms = sysfor_perms(temp);
					 if ((perms & M_BIT) != 0x000) // page is marked
				     {//cprintf("hello8");
					   isPageNMarked = 0;
					   break;
					}
					 //cprintf("hello9");
					   temp += PAGE_SIZE;
					   remaining_pages--;
					}

					if (isPageNMarked && remaining_pages == 0)
					{//cprintf("hello10");
					    pageVa = va;
						break;
					}
				}
			}
			// must check on pageVa
			if(pageVa == 0 || !isPageNMarked) {
				//cprintf("hello11");
			return NULL;
		}
			int var= sys_getSharedObject(ownerEnvID,sharedVarName,(void*)pageVa);
			if(var==E_SHARED_MEM_NOT_EXISTS)
			{//cprintf("hello12");
				return NULL;
			}
			//cprintf("hello13");
			return (void*)pageVa;
		}
		//cprintf("hello14");
	return NULL;

}


//==================================================================================//
//============================== BONUS FUNCTIONS ===================================//
//==================================================================================//

//=================================
// FREE SHARED VARIABLE:
//=================================
//	This function frees the shared variable at the given virtual_address
//	To do this, we need to switch to the kernel, free the pages AND "EMPTY" PAGE TABLES
//	from main memory then switch back to the user again.
//
//	use sys_freeSharedObject(...); which switches to the kernel mode,
//	calls freeSharedObject(...) in "shared_memory_manager.c", then switch back to the user mode here
//	the freeSharedObject() function is empty, make sure to implement it.

void sfree(void* virtual_address)
{
	//TODO: [PROJECT'24.MS2 - BONUS#4] [4] SHARED MEMORY [USER SIDE] - sfree()
	// Write your code here, remove the panic and write your code
	panic("sfree() is not implemented yet...!!");
}

//=================================
// REALLOC USER SPACE:
//=================================
//	Attempts to resize the allocated space at "virtual_address" to "new_size" bytes,
//	possibly moving it in the heap.
//	If successful, returns the new virtual_address, in which case the old virtual_address must no longer be accessed.
//	On failure, returns a null pointer, and the old virtual_address remains valid.

//	A call with virtual_address = null is equivalent to malloc().
//	A call with new_size = zero is equivalent to free().

//  Hint: you may need to use the sys_move_user_mem(...)
//		which switches to the kernel mode, calls move_user_mem(...)
//		in "kern/mem/chunk_operations.c", then switch back to the user mode here
//	the move_user_mem() function is empty, make sure to implement it.
void *realloc(void *virtual_address, uint32 new_size)
{
	//[PROJECT]
	// Write your code here, remove the panic and write your code
	panic("realloc() is not implemented yet...!!");
	return NULL;

}

//==================================================================================//
//========================== MODIFICATION FUNCTIONS ================================//
//==================================================================================//

void expand(uint32 newSize)
{
	panic("Not Implemented");
}

void shrink(uint32 newSize)
{
	panic("Not Implemented");
}

void freeHeap(void* virtual_address)
{
	panic("Not Implemented");
}
