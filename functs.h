#ifndef FUNCTS_H
#define FUNCTS_H

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/shm.h>//Shared Mem
#include <sys/ipc.h>
#include <sys/wait.h>//wait()
#include <signal.h>
#include <unistd.h> //fork()
#include <sys/stat.h>
#include <sys/time.h>
#include <string.h>
#include <sys/sem.h>
#include <stdbool.h>

#define MAX_SIZE 18

struct PCB{//In shared mem
        int pid[18];
        int maxClaim[18][20];//Process's maximum claim on resources
        int allocated[18][20];//18 processes, each could have any of 20 resources
        int need[18][20];//18 processes, each could request any of 20 resources
        int release[18][20];//Resources released by each process
        int running[18];
        int requesting[18];
        double cpuTimeUsed[18];//total CPU time used
        double totalSystemTime[18];//total time in the system
        double idle[18];//total time in the system
        double idleTotal[18];//total time in the system
        double turnAround[18];
        int processDone;
        int processRequesting;
};

struct timing{//In shared mem
        //1 sec = 1000 milli = 1000000 micro = 1000000000 nano
        unsigned int clockSecs;
        unsigned int clockNanos;
};

struct resourceTable{//20 resources
        int type[20];
        int total[20];
        int available[20];
        bool shared[20];
        int blocked[20][18];//Queue of processes requesting resources
        int blockedTail[20];
};


int deadlock(struct PCB*, struct resourceTable*, int processToRunId);
void printResources(struct PCB* processBlock, struct resourceTable* resource);
void printNeeds(struct PCB* processBlock, struct resourceTable* resource, int processId, int flag);

//Blocked Queues' manipulation/query functs
int addQueue(int processId, int resType, int blocked[20][18], int *tail);
void printQueue(int r, int p, int blocked[r][p]);
int inQueue(int r, int p, int blocked[r][p], int resType, int processId);
int queueEmpty(int r, int p, int blocked[r][p], int resType);
int allQueuesEmpty(int r, int p, int blocked[r][p]);
int selectFromQueue(int blocked[], int *tail, int processId);

//Timing
void advanceClock(int *clockSecs, int *clockNanos);
double getTotalTime(int *clockSecs, int *clockNanos, double *time);
double genRandomDouble();//For comparing to time when creating processes

//Bit Vector
void set(int bv[], int i);//Set value in bitVector
int member(int bv[], int i);//Check if value in bitVector
void clearBit(int bv[], int i);//Clear the process from the process table
int bvFull(int bv[], int numProcesses);//Check if process table full
int bvEmpty(int bv[], int numProcesses);//Check if process table empty

//Shared Memory:
extern int shmidVal;
extern int shmidVal2;
extern int shmidVal3;
extern key_t key; //For shared memory
extern key_t key2; //For shared memory
extern key_t key3; //For shared memory

//Signaling:
void sigHandler(int mysignal);
void initSigHandler();

//Semaphores:
extern int semKey, semId;//for waitClock(), signalClock()
extern int semKeyR, semIdR;//For processes requesting resources
extern int semKeyCond, semIdCond;
struct sembuf operation;
struct sembuf removeP;
struct sembuf condition;
void waitClock();
void signalClock();
void waitRequest();//For when processes requesting resources
void signalRequest(int processRequesting);
void waitSem();
void signalSem();
void waitSem2();
void signalSem2();
void waitRemove();
void signalRemove();

#endif
/*$Author: o2-gray $
 *$Date: 2016/04/06 03:19:31 $
 *$Log: functs.h,v $
 *Revision 1.3  2016/04/06 03:19:31  o2-gray
 *Moved some vars to proceesBlock
 *
 *Revision 1.2  2016/03/31 03:37:20  o2-gray
 *Changed resource struct to have 2d arrays rather than making an array of structs.
 *
 *Revision 1.1  2016/03/30 04:03:19  o2-gray
 *Initial revision
 *
 */
