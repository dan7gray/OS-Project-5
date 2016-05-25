#include "functs.h"

int semKey = 8675309, semId;//For clock
// int semKey2 = 8008135, semId2;//For printing resources in process.c
int semKeyR = 3333333, semIdR;//For Removing resources in process.c
int semKeyCond = 90210, semIdCond;//For processes requesting resources
int key = 4444444;
int key2 = 5555555;
int key3 = 6666666;
int shmidVal;
int shmidVal2;
int shmidVal3;

int main(int argc, char * argv[]){
/*******************************************************************************************
Initialize vars.  totalNumAllowed, numAllowedConcurrently, & runTime can be modified safely
*******************************************************************************************/
int numAllowedConcurrently = 5;//Number of processes allowed to run at the same time
int totalNumAllowed = 15;//Number of processes allowing to be created
double runTime = 18;//Time the program can run until in stops (seconds)

int bv[1] = {0};//bv = bitVector. keep track of PCBs taken
int numberCompleted = 0;//Keep track of number of processes complete for calculations at the end
int processId = 0, processCounter = 0, processToRunId = -1, processRequestId = -1;
pid_t procPid, waitingPid;
int status;
double timePassed = 0, totalTime = 0, prevTotalTime = 0, startTime = 0;
double randTime;//Timing for when to generate next process
srand(time(NULL));
int p, r;//for loop vars

char tempProcessId[sizeof(int)];//For sending processId through execl
char tempShmidVal[sizeof(int)];//For sending shmidVal through execl
char tempShmidVal2[sizeof(int)];//For sending shmidVal2 through execl tempPriority
char tempShmidVal3[sizeof(int)];//For sending shmidVal2 through execl tempPriority

initSigHandler();//Initialize signal handler

struct PCB *processBlock;
struct timing *timer;
struct resourceTable *resource;

//Create & Attach to Shared Memory: shared memory segment specified by shmidVal to the address space
if((shmidVal = shmget(key, sizeof(struct PCB), IPC_CREAT | 0666)) < 0){
        fprintf(stderr,"%s Line %d ",__FILE__,__LINE__);
        perror("shmget");
        exit(EXIT_FAILURE);
}
if((processBlock = (struct PCB*)shmat(shmidVal, 0, 0)) == (void *)-1){ //Returns void*
        fprintf(stderr,"%s Line %d ",__FILE__,__LINE__);
        perror("shmat");
        exit(EXIT_FAILURE);
}
if((shmidVal2 = shmget(key2, sizeof(struct timing), IPC_CREAT | 0666)) < 0){
        fprintf(stderr,"%s %d ",__FILE__, __LINE__);
        perror("shmget");
        exit(EXIT_FAILURE);
}
if((timer = (struct timing*)shmat(shmidVal2, 0, 0)) == (void *)-1){ //Returns void*
        fprintf(stderr,"%s Line %d ",__FILE__,__LINE__);
        perror("shmat");
        exit(EXIT_FAILURE);
}
if((shmidVal3 = shmget(key3, sizeof(struct resourceTable), IPC_CREAT | 0666)) < 0){
        fprintf(stderr,"%s Line %d ",__FILE__,__LINE__);
        perror("shmget");
        exit(EXIT_FAILURE);
}
if((resource = (struct resourceTable*)shmat(shmidVal3, 0, 0)) == (void *)-1){ //Returns void*
        fprintf(stderr,"%s Line %d ",__FILE__,__LINE__);
        perror("shmat");
        exit(EXIT_FAILURE);
}

//Create Semaphores
if((semId = semget(semKey,1,IPC_CREAT | 0666)) == -1){//For advancing clock
        fprintf(stderr,"%s Line %d ",__FILE__,__LINE__);
        perror(" semget");
        exit(EXIT_FAILURE);
}
semctl(semId, 0, SETVAL, 1);//Initialize binary semaphore for clock, unlocked from start
if((semIdCond = semget(semKeyCond,1,IPC_CREAT | 0666)) == -1){//For processes requesting resources
        fprintf(stderr,"%s Line %d ",__FILE__,__LINE__);
        perror(" semget");
        exit(EXIT_FAILURE);
}
semctl(semIdCond, 0, SETVAL, 1);//Initialize binary semaphore for requests, unlocked from start
if((semIdR = semget(semKeyR,1,IPC_CREAT | 0666)) == -1){//For removing resources in process.c
        fprintf(stderr,"%s Line %d ",__FILE__,__LINE__);
        perror(" semget");
        exit(EXIT_FAILURE);
}
semctl(semIdR, 0, SETVAL, 1);//Initialize binary semaphore

/******************************************************
Initialize all resources' values
******************************************************/
for(r = 0; r < 20; r++){
        resource->type[r] = r;//The total amount of this resource
        resource->total[r] = rand() % 10 + 1;//The total amount of this resource [1-10]
        resource->available[r] = resource->total[r];//The total amount available of this resource
        for(p = 0; p < 18; p++){
                processBlock->allocated[p][r] = 0;//Amount of resource r process p is allocated
                processBlock->need[p][r] = 0;//Amount of resource r process p is wanting so can run
                processBlock->release[p][r] = 0;//Amount of resource r process p has released
        }
}

//Processes waiting for resources empty at first
for(r = 0; r < 20; r++){
        for(p = 0; p < 18; p++){
                resource->blocked[r][p] = -1;//Queues empty
                processBlock->maxClaim[p][r] = -1;//No claims made yet
        }
        resource->blockedTail[r] = -1;//Nothing in Queues yet
        resource->shared[r] = false;//Nothing shared yet
}

fprintf(stdout,"********************************Shared resources***********************************\n");
//15-25% of these 20 resources will be shared.  So 3-5 of them.
int amountShared = rand() % 3 + 3;// [3-5]
if(amountShared == 3){
        int processShared1 = rand() % 20; int processShared2 = rand() % 20; int processShared3 = rand() % 20;
        // fprintf(stdout, "All 3: %d, %d, %d\n",processShared1, processShared2, processShared3);
        while(true){
                if(processShared1 != processShared2 != processShared3){
                        // fprintf(stdout, "All 3 different: %d, %d, %d\n",processShared1, processShared2, processShared3);
                        for(r = 0; r < 20; r++){
                                if(r == processShared1 || r == processShared2 || r == processShared3){
                                        resource->shared[r] = true;
                                }
                        }
                        break;
                }
                processShared1 = rand() % 20; processShared2 = rand() % 20; processShared3 = rand() % 20;
                // processShared1 = rand() % 20; processShared2 = rand() % 20; processShared3 = rand() % 20;
        }
}else if(amountShared == 4){
        int processShared1 = rand() % 20; int processShared2 = rand() % 20;
        int processShared3 = rand() % 20; int processShared4 = rand() % 20;
        // fprintf(stdout, "All 4: %d, %d, %d, %d\n",processShared1, processShared2, processShared3, processShared4);
        while(true){
                if(processShared1 != processShared2 != processShared3 != processShared4){
                        // fprintf(stdout, "All 4 different: %d, %d, %d, %d\n",processShared1, processShared2, processShared3, processShared4);
                        for(r = 0; r < 20; r++){
                                if(r == processShared1 || r == processShared2 || r == processShared3 || r == processShared4){
                                        resource->shared[r] = true;
                                }
                        }
                        break;
                }
                processShared1 = rand() % 20; processShared2 = rand() % 20;
                processShared3 = rand() % 20; processShared4 = rand() % 20;
        }
} else if(amountShared == 5){
        int processShared1 = rand() % 20; int processShared2 = rand() % 20; int processShared3 = rand() % 20;
        int processShared4 = rand() % 20; int processShared5 = rand() % 20;
        // fprintf(stdout, "All 5: %d, %d, %d, %d, %d\n",processShared1, processShared2, processShared3, processShared4, processShared5);
        while(true){
                if(processShared1 != processShared2 != processShared3 != processShared4 != processShared5){
                        // fprintf(stdout, "All 5 different: %d, %d, %d, %d, %d\n",processShared1, processShared2, processShared3, processShared4, processShared5);
                        for(r = 0; r < 20; r++){
                                if(r == processShared1 || r == processShared2 || r == processShared3 || r == processShared4|| r == processShared5){
                                        resource->shared[r] = true;
                                        // fprintf(stdout,"Resource[%d] shared: %d\n",r,resource->shared[r]);
                                }
                        }
                        break;
                }
                processShared1 = rand() % 20; processShared2 = rand() % 20; processShared3 = rand() % 20;
                processShared4 = rand() % 20; processShared5 = rand() % 20;
        }
}
for(r = 0; r < 20; r++){
        if(resource->shared[r] == 1){
                fprintf(stdout,"Resource %d, ",r);
        }
}
fprintf(stdout,"\n\n");

fprintf(stdout,"********************************Total Resources Starting***********************************\n");
for(r = 0; r < 20; r++){//Print resource totals
        fprintf(stdout,"Res %d = %d, ", r, resource->total[r]);
}
fprintf(stdout,"\n\n");
fprintf(stdout,"This program will run and display each process initial needs upon creation,\n");
fprintf(stdout,"whether the process is terminating or not, the resources it deallocates or \n");
fprintf(stdout,"is allocated to it, and an update of the available resources whenever a process\n");
fprintf(stdout,"is allocated a resource or deallocates a resource.  The screen may go by fast,\n");
fprintf(stdout,"but I wanted to display some visual information for you to follow so you can see it works.\n");
fprintf(stdout,"NOTE: There are sometimes 2 shared resources rather than > 3 (see above), keyword: sometimes.\n");
fprintf(stdout,"Mark said we didn't have to do the 15-25% resources shared anymore (like I already did), but already had it\n");
fprintf(stdout,"already had it implemented and I didn't want to waste the little time I have changing it.\n");
fprintf(stdout,"Soooo... Extra credit? haha\n");
fprintf(stdout,"PRESS ANY KEY TO CONTINUE\n");
getchar();

//Initialize clock items
timer->clockSecs = 0;  timer->clockNanos = 0;

processBlock->processDone = -1;//Hold Id of process ending
processBlock->processRequesting = -1;//Hold Id of process requesting
randTime = genRandomDouble();//Timing for when to generate next process

int t = 0;
// numAllowedConcurrently = 5;//Number of processes allowed to run at the same time
// totalNumAllowed = 15;//Number of processes allowing to be created
//runTime = 18;
while(true){

        if(((totalTime - prevTotalTime) > randTime) && processCounter < numAllowedConcurrently && processId < totalNumAllowed){
                // fprintf(stdout,"processCounter %d, processId %d\n", processCounter, processId);
                // fprintf(stdout,"AFTER time: %.9f, prevTotalTime: %.9f, time passed: %.9f\n",totalTime, prevTotalTime, totalTime - prevTotalTime);
                prevTotalTime = totalTime;//Set so we can use to determine when next process can be created
                totalTime = getTotalTime(&timer->clockSecs, &timer->clockNanos, &totalTime);

                if(bvFull(bv, totalNumAllowed) == 1){//Make sure bitVector/Process table not full. If is, don't generate new process
                        fprintf(stdout,"BV full.\n", processId);
                        continue;//If full, don't create newe process
                }
                if(member(bv, processId) == 0){//Insert process into bitVector.  Stop creating processes once bitVector full
                        set(bv, processId);//Put in bitvector
                }else{
                        fprintf(stdout,"Couldn't insert %d in bv.\n", processId);
                }

                if((procPid = fork()) == 0){//is child, new process receives a copy of the address space of the parent
                        // fprintf(stdout,"New child %d / %d, time: %.9f\n",processId, getpid(),prevTotalTime);
                        processBlock->running[processId] = 1;
                        processBlock->requesting[processId] = -1;
                        processBlock->pid[processId] = getpid();
                        processBlock->cpuTimeUsed[processId] = 0;//total CPU time used
                        processBlock->totalSystemTime[processId] = totalTime;//total CPU time used
                        processBlock->turnAround[processId] = 0;
                        processBlock->idle[processId] = 0;
                        processBlock->idleTotal[processId] = 0;

                        sprintf(tempProcessId,"%d", processId);//For passing the ID through exec
                        sprintf(tempShmidVal,"%d", shmidVal);//For passing the shared mem ID through exec
                        sprintf(tempShmidVal2,"%d", shmidVal2);//For passing the shared mem ID through exec
                        sprintf(tempShmidVal3,"%d", shmidVal3);//For passing the shared mem ID through exec

                        execl("./process",tempProcessId,tempShmidVal,tempShmidVal2,tempShmidVal3,NULL);//Execute executible
                        perror("Child failed to execl ");
                        exit(EXIT_FAILURE);
                }else if(procPid < 0){//fork() fails with -1
                        fprintf(stderr,"%s Line %d ",__FILE__,__LINE__);
                        perror("fork() error");
                        exit(EXIT_FAILURE);
                }
                randTime = genRandomDouble();//Create new random time to wait before making new process
                processId++;
                processCounter++;
        }

        if(processBlock->processDone != -1){//A process is done, remove from bitVector
                waitRemove();
                numberCompleted++;
                clearBit(bv, processBlock->processDone);
                waitingPid = waitpid(processBlock->pid[processBlock->processDone], &status, WUNTRACED);
                if(waitingPid == -1){//Wait for this process to end before decr processCounter and creating new process
                        fprintf(stderr,"%s Line %d ",__FILE__,__LINE__);
                        perror("waitpid");
                        exit(EXIT_FAILURE);
                }
                fprintf(stdout,"Process %d terminating...\n", processBlock->processDone);
                processBlock->processDone = -1;//Wait til next process done
                processCounter--;
                signalRemove();
        }

        processRequestId = processBlock->processRequesting;
        if(allQueuesEmpty(20, 18, resource->blocked) == 0 || processBlock->processRequesting != -1){//If processes in blocked queue waiting for resources
                int pid = deadlock(processBlock, resource, processRequestId);
                // fprintf(stdout,"DEADLOCK ALG returned %d, processBlock->processRequesting = %d, processBlock->running[%d] = %d\n", pid, processBlock->processRequesting, pid, processBlock->running[pid]);
                if(processBlock->processRequesting == -1 && pid != -1 && processBlock->running[pid] == 1){//If tried running a queued process && was allocated to
                        processBlock->idle[pid] = getTotalTime(&timer->clockSecs, &timer->clockNanos, &totalTime) - processBlock->idle[pid];//Curr time - time blocked
                        processBlock->idleTotal[pid] = processBlock->idleTotal[pid] + processBlock->idle[pid];
                        processBlock->idle[pid] = 0;
                }
                pid = -1;
                processBlock->processRequesting = -1;//Allow another process to be added, this is condition for process semaphore
        }

        advanceClock(&timer->clockSecs, &timer->clockNanos);//advance clock to simulate overhead
        totalTime = getTotalTime(&timer->clockSecs, &timer->clockNanos, &totalTime);

        if(t > 500000 && countArray(processBlock->running) == 0){//So get some feedback if all processes done and oss time not up
                fprintf(stdout,"OSS running...\n", totalTime - prevTotalTime, randTime, processBlock->processRequesting);
                t = 0;
        }
        t++;
/***************************************************************************************************************
If time is up and no processes are running.  A process may be in a Queue still and we may not get a "Deadlocked"
warning and termination if the time runs out before we try to allocate to that process to get the error
****************************************************************************************************************/
        if(totalTime > runTime && countArray(processBlock->running) == 0){
                fprintf(stdout,"Time is up!\n");
                break;
        }
}

sleep(1);//To ensure final/remaining values can be updated by running processes before calculations
/************************************************************************************************************
Statistics: Average CPU time, Average turnaround time, Average waiting time, Average Idle times for processes
*************************************************************************************************************/
double total = 0;
int numProc = 0;
fprintf(stdout,"\n***************************** CPU Utilization Times ********************************\n");
fprintf(stdout,"Total CPU Time    Idle Time    CPU Time Used     Cummulative CPU Time Used\n");
for(p = 0; p < 18; p++){
        if(processBlock->cpuTimeUsed[p] == 0){
                numProc = p;
                break;
        }
        if(processBlock->idleTotal[p] > 0){
                fprintf(stdout,"%d: %.9f - %.9f = %.9f    ",p,processBlock->cpuTimeUsed[p] + processBlock->idleTotal[p], processBlock->idleTotal[p], processBlock->cpuTimeUsed[p]);
        }else{
                fprintf(stdout,"%d: %.9f                                          ",p,processBlock->cpuTimeUsed[p]);
        }
        total += processBlock->cpuTimeUsed[p];//Idle time already subtracted for cpuTimeUsed
        fprintf(stdout,"total: %.9f\n",total);
}
fprintf(stdout,"\nCummulative CPU Time Used between All processes: %.9f\n",total);
fprintf(stdout,"Average CPU Time Used: %.9f/%d = %.9f\n", total, numProc, (total/(double)numProc));
fprintf(stdout,"CPU ran for: %.9fs\n",totalTime);
/************************************************************************************************************
CPU idle time left out.  Unlike last assignment, a process or oss will always be running since all are running
concurrently.  We aren't waiting for processes/oss to finish before oss/another process can run, so the CPU is
always running.
*************************************************************************************************************/
fprintf(stdout,"\n***************************** Idle Times **********************************\n");
total = 0;
for(p = 0; p < numProc; p++){
        if(processBlock->idleTotal[p] > 0){
                fprintf(stdout,"%d: %.9f          ", p, processBlock->idleTotal[p]);
                total = total + processBlock->idleTotal[p];
        }
}
if(total > 0){
        fprintf(stdout,"\nAverage idle time: %.9f/%d = %.9f\n", total, numProc, total/(double)numProc);
}else{
        fprintf(stdout,"No processes were Blocked during this execution\n");
}

fprintf(stdout,"\n***************************** Turnaround Times **********************************\n");
total = 0;
for(p = 0; p < numProc; p++){
        fprintf(stdout,"%d: %.9f          ", p, processBlock->turnAround[p]);
        total = total + processBlock->turnAround[p];
        if(p % 3 == 0){
                fprintf(stdout,"\n");
        }
}
fprintf(stdout,"\nAverage turnaround time: %.9f/%d = %.9f\n", total, numProc, total/(double)numProc);

fprintf(stdout,"\n***************************** Waiting Times ********************************\n");
total = 0;
//Waiting time of a process = finish time of that process - execution time - arrival time = turnAround - CPU time used
for(p = 0; p < numProc; p++){//Waiting time = endTime - creation time - turnaround
        fprintf(stdout,"%d: %.9f          ",p,processBlock->totalSystemTime[p] - processBlock->turnAround[p]);
        total = total + processBlock->totalSystemTime[p] - processBlock->turnAround[p];
        if(p % 3 == 0){
                fprintf(stdout,"\n");
        }
}
fprintf(stdout,"\nAverage Waiting time: %.9f/%d = %.9f\n",total, numProc, total/(double)numProc);
/************************************************************************************************************
No scheduling in this project, so waiting time is relatively short since upon creation, the process will
arrive in a short period of time and start executing immediately
*************************************************************************************************************/
fprintf(stdout,"\n***************************** Throughput ********************************\n");
fprintf(stdout,"Number of Processes Completed = %d, Total Time %.9f%\n", numProc, totalTime);
fprintf(stdout,"Throughput: %d/%.9f = %.9f\n",numProc, totalTime, totalTime/(double)numProc);
fprintf(stdout,"A process is created at a rate approximately every %.9fs\n\n", totalTime/(double)numProc);

fprintf(stdout,"\n***************************** Resources Released (Class: Amount) ********************************\n");
for(p = 0; p < numProc; p++){
        fprintf(stdout,"Process %d released: ", p);
        for(r = 0; r < 20; r++){
                if(processBlock->release[p][r] > 0){
                        fprintf(stdout,"%d: %d, ", r, processBlock->release[p][r]);
                }
        }
        fprintf(stdout,"\n");
}

if(procPid != 0){//child's pid returned to parent
        // printf("Master ID %d\n",getpid());
   bool childrenAlive = false;
   for(p = 0; p < 18; p++){
                if(processBlock->running[p] == 1){//If > 0 processes running, not deadlocked
                        childrenAlive = true;
                        break;
                }
        }
        if(allQueuesEmpty(20, 18, resource->blocked) == 0){//If > 0, processes running, not deadlocked
                childrenAlive = true;
        }
   if(childrenAlive){//If processes still alive, kill them
                fprintf(stdout,"Time is up, but children remain and must be eradicated...\n");
                fprintf(stdout,"     _.--''--._\n");
                fprintf(stdout,"    /  _    _  \\ \n");
                fprintf(stdout," _  ( (_\\  /_) )  _\n");
                fprintf(stdout,"{ \\._\\   /\\   /_./ }\n");
                fprintf(stdout,"/_'=-.}______{.-='_\\ \n");
                fprintf(stdout," _  _.=('''')=._  _\n");
                fprintf(stdout,"(_''_.-'`~~`'-._''_)\n");
                fprintf(stdout," {_'            '_}\n");
                fprintf(stderr,"\n");
                if(allQueuesEmpty(20, 18, resource->blocked) == 0){
                        fprintf(stderr,"Processes still exist in a blocked Queue\n");
                        printQueue(20, 18, resource->blocked);
                }
                fprintf(stdout,"Still running: ");
                for(p = 0; p < 18; p++){
                        if(processBlock->running[p] == 1){//If > 0 processes running, not deadlocked
                                fprintf(stdout,"%d, ", p);
                        }
                }
                fprintf(stdout,"\n");
                fprintf(stdout,"Not running: ");
                for(p = 0; p < 18; p++){
                        if(processBlock->running[p] == 0){//If > 0 processes running, not deadlocked
                                fprintf(stdout,"%d, ", p);
                        }
                }
                fprintf(stdout,"\n");
                kill(-getpid(),SIGINT);
        }
}

//Detach from shared mem and Remove shared mem when all processes done
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
if(semctl(semId, 0, IPC_RMID) == -1){//For clock
    fprintf(stderr,"%s Line %d ",__FILE__,__LINE__);
        perror("semctl");
        exit(EXIT_FAILURE);
}
if(semctl(semIdCond, 0, IPC_RMID) == -1){//For processes requesting resources
        fprintf(stderr,"%s Line %d ",__FILE__,__LINE__);
        perror("semctl");
        exit(EXIT_FAILURE);
}
if(semctl(semIdR, 0, IPC_RMID) == -1){//For clock
    fprintf(stderr,"%s Line %d ",__FILE__,__LINE__);
        perror("semctl");
        exit(EXIT_FAILURE);
}

fprintf(stdout,"Master process is complete\n");
return 0;
}//END main

