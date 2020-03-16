#include "proj2.h"

// variables for shared memory
int *shActionNum,
    *hackPierCount,
    *serfPierCount,
    *onBoardCount;

int sharedActionNumId, hackPierCountID, serfPierCountID, onBoardCountID;

//semaphores
sem_t
    *writeMutex,
    *mutex,
    *serfsSem,
    *hackersSem,
    *waitOnBoard,
    *captainCanLeave,
    *boatCountMutex,
    *allOnBoard;

//output file
FILE *file;

int main(int argc, char *argv[])
{
    argsData args;
    pid_t pidMainProc;
    
    if(ProcessArgs(argc, argv ,&args)) return EXIT_FAILURE;
    if(InitResources()) return EXIT_FAILURE;

    // initialise shm counters
    *shActionNum=0;
    *hackPierCount=0;
    *serfPierCount=0;
    *onBoardCount=0;

    //create serf generator
    if ((pidMainProc=fork()) == -1 )
    {
        CleanResources();
        fprintf(stderr, "Fork call failed");
        return EXIT_FAILURE;   
    }

    if(pidMainProc == 0)
    {   // generate serfs here
        if(generateMembers(false, args))
        {
            CleanResources();
            return EXIT_FAILURE;
        }
        exit(0); 
    }
        
    //create hacker generator
    if ((pidMainProc=fork()) == -1 )
    {
        CleanResources();
        fprintf(stderr, "Fork call failed");
        return EXIT_FAILURE;   
    }

    if(pidMainProc == 0)
    {   // generate hackers here
        if(generateMembers(true,args))
        {
            CleanResources();
            return EXIT_FAILURE;
        }
        exit(0); 
    }
       
    // main process waits for 2 generators
    WaitAllChilds();

    if(CleanResources()) return EXIT_FAILURE;

    return EXIT_SUCCESS;      
}   

int ProcessArgs(int argc, char *argv[], argsData* data)
{
    // check if the ammount of args is correct
    if (argc != 7) 
    {
        fprintf(stderr, "Too few arguments!!! \n");
        return EXIT_FAILURE;
    }

    // check if all are integers
    char *test;
    int err= 0;

    data->personCount = strtoul(argv[1], &test , 10);
    if(*test != '\0') err++;
    data->hackerMaxGenTime = strtoul(argv[2], &test , 10); 
    if(*test != '\0')  err++;
    data->serfMaxGenTime = strtoul(argv[3], &test , 10);
    if(*test != '\0') err++;
    data->maxCruiseTime = strtoul(argv[4], &test , 10);
    if(*test != '\0') err++;
    data->maxRetryPierTime = strtoul(argv[5], &test , 10);
    if(*test != '\0') err++;
    data->pierCap = strtoul(argv[6], &test , 10);
    if(*test != '\0') err++;
    
    // check values limits
    if(data->personCount < 2 || data->personCount %2 != 0) err++;
    if(data->hackerMaxGenTime < 0 || data->hackerMaxGenTime > 2000) err++;
    if(data->serfMaxGenTime < 0 || data->serfMaxGenTime > 2000) err++;
    if(data->maxCruiseTime < 0 || data->maxCruiseTime > 2000) err++;
    if(data->maxRetryPierTime < 0 || data->maxRetryPierTime > 2000) err++;
    if(data->pierCap < 5) err++;

        if(err)
    {
        fprintf(stderr, "Wrong arguments!!! \n");
        return EXIT_FAILURE;
    }
        
    return EXIT_SUCCESS;
}

