// User-level Semaphore

#include "inc/lib.h"

struct semaphore create_semaphore(char *semaphoreName, uint32 value)
{
	//TODO: [PROJECT'24.MS3 - #02] [2] USER-LEVEL SEMAPHORE - create_semaphore
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("create_semaphore is not implemented yet");
	//Your Code is Here...
	 // create a semaphore wrapper object
     // dyn allocate mem for actual semaphore data
	struct semaphore sem_mir;
	void* semaphoreMemory = smalloc(semaphoreName, sizeof(struct __semdata), 1);
	if (!semaphoreMemory) {
		return sem_mir;
	}
	sem_mir.semdata = (struct __semdata*)semaphoreMemory;
	    sem_mir.semdata->count = value;               //count value
	    sem_mir.semdata->lock = 0;                    //unlocked
	    sys_init_queue(&sem_mir.semdata->queue);   //env queue for blocked
	    strncpy(sem_mir.semdata->name, semaphoreName, sizeof(sem_mir.semdata->name) - 1);
	    sem_mir.semdata->name[sizeof(sem_mir.semdata->name) - 1] = '\0'; //copy semaphore name

	    return sem_mir;

}

struct semaphore get_semaphore(int32 ownerEnvID, char* semaphoreName)
{
	//TODO: [PROJECT'24.MS3 - #03] [2] USER-LEVEL SEMAPHORE - get_semaphore
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("get_semaphore is not implemented yet");
	//Your Code is Here...
	    struct semaphore sem;
	int32 parentEnvID = sys_getparentenvid();
	    if (parentEnvID != ownerEnvID) {
		    return sem;
	    }
	 void* semaphoreMemory = sget(ownerEnvID, semaphoreName);
	 if (!semaphoreMemory) {
		    return sem;
	 }
	    sem.semdata = (struct __semdata*)semaphoreMemory;

	    return sem;
}



void wait_semaphore(struct semaphore sem)
{
	//TODO: [PROJECT'24.MS3 - #04] [2] USER-LEVEL SEMAPHORE - wait_semaphore
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("wait_semaphore is not implemented yet");
	//Your Code is Here...
	//struct Env* myEnv = get_cpu_proc(); // The calling environment
	// system call to get cpu
	// u should open the lock after block
	// call the sched to get next process after open lock
	// use the intterupt at the start and end of the function close it at the start and open it at the end
//  use it after the sched
    while (xchg(&(sem.semdata->lock),1)!=0);

	sem.semdata->count--;
	if (sem.semdata->count < 0) {

	sys_enq(&(sem.semdata->queue),(struct Env*)myEnv);
	sem.semdata->lock=0;
	sys_sched();

	}
	sem.semdata->lock = 0;


}

void signal_semaphore(struct semaphore sem)
{
	//TODO: [PROJECT'24.MS3 - #05] [2] USER-LEVEL SEMAPHORE - signal_semaphore
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("signal_semaphore is not implemented yet");
	//Your Code is Here...

    while (xchg(&(sem.semdata->lock),1)!=0);

	sem.semdata->count++;
	if (sem.semdata->count <= 0) {
	sys_dequeue(&(sem.semdata->queue));
	sem.semdata->lock=0;

	}
	sem.semdata->lock = 0;

}

int semaphore_count(struct semaphore sem)
{
	return sem.semdata->count;
}
