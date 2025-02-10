#include <inc/memlayout.h>
#include "shared_memory_manager.h"

#include <inc/mmu.h>
#include <inc/error.h>
#include <inc/string.h>
#include <inc/assert.h>
#include <inc/queue.h>
#include <inc/environment_definitions.h>

#include <kern/proc/user_environment.h>
#include <kern/trap/syscall.h>
#include "kheap.h"
#include "memory_manager.h"

//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//
struct Share* get_share(int32 ownerID, char* name);

//===========================
// [1] INITIALIZE SHARES:
//===========================
//Initialize the list and the corresponding lock
void sharing_init()
{
#if USE_KHEAP
	LIST_INIT(&AllShares.shares_list) ;
	init_spinlock(&AllShares.shareslock, "shares lock");
#else
	panic("not handled when KERN HEAP is disabled");
#endif
}

//==============================
// [2] Get Size of Share Object:
//==============================
int getSizeOfSharedObject(int32 ownerID, char* shareName)
{
	//[PROJECT'24.MS2] DONE
	// This function should return the size of the given shared object
	// RETURN:
	//	a) If found, return size of shared object
	//	b) Else, return E_SHARED_MEM_NOT_EXISTS
	//
	struct Share* ptr_share = get_share(ownerID, shareName);
	if (ptr_share == NULL)
		return E_SHARED_MEM_NOT_EXISTS;
	else
		return ptr_share->size;

	return 0;
}

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//
// [1] Create frames_storage:
//===========================
// Create the frames_storage and initialize it by 0
inline struct FrameInfo** create_frames_storage(int numOfFrames)
{
	//TODO: [PROJECT'24.MS2 - #16] [4] SHARED MEMORY - create_frames_storage()
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("create_frames_storage is not implemented yet");
	//Your Code is Here...
	if(numOfFrames <= 0) return NULL;
    struct FrameInfo **framesStorage;
    uint32 sizeOfArray = numOfFrames * sizeof(struct FrameInfo*); // ???
    framesStorage = (struct FrameInfo **)kmalloc(sizeOfArray); //must allocate framesStorage @ kernel heap or user heap??

    // if found mem for this storage
    if (framesStorage != NULL)
    {
		for (int i = 0; i < numOfFrames; i++)
			framesStorage[i] = 0;

	    return framesStorage;
    }
    return NULL;
}

//=====================================
// [2] Alloc & Initialize Share Object:
//=====================================
//Allocates a new shared object and initialize its member
//It dynamically creates the "framesStorage"
//Return: allocatedObject (pointer to struct Share) passed by reference

struct Share* create_share(int32 ownerID, char* shareName, uint32 size, uint8 isWritable)
{
	//TODO: [PROJECT'24.MS2 - #16] [4] SHARED MEMORY - create_share()
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("create_share is not implemented yet");
	//Your Code is Here...
	struct Share *sharedObject = (struct Share*)kmalloc(sizeof(struct Share));
	if(sharedObject != NULL)
	{
		strncpy(sharedObject->name, shareName, sizeof(sharedObject->name) - 1);
		sharedObject->name[sizeof(sharedObject->name) - 1] = '\0';
		sharedObject->ID = (int32)((uint32)sharedObject & 0x7FFFFFFF); // 0x7FFFFFFF = 0111 1111 1111 1111 1111 1111 1111 1111
		//sharedObject->name = shareName;

		sharedObject->ownerID = ownerID;
		sharedObject->isWritable = isWritable;
		sharedObject->size = size;
		sharedObject->references = 1;

		int numOfFrames = ROUNDUP(size, PAGE_SIZE)/ PAGE_SIZE;
		struct FrameInfo** framesStorage = create_frames_storage(numOfFrames);
		if(framesStorage != NULL){
			// must put this obj in array ???
			sharedObject->framesStorage = framesStorage;
			return sharedObject;
		}
		else
		{
			// must undo any allocation & return null
			// how to undo any allocations was made ???
			kfree((void*)sharedObject);
			return NULL;
		}
	}
	return NULL;
}