void sigHandler(int mysignal){
        // printf("MASTER process id %d  signal %d\n", getpid(),mysignal);
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

        switch(mysignal){
                case SIGINT:
                        fprintf(stderr,"The Parent killed all of the children because something interrupted it (probably the children)!\n");
                        // kill(-getpid(),SIGTERM);
                        //Detach & Remove shared mem when all processes done
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
                        if(semctl(semIdR, 0, IPC_RMID) == -1){//For clock
                                fprintf(stderr,"%s Line %d ",__FILE__,__LINE__);
                                perror("semctl");
                                exit(EXIT_FAILURE);
                        }

                        exit(1);
                        break;
                default:
                        printf("Default signal %d\n", mysignal);
                        break;
        }
        return;
}

void initSigHandler(){
        struct sigaction signalAct;
        signalAct.sa_handler = &sigHandler;//Setup sigHandler
        signalAct.sa_flags = SA_RESTART|SA_SIGINFO;
        // sigfillset(&signalAct.sa_mask);//Block all other signals

        if(sigaction(SIGINT, &signalAct, NULL) == -1){
                perror("SIGINT receipt failed");
        }
}

int deadlock(struct PCB* processBlock, struct resourceTable* resource, int processToRunId){
        int r, p, i;//for loop vars
        int safe, exec = 0, processId;
        int processNumPassed = processToRunId;
        int resourceGiving, resAmountGiving, notRunning = 0, num = 0;
        bool processRunning = false;

        // fprintf(stdout,"processToRunId = %d\n", processToRunId);
        if(processNumPassed == -1 && allQueuesEmpty(20, 18, resource->blocked) == 0){//If trying a queued process && Queues not empty
                // fprintf(stdout,"Try a blocked process from:\n", processToRunId);
                int processQueued[20];//Array will hold all process Ids currently blocked
                int index = 0;
                for(r = 0; r < 20; r++){
                        processQueued[r] = -1;
                }
                for(r = 0; r < 20; r++){
                        if(resource->blockedTail[r] != -1){//If Queue has something in it
                                for(p = 0; p < 18; p++){//Add all processes in this queue
                                        if(resource->blocked[r][p] != -1){
                                                processQueued[index] = resource->blocked[r][p];
                                                // fprintf(stdout,"resource->blockedTail[%d] = %d, resource->blocked[%d][0] = %d, processQueued[%d] = %d\n",r, resource->blockedTail[r], r, resource->blocked[r][0], index, processQueued[index]);
                                                index++;
                                        }else{
                                                break;
                                        }
                                }
                        }
                }
                if(index == 0){
                }else if(index == 1){//If only one process Blocked
                        processToRunId = processQueued[0];
                        // fprintf(stdout,"Try running %d from blocked Queue\n", processToRunId);
                }else{//Choose random process
                        int processIndex = rand() % index;
                        processToRunId = processQueued[processIndex];
                        // fprintf(stdout,"Try running %d from blocked Queue:\n", processToRunId);
                }
        }else if(processNumPassed == -1){
                // fprintf(stdout,"NOTHING RUNNING processToRunId = %d\n", processToRunId);
                return 1;
        }

        if(processToRunId == -1){
                // fprintf(stdout,"PROCESS ENDED before allocation processToRunId = %d\n", processToRunId);
                return 1;
        }

        // fprintf(stdout,"processToRunId = %d, requesting: %d\n", processToRunId, processBlock->requesting[processToRunId]);
        safe = 0;
        p = processToRunId;
        exec = 1;
        if(processNumPassed != -1){//Not trying queued process
                for(r = 0; r < 20; r++){
                        if(resource->available[r] < processBlock->need[p][r]){//Not enough resources available for this process
                                if(!inQueue(20, 18, resource->blocked, r, p)){//If process not already in queue for requesting resources
                                        fprintf(stdout,"Not enough %d resources for %d to run, ", r, p);
                                        addQueue(p, r, resource->blocked, &resource->blockedTail[r]);//Add to blocked queue since can't run
                                        processBlock->running[p] = 0;
                                        processBlock->requesting[p] = r;//Waiting on this resource to become available
                                        printQueue(20, 18, resource->blocked);
                                }else{
                                        // fprintf(stdout,"%d ALREADY QUEUED, running = %d\n", p, processBlock->running[p]);
                                }
                                exec = 0;
                                break;
                        }
                }
        }else if(processNumPassed == -1){//Trying a queued process
                if(resource->available[processBlock->requesting[p]] < processBlock->need[p][processBlock->requesting[p]]){
                        exec = 0;//Resource not available for this process yet
                }
        }
        if(exec){
                // fprintf(stdout,"Process %d safe to allocate to!\n", p);
                safe = 1;
                // printNeeds(processBlock , resource, p, 0);//Print this processes Needs
                resourceGiving = processBlock->requesting[p];//Resource to allocate
                resAmountGiving = rand() % processBlock->need[p][resourceGiving] + 1;//Allocate a random amount
                if(processNumPassed == -1){//Resource allocated, no longer in Queue
                        int w = selectFromQueue(resource->blocked[resourceGiving], &resource->blockedTail[resourceGiving], p);//Remove from queue
                        if(w == -1){//Process ended sometime recently
                                return 1;
                        }
                        fprintf(stdout,"Process %d REMOVED from queue\n", w);
                        printQueue(20, 18, resource->blocked);
                }
                // fprintf(stdout,"%d numResClasses = %d, Giving %d of res %d\n", p, numResClasses, resAmountGiving, resourceGiving);
                processBlock->allocated[p][resourceGiving] = processBlock->allocated[p][resourceGiving] + resAmountGiving;//Allocate resource for this process
                processBlock->need[p][resourceGiving] = processBlock->maxClaim[p][resourceGiving] - processBlock->allocated[p][resourceGiving];//Need = Max - Allocated
                if(resource->shared[resourceGiving] == 0){//If resources isn't shared, decrement
                        resource->available[resourceGiving] = resource->available[resourceGiving] - resAmountGiving;//Update available number of resources
                        fprintf(stdout,"Process %d Allocated %d of resource %d\n", p, resAmountGiving, resourceGiving);
                }else{
                        fprintf(stdout,"Process %d Allocated %d of Sharable resource %d\n", p, resAmountGiving, resourceGiving);
                }
                // printNeeds(processBlock , resource, p, 1);//Print this processes Needs
        }

        if (!safe) {//Check for Deadlock
                // printf("Process %d is in an unsafe state.\n", p);
                for(p = 0; p < 18; p++){
                        if(processBlock->running[p] == 1){//If > 0 processes running, not deadlocked
                                processRunning = true;
                                break;
                        }
                }
                if(processRunning == false){//If no processes running, they are all blocked & deadlocked
                        fprintf(stdout,"DEADLOCKED! Killing the children...\n");
                        kill(-getpid(),SIGINT);
                }
                return -1;
        } else {
                // printf("The process is in safe state\n");
                if(processNumPassed == -1){
                        processBlock->running[p] = 1;//Queued process is running again
                }
                int numResClasses = 0;
                int resWanted[20];//Array will hold all res classes this process currently wanting
                for(r = 0; r < 20; r++){
                        resWanted[r] = -1;
                }
                for(r = 0; r < 20; r++){//Count number of resource classes & store the classes in array to choose from
                        if(processBlock->need[p][r] != 0){
                                resWanted[numResClasses] = r;
                                numResClasses++;
                        }
                }
                int resourceIndex = rand() % numResClasses;//Pick a random resource to allocate
                processBlock->requesting[p] = resWanted[resourceIndex];//Pick a random resource to allocate
        }
        // fprintf(stdout,"LEAVE deadlock alg\n");
        return p;
}

