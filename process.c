#include "functs.h"

int semKey = 8675309, semId;
// int semKey2 = 8008135, semId2;
int semKeyCond = 90210, semIdCond;//For processes requesting resources
int key = 4444444;
int key2 = 5555555;
int key3 = 6666666;
int shmidVal;
int shmidVal2;
int shmidVal3;

int main(int argc, char * argv[]){

char *p;
int processId = strtol(argv[0],&p,10);
shmidVal = strtol(argv[1],&p,10);
shmidVal2 = strtol(argv[2],&p,10);
shmidVal3 = strtol(argv[3],&p,10);

double currTime, currTime2, arrivalTime, startTime, endTime, turnAround, currRunTime, terminateTime, checkTerminate, deallocateTime, checkDeallocate;
srand(time(NULL));
int processRequestId = -2;//Initialize to something other than process number
int terminate = 0, resourceIndex, amountDeallocate;
int r;//For for loops
bool oneSecPassed = false;//if 1 second has passed, can start checking if can terminate
bool processBlocked = false;//True if process blocked (in a Queue)
bool hasResources = false;//True if process has a resource allocated to it

double genRandomDouble(int processId);//No funct overloading in C, this oss & process funct different

struct PCB *processBlock;
struct timing *timer;
struct resourceTable *resource;

//Attach to shared mem
if((processBlock = (struct PCB *)shmat(shmidVal,(void *)0, 0)) == (void *)-1){ //Returns void*
        fprintf(stderr,"%s Line %d ",__FILE__,__LINE__);
        perror("shmat");
        exit(EXIT_FAILURE);
}
if((timer = (struct timing *)shmat(shmidVal2, 0, 0)) == (void *)-1){ //Returns void*
        fprintf(stderr,"%s Line %d ",__FILE__,__LINE__);
        perror("shmat");
        exit(EXIT_FAILURE);
}
if((resource = (struct resourceTable*)shmat(shmidVal3, 0, 0)) == (void *)-1){ //Returns void*
        fprintf(stderr,"%s Line %d ",__FILE__,__LINE__);
        perror("shmat");
        exit(EXIT_FAILURE);
}

//Get/Create semaphores
if((semId = semget(semKey,1,0)) == -1){//For clock
        fprintf(stderr,"%s Line %d ",__FILE__,__LINE__);
        perror(" semget");
        exit(EXIT_FAILURE);
}
if((semIdCond = semget(semKeyCond,1,0)) == -1){//For resource requests
        fprintf(stderr,"%s Line %d ",__FILE__,__LINE__);
        perror(" semget");
        exit(EXIT_FAILURE);
}

// fprintf(stdout,"********************************Process %d Max Claims***********************************\n", processId);
for(r = 0; r < 20; r++){//Figure out this processes max claim for each resource
        processBlock->maxClaim[processId][r] = rand() % resource->total[r];//Max claim = [0-total number of this resource in system]
        // fprintf(stdout,"Res %d = %d, ", r, processBlock->maxClaim[processId][r]);
}
// fprintf(stdout,"\n\n");

fprintf(stdout,"********************************Process %d Initial Needs***********************************\n", processId);
for(r = 0; r < 20; r++){
        processBlock->need[processId][r] = processBlock->maxClaim[processId][r] - processBlock->allocated[processId][r];//Need = Max - Allocated
        fprintf(stdout,"%d of %d, ", processBlock->need[processId][r], r);
}
fprintf(stdout,"\n\n");

int t = 0;
arrivalTime = getTotalTime(&timer->clockSecs, &timer->clockNanos, &arrivalTime);//Get arrival time
processBlock->turnAround[processId] = arrivalTime;
startTime = getTotalTime(&timer->clockSecs, &timer->clockNanos, &startTime);//Get start time
while(true){
        // fprintf(stdout,"ENTER %d Running time: %.9f\n",processId, currRunTime);

if(oneSecPassed){//If process has run for more than 1 second, start seeing if can terminate
        // fprintf(stdout,"%d: %.9f s > %.9f s ?\n", processId, terminateTime, checkTerminate);
        if(terminateTime > checkTerminate){//Check if process chould terminate at random times between 0 and 250ms
                terminate = rand() % (processId + 10);// + processId so concurrent processes don't get same num since seeded with time, +10 so large enough to % again
                terminate = terminate % 2;//Check if process will terminate
                if(terminate == 1){//Deallocate all resources and end
                        waitSem();
                        /***************************
                        Deallocate/Release resources
                        ***************************/
                        processBlock->processDone = processId;//Remove from bitVector
                        processBlock->running[processId] = 0;
                        for(r = 0; r < 20; r++){
                                processBlock->maxClaim[processId][r] = -1;
                                processBlock->release[processId][r] = processBlock->release[processId][r] + processBlock->allocated[processId][r];
                                resource->available[r] = resource->available[r] + processBlock->allocated[processId][r];
                                processBlock->allocated[processId][r] = -1;
                                if(inQueue(20, 18, resource->blocked, r, processId)){//If process blocked in a queue, remove it
                                        int id = selectFromQueue(resource->blocked[r], &resource->blockedTail[processBlock->requesting[processId]], processId);
                                        // fprintf(stdout,"Process %d Removed from %d Queue\n", id, r);
                                }
                        }
                        endTime = getTotalTime(&timer->clockSecs, &timer->clockNanos, &endTime);//Get time ended at
                        processBlock->turnAround[processId] = endTime - processBlock->turnAround[processId];// turnAround = endTime - arrivalTime;
                        processBlock->cpuTimeUsed[processId] = processBlock->cpuTimeUsed[processId] + processBlock->turnAround[processId] - processBlock->idleTotal[processId];
                        processBlock->totalSystemTime[processId] = endTime - processBlock->totalSystemTime[processId];//endTime - Creation time
                        // fprintf(stdout,"%d: arrivalTime %.9f, endTime %.9f, turnAround: %.9f, cpuTimeUsed: %.9f\n", processId, arrivalTime, endTime, processBlock->turnAround[processId], processBlock->cpuTimeUsed[processId]);
                        signalSem();
                        return 0;
                }

                if(processBlock->running[processId] == 1){
                        waitRequest();
                        /***********************************
                                Request random resource
                        ***********************************/
                        int numResClasses = 0;
                        int resWanted[20];//Array will hold all res classes this process currently wanting
                        for(r = 0; r < 20; r++){
                                resWanted[r] = -1;
                        }
                        for(r = 0; r < 20; r++){//Count number of resource classes & store the classes in array so can choose one
                                if(processBlock->need[processId][r] != 0){
                                        resWanted[numResClasses] = r;
                                        numResClasses++;
                                }
                        }
                        resourceIndex = rand() % numResClasses;//Pick a random resource to allocate
                        processBlock->requesting[processId] = resWanted[resourceIndex];//Pick a random resource to allocate
                        processBlock->processRequesting = processId;
                        processRequestId = processBlock->processRequesting;
                        // fprintf(stdout,"***Process processBlock->requesting[%d] = %d, resourceIndex = %d, resWanted[%d] = %d, numResClasses = %d\n", processId, processBlock->requesting[processId], resourceIndex, resourceIndex, resWanted[resourceIndex], numResClasses);
                        while(processRequestId != -1){//Wait until can request resources
                                processRequestId = processBlock->processRequesting;
                                signalRequest(processRequestId);
                        }
                        if(processBlock->running[processId] == 0){//If process was blocked, mark the time
                                processBlock->idle[processId] = getTotalTime(&timer->clockSecs, &timer->clockNanos, &currRunTime);
                        }
                        printResources(processBlock , resource);
                        advanceClock(&timer->clockSecs, &timer->clockNanos);//advance clock since not terminating
                        currTime = getTotalTime(&timer->clockSecs, &timer->clockNanos, &currTime);
                        checkTerminate = genRandomDouble(processId);//Generate new time 0-250ms to see if we will terminate
                        // fprintf(stdout,"Process %d NOT terminating %d, processBlock->processRequesting AFTER %d, NEW terminate time: %.9f...\n", processId, terminate, processBlock->processRequesting, checkTerminate);
                        if(processBlock->running[processId] == 1){//If didn't get blocked
                                fprintf(stdout,"Process %d NOT terminating\n", processId);
                        }
                }
        }
}

if(oneSecPassed){
        for(r = 0; r < 20; r++){//Check to see if process has any resources to get rid of
                if(processBlock->allocated[processId][r] != 0){
                        hasResources = true;//process has a resource allocated to it
                        break;
                }
        }
}

/***********************************
    Deallocate random resource
***********************************/
//Make sure random time passed, process has resources, is running (not blocked), and has had chance to request resources
if(deallocateTime > checkDeallocate && hasResources && processBlock->running[processId] == 1 && oneSecPassed){
        waitSem();//Only one can deallocate at a time
        // fprintf(stdout,"%d: %.9fs > %.9fs? hasResources = %d\n", processId, deallocateTime, checkDeallocate, hasResources);
        int numResHave = 0;
        int resHave[20];//Array will hold all res classes this process currently wanting
        for(r = 0; r < 20; r++){
                resHave[r] = -1;
        }
        for(r = 0; r < 20; r++){//Count number of resource classes & store the classes in array to choose from
                if(processBlock->allocated[processId][r] != 0){//If have > 0 of this resource
                        resHave[numResHave] = r;
                        numResHave++;
                }
        }
        resourceIndex = rand() % numResHave;//Pick a random resource to deallocate
        amountDeallocate = rand() % processBlock->allocated[processId][resHave[resourceIndex]] + 1;
        // fprintf(stdout,"%d allocated = %d Before, deallocate %d of them\n", resHave[resourceIndex], processBlock->allocated[processId][resHave[resourceIndex]], amountDeallocate);
        processBlock->release[processId][resHave[resourceIndex]] = processBlock->release[processId][resHave[resourceIndex]] + amountDeallocate;
        processBlock->allocated[processId][resHave[resourceIndex]] = processBlock->allocated[processId][resHave[resourceIndex]] - amountDeallocate;//Pick a random resource to deallocate
        if(resource->shared[resHave[resourceIndex]] == 0){//If not sharable, decrement this resource
                resource->available[resHave[resourceIndex]] = resource->available[resHave[resourceIndex]] + amountDeallocate;
        }
        currTime2 = getTotalTime(&timer->clockSecs, &timer->clockNanos, &currTime2);
        checkDeallocate = genRandomDouble(processId);
        hasResources = false;
        fprintf(stdout,"%d Deallocated %d of Resource %d\n", processId, amountDeallocate, resHave[resourceIndex]);
        printResources(processBlock, resource);
        signalSem();
}

currRunTime = getTotalTime(&timer->clockSecs, &timer->clockNanos, &currRunTime);
if(currRunTime - startTime > 1 && oneSecPassed == false){
        oneSecPassed = true;//If process has run for more than 1 second, start seeing if can terminate
        checkTerminate = genRandomDouble(processId);
        checkDeallocate = genRandomDouble(processId);
        terminateTime = 0;//Check if will terminate if terminateTime > checkTerminate. Start at 0 so doesn't try to terminate immediately
        deallocateTime = 0;//Check if will deallocate if deallocateTime > checkDeallocate.
        currTime = getTotalTime(&timer->clockSecs, &timer->clockNanos, &currTime);
        currTime2 = getTotalTime(&timer->clockSecs, &timer->clockNanos, &currTime2);
        // fprintf(stdout,"%d: 1 Sec passed: %.9f - %.9f = %.9f. Random term time: %.9f, terminateTime: %.9f\n", processId, currRunTime, startTime, currRunTime - startTime, checkTerminate, terminateTime);
}else if(oneSecPassed){
        terminateTime = currRunTime - currTime;//Check if will terminate if terminateTime > checkTerminate
        deallocateTime = currRunTime - currTime2;//Check if will terminate if terminateTime > checkTerminate
}

}
processBlock->processDone = processId;
endTime = getTotalTime(&timer->clockSecs, &timer->clockNanos, &endTime);//Get time ended at
processBlock->totalSystemTime[processId] = endTime - processBlock->totalSystemTime[processId];//endTime - Creation time
processBlock->turnAround[processId] = endTime - processBlock->turnAround[processId];// turnAround = endTime - arrivalTime;
processBlock->cpuTimeUsed[processId] = processBlock->cpuTimeUsed[processId] + processBlock->turnAround[processId] - processBlock->idleTotal[processId];
// fprintf(stdout,"%d: arrivalTime %.9f, endTime %.9f, turnAround: %.9f, cpuTimeUsed: %.9f\n", processId, arrivalTime, endTime, processBlock->turnAround[processId], processBlock->cpuTimeUsed[processId]);
printf("Process %d is Done\n",processId);
return 0;
}