int InitResources()
{
     
    // create output file
    if((file = fopen("proj2.out", "w")) == NULL) {
    fprintf(stderr, "Error, failed to open output file\n");
    return EXIT_FAILURE;
    }

    //for correct writing (deactivate buffering)
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);
    setbuf(file, NULL);

    // initialisation of semaphores
    if
    (
        (writeMutex = mmap(NULL, semSIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0)) == MAP_FAILED ||
        (serfsSem = mmap(NULL, semSIZE , PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0)) == MAP_FAILED ||
        (captainCanLeave = mmap(NULL, semSIZE , PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0)) == MAP_FAILED ||
        (waitOnBoard = mmap(NULL, semSIZE , PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0)) == MAP_FAILED ||
        (hackersSem = mmap(NULL, semSIZE , PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0)) == MAP_FAILED||
        (mutex = mmap(NULL, semSIZE , PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0)) == MAP_FAILED||
        (boatCountMutex = mmap(NULL, semSIZE , PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0)) == MAP_FAILED||
        (allOnBoard = mmap(NULL, semSIZE , PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0)) == MAP_FAILED||

        sem_init(writeMutex, 1, 1)||
        sem_init(boatCountMutex, 1, 1)||
        sem_init(captainCanLeave, 1, 0)||
        sem_init(waitOnBoard, 1, 0)||
        sem_init(mutex, 1, 1)||
        sem_init(hackersSem, 1 , 0) ||
        sem_init(serfsSem, 1, 0)||
        sem_init(allOnBoard, 1, 0)
    )
    {
        fprintf(stderr, "Failed to init semaphores \n"); 
        return EXIT_FAILURE;
    }

     //init shared memory
     if 
     (
        (sharedActionNumId = shmget(IPC_PRIVATE, sizeof (int), IPC_CREAT | 0666)) == -1 ||
        (hackPierCountID = shmget(IPC_PRIVATE, sizeof (int), IPC_CREAT | 0666)) == -1 ||
        (onBoardCountID= shmget(IPC_PRIVATE, sizeof (int), IPC_CREAT | 0666)) == -1 ||
        (serfPierCountID = shmget(IPC_PRIVATE, sizeof (int), IPC_CREAT | 0666)) == -1 ||

        (shActionNum = (int *) shmat(sharedActionNumId, NULL, 0)) == NULL ||
        (hackPierCount = (int *) shmat(hackPierCountID, NULL, 0)) == NULL ||
        (onBoardCount = (int *) shmat(onBoardCountID, NULL, 0)) == NULL ||
        (serfPierCount = (int *) shmat(serfPierCountID, NULL, 0)) == NULL 
     ) 
    { 
        fprintf(stderr, "Failed to alocate shared memory \n"); 
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;  
}

int CleanResources()
{
    // close file 
    if(fclose(file))
    {
        fprintf(stderr, "Failed to close file");
        return EXIT_FAILURE;
    }

    // destroy semaphores
    if
    (
        sem_destroy(writeMutex) ||
        sem_destroy(waitOnBoard) ||
        sem_destroy(serfsSem) ||
        sem_destroy(hackersSem) ||
        sem_destroy(mutex)||
        sem_destroy(boatCountMutex)||
        sem_destroy(allOnBoard) ||
        sem_destroy(captainCanLeave)||

        munmap(writeMutex, semSIZE) ||
        munmap(waitOnBoard, semSIZE) ||
        munmap(serfsSem, semSIZE) ||
        munmap(hackersSem, semSIZE)||
        munmap(boatCountMutex, semSIZE)||
        munmap(mutex, semSIZE)||
        munmap(allOnBoard, semSIZE) ||
        munmap(captainCanLeave, semSIZE)
    )
    {
        fprintf(stderr, "Failed to destroy semaphores \n"); 
        return EXIT_FAILURE;
    }

    // dealoc shared mem
    if 
    (
        shmctl(sharedActionNumId, IPC_RMID, NULL) == -1 ||
        shmctl(serfPierCountID, IPC_RMID, NULL) == -1 ||
        shmctl(onBoardCountID, IPC_RMID, NULL) == -1 ||
        shmctl(hackPierCountID, IPC_RMID, NULL) == -1 ||

        shmdt(shActionNum) == -1 ||
        shmdt(serfPierCount) == -1  ||
        shmdt(onBoardCount) == -1  ||
        shmdt(hackPierCount) == -1 
    )
    {
        fprintf(stderr, "Failed to dealocate shared memory");
        return EXIT_FAILURE;
    }  

    return EXIT_SUCCESS;
}

void WaitAllChilds()
{
    while (true) 
    {
        if(wait(NULL) == -1) break;
    }
}


// waits random time from 0 t0 maxRand (in msec)
void WaitRand(int maxRand)
{
    if(maxRand == 0) return;
    srand (time(NULL));
    int waitTime = (rand() % maxRand) +1;
    // * 1000 because of conversion to usec
    usleep(waitTime*1000);
}

int generateMembers(bool isHacker, argsData args)
{
    pid_t pidPerson;
        
    //generate all members of given category
    for(int i=1; i <= args.personCount ; i++)
    {
        // fork child and check syscall sucess
        pidPerson=fork();
        if(pidPerson == -1)
        {
            CleanResources();
            fprintf(stderr, "Fork call failed");
            return EXIT_FAILURE;   
        }
            
        //if this is child process
        if(pidPerson == 0)
        {   
            //do stuff in member process
            Member(i, args, isHacker);
            exit(0);
        }
            
        // time between generating processes
        WaitRand(args.hackerMaxGenTime); 
    }
        // generator waits for all generated child processes
        WaitAllChilds(); 
        return EXIT_FAILURE;
}

