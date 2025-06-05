#include "lab8common.h"

int main(void)
{
  key_t key = ftok(FTOK_PATH, FTOK_PROJ);
  int shm_id = shmget(key, 0, 0);
  int sem_id = semget(key, 0, 0);
  if (shm_id == -1 || sem_id == -1)
  {
    fputs("Brak serwera!\n", stderr);
    return 1;
  }

  struct Shared *shm = shmat(shm_id, NULL, 0);
  int id = getpid();

  while (1)
  {

    // pobieranie zlecenia
    P(SEM_FULL, sem_id);
    P(SEM_MUTEX, sem_id);

    char job[JOB_LEN + 1];
    strcpy(job, shm->jobs[shm->head]);
    shm->head = (shm->head + 1) % QUEUE_SIZE;

    V(SEM_MUTEX, sem_id);
    V(SEM_EMPTY, sem_id);

    // wypisywanie
    printf("[PRN %d] start \"%s\"\n", id, job);
    fflush(stdout);
    for (int i = 0; i < JOB_LEN; ++i)
    {
      putchar(job[i]);
      fflush(stdout);
      sleep(1);
    }
    puts("");
  }
}