void printNeeds(struct PCB* processBlock, struct resourceTable* resource, int processId, int flag){
        int p, r;
        if(flag == 0){
                fprintf(stdout,"*--------------- Process %d Needs ---------------*\n", processId);
        }else{
                fprintf(stdout,"*--------------- Process %d Needs After ---------------*\n", processId);
        }
        for(r = 0; r < 20; r++){
                fprintf(stdout,"%d = %d, ", r, processBlock->need[processId][r]);
        }
        fprintf(stdout,"\n");
        return;
}

int addQueue(int processId, int resType, int blocked[20][18], int *tail){
        fprintf(stdout,"Added Process %d to Resource %d's Blocked Queue\n", processId, resType);
        if(*tail == MAX_SIZE){ // Check to see if the Queue is full
                fprintf(stdout,"%d QUEUE IS FULL\n",processId);
                return 0;
        }
        *tail = *tail + 1;
        blocked[resType][*tail % MAX_SIZE] = processId;// Add the item to the Queue
        // fprintf(stdout,"blocked[%d][%d] = %d = %d, tail = %d \n",resType, *tail % MAX_SIZE, processId,blocked[resType][*tail % MAX_SIZE], *tail);
        return 1;
}

void printQueue(int r, int p, int blocked[r][p]){
        fprintf(stdout,"Blocked Queues: ");
        int x, y;
        for(y = 0; y < 20; y++){
                fprintf(stdout,"%d [", y);
                for(x = 0; blocked[y][x] != -1; x++){
                        fprintf(stdout,"%d ",blocked[y][x]);
                }
                fprintf(stdout,"], ", y);
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

int queueEmpty(int r, int p, int blocked[r][p], int resType){
        int n = 0, x;
        for(x = 0; blocked[resType][x] != -1; x++){
                n++;
                return n;
        }
        return n;//If 0 is empty
}

int allQueuesEmpty(int r, int p, int blocked[r][p]){
        int x, y;
        for(x = 0; x < p; x++){
                for(y = 0; blocked[x][y] != -1; y++){
                        return 0;
                }
        }
        return 1;//All queues empty
}

int selectFromQueue(int blocked[], int *tail, int processId){
        // fprintf(stderr,"tail: %d, processId: %d \n", *tail, processId);
        if(*tail == -1){// Check for empty Queue
                // fprintf(stderr,"BLOCKED QUEUE IS EMPTY \n");
                return -1;  // Return 0 if queue is empty
        }else{
                *tail = *tail - 1;
                // fprintf(stdout,"processId Want = %d, tail = %d\n",processId, *tail);
                int i = 0;
                while(blocked[i] != processId){//Find element in queue
                        // fprintf(stdout,"Id search blocked[%d] = %d != %d\n",i, blocked[i], processId);
                        i++;
                }
                // fprintf(stdout,"Id Found blocked[%d] = %d\n",i, blocked[i]);
                processId = blocked[i];// Get Id to return
                blocked[i] = -1;
                // int i = 1;
                while(blocked[i + 1] != -1){//Move elements forward in the queue
                        // fprintf(stdout,"Shift blocked[%d] = %d\n",i , blocked[i + 1]);
                        blocked[i] = blocked[i + 1];
                        i++;
                }
                // fprintf(stdout,"blocked[%d] = 0\n",i);
                blocked[i] = -1;//Set last spot that was filled to 0 since process moved up in queue
                // fprintf(stdout,"Oss: Select queue return: %d\n",processId);
                return processId;// Return popped Id
        }
}

int countArray(int array[]){
        int n = 0, x;
        for(x = 0; x < 18; x++){
                if(array[x] != 0){
                        n++;
                }
        }
        return n;
}

void advanceClock(int *clockSecs, int *clockNanos){//1 sec = 1,000 milli = 1,000,000 micro = 1,000,000,000 nano
        waitClock();
        // int randTime = rand() % 9999 + 1;//simulate overhead activity for each iteration by 0.001s
        int randTime = rand() % 999 + 1;//simulate overhead activity for each iteration by 0.0001s
        *clockNanos += randTime;
        // fprintf(stdout,"ENTER clockNanos %d, randTime %d, %d += %d\n",*clockNanos,randTime,*clockNanos,randTime);
        if(*clockNanos > 999999999){
                *clockSecs = *clockSecs + 1;
                *clockNanos -= 1000000000;
        }
        // fprintf(stdout,"LEAVE clockSecs %d, clockNanos %d\n",*clockSecs,*clockNanos);
        signalClock();
}

double getTotalTime(int *clockSecs, int *clockNanos, double *time){
        // double time = (double)*clockSecs + ((double)*clockNanos/1000000000);
        *time = 0;
        *time = (double)*clockSecs + ((double)*clockNanos/1000000000);
        return *time;
}

void clearBit(int bv[], int i){//Clear the process from the process table
  int j = i/32;
  int pos = i%32;//2 % 32 = 2.  33 % 32 = 1.
  unsigned int flag = 1;  // flag = 0000.....00001
  flag = flag << pos;     // flag = 0000...010...000   (shifted i positions)
  flag = ~flag;           // flag = 1111...101..111 - Reverse the bits
  bv[j] = bv[j] & flag;   // RESET the bit at the i-th position in bv[i]
  //OR this is all the above consolidated: bv[i/32] &= ~(1 << (i%32));
}

void set(int bv[], int i){//set value in bv.  i is value to set in bv array
  int j = i/32;//Get the array position
  int pos = i%32;//Get the bit position
  unsigned int flag = 1;   // flag = 0000.....00001
  flag = flag << pos;      // flag = 0000...010...000   (shifted i positions)
  int p;
        // fprintf(stdout,"set flag: %d\n", flag);

  bv[j] = bv[j] | flag;    // Set the bit at the i-th position in bv[i]
  //OR this is all the above consolidated: bv[i/32] |= 1 << (i%32);
}

int member(int bv[], int i){//check if i in bv
  int j = i/32;//Get the array position
  int pos = i%32;//Get the bit position
  unsigned int flag = 1;  // flag = 0000.....00001
  flag = flag << pos;     // flag = 0000...010...000   (shifted i positions)
  int p;
        // fprintf(stdout,"member flag: %d, number looking for: %d\n", flag, i);
  // fprintf(stdout,"member bv[%d]: %d\n", j, bv[j]);
  if (bv[j] & flag)// Test the bit at the k-th position in bv[i]
         return 1;
  else
         return 0;
 //OR this is all the above consolidated: return ((bv[i/32] & (1 << (i%32) )) != 0) ;
}

int bvFull(int bv[], int numProcesses){//check if i in bv
        int i;
        for(i = 0; i <= numProcesses; i++){
                  int j = i/32;//Get the array position
                  int pos = i%32;//Get the bit position
                  unsigned int flag = 1;  // flag = 0000.....00001
                  flag = flag << pos;     // flag = 0000...010...000   (shifted i positions)
                  int p;
                  if (bv[j] & flag){// Test the bit at the k-th position in bv[i]
                        //Spot taken
                  }else{
                          return 0;//Spot open
                  }
        }
        return 1;//Is full
}

int bvEmpty(int bv[], int numProcesses){//check if i in bv
        int i;
        for(i = 1; i <= numProcesses; i++){
                  int j = i/32;//Get the array position
                  int pos = i%32;//Get the bit position
                  unsigned int flag = 1;  // flag = 0000.....00001
                  flag = flag << pos;     // flag = 0000...010...000   (shifted i positions)
                  int p;
                  if (bv[j] & flag){// Test the bit at the k-th position in bv[i]
                        return 0;//Not empty
                  }else{
                  }
        }
        return 1;//Is empty
}

void waitClock(){
        //fprintf(stdout,"waitClock child %d\n",childId);
        operation.sem_num = 0;/* Which semaphore in the semaphore array*/
    operation.sem_op = -1;/* Subtract 1 from semaphore value*/
    operation.sem_flg = 0;/* Set the flag so we will wait*/
    if(semop(semId, &operation, 1) == -1){
        exit(EXIT_FAILURE);
    }
}

void signalClock(){
        //fprintf(stdout,"signalClock\n");
        operation.sem_num = 0;/* Which semaphore in the semaphore array*/
    operation.sem_op = 1;/* Add 1 to semaphore value*/
    operation.sem_flg = 0;/* Set the flag so we will wait*/
        if(semop(semId, &operation, 1) == -1){
        exit(EXIT_FAILURE);
    }
}

void waitRemove(){
        // fprintf(stdout,"waitClock child %d\n",childId);
        removeP.sem_num = 0;/* Which semaphore in the semaphore array*/
    removeP.sem_op = -1;/* Subtract 1 from semaphore value*/
    removeP.sem_flg = 0;/* Set the flag so we will wait*/
    if(semop(semIdR, &removeP, 1) == -1){
        exit(EXIT_FAILURE);
    }
}

void signalRemove(){
        // fprintf(stdout,"signalClock\n");
        removeP.sem_num = 0;/* Which semaphore in the semaphore array*/
    removeP.sem_op = 1;/* Add 1 to semaphore value*/
    removeP.sem_flg = 0;/* Set the flag so we will wait*/
        if(semop(semIdR, &removeP, 1) == -1){
        exit(EXIT_FAILURE);
    }
}

double genRandomDouble(){
        // fprintf(stdout,"returning %.9f\n", ((double)rand() * ( 0.5 - 0 ) ) / (double)RAND_MAX + 0);
        // return ((double)rand() * (max - min)) / (double)RAND_MAX + min;
        return ((double)rand() * ( 0.5 - 0 ) ) / (double)RAND_MAX + 0;//[0-500ms]
}

/*$Author: o2-gray $
 *$Date: 2016/04/10 23:09:34 $
 *$Log: oss.c,v $
 *Revision 1.10  2016/04/10 23:09:34  o2-gray
 *After 40+ hours... Done!
 *
 *Revision 1.9  2016/04/10 03:01:56  o2-gray
 *Finishing up final details and putting statistics together
 *
 *Revision 1.8  2016/04/09 03:17:39  o2-gray
 *Testing with multiple processes now
 *
 *Revision 1.7  2016/04/08 03:17:53  o2-gray
 *Processes not in Queues running good, now implementing taking care of
 *running a Blocked Process in a queue if another process not Requesting
 *resources at the time.
 *
 *Revision 1.6  2016/04/06 03:17:56  o2-gray
 *Algorithm running with 1 process.
 *ToDo: Test multiple processes, statistics at the end
 *
 *Revision 1.4  2016/04/02 03:56:43  o2-gray
 *Starting to try making oss communicate with processes on resource needs.
 *To do: Bankers algorithm, statistics, test with more processes
 *
 *Revision 1.3  2016/04/01 03:06:53  o2-gray
 *All resource values are initialized and starting to exec to child processes
 *Signaling created and handled
 *
 *Revision 1.2  2016/03/31 03:36:27  o2-gray
 *Changed how I'm doing my resource struct so made sure everything initializing correctly.
 *
 *Revision 1.1  2016/03/30 04:00:10  o2-gray
 *Initial revision
 *
 */
