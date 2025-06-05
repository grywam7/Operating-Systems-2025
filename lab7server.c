#define _GNU_SOURCE
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#define MAX_CLIENTS 32
#define TEXTSZ 256

struct msgbuf
{
  long mtype;
  char mtext[TEXTSZ];
};

static int main_qid = -1;
static int client_qid[MAX_CLIENTS]; // ­‑1 = wolne

static void cleanup(int s)
{
  if (main_qid != -1)
    msgctl(main_qid, IPC_RMID, NULL);
  puts("\n[server] zabity bezpiecznie (nie to co dziewczyna na UW :) )");
  exit(0);
}

int main(void)
{
  signal(SIGINT, cleanup);

  // kolejka serwera
  key_t key = ftok(".", 'C');
  main_qid = msgget(key, IPC_CREAT | 0666);
  if (main_qid == -1)
  {
    perror("msgget");
    return 1;
  }
  memset(client_qid, -1, sizeof(client_qid));

  puts("[server] started");

  // odbieranie wiadomości
  struct msgbuf buf;
  while (1)
  {
    if (msgrcv(main_qid, &buf, TEXTSZ, 0, 0) == -1)
      continue;

    if (buf.mtype == 1) // nowy klient
    {
      int id = -1;
      for (int i = 0; i < MAX_CLIENTS; ++i)
        if (client_qid[i] == -1)
        {
          id = i;
          break;
        }
      if (id == -1) // brak miejsca
        continue;

      key_t k = (key_t)strtol(buf.mtext, NULL, 10);
      int q = msgget(k, 0); // otwórz kolejkę klienta
      if (q == -1)
        continue;

      client_qid[id] = q;
      // ID do klienta
      struct msgbuf out = {.mtype = 3};
      snprintf(out.mtext, TEXTSZ, "%d", id);
      msgsnd(q, &out, strlen(out.mtext) + 1, 0);
      printf("[server] klient %d podłączony (qid=%d)\n", id, q);
    }
    else if (buf.mtype == 2)
    { // normalna wiadomość
      int from;
      char *msg = strchr(buf.mtext, ' ');
      if (!msg)
        continue;
      *msg++ = 0;
      from = atoi(buf.mtext);

      // wyslij wiad do reszty klientow
      for (int i = 0; i < MAX_CLIENTS; ++i)
        if (client_qid[i] != -1 && i != from)
        {
          struct msgbuf out = {.mtype = 4};
          snprintf(out.mtext, TEXTSZ, "[%d] %s", from, msg);
          msgsnd(client_qid[i], &out, strlen(out.mtext) + 1, 0);
        }
      printf("[server] od %d: %s\n", from, msg);
    }
  }
}
