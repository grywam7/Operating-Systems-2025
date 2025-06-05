#ifndef COMMON_H
#define COMMON_H

#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define QUEUE_SIZE 16
#define JOB_LEN 10

#define FTOK_PATH "."
#define FTOK_PROJ 'P'

// struktura pamięci współdzielonej
struct Shared
{
  int head, tail;
  char jobs[QUEUE_SIZE][JOB_LEN + 1];
};

// semafory
enum
{
  SEM_MUTEX = 0,
  SEM_EMPTY = 1,
  SEM_FULL = 2
};

// operacje na semaforach
static inline void sem_op(int semid, int num, int delta)
{
  struct sembuf op = {.sem_num = num, .sem_op = delta, .sem_flg = 0};
  if (semop(semid, &op, 1) == -1)
  {
    perror("semop");
    exit(1);
  }
}
#define P(num, id) sem_op(id, num, -1)
#define V(num, id) sem_op(id, num, +1)

#endif