#define _GNU_SOURCE
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>

#define TEXTSZ 256
struct msgbuf
{
  long mtype;
  char mtext[TEXTSZ];
};

static int my_qid = -1;
static int srv_qid;
static int my_id = -1;

static void bye(int s)
{
  if (my_qid != -1)
    msgctl(my_qid, IPC_RMID, NULL);
  printf("bezpiecznie zamknięto");
  exit(0);
}

void *inbox(void *arg)
{
  struct msgbuf buf;
  while (1)
  {
    if (msgrcv(my_qid, &buf, TEXTSZ, 0, 0) == -1)
      continue;
    if (buf.mtype == 3)
    { /* ID od serwera */
      my_id = atoi(buf.mtext);
      printf("== masz id %d ==\n", my_id);
    }
    else if (buf.mtype == 4)
    {
      printf("%s\n", buf.mtext);
    }
  }
}

int main(void)
{
  signal(SIGINT, bye);

  // otwarcie kolejki serwera
  key_t srv_key = ftok(".", 'C');
  srv_qid = msgget(srv_key, 0);
  if (srv_qid == -1)
  {
    perror("msgget srv");
    return 1;
  }

  // kolejka klientcka
  key_t my_key = ftok(".", getpid() & 0xFF);
  my_qid = msgget(my_key, IPC_CREAT | 0600);

  // poinformowanie serwera
  struct msgbuf init = {.mtype = 1};
  snprintf(init.mtext, TEXTSZ, "%d", my_key);
  msgsnd(srv_qid, &init, strlen(init.mtext) + 1, 0);

  // odbieranie (nowy wątek)
  pthread_t thr;
  pthread_create(&thr, NULL, inbox, NULL);

  // wysyłanie
  char line[TEXTSZ - 20];
  while (fgets(line, sizeof line, stdin))
  {
    if (my_id < 0)
      continue; // czekamy na ID
    struct msgbuf out = {.mtype = 2};
    snprintf(out.mtext, TEXTSZ, "%d %s", my_id, line);
    msgsnd(srv_qid, &out, strlen(out.mtext) + 1, 0);
  }
}
