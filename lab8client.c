#include "lab8common.h"
#include <time.h>

int main(void)
{
  srand(getpid());

  key_t key = ftok(FTOK_PATH, FTOK_PROJ);
  int shm_id = shmget(key, 0, 0);
  int sem_id = semget(key, 0, 0);
  if (shm_id == -1 || sem_id == -1)
  {
    fputs("Brak serwera!\n", stderr);
    return 1;
  }

  struct Shared *shm = shmat(shm_id, NULL, 0);

  while (1)
  {
    // stwórz zlecenie
    char txt[JOB_LEN + 1];
    for (int i = 0; i < JOB_LEN; ++i)
      txt[i] = 'a' + rand() % 26;
    txt[JOB_LEN] = '\0';

    // wstaw do kolejki
    P(SEM_EMPTY, sem_id);
    P(SEM_MUTEX, sem_id);

    strcpy(shm->jobs[shm->tail], txt);
    shm->tail = (shm->tail + 1) % QUEUE_SIZE;

    V(SEM_MUTEX, sem_id);
    V(SEM_FULL, sem_id);

    printf("[USER %d] zlecono \"%s\"\n", getpid(), txt);
    fflush(stdout);

    sleep(1 + rand() % 6);
  }
}