void sigHandler(int mysignal){
        pid_t childId;
        struct PCB *processBlock;
        struct timing *timer;
        struct resourceTable *resource;

    //Attach to shared memory
        if((processBlock = (struct PCB *)shmat(shmidVal, (void *)0, 0)) == (void *)-1){ //Returns void*
                fprintf(stderr,"%s Line %d ",__FILE__,__LINE__);
                perror("shmat");
                exit(EXIT_FAILURE);
        }
        if((timer = (struct timing *)shmat(shmidVal2, 0, 0)) == (void *)-1){ //Returns void*
                fprintf(stderr,"%s Line %d ",__FILE__,__LINE__);
                perror("shmat");
                exit(EXIT_FAILURE);
        }
        if((resource = (struct resourceTable *)shmat(shmidVal3, 0, 0)) == (void *)-1){ //Returns void*
                fprintf(stderr,"%s Line %d ",__FILE__,__LINE__);
                perror("shmat");
                exit(EXIT_FAILURE);
        }
        printf("SLAVE process id %d  signal %d\n", getpid(),mysignal);
        switch(mysignal){
                        case SIGINT:
                                fprintf(stderr,"Child %d dying because of an interrupt!\n",childId);
                                //Detach and Remove shared mem
                                if(shmdt(processBlock) == -1){
                                        fprintf(stderr,"%s Line %d ",__FILE__,__LINE__);
                                        perror("shmdt");
                                   exit(EXIT_FAILURE);
                                }
                                if (shmctl(shmidVal,IPC_RMID,NULL) == -1){
                                        fprintf(stderr,"%s Line %d ",__FILE__,__LINE__);
                                        perror("shmctl");
                                        exit(EXIT_FAILURE);
                                }
                                if(shmdt(timer) == -1){
                                        fprintf(stderr,"%s Line %d ",__FILE__,__LINE__);
                                                perror("shmdt");
                                           exit(EXIT_FAILURE);
                                }
                                if (shmctl(shmidVal2,IPC_RMID,NULL) == -1){
                                        fprintf(stderr,"%s Line %d ",__FILE__,__LINE__);
                                        perror("shmctl");
                                        exit(EXIT_FAILURE);
                                }
                                if(shmdt(resource) == -1){
                                        fprintf(stderr,"%s Line %d ",__FILE__,__LINE__);
                                                perror("shmdt");
                                           exit(EXIT_FAILURE);
                                }
                                if (shmctl(shmidVal3,IPC_RMID,NULL) == -1){
                                        fprintf(stderr,"%s Line %d ",__FILE__,__LINE__);
                                        perror("shmctl");
                                        exit(EXIT_FAILURE);
                                }

                                //Delete Semaphore.  IPC_RMID Remove the specified semaphore set
                                if(semctl(semId, 0, IPC_RMID) == -1){
                                        fprintf(stderr,"%s Line %d ",__FILE__,__LINE__);
                                        perror("semctl");
                                        exit(EXIT_FAILURE);
                                }
                                if(semctl(semIdCond, 0, IPC_RMID) == -1){
                                        fprintf(stderr,"%s Line %d ",__FILE__,__LINE__);
                                        perror("semctl");
                                        exit(EXIT_FAILURE);
                                }
                                //raise(SIGTERM);
                                exit(1);
                                break;
                   default:
                                printf("Default signal in slave %d\n", mysignal);
                                break;
        }
        return;
}