void sem_post_n_times(sem_t *sem, unsigned int n)
{
    for(unsigned i=0 ; i<n; i++)
    {
        sem_post(sem);
    }
}

///////////////////////////////// STAF IN MEMBER ///////////////////////////////
void Member(int currentId,argsData args, bool isHacker)
{ 
    // role specific variables (by default role is set to serf)
    char role[] = "SERF";
    sem_t *memberSem = serfsSem;
    int *memberPierCount = serfPierCount;

    // if role is hakcer, role specific variables are adapted
    if(isHacker)
    {
        strcpy(role, "HACK");
        memberSem = hackersSem;
        memberPierCount = hackPierCount;
    }

    bool isCaptain=false;

    sem_wait(writeMutex);
    fprintf(outFILE, "%d  :%s %d  :starts\n", ++(*shActionNum), role, currentId);  
    sem_post(writeMutex);

    sem_wait(mutex);
    
    // try to enter pier
    while((*hackPierCount+*serfPierCount) == args.pierCap )
    {
        sem_wait(writeMutex);
        fprintf(outFILE, "%d  :%s %d  :leaves queue  :%d  :%d\n",++(*shActionNum),role, currentId, *hackPierCount, *serfPierCount);
        sem_post(writeMutex);

        WaitRand(args.maxRetryPierTime);

        sem_wait(writeMutex);
        fprintf(outFILE, "%d  :%s %d  :isback\n",++(*shActionNum),role, currentId);
        sem_post(writeMutex);
    }

    // process count itself as a waiting process on pier
    (*memberPierCount)++;
    sem_wait(writeMutex);
    fprintf(outFILE, "%d  :%s %d  :waits  :%d  :%d\n",++(*shActionNum),role, currentId, *hackPierCount, *serfPierCount);
    sem_post(writeMutex);

    //try to assamble suitable group (4 of the same cat or 2 from one and 2 from the other)
    if((*memberPierCount) >= 4)
    {
        // 4 members of current category can board
        sem_post_n_times(memberSem, 4);

        //change amount of waiting hackers
        (*memberPierCount)-=4;
        isCaptain=true;          
    }
    else if (((*serfPierCount) >= 2) && ((*hackPierCount) >= 2))
    {
        // 2 hackers and 2 serfs can board
        sem_post_n_times(hackersSem, 2);
        sem_post_n_times(serfsSem, 2);

        *serfPierCount-=2;
        (*hackPierCount)-=2;
        isCaptain=true;
    }
    else
    {
        // current member wasn't able to create group, so other is allowed to try
        sem_post(mutex);
    }

    // try to board boat
    sem_wait(memberSem);

    //process counts itself as boarded and check if boarding is done
    sem_wait(boatCountMutex);
    if(++(*onBoardCount) == 4) sem_post(allOnBoard);
    sem_post(boatCountMutex);

    if(isCaptain)
    {
        // captain waits untill all members are on board
        sem_wait(allOnBoard);

        sem_wait(writeMutex);
            fprintf(outFILE, "%d  :%s %d  :boards  :%d  :%d \n",++(*shActionNum),role, currentId, *hackPierCount, *serfPierCount);
        sem_post(writeMutex);

        //simulate cruise
        WaitRand(args.maxCruiseTime);
        
        //allow members to leave boat
        sem_post_n_times(waitOnBoard, 3);

        //wait until all members are out
        sem_wait(captainCanLeave);
        
        sem_wait(writeMutex);
        fprintf(outFILE, "%d  :%s %d  :captain exits :%d  :%d \n", ++(*shActionNum),role, currentId, *hackPierCount, *serfPierCount);
        sem_post(writeMutex);

        sem_wait(boatCountMutex);
        // decrement on board counter 
        (*onBoardCount)--;
        sem_post(boatCountMutex);
        
        //allow other member to bocame a captain and board group
        sem_post(mutex);
    }
    else
    {                
        //if this is a member, waits on board until cruise is over
        sem_wait(waitOnBoard);   

            sem_wait(writeMutex);                    
            fprintf(outFILE, "%d  :%s %d  :member exits :%d  :%d \n",++(*shActionNum), role, currentId, *hackPierCount, *serfPierCount);
            sem_post(writeMutex);
        
            //leave boat and if there is only captain left, let him leave too
            sem_wait(boatCountMutex);
            if(--(*onBoardCount) == 1) sem_post(captainCanLeave);
            sem_post(boatCountMutex);
    } 
}




