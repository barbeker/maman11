/*
 * binsem.c
 *
 *  Created on: Apr 6, 2018
 *      Author: Bar Beker
 *      Student ID: 301518874
 */
#include <signal.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include "binsem.h"

#define SYS_ERR -1

void binsem_init(sem_t *s, int init_val) {
    /* semaphore pointer is invalid */
    if (s == NULL) { return; }
    int initVal = (0 == init_val)? 0 : 1;
    *s = initVal;
}

void binsem_up(sem_t *s) {
    /* semaphore pointer is invalid */
    if (s == NULL) { return; }
    *s = 1;
}

int binsem_down(sem_t *s) {
    /* semaphore pointer is invalid */
    if (s == NULL) { return -1; }
    while (0 == xchg(s, 0)) {
        kill(getpid(), SIGALRM);
        if (errno != 0) { return SYS_ERR; }
    }
    return 0;
}