void initSigHandler(){
        struct sigaction signalAct;
        signalAct.sa_handler = &sigHandler;//Setup sigHandler
        signalAct.sa_flags = SA_RESTART|SA_SIGINFO;
        sigfillset(&signalAct.sa_mask);//Block all other signals

        if(sigaction(SIGINT, &signalAct, NULL) == -1){
                        perror("SIGINT receipt failed");
        }
}

void printResources(struct PCB* processBlock, struct resourceTable* resource){
        int p, r;
        fprintf(stdout,"*************** Current Available Resources **************\n");
        for(r = 0; r < 20; r++){
                fprintf(stdout,"%d = %d, ", r, resource->available[r]);
        }
        fprintf(stdout,"\n");
        return;
}

int inQueue(int r, int p, int blocked[r][p], int resType, int processId){
        int i;
        for(i = 0; blocked[resType][i] != -1; i++){//Move elements forward in the queue
                if(blocked[resType][i] == processId){
                        return 1;//Process already in queue
                }
        }
        return 0;//Process not in queue
}

int selectFromQueue(int blocked[], int *tail, int processId){
// int selectFromQueue(int r, int p, int blocked[r][p], int resType, int *tail, int processId){
        if(*tail == -1){// Check for empty Queue
                fprintf(stderr,"BLOCKED QUEUE IS EMPTY \n");
                return -1;  // Return 0 if queue is empty
        }else{
                *tail = *tail - 1;
                int i = 0;
                while(blocked[i] != processId){//Find element in queue
                        i++;
                }
                // fprintf(stdout,"Id Found blocked[%d] = %d\n",i, blocked[i]);
                processId = blocked[i];// Get Id to return
                blocked[i] = -1;

                while(blocked[i + 1] != -1){//Move elements forward in the queue
                        blocked[i] = blocked[i + 1];
                        i++; // fprintf(stdout,"Shift blocked[%d] = %d\n",i , blocked[i + 1]);
                }
                // fprintf(stdout,"blocked[%d] = 0\n",i);
                blocked[i] = -1;//Set last spot that was filled to -1 (nothing) since process moved up in queue
                // fprintf(stdout,"Process: Select queue return: %d\n",processId);
                return processId;// Return popped Id
        }
}

