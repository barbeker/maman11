/*
 * ut.c
 *
 *  Created on: Apr 1, 2018
 *      Author: Bar Beker
 *      Student ID: 301518874
 */

#include "ut.h"
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>

#define ALARM_TIME_QUANTA 1
#define VTIME_TIME_QUANTA 10

static ucontext_t dummyUcontext;
static ut_slot_t  threadTable[MAX_TAB_SIZE];
static volatile tid_t currThreadIdx          = 0;
static volatile tid_t sapwnThreadsNum        = 0;
static volatile unsigned int threadTableSize = 0;

void Handler(int signal);
int  InitSignalHandler();
void ContextSwitcher();

void ContextSwitcher() {
    tid_t oldThreadIdx = currThreadIdx;
    tid_t newThreadIdx = (oldThreadIdx + 1) % sapwnThreadsNum;
    currThreadIdx = newThreadIdx;
    swapcontext(&threadTable[oldThreadIdx].uc, &threadTable[newThreadIdx].uc);}

int InitSignalHandler() {

    struct sigaction sa;
    struct itimerval itv;

    /* Initialize the data structures for SIGALRM handling. */
    sa.sa_flags = SA_RESTART;
    sigfillset(&sa.sa_mask);
    sa.sa_handler = Handler;

    /* set up vtimer for accounting */
    itv.it_interval.tv_sec  = 0;
    itv.it_interval.tv_usec = 10000;
    itv.it_value            = itv.it_interval;

    /* Install the signal handler*/
    if (sigaction(SIGVTALRM, &sa, NULL)       < 0  ||
        setitimer(ITIMER_VIRTUAL, &itv, NULL) < 0  ||
        sigaction(SIGALRM, &sa, NULL)         < 0  ) {
        perror("error on signal handler installation");
        return SYS_ERR;
}

    return 0;
}

int ut_init(int tab_size) {

    int tableSize = 0;
    if (InitSignalHandler()) {
        perror("error on signal handler initialization");
        return SYS_ERR; }

    /* Set thread table size */
    if ((tab_size >= MIN_TAB_SIZE) && (tab_size <= MAX_TAB_SIZE)) {
        tableSize = tab_size;
    } else {
        tableSize = MAX_TAB_SIZE;
    }

    /* allocate thread */
    threadTableSize = tableSize;
    currThreadIdx   = 0;

return 0;
}

tid_t ut_spawn_thread(void (*func)(int), int arg) {
    /* Increment spawn thread index */
    tid_t spawnThreadIdx = sapwnThreadsNum;

    /* check thread table is valid */
    if (threadTableSize == 0)             {
        perror("thread table size is 0");
        return SYS_ERR;
    }
    if (spawnThreadIdx > threadTableSize) {
        perror("thread table size is full");
        return TAB_FULL;
    }

    /* get and make thread context*/
    if (getcontext(&threadTable[spawnThreadIdx].uc) == -1) {
        perror("Error occurred on context fetching");
        return SYS_ERR;
    }

    /* allocate stack for thread */
    char* stack = (char*)malloc(sizeof(char) * STACKSIZE);
    if (!stack) {
        perror("Error occurred on allocating stack:");
        return SYS_ERR;
    }

    /* init thread args */
    threadTable[spawnThreadIdx].uc.uc_link          = &dummyUcontext;
    threadTable[spawnThreadIdx].uc.uc_stack.ss_sp   = stack;
    threadTable[spawnThreadIdx].uc.uc_stack.ss_size = (sizeof(char) * STACKSIZE);

    threadTable[spawnThreadIdx].vtime = 0;
    threadTable[spawnThreadIdx].func  = func;
    threadTable[spawnThreadIdx].arg   = arg;

    makecontext(&threadTable[spawnThreadIdx].uc, (void(*)(void))threadTable[spawnThreadIdx].func, 1, threadTable[spawnThreadIdx].arg);

    sapwnThreadsNum++;
return spawnThreadIdx;
}

void Handler(int signal) {
    if (SIGALRM == signal) {
        alarm(ALARM_TIME_QUANTA);
        ContextSwitcher();
    } else if (SIGVTALRM == signal){
        threadTable[currThreadIdx].vtime += VTIME_TIME_QUANTA;
    }
}

int ut_start(void) {
    /*check thread table is valid and that there are threads to run*/
    if (threadTableSize == 0 || sapwnThreadsNum == 0) {
        perror("no threads in thread table to run");
        return SYS_ERR;
    }

    /*Start running*/
    alarm(ALARM_TIME_QUANTA);
    swapcontext(&dummyUcontext, &threadTable[0].uc);
    return 0;
}

unsigned long ut_get_vtime(tid_t tid) {
    /*check thread is in thread table*/
    if (threadTableSize == 0 || tid >= threadTableSize || tid >= sapwnThreadsNum) {
        perror("there is no such thread in thread table");
        return SYS_ERR;
    }
    /*acquire thread running time */
    return threadTable[tid].vtime;
}
