#ifndef PROJ2_H
#define PROJ2_H

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdbool.h>
#include <string.h>

#define semSIZE  sizeof(sem_t)

// for debuging purposes
#define outFILE file

// struct for storing values ovtained in args
typedef struct argsData
{
    int personCount;
    int hackerMaxGenTime;
    int serfMaxGenTime;
    int maxCruiseTime;
    int maxRetryPierTime;
    int pierCap;
}
argsData;

int ProcessArgs(int argc, char *argv[], argsData* data);
int InitResources();
int CleanResources();
void Member(int currentId, argsData args, bool isHacker);
void WaitAllChilds();
void WaitRand(int mTime);
int generateMembers(bool isHacker, argsData args);
void sem_post_n_times(sem_t *sem, unsigned int incBy);

#endif /*PROJ2_H*/