void advanceClock(int *clockSecs, int *clockNanos){//1 sec = 1,000 milli = 1,000,000 micro = 1,000,000,000 nano
        waitClock();
        int randTime = rand() % 999 + 1;//simulate overhead activity for each iteration
        *clockNanos += randTime;
        if(*clockNanos > 999999999){
                *clockSecs = *clockSecs + 1;
                *clockNanos -= 1000000000;
        }
        *clockSecs = *clockSecs + 1;
        signalClock();
}

double getTotalTime(int *clockSecs, int *clockNanos, double *time){
        *time = (double)*clockSecs + ((double)*clockNanos/1000000000);
        return *time;
}

double genRandomDouble(int processId){//72500000ns == 72.5 millisecs == .725s
        // fprintf(stdout,"returning %.9f\n", ((double)rand() * ( 0.5 - 0 ) ) / (double)RAND_MAX + 0);
        double randNum = (double)rand() + (double)processId;// + processId so concurrent processes don't get same num since seeded with time
        return (randNum * ( 0.25 - 0 ) ) / (double)RAND_MAX + 0;
        // return ((double)rand() * ( 0.25 - 0 ) ) / (double)RAND_MAX + 0;
}

void waitClock(){
        operation.sem_num = 0;/* Which semaphore in the semaphore array*/
    operation.sem_op = -1;/* Subtract 1 from semaphore value*/
    operation.sem_flg = 0;/* Set the flag so we will wait*/
    if(semop(semId, &operation, 1) == -1){
        exit(EXIT_FAILURE);
    }
}

