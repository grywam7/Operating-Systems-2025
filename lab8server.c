#include "lab8common.h"
#include <signal.h>

static int shm_id = -1, sem_id = -1;
static struct Shared *shm = NULL;

static void cleanup(int s)
{
  if (shm)
    shmdt(shm);
  if (shm_id != -1)
    shmctl(shm_id, IPC_RMID, NULL);
  if (sem_id != -1)
    semctl(sem_id, 0, IPC_RMID);
  puts("\n[SERVER] zasoby IPC usunięte - bye.");
  exit(0);
}

int main(void)
{
  key_t key = ftok(FTOK_PATH, FTOK_PROJ);
  if (key == -1)
  {
    perror("ftok");
    return 1;
  }

  // pamięć współdzielona
  shm_id = shmget(key, sizeof(struct Shared), IPC_CREAT | 0666);
  if (shm_id == -1)
  {
    perror("shmget");
    return 1;
  }
  shm = shmat(shm_id, NULL, 0);
  shm->head = shm->tail = 0;

  // semafory
  sem_id = semget(key, 3, IPC_CREAT | 0666);
  if (sem_id == -1)
  {
    perror("semget");
    return 1;
  }
  semctl(sem_id, SEM_MUTEX, SETVAL, 1);
  semctl(sem_id, SEM_EMPTY, SETVAL, QUEUE_SIZE);
  semctl(sem_id, SEM_FULL, SETVAL, 0);

  signal(SIGINT, cleanup);
  printf("[SERVER] gotowe - Ctrl-C aby zakończyć.\n");

  while (1)
    pause();
}