//=============================
// [3] Search for Share Object:
//=============================
//Search for the given shared object in the "shares_list"
//Return:
//	a) if found: ptr to Share object
//	b) else: NULL

struct Share* get_share(int32 ownerID, char* name)
{//cprintf("sh1");
  //[PROJECT'24.MS2]
  //COMMENT THE FOLLOWING LINE BEFORE START CODING
  //panic("get_share is not implemented yet");
  //Your Code is Here...
   acquire_spinlock(&AllShares.shareslock);
   //cprintf("sh2");
    struct Share* currentShare;
    //cprintf("sh3");
    LIST_FOREACH(currentShare, &AllShares.shares_list)
    {
    	//cprintf("sh4");
      if (currentShare->ownerID == ownerID && strcmp(currentShare->name, name) == 0)
      {
    	  //cprintf("sh5");
        release_spinlock(&AllShares.shareslock);
        //cprintf("sh6");
        return currentShare;
      }
    }
   // cprintf("sh7");
    release_spinlock(&AllShares.shareslock);
    //cprintf("sh8");
    return NULL;
}

//=========================
// [4] Create Share Object:
//=========================
int create_SharedObject(int32 ownerID, char* shareName, uint32 size, uint8 isWritable, void* virtual_address)
{
	//TODO: [PROJECT'24.MS2 - #19] [4] SHARED MEMORY [KERNEL SIDE] - createSharedObject()
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("createSharedObject is not implemented yet");
	//Your Code is Here...
int numOfFrames;
size=ROUNDUP(size,PAGE_SIZE);
uint32 numPages = size / PAGE_SIZE;
struct Env* myenv = get_cpu_proc(); //The calling environment
//create_frames_storage(numOfFrames);
// Check if a shared object with the same name already exists
	    struct Share* existingShare = get_share(ownerID, shareName);
	    if (existingShare != NULL) {
        return E_SHARED_MEM_EXISTS; // Shared object already exists
	    }
struct Share* new_share_object=create_share(ownerID,shareName,size,isWritable);//allocate, initialize shared object
//initialize list by empty and lock of list
// Add the new share to the shares list
if(new_share_object==NULL)
{
	return E_NO_SHARE;
}
else{
acquire_spinlock(&AllShares.shareslock);
LIST_INSERT_HEAD(&AllShares.shares_list,new_share_object);
release_spinlock(&AllShares.shareslock);
}
	getSizeOfSharedObject(ownerID,shareName);//return size of shared object
	//allocate and map in physical memory


	for (uint32 i = 0; i < numPages; i++)
	{

		struct FrameInfo*  frame;
		int ret=allocate_frame(&frame);
			if(ret!=E_NO_MEM)//correctly allocate
			{
				int ret11 = map_frame(myenv->env_page_directory, frame,
				 ((uint32)virtual_address +( i * PAGE_SIZE)),
			 PERM_WRITEABLE);
				// Store the frame in the share's frame storage
					 int numOfFrames=(int)(ROUNDUP(size,PAGE_SIZE)/PAGE_SIZE);
					ret= (int)new_share_object->framesStorage;
					 //return (uint32)virtual_address;
			}
			else//failed to allocate and map
			{
				 kfree((void*)new_share_object);
				return E_NO_SHARE;
			}

	} return (int)new_share_object->ID;

}