void signalClock(){
        operation.sem_num = 0;/* Which semaphore in the semaphore array*/
    operation.sem_op = 1;/* Add 1 to semaphore value*/
    operation.sem_flg = 0;/* Set the flag so we will wait*/
        if(semop(semId, &operation, 1) == -1){
        exit(EXIT_FAILURE);
    }
}

void waitRequest(){
        condition.sem_num = 0;/* Which semaphore in the semaphore array*/
    condition.sem_op = -1;/* Subtract 1 from semaphore value*/
    condition.sem_flg = 0;/* Set the flag so we will wait*/
    if(semop(semIdCond, &condition, 1) == -1){
        exit(EXIT_FAILURE);
    }
}

void signalRequest(int processRequesting){
        if(processRequesting == -1){
                condition.sem_num = 0;/* Which semaphore in the semaphore array*/
                condition.sem_op = 1;/* Add 1 to semaphore value*/
                condition.sem_flg = 0;/* Set the flag so we will wait*/
                if(semop(semIdCond, &condition, 1) == -1){
                        exit(EXIT_FAILURE);
                }
        }
}
//Uses same Semaphore as above since both regulate when a process can change resources
//But the area this is used doesn't require a condition to continue
void waitSem(){
        condition.sem_num = 0;/* Which semaphore in the semaphore array*/
    condition.sem_op = -1;/* Subtract 1 from semaphore value*/
    condition.sem_flg = 0;/* Set the flag so we will wait*/
    if(semop(semIdCond, &condition, 1) == -1){
        exit(EXIT_FAILURE);
    }
}

void signalSem(){
        condition.sem_num = 0;/* Which semaphore in the semaphore array*/
    condition.sem_op = 1;/* Add 1 to semaphore value*/
    condition.sem_flg = 0;/* Set the flag so we will wait*/
        if(semop(semIdCond, &condition, 1) == -1){
        exit(EXIT_FAILURE);
    }
}

/*$Author: o2-gray $
 *$Date: 2016/04/10 03:02:59 $
 *$Log: process.c,v $
 *Revision 1.6  2016/04/10 03:02:59  o2-gray
 *Now deallocating and communicating with oss better with many processes.
 *Collecting statistics data along the way now.
 *
 *Revision 1.5  2016/04/06 03:18:40  o2-gray
 *Communicating with oss and deadlock algorithm correctly
 *
 *Revision 1.4  2016/04/03 03:04:41  o2-gray
 *Communicating with oss when needs resources now.
 *
 *Revision 1.3  2016/04/02 03:54:32  o2-gray
 *Everything set up besides requesting and releasing resources between oss and the processes
 *
 *Revision 1.2  2016/04/01 03:07:40  o2-gray
 *Getting values properly from oss right now
 *
 *Revision 1.1  2016/03/30 04:02:54  o2-gray
 *Initial revision
 *
 */