int createSharedObject(int32 ownerID, char* shareName, uint32 size, uint8 isWritable, void* virtual_address)
{
	//TODO: [PROJECT'24.MS2 - #19] [4] SHARED MEMORY [KERNEL SIDE] - createSharedObject()
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("createSharedObject is not implemented yet");
	//Your Code is Here...

	struct Env* myenv = get_cpu_proc(); // The calling environment
	struct Share *IsFoundObject = get_share(ownerID, shareName);
	if(IsFoundObject != NULL) return E_SHARED_MEM_EXISTS;

	struct Share *sharedObject = create_share(ownerID, shareName, size, isWritable); // isWritable ???
	// if creation is done successfully
	if(sharedObject != NULL)
	{
		acquire_spinlock(&AllShares.shareslock);
		LIST_INSERT_TAIL(&AllShares.shares_list, sharedObject); // insert created obj in shares_list
		release_spinlock(&AllShares.shareslock);

		// isWritable --> check eno 1 if true ????

		int numOfPages = ROUNDUP(size, PAGE_SIZE)/ PAGE_SIZE;
		int i = 0;
		bool mappedIsCompleted = 0;
		while(numOfPages > 0)
		{
			struct FrameInfo *ptr = NULL;
			allocate_frame(&ptr);
			if(ptr != NULL)
			{
				map_frame(myenv->env_page_directory, ptr, (uint32)virtual_address + (PAGE_SIZE * i), (PERM_WRITEABLE | PERM_PRESENT | PERM_USER | M_BIT));
				ptr->my_own_va = (uint32)(virtual_address + (PAGE_SIZE * i));
				sharedObject->framesStorage[i] = ptr;
				mappedIsCompleted = 1;
				i++;
			}
			else
			{
				mappedIsCompleted = 0;
				break;
			}
			numOfPages--;
		}

		if(!mappedIsCompleted){
			return E_NO_SHARE;
		}
		return sharedObject->ID;
	}
	return E_NO_SHARE;
}

//======================
// [5] Get Share Object:
//======================

int getSharedObject(int32 ownerID, char* shareName, void* virtual_address)
{
//	//TODO: [PROJECT'24.MS2 - #21] [4] SHARED MEMORY [KERNEL SIDE] - getSharedObject()
//	//COMMENT THE FOLLOWING LINE BEFORE START CODING
////	panic("getSharedObject is not implemented yet");
//	//Your Code is Here...
//
		struct Env* myenv = get_cpu_proc(); //The calling environment
		struct Share*object=get_share(ownerID,shareName);
	if (object == NULL) {
	    return E_SHARED_MEM_NOT_EXISTS;
	}

	int pages = ROUNDUP(object->size,PAGE_SIZE);
	int no_of_pages=pages/PAGE_SIZE;
	for(int i=0;i<no_of_pages;i++)
	{
		struct FrameInfo * ff=object->framesStorage[i];
		map_frame(myenv->env_page_directory,ff,(uint32)virtual_address+(i*PAGE_SIZE),PERM_WRITEABLE|PERM_USER|M_BIT);


		if(object->isWritable==0)
		{
			pt_set_page_permissions(myenv->env_page_directory,(uint32)virtual_address+(i*PAGE_SIZE),0,PERM_WRITEABLE);
		}
		else
		{
			pt_set_page_permissions(myenv->env_page_directory,(uint32)virtual_address+(i*PAGE_SIZE),PERM_WRITEABLE,0);
		}

	}
	object->references++;
	return object->ID;
}

//==================================================================================//
//============================== BONUS FUNCTIONS ===================================//
//==================================================================================//

//==========================
// [B1] Delete Share Object:
//==========================
//delete the given shared object from the "shares_list"
//it should free its framesStorage and the share object itself
void free_share(struct Share* ptrShare)
{
	//TODO: [PROJECT'24.MS2 - BONUS#4] [4] SHARED MEMORY [KERNEL SIDE] - free_share()
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	panic("free_share is not implemented yet");
	//Your Code is Here...

}

//========================
// [B2] Free Share Object:
//========================
int freeSharedObject(int32 sharedObjectID, void *startVA)
{
	//TODO: [PROJECT'24.MS2 - BONUS#4] [4] SHARED MEMORY [KERNEL SIDE] - freeSharedObject()
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	panic("freeSharedObject is not implemented yet");
	//Your Code is Here...

}
